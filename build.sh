#!/bin/sh
# Linux/gcc build. Pass extra -D flags to override rc_config.inc, e.g.
#   ./build.sh -DRCNUM=8 -DRC_LOWBYTES=6
set -e

# OPENCL=1 (the default when an ICD loader is present) builds the device path;
# OPENCL=0 leaves it out entirely and links against nothing extra.
CXX=${CXX:-g++}
OPTS="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-rtti -fno-exceptions -fstrict-aliasing"
INCS="-DNDEBUG -I. -ILib3"
DIRNAM=$(basename "$(pwd)")

if [ -z "$OPENCL" ]; then
  if [ -e /usr/include/CL/cl.h ]; then OPENCL=1; else OPENCL=0; fi
fi
if [ "$OPENCL" = 1 ]; then
  CLDEF="-DRC_OPENCL=1"; CLLIB="-lOpenCL"
else
  CLDEF="-DRC_OPENCL=0"; CLLIB=""
fi

$CXX -std=gnu++11 -O3 $INCS $OPTS $CLDEF "-D__DIRNAM__=$DIRNAM" "$@" \
     coder.cpp FSM.cpp rc_cl.cpp misc/model0.cpp misc/model1.cpp misc/timer.cpp \
     $CLLIB -o coder

echo "built ./coder (opencl=$OPENCL)"
