/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CWORKSPACE_TREE_VIEW_H
#define CWORKSPACE_TREE_VIEW_H

#include "CDebug.h"
#include "CCfgItem.h"

#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <QWidget>
#include <QDockWidget>
#include <QTreeView>

/***********************************************************************************************************************
*   CWorkspaceTreeView
***********************************************************************************************************************/
class CWorkspaceTreeView : public QTreeView
{
    Q_OBJECT

public:
    CWorkspaceTreeView(QWidget *parent = nullptr);
    virtual ~CWorkspaceTreeView() override;

    bool GetSelections(CfgItemKind_t kind, QList<CCfgItem *>& selectionList);
    bool GetSelectionIndexes(QModelIndexList& selectionList);
    void UnselectAll(void);
    void Unselect(CCfgItem *cfgItem_p);
    void Select(CCfgItem *cfgItem_p);
    void storeSettings(void);
    void setAdaptWindowSize(QSize& size) {m_adaptWindowSize = size;}

    /* From QTreeView or QAbstractTreeView */
    virtual void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

public slots:
    void selectionChanged(const QItemSelection& selected,
                          const QItemSelection& deselected) Q_DECL_OVERRIDE;

protected:
    /****/
    virtual void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE {
        CSCZ_UnsetLastViewSelectionKind(CSCZ_LastViewSelectionKind_Workspace_e);
        PRINT_FOCUS(QString("Workspace focus out"));
        QTreeView::focusOutEvent(event);
    }

    /****/
    virtual void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE
    {
        CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_Workspace_e);
        PRINT_FOCUS(QString("Workspace focus in"));
        QTreeView::focusInEvent(event);
    }

    /****/
    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE
    {
        PRINT_FOCUS(QString("Workspace mouse move"));
        QTreeView::mouseMoveEvent(event);
    }

    /****/
    virtual bool event(QEvent *event) Q_DECL_OVERRIDE
    {
        auto type = event->type();
        if (type != QEvent::Paint) {
            TRACEX_D(QString("Workspace event %1").arg(type))
        }
        return QTreeView::event(event);
    }

    void resizeText(bool increase);
    QSize m_adaptWindowSize;
    QSize m_lastResize;
};

#endif /* CWORKSPACE_TREE_VIEW_H */
