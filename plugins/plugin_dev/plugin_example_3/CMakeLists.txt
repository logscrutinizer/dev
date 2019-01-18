cmake_minimum_required(VERSION 3.0)
project(plugin_example_3)

#------------------------------------------------------
#--- Add include directories
#------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../plugin_framework)

#------------------------------------------------------
# Add files to the library
#------------------------------------------------------
file(GLOB ${PROJECT_NAME}_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp    
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

#------------------------------------------------------
# Define the library
#------------------------------------------------------
add_library(${PROJECT_NAME} SHARED 
            ${${PROJECT_NAME}_sources}  
            ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

target_link_libraries(${PROJECT_NAME} plugin_framework)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER plugin.h)
add_dependencies(${PROJECT_NAME} plugin_framework)
