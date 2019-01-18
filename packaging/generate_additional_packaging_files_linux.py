#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys
import subprocess

def ldd_parse_ldd(filename):
    so_names = []
    lib_names = []
    skip_list = ["libc", "libpthread", "libd", "libm"]
    # libc-2.17.so  libc.so.6  libdl-2.17.so  libdl.so.2  libpthread-2.17.so  libpthread.so.0

    for x in filename:
        print(x)
        p = subprocess.Popen(["ldd", x],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        result = p.stdout.readlines()

        for x in result:
            s = x.split()
            if "=>" in x:
                if len(s) == 3: # virtual library
                    print("GEN_PRINT Bad len :: " + s[0])
                    pass
                else:
                    if (s[0] not in so_names):
                        if any(skip in s[0] for skip in skip_list):
                            print("Skip " + s[0])
                        else:
                            print("GEN_PRINT Added :: " + s[0] + " @ " + os.path.realpath(s[2]))
                            lib_names.append(os.path.realpath(s[2]))
                            so_names.append(s[0])

    return lib_names, so_names

def generate_install_libs(filename, libs):
    thefile = open(filename, 'w')
    thefile.write("set(INSTALL_LIBS \"")
    print(libs)

    for element in libs:
        thefile.write(element + ";")

    thefile.write("\")")
    thefile.close()

def generate_postinst(filename, libs, so, dest_path):
    thefile = open(filename, 'w')
    thefile.write("#!/bin/bash\n")
    for i in range(len(libs)):
        lib_dest = os.path.join(dest_path + "/libs", os.path.basename(libs[i]))
        so_dest = os.path.join(dest_path + "/libs", so[i])
        if (lib_dest != so_dest):
            thefile.write("ln -s " + lib_dest + " " + so_dest + "\n")
    thefile.write("chmod +x " + dest_path + "/start.sh\n")
    thefile.close()

def generate_prerm(filename, so, dest_path):
    thefile = open(filename, 'w')
    thefile.write("#!/bin/bash\n")
    thefile.write("rm -rf " + dest_path + "\n")
    thefile.close()

def generate_qt_conf(filename, dest_path):
    thefile = open(filename, 'w')
    thefile.write("[Paths]\n")
    thefile.write("Prefix = " + dest_path + "\n")
    thefile.write("Libraries = libs" + "\n")
    thefile.write("libexec = libs" + "\n")
    thefile.write("Plugins = plugins" + "\n")
    thefile.close()

def generate_start_sh(filename, dest_path):
    thefile = open(filename, 'w')
    thefile.write("#!/bin/sh\n")
    thefile.write("export LD_LIBRARY_PATH=" + dest_path + "/libs\n")
    thefile.write("export QT_QPA_PLATFORM_PLUGIN_PATH=" + dest_path + "/plugins\n")
    thefile.write(dest_path + "/LogScrutinizer\n")
    thefile.close()

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        #                    1      2        3      4       5        6        7 ... X - 1           X
        print("usage: %s ldd.cmake postinst postrm qt.conf start.sh dest-path platform-plugin-files path-to-ls-exec "%(sys.argv[0]))
    else:
        lib_names, so_names =  ldd_parse_ldd(sys.argv[7:])
        generate_install_libs(sys.argv[1], lib_names)
        generate_postinst(sys.argv[2], lib_names, so_names, sys.argv[6])
        generate_prerm(sys.argv[3], so_names, sys.argv[6])
        generate_start_sh(sys.argv[5], sys.argv[6])
        generate_qt_conf(sys.argv[4], sys.argv[6])
