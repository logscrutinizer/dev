/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include <stdio.h>
#include <string.h>

#include "CDebug.h"
#include "plugin_text_parser.h"
#include "utils.h"

#define MAX_TEST_STRING_LENGTH        512

/***********************************************************************************************************************
*   CParseTest_Base
***********************************************************************************************************************/
class CParseTest_Base
{
public:
    CParseTest_Base(void) {m_setup = false;}

    /****/
    virtual void BaseReset(const char *testInput)
    {
        m_setup = true;

        m_parser.ResetParser();
        m_parser.SetText(testInput, (int)strlen(testInput));
        SAFE_STR_MEMCPY(m_testInput, MAX_TEST_STRING_LENGTH, testInput, strlen(testInput));
    }

    /****/
    bool CheckSetup(void)
    {
        if (m_setup) {m_setup = false; return true;}

        TRACEX_E("CParseTest_Base::CheckSetup  Test class not reinitialized\n")
        return false;
    }

    virtual void Execute(void) = 0;

protected:
    bool m_setup;
    CTextParser m_parser;
    char m_testInput[MAX_TEST_STRING_LENGTH];

private:
};

/***********************************************************************************************************************
*   CParseTest_Extract
***********************************************************************************************************************/
class CParseTest_Extract : CParseTest_Base
{
public:
    CParseTest_Extract(void) {}

    /****/
    virtual void Reset(const char *testInput, int start, int end, const char *expectedResult)
    {
        CParseTest_Base::BaseReset(testInput);

        SAFE_STR_MEMCPY(m_expectedResult, MAX_TEST_STRING_LENGTH, expectedResult, strlen(expectedResult));
        m_start = start;
        m_end = end;
    }

    virtual void Execute(void);

private:
    char m_expectedResult[MAX_TEST_STRING_LENGTH];
    int m_start;
    int m_end;
};

/***********************************************************************************************************************
*   CParseTest_Float
***********************************************************************************************************************/
class CParseTest_Float : CParseTest_Base
{
public:
    CParseTest_Float(void) {}

    /****/
    virtual void Reset(const char *testInput, double expectedResult)
    {
        CParseTest_Base::BaseReset(testInput);
        m_expectedResult = expectedResult;
    }

    virtual void Execute(void) override;

private:
    double m_expectedResult;
};

/***********************************************************************************************************************
*   CParseTest_x64
***********************************************************************************************************************/
class CParseTest_x64 : CParseTest_Base
{
public:
    CParseTest_x64(void) {}

    /****/
    void Reset(const char *testInput, int64_t expectedResult)
    {
        CParseTest_Base::BaseReset(testInput);
        m_expectedResult = expectedResult;
    }

    virtual void Execute(void);

private:
    int64_t m_expectedResult;
};

/***********************************************************************************************************************
*   CParseTest_HexInt
***********************************************************************************************************************/
class CParseTest_HexInt : CParseTest_Base
{
public:
    CParseTest_HexInt(void) {}

    /****/
    void Reset(const char *testInput, unsigned long expectedResult)
    {
        CParseTest_Base::BaseReset(testInput);
        m_expectedResult = expectedResult;
    }

    virtual void Execute(void);

private:
    unsigned long m_expectedResult;
};

/***********************************************************************************************************************
*   CParseTesting
***********************************************************************************************************************/
class CParseTesting
{
public:
    CParseTesting(void) {}
    ~CParseTesting(void) {}

    void ParseTest(void);
};
