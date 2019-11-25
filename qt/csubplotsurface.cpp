/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdlib.h>

#include "math.h"

#include "plugin_api.h"
#include "plugin_constants.h"

#include "csubplotsurface.h"
#include "CLogScrutinizerDoc.h"

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "CTimeMeas.h"

extern QPen *g_plotWnd_focusPen_p;
extern QPen *g_plotWnd_passiveFocusPen_p;
extern QPen *g_plotWnd_defaultPen_p;
extern QPen *g_plotWnd_defaultDotPen_p;
extern FontItem_t *g_plotWnd_GrayFont_p;
extern FontItem_t *g_plotWnd_BlackFont_p;

#define ARROW_TIP_ADJUST 1 /* Since the arrow otherwise ends up to far up on the life line */
#define NUM_OF_NO_SUBPLOT_STRINGS 7

const char NO_SUBPLOT_DATA[NUM_OF_NO_SUBPLOT_STRINGS][256] =
{
    "Nothing to paint",
    " ",
    "No graphical objects to paint, the Plot within the Plugin (dll) didn't produce any.",
    "Please check the documentation for the plugin and verify that the log contains",
    "the text items used to create the graphical objects (graphs etc). Further, typically",
    "a plugin is sensitive to how the strings are formatted and that the values are in the ",
    "required order and format"
};

#define GRAPH_RED             Q_RGB(231, 47, 39)
#define GRAPH_BLUE            Q_RGB(46, 20, 141)
#define GRAPH_GREEN           Q_RGB(19, 154, 47)
#define GRAPH_PURPLE          Q_RGB(178, 137, 166)
#define GRAPH_ORANGE          Q_RGB(238, 113, 25)
#define GRAPH_GRAY            Q_RGB(160, 160, 160)
#define GRAPH_DARK_RED_GRAY   Q_RGB(101, 55, 55)

#define ABS(A)    ((A) > 0 ? (A) : (-1 * (A)))

extern CThreadManager *g_CPlotPane_ThreadMananger_p;

#define Y_ADJUST 3.0 /* Unknown why it looks like text is a bit under the center line */

typedef struct {
    CSubPlotSurface *subplotSurface_p;
    CDisplayGraph *dgraph_p;
    int startIndex;
    int stopIndex;
} SetupGraph_Data_t;

static SetupGraph_Data_t g_setupGraph_WorkData[MAX_NUM_OF_THREADS];
void SetupGraph_ThreadAction(void *data_p);

const QString g_avg_str("XXXXXXxxxxxZZZZzzzzz");   /* used to calculate size of text in a box/line */

CSubPlotSurface::CSubPlotSurface(CSubPlot *subPlot_p, CPlot *parentPlot_p, bool shadow)
    :
    m_deactivedIconPos(0, 0, 0, 0), m_subPlot_p(subPlot_p), m_parentPlot_p(parentPlot_p), m_subPlotTitle_len(0),
    m_subPlotTitle_Size(0, 0), m_parentPlotTitle_len(0), m_parentPlotTitle_Size(0, 0), m_Y_Label_len(0),
    m_Y_Label_Size(0, 0), m_X_Label_len(0), m_X_Label_Size(0, 0),
    m_numOfDisplayGraphs(0), m_displayGraphs_a(nullptr), m_shadow(shadow), m_renderMode(RenderMode_Maximized_en),
    m_numOf_Y_Lines(0), m_numOf_X_Lines(0), m_cursorRowEnabled(false), m_cursorRow(0), m_cursorTimeEnabled(false),
    m_cursorTime(0.0), m_painter_p(nullptr), m_DC_windowRect(0, 0, 0, 0), m_DC_viewPortRect(0, 0, 0, 0),
    m_windowRect(0, 0, 0, 0), m_viewPortRect(0, 0, 0, 0), m_zoomEnabled(false), m_unitsPerPixel_X(0.0),
    m_unitsPerPixel_Y(0.0), m_unitsPerPixel_X_inv(0.0), m_unitsPerPixel_Y_inv(0.0),
    m_viewPort_Width(0.0), m_viewPort_Height(0.0), m_viewPort_X_Center(0.0), m_viewPort_Y_Center(0.0),
    m_bgBrush_p(nullptr), m_bgPen_p(nullptr), m_graphPenArray_p(nullptr), m_graphPenArraySize(0),
    m_colorTable_p(nullptr), m_graphColors(0), m_numOfGraphUserColors(0),
    m_linePen_Y_p(nullptr), m_cursorPen_Y_p(nullptr), m_resourcesLoaded(false), m_setupGraphs(false),
    m_label_refs_a(nullptr), m_numOfLabelRefs(0), m_lineSize(0, 0), m_halfLineHeight(0), m_subplot_properties(0),
    m_avgPixPerLetter(0), m_avgPixPerLetterHeight(0.0),
    m_hasFocus(false)
{
    m_surfaceZoom.x_max = 0.0;
    m_surfaceZoom.x_min = 0.0;
    m_surfaceZoom.x_offset = 0.0;
    m_surfaceZoom.y_max = 0.0;
    m_surfaceZoom.y_min = 0.0;
    m_surfaceZoom.y_offset = 0.0;

    char *title_p;
    char *subTitle_p;
    char *X_AxisLabel_p;
    char *Y_AxisLabel_p;

    m_subPlot_p->GetTitle(&subTitle_p, &Y_AxisLabel_p);
    m_parentPlot_p->GetTitle(&title_p, &X_AxisLabel_p);

    strcpy_s(m_subPlotTitle, CFG_TEMP_STRING_MAX_SIZE, subTitle_p);
    m_subPlotTitle_len = static_cast<int>(strlen(m_subPlotTitle));

    strcpy_s(m_parentPlotTitle, CFG_TEMP_STRING_MAX_SIZE, title_p);
    m_parentPlotTitle_len = static_cast<int>(strlen(m_parentPlotTitle));

    strcpy_s(m_Y_Label, CFG_TEMP_STRING_MAX_SIZE, Y_AxisLabel_p);
    m_Y_Label_len = static_cast<int>(strlen(m_Y_Label));

    strcpy_s(m_X_Label, CFG_TEMP_STRING_MAX_SIZE, X_AxisLabel_p);
    m_X_Label_len = static_cast<int>(strlen(m_X_Label));

    if (g_cfg_p->m_pluginDebugBitmask != 0) {
        TRACEX_I("Adding subPlot:%s to the plot:%s ", m_subPlotTitle, m_parentPlotTitle)
    }

    /* Setup the label array for quick access */
    CList_LSZ *labelList_p;
    m_subPlot_p->GetLabels(&labelList_p);

    if ((labelList_p != nullptr) && (labelList_p->count() > 0)) {
        m_label_refs_a =
            reinterpret_cast<CGO_Label **>(malloc(sizeof(CGO_Label *) * static_cast<size_t>(labelList_p->count())));
        memset(m_label_refs_a, 0, sizeof(CGO_Label *) * static_cast<size_t>(labelList_p->count()));

        CGO_Label *label_p = reinterpret_cast<CGO_Label *>(labelList_p->first());
        int index = 0;
        while (label_p != nullptr) {
            m_label_refs_a[index++] = label_p;
            label_p = reinterpret_cast<CGO_Label *>(labelList_p->GetNext(label_p));
        }

        m_numOfLabelRefs = index;
    }

    if (g_cfg_p->m_pluginDebugBitmask != 0) {
        TRACEX_I("   NumOfLabels:%d", labelList_p->count())
    }

    Initialize();
    LoadResources();

    /* Transfer graphicalObject data to m_displayGraphs_a */

    m_numOfDisplayGraphs = 0;
    m_displayGraphs_a = nullptr;

    CList_LSZ *list_p;

    m_subplot_properties = m_subPlot_p->GetProperties();

    subPlot_p->GetGraphs(&list_p);
    m_numOfDisplayGraphs = list_p->count();

    if (m_numOfDisplayGraphs != 0) {
        m_displayGraphs_a = new CDisplayGraph[static_cast<size_t>(m_numOfDisplayGraphs)];

        CGraph_Internal *graph_p = reinterpret_cast<CGraph_Internal *>(list_p->first());
        int graphIndex = 0;
        int itemIndex = 0;

        while (graph_p != nullptr) {
#ifdef _DEBUG
            if (graphIndex >= m_numOfDisplayGraphs) {
                TRACEX_E("CSubPlotSurface::CSubPlotSurface  Graph object corrupt, "
                         "numOfItems doesn't match")
            }
#endif

            CDisplayGraph *dgraph_p = &m_displayGraphs_a[graphIndex];

            dgraph_p->m_numOfItems = graph_p->GetNumOfObjects();
            dgraph_p->m_graph_p = graph_p;

            graph_p->GetOverrides(&dgraph_p->m_isOverrideColorSet, &dgraph_p->m_overrideColor,
                                  &dgraph_p->m_overrideLinePattern);

            if (dgraph_p->m_numOfItems > 0) {
                itemIndex = 0;

                dgraph_p->m_items_a =
                    reinterpret_cast<displayItem_t *>(malloc(static_cast<size_t>(dgraph_p->m_numOfItems) *
                                                             sizeof(displayItem_t)));

                if (dgraph_p->m_items_a == nullptr) {
                    TRACEX_E("CSubPlotSurface::CSubPlotSurface  Out of memory "
                             "allocating dgraph_p->m_items_a")
                    return;
                }

                memset(dgraph_p->m_items_a, 0, static_cast<size_t>(dgraph_p->m_numOfItems) * sizeof(displayItem_t));

                GraphicalObject_t *go_p = graph_p->GetFirstGraphicalObject();
                const int numOfItems = dgraph_p->m_numOfItems;

                while (go_p != nullptr && itemIndex < numOfItems) {
                    dgraph_p->m_items_a[itemIndex].go_p = go_p;
                    dgraph_p->m_items_a[itemIndex].properties = go_p->properties;

                    go_p = graph_p->GetNextGraphicalObject();
                    ++itemIndex;
                }

#ifdef _DEBUG
                if (itemIndex > dgraph_p->m_numOfItems) {
                    TRACEX_E("CSubPlotSurface::CSubPlotSurface  Graph object corrupt, "
                             "numOfItems doesn't match")
                }
#endif
            }

            if (g_cfg_p->m_pluginDebugBitmask != 0) {
                GraphicalObject_Extents_t go_extents;

                graph_p->GetExtents(&go_extents);

                TRACEX_I("   Graph(%d) Items:%d CO:%d OP:%d\n xmin:%f xmax:%f "
                         "ymin:%f ymax:%f",
                         graphIndex, dgraph_p->m_numOfItems, dgraph_p->m_overrideColor,
                         dgraph_p->m_overrideLinePattern, go_extents.x_min, go_extents.x_max,
                         go_extents.y_min, go_extents.y_max)
            }

            graph_p = reinterpret_cast<CGraph_Internal *>(list_p->GetNext(graph_p));

            ++graphIndex;
        }
    }

    /* List of Decorators, each decorator contains one or several graphical objects. All similar to graphs */
    CDecorator *decorator_p;
    subPlot_p->GetDecorator(&decorator_p);

    if (decorator_p != nullptr) {
        CGraph_Internal *graph_p = decorator_p;
        CDisplayGraph *dgraph_p = &m_displayDecorator;

        dgraph_p->m_numOfItems = graph_p->GetNumOfObjects();
        dgraph_p->m_graph_p = graph_p;

        if (dgraph_p->m_numOfItems > 0) {
            dgraph_p->m_items_a =
                static_cast<displayItem_t *>(malloc(static_cast<size_t>(dgraph_p->m_numOfItems) *
                                                    sizeof(displayItem_t)));

            if (dgraph_p->m_items_a == nullptr) {
                TRACEX_E("CSubPlotSurface::CSubPlotSurface    dgraph_p->m_items_a is nullptr")
                return;
            }

            memset(dgraph_p->m_items_a, 0, static_cast<size_t>(dgraph_p->m_numOfItems) * sizeof(displayItem_t));

            GraphicalObject_t *go_p = graph_p->GetFirstGraphicalObject();
            int itemIndex = 0;
            const auto numOfItems = dgraph_p->m_numOfItems;

            while (go_p != nullptr && itemIndex < numOfItems) {
                dgraph_p->m_items_a[itemIndex].go_p = go_p;
                dgraph_p->m_items_a[itemIndex].properties = go_p->properties;

                go_p = graph_p->GetNextGraphicalObject();
                ++itemIndex;
            }
#ifdef _DEBUG
            if (itemIndex > numOfItems) {
                TRACEX_E("CSubPlotSurface::CSubPlotSurface  CDecorator,  "
                         "Graph object corrupt, numOfItems doesn't match")
            }
#endif
        }
    }

#ifdef _DEBUG
 #ifdef _WIN32
    (void)_CrtCheckMemory();  /*http://msdn.microsoft.com/en-us/library/z8h19c37(v=vs.71).aspx */
 #endif
#endif
}

CSubPlotSurface::~CSubPlotSurface()
{
    if (m_bgBrush_p != nullptr) {
        delete m_bgBrush_p;
        m_bgBrush_p = nullptr;
    }

    if (m_bgPen_p != nullptr) {
        delete m_bgPen_p;
        m_bgPen_p = nullptr;
    }

    if (m_graphPenArray_p != nullptr) {
        for (int index = 0; index < m_graphPenArraySize; ++index) {
            if (m_graphPenArray_p[index].pen_p != nullptr) {
                delete m_graphPenArray_p[index].pen_p;
            }
        }

        free(m_graphPenArray_p);
    }

    if (m_numOfGraphUserColors != 0) {
        for (int index = 0; index < m_numOfGraphUserColors; ++index) {
            if (m_graphPenArrayUser[index].pen_p != nullptr) {
                delete m_graphPenArrayUser[index].pen_p;
                m_graphPenArrayUser[index].pen_p = nullptr;
            }
        }
    }

    if (m_linePen_Y_p != nullptr) {
        delete m_linePen_Y_p;
        m_linePen_Y_p = nullptr;
    }

    if (m_cursorPen_Y_p != nullptr) {
        delete  m_cursorPen_Y_p;
        m_cursorPen_Y_p = nullptr;
    }

    if (m_label_refs_a != nullptr) {
        free(m_label_refs_a);
    }

    if (m_displayGraphs_a != nullptr) {
        delete[] m_displayGraphs_a;
        m_displayGraphs_a = nullptr;
        m_numOfDisplayGraphs = 0;
    }

#ifdef _DEBUG
 #ifdef _WIN32
    (void)_CrtCheckMemory();  /*http://msdn.microsoft.com/en-us/library/z8h19c37(v=vs.71).aspx */
 #endif
#endif
}

/***********************************************************************************************************************
*   SetZoom
***********************************************************************************************************************/
void CSubPlotSurface::SetZoom(double x_min, double x_max, bool reset_Y_Zoom)
{
    GraphicalObject_Extents_t extents;  /*TODO: */
    m_subPlot_p->GetExtents(&extents);

    if (!reset_Y_Zoom && almost_equal(m_surfaceZoom.x_max, x_max) && almost_equal(m_surfaceZoom.x_min, x_min)) {
        return;
    }

    if (reset_Y_Zoom) {
        double zoom_out = (extents.y_max - extents.y_min) * 0.2;

        m_surfaceZoom.y_max = extents.y_max + zoom_out;
        m_surfaceZoom.y_min = extents.y_min - zoom_out;
    }

    m_surfaceZoom.x_max = x_max;
    m_surfaceZoom.x_min = x_min;

    PRINT_SUBPLOTSURFACE("CSubPlotSurface::SetZoom  %s  x_min:%e x_max:%e",
                         m_subPlotTitle, x_min, x_max)

    Reconfigure(nullptr);
    m_setupGraphs = true;
}

/***********************************************************************************************************************
*   SetSurfaceZoom
***********************************************************************************************************************/
void CSubPlotSurface::SetSurfaceZoom(SurfaceZoom_t *zoom_p)
{
    if (almost_equal(zoom_p->x_max, m_surfaceZoom.x_max) &&
        almost_equal(zoom_p->x_min, m_surfaceZoom.x_min) &&
        almost_equal(zoom_p->y_max, m_surfaceZoom.y_max) &&
        almost_equal(zoom_p->y_min, m_surfaceZoom.y_min) &&
        almost_equal(zoom_p->x_offset, m_surfaceZoom.x_offset) &&
        almost_equal(zoom_p->y_offset, m_surfaceZoom.y_offset)) {
        return;
    }

    m_surfaceZoom = *zoom_p;
    Reconfigure(nullptr);
    m_setupGraphs = true;
}

/***********************************************************************************************************************
*   Initialize
***********************************************************************************************************************/
void CSubPlotSurface::Initialize(void)
{
    m_viewPort_Width = 0;
    m_viewPort_Height = 0.0;

    m_viewPort_X_Center = 0;
    m_viewPort_Y_Center = 0.0;

    m_unitsPerPixel_X = 0.0;
    m_unitsPerPixel_Y = 0.0;
    m_unitsPerPixel_X_inv = 1.0;
    m_unitsPerPixel_Y_inv = 1.0;

    m_surfaceZoom.x_max = 0;
    m_surfaceZoom.x_min = 0;
    m_surfaceZoom.y_max = 0.0;
    m_surfaceZoom.y_min = 0.0;

    m_cursorRowEnabled = false;
    m_cursorRow = 0;

    m_cursorTimeEnabled = false;
    m_cursorTime = 0.0;
}

/***********************************************************************************************************************
*   LoadResources
***********************************************************************************************************************/
bool CSubPlotSurface::LoadResources(void)
{
    bool success = true;

    m_bitmap_top = QImage(":IDB_BRUSH_TOP_BMP");
    if (m_bitmap_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_TOP_BMP load FAILED")
    }

    m_bitmap_bottom = QImage(":IDB_BRUSH_BOTTOM_BMP");
    if (m_bitmap_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_BOTTOM_BMP load FAILED")
    }

    m_bitmap_left = QImage(":IDB_BRUSH_LEFT_BMP");
    if (m_bitmap_left.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_LEFT_BMP load FAILED")
    }

    m_bitmap_right = QImage(":IDB_BRUSH_RIGHT_BMP");
    if (m_bitmap_right.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_RIGHT_BMP load FAILED")
    }

    m_bitmap_left_top = QImage(":IDB_BRUSH_LEFT_TOP_BMP");
    if (m_bitmap_left_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_LEFT_TOP_BMP "
                 "load FAILED")
    }

    m_bitmap_left_bottom = QImage(":IDB_BRUSH_LEFT_BOTTOM_BMP");
    if (m_bitmap_left_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_LEFT_BOTTOM_BMP "
                 "load FAILED")
    }

    m_bitmap_right_bottom = QImage(":IDB_BRUSH_RIGHT_BOTTOM_BMP");
    if (m_bitmap_right_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_RIGHT_BOTTOM_BMP "
                 "load FAILED")
    }

    m_bitmap_right_top = QImage(":IDB_BRUSH_RIGHT_TOP_BMP");
    if (m_bitmap_right_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_RIGHT_TOP_BMP "
                 "load FAILED")
    }

    m_resourcesLoaded = success;

    m_bgBrush_p = new QBrush(BACKGROUND_COLOR);

    m_bgPen_p = new QPen(BACKGROUND_COLOR);
    m_bgPen_p->setWidth(1);

    SetupGraphPens();

    return success;
}

/***********************************************************************************************************************
*   SetupGraphPens
***********************************************************************************************************************/
void CSubPlotSurface::SetupGraphPens(void)
{
    int index;

    if (m_graphPenArray_p != nullptr) {
        for (index = 0; index < m_graphPenArraySize; ++index) {
            if (m_graphPenArray_p[index].pen_p != nullptr) {
                delete m_graphPenArray_p[index].pen_p;
            }
        }

        free(m_graphPenArray_p);
    }

    m_graphPenArraySize = 0;

    if (m_linePen_Y_p != nullptr) {
        delete m_linePen_Y_p;
    }

    /*m_linePen_Y_p = new QPen(PS_SOLID, g_cfg_p->m_plot_GraphAxisLineSize, g_cfg_p->m_plot_GraphAxisLineColor); */
    m_linePen_Y_p = new QPen(static_cast<QRgb>(g_cfg_p->m_plot_GraphAxisLineColor));
    m_linePen_Y_p->setWidth(g_cfg_p->m_plot_GraphAxisLineSize);
    m_linePen_Y_p->setStyle(Qt::SolidLine);

    if (m_cursorPen_Y_p != nullptr) {
        delete m_cursorPen_Y_p;
    }

    /*m_cursorPen_Y_p = new QPen(PS_SOLID, g_cfg_p->m_plot_GraphCursorLineSize, g_cfg_p->m_plot_GraphCursorLineColor);
     * */
    m_cursorPen_Y_p = new QPen(static_cast<QRgb>(g_cfg_p->m_plot_GraphCursorLineColor));
    m_cursorPen_Y_p->setWidth(g_cfg_p->m_plot_GraphCursorLineSize);
    m_cursorPen_Y_p->setStyle(Qt::SolidLine);

#define MAX_GRAPH_TYPES 5

    m_graphColors = g_cfg_p->m_graph_ColorTable_p->GetCurrentNumberOfColors();
    m_colorTable_p = g_cfg_p->m_graph_ColorTable_p->GetTable();

    Qt::PenStyle graphLineType = Qt::SolidLine;
    GraphLinePattern_e graphLinePattern = GLP_SOLID;

    m_graphPenArray_p = reinterpret_cast<PendDescription_t *>(malloc(static_cast<size_t>(m_graphColors) *
                                                                     sizeof(PendDescription_t) * MAX_GRAPH_TYPES));

    if (m_graphPenArray_p == nullptr) {
        TRACEX_E("CSubPlotSurface::SetupGraphPens  Out of memory when allocating pens")
        return;
    }

    for (int graphLineTypeEnum = 0; graphLineTypeEnum < MAX_GRAPH_TYPES; ++graphLineTypeEnum) {
        switch (graphLineTypeEnum)
        {
            case 0:
                graphLineType = Qt::SolidLine;
                graphLinePattern = GLP_SOLID;
                break;

            case 1:
                graphLineType = Qt::DashLine;
                graphLinePattern = GLP_DASH;
                break;

            case 2:
                graphLineType = Qt::DotLine;
                graphLinePattern = GLP_DOT;
                break;

            case 3:
                graphLineType = Qt::DashDotLine;
                graphLinePattern = GLP_DASHDOT;
                break;

            case 4:
                graphLineType = Qt::DashDotDotLine;
                graphLinePattern = GLP_DASHDOTDOT;
                break;
        }

        for (int colorIndex = 0;
             colorIndex < m_graphColors && m_graphPenArraySize < MAX_GRAPH_TYPES;
             ++colorIndex, ++m_graphPenArraySize) {
            m_graphPenArray_p[m_graphPenArraySize].pen_p = new QPen(m_colorTable_p[colorIndex].color);
            m_graphPenArray_p[m_graphPenArraySize].pen_p->setWidth(g_cfg_p->m_plot_GraphLineSize);
            m_graphPenArray_p[m_graphPenArraySize].pen_p->setStyle(graphLineType);
            m_graphPenArray_p[m_graphPenArraySize].color = m_colorTable_p[colorIndex].color;
            m_graphPenArray_p[m_graphPenArraySize].pattern = graphLinePattern;
        }
    }

    m_numOfGraphUserColors = 0;
    memset(m_graphPenArrayUser, 0, sizeof(PendDescription_t) * MAX_NUM_OF_PENS);  /* Dynamically added pens */
}

/***********************************************************************************************************************
*   GetUserDefinedPen
***********************************************************************************************************************/
QPen *CSubPlotSurface::GetUserDefinedPen(Q_COLORREF color, GraphLinePattern_e pattern)
{
    int index;

    for (index = 0; index < m_numOfGraphUserColors; ++index) {
        if ((m_graphPenArrayUser[index].pen_p != nullptr) &&
            (m_graphPenArrayUser[index].color == color) &&
            (m_graphPenArrayUser[index].pattern == pattern)) {
            return m_graphPenArrayUser[index].pen_p;
        }
    }

    if (m_numOfGraphUserColors >= (MAX_NUM_OF_PENS - 1)) {
        /* Too many pens */
        TRACEX_W("CSubPlotSurface::GetUserDefinedPen  Too many plugin defined "
                 "colors (max is:%d), no more pens to use, instead uses color: %d",
                 MAX_NUM_OF_PENS, m_graphPenArrayUser[0].color)

        return m_graphPenArrayUser[0].pen_p;
    }

    Qt::PenStyle style = Qt::SolidLine;

    if (pattern != GLP_NONE) {
        switch (pattern)
        {
            case GLP_DASH:
                style = Qt::DashLine;
                break;

            case GLP_DOT:
                style = Qt::DotLine;
                break;

            case GLP_DASHDOT:
                style = Qt::DashDotLine;
                break;

            case GLP_DASHDOTDOT:
                style = Qt::DashDotDotLine;
                break;

            default:
                break;
        }
    }

    /* No matching pen found, need to create one */
    m_graphPenArrayUser[m_numOfGraphUserColors].pen_p = new QPen(color);
    m_graphPenArrayUser[m_numOfGraphUserColors].pen_p->setWidth(g_cfg_p->m_plot_GraphLineSize);
    m_graphPenArrayUser[m_numOfGraphUserColors].pen_p->setStyle(style);
    m_graphPenArrayUser[m_numOfGraphUserColors].color = color;
    m_graphPenArrayUser[m_numOfGraphUserColors].pattern = pattern;

    TRACEX_I(QString("Added graph line type, color:%1 pattern:%2").arg(color).arg(pattern))

    QPen *pen_p = m_graphPenArrayUser[m_numOfGraphUserColors].pen_p;

    ++m_numOfGraphUserColors;

    return pen_p;
}

/***********************************************************************************************************************
*   isPaintable
***********************************************************************************************************************/
bool CSubPlotSurface::isPaintable(void)
{
    int totalItems = 0;

    if (m_numOfDisplayGraphs > 0) {
        const int numOfGraphs = m_numOfDisplayGraphs;

        for (int graphIndex = 0; graphIndex < numOfGraphs; ++graphIndex) {
            totalItems += m_displayGraphs_a[graphIndex].m_numOfItems;
        }
    }

    if ((totalItems > 0) && !almost_equal(m_surfaceZoom.x_max, m_surfaceZoom.x_min) &&
        !almost_equal(m_surfaceZoom.y_max, m_surfaceZoom.y_min)) {
        return true;
    }

    return false;
}

/***********************************************************************************************************************
*   OnPaint_Empty
***********************************************************************************************************************/
void CSubPlotSurface::OnPaint_Empty(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    m_painter_p->fillRect(m_windowRect, BACKGROUND_COLOR);

    m_lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, g_avg_str);

    const QSize lineSize = m_lineSize;

    doc_p->m_fontCtrl.SetFont(m_painter_p, g_plotWnd_BlackFont_p);

    const int textRowHeight = lineSize.height() + static_cast<int>(lineSize.height() * 0.05);
    const int textBoxHeight = textRowHeight * NUM_OF_NO_SUBPLOT_STRINGS;
    int x;
    int y;

    x = static_cast<int>(m_windowRect.width() * 0.1);
    y = 15;

    const int totalHeight = m_windowRect.height();

    if (textBoxHeight < totalHeight) {
        y = (totalHeight - textBoxHeight) / 2;
    }

    for (int index = 0; index < NUM_OF_NO_SUBPLOT_STRINGS; ++index) {
        m_painter_p->drawText(QPoint(x, y), QString(NO_SUBPLOT_DATA[index]));
        y += textRowHeight;
    }
    DrawBoarders();
}

/***********************************************************************************************************************
*   OnPaint_1
***********************************************************************************************************************/
void CSubPlotSurface::OnPaint_1(void)
{
    PRINT_SUBPLOTSURFACE("OnPaint_1 - %s Width:%d Height:%d", m_subPlotTitle, m_windowRect.width(),
                         m_windowRect.height())

    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->m_fontCtrl.SetFont(m_painter_p, g_plotWnd_GrayFont_p);

    if (m_setupGraphs) {
        m_lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, g_avg_str);
        m_avgPixPerLetter = m_lineSize.width() / static_cast<double>(g_avg_str.length());
        m_avgPixPerLetterHeight = static_cast<double>(m_lineSize.height());
        m_halfLineHeight = static_cast<int>(m_lineSize.height() / 2.0);
        PRINT_SUBPLOTSURFACE(QString("m_avgPixPerLetter %1 %2").arg(m_lineSize.width()).arg(m_lineSize.height()))

        if (m_renderMode == RenderMode_Maximized_en) {
            SetupGraphs();
            SetupDecorators();
        }
    }
}

/***********************************************************************************************************************
*   OnPaint_2
***********************************************************************************************************************/
void CSubPlotSurface::OnPaint_2(void)
{
    CTimeMeas execTime;

    m_painter_p->fillRect(m_windowRect, BACKGROUND_COLOR);

    if (m_renderMode != RenderMode_Maximized_en) {
        QRect minimizedRect = m_windowRect;

        minimizedRect.setTop(((m_windowRect.bottom() - m_windowRect.top()) / 2) - 5);
        minimizedRect.setBottom(minimizedRect.top() + 10);
        minimizedRect.setRight(minimizedRect.left() + 10);

        m_painter_p->fillRect(minimizedRect, Q_RGB(0x55, 0xff, 0x66));
        return;
    }

    QPen oldPen = m_painter_p->pen();
    QBrush oldBrush = m_painter_p->brush();

    m_painter_p->setPen(*m_bgPen_p);
    m_painter_p->setBrush(*m_bgBrush_p);

#if 0
    TRACEX_DE("CSubPlotSurface::OnPaint  OnPaint_2 - %s Width:%d Height:%d",
              m_subPlotTitle,
              m_windowRect.width(),
              m_windowRect.height());
#endif

    if (m_windowRect.height() > 0) {
        DrawAxis_Under();
        DrawDecorators(false);  /* under */
        DrawGraphs();
        DrawDecorators(true);   /* over */
        DrawAxis_Over();
    } else {
        TRACEX_W("CSubPlotSurface::OnPaint  FAILED WINDOW TOO SMALL")
    }

    DrawBoarders();

    if (m_cursorTimeEnabled) {
        DrawCursorTime();
    }

    if (m_hasFocus) {
        if (CSCZ_LastViewSelectionKind() == CSCZ_LastViewSelectionKind_PlotView_e) {
            m_painter_p->setPen(*g_plotWnd_focusPen_p);
        } else {
            m_painter_p->setPen(*g_plotWnd_passiveFocusPen_p);
        }

        m_painter_p->drawLine(m_viewPortRect.left(), m_viewPortRect.top(),
                              m_viewPortRect.left(), m_viewPortRect.bottom());

        m_painter_p->drawLine(m_viewPortRect.left(), m_viewPortRect.bottom(),
                              m_viewPortRect.right(), m_viewPortRect.bottom());

        m_painter_p->drawLine(m_viewPortRect.right(), m_viewPortRect.bottom(),
                              m_viewPortRect.right(), m_viewPortRect.top());

        m_painter_p->drawLine(m_viewPortRect.right(), m_viewPortRect.top(),
                              m_viewPortRect.left(), m_viewPortRect.top());
    }

    m_painter_p->setPen(oldPen);
    m_painter_p->setBrush(oldBrush);

    PRINT_SUBPLOTSURFACE(QString("%1 onPaint_2 exec:%2").arg(__FUNCTION__).arg(execTime.ms()))

#ifdef _DEBUG
 #ifdef _WIN32
    (void)_CrtCheckMemory();  /*http://msdn.microsoft.com/en-us/library/z8h19c37(v=vs.71).aspx */
 #endif
#endif
}

/***********************************************************************************************************************
*   Reconfigure
***********************************************************************************************************************/
void CSubPlotSurface::Reconfigure(QRect *rect_p)
{
    if (rect_p != nullptr) {
        /* Location in the CPlotWnd */

        m_DC_windowRect = *rect_p;

        /* Aboslute coordinates within the total DC Bitmap */

        m_DC_viewPortRect.setLeft(m_DC_windowRect.left() + BOARDER_WIDTH);
        m_DC_viewPortRect.setRight(m_DC_windowRect.right() - BOARDER_WIDTH);
        m_DC_viewPortRect.setTop(m_DC_windowRect.top() + BOARDER_HEIGHT);
        m_DC_viewPortRect.setBottom(m_DC_windowRect.bottom() - BOARDER_HEIGHT);

        /* The normalized values within a per Subplot bitmap which would then later be bitblitted to the total DC Bitmap
         * */

        m_windowRect = QRect(0, 0, m_DC_windowRect.width(), m_DC_windowRect.height());

        m_viewPortRect.setLeft(m_windowRect.left() + BOARDER_WIDTH);
        m_viewPortRect.setRight(m_windowRect.right() - BOARDER_WIDTH);
        m_viewPortRect.setTop(m_windowRect.top() + BOARDER_HEIGHT);
        m_viewPortRect.setBottom(m_windowRect.bottom() - BOARDER_HEIGHT);
    }

    m_viewPort_Width = m_surfaceZoom.x_max - m_surfaceZoom.x_min;
    m_viewPort_Height = m_surfaceZoom.y_max - m_surfaceZoom.y_min;

    m_viewPort_X_Center = m_surfaceZoom.x_min + (m_surfaceZoom.x_max - m_surfaceZoom.x_min) / 2.0;
    m_viewPort_Y_Center = m_surfaceZoom.y_min + (m_surfaceZoom.y_max - m_surfaceZoom.y_min) / 2.0;

    m_unitsPerPixel_X = m_viewPort_Width / m_viewPortRect.width();
    m_unitsPerPixel_Y = m_viewPort_Height / m_viewPortRect.height();

    m_unitsPerPixel_X_inv = 1.0 / m_unitsPerPixel_X;
    m_unitsPerPixel_Y_inv = 1.0 / m_unitsPerPixel_Y;

    PRINT_SUBPLOTSURFACE(
        "CSubPlotSurface::Reconfigure  %s  WP_RECT %d %d %d %d Width:%.2e Height:%5.2 CenterX:%.2e"
        " CenterY:%4.2 ExtX %.2e %.2e ExtY:%4.2 %4.2 UnitPerPixel %e %e INV:%e %e",
        m_subPlotTitle,
        m_viewPortRect.left(), m_viewPortRect.right(), m_viewPortRect.top(), m_viewPortRect.bottom(),
        m_viewPort_Width, m_viewPort_Height,
        m_viewPort_X_Center, m_viewPort_Y_Center,
        m_surfaceZoom.x_min, m_surfaceZoom.x_max,
        m_surfaceZoom.y_min, m_surfaceZoom.y_max,
        m_unitsPerPixel_X, m_unitsPerPixel_Y,
        m_unitsPerPixel_X_inv, m_unitsPerPixel_Y_inv)
}

/***********************************************************************************************************************
*   SurfaceReconfigure
***********************************************************************************************************************/
void CSubPlotSurface::SurfaceReconfigure(QRect *windowRect_p)
{
    PRINT_SIZE(QString("%1").arg(__FUNCTION__))
    Reconfigure(windowRect_p);
    m_setupGraphs = true;
}

/***********************************************************************************************************************
*   DrawBoarders
***********************************************************************************************************************/
void CSubPlotSurface::DrawBoarders(void)
{
    int index;

    /* Paint left side */
    for (index = 0; index < (m_windowRect.bottom() - m_windowRect.top()) / m_bitmap_left.height(); ++index) {
        m_painter_p->drawImage(m_windowRect.left(), m_windowRect.top() + index * m_bitmap_left.height(), m_bitmap_left);
    }

    /* Paint right side */
    for (index = 0; index < m_windowRect.bottom() / m_bitmap_right.height(); ++index) {
        m_painter_p->drawImage(m_windowRect.right() - m_bitmap_right.width(),
                               m_windowRect.top() + index * m_bitmap_right.height(),
                               m_bitmap_right);
    }

    /* Paint top */
    for (index = 0; index < (m_windowRect.right() - m_windowRect.left()) / m_bitmap_top.width(); ++index) {
        m_painter_p->drawImage(m_windowRect.left() + m_bitmap_top.width() * index, m_windowRect.top(), m_bitmap_top);
    }

    /* Paint bottom */
    for (index = 0; index < (m_windowRect.right() - m_windowRect.left()) / m_bitmap_bottom.width(); ++index) {
        m_painter_p->drawImage(m_windowRect.left() + m_bitmap_bottom.width() * index,
                               m_windowRect.bottom() - m_bitmap_bottom.height(),
                               m_bitmap_bottom);
    }

    m_painter_p->drawImage(m_windowRect.left(),
                           m_windowRect.top(),
                           m_bitmap_left_top);
    m_painter_p->drawImage(m_windowRect.left(),
                           m_windowRect.bottom() - m_bitmap_left_bottom.height(),
                           m_bitmap_left_bottom);
    m_painter_p->drawImage(m_windowRect.right() - m_bitmap_right_top.width(),
                           m_windowRect.top(),
                           m_bitmap_right_top);
    m_painter_p->drawImage(m_windowRect.right() - m_bitmap_right_bottom.width(),
                           m_windowRect.bottom() - m_bitmap_right_bottom.height(),
                           m_bitmap_right_bottom);
}

/***********************************************************************************************************************
*   DrawAxis_Under
***********************************************************************************************************************/
void CSubPlotSurface::DrawAxis_Under(void)
{
    /* Draw all that shall be under all other parts drawn */
}

/***********************************************************************************************************************
*   DrawAxis_Over
***********************************************************************************************************************/
void CSubPlotSurface::DrawAxis_Over(void)
{
    /* Draw all that shall be ontop all other parts drawn */

    if (m_subplot_properties & SUB_PLOT_PROPERTY_SCHEDULE) {
        Draw_Y_Axis_Schedule();
    } else if (m_subplot_properties & SUB_PLOT_PROPERTY_SEQUENCE) {
        /* don't draw anything */
    } else {
        Draw_Y_Axis_Graphs();
    }

    Draw_X_Axis();

    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if (m_subPlotTitle_Size.width() == 0) {
        m_subPlotTitle_Size = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, QString(m_subPlotTitle));
        m_parentPlotTitle_Size = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, QString(m_parentPlotTitle));
        m_Y_Label_Size = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, QString(m_Y_Label));
        m_X_Label_Size = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, QString(m_X_Label));
    }

    int middle = m_viewPortRect.right() / 2;

    m_painter_p->drawText(QPoint(m_viewPortRect.left() + 10, m_viewPortRect.top() + 10), m_Y_Label);
    m_painter_p->drawText(QPoint(middle - m_subPlotTitle_Size.width() / 2, m_viewPortRect.top()), m_subPlotTitle);
    m_painter_p->drawText(QPoint(middle - m_X_Label_Size.width() / 2, m_viewPortRect.bottom() + 2), m_X_Label);
}

/***********************************************************************************************************************
*   Draw_Y_Axis_Schedule
***********************************************************************************************************************/
void CSubPlotSurface::Draw_Y_Axis_Schedule(void)
{
    m_painter_p->setBackgroundMode(Qt::TransparentMode);

    const QSize lineSize = m_lineSize;
    const int halfLineHeight = m_halfLineHeight;
    const SubPlot_Properties_t subplot_properties = m_subplot_properties;

    m_painter_p->setPen(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                              g_cfg_p->m_plot_GrayIntensity,
                              g_cfg_p->m_plot_GrayIntensity));

    CList_LSZ *graphList_p;
    m_subPlot_p->GetGraphs(&graphList_p);

    if (!graphList_p->isEmpty()) {
        CGraph_Internal *graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->first());
        int graphColorIndex = 0;
        int graphIndex = 0;

        /* Print the LEGEND */

        while (graph_p != nullptr) {
            if (graph_p->isEnabled()) {
                if ((subplot_properties & SUB_PLOT_PROPERTY_NO_LEGEND_COLOR) == 0) {
                    m_painter_p->setPen(m_colorTable_p[graphColorIndex % m_graphColors].color);
                } else {
                    m_painter_p->setPen(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                              g_cfg_p->m_plot_GrayIntensity,
                                              g_cfg_p->m_plot_GrayIntensity));
                }

                if (graph_p->GetNumOfObjects() > 0) {
                    /* PROPERTY_SCHEDULE */

                    /* BOXes */
                    GraphicalObject_Extents_t extents;
                    graph_p->GetExtents(&extents);

                    double center = extents.y_min + (extents.y_max - extents.y_min) / 2.0;
                    int y_pix =
                        static_cast<int>(m_viewPortRect.bottom() -
                                         ((center - m_surfaceZoom.y_min) * m_unitsPerPixel_Y_inv) - halfLineHeight);

                    if ((y_pix > m_windowRect.top()) && (y_pix < (m_windowRect.bottom() - lineSize.height()))) {
                        m_painter_p->drawText(
                            QPoint(static_cast<int>(m_viewPortRect.left() + m_viewPortRect.width() * 0.05), y_pix),
                            QString(graph_p->GetName()));
                    }

                    /* Draw graph boarders for schedule plots */
                    m_painter_p->setPen(*g_plotWnd_defaultDotPen_p);
                    g_plotWnd_defaultDotPen_p->setColor(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                                              g_cfg_p->m_plot_GrayIntensity,
                                                              g_cfg_p->m_plot_GrayIntensity));

                    int y_low =
                        static_cast<int>(m_viewPortRect.bottom() -
                                         ((extents.y_min - m_surfaceZoom.y_min) * m_unitsPerPixel_Y_inv));
                    int y_high =
                        static_cast<int>(m_viewPortRect.bottom() -
                                         ((extents.y_max - m_surfaceZoom.y_min) * m_unitsPerPixel_Y_inv));
                    QPoint linePoint_1;
                    QPoint linePoint_2;

                    if ((y_high < m_viewPortRect.bottom()) && (y_high > m_viewPortRect.top())) {
                        linePoint_1 = QPoint(m_viewPortRect.left(), y_high);
                        linePoint_2 = QPoint(m_viewPortRect.right(), y_high);
                        m_painter_p->drawLine(linePoint_1, linePoint_2);
                    }

                    if ((y_low < m_viewPortRect.bottom()) && (y_low > m_viewPortRect.top())) {
                        linePoint_1 = QPoint(m_viewPortRect.left(), y_low);
                        linePoint_2 = QPoint(m_viewPortRect.right(), y_low);
                        m_painter_p->drawLine(linePoint_1, linePoint_2);
                    }
                } /* PROPERTY_SCHEDULE */
                ++graphIndex;
            }
            ++graphColorIndex;
            graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->GetNext(graph_p));
        }
    }
}

/***********************************************************************************************************************
*   Draw_Y_Axis_Graphs
***********************************************************************************************************************/
void CSubPlotSurface::Draw_Y_Axis_Graphs(void)
{
    /* Legends and y-axis measurement lines */
    bool isOpaque = false;
    const int halfLineHeight = m_halfLineHeight;
    const SubPlot_Properties_t subplot_properties = m_subplot_properties;

    m_painter_p->setBackgroundMode(Qt::TransparentMode);
    m_painter_p->setPen(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                              g_cfg_p->m_plot_GrayIntensity,
                              g_cfg_p->m_plot_GrayIntensity));

    CList_LSZ *graphList_p;
    m_subPlot_p->GetGraphs(&graphList_p);

    if (!graphList_p->isEmpty()) {
        auto *graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->first());
        int graphColorIndex = 0;
        int graphIndex = 0;

        /* Print the LEGEND */

        while (graph_p != nullptr) {
            if (graph_p->isEnabled()) {
                bool isOverrideColorSet;
                Q_COLORREF overrideColor;
                GraphLinePattern_e overrideLinePattern;
                QRgb color;

                graph_p->GetOverrides(&isOverrideColorSet, &overrideColor, &overrideLinePattern);

                if (isOverrideColorSet) {
                    color = static_cast<QRgb>(overrideColor); /*m_painter_p->setPen(QColor(overrideColor)); */
                } else if ((subplot_properties & SUB_PLOT_PROPERTY_NO_LEGEND_COLOR) == 0) {
                    color = m_colorTable_p[graphColorIndex % m_graphColors].color;
                } else {
                    color = Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                  g_cfg_p->m_plot_GrayIntensity,
                                  g_cfg_p->m_plot_GrayIntensity);
                }

                m_painter_p->setPen(color);

                if (!isOpaque) {
                    m_painter_p->setBackgroundMode(Qt::OpaqueMode);
                    isOpaque = true;
                }

                /* LINEs */
                m_painter_p->drawText(
                    QPoint(static_cast<int>(m_viewPortRect.left() + (m_viewPortRect.width() * 0.05)),
                           m_viewPortRect.top() + AXIS_DECORATION_HIGHT + graphIndex * halfLineHeight * 2 + 1),
                    QString(graph_p->GetName()));

                PRINT_SUBPLOTSURFACE(QString("Graph: %1 Color:%2 prop:%3 override:%4")
                                         .arg(graph_p->GetName()).arg(color).arg(subplot_properties)
                                         .arg(isOverrideColorSet))

                ++ graphIndex;
            }

            ++graphColorIndex;

            graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->GetNext(graph_p));
        }
    }

    /* Y - Lines */
    if (!isOpaque) {
        m_painter_p->setBackgroundMode(Qt::OpaqueMode);
        isOpaque = true;
    }

    if (m_viewPortRect.height() > 30) {
        /* avoid y-lines (value) if plot is too small */

        /* Setup the Y grids */
        Setup_Y_Lines();

        QPoint startPoint;
        QPoint endPoint;

        startPoint.setX(m_viewPortRect.left());
        endPoint.setX(m_viewPortRect.right());

        m_painter_p->setPen(*g_plotWnd_defaultDotPen_p);
        g_plotWnd_defaultDotPen_p->setColor(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                                  g_cfg_p->m_plot_GrayIntensity,
                                                  g_cfg_p->m_plot_GrayIntensity));

        int index = 0;

        while (index < m_numOf_Y_Lines) {
            startPoint.setY(static_cast<int>(m_viewPortRect.bottom() -
                                             ((m_lines_Y[index] - m_surfaceZoom.y_min) * m_unitsPerPixel_Y_inv)));
            endPoint.setY(startPoint.y());

            if ((startPoint.y() > (m_viewPortRect.top() + 1)) && (startPoint.y() < (m_viewPortRect.bottom() - 1))) {
                m_painter_p->drawLine(startPoint, endPoint);
                m_painter_p->drawText(QPoint(static_cast<int>(startPoint.x() + (m_viewPortRect.width() * 0.3)),
                                             startPoint.y() + halfLineHeight), QString("%1").arg(m_lines_Y[index]));
            }
            ++index;
        }
    }
}

/***********************************************************************************************************************
*   Draw_X_Axis
***********************************************************************************************************************/
void CSubPlotSurface::Draw_X_Axis(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString temp;
    QPoint startPoint;
    QPoint endPoint;
    QSize oneNumberSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, "1");

    endPoint.setY(m_viewPortRect.bottom());
    startPoint.setY(endPoint.y() - oneNumberSize.height());

    int xaxis_label_y = startPoint.y() - oneNumberSize.height();

    /* avoid x-lines (time) if plot is too small */
    if (m_viewPortRect.height() > 50) {
        Setup_X_Lines();

        m_painter_p->setPen(*g_plotWnd_defaultPen_p);
        g_plotWnd_defaultPen_p->setColor(Q_RGB(g_cfg_p->m_plot_GrayIntensity,
                                               g_cfg_p->m_plot_GrayIntensity,
                                               g_cfg_p->m_plot_GrayIntensity));

        QSize lineSize;
        int index = 0;
        auto skipLineCount = 0;

        while (index < m_numOf_X_Lines) {
            startPoint.setX(static_cast<int>(m_viewPortRect.left() +
                                             (m_lines_X[index] - m_surfaceZoom.x_min) * m_unitsPerPixel_X_inv));
            endPoint.setX(startPoint.x());

            if (index == m_base_X_right_count) {
                /* When starting to plot the lines to the left of 0 (or the base)then we must restart the
                 * skip line counter */
                skipLineCount = 1;
            }

            if (skipLineCount % 10 == 0) {
                /* Text only at every other 10, make a bit taller line as well */
                QPoint longStartPoint = startPoint;
                longStartPoint.setY(longStartPoint.y() - oneNumberSize.height());
                m_painter_p->drawLine(longStartPoint, endPoint);

                /* temp = QString("+%.2e(s)", (double)(m_lines_X[index] - m_lines_X[0])); */
                temp = QString("%1(s)").arg(m_lines_X[index], 0, 'E', 2);
                lineSize = doc_p->m_fontCtrl.GetTextPixelLength(m_painter_p, temp);
                m_painter_p->drawText(QPoint(startPoint.x() - lineSize.width() / 2, xaxis_label_y), temp);
            } else {
                m_painter_p->drawLine(startPoint, endPoint);
            }

            ++skipLineCount;
            ++index;
        }
    }

    m_painter_p->setBackgroundMode(Qt::TransparentMode);
}

/***********************************************************************************************************************
*   DrawCursorTime
***********************************************************************************************************************/
void CSubPlotSurface::DrawCursorTime(void)
{
    int x =
        static_cast<int>(m_viewPortRect.left() + ((m_cursorTime - m_surfaceZoom.x_min) * m_unitsPerPixel_X_inv));
    m_painter_p->setPen(*m_cursorPen_Y_p);
    m_painter_p->drawLine(x, m_windowRect.top(), x, m_windowRect.bottom());
}

/***********************************************************************************************************************
*   Setup_Y_Lines
***********************************************************************************************************************/
void CSubPlotSurface::Setup_Y_Lines(void)
{
    const double y_min = static_cast<double>(m_surfaceZoom.y_min);
    const double y_max = static_cast<double>(m_surfaceZoom.y_max);
    const double y_diff = y_max - y_min;

    m_numOf_Y_Lines = static_cast<int>(std::round(m_viewPortRect.height() / m_avgPixPerLetterHeight));

    double y_diff_perline = abs(y_diff) / m_numOf_Y_Lines;
    bool is_neg = y_min < 0.0 ? true : false;
    double base = abs(pow(10, floor(log10(abs(m_surfaceZoom.y_min)))));
    base *= is_neg ? -1.0 : 1.0;

    double step = pow(10, floor(log10(y_diff_perline)));

    while (y_diff / step < m_numOf_Y_Lines) {
        step /= 2.0;
    }
    while (y_diff / step > m_numOf_Y_Lines) {
        step *= 5.0;
    }

    /* -2.4   -2    -0.4 */
    auto steps = floor((y_min - base) / step); /* could start with a negative step */
    while ((base + (steps * step)) < y_min) {
        steps++;
    }

    /* first line is base + offset* step */

    m_numOf_Y_Lines = 0;

    if (y_min < 0.0) {
        /* Ensure 0 is shown */
        m_lines_Y[m_numOf_Y_Lines++] = 0.0;
        steps = 0;
        while ((steps * step) < y_max && m_numOf_Y_Lines < ARRAY_COUNT(m_lines_Y)) {
            m_lines_Y[m_numOf_Y_Lines++] = (steps * step);
            ++steps;
        }
        steps = -1;
        while ((steps * step) > y_min && m_numOf_Y_Lines < ARRAY_COUNT(m_lines_Y)) {
            m_lines_Y[m_numOf_Y_Lines++] = (steps * step);
            --steps;
        }
    } else {
        while ((base + (steps * step)) < y_max && m_numOf_Y_Lines < ARRAY_COUNT(m_lines_Y)) {
            m_lines_Y[m_numOf_Y_Lines] = base + (steps * step);
            ++m_numOf_Y_Lines;
            ++steps;
        }
    }
}

/***********************************************************************************************************************
*   Setup_X_Lines
***********************************************************************************************************************/
void CSubPlotSurface::Setup_X_Lines(void)
{
    /* Take base from the highest possible number in the range from min to max */
    const double x_min = static_cast<double>(m_surfaceZoom.x_min);
    const double x_max = static_cast<double>(m_surfaceZoom.x_max);
    const double x_diff = x_max - x_min;

    if ((x_min < 0.0) && (x_max < 0.0)) {
        m_numOf_X_Lines = 0;
    }

    auto numLinesEstimate = static_cast<int>(m_viewPortRect.width() / m_avgPixPerLetter);
    double x_diff_per_line = abs(x_diff) / numLinesEstimate;
    double base;
    double step = pow(10, floor(log10(x_diff_per_line)));

    while (x_diff / step < numLinesEstimate) {
        step /= 2.0;
    }
    while (x_diff / step > numLinesEstimate) {
        step *= 5.0;
    }

    if (x_min <= 0.0) {
        /* if 0 is within the plot that shall be the base */
        base = 0.0;
    } else {
        /* No negative numbers.. */
        int steps_to_xmin = static_cast<int>(floor(x_min / step));
        base = (steps_to_xmin - (steps_to_xmin % 10)) * step;

        while (base < x_min) {
            base += step * 10;
        }
    }

    int index = 0;
    int steps = 0;

    while (base + (steps * step) < x_max && (index < MAX_X_LINES)) {
        m_lines_X[index++] = base + (steps * step);
        ++steps;
    }
    m_base_X_right_count = index;

    steps = -1;

    auto next_line = base + (steps * step);
    while (next_line > 0.0 && next_line > x_min && (index < MAX_X_LINES)) {
        m_lines_X[index++] = next_line;
        next_line = base + (steps * step);
        --steps;
    }
    m_numOf_X_Lines = index;
}

/***********************************************************************************************************************
*   DrawGraphs
***********************************************************************************************************************/
void CSubPlotSurface::DrawGraphs(void)
{
    /* Define the avgPixPerLetter */
    const QSize lineSize = m_lineSize;
    const int lineEnds = g_cfg_p->m_plot_lineEnds;
    const int minPixelDist = g_cfg_p->m_plot_LineEnds_MinPixelDist;
    const int maxPixelDist = g_cfg_p->m_plot_LineEnds_MaxCombinePixelDist;
    double label_relative_from_top = 0.2; /* to enable labels not overlapping entirely */

    /* In-case lines and boxes are labelled */
    m_painter_p->setBackgroundMode(Qt::TransparentMode);

    int graphIndex = 0;
    int objectIndex = 0;

    if (m_numOfDisplayGraphs > 0) {
        const int numOfGraphs = m_numOfDisplayGraphs;
        QPen *selectedPen_p = nullptr;

        for (graphIndex = 0; graphIndex < numOfGraphs; ++graphIndex) {
            CDisplayGraph *dgraph_p = &m_displayGraphs_a[graphIndex];
            const int numOfItems = dgraph_p->m_numOfItems;
            Q_COLORREF color = 0;
            Q_COLORREF prevColor = 0;
            GraphLinePattern_e pattern = GLP_NONE;

            if (dgraph_p->m_graph_p->isEnabled() && (numOfItems > 0)) {
                /* Set the "default" color for the graph, this might be overriden by LineEx colors etc */

                if (selectedPen_p != dgraph_p->m_pen_p) {
                    if (nullptr == dgraph_p->m_pen_p) {
                        TRACEX_E("Missing pen")
                    }
                    m_painter_p->setPen(*dgraph_p->m_pen_p);

                    selectedPen_p = dgraph_p->m_pen_p;
                    color = dgraph_p->m_color;
                    pattern = dgraph_p->m_pattern;
                }

                /* Find first GO inside viewPort, this is intersected in-case it would otherwise be outside */
                for (objectIndex = 0; objectIndex < numOfItems; ++objectIndex) {
                    /* loop until the objects are starting to be visible */
                    if (dgraph_p->m_items_a[objectIndex].properties & PROPERTIES_BITMASK_VISIBILITY_MASK) {
                        break;
                    }
                }

                int x1_old = 0;
                int y1_old = 0;
                int x2_old = 0;
                int y2_old = 0;
                QRect rect;
                int prevPointsHidden = 0;

                for ( ; objectIndex < numOfItems; ++objectIndex) {
                    const displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];

                    if (di_p->properties & (PROPERTIES_BITMASK_VISIBLE | PROPERTIES_BITMASK_VISIBLE_INTERSECT)) {
                        int x_1 = di_p->x1_pix;
                        int y_1 = di_p->y1_pix;
                        const int x_2 = di_p->x2_pix;
                        const int y_2 = di_p->y2_pix;
                        const short properties = di_p->properties;
                        double relative_X = 0.0;  /* Some graphical object may offset the label */
                        int x_len = 0;
                        int y_len = 0;
                        int x_dist = 0;
                        int y_dist = 0;
                        int x_dist_hidden = 0;
                        int y_dist_hidden = 0;

                        x_len = x_2 >= x_1 ? x_2 - x_1 : x_1 - x_2;
                        y_len = y_2 >= y_1 ? y_2 - y_1 : y_1 - y_2;
                        x_dist = x2_old >= x_1 ? x2_old - x_1 : x_1 - x2_old;
                        y_dist = y2_old >= y_1 ? y2_old - y_1 : y_1 - y2_old;

                        if (prevPointsHidden) {
                            /* make sure that the point farthest from previous is considered, assume that
                             * previous line was drawn if y1_old and y2_old diff was large */
                            int y11 = ABS(y1_old - y_1);
                            int y12 = ABS(y1_old - y_2);

                            y_dist_hidden = y11 > y12 ? y11 : y12;
                            x_dist_hidden = x_1 - x1_old; /* time shall always progress forwamd */
                        } else {
                            x_dist_hidden = 0;
                            y_dist_hidden = 0;
                        }

                        if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                            /* LINE */
                            if ((properties & GRAPHICAL_OBJECT_KIND_LINE) == 0) {
                                /* is a special line */
                                Q_COLORREF lineExColor =
                                    reinterpret_cast<GraphicalObject_Line_Ex_t *>(di_p->go_p)->lineColorRGB;

                                /* User may specify 0 to have legend color */
                                if ((lineExColor != 0) && (color != lineExColor)) {
                                    selectedPen_p = GetUserDefinedPen(lineExColor, pattern);
                                    if (selectedPen_p != nullptr) {
                                        /* Necessary in-case label changed in previously */
                                        m_painter_p->setPen(*selectedPen_p);
                                        color = lineExColor;
                                    } else {
                                        TRACEX_E(QString("SetPen null"))
                                    }
                                }
                            }

                            if (lineEnds == 0) {
                                /* Never any line end */
                                m_painter_p->drawLine(x_1, y_1, x_2, y_2);
                                prevPointsHidden = 0;
                                x1_old = x_1;
                                y1_old = y_1;
                                x2_old = x_2;
                                y2_old = y_2;
                            } else if (lineEnds == 1) {
                                /* Always line end */
                                m_painter_p->drawLine(x_1, y_1, x_2, y_2);
                                prevPointsHidden = 0;
                                x1_old = x_1;
                                y1_old = y_1;
                                x2_old = x_2;
                                y2_old = y_2;

                                rect.setLeft(x_2 - 1);
                                rect.setRight(x_2 + 1);
                                rect.setTop(y_2 - 1);
                                rect.setBottom(y_2 + 1);

                                m_painter_p->fillRect(rect, static_cast<QRgb>(color));
                            } else if (

                                /* always plot lines with intersection flag set */
                                (properties & PROPERTIES_BITMASK_VISIBLE_INTERSECT) ||

                                /* typically lines with arrows should always be drawn */
                                properties & PROPERTIES_BITMASK_KIND_LINE_ARROW_MASK ||

                                /* If at least some distance to the previous line */
                                ((x_dist > minPixelDist) || (y_dist > minPixelDist) ||
                                 (x_len > minPixelDist) || (y_len > minPixelDist)) ||

                                /* If line is changing color we shall show the line */
                                (color != prevColor) ||

                                /* If an old saved line causes a long distance */
                                (prevPointsHidden && ((x_dist_hidden > minPixelDist) ||
                                                      (y_dist_hidden > minPixelDist)))) {
                                if (prevPointsHidden) {
                                    if ((x_dist_hidden > maxPixelDist) || (y_dist_hidden > maxPixelDist)) {
                                        /* If the new line is too far from the last, then these must be seperatly drawn
                                         * */
                                        m_painter_p->drawLine(x1_old, y1_old, x2_old, y2_old);
                                        m_painter_p->drawLine(x_1, y_1, x_2, y_2);
                                    } else {
                                        m_painter_p->drawLine(x1_old, y1_old, x_2, y_2);

                                        rect = QRect(QPoint(x_2 - 2, y_2 - 2), QPoint(x_2 + 2, y_2 + 2));
                                        m_painter_p->fillRect(rect, static_cast<QRgb>(color));
                                    }
                                } else {
                                    m_painter_p->drawLine(x_1, y_1, x_2, y_2);

                                    rect = QRect(QPoint(x_2 - 1, y_2 - 1), QPoint(x_2 + 1, y_2 + 1));
                                    m_painter_p->fillRect(rect, static_cast<QRgb>(color));
                                }

                                prevPointsHidden = 0;
                                x1_old = x_1;
                                y1_old = y_1;
                                x2_old = x_2;
                                y2_old = y_2;
                            } else {
                                if (prevPointsHidden == 0) {
                                    x1_old = x_1;
                                    y1_old = y_1;
                                }

                                x2_old = x_2;
                                y2_old = y_2;

                                ++prevPointsHidden;
                            }

                            if ((prevPointsHidden == 0) && (properties & PROPERTIES_BITMASK_KIND_LINE_ARROW_MASK)) {
                                int arrow_1_x1;
                                int arrow_1_x2;
                                int arrow_1_y1;
                                int arrow_1_y2;
                                int arrow_2_x1;
                                int arrow_2_x2;
                                int arrow_2_y1;
                                int arrow_2_y2;

                                if (properties & (PROPERTIES_BITMASK_LINE_ARROW_OPEN_START |
                                                  PROPERTIES_BITMASK_LINE_ARROW_SOLID_START)) {
                                    if (y_1 < y_2) {
                                        /* arrow point up, since this is the start end */
                                        arrow_1_x1 = x_1;
                                        arrow_1_x2 = x_1 - 3;
                                        arrow_1_y1 = y_1;
                                        arrow_1_y2 = y_1 + 6;
                                        arrow_2_x1 = x_1;
                                        arrow_2_x2 = x_1 + 3;
                                        arrow_2_y1 = y_1;
                                        arrow_2_y2 = y_1 + 6;
                                    } else {
                                        arrow_1_x1 = x_1;
                                        arrow_1_x2 = x_1 - 3;
                                        arrow_1_y1 = y_1;
                                        arrow_1_y2 = y_1 - 6;
                                        arrow_2_x1 = x_1;
                                        arrow_2_x2 = x_1 + 3;
                                        arrow_2_y1 = y_1;
                                        arrow_2_y2 = y_1 - 6;
                                    }

                                    m_painter_p->drawLine(arrow_1_x1, arrow_1_y1, arrow_1_x2, arrow_1_y2);
                                    m_painter_p->drawLine(arrow_2_x1, arrow_2_y1, arrow_2_x2, arrow_2_y2);
                                }

                                if (properties & (PROPERTIES_BITMASK_LINE_ARROW_OPEN_END |
                                                  PROPERTIES_BITMASK_LINE_ARROW_SOLID_END)) {
                                    if (y_1 < y_2) {
                                        /* arrow point down */
                                        arrow_1_x1 = x_2;
                                        arrow_1_x2 = x_2 - 3;
                                        arrow_1_y1 = y_2 + ARROW_TIP_ADJUST;
                                        arrow_1_y2 = y_2 - 6 + ARROW_TIP_ADJUST;
                                        arrow_2_x1 = x_2;
                                        arrow_2_x2 = x_2 + 3;
                                        arrow_2_y1 = y_2 + ARROW_TIP_ADJUST;
                                        arrow_2_y2 = y_2 - 6 + ARROW_TIP_ADJUST;
                                    } else {
                                        arrow_1_x1 = x_2;
                                        arrow_1_x2 = x_2 - 3;
                                        arrow_1_y1 = y_2;
                                        arrow_1_y2 = y_2 + 6;
                                        arrow_2_x1 = x_2;
                                        arrow_2_x2 = x_2 + 3;
                                        arrow_2_y1 = y_2;
                                        arrow_2_y2 = y_2 + 6;
                                    }

                                    m_painter_p->drawLine(arrow_1_x1, arrow_1_y1, arrow_1_x2, arrow_1_y2);
                                    m_painter_p->drawLine(arrow_2_x1, arrow_2_y1, arrow_2_x2, arrow_2_y2);

                                    if (properties & PROPERTIES_BITMASK_LINE_ARROW_SOLID_END) {
                                        m_painter_p->drawLine(arrow_2_x2, arrow_2_y2, arrow_1_x2, arrow_1_y2);
                                    }
                                }
                            }
                        } else {
                            /* BOX */
                            if ((properties & GRAPHICAL_OBJECT_KIND_BOX) == 0) {
                                if (reinterpret_cast<GraphicalObject_Box_Ex_t *>(di_p->go_p)->fillColorRGB != 0) {
                                    /* User may speficy 0 to have legend color */
                                    color = reinterpret_cast<GraphicalObject_Box_Ex_t *>(di_p->go_p)->fillColorRGB;
                                }
                            }

                            /* always plot boxes with intersection flag set */
                            if ((properties & PROPERTIES_BITMASK_VISIBLE_INTERSECT) ||

                                /* if the box is large enough */
                                (x_len > minPixelDist) || (y_len > minPixelDist) ||

                                /* if the box distance to previous is large enough */
                                (x_dist > 0) || (y_dist > 0) ||

                                /* this box had different color than the previous, needs to be drawn */
                                (prevColor != color) ||

                                /* if the hidden box distance tell us to paint */
                                (prevPointsHidden && ((x_dist_hidden > minPixelDist) ||
                                                      (y_dist_hidden > minPixelDist)))) {
                                if (prevPointsHidden) {
                                    /* There is a pending/hidden box that needs to be considered */

                                    /*1. should they be combined? */

                                    if ((prevColor != color) || (x_dist > 0) || (y_dist > 0)) {
                                        /* no combine */

                                        rect = QRect(QPoint(x1_old, y1_old), QPoint(x2_old, y2_old));
                                        if (rect.width() == 0) {
                                            rect.setRight(rect.right() + 1);
                                        }
                                        if (rect.height() == 0) {
                                            rect.setBottom(rect.bottom() + 1);
                                        }
                                        m_painter_p->fillRect(rect, static_cast<QRgb>(prevColor));

                                        rect = QRect(QPoint(x_1, y_1), QPoint(x_2, y_2));
                                        if (rect.width() == 0) {
                                            rect.setRight(rect.right() + 1);
                                        }
                                        if (rect.height() == 0) {
                                            rect.setBottom(rect.bottom() + 1);
                                        }
                                        m_painter_p->fillRect(rect, static_cast<QRgb>(color));
                                    } else {
                                        /* combine */
                                        rect = QRect(QPoint(x1_old, y_1), QPoint(x2_old, y_2));
                                        if (rect.width() == 0) {
                                            rect.setRight(rect.right() + 1);
                                        }
                                        if (rect.height() == 0) {
                                            rect.setBottom(rect.bottom() + 1);
                                        }
                                        m_painter_p->fillRect(rect, static_cast<QRgb>(prevColor));
                                    }
                                } else {
                                    rect = QRect(QPoint(x_1, y_1), QPoint(x_2, y_2));
                                    if (rect.width() == 0) {
                                        rect.setRight(rect.right() + 1);
                                    }
                                    if (rect.height() == 0) {
                                        rect.setBottom(rect.bottom() + 1);
                                    }
                                    m_painter_p->fillRect(rect, static_cast<QRgb>(color));
                                }

                                prevPointsHidden = 0;
                                x1_old = x_1;
                                y1_old = y_1;
                                x2_old = x_2;
                                y2_old = y_2;
                            } else {
                                if (prevPointsHidden == 0) {
                                    x1_old = x_1;
                                    y1_old = y_1;
                                }

                                x2_old = x_2;
                                y2_old = y_2;

                                ++prevPointsHidden;
                            }
                        }

                        if ((prevPointsHidden == 0) && (di_p->label_p != nullptr)) {
                            bool showLabel = false;

                            if ((x_2 == x_1) && ((properties & PROPERTIES_BITMASK_KIND_BOX_MASK) == 0)) {
                                const int y_Size = y_2 > y_1 ? y_2 - y_1 : y_1 - y_2;

                                if (y_Size > (lineSize.height() >> 1)) {
                                    showLabel = true;
                                }
                            } else if (di_p->label_pix_length < (x_2 - x_1)) {
                                showLabel = true;
                            }

                            if (showLabel) {
                                int totalColor = ((color & 0xff0000) >> 16) + ((color & 0xff00) >> 8) + (color & 0xff);

                                if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                                    (void)m_painter_p->setPen(QRgb(0));
                                } else if (totalColor > (0x85 * 3)) {
                                    /* lighter colors require a black label */
                                    (void)m_painter_p->setPen(QRgb(0));
                                } else {
                                    (void)m_painter_p->setPen(QRgb(0xffffff));
                                }

                                int label_x_start;

                                if (almost_equal(relative_X, 0.0)) {
                                    label_x_start = x_1 + static_cast<int>((x_2 - x_1) / 2.0 -
                                                                           (di_p->label_pix_length) / 2.0);
                                } else {
                                    label_x_start = x_1 + static_cast<int>((x_2 - x_1) * relative_X);
                                }

                                double y_text;
                                const double y_1_temp = static_cast<double>(y_1 > y_2 ? y_1 : y_2);
                                const double y_2_temp = static_cast<double>(y_1 > y_2 ? y_2 : y_1);

                                if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK &&
                                    (di_p->x2_pix - di_p->x1_pix < 2)) {
                                    y_text = y_2_temp + (y_1_temp - y_2_temp) * label_relative_from_top;
                                    label_relative_from_top = label_relative_from_top < 0.8 ?
                                                              label_relative_from_top + 0.2 : 0.2;
                                } else {
                                    y_text = y_2_temp + (y_1_temp - y_2_temp) / 2.0 +
                                             m_avgPixPerLetterHeight / 2.0 - Y_ADJUST;
                                }

                                const auto box_upper = y_text - m_avgPixPerLetterHeight;
                                const auto box_lower = y_text + 2.0;
                                if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                                    rect = QRect(QPoint(label_x_start - 2, static_cast<int>(box_upper)),
                                                 QPoint(static_cast<int>(label_x_start +
                                                                         static_cast<int>(di_p->label_pix_length) + 2),
                                                        static_cast<int>(box_lower)));

                                    m_painter_p->fillRect(rect, BACKGROUND_COLOR);
                                }

                                m_painter_p->drawText(QPoint(label_x_start, static_cast<int>(y_text)),
                                                      QString(di_p->label_p));
                                m_painter_p->setPen(*selectedPen_p); /* Set pen back */
                            }
                        }

                        if (m_cursorRowEnabled && (di_p->go_p->row == m_cursorRow)) {
                            m_painter_p->setPen(*m_cursorPen_Y_p);
                            rect = QRect(QPoint(x_1 - 2, y_1 - 2), QPoint(x_1 + 2, y_1 + 2));
                            m_painter_p->drawEllipse(rect);

                            m_painter_p->drawLine(x_1, m_windowRect.top(), x_1, m_windowRect.bottom());
                            m_painter_p->setPen(*selectedPen_p);
                        }
                        prevColor = color;
                    } /* if visible */
                } /* for go */
            } /* if graph_p->isEnabled() */
        } /* for graphs */
    } /* if numOfGraphs > 0 */
}

/***********************************************************************************************************************
*   DrawDecorators
***********************************************************************************************************************/
void CSubPlotSurface::DrawDecorators(bool over)
{
    /* lifeline box is drawn over
     * lifeline liness are drawn under */

    if (m_displayDecorator.m_numOfItems == 0) {
        return;
    }

    /* Define the avgPixPerLetter
     * In-case lines and boxes are labelled */
    m_painter_p->setBackgroundMode(Qt::TransparentMode);

    int objectIndex = 0;
    CDisplayGraph *dgraph_p = &m_displayDecorator;
    const int numOfItems = dgraph_p->m_numOfItems;

    if (dgraph_p->m_graph_p->isEnabled() && (numOfItems > 0)) {
        QRect rect;
        Q_COLORREF color = Q_RGB(0x55, 0x55, 0x55);  /* use red */

        /* First align the x2 boundary for all life line boxes */
        int life_line_x2 = 0;
        int objectIndexPreLoop = objectIndex;
        for ( ; objectIndexPreLoop < numOfItems; ++objectIndexPreLoop) {
            const displayItem_t *di_p = &dgraph_p->m_items_a[objectIndexPreLoop];
            auto x_2 = di_p->x2_pix;
            auto properties = di_p->properties;

            if (properties & (PROPERTIES_BITMASK_VISIBLE | PROPERTIES_BITMASK_VISIBLE_INTERSECT)) {
                if ((properties & PROPERTIES_BITMASK_KIND_BOX_MASK)) {
                    if (x_2 > life_line_x2) {
                        life_line_x2 = x_2;
                    }
                }
            }
        }

        for ( ; objectIndex < numOfItems; ++objectIndex) {
            const displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];
            const int x_1 = di_p->x1_pix;
            const int y_1 = di_p->y1_pix;
            int x_2 = di_p->x2_pix;
            const int y_2 = di_p->y2_pix;
            const short properties = di_p->properties;
            bool checkLabel = false;            /* true if go was painted */
            double relative_X = 0.0;      /* Some graphical object may offset the label */

            if (properties & (PROPERTIES_BITMASK_VISIBLE | PROPERTIES_BITMASK_VISIBLE_INTERSECT)) {
                if ((properties & GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE)) {
                    if (over) {
                        /* Add all life line objects here that shall be drawn over */

                        if (properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
                            x_2 = life_line_x2;
                            rect = QRect(QPoint(x_1, y_2),
                                         QPoint(x_2 - x_1 > 3 ? x_2 : x_1 + 3, y_1 - y_2 > 3 ?
                                                y_1 : y_2 + 3 /* at least 3 pix height */));
                            color = static_cast<QRgb>(reinterpret_cast<GraphicalObject_LifeLine_Box_t *>
                                                      (di_p->go_p)->fillColorRGB);

                            m_painter_p->fillRect(rect, color);
                            checkLabel = true;
                        }
                    } else {
                        /* under */

                        /* Add all life line objects here that shall be drawn under */

                        if ((properties & PROPERTIES_BITMASK_KIND_LINE_MASK) && !over) {
                            /* lifeline line, drawn under */
                            if (properties & (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX |
                                              GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR)) {
                                relative_X = reinterpret_cast<GraphicalObject_LifeLine_Line_t *>
                                             (di_p->go_p)->relative_X;
                                color = static_cast<QRgb>(reinterpret_cast<GraphicalObject_LifeLine_Line_t *>
                                                          (di_p->go_p)->lineColorRGB);
                            }

                            /* The life line is drawn as a box to get some thickness and color */
                            rect = QRect(QPoint(x_1, y_1 - 1), QPoint(x_2, y_1 + 1));
                            m_painter_p->fillRect(rect, color);

                            checkLabel = true;
                        } /* if line */
                    } /* else over */
                } /* if life line */
                else {
                    if (over) {
                        if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                            /* LINE (normal) */
                            if (properties & (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX |
                                              GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR)) {
                                relative_X = reinterpret_cast<GraphicalObject_Line_Ex_t *>(di_p->go_p)->relative_X;
                                color = static_cast<QRgb>(reinterpret_cast<GraphicalObject_Line_Ex_t *>
                                                          (di_p->go_p)->lineColorRGB);
                            }
                            rect = QRect(QPoint(x_1, y_1), QPoint(x_2, y_1 + 1));
                            m_painter_p->fillRect(rect, color);

                            checkLabel = true;
                        } else {
                            /* BOX (normal box) */
                            rect.setLeft(x_1);
                            rect.setRight(x_2 - x_1 > 3 ? x_2 : x_1 + 3);

                            if (y_1 < y_2) {
                                rect.setTop(y_1);
                                rect.setBottom(y_2 - y_1 > 3 ? y_2 : y_1 + 3);
                            } else {
                                rect.setTop(y_2);
                                rect.setBottom(y_1 - y_2 > 3 ? y_1 : y_2 + 3);
                            }

                            if (properties & (GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX |
                                              GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR)) {
                                color = static_cast<Q_COLORREF>(reinterpret_cast<GraphicalObject_Box_Ex_t *>
                                                                (di_p->go_p)->fillColorRGB);
                            }

                            m_painter_p->fillRect(rect, color);
                            checkLabel = true;
                        }
                    }
                } /* else lifeline */

                if (checkLabel && (di_p->label_p != nullptr) && (di_p->label_pix_length < (x_2 - x_1))) {
                    int totalColor = ((color & 0xff0000) >> 16) + ((color & 0xff00) >> 8) + (color & 0xff);

                    if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                        (void)m_painter_p->setPen(QRgb(0x0));
                    }

                    /* lighter colors require a black label */
                    else if (totalColor > (0x85 * 3)) {
                        (void)m_painter_p->setPen(QRgb(0x0));
                    } else {
                        (void)m_painter_p->setPen(QRgb(0xffffff));
                    }

                    int label_x_start;

                    if (relative_X == 0.0) {
                        label_x_start = x_1 + ((x_2 - x_1) >> 1) - (di_p->label_pix_length >> 1);
                    } else {
                        label_x_start = x_1 + static_cast<int>((x_2 - x_1) * relative_X);
                    }

                    const int y_text = y_1 - static_cast<int>(((y_1 - y_2) >> 1) - m_avgPixPerLetterHeight / 2.0);

                    if (properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                        /* Erase a small box where the label should be, mainly covering just the own line */
                        rect = QRect(QPoint(label_x_start, y_text - 2),
                                     QPoint(label_x_start + di_p->label_pix_length,
                                            static_cast<int>(static_cast<double>(y_text) -
                                                             m_avgPixPerLetterHeight + 2.0)));

                        m_painter_p->fillRect(rect, BACKGROUND_COLOR);
                    }
                    m_painter_p->drawText(QPoint(label_x_start, y_text - static_cast<int>(Y_ADJUST)),
                                          QString(di_p->label_p));
                }
            }  /* if visible */
        } /* for go */
    } /* if graph_p->isEnabled() */
}

/***********************************************************************************************************************
*   Intersection_LINE_Out2In
*   Calculates where the line first is visible going into the visible area, and update/move the point to that place
***********************************************************************************************************************/
bool CSubPlotSurface::Intersection_LINE_Out2In(int pl_0, double *p0_x_p, double *p0_y_p, double p1_x, double p1_y)
{
    /* NOTE:  This function checks against original values, and not the Y inverted window locations.  Y_T -> Y_MAX */
    const double x_min = m_surfaceZoom.x_min;
    const double x_max = m_surfaceZoom.x_max;
    const double y_min = m_surfaceZoom.y_min;
    const double y_max = m_surfaceZoom.y_max;
    double x_intersect;
    double y_intersect;

    switch (pl_0) /* (point location) */
    {
        case PL_YT_XL_en: /* cross either Y_T (y top) or X_L (x left) */
            /* Y_T,  check against top horizontal */
            if (!Intersection_LINE(x_min, y_max, x_max, y_max, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                if (!Intersection_LINE(x_min, y_min, x_min, y_max,
                                       *p0_x_p, *p0_y_p, p1_x, p1_y,
                                       &x_intersect, &y_intersect)) {
                    /* X_L, check against left vertical */
                    return false;
                }
            }
            break;

        case PL_YC_XL_en: /* cross only X_L */
            /* X_L, check against left vertical */
            if (!Intersection_LINE(x_min, y_min, x_min, y_max, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                return false;
            }
            break;

        case PL_YB_XL_en:  /* cross either Y_B or X_L */
            /* Y_B,  check against bottom horizontal */
            if (!Intersection_LINE(x_min, y_min, x_max, y_min,
                                   *p0_x_p, *p0_y_p, p1_x, p1_y,
                                   &x_intersect, &y_intersect)) {
                /* X_L, check against left vertical */
                if (!Intersection_LINE(x_min, y_min, x_min, y_max,
                                       *p0_x_p, *p0_y_p, p1_x, p1_y,
                                       &x_intersect, &y_intersect)) {
                    return false;
                }
            }
            break;

        case PL_YT_XC_en:

            /* Y_T,  check against top horizontal */
            if (!Intersection_LINE(x_min, y_max, x_max, y_max,
                                   *p0_x_p, *p0_y_p, p1_x, p1_y,
                                   &x_intersect, &y_intersect)) {
                return false;
            }
            break;

        case PL_YC_XC_en:
            return true;

        case PL_YB_XC_en:

            /* Y_B,  check against bottom horizontal */
            if (!Intersection_LINE(x_min, y_min, x_max, y_min, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                return false;
            }
            break;

        case PL_YT_XR_en:
        case PL_YC_XR_en:
        case PL_YB_XR_en:
        default:
            TRACEX_W("CSubPlotSurface::Intersection_LINE_Out2In  INPUT WARNING")
            return false;
    }

    *p0_x_p = x_intersect;
    *p0_y_p = y_intersect;

    return true;
}

/***********************************************************************************************************************
*   Intersection_LINE_In2Out
*   Calculate where the line ends going out of the visible area
***********************************************************************************************************************/
bool CSubPlotSurface::Intersection_LINE_In2Out(int pl_1, double *p0_x_p, double *p0_y_p, double p1_x, double p1_y)
{
    /* NOTE:  This function checks against original values, and not the Y inverted window locations.  Y_T -> Y_MAX */
    const double x_min = m_surfaceZoom.x_min;
    const double x_max = m_surfaceZoom.x_max;
    const double y_min = m_surfaceZoom.y_min;
    const double y_max = m_surfaceZoom.y_max;
    double x_intersect;
    double y_intersect;

    /* The right point is outside the viewport/surface, check which
     * axis it crosses, base it against where the right point is located. */

    switch (pl_1)
    {
        case PL_YT_XC_en: /* crosses Y_T */
            /* Y_T,  check against top horizontal */
            if (!Intersection_LINE(x_min, y_max, x_max, y_max, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                return false;
            }
            break;

        case PL_YC_XC_en:
            return true;

        case PL_YB_XC_en: /* crosses only Y_B */
            /* Y_B, check against bottom horizontal */
            if (!Intersection_LINE(x_min, y_min, x_max, y_min, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                return false;
            }
            break;

        case PL_YB_XR_en:  /* cross either Y_B or X_R */
            /* Y_B,  check against bottom horizontal */
            if (!Intersection_LINE(x_min, y_min, x_max, y_min, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                if (!Intersection_LINE(x_max, y_min, x_max, y_max,
                                       *p0_x_p, *p0_y_p, p1_x, p1_y,
                                       &x_intersect, &y_intersect)) {
                    /* X_R, check against right vertical */
                    return false;
                }
            }
            break;

        case PL_YC_XR_en:  /* cross X_R */
            /* X_R, check against right vertical */
            if (!Intersection_LINE(x_max, y_min, x_max, y_max, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                return false;
            }
            break;

        case PL_YT_XR_en:  /* crosses either Y_T or X_R */
            /* Y_T,  check against top horizontal */
            if (!Intersection_LINE(x_min, y_max, x_max, y_max, *p0_x_p, *p0_y_p, p1_x, p1_y, &x_intersect,
                                   &y_intersect)) {
                if (!Intersection_LINE(x_max, y_min, x_max, y_max,
                                       *p0_x_p, *p0_y_p, p1_x, p1_y,
                                       &x_intersect, &y_intersect)) {
                    /* X_R, check against right vertical */
                    return false;
                }
            }
            break;

        case PL_YT_XL_en:
        case PL_YC_XL_en:
        case PL_YB_XL_en:
        default:
            TRACEX_W("CSubPlotSurface::Intersection_LINE_In2Out  INPUT WARNING")
            return false;
    }

    *p0_x_p = x_intersect;
    *p0_y_p = y_intersect;

    return true;
}

/***********************************************************************************************************************
*   Intersection_LINE
***********************************************************************************************************************/
bool CSubPlotSurface::Intersection_LINE(
    double p0_x, double p0_y, double p1_x, double p1_y,   /* Line 1 */
    double p2_x, double p2_y, double p3_x, double p3_y,   /* Line 2 */
    double *i_x, double *i_y)
{
    /* Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
     * intersect the intersection point may be stored in the floats i_x and i_y. */

    double s1_x, s1_y, s2_x, s2_y;

    s1_x = p1_x - p0_x;
    s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;
    s2_y = p3_y - p2_y;

    double s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if ((s >= 0) && (s <= 1) && (t >= 0) && (t <= 1)) {
        /* Collision detected */
        if (i_x != nullptr) {
            *i_x = p0_x + t * s1_x;
        }

        if (i_y != nullptr) {
            *i_y = p0_y + t * s1_y;
        }

        return true;
    }

    return false; /* No collision */
}

/***********************************************************************************************************************
*   Intersection_BOX_Out2In
***********************************************************************************************************************/
bool CSubPlotSurface::Intersection_BOX_Out2In(int pl_0, double *p0_x_p, double *p0_y_p)
{
    /* NOTE:  This function checks against original values, and not the Y inverted window locations.  Y_T -> Y_MAX */
    const double x_min = m_surfaceZoom.x_min;
    const double y_min = m_surfaceZoom.y_min;
    const double y_max = m_surfaceZoom.y_max;

    /* Only modify p0_x and P0_y if needed */

    switch (pl_0)
    {
        case PL_YT_XL_en: /* cross either Y_T or X_L */
            *p0_x_p = x_min;
            *p0_y_p = y_max;
            break;

        case PL_YC_XL_en: /* cross only X_L */
            *p0_x_p = x_min;
            break;

        case PL_YB_XL_en:  /* cross either Y_B or X_L */
            *p0_x_p = x_min;
            *p0_y_p = y_min;
            break;

        case PL_YT_XC_en:
            *p0_y_p = y_max;
            break;

        case PL_YB_XC_en:
            *p0_y_p = y_min;
            break;

        case PL_YC_XC_en:

            /* do nothing */
            break;

        case PL_YT_XR_en:
        case PL_YC_XR_en:
        case PL_YB_XR_en:
        default:
            TRACEX_W("CSubPlotSurface::Intersection_BOX_Out2In  INPUT WARNING")
            return false;
    }

    return true;
}

/***********************************************************************************************************************
*   Intersection_BOX_In2Out
***********************************************************************************************************************/
bool CSubPlotSurface::Intersection_BOX_In2Out(int pl_1, double *p1_x_p, double *p1_y_p)
{
    /* NOTE:  This function checks against original values, and not the Y inverted window locations.  Y_T -> Y_MAX */
    const double x_max = m_surfaceZoom.x_max;
    const double y_min = m_surfaceZoom.y_min;
    const double y_max = m_surfaceZoom.y_max;

    /* The right point is outside the viewport/surface, check which axis it crosses, base it against
     * where the right point is located. */

    switch (pl_1)
    {
        case PL_YT_XC_en: /* crosses Y_T */
            *p1_y_p = y_max;
            break;

        case PL_YB_XC_en: /* crosses only Y_B */
            *p1_y_p = y_min;
            break;

        case PL_YB_XR_en:  /* cross either Y_B or X_R */
            *p1_x_p = x_max;
            *p1_y_p = y_min;
            break;

        case PL_YC_XR_en:  /* cross X_R */
            *p1_x_p = x_max;
            break;

        case PL_YT_XR_en:  /* crosses either Y_T or X_R */
            *p1_x_p = x_max;
            *p1_y_p = y_max;
            break;

        case PL_YC_XC_en:

            /* do nothing */
            break;

        case PL_YT_XL_en:
        case PL_YC_XL_en:
        case PL_YB_XL_en:
        default:
            TRACEX_D("CSubPlotSurface::Intersection_BOX_In2Out  INPUT WARNING")
            return false;
    }

    return true;
}

/***********************************************************************************************************************
*   SetupGraphs
***********************************************************************************************************************/
void CSubPlotSurface::SetupGraphs(void)
{
    int graphIndex;
    const int numOfGraphs = m_numOfDisplayGraphs;
    m_setupGraphs = false;

    int threadIndex = 0;

#ifdef _DEBUG
    CTimeMeas execTime;
#endif

    for (graphIndex = 0; graphIndex < numOfGraphs; ++graphIndex) {
        if (m_displayGraphs_a[graphIndex].m_numOfItems > 0) {
            g_setupGraph_WorkData[threadIndex].dgraph_p = &m_displayGraphs_a[graphIndex];
            g_setupGraph_WorkData[threadIndex].subplotSurface_p = this;
            g_setupGraph_WorkData[threadIndex].startIndex = -1;
            g_setupGraph_WorkData[threadIndex].stopIndex = -1;

            int penSelection = graphIndex % m_graphPenArraySize;

            if (m_displayGraphs_a[graphIndex].m_isOverrideColorSet) {
                m_displayGraphs_a[graphIndex].m_color = m_displayGraphs_a[graphIndex].m_overrideColor;
            } else {
                /* The default color for this graph */
                m_displayGraphs_a[graphIndex].m_color = m_graphPenArray_p[penSelection].color;
            }

            if (m_displayGraphs_a[graphIndex].m_overrideLinePattern != GLP_NONE) {
                m_displayGraphs_a[graphIndex].m_pattern = m_displayGraphs_a[graphIndex].m_overrideLinePattern;
            } else {
                /* The default pattern for this graph */
                m_displayGraphs_a[graphIndex].m_pattern = m_graphPenArray_p[penSelection].pattern;
            }

            m_displayGraphs_a[graphIndex].m_pen_p = GetUserDefinedPen(m_displayGraphs_a[graphIndex].m_color,
                                                                      m_displayGraphs_a[graphIndex].m_pattern);

#ifdef MULTIPROC_SETUPGRAPH
            if (m_displayGraphs_a[graphIndex].m_numOfItems > 10000) {
                const int numOfItems = m_displayGraphs_a[graphIndex].m_numOfItems;
                const int itemsPerThread = numOfItems / g_cfg_p->m_filter_NumOfThreads;
                int startIndex = 0;
                int loop = 1;

                /* use each thread for this */
                while (loop <= g_cfg_p->m_numOfThreads) {
                    /* loop count from 1 upwards */
                    g_setupGraph_WorkData[threadIndex].dgraph_p = &m_displayGraphs_a[graphIndex];
                    g_setupGraph_WorkData[threadIndex].subplotSurface_p = this;
                    g_setupGraph_WorkData[threadIndex].startIndex = startIndex;

                    g_setupGraph_WorkData[threadIndex].stopIndex = startIndex + itemsPerThread - 1;

                    if (loop == g_cfg_p->m_numOfThreads) {
                        g_setupGraph_WorkData[threadIndex].stopIndex = numOfItems - 1;
                    }

                    g_CPlotPane_ThreadMananger_p->ConfigureThread(threadIndex,
                                                                  SetupGraph_ThreadAction,
                                                                  (void *)&g_setupGraph_WorkData[threadIndex]);

                    startIndex = g_setupGraph_WorkData[threadIndex].stopIndex + 1;

                    loop++;
                    threadIndex++;

                    if ((threadIndex % threadCount == 0) || (graphIndex == (numOfGraphs - 1))) {
                        threadIndex = 0;
                        g_CPlotPane_ThreadMananger_p->StartConfiguredThreads();
                        g_CPlotPane_ThreadMananger_p->WaitForAllThreads();
                    }
                } /* while */
            } /* 10000 */
            else {
                g_CPlotPane_ThreadMananger_p->ConfigureThread(threadIndex,
                                                              SetupGraph_ThreadAction,
                                                              (void *)&g_setupGraph_WorkData[threadIndex]);

                threadIndex++;

                if (threadIndex % threadCount == 0) {
                    threadIndex = 0;
                    g_CPlotPane_ThreadMananger_p->StartConfiguredThreads();
                    g_CPlotPane_ThreadMananger_p->WaitForAllThreads();
                }
            }
#endif

#ifndef MULTIPROC_SETUPGRAPH
            SetupGraph(&m_displayGraphs_a[graphIndex]);
#endif
        }
    } /* for graphs */

#ifdef MULTIPROC_SETUPGRAPH

    /* Check for remainder */
    if (threadIndex > 0) {
        g_CPlotPane_ThreadMananger_p->StartConfiguredThreads();
        g_CPlotPane_ThreadMananger_p->WaitForAllThreads();
    }
#endif

#ifdef _DEBUG
    double time = execTime.ms();
    PRINT_SUBPLOTSURFACE("CSubPlotSurface::SetupGraphs  Time:%f", time)
#endif
}

/***********************************************************************************************************************
*   SetupDecorators
***********************************************************************************************************************/
void CSubPlotSurface::SetupDecorators(void)
{
    int itemIndex;
    const int numOfItems = m_displayDecorator.m_numOfItems;

    m_setupGraphs = false;

    if (numOfItems != 0) {
        for (itemIndex = 0; itemIndex < numOfItems; ++itemIndex) {
            /* Possibly modify the location of the items to make them always visible */

            if (m_displayDecorator.m_items_a[itemIndex].properties & GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE) {
                GraphicalObject_Extents_t extents;  /*TODO: */

                m_subPlot_p->GetExtents(&extents);

                if (m_displayDecorator.m_items_a[itemIndex].properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
                    /*m_displayDecorator.m_items_a[itemIndex].go_p->x1 = m_surfaceZoom.x_min; */
                    m_displayDecorator.m_items_a[itemIndex].go_p->x1 = extents.x_min;
                    m_displayDecorator.m_items_a[itemIndex].go_p->x2 =
                        m_surfaceZoom.x_min + (m_surfaceZoom.x_max - m_surfaceZoom.x_min) * 0.1;

                    if (m_displayDecorator.m_items_a[itemIndex].properties & GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR) {
                        GO_Label_t *label_p =
                            &(reinterpret_cast<GraphicalObject_LifeLine_Box_t *>
                              (m_displayDecorator.m_items_a[itemIndex].go_p)->label);
                        int pix_length =
                            static_cast<int>(label_p->labelKind.textLabel.length * m_avgPixPerLetter);
                        pix_length = static_cast<int>(pix_length * 1.1);

                        m_displayDecorator.m_items_a[itemIndex].go_p->x2 =
                            extents.x_min + m_unitsPerPixel_X * pix_length;
                    } else if (m_displayDecorator.m_items_a[itemIndex].properties &
                               GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX) {
                        GO_Label_t *label_p = &(reinterpret_cast<GraphicalObject_LifeLine_Box_t *>
                                                (m_displayDecorator.m_items_a[itemIndex].go_p)->label);
                        int pix_length = static_cast<int>
                                         (m_label_refs_a[label_p->labelKind.labelIndex]->m_labelLength *
                                          m_avgPixPerLetter);

                        pix_length = static_cast<int>(pix_length * 1.1);

                        m_displayDecorator.m_items_a[itemIndex].go_p->x2 =
                            extents.x_min + m_unitsPerPixel_X * pix_length;
                    }
                } else {
                    m_displayDecorator.m_items_a[itemIndex].go_p->x1 = extents.x_min;
                    m_displayDecorator.m_items_a[itemIndex].go_p->x2 = m_surfaceZoom.x_max;
                }
            }
        } /* for graphs */

        SetupGraph(&m_displayDecorator);
    }
}

/***********************************************************************************************************************
*   SetupGraph_ThreadAction
***********************************************************************************************************************/
void SetupGraph_ThreadAction(void *data_p)
{
    SetupGraph_Data_t *setupGraphData_p = reinterpret_cast<SetupGraph_Data_t *>(data_p);

    setupGraphData_p->subplotSurface_p->SetupGraph(setupGraphData_p->dgraph_p,
                                                   setupGraphData_p->startIndex,
                                                   setupGraphData_p->stopIndex);
}

/***********************************************************************************************************************
*   SetupGraph
***********************************************************************************************************************/
void CSubPlotSurface::SetupGraph(CDisplayGraph *dgraph_p, int cfgStartIndex, int cfgStopIndex)
{
    /* This function setup the graph elements, and decides which ones that shall be displayed and which that shall not.
     * Used for setting up labels */
    const double avgPixPerLetter = m_avgPixPerLetter;

    if (cfgStopIndex == -1) {
        cfgStopIndex = dgraph_p->m_numOfItems - 1;
    }

    if (cfgStartIndex == -1) {
        cfgStartIndex = 0;
    }

    const int stopIndex = cfgStopIndex;

    PRINT_SUBPLOTSURFACE("SetupGraph start:%d stop:%d", cfgStartIndex, cfgStopIndex)

    if (dgraph_p->m_graph_p->isEnabled() && (stopIndex > 0)) {
        /* Find first GO inside viewPort/surface */

        int pl_1;     /* point location 1 */
        int pl_2;
        int objectIndex;

        for (objectIndex = cfgStartIndex; objectIndex <= stopIndex; ++objectIndex) {
            displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];

            /*clear the visibility bits */
            di_p->properties &= static_cast<int16_t>(~PROPERTIES_BITMASK_VISIBILITY_MASK);
            GetPointLocation(di_p->go_p, &pl_1, &pl_2);

            /* if point 1 or 2 location is within the surface first point is found */
            if ((pl_1 & X_C) || (pl_2 & X_C) || (pl_2 & X_R)) {
                break;
            }
        } /* for */

        double x1_intersect;
        double y1_intersect;
        double x2_intersect;
        double y2_intersect;

        if (g_cfg_p->m_pluginDebugBitmask > 10) {
            TRACEX_I("Setting up graph:%s  GraphicalItems:%d->%d",
                     dgraph_p->m_graph_p->GetName(),
                     objectIndex,
                     stopIndex)
        }

        /* At this step the go_p points at the first graphical object which is the first that has one
         * x_coord within the window */

        for ( ; objectIndex <= stopIndex; ++objectIndex) {
            displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];
            GraphicalObject_t *go_p = di_p->go_p;

            /*clear the visibility bits */
            di_p->properties &= static_cast<int16_t>(~PROPERTIES_BITMASK_VISIBILITY_MASK);

            GetPointLocation(go_p, &pl_1, &pl_2);

            /* Move original values to temporary intersect values that might be updated depending if the
             *  grapical object isn't fully visible.*/
            x1_intersect = go_p->x1;
            x2_intersect = go_p->x2;
            y1_intersect = go_p->y1;
            y2_intersect = go_p->y2;

            if (((pl_1 & ALL_CENTER) == ALL_CENTER) && ((pl_2 & ALL_CENTER) == ALL_CENTER)) {
                /* no intersection at all, the line is entirely inside the viewport/surface */
                di_p->properties |= PROPERTIES_BITMASK_VISIBLE;
            } else if (((pl_1 & Y_T) && (pl_2 & Y_T)) || ((pl_1 & Y_B) && (pl_2 & Y_B))) {
                /* no intersection at all, the line is entirely outside the viewport/surface */
                di_p->properties |= PROPERTIES_BITMASK_VISIBLE_X;
            } else if (pl_1 & X_R) {
                break;
            } else {
                di_p->properties |= PROPERTIES_BITMASK_VISIBLE_INTERSECT;

                /* intersection */

                if (((pl_1 & X_L) && (pl_2 & (X_C | X_R))) ||  /* Passes from left, to center or to right */
                    ((pl_1 & Y_T) && (pl_2 & (Y_C | Y_B))) ||  /* Passes from top, to center or bottom */
                    ((pl_1 & Y_B) && (pl_2 & (Y_C | Y_T)))) {
                    /* Passes from bottom, to center or top */
                    if (di_p->properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                        Intersection_LINE_Out2In(pl_1, &x1_intersect, &y1_intersect, go_p->x2, go_p->y2);
                    } else {
                        Intersection_BOX_Out2In(pl_1, &x1_intersect, &y1_intersect);
                    }
                }

                if (((pl_1 & (X_L | X_C)) && (pl_2 & X_R)) || /* Passes from left or center, to right */
                    ((pl_1 & (Y_T | Y_C)) && (pl_2 & Y_B)) ||  /* Passes from top or center, to bottom */
                    ((pl_1 & (Y_B | Y_C)) && (pl_2 & Y_T))) {
                    /* Passes from bottom or center, to top */

                    if (di_p->properties & PROPERTIES_BITMASK_KIND_LINE_MASK) {
                        Intersection_LINE_In2Out(pl_2, &x2_intersect, &y2_intersect, go_p->x1, go_p->y1);
                    } else {
                        Intersection_BOX_In2Out(pl_2, &x2_intersect, &y2_intersect);
                    }
                }
            }

            /* Truncate the graphical objects to fit to the shown area
             * If the graphical object is fully visible, or partly, draw it with the perhaps modified intersection
             * values.  */

            if (di_p->properties & (PROPERTIES_BITMASK_VISIBLE_INTERSECT | PROPERTIES_BITMASK_VISIBLE)) {
                di_p->x1_pix = m_viewPortRect.left() +
                               static_cast<int>(static_cast<double>(x1_intersect - m_surfaceZoom.x_min) *
                                                m_unitsPerPixel_X_inv);

                di_p->y1_pix = m_viewPortRect.bottom() -
                               static_cast<int>(static_cast<double>(y1_intersect - m_surfaceZoom.y_min) *
                                                m_unitsPerPixel_Y_inv);

                di_p->x2_pix = m_viewPortRect.left() +
                               static_cast<int>(static_cast<double>(x2_intersect - m_surfaceZoom.x_min) *
                                                m_unitsPerPixel_X_inv);

                di_p->y2_pix = m_viewPortRect.bottom() -
                               static_cast<int>(static_cast<double>(y2_intersect - m_surfaceZoom.y_min) *
                                                m_unitsPerPixel_Y_inv);
            }

            /* Setup label, if necessary */
            if (di_p->properties & (GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR |
                                    GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX |
                                    GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR |
                                    GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX)) {
                GO_Label_t *label_p = nullptr;
                const short properties = di_p->properties;

                if (properties & GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE) {
                    if (properties &
                        (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR | GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX)) {
                        label_p = &reinterpret_cast<GraphicalObject_LifeLine_Line_t *>(di_p->go_p)->label;
                    } else {
                        label_p = &reinterpret_cast<GraphicalObject_LifeLine_Box_t *>(di_p->go_p)->label;
                    }
                } else if (properties & (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR |
                                         GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX)) {
                    label_p = &reinterpret_cast<GraphicalObject_Line_Ex_t *>(di_p->go_p)->label;
                } else if (properties &
                           (GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR | GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX)) {
                    label_p = &reinterpret_cast<GraphicalObject_Box_Ex_t *>(di_p->go_p)->label;
                }

                if (label_p != nullptr) {
                    if (properties & (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR |
                                      GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR)) {
                        di_p->label_p = &label_p->labelKind.textLabel.label_a;
                        di_p->label_pix_length =
                            static_cast<int16_t>(static_cast<double>(label_p->labelKind.textLabel.length) *
                                                 avgPixPerLetter);
                        di_p->labelLength = label_p->labelKind.textLabel.length;
                    } else if (properties &
                               (GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX | GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX)) {
                        di_p->label_p = m_label_refs_a[label_p->labelKind.labelIndex]->m_label_p;
                        di_p->label_pix_length =
                            static_cast<int16_t>(
                                static_cast<double>(m_label_refs_a[label_p->labelKind.labelIndex]->m_labelLength) *
                                avgPixPerLetter);
                        di_p->labelLength =
                            static_cast<int16_t>(m_label_refs_a[label_p->labelKind.labelIndex]->m_labelLength);
                    }
                }
            } /* if label needs to be configured */
        } /* for loop */

        for ( ; objectIndex <= stopIndex; ++objectIndex) {
            displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];

            /*clear the visibility bits */
            di_p->properties &= static_cast<int16_t>(~PROPERTIES_BITMASK_VISIBILITY_MASK);
        }
    }
}

/***********************************************************************************************************************
*   GetPointLocation
***********************************************************************************************************************/
void CSubPlotSurface::GetPointLocation(GraphicalObject_t *go_p, int *pl_1_p, int *pl_2_p)
{
    *pl_1_p = 0;
    *pl_2_p = 0;

    /* X1 */
    if (go_p->x1 < m_surfaceZoom.x_min) {
        *pl_1_p |= X_L;
    } else if (go_p->x1 > m_surfaceZoom.x_max) {
        *pl_1_p |= X_R;
    } else {
        *pl_1_p |= X_C;
    }

    /* X2 */
    if (go_p->x2 < m_surfaceZoom.x_min) {
        *pl_2_p |= X_L;
    } else if (go_p->x2 > m_surfaceZoom.x_max) {
        *pl_2_p |= X_R;
    } else {
        *pl_2_p |= X_C;
    }

    /* Y1 */
    if (go_p->y1 < m_surfaceZoom.y_min) {
        *pl_1_p |= Y_B;
    } else if (go_p->y1 > m_surfaceZoom.y_max) {
        *pl_1_p |= Y_T;
    } else {
        *pl_1_p |= Y_C;
    }

    /* Y2 */
    if (go_p->y2 < m_surfaceZoom.y_min) {
        *pl_2_p |= Y_B;
    } else if (go_p->y2 > m_surfaceZoom.y_max) {
        *pl_2_p |= Y_T;
    } else {
        *pl_2_p |= Y_C;
    }
}

/***********************************************************************************************************************
*   GetCursorRow
***********************************************************************************************************************/
const displayItem_t *CSubPlotSurface::GetCursorRow(const QPoint *point_p, int *row_p,
                                                   double *time, double *distance_p)
{
    if ((point_p->x() >= m_DC_viewPortRect.left()) &&
        (point_p->x() <= m_DC_viewPortRect.right()) &&
        (point_p->y() >= m_DC_viewPortRect.top()) &&
        (point_p->y() <= m_DC_viewPortRect.bottom())) {
        /* Translate the point into the normalized window which is 0,0 in the top corner of each sub-plot */

        QPoint translatedPoint = *point_p;
        translatedPoint -= QPoint((m_DC_viewPortRect.left() - BOARDER_WIDTH),
                                  (m_DC_viewPortRect.top() - BOARDER_HEIGHT));

        auto di_p = FindClosest_GO(translatedPoint.x(), translatedPoint.y(), distance_p);

        if (di_p == nullptr) {
            *row_p = 0;
            return nullptr;
        }

        *row_p = di_p->go_p->row;
        *time = di_p->go_p->x2;

        if (di_p->go_p->properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
            /* A box can refer to two different rows, one for where it started and the other were it
             * ended. */
            auto x = translatedPoint.x();
            auto y = translatedPoint.y();
            auto x1 = (x - di_p->x1_pix) * (x - di_p->x1_pix);
            auto y1 = (y - di_p->y1_pix) * (y - di_p->y1_pix);

            if (almost_equal(*distance_p, sqrt(x1 + y1))) {
                *time = di_p->go_p->x1;
                *row_p = di_p->go_p->row;
            } else {
                *row_p = reinterpret_cast<GraphicalObject_Box_t *>(di_p->go_p)->row2;
            }
        }

#ifdef _DEBUG
        PRINT_SUBPLOTSURFACE("CSubPlotSurface::GetCursorRow  %s  x:%d y:%d dist:%3.2 row:%d",
                             m_subPlotTitle, point_p->x(), point_p->y(), *distance_p, *row_p)
#endif
        return di_p;
    }
    return nullptr;
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CSubPlotSurface::GetClosestGraph(QPoint *point_p,
                                      CGraph_Internal **graph_pp,
                                      double *distance_p,
                                      GraphicalObject_t **go_pp)
{
    if ((graph_pp == nullptr) || (go_pp == nullptr)) {
        TRACEX_E("CSubPlotSurface::GetGraph  INPUT ERROR")
        return false;
    }

    if ((point_p->x() >= m_DC_viewPortRect.left()) &&
        (point_p->x() <= m_DC_viewPortRect.right()) &&
        (point_p->y() >= m_DC_viewPortRect.top()) &&
        (point_p->y() <= m_DC_viewPortRect.bottom())) {
        QPoint translatedPoint = *point_p;
        translatedPoint -= QPoint((m_DC_viewPortRect.left() - BOARDER_WIDTH),
                                  (m_DC_viewPortRect.top() - BOARDER_HEIGHT));

        auto di_p = FindClosest_GO(translatedPoint.x(), translatedPoint.y(), distance_p, graph_pp);
        *go_pp = nullptr;
        if (nullptr != di_p) {
            *go_pp = di_p->go_p;
        }

        if ((*go_pp == nullptr) || (*graph_pp == nullptr)) {
            *graph_pp = nullptr;
            *go_pp = nullptr;
            return false;
        }

        PRINT_SUBPLOTSURFACE("CSubPlotSurface::GetGraph  %s  x:%d y:%d dist:%3.2 row:%d",
                             m_subPlotTitle, point_p->x(), point_p->y(), *distance_p, (*go_pp)->row)

        return true;
    }

    return false;
}

/***********************************************************************************************************************
*   GetClosestGraph
***********************************************************************************************************************/
bool CSubPlotSurface::GetClosestGraph(int row, CGraph_Internal **graph_pp, int *distance_p,
                                      GraphicalObject_t **go_pp)
{
    if ((graph_pp == nullptr) || (go_pp == nullptr)) {
        TRACEX_E("CSubPlotSurface::GetGraph  INPUT ERROR")
        return false;
    }

    *go_pp = FindClosest_GO(row, distance_p, graph_pp);

    if ((*go_pp == nullptr) || (*graph_pp == nullptr)) {
        *graph_pp = nullptr;
        *go_pp = nullptr;
        return false;
    }

    PRINT_SUBPLOTSURFACE(QString("CSubPlotSurface::GetGraph  %1  row:%2 dist:%3")
                             .arg(m_subPlotTitle).arg(row).arg(*distance_p))
    return true;
}

/***********************************************************************************************************************
*   FindClosest_GO
***********************************************************************************************************************/
const displayItem_t *CSubPlotSurface::FindClosest_GO(
    const int x,
    const int y,
    double *distance_p,
    CGraph_Internal **graph_pp)
{
    const displayItem_t *found_di_p = nullptr;
    double smallestDiff = 0.0;

    if (graph_pp != nullptr) {
        *graph_pp = nullptr;
    }

    const int numOfGraphs = m_numOfDisplayGraphs;
    int graphIndex;

    for (graphIndex = 0; graphIndex < numOfGraphs; ++graphIndex) {
        double graphSmallestDiff;
        CDisplayGraph *dgraph_p = &m_displayGraphs_a[graphIndex];
        const int numOfItems = dgraph_p->m_numOfItems;
        int objectIndex;

        if (numOfItems == 0) {
            continue;
        }

        /* Initialize the at least one match */
        int x2 = (x - dgraph_p->m_items_a[0].x2_pix) * (x - dgraph_p->m_items_a[0].x2_pix);
        int y2 = (y - dgraph_p->m_items_a[0].y2_pix) * (y - dgraph_p->m_items_a[0].y2_pix);
        graphSmallestDiff = sqrt(static_cast<double>(x2 + y2));

        if (found_di_p == nullptr) {
            smallestDiff = graphSmallestDiff;
            found_di_p = &dgraph_p->m_items_a[0];
        }

        for (objectIndex = 0; objectIndex < numOfItems; ++objectIndex) {
            const displayItem_t *di_p = &dgraph_p->m_items_a[objectIndex];

            x2 = (x - di_p->x2_pix) * (x - di_p->x2_pix);
            y2 = (y - di_p->y2_pix) * (y - di_p->y2_pix);
            graphSmallestDiff = sqrt(static_cast<double>(x2 + y2));

            if (graphSmallestDiff < smallestDiff) {
                smallestDiff = graphSmallestDiff;
                found_di_p = di_p;

                if (graph_pp != nullptr) {
                    *graph_pp = dgraph_p->m_graph_p;
                }
            }

            /* For boxes lets check the left coordiates as well. */
            if (di_p->properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
                auto x1 = (x - di_p->x1_pix) * (x - di_p->x1_pix);
                auto y1 = (y - di_p->y1_pix) * (y - di_p->y1_pix);
                graphSmallestDiff = sqrt(static_cast<double>(x1 + y1));

                if (graphSmallestDiff < smallestDiff) {
                    smallestDiff = graphSmallestDiff;
                    found_di_p = di_p;

                    if (graph_pp != nullptr) {
                        *graph_pp = dgraph_p->m_graph_p;
                    }
                }
            }
        }
    }

    *distance_p = smallestDiff;
    return found_di_p;
}

/***********************************************************************************************************************
*   FindClosest_GO
***********************************************************************************************************************/
GraphicalObject_t *CSubPlotSurface::FindClosest_GO(
    int row,
    int *distance_p,
    CGraph_Internal **graph_pp)
{
    CList_LSZ *graphList_p;
    GraphicalObject_t *found_GO_p = nullptr;
    int total_smallestDiff = 0;
    int currentDiff = 0;
    bool graphStop = false;

    if (graph_pp != nullptr) {
        *graph_pp = nullptr;
    }

    m_subPlot_p->GetGraphs(&graphList_p);

    if (graphList_p->isEmpty()) {
        return nullptr;
    }

    CGraph_Internal *graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->first());

    while (graph_p != nullptr) {
        if ((graph_p->GetNumOfObjects() > 0) && graph_p->isEnabled()) {
            int graphSmallestDiff;

            graphStop = false;

            GraphicalObject_t *go_p = graph_p->GetFirstGraphicalObject();

            if (go_p == nullptr) {
                return nullptr;
            }

            graphSmallestDiff = abs(static_cast<int>(row) - static_cast<int>(go_p->row));

            if (found_GO_p == nullptr) {
                total_smallestDiff = graphSmallestDiff;
                found_GO_p = go_p;
                if (graph_pp != nullptr) {
                    *graph_pp = graph_p;
                }
            }

            while (go_p != nullptr && !graphStop) {
                currentDiff = abs(static_cast<int>(row) - static_cast<int>(go_p->row));

                if (go_p->properties & PROPERTIES_BITMASK_KIND_BOX_MASK) {
                    int diff = abs(static_cast<int>(reinterpret_cast<GraphicalObject_Box_t *>(go_p)->row2) -
                                   static_cast<int>(row));
                    if (diff < currentDiff) {
                        currentDiff = diff;
                    }
                }

                if (currentDiff > graphSmallestDiff) {
                    /* since scanning from right to left, and row number shall be growing, if we pass row number
                     * then the diff will start to grow */
                    graphStop = true;
                } else {
                    graphSmallestDiff = currentDiff;
                }

                if (graphSmallestDiff < total_smallestDiff) {
                    total_smallestDiff = graphSmallestDiff;
                    found_GO_p = go_p;

                    if (graph_pp != nullptr) {
                        *graph_pp = graph_p;
                    }
                }

                go_p = graph_p->GetNextGraphicalObject();
            } /* while go */
        }

        graph_p = reinterpret_cast<CGraph_Internal *>(graphList_p->GetNext(reinterpret_cast<CListObject *>(graph_p)));
    } /* while graph */

    *distance_p = static_cast<int>(abs(total_smallestDiff));
    return found_GO_p;
}

/***********************************************************************************************************************
*   SetCursor
***********************************************************************************************************************/
void CSubPlotSurface::SetCursor(double cursorTime)
{
    m_cursorRowEnabled = false;
    m_cursorTimeEnabled = true;
    m_cursorRow = 0;
    m_cursorTime = cursorTime;
}

/***********************************************************************************************************************
*   DisableCursor
***********************************************************************************************************************/
void CSubPlotSurface::DisableCursor(void)
{
    m_cursorRowEnabled = false;
    m_cursorRow = 0;
}

/***********************************************************************************************************************
*   CreatePainter
***********************************************************************************************************************/
void CSubPlotSurface::CreatePainter(QWidget *widget_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if (m_double_buffer_image.size() != m_DC_windowRect.size()) {
        m_double_buffer_image = QImage(QSize(m_DC_windowRect.width(),
                                             m_DC_windowRect.height()),
                                       QImage::Format_ARGB32_Premultiplied);
    }

    m_painter_p = new LS_Painter(&m_double_buffer_image);
    m_painter_p->begin(widget_p);
    doc_p->m_fontCtrl.SetFont(m_painter_p, g_plotWnd_BlackFont_p);
    m_painter_p->setRenderHint(QPainter::Antialiasing, true);
}

/***********************************************************************************************************************
*   DestroyPainter
***********************************************************************************************************************/
void CSubPlotSurface::DestroyPainter(void)
{
    /*    m_painter_p->end(); */
    delete m_painter_p;
    m_painter_p = nullptr;
}

/***********************************************************************************************************************
*   BitBlit
***********************************************************************************************************************/
void CSubPlotSurface::BitBlit(QPainter *pDC)
{
    pDC->drawImage(m_DC_windowRect.left(), m_DC_windowRect.top(), m_double_buffer_image);
}
