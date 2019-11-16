/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "cfilteritemwidget.h"
#include "CDebug.h"
#include "CFontCtrl.h"
#include "quickFix.h"

#include <QDialog>
#include "ui_filteritemeditor.h"
#include <QColorDialog>
#include <QPainter>
#include <QPen>
#include <QBrush>

CFilterItemWidget::CFilterItemWidget(
    QString *filterText_p,
    bool *enabled_p,
    Q_COLORREF *colorRef_p,
    Q_COLORREF *bgColorRef_p,
    bool *caseSensitive_p,
    bool *exclude_p,
    bool *regExpr_p,
    CCfgItem_Filters *filters_p,
    CCfgItem_Filter **filterSelection_pp,
    QWidget *pParent)
    : QDialog(pParent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint), ui(new Ui::FilterItemEditor)
{
    TRACEX_DE(QString("CFilterItemWidget text:%1").arg(*filterText_p))

    m_filterText_p = filterText_p;
    m_enabled_p = enabled_p;
    m_colorRef_p = colorRef_p;
    m_bgColorRef_p = bgColorRef_p;
    m_caseSensitive_p = caseSensitive_p;
    m_exclude_p = exclude_p;
    m_regExpr_p = regExpr_p;
    m_filters_p = filters_p;
    m_filterSelection_pp = filterSelection_pp;

    m_BG_isColorEnabled = false;
    if (*m_bgColorRef_p != BACKGROUND_COLOR) {
        m_BG_isColorEnabled = true;
    }

    m_temp_color = *m_colorRef_p;
    m_temp_bgColor = *m_bgColorRef_p;
    m_temp_caseSensitive = *m_caseSensitive_p;
    m_temp_exclude = *m_exclude_p;
    m_temp_regExpr = *m_regExpr_p;
    m_temp_enabled = *m_enabled_p;

    setWindowTitle("Filter Item Editor");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);

    const QIcon mainIcon = QIcon(":main_icon");
    setWindowIcon(mainIcon);

    /* Use filteritemeditor.ui to setup the layout */
    ui->setupUi(this);

    if (m_filterSelection_pp != nullptr) {
        ui->comboBox_filter->addItem(
            *(static_cast<CCfgItem_Filter *>(*m_filterSelection_pp)->m_filter_ref_p->GetShortFileName()),
            QVariant::fromValue(static_cast<void *>(*m_filterSelection_pp)));
    }

    /* Add filters to the combobox (besides the selected one that has already been added */
    if ((m_filters_p != nullptr) && !m_filters_p->m_cfgChildItems.isEmpty()) {
        for (auto& filter_p : m_filters_p->m_cfgChildItems) {
            if (filter_p != *m_filterSelection_pp) {
                ui->comboBox_filter->addItem(
                    *(static_cast<CCfgItem_Filter *>(filter_p)->m_filter_ref_p->GetShortFileName()),
                    QVariant::fromValue(static_cast<void *>(filter_p)));
            }
        }
    }

    ui->checkBox_enable->setChecked(m_temp_enabled);
    ui->checkBox_enableBGColor->setChecked(m_BG_isColorEnabled);
    ui->pushButton_BGTextColor->setEnabled(m_BG_isColorEnabled);
    ui->checkBox_exclude->setChecked(m_temp_exclude);
    ui->checkBox_matchCase->setChecked(m_temp_caseSensitive);
    ui->checkBox_regExpr->setChecked(m_temp_regExpr);

    if (filterText_p->isEmpty()) {
        *filterText_p = QString("Add match string");
    }
    ui->lineEdit_filterItemEdit->setText(*filterText_p);
    ui->lineEdit_filterItemEdit->selectAll();
}

CFilterItemWidget::~CFilterItemWidget()
{
    delete ui;
}

/***********************************************************************************************************************
*   paintEvent
***********************************************************************************************************************/
void CFilterItemWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect labelFrame = ui->label_textExampleFrame->geometry();

    if (m_BG_isColorEnabled) {
        painter.fillRect(labelFrame, m_temp_bgColor);
    } else {
        painter.fillRect(labelFrame, BACKGROUND_COLOR);
    }

    QBrush brushy(m_temp_color);
    QPen pen(brushy, 2);

    painter.setBrush(brushy); /* To achive the color */
    painter.setPen(pen); /* To achive the color */

    painter.drawText(labelFrame, ui->lineEdit_filterItemEdit->text(), Qt::AlignHCenter | Qt::AlignVCenter);
}

/***********************************************************************************************************************
*   on_buttonBox_accepted
***********************************************************************************************************************/
void CFilterItemWidget::on_buttonBox_accepted()
{
    int currentIndex = ui->comboBox_filter->currentIndex();
    QVariant variant = ui->comboBox_filter->itemData(currentIndex);
    if ((m_filterSelection_pp != nullptr) && (static_cast<CCfgItem_Filter *>(variant.value<void *>()) != nullptr)) {
        *m_filterSelection_pp = static_cast<CCfgItem_Filter *>(variant.value<void *>());
    }

    *m_caseSensitive_p = m_temp_caseSensitive;
    *m_exclude_p = m_temp_exclude;
    *m_regExpr_p = m_temp_regExpr;
    *m_enabled_p = m_temp_enabled;

    *m_colorRef_p = m_temp_color.rgb();
    if (m_BG_isColorEnabled) {
        *m_bgColorRef_p = m_temp_bgColor.rgb();
    } else {
        *m_bgColorRef_p = BACKGROUND_COLOR;
    }
    *m_filterText_p = ui->lineEdit_filterItemEdit->text();
}

/***********************************************************************************************************************
*   on_buttonBox_rejected
***********************************************************************************************************************/
void CFilterItemWidget::on_buttonBox_rejected()
{}

/***********************************************************************************************************************
*   on_checkBox_exclude_stateChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_checkBox_exclude_stateChanged(int arg1)
{
    m_temp_exclude = arg1 ? true : false;
}

/***********************************************************************************************************************
*   on_checkBox_matchCase_stateChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_checkBox_matchCase_stateChanged(int arg1)
{
    m_temp_caseSensitive = arg1 ? true : false;
}

/***********************************************************************************************************************
*   on_checkBox_regExpr_stateChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_checkBox_regExpr_stateChanged(int arg1)
{
    m_temp_regExpr = arg1 ? true : false;
}

/***********************************************************************************************************************
*   on_pushButton_textColor_clicked
***********************************************************************************************************************/
void CFilterItemWidget::on_pushButton_textColor_clicked()
{
    QColor color = QColorDialog::getColor(*m_colorRef_p, this);
    if (color.isValid()) {
        m_temp_color = color;
    }
    repaint();
}

/***********************************************************************************************************************
*   on_pushButton_BGTextColor_clicked
***********************************************************************************************************************/
void CFilterItemWidget::on_pushButton_BGTextColor_clicked()
{
    QColor color = QColorDialog::getColor(*m_bgColorRef_p, this);
    if (color.isValid()) {
        m_temp_bgColor = color;
    }
    repaint();
}

/***********************************************************************************************************************
*   on_checkBox_enableBGColor_stateChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_checkBox_enableBGColor_stateChanged(int arg1)
{
    m_BG_isColorEnabled = arg1 ? true : false;

    ui->pushButton_BGTextColor->setDisabled(!m_BG_isColorEnabled);

    repaint();
}

/***********************************************************************************************************************
*   on_lineEdit_filterItemEdit_textChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_lineEdit_filterItemEdit_textChanged(const QString &arg1)
{
    repaint();
}

/***********************************************************************************************************************
*   on_checkBox_enable_stateChanged
***********************************************************************************************************************/
void CFilterItemWidget::on_checkBox_enable_stateChanged(int arg1)
{
    m_temp_enabled = arg1 ? true : false;
}
