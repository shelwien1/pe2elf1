#!/bin/sh
# mk.sh            -- tuning build:   Debug 1, Const 0.  Every knob is a live
#                     mapping object and opt.pl can hill-climb it.
# mk.sh release    -- shipping build: Const 1.  Every knob is a literal the
#                     compiler folds; no mapping objects, no indirection.
#
# Both builds must produce byte-identical streams -- that is the contract, and
# ./mk.sh check is the test.  See IDX-FORMAT.md sections 1 and 12.
set -e

CXX=${CXX:-g++}
OPT=${OPT:--O2}
STD=${STD:--std=gnu++2a}
FLAGS="-DNDEBUG -fpermissive -fno-exceptions -fno-rtti -fomit-frame-pointer"

gen() {   # gen <idx-basename> <use_new>  -- $1 is copied because a shell
  gb=$1   # function has no local scope and the caller's $b must survive
  if [ -x "$(command -v perl)" ]; then
    ( cd IDX && perl idx2inc.pl "$gb.idx" "$2" >/dev/null )
    mv "IDX/${gb}_h.inc" "IDX/${gb}_p.inc" MOD/
  fi
}

case "$1" in
release)
  # Derive the shipping source from the tuning source with one substitution, so
  # the two can never drift.
  for f in IDX/bmg-*.idx; do
    case "$f" in *-const.idx) continue;; esac
    b=$(basename "$f" .idx)
    sed 's/^Const 0/Const 1/' "$f" > "IDX/$b-const.idx"
    cp "IDX/$b.inc" "IDX/$b-const.inc" 2>/dev/null || : > "IDX/$b-const.inc"
    gen "$b-const" 0
    mv "MOD/$b-const_h.inc" "MOD/${b}_h.inc"
    mv "MOD/$b-const_p.inc" "MOD/${b}_p.inc"
    rm -f "IDX/$b-const.idx" "IDX/$b-const.inc"
  done
  $CXX $OPT $STD $FLAGS -s bmg.cpp -o bmg
  echo "shipping build: MOD/ holds literals"
  ;;
check)
  ./mk.sh          >/dev/null; cp bmg /tmp/bmg_tune$$
  ./mk.sh release  >/dev/null; cp bmg /tmp/bmg_ship$$
  rc=0
  for f in ${2:-testfiles/*.bmp}; do
    /tmp/bmg_tune$$ c "$f" /tmp/a$$.bmg >/dev/null 2>&1
    /tmp/bmg_ship$$ c "$f" /tmp/b$$.bmg >/dev/null 2>&1
    if cmp -s /tmp/a$$.bmg /tmp/b$$.bmg
      then echo "same   $f"
      else echo "DIFFER $f"; rc=1; fi
  done
  rm -f /tmp/bmg_tune$$ /tmp/bmg_ship$$ /tmp/a$$.bmg /tmp/b$$.bmg
  exit $rc
  ;;
*)
  for f in IDX/bmg-*.idx; do
    case "$f" in *-const.idx) continue;; esac
    gen "$(basename "$f" .idx)" 1
  done
  $CXX $OPT $STD $FLAGS bmg.cpp -o bmg
  echo "tuning build: $(grep -c 'pdesc(' MOD/bmg-P0_h.inc 2>/dev/null || echo '?') live knobs, \
$(grep -oa "!MAP!" bmg 2>/dev/null | wc -l) markers in the binary"
  ;;
esac
