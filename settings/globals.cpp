/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CConfig.h"
#include "globals.h"
#include "CDebug.h"
#include "quickFix.h"

bool CSCZ_TreeViewSelectionEnabled = true; /**< Determine if it is OK to do TreeView selections, typically turned of
                                            * when loading files */
bool CSCZ_ToolTipDebugEnabled = false;
bool CSCZ_AdaptWindowSizes = false; /**< When no default window sizes was found in settings */
bool CSZ_DB_PendingUpdate = false; /**< When DB is being updated */
SystemState CSCZ_SystemState = SYSTEM_STATE_STARTING; /**< flag that indicates when the system is
                                                       *  starting and shutting */

CSCZ_LastSelectionKind_en CSCZ_GetLastSelection(void);

static CSCZ_LastSelectionKind_en CSCZ_Global_LastSelectionKind = CSCZ_LastSelectionKind_None_e;
static CSCZ_LastViewSelectionKind_en CSCZ_Global_LastViewSelectionKind = CSCZ_LastViewSelectionKind_None_e;
static CSCZ_LastViewSelectionKind_en CSCZ_Global_PendingSelectionKind = CSCZ_LastViewSelectionKind_None_e;

/***********************************************************************************************************************
*   CSCZ_SetLastSelectionKind
***********************************************************************************************************************/
void CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_en selectionKind)
{
    CSCZ_Global_LastSelectionKind = selectionKind;
}

/***********************************************************************************************************************
*   CSCZ_GetLastSelectionKind
***********************************************************************************************************************/
CSCZ_LastSelectionKind_en CSCZ_GetLastSelectionKind(void)
{
    return CSCZ_Global_LastSelectionKind;
}

/***********************************************************************************************************************
*   CSCZ_SetLastViewSelectionKind
***********************************************************************************************************************/
void CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_en selectionKind)
{
    if (selectionKind != CSCZ_Global_LastViewSelectionKind) {
        CSCZ_Global_LastViewSelectionKind = selectionKind;
#ifdef _DEBUG
        TRACEX_DE("CSCZ_SetLastViewSelectionKind %d", CSCZ_Global_LastViewSelectionKind)
#endif
    }
}

/***********************************************************************************************************************
*   CSCZ_UnsetLastViewSelectionKind
***********************************************************************************************************************/
void CSCZ_UnsetLastViewSelectionKind(CSCZ_LastViewSelectionKind_en selectionKind)
{
    if ((selectionKind != CSCZ_LastViewSelectionKind_None_e) &&
        (selectionKind == CSCZ_Global_LastViewSelectionKind)) {
        CSCZ_Global_PendingSelectionKind = selectionKind;
        CSCZ_Global_LastViewSelectionKind = CSCZ_LastViewSelectionKind_None_e;
#ifdef _DEBUG
        TRACEX_DE("CSCZ_UnsetLastViewSelectionKind")
#endif
    }
}

/***********************************************************************************************************************
*   CSCZ_LastViewSelectionKind
***********************************************************************************************************************/
CSCZ_LastViewSelectionKind_en CSCZ_LastViewSelectionKind(void)
{
    if (CSCZ_Global_LastViewSelectionKind != CSCZ_LastViewSelectionKind_Menu_e) {
        return CSCZ_Global_LastViewSelectionKind;
    } else {
        return CSCZ_Global_PendingSelectionKind;    /* Since menu took over the view in focus we have saved the previous
                                                     * here */
    }
}
