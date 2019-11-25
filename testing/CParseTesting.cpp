/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CParseTesting.h"

/***********************************************************************************************************************
*   BaseReset
***********************************************************************************************************************/
void CParseTest_Base::BaseReset(const char *testInput)
{
    m_setup = true;

    m_parser.ResetParser();
    m_parser.SetText(testInput, static_cast<int>(strlen(testInput)));
    SAFE_STR_MEMCPY(m_testInput, MAX_TEST_STRING_LENGTH, testInput, strlen(testInput))
}

/***********************************************************************************************************************
*   ParseTest
***********************************************************************************************************************/
void CParseTesting::ParseTest(void)
{
    CParseTest_Extract extractTest;

    extractTest.Reset("Hubba bubba", 0, 4, "Hubba");
    extractTest.Execute();
    extractTest.Reset("Hubba bubba", 4, 6, "a b");
    extractTest.Execute();
    extractTest.Reset("Hubba bubba", 6, 10, "bubba");
    extractTest.Execute();
    extractTest.Reset("Hubba bubba", 1, 1, "u");
    extractTest.Execute();
    extractTest.Reset("Hubba bubba", 2, 1, "");
    extractTest.Execute();

    CParseTest_Float floatTest;

    floatTest.Reset("1.23399997", 1.23399997);
    floatTest.Execute();
    floatTest.Reset("0.000123399997", 0.000123399997);
    floatTest.Execute();
    floatTest.Reset("123.399997", 123.399997);
    floatTest.Execute();
    floatTest.Reset("   123.399997", 123.399997);
    floatTest.Execute();
    floatTest.Reset(" 123.399997   ", 123.399997);
    floatTest.Execute();
    floatTest.Reset(" -123.399997   ", -123.399997);
    floatTest.Execute();

    CParseTest_x64 x64Test;

    x64Test.Reset("72057594037927936", 72057594037927936);
    x64Test.Execute();
    x64Test.Reset("0", 0);
    x64Test.Execute();
    x64Test.Reset("-1000", -1000);
    x64Test.Execute();

    CParseTest_HexInt HexIntTest;

    HexIntTest.Reset("0x0", 0);
    HexIntTest.Execute();
    HexIntTest.Reset("0", 0);
    HexIntTest.Execute();
    HexIntTest.Reset("0x1", 1);
    HexIntTest.Execute();
    HexIntTest.Reset("1", 1);
    HexIntTest.Execute();
    HexIntTest.Reset("0x100", 0x100);
    HexIntTest.Execute();
    HexIntTest.Reset("  0x100", 0x100);
    HexIntTest.Execute();
    HexIntTest.Reset("0x100 ", 0x100);
    HexIntTest.Execute();
    HexIntTest.Reset(" 100 ", 0x100);
    HexIntTest.Execute();
    HexIntTest.Reset("0xffffffff", 0xffffffff);
    HexIntTest.Execute();
    HexIntTest.Reset("0xfffffffe", 0xfffffffe);
    HexIntTest.Execute();
}

/***********************************************************************************************************************
*   Execute
***********************************************************************************************************************/
void CParseTest_Extract::Execute(void)
{
    char resultString[MAX_TEST_STRING_LENGTH];

    CheckSetup();

    m_parser.Extract(m_start, m_end, resultString);

    if ((strlen(m_expectedResult) == 0) && (strlen(resultString) == 0)) {
        return;
    }

    if (strcmp(m_expectedResult, resultString) != 0) {
        TRACEX_E("CParseTest_Extract::Execute  test ERROR  Expected:%s Result:%s", m_expectedResult, resultString)
    }
}

/***********************************************************************************************************************
*   Execute
***********************************************************************************************************************/
void CParseTest_Float::Execute(void)
{
    CheckSetup();

    double value;

    m_parser.ParseDouble(&value);

    if (!almost_equal(value, m_expectedResult)) {
        TRACEX_E(QString("CParseTest_Float::Execute  test ERROR  Expected:%1 Result:%2")
                     .arg(static_cast<double>(m_expectedResult), 'f').arg(static_cast<double>(value), 'f'))
    }
}

/***********************************************************************************************************************
*   Execute
***********************************************************************************************************************/
void CParseTest_x64::Execute(void)
{
    CheckSetup();

    int64_t value;

    m_parser.Parse_INT64(&value);

    if (value != m_expectedResult) {
        TRACEX_E("CParseTest_x64::Execute  test ERROR  Expected:%f Result:%f \n", m_expectedResult, value)
    }
}

/***********************************************************************************************************************
*   Execute
***********************************************************************************************************************/
void CParseTest_HexInt::Execute(void)
{
    CheckSetup();

    unsigned long value;

    m_parser.ParseHexInt(&value);

    if (value != m_expectedResult) {
        TRACEX_E("CParseTest_HexInt::Execute  test ERROR  Expected:%f Result:%f \n", m_expectedResult, value)
    }
}
