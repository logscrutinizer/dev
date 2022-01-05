/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QFileInfo>

#include "CFilter.h"
#include "CDebug.h"
#include <hs/hs.h>

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
    /* PopulatePackedFIRA
     *
     * This function creates (or extends) the packedFIRA table. The packedFIRA table contains
     * just information about each filter match, hence the number rows in this table is identical to
     * the total number of filter matches.
     *
     * The function loops through the FIRA table and create one item in the packedFIRA for
     * each filter match. In case of incremental filtering it extends the table.
     *
     * startIndex: From where in the log file, the TIA index
     * startCount: If the packedFIR_base_p already contains items, then how many.

     * When doing incremental filtering then some of the old (TIA) rows might be "re-filtered", then we need to update
     * these entries in the packedFIR. In that case we first need to establish from where in 
     * the packedFIR we need to start and overwrite existing information.
     * 
     */
    bool PopulatePackedFIRA(TIA_t& TIA,
                            FIRA_t& FIRA,
                            packed_FIR_t *packedFIR_base_p,
                            CFilterItem **filterItem_LUT_pp,
                            unsigned startIndex,
                            unsigned startCount)
    {
        if (FIRA.filterMatches == 0) {
            return false;
        }

        FIR_t *FIR_Array_p = &FIRA.FIR_Array_p[0];

        if (packedFIR_base_p == nullptr) {
            TRACEX_E("CLogScrutinizerDoc::CreatePackedFIRA    packedFIRA_p nullptr, out of memory?")
            FIRA.filterMatches = 0;
            return false;
        }

        int packedCount = startCount;

        if (startCount > 0) {
            /* There is pre-existing filter items in the packedFIR, search for first packed item that needs to be part of
               incremental update. */
            auto packedFIR_p = &packedFIR_base_p[startCount - 1];
            for (unsigned index = startCount - 1; index > 0; index--) {
                if (packedFIR_p->row < static_cast<int>(startIndex)) {
                    packedCount = index + 1;
                    break;
                }
                --packedFIR_p;
            }
        }

        auto packedFIR_p = &packedFIR_base_p[packedCount];
        /* Note: do not pack exclude filters, these are not part of the count of m_database.FIRA.filterMatches */
        const int numOfItems = TIA.rows;
        for (int index = startIndex; index < numOfItems; ++index) {
            uint8_t LUT_Index = FIR_Array_p[index].LUT_index;
            if ((LUT_Index != 0) && !filterItem_LUT_pp[LUT_Index]->m_exclude) {
                packedFIR_p->LUT_index = LUT_Index;
                packedFIR_p->row = index;
                if (FIR_Array_p[index].index != packedCount) {
                    TRACEX_E(
                        QString("Internal error packedFIRA and FIRA doesn't match packed:%1 matches:%2")
                            .arg(FIR_Array_p[index].index).arg(packedCount))
                }
                ++packedFIR_p;
                ++packedCount;
            }
        }

        if ((packedFIR_p - packedFIR_base_p) != FIRA.filterMatches) {
#ifdef _DEBUG
            TRACEX_E(
                QString("Internal error  packedFIRA and FIRA doesn't match packed:%1 matches:%2")
                    .arg(packedFIR_p - packedFIR_base_p).arg(FIRA.filterMatches).arg(packedCount))
#endif
            FIRA.filterMatches = 0;
            return false;
        }

        return true;
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
            const auto fira_p = &FIRA_p[index];
            const auto lut_index = fira_p->LUT_index;
            if (lut_index != 0) {
                if (!filterItem_LUT_pp[lut_index]->m_exclude) {
                    fira_p->index = FIR_Count;
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
