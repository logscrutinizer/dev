/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CRecentFile.h"
#include "CLogScrutinizerDoc.h"
#include "CWorkspace.h"
#include "CConfigurationCtrl.h"
#include "ceditorwidget.h"
#include "mainwindow_cb_if.h"
#include "ceditorwidget_cb_if.h"
#include "cplotpane_cb_if.h"
#include "CWorkspace_cb_if.h"
#include "globals.h"
#include "csearchwidget.h"
#include "CProgressCtrl.h"
#include "CConfig.h"
#include "cplotwidget.h"

#include "CDebug.h"
#include "CWorkspace.h"
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QDesktopServices>
#include <QScreen>

#ifdef _WEB_HELP
 #include <QWebEngineView>
#endif
#include <QUrl>
#ifdef USE_SOUND
 #include <QSound>
#endif

#include "ui_searchform.h"

extern CWorkspace *g_workspace_p;
extern void APP_UPDATE_CSS(void);
static bool firstStart = true;
static MainWindow *g_mainWindow_p = nullptr;
static CTimeMeas g_system_tick;

/* Style sheets for qtreeview
 * http://doc.qt.io/qt-5/stylesheet-examples.html#customizing-qtreeview */
void MW_TV_Expand(QModelIndex& modelIndex)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        g_mainWindow_p->m_treeView_p->setCurrentIndex(modelIndex);
        g_mainWindow_p->m_treeView_p->expand(modelIndex);
    }
}

/***********************************************************************************************************************
*   MW_Refresh
***********************************************************************************************************************/
void MW_Refresh(bool forceVScroll)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        Q_ASSERT(g_editorWidget_p != nullptr);
        Q_ASSERT(g_mainWindow_p != nullptr);
        if (nullptr == g_mainWindow_p) {
            return;
        }

        if (forceVScroll) {
            g_editorWidget_p->ForceVScrollBitmap();
        }

        g_editorWidget_p->Refresh(false);
        g_mainWindow_p->update();
    }
}

/***********************************************************************************************************************
*   MW_DO_SHOW
***********************************************************************************************************************/
void MW_DO_SHOW(void)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        g_mainWindow_p->doShow();
    }
}

/***********************************************************************************************************************
*   MW_PlaySystemSound
***********************************************************************************************************************/
void MW_PlaySystemSound(MW_SystemSound sound)
{
    TRACEX_I("MW_PlaySystemSound Playing %d", sound)
#ifdef USE_SOUND
    switch (sound)
    {
        case SYSTEM_SOUND_QUESTION:
            QSound::play(":SND_QUESTION");
            break;

        case SYSTEM_SOUND_FAILURE:
            QSound::play(":SND_FAILURE");
            break;

        case SYSTEM_SOUND_EXIT:
        case SYSTEM_SOUND_START:
        default:
        case SYSTEM_SOUND_PING:
            QSound::play(":SND_PING");
            break;
    }
#else
    switch (sound)
    {
        case SYSTEM_SOUND_QUESTION:
        case SYSTEM_SOUND_FAILURE:
        case SYSTEM_SOUND_EXIT:
        case SYSTEM_SOUND_START:
        case SYSTEM_SOUND_PING:
            TRACEX_I("MW_PlaySystemSound SOUND DISABLED")
            break;
    }
#endif
}

/***********************************************************************************************************************
*   MW_Size
***********************************************************************************************************************/
QSize MW_Size(void)
{
    if (nullptr != g_mainWindow_p) {
        return g_mainWindow_p->size();
    } else {
        return QSize();
    }
}

/***********************************************************************************************************************
*   MW_Parent
***********************************************************************************************************************/
QWidget *MW_Parent(void)
{
    return g_mainWindow_p;
}

/***********************************************************************************************************************
*   MW_RebuildRecentFileMenu
***********************************************************************************************************************/
void MW_RebuildRecentFileMenu(void)
{
    if (nullptr != g_mainWindow_p) {
        g_mainWindow_p->setupRecentFiles();
    }
}

/***********************************************************************************************************************
*   MW_AttachPlot
***********************************************************************************************************************/
CPlotWidgetInterface *MW_AttachPlot(CPlot *plot_p)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        return g_mainWindow_p->attachPlot(plot_p);
    }
    Q_ASSERT(false);
    return nullptr;
}

/***********************************************************************************************************************
*   MW_DetachPlot
***********************************************************************************************************************/
void MW_DetachPlot(CPlot *plot_p)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        g_mainWindow_p->detachPlot(plot_p);
    }
}

/***********************************************************************************************************************
*   MW_SetSubPlotUpdated
***********************************************************************************************************************/
void MW_SetSubPlotUpdated(CSubPlot *subPlot_p)
{
    if ((CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) && (g_mainWindow_p != nullptr)) {
        g_mainWindow_p->setSubPlotUpdated(subPlot_p);
    }
}

/***********************************************************************************************************************
*   MW_RedrawPlot
***********************************************************************************************************************/
void MW_RedrawPlot(CPlot *plot_p)
{
    g_mainWindow_p->redrawPlot(plot_p);
}

/***********************************************************************************************************************
*   MW_ActivateSearch
***********************************************************************************************************************/
void MW_ActivateSearch(const QString& searchText, bool caseSensitive, bool regExp)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->activateSearch(searchText, caseSensitive, regExp);
    }
}

/***********************************************************************************************************************
*   MW_UpdateSearchParameters
***********************************************************************************************************************/
void MW_UpdateSearchParameters(const QString& searchText, bool caseSensitive, bool regExp)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->updateSearchParameters(searchText, caseSensitive, regExp);
    }
}

/***********************************************************************************************************************
*   MW_Search
***********************************************************************************************************************/
void MW_Search(bool forward)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->handleSearch(forward);
    }
}

/***********************************************************************************************************************
*   MW_ModifyFontSize
***********************************************************************************************************************/
void MW_ModifyFontSize(int increase)
{
    if (g_mainWindow_p == nullptr) {
        TRACEX_W("Internal error")
        return;
    }
    g_mainWindow_p->modifyFontSize(increase);
}

/***********************************************************************************************************************
*   MW_SelectionUpdated
***********************************************************************************************************************/
void MW_SelectionUpdated(void)
{
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->selectionUpdated();
    }
}

/***********************************************************************************************************************
*   MW_GeneralKeyHandler
***********************************************************************************************************************/
bool MW_GeneralKeyHandler(int key, bool ctrl, bool shift)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif

    if (g_mainWindow_p == nullptr) {
        return false;
    }

    switch (key)
    {
        case Qt::Key_F:
            if (ctrl) {
                CSelection selection;
                QString text;
                CEditorWidget_GetActiveSelection(selection, text);
                MW_ActivateSearch(text);
            }
            break;

        case Qt::Key_T:
            MW_toggleLogFileTrackState();
            break;

        case Qt::Key_R:
            if (ctrl) {
                CWorkspace_RunAllPlots();
            }
            break;

        case Qt::Key_N:
            if (ctrl) {
                CEditorWidget_AddFilterItem();
            }
            break;

        case Qt::Key_0:
            if (ctrl) {
                TRACEX_I("Set default font size")
                MW_ModifyFontSize(0);
            }
            break;

        case Qt::Key_Plus:
            if (ctrl) {
                TRACEX_I("Increase font size")
                MW_ModifyFontSize(+1);
            }
            break;

        case Qt::Key_Minus:
            if (ctrl) {
                TRACEX_I("Decrease font size")
                MW_ModifyFontSize(-1);
            }
            break;

        case Qt::Key_F1:
            MW_StartWebPage(QString("http://www.logscrutinizer.com/quick_help.php"));
            break;

        case Qt::Key_F2:
            if (ctrl) {
                TRACEX_I("F2  Set bookmark FAILED -- JUST MUST SELECT A ROW FIRST")
            }

            if (shift) {
                TRACEX_I("F2  Previous bookmark")
                CEditorWidget_NextBookmark(true);
            } else {
                TRACEX_I("F2  Next bookmark")
                CEditorWidget_NextBookmark(false);
            }
            break;

        case Qt::Key_F3:
            if (shift) {
                TRACEX_I("F3  Search up")
                MW_Search(false);
            } else {
                TRACEX_I("F3  Search Forward")
                MW_Search(true);
            }
            break;

        case Qt::Key_F5:
            TRACEX_I("F5  Filter")
            CEditorWidget_Filter();
            break;

        case Qt::Key_F4:
            TRACEX_I("F4  Toggle presentation mode")
            CEditorWidget_TogglePresentationMode();
            break;

        default:
            TRACEX_D(QString("Unsupported key ctrl:%1 shift:%2").arg(ctrl).arg(shift))
            return false;
    }
    return true;
}

/***********************************************************************************************************************
*   MW_QuickSearch
***********************************************************************************************************************/

/* TODO */
void MW_QuickSearch(char *searchString, bool searchDown, bool caseSensitive, bool regExpr)
{
    Q_UNUSED(searchString)
    Q_UNUSED(searchDown)
    Q_UNUSED(caseSensitive)
    Q_UNUSED(regExpr)
#ifdef QT_TODO
    g_mainFrame_p->HandleQuickSearch(searchString, searchDown, caseSensitive, regExpr);
#endif
}

/***********************************************************************************************************************
*   MW_SetCursor
***********************************************************************************************************************/
void MW_SetCursor(QCursor *cursor_p)
{
    if (g_mainWindow_p != nullptr) {
        if (cursor_p != nullptr) {
            g_mainWindow_p->setCursor(*cursor_p);
        } else {
            g_mainWindow_p->unsetCursor();
        }
    }
}

/***********************************************************************************************************************
*   MW_SetApplicationName
***********************************************************************************************************************/
void MW_SetApplicationName(QString *name)
{
    if (nullptr != g_mainWindow_p) {
        g_mainWindow_p->setWindowTitle(*name);
    }
}

/***********************************************************************************************************************
*   MW_AppendLogMsg
***********************************************************************************************************************/
bool MW_AppendLogMsg(const QString& message)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
    Q_ASSERT(g_mainWindow_p->m_logWindow_p != nullptr);
#endif

    /* Scope is locked from TRACEX */

    static QStringList string_queue;

    if ((CSCZ_SystemState == SYSTEM_STATE_RUNNING) &&
        (g_mainWindow_p != nullptr) &&
        (g_mainWindow_p->m_logWindow_p != nullptr)) {
        while (!string_queue.isEmpty()) {
            QString queued = string_queue.takeFirst();
            QMetaObject::invokeMethod(g_mainWindow_p->m_logWindow_p, "message_receiver", Qt::QueuedConnection,
                                      Q_ARG(QString, queued));
        }

        QMetaObject::invokeMethod(g_mainWindow_p->m_logWindow_p, "message_receiver", Qt::QueuedConnection,
                                  Q_ARG(QString, message));
        return true;
    } else {
        string_queue.append(message);
    }
    return false;
}

/***********************************************************************************************************************
*   MW_GetTick
***********************************************************************************************************************/
int64_t MW_GetTick(void)
{
    return g_system_tick.ms();
}

/***********************************************************************************************************************
*   MW_StartWebPage
***********************************************************************************************************************/
void MW_StartWebPage(const QString& url)
{
    g_mainWindow_p->startWebPage(url);
}

/***********************************************************************************************************************
*   MW_SetWorkspaceWidgetFocus
***********************************************************************************************************************/
void MW_SetWorkspaceWidgetFocus(void)
{
    g_mainWindow_p->setWorkspaceWidgetFocus();
}

/***********************************************************************************************************************
*   MW_toggleLogFileTrackState
***********************************************************************************************************************/
void MW_toggleLogFileTrackState(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->updateLogFileTrackState(g_cfg_p->m_logFileTracking ? false : true);
    }
}

/***********************************************************************************************************************
*   MW_updateLogFileTrackState
***********************************************************************************************************************/
void MW_updateLogFileTrackState(bool track)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_mainWindow_p != nullptr);
#endif
    if (g_mainWindow_p != nullptr) {
        g_mainWindow_p->updateLogFileTrackState(track);
    }
}

/***********************************************************************************************************************
*   MW_updatePendingStateGeometry
***********************************************************************************************************************/
void MW_updatePendingStateGeometry(void)
{
    /* must check firstStart, otherwise the dockwidgets will overwrite the saved settings when they are being created.
     * */
    if (!firstStart) {
        g_mainWindow_p->updatePendingStateGeometry();
    }
}

/***********************************************************************************************************************
*   setWorkspaceWidgetFocus
***********************************************************************************************************************/
void MainWindow::setWorkspaceWidgetFocus(void)
{
    m_treeView_p->setFocus(Qt::OtherFocusReason);
}

/***********************************************************************************************************************
*   message_receiver
***********************************************************************************************************************/
void CLogWindow::message_receiver(const QString& text)
{
    appendPlainText(text); /* Adds the message to the widget */
    verticalScrollBar()->setValue(verticalScrollBar()->maximum()); /* Scrolls to the bottom */
}

/***********************************************************************************************************************
*   storeSettings
***********************************************************************************************************************/
void CLogWindow::storeSettings(void)
{
#ifdef LOCAL_GEOMETRY_SETTING
    QSettings settings;
    settings.setValue("logWindow", size());
#endif
}

/***********************************************************************************************************************
*   appStateChanged
***********************************************************************************************************************/
void MainWindow::appStateChanged(Qt::ApplicationState state)
{
    PRINT_SIZE(QString("AppState %1").arg(state))
}

/***********************************************************************************************************************
*   hasPlotPane
***********************************************************************************************************************/
bool MainWindow::hasPlotPane(void)
{
    return m_plotPane_p != nullptr ? true : false;
}

/***********************************************************************************************************************
*   addPlotPane
***********************************************************************************************************************/
void MainWindow::addPlotPane(void)
{
    if (m_plotPane_p == nullptr) {
        m_plotPane_p = new CPlotPane(m_tabWidget_p);
    }

    const QIcon icon = QIcon(":plugin_32x32.png");
    m_tabWidget_p->addTab(m_plotPane_p, icon, "Plugin plots");
}

/***********************************************************************************************************************
*   startWebPage
***********************************************************************************************************************/
void MainWindow::startWebPage(const QString& url)
{
#ifdef _WEB_HELP
    QDockWidget *webPaneDock_p = new QDockWidget("Web", this);
    webPaneDock_p->setFeatures(QDockWidget::DockWidgetFloatable |
                               QDockWidget::DockWidgetMovable |
                               QDockWidget::DockWidgetClosable);

    webPaneDock_p->setAllowedAreas(Qt::AllDockWidgetAreas);

    QWebEngineView *view = new QWebEngineView(webPaneDock_p);
    webPaneDock_p->setWidget(view);

    addDockWidget(Qt::LeftDockWidgetArea, webPaneDock_p);

    view->load(QUrl(url));
    view->show();
#else
    QDesktopServices::openUrl(QUrl(url));
#endif
}

/***********************************************************************************************************************
*   modifyFontSize
***********************************************************************************************************************/
void MainWindow::modifyFontSize(int increase)
{
    m_editor_p->ModifyFontSize(increase);
    if (hasPlotPane()) {
        m_plotPane_p->redrawPlot(nullptr); /* update all plots */
    }
    update();
    if (increase < 0) {
        m_logWindow_p->zoomOut();
    } else if (increase > 0) {
        m_logWindow_p->zoomIn();
    }
}

/***********************************************************************************************************************
*   selectionUpdated
***********************************************************************************************************************/
void MainWindow::selectionUpdated(void)
{
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);

    if ((m_saveFilterAct != nullptr) && (m_saveFilterAsAct != nullptr)) {
        if (selectionList.count() == 1) {
            m_saveFilterAct->setEnabled(true);
            m_saveFilterAsAct->setEnabled(true);
        } else {
            m_saveFilterAct->setEnabled(false);
            m_saveFilterAsAct->setEnabled(false);
        }
    }
}

/***********************************************************************************************************************
*   removePlotPane
***********************************************************************************************************************/
void MainWindow::removePlotPane(void)
{
    Q_ASSERT(nullptr != m_plotPane_p);
    delete m_plotPane_p;
    m_plotPane_p = nullptr;
}

/***********************************************************************************************************************
*   attachPlot
***********************************************************************************************************************/
CPlotWidgetInterface *MainWindow::attachPlot(CPlot *plot_p)
{
    if (!hasPlotPane()) {
        addPlotPane();
    }

    char *name_p;
    char *x_axis_p;
    CPlotWidgetInterface *widgetInterface;
    plot_p->GetTitle(&name_p, &x_axis_p);
    if (name_p != nullptr) {
        widgetInterface = m_plotPane_p->addPlot(name_p, plot_p);
    } else {
        widgetInterface = m_plotPane_p->addPlot("No name", plot_p);
    }
    update();
    return widgetInterface;
}

/***********************************************************************************************************************
*   detachPlot
***********************************************************************************************************************/
void MainWindow::detachPlot(CPlot *plot_p)
{
    Q_ASSERT(hasPlotPane());
    Q_ASSERT(plot_p);
    if (!hasPlotPane()) {
        TRACEX_W(QString("%1 bad input, no plotPane"))
        return;
    }
    m_plotPane_p->removePlot(plot_p);
    if (!m_plotPane_p->plotExists()) {
        removePlotPane(); /* if there is no plots left */
    }
    update();
}

/***********************************************************************************************************************
*   setSubPlotUpdated
***********************************************************************************************************************/
void MainWindow::setSubPlotUpdated(CSubPlot *subPlot_p)
{
    Q_ASSERT(subPlot_p);
    if (!m_plotPane_p->plotExists() || (subPlot_p == nullptr)) {
        TRACEX_W(QString("%1 bad input"))
    }
    m_plotPane_p->setSubPlotUpdated(subPlot_p);
}

/***********************************************************************************************************************
*   redrawPlot
***********************************************************************************************************************/
void MainWindow::redrawPlot(CPlot *plot_p)
{
    if (hasPlotPane()) {
        if (!m_plotPane_p->plotExists()) {
            TRACEX_W(QString("%1 bad input"))
        } else {
            m_plotPane_p->redrawPlot(plot_p);
        }
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_treeView_p(nullptr), m_logWindow_p(nullptr), m_editor_p(nullptr), m_tabWidget_p(nullptr),
    m_searchWidget_p(nullptr), m_plotPane_p(nullptr)
{
    Ui::UI_SearchForm ui;
    const QIcon mainIcon = QIcon(":logo.png");
    setWindowIcon(mainIcon);

    createActions();

    g_workspace_p->FillWorkspace();

    QDockWidget *editorDockWidget_p = new QDockWidget("Viewer", this);
    editorDockWidget_p->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    editorDockWidget_p->setObjectName("Viewer");

    QDockWidget *workspaceDockWidget_p = new QDockWidget("Workspace Docker", this);
    workspaceDockWidget_p->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    workspaceDockWidget_p->setObjectName("Workspace Docker");

    QDockWidget *tabDockWidget_p = new QDockWidget("Tab Docker", this);
    tabDockWidget_p->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    tabDockWidget_p->setObjectName("Tab Docker");

    addDockWidget(Qt::RightDockWidgetArea, editorDockWidget_p);
    addDockWidget(Qt::LeftDockWidgetArea, workspaceDockWidget_p);
    addDockWidget(Qt::RightDockWidgetArea, tabDockWidget_p);

    /*addDockWidget(Qt::BottomDockWidgetArea, searchDockWidget_p); */

    m_editor_p = new CEditorWidget(editorDockWidget_p);
    m_tabWidget_p = new CTabWidget(tabDockWidget_p);

    editorDockWidget_p->setWidget(m_editor_p);
    editorDockWidget_p->setAllowedAreas(Qt::AllDockWidgetAreas);

    workspaceDockWidget_p->setWidget(setupModelView(workspaceDockWidget_p));
    workspaceDockWidget_p->setAllowedAreas(Qt::AllDockWidgetAreas);

    /* searchDockWidget_p->setWidget(m_searchWidget_p);
     * searchDockWidget_p->setAllowedAreas(Qt::AllDockWidgetAreas); */

    tabDockWidget_p->setWidget(m_tabWidget_p);
    tabDockWidget_p->setAllowedAreas(Qt::AllDockWidgetAreas);

    //PANEW
    m_logWindow_p = new CLogWindow();
    m_tabWidget_p->addTab(m_logWindow_p, mainIcon, "Log window");
    m_searchWidget_p = new CSearchWidget();
    ui.setupUi(m_searchWidget_p);

    /*Qt::WindowFlags flags = m_searchWidget_p->windowFlags(); */

    /*m_searchWidget_p->setWindowFlags(flags & ~Qt::WindowTitleHint &
     * ~Qt::WindowSystemMenuHint & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowContextHelpButtonHint);*/

    m_tabWidget_p->addTab(m_searchWidget_p, mainIcon, "Search window");

    statusBar()->showMessage(tr("Ready"));
    setAcceptDrops(true);
    g_mainWindow_p = this;

    for (QScreen *screen : QGuiApplication::screens()) {
        TRACEX_I(QString("Screen: %1 Model:%2  avail:%3,%4  virtual:%5,%6 size:%7,%8")
                     .arg(screen->manufacturer()).arg(screen->model())
                     .arg(screen->availableGeometry().width()).arg(screen->availableGeometry().height())
                     .arg(screen->virtualSize().width()).arg(screen->virtualSize().height()).arg(screen->size().width())
                     .arg(screen->size().height()))
    }

    /* Print all the stored settings */
    QSettings settings;
    QStringList stringList = settings.allKeys();
    TRACEX_I(QString("Setting keys in Qt registry"))
    for (auto& item : stringList) {
        TRACEX_I(QString("Key %1").arg(item))
    }

/*    if (!isExitStateMaximized()) { */
    restoreSavedStateGeometry();

    /*  } */
}

/***********************************************************************************************************************
*   showEvent
***********************************************************************************************************************/
void MainWindow::showEvent(QShowEvent *e)
{
    PRINT_SIZE(QString("MW showEvent %1 first:%2").arg(e->type()).arg(firstStart))
    QMainWindow::showEvent(e);

    if (firstStart) {
        GetTheDoc()->m_recentFiles.ReadFromFile();
        CFGCTRL_LoadDefaultWorkspace();
        if (!m_commandLine.isEmpty()) {
            QString cmdLine = QString("CommandLine (%1 args): ").arg(m_commandLine.length());
            for (auto& item : m_commandLine) {
                cmdLine += QString(" ") + item;
            }
            TRACEX_I(cmdLine);
            CFGCTRL_LoadFiles(m_commandLine);
        }

        if (isExitStateMaximized()) {
            showMaximized();

            auto screenList = QGuiApplication::screens();
            auto first_screen = screenList.first();
            resize(first_screen->size());
            raise();
        }

        firstStart = false;
        CSCZ_SystemState = SYSTEM_STATE_RUNNING;
        setupRecentFiles();

        TRACEX_I(QString("Ready"))
    }
}

/*
 * The window resizing mechanics
 *
 * To start with the Qt Layout manager really adhere to the provided sizeHintfor each Widget. It also use the
 * sizePolicy. Typically the sizePolicy is expanding which means that it is up to the layout manager to decide
 * which Widget that should occupy which space. When this needs to be overriden we can instead use a static/fixed
 * sizePolicy.
 *
 * Typically a Widget can have the states normal and maximized. In LS, each of these states has its own geometry
 * settings stored in QSetting with different keys. When changing state LS will fetch these settings and force a
 * restore to these window sizes. As such, if use do specific resizing in the different states these will be
 * kept when going back and forward between states.
 *
 * When LS is started with a clean QSettings registry, then LS will fallback to a predefined aspect ratio between
 * Workspace/Editor/Search Widget.
 *
 * Problems that was solved
 * ------------------------
 * (I) A problem that was seen was that if sizeHint isn't responding with the latest given size from the resizeEvent
 * then  the layout manager shift the windows back to a position it feels best, typically a 50% share of workspace and
 * editor. As such, to maintain the aspect ration that the user gives the windows it is essential that when the
 * resizeEvent occur that the latet size is as well provided in the next called sizeHint.
 *
 * (II) Events are provided as a reflection on that something took place, and not provided before something
 * happends which then would allow a prohibit, such a resize. Still, you cannot be sure when an event is provided
 * in relation to other events. So when the main window change state from maximized to normal it is likely that
 * Widgets starts to be resized, and then the state change event occurs. However, the opposite occurs when changing
 * from maximized to normal, then the event comes first but the Widgets hasn't been resized yet. To
 * remedy this a "pending geometry" mechanism was introduced. This is based on a timer, when any widget wants to update
 * its geometry the timer is started. When the timer has elapsed then the geometry is stored to QSettings. This
 * prevents Widget window size changes during state transition to overwrite values from another state.
 *
 */

const double BIT_LESS = 0.90;

/***********************************************************************************************************************
*   sizeHint
***********************************************************************************************************************/
QSize MainWindow::sizeHint() const
{
    static QSize windowSize;
    auto refreshMainWindow = makeMyScopeGuard([&] () {
        PRINT_SIZE(QString("MainWindow sizeHint %1,%2 state:%3")
                       .arg(windowSize.width()).arg(windowSize.height()).arg(windowState()))
    });
    windowSize = QMainWindow::sizeHint();

    if (CSCZ_AdaptWindowSizes) {
        /*        windowSize = m_adaptWindowSize; */
        PRINT_SIZE(QString("MainWindow adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
    }

    return windowSize;
}

/***********************************************************************************************************************
*   getSettingKeys
***********************************************************************************************************************/
void MainWindow::getSettingKeys(QString& geometry, QString& state) const
{
    geometry = QString("geometry_");
    state = QString("state_");

    switch (windowState())
    {
        case Qt::WindowMinimized:
            geometry += QString("min");
            state += QString("min");
            break;

        case Qt::WindowMaximized:
            geometry += QString("max");
            state += QString("max");
            break;

        case Qt::WindowFullScreen:
            geometry += QString("full");
            state += QString("full");
            break;

        case Qt::WindowActive:
            geometry += QString("active");
            state += QString("active");
            break;

        default:
            geometry += QString("def");
            state += QString("def");
            break;

        case Qt::WindowNoState:
            geometry += QString("no");
            state += QString("no");
            break;
    }
}

/***********************************************************************************************************************
*   setupPredefinedWindowSizes
***********************************************************************************************************************/
void MainWindow::setupPredefinedWindowSizes(void)
{
    /* This function manufacture normal window state values and put the into QSetting, and then call to
     * restore them */

    CGeometryState gs;
    getSettingKeys(gs.geoKey, gs.stateKey);

    QRect rec = QGuiApplication::primaryScreen()->geometry();

    if (windowState() != Qt::WindowMaximized) {
        auto scale = 0.6;
        gs.mainWindow = QSize(static_cast<int>(static_cast<double>(rec.width()) * scale),
                              static_cast<int>(static_cast<double>(rec.height()) * scale));
    } else {
        gs.mainWindow = QSize(rec.width(), rec.height());
    }

    TRACEX_I(QString("MW setupPredefinedWindowSizes resize %1,%2 state:%3")
                 .arg(gs.mainWindow.width()).arg(gs.mainWindow.height()).arg(windowState()))

    gs.workspace = QSize(static_cast<int>(static_cast<double>(gs.mainWindow.width()) * 0.2),
                         static_cast<int>(static_cast<double>(gs.mainWindow.height()) * 0.8));
    gs.editor = QSize(static_cast<int>((gs.mainWindow.width() - gs.workspace.width()) * BIT_LESS),
                      static_cast<int>(static_cast<double>(gs.mainWindow.height()) * 0.8 * BIT_LESS));
    gs.log = QSize(gs.editor.width(), static_cast<int>((gs.mainWindow.height() - gs.editor.height()) * BIT_LESS));
    gs.search = gs.log;
    m_pendingGeometryState.insert(std::pair<Qt::WindowStates, CGeometryState>(windowState(), gs));

    savePendingStateGeometry(windowState());
    restoreSavedStateGeometry(false /*no fallback*/);
}

/* Called when any of the DockWidgets gets their size changed. The data is put for later save, if the window is
 * maximized, minimized or put to normal. */
void MainWindow::updatePendingStateGeometry(void)
{
    m_pendingGeometryTimer = std::make_unique<QTimer>(this);

    connect(m_pendingGeometryTimer.get(), &QTimer::timeout,
            [ = ] ()
    {
        if (m_activeState != windowState()) {
            TRACEX_I("Window update ignored, state change pending")
            return;
        }

        PRINT_SIZE(QString("Pending %1 before no:%2 max:%3")
                       .arg(windowState()).arg(m_pendingGeometryState.count(Qt::WindowNoState))
                       .arg(m_pendingGeometryState.count(Qt::WindowMaximized)))

        auto it = m_pendingGeometryState.find(windowState());
        while (it != m_pendingGeometryState.end()) {
            m_pendingGeometryState.erase(it);
            it = m_pendingGeometryState.find(windowState());
        }

        PRINT_SIZE(QString("Pending after clean no:%1 max:%2")
                       .arg(m_pendingGeometryState.count(Qt::WindowNoState))
                       .arg(m_pendingGeometryState.count(Qt::WindowMaximized)))

        CGeometryState gs;
        getSettingKeys(gs.geoKey, gs.stateKey);

        /* gs.geometry = saveGeometry();
         * gs.state = saveState(); */
        gs.mainWindow = size();
        gs.editor = m_editor_p->size();
        gs.workspace = m_treeView_p->size();
        gs.log = m_logWindow_p->size();
        gs.search = m_searchWidget_p->size();
        auto result = m_pendingGeometryState.insert(std::pair<Qt::WindowStates, CGeometryState>(windowState(), gs));

        PRINT_SIZE(QString("Update pending window %1 %2 overwrite:%3 isMax:%4 MW(%5,%6) Editor(%7,%8) W(%9,%10")
                       .arg(gs.geoKey).arg(windowState()).arg(result.second == false ? QString("Yes") : QString("No"))
                       .arg(isMaximized()).arg(rect().width()).arg(rect().height())
                       .arg(gs.editor.width()).arg(gs.editor.height()).arg(gs.workspace.width()).arg(
                       gs.workspace.height()))

        m_pendingGeometryTimer.get()->stop();

        PRINT_SIZE(QString("Pending after insert clean no:%1 max:%2")
                       .arg(m_pendingGeometryState.count(Qt::WindowNoState))
                       .arg(m_pendingGeometryState.count(Qt::WindowMaximized)))
    });

    m_pendingGeometryTimer.get()->start(400);
}

const int GEOMETRY_VERSION = (1 + 0x5555);

/***********************************************************************************************************************
*   savePendingStateGeometry
***********************************************************************************************************************/
void MainWindow::savePendingStateGeometry(Qt::WindowStates previousState)
{
    QSettings settings;
    auto it = m_pendingGeometryState.find(previousState);

    if (it != m_pendingGeometryState.end()) {
        QByteArray ba;
        QDataStream stream(&ba, QIODevice::WriteOnly);
        auto gs = (*it).second;
        if (gs.editor != QSize()) {
            stream << GEOMETRY_VERSION;
            stream << gs.mainWindow;
            stream << gs.editor;
            stream << GEOMETRY_VERSION;
            stream << gs.workspace;
            stream << gs.log;
            stream << gs.search;
            settings.setValue(gs.geoKey, ba);
            TRACEX_I(QString("MW Geometry - Save pending  %1  mw(%2,%3) editor(%4,%5) workspace (%6,%7) bytes:%8")
                         .arg(gs.mainWindow.width()).arg(gs.mainWindow.height())
                         .arg(gs.geoKey).arg(gs.editor.width()).arg(gs.editor.height())
                         .arg(gs.workspace.width()).arg(gs.workspace.height()).arg(ba.size()))
            return;
        } else {
            TRACEX_I(QString("MW Geometry - Failure, pending Geometry not saved, bad editor size %1,%2")
                         .arg(gs.editor.width()).arg(gs.editor.height()))
        }
    }
    TRACEX_I(QString("MW Geometry - Failure, pending Geometry state:%1 not saved, empty data").arg(previousState))
}

/***********************************************************************************************************************
*   saveExitState
***********************************************************************************************************************/
void MainWindow::saveExitState(void)
{
    QSettings settings;
    QByteArray ba;
    QDataStream stream(&ba, QIODevice::WriteOnly);
    const int state = static_cast<int>(windowState());

    stream << GEOMETRY_VERSION;
    stream << state;
    settings.setValue("exit_state", ba);
    TRACEX_I(QString("MW Exit State maximized(%1)").arg(state))
}

/***********************************************************************************************************************
*   isExitStateMaximized
***********************************************************************************************************************/
bool MainWindow::isExitStateMaximized(void)
{
    QSettings settings;
    QByteArray state_ba = settings.value("exit_state", QByteArray()).toByteArray();
    QDataStream stream(&state_ba, QIODevice::ReadOnly);
    int version;
    int state;

    stream >> version;

    if (GEOMETRY_VERSION != version) {
        TRACEX_I(QString("MW Exit state wrong geometry version, expected:%1 stored:%2")
                     .arg(GEOMETRY_VERSION).arg(version))
        return false;
    }

    stream >> state;

    if (state == Qt::WindowMaximized) {
        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   restoreSavedStateGeometry
***********************************************************************************************************************/
void MainWindow::restoreSavedStateGeometry(bool enable_fallback)
{
    QSettings settings;
    QString geometry_key;
    QString state_key;
    getSettingKeys(geometry_key, state_key);

    TRACEX_I(QString("MW Geometry Restore  %1").arg(geometry_key))

    QByteArray geometry = settings.value(geometry_key, QByteArray()).toByteArray();
    QDataStream stream(&geometry, QIODevice::ReadOnly);
    int version;
    QSize mainWindow, editor, workspace, log, search;
    QSizePolicy policy;

    m_pendingWindowAdaptationTimer = std::make_unique<QTimer>(this);
    connect(m_pendingWindowAdaptationTimer.get(), &QTimer::timeout, [ = ] ()
    {
        CWorkspace_LayoutChanged();

        m_pendingWindowAdaptationTimer.get()->stop();
        QSizePolicy policy(sizePolicy());
        policy.setHorizontalPolicy(QSizePolicy::Expanding);

        m_editor_p->setSizePolicy(policy);
        m_treeView_p->setSizePolicy(policy);
        m_searchWidget_p->setSizePolicy(policy);
        m_tabWidget_p->setSizePolicy(policy);
        m_logWindow_p->setSizePolicy(policy);
        if (m_plotPane_p != nullptr) {
            m_plotPane_p->setSizePolicy(policy);
        }

        m_pendingWindowAdaptationTimer_2.get()->start(1000);
        PRINT_SIZE(QString("MW Geometry Restore ended 1"))
    });

    m_pendingWindowAdaptationTimer_2 = std::make_unique<QTimer>(this);
    connect(m_pendingWindowAdaptationTimer_2.get(), &QTimer::timeout, [ = ] ()
    {
        m_pendingWindowAdaptationTimer_2.get()->stop();

        CSCZ_AdaptWindowSizes = false;

        TRACEX_I(QString("MW Geometry Restore ended"))
    });

    if (geometry.isEmpty()) {
        TRACEX_I(QString("MW Geometry - Failure, no geometry state info for %1").arg(geometry_key))
        goto fallback;
    }

    stream >> version;

    if (GEOMETRY_VERSION != version) {
        TRACEX_I(QString("MW Geometry - Failure, wrong geometry version, expected:%1 stored:%2")
                     .arg(GEOMETRY_VERSION).arg(version))
        goto fallback;
    }

    stream >> mainWindow;
    stream >> editor;
    stream >> version;

    if (GEOMETRY_VERSION != version) {
        TRACEX_I(QString("MW Geometry - Failure, wrong geometry version, expected:%1 stored:%2")
                     .arg(GEOMETRY_VERSION).arg(version))
        goto fallback;
    }

    stream >> workspace;
    stream >> log;
    stream >> search;

    TRACEX_I(QString("MW Geometry Restore started editor(%1,%2), workspace(%3,%4)")
                 .arg(editor.width()).arg(editor.height()).arg(workspace.width()).arg(workspace.height()))

    m_editor_p->setAdaptWindowSize(editor);
    m_treeView_p->setAdaptWindowSize(workspace);
    m_logWindow_p->setAdaptWindowSize(QSize());
    m_searchWidget_p->setAdaptWindowSize(QSize());
    m_tabWidget_p->setAdaptWindowSize(QSize());
    if (m_plotPane_p != nullptr) {
        m_plotPane_p->setAdaptWindowSize(QSize());
    }

    CWorkspace_LayoutAboutToBeChanged();

    /* Force Layout manager to use the settings */
    policy = QSizePolicy(sizePolicy());
    policy.setHorizontalPolicy(QSizePolicy::Fixed);
    m_editor_p->setSizePolicy(policy);
    m_treeView_p->setSizePolicy(policy);

    CSCZ_AdaptWindowSizes = true;

    if (windowState() != Qt::WindowMaximized) {
        resize(mainWindow);
    }

    /* Make Layout manager make new calls to the sizeHint functions */
    m_editor_p->updateGeometry();
    m_treeView_p->updateGeometry();

    m_pendingWindowAdaptationTimer.get()->start(500);

    return;

fallback:
    if (enable_fallback) {
        TRACEX_I(QString("MW Geometry - Failure - No window settings, activating adaptWindowSizes"))
        setupPredefinedWindowSizes();
    }
}

MainWindow::~MainWindow()
{
    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        TRACEX_I(QString("\n\nSystem shutdown\n\n"))
        CSCZ_SystemState = SYSTEM_STATE_SHUTDOWN;
    }
}

/***********************************************************************************************************************
*   createActions
***********************************************************************************************************************/
void MainWindow::createActions(void)
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("File");

    QToolBar *devCmd = addToolBar(tr("DevCmds"));
    devCmd->setObjectName("DevCmds");

    QToolBar *trackingBar = addToolBar(tr("FileTracking"));
    trackingBar->setObjectName("FileTracking");

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":open-32.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon saveDefaultWorkspaceIcon = QIcon::fromTheme("document-save-as", QIcon(":save-as-32"));
    QAction *saveDefaultWorkspaceAct = new QAction(saveDefaultWorkspaceIcon, tr("&Save As Default-Workspace"), this);
    saveDefaultWorkspaceAct->setStatusTip(tr("Save current workspace as default"));
    connect(saveDefaultWorkspaceAct, &QAction::triggered, this, &MainWindow::saveDefaultWorkspace);
    fileMenu->addAction(saveDefaultWorkspaceAct);

    {
        QAction *action_p;
        action_p = fileMenu->addAction(QString("Save Workspace"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] () {
            CFGCTRL_SaveWorkspaceFile();
        });
    }

    {
        QAction *action_p;
        action_p = fileMenu->addAction(QString("Save Workspace As..."));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] () {
            CFGCTRL_SaveWorkspaceFileAs();
        });
    }

    {
        /*
         * bool CFGCTRL_SaveFilterFile(QString& fileName, CCfgItem_Filter *filterItem_p);
         *  bool CFGCTRL_SaveFilterFileAs(QString *fileName_p, CCfgItem_Filter *filterItem_p);
         */
        const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":save-32"));
        if (m_saveFilterAct == nullptr) {
            m_saveFilterAct = new QAction(saveIcon, "TBD", this);
        }
        m_saveFilterAct->setShortcuts(QKeySequence::Save);
        m_saveFilterAct->setStatusTip(tr("Save the filter(s) to disk"));
        connect(m_saveFilterAct, &QAction::triggered, [ = ] () {
            QList<CCfgItem *> selectionList;
            CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);
            for (auto& item : selectionList) {
                auto filter = reinterpret_cast<CCfgItem_Filter *>(item);
                auto fileName = filter->m_filter_ref_p->GetFileNameRef();
                CFGCTRL_SaveFilterFile(fileName, filter);
            }
        });

        fileMenu->addAction(m_saveFilterAct);
        fileToolBar->addAction(m_saveFilterAct);
        m_saveFilterAct->setEnabled(false);
    }

    {
        const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":save-32"));
        if (m_saveFilterAct == nullptr) {
            m_saveFilterAct = new QAction(saveIcon, "TBD", this);
        }
        m_saveFilterAct->setShortcuts(QKeySequence::Save);
        m_saveFilterAct->setStatusTip(tr("Save the filter(s) to disk"));
        connect(m_saveFilterAct, &QAction::triggered, [ = ] () {
            QList<CCfgItem *> selectionList;
            CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);
            for (auto& item : selectionList) {
                auto filter = reinterpret_cast<CCfgItem_Filter *>(item);
                auto fileName = filter->m_filter_ref_p->GetFileNameRef();
                CFGCTRL_SaveFilterFile(fileName, filter);
            }
        });

        fileMenu->addAction(m_saveFilterAct);
        fileToolBar->addAction(m_saveFilterAct);
        m_saveFilterAct->setEnabled(false);
    }

    {
        const QIcon saveAsIcon = QIcon::fromTheme("document-save-as", QIcon(":save-as-32"));
        if (m_saveFilterAsAct == nullptr) {
            m_saveFilterAsAct = new QAction(saveAsIcon, "TBD", this);
        }
        m_saveFilterAsAct->setShortcuts(QKeySequence::SaveAs);
        m_saveFilterAsAct->setStatusTip(tr("Save the filter(s) under a new name"));
        connect(m_saveFilterAsAct, &QAction::triggered, [ = ] () {
            QList<CCfgItem *> selectionList;
            CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);
            for (auto& item : selectionList) {
                auto filter = reinterpret_cast<CCfgItem_Filter *>(item);
                auto fileName = filter->m_filter_ref_p->GetFileNameRef();
                CFGCTRL_SaveFilterFileAs(&fileName, filter);
            }
        });

        fileMenu->addAction(m_saveFilterAsAct);
        fileToolBar->addAction(m_saveFilterAsAct);
        m_saveFilterAsAct->setEnabled(false);
    }

#if CSS_SUPPORT == 1 || defined (_DEBUG)
    const QIcon cssIcon = QIcon(":css.png");
    QAction *reloadCSSAct = new QAction(cssIcon, tr("&ReloadCSS"), this);
    reloadCSSAct->setStatusTip(tr("Reload application CSS"));
    connect(reloadCSSAct, &QAction::triggered, this, &MainWindow::reloadCSS);
    devCmd->addAction(reloadCSSAct);
#endif

    /*     const QIcon cssIcon = QIcon(":css.png");
     *    QAction *reloadCSSAct = new QAction(cssIcon, tr("&ReloadCSS"), this);
     *    reloadCSSAct->setStatusTip(tr("Reload application CSS"));
     *   connect(reloadCSSAct, &QAction::triggered, this, &MainWindow::reloadCSS);
     *  devCmd->addAction(reloadCSSAct); */
    QWidget *empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    trackingBar->addWidget(empty);

    m_checkBox_p = new QCheckBox(QString("Track file tail"), this);
    m_checkBox_p->setCheckState(g_cfg_p->m_logFileTracking ? Qt::Checked : Qt::Unchecked);
    connect(m_checkBox_p, &QCheckBox::stateChanged, [ = ] (int newValue)
    {
        Q_UNUSED(newValue)
        auto doc_p = GetTheDoc();
        auto state = m_checkBox_p->checkState();
        g_cfg_p->m_logFileTracking = state == Qt::Unchecked ? false : true;
        doc_p->enableLogFileTracking(g_cfg_p->m_logFileTracking);
        m_editor_p->setFocus();
    });
    trackingBar->addWidget(m_checkBox_p);

    connect(MW_getApp(), &QApplication::applicationStateChanged, this, &MainWindow::appStateChanged);

#ifdef _DEBUG
    QPixmap pixmap(100, 100);
    pixmap.fill(QColor("red"));

    const QIcon devTestIcon = QIcon(pixmap);
    QAction *startDevTestAct = new QAction(devTestIcon, tr("&StartDevTest"), this);
    startDevTestAct->setStatusTip(tr("Start dev tests"));
    connect(startDevTestAct, &QAction::triggered, this, &MainWindow::startDevTest);
    devCmd->addAction(startDevTestAct);
#endif

    {
        /* Increase cursor size */
        QAction *action_p;
        action_p = viewMenu->addAction(QString("Increase text size (ctrl+)"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] ()
        {
            modifyFontSize(1);
        });
    }

    {
        /* Decrease cursor size */
        QAction *action_p;
        action_p = viewMenu->addAction(QString("Decrease text size (ctrl-)"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] ()
        {
            modifyFontSize(-1);
        });
    }

    /*auto settingsMenu = toolsMenu->addMenu(tr("&Settings...")); */
#if _DEBUG
    auto debugMenu = toolsMenu->addMenu(tr("&Settings..."));
    {
        /* Tools - clear QSetting */
        QAction *action_p;
        action_p = debugMenu->addAction(QString("Clear Window Settings"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] ()
        {
            QSettings settings;
            settings.clear();
        });
    }
    {
        /* Tools - show setting in log window */
        QAction *action_p;
        action_p = debugMenu->addAction(QString("Show Window Setting Keys in log"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] ()
        {
            QSettings settings;
            QStringList stringList = settings.allKeys();
            TRACEX_I(QString("Setting keys in Qt registry"))
            for (auto& item : stringList) {
                TRACEX_I(QString("Setting key: %1").arg(item))
            }
        });
    }
#endif

    /*
     *
     *  fileMenu->addSeparator();
     *
     *  const QIcon exitIcon = QIcon::fromTheme("application-exit");
     *  QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
     *  exitAct->setShortcuts(QKeySequence::Quit);
     *  exitAct->setStatusTip(tr("Exit the application"));
     *
     *  QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
     *  QToolBar *editToolBar = addToolBar(tr("Edit"));
     #ifndef QT_NO_CLIPBOARD
     *  const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
     *  QAction *cutAct = new QAction(cutIcon, tr("Cu&t"), this);
     *  cutAct->setShortcuts(QKeySequence::Cut);
     *  cutAct->setStatusTip(tr("Cut the current selection's contents to the "
     *   "clipboard"));
     *  connect(cutAct, &QAction::triggered, textEdit, &QPlainTextEdit::cut);
     *  editMenu->addAction(cutAct);
     *  editToolBar->addAction(cutAct);
     *
     *  const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
     *  QAction *copyAct = new QAction(copyIcon, tr("&Copy"), this);
     *  copyAct->setShortcuts(QKeySequence::Copy);
     *  copyAct->setStatusTip(tr("Copy the current selection's contents to the "
     *   "clipboard"));
     *  connect(copyAct, &QAction::triggered, textEdit, &QPlainTextEdit::copy);
     *  editMenu->addAction(copyAct);
     *  editToolBar->addAction(copyAct);
     *
     *  const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
     *  QAction *pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
     *  pasteAct->setShortcuts(QKeySequence::Paste);
     *  pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
     *   "selection"));
     *  connect(pasteAct, &QAction::triggered, textEdit, &QPlainTextEdit::paste);
     *  editMenu->addAction(pasteAct);
     *  editToolBar->addAction(pasteAct);
     *
     *  menuBar()->addSeparator();
     *
     #endif // !QT_NO_CLIPBOARD
     *
     *  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
     *  QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
     *  aboutAct->setStatusTip(tr("Show the application's About box"));
     *
     *
     *  QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
     *  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
     *
     #ifndef QT_NO_CLIPBOARD
     *  cutAct->setEnabled(false);
     *  copyAct->setEnabled(false);
     *  connect(textEdit, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
     *  connect(textEdit, &QPlainTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
     #endif // !QT_NO_CLIPBOARD
     */
    connect(fileMenu, &QMenu::aboutToShow, [ = ] ()
    {
        SetupDynamicFileMenuActions();
    });

    m_recentFileMenu = fileMenu->addMenu(tr("&Recent files..."));
    {
        QAction *action_p = new QAction();
        connect(m_recentFileMenu, &QMenu::aboutToShow, [ = ] ()
        {
            setupRecentFiles();
        });
        m_recentFileMenu->setDefaultAction(action_p);
    }
}

/***********************************************************************************************************************
*   SetupFilterSaveMenuAction
***********************************************************************************************************************/
void MainWindow::SetupDynamicFileMenuActions(void)
{
    /* Dynamically add save on filter */
    QString fileName = "";
    QString saveFilterAsMenuText = "";
    QString saveFilterMenuText = "";
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);

    if (selectionList.count() == 1) {
        auto filter = reinterpret_cast<CCfgItem_Filter *>(selectionList.first());
        fileName = *filter->m_filter_ref_p->GetFileName();
        if (fileName.size() > 0) {
            QFileInfo fileInfo(fileName);
            saveFilterMenuText = "Save " + fileInfo.fileName();
            saveFilterAsMenuText = "Save " + fileInfo.fileName() + " As...";
        }
    } else if (selectionList.count() > 1) {
        saveFilterMenuText = "Save Multiple Filters";
        saveFilterAsMenuText = "Save Multiple Filters As";
    }

    if (saveFilterMenuText.size() > 0) {
        m_saveFilterAct->setEnabled(true);
        m_saveFilterAsAct->setEnabled(true);
        m_saveFilterAct->setText(saveFilterMenuText);
        m_saveFilterAsAct->setText(saveFilterAsMenuText);
    } else {
        m_saveFilterAct->setEnabled(false);
        m_saveFilterAsAct->setEnabled(false);
        m_saveFilterAct->setText("Save Filter");
        m_saveFilterAsAct->setText("Save Filter As");
    }
}

/***********************************************************************************************************************
*   updateLogFileTrackState
***********************************************************************************************************************/
void MainWindow::updateLogFileTrackState(bool track)
{
    auto doc_p = GetTheDoc();
    g_cfg_p->m_logFileTracking = track;
    doc_p->enableLogFileTracking(g_cfg_p->m_logFileTracking);
    m_checkBox_p->setCheckState(track ? Qt::Checked : Qt::Unchecked);
}

/***********************************************************************************************************************
*   setupRecentFiles
***********************************************************************************************************************/
void MainWindow::setupRecentFiles(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QList<RecentFile_Kind_e> kindList;
    QStringList list;

    /* First clear all previous actions */
    m_recentFileMenu->clear();

    kindList.append(RecentFile_Kind_LogFile_en);
    doc_p->m_recentFiles.GetRecentPaths(list, kindList);
    if (!list.isEmpty()) {
        for (auto& path : list) {
            /*const QIcon saveAsIcon = QIcon::fromTheme("document-save-as", QIcon(":save-as-32")); */
            auto act = m_recentFileMenu->addAction(QIcon(":text.png"), path, this,
                                                   [ = ] () {
                QList<QString> fileList;
                fileList.append(path);
                CFGCTRL_LoadFiles(fileList);
            });
            act->setEnabled(true);
        }

        while (!kindList.isEmpty()) {
            kindList.takeFirst();
        }
        while (!list.isEmpty()) {
            list.takeFirst();
        }
    }

    kindList.append(RecentFile_Kind_FilterFile_en);
    doc_p->m_recentFiles.GetRecentPaths(list, kindList);
    if (!list.isEmpty()) {
        for (auto& path : list) {
            auto act = m_recentFileMenu->addAction(QIcon(":filterItem.bmp"), path, this,
                                                   [ = ] () {
                QList<QString> fileList;
                fileList.append(path);
                CFGCTRL_LoadFiles(fileList);
            });
            act->setEnabled(true);
        }

        while (!kindList.isEmpty()) {
            kindList.takeFirst();
        }
        while (!list.isEmpty()) {
            list.takeFirst();
        }
    }

    kindList.append(RecentFile_Kind_PluginFile_en);
    doc_p->m_recentFiles.GetRecentPaths(list, kindList);
    if (!list.isEmpty()) {
        for (auto& path : list) {
            auto act = m_recentFileMenu->addAction(QIcon(":plugin_32x32.png"), path, this,
                                                   [ = ] () {
                QList<QString> fileList;
                fileList.append(path);
                CFGCTRL_LoadFiles(fileList);
            });

            act->setEnabled(true);
        }

        while (!kindList.isEmpty()) {
            kindList.takeFirst();
        }
        while (!list.isEmpty()) {
            list.takeFirst();
        }
    }

    kindList.append(RecentFile_Kind_WorkspaceFile_en);
    doc_p->m_recentFiles.GetRecentPaths(list, kindList);

    if (!list.isEmpty()) {
        for (auto& path : list) {
            auto act = m_recentFileMenu->addAction(QIcon(":logo.png"), path, this,
                                                   [ = ] () {
                QList<QString> fileList;
                fileList.append(path);
                CFGCTRL_LoadFiles(fileList);
            });

            act->setEnabled(true);
        }
    }
}

/***********************************************************************************************************************
*   closeEvent
***********************************************************************************************************************/
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        TRACEX_I(QString("\n\nSystem shutdown\n\n"))
        CSCZ_SystemState = SYSTEM_STATE_SHUTDOWN;
    }

    savePendingStateGeometry(windowState());

    if (m_treeView_p != nullptr) {
        m_treeView_p->storeSettings();
    }
    if (m_logWindow_p != nullptr) {
        m_logWindow_p->storeSettings();
    }
    if (m_editor_p != nullptr) {
        m_editor_p->storeSettings();
    }
    if (m_searchWidget_p != nullptr) {
        m_searchWidget_p->storeSettings();
    }
    if (m_plotPane_p != nullptr) {
        m_plotPane_p->storeSettings();
    }

    saveExitState();

    if (maybeSave()) {
        event->accept();
    } else {
        QMainWindow::closeEvent(event);
    }
}

/***********************************************************************************************************************
*   maybeSave
***********************************************************************************************************************/
bool MainWindow::maybeSave(void)
{
    /*
     *  //if (!textEdit->document()->isModified())
     *  //    return true;
     *  const QMessageBox::StandardButton ret
     *   = QMessageBox::warning(this, tr("Application"),
     *       tr("The document has been modified.\n"
     *           "Do you want to save your changes?"),
     *       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
     *  switch (ret) {
     *  case QMessageBox::Save:
     *   return save();
     *  case QMessageBox::Cancel:
     *   return false;
     *  default:
     *   break;
     *  }
     *  return true;
     */
    return true; /* always quit */
}

/***********************************************************************************************************************
*   open
***********************************************************************************************************************/
bool MainWindow::open(void)
{
    QStringList filters;
    filters << "Text files (*.txt *.log)";

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_LogFile_en);
    return CFGCTRL_Load_FileType(QString("Load plugin"), filters, kindList,
                                 GetTheDoc()->m_recentFiles.GetRecentPath(RecentFile_Kind_LogFile_en));
}

/***********************************************************************************************************************
*   saveDefaultWorkspace
***********************************************************************************************************************/
bool MainWindow::saveDefaultWorkspace(void)
{
    CFGCTRL_SaveDefaultWorkspaceFile();
    return true;
}

/***********************************************************************************************************************
*   save
***********************************************************************************************************************/
bool MainWindow::save(void)
{
    return true;
}

/***********************************************************************************************************************
*   saveAs
***********************************************************************************************************************/
bool MainWindow::saveAs(void)
{
    return true;
}

/***********************************************************************************************************************
*   reloadCSS
***********************************************************************************************************************/
bool MainWindow::reloadCSS(void)
{
    APP_UPDATE_CSS();
    return true;
}

/***********************************************************************************************************************
*   startDevTest
***********************************************************************************************************************/
bool MainWindow::startDevTest(void)
{
    extern void MainTest();
    MainTest();
    return true;
}

/***********************************************************************************************************************
*   dragEnterEvent
***********************************************************************************************************************/
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    /* if some actions should not be usable, like move, this code must be adopted */
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

/***********************************************************************************************************************
*   dragMoveEvent
***********************************************************************************************************************/
void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    /* if some actions should not be usable, like move, this code must be adopted */
    event->acceptProposedAction();
}

/***********************************************************************************************************************
*   dragLeaveEvent
***********************************************************************************************************************/
void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

/***********************************************************************************************************************
*   doShow
***********************************************************************************************************************/
void MainWindow::doShow(void)
{
    raise();
    setFocus();
    show();
}

/***********************************************************************************************************************
*   activateSearch
***********************************************************************************************************************/
void MainWindow::activateSearch(const QString& searchText, bool caseSensitive, bool regExp)
{
    m_searchWidget_p->updateSearchParameters(searchText, caseSensitive, regExp);
    m_tabWidget_p->setCurrentWidget(m_searchWidget_p);
    m_searchWidget_p->activate();
}

/***********************************************************************************************************************
*   updateSearchParameters
***********************************************************************************************************************/
void MainWindow::updateSearchParameters(const QString& searchText, bool caseSensitive, bool regExp)
{
    m_searchWidget_p->updateSearchParameters(searchText, caseSensitive, regExp);
}

/*
 * The Search start needs to be adjusted to potential rowClipping and number of rows in file
 * The adjustments are done from where the cursor is located.
 */
bool MainWindow::adjustSearchStart(bool forward, CSelection& cursorPosition, int& startRow, int& endRow,
                                   int& numOfFastSearchRows, bool& doFullSearch)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    numOfFastSearchRows = 1;
    doFullSearch = true;

    CSelection activeSelection;
    QString text;
    if (CEditorWidget_GetActiveSelection(activeSelection, text)) {
        /* If there is an active selection, we move the cursor to either infront, or after,
         * depending on search direction */
        cursorPosition.row = activeSelection.row;
        if (forward) {
            cursorPosition.endCol = activeSelection.endCol + 1;
            cursorPosition.startCol = cursorPosition.endCol;
        } else {
            cursorPosition.startCol = activeSelection.startCol;
            cursorPosition.endCol = cursorPosition.startCol;
        }
    }

    if (!forward) {
        /* search backwards */

        endRow = 0;
        startRow = cursorPosition.row - 1;

        /* To avoid confusion... as user might not really understand row clips, hence make search
         * restart from bottom (end of region) if user is outside (before) clip area. */
        if ((g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) && (startRow <= g_cfg_p->m_Log_rowClip_Start)) {
            startRow = g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET ?
                       g_cfg_p->m_Log_rowClip_End : doc_p->m_database.TIA.rows - 1;
        }

        /* Make sure not to start search after of zoom/rowclip... as well to avoid confusion about row clips */
        if ((g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET) && (startRow >= g_cfg_p->m_Log_rowClip_End)) {
            startRow = g_cfg_p->m_Log_rowClip_End - 1;
        }

        /* Make sure not to start search infront of zoom/rowclip */
        if ((g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) && (endRow <= g_cfg_p->m_Log_rowClip_Start)) {
            endRow = g_cfg_p->m_Log_rowClip_Start + 1;
        }

        if (startRow <= 0) {
            doFullSearch = false;
            if (startRow == 0) {
                numOfFastSearchRows = 2;
            } else {
                numOfFastSearchRows = 1;
            }
        }

        if (startRow <= endRow) {
            startRow = g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET ?
                       g_cfg_p->m_Log_rowClip_End - 1 : doc_p->m_database.TIA.rows - 1;
        }

        Q_ASSERT(startRow > endRow); /* searching backwards */

        if (startRow < endRow) {
            MW_PlaySystemSound(SYSTEM_SOUND_FAILURE);
            TRACEX_I("Search failed, Backward TOO FEW LINES to SEARCH, startRow:%d endRow:%d\n", startRow, endRow)
            return false;
        }
    } else {
        endRow = doc_p->m_database.TIA.rows - 1;
        startRow = cursorPosition.row + 1;

        if (startRow >= doc_p->m_database.TIA.rows - 1) {
            doFullSearch = false;
            numOfFastSearchRows = 2;
        }

        /* When row clip is set, to avoid confusion the search will wrap */
        if ((g_cfg_p->m_Log_rowClip_End > -1) && (startRow >= g_cfg_p->m_Log_rowClip_End)) {
            if (startRow == g_cfg_p->m_Log_rowClip_End) {
                /* special case, cursor at last line */
                doFullSearch = false;
                numOfFastSearchRows = 2;
            } else {
                startRow = g_cfg_p->m_Log_rowClip_Start > -1 ? g_cfg_p->m_Log_rowClip_Start : 0;
            }
        }

        if ((g_cfg_p->m_Log_rowClip_Start > -1) && (startRow <= g_cfg_p->m_Log_rowClip_Start)) {
            startRow = g_cfg_p->m_Log_rowClip_Start + 1;
        }

        /* Make sure not to start search after of zoom/rowclip */
        if ((g_cfg_p->m_Log_rowClip_End > -1) && (endRow >= g_cfg_p->m_Log_rowClip_End)) {
            endRow = g_cfg_p->m_Log_rowClip_End - 1;
        }

        if (startRow >= endRow) {
            startRow = g_cfg_p->m_Log_rowClip_Start > -1 ? g_cfg_p->m_Log_rowClip_Start : 0;
        }

        Q_ASSERT(startRow < endRow);

        if (startRow > endRow) {
            MW_PlaySystemSound(SYSTEM_SOUND_FAILURE);
            TRACEX_I("Search failed, Forward TOO FEW LINES to SEARCH, startRow:%d endRow:%d\n", startRow, endRow)
            return false;
        }
    }

    return true;
}

/***********************************************************************************************************************
*   event
***********************************************************************************************************************/
bool MainWindow::event(QEvent *event)
{
    bool res = false;
    auto type = event->type();
    QString msg;

    switch (type)
    {
        case QEvent::ChildAdded:
            msg = QString("MW QEvent::ChildAdded");
            break;

        case QEvent::ChildPolished:
            msg = QString("MW QEvent::ChildPolished");
            break;

        case QEvent::DynamicPropertyChange:
            msg = QString("MW QEvent::DynamicPropertyChange");
            break;

        case QEvent::Move:
            msg = QString("MW QEvent::Move");
            break;

        case QEvent::WindowIconChange:
            msg = QString("MW QEvent::WindowIconChange");
            break;

        case QEvent::UpdateLater:
            msg = QString("MW QEvent::UpdateLater");
            break;

        case QEvent::ShowToParent:
            msg = QString("MW QEvent::ShowToParent");
            break;

        case QEvent::ZOrderChange:
            msg = QString("MW QEvent::ZOrderChange");
            break;

        case QEvent::Resize:
            msg = QString("MW QEvent::Resize");
            break;

        case QEvent::ActivationChange: /*msg = QString("MW QEvent::ActivationChange"); break; */
        case QEvent::LayoutRequest:
            msg = QString("MW QEvent::LayoutRequest");
            break;

        case QEvent::Timer:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:

        case QEvent::StatusTip:
        case QEvent::ToolTip:
        case QEvent::UpdateRequest:  /* repaint request */
        case QEvent::Paint:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::Wheel:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
        case QEvent::FutureCallOut:
        case QEvent::CursorChange:
        case QEvent::WindowTitleChange:
        case QEvent::InputMethodQuery:

            /* skip */
            break;

        default:
            msg = QString("MW Event %1").arg(event->type());
            break;
    }

    if (!msg.isEmpty()) {
        PRINT_SIZE(msg)
    }

    switch (event->type())
    {
        case QEvent::KeyPress:
        {
            QKeyEvent *k = reinterpret_cast<QKeyEvent *>(event);
            if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
                if ((k->key() == Qt::Key_Backtab) ||
                    ((k->key() == Qt::Key_Tab) && (k->modifiers() & Qt::ShiftModifier))) {
                    res = CPlotPane_NextPlot(true);
                } else if (k->key() == Qt::Key_Tab) {
                    res = CPlotPane_NextPlot(false);
                }
                if (res) {
                    return true;
                }
            }
            break;
        } /*KeyPress */

        default:
            break;
    }
    return QMainWindow::event(event);
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void MainWindow::keyPressEvent(QKeyEvent *e)
{
    int index = e->key();
    auto SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    auto CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;

    PRINT_KEYPRESS(QString("%1 key:%2").arg(__FUNCTION__).arg(index))

    if (!MW_GeneralKeyHandler(index, CTRL_Pressed, SHIFT_Pressed)) {
        QMainWindow::keyPressEvent(e);
    } else {
        e->accept();
    }
}

/***********************************************************************************************************************
*   handleSearch
***********************************************************************************************************************/
bool MainWindow::handleSearch(bool forward)
{
    bool searchResult = false;
    int searchRow = 0;
    QString searchText;
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    CSelection cursorPosition = CEditorWidget_GetCursorPosition();
    CSelection editorSelection;
    QString editorSelectionText;
    int startRow = 0;
    int endRow = doc_p->m_database.TIA.rows - 1;
    CSelection searchResultSelection;
    int numOfFastSearchRows = 0;
    bool doFullSearch = false;
    bool caseSensitive, regExp;

    if (doc_p->m_database.TIA.rows == 0) {
        return false;
    }

    m_searchWidget_p->getSearchParameters(searchText, &caseSensitive, &regExp);

    /* Using the latest selection is a bit troublesome since when doing a succesfull search that in it self
     * will cause a new text selection. This is not an issue unless you consider regular expressions...
     *
     *  if (CEditorWidget_GetActiveSelection(editorSelection, editorSelectionText)) {
     *  searchText = editorSelectionText;
     *  }
     */
    if (CSZ_DB_PendingUpdate) {
        return false;
    }

    /* Update startRow, endRow based on number of rows and if rowClipping is enabled. */
    adjustSearchStart(forward, cursorPosition, startRow, endRow, numOfFastSearchRows, doFullSearch);

    TRACEX_I(QString("Search cursor row:%1 col:%2 -> startRow:%3")
                 .arg(cursorPosition.row).arg(cursorPosition.startCol).arg(startRow))

#ifdef QT_TODO
    /* Having the setFocus here will force the last edit in the combo box to be saved, and get be
     * fetched with GetEditString(...) */
    view_p->SetFocus();
#endif

    if (searchText == "%%devtools%%") {
        g_cfg_p->m_devToolsMenu = true;
#ifdef QT_TODO
        DevToolsMenu(true);
#endif
    }

    if (!searchText.isEmpty()) {
        g_processingCtrl_p->Processing_StartReport();

        /* First check if there are more matches on the same row
         * (NextOnRowSearch), otherwise start the "full" search */

        if (numOfFastSearchRows > 0) {
            auto tempCursorPos = cursorPosition;
            while (numOfFastSearchRows != 0 && !searchResult) {
                searchResult =
                    doc_p->m_autoHighLighter_p->SearchAndUpdateNextAutoHighlight(
                        searchText.toLatin1().constData(), &tempCursorPos /*start*/,
                        forward, &searchResultSelection, caseSensitive, regExp);

                if (searchResult) {
                    searchRow = static_cast<int>(tempCursorPos.row);
                } else {
                    /* When looping we start from the end, or beginning of a new line */
                    numOfFastSearchRows--;
                    tempCursorPos.startCol = -1;
                    tempCursorPos.endCol = -1;
                    tempCursorPos.row += forward == true ? 1 : -1;
                }
            }
        }

        if (!searchResult && doFullSearch && (startRow != endRow)) {
            /* Full search */
            TRACEX_I(QString("Full search:%1 startRow:%2 endRow:%3\n").arg(searchText).arg(startRow).arg(endRow))

            searchResult = doc_p->StartSearch(
                searchText,
                startRow,
                endRow,
                &searchRow,
                forward == true ? false : true /*backward*/,
                (CEditorWidget_isPresentationModeFiltered() &&
                 doc_p->m_database.FIRA.filterMatches > 0) ? true : false,
                regExp,
                caseSensitive);

            if (searchResult) {
                searchResultSelection.row = searchRow;
                searchResultSelection.startCol = -1;
                searchResultSelection.endCol = -1;

                /* SearchAndUpdateNextAutoHighlight will update
                 * searchResultSelection with the either first or last match
                 * depending on "forward" */
                searchResult =
                    doc_p->m_autoHighLighter_p->SearchAndUpdateNextAutoHighlight(
                        searchText.toLatin1().constData(),
                        &searchResultSelection /*start*/,
                        forward, &searchResultSelection,
                        caseSensitive, regExp);
            }
        }

        g_processingCtrl_p->Processing_StopReport();

        if (searchResult) {
            CEditorWidget_EmptySelectionList();
            m_searchWidget_p->addToHistory(searchText);
            CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_SearchRun_e);
#ifdef QT_TODO
            view_p->SetFocus();
#endif
        }
    }

    if (searchResult) {
        TRACEX_I("Search OK, Row:%d\n", searchResultSelection.row)

        /* Prevent the autohighlight to be updated */
        doc_p->m_autoHighLighter_p->EnableAutoHighlightAutomaticUpdates(false);

        /* will move the cursor as well */
        CEditorWidget_AddSelection(searchResultSelection.row, searchResultSelection.startCol,
                                   searchResultSelection.endCol, true, true, true, forward /*put cursor after*/);

        doc_p->m_autoHighLighter_p->EnableAutoHighlightAutomaticUpdates(true);

#ifdef QT_TODO
        view_p->HorizontalCursorFocus(HCursorScrollAction_Focus_en);
#endif
        if (CEditorWidget_isPresentationModeFiltered() &&
            ((doc_p->m_rowCache_p->GetFilterRef(searchRow) == nullptr) ||
             doc_p->m_rowCache_p->GetFilterRef(searchRow)->m_exclude)) {
            CEditorWidget_SetPresentationMode_ShowAll();
        }

        CEditorWidget_SearchNewTopLine(searchResultSelection.row);
        update();
    } else {
        TRACEX_I("Search Failed\n")
        MW_PlaySystemSound(SYSTEM_SOUND_FAILURE);
    }

    return true;
}

/***********************************************************************************************************************
*   changeEvent
***********************************************************************************************************************/
void MainWindow::changeEvent(QEvent *event)
{
    static bool doExit = false;

    if (doExit) {
        return;
    }
    doExit = true;
    PRINT_SIZE(QString("MW change event %1").arg(event->type()))
    if (event->type() == QEvent::WindowStateChange) {
        /* Prevent any pending update caused by the state change */
        if (m_pendingGeometryTimer.get() != nullptr) {
            m_pendingGeometryTimer.get()->stop();
        }

        auto ch_event = static_cast<QWindowStateChangeEvent *>(event);
        auto oldState = ch_event->oldState();
        auto newState = windowState();
        PRINT_SIZE(QString("MW state %1 -> %2").arg(oldState).arg(newState))
        savePendingStateGeometry(oldState);
        m_activeState = newState;
        restoreSavedStateGeometry();

        /*setupAdaptWindowSizes(false); */
        PRINT_SIZE(QString("MW after restore, active state %1").arg(m_activeState))
        if (newState != m_activeState) {
            TRACEX_W(QString("Restore messed up the window state %1 %2").arg(m_activeState).arg(newState))
        }
    }
    QMainWindow::changeEvent(event);
    doExit = false;
}

/***********************************************************************************************************************
*   dropEvent
***********************************************************************************************************************/
void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QString> pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (auto& url : urlList) {
            pathList.append(url.toLocalFile());
        }

        if (CFGCTRL_LoadFiles(pathList)) {
            event->acceptProposedAction();
        }
    }
}

/***********************************************************************************************************************
*   moveEvent
***********************************************************************************************************************/
void MainWindow::moveEvent(QMoveEvent *event)
{
    /* To avoid the cursor toggeling while the window is being moved, as the cursor toggeling is repainting the
     * entire screen and causes some glitches. */
    if (m_editor_p != nullptr) {
        m_editor_p->mainWindowIsMoving(event);
    }
    PRINT_SIZE(QString("MW move x:%1 -> %2").arg(event->oldPos().x()).arg(event->pos().x()))
    QMainWindow::moveEvent(event);

    /*  updatePendingStateGeometry(); */
}

/***********************************************************************************************************************
*   resizeEvent
***********************************************************************************************************************/
void MainWindow::resizeEvent(QResizeEvent *event)
{
    auto screenList = QGuiApplication::screens();
    auto first_screen = screenList.first();
    auto rec = first_screen->size();

    PRINT_SIZE(QString("MainWindow resize %1,%2 -> %3,%4 state:%5 (max %6,%7 - %8,%9)")
                   .arg(event->oldSize().width()).arg(event->oldSize().height())
                   .arg(event->size().width()).arg(event->size().height())
                   .arg(windowState()).arg(rec.width()).arg(rec.height())
                   .arg(mapToGlobal(QPoint(0, 0)).x()).arg(mapToGlobal(QPoint(0, 0)).y()))
    updatePendingStateGeometry();

    if (CSCZ_AdaptWindowSizes) {
        updateGeometry();
    }
    QMainWindow::resizeEvent(event);
}

/* Setup the QTreeView with our own specific QAbstractItemModel, and attach a QItemSelectionModel
 *
 * The QTreeView/Model is the interaction with the user. CWorkspace, the LS legacy impl. is more or a lib supporting
 * the model/view */
QTreeView *MainWindow::setupModelView(QWidget *parentWidget)
{
    g_workspace_p->m_model_p = new Model(this);

    /* Selection model */
    QItemSelectionModel *selections = new SelectionModel(static_cast<QAbstractItemModel *>(g_workspace_p->m_model_p));
    CWorkspaceTreeView *tree = new CWorkspaceTreeView(parentWidget);
    m_treeView_p = tree;

    tree->setModel(static_cast<QAbstractItemModel *>(g_workspace_p->m_model_p));
    tree->setSelectionModel(selections);

    connect(selections, SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            tree, SLOT(selectionChanged(const QItemSelection&,const QItemSelection&)));

    return tree;
}

/***********************************************************************************************************************
*   MW_selectionModel
***********************************************************************************************************************/
QItemSelectionModel *MW_selectionModel(void)
{
    return (g_mainWindow_p->selectionModel());
}

/***********************************************************************************************************************
*   selectionModel
***********************************************************************************************************************/
QItemSelectionModel *MainWindow::selectionModel(void)
{
    return (m_treeView_p->selectionModel());
}
