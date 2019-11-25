/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "mainwindow.h"

#include "CDebug.h"
#include "CRecentFile.h"
#include "CMemPool.h"
#include "CFileCtrl.h"
#include "CSearchCtrl.h"
#include "CFileProcBase.h"
#include "CFilterProcCtrl.h"
#include "CRowCache.h"

#include "TextDecoration.h"
#include "CParseTesting.h"
#include "CLogScrutinizerDoc.h"
#include "CProgressDlg.h"
#include "CWorkspace.h"
#include "CConfigurationCtrl.h"
#include "utils/utils.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QDir>
#include <QFileDevice>
#include <QSplitter>
#include <QHeaderView>
#include <QListView>
#include <QTableView>
#include <QTreeView>
#include <QProcess>
#include <QFontDatabase>
#include <QCommandLineParser>

#ifdef _WIN32
 #include "../crash/crash_handler_win.h"
#else
 #include "../crash/crash_handler_linux.h"
 #include <sys/time.h>
 #include <sys/resource.h>
#endif

#ifdef WIN32

/* AllowSetForegroundWindow */
 #include "Windows.h"
#endif

void SaveDefaultCSS(void);

static QApplication *g_app = nullptr;
extern int TestHS(void);

/***********************************************************************************************************************
*   MW_getApp
***********************************************************************************************************************/
QApplication *MW_getApp(void)
{
    return g_app;
}

/***********************************************************************************************************************
*   APP_UPDATE_CSS
***********************************************************************************************************************/
void APP_UPDATE_CSS(void)
{
    QFile File("stylesheet.qss");

    if (File.open(QFile::ReadOnly)) {
        QString StyleSheet = QLatin1String(File.readAll());
        g_app->setStyleSheet(StyleSheet);
    }

    File.close();
}

/***********************************************************************************************************************
*   main
***********************************************************************************************************************/
int main(int argc, char *argv[])
{
#ifndef _WIN32

    /* Enable dumps core dumps may be disallowed by parent of this process; change that */
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);
#endif

    QApplication app(argc, argv);
    initUpperChar(); /* Converstion table for small/large caps */

    CConfig cfg;

    QCoreApplication::setApplicationName(APP_TITLE);
    QCoreApplication::setApplicationVersion(cfg.GetApplicationVersion());
    QCoreApplication::setOrganizationName("SoftFairy");
    QCoreApplication::setOrganizationDomain("logscrutinizer.com");

    CDebug debug;
    CRamLog ramLog;
    QString title, version, buildDate, configuration;
    QDir dir;
    QString workingDir = CFGCTRL_GetWorkingDir();

    dir.mkpath(workingDir);
    dir.setCurrent(workingDir);

    g_DebugLib->Initialize();

#ifdef _DEBUG
    g_RamLog->TestRamLogs();
#endif

    g_RamLog->RegisterThread();

    g_cfg_p->Init();
    g_cfg_p->readDefaultSettings(); /*settings.xml */

    CCrashHandler crash;

    g_app = &app;
    SaveDefaultCSS();
    APP_UPDATE_CSS();

    CWorkspace workspace;
    CLogScrutinizerDoc doc;

    TestHS();

    g_cfg_p->m_fontRes = QString(":cn_font");
    g_cfg_p->m_fontRes_Italic = QString(":cn_i_font");
    g_cfg_p->m_fontRes_Bold = QString(":cn_b_font");
    g_cfg_p->m_fontRes_BoldItalic = QString(":cn_bi_font");

    if (g_cfg_p->m_default_Font == QString("Deja Vu Sans")) {
        g_cfg_p->m_fontRes = QString(":dvsm_font");
        g_cfg_p->m_fontRes_Italic = QString(":dvsm_i_font");
        g_cfg_p->m_fontRes_Bold = QString(":dvsm_b_font");
        g_cfg_p->m_fontRes_BoldItalic = QString(":dvsm_bi_font");
        TRACEX_I(QString("Font: ") + QString("Deja Vu Sans"))
    } else if (g_cfg_p->m_default_Font == QString("Courier New")) {
        g_cfg_p->m_fontRes = QString(":cn_font");
        g_cfg_p->m_fontRes_Italic = QString(":cn_i_font");
        g_cfg_p->m_fontRes_Bold = QString(":cn_b_font");
        g_cfg_p->m_fontRes_BoldItalic = QString(":cn_bi_font");
        TRACEX_I(QString("Font: ") + QString("Courier New"))
    } else if (g_cfg_p->m_default_Font == QString("Ubuntu Mono")) {
        g_cfg_p->m_fontRes = QString(":um_font");
        g_cfg_p->m_fontRes_Italic = QString(":um_i_font");
        g_cfg_p->m_fontRes_Bold = QString(":um_b_font");
        g_cfg_p->m_fontRes_BoldItalic = QString(":um_bi_font");
        TRACEX_I(QString("Font: ") + QString("Courier New"))
    } else {
        TRACEX_I(QString("Unsupported font ") + g_cfg_p->m_default_Font)
    }

    if (-1 == QFontDatabase::addApplicationFont(g_cfg_p->m_fontRes)) {
        TRACEX_I(QString("Failed to load font: ") + g_cfg_p->m_fontRes)
    }

    if (-1 == QFontDatabase::addApplicationFont(g_cfg_p->m_fontRes_Italic)) {
        TRACEX_I(QString("Failed to load font: ") + g_cfg_p->m_fontRes_Italic)
    }

    if (-1 == QFontDatabase::addApplicationFont(g_cfg_p->m_fontRes_Bold)) {
        TRACEX_I(QString("Failed to load font: ") + g_cfg_p->m_fontRes_Bold)
    }

    if (-1 == QFontDatabase::addApplicationFont(g_cfg_p->m_fontRes_BoldItalic)) {
        TRACEX_I(QString("Failed to load font: ") + g_cfg_p->m_fontRes_BoldItalic)
    }

#ifdef _WIN32

    /* Not sure if this is really needed */
    if (!AllowSetForegroundWindow(QCoreApplication::applicationPid())) {
        TRACEX_I(QString("Failed to allow LS to set Focus"))
    }
#endif

    /* prevent application to quit when main frame is closed... not wanted really
     * QApplication::setQuitOnLastWindowClosed(false); */

    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

#if 0
 #ifdef _DEBUG

    /* Print elements in the resource database */
    QDir rcDir(":/");
    QFileInfoList list = rcDir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        std::cout << qPrintable(QString("%1 %2").arg(fileInfo.size(), 10)
                                    .arg(fileInfo.fileName()));
        std::cout << std::endl;
    }
 #endif
#endif

    g_cfg_p->GetAppInfo(title, version, buildDate, configuration);
    TRACEX_I(QString("%1 %2  Build:%3").arg(title).arg(version).arg(buildDate)) /*   xxx QT */
    TRACEX_I(QString("%1").arg(configuration))

    QCommandLineParser parser;
    parser.setApplicationDescription("LogScrutinizer text analysis tool");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Files to load"));

    /* Process the actual command line arguments given by the user */
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    MainWindow w;
    w.setCommandLineParams(args);
    w.show();

    (void)app.exec();

    /* Application terminated */

    g_cfg_p->writeDefaultSettings(); /* settings.xml */

    g_RamLog->UnregisterThread();

    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        TRACEX_I(QString("\n\nSystem shutdown\n\n"))
        CSCZ_SystemState = SYSTEM_STATE_SHUTDOWN;
    }

    doc.CleanDB();
    workspace.cleanAll();
    ramLog.cleanUp();
    debug.cleanUp();

    return 0;
}

/***********************************************************************************************************************
*   SaveDefaultCSS
***********************************************************************************************************************/
void SaveDefaultCSS(void)
{
    QFile File("stylesheet.qss");

#ifndef _DEBUG
    if (!File.exists())
#endif
    {
        File.open(QFile::WriteOnly);

        QFile defaultFile(":/stylesheet.qss");
        defaultFile.open(QIODevice::ReadOnly);

        QByteArray array = defaultFile.readAll();
        File.write(array);
    }

    File.close();
}
