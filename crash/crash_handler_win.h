#ifndef _CRASH_HANDLER_H
#define _CRASH_HANDLER_H

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif

#include <stdint.h>
#include <string>

class CCrashHandler  
{
public:

  /// Constructor
  CCrashHandler();

  /// Destructor
  virtual ~CCrashHandler();

  /// Sets exception handlers that work on per-process basis
  void SetProcessExceptionHandlers();

  /// Installs C++ exception handlers that function on per-thread basis
  void SetThreadExceptionHandlers();

// QT_TODO
#ifdef _WIN32
  /// Collects current process state.
  static void GetExceptionPointers(uint32_t dwExceptionCode, EXCEPTION_POINTERS** pExceptionPointers);

  /// This method creates minidump of the process
  static void CreateMiniDump(EXCEPTION_POINTERS* pExcPtrs);

  /// Exception handler functions.
  static int32_t WINAPI SehHandler(PEXCEPTION_POINTERS pExceptionPtrs);
  static void __cdecl GetExceptionAndTerminate();
  static void __cdecl TerminateHandler();
  static void __cdecl UnexpectedHandler();

  static void __cdecl PureCallHandler();

  static void __cdecl InvalidParameterHandler(
      const wchar_t* expression,
      const wchar_t* function,
      const wchar_t* file,
      unsigned int line,
      uintptr_t pReserved);

  static int __cdecl NewHandler(size_t);
  static void SetWin32ExceptionHandler();
#endif

  static void SigabrtHandler(int);
  static void SigfpeHandler(int /*code*/, int subcode);
  static void SigintHandler(int);
  static void SigillHandler(int);
  static void SigsegvHandler(int);
  static void SigtermHandler(int);

  static bool m_isStrategyChange;
};

#ifdef _WIN32
// Functionality to handle CRT asserts, errors and warnings
void CrtSetReportHook();
int ReportingHookFunction( int reportType, char *userMessage, int *retVal );

// Exception translator function
void se_translator( int, EXCEPTION_POINTERS* );
#endif

#endif // _CRASH_HANDLER_H

