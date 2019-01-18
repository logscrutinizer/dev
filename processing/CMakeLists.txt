project(processing)

include_directories(${CMAKE_SOURCE_DIR}/containers)
include_directories(${CMAKE_SOURCE_DIR}/plugins/plugin_dev/plugin_framework)
include_directories(${CMAKE_SOURCE_DIR}/crash)
include_directories(${CMAKE_SOURCE_DIR}/qt)

file(GLOB ${PROJECT_NAME}_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_sources})
