/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CProgressCtrl.h"
#include "CDebug.h"

#include <QApplication>
#include <QWidget>

static CProgressMgr procCtrl;
CProgressMgr *g_processingCtrl_p = nullptr;

/* Main for v-table generation */
CProgressMgr::~CProgressMgr(void)
{}

/***********************************************************************************************************************
*   CProgressMgr_Abort
***********************************************************************************************************************/
void CProgressMgr_Abort(void)
{
    /* This function can be used from anywhere in the system to force a shutdown of all running threads as quickly as
     * possible, e.g. when a crash to avoid data corruption */

    if (g_processingCtrl_p != nullptr) {
        g_processingCtrl_p->m_abort = true; /* Called by e.g. crash_handler .... */
    }
}

/***********************************************************************************************************************
*   ResetProgressInfo
***********************************************************************************************************************/
void CProgressMgr::ResetProgressInfo(void)
{
    m_exceptionTitle[0] = 0;
    m_exceptionInfo[0] = 0;
    InitProgressCounter();
}

/***********************************************************************************************************************
*   InitProgressCounter
***********************************************************************************************************************/
void CProgressMgr::InitProgressCounter(void)
{
    for (int index = 0; index < MAX_NUM_OF_THREADS; ++index) {
        m_progressCounter[index] = 0.0f;
    }
    m_timer.start();
}

/***********************************************************************************************************************
*   Processing_StartReport
***********************************************************************************************************************/
void CProgressMgr::Processing_StartReport(void)
{
    SetInit();
    ++m_processingLevel;
    PRINT_PROGRESS(QString("Processing - start, level %1").arg(m_processingLevel));
}

/***********************************************************************************************************************
*   Processing_StopReport
***********************************************************************************************************************/
void CProgressMgr::Processing_StopReport(void)
{
    if (m_processingLevel <= 1) {
        PRINT_CURSOR(QString("Restore cursor"));
        m_processingLevel = 0;
    } else {
        --m_processingLevel;
    }
    PRINT_PROGRESS(QString("Processing - stop, level %1").arg(m_processingLevel));
}

/***********************************************************************************************************************
*   AddProgressInfo
***********************************************************************************************************************/
void CProgressMgr::AddProgressInfo(const QString& info)
{
    QMutexLocker Lock(&m_mutex); /* RAII */
    m_progressInfo.append(info);
}

/***********************************************************************************************************************
*   GetProgressInfo
***********************************************************************************************************************/
bool CProgressMgr::GetProgressInfo(QString& info)  /* return true if more to read. */
{
    QMutexLocker Lock(&m_mutex); /* RAII */
    if (m_progressInfo.isEmpty()) {
        info = QString("");
        return false;
    }
    info = m_progressInfo.takeFirst();
    return m_progressInfo.isEmpty() ? false : true;
}

/***********************************************************************************************************************
*   SetProgressCounter
***********************************************************************************************************************/
void CProgressMgr::SetProgressCounter(float value)
{
    const int COUNT = m_numOfProgressCounters;

    for (int index = 0; index < COUNT; ++index) {
        m_progressCounter[index] = value;
    }
}

/***********************************************************************************************************************
*   StepProgressCounter
***********************************************************************************************************************/
void CProgressMgr::StepProgressCounter(int counterIndex)
{
    if (counterIndex >= m_numOfProgressCounters) {
        TRACEX_ERROR("CProgressMgr::StepProgressCounter  Counter out of range");
        counterIndex = 0;
    }

    m_progressCounter[counterIndex] += m_progressStep;
}
