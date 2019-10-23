/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CFilter.h"
#include "CConfig.h"
#include "CTimeMeas.h"
#include "CDebug.h"
#include "CProgressCtrl.h"
#include "globals.h"

#include "crash_handler_linux.h"

#include <iostream>
#include <string>
#include <stdint.h>

#include <QString>
#include <QThread>
#include <QSemaphore>
#include <atomic>

#include <hs/hs.h>

/*https://intel.github.io/hyperscan/dev-reference/ */
const unsigned int REGEXP_HYPERSCAN_FLAGS = (HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH);

#define PROGRESS_COUNTER_STEP     (10000)

typedef struct {
    int numOfRows; /* Number of rows loaded in workMem */
    int TIA_startRow; /* The corresponding start index in TIA */
    bool first; /* first chunk load */
    int64_t fileIndex; /* the last start file index location of the m_workMem_p start */
    int64_t temp_offset; /* offset is used in-case not all bytes could be read, failure? */
}Chunk_Description_t;

typedef struct {
    char *text_p;
    int textLength;
    const char *filter_p;
    int filterLength;

    /* hyperscan regexp engine */
    hs_database_t *regexp_database;
    hs_scratch_t *regexp_scratch;
    int32_t threadIndex; /* used for saving result */
    bool match;
} Match_Description_t;

extern bool thread_Match(Match_Description_t *desc_p);
extern bool thread_Match_RegExp(Match_Description_t *desc_p);
extern bool thread_Match_RegExp_HyperScan(Match_Description_t *desc_p);
extern bool thread_Match_CS(Match_Description_t *desc_p);

/***********************************************************************************************************************
*   Sleeper
***********************************************************************************************************************/
class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs) {QThread::usleep(usecs);}
    static void msleep(unsigned long msecs) {QThread::msleep(msecs);}
    static void sleep(unsigned long secs) {QThread::sleep(secs);}
};

class CFileProcBase;

/***********************************************************************************************************************
*   CThreadConfiguration
*
*  QSemaphore startSem(numOfThreads)
*  QSemaphore readySem(numOfThreads)
*  QSemaphore holdUpSem(numOfThreads)
*
*  thread                                                   ctrl
*  startSem.acquire(numOfThreads) (proceeds)
*  holdUpSem.acquire(numOfThreads) (proceeds)
*
*  loop point
*  readySem.aquire(1) (proceed)
*   startSem.aquire(1) (waiting)
*  while (readySem.available() > 0) sleep 1ms    // wait for all threads to reach the point waiting for the startSem
*  holdUpSem.acquire(numOfThreads) (proceeds)
*  startSem.release(numOfThreads)   (start thread)
*  proceed from startmutex
*  unlock startmutex
*  readySem.acquire(numOfThreads) (wait for all threads)
*  processing
*
*  (done)  readySem.release(1)
*  holupSem.aquire(1) (waiting)
*  startSem.acquire(numOfThreads) (proceeds)
*  holdupSem.release(numOfThreads)
*  holdupSem.release(1) s1 mutex (proceeds)
***********************************************************************************************************************/
class CThreadConfiguration
{
public:
    CThreadConfiguration() : m_numOf_TI(0), m_start_TIA_index(0), m_stop_TIA_Index(0), m_TIA_step(0),
        m_servedBy_threadIndex(-1), m_workMem_p(nullptr), m_TIA_p(nullptr) {}

    virtual ~CThreadConfiguration();

    /****/
    void BasicInit(char *workMem_p, TIA_t *TIA_p)
    {
        m_workMem_p = workMem_p;
        m_TIA_p = TIA_p;
    }

    /****/
    void DeltaInit(Chunk_Description_t *chunkDescr_p, int numOf_TI, int start_TIA_index,
                   int stop_TIA_Index, int TIA_step)
    {
        m_chunkDescr = *chunkDescr_p;
        m_numOf_TI = numOf_TI;
        m_start_TIA_index = start_TIA_index;
        m_stop_TIA_Index = stop_TIA_Index;
        m_TIA_step = TIA_step;
    }

    /****/
    virtual void Clean()
    {
        /* If you override this vfunction don't forget to call the base class such that servedBy is set to -1 */
        m_servedBy_threadIndex = -1;
    }

    /****/
    virtual void PrepareRemove()
    {
        /* If you override this vfunction don't forget to call the base class */
    }

    int GetNumOf_TI() {return m_numOf_TI;}

    int m_numOf_TI; /* Number of TIA rows this thread shall process from the chunk */
    int m_start_TIA_index; /* At which TIA index this thread shall start */
    int m_stop_TIA_Index; /* Where this thread should stop */
    int m_TIA_step; /* The index step taken when fetching new line */

    /* Identify of the thread, set by the thread that picks up the configuration. If not processed this value is -1 */
    int m_servedBy_threadIndex;
    Chunk_Description_t m_chunkDescr;
    char *m_workMem_p; /* Memory containing the loaded text file */
    TIA_t *m_TIA_p; /* Text Item Array, mapping between fileIndex and rows in the textFile */
};

/***********************************************************************************************************************
*   CFileProcThreadBase
***********************************************************************************************************************/
class CFileProcThreadBase : public QThread
{
    Q_OBJECT

public:
    CFileProcThreadBase(int32_t threadIndex, QSemaphore *readySem, QSemaphore *startSem, QSemaphore *holdupSem,
                        QMutex *configurationListMutex_p, QList<CThreadConfiguration *> *configurationList_p,
                        QList<CThreadConfiguration *> *configurationListPool_p) :
        m_threadIndex(threadIndex), m_startSem_p(startSem), m_readySem_p(readySem), m_holdupSem_p(holdupSem),
        m_configurationListMutex_p(configurationListMutex_p), m_configurationList_p(configurationList_p),
        m_configurationListPool_p(configurationListPool_p)
    {
#ifdef THREAD_DEBUGGING
        TRACEX_DE("Thread Created");
#endif
    }

    CFileProcThreadBase(void) = delete;

    virtual ~CFileProcThreadBase() override
    {}

    /****/
    void run() override
    {
        g_RamLog->RegisterThread();

        auto unregisterRamLog = makeMyScopeGuard([&] () {
            g_RamLog->UnregisterThread();
        });

        m_isStopped = false;
        m_stop = false;

        int loops = 0;

        while (!m_stop) {
            /* Proc has taken the startSem and holdupSem, and are waiting for the thread to take the ready.
             * When all readySem has been taken Proc will release all startSem. */

            /*PRINT_PROGRESS("Thread %d waiting for readySem", m_threadIndex); */
            m_readySem_p->acquire(1);

            /*PRINT_PROGRESS("Thread %d waiting for readySem", m_threadIndex); */
            m_startSem_p->acquire(1);
            m_startSem_p->release(1);

            loops++;

            if (!m_stop) {
                if (GetConfiguration()) {
                    PRINT_PROGRESS_DBG("Thread %d processing start %d", m_threadIndex, loops);
                    thread_Process(m_configuration_p);
                } else {
                    TRACEX_E("Thread was allowed to start, but there was no configuration");
                    m_stop = true; /* error */
                }

                /* When thread release the readySem proc knows it has completed its task. */
                m_readySem_p->release(1);

                /*  PRINT_PROGRESS("Thread %d processing done, waiting for holdupSem", m_threadIndex);
                 * holdupSem is used to pile up the threads */
                m_holdupSem_p->acquire(1);  /* get stuck here until ctrl is ready */
                m_holdupSem_p->release(1);  /* */
            } else {
                m_readySem_p->release(1);
#ifdef THREAD_DEBUGGING
                TRACEX_DE("Thread exiting, just release the ready sem");
#endif
            }
        }

        m_isStopped = true;
    }

    /****/
    void Stop() {m_stop = true;}

protected:
    /* Functions called in thread context only */
    virtual void thread_Process(CThreadConfiguration *config_p)
    {
        Q_UNUSED(config_p);
        m_isStopped = false;
        thread_ProcessingDone();
    }
    void thread_ProcessingDone(void) {}
    bool thread_isStopped(void) {return m_isStopped;}

    /****/
    bool GetConfiguration()
    {
        QMutexLocker locker(m_configurationListMutex_p);
        if (m_configurationList_p->empty()) {return false;} /* error if list is empty */
        m_configuration_p = m_configurationList_p->takeFirst();
        m_configurationListPool_p->append(m_configuration_p); /* put it back to the pool */
        return true;
    }

public:
    /* Specific variables used for regular expression operations */
    bool m_isConfiguredOnce;
    int32_t m_threadIndex;

protected:
    std::atomic_bool m_isStopped;
    std::atomic_bool m_stop;
    CThreadConfiguration *m_configuration_p;

    /* Used to trigger when thread shall start, this is unique for each thread. This is to be able to
     *  start a specific thread */
    QSemaphore *m_startSem_p;

    /* Used to indicate to ctrl when thread is ready to run again */
    QSemaphore *m_readySem_p;

    /* Extra semaphore point to make all threads rally, used for extra safety in-case some of the threads pass
     * processing fast */
    QSemaphore *m_holdupSem_p;
    QMutex *m_configurationListMutex_p;
    QList<CThreadConfiguration *> *m_configurationList_p;
    QList<CThreadConfiguration *> *m_configurationListPool_p;
};

/***********************************************************************************************************************
*   CFileProcBase
***********************************************************************************************************************/
class CFileProcBase : public QObject
{
    Q_OBJECT

public:
    CFileProcBase() : m_readySem_p(nullptr), m_holdupSem_p(nullptr)
    {
        m_qfile_p = nullptr;
        m_fileSize_LDW = 0;
        m_fileSize_HDW = 0;
        m_workMem_p = nullptr;
        m_workMemSize = 0;
        m_TIA_p = nullptr;
        m_priority = 0;
        m_startRow = 0;
        m_endRow = 0;
        m_backward = false;
        m_threadTI_Split = true;
        m_execTime = 0.0;
        m_progress = 0.0;

        g_processingCtrl_p->m_abort = false;

        for (int index = 0; index < g_cfg_p->m_numOfThreads; index++) {
            m_startSem_pp[index] = nullptr;
        }

        memset(&m_chunkDescr, 0, sizeof(Chunk_Description_t));
        memset(m_threadInstances, 0, sizeof(m_threadInstances));
    }

    virtual ~CFileProcBase(void)
    {
        for (int index = 0; index < g_cfg_p->m_numOfThreads; ++index) {
            if (m_threadInstances[index] != nullptr) {
                delete m_threadInstances[index];
            }
            if (m_startSem_pp[index] != nullptr) {
                delete m_startSem_pp[index];
            }
        }

        while (!m_configurationPoolList.isEmpty()) {
            auto element = m_configurationPoolList.takeFirst();
            element->PrepareRemove();
            delete element;
        }

        while (!m_configurationList.isEmpty()) {
            auto element = m_configurationList.takeFirst();
            element->PrepareRemove();
            delete element;
        }

        if (m_readySem_p != nullptr) {
            delete m_readySem_p;
        }

        if (m_holdupSem_p != nullptr) {
            delete m_holdupSem_p;
        }
    }

public:
    /* API */

    /* Call the base function to get the processing started */
    virtual void Start(QFile *qFile_p, char *workMem_p, int64_t workMemSize, TIA_t *TIA_p,
                       int priority, int startRow, int endRow, bool backward);
    double GetExecTime(void) {return m_execTime;}
    double GetProgress(void) {return m_progress;}
    void Cancel(void); /* Cancel pending operation */

public:
protected:
    /****/
    virtual void ConfigureChunkProcessing(void)
    {
        g_processingCtrl_p->SetNumOfProgressCounters(m_numberOfChunkThreads);
    }

    /* Override this function to configure m_threadInstances[threadIndex] */
    virtual bool ConfigureThread(CThreadConfiguration *config_p, Chunk_Description_t *chunkDescription_p,
                                 int32_t threadIndex);
    virtual CFileProcThreadBase *CreateProcThread(int32_t threadIndex, QSemaphore *readySem, QSemaphore *startSem,
                                                  QSemaphore *holdupSem, QMutex *configurationListMutex_p,
                                                  QList<CThreadConfiguration *> *configurationList_p,
                                                  QList<CThreadConfiguration *> *configurationListPool_p) = 0;
    virtual CThreadConfiguration *CreateConfigurationObject(void) = 0;
    virtual bool isProcessingDone(void);
    virtual void WrapUp(void) {}

    void Process(void);
    bool LoadNextChunk(void);

public:
    QMutex m_configurationListMutex;
    QList<CThreadConfiguration *> m_configurationPoolList;
    QList<CThreadConfiguration *> m_configurationList;

protected:
    QFile *m_qfile_p;
    int64_t m_fileSize;
    int64_t m_fileEndIndex; /* Where the chunk load will end */
    uint32_t m_fileSize_LDW;
    uint32_t m_fileSize_HDW;
    char *m_workMem_p;
    int64_t m_workMemSize;
    TIA_t *m_TIA_p;
    int m_priority;
    int m_totalNumOfRows; /* start - end row */
    int m_startRow; /* Zooming... restricting lines */
    int m_endRow; /* Zooming... restricting lines */
    bool m_backward; /* In case reading file backwards this flag is set */

    /* WORK DATA */
    CFileProcThreadBase *m_threadInstances[MAX_NUM_OF_THREADS]; /* Work data for the threads */
    QSemaphore *m_startSem_pp[MAX_NUM_OF_THREADS]; /* Used to trigger when thread shall start */
    QSemaphore *m_readySem_p; /* Used to indicate to ctrl when thread is ready to run again */

    /* Extra semaphore point to make all threads rally, used for extra safety in-case some of the threads pass
     * processing fast */
    QSemaphore *m_holdupSem_p;
    CTimeMeas m_timeExec; /* It's life time measure the total execution time */
    double m_execTime;
    double m_progress;
    Chunk_Description_t m_chunkDescr;
    int32_t m_linesPerThread;
    int32_t m_linesExtra;
    int32_t m_numberOfChunkThreads;

    /* In-case the thread processing should be on each line, not splitting the work (plugin typically). Each thread
     * has its own unique task */
    bool m_threadTI_Split;
    char m_tempString[CFG_TEMP_STRING_MAX_SIZE];
};
