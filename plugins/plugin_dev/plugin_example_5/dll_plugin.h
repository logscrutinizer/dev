/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlugin_Example_5
***********************************************************************************************************************/
class CPlugin_Example_5 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_5();
};

/***********************************************************************************************************************
*   CPlot_Example_5
***********************************************************************************************************************/
class CPlot_Example_5 : public CPlot
{
public:
    CPlot_Example_5();
    ~CPlot_Example_5();

    virtual void pvPlotBegin(void);
    virtual void pvPlotRow(const char *row_p, const int *length_p, int rowIndex);
    virtual void pvPlotEnd(void);
    virtual void pvPlotClean(void);
    virtual bool vPlotGraphicalObjectFeedback(const char *row_p, const int length, const double time,
                                              const int rowIndex, const CGraph *graphRef_p, char *feedbackText_p,
                                              const int maxFeedbackTextLength);

private:
    int m_subPlotID;
    CTextParser m_parser;
    bool m_graphAdded;
    CGraph *m_valueGraph_p;
    int m_prevTime;
    int m_prevValue;
};
