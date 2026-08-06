#!/bin/sh
# mk.sh          -- Debug build: every parameter stays in the binary as a
#                   pdesc/pmask pattern string, so IDX/opt.pl can flip its bits.
#                   Use this for tuning.
# mk.sh release  -- Const build: idx2inc evaluates the patterns at preprocess
#                   time (no mapping objects, no indirection). Use for shipping.
set -e
CXX=${CXX:-g++}
# -I./Lib3: the core's RC derives from Coroutine, so common.inc/coro3b.inc/
#   coro_fp.inc must be reachable -- dxt5comp.cpp includes them before the core.
# -O3 rather than -O2: measured -8.2% retired instructions on the mixed corpus,
#   almost all of it from loop unrolling, and byte-identical output.  g.bat has
#   used -Ofast -march=haswell for a while; this closes most of that gap without
#   -ffast-math (see the SQT/STT note in dxt5comp_core.inc).
# ARCH: empty by default so the binary stays portable.  ARCH=-march=native, or
#   -march=haswell to match g.bat, is worth another ~3.6% via AVX2 in the mixer.
ARCH=${ARCH:-}
OPT="-std=gnu++2a -O3 -fstrict-aliasing -fomit-frame-pointer $ARCH -I./Lib3 -I."

mkdir -p MOD
for f in IDX/*.idx; do
  b=`basename "$f" .idx`
  case "$b" in *-const) continue;; esac
  if [ "$1" = "release" ]; then
    sed 's/^Const 0/Const 1/' "$f" > "IDX/$b-const.idx"
    cp "IDX/$b.inc" "IDX/$b-const.inc"
    ( cd IDX && perl idx2inc.pl "$b-const.idx" 0 >/dev/null )
    mv -f "IDX/$b-const_h.inc" "MOD/${b}_h.inc"
    mv -f "IDX/$b-const_p.inc" "MOD/${b}_p.inc"
    rm -f "IDX/$b-const.idx" "IDX/$b-const.inc"
  else
    ( cd IDX && perl idx2inc.pl "$b.idx" 1 >/dev/null )   # UseNew=1: Debug sizes are load-time
    mv -f "IDX/${b}_h.inc" "IDX/${b}_p.inc" MOD/
  fi
done

$CXX $OPT dxt5comp.cpp -o dxt5comp
$CXX $OPT ddsdet.cpp   -o ddsdet
if [ "$1" = "release" ]; then echo "built dxt5comp + ddsdet (Const/release)"; else echo "built dxt5comp + ddsdet (Debug/optimizable)"; fi
