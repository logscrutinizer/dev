/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef MAINWINDOW_CB_IF_H
#define MAINWINDOW_CB_IF_H

#include <QItemSelectionModel>
#include "cplotwidget_if.h"

class CPlot; /* forward */
class CSubPlot; /*f forward */

typedef enum {
    SYSTEM_SOUND_PING,
    SYSTEM_SOUND_FAILURE,
    SYSTEM_SOUND_QUESTION,
    SYSTEM_SOUND_EXIT,
    SYSTEM_SOUND_START
} MW_SystemSound;

QItemSelectionModel *MW_selectionModel(void);
QApplication *MW_getApp(void);

void MW_SetWorkspaceWidgetFocus(void);
void MW_updateLogFileTrackState(bool track);
void MW_toggleLogFileTrackState(void);

/* Store the current geometry and state to be saved when state is changed. When eventually the state is changed
 * these values are then written to the registry such it can be reloaded when the state is put back to the same. */
void MW_updatePendingStateGeometry(void);

void MW_PlaySystemSound(MW_SystemSound sound);
QSize MW_Size(void);

QWidget *MW_Parent(void);
void MW_RebuildRecentFileMenu(void);
void MW_Refresh(bool forceVScroll = false);

CPlotWidgetInterface *MW_AttachPlot(CPlot *plot_p);
void MW_DetachPlot(CPlot *plot_p);
void MW_SetSubPlotUpdated(CSubPlot *subPlot_p);

void MW_RedrawPlot(CPlot *plot_p);
void MW_DetachPlot(CPlot *plot_p);
void MW_RedrawPlot(CPlot *plot_p);
void MW_QuickSearch(char *searchString, bool searchDown, bool caseSensitive, bool regExpr);
void MW_SetCursor(QCursor *cursor_p);

void MW_SetApplicationName(QString *name);
bool MW_AppendLogMsg(const QString& message);
void MW_StartWebPage(const QString& url);

int64_t MW_GetTick(void);

void MW_Search(bool forward = true);
void MW_ActivateSearch(const QString& searchText, bool caseSensitive = false, bool regExp = false);
void MW_UpdateSearchParameters(const QString& searchText, bool caseSensitive = false, bool regExp = false);
void MW_ModifyFontSize(int increase);

bool MW_GeneralKeyHandler(int key, bool ctrl, bool shift);

#endif /* MAINWINDOW_CB_IF_H */
