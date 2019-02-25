/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QSplitter>
#include <QVBoxLayout>

#include "csubplotsurface.h"
#include "plugin_api.h"
#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "ceditorwidget.h"
#include "cplotwidget_if.h"

#undef min
#undef max
#include <algorithm>

#define ZOOM_STEP                   ((double)0.1)

/***********************************************************************************************************************
*   CGraphicsSubplotSurface
***********************************************************************************************************************/
class CGraphicsSubplotSurface
{
public:
    CGraphicsSubplotSurface(QWidget *parent, CSubPlot *subPlot_p)
        : m_view(&m_scene, parent), m_subPlot_p(subPlot_p)
    {
        m_view.setRenderHint(QPainter::Antialiasing, false);
        m_view.setDragMode(QGraphicsView::ScrollHandDrag);
        m_view.setOptimizationFlags(QGraphicsView::DontSavePainterState);
        m_view.setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
        m_view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        m_view.setInteractive(true);
    }

    CGraphicsSubplotSurface() = delete;
    virtual ~CGraphicsSubplotSurface() {}

    /****/
    void OptimumScale(void)
    {
        auto windowRect = m_view.rect();
        QMatrix matrix;
        auto x_scale = (windowRect.width() * 0.95) / m_boundingRect.width();
        auto y_scale = (windowRect.height() * 0.95) / m_boundingRect.height();
        auto best_scale = std::min(x_scale, y_scale);
        matrix.scale(best_scale, best_scale);
        m_view.setMatrix(matrix);
    }

    void Populate(void);

    QGraphicsView *view(void) {return &m_view;}

private:
    QGraphicsScene m_scene;
    QGraphicsView m_view;
    CSubPlot *m_subPlot_p;
    QRectF m_boundingRect;

    /* members from subplotsurface */
    std::vector<CGO_Label *> m_label_refs_a;
    int m_numOfLabelRefs;
    CDisplayGraph m_displayDecorator;   /* Possibly there is a decorator */
    int m_subplot_properties;   /*updated each time when drawing axis   = m_subPlot_p->GetProperties(); */
};

/* The CPlotWidgetGraphics has only one surface */

class CPlotWidgetGraphics : public QFrame, public CPlotWidgetInterface
{
    /*class CPlotWidgetGraphics : public QGraphicsView, public CPlotWidgetInterface { */

    Q_OBJECT

public:
    CPlotWidgetGraphics(CPlot *plot_p);

    virtual ~CPlotWidgetGraphics() override;

    virtual void RemoveSurfaces(void) override;
    virtual CPlot *GetPlotRef(void) override {return m_plot_p;}
    virtual int GetMinSize_Y(void) override {return MIN_WINDOW_HEIGHT;}

    /****/
    virtual QRectF GetMinMax(void) override {
        QRectF rect = QRect(0.0, 0.0, 0.0, 0.0);
        return rect;
    }

    /****/
    virtual void SetMinMax(QRectF& rect) override {
        Q_UNUSED(rect);
    }

    /****/
    virtual void SetUpdated(CSubPlot *subPlot_p) override {
        /* CPlotWidgetGraphics has only one subplot */
        SetUpdateSubplots();
    }

    /****/
    virtual void RemoveShadowSubPlotsMatching(QList<CSubPlotSurface *> *matchSurfaces) override {
        if (!matchSurfaces->isEmpty()) {
            for (auto& surface_p : *matchSurfaces) {
                RemoveShadowSubPlot(surface_p);
            }
        }
    }

    /****/
    virtual void GetSurfaces(QList<CSubPlotSurface *> **active, QList<CSubPlotSurface *> **deactivated) override {
        *active = nullptr;
        *deactivated = nullptr;
    }

    virtual void Initialize(void) override;
    virtual void Redraw(void) override;

    /****/
    virtual QString GetTitle(void) override {
        char *title_p, *axis_p;
        m_plot_p->GetTitle(&title_p, &axis_p);
        return QString(title_p);
    }

    virtual void SurfaceToClipBoard(const ScreenPoint_t *screenPoint_p) override;
    virtual bool GetClosestGraph(ScreenPoint_t *screenPoint_p, CGraph **graph_pp,
                                 GraphicalObject_t **go_pp, CSubPlot **subPlot_pp) override;
    virtual bool GetClosestGraph(int row, CGraph **graph_pp, GraphicalObject_t **go_pp,
                                 CSubPlot **subPlot_pp) override;
    virtual bool GetClosest_GO(int row, GraphicalObject_t **go_pp, int *distance_p) override;
    virtual void SetFocusTime(const double time) override;
    virtual bool isInitialized(void) override {return m_surfacesInitialized;}
    virtual void SetUpdateSubplots(void) override {m_restoreSubPlotSize = true;}

    virtual bool isSurfaceInList(QList<CSubPlotSurface *> *list_p, CSubPlotSurface *surface_p) override;
    virtual void SetSurfaceFocus(CSubPlot *subPlot_p) override;
    virtual bool SearchFocus(CSubPlot **subPlot_pp, CSubPlotSurface **surface_pp) override;
    virtual bool SearchSubPlot(CSubPlot *subPlot_p, CSubPlotSurface **surface_pp) override;
    virtual void ResetFocus(void) override; /* Go through all subPlots and set the focus flag to false */

    /****/
    virtual void MoveSubPlot(QList<CSubPlotSurface *> *to_list_p, QList<CSubPlotSurface *> *from_list_p,
                             CSubPlotSurface *moveSubPlot_p) override {
        Q_UNUSED(to_list_p);
        Q_UNUSED(from_list_p);
        Q_UNUSED(moveSubPlot_p); /* No action */
    }

    virtual void RemoveShadowSubPlot(CSubPlotSurface *subPlot_p) override {Q_UNUSED(subPlot_p); /* No action */}

    virtual void Align_X_Zoom(double x_min, double x_max) override;
    virtual void Align_Reset_Zoom(double x_min, double x_max) override;
    virtual void Align_X_Cursor(double cursorTime, double x_min, double x_max) override;
    virtual void ZoomSubPlotInFocus(double zoom, bool in, bool horizontal) override;
    virtual void ResizeSubPlotInFocus(double zoom, bool increase) override;
    virtual void RestoreSubPlotSizes(void) override {m_restoreSubPlotSize = true; update();}
    virtual void ZoomRestore(void) override;

public:
private:
    CPlotWidgetGraphics() = delete;
    void Init(void);
    void InitilizeSubPlots(void);
    void Populate(void);
    int GraphsObjectCount(void);

    void FillEmptyWindow(QPainter *painter);

    void UpdateFocus(const ScreenPoint_t *screenPoint_p);

    void RestoreSubPlotWindows(void);  /* Spread them evenly */
    bool SetupWindowSizes(void);

    void ModifySubPlotSize(short zDelta, const ScreenPoint_t *screenPoint_p,
                           bool *invalidate_p = nullptr);
    void ZoomSubPlot_X_Axis(short zDelta, const ScreenPoint_t *screenPoint_p,
                            bool *invalidate_p = nullptr);
    void ZoomSubPlot_Y_Axis(short zDelta, const ScreenPoint_t *screenPoint_p,
                            bool *invalidate_p = nullptr);

    void FillScreenPoint_FromDCViewPortPoint(QPoint *viewPortPoint_p,
                                             ScreenPoint_t *screenPoint_p);

    void DeactivateSubPlot(const ScreenPoint_t *screenPoint_p);
    void ActivateAllSubPlots(void);

    void CreateShadow(const ScreenPoint_t *screenPoint_p);
    void InsertShadow(const ScreenPoint_t *screenPoint_p);

    void OnContextMenu(ScreenPoint_t& screenPoint);

    bool HandleKeyDown(QKeyEvent *e);

    void DrawToolTip(void);
    bool OpenToolTip(void);
    void CloseToolTip(void);

    /****/
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE
    {
        Q_UNUSED(event);

        QPainter painter(this);

        m_windowRect = rect();
        painter.fillRect(m_windowRect, BOARDER_BACKGROUND_COLOR);

        if (GraphsObjectCount() == 0) {
            FillEmptyWindow(&painter);
            return;
        }

        if (m_rescale && !m_surfaces.isEmpty()) {
            for (auto& surface_p : m_surfaces) {
                surface_p->OptimumScale();
            }
            m_rescale = false;
        }
    }

    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const Q_DECL_OVERRIDE {return QSize();}
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

public slots:
    void onToolTipTimer(void);

private:
    /* A painting has only one plot/subplot */
    CPlot *m_plot_p;
    QHBoxLayout *m_hlayout_p = nullptr;
    QSplitter *m_splitter_p = nullptr;
    QList<int> m_keyPressedList; /* store the pressed/released keys */
    QRectF m_boundingRect;
    bool m_rescale = false;
    QList<CGraphicsSubplotSurface *> m_surfaces;
    std::unique_ptr<QTimer> m_toolTipTimer;
    ToolTipState m_toolTipState
    ;

    /* Contains tooltip strings and extra info that plugins might provide */
    QList<QString> m_toolTipStrings;
    bool m_toolTipEnabled;
    FontItem_t *m_tooltipFont_p;
    FontItem_t *m_FontEmptyWindow_p;
    bool m_surfacesInitialized;
    bool m_surfacesCreated;
    QRect m_windowRect;   /* The full widget coordinates on screen */
    QRect m_viewRect;   /* The m_view_p coordinates on screen */
    bool m_restoreSubPlotSize;
    QRectF m_viewPortRect; /* The window in wich the graphs is drawn */
    LS_Painter *m_pDC;
    QBrush *m_bgBoarderBrush_p;
    QPen *m_bgBoarderPen_p;
    ScreenPoint_t m_lastCursorPos;
    bool m_inDestructor;
    bool m_inFocus;
    CDisplayGraph m_displayDecorator;   /* Possibly there is a decorator */
    int m_subplot_properties;   /*updated each time when drawing axis            =
                                 * m_subPlot_p->GetProperties(); */
};
