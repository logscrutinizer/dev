# LogScrutinizer
![LogScrutinizer Logo](http://www.logscrutinizer.com/img/Logo_4_256_256.png)
The LogScrutinizer is a tool for analyzing text-based logs.
Read more about it here: [LogScrutinizer Home](http://www.logscrutinizer.com)

<p><b> Key features </b></p>

* Create filters to view just matching rows in the log
* Multi-threaded for fastest search and filtering performance
* You can restrict which part of the log that should be analyzed with row clipping
* It tracks file tail for runtime logging
* You may develop plugins to visualize your data
  *  Line graphs
  *  Sequence charts
  *  Timing diagrams
* Based on Qt 5.10, runs on Linux and Windows platforms
* Uses the super-fast [hyperscan.io](https://www.hyperscan.io/) engine for regular expressions
* Handles large sized text-files, filters GB-sized logs within seconds

## Table of Contents
1. [Prebuild binaries](#prebuilt_binaries)
2. [Build instructions - Ubuntu 18.10](#build_instructions_linux_ubuntu18_10)
3. [Build instructions - Microsoft Windows](#build_instructions_windows)
4. [Contribute](#contribute)
   1. [Coding style guide](#coding_style_guide)
   2. [Contributor License Agreement](#cla)


# Prebuild binaries <a  name="prebuilt_binaries">
Visit [LogScrutinizer Home](http://www.logscrutinizer.com).
<p>Note: currently the official version is an older MFC based solution (from 2015) without e.g. hyperscan. However, if you download any of the 2.0 Beta versions you get the Qt-based multi-platform version. </p>

# Build instructions

--------------------------------------------------------------------------------------------------
## Build instructions - Linux (Ubuntu 18.10) <a name="build_instructions_linux_ubuntu18_10">
### Prerequisites
* Qt 5.10
  * Install requirements for [Development Host](http://doc.qt.io/qt-5/linux.html)
    * Note: Ubuntu 18.10 has gcc version 8, which is great as you need support for c++14. For other older versions of Ubuntu you will need to download and build gcc with a version 7.3 or newer.
  * Download Qt from [Qt download page](https://www.qt.io/download)
    * You may choose the open-source alternative, as LogScrutinizer is open-source
    * The Qt Company requires that you have an account
    * You only need Desktop gcc 64-bit and default tools. Unselecting the others will speed up download.
* Git, CMake, Python and some more...
  * `sudo apt-get install git cmake python pkg-config  libhyperscan-dev freeglut3 freeglut3-dev libgtk2.0-dev`

### Build hyperscan library from source (optional)
This step is optional for Ubuntu, as hyperscan is available using apt (as libhyperscan-dev above). But, if you desire to use the latest version of hyperscan the following will help you build it.
* First, download hyperscan prerequisites
  * sudo apt-get install ragel libboost-all-dev
* Fetch HyperScan from GitHub
  * `git clone https://github.com/intel/hyperscan.git` (v5.0.0)
* Build hypercan
  * In hyperscan source/root folder, create a <b>build</b> folder
  *  Step into build folder and run
    *  `cmake ..`
    *  `make -j4`
  * Note: If you are an extensive user of regexpr then your should perhaps look a bit closer to the [build flags](https://github.com/intel/hyperscan/blob/master/doc/dev-reference/getting_started.rst) for hyperscan to get the most performance out of this excellent engine

### Build LogScrutinizer
#### from terminal
* Create <b>local.cmake</b> in LogScrutinizer root folder (dev)
* Add the following lines to <b>local.cmake</b> (make sure that the paths are updated to your system setup/paths).
  ```
  set(CMAKE_PREFIX_PATH "/home/klang/Qt/5.10.0/gcc_64")
  set(LOCAL_PKG_CONFIG_PATH "/usr/lib/pkgconfig")
  set(OS_VARIANT "Ubuntu18")
  set(CPACK_DEBIAN 1)
  ```
* In the LogScrutinizer source folder (should be named dev after a git clone), create a <b>build</b> folder
* Inside the build folder, type run the following from your terminal
  * `cmake ..`
  * `make -j4`

#### from QtCreator
* Do as above and create the <b>local.cmake</b> with the specified lines.
* Then from QtCreator, just open the top most CMakeLists.txt (in dev folder)
* Configure the build folder and a target compiler

--------------------------------------------------------------------------------------------------
## Build instructions - Windows <a name="build_instructions_windows">

### Prerequisites
* <b>Qt (5.10.0) for Windows</b>
  * Download from [https://www.qt.io/download](https://www.qt.io/download)
    * You may choose the open-source alternative, as LogScrutinizer is open-source
    * The Qt Company requires that you have an account
    * You only need to enable the 2015/2017 VisualStudio 64-bit build variants under 5.10.0 (to minimize the HDD impact)
    * > Add the Qt binary path as Win10 environment variable (PATH). To find the Qt dlls when running locally <b>(C:\Qt\5.10.0\msvc2017_64\bin)</b>
* <b>Visual Studio 2017</b>
  * Download from [https://visualstudio.microsoft.com/vs/community](https://visualstudio.microsoft.com/vs/community)
    * Check/Use "Desktop development with C++"
* <b>Git</b>
  * Download from [https://github.com/git-for-windows/git/releases/download/v2.20.1.windows.1/Git-2.20.1-64-bit.exe](https://github.com/git-for-windows/git/releases/download/v2.20.1.windows.1/Git-2.20.1-64-bit.exe)
* <b>CMake (cmake-gui)</b>
  * Download from [https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-win64-x64.msi](https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3-win64-x64.msi)
* <b> Python </b>
  * Download from [https://www.python.org/ftp/python/3.7.2/python-3.7.2-webinstall.exe](https://www.python.org/ftp/python/3.7.2/python-3.7.2-webinstall.exe)
  * After installation, ensure that you can run python from console window, otherwise you need to add a system PATH to the python.exe
* <b> NSIS - Installer </b> (Note: Only if you intend to build installer packages)
  * Download from [http://prdownloads.sourceforge.net/nsis/nsis-3.04-setup.exe?download](http://prdownloads.sourceforge.net/nsis/nsis-3.04-setup.exe?download)

### Build hyperscan library
There is no pre-built download for this component, hence you need to build it yourself. However to build it you need some other components first, Boost and Ragel.
* Download and install boost
    * Download from [https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/boost_1_69_0-msvc-14.1-64.exe/download](https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/boost_1_69_0-msvc-14.1-64.exe/download)
* Fetch Ragel from GitHub (contains ragel.exe)
  * `git clone https://github.com/eloraiby/ragel-windows.git`
  * You do not need to build anything really, this repo contains a pre-built exe file that we use later on.
* Fetch HyperScan from GitHub
  * `git clone https://github.com/intel/hyperscan.git` (v5.0.0)
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
  * `git clone https://github.com/logscrutinizer/dev.git`
* In the "dev" directory create/add file <b>local.cmake</b> with the following lines (<i>Change the paths to your local installation</i>)
    ```
    set(CMAKE_PREFIX_PATH "C:/Qt/5.10.0/msvc2017_64")
    set(OS_VARIANT "Win32 64-bit")
    set(CPACK_NSIS 1)
    set(VC_REDIST "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Redist/MSVC/14.16.27012")
    set(HS_PATH "C:/Program Files/hyperscan")
    ```
#### Building using Visual Studio (Generate sln-file from cmake-gui)
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

#### Building using QtCreator
  * Start QtCreator
  * Open the top-most CMakeLists.txt file (QtCreator reads the CMake-files natively)
  * Configure you build platform, typically use the x64 variant.
  * Build
  * Note: You will need to install Debugger separately, instructions [http://doc.qt.io/qtcreator/creator-debugger-engines.html](http://doc.qt.io/qtcreator/creator-debugger-engines.html)

#### Remarks
* If you intend run LogScrutinizer in <b>Debug</b> mode then you need to build both HyperScan and LogScrutinizer for Debug x64 (instead of Release). You need to re-install the HyperScan library after building for Debug.

* Visual Studio also support CMake natively, however currently the sub-targets are not shown preventing us from running these, e.g. packaging. As such, the best option if you want to use Visual Studio is to first generate the solution file with cmake-gui as described above.

--------------------------------------------------------------------------------------------------
# Contribute <a name="contribute">

## Coding style guide <a name="coding_style_guide">
The coding style guide is similar to the Qt style-guide, but not spot on. Use the uncrustify.cfg file for basic formatting, and <u>common sense for those parts not covered by it</u>. Make the code explain itself, do not format it to death, and most of all let your c++ inspiration flow. The version supported by the uncrustify.cfg file is <b>0.68-2</b>, but possible other versions are working as well (its about supporting the tags in the cfg-file).

### Linux
To use the uncrustify.cfg to format the code you need to download and build the uncrustify program. Then you configure QtCreator to enable the uncrustify as a beautifyer, start from help and plugins.
```
git clone https://github.com/uncrustify/uncrustify.git
mkdir build
cd build
cmake ..
make -j12
sudo make install
```

### Windows
* Download for Win32: [https://sourceforge.net/projects/uncrustify](https://sourceforge.net/projects/uncrustify)
* In Visual Studio, add external command:
  * Command: "Path to uncrustify.exe"
  * Arguments: -c  $(SolutionDir)..\uncrustify.cfg  --no-backup  $(ItemPath)
  * Initial directory: $(ItemDir)

## Contributor License Agreement - CLA <a name="cla">
<p> The purpose of a CLA is that when you contribute, e.g. c++ code, to the LogScrutinizer project you will automatically transfer your ownership of that contribution to the project such that it will be possible to continue to distribute the project with <a href="https://www.gnu.org/licenses/gpl-3.0.en.html">GPL 3</a>. It also enables an easier transition for distribution under a different licensing model in case of a fork. </p>
<p> Additionally, the CLA highlights that if you are contributing while working for a company that company will not own that piece of code, rather it has the same rights as all others enjoying GPL 3 software.</p>
<p> Send an email to robert.klang@softfairy.com for more information. </p>


