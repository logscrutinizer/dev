/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QEvent>
#include <QApplication>
#include <QFileDialog>
#include <QTime>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QMimeData>

#include "CDebug.h"
#include "CConfig.h"
#include "globals.h"
#include "utils.h"

#include "ceditorwidget.h"
#include "CLogScrutinizerDoc.h"
#include "CWorkspace.h"
#include "CWorkspace_cb_if.h"
#include "cplotpane_cb_if.h"
#include "CProgressCtrl.h"
#include "CProgressDlg.h"
#include "mainwindow_cb_if.h"
#include "CConfigurationCtrl.h"

#include "cplotpane.h"

extern CLogScrutinizerDoc *GetDocument(void);

typedef enum {
    VIEW_CURSOR_BEAM_e,
    VIEW_CURSOR_ARROW_e
} VIEW_CURSOR_en;

struct info_text_elem {
    char text[256];
    double y_rel;
};
const struct info_text_elem welcome[] =
{
    {"Welcome to LogScrutinizer", 1.0},
    {"(1.) Open, or drag and drop, a text log into the main window", 1.5},
    {"(2.) Create one or several filters by pressing Ctrl-N", 1.0},
    {"(3.) Press F5 to filter your text file with your new filters", 1.0},
    {"(4.) Press F4 to toggle between viewing only filter matches", 1.0},
    {"Read more at www.logscrutinizer.com. Press F1 for quick help web page", 1.5}
};
const struct info_text_elem nothing_to_show[] =
{
    {"LogScrutinizer has currently no text rows to show?", 1.5}
};
const struct info_text_elem pending_update[] =
{
    {"Window being updated, pending...", 1.5}
};
const struct info_text_elem internal_error[] =
{
    {"LogScrutinizer is experiencing internal error, please save workspace and restart", 1.5}
};
const struct info_text_elem log_file_empty[] =
{
    {"Log file empty ...", 1.5}
};

/*
 *  TOPLINE
 *
 *  m_topLine points at the row index that is shown at first line
 *
 *  m_relPos
 *  - Points to where the center is of the slider
 *  - Points to the middle row of the currently prented. This will yeild that the slider in the rockScroll covers the
 *    lines shown
 *
 *
 *  The center of the slider will never reach the top or bottom window. It will as closest become  topPixel +
 * sliderSize/2,
 *  and bottomPixel - sliderSize/2.
 *
 *  Hence, the m_relPos will never reach the endPoints 0% and 100%, as it must represent what is in the middle
 *
 *
 *  Example: we have a 10 "dotted" sized log, and we may present one dot at the screen
 *
 *  The max top line is then: 9
 *  The m_relPos at that time is: 9.5dot  or
 *
 *  m_relPos min:  0.5dot,
 *        max:  9.5dot
 *
 *  m_topLine = m_relPos * (m_totalNumOfRows - 1) - m_maxDisplayRows/2;
 *  m_relPos  = (m_topLine + m_maxDisplayRows/2) / (m_totalNumOfRows - 1)
 *
 *
 *  Slider...
 *
 *  The size of the slider shall be in relation to m_maxDisplayRows and m_totalNumOfRows
 *
 *  Further, just as will m_relPos, the slider center can never reach the bottom of the screen
 *
 *  The translation must be that when m_relPos reach is max value the slider bottom reach the bottom of the sliderbar.
 *  And when m_relPos reach its mininum value the slider top reaches the sliderBar top.
 *
 *  The slider center and the m_relPos shall be aligned.
 *
 *  sliderCenter will hence move between sliderFrameTop + sliderSize/2 -> sliderFrameBottom - sliderSize/2
 *
 *  To translate m_relPos to slider you first must translate m_relPos to 0 - 100% of its range, and then move that to
 * the
 *  slider
 *
 *  translateToFullRange()
 *
 *  FullRangeRelPos = (m_relPos - relPosMin) / (relPosMax - relPosMin)             -> 0 - 1
 *
 *  SliderMin, is the center point of the slider when the top is at SliderFrameTop, SliderFrameTop + SliderSize/2
 *  SliderMax, is the cetner point of the slider when the top is at SliderFrameTop, SliderFrameBottom - SliderSize/2
 *
 *  RelativeCoord: ((SliderMax - SliderMin) * FullRange) + SliderMin
 *
 *  FullRangeSlider = (SliderMiddle - SliderMin) / (SliderMax - sliderMin)
 *
 *  SliderMiddle  = FullRangeSlizer * (SliderMax - SliderMin) + SliderMin
 *  SliderTop     = SliderMiddle - SliderSize/2
 *  SliderBottom  = SliderMiddle + SliderSize/2
 *
 *
 */

/* m_relPos doesn't range from 0 - 1, as m_relPos describe the center of the vertical scroll slider.
 * The macro RELPOS_NORMALIZED translates m_relPos to 0-1, compensating for the start and end point of m_relPos.
 * (m_max_rel_pos - m_min_rel_pos) ... seems to be 1.0 ???
 * normalized m_relPos to 0-1 */
#define RELPOS_NORMALIZED   (((m_relPos - m_min_rel_pos) / (m_max_rel_pos - m_min_rel_pos)))

/***********************************************************************************************************************
*   Initialize_0
***********************************************************************************************************************/
void CEditorWidget::Initialize_0(void)
{
    m_textWindow = QRect(0, 0, 0, 0);
    m_vscrollFrame = QRect(0, 0, 0, 0);
    m_vscrollSlider = QRect(0, 0, 0, 0);

    m_bitmapsLoaded = false;
    m_onDrawCount = 0;
    m_inFocus = false;

    m_maxColWidth = 0;
    m_maxColWidthRow = 0;

    bool success = true;

    m_bitmap_top = QImage(":IDB_BRUSH_TOP_BMP");
    if (m_bitmap_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_TOP_BMP load FAILED")
    }

    m_bitmap_bottom = QImage(":IDB_BRUSH_BOTTOM_BMP");
    if (m_bitmap_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_BOTTOM_BMP load FAILED")
    }

    m_bitmap_left = QImage(":IDB_BRUSH_LEFT_BMP");
    if (m_bitmap_left.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_LEFT_BMP load FAILED")
    }

    m_bitmap_right = QImage(":IDB_BRUSH_RIGHT_BMP");
    if (m_bitmap_right.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BRUSH_RIGHT_BMP load FAILED")
    }

    m_bitmap_left_top = QImage(":IDB_BRUSH_LEFT_TOP_BMP");
    if (m_bitmap_left_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  "
                 "Bitmap IDB_BRUSH_LEFT_TOP_BMP load FAILED")
    }

    m_bitmap_left_bottom = QImage(":IDB_BRUSH_LEFT_BOTTOM_BMP");
    if (m_bitmap_left_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap "
                 "IDB_BRUSH_LEFT_BOTTOM_BMP load FAILED")
    }

    m_bitmap_right_bottom = QImage(":IDB_BRUSH_RIGHT_BOTTOM_BMP");
    if (m_bitmap_right_bottom.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  "
                 "Bitmap IDB_BRUSH_RIGHT_BOTTOM_BMP load FAILED")
    }

    m_bitmap_right_top = QImage(":IDB_BRUSH_RIGHT_TOP_BMP");
    if (m_bitmap_right_top.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  "
                 "Bitmap IDB_BRUSH_RIGHT_TOP_BMP load FAILED")
    }

    m_bitmap_bug = MakeTransparantImage(":IDB_BUG_BITMAP_24", Q_RGB(0x7C, 0xFC, 0x00));
    if (m_bitmap_bug.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BUG_BITMAP_24 load FAILED")
    }

    double scale = static_cast<double>(g_cfg_p->m_scrollArrowScale) / 100.0;
    auto temp = MakeTransparantImage(":IDB_RIGHT_ARROW", Q_RGB(0x7C, 0xFC, 0x00));
    m_bitmap_bug_horiz = temp.scaled(static_cast<int>(static_cast<double>(temp.width() * scale)),
                                     static_cast<int>(static_cast<double>(temp.height() * scale)),
                                     Qt::KeepAspectRatio);

    if (m_bitmap_bug.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_RIGHT_ARROW load FAILED")
    }
#if 0
    m_bitmap_bug_horiz = MakeTransparantImage(":IDB_BUG_BITMAP_HORIZ_24", Q_RGB(0x7C, 0xFC, 0x00));
#endif

    m_bmp_bookmark_48_36 = QImage(":IDB_BOOKMARK_48_36");
    if (m_bmp_bookmark_48_36.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_BOOKMARK_48_36 load FAILED")
    }

    m_colClip_Bitmap_ClipStart = MakeTransparantImage(":IDB_CLIP_START", Q_RGB(0xff, 0xff, 0xff));
    if (m_colClip_Bitmap_ClipStart.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_CLIP_START load FAILED")
    }

    m_colClip_Bitmap_ClipEnd = MakeTransparantImage(":IDB_CLIP_END", Q_RGB(0xff, 0xff, 0xff));
    if (m_colClip_Bitmap_ClipEnd.isNull()) {
        TRACEX_W("CEditorWidget::CEditorWidget  Bitmap IDB_CLIP_END load FAILED")
    }

    m_bitmapsLoaded = success;

    m_bgBoarderBrush_p = std::make_unique<QBrush>(BOARDER_BACKGROUND_COLOR);
    m_bgBoarderPen_p = std::make_unique<QPen>(BOARDER_BACKGROUND_COLOR);
    m_bgBrush_p = std::make_unique<QBrush>(BACKGROUND_COLOR);
    m_bgPen_p = std::make_unique<QPen>(BACKGROUND_COLOR);
    m_scrollFrameBrush_p = std::make_unique<QBrush>(SCROLLBAR_FRAME_COLOR);

    Initialize();

    /* Test the pixel stamps */

    uint64_t startx = 0xfefe;
    uint64_t starty = 0xefef;
    uint64_t font = 0x55;
    uint64_t pixelStamp = 0;

    PIXEL_STAMP_SET_STARTX(pixelStamp, startx);
    PIXEL_STAMP_SET_STARTY(pixelStamp, starty);
    PIXEL_STAMP_SET_FONT(pixelStamp, font);

    startx = 0xffff;
    starty = 0xffff;
    font = 0xffff;

    startx = PIXEL_STAMP_GET_STARTX(pixelStamp);
    starty = PIXEL_STAMP_GET_STARTY(pixelStamp);
    font = PIXEL_STAMP_GET_FONT(pixelStamp);

    if ((startx != 0xfefe) || (starty != 0xefef) || (font != 0x55)) {
        TRACEX_E("CEditorWidget::CEditorWidget  Pixel stamp startup test FAILED,  "
                 "Internal Error")
    }
}

/***********************************************************************************************************************
*   Initialize
***********************************************************************************************************************/
void CEditorWidget::Initialize(void)
{
    EmptySelectionList();

    m_init = true;
    m_relPos = 0.0;
    m_hrelPos = 0.0;

    m_maxDisplayRows = 0;

    m_drawTimesZero = true;
    m_maxDrawTime_ns = 0.0;
    m_minDrawTime_ns = 0.0;
    m_TextItemSetupTime_ns = 0.0;

    m_topLine = 0;

    m_LMousePressed = false;
    m_CTRL_Pressed = false;
    m_SHIFT_Pressed = false;
    m_ALT_Pressed = false;

    m_vscrollBitmapForce = false;
    m_mouseTracking = false;

    m_rockScroll_Valid = false;

    m_windowCfgChanged = true;
    m_presentationMode = PRESENTATION_MODE_ALL_e;
    m_minRowIndex = 0;
    m_maxRowIndex = 0;

    m_minFIRAIndex = 0;
    m_maxFIRAIndex = 0;

    m_EraseBkGrndDisabled = false;

    m_vscrollSliderGlue = false;
    m_hscrollSliderGlue = false;

    m_captureOn = false;
    m_dragEnabled = false;
    m_dragOngoing = false;
    m_dragSelectionOngoing = false;

    m_numOfScreenRows = 0;

    m_lastSelectionValid = false;
    m_multiSelectionActive = false;
    m_pendingSingleSelection = false;

    m_vbmpOffset = 0;
    m_hbmpOffset = 0;

    m_cursorActive = false;
    m_cursorToggleVisible = false;
    m_cursorSel.row = 0;
    m_cursorSel.endCol = 0;
    m_cursorSel.startCol = 0;
    m_cursorDesiredCol = 0;

    m_maxColWidth = 0;
    m_maxColWidthRow = 0;

    /* Even though it is not used in release, it feels best to have it initialized (in-case it is started to be used
     * for release as well) */
    m_rockScrollInfo.itemArray_p = nullptr;
    m_rockScrollInfo.numberOfItems = 0;

    m_cursorRect = QRect(0, 0, 0, 0);

    m_vscrollBitmapEnabled = false;

    m_focusLostPreviously = false;

    memset(&m_screenPointHistory, 0, sizeof(ScreenPoint_t) * SCREEN_POINT_HISTORY_LENGTH);
    m_isDebuggingSelection = true;

    UpdateTotalNumRows();
    InvalidateRockScroll();

    /* Don't call refresh... */
}

/***********************************************************************************************************************
*   Refresh
***********************************************************************************************************************/
void CEditorWidget::Refresh(bool doUpdate)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    UpdateTotalNumRows();
    InvalidateRockScroll();

    /* Update the calc of longest row */
    m_maxColWidth = 0;
    m_maxColWidthRow = 0;

    doc_p->UpdateTitleRow();
    if (doUpdate) {
        update();
    }
}

/***********************************************************************************************************************
*   CleanUp_0
***********************************************************************************************************************/
void CEditorWidget::CleanUp_0(void)
{
    EmptySelectionList();

    m_cursorTimer->stop();
    m_autoHighLightTimer->stop();

    if (m_rockScrollInfo.itemArray_p != nullptr) {
        free(m_rockScrollInfo.itemArray_p);
    }
}

/***********************************************************************************************************************
*   UpdateTotalNumRows
***********************************************************************************************************************/
void CEditorWidget::UpdateTotalNumRows(void)
{
    /* In presentation mode "ALL"  then rowClip rows shall be seen. In "Filtered" mode the row-clipped parts will not
     * be filtered and hence there is not specific need to update the total number of rows considering row clipping,
     * this should be automagically be fixed by the filtering. */

    CLogScrutinizerDoc *doc_p = GetDocument();

    m_totalNumOfRows = doc_p->m_database.TIA.rows;
    m_minRowIndex = 0;
    m_maxRowIndex = m_totalNumOfRows - 1;
    m_minFIRAIndex = 0;
    m_maxFIRAIndex = doc_p->m_database.FIRA.filterMatches > 0 ? doc_p->m_database.FIRA.filterMatches - 1 : 0;

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        m_totalNumOfRows = m_maxFIRAIndex - m_minFIRAIndex + 1;
    }

#ifdef _DEBUG
    TRACEX_DE("CEditorWidget::UpdateTotalNumRows TIA:%d FIRA:%d Total:%d Min:%d Max:%d "
              "FIRA_min:%d FIRA_max:%d RowClip:%d %d",
              doc_p->m_database.TIA.rows, doc_p->m_database.FIRA.filterMatches,
              m_totalNumOfRows, m_minRowIndex, m_maxRowIndex, m_minFIRAIndex, m_maxFIRAIndex,
              g_cfg_p->m_Log_rowClip_Start, g_cfg_p->m_Log_rowClip_End)
#endif
}

/***********************************************************************************************************************
*   SetupScreenProperties_Step0
***********************************************************************************************************************/
void CEditorWidget::SetupScreenProperties_Step0(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    bool static first = true;
    QRect rcClient;

    if (first) {
        first = false;
    }

    doc_p->m_fontCtrl.SetFont(m_painter_p, m_blackFont_p);  /* just to have a font selected */

    m_fontHeigth = doc_p->m_fontCtrl.GetLogFontLetterHeight();
    m_fontWidth = doc_p->m_fontCtrl.GetLogFontLetterWidth();

    auto newTabSize = m_fontWidth * g_cfg_p->m_default_tab_stop_char_length;
    if (newTabSize != m_tabSize) {
        m_tabSize = newTabSize;
        UpdateTextOption();
    }

    rcClient = m_painter_p->window();

    /* 1. First put the scroll bars in place. These are in the outer regions, before any boarder */
    m_hscrollFrame = QRect(rcClient.left(),
                           rcClient.bottom() - SCROLL_FRAME_THICKNESS,
                           rcClient.width(), /* Full width for the horizontal scroll */
                           SCROLL_FRAME_THICKNESS);

    m_hscrollFrameWidth = m_hscrollFrame.width();
    m_hscrollSlider = m_hscrollFrame; /* left and right is setup later */

    m_rcBoarder.setLeft(rcClient.left());
    m_rcBoarder.setRight(0);        /* This can only be setup when the left most character position is known */
    m_rcBoarder.setTop(rcClient.top());
    m_rcBoarder.setBottom(m_hscrollFrame.top());

    m_vscrollFrame.setRight(rcClient.right());
    m_vscrollFrame.setLeft(rcClient.right() - 20);
    m_vscrollFrame.setBottom(m_hscrollFrame.top());
    m_vscrollFrame.setTop(rcClient.top());
    m_vscrollFrameHeigth = m_vscrollFrame.height();

    m_vscrollSlider = m_vscrollFrame; /* top / bottom is setup later */

    m_rowHeigth = m_fontHeigth + static_cast<int>(m_fontHeigth * CFG_SCREEN_TEXT_ROW_PADDING_persistent.top +
                                                  m_fontHeigth * CFG_SCREEN_TEXT_ROW_PADDING_persistent.bottom);

    m_windowCfgChanged = true;

    /*
     *  TRACEX_DE("CEditorWidget::SetupScreenProperties_Step0   m_vscrollFrame x1:%d "
     *                                       "x2:%d zy1:%d y2:%d  Letter_Y:%d Letter_X:%d tabSize:%d",
     *    m_vscrollFrame.left(), m_vscrollFrame.right(), m_vscrollFrame.top(),
     *                  m_vscrollFrame.bottom(), m_fontHeigth, m_fontWidth, m_tabSize); */
}

/***********************************************************************************************************************
*   SetupScreenProperties_Step1
***********************************************************************************************************************/
void CEditorWidget::SetupScreenProperties_Step1(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    /* Let the text window padding be dependent on the font size */

    m_textWindow.setBottom(m_rcBoarder.bottom());
    m_textWindow.setTop(m_rcBoarder.top());
    m_textWindow.setLeft(m_rcBoarder.left() + m_bitmap_left_top.width());
    m_textWindow.setRight(0); /* Temporary until right pos is known */

    QString testString = QString("%1  ").arg(doc_p->m_database.TIA.rows);
    doc_p->m_fontCtrl.SetFont(m_painter_p, m_blackFont_p);

    QSize oneLetter = doc_p->m_fontCtrl.GetFontSize();
    QSize rowCount_lineSize = GetTabbedSize(testString, m_blackFont_p->font_p);
    auto hightRate = m_rowHeigth / static_cast<double>(m_bmp_bookmark_48_36.height());

    m_bookmarkWidth = static_cast<int>(m_bmp_bookmark_48_36.width() * hightRate);
    m_decodedRectWidth = m_rowHeigth / 2;

    m_textRow_Head_X.setLeft(static_cast<int>(m_textWindow.left() + m_fontWidth *
                                              CFG_SCREEN_TEXT_ROW_PADDING_persistent.left));

    m_textRow_Head_X.setRight(m_textRow_Head_X.left()
                              + rowCount_lineSize.width() + 2 /*bookmark spacing */
                              + m_bookmarkWidth
                              + 2 /*decoded spacing */
                              + m_decodedRectWidth);

    m_textRow_X.setLeft(static_cast<int>(m_textRow_Head_X.right()
                                         + m_fontWidth * CFG_SCREEN_TEXT_ROW_PADDING_persistent.left));

    m_beam_left_padding = oneLetter.width();

    m_rowAdjustment = 1;
    m_numOf_1_PixelBonus = 0;

    m_maxDisplayRows = ((m_textWindow.bottom() - m_textWindow.top()) / m_rowHeigth);

    if (m_maxDisplayRows > m_totalNumOfRows) {
        m_maxDisplayRows = m_totalNumOfRows;
        m_numOfRowAdjustments = 0;
    } else {
        int requiredRowAdjustment = m_textWindow.height() - m_maxDisplayRows * m_rowHeigth;
        m_numOfRowAdjustments = m_textWindow.height() - m_maxDisplayRows * m_rowHeigth;

        if (requiredRowAdjustment > m_maxDisplayRows) {
            /* Not enough to adjust one pixel per row */
            m_rowAdjustment = requiredRowAdjustment / m_maxDisplayRows;
            m_numOfRowAdjustments = m_maxDisplayRows;
            m_numOf_1_PixelBonus = requiredRowAdjustment - m_rowAdjustment * m_maxDisplayRows;
        }
    }

    /* not compensated for having few lines */
    if (m_maxDisplayRows != m_totalNumOfRows) {
        m_min_rel_pos = (m_maxDisplayRows / 2.0) / (m_totalNumOfRows - m_maxDisplayRows);
        m_max_rel_pos = (m_totalNumOfRows - (m_maxDisplayRows / 2.0)) / (m_totalNumOfRows - m_maxDisplayRows);
    } else {
        m_min_rel_pos = 0.5;
        m_max_rel_pos = 0.5;
    }

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        m_min_topLine = doc_p->m_database.packedFIRA_p[0].row;
        m_max_topLine = m_maxFIRAIndex > m_maxDisplayRows ?
                        doc_p->m_database.packedFIRA_p[m_maxFIRAIndex - m_maxDisplayRows + 1].row : m_min_topLine;
    } else {
        m_min_topLine = m_minRowIndex;
        m_max_topLine = m_totalNumOfRows > m_maxDisplayRows ? m_maxRowIndex - m_maxDisplayRows + 1 : m_minRowIndex;
    }

    /* If the fileTracking option is enabled then the last row shall always be printed */
    if (g_cfg_p->m_logFileTracking) {
        m_topLine = m_max_topLine;
        m_relPos = m_max_rel_pos;
    }
}

/***********************************************************************************************************************
*   SetupScreenProperties_Step2
***********************************************************************************************************************/
void CEditorWidget::SetupScreenProperties_Step2(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    QSize lineSize(0, 0);
    char *start_p;
    int size;

    doc_p->GetTextItem(m_maxColWidthRow, &start_p, &size);
    doc_p->m_fontCtrl.SetFont(m_painter_p, m_blackFont_p);

    size = size > DISPLAY_MAX_ROW_SIZE ? DISPLAY_MAX_ROW_SIZE : size;

    lineSize = GetTabbedSize(start_p, size, m_blackFont_p->font_p);

    m_textRow_X.setRight(m_textRow_X.left() + lineSize.width());
    m_textWindow.setRight(static_cast<int>(m_textRow_X.right() +
                                           (m_fontWidth * CFG_SCREEN_TEXT_ROW_PADDING_persistent.right)));
    m_rcBoarder.setRight(m_textWindow.right());

    m_textRectOffset_Y = static_cast<int>((m_rowHeigth / 2.0) - (m_fontHeigth / 2.0));

    m_bmpWindow = m_rcClient;

    if ((m_rcBoarder.right() + m_bitmap_left_top.width()) > m_rcClient.right()) {
        m_bmpWindow.setRight(m_rcBoarder.right() + m_bitmap_left_top.width());
        m_bmpWindow.setRight(m_bmpWindow.right() + 30);
    }

    m_EraseBkGrndDisabled = true;

    if (m_init) {
        m_relPos = m_min_rel_pos;
        m_hrelPos = 0.0;
    }

    int temp_scrollSliderHeight = m_vscrollFrameHeigth; /* fallback */

    if (m_maxDisplayRows > m_totalNumOfRows) {
        temp_scrollSliderHeight = m_vscrollFrameHeigth;
    } else if (m_vscrollFrameHeigth > 0) {
        temp_scrollSliderHeight = static_cast<int>(m_vscrollFrameHeigth *
                                                   (m_maxDisplayRows / static_cast<double>(m_totalNumOfRows)));
    }

    if (temp_scrollSliderHeight < 1) {
        temp_scrollSliderHeight = 1;
    }

    m_min_vslider_center_pos = static_cast<int>(m_vscrollFrame.top() + (temp_scrollSliderHeight / 2.0) - 0.5);
    m_max_vslider_center_pos = static_cast<int>(m_vscrollFrame.bottom() - (temp_scrollSliderHeight / 2.0) + 0.5);

    /* Setup the size of the horizontal scroll slider */

    if (m_hscrollFrameWidth > m_textWindow.width()) {
        m_hscrollSliderWidth = m_hscrollFrameWidth;
    } else {
        m_hscrollSliderWidth = static_cast<int>(m_hscrollFrameWidth *
                                                (m_hscrollFrameWidth / static_cast<double>(m_textWindow.width())));
    }

    LimitTopLine();

    if (m_init) {
        UpdateRelPosition();
        m_init = false;
    }

    /* Setup the size of the vertical scroll slider */

    CheckRelPosition();

    /* Perhaps a little bit of "cake on cake", as m_relPos already describe the middle of the vslider (and have
     * compensation for min and max), it should be enough to take the size of the slider bar and multiply it
     * with m_relPos and then subtract half of the slider size */
    m_vscrollSlider.setTop(static_cast<int>(m_min_vslider_center_pos +
                                            (((m_max_vslider_center_pos - m_min_vslider_center_pos) *
                                              RELPOS_NORMALIZED) -
                                             (temp_scrollSliderHeight / 2.0) + 0.5)));

    if (m_vscrollSlider.top() < 0) {
        m_vscrollSlider.setTop(0);
    }

    m_vscrollSlider.setBottom(m_vscrollSlider.top() + temp_scrollSliderHeight);

    m_hscrollSlider.setLeft(static_cast<int>(m_hscrollFrame.left() +
                                             m_hrelPos * (m_hscrollFrameWidth - m_hscrollSliderWidth)));
    m_hscrollSlider.setRight(m_hscrollSlider.left() + m_hscrollSliderWidth);
}

/***********************************************************************************************************************
*   GetTabbedSize
***********************************************************************************************************************/
QSize CEditorWidget::GetTabbedSize(const char *start_p, int size, const QFont *font_p)
{
    static char dummyString[DISPLAY_MAX_ROW_SIZE];
    size = size >= DISPLAY_MAX_ROW_SIZE ? DISPLAY_MAX_ROW_SIZE : size;
    memcpy(dummyString, start_p, static_cast<size_t>(size));
    dummyString[size] = 0;
    return GetTabbedSize(QString(dummyString), font_p);
}

/***********************************************************************************************************************
*   GetTabbedSize
***********************************************************************************************************************/
QSize CEditorWidget::GetTabbedSize(const QString& string, const QFont *font_p)
{
    QFontMetrics fontMetric(*font_p);
    return fontMetric.size(Qt::TextExpandTabs, string, m_tabSize, m_tabStopsArray);
}

/***********************************************************************************************************************
*   UpdateTextOption
***********************************************************************************************************************/
void CEditorWidget::UpdateTextOption(void)
{
    if (m_tabStops.isEmpty()) {
        /* Only done once initially */
        for (int index = 0; index < NUM_TAB_STOBS; ++index) {
            m_tabStops.append(QTextOption::Tab(index, QTextOption::LeftTab)); /* dummy add */
        }
    }

    int tabStop = m_tabSize - m_textRow_X.left() % m_tabSize;
    unsigned index = 0;
    for (auto& tab : m_tabStops) {
        tab.position = tabStop;
        m_tabStopsArray[index] = tabStop;
        tabStop += m_tabSize;
        index++;
    }

    m_textOption.setTabs(m_tabStops);
}

/***********************************************************************************************************************
* DrawTextPart
* When drawing a part of a text row it is essential that the TAB stops are realigned with the where the text is
* started being drawn.
***********************************************************************************************************************/
void CEditorWidget::DrawTextPart(const QRect *screenRowRect_p, const QString& text, int startCol, int length,
                                 const QFont *font_p)
{
    QList<QTextOption::Tab> alignedTabStops = m_tabStops; /* make a copy to modify */
    const auto MAX_COUNT = alignedTabStops.count();

    if (startCol != 0) {
        QString letterInfront = text.left(startCol);
        QSize size = GetTabbedSize(letterInfront, font_p);
        auto startPixel = size.width();

        /* find where in the original tab list we should start, find the clostest tab index infront of the
         * current pixel position. */
        auto tabIndex = startPixel / m_tabSize;
        if (startPixel > m_tabStops[tabIndex].position) {
            while (tabIndex < MAX_COUNT && startPixel > m_tabStops[tabIndex].position) {
                tabIndex++;
            }
        } else {
            while (tabIndex < MAX_COUNT && tabIndex != 0 && startPixel < m_tabStops[tabIndex].position) {
                tabIndex--;
            }
            if ((tabIndex >= 0) && (tabIndex < (MAX_COUNT - 1)) && (startPixel > m_tabStops[tabIndex].position)) {
                tabIndex++; /* step one back */
            }
        }

        if (tabIndex < MAX_COUNT) {
            /*Now tabIndex is at the one before next tab. Update the alignedTabStops to map to the new start point. */
            auto tabStopPixel = alignedTabStops[tabIndex].position - static_cast<qreal>(startPixel);
            for (auto& tabStop : alignedTabStops) {
                tabStop.position = tabStopPixel;
                tabStopPixel += m_tabSize;
            }
        }

        QTextOption textOption;
        textOption.setTabs(alignedTabStops);

        QString textPart = text.right(text.length() - startCol);
        textPart = textPart.left(length);

        m_painter_p->drawText(QRect(startPixel + screenRowRect_p->left(),
                                    screenRowRect_p->top() + static_cast<int>(m_textRectOffset_Y),
                                    screenRowRect_p->right(),
                                    screenRowRect_p->bottom()),
                              textPart, textOption);

#if 0
        m_painter_p->drawText(QRect(headRect.left(), screenRowRect_p->top() + static_cast<int>(m_textRectOffset_Y),
                                    screenRowRect_p->left(), screenRowRect_p->bottom()),
                              QString("%1").arg(row, 5), m_textOption);
#endif
    } else {
        /* Draw with original tabStops */
        m_painter_p->drawText(QRect(screenRowRect_p->left(),
                                    screenRowRect_p->top() + static_cast<int>(m_textRectOffset_Y),
                                    screenRowRect_p->right(),
                                    screenRowRect_p->bottom()),
                              text.left(length), m_textOption);
    }
}

/***********************************************************************************************************************
*   OnDraw
***********************************************************************************************************************/
void CEditorWidget::OnDraw(void)
{
    QBitmap bitmap;
    QRect rcClient;
    auto doc_p = GetTheDoc();

    m_onDrawCount++;
    rcClient = m_painter_p->window();
    m_rcClient = rcClient;

    if (g_DebugLib->m_isSystemError) {
        FillWindowWithConstData(internal_error, sizeof(internal_error));
        return;
    }

    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        FillWindowWithConstData(pending_update, sizeof(pending_update));
        return;
    }

    if (CSZ_DB_PendingUpdate) {
        /* Very strange, the GUI thread was initiated even though filter/file loading was processing */
        FillWindowWithConstData(pending_update, sizeof(pending_update));
        return;
    }

    const auto rows = doc_p->m_database.TIA.rows;
    if (rows == 0) {
        /* Scenario with no log file */
        FillEmptyWindow();
        return;
    } else if (rows == 1) {
        int size;
        doc_p->m_rowCache_p->GetTextItemLength(0, &size);
        if (size == 0) {
            /* Scenario when file been open for writing, but no content yet */
            FillWindowWithConstData(log_file_empty, sizeof(log_file_empty));
            return;
        }
    }

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        if (m_totalNumOfRows != doc_p->m_database.FIRA.filterMatches) {
            TRACEX_W("OnDraw, row count missmatch, FILTERED Mode total:%d expected:%d",
                     m_totalNumOfRows, doc_p->m_database.FIRA.filterMatches)
        }
    } else if (m_totalNumOfRows != doc_p->m_database.TIA.rows) {
        TRACEX_W("OnDraw, row count missmatch, non-FILTERED Mode total:%d expected:%d",
                 m_totalNumOfRows, doc_p->m_database.TIA.rows)
    }

    if ((rcClient.height() < 20) || (rcClient.width() < 20)) {
        if (doc_p->m_database.TIA.rows < 1) {
            /* since the rest of the screen is otherwise in this color */
            m_painter_p->fillRect(rcClient, QColor(BOARDER_BACKGROUND_COLOR));
        } else {
            /* since the rest of the screen is otherwise in this color */
            m_painter_p->fillRect(rcClient, QColor(SCROLLBAR_FRAME_COLOR));
        }
        return;
    }

    if (doc_p->m_database.TIA.rows < 1) {
        FillEmptyWindow();
        return;
    }

    if (m_totalNumOfRows < 1) {
        FillNoRowsToShow();
        return;
    }

    SetupScreenProperties_Step0();    /* Setup basic parameters such as window sizes and totalMaxRows that could fit */
    SetupScreenProperties_Step1();    /* Sizes the text window */

    if ((!doc_p->m_allEnabledFilterItems.empty() || (g_workspace_p->GetBookmarksCount() > 0)) &&
        (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e)) {
        FillScreenRows_Filtered();      /* Add max number of rows to the screenBuffer */
    } else {
        FillScreenRows();               /* Add max number of rows to the screenBuffer */
    }

    OutlineScreenRows();
    SetupScreenProperties_Step2();

    m_bmpWindow.setBottom(m_bmpWindow.bottom() + 200);

    QImage double_buffer_image(QSize(m_bmpWindow.width(), m_bmpWindow.height()), QImage::Format_ARGB32_Premultiplied);
    LS_Painter painter(&double_buffer_image);
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    LS_Painter *original_painter = m_painter_p;

    m_painter_p = &painter; /* Switch m_painter_p to double buffer */
    doc_p->m_fontCtrl.SetFont(m_painter_p, m_whiteFont_p);
    m_painter_p->setBrush(m_background);

    /* since the rest of the screen is otherwise in this color */
    m_painter_p->fillRect(m_bmpWindow, BOARDER_BACKGROUND_COLOR);

    QRect textWindow = m_textWindow;

    /* since the rest of the screen is otherwise in this color */
    m_painter_p->fillRect(textWindow, QColor(BACKGROUND_COLOR));

    m_hbmpOffset = 0;

    if (rcClient.width() < m_bmpWindow.width()) {
        m_hbmpOffset = static_cast<int>(m_hrelPos * (m_bmpWindow.width() - rcClient.width()));
    }

    /* Fill all the text to the DC */
    DrawWindow();

    if (!m_rockScroll_Valid) {
        FillRockScroll();
        m_rockScroll_Valid = true;
    }

    CheckVScrollSliderPosition();  /* uses rockscroll to verify alignment (as such rockscroll must be done) */

    /* Draw graphics around the window (boarders) TODO: Make it gradient fills instead */

    if (m_bitmapsLoaded) {
        int index;

        /* Left side */
        int count = static_cast<int>((m_rcBoarder.height() / static_cast<double>(m_bitmap_left.height())) + 1.0);
        for (index = 0; index < count; ++index) {
            m_painter_p->drawImage(m_rcBoarder.left(),
                                   m_rcBoarder.top() + index * m_bitmap_left.height(), m_bitmap_left);
        }

        /* Right side */
        count = static_cast<int>(((m_rcBoarder.height()) / static_cast<double>(m_bitmap_right.height())) + 1.0);
        for (index = 0; index < count; ++index) {
            m_painter_p->drawImage(m_rcBoarder.right(), m_rcBoarder.top() +
                                   index * m_bitmap_right.height(), m_bitmap_right);
        }

        /* TODO: The following below isn't really spot on, since there is no shadow at the top and bottom.
         * Now the "paper" goes from top to bottom, hence it looks a bit odd that the shadow bends in
         * over the "paper", even though the paper continues */

        /* Left top */
        m_painter_p->drawImage(m_rcBoarder.left(), m_rcBoarder.top(), m_bitmap_left_top);

        /* Left bottom */
        m_painter_p->drawImage(m_rcBoarder.left(), m_rcBoarder.bottom(), m_bitmap_left_bottom);

        /* Right top */
        m_painter_p->drawImage(m_rcBoarder.right(), m_rcBoarder.top(), m_bitmap_right_top);

        /* Right bottom */
        m_painter_p->drawImage(m_rcBoarder.right(), m_rcBoarder.bottom(), m_bitmap_right_bottom);
    }

    DrawColumnClip();
    DrawScrollWindow();

#ifdef _DEBUG
    CheckRockScroll();
#endif

    if (g_cfg_p->m_devToolsMenu) {
        DrawDebugWindow();
    }

    m_EraseBkGrndDisabled = false;
    UpdateCursor(true);

    original_painter->drawImage(0, 0, double_buffer_image, m_hbmpOffset);
    painter.end();
}

/***********************************************************************************************************************
*   DrawColumnClip
***********************************************************************************************************************/
void CEditorWidget::DrawColumnClip(void)
{
    int x;

    if ((g_cfg_p->m_Log_colClip_Start > CFG_CLIP_NOT_SET) && ColumnToPoint(g_cfg_p->m_Log_colClip_Start, x)) {
        x -= m_colClip_Bitmap_ClipStart.width() / 2; /* Center the tip of the bitmap to the start of the character */
        m_painter_p->drawImage(x, m_rcClient.top(), m_colClip_Bitmap_ClipStart);
        m_colClip_StartRect.setLeft(x);
        m_colClip_StartRect.setRight(x + m_colClip_Bitmap_ClipStart.width());
        m_colClip_StartRect.setTop(m_rcClient.top());
        m_colClip_StartRect.setBottom(m_rcClient.top() + m_colClip_Bitmap_ClipStart.height());
    }

    if ((g_cfg_p->m_Log_colClip_End > CFG_CLIP_NOT_SET) && ColumnToPoint(g_cfg_p->m_Log_colClip_End, x)) {
        x -= m_colClip_Bitmap_ClipEnd.width() / 2;
        m_painter_p->drawImage(x, m_rcClient.top(), m_colClip_Bitmap_ClipEnd);
        m_colClip_EndRect.setLeft(x);
        m_colClip_EndRect.setRight(x + m_colClip_Bitmap_ClipEnd.width());
        m_colClip_EndRect.setTop(m_rcClient.top());
        m_colClip_EndRect.setBottom(m_rcClient.top() + m_colClip_Bitmap_ClipEnd.height());
    }
}

/***********************************************************************************************************************
*   DrawScrollWindow
***********************************************************************************************************************/
void CEditorWidget::DrawScrollWindow(void)
{
    m_painter_p->drawImage(m_vscrollFrame.left() + m_hbmpOffset, m_vscrollFrame.top(), m_rockScroll_All);

    if ((m_topLine > -1) && (m_totalNumOfRows >= m_maxDisplayRows)) {
        /* Bitblit the BUG */

        if (m_vscrollTimer->isActive()) {
            int scrollCenter = m_vscrollSlider.top() + m_vscrollSlider.height() / 2;
            int bugTop = scrollCenter - m_bitmap_bug_horiz.height() / 2;

            /*Format_ARGB32_Premultiplied */
            m_painter_p->drawImage(m_vscrollSlider.left() + m_hbmpOffset - m_bitmap_bug_horiz.width(),
                                   bugTop, m_bitmap_bug_horiz);

            m_vScrollBMP_Rect.setLeft(m_vscrollSlider.left() - m_bitmap_bug_horiz.width());
            m_vScrollBMP_Rect.setRight(m_vScrollBMP_Rect.left() + m_bitmap_bug_horiz.width());
            m_vScrollBMP_Rect.setTop(bugTop);
            m_vScrollBMP_Rect.setBottom(bugTop + m_bitmap_bug_horiz.height());
        }

        if (m_vscrollSliderGlue) {
            m_painter_p->fillRect(m_vscrollSlider.left() + m_hbmpOffset,
                                  m_vscrollSlider.top(),
                                  m_vscrollSlider.width(),
                                  m_vscrollSlider.height(),
                                  SCROLLBAR_SLIDER_SELECTED_COLOR);
        } else {
            m_painter_p->fillRect(m_vscrollSlider.left() + m_hbmpOffset,
                                  m_vscrollSlider.top(),
                                  m_vscrollSlider.width(),
                                  m_vscrollSlider.height(),
                                  SCROLLBAR_SLIDER_COLOR);
        }
    }

    /* Draw the Horizontal scroll Slider */
    m_painter_p->fillRect(m_hscrollFrame.left() + m_hbmpOffset, m_hscrollFrame.top(),
                          m_hscrollFrameWidth, m_hscrollFrame.height(), SCROLLBAR_FRAME_COLOR);

    if (m_bmpWindow.width() > m_rcClient.width()) {
        QRgb color = m_hscrollSliderGlue ? SCROLLBAR_SLIDER_SELECTED_COLOR : SCROLLBAR_SLIDER_COLOR;
        m_painter_p->fillRect(m_hscrollSlider.left() + m_hbmpOffset,
                              m_hscrollSlider.top(),
                              m_hscrollSliderWidth,
                              m_hscrollFrame.height(),
                              color);
    }
}

/***********************************************************************************************************************
*   DrawWindow
***********************************************************************************************************************/
void CEditorWidget::DrawWindow(void)
{
    if (m_relPos > m_max_rel_pos) {
        m_relPos = m_max_rel_pos;
    } else if (m_relPos < m_min_rel_pos) {
        m_relPos = m_min_rel_pos;
    }

    if (m_hrelPos > 1.0) {
        m_hrelPos = 1.0;
    } else if (m_hrelPos < 0.0) {
        m_hrelPos = 0.0;
    }

    m_painter_p->setBackgroundMode(Qt::TransparentMode);
    m_windowCfgChanged = false;
    DrawTextWindow();
}

/***********************************************************************************************************************
*   DrawTextWindow
***********************************************************************************************************************/
void CEditorWidget::DrawTextWindow(void)
{
    static QElapsedTimer e_timer;
    e_timer.restart();

    DrawRows();

    qint64 elapsedTime = e_timer.nsecsElapsed();
    if ((elapsedTime != 0) && !m_drawTimesZero) {
        m_maxDrawTime_ns = m_maxDrawTime_ns < elapsedTime ? elapsedTime : m_maxDrawTime_ns;
        m_minDrawTime_ns = m_minDrawTime_ns > elapsedTime ? elapsedTime : m_minDrawTime_ns;
    } else {
        m_maxDrawTime_ns = elapsedTime;
        m_minDrawTime_ns = elapsedTime;
        m_drawTimesZero = false;
    }
    m_currDrawTime_ns = elapsedTime;
}

/***********************************************************************************************************************
*   FillWindowWithConstData
***********************************************************************************************************************/
void CEditorWidget::FillWindowWithConstData(const struct info_text_elem *text_elems, const int byte_size)
{
    int x_pos = static_cast<int>((m_rcClient.right() - m_rcClient.left()) * 0.1);
    auto y_pos = (m_rcClient.bottom() - m_rcClient.top()) * 0.1;

    m_painter_p->fillRect(m_rcClient, QColor(BOARDER_BACKGROUND_COLOR));
    m_painter_p->setFont(m_FontEmptyWindow);

    QFontMetrics fontMetrics(m_FontEmptyWindow);

    GetTheDoc()->m_fontCtrl.SetFont(m_painter_p, m_whiteFont_p);

    QFontMetrics metrics(m_FontEmptyWindow);
    const int offset = metrics.height() * 2;

    for (int i = 0; i < byte_size / static_cast<int>(sizeof(struct info_text_elem)); ++i) {
        y_pos += offset * text_elems[i].y_rel;
        m_painter_p->drawText(QPoint(x_pos, static_cast<int>(round(y_pos))), QString(text_elems[i].text));
    }
}

/***********************************************************************************************************************
*   FillEmptyWindow
***********************************************************************************************************************/
void CEditorWidget::FillEmptyWindow(void)
{
    FillWindowWithConstData(welcome, sizeof(welcome));
}

/***********************************************************************************************************************
*   FillNoRowsToShow
***********************************************************************************************************************/
void CEditorWidget::FillNoRowsToShow(void)
{
    FillWindowWithConstData(nothing_to_show, sizeof(nothing_to_show));
}

/***********************************************************************************************************************
*   FillScreenRows
***********************************************************************************************************************/
void CEditorWidget::FillScreenRows(void)
{
    int index;
    bool visible;
    auto doc_p = GetDocument();
    bool filteringEnabled = doc_p->m_allEnabledFilterItems.isEmpty() ? false : true;
    int index_toplined = m_topLine;

    m_numOfScreenRows = 0;

    /* clean the screenRows */
    memset(&m_screenRows[0], 0, sizeof(LogScrutinizerView_ScreenRow_t) * LOG_SCRUTINIZER_MAX_SCREEN_ROWS);

    for (index = 0; index_toplined < m_totalNumOfRows && index < m_maxDisplayRows; ++index_toplined) {
        m_screenRows[index].valid = true;
        visible = false;

        if (index_toplined < 0) {
            /* Drawing the head frame */
            visible = true;
            m_screenRows[index].presentation = ROW_PRESENTATION_HEAD_FRAME_e;
        } else if (index_toplined >= doc_p->m_database.TIA.rows) {
            /* Drawing the bottom frame */
            visible = true;
            m_screenRows[index].presentation = ROW_PRESENTATION_FOOTER_FRAME_e;
        } else if (doc_p->m_database.FIRA.FIR_Array_p[index_toplined].LUT_index == BOOKMARK_FILTER_LUT_INDEX) {
            visible = true;

            m_screenRows[index].presentation = ROW_PRESENTATION_BOOKMARKED_e;
        } else if (!filteringEnabled) {
            visible = true;

            if (doc_p->isRowClipped(index_toplined)) {
                m_screenRows[index].presentation = ROW_PRESENTATION_CLIPPED_e;
            } else {
                m_screenRows[index].presentation = ROW_PRESENTATION_NORMAL_e;
            }
        } else if ((doc_p->m_rowCache_p->GetFilterRef(index_toplined) != nullptr) &&
                   !doc_p->m_rowCache_p->GetFilterRef(index_toplined)->m_exclude) {
            /* check that it is not excluded */
            visible = true;
            if (doc_p->isRowClipped(index_toplined)) {
                m_screenRows[index].presentation = ROW_PRESENTATION_CLIPPED_e;
            } else {
                m_screenRows[index].presentation = ROW_PRESENTATION_FILTERED_e;
            }
        } else if (m_presentationMode == PRESENTATION_MODE_ALL_e) {
            visible = true;

            if (doc_p->isRowClipped(index_toplined)) {
                m_screenRows[index].presentation = ROW_PRESENTATION_CLIPPED_e;
            } else {
                m_screenRows[index].presentation = ROW_PRESENTATION_GRAYED_e;
            }
        }

        if (visible) {
            m_screenRows[index].row = index_toplined;
            ++index;
        }
    }
}

/***********************************************************************************************************************
*   FillScreenRows_Filtered
***********************************************************************************************************************/
void CEditorWidget::FillScreenRows_Filtered(void)
{
    auto doc_p = GetDocument();

    m_numOfScreenRows = 0;

    /* clean the screenRows */
    memset(&m_screenRows[0], 0, sizeof(LogScrutinizerView_ScreenRow_t) * LOG_SCRUTINIZER_MAX_SCREEN_ROWS);

    if ((m_totalNumOfRows == 0) && (g_workspace_p->GetBookmarksCount() == 0)) {
        return;
    }

    /* First ensure the current topline is actually a filtered line */

    int new_startRow;

    /* m_totalNumOfRows is compensated with the number of filter matches there are considering rowClips as well */
    if (m_totalNumOfRows > 0) {
        if (!GetClosestFilteredRow(m_topLine, true, &new_startRow)) {
            if (!GetClosestFilteredRow(m_topLine, false, &new_startRow)) {
                TRACEX_W("FillScreenRows_Filtered   No filters found when trying to fill rows")
                return;
            }
        }
    }

    LimitTopLine(); /* Precaution */

    int packed_FIRA_Index = doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index;
    const int Max_FIRA_Index = m_maxFIRAIndex;
    int index;

    for (index = 0; packed_FIRA_Index <= Max_FIRA_Index && index < m_maxDisplayRows; ++index, ++packed_FIRA_Index) {
        int row = doc_p->m_database.packedFIRA_p[packed_FIRA_Index].row;

        m_screenRows[index].valid = true;
        m_screenRows[index].row = row;

        if (doc_p->m_database.FIRA.FIR_Array_p[row].LUT_index == BOOKMARK_FILTER_LUT_INDEX) {
            m_screenRows[index].presentation = ROW_PRESENTATION_BOOKMARKED_e;
        } else if (doc_p->isRowClipped(row)) {
            m_screenRows[index].presentation = ROW_PRESENTATION_CLIPPED_e;
        } else {
            m_screenRows[index].presentation = ROW_PRESENTATION_FILTERED_e;
        }
    }
}

/***********************************************************************************************************************
*   OutlineScreenRows
***********************************************************************************************************************/
bool CEditorWidget::OutlineScreenRows(void)
{
    QRect lineRect;
    auto doc_p = GetDocument();
    int index = 0;
    int numOfRowAdjustments = m_numOfRowAdjustments;
    int rowAdjustment = m_rowAdjustment;
    int numOf_1_PixelBonus = m_numOf_1_PixelBonus;

    lineRect.setTop(m_textWindow.top());
    lineRect.setLeft(m_textRow_X.left());
    lineRect.setRight(0);  /* will be set when drawing the line */
    lineRect.setBottom(lineRect.top() + m_rowHeigth);

    char *text_p;

    while (m_screenRows[index].valid) {
        /* The outline remove the lines that cannot fit in the bottom */
        if (lineRect.top() < m_textWindow.bottom()) {
            int textLength;

            m_screenRows[index].screenRect = lineRect;
            doc_p->m_rowCache_p->Get(m_screenRows[index].row, &text_p, &textLength, nullptr);

            QSize textTabbedLength = GetTabbedSize(text_p, textLength, m_blackFont_p->font_p);

            if (textTabbedLength.width() > m_maxColWidth) {
                m_maxColWidth = textTabbedLength.width();
                m_maxColWidthRow = m_screenRows[index].row;
            }

            lineRect.setTop(lineRect.bottom());
            lineRect.setBottom(lineRect.top() + m_rowHeigth);

            if (numOfRowAdjustments > 0) {
                --numOfRowAdjustments;

                /* Make the spacing between lines rowAdjustment numOfPixels larger to ensure last line lies perfect
                 * at the bottom */
                lineRect.setBottom(lineRect.bottom() + rowAdjustment);
            }

            if (numOf_1_PixelBonus > 0) {
                --numOf_1_PixelBonus;
                lineRect.setBottom(lineRect.bottom() + 1);
            }
            ++m_numOfScreenRows;
        } else {
            m_screenRows[index].valid = false;
        }
        ++index;
    }
    return true;
}

/* #CRUSTI-OFF# */
/***********************************************************************************************************************
*   DrawRows
*   This function go through what has been put into the screenRows buffer and put them on screen. Hence this function
*   is just considering the layout of the rows, not which ones.
***********************************************************************************************************************/
void CEditorWidget::DrawRows(void)
{
    auto doc_p = GetDocument();
    int size;
    char* start_p;
    int rowProperties;
    int row;
    CFilterItem* filterItem_p;
    int index;
    QRect* screenRowRect_p;
    QRect headRect;
    CSelection* selection_p;
    bool bookmark;
    bool selected;
    QSize  lineSize(0, 0);
    FontItem_t* currentFont_p = nullptr;

    const auto selectionColor = (CSCZ_LastViewSelectionKind() == CSCZ_LastViewSelectionKind_TextView_e) ?
                SELECTION_COLOR_IN_FOCUS : SELECTION_COLOR_PASSIVE_FOCUS;

    headRect.setLeft(m_textRow_Head_X.left());
    headRect.setRight(m_textRow_Head_X.right());

    QSize fontSize = GetTheDoc()->m_fontCtrl.GetFontSize();

    m_numOfHighLights = 0;
    debug_numOfRowsForHighLight = 0;
    for (index = 0; m_screenRows[index].valid; ++index) {
        row = m_screenRows[index].row;
        screenRowRect_p = &m_screenRows[index].screenRect;

        filterItem_p = nullptr;

        if ((m_screenRows[index].presentation != ROW_PRESENTATION_HEAD_FRAME_e) &&
                (m_screenRows[index].presentation != ROW_PRESENTATION_FOOTER_FRAME_e)) {
            filterItem_p = doc_p->m_rowCache_p->GetFilterRef(row);
        }

        headRect.setTop(screenRowRect_p->top());
        headRect.setBottom(screenRowRect_p->bottom());

        switch (m_screenRows[index].presentation) {
            case ROW_PRESENTATION_HEAD_FRAME_e:
            case ROW_PRESENTATION_FOOTER_FRAME_e:
                continue; /* next item in the for loop... don't care about the head/foot here */

            default:
            case ROW_PRESENTATION_BOOKMARKED_e:
            case ROW_PRESENTATION_NORMAL_e:
                currentFont_p = m_blackFont_p;
                break;

            case ROW_PRESENTATION_FILTERED_e:
            {
                if (filterItem_p != nullptr) {
                    if (filterItem_p->m_font_p != nullptr) {
                        currentFont_p = filterItem_p->m_font_p;
                    } else {
                        currentFont_p = nullptr;
                        TRACEX_E("Row:%d ScreenRow:%d, Font is nullptr", row, index)
                    }
                } else {
                    /* This is a fallback, there was a severe dump because screen was update at the same time as
                     * filtering was processing... and the filter setup was cleared. If this occurs, which is
                     * wrong, then fallback to gray font */
                    currentFont_p = m_grayFont_p;
                }
                break;
            }

            case ROW_PRESENTATION_GRAYED_e:
                 currentFont_p = m_grayFont_p;
                 break;

            case ROW_PRESENTATION_CLIPPED_e:
                currentFont_p = m_clippedFont_p;
                filterItem_p = nullptr;
                break;
        } // switch

        if (currentFont_p != nullptr) {
            doc_p->m_fontCtrl.SetFont(m_painter_p, currentFont_p);
        }

        bookmark = m_screenRows[index].presentation == ROW_PRESENTATION_BOOKMARKED_e ? true : false;
        selected = isRowSelected(row, &selection_p);

        /* ---- Format text about to be shown ---- */

        doc_p->GetTextItem(row, &start_p, &size, &rowProperties);
        size = size > DISPLAY_MAX_ROW_SIZE ? DISPLAY_MAX_ROW_SIZE : size;
        lineSize = QSize(0, 0);
        memcpy(m_tempStr, start_p, static_cast<size_t>(size));
        if (size >= DISPLAY_MAX_ROW_SIZE - 3) {
            m_tempStr[size - 3] = '.';
            m_tempStr[size - 2] = '.';
            m_tempStr[size - 1] = '.';
        }
        m_tempStr[size] = 0;

        lineSize = GetTabbedSize(m_tempStr, size, currentFont_p->font_p);
        screenRowRect_p->setRight(screenRowRect_p->left() + lineSize.width());

        /* ---- Draw the bookmark bitmap/icon  ---- */

        if (bookmark) {
            QRect bookmarkRect = QRect(headRect.right() - 1 - m_bookmarkWidth, headRect.top(),
                                       m_bookmarkWidth, screenRowRect_p->height());
            DrawBookmark(&bookmarkRect);
        }

        /* ---- Draw the Decoded box ---- */

        if (rowProperties & TIA_CACHE_MEMMAP_PROPERTY_DECODED) {
            int xstart = headRect.right() - 2 - m_bookmarkWidth - 1 - m_decodedRectWidth;
            m_painter_p->fillRect(xstart, screenRowRect_p->top(),
                                  m_decodedRectWidth, screenRowRect_p->height(),
                                  QColor(DECODED_COLOR));
        }

        /* --- Draw textrow background color (only for the text, not the line number) ---- */

        Q_COLORREF bgColor;
        if (doc_p->m_fontCtrl.GetBGColor(currentFont_p, &bgColor)) {
            m_painter_p->fillRect(screenRowRect_p->left(), screenRowRect_p->top(),
                                  screenRowRect_p->width(), screenRowRect_p->height(),
                                  QColor(bgColor));
        }

        /* ---- Draw auto-highlights ----- */

        AutoHightlight_RowInfo_t* autoHighlightInfo_p;

        if (m_inFocus && doc_p->m_autoHighLighter_p->GetAutoHighlightRowInfo(static_cast<int>(row),
                                                                             &autoHighlightInfo_p)) {

            QList<TextRectElement_t*>& list = autoHighlightInfo_p->elementRefs;
            bool pixelUpdate = false;
            bool topUpdate = false;

            m_numOfHighLights += static_cast<int>(list.count());

            auto startx = PIXEL_STAMP_GET_STARTX(autoHighlightInfo_p->pixelStamp);
            auto starty = PIXEL_STAMP_GET_STARTY(autoHighlightInfo_p->pixelStamp);
            auto font = PIXEL_STAMP_GET_FONT(autoHighlightInfo_p->pixelStamp);

            // If the auto hightlight info is in-correct then update the data
            if (startx != static_cast<uint64_t>(m_textRow_X.left()) ||
                    font != static_cast<uint64_t>(fontSize.width())) {
                PIXEL_STAMP_SET_STARTX(autoHighlightInfo_p->pixelStamp, static_cast<uint64_t>(m_textRow_X.left()));
                PIXEL_STAMP_SET_FONT(autoHighlightInfo_p->pixelStamp, fontSize.width());
                pixelUpdate = true;
            }

            if (starty != static_cast<uint64_t>(screenRowRect_p->top())) {
                PIXEL_STAMP_SET_STARTY(autoHighlightInfo_p->pixelStamp, screenRowRect_p->top());
                topUpdate = true;
            }

            for (auto element_p : list) {
                if (pixelUpdate) {
                    QSize  highLightLength;
                    QSize  highLightStart;

                    highLightStart = QSize(0, 0);

                    if (element_p->startCol > 0) {
                        highLightStart = GetTabbedSize(m_tempStr, element_p->startCol, currentFont_p->font_p);
                    }

                    highLightLength = GetTabbedSize(m_tempStr, element_p->endCol + 1,
                                                    currentFont_p->font_p) - highLightStart;

                    element_p->rectPixel = *screenRowRect_p;
                    element_p->rectPixel.setLeft(m_textRow_X.left() + highLightStart.width());
                    element_p->rectPixel.setRight(element_p->rectPixel.left() + highLightLength.width());
                }
                else if (topUpdate) {
                    element_p->rectPixel.setTop(screenRowRect_p->top());
                    element_p->rectPixel.setBottom(screenRowRect_p->bottom());
                }
                m_painter_p->fillRect(element_p->rectPixel.left(), element_p->rectPixel.top(),
                                      element_p->rectPixel.width(), element_p->rectPixel.height(),
                                      QColor(AUTO_HIGHLIGHT_COLOR));
            }
        }

        /* ---- Draw Line text ---- */

        FontModification_RowInfo_t* fontModRowInfo_p;

        /* If the row contains TABs its not currently with present algorithm possible to draw
         * parts of the line in bold (text shifted), as when drawing pieces of text the */
        if (filterItem_p != nullptr && (filterItem_p->m_size > 0) &&
                (doc_p->m_fontModifier_p->GetFontModRowInfo(row, filterItem_p, &fontModRowInfo_p))) {
            DrawModifiedFontRow(fontModRowInfo_p, screenRowRect_p, m_tempStr, size, filterItem_p);
        }
        else {
            m_painter_p->drawText(QRect(screenRowRect_p->left(),
                                        screenRowRect_p->top() + static_cast<int>(m_textRectOffset_Y),
                                        screenRowRect_p->right(),
                                        screenRowRect_p->bottom()),
                                  QString(m_tempStr), m_textOption);
        }

        if (currentFont_p != m_grayFont_p && currentFont_p != m_clippedFont_p) {
            doc_p->m_fontCtrl.SetFont(m_painter_p, m_blackFont_p);
        }

        /* Draw Line numbers */

        m_painter_p->drawText(QRect(headRect.left(), screenRowRect_p->top() + static_cast<int>(m_textRectOffset_Y),
                                    screenRowRect_p->left(), screenRowRect_p->bottom()),
                              QString("%1").arg(row, 5), m_textOption);

        /* Draw exclude line, small thin red line, "exclude marker" */

        if (filterItem_p != nullptr && filterItem_p->m_exclude) {
            m_painter_p->fillRect(screenRowRect_p->left(),
                                  screenRowRect_p->top() + (screenRowRect_p->bottom() - screenRowRect_p->top()) / 2 - 1,
                                  screenRowRect_p->right() - screenRowRect_p->left(),
                                  2,
                                  QColor(Q_RGB(0xff, 0x00, 0x00)));
        }

        /* Draw selection box */

        if (selected && selection_p != nullptr &&
                (selection_p->startCol < 0 || selection_p->startCol < size)) {
            doc_p->m_fontCtrl.SetFont(m_painter_p, m_blackFont_p);

            if (selection_p->startCol == -1 || selection_p->endCol == -1) {
                m_painter_p->fillRect(*screenRowRect_p, selectionColor);
            }
            else if (start_p != nullptr) {
                QSize size_start;
                QSize size_end;

                size_start = QSize(0, 0);
                size_end = QSize(0, 0);

                if (selection_p->startCol > -1) {
                    size_start = GetTabbedSize(m_tempStr, selection_p->startCol, currentFont_p->font_p);
                }

                int endCol = selection_p->endCol < (DISPLAY_MAX_ROW_SIZE - 1) ?
                            selection_p->endCol : DISPLAY_MAX_ROW_SIZE - 1;

                size_end = GetTabbedSize(m_tempStr, endCol + 1, currentFont_p->font_p);

                m_painter_p->fillRect(screenRowRect_p->left() + size_start.width(),
                                      screenRowRect_p->top(),
                                      size_end.width() - size_start.width(),
                                      screenRowRect_p->height(),
                                      QColor(selectionColor));

                /* terminate the string (note that this string is useless after this */
                m_tempStr[/*selection_p->startCol +*/ endCol + 1] = 0;
                DrawTextPart(screenRowRect_p, m_tempStr, selection_p->startCol,
                             endCol - selection_p->startCol + 1, m_blackFont_p->font_p);
            }
        }

    }// for
}
/* #CRUSTI-ON# */

/***********************************************************************************************************************
*   ContainsTabs
***********************************************************************************************************************/
bool CEditorWidget::ContainsTabs(const char *text_p, const int textSize)
{
    for (int index = 0; index < textSize; index++) {
        if (*text_p == 9) {
            return true;
        }
        text_p++;
    }
    return false;
}

/***********************************************************************************************************************
*   DrawModifiedFontRow
***********************************************************************************************************************/
void CEditorWidget::DrawModifiedFontRow(FontModification_RowInfo_t *fontModRowInfo_p, QRect *rect_p,
                                        const char *text_p, const int textSize, CFilterItem *filterItem_p)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    int currentCol = 0;

    /* Parts of the text line that is not part of the elementRefs shall be drawn normally, and this will be mixed all
     * along the row. Hence, a loop is required where normal + modfied is printed until the row ends.
     *
     *  a. Loop while currentCol is less than textSize
     *  b. Define start point for next element_p, draw normal letters from currentCol up to start point of element_p
     *  c. Set currentCol to point to the beginning of next character index */
    FontItem_t *font_p = doc_p->m_fontCtrl.RegisterFont(filterItem_p->m_color,
                                                        filterItem_p->m_bg_color, FONT_MOD_COLOR_MOD);
    QString text(text_p);
    QRect adjustedBox(*rect_p);
    adjustedBox.adjust(1, 0, 1, 0);

    for (auto& fontElement_p : fontModRowInfo_p->elementRefs) {
        auto element_p = &fontElement_p->textRectElement;

        /* 1. First print normal characters. */
        const auto normalLetters = element_p->startCol - currentCol;

        if (normalLetters > 0) {
            DrawTextPart(rect_p, text, currentCol, normalLetters, filterItem_p->m_font_p->font_p);
        }

        /* 2. Print the special (modified characters). To create a bold effect this text is printed twice */

        const auto length = element_p->endCol - element_p->startCol + 1;
        DrawTextPart(rect_p, text, element_p->startCol, length, font_p->font_p);
        DrawTextPart(&adjustedBox, text, element_p->startCol, length, font_p->font_p);

        currentCol = element_p->endCol + 1;
    }

    /* Finalize with printing the last normal characters that might be ending the row */
    if (currentCol < textSize) {
        DrawTextPart(rect_p, text, currentCol, textSize - currentCol, filterItem_p->m_font_p->font_p);
    }
}

/***********************************************************************************************************************
*   DrawBookmark
***********************************************************************************************************************/
void CEditorWidget::DrawBookmark(QRect *rect_p)
{
    QBrush bookmarkBrush = QBrush(QColor(BOOKMARK_COLOR /*BOOKMARK_COLOR_FRAME*/));

    m_painter_p->setBrush(bookmarkBrush);
    m_painter_p->drawRoundedRect(*rect_p, 2, 2);
}

/***********************************************************************************************************************
*   LimitTopLine
* This function is used when topLine has been modified, typically by a page-down or scroll operation. It shall make
* sure that there is always lines on screen, hence re-align the topLine such that screen is filled
***********************************************************************************************************************/
void CEditorWidget::LimitTopLine(void)
{
    if (m_topLine < 0) {
        m_topLine = 0;
    }

    int currentTopLine = m_topLine;
    int dummyTopLine = 0;
    int remains = 0;
    double currentRelPos = m_relPos;

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        /* Just check that we may fill the rest of screen with filtered text lines */
        if (!SearchFilteredRows_TIA(m_topLine, m_maxDisplayRows, false, &dummyTopLine, &remains)) {
            int searchUp = remains;

            /* There was not enough lines to fill the screen, move the topLine upwards, exclude the current topLine */

            if (!SearchFilteredRows_TIA(m_topLine - 1, searchUp, true, &m_topLine, &remains)) {
                TRACEX_DE("CEditorWidget::LimitTopLine Not enough lines to position topLine")
            }
        }
    } else {
        if (m_topLine + m_maxDisplayRows > m_totalNumOfRows) {
            m_topLine = m_totalNumOfRows - m_maxDisplayRows;
        }
    }

    if (m_topLine < 0) {
        m_topLine = 0;
    }

    UpdateRelPosition();

    if (m_topLine != currentTopLine) {
        TRACEX_DE("CEditorWidget::LimitTopLine rePos %f -> %f  TopLine %d -> %d",
                  currentRelPos, m_relPos, currentTopLine, m_topLine)
    }
}

/***********************************************************************************************************************
*   SuggestedFocusRow
***********************************************************************************************************************/
int CEditorWidget::SuggestedFocusRow(void)
{
    int row = m_topLine;

    if (!m_selectionList.isEmpty()) {
        CSelection *selection_p = m_selectionList.first();
        row = selection_p->row;
    } else if ((m_cursorSel.row >= 0) && isCursorVisible()) {
        /* Ensure cursor is visible if it should be used */
        row = m_cursorSel.row;
    } else {
        /* Pick a line in the middle of the screen */
        row = m_screenRows[(m_maxDisplayRows > 0 ? m_maxDisplayRows : 0) / 2].row;
    }

    return row;
}

/***********************************************************************************************************************
*   SuggestedFocusRow
***********************************************************************************************************************/
void CEditorWidget::SearchNewTopLine(bool checkDisplayCache, int focusRow)
{
    auto doc_p = GetTheDoc();

    TRACEX_D("CEditorWidget::SearchNewTopLine  checkDisplayCache:%d focusRow:%d", checkDisplayCache, focusRow)

    if (focusRow == -1) {
        focusRow = SuggestedFocusRow();
    }

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        /* If there is a selection this shall be used to center the row. As such, start from the selected line
         * and search enough filtered rows to fill the upper half of the screen. This will asure that the selected
         * line will be put in the center of the screen.
         * If there is no selection, and the cursor is not currently visible, then pick a row in the middle of the
         * screen to align to. */
        if (checkDisplayCache && RowExist_inScreen(static_cast<int>(focusRow))) {
            TRACEX_D("CEditorWidget::SearchNewTopLine   Cursor or Selection Visible at row:%d", focusRow)
            return;
        }

        if (isRowVisible(focusRow)) {
            /* Ensure the same offset to topline for the focusRow */
            int offset = GetScreenRowOffset(focusRow);
            if (!SearchFilteredRows_TIA(focusRow, offset + 1, true, &m_topLine)) {
                m_topLine = doc_p->m_database.packedFIRA_p[m_minFIRAIndex].row;
            }
        } else {
            /* Make the focus row apear in the middle of the screen */
            if (!SearchFilteredRows_TIA(focusRow, m_maxDisplayRows / 2 - 2, true, &m_topLine)) {
                m_topLine = doc_p->m_database.packedFIRA_p[m_minFIRAIndex].row;
            }
        }

        UpdateRelPosition();
        TRACEX_D("CEditorWidget::SearchNewTopLine   New topline:%d", m_topLine)
        return;
    } else {
        /* if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) */

        if (RowExist_inScreen(focusRow)) {
            /* Ensure focus row remains still in on screen */
            int offset = GetScreenRowOffset(focusRow);
            m_topLine = focusRow - offset;
            if (m_topLine < 0) {
                m_topLine = 0;
            }
            return;
        }

        /* Put focusRow in the middle */
        m_topLine = focusRow - m_maxDisplayRows / 2;
        if (m_topLine < 0) {
            m_topLine = 0;
        }
        UpdateRelPosition();
    }
}

/***********************************************************************************************************************
*   GetCursorDisplayRow
***********************************************************************************************************************/
bool CEditorWidget::GetCursorDisplayRow(int *displayRow_p)
{
    int rowIndex = 0;

    while (m_screenRows[rowIndex].valid) {
        if (m_screenRows[rowIndex].row == m_cursorSel.row) {
            *displayRow_p = rowIndex;
            return true;
        }
        ++rowIndex;
    }
    return false;
}

/***********************************************************************************************************************
*   RowExist_inScreen
***********************************************************************************************************************/
bool CEditorWidget::RowExist_inScreen(const int row)
{
    int rowIndex = 0;

    while (m_screenRows[rowIndex].valid) {
        if (m_screenRows[rowIndex].row == static_cast<int>(row)) {
            return true;
        }
        ++rowIndex;
    }
    return false;
}

/***********************************************************************************************************************
*   RowExist_inScreen
*   This function searches rows that have a filter attached to it. It may either searches in the inScreenRows.
*   The startRow decide which index to start from, and up decided if the search should go up or down. Further, the
*   search doesn't stop until the count number of filtered rows has been located
* IN, startRow:        Where to start the search from
* IN, count:           How many filtered lines to locate
* IN, up:              Search up or down
* OUT, row_p           Row inScreenRows
***********************************************************************************************************************/
bool CEditorWidget::SearchFilteredRows_inScreen(int startRow, int count, bool up, int *row_p)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    FIR_t *FIRA_p = doc_p->m_database.FIRA.FIR_Array_p;
    int index = startRow;

    *row_p = startRow; /* get the same row back (no move) */

    TRACEX_DE("SearchFilteredRows_inScreen startRow:%d count:%d up:%d", startRow, count, up)

    if (up) {
        for (index = startRow; index > 1; --index) {
            if (FIRA_p[m_screenRows[index].row].LUT_index != 0) {
                *row_p = index;
                count--;

                if (count == 0) {
                    TRACEX_DE("SearchFilteredRows_inScreen OK Row:%d", *row_p)
                    return true;
                }
            }
        }

        TRACEX_DE("SearchFilteredRows_inScreen FAILED Row:%d", *row_p)
        return false;
    } else {
        for (index = startRow; m_screenRows[index].valid; ++index) {
            if (FIRA_p[m_screenRows[index].row].LUT_index != 0) {
                *row_p = index;
                count--;

                if (count == 0) {
                    TRACEX_DE("SearchFilteredRows_inScreen OK Row:%d", *row_p)
                    return true;
                }
            }
        }

        TRACEX_DE("SearchFilteredRows_inScreen FAILED Row:%d", *row_p)
        return false;
    }
}

/***********************************************************************************************************************
*   GetClosestFilteredRow
***********************************************************************************************************************/
bool CEditorWidget::GetClosestFilteredRow(int startRow, bool up, int *row_p)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    bool found = false;
    int row = startRow;

    /* TODO should be possible to utilize the enumeration in FIR, take that enum index into the packed FIR and
     *  just take next */

    if (up) {
        while (row > 0 && !found) {
            uint8_t LUT_Index = doc_p->m_database.FIRA.FIR_Array_p[row].LUT_index;

            if ((LUT_Index != 0) && !(doc_p->m_database.filterItem_LUT[LUT_Index]->m_exclude)) {
                found = true;
            } else {
                --row;
            }
        }
    } else {
        const int max_row = doc_p->m_database.TIA.rows;

        while (row < max_row && !found) {
            uint8_t LUT_Index = doc_p->m_database.FIRA.FIR_Array_p[row].LUT_index;
            if ((LUT_Index != 0) && !(doc_p->m_database.filterItem_LUT[LUT_Index]->m_exclude)) {
                found = true;
            } else {
                ++row;
            }
        }
    }

    if (found) {
        *row_p = row;
    }

    return found;
}

/***********************************************************************************************************************
*   GetClosestFilteredRow
*  This function searches rows that have a filter attached to it. It the TIA database.
*  The startRow decide which index to start from, and up decided if the search should go up or down. Further,
*  the search doesn't stop until the count number of filtered rows has been located. Include the current row
*  If the start row contains a filter then the search stops right away, since on filter has been found
*
* IN, startRow:        Where to start the search from
* IN, count:           How many filtered lines to locate
* IN, up:              Search up or down
* OUT, row_p           Row in TIA
* OUT, remains_p      Indicate how many rows from the total count that couldn't be used, 0 if success, default nullptr
***********************************************************************************************************************/
bool CEditorWidget::SearchFilteredRows_TIA(int startRow, int count, bool up, int *row_p, int *remains_p)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    int row = 0;

    *row_p = startRow; /* get the same row back (no move) */

    if (remains_p != nullptr) {
        *remains_p = count;
    }

    if (m_totalNumOfRows == 0) {
        TRACEX_W("SearchFilteredRows_TIA  No filters")
        return false;
    }

    if (count == 0) {
        TRACEX_D("SearchFilteredRows_TIA  Count zero")
        return false;
    }

    if (startRow < m_minRowIndex) {
        /* Outside region, perhaps row clip */
        TRACEX_D("SearchFilteredRows_TIA startRow:%d WRONG  -> %d", startRow, m_minRowIndex)
        startRow = m_minRowIndex;
    } else if (startRow > m_maxRowIndex) {
        /* Outside region, perhaps row clip */
        TRACEX_D("SearchFilteredRows_TIA startRow:%d Outside  -> %d", startRow, m_maxRowIndex)
        startRow = m_maxRowIndex;
    }

    TRACEX_D("SearchFilteredRows_TIA startRow:%d count:%d up:%d", startRow, count, up)

    int start_LUT_Index;
    int new_LUT_Index;
    bool searchStart = true;

    /* special case, as startRow might be a filtered line */

    if ((doc_p->m_database.FIRA.FIR_Array_p[startRow].LUT_index != 0) &&
        !doc_p->m_database.filterItem_LUT[doc_p->m_database.FIRA.FIR_Array_p[startRow].LUT_index]->m_exclude) {
        searchStart = false;
        row = startRow;
        --count;      /* startRow counts as one hit */
    }

    if (count == 0) {
        if (remains_p != nullptr) {
            *remains_p = 0;
        }
        *row_p = startRow;
        return true;
    }

    if (up) {
        if (searchStart) {
            if (!GetClosestFilteredRow(startRow, false, &row)) {
                if (!GetClosestFilteredRow(startRow, true, &row)) {
                    TRACEX_W("SearchFilteredRows_TIA no filters either up or down from startRow:%d "
                             "count:%d up:%d m_totalNumOfRows:%d", startRow, count, up, m_totalNumOfRows)
                    return false;  /* total failure */
                }

                count--; /* take away one count since we need to search for one up */
            }
        }

        start_LUT_Index = doc_p->m_database.FIRA.FIR_Array_p[row].index;
        new_LUT_Index = start_LUT_Index - count;

        if (new_LUT_Index >= 0) {
            /* move count number of steps in the array, make sure there are enough steps left */
            if (remains_p != nullptr) {
                *remains_p = 0;
            }
            *row_p = doc_p->m_database.packedFIRA_p[new_LUT_Index].row;
            return true;
        } else {
            /* There wasn't enough steps left... */
            if (remains_p != nullptr) {
                *remains_p = count - start_LUT_Index;
            }
            *row_p = doc_p->m_database.packedFIRA_p[0].row;
        }
        return false;
    } else {
        if (searchStart) {
            if (!GetClosestFilteredRow(startRow, true, &row)) {
                if (!GetClosestFilteredRow(startRow, false, &row)) {
                    TRACEX_W("SearchFilteredRows_TIA no filters either up or down from startRow:%d "
                             "count:%d up:%d m_totalNumOfRows:%d", startRow, count, up, m_totalNumOfRows)
                    return false;  /* total failure */
                }
                count--; /* take away one count since we need to search for one up */
            }
        }

        start_LUT_Index = doc_p->m_database.FIRA.FIR_Array_p[row].index;
        new_LUT_Index = start_LUT_Index + count;

        /* move count number of steps in the array, make sure there are enough steps left */
        if (new_LUT_Index <= m_maxFIRAIndex) {
            if (remains_p != nullptr) {
                *remains_p = 0;
            }
            *row_p = doc_p->m_database.packedFIRA_p[new_LUT_Index].row;
            return true;
        } else {
            /* There wasn't enough steps left... */
            if (remains_p != nullptr) {
                *remains_p = (start_LUT_Index + count) - (doc_p->m_database.FIRA.filterMatches - 1);
            }

            *row_p = doc_p->m_database.packedFIRA_p[m_maxFIRAIndex].row;
        }
        return false;
    }
}

/***********************************************************************************************************************
*   GotoRow
***********************************************************************************************************************/
void CEditorWidget::GotoTop(void)
{
    int remains = 0;

    EmptySelectionList();

    m_topLine = m_min_topLine;

    HorizontalCursorFocus(HCursorScrollAction_Focus_en);

    LimitTopLine();
    UpdateRelPosition();
    UpdateCursor(false, m_topLine, 0);

    TRACEX_D("CEditorWidget::GotoTop Row:%d Remains:%d", m_topLine, remains)

    ForceVScrollBitmap();

    Refresh(); /* This was added during the QT port, instead of invalidaterect */
}

/***********************************************************************************************************************
*   GotoBottom
***********************************************************************************************************************/
void CEditorWidget::GotoBottom(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    EmptySelectionList();

    m_topLine = m_max_topLine;

    HorizontalCursorFocus(HCursorScrollAction_Focus_en);

    LimitTopLine();
    UpdateRelPosition();

    if ((m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) && (doc_p->m_database.FIRA.filterMatches > 0)) {
        UpdateCursor(false,
                     doc_p->m_database.packedFIRA_p[m_maxFIRAIndex].row,
                     doc_p->m_database.TIA.textItemArray_p[doc_p->m_database.packedFIRA_p[m_maxFIRAIndex].row].size);
    } else {
        UpdateCursor(false,
                     doc_p->m_database.TIA.rows - 1,
                     doc_p->m_database.TIA.textItemArray_p[m_maxRowIndex].size);
    }

    TRACEX_I("Goto bottom Row:%d max_top:%d", m_topLine, m_max_topLine)
    ForceVScrollBitmap();

    Refresh(); /* instead of invalidate rect */
}

/***********************************************************************************************************************
*   GotoRow
***********************************************************************************************************************/
void CEditorWidget::GotoRow(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    bool ok;
    int row = QInputDialog::getInt(this, tr("Goto row"), tr("Row"), 0, 0, doc_p->m_database.TIA.rows, 1, &ok);

    if (ok) {
        if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
            CFilterItem *filterItem_p = doc_p->m_rowCache_p->GetFilterRef(row);

            if ((filterItem_p == nullptr) || filterItem_p->m_exclude) {
                SetPresentationMode(PRESENTATION_MODE_ALL_e);
            }
        }

        EmptySelectionList();
        AddSelection(row, -1, -1, true, false);

        SearchNewTopLine(); /* uses the selection as focus */
        LimitTopLine();

        m_EraseBkGrndDisabled = true;
        update();
    }
}

/***********************************************************************************************************************
*   DrawDebugWindow
***********************************************************************************************************************/
void CEditorWidget::DrawDebugWindow(void)
{
    if (m_isDebuggingSelection) {
        m_isDebuggingSelection = false;

        if (isMouseCursorMovingRight()) {
            m_painter_p->fillRect(m_debuggingSelection, CLIPPED_TEXT_COLOR);
        } else {
            m_painter_p->fillRect(m_debuggingSelection, BOARDER_BACKGROUND_COLOR);
        }
    }
}

/***********************************************************************************************************************
*   showEvent
***********************************************************************************************************************/
void CEditorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

/***********************************************************************************************************************
*   Filter
***********************************************************************************************************************/
void CEditorWidget::Filter(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->Filter();
    Refresh();
}

/***********************************************************************************************************************
*   OnColClipStartSet
***********************************************************************************************************************/
void CEditorWidget::OnColClipStartSet(void)
{
    int column;
    QPoint alignedPoint;

    g_cfg_p->m_Log_colClip_Start = CFG_CLIP_NOT_SET;

    if (PointToColumn(&m_contextMenuScreenPoint, &column, &alignedPoint)) {
        g_cfg_p->m_Log_colClip_Start = column;
        if ((g_cfg_p->m_Log_colClip_End > CFG_CLIP_NOT_SET) &&
            (g_cfg_p->m_Log_colClip_Start >= g_cfg_p->m_Log_colClip_End)) {
            g_cfg_p->m_Log_colClip_End = CFG_CLIP_NOT_SET;
        }
    }
    TRACEX_D(QString("%1 %2").arg(__FUNCTION__).arg(g_cfg_p->m_Log_colClip_Start))
    update();
}

/***********************************************************************************************************************
*   OnColClipDisable
***********************************************************************************************************************/
void CEditorWidget::OnColClipDisable(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    g_cfg_p->m_Log_colClip_Start = CFG_CLIP_NOT_SET;
    g_cfg_p->m_Log_colClip_End = CFG_CLIP_NOT_SET;
    update();
}

/***********************************************************************************************************************
*   OnColClipEndSet
***********************************************************************************************************************/
void CEditorWidget::OnColClipEndSet(void)
{
    int column;
    QPoint alignedPoint;

    g_cfg_p->m_Log_colClip_End = CFG_CLIP_NOT_SET;

    if (PointToColumn(&m_contextMenuScreenPoint, &column, &alignedPoint)) {
        g_cfg_p->m_Log_colClip_End = column;
        if ((g_cfg_p->m_Log_colClip_Start > CFG_CLIP_NOT_SET) &&
            (g_cfg_p->m_Log_colClip_End <= g_cfg_p->m_Log_colClip_Start)) {
            g_cfg_p->m_Log_colClip_Start = CFG_CLIP_NOT_SET;
        }
    }
    TRACEX_D(QString("%1 %2").arg(__FUNCTION__).arg(g_cfg_p->m_Log_colClip_End))
    update();
}

/***********************************************************************************************************************
*   OnFilterItemAdd
***********************************************************************************************************************/
void CEditorWidget::OnFilterItemAdd(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    AddFilterItem();
}

/***********************************************************************************************************************
*   OnFilterItemProperties
***********************************************************************************************************************/
void CEditorWidget::OnFilterItemProperties(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    CFilterItem *filterItem_p = doc_p->m_rowCache_p->GetFilterRef(m_cursorSel.row);
    if ((filterItem_p != nullptr) && (g_workspace_p != nullptr)) {
        CWorkspace_FilterItemProperties(filterItem_p->m_uniqueID, this);
        TRACEX_D("%s %d", __FUNCTION__, filterItem_p->m_uniqueID)
    }
}

/***********************************************************************************************************************
*   OnFilterItemDisable
***********************************************************************************************************************/
void CEditorWidget::OnFilterItemDisable(void)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    CFilterItem *filterItem_p = doc_p->m_rowCache_p->GetFilterRef(m_cursorSel.row);
    if ((filterItem_p != nullptr) && (g_workspace_p != nullptr)) {
        CWorkspace_DisableFilterItem(filterItem_p->m_uniqueID);
        TRACEX_D("%s %d", __FUNCTION__, filterItem_p->m_uniqueID)
    }
}

/***********************************************************************************************************************
*   OnBookmarkToggle
***********************************************************************************************************************/
void CEditorWidget::OnBookmarkToggle(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    ToggleBookmark();
}

/***********************************************************************************************************************
*   OnCopy
***********************************************************************************************************************/
void CEditorWidget::OnCopy(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    SelectionsToClipboard();
}

/***********************************************************************************************************************
*   OnSearch
***********************************************************************************************************************/
void CEditorWidget::OnSearch(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    MW_Search();
}

/***********************************************************************************************************************
*   OnSave
***********************************************************************************************************************/
void CEditorWidget::OnSave(void)
{
    TRACEX_D("%s ", __FUNCTION__)
}

/***********************************************************************************************************************
*   OnFilter
***********************************************************************************************************************/
void CEditorWidget::OnFilter(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    Filter();
}

/***********************************************************************************************************************
*   OnPresentationToggle
***********************************************************************************************************************/
void CEditorWidget::OnPresentationToggle(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    TogglePresentationMode();
}

/***********************************************************************************************************************
*   OnPlotCursorSet
***********************************************************************************************************************/
void CEditorWidget::OnPlotCursorSet(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    SetPlotCursor();
}

/***********************************************************************************************************************
*   OnGotoRow
***********************************************************************************************************************/
void CEditorWidget::OnGotoRow(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    GotoRow();
}

/***********************************************************************************************************************
*   OnRowClipDisable
***********************************************************************************************************************/
void CEditorWidget::OnRowClipDisable(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    SetRowClip(true /*start*/, CFG_CLIP_NOT_SET);
    SetRowClip(false /*end*/, CFG_CLIP_NOT_SET);
}

/***********************************************************************************************************************
*   OnRowClipStartSet
***********************************************************************************************************************/
void CEditorWidget::OnRowClipStartSet(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    if (m_selectionList.isEmpty()) {
        SetRowClip(true /*start*/);
    } else {
        CSelection *selection_p = m_selectionList.first();
        SetRowClip(true /*start*/, selection_p->row);
    }
    Refresh();
}

/***********************************************************************************************************************
*   OnRowClipEndSet
***********************************************************************************************************************/
void CEditorWidget::OnRowClipEndSet(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    if (m_selectionList.isEmpty()) {
        SetRowClip(false /*end*/);
    } else {
        CSelection *selection_p = m_selectionList.first();
        SetRowClip(false /*end*/, selection_p->row);
    }
    Refresh();
}

/***********************************************************************************************************************
*   OnRowClipGotoStart
***********************************************************************************************************************/
void CEditorWidget::OnRowClipGotoStart(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    SetFocusRow(g_cfg_p->m_Log_rowClip_Start);
}

/***********************************************************************************************************************
*   OnRowClipGotoEnd
***********************************************************************************************************************/
void CEditorWidget::OnRowClipGotoEnd(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    SetFocusRow(g_cfg_p->m_Log_rowClip_End);
}

/***********************************************************************************************************************
*   OnFilterFileOpen
***********************************************************************************************************************/
void CEditorWidget::OnFilterFileOpen(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    CFGCTRL_LoadFilterFile();
}

/***********************************************************************************************************************
*   OnPluginFileOpen
***********************************************************************************************************************/
void CEditorWidget::OnPluginFileOpen(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    CFGCTRL_LoadPluginFile();
}

/***********************************************************************************************************************
*   OnLogFileOpen
***********************************************************************************************************************/
void CEditorWidget::OnLogFileOpen(void)
{
    TRACEX_D("%s ", __FUNCTION__)
    CFGCTRL_LoadLogFile();
}

/***********************************************************************************************************************
*   OnFileSaveSelectionsAs
***********************************************************************************************************************/
void CEditorWidget::OnFileSaveSelectionsAs(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QStringList filters;
    filters << "Text files (*.txt *.log)";

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_WorkspaceFile_en);
    kindList.append(RecentFile_Kind_LogFile_en);

    QStringList list; /* list to be converted to urls */
    doc_p->m_recentFiles.GetRecentPaths(list, kindList); /* append list with recent used paths to file */

    QStringList fileNames = CFGCTRL_GetUserPickedFileNames(QString("cutout.txt"), QFileDialog::AcceptSave,
                                                           filters, kindList, list.first());
    if (!fileNames.empty() && !fileNames.first().isEmpty()) {
        SaveSelectionsToFile(fileNames.first());
    }
}

/***********************************************************************************************************************
*   contextMenuEvent
***********************************************************************************************************************/
void CEditorWidget::contextMenuEvent(QContextMenuEvent *event)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if ((doc_p->m_database.TIA.rows == 0) || (g_workspace_p == nullptr)) {
        return;
    }

    m_contextMenuScreenPoint = MakeScreenPoint(event, m_hbmpOffset, m_vbmpOffset);

    QMenu contextMenu(tr("Context menu"), this);

    contextMenu.setToolTipsVisible(true);

    QAction filterItemAddAction("Add Filter Item (Ctrl-N)", this);
    connect(&filterItemAddAction, SIGNAL(triggered()), this, SLOT(OnFilterItemAdd()));
    filterItemAddAction.setToolTip(QString("Add a filter item to filter the log"));

    QAction filterItemDisableAction("Disable Filter Item", this);
    connect(&filterItemDisableAction, SIGNAL(triggered()), this, SLOT(OnFilterItemDisable()));
    filterItemDisableAction.setToolTip(QString("Disable the filter item matching this text"));

    QAction filterItemPropertiesAction("Filter Item Properties", this);
    connect(&filterItemPropertiesAction, SIGNAL(triggered()), this, SLOT(OnFilterItemProperties()));
    filterItemPropertiesAction.setToolTip(QString("Open the filter item editor for the filter item matching this text"));

    QAction colClipDisableAction("Disable Column Clip", this);
    connect(&colClipDisableAction, SIGNAL(triggered()), this, SLOT(OnColClipDisable()));
    colClipDisableAction.setToolTip(QString("Disable the column clip that is used to narrow\n"
                                            "down the filter processing on each row"));

    QAction colClipStartSetAction("Set Column Clip Start", this);
    connect(&colClipStartSetAction, SIGNAL(triggered()), this, SLOT(OnColClipStartSet()));
    colClipStartSetAction.setToolTip(QString("Set column clipping to limit where filter matches\n"
                                             "starts on each row"));

    QAction colClipEndSetAction("Set Column Clip End", this);
    connect(&colClipEndSetAction, SIGNAL(triggered()), this, SLOT(OnColClipEndSet()));
    colClipEndSetAction.setToolTip(QString("Set column clipping to limit where filter matches\n"
                                           "stops on each row"));

    QAction copyAction("Copy (Ctrl-C)", this);
    connect(&copyAction, SIGNAL(triggered()), this, SLOT(OnCopy()));
    copyAction.setToolTip(QString("Copy selected text to clip board"));

    QAction bookmarkToggleAction("Toggle Bookmark (Ctrl-F2)", this);
    connect(&bookmarkToggleAction, SIGNAL(triggered()), this, SLOT(OnBookmarkToggle()));
    bookmarkToggleAction.setToolTip(QString("Toggle a bookmark at this row, if there is already a bookmark\n"
                                            "it is removed, otherwise a bookmark is added. Then use F2 to\n"
                                            "navigate between existing bookmarks in this log file"));

    QAction fileSaveAction("Save Selections to File", this);
    connect(&fileSaveAction, SIGNAL(triggered()), this, SLOT(OnFileSaveSelectionsAs()));
    fileSaveAction.setToolTip(QString("Save the currently selected text to a seperate text file"));

    QAction searchAction("Search (F3, Shift-F3)", this);
    connect(&searchAction, SIGNAL(triggered()), this, SLOT(OnSearch()));

    QAction filterAction("Filter (F5)", this);
    connect(&filterAction, SIGNAL(triggered()), this, SLOT(OnFilter()));
    filterAction.setToolTip(QString("Filter the log based on the enabled filter items in the workspace. Only\n"
                                    "the rows having text matching filter items will then be shown.\n"
                                    "You can then switch between seeing the entire file or just the matching \n"
                                    "using F4"));

    QAction presentationToggleAction("Toggle Presentation Mode (F4)", this);
    connect(&presentationToggleAction, SIGNAL(triggered()), this, SLOT(OnPresentationToggle()));
    presentationToggleAction.setToolTip(QString("Switch between seeing just rows matching filter items and\n"
                                                "seeing all rows."));

    QAction plotCursorSetAction("Set Plot Cursor", this);
    connect(&plotCursorSetAction, SIGNAL(triggered()), this, SLOT(OnPlotCursorSet()));

    QAction gotoRowAction("Go to Row (Ctrl-G)", this);
    connect(&gotoRowAction, SIGNAL(triggered()), this, SLOT(OnGotoRow()));

    QAction rowClipDisableAction("Disable Row Clip  (Ctrl-F7)", this);
    connect(&rowClipDisableAction, SIGNAL(triggered()), this, SLOT(OnRowClipDisable()));

    QAction rowClipStartSetAction("Set Row Clip Start (Ctrl-F8)", this);
    connect(&rowClipStartSetAction, SIGNAL(triggered()), this, SLOT(OnRowClipStartSet()));
    rowClipStartSetAction.setToolTip(QString("Set row clipping to limit where filtering and searches start\n"
                                             "in the log"));

    QAction rowClipStartGotoAction("Goto Row Clip Start", this);
    connect(&rowClipStartGotoAction, SIGNAL(triggered()), this, SLOT(OnRowClipGotoStart()));
    rowClipStartGotoAction.setToolTip(QString("Goto the row where the row clipping starts"));

    QAction rowClipEndSetAction("Set Row Clip End (Ctrl-F9)", this);
    connect(&rowClipEndSetAction, SIGNAL(triggered()), this, SLOT(OnRowClipEndSet()));
    rowClipEndSetAction.setToolTip(QString("Set row clipping to limit where the filter and searches end\n"
                                           "in the log"));

    QAction rowClipEndGotoAction("Goto Row Clip End", this);
    connect(&rowClipEndGotoAction, SIGNAL(triggered()), this, SLOT(OnRowClipGotoEnd()));
    rowClipEndGotoAction.setToolTip(QString("Goto the row where the row clipping ends"));

    QAction logFileOpenAction("Open Log File...", this);
    connect(&logFileOpenAction, SIGNAL(triggered()), this, SLOT(OnLogFileOpen()));

    QAction filterFileOpenAction("Open Filter File...", this);
    connect(&filterFileOpenAction, SIGNAL(triggered()), this, SLOT(OnFilterFileOpen()));

    QAction pluginFileOpenAction("Open Plugin File...", this);
    connect(&pluginFileOpenAction, SIGNAL(triggered()), this, SLOT(OnPluginFileOpen()));

    QAction fileSaveSelectionsAsAction("Save Selections to File...", this);
    connect(&fileSaveSelectionsAsAction, SIGNAL(triggered()), this, SLOT(OnFileSaveSelectionsAs()));

    /* MENU LAYOUT
     *
     * Groups of actions, like row selections, do not add actions that are not valid
     * Single actions, like set plot cursor, disable it if not valid */
    contextMenu.addAction(&filterItemAddAction);

    bool filterItemFound = false;
    CFilterItem *filterItem_p = doc_p->m_rowCache_p->GetFilterRef(m_cursorSel.row);
    if (filterItem_p != nullptr) {
        filterItemFound = true;
    }
    contextMenu.addAction(&filterItemPropertiesAction);
    contextMenu.addAction(&filterItemDisableAction);
    filterItemPropertiesAction.setDisabled(filterItemFound ? false : true);
    filterItemDisableAction.setDisabled(filterItemFound ? false : true);

    if (m_selectionList.isEmpty()) {
        /* All actions dependent on selections shall be disabled here */
        copyAction.setDisabled(true);
        fileSaveAction.setDisabled(true);
        searchAction.setDisabled(true);

        if (!m_cursorActive) {
            bookmarkToggleAction.setDisabled(true);
        }
    } else {
        QString selectionText;
        CSelection dummy;

        if (GetActiveSelection(&dummy, selectionText)) {
            QList<CCfgItem_FilterItem *> filterItemList;
            CWorkspace_GetMatchingFilters(&filterItemList, selectionText);
            if (!filterItemList.isEmpty()) {
                QMenu *similar_to = contextMenu.addMenu(tr("&Similar to"));
                for (auto& filterItem_p : filterItemList) {
                    QAction *action_p;
                    action_p = similar_to->addAction(filterItem_p->m_itemText);
                    action_p->setEnabled(true);
                    connect(action_p, &QAction::triggered, [ = ] () {
                        CWorkspace_FilterItemProperties(filterItem_p->m_filterItem_ref_p->m_uniqueID, this);
                    });
                }
            }
        }
    }

    contextMenu.addSeparator();

    contextMenu.addAction(&copyAction);
    contextMenu.addAction(&searchAction);
    contextMenu.addAction(&bookmarkToggleAction);
    contextMenu.addAction(&fileSaveAction);

    contextMenu.addSeparator();

    contextMenu.addAction(&filterAction);

    if (CWorkspace_GetEnabledFilterItemCount() == 0) {
        filterAction.setEnabled(false);
    } else {
        filterAction.setEnabled(true);
    }

    contextMenu.addAction(&presentationToggleAction);
    if ((doc_p->m_database.FIRA.filterMatches + doc_p->m_database.FIRA.filterExcludeMatches) > 0) {
        presentationToggleAction.setEnabled(true);
    } else {
        presentationToggleAction.setEnabled(false);
    }

    contextMenu.addSeparator();

    CPlotPane *plotPane_p = CPlotPane_GetPlotPane();
    if ((plotPane_p == nullptr) || !plotPane_p->isPlotsActive()) {
        plotCursorSetAction.setDisabled(true);
    }

    contextMenu.addAction(&plotCursorSetAction);

    contextMenu.addSeparator();

    contextMenu.addAction(&gotoRowAction);

    contextMenu.addSeparator();

    /* Add Row Clip */
    contextMenu.addAction(&rowClipStartSetAction);
    contextMenu.addAction(&rowClipEndSetAction);
    if ((g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) || (g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET)) {
        if (g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) {
            contextMenu.addAction(&rowClipStartGotoAction);
        }
        if (g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET) {
            contextMenu.addAction(&rowClipEndGotoAction);
        }
        contextMenu.addAction(&rowClipDisableAction);
    }

    contextMenu.addSeparator();

    /* Add Column Clip */
    contextMenu.addAction(&colClipStartSetAction);
    contextMenu.addAction(&colClipEndSetAction);
    if ((g_cfg_p->m_Log_colClip_Start > CFG_CLIP_NOT_SET) || (g_cfg_p->m_Log_colClip_End > CFG_CLIP_NOT_SET)) {
        contextMenu.addAction(&colClipDisableAction);
    }

    contextMenu.addSeparator();

    contextMenu.addAction(&filterFileOpenAction);
    contextMenu.addAction(&logFileOpenAction);
    contextMenu.addAction(&pluginFileOpenAction);

    contextMenu.exec(event->globalPos() /*screenPoint_p->mouse mapToGlobal(screenPoint_p->mouse)*/);
}

/***********************************************************************************************************************
*   SetPlotCursor
***********************************************************************************************************************/
void CEditorWidget::SetPlotCursor(void)
{
    CPlotPane *plotPane_p = CPlotPane_GetPlotPane();
    CSelection selection;
    CLogScrutinizerDoc *doc_p = GetDocument();

    if (plotPane_p == nullptr) {
        return;
    }

    if (GetActiveSelection(&selection) || GetCursorPosition(&selection)) {
        int newRow = 0;
        int size;
        char *text_p;
        double time;

        doc_p->GetTextItem(selection.row, &text_p, &size);

        int resultCode = CWorkspace_PlotExtractTime(text_p, size, &time);

        if (resultCode == 1) {
            plotPane_p->setPlotCursor(time);
            return;
        }

        plotPane_p->setPlotCursor(selection.row, &newRow);

        if (newRow != selection.row) {
            MW_PlaySystemSound(SYSTEM_SOUND_QUESTION);
            QMessageBox::warning(this,
                                 tr("Cursor not exact"),
                                 tr("It wasn't possible to find an exact match for the row and the cursor position"));

            QString message;
            if (resultCode == -1) {
                message = QString("None of the plugins could extract the time from current row, after search "
                                  "closest row in graphs is %1 (dist:%2)")
                              .arg(newRow).arg(newRow - selection.row);
            } else {
                message = QString("None of the plugins support extracting the time from current row, after search "
                                  "closest row in graphs is %1 (dist:%2)")
                              .arg(newRow).arg(newRow - selection.row);
            }
            TRACEX_I(message)
        }
    }
}

/***********************************************************************************************************************
*   SetRowClip
***********************************************************************************************************************/
void CEditorWidget::SetRowClip(bool isStart, int row)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    if (row != -3) {
        /* do nothing */
    } else if (m_cursorActive) {
        row = m_cursorSel.row;
    } else if (!m_selectionList.isEmpty()) {
        CSelection *selection_p = m_selectionList.first();

        if (selection_p == nullptr) {
            return;
        }
        row = selection_p->row;
    }

    if (!isRowVisible(row) && (row != -1)) {
        QMessageBox::warning(this,
                             tr("Row clip warning"),
                             tr("To avoid misstakes it is necessary to have the cursor, or a selection,\r\n"
                                "visible on-screen to mark where the row clip shall be located. Please try\r\n"
                                "again with the cursor visible on-screen"));
        return;
    }

    TRACEX_D("CEditorWidget::SetRowClip - %d", row)

    if (isStart) {
        g_cfg_p->m_Log_rowClip_Start = row;
        if ((row != -1) && (g_cfg_p->m_Log_rowClip_End != CFG_CLIP_NOT_SET) &&
            (g_cfg_p->m_Log_rowClip_Start > g_cfg_p->m_Log_rowClip_End)) {
            g_cfg_p->m_Log_rowClip_End = CFG_CLIP_NOT_SET;
        }
    } else {
        g_cfg_p->m_Log_rowClip_End = row;

        if ((row != -1) && (g_cfg_p->m_Log_rowClip_Start != CFG_CLIP_NOT_SET) &&
            (g_cfg_p->m_Log_rowClip_End < g_cfg_p->m_Log_rowClip_Start)) {
            g_cfg_p->m_Log_rowClip_Start = CFG_CLIP_NOT_SET;
        }
    }

    if ((g_cfg_p->m_Log_rowClip_Start > 0) && (g_cfg_p->m_Log_rowClip_Start < doc_p->m_database.TIA.rows - 1)) {
        FIR_t *FIRA_p = doc_p->m_database.FIRA.FIR_Array_p;
        memset(&FIRA_p[0], 0, sizeof(FIR_t) * static_cast<size_t>(g_cfg_p->m_Log_rowClip_Start + 1));
    }

    if (((g_cfg_p->m_Log_rowClip_End) > 0) && ((g_cfg_p->m_Log_rowClip_End) < doc_p->m_database.TIA.rows - 1)) {
        FIR_t *FIRA_p = doc_p->m_database.FIRA.FIR_Array_p;
        memset(&FIRA_p[g_cfg_p->m_Log_rowClip_End], 0,
               sizeof(FIR_t) * static_cast<size_t>(doc_p->m_database.TIA.rows - g_cfg_p->m_Log_rowClip_End));
    }

    doc_p->CleanRowCache();  /* FIR information is stored there */
    doc_p->ReNumerateFIRA();
    doc_p->CreatePackedFIRA();
    doc_p->UpdateTitleRow();

    UpdateTotalNumRows();
    SearchNewTopLine(false, row);

    LimitTopLine();

    Refresh();
}

/***********************************************************************************************************************
*   SetRowClip
***********************************************************************************************************************/
void CEditorWidget::SaveMouseMovement(ScreenPoint_t *screenPoint_p)
{
    if (m_screenPointHistory[1].mouse != screenPoint_p->mouse) {
        /* Just shift the array */
        m_screenPointHistory[0] = m_screenPointHistory[1];
        m_screenPointHistory[1] = *screenPoint_p;
        PRINT_MOUSE(QString("%1 mx:%2 my:%3 dcx:%4 dcy:%5  scrollFL:%6 textWR:%7")
                        .arg(__FUNCTION__)
                        .arg(screenPoint_p->mouse.x())
                        .arg(screenPoint_p->mouse.y())
                        .arg(screenPoint_p->DCBMP.x())
                        .arg(screenPoint_p->DCBMP.y())
                        .arg(m_vscrollFrame.left())
                        .arg(m_textWindow.right()))
    }
}

/***********************************************************************************************************************
*   isMouseCursorMovingRight
***********************************************************************************************************************/
bool CEditorWidget::isMouseCursorMovingRight(void)
{
    return (m_screenPointHistory[1].mouse.x() > m_screenPointHistory[0].mouse.x() ? true : false);
}

/***********************************************************************************************************************
*   ProcessMouseMove
***********************************************************************************************************************/
void CEditorWidget::ProcessMouseMove(QMouseEvent *event, ScreenPoint_t& screenPoint)
{
    bool invalidate = false;
    bool vscrollSliderHit = false;
    Qt::CursorShape requestCursor = Qt::ArrowCursor;
    CLogScrutinizerDoc *doc_p = GetDocument();

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    SaveMouseMovement(&screenPoint);

    if (event->buttons() & Qt::LeftButton) {
        m_mouseTracking = true;  /* used to capture/detect when the mouse leaves the window with l-mouse pressed */
    }

    if (LOG_TRACE_MOUSE_COND) {
        TRACEX_DE(QString("%1 mouse(%2,%3#%4,%5)  scrollFL:%6 textWR:%7")
                      .arg(__FUNCTION__).arg(screenPoint.mouse.x()).arg(screenPoint.mouse.y())
                      .arg(screenPoint.DCBMP.x()).arg(screenPoint.DCBMP.y())
                      .arg(m_vscrollFrame.left()).arg(m_textWindow.right()))
    }

    if (doc_p->m_database.TIA.rows == 0) {
        return;
    }

    /* m_beam_left_padding is the small extra pixels needed to some extra space infront of the row such that it
     * is easier to select the first letter. It is dependent on the current font-size. */

    if ((screenPoint.DCBMP.x() > (m_textRow_X.left() - m_beam_left_padding)) &&
        (screenPoint.DCBMP.x() < m_textWindow.right()) &&
        (screenPoint.mouse.y() > m_vscrollFrame.top()) &&
        (screenPoint.mouse.y() < m_vscrollFrame.bottom())) {
        requestCursor = Qt::IBeamCursor;
    }

    /* Check if v-slide window selected (the bug) */

    if (((screenPoint.mouse.x() >= m_vscrollFrame.left()) && (screenPoint.mouse.x() < (m_vscrollFrame.right()))) ||

        /*hysteresis, if bug is shown... show it while over it */
        (m_vscrollBitmapEnabled && ((screenPoint.mouse.x() >= m_vScrollBMP_Rect.left()) &&
                                    ((screenPoint.mouse.y() >= m_vScrollBMP_Rect.top()) &&
                                     (screenPoint.mouse.y() <= m_vScrollBMP_Rect.bottom()))))) {
        requestCursor = Qt::ArrowCursor;
        vscrollSliderHit = true;
        invalidate |= SetVScrollBitmap(true);    /* show the bitmap... mouse cursor over the scroll */
    }

    if (!vscrollSliderHit && !(m_vscrollSliderGlue)) {
        invalidate |= SetVScrollGlue(false);
    }

    if (event->buttons() & Qt::LeftButton) {
        /* Dragging, sliding */

        if ((screenPoint.mouse.y() > m_vscrollFrame.top()) && (screenPoint.mouse.y() < m_vscrollFrame.bottom())) {
            /* Within the frame window
             * Within the text window */

            /* SCROLL SLIDER GLUE */

            if (m_vscrollSliderGlue) {
                double temp_relPos;
                int vscrollSlider_top = screenPoint.mouse.y() - m_vscrollSliderGlueOffset;

                if (vscrollSlider_top < m_vscrollFrame.top()) {
                    /* Above slider */

                    m_relPos = m_min_rel_pos;
                    temp_relPos = m_relPos;

                    if (LOG_TRACE_MOUSE_COND) {
                        TRACEX_DE("%s m_relPos -> m_min_rel_pos, scrollTop:%d FrameTop:%d",
                                  __FUNCTION__, vscrollSlider_top, m_vscrollFrame.top())
                    }
                } else if (vscrollSlider_top + m_vscrollSlider.height() > m_vscrollFrame.bottom()) {
                    /* Below slider */

                    m_relPos = m_max_rel_pos;
                    temp_relPos = m_relPos;

                    if (LOG_TRACE_MOUSE_COND) {
                        TRACEX_DE("%s m_relPos -> m_max_rel_pos, scrollTop:%d Height:%d FrameBottom:%d",
                                  __FUNCTION__, vscrollSlider_top, m_vscrollSlider.height(), m_vscrollFrame.bottom())
                    }
                } else {
                    /* This equ. is normalized in the sense it is in the range of 0 - 1
                     *  Note also that we do not advance to the middle of the slider, that is instead handled in
                     *  equ.2 where the "middle" of set is added ontop. As when the middle is at m_min_rel_pos then
                     *  topLine is 0. */
                    m_relPos = ((vscrollSlider_top - m_vscrollFrame.top())) /
                               static_cast<double>(m_vscrollFrameHeigth - (m_vscrollSlider.height()));  /* equ.1 */
                    m_relPos += m_min_rel_pos;       /* equ.2 */

                    if (LOG_TRACE_MOUSE_COND_WARN) {
                        if ((m_vscrollFrameHeigth != m_vscrollFrame.height()) ||
                            (m_vscrollSlider.height() != m_vscrollSlider.height())) {
                            TRACEX_W("%s Frame or Slider not setup OK cFrameHight:%d FrameHeight:%d cSlider:%d Slider",
                                     __FUNCTION__, m_vscrollFrameHeigth, m_vscrollFrame.height(),
                                     m_vscrollSlider.height(), m_vscrollSlider.height())
                        }
                    }

                    temp_relPos = m_relPos;

                    if (m_relPos < m_min_rel_pos) {
                        if (LOG_TRACE_MOUSE_COND_WARN) {
                            TRACEX_W("%s m_relPos TOO LOW", __FUNCTION__)
                        }
                        m_relPos = m_min_rel_pos;
                    } else if (m_relPos > m_max_rel_pos) {
                        if (LOG_TRACE_MOUSE_COND_WARN) {
                            TRACEX_W("%s m_relPos TOO HIGH", __FUNCTION__)
                        }
                        m_relPos = m_max_rel_pos;
                    }
                }

                UpdateRelTopLinePosition();

                int evalRaster =
                    static_cast<int>(m_min_vslider_center_pos
                                     + ((m_max_vslider_center_pos - m_min_vslider_center_pos) * RELPOS_NORMALIZED)
                                     - (m_vscrollSlider.height() / 2.0) + 0.5);

                AlignRelPosToRockScroll(evalRaster);

                if (LOG_TRACE_MOUSE_COND) {
                    TRACEX_DE("%s V-Glued dragging vscrollSlider_top:%d raster:%d "
                              "m_relPos:%f relPosCorrection:%f (%f) m_topLine:%d "
                              "g-offset:%d offset-mouse:%d sliderHeight:%d frameHeight:%d",
                              __FUNCTION__, vscrollSlider_top, evalRaster, m_relPos,
                              temp_relPos - m_relPos,
                              (temp_relPos - m_relPos) * m_totalNumOfRows,
                              m_topLine, m_vscrollSliderGlueOffset,
                              screenPoint.mouse.y() - vscrollSlider_top,
                              m_vscrollSlider.height(), m_vscrollFrame.height())
                }

                invalidate = true;
            } /* if (m_vscrollSliderGlue) */
            else if (m_hscrollSliderGlue) {
                m_hscrollSlider.setLeft(screenPoint.mouse.x() - m_hscrollSliderGlueOffset);
                m_hrelPos = (m_hscrollSlider.left() - m_hscrollFrame.left()) /
                            static_cast<double>(m_hscrollFrameWidth - m_hscrollSliderWidth);

                if (LOG_TRACE_MOUSE_COND) {
                    TRACEX_DE("%s  V-Glued dragging HSCROLL m_hrelPos:%f", __FUNCTION__, m_hrelPos)
                }
                invalidate = true;
            } else {
                /* Possibly text dragging or text drag selection... */

                invalidate = textWindow_SelectionUpdate(&screenPoint, LS_LG_EVENT_MOUSE_MOVE_e, event->modifiers());

                /* text selection ongoing, check if close to the left */
                if (invalidate) {
                    if ((screenPoint.mouse.x() > m_vscrollFrame.left() - 10) &&
                        (screenPoint.mouse.x() < m_vscrollFrame.left())) {
                        PageLeftRight(false, 10);
                    } else if (screenPoint.mouse.x() < m_hscrollFrame.left() + 10) {
                        PageLeftRight(true, -10);
                    } else if (screenPoint.mouse.y() < m_vscrollFrame.top() + 10) {
                        PageUpDown(true, 2);
                    } else if (screenPoint.mouse.y() > m_hscrollFrame.top() - 10) {
                        PageUpDown(false, 2);
                    }
                }
            }
        } else {
            /* Inside frame, outside text-window y-space...
             * typically at the horiz scrollbar or vertical bar dragged under the text window */

            if (m_vscrollSliderGlue) {
                /* VERTICAL SCROLL */
                invalidate = true;
                if ((screenPoint.mouse.y() < m_textWindow.top()) || (m_maxDisplayRows >= doc_p->m_database.TIA.rows)) {
                    m_relPos = m_min_rel_pos;
                    UpdateRelTopLinePosition();
                } else if (screenPoint.mouse.y() > m_textWindow.bottom()) {
                    m_relPos = m_max_rel_pos;
                    UpdateRelTopLinePosition();
                }
            } else if (m_hscrollSliderGlue) {
                /* HORIZONTAL SCROLL */
                m_hscrollSlider.setLeft(screenPoint.mouse.x() - m_hscrollSliderGlueOffset);
                if (m_hscrollSlider.left() < 0) {
                    m_hscrollSlider.setLeft(0);
                }
                m_hrelPos = (m_hscrollSlider.left() - m_hscrollFrame.left()) /
                            static_cast<double>(m_hscrollFrameWidth - m_hscrollSliderWidth);
                if (LOG_TRACE_MOUSE_COND) {
                    TRACEX_DE("%s HSCROLL hrelPos:%f", __FUNCTION__, m_hrelPos)
                }
                invalidate = true;
            }
        }
    } else {
        /* Mouse move without key selection */
        if (LOG_TRACE_MOUSE_COND) {
            TRACEX_DE("%s  Mouse without key selection", __FUNCTION__)
        }

        /* If mouse is close to vscroll bar then show bitmap */
        if (((screenPoint.mouse.x() >= m_vscrollSlider.left()) && (screenPoint.mouse.x() <= m_vscrollSlider.right()))
            ||
            (((screenPoint.mouse.x() >= m_vScrollBMP_Rect.left()) &&
              (screenPoint.mouse.x() <= m_vScrollBMP_Rect.right())) &&

             ((screenPoint.mouse.y() >= m_vScrollBMP_Rect.top()) &&
              (screenPoint.mouse.y() <= m_vScrollBMP_Rect.bottom())))) {
            TRACEX_DE("%s HIT", __FUNCTION__)
            invalidate |= ForceVScrollBitmap();
        }

        if (m_vscrollSliderGlue) {
            /* strange... we have lost the unset of glue */
            invalidate |= SetVScrollGlue(false);
            if (LOG_TRACE_MOUSE_COND) {
                TRACEX_DE("%s  strange... we have lost the unset of glue", __FUNCTION__)
            }
        }
        m_hscrollSliderGlue = false;
    }

    if (invalidate) {
        LimitTopLine();
        m_EraseBkGrndDisabled = true;
        update();
    }

    if ((cursor().shape() != m_cursorShape) || m_resizeCursorSync) {
        m_resizeCursorSync = false;
        unsetCursor();
        setCursor(Qt::ArrowCursor);
        setCursor(m_cursorShape);
        PRINT_CURSOR(QString("Reset/set cursor %1").arg(m_cursorShape))
    }

    if (m_cursorShape != requestCursor) {
        if (requestCursor == Qt::ArrowCursor) {
            unsetCursor();
            PRINT_CURSOR(QString("Unset cursor"))
        } else {
            setCursor(requestCursor);
            PRINT_CURSOR(QString("Set cursor %1").arg(requestCursor))
        }
    }
    m_cursorShape = requestCursor;
}

/***********************************************************************************************************************
*   OnMouseWheel
***********************************************************************************************************************/
void CEditorWidget::OnMouseWheel(QWheelEvent *event)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    MW_updateLogFileTrackState(false);

    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;
    int zDelta = 0;
    if (!numDegrees.isNull()) {
        zDelta = numDegrees.y();
    } else if (!numPixels.isNull()) {
        zDelta = numPixels.y();
    } else {
        TRACEX_W(QString("%1 zDelta:%2").arg(__FUNCTION__).arg(zDelta))
        return;
    }

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    PRINT_WHEEL_INFO(QString("%1 zDelta %2 deg:%3 pix:%4")
                         .arg(__FUNCTION__)
                         .arg(zDelta)
                         .arg(numDegrees.isNull() ? -1 : numDegrees.y())
                         .arg(numPixels.isNull() ? -1 : numPixels.y()))

    ForceVScrollBitmap();

    if (m_keys[Qt::Key_C]) {
        /* Scroll the color */
        g_cfg_p->m_log_GrayIntensity += zDelta;
        g_cfg_p->m_log_GrayIntensity = g_cfg_p->m_log_GrayIntensity > 240 ? 240 : g_cfg_p->m_log_GrayIntensity;
        g_cfg_p->m_log_GrayIntensity = g_cfg_p->m_log_GrayIntensity < 0 ? 0 : g_cfg_p->m_log_GrayIntensity;

        UpdateGrayFont();
        return;
    } else if (m_keys[Qt::Key_H]) {
        /* Scroll the color lighter */
        g_cfg_p->m_log_FilterMatchColorModify += zDelta / 2;
        g_cfg_p->m_log_FilterMatchColorModify = g_cfg_p->m_log_FilterMatchColorModify > 0xff ?
                                                0xff : g_cfg_p->m_log_FilterMatchColorModify;
        g_cfg_p->m_log_FilterMatchColorModify = g_cfg_p->m_log_FilterMatchColorModify < 0 ?
                                                0 : g_cfg_p->m_log_FilterMatchColorModify;

        GetDocument()->m_fontCtrl.UpdateAllFontsWith_fontMod(FONT_MOD_COLOR_MOD);
        m_EraseBkGrndDisabled = true;
        update();
        return;
    }

    if (doc_p->m_database.TIA.rows == 0) {
        return;
    }

    if (m_CTRL_Pressed) {
        if (zDelta < 0) {
            MW_ModifyFontSize(-1);
        } else {
            MW_ModifyFontSize(1);
        }
    } else {
        /* select next top line */
        int delta;
        int scrollLines = (zDelta / WHEEL_ANGLE_PER_TICK) * g_cfg_p->m_v_scrollSpeed;

        if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
            int dummy;

            delta = zDelta < 0 ? 1 : -1;
            scrollLines *= -1 * delta;  /* If delta is positive then scrollLines is negative, and need to get positive
                                         * */

            SearchFilteredRows_TIA(m_topLine, scrollLines, (zDelta > 0 ? true : false), &m_topLine, &dummy);
        } else {
            m_topLine -= scrollLines;

            m_topLine = (m_topLine + m_maxDisplayRows) < doc_p->m_database.TIA.rows ?
                        m_topLine : doc_p->m_database.TIA.rows - m_maxDisplayRows;
            m_topLine = m_topLine < m_min_topLine ? m_min_topLine : m_topLine;
        }

        if (m_maxDisplayRows >= doc_p->m_database.TIA.rows) {
            m_relPos = m_min_rel_pos;
        } else {
            UpdateRelPosition();
        }

        LimitTopLine();
    }

    m_EraseBkGrndDisabled = true;
    update();
}

/***********************************************************************************************************************
*   PageLeftRight
***********************************************************************************************************************/
void CEditorWidget::PageLeftRight(bool left, int charsToScroll)
{
    if (charsToScroll == 0) {
        double relToScroll = static_cast<double>(m_hscrollSlider.width()) / static_cast<double>(m_hscrollFrameWidth);
        m_hrelPos += relToScroll * (left ? -1 : 1);
    } else {
        static QString dummy = "aaaaaaaaaaaaaaaaaa";
        QSize textPixSize = GetTabbedSize(dummy, m_blackFont_p->font_p);
        double hscrollPixels = (charsToScroll * textPixSize.width()) / dummy.size();

        /* If scrolling is required to the left then charsToScroll will have a negative value */
        m_hrelPos += hscrollPixels / static_cast<double>(m_hscrollFrameWidth);
    }

    m_hrelPos = m_hrelPos > 1.0 ? 1.0 : m_hrelPos;
    m_hrelPos = m_hrelPos < 0.0 ? 0.0 : m_hrelPos;

    update();
}

/***********************************************************************************************************************
*   HorizontalCursorFocus
***********************************************************************************************************************/
void CEditorWidget::HorizontalCursorFocus(HCursorScrollAction_e scrollAction)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    /* In the bitmap where all is blitted...
     *
     * m_textRow_X.left;  ... start pixel position of first letter in a row.
     * m_textRow_X.right; ... max pixel position in the bitmap of the longest line */
    QSize stringPixelOffset(0, 0);
    int cursorPixelStart;
    char *text_p;
    int textSize;

    doc_p->GetTextItem(m_cursorSel.row, &text_p, &textSize);

    TRACEX_DE("cursorFocus m_hbmpOffset:%d m_hrelPos:%f", m_hbmpOffset, m_hrelPos)

    /* m_hbmpOffset = (long)(m_hrelPos * (m_bmpWindow.Width() - rcClient.Width())); */

    if (m_bmpWindow.width() <= m_rcClient.width()) {
        m_hrelPos = 0.0;
        update();
        return;
    }

    if ((scrollAction == HCursorScrollAction_Right_en) ||
        (scrollAction == HCursorScrollAction_End_en)) {
        stringPixelOffset = GetTabbedSize(text_p, m_cursorSel.startCol + 1, m_blackFont_p->font_p);
    } else {
        if (m_cursorSel.startCol > 1) {
            stringPixelOffset = GetTabbedSize(text_p, m_cursorSel.startCol - 1, m_blackFont_p->font_p);
        }
    }

    cursorPixelStart = stringPixelOffset.width() + m_textRow_X.left();      /* The pixel where the cursor would like to
                                                                             * be shown, in the BMP */

    /* Check if scroll needed */

    int currentHorizOffset = static_cast<int>(m_hrelPos * (m_bmpWindow.width() - m_rcClient.width()));

    /* BMP not usable for text, m_textRow_X.left
     * Screen not usable to the right, m_hscrollFrame.Width() */
    int leftSideWaste = m_textRow_X.left() - currentHorizOffset;
    leftSideWaste = leftSideWaste > 0 ? leftSideWaste : 0;

    int rightSideWaste = m_vscrollFrame.width();
    int maxTextWidthVisible = m_rcClient.width() - (rightSideWaste + leftSideWaste);
    int textPixelsShownOnScreen_X1 = currentHorizOffset + leftSideWaste;
    int textPixelsShownOnScreen_X2 = textPixelsShownOnScreen_X1 + maxTextWidthVisible;

    if ((cursorPixelStart <= textPixelsShownOnScreen_X1) ||
        (cursorPixelStart >= textPixelsShownOnScreen_X2)) {
        /* Define desired display position */

        int desiredScreenX;

        switch (scrollAction)
        {
            case HCursorScrollAction_Right_en:
            case HCursorScrollAction_End_en:
                desiredScreenX = m_vscrollFrame.left() - 5;
                break;

            case HCursorScrollAction_Left_en:
            case HCursorScrollAction_Home_en:
                desiredScreenX = m_hscrollFrame.left() + 5;
                break;

            case HCursorScrollAction_Focus_en:
                desiredScreenX = m_hscrollFrame.width() / 2;
                if (cursorPixelStart < desiredScreenX) {
                    m_hrelPos = 0.0;
                    update();
                    return;
                }
                break;
        }

        int bmpOffset = cursorPixelStart - desiredScreenX;

        if (bmpOffset > (m_bmpWindow.width() - m_rcClient.width())) {
            m_hrelPos = 1.0;
            update();
            return;
        }

        if (bmpOffset > 0) {
            m_hrelPos = bmpOffset / static_cast<double>((m_bmpWindow.width() - m_rcClient.width()));
        } else {
            /* backup... */
            m_hrelPos = 0.0;
        }
    }

    if (m_hrelPos > 1.0) {
        m_hrelPos = 1.0;
    } else if (m_hrelPos < 0.0) {
        m_hrelPos = 0.0;
    }

    if (m_cursorSel.startCol == 0) {
        m_hrelPos = 0.0;
    }

    update();
}

/***********************************************************************************************************************
*   UpdateRelPosition
***********************************************************************************************************************/
void CEditorWidget::UpdateRelPosition(void)
{
    /* This function calculates the m_relPos. It is located in the middle of the currently displayed rows.
     *
     * Ex.
     *      TopLine = 10, m_maxDisplayRows = 20, TIA.rows        = 10
     *      As such, on-screen we have rows 10-20. The middle of these rows are 15. m_relPos is then 15/100 *//* Min
     * value of m_relPos should be m_maxDisplayRows/2, and
     * Max value should be (doc_p->m_database.TIA.rows - m_maxDisplayRows/2) / doc_p->m_database.TIA.rows *//* When
     * showing filtered lines then the relative rowIndex needs to be translated from the position among the filters
     * the row has */
    if (m_topLine < 0) {
        m_topLine = 0;
    }

    CLogScrutinizerDoc *doc_p = GetDocument();
    double old_relPos = m_relPos;
    double rowIndex_f = static_cast<double>(m_topLine);

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        if (m_topLine >= 0) {
            /* must be an index larger than -1 */
            rowIndex_f = static_cast<double>(doc_p->m_rowCache_p->GetFilterIndex(m_topLine));  /* 0 based */
        }
    }

    if (m_maxDisplayRows < m_totalNumOfRows) {
        /* Removing m_maxDisplayRows to compensate for 1/2 * m_maxDisplayRows at the top and bottom that cannot be seen
         * */
        m_relPos = (rowIndex_f + (m_maxDisplayRows / 2.0)) / (m_totalNumOfRows - m_maxDisplayRows);
    } else {
        m_relPos = m_min_rel_pos;
    }

    if (m_relPos < m_min_rel_pos) {
        m_relPos = m_min_rel_pos;
    } else if (m_relPos > m_max_rel_pos) {
        m_relPos = m_max_rel_pos;
    }

    CheckRelPosition();

    if (!almost_equal(old_relPos, m_relPos)) {
        UpdateCursor(false, m_cursorSel.row, m_cursorSel.startCol);
    }
}

/***********************************************************************************************************************
*   UpdateRelTopLinePosition
*
* The topLine is used to locate which row should be displayed first on the screen, at index 0. topLine is used
* throughout the editorWidget as a reference point. m_relPos is a double that exactly describes where the middle of
* the screen is from the entire log view point, considering the presentation mode as well.
* This function uses the current relPos to locate the row thats in the middle of the screen, and then move from there
* a half number of screen rows up to locate the topRow.
***********************************************************************************************************************/
void CEditorWidget::UpdateRelTopLinePosition(void)
{
    CheckRelPosition();

    const auto middleIndex = static_cast<int>((m_relPos * static_cast<double>(m_totalNumOfRows - m_maxDisplayRows)));
    auto topIndex = static_cast<int>(middleIndex - static_cast<double>((m_maxDisplayRows) / 2.0) + 0.5);

    if (topIndex < 0) {
        topIndex = 0;
    }

    m_topLine = topIndex;

    if ((m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) && (m_totalNumOfRows > 0)) {
        CLogScrutinizerDoc *doc_p = GetDocument();
        if (m_maxDisplayRows >= m_totalNumOfRows) {
            m_topLine = doc_p->m_database.packedFIRA_p[0].row;
        } else if (topIndex >= 0) {
            m_topLine = doc_p->m_database.packedFIRA_p[topIndex].row;
        }
    }

    UpdateRelPosition();
}

/***********************************************************************************************************************
*   CheckRelPosition
***********************************************************************************************************************/
void CEditorWidget::CheckRelPosition(void)
{
    /* Min value of m_relPos should be m_maxDisplayRows/2, and
     * Max value should be (doc_p->m_database.TIA.rows - m_maxDisplayRows/2) / doc_p->m_database.TIA.rows */

    m_hrelPos = m_hrelPos > 1.0 ? 1.0 : m_hrelPos;
    m_hrelPos = m_hrelPos < 0.0 ? 0.0 : m_hrelPos;

    if (m_relPos < m_min_rel_pos) {
#ifdef _DEBUG
        TRACEX_D("CEditorWidget::CheckRelPosition  Too low m_relPos  %f < %f ", m_relPos, m_min_rel_pos)
#endif
        m_relPos = m_min_rel_pos;
    }

    if (m_relPos > m_max_rel_pos) {
#ifdef _DEBUG
        TRACEX_D("CEditorWidget::CheckRelPosition  Too low m_relPos  %f > %f ", m_relPos, m_max_rel_pos)
#endif
        m_relPos = m_max_rel_pos;
    }
}

/***********************************************************************************************************************
*   CheckVScrollSliderPosition
***********************************************************************************************************************/
void CEditorWidget::CheckVScrollSliderPosition(void)
{
    if (m_rockScrollInfo.numberOfItems == m_vscrollFrame.height()) {
        RockScrollItem_t *startItem_p = nullptr;
        RockScrollItem_t *endItem_p = nullptr;
        bool found = false;
        int index = 0;
        const int endLine = m_topLine + m_maxDisplayRows;
        int y_start_raster = 0;
        int y_end_raster = 0;

        /* Find the start raster */
        while (!found && index < m_rockScrollInfo.numberOfItems) {
            RockScrollItem_t *item_p = &m_rockScrollInfo.itemArray_p[index];
            RockScrollItem_t *nextItem_p = index <
                                           m_rockScrollInfo.numberOfItems -
                                           1 ? &m_rockScrollInfo.itemArray_p[index + 1] : nullptr;

            if ((((item_p->bestRow != 0) && (m_topLine <= item_p->bestRow)) ||
                 ((item_p->bestRow == 0) && (m_topLine >= item_p->startRow))) &&
                (m_topLine <= item_p->endRow)) {
                startItem_p = item_p;
                found = true;
                y_start_raster = item_p->y_line;
            } else if ((nextItem_p != nullptr) && (m_topLine < nextItem_p->startRow)) {
                startItem_p = nextItem_p;
                found = true;
                y_start_raster = nextItem_p->y_line;
            } else {
                ++index;
            }
        }

        if (found) {
            found = false;

            /* Find the end raster */
            while (!found && index < m_rockScrollInfo.numberOfItems) {
                RockScrollItem_t *item_p = &m_rockScrollInfo.itemArray_p[index];

                /*RockScrollItem_t* prevItem_p  = index > 0 ? &m_rockScrollInfo.itemArray_p[index - 1] : nullptr; */
                RockScrollItem_t *nextItem_p = index <
                                               m_rockScrollInfo.numberOfItems -
                                               1 ? &m_rockScrollInfo.itemArray_p[index + 1] : nullptr;

                if ((((item_p->bestRow != 0) && (endLine <= item_p->bestRow)) ||
                     ((item_p->bestRow == 0) && (endLine <= item_p->endRow))) &&
                    (endLine >= item_p->startRow)) {
                    endItem_p = item_p;
                    found = true;
                    y_end_raster = item_p->y_line;
                } else if ((nextItem_p != nullptr) && (nextItem_p->startRow > endLine)) {
                    endItem_p = nextItem_p;
                    found = true;
                    y_end_raster = nextItem_p->y_line;
                } else {
                    ++index;
                }
            }
        }

        if (found) {
            TRACEX_DE("CEditorWidget::CheckVScrollSliderPosition  row_start_end (%d, %d) item_start_end "
                      "(%d, %d) y_raster _start (%d, %d)  y_raster_end (%d, %d)",
                      m_topLine, endLine, startItem_p != nullptr ? startItem_p->startRow : -1,
                      endItem_p != nullptr ? endItem_p->endRow : -1, y_start_raster, m_vscrollSlider.top(),
                      y_end_raster, m_vscrollSlider.bottom())

            if (y_start_raster != m_vscrollSlider.top()) {
                m_vscrollSlider.setTop(y_start_raster);
            }

            if (y_end_raster != m_vscrollSlider.bottom()) {
                m_vscrollSlider.setBottom(y_end_raster);
            }

            if (y_start_raster == y_end_raster) {
                m_vscrollSlider.setBottom(m_vscrollSlider.bottom() + 1);
            }
        }
    }
}

/***********************************************************************************************************************
*   OnLButtonDown
***********************************************************************************************************************/
void CEditorWidget::OnLButtonDown(Qt::KeyboardModifiers modifiers, ScreenPoint_t& screenPoint)
{
    bool invalidate = false;

    if (!m_inFocus) {
        setFocus();
    }

    CLogScrutinizerDoc *doc_p = GetDocument();

    if (doc_p->m_database.TIA.rows == 0) {
        return;
    }

    if ((((screenPoint.mouse.x() >= m_vscrollSlider.left()) && (screenPoint.mouse.x() <= m_vscrollSlider.right())) &&
         ((screenPoint.mouse.y() >= m_vscrollSlider.top()) && (screenPoint.mouse.y() <= m_vscrollSlider.bottom())))
        ||
        (m_vscrollBitmapEnabled &&
         (((screenPoint.mouse.x() >= m_vScrollBMP_Rect.left()) &&
           (screenPoint.mouse.x() <= m_vScrollBMP_Rect.right())) &&
          ((screenPoint.mouse.y() >= m_vScrollBMP_Rect.top()) &&
           (screenPoint.mouse.y() <= m_vScrollBMP_Rect.bottom()))))) {
        /*Scroll slider/Pane */
        if (!m_captureOn) {
            m_captureOn = true;
        }

        invalidate = SetVScrollGlue(true);
        m_vscrollSliderGlueOffset = screenPoint.mouse.y() - m_vscrollSlider.top();
    } else if (((screenPoint.mouse.x() >= m_vscrollFrame.left()) &&
                (screenPoint.mouse.x() <= m_vscrollFrame.right())) &&
               ((screenPoint.mouse.y() >= m_vscrollFrame.top()) &&
                (screenPoint.mouse.y() <= m_vscrollFrame.bottom()))) {
        /* Vertical scroll Page up down */

        if (screenPoint.mouse.y() < m_vscrollSlider.top()) {
            /* Pageup */
            PageUpDown(true);
        } else {
            PageUpDown(false);
        }
    } else if ((screenPoint.mouse.y() > m_hscrollSlider.top()) && (screenPoint.mouse.y() < m_hscrollSlider.bottom())) {
        /* Horizontal scroll bar */

        if ((screenPoint.mouse.x() >= m_hscrollSlider.left()) && (screenPoint.mouse.x() <= m_hscrollSlider.right())) {
            if (!m_captureOn) {
                m_captureOn = true;
            }
            m_hscrollSliderGlue = true;
            invalidate = true; /* to repaint the scrollbars */
            m_hscrollSliderGlueOffset = screenPoint.mouse.x() - m_hscrollSlider.left();
        } else if ((screenPoint.mouse.x() < m_hscrollSlider.left()) && !m_hscrollSliderGlue) {
            /* scroll to left */
            PageLeftRight(true);
        } else if ((screenPoint.mouse.x() > m_hscrollSlider.right()) && !m_hscrollSliderGlue) {
            PageLeftRight(false);
        }
    } else if ((screenPoint.mouse.y() > m_textWindow.top()) && (screenPoint.mouse.x() > m_textWindow.left()) &&
               (screenPoint.mouse.x() < m_textWindow.right()) && (screenPoint.mouse.y() < m_textWindow.bottom())) {
        /* Text window */

        if (!(modifiers & Qt::ControlModifier) &&   /* The full row selection works fine in textWindow_SelectionUpdate,
                                                     * hence use that instead */
            !(modifiers & Qt::ShiftModifier) &&
            (screenPoint.DCBMP.x() < (m_textRow_X.left() - m_beam_left_padding)) &&
            (screenPoint.mouse.y() > m_vscrollFrame.top()) && (screenPoint.mouse.y() < m_vscrollFrame.bottom())) {
            /* Full row selection */

            int screenRow;
            int screenCol;

            if (PointToCursor(&screenPoint, &screenRow, &screenCol)) {
                int TIA_row = CursorTo_TIA_Index(screenRow);

                EmptySelectionList();

                AddSelection(TIA_row, -1, -1, true, true);
                AddDragSelection(TIA_row, 0, 0);

                DisableCursor();

                invalidate = true;
            }
        } else {
            invalidate = textWindow_SelectionUpdate(&screenPoint, LS_LG_EVENT_LMOUSE_DOWN_e, modifiers);
        }

        m_LMousePressed = true;
    } else if (screenPoint.mouse.y() < m_textWindow.top()) {
        /*m_window */

        if ((g_cfg_p->m_Log_colClip_Start > CFG_CLIP_NOT_SET) || (g_cfg_p->m_Log_colClip_End > CFG_CLIP_NOT_SET)) {
            if ((screenPoint.mouse.x() >= m_colClip_StartRect.left()) &&
                (screenPoint.mouse.x() <= m_colClip_StartRect.right())) {
                g_cfg_p->m_Log_colClip_Start = CFG_CLIP_NOT_SET;
            } else if ((screenPoint.mouse.x() >= m_colClip_EndRect.left()) &&
                       (screenPoint.mouse.x() <= m_colClip_EndRect.right())) {
                g_cfg_p->m_Log_colClip_End = CFG_CLIP_NOT_SET;
            }
        }

        /*    TRACEX_DE("CEditorWidget::OnMouseMove  Above text window") */
    }

    if (invalidate) {
        m_EraseBkGrndDisabled = true;
        MW_updateLogFileTrackState(false); /* Any click in the log window disables the log file tracking. */
        update();
    }
}

/***********************************************************************************************************************
*   OnLButtonUp
***********************************************************************************************************************/
void CEditorWidget::OnLButtonUp(ScreenPoint_t& screenPoint)
{
    bool invalidate = false;
    bool mouseOverBitmap = false;

    if (m_captureOn) {
        m_captureOn = false;
    }

    CLogScrutinizerDoc *doc_p = GetDocument();

    if (doc_p->m_database.TIA.rows == 0) {
        return;
    }

    /* If mouse is still over the slider bitmap keep it visible */

    if (((screenPoint.mouse.x() >= (m_vscrollFrame.left())) && (screenPoint.mouse.x() < (m_vscrollFrame.right()))) ||
        (m_vscrollBitmapEnabled && ((screenPoint.mouse.x() >= m_vScrollBMP_Rect.left()) &&   /*hysteresis, if bug is
                                                                                              * shown... show it while
                                                                                              * over it */
                                    ((screenPoint.mouse.y() >= m_vScrollBMP_Rect.top()) &&
                                     (screenPoint.mouse.y() <= m_vScrollBMP_Rect.bottom()))))) {
        mouseOverBitmap = true;
    }

    invalidate = textWindow_SelectionUpdate(&screenPoint, LS_LG_EVENT_LMOUSE_UP_e, Qt::NoModifier);

    if (m_vscrollSliderGlue || m_hscrollSliderGlue) {
        SetVScrollGlue(false);

        m_hscrollSliderGlue = false;
        invalidate = true;
    }

    if (mouseOverBitmap) {
        SetVScrollBitmap(true);
    }

    if (invalidate) {
        m_EraseBkGrndDisabled = true;
        update();
    }
}

/***********************************************************************************************************************
*   OnRButtonDown
***********************************************************************************************************************/
void CEditorWidget::OnRButtonDown(Qt::KeyboardModifiers modifiers, ScreenPoint_t& screenPoint)
{
    Q_UNUSED(modifiers)
    Q_UNUSED(screenPoint)

    if (!m_inFocus) {
        setFocus();
    }
}

/***********************************************************************************************************************
*   SetVScrollGlue
***********************************************************************************************************************/
bool CEditorWidget::SetVScrollGlue(bool enabled)
{
    if ((m_vscrollSliderGlue != enabled) || (m_vscrollBitmapEnabled != enabled)) {
        m_vscrollSliderGlue = enabled;
        SetVScrollBitmap(enabled);

#ifdef _DEBUG
        TRACEX_DE("CEditorWidget::SetVScrollGlue  SWITCH:%d", enabled)
#endif
        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   SetVScrollBitmap
***********************************************************************************************************************/
bool CEditorWidget::SetVScrollBitmap(bool enabled)
{
    if (enabled) {
        m_vscrollTimer->start(VSCROLL_TIMER_DURATION);
    }
#ifdef _DEBUG
    TRACEX_DE(enabled ? "VScroll ENABLED" : "VScroll DISABLED")
#endif

    if ((m_vscrollSliderGlue || m_vscrollBitmapForce) && !m_vscrollBitmapEnabled) {
        /* override, align to vscroll glue */
#ifdef _DEBUG

        /*TRACEX_DE("CEditorWidget::SetVScrollBitmap  %d", 1) */
#endif
        m_vscrollBitmapEnabled = true;
        return true;
    }

    if (m_vscrollBitmapEnabled != enabled) {
        m_vscrollBitmapEnabled = enabled;
        return true;
    }

    m_vscrollBitmapEnabled = enabled;
    return false;
}

/***********************************************************************************************************************
*   ForceVScrollBitmap
***********************************************************************************************************************/
bool CEditorWidget::ForceVScrollBitmap(void)
{
    bool invalidate = m_vscrollBitmapEnabled ? false : true;
    TRACEX_DE(QString("%1").arg(__FUNCTION__))
    m_vscrollBitmapForce = true;
    m_vscrollBitmapEnabled = true;
    m_vscrollTimer->start(VSCROLL_TIMER_DURATION);
    return invalidate;
}

/***********************************************************************************************************************
*   HandleVScrollBitmapTimer
***********************************************************************************************************************/
void CEditorWidget::HandleVScrollBitmapTimer(void)
{
    m_vscrollTimer->stop();
    m_vscrollBitmapForce = false;

    bool mouseOverBitmap = false;
    ScreenPoint_t screenPoint = MakeScreenPoint(this, mapFromGlobal(QCursor::pos()));

    /*hysteresis, if bug is shown show it while over it */
    if (((screenPoint.mouse.x() >= (m_vscrollFrame.left())) && (screenPoint.mouse.x() < (m_vscrollFrame.right()))) ||
        (m_vscrollBitmapEnabled && ((screenPoint.mouse.x() >= m_vScrollBMP_Rect.left()) &&
                                    ((screenPoint.mouse.y() >= m_vScrollBMP_Rect.top()) &&
                                     (screenPoint.mouse.y() <= m_vScrollBMP_Rect.bottom()))))) {
        mouseOverBitmap = true;
    }

    if (!m_vscrollSliderGlue && !mouseOverBitmap) {
        SetVScrollBitmap(false);
    } else {
        m_vscrollTimer->start(VSCROLL_TIMER_DURATION);
    }

    TRACEX_DE(QString("%1 glue:%2 mouseOver:%3").arg(__FUNCTION__).arg(m_vscrollSliderGlue).arg(mouseOverBitmap))
    update();
}

/***********************************************************************************************************************
*   mouseDoubleClickEvent
***********************************************************************************************************************/
void CEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        bool invalidate = false;
        CLogScrutinizerDoc *doc_p = GetDocument();

        if (doc_p->m_database.TIA.rows == 0) {
            return;
        }

        ScreenPoint_t screenPoint = MakeScreenPoint(e, m_hbmpOffset, m_vbmpOffset);

        if (((screenPoint.mouse.x() >= m_vscrollSlider.left()) && (screenPoint.mouse.x() <= m_vscrollSlider.right())) &&
            ((screenPoint.mouse.y() >= m_vscrollSlider.top()) && (screenPoint.mouse.y() <= m_vscrollSlider.bottom()))) {
            invalidate = SetVScrollGlue(false); /* to repaint the scrollbars */
        } else if (((screenPoint.mouse.x() >= m_vscrollFrame.left()) &&
                    (screenPoint.mouse.x() <= m_vscrollFrame.right())) &&        /* Page up down */
                   ((screenPoint.mouse.y() >= m_vscrollFrame.top()) &&
                    (screenPoint.mouse.y() <= m_vscrollFrame.bottom()))) {
            if (screenPoint.mouse.y() < m_vscrollSlider.top()) {
                /* Pageup */
                PageUpDown(true);
            } else {
                PageUpDown(false);
            }

            invalidate = SetVScrollGlue(false);  /* to repaint the scrollbars */
        } else if ((screenPoint.mouse.y() > m_textWindow.top()) && (screenPoint.mouse.x() > m_textWindow.left()) &&
                   (screenPoint.mouse.x() < m_textWindow.right()) && (screenPoint.mouse.y() < m_textWindow.bottom())) {
            /* Text window */
            m_LMousePressed = true;
            invalidate = textWindow_SelectionUpdate(&screenPoint, LS_LG_EVENT_LMOUSE_DBL_e, e->modifiers());
        }

        if (invalidate) {
            m_EraseBkGrndDisabled = true;
        }
    }

    e->accept();
}

/***********************************************************************************************************************
*   CursorUpDown
***********************************************************************************************************************/
void CEditorWidget::CursorUpDown(bool up)
{
/* If there is a selection
 *
 *  1. Shift
 *      a. pressed - Extend selection
 *
 *  2. Control
 *      a. pressed - Scroll entire window, cursor fixed
 *
 * When no function key is pressed only cursor moves */
    CLogScrutinizerDoc *doc_p = GetDocument();
    int cursorRow = m_cursorSel.row;
    int cursorCol = m_cursorSel.startCol;
    int searchRow = 0;
    int delta = up ? -1 : 1;

    ForceVScrollBitmap();

    if (!m_CTRL_Pressed) {
        /*
         * 1. No keys (SHIFT-CTRL) pressed
         * Move the start such that the search start at the next index
         */
        cursorRow += delta;

        if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
            if (!SearchFilteredRows_TIA(cursorRow, 1, up, &searchRow)) {
                TRACEX_DE("CEditorWidget::CursorUpDown NO scroll, cursor at filtered top")

                /* No new filtered line could be found */
                return;
            }

            /* A new filtered line has been found */
            cursorRow = searchRow;

            /* Is it necessary to scroll the lines on the screen */
            if (cursorRow < m_screenRows[0].row) {
                TRACEX_DE("CEditorWidget::CursorUpDown NO scroll, cursor at top")

                /* scrolling up */
                m_topLine = cursorRow;
            } else if ((m_maxDisplayRows < m_totalNumOfRows) && (cursorRow > m_screenRows[m_maxDisplayRows - 1].row)) {
                int tempRemains = 0;

                /* Find the next filtered line below the top line, to scroll down */
                SearchFilteredRows_TIA(cursorRow, m_maxDisplayRows, true, &m_topLine, &tempRemains);
            }
        } else {
            /* Check movement of screen...
             * Check if the new selection when outside of the present screen rows, is such case all lines must be
             * scrolled as well */
            if (cursorRow < m_topLine) {
                cursorRow = cursorRow < 0 ? 0 : cursorRow;
                m_topLine = cursorRow;
            } else if ((cursorRow > m_topLine + m_maxDisplayRows - 1) ||   /* cursor beyond current screen rows */
                       (cursorRow >= doc_p->m_database.TIA.rows - 1)) {
                /* cursor beyond log end */

                /* scroll screen down */

                if (cursorRow >= doc_p->m_database.TIA.rows - 1) {
                    cursorRow = doc_p->m_database.TIA.rows - 1;
                    m_topLine = doc_p->m_database.TIA.rows - m_maxDisplayRows;
                } else {
                    m_topLine = cursorRow - (m_maxDisplayRows - 1);
                }

                m_topLine = m_topLine < m_min_topLine ? m_min_topLine : m_topLine;
            }
        } /* presentation mode */

        /* Check and adjust the cursor col pos, in-case the current col pos doens't exist on the new row
         * The desiredCol is used to put the cursor where the user wanted the cursor to be, but might be prevented by
         * too short rows */

        if (m_cursorSel.row != cursorRow) {
            if (m_cursorDesiredCol == doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size) {
                /* Tracking the end of the row */
                m_cursorDesiredCol = doc_p->m_database.TIA.textItemArray_p[cursorRow].size;
                cursorCol = m_cursorDesiredCol;
            } else if (m_cursorDesiredCol >= doc_p->m_database.TIA.textItemArray_p[cursorRow].size) {
                cursorCol = doc_p->m_database.TIA.textItemArray_p[cursorRow].size;
            } else if (cursorCol >= doc_p->m_database.TIA.textItemArray_p[cursorRow].size) {
                cursorCol = doc_p->m_database.TIA.textItemArray_p[cursorRow].size;
            } else if ((m_cursorDesiredCol != cursorCol) &&
                       (m_cursorDesiredCol <= (doc_p->m_database.TIA.textItemArray_p[cursorRow].size - 1))) {
                cursorCol = m_cursorDesiredCol;
            }
        }

        /* Figure out the selections */

        /* Rules, going up
         *
         * New row
         * 1.1 if there is no selection on the new row, add a selection on that row from the end to the cursor
         * 1.2 if there is a selection already then
         *   1.2.1 if the selection starts before the cursor shorten the selection to end where the cursor is
         *   1.2.2 if the selection starts after where the cursor is then move the selection to start where the cursor
         * is and end where the selection was
         *     1.2.2.1 If the selection started where the cursor is then it shall be removed *//* Old row
         * 1.3 If there is no selection on the old row, add a selection from the old cursorCol to the beginning of the
         * row
         * 1.4 if there is a selection on the row
         * 1.4.1  If it start before the cursor then replace it with a new selection that starts from the first letter
         * and stretch to where the cursor was.
         *   1.4.1.1 If the selection was from the beginning already it shall be removed
         * 1.4.2 If the selection extends to after the old cursor pos it is extended to the start of the row */
        if (m_SHIFT_Pressed && (m_cursorSel.row != cursorRow)) {
            CSelection *selection_p;
            bool newSelected;
            bool oldSelected;

            if ((m_origDragSelection.row == cursorRow) && (m_origDragSelection.startCol == cursorCol)) {
                EmptySelectionList();

                HorizontalCursorFocus(HCursorScrollAction_Focus_en);

                LimitTopLine();
                UpdateRelPosition();

                UpdateCursor(false, cursorRow, cursorCol);
                update();

                return;
            }

            if (m_selectionList.isEmpty()) {
                /*
                 * Add the initial selection
                 * Mark where first selection originates from (where the cursor previously was
                 */
                AddDragSelection(m_cursorSel.row, m_cursorSel.startCol, m_cursorSel.startCol);
                if (up) {
                    if (doc_p->m_database.TIA.textItemArray_p[cursorRow].size != 0) {
                        if (cursorCol <= doc_p->m_database.TIA.textItemArray_p[cursorRow].size - 1) {
                            auto endCol = doc_p->m_database.TIA.textItemArray_p[cursorRow].size - 1;
                            AddSelection(cursorRow, cursorCol, endCol, true, false, false);
                        } else {
                            /*AddSelection(cursorRow, cursorCol, cursorCol, true, false, false); */
                        }
                    } else {
                        AddSelection(cursorRow, 0, 0, true, false, false);
                    }

                    if (m_cursorSel.startCol > 0) {
                        AddSelection(m_cursorSel.row, 0, m_cursorSel.startCol - 1, true, false, false);
                    }
                } else {
                    /* down */
                    if (cursorCol > 0) {
                        AddSelection(cursorRow, 0, cursorCol - 1, true, false, false);
                    }

                    if (doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size != 0) {
                        if (m_cursorSel.startCol <= doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1) {
                            auto endCol = doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1;
                            AddSelection(m_cursorSel.row, m_cursorSel.startCol, endCol, true, false, false);
                        }
                    } else {
                        /* Text line is of 0 length... if * multiple copy it is expected to get * an empty line */
                        AddSelection(m_cursorSel.row, 0, 0, true, false, false);
                    }
                }
            } /* selection list empty */
            else {
                /* Update with the new selection, and update the row we are moving away from. */

                if (up) {
                    /* New row */

                    newSelected = isRowSelected(cursorRow, &selection_p);

                    if (m_origDragSelection.row <= cursorRow) {
                        /* moving up with orig above, must be a selection above us */
                        if (!newSelected) {
                            TRACEX_W("CEditorWidget::CursorUpDown There should be a selection above")
                            return;
                        }

                        if (selection_p->startCol < cursorCol) {
                            /* 1.2.1 */
                            selection_p->endCol = cursorCol - 1;
                            selection_p->endCol = selection_p->endCol < 0 ? 0 : selection_p->endCol;

                            if (selection_p->startCol == -1) {
                                selection_p->startCol = 0;
                            }

                            SelectionUpdated(selection_p);
                        } else if (selection_p->startCol > cursorCol) {
                            /* 1.2.2 */
                            selection_p->endCol = selection_p->startCol - 1;
                            selection_p->startCol = cursorCol;
                            selection_p->endCol = selection_p->endCol >= 0 ? selection_p->endCol : 0;

                            if (selection_p->startCol == selection_p->endCol) {
                                RemoveSelection(selection_p->row);
                                if (m_selectionList.isEmpty()) {
                                    AddDragSelection(cursorRow, selection_p->endCol, selection_p->endCol);
                                }
                            } else {
                                SelectionUpdated(selection_p);
                            }
                        } else if (selection_p->startCol == cursorCol) {
                            RemoveSelection(selection_p->row);
                        }
                    } else {
                        /* not selected */
                        if ((doc_p->m_database.TIA.textItemArray_p[cursorRow].size != 0) &&
                            (cursorCol < (doc_p->m_database.TIA.textItemArray_p[cursorRow].size - 1))) {
                            /* if cursor is at end of the row then there shouldn't be a selection added */
                            AddSelection(cursorRow,
                                         cursorCol,
                                         doc_p->m_database.TIA.textItemArray_p[cursorRow].size - 1,
                                         true,
                                         false,
                                         false); /* 1.1 */
                        }
                    }

                    /* Old row */
                    oldSelected = isRowSelected(m_cursorSel.row, &selection_p);
                    if (m_origDragSelection.row == m_cursorSel.row) {
                        if (!oldSelected) {
                            TRACEX_W("CEditorWidget::CursorUpDown There should be a selection on this row")
                            return;
                        } /* abort */

                        if (selection_p->endCol > m_origDragSelection.startCol) {
                            selection_p->endCol = selection_p->startCol > 0 ? selection_p->startCol - 1 : 0;
                        }

                        selection_p->startCol = 0;

                        if (selection_p->endCol == 0) {
                            RemoveSelection(selection_p->row);
                            if (m_selectionList.isEmpty()) {
                                AddDragSelection(cursorRow, 0, 0);
                            }
                        } else {
                            SelectionUpdated(selection_p);
                        }
                    } else if (m_origDragSelection.row < m_cursorSel.row) {
                        if (oldSelected) {
                            RemoveSelection(m_cursorSel.row);
                        }
                    } else {
                        if (!oldSelected) {
                            auto endCol = doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1;
                            AddSelection(m_cursorSel.row, 0, endCol, true, false, false);
                        } else {
                            selection_p->startCol = 0;
                            selection_p->endCol = doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size !=
                                                  0 ? doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size -
                                                  1 : 0;
                        }
                    }
                } else {
                    /*
                     * down
                     * New row
                     */
                    newSelected = isRowSelected(cursorRow, &selection_p);

                    if (m_origDragSelection.row >= cursorRow) {
                        if (!newSelected) {
                            TRACEX_W("CEditorWidget::CursorUpDown There should be a selection above")
                            return;
                        } /* abort */

                        if (selection_p->endCol == cursorCol) {
                            RemoveSelection(selection_p->row);
                            if (m_selectionList.isEmpty()) {
                                AddDragSelection(cursorRow, cursorCol, cursorCol);
                            }
                        } else if (selection_p->endCol < cursorCol) {
                            /* xx */
                            selection_p->startCol = selection_p->endCol + 1;
                            selection_p->endCol = cursorCol - 1;

                            selection_p->startCol = selection_p->startCol > 0 ? selection_p->startCol : 0;
                            selection_p->endCol = selection_p->endCol <
                                                  (doc_p->m_database.TIA.textItemArray_p[selection_p->row].size) ?
                                                  selection_p->endCol :
                                                  (doc_p->m_database.TIA.textItemArray_p[selection_p->row].size) - 1;

                            SelectionUpdated(selection_p);
                        } else if (selection_p->startCol <= cursorCol) {
                            /*xx */
                            selection_p->startCol = cursorCol;

                            SelectionUpdated(selection_p);
                        }
                    } else {
                        if (cursorCol != 0) {
                            AddSelection(cursorRow, 0, cursorCol - 1, true, false, false); /* 1.3 */
                        }
                    }

                    /* Old row */
                    oldSelected = isRowSelected(m_cursorSel.row, &selection_p);

                    if (m_origDragSelection.row == m_cursorSel.row) {
#ifdef _DEBUG
                        if (!oldSelected) {
                            TRACEX_W("CEditorWidget::CursorUpDown There should be a selection on this row")
                            return;
                        } /* abort */
#endif

                        int startCol;

                        if (m_cursorSel.startCol <= m_origDragSelection.startCol) {
                            /* check if the selection is to the left of the orig start, in that case it should now start
                             * to the right of it */
                            startCol = m_origDragSelection.startCol + 1;
                        } else {
                            startCol = m_origDragSelection.startCol;
                        }

                        startCol = startCol < (doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1) ?
                                   startCol : doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1;

                        int endCol = doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size != 0 ?
                                     doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1 : 0;

                        if (startCol != endCol) {
                            selection_p->startCol = startCol;
                            selection_p->endCol = endCol;

                            SelectionUpdated(selection_p);
                        } else {
                            RemoveSelection(selection_p->row);
                            if (m_selectionList.isEmpty()) {
                                AddDragSelection(cursorRow, startCol, startCol);
                            }
                        }
                    } else if (m_origDragSelection.row > m_cursorSel.row) {
                        if (!oldSelected) {
                            return;
                        } /* abort */

                        RemoveSelection(m_cursorSel.row);
                    } else {
                        if (!oldSelected) {
                            auto endCol = (doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size !=
                                           0 ? doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1 : 0);
                            if (cursorCol == 0) {
                                AddSelection(m_cursorSel.row, 0, endCol, true, false, false);
                            } else {
#ifdef _DEBUG
                                TRACEX_W("CEditorWidget::CursorUpDown There should be a selection on the row above")
                                return; /* abort */
#endif
                            }
                        } else {
                            selection_p->startCol = 0;
                            selection_p->endCol = doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size !=
                                                  0 ? doc_p->m_database.TIA.textItemArray_p[m_cursorSel.row].size -
                                                  1 : 0;
                            SelectionUpdated(selection_p);
                        }
                    }
                }
            }  /* selection list not empty */
        } else {
            /* shift not pressed */
            EmptySelectionList();
            AddDragSelection(cursorRow, cursorCol, cursorCol);
        } /* */
    } /* shift pressed */
    else if (m_CTRL_Pressed) {
        /* Always shift the entire screen, do not move the cursor */

        if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
            if ((m_totalNumOfRows < m_maxDisplayRows) ||
                (up && (doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index == 0)) || /* scroll up but topLine is at
                                                                                       * first filter */
                (!up &&
                 ((doc_p->m_database.FIRA.filterMatches - doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index) <=
                  m_maxDisplayRows))) {
                /* scroll down but topLine is at last possible filter for the top */
                TRACEX_DE("CEditorWidget::CursorUpDown NO scroll possible")
                return;
            }

            if (!SearchFilteredRows_TIA(m_topLine + delta, 1, up, &searchRow)) {
                TRACEX_DE("CEditorWidget::CursorUpDown NO scroll possible, no filtered rows left")
                return;
            }

            m_topLine = searchRow;
            cursorRow = cursorRow < m_topLine ? m_topLine : cursorRow;

            /* Figure out if cursor might be below last row */
            int lastFilteredIndexOnScreen = doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index + m_maxDisplayRows - 1;

            if (lastFilteredIndexOnScreen > m_maxFIRAIndex) {
                lastFilteredIndexOnScreen = m_maxFIRAIndex;
            }

            int lastFilteredRowOnScreen = doc_p->m_database.packedFIRA_p[lastFilteredIndexOnScreen].row;
            cursorRow = cursorRow > lastFilteredRowOnScreen ? lastFilteredRowOnScreen : cursorRow;
        } else {
            if ((doc_p->m_database.TIA.rows < m_maxDisplayRows) ||
                (up && (m_topLine == m_min_topLine)) ||
                (!up && (m_min_topLine >= m_max_topLine))) {
                TRACEX_DE("CEditorWidget::CursorUpDown NO scroll possible, at top/bottom")
                return;
            }

            m_topLine += delta;

            /* Secure that the cursor is never outside the rows in display */

            cursorRow = cursorRow < m_topLine ? m_topLine : cursorRow;
            cursorRow = cursorRow > (m_topLine + m_maxDisplayRows - 1) ? m_topLine + m_maxDisplayRows - 1 : cursorRow;
        }
    } /* if CTRL pressed */

    UpdateCursor(false, cursorRow, cursorCol);

    HorizontalCursorFocus(HCursorScrollAction_Focus_en);

    LimitTopLine();
    UpdateRelPosition();

    m_EraseBkGrndDisabled = true;
    update();
}

/***********************************************************************************************************************
*   TogglePresentationMode
***********************************************************************************************************************/
void CEditorWidget::CursorLeftRight(bool left)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    int cursorRow = m_cursorSel.row;
    int cursorCol = m_cursorSel.startCol;
    char *text_p = nullptr;
    int size;
    doc_p->GetTextItem(cursorRow, &text_p, &size, nullptr);

    const int maxRowCol = size;

    if (!m_CTRL_Pressed) {
        cursorCol += left ? -1 : 1;
    } else {
        /*
         * CTRL is used to make special steps * http://support.microsoft.com/kb/290938?wa=wsignin1.0
         *//* step to the beginning of next word. Stop if space is detected *
         * switch between letters 0-9a-z and special characters
         *    111 ??111???111 */
        if (left) {
            cursorCol--;

            if (cursorCol > 0) {
                /* Search to the left until spaces ends and a char starts */
                bool foundSpaces = false;
                bool done = false;

                while (cursorCol > 0 && !done) {
                    if (!foundSpaces) {
                        /* first step over some spaces */
                        if (text_p[cursorCol] == 0x20) {
                            foundSpaces = true;
                        }
                    } else {
                        if (text_p[cursorCol] != 0x20) {
                            done = true;
                        }
                    }

                    if (done) {
                        cursorCol++;
                    } else {
                        --cursorCol;
                    }
                } /* while */
            }
        } else {
            /* Search to the left until spaces ends and a char starts */
            bool foundSpaces = false;
            bool done = false;

            while (cursorCol < maxRowCol && !done) {
                if (!foundSpaces) {
                    /* first step over some spaces */
                    if (text_p[cursorCol] == 0x20) {
                        foundSpaces = true;
                    }
                } else {
                    if (text_p[cursorCol] != 0x20) {
                        done = true;
                    }
                }

                if (!done) {
                    ++cursorCol;
                }
            } /* while */
        }
    }

    /* Is it necessary to scroll the columns on the screen */
    if (cursorCol < 0) {
        TRACEX_DE("CEditorWidget::CursorLeftRight NO scroll, cursor at 0")
        cursorCol = 0;
    }

    if (cursorCol >= maxRowCol) {
        cursorCol = maxRowCol;
    }

    if (left) {
        HorizontalCursorFocus(HCursorScrollAction_Left_en);
    } else {
        HorizontalCursorFocus(HCursorScrollAction_Right_en);
    }

    if (m_SHIFT_Pressed) {
        CSelection *selection_p;

        if (m_cursorSel.startCol != cursorCol) {
            /* did the cursor move */
            if (isRowSelected(cursorRow, &selection_p)) {
                if (left) {
                    if (selection_p->startCol > cursorCol) {
                        /* extend to the left */
                        selection_p->startCol = cursorCol;
                    } else {
                        selection_p->endCol = cursorCol - 1;
                    }
                } else {
                    if ((selection_p->endCol == selection_p->startCol) && (selection_p->endCol == (cursorCol - 1))) {
                        selection_p->startCol = selection_p->endCol + 1; /* remove */
                    } else if (selection_p->endCol >= cursorCol) {
                        /* extend to the left */
                        selection_p->startCol = cursorCol;
                    } else {
                        selection_p->endCol = cursorCol - 1;
                    }
                }

                if (selection_p->startCol > selection_p->endCol) {
                    RemoveSelection(cursorRow);
                }
            } else {
                /* not selected */
                if (m_cursorSel.startCol < cursorCol) {
                    /* right */
                    AddSelection(cursorRow, m_cursorSel.startCol, cursorCol - 1, true, false, false);
                    AddDragSelection(cursorRow, m_cursorSel.startCol, cursorCol - 1); /* */
                } else {
                    /* left */
                    AddSelection(cursorRow, cursorCol, m_cursorSel.startCol - 1, true, false, false);
                    AddDragSelection(cursorRow, cursorCol, m_cursorSel.startCol - 1);
                }
            }
        } /* no movement */
    } else {
        /* shift not pressed */
        CSelection *selection_p;

        if (isRowSelected(cursorRow, &selection_p)) {
            if (left) {
                selection_p = m_selectionList.first();  /* position where the first selection start is */
                cursorCol = selection_p->startCol;
                cursorRow = selection_p->row;
            } else {
                selection_p = m_selectionList.last();  /* position where the first selection start is */
                cursorCol = selection_p->endCol + 1;
                cursorRow = selection_p->row;
            }
        }

        AddDragSelection(cursorRow, cursorCol, cursorCol);

        EmptySelectionList();
    }

    m_cursorDesiredCol = cursorCol;

    UpdateCursor(false, cursorRow, cursorCol);

    if (m_selectionList.isEmpty() || (m_selectionList.count() > 1)) {
        SelectionUpdated(nullptr);
    } else {
        SelectionUpdated(m_selectionList.first());
    }

    m_EraseBkGrndDisabled = true;
    update();
}

/***********************************************************************************************************************
*   PageUpDown
***********************************************************************************************************************/
void CEditorWidget::PageUpDown(bool up, int lines)
{
    /* select next top line */
    CLogScrutinizerDoc *doc_p = GetDocument();
    CSelection currentCur = m_cursorSel;

    if (lines == 0) {
        lines = m_maxDisplayRows;
    }

    /* m_totalNumOfRows is adjusted based on presentation mode (UpdateTotalNumRows(..)) */
    if ((m_totalNumOfRows <= 0) || (m_totalNumOfRows < m_maxDisplayRows)) {
        return;
    }

    ForceVScrollBitmap();

    int new_cursor_row = 0;
    int new_cursor_col = 0;

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        int remains;
        SearchFilteredRows_TIA(m_topLine, lines, up, &m_topLine, &remains);

        int packedFIR_Index = doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index;

        if (m_maxFIRAIndex - packedFIR_Index < m_maxDisplayRows) {
            packedFIR_Index = m_maxFIRAIndex - m_maxDisplayRows;
        }
        packedFIR_Index = packedFIR_Index < 0 ? 0 : packedFIR_Index;

        /* Move the cursor */
        new_cursor_row = m_topLine; /* fallback */
        SearchFilteredRows_TIA(m_cursorSel.row, lines, up, &new_cursor_row, &remains);
    } else {
        /* presentation mode */

        m_topLine += up ? lines * -1 : lines;

        new_cursor_row = up ? m_cursorSel.row - lines : m_cursorSel.row + lines;
        new_cursor_row = new_cursor_row > doc_p->m_database.TIA.rows ? doc_p->m_database.TIA.rows : new_cursor_row;
        new_cursor_row = new_cursor_row < 0 ? 0 : new_cursor_row;

        /* extra extra check */
        m_topLine = m_topLine < m_min_topLine ? m_min_topLine : m_topLine;
        m_topLine = m_topLine > m_max_topLine ? m_max_topLine : m_topLine;
    }

    int length;
    doc_p->GetTextItemLength(new_cursor_row, &length);
    new_cursor_col = m_cursorSel.startCol > length ? length - 1 : m_cursorSel.startCol;

    UpdateCursor(false, new_cursor_row, new_cursor_col);
    HorizontalCursorFocus(HCursorScrollAction_Focus_en);

    if (m_SHIFT_Pressed) {
        if (m_selectionList.isEmpty()) {
            m_origDragSelection = currentCur;
        }

        UpdateSelection(m_cursorSel.row, m_cursorSel.startCol);
    }

    UpdateRelPosition();
    LimitTopLine();
    m_EraseBkGrndDisabled = true;
    update();
}

/***********************************************************************************************************************
*   ModifyFontSize
***********************************************************************************************************************/
void CEditorWidget::ModifyFontSize(int increase)
{
    auto doc_p = GetTheDoc();
    auto size = doc_p->m_fontCtrl.GetSize();
    size += increase;
    if (increase > 0) {
        size = size > 80 ? 80 : size;
    } else if (increase < 0) {
        size = size < 3 ? 3 : size;
    } else {
        /* increase == 0 */
        size = g_cfg_p->m_default_FontSize;
    }
    doc_p->m_fontCtrl.ChangeFontSize(size);
    MW_Refresh();
}

/***********************************************************************************************************************
*   TogglePresentationMode
***********************************************************************************************************************/
void CEditorWidget::TogglePresentationMode(void)
{
    switch (m_presentationMode)
    {
        case PRESENTATION_MODE_ALL_e:
            SetPresentationMode(PRESENTATION_MODE_ONLY_FILTERED_e);
            break;

        case PRESENTATION_MODE_ONLY_FILTERED_e:
            SetPresentationMode(PRESENTATION_MODE_ALL_e);
            break;
    }
}

/***********************************************************************************************************************
*   TogglePresentationMode
***********************************************************************************************************************/
void CEditorWidget::SetPresentationMode(LogScrutinizerView_PresentaiontMode_t mode)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    if ((mode == PRESENTATION_MODE_ONLY_FILTERED_e) && (doc_p->m_database.FIRA.filterMatches == 0)) {
        TRACEX_I("No filter matches")
        return;
    }

    m_presentationMode = mode;
    UpdateTotalNumRows();

    SearchNewTopLine(false, SuggestedFocusRow());
    LimitTopLine();
    m_EraseBkGrndDisabled = true;
    ForceVScrollBitmap();
    Refresh();
}

/***********************************************************************************************************************
*   AddFilterItem
***********************************************************************************************************************/
bool CEditorWidget::AddFilterItem(void)
{
    if (m_selectionList.isEmpty()) {
        strcpy(m_tempStr, "");
    } else {
        CLogScrutinizerDoc *doc_p = GetDocument();
        CSelection *selection_p = m_selectionList.first();
        char *start_p;
        int textSize;
        int TIA_Row = selection_p->row;
        doc_p->GetTextItem(TIA_Row, &start_p, &textSize);

        if ((selection_p->endCol - selection_p->startCol) > 0) {
            textSize = selection_p->endCol - selection_p->startCol + 1;
            memcpy(m_tempStr, &start_p[selection_p->startCol], static_cast<size_t>(textSize));
        } else {
            memcpy(m_tempStr, start_p, static_cast<size_t>(textSize));
        }

        m_tempStr[textSize] = 0;
    }

    auto filterItem = CWorkspace_AddFilterItem(m_tempStr, nullptr, nullptr, this);
    if (filterItem != nullptr) {
        CWorkspace_TreeView_Select(filterItem);
    }
    return true;
}

/***********************************************************************************************************************
*   isRowReselected
***********************************************************************************************************************/
bool CEditorWidget::isRowReselected(int row, int startCol, CSelection **selection_pp)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    if (m_selectionList.isEmpty() || (m_totalNumOfRows == 0)) {
        return false;
    }

    for (auto selection_p : m_selectionList) {
        if (selection_p->row >= doc_p->m_database.TIA.rows) {
            TRACEX_E("CEditorWidget::isRowSelected  bad seletion in listL")
            return false;
        }
        if ((row == selection_p->row) &&
            (((startCol >= selection_p->startCol) && (startCol <= selection_p->endCol)) ||
             ((selection_p->startCol == -1) && (selection_p->endCol == -1)))) {
            if (selection_pp != nullptr) {
                *selection_pp = selection_p;
            } else {
                TRACEX_E("CEditorWidget::isRowSelected  called with nullptr")
                return false;
            }

            return true;
        }
    }

    return false;
}

/***********************************************************************************************************************
*   isRowSelected
***********************************************************************************************************************/
bool CEditorWidget::isRowSelected(int row, CSelection **selection_pp)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    *selection_pp = nullptr;

    if (m_selectionList.isEmpty() || (m_totalNumOfRows == 0)) {
        return false;
    }
    for (auto selection_p : m_selectionList) {
        if (selection_p->row >= doc_p->m_database.TIA.rows) {
            TRACEX_E("CEditorWidget::isRowSelected  bad seletion in listL")
            return false;
        }

        if (row == selection_p->row) {
            if (selection_pp != nullptr) {
                *selection_pp = selection_p;
            } else {
                TRACEX_E("CEditorWidget::isRowSelected  called with nullptr")
                return false;
            }
            return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   EmptySelectionList
***********************************************************************************************************************/
void CEditorWidget::EmptySelectionList(bool invalidate)
{
    auto doupdate = makeMyScopeGuard([&] () {
        if (invalidate) {
            update();
        }
    });

    PRINT_SELECTION("Empty selection list, count:%d", m_selectionList.count())

    for (auto selection : m_selectionList) {
        delete (selection);
    }

    m_selectionList.clear();
    m_lastSelectionValid = false;
    m_multiSelectionActive = false;
    SelectionUpdated();
}

/***********************************************************************************************************************
*   EmptySelectionListAbove
***********************************************************************************************************************/
void CEditorWidget::EmptySelectionListAbove(CSelection *selection_p)
{
    int count = 0;

    if (m_selectionList.isEmpty() || (m_selectionList.count() == 1)) {
        return;
    }

    while (m_selectionList.first() != selection_p) {
        if (m_selectionList.first()->row == m_lastSelection.row) {
            m_lastSelectionValid = false;
        }
        ++count;
        delete (m_selectionList.takeFirst());
    }

    SelectionUpdated();
    PRINT_SELECTION("EmptySelectionListAbove, count:%d", count)
}

/***********************************************************************************************************************
*   EmptySelectionListBelow
***********************************************************************************************************************/
void CEditorWidget::EmptySelectionListBelow(CSelection *selection_p)
{
    int count = 0;

    if (m_selectionList.isEmpty() || (m_selectionList.count() == 1)) {
        return;
    }

    while (m_selectionList.last() != selection_p) {
        if (m_selectionList.last()->row == m_lastSelection.row) {
            m_lastSelectionValid = false;
        }

        ++count;

        auto item = m_selectionList.takeLast();
        if (nullptr != item) {
            delete (item);
        }
    }

    SelectionUpdated();
    PRINT_SELECTION("EmptySelectionListBelow, count:%d", count)
}

/***********************************************************************************************************************
*   RemoveSelection
***********************************************************************************************************************/
void CEditorWidget::RemoveSelection(int row)
{
    if (m_selectionList.isEmpty()) {
        return;
    }

    PRINT_SELECTION("RemoveSelection, Row:%d", row)
    m_multiSelectionActive = false;

    for (auto selection_p : m_selectionList) {
        if (row == selection_p->row) {
            m_selectionList.removeOne(selection_p);
            delete selection_p;
            if (m_lastSelectionValid && (row == m_lastSelection.row)) {
                /* last selection was removed again, now there is none */
                m_lastSelectionValid = false;
            }
            SelectionUpdated();
            return;
        }
    }
    TRACEX_E("Selection not found, Row:%d", row)
}

/***********************************************************************************************************************
*   AddSelection
***********************************************************************************************************************/
void CEditorWidget::AddSelection(
    int TIA_Row,
    int startCol,
    int endCol,
    bool updateLastSelection,
    bool updateMultiSelection,
    bool updateCursor,
    bool cursorAfterSelection)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    CSelection *selection_p;
    int TIA_RowLength;
    CSelection *selectionIter_p;

    if (m_totalNumOfRows == 0) {
        return;
    }

    if (TIA_Row >= doc_p->m_database.TIA.rows) {
        /*(int) rows will never be neg */
        TRACEX_W("AddSelection failed, Row outside database, Row:%d SCol:%d ECol:%d", TIA_Row, startCol, endCol)
        return;
    }

    if ((startCol < -3) || (endCol < -3)) {
        TRACEX_W("AddSelection failed, Bad column values, Row:%d SCol:%d ECol:%d", TIA_Row, startCol, endCol)
        return;
    }

    doc_p->m_rowCache_p->GetTextItemLength(TIA_Row, &TIA_RowLength);

    if ((startCol == -1) && (endCol == -1)) {
        startCol = 0;
        endCol = TIA_RowLength - 1;
    }

    endCol = endCol < 0 ? 0 : endCol;   /* For rows of length 0 */

    if ((startCol != endCol) && (startCol > TIA_RowLength)) {
        TRACEX_W("AddSelection failed, start is in-front of total row size, Row:%d SCol:%d ECol:%d",
                 TIA_Row,
                 startCol,
                 endCol)
        return;
    }

    selection_p = new CSelection;

    if (updateMultiSelection) {
        m_multiSelectionActive = false;  /* single selection, multi selection disabled */
    }

    if ((TIA_RowLength != 0) && (endCol > TIA_RowLength - 1)) {
        if (startCol == endCol) {
            startCol = TIA_RowLength - 1;
        }
        endCol = TIA_RowLength - 1;
    }

    if (endCol < startCol) {
        int swap = startCol;
        startCol = endCol;
        endCol = swap;
    }

    if (updateLastSelection) {
        m_lastSelectionValid = true;
        m_lastSelection.row = TIA_Row;
        m_lastSelection.startCol = startCol;
        m_lastSelection.endCol = endCol;
    }

    selection_p->row = TIA_Row;
    selection_p->startCol = startCol;
    selection_p->endCol = endCol;

    if (updateCursor) {
        int newCursorPos = cursorAfterSelection ? endCol + 1 : startCol;
        UpdateCursor(false, TIA_Row, newCursorPos);
        m_cursorDesiredCol = newCursorPos;
    }

    PRINT_SELECTION("AddSelection, Row:%d SCol:%d ECol:%d", selection_p->row, selection_p->startCol,
                    selection_p->endCol)

    if (m_selectionList.isEmpty()) {
        m_selectionList.append(selection_p);
        if (startCol != endCol) {
            CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_TextSel_e);
        }
        SelectionUpdated();
        return;
    }

    /* Check if it already exists, convert to an update in such case */

    for (auto currentSelection_p : m_selectionList) {
        if (currentSelection_p->row == TIA_Row) {
            currentSelection_p->startCol = selection_p->startCol;
            currentSelection_p->endCol = selection_p->endCol;
            delete selection_p;
            return;
        }
    }

    /* 1. Check if row line is before first
     * 2. Check if row line is after last
     * 3. Determine if row line is closer to the first or last */

    CSelection *firstSelection_p = m_selectionList.first();
    CSelection *lastSelection_p = m_selectionList.last();

    if (firstSelection_p->row > TIA_Row) {
        m_selectionList.insert(0, selection_p); /* add to front */
        if (startCol != endCol) {
            CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_TextSel_e);
        }
        SelectionUpdated();
        return;
    } else if (lastSelection_p->row < TIA_Row) {
        m_selectionList.append(selection_p);
        if (startCol != endCol) {
            CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_TextSel_e);
        }
        SelectionUpdated();
        return;
    }

    /* Forward search
     * Add sorted */
    QList<CSelection *>::iterator iter = m_selectionList.begin();
    for ( ; iter < m_selectionList.end(); ++iter) {
        selectionIter_p = *iter;
        if (selection_p->row < selectionIter_p->row) {
            m_selectionList.insert(iter, selection_p); /* If the selection row is less than the item row just found,
                                                        * insert before that item */
            if (startCol != endCol) {
                CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_TextSel_e);
            }
            SelectionUpdated();
            return;
        }
    }
    m_selectionList.append(selection_p); /* error? */
    SelectionUpdated();
}

/***********************************************************************************************************************
*   UpdateSelection
***********************************************************************************************************************/
void CEditorWidget::UpdateSelection(int TIA_Row, int startCol)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    bool skipFirstRow = false;
    bool skipLastRow = false;

    PRINT_SELECTION("UpdateSelection Row:%-6d Col:%-4d", TIA_Row, startCol)

    if (m_totalNumOfRows == 0) {
        return;
    }

    if ((m_origDragSelection.row == TIA_Row) && (m_origDragSelection.startCol == startCol)) {
        PRINT_SELECTION("UpdateSelection same ROW and COL - No action")
        return;
    }

    EmptySelectionList();

    /* Add all back */

    CSelection firstRow;
    CSelection lastRow;

    if (m_origDragSelection.row == TIA_Row) {
        /* Only one row selection */

        if (startCol != m_origDragSelection.startCol) {
            /* AddSelection(TIA_Row, m_origDragSelection.startCol, startCol, true, false, false);
             * m_cursorDesiredCol = startCol; */
            if (startCol > m_origDragSelection.startCol) {
                AddSelection(TIA_Row, m_origDragSelection.startCol, startCol, true, false, false);
                UpdateCursor(false, TIA_Row, startCol + 1);
                m_cursorDesiredCol = startCol + 1;
            } else {
                /* isMouseCursorMovingRight */
                if (startCol < m_origDragSelection.startCol) {
                    if (isMouseCursorMovingRight() && ((m_origDragSelection.startCol - startCol) == 1)) {
                        /* Special case, just started to move to the right however it seems like the current
                        *  col is before the drag selection.
                        *  To start with, when doing a mouse move selection (drag selection) to the right we do not
                        * want
                        *  the selection to go beyond the current mouse position. As such, in that case the selection
                        *  should end in the current column. However, we want to align the start selection
                        *  (m_origDragSelection) to the closest lettert, this may then mean that the the cursor ends up
                        *  slightly infront of where the mouse is, and that is ok for initial selection. However, when
                        * we
                        *  then starts a drag selection the mouse will not have reached the position where the cursor
                        * is.
                        *  To prevent that a selection is created before the cursor the following if-statement is put
                        * in
                        *  place, and this early return will effectivly prevent any selection being made. */
                        SelectionUpdated();
                        return;
                    }
                }
                AddSelection(TIA_Row, startCol, m_origDragSelection.startCol - 1, true, false, false);
                UpdateCursor(false, TIA_Row, startCol);
                m_cursorDesiredCol = startCol;
            }
            SelectionUpdated();
        }
        return;
    } else if (m_origDragSelection.row < TIA_Row) {
        /* Selecting down
         * 1. First row is added as m_origDragSelection.startCol until end of string
         * 2. Last row is added as 0 until cursor/startCol
         * 3. Lines in between is full line selections */
        int length;

        doc_p->m_rowCache_p->GetTextItemLength(m_origDragSelection.row, &length);

        /* First row */

        firstRow.row = m_origDragSelection.row;
        firstRow.startCol = m_origDragSelection.startCol;
        firstRow.endCol = m_CTRL_Pressed ? startCol : length - 1;

        if (firstRow.startCol >= firstRow.endCol) {
            skipFirstRow = true;
        }

        lastRow.row = TIA_Row;
        lastRow.startCol = m_CTRL_Pressed ? m_origDragSelection.startCol : 0;
        lastRow.endCol = startCol - 1;

        UpdateCursor(false, TIA_Row, startCol);
        m_cursorDesiredCol = startCol;
    } else {
        /* Selecting up  (however adding selections in increasing row order) */

        /* 1. First row is added as startCol to end
         * 2. Last row is added as 0 to m_origDragSelection.startCol
         * 3. Lines in between is full line selections */

        int length;

        /* First row */

        firstRow.row = TIA_Row;

        doc_p->m_rowCache_p->GetTextItemLength(firstRow.row, &length);

        firstRow.endCol = length - 1 > 0 ? length - 1 : 0;
        firstRow.startCol = startCol > firstRow.endCol ? firstRow.endCol : startCol;

        lastRow.row = m_origDragSelection.row;
        lastRow.startCol = 0;
        lastRow.endCol = m_origDragSelection.startCol - 1 > 0 ? m_origDragSelection.startCol - 1 : 0;

        UpdateCursor(false, TIA_Row, startCol);
        m_cursorDesiredCol = startCol;
    }

    if (!skipFirstRow) {
        AddSelection(firstRow.row, firstRow.startCol, firstRow.endCol, true, false, false);
    }

    if (!skipLastRow) {
        AddSelection(lastRow.row, lastRow.startCol, lastRow.endCol, true, false, false);
    }

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        int startIndex = doc_p->m_database.FIRA.FIR_Array_p[firstRow.row].index + 1;  /* start at filtered row
                                                                                       * after this one */
        int endIndex = doc_p->m_database.FIRA.FIR_Array_p[lastRow.row].index;       /* stops at row before this one
                                                                                     * */

        for (int index = startIndex; index < endIndex; ++index) {
            if (m_CTRL_Pressed) {
                AddSelection(doc_p->m_database.packedFIRA_p[index].row, m_origDragSelection.startCol, startCol,
                             true, false, false);
            } else {
                AddSelection(doc_p->m_database.packedFIRA_p[index].row, -1, -1, true, false, false);
            }
        }
    } else {
        int startIndex = firstRow.row + 1;  /* start at filtered row after this one */
        int endIndex = lastRow.row;        /* stops at row before this one */

        for (int index = startIndex; index < endIndex; ++index) {
            if (m_CTRL_Pressed) {
                AddSelection(index, m_origDragSelection.startCol, startCol, true, false, false);
            } else {
                AddSelection(index, -1, -1, true, false, false);
            }
        }
    }

    SelectionUpdated();
}

/***********************************************************************************************************************
*   ExpandSelection
***********************************************************************************************************************/
void CEditorWidget::ExpandSelection(CSelection *selection_p)
{
    CLogScrutinizerDoc *doc_p = GetDocument();
    char *text_p = nullptr;
    int size = 0;
    bool stop = false;
    bool expandExisting = (selection_p->endCol - selection_p->startCol) > 0 ? true : false;

    typedef enum {
        ExpandKind_Letter,
        ExpandKind_Digit,
        ExpandKind_Other
    }ExpandKind_t;

    ExpandKind_t expandKind;

    PRINT_SELECTION("Expand selection");

    /* Check selection */
    if (selection_p->row >= doc_p->m_database.TIA.rows) {
        TRACEX_E("CEditorWidget::ExpandSelection  Seletion row:%d out of range", selection_p->row)
        return;
    }

    doc_p->GetTextItem(selection_p->row, &text_p, &size);

    if (size == 0) {
        /* Expand on empty row */
        return;
    }

    /* Check selection */
    if (selection_p->endCol >= size) {
        return;
    }

    /* Expansion rules (By Notepad++)
     *
     * If expansion starts in alpha or digit, then all alfa and digits are included in vicinity.. including _
     * non alpha or digit, only these are included, excluding space */
    if (text_p != nullptr) {
        int index = selection_p->startCol;

        if (isdigit((uint8_t)text_p[selection_p->startCol])) {
            expandKind = ExpandKind_Digit;
        } else if (isalpha((uint8_t)text_p[selection_p->startCol])) {
            expandKind = ExpandKind_Letter;
        } else if (text_p[selection_p->startCol] == '_') {
            expandKind = ExpandKind_Digit;
        } else {
            expandKind = ExpandKind_Other;
        }

        while (!stop) {
            /* exand to the left */
            if (index <= 0) {
                index = 0;
                stop = true;
            } else if ((text_p[index] == ' ') ||
                       ((expandKind == ExpandKind_Digit) &&
                        (!isdigit((uint8_t)text_p[index]) && !isalpha((uint8_t)text_p[index]) &&
                         (text_p[index] != '_'))) ||
                       ((expandKind == ExpandKind_Letter) &&
                        (!isdigit((uint8_t)text_p[index]) && !isalpha((uint8_t)text_p[index]) &&
                         (text_p[index] != '_'))) ||
                       ((expandKind == ExpandKind_Other) &&
                        (isalpha((uint8_t)text_p[index]) || isdigit((uint8_t)text_p[index])))) {
                stop = true;
                ++index;
            } else {
                --index;
            }
        }

        if (index < selection_p->startCol) {
            selection_p->startCol = index;
        }

        index = selection_p->endCol;

        if (expandExisting) {
            /* skip spaces until next other char{ */
            stop = false;

            /*expand to the right */
            while (!stop && index < size) {
                if ((text_p[index] == ' ') ||
                    ((expandKind == ExpandKind_Digit) &&
                     (!isdigit((uint8_t)text_p[index]) && !isalpha((uint8_t)text_p[index]) &&
                      (text_p[index] != '_'))) ||
                    ((expandKind == ExpandKind_Letter) &&
                     (!isdigit((uint8_t)text_p[index]) && !isalpha((uint8_t)text_p[index]) &&
                      (text_p[index] != '_'))) ||
                    ((expandKind == ExpandKind_Other) &&
                     (isalpha((uint8_t)text_p[index]) || isdigit((uint8_t)text_p[index])))) {
                    stop = true;
                } else {
                    ++index;
                }
            }
        }

        index++; /* make sure to expand up one step */

        while (index < size &&
               text_p[index] != ' ' &&
               (((expandKind == ExpandKind_Digit) &&
                 (isdigit((uint8_t)text_p[index]) || isalpha((uint8_t)text_p[index]) ||
                  text_p[index] == '_')) ||
                ((expandKind == ExpandKind_Letter) &&
                 (isdigit((uint8_t)text_p[index]) || isalpha((uint8_t)text_p[index]) ||
                  text_p[index] == '_')) ||
                ((expandKind == ExpandKind_Other) &&
                 (!isalpha((uint8_t)text_p[index]) && !isdigit((uint8_t)text_p[index]))))) {
            ++index;
        }

        --index;

        if (index >= size) {
            index = size - 1;
        }

        if (index > selection_p->endCol) {
            selection_p->endCol = index;
        }

        if (selection_p->endCol < selection_p->startCol) {
            /* Make sure start col is before end col */

            int tempCol = selection_p->startCol;
            selection_p->startCol = selection_p->endCol;
            selection_p->endCol = tempCol;
        }

        UpdateCursor(false, selection_p->row, selection_p->startCol);

        /* Check selection */
        if (selection_p->row >= doc_p->m_database.TIA.rows) {
            TRACEX_E("CEditorWidget::ExpandSelection  Seletion row:%d out of range", selection_p->row)
            return;
        }

        doc_p->GetTextItem(selection_p->row, &text_p, &size);

        /* Check selection */
        if (selection_p->endCol >= size) {
            TRACEX_E("CEditorWidget::ExpandSelection  Seletion end col:%d out of range", selection_p->endCol)
            return;
        }
    }

    AddDragSelection(selection_p->row, selection_p->startCol, selection_p->startCol);
    SelectionUpdated();
}

/***********************************************************************************************************************
*   GetActiveSelection
***********************************************************************************************************************/
bool CEditorWidget::GetActiveSelection(CSelection *selection_p, QString& text)
{
    if (m_selectionList.isEmpty()) {
        text = QString("");
        return false;
    }

    if (m_lastSelectionValid) {
        selection_p->row = m_lastSelection.row;
        selection_p->startCol = m_lastSelection.startCol;
        selection_p->endCol = m_lastSelection.endCol;
    } else {
        *selection_p = *(m_selectionList.last());
    }

    text = GetTheDoc()->GetTextItem(selection_p->row);
    text.remove(selection_p->endCol + 1, text.size());
    text.remove(0, selection_p->startCol);
    return true;
}

/***********************************************************************************************************************
*   GetActiveSelection
***********************************************************************************************************************/
bool CEditorWidget::GetActiveSelection(CSelection *selection_p)
{
    if (m_selectionList.isEmpty()) {
        return false;
    }

    if (m_lastSelectionValid) {
        selection_p->row = m_lastSelection.row;
        selection_p->startCol = m_lastSelection.startCol;
        selection_p->endCol = m_lastSelection.endCol;
    } else {
        *selection_p = *(m_selectionList.last());
    }

    return true;
}

/***********************************************************************************************************************
*   GetCursorPosition
***********************************************************************************************************************/
bool CEditorWidget::GetCursorPosition(CSelection *selection_p)
{
    /* Always update the selection data */
    selection_p->row = m_cursorSel.row;
    selection_p->startCol = m_cursorSel.startCol;
    selection_p->endCol = m_cursorSel.startCol;
    return m_cursorActive ? true : false;
}

/***********************************************************************************************************************
*   AddMultipleSelections
***********************************************************************************************************************/
void CEditorWidget::AddMultipleSelections(const int startRow, const int endRow, bool usePresentationMode)
{
    auto doc_p = GetTheDoc();
    EmptySelectionList();
    setCursor(Qt::WaitCursor);
    try {
        for (int row = startRow; row < endRow; ++row) {
            int length;
            if (!usePresentationMode ||
                (m_presentationMode != PRESENTATION_MODE_ONLY_FILTERED_e) ||
                (doc_p->m_rowCache_p->GetFilterRef(row) != nullptr)) {
                doc_p->m_rowCache_p->GetTextItemLength(row, &length);

                auto selection_p = new CSelection(row, 0, length);
                m_selectionList.append(selection_p);
            }
        }
    } catch (...) {
        EmptySelectionList();
        TRACEX_W(QString("Selecting rows from %1 to %2 failed").arg(startRow).arg(endRow))
    }
    unsetCursor();
}

/***********************************************************************************************************************
*   ContinueSelection
*
* Quick analyze of windows style guide for multiselection
*
* x. Multiselection is extended if next selection is in the same direction as the previous multiselection
*          row1 - row3, +row6 -> row1-row6
*          row3 - row2, +row1 -> row1-row3
* x. If there exist a selection, and there is a new multi selection, then the old multiselection is removed
*    and the new multiselection starts from the same place as the old.
*
* x. If there is an existing selections (including multiselections), and there is added single selections
* (CTRL-LMOUSE),
*    then at additional multiselection (SHIFT-LMOUSE) all other selection are removed and a new multiselection is
*   started from the
*    latest single selection
*
***********************************************************************************************************************/
void CEditorWidget::ContinueSelection(int TIA_selectedRow, int startCol, int endCol)
{
    CSelection startPoint;

    TRACEX_D("ContinueSelection Row:%d SCol:%d ECol:%d", TIA_selectedRow, startCol, endCol)

    if (m_selectionList.isEmpty()) {
        if (m_cursorActive) {
            TRACEX_D("ContinueSelection from cursor")
            EmptySelectionList();

            startPoint = m_cursorSel;

            /* Setup a new multi selection */
            m_multiSelectionActive = true;
            m_multiSelection.row = m_cursorSel.row;
            m_multiSelection.startCol = m_cursorSel.startCol;
            m_multiSelection.endCol = m_cursorSel.endCol;
        } else {
            TRACEX_D("ContinueSelection Selection list empty, adding simple selection")
            AddSelection(TIA_selectedRow, startCol, endCol, true, true);
            return;
        }
    } else if (m_multiSelectionActive) {
        TRACEX_D("ContinueSelection Multi active, empty and refill")
        startPoint = m_multiSelection;
        EmptySelectionList();

        /* EmptySelectionList will clean out m_multiSelectionActive flag, reset it to true */
        m_multiSelectionActive = true;
    } else if (m_lastSelectionValid) {
        TRACEX_D("ContinueSelection Last valid, empty and refill")
        startPoint = m_lastSelection;
        EmptySelectionList();

        /* Setup a new multi selection */
        m_multiSelectionActive = true;
        m_multiSelection.row = m_lastSelection.row;
        m_multiSelection.startCol = m_lastSelection.startCol;
        m_multiSelection.endCol = m_lastSelection.endCol;
    } else {
        TRACEX_D("ContinueSelection No Multi or Last, empty and adding simple selection")
        EmptySelectionList();
        AddSelection(TIA_selectedRow, startCol, endCol, true, true);
        return;
    }

    int index;
    int endRow;

    if (startPoint.row > (int)TIA_selectedRow) {
        index = TIA_selectedRow;
        endRow = startPoint.row;
    } else {
        index = startPoint.row;
        endRow = TIA_selectedRow;
    }

    CLogScrutinizerDoc *doc_p = GetDocument();

    if (endRow - index > 1000) {
        g_processingCtrl_p->Processing_StartReport();
    }

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        /* step the amount of filtered lines */
        for ( ; index <= endRow; ++index) {
            if ((doc_p->m_rowCache_p->GetFilterRef(index) != nullptr) &&
                !doc_p->m_rowCache_p->GetFilterRef(index)->m_exclude) {
                AddSelection(index, m_multiSelection.startCol, m_multiSelection.endCol, false, false);
            }
        }
    } else {
        /* step the amount of filtered lines */
        for ( ; index <= endRow; ++index) {
            AddSelection(index, m_multiSelection.startCol, m_multiSelection.endCol, false, false);
        }
    }

    SelectionUpdated();

    g_processingCtrl_p->Processing_StopReport();
}

/***********************************************************************************************************************
*   GetSelectionText
* When a selection has been modified this function should be called. If this function is called with nullptr parameters
* that means that
* the list of selections has been updated
***********************************************************************************************************************/
void CEditorWidget::SelectionUpdated(CSelection *selection_p)
{
#ifdef _DEBUG
    if (selection_p == nullptr) {
        TRACEX_DE("Selection updated nullptr")
    } else {
        TRACEX_DE("Selection updated s:%d e:%d ", selection_p->startCol, selection_p->endCol)
    }
#endif

    if (m_autoHighLightTimer->isActive()) {
#ifdef _DEBUG
        TRACEX_D("CEditorWidget::SelectionUpdated  KillTimer - AUTO_HIGHLIGHT_TIMER_ID")
#endif
        m_autoHighLightTimer->stop();
    }

    if (m_selectionList.isEmpty() || (m_selectionList.count() > 1) ||
        ((m_selectionList.first()->endCol - (m_selectionList.first()->startCol)) < 2)) {
        CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
#ifdef _DEBUG
        TRACEX_D("SetAutoHighlight RESET  selections:%d", m_selectionList.count())
#endif
        doc_p->m_autoHighLighter_p->SetAutoHighlight(0, nullptr);
        return;
    } else {
        m_autoHighLightTimer->start(AUTO_HIGHLIGHT_TIMER_DURATION); /* will be a single shot */
#ifdef _DEBUG
        TRACEX_D("AUTO_HIGHLIGHT_TIMER_ID  Activated")
#endif
    }

    if (!hasFocus()) {
        setFocus();
    }
}

/***********************************************************************************************************************
*   GetSelectionText
***********************************************************************************************************************/
QString CEditorWidget::GetSelectionText(void)
{
    int textSize;
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    char *start_p;

    if (m_selectionList.isEmpty()) {
        return QString();
    }

    CSelection *selection_p = m_selectionList.first();
    if ((selection_p->endCol - selection_p->startCol) > 0) {
        doc_p->GetTextItem(selection_p->row, &start_p, &textSize);

        int textSize = selection_p->endCol - selection_p->startCol + 1;
        QString text(&start_p[selection_p->startCol]);
        return text.left(textSize);
    } else {
        return QString();
    }
}

/***********************************************************************************************************************
*   GetSelectionsTextSize
***********************************************************************************************************************/
int CEditorWidget::GetSelectionsTextSize(void)
{
    int textSize;
    int TIA_Row;
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    char *start_p;

    /* Find out the total size first */
    int totalSize = 0;

    for (auto selection_p : m_selectionList) {
        textSize = 0;
        TIA_Row = selection_p->row;
        doc_p->GetTextItem(TIA_Row, &start_p, &textSize);

        if ((selection_p->endCol - selection_p->startCol) > 0) {
            totalSize += selection_p->endCol - selection_p->startCol + 1;
        } else {
            totalSize += textSize;
        }
        totalSize += 2;   /* + 0x0d + 0x0a */
    }

    ++totalSize;      /* + 0x0 */
    totalSize += 100; /* to be really really sure... */

    return totalSize;
}

/***********************************************************************************************************************
*   GetSelectionsTextMaxSize
***********************************************************************************************************************/
int CEditorWidget::GetSelectionsTextMaxSize(void)
{
    int textSize;
    int TIA_Row;
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    char *start_p;
    int selectionMaxSize = 0;
    int selectionSize = 0;

    /* Find out the total size first */

    for (auto selection_p : m_selectionList) {
        textSize = 0;
        TIA_Row = selection_p->row;
        doc_p->GetTextItem(TIA_Row, &start_p, &textSize);
        if ((selection_p->endCol - selection_p->startCol) > 0) {
            selectionSize = selection_p->endCol - selection_p->startCol + 1;
        } else {
            selectionSize = textSize;
        }
        if (selectionSize > selectionMaxSize) {
            selectionMaxSize = selectionSize;
        }
    }
    selectionMaxSize += (3 + 20);   /* + 0x0d + 0x0a + 0x0   +20 (some extra margin) */
    return selectionMaxSize;
}

/***********************************************************************************************************************
*   OnAutoHighlightTimer
***********************************************************************************************************************/
void CEditorWidget::OnAutoHighlightTimer(void)
{
    auto doupdate = makeMyScopeGuard([&] () {
        update();
    });
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();

#ifdef _DEBUG
    TRACEX_D("CEditorWidget::OnAutoHighlightTimer  KillTimer - AUTO_HIGHLIGHT_TIMER_ID")
#endif

    if (m_selectionList.isEmpty() || (m_selectionList.count() > 1)) {
#ifdef _DEBUG
        TRACEX_D("OnAutoHighlightTimer RESET  selections:%d", m_selectionList.count())
#endif
        doc_p->m_autoHighLighter_p->SetAutoHighlight(0, nullptr);
        return;
    }

    CSelection *selection_p = m_selectionList.first();
    char *text_p;
    int textSize;
    const int selectionSize = selection_p->endCol - selection_p->startCol + 1;

    doc_p->GetTextItem(selection_p->row, &text_p, &textSize);

    if (selectionSize >= MAX_AUTOHIGHLIGHT_LENGTH - 2) {
#ifdef _DEBUG
        TRACEX_D("OnAutoHighlightTimer RESET  MAX_AUTOHIGHLIGHT_LENGTH:%d", selectionSize)
#endif
        doc_p->m_autoHighLighter_p->SetAutoHighlight(0, nullptr);
        return;
    }

    memcpy(m_tempStr, &text_p[selection_p->startCol], selectionSize);
    m_tempStr[selectionSize] = 0;

    doc_p->m_autoHighLighter_p->SetAutoHighlight(MW_GetTick(), m_tempStr);

    TRACEX_D("OnAutoHighlightTimer OK %s", m_tempStr)
}

/***********************************************************************************************************************
*   textWindow_SelectionUpdate
***********************************************************************************************************************/
bool CEditorWidget::textWindow_SelectionUpdate(ScreenPoint_t *screenPoint_p, LS_LG_EVENT_t LS_LG_Event,
                                               Qt::KeyboardModifiers modifiers)
{
    int screenRow;
    int screenCol;
    bool reselection = false;
    int TIA_row;
    CSelection *reselection_p;
    bool overHalf;

    if (LS_LG_Event & LS_LG_EVENT_MOUSE_e) {
        if ((screenPoint_p == nullptr) || !(PointToCursor(screenPoint_p, &screenRow, &screenCol, &overHalf))) {
            if (screenPoint_p == nullptr) {
                TRACEX_E("Selection update, point_p = nullptr")
            }
            return true;
        }

        TIA_row = CursorTo_TIA_Index(screenRow);

        /* Be careful, reselection_p point to a selection in the selectionList, if the selection list is cleared then
         * this reference will not be valid anymore */
        reselection = isRowReselected(TIA_row, screenCol, &reselection_p);

        PRINT_SELECTION(QString("textWindow_SelectionUpdate MOUSE %1,%2 Row:%3, Col:%4")
                            .arg(screenPoint_p->mouse.rx()).arg(screenPoint_p->mouse.ry()).arg(screenRow).arg(screenCol));

        if (LS_LG_Event == LS_LG_EVENT_LMOUSE_DOWN_e) {
            /* Mouse based event */
            if (modifiers & Qt::ControlModifier) {
                PRINT_SELECTION("Selection update, LS_LG_EVENT_LMOUSE_DOWN_e + MK_LBUTTON, reselection:%d",
                                reselection);

                if (reselection) {
                    RemoveSelection(TIA_row);
                } else {
                    AddSelection(TIA_row, -1, -1, true, true);
                    AddDragSelection(TIA_row, 0, 0);
                }
                DisableCursor();
                return true;
            } else if (modifiers & Qt::ShiftModifier) {
                m_pendingSingleSelection = false;

                PRINT_SELECTION("Selection update, LS_LG_EVENT_LMOUSE_DOWN_e, MULTIPLE  Reselection:%d", reselection);

                /*ContinueSelection(TIA_row, screenCol, screenCol); */

                if (m_selectionList.isEmpty()) {
                    AddDragSelection(m_cursorSel.row, m_cursorSel.startCol, m_cursorSel.endCol);
                    UpdateSelection(TIA_row, screenCol);
                } else {
                    UpdateSelection(TIA_row, screenCol);
                }
            } else {
                PRINT_SELECTION("Selection update, LS_LG_EVENT_LMOUSE_DOWN_e, reselection:%d", reselection);

                if (reselection) {
                    PRINT_SELECTION("Selection update, DRAG ENABLED");
                    m_dragEnabled = true;
                    m_dragOngoing = false;
                    m_pendingSingleSelection = true;     /* If there is no drag then we should empty selection list and
                                                          * select the last one */
                    m_dragSelectionOngoing = false;
                    m_singleSelection.row = TIA_row;
                    m_singleSelection.startCol = screenCol;
                    m_singleSelection.endCol = screenCol;
                } else {
                    EmptySelectionList();
                    UpdateCursor(false, TIA_row, screenCol);

                    m_cursorDesiredCol = screenCol;
                    m_dragSelectionOngoing = true; /* User is moving mouse with LeftB down making multiple selections */

                    AddDragSelection(TIA_row, m_cursorSel.startCol, m_cursorSel.startCol);
                }
            }
        } else if (LS_LG_Event == LS_LG_EVENT_MOUSE_MOVE_e) {
            if (m_dragEnabled) {
                if (!m_dragOngoing) {
                    m_dragOngoing = true;
                    m_pendingSingleSelection = false;
#ifdef QT_TODO
                    DragAndDrop();
#endif
                    PRINT_SELECTION("Selection update, LS_LG_EVENT_MOUSE_MOVE_e  DragOnGoing x:%-4d y:%-4d",
                                    screenPoint_p->mouse.x(), screenPoint_p->mouse.y());
                }
                return false;
            }

            if (m_LMousePressed) {
                /* Column selection */
                PRINT_SELECTION(QString("Selection update, column selection col:%1").arg(screenCol));
                UpdateSelection(TIA_row, screenCol - (overHalf ? 1 : 0));
                m_pendingSingleSelection = false;
            }
        } else if (LS_LG_Event == LS_LG_EVENT_LMOUSE_UP_e) {
            PRINT_SELECTION("Selection update, LS_LG_EVENT_LMOUSE_UP_e");

            /* E.g. the user has select many rows, then a already selected row is selected again, hence all but the
             * re-selected row should be selected. This
             * is the purpose of the m_pendingSingleSelection */
            if (m_pendingSingleSelection) {
                /* Necessary to know if a line (without shift/ctrl) was selected before, since */
                m_pendingSingleSelection = false;
                EmptySelectionList();

                /* just set cursor */
                UpdateCursor(false, m_singleSelection.row, m_singleSelection.startCol);
                AddDragSelection(m_singleSelection.row, m_singleSelection.startCol, m_singleSelection.startCol);
            } else {
                m_dragSelectionOngoing = false;
            }

            m_LMousePressed = false;
            m_dragEnabled = false;
            m_dragOngoing = false;
            m_dragSelectionOngoing = false;
        } else if (LS_LG_Event == LS_LG_EVENT_LMOUSE_DBL_e) {
            CSelection newSelection;
            bool expandExisting = false;

            PRINT_SELECTION("Selection update, LS_LG_EVENT_LMOUSE_DBL_e");

            m_LMousePressed = false;
            m_dragEnabled = false;
            m_dragOngoing = false;
            m_dragSelectionOngoing = false;
            m_pendingSingleSelection = false;

            SetVScrollGlue(false);

            if (reselection && ((reselection_p->endCol - reselection_p->startCol) != 0)) {
                expandExisting = true;
            }

            if (expandExisting) {
                newSelection.row = reselection_p->row;
                newSelection.startCol = reselection_p->startCol;
                newSelection.endCol = reselection_p->endCol;
            } else {
                newSelection.row = TIA_row;
                newSelection.startCol = screenCol;
                newSelection.endCol = screenCol;
            }

            if (newSelection.startCol > newSelection.endCol) {
                int tempCol = newSelection.startCol;
                newSelection.startCol = newSelection.endCol;
                newSelection.endCol = tempCol;
            }

            reselection_p = nullptr; /* When we empty selections then reselection_p will no longer point to anything
                                      * valid */
            EmptySelectionList();

            ExpandSelection(&newSelection);
            AddSelection(newSelection.row, newSelection.startCol, newSelection.endCol, true, true);
        }

        return true;
    } else if (LS_LG_Event & LS_LG_EVENT_KEY_e) {
        if (LS_LG_Event == LS_LG_EVENT_KEY_CTRL_C_e) {
            PRINT_SELECTION("Selection update, LS_LG_EVENT_KEY_CTRL_C_e");

            SelectionsToClipboard();
            return false; /* no redraw necessary */
        } else if (LS_LG_Event == LS_LG_EVENT_KEY_CTRL_A_e) {
            AddMultipleSelections(0, GetTheDoc()->m_database.TIA.rows);
            return true;
        }
    } else {
        TRACEX_E("Unknown LS_LG_Event: %d", LS_LG_Event)
    }

    return true;
}

/***********************************************************************************************************************
*   AddDragSelection
***********************************************************************************************************************/
void CEditorWidget::AddDragSelection(int TIA_Row, int startCol, int endCol)
{
    m_origDragSelection.row = TIA_Row;
    m_origDragSelection.startCol = startCol;
    m_origDragSelection.endCol = endCol;

    PRINT_SELECTION(QString("Add drag selection, row:%1 start:%2 end:%3").arg(TIA_Row).arg(startCol).arg(endCol));
}

/* Use QByteArray instead of CShareFile */
bool CEditorWidget::CopySelectionsToSharedFile(QString sharedFile /*CSharedFile* sf_p*/, int *bytesWritten_p)
{
    Q_UNUSED(sharedFile)
    Q_UNUSED(bytesWritten_p)

#ifdef QT_TODO
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();

    CSelection *selection_p = nullptr;
    POSITION pos;
    char *start_p;
    int textSize;
    bool dataCopied = false;
    int TIA_Row;
    char *tempStr_p;

    *bytesWritten_p = 0;

    if (m_selectionList.isEmpty()) {
        TRACEX_D("CEditorWidget::CopySelectionsToSharedFile  Empty selection list")
        return false;
    }

    int totalSize = GetSelectionsTextMaxSize();

    tempStr_p = (char *)malloc(totalSize);

    if (tempStr_p == nullptr) {
        TRACEX_D("CEditorWidget::CopySelectionsToSharedFile  tempStr_p is nullptr")
        return false;
    }

    pos = m_selectionList.GetHeadPosition();

    while (pos != nullptr) {
        selection_p = m_selectionList.GetNext(pos);

        TIA_Row = selection_p->row;
        doc_p->GetTextItem(TIA_Row, &start_p, &textSize);

        if ((selection_p->endCol - selection_p->startCol) > 0) {
            textSize = selection_p->endCol - selection_p->startCol + 1;
            memcpy(tempStr_p, &start_p[selection_p->startCol], textSize);
        } else {
            memcpy(tempStr_p, start_p, textSize);
        }

        if (pos != nullptr) {
            /* Only add newlines if it is NOT the last row */
            tempStr_p[textSize++] = 0x0d;
            tempStr_p[textSize++] = 0x0a;
        }

        *bytesWritten_p += textSize;

        dataCopied = true;
        sf_p->Write(tempStr_p, textSize); /* You can write to the clipboard as you would to any CFile */

        /*    if (TRACEX_IS_ENABLED(LOG_LEVEL_DEBUG_EXT))
         *    {
         *      tempStr_p[textSize++] = 0;
         *      TRACEX_DE("Text in Drag and Drop: %d %s", textSize, tempStr_p)
         *    } */
    }

    free(tempStr_p);

    return dataCopied;
#endif
    Q_ASSERT(false);
    return 0;
}

/***********************************************************************************************************************
*   CopySelectionsToMem
***********************************************************************************************************************/
bool CEditorWidget::CopySelectionsToMem(char *dest_p, int *bytesWritten_p)
{
    Q_UNUSED(dest_p)
    Q_UNUSED(bytesWritten_p)

#ifdef QT_TODO
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();

    CSelection *selection_p = nullptr;
    POSITION pos;
    char *start_p;
    int textSize;
    bool dataCopied = false;
    int TIA_Row;
    int maxPrint = 10;
    int destIndex = 0; /* *bytesWritten_p */

    *bytesWritten_p = 0;

    if (m_selectionList.isEmpty()) {
        TRACEX_D("CEditorWidget::CopySelectionsToSharedFile  Empty selection list")
        return false;
    }

    pos = m_selectionList.GetHeadPosition();
    destIndex = 0;

    while (pos != nullptr) {
        char *copiedText_p = &dest_p[destIndex];

        selection_p = m_selectionList.GetNext(pos);

        TIA_Row = selection_p->row;
        doc_p->GetTextItem(TIA_Row, &start_p, &textSize);

        if ((selection_p->endCol - selection_p->startCol) > 0) {
            textSize = selection_p->endCol - selection_p->startCol + 1;
            memcpy(&dest_p[destIndex], &start_p[selection_p->startCol], textSize);
        } else {
            memcpy(&dest_p[destIndex], start_p, textSize);
        }

        destIndex += textSize;

        if (pos != nullptr) {
            /* Only add newlines if it is NOT the last row */
            dest_p[destIndex++] = 0x0d;
            dest_p[destIndex++] = 0x0a;
        }

        dataCopied = true;

        if (TRACEX_IS_ENABLED(LOG_LEVEL_DEBUG) && (maxPrint > 0)) {
            dest_p[destIndex] = 0;
            TRACEX_D("Text in Drag and Drop: %d %s", textSize, copiedText_p)
            maxPrint--;
        }
    }

    return dataCopied;
#endif
    Q_ASSERT(false);
    return 0;
}

/***********************************************************************************************************************
*   SaveSelectionsToFile
***********************************************************************************************************************/
bool CEditorWidget::SaveSelectionsToFile(QString& fileName)
{
    auto doc_p = GetTheDoc();

    if (m_selectionList.isEmpty()) {
        TRACEX_I("SaveSelectionsToFile  Empty selection list")
        return false;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        TRACEX_E(QString("Save selections to file failed, cannot open file:%1").arg(fileName))
        return false;
    }

    QTextStream fileStream(&file);

    for (auto& selection_p : m_selectionList) {
        QString text = doc_p->GetTextItem(selection_p->row);
        if (selection_p->endCol != 0) {
            text.truncate(selection_p->endCol + 1);
        }
        if (selection_p->startCol != 0) {
            text.remove(0, selection_p->startCol);
        }
        fileStream << text << endl;
    }

    fileStream.flush();
    file.close();
    return true;
}

/***********************************************************************************************************************
*   SelectionsToClipboard
*
* Copy the text selection decorated with HTML to keep the formatting (e.g. color) when later pasting it. The text
* copied needs to included e.g. meta tag.
*
***********************************************************************************************************************/
void CEditorWidget::SelectionsToClipboard(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();

#ifdef _DEBUG
    const QMimeData *mimeDataCopy = clipboard->mimeData();

    if (mimeDataCopy->hasImage()) {
        TRACEX_I("imageData")
    } else if (mimeDataCopy->hasHtml()) {
        TRACEX_I(QString("htmlData %1").arg(mimeDataCopy->html()))
    } else if (mimeDataCopy->hasText()) {
        TRACEX_I(QString("textData %1").arg(mimeDataCopy->text()))
    } else {
        TRACEX_I(tr("Cannot display data"))
    }
#endif

    clipboard->clear();

    QString plain_text;
    QString html_text;
    QMimeData *mimeData = new QMimeData;
    QString fontSize = QString("%1px").arg(GetTheDoc()->m_fontCtrl.GetSize() + 4);

    /* Important, the content that needs to be copied differs between Win32 and Linux */
#if _WIN32
    QString head("<html><body>");
    QString tail("</html></body>");
#else
    QString head("");
    QString tail("");
#endif

    QString meta("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">");

    /*    rich_text->setHtml(str); */
    int selectionCount = (int)m_selectionList.count();
    if (selectionCount == 0) {
        TRACEX_W("No selection to copy")
        return;
    }

    html_text = head + meta;

    setCursor(Qt::WaitCursor);

    bool first = true;
    QString style;
    html_text += QString("<p>");
    try {
        for (auto& selection_p : m_selectionList) {
            auto text = doc_p->GetTextItem(selection_p->row);
            auto filterItem_p = doc_p->m_rowCache_p->GetFilterRef(selection_p->row);

            /* #define BACKGROUND_COLOR                    Q_RGB(255, 250, 240)          //FLORALWHITE */
            if (filterItem_p == nullptr) {
                style = QString("style=\"color: rgb(192, 192, 192); background-color: rgb(255, 255, 255); "
                                "font-size: %1;white-space: pre;\"").arg(fontSize);
            } else {
                QRgb bg_color = filterItem_p->m_bg_color;
                if (bg_color == BACKGROUND_COLOR) {
                    bg_color = Q_RGB(255, 255, 255);
                }
                style = QString("style=\"color: rgb(%1, %2, %3); background-color: rgb(%4, %5, %6); "
                                "font-size: %7;white-space: pre;\"")
                            .arg(qRed(filterItem_p->m_color))
                            .arg(qGreen(filterItem_p->m_color))
                            .arg(qBlue(filterItem_p->m_color))
                            .arg(qRed(bg_color))
                            .arg(qGreen(bg_color))
                            .arg(qBlue(bg_color))
                            .arg(fontSize);
            }

            text.remove(selection_p->endCol + 1, text.size());
            text.remove(0, selection_p->startCol);
            if (!first) {
                plain_text += QString("\n");
            }
            html_text += QString("<span %1>%2</span><br>").arg(style).arg(QString(text));
            plain_text += text;
            first = false;
        }
    } catch (...) {
        html_text = QString("Too many selections");
    }

    html_text += QString("</p>");
    html_text += tail;
    mimeData->setHtml(html_text);
    mimeData->setText(plain_text);
    clipboard->setMimeData(mimeData);

    unsetCursor();
}

/***********************************************************************************************************************
*   PointToCursor
***********************************************************************************************************************/
bool CEditorWidget::PointToCursor(ScreenPoint_t *screenPoint_p, int *screenRow_p, int *screenCol_p,
                                  bool *overHalf)
{
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    int TIA_row;

    *screenRow_p = 0;
    *screenCol_p = 0;
    if (overHalf != nullptr) {
        *overHalf = false;
    }

    if ((screenPoint_p->mouse.x() < m_textWindow.left()) ||
        (screenPoint_p->mouse.x() > m_textWindow.right()) ||
        (screenPoint_p->mouse.y() < m_textWindow.top()) ||
        (screenPoint_p->mouse.y() > m_textWindow.bottom())) {
        return false;
    }

    /* Find the row */

    bool found = false;
    int index = 0;

    while (m_screenRows[index].valid && !found) {
        if ((m_screenRows[index].screenRect.top() <= screenPoint_p->DCBMP.y()) &&
            (m_screenRows[index].screenRect.bottom() >= screenPoint_p->DCBMP.y())) {
            found = true;
        } else {
            ++index;
        }
    }

    *screenRow_p = index;
    if ((int)*screenRow_p > m_maxDisplayRows - 1) {
        *screenRow_p = m_maxDisplayRows - 1;
    }

    if ((int)*screenRow_p < 0) {
        *screenRow_p = 0;
    }

    if (m_screenRows[*screenRow_p].valid) {
        TIA_row = m_screenRows[*screenRow_p].row;
    } else {
#ifdef _DEBUG

        /* Until the screen is as large as the text the below print cannot be a WARNING */
        TRACEX_DE("CEditorWidget::PointToCursor  WARNING screenRow:%d NOT VALID y:%d w_top:%d height:%d",
                  *screenRow_p, screenPoint_p->DCBMP.y(), m_textWindow.top(), m_rowHeigth);
#endif

        update();
        return false;
    }

    if ((TIA_row <= doc_p->m_database.TIA.rows) &&
        (m_screenRows[*screenRow_p].screenRect.top() <= screenPoint_p->DCBMP.y()) &&
        (m_screenRows[*screenRow_p].screenRect.bottom() >= screenPoint_p->DCBMP.y())) {
        char *text_p = nullptr;
        int textSize = 0;
        int x_pos_left = 0;
        int x_pos_right = 0;
        int x_offset = 0;
        QSize size;

        doc_p->GetTextItem(TIA_row, &text_p, &textSize);

        /* The row was selected, now find out in which column. Necessary to search through the row for each letter */

        x_offset = m_screenRows[*screenRow_p].screenRect.left();
        x_pos_left = x_offset;

        if (screenPoint_p->DCBMP.x() <= x_pos_left) {
            *screenCol_p = 0;
            return true;
        }

        /* Pre-search */

        auto mousePoint = screenPoint_p->DCBMP.x();
        QSize oneLetter = GetTabbedSize("x", 1, m_blackFont_p->font_p);
        const int halfLetterWidth = oneLetter.width() / 2;
        int initial_index = (mousePoint - x_offset) / oneLetter.width();

        /* search for closest char left to the mouse */

        size = GetTabbedSize(text_p, initial_index, m_blackFont_p->font_p);

        while (mousePoint < (size.width() + x_offset)) {
            size = GetTabbedSize(text_p, --initial_index, m_blackFont_p->font_p);
        }

        for (index = initial_index; index < textSize; ++index) {
            size = GetTabbedSize(text_p, index + 1, m_blackFont_p->font_p);

            x_pos_right = x_offset + size.width();

            if (mousePoint <= x_pos_right) {
                if (LOG_TRACE_SELECTION) {
                    QRect rect;

                    rect.setLeft(mousePoint);
                    rect.setRight(x_pos_right);
                    rect.setTop(m_screenRows[*screenRow_p].screenRect.bottom() - 3);
                    rect.setBottom(m_screenRows[*screenRow_p].screenRect.bottom());

                    m_isDebuggingSelection = true;
                    m_debuggingSelection = rect;
                }

                *screenCol_p = index;

                /* Note: Index is pointing at current letter
                 * check if close to the next letter */

                *overHalf = false;
                if (mousePoint >= (x_pos_right - halfLetterWidth)) {
                    *screenCol_p = index + 1;
                    if (overHalf != nullptr) {
                        *overHalf = true;
                    }
                }

                if (*screenCol_p >= (int)textSize) {
                    if (textSize <= 0) {
                        *screenCol_p = 0;
                    } else {
                        *screenCol_p = (int)textSize;
                    }
                }
                return true;
            }

            x_pos_left = x_pos_right;
        }

        if (mousePoint >= x_pos_right) {
            if (textSize == 0) {
                *screenCol_p = 0;
            } else {
                *screenCol_p = (int)textSize;
            }
            return true;
        }

#ifdef _DEBUG
        PRINT_SELECTION(
            "CEditorWidget::PointToCursor Search failed (%d), TextSize:%d, last x_pos_left:%d x_pos_right:%d",
            index,
            textSize,
            x_pos_left,
            x_pos_right);
#endif
    } else {
#ifdef _DEBUG
        TRACEX_DE("CEditorWidget::PointToCursor NOT IN BOX  top:%d bottom:%d screen_row:%d",
                  m_screenRows[*screenRow_p].screenRect.top(), m_screenRows[*screenRow_p].screenRect.bottom(),
                  m_screenRows[*screenRow_p].row);
#endif
    }

    return false;
}

/***********************************************************************************************************************
*   GetSelectionRect
***********************************************************************************************************************/
bool CEditorWidget::GetSelectionRect(CSelection *selection_p, QRect *rect_p)
{
    for (int index = 0; index < LOG_SCRUTINIZER_MAX_SCREEN_ROWS && m_screenRows[index].valid; ++index) {
        if (m_screenRows[index].row == selection_p->row) {
            CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
            QSize size;
            char *text_p;
            int textSize;

            rect_p->setRect(0, 0, 0, 0);

            rect_p->setTop(m_screenRows[index].screenRect.top());
            rect_p->setBottom(m_screenRows[index].screenRect.bottom());

            doc_p->GetTextItem(m_screenRows[index].row, &text_p, &textSize);

            if (selection_p->endCol < (int)textSize - 1) {
                size = GetTabbedSize(text_p, selection_p->startCol, m_blackFont_p->font_p);
                rect_p->setLeft(m_screenRows[index].screenRect.left() + size.width());

                size = GetTabbedSize(text_p, selection_p->endCol + 1, m_blackFont_p->font_p);
                rect_p->setRight(m_screenRows[index].screenRect.left() + size.width());
            }
            return true;
        }
    }
    return false;
}

/***********************************************************************************************************************
*   GetSelectionRect
*
* Return the CURRENT position of the row column
***********************************************************************************************************************/
bool CEditorWidget::RowColumnToRect(int row, int screenCol, QRect *rect_p)
{
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    int index = 0;
    bool found = false;
    char *text_p = nullptr;
    int textSize = 0;

    doc_p->GetTextItem(row, &text_p, &textSize);

    while (m_screenRows[index].valid && !found) {
        if (m_screenRows[index].row == row) {
            found = true;
        } else {
            ++index;
        }
    }

    if (!found) {
#ifdef _DEBUG
        TRACEX_DE("CEditorWidget::RowColumnToRect  Line not among screen rows %d", row)
#endif
        return false;
    }

    if (screenCol <= textSize) {
        QSize size(0, 0);

        if (screenCol > 0) {
            size = GetTabbedSize(text_p, screenCol, m_blackFont_p->font_p);
        }

        /*rect_p->setLeft(m_screenRows[index].screenRect.left() + size.width() - m_hbmpOffset); */
        rect_p->setLeft(m_screenRows[index].screenRect.left() + size.width());
        rect_p->setRight(rect_p->left());
        rect_p->setTop(m_screenRows[index].screenRect.top());
        rect_p->setBottom(m_screenRows[index].screenRect.bottom());

        return true;
    }
    return false;
}

/***********************************************************************************************************************
*   PointToColumn
***********************************************************************************************************************/
bool CEditorWidget::PointToColumn(ScreenPoint_t *screenPoint_p, int *screenCol_p, QPoint *alignedPoint_p)
{
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    int TIA_row;

    *screenCol_p = 0;

    TIA_row = m_maxColWidthRow;

    char *text_p = nullptr;
    int textSize = 0;
    int x_pos_left = 0;
    int x_pos_right = 0;
    int x_offset = 0;
    QSize size;
    int index;

    doc_p->GetTextItem(TIA_row, &text_p, &textSize);

    *alignedPoint_p = screenPoint_p->DCBMP;

    /* The row was selected, now find out in which column. Necessary to search through the row for each letter */

    x_offset = m_screenRows[0].screenRect.left();
    x_pos_left = x_offset;

    if (screenPoint_p->DCBMP.x() <= x_pos_left) {
        *screenCol_p = 0;

        TRACEX_DE("CEditorWidget::PointToColumn Before line, textLeft:%-4d col:%-4d",
                  m_screenRows[0].screenRect.left(),
                  *screenCol_p);

        alignedPoint_p->setX(x_pos_left);
        return true;
    }

    /* Pre-search */
    QSize oneLetter = GetTabbedSize(text_p, 1, m_blackFont_p->font_p);
    int initial_index = (screenPoint_p->DCBMP.x() - x_offset) / oneLetter.width();

    for (index = initial_index; index < textSize; ++index) {
        size = GetTabbedSize(text_p, index + 1, m_blackFont_p->font_p);

        x_pos_right = x_offset + size.width();

        if ((screenPoint_p->DCBMP.x() >= x_pos_left) && (screenPoint_p->DCBMP.x() <= x_pos_right)) {
            *screenCol_p = index + 1;

            if (*screenCol_p >= textSize) {
                if (textSize == 0) {
                    *screenCol_p = 0;
                } else {
                    *screenCol_p = textSize - 1;
                }
            }
            alignedPoint_p->setX(x_pos_right);
            TRACEX_DE("CEditorWidget::PointToColumn  Column:%d", *screenCol_p)
            return true;
        }
        x_pos_left = x_pos_right;
    }

    if (screenPoint_p->DCBMP.x() >= x_pos_right) {
        if (textSize == 0) {
            *screenCol_p = 0;
        } else {
            *screenCol_p = textSize - 1;
        }
        alignedPoint_p->setX(x_pos_left);
        TRACEX_DE("CEditorWidget::PointToColumn  Column:%d", *screenCol_p)
        return true;
    }

    TRACEX_W("CEditorWidget::PointToCursor Search failed (%d), TextSize:%d, last x_pos_left:%d x_pos_right:%d",
             index,
             textSize,
             x_pos_left,
             x_pos_right);

    return false;
}

/***********************************************************************************************************************
*   ColumnToPoint
***********************************************************************************************************************/
bool CEditorWidget::ColumnToPoint(int screenCol, int& x)
{
    static char temp[CFG_TEMP_STRING_MAX_SIZE] = "";

    Q_ASSERT(screenCol <= CFG_TEMP_STRING_MAX_SIZE);

    if (screenCol >= CFG_TEMP_STRING_MAX_SIZE) {
        TRACEX_W(tr("%1 Bad input %2").arg(__FUNCTION__).arg(screenCol))
        return false;
    }

    if (temp[0] == 0) {
        memset(temp, 'X', sizeof(temp) - 1);
        temp[CFG_TEMP_STRING_MAX_SIZE - 1] = 0;
    }

    /* Get the length/size of a typical string up to screenCol */
    QSize size = GetTabbedSize(temp, screenCol, m_blackFont_p->font_p);
    x = m_textRow_X.left() + size.width();

    return true;
}

/***********************************************************************************************************************
*   CursorTo_TIA_Index
***********************************************************************************************************************/
int CEditorWidget::CursorTo_TIA_Index(int screenRow)
{
    CLogScrutinizerDoc *doc_p = GetDocument();

    if (m_screenRows[screenRow].valid) {
        if (m_screenRows[screenRow].row >= doc_p->m_database.TIA.rows) {
            TRACEX_E("CEditorWidget::isRowSelected  called with nullptr")
            return 0;
        }

        return m_screenRows[screenRow].row;
    }

    TRACEX_W("CEditorWidget::CursorTo_TIA_Index  screenRow:%d not valid", screenRow)
    return 0;
}

/***********************************************************************************************************************
*   ToggleBookmark
***********************************************************************************************************************/
void CEditorWidget::ToggleBookmark(void)
{
    int row;

    TRACEX_DE(QString("%1").arg(__FUNCTION__))

    if (m_cursorActive) {
        row = m_cursorSel.row;
    } else {
        CSelection selection;
        if (!GetActiveSelection(&selection)) {
            TRACEX_W(QString("%1  No active selection or cursor").arg(__FUNCTION__))
            return;
        }
        row = selection.row;
    }

    QString noComment = "";
    g_workspace_p->ToggleBookmark(this, &noComment, row);

    if (!m_inFocus) {
        setFocus();
    }
}

/***********************************************************************************************************************
*   NextBookmark
***********************************************************************************************************************/
void CEditorWidget::NextBookmark(bool backward)
{
    CLogScrutinizerDoc *doc_p = (CLogScrutinizerDoc *)GetDocument();
    CSelection selection;
    int row;
    bool status = false;

    TRACEX_DE(QString("%1").arg(__FUNCTION__))

    if (GetActiveSelection(&selection) || GetCursorPosition(&selection)) {
        status = g_workspace_p->NextBookmark(selection.row, &row, backward);
    } else {
        status = g_workspace_p->NextBookmark(m_topLine, &row, backward);
    }

    if (status && (row <= doc_p->m_database.TIA.rows)) {
        SetFocusRow(row);
        update();
    }
}

/***********************************************************************************************************************
*   isRowVisible
***********************************************************************************************************************/
bool CEditorWidget::isRowVisible(const int row)
{
    int index = 0;

    while (m_screenRows[index].valid) {
        if (m_screenRows[index].row == row) {
            return true;
        }
        ++index;
    }
    return false;
}

/***********************************************************************************************************************
*   isCursorVisible
***********************************************************************************************************************/
bool CEditorWidget::isCursorVisible(void)
{
    if (m_cursorActive) {
        return isRowVisible(m_cursorSel.row);
    }

    return false;
}

/***********************************************************************************************************************
*   GetScreenRowOffset
***********************************************************************************************************************/
int CEditorWidget::GetScreenRowOffset(const int row)
{
    int index = 0;

    while (m_screenRows[index].valid) {
        if (m_screenRows[index].row == row) {
            return index;
        }
        ++index;
    }
    return 0;
}

/***********************************************************************************************************************
*   FillRockScroll
*
* The rock scroll is the coloring on the right hand side, e.g. showing filter matches in relation to the total file.
*
***********************************************************************************************************************/
void CEditorWidget::FillRockScroll(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if ((m_vscrollFrame.height() <= 10) || (m_vscrollFrame.width() <= 2)) {
        return;
    }

    /* Replace the current rock scroll image */
    m_rockScroll_All = QImage(m_vscrollFrame.width(), m_vscrollFrame.height(), QImage::Format_RGB32);
    m_rockScroll_All.fill(SCROLLBAR_FRAME_COLOR);

    if (m_totalNumOfRows == 0) {
        return;
    }

    QPainter rs_painter(&m_rockScroll_All);

    if (m_rockScrollInfo.numberOfItems != m_vscrollFrame.height()) {
        if (m_rockScrollInfo.itemArray_p != nullptr) {
            free(m_rockScrollInfo.itemArray_p);
        }

        /* +100 some extra */
        m_rockScrollInfo.itemArray_p =
            reinterpret_cast<RockScrollItem_t *>(malloc(sizeof(RockScrollItem_t) *
                                                        static_cast<size_t>(m_vscrollFrame.height() + 100)));
        memset(m_rockScrollInfo.itemArray_p, 0,
               sizeof(RockScrollItem_t) * static_cast<size_t>(m_vscrollFrame.height() + 100));

        m_rockScrollInfo.itemArraySize = m_vscrollFrame.height() + 100; /* MAX size */
    }

    double deltaRasterPerRow;
    double rowsPerRaster;
    double y = 0.0;
    int next_y = 1;
    uint8_t bestLUT = 0;
    int bestLUT_ref = 0;    /* Row, or packed FIR index in-case of PRESENTATION_MODE_ONLY_FILTERED_e */
    packed_FIR_t *packedFIR_p = &doc_p->m_database.packedFIRA_p[0];
    rowsPerRaster = static_cast<double>(m_totalNumOfRows) / static_cast<double>(m_vscrollFrame.height());

    m_rockScrollInfo.rowsPerRaster = static_cast<int>(rowsPerRaster);
    m_rockScrollInfo.numberOfItems = 0;

    if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
        if ((doc_p->m_database.FIRA.filterMatches == 0) || (m_totalNumOfRows == 0) || !g_cfg_p->m_rockSrollEnabled) {
            return;
        }

        const int MAX_ITEM_INDEX = m_maxFIRAIndex + 1; /* m_maxFIRAIndex is the last index value, not the number of */
        m_rockScrollInfo.itemArray_p[0].startRow = packedFIR_p[0].row;

        if (rowsPerRaster > 1.0) {
            /* More rows than rasters */
            deltaRasterPerRow =
                static_cast<double>(m_vscrollFrame.height()) / static_cast<double>(MAX_ITEM_INDEX - m_minFIRAIndex);

            for (int index = m_minFIRAIndex; index < MAX_ITEM_INDEX; ++index) {
                uint8_t currentLUT = packedFIR_p[index].LUT_index;

                if ((bestLUT == 0) || (currentLUT < bestLUT)) {
                    bestLUT = currentLUT;
                    bestLUT_ref = index;
                }

                if (static_cast<int>(y) >= next_y) {
                    /* ensure that we get the last index
                     * Enters here when we have reached as NEW section, now fill the color for the previous
                     * Define Y and Height */
                    if (bestLUT != 0) {
                        if (next_y >= (m_vscrollFrame.height() - 1)) {
                            next_y = m_vscrollFrame.height() - 1;
                        }

                        /* x. fill 1 line for previous saved section */
                        Q_COLORREF color = doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color == BACKGROUND_COLOR ?
                                           doc_p->m_database.filterItem_LUT[bestLUT]->m_color :
                                           doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color;

                        rs_painter.fillRect(0, next_y - 1, m_vscrollFrame.width(), 1, color);

                        m_rockScrollInfo.itemArray_p[next_y - 1].color = color;
                        m_rockScrollInfo.itemArray_p[next_y - 1].bestRow = packedFIR_p[bestLUT_ref].row;
                        m_rockScrollInfo.itemArray_p[next_y - 1].y_line = next_y - 1;
                        m_rockScrollInfo.itemArray_p[next_y - 1].endRow = packedFIR_p[index - 1].row;
                        m_rockScrollInfo.itemArray_p[next_y].startRow = packedFIR_p[index].row;
                        m_rockScrollInfo.numberOfItems++;
                    }
                    ++next_y;
                    bestLUT = 0;
                } /* if time to draw line */
                y += deltaRasterPerRow;
            } /* for each filter */

#ifdef _DEBUG
            if (m_rockScrollInfo.numberOfItems > m_vscrollFrame.height()) {
                TRACEX_E("%s Too many items num:%d pixels:%d", __FUNCTION__,
                         m_rockScrollInfo.numberOfItems, m_vscrollFrame.height());
            }
#endif
        } else {
            /* more pixels/raster then rows */
            double rasterHeight = static_cast<double>(m_vscrollFrame.height()) /
                                  static_cast<double>(m_totalNumOfRows);
            int y_start_pix = 0;
            int y_end_pix = 0;
            int y_fill_pix = 0;

            for (int index = m_minFIRAIndex; index < MAX_ITEM_INDEX; ++index) {
                uint8_t currentLUT = packedFIR_p[index].LUT_index;  /* packedFIR_p doesn't contain exclude filters
                                                                     * */
                y_start_pix = static_cast<int>(y);
                y_end_pix = y_start_pix + static_cast<int>(rasterHeight + 0.5);
                y_fill_pix = y_end_pix - y_start_pix + 1;

                Q_COLORREF color = doc_p->m_database.filterItem_LUT[currentLUT]->m_bg_color == BACKGROUND_COLOR ?
                                   doc_p->m_database.filterItem_LUT[currentLUT]->m_color :
                                   doc_p->m_database.filterItem_LUT[currentLUT]->m_bg_color;

                rs_painter.fillRect(0, y_start_pix, m_vscrollFrame.width(), y_fill_pix, color);

                /* memDC.FillSolidRect(0, y_start_pix, m_vscrollFrame.Width(), y_fill_pix, color);
                 * Fill m_rockScrollInfo from y to (int)(rasterHeight + 0.5f) */
                for (int infoIndex = static_cast<int>(y);
                     infoIndex <= static_cast<int>(y + rasterHeight + 0.5);
                     ++infoIndex) {
                    m_rockScrollInfo.itemArray_p[infoIndex].bestRow = packedFIR_p[index].row;
                    m_rockScrollInfo.itemArray_p[infoIndex].y_line = infoIndex;
                    m_rockScrollInfo.itemArray_p[infoIndex].endRow = packedFIR_p[index].row;
                    m_rockScrollInfo.itemArray_p[infoIndex].startRow = packedFIR_p[index].row;
                    m_rockScrollInfo.itemArray_p[infoIndex].color = color;
                    m_rockScrollInfo.numberOfItems = infoIndex;
                }
                y += rasterHeight;
            } /* for each filter */
            m_rockScrollInfo.numberOfItems++;
#ifdef _DEBUG
            if (m_rockScrollInfo.numberOfItems > (m_vscrollFrame.height() + 1)) {
                TRACEX_E("%s Too many items num:%d pixels:%d", __FUNCTION__,
                         m_rockScrollInfo.numberOfItems, m_vscrollFrame.height() + 1);
            }
#endif
        }
    }  /* Presentation Mode */
    else {
        /* Presentation mode all */
        double rasterSize = 1.0;
        deltaRasterPerRow = static_cast<double>(m_vscrollFrame.height()) /
                            static_cast<double>(m_totalNumOfRows);
        rowsPerRaster = static_cast<double>(m_totalNumOfRows) /
                        static_cast<double>(m_vscrollFrame.height());

        const int NUM_ROWS = m_totalNumOfRows;
        m_rockScrollInfo.itemArray_p[0].startRow = 0;

        if (deltaRasterPerRow > 1.0) {
            /* More pixels than rows */
            rasterSize = deltaRasterPerRow;
            next_y = static_cast<int>(rasterSize);

            /* We only loop of the the filter table, non-filtered lines we skip since they will be grayed anyway */
            for (int index = 0; index < doc_p->m_database.FIRA.filterMatches; ++index) {
                y = doc_p->m_database.packedFIRA_p[index].row * deltaRasterPerRow;
                bestLUT = doc_p->m_database.packedFIRA_p[index].LUT_index;

                Q_COLORREF color = doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color == BACKGROUND_COLOR ?
                                   doc_p->m_database.filterItem_LUT[bestLUT]->m_color :
                                   doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color;

                rs_painter.fillRect(0,
                                    static_cast<int>(y + 0.5),
                                    m_vscrollFrame.width(),
                                    static_cast<int>(deltaRasterPerRow + 0.5),
                                    color);
                for (int infoIndex = static_cast<int>(y);
                     infoIndex < static_cast<int>(y + deltaRasterPerRow + 0.5);
                     ++infoIndex) {
                    m_rockScrollInfo.itemArray_p[infoIndex].color = color;
                    m_rockScrollInfo.itemArray_p[infoIndex].bestRow = packedFIR_p[index].row;
                    m_rockScrollInfo.itemArray_p[infoIndex].y_line = infoIndex;
                    m_rockScrollInfo.itemArray_p[infoIndex].endRow = packedFIR_p[index].row;
                    m_rockScrollInfo.itemArray_p[infoIndex].startRow = packedFIR_p[index].row;
                    m_rockScrollInfo.numberOfItems = infoIndex;
                }
            }
        } else {
            /* More rows than pixels */
            if ((doc_p->m_database.FIRA.filterMatches != 0) && (m_totalNumOfRows != 0) && g_cfg_p->m_rockSrollEnabled) {
                double nextRow = rowsPerRaster;
                int nextRow_int = static_cast<int>(nextRow + 0.5);
                int index = 0;

                for (int row = 0; row < NUM_ROWS; ++row) {
                    uint8_t currentLUT = doc_p->m_database.FIRA.FIR_Array_p[row].LUT_index;
                    if ((bestLUT == 0) || ((currentLUT < bestLUT) && (currentLUT != 0))) {
                        bestLUT = currentLUT;
                        bestLUT_ref = row;
                    }
                    if (row == nextRow_int) {
                        Q_COLORREF color = BACKGROUND_COLOR;

                        /* If bestLUT is 0 then there was not filtered row found for the searched rows. */
                        if (bestLUT != 0) {
                            m_rockScrollInfo.itemArray_p[index].bestRow = bestLUT_ref;

                            color = doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color == BACKGROUND_COLOR ?
                                    doc_p->m_database.filterItem_LUT[bestLUT]->m_color :
                                    doc_p->m_database.filterItem_LUT[bestLUT]->m_bg_color;

                            rs_painter.fillRect(0, index, m_vscrollFrame.width(), 1, color);
                            bestLUT = 0;
                        }
                        m_rockScrollInfo.itemArray_p[index].color = color;
                        m_rockScrollInfo.itemArray_p[index].y_line = index;
                        m_rockScrollInfo.itemArray_p[index].endRow = row;
                        m_rockScrollInfo.itemArray_p[index + 1].startRow = row + 1;
                        m_rockScrollInfo.numberOfItems++;
                        nextRow += rowsPerRaster;
                        nextRow_int = static_cast<int>(nextRow + 0.5);
                        ++index;

                        if (nextRow >= NUM_ROWS - 1) {
                            /* make sure that last raster contains the final rows */
                            nextRow = static_cast<double>(NUM_ROWS - 1);
                            nextRow_int = static_cast<int>(nextRow + 0.5);
                        }
                    }
                } /* for */
#ifdef _DEBUG
                if (m_rockScrollInfo.numberOfItems > m_vscrollFrame.height()) {
                    TRACEX_E("%s Too many items num:%d pixels:%d", __FUNCTION__,
                             m_rockScrollInfo.numberOfItems, m_vscrollFrame.height());
                }
#endif
            } /*  (deltaRasterPerRow > 1.0f) */
        }

        /* Add the brush color-hashed rowClip */

        if ((g_cfg_p->m_Log_rowClip_Start > 0) || (g_cfg_p->m_Log_rowClip_End > 0)) {
            rs_painter.setBrush(QBrush(CLIPPED_TEXT_ROCKSCROLL_COLOR, Qt::FDiagPattern));

            /*  rs_painter.setPen(CLIPPED_TEXT_COLOR); */

            if (g_cfg_p->m_Log_rowClip_Start > 0) {
                int startRaster = 0;
                int endRaster = static_cast<int>(deltaRasterPerRow *
                                                 static_cast<double>(g_cfg_p->m_Log_rowClip_Start + 0.5));
                QRect rect(0, startRaster, m_vscrollFrame.width(), endRaster);

                /* Draw first a light red backround frame, then put a black diag pattern ontop */
                rs_painter.setBrush(QBrush(CLIPPED_TEXT_ROCKSCROLL_COLOR, Qt::SolidPattern));
                rs_painter.drawRect(rect);
                rs_painter.setBrush(QBrush(Qt::black, Qt::FDiagPattern));
                rs_painter.drawRect(rect);
            }

            if (g_cfg_p->m_Log_rowClip_End > 0) {
                int startRaster = static_cast<int>(deltaRasterPerRow *
                                                   static_cast<double>(g_cfg_p->m_Log_rowClip_End + 0.5));
                int endRaster = m_vscrollFrame.height() - 1;
                QRect rect(0, startRaster, m_vscrollFrame.width(), endRaster);

                /* Draw first a light red backround frame, then put a black diag pattern ontop */
                rs_painter.setBrush(QBrush(CLIPPED_TEXT_ROCKSCROLL_COLOR, Qt::SolidPattern));
                rs_painter.drawRect(rect);
                rs_painter.setBrush(QBrush(Qt::black, Qt::FDiagPattern));
                rs_painter.drawRect(rect);
            }
        }
    } /* presentationMode */

#ifdef _DEBUG
    QFile file("rockscroll.txt");

    if (!file.open(QIODevice::ReadWrite)) {
        return;
    }

    QTextStream fileStream(&file);
    for (int index = 0; index < m_rockScrollInfo.numberOfItems; ++index) {
        RockScrollItem_t *rsi = &m_rockScrollInfo.itemArray_p[index];
        fileStream << QString("[%1] br:%2 sr:%3 er:%4 ypix:%5 color:%6\n")
            .arg(index).arg(rsi->bestRow).arg(rsi->startRow).arg(rsi->endRow).arg(rsi->y_line).arg(rsi->color);
    }
#endif
}

/***********************************************************************************************************************
*   AlignRelPosToRockScroll
***********************************************************************************************************************/
void CEditorWidget::AlignRelPosToRockScroll(int vscrollPos)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    int OldTopLine = m_topLine;

    PRINT_ROCKSCROLL(QString("%1 vslider_h:%2 rowPerRaster:%3 bestRow:%4").
                         arg(__FUNCTION__).arg(m_vscrollSlider.height()).arg(m_rockScrollInfo.rowsPerRaster).arg(
                         m_rockScrollInfo.itemArray_p[vscrollPos].bestRow));

    if ((m_vscrollSlider.height() < 3) &&
        (m_rockScrollInfo.rowsPerRaster > 1) &&
        (m_totalNumOfRows > 0) &&
        (doc_p->m_database.FIRA.filterMatches > 0) &&
        (vscrollPos < m_rockScrollInfo.numberOfItems) &&
        (m_rockScrollInfo.itemArray_p[vscrollPos].bestRow != 0)) {
        /* x. Search the RockScroll and find the range in-which the row/FIRA_index is within.
         * x. Re-calculate the topRow based on the "chosen" filter and row, such that is displayed in the middle of the
         * screen */
        int middleRow = m_rockScrollInfo.itemArray_p[vscrollPos].bestRow;   /* middle row */
        if (m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) {
            int rowRef;
            rowRef = doc_p->m_database.FIRA.FIR_Array_p[middleRow].index;
            rowRef -= m_maxDisplayRows / 2;
            rowRef = rowRef > 0 ? rowRef : 0;
            m_topLine = doc_p->m_database.packedFIRA_p[rowRef].row;
        } else {
            m_topLine = middleRow - m_maxDisplayRows / 2;
        }

        m_topLine = m_topLine >= 0 ? m_topLine : 0;
        UpdateRelPosition();

        PRINT_ROCKSCROLL(QString("%1 %2 -> %3  Raster:%4").arg(__FUNCTION__).arg(OldTopLine).arg(m_topLine).arg(
                             vscrollPos));
    }
}

/***********************************************************************************************************************
*   InvalidateRockScroll
***********************************************************************************************************************/
void CEditorWidget::InvalidateRockScroll(void)
{
    m_rockScroll_Valid = false;
}

#ifdef _DEBUG

/***********************************************************************************************************************
*   CheckRockScroll
***********************************************************************************************************************/
void CEditorWidget::CheckRockScroll(void)
{
 #ifdef QT_TODO

    /* check the scrollSlider and the rockScroll */

    if ((m_rockScrollInfo.numberOfItems == 0) || (m_rockScrollInfo.itemArray_p == nullptr)) {
        return;
    }

    if ((m_presentationMode == PRESENTATION_MODE_ONLY_FILTERED_e) && (doc_p->m_database.FIRA.filterMatches > 0)) {
        /* 1. Find location in rockScroll that match the top and end of the scroll slider
         * 2. Make sure that topLine is within the start and end of the matching rockScroll entry */

        int errorCount = 0;
        for (int index = 0; index < m_rockScrollInfo.numberOfItems; ++index) {
            if (m_rockScrollInfo.itemArray_p[index].startRow <0 ||
                                                              m_rockScrollInfo.itemArray_p[index].startRow>
                m_rockScrollInfo.itemArray_p[index].endRow ||
                (m_rockScrollInfo.itemArray_p[index].endRow < 0) ||
                m_rockScrollInfo.itemArray_p[index].y_line <0 ||
                                                            m_rockScrollInfo.itemArray_p[index].y_line> m_vscrollFrame.
                    Height()) {
                ++errorCount;

                if (errorCount == 1) {
  #ifdef _DEBUG
                    TRACEX_W("CEditorWidget::CheckRockScroll    m_rockScrollInfo.itemArray_p corrupt")
  #endif
                }
            }
        }

        int top_entry;
        int bottom_entry;
        int FIRA_index_half = doc_p->m_database.FIRA.FIR_Array_p[m_topLine].index + m_maxDisplayRows / 2;

        /* double topLine_relPos       = (double)FIRA_index_half / (double)doc_p->m_database.FIRA.filterMatches;
         * double scroll_relPos        = ((double)(m_vscrollSlider.top + ((double)m_vscrollSliderHeight) / 2.0)) /
         * ((double)m_vscrollFrame.Height() -  ((double)m_vscrollSliderHeight / 2.0)); */
        double linesPerPixel = (double)doc_p->m_database.FIRA.filterMatches / (double)(m_vscrollFrame.Height());
        double relAccuracy = 1.0 / linesPerPixel;

        top_entry = m_rockScrollInfo.itemArray_p[m_vscrollSlider.top - m_vscrollFrame.top].startRow;
        bottom_entry = m_rockScrollInfo.itemArray_p[m_vscrollSlider.bottom - m_vscrollFrame.top].endRow;
    }
 #endif
}
#endif

/***********************************************************************************************************************
*   RockScrollRasterToRow
***********************************************************************************************************************/
void CEditorWidget::RockScrollRasterToRow(int raster, int *startRow_p, int *endRow_p)
{
    *startRow_p = -1;
    *endRow_p = -1;

    /* check the scrollSlider and the rockScroll */

    if ((m_rockScrollInfo.numberOfItems == 0) || (m_rockScrollInfo.itemArray_p == nullptr)) {
        return;
    }

    for (int index = 0; index < m_rockScrollInfo.numberOfItems; ++index) {
        if (m_rockScrollInfo.itemArray_p[index].y_line == raster) {
            *startRow_p = m_rockScrollInfo.itemArray_p[index].startRow;
            *endRow_p = m_rockScrollInfo.itemArray_p[index].endRow;
            PRINT_ROCKSCROLL(QString("%1 raster:%2 -> startRow:%2 endRow:%3")
                                 .arg(__FUNCTION__).arg(*startRow_p).arg(*endRow_p));
            return;
        }
    }
}

/***********************************************************************************************************************
*   UpdateCursor
***********************************************************************************************************************/
void CEditorWidget::UpdateCursor(bool isOnDraw, int row, int startCol)
{
    if (m_totalNumOfRows == 0) {
        DisableCursor();
        PRINT_CURSOR("Disable cursor, no rows");
        return;
    }

    if (!isOnDraw) {
        /* When calling UpdateCursor from OnDraw it is not clear for the caller if the cursor should be visible or not
         * Typically during a drag selection the cursor shall not be shown */
        m_cursorActive = true;
    }

    if (startCol == CURSOR_UPDATE_FULL_ROW) {
        /* full line selection */
        startCol = 0;
    }

    if (row != CURSOR_UPDATE_NO_ACTION) {
        m_cursorSel.row = row;
    }

    if (startCol != CURSOR_UPDATE_NO_ACTION) {
        m_cursorSel.startCol = startCol;
        m_cursorSel.endCol = startCol;
    }

    if (!RowColumnToRect(m_cursorSel.row, m_cursorSel.startCol, &m_cursorRect)) {
        /*DisableCursor(); // Added this during QT porting (untested) */
        PRINT_CURSOR("Disable cursor, RowColumnToRect failure");
        return;
    }

    m_cursorRect.setRight(m_cursorRect.left() + 1);

    if (!isCursorVisible()) {
        m_cursorToggleVisible = false;
    }

    if ((m_cursorToggleVisible && isOnDraw)) {
        m_painter_p->fillRect(m_cursorRect.left(), m_cursorRect.top(), 1, m_cursorRect.height(),
                              QColor(Qt::black)); /* draw only in the paintEvent */
    }

    if (!isOnDraw) {
        PRINT_CURSOR(QString("%1    isOnDraw:%2 row:%3 startCol:%4 drag:%5")
                         .arg(__FUNCTION__).arg(isOnDraw).arg(row).arg(startCol).arg(m_dragSelectionOngoing));

        /* Restart the timer, the cursor was updated... let get the blinking a new period start */
        m_cursorToggleVisible = true;
        m_cursorTimer->stop();
        m_cursorTimer->start(CURSOR_TIMER_DURATION);
    }
}

/***********************************************************************************************************************
*   OnCursorTimer
***********************************************************************************************************************/
void CEditorWidget::OnCursorTimer(void)
{
    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }

    if (m_cursorActive) {
        if (!isCursorVisible()) {
            /*if (m_cursorTimer->isActive())   // Removed this since the blink should continue even if the cursor isn't
             * shown, such that as soon as it gets visible it continues to blink
             *   m_cursorTimer->stop();*/
            return;
        }

        m_cursorToggleVisible = m_cursorToggleVisible == true ? false : true;  /* the the paintEvent draw the cursor */
        update();
    }
}

/***********************************************************************************************************************
*   DisableCursor
***********************************************************************************************************************/
void CEditorWidget::DisableCursor(void)
{
    if (m_cursorActive || m_cursorToggleVisible) {
        m_cursorActive = false;
        m_cursorToggleVisible = false;

        if (m_cursorTimer->isActive()) {
            m_cursorTimer->stop();
        }
    }
}

/***********************************************************************************************************************
*   SetFocusRow
***********************************************************************************************************************/
void CEditorWidget::SetFocusRow(int row, int cursorCol)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    EmptySelectionList();
    AddSelection(row, -1, -1, false, false);
    UpdateCursor(false, row, cursorCol);

    m_cursorDesiredCol = cursorCol;

    CFilterItem *filterItem_p = doc_p->m_rowCache_p->GetFilterRef(row);

    if ((filterItem_p == nullptr) || filterItem_p->m_exclude) {
        SetPresentationMode(PRESENTATION_MODE_ALL_e);
    }

    SearchNewTopLine(false, row);

    update();
    setFocus();
}

/***********************************************************************************************************************
*   leaveEvent
***********************************************************************************************************************/
void CEditorWidget::leaveEvent(QEvent *event)
{
    /* Leave isn't called when LMouseButton is pressed, that occurs after the button has been released, which
     * then secure that the capture is maintained. */

    /* While tracking the mouse outside of the screen keep the glue */
    if (!m_mouseTracking) {
        m_vscrollSliderGlue = false;
        m_vscrollBitmapForce = false;
        m_vscrollBitmapEnabled = false;
        update();
    }
    QWidget::leaveEvent(event);
}

/***********************************************************************************************************************
*   resizeEvent
***********************************************************************************************************************/
void CEditorWidget::resizeEvent(QResizeEvent *event)
{
    m_lastResize = event->size();
    PRINT_SIZE(QString("Editor resize %1,%2 -> %3,%4 state:%5")
                   .arg(event->oldSize().width()).arg(event->oldSize().height())
                   .arg(event->size().width()).arg(event->size().height())
                   .arg(windowState()));
    m_resizeCursorSync = true; /* Required to reset the cursor after a resize operation */
    Refresh(false);
    updateGeometry();
    MW_updatePendingStateGeometry();
    QWidget::resizeEvent(event);
}

/***********************************************************************************************************************
*   focusInEvent
***********************************************************************************************************************/
void CEditorWidget::focusInEvent(QFocusEvent *event)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_TextView_e);

    if (doc_p->m_database.TIA.rows > 0) {
        UpdateCursor(false);
    }

    ForceVScrollBitmap();
    setMouseTracking(true);

    MW_Refresh();

    m_inFocus = true;
    PRINT_FOCUS("CEditorWidget::focusInEvent");
}

/***********************************************************************************************************************
*   focusOutEvent
***********************************************************************************************************************/
void CEditorWidget::focusOutEvent(QFocusEvent *event)
{
    /*DisableCursor(); */

    m_inFocus = false;
    m_focusLostPreviously = true;

    if (m_captureOn) {
        m_captureOn = false;
    }

    setMouseTracking(false);
    m_vscrollTimer->stop();

    update();

    CSCZ_UnsetLastViewSelectionKind(CSCZ_LastViewSelectionKind_TextView_e);

    update();

    PRINT_FOCUS("CEditorWidget::focusOutEvent");
}

/***********************************************************************************************************************
*   UpdateGrayFont
***********************************************************************************************************************/
void CEditorWidget::UpdateGrayFont(void)
{
    m_grayFont_p = ((CLogScrutinizerDoc *)GetDocument())->m_fontCtrl.RegisterFont(
        Q_RGB(g_cfg_p->m_log_GrayIntensity,
              g_cfg_p->m_log_GrayIntensity, g_cfg_p->m_log_GrayIntensity), BACKGROUND_COLOR);

    TRACEX_I(QString("Update gray font - Intensity:%1").arg(g_cfg_p->m_log_GrayIntensity))
    m_EraseBkGrndDisabled = true;
    update();
}

/***********************************************************************************************************************
*   moveEvent
***********************************************************************************************************************/
void CEditorWidget::moveEvent(QMoveEvent *event)
{
    TRACEX_D("CEditorWidget::moveEvent")

    if (m_cursorTimer->isActive()) {
        m_cursorTimer->stop();
        m_cursorTimer->start(CURSOR_TIMER_DURATION);
    }
    QWidget::moveEvent(event);
}

/***********************************************************************************************************************
*   mainWindowIsMoving
***********************************************************************************************************************/
void CEditorWidget::mainWindowIsMoving(QMoveEvent *event)
{
    Q_UNUSED(event)

    /* When porting to QT it is required to do the cursor toggeling in a paint event, hence to make the cursor toggle
     * the entire window
     * is currently repainted. This causes some heavy load, and if the entire window is moved it creates some glitches
     * where it seems like the window
     * is getting stuck in the screen. The cursor timer is as such not running while the window is moving. */
    static CTimeFilter filter(500);

    /* block this print from printing more than twice per second. */
    if (filter.Check()) {
        TRACEX_D("CEditorWidget::mainWindowIsMoving")
    }

    if (m_cursorTimer->isActive()) {
        m_cursorTimer->stop();
        m_cursorTimer->start(CURSOR_TIMER_DURATION);
    }
}

/***********************************************************************************************************************
*   mousePressEvent
***********************************************************************************************************************/
void CEditorWidget::mousePressEvent(QMouseEvent *event)
{
    TRACEX_D("CEditorWidget::mousePressEvent")

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    ScreenPoint_t screenPoint = MakeScreenPoint(event, m_hbmpOffset, m_vbmpOffset);

    switch (event->button())
    {
        case Qt::LeftButton:
            OnLButtonDown(event->modifiers(), screenPoint);
            event->accept();
            break;

        /*    case Qt::RightButton:
         *           OnRButtonDown(event->modifiers(), screenPoint);
         *           event->accept();
         *           break;*/
        default:
            event->ignore();
            break;
    }
    event->accept();
}

/***********************************************************************************************************************
*   mouseMoveEvent
***********************************************************************************************************************/
void CEditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    /* (THEORY): Take the lastest mouse position, as it seems that if the painting goes slow then there might be some
     * events piling up in the event queue which
     * causes especially fast selection to look weird. At least it seems to look better. */
    QPoint cursorPos = QCursor::pos();
    ScreenPoint_t screenPoint = MakeScreenPoint_FromGlobal(this, cursorPos, m_hbmpOffset, m_vbmpOffset);

    /*ScreenPoint_t screenPoint = MakeScreenPoint(event, m_hbmpOffset, m_vbmpOffset); */

    ProcessMouseMove(event, screenPoint);
    event->accept();
    QWidget::mouseMoveEvent(event);
}

/***********************************************************************************************************************
*   mouseReleaseEvent
***********************************************************************************************************************/
void CEditorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    TRACEX_D("CEditorWidget::mouseReleaseEvent")

    ScreenPoint_t screenPoint = MakeScreenPoint(event, m_hbmpOffset, m_vbmpOffset);

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    switch (event->button())
    {
        case Qt::LeftButton:
            OnLButtonUp(screenPoint);
            event->accept();
            break;

        default:
            event->ignore();
            break;
    }
}

/***********************************************************************************************************************
*   wheelEvent
***********************************************************************************************************************/
void CEditorWidget::wheelEvent(QWheelEvent *event)
{
    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    OnMouseWheel(event);
    event->accept();
}

/***********************************************************************************************************************
*   keyPressEvent
***********************************************************************************************************************/
void CEditorWidget::keyPressEvent(QKeyEvent *e)
{
    int index = e->key();
    if (index < 256) {
        m_keys[index] = true;
    }

    PRINT_KEYPRESS(QString("%1 key:%2").arg(__FUNCTION__).arg(index));

    if (!GetTheDoc()->isValidLog()) {
        QWidget::keyPressEvent(e);
        return;
    }

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;

    switch (index)
    {
        case Qt::Key_PageDown:
            PageUpDown(false);
            break;

        case Qt::Key_PageUp:
            PageUpDown(true);
            break;

        case Qt::Key_Left:
            CursorLeftRight(true);
            break;

        case Qt::Key_Right:
            CursorLeftRight(false);
            break;

        case Qt::Key_Down:
            CursorUpDown(false);
            break;

        case Qt::Key_Up:
            CursorUpDown(true);
            break;

        case Qt::Key_Home:
            onHome();
            break;

        case Qt::Key_End:
            onEnd();
            break;

        case Qt::Key_A:
            if (m_CTRL_Pressed) {
                if (textWindow_SelectionUpdate(nullptr, LS_LG_EVENT_KEY_CTRL_A_e, 0)) {
                    update();
                }
            }
            break;

        case Qt::Key_C:
            if (m_CTRL_Pressed) {
                if (textWindow_SelectionUpdate(nullptr, LS_LG_EVENT_KEY_CTRL_C_e, 0)) {
                    update();
                }
            }
            break;

        case Qt::Key_G:
            if (m_CTRL_Pressed) {
                TRACEX_D("CEditorWidget::keyPressEvent  CTRL-G  Go to Row")
                GotoRow();
            }
            break;

        case Qt::Key_F2:
            if (m_CTRL_Pressed) {
                TRACEX_D("CEditorWidget::keyPressEvent  CTRL-F2  Add bookmark")
                ToggleBookmark();
                update();
            } else if (m_SHIFT_Pressed) {
                TRACEX_D("CEditorWidget::keyPressEvent  SHIFT-F2  Previous bookmark")
                NextBookmark(true);
            } else {
                TRACEX_D("CEditorWidget::keyPressEvent  F2  Next bookmark")
                NextBookmark();
            }
            break;

        case Qt::Key_F7:
            if (m_CTRL_Pressed) {
                g_cfg_p->m_Log_rowClip_Start = CFG_CLIP_NOT_SET;
                g_cfg_p->m_Log_rowClip_End = CFG_CLIP_NOT_SET;
                GetDocument()->UpdateTitleRow();
                Refresh();
                update();
            }
            break;

        case Qt::Key_F8:
            if (m_CTRL_Pressed) {
                SetRowClip(true);
            } else {
                if (g_cfg_p->m_Log_rowClip_Start != CFG_CLIP_NOT_SET) {
                    SetFocusRow(g_cfg_p->m_Log_rowClip_Start);
                } else {
                    MW_PlaySystemSound(SYSTEM_SOUND_QUESTION);
                    QMessageBox::information(this,
                                             tr("Row clip start not set"),
                                             tr("Row clip start not set, (use Ctrl-F8)", "LogScrutinizer - Row Clip"),
                                             QMessageBox::Ok);
                }
            }
            break;

        case Qt::Key_Escape:
            EmptySelectionList(true);
            break;

        case Qt::Key_F9:
            if (m_CTRL_Pressed) {
                SetRowClip(false);
            } else {
                if (g_cfg_p->m_Log_rowClip_End != CFG_CLIP_NOT_SET) {
                    SetFocusRow(g_cfg_p->m_Log_rowClip_End);
                } else {
                    MW_PlaySystemSound(SYSTEM_SOUND_QUESTION);
                    QMessageBox::information(this,
                                             tr("Row clip end not set"),
                                             tr("Row clip end not set, (use Ctrl-F9)", "LogScrutinizer - Row Clip"),
                                             QMessageBox::Ok);
                    TRACEX_I("Row clip end not set, (use Ctrl-F9)")
                }
            }
            break;

        default:
            if (!MW_GeneralKeyHandler(index, m_CTRL_Pressed, m_SHIFT_Pressed)) {
                QWidget::keyPressEvent(e);
            }
            break;
    }
}

/***********************************************************************************************************************
*   keyReleaseEvent
***********************************************************************************************************************/
void CEditorWidget::keyReleaseEvent(QKeyEvent *e)
{
    /*TRACEX_D("CEditorWidget::keyReleaseEvent %d", e->key()) */
    int index = e->key();
    if (index < 256) {
        m_keys[e->key()] = false;
    }

    m_CTRL_Pressed = QApplication::keyboardModifiers() & Qt::ControlModifier ? true : false;
    m_SHIFT_Pressed = QApplication::keyboardModifiers() & Qt::ShiftModifier ? true : false;
    m_ALT_Pressed = QApplication::keyboardModifiers() & Qt::AltModifier ? true : false;
    QWidget::keyReleaseEvent(e);
}

/***********************************************************************************************************************
*   onHome
***********************************************************************************************************************/
void CEditorWidget::onHome(void)
{
    if (m_CTRL_Pressed) {
        GotoTop();
    } else {
        m_hrelPos = 0.0;

        if (m_cursorActive) {
            if (m_SHIFT_Pressed) {
                /* Check if there is a selection to update */

                CSelection *selection_p;

                if (isRowSelected(m_cursorSel.row, &selection_p)) {
                    /* If cursor is to the right of the current selection */
                    if (m_cursorSel.startCol > selection_p->startCol) {
                        selection_p->endCol = selection_p->startCol - 1;
                    }

                    selection_p->startCol = 0;

                    if (selection_p->endCol < selection_p->startCol) {
                        /* if entire row was selected then endCol is -1, and startCol is 0 */
                        RemoveSelection(selection_p->row);
                    } else {
                        SelectionUpdated(selection_p);
                    }
                } else {
                    AddSelection(m_cursorSel.row, 0, m_cursorSel.startCol - 1, true, false, true);
                }
            } else {
                EmptySelectionList();
                AddDragSelection(m_cursorSel.row, 0, 0);
            }

            UpdateCursor(false, m_cursorSel.row, 0);
        }
        update();
    }
}

/***********************************************************************************************************************
*   onEnd
***********************************************************************************************************************/
void CEditorWidget::onEnd(void)
{
    if (m_CTRL_Pressed) {
        GotoBottom();
    } else {
        if (m_cursorActive && (m_cursorSel.row >= 0) && (m_cursorSel.row < GetDocument()->m_database.TIA.rows)) {
            int newCursorCol = GetDocument()->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1;

            if (m_SHIFT_Pressed) {
                /* Check if there is a selection to update */

                CSelection *selection_p;

                if (isRowSelected(m_cursorSel.row, &selection_p)) {
                    if (selection_p->endCol >=
                        (int)(GetDocument()->m_database.TIA.textItemArray_p[m_cursorSel.row].size) - 1) {
                        RemoveSelection(selection_p->row);
                    } else {
                        /* If cursor is to the right of the current selection */
                        if (m_cursorSel.startCol <= selection_p->endCol) {
                            selection_p->startCol = selection_p->endCol + 1;
                        }

                        selection_p->endCol = GetDocument()->m_database.TIA.textItemArray_p[m_cursorSel.row].size - 1;
                        SelectionUpdated(selection_p);
                    }
                } else {
                    AddSelection(m_cursorSel.row, m_cursorSel.startCol, newCursorCol, true, false, true);
                }
            } else {
                EmptySelectionList();
                AddDragSelection(m_cursorSel.row, newCursorCol, newCursorCol);
            }

            m_cursorSel.startCol = newCursorCol + 1;
            m_cursorSel.endCol = m_cursorSel.startCol;
            m_cursorDesiredCol = m_cursorSel.startCol;

            UpdateCursor(false, m_cursorSel.row, m_cursorSel.startCol);

            HorizontalCursorFocus(HCursorScrollAction_Right_en);
        }

        update();
    }
}
