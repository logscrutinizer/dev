/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QMutex>
#include "CConfig.h"

#include <stdint.h>
#include <QElapsedTimer>

#define PROCESSING_CURSOR_WAIT    0
#define PROCESSING_CURSOR_MOVE    1

#define MAX_PROGRESS_INFO         128
#define MAX_PROGRESS_INFO_LENGTH  256
#define EXCEPTION_INFO_STRING_LENGTH  1024

typedef struct {
    QString info;
}ProgressInfo_t;

class CProgressMgr;

extern CProgressMgr *g_processingCtrl_p;

/***********************************************************************************************************************
*   CProgressMgr
***********************************************************************************************************************/
class CProgressMgr
{
public:
    CProgressMgr(void)
    {
        m_processingLevel = 0;
        g_processingCtrl_p = this;
    }
    virtual ~CProgressMgr(void);

    /* The count step is a  precentile  of 100%. Each call to StepProgress will increase the
     *  counter until reaching * 1.0 */
    void SetupProgessCounter(double countStep) {m_progressStep = countStep;}

    /****/
    void GetProgressCounter(double **progressCounters_pp, int *count_p) {
        *progressCounters_pp = &m_progressCounter[0];
        *count_p = m_numOfProgressCounters;
    }

    void Processing_StartReport(void);
    void Processing_StopReport();
    void ResetProgressInfo(void);
    void AddProgressInfo(const QString& info);
    bool GetProgressInfo(QString& info);  /* Returns true if there are more strings to fetch */
    void InitProgressCounter(void);
    void SetNumOfProgressCounters(int numOfProgressCounters) {m_numOfProgressCounters = numOfProgressCounters;}
    void SetProgressCounter(double value); /* To align the * progress * this could * be used to * "jump" * forward */
    void StepProgressCounter(int counterIndex = 0);
    void SetSuccess(void) {m_success = 1; SetProgressCounter(1.0);}
    void SetFail(void) {m_success = 0; SetProgressCounter(1.0);}
    void SetInit(void) {m_success = -1; SetProgressCounter(0.0);}
    void SetFileOperationOngoing(bool ongoing) {m_fileOperationOngoing = ongoing;}
    bool isProcessing(void) {return m_processingLevel > 0 ? true : false;}

public slots:
    int m_success = 0; /* Indicate if the operation is successful or not, -1 no score, 0 fail, 1 success */
    bool m_abort = false;
    bool m_isException = false; /* True if the m_abort was flagged because of an exception */
    char m_exceptionTitle[EXCEPTION_INFO_STRING_LENGTH];
    char m_exceptionInfo[EXCEPTION_INFO_STRING_LENGTH];
    bool m_fileOperationOngoing = false;

private:
    int m_processingLevel = 0; /* Used to indicate how many that has reported them being processing */
    int m_numOfProgressCounters = 0;
    QBasicMutex m_mutex;
    double m_progressStep = 0; /* Each call to StepProgress will increase m_progressCounter with this value */
    double m_progressCounter[MAX_NUM_OF_THREADS];  /* Keeping count of the current progress..  1.0 means done */
    QList<QString> m_progressInfo;
    QElapsedTimer m_timer;
};
