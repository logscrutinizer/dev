/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "ceditorwidget.h"
#include <QPainter>
#include <memory>
#include <QSettings>
#include <QApplication>

#include <exception>

#include "mainwindow_cb_if.h"

using namespace std;

CEditorWidget *g_editorWidget_p = nullptr;
extern CLogScrutinizerDoc *GetDocument(void);

/***********************************************************************************************************************
*   GetTheView
***********************************************************************************************************************/
CEditorWidget *GetTheView(void)
{
    return g_editorWidget_p;
}

/***********************************************************************************************************************
*   CEditorWidget_UpdateGrayFont
***********************************************************************************************************************/
void CEditorWidget_UpdateGrayFont(int grayIntensity)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    if (g_editorWidget_p != nullptr) {
        g_cfg_p->m_log_GrayIntensity = grayIntensity;
        g_editorWidget_p->UpdateGrayFont();
    }
}

/***********************************************************************************************************************
 *   CEditorWidget_ToggleBookmark
 * row value of
 *      -1 means disable
 *      -2 means take from first selection
 **********************************************************************************************************************/
void CEditorWidget_SetRowClip(bool start, int row)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    if (g_editorWidget_p != nullptr) {
        TRACEX_D("CEditorWidget_SetClipStart - start:%d row:%d", static_cast<int>(start), row)

        CSelection selection;
        int selectionRow = 0;

        if (row == -1) {
            selectionRow = -1;
        } else if (row == -2) {
            if (g_editorWidget_p->GetActiveSelection(&selection) || g_editorWidget_p->GetCursorPosition(&selection)) {
                selectionRow = selection.row;
            } else {
                TRACEX_I("There was no selection to create a rowClip")
                return;
            }
        } else {
            selectionRow = row;
        }

        g_editorWidget_p->SetRowClip(start, selectionRow);
        g_editorWidget_p->Refresh();
    }
}

/***********************************************************************************************************************
*   CEditorWidget_ToggleBookmark
***********************************************************************************************************************/
void CEditorWidget_ToggleBookmark(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    if (g_editorWidget_p != nullptr) {
        g_editorWidget_p->ToggleBookmark();
    }
}

/***********************************************************************************************************************
*   CEditorWidget_NextBookmark
***********************************************************************************************************************/
void CEditorWidget_NextBookmark(bool backward)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    if (g_editorWidget_p != nullptr) {
        g_editorWidget_p->NextBookmark(backward);
    }
}

/***********************************************************************************************************************
*   CEditorWidget_SetFocusRow
***********************************************************************************************************************/
void CEditorWidget_SetFocusRow(int row)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->SetFocusRow(row);
}

/***********************************************************************************************************************
*   CEditorWidget_SetFocus
***********************************************************************************************************************/
void CEditorWidget_SetFocus(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->setFocus();
}

/***********************************************************************************************************************
*   CEditorWidget_SetCursor
***********************************************************************************************************************/
void CEditorWidget_SetCursor(QCursor *cursor_p)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    if (cursor_p != nullptr) {
        g_editorWidget_p->setCursor(*cursor_p);
    } else {
        g_editorWidget_p->unsetCursor();
    }
}

/***********************************************************************************************************************
*   CEditorWidget_Initialize
***********************************************************************************************************************/
void CEditorWidget_Initialize(bool reload)
{
    Q_UNUSED(reload)

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->Initialize();
}

/***********************************************************************************************************************
*   CEditorWidget_SetTitle
***********************************************************************************************************************/
void CEditorWidget_SetTitle(QString& title)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->setWindowTitle(title);
}

/***********************************************************************************************************************
*   CEditorWidget_Repaint
***********************************************************************************************************************/
void CEditorWidget_Repaint(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->update();
}

/***********************************************************************************************************************
*   CEditorWidget_EmptySelectionList
***********************************************************************************************************************/
void CEditorWidget_EmptySelectionList(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->EmptySelectionList();
}

/***********************************************************************************************************************
*   CEditorWidget_GetActiveSelection
***********************************************************************************************************************/
bool CEditorWidget_GetActiveSelection(CSelection& selection, QString& text)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return false;
    }
    return g_editorWidget_p->GetActiveSelection(&selection, text);
}

/***********************************************************************************************************************
*   CEditorWidget_AddFilterItem
***********************************************************************************************************************/
void CEditorWidget_AddFilterItem(void)
{
    if (g_editorWidget_p == nullptr) {
        return;
    }
    g_editorWidget_p->AddFilterItem();
    g_editorWidget_p->update();
}

/***********************************************************************************************************************
*   CEditorWidget_AddSelection
***********************************************************************************************************************/
void CEditorWidget_AddSelection(int TIA_Row, int startCol, int endCol, bool updateLastSelection,
                                bool updateMultiSelection, bool updateCursor, bool cursorAfterSelection)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->AddSelection(TIA_Row, startCol, endCol, updateLastSelection, updateMultiSelection, updateCursor,
                                   cursorAfterSelection);
}

/***********************************************************************************************************************
*   CEditorWidget_GetCursorPosition
***********************************************************************************************************************/
CSelection CEditorWidget_GetCursorPosition(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return CSelection();
    }

    return g_editorWidget_p->GetCursorPosition();
}

/***********************************************************************************************************************
*   CEditorWidget_SearchNewTopLine
***********************************************************************************************************************/
void CEditorWidget_SearchNewTopLine(int focusRow)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->SearchNewTopLine(false, focusRow);
}

/***********************************************************************************************************************
*   CEditorWidget_TogglePresentationMode
***********************************************************************************************************************/
void CEditorWidget_TogglePresentationMode(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->TogglePresentationMode();
}

/***********************************************************************************************************************
*   CEditorWidget_SetPresentationMode_ShowOnlyFilterMatches
***********************************************************************************************************************/
void CEditorWidget_SetPresentationMode_ShowOnlyFilterMatches(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->SetPresentationMode(PRESENTATION_MODE_ONLY_FILTERED_e);
}

/***********************************************************************************************************************
*   CEditorWidget_SetPresentationMode_ShowAll
***********************************************************************************************************************/
void CEditorWidget_SetPresentationMode_ShowAll(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->SetPresentationMode(PRESENTATION_MODE_ALL_e);
}

/***********************************************************************************************************************
*   CEditorWidget_isPresentationModeFiltered
***********************************************************************************************************************/
bool CEditorWidget_isPresentationModeFiltered(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return PRESENTATION_MODE_ALL_e;
    }

    return (PRESENTATION_MODE_ONLY_FILTERED_e == g_editorWidget_p->GetPresentationMode()) ? true : false;
}

/***********************************************************************************************************************
*   CEditorWidget_gotoBottom
***********************************************************************************************************************/
void CEditorWidget_gotoBottom(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->GotoBottom();
}

/***********************************************************************************************************************
*   CEditorWidget_Filter
***********************************************************************************************************************/
void CEditorWidget_Filter(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (g_editorWidget_p == nullptr) {
        return;
    }

    g_editorWidget_p->Filter();
    g_editorWidget_p->SetPresentationMode(PRESENTATION_MODE_ONLY_FILTERED_e);
    g_editorWidget_p->setFocus(Qt::ActiveWindowFocusReason);
}

CEditorWidget::CEditorWidget(QWidget *parent) :
    QWidget(parent)
{
    m_background = QBrush(QColor(64, 32, 64));

    setFocusPolicy(Qt::StrongFocus);  /* This property holds the way the widget accepts keyboard focus. The policy is
                                       * Qt::TabFocus if the widget accepts keyboard focus by tabbing, Qt::ClickFocus if
                                       * the widget accepts focus by clicking, Qt::StrongFocus if it accepts both, */
    memset(m_keys, 0, sizeof(m_keys));

    m_cursorTimer = std::make_unique<QTimer>(this);
    connect(m_cursorTimer.get(), SIGNAL(timeout()), this, SLOT(OnCursorTimer()));

    m_vscrollTimer = std::make_unique<QTimer>(this);
    connect(m_vscrollTimer.get(), SIGNAL(timeout()), this, SLOT(HandleVScrollBitmapTimer()));

    m_autoHighLightTimer = std::make_unique<QTimer>(this);
    m_autoHighLightTimer->setSingleShot(true);
    connect(m_autoHighLightTimer.get(), SIGNAL(timeout()), this, SLOT(OnAutoHighlightTimer()));

    /*setFixedSize(200, 200); */
    setAutoFillBackground(false);

    auto doc_p = GetDocument();

    m_blackFont_p = doc_p->m_fontCtrl.RegisterFont(BLACK, BACKGROUND_COLOR);
    m_whiteFont_p = doc_p->m_fontCtrl.RegisterFont(WHITE, BACKGROUND_COLOR);
    m_grayFont_p = doc_p->m_fontCtrl.RegisterFont(Q_RGB(g_cfg_p->m_log_GrayIntensity,
                                                        g_cfg_p->m_log_GrayIntensity,
                                                        g_cfg_p->m_log_GrayIntensity), BACKGROUND_COLOR);
    m_clippedFont_p = doc_p->m_fontCtrl.RegisterFont(CLIPPED_TEXT_COLOR, BACKGROUND_COLOR);
    m_FontStatusBar = QFont(g_cfg_p->m_default_Font, STATUS_BAR_CHAR_SIZE, QFont::Normal);
    m_FontEmptyWindow = QFont(g_cfg_p->m_default_Font, EMPTY_WINDOW_CHAR_SIZE, QFont::Normal);

    g_editorWidget_p = this;

    Initialize_0();
}

/***********************************************************************************************************************
*   paintEvent
***********************************************************************************************************************/
void CEditorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    try {
        LS_Painter painter(this);
        m_painter_p = &painter; /* this will be switched in ceditorwidget_ext in-case double buffering is used. */

        m_painter_p->setBrush(m_background);
        GetTheDoc()->m_fontCtrl.SetFont(m_painter_p, m_whiteFont_p);
        OnDraw(); /* After this call it might be that m_painter_p is exchanged to the double buffer */
    } catch (...) {}

    /* DO NOT USE m_painter_p HERE */
}

/***********************************************************************************************************************
*   storeSettings
***********************************************************************************************************************/
void CEditorWidget::storeSettings(void)
{
#ifdef LOCAL_GEOMETRY_SETTING
    QSettings settings;
    settings.setValue("editorWindow", size());
#endif
}

/***********************************************************************************************************************
*   sizeHint
***********************************************************************************************************************/
QSize CEditorWidget::sizeHint() const
{
    static QSize windowSize;
    windowSize = m_lastResize;

    auto refreshEditorWindow = makeMyScopeGuard([&] () {
        PRINT_SIZE(QString("Editor sizeHint %1,%2").arg(windowSize.width()).arg(windowSize.height()))
    });

    if (CSCZ_AdaptWindowSizes) {
        windowSize = m_adaptWindowSize;
        PRINT_SIZE(QString("Editor adaptWindowSizes %1,%2").arg(windowSize.width()).arg(windowSize.height()))
    }

    return windowSize;
}
