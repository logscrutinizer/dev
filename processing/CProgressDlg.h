/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QSemaphore>
#include <QDialogButtonBox>
#include <QThread>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QGroupBox>
#include <QTimer>
#include <QPlainTextEdit>
#include <QLabel>
#include <QBitmap>
#include <QCheckBox>

#include <stdint.h>
#include "CDebug.h"
#include "globals.h"

typedef enum {
    ProgressCmd_Search_en = 0x55,
    ProgressCmd_Filter_en,
    ProgressCmd_LoadLog_en,
    ProgressCmd_Plot_en,
    ProgressCmd_TestProgress_en
}ProgressCmd_t;

class CProgressThread;

typedef struct {
    CProgressThread *progressThread;
    Qt::HANDLE parentThreadId;
    bool done;
    ProgressCmd_t cmd;
    QSemaphore *gui_sem_p;
} ProgressThread_t;

/***********************************************************************************************************************
*   CProgressThread
***********************************************************************************************************************/
class CProgressThread : public QThread
{
    Q_OBJECT

public:
    explicit CProgressThread(ProgressThread_t *data_p) : m_threadCfg_p(data_p) {}

    /****/
    virtual void run() override
    {
        g_RamLog->RegisterThread();

        auto unregisterRamLog = makeMyScopeGuard([&] () {
            g_RamLog->UnregisterThread();
        });

        Progress_ThreadMain();
    }

    uint32_t Progress_ThreadMain();

private:
    ProgressThread_t *m_threadCfg_p;
    uint32_t TestProgress();

signals:
    void ThreadProcessingComplete();
};

/***********************************************************************************************************************
*   CProgressDlg
***********************************************************************************************************************/
class CProgressDlg : public QDialog
{
    Q_OBJECT

public:
    CProgressDlg() = delete;
    CProgressDlg(QString title, ProgressCmd_t cmd, bool autoCloseOverride = false);

    virtual ~CProgressDlg() override {}

    QPixmap *MakeTransparent(const QImage& image, QRgb transparencyColor);
    void ExecThread(void);
    void UpdateProgressInfo(void);
    int OnPluginMsg(int wParam, int lParam);
    void OPostInitDialog();
    bool OnInitDialog();
    void OnPaint();

    virtual bool event(QEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

public slots:
    void OnBnClickedAbort();
    void OnTimer();
    void OnProcessingDone();

public:
    bool m_done; /* set when operation is done (e.g. filtering) */
    QSemaphore m_waitGUILock;

protected:
    ProgressThread_t m_threadCfg;
    bool m_autoClose;
    bool m_autoCloseOverride;
    bool m_HDD_Active;
    bool m_visible; /* true when dialog is fully visible */
    QString m_log;
    QPlainTextEdit *m_logWindow_p;
    QDialogButtonBox *m_buttonBox_p;
    QPushButton *m_abortButton_p;
    QProgressBar *m_progressBar_p;
    QVBoxLayout *m_mainLayout_p;
    QHBoxLayout *m_logLayout_p;
    QHBoxLayout *m_autoCheckLayout_p;
    QCheckBox *m_autoCloseCheck_p;
    QTimer *m_timer_p;
    QGroupBox *m_horizontalGroupBox_p;
    QGroupBox *m_autoCloseGroupBox_p;
    QLabel *m_bitmapLabel_p;
    QPixmap *m_hdd_pixmap_p;
    QPixmap *m_clear_hdd_pixmap_p;
    QPixmap *m_ok_pixmap_p;
    QPixmap *m_fail_pixmap_p;
};
