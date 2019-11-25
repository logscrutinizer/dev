/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlugin_Example_1
***********************************************************************************************************************/
class CPlugin_Example_1 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_1();
};

/***********************************************************************************************************************
*   CPlot_Example_1
***********************************************************************************************************************/
class CPlot_Example_1 : public CPlot
{
public:
    CPlot_Example_1();
    ~CPlot_Example_1();

    virtual void pvPlotBegin(void);
    virtual void pvPlotRow(const char *row_p, const int *length_p, int rowIndex);
    virtual void pvPlotEnd(void);
    virtual void pvPlotClean(void);

private:
    int m_subPlotID;
    CTextParser m_parser;
    bool m_graphAdded;
    CGraph *m_valueGraph_p;
    int m_prevTime;
    int m_prevValue;
};
