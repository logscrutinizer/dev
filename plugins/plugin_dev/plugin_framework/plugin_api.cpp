/*--------------------------------------------------------------------------------------------------------------------*/

/* File: plugin_api.cpp
 *
 * Description:
 * Implementation of the DLL API used by LogScrutinizer to check API version and to create the
 * plugin API class (class CPlugin_DLL_API)
 *
 * IMPORTANT: DO NOT MODIFY THIS FILE
 *  */

/*--------------------------------------------------------------------------------------------------------------------*/

#include "plugin_api.h"
#include "plugin_base.h"
#include "plugin_utils.h"
#include "plugin_text_parser.h"
#include "plugin_utils_internal.h"

#include <stdlib.h>

#ifndef USE_MSC
 #define USE_QT_LIB
 #include <QtCore/QtGlobal>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* #CRUSTI-OFF# */

/*--------------------------------------------------------------------------------------------------------------------*/
#ifdef USE_MSC
__declspec(dllexport) int __cdecl DLL_API_GetPluginAPIVersion(DLL_API_PluginVersion_t* version_p)
#else
Q_DECL_EXPORT int DLL_API_GetPluginAPIVersion(DLL_API_PluginVersion_t* version_p)
#endif
{
    version_p->version = DLL_API_VERSION;
    return 1;
}

/****/
#ifdef USE_MSC
__declspec(dllexport) int __cdecl DLL_API_SetAttachConfiguration(DLL_API_AttachConfiguration_t* configuration_p)
#else
Q_DECL_EXPORT int DLL_API_SetAttachConfiguration(DLL_API_AttachConfiguration_t* configuration_p)
#endif
{
#ifdef _WIN32 // LINUX_TODO
    EnableMsgTrace(configuration_p->hwnd_traceConsumer, configuration_p->h_traceHeap);
#else
    Q_UNUSED(configuration_p)
#endif
    return 1;
}

/*--------------------------------------------------------------------------------------------------------------------*/
#ifdef USE_MSC
__declspec(dllexport) CPlugin_DLL_API* __cdecl DLL_API_CreatePlugin(void)
#else
Q_DECL_EXPORT CPlugin_DLL_API* DLL_API_CreatePlugin(void)
#endif
{
    return DLL_API_Factory();
}

/*--------------------------------------------------------------------------------------------------------------------*/
#ifdef USE_MSC
__declspec(dllexport) void __cdecl DLL_API_DeletePlugin(CPlugin_DLL_API* plugIn_p)
#else
Q_DECL_EXPORT void DLL_API_DeletePlugin(CPlugin_DLL_API* plugIn_p)
#endif
{
    delete plugIn_p;
}

#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------------------------------------------------------------------*/
CPlugin_DLL_API::~CPlugin_DLL_API()
{
    if (!m_decoders.isEmpty()) {
        m_decoders.DeleteAll();
    }

    if (!m_plots.isEmpty()) {
        m_plots.DeleteAll();
    }
}
