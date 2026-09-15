#!/bin/sh
#
# build.sh [program...] - build with GCC or Clang (Linux; also MSYS2/MinGW bash).
# Programs default to "fpaq0mw tangelo_w"; each is built from <program>.cpp.
#
#   ./build.sh                                    # both
#   ./build.sh tangelo_w                          # just one
#   CXX=clang++ ./build.sh                        # pick the compiler
#   CXXFLAGS="-O3 -ffast-math" ./build.sh         # replace the default flags
#   CXX=x86_64-w64-mingw32-g++ EXT=.exe LDFLAGS="-static -s" ./build.sh   # cross to Windows
#
# The three steps per program mirror the original g.bat:
#   1. compile the model step (encode_sim) to Intel-syntax assembly,
#   2. let track.pl insert a journaling call in front of every store,
#   3. build the executable from the main source plus the instrumented assembly.
set -eu
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
CXXFLAGS="${CXXFLAGS:--O3 -ffast-math -fomit-frame-pointer -fno-rtti -fno-exceptions -fno-stack-protector -march=native}"
# Step 1 only. track.pl puts a "push" in front of every store, which writes just
# below rsp, so the 128-byte SysV red zone must not be in use there. It costs the
# instrumented step a real stack frame and changes nothing else.
SIMFLAGS="${SIMFLAGS:--mno-red-zone}"
LDFLAGS="${LDFLAGS:-}"
EXT="${EXT:-}"

progs="${*:-fpaq0mw tangelo_w}"

for p in $progs; do
  exe="$p$EXT"
  rm -f "coder-$p.s" "coder1-$p.s" "$exe"

  echo "[$p 1/3] $CXX -S -masm=intel -DSIM_FUNC $p.cpp -o coder-$p.s"
  $CXX $CXXFLAGS $SIMFLAGS -S -masm=intel -DSIM_FUNC "$p.cpp" -o "coder-$p.s"

  echo "[$p 2/3] perl track.pl coder-$p.s coder1-$p.s"
  perl track.pl "coder-$p.s" "coder1-$p.s"

  echo "[$p 3/3] $CXX $p.cpp coder1-$p.s -o $exe"
  $CXX $CXXFLAGS "$p.cpp" "coder1-$p.s" $LDFLAGS -o "$exe"
done

# timing helper used by test.sh (optional)
$CXX -O2 timetest.cpp $LDFLAGS -o "timetest$EXT" 2>/dev/null || echo "note: timetest not built"
