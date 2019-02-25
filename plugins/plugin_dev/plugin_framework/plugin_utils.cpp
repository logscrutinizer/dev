/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "plugin_utils_internal.h"
#include "plugin_utils.h"

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CList_LSZ
 * Description: See plugin_utils.h file
 * ----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::InsertAfter(CListObject *afterListObject_p, CListObject *listObject_p)
{
    listObject_p->m_parentList_p = this;

    if (afterListObject_p->m_next_p != nullptr) {
        afterListObject_p->m_next_p->m_previous_p = listObject_p;
        listObject_p->m_next_p = afterListObject_p->m_next_p;
    } else {
        listObject_p->m_next_p = nullptr;
    }

    afterListObject_p->m_next_p = listObject_p;
    listObject_p->m_previous_p = afterListObject_p;

    if (afterListObject_p == m_tail_p) {
        m_tail_p = listObject_p;
    }

    ++m_items;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::InsertBefore(CListObject *beforeListObject_p, CListObject *listObject_p)
{
    listObject_p->m_parentList_p = this;

    if (beforeListObject_p->m_previous_p != nullptr) {
        beforeListObject_p->m_previous_p->m_next_p = listObject_p;
        listObject_p->m_previous_p = beforeListObject_p->m_previous_p;
    } else {
        listObject_p->m_previous_p = nullptr;
    }

    listObject_p->m_next_p = beforeListObject_p;
    beforeListObject_p->m_previous_p = listObject_p;

    if (beforeListObject_p == m_head_p) {
        m_head_p = listObject_p;
    }

    ++m_items;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::InsertHead(CListObject *listObject_p)
{
    listObject_p->m_parentList_p = this;
    listObject_p->m_next_p = nullptr;
    listObject_p->m_previous_p = nullptr;

    if (m_head_p != nullptr) {
        m_head_p->m_previous_p = listObject_p;
        listObject_p->m_next_p = m_head_p;
    }

    if (m_tail_p == nullptr) {
        m_tail_p = listObject_p;
    }

    m_head_p = listObject_p;

    ++m_items;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::InsertTail(CListObject *listObject_p)
{
    listObject_p->m_parentList_p = this;
    listObject_p->m_next_p = nullptr;
    listObject_p->m_previous_p = nullptr;

    if (m_tail_p != nullptr) {
        m_tail_p->m_next_p = listObject_p;
        listObject_p->m_previous_p = m_tail_p;
    }

    if (m_head_p == nullptr) {
        m_head_p = listObject_p;
    }

    m_tail_p = listObject_p;

    ++m_items;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::TakeOut(CListObject *listObject_p)
{
    listObject_p->m_parentList_p = nullptr;

    if (m_head_p == listObject_p) {
        m_head_p = listObject_p->m_next_p;
        m_head_p->m_previous_p = nullptr;
    }

    if (m_tail_p == listObject_p) {
        m_tail_p = listObject_p->m_previous_p;
        m_tail_p->m_next_p = nullptr;
    }

    if (listObject_p->m_next_p != nullptr) {
        listObject_p->m_next_p->m_previous_p = listObject_p->m_previous_p;
    }

    if (listObject_p->m_previous_p != nullptr) {
        listObject_p->m_previous_p->m_next_p = listObject_p->m_next_p;
    }

    --m_items;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CList_LSZ::DeleteAll(void)
{
    CListObject *listObject_p = m_head_p;
    CListObject *nextlistObject_p = nullptr;

    if (m_head_p == nullptr) {
        return;
    }

    while (listObject_p != nullptr) {
        nextlistObject_p = listObject_p->m_next_p;
        delete listObject_p;
        listObject_p = nextlistObject_p;
    }

    m_head_p = nullptr;
    m_tail_p = nullptr;
    m_items = 0;
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CGraph
 * Description: See plugin_utils.h file */

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddLine(double x1, float y1, double x2, float y2, int row)
{
    GraphicalObject_Line_t *newLine_p =
        (GraphicalObject_Line_t *)m_byteStreamManager_p->AddBytes(sizeof(GraphicalObject_Line_t));

    if (newLine_p == nullptr) {
        ErrorHook("CGraph::AddLine failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddLine failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    if (!painting && (x1 > x2)) {
        ErrorHook("CGraph::AddLine failed, input parameter error\n");

        double temp = x2;
        x2 = x1;
        x1 = temp;
    }

    memset(newLine_p, 0, sizeof(GraphicalObject_Line_t));

    GraphicalObject_t *go_p = &newLine_p->go;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;

    go_p->properties = GRAPHICAL_OBJECT_KIND_LINE;

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddLine(double x1, float y1, double x2, float y2, int row, const char *label_p,
                     int labelLength, int lineColorRGB, float relative_X, int lineEnds)
{
    int totalObjectSize = sizeof(GraphicalObject_Line_Ex_t) + labelLength + 1;  /* +1 for EOL */
    GraphicalObject_Line_Ex_t *newLine_p =
        reinterpret_cast<GraphicalObject_Line_Ex_t *>(m_byteStreamManager_p->AddBytes(totalObjectSize));

    if (newLine_p == nullptr) {
        ErrorHook("CGraph::AddLine_Ex failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddLine failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    if (!painting && (x1 > x2)) {
        ErrorHook("CGraph::AddLine failed, input parameter error\n");

        double temp = x2;
        x2 = x1;
        x1 = temp;
    }

    memset(newLine_p, 0, totalObjectSize);

    GraphicalObject_t *go_p = &newLine_p->go;

    go_p->properties = GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR | lineEnds;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;

    newLine_p->lineColorRGB = lineColorRGB;
    newLine_p->relative_X = relative_X;
    newLine_p->label.labelKind.textLabel.length = labelLength;

    if ((label_p != nullptr) && (labelLength > 0)) {
        memcpy(&newLine_p->label.labelKind.textLabel.label_a, label_p, labelLength);
    }

    (&newLine_p->label.labelKind.textLabel.label_a)[labelLength] = 0;  /* Add EOL */

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddLine(double x1, float y1, double x2, float y2, int row, int labelIndex,
                     int lineColorRGB, float relative_X, int lineEnds)
{
    GraphicalObject_Line_Ex_t *newLine_p =
        (GraphicalObject_Line_Ex_t *)m_byteStreamManager_p->AddBytes(sizeof(GraphicalObject_Line_Ex_t));

    if (newLine_p == nullptr) {
        ErrorHook("CGraph::AddLine_Ex failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddLine failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    memset(newLine_p, 0, sizeof(GraphicalObject_Line_Ex_t));

    GraphicalObject_t *go_p = &newLine_p->go;

    go_p->properties = GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX | lineEnds;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;

    newLine_p->lineColorRGB = lineColorRGB;
    newLine_p->relative_X = relative_X;
    newLine_p->label.labelKind.labelIndex = labelIndex;

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddBox(double x1, float y1, int row, double x2, float y2, int row2)
{
    GraphicalObject_Box_t *newBox_p =
        (GraphicalObject_Box_t *)m_byteStreamManager_p->AddBytes(sizeof(GraphicalObject_Box_t));

    if (newBox_p == nullptr) {
        ErrorHook("CGraph::AddBox failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddBox failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    if (x1 > x2) {
        double temp_x = x2;

        x2 = x1;
        x1 = temp_x;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    if (y1 > y2) {
        y2 = y1;
        y1 = y2;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    memset(newBox_p, 0, sizeof(GraphicalObject_Box_t));

    GraphicalObject_t *go_p = &newBox_p->go;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;
    newBox_p->row2 = row2;

    go_p->properties = GRAPHICAL_OBJECT_KIND_BOX;

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddBox(double x1, float y1, int row, double x2, float y2, int row2, const char *label_p,
                    int labelLength, int fillColorRGB)
{
    int totalObjectSize = sizeof(GraphicalObject_Box_Ex_t) + labelLength + 1; /* +1 for EOL */
    GraphicalObject_Box_Ex_t *newBox_p = (GraphicalObject_Box_Ex_t *)m_byteStreamManager_p->AddBytes(totalObjectSize);

    if (newBox_p == nullptr) {
        ErrorHook("CGraph::AddBox failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddBox failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    if (x1 > x2) {
        double temp_x = x2;
        x2 = x1;
        x1 = temp_x;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    if (y1 > y2) {
        y2 = y1;
        y1 = y2;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    memset(newBox_p, 0, totalObjectSize);

    GraphicalObject_t *go_p = &newBox_p->go;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;
    newBox_p->row2 = row2;

    go_p->properties = GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR;

    newBox_p->fillColorRGB = fillColorRGB;
    newBox_p->label.labelKind.textLabel.length = labelLength;

    memcpy(&newBox_p->label.labelKind.textLabel.label_a, label_p, labelLength);

    (&newBox_p->label.labelKind.textLabel.label_a)[labelLength] = 0;  /* Add EOL */

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph::AddBox(double x1, float y1, int row, double x2, float y2, int row2, int labelIndex, int fillColorRGB)
{
    GraphicalObject_Box_Ex_t *newBox_p =
        (GraphicalObject_Box_Ex_t *)m_byteStreamManager_p->AddBytes(sizeof(GraphicalObject_Box_Ex_t));

    if (newBox_p == nullptr) {
        ErrorHook("CGraph::AddBox failed, out of memory\n");
        return false;
    }

    const bool painting = (m_property & GRAPH_PROPERTY_PAINTING) ? true : false;
    if (!painting && ((x1 < 0.0) || (x2 < 0.0))) {
        ErrorHook("CGraph::AddBox failed, x1(%f) or x2(%f) less than 0\n", x1, x2);
        return false;
    }

    ++m_numOfObjects;

    if (x1 > x2) {
        double temp_x = x2;
        x2 = x1;
        x1 = temp_x;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    if (y1 > y2) {
        y2 = y1;
        y1 = y2;

        ErrorHook("CGraph::AddBox failed, input parameter error\n");
    }

    memset(newBox_p, 0, sizeof(GraphicalObject_Box_Ex_t));

    GraphicalObject_t *go_p = &newBox_p->go;

    go_p->x1 = x1;
    go_p->y1 = y1;
    go_p->x2 = x2;
    go_p->y2 = y2;
    go_p->row = row;
    newBox_p->row2 = row2;

    go_p->properties = GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX;

    newBox_p->fillColorRGB = fillColorRGB;
    newBox_p->label.labelKind.labelIndex = labelIndex;

    UpdateExtents(go_p);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CGraph::SetGraphColor(int colorRGB)
{
    m_isOverrideColorSet = true;
    m_overrideColor = colorRGB;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CGraph::SetLinePattern(GraphLinePattern_e pattern)
{
    m_overrideLinePattern = pattern;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
lifeLine_h CSequenceDiagram::AddLifeLine(float y1, float y2, const char *label_p, int labelLength,
                                         int colorRGB)
{
    /* 1. Add the life line box
     * 2. Add the life line line */

    int totalObjectSize = sizeof(GraphicalObject_LifeLine_Box_t) + labelLength + 1; /*+1 EOL */

    /* Add the lifeline box in the m_decorator member */
    GraphicalObject_LifeLine_Box_t *newLifeLine_Box_p =
        (GraphicalObject_LifeLine_Box_t *)m_decorator_p->m_byteStreamManager_p->AddBytes(totalObjectSize);

    if (newLifeLine_Box_p == nullptr) {
        ErrorHook("CSequenceDiagram::AddLifeLine Box failed, out of memory\n");
        return (lifeLine_h)nullptr;
    }

#ifdef _DEBUG
    if (y1 > y2) {
        float temp_y = y2;
        y2 = y1;
        y1 = temp_y;

        ErrorHook("CSequenceDiagram::AddLifeLine y1 is larger than y2\n");
    }
#endif

    memset(newLifeLine_Box_p, 0, totalObjectSize);

    GraphicalObject_t *go_p = &newLifeLine_Box_p->go;

    go_p->x1 = 0.0;
    go_p->y1 = y1;
    go_p->x2 = 0.0;
    go_p->y2 = y2;

    const float halfHeigth = (y2 - y1) / 2.0f;

    newLifeLine_Box_p->y_center = y1 + halfHeigth;
    newLifeLine_Box_p->y_execTop = newLifeLine_Box_p->y_center + halfHeigth * 0.3f;
    newLifeLine_Box_p->y_execBottom = newLifeLine_Box_p->y_center - halfHeigth * 0.3f;

    go_p->row = 0;
    go_p->properties = GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE | GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR;

    newLifeLine_Box_p->fillColorRGB = colorRGB;
    newLifeLine_Box_p->label.labelKind.textLabel.length = labelLength;

    memcpy(&newLifeLine_Box_p->label.labelKind.textLabel.label_a, label_p, labelLength);

    ++m_decorator_p->m_numOfObjects;

    /* 2. Add the life line line */

    totalObjectSize = sizeof(GraphicalObject_LifeLine_Line_t) + labelLength;

    /* Add the lifeline box in the m_decorator member */
    GraphicalObject_LifeLine_Line_t *newLifeLine_Line_p =
        (GraphicalObject_LifeLine_Line_t *)m_decorator_p->m_byteStreamManager_p->AddBytes(totalObjectSize);

    if (newLifeLine_Line_p == nullptr) {
        ErrorHook("CSequenceDiagram::AddLifeLine Line failed, out of memory\n");
        return (lifeLine_h)nullptr;
    }

    memset(newLifeLine_Line_p, 0, totalObjectSize);

    go_p = &newLifeLine_Line_p->go;

    go_p->x1 = 0.0;
    go_p->y1 = newLifeLine_Box_p->y_center;
    go_p->x2 = 0.0;
    go_p->y2 = newLifeLine_Box_p->y_center;

    go_p->row = 0;
    go_p->properties = GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE | GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR;

    newLifeLine_Line_p->lineColorRGB = colorRGB;
    newLifeLine_Line_p->label.labelKind.textLabel.length = labelLength;
    newLifeLine_Line_p->relative_X = 0.2f;         /* 20% in on the line */

    memcpy(&newLifeLine_Line_p->label.labelKind.textLabel.label_a, label_p, labelLength);
    (&newLifeLine_Line_p->label.labelKind.textLabel.label_a)[labelLength] = 0;

    ++m_decorator_p->m_numOfObjects;

    return ((lifeLine_h)newLifeLine_Box_p);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row,
                                  int labelIndex, int colorRGB, bool synched, bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine1_p =
        (GraphicalObject_LifeLine_Box_t *)lifeLine1;
    GraphicalObject_LifeLine_Box_t *lifeLine2_p =
        (GraphicalObject_LifeLine_Box_t *)lifeLine2;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;

    if (synched) {
        lineEnds = PROPERTIES_BITMASK_LINE_ARROW_SOLID_END;
    }

    if (x < 0.0) {
        ErrorHook("%s x (%f) is less than 0.0", __FUNCTION__, x);
    }

    float y_dest = lifeLine2_p->y_center;

    if (startsExecution) {
        /* Check if the line goes up or down, in-case down then use the
         * top y coord of lifeline2, otherwise the low */

        if (lifeLine1_p->go.y1 > lifeLine1_p->go.y2) {
            y_dest = lifeLine2_p->y_execTop;
        } else {
            y_dest = lifeLine2_p->y_execBottom;
        }
    }

    AddLine(x, lifeLine1_p->y_center, x, y_dest, row, labelIndex,
            colorRGB, 0.5f, lineEnds);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row,
                                  const char *label_p, int labelLength, int colorRGB, bool synched,
                                  bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine1_p = (GraphicalObject_LifeLine_Box_t *)lifeLine1;
    GraphicalObject_LifeLine_Box_t *lifeLine2_p = (GraphicalObject_LifeLine_Box_t *)lifeLine2;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;

    if (synched) {
        lineEnds = PROPERTIES_BITMASK_LINE_ARROW_SOLID_END;
    }

    float y_dest = lifeLine2_p->y_center;

    if (startsExecution) {
        /* Check if the line goes up or down, in-case down then use the top y coord of lifeline2, otherwise the low */

        if (lifeLine1_p->go.y1 > lifeLine1_p->go.y2) {
            y_dest = lifeLine2_p->y_execTop;
        } else {
            y_dest = lifeLine2_p->y_execBottom;
        }
    }

    AddLine(x, lifeLine1_p->y_center, x, y_dest, row, label_p, labelLength, colorRGB, 0.5f, lineEnds);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddReturnMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row,
                                        int labelIndex, int colorRGB, bool fromExecution, bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine1_p = (GraphicalObject_LifeLine_Box_t *)lifeLine1;
    GraphicalObject_LifeLine_Box_t *lifeLine2_p = (GraphicalObject_LifeLine_Box_t *)lifeLine2;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;
    float y_src = lifeLine1_p->y_center;
    float y_dest = lifeLine2_p->y_center;

    if (fromExecution) {
        /* Check if the line goes up or down, in-case down then use the top y coord of lifeline2, otherwise the low */

        if (lifeLine1_p->go.y1 > lifeLine2_p->go.y1) {
            y_src = lifeLine1_p->y_execBottom;
        } else {
            y_src = lifeLine1_p->y_execTop;
        }
    }

    if (startsExecution) {
        if (lifeLine1_p->go.y1 > lifeLine2_p->go.y1) {
            y_dest = lifeLine2_p->y_execTop;
        } else {
            y_dest = lifeLine2_p->y_execBottom;
        }
    }

    AddLine(x, y_src, x, y_dest, row, labelIndex, colorRGB, 0.5f, lineEnds);
    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddReturnMessage(lifeLine_h lifeLine1, double x, lifeLine_h lifeLine2, int row,
                                        const char *label_p, int labelLength, int colorRGB,
                                        bool fromExecution, bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine1_p = (GraphicalObject_LifeLine_Box_t *)lifeLine1;
    GraphicalObject_LifeLine_Box_t *lifeLine2_p = (GraphicalObject_LifeLine_Box_t *)lifeLine2;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;
    float y_src = lifeLine1_p->y_center;
    float y_dest = lifeLine2_p->y_center;

    if (fromExecution) {
        /* Check if the line goes up or down, in-case down then use the top y coord of lifeline2, otherwise the low */

        if (lifeLine1_p->go.y1 > lifeLine2_p->go.y1) {
            y_src = lifeLine1_p->y_execBottom;
        } else {
            y_src = lifeLine1_p->y_execTop;
        }
    }

    if (startsExecution) {
        if (lifeLine1_p->go.y1 > lifeLine2_p->go.y1) {
            y_dest = lifeLine2_p->y_execTop;
        } else {
            y_dest = lifeLine2_p->y_execBottom;
        }
    }

    AddLine(x, y_src, x, y_dest, row, label_p, labelLength, colorRGB, 0.5f, lineEnds);
    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddEvent(lifeLine_h lifeLine, double x, int row, int labelIndex,
                                int colorRGB, bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine_p = (GraphicalObject_LifeLine_Box_t *)lifeLine;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;
    const float start = lifeLine_p->go.y2 + (lifeLine_p->go.y2 - lifeLine_p->y_center);

    AddLine(x, start, x, lifeLine_p->y_center, row, labelIndex, colorRGB, 0.5f, lineEnds);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddEvent(lifeLine_h lifeLine, double x, int row, const char *label_p,
                                int labelLength, int colorRGB, bool startsExecution)
{
    GraphicalObject_LifeLine_Box_t *lifeLine_p = (GraphicalObject_LifeLine_Box_t *)lifeLine;
    Object_Properties_Bitmask_t lineEnds = PROPERTIES_BITMASK_LINE_ARROW_OPEN_END;
    const float start = lifeLine_p->go.y2 + (lifeLine_p->go.y2 - lifeLine_p->y_center);

    AddLine(x, start, x, lifeLine_p->y_center, row, label_p, labelLength, colorRGB, 0.5f, lineEnds);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddExecution(lifeLine_h lifeLine, double x1, double x2, int row, int labelIndex, int colorRGB)
{
    GraphicalObject_LifeLine_Box_t *lifeLine_p = (GraphicalObject_LifeLine_Box_t *)lifeLine;

    AddBox(x1, lifeLine_p->y_execBottom, row, x2, lifeLine_p->y_execTop, row, labelIndex, colorRGB);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CSequenceDiagram::AddExecution(lifeLine_h lifeLine, double x1, double x2, int row, const char *label_p,
                                    int labelLength, int colorRGB)
{
    GraphicalObject_LifeLine_Box_t *lifeLine_p = (GraphicalObject_LifeLine_Box_t *)lifeLine;

    AddBox(x1, lifeLine_p->y_execBottom, row, x2, lifeLine_p->y_execTop, row, label_p, labelLength, colorRGB);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
