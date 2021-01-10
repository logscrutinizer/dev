/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"
#include <ctime>

/***********************************************************************************************************************
*   CPlugin_Example_7
***********************************************************************************************************************/
class CPlugin_Example_7 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_7();
};

typedef struct
{
    double start_time;
    double start_value;
    CGraph *graph_p;
    int start_row;
}graph_info_t;

typedef struct
{
    int start_row;
    double start_time;
    CGraph *graph_p;
} box_info_t;

/***********************************************************************************************************************
*   CPlot_Example_7
***********************************************************************************************************************/
class CPlot_Example_7 : public CPlot
{
public:
    CPlot_Example_7();
    ~CPlot_Example_7();

    virtual void pvPlotBegin(void);
    virtual void pvPlotRow(const char *row_p, const int *length_p, int rowIndex);
    virtual void pvPlotEnd(void);
    virtual void pvPlotClean(void);

private:
    void parse_journalctl_row_time(CTextParser& parser, std::tm& tm) const;
    void parse_journalctl_header(CTextParser& parser, std::tm& tm) const;
    void set_log_start_time(CTextParser& parser);
    std::time_t make_time(std::tm& tm);

    int m_subplot_id_temp;
    std::tm m_log_start_time;
    std::time_t _log_start_time = 0;
    std::time_t _last_logged_time = 0;

    CTextParser m_parser;

    graph_info_t m_temp;
    bool first_element;
};
