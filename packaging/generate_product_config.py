#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys
import subprocess

#define APP_VER_CODE_KIND_BETA     (0x00)
#define APP_VER_CODE_KIND_RC       (0x01)         // Relase candidate
#define APP_VER_CODE_KIND_FINAL    (0x02)
#define APP_VER_CODE_KIND_PATCH    (0x03)

#define APP_VER_CODE_MAJOR         2                          // If 1: -> v1
#define APP_VER_CODE_MINOR         0                          // If 4: -> vx.4
#define APP_VER_CODE_KIND          APP_VER_CODE_KIND_BETA     // BETA, RC, Patch, Final
#define APP_VER_CODE_SUB_VERSION   0                          // If 1 : -> vx.x B1  (beta),  vx.x.1 (patch), vx.x RC1 (Release candidate)
#define APP_VER_CODE_BUILD         2                          // If 1 : -> vx.x Bx build1
#SET(CPACK_PACKAGE_VERSION_MAJOR "2")
#SET(CPACK_PACKAGE_VERSION_MINOR "0")
#SET(CPACK_PACKAGE_VERSION_PATCH "2")
#SET(CPACK_PACKAGE_FILE_NAME "LogScrutinizer_v2_0_B0_build2-Linux-64bit") #package name override


def generate_ls_versions_file(filename, config_hfile, build_variant):

    configfile = open(config_hfile, 'r')
    lines = configfile.readlines()
    for line in lines:
        parts = line.split()
        if (len(parts) >= 3):
            if (parts[0] == "#define" and parts[1] == "APP_VER_CODE_MAJOR"):
                major = parts[2]
                print("Major:" + major)
            if (parts[0] == "#define" and parts[1] == "APP_VER_CODE_MINOR"):
                minor = parts[2]
                print(" Minor:" + minor)
            if (parts[0] == "#define" and parts[1] == "APP_VER_CODE_KIND"):
                kind = parts[2]
                print(" Kind:" + kind)
            if (parts[0] == "#define" and parts[1] == "APP_VER_CODE_SUB_VERSION"):
                sub_version = parts[2]
                print(" Subversion:" + sub_version)
            if (parts[0] == "#define" and parts[1] == "APP_VER_CODE_BUILD"):
                build = parts[2]
                print(" build:" + build)
    configfile.close()

    version = "v" + major + "." + minor
    patch = "0"

    if (kind == "APP_VER_CODE_KIND_BETA"):
        version += " B" + sub_version
    elif (kind == "APP_VER_CODE_KIND_RC"):
        version += " RC" + sub_version
    elif (kind == "APP_VER_CODE_KIND_PATCH"):
        version += "." + sub_version
        patch = sub_version

    if (build != "0"):
        version += " build" + build

    app_name = "LogScrutinizer " + version
    package_name = app_name.lower()
    package_name = package_name.replace(" ", "_")
    package_name = package_name.replace(".", "_")
    package_name += "-" + build_variant

    install_folder = version.lower()
    install_folder = install_folder.replace(" ", "_")
    install_folder = install_folder.replace(".", "_")


    output_file = open(filename, 'w')
    output_file.write("set (CPACK_PACKAGE_FILE_NAME \"" + package_name + "\")\n")
    output_file.write("set (CPACK_PACKAGE_VERSION_MAJOR \"" + major + "\")\n")
    output_file.write("set (CPACK_PACKAGE_VERSION_MINOR \"" + minor + "\")\n")
    output_file.write("set (CPACK_PACKAGE_VERSION_PATCH \"" + patch + "\")\n")
    output_file.write("set (APPLICATION_FULL_NAME \"" + app_name + "\")\n")
    output_file.write("set (CPACK_PACKAGE_INSTALL_DIRECTORY \"" + install_folder + "\")\n")
	
    output_file.close()

    print("package_name: " + package_name)
    print("app_name: " + app_name)
    print("install_folder: " + install_folder)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        #                     1      2        3      4       5        6         7           8
        print("usage: %s output.cmake CConfig.h -Ubuntu16"%(sys.argv[0]))
    else:
        generate_ls_versions_file(sys.argv[1], sys.argv[2], sys.argv[3])
