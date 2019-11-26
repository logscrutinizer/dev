/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ceditorwidget.h"
#include "cworkspaceTreeView.h"
#include "CCfgItem.h"
#include "cplotpane.h"
#include "cplotwidget.h"
#include "csearchwidget.h"

#include <map>

/* This is for the Model */
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QIcon>
#include <QVector>
#include <QSplitter>
#include <QTreeView>

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTabWidget>
#include <QPlainTextEdit>

/***********************************************************************************************************************
*   CLogWindow
***********************************************************************************************************************/
class CLogWindow : public QPlainTextEdit
{
    Q_OBJECT

    /* snip */

public:
    CLogWindow() {
        setReadOnly(true);
    }

    void storeSettings(void);

    /****/
    void wheelEvent(QWheelEvent *event) override {
        auto CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
        if (CTRL_Pressed) {
            int zDelta = 0;
            QPoint numPixels = event->pixelDelta();
            QPoint numDegrees = event->angleDelta() / 8;

            if (!numDegrees.isNull()) {
                zDelta = numDegrees.y();
            } else if (!numPixels.isNull()) {
                zDelta = numPixels.y();
            }

            if (zDelta < 0) {
                zoomOut();
            } else {
                zoomIn();
            }
            event->accept();
        } else {
            QPlainTextEdit::wheelEvent(event);
        }
    }

    /****/
    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        static QSize windowSize;
        auto refreshEditorWindow = makeMyScopeGuard([&] () {
            PRINT_SIZE(QString("Log sizeHint %1,%2").arg(windowSize.width()).arg(windowSize.height()))
        });
        windowSize = QWidget::sizeHint();

        if (CSCZ_AdaptWindowSizes) {
            windowSize = m_adaptWindowSize;
            PRINT_SIZE(QString("Log adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
        }

        return windowSize;
    }

    void setAdaptWindowSize(const QSize& size) {m_adaptWindowSize = size;}

private:
    QSize m_adaptWindowSize;

public slots:
    void message_receiver(const QString& message);
};

/***********************************************************************************************************************
*   CMenuSlider
***********************************************************************************************************************/
class CMenuSlider : public QSlider
{
    Q_OBJECT

public:
    CMenuSlider() : QSlider(Qt::Horizontal) {}

    /****/
    virtual QSize sizeHint() const override {
        return QSize(300, 10);
    }
};

/***********************************************************************************************************************
*   CGeometryState
***********************************************************************************************************************/
class CGeometryState
{
public:
    QString stateKey;
    QString geoKey;
    QByteArray state;
    QByteArray geometry;
    QSize mainWindow;
    QSize editor;
    QSize workspace;
    QSize log;
    QSize search;
};

/***********************************************************************************************************************
*   CTabWidget
***********************************************************************************************************************/
class CTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit CTabWidget(QWidget *parent = Q_NULLPTR) : QTabWidget(parent) {}
    virtual ~CTabWidget() Q_DECL_OVERRIDE {}

    /****/
    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        static bool loadSettings = true;
        static QSize windowSize = QSize();
        auto refreshEditorWindow = makeMyScopeGuard([&] () {
            PRINT_SIZE(QString("TabWidget sizeHint %1,%2 load:%3").arg(windowSize.width()).arg(windowSize.height())
                           .arg(loadSettings))
            loadSettings = false;
        });

        if (CSCZ_AdaptWindowSizes) {
            windowSize = m_adaptWindowSize;
            PRINT_SIZE(QString("TabWidget adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
        }

        return windowSize;
    }

    void setAdaptWindowSize(const QSize& size) {m_adaptWindowSize = size;}

private:
    QSize m_adaptWindowSize;
};

/***********************************************************************************************************************
*   MainWindow
***********************************************************************************************************************/

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow() override;

    void getSettingKeys(QString& geo, QString& state) const;

    /* If there where no window sizes saved in settings then LS will do a basic layout 20/80% */
    void setupPredefinedWindowSizes_step1(bool doResize = false);
    void setupPredefinedWindowSizes_step2(void);

    void saveExitState(void);
    bool isExitStateMaximized(void);

    /* Returns false if there were no state/geometry saved in settings, typically scenario at first start after
     * installation */
    void restoreSavedStateGeometry(bool enable_fallback = true);

    void setupPredefinedWindowSizes(void);
    void updatePendingStateGeometry(void);
    void savePendingStateGeometry(Qt::WindowStates previousState);

    void createActions(void);
    bool maybeSave(void);
    bool open(void);
    bool save(void);
    bool saveAs(void);
    bool reloadCSS(void);
    bool startDevTest(void);
    bool saveDefaultWorkspace(void);
    void setupRecentFiles(void);

    void setCommandLineParams(const QStringList& list) {m_commandLine = list;}

    void addPlotPane(void);
    void removePlotPane(void);
    bool hasPlotPane(void); /* Return true if the PlotPlan is created/exist */

    CPlotWidgetInterface *attachPlot(CPlot *plot_p);
    void detachPlot(CPlot *plot_p);
    void setSubPlotUpdated(CSubPlot *subPlot_p);
    void redrawPlot(CPlot *plot_p);
    void setWorkspaceWidgetFocus(void);

    void startWebPage(const QString& url);

    void modifyFontSize(int increase);

    QItemSelectionModel *selectionModel(void); /* get the selection model for QTreeView (m_treeView_p) */

    void doShow(void);
    bool adjustSearchStart(bool forward, CSelection& cursorPosition, int& startRow, int& endRow,
                           int& numOfFastSearchRows, bool& doFullSearch);

    bool handleSearch(bool forward);
    void activateSearch(const QString& searchText, bool caseSensitive = false, bool regExp = false);
    void updateSearchParameters(const QString& searchText, bool caseSensitive = false, bool regExp = false);

    void SetupDynamicFileMenuActions(void);
    void updateLogFileTrackState(bool track);

    virtual void showEvent(QShowEvent *e) Q_DECL_OVERRIDE;

    /* this event is called when the mouse enters the widgets area during a drag/drop operation */
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

    /* this event is called when the mouse moves inside the widgets area during a drag/drop operation */
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;

    /* this event is called when the mouse leaves the widgets area during a drag / drop operation */
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;

    /* this event is called when the drop operation is initiated at the widget */
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    /* this event is called when the drop operation is initiated at the widget */
    virtual void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void changeEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const override;
    QTreeView *setupModelView(QWidget *parentWidget);
    CWorkspaceTreeView *m_treeView_p;
    CLogWindow *m_logWindow_p;

public slots:
    void appStateChanged(Qt::ApplicationState state);

private:
    std::unique_ptr<QTimer> m_pendingGeometryTimer;
    std::unique_ptr<QTimer> m_pendingWindowAdaptationTimer;
    std::unique_ptr<QTimer> m_pendingWindowAdaptationTimer_2;

    /*bool m_pendingAdaptationSetup = false; */
    CEditorWidget *m_editor_p;
    CTabWidget *m_tabWidget_p;
    CSearchWidget *m_searchWidget_p;
    QCheckBox *m_checkBox_p;
    QDockWidget *m_tabPlotPaneDock_p;
    CPlotPane *m_plotPane_p;   /* is a QTabWidget */
    QAction *m_saveFilterAct;
    QAction *m_saveFilterAsAct;
    QMenu *m_recentFileMenu;
    QStringList m_commandLine;
    std::map<Qt::WindowStates /*key*/, CGeometryState /*data*/> m_pendingGeometryState;
    Qt::WindowStates m_activeState = Qt::WindowNoState; /* to keep track of what has been given by event */
};

#endif /* MAINWINDOW_H */
