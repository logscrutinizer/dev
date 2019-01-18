
#include "plugin_api.h"

class CPlugin_Example_6 : public CPlugin_DLL_API
{
public:
  CPlugin_Example_6();
};

class CPlot_Example_6 : public CPlot
{
public:
  CPlot_Example_6();
  ~CPlot_Example_6();

  virtual void                        pvPlotBegin(void);
  virtual void                        pvPlotRow(const char* row_p, const unsigned int* length_p, unsigned int rowIndex);
  virtual void                        pvPlotEnd(void);
  virtual void                        pvPlotClean(void);

  virtual bool vPlotGraphicalObjectFeedback(
    const char*         row_p,
    const unsigned int  length,
    const double        time,
    const unsigned int  rowIndex,
    const CGraph*       graphRef_p,
    char*               feedbackText_p,
    const unsigned int  maxFeedbackTextLength);

private:

  unsigned int                        m_subPlotID_boxes;
  unsigned int                        m_subPlotID_lines;

  CTextParser                         m_parser;

  bool                                m_graphAdded;
  CGraph*                             m_valueGraph_boxes_p;
  CGraph*                             m_valueGraph_lines_p;
};
