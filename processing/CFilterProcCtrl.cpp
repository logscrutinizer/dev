/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CDebug.h"
#include "CFilterProcCtrl.h"
#include "CProgressCtrl.h"
#include "CMemPool.h"

/***********************************************************************************************************************
*   FileIndex_To_MemRef
***********************************************************************************************************************/
inline char *FileIndex_To_MemRef(int64_t *fileIndex_p, int64_t *workMemFileIndex_p, char *WorkMem_p)
{
    return (static_cast<char *>(WorkMem_p + (*fileIndex_p - *workMemFileIndex_p)));
}

/* Main for v-table generation */
CFilterThreadConfiguration::~CFilterThreadConfiguration() {}

/***********************************************************************************************************************
*   thread_Process
***********************************************************************************************************************/
void CFilterThread::thread_Process(CThreadConfiguration *config_p)
{
    CFilterThreadConfiguration *filterConfig_p = static_cast<CFilterThreadConfiguration *>(config_p);

    m_isStopped = false;

    int TIA_Index = filterConfig_p->m_start_TIA_index; /* use local variable for quicker access */
    int filterIndex;
    const int stop_TIA_Index = filterConfig_p->m_stop_TIA_Index;
    const int TIA_step = filterConfig_p->m_TIA_step;
    int progressCount = PROGRESS_COUNTER_STEP;
    bool match = false;
    Match_Description_t matchDescr;

    memset(&matchDescr, 0, sizeof(Match_Description_t));

    int numOfFilterItems = filterConfig_p->m_numOfFilterItems;

    progressCount = PROGRESS_COUNTER_STEP;

    const int colClipStart = g_cfg_p->m_Log_colClip_Start > 0 ? g_cfg_p->m_Log_colClip_Start : 0;

    /*compensate for the moved start */
    const int colClipEnd = g_cfg_p->m_Log_colClip_End > 0 ? g_cfg_p->m_Log_colClip_End - colClipStart : 0;

    while (TIA_Index < stop_TIA_Index && !g_processingCtrl_p->m_abort) {
        matchDescr.textLength = filterConfig_p->m_TIA_p->textItemArray_p[TIA_Index].size;
        matchDescr.text_p = FileIndex_To_MemRef(&filterConfig_p->m_TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                                &filterConfig_p->m_chunkDescr.fileIndex,
                                                filterConfig_p->m_workMem_p);
        match = false;
        filterIndex = 0;

        --progressCount;
        if (progressCount <= 0) {
            g_processingCtrl_p->StepProgressCounter(m_threadIndex);
            progressCount = PROGRESS_COUNTER_STEP;
        }

        if (colClipStart > 0) {
            matchDescr.textLength -= colClipStart;
            matchDescr.text_p += colClipStart;
        }

        if (colClipEnd > 0) {
            matchDescr.textLength = colClipEnd;
        }

        while (filterIndex < numOfFilterItems && !match) {
            packedFilterItem_t *packedFilterItem_p = &filterConfig_p->m_packedFilterItems_p[filterIndex];
            const bool regExp = packedFilterItem_p->filterRef_p->m_regexpr;
            if (regExp || (packedFilterItem_p->length <= matchDescr.textLength)) {
                matchDescr.filter_p = packedFilterItem_p->start_p;
                if (regExp) {
                    matchDescr.filterLength = packedFilterItem_p->length;
                    matchDescr.regexp_database =
                        filterConfig_p->m_regexp_database_array[packedFilterItem_p->m_regExpLUTIndex];
                    matchDescr.regexp_scratch =
                        filterConfig_p->m_regexp_scratch_array[packedFilterItem_p->m_regExpLUTIndex];
                    match = thread_Match_RegExp_HyperScan(&matchDescr);
                } else if (packedFilterItem_p->filterRef_p->m_caseSensitive) {
                    /*CASE */
                    matchDescr.filterLength = packedFilterItem_p->length - 1;
                    match = thread_Match_CS(&matchDescr);
                } else {
                    matchDescr.filterLength = packedFilterItem_p->length - 1;
                    match = thread_Match(&matchDescr);
                }
            }

            ++filterIndex;
        }

        --filterIndex;
        filterConfig_p->m_FIRA_p[TIA_Index].LUT_index = 0;

        if (match) {
            /* Index 0 means none found, hence first filter starts at index +1 */
            filterConfig_p->m_FIRA_p[TIA_Index].LUT_index = static_cast<uint8_t>(filterIndex + 1);
        }

        TIA_Index += TIA_step;
    } /* while */

    (void)thread_ProcessingDone();
}

/***********************************************************************************************************************
*   FilterInit
***********************************************************************************************************************/
void CFilterThreadConfiguration::FilterInit(FIR_t *FIRA_p, packedFilterItem_t *packedFilterItems_p,
                                            int numOfFilterItems)
{
    m_FIRA_p = FIRA_p;
    m_packedFilterItems_p = packedFilterItems_p;
    m_numOfFilterItems = numOfFilterItems;

    if (-1 == m_numOfRegExpFilters) {
        m_numOfRegExpFilters = 0;

        /* Count number of regex based filters */
        for (int index = 0; index < m_numOfFilterItems; ++index) {
            if (m_packedFilterItems_p[index].filterRef_p->m_regexpr) {
                ++m_numOfRegExpFilters;
            }
        }

        /* Assign the RegEx filter, one for each, this is done for each thread
         * If there are RegEx based filters, create an array an initialize each filter (assign the regEx) */

        if (m_numOfRegExpFilters > 0) {
            m_regexp_database_array = new hs_database_t *[static_cast<uint32_t>(m_numOfRegExpFilters)];
            m_regexp_scratch_array = new hs_scratch_t *[static_cast<uint32_t>(m_numOfRegExpFilters)];

            /* Create a set of database and scratch for each filter (in each configuration objects) */
            for (int index = 0; index < m_numOfFilterItems; index++) {
                auto packedFilter_p = &m_packedFilterItems_p[index];
                if (-1 != m_packedFilterItems_p[index].m_regExpLUTIndex) {
                    hs_compile_error_t *compile_err;
                    hs_database_t *database = nullptr;
                    hs_scratch_t *scratch = nullptr;

                    if (hs_compile_ext_multi(&packedFilter_p->start_p,
                                             &REGEXP_HYPERSCAN_FLAGS,
                                             nullptr, /*ids*/
                                             nullptr, /*ext*/
                                             1, /*elements*/
                                             HS_MODE_BLOCK, /*mode*/
                                             nullptr, /*platform*/
                                             &database,
                                             &compile_err) != HS_SUCCESS) {
                        TRACEX_I(QString("RegExp failed %1").arg(compile_err->message))
                        g_processingCtrl_p->AddProgressInfo(QString("Regular expression contains error: %1")
                                                                .arg(packedFilter_p->start_p));
                        g_processingCtrl_p->m_abort = true;
                        hs_free_compile_error(compile_err);
                    }

                    hs_error_t error;
                    if ((error = hs_alloc_scratch(database, &scratch)) != HS_SUCCESS) {
                        TRACEX_I(QString("ERROR: Unable to allocate scratch space. "
                                         "Exiting. Code:%1").arg(error))
                    }
                    m_regexp_database_array[packedFilter_p->m_regExpLUTIndex] = database;
                    m_regexp_scratch_array[packedFilter_p->m_regExpLUTIndex] = scratch;
                }
            }
        }
    }
}

/***********************************************************************************************************************
*   StartProcessing
***********************************************************************************************************************/
void CFilterProcCtrl::StartProcessing(QFile *qFile_p, char *workMem_p, int64_t workMemSize, TIA_t *TIA_p, FIR_t *FIRA_p,
                                      int numOfTextItems, QList<CFilterItem *> *filterItems_p,
                                      CFilterItem **filterItem_LUT_p, FilterExecTimes_t *execTimes_p,
                                      int priority, int colClip_Start, int colClip_End,
                                      int rowClip_Start, int rowClip_End, int *totalFilterMatches_p,
                                      int *totalExcludeFilterMatches_p, QList<int> *bookmarkList_p, bool incremental)
{
    m_qfile_p = qFile_p;
    m_workMem_p = workMem_p;
    m_workMemSize = workMemSize;

    m_TIA_p = TIA_p;
    m_FIRA_p = FIRA_p;
    m_totalNumOf_DB_TextItems = numOfTextItems;
    m_filterItems_p = filterItems_p;
    m_filterItem_LUT_p = filterItem_LUT_p;
    m_execTimes_p = execTimes_p;
    m_priority = priority;
    m_bookmarkList_p = bookmarkList_p;
    m_incremental = incremental;

    if (colClip_Start >= 0) {
        m_colClip_StartEnabled = true;
        m_colClip_Start = colClip_Start;
    } else {
        m_colClip_StartEnabled = false;
        m_colClip_Start = 0;
    }

    if (colClip_End > 0) {
        m_colClip_EndEnabled = true;
        m_colClip_End = colClip_End;
    } else {
        m_colClip_EndEnabled = false;
        m_colClip_End = 0;
    }

    if (rowClip_Start >= 0) {
        m_startRow = rowClip_Start + (incremental ? 0 : 1);
    } else {
        m_startRow = 0;
    }

    if (rowClip_End > 0) {
        m_endRow = rowClip_End - 1;
    } else {
        m_endRow = m_totalNumOf_DB_TextItems - 1;
    }

    m_numOfRegExpFilters = -1;   /* not initialized yet */

    m_fileSize = m_qfile_p->size();

    /* If incremental, check if the last row was filtered, since we are going to filter it again we must exclude it from
     * the present count of filter matches and excludes. */
    if (m_incremental) {
        const auto LUT_Index = m_FIRA_p[m_startRow].LUT_index;
        if (0 != LUT_Index) {
            /* Last row (first for this filtering) was filtered, compensate... */
            if (m_filterItem_LUT_p[LUT_Index]->m_exclude) {
                (*totalExcludeFilterMatches_p) = (*totalExcludeFilterMatches_p) > 0 ?
                                                 (*totalExcludeFilterMatches_p) - 1 : 0;
            } else {
                (*totalFilterMatches_p) = (*totalFilterMatches_p) > 0 ?
                                          (*totalFilterMatches_p) - 1 : 0;
            }
        }
    } else {
        *totalFilterMatches_p = 0;
        *totalExcludeFilterMatches_p = 0;
    }

    m_totalFilterMatches = *totalFilterMatches_p;
    m_totalExcludeFilterMatches = *totalExcludeFilterMatches_p;

    if (m_execTimes_p != nullptr) {
        memset(m_execTimes_p, 0, sizeof(*m_execTimes_p));
    }

    if ((filterItems_p->count() == 0) && m_bookmarkList_p->isEmpty()) {
        ClearFilterRefs();
        g_processingCtrl_p->SetFail();
        g_processingCtrl_p->AddProgressInfo(QString("Filtering completed, no matches, NO filters..."));
        g_processingCtrl_p->AddProgressInfo(QString("Please add filter items before filtering"));
        return;
    }

    ClearFilterRefs();

    g_processingCtrl_p->AddProgressInfo(QString("Packing filters"));

    /* Might be 0 if only bookmarks */
    if (filterItems_p->count() > 0) {
        PackFilters(); /* Put all filter texts together to minimize cache misses */

        g_processingCtrl_p->AddProgressInfo(QString("Start filtering"));

        m_timeExec.Restart();

        CFileProcBase::Start(m_qfile_p, workMem_p, workMemSize, m_TIA_p, priority, m_startRow, m_endRow, false);
    } else {
        WrapUp();
    }

    *totalFilterMatches_p = m_totalFilterMatches;
    *totalExcludeFilterMatches_p = m_totalExcludeFilterMatches;
}

/***********************************************************************************************************************
*   StartOneLine
***********************************************************************************************************************/
void CFilterProcCtrl::StartOneLine(TIA_t *TIA_p, FIR_t *FIRA_p, int row, char *text_p, int textLength,
                                   QList<CFilterItem *> *filterItems_p, CFilterItem **filterItem_LUT_p,
                                   int *totalFilterMatches_p, int *totalExcludeFilterMatches_p,
                                   QList<int> *bookmarkList_p, bool isBookmarkRemove)
{
    bool match = false;
    Match_Description_t matchDescr;
    uint8_t filterIndex = 0;

    m_execTimes_p = nullptr;
    m_bookmarkList_p = bookmarkList_p;

    m_TIA_p = TIA_p;
    m_FIRA_p = FIRA_p;
    m_totalNumOf_DB_TextItems = TIA_p->rows;
    m_filterItems_p = filterItems_p;
    m_filterItem_LUT_p = filterItem_LUT_p;
    m_priority = 0;

    memset(&matchDescr, 0, sizeof(Match_Description_t));

    m_FIRA_p[row].LUT_index = 0;  /* Clear out the current filter (to be replaced) */

    /* Number of filters (which contains filterItems) */
    if (filterItems_p->count() > 0) {
        PackFilters();  /* Put all filter texts together to minimize cache misses -> m_numOfFilterItems */

        matchDescr.textLength = textLength;
        matchDescr.text_p = text_p;

        while (filterIndex < m_numOfFilterItems && !match) {
            packedFilterItem_t *packedFilterItem_p = &m_packedFilterItems_p[filterIndex];
            const bool regExp = m_packedFilterItems_p[filterIndex].filterRef_p->m_regexpr;
            matchDescr.filter_p = packedFilterItem_p->start_p;

            if (regExp) {
                hs_compile_error_t *compile_err;
                hs_database_t *database;
                hs_scratch_t *scratch;

                matchDescr.filterLength = packedFilterItem_p->length;

                TRACEX_I(QString("-pf %1").arg(packedFilterItem_p->start_p))
                if (hs_compile(packedFilterItem_p->start_p,
                               REGEXP_HYPERSCAN_FLAGS,
                               HS_MODE_BLOCK,
                               nullptr,
                               &database,
                               &compile_err) != HS_SUCCESS) {
                    TRACEX_I(QString("RegExp failed %1").arg(compile_err->message))
                    g_processingCtrl_p->AddProgressInfo(QString("Regular expression contains error: %1")
                                                            .arg(packedFilterItem_p->start_p));
                    g_processingCtrl_p->m_abort = true;
                    hs_free_compile_error(compile_err);
                }

                if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
                    TRACEX_I(QString("ERROR: Unable to allocate scratch space. Exiting."))
                    fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.");
                }

                matchDescr.regexp_database = database;
                matchDescr.regexp_scratch = scratch;
                match = thread_Match_RegExp_HyperScan(&matchDescr);
                hs_free_database(database);
                hs_free_scratch(scratch);
            } else if (m_packedFilterItems_p[filterIndex].filterRef_p->m_caseSensitive) {
                matchDescr.filterLength = packedFilterItem_p->length - 1; /* index is compared to filter length...
                                                                           * length is +1 */
                match = thread_Match_CS(&matchDescr);
            } else {
                matchDescr.filterLength = packedFilterItem_p->length - 1; /* index is compared to filter length...
                                                                           * length is +1 */
                match = thread_Match(&matchDescr);
            }

            ++filterIndex;
        }

        --filterIndex;

        m_FIRA_p[row].LUT_index = 0;

        if (match) {
            /* Index 0 means none found, hence first filter starts at index +1 */
            m_FIRA_p[row].LUT_index = (filterIndex + 1);
        }
    }

    if (!isBookmarkRemove) {
        DecorateFIRA();                         /* Add bookmarks */
    }

    /* Each filtered line is provided with an index used for presentation (setup m_totalFilterMatches) */
    NumerateFIRA();

    *totalFilterMatches_p = m_totalFilterMatches;
    *totalExcludeFilterMatches_p = m_totalExcludeFilterMatches;
}

/***********************************************************************************************************************
*   ConfigureThread
***********************************************************************************************************************/
bool CFilterProcCtrl::ConfigureThread(CThreadConfiguration *config_p, Chunk_Description_t *chunkDescription_p,
                                      int32_t threadIndex)
{
    static_cast<CFilterThreadConfiguration *>(config_p)->FilterInit(m_FIRA_p, m_packedFilterItems_p,
                                                                    m_numOfFilterItems);

    CFileProcBase::ConfigureThread(config_p, chunkDescription_p, threadIndex); /* Use the default initialization */
    return true;
}

/***********************************************************************************************************************
*   CreateConfigurationObject
***********************************************************************************************************************/
CThreadConfiguration *CFilterProcCtrl::CreateConfigurationObject(void)
{
    return static_cast<CThreadConfiguration *>(new CFilterThreadConfiguration());
}

/***********************************************************************************************************************
*   WrapUp
***********************************************************************************************************************/
void CFilterProcCtrl::WrapUp(void)
{
    if (!g_processingCtrl_p->m_abort) {
        g_processingCtrl_p->AddProgressInfo(QString("Post-process filtering"));

        /* Add bookmarks (will be overriden by filter matches) */
        DecorateFIRA();

        /* Each filtered line is provided with an index used for presentation (setup m_totalFilterMatches) */
        NumerateFIRA();
    } else {
        ClearFilterRefs();
        g_processingCtrl_p->AddProgressInfo(QString("Filtering aborted, all cleaned"));
        m_totalFilterMatches = 0;
        m_totalExcludeFilterMatches = 0;
    }

    TRACEX_ENABLE_WINDOW()

    if (m_filterStrings_p != nullptr) {
        VirtualMem::Free(m_filterStrings_p);
    }

    if (m_execTimes_p != nullptr) {
        m_execTimes_p->totalFilterTime = m_timeExec.ms();
    }

    if (g_processingCtrl_p != nullptr) {
        g_processingCtrl_p->AddProgressInfo(QString("Filtering done"));

        if (g_processingCtrl_p->m_abort) {
            g_processingCtrl_p->SetFail();
            g_processingCtrl_p->AddProgressInfo(QString("Filtering aborted"));
        } else if (m_totalFilterMatches == 0) {
            g_processingCtrl_p->SetFail();
            g_processingCtrl_p->AddProgressInfo(QString("Filtering completed, no matches"));
        } else {
            g_processingCtrl_p->SetSuccess();
            g_processingCtrl_p->AddProgressInfo(QString("Filtering completed"));
        }
    }
}

/***********************************************************************************************************************
*   PackFilters
* The idea it to move require filter content, into a packed (not zipped) structure, to minimize cache-misses.
***********************************************************************************************************************/
void CFilterProcCtrl::PackFilters(void)
{
    int64_t totalFilterTextSize = 0;
    char *destMem_p = nullptr;
    packedFilterItem_t *packedfilterItem_p;
    CTimeMeas timeExec;
    QList<CFilter *>::iterator filterIter;
    QList<CFilterItem *>::iterator filterItemIter;

    for (auto& filterItem_p : *m_filterItems_p) {
        TRACEX_D(QString("Filter: %1 regexp:%2 case:%3")
                     .arg(filterItem_p->m_start_p).arg(filterItem_p->m_regexpr)
                     .arg(filterItem_p->m_caseSensitive))
        totalFilterTextSize += static_cast<uint64_t>(filterItem_p->m_size) + 1;  /* +1 for zero terminating strings */
        ++m_numOfFilterItems;
    }

    m_filterStrings_p = static_cast<char *>(VirtualMem::Alloc(totalFilterTextSize));
    m_packedFilterItems_p =
        static_cast<packedFilterItem_t *>(VirtualMem::Alloc(m_numOfFilterItems *
                                                            static_cast<int>(sizeof(packedFilterItem_t))));

    packedfilterItem_p = m_packedFilterItems_p;
    destMem_p = m_filterStrings_p;

    int filterIndex = 1;   /* start at 1, since index 0 is nullptr (no filter match) */
    int regExpCount = 0;

    for (auto& filterItem_p : *m_filterItems_p) {
        memcpy(destMem_p, filterItem_p->m_start_p, static_cast<size_t>(filterItem_p->m_size));
        packedfilterItem_p->filterRef_p = filterItem_p;

        /* it is terminated, although that is not included in the size */
        packedfilterItem_p->length = filterItem_p->m_size;
        packedfilterItem_p->start_p = destMem_p;
        packedfilterItem_p->m_colClip_StartEnabled = m_colClip_StartEnabled;
        packedfilterItem_p->m_colClip_Start = m_colClip_Start;
        packedfilterItem_p->m_colClip_EndEnabled = m_colClip_EndEnabled;
        packedfilterItem_p->m_colClip_End = m_colClip_End;
        packedfilterItem_p->m_adaptiveClipEnabled = filterItem_p->m_adaptiveClipEnabled;
        packedfilterItem_p->m_regExpLUTIndex = -1;

        if (filterItem_p->m_regexpr) {
            packedfilterItem_p->m_regExpLUTIndex = regExpCount++;
        }

        destMem_p += filterItem_p->m_size;

        *destMem_p = 0;
        ++destMem_p; /* +1 to include the 0 termination added */

        ++packedfilterItem_p;  /* this must be done last */

        if (filterItem_p != m_filterItem_LUT_p[filterIndex++]) {
            /* Ensure that the packed filters match the LUT */
            TRACEX_E("Filter packing doesn't match Filter LUT")
        }
    }

    if ((destMem_p - m_filterStrings_p - 1) > totalFilterTextSize) {
        TRACEX_E("Filter packing wrong in size %d != %d",
                 totalFilterTextSize, destMem_p - m_filterStrings_p - 1)
    }

    if (!VirtualMem::CheckMem(m_filterStrings_p)) {
        TRACEX_E("Check mem failure")
    }

    if (m_execTimes_p != nullptr) {
        m_execTimes_p->packTime = timeExec.ms();
    }

#ifdef _DEBUG
    if (TRACEX_IS_ENABLED(LOG_LEVEL_DEBUG)) {
        char tempString[CFG_TEMP_STRING_MAX_SIZE];

        for (int index = 0; index < m_numOfFilterItems; ++index) {
            memcpy(tempString, m_packedFilterItems_p[index].start_p,
                   static_cast<size_t>(m_packedFilterItems_p[index].length));
            tempString[m_packedFilterItems_p[index].length] = 0;
            TRACEX_D("CFilterProcCtrl::Filter[%d] Length:%-5d Text:%s",
                     index, m_packedFilterItems_p[index].length, tempString)
        }
    }
#endif
}

/***********************************************************************************************************************
*   DecorateFIRA
***********************************************************************************************************************/
void CFilterProcCtrl::DecorateFIRA(void)
{
    FIR_t *FIRA_p = m_FIRA_p;
    QList<int>::iterator bookmarkIter;

    for (bookmarkIter = m_bookmarkList_p->begin(); bookmarkIter != m_bookmarkList_p->end(); ++bookmarkIter) {
        int row = *bookmarkIter;
        FIRA_p[row].LUT_index = BOOKMARK_FILTER_LUT_INDEX;  /* bookmark index */
    }
}

/***********************************************************************************************************************
*   NumerateFIRA
***********************************************************************************************************************/
void CFilterProcCtrl::NumerateFIRA(void)
{
    int FIR_Count = 0;
    int FIR_Exclude_Count = 0;
    int index;
    FIR_t *FIRA_p = m_FIRA_p;
    CFilterItem **filterItem_LUT_p = m_filterItem_LUT_p;
    const int rows = m_incremental ? m_endRow + 1 : m_totalNumOf_DB_TextItems;
    const int startIndex = m_incremental ? m_startRow : 0;

    if (m_incremental) {
        /* Since not starting from the beginning we are "adding up" filtering. Hence
         * start counting from last known filter */
        FIR_Count = m_totalFilterMatches;
        FIR_Exclude_Count = m_totalExcludeFilterMatches;
    }

    for (index = startIndex; index < rows; ++index) {
        const auto LUT_Index = FIRA_p[index].LUT_index;
        if (LUT_Index != 0) {
            const auto LUT_p = filterItem_LUT_p[LUT_Index];
            if ((LUT_p != nullptr) && !LUT_p->m_exclude) {
                if (!LUT_p->m_exclude) {
                    FIRA_p[index].index = FIR_Count;
                    ++FIR_Count;
                } else {
                    ++FIR_Exclude_Count;
                }
            }
        }
    }

    m_totalFilterMatches = FIR_Count;
    m_totalExcludeFilterMatches = FIR_Exclude_Count;
}

/***********************************************************************************************************************
*   ClearFilterRefs
***********************************************************************************************************************/
void CFilterProcCtrl::ClearFilterRefs(void)
{
    const int startIndex = m_incremental ? m_startRow : 0;
    const int rows = (m_endRow + 1) - startIndex;

    memset(&m_FIRA_p[startIndex], 0, sizeof(FIR_t) * static_cast<unsigned int>(rows));
}

/***********************************************************************************************************************
***********************************************************************************************************************
***********************************************************************************************************************
* TESTING
***********************************************************************************************************************
***********************************************************************************************************************
***********************************************************************************************************************/
struct test_vector_element {
    char text[64];
    char valid;
};
static char global_match = 0;

/****/
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    Q_UNUSED(id)
    Q_UNUSED(from)
    Q_UNUSED(to)
    Q_UNUSED(flags)
    Q_UNUSED(ctx)
    global_match = 1;
    return HS_SUCCESS; /*HS_SCAN_TERMINATED */
}

/****/
int TestHS(void)
{
    hs_compile_error_t *compile_err;
    hs_database_t *database = nullptr;
    hs_scratch_t *scratch = nullptr;
    struct test_vector_element test_vector[] = {
        {"find my mix mox", 1},
        {"find my mix mox mix", 1},
        {"find my mix mox my", 1},
        {"find my mix mox fly", 0},
        {"find my mix mox ply", 0},
        {"find my mix mox", 1},
        {"find my mix mox mIx", 1},
        {"find my mix mox plox", 0},
        {"find my mix mox my", 1},
        {"", 0}
    };
    int index = 0;
    const char *const filter[] = {"mox$|my$|m[iI]x$"};
    const unsigned int REGEXP_HYPERSCAN_FLAGS_TEST = (HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH);

    if (hs_compile_ext_multi(&filter[0],
                             &REGEXP_HYPERSCAN_FLAGS_TEST,
                             nullptr, /*ids*/
                             nullptr, /*ext*/
                             1, /*elements*/
                             HS_MODE_BLOCK, /*mode*/
                             nullptr, /*platform*/
                             &database,
                             &compile_err) != HS_SUCCESS) {
        hs_free_compile_error(compile_err);
        return -1;
    }

    hs_error_t error;
    if ((error = hs_alloc_scratch(database, &scratch)) != HS_SUCCESS) {
        return -1;
    }

    if ((database == nullptr) || (scratch == nullptr)) {
        return -1;
    }

    while (strlen(test_vector[index].text) > 0) {
        global_match = 0;

        hs_error_t result =
            hs_scan(database, test_vector[index].text, static_cast<unsigned int>(strlen(test_vector[index].text)),
                    0 /*flags*/, scratch, eventHandler, nullptr);

        switch (result)
        {
            case HS_SUCCESS:
                break;

            case HS_SCAN_TERMINATED:
                return -1;

            default:
                return -1;
        }
        if (test_vector[index].valid != global_match) {
            return -1;
        }
        ++index;
    }

    return index;
}
