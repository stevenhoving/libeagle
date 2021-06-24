#!/bin/sh

rm -rf vs_build
mkdir vs_build
cd vs_build
cmake ../ -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=${QT_PATH}
cd ..
