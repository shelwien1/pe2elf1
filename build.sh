#!/bin/sh
# Linux/POSIX build of coder0 (the Windows build is gc.bat).
#
# One translation unit: coder0.cpp includes the transformer's sources
# (tf/*.inc) directly, and every system header lives at the top of coder0.cpp.
#
# The transformer's numerics want IEEE semantics, so the fast-math options
# coder0 has always been built with are NOT used: -O3 with -ffp-contract=off
# rather than -Ofast.  The weights decoder reproduces CUDA's sinf/cosf
# bit-exactly, and -ffinite-math-only would delete the isinf test inside it.
set -e
CXX=${CXX:-clang++}
ARCH=${ARCH:--march=haswell -mtune=haswell}
OUT=${OUT:-coder0}

# Model switches (see the header of transformer.inc):
#   TFDEFS="-DTF_TRAIN=3"          train the whole model online
#   TFDEFS="-DTF_LOAD_WEIGHTS=0"   initialize the weights instead of loading
#   TFDEFS="-DTF_FP32=0"           the packed int4/int8 engine
TFDEFS=${TFDEFS:-}

$CXX -O3 -std=c++17 -fno-math-errno -ffp-contract=off -fomit-frame-pointer \
     -fno-stack-protector $ARCH -Wno-format $TFDEFS coder0.cpp -o "$OUT" -lm
echo "done: $OUT"
