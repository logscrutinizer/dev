/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"
#include "cplotwidget.h"
#include "CProgressCtrl.h"
#include "CLogScrutinizerDoc.h"

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "mainwindow_cb_if.h"
#include "CWorkspace_cb_if.h"
#include "cplotpane_cb_if.h"
#include "utils.h"
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

extern QPen *g_plotWnd_defaultPen_p;
extern QPen *g_plotWnd_defaultDotPen_p;
extern FontItem_t *g_plotWnd_GrayFont_p;
extern FontItem_t *g_plotWnd_BlackFont_p;
extern FontItem_t *g_plotWnd_WhiteFont_p;
extern CSubPlotSurface *g_currentShadowCopy;  /* defined in CPlotPane */
extern CThreadManager *g_CPlotPane_ThreadMananger_p;

typedef struct {
    CSubPlotSurface *subplotSurface_p;
} OnPaint_Data_t;

static OnPaint_Data_t g_onPaint_WorkData[MAX_NUM_OF_THREADS];
void OnPaint_ThreadAction(volatile void *data_p);

CPlotWidgetInterface::~CPlotWidgetInterface() {} /* for vtable impl. */

/***********************************************************************************************************************
*   CZoomRect
***********************************************************************************************************************/
class CZoomRect
{
public:
    QRect rect;
    int height;
    CSubPlotSurface *surface_p;
    double share;
};

CPlotWidget::CPlotWidget(CPlot *plot_p)
{
    m_inDestructor = false;

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

    m_plot_p = plot_p;
    m_subPlot_Moving = false;

    m_inFocus = false;
    m_surfaceFocus_p = nullptr;
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CPlotWidget::Init(void)
{
    m_surfacesInitialized = false;
    m_surfacesCreated = false;

    m_toolTipEnabled = false;
    m_toolTipState = ToolTipState_WaitForRequest;

    m_min_X = 0.0;
    m_max_X = 0.0;
    m_zoom_left = 0.0;
    m_zoom_right = 0.0;

    m_vbmpOffset = 0;

    m_vrelPos = 0.0;
    m_hrelPos = 0.0;
    m_vscrollSliderGlue = false;
    m_vscrollSliderGlueOffset = 0;
    m_lmousebutton = false;
}

CPlotWidget::~CPlotWidget()
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
void CPlotWidget::RemoveSurfaces(void)
{
    extern void CPlotPane_RemoveRelatedShadowSubPlots(CPlotWidgetInterface * plotWndSrc_p);

    if (!m_inDestructor) {
        CPlotPane_RemoveRelatedShadowSubPlots(dynamic_cast<CPlotWidgetInterface *>(this));
    }

    while (!m_surfaces_Deactivated.isEmpty()) {
        auto item = m_surfaces_Deactivated.takeLast();
        if (nullptr != item) {
            delete item;
        }
    }

    /* Just pointers to the objects that are stored in either m_surfaces or m_surfaces_Deactivated, hence not
     * need delete the object */
    m_surfaceShadows.clear();

    while (!m_surfaces.isEmpty()) {
        auto surface_p = m_surfaces.takeLast();
        if (surface_p == g_currentShadowCopy) {
            g_currentShadowCopy = nullptr;
        }
        delete surface_p;
    }
}

/***********************************************************************************************************************
*   SetupWindowSizes
***********************************************************************************************************************/
bool CPlotWidget::SetupWindowSizes(void)
{
    auto rcClient = rect();

    if ((rcClient != m_windowRect) || m_restoreSubPlotSize) {
        PRINT_SIZE(QString("%1 rect old:%2,%3,%4,%5 new:%6,%7,%8,%9")
                       .arg(__FUNCTION__).arg(m_windowRect.left()).arg(m_windowRect.top()).arg(m_windowRect.right())
                       .arg(m_windowRect.bottom()).arg(rcClient.left()).arg(rcClient.top()).arg(rcClient.right())
                       .arg(rcClient.bottom()))

        m_windowRect = rcClient;
        m_plotWindowRect = m_windowRect;
        m_plotWindowRect.setRight(m_plotWindowRect.right() - V_SCROLLBAR_WIDTH);
        m_plotWindowRect.setBottom(m_plotWindowRect.bottom() - H_SCROLLBAR_HEIGHT);

        m_vscrollFrame = m_windowRect;
        m_vscrollFrame.setRight(rcClient.right());
        m_vscrollFrame.setLeft(rcClient.right() - V_SCROLLBAR_WIDTH);
        m_vscrollSlider = m_vscrollFrame; /* top / bottom is setup later */

        m_hscrollFrame = m_windowRect;
        m_hscrollFrame.setTop(rcClient.bottom() - H_SCROLLBAR_HEIGHT);
        m_hscrollFrame.setBottom(rcClient.bottom());
        m_hscrollSlider = m_hscrollFrame;

        PRINT_SIZE(QString("%1 plot:%10,%11,%12,%13")
                       .arg(__FUNCTION__).arg(m_plotWindowRect.left()).arg(m_plotWindowRect.top())
                       .arg(m_plotWindowRect.right()).arg(m_plotWindowRect.bottom()))

        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   event
***********************************************************************************************************************/
bool CPlotWidget::event(QEvent *event)
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
*   paintEvent
***********************************************************************************************************************/
void CPlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    LS_Painter originalPainter(this);
    static bool inPaintEvent = false;

    if (inPaintEvent) {
        return;
    }

    auto resetInPaintEvent = makeMyScopeGuard([&] () {inPaintEvent = false;});
    inPaintEvent = true;

    auto windowSizeSetup = SetupWindowSizes();
    SetupTotalDQRect();

    QImage double_buffer_image(QSize(m_totalDQRect.width(), m_totalDQRect.height()),
                               QImage::Format_ARGB32_Premultiplied);
    LS_Painter painter(&double_buffer_image);
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_pDC = &painter; /* Switch m_painter_p to double buffer */

    const QPen oldPen = m_pDC->pen();
    m_pDC->setPen(*m_bgBoarderPen_p);

    const QBrush oldBrush = m_pDC->brush();
    m_pDC->setBrush(*m_bgBoarderBrush_p);

    m_pDC->fillRect(m_windowRect, BOARDER_BACKGROUND_COLOR);

    if (!m_surfacesInitialized) {
        PRINT_SUBPLOTSURFACE("Empty window - not initialized")
        FillEmptyWindow();
        originalPainter.drawImage(
            m_windowRect.left(), m_windowRect.top(),                   /* Destination point */
            double_buffer_image,
            m_windowRect.left(), m_windowRect.top() + m_vbmpOffset,    /* Source point */
            m_windowRect.width(), m_windowRect.height());
        return;
    }

    if (windowSizeSetup) {
        if (m_restoreSubPlotSize) {
            RestoreSubPlotWindows();
        } else {
            RealignSubPlots();
        }
    } else if (m_restoreSubPlotSize) {
        RestoreSubPlotWindows();
    }

    PRINT_SUBPLOTSURFACE(QString("%1 surfaces:%2 wr(%3-%4) totDQ(%5-%6) pw(%7-%8)"
                                 "vscrf(%9-%10) vscrs(%11-%12) hscrf(%13-%14) hscrs(%15-%16)")
                             .arg(__FUNCTION__)
                             .arg(m_surfaces.count())
                             .arg(m_windowRect.width())
                             .arg(m_windowRect.height())
                             .arg(m_totalDQRect.width())
                             .arg(m_totalDQRect.height())
                             .arg(m_plotWindowRect.width())
                             .arg(m_plotWindowRect.height())
                             .arg(m_vscrollFrame.width())
                             .arg(m_vscrollFrame.height())
                             .arg(m_vscrollSlider.width())
                             .arg(m_vscrollSlider.height())
                             .arg(m_hscrollFrame.width())
                             .arg(m_hscrollFrame.height())
                             .arg(m_hscrollSlider.width())
                             .arg(m_hscrollSlider.height()))

    if ((m_surfaces.count() != 0) &&
        ((m_windowRect.width() > 10) &&
         (m_windowRect.height() > 10) &&
         (m_totalDQRect.width() > 10) &&
         (m_totalDQRect.height() > 10) &&
         (m_plotWindowRect.width() > 1) &&
         (m_plotWindowRect.height() > 1) &&
         (m_vscrollFrame.width() > 1) &&
         (m_vscrollFrame.height() > 1) &&
         (m_vscrollSlider.width() > 1) &&
         (m_vscrollSlider.height() > 1) &&
         (m_hscrollFrame.width() > 1) &&
         (m_hscrollFrame.height() > 1) &&
         (m_hscrollSlider.width() > 1) &&
         (m_hscrollSlider.height() > 1))) {
        DrawSubPlots();
        DrawScrollbar();
        DrawToolTip();
    } else {
        PRINT_SUBPLOTSURFACE("Empty window - bad parameters")
        FillEmptyWindow();
    }

    m_pDC->setPen(oldPen);
    m_pDC->setBrush(oldBrush);

    originalPainter.drawImage(
        m_windowRect.left(), m_windowRect.top(), /* Destination point */
        double_buffer_image,
        m_windowRect.left(), m_windowRect.top() + m_vbmpOffset, /* Source point */
        m_windowRect.width(), m_windowRect.height());

    painter.end();
}

/***********************************************************************************************************************
*   DrawScrollbar
***********************************************************************************************************************/
void CPlotWidget::DrawScrollbar(void)
{
    /* The scrollbar isn't draw through the entire DC bitmap, only covering what is shown. Hence we must align where
     * the slider and frame are drawn such that they are part of which parts that are bitblit from the DC bitmap
     * onto the screen. (m_vbmpOffset) */

    m_hscrollSliderActive = false;
    if (!m_surfaces.isEmpty()) {
        CSubPlotSurface *surface = nullptr;
        for (auto& temp_surface : m_surfaces) {
            if (temp_surface->isPaintable()) {
                surface = temp_surface;
                break;
            }
        }

        if (nullptr == surface) {
            return; /*abort */
        }

        GraphicalObject_Extents_t maxExtents;
        QRect viewPort;
        SurfaceZoom_t zoom;

        surface->GetSurfaceZoom(&zoom);     /* The current zoomed extents */
        surface->GetMaxExtents(&maxExtents);   /* The sub-plot max/min extents */
        surface->GetViewPortRect(&viewPort);   /* The viewPort in pixels (for the surface) */

        auto max_window = (maxExtents.x_max - maxExtents.x_min);
        auto zoomWindow = (zoom.x_max - zoom.x_min) / max_window;
        if (zoomWindow > 1.0) {
            zoomWindow = 1.0;
        }
        m_hscrollSliderActive = true;
        m_hscrollSlider_Width = static_cast<int>(m_hscrollFrame.width() * zoomWindow);
        m_hscrollSlider_Width = m_hscrollSlider_Width < 2 ? 2 : m_hscrollSlider_Width;
        m_hrelPos = (zoom.x_min - maxExtents.x_min) / max_window;
        PRINT_SUBPLOTSURFACE(QString("hscrollSlider:%1 zoomWindow:%2 m_hrelPos:%3 zmax:%4 zmin:%5 max:%6 min:%7")
                                 .arg(m_hscrollSlider_Width).arg(zoomWindow).arg(m_hrelPos)
                                 .arg(zoom.x_max).arg(zoom.x_min).arg(maxExtents.x_max).arg(maxExtents.x_min))
    }

    m_vscrollSlider_Height = (m_windowRect.height() * m_vscrollFrame.height()) / m_totalDQRect.height();

    m_vbmpOffset = static_cast<int>(m_vrelPos * (m_totalDQRect.height() - m_windowRect.height()));

    m_vscrollSlider = m_vscrollFrame;
    m_hscrollSlider = m_hscrollFrame;

    /* Draw scroll frames, the bounding box */
    m_pDC->fillRect(m_vscrollFrame.left(), m_vscrollFrame.top() + m_vbmpOffset, m_vscrollFrame.width(),
                    m_vscrollFrame.height(), SCROLLBAR_FRAME_COLOR);

    m_pDC->fillRect(m_hscrollFrame.left(), m_hscrollFrame.top() + m_vbmpOffset, m_hscrollFrame.width(),
                    m_hscrollFrame.height(), SCROLLBAR_FRAME_COLOR);

    m_vscrollSlider.setTop(static_cast<int>(m_vscrollFrame.top() +
                                            m_vrelPos * (m_vscrollFrame.height() - m_vscrollSlider_Height)));
    m_vscrollSlider.setBottom(m_vscrollSlider.top() + m_vscrollSlider_Height);

    /* Draw v scroll slider */
    auto color = SCROLLBAR_SLIDER_COLOR;
    if (m_vscrollSliderGlue) {
        color = SCROLLBAR_SLIDER_SELECTED_COLOR;
    }
    m_pDC->fillRect(m_vscrollSlider.left(),
                    m_vscrollSlider.top() + m_vbmpOffset,
                    m_vscrollSlider.width(),
                    m_vscrollSlider.height(),
                    color);

    /* Draw h scroll slider */
    color = SCROLLBAR_SLIDER_COLOR;
    if (m_hscrollSliderGlue) {
        color = SCROLLBAR_SLIDER_SELECTED_COLOR;
    }
    m_hscrollSlider.setLeft(static_cast<int>(m_hscrollFrame.left() +
                                             m_hrelPos * m_hscrollFrame.width()));
    m_hscrollSlider.setWidth(m_hscrollSlider_Width);

    /* Draw v scroll slider */
    m_pDC->fillRect(m_hscrollSlider.left(),
                    m_hscrollSlider.top() + m_vbmpOffset,
                    m_hscrollSlider.width(),
                    m_hscrollSlider.height(),
                    color);

    PRINT_SCROLL_INFO(QString("hscrollActive:%1 vscrollActive:%2").arg(m_hscrollSliderActive)
                          .arg(m_vscrollSliderActive))
}

/***********************************************************************************************************************
*   FillEmptyWindow
***********************************************************************************************************************/
void CPlotWidget::FillEmptyWindow(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    TRACEX_DE(QString("%1").arg(__FUNCTION__))

    int x_pos = static_cast<int>((m_windowRect.right() - m_windowRect.left()) * 0.1);
    int y_pos = static_cast<int>((m_windowRect.bottom() - m_windowRect.top()) * 0.1);

    doc_p->m_fontCtrl.SetFont(m_pDC, m_FontEmptyWindow_p);

    QSize fontSize = doc_p->m_fontCtrl.GetFontSize();
    m_pDC->drawText(x_pos, y_pos, "This is a Plot View");
    TRACEX_DE(QString("Draw text %1 %2").arg(x_pos).arg(y_pos))

    y_pos += fontSize.height() * 2;
    x_pos += fontSize.width();

    m_pDC->drawText(x_pos, y_pos, "The window is empty because you have loaded a plugin but you haven't run it's "
                                  "plot-function yet, or that");
    y_pos += static_cast<int>(fontSize.height() * 1.1);
    m_pDC->drawText(x_pos, y_pos, "the plugin plot-function didn't create any graphical objects for LogScrutinizer "
                                  "to present");

    y_pos += static_cast<int>(fontSize.height() * 1.5);
    x_pos += fontSize.width();

    char *title_p;
    char *x_axis_p;
    extern CPlugin_DLL_API *CWorkspace_SearchPlugin(CPlot *childPlot_p);
    CPlugin_DLL_API *pluginAPI_p = CWorkspace_SearchPlugin(m_plot_p);

    m_plot_p->GetTitle(&title_p, &x_axis_p);

    if (pluginAPI_p != nullptr) {
        DLL_API_PluginInfo_t *info_p = pluginAPI_p->GetInfo();
        m_pDC->drawText(x_pos, y_pos, QString("(1.) Right-click on Plugins->%1->Plots->%2").arg(info_p->name, title_p));
    } else {
        m_pDC->drawText(x_pos, y_pos, QString("(1.) Right-click on Plots->%1").arg(title_p));
    }

    y_pos += static_cast<int>(fontSize.height() * 1.1);

    m_pDC->drawText(x_pos, y_pos, "(2.) Select  Run");

    y_pos += static_cast<int>(fontSize.height() * 1.1);

    m_pDC->drawText(x_pos, y_pos, "(3.) LogScrutinizer will then activate the plugin to process the log, "
                                  "and present graphical output in this window");

    y_pos += static_cast<int>(fontSize.height() * 1.5);

    m_pDC->drawText(x_pos, y_pos, "Tip: You can also run all the loaded plugin's plot-function by pressing CTRL-R");

    y_pos += static_cast<int>(fontSize.height() * 2.0);

    m_pDC->drawText(x_pos, y_pos, "Read more about plugins at www.logscrutinizer.com. Press F1 for quick "
                                  "help web page");
}

/***********************************************************************************************************************
*   DrawSubPlots
***********************************************************************************************************************/
void CPlotWidget::DrawSubPlots(void)
{
    if (m_surfaces.count() == 0) {
        return;
    }

    /* Thread it */

    int threadCount = 1; /* g_CPlotPane_ThreadMananger_p->GetThreadCount(); Mulithreaded mess up in font somehow */
    int threadIndex = 0;
    QRect tempRect;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            surface_p->GetWindowRect(&tempRect);
            if (tempRect.height() >= MIN_WINDOW_HEIGHT / 2) {
                /* don't draw too small sub-plots */

                surface_p->CreatePainter(this); /* create DC in main thread */

                g_onPaint_WorkData[threadIndex].subplotSurface_p = surface_p;

                g_CPlotPane_ThreadMananger_p->ConfigureThread(threadIndex,
                                                              OnPaint_ThreadAction,
                                                              reinterpret_cast<void *>(&g_onPaint_WorkData[threadIndex]));

                /* OnPaint_ThreadAction((void*)&g_onPaint_WorkData[threadIndex]); */

                ++threadIndex;

                if (threadIndex % threadCount == 0) {
                    threadIndex = 0;
                    g_CPlotPane_ThreadMananger_p->StartConfiguredThreads();
                    g_CPlotPane_ThreadMananger_p->WaitForAllThreads();
                }
            }
        }

        /* Remainders */
        if (threadIndex > 0) {
            g_CPlotPane_ThreadMananger_p->StartConfiguredThreads();
            g_CPlotPane_ThreadMananger_p->WaitForAllThreads();
        }

        for (auto& surface_p : m_surfaces) {
            surface_p->GetWindowRect(&tempRect);
            if (tempRect.height() >= MIN_WINDOW_HEIGHT / 2) {
                surface_p->BitBlit(m_pDC);
                surface_p->DestroyPainter();
            }
        }
    }
}

/***********************************************************************************************************************
*   SurfaceToClipBoard
***********************************************************************************************************************/
void CPlotWidget::SurfaceToClipBoard(const ScreenPoint_t *screenPoint_p)
{
    if (m_surfaces.count() == 0) {
        return;
    }

    CSubPlotSurface *surface_p = GetSurfaceFromPoint(screenPoint_p);

    Q_ASSERT(surface_p != nullptr);
    if (surface_p == nullptr) {
        TRACEX_W(QString("%1 Failed to put surface on clipbard").arg(__FUNCTION__))
        return;
    }
    surface_p->CreatePainter(this); /* create DC in main thread */

    OnPaint_Data_t paintData;
    paintData.subplotSurface_p = surface_p;
    OnPaint_ThreadAction(&paintData);

    QGuiApplication::clipboard()->setImage(surface_p->getImage());

    surface_p->DestroyPainter();
}

/***********************************************************************************************************************
*   OnPaint_ThreadAction
***********************************************************************************************************************/
void OnPaint_ThreadAction(volatile void *data_p)
{
    auto *onPaint_data_p = reinterpret_cast<volatile OnPaint_Data_t *>(data_p);

    if (onPaint_data_p->subplotSurface_p->isPaintable()) {
        onPaint_data_p->subplotSurface_p->OnPaint_1();
        onPaint_data_p->subplotSurface_p->OnPaint_2();
    } else {
        onPaint_data_p->subplotSurface_p->OnPaint_Empty();
    }
}

/***********************************************************************************************************************
*   ResetZoom
***********************************************************************************************************************/
void CPlotWidget::ResetZoom(void)
{
#ifdef QT_TODO
    CList_LSZ *subPlots_p;

    m_plot_p->GetSubPlots(&subPlots_p);

    if (subPlots_p->isEmpty()) {
        return;
    }

    CSubPlotSurface *subPlot_p;
    POSITION pos = m_surfaces.GetHeadPosition();

    while (pos != nullptr) {
        subPlot_p = m_surfaces.GetNext(pos);
        subPlot_p->SetZoom(m_zoom_left, m_zoom_right, true);
    }
#endif
}

/***********************************************************************************************************************
*   InitilizeSubPlots
* Check the current size of the plot window
* Try to size all sub-plots such they fit, although none of the should be smaller than minimum
***********************************************************************************************************************/
void CPlotWidget::InitilizeSubPlots(void)
{
    CList_LSZ *subPlots_p;
    CSubPlot *subPlot_p;
    bool go_exists = false;

    if (m_surfacesInitialized) {
        return;
    }
    m_plot_p->GetSubPlots(&subPlots_p);

    if (subPlots_p->isEmpty()) {
        return;
    }

    subPlot_p = static_cast<CSubPlot *>(subPlots_p->first());
    while (subPlot_p != nullptr && !go_exists) {
        CList_LSZ *graphList_p;

        /*  if (subPlot_p->GetProperties() & SUB_PLOT_PROPERTY_SEQUENCE)
         *  {
         *    subPlot_p->GetSequenceDiagrams(&graphList_p);
         *  }
         *  else*/
        {
            subPlot_p->GetGraphs(&graphList_p);
        }

        if (!graphList_p->isEmpty()) {
            go_exists = true;
        } else {
            CDecorator *decorator_p;
            subPlot_p->GetDecorator(&decorator_p);
            if ((decorator_p != nullptr) && (decorator_p->GetNumOfObjects() != 0)) {
                go_exists = true;
            }
        }

        subPlot_p = static_cast<CSubPlot *>(subPlots_p->GetNext(static_cast<CListObject *>(subPlot_p)));
    }

    if (!go_exists) {
        return;
    }

    m_surfacesInitialized = true;

    QRect subPlotRect = m_plotWindowRect;
    int subPlotIndex = 0;
    int subPlotHeigth = m_plotWindowRect.height() / subPlots_p->count();
    GraphicalObject_Extents_t extents;

    subPlot_p = static_cast<CSubPlot *>(subPlots_p->first());

    int count = subPlots_p->count();

    subPlot_p->GetExtents(&extents);

    m_max_X = extents.x_max;
    m_min_X = extents.x_min;

    CSubPlotSurface *subPlotSurface_p;

    while (subPlot_p != nullptr) {
        --count;

        subPlotRect.setTop(subPlotIndex * subPlotHeigth);

        if (count > 0) {
            subPlotRect.setBottom((subPlotIndex + 1) * subPlotHeigth);
        } else {
            /* last in list */
            subPlotRect.setBottom(m_plotWindowRect.bottom());
        }

        subPlotSurface_p = new CSubPlotSurface(subPlot_p, m_plot_p);
        m_surfaces.append(subPlotSurface_p);
        subPlot_p->GetExtents(&extents);

        if (m_max_X < extents.x_max) {
            m_max_X = extents.x_max;
        }

        if (m_min_X > extents.x_min) {
            m_min_X = extents.x_min;
        }

        subPlot_p = static_cast<CSubPlot *>(subPlots_p->GetNext(static_cast<CListObject *>(subPlot_p)));
        ++subPlotIndex;
    }
    m_surfacesCreated = true;

    m_zoom_left = m_min_X;
    m_zoom_right = m_max_X;

    extern void CPlotPane_Align_Reset_Zoom(void);
    CPlotPane_Align_Reset_Zoom();
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CPlotWidget::keyPressEvent(QKeyEvent *e)
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
void CPlotWidget::keyReleaseEvent(QKeyEvent *e)
{
    int key = e->key();
    if (!m_keyPressedList.contains(key)) {
        return;
    }

    int index = m_keyPressedList.indexOf(key);
    if (index > -1) {
        m_keyPressedList.removeAt(index);
#ifdef _DEBUG
        TRACEX_DE(QString("%1 Key:%2 keyList:%3").arg(__FUNCTION__).arg(e->key()).arg(m_keyPressedList.count()))
#endif
    }
    QWidget::keyReleaseEvent(e);
}

/***********************************************************************************************************************
*   wheelEvent
***********************************************************************************************************************/
void CPlotWidget::wheelEvent(QWheelEvent *event)
{
    bool CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    bool SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    bool updateNeeded = false;
    ScreenPoint_t screenPoint = MakeScreenPoint(event, 0, m_vbmpOffset);
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

    if (m_surfaces.isEmpty()) {
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

        m_vrelPos += g_cfg_p->m_v_scrollGraphSpeed / (100.0 * (zDelta > 0 ? -1.0 : 1.0));

        if (m_vrelPos < 0.0) {
            m_vrelPos = 0.0;
        }
        if (m_vrelPos > 1.0) {
            m_vrelPos = 1.0;
        }

        PRINT_SCROLL_INFO(QString("%1 zDelta:%2 m_vrelPos:%3").arg(__FUNCTION__).arg(zDelta).arg(m_vrelPos))
        updateNeeded = true;
    }

    if (updateNeeded) {
        update();

        event->accept();
    }
}

/***********************************************************************************************************************
*   ModifySubPlotSize
* This function check the current size of the surfaces, if there is a missmatch in the total size of the window
* and the sum of the surfaces, then this difference is distributed equally to all surfaces. E.g. if the window
* is enlarged then all plots will be equally re-sized. If the window is smaller than the total size then no action
* besides aligning the subplots will be done
***********************************************************************************************************************/
void CPlotWidget::RealignSubPlots(void)
{
    if (m_surfaces.isEmpty()) {
        return;
    }

    PRINT_SIZE(QString("%1").arg(__FUNCTION__))

    /* First check if each surface needs some additional pixels to cover the entire window */
    int y_top = 0;
    int sum_Y_Size = 0;
    QRect rect;
#ifdef _DEBUG
    int count = 0;
#endif
    for (auto& surface_p : m_surfaces) {
        surface_p->GetWindowRect(&rect);
#ifdef _DEBUG
        PRINT_SIZE(QString("(%1,%2) (%3,%4) h:%5 count:%6").arg(rect.left()).arg(rect.top()).arg(rect.right())
                       .arg(rect.bottom()).arg(rect.height()).arg(count++))
#endif
        sum_Y_Size += rect.height();
    }

    int y_bias = (m_plotWindowRect.height() - sum_Y_Size) / m_surfaces.count();
    y_bias = y_bias > 0 ? y_bias : 0;

    PRINT_SIZE(QString("%1 y_bias:%2 sum_Y_size:%3 winRectHeight:%4")
                   .arg(__FUNCTION__).arg(y_bias).arg(sum_Y_Size).arg(m_windowRect.height()))

    QRect subPlotRect = m_plotWindowRect;
#ifdef _DEBUG
    count = 0;
#endif
    for (auto& surface_p : m_surfaces) {
        surface_p->GetWindowRect(&rect);

        int height = rect.height();
        subPlotRect.setTop(y_top);
        subPlotRect.setHeight(height + y_bias);
        surface_p->SurfaceReconfigure(&subPlotRect);
#ifdef _DEBUG
        surface_p->GetWindowRect(&rect);
        PRINT_SIZE(QString("U (%1,%2) (%3,%4) count:%5").arg(rect.left()).arg(rect.top()).arg(rect.right())
                       .arg(rect.bottom()).arg(count++))
#endif
        y_top = subPlotRect.bottom();
    } /* for */

    if (y_bias > 0) {
        /* Make the last surface stretch to the bottom in-case the surfaces didn't really fill the window */
        auto surface_p = m_surfaces.last();
        surface_p->GetWindowRect(&rect);
        rect.setBottom(m_plotWindowRect.bottom());
        surface_p->SurfaceReconfigure(&rect);
    }
}

/***********************************************************************************************************************
*   ModifySubPlotSize
***********************************************************************************************************************/
void CPlotWidget::ModifySubPlotSize(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    Q_UNUSED(invalidate_p)

    /* screenPoint_p, defines which subPlot to increase
     * zDelta, if larger than 0 the subPlot will increase in size
     * Example, say we have subplots 1,2,3. SP_1: 100y, SP_2 50y, SP_3 20y
     * Create a zoomRect list with the current dimensions of all active subplots */
    if (m_surfaces.isEmpty()) {
        return;
    }

    PRINT_SIZE(QString("%1").arg(__FUNCTION__))

    for (auto& surface_p : m_surfaces) {
        QRect rect;
        surface_p->GetWindowRect(&rect);

        if (rect.contains(screenPoint_p->DCBMP)) {
            if (zDelta > 0) {
                /* increase size of window */
                rect.adjust(0, 0, 0, static_cast<int>(rect.height() * ZOOM_STEP));
            } else {
                rect.adjust(0, 0, 0, -static_cast<int>(rect.height() * ZOOM_STEP));
            }

            if (rect.height() < MIN_WINDOW_HEIGHT) {
                rect.setBottom(rect.top() + MIN_WINDOW_HEIGHT);
            }
            if (rect.height() > MAX_WINDOW_HEIGHT) {
                rect.setBottom(rect.top() + MAX_WINDOW_HEIGHT);
            }

            surface_p->SurfaceReconfigure(&rect);
        }
    } /* while */

    RealignSubPlots();
}

/***********************************************************************************************************************
*   SetupTotalDQRect
***********************************************************************************************************************/
void CPlotWidget::SetupTotalDQRect(void)
{
    /* Loop through all subplots and sum their sizes */
    m_totalDQRect = QRect(0, 0, m_windowRect.width(), m_windowRect.height());

    if (m_surfaces.isEmpty()) {
        return;
    }

    int currentTotalHeight = 0;
    QRect rect;
    for (auto& surface_p : m_surfaces) {
        surface_p->GetWindowRect(&rect);
        currentTotalHeight += rect.height();
    }

    if (currentTotalHeight > m_windowRect.height()) {
        m_totalDQRect.setBottom(currentTotalHeight + H_SCROLLBAR_HEIGHT);
    }
}

/***********************************************************************************************************************
*   RestoreSubPlotWindows
***********************************************************************************************************************/
void CPlotWidget::RestoreSubPlotWindows(void)
{
    if (m_surfaces.isEmpty()) {
        return;
    }

    PRINT_SIZE(QString("%1").arg(__FUNCTION__))

    QRect surfaceRect = m_plotWindowRect;
    int subPlotIndex = 0;
    int subPlotHeight = m_plotWindowRect.height() / m_surfaces.count();

    if (subPlotHeight < DEFAULT_WINDOW_HEIGHT) {
        subPlotHeight = DEFAULT_WINDOW_HEIGHT;
    }

    int count = m_surfaces.count();
    for (auto& surface_p : m_surfaces) {
        surfaceRect.setTop(subPlotIndex * subPlotHeight);
        surfaceRect.setBottom((subPlotIndex + 1) * subPlotHeight);
        if (0 == --count) {
            /* This was the last plot, make sure that it fills down to the bottom */
            surfaceRect.setBottom(m_plotWindowRect.bottom());
        }
        surface_p->SurfaceReconfigure(&surfaceRect);
        ++subPlotIndex;
    }
    m_restoreSubPlotSize = false;
}

/***********************************************************************************************************************
*   ZoomSubPlot_X_Axis
***********************************************************************************************************************/
void CPlotWidget::ZoomSubPlot_X_Axis(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    QRect viewPort;

    if (m_surfaces.isEmpty()) {
        return;
    }

    CSubPlotSurface *subPlot_p = m_surfaces.first();

    subPlot_p->GetViewPortRect(&viewPort);

    double x_rel = (screenPoint_p->DCBMP.x() - viewPort.left()) / static_cast<double>(viewPort.width());
    double maxWidth = m_max_X - m_min_X;
    double currentZoom = maxWidth / (m_zoom_right - m_zoom_left);
    double newWidth = 0.0;

    m_offset_X = m_zoom_left + (m_zoom_right - m_zoom_left) * x_rel;

    TRACEX_D("CPlotWidget::ZoomSubPlot_X_Axis  pt.x:%d rel:%f curr offset:%e",
             screenPoint_p->DCBMP.x(), x_rel, m_offset_X)

    if (zDelta < 0) {
        /* zoom out */
        currentZoom *= 0.8;

        if (currentZoom < 1.0) {
            /*currentZoom = 1.0; */
        }

        newWidth = maxWidth / currentZoom;
    } else {
        currentZoom *= 1.2;

        newWidth = maxWidth / currentZoom;

        if (newWidth > maxWidth) {
            /*newWidth = maxWidth; */
        }
    }

    m_zoom_left = m_offset_X - (newWidth * x_rel);
    m_zoom_right = m_offset_X + (newWidth * (1.0 - x_rel));

    if (almost_equal(m_zoom_left, m_zoom_right)) {
        TRACEX_W("CPlotWidget::ZoomSubPlot_X_Axis  Zoom error")

        m_zoom_right = m_zoom_left + 1;
    }

#ifdef PREVENT_X_AXIS_OUTOF_MAXEXT
    m_zoom_left = m_zoom_left < m_min_X ? m_min_X : m_zoom_left;
    m_zoom_right = m_zoom_right > m_max_X ? m_max_X : m_zoom_right;
#endif

    if (invalidate_p != nullptr) {
        *invalidate_p = true;
    }

    TRACEX_D("CPlotWidget::ZoomSubPlot_X_Axis  zoom:%4.2 zoom_left:%f "
             "zoom_right:%f min:%e max:%e offset:%e",
             currentZoom, m_zoom_left, m_zoom_right, m_min_X, m_max_X, m_offset_X)

    extern void CPlotPane_Align_X_Zoom(double x_min, double x_max);

    CPlotPane_Align_X_Zoom(m_zoom_left, m_zoom_right);

    /*PropagateZoom(); */
}

/***********************************************************************************************************************
*   ZoomSubPlot_Y_Axis
***********************************************************************************************************************/
void CPlotWidget::ZoomSubPlot_Y_Axis(int zDelta, const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    CSubPlotSurface *subPlot_p = nullptr;

    /* Locate subplot where the zoom command was issued */

    subPlot_p = GetSurfaceFromPoint(screenPoint_p);

    if (subPlot_p == nullptr) {
        return;
    }

    GraphicalObject_Extents_t maxExtents;
    QRect viewPort;
    SurfaceZoom_t zoom;

    subPlot_p->GetSurfaceZoom(&zoom);     /* The current zoomed extents */
    subPlot_p->GetMaxExtents(&maxExtents);   /* The sub-plot max/min extents */
    subPlot_p->GetViewPortRect(&viewPort);   /* The viewPort in pixels (for the surface) */

    double y_rel = (screenPoint_p->DCBMP.y() - viewPort.top()) / static_cast<double>(viewPort.height());
    double maxHeight = maxExtents.y_max - maxExtents.y_min;
    double currentZoom = maxHeight / static_cast<double>(zoom.y_max - zoom.y_min);
    double newHeight = 0.0;

    /* since Y 0 is a the top, however the graph 0 is at the bottom, it is necessary to reverse the
     * y_offset */
    double y_offset = zoom.y_min + (zoom.y_max - zoom.y_min) * (1.0 - y_rel);

    TRACEX_D("CPlotWidget::ZoomSubPlot_Y_Axis  pt.y:%d rel:%f curr offset:%e",
             screenPoint_p->DCBMP.y(), y_rel, y_offset)

    if (zDelta < 0) {
        /* zoom out */
        currentZoom *= 0.9;
        newHeight = maxHeight / currentZoom;
    } else {
        currentZoom *= 1.1;
        newHeight = maxHeight / currentZoom;
    }

    zoom.y_min = y_offset - (newHeight * (1.0 - y_rel)); /* Maintain the center of zoom * */
    zoom.y_max = y_offset + (newHeight * y_rel);

#ifdef PREVENT_Y_AXIS_OUTOF_MAXEXT
    zoom.y_min = zoom.y_min < maxExtents.y_min ? maxExtents.y_min : zoom.y_min;
    zoom.y_max = zoom.y_max > maxExtents.y_max ? maxExtents.y_max : zoom.y_max;
#endif

    if (almost_equal(zoom.y_min, zoom.y_max)) {
        TRACEX_W("CPlotWidget::ZoomSubPlot_Y_Axis  Zoom error")
        zoom.y_max = zoom.y_min + 1;
    }

    if (invalidate_p != nullptr) {
        *invalidate_p = true;
    }

    TRACEX_D("CPlotWidget::ZoomSubPlot_Y_Axis  zoom:%e min:%e max:%e offset:%e",
             currentZoom, static_cast<double>(zoom.y_min), static_cast<double>(zoom.y_max), y_offset)

    subPlot_p->SetSurfaceZoom(&zoom);
}

/***********************************************************************************************************************
*   ZoomSubPlot_Move
***********************************************************************************************************************/
void CPlotWidget::ZoomSubPlot_Move(const ScreenPoint_t *screenPoint_p, bool *invalidate_p)
{
    QPoint diff = m_subPlot_StartMovePoint - screenPoint_p->mouse;
    double unitsPerPixel_X;
    double unitsPerPixel_Y;
    double unitsPerPixel_X_inv;
    double unitsPerPixel_Y_inv;

    /*ScreenToClient(&pt); */

    *invalidate_p = false;

    /* Locate subplot where the zoom command was issued */

    /* WindowToDCBMP */

    if (m_subPlot_inFocus_p == nullptr) {
        return;
    }

    m_subPlot_inFocus_p->GetUnitsPerPixel(&unitsPerPixel_X,
                                          &unitsPerPixel_Y,
                                          &unitsPerPixel_X_inv,
                                          &unitsPerPixel_Y_inv);

    GraphicalObject_Extents_t maxExtents;
    QRect viewPort;
    SurfaceZoom_t zoom;

    m_subPlot_inFocus_p->GetSurfaceZoom(&zoom);     /* The current zoomed extents */
    m_subPlot_inFocus_p->GetMaxExtents(&maxExtents);   /* The sub-plot max/min extents */
    m_subPlot_inFocus_p->GetViewPortRect(&viewPort);   /* The viewPort in pixels (for the surface) */

    double y_diff = -1.0 * (diff.y() * unitsPerPixel_Y);
    double x_diff = diff.x() * unitsPerPixel_X;

    m_subPlot_StartMovePoint = screenPoint_p->mouse;

    auto new_zoom = zoom;

    new_zoom.y_min += y_diff;
    new_zoom.y_max += y_diff;
    new_zoom.x_min += x_diff;
    new_zoom.x_max += x_diff;

#ifdef PREVENT_Y_AXIS_OUTOF_MAXEXT
    if ((new_zoom.y_min > maxExtents.y_min) && (new_zoom.y_max < maxExtents.y_max))
#endif
    {
        zoom.y_max = new_zoom.y_max;
        zoom.y_min = new_zoom.y_min;
    }

    zoom.x_max = new_zoom.x_max;
    zoom.x_min = new_zoom.x_min;

    *invalidate_p = true;

    m_subPlot_inFocus_p->SetSurfaceZoom(&zoom);

    /* If X axis is moved for one subplot, all shall be moved */
    if (!almost_equal(x_diff, 0.0)) {
        m_zoom_left = zoom.x_min;
        m_zoom_right = zoom.x_max;

        for (auto& surface_p : m_surfaces) {
            if (surface_p != m_subPlot_inFocus_p) {
                SurfaceZoom_t zoom_X;
                surface_p->GetSurfaceZoom(&zoom_X);
                zoom_X.x_max = zoom.x_max;
                zoom_X.x_min = zoom.x_min;
                surface_p->SetSurfaceZoom(&zoom_X);
            }
        }
    }

    extern void CPlotPane_Align_X_Zoom(double x_min, double x_max);
    CPlotPane_Align_X_Zoom(m_zoom_left, m_zoom_right);
}

/***********************************************************************************************************************
*   GetSurfaceFromPoint
***********************************************************************************************************************/
CSubPlotSurface *CPlotWidget::GetSurfaceFromPoint(const ScreenPoint_t *screenPoint_p)
{
    if (m_surfaces.isEmpty()) {
        return nullptr;
    }

    QRect rect;
#ifdef _DEBUG
    for (auto& surface_p : m_surfaces) {
        surface_p->GetWindowRect(&rect);

        TRACEX_D(QString("GetSubPLotFromPoint %1 x:%2 y:%3 w:%4 h:%5")
                     .arg(surface_p->m_subPlotTitle).arg(rect.x()).arg(rect.y())
                     .arg(rect.width()).arg(rect.height()))
    }
#endif

    for (auto& surface_p : m_surfaces) {
        surface_p->GetWindowRect(&rect);
        if (rect.contains(screenPoint_p->DCBMP)) {
            return surface_p;
        }
    }
    return nullptr;
}

/***********************************************************************************************************************
*   ZoomRestore
***********************************************************************************************************************/
void CPlotWidget::ZoomRestore(void)
{
    extern void CPlotPane_Align_Reset_Zoom(void);
    CPlotPane_Align_Reset_Zoom();
}

/***********************************************************************************************************************
*   mousePressEvent
***********************************************************************************************************************/
void CPlotWidget::mousePressEvent(QMouseEvent *event)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif

    if (!m_inFocus) {
        setFocus();
    }

    ScreenPoint_t screenPoint = MakeScreenPoint(event, 0, m_vbmpOffset);
    UpdateFocus(&screenPoint);

    if (event->button() == Qt::RightButton) {
        if (m_lmousebutton) {
            OnLButtonUp();
        }
        OnContextMenu(screenPoint);
    } else if (event->button() & Qt::LeftButton) {
        OnLButtonDown(screenPoint);
    }

    event->accept();
}

/***********************************************************************************************************************
*   mouseReleaseEvent
***********************************************************************************************************************/
void CPlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif

    if (event->button() & Qt::LeftButton) {
        OnLButtonUp();
    }
    event->accept();
}

/***********************************************************************************************************************
*   mouseMoveEvent
***********************************************************************************************************************/
void CPlotWidget::mouseMoveEvent(QMouseEvent *event)
{
    bool updateNeeded = false;

    if (!m_inFocus) {
        return;
    }

    ScreenPoint_t screenPoint = MakeScreenPoint(event, 0, m_vbmpOffset);

    if (!m_toolTipEnabled && !m_toolTipTimer->isActive()) {
        m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
        m_toolTipState = ToolTipState_WaitForRequest;
        TRACEX_I(QString("ToolTip timer started"))
    }

    if (m_lmousebutton && ((event->buttons() & Qt::LeftButton) == 0)) {
        /*A mouse up event was missed... restore */
        OnLButtonUp();
    }

    if (m_lmousebutton) {
        if (m_subPlot_Moving) {
            ZoomSubPlot_Move(&screenPoint, &updateNeeded);
        } else if (m_vscrollSliderGlue) {
            updateNeeded = true;
            m_vrelPos = (screenPoint.mouse.y() - m_vscrollSliderGlueOffset) /
                        static_cast<double>(m_windowRect.height() - m_vscrollSlider_Height);

            if (m_vrelPos < 0.0) {
                m_vrelPos = 0.0;
            } else if (m_vrelPos > 1.0) {
                m_vrelPos = 1.0;
            }
            PRINT_SCROLL_INFO("%s m_vrelPos: %f", __FUNCTION__, m_vrelPos)
        } else if (m_hscrollSliderGlue) {
            updateNeeded = true;
            m_hrelPos = (screenPoint.mouse.x() - m_hscrollSliderGlueOffset) / static_cast<double>(m_windowRect.width());

            if (m_hrelPos < 0.0) {
                m_hrelPos = 0.0;
            } else if (m_hrelPos > 1.0) {
                m_hrelPos = 1.0;
            }

            GraphicalObject_Extents_t maxExtents;
            SurfaceZoom_t zoom;
            auto surface = m_surfaces.first();
            if (nullptr == surface) {
                return;
            }

            surface->GetSurfaceZoom(&zoom);     /* The current zoomed extents */
            surface->GetMaxExtents(&maxExtents);   /* The sub-plot max/min extents */

            auto zoom_win = zoom.x_max - zoom.x_min;
            zoom.x_min = maxExtents.x_min + (maxExtents.x_max - maxExtents.x_min) * m_hrelPos;
            zoom.x_max = zoom.x_min + zoom_win;

            PRINT_SCROLL_INFO(QString("%1 zxmin:%2 zxmax:%3").arg(__FUNCTION__).arg(zoom.x_min).arg(zoom.x_max))

            if (zoom.x_min < maxExtents.x_min) {
                zoom.x_min = maxExtents.x_min;
                zoom.x_max = zoom.x_min + zoom_win;
            }
            if (zoom.x_max > maxExtents.x_max) {
                zoom.x_max = maxExtents.x_max;
                zoom.x_min = zoom.x_max - zoom_win;
            }

            extern void CPlotPane_Align_X_Zoom(double x_min, double x_max);
            CPlotPane_Align_X_Zoom(zoom.x_min, zoom.x_max);
            PRINT_SCROLL_INFO("%s m_vrelPos: %f", __FUNCTION__, m_hrelPos)
        }
    }

    if (updateNeeded) {
        update();
    }

    event->accept();
}

/***********************************************************************************************************************
*   OnLButtonDown
***********************************************************************************************************************/
void CPlotWidget::OnLButtonDown(ScreenPoint_t& screenPoint)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif

    m_lmousebutton = true;

    if (m_vscrollFrame.contains(screenPoint.mouse)) {
        m_vscrollSliderGlueOffset = screenPoint.mouse.y() - m_vscrollSlider.top();
        m_vscrollSliderGlue = true;
    } else if (m_hscrollFrame.contains(screenPoint.mouse)) {
        m_hscrollSliderGlueOffset = screenPoint.mouse.x() - m_hscrollSlider.left();
        m_hscrollSliderGlue = true;
    } else if (m_windowRect.contains(screenPoint.mouse)) {
        m_subPlot_Moving = true;
        m_subPlot_StartMovePoint = screenPoint.mouse;
        m_subPlot_inFocus_p = GetSurfaceFromPoint(&screenPoint);
    }

    setCursor(Qt::DragMoveCursor);
    g_processingCtrl_p->Processing_StartReport();
}

/***********************************************************************************************************************
*   OnLButtonUp
***********************************************************************************************************************/
void CPlotWidget::OnLButtonUp(void)
{
#ifdef _DEBUG
    TRACEX_D("%s", __FUNCTION__)
#endif

    auto redraw = false;

    g_processingCtrl_p->Processing_StopReport();

    m_subPlot_Moving = false;
    m_subPlot_inFocus_p = nullptr;

    m_lmousebutton = false;

    if (m_vscrollSliderGlue) {
        TRACEX_DE("%s   m_vscrollSliderGlue OFF", __FUNCTION__)
        m_vscrollSliderGlue = false;
        redraw = true;
    }

    if (m_hscrollSliderGlue) {
        TRACEX_DE("%s   m_hscrollSliderGlue OFF", __FUNCTION__)
        m_hscrollSliderGlue = false;
        redraw = true;
    }

    unsetCursor();

    if (redraw) {
        update();
    }

#ifdef QT_TODO
    ReleaseCapture(); /* stop receiving windows mouse event outside window */
#endif
}

/***********************************************************************************************************************
*   FillScreenPoint_FromDCViewPortPoint
***********************************************************************************************************************/
void CPlotWidget::FillScreenPoint_FromDCViewPortPoint(QPoint *viewPortPoint_p, ScreenPoint_t *screenPoint_p)
{
    screenPoint_p->DCBMP = *viewPortPoint_p;
    screenPoint_p->mouse.setX(viewPortPoint_p->x());
    screenPoint_p->mouse.setY(viewPortPoint_p->y() - m_vbmpOffset);
    screenPoint_p->ClientToScreen = mapToGlobal(screenPoint_p->mouse);
}

/***********************************************************************************************************************
*   GetClosest_GO
***********************************************************************************************************************/
bool CPlotWidget::GetClosest_GO(int row, GraphicalObject_t **go_pp, int *distance_p)
{
    int row_Best = -1;
    int distance = 0;
    int distance_Best = 0;
    CSubPlotSurface *CSubPlot_Best_p = nullptr;
    CGraph_Internal *graph_p = nullptr;
    GraphicalObject_t *go_p = nullptr;
    GraphicalObject_t *goBest_p = nullptr;

    TRACEX_D("CPlotWidget::GetClosest_GO")

    * distance_p = 0.0;
    *go_pp = nullptr;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->GetClosestGraph(row, &graph_p, &distance, &go_p)) {
                if ((CSubPlot_Best_p == nullptr) || (distance < distance_Best)) {
                    CSubPlot_Best_p = surface_p;
                    distance_Best = distance;
                    goBest_p = go_p;
                    row_Best = go_p->row;
                }
            }
        }
    }

    if (row_Best == -1) {
        return false;
    }

    *distance_p = distance_Best;
    *go_pp = goBest_p;

    return true;
}

/***********************************************************************************************************************
*   SetFocusTime
***********************************************************************************************************************/
void CPlotWidget::SetFocusTime(const double time)
{
    /* Define where the current x_min and x_max should be moved to.
     * Based on the zoom, and GO
     * Move either x_min or x_max (not both) to make sure the GO fits in the current zoom. None if not necessary
     * But both ends should be visible */
    double zoom_x1;
    double zoom_x2;
    double zoom_width;
    SurfaceZoom_t zoom;

    if (m_surfaces.isEmpty()) {
        return;
    }

    CSubPlotSurface *subPlot_p = m_surfaces.first();

    if (subPlot_p == nullptr) {
        return;
    }

    subPlot_p->GetSurfaceZoom(&zoom);

    zoom_x1 = zoom.x_min; /*x_max and x_min are the current zoom coordinates (not the max extents) */
    zoom_x2 = zoom.x_max;

    zoom_width = zoom_x2 - zoom_x1;

    if ((time - (zoom_width * 0.1)) < zoom_x1) {
        /* decrease with 10%, to be sure it is not in the outscirts... */

        /* Shift the left zoom point such that with the current zoom x1 is 10% within from the left
         * This means also that the zoom is a bit streched out as well. */
        zoom_x1 = time - (zoom_width * 0.1);
    }

    if ((time + (zoom_width * 0.1)) > zoom_x2) {
        /* Shift the right zoom point such that with the current zoom x1 is 10% within from the left
         * This means also that the zoom is a bit streched out as well. */
        zoom_x2 = time + (zoom_width * 0.1);
    }

    extern void CPlotPane_Align_X_Cursor(double cursorTime, double x_min, double x_max);

    CPlotPane_Align_X_Cursor(time, zoom_x1, zoom_x2);

    update();
}

/***********************************************************************************************************************
*   SetRow
***********************************************************************************************************************/
void CPlotWidget::SetRow(const ScreenPoint_t *screenPoint_p)
{
    /* This function is called from the popup menu, with the point of the mouse cursor. Hence the PlotWnd need to
     * find the subplot where the press was, and then find the closest graph and graphical object.
     *
     * When the go has been found, then log row is determined. The log window is modified with a call to
     * SetFocusRow(...), this is a function in the log window. That will change the row which is currently in focus. */
    int row;
    int row_Best = -1;
    double distance;
    double distance_Best = 0.0;
    double time = 0.0;
    CSubPlotSurface *CSubPlot_Best_p = nullptr;

    TRACEX_D("CPlotWidget::SetRow")

    setFocus();

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->GetCursorRow(&screenPoint_p->DCBMP, &row, &time, &distance) != nullptr) {
                if ((CSubPlot_Best_p == nullptr) || (distance < distance_Best)) {
                    CSubPlot_Best_p = surface_p;
                    row_Best = row;
                    distance_Best = distance;
                }
            }
        }

        if (row_Best == -1) {
            TRACEX_W("Warning: Couldn't set log row from plot, no matching graphical element")
            MW_PlaySystemSound(SYSTEM_SOUND_QUESTION);
            QMessageBox::warning(this,
                                 tr("Cursor not exact"),
                                 tr("Couldn't find a proper row in the plot to set the cursor"));
            return;
        }

        SetFocusTime(time);

        extern void CEditorWidget_SetFocusRow(int row);
        CEditorWidget_SetFocusRow(row_Best);
    }
    update();
}

/***********************************************************************************************************************
*   HandleKeyDown
***********************************************************************************************************************/
bool CPlotWidget::HandleKeyDown(QKeyEvent *e)
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
    ScreenPoint_t screenPoint = MakeScreenPoint(this, point, 0, m_vbmpOffset);

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
*   GraphsObjectCount
***********************************************************************************************************************/
int CPlotWidget::GraphsObjectCount(void)
{
    int count = 0;
    CList_LSZ *subplotList_p;
    m_plot_p->GetSubPlots(&subplotList_p);

    if (subplotList_p->count() == 0) {
        return 0;
    }

    auto subPlot_p = reinterpret_cast<CSubPlot *>(subplotList_p->first());

    while (subPlot_p != nullptr) {
        CList_LSZ *graphList_p;
        subPlot_p->GetGraphs(&graphList_p);

        if (graphList_p->count() > 0) {
            CGraph_Internal *graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->first());
            while (graph_p != nullptr) {
                count += graph_p->GetNumOfObjects();
                graph_p =
                    reinterpret_cast<CGraph_Internal *>(graphList_p->GetNext(reinterpret_cast<CListObject *>(graph_p)));
            }
        }

        subPlot_p = reinterpret_cast<CSubPlot *>(subplotList_p->GetNext(reinterpret_cast<CListObject *>(subPlot_p)));
    }
    return count;
}

/***********************************************************************************************************************
*   OnContextMenu
***********************************************************************************************************************/
void CPlotWidget::OnContextMenu(ScreenPoint_t& screenPoint)
{
    QMenu contextMenu(this);

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

    contextMenu.addSeparator();

    if (GraphsObjectCount() > 0) {
        {
            /* Copy to Clipboard */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Copy to clipboard (Ctrl-C)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {SurfaceToClipBoard(&screenPoint);});
        }

        contextMenu.addSeparator();

        {
            /* Set Plot Cursor */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Set Plot Cursor"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {SetRow(&screenPoint);});
        }

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

        contextMenu.addSeparator();

        {
            /* Enlarge Subplot (press s) or (shift+ctrl+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Enlarge Subplot (press s) or (Shift+Ctrl+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ModifySubPlotSize(1, &screenPoint);});
        }

        {
            /* Reduce Subplot (shift+s) or (shift+ctrl+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Reduce Subplot (Shift+s) or (Shift+Ctrl+wheel)"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {ModifySubPlotSize(-1, &screenPoint);});
        }

        {
            /* Zoom Vert In (press y) or (ctrl+wheel) */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Restore Subplot Sizes"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {RestoreSubPlotSizes();});
        }

        contextMenu.addSeparator();

        {
            /* Hide Subplot */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Hide Subplot"));
            action_p->setEnabled(true);
            connect(action_p, &QAction::triggered, [ = ] () {DeactivateSubPlot(&screenPoint); RealignSubPlots();});
        }

        {
            /* Show all Hidden Subplots */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Show all Hidden Subplots"));
            action_p->setEnabled(m_surfaces_Deactivated.isEmpty() ? false : true);
            connect(action_p, &QAction::triggered, [ = ] () {ActivateAllSubPlots();});
        }

        contextMenu.addSeparator();

        {
            /* Create Subplot Duplicate */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Create Subplot Duplicate"));
            action_p->setEnabled(GetSurfaceFromPoint(&screenPoint) != nullptr ? true : false);
            connect(action_p, &QAction::triggered, [ = ] () {CreateShadow(&screenPoint);});
        }

        {
            /* Insert Subplot Duplicate */
            QAction *action_p;
            action_p = contextMenu.addAction(QString("Insert Subplot Duplicate"));

            bool enabled = false;

            if ((g_currentShadowCopy != nullptr) &&
                (GetSurfaceFromPoint(&screenPoint) != nullptr) &&
                !isSurfaceInList(&m_surfaces, g_currentShadowCopy) &&
                !isSurfaceInList(&m_surfaces_Deactivated, g_currentShadowCopy) &&
                !isSurfaceInList(&m_surfaceShadows, g_currentShadowCopy)) {
                enabled = true;
            }
            action_p->setEnabled(enabled);
            connect(action_p, &QAction::triggered, [ = ] () {InsertShadow(&screenPoint);});
        }
    }

    contextMenu.exec(screenPoint.ClientToScreen);
}

/***********************************************************************************************************************
*   focusInEvent
***********************************************************************************************************************/
void CPlotWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    m_toolTipTimer->start(TO_TT_WAIT_FOR_TOOL_TIP_REQUEST);
    m_toolTipState = ToolTipState_WaitForRequest;

    CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_PlotView_e);

    m_inFocus = true;
    m_surfaceFocus_p = nullptr;
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
void CPlotWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)

    m_toolTipTimer->stop();
    m_toolTipEnabled = false;
    if (m_toolTipState == ToolTipState_Running) {
        CloseToolTip();
    }
    m_toolTipState = ToolTipState_WaitForRequest;

    m_inFocus = false;
    m_surfaceFocus_p = nullptr;
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
void CPlotWidget::onToolTipTimer(void)
{
    /* 1.) User is still for 200ms,
     * 2.) User is still still after 400ms -> show tool tip
     * 3.) Check if user still after 200ms
     *     3.a) If user not still after 200ms, start close timer of 200ms
     * 4.) At close timer, remove too_tip */
    bool still = false;
    QPoint screenCoordPoint = mapFromGlobal(QCursor::pos());

    if (CSCZ_ToolTipDebugEnabled) {
        TRACEX_I(QString("%1  (x%2,y%3)").arg(__FUNCTION__).arg(screenCoordPoint.x()).arg(screenCoordPoint.y()))
    }

    if (!m_windowRect.contains(screenCoordPoint)) {
        if (CSCZ_ToolTipDebugEnabled) {
            TRACEX_I(QString("%1  Curser outside (x%2,y%3) "
                             "(l%4,t%5,r%6,b%7)")
                         .arg(__FUNCTION__).arg(screenCoordPoint.x())
                         .arg(screenCoordPoint.y()).arg(m_windowRect.left())
                         .arg(m_windowRect.top()).arg(m_windowRect.right())
                         .arg(m_windowRect.bottom()))
        }
        CloseToolTip();
        m_toolTipEnabled = false;
        return;
    }

    ScreenPoint_t screenPoint = MakeScreenPoint(this, screenCoordPoint, 0, m_vbmpOffset);
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
void CPlotWidget::DrawToolTip(void)
{
    if (!m_toolTipStrings.isEmpty()) {
        CLogScrutinizerDoc *doc_p = GetTheDoc();

        doc_p->m_fontCtrl.SetFont(m_pDC, g_plotWnd_BlackFont_p);

        QSize size = doc_p->m_fontCtrl.GetFontSize();
        QPoint point(m_lastCursorPos.DCBMP.x() + 5, m_lastCursorPos.DCBMP.y() - 5);
        double delta = size.height() * 1.2;
        double y = static_cast<double>(point.y());

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
bool CPlotWidget::OpenToolTip(void)
{
    CGraph_Internal *graph_p;
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
            m_toolTipStrings.append(QString("%1 %2 - %3").arg(static_cast<double>(go_p->y2)).arg(y_axis_label_p).arg(
                                        graphName_p));
        } else {
            int index = 0;    /* current parser index of the temp string (which may contain sub-strings) */
            int startIndex = 0;    /* where the current sub-string started */
            int maxLength = 0;    /* the max length of multiple sub-strings */
            int currentLength = 0;    /* length of the current sub-string */

            while ((index < CFG_TEMP_STRING_MAX_SIZE) && (temp[index] != 0)) {
                if (temp[index] == 0x0a) {
                    if ((index > 0) && (temp[index - 1] == 0x0d)) {
                        temp[index - 1] = 0; /* if CRLF in the string these are replaced with double-0 */
                    }
                    temp[index] = 0;
                    m_toolTipStrings.append(QString(&temp[startIndex]));

                    currentLength = static_cast<int>(strlen(&temp[startIndex]));
                    if (currentLength > maxLength) {
                        maxLength = currentLength;
                    }
                    startIndex = index + 1; /* start a new string */
                }

                ++index; /* next character */

                /* one line without CRLF, if temp[index - 1] == 0 then that string has already been added */
                if ((temp[index] == 0) && (temp[index - 1] != 0)) {
                    m_toolTipStrings.append(QString(&temp[startIndex]));

                    currentLength = static_cast<int>(strlen(&temp[startIndex]));
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
void CPlotWidget::CloseToolTip(void)
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
void CPlotWidget::Initialize(void)
{
    Init();

    m_restoreSubPlotSize = true;
    m_surfacesInitialized = false;

    InitilizeSubPlots();
    update();
}

/***********************************************************************************************************************
*   closeEvent
***********************************************************************************************************************/
void CPlotWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_toolTipTimer->stop();
}

/***********************************************************************************************************************
*   Redraw
***********************************************************************************************************************/
void CPlotWidget::Redraw(void)
{
    if (m_surfaces.isEmpty()) {
        return;
    }

    for (auto& surface_p : m_surfaces) {
        surface_p->ForceRedraw();
    }

    update();
}

/***********************************************************************************************************************
*   ResetFocus
***********************************************************************************************************************/
void CPlotWidget::ResetFocus(void)
{
    /* This function clears out the focus flag from all surfaces within the CPlotWidget.
     * The Active subPlots */
    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            surface_p->SetFocus(false);
        }
    }

    /* The Deactivated subPlots */
    if (!m_surfaces_Deactivated.isEmpty()) {
        for (auto& surface_p : m_surfaces_Deactivated) {
            surface_p->SetFocus(false);
        }
    }

    /* The Shadow subPlots */
    if (!m_surfaceShadows.isEmpty()) {
        for (auto& surface_p : m_surfaceShadows) {
            surface_p->SetFocus(false);
        }
    }

    m_surfaceFocus_p = nullptr;
}

/***********************************************************************************************************************
*   UpdateFocus
***********************************************************************************************************************/
void CPlotWidget::UpdateFocus(const ScreenPoint_t *screenPoint_p)
{
    CSubPlotSurface *surfaceFocus_p = nullptr;

    if (m_surfaces.isEmpty() || !m_inFocus) {
        return;
    }

    surfaceFocus_p = GetSurfaceFromPoint(screenPoint_p);

    if ((surfaceFocus_p != nullptr) && (m_surfaceFocus_p != surfaceFocus_p)) {
        extern void CPlotPane_ResetPlotFocus(void);
        CPlotPane_ResetPlotFocus();
        SetSurfaceFocus(surfaceFocus_p->m_subPlot_p);
        update();
    }
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CPlotWidget::GetClosestGraph(ScreenPoint_t *screenPoint_p, CGraph_Internal **graph_pp, GraphicalObject_t **go_pp,
                                  CSubPlot **subPlot_pp)
{
    double distance = 0.0;
    double distance_Best = 0.0;
    CSubPlotSurface *CSubPlot_Best_p = nullptr;
    CGraph_Internal *graph_p = nullptr;
    CGraph_Internal *graph_Best_p = nullptr;
    GraphicalObject_t *go_p = nullptr;
    GraphicalObject_t *go_Best_p = nullptr;
    CSubPlot *subPlot_Best_p = nullptr;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->GetClosestGraph(&screenPoint_p->DCBMP, &graph_p, &distance, &go_p)) {
#ifdef _DEBUG
                TRACEX_DE("CPlotWidget::GetClosestGraph  %s %f", graph_p->GetName(), distance)
#endif
                if ((CSubPlot_Best_p == nullptr) || (distance < distance_Best)) {
                    CSubPlot_Best_p = surface_p;
                    distance_Best = distance;
                    graph_Best_p = graph_p;
                    go_Best_p = go_p;
                    subPlot_Best_p = surface_p->GetSubPlot();
                }
            }
        }
    }

#ifdef _DEBUG
    TRACEX_DE("CPlotWidget::GetClosestGraph distance:%f", distance)
#endif

    if ((graph_Best_p != nullptr) && (distance < 10.0)) {
        *graph_pp = graph_Best_p;
        *go_pp = go_Best_p;
        *subPlot_pp = subPlot_Best_p;
        return true;
    } else {
        *graph_pp = nullptr;
        *go_pp = nullptr;
        *subPlot_pp = nullptr;
        return false;
    }
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CPlotWidget::GetClosestGraph(int row, CGraph_Internal **graph_pp, GraphicalObject_t **go_pp, CSubPlot **subPlot_pp)
{
    int distance = 0;
    int distance_Best = 0;
    CSubPlotSurface *CSubPlot_Best_p = nullptr;
    CGraph_Internal *graph_p = nullptr;
    CGraph_Internal *graph_Best_p = nullptr;
    GraphicalObject_t *go_p = nullptr;
    GraphicalObject_t *go_Best_p = nullptr;
    CSubPlot *subPlot_Best_p = nullptr;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if ((surface_p != nullptr) && surface_p->GetClosestGraph(row, &graph_p, &distance, &go_p)) {
#ifdef _DEBUG
                TRACEX_DE("CPlotWidget::GetClosestGraph  %s %f", graph_p->GetName(), distance)
#endif
                if ((CSubPlot_Best_p == nullptr) || (distance < distance_Best)) {
                    CSubPlot_Best_p = surface_p;
                    distance_Best = distance;
                    graph_Best_p = graph_p;
                    go_Best_p = go_p;
                    subPlot_Best_p = surface_p->GetSubPlot();
                }
            }
        }
    }

    if ((graph_Best_p != nullptr) && (distance < 10)) {
        *graph_pp = graph_Best_p;
        *go_pp = go_Best_p;
        *subPlot_pp = subPlot_Best_p;
        return true;
    } else {
        *graph_pp = nullptr;
        *go_pp = nullptr;
        *subPlot_pp = nullptr;
        return false;
    }
}

/***********************************************************************************************************************
*   isSurfaceInList
***********************************************************************************************************************/
bool CPlotWidget::isSurfaceInList(QList<CSubPlotSurface *> *list_p, CSubPlotSurface *surface_p)
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
void CPlotWidget::SetSurfaceFocus(CSubPlot *subPlot_p)
{
    CSubPlotSurface *surfaceFocus_p = nullptr;

    if (m_surfaces.isEmpty()) {
        return;
    } else if (subPlot_p == nullptr) {
        surfaceFocus_p = m_surfaces.first();
    } else {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->m_subPlot_p == subPlot_p) {
                surfaceFocus_p = surface_p;
                break;
            }
        }
    }

    if (surfaceFocus_p == nullptr) {
        return;
    }

    surfaceFocus_p->SetFocus(true);

    m_surfaceFocus_p = surfaceFocus_p;

    QRect surfaceRect;

    surfaceFocus_p->GetWindowRect(&surfaceRect);  /*m_DC_windowRect */

    /* Modify the m_vbmpOffset such that the surface inFocus will be visible */

    if (((m_vbmpOffset + m_windowRect.bottom()) < (surfaceRect.top() + 30)) ||    /* +10 to add some margin... */
        (surfaceRect.bottom() < (m_vbmpOffset + 30))) {
        if ((m_vbmpOffset + m_windowRect.bottom()) < (surfaceRect.top() + 30)) {
            m_vbmpOffset = surfaceRect.top() + (surfaceRect.height() / 2);
            m_vrelPos = (m_vbmpOffset + m_windowRect.height()) / m_totalDQRect.height();
        } else if (surfaceRect.bottom() < (m_vbmpOffset) + 30) {
            m_vbmpOffset = surfaceRect.top();
            m_vrelPos = (m_vbmpOffset + m_windowRect.height()) / m_totalDQRect.height();
        }

        m_vrelPos = m_vrelPos > 1.0 ? 1.0 : m_vrelPos;
        m_vrelPos = m_vrelPos < 0.0 ? 0.0 : m_vrelPos;
    }

    setFocus();
    update();
}

/***********************************************************************************************************************
*   SearchFocus
***********************************************************************************************************************/
bool CPlotWidget::SearchFocus(CSubPlot **subPlot_pp, CSubPlotSurface **surface_pp)
{
    *subPlot_pp = nullptr;
    *surface_pp = nullptr;

    if (m_surfaces.isEmpty()) {
        return false;
    }

    for (auto& surface_p : m_surfaces) {
        if (surface_p->GetFocus()) {
            *surface_pp = surface_p;
            return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   SearchSubPlot
***********************************************************************************************************************/
bool CPlotWidget::SearchSubPlot(CSubPlot *subPlot_p, CSubPlotSurface **surface_pp)
{
    *surface_pp = nullptr;

    if (m_surfaces.isEmpty()) {
        return false;
    }

    for (auto& surface_p : m_surfaces) {
        if (surface_p->GetSubPlot() == subPlot_p) {
            *surface_pp = surface_p;
            return true;
        }
    }

    return false;
}

/***********************************************************************************************************************
*   MoveSubPlot
***********************************************************************************************************************/
void CPlotWidget::MoveSubPlot(QList<CSubPlotSurface *> *to_list_p, QList<CSubPlotSurface *> *from_list_p,
                              CSubPlotSurface *moveSubPlot_p)
{
    if ((from_list_p != nullptr) && !from_list_p->isEmpty() && (to_list_p != nullptr)) {
        int index = from_list_p->indexOf(moveSubPlot_p);
        Q_ASSERT(index != -1);
        if (index != -1) {
            CSubPlotSurface *taken_p = from_list_p->takeAt(index);
            to_list_p->append(taken_p);
        }
    }
}

/***********************************************************************************************************************
*   RemoveShadowSubPlot
***********************************************************************************************************************/
void CPlotWidget::RemoveShadowSubPlot(CSubPlotSurface *removeSubPlot_p)
{
    /* This function loops through the shadow list to check if this plotwidget has a shadow that maches m_subPlot_p of
     * removeSubPlot_p. The reason why the shadows have to be removed is typically that the plugin is being removed,
     * and all the surfaces are using the data through direct references. */

    if (m_surfaceShadows.isEmpty()) {
        return;
    }

    CSubPlotSurface *shadow_p = nullptr;

    /* First check if there is shadow that matches. */
    for (auto& surface_p : m_surfaces) {
        if (removeSubPlot_p->m_subPlot_p == surface_p->m_subPlot_p) {
            int index = m_surfaceShadows.indexOf(surface_p);
            Q_ASSERT(index != -1);
            shadow_p = m_surfaceShadows.takeAt(index);
            break;
        }
    }

    if (shadow_p == nullptr) {
#ifdef _DEBUG
        TRACEX_DE("%s not found", __FUNCTION__)
#endif
        return;
    }

    /* Check if the shadow was in the active list */
    if (!m_surfaces.isEmpty()) {
        int index = m_surfaces.indexOf(shadow_p);
        if (index != -1) {
            auto activeShadow = m_surfaces.takeAt(index);
            Q_ASSERT(removeSubPlot_p->m_subPlot_p == activeShadow->m_subPlot_p);
            m_restoreSubPlotSize = true;
            delete activeShadow; /* this is new... should be done, right? */
            update();
            return;
        }
    }

    /* Check if the shadow was in the deactivated list */
    if (!m_surfaces_Deactivated.isEmpty()) {
        int index = m_surfaces_Deactivated.indexOf(shadow_p);
        Q_ASSERT(index != 1); /* since the surface wasn't in the activated this is a failure. */
        if (index != -1) {
            auto deactiveShadow = m_surfaces_Deactivated.takeAt(index);
            Q_ASSERT(removeSubPlot_p->m_subPlot_p == deactiveShadow->m_subPlot_p);
            m_restoreSubPlotSize = true;
            delete deactiveShadow; /* this is new... should be done, right? */
            update();
            return;
        }
    }
}

/***********************************************************************************************************************
*   CreateShadow
***********************************************************************************************************************/
void CPlotWidget::CreateShadow(const ScreenPoint_t *screenPoint_p)
{
    g_currentShadowCopy = GetSurfaceFromPoint(screenPoint_p);
}

/***********************************************************************************************************************
*   InsertShadow
***********************************************************************************************************************/
void CPlotWidget::InsertShadow(const ScreenPoint_t *screenPoint_p)
{
    if (g_currentShadowCopy == nullptr) {
        return;
    }
    m_restoreSubPlotSize = true;  /* make sure that all subplots gets a new rectangle with equal share */

    CSubPlotSurface *selectedSubPlot_p = GetSurfaceFromPoint(screenPoint_p);
    CSubPlotSurface *newSubPlotSurface_p = new CSubPlotSurface(g_currentShadowCopy->m_subPlot_p,
                                                               g_currentShadowCopy->m_parentPlot_p, true);
    int index = m_surfaces.indexOf(selectedSubPlot_p);
    if (index != -1) {
        m_surfaces.insert(index + 1, newSubPlotSurface_p); /* insert after */
    } else {
        m_surfaces.append(newSubPlotSurface_p);
    }
    m_surfaceShadows.append(newSubPlotSurface_p); /* and a ref into the shadows */

    extern void CPlotPane_Align_Reset_Zoom(void);
    CPlotPane_Align_Reset_Zoom();
    update();
}

/***********************************************************************************************************************
*   DeactivateSubPlot
***********************************************************************************************************************/
void CPlotWidget::DeactivateSubPlot(const ScreenPoint_t *screenPoint_p)
{
    auto surface_p = GetSurfaceFromPoint(screenPoint_p);
    if (surface_p == nullptr) {
        return;
    }
    MoveSubPlot(&m_surfaces_Deactivated, &m_surfaces, surface_p);
}

/***********************************************************************************************************************
*   ActivateAllSubPlots
***********************************************************************************************************************/
void CPlotWidget::ActivateAllSubPlots(void)
{
    if (m_surfaces_Deactivated.isEmpty()) {
        return;
    }
    while (!m_surfaces_Deactivated.isEmpty()) {
        auto surface_p = m_surfaces_Deactivated.takeFirst();
        m_surfaces.append(surface_p);
    }
    m_restoreSubPlotSize = true;  /* make sure that all subplots gets a new rectangle with equal share */
    update();
}

/***********************************************************************************************************************
*   Align_X_Zoom
***********************************************************************************************************************/
void CPlotWidget::Align_X_Zoom(double x_min, double x_max)
{
    /* Since a shadow is both in shadow list and in either active/deactive the shadow list shouldn't be updated. */

    m_zoom_left = x_min;
    m_zoom_right = x_max;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            surface_p->SetZoom(x_min, x_max);
        }
    }

    if (!m_surfaces_Deactivated.isEmpty()) {
        for (auto& surface_p : m_surfaces_Deactivated) {
            surface_p->SetZoom(x_min, x_max);
        }
    }

    /* ? There is no update? */
}

/***********************************************************************************************************************
*   Align_Reset_Zoom
***********************************************************************************************************************/
void CPlotWidget::Align_Reset_Zoom(double x_min, double x_max)
{
    /* Since a shadow is both in shadow list and in either active/deactive the shadow list shouldn't be updated. */

    m_zoom_left = x_min;
    m_zoom_right = x_max;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            surface_p->SetZoom(x_min, x_max, true);
        }
    }

    if (!m_surfaces_Deactivated.isEmpty()) {
        for (auto& surface_p : m_surfaces_Deactivated) {
            surface_p->SetZoom(x_min, x_max, true);
        }
    }

    update();
}

/***********************************************************************************************************************
*   Align_X_Cursor
***********************************************************************************************************************/
void CPlotWidget::Align_X_Cursor(double cursorTime, double x_min, double x_max)
{
    m_zoom_left = x_min;
    m_zoom_right = x_max;

    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            surface_p->SetZoom(x_min, x_max);
            surface_p->SetCursor(cursorTime);
        }
    }

    if (!m_surfaces_Deactivated.isEmpty()) {
        for (auto& surface_p : m_surfaces_Deactivated) {
            surface_p->SetZoom(x_min, x_max);
            surface_p->SetCursor(cursorTime);
        }
    }

    /* ? There is no update? */
}

/***********************************************************************************************************************
*   ZoomSubPlotInFocus
***********************************************************************************************************************/
void CPlotWidget::ZoomSubPlotInFocus(bool in, bool horizontal)
{
    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->GetFocus()) {
                QRect rect;
                QPoint point;
                ScreenPoint_t screenPoint;
                bool updateNeeded;

                surface_p->GetWindowRect(&rect);  /* Returns the DC Bitmap coordinates of the subPlot */
                point = QPoint(rect.left() + (rect.width() / 2), rect.top() + (rect.height() / 2));
                FillScreenPoint_FromDCViewPortPoint(&point, &screenPoint);

                short zDelta = in ? 1 : -1;
                if (horizontal) {
                    ZoomSubPlot_X_Axis(zDelta, &screenPoint, &updateNeeded);
                } else {
                    ZoomSubPlot_Y_Axis(zDelta, &screenPoint, &updateNeeded);
                }

                update();
                return;
            }
        }
    }
}

/***********************************************************************************************************************
*   ResizeSubPlotInFocus
***********************************************************************************************************************/
void CPlotWidget::ResizeSubPlotInFocus(bool increase)
{
    if (!m_surfaces.isEmpty()) {
        for (auto& surface_p : m_surfaces) {
            if (surface_p->GetFocus()) {
                QRect rect;
                QPoint point;
                ScreenPoint_t screenPoint;
                bool updateNeeded;

                surface_p->GetWindowRect(&rect);  /* Returns the DC Bitmap coordinates of the subPlot */
                point = QPoint(rect.left() + (rect.width() / 2), rect.top() + (rect.height() / 2));
                FillScreenPoint_FromDCViewPortPoint(&point, &screenPoint);

                short zDelta = increase ? 1 : -1;

                ModifySubPlotSize(zDelta, &screenPoint, &updateNeeded);

                update();
                return;
            }
        }
    }
}
