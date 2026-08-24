#!/bin/sh
# mkraw.sh -- build rawadpcm.  One source file, nothing but a C++ compiler,
# nothing to find at run time.
set -e
CXX=${CXX:-g++}
$CXX -std=gnu++17 -O2 -fomit-frame-pointer ${ARCH:-} rawadpcm.cpp -o rawadpcm
echo "built rawadpcm"
