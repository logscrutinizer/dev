/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlugin_Example_6
***********************************************************************************************************************/
class CPlugin_Example_6 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_6();
};

/***********************************************************************************************************************
*   CPlot_Example_6
***********************************************************************************************************************/
class CPlot_Example_6 : public CPlot
{
public:
    CPlot_Example_6();
    ~CPlot_Example_6();

    virtual void pvPlotBegin(void);
    virtual void pvPlotRow(const char *row_p, const int *length_p, int rowIndex);
    virtual void pvPlotEnd(void);
    virtual void pvPlotClean(void);

private:
    int m_subPlotID_boxes;
    int m_subPlotID_lines;
    CTextParser m_parser;
    bool m_graphAdded;
    CGraph *m_valueGraph_boxes_p;
    CGraph *m_valueGraph_lines_p;
};
