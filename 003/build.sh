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
# -m32 is not negotiable.  The recovered structs are laid out for a four-byte
# pointer and their static_asserts say so, the moved entry points carry i386
# calling-convention attributes, and the CPUID helpers are i386 inline asm.  On
# a 64-bit host that means the multilib runtime:
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

# The dialect %gcc% pinned in g.bat.

opts=(-m32 "$ARCH" -msse2 -mfpmath=sse -std=c++17
      -fno-strict-aliasing -fpermissive -fno-rtti -fno-exceptions)

# -fpermissive is no longer load-bearing: the file compiles clean without it
# (BMF_STRICT=1 below, which is how that is checked).  It stays because g.bat
# had it, and because keeping it means a future extraction that reintroduces
# the old conversions still builds while BMF_STRICT says what it cost.

# BMF_STRICT=1 drops -fpermissive and counts what breaks.  That is Phase C's
# scoreboard (REFACTORING2.md §4.2): every one of them is a place where the
# decompilation put an integer where a pointer belongs, or the reverse, or
# compared two things that are not the same kind.  The target is zero.
#
#   BMF_STRICT=1 ./build.sh
if [ "${BMF_STRICT:-0}" = 1 ]; then
  # Drop the flag, not just its text: "${a[@]/x/}" leaves an empty element,
  # which g++ then reports as a missing linker input and which counted as an
  # error of its own.
  strict=(); for o in "${opts[@]}"; do [ "$o" = -fpermissive ] || strict+=("$o"); done
  opts=("${strict[@]}")
  set -x
  "$CXX" "${opts[@]}" -O2 "${incs[@]}" -fsyntax-only -fdiagnostics-plain-output \
      bmf.cpp "$@" 2>&1 | tee strict.log | grep -c 'error:' || true
  set +x
  echo "^ conversions still needing -fpermissive (see strict.log)"
  exit 0
fi

# BMF_WARN=1 counts the four conversion families REFACTORING5.md §2.5 uses as a
# scoreboard: `-Wconversion`, `-Wsign-conversion`, `-Wsign-compare` and
# `-Wuseless-cast`.  Unlike BMF_STRICT this is a *ratchet* and not a target --
# a decompilation of 1997 x86 reinterprets memory on purpose, so some of these
# warnings are the program.  `test.sh` fails when the count rises above what
# `warn.txt` records; lowering that file is how progress is recorded.
#
#   BMF_WARN=1 ./build.sh          # print the count, write warn.log
if [ "${BMF_WARN:-0}" = 1 ]; then
  warn=(-Wconversion -Wsign-conversion -Wsign-compare -Wuseless-cast)
  "$CXX" "${opts[@]}" -O2 "${incs[@]}" "${warn[@]}" -fsyntax-only \
      -fdiagnostics-plain-output bmf.cpp "$@" 2>&1 | tee warn.log |
      grep -cE '\[-W(conversion|sign-conversion|sign-compare|useless-cast)\]' || true
  exit 0
fi

# g.bat set these in %opts% but its live command line never used them; kept
# here for the same reason, so the two files still read the same.
# unused=(-fomit-frame-pointer -fno-stack-protector -fno-stack-check
#         -fno-check-new -flto -ffat-lto-objects)

# Give every function its own section and let the linker drop the ones nothing
# reaches.  The mode is a set of constants now (subs1.hpp, "The compression
# mode"), so the branches that tested them fold and whole functions fall out
# behind them; this is what stops those shipping in the binary whether or not
# they have been deleted from the source yet.
#
# BMF_GC=0 turns it off, which is how to see what it was removing:
#   BMF_GC=list ./build.sh          # print each discarded function
opts+=(-ffunction-sections -fdata-sections)
gc=(-Wl,--gc-sections)
[ "${BMF_GC:-1}" = 0 ] && gc=()
[ "${BMF_GC:-1}" = list ] && gc+=(-Wl,--print-gc-sections)

# -static, as in g.bat.  Set BMF_STATIC=0 for a dynamically linked build.
link=("${gc[@]}")
[ "${BMF_STATIC:-1}" = 0 ] || link+=(-static)

if ! "$CXX" -m32 -E -x c++ /dev/null >/dev/null 2>&1; then
  echo "$CXX cannot target -m32 — install the 32-bit toolchain (see the notes at the top of $0)" >&2
  exit 1
fi

rm -f "$OUT" ./*.o

set -x
"$CXX" "${opts[@]}" -s -O2 "${incs[@]}" "${link[@]}" bmf.cpp -o "$OUT" "$@"
