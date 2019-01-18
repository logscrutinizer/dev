cmake_minimum_required(VERSION 3.0)

project(LogScrutinizer)

#------------------------------------------------------
#--- Global settings
#------------------------------------------------------
set(CMAKE_VERBOSE_MAKEFILE ON)

#------------------------------------------------------
#--- Local settings
#------------------------------------------------------
# Override settings by adding a local cmake file
if (EXISTS ${CMAKE_SOURCE_DIR}/local.cmake)
    message ("Found local.cmake")
    include (${CMAKE_SOURCE_DIR}/local.cmake OPTIONAL)
else ()
    message ("!!! local.cmake missing !!!")
endif()

#------------------------------------------------------
#--- Switch framework
#------------------------------------------------------
# Static/Dynamic linking
if (NOT DEFINED SHARED_QT_LIBS)
    set (SHARED_QT_LIBS 1)
endif()

if (NOT DEFINED USE_WEB_ENGINE)
    set (USE_WEB_ENGINE 0)
endif()

# Setup paths to QT
if (NOT DEFINED CMAKE_PREFIX_PATH)
    if (WIN32)
        set(CMAKE_PREFIX_PATH "C:\\Qt\\5.10.0\\msvc2017_64")
    else (WIN32)
#        set(CMAKE_PREFIX_PATH "/home/klang/workspace/qt510_opt_clang_debug")
    endif(WIN32)
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set (USE_CLANG 1)
    message ("CLANG compiler")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set (USE_GCC 1)
    message("GCC compiler")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
elseif (${MSVC})
    message("MSVC compiler")
endif()

# Specific compiler (CLANG/GCC)
if (WIN32)
else (WIN32)
    if (NOT DEFINED USE_CLANG)
        #set (USE_CLANG 1)
    endif()
endif(WIN32)

#------------------------------------------------------
#--- Setup QT base
#------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_USE_QTOPENGL TRUE)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(OpenGL REQUIRED)

if (USE_WEB_ENGINE)
    find_package(Qt5WebEngine REQUIRED)
    find_package(Qt5WebEngineWidgets REQUIRED)
endif (USE_WEB_ENGINE)

if(WIN32)
    find_package(Qt5WinExtras)
    set (HS_INCLUDEDIR "c:/Users/rober/workspace/src/hs_win32")
else (WIN32)
    find_package(PkgConfig REQUIRED)
    find_package(Threads)
    find_package(JPEG)
    find_package(PNG)

    if (NOT ICU_ROOT)
        set (ICU_ROOT "/usr/share/icu/57.1 /usr/lib/x86_64-linux-gnu" )
    endif (NOT ICU_ROOT)
    find_package(ICU)

    pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.36.0)

    if (DEFINED LOCAL_PKG_CONFIG_PATH)
        set(TOTAL_PKG_CONFIG_PATH "${LOCAL_PKG_CONFIG_PATH}:${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/:$ENV{PKG_CONFIG_PATH}")
        message("TOTAL_PKG_CONFIG_PATH => " ${TOTAL_PKG_CONFIG_PATH})
        set(ENV{PKG_CONFIG_PATH}  ${TOTAL_PKG_CONFIG_PATH})
    endif()
    pkg_check_modules(HS REQUIRED libhs)

    find_package(Freetype REQUIRED)
    set(HARFBUZZ_DIR "$ENV{HARFBUZZ_DIR}" CACHE PATH "HarfBuzz directory")
    find_path(HARFBUZZ_INCLUDE_DIRS
        NAMES hb-ft.h PATH_SUFFIXES harfbuzz
        HINTS ${HARFBUZZ_DIR}/include)
    find_library(HARFBUZZ_LIBRARIES
        NAMES harfbuzz
        HINTS ${HARFBUZZ_DIR}/lib)
endif(WIN32)

get_target_property(QtCore_location Qt5::Core LOCATION)

#------------------------------------------------------
#--- Setup general include directories for all modules
#------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/utils)
include_directories(${CMAKE_SOURCE_DIR}/settings)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Multimedia_INCLUDE_DIRS})
include_directories(${HS_INCLUDEDIR})

if (USE_WEB_ENGINE)
    include_directories(${Qt5WebEngine_INCLUDE_DIRS})
    include_directories(${Qt5WebEngineWidgets_INCLUDE_DIRS})
endif (USE_WEB_ENGINE)

if(WIN32)
    include_directories(${Qt5WinExtras_INCLUDE_DIRS})
else (WIN32)
    if (USE_CLANG)
        include_directories(/usr/include/llvm-5.0/llvm/Support) #type_traits
        include_directories(/usr/include/c++/v1)
    else (USE_CLANG)
    endif (USE_CLANG)
endif(WIN32)

#------------------------------------------------------
#--- Set compiler flags
#------------------------------------------------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(WIN32)      #---- Windows, Visual Studio

    set(DEFAULT_COMPILE_OPTIONS
        /analyze- # (- disables) Enables code analysis and control options.
        /EHsc
        /fp:precise # Specifies floating-point behavior in a source code file
        /MP # Option causes the compiler to create one or more copies of itself, each in a separate process. Then
            # these copies simultaneously compile the source files
        /bigobj # Enable larger obj-files, otherwise some cpp files will generate compiler error since they
                # are too large
        /W3  # Highest warning level
        /nologo # Suppresses display of the sign-on banner when the compiler starts up and display of informational
                # messages during compiling. It is only available when compiling from the command line
        /GT # Supports fiber safety for data allocated using static thread-local storage.
        /GR # Enables run-time type information (RTTI).
        /Gd # The default setting, specifies the __cdecl calling convention for all functions except C++ member
            # functions and functions that are marked __stdcall, __fastcall, or __vectorcall.
        /Zc:wchar_t # enabe char as wchar
        /Zc:forScope # Used to implement standard C++ behavior for for-loops with Microsoft extensions
        /errorReport:queue # Lets you provide internal compiler error (ICE) information directly to Microsoft.
                           # Queues the error report. When you log in with administrator privileges, a window is
                           # displayed so that you can report any failures since the last time you were logged in
        /GF # Enables the compiler to create a single copy of identical strings in the program image and in memory
            # during execution. This is an optimization called string pooling that can create smaller programs.
        /Gm- # (- disables) Enables minimal rebuild, which determines whether C++ source files that include
             # changed C++ class definitions (stored in header (.h) files) need to be recompiled.
        /Zi # Generates complete debugging information.
        )

    set(DEFAULT_DEBUG_COMPILE_OPTIONS ${DEFAULT_COMPILE_OPTIONS}
        /Od # Disables optimization
        /GS # Detects some buffer overruns that overwrite a function's return address, exception handler
            # address, or certain types of parameters. Causing a buffer overrun is a technique used by hackers to
            # exploit code that does not enforce buffer size restrictions.
        )


    set(DEFAULT_RELEASE_COMPILE_OPTIONS ${DEFAULT_COMPILE_OPTIONS}
        /O2 # Optimization level - O2, Creates fast code
        /GS- # Detects some buffer overruns that overwrite a function's return address, exception handler address, or
             # certain types of parameters. Causing a buffer overrun is a technique used by hackers to exploit code
             # that does not enforce buffer size restrictions.
        /Ob2 # (Ob2 is the default value). Allows expansion of functions marked as inline, __inline, or __forceinline,
             # and any other function that the compiler chooses.
        /Oy # Omits frame pointer (x86 only). Suppresses creation of frame pointers on the call stack.
        /Oi # Generates intrinsic functions, Replaces some function calls with intrinsic or otherwise special forms
            # of the function that help your application run faster.
        /Gy # Enable Function-Level Linking, Allows the compiler to package individual functions in the form of
            # packaged functions (COMDATs).
        )

    # Generator expression, supported by the CMake Visual Studio generator
    add_compile_options(
        "$<$<CONFIG:DEBUG>:${DEFAULT_DEBUG_COMPILE_OPTIONS}>"
        "$<$<CONFIG:RELEASE>:${DEFAULT_RELEASE_COMPILE_OPTIONS}>"
        )

    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS)      # No warnings for not using strcpy_s

    # mainCRTStartup enables application to start from main instead of WinMain
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:windows /ENTRY:mainCRTStartup")

else(WIN32)    #---- Linux

    # -- Defining re-usable flags
    # See http://clang.llvm.org/docs/UsersManual.html
    if(USE_CLANG)
        set (DEFAULT_COMPILE_OPTIONS
            -std=c++17
            -stdlib=libc++ # Use LLVM's libc++ instead of GNU libstdc++
            -fno-operator-names # Do not treat the operator name keywords "and", "bitand", "bitor", "compl",
                                # "not", "or" and "xor" as synonyms as keywords.
            -Wall # Warning level
            -D_M_AMD64 # Target processor architecture. _M_AMD64 and _M_X64 are equivalent
            -m64
            -fPIC
            -__USE_XOPEN2K8
            -pthread
            # Note that function like macros need to be defined inside "".
            " -D'UNREFERENCED_PARAMETER(a)=(void)a' " # Not defined in libc++
            )
    else(USE_CLANG)
        set (DEFAULT_COMPILE_OPTIONS
           -std=c++17
            -fPIC
            -Wall # Warning level
            -D_M_AMD64 # Target processor architecture. _M_AMD64 and _M_X64 are equivalent
            -m64
            )
#        link_directories(directory1 directory2 ...)
    endif(USE_CLANG)

    set (DEFAULT_DEBUG_OPTIONS
        -O0 # Optimization level, 0 means none (good for debugging)
        -g3 # Generate debug symbols
        -D_DEBUG # _DEBUG target
        -ggdb # Produce debugging information for use by GDB. This means to use the most expressive format
              # available (DWARF 2, stabs, or the native format if neither of those are supported), including GDB
              # extensions if at all possible.
        )

    set (DEFAULT_RELEASE_OPTIONS
        -O2 # Highest optimization level, turns on all optimizations
        -g3
        )

    set (DEFAULT_RELEASE_SYM_OPTIONS
        -O2 # Highest optimization level, turns on all optimizations
        -g3 # Generate debug symbols
        )

    # -- Setting build variant flags (base on the re-usable flags)

    set (CMAKE_CXX_FLAGS_x  ${DEFAULT_COMPILE_OPTIONS})
    set (CMAKE_C_FLAGS_x    ${DEFAULT_COMPILE_OPTIONS}) # Can we use exactly the same flags between c and c++

    # The CMake unix make generator doesn't support compiler flags as a list parameter, it must be a string.
    # A cmake list is presented as a string where each item is seperated with a ";". To convert compiler options from a
    # commented list into a string just replace the ";".

    # These are basic flags that always will be appended (at least for the CMake Unix generator)
    string (REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_x}")
    string (REPLACE ";" " " CMAKE_C_FLAGS   "${CMAKE_C_FLAGS_x}")

    string (REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG   "${DEFAULT_DEBUG_OPTIONS}")
    string (REPLACE ";" " " CMAKE_C_FLAGS_DEBUG     "${DEFAULT_DEBUG_OPTIONS}")

    string (REPLACE ";" " " CMAKE_CXX_FLAGS_RELEASE "${DEFAULT_RELEASE_OPTIONS}")
    string (REPLACE ";" " " CMAKE_C_FLAGS_RELEASE   "${DEFAULT_RELEASE_OPTIONS}")

endif(WIN32)

if (USE_WEB_ENGINE)
    add_definitions( -D_WEB_HELP)
endif (USE_WEB_ENGINE)

#------------------------------------------------------
#--- Setup module lists
#------------------------------------------------------
set (MODULE_PATH
    settings
    crash
    containers
    processing
    utils
    testing
    plugins/plugin_dev/plugin_framework)

#------------------------------------------------------
#--- Add all sub-module source and header files
#------------------------------------------------------
foreach(module ${MODULE_PATH})
    add_subdirectory(${module})
    get_filename_component(barename ${module} NAME)
    list(APPEND moduleList ${barename})
endforeach(module)

#------------------------------------------------------
#--- Additional modules
#------------------------------------------------------
add_subdirectory(plugins/plugin_dev/plugin_example_1)
add_subdirectory(plugins/plugin_dev/plugin_example_2)
add_subdirectory(plugins/plugin_dev/plugin_example_3)
add_subdirectory(plugins/plugin_dev/plugin_example_3_1)
add_subdirectory(plugins/plugin_dev/plugin_example_4)
add_subdirectory(plugins/plugin_dev/plugin_example_5)
add_subdirectory(plugins/plugin_dev/plugin_example_6)
add_subdirectory(plugins/plugin_dev/plugin_tester)
add_subdirectory(plugins/plugin_dev/file_streamer)

#------------------------------------------------------
#--- Add the QT folder
#------------------------------------------------------
include (${CMAKE_SOURCE_DIR}/qt/CMakeLists.txt)

#------------------------------------------------------
#--- Print some general settings
#------------------------------------------------------

if (WIN32)
    message ("WIN32 platform")
else (WIN32)
    if (USE_CLANG)
        message ("LINUX - CLANG BUILD")
    else(USE_CLANG)
        message ("LINUX - GCC BUILD")
    endif(USE_CLANG)
endif(WIN32)

message ("CMAKE_PREFIX_PATH - ${CMAKE_PREFIX_PATH}")
message ("Project modules " ${MODULE_PATH})
message ("USE_WEB_ENGINE: ${USE_WEB_ENGINE}")
message ("SHARED_QT_LIBS: ${SHARED_QT_LIBS}")

message ("utilsDIR ${CMAKE_SOURCE_DIR}/utils")
message ("Qt5Widgets_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS}")
message ("Qt5Multimedia_INCLUDE_DIRS ${Qt5Multimedia_INCLUDE_DIRS}")
message ("Qt5WebEngine_INCLUDE_DIRS ${Qt5WebEngine_INCLUDE_DIRS}")
message ("Qt5WebEngineWidgets_INCLUDE_DIRS ${Qt5WebEngineWidgets_INCLUDE_DIRS}")

message ("GLIB2 ${GLIB2_LIBRARIES}")
message ("PulseAudio: ${PULSEAUDIO_LIBRARY}")
message ("DL: ${CMAKE_DL_LIBS}")
message ("ICU: ${ICU_i18n_FOUND} ")
message ("CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
message ("LIBHS " ${HS_INCLUDEDIR} "  " ${HS_LIBDIR})
message ("QtCoreLocation " ${QtCore_location})
message ("Linker flags: " ${CMAKE_EXE_LINKER_FLAGS})
message ("CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
message ("CMAKE_SYSTEM_VERSION: " ${CMAKE_SYSTEM_VERSION})
