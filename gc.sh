#!/bin/sh
# gc.sh -- Linux build script for coder0 (counterpart of gc.bat).
#
#   ./gc.sh            shipping build:  every IDX knob folded to a literal -> ./coder0
#                      (idx2inc.pl writes USE_NEW=0 into MOD/)
#   ./gc.sh tune       tuning build:    "Const 0" knobs stay live "!MAP!" objects
#                      that IDX/opt.pl can patch in the binary        -> ./coder0t
#                      (USE_NEW=1: the SSE stage dispatches on its NB knob)
#
# Both builds must produce byte-identical output (IDX-FORMAT.md sec.1);
# t1.sh checks that when both executables are present.
#
# Overridable from the environment:
#   CXX    compiler            (default: g++)
#   TARG   -march/-mtune target (default: haswell, as in gc.bat)
#   CXXEXTRA  extra flags appended to the compile line (e.g. -DS0_CACHE_WR=1)
#   OUT    output executable name (default coder0 / coder0t)

set -e
cd "$(dirname "$0")"

MODE=${1:-release}
CXX=${CXX:-g++}
TARG=${TARG:-haswell}

case "$MODE" in
  release|"") NOCONST=1; USENEW=0; EXE=${OUT:-coder0} ;;
  tune)       NOCONST=0; USENEW=1; EXE=${OUT:-coder0t} ;;
  *) echo "usage: $0 [release|tune]" >&2; exit 1 ;;
esac

# 1. Regenerate MOD/*_h.inc + MOD/*_p.inc from IDX/*.idx (same loop as gc.bat).
#    IDX_NOCONST=1 folds every knob (shipping); 0 honours the Const flags in the
#    .idx, so "Const 0" lines become patchable mapping objects (tuning).  The
#    second idx2inc.pl argument is USE_NEW, written into the generated headers
#    (IDX-FORMAT.md sec.7): 1 for the tuning build, 0 for shipping.
genmod() {
  mkdir -p MOD
  for f in IDX/*.idx; do
    b=$(basename "$f" .idx)
    IDX_NOCONST=$1 perl IDX/idx2inc.pl "$f" $2 >/dev/null
    mv -f "IDX/${b}_h.inc" "IDX/${b}_p.inc" MOD/
  done
}
genmod $NOCONST $USENEW

# 2. Compile.  Mirrors the clang line in gc.bat as far as gcc/linux allow:
#    -O3 -ffast-math == -Ofast, LTO, static link, stripped.
#    -ffp-contract=off -fno-unsafe-math-optimizations: the shipping and the
#    tuning build compile the same float expressions with different
#    constness (folded vs. runtime knobs), and under full -ffast-math gcc
#    then contracts a*b+c into FMAs, reassociates sums, replaces divisions
#    by reciprocals or refactors common terms differently in the two, which
#    changed the stream by a byte or two between them.  With contraction and
#    the unsafe algebra off both builds are byte-identical (the rest of
#    -ffast-math -- no errno, finite math -- stays); it costs nothing
#    measurable.
INCS="-std=c++23 -DNDEBUG -D_FILE_OFFSET_BITS=64"
#    -fno-builtin-logf -fno-builtin-expf: logf/expf are never recognized as
#    builtins, so the shipping build cannot constant-fold one of a folded knob
#    (MPFR) where the tuning build calls libm -- the F5 drift of
#    SSE-DESIGN.md sec.4.1; every transcendental is a libm call in both.
OPTS="-O3 -ffast-math -ffp-contract=off -fno-unsafe-math-optimizations -fno-builtin-logf -fno-builtin-expf -fomit-frame-pointer -fno-stack-protector -fstrict-aliasing -flto"
ARCH="-march=$TARG -mtune=$TARG -m64"
WARN="-Wno-format -Wno-unused-result -Wno-unused-variable -Wno-unused-but-set-variable"

rm -f "$EXE" ./*.o
echo "$CXX $INCS $OPTS $ARCH $WARN $CXXEXTRA coder0.cpp -static -s -o $EXE"
$CXX $INCS $OPTS $ARCH $WARN $CXXEXTRA coder0.cpp -static -s -o "$EXE"
rm -f ./*.o
ls -l "$EXE"

# 3. MOD/ is a build input that ships in git (IDX-FORMAT.md sec.12); keep it in
#    the shipping (folded) state whatever build ran last.
[ "$NOCONST" = 1 ] || genmod 1 0
