/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CEDITORWIDGET_H
#define CEDITORWIDGET_H

#include "CSelection.h"
#include "CLogScrutinizerDoc.h"
#include "utils.h"

#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <QWidget>
#include <QDockWidget>
#include <QRect>
#include <QBitmap>
#include <QTimer>
#include <QTextOption>

#include <memory>

#define CURSOR_TIMER_ID         10
#define CURSOR_TIMER_DURATION       500
#define CURSOR_TIMER_DURATION_FAST  100
#define CURSOR_BMP_INDEX        0

#define CURSOR_UPDATE_NO_ACTION (-2)
#define CURSOR_UPDATE_FULL_ROW (-1)

#define AUTO_HIGHLIGHT_TIMER_ID           11
#define AUTO_HIGHLIGHT_TIMER_DURATION     1000

#define VSCROLL_TIMER_ID            21
#define VSCROLL_TIMER_DURATION      1500

#define WINDOW_MARGIN_TOP     10
#define WINDOW_MARGIN_BOTTOM  10
#define WINDOW_MARGIN_LEFT    10
#define WINDOW_MARGIN_RIGHT   10

#define WINDOW_WINDOW_MARGIN  2

#define STATUS_FIELD_HEIGHT   20

#define TAB_TEXT_MARGIN       10

#define SLIDER_SIZE_X         20
#define SLIDER_SIZE_Y         40

#define TEXT_SCROLL_SLIDER_SIZE_X 20

#define TAB_WINDOW_FRACTION_X         ((float)0.3)
#define TAB_WINDOW_FRACTION_Y         ((float)0.7)

#define MAX_BMP_STORAGE_INDEX       8

#define LS_LG_NONE        0

const int NUM_TAB_STOBS = DISPLAY_MAX_ROW_SIZE / 4;

typedef enum {
    PRESENTATION_MODE_ALL_e,

    /* Show all, if there are filtered lines these are in gray */
    PRESENTATION_MODE_ONLY_FILTERED_e,

    /* Show only filtered lines */
} LogScrutinizerView_PresentaiontMode_t;

typedef enum {
    ROW_PRESENTATION_NORMAL_e,
    ROW_PRESENTATION_FILTERED_e,
    ROW_PRESENTATION_GRAYED_e,
    ROW_PRESENTATION_CLIPPED_e,
    ROW_PRESENTATION_BOOKMARKED_e,
    ROW_PRESENTATION_COMMENT_e,
    ROW_PRESENTATION_HEAD_FRAME_e,
    ROW_PRESENTATION_FOOTER_FRAME_e
} LogScrutinizerView_RowPresentaion_t;

typedef struct {
    int row;
    QRect screenRect;
    LogScrutinizerView_RowPresentaion_t presentation;
    bool valid;
} LogScrutinizerView_ScreenRow_t;

typedef struct {
    QBitmap *bmp_p;
    QRect destRect;
} BMP_Storage_t;

/* MOUSE EVENTS */

typedef enum {
    LS_LG_EVENT_NONE_e = 0,
    LS_LG_EVENT_MOUSE_e = 0x80,

    /* Group */
    LS_LG_EVENT_LMOUSE_DOWN_e,
    LS_LG_EVENT_LMOUSE_UP_e,
    LS_LG_EVENT_LMOUSE_DBL_e,
    LS_LG_EVENT_MOUSE_MOVE_e,
    LS_LG_EVENT_KEY_e = 0x100,

    /* Group */
    LS_LG_EVENT_KEY_CTRL_C_e,
    LS_LG_EVENT_KEY_CTRL_A_e
} LS_LG_EVENT_t;

typedef enum {
    HCursorScrollAction_Left_en,

    /* Make sure that the cursor is visible, and if needed scroll screen such that the cursor is at the far left
     * position */
    HCursorScrollAction_Right_en,

    /* Make sure that the cursor is visible, and if needed scroll screen such that the cursor is at the far right
     * position */
    HCursorScrollAction_Home_en,

    /* If cursorCol is 0, then scroll max left.
     * special
     * If cursorCol is not 0, but within 10% of left, then scroll max left.
     * Or, scroll such that cursor ends up at 10% left of the screen */
    HCursorScrollAction_End_en,

    /* if cursorCol is max, and longest line on screen, then scroll max right
     * if not longest line on screen scroll, but less than 10% from the right
     * if scrolled to the max right, scroll max right
     * if not longest line on  screen scroll such that cursor is at 10% the right */
    HCursorScrollAction_Focus_en      /* Used when a certain column is to be focused around, e.g. when row scrolling and
                                       * next row isn't long enough */
} HCursorScrollAction_e;

typedef struct {
    int y_line; /**< The pixel coordinates */
    int startRow; /**< Defines the rows analyzed for a certain rock scroll raster */
    int endRow;
    int bestRow; /**< The row winning the rock scroll raster, defining the color. Used to position top row to show
                  * that row in the middle */
    Q_COLORREF color;
} RockScrollItem_t;

typedef struct {
    int rowsPerRaster;
    RockScrollItem_t *itemArray_p;
    int numberOfItems; /**< actual number of items in itemArray_p */
    int itemArraySize; /**< MAX size, numberOfItems describe the actual number of items */
} RockScrollInfo_t;

/***********************************************************************************************************************
*   CEditorWidget
***********************************************************************************************************************/
class CEditorWidget : public QWidget /*QOpenGLWidget */
{
    Q_OBJECT

public:
    explicit CEditorWidget(QWidget *parent = nullptr);
    ~CEditorWidget() {CleanUp_0();}

private:
    QBrush m_background;
    std::unique_ptr<QTimer> m_autoHighLightTimer;
    std::unique_ptr<QTimer> m_cursorTimer;
    std::unique_ptr<QTimer> m_vscrollTimer;

protected:
    int m_onDrawCount;
    bool m_windowCfgChanged;
    QSize m_adaptWindowSize; /**< Set by main window when settings didn't contain window sizes. */
    QSize m_lastResize; /**< From resizeEvent */

    /* Seems to be a bug in Qt, after resize the cursor sometimes get stuck in resize icon. Hence, after are resize
     * action LogScrutinizer will manually reset the icon regardless... as the missalignment cannot be detected
     * with reading the current cursor shape. */
    bool m_resizeCursorSync = false;
    QRect m_rcClient; /**< The rcClient provided at OnDraw() */
    QRect m_textWindow; /**< m_window excluding the padding from the screen edge to first text pixels, the
                         * area where text can be put */
    QRect m_textRow_Head_X; /**< The extra characters to the left, start and stop X coordinates (m_textWindow.left
                             * + padding  -> size of largest row index in pixels) */
    QRect m_textRow_X; /**< The start and stop X coordinates for a text row  (m_textRow_Head_X.right +
                        * padding  -> m_textWindow.right - padding) */
    int m_beam_left_padding; /**< m_beam_left_padding is the small extra pixels needed to some extra space infront of
                              * the row such that it is easier to select the first letter. It is dependent on the
                              * current font-size. */
    QRect m_rcBoarder; /**< The rcBoarder is used to draw the boarder bitmaps. It is 10pix shifted to the
                        * left and up such that it can be used directly to bitblit */
    QRect m_bmpWindow; /**< The area of the bitmap where everything is first drawn to, little bit larger than
                        * necessary. A portion of this is copied on-screen (horizontal scroll offset) */
    int m_hbmpOffset; /**< How much the graphics (bitmap) buffer has been shifted to the left */
    int m_vbmpOffset; /**< How much the graphics (bitmap) buffer has been shifted down */
    int m_vscrollFrameHeigth;
    int m_hscrollFrameWidth;
    QRect m_vscrollFrame; /**< size of the scroll frame on the right */
    QRect m_vscrollSlider; /**< the scroll slider itself */
    bool m_vscrollSliderGlue;
    int m_vscrollSliderGlueOffset; /**< In pixels, where on the slider did the slider movement start... maintain this
                                    * offset */
    bool m_vscrollBitmapEnabled; /**< If the scroll bug should be seen */
    bool m_vscrollBitmapForce; /**< If the scroll bug is forced to be seen */
    int m_vscrollBitmapTimerID; /**< Ensure that m_vscrollBitmap is shown just a short duration after it is not longer
                                 * required */
    bool m_mouseTracking; /**< While tracking off screen mouse with l-mouse pressed (glue) */
    QRect m_hscrollSlider;
    QRect m_hscrollFrame; /**< size of the scroll frame on the right */
    bool m_hscrollSliderGlue;
    int m_hscrollSliderGlueOffset; /**< In pixels, where on the slider did the slider movement start... maintain this
                                    * offset */
    int m_hscrollSliderWidth;
    bool m_captureOn;
    bool m_dragEnabled; /**< true when an already selected line is clicked again, false at LBUTTON UP */
    bool m_dragOngoing; /**< true when there is an ongoing drag */

#define SCREEN_POINT_HISTORY_LENGTH         2

    ScreenPoint_t m_screenPointHistory[SCREEN_POINT_HISTORY_LENGTH];
    int m_screenPointWritePos;
    bool m_isDebuggingSelection;
    QRect m_debuggingSelection;
    std::unique_ptr<QBrush> m_bgBoarderBrush_p;
    std::unique_ptr<QPen> m_bgBoarderPen_p;
    std::unique_ptr<QBrush> m_bgBrush_p;
    std::unique_ptr<QPen> m_bgPen_p;
    std::unique_ptr<QBrush> m_scrollFrameBrush_p;
    QImage m_bitmap_top;
    QImage m_bitmap_bottom;
    QImage m_bitmap_left;
    QImage m_bitmap_right;
    QImage m_bitmap_right_top;
    QImage m_bitmap_right_bottom;
    QImage m_bitmap_left_top;
    QImage m_bitmap_left_bottom;
    QImage m_bitmap_bug;
    QImage m_bitmap_bug_horiz;
    QImage m_bmp_bookmark_48_36;
    int m_bookmarkWidth;
    int m_decodedRectWidth;
    QRect m_vScrollBMP_Rect;
    bool m_bitmapsLoaded;
    double m_relPos; /**< Relative Y position, is the center of the scroll slider in relative coordinates
                      * 0-1 */
    double m_min_rel_pos; /**< The minimum value that m_relPos may take */
    double m_max_rel_pos; /**< The max value that m_relPos may take */
    int m_min_vslider_center_pos; /**< The minimum value that the center of the vscroll slider may take */
    int m_max_vslider_center_pos; /**< The maximum value that the center of the vscroll slider may take */
    int m_min_topLine;
    int m_max_topLine;
    double m_hrelPos;
    int m_topLine; /**< the TIA index at the of top of the textWindow */
    LogScrutinizerView_ScreenRow_t m_screenRows[LOG_SCRUTINIZER_MAX_SCREEN_ROWS];
    int m_maxColWidth;
    int m_maxColWidthRow;
    int m_maxDisplayRows; /**< The number of rows that we where able to fit inside the window, that includes the
                           * rows for head and footer */
    int m_numOfScreenRows; /**< The current number of rows fitted on screen */
    int m_numOfRowAdjustments; /**< Number of rows that shall have its size increase by one pixel (to align such that
                                * all rows are displayed) */
    int m_rowAdjustment; /**< Number of pixels to adjust per row */
    int m_numOf_1_PixelBonus; /**< Besides the m_numOfRowAdjustments * m_rowAdjustment, add +1 pixel to
                               * m_numOf_1_PixelBonus number of rows */
    int m_totalNumOfRows; /**< This number of rows that may be in total displayed, compensated depending on the
                           * display mode and row clips */
    int m_minRowIndex; /**< The first possible row index, compensated for presentation mode and rowClipping */
    int m_maxRowIndex; /**< The last possible row index, compensated for presentation mode and rowClipping */
    int m_minFIRAIndex; /**< The last FIRA index that can be used considering presentation mode and rowClipping */
    int m_maxFIRAIndex; /**< The last FIRA index that can be used considering presentation mode and rowClipping */
    int m_textRectOffset_Y; /**< Since the text is positioned at the top of the RECT, we move the text
                             * placement compared to where the selection rect will be */
    QFont m_FontStatusBar;
    QFont m_FontEmptyWindow;
    bool m_init;
    bool m_rockScroll_Valid; /**< If false the rock scroll needs to be updated the next OnDraw */
    QImage m_rockScroll_Filtered; /**< Used when the displayed text is showing filtered lines only */
    QImage m_rockScroll_All; /**< Used when displaying all lines */
    RockScrollInfo_t m_rockScrollInfo;
    int m_log_x;
    int m_log_y;
    bool m_drawTimesZero;
    qint64 m_maxDrawTime_ns;
    qint64 m_minDrawTime_ns;
    qint64 m_currDrawTime_ns;
    qint64 m_TextItemSetupTime_ns;
    bool m_SHIFT_Pressed;
    bool m_CTRL_Pressed;
    bool m_ALT_Pressed;
    bool m_LMousePressed;
    QList<CSelection *> m_selectionList;
    bool m_lastSelectionValid;
    CSelection m_lastSelection;
    bool m_multiSelectionActive;
    CSelection m_multiSelection; /**< row is where it started from */
    bool m_pendingSingleSelection;
    CSelection m_singleSelection;
    bool m_dragSelectionOngoing; /**< When selection is done by draging mouse over text */
    CSelection m_origDragSelection;
    LogScrutinizerView_PresentaiontMode_t m_presentationMode;
    FontItem_t *m_blackFont_p;
    FontItem_t *m_whiteFont_p;
    FontItem_t *m_grayFont_p;
    FontItem_t *m_clippedFont_p;
    LS_Painter *m_painter_p; /**< Used for double buffering */
    char m_tempStrHead[16];
    char m_tempStr[CFG_TEMP_STRING_MAX_SIZE];
    bool m_EraseBkGrndDisabled;
    int m_fontHeigth;
    int m_fontWidth;
    int m_rowHeigth;
    QImage m_colClip_Bitmap_ClipStart;
    QImage m_colClip_Bitmap_ClipEnd;
    QRect m_colClip_StartRect; /**< The Rect describing where the Clip start bitmap is located */
    QRect m_colClip_EndRect; /**< The Rect describing where the Clip end bitmap is located */
    int m_tabSize;
    QList<QTextOption::Tab> m_tabStops; /* injected into m_textOption when changed */
    int m_tabStopsArray[NUM_TAB_STOBS]; /* Same as m_tabStops but as int array */
    QTextOption m_textOption;
    bool m_cursorActive;
    bool m_cursorToggleVisible; /**< The timer toggle of the cursor */
    CSelection m_cursorSel;
    int m_cursorDesiredCol; /**< In-case the user marked the end col of a row... end row shall be kept if
                             * possible when scrolling */

    /* When switching presentation mode the this member is used for easy switch back. So if user is stepping in
     * text without filter, and then switch to filter mode, and back then cursor should be located based on this. */
    int m_cursorToggleBackToAllRow = -1; /* -1 if not valid */
    QRect m_cursorRect;
    bool m_AutoHighlight_Enabled; /* True when there is an valid AutoHighlight */
    CSelection m_AutoHighlightSelection;
    int m_numOfHighLights; /* current number of visible highlights */
    bool m_inFocus; /* True when the textView has the focus */
    bool m_focusLostPreviously; /* Used to avoid that reselections on "dimmed" rows causes them */
    Qt::CursorShape m_cursorShape = Qt::ArrowCursor; /* Current mouse cursor ( e.g. I-beam ) */
    char m_keys[256];

public:
    void Initialize_0(void); /* The previous contstructor for CLogScrutinizerView */
    void Initialize(void); /* Clean all members */
    void CleanUp_0(void);

    void EmptySelectionList(bool invalidate = false);     /* Remove all selections */
    void SelectionUpdated(CSelection *selection_p = nullptr);
    bool isRowReselected(int row, int startCol, CSelection **selection_pp); /* Ask if this row is selected */
    bool isRowSelected(int row, CSelection **selection_pp);

    QSize GetTabbedSize(const char *start_p, int size, const QFont *font_p);
    QSize GetTabbedSize(const QString& string, const QFont *font_p);
    void UpdateTextOption(void);

    void DrawTextPart(const QRect *screenRowRect_p, const QString& text, int startCol, int length,
                      const QFont *font_p);

    void InvalidateRockScroll(void);

    bool GetClosestFilteredRow(int startRow, bool up, int *row_p);
    bool SearchFilteredRows_TIA(int startRow, int count, bool up, int *row_p, int *remains_p = nullptr);

    void LimitTopLine(void);
    void UpdateRelPosition(void);
    void CheckRelPosition(void);

    bool isRowVisible(const int row);
    bool isCursorVisible(void);
    int GetScreenRowOffset(const int row);

    void UpdateCursor(bool isOnDraw, int row = CURSOR_UPDATE_NO_ACTION,
                      int startCol = CURSOR_UPDATE_NO_ACTION);

    void DisableCursor(void);

    bool RowColumnToRect(int row, int screenCol, QRect *rect_p);

    void UpdateTotalNumRows(void);

    void SetupScreenProperties_Step0(void);
    void SetupScreenProperties_Step1(void);
    void SetupScreenProperties_Step2(void);

    bool OutlineScreenRows(void);

    bool ContainsTabs(const char *text_p, const int textSize);
    void DrawModifiedFontRow(FontModification_RowInfo_t *fontModRowInfo_p,
                             QRect *rect_p,
                             const char *text_p,
                             const int textSize,
                             CFilterItem *filterItem_p);

    void Refresh(bool doUpdate = true); /**< Refresh all member variables, especially those dependent on values from
                                         * the CDocument */

    void OnDraw(void);  /* overridden to draw this view */
    void FillWindowWithConstData(const struct info_text_elem *text_elems, const int byte_size);
    void FillEmptyWindow(void);
    void FillNoRowsToShow(void); /* Used when there are rows in the log, but the display mode
                                  * (or something else prevent anything to be shown */
    void FillScreenRows(void);
    void FillScreenRows_Filtered(void);

    void DrawRows(void);
    void DrawWindow(void);
    void DrawTextWindow(void);
    void DrawColumnClip();
    void DrawScrollWindow(void);
    void DrawDebugWindow(void); /* Shows a window on-top of everything with some realtime info */
    void DrawBookmark(QRect *rect_p);

    void SearchNewTopLine(bool checkDisplayCache = true, int focusRow = -1);
    bool RowExist_inScreen(const int row); /**< to check if the row is already present on the screen... then
                                            * e.g. no reason for scrolling */
    bool SearchFilteredRows_inScreen(int startRow, int count, bool up, int *row_p);
    int SuggestedFocusRow(void);

    bool GetCursorDisplayRow(int *displayRow_p);

    void GotoTop(void);
    void GotoBottom(void);
    void GotoRow(void);
    void HorizontalCursorFocus(HCursorScrollAction_e scrollAction);
    bool ForceVScrollBitmap(void); /* This function makes the bug scroll icon be shown for a short while, e.g. when the
                                    * window gets focus. It will the be light up for e.g. 1 */
    void UpdateRelTopLinePosition(void);

    void CheckVScrollSliderPosition(void);

    void RockScrollRasterToRow(int raster, int *startRow_p, int *endRow_p);

    void ModifyFontSize(int increase);  /* 1, increase, 0 restore, -1 decrease */

    void TogglePresentationMode(void);
    void SetPresentationMode(LogScrutinizerView_PresentaiontMode_t mode);
    LogScrutinizerView_PresentaiontMode_t GetPresentationMode(void) {return m_presentationMode;}

    bool AddFilterItem(void);

    void RemoveSelection(int row);
    void AddSelection(int TIA_Row, int startCol, int endCol, bool updateLastSelection, bool updateMultiSelection,
                      bool updateCursor = true, bool cursorAfterSelection = true);
    void UpdateSelection(int TIA_Row, int startCol);
    bool GetActiveSelection(CSelection *selection_p); /* Returns the "last" selection */
    bool GetActiveSelection(CSelection *selection_p, QString& text);
    bool GetCursorPosition(CSelection *selection_p); /* Fill selection with cursor position, if active return true,
                                                      * otherwise return false */
    void ContinueSelection(int TIA_selectedRow, int startCol, int endCol);
    void AddMultipleSelections(const int startRow, const int endRow, bool usePresentationMode = true);
    void ExpandSelection(CSelection *selection_p);
    CSelection GetCursorPosition(void) {return m_cursorSel;}
    bool GetSelectionRect(CSelection *selection_p, QRect *rect_p);
    void EmptySelectionListAbove(CSelection *selection_p);
    void EmptySelectionListBelow(CSelection *selection_p);

    void AddDragSelection(int TIA_Row, int startCol, int endCol);
    bool textWindow_SelectionUpdate(ScreenPoint_t *screenPoint_p,
                                    LS_LG_EVENT_t mouseEvent,
                                    Qt::KeyboardModifiers modifiers);

    /* QT_TODO  Should * use QtByteArray * instead */
    bool CopySelectionsToSharedFile(QString sharedFile /*CSharedFile* sf_p*/, int *bytesWritten_p);
    bool CopySelectionsToMem(char *dest_p, int *bytesWritten_p);
    bool SaveSelectionsToFile(QString& fileName);
    QString GetSelectionText(void);

    int GetSelectionsTextSize(void);
    int GetSelectionsTextMaxSize(void);
    void SelectionsToClipboard(void);

    int CursorTo_TIA_Index(int screenRow);
    bool SetVScrollGlue(bool enabled);
    bool SetVScrollBitmap(bool enabled);

    bool PointToCursor(ScreenPoint_t *screenPoint_p, int *screenRow_p, int *screenCol_p,
                       bool *overHalf = nullptr);
    bool PointToColumn(ScreenPoint_t *screenPoint_p, int *screenCol_p, QPoint *alignedPoint_p);
    bool ColumnToPoint(int screenCol, int& x);

    void mainWindowIsMoving(QMoveEvent *event);

    void setAdaptWindowSize(QSize& size) {m_adaptWindowSize = size;}

    void storeSettings(void);
    void UpdateGrayFont(void); /* Re-create the gray font based on m_log_GrayIntensity */

    void PageUpDown(bool up, int lines = 0);
    void onHome(void);
    void onEnd(void);
    void CursorUpDown(bool up);
    void CursorLeftRight(bool left);
    void PageLeftRight(bool left, int charsToScroll = 0);

    void SetRowClip(bool isStart, int row = -3);

    void ToggleBookmark(void);
    void NextBookmark(bool backward = false);

    void SetPlotCursor(void); /* take current selection and tries to set a cursor in the plot windows */

    void FillRockScroll(void);
#ifdef _DEBUG
    void CheckRockScroll(void);
#endif
    void AlignRelPosToRockScroll(int vscrollPos); /* Used to modify the topLine in order to have the
                                                   * colored match in the rockscroll in the middle of the
                                                   * screen */

    void OnLButtonDown(Qt::KeyboardModifiers modifiers, ScreenPoint_t& screenPoint);
    void OnLButtonUp(ScreenPoint_t& screenPoint);
    void OnRButtonDown(Qt::KeyboardModifiers modifiers, ScreenPoint_t& screenPoint);

    void ProcessMouseMove(QMouseEvent *event, ScreenPoint_t& screenPoint);
    bool isMouseCursorMovingRight(void);
    void OnMouseWheel(QWheelEvent *event);
    void SaveMouseMovement(ScreenPoint_t *screenPoint_p);

    ScreenPoint_t m_contextMenuScreenPoint; /* This is a temp. var which indicate where the context menu was started. It
                                             * is used by the menu actions just after */

    void SetFocusRow(int row, int cursorCol = 0);
    void Filter(void);

protected:
    virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    virtual void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void keyReleaseEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent *event) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    virtual void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    virtual QSize sizeHint() const Q_DECL_OVERRIDE;

public slots:
    void OnAutoHighlightTimer(void);
    void OnCursorTimer(void);
    void HandleVScrollBitmapTimer(void);
    void OnColClipStartSet(void);
    void OnColClipDisable(void);
    void OnColClipEndSet(void);

    void OnSave(void);
    void OnSearch(void);
    void OnCopy(void);
    void OnBookmarkToggle(void);
    void OnFilterItemDisable(void);
    void OnFilterItemProperties(void);
    void OnFilterItemAdd(void);

    void OnFilter(void);
    void OnPresentationToggle(void);
    void OnPlotCursorSet(void);
    void OnGotoRow(void);
    void OnRowClipDisable(void);
    void OnRowClipStartSet(void);
    void OnRowClipEndSet(void);
    void OnRowClipGotoStart(void);
    void OnRowClipGotoEnd(void);

    void OnFilterFileOpen(void);
    void OnPluginFileOpen(void);
    void OnLogFileOpen(void);
    void OnFileSaveSelectionsAs(void);
};

extern CEditorWidget *g_editorWidget_p;

#endif /* CEDITORWIDGET_H */
