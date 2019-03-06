/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#pragma once

#include "CConfig.h"

typedef enum {
    NO_STATE = 0,
    READ_XML_HEADER_STATE,
    READ_ELEMENT_NAME,
    READ_ELEMENT_ATTRIBUTES_STATE,
    END_STATE = 100
} XML_ParserState_t;

typedef enum {
    NO_TOKEN,
    ELEMENT_START_TOKEN,

    /**< <Element */
    ELEMENT_END_TOKEN,

    /**< </Element,  /> */
    ELEMENT_ATTRIBUTE_TOKEN,

    /**< Atrribute= "Value" */
}XML_Token_t;

/***********************************************************************************************************************
*   CXMLBase
***********************************************************************************************************************/
class CXMLBase : public QObject
{
    Q_OBJECT

public:
    CXMLBase() {m_state = NO_STATE;}
    ~CXMLBase(void) {}

    void Parse(char *document_p, int size);

    /***********************************************************************************************************************
    *   SetDocument
    ***********************************************************************************************************************/
    void SetDocument(char *document_p, size_t size)
    {
        m_document_p = document_p;
        m_documentSize = size;
        m_ref_p = m_document_p;
        m_ref_end_p = m_ref_p + m_documentSize;
    }

    /* Find first element with identity elementName, and then extract attribute with identity attributeName.
     * Store the attribute value into value_p (out) (allocated by caller)
     * The value_p string may contain maximum maxValueLength letters */
    bool SearchElementAttribute(const char *elementName_p, const char *attributeName,
                                char *value_p, const int maxValueLength);

    /* As above but requires that the current references are setup and that the XML header is parsed allready */
    bool SearchNextElementAttribute(const char *elementName_p, const char *attributeName_p,
                                    char *value_p, const size_t maxValueLength);

protected:
    virtual void Init(void) = 0;
    virtual void ElementStart(char *name_p) = 0;
    virtual void ElementEnd(void) = 0;
    virtual void Element_Attribute(char *name_p, char *value_p) = 0;
    virtual void Element_Value(char *value_p) = 0;

    void XML_Error(void); /**< Call this to abort the parse, when an error has been detected */

    char *m_document_p; /**< Text file to parse XML elements */
    size_t m_documentSize;
    int m_elementLevel; /**< In which hierachy we are at */

private:
    bool GetNextElement(void);
    bool SearchNextToken(XML_Token_t *token_p);
    bool ReadXMLHeader(void);
    bool GetName(void);
    bool GetAttribute(void);

    XML_ParserState_t m_state;
    char *m_ref_p; /**< Current parser location in m_document_p */
    char *m_ref_end_p; /**< Last position in the m_document_p */
    char m_tempString[CFG_TEMP_STRING_MAX_SIZE];
    char m_tempAttribute[CFG_TEMP_STRING_MAX_SIZE];
    bool m_error;
};

#ifdef _DEBUG

/***********************************************************************************************************************
*   CXMLBase_Test
***********************************************************************************************************************/
class CXMLBase_Test : public CXMLBase
{
public:
    CXMLBase_Test() {}
    ~CXMLBase_Test(void) {}
    void Start(void);

protected:
    virtual void Init(void) {}
    virtual void ElementStart(char *name_p);
    virtual void ElementEnd(void);
    virtual void Element_Attribute(char *name_p, char *value_p);
    virtual void Element_Value(char *value_p);
};

#endif
