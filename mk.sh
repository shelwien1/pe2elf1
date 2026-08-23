#!/bin/sh
# mk.sh -- regenerate MOD/ from IDX/ and build bmf.
#
#   ./mk.sh           tuning build:   Debug 1, Const 0.  Every knob is a live
#                                     mapping object with a "!MAP!" descriptor,
#                                     so IDX/opt.pl can find and hill-climb it.
#   ./mk.sh release   shipping build: Const 1.  Every knob folds to a literal.
#
# The shipping build is derived from the same sources with one substitution, so
# the two cannot drift (IDX-FORMAT.md sec.1).  Both must produce the same coded
# stream -- that is the contract, and ./t.sh plus the corpus manifest test it.
#
# MOD/ is checked in, so the tree builds without perl; this script refreshes it
# when perl is present.  A stale MOD/ compiles fine and codes differently.

set -e

mode=${1:-tuning}

if command -v perl >/dev/null 2>&1; then
  mkdir -p MOD
  for f in IDX/*.idx; do
    b=$(basename "$f" .idx)
    case "$b" in *-const) continue;; esac
    if [ "$mode" = release ]; then
      sed 's/^Const 0/Const 1/' "$f" > "IDX/$b-const.idx"
      # idx2inc.pl finds the template by swapping .idx for .inc, so the copy
      # needs one too; a module with no Index has no template and needs none.
      [ -f "IDX/$b.inc" ] && cp "IDX/$b.inc" "IDX/$b-const.inc"
      ( cd IDX && perl idx2inc.pl "$b-const.idx" 0 >/dev/null )
      mv "IDX/$b-const_h.inc" "MOD/${b}_h.inc"
      [ -f "IDX/$b-const_p.inc" ] && mv "IDX/$b-const_p.inc" "MOD/${b}_p.inc"
      rm -f "IDX/$b-const.idx" "IDX/$b-const.inc"
    else
      ( cd IDX && perl idx2inc.pl "$b.idx" 1 >/dev/null )
      mv "IDX/${b}_h.inc" "MOD/${b}_h.inc"
      [ -f "IDX/${b}_p.inc" ] && mv "IDX/${b}_p.inc" "MOD/${b}_p.inc"
    fi
  done
else
  echo "mk.sh: no perl, building against the checked-in MOD/" >&2
fi

exec ./gc.sh
