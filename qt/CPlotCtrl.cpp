/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdlib.h>

#include "CDebug.h"
#include "cplotctrl.h"
#include "../processing/CProgressCtrl.h"

/***********************************************************************************************************************
*   FileIndex_To_MemRef
***********************************************************************************************************************/
inline char *FileIndex_To_MemRef(int64_t *fileIndex_p, int64_t *workMemFileIndex_p, char *WorkMem_p)
{
    return (reinterpret_cast<char *>(WorkMem_p + (*fileIndex_p - *workMemFileIndex_p)));
}

/***********************************************************************************************************************
*   thread_Process
***********************************************************************************************************************/
void CPlotThread::thread_Process(CThreadConfiguration *config_p)
{
    TRACEX_DE(
        "CPlotThread::thread_Process  0x%x   m_start_TIA_index:%d m_stop_TIA_Index:%d m_TIA_step:%d",
        this, config_p->m_start_TIA_index, config_p->m_stop_TIA_Index, config_p->m_TIA_step)

    int TIA_Index = config_p->m_start_TIA_index;    /* use local variable for quicker access */
    const int stop_TIA_Index = config_p->m_stop_TIA_Index;
    const int TIA_step = config_p->m_TIA_step;
    const TIA_t *TIA_p = config_p->m_TIA_p;
    int progressCount = PROGRESS_COUNTER_STEP;

    try {
        while (TIA_Index < stop_TIA_Index && !g_processingCtrl_p->m_abort) {
            int textLength = TIA_p->textItemArray_p[TIA_Index].size;
            char *text_p = FileIndex_To_MemRef(&TIA_p->textItemArray_p[TIA_Index].fileIndex,
                                               &config_p->m_chunkDescr.fileIndex, config_p->m_workMem_p);

            --progressCount;

            if (progressCount == 0) {
                g_processingCtrl_p->StepProgressCounter(m_threadIndex);
                progressCount = PROGRESS_COUNTER_STEP;
            }

            m_plot_p->PlotRow(text_p, &textLength, TIA_Index);
            TIA_Index += TIA_step;
        }
    } catch (int e) {
        g_processingCtrl_p->m_abort = true;
        g_processingCtrl_p->m_isException = true;

        char *title_p;
        char *x_axis_p;

        m_plot_p->GetTitle(&title_p, &x_axis_p);

        QString error("LogScrutinizer stopped generating plot data since the plugin Plot: %1 has crashed.\n"
                      "Please unload that plugin and contact the plugin supplier.\nWhen a plugin crashes it "
                      "is possible that it corrupts the internal states of the\n LogScruitnizer, "
                      "hence it might be a good idea to save your workspace as well. Error:%2");

        error = error.arg(title_p).arg(e);

        TRACEX_DE(g_processingCtrl_p->m_exceptionInfo)
    }

    (void)thread_ProcessingDone();
}

/***********************************************************************************************************************
*   Start_PlotProcessing
***********************************************************************************************************************/
void CPlotCtrl::Start_PlotProcessing(QFile *qFile_p, char *workMem_p, int64_t workMemSize, TIA_t *TIA_p,
                                     int priority, QList<CPlot *> *pendingPlot_execList_p, int startRow,
                                     int endRow)
{
    TRACEX_I("Plugin plot generation started   startRow:%d endRow:%d", startRow, endRow)
    g_processingCtrl_p->AddProgressInfo(QString("Starting plot generation"));

    m_pendingPlot_execList_p = pendingPlot_execList_p;

    /* Make sure that each thread work with all line, since each thread has its own plugin to work with */
    m_threadTI_Split = false;

    int savedNumOfThreads = g_cfg_p->m_numOfThreads;          /* override temporary */
    g_cfg_p->m_numOfThreads = m_pendingPlot_execList_p->count();     /* override temporary */

    CFileProcBase::Start(qFile_p, workMem_p, workMemSize, TIA_p, priority, startRow, endRow, false /*backward*/);
    g_cfg_p->m_numOfThreads = savedNumOfThreads;
}

/***********************************************************************************************************************
*   ConfigureThread
***********************************************************************************************************************/
bool CPlotCtrl::ConfigureThread(CThreadConfiguration *config_p, Chunk_Description_t *chunkDescription_p,
                                int32_t threadIndex)
{
    CFileProcBase::ConfigureThread(config_p, chunkDescription_p, threadIndex);    /* Setup thread */

    auto plotThread_p = reinterpret_cast<CPlotThread *>(m_threadInstances[threadIndex]);

    if (!plotThread_p->m_isConfiguredOnce) {
        plotThread_p->m_isConfiguredOnce = true;
        plotThread_p->m_plot_p = m_pendingPlot_execList_p->at(threadIndex);

        char *title_p;
        char *x_axis_p;
        plotThread_p->m_plot_p->GetTitle(&title_p, &x_axis_p);
        g_processingCtrl_p->AddProgressInfo(QString("  Configuring plot generation for: %1, (%2)")
                                                .arg(title_p).arg(threadIndex));
    }
    return true;
}

/***********************************************************************************************************************
*   CreateConfigurationObject
***********************************************************************************************************************/
CThreadConfiguration *CPlotCtrl::CreateConfigurationObject(void)
{
    return new CThreadConfiguration();
}

/***********************************************************************************************************************
*   WrapUp
***********************************************************************************************************************/
void CPlotCtrl::WrapUp(void)
{
    if (!g_processingCtrl_p->m_abort) {
        g_processingCtrl_p->SetSuccess();
        g_processingCtrl_p->AddProgressInfo(QString("Plot finished"));
    } else {
        g_processingCtrl_p->SetFail();
    }
    TRACEX_DE("CPlotCtrl::WrapUp")
}
