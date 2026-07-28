#!/bin/sh
# mk.sh          -- Debug build: every parameter stays in the binary as a
#                   pdesc/pmask pattern string, so IDX/opt.pl can flip its bits.
#                   Use this for tuning.
# mk.sh release  -- Const build: idx2inc evaluates the patterns at preprocess
#                   time (no mapping objects, no indirection). Use for shipping.
#
# Both builds produce the SAME stream for the same parameter set; release is
# only faster.  Regenerating MOD/ needs perl; the tree ships with MOD/ already
# generated, so a plain `g++ -std=gnu++20 -O2 -I. -ILib3 xadpcm.cpp -o xadpcm`
# also works.
#
# -I./Lib3: the codec's RC derives from Coroutine, so common.inc / coro3b.inc /
#   coro3_pin*.inc / coro3_setjmp_*.h must be reachable.
set -e
CXX=${CXX:-g++}
ARCH=${ARCH:-}
OPT="-std=gnu++20 -O2 -fstrict-aliasing -fomit-frame-pointer -fno-exceptions -fno-rtti $ARCH -I. -I./Lib3"

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
    ( cd IDX && perl idx2inc.pl "$b.idx" 1 >/dev/null )
    mv -f "IDX/${b}_h.inc" "IDX/${b}_p.inc" MOD/
  fi
done

$CXX $OPT xadpcm.cpp -o xadpcm -lm
if [ "$1" = "release" ]; then echo "built xadpcm (Const/release)"; else echo "built xadpcm (Debug/optimizable)"; fi
