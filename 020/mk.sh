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
#
# Knobs, both measured -- see STATUS-speed.md:
#   CXX=clang++   ~6-9% faster than g++ on this codec, bit-identical output.
#   ARCH=-march=native (or -march=haswell, which g.bat has always used)
#                 ~5%: it lets the AVX2 NLMS kernels inline instead of being
#                 reached through the CPUID-dispatched function pointer.
#   XAD_STATS=1   restores the -v per-stream byte accounting, which costs a
#                 table lookup and an add on every coded bit.
# Left OFF by default: PGO.  Measured a small REGRESSION on top of -O3, not the
# 10-20% the plan expected -- the branchy scalar code it would have helped is
# what the SIMD work replaced.
# Cross-building for Windows, which is worth doing because two of this tree's
# bugs were Windows-only and neither could be reached from linux:
#   apt install g++-mingw-w64-x86-64 wine64
#   x86_64-w64-mingw32-g++ -std=gnu++20 -O2 -fno-exceptions -fno-rtti -static \
#       -I. -I./Lib3 xadpcm.cpp -o xadpcm.exe
#   WINEDEBUG=warn+heap wine ./xadpcm.exe c wavs2 out
# The heap warnings matter: wine TOLERATES a free() on an _aligned_malloc block
# where real Windows crashes, but it prints "invalid block type" for it, so that
# grep is the reproducer for a class of bug the linux build cannot show.
set -e
CXX=${CXX:-g++}
ARCH=${ARCH:-}
OPT="-std=gnu++20 -O3 -fstrict-aliasing -fomit-frame-pointer -fno-exceptions -fno-rtti $ARCH -I. -I./Lib3"

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

# No -lm: the squash/stretch/code-length tables were the only libm callers and
# they are integer arithmetic now, so the coding path has no floating point at
# all.  That also makes -ffast-math/-Ofast harmless rather than dangerous --
# still not adopted, because they were measured at 0.5% (gcc) / -1% (clang),
# inside the noise band.  See xad_logistic.inc and STATUS-speed.md.
$CXX $OPT xadpcm.cpp -o xadpcm
if [ "$1" = "release" ]; then echo "built xadpcm (Const/release)"; else echo "built xadpcm (Debug/optimizable)"; fi
