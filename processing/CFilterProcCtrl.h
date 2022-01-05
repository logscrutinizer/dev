/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stdlib.h>

#ifdef _WIN32
 #define _CRTDBG_MAP_ALLOC
 #include <crtdbg.h>
#endif

#include "CFilter.h"
#include "CConfig.h"
#include "CFileProcBase.h"
#include "CTimeMeas.h"
#include "CFilter.h"
#include <hs/hs.h>

/* This class is used to carry configuration data */
class CFilterThreadConfiguration : public CThreadConfiguration
{
public:
    CFilterThreadConfiguration() : CThreadConfiguration() {}
    virtual ~CFilterThreadConfiguration() override;

    void init(FIR_t *FIRA_p, packedFilterItem_t *packedFilters_p, int numOfFilterItems);

    /****/
    virtual void PrepareRemove() override {
        if ((0 < m_numOfRegExpFilters) && (nullptr != m_regexp_database_array)) {
            for (int index = 0; index < m_numOfRegExpFilters; index++) {
                hs_free_database(m_regexp_database_array[index]);
                hs_free_scratch(m_regexp_scratch_array[index]);
            }
        }
        CThreadConfiguration::PrepareRemove();
    }

public:
    FIR_t *m_FIRA_p = nullptr;
    packedFilterItem_t *m_packedFilterItems_p = nullptr;
    int m_numOfFilterItems = 0;
    int m_numOfRegExpFilters = -1;
    bool m_useColClip = false;

    /* hyperscan regexp engine */
    hs_database_t **m_regexp_database_array = nullptr;
    hs_scratch_t **m_regexp_scratch_array = nullptr;
};

/***********************************************************************************************************************
*   CFilterThread
***********************************************************************************************************************/
class CFilterThread : public CFileProcThreadBase
{
public:
    CFilterThread(int32_t threadIndex,
                  QSemaphore *readySem,
                  QSemaphore *startSem,
                  QSemaphore *holdupSem,
                  QMutex *configurationListMutex_p,
                  QList<CThreadConfiguration *> *configurationList_p,
                  QList<CThreadConfiguration *> *configurationListPool_p) :
        CFileProcThreadBase(threadIndex,
                            readySem,
                            startSem,
                            holdupSem,
                            configurationListMutex_p,
                            configurationList_p,
                            configurationListPool_p) {}

    virtual ~CFilterThread(void) { /* Not sure if anything should be put here, see ExitInstance instead */}

protected:
    virtual void thread_Process(CThreadConfiguration *config_p);
};

/***********************************************************************************************************************
*   CFilterProcCtrl
***********************************************************************************************************************/
class CFilterProcCtrl : public CFileProcBase
{
public:
    CFilterProcCtrl(void) : m_numOfFilterItems(0) {}
    virtual ~CFilterProcCtrl(void) override {}

public:
    void init(QFile *qFile_p, char *workMem_p, int64_t workMemSize, TIA_t *TIA_p, FIR_t *FIRA_p,
              int numOfTextItems, QList<CFilterItem *> *filterItems_p,
              CFilterItem **filterItem_LUT_p, FilterExecTimes_t *execTimes_p,
              int priority, int colClip_Start, int colClip_End,
              int rowClip_Start, int rowClip_End, int *totalFilterMatches_p,
              int *totalExcludeFilterMatches_p, QList<int> *bookmarkList_p, bool incremental);

    void StartProcessing(
        QFile *qFile_p,
        char *DB_Mem_p,
        int64_t DB_MemSize,
        TIA_t *TIA_p,
        FIR_t *FIRA_p,
        int numOfTextItems,
        QList<CFilterItem *> *filterItems_p,
        CFilterItem **filterItem_LUT_p,
        FilterExecTimes_t *execTimes_p,
        int priority,
        int colClip_Start,
        int colClip_End,
        int rowClip_Start,
        int rowClip_End,
        int *totalFilterMatches_p,
        int *totalExcludeFilterMatches_p,
        QList<int> *bookmarkList_p,
        bool incremental = false);

    /* Typically used when adding/removing bookmark... */
    void StartOneLine(
        TIA_t *TIA_p,
        FIR_t *FIRA_p,
        int row,
        char *text_p,
        int textLength,
        QList<CFilterItem *> *filterItems_p,
        CFilterItem **filterItem_LUT_p,
        int *totalFilterMatches_p,
        int *totalExcludeFilterMatches_p,
        QList<int> *bookmarkList_p,
        bool isBookmarkRemove);

    void SetupIncrementalFiltering(QList<CFilterItem *> *filterItems_p, CFilterItem **filterItem_LUT_p,
                                   int colClip_Start, int colClip_End);

    bool ExecuteIncrementalFiltering(QFile *qFile_p,
                                     char *workMem_p,
                                     int64_t workMemSize,
                                     TIA_t *TIA_p,
                                     FIR_t *FIRA_p,
                                     unsigned startIndex,
                                     FilterExecTimes_t *execTimes_p,
                                     int *totalFilterMatches_p,
                                     int *totalExcludeFilterMatches_p);

    CFilterItem *GetFilterMatch(
        char *text_p,
        int textLength,
        QList<CFilterItem *> *filterItems_p,
        CFilterItem **filterItem_LUT_p);

    void PackFilters(void);
    void ClearFilterRefs(void);
    void NumerateFIRA(void);
    void DecorateFIRA(void);

protected:
    virtual bool ConfigureThread(CThreadConfiguration *config_p,
                                 Chunk_Description_t *chunkDescription_p,
                                 int32_t threadIndex) override;

    /****/
    virtual CFileProcThreadBase *CreateProcThread(int32_t threadIndex,
                                                  QSemaphore *readySem,
                                                  QSemaphore *startSem,
                                                  QSemaphore *holdupSem,
                                                  QMutex *configurationListMutex_p,
                                                  QList<CThreadConfiguration *> *configurationList_p,
                                                  QList<CThreadConfiguration *> *configurationListPool_p) override
    {
        return new CFilterThread(threadIndex, readySem, startSem, holdupSem, configurationListMutex_p,
                                 configurationList_p, configurationListPool_p);
    }

    virtual CThreadConfiguration *CreateConfigurationObject(void) override;
    virtual void WrapUp(void) override;

private:
    FIR_t *m_FIRA_p = nullptr;
    int m_totalNumOf_DB_TextItems; /* Total number of rows in database */
    QList<CFilterItem *> *m_filterItems_p = nullptr;

    /* An array of references to filters 0-255 positions (contains more or less the same as m_filters_p) */
    CFilterItem **m_filterItem_LUT_p = nullptr;
    QList<int> *m_bookmarkList_p = nullptr;
    char *m_filterStrings_p = nullptr;
    packedFilterItem_t *m_packedFilterItems_p = nullptr;
    int m_numOfFilterItems = 0;
    bool m_colClip_StartEnabled = false; /* Column wise clip start enabling */
    int m_colClip_Start = false; /* Column wise clip start enabling */
    bool m_colClip_EndEnabled = false;
    int m_colClip_End = false;
    FilterExecTimes_t *m_execTimes_p = nullptr;
    int m_numOfRegExpFilters = -1;
    int m_totalFilterMatches = 0; /* initially contains matches from prevous filtering */
    int m_totalExcludeFilterMatches = 0; /* initially contains matches from prevous filtering */
    bool m_incremental = false; /* If the filtering is incremental */
    CFilterThreadConfiguration *m_incrementalThreadConfig_p = nullptr;
};
