/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "plugin_text_parser.h"

/*----------------------------------------------------------------------------------------------------------------------
 * */
inline bool isDigit(uint8_t ch)
{
    if (((ch >= '0') && (ch <= '9')) || (ch == '-')) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
inline bool isDoubleDigit(uint8_t ch)
{
    if (((ch >= '0') && (ch <= '9')) ||
        (ch == '-') ||
        (ch == '.')) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
inline bool isHexDigit(int ch)
{
    if (((ch >= '0') && (ch <= '9')) ||
        ((ch >= 'A') && (ch <= 'F')) ||
        ((ch >= 'a') && (ch <= 'f')) ||
        (ch == '-') ||
        (ch == '.') ||
        (ch == 'x') ||
        (ch == 'X')) {
        return true;
    } else {
        return false;
    }
}

#define MAX_INT_STRING 24

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CTextParser::ParseInt(int *value_p)
{
    char value[MAX_INT_STRING] = "";
    const char *loopText_p = &m_text_p[m_parseIndex];
    const char *loopTextEnd_p = &m_text_p[m_textLength];
    const char *startpoint_p = loopText_p;

    while (loopText_p < loopTextEnd_p && !isDigit(*loopText_p)) {
        ++loopText_p;
    }

    if (loopText_p == loopTextEnd_p) {
        return false;
    }

    int index = 0;
    while (loopText_p < loopTextEnd_p && index < (MAX_INT_STRING - 1) && isDigit(*loopText_p)) {
        value[index] = *loopText_p;
        ++loopText_p;
        ++index;
    }

    m_parseIndex += (int)(loopText_p - startpoint_p);
    value[index] = 0;
    *value_p = strtol(value, nullptr, 10);

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CTextParser::ParseHexInt(unsigned long *value_p)
{
    char value[MAX_INT_STRING] = "";
    const char *loopText_p = &m_text_p[m_parseIndex];
    const char *loopTextEnd_p = &m_text_p[m_textLength];
    const char *startpoint_p = loopText_p;

    while (loopText_p < loopTextEnd_p && !isHexDigit(*loopText_p)) {
        ++loopText_p;
    }

    if (loopText_p == loopTextEnd_p) {
        return false;
    }

    int index = 0;
    while (loopText_p < loopTextEnd_p && index < (MAX_INT_STRING - 1) && isHexDigit(*loopText_p)) {
        value[index] = *loopText_p;
        ++loopText_p;
        ++index;
    }

    m_parseIndex += (int)(loopText_p - startpoint_p);
    value[index] = 0;
    *value_p = strtoul(value, nullptr, 16);

    return true;
}

#define MAX_INT64_STRING 21

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CTextParser::Parse_INT64(int64_t *value_p)
{
    char value[25] = "";
    const char *loopText_p = &m_text_p[m_parseIndex];
    const char *loopTextEnd_p = &m_text_p[m_textLength];
    const char *startpoint_p = loopText_p;

    while (loopText_p < loopTextEnd_p && !isDigit(*loopText_p)) {
        ++loopText_p;
    }

    if (loopText_p == loopTextEnd_p) {
        return false;
    }

    int index = 0;
    while (loopText_p < loopTextEnd_p && index < (MAX_INT64_STRING - 1) && isDigit(*loopText_p)) {
        value[index] = *loopText_p;
        ++loopText_p;
        ++index;
    }

    m_parseIndex += (int)(loopText_p - startpoint_p);

    value[index] = 0;

#ifdef _WIN32
    *value_p = _atoi64(value);
#else
    *value_p = atoll(value);
#endif

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CTextParser::ParseDouble(double *value_p)
{
    char value[25] = "";
    const char *loopText_p = &m_text_p[m_parseIndex];
    const char *loopTextEnd_p = &m_text_p[m_textLength];
    const char *startpoint_p = loopText_p;

    while (loopText_p < loopTextEnd_p && !isDoubleDigit(*loopText_p)) {
        ++loopText_p;
    }

    if (loopText_p == loopTextEnd_p) {
        return false;
    }

    int index = 0;
    while (loopText_p < loopTextEnd_p && index < (MAX_INT64_STRING - 1) && isDoubleDigit(*loopText_p)) {
        value[index] = *loopText_p;
        ++loopText_p;
        ++index;
    }

    m_parseIndex += (int)(loopText_p - startpoint_p);
    value[index] = 0;

#ifdef _WIN32  /* LINUX_TODO */
    _CRT_FLOAT floatVal;
    int status = _atoflt(&floatVal, value);
    *value_p = floatVal.f;
    return status == 0 ? true : false;
#else
    *value_p = atof(value);
    return true;
#endif
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CTextParser::Extract(int startIndex, int endIndex, char *value_p)
{
#ifdef _DEBUG
    if ((startIndex >= m_textLength) || (endIndex >= m_textLength) || (startIndex > endIndex)) {
        value_p[0] = 0;
        ErrorHook("CTextParser::Extract  Bad input parameters\n");
        return;
    }
#endif

    memcpy(value_p, &m_text_p[startIndex], endIndex - startIndex + 1);
    value_p[endIndex - startIndex + 1] = 0;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CTextParser::Search(const char *match_p, int matchLength, int limitSearchOffset)
{
    int textLength;

#ifdef _DEBUG
    if ((match_p == nullptr) || (matchLength == 0) || (m_textLength == 0) || (m_text_p == 0)) {
        ErrorHook("CTextParser::Extract  Bad input parameters\n");
        return false;
    }
#endif

    int parseIndex = m_parseIndex;
    m_isSearchMatch = false;

    if ((limitSearchOffset != -1) && ((limitSearchOffset + parseIndex) < m_textLength)) {
        textLength = limitSearchOffset + parseIndex;
    } else {
        textLength = m_textLength;  /* Move it to a local variable for speed */
    }

    /* Loop over each letter in the m_text_p */

    matchLength--;  /* to make it 0 index based */

    for ( ; parseIndex < textLength && (matchLength <= (textLength - parseIndex)); ++parseIndex) {
        const char *loopText_p = &m_text_p[parseIndex]; /* Direct reference to the character in the loop evaluation */
        const char *loopSearch_p = match_p;
        int textIndex = parseIndex;   /* Index to the start/next letter in the current string */
        int searchIndex = 0;
        bool loop = true;

        /* Loop evaluation, from a letter in the text string see if the filter matches... loop the filter and the
         * text together See if the filter fits well from the current char and on */
        while ((*loopSearch_p == *loopText_p) && loop) {
            /* If we have a match for all the letters in the filter then it was success */
            if (searchIndex == matchLength) {
                m_searchMatch_StartIndex = parseIndex;
                m_parseIndex = parseIndex + matchLength + 1;
                m_searchMatch_EndIndex = m_parseIndex;
                m_isSearchMatch = true;
                return true;
            }

            ++searchIndex;

            if (textIndex == textLength) {
                loop = false;
            }

            ++textIndex;
            ++loopSearch_p;
            ++loopText_p;
        }
    }

    return false;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
