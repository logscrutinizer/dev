
#include "plugin_api.h"

class CPlugin_Example_3 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_3();
};

typedef struct 
{
    double          prevX;
    float           prevY;
    CGraph*         graph_p;
}graphInfo_t;

typedef struct
{
    int start;
    int end;
    unsigned int start_row;
    unsigned int end_row;
    CGraph* graph_p;
} boxInfo_t;

class CPlot_Example_3 : public CPlot
{
public:
    CPlot_Example_3();
    ~CPlot_Example_3();

    virtual void                        pvPlotBegin(void);
    virtual void                        pvPlotRow(const char* row_p, const unsigned int* length_p, unsigned int rowIndex);
    virtual void                        pvPlotEnd(void);
    virtual void                        pvPlotClean(void);

private:

    unsigned int                        m_subPlotID_Lines;
    unsigned int                        m_subPlotID_Boxes;
    unsigned int                        m_subPlotID_Lines_OverrideColor;
    unsigned int                        m_subPlotID_Lines_OverridePattern;

    CTextParser                         m_parser;

    graphInfo_t                         m_lines_a[2];
    boxInfo_t                           m_boxes_a[2];
    int                                 m_currentBox_0_index;

    CGraph*                             m_graph_OverrideColor_1_p;
    CGraph*                             m_graph_OverrideColor_2_p;

    CGraph*                             m_graph_OverridePattern_1_p;
    CGraph*                             m_graph_OverridePattern_2_p;

    int                                 m_subPlot_Lines_labelIndex_0;
    int                                 m_subPlot_Boxes_labelIndex_0;
};
