/*----------------------------------------------------------------------------------------------------------------------
 * */

/* File:        plugin_base_internal.cpp
 *
 * Description:
 *
 * IMPORTANT: DO NOT MODIFY THIS FILE
 *
 * ----------------------------------------------------------------------------------------------------------------------
 * */

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * Include files
 * ----------------------------------------------------------------------------------------------------------------------
 * */

#include "plugin_api.h"
#include "plugin_utils.h"
#include "plugin_utils_internal.h"
#include "plugin_text_parser.h"
#include <stdlib.h>

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CPlot_Internal
 * Description: See plugin_base.h file
 * ----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Internal::CPlot_Internal(void)
{
    m_title[0] = 0;
    m_X_AxisLabel[0] = 0;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CPlot_Internal::~CPlot_Internal()
{
    m_subPlots.DeleteAll();
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Internal::PlotBegin(void)
{
    pvPlotBegin();
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Internal::PlotRow(const char *row_p, const int *length_p, int rowIndex)
{
    pvPlotRow(row_p, length_p, rowIndex);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Internal::PlotClean(void)
{
    pvPlotClean();

    /* Keep the registered sub-plots however clean/reset their contents */
    CSubPlot *subPlot_p = (CSubPlot *)m_subPlots.first();
    while (subPlot_p != nullptr) {
        subPlot_p->Clean();
        subPlot_p = (CSubPlot *)m_subPlots.GetNext((CListObject *)subPlot_p);
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CPlot_Internal::PlotEnd(void)
{
    CSubPlot *subPlot_p = (CSubPlot *)m_subPlots.first();

    pvPlotEnd();

    while (subPlot_p != nullptr) {
        subPlot_p->CalcExtents();

        auto properties = subPlot_p->GetProperties();

        if (properties & SUB_PLOT_PROPERTY_SEQUENCE) {
            bool lifeLineExists = false;

            /* Go through the decorator lifelines and setup the X coordinates */
            CDecorator *decorator_p = nullptr;

            subPlot_p->GetDecorator(&decorator_p);

            if (decorator_p->GetNumOfObjects() > 0) {
                GraphicalObject_Extents_t extents;

                subPlot_p->GetExtents(&extents);

                /* Debugging */
                if (extents.x_min < 0.0) {
                    extents.x_min = 0.0;
                }

                double x_width = extents.x_max - extents.x_min;
                double x_lifeline_width = x_width * 0.1;  /* 10% of the total width is devoted to the lifeline objects
                                                           * */

                extents.x_min -= x_lifeline_width;

                GraphicalObject_t *go_p = decorator_p->GetFirstGraphicalObject();

                while (go_p != nullptr) {
                    if (go_p->properties & GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE) {
                        lifeLineExists = true;

                        go_p->x1 = extents.x_min;
                        go_p->x2 = extents.x_min + x_lifeline_width;

                        go_p = decorator_p->GetNextGraphicalObject();
                    }
                }

                if (lifeLineExists) {
                    /* Update the extents such that the life lines will fit */
                    subPlot_p->SetExtents(&extents);
                }
            }
        }

        subPlot_p = (CSubPlot *)m_subPlots.GetNext((CListObject *)subPlot_p);
    } /* while subPlots */
}
