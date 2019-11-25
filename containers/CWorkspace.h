/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CDebug.h"
#include "CFilter.h"
#include "CCfgItem.h"
#include "plugin_api.h"

#include <QBitmap>
#include <QFont>
#include <QSurfaceFormat>
#include <QDir>
#include <QFileDevice>
#include <QSplitter>
#include <QHeaderView>
#include <QListView>
#include <QTableView>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QIcon>
#include <QVector>
#include <QSplitter>
#include <QLibrary>

/***********************************************************************************************************************
*   SelectionModel
***********************************************************************************************************************/
class SelectionModel : public QItemSelectionModel
{
    Q_OBJECT

public:
    explicit SelectionModel (QAbstractItemModel *model = Q_NULLPTR) : QItemSelectionModel(model) {}
    explicit SelectionModel (QAbstractItemModel *model, QObject *parent) : QItemSelectionModel(model, parent) {}

    /****/
    virtual void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override
    {
#ifdef _DEBUG
        TRACEX_DE(QString("set Selection %1").arg(index.row()))
#endif
        QItemSelectionModel::select(index, command);
    }

    /****/
    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) override
    {
#ifdef _DEBUG
        if (!selection.isEmpty() && !selection.first().isEmpty() && !selection.first().indexes().isEmpty()) {
            TRACEX_DE(QString("set Selection %1").arg(selection.first().indexes().first().row()))
        }
#endif
        QItemSelectionModel::select(selection, command);
    }

    /****/
    virtual void setCurrentIndex(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override
    {
#ifdef _DEBUG
        TRACEX_DE(QString("set Selection %1").arg(index.row()))
#endif
        QItemSelectionModel::setCurrentIndex(index, command);
    }
};

typedef enum {
    LS_VIEWTREE_EVENT_NONE_e = 0,
    LS_VIEWTREE_EVENT_MOUSE_e = 0x80,
    LS_VIEWTREE_EVENT_LMOUSE_DOWN_e,
    LS_VIEWTREE_EVENT_LMOUSE_UP_e,
    LS_VIEWTREE_EVENT_LMOUSE_DBL_e,
    LS_VIEWTREE_EVENT_MOUSE_MOVE_e,
}LS_VIEWTREE_EVENT_t;

/***********************************************************************************************************************
*   Model
***********************************************************************************************************************/
class Model : public QAbstractItemModel
{
    Q_OBJECT

public:
    Model(QObject *parent = nullptr) : QAbstractItemModel(parent), services(QPixmap(":main_icon")) {}
    virtual ~Model() override {}

    QModelIndex toIndex(CCfgItem *cfgItem_p);
    void itemUpdated(CCfgItem *item_p);
    void beginReset(void);
    void endReset(void);
    void startInsertRows(CCfgItem *parent, const CCfgItem *before, int count);
    void stopInsertRows(bool expand = true);
    void startRemoveRows(CCfgItem *parent, const CCfgItem *before, int count);
    void stopRemoveRows(void);
    int itemRow(CCfgItem *item_p) const;

    virtual QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const Q_DECL_OVERRIDE;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                              const QModelIndex &parent) Q_DECL_OVERRIDE;
    virtual bool hasChildren(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE;
    Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;
    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &child) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;

private:
    QIcon services;
    QFileIconProvider iconProvider;
};

/***********************************************************************************************************************
*   CWorkspace
***********************************************************************************************************************/
class CWorkspace : public QObject
{
    Q_OBJECT

public:
    CWorkspace();
    virtual ~CWorkspace() {}

public:
    void cleanAll();
    void CreateFilter(void);

    CCfgItem_FilterItem *AddFilterItem(char *filterText_p, CCfgItem_FilterItem *cfgFilterItem_p,
                                       CCfgItem_Filter *parentFilter_p, QWidget *parent);
    CCfgItem_Filter *CreateCfgFilter(CFilter *filterRef_p);
    bool ReinsertCfgFilter(CCfgItem_Filter *treeItemFilter_p);
    void DisableFilterItem(int uniqueID);
    void FilterItemProperties(int uniqueID, QWidget *widget_p);
    void GetMatchingFilters(QList<CCfgItem_FilterItem *> *filterItemList_p, const QString &match);
    int GetEnabledFilterItemCount(void);
    void GetFiltersHash(QByteArray& data);
    bool isFiltersChangedSizeLastFilter(void);
    void ToggleBookmark(QWidget *parent, QString *comment_p, int row);
    void AddBookmark(const QString& comment, int row);
    int GetClosestBookmark(int row);
    bool isBookmarked(int row);
    bool GetBookmarkList(QList<int> *bookmarkList_p);
    int GetBookmarksCount(void) {return m_bookmarks_p->m_cfgChildItems.count();}
    bool NextBookmark(int currentRow, int *bookmarkRow_p, bool backward = false);
    void RemoveAllBookmarks(void);
    void EditBookmark(int row);
    void AddPlugin(CPlugin_DLL_API *pluginAPI_p, QLibrary *library_p);
    void CloseAllPlugins(void);
    void CloseAllSelectedPlugins(void);
    void AddLog(char *path_p);
    void RemoveLog(void);
    void FillWorkspace(void);
    void TakeFocus(void);

    CCfgItem *GetCfgItem(const QString& fileName);
    bool GetSelectionFileInformation(QString *fileSaveInfo_p, CfgItemKind_t *itemKind_p = nullptr,
                                     CfgItem_PossibleFileOperations_t *fileOperations_p = nullptr);
    void ExecuteFileOperationOnSelection(CfgItem_PossibleFileOperations_t fileOperations);
    QString GetShortName(QString& filePath);
    QModelIndex toModelIndex(CCfgItem *cfgItem_p) {return m_model_p->toIndex(cfgItem_p);}
    void SetQuickSearch(int nChar);
    void QuickSearch(int nChar, bool searchDown);
    bool isItemSelected(CCfgItem *cfgItem_p);
    bool isSingleKindSelections(CfgItemKind_t *kind_p = nullptr);
    void PopupAction(int action);
    void Reorder(CCfgItem *afterItem_p);

private:
    QBitmap m_bmp_tree_closed;
    QBitmap m_bmp_tree_open;
    QBitmap m_bmp_F5;
    QBitmap m_bmp_filter_16_18;
    QBitmap m_bmp_filter_12_14;
    QBitmap m_bmp_filter_9_11;
    QBitmap m_bmp_filter_7_9;
    QBitmap m_bmp_enabled_16_14;
    QBitmap m_bmp_enabled_12_11;
    QBitmap m_bmp_enabled_8_7;
    bool m_CTRL_Pressed;
    bool m_SHIFT_Pressed;
    bool m_LMousePressed;
    QRect m_rect; /* The client rect when drawing */
    int m_h_offset; /* 0... -x (scroll offset) */
    int m_h_size; /* width of unclipped window */
    int m_v_offset; /* 0... -y (scroll offset) */
    int m_v_size; /* height of unclipped window */
    bool m_dragEnabled;
    bool m_dragOngoing;
    bool m_pendingSingleSelection;
    CCfgItem *m_singleSelection_p;
    QPoint m_point;
    int m_dragImageIndex;
    bool m_inFocus;
    bool m_needKillMenuOperation;
    bool m_EraseBackGroundDisabled;

public:
    CCfgItem *m_root_p;
    CCfgItem_Root *m_workspaceRoot_p;
    CCfgItem_Filters *m_filters_p;
    CCfgItem_Plugins *m_plugins_p;
    CCfgItem_Logs *m_logs_p;
    CCfgItem_Comments *m_comments_p;
    CCfgItem_Bookmarks *m_bookmarks_p;
    Model *m_model_p;
};

extern CWorkspace *g_workspace_p;
