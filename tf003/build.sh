#!/bin/sh
#
# build.sh - build coder0 on Linux (or MSYS2/MinGW bash). The Windows/clang
# build with the MSVC headers is gc.bat; this is the same translation unit with
# the same switches.
#
#   ./build.sh                      # Tangelo as the context model (the default)
#   USE_PPMD=1 ./build.sh           # PPMD instead, for comparison
#   TF_CHAIN=1 ./build.sh           # a second transformer fed the mixed distribution
#   CXX=clang++ ./build.sh
#   VERIFY="-DTRACK_VERIFY=32 -DTRACK_VERIFY_EVERY=200" ./build.sh
#
# One translation unit: coder0.cpp includes the transformer's sources (tf/*.inc)
# and, for the context model, ../tangelo_bm.inc - which brings in this
# repository's journal, walk and Tangelo model. Every system header lives at the
# top of coder0.cpp.
#
# The transformer's numerics want IEEE semantics, so -Ofast/-ffast-math are NOT
# used even though coder0 always was built with them: the weights decoder
# reproduces CUDA's sinf/cosf bit-exactly to rebuild the RoPE tables, and
# -ffinite-math-only would delete the isinf test inside it. -O3 with
# -ffp-contract=off is what the transformer was validated under, and the
# Tangelo half is integer arithmetic throughout, so it does not care either way.
#
# -march=haswell is the floor: the transformer kernels use AVX2/FMA/F16C.
set -eu
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
ARCH="${ARCH:--march=haswell}"
CXXFLAGS="${CXXFLAGS:--O3 -fno-math-errno -ffp-contract=off}"
TFDEFS="${TFDEFS:--DTF_TRAIN=0 -DTF_FP32=0}"
VERIFY="${VERIFY:-}"
OUT="${OUT:-coder0}"

DEFS="-DUSE_PPMD=${USE_PPMD:-0} -DTF_CHAIN=${TF_CHAIN:-0}"

echo "$CXX -std=c++17 $CXXFLAGS $ARCH $TFDEFS $DEFS $VERIFY -fpermissive -Wno-format coder0.cpp -o $OUT"
$CXX -std=c++17 $CXXFLAGS $ARCH $TFDEFS $DEFS $VERIFY \
     -fpermissive -Wno-format -Wno-unused-result coder0.cpp -o "$OUT"
