#!/bin/sh
# gc.sh -- build tsvcomp (same scheme as ../gc.sh for coder0).
#
#   ./gc.sh        shipping build: every IDX knob folded   -> ./tsvcomp
#   ./gc.sh tune   tuning build: "Const 0" knobs patchable  -> ./tsvcompt
#                  (tune with: perl ../IDX/opt.pl opt.lst ./tsvcompt)
#
# CXX, TARG, CXXEXTRA, OUT as in ../gc.sh.
set -e
cd "$(dirname "$0")"

MODE=${1:-release}
CXX=${CXX:-g++}
TARG=${TARG:-haswell}

case "$MODE" in
  release|"") NOCONST=1; USENEW=0; EXE=${OUT:-tsvcomp} ;;
  tune)       NOCONST=0; USENEW=1; EXE=${OUT:-tsvcompt} ;;
  *) echo "usage: $0 [release|tune]" >&2; exit 1 ;;
esac

# MOD/ from IDX/*.idx, with coder0's generator
genmod() {
  mkdir -p MOD
  for f in IDX/*.idx; do
    b=$(basename "$f" .idx)
    IDX_NOCONST=$1 perl ../IDX/idx2inc.pl "$f" $2 >/dev/null
    mv -f "IDX/${b}_h.inc" "IDX/${b}_p.inc" MOD/
  done
}
genmod $NOCONST $USENEW

# the flags of ../gc.sh: both builds must give byte-identical streams
INCS="-std=c++23 -DNDEBUG -D_FILE_OFFSET_BITS=64"
OPTS="-O3 -ffast-math -ffp-contract=off -fno-unsafe-math-optimizations -fno-builtin-logf -fno-builtin-expf -fomit-frame-pointer -fno-stack-protector -fstrict-aliasing -flto"
ARCH="-march=$TARG -mtune=$TARG -m64"
WARN="-Wno-format -Wno-unused-result -Wno-unused-variable -Wno-unused-but-set-variable"

rm -f "$EXE"
echo "$CXX $INCS $OPTS $ARCH $WARN $CXXEXTRA tsvcomp.cpp -static -s -o $EXE"
$CXX $INCS $OPTS $ARCH $WARN $CXXEXTRA tsvcomp.cpp -static -s -o "$EXE"
ls -l "$EXE"

# MOD/ ships in git in the shipping (folded) state
[ "$NOCONST" = 1 ] || genmod 1 0
