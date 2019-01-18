//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// File: dll_plugin.cpp
//
// Description: This is Plugin Example 2
//              It shows how to create a simple plugin that decode rows and
//              alter the row content, a Decoder
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include "plugin_api.h"
#include "plugin_utils.h"
#include "plugin_text_parser.h"
#include "dll_plugin.h"

#define Q87_MATCH  "Time:"

CPlugin_DLL_API* DLL_API_Factory(void)
{
    return (CPlugin_DLL_API*)new CPlugin_Example_2;
}

//----------------------------------------------------------------------------------------------------------------------

CPlugin_Example_2::CPlugin_Example_2()
{
    SetPluginName("Plugin Example 2");
    SetPluginVersion("v1.0");
    SetPluginAuthor("Robert Klang");

    SetPluginFeatures(SUPPORTED_FEATURE_DECODER | SUPPORTED_FEATURE_HELP_URL);

    RegisterDecoder(new Q87_Decoder());
}

//----------------------------------------------------------------------------------------------------------------------
//------------------------- TP274:  DECODER ------------------------------------
//----------------------------------------------------------------------------------------------------------------------
Q87_Decoder::Q87_Decoder() :
    CDecoder(Q87_MATCH, 0)
{

}

//----------------------------------------------------------------------------------------------------------------------
bool Q87_Decoder::pvDecode(char* row_p, unsigned int* length_p,
    const unsigned int maxLength)
{
    CTextParser parser;

    parser.SetText(row_p, *length_p);

    // Log string:   Time:0 Value:140

    // The parser member function search will return true if there is a match
    // with the Time: string. It will also place the parser just
    // after the match.

    if (parser.Search(Q87_MATCH, 5)) {
        if (parser.Search("Value:", 6)) {
            int value;

            if (!parser.ParseInt(&value)) return false;

            unsigned int startIndex = parser.GetParseIndex();

            // Add the following to the line,   <DECODED> %-4.2f
            sprintf(&row_p[startIndex - 1],
                "  <DECODED>  %-4.2f", (float)value / (float)128.0);

            *length_p = (unsigned int)strlen(row_p);

            return true;
        }
    }

    return false;
}
