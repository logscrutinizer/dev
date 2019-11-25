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
#include "hs/hs.h"

/***********************************************************************************************************************
*   CSearchThreadConfiguration
***********************************************************************************************************************/
class CSearchThreadConfiguration : public CThreadConfiguration
{
public:
    CSearchThreadConfiguration() : CThreadConfiguration() {}

    virtual ~CSearchThreadConfiguration(void) override;

    /***********************************************************************************************************************
    *   PrepareRemove
    ***********************************************************************************************************************/
    virtual void PrepareRemove() override {
        if (nullptr != m_regexp_database) {
            hs_free_database(m_regexp_database);
        }

        if (nullptr != m_regexp_scratch) {
            hs_free_scratch(m_regexp_scratch);
        }

        CThreadConfiguration::PrepareRemove();
    }

public:
    char m_searchText[CFG_TEMP_STRING_MAX_SIZE];
    volatile bool *m_searchStop_p; /* Direct reference to a shared flag which may be signaled by any thread, if set the
                                    * thread should exit. If search hit, flag should be set */
    bool m_backward; /* If we are searching up/down */
    bool m_regExp; /* True if regular expression search */
    bool m_caseSensitive; /* true if match is case sensitive */
    FIRA_t *m_FIRA_p; /* Keeping track of filtered rows, if required. Null if full search */
    CFilterItem **m_filterItem_LUT_p; /* Lookup table, contains all filter items */
    int32_t *m_TIA_Index_p; /* The TIA index where the search thread stopped  OUT-VALUE, -1 if not set */

    /* hyperscan regexp engine */
    hs_database_t *m_regexp_database = nullptr;
    hs_scratch_t *m_regexp_scratch = nullptr;
};

/***********************************************************************************************************************
*   CSearchThread
***********************************************************************************************************************/
class CSearchThread : public CFileProcThreadBase
{
public:
    CSearchThread(int32_t threadIndex,
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
                            configurationListPool_p)
    {}

    virtual ~CSearchThread(void) {
        /* Not sure if anything should be put here, see ExitInstance instead */
    }

protected:
    virtual void thread_Process(CThreadConfiguration *config_p);
};

/***********************************************************************************************************************
*   CSearchCtrl
***********************************************************************************************************************/
class CSearchCtrl : public CFileProcBase
{
public:
    CSearchCtrl(void)
    {
        m_searchText_p = nullptr;
        m_searchStop = false;
        m_searchSuccess = false;
        m_searchResult_TI = 0;
    }

    virtual ~CSearchCtrl(void) override {}

public:
    virtual void StartProcessing(QFile *qFile_p, char *workMem_p, int64_t workMemSize, TIA_t *TIA_p,
                                 FIRA_t *FIRA_p, CFilterItem **filterItem_LUT_p, int priority, QString *searchText_p,
                                 int startRow, int endRow, bool backward, bool regExp,
                                 bool caseSensitive);

    /****/
    bool GetSearchResult(int *searchResult_TI_p)
    {
        *searchResult_TI_p = m_searchResult_TI;
        return m_searchSuccess;
    }

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
        return new CSearchThread(threadIndex,
                                 readySem,
                                 startSem,
                                 holdupSem,
                                 configurationListMutex_p,
                                 configurationList_p,
                                 configurationListPool_p);
    }
    virtual CThreadConfiguration *CreateConfigurationObject(void) override;
    virtual bool isProcessingDone(void) override; /* Override to enable early stop when search successful */
    virtual void WrapUp(void) override;

private:
    QString *m_searchText_p; /* Search string */
    volatile bool m_searchStop; /* This flag is provided to all the threads, in-case one thread finds anything this
                                 * value is set. */
    bool m_regExp; /* True if regular expression search */
    bool m_caseSensitive;
    FIRA_t *m_FIRA_p; /* Keeping track of filtered rows, if required. Null if full search */
    CFilterItem **m_filterItem_LUT_p;
    bool m_searchSuccess; /* Result of the search */
    int m_searchResult_TI; /* Result of the search */
    int32_t m_threadStopRow[MAX_NUM_OF_THREADS]; /* Each thread put their value into the array when they exit the
                                                  * processing loop. Wrap up will determine the final search hit row */
};
