/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CConfig.h"
#include "CFilter.h"
#include "CTimeMeas.h"
#include "CMemPool.h"

#include <stdint.h>

#include <QThread>

/* Each time the decidated thread has run through 10000 lines the progress counter is stepped */
#define PROGRESS_COUNTER_STEP (10000)

typedef struct {
    int64_t filePos;
    QFile *qFile_p;
} CFileCtrl_FileHandle_t;

#define   SEEK_EOL_SUCCESS      1
#define   SEEK_EOL_ERROR       -1
#define   SEEK_EOL_FILE_END    -2

/***********************************************************************************************************************
*   CTIA_Chunk
***********************************************************************************************************************/
class CTIA_Chunk
{
public:
    CTIA_Chunk(void) : m_num_TI(0) {}
    CTIA_Chunk(int64_t maxNumOf_TI) : m_num_TI(0) {
        m_TIA_p = reinterpret_cast<TI_t *>(VirtualMem::Alloc(static_cast<int64_t>(sizeof(TI_t)) * maxNumOf_TI));
    }

    ~CTIA_Chunk(void)
    {
        if (m_TIA_p != nullptr) {
            if (VirtualMem::Free(m_TIA_p)) {
                m_TIA_p = nullptr;
            }
        }
    }

public:
    TI_t *m_TIA_p;
    int m_num_TI;
};

/***********************************************************************************************************************
*   CTIA_FileStorage
***********************************************************************************************************************/
class CTIA_FileStorage
{
public:
    CTIA_FileStorage(void) {m_num_TI = 0; m_StorageSize = 0;}
    ~CTIA_FileStorage(void);

    bool Store_CTIA(QFile& qfile);
    void AddChunk(CTIA_Chunk *chunk_p) {m_CTIA_Chunks.append(chunk_p);}

private:
    int64_t m_num_TI; /* Total number of stored TIs in file */
    int64_t m_StorageSize; /* The size of the external file */
    QList<CTIA_Chunk *> m_CTIA_Chunks;
};

/***********************************************************************************************************************
*   CTIA_Thread
***********************************************************************************************************************/
class CTIA_Thread : public QThread
{
public:
    CTIA_Thread(void) : m_start_p(nullptr), m_size(0), m_isSteppingProgressCounter(0), m_maxNumOf_TI_estimated(0)
    {}

    virtual ~CTIA_Thread(void) override
    {
        while (!m_CTIA_Chunks.isEmpty()) {
            delete (m_CTIA_Chunks.takeFirst());
        }
    }

    void run() override; /* for override of QThread */
    uint32_t ThreadMain_CTIA(void);

    /****/
    void Configure(char *start_p, int64_t size, int64_t fileStartIndex) {
        m_start_p = start_p;
        m_size = size;
        m_fileStartIndex = fileStartIndex;
    }

public:
    QList<CTIA_Chunk *> m_CTIA_Chunks; /* List of chunks, possible more than one element if estimated line size was too
                                        * large */
    char *m_start_p; /* Reference to where in the WORK MEM the thread shall start to parse */
    int64_t m_size;  /* number of bytes to parse */
    int64_t m_fileStartIndex; /* Which index in the file the current thread starts, used in combination with m_start_p
                               * (MEM_REF) */
    double m_execTime;
    bool m_isSteppingProgressCounter; /* Indicate if this thread should step the progress counter */
    int64_t m_maxNumOf_TI_estimated; /* Estimate the number of TIs to create in each CTIA_Chunk */
};

/***********************************************************************************************************************
*   CParseCmd
*
* The parse command is used to instruct the file parser how to parse a chunk of working memory
* It contains the config to load the file, and a set of threads pre-configured where to start
***********************************************************************************************************************/
class CParseCmd
{
public:
    ~CParseCmd(void)
    {
        EmptyThreads();
        m_threadExecTimeList.clear();
    }

    /***********************************************************************************************************************
    *   EmptyThreads
    ***********************************************************************************************************************/
    void EmptyThreads(void)
    {
        while (!m_threadList.isEmpty()) {
            CTIA_Thread *thread_p = m_threadList.first();

            if (thread_p != nullptr) {
                /* Code analysis, C28182 */
                m_threadExecTimeList.append(thread_p->m_execTime);
                delete (thread_p);
                m_threadList.removeFirst();
            }
        }
    }

    void PrintExecTimes(void);

    void Setup(CFileCtrl_FileHandle_t *fileHandle_p, int64_t file_StartOffset, int64_t file_TotalSize,
               char *workMem_p, int64_t size);    /* Creates a set of threads to handle the ParseCmd */
    void Execute(void);

    void FileStore(QFile& qfile);

    int GetNumOf_TI(void);

    QList <CTIA_Thread *> m_threadList;                   /* Each thread is configured with start and length */
	double m_seekTime {0.0};
	int64_t m_fileLoadTime {0};
	double m_processTime{0.0};
	int64_t m_fileStart{0};                                  /* File index to seek to */

private:
    /* A thread never works on memory more than 2^31 */
    void AddThread(char *start_p, int64_t size, int64_t fileStartIndex);

    CFileCtrl_FileHandle_t *m_fileHandle_p;
    int64_t m_fileSize = 0;                                   /* The total size of the file */
    int64_t m_size = 0;                                       /* Number of bytes to read */
    char *m_workMem_p = nullptr;
    QList <double> m_threadExecTimeList;
    CTIA_FileStorage m_fileStorage;                                /* Storage of the result of the parse command (TIAs)
                                                                    * */
};

/***********************************************************************************************************************
*   CFileCtrl
***********************************************************************************************************************/
class CFileCtrl final /* final as no virtual desctructor */
{
public:
    CFileCtrl(void);
    ~CFileCtrl(void)
    {
        while (!m_parseCmdList.isEmpty()) {
            delete (m_parseCmdList.takeFirst());
        }
    }

    bool Search_TIA(QFile *qfile_p, const QString& TIA_fileName, char *work_mem_p, int64_t workMemSize, int *rows_p);
    bool Search_TIA_Incremental(QFile& logFile, const QString& TIA_fileName, char *work_mem_p, int64_t workMemSize,
                                int64_t startFromIndex, int32_t fromRowIndex, int *rows_added_p);

private:
    bool Write_TIA_Header(bool empty = false); /* Set empty=true and just the space for the header will be written */

    CFileCtrl_FileHandle_t m_LogFile;   /* file handle etc to the log file */
    int m_numOf_TI;
    double m_loadTime;
    QFile m_TIA_File;
    QString m_TIA_FileName;
    QList <CParseCmd *> m_parseCmdList;
};
