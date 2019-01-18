
#include "plugin_api.h"

class CPlugin_Example_3_1 : public CPlugin_DLL_API
{
public:
  CPlugin_Example_3_1();   
};

typedef struct 
{
  CGraph*         graph_p;

}graphInfo_t;

class CPlot_Example_3_1 : public CPlot
{
public:
  CPlot_Example_3_1();
  ~CPlot_Example_3_1();

  virtual void                        pvPlotBegin(void);
  virtual void                        pvPlotRow(const char* row_p, const unsigned int* length_p, unsigned int rowIndex);
  virtual void                        pvPlotEnd(void);
  virtual void                        pvPlotClean(void);

private:
  unsigned int                        m_subPlotID_Lines;
  CTextParser                         m_parser;
  graphInfo_t                         m_line;
};
