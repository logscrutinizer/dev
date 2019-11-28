/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "plugin_api.h"

/***********************************************************************************************************************
*   CPlugin_Example_2
***********************************************************************************************************************/
class CPlugin_Example_2 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_2();
};

/***********************************************************************************************************************
*   Q87_Decoder
***********************************************************************************************************************/
class Q87_Decoder : public CDecoder
{
public:
    Q87_Decoder();

    virtual bool pvDecode(char *row_p, int *length_p, const int maxLength);
};
