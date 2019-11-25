/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#pragma once

#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
 #include <windows.h>
#endif

#include <stdint.h>
#include <string>

/***********************************************************************************************************************
*   CCrashHandler
***********************************************************************************************************************/
class CCrashHandler
{
public:
    CCrashHandler();
    virtual ~CCrashHandler();

    /* Sets exception handlers that work on per-process basis */
    void SetProcessExceptionHandlers();

    /* Installs C++ exception handlers that function on per-thread basis */
    void SetThreadExceptionHandlers();

/* QT_TODO */
#ifdef _WIN32

    /*/ Collects current process state. */
    static void GetExceptionPointers(uint32_t dwExceptionCode, EXCEPTION_POINTERS **pExceptionPointers);

    /*/ This method creates minidump of the process */
    static void CreateMiniDump(EXCEPTION_POINTERS *pExcPtrs);

    /*/ Exception handler functions. */
    static int32_t WINAPI SehHandler(PEXCEPTION_POINTERS pExceptionPtrs);
    static void __cdecl GetExceptionAndTerminate();
    static void __cdecl PureCallHandler();
    static void __cdecl InvalidParameterHandler(const wchar_t *expression,
                                                const wchar_t *function,
                                                const wchar_t *file,
                                                int line,
                                                uintptr_t pReserved);
    static int __cdecl NewHandler(size_t);
    static void SetWin32ExceptionHandler();
#endif

    [[ noreturn ]] static void SigabrtHandler(int);
    [[ noreturn ]] static void SigfpeHandler(int /*code*/, int subcode);
    [[ noreturn ]] static void SigintHandler(int);
    [[ noreturn ]] static void SigillHandler(int);
    [[ noreturn ]]  static void SigsegvHandler(int);
    [[ noreturn ]] static void SigtermHandler(int);
    [[ noreturn ]] static void TerminateHandler();
    [[ noreturn ]] static void UnexpectedHandler();

    static bool m_isStrategyChange;
};

#ifdef _WIN32

/* Functionality to handle CRT asserts, errors and warnings */
void CrtSetReportHook();
int ReportingHookFunction(int reportType, char *userMessage, int *retVal);

/* Exception translator function */
void se_translator(int, EXCEPTION_POINTERS *);
#endif

#endif /* CRASH_HANDLER_H */
