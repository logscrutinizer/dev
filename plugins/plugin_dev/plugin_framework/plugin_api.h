/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#ifndef PLUGIN_API_H
 #define PLUGIN_API_H

/* #CRUSTI-OFF# */

/* ----------------------------------------------------------------------------------------------------------------------
 * File: plugin_api.h
 *
 * Description:
 * This header file contains the DLL API used by LogScrutinizer to check API version and create the
 * plugin API class (class CPlugin_DLL_API)
 *
 * IMPORTANT: DO NOT MODIFY THIS FILE
 *
 * ----------------------------------------------------------------------------------------------------------------------
 * */

/* ----------------------------------------------------------------------------------------------------------------------
 * Include files
 * ----------------------------------------------------------------------------------------------------------------------
 * */

#include "plugin_base.h"
#include "plugin_utils.h"
#include "plugin_text_parser.h"

/* ----------------------------------------------------------------------------------------------------------------------
 * API: GetPluginAPIVersion
 *
 * Description: This function is the first called by LogScrutinizer to establish which interface the DLL plugin is
 * using.
 *              The function GetPluginAPIVersion will remain unchanged through all
 *              API changes. This function is implemented in the plugin framework and responds with DLL_API_VERSION */

#define DLL_API_GET_PLUGIN_API_VERSION  "DLL_API_GetPluginAPIVersion"

typedef int (*DLL_API_GetPluginAPIVersion_t)(DLL_API_PluginVersion_t *);         /* Implemented in dll_api_v1.cpp */

/* ----------------------------------------------------------------------------------------------------------------------
 * API: DLL_API_AttachConfiguration
 *
 * Description: This function setup attach configuration, such as logging and command channels */

#define DLL_API_SET_ATTACH_CONFIGURATION  "DLL_API_SetAttachConfiguration"

typedef void (*DLL_API_SetAttachConfiguration_t)(DLL_API_AttachConfiguration_t *);         /* Implemented in
                                                                                            * dll_api_v1.cpp */

/* ----------------------------------------------------------------------------------------------------------------------
 * API: DLL_API_CreatePlugin
 *
 * Description: Initiates the creation of a sub-class of the base class CPlugin_DLL_API
 *              Calls the factory function DLL_API_Factory(...), which must be implemented by the sub-classed dll
 * plugin. */

#define DLL_API_CREATE_PLUGIN  "DLL_API_CreatePlugin"

typedef CPlugin_DLL_API * (*DLL_API_CreatePlugin_t)(void);                 /* Implemented in dll_api_v1.cpp */

/* ----------------------------------------------------------------------------------------------------------------------
 * API: DLL_API_DeletePlugin
 *
 * Description: Triggers the destructor of the sub-classed dll plugin. */

#define DLL_API_DELETE_PLUGIN  "DLL_API_DeletePlugin"

typedef void (*DLL_API_DeletePlugin_t)(CPlugin_DLL_API *plugIn_p);        /* Implemented in dll_api_v1.cpp */

/* ----------------------------------------------------------------------------------------------------------------------
 * Function: DLL_API_Factory
 *
 * Description: A prototype function called by LogScrutinizer, through DLL API DLL_API_CreatePlugin, which shall create
 * the plugin
 *              sub-class. Its a (sort of) factory pattern where this function is called to create the desired instance
 * of a class, unknown to
 *              the caller (LogScrutinizer)
 *
 * IMPORTANT:   Must be implemented in the sub-classed plugin DLL */

extern CPlugin_DLL_API *DLL_API_Factory(void);                                    /* IMPLEMENTs THIS in your custom
                                                                                   * plugin.cpp file */

/* ----------------------------------------------------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------
 * */

#endif  /* DLL_API_H */
