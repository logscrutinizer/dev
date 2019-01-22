# LogScrutinizer
<p> The LogScrutinizer is a tool for analyzing text-based logs. Read more about it here: [LogScrutinizer Home](http://www.logscrutinizer.com) <p>

<p><b> Key features </b></p>

* Create filters to view just matching rows in the log
* Multi-threaded for fastest search and filtering performance
* You can restrict which part of the log that should be analyzed
* It tracks file tail for runtime logging 
* You may develop plugins to visualize your data
  *  Line graphs
  *  Sequence charts
  *  Timing diagrams
* Based on Qt 5.10, runs on Linux and Windows platforms
* Uses the super-fast [HyperScan](https://www.hyperscan.io/) engine for regular expressions
* Handles large sized text-files, filters GB-sized logs within seconds

# Prebuild binaries
Visit [LogScrutinizer Home](http://www.logscrutinizer.com)

# Build instructions

## Build instructions - Linux

<p> UNDER CONSTRUCTION </p>
 


## Build instructions - Windows

### Prerequisites
* <b>Qt (5.10) for Windows</b>
  * https://www.qt.io/download
    * You may choose the open-source alternative, as LogScrutinizer is open-source
    * The Qt Company requires that you have an account
    * You only need to enable the 2015/2017 VisualStudio 64-bit build variants under 5.10.0 (to minimize the HDD impact)
    * Add the Qt binary path as Win10 environment variable (PATH). To find the Qt dlls when running locally (C:\Qt\5.10.0\msvc2017_64\bin)
* <b>Visual Studio 2017</b>
  * https://visualstudio.microsoft.com/vs/community/
    * Check/Use "Desktop development with C++"
* <b>Git</b>
  * https://github.com/git-for-windows/git/releases/download/v2.20.1.windows.1/Git-2.20.1-64-bit.exe
* <b>CMake (cmake-gui)</b>
  * https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-win64-x64.msi
* <b> Python </b> 
  * https://www.python.org/ftp/python/3.7.2/python-3.7.2-webinstall.exe
  * After installation, ensure that you can run python from console window, otherwise you need to add a system PATH to the python.exe
* <b> NSIS - Installer </b> (Note: Only if you intend to build installer packages)
  * http://prdownloads.sourceforge.net/nsis/nsis-3.04-setup.exe?download
 
### Build hyperscan library
There is no pre-built download for this component, hence you need to build it yourself. However to build it you need some other components first, Boost and Ragel.
* Download and install boost
    * https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/boost_1_69_0-msvc-14.1-64.exe/download        
* Fetch Ragel from GitHub (contains ragel.exe)
  * <b>git clone https://github.com/eloraiby/ragel-windows.git</b>
  * You do not need to build anything really, this repo contains a pre-built exe file that we use later on.  
* Fetch HyperScan from GitHub
  * <b>git clone https://github.com/intel/hyperscan.git</b> (v5.0.0)
* Configure and Generate the <b>hyperscan.sln</b> from cmake-gui
  * Start cmake-gui
  * Set source path to the top level CMakeLists.txt for HyperScan (in the git clone dir)
  * Add a build directory in the folder as where the source root is, set the CMake output directory to this (hyperscan/build)
  * Press <b>Configure</b> and select <b>Visual Studio 15 2017 Win64 </b> as Generator (Otherwise you will only be able to build 32-bit versions)
    * You will notice two remaining issues
      * Boost not found -> Manually "Add Entry" the <i>BOOST_ROOT</i> variable to the path of your BOOST installation
      * Ragel not found -> Manually set, also in CMake GUI, the <i>RAGEL</i> variable to the path of the ragel.exe (in your cloned Ragel repo)
    * Run Configure again with the above fixed
  * Press <b>Generate</b> to create the HyperScan *.sln project into the CMake output/build directory
* Build HyperScan from Visual Studio
  * Start Visual Studio 2017
  * Open the hyperscan.sln solution file from Visual Studio (in the cmake-gui output directory)
  * Select Release x64 build variant
  * From Build menu select "Build Solution" ... wait.... this takes a while...
* Install HyperScan using Visual Studio
  * In the VS solution there is a target INSTALL, build this target as Administrator. Typically you need to restart Visual Studio as Administrator to achieve this.
  * The hyperscan files will normally be installed in: C:/Program Files/hyperscan

### Build LogScrutinizer
* Fetch LogScrutinizer from GitHub 
  * <b>git clone https://github.com/logscrutinizer/dev.git</b>
* In the "dev" directory create/add file <b>local.cmake</b> with the following lines (<i>Change the paths to your local installation</i>)
  * <i>set(CMAKE_PREFIX_PATH "C:/Qt/5.10.0/msvc2017_64")
  * set(OS_VARIANT "Win32 64-bit")
  * set(CPACK_NSIS 1)
  * set(VC_REDIST "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Redist/MSVC/14.16.27012")
  * set(HS_PATH "C:/Program Files/hyperscan")</i>
* Configure and Generate the <b>LogScrutinizer.sln</b> from cmake-gui
  * Start cmake-gui
  * In cmake-gui, set source path to the "dev" folder and the output/build directory inside dev, e.g. dev/build.
  * Press <b>Configure</b> and select <b>Visual Studio 15 2017 Win64 </b> as Generator when the Generator selection pop-up shows.
  * Press <b>Generate</b> to create the LogScrutinizer.sln file into the cmake-gui output/build directory
* Build LogScrutinizer from Visual Studio 
  * Start Visual Studio 2017
  * Open the LogScrutinizer.sln solution file from the cmake-gui build directory
  * Select the LogScrutinizer target as "Set as Startup project"
  * Select <b>Release x64</b> build variant
  * Build solution
  * Press Ctrl-F5 to run (run Release version).

### Remarks
* If you intend run LogScrutinizer in <b>Debug</b> mode then you need to build both HyperScan and LogScrutinizer for Debug x64 (instead of Release). You need to re-install the HyperScan library after building for Debug.

