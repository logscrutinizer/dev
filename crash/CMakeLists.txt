project(crash)

include_directories(${CMAKE_SOURCE_DIR}/plugins/plugin_dev/plugin_framework)
include_directories(${CMAKE_SOURCE_DIR}/utils)
include_directories(${CMAKE_SOURCE_DIR}/containers)
include_directories(${CMAKE_SOURCE_DIR}/processing)
include_directories(${CMAKE_SOURCE_DIR}/crash)
include_directories(${CMAKE_SOURCE_DIR}/testing)
include_directories(${CMAKE_SOURCE_DIR}/qt_model)
include_directories(${CMAKE_SOURCE_DIR}/qt)

if (WIN32)
   set(${PROJECT_NAME}_sources
       ${CMAKE_CURRENT_SOURCE_DIR}/crash_handler_win.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/crash_handler_win.h)
else (WIN32)

   set(${PROJECT_NAME}_sources
       ${CMAKE_CURRENT_SOURCE_DIR}/crash_handler_linux.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/crash_handler_linux.h)
endif (WIN32)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_sources})
