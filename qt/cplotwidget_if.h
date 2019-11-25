/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

/*#include "csubplotsurface.h" */
#include "plugin_api.h"
#include "CDebug.h"
#include "CConfig.h"
#include "utils.h"

class CSubPlot;
class CSubPlotSurface;

#define ZOOM_STEP                   0.1

/* Abstract class, just a interface definition */
class CPlotWidgetInterface
{
public:
    virtual ~CPlotWidgetInterface();

    virtual void RemoveSurfaces(void) = 0;
    virtual CPlot *GetPlotRef(void) = 0;
    virtual int GetMinSize_Y(void) = 0;
    virtual QRectF GetMinMax(void) = 0;
    virtual void SetMinMax(QRectF& rect) = 0;
    virtual void SetUpdated(CSubPlot *subPlot_p) = 0;
    virtual void RemoveShadowSubPlotsMatching(QList<CSubPlotSurface *> *matchSurfaces) = 0;
    virtual void GetSurfaces(QList<CSubPlotSurface *> **active, QList<CSubPlotSurface *> **deactivated) = 0;
    virtual void Initialize(void) = 0;
    virtual void Redraw(void) = 0;
    virtual QString GetTitle(void) = 0;
    virtual void SurfaceToClipBoard(const ScreenPoint_t *screenPoint_p) = 0;
    virtual bool GetClosestGraph(ScreenPoint_t *screenPoint_p, CGraph_Internal **graph_pp,
                                 GraphicalObject_t **go_pp, CSubPlot **subPlot_pp) = 0;
    virtual bool GetClosestGraph(int row, CGraph_Internal **graph_pp, GraphicalObject_t **go_pp,
                                 CSubPlot **subPlot_pp) = 0;
    virtual bool GetClosest_GO(int row, GraphicalObject_t **go_pp, int *distance_p) = 0;
    virtual void SetFocusTime(const double time) = 0;
    virtual bool isInitialized(void) = 0;
    virtual void SetUpdateSubplots(void) = 0;
    virtual bool isSurfaceInList(QList<CSubPlotSurface *> *list_p, CSubPlotSurface *surface_p) = 0;
    virtual void SetSurfaceFocus(CSubPlot *subPlot_p) = 0;
    virtual bool SearchFocus(CSubPlot **subPlot_pp, CSubPlotSurface **surface_pp) = 0;
    virtual bool SearchSubPlot(CSubPlot *subPlot_p, CSubPlotSurface **surface_pp) = 0;
    virtual void ResetFocus(void) = 0; /* Go through all subPlots and set the focus flag to false */
    virtual void MoveSubPlot(QList<CSubPlotSurface *> *to_list_p, QList<CSubPlotSurface *> *from_list_p,
                             CSubPlotSurface *moveSubPlot_p) = 0;
    virtual void RemoveShadowSubPlot(CSubPlotSurface *subPlot_p) = 0;
    virtual void Align_X_Zoom(double x_min, double x_max) = 0;
    virtual void Align_Reset_Zoom(double x_min, double x_max) = 0;
    virtual void Align_X_Cursor(double cursorTime, double x_min, double x_max) = 0;
    virtual void ZoomSubPlotInFocus(bool in, bool horizontal) = 0;
    virtual void ResizeSubPlotInFocus(bool increase) = 0;
    virtual void RestoreSubPlotSizes(void) = 0;
    virtual void ZoomRestore(void) = 0;
};
