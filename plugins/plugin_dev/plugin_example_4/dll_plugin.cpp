/*----------------------------------------------------------------------------------------------------------------------
 * */

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * File: dll_plugin.cpp
 *
 * Description: This is Plugin Example 4
 *              It shows how to create a simple plugin to generate a graph out of text rows matching
 *              e.g. "Time:0 Value:1"
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
    return (CPlugin_DLL_API *)new CPlugin_Example_4;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlugin_Example_4::CPlugin_Example_4()
{
    SetPluginName("Plugin Example 4");
    SetPluginVersion("v1.0");
    SetPluginAuthor("Robert Klang");

    SetPluginFeatures(SUPPORTED_FEATURE_PLOT);

    RegisterPlot(new CPlot_Example_4());
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Example_4::CPlot_Example_4()
{
    SetTitle("Plugin Example 4", "Time");

    m_subPlotID_Sequence = RegisterSubPlot("Sequence Diagram", "Unit");

    SetSubPlotProperties(m_subPlotID_Sequence, SUB_PLOT_PROPERTY_SEQUENCE);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Example_4::~CPlot_Example_4()
{
    PlotClean();
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_4::pvPlotClean(void)
{
    m_sequenceDiagram_p = nullptr;     /* The CSequenceDiagram class instance is removed by the plugin framework */

    memset(m_lifeLines_a, 0, sizeof(lifeLine_h) * MAX_NUM_OF_LIFE_LINES);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_4::pvPlotBegin(void)
{
    m_sequenceDiagram_p = AddSequenceDiagram(m_subPlotID_Sequence, "Test", 1000);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_4::pvPlotRow(const char *row_p, const int *length_p, int rowIndex)
{
    /*
     *  In the Example_4 we are parsing a text-file that contains information about execution flow. The intention
     *  is to plot the execution as a sequence diagram with events, messages, and execution bars.
     *
     *  To make it easier to read the code we first explain the "made up" syntax found in the text file:
     *
     *  time:1 op:event          id:100           dest:1
     *  time:2 op:msg            id:200     src:1 dest:2
     *  time:3 op:msg            id:300     src:2 dest:3
     *  time:4 op:exec_msg       id:start   src:1 dest:2 exec_id:read   exec_duration:2
     *  time:7 op:msg            id:400     src:3 dest:1
     *  time:8 op:sync_exec_msg  id:reload  src:1 dest:2 exec_id:reload exec_duration:2
     *
     *  OP stands for operation/command. There are
     *
     *  event:          An event occurs, there is no src life line
     *  msg:            A message is sent from one life line to another (src -> dest)
     *  exec_msg:       A asynchronous message is sent from one life line to another starting execution, execution of
     *               caller is not blocked
     *  sync_exec_msg:  As above, however this is a synchronous message hence the execution on the src life line is
     *               blocked until call returns
     *
     *  The ID of a message is put as a label.
     *
     */
    m_parser.SetText(row_p, *length_p);

    /* 2. Search for Life line.
     *      Add a life line to the sequence diagram and keep track for the lifeline handles in the m_lifeLines_a array
     **/

    if (m_parser.Search("lifeline", 8)) {
        int lifeLineDest;

        m_parser.ParseInt(&lifeLineDest);

        char lifeLineName[128];

        sprintf(lifeLineName, "LifeLine:%d", lifeLineDest);

        m_lifeLines_a[lifeLineDest] =
            m_sequenceDiagram_p->AddLifeLine((double)lifeLineDest - 0.25,
                                             (double)lifeLineDest + 0.25,
                                             lifeLineName,
                                             static_cast<uint8_t>(strlen(lifeLineName)),
                                             Q_RGB(85, 20 + 20 * lifeLineDest, 20 + 20 * lifeLineDest));
        return;
    }

    /* 2. Search for event */

    m_parser.ResetParser();

    if (m_parser.Search("op:event", 8)) {
        char msgString[10];
        int eventID;
        int lifeLineDest;
        int time;

        m_parser.Search("id:", 3);
        m_parser.ParseInt(&eventID);

        m_parser.Search("dest:", 5);
        m_parser.ParseInt(&lifeLineDest);

        m_parser.ResetParser();

        m_parser.Search("time:", 5);
        m_parser.ParseInt(&time);

        sprintf(msgString, "%d", eventID);

        m_sequenceDiagram_p->AddEvent(m_lifeLines_a[lifeLineDest],
                                      time,
                                      rowIndex,
                                      msgString,
                                      (int)strlen(msgString),
                                      Q_RGB(90, 90, 90), false);

        return;
    }

    /* 3. Search for (msg) message between two lifelines */

    m_parser.ResetParser();

    if (m_parser.Search("op:msg", 6)) {
        char msgString[10];
        int msgID;
        int lifeLineSrc;
        int lifeLineDest;
        int time;

        m_parser.Search("id:", 3);
        m_parser.ParseInt(&msgID);

        m_parser.Search("src:", 4);
        m_parser.ParseInt(&lifeLineSrc);

        m_parser.Search("dest:", 5);
        m_parser.ParseInt(&lifeLineDest);

        m_parser.ResetParser();

        m_parser.Search("time:", 5);
        m_parser.ParseInt(&time);

        sprintf(msgString, "%d", msgID);

        m_sequenceDiagram_p->AddMessage(m_lifeLines_a[lifeLineSrc],
                                        time,
                                        m_lifeLines_a[lifeLineDest],
                                        rowIndex,
                                        msgString,
                                        static_cast<uint8_t>(strlen(msgString)),
                                        Q_RGB(90, 90, 90), false);

        return;
    }

    /* 4-5. Search for messages starting execution. Since the syntax for both synchrounous and asynchronous
     * the code is made common. First check if there is a match against sync_exec_msg or exec_msg. */

    m_parser.ResetParser();

    bool exec_op = false;
    bool exec_op_sync = false;

    if (m_parser.Search("op:sync_exec_msg", 16)) {
        exec_op_sync = true;
    }

    if (!exec_op_sync) {
        m_parser.ResetParser();

        if (m_parser.Search("op:exec_msg", 11)) {
            exec_op = true;
        }
    }

    if (exec_op_sync || exec_op) {
        int lifeLineSrc;
        int lifeLineDest;
        int time;
        int startIndex;
        int endIndex;
        int duration;
        char execName[256];
        char msgString[10];
        int msgID;

        /* For the exec_msg the */

        m_parser.Search("id:", 3);
        m_parser.ParseInt(&msgID);

        /* Determine the name of the message, instead of a number */
        switch (msgID)
        {
            case 1000:
                strcpy(msgString, "Start");
                break;

            case 1001:
                strcpy(msgString, "Stop");
                break;

            case 1003:
                strcpy(msgString, "Reload");
                break;

            default:
                strcpy(msgString, "???");
                break;
        }

        m_parser.Search("src:", 4);
        m_parser.ParseInt(&lifeLineSrc);

        m_parser.Search("dest:", 5);
        m_parser.ParseInt(&lifeLineDest);

        m_parser.Search("exec_id:", 8);  /* get space after exec_msg text */
        startIndex = m_parser.GetParseIndex();
        m_parser.Search(" ", 1);  /* get space after exec_msg text */
        endIndex = m_parser.GetParseIndex() - 2;

        /* extract a text string from the row. This string contains the name of the execution bar */
        m_parser.Extract(startIndex, endIndex, execName);

        m_parser.Search("exec_duration:", 14);
        m_parser.ParseInt(&duration);

        m_parser.ResetParser();

        m_parser.Search("time:", 5);
        m_parser.ParseInt(&time);

        /* In-case of synchronous message we add a return message. To make */
        if (exec_op_sync) {
            m_sequenceDiagram_p->AddMessage(m_lifeLines_a[lifeLineSrc],
                                            time,
                                            m_lifeLines_a[lifeLineDest],
                                            rowIndex,
                                            msgString,
                                            static_cast<int>(strlen(msgString)),
                                            Q_RGB(90, 90, 90), true, true);

            m_sequenceDiagram_p->AddExecution(m_lifeLines_a[lifeLineDest],
                                              time,
                                              time + (double)duration,
                                              rowIndex,
                                              execName,
                                              static_cast<int>(strlen(execName)),
                                              Q_RGB(90, 90, 90));

            m_sequenceDiagram_p->AddReturnMessage(m_lifeLines_a[lifeLineDest],
                                                  time + (double)duration,
                                                  m_lifeLines_a[lifeLineSrc],
                                                  rowIndex,
                                                  "done",
                                                  4,
                                                  Q_RGB(90, 90, 90), true, false);
        } else {
            m_sequenceDiagram_p->AddMessage(m_lifeLines_a[lifeLineSrc],
                                            time, m_lifeLines_a[lifeLineDest],
                                            rowIndex,
                                            msgString,
                                            static_cast<int>(strlen(msgString)),
                                            Q_RGB(90, 90, 90),
                                            false,
                                            true);

            m_sequenceDiagram_p->AddExecution(m_lifeLines_a[lifeLineDest],
                                              time,
                                              time + static_cast<double>(duration),
                                              rowIndex,
                                              execName,
                                              static_cast<int>(strlen(execName)),
                                              Q_RGB(90, 90, 90));
        }
        return;
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Example_4::pvPlotEnd(void)
{}

/*----------------------------------------------------------------------------------------------------------------------
 * */
