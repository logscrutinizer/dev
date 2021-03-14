/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <QFile>
#include <QThread>
#include <QMutex>
#include <QTextStream>
#include <QDebug>
#include <QThreadStorage>

#define DBG_CFG_TEMP_STRING_MAX_SIZE 2048

#define LOG_LEVEL_ERROR           0    /* Detected errors */
#define LOG_LEVEL_WARNING         1    /* Things that might be wrong */
#define LOG_LEVEL_INFO            2    /* Mem sizes, loaded files, w */
#define LOG_LEVEL_DEBUG           3    /* Good for debugging */
#define LOG_LEVEL_DEBUG_EXT       4    /* Much data */

#define LOG_TRACE_CATEGORY_NONE       0
#define LOG_TRACE_CATEGORY_FOCUS      (1 << 0)
#define LOG_TRACE_CATEGORY_THREADS    (1 << 1)
#define LOG_TRACE_CATEGORY_MOUSE      (1 << 2)
#define LOG_TRACE_CATEGORY_ROCKSCROLL (1 << 3)
#define LOG_TRACE_CATEGORY_SIZE       (1 << 4)
#define LOG_TRACE_CATEGORY_SUBPLOTSURFACE (1 << 5)
#define LOG_TRACE_CATEGORY_SCROLL_INFO (1 << 6)
#define LOG_TRACE_CATEGORY_WHEEL_INFO (1 << 7)
#define LOG_TRACE_CATEGORY_FILE_TRACKING (1 << 8)
#define LOG_TRACE_CATEGORY_CURSOR (1 << 9)
#define LOG_TRACE_CATEGORY_PROGRESS (1 << 10)
#define LOG_TRACE_CATEGORY_SELECTION (1 << 11)
#define LOG_TRACE_CATEGORY_KEYPRESS (1 << 12)
#define LOG_TRACE_CATEGORY_CPLOTWIDGET_GRAPHICS (1 << 13)
#define LOG_TRACE_CATEGORY_WORKSPACE_MODEL (1 << 14)

#define LOG_TRACE_CATEGORY_ALL        (0xffffff)
#define FOO(fmt, ...) printf(fmt, ## __VA_ARGS__)

#define TRACEX_I(...) {if (g_DebugLib != nullptr) {g_DebugLib->TRACEX(LOG_LEVEL_INFO, ## __VA_ARGS__);}}
#define TRACEX_W(...) {if (g_DebugLib != nullptr) {g_DebugLib->TRACEX(LOG_LEVEL_WARNING, ## __VA_ARGS__);}}
#define TRACEX_E(...) {if (g_DebugLib != nullptr) {g_DebugLib->TRACEX(LOG_LEVEL_ERROR, ## __VA_ARGS__);}}
#define TRACEX_D(...) {if (g_DebugLib != nullptr) {g_DebugLib->TRACEX(LOG_LEVEL_DEBUG, ## __VA_ARGS__);}}
#define TRACEX_DE(...) {if (g_DebugLib != nullptr) {g_DebugLib->TRACEX(LOG_LEVEL_DEBUG_EXT, ## __VA_ARGS__);}}
#define TRACEX_QFILE(LEVEL, STR, QFILE) {if (g_DebugLib != nullptr) \
                                         {g_DebugLib->TRACEX_with_QFile(LEVEL, STR, QFILE);}}
#define TRACEX_IS_ENABLED(LEVEL) (((g_DebugLib != nullptr) && (LEVEL <= g_DebugLib->m_logLevel)))

#define TRACEX_ERROR(STR) {if (g_DebugLib != nullptr) {g_DebugLib->ErrorHook(STR);}}

#define TRACEX_ENABLE_WINDOW() {if (g_DebugLib != nullptr) {g_DebugLib->EnableTraceWindow();}}
#define TRACEX_DISABLE_WINDOW() {if (g_DebugLib != nullptr) {g_DebugLib->DisableTraceWindow();}}

#define LOG_TRACE_MOUSE_COND (g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_MOUSE)
#ifdef _DEBUG
 #define LOG_TRACE_MOUSE_COND_WARN (true)  /* Always show mouse warnings */
#else
 #define LOG_TRACE_MOUSE_COND_WARN LOG_TRACE_MOUSE_COND  /* Only show mouse warning if trace enabled */
#endif /*_DEBUG */

#define LOG_TRACE_SELECTION (g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_SELECTION)
#define LOG_TRACE_CAT_IS_ENABLED(CAT) ((g_DebugLib->m_traceCategory & (CAT)) > 0) ? true : false

#define PRINT_FOCUS(STRING) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_FOCUS) \
                             {g_DebugLib->TRACEX(LOG_LEVEL_INFO, (STRING));}}
#define PRINT_ROCKSCROLL(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_ROCKSCROLL) \
                               {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_SIZE(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_SIZE) \
                         {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_SUBPLOTSURFACE(...) {if (g_DebugLib != nullptr && \
                                       g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_SUBPLOTSURFACE) \
                                   {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_SCROLL_INFO(...) {if (g_DebugLib != nullptr && \
                                    g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_SCROLL_INFO) \
                                {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_WHEEL_INFO(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_WHEEL_INFO) \
                               {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_FILE_TRACKING(...) {if (g_DebugLib != nullptr && \
                                      g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_FILE_TRACKING) \
                                  {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_CURSOR(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_CURSOR) \
                           {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_SELECTION(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_SELECTION) \
                              {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_MOUSE(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_MOUSE) \
                          {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_PROGRESS(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_PROGRESS) \
                             {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_CPLOTWIDGET_GRAPHICS(...) {if (g_DebugLib != nullptr && \
                                             g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_CPLOTWIDGET_GRAPHICS) \
                                         {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_KEYPRESS(...) {if (g_DebugLib != nullptr && g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_KEYPRESS) \
                             {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}
#define PRINT_WS_MODEL(...) {if (g_DebugLib != nullptr && \
                                 g_DebugLib->m_traceCategory & LOG_TRACE_CATEGORY_WORKSPACE_MODEL) \
                             {g_DebugLib->TRACEX(LOG_LEVEL_INFO, __VA_ARGS__);}}

#ifdef _DEBUG
 #define PRINT_PROGRESS_DBG(...) PRINT_PROGRESS(__VA_ARGS__)
#else
 #define PRINT_PROGRESS_DBG(...)
#endif

/***********************************************************************************************************************
*   CDebug
***********************************************************************************************************************/
class CDebug
{
public:
    CDebug(void);
    ~CDebug(void) {}
    void cleanUp(void);

    void ErrorHook(const char *hookText);
    void WarningHook(void);

    void FreePluginMsg(char *msg_p);

    void Initialize(void);
    void TRACEX(int logLevel, const char *pcStr, ...);
    void TRACEX(int logLevel, const QString& msgString);
    void TRACEX(int logLevel, const QString *msgString_p);
    void TRACEX_with_QFile(int logLevel, const char *msgString_p, QFile *qfile_p);

    inline bool isTraceEnabled(int logLevel) {return (logLevel <= m_logLevel ? true : false);}

    void DisableTraceWindow(void) {m_traceWindowEnabled = false;}
    void EnableTraceWindow(void) {m_traceWindowEnabled = true;}

    bool m_isSystemError = false; /**< Set when in system error handler */
    bool m_isThreadCrashTrigger = false; /**< Used to trigger a crash in a thread */
    int m_logLevel; /**< Setting to control how much that is put into the log, 0 - Not important */
    bool m_traceWindowEnabled;
    int m_traceCategory; /**< Enable specific tracing of some areas */
    time_t m_lastlocaltime = 0;
    QString m_timeString;

private:
    QFile *m_file_p;
    Qt::HANDLE m_mainThread;
    QMutex m_mutex;
};

extern CDebug *g_DebugLib;

/*   RAM LOG */

#define RAM_LOG_MAX_COUNT  (MAX_NUM_OF_THREADS * 2) /**< Should be enough with MAX_NUM_OF_THREADS + 1, but
                                                     * there is competing thread hanling, CFileProcControl and
                                                     * thread manager in cthread.cpp */
#define RAM_LOG_SIZE                  (10 * 1024)  /* 10kB */
#define RAM_LOG_ENTRY_START_MARKER    (0xBEEFBEEF)
#define RAM_LOG_ENTRY_END_MARKER      (0xDEADBEEF)

typedef struct {
    uint32_t startMarker;
    int seqCount; /**< Value used to order the prints later on */
    size_t size;
    char data[1]; /**< MUST BE LAST MEMBER */
} ramLogEntry_t;

typedef struct {
    Qt::HANDLE threadID; /**< Which thread that registered to use this ramLog */
    ramLogEntry_t *nextRamLogEntryStart_p; /**< this will wrap around to the beginning */
    ramLogEntry_t *lastRamLogEntryStart_p; /**< this always points to the currently last entry, set at wrap */
    int count; /**< total number of entries */
    bool used; /**< True if the ramLog is being used */
    uint32_t stamp; /**< Extra check for valid */
    char ramLog[RAM_LOG_SIZE]; /**< Contains ramLogEntries */
} ramLogData_t;

typedef struct {
    ramLogData_t *ramLogData_p;
}tls_data_t;

/***********************************************************************************************************************
   CRamLog
***********************************************************************************************************************/
class CRamLog
{
public:
    CRamLog();
    ~CRamLog() {}

    void cleanUp(void);

    bool AddBuffer(char *buffer_p, size_t size);

    /***********************************************************************************************************************
       AddBuffer
    ***********************************************************************************************************************/
    bool AddBuffer(char *buffer_p)
    {
        return AddBuffer(buffer_p, strlen(buffer_p));
    }

    bool RegisterThread(void);
    void UnregisterThread(void);

    void TestRamLogs(void);

    /* returns number of strings in the RamLog transfered to pointerArray_pp */
    int GetRamLog(int index, char **pointerArray_pp, int pointerArraySize);

    /* Dump whats ever in the RAM log to file */
    void fileDump(bool updateTimeStamp = true);

private:
    void CheckRamLogs(void);

    bool m_cleanUpDone = false;
    QMutex m_mutex; /**< Protect the lists */
    QList<ramLogData_t *> m_ramLogPool;
    QList<ramLogData_t *> m_ramLogPoolTracking;  /**< m_ramLogPool keeps only unused,
                                                      m_ramLogPoolTracking keeps all */
};

extern CRamLog *g_RamLog;
