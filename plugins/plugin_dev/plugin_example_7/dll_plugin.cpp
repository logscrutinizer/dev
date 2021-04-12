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
#include <functional>

/***********************************************************************************************************************
*   DLL_API_Factory
***********************************************************************************************************************/
CPlugin_DLL_API *DLL_API_Factory(void)
{
    return new CPlugin_Example_7;
}

CPlugin_Example_7::CPlugin_Example_7()
{
    SetPluginName("Plugin Example 7");
    SetPluginVersion("v1.0");
    SetPluginAuthor("Robert Klang");

    SetPluginFeatures(SUPPORTED_FEATURE_PLOT | SUPPORTED_FEATURE_PLOT_UNIX_TIME);

    RegisterPlot(new CPlot_Example_7());
}

CPlot_Example_7::CPlot_Example_7()
{
    SetTitle("Plugin Example 7", "Time");
    m_subplot_id_temp = RegisterSubPlot("Temperature", "Celsius");
}

CPlot_Example_7::~CPlot_Example_7()
{
    PlotClean();
}

/***********************************************************************************************************************
*   pvPlotClean
***********************************************************************************************************************/
void CPlot_Example_7::pvPlotClean(void)
{
    memset(&m_temp, 0, sizeof(m_temp));
    memset(&m_log_start_time, 0, sizeof(m_log_start_time));
    Trace("CPlot_Example_7 pvPlotClean\n");
}

/***********************************************************************************************************************
*   pvPlotBegin
***********************************************************************************************************************/
void CPlot_Example_7::pvPlotBegin(void)
{
    Trace("CPlot_Example_7 pvPlotBegin\n");
    m_temp.graph_p = AddGraph(m_subplot_id_temp, "temp", 1000);
}

/***********************************************************************************************************************
*   make_time
***********************************************************************************************************************/
std::time_t CPlot_Example_7::make_time(std::tm& tm)
{
    std::time_t t = std::mktime(&tm);
    _last_logged_time = t;
    return t;
}

/***********************************************************************************************************************
*   set_log_start_time
***********************************************************************************************************************/
void CPlot_Example_7::set_log_start_time(CTextParser& parser)
{
    memset(&m_log_start_time, 0, sizeof(m_log_start_time));
    parser.ResetParser();

    if (parser.Search("Logs begin at", 13)) {
        parse_journalctl_header(parser, m_log_start_time);
    } else {
        parse_journalctl_row_time(parser, m_log_start_time);
        if (m_log_start_time.tm_year == 0) {
            m_log_start_time.tm_year = 120; /* since 1900 -> 2020 */
        }
    }
}

/*
 * tm_sec   int seconds after the minute    0-60*
 *  tm_min  int minutes after the hour  0-59
 *  tm_hour int hours since midnight    0-23
 *  tm_mday int day of the month    1-31
 *  tm_mon  int months since January    0-11
 *  tm_year int years since 1900
 *  tm_wday int days since Sunday   0-6
 *  tm_yday int days since January 1    0-365
 *  tm_isdst    int Daylight Saving Time flag
 */
void CPlot_Example_7::parse_journalctl_header(CTextParser& parser, std::tm& tm) const
{
    /* -- Logs begin at Fri 2020-12-04 08:04:29 UTC, end at Sun 2020-12-13 12:57:47 UTC. -- */
    memset(&tm, 0, sizeof(tm));

    unsigned int ui;

    parser.ParseUInt(&ui);
    tm.tm_year = ui - 1900;

    parser.ParseUInt(&ui);
    tm.tm_mon = ui - 1;

    parser.ParseUInt(&ui);
    tm.tm_mday = ui;

    parser.ParseUInt(&ui);
    tm.tm_hour = ui;

    parser.ParseUInt(&ui);
    tm.tm_min = ui;

    parser.ParseUInt(&ui);
    tm.tm_sec = ui;

    tm.tm_isdst = -1;
}

/***********************************************************************************************************************
*   parse_journalctl_row_time
***********************************************************************************************************************/
void CPlot_Example_7::parse_journalctl_row_time(CTextParser& parser, std::tm& tm) const
{
    /* Nov 15 19:23:23 raspberrypi4 */
    memset(&tm, 0, sizeof(tm));

    parser.ResetParser();

    static const char months[12][4] =
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    auto p = parser.GetText();

    while (*p == ' ') {
        p++; /* skip initial spaces */
    }

    char month[4];
    memset(month, 0, sizeof(month));

    auto i = 0;

    while (*p >= 'A' && *p <= 'z' && i < 3) {
        month[i] = *p;
        p++;
        i++;
    }

    auto month_idx = 0;
    for ( ; month_idx < 12; month_idx++) {
        if ((month[0] == months[month_idx][0]) && (month[1] == months[month_idx][1]) &&
            (month[2] == months[month_idx][2])) {
            break;
        }
    }

    tm.tm_mon = std::min(11, month_idx);

    unsigned int ui;

    parser.ParseUInt(&ui);
    tm.tm_mday = ui;

    parser.ParseUInt(&ui);
    tm.tm_hour = ui;

    parser.ParseUInt(&ui);
    tm.tm_min = ui;

    parser.ParseUInt(&ui);
    tm.tm_sec = ui;

    tm.tm_year = m_log_start_time.tm_year;

    if (tm.tm_mon < m_log_start_time.tm_mon) {
        /* year wrap */
        tm.tm_year++;
    }

    tm.tm_isdst = -1;
}

/***********************************************************************************************************************
*   pvPlotRow
***********************************************************************************************************************/
void CPlot_Example_7::pvPlotRow(const char *row_p, const int *length_p, int rowIndex)
{
    bool status = true;

    m_parser.SetText(row_p, *length_p);
    m_parser.ResetParser();

    if (m_log_start_time.tm_year == 0) {
        set_log_start_time(m_parser);
    }

    /* Make sure the parser starts looking at index 0 */
    m_parser.ResetParser();

    /* Temperature: 1558.2 (1558.3) 89 2.2 1 */
    if (m_parser.Search("temp:", 5)) {
        if (first_element) {
            /*set_log_start_time(m_parser); */
            first_element = false;
        }

        /* Get the line index, in this example it is either 0 or 1 */
        double temp;
        status = m_parser.ParseDouble(&temp);

        std::tm tm;
        parse_journalctl_row_time(m_parser, tm);

        auto now = static_cast<double>(std::mktime(&tm));

        if (m_temp.start_time != 0) {
            m_temp.graph_p->AddLine(m_temp.start_time, m_temp.start_value, now, temp, rowIndex);
        }

        m_temp.start_time = now;
        m_temp.start_value = temp;
    }
}

/***********************************************************************************************************************
*   pvPlotEnd
***********************************************************************************************************************/
void CPlot_Example_7::pvPlotEnd(void)
{
    Trace("CPlot_Example_7 pvPlotEnd\n");
}
