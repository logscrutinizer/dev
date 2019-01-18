
#include "plugin_api.h"

class CPlugin_Example_2 : public CPlugin_DLL_API
{
public:
    CPlugin_Example_2();
};

//----------------------------------------------------------------------------------------------------------------------
class Q87_Decoder : public CDecoder
{
public:
    Q87_Decoder();

    virtual bool pvDecode(char* row_p, unsigned int* length_p,
                          const unsigned int maxLength);
};
