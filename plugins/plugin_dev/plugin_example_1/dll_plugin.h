
#include "plugin_api.h"

class CPlugin_Example_1 : public CPlugin_DLL_API
{
public:
  CPlugin_Example_1();   
};

class CPlot_Example_1 : public CPlot
{
public:
  CPlot_Example_1();
  ~CPlot_Example_1();

  virtual void                        pvPlotBegin(void);
  virtual void                        pvPlotRow(const char* row_p, const unsigned int* length_p, unsigned int rowIndex);
  virtual void                        pvPlotEnd(void);
  virtual void                        pvPlotClean(void);

private:

  unsigned int                        m_subPlotID;

  CTextParser                         m_parser;

  bool                                m_graphAdded;
  CGraph*                             m_valueGraph_p;

  int                                 m_prevTime;
  int                                 m_prevValue;
};
