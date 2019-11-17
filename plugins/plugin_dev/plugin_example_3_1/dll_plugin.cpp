/*----------------------------------------------------------------------------------------------------------------------
 * */

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * File: dll_plugin.cpp
 *
 * Description: This is Plugin Example 3_1
 *              It shows how to create a simple plugin to generate a graph (lines and boxes) out of text rows matching
 *               e.g. "Time:0 Value:1". This particular plugin execises non-continous lines
 * ----------------------------------------------------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * */

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
    return (CPlugin_DLL_API *)new CPlugin_Example_3_1;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlugin_Example_3_1::CPlugin_Example_3_1()
{
    SetPluginName("Plugin Example 3_1");
    SetPluginVersion("v1.0");
    SetPluginAuthor("LogScrutinizer Plugin DevTeam");

    SetPluginFeatures(SUPPORTED_FEATURE_PLOT);

    RegisterPlot(new CPlot_Example_3_1());
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Example_3_1::CPlot_Example_3_1()
{
    SetTitle("Plugin Example 3.1", "Time");

    m_subPlotID_Lines = RegisterSubPlot("Values", "Unit");
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Example_3_1::~CPlot_Example_3_1()
{
    PlotClean();
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_3_1::pvPlotClean(void)
{
    m_line.graph_p = nullptr;

    Trace("CPlot_Example_3_1 pvPlotClean\n");
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_3_1::pvPlotBegin(void)
{
    Trace("CPlot_Example_3_1 pvPlotBegin\n");
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_3_1::pvPlotRow(const char *row_p, const int *length_p, int rowIndex)
{
    int index = 0;

    /* Example strings in the log
     *
     * Line:0 Time1:0 Time2:2 Value1:0 Value2:1 */

    /* Initialize the parser with the string */
    m_parser.SetText(row_p, *length_p);

    /* Make sure the parser starts looking at index 0 */
    m_parser.ResetParser();

    m_parser.ResetParser();

    if (m_parser.Search("Line:", 5)) {
        /* Get the line index */
        if (!m_parser.ParseInt(&index)) {
            return;
        }

        if (m_line.graph_p == nullptr) {
            char graphName[16];
            sprintf(graphName, "line:%d", index);

            m_line.graph_p = AddGraph(m_subPlotID_Lines, graphName, 1000);
        }

        int time1;
        int value1;
        int time2;
        int value2;

        /* 1. Search and parse the "Time1:" member */
        if (!m_parser.Search("Time1:", 6)) {
            return;
        }

        /* 2. Parse the INT value that comes after the "Time1:" string */
        if (!m_parser.ParseInt(&time1)) {
            return;
        }

        /* 3. Do the same for "Time2:" */
        if (!m_parser.Search("Time2:", 6)) {
            return;
        }
        if (!m_parser.ParseInt(&time2)) {
            return;
        }

        /* 3. Search for the string "Value1:" */
        if (!m_parser.Search("Value1:", 7)) {
            return;
        }

        /* 4. Parse the INT value that comes after the "Value:" string */
        if (!m_parser.ParseInt(&value1)) {
            return;
        }

        /* x. Do the same for value 2 */
        if (!m_parser.Search("Value2:", 7)) {
            return;
        }
        if (!m_parser.ParseInt(&value2)) {
            return;
        }

        char temp[128];
        sprintf(temp, "t:(%d-%d) v:(%d-%d)", time1, time2, value1, value2);

        m_line.graph_p->AddLine((double)time1, (double)value1, (double)time2, (double)value2, rowIndex, temp,
                                static_cast<uint8_t>(strlen(temp)), Q_RGB(0xcc, 0xcc, 0xcc), 0.5f);
        return;
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_3_1::pvPlotEnd(void)
{
    Trace("CPlot_Example_3_1 pvPlotEnd\n");
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
