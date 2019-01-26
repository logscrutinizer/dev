

message ("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
# http://wiki.qt.io/Building_Qt_5_from_Git

include_directories(${CMAKE_SOURCE_DIR}/plugins/plugin_dev/plugin_framework)
include_directories(${CMAKE_SOURCE_DIR}/containers)
include_directories(${CMAKE_SOURCE_DIR}/processing)
include_directories(${CMAKE_SOURCE_DIR}/crash)
include_directories(${CMAKE_SOURCE_DIR}/testing)

file(GLOB ${PROJECT_NAME}_sources
    ${CMAKE_SOURCE_DIR}/qt/*.cpp
    ${CMAKE_SOURCE_DIR}/qt/*.c
    ${CMAKE_SOURCE_DIR}/qt/*.h)

# Files included to the project just to be shown by the IDE (QtCreator)
file(GLOB ${PROJECT_NAME}_ide
    ${CMAKE_SOURCE_DIR}/*.txt
    ${CMAKE_SOURCE_DIR}/*.cmake
    ${CMAKE_SOURCE_DIR}/misc/*.cfg
    ${CMAKE_SOURCE_DIR}/misc/*.sh
    ${CMAKE_SOURCE_DIR}/misc/*.txt
    ${CMAKE_SOURCE_DIR}/qt/*.cmake # additional cmake files (local.cmake)
    ${CMAKE_SOURCE_DIR}/qt/*.qss # stylesheets (stylesheet.qss)
    ${CMAKE_SOURCE_DIR}/CMakeLists.txt # top-level
    ${CMAKE_SOURCE_DIR}/packaging/*
    ${CMAKE_SOURCE_DIR}/qt/CMakeLists.txt) # qt-dir level

file(GLOB ${PROJECT_NAME}_resources
    ${CMAKE_SOURCE_DIR}/qt/*.ui
    ${CMAKE_SOURCE_DIR}/qt/*.qrc)

if (WIN32)
    file(GLOB ${PROJECT_NAME}_platform
        ${CMAKE_SOURCE_DIR}/qt/win.rc) # contains the application icon (for the exe-file)

    set (EXEC_OPTIONS "WIN32") # Prevent console to show when starting Win32 window
else (WIN32)
endif (WIN32)
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

# Extra check since there are some hard-coded "LogScrutinizer" values in the CMakeFiles.
if (NOT "${PROJECT_NAME}" STREQUAL "LogScrutinizer")
    message( FATAL_ERROR "PROJECT_NAME not setup correctly, CMake will exit." )
endif (NOT "${PROJECT_NAME}" STREQUAL "LogScrutinizer")

#-------------------------------------------------------------------------------------------------
# From product_version.cmake
#-------------------------------------------------------------------------------------------------

# Installation directory on the target system. This may be used by some CPack generators like NSIS
# to create an installation directory e.g., “CMake 2.5” below the installation prefix. All installed
# element will be put inside this directory.
# set (CPACK_PACKAGE_INSTALL_DIRECTORY "v2_0_b0_build1")

# set(CPACK_PACKAGE_VERSION_MAJOR "2")   These are set from package_config.cmake
# set(CPACK_PACKAGE_VERSION_MINOR "0")
# set(CPACK_PACKAGE_VERSION_PATCH "2")
# set (APPLICATION_FULL_NAME "LogScrutinizer v2.0 Beta1 build1")

# The name of the package file to generate, not including the extension.
# For example, cmake-2.6.1-Linux-i686. The default value is:
# ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}.

# set(CPACK_PACKAGE_FILE_NAME "LogScrutinizer_v2_0_B0_build2-Linux-64bit") #package name override
# set (CPACK_PACKAGE_NAME "${PROJECT_NAME}-v) # Default, not really necessary

set (PRODUCT_VERSION_GEN_SCRIPT_PY ${CMAKE_SOURCE_DIR}/packaging/generate_product_config.py)
# output from the generate_product_config
set (PRODUCT_VERSION_CMAKE ${CMAKE_BINARY_DIR}/product_version.cmake)
# input to the generate_product_config script
set (CCONFIG_H ${CMAKE_SOURCE_DIR}/settings/CConfig.h)

# Note, OS_VARIANT should be set outside, e.g. in local.cmake or as -DOS_VARIANT=Ubuntu16. If not set it is
# platform name, e.g. Linux.
if (OS_VARIANT STREQUAL "")
    set (OS_VARIANT ${CMAKE_SYSTEM_NAME})
endif (OS_VARIANT STREQUAL "")

ADD_CUSTOM_COMMAND(OUTPUT ${PRODUCT_VERSION_CMAKE} non_existing_file1.txt
                   COMMAND python ${PRODUCT_VERSION_GEN_SCRIPT_PY} ${PRODUCT_VERSION_CMAKE} ${CCONFIG_H} ${OS_VARIANT}
                   COMMENT "Generating product version cmake")

# Custom target for running the PYTHON scripts
ADD_CUSTOM_TARGET(gen_product_version ALL
                  DEPENDS ${PRODUCT_VERSION_CMAKE} non_existing_file1.txt)

if (EXISTS ${PRODUCT_VERSION_CMAKE})
    message ("Found ${PRODUCT_VERSION_CMAKE}")
    include (${PRODUCT_VERSION_CMAKE})
else ()
    message ("!!! ${PRODUCT_VERSION_CMAKE} MISSING !!!")
endif()

#-------------------------------------------------------------------------------
# PREFIX
#------------------------------------------------------------------------------
if (WIN32)
    set (CPACK_PACKAGING_INSTALL_PREFIX "")
    set (CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files\\\\LogScrutinizer")
    # The variable CPACK_PACKAGE_INSTALL_DIRECTORY is generated from python script (based on LS version),
    # and adds an extra directory where the LS exec will be put. CPACK_PACKAGING_INSTALL_PREFIX isn't used
    # with NSIS.
else (WIN32)
    set (CPACK_PACKAGING_INSTALL_PREFIX "/opt/logscrutinizer/${CPACK_PACKAGE_INSTALL_DIRECTORY}")
endif (WIN32)
set (CMAKE_INSTALL_PREFIX ${CPACK_PACKAGING_INSTALL_PREFIX})
set (CPACK_INSTALL_PREFIX ${CPACK_PACKAGING_INSTALL_PREFIX})

#-------------------------------------------------------------------------------
# Setup Packaging utility scripts  -- install_script_gen.cmake (only Linux)
#-------------------------------------------------------------------------------------------------

# Add/Search additional files that should be installed, provided as input to script
# If WEB_ENGINE is to be used then the python script which filter out some WebFiles needs to be updated as well.
 if (WIN32)
    file(GLOB QT_PLATFORM_PLUGIN ${CMAKE_PREFIX_PATH}/plugins/platforms/*.dll)
    # Additional libs isn't used by Linux variant, these are instead extracted using ldd
    file(GLOB QT_ADDITIONAL_LIBS ${CMAKE_PREFIX_PATH}/bin/*.dll)
 else (WIN32)
    if (USE_WEB_ENGINE)
        file(GLOB QT_PLATFORM_PLUGIN ${CMAKE_PREFIX_PATH}/plugins/platforms/*.so)
    endif (USE_WEB_ENGINE)
        set (QT_PLATFORM_PLUGIN ${CMAKE_PREFIX_PATH}/plugins/platforms/libqxcb.so)
endif(WIN32)

if (WIN32)
    set (INSTALL_SCRIPT_GEN_PY ${CMAKE_SOURCE_DIR}/packaging/generate_additional_packaging_files_win32.py)
else (WIN32)
    set (INSTALL_SCRIPT_GEN_PY ${CMAKE_SOURCE_DIR}/packaging/generate_additional_packaging_files_linux.py)
endif (WIN32)

# Files generated by the INSTALL_SCRIPT_GEN_PY (Linux only)
set (INSTALL_POSTINST ${CMAKE_BINARY_DIR}/postinst)
set (INSTALL_PRERM ${CMAKE_BINARY_DIR}/prerm)
set (QT_PLATFORM_CONF_FILE ${CMAKE_BINARY_DIR}/qt.conf)
set (START_SCRIPT_SH ${CMAKE_BINARY_DIR}/start.sh)
set (INSTALL_LIB_CMAKE ${CMAKE_BINARY_DIR}/install_lib.cmake)
set (INSTALL_PLATFORM_PLUGIN_CMAKE ${CMAKE_BINARY_DIR}/platform_plugin.cmake) #just for win32

if (WIN32)
# The python script checks with platform it is, e.g. POSTINT will not be used for WIN32. Actually only the QT-conf file will
# be generated (for Win32)
    ADD_CUSTOM_COMMAND(OUTPUT ${INSTALL_LIB_CMAKE} non_existing_file0.txt
                       COMMAND python ${INSTALL_SCRIPT_GEN_PY} ${INSTALL_LIB_CMAKE} ${INSTALL_PLATFORM_PLUGIN_CMAKE} ${QT_PLATFORM_CONF_FILE} ${QT_PLATFORM_PLUGIN} "@@@" ${QT_ADDITIONAL_LIBS}
                                        COMMENT "Generating additional packaging files")

else (WIN32)

    ADD_CUSTOM_COMMAND(OUTPUT ${INSTALL_LIB_CMAKE} ${INSTALL_POSTINST} ${INSTALL_PRERM} non_existing_file0.txt
                       COMMAND python ${INSTALL_SCRIPT_GEN_PY} ${INSTALL_LIB_CMAKE} ${INSTALL_POSTINST}
                                        ${INSTALL_PRERM} ${QT_PLATFORM_CONF_FILE} ${START_SCRIPT_SH}
                                        ${CPACK_PACKAGING_INSTALL_PREFIX} ${QT_PLATFORM_PLUGIN} ${QT_ADDITIONAL_LIBS}
                                        $<TARGET_FILE:LogScrutinizer>
                                        COMMENT "Generating additional packaging files")

endif(WIN32)

# Custom target for running the PYTHON script generating help files
 ADD_CUSTOM_TARGET(gen_inst_scripts
                   DEPENDS ${INSTALL_LIB_CMAKE} ${INSTALL_POSTINST} ${INSTALL_PRERM}
                           ${INSTALL_SCRIPT_GEN_PY} ${PRODUCT_VERSION_CMAKE}
                           non_existing_file0.txt)

if (EXISTS ${INSTALL_LIB_CMAKE})
    message ("Found ${INSTALL_LIB_CMAKE}")
    include (${INSTALL_LIB_CMAKE} OPTIONAL)
else ()
    message ("!!! ${INSTALL_LIB_CMAKE} MISSING !!!")
endif()

if (EXISTS ${INSTALL_PLATFORM_PLUGIN_CMAKE})
    message ("Found ${INSTALL_PLATFORM_PLUGIN_CMAKE}")
    include (${INSTALL_PLATFORM_PLUGIN_CMAKE} OPTIONAL)
else ()
    message ("!!! ${INSTALL_PLATFORM_PLUGIN_CMAKE} MISSING !!!")
endif()

if (WIN32)
    install (FILES ${INSTALL_LIBS} DESTINATION "bin")
    install (FILES ${QT_PLATFORM_PLUGIN} DESTINATION "bin/plugins/platforms")
    install (FILES ${QT_PLATFORM_CONF_FILE} DESTINATION "bin")
else (WIN32)
    # In the generated install_libs.cmake the variable ${INSTALL_LIBS} is defined
    install (FILES ${INSTALL_LIBS} DESTINATION "${CPACK_PACKAGING_INSTALL_PREFIX}/libs")
    install (FILES ${QT_PLATFORM_PLUGIN} DESTINATION "${CPACK_PACKAGING_INSTALL_PREFIX}/plugins/platforms")
    install (FILES ${QT_PLATFORM_CONF_FILE} DESTINATION "${CPACK_PACKAGING_INSTALL_PREFIX}")
    install (FILES ${START_SCRIPT_SH} DESTINATION "${CPACK_PACKAGING_INSTALL_PREFIX}")
endif (WIN32)

#-------------------------------------------------------------------------------------------------
# General CPack settings
#-------------------------------------------------------------------------------------------------
# http://neurosuite.sourceforge.net/packaging.html
# https://cmake.org/cmake/help/v3.3/module/CPack.html

set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "LogScrutinizer, used for text log processing")
set (CPACK_PACKAGE_VENDOR "SoftFairy AB")
set (CPACK_PACKAGE_CONTACT "robert.klang@softfairy.com")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/packaging/README.txt")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/copyright.txt")

# A branding image that will be displayed inside the installer (used by GUI installers)
#set (CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/../media/icons/logo.png")

# Lists each of the executables and associated text label to be used to create Start Menu shortcuts.
# For example, setting this to the list ccmake;CMake will create a shortcut named “CMake” that will
# execute the installed executable ccmake. Not all CPack generators use it (at least NSIS, WIX and OSXX11 do).
set (CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME};${PROJECT_NAME})

# List of desktop links to create. Each desktop link requires a corresponding start menu shortcut as
# created by CPACK_PACKAGE_EXECUTABLES.
set (CPACK_CREATE_DESKTOP_LINKS ${PROJECT_NAME})

set (CPACK_STRIP_FILES FALSE)
set (CPACK_SOURCE_STRIP_FILES "")

#-------------------------------------------------------------------------------------------------
# Packager specifics
#-------------------------------------------------------------------------------------------------

# Set which  packager in local.cmake

if (CPACK_DEBIAN)
# https://gitlab.kitware.com/cmake/community/wikis/doc/cpack/examples/linux/DEB
# make package
# sudo dpkg -i DistanceBetweenPoints-0.1.1-Linux.deb
    set (CPACK_GENERATOR "DEB")
    set (CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/prerm;" )
    set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Robert Klang") #required
    # https://github.com/MiKTeX/miktex/blob/master/CMakeLists.txt
    set (CPACK_DEBIAN_PACKAGE_DEPENDS "fontconfig")
endif (CPACK_DEBIAN)

if (CPACK_RPM)
    #sudo apt-get install rpm
    # https://cmake.org/cmake/help/latest/module/CPackRPM.html
    set (CPACK_GENERATOR "RPM")
    set (CPACK_RPM_PACKAGE_LICENSE "") # TODO
    set (CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/postinst")
    set (CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/prerm")
    set (CPACK_RPM_PACKAGE_REQUIRES "fontconfig")

    # May be used to enable (1, yes) or disable (0, no) automatic shared libraries dependency detection.
    # Dependencies are added to requires list.
    # rpm -qp --requires file.rpm
    set (CPACK_RPM_PACKAGE_AUTOREQ 0)

    # May be used to enable (1, yes) or disable (0, no) automatic listing of shared libraries that
    # are provided by the package. Shared libraries are added to provides list.
    # rpm -qp --provides file.rpm
    # SET(CPACK_RPM_PACKAGE_AUTOPROV 1)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set (CPACK_RPM_SPEC_INSTALL_POST "/bin/true")
      message ("CPACK NOT stripping RPM from debug symbols")
    endif()

endif (CPACK_RPM)


# CPACK_PACKAGE_INSTALL_DIRECTORY (generated from python script)
# Installation directory on the target system. This may be used by some CPack generators like NSIS to create an installation directory
# e.g., “v2_0_0” below the installation prefix. All installed element will be put inside this directory.

# logscrutinizer <- prefix
# -> v2_0_0      <- CPACK_PACKAGE_INSTALL_DIRECTORY
#    -> bin
#    -> libs #    -> plugins -> platform (lets try to move the Qt plugin/platform into libs)
#    -> plugin_dev
#
# LogScrutinizer v2.0.0  <- CPACK_NSIS_DISPLAY_NAME


if (CPACK_NSIS)
# There is a bug in NSI that does not handle full unix paths properly. Make
# sure there is at least one set of four (4) backlasshes.
    set (CPACK_GENERATOR "NSIS")
    set (CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\LogScrutinizer.exe")

    # The display name string that appears in the Windows Add/Remove Program control pane
    set (CPACK_NSIS_DISPLAY_NAME ${APPLICATION_FULL_NAME})

    # The title displayed at the top of the installer.
    set (CPACK_NSIS_PACKAGE_NAME ${APPLICATION_FULL_NAME})

    # An icon filename. The name of a *.ico file used as the main icon for the generated install program.
    # set (CPACK_NSIS_MUI_ICON )
    # An icon filename. The name of a *.ico file used as the main icon for the generated uninstall program.
    # set (CPACK_NSIS_MUI_UNIICON)
    find_file (LOGO_ICO_PATH "logo.ico" PATHS "${CMAKE_SOURCE_DIR}/../media/icons}" NO_DEFAULT_PATH)

    set(CPACK_PACKAGE_ICON ${LOGO_ICO_PATH})
    set(CPACK_NSIS_MUI_ICON ${LOGO_ICO_PATH})
    set(CPACK_NSIS_MUI_UNIICON ${LOGO_ICO_PATH})

    set (CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.logscrutinizer.com")
    set (CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.logscrutinizer.com")
    set (CPACK_NSIS_CONTACT "robert.klang@softfairy.com")
    set (CPACK_NSIS_MODIFY_PATH ON)

    # VC_REDIST must be set in local.cmake, its the path to the vcredist_x64.exe
    find_file (VC_REDIST_PATH "vcredist_x64.exe" PATHS ${VC_REDIST} NO_DEFAULT_PATH)
    message ("VC_REDIST_PATH: " ${VC_REDIST_PATH} " ... search in: " ${VC_REDIST})

    if ("${VC_REDIST_PATH}" STREQUAL "VC_REDIST_PATH-NOTFOUND")
        message( FATAL_ERROR "VC_REDIST not found")
    endif ("${VC_REDIST_PATH}" STREQUAL "VC_REDIST_PATH-NOTFOUND")

    # Setup installation command for the VC redist
    install (FILES ${VC_REDIST_PATH} DESTINATION vcredist)
    set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS " ExecWait ./vcredist/vcredist_x64.exe")

endif (CPACK_NSIS)

INCLUDE(CPack)
INCLUDE(CPackIFW)


#-------------------------------------------------------------------------------
# ADD rPATH stuff that needs to be added before add_executable
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
# https://stackoverflow.com/questions/15461556/cpack-cannot-find-libraries-target-doesnt-exist-in-this-directory
#-------------------------------------------------------------------------------

# Change the install location
# Each CPack generator has a default value (like /usr). This default value may be overwritten
# from the CMakeLists.txt or the cpack command line by setting an alternative value.
# This is not the same purpose as CMAKE_INSTALL_PREFIX which is used when installing from the build tree
# without building a package.

if (WIN32)
else (WIN32)

    set (CMAKE_SKIP_BUILD_RPATH FALSE)
    set (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set (CMAKE_INSTALL_RPATH "${CPACK_PACKAGING_INSTALL_PREFIX}/libs")

    # What is this? Used?
    set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list (FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CPACK_PACKAGING_INSTALL_PREFIX}/libs" isSystemDir)
    if ("${isSystemDir}" STREQUAL "-1")
        set (CMAKE_INSTALL_RPATH "${CPACK_PACKAGING_INSTALL_PREFIX}/libs")
    endif ("${isSystemDir}" STREQUAL "-1")
endif (WIN32)

#-------------------------------------------------------------------------------
# Define the executable
#-------------------------------------------------------------------------------
# Setting application icons on different os http://doc.qt.io/qt-5/appicon.html
add_executable(${PROJECT_NAME}
               ${EXEC_OPTIONS}
               ${${PROJECT_NAME}_sources}
               ${${PROJECT_NAME}_resources}
               ${${PROJECT_NAME}_platform}
               ${${PROJECT_NAME}_ide})

#-------------------------------------------------------------------------------
# Setup additional target/link dependencies
#-------------------------------------------------------------------------------

if (USE_WEB_ENGINE)
    set (ADDITIONAL_LINK_DEP ${ADDITIONAL_LINK_DEP} Qt5::WebEngine Qt5::WebEngineWidgets)
endif(USE_WEB_ENGINE)

if (USE_QT_SOUND)
    set (ADDITIONAL_LINK_DEP ${ADDITIONAL_LINK_DEP} Qt5::Multimedia)
endif(USE_QT_SOUND)

if (WIN32)
    set (ADDITIONAL_LINK_DEP ${ADDITIONAL_LINK_DEP} Qt5::WinExtras)

    set (SPECIAL_LINK_DEP ${SPECIAL_LINK_DEP}
        $<$<CONFIG:Debug>:${HS_PATH}/lib/hs.lib>
        $<$<CONFIG:Release>:${HS_PATH}/lib/hs.lib>)

    link_directories(
        $<$<CONFIG:Debug>:${HS_PATH}/lib>
        $<$<CONFIG:Release>:${HS_PATH}/lib>)

else (WIN32)
    set (SPECIAL_LINK_DEP ${SPECIAL_LINK_DEP} "hs")

    if (USE_CLANG)
    else (USE_CLANG)
        link_directories("/usr/local/lib64")
    endif(USE_CLANG)
endif (WIN32)

#-------------------------------------------------------------------------------
# Add properties and dependencies to executable
#-------------------------------------------------------------------------------

set (ADDITIONAL_LINK_DEP Qt5::Widgets Qt5::Gui Qt5::Core ${ADDITIONAL_LINK_DEP})

target_link_libraries(${PROJECT_NAME} ${moduleList} ${ADDITIONAL_LINK_DEP} ${SPECIAL_LINK_DEP})

add_dependencies(${PROJECT_NAME} ${moduleList} plugin_example_1 plugin_example_2 plugin_example_3)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}) # Not really needed

#-------------------------------------------------------------------------------
# INSTALL TARGET
#-------------------------------------------------------------------------------
if (WIN32)
    INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
else (WIN32)
    INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CPACK_PACKAGING_INSTALL_PREFIX})
endif (WIN32)

#-------------------------------------------------------------------------------
# Print info
#-------------------------------------------------------------------------------
message("moduleList" ${moduleList})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
message("Linker flags " ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
message("ADDITIONAL_LINK_DEP" ${ADDITIONAL_LINK_DEP})
message("System name: " ${CMAKE_SYSTEM_NAME})
message("INSTALL_LIBS: " ${INSTALL_LIBS})
message("QT_PLATFORM_PLUGIN: " ${QT_PLATFORM_PLUGIN})
