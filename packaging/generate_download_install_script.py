#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys
#import os.path
import subprocess
#!/bin/bash

def generate_download_install_script(package_file):

    extension = os.path.splitext(package_file)[1]
    package_filename = os.path.basename(package_file)
    package_folder_name = os.path.dirname(package_file)

    print("product_version: " +  package_folder_name  + "/product_version.cmake")

    # first parse the product version file and get the sub-path
    product_version_file = open(package_folder_name  + "/product_version.cmake")
    lines = product_version_file.readlines()
    for line in lines:
        if ("CPACK_PACKAGE_INSTALL_DIRECTORY" in line):
             sub_path = line[line.find("\"")+1:line.find("\"", line.find("\"")+1)]

    output_file = open(package_file + ".sh", 'w')
    output_file.write("#!/bin/bash\n")

    output_file.write("echo \"Downloading " + package_filename + "\"\n")
    output_file.write("wget www.logscrutinizer.com/downloads/" + package_filename + "\n")

    print("package: " + package_filename)
    print("extention " + extension)

    if (extension == ".deb"):
        output_file.write("if dpkg -l | grep logscrutinizer; then\n")
        output_file.write("    echo \"Uninstalling logscrutinizer...\"\n")
        output_file.write("    sudo dpkg -P logscrutinizer\n")
        output_file.write("fi\n")
    elif (extension == ".rpm"):
        output_file.write("if rpm -qa | grep logscrutinizer; then\n")
        output_file.write("    echo \"Uninstalling logscrutinizer...\"\n")
        output_file.write("    sudo rpm -e logscrutinizer\n")
        output_file.write("fi\n")

    output_file.write("echo \"If you want to check if LogScrutinizer is installed\"\n")
    output_file.write("echo \"you could test using\"\n")

    if (extension == ".deb"):
        output_file.write("echo \"> dpkg -l | grep logscrutinizer\"\n")
    elif (extension == ".rpm"):
        output_file.write("echo \"> rpm -qa | grep logscrutinizer\"\n")

    output_file.write("echo \"If you want to uninstall LogScrutinizer you can use\"\n")

    if (extension == ".deb"):
        output_file.write("echo \"> sudo dpkg -P logscrutinizer\"\n")
    elif (extension == ".rpm"):
        output_file.write("echo \"> sudo rpm -e logscrutinizer\"\n")

    output_file.write("echo \"Installing ... \"\n")

    if (extension == ".deb"):
        output_file.write("sudo dpkg -i " + package_filename + "\n")
    elif (extension == ".rpm"):
        output_file.write("sudo rpm -i " + package_filename + "\n")

    output_file.write("echo \"To start LogScrutinizer type  \"\n")
    output_file.write("echo \" \"\n")
    output_file.write("echo \"> /opt/logscrutinizer/" + sub_path + "/start.sh \"\n")
    output_file.write("echo \"\"\n")

    output_file.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        #                     1      2        3      4       5        6         7           8
        print("usage: %s package_file"%(sys.argv[0]))
    else:
        generate_download_install_script(sys.argv[1])
