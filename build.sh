#!/bin/sh
# Linux/gcc build. Pass extra -D flags to override rc_config.inc, e.g.
#   ./build.sh -DRCNUM=8 -DRC_LOWBYTES=6
set -e

CXX=${CXX:-g++}
OPTS="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-rtti -fno-exceptions -fstrict-aliasing"
INCS="-DNDEBUG -I. -ILib3"
DIRNAM=$(basename "$(pwd)")

$CXX -std=gnu++11 -O3 $INCS $OPTS "-D__DIRNAM__=$DIRNAM" "$@" \
     coder.cpp FSM.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp -o coder

echo "built ./coder"
