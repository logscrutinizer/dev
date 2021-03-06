/*----------------------------------------------------------------------------------------------------------------------
 * */

/* File:        plugin_base.cpp
 *
 * Description: Implementation file for plugin base classes (to inherit from)
 *
 * IMPORTANT: DO NOT MODIFY THIS FILE
 *
 * ---------------------------------------------------------------------------------------------------------------------
 * */

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * Include files
 * ---------------------------------------------------------------------------------------------------------------------
 * */

#include "plugin_api.h"
#include "plugin_base.h"
#include "plugin_utils.h"
#include "plugin_text_parser.h"

#include <stdlib.h>

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * CLASS: D-TOR CDecoder
 * ---------------------------------------------------------------------------------------------------------------------
 * */
CDecoder::~CDecoder()
{}

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * CLASS: D-TOR CPlot
 * ---------------------------------------------------------------------------------------------------------------------
 * */

CPlot::~CPlot()
{}

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * CLASS: CPlot
 * Description: See plugin_base.h file
 * ---------------------------------------------------------------------------------------------------------------------
 * */
int CPlot::RegisterSubPlot(const char *title_p, const char *Y_AxisLabel)
{
    int numOfSubPlots = m_subPlots.count();
    CSubPlot *subPlot_p = new CSubPlot(title_p, numOfSubPlots, Y_AxisLabel);

    if (subPlot_p != nullptr) {
        m_subPlots.InsertTail(static_cast<CListObject *>(subPlot_p));
        m_subPlotRefs[numOfSubPlots] = subPlot_p;
        return numOfSubPlots;
    }

    return 0;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CPlot::SetSubPlotProperties(int subPlotID, SubPlot_Properties_t properties)
{
    if (m_subPlots.isEmpty()) {
        return false;
    }

    CSubPlot *subPlot_p = static_cast<CSubPlot *>(m_subPlots.first());
    while (subPlot_p != nullptr) {
        if (subPlot_p->GetID() == subPlotID) {
            subPlot_p->SetProperties(properties);
            return true;
        }
        subPlot_p = static_cast<CSubPlot *>(m_subPlots.GetNext(static_cast<CListObject *>(subPlot_p)));
    }

    return false;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CGraph *CPlot::AddGraph(int subPlotID, const char *name_p, int estimatedNumOfObjects)
{
    return (m_subPlotRefs[subPlotID]->AddGraph(name_p, estimatedNumOfObjects));
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
int CPlot::AddLabel(int subPlotID, const char *label_p, const int labelLength)
{
    auto labelLengthTemp = labelLength;
    if (labelLength > (MAX_LABEL_LENGTH - 1)) {
        labelLengthTemp = MAX_LABEL_LENGTH - 1;
    }
    return (m_subPlotRefs[subPlotID]->AddLabel(label_p, labelLengthTemp));
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CDecorator *CPlot::AddDecorator(int subPlotID)
{
    return (m_subPlotRefs[subPlotID]->AddDecorator());
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CSequenceDiagram *CPlot::AddSequenceDiagram(int subPlotID, const char *name_p, int estimatedNumOfObjects)
{
    return (m_subPlotRefs[subPlotID]->AddSequenceDiagram(name_p, estimatedNumOfObjects));
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
