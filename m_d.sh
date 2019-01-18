#!/bin/bash


cd packaging

find ../build_cc/. -maxdepth 1 -regex ".*rpm$\|.*deb$" -print0 | xargs -0 -I file ./generate_download_install_script.py file
cd ..
