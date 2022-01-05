project(file_streamer)

message ("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
# http://wiki.qt.io/Building_Qt_5_from_Git

#------------------------------------------------------
#--- Add include directories
#------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../plugin_framework)

#------------------------------------------------------
#--- Add build files
#------------------------------------------------------
file(GLOB ${PROJECT_NAME}_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

#------------------------------------------------------
# Define the executable
#------------------------------------------------------
# Setting application icons on different os http://doc.qt.io/qt-5/appicon.html

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_sources} ${CMAKE_SOURCE_DIR}/CMakeLists.txt)

#---------------------------------------------------------
# Setup additional target/link dependencies
#---------------------------------------------------------

set (ADDITIONAL_LINK_DEP plugin_framework)

#---------------------------------------------------------
# Add properties and dependencies to executable
#---------------------------------------------------------
target_link_libraries(${PROJECT_NAME} Qt6::Core ${ADDITIONAL_LINK_DEP})
add_dependencies(${PROJECT_NAME} plugin_example_1)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "file_streamer")

#---------------------------------------------------------
# Print info
#---------------------------------------------------------
message("moduleList" ${moduleList})

#---------------------------------------------------------
# Add targets to run the Plugin examples from the plugin_tester
#---------------------------------------------------------
