/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QSurfaceFormat>

#include "CDebug.h"
#include "CRecentFile.h"
#include "CMemPool.h"
#include "CFileCtrl.h"
#include "CSearchCtrl.h"
#include "CFileProcBase.h"
#include "CFilterProcCtrl.h"
#include "CRowCache.h"
#include "filemapping.h"
#include "TextDecoration.h"
#include "CParseTesting.h"
#include "CLogScrutinizerDoc.h"
#include "CWorkspace.h"
#include "CMemPool.h"
#include "CConfigurationCtrl.h"
#include "CProgressDlg.h"

#include <QDir>
#include <QFileDevice>

#define TOTAL_NUM_OF_ROWS (1024 * 1024 * 1)

FilterItemInitializer myFilters[] = {{"Match me", false, false}};
extern void TestFileCtrl(void);
extern void TestSeek();
extern bool TestDocument();

void MultiTestFiltering(void);
bool TestFiltering(bool useIfExist = true);
bool TestSearch(bool useIfExist);
bool TestRowCacheAndAutoHighlight(void);
bool TestWorkspace(void);
void TestMemory(void);
void TestLoadAndFilter(void);

bool GenerateFilterTestLog(const QString& fileName, const QString& repetitionPattern, const QString& matchPattern,
                           int totalNumOfRows, int modulus, bool useIfExist);
bool LoadMapTIAandFIRA(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, uint8_t *& TIA_mem_p,
                       uint8_t *& FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/, int& rows);
bool LoadMapTIAandFIRA_filemapping(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File,
                                   TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/, int& rows);
bool Filter(QFile& Log_File, int rows, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/,
            FilterItemInitializer *filterInitializers_p, int count, CFilterContainer& container);
int Search(QFile& Log_File, uint8_t *TIA_mem_p, uint8_t *FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/,
           QString& searchText, unsigned int startRow, unsigned int endRow, bool backward, bool regExp);
bool VerifyTIA(TIA_t& TIA, const QString& repetitionPattern, const QString& matchPattern, int totalNumOfRows,
               int modulus);
bool VerifyFIRA(FIRA_t& FIRA, TIA_t& TIA, const QString& repetitionPattern, const QString& matchPattern,
                int totalNumOfRows, int modulus);
void CloseAndUnmap(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, uint8_t *& TIA_mem_p,
                   uint8_t *& FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA);
void CloseAndUnmap_filemapping(QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, TIA_t& TIA, FIRA_t& FIRA);

/***********************************************************************************************************************
*   MainTest
***********************************************************************************************************************/
void MainTest()
{
    g_RamLog->fileDump(false);

    TRACEX_I("----------- Test memory ----------\n");
    TestMemory();

    TRACEX_I("----------- Test workspace ----------\n");
    TestWorkspace();

    TRACEX_I("\n\n----------- TestRowCache ----------\n");
    TestRowCacheAndAutoHighlight();

    TRACEX_I("----------- Test Load and Filter --------\n");
    TestLoadAndFilter();

    /*
     *  CProgressDlg dlg(QString("hej"), ProgressCmd_TestProgress_en);
     *  dlg.setModal(true);
     *  dlg.exec();
     */
    TRACEX_I("----------- Test Parsing ----------\n");
    {
        /* Plugin text parse testing */
        CParseTesting parseTesting;
        parseTesting.ParseTest();
    }

    TRACEX_I("----------- Test Document ----------\n");
    if (!TestDocument()) {
        g_DebugLib->ErrorHook("TestDocument settings failed");
    }

    CRecentFile m_recentFiles;
    m_recentFiles.Test();

    QFile *file_p = new QFile("testing.lsz");

    if (!file_p->open(QIODevice::WriteOnly | QIODevice::Text)) {
        g_DebugLib->ErrorHook("Write settings failed");
    }

    CTimeMeas measTime;
    g_cfg_p->WriteSettings(file_p);

    file_p->flush();
    file_p->close();

    TRACEX_I("----------- VirtualMem::test ----------\n");

    if (!VirtualMem::test()) {
        TRACEX_E("VirtualMem::test() failed\n");
    }

    TRACEX_I("----------- myRecentFile ----------\n");

    CRecentFile myRecentFile;

    myRecentFile.Test();

    TRACEX_I("\n\n----------- TestSeek ----------\n");

    TestSeek();

    TRACEX_I("\n\n----------- MultiTestFiltering ----------\n\n\n");

    MultiTestFiltering();

    TRACEX_I("\n\n----------- TestFileCtrl ----------\n\n\n");

    TestFileCtrl();

    TRACEX_I("\n\n----------- TestSearch ----------\n\n\n");

    TestSearch(true);

    TRACEX_I("\n\n----------- TestFiltering ----------\n\n\n");

    (void)TestFiltering();

    TRACEX_I(QString("Time: to write settings:%1").arg(measTime.ms()));
}

/***********************************************************************************************************************
*   TestLoadAndFilter
***********************************************************************************************************************/
void TestLoadAndFilter(void)
{
    auto doc_p = GetTheDoc();
    QDir dir = QFileInfo(doc_p->m_Log_FileName).dir();
    QStringList nameFltFilter("*.flt");
    QStringList nameLogFilter;
    nameLogFilter.append("*.log");
    nameLogFilter.append("*.txt");

    bool autoClose = g_cfg_p->m_autoCloseProgressDlg;
    g_cfg_p->m_autoCloseProgressDlg = true;

    doc_p->CleanDB();

    QStringList fltFiles = dir.entryList(nameFltFilter);
    QStringList logFiles = dir.entryList(nameLogFilter);

    if ((logFiles.count() == 0) || (fltFiles.count() == 0)) {
        TRACEX_E("Files missing for tests");
    }

    TRACEX_I(QString("   Filter file:%1").arg(fltFiles.first()));
    for (auto& logFile : logFiles) {
        TRACEX_I(QString("   Log file:%1").arg(logFile));
    }

    QStringList oneFilter;
    oneFilter.append(dir.filePath(fltFiles.first()));
    CFGCTRL_LoadFiles(oneFilter);

    for (int i = 0; i < 20; i++) {
        for (auto& logFile : logFiles) {
            QStringList temp;
            temp.append(dir.filePath(logFile));
            CFGCTRL_LoadFiles(temp);

            doc_p->Filter();
            doc_p->Filter();
        }
        TRACEX_I(QString("   %1").arg(i));
    }

    doc_p->CleanDB();

    g_cfg_p->m_autoCloseProgressDlg = autoClose;
}

/***********************************************************************************************************************
*   MultiTestFiltering
***********************************************************************************************************************/
void MultiTestFiltering(void)
{
    for (int index = 0; index < 4; index++) {
        /*if (!TestFiltering(index != 0 ? true : false)) { */
        if (!TestFiltering(true)) {
            TRACEX_E("MultiTestFilter Failed\n");
            return;
        }
    }
}

/***********************************************************************************************************************
*   TestFiltering
***********************************************************************************************************************/
bool TestFiltering(bool useIfExist)
{
#define TEST_FILTER_PROC_MEM_SIZE  (1024 * 1024 * 50)

    /*#define TEST_FILTER_PROC_MEM_SIZE  (1024 * 1024 * 2) */

    char *mem_p = (char *)VirtualMem::Alloc(TEST_FILTER_PROC_MEM_SIZE);

    if (mem_p == nullptr) {
        TRACEX_E("TestFiltering - Virtual Alloc failed\n");
        return false;
    }

    CFilterContainer container;
    QString logFileName = "test_log.txt";
    QString repetitionPattern = "Dummy string Dummy string Dummy string Dummy string";
    QString matchPattern = "Match me";
    int totalNumOfRows = TOTAL_NUM_OF_ROWS;
    int modulus = 10;
    TIA_t TIA;
    FIRA_t FIRA;
    QFile Log_File, TIA_File, FIRA_File;

    if (!GenerateFilterTestLog(logFileName, repetitionPattern, matchPattern, totalNumOfRows, modulus, useIfExist)) {
        TRACEX_E("TestFiltering - Test file couldn't be generated\n");
        return false;
    }

    if (!LoadMapTIAandFIRA_filemapping(logFileName, Log_File, TIA_File, FIRA_File, TIA, FIRA, mem_p, totalNumOfRows)) {
        TRACEX_E("TestFiltering - LoadMapTIAandFIRA\n");
        return false;
    }

    if (!Filter(Log_File, totalNumOfRows, TIA, FIRA, mem_p, myFilters, 1, container)) {
        TRACEX_E("TestFiltering - Filter\n");
        return false;
    }

    if (!VerifyTIA(TIA, repetitionPattern, matchPattern, totalNumOfRows, modulus)) {
        TRACEX_E("TestFiltering - VerifyTIA\n");
        return false;
    }

    if (!VerifyFIRA(FIRA, TIA, repetitionPattern, matchPattern, totalNumOfRows, modulus)) {
        TRACEX_E("TestFiltering - VerifyFIRA\n");
        return false;
    }

    CloseAndUnmap_filemapping(Log_File, TIA_File, FIRA_File, TIA, FIRA);

    VirtualMem::Free(mem_p);

    return true;
}

/***********************************************************************************************************************
*   TestRowCacheAndAutoHighlight
***********************************************************************************************************************/
bool TestRowCacheAndAutoHighlight(void)
{
#define TEST_ROW_CACHE_PROC_MEM_SIZE  (1024 * 1024 * 50)

    char *mem_p = (char *)VirtualMem::Alloc(TEST_ROW_CACHE_PROC_MEM_SIZE);

    if (mem_p == nullptr) {
        TRACEX_E("TestRowCacheAndAutoHighlight - Virtual Alloc failed\n");
        return false;
    }

    QString logFileName = "D:\\Projects\\LogScrutinizer\\logs\\testLog1.txt";
    QString repetitionPattern = "Dummy string Dummy string Dummy string Dummy string";
    QString matchPattern = "Match me";
    int totalNumOfRows = TOTAL_NUM_OF_ROWS;
    int modulus = 10;
    CFilterContainer container;
    uint8_t *TIA_mem_p, *FIRA_mem_p;
    TIA_t TIA;
    FIRA_t FIRA;
    QFile Log_File, TIA_File, FIRA_File;

    if (!GenerateFilterTestLog(logFileName, repetitionPattern, matchPattern, totalNumOfRows, modulus, true)) {
        TRACEX_E("TestRowCacheAndAutoHighlight - Test file couldn't be generated\n");
        return false;
    }

    if (!LoadMapTIAandFIRA(logFileName, Log_File, TIA_File, FIRA_File, TIA_mem_p, FIRA_mem_p, TIA, FIRA,
                           mem_p /*input*/, totalNumOfRows)) {
        TRACEX_E("TestRowCacheAndAutoHighlight - LoadMapTIAandFIRA\n");
        return false;
    }

    if (!Filter(Log_File, totalNumOfRows, TIA, FIRA, mem_p, myFilters, 1, container)) {
        TRACEX_E("TestRowCacheAndAutoHighlight - Filter\n");
        return false;
    }

    if (!VerifyTIA(TIA, repetitionPattern, matchPattern, totalNumOfRows, modulus)) {
        TRACEX_E("TestRowCacheAndAutoHighlight - VerifyTIA\n");
        return false;
    }

    if (!VerifyFIRA(FIRA, TIA, repetitionPattern, matchPattern, totalNumOfRows, modulus)) {
        TRACEX_E("TestRowCacheAndAutoHighlight - VerifyFIRA\n");
        return false;
    }

    const CMemPool_Config_t screenCacheMemPoolConfig =
    {
        5,                              /* numOfRanges */
        {CACHE_MEM_MAP_SIZE, 8, 4, 1, 1, 0},   /* startNumPerRange */
        {CACHE_CMEM_POOL_SIZE_SMALLEST, CACHE_CMEM_POOL_SIZE_1, CACHE_CMEM_POOL_SIZE_2, CACHE_CMEM_POOL_SIZE_3,
         CACHE_CMEM_POOL_SIZE_MAX, 0}   /* ranges */
    };
    CFilterItem **filterItem_LUT = container.GetFilterLUT();
    packed_FIR_t *LUT_FIR_p = FilterMgr::GeneratePackedFIRA(TIA, FIRA, filterItem_LUT);
    CMemPool memPool(&screenCacheMemPoolConfig);
    CRowCache rowCache(&Log_File, &TIA, &FIRA, filterItem_LUT, memPool);
    char *text_p;
    int size;
    int props;
    const int repPatLength = repetitionPattern.length();
    const int matchPatLength = matchPattern.length();

    for (int index = 0; index < totalNumOfRows; ++index) {
        rowCache.Get(index, &text_p, &size, &props);

        if ((index % modulus) == 0) {
            if (matchPatLength != size) {
                TRACEX_E("TestRowCacheAndAutoHighlight  VerifyTIA Failed - Line not matching");
                return false;
            }
        } else {
            if (repPatLength != size) {
                TRACEX_E("TestRowCacheAndAutoHighlight  VerifyTIA Failed - Line not matching");
                return false;
            }
        }
    }

    /* Do the auto highlight testing */
    CAutoHighLight autoHighlight(&rowCache);
    autoHighlight.AutoHighlightTest();

    /* Wrapup and exit */
    CloseAndUnmap(logFileName, Log_File, TIA_File, FIRA_File, TIA_mem_p, FIRA_mem_p, TIA, FIRA);

    VirtualMem::Free(LUT_FIR_p);
    VirtualMem::Free(mem_p);

    return true;
}

/***********************************************************************************************************************
*   TestSearch
***********************************************************************************************************************/
bool TestSearch(bool useIfExist)
{
#define TEST_FILTER_PROC_MEM_SIZE  (1024 * 1024 * 50)

    /*#define TEST_FILTER_PROC_MEM_SIZE  (1024 * 1024 * 2) */

    CSearchCtrl searchCtrl;
    char *mem_p = (char *)VirtualMem::Alloc(TEST_FILTER_PROC_MEM_SIZE);

    if (mem_p == nullptr) {
        TRACEX_E("TestFiltering - Virtual Alloc failed\n");
        return false;
    }

    QString logFileName = "testLog1.txt";
    QString repetitionPattern = "Dummy string Dummy string Dummy string Dummy string";
    QString matchPattern = "Match me";
    int totalNumOfRows = TOTAL_NUM_OF_ROWS;
    int modulus = 10;
    uint8_t *TIA_mem_p, *FIRA_mem_p;
    TIA_t TIA;
    FIRA_t FIRA;
    QFile Log_File, TIA_File, FIRA_File;

    if (!GenerateFilterTestLog(logFileName, repetitionPattern, matchPattern, totalNumOfRows, modulus, useIfExist)) {
        TRACEX_E("TestSearch - Test file couldn't be generated\n");
        return false;
    }

    if (!LoadMapTIAandFIRA(logFileName, Log_File, TIA_File, FIRA_File, TIA_mem_p, FIRA_mem_p, TIA, FIRA,
                           mem_p /*input*/, totalNumOfRows)) {
        TRACEX_E("TestSearch - LoadMapTIAandFIRA\n");
        return false;
    }

    if (!VerifyTIA(TIA, repetitionPattern, matchPattern, totalNumOfRows, modulus)) {
        TRACEX_E("TestSearch - VerifyTIA\n");
        return false;
    }

    int startRow = 0;
    int endRow = TIA.rows - 1;
    bool backward = false;
    bool regExp = false;
    int matchRow;
    int nextMatchRow = 0;

    /* Forward search */
    for (int currentRow = startRow; currentRow < 100; currentRow++) {
        matchRow = Search(Log_File,
                          TIA_mem_p,
                          nullptr,
                          TIA,
                          FIRA,
                          mem_p,
                          matchPattern,
                          currentRow,
                          endRow,
                          backward,
                          regExp);
        if (matchRow == -1) {
            TRACEX_E("TestSearch - Filter\n");
            return false;
        }

        if (matchRow != nextMatchRow) {
            TRACEX_E("TestSearch - Filter\n");
            return false;
        }

        if (currentRow % 10 == 0) {
            nextMatchRow = ((currentRow / modulus) * modulus) + modulus;
        }
    }

    /* Backward search */

    backward = true;
    endRow = (endRow / 10) * 10;
    nextMatchRow = endRow;

    for (int currentRow = endRow; currentRow > endRow - 100; currentRow--) {
        matchRow =
            Search(Log_File, TIA_mem_p, nullptr, TIA, FIRA, mem_p, matchPattern, currentRow, 0, backward, regExp);
        if (matchRow == -1) {
            TRACEX_E("TestSearch - Filter\n");
            return false;
        }

        if (matchRow != nextMatchRow) {
            TRACEX_E("TestSearch - Filter\n");
            return false;
        }

        if (currentRow % 10 == 0) {
            nextMatchRow = endRow - ((((endRow - currentRow) / modulus) * modulus) + modulus);
        }
    }

    CloseAndUnmap(logFileName, Log_File, TIA_File, FIRA_File, TIA_mem_p, FIRA_mem_p, TIA, FIRA);

    VirtualMem::Free(mem_p);

    return true;
}

/***********************************************************************************************************************
*   LoadMapTIAandFIRA
***********************************************************************************************************************/
bool LoadMapTIAandFIRA(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, uint8_t *& TIA_mem_p,
                       uint8_t *& FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/, int& rows)
{
    QString TIA_FileName = LogFileName;
    TIA_FileName.append(".tia");

    QString FIRA_FileName = LogFileName;
    FIRA_FileName.append(".fira");

    Log_File.setFileName(LogFileName);
    TIA_File.setFileName(TIA_FileName);
    FIRA_File.setFileName(FIRA_FileName);

    if (!Log_File.exists()) {
        TRACEX_E("TestFilterProcCtrl Failed - Log File doesn't exist");
        return false;
    }

    if (!Log_File.open(QIODevice::ReadWrite)) {
        TRACEX_E("TestFilterProcCtrl Failed - Log couldn't be open");
        return false;
    }

    FIRA_File.remove();

    if (FIRA_File.exists()) {
        TRACEX_E("TestFilterProcCtrl Failed - FIRA File still exists");
        return false;
    }

    CFileCtrl fileCtrl;
    int tempRows;

    fileCtrl.Search_TIA(&Log_File, TIA_File.fileName(), mem_p, TEST_FILTER_PROC_MEM_SIZE, &tempRows);

    if (rows != tempRows) {
        TRACEX_E("TestFilterProcCtrl Failed - Wrong number of rows");
        return false;
    }

    if (!TIA_File.exists()) {
        TRACEX_E("TestFilterProcCtrl Failed - TIA File doesn't exist");
        return false;
    }

    if (!TIA_File.open(QIODevice::ReadWrite)) {
        TRACEX_E("TestFilterProcCtrl Failed - TIA file couldn't be opened");
        return false;
    }

    TIA_mem_p = TIA_File.map(0, TIA_File.size());

    if (TIA_mem_p == nullptr) {
        TRACEX_E("TestFilterProcCtrl Failed - TIA file couldn't be mapped");
        return false;
    }

    if (reinterpret_cast<TIA_FileHeader_t *>(TIA_mem_p)->numOfRows != rows) {
        TRACEX_E("TestFilterProcCtrl Failed - TIA header doesn't match");
        return false;
    }

    TIA.rows = rows;
    TIA_mem_p += sizeof(TIA_FileHeader_t);
    TIA.textItemArray_p = reinterpret_cast<TI_t *>(TIA_mem_p);

    if (!FIRA_File.open(QIODevice::ReadWrite)) {
        TRACEX_E("TestFilterProcCtrl Failed - FIRA file couldn't be open/created");
        return false;
    }

    if (!FIRA_File.resize(sizeof(FIR_t) * rows)) {
        TRACEX_E("TestFilterProcCtrl Failed - FIRA file couldn't be resized");
        return false;
    }

    FIRA_mem_p = FIRA_File.map(0, sizeof(FIR_t) * rows);

    if (FIRA_mem_p == nullptr) {
        TRACEX_E("TestFilterProcCtrl Failed - FIRA file couldn't be mapped");
        return false;
    }

    FIRA.FIR_Array_p = reinterpret_cast<FIR_t *>(FIRA_mem_p);

    return true;
}

/***********************************************************************************************************************
*   LoadMapTIAandFIRA_filemapping
***********************************************************************************************************************/
bool LoadMapTIAandFIRA_filemapping(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File,
                                   TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/, int& rows)
{
    QString TIA_FileName = LogFileName;
    TIA_FileName.append(".tia");

    QString FIRA_FileName = LogFileName;
    FIRA_FileName.append(".fira");

    Log_File.setFileName(LogFileName);
    TIA_File.setFileName(TIA_FileName);
    FIRA_File.setFileName(FIRA_FileName);

    if (!Log_File.exists()) {
        TRACEX_E("LoadMapTIAandFIRA_filemapping Failed - Log File doesn't exist");
        return false;
    }

    if (!Log_File.open(QIODevice::ReadWrite)) {
        TRACEX_E("LoadMapTIAandFIRA_filemapping Failed - Log couldn't be open");
        return false;
    }

    FIRA_File.remove();

    if (FIRA_File.exists()) {
        TRACEX_E("LoadMapTIAandFIRA_filemapping Failed - FIRA File still exists");
        return false;
    }

    CFileCtrl fileCtrl;
    int tempRows;
    int64_t fileSize;

    fileCtrl.Search_TIA(&Log_File, TIA_File.fileName(), mem_p, TEST_FILTER_PROC_MEM_SIZE, &tempRows);

    TIA.rows = -1; /* this will have CreateTIA_MemMapped set the number of rows found in the TIA file */

    bool status = FileMapping::CreateTIA_MemMapped(Log_File, TIA_File, &TIA.rows, TIA.textItemArray_p, &fileSize);

    if (tempRows != TIA.rows) {
        TRACEX_E("LoadMapTIAandFIRA_filemapping Failed - Number of TIA rows doesn't match\n");
        return false;
    }

    if (status) {
        status = FileMapping::CreateFIRA_MemMapped(FIRA_File, FIRA.FIR_Array_p, rows);
    }

    return status;
}

/***********************************************************************************************************************
*   Filter
***********************************************************************************************************************/
bool Filter(QFile& Log_File, int rows, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/,
            FilterItemInitializer *filterInitializers_p, int count, CFilterContainer& container)
{
    CFilterProcCtrl filterCtrl;
    CFilterItem **filterItem_LUT;
    QList<CFilterItem *> filterItems;

    container.GenerateFilterItems(filterInitializers_p, count);
    container.GenerateLUT();

    filterItem_LUT = container.GetFilterLUT();
    container.PopulateFilterItemList(filterItems);

    FilterExecTimes_t execTimes;
    QList <int> bookmarks;

    filterCtrl.StartProcessing(
        &Log_File,
        mem_p,
        TEST_FILTER_PROC_MEM_SIZE,
        &TIA,
        FIRA.FIR_Array_p,
        rows,
        &filterItems,
        filterItem_LUT,
        &execTimes,
        0,
        -1,
        -1,
        -1,
        -1,
        &FIRA.filterMatches,
        &FIRA.filterExcludeMatches,
        &bookmarks);

    return true;
}

/***********************************************************************************************************************
*   TestDocument
***********************************************************************************************************************/
bool TestDocument()
{
#define TEST_DOC_PROC_MEM_SIZE  (1024 * 1024 * 50)

    char *mem_p = (char *)VirtualMem::Alloc(TEST_DOC_PROC_MEM_SIZE);

    if (mem_p == nullptr) {
        TRACEX_E("TestDocument - Virtual Alloc failed\n");
        return false;
    }

    QString logFileName = "testLog1.txt";
    QString repetitionPattern = "Dummy string Dummy string Dummy string Dummy string";
    QString matchPattern = "Match me";
    int totalNumOfRows = TOTAL_NUM_OF_ROWS;
    int modulus = 10;
    int expected_matches = totalNumOfRows / modulus + 1;

    /*
     *  uint8_t* TIA_mem_p, *FIRA_mem_p;
     *  TIA_t TIA;
     *  FIRA_t FIRA;
     */
    QFile Log_File, TIA_File, FIRA_File;

    if (!GenerateFilterTestLog(logFileName, repetitionPattern, matchPattern, totalNumOfRows, modulus, true)) {
        TRACEX_E("TestDocument - Test file couldn't be generated\n");
        return false;
    }

    CLogScrutinizerDoc *doc = GetTheDoc();

    if (!doc->LoadLogFile(logFileName)) {
        TRACEX_E("TestDocument - Failed to open test log\n");
        return false;
    }

    CFilterContainer container;
    container.GenerateFilterItems(myFilters, sizeof(myFilters) / sizeof(FilterItemInitializer));
    container.GenerateLUT();

    doc->CreateFiltersFromContainer(container);

    doc->ExecuteFiltering();
    doc->PostProcFiltering();

    if (doc->m_database.FIRA.filterMatches != expected_matches) {
        TRACEX_E("TestDocument - Filtering failed, wrong match count %d %d\n",
                 doc->m_database.FIRA.filterMatches,
                 expected_matches);
    }

    doc->CleanDB();

    return true;
}

/***********************************************************************************************************************
*   Search
***********************************************************************************************************************/
int Search(QFile& Log_File, uint8_t *TIA_mem_p, uint8_t *FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA, char *mem_p /*input*/,
           QString& searchText, unsigned int startRow, unsigned int endRow, bool backward, bool regExp)
{
    CSearchCtrl searchCtrl;

    FIRA.FIR_Array_p = nullptr;

    searchCtrl.StartProcessing(
        &Log_File,
        mem_p,
        TEST_FILTER_PROC_MEM_SIZE,
        &TIA,
        nullptr,
        nullptr,
        0,
        &searchText,
        startRow,
        endRow,
        backward,
        regExp,
        false /* case sensitive ... TODO */);

    unsigned int searchTI;
    if (searchCtrl.GetSearchResult(&searchTI)) {
        return static_cast<int>(searchTI);
    } else {
        return -1;
    }
}

/***********************************************************************************************************************
*   CloseAndUnmap
***********************************************************************************************************************/
void CloseAndUnmap(QString& LogFileName, QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, uint8_t *& TIA_mem_p,
                   uint8_t *& FIRA_mem_p, TIA_t& TIA, FIRA_t& FIRA)
{
    if (FIRA_mem_p != nullptr) {
        FIRA_File.unmap(FIRA_mem_p);
    }

    if (TIA_mem_p != nullptr) {
        TIA_File.unmap(TIA_mem_p);
    }

    FIRA_File.close();
    TIA_File.close();
    Log_File.close();
}

/***********************************************************************************************************************
*   CloseAndUnmap_filemapping
***********************************************************************************************************************/
void CloseAndUnmap_filemapping(QFile& Log_File, QFile& TIA_File, QFile& FIRA_File, TIA_t& TIA, FIRA_t& FIRA)
{
    if (FIRA.FIR_Array_p != nullptr) {
        FIRA_File.unmap(reinterpret_cast<uchar *>(FIRA.FIR_Array_p));
    }

    if (TIA.textItemArray_p != nullptr) {
        TIA_File.unmap(reinterpret_cast<uchar *>(TIA.textItemArray_p));
    }

    FIRA_File.close();
    TIA_File.close();
    Log_File.close();
}

/***********************************************************************************************************************
*   GenerateFilterTestLog
***********************************************************************************************************************/
bool GenerateFilterTestLog(const QString& fileName, const QString& repetitionPattern, const QString& matchPattern,
                           int totalNumOfRows, int modulus, bool useIfExist)
{
    QFile LogFile(fileName);

    if (LogFile.exists()) {
        if (useIfExist) {
            return true;
        } else {
            LogFile.remove();
        }
    }

    if (!LogFile.open(QIODevice::ReadWrite)) {
        TRACEX_E("GenerateFilterTestLog Failed - Log couldn't be open");
        return false;
    }

    QTextStream outStream(&LogFile);

    try {
        for (int index = 0; index < totalNumOfRows; ++index) {
            if ((index % modulus) == 0) {
                outStream << matchPattern << "\r\n";
            } else {
                outStream << repetitionPattern << "\r\n";
            }
        } /* for */
    } catch (std::exception &e) {
        TRACEX_E("GenerateFilterTestLog Failed - ASSERT");
        e = e;
        qFatal("  ");
    } catch (...) {
        TRACEX_E("GenerateFilterTestLog Failed - ASSERT");
        qFatal("  ");
    }

    LogFile.close();

    return true;
}

/***********************************************************************************************************************
*   VerifyTIA
***********************************************************************************************************************/
bool VerifyTIA(TIA_t& TIA, const QString& repetitionPattern, const QString& matchPattern, int totalNumOfRows,
               int modulus)
{
    if (TIA.rows != totalNumOfRows) {
        TRACEX_E("VerifyTIA Failed - Number of rows");
        return false;
    }

    const int repPatLength = repetitionPattern.length();
    const int matchPatLength = matchPattern.length();

    for (int index = 0; index < totalNumOfRows; ++index) {
        if ((index % modulus) == 0) {
            if (matchPatLength != TIA.textItemArray_p[index].size) {
                TRACEX_E("VerifyTIA Failed - Line not matching");
                return false;
            }
        } else {
            if (repPatLength != TIA.textItemArray_p[index].size) {
                TRACEX_E("VerifyTIA Failed - Line not matching");
                return false;
            }
        }
    }

    return true;
}

/***********************************************************************************************************************
*   VerifyFIRA
***********************************************************************************************************************/
bool VerifyFIRA(FIRA_t& FIRA, TIA_t& TIA, const QString& repetitionPattern, const QString& matchPattern,
                int totalNumOfRows, int modulus)
{
    int filterIndex = 0;
    for (int index = 0; index < totalNumOfRows; ++index) {
        if ((index % modulus) == 0) {
            if ((FIRA.FIR_Array_p[index].LUT_index <= 0) || (FIRA.FIR_Array_p[index].index != filterIndex)) {
                TRACEX_E("VerifyFIRA - Bad FIRA table, TIA is %d", TIA.textItemArray_p[index].size);
                return false;
            }
            ++filterIndex;
        } else {
            if ((FIRA.FIR_Array_p[index].LUT_index != 0) || (FIRA.FIR_Array_p[index].index != 0)) {
                TRACEX_E("VerifyFIRA - Bad FIRA table");
                return false;
            }
        }
    }

    return true;
}

/***********************************************************************************************************************
*   TestMemory
***********************************************************************************************************************/
void TestMemory(void)
{
    char *mem_p = (char *)VirtualMem::Alloc(1000);
    memset(mem_p, 0x11, 1000);
    VirtualMem::Free(mem_p);
}

/***********************************************************************************************************************
*   TestWorkspace
***********************************************************************************************************************/
bool TestWorkspace(void)
{
    return true;
}
