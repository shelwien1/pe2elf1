#!/bin/sh
# mk.sh            -- tuning build.  Debug 1, Const 0, USE_NEW 1: every IDX
#                     parameter stays in the binary as a pdesc/pmask pattern
#                     string that IDX/opt.pl can flip the bits of.
# mk.sh release    -- shipping build.  Const 1, USE_NEW 0: idx2inc.pl folds the
#                     patterns at generate time, so there are no mapping
#                     objects and no indirection left.
#
# Both must produce identical archives -- that is the contract, and ./verify.sh
# is the test:
#
#   ./mk.sh         && ./verify.sh ./paq8hpc_idx tuning
#   ./mk.sh release && ./verify.sh ./paq8hpc_idx release
#
# MOD/ is committed, generated in release mode, so the tree builds without perl.
# It is a build INPUT, not an artefact: a stale MOD/ compiles fine and codes
# differently, which is why this script regenerates it whenever perl is present.
set -e
CXX=${CXX:-g++}
OPT=${OPT:--O3 -march=native}
OUT=${OUT:-./paq8hpc_idx}

MODE=
if [ "${1:-}" = "release" ]; then MODE=release; shift; fi   # rest of $@ -> the compiler

mkdir -p MOD
if command -v perl >/dev/null 2>&1; then
  for f in IDX/*.idx; do
    b=`basename "$f" .idx`
    case "$b" in *-const) continue;; esac
    if [ "$MODE" = release ]; then
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
else
  echo "mk.sh: no perl, building against the committed MOD/ as-is" >&2
fi

$CXX $OPT "$@" -o "$OUT" paq8hpc.cpp
if [ "$MODE" = release ]; then echo "built $OUT (Const/release)"; else echo "built $OUT (Debug/optimizable)"; fi
