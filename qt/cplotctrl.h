/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stdlib.h>

#include "CFilter.h"
#include "CConfig.h"
#include "CFileProcBase.h"
#include "plugin_utils.h"
#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlotThread
***********************************************************************************************************************/
class CPlotThread : public CFileProcThreadBase
{
public:
    CPlotThread(int32_t threadIndex,
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
    {
        m_isConfiguredOnce = false;
    }

    virtual ~CPlotThread(void)
    {
        /* Not sure if anything should be put here, see ExitInstance instead */
    }

public:
    CPlot *m_plot_p;

protected:
    virtual void thread_Process(CThreadConfiguration *config_p);

private:
};

/***********************************************************************************************************************
*   CPlotCtrl
***********************************************************************************************************************/
class CPlotCtrl : public CFileProcBase
{
public:
    CPlotCtrl(void) {}
    virtual ~CPlotCtrl(void) override {}

public:
    /* API */
    void Start_PlotProcessing(QFile *qFile_p,
                              char *workMem_p,
                              int64_t workMemSize,
                              TIA_t *TIA_p,
                              int priority,
                              QList<CPlot *> *pendingPlot_execList_p,
                              int startRow,
                              int endRow);

protected:
    /* Overrides */

    virtual bool ConfigureThread(CThreadConfiguration *config_p,
                                 Chunk_Description_t *chunkDescription_p,
                                 int32_t threadIndex) override;
    virtual CThreadConfiguration *CreateConfigurationObject(void) override;

    /****/
    virtual CFileProcThreadBase *CreateProcThread(int32_t threadIndex,
                                                  QSemaphore *readySem,
                                                  QSemaphore *startSem,
                                                  QSemaphore *holdupSem,
                                                  QMutex *configurationListMutex_p,
                                                  QList<CThreadConfiguration *> *configurationList_p,
                                                  QList<CThreadConfiguration *> *configurationListPool_p) override
    {
        return new CPlotThread(threadIndex,
                               readySem,
                               startSem,
                               holdupSem,
                               configurationListMutex_p,
                               configurationList_p,
                               configurationListPool_p);
    }
    virtual void WrapUp(void) override;

private:
    QList<CPlot *> *m_pendingPlot_execList_p; /* List of plots to process */
};
