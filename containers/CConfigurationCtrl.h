/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QFileDialog>

#include "CCfgItem.h"
#include "CConfig.h"
#include "CFilter.h"
#include "CRecentFile.h"

typedef enum {
    inElement_logscrutinizer_e,
    inElement_filters_e,
    inElement_filter_e,
    inElement_logs_e,
    inElement_log_e,
    inElement_bookmarks_e,
    inElement_bookmark_e,
    inElement_comments_e,
    inElement_comment_e,
    inElement_plugins_e,
    inElement_plugin_e,
    inElement_searches_e,
    inElement_search_e,
    inElement_settings_e,
    inElement_setting_e,
    inElement_recentfiles_e,
    inElement_recentfile_e,
    inElement_none_e
}inElement_en;

typedef enum {
    inSetting_colorTable_e,
    inSetting_none_e,
}inSetting_en;

/* CFGCTRL_GetUserPickedFileNames is a helper function for a standardized way to open file dialogs and get the default
 * search path rigth. */
QStringList CFGCTRL_GetUserPickedFileNames(const QString& proposedFileName, const QFileDialog::AcceptMode mode,
                                           const QStringList& filters, const QList<RecentFile_Kind_e>& kindList,
                                           const QString& defaultDIR);
void CFGCTRL_RemoveDuplicateUrls(QList<QUrl>& urlList);
bool CFGCTRL_LoadFiles(QList<QString>& fileList_p);
bool CFGCTRL_UnloadAll(void);
bool CFGCTRL_LoadLogFile(void);
bool CFGCTRL_LoadPluginFile(void);
bool CFGCTRL_LoadDefaultWorkspace(void);
bool CFGCTRL_SaveDefaultWorkspaceFile(void);
bool CFGCTRL_SaveWorkspaceFile(QString& fileName, bool force = false);
bool CFGCTRL_SaveWorkspaceFile(void);
bool CFGCTRL_SaveWorkspaceFileAs(void);
bool CFGCTRL_LoadFilterFile(void);
bool CFGCTRL_SaveFilterFile(QString& fileName, CCfgItem_Filter *filterItem_p);
bool CFGCTRL_SaveFilterFileAs(QString *fileName_p, CCfgItem_Filter *filterItem_p);
bool CFGCTRL_ReloadFilterFile(QString& fileName, CCfgItem_Filter *filterItem_p);
bool CFGCTRL_Load_FileType(const QString& title, const QStringList& filters, const QList<RecentFile_Kind_e>& kindList,
                           const QString& defaultDIR);
QString CFGCTRL_GetWorkingDir();
QString CFGCTRL_GetDefaultWorkspaceFilePath();

/***********************************************************************************************************************
*   CConfigurationCtrl
***********************************************************************************************************************/
class CConfigurationCtrl : public CXMLBase
{
    Q_OBJECT

public:
    CConfigurationCtrl();
    virtual ~CConfigurationCtrl() {}

    bool LoadFile(const QString& fileName);
    bool ProcessData();

    void CleanAll(void);

    /* If asDefault is set then the workspace will be stiped and saved as default */
    bool Save_WorkspaceFile(const QString& fileName, bool asDefault = false);
    bool Save_WorkspaceFile(void); /* Save the current workspace file (document has the name) */
    bool Save_WorkspaceFileAs(void); /* Save the current workspace into a new name (shows file dialog) */
    bool Load_WorkspaceFile(void); /* Load a workspace, (shows file dialog) */

    QString GetSaveAs_FileName(QString *originalFileName);

    bool WriteHeader(QTextStream& fileStream);
    bool WriteFooter(QTextStream& fileStream);

    bool Save_FilterAs(const QString *originalFileName, CCfgItem_Filter *filterItem_p);
    bool Save_TAT_File(const QString& fileName, CCfgItem_Filter *cfgFilter_p);
    bool Reload_TAT_File(const QString& fileName, CCfgItem_Filter *cfgFilter_p);

    bool LoadFileList(QList<QString>& fileList);

    bool CanFileBeOpenedForFileRead(const QString& fileName, bool promtWarning = true);

    bool isDuplicate(const QString& fileName);
    void LoadPlugin(const QString& fileName);

    virtual void Init(void);
    virtual void Element_Value(char *value_p);
    virtual void Element_Attribute(char *name_p, char *value_p);
    virtual void ElementEnd(void);
    virtual void ElementStart(char *name_p);

    void Element_Attribute_Filter(char *name_p, char *value_p);
    void Element_Attribute_FilterItem(char *name_p, char *value_p);
    void Element_Attribute_Log(char *name_p, char *value_p);
    void Element_Attribute_Bookmark(char *name_p, char *value_p);
    void Element_Attribute_Plugin(char *name_p, char *value_p);
    void Element_Attribute_Setting(char *name_p, char *value_p);
    void Element_Attribute_Comment(char *name_p, char *value_p);
    void Element_Attribute_Search(char *name_p, char *value_p);
    void Element_Attribute_RecentFile(char *name_p, char *value_p);

    void ElementEnd_Setting(void);
    void ElementEnd_RecentFile(void);

protected:
    inElement_en m_inElement_L1;
    inElement_en m_inElement_L2;
    inSetting_en m_inSetting; /* Used for settings with more than one parameter */
    unsigned int m_version;
    QString m_filterFileName; /* The name of the filter being parsed, can be set at parse start if TAT file */
    QString m_filterFileName_Loaded; /* The name of the filter file that was loaded */
    CFilterItem *m_newFilterItem_p;
    CFilter *m_newFilter_p;
    bool m_reloadingFilter; /* Set to true when a re-load operation is ongoing */
    CCfgItem_Filter *m_cfgFilterReload_p;            /* Used for re-loading */
    char m_tempText[CFG_TEMP_STRING_MAX_SIZE]; /* used for temp storage of attributes */
    char m_tempText_2[CFG_TEMP_STRING_MAX_SIZE]; /* used for temp storage of attributes */
    unsigned int m_tempIndex;
    unsigned int m_tempRow;
    unsigned int m_tempStartRow;
    unsigned int m_tempStartCol;
    unsigned int m_tempEndRow;
    unsigned int m_tempEndCol;
    unsigned int m_tempColor;
    RecentFileInfo_t m_recentFile;
    char m_tempStr[CFG_TEMP_STRING_MAX_SIZE];
    QFile m_file; /* Member used to load file */
    int m_fileSize; /* Member used to load file */
    char *m_fileRef_p; /* Member used to load file */
    CMemPoolItem *m_memPoolItem_p; /* Member used to load file */
    uint32_t m_numOfBytesRead; /* Member used to load file */
    bool m_inWorkspace; /* Set when it is a workspace being parsed/loaded */

    /****/
    void InitTemp(void)
    {
        m_tempText[0] = 0;
        m_tempStr[0] = 0;
        m_tempIndex = 0;
        m_tempRow = 0;
        m_tempStartCol = 0;
        m_tempEndRow = 0;
        m_tempEndCol = 0;
        m_tempColor = 0;
    }

    /****/
    void InitTempFileNames(void)
    {
        m_filterFileName_Loaded = "";
        m_filterFileName = "";
    }
};
