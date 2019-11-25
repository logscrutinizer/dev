/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QFileInfo>

#include "CFilter.h"
#include "CDebug.h"
#include <hs/hs.h>

CFilter::CFilter(void)
{
    m_ID = -1;

    m_fontCtrl_p = nullptr;

    m_fileName = "";
    m_fileName_short = "";

    m_newFilterItem_p = nullptr;
    m_inFilterTag = false;
}

CFilter::~CFilter(void)
{
    while (!m_filterItemList.isEmpty()) {
        auto item = m_filterItemList.takeLast();
        if (nullptr != item) {
            delete(item);
        }
    }
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CFilter::Init(const char *name_p, int ID, CFontCtrl *fontCtrl_p)
{
    m_fileName = name_p;
    m_ID = ID;
    m_fontCtrl_p = fontCtrl_p;

    SetFileName(name_p);
}

/***********************************************************************************************************************
*   GetFileNameOnly
***********************************************************************************************************************/
void CFilter::GetFileNameOnly(QString *fileName_p)
{
    QFileInfo fileInfo(m_fileName);
    *fileName_p = fileInfo.fileName(); /* its actually the same as m_fileName_short */
}

/***********************************************************************************************************************
*   ~CFilterItem
***********************************************************************************************************************/
CFilterItem:: ~CFilterItem(void)
{
    /* The m_font_p is deleted in the CFontCtrl object when that is destructed */
    if (m_freeStartRef) {
        free(m_start_p);
    }
}

/***********************************************************************************************************************
*   Check
***********************************************************************************************************************/
int CFilterItem::Check(QString& string)
{
    if (m_start_p == nullptr) {
        string = "internal error, string is nullptr";
        return -1;
    }

    if (m_size <= 0) {
        string = "emtpy filter";
        return -1;
    }

    if (m_regexpr) {
        hs_compile_error_t *compile_err;
        hs_database_t *database = nullptr;
        const unsigned int REGEXP_HYPERSCAN_FLAGS_TEST = (HS_FLAG_DOTALL | HS_FLAG_SINGLEMATCH);

        if (hs_compile_ext_multi(&m_start_p,
                                 &REGEXP_HYPERSCAN_FLAGS_TEST,
                                 nullptr, /*ids*/
                                 nullptr, /*ext*/
                                 1, /*elements*/
                                 HS_MODE_BLOCK, /*mode*/
                                 nullptr, /*platform*/
                                 &database,
                                 &compile_err) != HS_SUCCESS) {
            string = QString("Bad regexp, %1").arg(compile_err->message);
            hs_free_compile_error(compile_err);
            return -1;
        }
    }

    return 1;
}

namespace FilterMgr
{
    /* Go through the entire FIRA and move the items to the packed FIRA */
    packed_FIR_t *GeneratePackedFIRA(TIA_t& TIA, FIRA_t& FIRA, CFilterItem **filterItem_LUT_pp)
    {
        int index;
        packed_FIR_t *packedFIR_base_p;
        packed_FIR_t *packedFIR_p;
        const int numOfItems = TIA.rows;

        if (FIRA.filterMatches == 0) {
            return nullptr;
        }

        FIR_t *FIR_Array_p = &FIRA.FIR_Array_p[0];

        packedFIR_base_p =
            reinterpret_cast<packed_FIR_t *>(VirtualMem::Alloc(static_cast<int64_t>(sizeof(packed_FIR_t)) *
                                                               FIRA.filterMatches));

        if (packedFIR_base_p == nullptr) {
            TRACEX_E("CLogScrutinizerDoc::CreatePackedFIRA    packedFIRA_p nullptr, out of memory?")
            FIRA.filterMatches = 0;
            return nullptr;
        }

        packedFIR_p = packedFIR_base_p;

        /* Do not pack exclude filters, these are not part of the count of m_database.FIRA.filterMatches */

        int count = 0;
        for (index = 0; index < numOfItems; ++index) {
            uint8_t LUT_Index = FIR_Array_p[index].LUT_index;

            if ((LUT_Index != 0) && !filterItem_LUT_pp[LUT_Index]->m_exclude) {
                packedFIR_p->LUT_index = LUT_Index;
                packedFIR_p->row = index;
                if (FIR_Array_p[index].index != count) {
                    TRACEX_E(
                        QString("Internal error packedFIRA and FIRA doesn't match packed:%1 matches:%2")
                            .arg(FIR_Array_p[index].index).arg(count))
                }
                ++packedFIR_p;
                count++;
            }
        }

        if ((packedFIR_p - packedFIR_base_p) != FIRA.filterMatches) {
#ifdef _DEBUG
            TRACEX_E(
                QString("Internal error  packedFIRA and FIRA doesn't match packed:%1 matches:%2")
                    .arg(packedFIR_p - packedFIR_base_p).arg(FIRA.filterMatches).arg(count))
#endif
            FIRA.filterMatches = 0;
            return nullptr;
        }

        return packedFIR_base_p;
    }

    /****/
    void InitializeFilterItem_LUT(CFilterItem **filterItem_LUT_pp, CFilterItem *bookmark_p)
    {
        memset(&filterItem_LUT_pp[0], 0, sizeof(CFilterItem *) * MAX_NUM_OF_ACTIVE_FILTERS);
        filterItem_LUT_pp[BOOKMARK_FILTER_LUT_INDEX] = bookmark_p;
    }

    /****/
    void ReNumerateFIRA(FIRA_t& FIRA, TIA_t& TIA, CFilterItem **filterItem_LUT_pp)
    {
        int FIR_Count = 0;
        int FIR_Exclude_Count = 0;
        int index;
        FIR_t *FIRA_p = FIRA.FIR_Array_p;
        const int numOfTextItems = TIA.rows;

        for (index = 0; index < numOfTextItems; ++index) {
            if (FIRA_p[index].LUT_index != 0) {
                if (!filterItem_LUT_pp[FIRA_p[index].LUT_index]->m_exclude) {
                    FIRA_p[index].index = FIR_Count;
                    ++FIR_Count;
                } else {
                    ++FIR_Exclude_Count;
                }
            }
        }

        FIRA.filterMatches = FIR_Count;
        FIRA.filterExcludeMatches = FIR_Exclude_Count;
    }
}

/***********************************************************************************************************************
*   GenerateFilterItems
***********************************************************************************************************************/
void CFilterContainer::GenerateFilterItems(FilterItemInitializer *filterInitializers_p, int count)
{
    CFilter *filter_p = new CFilter();
    filter_p->m_fileName = "default_filter";
    filter_p->m_fileName_short = "default_filter";
    m_filters.append(filter_p); /* This append doesn't affect the workspace model since the append is to a container */

    for (int index = 0; index < count; ++index) {
        CFilterItem *filterItem_p = new CFilterItem; /* will be removed when filter is removed (destructor) */
        filterItem_p->m_start_p = filterInitializers_p[index].text;
        filterItem_p->m_size = static_cast<int>(strlen(filterItem_p->m_start_p));
        filterItem_p->m_enabled = true;
        filter_p->m_filterItemList.append(filterItem_p);
    }
}

/***********************************************************************************************************************
*   GenerateLUT
***********************************************************************************************************************/
void CFilterContainer::GenerateLUT()
{
    /* The LUT is a simple array of pointers to the filterItems. The position in the array gives the filter its LUT
     * index, used later */

    memset(m_filterItem_LUT, 0, sizeof(m_filterItem_LUT));

    int filterItemIndex = 0;
    m_filterItem_LUT[filterItemIndex++] = nullptr; /* First (at index 0) shall always be nullptr. In the FIR array a LUT
                                                    * index of 0 means that there was no filter match for that row */

    for (QList<CFilter *>::iterator filterIter = m_filters.begin(); filterIter != m_filters.end(); ++filterIter) {
        for (QList<CFilterItem *>::iterator filterItemIter = (*filterIter)->m_filterItemList.begin();
             filterItemIter != (*filterIter)->m_filterItemList.end();
             ++filterItemIter) {
            if ((*filterItemIter)->m_enabled) {
                m_filterItem_LUT[filterItemIndex++] = *filterItemIter;
            }
        }
    }
}
