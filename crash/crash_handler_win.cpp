
#include "crash_handler_win.h"

#include <new.h>
#include <rtcapi.h>
#include <dbghelp.h>
#include <Shellapi.h>
#include <iostream>
#include <sstream>          // ofstream
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <signal.h>
#include <exception>
#include <sys/stat.h>
#include <psapi.h>
#include <io.h>
#include <crtdbg.h> // Needed for _CRT_ASSERT, _CRT_WARN, _CRT_ERROR in VCTK2003
#include <fstream>

#include "time.h"
//#include "..\zip\XZip.h"
//#include "..\zip\XUnzip.h"

#include "..\containers\CConfigurationCtrl.h"
#include "CConfig.h"
#include "CDebug.h"

using namespace std;

#ifndef _AddressOfReturnAddress

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// _ReturnAddress and _AddressOfReturnAddress should be prototyped before use
EXTERNC void * _AddressOfReturnAddress(void);
EXTERNC void * _ReturnAddress(void);

#endif

#define CrashHandlerTerminateProcessValue 0xffffffff

bool CCrashHandler::m_isStrategyChange = false;

CCrashHandler::CCrashHandler()
{
}

CCrashHandler::~CCrashHandler()
{
}

void CCrashHandler::SetProcessExceptionHandlers()
{
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

  // Install top-level SEH handler
//  SetUnhandledExceptionFilter(SehHandler);

  // Catch pure virtual function calls.
  // Because there is one _purecall_handler for the whole process,
  // calling this function immediately impacts all threads. The last
  // caller on any thread sets the handler.
  // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
  _set_purecall_handler(PureCallHandler);

  // Catch new operator memory allocation exceptions
  _set_new_handler(NewHandler);

#ifndef VCTK2003
  // Catch invalid parameter exceptions.
  _set_invalid_parameter_handler(InvalidParameterHandler);

  // Set up C++ signal handlers

  _set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif

  // Catch an abnormal program termination
  signal(SIGABRT, SigabrtHandler);

  // Catch illegal instruction handler
  signal(SIGINT, SigintHandler);

  // Catch a termination request
  signal(SIGTERM, SigtermHandler);

  TRACEX_I("Error handling is configured\n");
}

void CCrashHandler::SetThreadExceptionHandlers()
{

  // Catch terminate() calls.
  // In a multithreaded environment, terminate functions are maintained
  // separately for each thread. Each new thread needs to install its own
  // terminate function. Thus, each thread is in charge of its own termination handling.
  // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
  set_terminate(TerminateHandler);

  // Catch unexpected() calls.
  // In a multithreaded environment, unexpected functions are maintained
  // separately for each thread. Each new thread needs to install its own
  // unexpected function. Thus, each thread is in charge of its own unexpected handling.
  // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx
  set_unexpected(UnexpectedHandler);

  // Catch a floating point error
  typedef void (*sigh)(int);
  signal(SIGFPE, (sigh)SigfpeHandler);

  // Catch an illegal instruction
  signal(SIGILL, SigillHandler);

  // Catch illegal storage access errors
  signal(SIGSEGV, SigsegvHandler);

}

// The following code gets exception pointers using a workaround found in CRT code.
void CCrashHandler::GetExceptionPointers(uint32_t dwExceptionCode,
                                         EXCEPTION_POINTERS** ppExceptionPointers)
{
  // The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)

  EXCEPTION_RECORD ExceptionRecord;
  CONTEXT ContextRecord;
  memset(&ContextRecord, 0, sizeof(CONTEXT));

#ifdef _X86_

  __asm {
    mov dword ptr [ContextRecord.Eax], eax
    mov dword ptr [ContextRecord.Ecx], ecx
    mov dword ptr [ContextRecord.Edx], edx
    mov dword ptr [ContextRecord.Ebx], ebx
    mov dword ptr [ContextRecord.Esi], esi
    mov dword ptr [ContextRecord.Edi], edi
    mov word ptr [ContextRecord.SegSs], ss
    mov word ptr [ContextRecord.SegCs], cs
    mov word ptr [ContextRecord.SegDs], ds
    mov word ptr [ContextRecord.SegEs], es
    mov word ptr [ContextRecord.SegFs], fs
    mov word ptr [ContextRecord.SegGs], gs
    pushfd
    pop [ContextRecord.EFlags]
  }

  ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
  ContextRecord.Eip = (ULONG)_ReturnAddress();
  ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
  ContextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress()-1);


#elif defined (_IA64_) || defined (_AMD64_)

  /* Need to fill up the Context in IA64 and AMD64. */
  RtlCaptureContext(&ContextRecord);

#else  /* defined (_IA64_) || defined (_AMD64_) */

  ZeroMemory(&ContextRecord, sizeof(ContextRecord));

#endif  /* defined (_IA64_) || defined (_AMD64_) */

  ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));

  ExceptionRecord.ExceptionCode = dwExceptionCode;
  ExceptionRecord.ExceptionAddress = _ReturnAddress();

  ///

  EXCEPTION_RECORD* pExceptionRecord = new EXCEPTION_RECORD;
  memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));
  CONTEXT* pContextRecord = new CONTEXT;
  memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));

  *ppExceptionPointers = new EXCEPTION_POINTERS;
  (*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
  (*ppExceptionPointers)->ContextRecord = pContextRecord;
}

// This method creates minidump of the process
void CCrashHandler::CreateMiniDump(EXCEPTION_POINTERS* pExcPtrs)
{
  HMODULE hDbgHelp = nullptr;
  HANDLE hFile = nullptr;
  MINIDUMP_EXCEPTION_INFORMATION mei;
  MINIDUMP_CALLBACK_INFORMATION mci;

  struct tm timeinfo;
  time_t    rawtime;

  g_RamLog->fileDump();

  time(&rawtime);

  errno_t error = localtime_s(&timeinfo, &rawtime ); // Get local time

  char buf[25];
  strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &timeinfo); // Convert time in YYYYMMDD_HHMMSS format

  QString timeExtension = buf;
  QString fileExtension = ".dmp";

  // Save the dump file to LogPath location

  /*
  char* title_p;
  char* version_p;
  char* buildDate_p;
  char* configuration_p;

  g_cfg_p->GetAppInfo(&title_p, &version_p, &buildDate_p, &configuration_p);

  //string dumpFile = "logscrutinizer_dump" + timeExtension + fileExtension;
  */

  QString zipFile;
  QString workspaceFile;

  QString title, version, buildDate, configuration;
  g_cfg_p->GetAppInfo(title, version, buildDate, configuration);

  QString dumpFile = title;  // "logscrutinizer_dump" + timeExtension + fileExtension;
  dumpFile += "_";
  dumpFile += version + QString("_") + buildDate + QString("_") + configuration;
  dumpFile += "_";
  dumpFile += timeExtension;

  zipFile       = dumpFile + ".zip";
  workspaceFile = dumpFile + ".lsz";

  dumpFile += fileExtension;

  // Load dbghelp.dll
  hDbgHelp = LoadLibrary(_T("dbghelp.dll"));

  if (hDbgHelp==nullptr)
  {
    return;
  }

  // Create the minidump file
  hFile = CreateFile(
    dumpFile.toLatin1().constData(),
    GENERIC_WRITE,
    0,
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr);

  if (hFile==INVALID_HANDLE_VALUE)
  {
    // Couldn't create file
    cout << "******* couldn't create the minidump file*******" << endl;
    Sleep(3000);
    return;
  }

  // Write minidump to the file
  mei.ThreadId = GetCurrentThreadId();
  mei.ExceptionPointers = pExcPtrs;
  mei.ClientPointers = FALSE;
  mci.CallbackRoutine = nullptr;
  mci.CallbackParam = nullptr;

  typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(
    HANDLE hProcess,
    uint32_t ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump =
    (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
  if (!pfnMiniDumpWriteDump)
  {
    // Bad MiniDumpWriteDump function
    cout << "******* Bad MiniDumpWriteDump function*******" << endl;
    Sleep(3000);
    return;
  }

  HANDLE hProcess = GetCurrentProcess();
  uint32_t dwProcessId = GetCurrentProcessId();

  /*
  MiniDumpWithPrivateReadWriteMemory,
MiniDumpWithDataSegs,
MiniDumpWithHandleData,
MiniDumpWithFullMemoryInfo,
MiniDumpWithThreadInfo,
MiniDumpWithUnloadedModules
*/

  BOOL bWriteDump = pfnMiniDumpWriteDump(
    hProcess,
    dwProcessId,
    hFile,
    //MiniDumpNormal,
    (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules),
    &mei,
    nullptr,
    &mci);

  if (!bWriteDump)
  {
    // Error writing dump.
    cout << "******* Error writing dump *******" << endl;
    Sleep(3000);
    return;
  }

  char errorMessage[CFG_TEMP_STRING_MAX_SIZE];
  char directory[CFG_TEMP_STRING_MAX_SIZE];

  directory[0] = 0;

  GetCurrentDirectory(CFG_TEMP_STRING_MAX_SIZE, directory);

  sprintf_s(errorMessage, CFG_TEMP_STRING_MAX_SIZE,
            "A dump file has been generated! \n"
            "Please send the dump file by mail to support@logscrutinizer.com, and we will try to correct the bug and send you an updated version of this software.\n"
            "You find the dump file\n"
            "<%s>\n"
            "in this folder\n"
            "<%s>\n"
            "A backup of your workspace was also saved in that same folder with same name, but with *.lsz as extension",
            zipFile.toLatin1().constData(),
            directory);

  MessageBox(nullptr, errorMessage, "LogScrutinizer has crashed", MB_OK | MB_ICONERROR | MB_TOPMOST);

  // Close file
  CloseHandle(hFile);

  // Unload dbghelp.dll
  FreeLibrary(hDbgHelp);

  // zip it
  /*
  HZIP hz = CreateZip((void *)zipFile.toLatin1().constData(), 0, ZIP_FILENAME);

    if (hz)
    {
        ZRESULT zr = ZipAdd(hz, dumpFile.toLatin1().constData(), (void *)dumpFile.toLatin1().constData(), 0, ZIP_FILENAME);
        CloseZip(hz);
  }
  */

  ::DeleteFile(dumpFile.toLatin1().constData());

  CFGCTRL_SaveWorkspaceFile(workspaceFile, true);
}

// Structured exception handler
int32_t WINAPI CCrashHandler::SehHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{
  // Write minidump file
  CreateMiniDump(pExceptionPtrs);

  // Terminate process
  TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);

  // Unreacheable code
  return EXCEPTION_EXECUTE_HANDLER;
}

void __cdecl CCrashHandler::GetExceptionAndTerminate()
{
extern void CProgressMgr_Abort(void);

  CProgressMgr_Abort();

  // Retrieve exception information
  EXCEPTION_POINTERS* pExceptionPtrs = nullptr;
  GetExceptionPointers(0, &pExceptionPtrs);

  // Write minidump file
  CreateMiniDump(pExceptionPtrs);

  // Terminate process
  TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);

}
// CRT terminate() call handler
void __cdecl CCrashHandler::TerminateHandler()
{
  // Abnormal program termination (terminate() function was called)
  GetExceptionAndTerminate();

}

// CRT unexpected() call handler
void __cdecl CCrashHandler::UnexpectedHandler()
{
  // Unexpected error (unexpected() function was called)
  GetExceptionAndTerminate();
}

// CRT Pure virtual method call handler
void __cdecl CCrashHandler::PureCallHandler()
{
  // Pure virtual function call
  GetExceptionAndTerminate();

}


// CRT invalid parameter handler
void __cdecl CCrashHandler::InvalidParameterHandler(
  const wchar_t* expression,
  const wchar_t* function,
  const wchar_t* file,
  unsigned int line,
  uintptr_t pReserved)
{
  pReserved;

  // Invalid parameter exception
  GetExceptionAndTerminate();
}


// CRT new operator fault handler
int __cdecl CCrashHandler::NewHandler(size_t)
{
  // 'new' operator memory allocation exception
  GetExceptionAndTerminate();

  // Unreacheable code
  return 0;
}

// Handles Win32 exceptions (C structured exceptions) as C++ typed exceptions
void CCrashHandler::SetWin32ExceptionHandler()
{
  //_set_se_translator(se_translator);  //requires /EHa
}

// CRT SIGABRT signal handler
void CCrashHandler::SigabrtHandler(int)
{
  // Caught SIGABRT C++ signal
  GetExceptionAndTerminate();

}

// CRT SIGFPE signal handler
void CCrashHandler::SigfpeHandler(int /*code*/, int subcode)
{
  // Floating point exception (SIGFPE)

  EXCEPTION_POINTERS* pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

  // Write minidump file
  CreateMiniDump(pExceptionPtrs);

  // Terminate process
  TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);

}

// CRT sigill signal handler
void CCrashHandler::SigillHandler(int)
{
  // Illegal instruction (SIGILL)
  GetExceptionAndTerminate();

}

// CRT sigint signal handler
void CCrashHandler::SigintHandler(int)
{
  // Interruption (SIGINT)
  GetExceptionAndTerminate();

}

// CRT SIGSEGV signal handler
void CCrashHandler::SigsegvHandler(int)
{
  // Invalid storage access (SIGSEGV)

  PEXCEPTION_POINTERS pExceptionPtrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;

  // Write minidump file
  CreateMiniDump(pExceptionPtrs);

  // Terminate process
  TerminateProcess(GetCurrentProcess(), CrashHandlerTerminateProcessValue);

}

// CRT SIGTERM signal handler
void CCrashHandler::SigtermHandler(int)
{
  // Termination request (SIGTERM)
  GetExceptionAndTerminate();

}

// Define a global int to keep track of how many assertion failures occur.
int gl_num_asserts=0;

// Reporting function. Will hook it into the debug reporting
// process later using _CrtSetReportHook.
int ReportingHookFunction( int reportType, char *userMessage, int *retVal )
{
    // Redirect stdout to file
    FILE* pCrtDumpFile;
    string CrtDumpFile = "CRT_Dump.txt";

    errno_t error = freopen_s(&pCrtDumpFile, CrtDumpFile.c_str(), "w", stdout);

    if (!pCrtDumpFile)
    {
      return 0;
    }
    else
    {
       /*
        * When the report type is for an ASSERT,
        * Will report some information, but also want _CrtDbgReport to get called -
        * so return TRUE.
        *
        * When the report type is a WARNING or ERROR,
        * it will take care of all of the reporting. It don't
        * want _CrtDbgReport to get called so it will return FALSE.
        */
       if ((reportType == _CRT_ASSERT) || (reportType == _CRT_ERROR))
       {
          gl_num_asserts++;
          fprintf(pCrtDumpFile, "Number of Assertion failures that have occurred: %d \n", gl_num_asserts);
          fprintf(pCrtDumpFile, "Assertion failed at: %s \n", userMessage);
          fprintf(pCrtDumpFile, "Returning TRUE from the defined reporting function.\n");
          fflush(pCrtDumpFile);
          // CRT_ASSERT & CRT_ERROR handler
          CCrashHandler::GetExceptionAndTerminate();
          return 1;
       }
       else
       {
          fprintf(pCrtDumpFile, "Assertion failed at: %s \n", userMessage);
          fprintf(pCrtDumpFile, "Returning FALSE from the defined reporting function.\n");
          fflush(pCrtDumpFile);
          // CRT_WARNING handler
          // Retrieve exception information
          EXCEPTION_POINTERS* pExceptionPtrs = nullptr;
          CCrashHandler::GetExceptionPointers(0, &pExceptionPtrs);
          // Write minidump file
          CCrashHandler::CreateMiniDump(pExceptionPtrs);
          return 0;
       }
    }
}

void CrtSetReportHook()
{
  // CRT Dump coming from CRTDBG
  /*
   * Hook in defined reporting function.
   * Every time a _CrtDbgReport is called to generate
   * a debug report, CRT reporting function will get called first.
   */
  _CrtSetReportHook(ReportingHookFunction);

  /*
   * Define the report destination(s) for each type of report
   * it is going to generate.  In this case, it is going to
   * generate a report for every report type: _CRT_WARN,
   * _CRT_ERROR, and _CRT_ASSERT.
   * The destination(s) is defined by specifying the report mode(s)
   * and report file for each report type.
   * This program sends all report types to pCrtDumpFile.
   */
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
}

// Exception translator function
void se_translator( unsigned int u, EXCEPTION_POINTERS* pExp )
{
  CCrashHandler::m_isStrategyChange = true;
  throw runtime_error("Win32 exception during test case execution");
}
