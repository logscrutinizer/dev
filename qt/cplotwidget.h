/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QWidget>

#include "csubplotsurface.h"
#include "plugin_api.h"
#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "ceditorwidget.h"
#include "cplotwidget_if.h"

#define V_SCROLLBAR_WIDTH 20
#define H_SCROLLBAR_HEIGHT 20

/***********************************************************************************************************************
*   CPlotWidget
***********************************************************************************************************************/
class CPlotWidget : public QWidget, public CPlotWidgetInterface
{
    Q_OBJECT

public:
    CPlotWidget(CPlot *plot_p);
    virtual ~CPlotWidget() override;

    virtual void RemoveSurfaces(void) override;
    virtual CPlot *GetPlotRef(void) override {return m_plot_p;}
    virtual int GetMinSize_Y(void) override {return (m_surfaces.count() * MIN_WINDOW_HEIGHT);}

    /****/
    virtual QRectF GetMinMax(void) override {
        QRectF rect = QRect(0.0, 0.0, 0.0, 0.0);
        rect.setLeft(m_min_X);
        rect.setRight(m_max_X);
        return rect;
    }

    /****/
    virtual void SetMinMax(QRectF& rect) override {
        m_min_X = rect.left();
        m_max_X = rect.right();
    }

    /****/
    virtual void SetUpdated(CSubPlot *subPlot_p) override {
        for (auto& surface_p : m_surfaces) {
            if (subPlot_p == surface_p->GetSubPlot()) {
                /* There was a match, update the plot window that contains the subplot */
                SetUpdateSubplots();
                return;
            }
        }
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
        *active = &m_surfaces;
        *deactivated = &m_surfaces_Deactivated;
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
    virtual bool GetClosestGraph(ScreenPoint_t *screenPoint_p, CGraph_Internal **graph_pp,
                                 GraphicalObject_t **go_pp, CSubPlot **subPlot_pp) override;
    virtual bool GetClosestGraph(int row, CGraph_Internal **graph_pp, GraphicalObject_t **go_pp,
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
    virtual void MoveSubPlot(QList<CSubPlotSurface *> *to_list_p, QList<CSubPlotSurface *> *from_list_p,
                             CSubPlotSurface *moveSubPlot_p) override;
    virtual void RemoveShadowSubPlot(CSubPlotSurface *subPlot_p) override;
    virtual void Align_X_Zoom(double x_min, double x_max) override;
    virtual void Align_Reset_Zoom(double x_min, double x_max) override;
    virtual void Align_X_Cursor(double cursorTime, double x_min, double x_max) override;
    virtual void ZoomSubPlotInFocus(bool in, bool horizontal) override;
    virtual void ResizeSubPlotInFocus(bool increase) override;
    virtual void RestoreSubPlotSizes(void) override {m_restoreSubPlotSize = true; update();}
    virtual void ZoomRestore(void) override;

public:
private:
    CPlotWidget() = delete;
    void Init(void);
    void InitilizeSubPlots(void);
    void DrawSubPlots(void);

    void DrawScrollbar(void);
    void FillEmptyWindow(void);

    void ResetZoom(void);
    void UpdateFocus(const ScreenPoint_t *screenPoint_p);

    void RestoreSubPlotWindows(void);  /* Spread them evenly */
    void SetupTotalDQRect(void);
    bool SetupWindowSizes(void);

    void ModifySubPlotSize(int zDelta, const ScreenPoint_t *screenPoint_p,
                           bool *invalidate_p = nullptr);
    void ZoomSubPlot_X_Axis(int zDelta, const ScreenPoint_t *screenPoint_p,
                            bool *invalidate_p = nullptr);
    void ZoomSubPlot_Y_Axis(int zDelta, const ScreenPoint_t *screenPoint_p,
                            bool *invalidate_p = nullptr);
    void ZoomSubPlot_Move(const ScreenPoint_t *screenPoint_p, bool *invalidate_p);

    void FillScreenPoint_FromDCViewPortPoint(QPoint *viewPortPoint_p,
                                             ScreenPoint_t *screenPoint_p);

    /* Based on the current subPlot sized their window positions are updated accordingly */
    void RealignSubPlots(void);

    void DeactivateSubPlot(const ScreenPoint_t *screenPoint_p);
    void ActivateAllSubPlots(void);

    void CreateShadow(const ScreenPoint_t *screenPoint_p);
    void InsertShadow(const ScreenPoint_t *screenPoint_p);

    CSubPlotSurface *GetSurfaceFromPoint(const ScreenPoint_t *screenPoint_p);

    void OnContextMenu(ScreenPoint_t& screenPoint);
    int GraphsObjectCount(void);

    void SetRow(const ScreenPoint_t *screenPoint_p);

    bool HandleKeyDown(QKeyEvent *e);
    void OnLButtonDown(ScreenPoint_t& screenPoint);
    void OnLButtonUp(void);

    void DrawToolTip(void);
    bool OpenToolTip(void);
    void CloseToolTip(void);

    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE;

public slots:
    void onToolTipTimer(void);

private:
    CPlot *m_plot_p;
    QList<CSubPlotSurface *> m_surfaces; /* These are the currently active subplots, including shadows */
    QList<CSubPlotSurface *> m_surfaces_Deactivated; /* These are the plots that has been deactivated */
    /* This list contains shadows, which are subplots originating from other plots */
    QList<CSubPlotSurface *> m_surfaceShadows;
    QList<int> m_keyPressedList; /* store the pressed/released keys */
    std::unique_ptr<QTimer> m_toolTipTimer;
    ToolTipState m_toolTipState;

    /* Contains tooltip strings and extra info that plugins might provide */
    QList<QString> m_toolTipStrings;
    bool m_toolTipEnabled;
    FontItem_t *m_tooltipFont_p;
    FontItem_t *m_FontEmptyWindow_p;
    bool m_subPlot_Moving;
    QPoint m_subPlot_StartMovePoint;
    QPoint m_subPlot_LastUpdateStartMovePoint;
    CSubPlotSurface *m_subPlot_inFocus_p;
    bool m_surfacesInitialized;
    bool m_surfacesCreated;
    QRect m_windowRect;   /* The window coordinates on screen, Client */
    /* The visible window in which graphs can be put (space for scrollbar is removed) */
    QRect m_plotWindowRect;
    QRect m_totalDQRect;
    bool m_restoreSubPlotSize;
    LS_Painter *m_pDC;
    QBrush *m_bgBoarderBrush_p;
    QPen *m_bgBoarderPen_p;
    ScreenPoint_t m_lastCursorPos;
    bool m_inDestructor;
    bool m_inFocus;
    CSubPlotSurface *m_surfaceFocus_p;  /* The current subPlot in focus, set to nullptr when focus is lost */
    QRect m_vscrollFrame; /* size of the scroll frame on the right */
    QRect m_vscrollSlider;  /* the scroll slider itself */
    int m_vscrollSlider_Height; /* A relation between the total DC and the area presented */
    bool m_vscrollSliderGlue;

    /* In pixels, where on the slider did the slider movement start... maintain this offset */
    int m_vscrollSliderGlueOffset;
    bool m_vscrollSliderActive;
    QRect m_hscrollFrame;   /* size of the scroll frame on the right */
    QRect m_hscrollSlider;   /* the scroll slider itself */
    int m_hscrollSlider_Width;  /* A relation between the total DC and the area presented */
    bool m_hscrollSliderGlue;

    /* In pixels, where on the slider did the slider movement start... maintain this offset */
    int m_hscrollSliderGlueOffset;
    bool m_hscrollSliderActive;
    int m_vbmpOffset;
    double m_min_X;
    double m_max_X;
    double m_zoom_left;
    double m_zoom_right;
    double m_offset_X;

    /* Relative Y position, determine the relation between the rcClient window and */
    double m_vrelPos;
    double m_hrelPos;
    bool m_lmousebutton;  /* true when left mouse button is pressed */
};
