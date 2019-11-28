/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdio.h>
#include "plugin_api.h"
#include "plugin_utils.h"
#include "plugin_text_parser.h"
#include "dll_plugin.h"

#include <QRgb>

/***********************************************************************************************************************
*   DLL_API_Factory
***********************************************************************************************************************/
CPlugin_DLL_API *DLL_API_Factory(void)
{
    return new CPlugin_Example_6;
}

CPlugin_Example_6::CPlugin_Example_6()
{
    SetPluginName("Plugin Example 6");
    SetPluginVersion("v1.0");
    SetPluginAuthor("Robert Klang");

    SetPluginFeatures(SUPPORTED_FEATURE_PLOT);

    RegisterPlot(new CPlot_Example_6());
}

CPlot_Example_6::CPlot_Example_6()
{
    SetTitle("Plugin Example 6", "Time");

    m_subPlotID_boxes = RegisterSubPlot("Values", "Unit");
    m_subPlotID_lines = RegisterSubPlot("Values", "Unit");
    SetSubPlotProperties(m_subPlotID_boxes, SUB_PLOT_PROPERTY_PAINTING);
    SetSubPlotProperties(m_subPlotID_lines, SUB_PLOT_PROPERTY_PAINTING);
}

CPlot_Example_6::~CPlot_Example_6()
{
    PlotClean();
}

/***********************************************************************************************************************
*   pvPlotClean
***********************************************************************************************************************/
void CPlot_Example_6::pvPlotClean(void)
{
    m_graphAdded = false;
}

/***********************************************************************************************************************
*   pvPlotBegin
***********************************************************************************************************************/
void CPlot_Example_6::pvPlotBegin(void)
{}

/***********************************************************************************************************************
*   pvPlotRow
***********************************************************************************************************************/
void CPlot_Example_6::pvPlotRow(const char *row_p, const int *length_p, int rowIndex)
{
    if (!m_graphAdded) {
        m_valueGraph_boxes_p = AddGraph(m_subPlotID_boxes, "Value graph", 1000);
        m_valueGraph_lines_p = AddGraph(m_subPlotID_lines, "Value graph", 1000);
        m_graphAdded = true;
    }

    /* The "data" string in the Example_6 text log looks like this:
     *   BOX(4,0,5,10)
     *   LINE(0,0,10,10) */

    /* 1. Search for a string starting with text Time:
     * 2. Parse the integer value
     * 3. Search for Value:
     * 4. Parse the integer value *//* Initialize the parser with the string */
    m_parser.SetText(row_p, *length_p);

    /* Make sure the parser starts looking at index 0 */
    m_parser.ResetParser();

    bool box = false;
    bool line = false;

    /* 1. Search for "Time:" */
    if (m_parser.Search("BOX(", 4)) {
        box = true;
    } else if (m_parser.Search("LINE(", 5)) {
        line = true;
    } else {
        return;
    }

    if (line || box) {
        bool status;
        int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        /* 2. Parse the INT value that comes after the "Time:" string */
        status = m_parser.ParseInt(&x1);
        if (status) {
            status = m_parser.ParseInt(&y1);
        }
        if (status) {
            status = m_parser.ParseInt(&x2);
        }
        if (status) {
            status = m_parser.ParseInt(&y2);
        }

        if (!status) {
            return;
        }

        if (line) {
            /* Add a line to the graph (x1,Y1) -> (x2,y2) */
            m_valueGraph_lines_p->AddLine(static_cast<double>(x1), static_cast<double>(y1),
                                          static_cast<double>(x2), static_cast<double>(y2), rowIndex);
        } else {
            m_valueGraph_boxes_p->AddBox(static_cast<double>(x1), static_cast<double>(y1), rowIndex,
                                         static_cast<double>(x2), static_cast<double>(y2), rowIndex);
        }
    }
}

/***********************************************************************************************************************
*   pvPlotEnd
***********************************************************************************************************************/
void CPlot_Example_6::pvPlotEnd(void)
{}
