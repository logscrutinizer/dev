/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#ifdef QT_TODO
 #include "LogScrutinizerView.h"
 #include "CPlotPane.h"
 #include "ViewTree.h"
 #include "CProgressCtrl.h"
 #include "CProgressDlg.h"
 #include "CPlotCtrl.h"
 #include "CErrorMsgDlg.h"
#endif

#include "CDebug.h"
#include "CMemPool.h"
#include "CRowCache.h"
#include "filemapping.h"
#include "CSelection.h"

#include <QFileInfo>
#include <QMessageBox>

#define MAX_AUTOHIGHLIGHT_LENGTH    512

extern int debug_numOfRowsForHighLight;

/***********************************************************************************************************************
*   CTextRectElementFactory
***********************************************************************************************************************/
class CTextRectElementFactory
{
public:
    explicit CTextRectElementFactory(QList<TextRectElement_t *> *autoHighlight_Pool_p) : m_autoHighlight_Pool_p(
            autoHighlight_Pool_p) {}

    /****/
    virtual TextRectElement_t *create()
    {
        if (m_autoHighlight_Pool_p->isEmpty()) {
            return static_cast<TextRectElement_t *>(malloc(sizeof(TextRectElement_t)));
        } else {
            return m_autoHighlight_Pool_p->takeLast();
        }
    }

private:
    QList<TextRectElement_t *> *m_autoHighlight_Pool_p;
};

/***********************************************************************************************************************
*   CFontModElementFactory
***********************************************************************************************************************/
class CFontModElementFactory : public CTextRectElementFactory
{
public:
    explicit CFontModElementFactory(QList<FontModification_Element_t *> *fontModification_Pool_p) :
        CTextRectElementFactory(nullptr), m_fontModification_Pool_p(fontModification_Pool_p) {}

    /****/
    virtual TextRectElement_t *create() override
    {
        if (m_fontModification_Pool_p->isEmpty()) {
            return static_cast<TextRectElement_t *>(malloc(sizeof(FontModification_Element_t)));
        } else {
            return reinterpret_cast<TextRectElement_t *>(m_fontModification_Pool_p->takeLast());
        }
    }

private:
    QList<FontModification_Element_t *> *m_fontModification_Pool_p;
};

/* FintExtElements is a utility function to search for text patterns in a string. This function is used from
 * AutoHighLight and FontModification. */
int FindTextElements(QList<TextRectElement_t *> *elementRefs_p, const char *text_p, const int textSize,
                     const char *textMatch_p, const int textMatchSize, bool caseSensitive, bool regExp,
                     CTextRectElementFactory *elementFactory_p);

/***********************************************************************************************************************
*   CAutoHighLight
***********************************************************************************************************************/
class CAutoHighLight
{
public:
    explicit CAutoHighLight(CRowCache *rowCache_p) : m_rowCache_p(rowCache_p)
    {
        m_autoHighligthMatchStamp = 0;
        m_autoHighLightEnabled = false;
        m_autoHighLight_AutomaticUpdate = true;
        m_autoHighLight_CS = true;
        m_autoHighLight_RegExp = false;

        SetAutoHighlight(0, nullptr);

        /**< Add some autoHighLight elements to the pool */
        for (int index = 0; index < 100; ++index) {
            TextRectElement_t *element_p = static_cast<TextRectElement_t *>(malloc(sizeof(TextRectElement_t)));
            m_autoHighlight_Pool.append(element_p);
        }
    }

    ~CAutoHighLight() {
        while (!m_autoHighlight_Pool.isEmpty()) {
            free(m_autoHighlight_Pool.takeFirst());
        }
    }

    /***********************************************************************************************************************
    *   Clean
    ***********************************************************************************************************************/
    void Clean() {
        m_autoHighligthMatchStamp = 0;
    }

    CAutoHighLight(CAutoHighLight& copy) = delete; /* remove the copy constructor */

    CAutoHighLight& operator=(CAutoHighLight& rhs) = delete; /* remove the assignment operator */

    void CleanAutoHighlight(TIA_Cache_MemMap_t *cacheRow_p);

    TextRectElement_t *GetFreeAutoHighlightElement(void);
    bool GetAutoHighlightRowInfo(int row, AutoHightlight_RowInfo_t **rowInfo_pp);
    void UpdateAutoHighlightList(int row);

    /* caseSensitive and regExp is not used when autoHightLight is configured from the log editor and a selection
     * has been made. However from e.g. a search both regExp and caseSens is set. */
    void SetAutoHighlight(uint64_t matchStamp = 0, const char *autoHighlight_p = nullptr, bool caseSensitive = true,
                          bool regExp = false);
    bool SearchAndUpdateNextAutoHighlight(const char *matchString_p, CSelection *startSelection_p, bool forward,
                                          CSelection *nextSelection_p, bool caseSensitive, bool regExp = false);
    bool GetFontModRowInfo(int row, CFilterItem *filterItem_p, FontModification_RowInfo_t **rowInfo_pp);

    TextRectElement_t *GetFreeFontModificationElement(void);
    void EnableAutoHighlightAutomaticUpdates(bool enable) {m_autoHighLight_AutomaticUpdate = enable;}
    void AutoHighlightTest(void);

    uint64_t m_autoHighligthMatchStamp;
    char m_autoHighLightMatch[CFG_TEMP_STRING_MAX_SIZE];
    int m_autoHighLightMatchLength;
    bool m_autoHighLightEnabled;
    bool m_autoHighLight_AutomaticUpdate; /**< Set when the autoHighlight shall  be automatically updated, typically
                                           * disabled during continous search */
    bool m_autoHighLight_CS; /**< Case Sensitive by default... but search may override */
    bool m_autoHighLight_RegExp;

private:
    CRowCache *m_rowCache_p; /**< Access to the cache memmap */
    QList<TextRectElement_t *> m_autoHighlight_Pool;
};

/***********************************************************************************************************************
*   CFontModification
***********************************************************************************************************************/
class CFontModification
{
public:
    CFontModification(CRowCache *rowCache_p) : m_rowCache_p(rowCache_p) {
        /* Add some autoHighLight elements to the pool */
        for (int index = 0; index < 100; ++index) {
            auto element_p = static_cast<FontModification_Element_t *>(malloc(sizeof(FontModification_Element_t)));
            m_fontModification_Pool.append(element_p);
        }
    }
    ~CFontModification() {
        while (!m_fontModification_Pool.isEmpty()) {
            free(m_fontModification_Pool.takeFirst());
        }
    }

    TextRectElement_t *GetFreeFontModificationElement(void);
    bool GetFontModRowInfo(const int row, CFilterItem *filterItem_p, FontModification_RowInfo_t **rowInfo_pp);

private:
    CRowCache *m_rowCache_p; /**< Access to the cache memmap */
    QList<FontModification_Element_t *> m_fontModification_Pool;
};
