/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <memory>
#include "CLogScrutinizerDoc.h"
#include "CProgressDlg.h"
#include "CProgressCtrl.h"
#include "cplotctrl.h"

#include "CFileCtrl.h"
#include "CSearchCtrl.h"
#include "CFilter.h"

#include "CDebug.h"
#include "CFilterProcCtrl.h"

#include "plugin_text_parser.h"
#include "CMemPool.h"
#include "CRowCache.h"
#include "filemapping.h"
#include "CWorkspace_cb_if.h"

#include "../qt/mainwindow_cb_if.h"
#include "../qt/ceditorwidget.h"
#include "../qt/ceditorwidget_cb_if.h"
#include "../qt/cplotpane.h"
#include "../qt/cplotpane_cb_if.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QProcessEnvironment>

const int NUM_ROWS_TO_CHECK = 10;
const int NUM_TAIL_ROWS_TO_CHECK = 10;
const int NUM_TAIL_ROWS_TO_RELOAD = 1;
static CLogScrutinizerDoc *theDoc_p = nullptr;
CLogScrutinizerDoc *GetTheDoc(void) {return theDoc_p;}
CLogScrutinizerDoc *GetDocument(void) {return theDoc_p;}

const CMemPool_Config_t doc_screenCacheMemPoolConfig =
{
    5,                                      /* numOfRanges */
    {CACHE_MEM_MAP_SIZE, 8, 4, 1, 1, 0},      /* startNumPerRange */
    {CACHE_CMEM_POOL_SIZE_SMALLEST, CACHE_CMEM_POOL_SIZE_1, CACHE_CMEM_POOL_SIZE_2, CACHE_CMEM_POOL_SIZE_3,
     CACHE_CMEM_POOL_SIZE_MAX, 0}   /* ranges */
};

/* this string is used as temporary storage for strings being decoded */
static char g_largeTempString[CACHE_CMEM_POOL_SIZE_MAX];

/***********************************************************************************************************************
*   CLogScrutinizerDoc_CleanAutoHighlight
***********************************************************************************************************************/
void CLogScrutinizerDoc_CleanAutoHighlight(TIA_Cache_MemMap_t *cacheRow_p)
{
    CLogScrutinizerDoc *doc_p = GetTheDoc();
    doc_p->m_autoHighLighter_p->CleanAutoHighlight(cacheRow_p   /*m_cache_memMap[cacheIndex]*/);
}

/***********************************************************************************************************************
*   CLogScrutinizerDoc - CTOR
***********************************************************************************************************************/
CLogScrutinizerDoc::CLogScrutinizerDoc()
    : m_memPool(&doc_screenCacheMemPoolConfig)
{
    m_workspaceFileName = "";

    m_filterExecTimes.packTime = 0.0;
    m_filterExecTimes.totalFilterTime = 0.0;

    m_numOfFilters = 0;

    m_workspaceFileName_revert = "";
    m_workspaceFileName = "";

    m_inDestructor = false;

    memset(&m_database, 0, sizeof(DB_t));

    m_rowCache_p = std::make_unique<CRowCache>(m_memPool);
    m_fontModifier_p = std::make_unique<CFontModification>(m_rowCache_p.get());
    m_autoHighLighter_p = std::make_unique<CAutoHighLight>(m_rowCache_p.get());

    /* Initiate the filterItem used specifically for bookmarked rows
     * 1E90FF   dodger blue     dodger blue */
    m_bookmarkFilterItem.m_font_p = m_fontCtrl.RegisterFont(Q_RGB(0xFF, 0xFF, 0xFF), Q_RGB(0x1E, 0x90, 0xFF));
    m_bookmarkFilterItem.m_enabled = true;

    InitializeFilterItem_LUT();

    connect(&m_fileSysWatcher, SIGNAL(fileChanged(QString)), this, SLOT(logFileUpdated(QString)));
    connect(&m_fileSysWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(logFileDirUpdated(QString)));

    m_fileChangeTimer = std::make_unique<QTimer>(this);

    connect(m_fileChangeTimer.get(), SIGNAL(timeout()), this, SLOT(onFileChangeTimer()));

    TRACEX_I("Cache pool size: %lldMB", (m_memPool.GetTotalSize() >> 10))

    theDoc_p = this;
}

/***********************************************************************************************************************
*   decideReloadStrategy
***********************************************************************************************************************/
ReloadStrategy_e CLogScrutinizerDoc::decideReloadStrategy(const QFileInfo& newFileInfo)
{
    if (m_logFileInfo.size() > newFileInfo.size()) {
        return RS_Full;
    }
    if ((m_logFileLastChanged.time() == newFileInfo.lastModified().time()) &&
        (m_database.fileSize == newFileInfo.size())) {
        return RS_Skip;
    }
    if (m_database.TIA.rows < NUM_TAIL_ROWS_TO_RELOAD) {
        return RS_Incremental; /* Reload entire file */
    }

    /* Close and Reopen to prevent file caching to "destroy" the file change validation. */
    m_qFile_Log.close();
    if (!m_qFile_Log.open(QIODevice::ReadOnly)) {
        TRACEX_W(QString("Failied to reopen log file at file validation"))
        return RS_Full;
    }

    char text_p[1];
    TI_t *ti_p = &m_database.TIA.textItemArray_p[0];

    /* Check that some of the existing rows hasn't been changed (else RS_Full)
     * Note: NUM_TAIL_ROWS_TO_RELOAD number of rows will anyway be reloaded, hence these do not need to
     * be unchanged for managing an incremental */
    const auto lastIndex = m_database.TIA.rows - NUM_TAIL_ROWS_TO_CHECK;

    /* (1) Check tail */
    auto index = lastIndex - NUM_TAIL_ROWS_TO_CHECK - NUM_TAIL_ROWS_TO_RELOAD;
    index = index < 0 ? 0 : index;
    for ( ; index < lastIndex; index++) {
        const auto eol_size = ti_p[index].size;
        if (!m_rowCache_p->rawFromFile(ti_p[index].fileIndex + eol_size,
                                       1, /* one byte for End Of Line check */
                                       text_p)) {
            return RS_Full;
        }

        /* Check the eol marker */
        if ((*text_p != 0x0a) && (*text_p != 0x0d)) {
            return RS_Full;
        }
    }

    /* (2) Check some rows spread across the file
     * Pick X spread points in log file and verify TIA against line endings */
    auto rowDeltaSpread = m_database.TIA.rows / NUM_ROWS_TO_CHECK;
    for (auto index = 0; index < NUM_ROWS_TO_CHECK; index++) {
        const auto ti_index = index * rowDeltaSpread;
        const auto eol_size = ti_p[ti_index].size;
        if (!m_rowCache_p->rawFromFile(ti_p[ti_index].fileIndex + eol_size,
                                       1, /* one byte for End Of Line check */
                                       text_p)) {
            return RS_Full;
        }

        /* Check the eol marker */
        if ((*text_p != 0x0a) && (*text_p != 0x0d)) {
            return RS_Full;
        }
    }

    return RS_Incremental;
}

/***********************************************************************************************************************
*   logFileUpdated
***********************************************************************************************************************/
void CLogScrutinizerDoc::logFileUpdated(const QString &path, bool useSavedReloadStrategy, bool forceUIUpdate)
{
    /* TODO:
     * While LogScrutinizer is doing processing, or just accessing the m_database, it is not safe to do any updates
     * as this could cause race-conditions to the memory. I guess that we will be running in the main thread
     * when this function is called... perhaps that needs to be checked.. If that is the case then the UI should
     * safe, as this function should be called when the UI is not processing anything else.
     *
     * (1) If user tries to move around in file whie tracking is on, the tracking should be disabled automatically
     * (2) Make tool that produces a growing file and test... */
    if (CSZ_DB_PendingUpdate) {
        return;
    }

    bool doFull = false;

    if (path == m_Log_FileName) {
        PRINT_FILE_TRACKING(QString(">> Log file modified: %1").arg(path))

        QFileInfo fileInfo(m_Log_FileName);
        fileInfo.refresh(); /* fetch new information about the file */

        auto reloadStrategy = decideReloadStrategy(fileInfo);

        /* Always reload if FULL required, as it might be that the log has been shortned, and if LogScrutinizer
         * is trying to read from the log after missing lines... things go south... */
        if (((RS_Full != reloadStrategy) && !g_cfg_p->m_logFileTracking) || CSZ_DB_PendingUpdate) {
            if (static_cast<int>(m_savedReloadStrategy) < static_cast<int>(reloadStrategy)) {
                m_savedReloadStrategy = reloadStrategy;
            }
            return;
        }

        if (useSavedReloadStrategy) {
            reloadStrategy = m_savedReloadStrategy;
            m_savedReloadStrategy = RS_Skip;
        }

        switch (reloadStrategy)
        {
            case RS_Skip:
                PRINT_FILE_TRACKING(QString("RS_Skip"))
                if (forceUIUpdate) {
                    MW_Refresh();
                }
                return;

            case RS_Incremental:
                if (loadLogIncrement()) {
                    PRINT_FILE_TRACKING(QString("RS_Incremental"))
                } else {
                    doFull = true;
                }
                break;

            case RS_Full: /* fall-through */
                doFull = true;
                break;
        }

        if (doFull) {
            PRINT_FILE_TRACKING(QString("RS_Full"))
            LoadLogFile(m_Log_FileName /*Call with a copy*/, true /*reload*/);

            /* do not filter... since a new file might not contain any filter matches */
        }

        if (RS_Full != reloadStrategy) {
            m_logFileLastChanged = fileInfo.lastModified();
            m_logFileInfo = fileInfo;
        }

        MW_Refresh(true);
    }
}

/***********************************************************************************************************************
*   loadLogIncrement
***********************************************************************************************************************/
bool CLogScrutinizerDoc::loadLogIncrement(void)
{
    auto pendingUpdateScopeGuard = makeMyScopeGuard([&] () {CSZ_DB_PendingUpdate = false;});
    CSZ_DB_PendingUpdate = true;

    if (m_workMem.Operation(WORK_MEM_OPERATION_COMMIT)) {
        auto freeScopeGuard = makeMyScopeGuard([&] () {(void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);});
        CFileCtrl fileCtrl;
        int rows_added;

        /* Start from the last row in the database, as this line might have been added to. */
        auto rowsToReload = NUM_TAIL_ROWS_TO_RELOAD;
        int32_t fromRowIndex = m_database.TIA.rows - rowsToReload;
        int64_t fromFileIndex = m_database.TIA.textItemArray_p[fromRowIndex].fileIndex;
        int64_t fileSize = 0;
        const auto oldNumRows = m_database.TIA.rows;

        /* TODO
         *  Trigger a filtering
         *  Trigger a plotRun of the last rows. ??? Or should plotting require manual trigger?
         */
        if (m_qFile_TIA.isOpen()) {
            if (m_database.TIA.textItemArray_p != nullptr) {
                FileMapping::RemoveTIA_MemMapped(m_qFile_TIA, m_database.TIA.textItemArray_p, false);
            } else {
                m_qFile_TIA.close();
            }
        }

        if (m_qFile_FIRA.isOpen()) {
            if (m_database.FIRA.FIR_Array_p != nullptr) {
                FileMapping::RemoveFIRA_MemMapped(m_qFile_FIRA, m_database.FIRA.FIR_Array_p, false /*remove*/);
            } else {
                m_qFile_FIRA.close();
            }
        }

        /* Incrementally add to the TIA file */
        fileCtrl.Search_TIA_Incremental(m_qFile_Log, m_TIA_FileName, m_workMem.GetRef(), m_workMem.GetSize(),
                                        fromFileIndex, fromRowIndex, &rows_added);

        m_database.TIA.rows = 0;

        /* Remap the TIA file */
        if (FileMapping::CreateTIA_MemMapped(m_qFile_Log, m_qFile_TIA, &m_database.TIA.rows,
                                             m_database.TIA.textItemArray_p, &fileSize)) {
            if (FileMapping::IncrementalFIRA_MemMap(m_qFile_FIRA, m_database.FIRA.FIR_Array_p, m_database.TIA.rows)) {
                /* uses m_FIRA_fileName */
                if (oldNumRows > m_database.TIA.rows) {
                    return false;
                }

                m_rowCache_p->Update(&m_qFile_Log, &m_database.TIA, &m_database.FIRA, m_database.filterItem_LUT,
                                     m_memPool);
                UpdateTitleRow();

                m_database.fileSize = fileSize;

                QFileInfo fileInfo(m_Log_FileName);
                fileInfo.refresh();

                if (CEditorWidget_isPresentationModeFiltered()) {
                    /* Start from the previous last row */
                    CSZ_DB_PendingUpdate = false;
                    ExecuteIncrementalFiltering(oldNumRows > 0 ? oldNumRows - 1 : 0);
                }

                PRINT_FILE_TRACKING(QString("Incremental load, rows:%1 fileSize:%2(%3)")
                                        .arg(rows_added)
                                        .arg(m_database.fileSize)
                                        .arg(fileInfo.size()))

                return true;
            }
        }
    }
    return false;
}

/***********************************************************************************************************************
*   logFileDirUpdated
***********************************************************************************************************************/
void CLogScrutinizerDoc::logFileDirUpdated(const QString &path)
{
    /* This function is called when Qt detects changes in the directory where the logFile is stored
     * If e.g. gEdit is saving the file then it first remove the file and then put it back in the same name.
     * For Qt QFileSystemWatcher this means that its a new file, hence we will need to replace the current watcher
     * with the log file name again.
     * Check if the file modified in the Dir update is our log file, if it was, check if it still exists... if it
     * doesn't
     * exist popup a dialog with information and ask if the file should be reloaded... or closed.
     * If the file already exist when checked then it is not necessary to ask the user...
     */
    if (CSZ_DB_PendingUpdate) {
        return;
    }

    QStringList list = m_fileSysWatcher.files();
    list.append(m_fileSysWatcher.directories());

    bool monitoredFileChanged = false;

    /* Check if the modified file is the log file */
    for (auto& fileName : list) {
        if (fileName == path) {
            QFileInfo fileInfo(m_Log_FileName);
            fileInfo.refresh();

            if (fileInfo.exists() &&
                (fileInfo.lastModified().time() ==
                 m_logFileLastChanged.time())) {
                return;
            }

            if (fileInfo.lastModified().time() != m_logFileLastChanged.time()) {
                logFileUpdated(m_Log_FileName);
            }

            bool closeLogFile = false;
            monitoredFileChanged = true;

            PRINT_FILE_TRACKING(QString("Log file (dir) modified: %1").arg(path))

            if (!fileInfo.exists()) {
                TRACEX_W(QString("Log file removed: %1").arg(path))

                auto ret = QMessageBox::information(
                    MW_Parent(),
                    QObject::tr("Log file was modified/removed outside LogScrutinizer"),
                    QObject::tr("The log file was removed by an external program, some editors,"
                                "when saving a file, will first\nremove the file and then "
                                "re-create it.\nShould LogScrutinizer try to reload it?"),
                    QMessageBox::Ok, QMessageBox::Close);
                closeLogFile = (ret == QMessageBox::Close) ? true : false;
            }

            if (closeLogFile || !fileInfo.exists()) {
                TRACEX_W(QString("Failed to reload removed log file, closing: %1").arg(path))
                CleanDB(false);
            } else {
                /* Replace the log file in the file system watcher */
                replaceFileSysWatcherFile(m_Log_FileName);
            }
        }
    }

    if (!monitoredFileChanged) {
        PRINT_FILE_TRACKING(QString("File modified, not log file: %1").arg(path))
    }
}

/***********************************************************************************************************************
*   onFileChangeTimer
***********************************************************************************************************************/
void CLogScrutinizerDoc::onFileChangeTimer(void)
{
    /* On Win10 file changes are not detected, not even if the program writing to the file do a flush
     * https://bugreports.qt.io/browse/QTBUG-41119
     * This timer is a fallback to check if the file has been updated */

    if (CSZ_DB_PendingUpdate) {
        return;
    }

    QFileInfo fileInfo(m_Log_FileName);
    fileInfo.refresh();

    PRINT_FILE_TRACKING(QString("File:%1 Size: %2").arg(fileInfo.fileName()).arg(fileInfo.size()))

    if (!fileInfo.exists() ||
        (m_logFileLastChanged.time() != fileInfo.lastModified().time()) ||
        (fileInfo.size() != m_database.fileSize)) {
        PRINT_FILE_TRACKING(QString("onFileChangeTimer %1 %2 size:%3 %4")
                                .arg(m_logFileLastChanged.time().toString())
                                .arg(fileInfo.lastModified().time().toString())
                                .arg(fileInfo.size()).arg(m_database.fileSize))
        logFileUpdated(m_Log_FileName);
    }
}

/***********************************************************************************************************************
*   enableLogFileTracking
***********************************************************************************************************************/
void CLogScrutinizerDoc::enableLogFileTracking(bool enable)
{
    if (m_logFileTrackingEnabled != enable) {
        PRINT_FILE_TRACKING(QString("%1 Enable:%2").arg(__FUNCTION__).arg(enable))

        m_logFileTrackingEnabled = enable;
        if (enable) {
            /* Make this call to have an initial check if it is required to load the file or not. */
            logFileUpdated(m_Log_FileName, true /* Apply save reload strategy */, true /*force UE update */);
            m_fileChangeTimer.get()->start(FILE_CHANGE_TIMER_DURATION);
        } else {
            m_savedReloadStrategy = RS_Skip;
            m_fileChangeTimer.get()->stop();
        }
    }
}

/***********************************************************************************************************************
*   isRowClipped
***********************************************************************************************************************/
bool CLogScrutinizerDoc::isRowClipped(const int row)
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

/***********************************************************************************************************************
*   CleanDB
***********************************************************************************************************************/
void CLogScrutinizerDoc::CleanDB(bool reload, bool unloadFilters)
{
    TRACEX_I("Cleaning all")

    /* First thing */
    m_database.TIA.rows = 0;
    m_database.FIRA.filterMatches = 0;

    m_fileChangeTimer.get()->stop();
    m_logFileInfo = QFileInfo();
    m_logFileLastChanged = QDateTime();

    /* Remove the files being watched */
    if (!m_fileSysWatcher.files().isEmpty()) {
        m_fileSysWatcher.removePaths(m_fileSysWatcher.files());
    }
    if (!m_fileSysWatcher.directories().isEmpty()) {
        m_fileSysWatcher.removePaths(m_fileSysWatcher.directories());
    }

    (void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);

    if (m_database.TIA.textItemArray_p != nullptr) {
        FileMapping::RemoveTIA_MemMapped(m_qFile_TIA, m_database.TIA.textItemArray_p, false); /* Will close m_qFile_TIA
                                                                                               * */
        m_database.TIA.textItemArray_p = nullptr;
    }

    if (m_database.FIRA.FIR_Array_p != nullptr) {
        FileMapping::RemoveFIRA_MemMapped(m_qFile_FIRA, m_database.FIRA.FIR_Array_p);         /* Will close m_qFile_FIRA
                                                                                               * */
        m_database.FIRA.FIR_Array_p = nullptr;
    }

    memset(&m_database, 0, sizeof(DB_t)); /*  This cannot be done when filters are not reloaded ...why ?? */

    if (m_qFile_Log.isOpen()) {
        TRACEX_I("Closed Log file: %s", m_qFile_Log.fileName().toLatin1().constData())
        m_qFile_Log.close();
    }

    if (m_inDestructor) {
        return;
    }

    m_Log_FileName = "";
    m_TIA_FileName = "";
    m_FIRA_FileName = "";

    if (m_database.packedFIRA_p != nullptr) {
        VirtualMem::Free(m_database.packedFIRA_p);
        m_database.packedFIRA_p = nullptr;
    }

    CWorkspace_CleanAllPlots();

    CPlotPane *plotPane_p = CPlotPane_GetPlotPane();
    if (plotPane_p != nullptr) {
        plotPane_p->cleanAllPlots();
    }

    if (!reload) {
        CWorkspace_RemoveAllBookmarks();
    }

    if (unloadFilters) {
        UnloadFilters();
    }

    if (m_rowCache_p != nullptr) {
        m_rowCache_p->Reset();
    }

    UpdateTitleRow();

    if (m_autoHighLighter_p != nullptr) {
        m_autoHighLighter_p->Clean();
    }

    CEditorWidget_Initialize(false);

    if (!reload) {
        MW_Refresh();
    }
}

/***********************************************************************************************************************
*   UpdateTitleRow
***********************************************************************************************************************/
void CLogScrutinizerDoc::UpdateTitleRow(void)
{
    QString tempString;
    QString programTitle, version, buildDate, configuration;

    g_cfg_p->GetAppInfo(programTitle, version, buildDate, configuration);

    if (m_database.TIA.rows != 0) {
        QFileInfo fileInfo(m_Log_FileName);

        tempString.append(QString("%1  %2").arg(fileInfo.fileName()).arg(FileSizeToString(m_database.fileSize)));
        if ((g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) || (g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET)) {
            int64_t clippedStart;
            int64_t clippedEnd;
            int64_t clippedSize;

            clippedStart = 0;
            clippedEnd = m_database.fileSize;

            if (g_cfg_p->m_Log_rowClip_Start > CFG_CLIP_NOT_SET) {
                clippedStart = m_database.TIA.textItemArray_p[g_cfg_p->m_Log_rowClip_Start].fileIndex;
            }

            if (g_cfg_p->m_Log_rowClip_End > CFG_CLIP_NOT_SET) {
                clippedEnd = m_database.TIA.textItemArray_p[g_cfg_p->m_Log_rowClip_End].fileIndex;
            }
            clippedSize = clippedEnd - clippedStart;
            tempString.append(QString("  <Clipped_%1>").arg(FileSizeToString(clippedSize)));
        }
        tempString.append(QString(" - %1 %2 %3").arg(programTitle).arg(version).arg(buildDate));
    } else {
        tempString.append(QString("%1 %2 %3").arg(programTitle).arg(version).arg(buildDate));
    }
    MW_SetApplicationName(&tempString);
}

/***********************************************************************************************************************
*   FileSizeToString
***********************************************************************************************************************/
QString CLogScrutinizerDoc::FileSizeToString(int64_t size)
{
    QString fileSizeStr;
    if (size < 1024) {
        fileSizeStr = QString("Size:%1 bytes").arg(size);
    } else if (size < (1024 * 1024)) {
        fileSizeStr = QString("Size:%1kB").arg(QString::number(static_cast<double>(size) / 1024.0, 'f', 1));
    } else if (size < (1024 * 1024 * 1024)) {
        fileSizeStr = QString("Size:%1MB").arg(QString::number(static_cast<double>(size) / (1024.0 * 1024.0),
                                                               'f', 1));
    } else {
        fileSizeStr = QString("Size:%1GB").arg(QString::number(
                                                   static_cast<double>(size) / (1024.0 * 1024.0 * 1024.0), 'f', 1));
    }
    return fileSizeStr;
}

/***********************************************************************************************************************
*   timeToString
***********************************************************************************************************************/
QString CLogScrutinizerDoc::timeToString(int64_t ms)
{
    QString timeStr;
    if (ms > 1000) {
        timeStr = QString("Time: %1sec").arg(QString::number(static_cast<double>(ms) / 1000.0, 'f', 1));
    } else if (ms > 0) {
        timeStr = QString("Time: %1ms").arg(ms);
    } else {
        timeStr = QString("Time: < 1ms");
    }
    return timeStr;
}

/***********************************************************************************************************************
*   UnloadFilters
***********************************************************************************************************************/
void CLogScrutinizerDoc::UnloadFilters(void)
{
    while (!m_allEnabledFilterItems.isEmpty()) {
        delete (m_allEnabledFilterItems.takeFirst());
    }
    InitializeFilterItem_LUT();
}

/***********************************************************************************************************************
*   UnloadFilters
* Function is used to make a "real-time" update of the colors used for a filter. This to speed up the process of
* finding a good matching color combination of all the filters
***********************************************************************************************************************/
void CLogScrutinizerDoc::UpdateFilterItem(int uniqueID, Q_COLORREF color, Q_COLORREF bg_color)
{
    for (auto& filterItem_p : m_allEnabledFilterItems) {
        if (filterItem_p->m_uniqueID == uniqueID) {
            filterItem_p->m_color = color;
            filterItem_p->m_bg_color = bg_color;
            filterItem_p->m_font_p = m_fontCtrl.RegisterFont(color, bg_color);
            CEditorWidget_Repaint();
            return;
        }
    }
}

/***********************************************************************************************************************
*   InitializeFilterItem_LUT
***********************************************************************************************************************/
void CLogScrutinizerDoc::InitializeFilterItem_LUT(void)
{
    FilterMgr::InitializeFilterItem_LUT(m_database.filterItem_LUT, &m_bookmarkFilterItem);
}

/***********************************************************************************************************************
*   PluginIsUnloaded
***********************************************************************************************************************/
void CLogScrutinizerDoc::PluginIsUnloaded(void)
{
    CEditorWidget_EmptySelectionList();
    CleanRowCache();
}

/***********************************************************************************************************************
*   CreateFiltersFromContainer
***********************************************************************************************************************/
void CLogScrutinizerDoc::CreateFiltersFromContainer(CFilterContainer& container)
{
    QList<CFilter *> list = container.GetFiltersList();

    InitializeFilterItem_LUT();

    int index = 0;
    m_database.filterItem_LUT[index++] = nullptr; /*No match filter item */

    QList<CFilter *>::iterator iter;
    for (auto& filter : list) {
        for (auto& filterItem_p : filter->m_filterItemList) {
            if (filterItem_p->m_enabled) {
                /* The LUT table is not part of the model, no need for model update */
                m_allEnabledFilterItems.append(new CFilterItem(filterItem_p));
                m_database.filterItem_LUT[index++] = m_allEnabledFilterItems.last();
            }
        }
    }
}

/***********************************************************************************************************************
*   CreateFiltersFromCCfgItems
***********************************************************************************************************************/
void CLogScrutinizerDoc::CreateFiltersFromCCfgItems(void)
{
    /* To perform the filtering we must first copy the filters from the CWorkspace into the
     * document, a duplicate list. */

    InitializeFilterItem_LUT();

    /* Get the filter list from the workspace */
    const QList<CCfgItem *> *list = CWorkspace_GetFilterList();

    if ((list != nullptr) && !list->isEmpty()) {
        /* First index in LUT shall be nullptr, used for indicating no match */
        m_database.filterItem_LUT[0] = nullptr;

        for (auto filter_p : *list) {
            CCfgItem_Filter *cfgItemFilter_p = static_cast<CCfgItem_Filter *>(filter_p);

            /* Loop over the CCfgItem_Filter children and store copies of filterItems into m_allEnabledFilterItems. */
            for (auto& filterItem_p : cfgItemFilter_p->m_cfgChildItems) {
                CCfgItem_FilterItem *cfgFilterItem_p = static_cast<CCfgItem_FilterItem *>(filterItem_p);
                if (cfgFilterItem_p->m_filterItem_ref_p->m_enabled) {
                    /* Adding filter items to allEnabledFilterItems, note that first filter will end up LUT index 1 */
                    m_allEnabledFilterItems.append(new CFilterItem(cfgFilterItem_p->m_filterItem_ref_p));
                    m_database.filterItem_LUT[m_allEnabledFilterItems.count()] = m_allEnabledFilterItems.last();
                }
            }
        }
    }
}

/***********************************************************************************************************************
*   CheckAllFilters
***********************************************************************************************************************/
bool CLogScrutinizerDoc::CheckAllFilters(void)
{
    QList<CCfgItem *> *list_p = CWorkspace_GetFilterList();

    if ((list_p != nullptr) && !list_p->isEmpty()) {
        for (auto& cfgItem_p : *list_p) {
            auto *cfgItemFilter_p = reinterpret_cast<CCfgItem_Filter *>(cfgItem_p);

            /* Copy all filterItem references */
            for (auto& cfgItem_fi : cfgItemFilter_p->m_cfgChildItems) {
                auto *filterItem_p = reinterpret_cast<CCfgItem_FilterItem *>(cfgItem_fi);
                QString errorString;

                if (filterItem_p->m_filterItem_ref_p->m_enabled) {
                    int errorCode = filterItem_p->m_filterItem_ref_p->Check(errorString);
                    if (errorCode != 1) {
                        QString message = "The filter ";
                        memcpy(g_largeTempString,
                               filterItem_p->m_filterItem_ref_p->m_start_p,
                               static_cast<size_t>(filterItem_p->m_filterItem_ref_p->m_size));
                        g_largeTempString[filterItem_p->m_filterItem_ref_p->m_size] = 0;
                        message += QString(g_largeTempString) + QString(" is corrupt - ") + errorString;
                        TRACEX_W(message)
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

/***********************************************************************************************************************
*   replaceFileSysWatcherFile
***********************************************************************************************************************/
void CLogScrutinizerDoc::replaceFileSysWatcherFile(QString& fileName)
{
    /* First remove the current watches */
    if (!m_fileSysWatcher.files().isEmpty()) {
        m_fileSysWatcher.removePaths(m_fileSysWatcher.files());
    }
    if (!m_fileSysWatcher.directories().isEmpty()) {
        m_fileSysWatcher.removePaths(m_fileSysWatcher.directories());
    }

    QFileInfo fileInfo(fileName);
    fileInfo.refresh();

    if (!fileInfo.exists()) {
        TRACEX_W(QString("File doesn't exist, cannot be tracked: %1").arg(fileName))
        return;
    }

    TRACEX_I(QString("Last changed updated: %1").arg(fileName))

    /* Add the new file watch */
    if (!m_fileSysWatcher.addPath(fileInfo.absoluteFilePath())) {
        TRACEX_W(QString("Failed to aa file tracking: %1").arg(fileInfo.absoluteFilePath()))
    }

    m_logFileInfo = fileInfo;
    m_logFileLastChanged = fileInfo.lastModified();

    /* Add the new directory watch */
    if (!m_fileSysWatcher.addPath(fileInfo.absolutePath())) {
        TRACEX_W(QString("Failed to add directory tracking: %1").arg(fileInfo.absolutePath()))
    }

    /* Print all the adds */
    QStringList files = m_fileSysWatcher.files();
    if (!files.empty()) {
        for (auto& file : files) {
            TRACEX_I(QString("Tracked file: %1").arg(file))
        }
    }

    QStringList dirs = m_fileSysWatcher.directories();
    if (!dirs.empty()) {
        for (auto& dir : dirs) {
            TRACEX_I(QString("Tracked dir: %1").arg(dir))
        }
    }
}

/***********************************************************************************************************************
*    logUpdated
***********************************************************************************************************************/
void CLogScrutinizerDoc::logUpdated(int64_t fileSize)
{
    TRACEX_I(QString("Loaded log: %1 Rows:%2").arg(m_Log_FileName).arg(m_database.TIA.rows))

    m_database.FIRA.filterExcludeMatches = 0;
    m_database.FIRA.filterMatches = 0;
    m_database.fileSize = fileSize;

    UpdateTitleRow();
    CSZ_DB_PendingUpdate = false;
    VerifyDB();
    MW_Refresh();

    if (m_logFileTrackingEnabled) {
        m_fileChangeTimer.get()->start(FILE_CHANGE_TIMER_DURATION);
    } else {
        m_fileChangeTimer.get()->stop();
    }

    /*the rowCache cannot work on filterItem lut, since that will be updated after each filter... */
    m_rowCache_p->Update(&m_qFile_Log, &m_database.TIA, &m_database.FIRA, m_database.filterItem_LUT, m_memPool);

    replaceFileSysWatcherFile(m_Log_FileName);
}

/***********************************************************************************************************************
*   LoadLogFile
***********************************************************************************************************************/
bool CLogScrutinizerDoc::LoadLogFile(QString fileName /*use copy*/, bool reload)
{
    CTimeMeas execTime;
    int64_t fileSize;
    auto pendingUpdateScopeGuard = makeMyScopeGuard([&] () {CSZ_DB_PendingUpdate = false;});
    CSZ_DB_PendingUpdate = true;

    CleanDB(reload, false);  /*Do not unload filters */

    m_delayedErrorMsg[0] = 0;

    TRACEX_I(QString("Loading log file: %1").arg(fileName))

    CEditorWidget_Initialize(reload);

    m_database.fileSize = 0;

    if (m_qFile_Log.isOpen()) {
#ifdef _DEBUG
        TRACEX_E("Logical error, log file is already open")
#else
        TRACEX_W("Logical error, log file is already open")
#endif
        return false;
    }

    m_Log_FileName = fileName;

    /* Open as read-only with shared reading
     * If the log file name is relative, then replace it with the absolute */
    GetAbsoluteFileName(m_Log_FileName, m_Log_FileName);

    m_qFile_Log.setFileName(m_Log_FileName);

    if (!m_qFile_Log.open(QIODevice::ReadOnly)) {
        TRACEX_QFILE(LOG_LEVEL_WARNING, "Failed to open the log file", &m_qFile_Log)
        QMessageBox::information(MW_Parent(), QObject::tr("File open failed"),
                                 QObject::tr("The file couldn't be opened, please close the "
                                             "program using it and re-try"), QMessageBox::Ok);
        m_Log_FileName = QString();
        m_qFile_Log.setFileName(m_Log_FileName);
        MW_Refresh();
        return false;
    }

    m_TIA_FileName = m_Log_FileName + QString(".tia");
    m_FIRA_FileName = m_Log_FileName + QString(".fira");

    /* Check if we may open, if not possible try with alternative names. If this fails entirely then this log
     * cannot be opened at all since we cannot create a corresponding TIA file
     * check tia file, open *_x.tia if necessary (x= 1-10) */
    if (!CanFileBeOpenedForFileReadWrite(m_TIA_FileName, CFG_MAX_FILE_NAME_SIZE, true, 0, true)) {
        CleanDB(false);  /* fallback solution */
#ifdef _DEBUG
        TRACEX_E(m_delayedErrorMsg)
#else
        TRACEX_W(m_delayedErrorMsg)
#endif
        return false;
    }

    /* Check if we may open, if not possible try with alternative names. If this fails entirely then this log
     * cannot be opened at all since we cannot create a corresponding FIRA file
     * check fira file, open *_x.fira if necessary (x= 1-10 */
    if (!CanFileBeOpenedForFileReadWrite(m_FIRA_FileName, CFG_MAX_FILE_NAME_SIZE, true, 0, true)) {
        CleanDB(false);  /* fallback solution */
#ifdef _DEBUG
        TRACEX_E(m_delayedErrorMsg)
#else
        TRACEX_W(m_delayedErrorMsg)
#endif
        return false;
    }

    m_recentFiles.AddFile(m_Log_FileName);

    m_qFile_TIA.setFileName(m_TIA_FileName);
    m_qFile_FIRA.setFileName(m_FIRA_FileName);

    /* Try QUICK LOADING of existing TIA and FIRA files, if they exists... and are valid */
    if (FileMapping::CreateTIA_MemMapped(m_qFile_Log, m_qFile_TIA, &m_database.TIA.rows,
                                         m_database.TIA.textItemArray_p, &fileSize, true /*check file size*/)) {
        if (FileMapping::CreateFIRA_MemMapped(m_qFile_FIRA, m_database.FIRA.FIR_Array_p, m_database.TIA.rows)) {
            logUpdated(fileSize);
            TRACEX_I(QString("Fast loading successful, based on TIA file:%1").arg(m_TIA_FileName))
            return true;  /* SUCCESS, halt here, we are DONE! */
        }
    }

    /* Quick loading failed, lets try at create TIA file
     * But first, just to be sure, clean up after the failed quick-load above. */

    if (m_qFile_TIA.isOpen()) {
        if (m_database.TIA.textItemArray_p != nullptr) {
            FileMapping::RemoveTIA_MemMapped(m_qFile_TIA, m_database.TIA.textItemArray_p, true);
        } else {
            m_qFile_TIA.close();
        }
    }

    if (m_qFile_FIRA.isOpen()) {
        if (m_database.FIRA.FIR_Array_p != nullptr) {
            FileMapping::RemoveFIRA_MemMapped(m_qFile_FIRA, m_database.FIRA.FIR_Array_p);
        } else {
            m_qFile_FIRA.close();
        }
    }

    if (m_workMem.Operation(WORK_MEM_OPERATION_COMMIT)) {
        CFileCtrl fileCtrl;
        m_pendingFileCtrl_p = &fileCtrl;
        m_pendingLoadLog_fileName = m_Log_FileName;

        CProgressDlg dlg("Loading log file", ProgressCmd_LoadLog_en);
        dlg.setModal(true);
        dlg.exec();

        bool result = m_pendingLoadLog_result;
        m_database.TIA.rows = m_pendingLoadLog_rows;

        (void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);

        if (!result) {
            /* If it was a failure we abort
             * If it was a USER REQUESTED abort it is not an error, although we have to unload the current log */
            if (!g_processingCtrl_p->m_abort) {
#ifdef _DEBUG
                TRACEX_E(QString("CLogScrutinizerDoc::Search_TIA  Failed FileName:%1").arg(m_Log_FileName))
#else
                TRACEX_W(QString("CLogScrutinizerDoc::Search_TIA  Failed FileName:%1").arg(m_Log_FileName))
#endif
            }

            CleanDB(false);  /* fallback solution */
            return false;
        }

        /* All went good to load the log file, now load and map the created TIA and FIRA files */
        if (FileMapping::CreateTIA_MemMapped(m_qFile_Log, m_qFile_TIA, &m_database.TIA.rows,
                                             m_database.TIA.textItemArray_p, &fileSize)) {
            if (FileMapping::CreateFIRA_MemMapped(m_qFile_FIRA, m_database.FIRA.FIR_Array_p, m_database.TIA.rows)) {
                logUpdated(fileSize);
                return true;
            }
        }
    }

    CleanDB(false);  /* fallback solution */

    if (!g_processingCtrl_p->m_abort) {
        /* Delayed error message, prints the last */
        TRACEX_E(m_delayedErrorMsg)
    }

    return false;
}

/***********************************************************************************************************************
*   ExecuteLoadLog
***********************************************************************************************************************/
void CLogScrutinizerDoc::ExecuteLoadLog(void)
{
    m_pendingLoadLog_result = m_pendingFileCtrl_p->Search_TIA(
        &m_qFile_Log,
        m_TIA_FileName.toLatin1().data(),
        m_workMem.GetRef(),   /* Todo, provide the workMem instance instead */
        m_workMem.GetSize(),
        &m_pendingLoadLog_rows);
}

/***********************************************************************************************************************
*   Filter
***********************************************************************************************************************/
void CLogScrutinizerDoc::Filter(void)
{
#ifdef ASSERT_ON_NULL
    Q_ASSERT(g_editorWidget_p != nullptr);
#endif
    if (CSZ_DB_PendingUpdate) {
        return;
    }

    if (CheckAllFilters()) {
        UnloadFilters(); /* This will empty the list of CFilters */
        CreateFiltersFromCCfgItems();
        StartFiltering();
    }
}

/***********************************************************************************************************************
*   StartFiltering
***********************************************************************************************************************/
void CLogScrutinizerDoc::StartFiltering(void)
{
    QString info;
    QByteArray hash;
    CWorkspace_GetFiltersHash(hash);

    if (hash == m_filtersHash) {
        info = QString("- No filter item changes");
    }

    TRACEX_I(QString("Start Filtering   Rows:%1 (%2) Start:%3 End:%4  %5")
                 .arg(g_cfg_p->m_Log_rowClip_End - g_cfg_p->m_Log_rowClip_Start)
                 .arg(m_database.TIA.rows).arg(g_cfg_p->m_Log_rowClip_Start).arg(g_cfg_p->m_Log_rowClip_End)
                 .arg(info))

    CSZ_DB_PendingUpdate = true;

    CProgressDlg dlg("Filtering...", ProgressCmd_Filter_en);
    dlg.setModal(true);
    dlg.exec();

    TRACEX_I("Filter results  Matches:%d Exclude Matches:%d",
             m_database.FIRA.filterMatches, m_database.FIRA.filterExcludeMatches)

    PostProcFiltering();
}

/***********************************************************************************************************************
*   PostProcFiltering
***********************************************************************************************************************/
void CLogScrutinizerDoc::PostProcFiltering(void)
{
    CreatePackedFIRA();

    if ((m_database.FIRA.filterMatches == 0) && (m_database.FIRA.filterExcludeMatches == 0)) {
#ifdef ASSERT_ON_NULL
        Q_ASSERT(g_editorWidget_p != nullptr);
#endif
        if (g_editorWidget_p != nullptr) {
            g_editorWidget_p->SetPresentationMode(PRESENTATION_MODE_ALL_e);
        }
        MW_PlaySystemSound(SYSTEM_SOUND_QUESTION);
        if (g_processingCtrl_p->m_abort) {
            QMessageBox msgBox(QMessageBox::Information, "LogScrutinizer - No filter matches",
                               "The aborted filtering has removed previous filtering results, please filter again",
                               QMessageBox::Ok,
                               MW_Parent());
            msgBox.exec();
            TRACEX_I("The aborted filtering has removed previous "
                     "filtering results, please filter again")
        } else {
            QMessageBox msgBox(QMessageBox::Information,
                               "LogScrutinizer - No filter matches",
                               "There are no filterItems matching at all, check your filterItems",
                               QMessageBox::Ok,
                               MW_Parent());
            msgBox.exec();
            TRACEX_I("There are no filterItems matching at all, check your filterItems")
        }
    }

    /* Update the hash value of all filters */
    CWorkspace_GetFiltersHash(m_filtersHash);

    CleanRowCache();
    CSZ_DB_PendingUpdate = false;
}

/***********************************************************************************************************************
*   ExecuteFiltering
***********************************************************************************************************************/
void CLogScrutinizerDoc::ExecuteFiltering(void)
{
    QList<int> bookmarkList;

    m_database.FIRA.filterMatches = 0;
    CWorkspace_GetBookmarks(&bookmarkList);

    if (m_database.TIA.rows != 0) {
        if (m_workMem.Operation(WORK_MEM_OPERATION_COMMIT)) {
            CFilterProcCtrl filterCtrl;

            filterCtrl.StartProcessing(&m_qFile_Log,
                                       m_workMem.GetRef(),
                                       m_workMem.GetSize(),
                                       &m_database.TIA,
                                       m_database.FIRA.FIR_Array_p,       /* FIRA Use OK */
                                       m_database.TIA.rows,
                                       &m_allEnabledFilterItems,
                                       &m_database.filterItem_LUT[0],
                                       &m_filterExecTimes,
                                       m_priority,
                                       g_cfg_p->m_Log_colClip_Start,
                                       g_cfg_p->m_Log_colClip_End,
                                       g_cfg_p->m_Log_rowClip_Start,
                                       g_cfg_p->m_Log_rowClip_End,
                                       &m_database.FIRA.filterMatches,
                                       &m_database.FIRA.filterExcludeMatches,
                                       &bookmarkList);

            (void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);
            TRACEX_I(QString("Filter processing - pack [%1] total [%2]")
                         .arg(timeToString(m_filterExecTimes.packTime))
                         .arg(timeToString(m_filterExecTimes.totalFilterTime)))
        } else {
            g_processingCtrl_p->AddProgressInfo(QString("Failed to aquire memory for filtering, filtering aborted"));
        }
    } else {
        g_processingCtrl_p->AddProgressInfo(QString("Log file is empty, filtering aborted"));
    }
}

/***********************************************************************************************************************
*   ExecuteIncrementalFiltering
***********************************************************************************************************************/
void CLogScrutinizerDoc::ExecuteIncrementalFiltering(int startRow)
{
    QList<int> bookmarkList;

    if (CFG_CLIP_NOT_SET != g_cfg_p->m_Log_rowClip_End) {
        return;
    }

    if (m_allEnabledFilterItems.isEmpty()) {
        return;
    }

    QByteArray hash;
    CWorkspace_GetFiltersHash(hash);
    if (hash != m_filtersHash) {
        TRACEX_I(QString("Filter items has changed, doing full filtering"))
        Filter();
        return;
    }

    CWorkspace_GetBookmarks(&bookmarkList);

    if (nullptr == m_workMem.GetRef()) {
        TRACEX_E(QString("%1 Internal error, WorkMem not set").arg(__FUNCTION__))
    }

    CFilterProcCtrl filterCtrl;

    filterCtrl.StartProcessing(&m_qFile_Log,
                               m_workMem.GetRef(),
                               m_workMem.GetSize(),
                               &m_database.TIA,
                               m_database.FIRA.FIR_Array_p, /* FIRA Use OK */
                               m_database.TIA.rows,
                               &m_allEnabledFilterItems,
                               &m_database.filterItem_LUT[0],
                               &m_filterExecTimes,
                               m_priority,
                               g_cfg_p->m_Log_colClip_Start,
                               g_cfg_p->m_Log_colClip_End,
                               startRow,
                               CFG_CLIP_NOT_SET,
                               &m_database.FIRA.filterMatches,
                               &m_database.FIRA.filterExcludeMatches,
                               &bookmarkList,
                               true /*incremental*/);

    CreatePackedFIRA();

    CSZ_DB_PendingUpdate = false;
    CEditorWidget_Repaint();
    CleanRowCache();
}

/***********************************************************************************************************************
*   StartSearch
* Delta ontop of what is already there
***********************************************************************************************************************/
void CLogScrutinizerDoc::StartOneLineFiltering(int row, bool isBookmarkRemove)
{
    QList<int> bookmarkList;
    CWorkspace_GetBookmarks(&bookmarkList);

    char *text_p = nullptr;
    int textLength = 0;

    if (m_database.TIA.rows == 0) {
        return;
    }

    GetTextItem(row, &text_p, &textLength);

    CFilterProcCtrl filterCtrl;

    filterCtrl.StartOneLine(
        &m_database.TIA,
        m_database.FIRA.FIR_Array_p,
        row,
        text_p,
        textLength,
        &m_allEnabledFilterItems,
        &m_database.filterItem_LUT[0],
        &m_database.FIRA.filterMatches,
        &m_database.FIRA.filterExcludeMatches,
        &bookmarkList,
        isBookmarkRemove);

    CreatePackedFIRA();

    CSZ_DB_PendingUpdate = false;
    CEditorWidget_Repaint();
    CleanRowCache();
}

/***********************************************************************************************************************
*   StartSearch
***********************************************************************************************************************/
bool CLogScrutinizerDoc::StartSearch(const QString& searchText, int startRow, int endRow,
                                     int *row_p, bool backward, bool onlyFiltered, bool regExp,
                                     bool caseSensitive)
{
    *row_p = 0;

    m_pendingSearch_searchText = searchText;
    m_pendingSearch_startRow = startRow;
    m_pendingSearch_endRow = endRow;
    m_pendingSearch_row_p = row_p;
    m_pendingSearch_backward = backward;
    m_pendingSearch_OnlyFiltered = onlyFiltered;
    m_pendingSearch_regExp = regExp;
    m_pendingSearch_caseSensitive = caseSensitive;

    TRACEX_I("Start Search  Rows:%d(%d) Start:%d End:%d RegEx:%d CS:%d Text: %s\n ",
             m_pendingSearch_backward ?
             m_pendingSearch_startRow - m_pendingSearch_endRow :
             m_pendingSearch_endRow - m_pendingSearch_startRow,
             m_database.TIA.rows,
             m_pendingSearch_startRow,
             m_pendingSearch_endRow,
             m_pendingSearch_regExp,
             m_pendingSearch_caseSensitive,
             m_pendingSearch_searchText.toLatin1().constData())

#ifdef TEST_FILEPROC
 #ifdef _DEBUG
    for (int index = 1; index < 100; ++index) {
        g_cfg_p->m_workMemSize = index * 1000 * 1024;
    }
 #endif
#endif
    CEditorWidget_SetFocus();

    QCursor cursor = QCursor(Qt::WaitCursor);
    CEditorWidget_SetCursor(&cursor);

    CProgressDlg dlg("Searching...", ProgressCmd_Search_en);
    dlg.setModal(true);

    int res = 1;
    if (!dlg.m_waitGUILock.tryAcquire(1, 2000 /*ms*/)) {
        res = dlg.exec();
    }

    CEditorWidget_SetCursor(nullptr);

    if (res) {
        return PostProcSearch();
    } else {
        return res;
    }
}

/***********************************************************************************************************************
*   ExecuteSearch
***********************************************************************************************************************/
bool CLogScrutinizerDoc::ExecuteSearch(void)
{
    bool result = false;
    int savedSize = g_cfg_p->m_workMemSize;

    if ((g_cfg_p->m_workMemSize == 0) || (g_cfg_p->m_workMemSize > 100 * 1000 * 1024)) {
        /* limit the search memory to 100MB, otherwise the search will take long time initially loading mem */
        g_cfg_p->m_workMemSize = 100 * 1000 * 1024;
    }

    if (m_database.TIA.rows != 0) {
        if (m_workMem.Operation(WORK_MEM_OPERATION_COMMIT)) {
            CSearchCtrl searchCtrl;

            searchCtrl.StartProcessing(
                &m_qFile_Log,
                m_workMem.GetRef(),
                m_workMem.GetSize(),
                &m_database.TIA,
                m_pendingSearch_OnlyFiltered ? &m_database.FIRA : nullptr,
                m_pendingSearch_OnlyFiltered ? m_database.filterItem_LUT : nullptr,
                m_priority,
                &m_pendingSearch_searchText,
                m_pendingSearch_startRow,
                m_pendingSearch_endRow,
                m_pendingSearch_backward,
                m_pendingSearch_regExp,
                m_pendingSearch_caseSensitive);

            result = searchCtrl.GetSearchResult(m_pendingSearch_row_p);

            (void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);
        } else {
            g_processingCtrl_p->AddProgressInfo(QString("Failed to aquire memory for search, search aborted"));
        }
    } else {
        g_processingCtrl_p->AddProgressInfo(QString("Log file is empty, search aborted"));
    }

    g_cfg_p->m_workMemSize = savedSize;

    return result;
}

/***********************************************************************************************************************
*   PostProcSearch
***********************************************************************************************************************/
bool CLogScrutinizerDoc::PostProcSearch(void)
{
    MW_Refresh();
    return g_processingCtrl_p->m_success;
}

/***********************************************************************************************************************
*   StartPlot
***********************************************************************************************************************/
bool CLogScrutinizerDoc::StartPlot(QList<CPlot *> *pendingPlot_execList_p, int startRow, int endRow)
{
    if ((pendingPlot_execList_p == nullptr) || pendingPlot_execList_p->isEmpty()) {
        return false;
    }

    if ((startRow == 0) && (endRow == 0)) {
        endRow = m_database.TIA.rows - 1;
    }

    if ((g_cfg_p->m_Log_rowClip_Start > 0) && (startRow < g_cfg_p->m_Log_rowClip_Start)) {
        startRow = g_cfg_p->m_Log_rowClip_Start + 1;
    }

    if ((g_cfg_p->m_Log_rowClip_End > 0) && (endRow > g_cfg_p->m_Log_rowClip_End)) {
        endRow = g_cfg_p->m_Log_rowClip_End - 1;
    }

    m_pendingPlot_execList_p = pendingPlot_execList_p;
    m_pendingPlot_startRow = startRow;
    m_pendingPlot_endRow = endRow;

    CProgressDlg dlg("Running plot generation...", ProgressCmd_Plot_en);
    dlg.setModal(true);
    dlg.exec();

    /*    PostProcPlot(); */

    if (g_processingCtrl_p->m_abort && g_processingCtrl_p->m_isException) {
        QMessageBox msgBox(QMessageBox::Information,
                           QObject::tr(g_processingCtrl_p->m_exceptionTitle),
                           QObject::tr(g_processingCtrl_p->m_exceptionInfo),
                           QMessageBox::Ok);
        msgBox.exec();
    }

    return true;
}

/***********************************************************************************************************************
*   ExecutePlot
***********************************************************************************************************************/
bool CLogScrutinizerDoc::ExecutePlot(void)
{
    for (QList<CPlot *>::iterator iter = m_pendingPlot_execList_p->begin();
         iter != m_pendingPlot_execList_p->end(); ++iter) {
        (*iter)->PlotClean();
        (*iter)->PlotBegin();
    }

    if (m_database.TIA.rows != 0) {
        if (m_workMem.Operation(WORK_MEM_OPERATION_COMMIT)) {
            CPlotCtrl plotCtrl;

            plotCtrl.Start_PlotProcessing(
                &m_qFile_Log,
                m_workMem.GetRef(),
                m_workMem.GetSize(),
                &m_database.TIA,
                m_priority,
                m_pendingPlot_execList_p,
                m_pendingPlot_startRow,
                m_pendingPlot_endRow);

            (void)m_workMem.Operation(WORK_MEM_OPERATION_FREE);
        }
    }

    for (auto& plot : *m_pendingPlot_execList_p) {
        plot->PlotEnd();
    }

    return true;
}

/***********************************************************************************************************************
*   PostProcPlot
***********************************************************************************************************************/
void CLogScrutinizerDoc::PostProcPlot(void)
{
    MW_Refresh();
}

/***********************************************************************************************************************
*   RefreshFIRACache
***********************************************************************************************************************/
void CLogScrutinizerDoc::RefreshFIRACache(int rowIndex)
{
    CleanRowCache();
}

/***********************************************************************************************************************
*   ClearDecoders
***********************************************************************************************************************/
void CLogScrutinizerDoc::ClearDecoders(void)
{
    m_rowCache_p->m_decoders.clear();
    CleanRowCache();
}

/***********************************************************************************************************************
*   AddDecoders
***********************************************************************************************************************/
void CLogScrutinizerDoc::AddDecoders(QList<CCfgItem *> *decoderList_p) /*QList<CCfgItem_Decoder*> *decoderList_p) */
{
    if (decoderList_p->isEmpty()) {
        return;
    }

    for (auto decoder : *decoderList_p) {
        m_rowCache_p->m_decoders.append(static_cast<CCfgItem_Decoder *>(decoder));
    }
}

/***********************************************************************************************************************
*   RemoveDecoder
***********************************************************************************************************************/
void CLogScrutinizerDoc::RemoveDecoder(CCfgItem *cfgDecoder_p)
{
    if (m_rowCache_p->m_decoders.isEmpty()) {
        return;
    }
    m_rowCache_p->m_decoders.removeOne(static_cast<CCfgItem_Decoder *>(cfgDecoder_p));
}

/***********************************************************************************************************************
*   Decode
***********************************************************************************************************************/
void CLogScrutinizerDoc::Decode(int cacheIndex)
{
    m_rowCache_p->Decode(cacheIndex);
}

/***********************************************************************************************************************
*   ReNumerateFIRA
***********************************************************************************************************************/
void CLogScrutinizerDoc::ReNumerateFIRA(void)
{
    FilterMgr::ReNumerateFIRA(m_database.FIRA, m_database.TIA, m_database.filterItem_LUT);
}

/***********************************************************************************************************************
*   CreatePackedFIRA
***********************************************************************************************************************/
void CLogScrutinizerDoc::CreatePackedFIRA(void)
{
    /* Go through the entire FIRA and move the items to the packed FIRA */
    if (m_database.packedFIRA_p != nullptr) {
        VirtualMem::Free(m_database.packedFIRA_p);
    }

    m_database.packedFIRA_p = nullptr;

    if (m_database.FIRA.filterMatches == 0) {
        return;
    }

    m_database.packedFIRA_p = FilterMgr::GeneratePackedFIRA(m_database.TIA, m_database.FIRA, m_database.filterItem_LUT);
}

/***********************************************************************************************************************
*   VerifyDB
***********************************************************************************************************************/
void CLogScrutinizerDoc::VerifyDB(void)
{
    const int DB_Size = m_database.TIA.rows;
    int index;
    TI_t *ti_p = m_database.TIA.textItemArray_p;
    const int MAX_RowSize = FILECTRL_ROW_MAX_SIZE_persistent;
    int MAX_Line = 0;
    int MAX_RowIndex = 0;
    int numOfMAX_Lines = 0;

    for (index = 0; index < DB_Size; ++index) {
        if (ti_p->size > MAX_Line) {
            MAX_Line = ti_p->size;
            MAX_RowIndex = index;
        }

        if (ti_p->size > MAX_RowSize) {
            ++numOfMAX_Lines;
        }

        ++ti_p;
    }

    TRACEX_I("Max row:%d size:%d, number of very long rows (>%d):%d ",
             MAX_RowIndex, MAX_Line, MAX_RowSize, numOfMAX_Lines);
}

/***********************************************************************************************************************
*   SaveRowsToFile
***********************************************************************************************************************/
bool CLogScrutinizerDoc::SaveRowsToFile(char *fileName_p, int startRow, int endRow)
{
    if ((startRow < 0) && (endRow <= -startRow)) {
        TRACEX_W("Failed to save row clipped region to file, row clips not set")
        return false;
    }

    QFile outputFile(fileName_p);

    if (!outputFile.open(QIODevice::WriteOnly)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to save row clipped region to file", &outputFile);
        return false;
    }

    for (int row = startRow; row <= endRow; ++row) {
        /* include endRow when saving rows */
        int size;
        char *text_p;
        GetTextItem(row, &text_p, &size);

        memcpy(g_largeTempString, text_p, size);

        g_largeTempString[size++] = 0x0d;
        g_largeTempString[size++] = 0x0a;

        int writtenBytes = static_cast<int32_t>(outputFile.write(g_largeTempString, size));

        if (writtenBytes != size) {
            TRACEX_W(
                "While writing text to file not all data was stored. Please try again");
        }
    }

    outputFile.close();
    return true;
}

/***********************************************************************************************************************
*   GetAbsoluteFileName
***********************************************************************************************************************/
void CLogScrutinizerDoc::GetAbsoluteFileName(const QString& relativeFileName, QString& absoluteFileName)
{
    /* Based on the current workspace path tries to construct an absolute path */
    QFileInfo workspaceInfo(m_workspaceFileName);
    QFileInfo fileInfo(workspaceInfo.canonicalPath(), relativeFileName);
    absoluteFileName = fileInfo.canonicalFilePath();
}

/***********************************************************************************************************************
*   CanFileBeOpenedForFileRead
***********************************************************************************************************************/
bool CLogScrutinizerDoc::CanFileBeOpenedForFileRead(QString& fileName, bool promtWarning)
{
    QString absFileName(fileName);

    GetAbsoluteFileName(absFileName, absFileName);

    QFile tempFile(absFileName);

    if (!tempFile.open(QIODevice::ReadOnly)) {
        if (promtWarning) {
            QMessageBox msgBox(QMessageBox::Warning,
                               QObject::tr("Failed to open file"),
                               QObject::tr("Couldn't open file: %1").arg(absFileName),
                               QMessageBox::Ok);
            msgBox.exec();
        }
        return false;
    } else {
        tempFile.close();
        return true;
    }
}

/***********************************************************************************************************************
*   CanFileBeOpenedForFileReadWrite
***********************************************************************************************************************/
bool CLogScrutinizerDoc::CanFileBeOpenedForFileReadWrite(QString& fileName, int maxFileNameSize,
                                                         bool promtWarning, int sharing, bool tryOtherName)
{
    /* This function checks if the file can be opened/created with the sharing specified. If tryOtherName is set then
     * this
     *  function will try to create a new name with _X, where X enumerates from 1 to 10.
     *  If the enumeration of file name succeeds the function will return true and the new file name is copied to the
     * supplied
     *  fileName input parameter. Make sure that there is space for a larger file name in the provided input parameters
     *  fileName. Specify the max size of the fileName in maxFileNameSize.
     */
    QFile temp(fileName);

    if (temp.open(QIODevice::ReadWrite)) {
        temp.close();
        return true;
    }

    if (!tryOtherName) {
        if (promtWarning) {
            TRACEX_W(QString("Warning: Failed to open file: %s").arg(temp.fileName()))

            QString message("Couldn't open file: ");
            message += fileName;

            QMessageBox msgBox(QMessageBox::Warning,
                               QObject::tr("Failed to open file"),
                               QObject::tr(message.toLatin1().constData()),
                               QMessageBox::Ok);
            msgBox.exec();
        }
        return false;
    } else {
        /* tryOtherName */

        QFileInfo fileInfo(temp);
        QString ext = QString(".") + fileInfo.suffix();
        QString fileName_tmp;
        int index = 1;

        while (!temp.isOpen() && index < 10) {
            fileName_tmp = fileName;
            fileName_tmp.replace(ext, QString("_") + QString::number(index) + ext);
            temp.setFileName(fileName_tmp);
            temp.open(QIODevice::ReadWrite);
        } /* while */

        if (temp.isOpen()) {
            fileName = fileName_tmp; /* SUCCESS */
            return true;
        } else if (promtWarning) {
            TRACEX_W(QString("Warning: Failed to open file: %s").arg(fileName))

            if (promtWarning) {
                QString message("Couldn't open file: ");
                message += fileName;

                QMessageBox msgBox(QMessageBox::Warning, "Failed to open file", message, QMessageBox::Ok);
                msgBox.exec();
            }
        } /* promtWarning */
        return false;
    } /* try other name */
    return false;
}

/***********************************************************************************************************************
*   GetRecommendedPath
***********************************************************************************************************************/
QString CLogScrutinizerDoc::GetRecommendedPath(void)
{
    QString filePath_CStr = "";

    /* Check if environment path is setup */
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    QString lsPath = processEnvironment.value("LS_PATH");

    if (!lsPath.isEmpty()) {
        TRACEX_W(QString("Using environment variable LS_PATH (%s)").arg(lsPath))
        return lsPath;
    } else {
        TRACEX_D("CLogScrutinizerDoc::CLogScrutinizerDoc    Reading environment "
                 "var LS_PATH failed, it doesn't exist");
    }

    filePath_CStr = m_recentFiles.GetRecentPath(RecentFile_Kind_LogFile_en);

    if (filePath_CStr.isEmpty()) {
        filePath_CStr = m_recentFiles.GetRecentPath(RecentFile_Kind_FilterFile_en);
    }
    if (filePath_CStr.isEmpty()) {
        filePath_CStr = m_recentFiles.GetRecentPath(RecentFile_Kind_WorkspaceFile_en);
    }
    if (filePath_CStr.isEmpty()) {
        filePath_CStr = m_recentFiles.GetRecentPath(RecentFile_Kind_PluginFile_en);
    }
    if (filePath_CStr.isEmpty()) {
        filePath_CStr = QDir::currentPath();
    }

    return filePath_CStr;
}

/***********************************************************************************************************************
*   GetWorkspaceDirectory
***********************************************************************************************************************/
QDir CLogScrutinizerDoc::GetWorkspaceDirectory()
{
    QFileInfo fileInfo(GetWorkspaceFileName());
    return fileInfo.dir();
}
