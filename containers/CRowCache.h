/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <time.h>
#include "globals.h"
#include "CXMLBase.h"
#include "CMemPool.h"
#include "CFilter.h"
#include "CCfgItem.h"

#define CACHE_MEM_MAP_SIZE 1024
#define TIA_CACHE_MEMMAP_PROPERTY_NONE 0
#define TIA_CACHE_MEMMAP_PROPERTY_DECODED 1  /* To keep track if a row has been decoded or not */

typedef struct {
    QRect rectPixel;
    int startCol;
    int endCol;
}TextRectElement_t;

typedef struct {
    uint64_t matchStamp; /* Used to verify if the autoHighlight contains latest information about the match string */
    uint64_t pixelStamp; /* bit 0-31 start_X, 32-54, bit 55-63  row_start_top, high 8-bit  fontSize, low 32-bit
                          *  (Decorated by View) */
    QList<TextRectElement_t *> elementRefs;
} AutoHightlight_RowInfo_t;

typedef struct {
    TextRectElement_t textRectElement;
    FontMod_t fontModification;
    FontItem_t *font_p;
}FontModification_Element_t;

typedef struct {
    QList<FontModification_Element_t *> elementRefs;
    char unused_padding[4]; /* Suppress padding warning */
    int pixelStamp;      /* high 8-bit  fontSize, low 24-bit start_X (Decorated by View) */
} FontModification_RowInfo_t;

typedef struct {
    AutoHightlight_RowInfo_t autoHighligth;
    FontModification_RowInfo_t fontModification;
    CMemPoolItem *poolItem_p;
    int size;
    int tabbedSize; /* Same as size, however compensated for the fact that a TAB shall be replaced with 4 spaces */
    int row;
    FIR_t FIR; /*  Copy of the FIRA table, to have less lookup in the memory file (causing cache misses) */
    int properties; /* TIA_CACHE_MEMMAP_PROPERTY_DECODED */
}TIA_Cache_MemMap_t;

/***********************************************************************************************************************
*   CRowCache
***********************************************************************************************************************/
class CRowCache
{
public:
    QList<CCfgItem_Decoder *> m_decoders;

    explicit CRowCache(QFile *qFile_p, TIA_t *TIA_p, FIRA_t *FIRA_p, CFilterItem **filterItem_LUT, CMemPool& memPool) :
        m_qFile_p(qFile_p), m_TIA_p(TIA_p), m_FIRA_p(FIRA_p), m_filterItem_LUT_pp(filterItem_LUT), m_memPool(memPool)
    {
        for (int index = 0; index < CACHE_MEM_MAP_SIZE; ++index) {
            m_cache_memMap[index].size = 0;
            m_cache_memMap[index].row = -1;
            m_cache_memMap[index].poolItem_p = m_memPool.AllocMem(FILECTRL_ROW_SIZE_ESTIMATE);

            if (m_cache_memMap[index].poolItem_p == nullptr) {
                TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    m_cache_memMap[index].poolItem_p nullptr")
            }
        }
    }

    explicit CRowCache(CMemPool& memPool) :
        m_qFile_p(nullptr), m_TIA_p(nullptr), m_FIRA_p(nullptr), m_filterItem_LUT_pp(nullptr), m_memPool(memPool)
    {
        for (int index = 0; index < CACHE_MEM_MAP_SIZE; ++index) {
            m_cache_memMap[index].size = 0;
            m_cache_memMap[index].row = -1;
            m_cache_memMap[index].poolItem_p = m_memPool.AllocMem(FILECTRL_ROW_SIZE_ESTIMATE);

            if (m_cache_memMap[index].poolItem_p == nullptr) {
                TRACEX_E("CLogScrutinizerDoc::CLogScrutinizerDoc    m_cache_memMap[index].poolItem_p nullptr")
            }
        }
    }

    CRowCache() = delete;

    ~CRowCache()
    {
        for (int index = 0; index < CACHE_MEM_MAP_SIZE; ++index) {
            if (m_cache_memMap[index].poolItem_p != nullptr) {
                m_memPool.ReturnMem(&m_cache_memMap[index].poolItem_p);
                m_cache_memMap[index].poolItem_p = nullptr;
            }

            while (!m_cache_memMap[index].autoHighligth.elementRefs.isEmpty()) {
                free(m_cache_memMap[index].autoHighligth.elementRefs.takeFirst());
            }

            while (!m_cache_memMap[index].fontModification.elementRefs.isEmpty()) {
                free(m_cache_memMap[index].fontModification.elementRefs.takeFirst());
            }
        }
    }

    /****/
    void Update(QFile *qFile_p, TIA_t *TIA_p, FIRA_t *FIRA_p, CFilterItem **filterItem_LUT, CMemPool& memPool)
    {
        Clean();
        m_qFile_p = qFile_p;
        m_TIA_p = TIA_p;
        m_FIRA_p = FIRA_p;
        m_filterItem_LUT_pp = filterItem_LUT;
        m_memPool = memPool;
    }

    void Clean(void);  /* Clean clears out the existing strings in the cache, however the connection to the document and
                        * the rows remains. To really reset the cache use Reset() */

    /****/
    void Reset(void)
    {
        /* The reset functio will first do a clean and then reset the references to the the log */
        Clean();
        m_qFile_p = nullptr;
        m_TIA_p = nullptr;
        m_FIRA_p = nullptr;
        m_filterItem_LUT_pp = nullptr;
    }

    bool rawFromFile(const int64_t fileIndex, const int64_t size, char *dataRef_p);
    void Get(const int rowIndex, char **text_p, int *size_p, int *properties_p = nullptr);
    void GetAtCacheIndex(int cacheIndex, char **text_p, int *size_p, int *properties_p);
    void UpdateAtCacheIndex(int cacheIndex, char *text_p, int size, int properties);
    void CleanAutoHighlight(TIA_Cache_MemMap_t *cacheRow_p);
    void Decode(int cacheIndex);
    void RemoveBadChars(int cacheIndex);

    /****/
    TIA_Cache_MemMap_t *GetCacheRow(int cacheIndex)
    {
        return &m_cache_memMap[cacheIndex];
    }

    /****/
    void AddPropertyAtCacheIndex(int cacheIndex, int properties)
    {
        /* BIT OR in the properties value */
        m_cache_memMap[cacheIndex].properties |= properties;
    }

    CFilterItem *GetFilterRef(const int rowIndex);
    int GetFilterIndex(const int rowIndex);
    void GetTextItemLength(const int rowIndex, int *size_p);

private:
    QFile *m_qFile_p;
    TIA_Cache_MemMap_t m_cache_memMap[CACHE_MEM_MAP_SIZE];
    TIA_t *m_TIA_p;
    FIRA_t *m_FIRA_p;
    CFilterItem **m_filterItem_LUT_pp;
    CMemPool& m_memPool;
};
