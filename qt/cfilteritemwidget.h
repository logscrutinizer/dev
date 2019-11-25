/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CFilter.h"
#include "CLogScrutinizerDoc.h"
#include "CCfgItem.h"
#include <QWidget>
#include <QDialog>
#include <QComboBox>
#include <QMouseEvent>

#include <QDialog>

namespace Ui
{
    class FilterItemEditor;
}

/***********************************************************************************************************************
*   CFilterItemWidget
***********************************************************************************************************************/
class CFilterItemWidget : public QDialog
{
    Q_OBJECT

public:
    CFilterItemWidget() = delete;
    CFilterItemWidget(QWidget *pParent = nullptr) = delete;   /* standard constructor */

    CFilterItemWidget(
        QString *filterText_p,
        bool *enabled_p,
        Q_COLORREF *colorRef_p,
        Q_COLORREF *bgColorRef_p,
        bool *caseSensitive_p,
        bool *exclude_p,
        bool *regExpr_p,
        CCfgItem_Filters *filters_p,
        CCfgItem_Filter **filterSelection_pp,
        QWidget *pParent = nullptr);

    virtual ~CFilterItemWidget() override;

public:
    QString *m_filterText_p;
    Q_COLORREF *m_colorRef_p;
    Q_COLORREF *m_bgColorRef_p;
    bool *m_enabled_p;
    bool *m_caseSensitive_p;
    bool *m_exclude_p;
    bool *m_regExpr_p;
    QColor m_temp_color;
    QColor m_temp_bgColor;
    bool m_temp_caseSensitive;
    bool m_temp_exclude;
    bool m_temp_regExpr;
    bool m_temp_enabled;
    CCfgItem_Filters *m_filters_p;
    CCfgItem_Filter **m_filterSelection_pp;
    bool m_BG_isColorEnabled;

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

public:
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_checkBox_exclude_stateChanged(int arg1);
    void on_checkBox_matchCase_stateChanged(int arg1);
    void on_checkBox_regExpr_stateChanged(int arg1);
    void on_pushButton_textColor_clicked();
    void on_pushButton_BGTextColor_clicked();
    void on_checkBox_enableBGColor_stateChanged(int arg1);
    void on_lineEdit_filterItemEdit_textChanged(const QString &arg1);

    void on_checkBox_enable_stateChanged(int arg1);

private:
    Ui::FilterItemEditor *ui;
};
