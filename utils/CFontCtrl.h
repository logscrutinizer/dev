/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CConfig.h"
#include "globals.h"
#include "utils.h"

#include <QPainter>

/***********************************************************************************************************************
*   CFontCtrl
***********************************************************************************************************************/
class CFontCtrl
{
public:
    CFontCtrl(void);
    ~CFontCtrl(void);

    /* This function setup a new font based on the color, it might already exist, then no action */
    FontItem_t *RegisterFont(Q_COLORREF color, Q_COLORREF bg_color, FontMod_t fontMod = FONT_MOD_NONE);
    void SetFont(LS_Painter *pDC, FontItem_t *font_p); /**< In case other font is active this font is set active. */
    void ChangeFontSize(int size); /* Change the size of all fonts */
    int GetLogFontLetterHeight(void) {return m_fontLetterHeight;}
    int GetLogFontLetterWidth(void) {return m_fontLetterWidth;} /* Width should be the same for all letters when using a
                                                                 * fixed size font */
    QSize GetFontSize(void) {return QSize(m_fontLetterWidth, m_fontLetterHeight);}
    int GetSize(void) {return m_size;}

    void UpdateAllFonts(void);
    void UpdateAllFontsWith_fontMod(FontMod_t fontMod);

    /****/
    QSize GetTextPixelLength(LS_Painter *painter_p, QString text)
    {
        QFontMetrics fontMetric(*(painter_p->getCurrentFontItem())->font_p);
        return fontMetric.size(Qt::TextSingleLine, text);
    }

    int EnhanceColorContrast(FontItem_t *fontItem_p);

    /****/
    bool GetBGColor(FontItem_t *font_p, Q_COLORREF *color_p) {
        if (font_p->bg_color == BACKGROUND_COLOR) {
            return false;
        } else {
            *color_p = font_p->bg_color;
            return true;
        }
    }

    QFont *GetCFont(FontItem_t *font_p) {return font_p->font_p;}

private:
    QList<FontItem_t *> m_fonts; /* List of all registered fonts */
    int m_fontLetterWidth; /* All fonts has the same size */
    int m_fontLetterHeight;
    int m_size; /* All fonts has the same size */
};
