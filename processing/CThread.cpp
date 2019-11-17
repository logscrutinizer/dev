/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdlib.h>
#include <QSemaphore>
#include <QThread>

#include "CThread.h"
#include "CDebug.h"
#include "CConfig.h"
#include "utils.h"
#include "globals.h"

/***********************************************************************************************************************
*   run
***********************************************************************************************************************/
void CWorker::run(void)
{
    g_RamLog->RegisterThread();

    auto unregisterRamLog = makeMyScopeGuard([&] () {
        g_RamLog->UnregisterThread();
    });

#ifdef _DEBUG
    TRACEX_D("Thread:%p Cmd:%d", m_data_p)
#endif

    m_data_p->cmdSync_sem_p->acquire();

#ifdef _DEBUG
    TRACEX_D("Thread:0x%x Cmd:%d", m_data_p, m_data_p->command)
#endif

    while (m_data_p->command & (THREAD_CMD_START | THREAD_CMD_CONTINUE)) {
        m_data_p->action_p(m_data_p->data_p);
        m_data_p->doneSync_sem_p->release();
        m_data_p->cmdSync_sem_p->acquire();

#ifdef _DEBUG
        TRACEX_D("Thread:%p Cmd:%d", m_data_p, m_data_p->command)
#endif
    }

#ifdef _DEBUG
    TRACEX_D("Thread:%p Cmd:%d", m_data_p, m_data_p->command)
#endif
    m_data_p->killed_sem_p->release();
}

CThreadManager::CThreadManager()
{
    m_numOfThreads = g_cfg_p->m_numOfThreads;

    m_threadInstanceArray =
        static_cast<threadInstance_base_t *>(malloc(sizeof(threadInstance_base_t) *
                                                    static_cast<size_t>(m_numOfThreads)));
    if (m_threadInstanceArray == nullptr) {
        TRACEX_E("CThreadManager::CThreadManager  m_threadInstanceArray is nullptr")
        return;
    }
    memset(m_threadInstanceArray, 0, sizeof(threadInstance_base_t) * static_cast<size_t>(m_numOfThreads));

    m_hThreadArray_pp = static_cast<CWorker **>(malloc(sizeof(CWorker *) * static_cast<size_t>(m_numOfThreads)));
    if (m_hThreadArray_pp == nullptr) {
        TRACEX_E("CThreadManager::CThreadManager  m_hThreadArray_pp is nullptr")
        return;
    }
    memset(m_hThreadArray_pp, 0, sizeof(QThread *) * static_cast<size_t>(m_numOfThreads));

    InitializeThreads();
}

CThreadManager::~CThreadManager()
{
    Q_ASSERT(m_numOfThreads == 0);  /* KillThreads must be called before destructor */

    for (int threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        IF_NOT_NULL_DELETE_AND_SET_NULL(m_threadInstanceArray[threadIndex].cmdSync_sem_p);
        IF_NOT_NULL_DELETE_AND_SET_NULL(m_threadInstanceArray[threadIndex].doneSync_sem_p);
        IF_NOT_NULL_DELETE_AND_SET_NULL(m_threadInstanceArray[threadIndex].killed_sem_p);
        IF_NOT_NULL_DELETE_AND_SET_NULL(m_hThreadArray_pp[threadIndex]);
    }

    if (m_threadInstanceArray != nullptr) {
        free(m_threadInstanceArray);
    }

    if (m_hThreadArray_pp != nullptr) {
        free(m_hThreadArray_pp);
    }
}

/***********************************************************************************************************************
*   InitializeThreads
***********************************************************************************************************************/
void CThreadManager::InitializeThreads(void)
{
    /* Create threads and semaphores and have them wait */

    /* Initialize threads, start them, and let them wait for first cmd */

    for (int threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        /* Initial count is 0, so it is not signalled, thread will wait */
        m_threadInstanceArray[threadIndex].doneSync_sem_p = new QSemaphore();
        m_threadInstanceArray[threadIndex].cmdSync_sem_p = new QSemaphore();
        m_threadInstanceArray[threadIndex].killed_sem_p = new QSemaphore();
        m_threadInstanceArray[threadIndex].command = THREAD_CMD_STOP;

        m_hThreadArray_pp[threadIndex] = new CWorker();
        TRACEX_DE(QString("%1 - Creating thread %2").arg(__FUNCTION__).arg(threadIndex))
        m_hThreadArray_pp[threadIndex]->setData(&m_threadInstanceArray[threadIndex]);

        if (m_hThreadArray_pp[threadIndex] == nullptr) {
            TRACEX_E("CThreadManager::InitializeThreads - Creating thread %u failed", threadIndex)
            return;
        }
        m_hThreadArray_pp[threadIndex]->start(); /* The thread will stop at wait for cmdSync_sem_p */
    }
}

/***********************************************************************************************************************
*   StartConfiguredThreads
***********************************************************************************************************************/
void CThreadManager::StartConfiguredThreads(void)
{
    /* Start/Release the threads */
    for (int threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        if (m_threadInstanceArray[threadIndex].isConfigured) {
            TRACEX_DE(QString("%1 - Started %2").arg(__FUNCTION__).arg(threadIndex))
            m_threadInstanceArray[threadIndex].cmdSync_sem_p->release();
        }
    }
}

/***********************************************************************************************************************
*   WaitForAllThreads
***********************************************************************************************************************/
void CThreadManager::WaitForAllThreads(void)
{
    int threadIndex;

    for (threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        if (m_threadInstanceArray[threadIndex].isConfigured) {
            TRACEX_DE(QString("%1 - Waiting for %2").arg(__FUNCTION__).arg(threadIndex))
            m_threadInstanceArray[threadIndex].doneSync_sem_p->acquire();
            TRACEX_DE(QString("%1 - Done waiting for %2").arg(__FUNCTION__).arg(threadIndex))
        }
    }

    for (threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        m_threadInstanceArray[threadIndex].isConfigured = false;
    }
}

/***********************************************************************************************************************
*   ConfigureThread
***********************************************************************************************************************/
void CThreadManager::ConfigureThread(int index, ThreadAction_fptr_t action_p, void *data_p)
{
    m_threadInstanceArray[index].data_p = data_p;
    m_threadInstanceArray[index].action_p = action_p;
    m_threadInstanceArray[index].isConfigured = true;
    m_threadInstanceArray[index].command = THREAD_CMD_CONTINUE;
    TRACEX_DE(QString("%1 - Configured %2").arg(__FUNCTION__).arg(index))
}

/***********************************************************************************************************************
*   KillThreads
***********************************************************************************************************************/
void CThreadManager::KillThreads(void)
{
    int threadIndex;

    if (m_numOfThreads == 0) {
        /*already called */
        return;
    }

    /* Kill all threads */
    for (threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        m_threadInstanceArray[threadIndex].command = THREAD_CMD_STOP;
        TRACEX_DE(QString("%1 - Stopping %2").arg(__FUNCTION__).arg(threadIndex))
        m_threadInstanceArray[threadIndex].cmdSync_sem_p->release();
    }

    for (threadIndex = 0; threadIndex < m_numOfThreads; ++threadIndex) {
        TRACEX_DE(QString("%1 - Waiting for %2").arg(__FUNCTION__).arg(threadIndex))
        m_threadInstanceArray[threadIndex].killed_sem_p->acquire();
        TRACEX_DE(QString("%1 - Done wainting for %2").arg(__FUNCTION__).arg(threadIndex))
    }

    m_numOfThreads = 0;
}
