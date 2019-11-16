/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QThread>
#include <QXmlStreamReader>
#include <QFile>
#include <QFileInfo>

#include "CConfig.h"
#include "CDebug.h"
#include "quickFix.h"
#include "globals.h"

CConfig *g_cfg_p;
int FILECTRL_ROW_SIZE_ESTIMATE_persistent = FILECTRL_ROW_SIZE_ESTIMATE;
int FILECTRL_ROW_MAX_SIZE_persistent = FILECTRL_ROW_MAX_SIZE;
int FILECTRL_MINIMAL_NUM_OF_TIs_persistent = 1000; /**< How many TIs that will be made space for in the TIA */
CFG_PADDING_t CFG_SCREEN_TEXT_ROW_PADDING_persistent = {1.0f, 1.0f, 0.05f, 0.05f};
CFG_PADDING_t CFG_SCREEN_TEXT_WINDOW_PADDING_persistent = {1.0f, 1.0f, 1.0f, 1.0f};

/* Color table */
static const ColorTableItem_t fontColorTable_DEFAULT[MAX_COLOR_TABLE] =
{
    {0x000080, "Maroon"},            /* 0 */
    {0x006400, "DarkGreen"},         /* 1 */
    {0x008080, "Olive"},             /* 2 */
    {0x8b0000, "DarkBlue"},          /* 3 */
    {0x800080, "Purple"},            /* 4 */
    {0xd4ff7f, "Aquamarine"},        /* 5 */
    {0x0000ff, "Red"},               /* 6 */
    {0x008000, "Green"},             /* 7 */
    {0x00ffff, "Yellow"},            /* 8 */
    {0xff0000, "Blue"},              /* 9 */
    {0xff00ff, "Magenta"},           /* 10 */
    {0xffff00, "Cyan"},              /* 11 */
    {0xa09e5f, "CadetBlue"},         /* 12 */
    {0x00ff7f, "Chartreuse"},        /* 13 */
    {0x1e69d2, "Chocolate"},         /* 14 */
    {0xed9564, "CornflowerBlue"},    /* 15 */
    {0x3c14dc, "Crimson"},           /* 16 */
    {0x8b8b00, "DarkCyan"},          /* 17 */
    {0x8b008b, "DarkMagenta"},       /* 18 */
    {0x9314ff, "DeepPink"},          /* 19 */
    {0x2222b2, "Firebrick"},         /* 20 */
    {0x20a5da, "Goldenrod"},         /* 21 */
    {0xb469ff, "Hotpink"},           /* 22 */
    {0x82004b, "Indigo"},            /* 23 */
    {0x00ff00, "Lime"},              /* 24 */
    {0x00a5ff, "Orange"},            /* 25 */
    {0x7280fa, "Salmon"},            /* 26 */
    {0x578b2e, "SeaGreen"},          /* 27 */
    {0xcd5a6a, "SlateBlue"},         /* 28 */
    {0x808000, "Teal"},              /* 29 */
    {0xee82ee, "Violet"},            /* 30 */
    {0x000000, "Black"}              /* 31 */
};

#define           DEFAULT_COLOR_TABLE_SIZE  32

const ColorTableItem_t *g_fontColorTable_DEFAULT_p = fontColorTable_DEFAULT;
int g_defaultNumOfFontColorItems = DEFAULT_COLOR_TABLE_SIZE;
static const ColorTableItem_t colorTable_DEFAULT[MAX_COLOR_TABLE] =
{
    {Q_RGB(231, 47, 39), "RED"},           /* 0 */
    {Q_RGB(46, 20, 141), "BLUE"},          /* 1 */
    {Q_RGB(19, 154, 47), "GREEN"},         /* 2 */
    {Q_RGB(178, 137, 166), "PURPLE"},      /* 3 */
    {Q_RGB(238, 113, 25), "ORANGE"},       /* 4 */
    {Q_RGB(160, 160, 160), "GRAY"},        /* 5 */
    {Q_RGB(101, 55, 55), "DARK_RED_GRAY"}, /* 6 */
};

#define DEFAULT_GRAPH_COLOR_TABLE_SIZE 7

const ColorTableItem_t *g_graphColorTable_DEFAULT_p = colorTable_DEFAULT;
int g_defaultNumOfGraphColorItems = DEFAULT_GRAPH_COLOR_TABLE_SIZE;

CConfig::CConfig(void)
{
    m_buildDate = APP_BUILD_VER; /* will cause the built binary to change each time you rebuild. Time macro is not
                                  * reproducable */
    m_programTitle = APP_TITLE;

    QTextStream configStream(&m_configuration);

    m_configuration = "";

#ifdef _WIN32
 #ifdef _WIN64
    configStream << " WIN64";  /* strcat_s(m_configuration, " WIN64 "); */
 #else
    configStream << " WIN32";  /* strcat_s(m_configuration, " WIN32 "); */
 #endif
#else
 #ifdef __x86_64__
    configStream << " 64-bit";  /* strcat_s(m_configuration, " WIN64 "); */
 #else
    configStream << " 32-bit";  /* strcat_s(m_configuration, " WIN32 "); */
 #endif
#endif

#ifdef _DEBUG
    configStream << " DEBUG";  /* strcat_s(m_configuration, " DEBUG "); */
#else
    configStream << " RELEASE";  /* strcat_s(m_configuration, " RELEASE "); */
#endif

    SetupApplicationVersion();  /* Initiates the m_applicationVersion and the  m_applicationFileVersion */

    g_cfg_p = this;

    m_minNumOfTIs = 1000;
    m_text_RowPadding.left = 0.01f;
    m_text_RowPadding.right = 0.01f;
    m_text_RowPadding.top = 0.3f;
    m_text_RowPadding.bottom = 0.3f;
    m_text_WindowPadding.left = 0.005f;
    m_text_WindowPadding.right = 0.005f;
    m_text_WindowPadding.top = 0.005f;
    m_text_WindowPadding.bottom = 0.005f;
    m_autoCloseProgressDlg = true;
    m_throwAtError = false;

    /* DO NOT INITIALIZE ANY OTHER COMPONENT THAT CCONFIG here.... */
#ifdef _DEBUG
    m_devToolsMenu = true;
#else
    m_devToolsMenu = false;
#endif

    m_v_scrollGraphSpeed = 10; /* Value is used as procentage (10%) */
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CConfig::Init(void)
{
    LoadSystemSettings();

    RegisterSetting_INT("NUM_OF_THREADS", "NUM_OF_THREADS", &m_numOfThreads,
                        m_numOfThreads, "Number of threads to use (common value)");

    RegisterSetting_INT("THREAD_PRIO", "THREAD_PRIO", &m_threadPriority,
                        QThread::NormalPriority, "The default thread priority");

    RegisterSetting_INT("WORK_MEM_SIZE", "WORK_MEM_SIZE", &(g_cfg_p->m_workMemSize),
                        0, "Override max used memory when parsing log and filtering");

    RegisterSetting_INT("PLOT_LINE_ENDS", "PLOT_LINE_ENDS", &(g_cfg_p->m_plot_lineEnds),
                        2, "Which line end to use");

    RegisterSetting_INT("PLOT_LINE_END_MIN_PIXEL_DIST", "PLOT_LINE_END_MIN_PIXEL_DIST",
                        &(g_cfg_p->m_plot_LineEnds_MinPixelDist), 2,
                        "Line ends are only drawn if at least these many pixels distance");

    RegisterSetting_INT("PLOT_LINE_COMBINE_MAX_PIXEL_DIST", "PLOT_LINE_COMBINE_MAX_PIXEL_DIST",
                        &(g_cfg_p->m_plot_LineEnds_MaxCombinePixelDist), 5,
                        "Line ends are only combined if closer than these many pixels");

    RegisterSetting_INT("PLOT_TEXT_FONT_SIZE", "PLOT_TEXT_FONT_SIZE", &(g_cfg_p->m_plot_TextFontSize),
                        8, "Font size of text in plots");

    RegisterSetting_INT("PLOT_TEXT_COLOR", "PLOT_TEXT_COLOR", &(g_cfg_p->m_plot_TextFontColor),
                        0, "Color of text in plots");

    RegisterSetting_INT("PLOT_GRAPH_LINE_SIZE", "PLOT_GRAPH_LINE_SIZE", &(g_cfg_p->m_plot_GraphLineSize),
                        1, "Pixel size of the graph lines");

    RegisterSetting_INT("PLOT_GRAPH_LINE_AXIS_SIZE", "PLOT_GRAPH_LINE_AXIS_SIZE",
                        &(g_cfg_p->m_plot_GraphAxisLineSize), 1, "Pixel size of the graph axis lines");

    /* 1 means not set (gray intensity is used instead) */
    RegisterSetting_INT("PLOT_GRAPH_LINE_AXIS_COLOR", "PLOT_GRAPH_LINE_AXIS_COLOR",
                        &(g_cfg_p->m_plot_GraphAxisLineColor), 1, "Color of graph axis lines");

    RegisterSetting_INT("PLOT_GRAPH_CURSOR_LINE_SIZE", "PLOT_GRAPH_CURSOR_LINE_SIZE",
                        &(g_cfg_p->m_plot_GraphCursorLineSize), 1, "Pixel size of the cursor line");

    RegisterSetting_INT("PLOT_GRAPH_CURSOR_LINE_COLOR", "PLOT_GRAPH_CURSOR_LINE_COLOR",
                        &(g_cfg_p->m_plot_GraphCursorLineColor), 0, "Color of the cursor line");

    RegisterSetting_INT("PLOT_FOCUS_LINE_SIZE", "PLOT_FOCUS_LINE_SIZE",
                        &(g_cfg_p->m_plot_FocusLineSize), 3, "Size of the sub-plot highlightning");

    RegisterSetting_INT("PLOT_FOCUS_LINE_COLOR", "PLOT_FOCUS_LINE_COLOR",
                        &(g_cfg_p->m_plot_FocusLineColor), PLOT_FOCUS_LINE_COLOR,
                        "Color of the sub-plot highlightning when having focus");

    RegisterSetting_INT("PLOT_NO_FOCUS_LINE_COLOR", "PLOT_PASSIVE_FOCUS_LINE_COLOR",
                        &(g_cfg_p->m_plot_PassiveFocusLineColor), PLOT_PASSIVE_FOCUS_LINE_COLOR,
                        "Color of the sub-plot highlightning when having passive focus");

    RegisterSetting_INT("PLOT_GRAPH_GRAY_INTENSITY", "PLOT_GRAPH_GRAY_INTENSITY",
                        &(g_cfg_p->m_plot_GrayIntensity), 64, "Intensity of labels and delimiters in plots");

    RegisterSetting_INT("LOG_TEXT_GRAY_INTENSITY", "LOG_TEXT_GRAY_INTENSITY",
                        &(g_cfg_p->m_log_GrayIntensity), FILTERED_TEXT_LTGRAY_COLOR,
                        "Intensity of log text that is filtered out");

    RegisterSetting_INT("LOG_FILTER_MATCH_COLOR_MODIFY", "LOG_FILTER_MATCH_COLOR_MODIFY",
                        &(g_cfg_p->m_log_FilterMatchColorModify), 20,
                        "How much filter matches shall be color modified to be easier spotted");

    RegisterSetting_INT("SCROLL_ARROW_ICON_SCALE", "SCROLL_ARROW_ICON_SCALE",
                        &(g_cfg_p->m_scrollArrowScale), DEFAULT_SCROLL_ARROW_SCALE,
                        "How much to scale the scroll icon, in parts of 100. 40 means 40% of original");

    RegisterSetting_INT("TAB_STOP_LENGTH_NUM_CHAR", "TAB_STOP_LENGTH_NUM_CHAR",
                        &(g_cfg_p->m_default_tab_stop_char_length), DEFAULT_TAB_STOP_CHARS,
                        "The size in chars between the text TAB stops");

    /*"Consolas",  "Default font, use monospace fonts only"); */
    RegisterSetting_STRING("DEFAULT_FONT", "DEFAULT_FONT", &(g_cfg_p->m_default_Font),
                           "Courier New", "Default font, use monospace fonts only");

    RegisterSetting_INT("DEFAULT_FONT_SIZE", "DEFAULT_FONT_SIZE", &(g_cfg_p->m_default_FontSize),
                        TEXT_WINDOW_CHAR_SIZE, "The default font size");

    RegisterSetting_INT("WORKSPACE_FONT_SIZE", "WORKSPACE_FONT_SIZE", &(g_cfg_p->m_workspace_FontSize),
                        WORKSPACE_TEXT_WINDOW_CHAR_SIZE, "The default workspace font size");

    RegisterSetting_INT("COPY_PASTE_FONT_SIZE", "COPY_PASTE_FONT_SIZE", &(g_cfg_p->m_copyPaste_FontSize),
                        COPY_PASTE_CHAR_SIZE, "The font sized used when formatting Rich to clipboard");

    RegisterSetting_INT("LOG_ROW_CLIP_START", "LOG_ROW_CLIP_START", &(g_cfg_p->m_Log_rowClip_Start),
                        -1, "Clips the start of the log");

    RegisterSetting_INT("LOG_ROW_CLIP_END", "LOG_ROW_CLIP_END", &(g_cfg_p->m_Log_rowClip_End),
                        -1, "Clips the end of the log");

    RegisterSetting_INT("LOG_COL_CLIP_START", "LOG_COL_CLIP_START", &(g_cfg_p->m_Log_colClip_Start),
                        -1, "Clips the start of the row");
    RegisterSetting_INT("LOG_COL_CLIP_END", "LOG_COL_CLIP_END", &(g_cfg_p->m_Log_colClip_End),
                        -1, "Clips the end of the row");

    RegisterSetting_INT("RECENT_FILE_MAX_HISTORY", "RECENT_FILE_MAX_HISTORY",
                        &(g_cfg_p->m_recentFile_MaxHistory), MAX_NUM_OF_RECENT_FILES,
                        "Number of recent files used to remeber");

    RegisterSetting_STRING("DEFAULT_WORKSPACE", "DEFAULT_WORKSPACE", &(g_cfg_p->m_defaultWorkspace),
                           "default.lsz", "Default workspace, loaded at startup");

    RegisterSetting_STRING("DEFAULT_RECENT_FILE", "DEFAULT_RECENT_FILE", &(g_cfg_p->m_defaultRecentFileDB),
                           "recentFiles.rcnt", "File database of recent files");

    RegisterSetting_BOOL("KEEP_TIA_FILE", "KEEP_TIA_FILE", &(g_cfg_p->m_keepTIA_File), true,
                         "If TIA files should be deleted or not");

    RegisterSetting_BOOL("ROCK_SCROLL_ENABLED", "ROCK_SCROLL_ENABLED", &(g_cfg_p->m_rockSrollEnabled), true,
                         "If rock scroll should be enabled");

    RegisterSetting_INT("LOG_V_SCROLL_SPEED", "LOG_V_SCROLL_SPEED", &(g_cfg_p->m_v_scrollSpeed), m_v_scrollSpeed,
                        "The number of lines to scroll when using mouse wheel");

    RegisterSetting_INT("PLOT_V_SCROLL_SPEED", "PLOT_V_SCROLL_SPEED", &(g_cfg_p->m_v_scrollGraphSpeed),
                        m_v_scrollGraphSpeed, "Procentage of subplot to scroll when using mouse wheel");

    RegisterSetting_UINT("PLUGIN_DEBUG_BITMASK", "PLUGIN_DEBUG_BITMASK", &(g_cfg_p->m_pluginDebugBitmask),
                         0, "Enables more information about plugin plots and decoders");

    RegisterSetting_BOOL("AUTO_CLOSE_PROGRESS_DLG", "AUTO_CLOSE_PROGRESS_DLG", &(g_cfg_p->m_autoCloseProgressDlg),
                         true, "Automatically close progress dlg when processing is done");

    RegisterSetting_BOOL("DEV_TOOLS_ENABLED", "DEV_TOOLS_ENABLED", &(g_cfg_p->m_devToolsMenu),
                         g_cfg_p->m_devToolsMenu, "Enable the developer tools menu");

    RegisterSetting_BOOL("DUMP_AT_ERROR", "DUMP_AT_ERROR", &(g_cfg_p->m_throwAtError),
                         g_cfg_p->m_throwAtError, "Dump if a critical error was detected");

    RegisterSetting_INT("LOG_LEVEL", "LOG_LEVEL", &(g_DebugLib->m_logLevel), g_DebugLib->m_logLevel,
                        "Defines what prints that should be present in the log");

    RegisterSetting_INT("TRACE_CATEGORY", "TRACE_CATEGORY", &(g_DebugLib->m_traceCategory),
                        g_DebugLib->m_traceCategory,
                        "Enable debugging of certain categories (0 None, 1 Focus, 2 Threads)");

    m_font_ColorTable_p = RegisterColorTable("FONT_COLOR_TABLE", "FONT_COLOR_TABLE", "Colors for filters",
                                             g_fontColorTable_DEFAULT_p, g_defaultNumOfFontColorItems);
    m_graph_ColorTable_p = RegisterColorTable("GRAPH_COLOR_TABLE", "GRAPH_COLOR_TABLE", "Colors for graphs",
                                              g_graphColorTable_DEFAULT_p, g_defaultNumOfGraphColorItems);

    m_font_ColorTable_p->RestoreColorTable();
    m_graph_ColorTable_p->RestoreColorTable();
}

/***********************************************************************************************************************
*   readDefaultSettings
***********************************************************************************************************************/
void CConfig::readDefaultSettings(void)
{
    QFile xmlFile("settings.xml");

    if (xmlFile.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xmlReader(&xmlFile);

        TRACEX_I(QString("Settings file read: %1").arg(QFileInfo(xmlFile).absoluteFilePath()))
        while (!xmlReader.atEnd() && xmlReader.readNext()) {
            if (xmlReader.isStartElement() && xmlReader.name().contains("setting")) {
                auto attributes = xmlReader.attributes();

                for (auto& setting : m_settingsList) {
                    /* Loop through all defined settings and try to match */
                    if (attributes.hasAttribute(setting->m_parseTag)) {
                        auto dummy = attributes.value(setting->m_parseTag).toString();

                        setting->SetValue(&dummy);
                    }
                }
            }
        }
    }
}

/***********************************************************************************************************************
*   writeDefaultSettings
***********************************************************************************************************************/
void CConfig::writeDefaultSettings(void)
{
    QFile xmlFile("settings.xml");

    if (xmlFile.open(QIODevice::WriteOnly)) {
        TRACEX_I(QString("Settings file write: %1").arg(QFileInfo(xmlFile).absoluteFilePath()))

        QTextStream fileStream(&xmlFile);

        fileStream << XML_FILE_HEADER << endl;
        fileStream << LCZ_HEADER << endl;
        WriteSettings(&xmlFile, true);
        fileStream << LCZ_FOOTER << endl;
        fileStream.flush();
    }
}

/***********************************************************************************************************************
*   SetupApplicationVersion
***********************************************************************************************************************/
void CConfig::SetupApplicationVersion(void)
{
    QTextStream appVerStream(&m_applicationVersion);

    appVerStream << "v" << APP_VER_CODE_MAJOR << "." << APP_VER_CODE_MINOR;

    switch (APP_VER_CODE_KIND)
    {
        case APP_VER_CODE_KIND_BETA:
            appVerStream << " B" << APP_VER_CODE_SUB_VERSION;
            break;

        case APP_VER_CODE_KIND_PATCH:
            appVerStream << "." << APP_VER_CODE_SUB_VERSION;
            break;

        case APP_VER_CODE_KIND_RC:
            appVerStream << " RC" << APP_VER_CODE_SUB_VERSION;
            break;

        case APP_VER_CODE_KIND_FINAL:
        default:
            break;
    }

#if APP_VER_CODE_BUILD > 0
    appVerStream << " build" << APP_VER_CODE_BUILD;
#endif

    appVerStream << m_configuration;

    m_applicationFileVersion = m_applicationVersion;
    m_applicationFileVersion = m_applicationFileVersion.replace(' ', "_");
}

/***********************************************************************************************************************
*   WriteSettings
***********************************************************************************************************************/
bool CConfig::WriteSettings(QFile *file_h, bool onlyChanged)
{
    if (!m_settingsList.isEmpty()) {
        QTextStream settingsStream(file_h);

        try {
            settingsStream << SETTINGS_HEADER << "\n";
        } catch (std::exception &e) {
            qFatal("Error %s writing settings header to file", e.what());
            TRACEX_E("CConfig::WriteSettings  Failed to write setting header to settings file,  Err:%s",
                     e.what());
            return false;
        } catch (...) {
            qFatal("Unknown Error writting settings to file");
            TRACEX_E("CConfig::WriteSettings  Failed to write setting header to settings file,  Unknown error")
            return false;
        }

        CSCZ_CfgSettingBase *setting_p = nullptr;

        for (int index = 0; index < m_settingsList.count(); ++index) {
            setting_p = m_settingsList[index];

            if (!onlyChanged || !setting_p->isDefaultValue()) {
                try {
                    setting_p->WriteToFile(&settingsStream);
                } catch (std::exception &e) {
                    qFatal("Error %s writing settings to file", e.what());
                    TRACEX_E("CConfig::WriteSettings  Failed to write settings to file,  Err:%s",
                             e.what());
                    return false;
                } catch (...) {
                    qFatal("Unknown Error writting settings to file");
                    TRACEX_E("CConfig::WriteSettings  Failed to write settings to file,  Unknown error")
                    return false;
                }
            }
        }

        try {
            settingsStream << SETTINGS_FOOTER << "\n";
        } catch (std::exception &e) {
            qFatal("Error %s writing settings footer to file", e.what());
            TRACEX_E("CConfig::WriteSettings  Failed to write setting footer to settings file,  Err:%s",
                     e.what());
            return false;
        } catch (...) {
            qFatal("Unknown Error writting settings to file");
            TRACEX_E("CConfig::WriteSettings  Failed to write setting footer to settings file,  Unknown error")
            return false;
        }

        settingsStream.flush();
    }

    return true;
}

/***********************************************************************************************************************
*   UnloadSettings
***********************************************************************************************************************/
void CConfig::UnloadSettings(void)
{
    while (!m_settingsList.isEmpty()) {
        delete m_settingsList.takeFirst();
    }
}

/***********************************************************************************************************************
*   ReloadSettings
***********************************************************************************************************************/
void CConfig::ReloadSettings(void)
{
    UnloadSettings();
    Init();
}

/***********************************************************************************************************************
*   LoadSystemSettings
***********************************************************************************************************************/
void CConfig::LoadSystemSettings(void)
{
    m_numOfThreads = QThread::idealThreadCount();

    /* Limit number of threads to the define... typically debugging */
    m_numOfThreads = m_numOfThreads > MAX_NUM_OF_THREADS ? MAX_NUM_OF_THREADS : m_numOfThreads;

    m_v_scrollSpeed = 3;      /* xxx QT   set to this temp */
}

/***********************************************************************************************************************
*   GetAppInfo
***********************************************************************************************************************/
void CConfig::GetAppInfo(QString& title, QString& version, QString& buildDate, QString& configuration)
{
    title = m_programTitle;
    version = m_applicationVersion;
    buildDate = m_buildDate;
    configuration = m_configuration;
}

/***********************************************************************************************************************
*   RegisterSetting
***********************************************************************************************************************/
bool CConfig::RegisterSetting(CSCZ_CfgSettingBase *setting_p)
{
    TRACEX_D(QString("RegisterSetting    Name:%1  ParseTag:%2  Description:%3")
                 .arg(setting_p->m_name).arg(setting_p->m_parseTag).arg(setting_p->m_description));
    m_settingsList.append(setting_p);
    setting_p->RestoreDefaultValue();
    return true;
}

/***********************************************************************************************************************
*   RegisterSetting_INT
***********************************************************************************************************************/
bool CConfig::RegisterSetting_INT(QString tag, QString name, int *ref_p, int defaultValue, QString description)
{
    CSCZ_SettingInt *setting_p = new CSCZ_SettingInt(&tag, &name, ref_p, defaultValue, &description);

    RegisterSetting(setting_p);
    return true;
}

/***********************************************************************************************************************
*   RegisterSetting_UINT
***********************************************************************************************************************/
bool CConfig::RegisterSetting_UINT(QString tag, QString name, int *ref_p,
                                   int defaultValue, QString description)
{
    CSCZ_SettingUInt *setting_p = new CSCZ_SettingUInt(&tag, &name, ref_p, defaultValue, &description);

    RegisterSetting(setting_p);
    return true;
}

/***********************************************************************************************************************
*   RegisterSetting_BOOL
***********************************************************************************************************************/
bool CConfig::RegisterSetting_BOOL(QString tag, QString name, bool *ref_p, bool defaultValue, QString description)
{
    CSCZ_SettingBool *setting_p = new CSCZ_SettingBool(&tag, &name, ref_p, defaultValue, &description);

    RegisterSetting(setting_p);
    return true;
}

/***********************************************************************************************************************
*   RegisterSetting_STRING
***********************************************************************************************************************/
bool CConfig::RegisterSetting_STRING(QString tag, QString name, QString *ref_p,
                                     QString defaultValue, QString description)
{
    CSCZ_SettingString *setting_p = new CSCZ_SettingString(&tag, &name, ref_p, defaultValue, &description);

    RegisterSetting(setting_p);
    return true;
}

/***********************************************************************************************************************
*   GetSetting
***********************************************************************************************************************/
CSCZ_CfgSettingBase *CConfig::GetSetting(QString *name_p)
{
    if (m_settingsList.isEmpty()) {
        return nullptr;
    }

    for (int index = 0; index < m_settingsList.count(); ++index) {
        if (m_settingsList[index]->m_name == *name_p) {
            return m_settingsList[index];
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   RegisterColorTable
***********************************************************************************************************************/
CSCZ_CfgSettingColorTable *CConfig::RegisterColorTable(
    QString tag, QString name, QString description, const ColorTableItem_t *colorTable_DEFAULT_p,
    const int defaultNumColors)
{
    CSCZ_CfgSettingColorTable *cfgSettingColorTable_p =
        new CSCZ_CfgSettingColorTable(&tag, &name, &description, colorTable_DEFAULT_p, defaultNumColors);

    RegisterSetting(cfgSettingColorTable_p);

    return cfgSettingColorTable_p;
}

/***********************************************************************************************************************
*   ReplaceColorInTable
***********************************************************************************************************************/
void CConfig::ReplaceColorInTable(char *id_p, int index, Q_COLORREF color, char *name_p)
{
    QString id = id_p;
    CSCZ_CfgSettingColorTable *colorTable_p = reinterpret_cast<CSCZ_CfgSettingColorTable *>(GetSetting(&id));

    if (colorTable_p != nullptr) {
        colorTable_p->ReplaceColorInTable(index, color, name_p);
    }
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_CfgSettingColorTable::WriteToFile(QTextStream *textStream_p)
{
    for (int index = 0; index < colorChangeList.count(); ++index) {
        CSCZ_CfgColorItem *item_p = colorChangeList[index];

        *textStream_p << SETTING_HEADER
                      << " color=\"0x" << (hex) << item_p->m_colorTableItem.color
                      << "\" id=\"" << m_parseTag
                      << "\" name=\"" << item_p->m_colorTableItem.name
                      << "\" index=\"" << (dec) << item_p->m_index
                      << "\""
                      << SETTING_FOOTER << endl;
    }

    return true;
}

/***********************************************************************************************************************
*   RestoreDefaultValue
***********************************************************************************************************************/
void CSCZ_CfgSettingColorTable::RestoreDefaultValue(void)
{
    memcpy(m_colorTable, m_colorTable_DEFAULT_p, sizeof(ColorTableItem_t) * MAX_COLOR_TABLE);
}

/***********************************************************************************************************************
*   isDefaultValue
***********************************************************************************************************************/
bool CSCZ_CfgSettingColorTable::isDefaultValue(void)
{
    if (colorChangeList.isEmpty()) {
        return true;
    }

    return false;
}

/***********************************************************************************************************************
*   SetValue
***********************************************************************************************************************/
bool CSCZ_CfgSettingColorTable::SetValue(QString *value_p)
{
    Q_UNUSED(value_p);
    return false;
}

/***********************************************************************************************************************
*   RemoveSetting
***********************************************************************************************************************/
bool CConfig::RemoveSetting(QString *identity_p)
{
    if ((identity_p != nullptr) && !m_settingsList.isEmpty()) {
        CSCZ_CfgSettingBase *setting_p = nullptr;

        for (int index = 0; index < m_settingsList.count(); ++index) {
            setting_p = m_settingsList[index];

            if (setting_p->m_name == *identity_p) {
                TRACEX_I(QString("RemoveSetting    %1  %1").arg(setting_p->m_name).arg(setting_p->m_parseTag))
                setting_p->RestoreDefaultValue();
                m_settingsList.removeAt(index);
                delete (setting_p);
                return true;
            }

            TRACEX_W("CConfig::RemoveSetting    Failed to find  %s", identity_p->toLatin1().constData())
        }
    } else {
        TRACEX_W("CConfig::RemoveSetting    parameter error")
    }
    return false;
}

/***********************************************************************************************************************
*   SetSetting
***********************************************************************************************************************/
bool CConfig::SetSetting(QString *parseTag_p, QString *value_p)
{
    if ((parseTag_p != nullptr) && !m_settingsList.isEmpty()) {
        CSCZ_CfgSettingBase *setting_p = nullptr;

        for (int index = 0; index < m_settingsList.count(); ++index) {
            setting_p = m_settingsList[index];

            if (setting_p->m_parseTag == *parseTag_p) {
                TRACEX_I(QString("SetSetting    %1  %2")
                             .arg(setting_p->m_name).arg(setting_p->m_parseTag));
                setting_p->SetValue(value_p);
                return true;
            }
        }
        TRACEX_W("CSCZ_CfgFilter::SetSetting    Failed to find  %s",
                 parseTag_p->toLatin1().constData());
    }
    TRACEX_W("CSCZ_CfgFilter::SetSetting    Failed to find  setting")
    return false;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_CfgSearchItem::WriteToFile(QTextStream *textStream_p)
{
    if (!m_name.isEmpty()) {
        *textStream_p << SEARCH_ITEM_HEADER << " " << " name=\"" << m_name << "\"" << SEARCH_ITEM_FOOTER << endl;
        return true;
    }

    return false;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_SettingInt::WriteToFile(QTextStream *textStream_p)
{
    *textStream_p << SETTING_HEADER << " " << m_parseTag << "=\"" << *m_valueRef_p << "\"" << SETTING_FOOTER << endl;
    return true;
}

/***********************************************************************************************************************
*   SetValue
***********************************************************************************************************************/
bool CSCZ_SettingInt::SetValue(QString *value_p)
{
    *m_valueRef_p = static_cast<int>(strtol(value_p->toLatin1().constData(), nullptr, 10));
    valueUpdated(value_p);
    return true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_SettingUInt::WriteToFile(QTextStream *textStream_p)
{
    *textStream_p << SETTING_HEADER << " " << m_parseTag << "=\"" << *m_valueRef_p << "\"" << SETTING_FOOTER << endl;
    return true;
}

/***********************************************************************************************************************
*   SetValue
***********************************************************************************************************************/
bool CSCZ_SettingUInt::SetValue(QString *value_p)
{
    *m_valueRef_p = static_cast<int>(strtol(value_p->toLatin1().constData(), nullptr, 10));
    valueUpdated(value_p);
    return true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_SettingBool::WriteToFile(QTextStream *textStream_p)
{
    *textStream_p << SETTING_HEADER << " " << m_parseTag << "=\"" << (*m_valueRef_p == true ? "y" : "n") << "\""
                  << SETTING_FOOTER << endl;
    return true;
}

/***********************************************************************************************************************
*   SetValue
***********************************************************************************************************************/
bool CSCZ_SettingBool::SetValue(QString *value_p)
{
    *value_p = value_p->toLower();

    if ((*value_p == "true") || (*value_p == "1") || (*value_p == "y")) {
        *m_valueRef_p = true;
    } else {
        *m_valueRef_p = false;
    }

    valueUpdated(value_p);
    return true;
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
bool CSCZ_SettingString::WriteToFile(QTextStream *textStream_p)
{
    *textStream_p << SETTING_HEADER << " " << m_parseTag << "=\"" << *m_valueRef_p << "\"" << SETTING_FOOTER << endl;
    return true;
}

/***********************************************************************************************************************
*   SetValue
***********************************************************************************************************************/
bool CSCZ_SettingString::SetValue(QString *value_p)
{
    *m_valueRef_p = *value_p;
    valueUpdated(value_p);
    return true;
}

CSCZ_CfgSettingColorTable::~CSCZ_CfgSettingColorTable()
{
    while (!colorChangeList.isEmpty()) {
        delete colorChangeList.takeLast();
    }
}

/***********************************************************************************************************************
*   ReplaceColorInTable
***********************************************************************************************************************/
void CSCZ_CfgSettingColorTable::ReplaceColorInTable(int index, Q_COLORREF color, char *name_p)
{
    CSCZ_CfgColorItem *foundItem_p = nullptr;

    if (!colorChangeList.isEmpty()) {
        for (int index = 0; index < colorChangeList.count() && foundItem_p == nullptr; ++index) {
            if (colorChangeList[index]->m_index == index) {
                foundItem_p = colorChangeList[index];
            }
        }
    }

    if (foundItem_p == nullptr) {
        foundItem_p = new CSCZ_CfgColorItem();
        colorChangeList.append(foundItem_p);
    }

    foundItem_p->m_index = index;
    foundItem_p->m_colorTableItem.color = color;

    strcpy(foundItem_p->m_colorTableItem.name, name_p);

    /* replace the color in the used color table */

    if ((index == 256) || ((index < MAX_COLOR_TABLE) && (index >= m_currentNumColors))) {
        /* Add a new color to the end */
        index = m_currentNumColors;
        ++(m_currentNumColors);
        TRACEX_I("ReplaceColor  ADD Index:%d", index)
    } else if (index > MAX_COLOR_TABLE) {
        TRACEX_E("ReplaceColor  Index:%d to large", index)
        return;
    } else {
        /* Replace an existing color */
        TRACEX_I("ReplaceColor  Index:%d Color:%d name:%s ", index, color, name_p)
    }

    if (color > 0xffffff) {
        TRACEX_E("ReplaceColor  Color:%d invalid", color)
        return;
    }

    if (index < MAX_COLOR_TABLE) {
        m_colorTable[index].color = color;
        strcpy(m_colorTable[index].name, name_p);
    } else {
        TRACEX_W("ReplaceColorInTable  index error")
    }
}

/***********************************************************************************************************************
*   RestoreColorTable
***********************************************************************************************************************/
void CSCZ_CfgSettingColorTable::RestoreColorTable(void)
{
    memcpy(m_colorTable, m_colorTable_DEFAULT_p, sizeof(m_colorTable));
}
