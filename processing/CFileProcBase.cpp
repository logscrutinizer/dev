/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CFileProcBase.h"
#include "CTimeMeas.h"
#include "CDebug.h"
#include "CConfig.h"
#include "CProgressCtrl.h"
#include "CRowCache.h"
#include <hs/hs.h>
#include "utils.h"
#include "CLogScrutinizerDoc.h"

/***********************************************************************************************************************
*   thread_Match_CS
***********************************************************************************************************************/
bool thread_Match_CS(Match_Description_t *desc_p)
{
    int textChIndex; /* Index to the start/next letter in the current string */
    const int textLength = desc_p->textLength;
    int textChLoopIndex;
    const char *filterStart_p = desc_p->filter_p;
    int filterLength = desc_p->filterLength;
    int filterChIndex;
    bool loop;

    /*early stop */
    for (textChIndex = 0; textChIndex < textLength && (filterLength <= (textLength - textChIndex)); ++textChIndex) {
        const char *loopText_p; /* Direct reference to the character in the loop evaluation */
        const char *loopFilter_p;

        loopText_p = &desc_p->text_p[textChIndex]; /*tempText_p is initilized with start or next letter */
        loopFilter_p = filterStart_p; /*tempFilter_p is initialized with the beginning of the filter */

        filterChIndex = 0;
        textChLoopIndex = textChIndex;
        loop = true;

        /* Loop evaluation, from a letter in the text string see if the filter matches... loop the filter and the text
         *  together See if the filter fits well from the current char and on. */
        while ((*loopFilter_p == *loopText_p) && loop) {
            /* If we have a match for all the letters in the filter then it was success */
            if (filterChIndex == filterLength) {
                return true;
            }

            ++filterChIndex;

            if (textChLoopIndex == textLength) {
                loop = false;
            }

            ++textChLoopIndex;

            ++loopFilter_p;
            ++loopText_p;
        }
    }

    return false;
}

/***********************************************************************************************************************
*   thread_Match
***********************************************************************************************************************/
bool thread_Match(Match_Description_t *desc_p)
{
    int textChIndex; /* Index to the start/next letter in the current string */
    const int textLength = desc_p->textLength;
    int textChLoopIndex;
    const char *filterStart_p = desc_p->filter_p;
    int filterLength = desc_p->filterLength;
    int filterChIndex;

    Q_ASSERT(g_upperChar_LUT_init == true);

    bool loop;

    /*early stop */
    for (textChIndex = 0; textChIndex < textLength && (filterLength <= (textLength - textChIndex)); ++textChIndex) {
        const unsigned char *loopText_p; /* Direct reference to the character in the loop evaluation */
        const unsigned char *loopFilter_p;

        /*tempText_p is initilized with start or next letter */
        loopText_p = reinterpret_cast<const unsigned char *>(&desc_p->text_p[textChIndex]);

        /*tempFilter_p is initialized with the beginning of the filter */
        loopFilter_p = reinterpret_cast<const unsigned char *>(filterStart_p);

        filterChIndex = 0;
        textChLoopIndex = textChIndex;
        loop = true;

        /* Loop evaluation, from a letter in the text string see if the filter matches... loop the filter and the text
         * together See if the filter fits well from the current char and on */
        while ((g_upperChar_LUT[*loopFilter_p] == g_upperChar_LUT[*loopText_p]) && loop) {
            /* If we have a match for all the letters in the filter then it was success */
            if (filterChIndex == filterLength) {
                return true;
            }

            ++filterChIndex;

            if (textChLoopIndex == textLength) {
                loop = false;
            }

            ++textChLoopIndex;

            ++loopFilter_p;
            ++loopText_p;
        }
    }

    return false;
}

/***********************************************************************************************************************
*   eventHandler
***********************************************************************************************************************/
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    auto descr_p = (reinterpret_cast<Match_Description_t *>(ctx));
    descr_p->match = true;
    return 0;
}

/***********************************************************************************************************************
*   thread_Match_RegExp_HyperScan
***********************************************************************************************************************/
bool thread_Match_RegExp_HyperScan(Match_Description_t *desc_p)
{
    if ((desc_p->regexp_database == nullptr) || (desc_p->regexp_scratch == nullptr)) {
        return false;
    }

    desc_p->match = false;

    if (hs_scan(desc_p->regexp_database, desc_p->text_p, desc_p->textLength,
                0 /*flags*/, desc_p->regexp_scratch,
                eventHandler, desc_p) != HS_SUCCESS) {
        return false;
    }

    return desc_p->match;
}

/***********************************************************************************************************************
*   Start
***********************************************************************************************************************/
void CFileProcBase::Start(QFile *qFile_p, char *workMem_p, unsigned int workMemSize, TIA_t *TIA_p, int priority,
                          unsigned int startRow, unsigned int endRow, bool backward)
{
    m_qfile_p = qFile_p;
    m_workMem_p = workMem_p;
    m_workMemSize = workMemSize;

    m_TIA_p = TIA_p;
    m_priority = priority;

    m_startRow = startRow;
    m_endRow = endRow;
    m_backward = backward;

    m_totalNumOfRows = m_startRow < m_endRow ? m_endRow - m_startRow + 1 : m_startRow - m_endRow + 1;

    /*Check some input parameters */

    if (workMem_p == nullptr) {
        TRACEX_E(" CFileProcBase::Start  BAD INPUT   workMem_p nullptr");
        return;
    }

    if (TIA_p == nullptr) {
        TRACEX_E(" CFileProcBase::Start  BAD INPUT   TIA_p nullptr");
        return;
    }

    /* (unsigned int) rows will never be neg. */
    if (!backward && (((unsigned int)TIA_p->rows <= endRow) || (startRow > endRow))) {
        TRACEX_E(" CFileProcBase::Start BAD INPUT  parameters workMem_p:0x%llx TIA_rows:%d "
                 "startRow:%d endRow:%d backward:%d", workMem_p, TIA_p->rows, startRow, endRow, backward);
        return;
    }

    /* (unsigned int) rows will never be neg */
    if (backward && (((unsigned int)TIA_p->rows <= startRow) || (endRow > startRow))) {
        TRACEX_E(" CFileProcBase::Start BAD INPUT  BACKWARD parameters workMem_p:0x%llx TIA_rows:%d "
                 "startRow:%d endRow:%d backward:%d", workMem_p, TIA_p->rows, startRow, endRow, backward);
        return;
    }

    for (int index = 0; index < g_cfg_p->m_numOfThreads; ++index) {
        /* This will create the derived configuration objects */
        m_configurationPoolList.append(CreateConfigurationObject());
    }

    m_fileSize = m_qfile_p->size();

    TRACEX_DISABLE_WINDOW();

    Process();

    TRACEX_ENABLE_WINDOW();

    m_execTime = m_timeExec.ms();
}

/***********************************************************************************************************************
*   ConfigureThread
***********************************************************************************************************************/
bool CFileProcBase::ConfigureThread(CThreadConfiguration *config_p, Chunk_Description_t *chunkDescription_p,
                                    int32_t threadIndex)
{
    /* ConfigureThread is actually configuration of a configuration object that will be picked from the
     * configuration list. But it is configuration for runnign a thread... sort of...
     * Default, typical for having threads searching at different areas in the chunk */

    uint32_t start_TIA_index;
    uint32_t num_Of_TI;
    uint32_t stop_TIA_Index;
    uint32_t TIA_step;

    /* Set to -1 to identify that this configuration hasn't been used yet. The thread will set its index when
     * picking it from the list */
    config_p->m_servedBy_threadIndex = -1;

    if (m_threadTI_Split) {
        start_TIA_index = m_chunkDescr.TIA_startRow + threadIndex * m_linesPerThread;

        if (threadIndex == (m_numberOfChunkThreads - 1)) {
            num_Of_TI = m_linesPerThread + m_linesExtra;
        } else {
            num_Of_TI = m_linesPerThread;
        }
    } else {
        start_TIA_index = m_chunkDescr.TIA_startRow;  /* Each thread use all TIs, starting from first TI */
        num_Of_TI = m_linesPerThread;
    }

    TIA_step = 1;
    stop_TIA_Index = start_TIA_index + num_Of_TI;

    config_p->DeltaInit(chunkDescription_p, num_Of_TI, start_TIA_index, stop_TIA_Index, TIA_step);

    return true;
}

/***********************************************************************************************************************
*   LoadNextChunk
***********************************************************************************************************************/
bool CFileProcBase::LoadNextChunk(void)
{
    int64_t bytesLeft;
    int64_t workMem_Max;
    int64_t maxEndFileIndex;
    bool stop = false;
    int64_t totalRead = 0;
    CTimeMeas execTime;

    if (!m_backward) {
        /* LOAD CHUNK FORWARDs */

        m_chunkDescr.TIA_startRow = m_chunkDescr.TIA_startRow + m_chunkDescr.numOfRows;

        if (m_chunkDescr.TIA_startRow > m_endRow) {
            /*(unsigned int) startRow will never be neg */
            return false;
        }

        m_chunkDescr.fileIndex = m_TIA_p->textItemArray_p[m_chunkDescr.TIA_startRow].fileIndex;
        bytesLeft = m_fileEndIndex - m_chunkDescr.fileIndex;

        if (bytesLeft <= 0) {
            return false;
        }

        workMem_Max = (bytesLeft > m_workMemSize ? m_workMemSize - 1 : bytesLeft);    /* -1, some headroom */
        maxEndFileIndex = m_chunkDescr.fileIndex + workMem_Max;

        /* Locate the first TIA index where the line starts outside the maxEndFileIndex. Loop through all the lines.
         * This means that the current line starts outside  of max data, then the previous line would end outside as
         * well. As such stop at TIA_index - 2.
         * TODO: Do this loop quicker... loop to middle, loop to middle of next side, etc. *//* (unsigned int) startRow
         * will never be neg */
        for (int index = m_chunkDescr.TIA_startRow; index < m_endRow && !stop; ++index) {
            if (m_TIA_p->textItemArray_p[index].fileIndex > maxEndFileIndex) {
                /* This TI starts outside workMem, then the previous ended outside, pick from index - 2 */
                m_chunkDescr.numOfRows = (index - m_chunkDescr.TIA_startRow) - 2;
                stop = true;
            }
        }

        if (!stop) {
            /*Reached end of TIA */
            m_chunkDescr.numOfRows = m_endRow - m_chunkDescr.TIA_startRow + 1; /* +1 since endRow is last VALID index */
        } else if (m_chunkDescr.numOfRows <= 0) {
            return false;
        }

        TRACEX_D("CFileProcBase::LoadNextChunk - StartRow:%d FileIndex:%lld Rows:%d workMem_Max:%lld Last:%d",
                 m_chunkDescr.TIA_startRow,
                 m_chunkDescr.fileIndex,
                 m_chunkDescr.numOfRows,
                 workMem_Max,
                 !stop);

        m_chunkDescr.temp_offset = m_chunkDescr.fileIndex;

        int TIA_LastIndex = m_chunkDescr.TIA_startRow + m_chunkDescr.numOfRows - 1;
        int64_t read = 0;
        int64_t toRead = m_TIA_p->textItemArray_p[TIA_LastIndex].fileIndex
                         + (int64_t)m_TIA_p->textItemArray_p[TIA_LastIndex].size
                         - m_chunkDescr.fileIndex;
        QString size = GetTheDoc()->FileSizeToString(toRead);
        g_processingCtrl_p->AddProgressInfo(QString("  Loading log file to memory, %1").arg(size));
        g_processingCtrl_p->SetFileOperationOngoing(true);

        bool isAllRead = false;
        char *tempWorkMem_p = m_workMem_p;

        while (!isAllRead) {
            m_qfile_p->reset();
            m_qfile_p->seek(m_chunkDescr.temp_offset);

            read = m_qfile_p->read(tempWorkMem_p, toRead);

            if (read < 0) {
                TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read log file data, file locked or removed??", m_qfile_p);
                g_processingCtrl_p->SetFileOperationOngoing(false);
                m_qfile_p->close();
                return false;
            }

            toRead -= read;
            totalRead += read;

            if (toRead == 0) {
                isAllRead = true;
            } else {
                /* For some strange reason not all of the file data was read...
                 * Advance the file load destination pointer */
                tempWorkMem_p += read;

                /* Advance the file offset */
                m_chunkDescr.temp_offset += read;
                TRACEX_D("CFileProcBase::LoadNextChunk, additional read of %d required ?? Error??", toRead);
            }
        }
    } else {
        /* LOAD CHUNK BACKWARDs */

        /* Even if the processing will start at the end of the memory, the file loading is still "forward". This means
         * that we need to figure out how much we may load by looking at the last row (where the processing starts
         * from), and going backwards seeing how many rows that will fit. Then, when the number of rows to load is
         * figured out, the row with lowest TIA index will be located first in the memory aligned to the start of
         * the working memory.
         *
         *  TIA_startRow shall point to the row where this chunk shall start read from, from end of file to beginning.
         *  TIA_startRow contains the value where the previous chunk load started, unless it is the first time */
        if (m_chunkDescr.TIA_startRow - m_chunkDescr.numOfRows <= m_endRow) {
            /* Reached the end */
            return false;
        }

        /* If previous chunk read started at index 20, and read 10 lines, then we shall start at index 10. As previous
         *  read lines with index 20 to 11. */

        /* TIA_startRow and numOfRows are from previous chunk load. Lets first step back over what was loaded
         *  the previous round */
        m_chunkDescr.TIA_startRow -= m_chunkDescr.numOfRows;

        bytesLeft = (m_TIA_p->textItemArray_p[m_chunkDescr.TIA_startRow].fileIndex
                     + (int64_t)m_TIA_p->textItemArray_p[m_chunkDescr.TIA_startRow].size)
                    - m_fileEndIndex;

        if (bytesLeft <= 0) {
            return false;
        }

        workMem_Max = (bytesLeft > m_workMemSize ? m_workMemSize - 1 : bytesLeft);    /* -1, some headroom */
        /* maxEndFileIndex is the top most FILE index we may use consider the work memory and whats left */
        maxEndFileIndex = m_fileEndIndex + bytesLeft - workMem_Max;

        /* Set topMostIndex to start at the highest index, there will then be a search to find index which gives
         * file index within scope */
        int topMostIndex = m_chunkDescr.TIA_startRow;

        /* TODO: Do this loop quicker... loop to middle, loop to middle of next side, etc.
         * Search for the first TIA index where the line starts outside the maxEndFileIndex. Loop through all the lines */
        while (!stop) {
            --topMostIndex; /* decrease index while not found/stopped */

            /* A GOOD "End"/TopMost index has been found */
            if ((topMostIndex >= 0) && (m_TIA_p->textItemArray_p[topMostIndex].fileIndex <= maxEndFileIndex)) {
                /* if fileIndex is spot on we do not need to move index one step */
                if (m_TIA_p->textItemArray_p[topMostIndex].fileIndex < maxEndFileIndex) {
                    ++topMostIndex; /* Step back down finding next higher FILE index */
                }

                /* Index now points at the last row index where we should start
                 * TIA_startRow index the last previous row */

                /* If TIA_startRow=20, topMostIndex=16, then we shall do file read from row index 16 to 20.
                 * Thats 20 - 16 + 1 rows. */
                m_chunkDescr.numOfRows = m_chunkDescr.TIA_startRow - topMostIndex + 1;
                stop = true;
            } else if (topMostIndex < 0) {
                TRACEX_E("Internal Error when searching for next chunk start in upward search");
                g_processingCtrl_p->SetFileOperationOngoing(false);
                m_qfile_p->close();
                return false;
            }
        }

        TRACEX_I("CFileProcBase::LoadNextChunk Backward - StartRow:%d FileIndex:%lld Rows:%d workMem_Max:%lld Last:%d",
                 topMostIndex,
                 m_chunkDescr.fileIndex,
                 m_chunkDescr.numOfRows,
                 workMem_Max,
                 !stop);

        /* fileIndex is where the this chunk shall be start read from */
        m_chunkDescr.fileIndex = m_TIA_p->textItemArray_p[topMostIndex].fileIndex;
        m_chunkDescr.temp_offset = m_chunkDescr.fileIndex;

        int64_t read = 0;
        int64_t toRead = m_TIA_p->textItemArray_p[m_chunkDescr.TIA_startRow].fileIndex
                         + m_TIA_p->textItemArray_p[m_chunkDescr.TIA_startRow].size
                         - m_chunkDescr.fileIndex;
        QString size = GetTheDoc()->FileSizeToString(toRead);
        g_processingCtrl_p->AddProgressInfo(QString("  Loading log file to memory, %1").arg(size));
        g_processingCtrl_p->SetFileOperationOngoing(true);

        bool isAllRead = false;
        char *tempWorkMem_p = m_workMem_p;

        while (!isAllRead) {
            if (!m_qfile_p->seek(m_chunkDescr.temp_offset)) {
                TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read log file data, file locked or removed?", m_qfile_p);
                g_processingCtrl_p->SetFileOperationOngoing(false);
                m_qfile_p->close();
                return false;
            }

            read = m_qfile_p->read(tempWorkMem_p, toRead);

            if (read < 0) {
                TRACEX_QFILE(LOG_LEVEL_ERROR, "Failed to read log file data, file locked or removed?", m_qfile_p);
                g_processingCtrl_p->SetFileOperationOngoing(false);
                m_qfile_p->close();
                return false;
            }

            toRead -= read;
            totalRead += read;

            if (toRead == 0) {
                isAllRead = true;
            } else {
                /* For some strange reason not all of the file data was read...
                 * Advance the file load destination pointer */
                tempWorkMem_p += read;

                /* Advance the file offset */
                m_chunkDescr.temp_offset += read;
                TRACEX_D("CFileProcBase::LoadNextChunk, additional read of %d required ??", toRead);
            }
        }
    }

    QString time = GetTheDoc()->timeToString(execTime.ms());
    g_processingCtrl_p->AddProgressInfo(QString("  Loading complete, %1").arg(time));
    g_processingCtrl_p->SetFileOperationOngoing(false);
    TRACEX_D("CFileProcBase::LoadNextChunk, Read:%d", totalRead);

    return true;
}

/***********************************************************************************************************************
*   Process
***********************************************************************************************************************/
void CFileProcBase::Process(void)
{
    int32_t threadIndex;
    bool continueProcessing = true;

    m_readySem_p = new QSemaphore(g_cfg_p->m_numOfThreads);
    m_holdupSem_p = new QSemaphore(g_cfg_p->m_numOfThreads);

    /* The threads pulls configurations from the list of configuraions */

    /* Initialize threads, start them, and let them wait for first cmd */
    for (threadIndex = 0; threadIndex < g_cfg_p->m_numOfThreads; ++threadIndex) {
        m_startSem_pp[threadIndex] = new QSemaphore(1);
        m_startSem_pp[threadIndex]->acquire(1);

        m_threadInstances[threadIndex] = CreateProcThread(threadIndex, m_readySem_p, m_startSem_pp[threadIndex],
                                                          m_holdupSem_p, &m_configurationListMutex,
                                                          &m_configurationList, &m_configurationPoolList); /* QT_THREAD */
        m_threadInstances[threadIndex]->start();
    }

    QList<CThreadConfiguration *>::Iterator configIter;
    for (configIter = m_configurationPoolList.begin(); configIter != m_configurationPoolList.end(); ++configIter) {
        (*configIter)->BasicInit(m_workMem_p, m_TIA_p);
    }

    /* m_startRow and numOfRows shall contain the values from the previous chunk load. These values are calculated when
     * doing the chunk load */
    m_chunkDescr.TIA_startRow = m_startRow;
    m_chunkDescr.numOfRows = 0;
    m_chunkDescr.first = true;

    /* Setup the start and end file index */

    if (m_backward) {
        m_fileEndIndex = m_TIA_p->textItemArray_p[m_endRow].fileIndex;
    } else {
        m_fileEndIndex = m_TIA_p->textItemArray_p[m_endRow].fileIndex + m_TIA_p->textItemArray_p[m_endRow].size;
    }

    /* Processing Loop */

    while (continueProcessing && LoadNextChunk()) {
        int allocatedLines = 0;
        m_chunkDescr.first = false;

        /* Only use multiple threads if it is at least CFG_MINIMUM_NUM_OF_TIs_FOR_MULTI_THREAD_FILERING rows
         * loaded in the workMem */

        if (m_threadTI_Split) {
            m_numberOfChunkThreads = m_chunkDescr.numOfRows > CFG_MINIMUM_NUM_OF_TIs_FOR_MULTI_THREAD_FILERING ?
                                     g_cfg_p->m_numOfThreads : 1;
            m_linesPerThread = m_chunkDescr.numOfRows / m_numberOfChunkThreads;
            m_linesExtra = m_chunkDescr.numOfRows - (m_linesPerThread * m_numberOfChunkThreads);
        } else {
            m_numberOfChunkThreads = g_cfg_p->m_numOfThreads;   /* g_cfg_p->m_numOfThreads must be overriden */
            m_linesPerThread = m_chunkDescr.numOfRows;
            m_linesExtra = 0;
        }

        /* If not all created holdUp sems is used, either because its little processing, or because in the end
         * of processing, ensure that the holdupSems not used is required... no one can slip through. */
        if (m_numberOfChunkThreads < g_cfg_p->m_numOfThreads) {
            m_holdupSem_p->acquire(g_cfg_p->m_numOfThreads - m_numberOfChunkThreads);
        }

        /* Setup the amount of configuration objects that shall be used
         * Move from pool to configlist, or take out from configlist into pool
         * The specific amount of m_numberOfChunkThreads threads will the pull their items out of their list
         * before starting their execution */
        if (m_configurationList.count() != m_numberOfChunkThreads) {
            if (m_configurationList.count() > m_numberOfChunkThreads) {
                /* Move configuration items back to pool */
                while (m_configurationList.count() != m_numberOfChunkThreads) {
                    m_configurationPoolList.append(m_configurationList.takeLast());
                }
            } else {
                /* Move configuration from pool */
                while (m_configurationList.count() != m_numberOfChunkThreads) {
                    m_configurationList.append(m_configurationPoolList.takeLast());
                }
            }
        }

        int threadIndex = 0;
        for (auto& config : m_configurationList) {
            /* Function to override by sub-class to add extra configuration parameters to the thread */
            if (!ConfigureThread(config, &m_chunkDescr, threadIndex++)) {
                continueProcessing = false;
                g_processingCtrl_p->m_abort = true;
                g_processingCtrl_p->AddProgressInfo(QString("Failed to setup processing"));
                break;
            }
            allocatedLines += config->GetNumOf_TI();
        }

        /* Enable override of chunk parameters */
        ConfigureChunkProcessing();

        if (m_threadTI_Split) {
            if (allocatedLines != m_chunkDescr.numOfRows) {
                TRACEX_E("CFileProcBase::Process - All rows in chunk was not allocated "
                         "to the threads total:%d allocated:%d", m_chunkDescr.numOfRows, allocatedLines);
            }
        } else {
            if (allocatedLines != m_chunkDescr.numOfRows * m_numberOfChunkThreads) {
                TRACEX_E("CFileProcBase::Process - All rows in chunk was not allocated for "
                         "each thread,  rows:%d allocated:%d expected:%d", m_chunkDescr.numOfRows, allocatedLines,
                         m_chunkDescr.numOfRows * m_numberOfChunkThreads);
            }
        }

        float currentStep;

        if (m_threadTI_Split) {
            currentStep = (float)(PROGRESS_COUNTER_STEP * m_numberOfChunkThreads) / (float)(m_totalNumOfRows);
        } else {
            currentStep = (float)(PROGRESS_COUNTER_STEP) / (float)(m_totalNumOfRows);
        }

        g_processingCtrl_p->SetupProgessCounter(currentStep);

        if (!m_backward) {
            g_processingCtrl_p->SetProgressCounter(
                (float)(m_chunkDescr.TIA_startRow - m_startRow) / (float)m_totalNumOfRows);
        } else {
            g_processingCtrl_p->SetProgressCounter(
                (float)(m_startRow - m_chunkDescr.TIA_startRow) / (float)m_totalNumOfRows);
        }
        g_processingCtrl_p->AddProgressInfo(QString("Processing, threads:%1").arg(m_numberOfChunkThreads));

        PRINT_PROGRESS_DBG("Waiting for %d threads to get ready, %d configs for processing",
                           m_numberOfChunkThreads, m_configurationList.count());

        /* Wait for all threads to be waiting for the start sem, they have grabbed their ready sem, and will not
         * release it until done */
        while (m_readySem_p->available() > 0) {
            Sleeper::usleep(10);
        }

        PRINT_PROGRESS_DBG("All threads ready");

        /* We only process the abort handling when we know that all threads has taken their ready sem and is
         * waiting for the start signal */

        if (g_processingCtrl_p->m_abort) {
            continueProcessing = false;
            PRINT_PROGRESS_DBG("Processing aborted");
        } else {
            /* Reload the holdup SEM, make sure that all threads has entered STARTING */
            m_holdupSem_p->acquire(m_numberOfChunkThreads);
            PRINT_PROGRESS_DBG("Holdup sems taken");

            /* Threads are in STARTING
             * Start/Release the threads */
            for (int index = 0; index < m_numberOfChunkThreads; ++index) {
                m_startSem_pp[index]->release(1);
                PRINT_PROGRESS_DBG("Starting %d, release of startSem", index);
            }

            PRINT_PROGRESS_DBG("Start waiting for %d threads", m_numberOfChunkThreads);

            /* Threads are processing, PROC is waiting for them to get info HOLD-UP (releasing their Ready sem)
             * Wait for all threads to release their ready SEM, which they took before started. */
            m_readySem_p->acquire(m_numberOfChunkThreads);
            PRINT_PROGRESS_DBG("All threads ready with processing");

            PRINT_PROGRESS_DBG("Releasing ready sems");
            m_readySem_p->release(m_numberOfChunkThreads);  /* "Reload" the ready Sem such that the threads can take
                                                             * them */

            for (int index = 0; index < m_numberOfChunkThreads; ++index) {
                /* "Reload" the start Sems */
                m_startSem_pp[index]->acquire(1);
                PRINT_PROGRESS_DBG("Took back startSem  %d", index);
            }

            PRINT_PROGRESS_DBG("Releasing all holdupSems");
            m_holdupSem_p->release(m_numberOfChunkThreads); /* Let the threads continue and acquire their ready sems */

            if (continueProcessing) {
                /* Possible to get to early stop for specific thread events, such as search */
                continueProcessing = isProcessingDone() ? false : true;
            }

            if (!m_configurationList.empty()) {
                /* Safety, make sure that all configurtion objects has been processed */
                TRACEX_E("Internal error, the thread sequenced processing has failed. Job aborted");
                continueProcessing = false;
                g_processingCtrl_p->m_abort = false;
                return;
            }

            for (auto& config : m_configurationPoolList) {
                config->Clean();
            }
        }
    } /* while continueProcessing */

    PRINT_PROGRESS("Finishing stopping threads and release start threads");
    for (threadIndex = 0; threadIndex < g_cfg_p->m_numOfThreads; ++threadIndex) {
        m_threadInstances[threadIndex]->Stop();
        m_startSem_pp[threadIndex]->release(1);
    }

    /* Wait for all threads to complete */

    PRINT_PROGRESS("Wait for all threads to exit");
    for (threadIndex = 0; threadIndex < g_cfg_p->m_numOfThreads; ++threadIndex) {
        m_threadInstances[threadIndex]->wait();
    }

    /* Typically a sub-class fetching out results from the threads */
    WrapUp();
}

/***********************************************************************************************************************
*   isProcessingDone
***********************************************************************************************************************/
bool CFileProcBase::isProcessingDone(void)
{
    return false; /* continue */
}

/***********************************************************************************************************************
*   Cancel
***********************************************************************************************************************/
void CFileProcBase::Cancel(void)
{}
