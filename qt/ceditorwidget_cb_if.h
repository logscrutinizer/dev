/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CEDITORWIDGET_CB_IF_H
#define CEDITORWIDGET_CB_IF_H

#include "CSelection.h"
#include <QCursor>

void CEditorWidget_UpdateGrayFont(int grayIntensity);
void CEditorWidget_SetRowClip(bool start, int row);
void CEditorWidget_ToggleBookmark(void);
void CEditorWidget_NextBookmark(bool backward);
void CEditorWidget_Initialize(bool reload);
void CEditorWidget_Repaint(void);
void CEditorWidget_SetTitle(QString& title);
void CEditorWidget_EmptySelectionList(void);
bool CEditorWidget_GetActiveSelection(CSelection& selection, QString& text);
void CEditorWidget_AddSelection(int TIA_Row, int startCol, int endCol, bool updateLastSelection,
                                bool updateMultiSelection = true, bool updateCursor = true,
                                bool cursorAfterSelection = true);
void CEditorWidget_SearchNewTopLine(int focusRow);
CSelection CEditorWidget_GetCursorPosition(void);
void CEditorWidget_AddFilterItem(void);
void CEditorWidget_TogglePresentationMode(void);
void CEditorWidget_SetPresentationMode_ShowOnlyFilterMatches(void);
void CEditorWidget_SetPresentationMode_ShowAll(void);
void CEditorWidget_Filter(void);
bool CEditorWidget_isPresentationModeFiltered(void);
void CEditorWidget_gotoBottom(void);
void CEditorWidget_SetFocusRow(int row);
void CEditorWidget_SetFocus(void);
void CEditorWidget_SetCursor(QCursor *cursor_p);

#endif /* CEDITORWIDGET_CB_IF_H */
