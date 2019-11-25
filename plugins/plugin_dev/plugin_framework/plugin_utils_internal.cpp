/***********************************************************************************************************************
** Copyright (C) 2018 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "plugin_utils.h"
#include "plugin_utils_internal.h"

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * ---- Module/Local Constants ----------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * */

#define GRAPHICAL_OBJECT_BYTE_STREAM_MAX_SIZE (1024 * 1000)    /* Max size of one stream is 1MB */

static int g_graphID = 0;

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * ---- Module/Local Varibles ----------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * */
typedef int HWND;
typedef int HANDLE;

static HWND g_hwnd_msgConsumer = 0;
static HANDLE g_h_msgHeap = 0;
char g_tempTraceString[TEMP_TRACE_STRING_SIZE];

/****/
CListObject::~CListObject()
{}

/****/
CGO_Label::~CGO_Label()
{
    if (m_label_p != nullptr) {
        free(m_label_p);
    }
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * ---- Error handling functions -------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * */

#ifndef _WIN32
__attribute__((__format__(__printf__, 1, 0)))
#endif

/***********************************************************************************************************************
*   ErrorHook
***********************************************************************************************************************/
void ErrorHook(const char *errorMsg, ...)
{
    va_list tArgumentPointer;
    char argumentsString[TEMP_TRACE_STRING_SIZE];

    va_start(tArgumentPointer, errorMsg);
#ifdef _WIN32
    vsprintf_s(argumentsString, errorMsg, tArgumentPointer);
#else
    vsprintf(argumentsString, errorMsg, tArgumentPointer);
#endif
    va_end(tArgumentPointer);

    if (g_h_msgHeap != 0) {
        Trace(argumentsString);
    }
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * ---- Messagge handling functions ------------------------------------------------------------------- */
void EnableMsgTrace(int hwnd_msgConsumer, int h_msgHeap)
{
    g_hwnd_msgConsumer = hwnd_msgConsumer;
    g_h_msgHeap = h_msgHeap;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void Trace(const char *pcStr, ...) /* TODO */
{
    Q_UNUSED(pcStr)
#if 0
 #ifndef _DEBUG
    if (g_h_msgHeap == 0) {
        ErrorHook("MsgHeap, no allocated heap", false);
        return;
    }

    if (g_hwnd_msgConsumer == 0) {
        ErrorHook("MsgHeap, no msgConsumer", false);
        return;
    }
 #endif

 #ifdef _WIN32 /* LINUX_TODO */
    va_list tArgumentPointer;
    va_start(tArgumentPointer, pcStr);

    int vslength = _vscprintf(pcStr, tArgumentPointer) + 10;
    char *heapString_p;

  #ifdef _DEBUG
    if (g_h_msgHeap != nullptr) {
        heapString_p = (char *)HeapAlloc(g_h_msgHeap, HEAP_ZERO_MEMORY, vslength);
    } else if ((vslength - 1) < TEMP_TRACE_STRING_SIZE) {
        heapString_p = g_tempTraceString;
    } else {
        ErrorHook("MsgHeap, string too large", false);
        return;
    }
  #else
    heapString_p = (char *)HeapAlloc(g_h_msgHeap, HEAP_ZERO_MEMORY, vslength);
  #endif

    if (heapString_p == nullptr) {
        ErrorHook("MsgHeap, heap full", false);
    }

    vsprintf_s(heapString_p, vslength, pcStr, tArgumentPointer);

    va_end(tArgumentPointer);

  #ifdef _DEBUG
    OutputDebugString(heapString_p); /* This is visible when running Visual Studio, in the output window */
  #endif

    if (g_hwnd_msgConsumer != 0) {
        PostMessage(g_hwnd_msgConsumer, WM_APP_PLUGIN_MSG, reinterpret_cast<WPARAM>(heapString_p), 0);
    }
 #endif
#endif
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CByteStream
 * Description: See plugin_utils.h file
 * ----------------------------------------------------------------------------------------------------------------------
 * */
CByteStream::CByteStream(int size)
{
    m_ref_p = nullptr;
    m_byteStream_p = nullptr;
    m_end_p = nullptr;

    m_usedSize = 0;
    m_totalSize = size > GRAPHICAL_OBJECT_BYTE_STREAM_MAX_SIZE ? GRAPHICAL_OBJECT_BYTE_STREAM_MAX_SIZE : size;

    m_byteStream_p = reinterpret_cast<uint8_t *>(malloc(static_cast<size_t>(m_totalSize)));

    if (m_byteStream_p != nullptr) {
        m_end_p = m_byteStream_p + m_totalSize - 1;
        m_ref_p = m_byteStream_p;
    } else {
        m_totalSize = 0;
        ErrorHook("Out of memory, allocating byte stream");
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CByteStream::~CByteStream()
{
    if (m_byteStream_p != nullptr) {
        free(m_byteStream_p);
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
uint8_t *CByteStream::AddBytes(int size)
{
    int totalSize = size + static_cast<int>(sizeof(ObjectByteStreamHead_t) + sizeof(ObjectByteStreamTail_t));
    uint8_t *temp_ref = m_ref_p;

    /* Check that this add doesn't take us beyond the size of this bytestream */

    if ((m_ref_p + totalSize) > m_end_p) {
        return nullptr;
    }

    /* Also check that the previous object is OK (by looking at the tail), as long as this isn't the first add (then
     * there is nothing before) */
    if ((m_ref_p != m_byteStream_p) &&
        ((reinterpret_cast<ObjectByteStreamTail_t *>(m_ref_p - sizeof(ObjectByteStreamTail_t)))->tag !=
         OBJECT_BYTE_STREAM_TAIL_TAG)) {
        ErrorHook("CByteStream::AddBytes   Corrupt tail at previous object, tail tag doesn't match\n");
        return nullptr;
    }

    auto head_p = reinterpret_cast<ObjectByteStreamHead_t *>(m_ref_p);
    auto tail_p = reinterpret_cast<ObjectByteStreamTail_t *>(m_ref_p + size + sizeof(ObjectByteStreamHead_t));

    head_p->size = static_cast<int16_t>(size);
    head_p->tag = OBJECT_BYTE_STREAM_HEAD_TAG;
    tail_p->tag = OBJECT_BYTE_STREAM_TAIL_TAG;

    m_ref_p += totalSize;
    m_usedSize += totalSize;

    return reinterpret_cast<uint8_t *>((temp_ref + sizeof(ObjectByteStreamHead_t)));
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 *
 * m_ref_p points at next ObjectByteStreamHead_t */
uint8_t *CByteStream::GetBytes(void)
{
    uint8_t *user_data_p = nullptr;
    ObjectByteStreamHead_t *head_p;
    ObjectByteStreamTail_t *tail_p;

    /*
     * First make sure we can at least extract the head and tail (assuming user data size = 0) */

    if ((m_ref_p - m_byteStream_p + static_cast<int>(sizeof(ObjectByteStreamHead_t) + sizeof(ObjectByteStreamTail_t))) >
        m_usedSize) {
        return nullptr;
    }

    /* m_ref_p shall point at the next ObjectByteStreamHead_t, check the tag and extract size */
    head_p = reinterpret_cast<ObjectByteStreamHead_t *>(m_ref_p);

    if (head_p->tag != OBJECT_BYTE_STREAM_HEAD_TAG) {
        ErrorHook("CByteStream::GetBytes   Corrupt head, head tag doesn't match\n");
        return nullptr;
    }

    auto size = (reinterpret_cast<ObjectByteStreamHead_t *>(m_ref_p))->size; /* extract the user size from the head */

    user_data_p = m_ref_p + sizeof(ObjectByteStreamHead_t); /* Move m_ref_p to point to where the data starts */

    /* Verify that user data and tail fit in the byte stream */
    uint8_t *objectEnd_p = user_data_p + size + sizeof(ObjectByteStreamTail_t) - 1;

    if ((objectEnd_p > m_end_p) || (static_cast<int>(objectEnd_p - m_byteStream_p) > m_usedSize)) {
        ErrorHook("CByteStream::GetBytes   Corrupt head, head + size + tail outside byte stream\n");
        return nullptr;
    }

    /*verify tail */
    tail_p = reinterpret_cast<ObjectByteStreamTail_t *>((user_data_p + size));
    if (tail_p->tag != OBJECT_BYTE_STREAM_TAIL_TAG) {
        ErrorHook("CByteStream::GetBytes   Corrupt tail, tail tag doesn't match\n");
        return nullptr;
    }

    m_ref_p = objectEnd_p + 1;

    return user_data_p;
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CByteStreamManager
 * Description: See plugin_utils.h file
 * ----------------------------------------------------------------------------------------------------------------------
 * */
uint8_t *CByteStreamManager::AddBytes(int size)
{
    uint8_t *ref_p;

    if ((ref_p = m_currentByteStream_p->AddBytes(size)) == nullptr) {
        m_currentByteStream_p = new CByteStream(m_allocByteStreamSize);

        if (m_currentByteStream_p == nullptr) {
            return nullptr;
        }

        m_byteStreamList.InsertTail(static_cast<CListObject *>(m_currentByteStream_p));
        ref_p = m_currentByteStream_p->AddBytes(size);
    }

    return ref_p;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
uint8_t *CByteStreamManager::AddBytes_ThreadSafe(int size)
{
#ifdef QT_TODO
    EnterCriticalSection(&m_criticalSection);
#endif

    return AddBytes(size);

#ifdef QT_TODO
    LeaveCriticalSection(&m_criticalSection);
#endif
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
uint8_t *CByteStreamManager::GetBytes(void)
{
    uint8_t *ref_p;

    if ((ref_p = m_currentByteStream_p->GetBytes()) == nullptr) {
        m_currentByteStream_p = reinterpret_cast<CByteStream *>
                                (m_byteStreamList.GetNext(static_cast<CListObject *>(m_currentByteStream_p)));

        if (m_currentByteStream_p == nullptr) {
            return nullptr;
        }

        m_currentByteStream_p->ResetRef();

        ref_p = m_currentByteStream_p->GetBytes();
    }

    return ref_p;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CGraph_Internal::CGraph_Internal(const char *name_p, int subPlotID, int estimatedNumOfObjects)
{
    m_byteStreamManager_p = new CByteStreamManager(static_cast<int>(sizeof(GraphicalObject_t)) * estimatedNumOfObjects);

    m_subPlotID = subPlotID;
    m_numOfObjects = 0;
    m_enabled = true;
    m_id = g_graphID++;

    m_isGraphExtentInitialized = false;
    m_graphExtent.x_max = 0;
    m_graphExtent.x_min = 0;
    m_graphExtent.y_max = 0.0;
    m_graphExtent.y_min = 0.0;

    m_isOverrideColorSet = false;
    m_overrideColor = 0;
    m_overrideLinePattern = GLP_NONE;

    m_property = GRAPH_PROPERTY_NONE;

    memset(m_name, 0, sizeof(m_name));
    if (name_p != nullptr) {
        strncpy(m_name, name_p, MAX_GRAPH_NAME_LENGTH);
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */

CGraph_Internal::~CGraph_Internal()
{
    if (m_byteStreamManager_p != nullptr) {
        delete m_byteStreamManager_p;
    }
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool CGraph_Internal::UpdateExtents(GraphicalObject_t *object_p)
{
    if (m_isGraphExtentInitialized) {
        if (object_p->x1 < m_graphExtent.x_min) {
            ErrorHook("x less than 0");
        }

        if (object_p->x2 > m_graphExtent.x_max) {
            m_graphExtent.x_max = object_p->x2;
        }

        if (object_p->y1 < m_graphExtent.y_min) {
            m_graphExtent.y_min = object_p->y1;
        }

        if (object_p->y2 < m_graphExtent.y_min) {
            m_graphExtent.y_min = object_p->y2;
        }

        if (object_p->y2 > m_graphExtent.y_max) {
            m_graphExtent.y_max = object_p->y2;
        }

        if (object_p->y1 > m_graphExtent.y_max) {
            m_graphExtent.y_max = object_p->y1;
        }
    } else {
        m_isGraphExtentInitialized = true;

        m_graphExtent.x_min = object_p->x1;
        m_graphExtent.x_max = object_p->x2;
        m_graphExtent.y_min = object_p->y1;
        m_graphExtent.y_max = object_p->y2;
    }

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
GraphicalObject_t *CGraph_Internal::GetFirstGraphicalObject(void)
{
    if ((m_numOfObjects == 0) && (m_byteStreamManager_p != nullptr)) {
        return nullptr;
    }

    m_byteStreamManager_p->ResetRef();

    /* This graphical object will need to be converted to the "correct" type, line or box
     * Although the data returned contains the correct number of bytes corresponding to the object
     * stored */
    GraphicalObject_t *go_p = reinterpret_cast<GraphicalObject_t *>(m_byteStreamManager_p->GetBytes());

    return (go_p);
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
GraphicalObject_t *CGraph_Internal::GetNextGraphicalObject(void)
{
    if ((m_numOfObjects == 0) && (m_byteStreamManager_p != nullptr)) {
        return nullptr;
    }

    /* This graphical object will need to be converted to the "correct" type, line or box
     * Although the data returned contains the correct number of bytes corresponding to the object
     * stored */
    GraphicalObject_t *go_p = reinterpret_cast<GraphicalObject_t *>(m_byteStreamManager_p->GetBytes());
    return go_p;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
void CGraph_Internal::GetOverrides(bool *isOverrideColorSet_p, Q_COLORREF *overrideColor_p,
                                   GraphLinePattern_e *m_overrideLinePattern_p)
{
    *isOverrideColorSet_p = m_isOverrideColorSet;
    *overrideColor_p = m_overrideColor;
    *m_overrideLinePattern_p = m_overrideLinePattern;
}

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * CLASS: CSubPlot
 * Description:
 * ----------------------------------------------------------------------------------------------------------------------
 * */
CSubPlot::CSubPlot(const char *title_p, int subPlotID, const char *Y_AxisLabel_p)
{
    strncpy(m_title, title_p, MAX_PLOT_NAME_LENTGH);
    strncpy(m_Y_AxisLabel, Y_AxisLabel_p, MAX_PLOT_STRING_LENTGH);

    m_ID = subPlotID;
    m_properties = 0;
    m_decorator_p = nullptr;

    Clean();
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
CSubPlot::~CSubPlot()
{
    Clean();

    m_labels.DeleteAll();

    if (m_decorator_p != nullptr) {
        delete m_decorator_p;
        m_decorator_p = nullptr;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
CGraph *CSubPlot::AddGraph(const char *name_p, int estimatedNumOfObjects)
{
    auto graph_p = new CGraph(name_p, m_ID, estimatedNumOfObjects);
    if (m_properties & SUB_PLOT_PROPERTY_PAINTING) {
        graph_p->SetProperty(GRAPH_PROPERTY_PAINTING);
    }
    m_graphs.InsertTail(static_cast<CListObject *>(graph_p));
    return graph_p;
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
CDecorator *CSubPlot::AddDecorator(void)
{
    if (m_decorator_p == nullptr) {
        m_decorator_p = new CDecorator(m_ID);
    }
    return m_decorator_p;
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
CSequenceDiagram *CSubPlot::AddSequenceDiagram(const char *name_p, int estimatedNumOfObject)
{
    (void)AddDecorator(); /* just make sure that it is added */

    CSequenceDiagram *sequenceDiagram_p = new CSequenceDiagram(name_p, m_ID, m_decorator_p, estimatedNumOfObject);
    m_graphs.InsertTail(static_cast<CListObject *>(sequenceDiagram_p));
    return sequenceDiagram_p;
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
int CSubPlot::AddLabel(const char *label_p, const int labelLength)
{
    if ((label_p != nullptr) && (labelLength != 0) && (label_p[labelLength] == 0)) {
        CGO_Label *newLabel_p = new CGO_Label(label_p, labelLength);

        if (newLabel_p == nullptr) {
            ErrorHook("Out of memory");
            return 0;
        }

        m_labels.InsertTail(static_cast<CListObject *>(newLabel_p));
        return m_labels.count() - 1;
    } else {
        ErrorHook("CSubPlot::AddLabel failed, bad input parameters\n");
        return 0;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
void CSubPlot::Clean(void)
{
    memset(&m_extents, 0, sizeof(GraphicalObject_Extents_t));

    m_graphs.DeleteAll();

    if (m_decorator_p != nullptr) {
        m_decorator_p->Clean();
        m_decorator_p = nullptr;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 * */
void CSubPlot::CalcExtents(void)
{
    GraphicalObject_Extents_t extents;
    CGraph *graph_p = static_cast<CGraph *>(m_graphs.first());

    if (graph_p != nullptr) {
        graph_p->GetExtents(&m_extents);

        while (graph_p != nullptr) {
            graph_p->GetExtents(&extents);

            if (extents.x_min < m_extents.x_min) {
                m_extents.x_min = extents.x_min;
            }
            if (extents.x_max > m_extents.x_max) {
                m_extents.x_max = extents.x_max;
            }
            if (extents.y_min < m_extents.y_min) {
                m_extents.y_min = extents.y_min;
            }
            if (extents.y_max > m_extents.y_max) {
                m_extents.y_max = extents.y_max;
            }

            graph_p = static_cast<CGraph *>(m_graphs.GetNext(static_cast<CListObject *>(graph_p)));
        }
    }
}
