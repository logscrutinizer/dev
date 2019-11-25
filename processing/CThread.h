/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stdlib.h>
#include <QThread>
#include <QSemaphore>

typedef enum
{
    THREAD_CMD_START = 0x01,

    /* First time a thread starts it is told to start */
    THREAD_CMD_CONTINUE = 0x02,

    /* For concequtive runs the thread is given continue commands */
    THREAD_CMD_STOP = 0x04,

    /* The stop command is given when a thread has done all loops successfully */
    THREAD_CMD_ABORT = 0x08    /* Abort is given when a thread shall exit directly */
}ThreadCmd_e;

typedef void (*ThreadAction_fptr_t)(volatile void *);   /* void is some kind of data point, up to the called function to
                                                         * define what */

typedef struct
{
    volatile ThreadCmd_e command; /* For */
    QSemaphore *cmdSync_sem_p; /* Used to stop thread such that a new command can be given, thread is released when
                                * thread shall read the new cmd */
    QSemaphore *doneSync_sem_p; /* When the thread is done with its cmd the doneSync_sem is released, and thread can be
                                 * given a new cmd */
    QSemaphore *killed_sem_p; /* When the thread has stopped and returned */
    uint32_t threadID;
    volatile bool isConfigured; /* When data is set to the thread this flag is set. When the thread reaches its stop
                                 * point this is reset */
    volatile void *data_p;
    ThreadAction_fptr_t action_p; /* What action to perform */
}threadInstance_base_t;

/***********************************************************************************************************************
*   CWorker
***********************************************************************************************************************/
class CWorker : public QThread
{
    void run() override;

public:
    void setData(threadInstance_base_t *data_p) {m_data_p = data_p;}

private:
    threadInstance_base_t *m_data_p;
};

/***********************************************************************************************************************
*   CThreadManager
***********************************************************************************************************************/
class CThreadManager
{
public:
    CThreadManager();
    ~CThreadManager();

    int GetThreadCount(void) {return m_numOfThreads;}
    void StartConfiguredThreads(void);
    void WaitForAllThreads(void);
    void ConfigureThread(int index, ThreadAction_fptr_t action_p, void *data_p);
    void PrepareDelete(void) {KillThreads();}

private:
    void KillThreads(void);
    void InitializeThreads(void);

    threadInstance_base_t *m_threadInstanceArray; /* Work data for the threads */
    CWorker **m_hThreadArray_pp; /* Used to wait for all threads to exit before it is possible to close the thread
                                  * handles */
    int m_numOfThreads;
};
