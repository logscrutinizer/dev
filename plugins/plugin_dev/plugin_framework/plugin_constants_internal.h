#ifndef PLUGIN_CONSTANTS_INTERNAL_H
#define PLUGIN_CONSTANTS_INTERNAL_H


//----------------------------------------------------------------------------------------------------------------------
// File: plugin_constants_internal.h
//
// Description:
// This header file contains constants for the plugin framework
//
// IMPORTANT: DO NOT MODIFY THIS FILE
//
//----------------------------------------------------------------------------------------------------------------------

//------ INTERNAL CONSTANTS ------

#define LINE_START_BIT        (0)                                             //0
#define LINE_SIZE_BIT         (3)
//----
#define GRAPHICAL_OBJECT_KIND_LINE                                (1 << (LINE_START_BIT + 0))           // bit 0        000001       1
#define GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_STR                   (1 << (LINE_START_BIT + 1))           // bit 1        000010       2
#define GRAPHICAL_OBJECT_KIND_LINE_EX_LABEL_INDEX                 (1 << (LINE_START_BIT + 2))           // bit 2        000100       4

#define BOX_START_BIT         (LINE_START_BIT + LINE_SIZE_BIT)                //3
#define BOX_SIZE_BIT          (3)
//----
#define GRAPHICAL_OBJECT_KIND_BOX                                 (1 << (BOX_START_BIT + 0))            // bit 3        001000       8
#define GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_STR                    (1 << (BOX_START_BIT + 1))            // bit 4        010000      16
#define GRAPHICAL_OBJECT_KIND_BOX_EX_LABEL_INDEX                  (1 << (BOX_START_BIT + 2))            // bit 5        100000      32


#define DECORATOR_START_BIT   (BOX_START_BIT + BOX_SIZE_BIT)                  //6
#define DECORATOR_SIZE_BIT    (1)
//----
#define GRAPHICAL_OBJECT_KIND_DECORATOR_LIFELINE                  (1 << (DECORATOR_START_BIT))          // bit 6        1000000     64


#define VISIBILITY_START_BIT  (DECORATOR_START_BIT + DECORATOR_SIZE_BIT)      //7
#define VISIBILITY_SIZE_BIT   (3)
//----
#define PROPERTIES_BITMASK_VISIBLE                      (static_cast<int16_t>(1 << (VISIBILITY_START_BIT + 0)))    // bit 7        0010000000 set by logscrutinizer, both x points are within window
#define PROPERTIES_BITMASK_VISIBLE_X                    (static_cast<int16_t>(1 << (VISIBILITY_START_BIT + 1)))    // bit 8        0100000000 set by logscrutinizer, x points are within window but not y points
#define PROPERTIES_BITMASK_VISIBLE_INTERSECT            (static_cast<int16_t>(1 << (VISIBILITY_START_BIT + 2)))    // bit 9        1000000000 set by logscrutinizer, one or more x/y points are within window, but other are outside

#define PROPERTIES_BITMASK_LINE_ARROW_START_BIT  (VISIBILITY_START_BIT + VISIBILITY_SIZE_BIT)   //10
#define PROPERTIES_BITMASK_LINE_ARROW_SIZE_BIT   (4)

#define PROPERTIES_BITMASK_LINE_ARROW_OPEN_START        (static_cast<int16_t>(1 << (PROPERTIES_BITMASK_LINE_ARROW_START_BIT + 0)))       //bit 10   1024
#define PROPERTIES_BITMASK_LINE_ARROW_SOLID_START       (static_cast<int16_t>(1 << (PROPERTIES_BITMASK_LINE_ARROW_START_BIT + 1)))       //bit 11   2048
#define PROPERTIES_BITMASK_LINE_ARROW_OPEN_END          (static_cast<int16_t>(1 << (PROPERTIES_BITMASK_LINE_ARROW_START_BIT + 2)))       //bit 12   4096
#define PROPERTIES_BITMASK_LINE_ARROW_SOLID_END         (static_cast<int16_t>(1 << (PROPERTIES_BITMASK_LINE_ARROW_START_BIT + 3)))       //bit 13   8192

// Masks

#define PROPERTIES_BITMASK_KIND_MASK                    (static_cast<int16_t>(1 << (DECORATOR_START_BIT + DECORATOR_SIZE_BIT) - 1))        //bit 0 - 6,    0000000001111111 , 7 bits, 2^7 -1 = 127 -> 1111111    // LINE, BOX, DECORATOR
#define PROPERTIES_BITMASK_KIND_LINE_MASK               (static_cast<int16_t>(((1 << LINE_SIZE_BIT) - 1) << LINE_START_BIT))               //bit 0 - 2     0000000000000111
#define PROPERTIES_BITMASK_KIND_BOX_MASK                (static_cast<int16_t>(((1 << BOX_SIZE_BIT) - 1) << BOX_START_BIT))      //bit 3 - 5     0000000000111000
#define PROPERTIES_BITMASK_KIND_DECORATOR_MASK          (static_cast<int16_t>(((1 << DECORATOR_SIZE_BIT) - 1) << DECORATOR_START_BIT))     //bit 6         0000000001000000
#define PROPERTIES_BITMASK_VISIBILITY_MASK              (static_cast<int16_t>(((1 << VISIBILITY_SIZE_BIT) - 1) << VISIBILITY_START_BIT))   //bit 7 - 9     0000001110000000
#define PROPERTIES_BITMASK_KIND_LINE_ARROW_MASK         (static_cast<int16_t>(((1 << PROPERTIES_BITMASK_LINE_ARROW_SIZE_BIT) - 1) << PROPERTIES_BITMASK_LINE_ARROW_START_BIT))      //bit 10 - 13   0011110000000000,

#endif
