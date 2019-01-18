
#include "plugin_api.h"

class CPlugin_Example_4 : public CPlugin_DLL_API
{
public:
  CPlugin_Example_4();   
};

class CPlot_Example_4 : public CPlot
{
public:
  CPlot_Example_4();
  ~CPlot_Example_4();

  virtual void                        pvPlotBegin(void);
  virtual void                        pvPlotRow(const char* row_p, const unsigned int* length_p, unsigned int rowIndex);
  virtual void                        pvPlotEnd(void);
  virtual void                        pvPlotClean(void);

private:

  CTextParser                         m_parser;


#define MAX_NUM_OF_LIFE_LINES   10
  lifeLine_h                          m_lifeLines_a[MAX_NUM_OF_LIFE_LINES];

  CSequenceDiagram*                   m_sequenceDiagram_p;

  unsigned int                        m_subPlotID_Sequence;
};