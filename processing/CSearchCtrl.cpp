/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CDebug.h"
#include "CSearchCtrl.h"

#include <hs/hs.h>

#ifdef TEST_HS

static unsigned long long g_to;
static unsigned long long g_from;

/***********************************************************************************************************************
*   eventHandler
***********************************************************************************************************************/
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    /*    printf("Match for pattern \"%s\" at offset %llu - %llu\n", (char *)ctx, from, to); */
    g_from = from;
    g_to = to;
    return 0;
}

/***********************************************************************************************************************
*   test
***********************************************************************************************************************/
void test(void)
{
    char pattern[] = "so.*ny";
    char text[] = "there are so many pigs around";
    hs_database_t *database;
    hs_compile_error_t *compile_err;
    if (hs_compile(pattern,
                   HS_FLAG_DOTALL | HS_FLAG_SOM_LEFTMOST,
                   HS_MODE_BLOCK,
                   NULL,
                   &database,
                   &compile_err) != HS_SUCCESS) {
        g_processingCtrl_p->AddProgressInfo(QString("Regular expression contains error: %1").arg(pattern));
        TRACEX_I(QString("RegExp failed %1").arg(compile_err->message));
        hs_free_compile_error(compile_err);
        g_processingCtrl_p->m_abort = true;
        return;
    }

    hs_scratch_t *scratch = NULL;
    if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
        hs_free_database(database);
        return;
    }

    if (hs_scan(database, text, strlen(text), 0, scratch, eventHandler, pattern) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to scan input buffer. Exiting.\n");
        hs_free_scratch(scratch);
        hs_free_database(database);
        return;
    }

    /* Scanning is complete, any matches have been handled, so now we just * clean up and exit. */
    hs_free_scratch(scratch);
    hs_free_database(database);
}
#endif

/***********************************************************************************************************************
*   FileIndex_To_MemRef
***********************************************************************************************************************/
inline char *FileIndex_To_MemRef(int64_t *fileIndex_p, int64_t *workMemFileIndex_p, char *WorkMem_p)
{
    return ((char *)(WorkMem_p + (*fileIndex_p - *workMemFileIndex_p)));
}

/***********************************************************************************************************************
*   thread_Process
***********************************************************************************************************************/
void CSearchThread::thread_Process(CThreadConfiguration *config_p)
{
    CSearchThreadConfiguration *searchConfig_p = static_cast<CSearchThreadConfiguration *>(config_p);
    Match_Description_t matchDescr;
    FIR_t *FIR_Array_p = nullptr;
    CFilterItem **filterItem_LUT_p = nullptr;

    memset(&matchDescr, 0, sizeof(Match_Description_t));

    if (searchConfig_p->m_FIRA_p != nullptr) {
        FIR_Array_p = &searchConfig_p->m_FIRA_p->FIR_Array_p[0];
        filterItem_LUT_p = searchConfig_p->m_filterItem_LUT_p;
    }

#ifdef THREAD_DEBUGGING
    TRACEX_D("CSearchThread::thread_Process  0x%x   m_start_TIA_index:%d m_stop_TIA_Index:%d m_TIA_step:%d",
             this,
             searchConfig_p->m_start_TIA_index,
             searchConfig_p->m_stop_TIA_Index,
             searchConfig_p->m_TIA_step);
#endif

    m_isStopped = false;
    *searchConfig_p->m_TIA_Index_p = -1; /* invalid value, hence shall not be used for wrap-up */

    matchDescr.filterLength = (int)strlen(searchConfig_p->m_searchText) - 1; /* filter length is compared to index */
    matchDescr.filter_p = searchConfig_p->m_searchText;

    const bool regExp = searchConfig_p->m_regExp;
    const bool CS = searchConfig_p->m_caseSensitive;

    if (searchConfig_p->m_regExp) {
        matchDescr.regexp_database = searchConfig_p->m_regexp_database;
        matchDescr.regexp_scratch = searchConfig_p->m_regexp_scratch;
        matchDescr.threadIndex = m_threadIndex;
    }

    if ((m_threadIndex == 0) && g_DebugLib->m_isThreadCrashTrigger) {
        int *a = nullptr;
        *a = 3;
    }

    int TIA_Index = searchConfig_p->m_start_TIA_index;    /* use local variable for quicker access */
    const int stop_TIA_Index = searchConfig_p->m_stop_TIA_Index;
    const int TIA_step = searchConfig_p->m_TIA_step;
    const TIA_t *TIA_p = searchConfig_p->m_TIA_p;
    int progressCount = PROGRESS_COUNTER_STEP;
    bool stopLoop = false;

    if (!searchConfig_p->m_backward) {
        /* Normal search "FORWARD" */

        while (TIA_Index < stop_TIA_Index && !stopLoop) {
            const auto LUT_Index = FIR_Array_p == nullptr ? 0 : FIR_Array_p[TIA_Index].LUT_index;
            if ((FIR_Array_p == nullptr) ||
                ((LUT_Index != 0) && !filterItem_LUT_p[LUT_Index]->m_exclude)) {
                /* The home made search relies on that size is one less than it should... */
                matchDescr.textLength = TIA_p->textItemArray_p[TIA_Index].size - (regExp ? 0 : 1);
                matchDescr.text_p = FileIndex_To_MemRef(&TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                                        &searchConfig_p->m_chunkDescr.fileIndex,
                                                        searchConfig_p->m_workMem_p);

                --progressCount;
                if (progressCount == 0) {
                    g_processingCtrl_p->StepProgressCounter(m_threadIndex);
                    progressCount = PROGRESS_COUNTER_STEP;
                }

                if (matchDescr.textLength > 0) {
                    if (regExp) {
                        if (thread_Match_RegExp_HyperScan(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true;  /* signal that there is a search match */
                            stopLoop = true;
                        }
                    } else if (CS) {
                        if (thread_Match_CS(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true; /* signal that there is a search match */
                            stopLoop = true;
                        }
                    } else {
                        if (thread_Match(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true; /* signal that there is a search match */
                            stopLoop = true;
                        }
                    }
                }
            }
            if (!*searchConfig_p->m_searchStop_p && !g_processingCtrl_p->m_abort) {
                TIA_Index += TIA_step;
            } else {
                stopLoop = true;
            }
        } /* while */
    } else {
        /* Search "BACKWARD" (upwards) */

        while (TIA_Index >= stop_TIA_Index && !stopLoop) {
            --progressCount;
            if (progressCount == 0) {
                g_processingCtrl_p->StepProgressCounter(m_threadIndex);
                progressCount = PROGRESS_COUNTER_STEP;
            }

            const auto LUT_Index = FIR_Array_p == nullptr ? 0 : FIR_Array_p[TIA_Index].LUT_index;
            if ((FIR_Array_p == nullptr) ||
                ((LUT_Index != 0) && !filterItem_LUT_p[LUT_Index]->m_exclude)) {
                /* The home made search relies on that size is one less than it should... */
                matchDescr.textLength = TIA_p->textItemArray_p[TIA_Index].size - (regExp ? 0 : 1);
                matchDescr.text_p = FileIndex_To_MemRef(&TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                                        &config_p->m_chunkDescr.fileIndex, config_p->m_workMem_p);

                if (matchDescr.textLength > 0) {
                    if (regExp) {
                        if (thread_Match_RegExp_HyperScan(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true;  /* signal that there is a search match */
                            stopLoop = true;
                        }
                    } else if (CS) {
                        if (thread_Match_CS(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true; /* signal that there is a search match */
                            stopLoop = true;
                        }
                    } else {
                        if (thread_Match(&matchDescr)) {
                            *searchConfig_p->m_searchStop_p = true; /* signal that there is a search match */
                            stopLoop = true;
                        }
                    }
                }
            }

            if (!*searchConfig_p->m_searchStop_p && !g_processingCtrl_p->m_abort) {
                if (TIA_Index != stop_TIA_Index) {
                    TIA_Index -= TIA_step;
                } else {
                    stopLoop = true;
                }
            } else {
                stopLoop = true;
            }
        } /* while search */
    } /* else upward */

    *searchConfig_p->m_TIA_Index_p = TIA_Index;

    (void)thread_ProcessingDone();
}

/***********************************************************************************************************************
*   StartProcessing
***********************************************************************************************************************/
void CSearchCtrl::StartProcessing(QFile *qFile_p, char *workMem_p, int workMemSize, TIA_t *TIA_p,
                                  FIRA_t *FIRA_p, CFilterItem **filterItem_LUT_p, int priority, QString *searchText_p,
                                  int startRow, int endRow, bool backward, bool regExp,
                                  bool caseSensitive)
{
    TRACEX_I("Search started  text:%s backward:%d regExp:%d startRow:%d endRow:%d  %s",
             searchText_p->toLatin1().constData(), (int)backward, (int)regExp, startRow, endRow,
             FIRA_p == nullptr ? "Full search" : "Filtered search");

#ifdef TEST_HS
    test();
#endif

    g_processingCtrl_p->AddProgressInfo(QString("Starting search"));

    m_searchText_p = searchText_p;
    m_regExp = regExp;
    m_caseSensitive = caseSensitive;
    m_FIRA_p = FIRA_p;
    m_filterItem_LUT_p = filterItem_LUT_p;

    CFileProcBase::Start(qFile_p, workMem_p, workMemSize, TIA_p, priority, startRow, endRow, backward);
}

/***********************************************************************************************************************
*   ConfigureThread
***********************************************************************************************************************/
bool CSearchCtrl::ConfigureThread(CThreadConfiguration *config_p, Chunk_Description_t *chunkDescription_p,
                                  int32_t threadIndex)
{
    CSearchThreadConfiguration *searchConfig_p = static_cast<CSearchThreadConfiguration *>(config_p);

    CFileProcBase::ConfigureThread(config_p, chunkDescription_p, threadIndex); /* Use the default initialization */
    SAFE_STR_MEMCPY(searchConfig_p->m_searchText, CFG_TEMP_STRING_MAX_SIZE, m_searchText_p->toLatin1().constData(),
                    m_searchText_p->size());

    searchConfig_p->m_searchStop_p = &m_searchStop;
    searchConfig_p->m_backward = m_backward;

    searchConfig_p->m_regExp = m_regExp;
    searchConfig_p->m_caseSensitive = m_caseSensitive;
    searchConfig_p->m_FIRA_p = m_FIRA_p;
    searchConfig_p->m_filterItem_LUT_p = m_filterItem_LUT_p;

    if (threadIndex == 0) {
        /* Clear the result array each time the processing restart for a new chunk */
        for (int index = 0; index < g_cfg_p->m_numOfThreads; ++index) {
            m_threadStopRow[index] = -1;
        }
    }

    /* Setup where the search thread should put its search result, used at wrapup */
    searchConfig_p->m_TIA_Index_p = &m_threadStopRow[threadIndex];

    /* Setup NumOf_TIs */
    if (threadIndex < m_linesExtra) {
        searchConfig_p->m_numOf_TI = m_linesPerThread + 1;
    } else {
        searchConfig_p->m_numOf_TI = m_linesPerThread;
    }

    searchConfig_p->m_TIA_step = m_numberOfChunkThreads;

    if (!m_backward) {
        /* FORWARD SEARCH */
        searchConfig_p->m_start_TIA_index = m_chunkDescr.TIA_startRow + threadIndex;

        /* The m_stop_TIA_Index index is not used, use in loop to be less than. */
        searchConfig_p->m_stop_TIA_Index = searchConfig_p->m_start_TIA_index +
                                           searchConfig_p->m_numOf_TI * searchConfig_p->m_TIA_step;
    } else {
        /* BACKWARD SEARCH
         * -1 since it is 0 based indexing (and max index is MAX-1)   XXX */
        searchConfig_p->m_start_TIA_index = m_chunkDescr.TIA_startRow - threadIndex;

        /* IMPORTANT, m_stop_TIA_Index is used, hence it must represent the last usable index. As such (m_numOf_TI - 1)
         * */
        searchConfig_p->m_stop_TIA_Index = searchConfig_p->m_start_TIA_index -
                                           (searchConfig_p->m_numOf_TI - 1) * searchConfig_p->m_TIA_step;
    }

    if (m_regExp && (nullptr == searchConfig_p->m_regexp_database)) {
        hs_compile_error_t *compile_err;
        if (hs_compile(searchConfig_p->m_searchText,
                       REGEXP_HYPERSCAN_FLAGS,
                       HS_MODE_BLOCK,
                       nullptr,
                       &searchConfig_p->m_regexp_database,
                       &compile_err) != HS_SUCCESS) {
            TRACEX_I(QString("RegExp failed %1").arg(compile_err->message));
            g_processingCtrl_p->AddProgressInfo(QString("Regular expression contains error: %1")
                                                    .arg(searchConfig_p->m_searchText));
            g_processingCtrl_p->m_abort = true;
            hs_free_compile_error(compile_err);
            return false;
        }

        if (hs_alloc_scratch(searchConfig_p->m_regexp_database, &searchConfig_p->m_regexp_scratch) != HS_SUCCESS) {
            TRACEX_I(QString("ERROR: Unable to allocate scratch space. Exiting."));
            fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.");
            return false;
        }
    }

    return true;
}

/***********************************************************************************************************************
*   CreateConfigurationObject
***********************************************************************************************************************/
CThreadConfiguration *CSearchCtrl::CreateConfigurationObject(void)
{
    return static_cast<CThreadConfiguration *>(new CSearchThreadConfiguration());
}

/***********************************************************************************************************************
*   isProcessingDone
***********************************************************************************************************************/
bool CSearchCtrl::isProcessingDone(void)
{
    if (m_searchStop) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************************************************
*   WrapUp
***********************************************************************************************************************/
void CSearchCtrl::WrapUp(void)
{
    int low_TI_Index = -1;
    int high_TI_Index = -1;
    FIR_t *FIR_Array_p = nullptr;

    if (m_FIRA_p != nullptr) {
        FIR_Array_p = &m_FIRA_p->FIR_Array_p[0];
    }

    char searchText[4096];

    SAFE_STR_MEMCPY(searchText, CFG_TEMP_STRING_MAX_SIZE, m_searchText_p->toLatin1().constData(),
                    m_searchText_p->size());

    m_searchResult_TI = 0;
    m_searchSuccess = false;

    if (g_processingCtrl_p->m_abort) {
        m_searchSuccess = false;
        g_processingCtrl_p->SetFail();
        g_processingCtrl_p->AddProgressInfo(QString("Search aborted by user"));
        return;
    }

    /* Hard to say if all threads manage to process all their lines, one thread might have been quicker and
     * the match is actually not the last one */

    TRACEX_D("CSearchCtrl::WrapUp");

    /*Sweep from thread with lowest TI until the one with the highest, pick the earliest */

    if (m_searchStop) {
        g_processingCtrl_p->AddProgressInfo(QString("  Search hit, aligning threads"));

        /* Get the low / high limits by comparing the results from each thread */
        for (int index = 0; index < m_numberOfChunkThreads; ++index) {
            if (m_threadStopRow[index] != -1) {
                if (low_TI_Index == -1) {
                    low_TI_Index = m_threadStopRow[index];
                    high_TI_Index = m_threadStopRow[index];
                }
                if (low_TI_Index > m_threadStopRow[index]) {
                    low_TI_Index = m_threadStopRow[index];
                }
                if (high_TI_Index < m_threadStopRow[index]) {
                    high_TI_Index = m_threadStopRow[index];
                }
            }
        }

        TRACEX_D("CSearchCtrl::WrapUp found:%d low:%d high:%d total:%d",
                 m_searchStop, low_TI_Index, high_TI_Index, m_TIA_p->rows);

        if (low_TI_Index == high_TI_Index) {
            m_searchSuccess = true;
            m_searchResult_TI = high_TI_Index;
            g_processingCtrl_p->SetSuccess();
            g_processingCtrl_p->AddProgressInfo(QString("Search complete, SUCCESS   Line:%1").arg(m_searchResult_TI));
            return;
        } else {
            Match_Description_t matchDescr;
            bool match = false;

            memset(&matchDescr, 0, sizeof(Match_Description_t));

            matchDescr.filterLength = (int)strlen(searchText) - 1;
            matchDescr.filter_p = searchText;

            if (m_regExp) {
                /* Hijack first configuraiton object to access hyperscan objects */
                CSearchThreadConfiguration *configuration;
                if (!m_configurationList.isEmpty()) {
                    configuration = static_cast<CSearchThreadConfiguration *>(m_configurationList.first());
                } else {
                    configuration = static_cast<CSearchThreadConfiguration *>(m_configurationPoolList.first());
                }
                matchDescr.regexp_database = configuration->m_regexp_database;
                matchDescr.regexp_scratch = configuration->m_regexp_scratch;
            }

            /* Sweep between the low - high row index to see if there is any earlier match in the lowest index
             * (search forward), or in the highest region when searching backwards. */

            /* SEARCH FORWARD */

            if (!m_backward) {
                for (int TIA_Index = low_TI_Index; TIA_Index <= high_TI_Index && !(match); ++TIA_Index) {
                    const auto LUT_Index = FIR_Array_p == nullptr ? 0 : FIR_Array_p[TIA_Index].LUT_index;
                    if ((FIR_Array_p == nullptr) ||
                        ((LUT_Index != 0) && !m_filterItem_LUT_p[LUT_Index]->m_exclude)) {
                        /* The home made search relies on that size is one less than it should... */
                        matchDescr.textLength = m_TIA_p->textItemArray_p[TIA_Index].size - (m_regExp ? 0 : 1);
                        matchDescr.text_p = FileIndex_To_MemRef(&m_TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                                                &m_chunkDescr.fileIndex, m_workMem_p);

                        if (!m_regExp) {
                            match = thread_Match(&matchDescr);
                        } else {
                            match = thread_Match_RegExp_HyperScan(&matchDescr);
                        }
                        if (match) {
                            m_searchSuccess = true;
                            m_searchResult_TI = TIA_Index;
                        }
                    }
                }
            }

            /* SEARCH BACKWARD */
            else {
                int TIA_Index = high_TI_Index;
                bool stop = false;

                while (TIA_Index >= low_TI_Index && !(match) && !stop) {
                    const auto LUT_Index = FIR_Array_p == nullptr ? 0 : FIR_Array_p[TIA_Index].LUT_index;
                    if ((FIR_Array_p == nullptr) ||
                        ((LUT_Index != 0) && !m_filterItem_LUT_p[LUT_Index]->m_exclude)) {
                        /* The home made search relies on that size is one less than it should... */
                        matchDescr.textLength = m_TIA_p->textItemArray_p[TIA_Index].size - (m_regExp ? 0 : 1);
                        matchDescr.text_p = FileIndex_To_MemRef(&m_TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                                                &m_chunkDescr.fileIndex, m_workMem_p);

                        if (!m_regExp) {
                            match = thread_Match(&matchDescr);
                        } else {
                            match = thread_Match_RegExp_HyperScan(&matchDescr);
                        }
                    }

                    if (match) {
                        m_searchSuccess = true;
                        m_searchResult_TI = TIA_Index;
                    } else {
                        /* being causius with unsigned values and decrements... if passing
                         * zero the value will be big... not neg */
                        if (TIA_Index != 0) {
                            --TIA_Index;
                        } else {
                            stop = true;
                        }
                    }
                }
            }
        } /* else if (low_TI_Index == high_TI_Index) */
    } else {
        g_processingCtrl_p->AddProgressInfo(QString("Search complete, FAIL  No match"));
    }

    if (m_searchSuccess) {
        g_processingCtrl_p->SetSuccess();
        g_processingCtrl_p->AddProgressInfo(QString("Search complete, SUCCESS   Line:%1").arg(m_searchResult_TI));
    } else {
        g_processingCtrl_p->SetFail();
    }

    TRACEX_D("CSearchCtrl::WrapUp match:%d row:%d", m_searchSuccess, m_searchResult_TI);
}
