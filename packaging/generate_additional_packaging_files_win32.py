#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys
import subprocess
import ntpath

def generate_install_libs(filename, libs):
    pruned = []
    pruned = libs
    
    skip_list = ["Qt53D", "Multimedia", "Game", "Sensors", "compiler", "ScriptTools", "Sql", "Svg", "Purchas", "Scxml", "Serial", "Speech", "RemoteObjects", "Web", "Positioning", "Nfc", "Qt5WebEngine", "Qt5Quick", "Qt5Qml", "Qt5Bluetooth", "Qt5XmlPatterns", "Qt5Designer", "Qt5Location",  "Qt5QuickTemplates2", "Qt5QuickParticles", "Qt5DataVisualization", "Qt5Charts"]

    # Extract only the libs (after @@@)
    index = 0
    while (libs[index] != "@@@"):
        index = index + 1
    pruned = libs[(index + 1):]

    for element in skip_list:
        pruned = filter (lambda s: not (element in s), pruned)

    for element in libs:
        basename = ntpath.basename(element)
        name, ext = os.path.splitext(basename)
        named = name + 'd'
        pruned = filter( lambda s: not (named in s), pruned)

    print("Pruned: ")
    print(pruned)

    print("To file " + filename)
    thefile = open(filename, 'w')
    thefile.write("set(INSTALL_LIBS \"")

    for element in pruned:
        thefile.write(element + ";")

    thefile.write("\")")
    thefile.close()

def generate_platform_plugin(filename, libs):
    pruned = []
    pruned = libs
    
    skip_list = ["qwebgl"]

    # Extract only the libs (before @@@)
    index = 0

    while (libs[index] != "@@@"):
        index = index + 1
    pruned = libs[:(index)]

    for element in skip_list:
        pruned = filter (lambda s: not (element in s), pruned)

    for element in libs:
        basename = ntpath.basename(element)
        name, ext = os.path.splitext(basename)
        named = name + 'd'
        pruned = filter( lambda s: not (named in s), pruned)

    print("Pruned: ")
    print(pruned)

    print("To file " + filename)
    thefile = open(filename, 'w')
    thefile.write("set(QT_PLATFORM_PLUGIN \"")

    for element in pruned:
        thefile.write(element + ";")

    thefile.write("\")")
    thefile.close()

def generate_qt_conf(filename):
    thefile = open(filename, 'w')
    thefile.write("[Paths]\n")
    thefile.write("Plugins = plugins" + "\n")
    thefile.close()

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        #                    1                  2                  3      4         
        print("usage: %s install_lib.cmake platform_plugin.cmake qt.conf platform-plugin-files @@@ libs "%(sys.argv[0]))
    else:
        generate_install_libs(sys.argv[1], sys.argv[4:])
        generate_platform_plugin(sys.argv[2], sys.argv[4:])
        generate_qt_conf(sys.argv[3])