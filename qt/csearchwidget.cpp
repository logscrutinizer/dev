/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CLogScrutinizerDoc.h"
#include "csearchwidget.h"
#include "mainwindow_cb_if.h"

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"

#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QSettings>
#include <QPushButton>
#include <QCheckBox>
#include <QApplication>

/***********************************************************************************************************************
*   on_pushButton_Forward_clicked
***********************************************************************************************************************/
void CSearchWidget::on_pushButton_Forward_clicked(void)
{
    TRACEX_DE("Forward");
    addCurrentToHistory();
    CEditorWidget_EmptySelectionList();
    MW_Search(true);
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CSearchWidget::keyPressEvent(QKeyEvent *e)
{
    int index = e->key();
    switch (index)
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            on_pushButton_Forward_clicked();
            break;

        default:

            auto SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
            auto CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
            if (!MW_GeneralKeyHandler(index, CTRL_Pressed, SHIFT_Pressed)) {
                QWidget::keyPressEvent(e);
            }
            break;
    }
}

/***********************************************************************************************************************
*   on_pushButton_Back_clicked
***********************************************************************************************************************/
void CSearchWidget::on_pushButton_Back_clicked(void)
{
    TRACEX_DE("Backwards");
    addCurrentToHistory();
    CEditorWidget_EmptySelectionList();
    MW_Search(false);
}

/***********************************************************************************************************************
*   addCurrentToHistory
***********************************************************************************************************************/
void CSearchWidget::addCurrentToHistory(void)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    if (combo_p->findText(combo_p->currentText(), Qt::MatchCaseSensitive) == -1) {
        combo_p->insertItem(0, combo_p->currentText(), QVariant());
    }
}

/***********************************************************************************************************************
*   addToHistory
***********************************************************************************************************************/
void CSearchWidget::addToHistory(const QString& searchText)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    auto item = combo_p->findText(searchText, Qt::MatchCaseSensitive);
    if (item == -1) {
        combo_p->insertItem(0, searchText, QVariant());
        item = 0;
    }
    combo_p->setCurrentIndex(item);
}

/***********************************************************************************************************************
*   updateSearchParameters
***********************************************************************************************************************/
void CSearchWidget::updateSearchParameters(const QString& searchText, bool caseSensitive, bool regExp)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    combo_p->setCurrentText(searchText);

    QCheckBox *caseOption_p = findChild<QCheckBox *>("case_option");
    if (caseOption_p != nullptr) {
        caseOption_p->setChecked(caseSensitive);
    }

    QCheckBox *regextOption_p = findChild<QCheckBox *>("regext_option");
    if (regextOption_p != nullptr) {
        regextOption_p->setChecked(regExp);
    }
}

/***********************************************************************************************************************
*   getSearchParameters
***********************************************************************************************************************/
void CSearchWidget::getSearchParameters(QString& searchString, bool *caseSensitive, bool *regExp)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    searchString = combo_p->currentText();

    Q_ASSERT(findChild<QCheckBox *>("case_option"));
    Q_ASSERT(findChild<QCheckBox *>("regext_option"));

    QCheckBox *caseOption_p = findChild<QCheckBox *>("case_option");
    *caseSensitive = caseOption_p->isChecked();

    QCheckBox *regextOption_p = findChild<QCheckBox *>("regext_option");
    *regExp = regextOption_p->isChecked();
}

/***********************************************************************************************************************
*   showEvent
***********************************************************************************************************************/
void CSearchWidget::showEvent(QShowEvent *event)
{
    activate();
}

/***********************************************************************************************************************
*   activate
***********************************************************************************************************************/
void CSearchWidget::activate(void)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    combo_p->setAutoCompletion(true);
    combo_p->setAutoCompletionCaseSensitivity(Qt::CaseSensitive);
    combo_p->setFocus();
}

/***********************************************************************************************************************
*   clearHistory
***********************************************************************************************************************/
void CSearchWidget::clearHistory(void)
{
    QComboBox *combo_p = findChild<QComboBox *>("comboBox");
    combo_p->clear();
}

/***********************************************************************************************************************
*   storeSettings
***********************************************************************************************************************/
void CSearchWidget::storeSettings(void)
{
#ifdef LOCAL_GEOMETRY_SETTING
    QSettings settings;
    settings.setValue("searchWindow", size());
#endif
}
