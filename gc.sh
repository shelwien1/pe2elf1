#!/bin/sh
# Linux build script -- port of gc.bat.
#
# gc.bat targets clang-cl-ish MSVC headers on win64; on linux all the
# -IC:\VC2019\... / -D_CRT_* / -ladvapi32 noise goes away and what is left is
# the part that actually shapes the code: C++23, -Ofast, -march=haswell, LTO,
# fast-math, no frame pointer / stack protector, static link.
#
# Usage:  ./gc.sh [target ...]     targets: coder0 bmpc (default: all)
#         CXX=g++ ./gc.sh          pick a compiler (default: clang++, else g++)
#         GCOPTS='-DFOO=1' ./gc.sh extra flags appended to the command line
#         MARCH=native ./gc.sh     override -march/-mtune (default: haswell)

set -e

cd "$(dirname "$0")"

# --- IDX regeneration (the ": goto skip" block at the top of gc.bat) ---------
# gc.bat has it disabled by default; keep the same default here, enable with
# GCIDX=1 when the .idx sources are edited.  MOD/ ships generated.
if [ "${GCIDX:-0}" = "1" ]; then
  for a in IDX/*.idx; do
    [ -e "$a" ] || continue
    b=$(basename "$a" .idx)
    ( cd IDX && perl idx2inc.pl "$b.idx" >/dev/null )
    mv -f IDX/"$b"_*.inc MOD/ 2>/dev/null || true
  done
fi

# --- compiler ---------------------------------------------------------------
if [ -z "$CXX" ]; then
  if command -v clang++ >/dev/null 2>&1; then CXX=clang++; else CXX=g++; fi
fi

targ=${MARCH:-haswell}
arch="-march=$targ -mtune=$targ -m64"

# gc.bat: -std=c++23 -Ofast -Wno-format -s -static
# -ffast-math is implied by -Ofast; kept explicit to mirror gc.bat's %opts%.
opts="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fstrict-aliasing -ffast-math"
opts="$opts -Wno-format -Wno-unused-value -Wno-unused-variable"

# clang-only switches from %opts% that gcc does not know.
case "$($CXX --version 2>&1 | head -1)" in
  *clang*)
    opts="$opts -fgnu-keywords -Wno-nonportable-include-path -Wno-expansion-to-defined"
    opts="$opts -Wno-ignored-attributes -fno-stack-clash-protection -flto -fuse-ld=lld"
    ;;
  *)
    opts="$opts -fgnu-keywords -Wno-expansion-to-defined -Wno-ignored-attributes -flto"
    ;;
esac

# -static matches gc.bat; drop it if the toolchain has no static libstdc++.
link="-static"
if ! echo 'int main(){return 0;}' | $CXX -x c++ -static -o /dev/null - 2>/dev/null; then
  link=""
fi

incs="-DNDEBUG -D__DIRNAM__=$(basename "$PWD")"

build() {   # build <source> <output>
  src=$1; out=$2
  [ -f "$src" ] || { echo "gc.sh: no $src, skipping $out"; return 0; }
  echo "$CXX -> $out"
  # shellcheck disable=SC2086
  $CXX -std=c++23 -Ofast $arch $incs $opts $link $GCOPTS "$src" -o "$out"
  strip "$out" 2>/dev/null || true
}

[ $# -eq 0 ] && set -- coder0 bmpc

rm -f *.o
for t in "$@"; do
  case "$t" in
    coder0) build coder0.cpp coder0 ;;
    bmpc)   build bmpc.cpp   bmpc   ;;
    *)      build "$t.cpp"   "$t"   ;;
  esac
done
rm -f *.o
