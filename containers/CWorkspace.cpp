/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CLogScrutinizerDoc.h"
#include "CDebug.h"
#include "globals.h"
#include "math.h"

#include "CProgressCtrl.h"
#include "CWorkspace.h"
#include "CWorkspace_cb_if.h"
#include "ceditorwidget_cb_if.h"
#include "CFilter.h"
#include "CConfigurationCtrl.h"

#include "../qt/mainwindow_cb_if.h"
#include "../qt/cfilteritemwidget.h"

#include <QWidget>
#include <QInputDialog>
#include <QMimeData>
#include <algorithm>
#include <QIcon>
#include <QPixmap>

CWorkspace *g_workspace_p = nullptr;

/***********************************************************************************************************************
*   CWorkspace_DisableFilterItem
***********************************************************************************************************************/
void CWorkspace_DisableFilterItem(int uniqueID)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    g_workspace_p->DisableFilterItem(uniqueID);
}

/***********************************************************************************************************************
*   CWorkspace_FilterItemProperties
***********************************************************************************************************************/
void CWorkspace_FilterItemProperties(int uniqueID, QWidget *widget_p)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    g_workspace_p->FilterItemProperties(uniqueID, widget_p);
}

/***********************************************************************************************************************
*   CWorkspace_GetMatchingFilters
***********************************************************************************************************************/
void CWorkspace_GetMatchingFilters(QList<CCfgItem_FilterItem *> *filterItemList_p, const QString& match)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    g_workspace_p->GetMatchingFilters(filterItemList_p, match);
}

/***********************************************************************************************************************
*   CWorkspace_ItemUpdated
***********************************************************************************************************************/
void CWorkspace_ItemUpdated(CCfgItem *item_p)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }

    g_workspace_p->m_model_p->itemUpdated(item_p);
}

/***********************************************************************************************************************
*   CWorkspace_SetFocus
***********************************************************************************************************************/
void CWorkspace_SetFocus(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    g_workspace_p->TakeFocus();
}

/***********************************************************************************************************************
*   CWorkspace_AddFilterItem
***********************************************************************************************************************/
CCfgItem_FilterItem *CWorkspace_AddFilterItem(char *filterText_p, CCfgItem_FilterItem *cfgFilterItem_p,
                                              CCfgItem_Filter *parentFilter_p)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return nullptr;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return nullptr;
    }
    return g_workspace_p->AddFilterItem(filterText_p, cfgFilterItem_p, parentFilter_p);
}

/***********************************************************************************************************************
*   CWorkspace_LayoutAboutToBeChanged
***********************************************************************************************************************/
void CWorkspace_LayoutAboutToBeChanged(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->layoutAboutToBeChanged();
}

/***********************************************************************************************************************
*   CWorkspace_LayoutChanged
***********************************************************************************************************************/
void CWorkspace_LayoutChanged(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->layoutChanged();
}

/***********************************************************************************************************************
*   CWorkspace_BeginReset
***********************************************************************************************************************/
void CWorkspace_BeginReset(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->beginReset();
}

/***********************************************************************************************************************
*   CWorkspace_EndReset
***********************************************************************************************************************/
void CWorkspace_EndReset(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->endReset();
}

/***********************************************************************************************************************
*   CWorkspace_BeginInsertRows
*   Call before to parent if inserting items at the start of the list
***********************************************************************************************************************/
void CWorkspace_BeginInsertRows(CCfgItem *parent, const CCfgItem *before, int count)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->startInsertRows(parent, before, count);
}

/***********************************************************************************************************************
*   CWorkspace_EndInsertRows
***********************************************************************************************************************/
void CWorkspace_EndInsertRows(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->stopInsertRows();
}

/***********************************************************************************************************************
*   CWorkspace_BeginRemoveRows
***********************************************************************************************************************/
void CWorkspace_BeginRemoveRows(CCfgItem *parent, const CCfgItem *startItem, int count)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->startRemoveRows(parent, startItem, count);
}

/***********************************************************************************************************************
*   CWorkspace_EndRemoveRows
***********************************************************************************************************************/
void CWorkspace_EndRemoveRows(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_model_p != nullptr);
#endif
    if (g_workspace_p->m_model_p == nullptr) {
        return;
    }
    g_workspace_p->m_model_p->stopRemoveRows();
}

/***********************************************************************************************************************
*   CWorkspace_GetEnabledFilterItemCount
***********************************************************************************************************************/
int CWorkspace_GetEnabledFilterItemCount(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return 0;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
    return g_workspace_p->GetEnabledFilterItemCount();
}

/***********************************************************************************************************************
*   CWorkspace_GetFilterList
***********************************************************************************************************************/
QList<CCfgItem *> *CWorkspace_GetFilterList(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return nullptr;
    }
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
    if (g_workspace_p->m_filters_p == nullptr) {
        return nullptr;
    }
    return &g_workspace_p->m_filters_p->m_cfgChildItems;
}

/***********************************************************************************************************************
*   CWorkspace_GetFiltersHash
***********************************************************************************************************************/
void CWorkspace_GetFiltersHash(QByteArray& data)
{
    if (g_workspace_p->m_filters_p != nullptr) {
        g_workspace_p->GetFiltersHash(data);
    }
}

/***********************************************************************************************************************
*   CWorkspace_CloseAllSelectedPlugins
***********************************************************************************************************************/
void CWorkspace_CloseAllSelectedPlugins(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    if (g_workspace_p != nullptr) {
        g_workspace_p->CloseAllSelectedPlugins();
    }
}

/***********************************************************************************************************************
*   CWorkspace_GetBookmarks
***********************************************************************************************************************/
void CWorkspace_GetBookmarks(QList<int> *bookmarks)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    if (g_workspace_p != nullptr) {
        g_workspace_p->GetBookmarkList(bookmarks);
    }
}

/***********************************************************************************************************************
*   CWorkspace_RemoveAllBookmarks
***********************************************************************************************************************/
void CWorkspace_RemoveAllBookmarks(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    if (g_workspace_p != nullptr) {
        g_workspace_p->RemoveAllBookmarks();
    }
}

/***********************************************************************************************************************
*   CWorkspace_CloseAllPlugins
***********************************************************************************************************************/
void CWorkspace_CloseAllPlugins(void)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return;
    }
    if (g_workspace_p != nullptr) {
        g_workspace_p->CloseAllPlugins();
    }
}

/***********************************************************************************************************************
*   CWorkspace_GetPlotRoot
***********************************************************************************************************************/
CCfgItem *CWorkspace_GetPlotRoot(CCfgItem_Plugin *plugin_p)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return nullptr;
    }

    QList<CCfgItem *>& list = plugin_p->m_cfgChildItems;
    QList<CCfgItem *>::iterator childIter = list.begin();

    for ( ; childIter != list.end(); ++childIter) {
        if ((*childIter)->m_itemKind == CFG_ITEM_KIND_PlotRoot) {
            return (*childIter);
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   CWorkspace_PlotExtractTime
* returns:
*     -2, No plugin support SUPPORTED_FEATURE_PLOT_TIME
*     -1, No plugin manage to convert log row to time
*      1, Success
***********************************************************************************************************************/
int CWorkspace_PlotExtractTime(const char *row_p, const int length, double *time_p)
{
    int resultCode = -2;
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return 0;
    }

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p != nullptr);
    Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
    if ((g_workspace_p == nullptr) || (g_workspace_p->m_filters_p == nullptr)) {
        return resultCode;
    }

    if (g_workspace_p->m_plugins_p->m_cfgChildItems.isEmpty()) {
        return resultCode;
    }

    QList<CCfgItem *>& cfgPluginItems = g_workspace_p->m_plugins_p->m_cfgChildItems;
    QList<CCfgItem *>::iterator pluginIter = cfgPluginItems.begin();

    for ( ; pluginIter != cfgPluginItems.end(); ++pluginIter) {
        CCfgItem_Plugin *cfgPlugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);
        if (cfgPlugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }
        if (cfgPlugin_p->m_info.supportedFeatures & SUPPORTED_FEATURE_PLOT_TIME) {
            resultCode = -1;
        } else {
            continue;
        }

        CCfgItem *cfgPlotRootItem_p = CWorkspace_GetPlotRoot(cfgPlugin_p);

        /* PlotRoot
         *     Plot1
         *        SubPlot1
         *            Graph1
         *            Graph2
         *        SubPlot2
         *            Graph1
         *            Graph2 */
        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotRootChilds = cfgPlotRootItem_p->m_cfgChildItems;
            QList<CCfgItem *>::iterator plotChildIter = plotRootChilds.begin();

            for ( ; plotChildIter != plotRootChilds.end(); ++plotChildIter) {
                if ((*plotChildIter)->m_itemKind == CFG_ITEM_KIND_Plot) {
                    if (static_cast<CCfgItem_Plot *>(*plotChildIter)->m_plot_ref_p->
                            vPlotExtractTime(row_p, length, time_p)) {
                        return 1;
                    }
                }
            }
        }
    } /* while plugins */
    return resultCode;
}

/***********************************************************************************************************************
*   CWorkspace_GetLastSelectedFilterItem
***********************************************************************************************************************/
bool CWorkspace_GetLastSelectedFilterItem(QString *string_p)
{
    if (g_workspace_p == nullptr) {
        Q_ASSERT(CSCZ_SystemState != SYSTEM_STATE_RUNNING);
        return false;
    }
    *string_p = "";

    QList<CCfgItem *> selectionList;
    if (!CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList)) {
        return false;
    }

    CCfgItem *cfgItem_p = selectionList.last();
    if (cfgItem_p->m_itemKind != CFG_ITEM_KIND_FilterItem) {
        return false;
    }
    *string_p = cfgItem_p->m_itemText;
    return true;
}

/* Check if all selections are of the same kind */
bool CWorkspace_isAllSelections(CfgItemKind_t selectionKind)
{
    QList<CCfgItem *> selectionList;
    if (!CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList)) {
        return false;
    }
    for (auto cfgItem_p : selectionList) {
        if (cfgItem_p->m_itemKind != selectionKind) {
            return false;
        }
    }
    return true;
}

/***********************************************************************************************************************
*   CWorkspace_CreateCfgFilter
***********************************************************************************************************************/
CCfgItem_Filter *CWorkspace_CreateCfgFilter(CFilter *filterRef_p)
{
    if (g_workspace_p == nullptr) {
        return nullptr;
    }
    return g_workspace_p->CreateCfgFilter(filterRef_p);
}

/***********************************************************************************************************************
*   CWorkspace_CleanAllPlots
***********************************************************************************************************************/
bool CWorkspace_CleanAllPlots(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p != nullptr);
    Q_ASSERT(g_workspace_p->m_plugins_p != nullptr);
#endif
    if ((g_workspace_p == nullptr) ||
        (g_workspace_p->m_plugins_p == nullptr) ||
        g_workspace_p->m_plugins_p->m_cfgChildItems.isEmpty()) {
        return false;
    }

    QList<CCfgItem *>& plugins = g_workspace_p->m_plugins_p->m_cfgChildItems;

    for (QList<CCfgItem *>::iterator pluginIter = plugins.begin(); pluginIter != plugins.end(); ++pluginIter) {
        CCfgItem_Plugin *plugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);

        if (plugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }

        CCfgItem *cfgPlotRootItem_p = CWorkspace_GetPlotRoot(plugin_p);

        /* PlotRoot
         *     Plot1
         *        SubPlot1
         *            Graph1
         *            Graph2
         *        SubPlot2
         *            Graph1
         *            Graph2 */
        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotList = cfgPlotRootItem_p->m_cfgChildItems;
            QList<CCfgItem *>::iterator plotIter = plotList.begin();

            for ( ; plotIter != plotList.end(); ++plotIter) {
                CCfgItem *cfgItem_Plot_p = (*plotIter);
                if ((cfgItem_Plot_p->m_itemKind == CFG_ITEM_KIND_Plot) && !cfgItem_Plot_p->m_cfgChildItems.isEmpty()) {
                    ((CCfgItem_Plot *)cfgItem_Plot_p)->RemoveElements();
                }
            }
        }
    } /* for all plugins */
    return true;
}

/***********************************************************************************************************************
*   CWorkspace_isPlugInPlotSelected
***********************************************************************************************************************/
bool CWorkspace_isPlugInPlotSelected(void)
{
    /* Check if the firstlection in the selection list is a CPlot */
    if (CWorkspace_GetSelectedPlot() != nullptr) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************************************************
*   CWorkspace_RemoveLog
***********************************************************************************************************************/
void CWorkspace_RemoveLog(void)
{
    g_workspace_p->RemoveLog();
}

/***********************************************************************************************************************
*   CWorkspace_RemoveSelectedFilter
***********************************************************************************************************************/
void CWorkspace_RemoveSelectedFilter(void)
{
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList);

    if (selectionList.isEmpty()) {
        return;
    }

    CCfgItem *cfgItem_p = selectionList.first();
    CCfgItem_Delete(cfgItem_p);
}

/***********************************************************************************************************************
*   CWorkspace_RemoveSelectedFilterItem
***********************************************************************************************************************/
void CWorkspace_RemoveSelectedFilterItem(void)
{
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_FilterItem, selectionList);

    if (selectionList.isEmpty()) {
        return;
    }

    CCfgItem *cfgItem_p = selectionList.first();
    CCfgItem_Delete(cfgItem_p);
}

/***********************************************************************************************************************
*   CWorkspace_GetSelectedPlot
***********************************************************************************************************************/
CCfgItem_Plot *CWorkspace_GetSelectedPlot(void)
{
    CCfgItem_Plot *itemPlot_p = nullptr;
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

    if (selectionList.isEmpty()) {
        return nullptr;
    }

    CCfgItem *cfgItem_p = selectionList.first();

    switch (cfgItem_p->m_itemKind)
    {
        case CFG_ITEM_KIND_Plot:
            itemPlot_p = (CCfgItem_Plot *)cfgItem_p;
            break;

        case CFG_ITEM_KIND_SubPlot:
            itemPlot_p = (CCfgItem_Plot *)cfgItem_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_Graph:
        case CFG_ITEM_KIND_SequenceDiagram:
            itemPlot_p = (CCfgItem_Plot *)cfgItem_p->m_itemParent_p->m_itemParent_p;
            break;

        default:
            return nullptr;  /* Failure */
    }

    if ((itemPlot_p != nullptr) && (itemPlot_p->m_itemKind != CFG_ITEM_KIND_Plot)) {
        /* Capture that the hierachy doesn't match */
        TRACEX_W("Error in CWorkspace_GetSelectedPlot");
        return nullptr;
    }

    return itemPlot_p;
}

/***********************************************************************************************************************
*   CWorkspace_GetSelectedPlugin
***********************************************************************************************************************/
CCfgItem_Plugin *CWorkspace_GetSelectedPlugin(CCfgItem *selection_p)
{
    /*  // Plugin
     *  //   PlotRoot
     *  //     Plot1
     *  //        SubPlot1
     *  //            Graph1
     *  //            Graph2
     *  //        SubPlot2
     *  //            Graph1
     *  //            Graph2
     */
    CCfgItem_Plugin *plugin_p = nullptr;
    CCfgItem *cfgItem_p = selection_p;
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

    if (selection_p == nullptr) {
        if (selectionList.isEmpty()) {
            return nullptr;
        }
        cfgItem_p = selectionList.first();
    }

    switch (cfgItem_p->m_itemKind)
    {
        case CFG_ITEM_KIND_PlugIn:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p;
            break;

        case CFG_ITEM_KIND_PlotRoot:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_Plot:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_SubPlot:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p->m_itemParent_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_Graph:
        case CFG_ITEM_KIND_SequenceDiagram:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p->m_itemParent_p->m_itemParent_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_DecoderRoot:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p;
            break;

        case CFG_ITEM_KIND_Decoder:
            plugin_p = (CCfgItem_Plugin *)cfgItem_p->m_itemParent_p->m_itemParent_p;
            break;

        default:
            return nullptr;  /* Failure */
    }

    if ((plugin_p != nullptr) && (plugin_p->m_itemKind != CFG_ITEM_KIND_PlugIn)) {
        TRACEX_W("Error in CWorkspace_GetSelectedPlugin");
        return nullptr;
    }

    return plugin_p;
}

/***********************************************************************************************************************
*   CWorkspace_PlugInPlotRunSelected
***********************************************************************************************************************/
void CWorkspace_PlugInPlotRunSelected(CPlot *plot_p)
{
    /* Run the first plugin Plot in the selection list */
    QList<CPlot *> plotList;
    CCfgItem_Plot *plotSelected_p;

    if (plot_p == nullptr) {
        if ((plotSelected_p = CWorkspace_GetSelectedPlot()) == nullptr) {
            return;
        }
    } else {
        plotSelected_p = CWorkspace_SearchPlot(plot_p);
    }

    if ((plotSelected_p != nullptr) && (plotSelected_p->m_itemKind == CFG_ITEM_KIND_Plot)) {
        plotSelected_p->PlotAllChildren(nullptr, nullptr);
    } else {
        TRACEX_W("Couldn't run plot, not found");
    }
}

/***********************************************************************************************************************
*   CWorkspace_PluginsExists
***********************************************************************************************************************/
bool CWorkspace_PluginsExists(void)
{
    if ((g_workspace_p == nullptr) || g_workspace_p->m_plugins_p->m_cfgChildItems.isEmpty()) {
        return false;
    }

    return true;
}

/***********************************************************************************************************************
*   CWorkspace_PlugInPlotsExists
***********************************************************************************************************************/
bool CWorkspace_PlugInPlotsExists(void)
{
    /* Search all plugins until at least one with Plots has been found
     *
     *  PlotRoot
     *     Plot1
     *        SubPlot1
     *            Graph1
     *            Graph2
     *        SubPlot2
     *            Graph1
     *            Graph2 */

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p);
#endif
    if (g_workspace_p->m_plugins_p == nullptr) {
        return false;
    }

    QList<CCfgItem *>& pluginList = g_workspace_p->m_plugins_p->m_cfgChildItems;

    if (pluginList.isEmpty()) {
        return false;
    }

    for (auto pluginIter = pluginList.begin(); pluginIter != pluginList.end(); ++pluginIter) {
        auto plugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);
        if (plugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }

        auto cfgPlotRootItem_p = CWorkspace_GetPlotRoot(plugin_p);

        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotList = cfgPlotRootItem_p->m_cfgChildItems;
            for (auto plotIter = plotList.begin(); plotIter != plotList.end(); ++plotIter) {
                /* loop inside the plotRoot... there should be only Plots */
                if (((*plotIter)->m_itemKind == CFG_ITEM_KIND_Plot) && !(*plotIter)->m_cfgChildItems.isEmpty()) {
                    return true;
                }
            } /* loop over plots */
        }
    }

    return false;
}

/***********************************************************************************************************************
*   CWorkspace_RunAllPlots
***********************************************************************************************************************/
void CWorkspace_RunAllPlots(void)
{
    /* PlotRoot/
     *  Plot1
     *        SubPlot1
     *            Graph1
     *            Graph2
     *        SubPlot2
     *            Graph1
     *            Graph2 */
    QList<CCfgItem_Plot *> cfgPlotList;
    QList<CPlot *> ToRunplotList;

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p);
#endif
    if (g_workspace_p->m_plugins_p == nullptr) {
        return;
    }

    g_workspace_p->m_plugins_p->PlotAllChildren(nullptr, nullptr);
}

/***********************************************************************************************************************
*   CWorkspace_SearchPlugin
***********************************************************************************************************************/
CPlugin_DLL_API *CWorkspace_SearchPlugin(CPlot *childPlot_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p);
#endif
    if (g_workspace_p->m_plugins_p == nullptr) {
        return nullptr;
    }

    QList<CCfgItem *>& pluginList = g_workspace_p->m_plugins_p->m_cfgChildItems;

    if (pluginList.isEmpty()) {
        return nullptr;
    }

    for (auto pluginIter = pluginList.begin(); pluginIter != pluginList.end(); ++pluginIter) {
        auto plugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);
        if (plugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }

        auto cfgPlotRootItem_p = CWorkspace_GetPlotRoot(plugin_p);

        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotList = cfgPlotRootItem_p->m_cfgChildItems;
            for (auto plotIter = plotList.begin(); plotIter != plotList.end(); ++plotIter) {
                /* loop inside the plotRoot... there should be only Plots */
                auto cfgItem_Plot_p = static_cast<CCfgItem_Plot *>(*plotIter);
                if ((cfgItem_Plot_p->m_itemKind == CFG_ITEM_KIND_Plot) &&
                    (cfgItem_Plot_p->m_plot_ref_p == childPlot_p)) {
                    return plugin_p->m_dll_api_p; /* correct plugin found */
                }
            } /* loop over plots */
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   CWorkspace_SearchAndUnloadPlugin
***********************************************************************************************************************/
void CWorkspace_SearchAndUnloadPlugin(CPlot *childPlot_p)
{
    QList<CCfgItem *>& pluginList = g_workspace_p->m_plugins_p->m_cfgChildItems;

    if (pluginList.isEmpty()) {
        return;
    }

    for (auto pluginIter = pluginList.begin(); pluginIter != pluginList.end(); ++pluginIter) {
        auto plugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);
        if (plugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }

        auto cfgPlotRootItem_p = CWorkspace_GetPlotRoot(plugin_p);

        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotList = cfgPlotRootItem_p->m_cfgChildItems;
            for (auto plotIter = plotList.begin(); plotIter != plotList.end(); ++plotIter) {
                /* loop inside the plotRoot... there should be only Plots */
                auto cfgItem_Plot_p = static_cast<CCfgItem_Plot *>(*plotIter);
                if ((cfgItem_Plot_p->m_itemKind == CFG_ITEM_KIND_Plot) &&
                    (cfgItem_Plot_p->m_plot_ref_p == childPlot_p)) {
                    CCfgItem_Delete(plugin_p);
                    return;
                }
            } /* loop over plots */
        }
    }
}

/***********************************************************************************************************************
*   CWorkspace_SearchPlot
***********************************************************************************************************************/
CCfgItem_Plot *CWorkspace_SearchPlot(CPlot *childPlot_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p);
#endif
    if (g_workspace_p->m_plugins_p == nullptr) {
        return nullptr;
    }

    QList<CCfgItem *>& pluginList = g_workspace_p->m_plugins_p->m_cfgChildItems;

    if (pluginList.isEmpty()) {
        return nullptr;
    }

    for (auto pluginIter = pluginList.begin(); pluginIter < pluginList.end(); ++pluginIter) {
        auto plugin_p = static_cast<CCfgItem_Plugin *>(*pluginIter);
        if (plugin_p->m_cfgChildItems.isEmpty()) {
            continue;
        }

        auto cfgPlotRootItem_p = CWorkspace_GetPlotRoot(plugin_p);

        if ((cfgPlotRootItem_p != nullptr) && !cfgPlotRootItem_p->m_cfgChildItems.isEmpty()) {
            QList<CCfgItem *>& plotList = cfgPlotRootItem_p->m_cfgChildItems;
            for (auto plotIter = plotList.begin(); plotIter != plotList.end(); ++plotIter) {
                /* loop inside the plotRoot... there should be only Plots */
                auto cfgItem_Plot_p = static_cast<CCfgItem_Plot *>(*plotIter);
                if ((cfgItem_Plot_p->m_itemKind == CFG_ITEM_KIND_Plot) &&
                    (cfgItem_Plot_p->m_plot_ref_p == childPlot_p)) {
                    return cfgItem_Plot_p;
                }
            } /* loop over plots */
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   CWorkspace - CTOR
***********************************************************************************************************************/
CWorkspace::CWorkspace() :
    m_dragOngoing(false), m_pendingSingleSelection(false), m_singleSelection_p(nullptr),
    m_dragImageIndex(0), m_inFocus(false), m_needKillMenuOperation(false), m_EraseBackGroundDisabled(false),
    m_root_p(nullptr), m_workspaceRoot_p(nullptr), m_filters_p(nullptr), m_plugins_p(nullptr), m_logs_p(nullptr),
    m_comments_p(nullptr), m_bookmarks_p(nullptr), m_model_p(nullptr)
{
    g_workspace_p = this;
    m_dragImageIndex = 0;
}

/***********************************************************************************************************************
*   cleanAll
***********************************************************************************************************************/
void CWorkspace::cleanAll(void)
{
    CSCZ_SystemState = SYSTEM_STATE_SHUTDOWN;

    /*Are part of m_root_p child list... shouldn't be deleted */
    m_filters_p = nullptr;
    m_plugins_p = nullptr;
    m_logs_p = nullptr;
    m_comments_p = nullptr;
    m_bookmarks_p = nullptr;

    if (m_root_p != nullptr) {
        CCfgItem_Delete(m_root_p);
        m_root_p = nullptr;
    }

    g_workspace_p = nullptr;
}

/***********************************************************************************************************************
*   FillWorkspace
***********************************************************************************************************************/
void CWorkspace::FillWorkspace(void)
{
    m_root_p = new CCfgItem(nullptr);

    /* Dummy construction to handle the header in QT tree which we may not expand etc. So adding one extra level to
     * achive a way to interact with root item */
    m_workspaceRoot_p = new CCfgItem_Root(m_root_p);
    m_root_p->m_cfgChildItems.append(m_workspaceRoot_p);

    /* All of the following items are at their own top level items, however inserted into the root item list, but said
     * to have no parent item. Then if taking the item row position it is the index into the root child item list. */
    m_filters_p = new CCfgItem_Filters(m_workspaceRoot_p);
    m_workspaceRoot_p->m_cfgChildItems.append(m_filters_p);

    m_plugins_p = new CCfgItem_Plugins(m_workspaceRoot_p);
    m_workspaceRoot_p->m_cfgChildItems.append(m_plugins_p);

    m_logs_p = new CCfgItem_Logs(m_workspaceRoot_p);
    m_workspaceRoot_p->m_cfgChildItems.append(m_logs_p);

    m_comments_p = new CCfgItem_Comments(m_workspaceRoot_p);
    m_workspaceRoot_p->m_cfgChildItems.append(m_comments_p);

    m_bookmarks_p = new CCfgItem_Bookmarks(m_workspaceRoot_p);
    m_workspaceRoot_p->m_cfgChildItems.append(m_bookmarks_p);
}

/***********************************************************************************************************************
*   SetQuickSearch
***********************************************************************************************************************/
void CWorkspace::SetQuickSearch(uint32_t nChar)
{
    /* Set a specific letter to designate a quick search/
     *  If letter is occupied it is re-assigned to the new one */

    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

    if (selectionList.isEmpty() || (selectionList.count() > 1)) {
        return;
    }

    /* Assign the filter the letter for quick search */
    auto cfgItem_p = selectionList.first();
    if (cfgItem_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
        static_cast<CCfgItem_FilterItem *>(cfgItem_p)->m_quickSearchNum = (int)nChar;
    } else {
        return;
    }

    /* Search through all filters/filterItems and remove any use of nChar */
    for (auto cfgFilter_p : m_filters_p->m_cfgChildItems) {
        for (auto cfgItem_p : cfgFilter_p->m_cfgChildItems) {
            auto cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(cfgItem_p);
            if (cfgFilterItem_p->m_quickSearchNum == (int)nChar) {
                cfgFilterItem_p->m_quickSearchNum = -1;
                MW_Refresh();
                return;
            }
        }
    }
}

/***********************************************************************************************************************
*   QuickSearch
***********************************************************************************************************************/
void CWorkspace::QuickSearch(uint32_t nChar, bool searchDown)
{
    /* Search through all filters/filterItems, when the cChar assign filter is found start the quick search and then
     * return */
    for (auto cfgFilter_p : m_filters_p->m_cfgChildItems) {
        for (auto cfgItem_p : cfgFilter_p->m_cfgChildItems) {
            auto cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(cfgItem_p);
            if (cfgFilterItem_p->m_quickSearchNum == (int)nChar) {
                char searchString[CFG_TEMP_STRING_MAX_SIZE];
                memcpy(searchString, cfgFilterItem_p->m_filterItem_ref_p->m_start_p,
                       cfgFilterItem_p->m_filterItem_ref_p->m_size);
                searchString[cfgFilterItem_p->m_filterItem_ref_p->m_size] = 0;

                MW_QuickSearch(searchString,
                               searchDown,
                               cfgFilterItem_p->m_filterItem_ref_p->m_caseSensitive,
                               cfgFilterItem_p->m_filterItem_ref_p->m_regexpr);

                return;
            }
        } /* filterItems */
    } /* filters */
}

/***********************************************************************************************************************
*   isItemSelected
***********************************************************************************************************************/
bool CWorkspace::isItemSelected(CCfgItem *cfgItem_p)
{
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

    for (auto selection_p : selectionList) {
        if (selection_p == cfgItem_p) {
            return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   GetSelectionFileInformation
* returns true if the selection can be saved, and with suitable menu text
***********************************************************************************************************************/
bool CWorkspace::GetSelectionFileInformation(QString *fileSaveInfo_p, CfgItemKind_t *itemKind_p,
                                             CfgItem_PossibleFileOperations_t *fileOperations_p)
{
    CfgItemKind_t itemKind;
    bool success = false;
    CfgItem_PossibleFileOperations_t fileOperation;

    if (isSingleKindSelections(&itemKind)) {
        QList<CCfgItem *> selectionList;
        CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

        CCfgItem *cfgItem_p = selectionList.first();

        switch (cfgItem_p->m_itemKind)
        {
            case CFG_ITEM_KIND_Filter:
            {
                CCfgItem_Filter *filter_p = (CCfgItem_Filter *)cfgItem_p;
                filter_p->m_filter_ref_p->GetFileNameOnly(fileSaveInfo_p);

                fileOperation = CFGITEM_FILE_OPERATION_SAVE | CFGITEM_FILE_OPERATION_SAVE_AS |
                                CFGITEM_FILE_OPERATION_CLOSE;
                success = true;
                break;
            }

            case CFG_ITEM_KIND_FilterItem:
            {
                CCfgItem_Filter *filter_p = (CCfgItem_Filter *)cfgItem_p->m_itemParent_p;
                filter_p->m_filter_ref_p->GetFileNameOnly(fileSaveInfo_p);

                fileOperation = CFGITEM_FILE_OPERATION_SAVE | CFGITEM_FILE_OPERATION_SAVE_AS |
                                CFGITEM_FILE_OPERATION_CLOSE;
                success = true;
                break;
            }

            case CFG_ITEM_KIND_Root:
                *fileSaveInfo_p = m_workspaceRoot_p->m_workspaceShortName;
                fileOperation = CFGITEM_FILE_OPERATION_SAVE | CFGITEM_FILE_OPERATION_SAVE_AS |
                                CFGITEM_FILE_OPERATION_CLOSE;
                success = true;
                break;

            case CFG_ITEM_KIND_PlugIn:
            {
                CCfgItem_Plugin *plugin_p = (CCfgItem_Plugin *)cfgItem_p;
                *fileSaveInfo_p = plugin_p->m_fileName;
                fileOperation = CFGITEM_FILE_OPERATION_CLOSE;
                success = true;
                break;
            }

            case CFG_ITEM_KIND_Log:
                *fileSaveInfo_p = GetShortName(static_cast<CCfgItem_Log *>(cfgItem_p)->m_path);
                fileOperation = CFGITEM_FILE_OPERATION_CLOSE;
                success = true;
                break;

            default:
                Q_ASSERT(false);
                break;
        } /* switch */
    } /* if single selection */

    if (success) {
        if (itemKind_p != nullptr) {
            *itemKind_p = itemKind;
        }
        if (fileOperations_p != nullptr) {
            *fileOperations_p = fileOperation;
        }
        return true;
    }

    return false;
}

/***********************************************************************************************************************
*   GetShortName
***********************************************************************************************************************/
QString CWorkspace::GetShortName(QString& filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.fileName();
}

/***********************************************************************************************************************
*   ExecuteFileOperationOnSelection
***********************************************************************************************************************/
void CWorkspace::ExecuteFileOperationOnSelection(CfgItem_PossibleFileOperations_t fileOperations)
{
    CfgItemKind_t itemKind;

    if (isSingleKindSelections(&itemKind)) {
        QList<CCfgItem *> selectionList;
        CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

        CCfgItem *cfgItem_p = selectionList.first();
        if ((itemKind == CFG_ITEM_KIND_Filter) || (itemKind == CFG_ITEM_KIND_FilterItem)) {
            CCfgItem_Filter *filter_p;
            if (itemKind == CFG_ITEM_KIND_Filter) {
                filter_p = (CCfgItem_Filter *)cfgItem_p;
            } else {
                filter_p = (CCfgItem_Filter *)cfgItem_p->m_itemParent_p;
            }

            switch (fileOperations)
            {
                case CFGITEM_FILE_OPERATION_SAVE_AS:
                    filter_p->SaveAs();
                    break;

                case CFGITEM_FILE_OPERATION_SAVE:
                    filter_p->Save();
                    break;

                case CFGITEM_FILE_OPERATION_CLOSE:
                    CCfgItem_Delete(filter_p);
                    break;

                default:
                    TRACEX_W("CWorkspace::ExecuteFileOperationOnSelection kind:%d "
                             "operation:%d NOT SUPPORTED", itemKind, fileOperations);
                    break;
            }
        } else if (itemKind == CFG_ITEM_KIND_Root) {
            switch (fileOperations)
            {
                case CFGITEM_FILE_OPERATION_CLOSE:
                    CFGCTRL_UnloadAll();
                    break;

                case CFGITEM_FILE_OPERATION_SAVE:
                {
                    extern bool CFGCTRL_SaveWorkspaceFile(void);
                    CFGCTRL_SaveWorkspaceFile();
                    break;
                }

                case CFGITEM_FILE_OPERATION_SAVE_AS:
                {
                    extern bool CFGCTRL_SaveWorkspaceFileAs(void);
                    CFGCTRL_SaveWorkspaceFileAs();
                    break;
                }

                default:
                    TRACEX_W("CWorkspace::ExecuteFileOperationOnSelection kind:%d "
                             "operation:%d NOT SUPPORTED", itemKind, fileOperations);
                    break;
            }
        } else if (itemKind == CFG_ITEM_KIND_PlugIn) {
            if (fileOperations == CFGITEM_FILE_OPERATION_CLOSE) {
                CloseAllSelectedPlugins();
            } else {
                TRACEX_W("CWorkspace::ExecuteFileOperationOnSelection kind:%d "
                         "operation:%d NOT SUPPORTED", itemKind, fileOperations);
            }
        } else if (itemKind == CFG_ITEM_KIND_Log) {
            if (fileOperations == CFGITEM_FILE_OPERATION_CLOSE) {
                RemoveLog();
            } else {
                TRACEX_W("CWorkspace::ExecuteFileOperationOnSelection kind:%d operation:%d NOT SUPPORTED",
                         itemKind, fileOperations);
            }
        }
    }
}

/***********************************************************************************************************************
*   isSingleKindSelections
***********************************************************************************************************************/
bool CWorkspace::isSingleKindSelections(CfgItemKind_t *kind_p)
{
    QList<CCfgItem *> selectionList;
    CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_None, selectionList);

    if (selectionList.isEmpty()) {
        return false;
    }

    auto itemKind = selectionList.first()->m_itemKind;

    if (kind_p != nullptr) {
        *kind_p = itemKind;
    }

    for (auto cfgItem_p : selectionList) {
        if (cfgItem_p->m_itemKind != itemKind) {
            return false;
        }
    }
    return true;
}

/***********************************************************************************************************************
*   AddFilterItem
*
* If filterItem_pp is nullptr then it shall be created.
***********************************************************************************************************************/
CCfgItem_FilterItem *CWorkspace::AddFilterItem(char *filterText_p, CCfgItem_FilterItem *cfgFilterItem_p,
                                               CCfgItem_Filter *parentFilter_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    CCfgItem_Filter *cfgFilter_p = parentFilter_p;
    CFilterItem *filterItem_p = nullptr;
    QString text;
    bool filterItemCreated = false;
    auto oldTrackState = g_cfg_p->m_logFileTracking;

    MW_updateLogFileTrackState(false);

    auto refreshEditorWindow = makeMyScopeGuard([&] () {
        MW_updateLogFileTrackState(oldTrackState);

        /* Update DOC lut */
        if (filterItem_p != nullptr) {
            doc_p->UpdateFilterItem(filterItem_p->m_uniqueID, filterItem_p->m_color, filterItem_p->m_bg_color);
        }
    });

    /* Check if there should a new filter created */
    if (nullptr == cfgFilterItem_p) {
        text = (filterText_p == nullptr) ? QString("") : QString(filterText_p);
        filterItem_p = new CFilterItem(&text);

        if (nullptr == filterItem_p) {
            TRACEX_E(QString("%1 filterItem_p is nullptr").arg(__FUNCTION__));
            return nullptr;
        }

        if (filterItem_p->m_start_p == nullptr) {
            delete filterItem_p;
            filterItem_p = nullptr;
            TRACEX_E(QString("%1 filterItem_p->m_start_p is nullptr").arg(__FUNCTION__));
            return nullptr;
        }

        if (parentFilter_p == nullptr) {
            /* 1. Check last selection, pick that
             * 2. If no selection pick first in list
             * 3. If no filters in list create a new one and insert */

            QList<CCfgItem *> selectionList;
            if (CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_Filter, selectionList)) {
                cfgFilter_p = static_cast<CCfgItem_Filter *>(selectionList.first());
            } else if (!m_filters_p->m_cfgChildItems.isEmpty()) {
                /* Pick first */
                cfgFilter_p = (CCfgItem_Filter *)m_filters_p->m_cfgChildItems.first();
            } else {
                cfgFilter_p = CreateCfgFilter(nullptr);
            }
        }
    } else {
        filterItem_p = cfgFilterItem_p->m_filterItem_ref_p;
    }

    text = QString(filterItem_p->m_start_p);

    CFilterItemWidget dlg(&text,
                          &filterItem_p->m_enabled,
                          &filterItem_p->m_color,
                          &filterItem_p->m_bg_color,
                          &filterItem_p->m_caseSensitive,
                          &filterItem_p->m_exclude,
                          &filterItem_p->m_regexpr,
                          m_filters_p,  /* MOVE_FILTER */
                          &cfgFilter_p);  /* MOVE_FILTER */

    dlg.setModal(true);

    if (dlg.exec() == QDialog::Accepted) {
        MW_SetWorkspaceWidgetFocus();
        TRACEX_DE(QString("%1 SetFocus").arg(__FUNCTION__));

        /* Copy the filter string (text) as it could have been changed in the Dlg */
        if (filterItem_p->m_start_p != nullptr) {
            free(filterItem_p->m_start_p);
        }

        filterItem_p->m_size = text.length();
        filterItem_p->m_start_p = (char *)malloc(text.length() + 1);

        if (filterItem_p->m_start_p != nullptr) {
            memcpy(filterItem_p->m_start_p, text.toLatin1().constData(), filterItem_p->m_size);
            filterItem_p->m_start_p[filterItem_p->m_size] = 0;
        } else {
            TRACEX_E("CCfgItem_FilterItem::PropertiesDlg    m_filterItem_ref_p->m_start_p  nullptr");
        }

        if (nullptr == cfgFilterItem_p) {
            cfgFilterItem_p = new CCfgItem_FilterItem(cfgFilter_p->m_filter_ref_p, filterItem_p, cfgFilter_p);
            if (nullptr == cfgFilterItem_p) {
                TRACEX_E(QString("%1 memory allocation of CCfgItem_FilterItem failed").arg(__FUNCTION__));
                return nullptr;
            }
            cfgFilterItem_p->InsertItem(false, true, true);  /*Will add the object to the tail (no auto focus) */
        }
        filterItem_p->m_font_p = doc_p->m_fontCtrl.RegisterFont(filterItem_p->m_color, filterItem_p->m_bg_color);
        cfgFilterItem_p->UpdateTreeName(filterItem_p->m_start_p);
        return cfgFilterItem_p;
    } else {
        if (filterItemCreated) {
            delete filterItem_p;
            filterItem_p = nullptr;
        }
    }
    return nullptr;
}

/***********************************************************************************************************************
*   CreateCfgFilter
***********************************************************************************************************************/
CCfgItem_Filter *CWorkspace::CreateCfgFilter(CFilter *filterRef_p)
{
    /* Create an empty filter and attach it to the workspace
     * The filter will not contain any path, so save operations should be disabled */

    CFilter *filter_p = filterRef_p;

    if (filterRef_p == nullptr) {
        filter_p = new CFilter();
        filter_p->m_fileName = "New_Filter.flt";
        filter_p->m_fileName_short = "New_Filter";
    }

    /* Must locate which */

    CCfgItem_Filter *treeItemFilter_p =
        new CCfgItem_Filter( /*this, m_filters_p->m_h_treeItem,*/ filter_p, m_filters_p);

    CWorkspace_TreeView_UnselectAll();

    /* This will loop through all filterItems and add them as CfgItems, and remove the filterItems from the
     * filterRef_p to avoid double refs */
    treeItemFilter_p->InsertFilter();
    return treeItemFilter_p;
}

/***********************************************************************************************************************
*   ReinsertCfgFilter
***********************************************************************************************************************/
bool CWorkspace::ReinsertCfgFilter(CCfgItem_Filter *treeItemFilter_p)
{
    /* This will loop through all filterItems and add them as CfgItems */
    treeItemFilter_p->InsertFilter(true /*reinsert*/);

    CWorkspace_TreeView_UnselectAll();

    return true;
}

/***********************************************************************************************************************
*   DisableFilterItem
*     Find the filterItem with the uniqueID and disable it
*       1. Loop through all filters
*        2. Loop through all filterItems
***********************************************************************************************************************/
void CWorkspace::DisableFilterItem(int uniqueID)
{
    for (auto& filter : m_filters_p->m_cfgChildItems) {
        for (auto& item : filter->m_cfgChildItems) {
            CCfgItem_FilterItem *cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(item);
            if (cfgFilterItem_p->m_filterItem_ref_p->m_uniqueID == uniqueID) {
                cfgFilterItem_p->m_filterItem_ref_p->m_enabled = false;
                m_model_p->itemUpdated(cfgFilterItem_p);
                return;
            }
        } /* filterItems */
    } /* filters */
}

/***********************************************************************************************************************
*   FilterItemProperties
***********************************************************************************************************************/
void CWorkspace::FilterItemProperties(int uniqueID, QWidget *widget_p)
{
    for (auto& filter : m_filters_p->m_cfgChildItems) {
        for (auto& item : filter->m_cfgChildItems) {
            CCfgItem_FilterItem *cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(item);
            if (cfgFilterItem_p->m_filterItem_ref_p->m_uniqueID == uniqueID) {
                cfgFilterItem_p->PropertiesDlg(widget_p);
                m_model_p->itemUpdated(cfgFilterItem_p);
                return;
            }
        } /* filterItems */
    } /* filters */
}

/***********************************************************************************************************************
*   GetEnabledFilterItemCount
***********************************************************************************************************************/
int CWorkspace::GetEnabledFilterItemCount(void)
{
    int count = 0;
    for (auto& filter : m_filters_p->m_cfgChildItems) {
        for (auto& item : filter->m_cfgChildItems) {
            CCfgItem_FilterItem *cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(item);
            if (cfgFilterItem_p->m_filterItem_ref_p->m_enabled) {
                count++;
            }
        } /* filterItems */
    } /* filters */

    return count;
}

/***********************************************************************************************************************
*   GetMatchingFilters
***********************************************************************************************************************/
void CWorkspace::GetMatchingFilters(QList<CCfgItem_FilterItem *> *filterItemList_p, const QString& match)
{
    for (auto& filter : m_filters_p->m_cfgChildItems) {
        for (auto& item : filter->m_cfgChildItems) {
            CCfgItem_FilterItem *cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(item);
            auto filterItem = QString(cfgFilterItem_p->m_filterItem_ref_p->m_start_p);
            if (filterItem.indexOf(match, 0, Qt::CaseInsensitive) != -1) {
                filterItemList_p->append(cfgFilterItem_p);
            }
        } /* filterItems */
    } /* filters */
}

/***********************************************************************************************************************
*   GetFiltersHash
***********************************************************************************************************************/
void CWorkspace::GetFiltersHash(QByteArray& data)
{
    QDataStream dstream(&data, QIODevice::WriteOnly);
    for (auto& filter : m_filters_p->m_cfgChildItems) {
        for (auto& item : filter->m_cfgChildItems) {
            CCfgItem_FilterItem *item_p = static_cast<CCfgItem_FilterItem *>(item);
            item_p->hash(dstream);
        } /* filterItems */
    } /* filters */
}

/***********************************************************************************************************************
*   AddPlugin
***********************************************************************************************************************/
void CWorkspace::AddPlugin(CPlugin_DLL_API *pluginAPI_p, QLibrary *library_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p);
#endif
    if (g_workspace_p->m_plugins_p == nullptr) {
        return;
    }

    /* The constructor of the CfgItem_Plugin will do insert itself, and on all its sub-items */
    (void)new CCfgItem_Plugin( /*this, m_plugins_p->m_h_treeItem,*/ pluginAPI_p, library_p, m_plugins_p);
}

/***********************************************************************************************************************
*   AddLog
***********************************************************************************************************************/
void CWorkspace::AddLog(char *path_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(m_logs_p != nullptr);
#endif
    if (m_logs_p == nullptr) {
        return;
    }

    m_logs_p->RemoveAllChildren();

    CCfgItem_Log *log_p = new CCfgItem_Log( /*this, m_logs_p->m_h_treeItem,*/ path_p, m_logs_p);
    log_p->InsertItem();
}

/***********************************************************************************************************************
*   RemoveLog
***********************************************************************************************************************/
void CWorkspace::RemoveLog(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(m_logs_p != nullptr);
#endif
    if (m_logs_p == nullptr) {
        return;
    }
    m_logs_p->RemoveAllChildren();
}

/***********************************************************************************************************************
*   ToggleBookmark
***********************************************************************************************************************/
void CWorkspace::ToggleBookmark(QWidget *parent, QString *comment_p, int row)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    bool found = false;
    bool insert = true;
    CCfgItem_Bookmark *bookmark_p = nullptr;
    CCfgItem *itemBefore_p = m_bookmarks_p;

    if (row > doc_p->m_database.TIA.rows) {
        TRACEX_W("CWorkspace::ToggleBookmark  Trying to toggle bookmark to row outside "
                 "logfile max:%d row:%d", doc_p->m_database.TIA.rows, row);
        return;
    }

    TRACEX_D("CWorkspace::ToggleBookmark");

    /* If a bookmark already exist at this row then it shall be removed
     * Based on row, find bookmark that is before and after */

    if (!m_bookmarks_p->m_cfgChildItems.isEmpty()) {
        QList<CCfgItem *>::iterator iter = m_bookmarks_p->m_cfgChildItems.begin();

        /* If possible, loop through all the existing bookmarks and find the other bookmark that should be infront. It
         * is possible that this bookmark should be infront, then it is directly found and itemBefore_p would then keep
         *  its (m_bookmarks_p) value. If none is found then it itemBefore_p contains the current last bookmark in the
         * list. */
        while (iter != m_bookmarks_p->m_cfgChildItems.end() && !found) {
            CCfgItem_Bookmark *bookmark_p = static_cast<CCfgItem_Bookmark *>(*iter);
            if (bookmark_p->m_row == row) {
                /*m_bookmarks_p->m_cfgChildItems.erase(iter);  --> this should be done by prepare delete instead */
                CCfgItem_Delete(bookmark_p);
                insert = false;
                found = true;
            } else if (bookmark_p->m_row > row) {
                found = true;
            } else {
                itemBefore_p = *iter;
                ++iter;
            }
        } /* while */
    }

    if (insert) {
        bool ok;
        QString description("Enter your bookmark comment here");
        QString text = QInputDialog::getText(parent, QString("Create bookmark"), QString("Enter description:"),
                                             QLineEdit::Normal, description, &ok);
        bookmark_p = new CCfgItem_Bookmark( /*this, m_bookmarks_p->m_h_treeItem,*/ text, row, m_bookmarks_p);
        bookmark_p->InsertItem(false, true, true, itemBefore_p);
    }

    /* The one line filter puts the bookmark in the FIR which is essential to get it shown. */
    doc_p->StartOneLineFiltering(row);
}

/***********************************************************************************************************************
*   AddBookmark
***********************************************************************************************************************/
void CWorkspace::AddBookmark(const QString& comment, int row)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    bool found = false;
    bool insert = true;
    CCfgItem_Bookmark *bookmark_p = nullptr;
    CCfgItem *itemBefore_p = m_bookmarks_p;

    if (row > doc_p->m_database.TIA.rows) {
        TRACEX_W("CWorkspace::AddBookmark  Trying to add bookmark to row outside logfile "
                 "max:%d row:%d", doc_p->m_database.TIA.rows, row);
        return;
    }
    TRACEX_D("CWorkspace::AddBookmark");

    /* If a bookmark already exist at this row then nothing shall be done (cannot add twice)
     * Based on row, find bookmark that is before and after */

    if (!m_bookmarks_p->m_cfgChildItems.isEmpty()) {
        QList<CCfgItem *>::iterator iter = m_bookmarks_p->m_cfgChildItems.begin();
        while (iter != m_bookmarks_p->m_cfgChildItems.end() && !found) {
            /* If possible, loop through all the existing bookmarks and find the other bookmark that should be
             * infront. It is possible that this bookmark should be infront, then it is directly found and
             * itemBefore_p would then keep its (m_bookmarks_p) value. If none is found then it itemBefore_p
             * contains the current last bookmark in the list. */
            bookmark_p = static_cast<CCfgItem_Bookmark *>(*iter);

            if (bookmark_p->m_row == row) {
                insert = false;
                found = true;
            } else if (bookmark_p->m_row > row) {
                found = true;
            } else {
                itemBefore_p = (CCfgItem *)bookmark_p;
                ++iter;
            }
        } /* while */
    }

    if (insert) {
        bookmark_p = new CCfgItem_Bookmark( /*this, m_bookmarks_p->m_h_treeItem,*/ comment, row, m_bookmarks_p);
        bookmark_p->InsertItem(false, true, true, itemBefore_p);
    }

    doc_p->StartOneLineFiltering(row);
}

/***********************************************************************************************************************
*   isBookmarked
***********************************************************************************************************************/
bool CWorkspace::isBookmarked(int row)
{
    for (auto cfgItem_p : m_bookmarks_p->m_cfgChildItems) {
        auto bookmark_p = static_cast<CCfgItem_Bookmark *>(cfgItem_p);
        if (bookmark_p->m_row == row) {
            return true;
        } else if (bookmark_p->m_row > row) {
            return false;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   GetClosestBookmark
***********************************************************************************************************************/
int CWorkspace::GetClosestBookmark(int row)
{
    bool first = true;
    int diff = 0;
    unsigned min_diff = 0;
    unsigned int min_row = 0;

    if (m_bookmarks_p->m_cfgChildItems.count() == 0) {
        return -1;
    }

    for (auto cfgItem_p : m_bookmarks_p->m_cfgChildItems) {
        auto bookmark_p = static_cast<CCfgItem_Bookmark *>(cfgItem_p);
        diff = bookmark_p->m_row > row ? bookmark_p->m_row - row : row - bookmark_p->m_row;
        if (first || (diff < (int)min_diff)) {
            min_diff = diff;
            min_row = bookmark_p->m_row;
            first = false;
        }
    }
    return min_row;
}

/***********************************************************************************************************************
*   GetBookmarkList
***********************************************************************************************************************/
bool CWorkspace::GetBookmarkList(QList<int> *bookmarkList_p)
{
    for (auto cfgItem_p : m_bookmarks_p->m_cfgChildItems) {
        auto bookmark_p = static_cast<CCfgItem_Bookmark *>(cfgItem_p);
        bookmarkList_p->append(bookmark_p->m_row);
    }
    return false;
}

/***********************************************************************************************************************
*   NextBookmark
***********************************************************************************************************************/
bool CWorkspace::NextBookmark(int currentRow, int *bookmarkRow_p, bool backward)
{
    if (m_bookmarks_p->m_cfgChildItems.isEmpty()) {
        return false;
    }

    bool found = false;
    QList<CCfgItem *>::iterator iter;
    if (!backward) {
        iter = m_bookmarks_p->m_cfgChildItems.begin();
        for (auto item_p : m_bookmarks_p->m_cfgChildItems) {
            auto bookmark_p = static_cast<CCfgItem_Bookmark *>(item_p);
            if (bookmark_p->m_row > currentRow) {
                *bookmarkRow_p = bookmark_p->m_row;
                return true;
            }
        } /* while */
        /* Wrap... take the first one */
        *bookmarkRow_p = static_cast<CCfgItem_Bookmark *>(m_bookmarks_p->m_cfgChildItems.last())->m_row;
        return true;
    } else {
        /* backwards */
        iter = m_bookmarks_p->m_cfgChildItems.end(); /* end() returns an item outside the list */
        --iter;
        while (iter != m_bookmarks_p->m_cfgChildItems.begin() && !found) {
            auto bookmark_p = static_cast<CCfgItem_Bookmark *>(*iter);
            if (bookmark_p->m_row < currentRow) {
                *bookmarkRow_p = bookmark_p->m_row;
                return true;
            }
            --iter;
        } /* while */
        /* Wrap... take the last one */
        *bookmarkRow_p = static_cast<CCfgItem_Bookmark *>(m_bookmarks_p->m_cfgChildItems.first())->m_row;
        return true;
    }
}

/***********************************************************************************************************************
*   RemoveAllBookmarks
***********************************************************************************************************************/
void CWorkspace::RemoveAllBookmarks(void)
{
    if ((m_bookmarks_p == nullptr) || m_bookmarks_p->m_cfgChildItems.isEmpty()) {
        return;
    }
    m_bookmarks_p->RemoveAllChildren();
}

/***********************************************************************************************************************
*   CloseAllPlugins
***********************************************************************************************************************/
void CWorkspace::CloseAllPlugins(void)
{
    TRACEX_I("Close all plugins");

    if ((m_plugins_p == nullptr) || m_plugins_p->m_cfgChildItems.isEmpty()) {
        return;
    }

    g_processingCtrl_p->Processing_StartReport();

    m_plugins_p->RemoveAllChildren();

    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->PluginIsUnloaded();

    g_processingCtrl_p->Processing_StopReport();
}

/***********************************************************************************************************************
*   CloseAllSelectedPlugins
***********************************************************************************************************************/
void CWorkspace::CloseAllSelectedPlugins(void)
{
    TRACEX_I("Close all selected Plugins");

    QList<CCfgItem *> selectionList;
    if (!CWorkspace_TreeView_GetSelections(CFG_ITEM_KIND_PlugIn, selectionList)) {
        return;
    }

    g_processingCtrl_p->Processing_StartReport();

    while (!selectionList.isEmpty()) {
        auto item = selectionList.takeLast();
        if (nullptr != item) {
            CCfgItem_Delete(item);
        }
    }

    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->PluginIsUnloaded();

    g_processingCtrl_p->Processing_StopReport();
}

/***********************************************************************************************************************
*   TakeFocus
***********************************************************************************************************************/
void CWorkspace::TakeFocus(void)
{
    MW_SetWorkspaceWidgetFocus();
    TRACEX_DE(QString("%1 SetFocus").arg(__FUNCTION__));
}

/***********************************************************************************************************************
*   GetCfgItem
* Will search the entire tree for an item with matching file name (entire path)... will stop at the first
***********************************************************************************************************************/
CCfgItem *CWorkspace::GetCfgItem(const QString& fileName)
{
    CCfgItem *CCfgItem_Parent_p;

    for (int kind = 0; kind < 2; ++kind) {
        CCfgItem_Parent_p = nullptr;

        /* Setup the correct item parent */

        switch (kind)
        {
            case 0:
                CCfgItem_Parent_p = m_filters_p;
                break;

            case 1:
                CCfgItem_Parent_p = m_plugins_p;
                break;

            default:
                continue;  /* no support for this iteration */
        }

        if ((CCfgItem_Parent_p != nullptr) && !CCfgItem_Parent_p->m_cfgChildItems.isEmpty()) {
            for (auto cfgItem_p : CCfgItem_Parent_p->m_cfgChildItems) {
                auto itemName = cfgItem_p->GetFileName();
                if (!itemName.isEmpty() && (itemName == fileName)) {
                    return cfgItem_p;
                }
            }
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   index
***********************************************************************************************************************/
QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
    /*
     *  Returns the index of the item in the model specified by the given row, column and parent index.
     *  When reimplementing this function in a subclass, call createIndex() to generate model indexes that
     *  other components can use to refer to items in your model.
     */
    if ((column != 0) || (row < 0)) {
        return QModelIndex();
    }

    if (parent.isValid() && (parent.internalPointer() != nullptr)) {
        CCfgItem *parentItem = static_cast<CCfgItem *>(parent.internalPointer());
        CCfgItem *childItem = parentItem->GetChildAt(row);

        if (childItem) {
            return createIndex(row, column, childItem);
        }
    } else {
        CCfgItem *childItem = g_workspace_p->m_root_p->GetChildAt(row);

        if (childItem) {
            return createIndex(row, column, childItem);
        }
    }

    return QModelIndex();
}

/***********************************************************************************************************************
*   parent
***********************************************************************************************************************/
QModelIndex Model::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        CCfgItem *childItem = static_cast<CCfgItem *>(child.internalPointer());
        if (childItem->m_tag != CCFG_ITEM_TAG) {
            return QModelIndex();
        }

        CCfgItem *parentItem = childItem->Parent();
        if (parentItem) {
            int row = itemRow(parentItem);
            return createIndex(row, 0, parentItem);
        }
    }
    return QModelIndex();
}

/***********************************************************************************************************************
*   rowCount
***********************************************************************************************************************/
int Model::rowCount(const QModelIndex &parent) const
{
    int rowCount = 0;
#ifdef _DEBUG

    /*TRACEX_D(QString("rowCount")); */
#endif
    if (parent.isValid()) {
        /* return the number childs of the parent of the parentIndex */
        CCfgItem *current_p = static_cast<CCfgItem *>(parent.internalPointer());

        if ((current_p != nullptr) && !current_p->m_cfgChildItems.isEmpty()) {
            rowCount = current_p->m_cfgChildItems.count();
#ifdef _DEBUG

            /*TRACEX_D(QString("    current:%1 kind:%2 rowCount:%3")
             * .arg(QString(QString::number(reinterpret_cast<int>(current_p), 16)))
             * .arg(current_p->m_itemKind).arg(rowCount));*/
#endif
        } else {
            rowCount = 1;
#ifdef _DEBUG

            /*  TRACEX_D(QString("    ERROR current:nullptr rowCount:%1").arg(rowCount)); */
#endif
        }
    } else {
        /* give the count of the children of root */
        rowCount = g_workspace_p->m_root_p->m_cfgChildItems.count();
#ifdef _DEBUG

        /*TRACEX_D(QString("    root rowCount:%1").arg(rowCount)); */
#endif
    }
    return rowCount;
}

/***********************************************************************************************************************
*   columnCount
***********************************************************************************************************************/
int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

/***********************************************************************************************************************
*   insertRows
***********************************************************************************************************************/
bool Model::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count);

    endInsertRows();

    return true;
}

/***********************************************************************************************************************
*   data
***********************************************************************************************************************/
QVariant Model::data(const QModelIndex &index, int role) const
{
    /*TRACEX_D(QString("Model::data index:%1,%2 role:%3 parent:%4 kind:%5")
     * .arg(index.row()).arg(index.column()).arg(role).arg(parentIndex.row()).arg(CfgItemGetKindString(parent_kind)));
     */
    if (!index.isValid()) {
        return QVariant();
    }

    CCfgItem *item_p = static_cast<CCfgItem *>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        if (nullptr != item_p) {
            QString prefix;
            switch (item_p->m_itemKind)
            {
                case CFG_ITEM_KIND_Graph:
                    prefix = "Graph - ";
                    break;

                case CFG_ITEM_KIND_SubPlot:
                    prefix = "SubPlot - ";
                    break;

                case CFG_ITEM_KIND_SequenceDiagram:
                    prefix = "Sequence - ";
                    break;

                default:
                    break;
            }
            return QVariant(prefix + QString(item_p->m_itemText));
        }
        return QVariant();
    } else if ((role == Qt::BackgroundRole) || (role == Qt::ForegroundRole)) {
        if (item_p != nullptr) {
            if (item_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
                switch (role)
                {
                    case Qt::BackgroundRole:
                    {
                        QRgb bg_color = static_cast<CCfgItem_FilterItem *>(item_p)->m_filterItem_ref_p->m_bg_color;
                        return bg_color == BACKGROUND_COLOR ? QVariant() : QVariant(QBrush(QColor(bg_color)));
                    }

                    case Qt::ForegroundRole:
                        return QVariant(QBrush(QColor(static_cast<CCfgItem_FilterItem *>
                                                      (item_p)->m_filterItem_ref_p->m_color)));

                    default:
                        return QVariant();
                }
            }
        }
    } else if (role == Qt::FontRole) {
        /* First column items are bold. */
        if (item_p != nullptr) {
            QFont font;
            font.setPointSize(g_cfg_p->m_workspace_FontSize);
            if (item_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
                if (static_cast<CCfgItem_FilterItem *>(item_p)->m_filterItem_ref_p->m_exclude) {
                    font.setStrikeOut(true);
                }
            }
            return font;
        }
        return QVariant();
    } else if (role == Qt::CheckStateRole) {
        switch (item_p->m_itemKind)
        {
            case CFG_ITEM_KIND_FilterItem:
                return static_cast<CCfgItem_FilterItem *>(item_p)->m_filterItem_ref_p->m_enabled ?

                       Qt::Checked : Qt::Unchecked;

            default:
                return QVariant();
        }
    } else if (role == Qt::DecorationRole) {
        if (item_p != nullptr) {
            switch (item_p->m_itemKind)
            {
                case CFG_ITEM_KIND_Log:
                    return QIcon(":text.png");

                case CFG_ITEM_KIND_Bookmark:
                    return QIcon(":blue_bookmark.ico");

                case CFG_ITEM_KIND_PlugIn:
                    return QIcon(":plugin_32x32.png");

                case CFG_ITEM_KIND_FilterItem:
                    return QIcon(":filter_12_14.bmp");

                case CFG_ITEM_KIND_Filter:
                    return QIcon(":filterItem.bmp");

                case CFG_ITEM_KIND_Decoder:
                    return QIcon(":IDB_DECODER");

                case CFG_ITEM_KIND_Plot:
                    return QIcon(":IDB_PLOT");

                case CFG_ITEM_KIND_SubPlot:
                    return QIcon(":text.png");

                case CFG_ITEM_KIND_Graph:
                    return QIcon(":text.png");

                case CFG_ITEM_KIND_SequenceDiagram:
                    return QIcon(":text.png");

                case CFG_ITEM_KIND_Comment:

                    /* Todos... */
                    break;

                case CFG_ITEM_KIND_BookmarkRoot: /* Root items, should be folders */
                case CFG_ITEM_KIND_Root:
                case CFG_ITEM_KIND_FilterRoot:
                case CFG_ITEM_KIND_CommentRoot:
                case CFG_ITEM_KIND_PlugInRoot:
                case CFG_ITEM_KIND_DecoderRoot:
                case CFG_ITEM_KIND_PlotRoot:
                case CFG_ITEM_KIND_LogRoot:
                    return iconProvider.icon(QFileIconProvider::Folder);

                default:
                    break;
            }
        }

        if (index.column() == 0) {
            return iconProvider.icon(QFileIconProvider::Folder);
        }

        /*return QIcon(); */
        return iconProvider.icon(QFileIconProvider::File);
    }
    return QVariant();
}

/***********************************************************************************************************************
*   headerData
***********************************************************************************************************************/
QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        return QString("Workspace");
    }
    if (role == Qt::DecorationRole) {
        return QVariant::fromValue(services);
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

/***********************************************************************************************************************
*   setData
***********************************************************************************************************************/
bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    CCfgItem *item_p = static_cast<CCfgItem *>(index.internalPointer());

    TRACEX_D(QString("Model::setData index:%1,%2 role:%3 user_type:%4 value:%5")
                 .arg(index.row()).arg(index.column()).arg(role).arg(value.userType()).arg(value.toBool()));

    switch (item_p->m_itemKind)
    {
        case CFG_ITEM_KIND_FilterItem:
            if (role == Qt::CheckStateRole) {
                bool enabled = static_cast<CCfgItem_FilterItem *>(item_p)->m_filterItem_ref_p->m_enabled;
                auto filterItem = static_cast<CCfgItem_FilterItem *>(item_p);
                filterItem->m_filterItem_ref_p->m_enabled = enabled ? false : true;
                TRACEX_I(QString("FilterItem: %1 - %2")
                             .arg(filterItem->m_itemText)
                             .arg(filterItem->m_filterItem_ref_p->m_enabled ? QString("Enabled") : QString("Disabled")));

                emit dataChanged(index, index);
                return true;
            }
            break;

        default:
            return false;
    }
    return false;
}

/***********************************************************************************************************************
*   hasChildren
***********************************************************************************************************************/
bool Model::hasChildren(const QModelIndex &parent) const
{
    if (parent.internalPointer() != nullptr) {
        return !static_cast<CCfgItem *>(parent.internalPointer())->m_cfgChildItems.isEmpty();
    }
    return true;
}

/***********************************************************************************************************************
*   flags
***********************************************************************************************************************/
Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    CCfgItem *current_p = static_cast<CCfgItem *>(index.internalPointer());
    if (current_p != nullptr) {
        CCfgItem *item_p = reinterpret_cast<CCfgItem *>(current_p);

        switch (item_p->m_itemKind)
        {
            case CFG_ITEM_KIND_FilterItem:
                flags |= Qt::ItemIsUserCheckable;
                flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

                /*flags |= Qt::ItemIsEditable; */
                break;

            case CFG_ITEM_KIND_Filter:
                flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
                break;

            case CFG_ITEM_KIND_Log:
            case CFG_ITEM_KIND_LogRoot:
            case CFG_ITEM_KIND_Root:
            case CFG_ITEM_KIND_FilterRoot:
            case CFG_ITEM_KIND_BookmarkRoot:
            case CFG_ITEM_KIND_Bookmark:
            case CFG_ITEM_KIND_PlugInRoot:
            case CFG_ITEM_KIND_PlugIn:
            case CFG_ITEM_KIND_DecoderRoot:
            case CFG_ITEM_KIND_Decoder:
            case CFG_ITEM_KIND_PlotRoot:
            case CFG_ITEM_KIND_Plot:
            case CFG_ITEM_KIND_SubPlot:
            case CFG_ITEM_KIND_Graph:
            case CFG_ITEM_KIND_SequenceDiagram:
                break;

            case CFG_ITEM_KIND_CommentRoot:
            case CFG_ITEM_KIND_Comment:
            default:
                flags &= ~Qt::ItemIsEnabled;
                break;
        }
    }
    return flags;
}

/***********************************************************************************************************************
*   itemRow
***********************************************************************************************************************/
int Model::itemRow(CCfgItem *item_p) const
{
    if (item_p->Parent() != nullptr) {
        return item_p->Parent()->m_cfgChildItems.indexOf(item_p);
    } else {
        return g_workspace_p->m_root_p->m_cfgChildItems.indexOf(item_p);
    }
}

/***********************************************************************************************************************
*   supportedDragActions
***********************************************************************************************************************/
Qt::DropActions Model::supportedDragActions() const
{
    /*http://doc.qt.io/qt-4.8/model-view-programming.html#using-drag-and-drop-with-item-views */
    return Qt::CopyAction | Qt::MoveAction;
}

/***********************************************************************************************************************
*   supportedDropActions
***********************************************************************************************************************/
Qt::DropActions Model::supportedDropActions() const
{
    /*http://doc.qt.io/qt-4.8/model-view-programming.html#using-drag-and-drop-with-item-views */
    return Qt::CopyAction | Qt::MoveAction;
}

/***********************************************************************************************************************
*   mimeTypes
***********************************************************************************************************************/
QStringList Model::mimeTypes() const
{
    QStringList types;
    types << "application/vnd.text.list" << "application/lsz.bytestream";
    return types;
}

/***********************************************************************************************************************
*   mimeData
***********************************************************************************************************************/
QMimeData *Model::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

#ifdef _DEBUG
    QModelIndexList selectionList;
    CWorkspace_TreeView_GetSelectionIndexes(selectionList);
    for (auto& index : selectionList) {
        TRACEX_DE(QString("selection row:%1").arg(index.row()));
    }
#endif

    QModelIndexList sorted_list = indexes;
    std::sort(sorted_list.begin(), sorted_list.end());

    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    stream << MIME_STREAM_START_TAG; /* start tag */
    for (auto& index : sorted_list) {
        TRACEX_DE(QString("mime row:%1").arg(index.row()));
        if (index.isValid()) {
            CCfgItem *cfgItem_p = reinterpret_cast<CCfgItem *>(index.internalPointer());
            stream << MIME_ITEM_START_TAG;
            stream << reinterpret_cast<qulonglong>(cfgItem_p);
            cfgItem_p->Serialize(stream, true);
            stream << MIME_ITEM_END_TAG;
        }
    }
    stream << MIME_STREAM_END_TAG;

    mimeData->setData("application/lsz.bytestream", encodedData);
    return mimeData;
}

/***********************************************************************************************************************
*   dropMimeData
***********************************************************************************************************************/
bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
/* Past from clipboard:
 * https://stackoverflow.com/questions/32980629/how-to-implement-clipboard-actions-for-custom-mime-types
 * */

    if (action == Qt::IgnoreAction) {
        return true;
    }

    CCfgItem *parentItem_p = static_cast<CCfgItem *>(parent.internalPointer());
    TRACEX_DE(QString("drop rows row:%1 action:%2 Parent:%3")
                  .arg(row).arg(action).arg(parentItem_p->m_itemText));

    QModelIndex trueParentIndex;

    CWorkspace_TreeView_UnselectAll();

    /*
     *  When row and column are -1 it means that the dropped data should be considered as dropped directly on
     * parent/item.
     *  Usually this will mean appending the data as child items of parent. If row and column are greater than or equal
     *  zero, it means that the drop occurred just before the specified row and column in the specified parent. */
    if (row == -1) {
        switch (parentItem_p->m_itemKind)
        {
            case CFG_ITEM_KIND_FilterItem:

                /* The item was dropped onto a filterItem, this shall not be the parent, only the item after. Instead,
                 * find the true parent */
                row = parentItem_p->index() + 1; /* start after the filterItem we are dropping onto */
                parentItem_p = parentItem_p->m_itemParent_p;
                trueParentIndex = toIndex(parentItem_p);
                break;

            case CFG_ITEM_KIND_Filter:

                /* The item was dropped onto a filter */
                row = 0;
                trueParentIndex = toIndex(parentItem_p);
                break;

            default:
                return false;
        }
    }

    if (data->hasFormat("application/vnd.text.list")) {
        /* Create a filter item at the index, at least if the row/column is among filterItems or in a filter */

        QByteArray encodedData = data->data("application/vnd.text.list");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QStringList newItems;
        int rows = 0;

        while (!stream.atEnd()) {
            QString text;
            stream >> text;
            newItems << text;
            ++rows;
        }

        return true;
    }

    if (data->hasFormat("application/lsz.bytestream")) {
        QByteArray encodedData = data->data("application/lsz.bytestream");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        /*
         *  const quint32 MIME_STREAM_START_TAG = 0x11111111;
         *  const quint32 MIME_ITEM_START_TAG = 0x22222222;
         *  const quint32 MIME_ITEM_END_TAG = 0x33333333;
         *  const quint32 MIME_STREAM_END_TAG = 0x44444444;
         */
        quint32 streamTag;

        stream >> streamTag;

        Q_ASSERT(streamTag == MIME_STREAM_START_TAG);
        if (streamTag != MIME_STREAM_START_TAG) {
            return false;
        }

        QList<CCfgItem *> src_list;
        QList<CCfgItem *> dest_list;

        while (true) {
            CCfgItem *src;

            stream >> streamTag;
            if (streamTag == MIME_STREAM_END_TAG) {
                break; /* last item parsed */
            }
            Q_ASSERT(streamTag == MIME_ITEM_START_TAG);
            if (streamTag != MIME_ITEM_START_TAG) {
                return false;
            }

            qulonglong pointer;
            stream >> pointer;
            src = reinterpret_cast<CCfgItem *>(pointer);

            qint32 kind;
            stream >> kind;

            if ((kind & 0xffff00) != 0xfeed00) {
                TRACEX_W(QString("Bad kind tag in dropped data"));
                return false;
            }

            switch (static_cast<CfgItemKind_t>(kind & 0xff))
            {
                case CFG_ITEM_KIND_FilterItem:
                {
                    CCfgItem_Filter *filter_p = static_cast<CCfgItem_Filter *>(parentItem_p);
                    Q_ASSERT(filter_p->m_itemKind == CFG_ITEM_KIND_Filter);

                    beginInsertRows(trueParentIndex, row, row);

                    CCfgItem_FilterItem *filterItem_p =
                        new CCfgItem_FilterItem(filter_p->m_filter_ref_p, nullptr, filter_p);
                    filterItem_p->Serialize(stream, false);
                    TRACEX_DE(QString("Begin insert row %1").arg(row));
                    filter_p->m_cfgChildItems.insert(row, filterItem_p);

                    endInsertRows();
                    src_list.append(src);
                    dest_list.append(filterItem_p);

                    row++;

                    break;
                }

                default:
                    return false;
            } /* switch */

            stream >> streamTag;

            if (streamTag != MIME_ITEM_END_TAG) {
                return false;
            }
        } /* while  (true) */

        if (action == Qt::MoveAction) {
            for (auto& src : src_list) {
                CCfgItem_Delete(src);
            }
        }

        for (auto& dest : dest_list) {
            CWorkspace_TreeView_Select(dest);
        }

        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   itemUpdated
***********************************************************************************************************************/
void Model::itemUpdated(CCfgItem *item_p)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }

    QModelIndex qindex = createIndex(item_p->index(), 0, item_p);
    TRACEX_D("IN %s", __FUNCTION__);

    emit dataChanged(qindex, qindex);
    TRACEX_D("OUT %s", __FUNCTION__);
}

/***********************************************************************************************************************
*   beginReset
***********************************************************************************************************************/
void Model::beginReset(void)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
    beginResetModel();
}

/***********************************************************************************************************************
*   endReset
***********************************************************************************************************************/
void Model::endReset(void)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
    endResetModel();
}

static CCfgItem *lastInsertParent = nullptr;

/***********************************************************************************************************************
*   startInsertRows
***********************************************************************************************************************/
void Model::startInsertRows(CCfgItem *parent, const CCfgItem *before, int count)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
#ifdef _DEBUG
    if (lastInsertParent != nullptr) {
        TRACEX_E("stopInsert ROW not called");
    }
#endif

    lastInsertParent = parent;

    if (parent == nullptr) {
#ifdef _DEBUG
        TRACEX_E("Internal error, parent is nullptr");
#endif
        return;
    }

    QModelIndex parentIndex = createIndex(parent->index(), 0, parent);
    int insertIndex = before == nullptr ? parent->m_cfgChildItems.count() : before->index();

    if (insertIndex >= 0) {
        beginInsertRows(parentIndex, insertIndex, insertIndex + count);
    }
#ifdef _DEBUG
    else {
        TRACEX_E("Internal error, before index couldn't be found");
    }
#endif
}

/***********************************************************************************************************************
*   toIndex
***********************************************************************************************************************/
QModelIndex Model::toIndex(CCfgItem *cfgItem_p)
{
    return createIndex(cfgItem_p->index(), 0, cfgItem_p);
}

extern void MW_TV_Expand(QModelIndex& modelIndex);

/***********************************************************************************************************************
*   stopInsertRows
***********************************************************************************************************************/
void Model::stopInsertRows(void)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
    endInsertRows();

    QModelIndex parentIndex = createIndex(lastInsertParent->index(), 0, lastInsertParent);
    MW_TV_Expand(parentIndex);
    lastInsertParent = nullptr;
}

/***********************************************************************************************************************
*   moveRows
***********************************************************************************************************************/
bool Model::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                     const QModelIndex &destinationParent, int destinationChild)
{
    TRACEX_DE(QString("Move rows from:%1 to:%2 count:%3"
                      ).arg(sourceRow).arg(destinationChild).arg(count));

#ifdef _DEBUG
    QModelIndexList selectionList;
    CWorkspace_TreeView_GetSelectionIndexes(selectionList);
    for (auto& index : selectionList) {
        TRACEX_DE(QString("selection row:%1").arg(index.row()));
    }
#endif

    return true;
}

/***********************************************************************************************************************
*   startRemoveRows
***********************************************************************************************************************/
void Model::startRemoveRows(CCfgItem *parent, const CCfgItem *item, int count)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
    if (parent == nullptr) {
#ifdef _DEBUG
        TRACEX_E("Internal error, parent is nullptr");
#endif
        return;
    }

    QModelIndex parentIndex = createIndex(parent->index(), 0, parent);
    beginRemoveRows(parentIndex, item->index(), item->index() + (count - 1));
}

/***********************************************************************************************************************
*   stopRemoveRows
***********************************************************************************************************************/
void Model::stopRemoveRows(void)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }
    endRemoveRows();
}
