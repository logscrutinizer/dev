#ifndef PLUGIN_CONSTANTS_H
#define PLUGIN_CONSTANTS_H

#include "plugin_constants_internal.h"

//----------------------------------------------------------------------------------------------------------------------
// File: plugin_constants.h
//
// Description:
// This header file contains constants for the plugin framework
//
// IMPORTANT: DO NOT MODIFY THIS FILE
//
//----------------------------------------------------------------------------------------------------------------------

#define DLL_API_VERSION_TXT "DLL_API_VERSION"
#define DLL_API_VERSION  23 // Current version of the dll plugin framework. Checked by LogScrutinizer such that it has the same version

// DLL_API_VERSION = 7 : v1.2.B4 Pre30
// DLL_API_VERSION = 8 : v1.2.B4 Pre33
// DLL_API_VERSION = 9 : v1.2.B4 Pre35
// DLL_API_VERSION = 21 : First version based on Qt
// DLL_API_VERSION = 22 : Removed match strings   v2.0.B2 build1
// DLL_API_VERSION = 23 : Added painting v2.0.B4

#define PLUGIN_INFO_NAME_MAX_SIZE      256
#define PLUGIN_INFO_VERSION_MAX_SIZE   256
#define PLUGIN_INFO_AUTHOR_MAX_SIZE    512
#define PLUGIN_INFO_HELP_URL_MAX_SIZE  1280

//#define SUPPORTED_FEATURE_POPUP                            1        // 1      NOT YET SUPPORTED
//#define SUPPORTED_FEATURE_POPUP_MULTI_SELECTION           (1 << 1)  // 2      NOT YET SUPPORTED
#define SUPPORTED_FEATURE_PLOT                              (1 << 2)  // 4      One or several CPlot are added to the plugin
//#define SUPPORTED_FEATURE_CONSOLE                         (1 << 3)  // 8      NOT YET SUPPORTED
//#define SUPPORTED_FEATURE_PLUGIN_SETTINGS                 (1 << 4)  // 16     NOT YET SUPPORTED
#define SUPPORTED_FEATURE_DECODER                           (1 << 5)  // 32     One or several CDecoder are added to the plugin
#define SUPPORTED_FEATURE_HELP_URL                          (1 << 6)  // 64     Supply a URL for help
#define SUPPORTED_FEATURE_PLOT_TIME                         (1 << 7)  // 128    The plugin may try to extract current time from the row string
#define SUPPORTED_FEATURE_PLOT_GRAPHICAL_OBJECT_FEEDBACK    (1 << 9)  // 512    The plugin may provide additional information when user is hovering the mouse over a graphical object (graph object)

// Subplot properties

// Property:      SUB_PLOT_PROPERTY_SCHEDULE
// Descritption:  Set this property if you want to display boxes where all boxes on a graph is within the same extents
//                Each graph label will be printed at the left
//                The graph label will be centered to the graph extents
//                Horizontal lines will be drawn at the graph extents.
#define SUB_PLOT_PROPERTY_SCHEDULE                                  0x01

// Property:      SUB_PLOT_PROPERTY_SEQUENCE_DIAGRAM
// Descritption:  Set this property if your subplot contains a sequence diagram instead of graphs
#define SUB_PLOT_PROPERTY_SEQUENCE                                  0x02

// Property:      SUB_PLOT_PROPERTY_NO_LEGEND_COLOR
// Descritption:  Makes the color of the legend text to be black. Typically used if plugin define colors itself
#define SUB_PLOT_PROPERTY_NO_LEGEND_COLOR                           0x08

// Property:      SUB_PLOT_PROPERTY_PAINTING
// Descritption:  A sub-plot with the painting property is considered without x-time, hence each graphical
//                object doesn't need to be continous.
#define SUB_PLOT_PROPERTY_PAINTING                                  0x10

#define MAX_STRING_LENGTH                                           512   // Used for match strings, and determine max string length allowed to be returned by a decoder

#define MAX_NUM_OF_SUB_PLOTS                                        32
#define MAX_PLOT_STRING_LENTGH                                      1024
#define MAX_PLOT_NAME_LENTGH                                        128

#define MAX_GRAPH_NAME_LENGTH                                       256

#define GRAPHICAL_OBJECT_BOX_LABEL_SIZE                             32    // Maximum letters for the box label


#endif
