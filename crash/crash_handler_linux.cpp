/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "crash_handler_linux.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <signal.h>
#include <exception>
#include <sys/stat.h>
#include <fstream>
#include "time.h"

#include "CConfig.h"
#include "CDebug.h"

using namespace std;

#define CrashHandlerTerminateProcessValue 0xffffffff

bool CCrashHandler::m_isStrategyChange = false;

CCrashHandler::CCrashHandler()
{
    SetProcessExceptionHandlers();
    SetThreadExceptionHandlers();
}

CCrashHandler::~CCrashHandler()
{}

/***********************************************************************************************************************
*   SetProcessExceptionHandlers
***********************************************************************************************************************/
void CCrashHandler::SetProcessExceptionHandlers()
{
    /* Catch an abnormal program termination */
    signal(SIGABRT, SigabrtHandler);

    /* Catch illegal instruction handler */
    signal(SIGINT, SigintHandler);

    /* Catch a termination request */
    signal(SIGTERM, SigtermHandler);

    TRACEX_I("Error handling is configured\n");
}

/***********************************************************************************************************************
*   SetThreadExceptionHandlers
***********************************************************************************************************************/
void CCrashHandler::SetThreadExceptionHandlers()
{
    /* Catch terminate() calls.
     * In a multithreaded environment, terminate functions are maintained
     * separately for each thread. Each new thread needs to install its own
     * terminate function. Thus, each thread is in charge of its own termination handling.
     * http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx */
    set_terminate(TerminateHandler);

    /* Catch unexpected() calls.
     * In a multithreaded environment, unexpected functions are maintained
     * separately for each thread. Each new thread needs to install its own
     * unexpected function. Thus, each thread is in charge of its own unexpected handling.
     * http://msdn.microsoft.com/en-us/library/h46t5b69.aspx */
    set_unexpected(UnexpectedHandler);

    /* Catch a floating point error */
    typedef void (*sigh)(int);
    signal(SIGFPE, (sigh)SigfpeHandler);

    /* Catch an illegal instruction */
    signal(SIGILL, SigillHandler);

    /* Catch illegal storage access errors */
    signal(SIGSEGV, SigsegvHandler);
}

#if 0

/* The following code gets exception pointers using a workaround found in CRT code. */
void CCrashHandler::GetExceptionPointers(uint32_t dwExceptionCode)
{}

/* This method creates minidump of the process */
void CCrashHandler::CreateMiniDump(void)
{
    struct tm timeinfo;
    time_t rawtime;

    time(&rawtime);

    errno_t error = localtime_s(&timeinfo, &rawtime); /* Get local time */
    char buf[25];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &timeinfo); /* Convert time in YYYYMMDD_HHMMSS format */

    QString timeExtension = buf;
    QString fileExtension = ".dmp";

    /* Save the dump file to LogPath location */

    /*
     *  char* title_p;
     *  char* version_p;
     *  char* buildDate_p;
     *  char* configuration_p;
     *
     *  g_cfg_p->GetAppInfo(&title_p, &version_p, &buildDate_p, &configuration_p);
     *
     *  //string dumpFile = "logscrutinizer_dump" + timeExtension + fileExtension;
     */
    QString zipFile;
    QString workspaceFile;
    QString dumpFile = APP_TITLE;  /* "logscrutinizer_dump" + timeExtension + fileExtension; */
    dumpFile += "_";
    dumpFile += APP_VER_FILE;
    dumpFile += "_";
    dumpFile += timeExtension;

    zipFile = dumpFile + ".zip";
    workspaceFile = dumpFile + ".lsz";

    dumpFile += fileExtension;

    char errorMessage[CFG_TEMP_STRING_MAX_SIZE];
    char directory[CFG_TEMP_STRING_MAX_SIZE];

    directory[0] = 0;

    GetCurrentDirectory(CFG_TEMP_STRING_MAX_SIZE, directory);

    sprintf_s(errorMessage,
              CFG_TEMP_STRING_MAX_SIZE,
              "A dump file has been generated! \n"
              "Please send the dump file by mail to support@logscrutinizer.com, and we will try to correct the bug and send you an updated version of this software.\n"
              "You find the dump file\n"
              "<%s>\n"
              "in this folder\n"
              "<%s>\n"
              "A backup of your workspace was also saved in that same folder with same name, but with *.lsz as extension",
              zipFile.GetBuffer(),
              directory);

    MessageBox(nullptr, errorMessage, "LogScrutinizer has crashed", MB_OK | MB_ICONERROR | MB_TOPMOST);

    /* zip it */
    HZIP hz = CreateZip((void *)zipFile.GetBuffer(), 0, ZIP_FILENAME);

    if (hz) {
        ZRESULT zr = ZipAdd(hz, dumpFile.GetBuffer(), (void *)dumpFile.GetBuffer(), 0, ZIP_FILENAME);
        CloseZip(hz);
    }

    ::DeleteFile(dumpFile.GetBuffer());

    extern bool CFGCTRL_SaveWorkspaceFile(LPCTSTR fileName, bool force = false);

    CFGCTRL_SaveWorkspaceFile(workspaceFile, true);
}

/* Structured exception handler */
LONG CCrashHandler::SehHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{
    /* Terminate process */
    TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);

    /* Unreacheable code */
    return EXCEPTION_EXECUTE_HANDLER;
}

/***********************************************************************************************************************
*   GetExceptionAndTerminate
***********************************************************************************************************************/
void CCrashHandler::GetExceptionAndTerminate()
{
    extern void CProgressMgr_Abort(void);

    CProgressMgr_Abort(); /* set abort flag in progress control */

    /* Terminate process */
    TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);
}

/* CRT Pure virtual method call handler */
void CCrashHandler::PureCallHandler()
{
    /* Pure virtual function call
     * GetExceptionAndTerminate(); */
    abort();
}

/* CRT invalid parameter handler */
void CCrashHandler::InvalidParameterHandler(
    const wchar_t *expression,
    const wchar_t *function,
    const wchar_t *file,
    unsigned int line,
    uintptr_t pReserved)
{
    pReserved;

    /* Invalid parameter exception */
    GetExceptionAndTerminate();
}

#endif

/***********************************************************************************************************************
*   TerminateHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::TerminateHandler()
{
    /* CRT terminate() call handler
     * Abnormal program termination (terminate() function was called)
     * GetExceptionAndTerminate(); */
    g_RamLog->fileDump();
    abort();
}

/***********************************************************************************************************************
*   UnexpectedHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::UnexpectedHandler()
{
    /* CRT unexpected() call handler
    * Unexpected error (unexpected() function was called)
    * GetExceptionAndTerminate(); */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigabrtHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigabrtHandler(int)
{
    /* CRT SIGABRT signal handler
     * Caught SIGABRT C++ signal */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigfpeHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigfpeHandler(int /*code*/, int subcode)
{
    /* CRT SIGFPE signal handler */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigillHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigillHandler(int)
{
    /* CRT sigill signal handler */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigintHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigintHandler(int)
{
    /* CRT sigint signal handler */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigsegvHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigsegvHandler(int)
{
    /* CRT SIGSEGV signal handler */
    TerminateHandler();
}

/***********************************************************************************************************************
*   SigtermHandler
***********************************************************************************************************************/
[[ noreturn ]] void CCrashHandler::SigtermHandler(int)
{
    /* CRT SIGTERM signal handler */
    TerminateHandler();
}
