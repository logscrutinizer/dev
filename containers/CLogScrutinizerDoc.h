/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CFilter.h"
#include "CFileCtrl.h"
#include "CFontCtrl.h"
#include "CTimeMeas.h"
#include "CXMLBase.h"
#include "CMemPool.h"
#include "CRecentFile.h"
#include "CCfgItem.h"
#include "CSelection.h"
#include "TextDecoration.h"

#include <memory>
#include <QDir>
#include <QObject>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QTimer>

#define PIXEL_STAMP_STARTX_MASK      ((static_cast<uint64_t>(1) << 32) - 1)          /* bit 0-31 start_X, 32 bits */
#define PIXEL_STAMP_STARTY_MASK      ((static_cast<uint64_t>(1 << 23) - 1) << 32)    /* bit 32-54, bit 23 bits */
#define PIXEL_STAMP_FONT_MASK        ((static_cast<uint64_t>(1 << 8) - 1) << 56)    /* bit 32-54, bit 8 bits */

#define PIXEL_STAMP_GET_STARTX(A) \
    (static_cast<uint64_t>(static_cast<uint64_t>(A) & PIXEL_STAMP_STARTX_MASK))
#define PIXEL_STAMP_GET_STARTY(A) \
    (static_cast<uint64_t>(static_cast<uint64_t>(A & PIXEL_STAMP_STARTY_MASK) >> 32))
#define PIXEL_STAMP_GET_FONT(A) \
    (static_cast<uint64_t>(static_cast<uint64_t>(A & PIXEL_STAMP_FONT_MASK)) >> 56)

#define PIXEL_STAMP_SET_STARTX(PIXELSTAMP, STARTX) \
    (PIXELSTAMP = static_cast<uint64_t>(((PIXELSTAMP)&static_cast<uint64_t>(~(PIXEL_STAMP_STARTX_MASK))) | STARTX))
#define PIXEL_STAMP_SET_STARTY(PIXELSTAMP, STARTY) \
    (PIXELSTAMP = static_cast<uint64_t>(((PIXELSTAMP)&static_cast<uint64_t>(~(PIXEL_STAMP_STARTY_MASK))) | \
                                        (static_cast<uint64_t>(STARTY) << 32)))

#define PIXEL_STAMP_SET_FONT(PIXELSTAMP, FONT) \
    (PIXELSTAMP = static_cast<uint64_t>((static_cast<uint64_t>((PIXELSTAMP)) & \
                                         static_cast<uint64_t>((~PIXEL_STAMP_FONT_MASK))) | \
                                        static_cast<uint64_t>((FONT)) << \
                                        56))

#ifdef WIN32
 #define FILE_CHANGE_TIMER_DURATION 1000 /* 1 sec, since there is a bug on win32 platforms, the timer is the only
                                          * trigger to get updates */
#else
 #define FILE_CHANGE_TIMER_DURATION 5000 /* 5 sec, backup if the file system monitor somehow fails */
#endif

typedef struct {
    int64_t fileSize;
    TIA_t TIA;  /* The Text Item Array   (all rows in the file tagged up) */
    FIRA_t FIRA;     /* Array that contains the filters matching each row in the TIA */

    /* Only the filter item hits, doesn't included the exclude filters, created after each filtering */
    packed_FIR_t *packedFIRA_p;
    CFilterItem *filterItem_LUT[MAX_NUM_OF_ACTIVE_FILTERS];  /* Used to be able to */
} DB_t;

typedef enum {
    /* The order is important */
    RS_Skip,

    /* Reload not necessary */
    RS_Incremental,

    /* Just incrementally add the new tail of the log */
    RS_Full /* Clean DB and reload entire log */
} ReloadStrategy_e;

extern void CEditorWidget_EmptySelectionList(void);

/***********************************************************************************************************************
*   CLogScrutinizerDoc
***********************************************************************************************************************/
class CLogScrutinizerDoc : public QObject
{
    Q_OBJECT

public:
    CLogScrutinizerDoc();
    ~CLogScrutinizerDoc() {m_inDestructor = true;}

    /* Operations */

public:
    void CleanDB(bool reload = false, bool unloadFilters = true);
    void VerifyDB(void);

    bool SaveRowsToFile(char *fileName_p, int startRow, int endRow);

    void UpdateTitleRow(void);

    void ClearDecoders(void);
    void AddDecoders(QList<CCfgItem *> *decoderList_p);
    void RemoveDecoder(CCfgItem *cfgDecoder_p);
    void Decode(int cacheIndex);

    /****/
    void SetWorkspaceFileName(const QString& fileName) {
        m_workspaceFileName_revert = m_workspaceFileName;
        m_workspaceFileName = fileName;
        TRACEX_D(QString("Current workspace file: %1\n").arg(fileName))
    }

    /****/
    void RevertWorkspaceFileName() {
        m_workspaceFileName = m_workspaceFileName_revert;
    }

    /****/
    QString GetWorkspaceFileName(void) {
        return m_workspaceFileName;
    }

    /* the aboslute file name is in relation to the workspace */
    void GetAbsoluteFileName(const QString& relativeFileName, QString& absoluteFileName);
    QString GetRecommendedPath(void);
    QDir GetWorkspaceDirectory();
    void ReNumerateFIRA(void);
    void CreatePackedFIRA(void);

    /****/
    void CleanRowCache(void) {
        if (m_rowCache_p != nullptr) {
            m_rowCache_p->Clean();
        }
    }

    /****/
    bool isValidLog(void) {
        if (m_database.TIA.rows > 0) {return true;}
        return false;
    }

    /****/
    void GetTextItem(int rowIndex, char **text_p, int *size_p, int *properties_p = nullptr) {
        m_rowCache_p->Get(rowIndex, text_p, size_p, properties_p);
    }

    /****/
    QString GetTextItem(int rowIndex) {
        char *text_p;
        int size, properties;
        m_rowCache_p->Get(rowIndex, &text_p, &size, &properties);
        return QString(text_p);
    }

    bool CanFileBeOpenedForFileRead(QString& fileName, bool promtWarning);
    bool CanFileBeOpenedForFileReadWrite(QString& fileName, bool promtWarning, bool tryOtherName);

    static QString FileSizeToString(int64_t size);
    static QString timeToString(int64_t ms);

    void UnloadFilters(void);
    void CreateFiltersFromContainer(CFilterContainer& container);
    void CreateFiltersFromCCfgItems(void);
    bool CheckAllFilters(void);
    void UpdateFilterItem(int uniqueID, Q_COLORREF color, Q_COLORREF bg_color);

    void PluginIsUnloaded(void);

    bool LoadLogFile(QString fileName, bool reload = false); /* Will start the progress dialog */
    void ExecuteLoadLog(void); /* Is run  from the progress dialog */

    /* logUpdated is used after the log has been loaded and memory mapped to inform other components such
     * rowcache about the changes */
    void logUpdated(int64_t fileSize);
    void replaceFileSysWatcherFile(QString& fileName);

    void Filter(void);
    void ExecuteFiltering(void);
    void PostProcFiltering(void);

    void ExecuteIncrementalFiltering(int startRow);
    void StartOneLineFiltering(int row, bool isBookmarkRemove = false);

    bool StartSearch(const QString& searchText, int startRow, int endRow,
                     int *row_p, bool backward, bool onlyFiltered, bool regExp, bool caseSensitive);
    bool ExecuteSearch(void);
    bool PostProcSearch(void);
    bool StartPlot(QList<CPlot *> *pendingPlot_execList_p, int startRow = 0, int endRow = 0);
    bool ExecutePlot(void);
    void PostProcPlot(void);

    void GetTextItemLength(int rowIndex, int *size_p) {m_rowCache_p->GetTextItemLength(rowIndex, size_p);}
    void InitializeFilterItem_LUT(void);

    /****/
    void SetColumnClip(int start, int end) {
        g_cfg_p->m_Log_colClip_Start = start;
        g_cfg_p->m_Log_colClip_End = end;
    }

    /****/
    inline bool isRowClipped(const int row)
    {
        if ((g_cfg_p->m_Log_rowClip_Start == CFG_CLIP_NOT_SET) && (g_cfg_p->m_Log_rowClip_End == CFG_CLIP_NOT_SET)) {
            return false;
        }

        if ((g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) && (row <= g_cfg_p->m_Log_rowClip_Start)) {
            return true;
        }

        if ((g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET) && (row >= g_cfg_p->m_Log_rowClip_End)) {
            return true;
        }

        return false;
    }

    ReloadStrategy_e decideReloadStrategy(const QFileInfo& newFileInfo);

    bool loadLogIncrement(void);
    void enableLogFileTracking(bool enable);

public slots:
    void logFileUpdated(const QString &path, bool useSavedReloadStrategy = false, bool forceUIUpdate = false);
    void logFileDirUpdated(const QString &path);
    void onFileChangeTimer(void);

private:
    void StartFiltering(void);

public:
    /* Priority of the created threads, possible to decrease, increase priority for speed, -1 less, +1 more */
    int m_priority;
    int m_numOfFilters;
    bool m_inDestructor;
    DB_t m_database;
    QList<CFilterItem *> m_allEnabledFilterItems;

    /* Used to keep track when full filtering is required, that filter has changed since last filtering */
    QByteArray m_filtersHash;
    CFilterItem m_bookmarkFilterItem;
    double m_LoadTime;
    double m_FilterLoadTime;
    FilterExecTimes_t m_filterExecTimes;
    QString m_textBuffer;
    char m_delayedErrorMsg[CFG_TEMP_STRING_MAX_SIZE];
    CFontCtrl m_fontCtrl;
    int m_topLine;
    QFile m_qFile_Log; /*HANDLE                    m_file_h;   //Loaded text file */
    QFile m_qFile_TIA; /* m_TIA_MemMapped_File_h; */
    QFile m_qFile_FIRA; /*  HANDLE                    m_FIRA_MemMapped_File_h; */
    QString m_Log_FileName;
    QString m_TIA_FileName;
    QString m_FIRA_FileName;
    QString m_workspaceFileName;
    QString m_workspaceFileName_revert;  /* in-case we failed to load a new workspace, we revert to the previous */
    CMemPool m_memPool;
    std::unique_ptr<CRowCache> m_rowCache_p;
    std::unique_ptr<CAutoHighLight> m_autoHighLighter_p;
    std::unique_ptr<CFontModification> m_fontModifier_p;

    /* Called pending because a ProgressDlg based search is about to start. The parameters are of temporary nature,
     * and used when the ProgressDlg calls to execute the search. The these parameters are brought to light
     *  and used to DO the search. */
    QString m_pendingSearch_searchText;
    int m_pendingSearch_startRow;
    int m_pendingSearch_endRow;
    int *m_pendingSearch_row_p;
    bool m_pendingSearch_backward;
    bool m_pendingSearch_OnlyFiltered;
    bool m_pendingSearch_regExp;
    bool m_pendingSearch_caseSensitive;

    /* Execute_LoadLog parameters */
    QString m_pendingLoadLog_fileName;
    int m_pendingLoadLog_rows;
    bool m_pendingLoadLog_result;
    CFileCtrl *m_pendingFileCtrl_p;
    QList<CPlot *> *m_pendingPlot_execList_p;
    int m_pendingPlot_startRow;
    int m_pendingPlot_endRow;
    CRecentFile m_recentFiles;
    CWorkMem m_workMem;
    QFileSystemWatcher m_fileSysWatcher;
    QDateTime m_logFileLastChanged; /* Used as extra check if file has been modified */
    QFileInfo m_logFileInfo;
    std::unique_ptr<QTimer> m_fileChangeTimer;
    bool m_logFileTrackingEnabled = false;
    ReloadStrategy_e m_savedReloadStrategy = RS_Skip;
};

extern CLogScrutinizerDoc *GetTheDoc(void);
