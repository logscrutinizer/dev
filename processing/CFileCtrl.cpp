/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CFileCtrl.h"
#include "CTimeMeas.h"
#include "CConfig.h"
#include "CDebug.h"
#include "CProgressCtrl.h"
#include "filemapping.h"
#include <QFileInfo>
#include <QDateTime>
#include "CLogScrutinizerDoc.h"

int g_totalNumOfThreads; /* Used for caluclating progress */
int64_t g_totalFileSize; /* Used for caluclating progress */

#define   SEEK_EOL_SUCCESS      1
#define   SEEK_EOL_ERROR       -1
#define   SEEK_EOL_FILE_END    -2

/***********************************************************************************************************************
*   MemRef_To_FileIndex
***********************************************************************************************************************/
inline static uint64_t MemRef_To_FileIndex(uint64_t fileStartIndex, char *startRef_p, char *ref_p)
{
    return fileStartIndex + (ref_p - startRef_p);
}

/***********************************************************************************************************************
*   run
***********************************************************************************************************************/
void CTIA_Thread::run()
{
    /* The thread is responsible for parsing X number of bytes and creating an array of TextItems
     * The TIA is fixed in size, if more TIs are required one additional TIA is created */
    unsigned int currentItemIndex = 0;
    char *ref_p;
    char *start_p;
    int progressCount = PROGRESS_COUNTER_STEP;
    CTimeMeas execTime;
    char *ref_end_p;
    CTIA_Chunk *CTIA_p;
    TI_t *TIA_p;

#ifdef DEBUG_TIA_PARSING
    TRACEX_D("Thread 0x%llx started", this);
#endif

    g_RamLog->RegisterThread();

    auto unregisterRamLog = makeMyScopeGuard([&] () {
        g_RamLog->UnregisterThread();
    });
    unsigned int m_maxNumOf_TI_estimated = m_size / FILECTRL_ROW_SIZE_ESTIMATE_persistent;

    m_maxNumOf_TI_estimated = m_maxNumOf_TI_estimated < FILECTRL_MINIMAL_NUM_OF_TIs_persistent ?
                              FILECTRL_MINIMAL_NUM_OF_TIs_persistent : m_maxNumOf_TI_estimated;

    const bool isSteppingProgressCounter = m_isSteppingProgressCounter;

    CTIA_p = new CTIA_Chunk(m_maxNumOf_TI_estimated);

    if (CTIA_p == nullptr) {
        TRACEX_E("ThreadMain_CTIA  out of memory");
        return;
    }

    m_CTIA_Chunks.append(CTIA_p);
    TIA_p = CTIA_p->m_TIA_p;

    ref_p = m_start_p;
    start_p = ref_p;
    ref_end_p = ref_p + m_size;

    currentItemIndex = 0;

    /* Initialize the first item */
    TIA_p[0].fileIndex = m_fileStartIndex;

    /* if the first row is a LF (unix system) starting with a newline
     * CR=0x0d, LF=0x0a
     * CR + LF, LF */

    if (*ref_p == 0x0a) {
        TIA_p[0].size = 0;
        ++ref_p;
        start_p = ref_p;

        currentItemIndex = 1;
        TIA_p[1].fileIndex = MemRef_To_FileIndex(m_fileStartIndex, m_start_p, start_p);
    }

    while (ref_p < ref_end_p) {
        if (*ref_p == 0x0a) {
            if (isSteppingProgressCounter) {
                --progressCount;
                if (progressCount == 0) {
                    progressCount = PROGRESS_COUNTER_STEP;
                    g_processingCtrl_p->SetProgressCounter(((float)m_fileStartIndex +
                                                            (float)((ref_p - m_start_p) * g_totalNumOfThreads))
                                                           / (float)(g_totalFileSize));
                }
            }

            if (g_processingCtrl_p->m_abort) {
#ifdef DEBUG_TIA_PARSING
                TRACEX_D("Thread 0x%llx early exit", this);
#endif
                return;
            }

            if (*(ref_p - 1) == 0x0d) {
                /* double zeroes, do not include the 0 in the count */
                TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p - 1);
            } else {
                /* double zeroes, do not include the 0 in the count */
                TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p);
            }

            /* initiate the next text item */
            ++currentItemIndex;

            if (currentItemIndex >= m_maxNumOf_TI_estimated) {
                CTIA_p->m_num_TI = currentItemIndex;

                CTIA_p = new CTIA_Chunk(m_maxNumOf_TI_estimated);

                if ((CTIA_p == nullptr) || (CTIA_p->m_TIA_p == nullptr)) {
                    TRACEX_E("ThreadMain_CTIA  out of memory");
                    return;
                }

                m_CTIA_Chunks.append(CTIA_p);

                TIA_p = CTIA_p->m_TIA_p;
                currentItemIndex = 0;
            }

            ++ref_p;
            start_p = ref_p;

            TIA_p[currentItemIndex].fileIndex = MemRef_To_FileIndex(m_fileStartIndex, m_start_p, start_p);
        } else {
            ++ref_p;
        }
    }

    /* Check if the last letter wasn't a return */
    if ((*(ref_p - 1) != 0x0a)) {
        TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p);

        if (*(ref_p - 1) == 0x0d) {
            TIA_p[currentItemIndex].size--; /* if line ends with 0x0a 0x0d (and not only 0x0a) */
        }
        ++currentItemIndex;
    }

    CTIA_p->m_num_TI = currentItemIndex;
    execTime.Trigger();

#ifdef TIME_MEASURE_TIA_PARSING
    TRACEX_DE("Thread 0x%llx done time:%e ms", this, execTime.ms());
#endif
}

/***********************************************************************************************************************
*   Seek_EOL
***********************************************************************************************************************/
static int Seek_EOL(QFile *qFile_p, char *workMem_p, int64_t fileStartIndex, int64_t fileSize,
                    int64_t *offset_EOL_p /*In/out*/)
{
    /* File end is the index from file start */
    int64_t readBytes;
    int64_t to_readBytes;
    int64_t totalIndex;
    unsigned int index;
    int64_t fileOffset;
    bool stop = false;

    /* The purpose of this function is to search for the row end from the file start
     * Avoid reading beond the file end */

    /* 0. Seek file handle
     * 1. Read 4096 bytes
     * 2. Search EOL
     * 3. Return fileStartindex to EOL */
    *offset_EOL_p = 0;

    to_readBytes = (((int64_t)FILECTRL_ROW_MAX_SIZE_persistent) < (fileSize - fileStartIndex) ?
                    FILECTRL_ROW_MAX_SIZE_persistent : (fileSize - fileStartIndex));
    totalIndex = 0;

    while (!stop) {
        fileOffset = fileStartIndex + totalIndex;
        readBytes = 0;

        bool status = qFile_p->reset();
        if (status) {
            status = qFile_p->seek(fileOffset);
        }

        if (status) {
            readBytes = qFile_p->read(&workMem_p[0], to_readBytes);
        } else {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to seek in file, file locked or removed ?", qFile_p);
            return SEEK_EOL_ERROR;
        }

        if (readBytes < 0) {
            TRACEX_QFILE(LOG_LEVEL_ERROR,
                         "Failed to read log file data, Failed to set the file to the beginning, "
                         "file locked or removed ?", qFile_p);
            return SEEK_EOL_ERROR;
        }

        index = 0;

        /* Scan the read bytes for line ending */
        while (index < readBytes) {
            if (workMem_p[index] == 0x0a) {
                if (workMem_p[index + 1] == 0x0d) {
                    /* (unsigned int) offset shouldn't become larger */
                    *offset_EOL_p = (unsigned int)(totalIndex + index + 1);
                } else {
                    *offset_EOL_p = (unsigned int)(totalIndex + index); /* (unsigned int) offset shouldn't become larger
                                                                         * */
                }
                return SEEK_EOL_SUCCESS;
            } else {
                ++index;
            }
        } /*  while(index < readBytes) */

        totalIndex += readBytes;

        /* This function loops in chunks of FILECTRL_ROW_MAX_SIZE_persistent rows. It is possible that a row is much
         * longer than FILECTRL_ROW_MAX_SIZE_persistent, then this function will loop several times.
         *  (FILECTRL_ROW_MAX_SIZE_persistent + totalIndex)
         * is to check that there is still FILECTRL_ROW_MAX_SIZE_persistent letters left to read, otherwise
         * compensation has to be made *//* to_readBytes, defines if there is more text to search EOL into. Hence, if
         * there is more letters
         * left to read then the while loop continues. Otherwise return EOF here */
        to_readBytes = (((int64_t)FILECTRL_ROW_MAX_SIZE_persistent + totalIndex) < (fileSize - fileStartIndex) ?
                        FILECTRL_ROW_MAX_SIZE_persistent : (fileSize - fileStartIndex - totalIndex));

        if (to_readBytes <= 0) {
            return SEEK_EOL_FILE_END;
        }
    } /* while */

    return SEEK_EOL_ERROR;
}

/***********************************************************************************************************************
*   AddThread
***********************************************************************************************************************/
void CParseCmd::AddThread(char *start_p, unsigned int size, int64_t fileStartIndex)
{
    CTIA_Thread *thread_p = new CTIA_Thread;

    if (thread_p == nullptr) {
        TRACEX_E("ParseCmd AddThread Failed, out of mem, start:0x%llx size:%d fileIndex:%lld", start_p,
                 size, fileStartIndex);
        return;
    }

    thread_p->Configure(start_p, size, fileStartIndex);

    m_threadList.append(thread_p);
}

/***********************************************************************************************************************
*   Setup
***********************************************************************************************************************/
void CParseCmd::Setup(CFileCtrl_FileHandle_t *fileHandle_p, int64_t file_StartOffset, int64_t file_TotalSize,
                      char *workMem_p, unsigned int size)
{
    m_fileStart = file_StartOffset;
    m_fileSize = file_TotalSize;
    m_size = size;
    m_fileHandle_p = fileHandle_p;
    m_workMem_p = workMem_p;

    /* Check that size is large enough for creating several threads
     * And that system isn't currently forcing usage of only one thread */

    if ((size > CFG_MINIMUM_FILE_SIZE_FOR_MULTI_THREAD_TI_PARSE) && (g_cfg_p->m_numOfThreads > 1)) {
        /* Create several threads */

        int64_t fileOffset = m_fileStart;
        char *currentStart_p = workMem_p;
        unsigned int sizePerThread;
        int index;
        int64_t file_SeekEnd_StartIndex;
        int64_t EOL_Offset;
        int seekResult;
        bool stop = false;

        sizePerThread = m_size / g_cfg_p->m_numOfThreads;

        /* Setup all threads but the last one */
        for (index = 0; !stop && index < (g_cfg_p->m_numOfThreads - 1); ++index) {
            /* fileStartIndex, estimated fileIndex where the current thread will stop processing chars */
            file_SeekEnd_StartIndex = fileOffset + sizePerThread;

            if (file_SeekEnd_StartIndex < m_fileSize) {
                /* Seek directly in the file where the last line for the threads ends. The offset from fileStartIndex is
                 * stored in EOL_Offset */
                if (!(seekResult = Seek_EOL(fileHandle_p->qFile_p, workMem_p,
                                            file_SeekEnd_StartIndex, m_fileSize, &EOL_Offset))) {
                    if (seekResult == SEEK_EOL_ERROR) {
                        TRACEX_E("CParseCmd::Setup  SeekError");
                    } else if (seekResult == SEEK_EOL_FILE_END) {
                        TRACEX_E("CParseCmd::Setup  SeekError Reached EOL during seek");
                    }

                    return;
                }

                uint32_t threadWorkSize = sizePerThread + EOL_Offset;

                AddThread(currentStart_p, threadWorkSize, fileOffset);

                currentStart_p += threadWorkSize + 1;     /* Working Memory, modify the next start with the overlap */
                fileOffset += threadWorkSize + 1;         /* File offset */
            } else {
                /* Reached the end sooner than expected, much likely because of EOL_Offset drifting way making the
                 * inital caluclation how many letters per thread wrong. Hence, break this for-loop and add the
                 * last thread with the remainder */

                stop = true;
            }
        }

        /* Setup the last thread, with the remainder, not necessary to seek for EOL */
        AddThread(currentStart_p, m_size - (unsigned int)(currentStart_p - workMem_p), fileOffset);
    } else {
        /* Create only one thread */
        AddThread(workMem_p, size, file_StartOffset);
    }
}

/***********************************************************************************************************************
*   Execute
***********************************************************************************************************************/
void CParseCmd::Execute(void)
{
    CTIA_Thread *thread_p;
    int64_t read;
    int64_t seekOffset;
    CTimeMeas execTime;

    if (m_threadList.isEmpty()) {
        /* Error */
        return;
    }

    seekOffset = m_fileStart;

    execTime.Restart();

    QString size = GetTheDoc()->FileSizeToString(m_size);

    g_processingCtrl_p->AddProgressInfo(QString("   Loading log file to memory, %1").arg(size));
    g_processingCtrl_p->SetFileOperationOngoing(true);

    if (!m_fileHandle_p->qFile_p->reset() || !m_fileHandle_p->qFile_p->seek(seekOffset)) {
        if (m_fileHandle_p->qFile_p->error() != QFile::NoError) {
            TRACEX_QFILE(LOG_LEVEL_ERROR,
                         "Failed to read log file data, failed to seek in "
                         "the file, file locked or removed ?", m_fileHandle_p->qFile_p);
            return;
        }
    }

    int64_t totalRead = 0;

    while (totalRead < m_size) {
        read = m_fileHandle_p->qFile_p->read(m_workMem_p, m_size);

        if (read == -1) {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read log file data", m_fileHandle_p->qFile_p);
            return;
        }

        totalRead += read;
    }

    g_processingCtrl_p->SetFileOperationOngoing(false);

    m_fileHandle_p->filePos += totalRead;

    if (m_size != totalRead) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read log file data", m_fileHandle_p->qFile_p);
        return;
    }

    bool isFirstThread = true;

    execTime.Trigger();
    m_fileLoadTime = execTime.ms();
    execTime.Restart();

    g_totalNumOfThreads = m_threadList.count();

    QString time = GetTheDoc()->timeToString(m_fileLoadTime);
    QString loadInfo = QString("   Loading complete, %1").arg(time);

    g_processingCtrl_p->AddProgressInfo(loadInfo);
    g_processingCtrl_p->AddProgressInfo(QString("   Processing, threads:%1").arg((int)g_totalNumOfThreads));

#ifdef DEBUG_TIA_PARSING
    TRACEX_D("Starting ParseCmd:0x%llx threads", this);
#endif

    QList <CTIA_Thread *>::iterator iter;
    for (iter = m_threadList.begin(); iter != m_threadList.end(); ++iter) {
        thread_p = *iter;

        if (isFirstThread) {
            isFirstThread = false;
            thread_p->m_isSteppingProgressCounter = true;
        }

        thread_p->start();
    }

#ifdef DEBUG_TIA_PARSING
    TRACEX_D("ParseCmd:0x%llx Waiting for threads", this);
#endif

    for (iter = m_threadList.begin(); iter != m_threadList.end(); ++iter) {
        (*iter)->wait();
    }

    m_processTime = execTime.ms();

#ifdef DEBUG_TIA_PARSING
    TRACEX_D("All ParseCmd:0x%llx thread DONE time:%e", this, m_processTime);
#endif
}

/***********************************************************************************************************************
*   PrintExecTimes
***********************************************************************************************************************/
void CParseCmd::PrintExecTimes(void)
{
    QString execTimeString = QString("ParseCmd:%1 Load:%2 Exec:%3 ").
                                 arg(QString::number(reinterpret_cast<uint64_t>(this), 16)).arg(m_fileLoadTime).arg(
        m_processTime);
    QList<double>::iterator iter;
    int index;
    for (index = 0, iter = m_threadExecTimeList.begin(); iter != m_threadExecTimeList.end(); ++iter, ++index) {
        execTimeString += QString("t%1:%2").arg(index).arg(*iter);
        ++index;
    }

    execTimeString += QString("");
    TRACEX_D(execTimeString);
}

/***********************************************************************************************************************
*   FileStore
***********************************************************************************************************************/
void CParseCmd::FileStore(QFile& qTIAfile)
{
    CTIA_Thread *thread_p;
    CTIA_Chunk *CTIA_Chunk_p;

    /* Loop through all threads in the CMD
     *    Loop through all CTIA_Chunks per thread
     *       Add all the CTIA chunk to fileStorage */

    if (m_threadList.isEmpty()) {
        return;
    }

    QList <CTIA_Thread *>::iterator iter;

    for (iter = m_threadList.begin(); iter != m_threadList.end(); ++iter) {
        thread_p = *iter;

        QList<CTIA_Chunk *>::iterator chunkIter;

        for (chunkIter = thread_p->m_CTIA_Chunks.begin(); chunkIter != thread_p->m_CTIA_Chunks.end(); ++chunkIter) {
            CTIA_Chunk_p = *chunkIter;
            m_fileStorage.AddChunk(CTIA_Chunk_p);
        }
    }

    m_fileStorage.Store_CTIA(qTIAfile);
}

/***********************************************************************************************************************
*   FileLoad
***********************************************************************************************************************/
void CParseCmd::FileLoad(char *destMem_p, unsigned int *size_p)
{
    /*  m_fileStorage.Load_CTIA(destMem_p, size_p); */
}

/***********************************************************************************************************************
*   GetNumOf_TI
***********************************************************************************************************************/
int CParseCmd::GetNumOf_TI(void)
{
    int numOf_TI = 0;
    CTIA_Thread *thread_p;
    CTIA_Chunk *CTIA_Chunk_p;

    /* Loop through all threads in the CMD
     *    Loop through all CTIA_Chunks per thread
     *       Sum number of TextItems */

    if (m_threadList.isEmpty()) {
        return 0;
    }

    QList <CTIA_Thread *>::iterator iter;

    for (iter = m_threadList.begin(); iter != m_threadList.end(); ++iter) {
        thread_p = *iter;

        QList<CTIA_Chunk *>::iterator chunkIter;

        for (chunkIter = thread_p->m_CTIA_Chunks.begin(); chunkIter != thread_p->m_CTIA_Chunks.end(); ++chunkIter) {
            CTIA_Chunk_p = *chunkIter;
            numOf_TI += CTIA_Chunk_p->m_num_TI;
        }
    }

    return numOf_TI;
}

/***********************************************************************************************************************
*   Search_TIA
***********************************************************************************************************************/
bool CFileCtrl::Search_TIA(QFile *qfile_p, const QString& TIA_fileName, char *work_mem_p,
                           int64_t workMemSize, int *rows_p)
{
    int32_t numOf_CMDs;
    int64_t CMD_size;
    int64_t CMD_size_adjusted;
    int64_t EOL_Offset;
    int64_t fileSize;
    int64_t fileIndex;
    int64_t seekIndex;
    CTimeMeas execTime;
    int32_t index;
    CParseCmd *parseCmd_p = nullptr;

#ifdef TIME_MEASURE_TIA_PARSING
    double fileSetupTime;
    double ParseCmdSetupTime;
    double ParseCmdExecTime;
#endif

    *rows_p = 0;

    if (!qfile_p->reset()) {
        /* Make sure that file point is at the beginning */
        TRACEX_QFILE(LOG_LEVEL_ERROR,
                     "Failed to set the file to the beginning, file locked or removed ?", qfile_p);
        return false;
    }

    fileSize = qfile_p->size();

    PRINT_FILE_TRACKING(QString("Search TIA, file:%1").arg(fileSize));

    g_totalFileSize = fileSize;

    m_LogFile.filePos = 0;
    m_LogFile.qFile_p = qfile_p;

    m_TIA_FileName = TIA_fileName;
    m_TIA_File.setFileName(m_TIA_FileName);

    if (!m_TIA_File.open(QIODevice::ReadWrite)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to open TIA file, file locked?", qfile_p);
        return false;
    }

    /* The TIA file contains a header, this should be passed before writing the TIA data. By making a dummy header
     *  write the file pointer will be positioned correctly.*/
    Write_TIA_Header(true /*empty*/);

    seekIndex = 0;
    fileIndex = 0;

    numOf_CMDs = (int32_t)((fileSize / workMemSize) + 1);

#ifdef TIME_MEASURE_TIA_PARSING
    fileSetupTime = execTime.ms();
#endif
    execTime.Restart();

    if (numOf_CMDs == 1) {
        CMD_size = fileSize;

        parseCmd_p = new CParseCmd();

        if (parseCmd_p == nullptr) {
            TRACEX_E(
                QString("CTIA_FileStorage::Search_TIA  new CParseCmd (1), Out of memory FileName:%1")
                    .arg(m_TIA_FileName));
            return false;
        }

        parseCmd_p->Setup(&m_LogFile, fileIndex, fileSize, work_mem_p, CMD_size);
        m_parseCmdList.append(parseCmd_p);
    } else {
        /* The size could be (2 * workMemSize) - 1, always decompose into one extra chunk to be on safe side
         *  that the workMem is large enough.
         * TODO: Improvement to make better filling of workingMem
         */
        numOf_CMDs += 1; /* make sure we have some extra room in the workMem, +1 for making extra space */
        CMD_size = fileSize / numOf_CMDs;

        g_processingCtrl_p->AddProgressInfo("File segmentation");

        for (index = 0; index < (numOf_CMDs - 1); ++index) {
            /* Seek for a proper end byte for the CMD */
            seekIndex = fileIndex + CMD_size;

            if (Seek_EOL(m_LogFile.qFile_p, work_mem_p, seekIndex, fileSize, &EOL_Offset) == SEEK_EOL_ERROR) {
                TRACEX_E("CTIA_FileStorage::Search_TIA  Failed to find a line ending");
                return false;
            }

            CMD_size_adjusted = CMD_size + EOL_Offset;

            parseCmd_p = new CParseCmd();

            if (parseCmd_p == nullptr) {
                TRACEX_E(
                    QString("CTIA_FileStorage::Search_TIA  new CParseCmd, "
                            "<INDEX.%1> Out of memory FileName:%2")
                        .arg(index).arg(m_TIA_FileName));
                return false;
            }

            parseCmd_p->Setup(&m_LogFile, fileIndex, fileSize, work_mem_p, CMD_size_adjusted);
            m_parseCmdList.append(parseCmd_p);

            fileIndex += CMD_size_adjusted + 1;
        }

        parseCmd_p = new CParseCmd();

        if (parseCmd_p == nullptr) {
            TRACEX_E(
                QString("CTIA_FileStorage::Search_TIA  new CParseCmd,"
                        " <FINAL> Out of memory FileName:%1").
                    arg(m_TIA_FileName));
            return false;
        }

        parseCmd_p->Setup(&m_LogFile, fileIndex, fileSize, work_mem_p, (unsigned int)(fileSize - fileIndex));
        m_parseCmdList.append(parseCmd_p);
    }

#ifdef DEBUG_TIA_PARSING
    TRACEX_D("Number of Parse commands = %d", numOf_CMDs);
#endif
    g_processingCtrl_p->AddProgressInfo(QString("   File will be loaded in %1 segments").arg(numOf_CMDs));

    m_LogFile.filePos = 0;

#ifdef TIME_MEASURE_TIA_PARSING
    ParseCmdSetupTime = execTime.ms();
#endif

    execTime.Restart();

    /* Make sure that file point is at the beginning */
    if (!m_LogFile.qFile_p->reset()) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "CFileCtrl::Search_TIA  Failed to Set file start at Exec ?",
                     m_LogFile.qFile_p);
        return false;
    }

    TRACEX_DISABLE_WINDOW();

    m_numOf_TI = 0;

    g_processingCtrl_p->AddProgressInfo("EOL parsing started");

    /* Load and parse all, create the TIAs */

    QList<CParseCmd *>::iterator iterCmd = m_parseCmdList.begin();

    while (iterCmd != m_parseCmdList.end() && !g_processingCtrl_p->m_abort) {
        parseCmd_p = *iterCmd;
        g_processingCtrl_p->SetProgressCounter((float)parseCmd_p->m_fileStart / (float)fileSize);

        parseCmd_p->Execute();

        m_numOf_TI += parseCmd_p->GetNumOf_TI();

#ifdef DEBUG_TIA_PARSING
        TRACEX_D("CFileCtrl::Search_TIA  TIs %d", parseCmd_p->GetNumOf_TI());
#endif

        parseCmd_p->FileStore(m_TIA_File);
        parseCmd_p->EmptyThreads();

        ++iterCmd;
    }

#ifdef TIME_MEASURE_TIA_PARSING
    ParseCmdExecTime = execTime.ms();
#endif

    if (!g_processingCtrl_p->m_abort) {
        g_processingCtrl_p->AddProgressInfo("  EOL parsing done");
        TRACEX_ENABLE_WINDOW();
        TRACEX_D("CFileCtrl::Search_TIA  Number of TextItems = %d", m_numOf_TI);

        m_TIA_File.flush();
        Write_TIA_Header();
        m_TIA_File.flush();
    } else {
        g_processingCtrl_p->SetFail();
        g_processingCtrl_p->AddProgressInfo("  EOL parsing aborted");
    }

    m_TIA_File.close();

    if (!g_processingCtrl_p->m_abort) {
        g_processingCtrl_p->SetSuccess();
        m_loadTime = execTime.ms();
        *rows_p = m_numOf_TI;
        return true;
    } else {
        g_processingCtrl_p->SetFail();
        *rows_p = 0;
        return false;
    }
}

/***********************************************************************************************************************
*   Search_TIA_Incremental
***********************************************************************************************************************/
bool CFileCtrl::Search_TIA_Incremental(QFile& logFile, const QString& TIA_fileName, char *work_mem_p,
                                       int64_t workMemSize, int64_t startFromIndex, int32_t fromRowIndex,
                                       int *rows_added_p)
{
    int64_t fileSize;
    int64_t readBytes;
    QList<CTIA_Chunk *> CTIA_Chunks;

    /* For some reason it is very important to close and reopen the file
     * to get the latest from file cache */
    if (logFile.isOpen()) {
        logFile.close();
    }
    if (!logFile.open(QIODevice::ReadOnly)) {
        TRACEX_W(QString("Failied to reopen log file at file validation"));
        return false;
    }

    m_LogFile.qFile_p = &logFile;
    m_LogFile.filePos = startFromIndex;
    m_TIA_FileName = TIA_fileName;
    m_TIA_File.setFileName(m_TIA_FileName);

    QFileInfo fileInfo(logFile);
    fileInfo.refresh();
    fileSize = fileInfo.size();

    PRINT_FILE_TRACKING(QString("Incremental: start_index:%1 start_pos:%2")
                            .arg(fromRowIndex).arg(startFromIndex));

    if (!m_TIA_File.open(QIODevice::ReadWrite)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to open TIA file, file locked?", &logFile);
        return false;
    }

    *rows_added_p = 0;
#if 0
    if (logFile.seek(fileSize - 1)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR,
                     " ?", &logFile);
        return false;
    }
#endif

    if (!logFile.seek(startFromIndex)) {
        /* Step forward to the previous end of file */
        TRACEX_QFILE(LOG_LEVEL_ERROR,
                     "Failed to set the file to the beginning, file locked or removed ?", &logFile);
        return false;
    }

    auto incrementalSize = fileSize - startFromIndex;
    if (incrementalSize > workMemSize) {
        TRACEX_QFILE(LOG_LEVEL_INFO,
                     "File increment to large, go for full load of entire file", &logFile);
        return false;
    }
    readBytes = logFile.read(work_mem_p, workMemSize);

    if ((readBytes == 0) || (readBytes < incrementalSize)) {
        PRINT_FILE_TRACKING(QString("Failed to read %1 inc bytes from file, got %2")
                                .arg(incrementalSize).arg(readBytes));
    }

    PRINT_FILE_TRACKING(QString("Incremental size:%1 file:%2").arg(incrementalSize).arg(fileSize));

    unsigned int currentItemIndex = 0;
    char *ref_p;
    char *start_p;
    char *ref_end_p;
    CTIA_Chunk *CTIA_p;
    TI_t *TIA_p;
    const int NUM_TI_IN_CHUNK = 4096;

    CTIA_p = new CTIA_Chunk(NUM_TI_IN_CHUNK);

    if (CTIA_p == nullptr) {
        TRACEX_E("ThreadMain_CTIA  out of memory");
        return false;
    }

    CTIA_Chunks.append(CTIA_p);
    TIA_p = CTIA_p->m_TIA_p;

    ref_p = work_mem_p;
    start_p = ref_p;
    ref_end_p = ref_p + readBytes;

    currentItemIndex = 0;

    /* Initialize the first item */
    TIA_p[0].fileIndex = startFromIndex;

    /* if the first row is a LF (unix system) starting with a newline
     * CR=0x0d, LF=0x0a
     * CR + LF, LF */

    if (*ref_p == 0x0a) {
        TIA_p[0].size = 0;
        ++ref_p;
        start_p = ref_p;

        currentItemIndex = 1;
        TIA_p[1].fileIndex = MemRef_To_FileIndex(startFromIndex, work_mem_p, start_p);
    }

    while (ref_p < ref_end_p) {
        if (*ref_p == 0x0a) {
            if (*(ref_p - 1) == 0x0d) {
                /* double zeroes, do not include the 0 in the count */
                TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p - 1);
            } else {
                /* double zeroes, do not include the 0 in the count */
                TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p);
            }

            /* initiate the next text item */
            ++currentItemIndex;

            if (currentItemIndex >= NUM_TI_IN_CHUNK) {
                CTIA_p->m_num_TI = currentItemIndex;

                CTIA_p = new CTIA_Chunk(NUM_TI_IN_CHUNK);

                if ((CTIA_p == nullptr) || (CTIA_p->m_TIA_p == nullptr)) {
                    TRACEX_E("ThreadMain_CTIA  out of memory");
                    return false;
                }

                CTIA_Chunks.append(CTIA_p);

                TIA_p = CTIA_p->m_TIA_p;
                currentItemIndex = 0;
            }

            ++ref_p;
            start_p = ref_p;

            TIA_p[currentItemIndex].fileIndex = MemRef_To_FileIndex(startFromIndex, work_mem_p, start_p);
        } else {
            ++ref_p;
        }
    }

    /* Check if the last letter wasn't a return */
    if ((*(ref_p - 1) != 0x0a)) {
        TIA_p[currentItemIndex].size = (unsigned int)(ref_p - start_p);

        if (*(ref_p - 1) == 0x0d) {
            TIA_p[currentItemIndex].size--; /* if line ends with 0x0a 0x0d (and not only 0x0a) */
        }
        ++currentItemIndex;
    }
    CTIA_p->m_num_TI = currentItemIndex;

    for (auto& chunk_p : CTIA_Chunks) {
        *rows_added_p += chunk_p->m_num_TI;
    }

    m_numOf_TI = fromRowIndex + *rows_added_p;

    /* Seek to where the new entries in TIA file should be added */
    m_TIA_File.seek(sizeof(TIA_FileHeader_t) + fromRowIndex * sizeof(TI_t));

    for (auto& chunk_p : CTIA_Chunks) {
        int64_t bytesToWrite = chunk_p->m_num_TI * sizeof(TI_t);
        int64_t bytesWritten = 0;

        bytesWritten = m_TIA_File.write(reinterpret_cast<char *>(chunk_p->m_TIA_p), bytesToWrite);

        if (bytesToWrite != bytesWritten) {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to save TIA data to the TIA file, "
                                          "file locked or removed ?", &m_TIA_File);
            return false;
        }
    }
    Write_TIA_Header();
    return true;
}

/***********************************************************************************************************************
*   Write_TIA_Header
***********************************************************************************************************************/
bool CFileCtrl::Write_TIA_Header(bool empty)
{
    TIA_FileHeader_t header;
    QFileInfo fileInfo(*m_LogFile.qFile_p);
    fileInfo.refresh();

    QFileInfo TIAFileInfo(m_TIA_FileName);
    TIAFileInfo.refresh();

    /* If data is being streamed into the file then this time might not match
     * what has been parsed, hence the file stamp is newer compared to the data read.
     * As such the file size should be used instead of the time stamp. */
    QDateTime lastModified(fileInfo.lastModified());

    header.fileTime = lastModified.toMSecsSinceEpoch();
    header.headerSize = sizeof(TIA_FileHeader_t);
    header.fileVersion = TIA_FILE_VERSION;
    header.numOfRows = m_numOf_TI;
    header.fileSize = 0;

    if (!empty) {
        /* First, find out the size of the file log file.
         * Read the last TIA */
        TI_t ti;
        if (!m_TIA_File.seek(sizeof(header) + sizeof(TI_t) * (m_numOf_TI - 1))) {
            TRACEX_E(QString("Failed to seek to last TI in TIA file size:%1 seek:%2")
                         .arg(m_TIA_File.size()).arg(sizeof(header) + sizeof(TI_t) * (m_numOf_TI - 1)));
        }

        auto readBytes = m_TIA_File.read(reinterpret_cast<char *>(&ti), sizeof(TI_t));
        if (readBytes != sizeof(TI_t)) {
            TRACEX_E(QString("Failed to read last entry in TIA file, "
                             "size:%1 seek:%2 toRead:%3")
                         .arg(TIAFileInfo.size())
                         .arg(sizeof(header) + sizeof(TI_t) * (m_numOf_TI - 1))
                         .arg(sizeof(TI_t)));
        }

        header.fileSize = ti.fileIndex + ti.size;

        /* It might be that the last row doesn't include the CR/LF chars in its size, this shall now be checked. */
        m_LogFile.qFile_p->seek(ti.fileIndex + ti.size); /* Put to end of file, check EOL marker */

        char data[2];
        int readSize = m_LogFile.qFile_p->read(data, 2);
        if (readSize == 2) {
            /* Read last + 2 char */
            if (data[1] == 0x0a) {
                header.fileSize++; /* Row ended with 0x0a */
            }
            if (data[0] == 0x0d) {
                header.fileSize++; /* Row ended with 0x0d+0x0a */
            }
        } else if (readSize == 1) {
            /* Read last + 1 char */
            if (data[0] == 0x0a) {
                header.fileSize++; /* Row ended with 0x0a */
            }
        }

        if (header.fileSize != m_LogFile.qFile_p->size()) {
            TRACEX_I("Size of log file has increased size TIA parsing started");
        }
    }

    if (m_TIA_File.seek(0)) {
        /* set file pointer to the beginning of the file */
        int64_t bytesWritten = m_TIA_File.write(reinterpret_cast<char *>(&header), sizeof(header));
        if (bytesWritten == sizeof(header)) {
            m_TIA_File.flush();
            return true;
        } else {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to write TIA file", &m_TIA_File);
        }
    }

    TRACEX_QFILE(LOG_LEVEL_ERROR,
                 "CFileCtrl::Write_TIA_Header  Failed to write TIA header", &m_TIA_File);

    return false;
}

CTIA_FileStorage::~CTIA_FileStorage(void)
{
    m_CTIA_Chunks.clear();
}

/***********************************************************************************************************************
*   Store_CTIA
***********************************************************************************************************************/
bool CTIA_FileStorage::Store_CTIA(QFile& qfile)
{
    m_num_TI = 0;
    m_StorageSize = 0;

    QList<CTIA_Chunk *>::iterator chunkIter = m_CTIA_Chunks.begin();

    while (chunkIter != m_CTIA_Chunks.end()) {
        CTIA_Chunk *chunk_p = *chunkIter;
        int64_t bytesToWrite = chunk_p->m_num_TI * sizeof(TI_t);
        int64_t bytesWritten = 0;

        bytesWritten = qfile.write(reinterpret_cast<char *>(chunk_p->m_TIA_p), bytesToWrite);

        if (bytesToWrite != bytesWritten) {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to save TIA data to the TIA file, "
                                          "file locked or removed ?", &qfile);
            return false;
        }

        m_num_TI += chunk_p->m_num_TI;
        m_StorageSize += bytesWritten;
        ++chunkIter;
    }

    if (TRACEX_IS_ENABLED(LOG_LEVEL_DEBUG) && (m_StorageSize % sizeof(TI_t) != 0)) {
        TRACEX_E("CTIA_FileStorage::Store_CTIA  Total number of SAVE is not equal to "
                 "mod of TI size Size:%d mod:%d", m_StorageSize, sizeof(TI_t));
    }

    qfile.flush();

    return true;
}

/***********************************************************************************************************************
*   TestFileCtrl
***********************************************************************************************************************/
void TestFileCtrl(void)
{
    CFileCtrl fileCtrl;
    char *mem_p = (char *)VirtualMem::Alloc(1024 * 1000);
    QFile logFile("D:\\Projects\\LogScrutinizer\\logs\\long_lines.txt");
    char tia_name[] = ("D:\\Projects\\LogScrutinizer\\logs\\long_lines_tia.txt");
    int rows;

    if (!logFile.open(QIODevice::ReadWrite)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to open test log", &logFile);
    }

    fileCtrl.Search_TIA(&logFile, tia_name, mem_p, 1024 * 1000, &rows);

    VirtualMem::Free(mem_p);
}

bool GenerateSeekLog(const QString& fileName, const QString& repetitionPattern,
                     int totalNumOfRows, int lineEndingCount);

/***********************************************************************************************************************
*   TestSeek
***********************************************************************************************************************/
void TestSeek()
{
    int numOfRows = 1000;
    QString fileName = "seektest.txt";
    QString repetitionPattern = "Test Test Test Test Test"; /* 24 letters, 26 chars including \n\r */
    int lineEndingCount = 2; /* \n\r */
    char workMem_p[4096 * 10];

    if (!GenerateSeekLog(fileName, repetitionPattern, numOfRows, lineEndingCount)) {
        TRACEX_E("GenerateSeekLog Failed");
        return;
    }

    int64_t offset_EOL;
    QFile LogFile(fileName);

    if (!LogFile.open(QIODevice::ReadWrite)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "TestSeek Failed - Log couldn't be open", &LogFile);
        return;
    }

    int64_t fileSize = LogFile.size();

    if (fileSize != (repetitionPattern.size() + lineEndingCount) * numOfRows) {
        TRACEX_E("TestSeek Failed - Generated Log wrong size");
        return;
    }

    const uint32_t repPatLength = repetitionPattern.size();
    const uint32_t lineLength = repPatLength + lineEndingCount;

    for (int row = 0; row < numOfRows; ++row) {
        int64_t fileStartIndex = row * lineLength + row % lineLength; /* continously shift start position */

        if (Seek_EOL(&LogFile, workMem_p, fileStartIndex, fileSize, &offset_EOL /*In/out*/) == SEEK_EOL_ERROR) {
            TRACEX_E("TestSeek Failed - Seek_EOL failed");
            return;
        }

        if (((fileStartIndex + offset_EOL) != (row + 1) * lineLength - 1)) {
            TRACEX_E("TestSeek Failed - Seek_EOL found wrong line ending");
            return;
        }
    }
}

/***********************************************************************************************************************
*   GenerateSeekLog
***********************************************************************************************************************/
bool GenerateSeekLog(const QString& fileName, const QString& repetitionPattern, int totalNumOfRows, int lineEndingCount)
{
    QFile LogFile(fileName);

    if (LogFile.exists()) {
        if (!LogFile.remove()) {
            TRACEX_QFILE(LOG_LEVEL_ERROR, "GenerateSeekLog Failed - Log couldn't be removed", &LogFile);
            return false;
        }
    }

    if (!LogFile.open(QIODevice::ReadWrite)) {
        TRACEX_QFILE(LOG_LEVEL_ERROR, "GenerateSeekLog Failed - Log couldn't be open", &LogFile);
        return false;
    }

    QTextStream outStream(&LogFile);

    try {
        for (int index = 0; index < totalNumOfRows; ++index) {
            if (lineEndingCount == 2) {
                outStream << repetitionPattern << "\r\n";
            } else {
                outStream << repetitionPattern << "\n";
            }
        } /* for */
    } catch (std::exception &e) {
        TRACEX_E(QString("GenerateTestLog Failed - ASSERT %1 ").arg(e.what()));
        qFatal("  ");
    } catch (...) {
        TRACEX_E("GenerateTestLog Failed - ASSERT");
        qFatal("  ");
    }

    LogFile.close();

    return true;
}
