
Hi there,
I'm extremly happy to find you here in the plugin readme, I guess you are thinking about writing
a new plugin for LogScrutinizer. This makes me truly excited and I will do my best to get you started. It is for sure
the plugins that makes LogScrutinizer really useful, and its you that can make that happen.

Everything you find in the plugin development kit is free for you to use, copy, and modify without any required
permissions.

In this folder (same as the readme file) you have a few examples and I belive that the fastest way to get your plugin
development started is to just copy one of these examples that best match your needs.

The most common use of a LogScrutinizer plugin is to graphically show the content of a log-file. An example is to plot
the value of a variable, pehaps it is the temperature of a sensor. Or perhaps drawing boxes to enlighten the start and
stop of a processing thread.

Currently the API supplied in the dev kit is based on the Qt framework and c++ v14. Hence it is an advantage if you
have some knowledge of c++. You will need to get your hands on the Qt framework and a pretty new c++ compiler, such
as gcc v7 or Visual Studio Community 2017. To generate the build environment you will as well need to use cmake.

I guess that the first bump is to get all the requirements installed, but please make the effort as you will be well
rewarded when finding your first plugin doing its first plots.

Requirements
------------
Windows - Visual Studio Community 2017 (or newer)
Linux - gcc 7 (or newer). If you do not have gcc 7 there are plenty of help on the Internet
CMake
Qt v5.10

Besides the examples you will find that the plugin_frame folder is pretty important as it contains the API in which the
plugins interacts with LogScruitinizer. When the plugin gets loaded, as either a shared object (Linux), or a DLL in
Microsoft Windows, the plugin gets called by LogScrutinizer through a set of mandatory API functions. These function
calls will then register the plugin interfaces to LogScrutinizer. In order for this to work the interface must be
binary compatible with LogScrutinizer and care should be taken if changes are made to the framework itself.

You are recommended NOT to modify any files in the plugin_framework folder, as these tend to change with new
LogScrutinizer and plug-in kit versions. Later, if you don't modify the framework files, it should be a simple copy
and replace action required when a new plug-in kit version is available/downloaded.

--------- SHORT STEP-BY-STEP HOW TO BUILD THE PLUG-INs ----------

1. First, we must help CMake find the Qt installation.
   Modify the 

     -->>>  local.cmake  <<<---- 

  (same folder as the readme) such that it points to your Qt installation, mine contains:

   On Linux:     set(CMAKE_PREFIX_PATH "/home/klang/Qt/5.10.0/gcc_64")
   On Windows    set(CMAKE_PREFIX_PATH "C:/Qt/5.10.0/msvc2017_64")

2. Now, we are ready to run CMake, this however might differ a bit if you are on a Window or Linux machine. Mainly
   depending if you will run CMake from command line or from the cmake-gui

WINDOWS users typically run cmake-gui (works on Linux as well)
--------------------------------------------------------------
W2.1 Start the cmake-gui
W2.2 On the row "Where is the source code" you put the path to the top folder with examples and framework.
   Example:  c:\users\klang\plugin_dev
W2.3 On the row "Were to build the binaries" you put the same path as source code but add build as final folder.
   Example:  c:\users\klang\plugin_dev\build
W2.4 Press Configure button
   This will go through the CMake file and verify it
W2.5 Press Generate button
   This will generate the Visual Studio *.sln file, and it is put into the build folder you specified.
W2.6 From Visual Studio, open the *.sln file located in the build folder
W2.7 When Visual Studio has loaded the project you may select the ALL_BUILD target and build. All examples
   now will be built.

LINUX users typically run from command line (works on Windows as well, e.g. from Cygwin)
----------------------------------------------------------------------------------------
L2.1 Create a build folder at the same folder level as the plugin_framework (and the exmaples).
L2.2 From inside the build-folder you run (do not forget the to dots... means run cmake on CMakeLists.txt in folder above)
   > cmake ..
   This will then generate the build environment.
L2.3 From the build folder run
   > make
   This will build all the examples

Now, for both WINDOWS and LINUX users the build directory has been populated with all examples built. On a windows
machine you will find a lot of dll files in the build/plugin_example_xx folders, and for Linux users there will
instead be so-files.

3. To tryout a plugin you simply Drag-and-drop the file to LogScrutinizer.

3.1 In the plugin-dev folder, for each example there is a text-file (test_data.txt) that can be used together with the
    plugin (so or dll file) to demonstrate its features.

4. In the LogScrutinizer workspace you will find that you have new entries in the workspace tree, plugin. Under
   a plugin select the plot name, right click, and select run (if it is a plug-in with plot capabilities)

TADA... now you should see a plot...


TIPS
----

1. I most of the times use the plugin-tester when developing plugins. The plugin-tester supplied in the
   development kit, its a very easy application that loads a plugin just as how LogScrutnizer does it internally. Hence,
   if you have success being called by the plugin-tester there are high chanses that your plugin will work.
   The plugin-tester can also be used to test how your plugin parses text, and which objects it creates. Provide the
   plugin-tester with a text file and it should run through your plugin and validate which objects it creates.

FAQ
----

Q: I get compile errors, its says its doesn't recognise the std=c++17 compile switch.
A: Well, this means that you probarbly hasn't installed a recent enough gcc compiler, or that CMake doesn't
   locate the correct one. I typically use QtCreator when developing and when I configure my projects I can choose
   where the compiler for gcc and g++ should be picked.


UNDER CONSTRUCTION

--------- DEBUGGING PLUG-INS ----------

To ease the plug-in debugging each of the example plug-ins has a testApp directory. This contains a similar
implementation compared to how LogScrutinizer runs a plug-in.

Hence, by building and modifying the testApp, according to how you want to test you plug-in, you can then set
break-points in you plug-in and debug line-by-line in developer studio.

--------- EXAMPLE DESCRIPTIONS ----------

Folder: Plugin_Example_1
Description:
  This plug-in example shows the minimum implementation to create a plug-in with PLOT capabilities drawing lines.

Folder: Plugin_Example_2
Description:
  This plug-in example shows the minimum implementation to create a plug-in with DECODER capabilities.

 Folder: Plugin_Example_3
Description:
  This plug-in example shows an implementation with PLOT capabilities drawing different variant of lines and boxes in
  several sub-plots.
     subPlot_1, boxes with different colors and labels
     subPlot_2, lines with different colors and labels
     subPlot_3, graphs with overriden color, preventing LogScrutinizer to assign them colors automatically

 Folder: Plugin_Example_3_1
Description:
  This plug-in example shows an implementation with PLOT capabilities drawing non-coherent lines

Folder: Plugin_Example_4
Description:
  This plug-in example shows an implementation using sequence diagrams

Folder: Plugin_Example_5
Description:
  This plug-in example shows the minimum implementation to create a plug-in with PLOT capabilities drawing lines, and
  implementing the callback function to provide additional information about the graphical objects in the plot when the
  user is hovering over it.

--------- HELP AND SUPPORT ----------

Ask questions here
http://www.logscrutinizer.com

