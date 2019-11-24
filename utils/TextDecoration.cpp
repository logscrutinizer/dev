/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CDebug.h"
#include "CMemPool.h"
#include "CRowCache.h"
#include "TextDecoration.h"
#include "mainwindow_cb_if.h"
#include "utils.h"

#include <hs/hs.h>

int debug_numOfRowsForHighLight = 0;
struct regExpEventHandlerData {
    int unused_SuppressAlignmentWarning; /* or use -wpadded */
    int matchCount;
    QList<TextRectElement_t *> *elementRefs_p;
    CTextRectElementFactory *elementFactory_p;
};

/***********************************************************************************************************************
*   create
***********************************************************************************************************************/
TextRectElement_t *CTextRectElementFactory::create(void)
{
    if (m_autoHighlight_Pool_p->isEmpty()) {
        return static_cast<TextRectElement_t *>(malloc(sizeof(TextRectElement_t)));
    } else {
        return m_autoHighlight_Pool_p->takeLast();
    }
}

/***********************************************************************************************************************
*   create
***********************************************************************************************************************/
TextRectElement_t *CFontModElementFactory::create(void)
{
    if (m_fontModification_Pool_p->isEmpty()) {
        return static_cast<TextRectElement_t *>(malloc(sizeof(FontModification_Element_t)));
    } else {
        return reinterpret_cast<TextRectElement_t *>(m_fontModification_Pool_p->takeLast());
    }
}

/***********************************************************************************************************************
*   eventHandler
***********************************************************************************************************************/
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    Q_UNUSED(id)
    Q_UNUSED(flags)

    auto data = reinterpret_cast<struct regExpEventHandlerData *>(ctx);

    /* MATCH!!! Create element, or merge */

    to = to > 0 ? to - 1 : 0;

    /* check overlap, if overlap merge elements */
    if (data->elementRefs_p->count() > 0) {
        for (auto& elem : *data->elementRefs_p) {
            if (((elem->endCol >= static_cast<int>(from)) && (elem->endCol < static_cast<int>(to))) ||
                (elem->startCol == static_cast<int>(from))) {
                /* merge elements */
                elem->endCol = static_cast<int>(to);
                return 0;
            }
        }
    }

    TextRectElement_t *element_p = data->elementFactory_p->create();
    element_p->startCol = static_cast<int>(from);
    element_p->endCol = static_cast<int>(to);

    data->elementRefs_p->append(element_p);
    ++data->matchCount;
    return 0;
}

/***********************************************************************************************************************
*   FindTextElements
***********************************************************************************************************************/
int FindTextElements(QList<TextRectElement_t *> *elementRefs_p, const char *text_p, const int textSize,
                     const char *textMatch_p, const int textMatchSize, bool caseSensitive, bool regExp,
                     CTextRectElementFactory *elementFactory_p)
{
    int matchCount = 0;

    Q_ASSERT(g_upperChar_LUT_init == true);

    if ((text_p != nullptr) && (textSize > 0)) {
        if (regExp) {
            struct regExpEventHandlerData data;
            data.elementFactory_p = elementFactory_p;
            data.elementRefs_p = elementRefs_p;
            data.matchCount = 0;

            hs_database_t *database;
            hs_compile_error_t *compile_err;
            if (hs_compile(textMatch_p,
                           HS_FLAG_DOTALL | HS_FLAG_SOM_LEFTMOST,
                           HS_MODE_BLOCK,
                           nullptr,
                           &database,
                           &compile_err) != HS_SUCCESS) {
                TRACEX_W(QString("RegExp failed %1").arg(compile_err->message))
                hs_free_compile_error(compile_err);
                return 0;
            }

            hs_scratch_t *scratch = nullptr;
            if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
                TRACEX_W(QString("ERROR: Unable to allocate scratch space"))
                hs_free_database(database);
                return 0;
            }

            if (hs_scan(database, text_p, static_cast<unsigned int>(textSize), 0, scratch, eventHandler,
                        &data) != HS_SUCCESS) {
                TRACEX_W(QString("ERROR: Unable to scan input buffer"))
                hs_free_scratch(scratch);
                hs_free_database(database);
                return 0;
            }

            /* Scanning is complete, any matches have been handled, so now we just * clean up and exit. */
            hs_free_scratch(scratch);
            hs_free_database(database);

            return data.matchCount;
        } else {
            /* Loop through the text against the match
             *  - originalStart, the entire row text start
             *  - whileLoopText(text_p), used to step over text, each step means doing a new match
             *  - forloop(currentText_p), used when for-looping the text and matchText,
             */
            const char *origStart_p = text_p;
            const uint8_t *endRef_p = reinterpret_cast<const uint8_t *>(text_p) +
                                      (textSize < DISPLAY_MAX_ROW_SIZE ? textSize : DISPLAY_MAX_ROW_SIZE);
            bool stop = false;
            const uint8_t *endMatch = reinterpret_cast<const uint8_t *>(textMatch_p) + textMatchSize - 1;

            while (reinterpret_cast<const uint8_t *>(text_p) < endRef_p &&
                   !stop &&
                   (textMatchSize <= (endRef_p - reinterpret_cast<const uint8_t *>(text_p)))) {
                const uint8_t *currentText_p = reinterpret_cast<const uint8_t *>(text_p);
                const uint8_t *currentMatch_p = reinterpret_cast<const uint8_t *>(textMatch_p);

                /* The for loop iterates while the textMatch is matching the text, at missmatch the for-loop breaks
                 * and goes to outer while loop */
                for ( ; ; ++currentText_p, ++currentMatch_p) {
                    /* Match letter */
                    if ((caseSensitive && (*currentMatch_p != *currentText_p)) ||
                        (g_upperChar_LUT[*currentMatch_p] != g_upperChar_LUT[*currentText_p])) {
                        ++text_p;  /* move forward only one to start the matching at one ahead only */
                        break; /* BREAK the loop */
                    }

                    /* Check for complete match */
                    if (currentMatch_p == endMatch) {
                        /* MATCH!!! Create element */
                        TextRectElement_t *element_p = elementFactory_p->create();
                        element_p->startCol = static_cast<int>(text_p - origStart_p);
                        element_p->endCol = static_cast<int>(element_p->startCol + textMatchSize - 1);
                        elementRefs_p->append(element_p);
                        ++matchCount;
                        text_p += textMatchSize;   /* move the text forward with the length of the filter */
                        break; /* BREAK the loop */
                    }

                    /* end of text string, stop the inner filter loop, no match */
                    if (currentText_p == endRef_p) {
                        stop = true; /* ABORT the while/search */
                        break; /* BREAK the loop */
                    }
                } /* for */
            } /* while */
        } /* if regExp */
    } /* if */

    return matchCount;
}

/***********************************************************************************************************************
*   CleanAutoHighlight
***********************************************************************************************************************/
void CAutoHighLight::CleanAutoHighlight(TIA_Cache_MemMap_t *cacheRow_p)
{
    cacheRow_p->autoHighligth.matchStamp = 0;

    /* Move all autoHighlight elements from the row to the pool */
    while (!cacheRow_p->autoHighligth.elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(cacheRow_p->autoHighligth.elementRefs.takeLast());
    }
}

/***********************************************************************************************************************
*   GetAutoHighlightRowInfo
***********************************************************************************************************************/
bool CAutoHighLight::GetAutoHighlightRowInfo(int row, AutoHightlight_RowInfo_t **rowInfo_pp)
{
    if (m_autoHighligthMatchStamp == 0) {
        *rowInfo_pp = nullptr;
        return false;
    }

    int cacheIndex = row % CACHE_MEM_MAP_SIZE;
    TIA_Cache_MemMap_t *cacheRow_p = m_rowCache_p->GetCacheRow(cacheIndex);

    if (cacheRow_p->autoHighligth.matchStamp != m_autoHighligthMatchStamp) {
        debug_numOfRowsForHighLight++;
        UpdateAutoHighlightList(row);
    }

    if (cacheRow_p->autoHighligth.elementRefs.isEmpty()) {
        *rowInfo_pp = nullptr;
        return false;
    }

    *rowInfo_pp = &cacheRow_p->autoHighligth;

    return true;
}

/***********************************************************************************************************************
*   UpdateAutoHighlightList
***********************************************************************************************************************/
void CAutoHighLight::UpdateAutoHighlightList(int row)
{
    if (m_autoHighligthMatchStamp == 0) {
        return;
    }

    /* Make sure that the correct text line is in cache */
    char *text_p;
    int textLength;
    m_rowCache_p->Get(row, &text_p, &textLength);

    int cacheIndex = row % CACHE_MEM_MAP_SIZE;
    TIA_Cache_MemMap_t *cacheRow_p = m_rowCache_p->GetCacheRow(cacheIndex);

    if (cacheRow_p->autoHighligth.matchStamp == m_autoHighligthMatchStamp) {
        return;
    }

    CleanAutoHighlight(cacheRow_p);

    CTextRectElementFactory factory(&m_autoHighlight_Pool);
    FindTextElements(&cacheRow_p->autoHighligth.elementRefs, text_p, textLength, m_autoHighLightMatch,
                     m_autoHighLightMatchLength, m_autoHighLight_CS, m_autoHighLight_RegExp, &factory);

    cacheRow_p->autoHighligth.pixelStamp = 0;
    cacheRow_p->autoHighligth.matchStamp = m_autoHighligthMatchStamp;
}

/***********************************************************************************************************************
*   SetAutoHighlight
***********************************************************************************************************************/
void CAutoHighLight::SetAutoHighlight(uint64_t matchStamp, const char *autoHighlight_p, bool caseSensitive, bool regExp)
{
    if (!m_autoHighLight_AutomaticUpdate) {
#ifdef _DEBUG
        TRACEX_D("SetAutoHighlight m_autoHighLight_AutomaticUpdate  -- OFF --")
#endif
        return;
    }

    m_autoHighLight_CS = caseSensitive;
    m_autoHighLight_RegExp = regExp;

    if ((matchStamp == 0) || (autoHighlight_p == nullptr) || (strlen(autoHighlight_p) > MAX_AUTOHIGHLIGHT_LENGTH - 2)) {
        m_autoHighligthMatchStamp = 0;
        m_autoHighLightEnabled = false;
        m_autoHighLight_AutomaticUpdate = true;

#ifdef _DEBUG
        if (autoHighlight_p != nullptr) {
            TRACEX_D("SetAutoHighlight    SetAutoHighlight  No match setup, no update, length:%d",
                     static_cast<int>(strlen(autoHighlight_p)))
        } else {
            TRACEX_D("SetAutoHighlight    SetAutoHighlight  No match setup, no update")
        }
#endif
        return;
    }

#ifdef _DEBUG
    TRACEX_D("SetAutoHighlight OK")
#endif

    m_autoHighligthMatchStamp = matchStamp;
    m_autoHighLightEnabled = true;

    strcpy_s(m_autoHighLightMatch, CFG_TEMP_STRING_MAX_SIZE, autoHighlight_p);
    m_autoHighLightMatchLength = static_cast<int>(strlen(m_autoHighLightMatch));
}

/***********************************************************************************************************************
*   SearchAndUpdateNextAutoHighlight
***********************************************************************************************************************/
bool CAutoHighLight::SearchAndUpdateNextAutoHighlight(const char *matchString_p, CSelection *startSelection_p,
                                                      bool forward, CSelection *nextSelection_p, bool caseSensitive,
                                                      bool regExp)
{
    bool quickSearchOK = false;
    char *text_p;
    int size;
    QList<TextRectElement_t *> elementRefs;

    SetAutoHighlight(static_cast<uint64_t>(MW_GetTick()), matchString_p, caseSensitive, regExp);
    m_rowCache_p->Get(startSelection_p->row, &text_p, &size);

    CTextRectElementFactory factory(&m_autoHighlight_Pool);
    if (FindTextElements(&elementRefs, text_p, size, m_autoHighLightMatch, m_autoHighLightMatchLength,
                         caseSensitive, regExp, &factory)) {
        TextRectElement_t *element_p = nullptr;
        QList<TextRectElement_t *>::iterator iter;
        if (forward) {
            if (startSelection_p->startCol == -1) {
                quickSearchOK = true;
                element_p = elementRefs.first();
            }
            for (iter = elementRefs.begin(); iter != elementRefs.end() && !quickSearchOK; ++iter) {
                element_p = (*iter);
                if (element_p->startCol >= startSelection_p->startCol) {
                    quickSearchOK = true;
                }
            }
        } else {
            if (startSelection_p->endCol == -1) {
                quickSearchOK = true;
                element_p = elementRefs.last();
            }
            for (iter = elementRefs.end() - 1; iter >= elementRefs.begin() && !quickSearchOK; --iter) {
                element_p = (*iter);
                if (element_p->endCol <= startSelection_p->endCol) {
                    quickSearchOK = true;
                }
            }
        }

        if (quickSearchOK) {
            nextSelection_p->startCol = element_p->startCol;
            nextSelection_p->endCol = element_p->endCol;
            nextSelection_p->row = startSelection_p->row;
        }
    }

    /* Move all autoHighlight elements from the row to the pool */
    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    return quickSearchOK;
}

/***********************************************************************************************************************
*   AutoHighlightTest
***********************************************************************************************************************/
void CAutoHighLight::AutoHighlightTest(void)
{
    QList<TextRectElement_t *> elementRefs;
    TextRectElement_t *element_p;

    strcpy_s(m_autoHighLightMatch, CFG_TEMP_STRING_MAX_SIZE, "Dummy");
    m_autoHighLightMatchLength = static_cast<int>(strlen(m_autoHighLightMatch));

    char testString[CFG_TEMP_STRING_MAX_SIZE];

    /*------------------------------------------------------------------------------------------------------------------
     * */

    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "Dummy dummy Bummy Dummy");

    CTextRectElementFactory factory(&m_autoHighlight_Pool);
    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength,
                         true /*CS*/, false /*regExp*/, &factory) != 2) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.first();
    if ((element_p->startCol != 0) || (element_p->endCol != 4)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(1);
    if ((element_p->startCol != 18) || (element_p->endCol != 22)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    /*------------------------------------------------------------------------------------------------------------------
     * */

    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "DummyDummyDummyDummy");

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, true /*CS*/, false /*regExp*/,
                         &factory) != 4) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(0);
    if ((element_p->startCol != 0) || (element_p->endCol != 4)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(1);
    if ((element_p->startCol != 5) || (element_p->endCol != 9)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(2);
    if ((element_p->startCol != 10) || (element_p->endCol != 14)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(3);
    if ((element_p->startCol != 15) || (element_p->endCol != 19)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    /*------------------------------------------------------------------------------------------------------------------
     * */

    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "DummDummDummDummy");

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, true /*CS*/, false /*regExp*/,
                         &factory) != 1) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    /*------------------------------------------------------------------------------------------------------------------
     * */

    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "Dummmy DummY DDDDDDDummy");

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, m_autoHighLight_CS, m_autoHighLight_RegExp,
                         &factory) != 1) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    /*
     * ------------------------------------------------------------------------------------------------------------------
     * RegExp */
    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "Dummy Bummy Pummy");

    strcpy_s(m_autoHighLightMatch, CFG_TEMP_STRING_MAX_SIZE, "[DBP]u.my");
    m_autoHighLightMatchLength = static_cast<int>(strlen(m_autoHighLightMatch));

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, true /*CS*/, true /*regExp*/,
                         &factory) != 3) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(0);
    if ((element_p->startCol != 0) || (element_p->endCol != 4)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(1);
    if ((element_p->startCol != 6) || (element_p->endCol != 10)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(2);
    if ((element_p->startCol != 12) || (element_p->endCol != 16)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    /*
     * ------------------------------------------------------------------------------------------------------------------
     * RegExp - overlap */
    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "Dummy Bummy Pummy");

    /* The regExp will match, (1) Dummy, (2) Dummy Bummy, (3) Dummy Bummy Pummy */
    strcpy_s(m_autoHighLightMatch, CFG_TEMP_STRING_MAX_SIZE, ".*my");
    m_autoHighLightMatchLength = static_cast<int>(strlen(m_autoHighLightMatch));

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, true /*CS*/, true /*regExp*/,
                         &factory) != 1) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(0);
    if ((element_p->startCol != 0) || (element_p->endCol != 16)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }

    /*
     * ------------------------------------------------------------------------------------------------------------------
     * Case in-sensitive */
    strcpy_s(testString, CFG_TEMP_STRING_MAX_SIZE, "DUM12 DUM12 Dummy");

    strcpy_s(m_autoHighLightMatch, CFG_TEMP_STRING_MAX_SIZE, "dUm12");
    m_autoHighLightMatchLength = static_cast<int>(strlen(m_autoHighLightMatch));

    if (FindTextElements(&elementRefs, testString, static_cast<int>(strlen(testString)),
                         m_autoHighLightMatch, m_autoHighLightMatchLength, false /*CS*/, false /*regExp*/,
                         &factory) != 2) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(0);
    if ((element_p->startCol != 0) || (element_p->endCol != 4)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    element_p = elementRefs.at(1);
    if ((element_p->startCol != 6) || (element_p->endCol != 10)) {
        TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    AutoHighlightTest failed")
    }

    while (!elementRefs.isEmpty()) {
        m_autoHighlight_Pool.append(elementRefs.takeLast());
    }
}

/***********************************************************************************************************************
*   GetFontModRowInfo
***********************************************************************************************************************/
bool CFontModification::GetFontModRowInfo(const int row,
                                          CFilterItem *filterItem_p,
                                          FontModification_RowInfo_t **rowInfo_pp)
{
    *rowInfo_pp = nullptr;

    if (filterItem_p == nullptr) {
        TRACEX_W("GetFontModRowInfo  filterItem_p is nullptr")
        return false;
    }

    int cacheIndex = row % CACHE_MEM_MAP_SIZE;
    TIA_Cache_MemMap_t *cacheRow_p = m_rowCache_p->GetCacheRow(cacheIndex);

    if (cacheRow_p->row != row) {
        /* Make sure to update the row first */
        char *text_p;
        int textLength;
        m_rowCache_p->Get(row, &text_p, &textLength);
    }

    cacheRow_p = m_rowCache_p->GetCacheRow(cacheIndex);
    if (cacheRow_p->row != row) {
        TRACEX_W("GetFontModRowInfo  row not found")
        return false;
    }

    /* TEMP
     * Move all autoHighlight elements from the row to the pool */
    QList<FontModification_Element_t *> *elementRefs_p = &cacheRow_p->fontModification.elementRefs;
    while (!elementRefs_p->isEmpty()) {
        m_fontModification_Pool.append(elementRefs_p->takeLast());
    }

    CFontModElementFactory factory(&m_fontModification_Pool);
    FindTextElements(
        reinterpret_cast<QList<TextRectElement_t *> *>(&cacheRow_p->fontModification.elementRefs),
        static_cast<char *>(cacheRow_p->poolItem_p->GetDataRef()),
        cacheRow_p->size,
        filterItem_p->m_start_p,
        filterItem_p->m_size,
        filterItem_p->m_caseSensitive,
        filterItem_p->m_regexpr,
        &factory);

    if (cacheRow_p->fontModification.elementRefs.isEmpty()) {
        *rowInfo_pp = nullptr;
        return false;
    }

    *rowInfo_pp = &cacheRow_p->fontModification;

    return true;
}
