/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlugin_Example_3
***********************************************************************************************************************/
class CPlugin_Example_3 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_3();
};

typedef struct
{
    double prevX;
    double prevY;
    CGraph *graph_p;
}graphInfo_t;

typedef struct
{
    int start;
    int end;
    int start_row;
    int end_row;
    CGraph *graph_p;
} boxInfo_t;

/***********************************************************************************************************************
*   CPlot_Example_3
***********************************************************************************************************************/
class CPlot_Example_3 : public CPlot
{
public:
    CPlot_Example_3();
    ~CPlot_Example_3();

    virtual void pvPlotBegin(void);
    virtual void pvPlotRow(const char *row_p, const int *length_p, int rowIndex);
    virtual void pvPlotEnd(void);
    virtual void pvPlotClean(void);

private:
    int m_subPlotID_Lines;
    int m_subPlotID_Boxes;
    int m_subPlotID_Lines_OverrideColor;
    int m_subPlotID_Lines_OverridePattern;
    CTextParser m_parser;
    graphInfo_t m_lines_a[2];
    boxInfo_t m_boxes_a[2];
    int m_currentBox_0_index;
    CGraph *m_graph_OverrideColor_1_p;
    CGraph *m_graph_OverrideColor_2_p;
    CGraph *m_graph_OverridePattern_1_p;
    CGraph *m_graph_OverridePattern_2_p;
    int m_subPlot_Lines_labelIndex_0;
    int m_subPlot_Boxes_labelIndex_0;
};
