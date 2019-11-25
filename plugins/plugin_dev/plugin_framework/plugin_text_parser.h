/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef PLUGIN_TEXT_PARSER_H
 #define PLUGIN_TEXT_PARSER_H

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * File: plugin_text_parser.h
 *
 * Description:
 * The CTextParser class can be used to parse text, e.g. locating a certain part of a string and then extract a value at
 * that index
 *
 *
 * IMPORTANT: DO NOT MODIFY THIS FILE
 *
 * ---------------------------------------------------------------------------------------------------------------------
 * */

/* #CRUSTI-OFF# */

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * Include files
 * ---------------------------------------------------------------------------------------------------------------------
 * */

#include "plugin_utils.h"

/*
 * ---------------------------------------------------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------------------------------------------------------
 * */

class CTextParser
{
public:
    CTextParser() {
        m_text_p = nullptr;
        m_textLength = 0;
        m_parseIndex = 0;
        m_isSearchMatch = false;
        m_searchMatch_StartIndex = 0;
        m_searchMatch_EndIndex = 0;
    }

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: SetText
     * Description: Before the parser can be used a text must be provided. Use this function for that. This function
     * will at the
     *              same time reset the current parse index.
     * Parameters
     *    Input:      text_p,     a character string which shall be parsed
     *    Input:      length,     the length of the character string, exluding the EOL (0) */
    void SetText(const char *text_p, int length)
    {
        m_text_p = text_p;
        m_textLength = length + 1;  /* +1 since the size is not including the 0 char */
        ResetParser();
    }

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: GetText
     * Description: Returns the text used by the parser
     * Parameters
     *    Returns:    The text used by the parser (direct reference, no copy) */
    const char *GetText(void) {return m_text_p;}

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: Search
     * Description: Use this function to located a certain match in the text. After a successful search the parse index
     *              points to the first letter after the match text. After a search e.g. ParseInt could be used to pick
     *              up a number after the matched text.
     *              If the search failes the parse index is restored to where it was before the search request
     * Parameters
     *    Input:       match_p,             reference to the string that shall match (be located)
     *    Input:       matchLength,         length of the string in the match_p reference, exluding EOL (0)
     *    Input:       limitSearchOffset,   if you know that from current parseIndex and forth there cannot be a match
     * beyond this value. -1 means use full string
     *    Returns:     true                 if match, otherwise false */
    bool Search(const char *match_p, int matchLength, int limitSearchOffset = -1);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: ResetParser
     * Description: Sets the current character index to 0. This could be used if a text needs to be "re-parsed" */
    void ResetParser(void) {m_parseIndex = 0;}

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: ParseInt
     * Description: The function parses the text string from the current index, it will skip any character that are not
     * in the
     *              "integer set"   0-9 or -
     *              It will stop parsing characters when characters are not in the "integer set"
     * Parameters
     *    Input/Ouput:  value_p,  the destination for the parsed INT64 value
     *    Returns:      true      if successfull */
    bool ParseInt(int *value_p);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: ParseHexInt
     * Description: The function parses the text string from the current index, it will skip any character that are not
     * in the
     *              "hex integer set"   0-9 or - or . or x or X or A-F or a-f
     *              It will stop parsing characters when characters are not in the "hex integer set"
     * Parameters
     *    Input/Ouput:  value_p,    the destination for the parsed INT64 value
     *    Returns:      true,       if successfull */
    bool ParseHexInt(unsigned long *value_p);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: ParseLongLongInt
     * Description: The function parses the text string from the current index, it will skip any character that are not
     * in the
     *              "integer set"   0-9 or -
     *              It will stop parsing characters when characters are not in the "integer set"
     * Parameters
     *    Input/Ouput:  value_p,  the destination for the parsed INT64 value
     *    Returns:      true,     if successfull */
    bool Parse_INT64(int64_t *value_p);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: ParseDouble
     * Description: The function parses the text string from the current index, it will skip any character that are not
     * in the
     *              "double set"   0-9 or - or .
     *              It will stop parsing characters when characters are not in the "double set"
     * Parameters
     *    Input/Ouput:  value_p,  the destination for the parsed double value
     *    Returns:      true,     if successfull */
    bool ParseDouble(double *value_p);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: Extract
     * Description: Copy characters startIndex to endIndex, into the value_p string
     * Parameters
     *    Input/Output:   value_p,  the destination, must be larger than endIndex - startIndex.
     *    Input:          startIndex, copies from this index
     *    Output:         endIndex, copies until this index (including this index). To copy only letter at index 0, used
     * startIndex = 0, endIndex = 0 */
    void Extract(int startIndex, int endIndex, char *value_p);

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: GetParseIndex
     * Description: Returns the current character index of the parsed string
     * Parameters
     *    Returns:      Current character index of the parsed string */
    inline int GetParseIndex(void) {return m_parseIndex;}

    /*
     * -----------------------------------------------------------------------------------------------------------------
     * Function: SetParseIndex
     * Description:   Set the character index where the parser will continue from at the next operation (such as search)
     * Parameters
     *    Input:    Character index into the string */
    inline void SetParseIndex(int parseIndex) {m_parseIndex = parseIndex;}

private:
    int m_parseIndex; /* While parsing m_text_p this member keep track of where the last parse ended */
    bool m_isSearchMatch; /* If the previous search was successful */
    int m_searchMatch_StartIndex; /* Index to the first character in m_text_p where the match started */
    int m_searchMatch_EndIndex; /* Index to the last character in m_text_p where the match ended */
    const char *m_text_p; /* The text string which is parsed (use SetText to set it) */
    int m_textLength; /* Size of the text string which is parsed */
};

#endif
