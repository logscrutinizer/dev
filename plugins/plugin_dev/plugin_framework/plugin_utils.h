/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef PLUGIN_UTILS_H
#define PLUGIN_UTILS_H

/*----------------------------------------------------------------------------------------------------------------------
 *   File: plugin_utils.h
 *   Description:
 *   This header file contains utilities for the plugin framework
 *   IMPORTANT: DO NOT MODIFY THIS FILE
 *
 * ----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include "plugin_constants.h"
#include "plugin_utils_internal.h"

extern void ErrorHook(const char *errorMsg, ...);
extern void Trace(const char *pcStr, ...);

#undef Q_RGB
#define Q_RGB(R, G, B) (static_cast<QRgb>((R) << 16 | (G) << 8 | (B)))

/*----------------------------------------------------------------------------------------------------------------------
 *   --- Exported Types
 * -----------------------------------------------------------------------------------------------------
 *
 * ----------------------------------------------------------------------------------------------------------------------*/
typedef int *lifeLine_h; /* A life line handle */

/*----------------------------------------------------------------------------------------------------------------------
 *   CLASS:       CGraph
 *   Description: A graph is line in a plot connected by several points. The graph contains a bytestream manager where
 *              graphical objects are stored (when running the plot function from LogScrutinizer).
 *              After the graph has been created use AddLine member function to add lines
 *
 * ----------------------------------------------------------------------------------------------------------------------*/
class CGraph : public CGraph_Internal
{
public:
    CGraph(const char *name_p, int subPlotID, int estimatedNumOfObjects)
        : CGraph_Internal(name_p, subPlotID, estimatedNumOfObjects) {}

    friend class CSubPlot;

    /* Function: AddLine
     *     Description: Draws a line from x1,y1 to x2,y2. LogScrutinizer decides line color and legend color
     *           IMPORTANT: X coordinates should always be specified in seconds, if applicable
     *     Parameters
     *     Input:         x1,y1          Coordinate of upper or lower left line start.
     *     Input:         x2,y2          Coordinate of upper or lower right line end
     *  Input:         row            Specifies the row in the log where this box was extracted from.
     *                                  Used for getting to and from log and plot
     */
    bool AddLine(double x1, double y1, double x2, double y2, int row);

    /* Function: AddLine
     *  Description: Draws a line from x1,y1 to x2,y2 with specified label and color.
     *           If the label is the same for many boxes then register the label and use the
     *           a AddLine function where you can provide labelIndex
     *           IMPORTANT: X coordinates should always be specified in seconds, if applicable
     *
     *  Parameters
     *  Input:         x1,y1          Coordinate of upper or lower left line start
     *  Input:         x2,y2          Coordinate of upper or
     *  Input:         row            Specifies the row in the log where this box was extracted from. Used for
     *                              getting to and from log and plot
     *  Input:         label_p,       Character string (0 as EOL not needed). May be nullptr
     *  Input:         labelLength    Number of characters in the label_p string. Length doesn't include any 0 as EOL.
     *                              If label_p is nullptr then length shall be 0
     *  Input:         lineColorRGB   The color to draw the line with, use Q_RGB macro to define value. Use -1 to let
     *                              LogScrutinizer chose color
     *  Input:         relative_X     The position of the label in % of the line length, 0.5 means in the middle
     *  Input:         lineEnds       Adds arrow to the line end, default none. Use these in the
     *                              bitmask: PROPERTIES_BITMASK_LINE_ARROW_OPEN_START,
     *                              PROPERTIES_BITMASK_LINE_ARROW_SOLID_START,
     *                                  PROPERTIES_BITMASK_LINE_ARROW_OPEN_END PROPERTIES_BITMASK_LINE_ARROW_SOLID_END
     */
    bool AddLine(double x1, double y1, double x2, double y2, int row, const char *label_p,
                 int labelLength, int lineColorRGB, double relative_X, int lineEnds = 0);

    /* Function: AddLine
     *
     *  Description: Draws a line from x1,y1 to x2,y2 with specified label from index and color.
     *           If the label is the same for many boxes then register the label and use the
     *           a AddLine function where you can provide labelIndex
     *           IMPORTANT: X coordinates should always be specified in seconds, if applicable
     *
     *  Parameters
     *  Input:         x1,y1          Coordinate of upper or lower left line start
     *  Input:         x2,y2          Coordinate of upper or
     *  Input:         row            Specifies the row in the log where this box was extracted from. Used for getting
     *                              to and from log and plot
     *  Input:         laberlIndex    Index to the label, the index was the number provided when calling CPlot::AddLabel
     *  Input:         lineColorRGB   The color to draw the line with, use Q_RGB macro to define value. Use -1 to let
     *                              LogScrutinizer chose color
     *  Input:         relative_X     The position of the label in % of the line length, 0.5 means in the middle
     *  Input:         lineEnds       Adds arrow to the line end, default none. Use these in the
     *                              bitmask: PROPERTIES_BITMASK_LINE_ARROW_OPEN_START,
     *                              PROPERTIES_BITMASK_LINE_ARROW_SOLID_START,
     *                                  PROPERTIES_BITMASK_LINE_ARROW_OPEN_END PROPERTIES_BITMASK_LINE_ARROW_SOLID_END
     */
    bool AddLine(double x1, double y1, double x2, double y2, int row, int labelIndex, int lineColorRGB,
                 double relative_X, int lineEnds = 0);

    /* Function: AddBox
     *  Description:   LogScrutinizer decides fill and line color. No label
     *             IMPORTANT: X coordinates should always be specified in seconds, if applicable
     *  Parameters
     *  Input:         x1,y1          Coordinate of LOWER left box corner
     *  Input:         row            Specifies the row in the log where this box x1,y1 was extracted from. Used for
     *                              getting to and from log and plot
     *  Input:         x2,y2          Coordinate of UPPER right box corner
     *  Input:         row2           As for row but for x2,y2
     *  NOTE: if x2,y2 isn't the upper right corner the box might not be displayed.
     */
    bool AddBox(double x1, double y1, int row, double x2, double y2, int row2);

    /* Function: AddBox
     *
     *  Description:   Draws a box with specified label and fill color.
     *
     *             IMPORTANT: X coordinates should always be specified in seconds, if applicable
     *
     *  Parameters
     *  Input:         x1,y1          Coordinate of LOWER left box corner
     *  Input:         row            Specifies the row in the log where this box x1,y1 was extracted from. Used for
     *                              getting to and from log and plot
     *  Input:         x2,y2          Coordinate of UPPER right box corner
     *  Input:         row2           As for row but for x2,y2
     *  Input:         label_p,       Character string (0 as EOL not needed)
     *  Input:         labelLength    Number of characters in the label_p string. Length doesn't include any 0 as EOL
     *  Input:         fillColorRGB   The color to fill the box with, specified in RBG (use Q_RGB macro), -1 to use
     *                              legend color
     *
     *  NOTE: if x2,y2 isn't the upper right corner the box might not be displayed.
     */
    bool AddBox(double x1, double y1, int row, double x2, double y2, int row2, const char *label_p,
                int labelLength, int fillColorRGB);

    /* Function: AddBox
     *
     *  Description: LogScrutinizer decides fill and line color, label provided as index (specified with AddLabel)
     *           X coordinates should always be specified in seconds, if applicable
     *
     *  Parameters
     *  Input:         x1,y1          Coordinate of LOWER left box corner
     *  Input:         row            Specifies the row in the log where this box x1,y1 was extracted from. Used for
     *                              getting to and from log and plot
     *  Input:         x2,y2          Coordinate of UPPER right box corner
     *  Input:         row2           As for row but for x2,y2
     *  Input:         laberlIndex    Index to the label, the index was the number provided when calling CPlot::AddLabel
     *  Input:         fillColorRGB   The color to fill the box with, specified in RBG (use Q_RGB macro), -1 to use
     *                              legend color
     *
     *  NOTE: if x2,y2 isn't the upper right corner the box might not be displayed.
     */
    bool AddBox(double x1, double y1, int row, double x2, double y2, int row2, int labelIndex, int fillColorRGB);

    /* Function: SetGraphColor
     *
     *  Description: Manual override of the LogScrutinizer color enumeration of graphs. This can be used if the plugin
     *           itself want to decide the color of each graph. E.g. the plugin may identify that a group of graphs
     *           in a subplot should have similar color, not identical, but since they all may be a bit blueish it
     *           is easy to understand that there is a relation.
     *  Parameters
     *  Input:         colorRGB          Color defined as Q_RGB
     */
    void SetGraphColor(int colorRGB);

    /* Function: SetLinePattern
     *  Description: Manual override of the LogScrutinizer line pattern enumeration of graphs. This can be used if the
     * plugin
     *           itself want to decide the pattern of each graph.
     *           E.g. the plugin may identify that a group of graphs in a subplot should have similar color, not
     *           identical, but since they all may be a bit blueish it is easy to understand that there is a relation,
     *           to create even better seperation then the pattern may be added on-top
     *  Parameters
     *  Input:         pattern
     */
    void SetLinePattern(GraphLinePattern_e pattern);

protected:
    CGraph() = delete;

    /* IMPORTANT: The dll_plugin shall NOT delete the CGraph* it got when doing AddGraph(...).
     *         This is done at every re-run automatically by the framework
     */
    virtual ~CGraph() {}
};

/*
 *
 *------------------------------------------------------------------------------------------------------------------------
 *
 *  CLASS:       CDecorator
 *
 *  Description: The CDecorator is very similar to a graph, although it provides some additional member functions to
 *             decorate a sub-plot with e.g. static lines, boxes and labels.
 *             The plugin framework uses the CDecorator to add lineline boxes/lines.
 *             NOTE: Currently there is no exported functions (under construction)
 *
 *----------------------------------------------------------------------------------------------------------------------
 */

class CDecorator : public CGraph_Internal
{
    friend class CSequenceDiagram;
    friend class CSubPlot;

public:
    CDecorator(int subPlotID)
        : CGraph_Internal("Decoration", subPlotID, 100) {}

protected:
    CDecorator() = delete;

    /*IMPORTANT: The dll_plugin shall NOT delete the CDecorator* it got when doing AddDecorator(...). This is done at
     * every re-run automatically by the framework*/
    ~CDecorator() {}
};

/*
 *
 *----------------------------------------------------------------------------------------------------------------------
 *  CLASS:       CSequenceDiagram
 *  Description: This class is used to create sequence diagrams. Use the member functions to add lifelines and messages
 * etc.
 *             IMPORTANT: Typically the CSequenceDiagram is dependent that the CDecorator is used as well to e.g. add
 *                        LifeLines
 *             IMPORTANT: To use the CSequenceDiagram the containing sub-plot must be given the
 *                        property SUB_PLOT_PROPERTY_SEQUENCE
 *
 *----------------------------------------------------------------------------------------------------------------------
 */

class CSequenceDiagram : public CGraph
{
public:
    CSequenceDiagram(const char *name_p, int subPlotID, CDecorator *decorator_p,
                     int estimatedNumOfObjects)
        : CGraph(name_p, subPlotID, estimatedNumOfObjects)
    {
        m_decorator_p = decorator_p;
    }

    friend class CSubPlot;

    /* Function: AddLifeLine
     *  Description:   Draws a box with specified label and fill color to the left. The life line box will be drawn left
     * most,
     *             its width will be large enough to contain the label.
     *  Parameters
     *  Input:         y1,y2           Position and heigth of the life line box, the width will be adjusted to the
     *                               label. IMPORTANT: y2 shall be larger than y1
     *  Input:         label_p,        Character string (0 as EOL not needed)
     *  Input:         labelLength     Number of characters in the label_p string. Length doesn't include any 0 as EOL
     *  Input:         colorRGB        The color to fill the box with, specified in RBG (use Q_RGB macro), -1 to use
     *                               legend color
     *  Returns:       lifeLine_h      A handle to a lifeLine that shall typically be supplied in many of the
     *                               CSequenceDiagram functions to add graphical objects
     */
    lifeLine_h AddLifeLine(double y1, double y2, const char *label_p, int labelLength, int colorRGB);

    /* Function: AddMessage
     *  Description: This function adds a message from lifeLine1 to lifeLine2, it will result in a line with open arrow
     * head
     *           if synched = false (otherwise filled)
     *  Parameters
     *  Input:         x                 At which time the message occur (in seconds)
     *  Input:         lifeLine1         From
     *  Input:         lifeLine2         To
     *  Input:         startsExecution   If false, the line will be drawn to the life line, otherwise it will end in
     *                                 y-led where a corner of the execution box would be added
     */
    bool AddMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row, int labelIndex,
                    int colorRGB, bool synched = false, bool startsExecution = false);

    bool AddMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row, const char *label_p,
                    int labelLength, int colorRGB, bool synched = false, bool startsExecution = false);

    /* Function: AddReturnMessage
     *  Description: This function adds a return message from lifeLine1 to lifeLine2, it will always result in a open
     *           arrow head. It is typically used as a response to a synched message
     *  Parameters
     *  Input:         x                 At which time the message occurs (in seconds)
     *  Input:         lifeLine1         From
     *  Input:         lifeLine2         To
     *  Input:         startsExecution   If false, the line will be drawn to the life line, otherwise it will end in
     *                                 y-led where a corner of the execution box would be added
     */
    bool AddReturnMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row,
                          int labelIndex, int colorRGB, bool fromExecution = false,
                          bool startsExecution = false);

    bool AddReturnMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row, const char *label_p,
                          int labelLength, int colorRGB, bool fromExecution = false, bool startsExecution = false);

    /* Function: AddEvent
     *  Description: This is an event that occurs from a "life line" that doesn't exist in the graph. It will be drawn
     * as an
     *           open arrow to the life line
     *  Parameters
     *  Input:         x                 At which time the event occurs (in seconds)
     *  Input:         lifeLine          To
     *  Input:         startsExecution   If false, the line will be drawn to the life line, otherwise it will end in
     *                                 y-led where a corner of the execution box would be added
     */
    bool AddEvent(lifeLine_h lifeLine, double x, int row, int labelIndex, int colorRGB,
                  bool startsExecution = false);
    bool AddEvent(lifeLine_h lifeLine, double x, int row, const char *label_p, int labelLength,
                  int colorRGB, bool startsExecution = false);

    /* Function: AddExecution
     *  Description: This displays a box on the life line, showing processing time. The height of the execution box will
     * be
     *           equal to the height of the life line
     *  Parameters
     *  Input:         x                 At which time the message occur (in seconds)
     *  Input:         lifeLine          Which lifeLine that shall be assigned the processing
     */
    bool AddExecution(lifeLine_h lifeLine, double x1, double x2, int row, int labelIndex,
                      int colorRGB);
    bool AddExecution(lifeLine_h lifeLine, double x1, double x2, int row, const char *label_p,
                      int labelLength, int colorRGB);

protected:
    CSequenceDiagram() = delete;

    /*IMPORTANT: The dll_plugin shall NOT delete the CGraph* it got when doing AddGraph(...). This is done at every
     * re-run automatically by the framework*/
    virtual ~CSequenceDiagram() {}

    CDecorator *m_decorator_p;
};

#endif
