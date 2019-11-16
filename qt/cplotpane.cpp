/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QPen>
#include <QSettings>
#include <QApplication>

#include "CDebug.h"
#include "cplotpane.h"
#include "CConfig.h"
#include "globals.h"
#include "utils.h"

#include "plugin_api.h"
#include "plugin_base.h"

#include "cplotwidget.h"
#include "cplotwidget_graphics.h"
#include "csubplotsurface.h"
#include "CLogScrutinizerDoc.h"

#include "mainwindow_cb_if.h"

#include "../processing/CThread.h"

CPlotPane *g_CPlotPane = nullptr;
QPen *g_plotWnd_focusPen_p = nullptr;
QPen *g_plotWnd_passiveFocusPen_p = nullptr;   /* In-case the sub-plot has focus but not the PlotWnd */
QPen *g_plotWnd_defaultPen_p = nullptr;
QPen *g_plotWnd_defaultDotPen_p = nullptr;
FontItem_t *g_plotWnd_GrayFont_p = nullptr;
FontItem_t *g_plotWnd_BlackFont_p = nullptr;
FontItem_t *g_plotWnd_WhiteFont_p = nullptr;
CSubPlotSurface *g_currentShadowCopy = nullptr; /* Used as temp storage for copying */
CThreadManager *g_CPlotPane_ThreadMananger_p = nullptr;
CPlot *CPlotPane_GetPlotFocus(void);

/***********************************************************************************************************************
*   CPlotPane_GetPlotPane
***********************************************************************************************************************/
CPlotPane *CPlotPane_GetPlotPane(void)
{
    return g_CPlotPane;
}

/***********************************************************************************************************************
*   CPlotPane_ResetPlotFocus
***********************************************************************************************************************/
void CPlotPane_ResetPlotFocus(void)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->resetPlotFocus();
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_Align_X_Zoom
***********************************************************************************************************************/
void CPlotPane_Align_X_Zoom(double x_min, double x_max)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->align_X_Zoom(x_min, x_max);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_Align_Reset_Zoom
***********************************************************************************************************************/
void CPlotPane_Align_Reset_Zoom(void)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->align_Reset_Zoom();
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_Align_X_Cursor
***********************************************************************************************************************/
void CPlotPane_Align_X_Cursor(double cursorTime, double x_min, double x_max)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->align_X_Cursor(cursorTime, x_min, x_max);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_RemoveRelatedShadowSubPlots
***********************************************************************************************************************/
void CPlotPane_RemoveRelatedShadowSubPlots(CPlotWidgetInterface *plotWidgetSrc_p)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->removeRelatedShadowSubPlots(plotWidgetSrc_p);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_SetPlotFocus
***********************************************************************************************************************/
void CPlotPane_SetPlotFocus(CPlot *plot_p)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->setPlotFocus(plot_p);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_SetSubPlotFocus
***********************************************************************************************************************/
void CPlotPane_SetSubPlotFocus(CSubPlot *subPlot_p)
{
    if (g_CPlotPane != nullptr) {
        g_CPlotPane->setSubPlotFocus(subPlot_p);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_NextPlot
***********************************************************************************************************************/
bool CPlotPane_NextPlot(bool backward)
{
    if ((g_CPlotPane != nullptr) && (g_CPlotPane->count() > 0)) {
        g_CPlotPane->nextTab(backward);
        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   CPlotPane_GetPlotFocus
***********************************************************************************************************************/
CPlot *CPlotPane_GetPlotFocus(void)
{
    CPlot *plot_p = nullptr;
    CPlotWidgetInterface *plotWidget_p = nullptr;

    if (g_CPlotPane != nullptr) {
        g_CPlotPane->getPlotFocus(&plotWidget_p, &plot_p);
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_E(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }

    return plot_p;
}

/***********************************************************************************************************************
*   CPlotPane_ZoomSubPlotInFocus
***********************************************************************************************************************/
void CPlotPane_ZoomSubPlotInFocus(bool in  /*or out*/, bool horizontal)
{
    /* The caller may choose to use the zoom factor or the in/out variable. If the zoom is 0.0 then the bool
     * in is used instead */

    CPlot *plot_p = nullptr;
    CPlotWidgetInterface *plotWidget_p = nullptr;

    if (g_CPlotPane != nullptr) {
        g_CPlotPane->getPlotFocus(&plotWidget_p, &plot_p);

        if (plotWidget_p != nullptr) {
            plotWidget_p->ZoomSubPlotInFocus(in, horizontal);
        } else {
            Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
            if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
                TRACEX_W(QString("%1  plotWidget_p nullptr").arg(__FUNCTION__))
            }
        }
    } else {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        if (CSCZ_SystemState == SYSTEM_STATE_RUNNING) {
            TRACEX_W(QString("%1  g_CPlotPane nullptr").arg(__FUNCTION__))
        }
    }
}

/***********************************************************************************************************************
*   CPlotPane_ResizeSubPlotInFocus
***********************************************************************************************************************/
void CPlotPane_ResizeSubPlotInFocus(bool increase)
{
    CPlot *plot_p = nullptr;
    CPlotWidgetInterface *plotWnd_p = nullptr;

    if (g_CPlotPane != nullptr) {
        g_CPlotPane->getPlotFocus(&plotWnd_p, &plot_p);
    } else {
        TRACEX_E(QString("%1   g_CPlotPane nullptr").arg(__FUNCTION__))
    }

    if (plotWnd_p != nullptr) {
        plotWnd_p->ResizeSubPlotInFocus(increase);
    } else {
        TRACEX_E(QString("%1 plot_p nullptr").arg(__FUNCTION__))
    }
}

/***********************************************************************************************************************
*   CPlotPane_RestoreSubPlotSizes
***********************************************************************************************************************/
void CPlotPane_RestoreSubPlotSizes(void)
{
    CPlot *plot_p = nullptr;
    CPlotWidgetInterface *plotWnd_p = nullptr;

    if (g_CPlotPane != nullptr) {
        g_CPlotPane->getPlotFocus(&plotWnd_p, &plot_p);
    } else {
        TRACEX_E(QString("%1    g_CPlotPane nullptr").arg(__FUNCTION__))
    }

    if (plotWnd_p != nullptr) {
        plotWnd_p->RestoreSubPlotSizes();
    } else {
        TRACEX_E(QString("%1    plotWnd_p nullptr").arg(__FUNCTION__))
    }
}

CPlotPane::CPlotPane(QWidget *parent) : QTabWidget(parent)
{
    g_plotWnd_defaultPen_p = new QPen(g_cfg_p->m_plot_GraphAxisLineColor ?
                                      Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                            g_cfg_p->m_plot_GrayIntensity,
                                            g_cfg_p->m_plot_GrayIntensity) :
                                      g_cfg_p->m_plot_GraphAxisLineColor);

    g_plotWnd_defaultPen_p->setStyle(Qt::SolidLine);
    g_plotWnd_defaultPen_p->setWidth(static_cast<int>(g_cfg_p->m_plot_GraphAxisLineSize));

    g_plotWnd_defaultDotPen_p = new QPen(g_cfg_p->m_plot_GraphAxisLineColor ?
                                         Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                               g_cfg_p->m_plot_GrayIntensity,
                                               g_cfg_p->m_plot_GrayIntensity) :
                                         g_cfg_p->m_plot_GraphAxisLineColor);
    g_plotWnd_defaultDotPen_p->setStyle(Qt::DotLine);
    g_plotWnd_defaultDotPen_p->setWidth(static_cast<int>(g_cfg_p->m_plot_GraphAxisLineSize));

    CLogScrutinizerDoc *doc_p = GetTheDoc();
    g_plotWnd_GrayFont_p = doc_p->m_fontCtrl.RegisterFont(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                                                g_cfg_p->m_plot_GrayIntensity,
                                                                g_cfg_p->m_plot_GrayIntensity), BACKGROUND_COLOR);

    g_plotWnd_BlackFont_p = doc_p->m_fontCtrl.RegisterFont(0, BACKGROUND_COLOR);

    g_plotWnd_focusPen_p = new QPen(g_cfg_p->m_plot_FocusLineColor);
    g_plotWnd_focusPen_p->setStyle(Qt::SolidLine);
    g_plotWnd_focusPen_p->setWidth(static_cast<int>(g_cfg_p->m_plot_FocusLineSize));

    g_plotWnd_passiveFocusPen_p = new QPen(g_cfg_p->m_plot_PassiveFocusLineColor);
    g_plotWnd_passiveFocusPen_p->setStyle(Qt::SolidLine);
    g_plotWnd_passiveFocusPen_p->setWidth(static_cast<int>(g_cfg_p->m_plot_FocusLineSize));

    g_CPlotPane_ThreadMananger_p = &m_threadManager;
    g_CPlotPane = this;
}

CPlotPane::~CPlotPane()
{
    /* If plotpane was cloese by plugins unloaded then prepare delete has already been called. However at system
     * shutdown closeEvent is never received */
    m_threadManager.PrepareDelete();

    DELETE_AND_CLEAR(g_plotWnd_defaultPen_p);
    DELETE_AND_CLEAR(g_plotWnd_defaultDotPen_p);
    DELETE_AND_CLEAR(g_plotWnd_focusPen_p);
    DELETE_AND_CLEAR(g_plotWnd_passiveFocusPen_p);

    g_CPlotPane = nullptr;

    g_CPlotPane_ThreadMananger_p = nullptr;
    g_currentShadowCopy = nullptr;
}

/***********************************************************************************************************************
*   showEvent
***********************************************************************************************************************/
void CPlotPane::showEvent(QShowEvent *event)
{
    static bool first = true;
    if (first) {
        first = false;
    }

    QTabWidget::showEvent(event);
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CPlotPane::keyPressEvent(QKeyEvent *e)
{
    auto SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    auto CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    int index = e->key();
    switch (index)
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Tab:
            break;

        default:
            if (MW_GeneralKeyHandler(index, CTRL_Pressed, SHIFT_Pressed)) {
                return;
            }
            break;
    }
    QWidget::keyPressEvent(e);
}

/***********************************************************************************************************************
*   event
***********************************************************************************************************************/
bool CPlotPane::event(QEvent *event)
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
    return QTabWidget::event(event);
}

/* Note: closeEvent is not called in-case application is terminated */
void CPlotPane::closeEvent(QCloseEvent *event)
{
    m_threadManager.PrepareDelete();
    QTabWidget::closeEvent(event);
}

/***********************************************************************************************************************
*   storeSettings
***********************************************************************************************************************/
void CPlotPane::storeSettings(void)
{
#ifdef LOCAL_GEOMETRY_SETTING
    QSettings settings;
    settings.setValue("plotPaneWindow", size());
#endif
}

/***********************************************************************************************************************
*   sizeHint
***********************************************************************************************************************/
QSize CPlotPane::sizeHint() const
{
    static QSize windowSize;
    auto refreshEditorWindow = makeMyScopeGuard([&] () {
        PRINT_SIZE(QString("PlotPane sizeHint %1,%2").arg(windowSize.width()).arg(windowSize.height()));
    });
    windowSize = QTabWidget::sizeHint();

    if (CSCZ_AdaptWindowSizes) {
        windowSize = m_adaptWindowSize;
        PRINT_SIZE(QString("PlotPane adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()));
    }

    return windowSize;
}

/***********************************************************************************************************************
*   addPlot
***********************************************************************************************************************/
CPlotWidgetInterface *CPlotPane::addPlot(const char *plotName_p, CPlot *plot_p)
{
    Q_UNUSED(plotName_p)

    CPlotWidgetInterface *pwi_p;
    CList_LSZ *list_p;
    if (plot_p->GetSubPlots(&list_p)) {
        auto subPlot_p = reinterpret_cast<CSubPlot *>(list_p->first());
        if (subPlot_p->GetProperties() & SUB_PLOT_PROPERTY_PAINTING) {
            pwi_p = dynamic_cast<CPlotWidgetInterface *>(new CPlotWidgetGraphics(plot_p));
        } else {
            pwi_p = dynamic_cast<CPlotWidgetInterface *>(new CPlotWidget(plot_p));
        }
    } else {
        return nullptr;
    }

    char *title_p = nullptr;
    char *axisLabel_p = nullptr;
    plot_p->GetTitle(&title_p, &axisLabel_p);
    Q_UNUSED(axisLabel_p)
    addTab(dynamic_cast<QWidget *>(pwi_p), QString(title_p));
    m_plotWnds.append(pwi_p);

    return pwi_p;
}

/***********************************************************************************************************************
*   removePlot
***********************************************************************************************************************/
void CPlotPane::removePlot(CPlot *plot_p)
{
    if (m_plotWnds.isEmpty()) {
        TRACEX_W("CPlotPane::RemovePlot  Failed part")
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        char *title, *axis;
        plot_p->GetTitle(&title, &axis);
        if (plotWidget_p->GetPlotRef() == plot_p) {
            TRACEX_I(QString("%1 %2").arg(__FUNCTION__).arg(title))

            /*Detach all shadow subplots from other plotWnds */
            removeRelatedShadowSubPlots(plotWidget_p);

            int index = indexOf(dynamic_cast<QWidget *>(plotWidget_p));
            Q_ASSERT(index != -1);
            if (index != -1) {
                removeTab(index);
            }
            index = m_plotWnds.indexOf(plotWidget_p);
            Q_ASSERT(index != -1);
            if (index != -1) {
                m_plotWnds.removeAt(index);
            }

            delete plotWidget_p;
            MW_Refresh();
            return;
        }
    }
}

/***********************************************************************************************************************
*   resetPlotFocus
***********************************************************************************************************************/
void CPlotPane::resetPlotFocus(void)
{
    for (auto& plotWidget_p : m_plotWnds) {
        plotWidget_p->ResetFocus();
    }
}

/***********************************************************************************************************************
*   setPlotFocus
***********************************************************************************************************************/
void CPlotPane::setPlotFocus(CPlot *plot_p)
{
    int index = 0;

    if (m_plotWnds.isEmpty()) {
        TRACEX_W("CPlotPane::SetPlotFocus  Failed part-1")
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        if (plotWidget_p->GetPlotRef() == plot_p) {
            index = indexOf(dynamic_cast<QWidget *>(plotWidget_p));
            Q_ASSERT(index != -1);
            if (index != -1) {
                auto title = plotWidget_p->GetTitle();
                TRACEX_I(QString("%1 Index:%2 %3").arg(__FUNCTION__).arg(index).arg(title))
                setCurrentIndex(index);
                return;
            }
        }
    }
}

/***********************************************************************************************************************
*   setSubPlotFocus
***********************************************************************************************************************/
void CPlotPane::setSubPlotFocus(CSubPlot *subPlotFocus_p)
{
    int index = 0;

    if (m_plotWnds.isEmpty()) {
        TRACEX_W(QString("%1 Failed part-1").arg(__FUNCTION__))
        return;
    }

    CPlotWidgetInterface *plotWidget_p = nullptr;
    CPlot *plot_p = nullptr;
    CSubPlot *subPlot_p = nullptr;
    CSubPlotSurface *surface_p = nullptr;

    getPlotFocus(&plotWidget_p, &plot_p, &subPlot_p, &surface_p);

    if ((subPlot_p != nullptr) && (subPlot_p == subPlotFocus_p)) {
        /* All is good already */
        return;
    }

    resetPlotFocus();

    for (auto& plotWidget_p : m_plotWnds) {
        if (plotWidget_p->SearchSubPlot(subPlotFocus_p, &surface_p)) {
            index = indexOf(dynamic_cast<QWidget *>(plotWidget_p));
            Q_ASSERT(index != -1);
            if (index != -1) {
                setCurrentIndex(index);
                plotWidget_p->SetSurfaceFocus(surface_p->m_subPlot_p);
            }
            return;
        }
    }
}

/***********************************************************************************************************************
*   getPlotFocus
***********************************************************************************************************************/
void CPlotPane::getPlotFocus(CPlotWidgetInterface **plotWidget_pp, CPlot **plot_pp, CSubPlot **subPlot_pp,
                             CSubPlotSurface **surface_pp)
{
    Q_ASSERT(plotWidget_pp);
    Q_ASSERT(plot_pp);

    *plotWidget_pp = nullptr;
    *plot_pp = nullptr;

    if (subPlot_pp != nullptr) {
        *subPlot_pp = nullptr;
    }
    if (surface_pp != nullptr) {
        *surface_pp = nullptr;
    }

    if (m_plotWnds.isEmpty()) {
        TRACEX_W(QString("%1   Failed, no plots").arg(__FUNCTION__))
        return;
    }

    *plotWidget_pp = static_cast<CPlotWidget *>(currentWidget());
    Q_ASSERT(*plotWidget_pp);

    *plot_pp = (*plotWidget_pp)->GetPlotRef();
    if ((subPlot_pp != nullptr) && (surface_pp != nullptr)) {
        (*plotWidget_pp)->SearchFocus(subPlot_pp, surface_pp);
    }
}

/***********************************************************************************************************************
*   getPlotWidget
***********************************************************************************************************************/
CPlotWidgetInterface *CPlotPane::getPlotWidget(CPlot *plot_p)
{
    if (m_plotWnds.isEmpty()) {
        return nullptr;
    }
    for (auto& plotWidget_p : m_plotWnds) {
        if (plot_p == plotWidget_p->GetPlotRef()) {
            return plotWidget_p;
        }
    }
    return nullptr;
}

/***********************************************************************************************************************
*   nextTab
***********************************************************************************************************************/
void CPlotPane::nextTab(bool backward)
{
    auto numOfTabs = count();
    auto index = currentIndex();
    index = backward ? index - 1 : index + 1;
    if ((index >= numOfTabs) || (index < 0)) {
        index = 0;
    }
    setCurrentIndex(index);
    widget(index)->setFocus();
}

/***********************************************************************************************************************
*   cleanAllPlots
***********************************************************************************************************************/
void CPlotPane::cleanAllPlots(void)
{
    CPlot *plot_p;

    if (m_plotWnds.isEmpty()) {
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        plot_p = plotWidget_p->GetPlotRef();
        plot_p->PlotClean();
        plotWidget_p->Initialize();
        plotWidget_p->Redraw();
    }

    update();
}

/***********************************************************************************************************************
*   align_X_Zoom
***********************************************************************************************************************/
void CPlotPane::align_X_Zoom(double x_min, double x_max)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        plotWidget_p->Align_X_Zoom(x_min, x_max);
    }

    update();
}

/***********************************************************************************************************************
*   align_Reset_Zoom
***********************************************************************************************************************/
void CPlotPane::align_Reset_Zoom(void)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }

    double min_x = 0.0;
    double max_x = 0.0;
    bool init = true;

    /* loop through and take the extents of all graphs and make it largest possible */

    for (auto& plotWidget_p : m_plotWnds) {
        if (!plotWidget_p->isInitialized()) {
            continue;
        }

        QRectF rect = plotWidget_p->GetMinMax();

        if (init) {
            init = false;

            min_x = rect.left();
            max_x = rect.right();
        } else {
            if (rect.left() < min_x) {
                min_x = rect.left();
            }

            if (rect.right() > max_x) {
                max_x = rect.right();
            }
        }
    }

    if (init) {
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        plotWidget_p->Align_Reset_Zoom(min_x, max_x);
    }

    update();
}

/***********************************************************************************************************************
*   align_X_Cursor
***********************************************************************************************************************/
void CPlotPane::align_X_Cursor(double cursorTime, double x_min, double x_max)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        plotWidget_p->Align_X_Cursor(cursorTime, x_min, x_max);
    }
    update();
}

/***********************************************************************************************************************
*   isPlotsActive
***********************************************************************************************************************/
bool CPlotPane::isPlotsActive(void)
{
    if (m_plotWnds.isEmpty()) {
        return false;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        if (plotWidget_p->isInitialized()) {
            return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   setPlotCursor
***********************************************************************************************************************/
void CPlotPane::setPlotCursor(int row, int *cursorRow_p)
{
    bool first = true;
    int min_distance = 0;
    int distance = 0;
    GraphicalObject_t *bestGo_p = nullptr;
    GraphicalObject_t *go_p = nullptr;
    CPlotWidgetInterface *bestPlotWidget_p = nullptr;

    *cursorRow_p = 0;

    if (m_plotWnds.isEmpty()) {
        return;
    }

    for (auto& plotWidget_p : m_plotWnds) {
        if (plotWidget_p->GetClosest_GO(static_cast<int>(row), &go_p, &distance)) {
            if (first) {
                first = false;
                bestGo_p = go_p;
                min_distance = distance;
                bestPlotWidget_p = plotWidget_p;
            } else {
                if (distance < min_distance) {
                    min_distance = distance;
                    bestGo_p = go_p;
                    bestPlotWidget_p = plotWidget_p;
                }
            }
        }
    }

    if (!first && (bestPlotWidget_p != nullptr)) {
        int bestRow = bestGo_p->row;
        double time = bestGo_p->x2;

        if (bestGo_p->properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
            int diff2 = static_cast<int>(row) -
                        static_cast<int>(reinterpret_cast<GraphicalObject_Box_t *>(bestGo_p)->row2);
            int diff1 = static_cast<int>(row) - static_cast<int>(bestGo_p->row);

            diff1 = diff1 < 0 ? diff1 * -1 : diff1;
            diff2 = diff2 < 0 ? diff2 * -1 : diff2;
            if (diff2 < diff1) {
                bestRow = reinterpret_cast<GraphicalObject_Box_t *>(bestGo_p)->row2;
            } else {
                time = bestGo_p->x1;
            }
        }

        *cursorRow_p = bestRow;
        bestPlotWidget_p->SetFocusTime(time); /* this will result in a callback to the plotPane to align the X zoom */
    } else {
        TRACEX_W(QString("%1   Warning: Failed to set plot cursor, couldn't "
                         "find matching ro").arg(__FUNCTION__));

        /* MessageBox("Couldn't find a proper row in the
         * plot to set the cursor", "Failed to set cursor", MB_OK | MB_TOPMOST); */
    }
}

/***********************************************************************************************************************
*   setPlotCursor
***********************************************************************************************************************/
void CPlotPane::setPlotCursor(const double time)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }
    m_plotWnds.first()->SetFocusTime(time); /* this will result in a callback to the plotPane to align the X zoom */
}

/***********************************************************************************************************************
*   setSubPlotUpdated
***********************************************************************************************************************/
void CPlotPane::setSubPlotUpdated(CSubPlot *subPlot_p)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }

    /* Search after the subplot surface which reference the subPlot */
    for (auto& plotWidget_p : m_plotWnds) {
        plotWidget_p->SetUpdated(subPlot_p); /* will search and update if exist */
    }
}

/***********************************************************************************************************************
*   redrawPlot
***********************************************************************************************************************/
void CPlotPane::redrawPlot(CPlot *plot_p)
{
    if (m_plotWnds.isEmpty()) {
        return;
    }

    /* Search after the PlotWnd which reference plot_p, if found invalidate */
    for (auto& plotWidget_p : m_plotWnds) {
        if ((plot_p == nullptr) || (plotWidget_p->GetPlotRef() == plot_p)) {
            plotWidget_p->Redraw();
        }
    }
}

/***********************************************************************************************************************
*   removeRelatedShadowSubPlots
***********************************************************************************************************************/
void CPlotPane::removeRelatedShadowSubPlots(CPlotWidgetInterface *plotWidgetSrc_p /*plotWndSrc_p*/)
{
    if (m_plotWnds.count() == 1) {
        /* The plotWnd being removed is the only one, no need to search in other windows */
        return;
    }

    /* The active and deactivated subplots in plotWidgetSrc_p shall be searched in other plotWnds
     * RemoveShadowSubPlot will remove the surface from plotWidget_p IF surface_p was found in plotWidget_p
     * shadow surfaces. */
    QList<CSubPlotSurface *> *active;
    QList<CSubPlotSurface *> *deactivated;
    plotWidgetSrc_p->GetSurfaces(&active, &deactivated);

    if ((active != nullptr) && (deactivated != nullptr)) {
        for (auto& plotWidget_p : m_plotWnds) {
            if (plotWidget_p != plotWidgetSrc_p) {
                plotWidget_p->RemoveShadowSubPlotsMatching(active);
                plotWidget_p->RemoveShadowSubPlotsMatching(deactivated);
            }
        }
    }
    MW_Refresh();
}
