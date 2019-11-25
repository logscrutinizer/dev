/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <time.h>
#include "globals.h"
#include "CXMLBase.h"

typedef enum {
    RecentFile_Kind_LogFile_en,
    RecentFile_Kind_FilterFile_en,
    RecentFile_Kind_PluginFile_en,
    RecentFile_Kind_WorkspaceFile_en,
}RecentFile_Kind_e;

typedef struct {
    RecentFile_Kind_e kind;
    QString filePath; /* complete path to the file */
    QString fileName; /* short file name */
    time_t lastAccessed;
}RecentFileInfo_t;

/***********************************************************************************************************************
*   CRecentFile
***********************************************************************************************************************/
class CRecentFile
{
public:
    CRecentFile() {TRACEX_DE("CRecentFile::CRecentFile")}
    ~CRecentFile() {Clean();}

    void Clean(void);
    void Test(void);
    void ReadFromFile(void);
    void WriteToFile(bool sync = true);
    void Sort(void);

    /*! Trim will adjust the recent file history to not be larger than configured. The oldest files are removed */
    void Trim(void);

    void AddFile(RecentFile_Kind_e kind, const QString *filePath_p,
                 time_t *timeStamp_p = nullptr, bool updateMenu = true);
    void AddFile(const QString& filePath, bool updateMenu = true);

    bool isRecentFiles(void) {return (m_recentFileList.isEmpty() ? false : true);}
    void DumpDatabase(void);
    void RemoveFile(QString *filePath_p);

    RecentFileInfo_t *GetRecentFile(const QString *filePath_p);
    QString GetRecentPath(RecentFile_Kind_e kind);

    /*! Set dir_only if the list shall contain directories and not files */
    void GetRecentPaths(QStringList& list, const QList<RecentFile_Kind_e>& kindList,
                        bool dir_only = false);

    void UpdateLastAccessed(QString *filePath_p);
    void UpdateLastAccessed(RecentFileInfo_t *fileInfo_p);
    void SetUserData(RecentFileInfo_t *fileInfo_p, char *userData_p);

private:
    bool QualifyFileInfo(RecentFileInfo_t *fileInfo_p);

    /*! QualifyAndClean will check all fileInfos in the recentFile database, any item that checks wrong or doesn't
     * exist is removed. If there was any changes the database is saved to file. */
    void QualifyAndClean(void);
    void RemoveFileInfo(RecentFileInfo_t *removeFileInfo_p);

private:
    QList<RecentFileInfo_t *> m_recentFileList;
};
