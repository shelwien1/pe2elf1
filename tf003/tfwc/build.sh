#!/bin/sh
# build.sh - build the FX2TFWC2 <-> FX2TFWC3 converter. No dependencies: the
# container formats are implemented here, not linked from zmix.
set -eu
cd "$(dirname "$0")"
CXX="${CXX:-g++}"
echo "$CXX -O2 -std=c++17 -o 3to2 3to2.cpp"
$CXX -O2 -std=c++17 -Wall -Wextra -o 3to2 3to2.cpp
