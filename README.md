# LogScrutinizer
![LogScrutinizer Logo](http://www.logscrutinizer.com/img/Logo_4_256_256.png)
The LogScrutinizer is a tool for analyzing text-based logs.
Read more about it here: [LogScrutinizer Home](http://www.logscrutinizer.com)

__Key features__

* Create filters to view just matching rows in the log
* Multi-threaded for fastest search and filtering performance
* You can restrict which part of the log that should be analyzed with row clipping
* It tracks file tail for runtime logging
* You may develop plugins to visualize your data
  *  Line graphs
  *  Sequence charts
  *  Timing diagrams
* Based on Qt 6.2.2, runs on Linux and Windows platforms
* Uses the super-fast [hyperscan.io](https://www.hyperscan.io/) engine for regular expressions
* Handles large sized text-files, filters GB-sized logs within seconds

## Table of Contents
1. [Prebuild binaries](#prebuilt_binaries)
2. [Build instructions - Ubuntu 18.10 and Debian 9](#build_instructions_linux_ubuntu_debian)
3. [Build instructions - Centos 7](#build_instructions_centos7)
4. [Build instructions - Microsoft Windows](#build_instructions_windows)
5. [Contribute](#contribute)
   1. [Coding style guide](#coding_style_guide)
   2. [Contributor License Agreement](#cla)

# Prebuild binaries <a  name="prebuilt_binaries">
Visit [LogScrutinizer Home](http://www.logscrutinizer.com).
<p>Note: currently the official version is an older MFC based solution (from 2015) without e.g. hyperscan. However, if you download any of the 2.0 Beta versions you get the Qt-based multi-platform version. </p>

# Build instructions
This section contains build instructions for Linux and Windows. There are two examples of how to build for Linux, one that describe the easy path when using a newer dist such as Ubuntu 18.10 or Debian 9, and another example using an older dist (Centos 7) where a lot of the default tools are a bit older and needs to be updated/replaced.

## Build instructions - Linux (Ubuntu 18.10 and Debian9) <a name="build_instructions_linux_ubuntu_debian">
The two distributions Ubuntu 18.10 and Debian 9 have almost an identical step-by-step procedure for installation.
### Prerequisites
* Qt 6.2.2
  * Install requirements for [Development Host](http://doc.qt.io/qt-6/linux.html)
  * Download Qt from [Qt download page](https://www.qt.io/download)
    * You may choose the open-source alternative, as LogScrutinizer is open-source
    * The Qt Company requires that you have an account
    * You only need Desktop gcc 64-bit and default tools. Unselecting the others will speed up download.
* Git, CMake, Python and some more...
  * `sudo apt-get install git cmake python pkg-config  libhyperscan-dev freeglut3 freeglut3-dev libgtk2.0-dev`
* gcc/g++ with c++17 support
  * If your Linux dist has an older gcc without support for c++17 you need to update. If you want a newer gcc compiler you may check [added gcc build instructions](build_install_gcc) (section Centos7 but same for Debians)

### Build hyperscan library from source (optional)
This step is optional for new dists such as Ubuntu 18.10 and Debian 9, as hyperscan is available using apt (as libhyperscan-dev above). But, if you desire to use the latest version of hyperscan the same [instructions as for Centos 7 can be used](#hyperscan_build).

### Build LogScrutinizer
* Fetch LogScrutinizer from GitHub
  * `git clone https://github.com/logscrutinizer/dev.git`
* Create file __local.cmake__ in LogScrutinizer top-level folder
  * Add the following lines to __local.cmake__ (make sure that the paths are updated to your system setup/paths. For Debian 9 users you should change the OS_VARIANT).
  ```
  set(CMAKE_PREFIX_PATH "/home/user/Qt/6.2.2/gcc_64")
  set(LOCAL_PKG_CONFIG_PATH "/usr/lib/pkgconfig")
  set(OS_VARIANT "Ubuntu20")
  set(CPACK_DEBIAN 1)
  ```

#### build and run from terminal
* In the LogScrutinizer source folder (should be named dev after a git clone), create a __build__ folder
* Inside the build folder, type run the following from your terminal
  ```
  mkdir build && cd build
  cmake ..
  make -j4
  ./LogScrutinizer
  ```

#### Remarks
* Warning: If you generated an install package then you will find a qt.conf file in your build folder, that file will prevent you from
running the binary from the build folder, LogScrutinizer will look for Qt binaries in the install folders. Just remove the qt.conf file.


#### build from QtCreator
* Then from QtCreator, just open the top most CMakeLists.txt (in __dev__ folder)
* Configure the build folder and a target compiler
* F5


## Build instructions - Linux (Centos 7) <a name="build_instructions_centos7">

This build instruction example represent a Linux distribution with tools older than desired. You will find instructions how to build your own gcc compiler, cmake, and hyperscan.

### Prerequisites
* Qt 6.2.2
  * Install requirements for [Development Host](http://doc.qt.io/qt-5/linux.html)
    ```
    sudo yum groups mark convert
    sudo yum groupinstall "Development Tools"
    sudo yum install glibc-devel.i686
    sudo yum groupinstall "Additional Development"
    ```
  * Download Qt from [Qt download page](https://www.qt.io/download)
    * Note: You may choose the open-source alternative, as LogScrutinizer is open-source
    * Note: The Qt Company requires that you have an account
    * Note: You only need Desktop gcc 64-bit and default tools. Unselecting the others will speed up download.
* Python and git
  * `sudo yum install python git`
* gcc/g++ with c++17 support, [build/install instructions](#build_install_gcc)
* CMake [build/install instructions](#build_cmake)

### Build and install gcc 8.2 <a name="build_install_gcc">
From the tutorial [https://solarianprogrammer.com/2016/10/07/building-gcc-ubuntu-linux/](https://solarianprogrammer.com/2016/10/07/building-gcc-ubuntu-linux/).

```
cd ~/Downloads
wget https://ftpmirror.gnu.org/gcc/gcc-8.2.0/gcc-8.2.0.tar.gz
tar xf gcc-8.2.0.tar.gz
cd gcc-8.2.0
contrib/download_prerequisites
mkdir build && cd build
../configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=/usr/local/gcc-8.2 --enable-checking=release --enable-languages=c,c++,fortran --disable-multilib --program-suffix=-8.2
make -j8
sudo make install
```
The instructions above will build and install gcc 8.2. However, it will not replace the existing compiler, instead it will be installed as a complement in __/usr/local__. Because of this we must instruct CMake explicitly to use this compiler when configuring the source code before build LogScrutinizer.

### Build and install CMake <a name="build_cmake">
From [https://cmake.org/install](#https://cmake.org/install)
```
wget https://github.com/Kitware/CMake/releases/download/v3.13.3/cmake-3.13.3.tar.gz
tar xf cmake-3.13.3.tar.gz
./bootstrap
make
sudo make install
```

### Build hyperscan library from source <a name="hyperscan_build">
#### Prerequisites
* Boost, Download and build
  ```
  wget https://sourceforge.net/projects/boost/files/boost/1.69.0/boost_1_69_0.tar.gz
  tar -xzf boost_1_69_0.tar.gz
  cd boost_1_69_0
  ./bootstrap.sh --prefix=/opt/boost
  sudo ./b2 install --prefix=/opt/boost --with=all
  ```
* Ragel, Install
  ```
  sudo yum install epel-release
  sudo rpm -Uvh http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm
  sudo yum install ragel
  ```

#### Build
* Fetch HyperScan from GitHub
  * `git clone https://github.com/intel/hyperscan.git` (v5.0.0)  
* Build hyperscan
  ```
  cd hyperscan
  mkdir build && cd build
  cmake -DBOOST_ROOT=/opt/boost ..
  make -j4
  sudo make install
  ```

* Note: The install command will move hyperscan pkgconfig description into `usr/local/lib64/pkgconfig/libhs.pc` which will be used later on when running cmake for LogScrutinizer to locate hyperscan.
* Note: If you are an extensive user of regexpr then your should perhaps look a bit closer to the [build flags](https://github.com/intel/hyperscan/blob/master/doc/dev-reference/getting_started.rst) for hyperscan to get the most performance out of this excellent engine

### Build LogScrutinizer
* Fetch LogScrutinizer from GitHub
  * `git clone git@github.com:logscrutinizer/dev.git`
* Create __local.cmake__ in LogScrutinizer root folder (dev). 
  * Note: the local.cmake is not part of the repository since its content is dependent on your install directories
* Add the following lines to __local.cmake__ (make sure that the paths are updated to your system setup/paths).
  ```
  set(CMAKE_PREFIX_PATH "/home/user/Qt/6.2.2/gcc_64")
  set(LOCAL_PKG_CONFIG_PATH "/usr/local/lib64/pkgconfig")
  set(OS_VARIANT "Centos7")
  set(CPACK_RPM 1)
  ```
#### build from terminal
* In the LogScrutinizer top-level folder create a folder for the build files, and then run __cmake__ from there with the path to your gcc compiler.
  ```
  mkdir build && cd build
  cmake -DCMAKE_CXX_COMPILER=/usr/local/gcc-8.2/bin/x86_64-linux-gnu-g++-8.2 -DCMAKE_CC_COMPILER=/usr/local/gcc-8.2/bin/x86_64-linux-gnu-gcc-8.2 ..
  make -j4
  ```
#### build from QtCreator
* In QtCreator, since the default gcc compiler is too old, we need to instruct QtCreator to use g++8.2 and gcc8.2 instead. This is done by adding a platform-kit which uses your newer c and c++ compiler.
* Then from QtCreator, open the CMakeLists.txt file in the LogScrutinizer top-level folder
* QtCreate will ask which platform kit you should use, and you must now select the kit you just created (with gcc/g++ 8.2 as compilers).
* Press __F5__ to run and build

## Build instructions - Windows <a name="build_instructions_windows">

### Prerequisites
* __Qt (6.2.2) for Windows__
  * Download from [https://www.qt.io/download](https://www.qt.io/download)
    * You may choose the open-source alternative, as LogScrutinizer is open-source
    * The Qt Company requires that you have an account
    * You only need to enable the 2019 VisualStudio 64-bit build variant under 6.2.2 (to minimize the HDD impact)
    * > Add the Qt binary path as Win10 environment variable (PATH). To find the Qt dlls when running locally __(C:\Qt\6.2.2\msvc2019_64\bin)__
* __Visual Studio 2019__
  * Download from [https://visualstudio.microsoft.com/vs/community](https://visualstudio.microsoft.com/vs/community)
    * Check/Use "Desktop development with C++"
    * Note: When you select your Visual Studio version ensure that the Qt6 download has support for your version. VS19 match Qt6.2.2
* __Git__
  * Download from [git 2.34](https://github.com/git-for-windows/git/releases/download/v2.34.1.windows.1/Git-2.34.1-64-bit.exe)
* __CMake (cmake-gui)__
  * Download from [cmake 3.22](https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-windows-x86_64.msi)
* __Python__
  * Download from [3.7.2](https://www.python.org/ftp/python/3.7.2/python-3.7.2-webinstall.exe)
  * After installation, ensure that you can run python from console window, otherwise you need to add a system PATH to the python.exe
* __NSIS - Installer__ (Note: Only if you intend to build installer packages)
  * Download from [nsis-3.04](http://prdownloads.sourceforge.net/nsis/nsis-3.04-setup.exe?download)

### Build hyperscan library
There is no pre-built download for this component, hence you need to build it yourself. However, to build it you need some other components first, Boost and Ragel.
* Download and install boost
    * Download from [https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/boost_1_69_0-msvc-14.1-64.exe/download](https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/boost_1_69_0-msvc-14.1-64.exe/download)
      * Or just take the newest...
* Fetch Ragel from GitHub (contains ragel.exe)
  * `git clone https://github.com/eloraiby/ragel-windows.git`
  * You do not need to build anything really, this repo contains a pre-built exe file that we use later on.
* Fetch HyperScan from GitHub
  * `git clone https://github.com/intel/hyperscan.git` (v5.0.0)
* Configure and Generate the __hyperscan.sln__ from cmake-gui
  * Start cmake-gui
  * Set source path to the top level CMakeLists.txt for HyperScan (in the git clone dir)
  * Add a build directory in the folder as where the source root is, set the CMake output directory to this (hyperscan/build)
  * Press __Configure__ and select __Visual Studio 15 2017 Win64__ as Generator (Otherwise you will only be able to build 32-bit versions)
    * You will notice two remaining issues
      * Boost not found -> Manually "Add Entry" the _BOOST_ROOT_ variable to the path of your BOOST installation
      * Ragel not found -> Manually set, also in CMake GUI, the _RAGEL_ variable to the path of the ragel.exe (in your cloned Ragel repo)
    * Run Configure again with the above fixed
  * Press __Generate__ to create the hyperscan.sln project into the CMake output/build directory

* Build HyperScan from Visual Studio
  * Start Visual Studio 2019
  * Open the hyperscan.sln solution file from Visual Studio (in the cmake-gui output directory)
  * Select Release x64 build variant
  * From Build menu select "Build Solution" ... wait.... this takes a while...
* Install HyperScan using Visual Studio
  * In the VS solution there is a target INSTALL, build this target as Administrator. Typically you need to restart Visual Studio as Administrator to achieve this.
  * The hyperscan files will normally be installed in: C:/Program Files/hyperscan

### Build LogScrutinizer
* Fetch LogScrutinizer from GitHub
  * `git clone https://github.com/logscrutinizer/dev.git`
* In the "dev" directory create/add file __local.cmake__ with the following lines (_Change the paths to your local installation_)
    ```
    set(CMAKE_PREFIX_PATH "C:/Qt/6.2.2/msvc2017_64")
    set(OS_VARIANT "Win32 64-bit")
    set(CPACK_NSIS 1)
    set(VC_REDIST "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Redist/MSVC/14.16.27012")
    set(HS_PATH "C:/Program Files/hyperscan")
    ```

#### Building using Visual Studio (Generate sln-file from cmake-gui)
  * Configure and Generate the __LogScrutinizer.sln__ from cmake-gui
    * Start cmake-gui
    * In cmake-gui, set source path to the "dev" folder and the output/build directory inside dev, e.g. dev/build.
    * Press __Configure__ and select __Visual Studio 15 2017 Win64__ as Generator when the Generator selection pop-up shows.
    * Press __Generate__ to create the LogScrutinizer.sln file into the cmake-gui output/build directory
  * Build LogScrutinizer from Visual Studio
    * Start Visual Studio 2017
    * Open the LogScrutinizer.sln solution file from the cmake-gui build directory
    * ___Select the LogScrutinizer target as "Set as Startup project"__
    * Select __Release x64__ build variant
    * Build solution
    
  * Run LogScrutinizer from Visual Studio
    * LogScrutinizer is based on Qt6 and during startup loads several of Qt6 dlls. When running LogScrutinizer from VisualStudio these Qt6 dlls needs to the be located in the same folder as LogScrutinizer.exe. You typically find the Dlls in your Qt6 install directory under e.g.

    ```
    C:\Qt\6.2.2\msvc2019_64\bin

    ```
     * However, you also needs to create a __qt.conf__ file and locate it in the same folder as the exe and the dlls. The qt.conf file is used by the Qt6 dlls to locate other required binaries. Create the qt.conf file containing the following
     ``` 
      [Paths]
      Prefix = C:/Qt/6.2.2/msvc2019_64
    ```
   
    * Press Ctrl-F5 to run (run Release version).
    * Note, if you want to run in Debug you need to build HyperScan in Debug as well, and vice-versa for release. Then you run the HyperScan install project as well again to have it installed to "program files".

_Notes:_

* When building with Visual Studio some of the build targets are actually performing tests on the plugin examples. The plugin tester requires the Qt DLL to be located in the same directory as the exe. Hence, you will probarbly get a few build errors that is releated to this. You find the dll test application under: 
   ``` 
    ...\build\plugins\plugin_dev\plugin_tester\Debug
   ```
* You need to locate the qt dll into this folder for it to be able to execute. You also need to put the qt.conf there such that when the Qt dll loads it find the platform libraries.
* You need to create the qt.conf file yourself and locate it there, it should contain the same lines as for the qt.conf used for starting the LogScrutinizer application
   ```
   [Paths]
   Prefix = C:/Qt/6.2.2/msvc2019_64
   ```

#### Make an installation package

* Select the PACKAGE project in the solution file and build
Note, if the version gets wrong in the installation package its because the file product_version.cmake hasn't be re-created. Just remove it, build the gen_product_version project (where it will be re-created), and then build the PACKAGE project again.

#### Building using QtCreator
  * Start QtCreator
  * Open the top-most CMakeLists.txt file (QtCreator reads the CMake-files natively)
  * Configure you build platform, typically use the x64 variant.
  * Build
  * Note: You will need to install Debugger separately, instructions [http://doc.qt.io/qtcreator/creator-debugger-engines.html](http://doc.qt.io/qtcreator/creator-debugger-engines.html)

#### Remarks
* If you intend run LogScrutinizer in __Debug__ mode then you need to build both HyperScan and LogScrutinizer for Debug x64 (instead of Release). You need to re-install the HyperScan library after building for Debug.

* Visual Studio also support CMake natively, however currently the sub-targets are not shown preventing us from running these, e.g. packaging. As such, the best option if you want to use Visual Studio is to first generate the solution file with cmake-gui as described above.

# Contribute <a name="contribute">

## Coding style guide <a name="coding_style_guide">
The coding style guide is similar to the Qt style-guide, but not spot on. Use the uncrustify.cfg file for basic formatting, and <u>common sense for those parts not covered by it</u>. Make the code explain itself, do not format it to death, and most of all let your c++ inspiration flow. The version supported by the uncrustify.cfg file is __0.70.1__, but possible other versions are working as well (its about supporting the tags in the cfg-file).

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

