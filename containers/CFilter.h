/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QFileInfo>

#include "CConfig.h"
#include "CXMLBase.h"
#include "CFontCtrl.h"
#include "CDebug.h"
#include "CMemPool.h"
#include <stdint.h>

#include <string>

static int g_unique_id_count = 0;

/***********************************************************************************************************************
*   CFilterItem
***********************************************************************************************************************/
class CFilterItem
{
public:
    CFilterItem(QString *start_p = nullptr) :
        m_start_p(nullptr), m_font_p(nullptr), m_uniqueID(0), m_size(0), m_color(BLACK), m_bg_color(BACKGROUND_COLOR),
        m_freeStartRef(false), m_enabled(true), m_caseSensitive(false),
        m_exclude(false), m_regexpr(false), m_adaptiveClipEnabled(false)
    {
        if (start_p != nullptr) {
            m_start_p = reinterpret_cast<char *>(malloc(static_cast<size_t>(start_p->length()) + 1));
            m_size = start_p->length();
            m_freeStartRef = true;
            memcpy(m_start_p, start_p->toLatin1().constData(), static_cast<size_t>(m_size));
            m_start_p[m_size] = 0;
        }
        m_uniqueID = g_unique_id_count++;
    }

    CFilterItem(CFilterItem& from)
    {
        copy(&from);
    }

    CFilterItem(CFilterItem *from_p)
    {
        copy(from_p);
    }

    /****/
    void copy(CFilterItem *from_p)
    {
        m_start_p = reinterpret_cast<char *>(malloc(static_cast<size_t>(from_p->m_size + 1)));

        m_size = from_p->m_size;
        m_freeStartRef = true;

        if (m_start_p != nullptr) {
            memcpy(m_start_p, from_p->m_start_p, static_cast<size_t>(m_size + 1));
        } else {
            TRACEX_E("CFilterItem::CFilterItem  m_start_p nullptr")
        }

        m_color = from_p->m_color;
        m_bg_color = from_p->m_bg_color;
        m_font_p = from_p->m_font_p;
        m_enabled = from_p->m_enabled;
        m_caseSensitive = from_p->m_caseSensitive;
        m_exclude = from_p->m_exclude;
        m_regexpr = from_p->m_regexpr;
        m_adaptiveClipEnabled = from_p->m_adaptiveClipEnabled;
        m_uniqueID = from_p->m_uniqueID;
    }

    int Check(QString& string); /* string will contain the error text */

    virtual ~CFilterItem(void);

public:
    char *m_start_p; /* Reference to the start of the filter text */
    FontItem_t *m_font_p; /* Font used to paint rows matching this filter (in combination with color) */
    int m_uniqueID; /* This is used to cross reference the filters that exist in ViewTree with the copy used for
                     * filtering, -1 when not set */
    int m_size; /* Number of lettr in this filter item, no 0 in the end, size is only letters */
    Q_COLORREF m_color; /* Which color lines matching this filter item will painted with */
    Q_COLORREF m_bg_color; /* */
    bool m_freeStartRef; /* this will be set to true if the m_start_p needs to be freed in the destructor */
    bool m_enabled; /* This flag is used to disable the filter from the filering */
    bool m_caseSensitive;
    bool m_exclude; /* This is set if a specific line matching this filter shall be not shown */
    bool m_regexpr; /* True if the m_start_p filterMatch is defined with regular expression */
    bool m_adaptiveClipEnabled;  /* True if this filter is enabled for adaptive clipping */
};

/***********************************************************************************************************************
*   CFilter
***********************************************************************************************************************/
class CFilter /*: public CXMLBase */
{
public:
    CFilter(void) = default;

    CFilter(const CFilter& from)
    {
        copy(&from);
    }

    CFilter(const CFilter *from_p)
    {
        copy(from_p);
    }

    /*
       CFilter(QString&& fileName, QList<CFilterItem *>&& filterItemList)
       {
        m_fileName = fileName;
        m_filterItemList = filterItemList;

        QFileInfo fileInfo(m_fileName);
        m_fileName_short = fileInfo.fileName();
       }
     */
    ~CFilter(void)
    {
        while (!m_filterItemList.isEmpty()) {
            auto item = m_filterItemList.takeLast();

            if (nullptr != item) {
                delete(item);
            }
        }
    }

    /**/
    void Init(const char *name_p)
    {
        m_fileName = name_p;
        SetFileName(name_p);
    }

    /**/
    void GetFileNameOnly(QString *fileName_p)
    {
        QFileInfo fileInfo(m_fileName);
        *fileName_p = fileInfo.fileName(); /* its actually the same as m_fileName_short */
    }

    /****/
    void copy(const CFilter *from_p)
    {
        m_fileName = from_p->m_fileName;
        m_fileName_short = from_p->m_fileName_short;

        m_newFilterItem_p = nullptr;
        m_inFilterTag = false;

        for (auto& filterItem_p : from_p->m_filterItemList) {
            CFilterItem *filterItemCopy_p = new CFilterItem(filterItem_p);
            m_filterItemList.append(filterItemCopy_p);
        }
    }

    /****/
    void SetFileName(const QString& fileName)
    {
        m_fileName = fileName;

        QFileInfo fileInfo(m_fileName);
        m_fileName_short = fileInfo.fileName();
    }

    QString *GetFileName(void) {return &m_fileName;}
    QString& GetFileNameRef() {return m_fileName;}
    QString *GetShortFileName(void) {return &m_fileName_short;}

public:
    QList<CFilterItem *> m_filterItemList;
    QString m_fileName = ""; /* for file save */
    QString m_fileName_short = ""; /* for presentation */
    bool m_inFilterTag = false; /* Used for XML parsing */
    CFilterItem *m_newFilterItem_p = nullptr; /* Used for XML parsing */
};

/* TODO: Shrink the TIA array
 *    1. Store only sizes in textItemArray_p
 *    2. Have a seperate array which contains rows / 1000 (x) number of fileIndexs.
 *    3. To find the file index entry of a row, you sum all the rows from closest fileIndex, and add it to the file
 * index
 *  */

/* Not much of the text file is kept in memory, the row database contains mainly an index into the log file.
 * However the rows are cached in the CLogScrutinizerDoc to improve load time */

typedef struct {
    int64_t fileIndex;   /* Offset into the file where the text line starts */
    int32_t size;        /* Size of the text string */
}TI_t; /* Text Item */

typedef struct
{
    int32_t rows; /* Total number of rows in the log file */
    TI_t *textItemArray_p;
}TIA_t; /* Text Item Array */

/* TODO, Eventhough the LUT_index is only one byte, it probarbly take up 4 bytes, in order to do structure alignment
 *       Perhaps seperate LUT_index from FIT_T */
typedef struct  /* Filter Item Reference Array */
{
    int32_t index; /* The index into the array of packed_FIR_t elements, for fast lookup
                    * A value (not zero) also corresponds to the number of matches up to that point */
    uint8_t LUT_index; /* Reference to a filter that matched the corresponding row
                        * BOOKMARK_FILTER_LUT_INDEX 0xff  reserved to bookmark
                        * 0 Means no match */
}FIR_t;

typedef struct  /* Use for array with only filter matches (packed) */
{
    int32_t row; /* Row lookup index (into the log) */
    uint8_t LUT_index; /* Reference to a filter that matched the corresponding row */
}packed_FIR_t;

typedef struct {
    FIR_t *FIR_Array_p; /* Filter selected for the row with the same index */
    int32_t filterMatches; /* Number of items in the FIR array that has a filter reference (filter hit, non
                            * exclude filters) */
    int32_t filterExcludeMatches; /* Number of items in the FIR array that has a filter reference to exclude filters*/
}FIRA_t;

typedef struct {
    char *start_p;
    int length;
    CFilterItem *filterRef_p;
    bool m_adaptiveClipEnabled;  /* True if this filter is enabled for adaptive clipping */
    bool m_colClip_StartEnabled; /* Column wise clip start enabling (filterItem specific) */
    int m_colClip_Start; /* Column wise clip start enabling (filterItem specific) */
    bool m_colClip_EndEnabled;
    int m_colClip_End;
    int m_regExpLUTIndex; /* Used during filtering to get correct RegExp data. */
}packedFilterItem_t;

namespace FilterMgr
{
    // startIndex and startCount is used when incrementally populating the FIRA array
    bool PopulatePackedFIRA(TIA_t& TIA, FIRA_t& FIRA, packed_FIR_t* packedFIR_base_p, CFilterItem** filterItem_LUT_pp, unsigned startIndex = 0, unsigned startCount = 0);
    void InitializeFilterItem_LUT(CFilterItem **filterItem_LUT_pp, CFilterItem *bookmark_p);
    void ReNumerateFIRA(FIRA_t& FIRA, TIA_t& TIA, CFilterItem **filterItem_LUT_pp);
}

struct FilterItemInitializer {
public:
    char text[256];
    bool regExp;
    bool m_caseSensitive;
};

/***********************************************************************************************************************
*   CFilterContainer
***********************************************************************************************************************/
class CFilterContainer
{
public:
    CFilterContainer() {memset(m_filterItem_LUT, 0, sizeof(m_filterItem_LUT));}

    ~CFilterContainer()
    {
        while (!m_filters.isEmpty()) {
            auto item = m_filters.takeLast();
            if (nullptr != item) {
                delete (item);
            }
        }
    }

    void GenerateFilterItems(FilterItemInitializer *filterInitializers_p, int count);
    void GenerateLUT();

    QList<CFilter *>& GetFiltersList() {return m_filters;}

    /****/
    void PopulateFilterItemList(QList<CFilterItem *>& filterItems) {
        for (auto& filter_p : m_filters) {
            for (auto& filterItem_p : filter_p->m_filterItemList) {
                filterItems.append(filterItem_p);
            }
        }
    }

    CFilterItem **GetFilterLUT() {return m_filterItem_LUT;}

private:
    QList<CFilter *> m_filters;
    CFilterItem *m_filterItem_LUT[MAX_NUM_OF_ACTIVE_FILTERS];
};
