/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
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
CFG_PADDING_t CFG_SCREEN_TEXT_ROW_PADDING_persistent = {1.0, 1.0, 0.05, 0.05};
CFG_PADDING_t CFG_SCREEN_TEXT_WINDOW_PADDING_persistent = {1.0, 1.0, 1.0, 1.0};

/* Color table */
static const std::vector<ColorTableItem_t> g_fontColorTable =
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

static const std::vector<ColorTableItem_t> g_colorTable =
{
    {Q_RGB(231, 47, 39), "RED"},           /* 0 */
    {Q_RGB(46, 20, 141), "BLUE"},          /* 1 */
    {Q_RGB(19, 154, 47), "GREEN"},         /* 2 */
    {Q_RGB(178, 137, 166), "PURPLE"},      /* 3 */
    {Q_RGB(238, 113, 25), "ORANGE"},       /* 4 */
    {Q_RGB(160, 160, 160), "GRAY"},        /* 5 */
    {Q_RGB(101, 55, 55), "DARK_RED_GRAY"}, /* 6 */
};

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/

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
    m_text_RowPadding.left = 0.01;
    m_text_RowPadding.right = 0.01;
    m_text_RowPadding.top = 0.3;
    m_text_RowPadding.bottom = 0.3;
    m_text_WindowPadding.left = 0.005;
    m_text_WindowPadding.right = 0.005;
    m_text_WindowPadding.top = 0.005;
    m_text_WindowPadding.bottom = 0.005;
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

/***/
template <>
int CSCZ_CfgT<int>::toValue(const QString *str, bool& ok)
{
    auto value = str->toInt(&ok);
    if (ok) {
        return value;
    }
    return *m_valueRef_p; /* fallback */
}

/***/
template <>
double CSCZ_CfgT<double>::toValue(const QString *str, bool& ok)
{
    auto value = str->toDouble(&ok);
    if (ok) {
        return value;
    }
    return *m_valueRef_p; /* fallback */
}

/***/
template <>
bool CSCZ_CfgT<bool>::toValue(const QString *str, bool& ok)
{
    ok = true;

    const QString& s = *str;
    if ((s.compare("true", Qt::CaseInsensitive) == 0) || (s == "1") || (s.compare("yes", Qt::CaseInsensitive)) ||
        (s.compare("y", Qt::CaseInsensitive) == 0)) {
        return true;
    } else if ((s.compare("false", Qt::CaseInsensitive) == 0) || (s.compare("no", Qt::CaseInsensitive)) ||
               (s == "0") || (s.compare("n", Qt::CaseInsensitive) == 0)) {
        return false;
    } else {
        ok = false;
    }
    return *m_valueRef_p; /* fallback */
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CConfig::Init(void)
{
    LoadSystemSettings();

    RegisterSetting(new CSCZ_CfgT<int>("NUM_OF_THREADS", "NUM_OF_THREADS", &m_numOfThreads,
                                       m_numOfThreads, "Number of threads to use (common value)"));

    RegisterSetting(new CSCZ_CfgT<int>("THREAD_PRIO", "THREAD_PRIO", &m_threadPriority,
                                       QThread::NormalPriority, "The default thread priority"));

    RegisterSetting(new CSCZ_CfgT<int>("WORK_MEM_SIZE", "WORK_MEM_SIZE", &(g_cfg_p->m_workMemSize),
                                       0, "Override max used memory when parsing log and filtering"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_LINE_ENDS", "PLOT_LINE_ENDS", &(g_cfg_p->m_plot_lineEnds),
                                       2, "Which line end to use"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_LINE_END_MIN_PIXEL_DIST", "PLOT_LINE_END_MIN_PIXEL_DIST",
                                       &(g_cfg_p->m_plot_LineEnds_MinPixelDist), 2,
                                       "Line ends are only drawn if at least these many pixels distance"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_LINE_COMBINE_MAX_PIXEL_DIST", "PLOT_LINE_COMBINE_MAX_PIXEL_DIST",
                                       &(g_cfg_p->m_plot_LineEnds_MaxCombinePixelDist), 5,
                                       "Line ends are only combined if closer than these many pixels"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_TEXT_FONT_SIZE", "PLOT_TEXT_FONT_SIZE", &(g_cfg_p->m_plot_TextFontSize),
                                       8, "Font size of text in plots"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_TEXT_COLOR", "PLOT_TEXT_COLOR", &(g_cfg_p->m_plot_TextFontColor),
                                       0, "Color of text in plots"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_LINE_SIZE", "PLOT_GRAPH_LINE_SIZE", &(g_cfg_p->m_plot_GraphLineSize),
                                       1, "Pixel size of the graph lines"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_LINE_AXIS_SIZE", "PLOT_GRAPH_LINE_AXIS_SIZE",
                                       &(g_cfg_p->m_plot_GraphAxisLineSize), 1, "Pixel size of the graph axis lines"));

    /* 1 means not set (gray intensity is used instead) */
    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_LINE_AXIS_COLOR", "PLOT_GRAPH_LINE_AXIS_COLOR",
                                       &(g_cfg_p->m_plot_GraphAxisLineColor), 1, "Color of graph axis lines"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_CURSOR_LINE_SIZE", "PLOT_GRAPH_CURSOR_LINE_SIZE",
                                       &(g_cfg_p->m_plot_GraphCursorLineSize), 1, "Pixel size of the cursor line"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_CURSOR_LINE_COLOR", "PLOT_GRAPH_CURSOR_LINE_COLOR",
                                       &(g_cfg_p->m_plot_GraphCursorLineColor), 0, "Color of the cursor line"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_FOCUS_LINE_SIZE", "PLOT_FOCUS_LINE_SIZE",
                                       &(g_cfg_p->m_plot_FocusLineSize), 3, "Size of the sub-plot highlightning"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_FOCUS_LINE_COLOR", "PLOT_FOCUS_LINE_COLOR",
                                       &(g_cfg_p->m_plot_FocusLineColor), PLOT_FOCUS_LINE_COLOR,
                                       "Color of the sub-plot highlightning when having focus"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_NO_FOCUS_LINE_COLOR", "PLOT_PASSIVE_FOCUS_LINE_COLOR",
                                       &(g_cfg_p->m_plot_PassiveFocusLineColor), PLOT_PASSIVE_FOCUS_LINE_COLOR,
                                       "Color of the sub-plot highlightning when having passive focus"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_GRAPH_GRAY_INTENSITY", "PLOT_GRAPH_GRAY_INTENSITY",
                                       &(g_cfg_p->m_plot_GrayIntensity), 64,
                                       "Intensity of labels and delimiters in plots"));

    RegisterSetting(new CSCZ_CfgT<int>("LOG_TEXT_GRAY_INTENSITY", "LOG_TEXT_GRAY_INTENSITY",
                                       &(g_cfg_p->m_log_GrayIntensity), FILTERED_TEXT_LTGRAY_COLOR,
                                       "Intensity of log text that is filtered out"));

    RegisterSetting(new CSCZ_CfgT<int>("LOG_FILTER_MATCH_COLOR_MODIFY", "LOG_FILTER_MATCH_COLOR_MODIFY",
                                       &(g_cfg_p->m_log_FilterMatchColorModify), 20,
                                       "How much filter matches shall be color modified to be easier spotted"));

    RegisterSetting(new CSCZ_CfgT<int>("SCROLL_ARROW_ICON_SCALE", "SCROLL_ARROW_ICON_SCALE",
                                       &(g_cfg_p->m_scrollArrowScale), DEFAULT_SCROLL_ARROW_SCALE,
                                       "How much to scale the scroll icon, in parts of 100. 40 means 40% of original"));

    RegisterSetting(new CSCZ_CfgT<int>("TAB_STOP_LENGTH_NUM_CHAR", "TAB_STOP_LENGTH_NUM_CHAR",
                                       &(g_cfg_p->m_default_tab_stop_char_length), DEFAULT_TAB_STOP_CHARS,
                                       "The size in chars between the text TAB stops"));

    /*"Consolas",  "Default font, use monospace fonts only"); */
    RegisterSetting(new CSCZ_CfgT<QString>("DEFAULT_FONT", "DEFAULT_FONT", &(g_cfg_p->m_default_Font),
                                           "Courier New", "Default font, use monospace fonts only"));

    RegisterSetting(new CSCZ_CfgT<int>("DEFAULT_FONT_SIZE", "DEFAULT_FONT_SIZE", &(g_cfg_p->m_default_FontSize),
                                       TEXT_WINDOW_CHAR_SIZE, "The default font size"));

    RegisterSetting(new CSCZ_CfgT<int>("WORKSPACE_FONT_SIZE", "WORKSPACE_FONT_SIZE", &(g_cfg_p->m_workspace_FontSize),
                                       WORKSPACE_TEXT_WINDOW_CHAR_SIZE, "The default workspace font size"));

    RegisterSetting(new CSCZ_CfgT<int>("COPY_PASTE_FONT_SIZE", "COPY_PASTE_FONT_SIZE", &(g_cfg_p->m_copyPaste_FontSize),
                                       COPY_PASTE_CHAR_SIZE, "The font sized used when formatting Rich to clipboard"));

    RegisterSetting(new CSCZ_CfgT<int>("LOG_ROW_CLIP_START", "LOG_ROW_CLIP_START", &(g_cfg_p->m_Log_rowClip_Start),
                                       -1, "Clips the start of the log", SettingScope_t::workspace));
    RegisterSetting(new CSCZ_CfgT<int>("LOG_ROW_CLIP_START", "LOG_ROW_CLIP_START", &(g_cfg_p->m_Log_rowClip_Start),
                                       -1, "Clips the start of the log", SettingScope_t::workspace));
    RegisterSetting(new CSCZ_CfgT<int>("LOG_ROW_CLIP_END", "LOG_ROW_CLIP_END", &(g_cfg_p->m_Log_rowClip_End),
                                       -1, "Clips the end of the log", SettingScope_t::workspace));
    RegisterSetting(new CSCZ_CfgT<int>("LOG_COL_CLIP_START", "LOG_COL_CLIP_START", &(g_cfg_p->m_Log_colClip_Start),
                                       -1, "Clips the start of the row", SettingScope_t::workspace));
    RegisterSetting(new CSCZ_CfgT<int>("LOG_COL_CLIP_END", "LOG_COL_CLIP_END", &(g_cfg_p->m_Log_colClip_End),
                                       -1, "Clips the end of the row", SettingScope_t::workspace));

    RegisterSetting(new CSCZ_CfgT<int>("RECENT_FILE_MAX_HISTORY", "RECENT_FILE_MAX_HISTORY",
                                       &(g_cfg_p->m_recentFile_MaxHistory), MAX_NUM_OF_RECENT_FILES,
                                       "Number of recent files used to remeber"));

    RegisterSetting(new CSCZ_CfgT<QString>("DEFAULT_WORKSPACE", "DEFAULT_WORKSPACE", &(g_cfg_p->m_defaultWorkspace),
                                           "default.lsz", "Default workspace, loaded at startup"));

    RegisterSetting(new CSCZ_CfgT<QString>("DEFAULT_RECENT_FILE", "DEFAULT_RECENT_FILE",
                                           &(g_cfg_p->m_defaultRecentFileDB),
                                           "recentFiles.rcnt", "File database of recent files"));

    RegisterSetting(new CSCZ_CfgT<bool>("KEEP_TIA_FILE", "KEEP_TIA_FILE", &(g_cfg_p->m_keepTIA_File), true,
                                        "If TIA files should be deleted or not"));

    RegisterSetting(new CSCZ_CfgT<bool>("ROCK_SCROLL_ENABLED", "ROCK_SCROLL_ENABLED", &(g_cfg_p->m_rockSrollEnabled),
                                        true, "If rock scroll should be enabled"));

    RegisterSetting(new CSCZ_CfgT<int>("LOG_V_SCROLL_SPEED", "LOG_V_SCROLL_SPEED", &(g_cfg_p->m_v_scrollSpeed),
                                       m_v_scrollSpeed,
                                       "The number of lines to scroll when using mouse wheel"));

    RegisterSetting(new CSCZ_CfgT<int>("PLOT_V_SCROLL_SPEED", "PLOT_V_SCROLL_SPEED", &(g_cfg_p->m_v_scrollGraphSpeed),
                                       m_v_scrollGraphSpeed, "Procentage of subplot to scroll when using mouse wheel"));

    RegisterSetting(new CSCZ_CfgT<int>("PLUGIN_DEBUG_BITMASK", "PLUGIN_DEBUG_BITMASK", &(g_cfg_p->m_pluginDebugBitmask),
                                       0, "Enables more information about plugin plots and decoders"));

    RegisterSetting(new CSCZ_CfgT<bool>("AUTO_CLOSE_PROGRESS_DLG", "AUTO_CLOSE_PROGRESS_DLG",
                                        &(g_cfg_p->m_autoCloseProgressDlg),
                                        true, "Automatically close progress dlg when processing is done"));

    RegisterSetting(new CSCZ_CfgT<bool>("DEV_TOOLS_ENABLED", "DEV_TOOLS_ENABLED", &(g_cfg_p->m_devToolsMenu),
                                        g_cfg_p->m_devToolsMenu, "Enable the developer tools menu"));

    RegisterSetting(new CSCZ_CfgT<bool>("DUMP_AT_ERROR", "DUMP_AT_ERROR", &(g_cfg_p->m_throwAtError),
                                        g_cfg_p->m_throwAtError, "Dump if a critical error was detected"));

    RegisterSetting(new CSCZ_CfgT<int>("LOG_LEVEL", "LOG_LEVEL", &(g_DebugLib->m_logLevel), g_DebugLib->m_logLevel,
                                       "Defines what prints that should be present in the log"));

    RegisterSetting(new CSCZ_CfgT<int>("TRACE_CATEGORY", "TRACE_CATEGORY", &(g_DebugLib->m_traceCategory),
                                       g_DebugLib->m_traceCategory,
                                       "Enable debugging of certain categories (0 None, 1 Focus, 2 Threads)"));
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
*   getFontColorTable
***********************************************************************************************************************/
const std::vector<ColorTableItem_t> *CConfig::getFontColorTable(void)
{
    return &g_fontColorTable;
}

/***********************************************************************************************************************
*   getColorTable
***********************************************************************************************************************/
const std::vector<ColorTableItem_t> *CConfig::getColorTable(void)
{
    return &g_colorTable;
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

        fileStream << XML_FILE_HEADER << Qt::endl;
        fileStream << LCZ_HEADER << Qt::endl;
        WriteSettings(&xmlFile, SettingScope_t::user, true);
        fileStream << LCZ_FOOTER << Qt::endl;
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
bool CConfig::WriteSettings(QFile *file_h, SettingScope_t scope, bool onlyChanged)
{
    if (!m_settingsList.isEmpty()) {
        QTextStream settingsStream(file_h);

        try {
            settingsStream << SETTINGS_HEADER << "\n";
        } catch (std::exception &e) {
            qFatal("Error %s writing settings header to file", e.what());
            TRACEX_E("CConfig::WriteSettings  Failed to write setting header to settings file,  Err:%s", e.what())
            return false;
        } catch (...) {
            qFatal("Unknown Error writting settings to file");
            TRACEX_E("CConfig::WriteSettings  Failed to write setting header to settings file,  Unknown error")
            return false;
        }

        CSCZ_CfgSettingBase *setting_p = nullptr;

        for (int index = 0; index < m_settingsList.count(); ++index) {
            setting_p = m_settingsList[index];

            if (((scope == SettingScope_t::all) || (scope == setting_p->m_scope)) &&
                (!onlyChanged || !setting_p->isDefaultValue())) {
                try {
                    setting_p->WriteToFile(&settingsStream);
                } catch (std::exception &e) {
                    qFatal("Error %s writing settings to file", e.what());
                    TRACEX_E("CConfig::WriteSettings  Failed to write settings to file,  Err:%s", e.what())
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
            TRACEX_E("CConfig::WriteSettings  Failed to write setting footer to settings file,  Err:%s", e.what())
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
                 .arg(setting_p->m_name).arg(setting_p->m_parseTag).arg(setting_p->m_description))
    m_settingsList.append(setting_p);
    setting_p->RestoreDefaultValue();
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
                TRACEX_I(QString("SetSetting    %1  %2").arg(setting_p->m_name).arg(setting_p->m_parseTag))
                setting_p->SetValue(value_p);
                return true;
            }
        }
        TRACEX_W("CSCZ_CfgFilter::SetSetting    Failed to find  %s", parseTag_p->toLatin1().constData())
    }
    TRACEX_W("CSCZ_CfgFilter::SetSetting    Failed to find  setting")
    return false;
}
