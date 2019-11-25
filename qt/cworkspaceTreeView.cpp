/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "cworkspaceTreeView.h"
#include "CCfgItem.h"
#include "CWorkspace.h"
#include "ceditorwidget_cb_if.h"
#include "mainwindow_cb_if.h"
#include "CWorkspace_cb_if.h"

#include <QMenu>
#include <QApplication>
#include <QSettings>
#include <algorithm>

static CWorkspaceTreeView *g_treeView = nullptr;

/***********************************************************************************************************************
*   CWorkspace_TreeView_GetSelections
***********************************************************************************************************************/
bool CWorkspace_TreeView_GetSelections(CfgItemKind_t kind, QList<CCfgItem *>& selectionList)
{
    return g_treeView->GetSelections(kind, selectionList);
}

/***********************************************************************************************************************
*   CWorkspace_TreeView_GetSelectionIndexes
***********************************************************************************************************************/
bool CWorkspace_TreeView_GetSelectionIndexes(QModelIndexList& selectionList)
{
    return g_treeView->GetSelectionIndexes(selectionList);
}

/***********************************************************************************************************************
*   CWorkspace_TreeView_UnselectAll
***********************************************************************************************************************/
void CWorkspace_TreeView_UnselectAll(void)
{
    g_treeView->UnselectAll();
}

/***********************************************************************************************************************
*   CWorkspace_TreeView_Unselect
***********************************************************************************************************************/
void CWorkspace_TreeView_Unselect(CCfgItem *cfgItem_p)
{
    g_treeView->Unselect(cfgItem_p);
}

/***********************************************************************************************************************
*   CWorkspace_TreeView_Select
***********************************************************************************************************************/
void CWorkspace_TreeView_Select(CCfgItem *cfgItem_p)
{
    g_treeView->UnselectAll();
    g_treeView->Select(cfgItem_p);
}

/***********************************************************************************************************************
*   CWorkspaceTreeView
***********************************************************************************************************************/
CWorkspaceTreeView::CWorkspaceTreeView(QWidget *parent) :
    QTreeView(parent)
{
    g_treeView = this;
    setExpandsOnDoubleClick(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction); /* Prefer moving elements instead of copying */
}

CWorkspaceTreeView::~CWorkspaceTreeView()
{}

/***********************************************************************************************************************
*   selectionChanged
***********************************************************************************************************************/
void CWorkspaceTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QList<CCfgItem *> selectionList;

    if (GetSelections(CFG_ITEM_KIND_FilterItem, selectionList)) {
        auto filterItem = dynamic_cast<CCfgItem_FilterItem *>(selectionList.first());
        MW_UpdateSearchParameters(filterItem->m_itemText,
                                  filterItem->m_filterItem_ref_p->m_caseSensitive,
                                  filterItem->m_filterItem_ref_p->m_regexpr);
    }
    PRINT_SELECTION(QString("%1 selected:%2 deselected:%3")
                        .arg(__FUNCTION__).arg(selected.indexes().count()).arg(deselected.indexes().count()))
    for (auto& modelIndex : selected.indexes()) {
        CCfgItem *cfgItem_p = static_cast<CCfgItem *>(modelIndex.internalPointer());
        PRINT_SELECTION(QString("  %1").arg(cfgItem_p->m_itemText))
    }

    QTreeView::selectionChanged(selected, deselected);
}

/***********************************************************************************************************************
*   mouseDoubleClickEvent
***********************************************************************************************************************/
void CWorkspaceTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QList<CCfgItem *> selectionList;
    if (GetSelections(CFG_ITEM_KIND_None, selectionList)) {
        TRACEX_DE(QString("CWorkspaceTreeView::mouseDoubleClickEvent"))

        CCfgItem *cfgItem_p = selectionList.last();
        cfgItem_p->OnDblClick(this);
    }
    QTreeView::mouseDoubleClickEvent(event);
}

/***********************************************************************************************************************
*   storeSettings
***********************************************************************************************************************/
void CWorkspaceTreeView::storeSettings(void)
{
#ifdef LOCAL_GEOMETRY_SETTING
    QSettings settings;
    settings.setValue("workspaceWindow", size());
#endif
}

/***********************************************************************************************************************
*   sizeHint
***********************************************************************************************************************/
QSize CWorkspaceTreeView::sizeHint() const
{
    static QSize windowSize;
    auto refreshEditorWindow = makeMyScopeGuard([&] () {
        PRINT_SIZE(QString("Workspace sizeHint %1,%2 policy:%3")
                       .arg(windowSize.width()).arg(windowSize.height()).arg(sizeAdjustPolicy()))
    });

    windowSize = m_lastResize; /*QSize(); */

    (void)QTreeView::sizeHint();

    if (CSCZ_AdaptWindowSizes) {
        windowSize = m_adaptWindowSize;
        PRINT_SIZE(QString("Workspace adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
    }

    return windowSize;
}

/***********************************************************************************************************************
*   resizeEvent
***********************************************************************************************************************/
void CWorkspaceTreeView::resizeEvent(QResizeEvent *event)
{
    PRINT_SIZE(QString("Workspace resize %1,%2 -> %3,%4  state:%5")
                   .arg(event->oldSize().width()).arg(event->oldSize().height())
                   .arg(event->size().width()).arg(event->size().height())
                   .arg(windowState()))
    MW_updatePendingStateGeometry();

    /* This prevents continous sliding of workspace. It apread as Qt was resizing the windows until editor
     * and workspace got same size, this occured when editor was small. This fix below stops the continous resizing
     * by the layout manager
     * UGLY-FIX-START */
    int diff = m_lastResize.width() - event->size().width();
    if (diff < 0) {
        diff *= -1;
    }
    if (diff > 5) {
        m_lastResize = event->size();
    }

    /* UGLY-FIX-END */

    CWorkspace_LayoutChanged();
    updateGeometry(); /* force sizeHint() */
    QTreeView::resizeEvent(event);
}

/***********************************************************************************************************************
*   wheelEvent
***********************************************************************************************************************/
void CWorkspaceTreeView::wheelEvent(QWheelEvent *event)
{
    auto CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    if (CTRL_Pressed) {
        int zDelta = 0;
        QPoint numPixels = event->pixelDelta();
        QPoint numDegrees = event->angleDelta() / 8;

        if (!numDegrees.isNull()) {
            zDelta = numDegrees.y();
        } else if (!numPixels.isNull()) {
            zDelta = numPixels.y();
        }

        if (zDelta < 0) {
            resizeText(false);
        } else {
            resizeText(true);
        }
    } else {
        QTreeView::wheelEvent(event);
    }
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CWorkspaceTreeView::keyPressEvent(QKeyEvent *event)
{
    QList<CCfgItem *> selectionList;
    GetSelections(CFG_ITEM_KIND_None, selectionList);

    int index = event->key();
    bool ctrl = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    bool alt = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    PRINT_KEYPRESS(QString("%1 key:%2 ctrl:%3 shift:%4 alt:%5").arg(__FUNCTION__).arg(index).arg(ctrl).arg(shift).arg(
                       alt))

    switch (index)
    {
        case Qt::Key_Delete:
            if (!selectionList.isEmpty()) {
                for (auto& cfgItem_p : selectionList) {
                    if (!cfgItem_p->m_always_exist) {
                        CCfgItem_Delete(cfgItem_p);
                        event->accept();
                        return;
                    }
                }
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!selectionList.isEmpty()) {
                CCfgItem *cfgItem_p = selectionList.last();
                cfgItem_p->OnDblClick(this);
                event->accept();
                return;
            }
            break;

        case Qt::Key_Plus:
            if (ctrl) {
                resizeText(true);
                event->accept();
                return;
            }
            break;

        case Qt::Key_Minus:
            if (ctrl) {
                resizeText(false);
                event->accept();
                return;
            }
            break;

        case Qt::Key_F:
            if (ctrl) {
                if (GetSelections(CFG_ITEM_KIND_FilterItem, selectionList)) {
                    auto filterItem = dynamic_cast<CCfgItem_FilterItem *>(selectionList.first());
                    MW_ActivateSearch(filterItem->m_itemText,
                                      filterItem->m_filterItem_ref_p->m_caseSensitive,
                                      filterItem->m_filterItem_ref_p->m_regexpr);
                    event->accept();
                    return;
                }
            }
            break;

        default:
            if (ctrl) {
                /* Check if 0-9, or a-z was pressed */
                if (((index >= '0') && (index <= '9'))) {
                    g_workspace_p->SetQuickSearch(index);
                    event->accept();
                    return;
                }
            }
            break;
    }

    QTreeView::keyPressEvent(event);
}

/***********************************************************************************************************************
*   resizeText
***********************************************************************************************************************/
void CWorkspaceTreeView::resizeText(bool increase)
{
    auto fontSize = g_cfg_p->m_workspace_FontSize;
    if (increase) {
        g_cfg_p->m_workspace_FontSize = fontSize > 31 ? 32 : fontSize + 1;
    } else {
        g_cfg_p->m_workspace_FontSize = fontSize < 7 ? 6 : fontSize - 1;
    }

    TRACEX_I("Workspace font size %d", g_cfg_p->m_workspace_FontSize)
    collapseAll();
    expandAll();
}

/***********************************************************************************************************************
*   contextMenuEvent
***********************************************************************************************************************/
void CWorkspaceTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        QMenu contextMenu(this);
        CCfgItem *cfgItem_p = static_cast<CCfgItem *>(index.internalPointer());
        QList<CCfgItem *> list;
        if (GetSelections(CFG_ITEM_KIND_None, list)) {
            TRACEX_I(QString("%1 first_kind:%2").arg(__FUNCTION__).arg(list.first()->m_itemKind))
            cfgItem_p->OnPopupMenu(&list, this, &contextMenu);
        }
        contextMenu.exec(mapToGlobal(event->pos()));
    }
}

/***********************************************************************************************************************
*   GetSelectionIndexes
***********************************************************************************************************************/
bool CWorkspaceTreeView::GetSelectionIndexes(QModelIndexList& selectionList)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_treeView != nullptr);
#endif
    if (g_treeView == nullptr) {
        return false;
    }

    QItemSelectionModel *selectionModel_p = selectionModel();

    if (!selectionModel_p->hasSelection()) {
        return false;
    }

    QModelIndexList *list = new QModelIndexList(selectionModel_p->selectedRows());

    while (!list->isEmpty()) {
        selectionList.append(list->takeFirst());
    }

    delete list;
    return selectionList.isEmpty() ? false : true;
}

/***********************************************************************************************************************
*   GetSelections
***********************************************************************************************************************/
bool CWorkspaceTreeView::GetSelections(CfgItemKind_t kind, QList<CCfgItem *>& selectionList)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_treeView != nullptr);
#endif
    if (g_treeView == nullptr) {
        return false;
    }

    QItemSelectionModel *selectionModel_p = selectionModel();

    if (!selectionModel_p->hasSelection()) {
        return false;
    }

    QModelIndexList *list = new QModelIndexList(selectionModel_p->selectedRows());
    std::sort(list->begin(), list->end());

    for (auto& modelIndex : *list) {
        CCfgItem *cfgItem_p = static_cast<CCfgItem *>(modelIndex.internalPointer());
        if ((kind == CFG_ITEM_KIND_None) || (cfgItem_p->m_itemKind == kind)) {
            selectionList.append(cfgItem_p);
        }
    }

    delete list;
    return selectionList.isEmpty() ? false : true;
}

/***********************************************************************************************************************
*   UnselectAll
***********************************************************************************************************************/
void CWorkspaceTreeView::UnselectAll(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_treeView != nullptr);
#endif
    if (g_treeView == nullptr) {
        return;
    }
    PRINT_SELECTION(QString("%1").arg(__FUNCTION__))

    QItemSelectionModel *selectionModel_p = g_treeView->selectionModel();
    selectionModel_p->clear();
}

/***********************************************************************************************************************
*   Unselect
***********************************************************************************************************************/
void CWorkspaceTreeView::Unselect(CCfgItem *cfgItem_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_treeView != nullptr);
#endif
    if (g_treeView == nullptr) {
        return;
    }

    PRINT_SELECTION(QString("%1 - %2").arg(__FUNCTION__).arg(cfgItem_p->m_itemText))

    QItemSelectionModel *selectionModel_p = g_treeView->selectionModel();

    QModelIndex index = g_workspace_p->toModelIndex(cfgItem_p);
    selectionModel_p->select(index, QItemSelectionModel::Deselect);
}

/***********************************************************************************************************************
*   Select
***********************************************************************************************************************/
void CWorkspaceTreeView::Select(CCfgItem *cfgItem_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_treeView != nullptr);
#endif
    if (g_treeView == nullptr) {
        return;
    }
    PRINT_SELECTION(QString("%1 - %2").arg(__FUNCTION__).arg(cfgItem_p->m_itemText))

    QItemSelectionModel *selectionModel_p = g_treeView->selectionModel();

    QModelIndex index = g_workspace_p->toModelIndex(cfgItem_p);
    selectionModel_p->select(index, QItemSelectionModel::Select);
    cfgItem_p->OnSelection();
}
