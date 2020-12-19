#!/bin/bash

cd build_cc
rm *.deb
rm *.rpm
rm install_script_gen.cmake
rm product_version.cmake
cmake .. -GNinja
ninja gen_product_version
cmake .. -GNinja
ninja
ninja gen_inst_scripts
ninja  .. -GNinja
ninja
ninja package

cd ../packaging
pwd

find ../build_cc/. -maxdepth 1 -regex ".*rpm$\|.*deb$" -print0 | xargs -0 -I file ./generate_download_install_script.py file
find ../build_cc/. -maxdepth 1 -regex ".*rpm.sh$\|.*deb.sh$" -print0 | xargs -0 -I file chmod +x file
cd ..
