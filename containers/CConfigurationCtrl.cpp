/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CConfigurationCtrl.h"
#include "CLogScrutinizerDoc.h"
#include "CDebug.h"
#include "CConfig.h"
#include "CProgressCtrl.h"
#include "CWorkspace.h"

#include "../qt/mainwindow_cb_if.h"

#include <QObject>
#include <QFile>
#include <QFileDialog>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QUrl>
#include <QLibrary>

static CConfigurationCtrl *g_cfg_ctrl_p = nullptr;

/* the same function for both workspace and settings */
static CConfigurationCtrl cfg_ctrl;

bool Load_FileType(const QString& title,
                   const QStringList& filters,
                   const QList<RecentFile_Kind_e>& kindList,
                   const QString& defaultDIR);

/***********************************************************************************************************************
*   CFGCTRL_UnloadAll
***********************************************************************************************************************/
bool CFGCTRL_UnloadAll(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    Q_CHECK_PTR(g_workspace_p);
    Q_CHECK_PTR(g_cfg_p);
    Q_CHECK_PTR(doc_p);

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_plugins_p != nullptr);
    Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
    if ((g_workspace_p->m_plugins_p == nullptr) || (g_workspace_p->m_filters_p == nullptr)) {
        return false;
    }

    CEditorWidget_EmptySelectionList();

    const QMessageBox::StandardButton ret
        = QMessageBox::question(MW_Parent(),
                                QObject::tr("Unloading Workspace"),
                                QObject::tr(
                                    "Everything in your workspace will be unloaded including bookmarks and rowClips. "
                                    "Press cancel to abort."),
                                QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) {
        g_workspace_p->RemoveLog();
        g_workspace_p->m_plugins_p->RemoveAllChildren();
        g_workspace_p->m_filters_p->RemoveAllChildren();
        g_cfg_p->ReloadSettings();
        doc_p->SetWorkspaceFileName(QString(""));
        g_workspace_p->m_workspaceRoot_p->UpdateName();
    }
    return true;
}

/***********************************************************************************************************************
*   CFGCTRL_LoadLogFile
***********************************************************************************************************************/
bool CFGCTRL_LoadLogFile(void)
{
    QStringList filters;
    filters << "Text files (*.txt *.log)";

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_LogFile_en);
    return CFGCTRL_Load_FileType(QString("Load log file"), filters, kindList,
                                 GetTheDoc()->m_recentFiles.GetRecentPath(RecentFile_Kind_LogFile_en));
}

/***********************************************************************************************************************
*   CFGCTRL_LoadPluginFile
***********************************************************************************************************************/
bool CFGCTRL_LoadPluginFile(void)
{
    QStringList filters;
    filters << "Plugin (*." << DLL_EXT;

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_PluginFile_en);
    return CFGCTRL_Load_FileType(QString("Load plugin"), filters, kindList,
                                 GetTheDoc()->m_recentFiles.GetRecentPath(RecentFile_Kind_PluginFile_en));
}

/***********************************************************************************************************************
*   CFGCTRL_LoadFilterFile
***********************************************************************************************************************/
bool CFGCTRL_LoadFilterFile(void)
{
    QStringList filters;
    filters << "Filter file (*.flt *.tat)";

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_FilterFile_en);
    return CFGCTRL_Load_FileType(QString("Load filter file"),
                                 filters,
                                 kindList,
                                 GetTheDoc()->m_recentFiles.GetRecentPath(RecentFile_Kind_FilterFile_en));
}

/***********************************************************************************************************************
*   CFGCTRL_Load_FileType
***********************************************************************************************************************/
bool CFGCTRL_Load_FileType(const QString& title,
                           const QStringList& filters,
                           const QList<RecentFile_Kind_e>& kindList,
                           const QString& defaultDIR)
{
    Q_UNUSED(title)

    QStringList fileNames = CFGCTRL_GetUserPickedFileNames(QString(""),
                                                           QFileDialog::AcceptOpen,
                                                           filters,
                                                           kindList,
                                                           defaultDIR);
    if (fileNames.isEmpty()) {
        return false;
    }
    return g_cfg_ctrl_p->LoadFileList(fileNames);
}

/***********************************************************************************************************************
*   CFGCTRL_GetUserPickedFileNames
***********************************************************************************************************************/
QStringList CFGCTRL_GetUserPickedFileNames(const QString& proposedFileName,
                                           const QFileDialog::AcceptMode mode,
                                           const QStringList& filters,
                                           const QList<RecentFile_Kind_e>& kindList,
                                           const QString& defaultDIR)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    TRACEX_D(QString("%1 proposedFileName:%2 defaultDir:%3").arg(__FUNCTION__).arg(proposedFileName).arg(defaultDIR))

    QFileDialog fileDialog(nullptr);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(mode);

    /* Restrict which files that might be opened */
    fileDialog.setNameFilters(filters);

    /* Give some favorites in the sidebar */
    QList<QUrl> urls;
    QStringList list; /* list to be converted to urls */

    doc_p->m_recentFiles.GetRecentPaths(list, kindList, true /*dir-only*/); /* append list with recent used paths to
                                                                             * files */

    for (auto& string : list) {
        urls << QUrl::fromLocalFile(string);
    }

    if (!defaultDIR.isEmpty()) {
        list.append(defaultDIR);
    }

    /* Add system usuable paths */
    QStringList standardLocations = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    for (auto& location : standardLocations) {
        urls << QUrl::fromLocalFile(location);
    }
    CFGCTRL_RemoveDuplicateUrls(urls);
    fileDialog.setOption(QFileDialog::DontUseNativeDialog); /* In order to get sideBars to work */
    fileDialog.setSidebarUrls(urls);

    if (!list.isEmpty()) {
        fileDialog.setDirectory(list.first());
    }

    if (!proposedFileName.isEmpty()) {
        fileDialog.selectFile(proposedFileName);
    }

    if (fileDialog.exec()) {
        return fileDialog.selectedFiles();
    }
    return QStringList(); /* if failure */
}

/***********************************************************************************************************************
*   CFGCTRL_RemoveDuplicateUrls
***********************************************************************************************************************/
void CFGCTRL_RemoveDuplicateUrls(QList<QUrl>& urlList)
{
    if (urlList.isEmpty()) {
        return;
    }

    int mainIndex = 0;

    while (mainIndex < urlList.count()) {
        QString path = urlList[mainIndex].path();
        int searchIndex = mainIndex + 1;
        while (searchIndex < urlList.count()) {
            if (path == urlList[searchIndex].path()) {
                urlList.removeAt(searchIndex);
            } else {
                searchIndex++;
            }
        }
        mainIndex++;
    }
}

/***********************************************************************************************************************
*   CFGCTRL_GetWorkingDir
***********************************************************************************************************************/
QString CFGCTRL_GetWorkingDir()
{
    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    return locations.first();
}

/***********************************************************************************************************************
*   CFGCTRL_GetDefaultWorkspaceFilePath
***********************************************************************************************************************/
QString CFGCTRL_GetDefaultWorkspaceFilePath()
{
    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    return QDir(locations.first()).filePath(g_cfg_p->m_defaultWorkspace);
}

/***********************************************************************************************************************
*   CFGCTRL_LoadDefaultWorkspace
***********************************************************************************************************************/
bool CFGCTRL_LoadDefaultWorkspace(void)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }

    QString workspacePath = CFGCTRL_GetDefaultWorkspaceFilePath();
    QFileInfo workspaceFileInfo(workspacePath);

    if (!g_cfg_ctrl_p->CanFileBeOpenedForFileRead(workspacePath, false)) {
        return false;
    }

    TRACEX_I(QString("Load default workspace, path:%1").arg(workspacePath))

    QList<QString> fileList;
    fileList.push_front(workspacePath);

    return g_cfg_ctrl_p->LoadFileList(fileList);
}

/***********************************************************************************************************************
*   CFGCTRL_LoadFiles
***********************************************************************************************************************/
bool CFGCTRL_LoadFiles(QList<QString>& fileList)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    return g_cfg_ctrl_p->LoadFileList(fileList);
}

/***********************************************************************************************************************
*   CFGCTRL_SaveWorkspaceFile
***********************************************************************************************************************/
bool CFGCTRL_SaveWorkspaceFile(QString& fileName, bool force)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }

    /* Avoid user to overwrite the default workspace when that is the current. Typically it is the one loaded, and if
     * user press Save then this will automatically be overwritten */
    if (!force && fileName.contains(g_cfg_p->m_defaultWorkspace)) {
        return (g_cfg_ctrl_p->Save_WorkspaceFileAs());
    } else {
        return (g_cfg_ctrl_p->Save_WorkspaceFile(fileName));
    }
}

/***********************************************************************************************************************
*   CFGCTRL_SaveDefaultWorkspaceFile
***********************************************************************************************************************/
bool CFGCTRL_SaveDefaultWorkspaceFile(void)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    return (g_cfg_ctrl_p->Save_WorkspaceFile(CFGCTRL_GetDefaultWorkspaceFilePath(), true));
}

/***********************************************************************************************************************
*   CFGCTRL_SaveWorkspaceFile
***********************************************************************************************************************/
bool CFGCTRL_SaveWorkspaceFile(void)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }

    CLogScrutinizerDoc *doc_p = GetTheDoc();

    if (doc_p->GetWorkspaceFileName().isEmpty()) {
        return (g_cfg_ctrl_p->Save_WorkspaceFileAs());
    }

    /* Avoid user to overwrite the default workspace when that is the current. Typically it is the one loaded, and if
     * user press Save then this will automatically be overwritten */
    if (doc_p->GetWorkspaceFileName().contains(g_cfg_p->m_defaultWorkspace)) {
        return (g_cfg_ctrl_p->Save_WorkspaceFileAs());
    } else {
        return (g_cfg_ctrl_p->Save_WorkspaceFile());
    }
}

/***********************************************************************************************************************
*   CFGCTRL_SaveFilterFileAs
***********************************************************************************************************************/
bool CFGCTRL_SaveFilterFileAs(QString *fileName_p, CCfgItem_Filter *filterItem_p)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    return (g_cfg_ctrl_p->Save_FilterAs(fileName_p, filterItem_p));
}

/***********************************************************************************************************************
*   CFGCTRL_SaveFilterFile
***********************************************************************************************************************/
bool CFGCTRL_SaveFilterFile(QString& fileName, CCfgItem_Filter *filter_p)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    return g_cfg_ctrl_p->Save_TAT_File(fileName, filter_p);
}

/***********************************************************************************************************************
*   CFGCTRL_ReloadFilterFile
***********************************************************************************************************************/
bool CFGCTRL_ReloadFilterFile(QString& fileName, CCfgItem_Filter *filter_p)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    TRACEX_I(QString("Reloading TAT File  %1").arg(fileName))
    g_cfg_ctrl_p->Reload_TAT_File(fileName, filter_p);
    return true;
}

/***********************************************************************************************************************
*   CFGCTRL_SaveWorkspaceFileAs
***********************************************************************************************************************/
bool CFGCTRL_SaveWorkspaceFileAs(void)
{
    if (g_cfg_ctrl_p == nullptr) {
        return false;
    }
    return (g_cfg_ctrl_p->Save_WorkspaceFileAs());
}

/***********************************************************************************************************************
*   CConfigurationCtrl - CTOR
***********************************************************************************************************************/
CConfigurationCtrl::CConfigurationCtrl()
{
    g_cfg_ctrl_p = this;

    m_inElement_L1 = inElement_none_e;
    m_inElement_L2 = inElement_none_e;
    m_inSetting = inSetting_none_e;

    m_reloadingFilter = false;
    m_newFilter_p = nullptr;
    m_newFilterItem_p = nullptr;
    m_cfgFilterReload_p = nullptr;

    m_fileRef_p = nullptr;
    m_memPoolItem_p = nullptr;
    m_numOfBytesRead = 0;
    m_fileSize = 0;

    m_filterFileName = "";

    m_inWorkspace = false;
}

#ifdef QT_TODO /* CheckPluginArchitecture */

/***********************************************************************************************************************
*   CheckPluginArchitecture
***********************************************************************************************************************/
bool CConfigurationView::CheckPluginArchitecture(char *fileName_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString absoluteFileName;

    doc_p->GetAbsoluteFileName(fileName_p, &absoluteFileName);

    HANDLE hFile = CreateFile(absoluteFileName.GetBuffer(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_READONLY,
                              nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        TRACEX_W("Loading plugin failed at file open, code:%d", GetLastError())
        return false;
    }

    HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);

    if ((hMapping == INVALID_HANDLE_VALUE) || (hMapping == 0)) {
        TRACEX_W("Loading plugin failed at file mapping, code:%d", GetLastError())
        return false;
    }

    LPVOID addrHeader = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

    if (addrHeader == nullptr) {
        CloseHandle(hFile);
        TRACEX_W("Loading plugin failed at mapping view of file, code:%d", GetLastError())
        return false;
    }

    PIMAGE_NT_HEADERS peHdr = ImageNtHeader(addrHeader);  /* Located the IMAGE_NT_HEADERS structure in a PE image and
                                                           * reutrns a pointer to the data */

    if (peHdr == nullptr) {
        UnmapViewOfFile(addrHeader);
        CloseHandle(hFile);

        TRACEX_W("Loading plugin failed at getting NT Headers, code:%d", GetLastError())
        return false;
    }

 #ifdef _WIN64
    if ((peHdr->FileHeader.Machine != IMAGE_FILE_MACHINE_IA64) &&
        (peHdr->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)) {
        UnmapViewOfFile(addrHeader);
        CloseHandle(hFile);

        TRACEX_W(
            "Warning: Load Plugin Failed - DLL not 64-bit plugin. This exe of LogScrutinizer is 64-bit, as such you need a version of the plugin which is also 64-bit");

        QMessageBox msgBox(QMessageBox::Warning,
                           QString("Wrong 32/64 architecture"),
                           QString(
                               "Load Plugin Failed - DLL not 64-bit plugin. This exe of LogScrutinizer is 64-bit, as such you need a version of the plugin which is also 64-bit"),
                           QMessageBox::Ok);
        msgBox.exec();

        MessageBox(
            "Load Plugin Failed - DLL not 64-bit plugin. This exe of LogScrutinizer is 64-bit, as such you need a version of the plugin which is also 64-bit",
            "Wrong 32/64 architecture",
            MB_OK | MB_ICONERROR | MB_TOPMOST);
        return false;
    }
 #else
    if (peHdr->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
        UnmapViewOfFile(addrHeader);
        CloseHandle(hFile);

        TRACEX_W(
            "Warning: Load Plugin Failed - DLL not 32-bit plugin. This exe of LogScrutinizer is 32-bit, as such you need a version of the plugin which is also 32-bit");
        MessageBox(
            "Load Plugin Failed - DLL not 32-bit plugin. This exe of LogScrutinizer is 32-bit, as such you need a version of the plugin which is also 32-bit",
            "Wrong 32/64 architecture",
            MB_OK | MB_ICONERROR | MB_TOPMOST);
        return false;
    }
 #endif

    UnmapViewOfFile(addrHeader);
    CloseHandle(hFile);

    return true;
}
#endif

/***********************************************************************************************************************
*   LoadPlugin
* This function should check if there is a duplicate of the file.
***********************************************************************************************************************/
bool CConfigurationCtrl::isDuplicate(const QString& fileName)
{
    if (g_workspace_p->GetCfgItem(fileName) != nullptr) {
        return true;
    } else {
        return false;
    }
}

/***********************************************************************************************************************
*   LoadPlugin
***********************************************************************************************************************/
void CConfigurationCtrl::LoadPlugin(const QString& fileName)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    DLL_API_PluginVersion_t DLL_API_Version;
    CPlugin_DLL_API *pluginAPI_p;
    extern void MW_DO_SHOW(void);

    /* DLL_API V1 */
    QString absoluteFileName;

    doc_p->GetAbsoluteFileName(fileName, absoluteFileName);

    memset(&DLL_API_Version, 0, sizeof(DLL_API_Version));

#ifdef QT_TODO /* CheckPluginArchitecture */
    if (!CheckPluginArchitecture(absoluteFileName.GetBuffer())) {
        uint32_t dwError = GetLastError();

        sprintf_s(m_tempStr,
                  CFG_TEMP_STRING_MAX_SIZE,
                  "Could not load the DLL-file {%s}, locked by another other application?\nError code:%u",
                  absoluteFileName.GetBuffer(),
                  dwError);

        /* Show information that the file format is not supported */
        MessageBox(m_tempStr, "Bad DLL file", MB_OK | MB_ICONERROR | MB_TOPMOST);
        return;
    }
#endif

    if (!QLibrary::isLibrary(absoluteFileName)) {
        return;
    }

    QLibrary *library_p = new QLibrary(absoluteFileName);
    library_p->load();

    if (!library_p->isLoaded()) {
        QString title(tr("Plugin load failed"));
        QString description;

        /* http://www.microsoft.com/en-us/download/details.aspx?id=30679# */
#ifdef WIN32
        uint32_t dwError = GetLastError();
        if (dwError == 126) {
            description = tr(
                "DLL platform and version checks was ok, but could not load the library {%1}\nPerhaps you are missing MSVCR110.dll, if true then try to download\n  http://www.microsoft.com/en-us/download/details.aspx?id=30679# \nError code:%2")
                              .arg(absoluteFileName).arg(dwError);
        } else {
            description = tr("DLL platform and version checks was ok, but loading it failed {%1}\nError code:%2").arg(
                absoluteFileName).arg(dwError);
        }
#else
        description =
            tr("so-file platform and version checks was ok, but loading it failed {%1} error:%2 dl_error:%3").arg(
                absoluteFileName).arg(errno).arg(library_p->errorString());
#endif
        TRACEX_W(description)

        QMessageBox msgBox(QMessageBox::Critical, title, description, QMessageBox::Ok);
        MW_DO_SHOW();
        msgBox.exec();
        library_p->unload();
        delete library_p;
        return;
    }

    DLL_API_GetPluginAPIVersion_t DLL_API_GetPluginAPIVersion =
        reinterpret_cast<DLL_API_GetPluginAPIVersion_t>(library_p->resolve(DLL_API_GET_PLUGIN_API_VERSION));

    if (DLL_API_GetPluginAPIVersion != nullptr) {
        DLL_API_GetPluginAPIVersion(&DLL_API_Version);
    } else {
        QString title(tr("Plugin load failed"));
        QString description = tr(
            "Could not find {%1} in library {%2}, the library is probarbly not a valid LogScrutinizer plugin").arg(
            DLL_API_GET_PLUGIN_API_VERSION).arg(absoluteFileName);
        TRACEX_W(description)

        QMessageBox msgBox(QMessageBox::Critical, title, description, QMessageBox::Ok);
        MW_DO_SHOW();
        msgBox.exec();
        library_p->unload();
        delete library_p;
        return;
    }

    if (DLL_API_Version.version == DLL_API_VERSION) {
        DLL_API_SetAttachConfiguration_t DLL_API_SetAttachConfiguration =
            reinterpret_cast<DLL_API_SetAttachConfiguration_t>(library_p->resolve(DLL_API_SET_ATTACH_CONFIGURATION));

        if (DLL_API_SetAttachConfiguration != nullptr) {
            DLL_API_AttachConfiguration_t attachConfiguration;

#ifdef QT_TODO /* Plugin Dbg message handling  hwnd_traceConsumer window handler to receive message, h_traceHeap is a
                * memory heap shared by plugin and LS */
            extern HWND g_hwndOutputWnd;
            attachConfiguration.hwnd_traceConsumer = g_hwndOutputWnd;
            attachConfiguration.h_traceHeap = g_DebugLib->GetPluginMsgHeap();
#else
            /* attachConfiguration.hwnd_traceConsumer = 0;
             * attachConfiguration.h_traceHeap = 0; */
#endif
            DLL_API_SetAttachConfiguration(&attachConfiguration);
        }

        DLL_API_CreatePlugin_t DLL_API_CreatePlugin =
            reinterpret_cast<DLL_API_CreatePlugin_t>(library_p->resolve(DLL_API_CREATE_PLUGIN));

        if (DLL_API_CreatePlugin != nullptr) {
            pluginAPI_p = DLL_API_CreatePlugin();
            g_workspace_p->AddPlugin(pluginAPI_p, library_p);
            doc_p->m_recentFiles.AddFile(absoluteFileName);
        }
    } else {
        QString title(tr("Plugin load failed"));
        QString description = tr(
            "The versions between LogScrutinizer and the plugin {%1} are not matching\nPlugin version=%2, "
            "LogScrutinzer version=%3\n Please either get matching versions")
                                  .arg(absoluteFileName).arg(DLL_API_Version.version).arg(DLL_API_VERSION);
        TRACEX_W(description)

        QMessageBox msgBox(QMessageBox::Critical, title, description, QMessageBox::Ok);
        MW_DO_SHOW();
        msgBox.exec();
        library_p->unload();
        delete library_p;
        return;
    }
}

/***********************************************************************************************************************
*   Save_WorkspaceFile
***********************************************************************************************************************/
bool CConfigurationCtrl::Save_WorkspaceFile(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    TRACEX_D(QString("%1").arg(__FUNCTION__))

    if (doc_p->GetWorkspaceFileName().isEmpty()) {
        return Save_WorkspaceFileAs();
    } else {
        return Save_WorkspaceFile(doc_p->GetWorkspaceFileName());
    }
}

/***********************************************************************************************************************
*   Save_WorkspaceFileAs
***********************************************************************************************************************/
bool CConfigurationCtrl::Save_WorkspaceFileAs(void)
{
    TRACEX_D(QString("%1").arg(__FUNCTION__))

    CLogScrutinizerDoc *doc_p = GetTheDoc();

    QStringList filters;
    filters << "Workspace file (*.lsz)";

    /* Set the proposed file name as the current log + .lsz */
    QFileInfo fileNameInfo(doc_p->m_Log_FileName);
    QString proposedFileName = fileNameInfo.path() + QDir::separator() + fileNameInfo.completeBaseName() + ".lsz";

    /* Give some favorites in the sidebar */
    QList<QUrl> urls;
    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_WorkspaceFile_en);
    kindList.append(RecentFile_Kind_LogFile_en);

    QStringList list; /* list to be converted to urls */
    doc_p->m_recentFiles.GetRecentPaths(list, kindList); /* append list with recent used paths to files */

    QStringList fileNames =
        CFGCTRL_GetUserPickedFileNames(proposedFileName, QFileDialog::AcceptSave, filters, kindList, list.first());

    if (!fileNames.empty() && !fileNames.first().isEmpty()) {
        return CFGCTRL_SaveWorkspaceFile(fileNames.first(), true);
    }
    return false;
}

/***********************************************************************************************************************
*   Save_FilterAs
***********************************************************************************************************************/
bool CConfigurationCtrl::Save_FilterAs(const QString *originalFileName, CCfgItem_Filter *filterItem_p)
{
    TRACEX_D(QString("%1").arg(__FUNCTION__))

    QStringList filters;
    filters << "Filter file (*.flt)";

    QString proposedFileName;
    QString defaultDIR;

    /* Set the proposed file name as the current file name + .flt */
    if ((originalFileName != nullptr) && !originalFileName->isEmpty()) {
        QFileInfo fileNameInfo(*originalFileName);
        proposedFileName = fileNameInfo.path() + QDir::separator() + fileNameInfo.completeBaseName() + ".flt";
        defaultDIR = fileNameInfo.absolutePath();
    }

    QList<RecentFile_Kind_e> kindList;
    kindList.append(RecentFile_Kind_FilterFile_en);
    kindList.append(RecentFile_Kind_WorkspaceFile_en);
    kindList.append(RecentFile_Kind_LogFile_en);

    QStringList fileNames =
        CFGCTRL_GetUserPickedFileNames(proposedFileName, QFileDialog::AcceptSave, filters, kindList, defaultDIR);

    if (!fileNames.isEmpty()) {
        return Save_TAT_File(fileNames.first(), filterItem_p);
    }
    return false;
}

/***********************************************************************************************************************
*   LoadFileList
***********************************************************************************************************************/
bool CConfigurationCtrl::LoadFileList(QList<QString>& fileList)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    auto beforeValue = CSCZ_TreeViewSelectionEnabled;

    /* To ensure that the global flag is restored when leaving the function */
    auto resetTreeViewSelectionEnableScopeGuard =
        makeMyScopeGuard([&] () {CSCZ_TreeViewSelectionEnabled = beforeValue;});

    TRACEX_D(QString("%1").arg(__FUNCTION__))

    QString fileName;
    QString fileName_temp;
    QString ext;

    if (fileList.isEmpty()) {
        TRACEX_D("CConfigurationCtrl::LoadFileList  - Empty file list")
        return true;
    }

    InitTempFileNames();

    /* First check that all files can be opened */

    for (auto& fileName : fileList) {
        if (!CanFileBeOpenedForFileRead(fileName)) {
            /* will prompt warning */
            return false;
        }
    }

    for (auto& fileName : fileList) {
        QFileInfo fileInfo(fileName);
        QString ext = fileInfo.suffix();
        auto cleanFileData = makeMyScopeGuard([&] () {
            if (m_memPoolItem_p != nullptr) {
                doc_p->m_memPool.ReturnMem(&m_memPoolItem_p);
            }
            m_memPoolItem_p = nullptr;
            m_fileRef_p = nullptr;
            m_numOfBytesRead = 0;
            m_fileSize = 0;
        });

        if (ext == "lsz") {
            /* Lets check if there is a log file in the *.lsz file. Otherwise we do not need to clear everything... just
             * add ontop */
            m_inWorkspace = true;
            TRACEX_I(QString("Loaded workspace file: %1").arg(fileName))
            doc_p->SetWorkspaceFileName(fileName);

            if (LoadFile(fileName)) {
                /* Check if we can find a log file in the *.lsz file. If so we need to unload all other stuff */
                SetDocument(m_fileRef_p, m_numOfBytesRead);

                char logFileName[CFG_TEMP_STRING_MAX_SIZE];

                if (SearchElementAttribute("log", "path", logFileName, CFG_TEMP_STRING_MAX_SIZE)) {
                    if (CanFileBeOpenedForFileRead(QString(logFileName))) {
                        CleanAll();
                        doc_p->SetWorkspaceFileName(fileName);   /* set it again, removed at  CleanAll(); */
                    }
                }

                /* Parse the *.lsz file */
                ProcessData();
            }
            m_inWorkspace = false;
        }
    } /* Loop through the entire file list looking for workspace files */

    /* Step 2. */

    for (auto& fileNameTemp : fileList) {
        auto cleanFileData = makeMyScopeGuard([&] () {
            if (m_memPoolItem_p != nullptr) {
                doc_p->m_memPool.ReturnMem(&m_memPoolItem_p);
            }
            m_memPoolItem_p = nullptr;
            m_fileRef_p = nullptr;
            m_numOfBytesRead = 0;
            m_fileSize = 0;
        });

        doc_p->GetAbsoluteFileName(fileNameTemp, fileName);

        QFileInfo fileInfo(fileName);
        QString ext = fileInfo.suffix();

        if ((ext == "txt") || (ext == "log")) {
            g_workspace_p->RemoveLog();

            if (CanFileBeOpenedForFileRead(fileName)) {
                g_workspace_p->RemoveLog(); /* ensure that if the loadlogfile fails the tree will be updated with that
                                             * the log has been cleaned out */
                if (doc_p->LoadLogFile(fileName)) {
                    /* this one will clean row clip, bookmarks, and comments */
                    doc_p->m_recentFiles.WriteToFile();
                    g_workspace_p->AddLog(fileName.toLatin1().data());
                }
            }
        } else if ((ext == "tat") || (ext == "flt")) {
            m_filterFileName_Loaded = fileName;
            m_filterFileName = fileName;  /* temporarily, will possible be found in the *.flt file */

            g_processingCtrl_p->Processing_StartReport();

            if (LoadFile(fileName)) {
                TRACEX_I(QString("Loaded filter: %1").arg(fileName))
                ProcessData();
            }

            g_processingCtrl_p->Processing_StopReport();

            InitTempFileNames();
        } else if (ext == DLL_EXT) {
            if (CanFileBeOpenedForFileRead(fileName) && !isDuplicate(fileName)) {
                LoadPlugin(fileName);
                TRACEX_I(QString("Loaded plugin file: %1").arg(fileName))
                doc_p->m_recentFiles.WriteToFile();
            }
        } else if (ext == "rcnt") {
            if (CanFileBeOpenedForFileRead(fileName, false)) {
                if (LoadFile(fileName)) {
                    ProcessData();
                    TRACEX_I(QString("Loaded RecentFile DB: %1").arg(fileName))
                    MW_RebuildRecentFileMenu();
                }
            }
        } else if (ext == "lsz") {
            /* Do nothing, already loaded above */
        } else {
            /* Show information that the file format is not supported */
            QMessageBox msgBox(QMessageBox::Warning,
                               QString("Warning, not supported file extension"),
                               QString(
                                   "The file extension *.%1 is not supported.\n Supported is: *.txt, *.log, *." DLL_EXT
                                   " *.lsz").arg(
                                   ext),
                               QMessageBox::Ok);
            msgBox.exec();
        }
    }

    doc_p->UpdateTitleRow();

    extern void MW_DO_SHOW(void);
    MW_DO_SHOW();
    return true;
}

/***********************************************************************************************************************
*   CleanAll
***********************************************************************************************************************/
void CConfigurationCtrl::CleanAll(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();

    /* Clean all */
    doc_p->CleanDB(true);  /*Do not unload filters */

    if (g_workspace_p != nullptr) {
        /* ----------------------------------  1. FILTERs ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
        if (g_workspace_p->m_filters_p != nullptr) {
            g_workspace_p->m_filters_p->RemoveAllChildren();
        }

        /* ----------------------------------  2. PLUGINs ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_plugins_p != nullptr);
#endif
        if (g_workspace_p->m_plugins_p != nullptr) {
            g_workspace_p->m_plugins_p->RemoveAllChildren();
        }

        /* ----------------------------------  3. Bookmarks ---------------------------------------- */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_bookmarks_p != nullptr);
#endif
        if (g_workspace_p->m_bookmarks_p != nullptr) {
            g_workspace_p->m_bookmarks_p->RemoveAllChildren();
        }

        /* ----------------------------------  4. SETTINGS ------------------------------------------
         *
         * ----------------------------------  5. Comments ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_comments_p != nullptr);
#endif
        if (g_workspace_p->m_comments_p != nullptr) {
            g_workspace_p->m_comments_p->RemoveAllChildren();
        }

        /* ----------------------------------  6. Log ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_logs_p != nullptr);
#endif
        if (g_workspace_p->m_logs_p != nullptr) {
            g_workspace_p->m_logs_p->RemoveAllChildren();
        }
    }
    TRACEX_D("Clean all")
}

/***********************************************************************************************************************
*   Save_WorkspaceFile
***********************************************************************************************************************/
bool CConfigurationCtrl::Save_WorkspaceFile(const QString& fileName, bool asDefault)
{
    CfgItemSaveOptions_t saveOptions = CFG_ITEM_SAVE_OPTION_NO_OPTIONS;
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QFile file(fileName);

#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p->m_root_p != nullptr);
#endif

    if (!file.open(QIODevice::WriteOnly)) {
        TRACEX_E(QString("CConfig::Save_WorkspaceFile  Failed to create settings file, FileName:%1").arg(fileName))
        return false;
    }

    QTextStream fileStream(&file);
    if (!WriteHeader(fileStream)) {
        return false;
    }

    fileStream.flush();

    saveOptions |= CFG_ITEM_SAVE_OPTION_REL_PATH;
    if (asDefault) {
        saveOptions |= CFG_ITEM_SAVE_OPTION_TO_DEFAULT_WORKSPACE;
    } else {}

    /* Write the workspace */

    /* 1. Write the filters
     * 2. Write the plugins     (path)
     * 3. Write the bookmarks
     * 4. Write the settings
     * 5. Write "the" log
     * 5. Write the comments *//* Note: Log/Search History shall be stored continously in a seperate file */
    if (g_workspace_p != nullptr) {
        /* Always have log file first in the workspace, since loading log file will cause the doc to clean all... e.g.
         * including bookmarks */

        /* ----------------------------------  0. SETTINGS ------------------------------------------ */
        g_cfg_p->WriteSettings(&file, SettingScope_t::workspace, true);
        fileStream.flush();

        /* ----------------------------------  1. Log ------------------------------------------
         * if (!asDefault) */
        {
#ifdef ASSERT_ON_NULL
            Q_ASSERT(g_workspace_p->m_logs_p != nullptr);
#endif
            if (g_workspace_p->m_logs_p != nullptr) {
                g_workspace_p->m_logs_p->WriteToFile(fileStream, saveOptions);
            }

            fileStream.flush();
        }

        /* ----------------------------------  2. FILTERs ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_filters_p != nullptr);
#endif
        if (g_workspace_p->m_filters_p != nullptr) {
            g_workspace_p->m_filters_p->WriteToFile(fileStream, saveOptions);
        }
        fileStream.flush();

        /* ----------------------------------  3. PLUGINs ------------------------------------------ */
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_workspace_p->m_plugins_p != nullptr);
#endif
        if (g_workspace_p->m_plugins_p != nullptr) {
            g_workspace_p->m_plugins_p->WriteToFile(fileStream, saveOptions);
        }
        fileStream.flush();

        /* ----------------------------------  4. Bookmarks ---------------------------------------- */
        if (!asDefault) {
#ifdef ASSERT_ON_NULL
            Q_ASSERT(g_workspace_p->m_bookmarks_p != nullptr);
#endif
            if (g_workspace_p->m_bookmarks_p != nullptr) {
                g_workspace_p->m_bookmarks_p->WriteToFile(fileStream, saveOptions);
            }
            fileStream.flush();
        }

        /* ----------------------------------  5. Comments ------------------------------------------ */
        if (!asDefault) {
#ifdef ASSERT_ON_NULL
            Q_ASSERT(g_workspace_p->m_comments_p != nullptr);
#endif
            if (g_workspace_p->m_comments_p != nullptr) {
                g_workspace_p->m_comments_p->WriteChildrensToFile(fileStream, saveOptions);
            }
            fileStream.flush();
        }
    }

    if (!WriteFooter(fileStream)) {
        return false;
    }

    if (!asDefault) {
        doc_p->SetWorkspaceFileName(fileName);

        if ((g_workspace_p != nullptr) && (g_workspace_p->m_workspaceRoot_p != nullptr)) {
            g_workspace_p->m_workspaceRoot_p->UpdateName();
        }
    }

    TRACEX_I(QString("Saved workspace to: %1").arg(fileName))
    doc_p->m_recentFiles.AddFile(fileName);
    doc_p->m_recentFiles.WriteToFile();
    fileStream.flush();
    return true;
}

/***********************************************************************************************************************
*   WriteHeader
***********************************************************************************************************************/
bool CConfigurationCtrl::WriteHeader(QTextStream& fileStream)
{
    try {
        fileStream << XML_FILE_HEADER << "\n" << LCZ_HEADER << "\n";
    } catch (...) {
        TRACEX_E("CConfigurationCtrl::WriteHeader  Failed to write header to settings file")
        return false;
    }
    return true;
}

/***********************************************************************************************************************
*   WriteFooter
***********************************************************************************************************************/
bool CConfigurationCtrl::WriteFooter(QTextStream& fileStream)
{
    try {
        fileStream << LCZ_FOOTER << "\n";
    } catch (...) {
        TRACEX_E("CConfigurationCtrl::WriteFooter  Failed to write footer to settings file")
        return false;
    }
    return true;
}

/***********************************************************************************************************************
*   Save_TAT_File
***********************************************************************************************************************/
bool CConfigurationCtrl::Save_TAT_File(const QString& fileName, CCfgItem_Filter *cfgFilter_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        TRACEX_E(QString("CConfigurationCtrl::Save_TAT_File  Failed to open TAT file %1 for save").arg(fileName))
        return false;
    }

    cfgFilter_p->m_filter_ref_p->SetFileName(fileName);

    try {
        QTextStream fileStream(&file);
        fileStream << XML_FILE_HEADER << "\n" << LCZ_HEADER << "\n";
        cfgFilter_p->WriteToFile(fileStream, CFG_ITEM_SAVE_OPTION_REL_PATH);
        fileStream << LCZ_FOOTER << "\n";
        doc_p->m_recentFiles.AddFile(fileName);
        doc_p->m_recentFiles.WriteToFile();
    } catch (...) {
        TRACEX_E(QString("CConfigurationCtrl::Save_TAT_File  Failed to write filter data to TAT file %1").arg(fileName))
    }
    cfgFilter_p->UpdateTreeName(*(cfgFilter_p->m_filter_ref_p->GetShortFileName()));
    TRACEX_I(QString("Saved filter file: %1").arg(fileName))
    return true;
}

/***********************************************************************************************************************
*   Reload_TAT_File
***********************************************************************************************************************/
bool CConfigurationCtrl::Reload_TAT_File(const QString& fileName, CCfgItem_Filter *cfgFilter_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    bool status = false;

    if (!CanFileBeOpenedForFileRead(fileName)) {
        TRACEX_E(QString("Reloading filter file failed, couldn't open file: %1").arg(fileName))
        return false;
    }

    if ((cfgFilter_p == nullptr) || (cfgFilter_p->m_filter_ref_p == nullptr)) {
        TRACEX_E("Reloading filter file failed, internal error")
        return false;
    }

    cfgFilter_p->m_filter_ref_p->Init(fileName.toLatin1().data());

    InitTempFileNames();

    m_newFilter_p = cfgFilter_p->m_filter_ref_p;
    m_reloadingFilter = true;
    m_cfgFilterReload_p = cfgFilter_p;
    m_filterFileName = fileName;

    if (LoadFile(fileName)) {
        status = ProcessData();
    } else {
        return false;
    }

    m_newFilter_p = nullptr;
    m_reloadingFilter = false;
    m_cfgFilterReload_p = nullptr;

    TRACEX_I(QString("Reloaded filter file: %1").arg(fileName))

    return status;
}

/***********************************************************************************************************************
*   LoadFile
***********************************************************************************************************************/
bool CConfigurationCtrl::LoadFile(const QString& fileName)
{
    bool status = false;
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString absoluteFileName;

    if (!CanFileBeOpenedForFileRead(fileName)) {
        return false;
    }

    doc_p->GetAbsoluteFileName(fileName, absoluteFileName);

    if (m_file.isOpen()) {
#ifdef _DEBUG
        TRACEX_QFILE(LOG_LEVEL_ERROR, "CConfigurationCtrl::LoadFile  m_file not cleaned up", &m_file)
#endif
        m_file.close();
    }

    m_file.setFileName(absoluteFileName);

    if (!m_file.open(QIODevice::ReadOnly)) {
        TRACEX_E(QString("CConfigurationCtrl::LoadFile  Failed to open file %1").arg(fileName))
        return false;
    } else {
        QFileInfo fileInfo(absoluteFileName);
        m_fileSize = static_cast<int>(fileInfo.size());

        if (m_memPoolItem_p != nullptr) {
#ifdef _DEBUG
            TRACEX_QFILE(LOG_LEVEL_ERROR, "CConfigurationCtrl::LoadFile  m_memPoolItem_p not cleaned up", &m_file)
#endif
            doc_p->m_memPool.ReturnMem(&m_memPoolItem_p);
        }

        m_memPoolItem_p = doc_p->m_memPool.AllocMem(m_fileSize);
        m_fileRef_p = static_cast<char *>(m_memPoolItem_p->GetDataRef());

        if (m_fileRef_p == nullptr) {
            TRACEX_QFILE(LOG_LEVEL_WARNING, "Could not allocate memory for file", &m_file)
            return false;
        }

        m_numOfBytesRead = m_file.read(m_fileRef_p, m_fileSize);
        status = m_numOfBytesRead > 0 ? true : false;
        m_file.close();
    }
    return status;
}

/* Regardless of the extension this function will be called. */
bool CConfigurationCtrl::ProcessData()
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_workspace_p != nullptr);
    Q_ASSERT(g_workspace_p->m_workspaceRoot_p != nullptr);
#endif
    if ((g_workspace_p == nullptr) || (g_workspace_p->m_workspaceRoot_p == nullptr) ||
        (g_workspace_p->m_plugins_p == nullptr)) {
        return false;
    }

    if (m_file.isOpen()) {
#ifdef _DEBUG
        TRACEX_E(QString("Internal error, file is still open"))
#endif
        m_file.close();
    }

    if ((m_fileRef_p == nullptr) || (m_fileSize == 0) || (m_memPoolItem_p == nullptr) || (m_numOfBytesRead == 0)) {
        TRACEX_E(QString("Internal error detected when processing file"))
        return false;
    }

    /*--- PARSING ---- */
    Parse(m_fileRef_p, static_cast<int>(m_numOfBytesRead));

    doc_p->m_recentFiles.AddFile(m_file.fileName());

    QFileInfo fileInfo(m_file.fileName());
    QString ext = fileInfo.suffix();

    if (ext == "lsz") {
        doc_p->SetWorkspaceFileName(m_file.fileName());

        if (g_workspace_p != nullptr) {
            g_workspace_p->m_workspaceRoot_p->UpdateName();
        }
    }
    if (ext != "rcnt") {
        doc_p->m_recentFiles.WriteToFile();
    }

    if (m_fileRef_p != nullptr) {
        doc_p->m_memPool.ReturnMem(&m_memPoolItem_p);
        m_fileRef_p = nullptr; /* this memory is freed, or pooled, when returning memory */
        m_memPoolItem_p = nullptr;
        m_numOfBytesRead = 0;
        m_fileSize = 0;
    }
    return true;
}

/***********************************************************************************************************************
*   CanFileBeOpenedForFileRead
***********************************************************************************************************************/
bool CConfigurationCtrl::CanFileBeOpenedForFileRead(const QString& fileName, bool promtWarning)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    QString absouteFileName = fileName;
    QFileInfo fileInfo(fileName);

    if (!fileInfo.exists()) {
        doc_p->GetAbsoluteFileName(fileName, absouteFileName);
    }

    TRACEX_D(QString("Checking if possible to open: %1").arg(absouteFileName))

    QFile testFile(absouteFileName);

    if (!testFile.exists() || !testFile.open(QIODevice::ReadOnly)) {
        if (promtWarning) {
            TRACEX_I(QString("Not possible to open file: %1, starting from path: %2")
                         .arg(fileName).arg(QDir::currentPath()))

            QMessageBox msgBox(QMessageBox::Warning, QString("File load failed"), QString(
                                   "Couldn't open file: %1, starting from path %2")
                                   .arg(fileName).arg(QDir::currentPath()), QMessageBox::Ok);
            msgBox.exec();
        }
        return false;
    }
    return true;
}

/***********************************************************************************************************************
*   Init
***********************************************************************************************************************/
void CConfigurationCtrl::Init(void)
{
    m_inElement_L1 = inElement_none_e;
    m_inElement_L2 = inElement_none_e;
}

/***********************************************************************************************************************
*   ElementStart
***********************************************************************************************************************/
void CConfigurationCtrl::ElementStart(char *name_p)
{
    inElement_en l1_old = m_inElement_L1;
    inElement_en l2_old = m_inElement_L2;

    InitTemp();

    if (m_inElement_L1 == inElement_none_e) {
        if ((strcmp(name_p, "LogScrutinizer") == 0) ||  /* 0 means equal */
            (strcmp(name_p, "TextAnalysisTool.NET") == 0)) {
            m_inElement_L1 = inElement_logscrutinizer_e;
        }
    } else if (m_inElement_L1 == inElement_logscrutinizer_e) {
        if (strcmp(name_p, "filters") == 0) {
            /* 0 means equal */
            if (!m_reloadingFilter) {
                /* In-case of NOT re-load operation, for Reload the m_newFilter_p is already setup */
                m_newFilter_p = new CFilter();
                /* this will be * done twice, * once now and * once again at * end tag */
                m_newFilter_p->Init(m_filterFileName.toLatin1().constData());
            }

            m_inElement_L1 = inElement_filters_e;
        } else if (strcmp(name_p, "logs") == 0) {
            m_inElement_L1 = inElement_logs_e;
        } else if (strcmp(name_p, "bookmarks") == 0) {
            m_inElement_L1 = inElement_bookmarks_e;
        } else if (strcmp(name_p, "comments") == 0) {
            m_inElement_L1 = inElement_comments_e;
        } else if (strcmp(name_p, "plugins") == 0) {
            m_inElement_L1 = inElement_plugins_e;
        } else if (strcmp(name_p, "logs") == 0) {
            m_inElement_L1 = inElement_logs_e;
        } else if (strcmp(name_p, "searches") == 0) {
            m_inElement_L1 = inElement_searches_e;
        } else if (strcmp(name_p, "settings") == 0) {
            m_inElement_L1 = inElement_settings_e;
        } else if (strcmp(name_p, "recentfiles") == 0) {
            m_inElement_L1 = inElement_recentfiles_e;
        } else {
            TRACEX_E(QString("CConfigurationCtrl::ElementStart  Unknown L1 tag %1").arg(name_p))
            XML_Error();
            return;
        }
    } else {
        switch (m_inElement_L1)
        {
            case inElement_filters_e:
                if (strcmp(name_p, "filter") == 0) {
                    m_inElement_L2 = inElement_filter_e;
                    m_newFilterItem_p = new CFilterItem();
                }
                break;

            case inElement_logs_e:
                if (strcmp(name_p, "log") == 0) {
                    m_inElement_L2 = inElement_log_e;
                }
                break;

            case inElement_bookmarks_e:
                if (strcmp(name_p, "bookmark") == 0) {
                    m_inElement_L2 = inElement_bookmark_e;
                }
                break;

            case inElement_comments_e:
                if (strcmp(name_p, "comment") == 0) {
                    m_inElement_L2 = inElement_comment_e;
                }
                break;

            case inElement_plugins_e:
                if (strcmp(name_p, "plugin") == 0) {
                    m_inElement_L2 = inElement_plugin_e;
                }
                break;

            case inElement_searches_e:
                if (strcmp(name_p, "search") == 0) {
                    m_inElement_L2 = inElement_search_e;
                }
                break;

            case inElement_settings_e:
                if (strcmp(name_p, "setting") == 0) {
                    m_inElement_L2 = inElement_setting_e;
                }
                break;

            case inElement_recentfiles_e:
                if (strcmp(name_p, "recentfile") == 0) {
                    m_inElement_L2 = inElement_recentfile_e;
                }
                break;

            default:
                TRACEX_E("CConfigurationCtrl::ElementStart  Unknown L2 tag %s   L1:%d L2:%d",
                         name_p, m_inElement_L1, m_inElement_L2)
                XML_Error();
                return;
        }
    }

    TRACEX_DE("CConfigurationCtrl::ElementStart   L1:%d L2:%d -> L1%d L2:%d",
              l1_old, l2_old, m_inElement_L1, m_inElement_L2)
}

/***********************************************************************************************************************
*   ElementEnd
***********************************************************************************************************************/
void CConfigurationCtrl::ElementEnd(void)
{
    inElement_en l1_old = m_inElement_L1;
    inElement_en l2_old = m_inElement_L2;

    if (m_inElement_L1 == inElement_logscrutinizer_e) {
        m_inElement_L1 = inElement_none_e;
        m_inElement_L2 = inElement_none_e;
    } else if (m_inElement_L2 == inElement_none_e) {
        switch (m_inElement_L1)
        {
            case inElement_logs_e:
                break;

            case inElement_filters_e:
            {
                CLogScrutinizerDoc *doc_p = GetTheDoc();

                if (m_reloadingFilter && (m_newFilter_p != nullptr)) {
                    m_newFilter_p->Init(m_filterFileName.toLatin1().constData());
                    (void)g_workspace_p->ReinsertCfgFilter(m_cfgFilterReload_p);
                } else if (m_newFilter_p != nullptr) {
                    /* put it into treeview */
                    doc_p->m_recentFiles.AddFile(m_filterFileName, false);

                    m_newFilter_p->Init(m_filterFileName.toLatin1().constData());

                    if (m_inWorkspace) {
                        m_newFilter_p->m_fileName_short += " #workspace";
                    }

                    g_workspace_p->CreateCfgFilter(m_newFilter_p);
                }

                m_reloadingFilter = false;    /* Just to be sure... */
                m_cfgFilterReload_p = nullptr;
                m_newFilter_p = nullptr;
                break;
            }

            case inElement_bookmarks_e:
                break;

            case inElement_comments_e:
                break;

            case inElement_plugins_e:
                break;

            case inElement_searches_e:
                break;

            case inElement_settings_e:
                break;

            case inElement_recentfiles_e:
                break;

            default:
                TRACEX_E("CConfigurationCtrl::ElementEnd   Parse Error   L1:%d L2:%d", m_inElement_L1, m_inElement_L2)
                XML_Error();
                return;
        }

        m_inElement_L1 = inElement_logscrutinizer_e;
        m_inElement_L2 = inElement_none_e;
    } else {
        switch (m_inElement_L2)
        {
            case inElement_log_e:
            {
                CLogScrutinizerDoc *doc_p = GetTheDoc();
                QString absolutePath;
                QString relativePath(m_tempText);

                doc_p->GetAbsoluteFileName(relativePath, absolutePath);

                if (CanFileBeOpenedForFileRead(absolutePath, false)) {
                    g_workspace_p->RemoveLog(); /* ensure that if the loadlogfile fails the tree will be updated with
                                                 * that the log has been cleaned out */
                    if (doc_p->LoadLogFile(absolutePath)) {
                        g_workspace_p->AddLog(m_tempText);
                    }
                }
                break;
            }

            case inElement_filter_e:   /* Add filter item */
                if ((m_newFilter_p != nullptr) && (m_newFilterItem_p != nullptr)) {
                    CLogScrutinizerDoc *doc_p = GetTheDoc();

                    m_newFilterItem_p->m_font_p = doc_p->m_fontCtrl.RegisterFont(m_newFilterItem_p->m_color,
                                                                                 m_newFilterItem_p->m_bg_color);
                    /* This will not affect the current model * as it is added to a stand alone object */
                    m_newFilter_p->m_filterItemList.append(m_newFilterItem_p);
                }

                m_newFilterItem_p = nullptr;
                break;

            case inElement_bookmark_e:
                if ((g_workspace_p != nullptr) && !g_workspace_p->isBookmarked(m_tempRow)) {
                    g_workspace_p->AddBookmark(QString(m_tempText), m_tempRow);
                }
                break;

            case inElement_comment_e:
                break;

            case inElement_plugin_e:
                if (CanFileBeOpenedForFileRead(QString(m_tempText)) && !isDuplicate(QString(m_tempText))) {
                    LoadPlugin(QString(m_tempText));
                }
                break;

            case inElement_search_e:
                break;

            case inElement_setting_e:
                ElementEnd_Setting();
                break;

            case inElement_recentfile_e:
                ElementEnd_RecentFile();
                break;

            default:
                TRACEX_E("CConfigurationCtrl::ElementEnd   Parse Error   L1:%d L2:%d", m_inElement_L1, m_inElement_L2)
                XML_Error();
                return;
        }

        m_inElement_L2 = inElement_none_e;
    }

    TRACEX_DE("CConfigurationCtrl::ElementEnd   L1:%d L2:%d -> L1:%d L2:%d",
              l1_old, l2_old, m_inElement_L1, m_inElement_L2)
}

/***********************************************************************************************************************
*   Element_Attribute
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute(char *name_p, char *value_p)
{
    if (m_inElement_L2 != inElement_none_e) {
        TRACEX_D("Element_Attribute  name:%-16s value:%s", name_p, value_p)

        switch (m_inElement_L2)
        {
            case inElement_log_e:
                Element_Attribute_Log(name_p, value_p);
                break;

            case inElement_filter_e:
                Element_Attribute_FilterItem(name_p, value_p);
                break;

            case inElement_bookmark_e:
                Element_Attribute_Bookmark(name_p, value_p);
                break;

            case inElement_comment_e:
                Element_Attribute_Comment(name_p, value_p);
                break;

            case inElement_plugin_e:
                Element_Attribute_Plugin(name_p, value_p);
                break;

            case inElement_search_e:
                Element_Attribute_Search(name_p, value_p);
                break;

            case inElement_setting_e:
                Element_Attribute_Setting(name_p, value_p);
                break;

            case inElement_recentfile_e:
                Element_Attribute_RecentFile(name_p, value_p);
                break;

            default:
                TRACEX_E("CConfigurationCtrl::Element_Attribute   Parse Error  Unknown L2?   L1:%d L2:d",
                         m_inElement_L1, m_inElement_L2)
                XML_Error();
                return;
        } /* switch */
    } else if (m_inElement_L1 != inElement_logscrutinizer_e) {
        switch (m_inElement_L1)
        {
            case inElement_logs_e:
            case inElement_comments_e:
            case inElement_bookmarks_e:
            case inElement_plugins_e:
            case inElement_search_e:
            case inElement_settings_e:
            case inElement_recentfiles_e:
                break;

            case inElement_filters_e:
                Element_Attribute_Filter(name_p, value_p);
                break;

            default:
                TRACEX_E("CConfigurationCtrl::Element_Attribute   Parse Error  Unknown L2?   L1:%d L2:d",
                         m_inElement_L1, m_inElement_L2)
                XML_Error();
                return;
        } /*switch */
    } else {
        if (strcmp(name_p, "version") == 0) {
            strcpy_s(m_tempStr, CFG_TEMP_STRING_MAX_SIZE, value_p);
            m_version = atoi(m_tempStr);
            if (m_version != LCZ_FILE_VERSION) {}
        }
    }
}

/***********************************************************************************************************************
*   Element_Attribute_Filter
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Filter(char *name_p, char *value_p)
{
    if (strcmp(name_p, "name") == 0) {
        m_filterFileName = value_p;

        if (!(m_filterFileName_Loaded.isEmpty()) && !(m_filterFileName.isEmpty()) &&
            (m_filterFileName_Loaded != m_filterFileName)) {
            m_filterFileName = m_filterFileName_Loaded;
        }
    }
}

/***********************************************************************************************************************
*   Element_Attribute_FilterItem
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_FilterItem(char *name_p, char *value_p)
{
    /*if (m_inFilterTag && m_newFilterItem_p != nullptr) */
    {
        if (strcmp(name_p, "text") == 0) {
            size_t size = strlen(value_p); /*do not include the 0 */
            m_newFilterItem_p->m_size = static_cast<int>(size);
            m_newFilterItem_p->m_freeStartRef = true;

            m_newFilterItem_p->m_start_p = reinterpret_cast<char *>(malloc(size + 1));

            if (m_newFilterItem_p->m_start_p != nullptr) {
                memcpy(m_newFilterItem_p->m_start_p, value_p, size_t(size));
                m_newFilterItem_p->m_start_p[size] = 0;
            } else {
                TRACEX_E("CConfigurationCtrl::Element_Attribute_FilterItem   m_newFilterItem_p->m_start_p nullptr ")
            }
        } else if (strcmp(name_p, "color") == 0) {
            Q_COLORREF color;
            QString value_s(value_p);

            if (!value_s.contains("0x", Qt::CaseInsensitive)) {
                value_s.insert(0, 'x');
                value_s.insert(0, '0');
            }

            QTextStream reader(&value_s);
            reader.setIntegerBase(16);
            reader >> color; /* Value is RGB, 0xrrggbb, however without the 0x */
            m_newFilterItem_p->m_color = color;
        } else if (strcmp(name_p, "bg_color") == 0) {
            Q_COLORREF color;
            QString value_s(value_p);

            if (!value_s.contains("0x", Qt::CaseInsensitive)) {
                value_s.insert(0, 'x');
                value_s.insert(0, '0');
            }

            QTextStream reader(&value_s);
            reader.setIntegerBase(16);
            reader >> color; /* Value is RGB, 0xrrggbb, however without the 0x */
            m_newFilterItem_p->m_bg_color = color;
        } else if (strcmp(name_p, "enabled") == 0) {
            if (value_p[0] == 'n') {
                m_newFilterItem_p->m_enabled = false;
            } else {
                m_newFilterItem_p->m_enabled = true;
            }
        } else if (strcmp(name_p, "case_sensitive") == 0) {
            if (value_p[0] == 'y') {
                m_newFilterItem_p->m_caseSensitive = true;
            } else {
                m_newFilterItem_p->m_caseSensitive = false;
            }
        } else if (strcmp(name_p, "regex") == 0) {
            if (value_p[0] == 'y') {
                m_newFilterItem_p->m_regexpr = true;
            } else {
                m_newFilterItem_p->m_regexpr = false;
            }
        } else if (strcmp(name_p, "excluding") == 0) {
            if (value_p[0] == 'y') {
                m_newFilterItem_p->m_exclude = true;
            } else {
                m_newFilterItem_p->m_exclude = false;
            }
        } else if (strcmp(name_p, "adaptive_clip") == 0) {
            if (value_p[0] == 'y') {
                m_newFilterItem_p->m_adaptiveClipEnabled = true;
            } else {
                m_newFilterItem_p->m_adaptiveClipEnabled = false;
            }
        }
    }

    TRACEX_D("Element_Attribute_FilterItem %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_Log
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Log(char *name_p, char *value_p)
{
    if (strcmp(name_p, "path") == 0) {
        strcpy_s(m_tempText, CFG_TEMP_STRING_MAX_SIZE, value_p);
    }
    TRACEX_D("Element_Attribute_Log %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_Bookmark
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Bookmark(char *name_p, char *value_p)
{
    if (strcmp(name_p, "text") == 0) {
        strcpy_s(m_tempText, CFG_TEMP_STRING_MAX_SIZE, value_p);
    } else if (strcmp(name_p, "row") == 0) {
        strcpy_s(m_tempStr, CFG_TEMP_STRING_MAX_SIZE, value_p);
        m_tempRow = atoi(m_tempStr);
    }

    TRACEX_D("Element_Attribute_Bookmark %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_Plugin
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Plugin(char *name_p, char *value_p)
{
    if (strcmp(name_p, "path") == 0) {
        strcpy_s(m_tempText, CFG_TEMP_STRING_MAX_SIZE, value_p);
    }

    TRACEX_D("Element_Attribute_Plugin %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_Setting
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Setting(char *name_p, char *value_p)
{
    QString tag = name_p;
    QString value = value_p;
    g_cfg_p->SetSetting(&tag, &value);

    TRACEX_D("Element_Attribute_Setting %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_RecentFile
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_RecentFile(char *name_p, char *value_p)
{
    QString tag = name_p;
    QString value = value_p;

    /*    <recentfiles>
     *      <kind="1" path="c:\...." / timestamp="21210012">
     *      <kind="1" path="c:\...." / timestamp="037231423">
     *    </recentfiles> */
    if (strcmp(name_p, "kind") == 0) {
        m_recentFile.kind = static_cast<RecentFile_Kind_e>(atoi(value_p));
    } else if (strcmp(name_p, "path") == 0) {
        m_recentFile.filePath = value_p;
    } else if (strcmp(name_p, "timestamp") == 0) {
        QTextStream reader(value_p);
        reader >> m_recentFile.lastAccessed;
    }

    TRACEX_D("Element_Attribute_RecentFile %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   ElementEnd_Setting
***********************************************************************************************************************/
void CConfigurationCtrl::ElementEnd_Setting(void)
{
    switch (m_inSetting)
    {
        default:
            break;
    }

    m_inSetting = inSetting_none_e;
}

/***********************************************************************************************************************
*   ElementEnd_RecentFile
***********************************************************************************************************************/
void CConfigurationCtrl::ElementEnd_RecentFile(void)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->m_recentFiles.AddFile(m_recentFile.kind, &m_recentFile.filePath, &m_recentFile.lastAccessed, false);
}

/***********************************************************************************************************************
*   Element_Attribute_Comment
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Comment(char *name_p, char *value_p)
{
    TRACEX_D("Element_Attribute_Comment %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Attribute_Search
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Attribute_Search(char *name_p, char *value_p)
{
    TRACEX_D("Element_Attribute_Search %s=%s", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Value
***********************************************************************************************************************/
void CConfigurationCtrl::Element_Value(char *value_p)
{
    TRACEX_D("(Value=%s)", value_p)
}
