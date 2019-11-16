/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include "CXMLBase.h"
#include "CDebug.h"
#include "quickFix.h"

/***********************************************************************************************************************
*   SearchElementAttribute
***********************************************************************************************************************/
bool CXMLBase::SearchElementAttribute(const char *elementName_p, const char *attributeName_p,
                                      char *value_p, const int maxValueLength)
{
    m_state = READ_XML_HEADER_STATE;

    if (!ReadXMLHeader()) {
        return false;
    }

    return SearchNextElementAttribute(elementName_p, attributeName_p, value_p, maxValueLength);
}

/***********************************************************************************************************************
*   SearchNextElementAttribute
***********************************************************************************************************************/
bool CXMLBase::SearchNextElementAttribute(const char *elementName_p, const char *attributeName_p,
                                          char *value_p, const int maxValueLength)
{
    bool isInsideElement = false;
    bool status = true;

    while (status) {
        XML_Token_t token;
        status = SearchNextToken(&token);
        if (status) {
            switch (token)
            {
                case ELEMENT_START_TOKEN:

                    isInsideElement = false;
                    status = GetName();
                    if (status) {
                        if (strcmp(m_tempString, elementName_p) == 0) {
                            isInsideElement = true;
                        }
                    }
                    break;

                case ELEMENT_ATTRIBUTE_TOKEN:
                    status = GetAttribute();

                    if (status && isInsideElement) {
                        if (strcmp(attributeName_p, m_tempString) == 0) {
                            int length = static_cast<int>(strlen(m_tempAttribute));

                            if (length > (maxValueLength - 1)) {
                                length = maxValueLength - 1;
                            }

                            memcpy(value_p, m_tempAttribute, static_cast<size_t>(length));
                            value_p[length] = 0;

                            return true;
                        }
                    }
                    break;

                default:
                    isInsideElement = false;
                    break;
            }
        }
    }

    return false;
}

/***********************************************************************************************************************
*   Parse
***********************************************************************************************************************/
void CXMLBase::Parse(char *document_p, int size)
{
    bool result = true;

    m_document_p = document_p;
    m_documentSize = size;

    m_ref_p = m_document_p;
    m_ref_end_p = m_ref_p + m_documentSize;
    m_error = false;

    m_state = READ_XML_HEADER_STATE;

    Init();

    while (result && (m_state != END_STATE) && !m_error) {
        switch (m_state)
        {
            case READ_XML_HEADER_STATE:
                result = ReadXMLHeader();
                if (result) {
                    m_state = READ_ELEMENT_NAME;
                } else {
                    return;
                }
                break;

            case READ_ELEMENT_NAME:
                result = GetNextElement();
                break;

            case READ_ELEMENT_ATTRIBUTES_STATE:
                break;

            default:
                break;
        }
    }
}

/* This if function leave m_ref_p point to the next letter after the xml header */
bool CXMLBase::ReadXMLHeader(void)
{
    /*<?xml version="1.0" encoding="utf-8" standalone="yes"?> */

    if (m_documentSize < static_cast<int>(strlen("<?xml version=\"1.0\"?>")) + 1) {
        TRACEX_W("XML start marker <?xml not found")
        return false;
    }

    while (m_ref_end_p != m_ref_p && *m_ref_p != '<') {
        ++m_ref_p;
    }

    if (m_ref_end_p == m_ref_p) {
        return false;
    }

    if ((*m_ref_p++ == '<') &&
        (*m_ref_p++ == '?') &&
        (*m_ref_p++ == 'x') &&
        (*m_ref_p++ == 'm') &&
        (*m_ref_p++ == 'l')) {
        while (m_ref_end_p != m_ref_p) {
            if ((*m_ref_p++ == '?') && (m_ref_end_p > m_ref_p) && (*m_ref_p++ == '>')) {
                return true;
            }
        }

        TRACEX_W("XML end marker ?> not found")
        return false;
    } else {
        TRACEX_W("XML start marker <?xml not found")
        return false;
    }
}

/***********************************************************************************************************************
*   GetNextElement
***********************************************************************************************************************/
bool CXMLBase::GetNextElement(void)
{
    XML_Token_t token;
    bool status;

    if (m_error) {
        return false;
    }

    if (SearchNextToken(&token)) {
        switch (token)
        {
            case ELEMENT_START_TOKEN:
                status = GetName();
                if (status) {
                    ++m_elementLevel;
                    ElementStart(m_tempString); /* Callback */
                    return true;
                }
                break;

            case ELEMENT_ATTRIBUTE_TOKEN:
                status = GetAttribute();
                if (status) {
                    Element_Attribute(m_tempString, m_tempAttribute);
                    return true;
                }
                break;

            case ELEMENT_END_TOKEN:
                --m_elementLevel;
                ElementEnd();
                return true;

            case NO_TOKEN:
                XML_Error();
                break;
        }
    }

    return false;
}

/***********************************************************************************************************************
 *   GetAttribute
 * output: m_ref_p will point at next letter after the name
 *        m_tempString will contain the name
 **********************************************************************************************************************/
bool CXMLBase::GetAttribute(void)
{
    bool stop;
    int index;

    m_tempString[0] = 0;
    m_tempAttribute[0] = 0;

    /* First loop until we reach the last space marker before the start ch of the attribute */
    while (*m_ref_p++ != ' ') {}

    /* Loop through the text and extract the string of the attribute name */
    stop = false;
    index = 0;
    while (m_ref_end_p != m_ref_p && !stop) {
        if (*m_ref_p == '=') {
            m_tempString[index] = 0;
            stop = true;
        } else {
            /* trim away all trail and tail spaces */
            if (*m_ref_p != ' ') {
                m_tempString[index] = *m_ref_p;
                ++index;
            }

            ++m_ref_p;
        }
    }

    /* Loop until we reach the last space marker before the start ch of the attribute */
    while ((m_ref_end_p != m_ref_p) && (*m_ref_p++ != '"')) {}

    /* If the while loop ended because the there where no more letters we quit */
    if (*(m_ref_p - 1) != '"') {
        return false;
    }

/* Loop through the text and extract the string of the attribute value
 * Get the following   '234"','234  "' */

    stop = false;
    index = 0;

    while (m_ref_end_p != m_ref_p && !stop) {
        if ((*m_ref_p == '"') && (*(m_ref_p - 1) != '\\')) {
            m_tempAttribute[index] = 0;
            return true;
        } else {
            /* allow attributes containing " if these are preceded with \  */

            if (((*(m_ref_p) != '"') || (*(m_ref_p - 1) == '\\')) &&   /* add the " only if there is a \ before */
                ((*(m_ref_p) != '\\') || (*(m_ref_p + 1) != '"'))) {
                /* add the \ only if there isn't a trailing " */
                m_tempAttribute[index] = *m_ref_p;
                ++index;
            }

            ++m_ref_p;
        }
    }

    return false;
}

/* output: m_ref_p will point at next letter after the name
 *         m_tempString will contain the name */
bool CXMLBase::GetName(void)
{
    int index = 0;
    m_tempString[0] = 0;

    /* First loop until we reach the < marker
     * while (*m_ref_p++ != '<'); */

    /* Get the following   'Name ', 'Name>' */

    while (m_ref_end_p != m_ref_p) {
        if ((*m_ref_p == ' ') || (*m_ref_p == '>')) {
            m_tempString[index] = 0;

            if (*m_ref_p == '>') {
                ++m_ref_p;
            }
            return true;
        } else {
            m_tempString[index] = *m_ref_p;
            ++index;
            ++m_ref_p;
        }
    }

    return false;
}

/***********************************************************************************************************************
* SearchNextToken
***********************************************************************************************************************/
bool CXMLBase::SearchNextToken(XML_Token_t *token_p)
{
    char *search_p = m_ref_p;

    while (m_ref_end_p != search_p) {
        switch (*search_p++)  /*search_p points to next letter after switch */
        {
            case '<':
                if (*search_p == '/') {
                    /* </Element>  return m_ref_p after the token such that next Element can be fetched */
                    *token_p = ELEMENT_END_TOKEN;

                    while (m_ref_end_p != search_p) {
                        if (*search_p++ == '>') {
                            m_ref_p = search_p;
                            return true;
                        }
                    }

                    return false;
                } else {
                    m_ref_p = search_p;
                    *token_p = ELEMENT_START_TOKEN;                     /* <Element ,  returns m_ref_p after '<' */
                    return true;
                }

            case '/':
                if (*(search_p) == '>') {
                    m_ref_p = ++search_p;
                    *token_p = ELEMENT_END_TOKEN;                       /* /> Return m_ref_P after > */
                    return true;
                } else {
                    *token_p = NO_TOKEN;                                /* ERROR */
                    return false;
                }

            case '=':

                /*  Attribute = "Value", return m_ref_p after =, should be either space or " */
                *token_p = ELEMENT_ATTRIBUTE_TOKEN;
                return true;

            case '?':
                return false;

            case '>':                                                /* Element>    No action */
            default:
                break;
        } /* switch */
    }
    return false;
}

/***********************************************************************************************************************
*   XML_Error
***********************************************************************************************************************/
void CXMLBase::XML_Error(void)
{
    TRACEX_W("XML ERROR, end marker ?> not found")

    m_error = true;
}

#ifdef _DEBUG

static char doc1[] = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>"
                     "<TextAnalysisTool.NET version=\"2006-12-04\" showOnlyFilteredLines=\"False\"><filters>"
                     "<filter enabled=\"y\" excluding=\"n\" color=\"ff0000\" type=\"matches_text\" case_sensitive=\"n\" "
                     "regex=\"n\" text=\"hickup din lille gris pis && \" /> <filter enabled=\"y \" excluding=\"  n\" "
                     "color   =\" ff0000\" type=\"  matches_text \" case_sensitive  =  \"n\" regex=\"n\" "
                     "text=\"hickup\" /> </filters></TextAnalysisTool.NET>";

/***********************************************************************************************************************
*   Start
***********************************************************************************************************************/
void CXMLBase_Test::Start(void)
{
    Parse(doc1, static_cast<int>(strlen(doc1) + 1));

    SetDocument(doc1, static_cast<int>(strlen(doc1)));

    char tempi[256];
    bool found = false;

    if (SearchElementAttribute("TextAnalysisTool.NET", "version", tempi, 256)) {
        if (strcmp(tempi, "2006-12-04") != 0) {
            TRACEX_E("XML ERROR")
        }
    }

    while (!found && SearchNextElementAttribute("filter", "text", tempi, 256)) {
        if (strcmp(tempi, "hickup") == 0) {
            found = true;
        }
    }

    if (!found) {
        TRACEX_E("XML ERROR")
    }
}

/***********************************************************************************************************************
*   ElementStart
***********************************************************************************************************************/
void CXMLBase_Test::ElementStart(char *name_p)
{
    TRACEX_DE("Enter - %s Level:%d", name_p, m_elementLevel)
}

/***********************************************************************************************************************
*   ElementEnd
***********************************************************************************************************************/
void CXMLBase_Test::ElementEnd(void)
{
    TRACEX_DE("\nLeave")
}

/***********************************************************************************************************************
*   Element_Attribute
***********************************************************************************************************************/
void CXMLBase_Test::Element_Attribute(char *name_p, char *value_p)
{
    TRACEX_DE("%s = %s   ", name_p, value_p)
}

/***********************************************************************************************************************
*   Element_Value
***********************************************************************************************************************/
void CXMLBase_Test::Element_Value(char *value_p)
{
    TRACEX_DE("(Value=%s)", value_p)
}

#endif
