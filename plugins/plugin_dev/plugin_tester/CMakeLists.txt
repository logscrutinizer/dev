project(plugin_tester)

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

if (WIN32)
    set (ADDITIONAL_LINK_DEP ${ADDITIONAL_LINK_DEP} Qt5::WinExtras)
endif (WIN32)

#---------------------------------------------------------
# Add properties and dependencies to executable
#---------------------------------------------------------
target_link_libraries(${PROJECT_NAME} Qt5::Core ${ADDITIONAL_LINK_DEP})
add_dependencies(${PROJECT_NAME} plugin_example_1)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "plugin_tester")

#---------------------------------------------------------
# Print info
#---------------------------------------------------------
message("moduleList" ${moduleList})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS_RELEASE})

#---------------------------------------------------------
# Add targets to run the Plugin examples from the plugin_tester
#---------------------------------------------------------

add_custom_target(
    test_plugin_ex_1
    DEPENDS ${PROJECT_NAME} plugin_example_1
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_1>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_1/test_data.txt)

add_custom_target(
    test_plugin_ex_2
    DEPENDS ${PROJECT_NAME} plugin_example_2
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_2>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_2/test_data.txt)

add_custom_target(
    test_plugin_ex_3
    DEPENDS ${PROJECT_NAME} plugin_example_3
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_3>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_3/test_data.txt)

add_custom_target(
    test_plugin_ex_3_1
    DEPENDS ${PROJECT_NAME} plugin_example_3_1
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_3_1>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_3_1/test_data.txt)

add_custom_target(
    test_plugin_ex_4
    DEPENDS ${PROJECT_NAME} plugin_example_4
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_4>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_4/test_data.txt)

add_custom_target(
    test_plugin_ex_5
    DEPENDS ${PROJECT_NAME} plugin_example_5
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE:plugin_example_5>
            ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_example_5/test_data.txt)

add_custom_target(test_all_plugins
    DEPENDS test_plugin_ex_1 test_plugin_ex_2 test_plugin_ex_3 test_plugin_ex_3_1 test_plugin_ex_4)
