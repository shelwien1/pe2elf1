#!/usr/bin/env bash
# build.sh — bmf.cpp -> a native ELF binary, the POSIX port of g.bat.
#
#   ./build.sh              builds ./bmf
#   BMF_OUT=bmf.dbg ./build.sh -O0 -g
#
# g.bat drove MinGW on Windows and ended on one line:
#
#   %gcc% -s -O2 %incs% -static bmf.cpp -o bmf.exe
#
# with %gcc% carrying -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 and four
# -f switches.  All of that is reproduced below; the only difference is the
# output name, since what comes out here is an ELF executable and not a PE.
#
# -m32 is not negotiable.  Every global the decompiled bodies touch is a
# 32-bit absolute address into blob.inc (see bmf.cpp's BMF_BLOB), the moved
# entry points carry i386 calling-convention attributes, and the CPUID helpers
# are i386 inline asm.  On a 64-bit host that means the multilib runtime:
#
#   Debian/Ubuntu   apt install g++-multilib
#   Fedora          dnf install glibc-devel.i686 libstdc++-static.i686
#
# Extra arguments are appended to the compile, so they win over what is here.
set -eu
cd "$(dirname "$0")"

CXX=${CXX:-g++}
OUT=${BMF_OUT:-bmf}

# -march=k8 is g.bat's: SSE2 without anything later, which is the floor the
# decompiled bodies' intrinsics need.  Override for a different baseline.
ARCH=${BMF_ARCH:--march=k8}

incs=(-DNDEBUG -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0)

# The dialect %gcc% pinned in g.bat.  -fpermissive is load-bearing: Hex-Rays
# hands ints to pointer parameters throughout, and C++ makes those errors.
opts=(-m32 "$ARCH" -msse2 -mfpmath=sse -std=c++17
      -fno-strict-aliasing -fpermissive -fno-rtti -fno-exceptions)

# g.bat set these in %opts% but its live command line never used them; kept
# here for the same reason, so the two files still read the same.
# unused=(-fomit-frame-pointer -fno-stack-protector -fno-stack-check
#         -fno-check-new -flto -ffat-lto-objects)

# -static, as in g.bat.  Set BMF_STATIC=0 for a dynamically linked build.
link=()
[ "${BMF_STATIC:-1}" = 0 ] || link+=(-static)

if ! "$CXX" -m32 -E -x c++ /dev/null >/dev/null 2>&1; then
  echo "$CXX cannot target -m32 — install the 32-bit toolchain (see the notes at the top of $0)" >&2
  exit 1
fi

rm -f "$OUT" ./*.o

set -x
"$CXX" "${opts[@]}" -s -O2 "${incs[@]}" "${link[@]}" bmf.cpp -o "$OUT" "$@"
