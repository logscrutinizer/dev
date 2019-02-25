/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QWidget>
#include <QTabWidget>

#include "plugin_api.h"
#include "cplotwidget.h"
#include "csubplotsurface.h"
#include "CThread.h"

extern QPen *g_plotWnd_focusPen_p;
extern QPen *g_plotWnd_passiveFocusPen_p;   /* In-case the sub-plot has focus but not the PlotWnd */
extern QPen *g_plotWnd_defaultPen_p;
extern QPen *g_plotWnd_defaultDotPen_p;
extern FontItem_t *g_plotWnd_GrayFont_p;
extern FontItem_t *g_plotWnd_BlackFont_p;
extern FontItem_t *g_plotWnd_WhiteFont_p;
extern CThreadManager *g_CPlotPane_ThreadMananger_p;
extern CSubPlotSurface *g_currentShadowCopy; /* Used as temp storage for copying */

/***********************************************************************************************************************
*   CPlotPane
***********************************************************************************************************************/
class CPlotPane : public QTabWidget
{
public:
    CPlotPane(QWidget *parent = nullptr);

    CPlotWidgetInterface *addPlot(const char *plotName_p, CPlot *plot_p);
    void removePlot(CPlot *plot_p);
    void setPlotFocus(CPlot *plot_p);
    void setSubPlotFocus(CSubPlot *subPlotFocus_p);
    void getPlotFocus(CPlotWidgetInterface **plotWidget_pp,
                      CPlot **plot_pp,
                      CSubPlot **subPlot_pp = nullptr,
                      CSubPlotSurface **surface_pp = nullptr);

    CPlotWidgetInterface *getPlotWidget(CPlot *plot_p);
    void storeSettings(void);

    void resetPlotFocus(void); /* Remove the focus flag from the current subPlot in focus */
    void nextTab(bool backward = false);
    void cleanAllPlots(void);

    bool plotExists(void) {return (m_plotWnds.isEmpty() ? false : true);}
    bool isPlotsActive(void);

    void setPlotCursor(int row, int *cursorRow_p); /* Called when time isn't known, search for best
                                                    * matching row, cursorRow_p contains the best
                                                    * match */
    void setPlotCursor(const double time); /* Called when time IS known, apply as is */
    void setSubPlotUpdated(CSubPlot *subPlot_p);
    void redrawPlot(CPlot *plot_p);
    void setAdaptWindowSize(QSize& size) {m_adaptWindowSize = size;}

    virtual bool CanBeClosed() const {return false;}

    void removeRelatedShadowSubPlots(CPlotWidgetInterface *plotWidgetSrc_p); /* Remove all subplots that might be
                                                                              * shadows from this plotW */
    void align_X_Zoom(double x_min, double x_max);
    void align_Reset_Zoom(void);
    void align_X_Cursor(double cursorTime, double x_min, double x_max);

public:
    virtual ~CPlotPane() override;

protected:
    virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

private:
    QSize m_adaptWindowSize;
    QList<CPlotWidgetInterface *> m_plotWnds;
    CThreadManager m_threadManager;
};

extern CPlotPane *g_CPlotPane;
