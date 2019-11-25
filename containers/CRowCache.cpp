/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CRowCache.h"
#include "CConfig.h"
#include "CDebug.h"
#include "limits.h"
#include "CMemPool.h"

#include <QDebug>

static char g_largeTempString[CACHE_CMEM_POOL_SIZE_MAX];  /* this string is used as temporary storage for strings being
                                                           * decoded */
static char errorText[] = "Error";

/***********************************************************************************************************************
*   GetFilterRef
***********************************************************************************************************************/
CFilterItem *CRowCache::GetFilterRef(const int rowIndex)
{
    int cacheIndex = rowIndex % CACHE_MEM_MAP_SIZE;

    if (CSZ_DB_PendingUpdate || (m_TIA_p->rows == 0)) {
        return nullptr;
    }

    if (m_cache_memMap[cacheIndex].row == rowIndex) {
        return m_filterItem_LUT_pp[m_cache_memMap[cacheIndex].FIR.LUT_index];
    } else {
        return m_filterItem_LUT_pp[m_FIRA_p->FIR_Array_p[rowIndex].LUT_index];
    }
}

/***********************************************************************************************************************
*   GetFilterIndex
***********************************************************************************************************************/
int CRowCache::GetFilterIndex(const int rowIndex)
{
    int cacheIndex = rowIndex % CACHE_MEM_MAP_SIZE;

    if (m_cache_memMap[cacheIndex].row == rowIndex) {
        return m_cache_memMap[cacheIndex].FIR.index;
    } else {
        return m_FIRA_p->FIR_Array_p[rowIndex].index;
    }
}

/***********************************************************************************************************************
*   rawFromFile
***********************************************************************************************************************/
bool CRowCache::rawFromFile(const int64_t fileIndex, const int64_t size, char *dataRef_p)
{
    if (!m_qFile_p->seek(fileIndex)) {
        return false;
    }

    int64_t readBytes = m_qFile_p->read(dataRef_p, size);

    if (readBytes != size) {
        return false;
    }

    return true;
}

/***********************************************************************************************************************
*   Get
***********************************************************************************************************************/
void CRowCache::Get(const int rowIndex, char **text_p, int *size_p, int *properties_p)
{
    int cacheIndex = rowIndex % CACHE_MEM_MAP_SIZE;

    *size_p = 5;
    *text_p = errorText;

    if (CSZ_DB_PendingUpdate || (m_TIA_p == nullptr) || (m_TIA_p->rows == 0)) {
        return;
    }

    if (m_cache_memMap[cacheIndex].row == rowIndex) {
        *text_p = reinterpret_cast<char *>(m_cache_memMap[cacheIndex].poolItem_p->GetDataRef());
        *size_p = m_cache_memMap[cacheIndex].size;

        if (properties_p != nullptr) {
            *properties_p = m_cache_memMap[cacheIndex].properties;
        }

        return;
    } else {
        /* In-case the row is really large we limit it here... */
        const auto rowSize = m_TIA_p->textItemArray_p[rowIndex].size > DISPLAY_MAX_ROW_SIZE ?
                             DISPLAY_MAX_ROW_SIZE : m_TIA_p->textItemArray_p[rowIndex].size;
        extern void CLogScrutinizerDoc_CleanAutoHighlight(TIA_Cache_MemMap_t * cacheRow_p);
        CLogScrutinizerDoc_CleanAutoHighlight(&m_cache_memMap[cacheIndex]);

        m_cache_memMap[cacheIndex].FIR.index = 0;  /* defensive... error handling */
        m_cache_memMap[cacheIndex].FIR.LUT_index = 0;

        if (nullptr == m_cache_memMap[cacheIndex].poolItem_p) {
            return;
        }

        /* Return memory if usage of larger than CACHE_CMEM_POOL_SIZE_SMALLEST, or if the current is to small to carry
         * the
         * line filling it */
        if ((m_cache_memMap[cacheIndex].poolItem_p->GetDataSize() > CACHE_CMEM_POOL_SIZE_SMALLEST) ||
            (rowSize >= m_cache_memMap[cacheIndex].poolItem_p->GetDataSize())) {
            m_memPool.ReturnMem(&m_cache_memMap[cacheIndex].poolItem_p);
            m_cache_memMap[cacheIndex].poolItem_p = m_memPool.AllocMem(rowSize);
        }

        if (nullptr == m_cache_memMap[cacheIndex].poolItem_p) {
            TRACEX_E(QString("Failed to allocate memory to string of size %1").arg(rowSize))
            return;
        }

        int32_t to_readBytes = rowSize;

        if (to_readBytes > (m_cache_memMap[cacheIndex].poolItem_p->GetDataSize() - 1)) {
            to_readBytes = m_cache_memMap[cacheIndex].poolItem_p->GetDataSize() - 1;
        }

        auto *dataRef_p = reinterpret_cast<char *>(m_cache_memMap[cacheIndex].poolItem_p->GetDataRef());

        /*
         *  if (!rawFromFile(m_TIA_p->textItemArray_p[rowIndex].fileIndex, to_readBytes, dataRef_p)) {
         *    TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read more data for log file (seek)", m_qFile_p);
         *    return;
         *  } */
        if (!m_qFile_p->seek(m_TIA_p->textItemArray_p[rowIndex].fileIndex)) {
            /*          TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read more data for log file (seek)", m_qFile_p); */
            return;
        }

        int64_t readBytes = m_qFile_p->read(dataRef_p, to_readBytes);

        if (readBytes != to_readBytes) {
/*            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read more data for log file (read)", m_qFile_p); */
            return;
        }

        m_cache_memMap[cacheIndex].properties = 0;
        m_cache_memMap[cacheIndex].FIR = m_FIRA_p->FIR_Array_p[rowIndex];   /* Copy the FIR content as well to cache */
        m_cache_memMap[cacheIndex].size = static_cast<int>(readBytes);
        m_cache_memMap[cacheIndex].row = rowIndex;
        dataRef_p[readBytes] = 0;  /* 0 terminated */

        RemoveBadChars(cacheIndex);
        Decode(cacheIndex);

        *size_p = m_cache_memMap[cacheIndex].size;
        *text_p = reinterpret_cast<char *>(m_cache_memMap[cacheIndex].poolItem_p->GetDataRef());

        m_cache_memMap[cacheIndex].tabbedSize = -1; /*CalculateTextItemTabbedLength(*text_p, *size_p); */

        if (properties_p != nullptr) {
            *properties_p = m_cache_memMap[cacheIndex].properties;
        }
    }
}

/***********************************************************************************************************************
*   RemoveBadChars
***********************************************************************************************************************/
void CRowCache::RemoveBadChars(int cacheIndex)
{
    int size;
    char *string_p;
    int properties;

    GetAtCacheIndex(cacheIndex, &string_p, &size, &properties);
    for (int i = 0; i < size; ++i) {
        if (*string_p < 0x08) {
            *string_p = '?';
        }
        string_p++;
    }
}

/***********************************************************************************************************************
*   Decode
***********************************************************************************************************************/
void CRowCache::Decode(int cacheIndex)
{
    if (m_decoders.isEmpty()) {
        return;
    }

    QList<CCfgItem_Decoder *>::iterator iter;

    for (auto& decoder_p : m_decoders) {
        int size;
        char *string_p;
        int properties;

        GetAtCacheIndex(cacheIndex, &string_p, &size, &properties);

        int newSize = size;

        memcpy(g_largeTempString, string_p, static_cast<size_t>(size));
        g_largeTempString[size] = 0;

        if (decoder_p->m_decoder_ref_p->Decode(g_largeTempString, &newSize, CACHE_CMEM_POOL_SIZE_MAX)) {
            /*<<< ---- Make the decoding, returns true if text was modified */
            g_largeTempString[newSize] = 0;     /* secure string termination */
            newSize++;                          /* secure space for string termination */

            UpdateAtCacheIndex(cacheIndex, g_largeTempString, static_cast<int>(newSize),
                               properties | TIA_CACHE_MEMMAP_PROPERTY_DECODED);
            return; /* Exit when first decoder has made changes */
        }
    }
}

/***********************************************************************************************************************
*   GetAtCacheIndex
***********************************************************************************************************************/
void CRowCache::GetAtCacheIndex(int cacheIndex, char **text_p, int *size_p, int *properties_p)
{
    TIA_Cache_MemMap_t *cache_p = &m_cache_memMap[cacheIndex];

    if (cache_p->poolItem_p != nullptr) {
        *text_p = reinterpret_cast<char *>(cache_p->poolItem_p->GetDataRef());
        *size_p = cache_p->size;
        *properties_p = cache_p->properties;
        return;
    }

    *text_p = nullptr;
    *size_p = 0;
    *properties_p = 0;
}

/***********************************************************************************************************************
*   UpdateAtCacheIndex
***********************************************************************************************************************/
void CRowCache::UpdateAtCacheIndex(int cacheIndex, char *text_p, int size, int properties)
{
    TIA_Cache_MemMap_t *cache_p = &m_cache_memMap[cacheIndex];

    if (cache_p->size != size) {
        if ((cache_p->poolItem_p->GetDataSize() > CACHE_CMEM_POOL_SIZE_SMALLEST) || (size >= cache_p->size)) {
            /* Replace the memory in the row cache with the decoded string size */
            m_memPool.ReturnMem(&cache_p->poolItem_p);
            cache_p->poolItem_p = m_memPool.AllocMem(size);
        }

        /* Replace the string in cache */
        cache_p->size = size - 1; /* -1 to avoid adding the zero termination to the string size */
        cache_p->properties = properties;
        memcpy(cache_p->poolItem_p->GetDataRef(), text_p, static_cast<size_t>(size));
    }
}

/***********************************************************************************************************************
*   GetTextItemLength
***********************************************************************************************************************/
void CRowCache::GetTextItemLength(const int rowIndex, int *size_p)
{
    int cacheIndex = rowIndex % CACHE_MEM_MAP_SIZE;

    *size_p = 0;

    if (m_cache_memMap[cacheIndex].row == rowIndex) {
        *size_p = m_cache_memMap[cacheIndex].size;
        return;
    }

    char *text_p;
    Get(rowIndex, &text_p, size_p);
}

/***********************************************************************************************************************
*   Clean
***********************************************************************************************************************/
void CRowCache::Clean(void)
{
    for (int index = 0; index < CACHE_MEM_MAP_SIZE; ++index) {
        /* Return memory if usage of larger than CACHE_CMEM_POOL_SIZE_SMALLEST */
        if (m_cache_memMap[index].poolItem_p->GetDataSize() > CACHE_CMEM_POOL_SIZE_SMALLEST) {
            m_memPool.ReturnMem(&m_cache_memMap[index].poolItem_p);
            m_cache_memMap[index].poolItem_p = m_memPool.AllocMem(CACHE_CMEM_POOL_SIZE_SMALLEST);
        }

        if (m_cache_memMap[index].row != -1) {
            m_cache_memMap[index].row = -1;
            m_cache_memMap[index].poolItem_p->MemSet();

            m_cache_memMap[index].FIR.index = 0;
            m_cache_memMap[index].FIR.LUT_index = 0;
            m_cache_memMap[index].autoHighligth.matchStamp = 0;
            m_cache_memMap[index].autoHighligth.pixelStamp = 0;
            m_cache_memMap[index].fontModification.pixelStamp = 0;
            m_cache_memMap[index].properties = 0;
            m_cache_memMap[index].size = 0;
            m_cache_memMap[index].tabbedSize = 0;
        }
    }
}
