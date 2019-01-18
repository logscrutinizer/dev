/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#define TOOL_TIP_DEBUG 1

#include "CConfig.h"

#define STATE_MOVEPLOT_NOT_ACTIVE         10
#define STATE_MOVEPLOT_PENDING            11      /* Used to post-pone update of screen */

#define TO_TT_WAIT_FOR_TOOL_TIP_REQUEST      (200)   /* milli seconds */
#define TO_TT_PENDING                        (400)
#define TO_TT_RUNNING                        (200)
#define TO_TT_CLOSE                          (200)

/* Constants used for encoding/decoding data for drag and drop */
const quint32 MIME_STREAM_START_TAG = 0x11111111;
const quint32 MIME_ITEM_START_TAG = 0x22222222;
const quint32 MIME_ITEM_END_TAG = 0x33333333;
const quint32 MIME_STREAM_END_TAG = 0x44444444;
const quint32 MIME_CFG_ITEM_KIND_TAG = 0xFEED00;
const quint32 MIME_CFG_ITEM_KIND_TAG_BITS = 0xFFFF00;
const quint32 MIME_CFG_ITEM_KIND_BITS = 0xFF;

typedef enum {
    ToolTipState_WaitForRequest,
    ToolTipState_Pending,
    ToolTipState_Running,
    ToolTipState_Closing
}  ToolTipState;

typedef enum {
    CSCZ_LastSelectionKind_None_e,
    CSCZ_LastSelectionKind_TextSel_e,
    CSCZ_LastSelectionKind_FilterSel_e,
    CSCZ_LastSelectionKind_FilterItemSel_e,
    CSCZ_LastSelectionKind_SearchCombo_e,
    CSCZ_LastSelectionKind_SearchRun_e,
    CSCZ_LastSelectionKind_SearchDialog_e,
    CSCZ_LastSelectionKind_SubPlot_e,
}CSCZ_LastSelectionKind_en;

typedef enum {
    CSCZ_LastViewSelectionKind_None_e,
    CSCZ_LastViewSelectionKind_TextView_e,
    CSCZ_LastViewSelectionKind_Workspace_e,
    CSCZ_LastViewSelectionKind_PlotView_e,
    CSCZ_LastViewSelectionKind_Menu_e,
    CSCZ_LastViewSelectionKind_Frame_e,
    CSCZ_LastViewSelectionKind_SearchCombo_e,
}CSCZ_LastViewSelectionKind_en;

typedef enum {
    SYSTEM_STATE_STARTING,
    SYSTEM_STATE_RUNNING,
    SYSTEM_STATE_SHUTDOWN
} SystemState;

extern SystemState CSCZ_SystemState; /* flag that indicates when the system is starting and shutting down */

/* VARIABLES */
extern bool CSCZ_TreeViewSelectionEnabled;
extern bool CSCZ_ToolTipDebugEnabled;
extern bool CSCZ_AdaptWindowSizes;
extern bool CSZ_DB_PendingUpdate; /* When DB is being updated */

/* FUNCTIONS */
extern void CSCZ_SetLastSelectionKind(CSCZ_LastSelectionKind_en selectionKind);
extern CSCZ_LastSelectionKind_en CSCZ_GetLastSelectionKind(void);
extern void CSCZ_SetLastViewSelectionKind(CSCZ_LastViewSelectionKind_en selectionKind);
extern void CSCZ_UnsetLastViewSelectionKind(CSCZ_LastViewSelectionKind_en selectionKind);
extern CSCZ_LastViewSelectionKind_en CSCZ_LastViewSelectionKind(void);

/***********************************************************************************************************************
*   ScopeGuard
*   @param exitFunc TODO
*   @return TODO
***********************************************************************************************************************/
template<typename lambda>
class ScopeGuard
{
public:
    explicit ScopeGuard(lambda exitFunc) : m_exitFunc(exitFunc) {}
    ~ScopeGuard() {m_exitFunc();}

    lambda m_exitFunc;
};

/***********************************************************************************************************************
*   makeMyScopeGuard
*   @param theLambda TODO
*   @return TODO
***********************************************************************************************************************/
template<typename rlambda>
ScopeGuard<rlambda> makeMyScopeGuard(rlambda theLambda)
{
    return ScopeGuard<rlambda>(theLambda);
}

/***********************************************************************************************************************
*   CGlobalsAutoReset_BOOL
***********************************************************************************************************************/
class CGlobalsAutoReset_BOOL
{
public:
    CGlobalsAutoReset_BOOL(bool *value_p, bool init, bool reset)
    {
        *value_p = init;
        m_reset = reset;
        m_value_p = value_p;
    }

    ~CGlobalsAutoReset_BOOL()
    {
        *m_value_p = m_reset;
    }

private:
    CGlobalsAutoReset_BOOL();

    bool m_reset;
    bool *m_value_p;
};
