/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"
#include "cplotwidget_graphics.h"
#include "CProgressCtrl.h"
#include "CLogScrutinizerDoc.h"

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "mainwindow_cb_if.h"
#include "CWorkspace_cb_if.h"
#include "cplotpane_cb_if.h"

#include "../processing/CThread.h"

#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <QWidget>
#include <QDockWidget>
#include <QRect>
#include <QBitmap>
#include <QTimer>
#include <QTextOption>
#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QClipboard>
#include <QEvent>
#include <QGraphicsTextItem>

extern QPen *g_plotWnd_defaultPen_p;
extern QPen *g_plotWnd_defaultDotPen_p;
extern FontItem_t *g_plotWnd_GrayFont_p;
extern FontItem_t *g_plotWnd_BlackFont_p;
extern FontItem_t *g_plotWnd_WhiteFont_p;

/***********************************************************************************************************************
*   Populate
***********************************************************************************************************************/
void CGraphicsSubplotSurface::Populate(void)
{
    char *subTitle_p;
    char *Y_AxisLabel_p;

    m_scene.clear();
    m_view.setBackgroundBrush(QBrush(BACKGROUND_COLOR));
    m_label_refs_a.clear();

    m_subPlot_p->GetTitle(&subTitle_p, &Y_AxisLabel_p);

    /* Setup the label array for quick access */
    CList_LSZ *labelList_p;
    m_subPlot_p->GetLabels(&labelList_p);

    if ((labelList_p != nullptr) && (labelList_p->count() > 0)) {
        m_label_refs_a = std::vector<CGO_Label *>(static_cast<std::vector<CGO_Label>::size_type>(labelList_p->count()));

        auto label_p = reinterpret_cast<CGO_Label *>(labelList_p->first());
        unsigned int index = 0;
        while (label_p != nullptr) {
            m_label_refs_a[index++] = label_p;
            label_p = reinterpret_cast<CGO_Label *>(labelList_p->GetNext(reinterpret_cast<CListObject *>(label_p)));
        }

        m_numOfLabelRefs = static_cast<int>(index);
    }

    if (g_cfg_p->m_pluginDebugBitmask != 0) {
        TRACEX_I("   NumOfLabels:%d", labelList_p->count())
    }

    /* Transfer graphicalObject data to m_displayGraphs_a */

    CList_LSZ *graphList_p;
    QPen pen;
    pen.setWidth(1);
    pen.setCosmetic(true);

    m_subplot_properties = m_subPlot_p->GetProperties();

    m_subPlot_p->GetGraphs(&graphList_p);

    auto numOfDisplayGraphs = graphList_p->count();

    if (numOfDisplayGraphs != 0) {
        CGraph *graph_p = reinterpret_cast<CGraph *>(graphList_p->first());
        int graphIndex = 0;
        int itemIndex = 0;

        while (graph_p != nullptr) {
#ifdef _DEBUG
            if (graphIndex >= numOfDisplayGraphs) {
                TRACEX_E("CSubPlotSurface::CSubPlotSurface  Graph object corrupt, numOfItems doesn't match")
            }
#endif

            const auto numOfItems = graph_p->GetNumOfObjects();
            bool isOverrideColorSet;
            int overrideColor;
            GraphLinePattern_e overrideLinePattern;

            graph_p->GetOverrides(&isOverrideColorSet, &overrideColor, &overrideLinePattern);

            if (numOfItems > 0) {
                itemIndex = 0;

                GraphicalObject_t *go_p = graph_p->GetFirstGraphicalObject();

                while (go_p != nullptr && itemIndex < numOfItems) {
                    if (go_p->properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                        auto item = m_scene.addLine(QLineF(go_p->x1, static_cast<double>(go_p->y1),
                                                           go_p->x2, static_cast<double>(go_p->y2)));
                        item->setPen(pen);
                    } else {
                        auto item = m_scene.addRect(QRectF(go_p->x1, static_cast<double>(go_p->y1),
                                                           go_p->x2, static_cast<double>(go_p->y2)));
                        item->setPen(pen);
                    }
                    go_p = graph_p->GetNextGraphicalObject();
                    ++itemIndex;
                }

#ifdef _DEBUG
                if (itemIndex > numOfItems) {
                    TRACEX_E("CSubPlotSurface::CSubPlotSurface  Graph object corrupt, numOfItems doesn't match")
                }
#endif
            }

            if (g_cfg_p->m_pluginDebugBitmask != 0) {
                GraphicalObject_Extents_t go_extents;

                graph_p->GetExtents(&go_extents);

                TRACEX_I("   Graph(%d) Items:%d CO:%d OP:%d\n xmin:%f xmax:%f ymin:%f ymax:%f",
                         graphIndex, numOfItems, overrideColor, overrideLinePattern, go_extents.x_min, go_extents.x_max,
                         static_cast<double>(go_extents.y_min), static_cast<double>(go_extents.y_max))
            }

            graph_p = reinterpret_cast<CGraph *>(graphList_p->GetNext(reinterpret_cast<CListObject *>(graph_p)));
            ++graphIndex;
        }
    }

    m_boundingRect = m_scene.itemsBoundingRect();
    m_scene.setSceneRect(m_boundingRect);

    m_scene.update();
}

CPlotWidgetGraphics::CPlotWidgetGraphics(CPlot *plot_p)
{
    m_inDestructor = false;

    m_plot_p = plot_p;

    CLogScrutinizerDoc *doc_p = GetTheDoc();

    m_toolTipTimer = std::make_unique<QTimer>(this);
    connect(m_toolTipTimer.get(), SIGNAL(timeout()), this, SLOT(onToolTipTimer()));

    m_bgBoarderBrush_p = new QBrush(BOARDER_BACKGROUND_COLOR);
    m_bgBoarderPen_p = new QPen(LTGRAY);

    if (g_plotWnd_GrayFont_p) {
        g_plotWnd_GrayFont_p = doc_p->m_fontCtrl.RegisterFont(
            Q_RGB(g_cfg_p->m_plot_GrayIntensity, g_cfg_p->m_plot_GrayIntensity, g_cfg_p->m_plot_GrayIntensity),
            BACKGROUND_COLOR);
    }
    if (g_plotWnd_BlackFont_p != nullptr) {
        g_plotWnd_BlackFont_p = doc_p->m_fontCtrl.RegisterFont(0, BACKGROUND_COLOR);
    }
    if (g_plotWnd_WhiteFont_p != nullptr) {
        g_plotWnd_WhiteFont_p = doc_p->m_fontCtrl.RegisterFont(Q_RGB(0xff, 0xff, 0xff), BACKGROUND_COLOR);
    }

    m_FontEmptyWindow_p = doc_p->m_fontCtrl.RegisterFont(Q_RGB(0xff, 0xff, 0xff), BACKGROUND_COLOR);
    m_tooltipFont_p = doc_p->m_fontCtrl.RegisterFont(Q_RGB(0xff, 0xff, 0xff), BACKGROUND_COLOR);

    Init();

    m_inFocus = false;
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CPlotWidgetGraphics::Init(void)
{
    m_surfacesInitialized = false;
    m_surfacesCreated = false;

    m_toolTipEnabled = false;
    m_toolTipState = ToolTipState_WaitForRequest;

    RemoveSurfaces();
}

CPlotWidgetGraphics::~CPlotWidgetGraphics()
{
    m_inDestructor = true;

    if (m_bgBoarderBrush_p != nullptr) {
        delete m_bgBoarderBrush_p;
        m_bgBoarderBrush_p = nullptr;
    }

    if (m_bgBoarderPen_p != nullptr) {
        delete m_bgBoarderPen_p;
        m_bgBoarderPen_p = nullptr;
    }

    RemoveSurfaces();
}

/***********************************************************************************************************************
*   RemoveSurfaces
***********************************************************************************************************************/
void CPlotWidgetGraphics::RemoveSurfaces(void)
{
    if (m_hlayout_p != nullptr) {
        while (!m_surfaces.isEmpty()) {
            auto surface_p = m_surfaces.takeLast();
            m_hlayout_p->removeWidget(surface_p->view());
            delete surface_p;
        }

        m_hlayout_p->removeWidget(m_splitter_p);

        delete m_splitter_p;
        m_splitter_p = nullptr;

        delete m_hlayout_p;
        m_hlayout_p = nullptr;
    }
}

/***********************************************************************************************************************
*   event
***********************************************************************************************************************/
bool CPlotWidgetGraphics::event(QEvent *event)
{
    bool res = false;
    switch (event->type())
    {
        case QEvent::KeyPress:
        {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
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
    return QWidget::event(event);
}

/***********************************************************************************************************************
*   resizeEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    update();
}

/***********************************************************************************************************************
*   FillEmptyWindow
***********************************************************************************************************************/
void CPlotWidgetGraphics::FillEmptyWindow(QPainter *painter)
{
    QSize fontSize = GetTheDoc()->m_fontCtrl.GetFontSize();
    auto br = rect();
    auto const y_pos_offset = static_cast<int>(static_cast<double>(fontSize.height() * 1.1));
    auto const y_pos_offset_extra = static_cast<int>(static_cast<double>(fontSize.height() * 1.5));
    auto const y_pos_offset_extra_extra = static_cast<int>(static_cast<double>(fontSize.height() * 2.0));
    int x_pos =
        static_cast<int>(static_cast<double>(br.right() - m_viewRect.left()) * static_cast<double>(0.1));
    int y_pos =
        static_cast<int>(static_cast<double>(br.bottom() - m_viewRect.top()) * static_cast<double>(0.1));

    painter->drawText(x_pos, y_pos, "This is a Plot View");
    TRACEX_DE(QString("Draw text %1 %2").arg(x_pos).arg(y_pos))

    y_pos += fontSize.height() * 2;
    x_pos += fontSize.width();

    painter->drawText(x_pos, y_pos, "The window is empty because you have loaded a plugin but you haven't run it's "
                                    "plot-function yet, or that");
    y_pos += y_pos_offset;
    painter->drawText(x_pos, y_pos, "the plugin plot-function didn't create any graphical objects for LogScrutinizer "
                                    "to present");

    y_pos += y_pos_offset_extra;
    x_pos += fontSize.width();

    char *title_p;
    char *x_axis_p;
    extern CPlugin_DLL_API *CWorkspace_SearchPlugin(CPlot *childPlot_p);
    CPlugin_DLL_API *pluginAPI_p = CWorkspace_SearchPlugin(m_plot_p);

    m_plot_p->GetTitle(&title_p, &x_axis_p);

    if (pluginAPI_p != nullptr) {
        DLL_API_PluginInfo_t *info_p = pluginAPI_p->GetInfo();
        painter->drawText(x_pos, y_pos,
                          QString("(1.) Right-click on Plugins->%1->Plots->%2").arg(info_p->name, title_p));
    } else {
        painter->drawText(x_pos, y_pos, QString("(1.) Right-click on Plots->%1").arg(title_p));
    }

    y_pos += y_pos_offset;
    painter->drawText(x_pos, y_pos, "(2.) Select  Run");

    y_pos += y_pos_offset;
    painter->drawText(x_pos, y_pos, "(3.) LogScrutinizer will then activate the plugin to process the log, "
                                    "and present graphical output in this window");

    y_pos += y_pos_offset_extra;
    painter->drawText(x_pos, y_pos, "Tip: You can also run all the loaded plugin's plot-function by pressing CTRL-R");

    y_pos += y_pos_offset_extra_extra;
    painter->drawText(x_pos, y_pos, "Read more about plugins at www.logscrutinizer.com. Press F1 for quick "
                                    "help web page");
}

/***********************************************************************************************************************
*   SurfaceToClipBoard
***********************************************************************************************************************/
void CPlotWidgetGraphics::SurfaceToClipBoard(const ScreenPoint_t *screenPoint_p)
{
    Q_UNUSED(screenPoint_p)
}

/***********************************************************************************************************************
*   InitilizeSubPlots
***********************************************************************************************************************/
void CPlotWidgetGraphics::InitilizeSubPlots(void)
{
    /* Check the current size of the plot window
     *   Try to size all sub-plots such they fit, although none of the should be smaller than minimum */

    if (m_surfacesInitialized || (GraphsObjectCount() == 0)) {
        return;
    }

    CList_LSZ *subplotList_p;
    m_plot_p->GetSubPlots(&subplotList_p);

    auto subPlot_p = reinterpret_cast<CSubPlot *>(subplotList_p->first());

    while (subPlot_p != nullptr) {
        m_surfaces.append(new CGraphicsSubplotSurface(this, subPlot_p));
        subPlot_p = reinterpret_cast<CSubPlot *>(subplotList_p->GetNext(reinterpret_cast<CListObject *>(subPlot_p)));
    }

    if (m_surfaces.isEmpty()) {
        return;
    }

    m_splitter_p = new QSplitter();
    m_splitter_p->setOrientation(Qt::Vertical);

    for (auto& surface_p : m_surfaces) {
        m_splitter_p->addWidget(surface_p->view());
    }

    /* Distribute initial equal height */
    auto winRect = rect();
    auto subPlotHeight = winRect.height() / m_surfaces.count();
    subPlotHeight = std::max(20, subPlotHeight);

    QList<int> sizes;
    for (int i = 0; i < m_surfaces.count(); i++) {
        sizes.append(subPlotHeight);
    }
    m_splitter_p->setSizes(sizes);

    m_hlayout_p = new QHBoxLayout;
    m_hlayout_p->addWidget(m_splitter_p);
    setLayout(m_hlayout_p);

    Populate();

    /*    m_boundingRect = m_scene_p->itemsBoundingRect();
     *    m_scene_p->setSceneRect(m_boundingRect); */

    /*    OptimumScale(); */

    m_surfacesCreated = true;
    m_surfacesInitialized = true;
    m_rescale = true;
}

/***********************************************************************************************************************
*   GraphsObjectCount
***********************************************************************************************************************/
int CPlotWidgetGraphics::GraphsObjectCount(void)
{
    int count = 0;
    CList_LSZ *subplotList_p;
    m_plot_p->GetSubPlots(&subplotList_p);

    if (subplotList_p->count() == 0) {
        return 0;
    }

    /* Currently only support for one subplot */
    auto subPlot_p = reinterpret_cast<CSubPlot *>(subplotList_p->first());
    CList_LSZ *graphList_p;
    subPlot_p->GetGraphs(&graphList_p);

    if (graphList_p->count() > 0) {
        CGraph *graph_p = reinterpret_cast<CGraph *>(graphList_p->first());
        while (graph_p != nullptr) {
            count += graph_p->GetNumOfObjects();
            graph_p = reinterpret_cast<CGraph *>(graphList_p->GetNext(reinterpret_cast<CListObject *>(graph_p)));
        }
    }
    return count;
}

/***********************************************************************************************************************
*   Populate
***********************************************************************************************************************/
void CPlotWidgetGraphics::Populate(void)
{
    for (auto& surface_p : m_surfaces) {
        surface_p->Populate();
    }
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();

    if (m_keyPressedList.contains(key)) {
        return;
    }

    m_keyPressedList.append(key);

#ifdef _DEBUG
    TRACEX_DE(QString("%1 Key:%2  keyList:%3").arg(__FUNCTION__).arg(e->key()).arg(m_keyPressedList.count()))
#endif

    if (!HandleKeyDown(e)) {
        QWidget::keyPressEvent(e);
    }
}

/***********************************************************************************************************************
*   keyReleaseEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if (!m_keyPressedList.contains(key)) {
        return;
    }

    int index = m_keyPressedList.indexOf(key);
    if (index > -1) {
        m_keyPressedList.removeAt(index);
#ifdef _DEBUG
        TRACEX_DE(QString("%1 Key:%2 keyList:%3")
                      .arg(__FUNCTION__).arg(e->key()).arg(m_keyPressedList.count()))
#endif
    }
    QWidget::keyReleaseEvent(e);
}

/***********************************************************************************************************************
*   wheelEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::wheelEvent(QWheelEvent *event)
{
    bool CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    bool SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    bool updateNeeded = false;
    ScreenPoint_t screenPoint = MakeScreenPoint(event, 0, 0);
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;
    int zDelta = 0;
    if (!numDegrees.isNull()) {
        zDelta = numDegrees.y();
    } else if (!numPixels.isNull()) {
        zDelta = numPixels.y();
    } else {
        TRACEX_W(QString("%1 zDelta:%2").arg(__FUNCTION__).arg(zDelta))
        return;
    }

    if (CTRL_Pressed && SHIFT_Pressed) {
        ModifySubPlotSize(zDelta, &screenPoint, &updateNeeded);
        updateNeeded = true;
    } else if (CTRL_Pressed) {
        ZoomSubPlot_Y_Axis(zDelta, &screenPoint, &updateNeeded);
        updateNeeded = true;
    } else if (SHIFT_Pressed) {
        ZoomSubPlot_X_Axis(zDelta, &screenPoint, &updateNeeded);
        updateNeeded = true;
    } else if (m_keyPressedList.contains(Qt::Key_C)) {
        /* Scroll the color */
        if (zDelta < 0) {
            /* lighter */
            g_cfg_p->m_plot_GrayIntensity += 20;
            g_cfg_p->m_plot_GrayIntensity = g_cfg_p->m_plot_GrayIntensity > 240 ? 240 : g_cfg_p->m_plot_GrayIntensity;
        } else {
            /* stronger */
            g_cfg_p->m_plot_GrayIntensity -= 20;
            g_cfg_p->m_plot_GrayIntensity = g_cfg_p->m_plot_GrayIntensity < 0 ? 0 : g_cfg_p->m_plot_GrayIntensity;
        }

        updateNeeded = true;
    } else {
        /*WHEEL_DELTA , A positive value indicates that the wheel was rotated forward */
        updateNeeded = true;
    }

    if (updateNeeded) {
        update();
        event->accept();
    } else {
        QFrame::wheelEvent(event);
    }
}

/***********************************************************************************************************************
*   ModifySubPlotSize
***********************************************************************************************************************/
void CPlotWidgetGraphics::ModifySubPlotSize(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    /* screenPoint_p, defines which subPlot to increase
     * zDelta, if larger than 0 the subPlot will increase in size
     * not supported */
    Q_UNUSED(zDelta)
    Q_UNUSED(screenPoint_p)
    Q_UNUSED(invalidate_p)
}

/***********************************************************************************************************************
*   RestoreSubPlotWindows
***********************************************************************************************************************/
void CPlotWidgetGraphics::RestoreSubPlotWindows(void)
{
    m_restoreSubPlotSize = false;
}

/***********************************************************************************************************************
*   ZoomSubPlot_X_Axis
***********************************************************************************************************************/
void CPlotWidgetGraphics::ZoomSubPlot_X_Axis(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    Q_UNUSED(screenPoint_p)

    if (m_surfaces.isEmpty()) {
        return;
    }

    QMatrix matrix;

    if (zDelta < 0) {
        /* zoom out */
        matrix.translate(m_viewRect.width() * -0.2, 1.0);
        matrix.scale(0.8, 1.0);
    } else {
        matrix.translate(m_viewRect.width() * 0.2, 1.0);
        matrix.scale(1.2, 1.0);
    }

    m_surfaces.first()->view()->setMatrix(matrix, true);
    if (invalidate_p != nullptr) {
        *invalidate_p = true;
    }
}

/***********************************************************************************************************************
*   ZoomSubPlot_Y_Axis
***********************************************************************************************************************/
void CPlotWidgetGraphics::ZoomSubPlot_Y_Axis(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    Q_UNUSED(screenPoint_p)

    if (m_surfaces.isEmpty()) {
        return;
    }

    QMatrix matrix;

    if (zDelta < 0) {
        /* zoom out */
        matrix.translate(1.0, m_viewRect.height() * -0.2);
        matrix.scale(1.0, 0.8);
    } else {
        matrix.translate(1.0, m_viewRect.height() * 0.2);
        matrix.scale(1.0, 1.2);
    }

    m_surfaces.first()->view()->setMatrix(matrix, true);

    if (invalidate_p != nullptr) {
        *invalidate_p = true;
    }
}

/***********************************************************************************************************************
*   ZoomRestore
***********************************************************************************************************************/
void CPlotWidgetGraphics::ZoomRestore(void)
{
    if (m_surfaces.isEmpty()) {
        return;
    }

    for (auto& surface_p : m_surfaces) {
        surface_p->OptimumScale();
    }
}

/***********************************************************************************************************************
*   mousePressEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::mousePressEvent(QMouseEvent *event)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif

    QFrame::mousePressEvent(event);

    ScreenPoint_t screenPoint = MakeScreenPoint(event, 0, 0);
    UpdateFocus(&screenPoint);

    if (event->button() == Qt::RightButton) {
        OnContextMenu(screenPoint);
    }
    event->accept();
}

/***********************************************************************************************************************
*   mouseReleaseEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif
    QFrame::mouseReleaseEvent(event);
}

/***********************************************************************************************************************
*   mouseMoveEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::mouseMoveEvent(QMouseEvent *event)
{
    QFrame::mouseMoveEvent(event);
}

/***********************************************************************************************************************
*   FillScreenPoint_FromDCViewPortPoint
***********************************************************************************************************************/
void CPlotWidgetGraphics::FillScreenPoint_FromDCViewPortPoint(QPoint *viewPortPoint_p, ScreenPoint_t *screenPoint_p)
{
    screenPoint_p->DCBMP = *viewPortPoint_p;
    screenPoint_p->mouse.setX(viewPortPoint_p->x());
    screenPoint_p->mouse.setY(viewPortPoint_p->y());
    screenPoint_p->ClientToScreen = mapToGlobal(screenPoint_p->mouse);
}

/***********************************************************************************************************************
*   GetClosest_GO
***********************************************************************************************************************/
bool CPlotWidgetGraphics::GetClosest_GO(int row, GraphicalObject_t **go_pp, int *distance_p)
{
    Q_UNUSED(row)
    Q_UNUSED(go_pp)
    Q_UNUSED(distance_p)
    return false;
}

/***********************************************************************************************************************
*   SetFocusTime
***********************************************************************************************************************/
void CPlotWidgetGraphics::SetFocusTime(const double time)
{
    /* Not supported */
    Q_UNUSED(time)
}

/***********************************************************************************************************************
*   HandleKeyDown
***********************************************************************************************************************/
bool CPlotWidgetGraphics::HandleKeyDown(QKeyEvent *e)
{
    Q_UNUSED(e)

    bool CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    bool SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    bool ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    TRACEX_D(QString("%1 ctrl:%2 shift:%3 alt:%4").arg(__FUNCTION__).arg(CTRL_Pressed)
                 .arg(SHIFT_Pressed).arg(ALT_Pressed))

    QPoint point = mapFromGlobal(QCursor::pos());
    bool updateNeeded = false;
    bool handled = false;
    ScreenPoint_t screenPoint = MakeScreenPoint(this, point, 0, 0);

    if (m_keyPressedList.contains(Qt::Key_X)) {
        handled = true;
        updateNeeded = true;
        if (SHIFT_Pressed) {
            ZoomSubPlot_X_Axis(-1, &screenPoint, &updateNeeded);
        } else {
            ZoomSubPlot_X_Axis(1, &screenPoint, &updateNeeded);
        }
    } else if (m_keyPressedList.contains(Qt::Key_Y)) {
        handled = true;
        updateNeeded = true;

        if (SHIFT_Pressed) {
            ZoomSubPlot_Y_Axis(-1, &screenPoint, &updateNeeded);
        } else {
            ZoomSubPlot_Y_Axis(1, &screenPoint, &updateNeeded);
        }
    } else if (m_keyPressedList.contains(Qt::Key_S)) {
        handled = true;
        updateNeeded = true;

        if (SHIFT_Pressed) {
            ModifySubPlotSize(-1, &screenPoint, &updateNeeded);
        } else {
            ModifySubPlotSize(1, &screenPoint, &updateNeeded);
        }
    } else if (m_keyPressedList.contains(Qt::Key_F1)) {
        extern CPlugin_DLL_API *CWorkspace_SearchPlugin(CPlot *childPlot_p);
        CPlugin_DLL_API *plugin_p = CWorkspace_SearchPlugin(m_plot_p);

        if (plugin_p != nullptr) {
            DLL_API_PluginInfo_t *info_p = plugin_p->GetInfo();

            if ((info_p != nullptr) && (info_p->supportedFeatures & SUPPORTED_FEATURE_HELP_URL)) {
                MW_StartWebPage(QString(info_p->helpURL));
                handled = true;
            }
        }
    }

    /* */
    else if (m_keyPressedList.contains(Qt::Key_C) && CTRL_Pressed) {
        handled = true;
        SurfaceToClipBoard(&screenPoint);
    }

    if (updateNeeded) {
        update();
    }

    return handled;
}

/***********************************************************************************************************************
*   OnContextMenu
***********************************************************************************************************************/
void CPlotWidgetGraphics::OnContextMenu(ScreenPoint_t& screenPoint)
{
    QMenu contextMenu(this);
    auto count = GraphsObjectCount();

    {
        /* Run Plot */
        QAction *action_p;
        action_p = contextMenu.addAction(QString("Run Plot"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] () {CWorkspace_PlugInPlotRunSelected(m_plot_p);});
    }

    {
        /* Run All Plots */
        QAction *action_p;
        action_p = contextMenu.addAction(QString("Run All Plots (Ctrl-R)"));
        action_p->setEnabled(true);
        connect(action_p, &QAction::triggered, [ = ] () {CWorkspace_RunAllPlots();});
    }

    if (count > 0) {
        contextMenu.addSeparator();

#if 0
        {
            /* Copy to Clipboard */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Copy to clipboard (Ctrl-C)"));
            action_p->setEnabled(false); /* not supported */
            connect(action_p, &QAction::triggered, [ = ] () {SurfaceToClipBoard(&screenPoint);});
        }
#endif

        contextMenu.addSeparator();

        {
            /* Zoom Horiz In (press x) or (shift+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Zoom Horiz In (press x) or (Shift+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ZoomSubPlot_X_Axis(1, &screenPoint);});
        }

        {
            /* Zoom Horiz Out (shift+x) or (shift+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Zoom Horiz Out (Shift+x) or (Shift+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ZoomSubPlot_X_Axis(-1, &screenPoint);});
        }

        {
            /* Zoom Vert In (press y) or (ctrl+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Zoom Vert In (press y) or (Ctrl+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ZoomSubPlot_Y_Axis(1, &screenPoint);});
        }

        {
            /* Zoom Vert Out (y+shift) or (ctrl+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Zoom Vert Out (y+Shift) or (Ctrl+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ZoomSubPlot_Y_Axis(-1, &screenPoint);});
        }

        contextMenu.addSeparator();

        {
            /* Restore Zoom (1x) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Restore Zoom"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ZoomRestore();});
        }
    }

    contextMenu.exec(screenPoint.ClientToScreen);
}

/***********************************************************************************************************************
*   focusInEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
    m_toolTipState = ToolTipState_WaitForRequest;

    CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_PlotView_e);

    m_inFocus = true;
    setMouseTracking(true); /* to get mousemove event continously */

    MW_Refresh();

    if (CSCZ_ToolTipDebugEnabled) {
        TRACEX_D(QString("%1").arg(__FUNCTION__))
    }
    PRINT_FOCUS(__FUNCTION__)
}

/***********************************************************************************************************************
*   focusOutEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    m_toolTipTimer->stop();
    m_toolTipEnabled = false;
    if (m_toolTipState == ToolTipState_Running) {
        CloseToolTip();
    }
    m_toolTipState = ToolTipState_WaitForRequest;

    m_inFocus = false;
    setMouseTracking(false);

    CSCZ_UnsetLastViewSelectionKind(CSCZ_LastViewSelectionKind_PlotView_e);
    update();

    if (CSCZ_ToolTipDebugEnabled) {
        TRACEX_D(QString("%1").arg(__FUNCTION__))
    }
    PRINT_FOCUS(__FUNCTION__)
}

/***********************************************************************************************************************
*   onToolTipTimer
***********************************************************************************************************************/
void CPlotWidgetGraphics::onToolTipTimer(void)
{
    /* 1.) User is still for 200ms,
     * 2.) User is still still after 400ms -> show tool tip
     * 3.) Check if user still after 200ms
     *     3.a) If user not still after 200ms, start close timer of 200ms
     * 4.) At close timer, remove too_tip */
    bool still = false;
    QPoint screenCoordPoint = mapFromGlobal(QCursor::pos());

    if (CSCZ_ToolTipDebugEnabled) {
        TRACEX_I(QString("%1  (x%2,y%3)")
                     .arg(__FUNCTION__).arg(screenCoordPoint.x())
                     .arg(screenCoordPoint.y()))
    }

    if (!m_viewRect.contains(screenCoordPoint)) {
        if (CSCZ_ToolTipDebugEnabled) {
            TRACEX_I(QString("%1  Curser outside (x%2,y%3) "
                             "(l%4,t%5,r%6,b%7)")
                         .arg(__FUNCTION__).arg(screenCoordPoint.x())
                         .arg(screenCoordPoint.y()).arg(m_viewRect.left())
                         .arg(m_viewRect.top()).arg(m_viewRect.right())
                         .arg(m_viewRect.bottom()))
        }
        CloseToolTip();
        m_toolTipEnabled = false;
        return;
    }

    ScreenPoint_t screenPoint = MakeScreenPoint(this, screenCoordPoint, 0, 0);
    QRect lastPosBox(m_lastCursorPos.mouse.x() - 2, m_lastCursorPos.mouse.y() - 2, 4, 4);

    if (lastPosBox.contains(screenPoint.mouse)) {
        if (CSCZ_ToolTipDebugEnabled) {
            TRACEX_I(QString("%1 mouse:%2 %3 box: %4 %5 %6 %7 Still").arg(__FUNCTION__)
                         .arg(screenPoint.mouse.x()).arg(screenPoint.mouse.y())
                         .arg(lastPosBox.left()).arg(lastPosBox.top())
                         .arg(lastPosBox.right()).arg(lastPosBox.bottom()))
        }
        still = true;
    }

    switch (m_toolTipState)
    {
        case ToolTipState_Closing:
            CloseToolTip();
            m_toolTipState = ToolTipState_WaitForRequest;
            if (CSCZ_ToolTipDebugEnabled) {
                TRACEX_I(QString("%1  ToolTipState_Closing -> "
                                 "ToolTipState_WaitForRequest")
                             .arg(__FUNCTION__))
            }

            m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
            break;

        case ToolTipState_WaitForRequest:
            m_lastCursorPos = screenPoint;
            if (still) {
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(QString("%1  ToolTipState_WaitForRequest -> ToolTipState_Pending").arg(__FUNCTION__))
                }
                m_toolTipTimer->start(TO_TT_PENDING);
                m_toolTipState = ToolTipState_Pending;
            } else {
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(QString("%1  ToolTipState_WaitForRequest -> ToolTipState_WaitForRequest")
                                 .arg(__FUNCTION__))
                }
                m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
                m_toolTipState = ToolTipState_WaitForRequest;
            }
            break;

        case ToolTipState_Pending:
            m_lastCursorPos = screenPoint;
            if (still && OpenToolTip()) {
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(QString("%1  ToolTipState_Pending -> ToolTipState_Running").arg(__FUNCTION__))
                }
                m_toolTipTimer->start(TO_TT_RUNNING);
                m_toolTipState = ToolTipState_Running;
            } else {
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(
                        QString("%1  ToolTipState_Pending -> ToolTipState_WaitForRequest").arg(__FUNCTION__))
                }
                m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
                m_toolTipState = ToolTipState_WaitForRequest;
            }
            break;

        case ToolTipState_Running:
            if (still) {
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(QString("%1  ToolTipState_Running -> ToolTipState_Running").arg(__FUNCTION__))
                }
                m_toolTipTimer->start(TO_TT_RUNNING);
                m_toolTipState = ToolTipState_Running;
            } else {
                m_lastCursorPos = screenPoint;
                m_toolTipTimer->start(TO_TT_CLOSE);
                m_toolTipState = ToolTipState_Closing;
                if (CSCZ_ToolTipDebugEnabled) {
                    TRACEX_I(QString("%1  ToolTipState_Running -> ToolTipState_Closing").arg(__FUNCTION__))
                }
            }
            break;
    }
}

/***********************************************************************************************************************
*   DrawToolTip
***********************************************************************************************************************/
void CPlotWidgetGraphics::DrawToolTip(void)
{
    if (!m_toolTipStrings.isEmpty()) {
        CLogScrutinizerDoc *doc_p = GetTheDoc();

        doc_p->m_fontCtrl.SetFont(m_pDC, g_plotWnd_BlackFont_p);

        QSize size = doc_p->m_fontCtrl.GetFontSize();
        QPoint point(m_lastCursorPos.DCBMP.x() + 5, m_lastCursorPos.DCBMP.y() - 5);
        auto delta = size.height() * 1.2;
        auto y = static_cast<double>(point.y());

        for (auto& string : m_toolTipStrings) {
            m_pDC->drawText(point, string);
            y += delta;
            point.setY(static_cast<int>(y));
        }
    }
}

/***********************************************************************************************************************
*   OpenToolTip
***********************************************************************************************************************/
bool CPlotWidgetGraphics::OpenToolTip(void)
{
    CGraph *graph_p;
    GraphicalObject_t *go_p;
    CSubPlot *subPlot_p;

    if (GetClosestGraph(&m_lastCursorPos, &graph_p, &go_p, &subPlot_p)) {
        char *graphName_p = graph_p->GetName();
        char temp[CFG_TEMP_STRING_MAX_SIZE];
        char *title_p;
        char *y_axis_label_p;
        subPlot_p->GetTitle(&title_p, &y_axis_label_p);

        char *text_p;
        int textSize;
        CLogScrutinizerDoc *doc_p = GetTheDoc();
        doc_p->GetTextItem(go_p->row, &text_p, &textSize);

        if (!m_plot_p->vPlotGraphicalObjectFeedback(text_p, textSize, go_p->x2, go_p->row, graph_p, temp,
                                                    CFG_TEMP_STRING_MAX_SIZE)) {
            m_toolTipStrings.append(QString("%1 %2 - %3")
                                        .arg(static_cast<double>(go_p->y2)).arg(y_axis_label_p).arg(graphName_p));
        } else {
            int index = 0;    /* current parser index of the temp string (which may contain sub-strings) */
            int startIndex = 0;    /* where the current sub-string started */
            size_t maxLength = 0;    /* the max length of multiple sub-strings */
            size_t currentLength = 0;    /* length of the current sub-string */

            while ((index < CFG_TEMP_STRING_MAX_SIZE) && (temp[index] != 0)) {
                if (temp[index] == 0x0a) {
                    if ((index > 0) && (temp[index - 1] == 0x0d)) {
                        temp[index - 1] = 0; /* if CRLF in the string these are replaced with double-0 */
                    }
                    temp[index] = 0;
                    m_toolTipStrings.append(QString(&temp[startIndex]));

                    currentLength = strlen(&temp[startIndex]);
                    if (currentLength > maxLength) {
                        maxLength = currentLength;
                    }
                    startIndex = index + 1; /* start a new string */
                }

                ++index; /* next character */

                /* one line without CRLF, if temp[index - 1] == 0 then that string has already been added */
                if ((temp[index] == 0) && (temp[index - 1] != 0)) {
                    m_toolTipStrings.append(QString(&temp[startIndex]));

                    currentLength = strlen(&temp[startIndex]);
                    if (currentLength > maxLength) {
                        maxLength = currentLength;
                    }
                }
            }
        }
        for (auto& tip : m_toolTipStrings) {
            TRACEX_I(QString("OpenToolTip  %1").arg(tip))
        }
        update();
        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   CloseToolTip
***********************************************************************************************************************/
void CPlotWidgetGraphics::CloseToolTip(void)
{
    m_toolTipStrings.clear();
    if (CSCZ_ToolTipDebugEnabled) {
        TRACEX_D(QString("%1").arg(__FUNCTION__))
    }
    update();
}

/***********************************************************************************************************************
*   Initialize
***********************************************************************************************************************/
void CPlotWidgetGraphics::Initialize(void)
{
    m_restoreSubPlotSize = true;
    m_surfacesInitialized = false;

    Init();
    InitilizeSubPlots();
    update();
}

/***********************************************************************************************************************
*   closeEvent
***********************************************************************************************************************/
void CPlotWidgetGraphics::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_toolTipTimer->stop();
}

/***********************************************************************************************************************
*   Redraw
***********************************************************************************************************************/
void CPlotWidgetGraphics::Redraw(void)
{
    update();
}

/***********************************************************************************************************************
*   Redraw
* This function clears out the focus flag from all surfaces within the CPlotWidgetGraphics.
***********************************************************************************************************************/
void CPlotWidgetGraphics::ResetFocus(void)
{}

/***********************************************************************************************************************
*   UpdateFocus
***********************************************************************************************************************/
void CPlotWidgetGraphics::UpdateFocus(const ScreenPoint_t *screenPoint_p)
{
    /* Not supported */
    Q_UNUSED(screenPoint_p)
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CPlotWidgetGraphics::GetClosestGraph(ScreenPoint_t *screenPoint_p, CGraph **graph_pp, GraphicalObject_t **go_pp,
                                          CSubPlot **subPlot_pp)
{
    /* Not supported */
    Q_UNUSED(screenPoint_p)
    Q_UNUSED(graph_pp)
    Q_UNUSED(go_pp)
    Q_UNUSED(subPlot_pp)
    return false;
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CPlotWidgetGraphics::GetClosestGraph(int row,
                                          CGraph **graph_pp,
                                          GraphicalObject_t **go_pp,
                                          CSubPlot **subPlot_pp)
{
    /* Not supported */
    Q_UNUSED(row)
    Q_UNUSED(graph_pp)
    Q_UNUSED(go_pp)
    Q_UNUSED(subPlot_pp)
    return false;
}

/***********************************************************************************************************************
*   isSurfaceInList
***********************************************************************************************************************/
bool CPlotWidgetGraphics::isSurfaceInList(QList<CSubPlotSurface *> *list_p, CSubPlotSurface *surface_p)
{
    if (list_p->isEmpty()) {
        return false;
    }

    for (auto& surfaceMatch_p : *list_p) {
        if (surface_p->m_subPlot_p == surfaceMatch_p->m_subPlot_p) {
            return true;
        }
    }

    return false;
}

/***********************************************************************************************************************
*   SetSurfaceFocus
***********************************************************************************************************************/
void CPlotWidgetGraphics::SetSurfaceFocus(CSubPlot *subPlot_p)
{
    /* Not supported */
    Q_UNUSED(subPlot_p)
}

/***********************************************************************************************************************
*   SearchFocus
***********************************************************************************************************************/
bool CPlotWidgetGraphics::SearchFocus(CSubPlot **subPlot_pp, CSubPlotSurface **surface_pp)
{
    Q_UNUSED(subPlot_pp)
    Q_UNUSED(surface_pp)
    return false;
}

/***********************************************************************************************************************
*   SearchSubPlot
***********************************************************************************************************************/
bool CPlotWidgetGraphics::SearchSubPlot(CSubPlot *subPlot_p, CSubPlotSurface **surface_pp)
{
    Q_UNUSED(subPlot_p)
    Q_UNUSED(surface_pp)
    return false;
}

/***********************************************************************************************************************
*   Align_X_Zoom
***********************************************************************************************************************/
void CPlotWidgetGraphics::Align_X_Zoom(double x_min, double x_max)
{
    /* Not supported */
    Q_UNUSED(x_min)
    Q_UNUSED(x_max)
}

/***********************************************************************************************************************
*   Align_Reset_Zoom
***********************************************************************************************************************/
void CPlotWidgetGraphics::Align_Reset_Zoom(double x_min, double x_max)
{
    /* Not supported */
    Q_UNUSED(x_min)
    Q_UNUSED(x_max)
}

/***********************************************************************************************************************
*   Align_X_Cursor
***********************************************************************************************************************/
void CPlotWidgetGraphics::Align_X_Cursor(double cursorTime, double x_min, double x_max)
{
    /* Not supported */
    Q_UNUSED(cursorTime)
    Q_UNUSED(x_min)
    Q_UNUSED(x_max)
}

/***********************************************************************************************************************
*   ZoomSubPlotInFocus
***********************************************************************************************************************/
void CPlotWidgetGraphics::ZoomSubPlotInFocus(bool in, bool horizontal)
{
    /* Not supported */
    Q_UNUSED(in)
    Q_UNUSED(horizontal)
}

/***********************************************************************************************************************
*   ResizeSubPlotInFocus
***********************************************************************************************************************/
void CPlotWidgetGraphics::ResizeSubPlotInFocus(bool increase)
{
    /* Not supported */
    Q_UNUSED(increase)
}
