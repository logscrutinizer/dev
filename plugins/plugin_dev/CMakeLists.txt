cmake_minimum_required(VERSION 3.0)
project(plugin_dev)

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
    message (FATAL_ERROR "!!! local.cmake missing !!!")
endif()

#------------------------------------------------------
#--- Switch framework
#------------------------------------------------------

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

#------------------------------------------------------
#--- Setup QT base
#------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(QT_USE_QTOPENGL TRUE)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

if(WIN32)
    find_package(Qt5WinExtras)
else (WIN32)
endif(WIN32)

get_target_property(QtCore_location Qt5::Core LOCATION)

#------------------------------------------------------
#--- Setup general include directories for all modules
#------------------------------------------------------

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})

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
        /analyze-               # (- disables) Enables code analysis and control options.
        /EHsc
        /fp:precise             # Specifies floating-point behavior in a source code file
        /MP                     # Option causes the compiler to create one or more copies of itself, each in a separate process. Then these copies simultaneously compile the source files
        /bigobj                 # Enable larger obj-files, otherwise some cpp files will generate compiler error since they are too large
        /W3                     # Highest warning level
        /nologo                 # Suppresses display of the sign-on banner when the compiler starts up and display of informational messages during compiling. It is only available when compiling from the command line
        /GT                     # Supports fiber safety for data allocated using static thread-local storage.
        /GR                     # Enables run-time type information (RTTI).
        /Gd                     # The default setting, specifies the __cdecl calling convention for all functions except C++ member functions and functions that are marked __stdcall, __fastcall, or __vectorcall.
        /Zc:wchar_t             # enabe char as wchar
        /Zc:forScope            # Used to implement standard C++ behavior for for-loops with Microsoft extensions
        /errorReport:queue      # Lets you provide internal compiler error (ICE) information directly to Microsoft. Queues the error report. When you log in with administrator privileges, a window is displayed so that you can report any failures since the last time you were logged in
        /GF                     # Enables the compiler to create a single copy of identical strings in the program image and in memory during execution. This is an optimization called string pooling that can create smaller programs.
        /Gm-                    # (- disables) Enables minimal rebuild, which determines whether C++ source files that include changed C++ class definitions (stored in header (.h) files) need to be recompiled.
        /Zi                     # Generates complete debugging information.
        )

    set(DEFAULT_DEBUG_COMPILE_OPTIONS ${DEFAULT_COMPILE_OPTIONS}
        #   /MTd                    # Causes the application to use the multithread, static, debug version of the run-time library
        /Od	                    # Disables optimization
        /GS                     # Detects some buffer overruns that overwrite a function's return address, exception handler address, or certain types of parameters. Causing a buffer overrun is a technique used by hackers to exploit code that does not enforce buffer size restrictions.
        )


    set(DEFAULT_RELEASE_COMPILE_OPTIONS ${DEFAULT_COMPILE_OPTIONS}
     #   /MT                     # Causes the application to use the multithread, static, release version of the run-time library
        /O2                     # Optimization level - O2, Creates fast code
        /GS-                    # Detects some buffer overruns that overwrite a function's return address, exception handler address, or certain types of parameters. Causing a buffer overrun is a technique used by hackers to exploit code that does not enforce buffer size restrictions.
        /Ob2                    # (Ob2 is the default value). Allows expansion of functions marked as inline, __inline, or __forceinline, and any other function that the compiler chooses.
        /Oy                     # Omits frame pointer (x86 only). Suppresses creation of frame pointers on the call stack.
        /Oi                     # Generates intrinsic functions, Replaces some function calls with intrinsic or otherwise special forms of the function that help your application run faster.
        /Gy                     # Enable Function-Level Linking, Allows the compiler to package individual functions in the form of packaged functions (COMDATs).
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
            -stdlib=libc++        # Use LLVM's libc++ instead of GNU libstdc++
            -fno-operator-names   # Do not treat the operator name keywords "and", "bitand", "bitor", "compl", "not", "or" and "xor" as synonyms as keywords.
            -Wall                 # Warning level
            -D_M_AMD64            # Target processor architecture. _M_AMD64 and _M_X64 are equivalent
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
            -Wall                 # Warning level
            -D_M_AMD64            # Target processor architecture. _M_AMD64 and _M_X64 are equivalent
            -m64
            )
    endif(USE_CLANG)

    set (DEFAULT_DEBUG_OPTIONS
        -O0                       # Optimization level, 0 means none (good for debugging)
        -g3                       # Generate debug symbols
        -D_DEBUG                  # _DEBUG target
        -ggdb                     # Produce debugging information for use by GDB. This means to 
                                  #   use the most expressive format available (DWARF 2, stabs, or the native format 
                                  #   if neither of those are supported), including GDB extensions if at all possible.
        )

    set (DEFAULT_RELEASE_OPTIONS
        -O2                       # Highest optimization level, turns on all optimizations
        -g3
        )

    set (DEFAULT_RELEASE_SYM_OPTIONS
        -O2                       # Highest optimization level, turns on all optimizations
        -g3                       # Generate debug symbols
        )

    # -- Setting build variant flags (base on the re-usable flags)

    set (CMAKE_CXX_FLAGS_x  ${DEFAULT_COMPILE_OPTIONS})
    set (CMAKE_C_FLAGS_x    ${DEFAULT_COMPILE_OPTIONS})     # Can we use exactly the same flags between c and c++

    # The CMake unix make generator doesn't support compiler flags as a list parameter, it must be a string.
    # A cmake list is presented as a string where each item is seperated with a ";". To convert compiler options from a
    # commented list into a string just replace the ";".

    string (REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_x}")     # These are basic flags that always will be appended (at least for the CMake Unix generator)
    string (REPLACE ";" " " CMAKE_C_FLAGS   "${CMAKE_C_FLAGS_x}")       # These are basic flags that always will be appended (at least for the CMake Unix generator)

    string (REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG   "${DEFAULT_DEBUG_OPTIONS}")
    string (REPLACE ";" " " CMAKE_C_FLAGS_DEBUG     "${DEFAULT_DEBUG_OPTIONS}")

    string (REPLACE ";" " " CMAKE_CXX_FLAGS_RELEASE "${DEFAULT_RELEASE_OPTIONS}")
    string (REPLACE ";" " " CMAKE_C_FLAGS_RELEASE   "${DEFAULT_RELEASE_OPTIONS}")

endif(WIN32)

#------------------------------------------------------
#--- Add framework and plugins
#------------------------------------------------------

add_subdirectory(plugin_framework)

set (PLUGIN_PATH ${CMAKE_SOURCE_DIR})
include(plugins.cmake)

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
message ("SHARED_QT_LIBS: ${SHARED_QT_LIBS}")
message ("Qt5Widgets_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS}")
message ("Qt5Core_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS}")
