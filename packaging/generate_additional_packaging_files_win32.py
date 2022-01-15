#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys
import subprocess
import ntpath

def generate_qt_conf(filename):
    thefile = open(filename, 'w')
    thefile.write("[Paths]\n")
    thefile.write("Plugins = plugins" + "\n")
    thefile.close()

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        print(f"usage: {sys.argv[0]} qt.conf")
    else:
        generate_qt_conf(sys.argv[1])