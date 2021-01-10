/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stdlib.h>
#include <QWidget>
#include <QPen>
#include <QPainter>

#include "CDebug.h"
#include "CConfig.h"
#include "plugin_api.h"
#include "plugin_utils_internal.h"
#include "CThread.h"
#include "utils.h"

#define MAX_NUM_OF_PENS 100 /* This is the max number of colors/patterns that LogScrutinizer will create */
#define MAX_NUM_OF_GRAPH_COLORS 10

#define BOARDER_WIDTH 10
#define BOARDER_HEIGHT 10
#define AXIS_DECORATION_WIDTH 10
#define AXIS_DECORATION_HIGHT 10

#define MIN_WINDOW_HEIGHT 60 /*pixels */
#define MAX_WINDOW_HEIGHT 2048 /*pixels */
#define DEFAULT_WINDOW_HEIGHT 80 /*pixels */

/* The CSubPlotSurface is only used to manage the subplots, drawing it etc. */

#define X_L 0x01  /* X left */
#define X_R 0x02  /* X right */
#define Y_T 0x04  /* Y top */
#define Y_B 0x08  /* Y bottom */
#define X_C 0x10  /* X center  (in the middle/surface) */
#define Y_C 0x20  /* Y center  (in the middle/surface) */
#define ALL_CENTER (X_C | Y_C)

typedef enum {
    PL_YT_XL_en = Y_T | X_L,
    PL_YC_XL_en = Y_C | X_L,
    PL_YB_XL_en = Y_B | X_L,
    PL_YT_XC_en = Y_T | X_C,
    PL_YC_XC_en = Y_C | X_C,
    PL_YB_XC_en = Y_B | X_C,
    PL_YT_XR_en = Y_T | X_R,
    PL_YC_XR_en = Y_C | X_R,
    PL_YB_XR_en = Y_B | X_R
}PointLocation_e;

typedef enum {
    RenderMode_Maximized_en,
    /* The sub-plot is shown in full */
    RenderMode_Maximized_Pending_en,
    /* The sub-plot desires to be shown full, however because of small plotWnd it is (minimized) */
    RenderMode_Minimized_en             /* Use decision to minimize the sub-plot */
}RenderMode_e;

typedef struct
{
    QPen *pen_p;
    Q_COLORREF color;
    GraphLinePattern_e pattern;
}PendDescription_t;

typedef struct
{
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double x_offset;
    double y_offset;
}SurfaceZoom_t;

typedef struct {
    int size;
}labelItem_t;

/* This structure contains the dynamic information about the graphical objects, e.g. their current position. This
 * representation "instance" also allows
 * multiple displays of the same object in different window. The go_p points to a "base" type, however the object may be
 * of any other kind.
 * The CSubPlotSurface has to create one displayItem for each graphical object, and put these in an array for the
 * specific graph where they belong. */

typedef struct {
    GraphicalObject_t *go_p;
    int x1_pix;
    int y1_pix;
    int x2_pix;
    int y2_pix;
    char *label_p;
    int16_t labelLength;
    int16_t label_pix_length;
    int16_t properties;
}displayItem_t;

/***********************************************************************************************************************
*   CDisplayGraph
***********************************************************************************************************************/
class CDisplayGraph
{
public:
    CDisplayGraph() {
        m_numOfItems = 0;
        m_items_a = nullptr;
        m_isOverrideColorSet = false;
        m_overrideColor = 0;
        m_overrideLinePattern = GLP_NONE;
    }

    ~CDisplayGraph() {
        Empty();
    }

    /****/
    void Empty(void) {
        if ((m_numOfItems > 0) && (m_items_a != nullptr)) {
            free(m_items_a);
        }
    }

    CGraph_Internal *m_graph_p;
    int m_numOfItems;
    displayItem_t *m_items_a;
    QPen *m_pen_p;
    Q_COLORREF m_color; /* int m_color; / * Enumerated, may be overriden by m_overrideColor * / */
    GraphLinePattern_e m_pattern; /* Enumerated, may be overriden by m_overrideLinePattern */
    bool m_isOverrideColorSet; /* If the member m_overrideColor is set or not */
    Q_COLORREF m_overrideColor; /* 0 default */
    GraphLinePattern_e m_overrideLinePattern; /* GLP_NONE default */
};

/* ------- */

enum class TimePeriod {
    NA = -1,
    Year = 0,
    Month = 1,
    Week = 2,
    Day = 3,
    HalfDay = 4,
    Hour = 5,
    HalfHour = 6,
    TenMin = 7,
    Min = 8,
    HalfMin = 9,
    Sec = 10,
    HalfSec = 11,
    Milli100 = 12,
    Milli10 = 13,
    Milli = 14
};

const int64_t SEC_PER_HALFMIN = 30;
const int64_t SEC_PER_MIN = 60;
const int64_t SEC_PER_TEN_MIN = SEC_PER_MIN * 10;
const int64_t SEC_PER_HALF_HOUR = SEC_PER_MIN * 30;
const int64_t SEC_PER_HOUR = SEC_PER_MIN * 60;
const int64_t SEC_PER_HALFDAY = SEC_PER_HOUR * 12;
const int64_t SEC_PER_DAY = SEC_PER_HOUR * 24;
const int64_t SEC_PER_WEEK = SEC_PER_DAY * 7;
const int64_t SEC_PER_MONTH = SEC_PER_WEEK * 4;
const int64_t SEC_PER_YEAR = SEC_PER_MONTH * 12;

typedef struct {
	int recSkips[10] = { 1 };
	int length = 1;
}skipConfig_t;

typedef struct {
    double msecInPeriod = -1;
    TimePeriod period;
    int idx;
	const skipConfig_t * skipConfig_p;

    bool micro_allowed = true;

    /*****  periodToStr **/
    QString periodToStr(void) {
        QString name;
        switch (period)
        {
            case TimePeriod::NA:
                name = "NA";
                break;

            case TimePeriod::Year:
                name = "Year";
                break;

            case TimePeriod::Month:
                name = "Month";
                break;

            case TimePeriod::Week:
                name = "Week";
                break;

            case TimePeriod::HalfDay:
                name = "Half-Day";
                break;

            case TimePeriod::Day:
                name = "Day";
                break;

            case TimePeriod::Hour:
                name = "Hour";
                break;

            case TimePeriod::HalfHour:
                name = "Half-Hour";
                break;

            case TimePeriod::TenMin:
                name = "10 Minute";
                break;

            case TimePeriod::Min:
                name = "Minute";
                break;

            case TimePeriod::HalfMin:
                name = "Half-Minute";
                break;

            case TimePeriod::Sec:
                name = "Second";
                break;

            case TimePeriod::HalfSec:
                name = "HalfSecond";
                break;

            case TimePeriod::Milli100:
                name = "Milli100";
                break;

            case TimePeriod::Milli10:
                name = "Milli10";
                break;

            case TimePeriod::Milli:
                name = "Milli";
                break;

            default:
                name = "NA";
        }
        return name;
    }
}TimeConfig_t;

const skipConfig_t skip_1_5_10_15_30 = { {4, 9, 14, 29}, 4 };
const skipConfig_t skip_1_5_10_100 = { {4, 9, 99}, 3};
const skipConfig_t skip_1_3_6_12 = { {2, 5, 11}, 3};
const skipConfig_t skip_1_2_4_8 = { {1, 3, 7}, 3};
const skipConfig_t skip_1_2_7_14 = { {1, 6, 13}, 3};

const TimeConfig_t allowedTimePeriods[] =
{
    {SEC_PER_YEAR * 1000.0, TimePeriod::Year, 0, &skip_1_5_10_100}
    , {SEC_PER_MONTH * 1000.0, TimePeriod::Month, 1, &skip_1_3_6_12}
    , {SEC_PER_WEEK * 1000.0, TimePeriod::Week, 2, &skip_1_2_4_8}
    , {SEC_PER_DAY * 1000.0, TimePeriod::Day, 3, &skip_1_2_7_14}
    , {SEC_PER_HALFDAY * 1000.0, TimePeriod::HalfDay, 4, nullptr, false}
    , {SEC_PER_HOUR * 1000.0, TimePeriod::Hour, 5, &skip_1_3_6_12}
    , {SEC_PER_HALF_HOUR * 1000.0, TimePeriod::HalfHour, 6, nullptr, false}
    , {SEC_PER_TEN_MIN * 1000.0, TimePeriod::TenMin, 7, &skip_1_3_6_12, false}
    , {SEC_PER_MIN * 1000.0, TimePeriod::Min, 8, &skip_1_5_10_15_30}
    , {SEC_PER_HALFMIN * 1000.0, TimePeriod::HalfMin, 9, nullptr, false}
    , {1 * 1000, TimePeriod::Sec, 10, &skip_1_5_10_15_30}
    , {1 * 500, TimePeriod::HalfSec, 11, nullptr, false}
    , {100.0, TimePeriod::Milli100, 12, &skip_1_5_10_100}
    , {10.0, TimePeriod::Milli10, 13, &skip_1_5_10_100}
    , {1.0, TimePeriod::Milli, 14, &skip_1_5_10_100}
};

const auto MAX_ALLOWED_TIME_PERIODS = static_cast<int>(sizeof(allowedTimePeriods) / sizeof(allowedTimePeriods[0]));

/* A table is created matching the allowedTimePeriods, such that it becomes possible to
 * figure out how large a string becomes */
typedef struct PixelLength {
    int majorWidth;
    int majorMaxWordCount;
    int minorWidth;
    int minorMaxWordCount;

	PixelLength() : majorWidth(0), majorMaxWordCount(0), minorWidth(0), minorMaxWordCount(0) {}

} PixelLength_t;

typedef struct XLineConfig {
    TimeConfig_t anchorCfg;
    std::tm anchorTime;
    TimeConfig_t majorCfg;
    TimeConfig_t minorCfg;
    TimeConfig_t microCfg;

    /* Ctor */
    XLineConfig() {
        memset(&anchorTime, 0, sizeof(anchorTime));
        anchorTime.tm_mday = 1;
        majorCfg.period = TimePeriod::NA;
        minorCfg.period = TimePeriod::NA;
        microCfg.period = TimePeriod::NA;
    }
} XLineConfig_t;

/***********************************************************************************************************************
*   CSubPlotSurface
***********************************************************************************************************************/
class CSubPlotSurface
{
public:
    explicit CSubPlotSurface(CSubPlot *subPlot_p, CPlot *parentPlot_p, bool shadow = false);
    virtual ~CSubPlotSurface();

    bool isPaintable(void);

    void OnPaint_Empty(void);
    void OnPaint_1(void);
    void OnPaint_2(void);
    void SetZoom(double x_min, double x_max, bool reset_Y_Zoom = false);

    const displayItem_t *GetCursorRow(const QPoint *point_p, int *row_p, double *time,
                                      double *distance_p);
    bool GetClosestGraph(QPoint *point_p, CGraph_Internal **graph_pp, double *distance_p,
                         GraphicalObject_t **go_pp);
    bool GetClosestGraph(int row, CGraph_Internal **graph_pp, int *distance_p,
                         GraphicalObject_t **go_pp);

    void SetCursor(double cursorTime);
    void DrawCursorTime(void);

    void DisableCursor(void);

    void GetWindowRect(QRect *windowRect_p) {*windowRect_p = m_DC_windowRect;}
    void GetViewPortRect(QRect *viewPortRect_p) {*viewPortRect_p = m_DC_viewPortRect;}
    void SurfaceReconfigure(QRect *windowRect_p);

    void GetMaxExtents(GraphicalObject_Extents_t *extents_p) {m_subPlot_p->GetExtents(extents_p);}

    CSubPlot *GetSubPlot(void) {return m_subPlot_p;}

    void SetFocus(bool hasFocus) {m_hasFocus = hasFocus;}
    bool GetFocus(void) {return m_hasFocus;}

    void GetSurfaceZoom(SurfaceZoom_t *zoom_p) {*zoom_p = m_surfaceZoom;}
    void SetSurfaceZoom(SurfaceZoom_t *zoom_p);
    int GetObjectCount(void) const noexcept;

    /****/
    void GetUnitsPerPixel(double *unitsPerPixel_X_p, double *unitsPerPixel_Y_p,
                          double *unitsPerPixel_X_inv_p, double *unitsPerPixel_Y_inv_p)
    {
        *unitsPerPixel_X_p = m_unitsPerPixel_X;
        *unitsPerPixel_Y_p = m_unitsPerPixel_Y;
        *unitsPerPixel_X_inv_p = m_unitsPerPixel_X_inv;
        *unitsPerPixel_Y_inv_p = m_unitsPerPixel_Y_inv;
    }

    /* cfgStopIndex, the last including index. If -1 then num of items. If cfgStopIndex is 0 then it is only the first
     * object that is used  used by extern c - function, must be public */
    void SetupGraph(CDisplayGraph *dgraph_p, int cfgStartIndex = -1, int cfgStopIndex = -1);

    /* Make sure to call re-configure if the window size changes, and call re-configure before this function */
    void CreatePainter(QWidget *widget_p);
    void DestroyPainter(void);
    void BitBlit(QPainter *pDC);

    void ForceRedraw(void) {m_setupGraphs = true;}
    void SetRenderMode(RenderMode_e renderMode) {m_renderMode = renderMode;}
    void SetPluginSupportedFeatures(Supported_Features_t features) {
		m_features = features;
	}

    QImage& getImage(void) {return m_double_buffer_image;}

private:
    CSubPlotSurface() {TRACEX_E("CSubPlotSurface::CSubPlotSurface  Default constructor not supported")}

    const displayItem_t *FindClosest_GO(const int x, const int y,
                                        double *distance_p, CGraph_Internal **graph_pp = nullptr);
    GraphicalObject_t *FindClosest_GO(int row, int *distance_p, CGraph_Internal **graph_pp = nullptr);

    bool LoadResources(void);
    void Initialize(void);
    void Reconfigure(QRect *rect_p);

    void DrawBoarders(void);

    void DrawAxis_Over(void);
    void DrawAxis_Under(void);

    void Draw_Y_Axis_Graphs(void);
    void Draw_Y_Axis_Schedule(void);

    void Draw_X_Axis(void);
    bool Draw_X_Axis_UnixTime(void);

    void EnumerateUnixTimeStringLengths(void);
    void Draw_UnixTimeXLine(qint64 ts);
    void Draw_UniXTimeLabel(const QStringList& labels, qint64 ts);

    QString GetMilliSecsString(const QDateTime& dtime, int numDecimals);
    QStringList GetMajorStrings(const QDateTime& dtime,
                                const TimePeriod& period,
                                bool isAnchor = false,
                                int appendEmptyCount = 1);
    QStringList GetMinorStrings(const QDateTime& dtime, const TimePeriod& period);
    bool StepTime(QDateTime& dtime, const TimePeriod period, const bool forward = true);
    void DrawGraphs(void);
    void DrawDecorators(bool over = true);  /* if over is set the drawDecorators are called */

    inline void GetPointLocation(GraphicalObject_t *go_p, int *pl_1_p, int *pl_2_p);
    inline void Setup_Y_Lines(void);
    inline void Setup_X_Lines(void);

    void SetupGraphPens(void);
    void SetupGraphs(void);

    QPen *GetUserDefinedPen(Q_COLORREF color, GraphLinePattern_e pattern = GLP_NONE);

    void SetupDecorators(void);

    bool Intersection_LINE_Out2In(int *x_old_p, int *y_old_p, int x, int y);

    /* The two function Intersection_LINE_Out2In and Intersection_LINE_In2Out will determine how the intersection
     * functions will be called, as the intersection needs to be against one of the axises. */

    bool Intersection_LINE_Out2In(int pl_0, /* POINT LOCATION - which square the line starts in  */
                                  double *p0_x_p, double *p0_y_p, /* p0 is outside, need to be adjusted */
                                  double p1_x, double p1_y); /* p1 is inside */

    bool Intersection_LINE_In2Out(int pl_1, /* POINT LOCATION - which square the line ends in */
                                  double *p0_x_p, double *p0_y_p,   /* p0 is inside */
                                  double p1_x, double p1_y); /* p1 is outside, need to be adjusted */

    bool Intersection_LINE(double p0_x, double p0_y, double p1_x, double p1_y, /* Line 1:   p0 -> p1 */
                           double p2_x, double p2_y, double p3_x, double p3_y, /* Line 2:   p2 -> p2 */
                           double *i_x, double *i_y); /* Intersection */

    /* p0 -> p1 */
    bool Intersection_BOX_Out2In(int pl_0, double *p0_x_p, double *p0_y_p);
    bool Intersection_BOX_In2Out(int pl_1, double *p1_x_p, double *p1_y_p);

public:

    QRect m_deactivedIconPos; /* when the subplot is deativated its icon is shown somewhere */
    CSubPlot *m_subPlot_p;
    CPlot *m_parentPlot_p;
    char m_subPlotTitle[CFG_TEMP_STRING_MAX_SIZE]; /* Initialized at construction time (once) */
    int m_subPlotTitle_len;
    QSize m_subPlotTitle_Size;
    char m_parentPlotTitle[CFG_TEMP_STRING_MAX_SIZE];
    int m_parentPlotTitle_len;
    QSize m_parentPlotTitle_Size;
    char m_Y_Label[CFG_TEMP_STRING_MAX_SIZE];
    int m_Y_Label_len;
    QSize m_Y_Label_Size;
    char m_X_Label[CFG_TEMP_STRING_MAX_SIZE];
    int m_X_Label_len;
    QSize m_X_Label_Size;
    int m_numOfDisplayGraphs; /* number of items in m_displayGraphs_a */
    CDisplayGraph *m_displayGraphs_a; /* Array of displayGraph_t, allocated at construction */
    CDisplayGraph m_displayDecorator; /* Possibly there is a decorator */
    bool m_shadow; /* A shadow is a unique instance having a the m_subPlot_p referencing to some others
                    * m_subPlot_p. As such, this CSubPlotSurface doesn't own the m_subPlot_p */
    RenderMode_e m_renderMode;
    Supported_Features_t m_features; /* From plugin */

private:

    SurfaceZoom_t m_surfaceZoom;

#define MAX_Y_LINES 1080
#define MAX_X_LINES 640

    double m_lines_Y[MAX_Y_LINES];
    int m_numOf_Y_Lines;
    double m_lines_X[MAX_X_LINES];
    int m_base_X_right_count; /* Number of x-values from 0 index that is to the right of base */
    int m_numOf_X_Lines;
    bool m_cursorRowEnabled;
    int m_cursorRow;
    bool m_cursorTimeEnabled;
    double m_cursorTime;
    LS_Painter m_painter;
    LS_Painter *m_painter_p;                /* Valid only during OnPaint */
    QImage m_double_buffer_image;
	std::vector<PixelLength_t> m_maxLengthArray; // (MAX_ALLOWED_TIME_PERIODS);

    /* The Normalized window is used such that painting to a temporary bitmap is always from top,left at point 0,0...
     * even if the subPlot is located
     * differently in the total bitmap which will represent the entire Plot. Hence the difference between
     * m_DC_windowRect and m_windowRect. Where m_DC_x
     * means the location within the total DC Bitmap, and m_windowRect is the normalized */
    QRect m_DC_windowRect; /* The window (coordinates) within the DC Bitmap, including the boarders */

public:

    QRect m_DC_viewPortRect; /* The viewPort (coordinates) within the DC Bitmap, exluding boarders */

private:

    QRect m_windowRect; /* Normalized window, where x1,y1 is always 0, and the width is as
                         * m_DC_windowRect. The last known window extent coordinates (includes all but the
                         * viewPort) */
    QRect m_viewPortRect; /* Normalized, The window in wich the graphs is drawn (exludes the axis and
                           * boarders) */
    bool m_zoomEnabled;
    double m_unitsPerPixel_X; /* m_plotExtents_ViewPort.x_max - m_plotExtents_ViewPort.x_min /
                               * m_plotExtents_ViewPort.width */
    double m_unitsPerPixel_Y; /* m_plotExtents_ViewPort.x_max - m_plotExtents_ViewPort.x_min /
                               * m_plotExtents_ViewPort.width */
    double m_unitsPerPixel_X_inv; /* 1 / m_horizUnitPerPixel */
    double m_unitsPerPixel_Y_inv; /* 1 / m_vertUnitPerPixel */
    double m_viewPort_Width; /* Initially m_plotExtents.x_max - m_plotExtents.x_min / 2 */
    double m_viewPort_Height; /* Initially m_plotExtents.y_max - m_plotExtents.y_min / 2 */
    double m_viewPort_X_Center;
    double m_viewPort_Y_Center;
    QBrush *m_bgBrush_p;
    QPen *m_bgPen_p;
    PendDescription_t *m_graphPenArray_p;
    int m_graphPenArraySize;
    ColorTableItem_t *m_colorTable_p;
    int m_graphColors;
    PendDescription_t m_graphPenArrayUser[MAX_NUM_OF_PENS]; /* These are pens with colors chosen by plugin
                                                             * (additional pens) */
    int m_numOfGraphUserColors; /* The current number of pens created */
    QPen *m_linePen_Y_p;
    QPen *m_cursorPen_Y_p;
    QImage m_bitmap_top;
    QImage m_bitmap_bottom;
    QImage m_bitmap_left;
    QImage m_bitmap_right;
    QImage m_bitmap_right_top;
    QImage m_bitmap_right_bottom;
    QImage m_bitmap_left_top;
    QImage m_bitmap_left_bottom;
    bool m_resourcesLoaded;
    bool m_setupGraphs; /* Set to true when it is required to call SetupGraphs, shall be called from OnPaint */
    CGO_Label **m_label_refs_a;
    int m_numOfLabelRefs;
    QSize m_lineSize; /*updated each time when drawing axis            = m_pDC->GetTextExtent("X", 1); */
    int m_halfLineHeight;     /*updated each time when drawing axis            = (int)((double)lineSize.cy /
                               * (double)2.0); */
    int m_subplot_properties; /*updated each time when drawing axis = * m_subPlot_p->GetProperties(); */
    double m_avgPixPerLetter;
    double m_avgPixPerLetterHeight;
    bool m_hasFocus;
};
