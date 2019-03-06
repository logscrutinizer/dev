/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CCfgItem.h"

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"

#include "CLogScrutinizerDoc.h"
#include "CConfigurationCtrl.h"
#include "CWorkspace_cb_if.h"
#include "ceditorwidget_cb_if.h"
#include "cplotpane_cb_if.h"
#include "mainwindow_cb_if.h"

#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMenu>
#include <QTreeView>
#include <QInputDialog>
#include <QList>

static char g_cfgItem_tempString[CFG_TEMP_STRING_MAX_SIZE];
enumStringItem CfgItemKindStringArray[] =
{
    MakeEnumStringItem(CFG_ITEM_KIND_None),
    MakeEnumStringItem(CFG_ITEM_KIND_Root),
    MakeEnumStringItem(CFG_ITEM_KIND_FilterRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Filter),
    MakeEnumStringItem(CFG_ITEM_KIND_FilterItem),
    MakeEnumStringItem(CFG_ITEM_KIND_CommentRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Comment),
    MakeEnumStringItem(CFG_ITEM_KIND_BookmarkRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Bookmark),
    MakeEnumStringItem(CFG_ITEM_KIND_PlugInRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_PlugIn),
    MakeEnumStringItem(CFG_ITEM_KIND_DecoderRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Decoder),
    MakeEnumStringItem(CFG_ITEM_KIND_PlotRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Plot),
    MakeEnumStringItem(CFG_ITEM_KIND_SubPlot),
    MakeEnumStringItem(CFG_ITEM_KIND_Graph),
    MakeEnumStringItem(CFG_ITEM_KIND_SequenceDiagram),
    MakeEnumStringItem(CFG_ITEM_KIND_LogRoot),
    MakeEnumStringItem(CFG_ITEM_KIND_Log)
};

/***********************************************************************************************************************
*   CheckRemoveSelection
***********************************************************************************************************************/
void CheckRemoveSelection(CCfgItem *cfgItem_p, QList<CCfgItem *> *selectionList_p)
{
    selectionList_p->removeOne(cfgItem_p);
}

/***********************************************************************************************************************
*   CCfgItem_Delete
***********************************************************************************************************************/
void CCfgItem_Delete(CCfgItem *cfgItem_p)
{
    CHECK_CCFGITEM(cfgItem_p);
    {
        CWorkspace_RemoveRowsScopeGuard guard(cfgItem_p->m_itemParent_p, cfgItem_p, 1);
        cfgItem_p->PrepareDelete();
    }
    delete cfgItem_p; /* unload */
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem::PrepareDelete(void)
{
    Q_ASSERT(m_tag == CCFG_ITEM_TAG);
    m_prepareDeleteCalled = true;

    RemoveAllChildren();

    if (m_removeFromParent && (m_itemParent_p != nullptr)) {
        m_itemParent_p->TakeChildFromList(this);   /* Remove this object from the parents list of childs */
    }
}

/***********************************************************************************************************************
*   Set
***********************************************************************************************************************/
void CCfgItem::Set(const QString& itemText, const CfgItemStatus_t itemStatus, const CfgItemKind_t itemKind)
{
    Q_ASSERT(m_tag == CCFG_ITEM_TAG);
    m_itemText = itemText;
    m_itemStatus = itemStatus;
    m_itemKind = itemKind;
}

/***********************************************************************************************************************
*   WriteChildrensToFile
***********************************************************************************************************************/
bool CCfgItem::WriteChildrensToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    if (!m_cfgChildItems.isEmpty()) {
        for (auto child : m_cfgChildItems) {
            if (!child->WriteToFile(fileStream, options)) {
                return false;
            }
        }
    }
    return true;
}

/***********************************************************************************************************************
*   WriteTagToFile
***********************************************************************************************************************/
bool CCfgItem::WriteTagToFile(QTextStream& fileStream, const QString& tag)
{
    try {
        fileStream << tag;
    } catch (...) {
        TRACEX_E("CCfgItem::WriteTagToFile  Failed to write item to file Tag:%s", tag.toLatin1().constData());
        return false;
    }
    return true;
}

/***********************************************************************************************************************
*   Serialize
***********************************************************************************************************************/
void CCfgItem::Serialize(QDataStream& dstream, bool pack)
{
    Q_ASSERT(m_tag == CCFG_ITEM_TAG);

    /* The base class serialization is always called before the derived class is processing its specifics. That also
     * mean that a class that is missing its serialize impl. will only get the base class impl. */
    if (pack) {
        dstream << static_cast<qint32>(m_itemKind | MIME_CFG_ITEM_KIND_TAG); /* This it put twice in the stream */
    } else {
        quint32 temp_kind;
        dstream >> temp_kind;
        Q_ASSERT((temp_kind & MIME_CFG_ITEM_KIND_TAG_BITS) != MIME_CFG_ITEM_KIND_TAG); /* This is a sort of stamp to
                                                                                        * ensure correct data is being
                                                                                        * loaded */
        m_itemKind = static_cast<CfgItemKind_t>(temp_kind & MIME_CFG_ITEM_KIND_BITS);
    }
}

/***********************************************************************************************************************
*   PlotAllChildren
***********************************************************************************************************************/
void CCfgItem::PlotAllChildren(QList<CCfgItem *> *cfgPlotList_p, QList<CPlot *> *plotList_p)
{
    auto doc_p = GetTheDoc();
    bool topLevel = plotList_p == nullptr ? true : false;

    if (topLevel) {
        cfgPlotList_p = new QList<CCfgItem *>;
        plotList_p = new QList<CPlot *>;
    }

    for (auto& cfgItem_p : m_cfgChildItems) {
        if (cfgItem_p->m_itemKind == CFG_ITEM_KIND_Plot) {
            auto plotItem_p = reinterpret_cast<CCfgItem_Plot *>(cfgItem_p);

            if (plotList_p->indexOf(plotItem_p->m_plot_ref_p) == -1) {
                /* Only append if the plot doesn't already exist */
                plotList_p->append(plotItem_p->m_plot_ref_p);
                cfgPlotList_p->append(cfgItem_p); /* Not modifying model based list, this list is used for processing */
            }
        } else {
            cfgItem_p->PlotAllChildren(cfgPlotList_p, plotList_p);
        }
    }

    if (topLevel) {
        if (!plotList_p->isEmpty()) {
            for (auto& plot_p : *cfgPlotList_p) {
                static_cast<CCfgItem_Plot *>(plot_p)->PreRunPlot();
            }

            doc_p->StartPlot(plotList_p);

            for (auto& plot_p : *cfgPlotList_p) {
                static_cast<CCfgItem_Plot *>(plot_p)->PostRunPlot();
            }

            extern void CPlotPane_SetPlotFocus(CPlot * plot_p);   /* Set focus the first in the list */
            CPlotPane_SetPlotFocus(plotList_p->first());
        }

        delete plotList_p;
        delete cfgPlotList_p;
    }
}

/***********************************************************************************************************************
*   UpdateTreeName
***********************************************************************************************************************/
void CCfgItem::UpdateTreeName(const QString& name)
{
    m_itemText = name;
    CWorkspace_ItemUpdated(this);
}

/***********************************************************************************************************************
*   InsertItem
***********************************************************************************************************************/
void CCfgItem::InsertItem(bool select, bool expand, bool insert, CCfgItem *itemBefore_p)
{
    /* select, should the item be selected right away
     * expand, should the childs of this item be expanded
     * insert, is it required to add the item to the parent list, or is it already in the list and this is just UI
     * updated (not for QT, or?)
     * last, if the item is last or not */
    Q_UNUSED(expand);

    Q_ASSERT(m_tag == CCFG_ITEM_TAG);
    if (itemBefore_p != nullptr) {
        /* For QT the assumption is that maintaining the view is not done from the model, but vice-versa. Hence there
         * will not be any item manipulations from
         * this code... besides alerting the view about the changes. */

        bool found = false;

        if (insert) {
            /* check if list handling is needed, otherwise this is already done */

            CWorkspace_InsertRowsScopeGuard guard(m_itemParent_p, itemBefore_p, 1, expand);

            if (m_itemParent_p == itemBefore_p) {
                m_itemParent_p->m_cfgChildItems.push_front(this); /* put this object first in the list if before is the
                                                                   * same as parent */
                found = true;
            } else {
                /* Find the position in this items' Parent list, at which POS it should be inserted */
                QList<CCfgItem *>::iterator iter = m_itemParent_p->m_cfgChildItems.begin();
                while (iter != m_itemParent_p->m_cfgChildItems.end() && !found) {
                    if (*iter == itemBefore_p) {
                        found = true;
                        ++iter; /* step over the found (before) element */
                        m_itemParent_p->m_cfgChildItems.insert(iter, this); /* insert before iter (if iter is last that
                                                                             * will be an append) */
                        break;
                    } /* search for the correct item_before */
                    ++iter;
                } /* while */
            } /* if before was parent or not */

            if (!found) {
                /* Emergency forced addition... */
                TRACEX_E("CCfgItem::InsertItem  Item before not found");
                if (insert) {
                    TRACEX_E("CCfgItem::InsertItem   malloc failed");
                    m_itemParent_p->m_cfgChildItems.append(this);
                }
            }
        } /* if insert */
    } else {
        /* last, itemBefore_p == nullptr */
        TRACEX_DE("CCfgItem::InsertItem %s parent:%p before:%p item:%p",
                  m_itemText.toLatin1().constData(),
                  m_itemParent_p,
                  itemBefore_p,
                  this);

        if (insert && (m_itemParent_p != nullptr)) {
            CWorkspace_InsertRowsScopeGuard guard(m_itemParent_p, itemBefore_p, 1);
            m_itemParent_p->m_cfgChildItems.append(this);
        }
    }

    /* Alert the view about the updates */

    if (CSCZ_TreeViewSelectionEnabled && select) {
        CWorkspace_TreeView_UnselectAll();
        CWorkspace_TreeView_Select(this);
    }
}

/***********************************************************************************************************************
*   TakeChildFromList
***********************************************************************************************************************/
void CCfgItem::TakeChildFromList(CCfgItem *cfgItem_p)
{
    /* Only take the child from the list, do not delete it, as this function might be called from the childs destructor
     * */
    if (!m_cfgChildItems.removeOne(cfgItem_p)) {
        TRACEX_W("CCfgItem::TakeChildFromList  Not found in list");
    }
}

/***********************************************************************************************************************
*   RemoveAllChildren
***********************************************************************************************************************/
void CCfgItem::RemoveAllChildren(void)
{
    if (!m_cfgChildItems.isEmpty()) {
        for (auto cfgItem_p : m_cfgChildItems) {
            cfgItem_p->m_removeFromParent = false;
            cfgItem_p->PrepareDelete();
        }

        CWorkspace_RemoveRowsScopeGuard guard(this, m_cfgChildItems.first(), m_cfgChildItems.count());

        while (!m_cfgChildItems.isEmpty()) {
            auto child = m_cfgChildItems.takeLast();
            if (nullptr != child) {
                delete child;
            }
        }
    }
}

/***********************************************************************************************************************
*   OnDblClick
***********************************************************************************************************************/
void CCfgItem::OnDblClick(QWidget *parent)
{
    Q_UNUSED(parent);
}

/***********************************************************************************************************************
*   OpenContainingFolder
***********************************************************************************************************************/
void CCfgItem::OpenContainingFolder(const QString *fileName_p)
{
    QString absoluteFileName;
    QString absoutePath;
    QFileInfo fileInfo(*fileName_p);
    QDesktopServices::openUrl(QUrl(fileInfo.absolutePath()));
}

/***********************************************************************************************************************
*   CCfgItem_Root - CTOR
***********************************************************************************************************************/
CCfgItem_Root::CCfgItem_Root(CCfgItem *headerRoot) : CCfgItem(headerRoot)
{
    Set(QString("Workspace"), 0, CFG_ITEM_KIND_Root);

    m_workspaceFileName = "";
    m_workspaceShortName = "";
    m_always_exist = true;
}

/***********************************************************************************************************************
*   UpdateName
***********************************************************************************************************************/
void CCfgItem_Root::UpdateName(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString m_workspaceFileName = doc_p->GetWorkspaceFileName();

    if (m_workspaceFileName != "") {
        QFileInfo fileInfo(m_workspaceFileName);
        m_workspaceShortName = fileInfo.fileName();

        if (m_workspaceShortName.isEmpty()) {
            m_workspaceShortName = m_workspaceFileName;
            TRACEX_W("CCfgItem_Root::UpdateName - Workspace file name is not a valid path");
        }

        QString treeName = QString("Workspace - ") + m_workspaceShortName;
        UpdateTreeName(treeName);
    } else {
        m_workspaceFileName = "workspace.lsz";
        m_workspaceShortName = "workspace.lsz";

        UpdateTreeName(QString("Workspace"));
    }
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Root::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Root) {
        TRACEX_W("%s Internal error, selection not Root", __FUNCTION__);
        return 0;
    }

    {
        /* Save */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Save"));

        if (!doc_p->GetWorkspaceFileName().isEmpty()) {
            action_p->setEnabled(true);
        } else {
            action_p->setEnabled(false);
        }
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CFGCTRL_SaveWorkspaceFile();});
    }

    {
        /* SaveAs */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Save As..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CFGCTRL_SaveWorkspaceFileAs();});
    }

    {
        /* SaveAsDefault */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Save As Default..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CFGCTRL_SaveDefaultWorkspaceFile();});
    }

    {
        /* OpenContianingFolder */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open containing folder"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            QString workspaceFileName = doc_p->GetWorkspaceFileName();
            OpenContainingFolder(&workspaceFileName);
        });
    }

    {
        /* Close */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Close"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CFGCTRL_UnloadAll();});
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Comments - CTOR
***********************************************************************************************************************/
CCfgItem_Comments::CCfgItem_Comments(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(tr("Comments"), CFG_ITEM_STATUS_BITMASK_DISABLED, CFG_ITEM_KIND_CommentRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Comments::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    bool status = true;

    if (m_cfgChildItems.isEmpty()) {
        return true;
    }

    if (status) {
        status = WriteTagToFile(fileStream, QString("  <Comments>\n"));
    }
    if (status) {
        status = WriteChildrensToFile(fileStream, options);
    }
    if (status) {
        status = WriteTagToFile(fileStream, QString("  </Comments>\n"));
    }

    return status;
}

/***********************************************************************************************************************
*   CCfgItem_Comment - CTOR
***********************************************************************************************************************/
CCfgItem_Comment::CCfgItem_Comment(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Comment_X"), CFG_ITEM_STATUS_BITMASK_DISABLED, CFG_ITEM_KIND_Comment);
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Comment::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    Q_UNUSED(options);
    WriteTagToFile(fileStream, QString("    <Comment text=\"not implemented\" />\n"));
    return false;
}

/***********************************************************************************************************************
*   CCfgItem_Plots - CTOR
***********************************************************************************************************************/
CCfgItem_Plots::CCfgItem_Plots(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(tr("Plots"), 0, CFG_ITEM_KIND_PlotRoot);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Plots::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_PlotRoot) {
        TRACEX_W("%s Internal error, selection not Root", __FUNCTION__);
        return 0;
    }

    {
        /* Run all */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Run all"));
        action_p->setEnabled(m_cfgChildItems.count() > 0 ? true : false);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            PlotAllChildren(nullptr, nullptr);
        });
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Plot - CTOR
***********************************************************************************************************************/
CCfgItem_Plot::CCfgItem_Plot(
    CPlot *plot_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    char *plotName_p;
    char *x_axis_p;

    m_plot_ref_p = plot_ref_p;
    m_enabled = true;

    plot_ref_p->GetTitle(&plotName_p, &x_axis_p);

    Set(QString(plotName_p), 0, CFG_ITEM_KIND_Plot);

    TRACEX_D("  Plot: %s", plotName_p);
    m_plotWidget_p = MW_AttachPlot(m_plot_ref_p);
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Plot::PrepareDelete(void)
{
    TRACEX_DE("CCfgItem_Plot::PrepareDelete");
    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        MW_DetachPlot(m_plot_ref_p);
    }
    CCfgItem::PrepareDelete();
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Plot::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Plot) {
        TRACEX_W("%s Internal error, selection not a plot item", __FUNCTION__);
        return 0;
    }

    {
        /* Run plot */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Run"));
        action_p->setEnabled(true);

        extern void CEditorWidget_SetFocusRow(int row);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            PlotAllChildren(nullptr, nullptr);
        });
    }
    return 1;
}

/***********************************************************************************************************************
*   PreRunPlot
***********************************************************************************************************************/
void CCfgItem_Plot::PreRunPlot(void)
{
    m_plotWidget_p->RemoveSurfaces();
    RemoveElements();
}

/***********************************************************************************************************************
*   PostRunPlot
***********************************************************************************************************************/
void CCfgItem_Plot::PostRunPlot(void)
{
    AddElements();
    m_plotWidget_p->Initialize();
    m_plotWidget_p->Redraw();
}

/***********************************************************************************************************************
*   RunPlot
***********************************************************************************************************************/
void CCfgItem_Plot::RunPlot(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    PreRunPlot();

    QList<CPlot *> plotList;

    plotList.push_front(m_plot_ref_p);
    doc_p->StartPlot(&plotList);

    PostRunPlot();

    extern void CPlotPane_SetPlotFocus(CPlot * plot_p);  /* Set focus the first in the list */
    CPlotPane_SetPlotFocus(m_plot_ref_p);
}

/***********************************************************************************************************************
*   AddElements
***********************************************************************************************************************/
void CCfgItem_Plot::AddElements(void)
{
    /* Go through all subplots in this plot and add graphs for each sub-plot */

    for (auto item_p : m_cfgChildItems) {
        CCfgItem_SubPlot *subplot_item_p = reinterpret_cast<CCfgItem_SubPlot *>(item_p);
        CList_LSZ *graphList_p;
        subplot_item_p->m_subPlot_ref_p->GetGraphs(&graphList_p);

        CCfgItem *graphItem_p = nullptr;
        auto graph_p = reinterpret_cast<CGraph *>(graphList_p->first());
        SubPlot_Properties_t subPlot_Properties = subplot_item_p->m_subPlot_ref_p->GetProperties();
        while (graph_p != nullptr) {
            /* QT Note,  its not really necessary to call the InsertItem function, would
             * be sufficient to just add the list to the child items */
            if (subPlot_Properties & SUB_PLOT_PROPERTY_SEQUENCE) {
                graphItem_p = new CCfgItem_SequenceDiagram(
                    reinterpret_cast<CSequenceDiagram *>(graph_p), subplot_item_p);
                graphItem_p->InsertItem(false, false);
            } else {
                graphItem_p = new CCfgItem_Graph(graph_p, subplot_item_p);
                graphItem_p->InsertItem(false, false);
            }
            graph_p = reinterpret_cast<CGraph *>(graphList_p->GetNext(reinterpret_cast<CListObject *>(graph_p)));
        }
    }
}

/***********************************************************************************************************************
*   RemoveElements
***********************************************************************************************************************/
void CCfgItem_Plot::RemoveElements(void)
{
    /* Go through all subplots in this plot and remove their graphs for each sub-plot */
    for (auto item : m_cfgChildItems) {
        static_cast<CCfgItem_SubPlot *>(item)->RemoveAllChildren();
    }
}

/***********************************************************************************************************************
*   CCfgItem_Bookmarks - CTOR
***********************************************************************************************************************/
CCfgItem_Bookmarks::CCfgItem_Bookmarks(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Bookmarks"), 0, CFG_ITEM_KIND_BookmarkRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Bookmarks::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    bool status = true;

    if (m_cfgChildItems.isEmpty()) {
        return true;
    }

    if (status) {
        status = WriteTagToFile(fileStream, QString("  <bookmarks>\n"));
    }
    if (status) {
        status = WriteChildrensToFile(fileStream, options);
    }
    if (status) {
        status = WriteTagToFile(fileStream, QString("  </bookmarks>\n"));
    }

    return status;
}

/***********************************************************************************************************************
*   CCfgItem_Bookmark - CTOR
***********************************************************************************************************************/
CCfgItem_Bookmark::CCfgItem_Bookmark(
    const QString&          comment,
    int row,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p), m_row(row), m_comment(comment)
{
    QString itemText = QString("%1 - %2").arg(m_row).arg(m_comment);
    Set(itemText, 0, CFG_ITEM_KIND_Bookmark);
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Bookmark::PrepareDelete(void)
{
    TRACEX_DE("CCfgItem_Bookmark::PrepareDelete");

    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        /* check this flag, since doing BookmarkDecorateFIRA takes a lot of time */
        CLogScrutinizerDoc *doc_p = GetTheDoc();
        doc_p->StartOneLineFiltering(m_row, true);
    }

    CCfgItem::PrepareDelete();
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Bookmark::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    Q_UNUSED(options);

    bool status = true;
    QString bookmarkString = QString("    <bookmark row=\"%1\" text=\"%2\" />\n").arg(m_row).arg(m_comment);
    if (status) {
        status = WriteTagToFile(fileStream, bookmarkString);
    }

    return status;
}

/***********************************************************************************************************************
*   OnDblClick
***********************************************************************************************************************/
void CCfgItem_Bookmark::OnDblClick(QWidget *parent)
{
    TRACEX_D("CCfgItem_Bookmark::OnDblClick");

    extern void CEditorWidget_SetFocusRow(int row);
    CEditorWidget_SetFocusRow(m_row);

    CCfgItem::OnDblClick(parent);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Bookmark::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Bookmark) {
        TRACEX_W("%s Internal error, selection not a bookmark item", __FUNCTION__);
        return 0;
    }

    CCfgItem_Bookmark *bookmark_p = static_cast<CCfgItem_Bookmark *>(selectionList_p->first());

    {
        /* Goto */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Go to"));
        action_p->setEnabled(true);

        extern void CEditorWidget_SetFocusRow(int row);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CEditorWidget_SetFocusRow(bookmark_p->m_row);});
    }

    {
        /* Edit */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Edit..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {bookmark_p->PropertiesDlg(treeview_p);});
    }

    {
        /* Remove */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Remove"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CCfgItem_Delete(bookmark_p);});
    }

    return 0;
}

/***********************************************************************************************************************
*   PropertiesDlg
***********************************************************************************************************************/
void CCfgItem_Bookmark::PropertiesDlg(QWidget *widget_p)
{
    bool ok;
    QString description(tr("Enter your bookmark comment here"));
    QString text = QInputDialog::getText(widget_p,
                                         tr("Edit bookmark"),
                                         tr("Enter your description"),
                                         QLineEdit::Normal,
                                         m_comment,
                                         &ok);

    if (ok) {
        m_comment = text;

        QString itemText = QString("%1 - %2").arg(m_row).arg(m_comment);
        Set(itemText, 0, CFG_ITEM_KIND_Bookmark);
        CWorkspace_ItemUpdated(this);
    }
}

CCfgItem_Plugins::CCfgItem_Plugins(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Plugins"), CFG_ITEM_STATUS_BITMASK_DISABLED, CFG_ITEM_KIND_PlugInRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Plugins::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    bool status = true;

    if (m_cfgChildItems.isEmpty()) {
        return true;
    }

    if (status) {
        status = WriteTagToFile(fileStream, QString("  <plugins>\n"));
    }
    if (status) {
        status = WriteChildrensToFile(fileStream, options);
    }
    if (status) {
        status = WriteTagToFile(fileStream, QString("  </plugins>\n"));
    }

    return status;
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Plugins::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_PlugInRoot) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    {
        /* Open Plugin... */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open Plugin..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CFGCTRL_LoadPluginFile();
        });
    }

    menu_p->addSeparator();

    {
        /* Close all Plugins */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Close all Plugins"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CWorkspace_CloseAllPlugins();
        });
    }

    /* RunAll */
    {
        QAction *action_p;
        action_p = menu_p->addAction(tr("Run All"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CWorkspace_RunAllPlots();
        });
    }

    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Plugin - CTOR
***********************************************************************************************************************/
CCfgItem_Plugin::CCfgItem_Plugin(
    CPlugin_DLL_API *dll_api_p,
    QLibrary *library_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p), m_dll_api_p(dll_api_p), m_library_p(library_p)
{
    m_itemKind = CFG_ITEM_KIND_PlugIn;
    m_dll_api_p = dll_api_p;

    CLogScrutinizerDoc *doc_p = GetTheDoc();

    m_path = m_library_p->fileName();

    /* Extract shortName */
    QFileInfo fileInfo(m_path);
    m_fileName = fileInfo.fileName();

    QString pluginName = QString("%1").arg(m_dll_api_p->GetInfo()->name);
    Set(pluginName, 0, CFG_ITEM_KIND_PlugIn);

    itemParent_p->m_itemStatus &= ~CFG_ITEM_STATUS_BITMASK_DISABLED; /* Enable parent */

    InsertItem();

    CList_LSZ *list_p;

    m_info = *(dll_api_p->GetInfo());

    QString info = GetInfo();
    TRACEX_D(info);

    if (dll_api_p->GetDecoders(&list_p) && !list_p->isEmpty()) {
        CCfgItem_Decoders *CfgDecoders_p = new CCfgItem_Decoders(this);
        CfgDecoders_p->InsertItem();

        auto *decoder_p = static_cast<CDecoder *>(list_p->first());

        while (decoder_p != nullptr) {
            CCfgItem_Decoder *CfgDecoder_p = new CCfgItem_Decoder(decoder_p, CfgDecoders_p);
            CfgDecoder_p->InsertItem();
            decoder_p = static_cast<CDecoder *>(list_p->GetNext(decoder_p));
        }

        doc_p->AddDecoders(&CfgDecoders_p->m_cfgChildItems);
        doc_p->CleanRowCache();
        CEditorWidget_Repaint();
    }

    if (dll_api_p->GetPlots(&list_p)) {
        CCfgItem_Plots *CfgPlots_p = new CCfgItem_Plots(this);
        CfgPlots_p->InsertItem();

        auto *plot_p = static_cast<CPlot *>(list_p->first());

        while (plot_p != nullptr) {
            CCfgItem_Plot *CfgPlot_p = new CCfgItem_Plot(plot_p, CfgPlots_p);
            CfgPlot_p->InsertItem();

            /* Subplots - start */

            CList_LSZ *subPlotList_p;
            plot_p->GetSubPlots(&subPlotList_p);

            auto *subPlot_p = static_cast<CSubPlot *>(subPlotList_p->first());

            while (subPlot_p != nullptr) {
                CCfgItem_SubPlot *CfgSubPlot_p = new CCfgItem_SubPlot(subPlot_p, CfgPlot_p);
                CfgSubPlot_p->InsertItem();
                subPlot_p = static_cast<CSubPlot *>(subPlotList_p->GetNext(subPlot_p));
            }

            /* Subplots - end */
            plot_p = static_cast<CPlot *>(list_p->GetNext(plot_p));
        }
    }
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Plugin::PrepareDelete(void)
{
    TRACEX_DE("CCfgItem_Plugin::PrepareDelete %s", m_itemText.toLatin1().constData());
    CCfgItem::PrepareDelete(); /* Remove all children */

    typedef void (*QT_LIB_API_DeletePlugin_t)(CPlugin_DLL_API *plugIn_p);                 /* Implemented in
                                                                                           * dll_api_v1.cpp */

    QT_LIB_API_DeletePlugin_t DLL_API_DeletePlugin =
        reinterpret_cast<QT_LIB_API_DeletePlugin_t>(m_library_p->resolve(__DLL_API_DeletePlugin__));

    if (DLL_API_DeletePlugin != nullptr) {
        DLL_API_DeletePlugin(m_dll_api_p);
        m_dll_api_p = nullptr;
    }

    m_library_p->unload();
    delete m_library_p;
    m_library_p = nullptr;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Plugin::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    bool status = true;
    QString fileName;
    QDir workspaceDir = doc_p->GetWorkspaceDirectory();

    if (options & CFG_ITEM_SAVE_OPTION_REL_PATH) {
        fileName = workspaceDir.relativeFilePath(m_path);
    } else {
        fileName = m_path;
    }

    QString pluginPath = QString("    <plugin path=\"%1\" />\n").arg(fileName);
    if (status) {
        status = WriteTagToFile(fileStream, pluginPath);
    }
    return status;
}

/***********************************************************************************************************************
*   GetInfo
***********************************************************************************************************************/
QString CCfgItem_Plugin::GetInfo(void)
{
    QString info;

    if (m_info.supportedFeatures & SUPPORTED_FEATURE_HELP_URL) {
        info =
            QString("  Name: %1\n  Author:  %2\n  Version: %3\n  Help: %4\n").arg(m_info.name).arg(m_info.author).arg(
                m_info.version).arg(m_info.helpURL);
    } else {
        info = QString("  Name: %1\n  Author:  %2\n  Version: %3\n").arg(m_info.name).arg(m_info.author).arg(
            m_info.version);
    }

    info += QString("  Supported features\n");

    if (m_info.supportedFeatures & SUPPORTED_FEATURE_PLOT) {
        info += QString("    Plot\n");
    }

    if (m_info.supportedFeatures & SUPPORTED_FEATURE_DECODER) {
        info += QString("    Decoder\n");
    }

    if (m_info.supportedFeatures & SUPPORTED_FEATURE_HELP_URL) {
        info += QString("    Help URL\n");
    }
    return info;
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Plugin::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_PlugIn) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    {
        /* Info */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Info"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            QString info = GetInfo();
            TRACEX_I(info);
            QMessageBox msgBox(QMessageBox::Information, QString("Pluin information"), info, QMessageBox::Ok);
            msgBox.exec();
        });
    }

    menu_p->addSeparator();

    {
        /* Open containing folder */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open containing folder"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            OpenContainingFolder(&m_path);
        });
    }

    menu_p->addSeparator();

    {
        /* Close */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Close"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CWorkspace_CloseAllSelectedPlugins();

            /* When returning from this call this instance doesn't exist anymore... */
        });
    }

    menu_p->addSeparator();

    {
        /* Web help */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Web help"));
        action_p->setEnabled((m_info.supportedFeatures & SUPPORTED_FEATURE_HELP_URL) ? true : false);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            MW_StartWebPage(QString(m_info.helpURL));
        });
    }

    /* RunAll */
    {
        QAction *action_p;
        action_p = menu_p->addAction(tr("Run All"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CWorkspace_RunAllPlots();
        });
    }

    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_SubPlot - CTOR
***********************************************************************************************************************/
CCfgItem_SubPlot::CCfgItem_SubPlot(
    CSubPlot *subPlot_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p), m_subPlot_ref_p(subPlot_ref_p)
{
    char *subPlotName_p;
    char *y_axis_p;

    subPlot_ref_p->GetTitle(&subPlotName_p, &y_axis_p);

    TRACEX_D("    Subplot: %s", subPlotName_p);

    Set(QString(subPlotName_p), 0, CFG_ITEM_KIND_SubPlot);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_SubPlot::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    bool extraOptions = true;
    CList_LSZ *list_p;

    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_SubPlot) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    if ((m_subPlot_ref_p == nullptr) || !m_subPlot_ref_p->GetGraphs(&list_p) || (list_p->count() == 0)) {
        extraOptions = false;
    }

    QMenu *zoomMenu = new QMenu(QString("Zoom..."));
    QMenu *sizeMenu = new QMenu(QString("Size..."));
    QAction *menuAction = menu_p->addMenu(zoomMenu);
    menuAction->setEnabled(extraOptions);
    menuAction = menu_p->addMenu(sizeMenu);
    menuAction->setEnabled(extraOptions);

    if (!extraOptions) {
        return 0;
    }

    {
        /* Zoom In Horizontal */
        QAction *action_p;
        action_p = zoomMenu->addAction(tr("Zoom In Horizontal\tX"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ZoomSubPlotInFocus(0.0, true, true);
        });
    }

    {
        /* Zoom Out Horizontal */
        QAction *action_p;
        action_p = zoomMenu->addAction(tr("Zoom Out Horizontal\tShift+X"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ZoomSubPlotInFocus(0.0, false, true);
        });
    }

    {
        /* Zoom In Vertical */
        QAction *action_p;
        action_p = zoomMenu->addAction(tr("Zoom In Vertical\tY"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ZoomSubPlotInFocus(0.0, true, false);
        });
    }

    {
        /* Zoom Out Vertical */
        QAction *action_p;
        action_p = zoomMenu->addAction(tr("Zoom Out Vertical\tShift+Y"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ZoomSubPlotInFocus(0.0, false, false);
        });
    }

    {
        /* Zoom restore */
        QAction *action_p;
        action_p = zoomMenu->addAction(tr("Zoom restore"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_Align_Reset_Zoom();
        });
    }

    {
        /* Size increase */
        QAction *action_p;
        action_p = sizeMenu->addAction(tr("Increase\tS"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ResizeSubPlotInFocus(0.0, true);
        });
    }

    {
        /* Size decrease */
        QAction *action_p;
        action_p = sizeMenu->addAction(tr("Decrease\tShift+S"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_ResizeSubPlotInFocus(0.0, false);
        });
    }

    {
        /* Size restore */
        QAction *action_p;
        action_p = sizeMenu->addAction(tr("Restore"));
        action_p->setEnabled(extraOptions);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlotPane_SetSubPlotFocus(m_subPlot_ref_p);
            CPlotPane_RestoreSubPlotSizes();
        });
    }
    return 0;
}

CCfgItem_Graph::CCfgItem_Graph(
    CGraph *graph_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    char *graphName_p;

    m_graph_ref_p = graph_ref_p;
    graphName_p = m_graph_ref_p->GetName();

    Set(QString(graphName_p), 0, CFG_ITEM_KIND_Graph);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Graph::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Graph) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    CCfgItem_Graph *first_p = static_cast<CCfgItem_Graph *>(selectionList_p->first());

    {
        /* Enable/Disable */
        QAction *action_p;
        bool doEnable = true;

        if (first_p->m_graph_ref_p->isEnabled()) {
            action_p = menu_p->addAction(tr("Disable"));
            doEnable = false;
        } else {
            action_p = menu_p->addAction(tr("Enable"));
        }
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlot *plotRef_p = nullptr;
            for (auto& item_p : *selectionList_p) {
                CCfgItem_Graph *graphItem_p = static_cast<CCfgItem_Graph *>(item_p);
                graphItem_p->m_graph_ref_p->SetEnableFlag(doEnable);
                if (plotRef_p == nullptr) {
                    plotRef_p = static_cast<CCfgItem_Plot *>(graphItem_p->m_itemParent_p->m_itemParent_p)->m_plot_ref_p;
                }
            }
            if (plotRef_p != nullptr) {
                MW_RedrawPlot(plotRef_p);
            }
        });
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_SequenceDiagram - CTOR
***********************************************************************************************************************/
CCfgItem_SequenceDiagram::CCfgItem_SequenceDiagram(
    CSequenceDiagram *sequenceDiagram_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    char *name_p;

    m_sequenceDiagram_ref_p = sequenceDiagram_ref_p;
    name_p = sequenceDiagram_ref_p->GetName();

    Set(QString(name_p), 0, CFG_ITEM_KIND_SequenceDiagram);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_SequenceDiagram::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Graph) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    CCfgItem_Graph *first_p = static_cast<CCfgItem_Graph *>(selectionList_p->first());

    {
        /* Enable/Disable */
        QAction *action_p;
        bool doEnable = true;

        if (first_p->m_graph_ref_p->isEnabled()) {
            action_p = menu_p->addAction(tr("Disable"));
            doEnable = false;
        } else {
            action_p = menu_p->addAction(tr("Enable"));
        }
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CPlot *plotRef_p = nullptr;
            for (auto& item_p : *selectionList_p) {
                CCfgItem_Graph *graphItem_p = static_cast<CCfgItem_Graph *>(item_p);
                graphItem_p->m_graph_ref_p->SetEnableFlag(doEnable);
                MW_SetSubPlotUpdated(static_cast<CCfgItem_SubPlot *>(graphItem_p->m_itemParent_p)->m_subPlot_ref_p);
                if (plotRef_p == nullptr) {
                    plotRef_p = static_cast<CCfgItem_Plot *>(graphItem_p->m_itemParent_p->m_itemParent_p)->m_plot_ref_p;
                }
            }
            if (plotRef_p != nullptr) {
                MW_RedrawPlot(plotRef_p);
            }
        });
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Decoders - CTOR
***********************************************************************************************************************/
CCfgItem_Decoders::CCfgItem_Decoders(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Decoders"), 0, CFG_ITEM_KIND_DecoderRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   CCfgItem_Decoder - CTOR
***********************************************************************************************************************/
CCfgItem_Decoder::CCfgItem_Decoder(
    CDecoder *decoder_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    int length = 0;
    char *match_p = decoder_ref_p->GetMatchString(&length);

    m_enabled = true;
    m_decoder_ref_p = decoder_ref_p;

    Set(QString(match_p), 0, CFG_ITEM_KIND_Decoder);
    TRACEX_D("  Decoder: %s", match_p);
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Decoder::PrepareDelete(void)
{
    TRACEX_DE("CCfgItem_Decoder::PrepareDelete %s", m_itemText.toLatin1().constData());
    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        /* check this flag, since doing BookmarkDecorateFIRA takes a lot of time */
        CLogScrutinizerDoc *doc_p = GetTheDoc();
        doc_p->RemoveDecoder(this);
    }
    CCfgItem::PrepareDelete();
}

/***********************************************************************************************************************
*   CCfgItem_Logs - CTOR
***********************************************************************************************************************/
CCfgItem_Logs::CCfgItem_Logs(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Logs"), 0, CFG_ITEM_KIND_LogRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Logs::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    bool status = true;

    if (m_cfgChildItems.isEmpty()) {
        return true;
    }

    if (status) {
        status = WriteTagToFile(fileStream, QString("  <logs>\n"));
    }
    if (status) {
        status = WriteChildrensToFile(fileStream, options);
    }
    if (status) {
        status = WriteTagToFile(fileStream, QString("  </logs>\n"));
    }

    return status;
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Logs::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_LogRoot) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    {
        /* Open Log File */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open Log File..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            CFGCTRL_LoadLogFile();
        });
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Log - CTOR
***********************************************************************************************************************/
CCfgItem_Log::CCfgItem_Log(
    char *path_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p), m_path(path_p)
{
    auto doc_p = GetTheDoc();
    QString label = QString("%1  %2").arg(QString(path_p))
                        .arg(doc_p->FileSizeToString(doc_p->m_database.fileSize));
    Set(label, 0, CFG_ITEM_KIND_Log);
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Log::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    QString filePath;

    if (options & CFG_ITEM_SAVE_OPTION_REL_PATH) {
        QDir workspaceDir = GetTheDoc()->GetWorkspaceDirectory();
        filePath = workspaceDir.relativeFilePath(m_path);
    } else {
        filePath = m_path;
    }

    QString fileTag = QString("    <log path=\"%1\" />\n").arg(filePath);

    return WriteTagToFile(fileStream, fileTag);
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Log::PrepareDelete(void)
{
    TRACEX_DE("%s", __FUNCTION__);
    if (CSCZ_SystemState != SYSTEM_STATE_SHUTDOWN) {
        /* check this flag, since doing BookmarkDecorateFIRA takes a lot of time */
        GetTheDoc()->CleanDB(false);
    }
    CCfgItem::PrepareDelete();
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Log::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_Log) {
        TRACEX_W("%s Internal error, selection not a log item", __FUNCTION__);
        return 0;
    }

    CCfgItem_Log *log_p = static_cast<CCfgItem_Log *>(selectionList_p->first());

    {
        /* Close */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Close"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CCfgItem_Delete(log_p);});
    }

    {
        /* Open Containing folder */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open containing folder"));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {OpenContainingFolder(&m_path);});
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_Filters - CTOR
***********************************************************************************************************************/
CCfgItem_Filters::CCfgItem_Filters(
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    Set(QString("Filters"), 0, CFG_ITEM_KIND_FilterRoot);
    m_always_exist = true;
}

/***********************************************************************************************************************
*   AppendHashWorkspaceToChildrens
***********************************************************************************************************************/
void CCfgItem_Filters::AppendHashWorkspaceToChildrens()
{
    for (auto& cfgItem : m_cfgChildItems) {
        CFilter *filter_p = static_cast<CCfgItem_Filter *>(cfgItem)->m_filter_ref_p;

        if (!filter_p->m_fileName_short.contains("#workspace")) {
            filter_p->m_fileName_short.append(" #workspace");
        }
    }
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Filters::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    /* This top container for filters are called when the workspace is saved. In such case the "ownership" of the
     * filters are put to the workspace.
     * Hence the name is updated with #workspace */

    if (m_cfgChildItems.isEmpty()) {
        return true;
    }

    if (options & CFG_ITEM_SAVE_OPTION_TO_DEFAULT_WORKSPACE) {
        return WriteChildrensToFile(fileStream, options);
    }

    /* Update the filter in the CCfgItem_Filter with a new file name, adding #workspace */
    AppendHashWorkspaceToChildrens();
    return WriteChildrensToFile(fileStream, options);
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Filters::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_FilterRoot) {
        TRACEX_W("%s Internal error, selection not a filter root", __FUNCTION__);
        return 0;
    }

    {
        /* Create */
        QAction *action_p;
        action_p = menu_p->addAction(tr("New Filter File..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CWorkspace_CreateCfgFilter(nullptr);});
    }

    {
        /* Open filter file */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Open Filter File..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {CFGCTRL_LoadFilterFile();});
    }

    {
        /* Add filter item */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Add filter item to Filter File..."));
        action_p->setEnabled(true);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            auto filterItem = CWorkspace_AddFilterItem(nullptr, nullptr, nullptr, treeview_p);
            if (filterItem != nullptr) {
                CWorkspace_TreeView_Select(filterItem);
            }
        });
    }
    return 0;
}

/***********************************************************************************************************************
*   CCfgItem_FilterItem - CTOR
***********************************************************************************************************************/
CCfgItem_FilterItem::CCfgItem_FilterItem(
    CFilter *parentFilter_ref_p,
    CFilterItem *filterItem_ref_p,
    CCfgItem *itemParent_p) :
    CCfgItem(itemParent_p)
{
    m_filterParent_ref_p = parentFilter_ref_p;
    m_filterItem_ref_p = filterItem_ref_p;
    m_quickSearchNum = -1;

    m_filterItem_ref_p = filterItem_ref_p;
    if (m_filterItem_ref_p != nullptr) {
        Set(QString(m_filterItem_ref_p->m_start_p), 0, CFG_ITEM_KIND_FilterItem);
    } else {
        Set(QString(""), 0, CFG_ITEM_KIND_FilterItem);
    }
}

CCfgItem_FilterItem::~CCfgItem_FilterItem(void)
{
    delete m_filterItem_ref_p;
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_FilterItem::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    if (selectionList_p->count() <= 0) {
        TRACEX_W("%s No selections %d", __FUNCTION__, selectionList_p->count());
        return 0;
    }

    if (selectionList_p->first()->m_itemKind != CFG_ITEM_KIND_FilterItem) {
        TRACEX_W("%s Internal error, selection not a filter item", __FUNCTION__);
        return 0;
    }

    bool sameKind = AllSelectionsOfSameKind(selectionList_p);
    CCfgItem_FilterItem *filterItem_p = static_cast<CCfgItem_FilterItem *>(selectionList_p->first());

    {
        /* Enable/disable */
        QAction *action_p;
        bool enabled = filterItem_p->m_filterItem_ref_p->m_enabled;

        if (enabled) {
            action_p = menu_p->addAction(tr("Disable"));
        } else {
            action_p = menu_p->addAction(tr("Enable"));
        }

        action_p->setEnabled(sameKind ? true : false);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            for (auto& item_p : *selectionList_p) {
                if (item_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
                    static_cast<CCfgItem_FilterItem *>(item_p)->m_filterItem_ref_p->m_enabled = enabled ? false : true;
                    CWorkspace_ItemUpdated(item_p);
                }
            }
        });
    }

    {
        /* Properties */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Properties"));
        if (selectionList_p->count() == 1) {
            action_p->setEnabled(true);
        } else {
            action_p->setEnabled(false);
        }
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            filterItem_p->PropertiesDlg(treeview_p);
            CWorkspace_ItemUpdated(filterItem_p);
        });
    }

    {
        /* Delete */
        QAction *action_p;
        action_p = menu_p->addAction(tr("Delete"));
        action_p->setEnabled(sameKind ? true : false);
        treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
            for (auto& item_p : *selectionList_p) {
                if (item_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
                    CCfgItem_Delete(item_p);
                }
            }
        });
    }

    return 0;
}

/***********************************************************************************************************************
*   OnDblClick
***********************************************************************************************************************/
void CCfgItem_FilterItem::OnDblClick(QWidget *parent)
{
    /*m_filterItem_ref_p->m_enabled = m_filterItem_ref_p->m_enabled == true ? false : true; */
    PropertiesDlg(parent);
    CCfgItem::OnDblClick(parent);
    CWorkspace_ItemUpdated(this);
}

/***********************************************************************************************************************
*   PropertiesDlg
***********************************************************************************************************************/
void CCfgItem_FilterItem::PropertiesDlg(QWidget *widget_p)
{
    if ((m_filterItem_ref_p == nullptr) ||
        (m_filterItem_ref_p->m_start_p == nullptr)) {
        /* Code analysis,  C6387 */
        TRACEX_E(
            "%s   Bad initialization     m_filterItem_ref_p:0x%x", __FUNCTION__, m_filterItem_ref_p);
        return;
    }

    CWorkspace_AddFilterItem(m_filterItem_ref_p->m_start_p, this, static_cast<CCfgItem_Filter *>(m_itemParent_p),
                             widget_p);
    CEditorWidget_Repaint();
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_FilterItem::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    Q_UNUSED(options);

    int destIndex = 0;
    for (int srcIndex = 0; srcIndex < m_filterItem_ref_p->m_size; ++srcIndex) {
        if (m_filterItem_ref_p->m_start_p[srcIndex] == '"') {
            g_cfgItem_tempString[destIndex++] = '\\';
        }

        g_cfgItem_tempString[destIndex++] = m_filterItem_ref_p->m_start_p[srcIndex];
    }

    g_cfgItem_tempString[destIndex] = 0;

    QString tag;
    tag.sprintf(
        "    <filter enabled=\"%c\" excluding=\"%c\" color=\"%.4x\"  bg_color=\"%.4x\" type=\"matches_text\" case_sensitive=\"%c\" regex=\"%c\" adaptive_clip=\"%c\" text=\"%s\" />\n",
        m_filterItem_ref_p->m_enabled == true ? 'y' : 'n',
        m_filterItem_ref_p->m_exclude == true ? 'y' : 'n',
        m_filterItem_ref_p->m_color,
        m_filterItem_ref_p->m_bg_color,
        m_filterItem_ref_p->m_caseSensitive == true ? 'y' : 'n',
        m_filterItem_ref_p->m_regexpr == true ? 'y' : 'n',
        m_filterItem_ref_p->m_adaptiveClipEnabled == true ? 'y' : 'n',
        g_cfgItem_tempString);

    return WriteTagToFile(fileStream, tag);
}

/***********************************************************************************************************************
*   hash
***********************************************************************************************************************/
void CCfgItem_FilterItem::hash(QDataStream& dstream) const
{
    dstream << m_filterItem_ref_p->m_enabled;
    dstream << m_filterItem_ref_p->m_exclude;
    dstream << m_filterItem_ref_p->m_color;
    dstream << m_filterItem_ref_p->m_bg_color;
    dstream << m_filterItem_ref_p->m_caseSensitive;
    dstream << m_filterItem_ref_p->m_regexpr;
    dstream << m_filterItem_ref_p->m_adaptiveClipEnabled;
    dstream << m_filterItem_ref_p->m_caseSensitive;
    dstream << m_filterItem_ref_p->m_size;
    dstream.writeRawData(m_filterItem_ref_p->m_start_p, m_filterItem_ref_p->m_size);
}

/***********************************************************************************************************************
*   Serialize
***********************************************************************************************************************/
void CCfgItem_FilterItem::Serialize(QDataStream& dstream, bool pack)
{
    if (pack) {
        CCfgItem::Serialize(dstream, pack);
        dstream << (m_itemKind | MIME_CFG_ITEM_KIND_TAG); /* This it put twice in the stream */
        dstream << m_filterItem_ref_p->m_enabled;
        dstream << m_filterItem_ref_p->m_exclude;
        dstream << m_filterItem_ref_p->m_color;
        dstream << m_filterItem_ref_p->m_bg_color;
        dstream << m_filterItem_ref_p->m_caseSensitive;
        dstream << m_filterItem_ref_p->m_regexpr;
        dstream << m_filterItem_ref_p->m_adaptiveClipEnabled;
        dstream << m_filterItem_ref_p->m_caseSensitive;
        dstream << m_filterItem_ref_p->m_size;
        dstream.writeRawData(m_filterItem_ref_p->m_start_p, m_filterItem_ref_p->m_size);
    } else {
        m_filterItem_ref_p = new CFilterItem();

        int temp_kind;
        dstream >> temp_kind;

        /* This is a sort of stamp to ensure correct data is being loaded */
        Q_ASSERT(temp_kind == (static_cast<int>(CFG_ITEM_KIND_FilterItem) | MIME_CFG_ITEM_KIND_TAG));

        m_itemKind = static_cast<CfgItemKind_t>(temp_kind & MIME_CFG_ITEM_KIND_BITS);
        dstream >> m_filterItem_ref_p->m_enabled;
        dstream >> m_filterItem_ref_p->m_exclude;
        dstream >> m_filterItem_ref_p->m_color;
        dstream >> m_filterItem_ref_p->m_bg_color;
        dstream >> m_filterItem_ref_p->m_caseSensitive;
        dstream >> m_filterItem_ref_p->m_regexpr;
        dstream >> m_filterItem_ref_p->m_adaptiveClipEnabled;
        dstream >> m_filterItem_ref_p->m_caseSensitive;
        dstream >> m_filterItem_ref_p->m_size;

        int temp = m_filterItem_ref_p->m_size;
        m_filterItem_ref_p->m_start_p = static_cast<char *>(malloc(size_t(temp) + 1));
        if (dstream.readRawData(m_filterItem_ref_p->m_start_p, temp) != temp) {
            Q_ASSERT(false);
        }
        m_filterItem_ref_p->m_start_p[m_filterItem_ref_p->m_size] = 0;
        m_filterItem_ref_p->m_font_p = GetTheDoc()->m_fontCtrl.RegisterFont(m_filterItem_ref_p->m_color,
                                                                            m_filterItem_ref_p->m_bg_color);
        Set(QString(m_filterItem_ref_p->m_start_p), 0, CFG_ITEM_KIND_FilterItem);
    }
}

/***********************************************************************************************************************
*   UpdateName
***********************************************************************************************************************/
void CCfgItem_Filter::UpdateName(void)
{
    UpdateTreeName(*m_filter_ref_p->GetShortFileName());
}

/***********************************************************************************************************************
*   PrepareDelete
***********************************************************************************************************************/
void CCfgItem_Filter::PrepareDelete(void)
{
    delete m_filter_ref_p;
    CCfgItem::PrepareDelete();
}

/***********************************************************************************************************************
*   OnPopupMenu
***********************************************************************************************************************/
int CCfgItem_Filter::OnPopupMenu(QList<CCfgItem *> *selectionList_p, QTreeView *treeview_p, QMenu *menu_p)
{
    CCfgItem_Filter *cfgItem_p = static_cast<CCfgItem_Filter *>(selectionList_p->first());
    if (selectionList_p->count() <= 0) {
        TRACEX_W("CCfgItem_Filter::OnPopupMenu No selections %d", selectionList_p->count());
        return 0;
    }

    if (cfgItem_p->m_itemKind == CFG_ITEM_KIND_Filter) {
        const bool reloadPossible = isReloadPossible(nullptr);

        {
            /* Add filter item */
            QAction *action_p = menu_p->addAction(tr("Add filter item..."));
            if (selectionList_p->count() == 1) {
                treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
                    CWorkspace_AddFilterItem(nullptr, nullptr, this, treeview_p);
                });
            } else {
                action_p->setEnabled(false);
            }
        }

        {
            /* Enable all */
            QAction *action_p;
            action_p = menu_p->addAction(tr("Enable All"));
            action_p->setEnabled(true);
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {cfgItem_p->Enable(true, selectionList_p);});
        }

        {
            /* Disable all */
            QAction *action_p;
            action_p = menu_p->addAction(tr("Disable All"));
            action_p->setEnabled(true);
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {cfgItem_p->Enable(false, selectionList_p);});
            action_p->setEnabled(selectionList_p->count() == 1 ? true : false);
        }

        menu_p->addSeparator();

        {
            /* Reload */
            QAction *action_p = menu_p->addAction(tr("Reload"));
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {Reload(selectionList_p);});
            action_p->setEnabled(selectionList_p->count() == 1 && reloadPossible ? true : false);
        }

        {
            /* Save */
            QAction *action_p = menu_p->addAction(tr("Save"));
            if (cfgItem_p->m_filter_ref_p->m_fileName.indexOf(tr("default_filter")) >= 0) {
                action_p->setEnabled(false);
            } else {
                action_p->setEnabled(selectionList_p->count() == 1 ? true : false);
                treeview_p->connect(action_p, &QAction::triggered, [ = ] () {cfgItem_p->Save(selectionList_p);});
            }
        }

        {
            /* SaveAs */
            QAction *action_p = menu_p->addAction(tr("Save As..."));
            action_p->setEnabled(selectionList_p->count() == 1 ? true : false);
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {cfgItem_p->SaveAs();});
        }

        {
            /* Open containing folder */
            QAction *action_p = menu_p->addAction(tr("Open containing folder..."));
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
                OpenContainingFolder(&m_filter_ref_p->m_fileName);
            });
            action_p->setEnabled(selectionList_p->count() == 1 && reloadPossible ? true : false);
        }

        {
            /* Delete */
            QAction *action_p;
            action_p = menu_p->addAction(tr("Delete"));
            action_p->setEnabled(true);
            treeview_p->connect(action_p, &QAction::triggered, [ = ] () {
                for (auto& item_p : *selectionList_p) {
                    if (item_p->m_itemKind == CFG_ITEM_KIND_Filter) {
                        CCfgItem_Delete(item_p);
                    }
                }
            });
        }
    } else {
        TRACEX_W(QString("%1 Selection not a filter").arg(__FUNCTION__));
    }

    return 0;
}

/***********************************************************************************************************************
*   SaveAs
***********************************************************************************************************************/
void CCfgItem_Filter::SaveAs(void)
{
    CFGCTRL_SaveFilterFileAs(m_filter_ref_p->GetFileName(), this);
}

/***********************************************************************************************************************
*   Save
***********************************************************************************************************************/
void CCfgItem_Filter::Save(QList<CCfgItem *> *selectionList_p)
{
    /* Save the filter(s) files if they are not part of the workspace. Otherwise the save should be part of the
     * workspace save */

    if (selectionList_p == nullptr) {
        CFGCTRL_SaveFilterFile(m_filter_ref_p->GetFileNameRef(), this);
    } else {
        QList<CCfgItem *>::iterator iter = selectionList_p->begin();
        for ( ; iter != selectionList_p->end(); ++iter) {
            CCfgItem_Filter *cfgFilter_p = static_cast<CCfgItem_Filter *>(*iter);

            if ((cfgFilter_p->m_itemKind == CFG_ITEM_KIND_Filter) && cfgFilter_p->isFileExisting()) {
                CFGCTRL_SaveFilterFile(cfgFilter_p->m_filter_ref_p->GetFileNameRef(), this);
            }
        }
    }
}

/***********************************************************************************************************************
*   isReloadPossible
***********************************************************************************************************************/
bool CCfgItem_Filter::isReloadPossible(QList<CCfgItem *> *selectionList_p)
{
    if ((selectionList_p != nullptr) && !selectionList_p->isEmpty()) {
        bool reloadPossible = true;
        QList<CCfgItem *>::iterator iter = selectionList_p->begin();
        for ( ; iter != selectionList_p->end(); ++iter) {
            CCfgItem_Filter *cfgFilter_p = static_cast<CCfgItem_Filter *>(*iter);
            if ((cfgFilter_p->m_itemKind != CFG_ITEM_KIND_Filter) || !cfgFilter_p->isFileExisting()) {
                reloadPossible = false;
            }
        }
        return reloadPossible;
    } else {
        return isFileExisting();
    }
}

/***********************************************************************************************************************
*   Reload
***********************************************************************************************************************/
void CCfgItem_Filter::Reload(QList<CCfgItem *> *selectionList_p)
{
    /* At reload filters may be replaced */

    if ((selectionList_p == nullptr) || (selectionList_p->count() <= 1)) {
        RemoveAllChildren();
        CFGCTRL_ReloadFilterFile(m_filter_ref_p->GetFileNameRef(), this);
    } else {
        /* Selection list will be emptied when reloading filter, copy its context first */
        QList<CCfgItem *> selectionList_temp;

        while (!selectionList_p->empty()) {
            selectionList_temp.append(selectionList_p->takeFirst());
        }

        for (auto& cfgItem : selectionList_temp) {
            CCfgItem_Filter *cfgFilter_p = static_cast<CCfgItem_Filter *>(cfgItem);
            if ((cfgFilter_p->m_itemKind == CFG_ITEM_KIND_Filter) && cfgFilter_p->isFileExisting()) {
                cfgFilter_p->RemoveAllChildren();
                CFGCTRL_ReloadFilterFile(cfgFilter_p->m_filter_ref_p->GetFileNameRef(), cfgFilter_p);
            }
        }
    }
}

/***********************************************************************************************************************
*   Enable
***********************************************************************************************************************/
void CCfgItem_Filter::Enable(bool enable, QList<CCfgItem *> *selectionList_p)
{
    for (auto& cfgItem_p : *selectionList_p) {
        if (cfgItem_p->m_itemKind == CFG_ITEM_KIND_Filter) {
            CCfgItem_Filter *filter_p = static_cast<CCfgItem_Filter *>(cfgItem_p);
            filter_p->Enable(enable, &filter_p->m_cfgChildItems); /*recursive */
            return;
        } else if (cfgItem_p->m_itemKind == CFG_ITEM_KIND_FilterItem) {
            static_cast<CCfgItem_FilterItem *>(cfgItem_p)->m_filterItem_ref_p->m_enabled = enable;
        }
    }
}

/***********************************************************************************************************************
*   isFileExisting
***********************************************************************************************************************/
bool CCfgItem_Filter::isFileExisting(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString absoluteFileName;

    doc_p->GetAbsoluteFileName(m_filter_ref_p->m_fileName, absoluteFileName);

    QFileInfo fileInfo(absoluteFileName);

    return (fileInfo.exists());
}

/***********************************************************************************************************************
*   InsertFilter
***********************************************************************************************************************/
void CCfgItem_Filter::InsertFilter(bool isReinsert)
{
    /* Insert filterItems from the CFilterItems in the CFilter list (m_filterItemList). It feels a bit stupid having
     * double lists representing the same thing, that is one representation for the CfgItem, and then the CFilter
     * representation with a reference to the CFilter and the CFilterItems... */
    if (!isReinsert) {
        CCfgItem::InsertItem();
    }

    UpdateTreeName(m_filter_ref_p->m_fileName_short);

    if (m_filter_ref_p->m_filterItemList.isEmpty()) {
        return;
    }

    CCfgItem_FilterItem *newItem_p;

    for (auto filterItem_p : m_filter_ref_p->m_filterItemList) {
        newItem_p = new CCfgItem_FilterItem(m_filter_ref_p, filterItem_p, this);
        newItem_p->InsertItem();
    }

    if (!m_filter_ref_p->m_filterItemList.isEmpty()) {
        /* Empty the CFilter from CFilterItems, these shall only be referenced from the CCfgItem_FilterItem class.
         * Otherwise there will be double references (and problem of who should remove the CFilterItem
         * when destructors are called) */
        m_filter_ref_p->m_filterItemList.clear();
    }
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CCfgItem_Filter::WriteToFile(QTextStream& fileStream, CfgItemSaveOptions_t options)
{
    bool status;
    QString path = m_filter_ref_p->GetFileNameRef();
    QString fileName;
    QDir workspaceDir;

    workspaceDir = GetTheDoc()->GetWorkspaceDirectory();

    if (options & CFG_ITEM_SAVE_OPTION_REL_PATH) {
        fileName = workspaceDir.relativeFilePath(path);
    } else {
        fileName = path;
    }

    status = WriteTagToFile(fileStream, QString("  <filters name=\"%1\">\n").arg(fileName));

    if (status) {
        status = WriteChildrensToFile(fileStream, options);
    }

    if (status) {
        status = WriteTagToFile(fileStream, QString("  </filters>\n"));
    }

    return status;
}
