#------------------------------------------------------
#--- When building plugins from LogScrutinizer the
#--- plugin.cmake is used to define which to include
#------------------------------------------------------

message("Adding plugins to LogScrutinizer project")
add_subdirectory(${PLUGIN_PATH}/plugin_example_1)
add_subdirectory(${PLUGIN_PATH}/plugin_example_2)
add_subdirectory(${PLUGIN_PATH}/plugin_example_3)
add_subdirectory(${PLUGIN_PATH}/plugin_example_3_1)
add_subdirectory(${PLUGIN_PATH}/plugin_example_4)
add_subdirectory(${PLUGIN_PATH}/plugin_example_5)
add_subdirectory(${PLUGIN_PATH}/plugin_example_6)
add_subdirectory(${PLUGIN_PATH}/plugin_example_7)
add_subdirectory(${PLUGIN_PATH}/plugin_tester)
add_subdirectory(${PLUGIN_PATH}/file_streamer)

if (EXISTS ${PLUGIN_PATH}/local_plugins.cmake)
    message ("Found ${PLUGIN_PATH}/local_plugins.cmake")
else()
    message ("No ${PLUGIN_PATH}/local_plugins.cmake")
endif()

include (${PLUGIN_PATH}/local_plugins.cmake OPTIONAL)




