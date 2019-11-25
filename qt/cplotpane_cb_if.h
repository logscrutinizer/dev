/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CPLOTPLANE_CB_IF
#define CPLOTPLANE_CB_IF

#include "../qt/cplotpane.h"
#include "../qt/cplotwidget.h"

extern CPlotPane *CPlotPane_GetPlotPane(void);
extern void CPlotPane_Align_X_Zoom(double x_min, double x_max);
extern void CPlotPane_Align_Reset_Zoom(void);
extern void CPlotPane_Align_X_Cursor(double cursorTime, double x_min, double x_max);
extern void CPlotPane_ZoomSubPlotInFocus(bool in /*or out*/, bool horizontal);
extern void CPlotPane_ResizeSubPlotInFocus(bool in /*or out*/);
extern void CPlotPane_RestoreSubPlotSizes(void);
extern void CPlotPane_RemoveRelatedShadowSubPlots(CPlotWidgetInterface *plotWidgetSrc_p);
extern void CPlotPane_SetPlotFocus(CPlot *plot_p);
extern CPlot *CPlotPane_GetPlotFocus(void);
extern bool CPlotPane_NextPlot(bool backward);

#endif
