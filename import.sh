#!/bin/sh
# import.sh -- linux port of IDX/import-all.bat.
#
# Folds an optimizer export (default ./export.!!!, a list of mdesc() lines --
# one per !MAP! descriptor the climber touched) back into the .idx sources, so
# the next mk.sh build starts from the tuned patterns.
#
#   ./import.sh [export-file] [idx-dir]
#
# The .bat also rar'd a dated backup of the .idx set first; here that job
# belongs to git, so it is not repeated.  import.pl rewrites one .idx at a time
# and is a pure filter, so each file goes through a temporary and is moved into
# place only if perl exited clean -- a half-written .idx would take the whole
# parameter set with it.
set -e
EXP=${1:-export.!!!}
IDXDIR=${2:-IDX}
[ -f "$EXP" ] || { echo "import.sh: $EXP not found" >&2; exit 1; }
for a in "$IDXDIR"/*.idx; do
  perl "$IDXDIR/import.pl" "$a" "$EXP" > "$a.tmp" 2>/dev/null
  mv -f "$a.tmp" "$a"
  echo "imported -> $a"
done
