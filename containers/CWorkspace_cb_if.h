/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

/* This file is used for exporting functions to access the workspace without introducing a circular dependency
 * cb_if  is abbreavation for callback interface  */

#pragma once

#include "CCfgItem.h"
#include <QList>
#include <QString>
#include "CFilter.h"

CCfgItem_FilterItem *CWorkspace_AddFilterItem(char *filterText_p, CCfgItem_FilterItem *cfgFilterItem_pp,
                                              CCfgItem_Filter *parentFilter_p);
CCfgItem_Filter *CWorkspace_CreateCfgFilter(CFilter *filterRef_p);
bool CWorkspace_CleanAllPlots(void);
void CWorkspace_CloseAllPlugins(void);
void CWorkspace_CloseAllSelectedPlugins(void);
void CWorkspace_DisableFilterItem(int uniqueID);
void CWorkspace_FilterItemProperties(int uniqueID, QWidget *widget_p);
void CWorkspace_GetMatchingFilters(QList<CCfgItem_FilterItem *> *filterItemList_p,
                                   const QString& match);
void CWorkspace_GetBookmarks(QList<int> *bookmarks);

CCfgItem_Plugin *CWorkspace_GetSelectedPlugin(CCfgItem *selection_p = nullptr);
QList<CCfgItem *> *CWorkspace_GetFilterList(void);
bool CWorkspace_GetLastSelectedFilterItem(QString *string_p);
int CWorkspace_GetEnabledFilterItemCount(void);
void CWorkspace_GetFiltersHash(QByteArray& data);

CCfgItem *CWorkspace_GetPlotRoot(CCfgItem_Plugin *plugin_p);
CCfgItem_Plot *CWorkspace_GetSelectedPlot(void);
bool CWorkspace_isAllSelections(CfgItemKind_t selectionKind);
bool CWorkspace_isPlugInPlotSelected(void);
void CWorkspace_ItemUpdated(CCfgItem *cfgItem_p);
int CWorkspace_PlotExtractTime(const char *row_p, const int length, double *time_p);
bool CWorkspace_PluginsExists(void);
bool CWorkspace_PlugInPlotsExists(void);
void CWorkspace_PlugInPlotRunSelected(CPlot *plot_p = nullptr);
void CWorkspace_RemoveAllBookmarks(void);
void CWorkspace_RemoveLog(void);
void CWorkspace_RemoveSelectedFilter(void);
void CWorkspace_RemoveSelectedFilterItem(void);
void CWorkspace_RunAllPlots(void);

CCfgItem_Plot *CWorkspace_SearchPlot(CPlot *childPlot_p);
CPlugin_DLL_API *CWorkspace_SearchPlugin(CPlot *childPlot_p);
void CWorkspace_SetFocus(void);
bool CWorkspace_TreeView_GetSelections(CfgItemKind_t kind, QList<CCfgItem *>& selectionList);
bool CWorkspace_TreeView_GetSelectionIndexes(QModelIndexList& selectionList);
void CWorkspace_TreeView_UnselectAll(void);
void CWorkspace_TreeView_Unselect(CCfgItem *cfgItem_p);
void CWorkspace_TreeView_Select(CCfgItem *cfgItem_p);

/* DO NOT USE - use the scope guard variants instead */
void CWorkspace_BeginReset(void);
void CWorkspace_EndReset(void);
void CWorkspace_BeginInsertRows(CCfgItem *parent, const CCfgItem *before, int count);
void CWorkspace_EndInsertRows(void);
void CWorkspace_BeginRemoveRows(CCfgItem *parent, const CCfgItem *startItem, int count);
void CWorkspace_EndRemoveRows(void);

void CWorkspace_LayoutAboutToBeChanged(void);
void CWorkspace_LayoutChanged(void);

/***********************************************************************************************************************
*   CWorkspace_ResetScopeGuard
***********************************************************************************************************************/
class CWorkspace_ResetScopeGuard
{
public:
    explicit CWorkspace_ResetScopeGuard() {CWorkspace_BeginReset();}
    ~CWorkspace_ResetScopeGuard() {CWorkspace_EndReset();}
};

/***********************************************************************************************************************
*   CWorkspace_InsertRowsScopeGuard
***********************************************************************************************************************/
class CWorkspace_InsertRowsScopeGuard
{
public:
    explicit CWorkspace_InsertRowsScopeGuard(CCfgItem *parent, const CCfgItem *before, int count)
    {
        CWorkspace_BeginInsertRows(parent, before, count);
    }

    ~CWorkspace_InsertRowsScopeGuard()
    {
        CWorkspace_EndInsertRows();
    }
};

/***********************************************************************************************************************
*   CWorkspace_RemoveRowsScopeGuard
***********************************************************************************************************************/
class CWorkspace_RemoveRowsScopeGuard
{
public:
    explicit CWorkspace_RemoveRowsScopeGuard(CCfgItem *parent, const CCfgItem *startItem, int count)
    {
        CWorkspace_BeginRemoveRows(parent, startItem, count);
    }

    ~CWorkspace_RemoveRowsScopeGuard()
    {
        CWorkspace_EndRemoveRows();
    }
};
