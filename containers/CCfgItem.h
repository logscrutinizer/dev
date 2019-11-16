/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CDebug.h"
#include "CFilter.h"
#include "quickFix.h"
#include "plugin_api.h"
#include "../qt/cplotwidget_if.h"

#include <QObject>
#include <QString>
#include <QTreeView>
#include <QLibrary>
#include <QWidget>

#define CCFG_ITEM_TAG 0x5555555555555555
#define CHECK_CCFGITEM(A) Q_ASSERT(A->m_tag == CCFG_ITEM_TAG);

#define MakeEnumStringItem(A) {A, # A}

typedef enum {
    CFG_ITEM_KIND_None = 0,
    CFG_ITEM_KIND_Root = 1,
    CFG_ITEM_KIND_FilterRoot = 2,
    CFG_ITEM_KIND_Filter = 3,
    CFG_ITEM_KIND_FilterItem = 4,
    CFG_ITEM_KIND_CommentRoot = 5,
    CFG_ITEM_KIND_Comment = 6,
    CFG_ITEM_KIND_BookmarkRoot = 7,
    CFG_ITEM_KIND_Bookmark = 8,
    CFG_ITEM_KIND_PlugInRoot = 9,
    CFG_ITEM_KIND_PlugIn = 10,
    CFG_ITEM_KIND_DecoderRoot = 11,
    CFG_ITEM_KIND_Decoder = 12,
    CFG_ITEM_KIND_PlotRoot = 13,
    CFG_ITEM_KIND_Plot = 14,
    CFG_ITEM_KIND_SubPlot = 15,
    CFG_ITEM_KIND_Graph = 16,
    CFG_ITEM_KIND_SequenceDiagram = 17,
    CFG_ITEM_KIND_LogRoot = 18,
    CFG_ITEM_KIND_Log = 19
}CfgItemKind_t;

typedef struct {
    int id;
    char enumName[64];
} enumStringItem;

extern enumStringItem CfgItemKindStringArray[];
#define CfgItemGetKindString(A) CfgItemKindStringArray[A].enumName

#define CFGITEM_FILE_OPERATION_NONE     0
#define CFGITEM_FILE_OPERATION_SAVE     1
#define CFGITEM_FILE_OPERATION_SAVE_AS  2
#define CFGITEM_FILE_OPERATION_OPEN     4
#define CFGITEM_FILE_OPERATION_CLOSE    8

typedef int CfgItem_PossibleFileOperations_t;

typedef enum {
    CFG_ITEM_UPDATE_PURPOSE_NEW_PARENT_HITEM,
    CFG_ITEM_UPDATE_PURPOSE_TREE_REINSERT
}CfgUpdatePurpose_t;

typedef int CfgItemStatus_t;

#define CFG_ITEM_STATUS_BITMASK_MODIFIED    (1 << 0)    /* 1, need to be saved */
#define CFG_ITEM_STATUS_BITMASK_ERROR       (1 << 1)    /* 2, Contains errors */
#define CFG_ITEM_STATUS_BITMASK_WARNING     (1 << 2)    /* 4, Contains warnings */
#define CFG_ITEM_STATUS_BITMASK_DISABLED    (1 << 3)    /* 8, Disabled */
#define CFG_ITEM_STATUS_BITMASK_GRAYED      (1 << 4)    /* 8, Disabled */

typedef int CfgItemSaveOptions_t;

#define CFG_ITEM_SAVE_OPTION_NO_OPTIONS                     0
#define CFG_ITEM_SAVE_OPTION_TO_DEFAULT_WORKSPACE     (1 << 0)
#define CFG_ITEM_SAVE_OPTION_REL_PATH                 (2 << 0)

#define CFG_ITEM_POPUP_RETURN_ACTION_NONE       0   /* Default - Focus, the caller of the popup doesn't need
                                                     * to take any action besides get in focus */
#define CFG_ITEM_POPUP_RETURN_ACTION_NO_FOCUS   1   /* After calling the popup action it shall not take
                                                     * focus */

extern void CPlotPane_SetPlotFocus(CPlot *plot_p);
extern void CPlotPane_SetSubPlotFocus(CSubPlot *subPlot_p);
extern void CEditorWidget_SetFocusRow(int row);

/***********************************************************************************************************************
*   CCfgItem
***********************************************************************************************************************/
class CCfgItem : public QObject
{
    Q_OBJECT

public:
    CCfgItem() {}
    CCfgItem(CCfgItem *itemParent_p)
    {
        init();
        m_itemParent_p = itemParent_p;
    }

private:
    /****/
    void init(void)
    {
        m_itemKind = CFG_ITEM_KIND_None;
        m_itemText = "";
        m_itemParent_p = nullptr;
        m_removeFromParent = true;
        m_temp_itemState = 0;
        m_temp_itemStateMask = 0;
        m_temp_itemStateValid = false;
        m_always_exist = false;
    }

protected:
    virtual ~CCfgItem()
    {
        /* To delete an CCfgItem the RemoveAllChildren must be called first, and then the object can be deleted. */
        if (!m_cfgChildItems.isEmpty()) {
#ifdef DEBUG
            TRACEX_E("Internal: Child not empty")
#endif
        }

        if (!m_prepareDeleteCalled) {
#ifdef DEBUG
            TRACEX_E("Internal: !m_prepareDeleteCalled")
#endif
        }
    }

public:
    friend void CCfgItem_Delete(CCfgItem *cfgItem_p);

    virtual void PrepareDelete(void);

    void Set(const QString& itemText, const CfgItemStatus_t itemStatus, const CfgItemKind_t itemKind);
    void TakeChildFromList(CCfgItem *cfgItem_p); /* this function will not delete the object, hence can be called from
                                                  * the childs destructor */
    void RemoveAllChildren(void);

    /****/
    int index(void) const /* Get this item index, if it is root of list then return -1 */
    {
        Q_ASSERT(m_tag == CCFG_ITEM_TAG);
        if (m_itemParent_p == nullptr) {
            return 0;
        } else {
            int index = 0;
            for (auto cfgItem : m_itemParent_p->m_cfgChildItems) {
                if (cfgItem == this) {
                    return index;
                }
                ++index;
            }
#ifdef _DEBUG
            TRACEX_E(QString("Internal error, item not found in parent list when getting items index"))
#endif
            return 0;
        }
    }

    /****/
    CCfgItem *GetChildAt(const int index)
    {
        Q_ASSERT(m_tag == CCFG_ITEM_TAG);
        if (m_cfgChildItems.isEmpty() || (m_cfgChildItems.count() < index + 1)) {
            return nullptr;
        }
        return m_cfgChildItems.at(index);
    }

    CCfgItem *Parent() {Q_ASSERT(m_tag == CCFG_ITEM_TAG); return m_itemParent_p;}

    void PlotAllChildren(QList<CCfgItem *> *cfgPlotList_p, QList<CPlot *> *plotList_p);
    virtual void PropertiesDlg(QWidget *widget_p) {}
    virtual void InsertItem(bool select = true, bool expand = true, bool insert = true,
                            CCfgItem *itemBefore_p = nullptr); /* if itemBefore_p is nullptr then insert last */
    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) {return 0;}
    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QPoint *pt_p) {return 0;}  /* Return non-zero if caller
                                                                                            * must take action */
    virtual void OnDblClick(QWidget *parent);
    virtual void OnSelection(void) {}     /* Action when item has been selected */

    virtual void Save(QList<CCfgItem *> *selectionList_p = nullptr) {}
    virtual void SaveAs(void) {}
    virtual void Serialize(QDataStream& dstream, bool pack = true); /* Pack data into the dstream, or pack it up */
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) {return true;}
    bool WriteChildrensToFile(QTextStream& fileStream, CfgItemSaveOptions_t options);
    bool WriteTagToFile(QTextStream& fileStream, const QString& tag);

    virtual void UpdateName(void) {} /* This will be called by UpdateChildNames */
    void UpdateTreeName(const QString& name); /* This is a helper function which the sub-classes may use */
    virtual QString GetFileName(void) {return QString();}
    void OpenContainingFolder(const QString *fileName_p);

    /****/
    static bool AllSelectionsOfSameKind(QList<CCfgItem *> *selectionList_p)
    {
        if (selectionList_p->isEmpty()) {return false;}

        CfgItemKind_t firstKind = selectionList_p->first()->m_itemKind;
        for (auto& cfgItem_p : *selectionList_p) {
            if (cfgItem_p->m_itemKind != firstKind) {return false;}
        }
        return true;
    }

public:
    uint64_t m_tag = CCFG_ITEM_TAG;
    CCfgItem *m_itemParent_p;
    QString m_itemText;
    CfgItemStatus_t m_itemStatus;
    CfgItemKind_t m_itemKind;
    bool m_inDestructor; /* When a child is destructed it calls its parent RemoveChild, this shall be
                          * prohibited if parent is in its destructor */
    bool m_removeFromParent; /* In case the parent initated the child delete, then the child shall not remove
                              * itself from the parent list */
    bool m_prepareDeleteCalled; /* Set to true after prepare delete has been called, all cfgitems shouldn't be
                                 * removed before the prepare delete has been called */
    QList<CCfgItem *> m_cfgChildItems;
    int m_temp_itemState; /* Used when reorder/reinserting items, to restore its previous state. Temporarily
                           * used */
    int m_temp_itemStateMask; /* Used when reorder/reinserting items, to restore its previous state. Temporarily
                               * used */
    bool m_temp_itemStateValid; /* True if m_temp_itemState is been setup */
    bool m_always_exist; /* If set to true this item shall not be possible to delete, such as top level items
                          * (root, filters, logs.. .etc. Each item must set this themselves, default is false */
};

void CheckRemoveSelection(CCfgItem *cfgItem_p, QList<CCfgItem *> *selectionList_p);

/***********************************************************************************************************************
*   CCfgItem_FilterItem
***********************************************************************************************************************/
class CCfgItem_FilterItem : public CCfgItem
{
public:
    CCfgItem_FilterItem(
        CFilter *parentFilter_ref_p,
        CFilterItem *filterItem_ref_p,
        CCfgItem *itemParent_p);
    CCfgItem_FilterItem(void) : CCfgItem(), m_filterItem_ref_p(nullptr), m_filterParent_ref_p(nullptr) {}
    virtual ~CCfgItem_FilterItem();

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual void OnDblClick(QWidget *parent) override;
    virtual void PropertiesDlg(QWidget *widget_p) override;
    virtual void OnSelection(void) override {CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_FilterItemSel_e);}
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
    virtual void Serialize(QDataStream& dstream, bool pack) override;

    void hash(QDataStream& dstream) const;

public:
    CFilterItem *m_filterItem_ref_p;
    CFilter *m_filterParent_ref_p;
    int m_quickSearchNum; /* If set (not -1), then this char is used to quick search. '0' - '9', 'a' - 'z' */
};

/***********************************************************************************************************************
*   CCfgItem_FilterItem
* It is important to empty the list of filterItems in the CFilter (m_filterItemList). When the filter has been loaded
* from a workspace file or
* a filter file, the CFilter shall be emptied from its filterItems. These filter items are instead referenced from the
* differnt CCfgItem_FilterItems. Otherwise there will be multiple references to the CFilter_FilterItems.
* When a filtering is requested then a copy of the CFilter object is created with all the required CFilter_FilterItems.
***********************************************************************************************************************/
class CCfgItem_Filter : public CCfgItem
{
public:
    CCfgItem_Filter() = delete;
    CCfgItem_Filter(CFilter *filter_p, CCfgItem *itemParent_p)
        : CCfgItem(itemParent_p), m_filter_ref_p(filter_p) {
        Set(*filter_p->GetShortFileName(), 0, CFG_ITEM_KIND_Filter);
    }
    virtual ~CCfgItem_Filter() {}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual void OnSelection(void) override {CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_FilterSel_e);}
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
    virtual QString GetFileName(void) override {return *m_filter_ref_p->GetFileName();}
    virtual void SaveAs(void) override;
    virtual void Save(QList<CCfgItem *> *selectionList_p = nullptr) override;
    virtual void PrepareDelete(void) override;
    virtual void UpdateName(void) override;

    void InsertFilter(bool isReinsert = false);
    bool isReloadPossible(QList<CCfgItem *> *selectionList_p = nullptr);
    void Reload(QList<CCfgItem *> *selectionList_p = nullptr);
    void Enable(bool enable, QList<CCfgItem *> *selectionList_p = nullptr);
    bool isFileExisting(void);

public:
    CFilter *m_filter_ref_p; /* Reference to the filter */
};

/***********************************************************************************************************************
*   CCfgItem_Filters
***********************************************************************************************************************/
class CCfgItem_Filters : public CCfgItem
{
public:
    CCfgItem_Filters(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Filters() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;

    void AppendHashWorkspaceToChildrens();

public:
    QDataStream m_lastFilterHash;
};

/***********************************************************************************************************************
*   CCfgItem_Root
***********************************************************************************************************************/
class CCfgItem_Root : public CCfgItem
{
public:
    CCfgItem_Root(CCfgItem *headerRoot = nullptr);
    virtual ~CCfgItem_Root() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual void UpdateName(void) override;

public:
    QString m_workspaceFileName;
    QString m_workspaceShortName;
};

/***********************************************************************************************************************
*   CCfgItem_Logs
***********************************************************************************************************************/
class CCfgItem_Logs : public CCfgItem
{
public:
    CCfgItem_Logs(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Logs() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
};

/***********************************************************************************************************************
*   CCfgItem_Log
***********************************************************************************************************************/
class CCfgItem_Log : public CCfgItem
{
public:
    CCfgItem_Log(char *path_p, CCfgItem *itemParent_p);
    virtual ~CCfgItem_Log() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
    virtual void PrepareDelete(void) override;

public:
    QString m_path;
};

/***********************************************************************************************************************
*   CCfgItem_Decoders
***********************************************************************************************************************/
class CCfgItem_Decoders : public CCfgItem
{
public:
    CCfgItem_Decoders(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Decoders() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}
};

/***********************************************************************************************************************
*   CCfgItem_Decoder
***********************************************************************************************************************/
class CCfgItem_Decoder : public CCfgItem
{
public:
    CCfgItem_Decoder(CDecoder *decoder_ref_p, CCfgItem *itemParent_p);
    virtual ~CCfgItem_Decoder() {TRACEX_DE(QString("%1 %2").arg(__FUNCTION__).arg(m_itemText))}

    virtual void PrepareDelete(void) override;

public:
    bool m_enabled;
    CDecoder *m_decoder_ref_p;
};

/***********************************************************************************************************************
*   CCfgItem_Plugins
***********************************************************************************************************************/
class CCfgItem_Plugins : public CCfgItem
{
public:
    CCfgItem_Plugins(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Plugins() {TRACEX_DE("Destructor CCfgItem_Plugins")}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
};

/***********************************************************************************************************************
*   CCfgItem_Plugin
***********************************************************************************************************************/
class CCfgItem_Plugin : public CCfgItem
{
public:
    CCfgItem_Plugin(
        CPlugin_DLL_API *dll_api_p,
        QLibrary *library_p,
        CCfgItem *itemParent_p);

    virtual ~CCfgItem_Plugin() {
        TRACEX_DE(QString("%1 %2").arg(__FUNCTION__).arg(m_itemText))
    }

    virtual void PrepareDelete(void) override;
    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
    virtual QString GetFileName(void) override {return m_path;}

public:
    QString GetInfo(void);
    CPlugin_DLL_API *m_dll_api_p;
    QLibrary *m_library_p;
    QString m_path;
    QString m_fileName;
    DLL_API_PluginInfo_t m_info;
};

/***********************************************************************************************************************
*   CCfgItem_Bookmarks
***********************************************************************************************************************/
class CCfgItem_Bookmarks : public CCfgItem
{
public:
    CCfgItem_Bookmarks(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Bookmarks() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
};

/***********************************************************************************************************************
*   CCfgItem_Bookmark
***********************************************************************************************************************/
class CCfgItem_Bookmark : public CCfgItem
{
public:
    CCfgItem_Bookmark(const QString& comment, int row, CCfgItem *itemParent_p);
    virtual ~CCfgItem_Bookmark() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
    virtual void OnDblClick(QWidget *parent) override;
    virtual void PropertiesDlg(QWidget *widget_p) override;
    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
    virtual void PrepareDelete(void) override;

public:
    int m_row;
    QString m_comment;
};

/***********************************************************************************************************************
*   CCfgItem_Plots
***********************************************************************************************************************/
class CCfgItem_Plots : public CCfgItem
{
public:
    CCfgItem_Plots(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Plots() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}
    int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;
};

/***********************************************************************************************************************
*   CCfgItem_Plot
***********************************************************************************************************************/
class CCfgItem_Plot : public CCfgItem
{
public:
    CCfgItem_Plot(CPlot *plot_ref_p, CCfgItem *itemParent_p);
    virtual ~CCfgItem_Plot() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual void PrepareDelete(void) override;
    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;

    void PreRunPlot(void);
    void PostRunPlot(void);
    void RunPlot(void);
    void AddElements(void);
    void RemoveElements(void);

    /****/
    virtual void OnSelection(void) override {CPlotPane_SetPlotFocus(m_plot_ref_p);}

    /****/
    virtual void OnDblClick(QWidget *parent) override {Q_UNUSED(parent); CPlotPane_SetPlotFocus(m_plot_ref_p);}

public:
    bool m_enabled;
    CPlot *m_plot_ref_p;
    CPlotWidgetInterface *m_plotWidget_p;
};

/***********************************************************************************************************************
*   CCfgItem_SubPlot
***********************************************************************************************************************/
class CCfgItem_SubPlot : public CCfgItem
{
public:
    CCfgItem_SubPlot(CSubPlot *subPlot_ref_p, CCfgItem *itemParent_p);
    virtual ~CCfgItem_SubPlot() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;

    /****/
    virtual void OnDblClick(QWidget *parent) override {Q_UNUSED(parent); CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);}

public:
    CSubPlot *m_subPlot_ref_p;
};

/***********************************************************************************************************************
*   CCfgItem_Graph
***********************************************************************************************************************/
class CCfgItem_Graph : public CCfgItem
{
public:
    CCfgItem_Graph(CGraph *graph_ref_p, CCfgItem *itemParent_p);
    virtual ~CCfgItem_Graph() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;

    /****/
    virtual void OnDblClick(QWidget *parent) override
    {
        CPlotPane_SetSubPlotFocus(((CCfgItem_SubPlot *)m_itemParent_p)->m_subPlot_ref_p);
    }

public:
    CGraph *m_graph_ref_p;
};

/***********************************************************************************************************************
*   CCfgItem_SequenceDiagram
***********************************************************************************************************************/
class CCfgItem_SequenceDiagram : public CCfgItem
{
public:
    CCfgItem_SequenceDiagram(
        CSequenceDiagram *sequenceDiagram_ref_p,
        CCfgItem *itemParent_p);

    virtual ~CCfgItem_SequenceDiagram() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual int OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p) override;

    /****/
    virtual void OnDblClick(QWidget *parent) override
    {
        CPlotPane_SetSubPlotFocus(((CCfgItem_SubPlot *)m_itemParent_p)->m_subPlot_ref_p);
    }

public:
    CSequenceDiagram *m_sequenceDiagram_ref_p;
};

/***********************************************************************************************************************
*   CCfgItem_Comments
***********************************************************************************************************************/
class CCfgItem_Comments : public CCfgItem
{
public:
    CCfgItem_Comments(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Comments() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;
};

/***********************************************************************************************************************
*   CCfgItem_Comment
***********************************************************************************************************************/
class CCfgItem_Comment : public CCfgItem
{
public:
    CCfgItem_Comment(CCfgItem *itemParent_p);
    virtual ~CCfgItem_Comment() {TRACEX_DE(QString("%1").arg(__FUNCTION__))}

    virtual bool WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options) override;

public:
    int m_rowStart;
    int m_rowEnd;
    int m_colStart;
    int m_colEnd;
    QString m_title;
    QString m_comment;
};

void CCfgItem_Delete(CCfgItem *cfgItem_p);
