#!/bin/sh
#
# build.sh - build fpaq0mw with GCC or Clang (Linux; also MSYS2/MinGW bash).
#
#   CXX=clang++ ./build.sh                        # pick the compiler
#   CXXFLAGS="-O3 -ffast-math" ./build.sh         # replace the default flags
#   CXX=x86_64-w64-mingw32-g++ EXE=fpaq0mw.exe ./build.sh   # cross-build for Windows
#
# The three steps mirror the original g.bat:
#   1. compile the model step (encode_sim) to Intel-syntax assembly,
#   2. let track.pl insert a journaling call in front of every store,
#   3. build the executable from the main source plus the instrumented assembly.
set -eu
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
CXXFLAGS="${CXXFLAGS:--O3 -ffast-math -fomit-frame-pointer -fno-rtti -fno-exceptions -fno-stack-protector -march=native}"
LDFLAGS="${LDFLAGS:-}"
EXE="${EXE:-fpaq0mw}"

rm -f coder.s coder1.s "$EXE" timetest timetest.exe

echo "[1/3] $CXX -S -masm=intel -DSIM_FUNC fpaq0mw.cpp -o coder.s"
$CXX $CXXFLAGS -S -masm=intel -DSIM_FUNC fpaq0mw.cpp -o coder.s

echo "[2/3] perl track.pl coder.s coder1.s"
perl track.pl coder.s coder1.s

echo "[3/3] $CXX fpaq0mw.cpp coder1.s -o $EXE"
$CXX $CXXFLAGS fpaq0mw.cpp coder1.s $LDFLAGS -o "$EXE"

# timing helper used by test.sh (optional)
$CXX -O2 timetest.cpp $LDFLAGS -o timetest 2>/dev/null || echo "note: timetest not built"
