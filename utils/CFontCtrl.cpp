/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QRgb>

#include "CDebug.h"
#include "CConfig.h"
#include "CFontCtrl.h"

CFontCtrl::CFontCtrl(void)
{
    m_size = g_cfg_p->m_default_FontSize;
}

CFontCtrl::~CFontCtrl(void)
{
    FontItem_t *fontItem_p = nullptr;

    if (m_fonts.isEmpty()) {
        return;
    }

    for (int index = 0; index < m_fonts.count(); index++) {
        fontItem_p = m_fonts[index];

        delete (fontItem_p->font_p);
        delete (fontItem_p->pen_p);
        delete (fontItem_p);
    }

    m_fonts.clear();
}

/***********************************************************************************************************************
*   RegisterFont
***********************************************************************************************************************/
FontItem_t *CFontCtrl::RegisterFont(Q_COLORREF color, Q_COLORREF bg_color, FontMod_t fontMod)
{
    auto modifiedColor = color;

    /* First check if the font already exist */
    if (!m_fonts.isEmpty()) {
        for (int index = 0; index < m_fonts.count(); ++index) {
            if ((m_fonts[index]->originalColor == color) &&
                (m_fonts[index]->bg_color == bg_color) &&
                (m_fonts[index]->fontMod == fontMod)) {
                return m_fonts[index];
            }
        }
    }

    auto fontItem_p = new (FontItem_t);
    fontItem_p->originalColor = color;
    fontItem_p->bg_color = bg_color;

    if (fontMod & FONT_MOD_COLOR_MOD) {
        modifiedColor = static_cast<Q_COLORREF>(EnhanceColorContrast(fontItem_p));
    }

    QColor penColor(modifiedColor);

    fontItem_p->color = modifiedColor;

    QString fontDescr = g_cfg_p->m_fontRes;
    int weight = QFont::Normal;
    bool italic = false;
    if ((fontMod & FONT_MOD_BOLD) && (fontMod & FONT_MOD_ITALIC)) {
        fontDescr = g_cfg_p->m_fontRes_BoldItalic;
        weight = QFont::Bold;
        italic = true;
    } else if (fontMod & FONT_MOD_BOLD) {
        fontDescr = g_cfg_p->m_fontRes_Bold;
        weight = QFont::Bold;
    } else if (fontMod & FONT_MOD_ITALIC) {
        fontDescr = g_cfg_p->m_fontRes_Italic;
        italic = true;
    }

    fontItem_p->font_p = new QFont(fontDescr, m_size, weight, italic);
    fontItem_p->pen_p = new QPen(penColor);
    fontItem_p->fontMod = fontMod;

    m_fonts.append(fontItem_p);

    return fontItem_p;
}

/***********************************************************************************************************************
*   SetFont
***********************************************************************************************************************/
void CFontCtrl::SetFont(LS_Painter *painter_p, FontItem_t *font_p)
{
    if (painter_p->isCurrentFontItem(font_p)) {
        return; /* optimization */
    }
    painter_p->setFont(*font_p->font_p);
    painter_p->setCurrentFontItem(font_p);

    painter_p->setBackgroundMode(Qt::TransparentMode);
    painter_p->setPen(*font_p->pen_p); /* To achive the color */

    QFontMetrics metrics(*font_p->font_p);

    m_fontLetterWidth = metrics.horizontalAdvance('X');
    m_fontLetterHeight = metrics.height();

    if ((m_fontLetterHeight == 0) || (m_fontLetterWidth == 0)) {
        TRACEX_E("Failed to get correct measurement on font size")
    }
}

/***********************************************************************************************************************
*   ChangeFontSize
***********************************************************************************************************************/
void CFontCtrl::ChangeFontSize(int size)
{
    if (size == m_size) {
        return;
    }

    m_size = size;
    g_cfg_p->m_default_FontSize = m_size;

    TRACEX_I(QString("Font size change: %1").arg(size))
    UpdateAllFonts();
}

/***********************************************************************************************************************
*   UpdateAllFonts
***********************************************************************************************************************/
void CFontCtrl::UpdateAllFonts(void)
{
    if (m_fonts.isEmpty()) {
        return;
    }

    for (auto& font : m_fonts) {
        font->font_p->setPointSize(m_size);
    }

    QFontMetrics metrics(*m_fonts[0]->font_p);

    m_fontLetterWidth = metrics.horizontalAdvance('X');
    m_fontLetterHeight = metrics.height();
}

/***********************************************************************************************************************
*   UpdateAllFontsWith_fontMod
***********************************************************************************************************************/
void CFontCtrl::UpdateAllFontsWith_fontMod(FontMod_t fontMod)
{
    FontItem_t *fontItem_p;

    for (int index = 0; index < m_fonts.count(); ++index) {
        fontItem_p = m_fonts[index];
        if (fontItem_p->fontMod & fontMod) {
            if (fontMod & FONT_MOD_COLOR_MOD) {
                fontItem_p->color = static_cast<QRgb>(EnhanceColorContrast(fontItem_p));
                fontItem_p->pen_p->setColor(fontItem_p->color);
            }
        }
    }
}

/***********************************************************************************************************************
*   EnhanceColorContrast
***********************************************************************************************************************/
int CFontCtrl::EnhanceColorContrast(FontItem_t *fontItem_p)
{
    auto color = fontItem_p->originalColor;
    auto bgColor = fontItem_p->bg_color;
    auto green = qGreen(color);
    auto red = qRed(color);
    auto blue = qBlue(color);
    auto bgGreen = qGreen(bgColor);
    auto bgRed = qRed(bgColor);
    auto bgBlue = qBlue(bgColor);
    const int COLOR_DARKEN = g_cfg_p->m_log_FilterMatchColorModify;

    /* Check for dark background color... in such case let color become brighter instead of darker */
    if ((fontItem_p->bg_color != BACKGROUND_COLOR) && ((bgGreen + bgRed + bgBlue) < (green + red + blue))) {
        red += COLOR_DARKEN * 2;
        green += COLOR_DARKEN * 2;
        blue += COLOR_DARKEN * 2;

        red = red > 0xff ? 0xff : red;
        blue = blue > 0xff ? 0xff : blue;
        green = green > 0xff ? 0xff : green;

        return static_cast<int>((Q_RGB(red, green, blue)));
    }

    if ((blue >= green) && (blue >= red)) {
        blue = (green <= COLOR_DARKEN) ? blue - COLOR_DARKEN : blue;
        blue = (red <= COLOR_DARKEN) ? blue - COLOR_DARKEN : blue;
        blue = (blue < 0) ? 0 : blue;

        green = (green >= COLOR_DARKEN) ? green - COLOR_DARKEN : 0;
        red = (red >= COLOR_DARKEN) ? red - COLOR_DARKEN : 0;
    } else if ((red >= green) && (red >= blue)) {
        red = (green <= COLOR_DARKEN) ? red - COLOR_DARKEN : red;
        red = (blue <= COLOR_DARKEN) ? red - COLOR_DARKEN : red;
        red = (red < 0) ? 0 : red;

        green = (green >= COLOR_DARKEN) ? green - COLOR_DARKEN : 0;
        blue = (blue >= COLOR_DARKEN) ? blue - COLOR_DARKEN : 0;
    } else {
        green = (blue <= COLOR_DARKEN) ? green - COLOR_DARKEN : green;
        green = (blue <= COLOR_DARKEN) ? green - COLOR_DARKEN : green;
        green = (green < 0) ? 0 : green;

        red = (red >= COLOR_DARKEN) ? red - COLOR_DARKEN : 0;
        blue = (blue >= COLOR_DARKEN) ? blue - COLOR_DARKEN : 0;
    }

    return static_cast<int>((Q_RGB(red, green, blue)));
}

/***********************************************************************************************************************
*   GetFontColorTable
***********************************************************************************************************************/
void GetFontColorTable(ColorTableItem_t **colorTable_pp, int *colorTableItems_p)
{
    *colorTable_pp = g_cfg_p->m_font_ColorTable_p->GetTable();
    *colorTableItems_p = static_cast<int>(g_cfg_p->m_font_ColorTable_p->GetCurrentNumberOfColors());
}
