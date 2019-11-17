/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QWidget>
#include <QCompleter>

#include "csubplotsurface.h"
#include "plugin_api.h"
#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "CFontCtrl.h"
#include "ceditorwidget.h"
#include "ui_searchform.h"

/***********************************************************************************************************************
*   CSearchWidget
***********************************************************************************************************************/
class CSearchWidget : public QWidget
{
    Q_OBJECT

public:
    CSearchWidget() {}
    virtual ~CSearchWidget() override {}

#if 0
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
#endif
    void storeSettings(void);
    void clearHistory(void);
    void addToHistory(const QString& searchText);
    void updateSearchParameters(const QString& searchText, bool caseSensitive = false, bool regExp = false);
    void addCurrentToHistory(void);

    void getSearchParameters(QString& searchString, bool *caseSensitive, bool *regExp);

    /* Activate is used to re-focus on the text edit */
    void activate(void);

    /***********************************************************************************************************************
    *   sizeHint
    ***********************************************************************************************************************/
    QSize sizeHint() const Q_DECL_OVERRIDE
    {
        static QSize windowSize;
        auto refreshEditorWindow = makeMyScopeGuard([&] () {
            PRINT_SIZE(QString("Search sizeHint %1,%2").arg(windowSize.width()).arg(windowSize.height()))
        });
        windowSize = QWidget::sizeHint();

        if (CSCZ_AdaptWindowSizes) {
            windowSize = m_adaptWindowSize;
            PRINT_SIZE(QString("Search adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
        }

        return windowSize;
    }

    void setAdaptWindowSize(const QSize& size) {m_adaptWindowSize = size;}

private:
    QSize m_adaptWindowSize;

private slots:
    void on_pushButton_Back_clicked(void);
    void on_pushButton_Forward_clicked(void);

private:
    QCompleter completer;

public:
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
};
