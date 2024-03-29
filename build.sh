#!/bin/bash


rm -rf lib
rm -rf build

mkdir build
cd build

rm CMakeCache.txt 
TOOLCHAIN="ARM64"
 echo "toolchain is arm64"
cmake .. -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_SHARED_LIBS=OFF" "-DCMAKE_SYSTEM_PROCESSOR=aarch64" 

make -B -j5
cmake .. "-DCMAKE_BUILD_TYPE=debug" 
make -B -j5

export LC_ALL=C
unset LANGUAGE

cd ..

# if [ $TOOLCHAIN == "LINUX" ]; then 
# echo "unittest for linux"
#     if [ -f "./build/bin/x64-linux/unittest" ]; then
#         ./build/bin/x64-linux/unittest
#     fi
# fi 
rm -rf build