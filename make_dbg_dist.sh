#!/bin/bash

cd build_dbg
rm *.deb
rm *.rpm
rm install_script_gen.cmake
rm product_version.cmake

cmake -DCMAKE_BUILD_TYPE=Debug ..
make gen_product_version
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j12
make gen_inst_scripts
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j12
make package

cd ../packaging
pwd

find ../build_cc/. -maxdepth 1 -regex ".*rpm$\|.*deb$" -print0 | xargs -0 -I file ./generate_download_install_script.py file
find ../build_cc/. -maxdepth 1 -regex ".*rpm.sh$\|.*deb.sh$" -print0 | xargs -0 -I file chmod +x file
cd ..
