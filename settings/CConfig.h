/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CCONFIG_H
#define CCONFIG_H

#include <QString>
#include <QColor>
#include <QTextStream>
#include <QFile>

#include <stdint.h>
#include "CDebug.h"

/*
 *
 * ---------- Product Version Kinds
 *
 * BETA
 * ----
 * BETA versions are used to grow functionality in steps, to reach a final version. Each BETA will consist of one or
 * several feature request tickets. Also fixes for error and support tickets will be included.
 *
 * Release Candidate (RC)
 * ----------------------
 * Before a Final version is defined there will be one or several release candidates to smoke out remaining bugs. Only
 * error tickets will be included. The version number will be as e.g. 1.2 RC1, first release candidate.
 *
 * Final
 * -----
 * A final version is a stable product, based on a collection of BETA and Release Candidates releases. A final version
 * contains only two version numbers, such as 2.1
 *
 * Patch
 * -----
 * A patch version contains only error tickets. A patch is numbered with a 3rd number, like 1.2.1
 *
 */

#define APP_VER_CODE_KIND_BETA     (0x00)
#define APP_VER_CODE_KIND_RC       (0x01)         /* Relase candidate */
#define APP_VER_CODE_KIND_FINAL    (0x02)
#define APP_VER_CODE_KIND_PATCH    (0x03)

/*
 * An example of version numbering that could occur when developing a new Final version would then start with some
 * Beta versions: 1.2 B1 -> 1.2 B2 ... then one or several Release Candidates: 1.2 RC1, and then the Final: 1.2. If bug
 * corrections are needed for a Final release these will show up as Patch versions, and version numbering would
 * start as 1.2.1 -> 1.2.2 etc.
 *
 * ---------- Product Version Status
 *
 * Status             Info
 *
 * Planned            The product version has been planned, however development has not started yet
 * Development    The product version is being developed, there are no public downloads
 * Evaluation     The product version developed, however its being tested, it is possible to download it
 * Released         The product version is considered well tested
 */

/* CHANGE BELOW TO UPDATE RELEASE VERSION */

#define APP_TITLE       ("LogScrutinizer")

#define APP_VER_CODE_MAJOR         2                          /* If 1: -> v1 */
#define APP_VER_CODE_MINOR         0                          /* If 4: -> vx.4 */
#define APP_VER_CODE_KIND          APP_VER_CODE_KIND_BETA     /* BETA, RC, */
/* Patch, Final */
#define APP_VER_CODE_SUB_VERSION   7                          /* If 1 : -> vx.x */

/* B1  (for BETA),
 *  vx.x.1 (for
 * PATCH), vx.x
 * RC1 (for
 * Release
 * Candidate) */
#define APP_VER_CODE_BUILD         0                          /* If 1 : -> vx.x */
/* Bx build1 */

/*
 * SLOGAN on the LogScrutinizer Logo:           It is there, you just have to find it
 *                              (in latin)   Is est illic, vos iustus have ut reperio is
 *
 * The greenish color in the logo DCF62D
 */

#define TIA_FILE_VERSION      0x55555556

#define APP_BUILD_VER   (__DATE__)

#define CSS_SUPPORT 0

#define COPY_PASTE_CHAR_SIZE    14
#define TEXT_WINDOW_CHAR_SIZE   8
#define WORKSPACE_TEXT_WINDOW_CHAR_SIZE   10
#define STATUS_BAR_CHAR_SIZE    12
#define EMPTY_WINDOW_CHAR_SIZE  10

#define PLUGIN_MSG_HEAP_SIZE       (1024 * 1000)    /* 1MB inital heap size */
#define PLUGIN_MSG_HEAP_SIZE_MAX   (1024 * 10000)   /* 10MB max heap size */

#define MAX_COLOR_TABLE                   48                  /* Maximum number */

/* of colors in
 * the Font table */

#define CACHE_CMEM_POOL_SIZE_SMALLEST     (1024)
#define CACHE_CMEM_POOL_SIZE_1            (2048)
#define CACHE_CMEM_POOL_SIZE_2            (1024 * 4)
#define CACHE_CMEM_POOL_SIZE_3            (1024 * 32)
#define CACHE_CMEM_POOL_SIZE_MAX          (1024 * 64)

#define BOOKMARK_FILTER_LUT_INDEX         0xff

#define MAX_NUM_OF_ACTIVE_FILTERS         256  /* Used for creating a FIR lookup table, which can only index 0 1-ff,
                                                * where 0 means no filter */

#define CFG_FILTERS_TAG              "filters"
#define CCFG_FILTER_TAG              "filter"

#define CFG_PLUGINS_TAG              "filters"
#define CFG_PLUGIN_TAG               "filter"

#define CFG_SETTINGS_TAG             "filters"
#define CFG_SETTING_TAG              "filter"

#define XML_FILE_HEADER               "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>"
#define LCZ_HEADER                    "<LogScrutinizer version=\"2\">"   /* file */

/* format
 * version */
#define LCZ_FOOTER                    "</LogScrutinizer>"

#define LCZ_FILE_VERSION              (2)

#define SETTINGS_HEADER               "  <settings>"
#define SETTINGS_FOOTER               "  </settings>"
#define SETTING_HEADER                "    <setting "
#define SETTING_FOOTER                "/>"
#define SEARCH_ITEM_HEADER            "    <search "
#define SEARCH_ITEM_FOOTER            "/>"

#define RECENTFILE_HEADER             "<recentfiles>"
#define RECENTFILE_FOOTER             "</recentfiles>"

#define EDITOR_WINDOW_Y_SIZE_RATIO_HINT      0.8
#define TAB_WINDOW_Y_SIZE_RATIO_HINT         (1.0 - EDITOR_WINDOW_Y_SIZE_RATIO_HINT)
#define WORKSPACE_WINDOW_Y_SIZE_RATIO_HINT   (1.0)

#define EDITOR_WINDOW_X_SIZE_RATIO_HINT      0.7
#define TAB_WINDOW_X_SIZE_RATIO_HINT         EDITOR_WINDOW_X_SIZE_RATIO_HINT
#define WORKSPACE_WINDOW_X_SIZE_RATIO_HINT   (1.0 - EDITOR_WINDOW_X_SIZE_RATIO_HINT)

/* The padding is % based, e.g. 30% of the text size */
typedef struct {
    float left;
    float right;
    float top;
    float bottom;
}CFG_PADDING_t;

#define MAX_NUM_OF_RECENT_FILES 30
#define MAX_COLOR_NAME 256

#undef Q_RGB
#define Q_RGB(R, G, B) (static_cast<QRgb>((R) << 16 | (G) << 8 | (B)))
#define Q_COLORREF QRgb

typedef struct {
    QRgb color;
    char name[MAX_COLOR_NAME];
}ColorTableItem_t;

struct CSCZ_CfgColorItem {
public:
    ColorTableItem_t m_colorTableItem;
    int m_index;
};

/*--------------  from cfontctrl ----------------------- */

typedef int FontMod_t;

#define FONT_MOD_NONE           0
#define FONT_MOD_BOLD           1
#define FONT_MOD_ITALIC         2
#define FONT_MOD_UNDERLINE      4
#define FONT_MOD_COLOR_MOD      8

typedef struct {
    Q_COLORREF color;
    Q_COLORREF bg_color;
    QFont *font_p;
    QPen *pen_p;
    FontMod_t fontMod;

    /* Used when color is modified to create a certain effect. Then if all fonts with
     * this effect shall be modified it is essential to know how to modify from the original */
    Q_COLORREF originalColor;
}FontItem_t;

/*-------------- */

#define GRAPH_RED             Q_RGB(231, 47, 39)
#define GRAPH_BLUE            Q_RGB(46, 20, 141)
#define GRAPH_GREEN           Q_RGB(19, 154, 47)
#define GRAPH_PURPLE          Q_RGB(178, 137, 166)
#define GRAPH_ORANGE          Q_RGB(238, 113, 25)
#define GRAPH_GRAY            Q_RGB(160, 160, 160)
#define GRAPH_DARK_RED_GRAY   Q_RGB(101, 55, 55)

/* C SCZ  C Scrutinizer */

#define FONT_DEFAULT_SIZE   8

/*WHEEL_DELTA , A positive value indicates that the wheel was rotated forward
 * (scroll value / WHEEL_DELTA ) × lines-to-scroll user setting = lines to scroll */
#define WHEEL_ANGLE_PER_TICK 15

#define SCROLL_FRAME_THICKNESS 20

#define SELECTION_COLOR_IN_FOCUS          (Q_RGB(0x97, 0xD2, 0xFC))  /* Color * used * when * something * has been
                                                                      * selected */
#define SELECTION_COLOR_PASSIVE_FOCUS     (Q_RGB(0xF0, 0xF0, 0xF0))  /* Color used when the selection remains but
                                                                      * the window is not in focus anymore */

#define PLOT_FOCUS_LINE_COLOR             (Q_RGB(0x47, 0x92, 0xAC))
#define PLOT_PASSIVE_FOCUS_LINE_COLOR     (Q_RGB(0x97, 0xD2, 0xFC))

#define BOOKMARK_COLOR                    (Q_RGB(0xCC, 0xFF, 0xFF))
#define BOOKMARK_COLOR_FRAME              (Q_RGB(0xA0, 0x9E, 0x5F))

#define DECODED_COLOR                     (Q_RGB(0xFF, 0x99, 0x00))  /* FF9900 */

#define AUTO_HIGHLIGHT_COLOR              (Q_RGB(230, 230, 250))

#define BLACK                             (Q_RGB(0, 0, 0))
#define WHITE                             (Q_RGB(255, 255, 255))
#define DKGRAY                            (Q_RGB(128, 128, 128))
#define LTGRAY                            (Q_RGB(192, 192, 192))

#define FILTERED_TEXT_LTGRAY_COLOR        (Q_RGB(135, 135, 135))

#define DEFAULT_SCROLL_ARROW_SCALE (30)   /* 30/100 => 30% of original size */

#define CB_BG_DEFAULT       LTGRAY
#define CB_FG_DEFAULT       BLACK            /* black text */
#define CB_SID_DEFAULT      DKGRAY          /* dark gray disabled text */

#define BACKGROUND_COLOR                    Q_RGB(255, 250, 240)          /*FLORALWHITE */
#define CLIPPED_TEXT_COLOR                  Q_RGB(0xff, 0xb2, 0xb2)       /* Light red */
#define CLIPPED_TEXT_ROCKSCROLL_COLOR       qRgba(0xff, 0xb2, 0xb2, 0xff) /* Light red, alpha 255 for opaque */

#define BOARDER_BACKGROUND_COLOR            Q_RGB(0x73, 0x70, 0x6c)     /*Greyish 8a8782 */

#define SCROLLBAR_FRAME_COLOR               Q_RGB(0xdd, 0xdd, 0xdd)     /* Light gray */
#define SCROLLBAR_SLIDER_COLOR              Q_RGB(0xbb, 0xbb, 0xbb)     /* Light gray */
#define SCROLLBAR_SLIDER_SELECTED_COLOR     Q_RGB(0xaa, 0xaa, 0xaa)     /* Light gray */

#define MAX_NUM_OF_THREADS               16
#define LOG_SCRUTINIZER_MAX_SCREEN_ROWS  1024 /* Defines the maximum rows that could be displayed */
#define CFG_MINIMUM_FILE_SIZE_FOR_MULTI_THREAD_TI_PARSE       (10240 * 1024) /* 10MB file at least */
#define CFG_MINIMUM_NUM_OF_TIs_FOR_MULTI_THREAD_FILERING      (10000)

#define RICHEDIT_TO_CLIPBOARD_MAX (500 * 1028)
#define DISPLAY_MAX_ROW_SIZE  1024 /* Limits how long the longest line on screen can be. */
#define FILECTRL_ROW_SIZE_ESTIMATE 512 /* Used by row cache as initial size */
#define FILECTRL_ROW_MAX_SIZE 4096 /* Use when seeking EOL between chunk */
#define CFG_TEMP_STRING_MAX_SIZE (FILECTRL_ROW_MAX_SIZE)
#define CFG_MAX_FILE_NAME_SIZE 4096

#define DEFAULT_MEM_SIZE (1024 * 1000 * 200)    /* 200 MB */
#define DEFAULT_MAX_MEM_SIZE (1024 * 1000 * 600)    /* 600 MB will enable smaller chunks of work */

#define MAX_SEARCH_HISTORY 10

#define CFG_CLIP_NOT_SET  (-1)

#define DEFAULT_TAB_STOP_CHARS (4) /* 4 chars */

#ifdef _WIN32
 #define DLL_EXT   "dll"
#else
 #define DLL_EXT   "so"
#endif

extern int FILECTRL_MINIMAL_NUM_OF_TIs_persistent;
extern int FILECTRL_ROW_SIZE_ESTIMATE_persistent;
extern int FILECTRL_ROW_MAX_SIZE_persistent;
extern CFG_PADDING_t CFG_SCREEN_TEXT_ROW_PADDING_persistent;
extern CFG_PADDING_t CFG_SCREEN_TEXT_WINDOW_PADDING_persistent;
extern const ColorTableItem_t *g_fontColorTable_DEFAULT_p;
extern int g_defaultNumOfFontColorItems;
extern const ColorTableItem_t *g_graphColorTable_DEFAULT_p;
extern int g_defaultNumOfGraphColorItems;

/***********************************************************************************************************************
*   CSCZ_CfgBase
***********************************************************************************************************************/
class CSCZ_CfgBase
{
public:
    CSCZ_CfgBase(QString *tag_p) : m_parseTag(*tag_p) {}
    virtual ~CSCZ_CfgBase() {}

    /***********************************************************************************************************************
    *   WriteToFile
    *   @param textStream_p TODO
    *   @return TODO
    ***********************************************************************************************************************/
    virtual bool WriteToFile(QTextStream *textStream_p) {
        Q_UNUSED(textStream_p);
        return true;
    }

    QString m_parseTag;
};

/***********************************************************************************************************************
*   CSCZ_CfgSettingBase
***********************************************************************************************************************/
class CSCZ_CfgSettingBase : public CSCZ_CfgBase
{
public:
    CSCZ_CfgSettingBase(QString *tag_p, QString *name_p, QString *description_p) : CSCZ_CfgBase(tag_p) {
        m_name = *name_p;
        m_description = *description_p;
    }
    virtual ~CSCZ_CfgSettingBase() {}

    virtual bool WriteToFile(QTextStream *textStream_p) = 0;
    virtual void RestoreDefaultValue(void) = 0;
    virtual bool isDefaultValue(void) = 0;
    virtual bool SetValue(QString *value_p) = 0;

    /*******************************************************************************************************************
    *   valueUpdated
    *   @param value TODO
    *******************************************************************************************************************/
    void valueUpdated(QString *value) {
        TRACEX_I(QString("Setting:%1 -> %2").arg(m_parseTag).arg(*value));
    }

    /*******************************************************************************************************************
    *   isComplex
    *   @return TODO
    *******************************************************************************************************************/
    virtual bool isComplex(void) {
        return false;
    }

    QString m_name;
    QString m_description;
};

/***********************************************************************************************************************
*   CSCZ_CfgSettingColorTable
***********************************************************************************************************************/
class CSCZ_CfgSettingColorTable : public CSCZ_CfgSettingBase
{
public:
    CSCZ_CfgSettingColorTable(
        QString *tag_p,
        QString *name_p,
        QString *description_p,
        const ColorTableItem_t *colorTable_DEFAULT_p,
        const int defaultNumOfColors) : CSCZ_CfgSettingBase(tag_p, name_p, description_p)
    {
        m_colorTable_DEFAULT_p = colorTable_DEFAULT_p;
        m_currentNumColors = defaultNumOfColors;
    }

    virtual ~CSCZ_CfgSettingColorTable();

    virtual bool WriteToFile(QTextStream *textStream_p);
    virtual void RestoreDefaultValue(void);
    virtual bool isDefaultValue(void);
    virtual bool SetValue(QString *value_p);
    virtual bool isComplex(void) {return true;}

    void RestoreColorTable(void);
    void ReplaceColorInTable(int index, Q_COLORREF color, char *name_p);

    /*******************************************************************************************************************
    *   GetTable
    *   @return TODO
    *******************************************************************************************************************/
    ColorTableItem_t *GetTable(void) {
        return m_colorTable;
    }

    /*******************************************************************************************************************
    *   GetCurrentNumberOfColors
    *   @return TODO
    *******************************************************************************************************************/
    int GetCurrentNumberOfColors(void) {
        return m_currentNumColors;
    }

    QList<CSCZ_CfgColorItem *> colorChangeList;
    ColorTableItem_t m_colorTable[MAX_COLOR_TABLE];
    int m_currentNumColors;
    const ColorTableItem_t *m_colorTable_DEFAULT_p;    /* Used for restore */
};

/***********************************************************************************************************************
*   CSCZ_CfgSearchItem
***********************************************************************************************************************/
class CSCZ_CfgSearchItem : public CSCZ_CfgBase
{
public:
    CSCZ_CfgSearchItem(QString *tag_p, QString *name_p) : CSCZ_CfgBase(tag_p) {
        m_name = *name_p;
    }
    ~CSCZ_CfgSearchItem() {}

    virtual bool WriteToFile(QTextStream *textStream_p);

private:
/*  CSCZ_CfgSearchItem() {}; */

    QString m_name;
};

/***********************************************************************************************************************
*   CSCZ_SettingInt
***********************************************************************************************************************/
class CSCZ_SettingInt : public CSCZ_CfgSettingBase
{
public:
    CSCZ_SettingInt(QString *tag_p, QString *identity_p, int *ref_p, int defaultValue, QString *description_p)
        : CSCZ_CfgSettingBase(tag_p, identity_p, description_p)
    {
        m_valueRef_p = ref_p;
        m_defaultValue = defaultValue;
        *ref_p = defaultValue;
    }

    ~CSCZ_SettingInt() {}

    virtual bool WriteToFile(QTextStream *textStream_p);
    virtual void RestoreDefaultValue(void) {*m_valueRef_p = m_defaultValue;}
    virtual bool isDefaultValue(void) {return (*m_valueRef_p == m_defaultValue ? true : false);}
    virtual bool SetValue(QString *value_p);

private:
/*CSCZ_SettingInt() {}; */

    int m_defaultValue;
    int *m_valueRef_p;
};

/***********************************************************************************************************************
*   CSCZ_SettingUInt
***********************************************************************************************************************/
class CSCZ_SettingUInt : public CSCZ_CfgSettingBase
{
public:
    CSCZ_SettingUInt(QString *tag_p,
                     QString *identity_p,
                     int *ref_p,
                     int defaultValue,
                     QString *description_p)
        : CSCZ_CfgSettingBase(tag_p, identity_p, description_p)
    {
        m_valueRef_p = ref_p;
        m_defaultValue = defaultValue;
        *ref_p = defaultValue;
    }

    ~CSCZ_SettingUInt() {}

    virtual bool WriteToFile(QTextStream *textStream_p);

    /*******************************************************************************************************************
    *   RestoreDefaultValue
    *
    *******************************************************************************************************************/
    virtual void RestoreDefaultValue(void) {
        *m_valueRef_p = m_defaultValue;
    }

    /*******************************************************************************************************************
    *   isDefaultValue
    *   @return TODO
    *******************************************************************************************************************/
    virtual bool isDefaultValue(void) {
        return (*m_valueRef_p == m_defaultValue ? true : false);
    }
    virtual bool SetValue(QString *value_p);

private:
    int m_defaultValue;
    int *m_valueRef_p;
};

/***********************************************************************************************************************
*   CSCZ_SettingBool
***********************************************************************************************************************/
class CSCZ_SettingBool : public CSCZ_CfgSettingBase
{
public:
    CSCZ_SettingBool(QString *tag_p, QString *identity_p, bool *ref_p, bool defaultValue, QString *description_p)
        : CSCZ_CfgSettingBase(tag_p, identity_p, description_p)
    {
        m_valueRef_p = ref_p;
        m_defaultValue = defaultValue;
        *ref_p = defaultValue;
    }

    ~CSCZ_SettingBool() {}

    virtual bool WriteToFile(QTextStream *textStream_p);

    /**< RestoreDefaultValue **/
    virtual void RestoreDefaultValue(void) {
        *m_valueRef_p = m_defaultValue;
    }

    /**< isDefaultValue **/
    virtual bool isDefaultValue(void) {
        return (*m_valueRef_p == m_defaultValue ? true : false);
    }
    virtual bool SetValue(QString *value_p);

private:
/*CSCZ_SettingBool() {}; */

    bool m_defaultValue;
    bool *m_valueRef_p;
};

/***********************************************************************************************************************
*   CSCZ_SettingString
***********************************************************************************************************************/
class CSCZ_SettingString : public CSCZ_CfgSettingBase
{
public:
    CSCZ_SettingString(QString *tag_p, QString *identity_p, QString *ref_p, QString defaultValue,
                       QString *description_p)
        : CSCZ_CfgSettingBase(tag_p, identity_p, description_p)
    {
        m_valueRef_p = ref_p;
        m_defaultValue = defaultValue;
        *ref_p = defaultValue;
    }

    ~CSCZ_SettingString() {}

    virtual bool WriteToFile(QTextStream *textStream_p);
    virtual void RestoreDefaultValue(void) {*m_valueRef_p = m_defaultValue;}
    virtual bool isDefaultValue(void) {return (*m_valueRef_p == m_defaultValue ? true : false);}
    virtual bool SetValue(QString *value_p);

private:
    QString m_defaultValue;
    QString *m_valueRef_p;
};

/***********************************************************************************************************************
*   CConfig
***********************************************************************************************************************/
class CConfig /*: public CXMLBase */
{
public:
    CConfig(void);
    ~CConfig(void) {UnloadSettings();}

    void GetAppInfo(QString& title, QString& version, QString& buildDate, QString& configuration);

    void Init(void);
    void readDefaultSettings(void);
    void writeDefaultSettings(void);
    void SetupApplicationVersion(void);

    void ReloadSettings(void);
    void UnloadSettings(void);
    void LoadSystemSettings(void);  /**< This function is run after ReloadSettings, and "tune" parameters from HW and
                                     * Registry */

    bool SetSetting(QString *parseTag_p, QString *value_p);

    bool RegisterSetting(CSCZ_CfgSettingBase *setting_p);

    bool RegisterSetting_INT(QString tag, QString identity, int *ref_p, int defaultValue, QString description);
    bool RegisterSetting_UINT(QString tag, QString identity, int *ref_p, int defaultValue,
                              QString description);
    bool RegisterSetting_BOOL(QString tag, QString identity, bool *ref_p, bool defaultValue, QString description);
    bool RegisterSetting_STRING(QString tag, QString name, QString *ref_p, QString defaultValue, QString description);

    CSCZ_CfgSettingBase *GetSetting(QString *name_p);
    CSCZ_CfgSettingColorTable *RegisterColorTable(QString tag, QString name, QString description,
                                                  const ColorTableItem_t *colorTable_DEFAULT_p,
                                                  const int defaultNumColors);

    void ReplaceColorInTable(char *id_p, int index, Q_COLORREF color, char *name_p);
    bool RemoveSetting(QString *identity_p);
    bool WriteSettings(QFile *file_h, bool onlyChanged = false);

    /****/
    QString GetApplicationVersion(void) {
        return m_applicationVersion;
    }

private:
    QList<CSCZ_CfgSettingBase *> m_settingsList;
    QString m_buildDate;
    QString m_programTitle;
    QString m_configuration;
    QString m_applicationVersion;
    QString m_applicationFileVersion;

public:
    int m_threadPriority;
    int32_t m_numOfThreads; /**< The default number of threads */
    int m_v_scrollSpeed; /**< The vertical scroll speed */
    int m_v_scrollGraphSpeed; /**< The vertical graph scroll speed */
    int m_pluginDebugBitmask; /**< 0 - disabled */
    int m_workMemSize;
    int m_maxWorkMemSize;
    int m_plot_lineEnds; /**< Determine with which line end  to use */
    int m_plot_LineEnds_MinPixelDist; /**< Minimal number of pixels that there must be between line ends */
    int m_plot_LineEnds_MaxCombinePixelDist; /**< Maximal number of pixels between lines that are
                                              *  combined, after this they will be * seperatly drawn */
    int m_plot_TextFontSize; /**< Size of font used for text in  plots */
    int m_plot_TextFontColor; /**< Size of font used for text in plots */
    int m_plot_GraphLineSize; /**< Thinkness of graphs */
    int m_plot_GraphAxisLineSize; /**< Thinkness of graphs axis */
    int m_plot_GraphAxisLineColor; /**< Color of graphs axis */
    int m_plot_GraphCursorLineSize; /**< Thinkness */
    int m_plot_GraphCursorLineColor; /**< Color */
    int m_plot_FocusLineSize;
    int m_plot_FocusLineColor; /**< When a subPlot has focus this color is drawn around the * subPlot */
    int m_plot_PassiveFocusLineColor; /**< When a subPlot has focus, but  not the PlotWnd, this color is
                                       * drawn around the subPlot */
    int m_scrollArrowScale; /**< Used to resize the vertial scroll arrow. 10  -> 0.1 */
    int m_plot_GrayIntensity;
    int m_log_GrayIntensity;
    int m_log_FilterMatchColorModify;
    QString m_default_Font;
    int m_default_tab_stop_char_length = DEFAULT_TAB_STOP_CHARS;
    QString m_fontRes;
    QString m_fontRes_Bold;
    QString m_fontRes_Italic;
    QString m_fontRes_BoldItalic;
    int m_default_FontSize = TEXT_WINDOW_CHAR_SIZE;
    int m_workspace_FontSize = TEXT_WINDOW_CHAR_SIZE;
    int m_copyPaste_FontSize = COPY_PASTE_CHAR_SIZE;
    CSCZ_CfgSettingColorTable *m_font_ColorTable_p;
    CSCZ_CfgSettingColorTable *m_graph_ColorTable_p;
    int m_Log_rowClip_Start;
    int m_Log_rowClip_End;
    int m_Log_colClip_Start;
    int m_Log_colClip_End;
    bool m_logFileTracking = false;
    int m_recentFile_MaxHistory;
    bool m_keepTIA_File;
    QString m_defaultWorkspace; /**< Where to look for the default workspace */
    QString m_defaultRecentFileDB; /**< Where to look for recent file information */
    bool m_rockSrollEnabled;
    bool m_autoCloseProgressDlg;
    bool m_devToolsMenu;
    bool m_throwAtError;
    int m_minNumOfTIs; /**< How many TIs that will be made space for in the TIA */
    CFG_PADDING_t m_text_RowPadding;
    CFG_PADDING_t m_text_WindowPadding;
};

extern CConfig *g_cfg_p;

#endif
