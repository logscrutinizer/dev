/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <time.h>
#include <string.h>

#include <QDateTime>
#include <QDebug>
#include <QMessageBox>

#include "CDebug.h"
#include "quickFix.h"
#include "CConfig.h"
#include "globals.h"

#include <exception>

using namespace std;

CDebug *g_DebugLib = nullptr;
CRamLog *g_RamLog = nullptr;
static int g_warnings = 0;
static int g_errors = 0;
static QThreadStorage<tls_data_t *> tls_data;

CDebug::CDebug(void)
{
    m_traceWindowEnabled = true;
    m_isSystemError = false;

    /* Default initalization of log level should be done in the module who own the value, as such here */
#ifdef _DEBUG

/*    m_traceCategory = LOG_TRACE_CATEGORY_SIZE | LOG_TRACE_CATEGORY_NONE; */
    m_traceCategory = LOG_TRACE_CATEGORY_NONE;
    m_logLevel = LOG_LEVEL_INFO;
#else
    m_traceCategory = LOG_TRACE_CATEGORY_NONE;
    m_logLevel = LOG_LEVEL_INFO;
#endif

    m_mainThread = QThread::currentThreadId();

    m_file_p = nullptr;
    g_DebugLib = this;

    /* QT_TODO: m_h_pluginMsgHeap = HeapCreate(0, PLUGIN_MSG_HEAP_SIZE, PLUGIN_MSG_HEAP_SIZE_MAX); */
}

/***********************************************************************************************************************
*   cleanUp
***********************************************************************************************************************/
void CDebug::cleanUp(void)
{
    if (m_file_p != nullptr) {
        m_file_p->flush();
        m_file_p->close();
        delete m_file_p;
        m_file_p = nullptr;
    }
}

/***********************************************************************************************************************
*   ErrorHook
***********************************************************************************************************************/
void CDebug::ErrorHook(const char *hookText)
{
    static time_t lastErrorDlgTime = 0;
    time_t timeNow;

    g_errors++;

    g_RamLog->fileDump();

    if (CSCZ_SystemState != SYSTEM_STATE_RUNNING) {
        return;
    }

#ifdef _DEBUG
    m_isSystemError = true;
#endif

    time(&timeNow);

    /* Must be 10 sec since last error { */
    if (timeNow - lastErrorDlgTime > 2) {
        lastErrorDlgTime = timeNow;

        QString message = QObject::tr("LogScrutinizer has detected an error, try to save "
                                      "you workspace before it dumps. Info: ");
        message += QString(hookText);

        extern QWidget *MW_Parent();
        QMessageBox::critical(MW_Parent(), QObject::tr("LogScrutinizer Error"), message, QMessageBox::Ok);
    }

    if (g_cfg_p->m_throwAtError) {
        throw 10;
    }
}

/***********************************************************************************************************************
*   WarningHook
***********************************************************************************************************************/
void CDebug::WarningHook(void)
{
    g_warnings++;
}

/***********************************************************************************************************************
*   Initialize
***********************************************************************************************************************/
void CDebug::Initialize(void)
{
    QDateTime now = QDateTime::currentDateTime();
    now = now.toLocalTime();

    QString dateString = now.toString("yyyy_MM_dd_hh_mm_ss_zzz");

#ifdef UNIQUE_LOG_FILES
    QString fileName;
    QTextStream fileNameStream(&fileName);
    fileNameStream << "LogScrutinizer_" << dateString << ".txt";

    auto openFlags = QIODevice::WriteOnly;
#else
    QString fileName("logscrutinizer_log.txt");
    auto openFlags = QIODevice::Append;
#endif

    QString logString = "";
    QTextStream textStream(&logString);
    m_file_p = new QFile(fileName);
    if (!m_file_p->open(openFlags)) {
        QString errorMsg = m_file_p->errorString();
        textStream << "Log file could not be opened: " << fileName << " Error:" << errorMsg << endl;
        qDebug() << logString;
        return;
    }

    textStream << "LogScrutinizer Started at ";
    textStream << dateString;

#ifdef _DEBUG
    textStream << QString(" - DEBUG - Log File") << endl << endl;
#else
    textStream << QString(" - RELEASE - Log File") << endl << endl;
#endif

    QTextStream out(m_file_p);
    out << logString;
}

/* SLOW, use from main thread in non-time critical operations */
#ifndef _WIN32

/* https://stackoverflow.com/questions/20167124/vsprintf-and-vsnprintf-wformat-nonliteral-warning-on-clang-5-0 */
__attribute__((__format__(__printf__, 3, 0)))
#endif

/***********************************************************************************************************************
*   TRACEX
***********************************************************************************************************************/
void CDebug::TRACEX(int logLevel, const char *pcStr, ...)
{
    QString msgString;
    va_list tArgumentPointer;
    char argumentsString[DBG_CFG_TEMP_STRING_MAX_SIZE];

#ifndef _DEBUG
    if (logLevel > m_logLevel) {
        return;
    }
#endif

    va_start(tArgumentPointer, pcStr);
#ifdef _WIN32
    vsprintf_s(argumentsString, pcStr, tArgumentPointer);
#else
    vsprintf(argumentsString, pcStr, tArgumentPointer);
#endif
    va_end(tArgumentPointer);

    msgString = argumentsString;

    TRACEX(logLevel, &msgString);
}

/***********************************************************************************************************************
*   TRACEX_with_QFile
***********************************************************************************************************************/
void CDebug::TRACEX_with_QFile(int logLevel, const char *msgString_p, QFile *qfile_p)
{
    QString errorMsg(msgString_p);
    errorMsg.append("\n");
    errorMsg.append(qfile_p->fileName());
    errorMsg.append("\n");
    errorMsg.append(qfile_p->errorString());
    qDebug() << errorMsg;
    g_DebugLib->TRACEX(logLevel, &errorMsg);
}

/***********************************************************************************************************************
*   TRACEX
***********************************************************************************************************************/
void CDebug::TRACEX(int logLevel, const QString& msgString)
{
    TRACEX(logLevel, &msgString);
}

/***********************************************************************************************************************
*   TRACEX
***********************************************************************************************************************/
void CDebug::TRACEX(int logLevel, const QString *msgString_p)
{
    time_t localtime;
    QString logString;
    QTextStream textStream(&logString);
    QTextStream fileStream(m_file_p);

    if (CSCZ_SystemState == SYSTEM_STATE_SHUTDOWN) {
        return;
    }

    if (m_isSystemError) {
        return;
    }

    if ((g_RamLog != nullptr) && (msgString_p != nullptr)) {
        g_RamLog->AddBuffer(msgString_p->toLatin1().data());
    }

    if (logLevel > m_logLevel) {
        /* Log-level of print is too high, not prioritized */
        return;
    }

    if (m_file_p == nullptr) {
        ErrorHook("CDebug::TRACEX, m_debugFile not set\n");
        return;
    }

    {
        /* Locking scope */
        QMutexLocker ml(&m_mutex);  /* when ml passes its scope the mutex will automatically be freed */
#ifndef _DEBUG
        if (logLevel >= LOG_LEVEL_INFO)
#endif
        {
            time(&localtime);

            if (localtime != m_lastlocaltime) {
                QDateTime now = QDateTime::currentDateTime();
                now = now.toLocalTime();

                m_timeString = now.toString("yyyy.MM.dd hh:mm:ss.zzz");
                m_lastlocaltime = localtime;
            }

            textStream << m_timeString << " ";
            textStream << *msgString_p;

            if (m_traceWindowEnabled && (logLevel <= m_logLevel)) {}

            fileStream << logString << endl;
        }

        extern bool MW_AppendLogMsg(const QString & message);
        if (logLevel <= m_logLevel) {
            MW_AppendLogMsg(logString);
        }

#ifdef _DEBUG
        qDebug() << logString.simplified();
#endif

        fileStream.flush();
    } /* Locking scope */

    if (logLevel == LOG_LEVEL_ERROR) {
        ErrorHook(msgString_p->toLatin1().data());
    } else if (logLevel == LOG_LEVEL_WARNING) {
        WarningHook();
    }
}

/* Plugin Dbg message handling  hwnd_traceConsumer window handler to receive message, h_traceHeap is a memory
 * heap shared by plugin and LS */
#ifdef QT_TODO
HANDLE CDebug::GetPluginMsgHeap(void)
{
    return m_h_pluginMsgHeap;
}

/***********************************************************************************************************************
*   FreePluginMsg
***********************************************************************************************************************/
void CDebug::FreePluginMsg(char *msg_p)
{
    /* HeapFree(m_h_pluginMsgHeap, 0, msg_p);  // xxx QT */
    Q_UNUSED(msg_p)
}
#endif

/* Each thread is given their own ramLogData_t, through QThreadStorage. The QThreadStorage is a template of kind
 * tls_data_t which currently only contains a reference to ramLogData_t. Hence, each thread may set its own ramLog
 * reference as local data.
 *
 *  The ramLogs them selves are stored in a pool (list), and each time a thread is created a ramLog is picked from
 *  that pool
 *
 *  The ramLog is constructed by adding additional bytes to the ramLogData_t structure, making the ramLog member grow
 *  to RAM_LOG_SIZE size. The ramLog itself is constructed by several ramLogEntry_t, defining a header and data, where
 *  data is filled with the actual text log.
 */

CRamLog::CRamLog()
{
    for (int index = 0; index < RAM_LOG_MAX_COUNT; ++index) {
        ramLogData_t *mem_p = static_cast<ramLogData_t *>(malloc(sizeof(ramLogData_t)));

        if (mem_p != nullptr) {
            m_ramLogPool.append(mem_p);
            m_ramLogPoolTracking.append(mem_p);
        }
    }

    g_RamLog = this;
}

/***********************************************************************************************************************
*   cleanUp
***********************************************************************************************************************/
void CRamLog::cleanUp(void)
{
    /* If all works correctly then each thread would have terminated and returned their Local Data before the CRamLog
     * class is destroyed, hence it should be sufficent to just free all items in the list.
     */

    m_cleanUpDone = true;

    if (m_ramLogPool.count() != RAM_LOG_MAX_COUNT) {
        qDebug() << "RamLog not entirely empty ... " << (RAM_LOG_MAX_COUNT - m_ramLogPool.count()) << " items left\n";
    }

    while (!m_ramLogPoolTracking.empty()) {
        ramLogData_t *ramLogData_p = m_ramLogPoolTracking.takeFirst();
        if (ramLogData_p != nullptr) {
            memset(ramLogData_p, 0, sizeof(*ramLogData_p));
            free(ramLogData_p);
        }
    }
}

/***********************************************************************************************************************
*   AddBuffer
***********************************************************************************************************************/
bool CRamLog::AddBuffer(char *buffer_p, size_t size)
{
    if (!tls_data.hasLocalData() || (tls_data.localData()->ramLogData_p == nullptr)) {
        RegisterThread();
    }

    size++; /*0 termination*/

    tls_data_t *tls_data_p = tls_data.localData();
    ramLogData_t *ramLog_p = tls_data_p->ramLogData_p;

#ifdef _DEBUG

    /* Qualify the RAM Log setup */
    if (!ramLog_p->used || (ramLog_p->stamp != 0xbeefbeef)) {
        return false;
    }

    /* check pointer ranges as well */
    if ((ramLog_p->nextRamLogEntryStart_p < reinterpret_cast<ramLogEntry_t *>(&(ramLog_p->ramLog[0])))) {
        g_DebugLib->ErrorHook("RamLog corrupt, entryStart_p is wrong\n");
        return false;
    }
#endif

    /* Check for log wrapping */
    if (&(ramLog_p->nextRamLogEntryStart_p->data[size]) >=
        &ramLog_p->ramLog[RAM_LOG_SIZE -
                          1]) {
        ramLog_p->nextRamLogEntryStart_p =
            reinterpret_cast<ramLogEntry_t *>(&ramLog_p->ramLog[0]);
    }

    /* Make an extra check such that if the text is longer than the total size of the RAM log then the
     * print will not be added to the buffer at all */
    if (&ramLog_p->nextRamLogEntryStart_p->data[size] >= &ramLog_p->ramLog[RAM_LOG_SIZE - 1]) {
        return false;
    }

    ramLog_p->nextRamLogEntryStart_p->startMarker = RAM_LOG_ENTRY_START_MARKER;
    ramLog_p->nextRamLogEntryStart_p->size = size;
    ramLog_p->nextRamLogEntryStart_p->seqCount = ramLog_p->count;
    ++ramLog_p->count;

    memcpy(&(ramLog_p->nextRamLogEntryStart_p->data[0]), buffer_p, size);  /* copy the print */
    ramLog_p->lastRamLogEntryStart_p = ramLog_p->nextRamLogEntryStart_p;
    ramLog_p->nextRamLogEntryStart_p = reinterpret_cast<ramLogEntry_t *>(&ramLog_p->nextRamLogEntryStart_p->data[size]);

    return true;
}

/***********************************************************************************************************************
*   RegisterThread
***********************************************************************************************************************/
bool CRamLog::RegisterThread(void)
{
    if (tls_data.hasLocalData()) {
        g_DebugLib->ErrorHook("Thread has already registered RamLog\n");
        return false;  /* allready registered */
    }

    tls_data_t *tls_data_p = static_cast<tls_data_t *>(malloc(sizeof(tls_data_t)));
    tls_data.setLocalData(tls_data_p);

    QMutexLocker ml(&m_mutex);  /* when ml passes its scope the mutex will automatically be freed */
    ramLogData_t *ramLogData_p = m_ramLogPool.takeFirst();

    tls_data_p->ramLogData_p = ramLogData_p;

    memset(ramLogData_p, 0, sizeof(*ramLogData_p));

    /*uint32_t *endMarker = reinterpret_cast<char*>(ramLogData) + sizeof(rmLogData_t) + RAM_LOG_SIZE */

    ramLogData_p->threadID = QThread::currentThreadId();
    ramLogData_p->used = true;
    ramLogData_p->stamp = 0xbeefbeef;
    ramLogData_p->nextRamLogEntryStart_p = reinterpret_cast<ramLogEntry_t *>(&ramLogData_p->ramLog[0]);
    ramLogData_p->lastRamLogEntryStart_p = nullptr;

    return true;
}

/***********************************************************************************************************************
*   UnregisterThread
***********************************************************************************************************************/
void CRamLog::UnregisterThread(void)
{
    tls_data_t *tls_data_p = static_cast<tls_data_t *>(tls_data.localData());
    ramLogData_t *ramLog_p = tls_data_p->ramLogData_p;

    if (m_cleanUpDone) {
        return;
    }

    if (ramLog_p != nullptr) {
        QMutexLocker ml(&m_mutex);  /* when ml passes its scope the mutex will automatically be freed */
        ramLog_p->used = false;
        m_ramLogPool.append(ramLog_p);  /* return the ramLog to the pool */
        ramLog_p->stamp = 0;
        tls_data.setLocalData(nullptr);  /* remove the tls_data */
    }
}

/***********************************************************************************************************************
*   CheckRamLogs
***********************************************************************************************************************/
void CRamLog::CheckRamLogs(void)
{
    ramLogData_t *ramLog_p;
    const quint32 totalSize = sizeof(ramLogData_t) + RAM_LOG_SIZE;
    QMutexLocker ml(&m_mutex);  /* when ml passes its scope the mutex will automatically be freed */

    for (int index = 0; index < RAM_LOG_MAX_COUNT; ++index) {
        ramLogEntry_t *entry_p;

        /* Must use m_ramLogPoolDebug since that keeps all ramLogData references, the other is just a pool */
        if (m_ramLogPoolTracking[index]->used && (m_ramLogPoolTracking[index]->stamp == 0xbeefbeef)) {
            ramLog_p = m_ramLogPoolTracking[index];

            const ramLogEntry_t *endEntry_p = reinterpret_cast<ramLogEntry_t *>(&ramLog_p->ramLog[0] + totalSize);
            entry_p = reinterpret_cast<ramLogEntry_t *>(&ramLog_p->ramLog[0]);

            bool stop = false;
            int count = 0;

            while (entry_p < endEntry_p && !stop) {
                if (entry_p->startMarker != 0xbeefbeef) {
                    TRACEX_E("RamLog entry overwritten\n")
                    return;
                }
                ++count;

                /* this will occur if the ramLog hasn't wrapped, otherwise the ramLog will end at wrap point */
                if (count == ramLog_p->count) {
                    stop = true;
                }

                /* when ramLog has wrapped this is used to indicate the current wrapPoint */
                if (entry_p == ramLog_p->lastRamLogEntryStart_p) {
                    stop = true;
                }
                entry_p = reinterpret_cast<ramLogEntry_t *>(&entry_p->data[entry_p->size]);
            }
        }
    }
}

static char RamLogTestArray[][40] =
{
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "Hej", "Svej", "Grej", "Paj", "1232312312HubbaHepp", "SmockDock", "Flubber", "Hubber", "HubbaHopps",
    "EOF"
};

/***********************************************************************************************************************
*   TestRamLogs
***********************************************************************************************************************/
void CRamLog::TestRamLogs(void)
{
    g_RamLog->RegisterThread();

    int addCount = 0;
    while (strcmp(RamLogTestArray[addCount], "EOF") != 0) {
        g_RamLog->AddBuffer(RamLogTestArray[addCount]);
        ++addCount;
    }

    g_RamLog->CheckRamLogs();

    char *pointerArray[100];
    int count = GetRamLog(0, pointerArray, 100);

    if (count != addCount) {
        TRACEX_E("RamLog Count Wrong\n")
    }

    int checkCount = 0;
    while (checkCount < count) {
        if (strcmp(RamLogTestArray[checkCount], pointerArray[checkCount]) != 0) {
            TRACEX_E("RamLog Data Wrong\n")
        }

        ++checkCount;
    }

    g_RamLog->fileDump(false);

    g_RamLog->UnregisterThread();
}

/***********************************************************************************************************************
*   GetRamLog
***********************************************************************************************************************/
int CRamLog::GetRamLog(int index, char **pointerArray_pp, int pointerArrayMaxSize)
{
    ramLogData_t *ramLog_p;
    ramLogEntry_t *entry_p;

    if (!m_ramLogPoolTracking[index]->used) {
        return 0;
    }

    ramLog_p = m_ramLogPoolTracking[index];

    const ramLogEntry_t *endEntry_p = reinterpret_cast<ramLogEntry_t *>(&(ramLog_p->ramLog[RAM_LOG_SIZE]));

    entry_p = reinterpret_cast<ramLogEntry_t *>(&ramLog_p->ramLog[0]);

    int entryIndex = 0;
    bool found = false;

    while ((entry_p < endEntry_p) && (entryIndex < pointerArrayMaxSize) &&
           (entry_p->startMarker == 0xbeefbeef)) {
        pointerArray_pp[entryIndex] = &entry_p->data[0];
        ++entryIndex;
        entry_p = reinterpret_cast<ramLogEntry_t *>(&entry_p->data[entry_p->size]);
    }

    if ((entry_p < endEntry_p) && (entryIndex < pointerArrayMaxSize)) {
        /* The previous loop ended becauce the log is wrapped, hence corrupt in the middle. Now search for
         * the next valid start of the log */
        char *search_ptr = reinterpret_cast<char *>(entry_p);

        while ((search_ptr < &(ramLog_p->ramLog[RAM_LOG_SIZE]) - sizeof(uint32_t)) && !found) {
            uint32_t marker = *reinterpret_cast<uint32_t *>(search_ptr);
            if (marker == 0xbeefbeef) {
                entry_p = reinterpret_cast<ramLogEntry_t *>(search_ptr);
                found = true;
            } else {
                search_ptr++;
            }
        }
    }

    while ((entry_p < endEntry_p) && (entryIndex < pointerArrayMaxSize) && (entry_p->startMarker == 0xbeefbeef)) {
        pointerArray_pp[entryIndex] = &entry_p->data[0];
        ++entryIndex;
        entry_p =
            reinterpret_cast<ramLogEntry_t *>(reinterpret_cast<char *>(entry_p) + entry_p->size +
                                              sizeof(ramLogEntry_t));
    }

    return entryIndex;
}

/***********************************************************************************************************************
*   fileDump
***********************************************************************************************************************/
void CRamLog::fileDump(bool updateTimeStamp)
{
    QString logString = "";
    QTextStream textStream(&logString);
    QString fileName;
    QTextStream fileNameStream(&fileName);
    static time_t lastDumpTime = 0; /* Seconds since last dump */

    if (updateTimeStamp) {
        time_t timeNow;
        time(&timeNow);
        if (timeNow - lastDumpTime > 2) {
            lastDumpTime = timeNow;
        } else {
            /* Do not RAM dump over and over again the same stuff */
            return;
        }
    }

    textStream << "LogScrutinizer RAM log at ";

    QDateTime now = QDateTime::currentDateTime();
    now = now.toLocalTime();

    QString dateString = now.toString("yyyy_MM_dd_hh_mm_ss_zzz");
    fileNameStream << "LogScrutinizer_RamLog_" << dateString << ".txt";
    textStream << dateString;

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        QString errorMsg = file.errorString();
        textStream << "Debug could not be opened: " << fileName << " Error:" << errorMsg << endl;
        qDebug() << logString;
        return;
    }

    QTextStream out(&file);
    out << logString;

    QMutexLocker ml(&m_mutex);  /* when ml passes its scope the mutex will automatically be freed */

    for (auto& ramLog_p : m_ramLogPoolTracking) {
        if (ramLog_p->used) {
            const ramLogEntry_t *endEntry_p = reinterpret_cast<ramLogEntry_t *>(&(ramLog_p->ramLog[RAM_LOG_SIZE]));
            auto entry_p = reinterpret_cast<ramLogEntry_t *>(&ramLog_p->ramLog[0]);
            out << QString("\n\nRAM LOG START thread:%1 count:%2\n\n")
                .arg(reinterpret_cast<uint64_t>(ramLog_p->threadID)).arg(ramLog_p->count);

            int entryIndex = 0;
            bool found = false;

            while ((entry_p < endEntry_p) && (entry_p->startMarker == 0xbeefbeef)) {
                out << QString("#%1 ").arg(entry_p->seqCount) << QString(&entry_p->data[0]) << QString("\n");
                ++entryIndex;
                entry_p = reinterpret_cast<ramLogEntry_t *>(&entry_p->data[entry_p->size]);
            }

            if (entry_p < endEntry_p) {
                /* The previous loop ended becauce the log is wrapped, hence corrupt in the middle. Now search for
                 * the next valid start of the log */

                char *search_ptr = reinterpret_cast<char *>(entry_p);

                while ((search_ptr < &(ramLog_p->ramLog[RAM_LOG_SIZE]) - sizeof(uint32_t)) && !found) {
                    uint32_t marker = *reinterpret_cast<uint32_t *>(search_ptr);
                    if (marker == 0xbeefbeef) {
                        entry_p = reinterpret_cast<ramLogEntry_t *>(search_ptr);
                        found = true;
                    } else {
                        search_ptr++;
                    }
                }
            }

            while ((entry_p < endEntry_p) && (entry_p->startMarker == 0xbeefbeef)) {
                out << QString("#%1 ").arg(entry_p->seqCount) << QString(&entry_p->data[0]) << QString("\n");
                ++entryIndex;
                entry_p = reinterpret_cast<ramLogEntry_t *>(&entry_p->data[entry_p->size]);
            }
        }
    }

    file.flush();
}
