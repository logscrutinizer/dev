/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CConfig.h"
#include "CDebug.h"
#include "limits.h"
#include "CRecentFile.h"
#include "CConfigurationCtrl.h"
#include "../qt/mainwindow_cb_if.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QApplication>

/***********************************************************************************************************************
*   Clean
***********************************************************************************************************************/
void CRecentFile::Clean(void)
{
    while (!m_recentFileList.isEmpty()) {
        auto item = m_recentFileList.takeLast();
        if (nullptr != item) {
            delete (item);
        }
    }
}

/***********************************************************************************************************************
*   Test
***********************************************************************************************************************/
void CRecentFile::Test(void)
{
    RecentFileInfo_t *fileInfo1_p, *fileInfo2_p, *fileInfo3_p, *fileInfoVerify_p;
    QFile testFile("testrecent.rcnt");

    if (testFile.exists()) {
        if (!testFile.remove()) {
            TRACEX_E("Failed to remove testrecent.rcnt, test aborted")
            return;
        }
    }

    if (!testFile.open(QIODevice::ReadWrite)) {
        TRACEX_E("Failed to open testrecent.rcnt, test aborted")
        return;
    }

    testFile.close();

    QString temp = g_cfg_p->m_defaultRecentFileDB;

    g_cfg_p->m_defaultRecentFileDB = "testrecent.rcnt";

    fileInfo1_p = new RecentFileInfo_t;
    fileInfo2_p = new RecentFileInfo_t;
    fileInfo3_p = new RecentFileInfo_t;

    fileInfo1_p->lastAccessed = 10;
    fileInfo2_p->lastAccessed = 20;
    fileInfo3_p->lastAccessed = 30;

    m_recentFileList.append(fileInfo1_p);
    m_recentFileList.append(fileInfo2_p);
    m_recentFileList.append(fileInfo3_p);

    Sort();

    fileInfoVerify_p = m_recentFileList.first();

    if (fileInfoVerify_p->lastAccessed != 30) {
        TRACEX_E("CRecentFile::Test   SORT ERROR")
    }

    int recentFileHistory = g_cfg_p->m_recentFile_MaxHistory;   /* temporarily modify the max count */

    g_cfg_p->m_recentFile_MaxHistory = 1;

    Trim();
    WriteToFile(false);

    if (m_recentFileList.count() != 1) {
        TRACEX_E("CRecentFile::Test   ERROR   Trim failed")
    }

    g_cfg_p->m_recentFile_MaxHistory = recentFileHistory;

    Clean();

    if (!m_recentFileList.isEmpty()) {
        TRACEX_E("CRecentFile::Test   ERROR   DB not empty after test")
    }

    g_cfg_p->m_defaultRecentFileDB = temp;

    testFile.remove();
}

/***********************************************************************************************************************
*   GetRecentFile
***********************************************************************************************************************/
RecentFileInfo_t *CRecentFile::GetRecentFile(const QString *filePath_p)
{
    if (m_recentFileList.isEmpty()) {
        return nullptr;
    }

    for (int index = 0; index < m_recentFileList.count(); ++index) {
        RecentFileInfo_t *fileInfo_p = m_recentFileList[index];

        if (fileInfo_p->filePath == *filePath_p) {
            return fileInfo_p;
        }
    }

    return nullptr;
}

/***********************************************************************************************************************
*   UpdateLastAccessed
***********************************************************************************************************************/
void CRecentFile::UpdateLastAccessed(QString *filePath_p)
{
    UpdateLastAccessed(GetRecentFile(filePath_p));
}

/***********************************************************************************************************************
*   UpdateLastAccessed
***********************************************************************************************************************/
void CRecentFile::UpdateLastAccessed(RecentFileInfo_t *fileInfo_p)
{
    /*m_recentFileList */

    if ((fileInfo_p == nullptr) || m_recentFileList.isEmpty()) {
        return;
    }

    time(&fileInfo_p->lastAccessed);
}

/***********************************************************************************************************************
*   AddFile
***********************************************************************************************************************/
void CRecentFile::AddFile(const QString& filePath, bool updateMenu)
{
    RecentFile_Kind_e kind = RecentFile_Kind_LogFile_en;
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix();

    if ((ext == "txt") || (ext == "log")) {
        kind = RecentFile_Kind_LogFile_en;
    } else if (ext == DLL_EXT) {
        kind = RecentFile_Kind_PluginFile_en;
    } else if (ext == "lsz") {
        kind = RecentFile_Kind_WorkspaceFile_en;
    } else if ((ext == "tat") || (ext == "flt")) {
        kind = RecentFile_Kind_FilterFile_en;
    } else if (ext == "rcnt") {
        return;
    }

    AddFile(kind, &filePath, nullptr, updateMenu);
}

/***********************************************************************************************************************
*   AddFile
***********************************************************************************************************************/
void CRecentFile::AddFile(RecentFile_Kind_e kind, const QString *filePath_p, time_t *timeStamp_p, bool updateMenu)
{
    RecentFileInfo_t *recentFileInfo_p;
    bool isFileValid = true;
    QFileInfo fileInfo(*filePath_p);

    fileInfo.refresh();
    if (!fileInfo.exists()) {
        isFileValid = false;
    }

    if ((recentFileInfo_p = GetRecentFile(filePath_p)) != nullptr) {
        if (!isFileValid) {
            /* The file didn't exist, as such remove the already existing fileInfo as well. */
            RemoveFileInfo(recentFileInfo_p);
            return;
        }
        if (timeStamp_p != nullptr) {
            if (*timeStamp_p > recentFileInfo_p->lastAccessed) {
                recentFileInfo_p->lastAccessed = *timeStamp_p;
            }
        } else {
            time(&recentFileInfo_p->lastAccessed);
        }
        Sort();
        Trim();

        if (updateMenu) {
            MW_RebuildRecentFileMenu();
        }
        return;
    }

    /* Since the file didn't exist it shouldn't be added to the recentlist either. */
    if (!isFileValid) {
        return;
    }

    recentFileInfo_p = new RecentFileInfo_t;

    recentFileInfo_p->kind = kind;
    recentFileInfo_p->filePath = *filePath_p;

    if (timeStamp_p != nullptr) {
        recentFileInfo_p->lastAccessed = *timeStamp_p;
    } else {
        time(&recentFileInfo_p->lastAccessed);
    }

    recentFileInfo_p->fileName = fileInfo.fileName();

    m_recentFileList.insert(0, recentFileInfo_p);

    if (updateMenu) {
        MW_RebuildRecentFileMenu();
    }
}

/***********************************************************************************************************************
*   QualifyAndClean
***********************************************************************************************************************/
void CRecentFile::QualifyAndClean(void)
{
    bool databaseModified = false;
    bool loop = false;
    do {
        loop = false;
        for (auto& entry : m_recentFileList) {
            bool fileValid = QualifyFileInfo(entry);
            if (!fileValid) {
                m_recentFileList.removeOne(entry);
                databaseModified = true;
                loop = true;
                break;
            }
        }
    } while (loop);

    if (databaseModified) {
        WriteToFile();
    }
}

/***********************************************************************************************************************
*   QualifyFileInfo
***********************************************************************************************************************/
bool CRecentFile::QualifyFileInfo(RecentFileInfo_t *fileInfo_p)
{
    if ((fileInfo_p->kind == RecentFile_Kind_LogFile_en) ||
        (fileInfo_p->kind == RecentFile_Kind_FilterFile_en) ||
        (fileInfo_p->kind == RecentFile_Kind_PluginFile_en) ||
        (fileInfo_p->kind == RecentFile_Kind_WorkspaceFile_en)) {
        QFileInfo qfileInfo(fileInfo_p->filePath);
        qfileInfo.refresh();
        if (qfileInfo.exists() && !fileInfo_p->fileName.isEmpty() && !fileInfo_p->filePath.isEmpty()) {
            return true;
        } else {
            TRACEX_I(QString("Recent file doesn't exist any more: %1").arg(fileInfo_p->filePath))
        }
    }
    return false;
}

/***********************************************************************************************************************
*   DumpDatabase
***********************************************************************************************************************/
void CRecentFile::DumpDatabase(void)
{
#if 0
    if (m_recentFileList.isEmpty()) {
        TRACEX_I("CRecentFile::DumpDatabase   EMPTY")
        return;
    }

    RecentFileInfo_t *fileInfo_p;

    TRACEX_I(QString("Recent files"))
    for (int index = 0; index < m_recentFileList.count(); ++index) {
        fileInfo_p = m_recentFileList[index];
        TRACEX_I(
            QString("Time:%2 Kind:%3  %4")
                .arg(fileInfo_p->lastAccessed).arg(fileInfo_p->kind).arg(fileInfo_p->filePath));
    }
#endif
}

/***********************************************************************************************************************
*   RemoveFileInfo
***********************************************************************************************************************/
void CRecentFile::RemoveFileInfo(RecentFileInfo_t *removeFileInfo_p)
{
    if (m_recentFileList.isEmpty()) {
        TRACEX_W("Internal Warning, fileInfo not found in recentFile list (EMPTY)")
        return;
    }

    RecentFileInfo_t *fileInfo_p;

    for (int index = 0; index < m_recentFileList.count(); ++index) {
        fileInfo_p = m_recentFileList[index];

        if (fileInfo_p == removeFileInfo_p) {
            delete (m_recentFileList.takeAt(index));
            return;
        }
    }

    TRACEX_W("Internal Warning, fileInfo not found in recentFile list")
}

/***********************************************************************************************************************
*   RemoveFile
***********************************************************************************************************************/
void CRecentFile::RemoveFile(QString *filePath_p)
{
    if (m_recentFileList.isEmpty()) {
        TRACEX_W("Internal Warning, filePath not found in recentFile list (EMPTY)")
        return;
    }

    ReadFromFile();
    Sort();

    RecentFileInfo_t *fileInfo_p;

    for (int index = 0; index < m_recentFileList.count(); ++index) {
        fileInfo_p = m_recentFileList[index];

        if (fileInfo_p->filePath == *filePath_p) {
            delete (m_recentFileList.takeAt(index));
            WriteToFile(false);
            return;
        }
    }

    TRACEX_W("Internal Warning, filePath not found in recentFile list")
}

/***********************************************************************************************************************
*   Sort
***********************************************************************************************************************/
void CRecentFile::Sort(void)
{
    /* Bubble sort */

    if (m_recentFileList.isEmpty() || (m_recentFileList.count() < 2)) {
        return;
    }

#ifdef _DEBUG
    DumpDatabase();  /* Only in _DEBUG */
#endif

    bool swapWasDone;
    RecentFileInfo_t *fileInfo_1_p;
    RecentFileInfo_t *fileInfo_2_p;

    /* iterate through list over and over again until no swap was done */
    do {
        swapWasDone = false;
        for (int index = 0; index < m_recentFileList.count() - 1; ++index) {
            fileInfo_1_p = m_recentFileList[index];
            fileInfo_2_p = m_recentFileList[index + 1];

            if (fileInfo_1_p->lastAccessed < fileInfo_2_p->lastAccessed) {
                m_recentFileList.swapItemsAt(index, index + 1);
                swapWasDone = true;
            }
        }
    } while (swapWasDone);

    DumpDatabase();
}

/***********************************************************************************************************************
*   Trim
***********************************************************************************************************************/
void CRecentFile::Trim(void)
{
    if (m_recentFileList.isEmpty() || (m_recentFileList.count() < g_cfg_p->m_recentFile_MaxHistory)) {
        return;
    }

    TRACEX_D("CRecentFile::Trim   Remove %d items from recent file history",
             (m_recentFileList.count() - g_cfg_p->m_recentFile_MaxHistory))

    while (m_recentFileList.count() > g_cfg_p->m_recentFile_MaxHistory) {
        auto item = m_recentFileList.takeLast();
        if (nullptr != item) {
            delete (item);
        }
    }
}

/***********************************************************************************************************************
*   ReadFromFile
***********************************************************************************************************************/
void CRecentFile::ReadFromFile(void)
{
    QList<QString> fileList;
    QFileInfo fileInfo(CFGCTRL_GetDefaultWorkspaceFilePath());
    QString recentFileName = fileInfo.dir().filePath(g_cfg_p->m_defaultRecentFileDB);
    QFile file(recentFileName);

    /* First check that the file can be opened */
    if (!file.open(QIODevice::ReadOnly)) {
        QString msg;
        QTextStream msgStream(&msg);

        if (file.exists()) {
            msgStream << "Could not open recent file database, " << recentFileName
                      << ", it exists but might be locked use by some other application." << Qt::endl;
        } else {
            msgStream << "Could not open recent file database, " << recentFileName << ", it DO NOT exist" << Qt::endl;
        }

        TRACEX_W(&msg)

        return;
    }
    fileList.insert(0, recentFileName);
    CFGCTRL_LoadFiles(fileList);
}

/***********************************************************************************************************************
*   WriteToFile
***********************************************************************************************************************/
void CRecentFile::WriteToFile(bool sync)
{
    /* SYNCH from file first */

    if (sync) {
        ReadFromFile();
        Sort();
    }

    if (m_recentFileList.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(CFGCTRL_GetDefaultWorkspaceFilePath());
    QString recentFileName = fileInfo.dir().filePath(g_cfg_p->m_defaultRecentFileDB);
    QFile file(recentFileName);

    /* First check that the file can be opened */
    if (!file.open(QIODevice::ReadWrite)) {
        QString msg;
        QTextStream msgStream(&msg);

        if (file.exists()) {
            msgStream << "Could not open recent file database, " << recentFileName
                      << ", it exists but might be locked use by some other application." << Qt::endl;
        } else {
            msgStream << "Could not open recent file database, " << recentFileName
                      << ", it DO NOT exist" << Qt::endl;
        }
        TRACEX_W(&msg)
        return;
    }

    file.resize(0);

    QTextStream fileStream(&file);

    try {
        fileStream << XML_FILE_HEADER << Qt::endl; /* "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>" */
        fileStream << LCZ_HEADER << Qt::endl; /* "<LogScrutinizer version=\"2\">"   // file format version */
        fileStream << RECENTFILE_HEADER << Qt::endl; /* "<recentfiles>" */

        RecentFileInfo_t *fileInfo_p;

        for (int index = 0; index < m_recentFileList.count(); ++index) {
            fileInfo_p = m_recentFileList[index];

            fileStream << "  <recentfile path=\"" << fileInfo_p->filePath <<
                "\" kind=\"" << fileInfo_p->kind <<
                "\" timestamp=\"" << fileInfo_p->lastAccessed <<
                "\" />" << Qt::endl;
        }

        fileStream << RECENTFILE_FOOTER << Qt::endl; /* </recentfiles> */
        fileStream << LCZ_FOOTER << Qt::endl; /* </LogScrutinizer> */
    } catch (std::exception &e) {
        qFatal("Error %s writing recent file database to file", e.what());
        TRACEX_E("CRecentFile::WriteToFile  Failed to write recent file database to file,  Err:%s", e.what())
        return;
    } catch (...) {
        qFatal("Unknown Error writing recent file database to file");
        TRACEX_E("CRecentFile::WriteToFile  Failed to write recent file database to file,  Unknown error")
        return;
    }

    fileStream.flush();
    file.close();
    MW_RebuildRecentFileMenu();
}

/***********************************************************************************************************************
*   GetRecentPath
***********************************************************************************************************************/
QString CRecentFile::GetRecentPath(RecentFile_Kind_e kind)
{
    QualifyAndClean();
    Sort();

    if (m_recentFileList.isEmpty()) {
        return "";
    }

    for (auto& recentFileInfo_p : m_recentFileList) {
        if (recentFileInfo_p->kind == kind) {
            QFileInfo info(recentFileInfo_p->filePath);
            return info.canonicalFilePath();
        }
    } /* for */

    return "";
}

/***********************************************************************************************************************
*   GetRecentPaths
***********************************************************************************************************************/
void CRecentFile::GetRecentPaths(QStringList& list, const QList<RecentFile_Kind_e>& kindList, bool dir_only)
{
    /* Create a list of UNIQUE path strings to directories used previously.
     * Also, the paths already in inList are considered, if any */

    QualifyAndClean();
    Sort();

#ifdef _DEBUG
    DumpDatabase();
#endif

    for (auto& kind : kindList) {
        for (auto fileInfo_p : m_recentFileList) {
            if (fileInfo_p->kind == kind) {
                QFileInfo info(fileInfo_p->filePath);
                QString path;
                if (dir_only) {
                    path = info.canonicalPath();
                } else {
                    path = info.canonicalFilePath();
                }
                if (!path.isEmpty() && !list.contains(path)) {
                    list.append(path);
                }
            }
        }
    }
}
