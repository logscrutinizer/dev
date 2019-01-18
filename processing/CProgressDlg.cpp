/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CConfig.h"
#include "CLogScrutinizerDoc.h"
#include "CProgressDlg.h"
#include "CProgressCtrl.h"
#include "globals.h"
#include "utils.h"

#include <QFontMetrics>

CProgressDlg::CProgressDlg(QString title, ProgressCmd_t cmd, bool autoCloseOverride)
    : QDialog(nullptr, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
{
    memset(&m_threadCfg, 0, sizeof(ProgressThread_t));

    m_autoClose = g_cfg_p->m_autoCloseProgressDlg;
    m_visible = false;
    m_autoCloseOverride = autoCloseOverride;
    m_done = false;
    m_HDD_Active = false;

    /*m_bmpRectSet = false; */

    m_threadCfg.cmd = cmd;

    if (g_processingCtrl_p != nullptr) {
        g_processingCtrl_p->InitProgressCounter();
        g_processingCtrl_p->ResetProgressInfo();
        m_log.clear();
    }

    m_mainLayout_p = new QVBoxLayout;

    m_buttonBox_p = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox_p, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox_p, SIGNAL(rejected()), this, SLOT(reject()));

    m_abortButton_p = new QPushButton("&Abort", this);
    Q_ASSERT(connect(m_abortButton_p, SIGNAL(released()), this, SLOT(OnBnClickedAbort())));

    std::unique_ptr<QFontMetrics> fontMetric = std::make_unique<QFontMetrics>(QFont());
    QRect rect =
        fontMetric->boundingRect(tr("This is a pretty long string that should be able to represent much text"));

    m_logWindow_p = new QPlainTextEdit(m_log);
    m_logWindow_p->setPlainText("Processing");
    m_logWindow_p->setMinimumWidth(rect.width());

    m_autoCloseCheck_p = new QCheckBox();
    m_autoCloseCheck_p->setChecked(m_autoClose);

    m_timer_p = new QTimer;
    connect(m_timer_p, SIGNAL(timeout()), this, SLOT(OnTimer()));
    m_timer_p->start(200);

    m_progressBar_p = new QProgressBar();
    m_progressBar_p->setValue(0);
    m_progressBar_p->setMinimum(0);
    m_progressBar_p->setMaximum(100);

    m_mainLayout_p->addWidget(m_progressBar_p);

    m_hdd_pixmap_p = new QPixmap(QPixmap::fromImage(MakeTransparantImage(":hdd_64.bmp", Q_RGB(0xf9, 0xf4, 0xf4))));
    m_ok_pixmap_p = new QPixmap(QPixmap::fromImage(MakeTransparantImage(":ok_64.bmp", Q_RGB(0xf9, 0xf4, 0xf4))));
    m_fail_pixmap_p = new QPixmap(QPixmap::fromImage(MakeTransparantImage(":fail_64.bmp", Q_RGB(0xf9, 0xf4, 0xf4))));

    if ((m_hdd_pixmap_p == nullptr) || (m_ok_pixmap_p == nullptr) || (m_fail_pixmap_p == nullptr)) {
        TRACEX_E("Internal error: Failed to load progress bitmaps");
    }

    m_clear_hdd_pixmap_p = new QPixmap(*m_hdd_pixmap_p);
    m_clear_hdd_pixmap_p->fill(Qt::transparent);

    m_bitmapLabel_p = new QLabel(this);
    m_bitmapLabel_p->setPixmap(*m_clear_hdd_pixmap_p);
    m_bitmapLabel_p->setStyleSheet("background-color: rgba(249, 244, 244, 0%)");

    m_horizontalGroupBox_p = new QGroupBox(tr("Log window"));
    m_autoCloseGroupBox_p = new QGroupBox(tr(""));

    m_logLayout_p = new QHBoxLayout;

    m_logLayout_p->addWidget(m_logWindow_p);
    m_logLayout_p->addWidget(m_bitmapLabel_p);

    m_autoCheckLayout_p = new QHBoxLayout;
    m_autoCheckLayout_p->addWidget(m_autoCloseCheck_p);
    m_autoCheckLayout_p->addWidget(new QLabel((tr("Auto close"))));
    m_autoCheckLayout_p->addStretch();

    m_horizontalGroupBox_p->setLayout(m_logLayout_p);
    m_autoCloseGroupBox_p->setLayout(m_autoCheckLayout_p);
    m_mainLayout_p->addWidget(m_horizontalGroupBox_p);
    m_mainLayout_p->addWidget(m_autoCloseGroupBox_p);
    m_mainLayout_p->addWidget(m_abortButton_p);

    setLayout(m_mainLayout_p);
    setWindowTitle(title);

    setCursor(Qt::WaitCursor);

    OnInitDialog();
}

/***********************************************************************************************************************
*   OnTimer
***********************************************************************************************************************/
void CProgressDlg::OnTimer()
{
    UpdateProgressInfo();
}

/***********************************************************************************************************************
*   UpdateProgressInfo
***********************************************************************************************************************/
void CProgressDlg::UpdateProgressInfo(void)
{
    if ((g_processingCtrl_p != nullptr) && m_visible) {
        float *progressCounters_p;
        int numOfProgressCounters;
        float progress;

        g_processingCtrl_p->GetProgressCounter(&progressCounters_p, &numOfProgressCounters);

        if (numOfProgressCounters > 1) {
            const int COUNT = numOfProgressCounters;

            progress = 1.0f;                   /* Keeps the smallest progress value */

            for (int index = 0; index < COUNT; ++index) {
                if (progressCounters_p[index] < progress) {
                    progress = progressCounters_p[index];
                }
            }
        } else if (numOfProgressCounters == 1) {
            progress = progressCounters_p[0];
        } else {
            progress = 0.0f;
        }

        m_progressBar_p->setValue(progress * 100.0f);

        bool moreToRead;
        do {
            QString info;
            moreToRead = g_processingCtrl_p->GetProgressInfo(info);
            if (!info.isEmpty() /* && m_logWindow_p->isVisible()*/) {
                m_logWindow_p->appendPlainText(info);
                TRACEX_I(info);
            }
        } while (moreToRead);

        /*    m_logCtrl.LineScroll(m_logCtrl.GetLineCount()); */
    }

    if (g_processingCtrl_p->m_abort || (g_processingCtrl_p->m_success == 0)) {
        /* fail */
        m_bitmapLabel_p->setPixmap(*m_fail_pixmap_p);
        PRINT_PROGRESS(QString("Progress abort or fail %1 %2\n")
                           .arg(g_processingCtrl_p->m_abort)
                           .arg(g_processingCtrl_p->m_success));
        m_timer_p->stop();
    } else if (g_processingCtrl_p->m_success == 1) {
        m_bitmapLabel_p->setPixmap(*m_ok_pixmap_p);
        PRINT_PROGRESS(QString("Progress Success\n"));
        m_timer_p->stop();
    } else if (g_processingCtrl_p->m_fileOperationOngoing) {
        PRINT_PROGRESS(QString("Progress HDD active:%1\n").arg(m_HDD_Active));
        if (!m_HDD_Active) {
            m_HDD_Active = true;
            m_bitmapLabel_p->setPixmap(*m_hdd_pixmap_p);
        }
    } else {
        if (m_HDD_Active) {
            m_HDD_Active = false;
            m_bitmapLabel_p->setPixmap(*m_clear_hdd_pixmap_p);
        }
    }
}

/***********************************************************************************************************************
*   OnBnClickedAbort
***********************************************************************************************************************/
void CProgressDlg::OnBnClickedAbort()
{
    m_autoClose = (int)m_autoCloseCheck_p->isChecked();

    if (m_autoClose != (int)g_cfg_p->m_autoCloseProgressDlg) {
        g_cfg_p->m_autoCloseProgressDlg = m_autoClose;
    }

    m_timer_p->stop();

    if (!m_done) {
        g_processingCtrl_p->m_abort = true;
        m_abortButton_p->setText("Aborting...");
        m_abortButton_p->setDisabled(true);
        return;
    } else {
        reject();

        /*   close(); */
    }
}

/***********************************************************************************************************************
*   OnInitDialog
***********************************************************************************************************************/
bool CProgressDlg::OnInitDialog()
{
    ExecThread();

    if (m_autoCloseOverride) {
        /*checkBox_p->EnableWindow(false); */
    } else {
        /*checkBox_p->EnableWindow(true); */
    }

    return true;
}

/***********************************************************************************************************************
*   event
***********************************************************************************************************************/
bool CProgressDlg::event(QEvent *event)
{
    int returnValue = QWidget::event(event);

    if (event->type() == QEvent::Polish) {}

    return returnValue;
}

/***********************************************************************************************************************
*   showEvent
***********************************************************************************************************************/
void CProgressDlg::showEvent(QShowEvent *event)
{
    m_visible = true;

    /* call whatever your base class is! */
    QDialog::showEvent(event);

    if (event->spontaneous()) {
        return;
    }

    UpdateProgressInfo();
}

/***********************************************************************************************************************
*   OnPluginMsg
***********************************************************************************************************************/
int CProgressDlg::OnPluginMsg(int wParam, int lParam)
{
    return 1;
}

/***********************************************************************************************************************
*   ExecThread
***********************************************************************************************************************/
void CProgressDlg::ExecThread(void)
{
    m_threadCfg.done = false;
    m_threadCfg.parentThreadId = QThread::currentThreadId();
    m_threadCfg.gui_sem_p = &m_waitGUILock;
    m_threadCfg.progressThread = new CProgressThread(&m_threadCfg);

    connect(m_threadCfg.progressThread, SIGNAL(ThreadProcessingComplete()), this, SLOT(OnProcessingDone()));

    g_processingCtrl_p->Processing_StartReport();

    m_threadCfg.progressThread->start();
}

/***********************************************************************************************************************
*   OnProcessingDone
***********************************************************************************************************************/
void CProgressDlg::OnProcessingDone()
{
    m_done = true;
    TRACEX_I("Processing Done!");

    g_processingCtrl_p->Processing_StopReport();

    m_autoClose = (int)m_autoCloseCheck_p->isChecked();
    if (m_autoClose != (int)g_cfg_p->m_autoCloseProgressDlg) {
        g_cfg_p->m_autoCloseProgressDlg = m_autoClose;
    }

    unsetCursor();
    if (m_autoClose || m_autoCloseOverride || g_processingCtrl_p->m_abort) {
        accept();
        return;
    }

    m_abortButton_p->setDisabled(false);
    m_abortButton_p->setText("Close");
    UpdateProgressInfo();
}

/***********************************************************************************************************************
*   Progress_ThreadMain
***********************************************************************************************************************/
uint32_t CProgressThread::Progress_ThreadMain()
{
    extern CLogScrutinizerDoc *GetTheDoc(void);
    CLogScrutinizerDoc *doc_p = GetTheDoc();

#ifdef _DEBUG
    TRACEX_DE("Progress_ThreadMain:0x%llx Cmd:%d", m_threadCfg_p, m_threadCfg_p->cmd);
#endif

    switch (m_threadCfg_p->cmd)
    {
        case ProgressCmd_Search_en:
            doc_p->ExecuteSearch();
            break;

        case ProgressCmd_Filter_en:
            doc_p->ExecuteFiltering();
            break;

        case ProgressCmd_LoadLog_en:
            doc_p->ExecuteLoadLog();
            break;

        case ProgressCmd_Plot_en:
            doc_p->ExecutePlot();
            break;

        case ProgressCmd_TestProgress_en:
            TestProgress();
            break;
    }

    if (!m_threadCfg_p->gui_sem_p->available()) {
        m_threadCfg_p->gui_sem_p->release();
    }

    emit ThreadProcessingComplete();

    return 1;
}

/***********************************************************************************************************************
*   TestProgress
***********************************************************************************************************************/
uint32_t CProgressThread::TestProgress()
{
    /*CProgressDlg dlg("Searching...", ProgressCmd_Search_en);
     *  dlg.setModal(true);
     *  dlg.exec();*/

    /* g_processingCtrl_p->SetupProgessCounter(currentStep);
    * g_processingCtrl_p->SetProgressCounter((float)(m_chunkDescr.TIA_startRow - m_startRow) /
    * (float)m_totalNumOfRows);
    * g_processingCtrl_p->AddProgressInfo(m_tempString, strlen(m_tempString));
    * g_processingCtrl_p->m_abort = false;
    * g_processingCtrl_p->SetFileOperationOngoing(false); */
    g_processingCtrl_p->InitProgressCounter();
    g_processingCtrl_p->SetupProgessCounter(1.0f / 1000.0f); /* 1000 ticks until completion */
    g_processingCtrl_p->SetNumOfProgressCounters(1);

    for (int i = 0; i < 1000 && !g_processingCtrl_p->m_abort; ++i) {
        msleep(1);
        if (((i > 200) && (i < 500)) || ((i > 800) && (i < 900))) {
            g_processingCtrl_p->m_fileOperationOngoing = true;
        } else {
            g_processingCtrl_p->m_fileOperationOngoing = false;
        }
        g_processingCtrl_p->StepProgressCounter();
    }

    g_processingCtrl_p->m_success = true;
    g_processingCtrl_p->SetProgressCounter(1.0);

    return 0;
}
