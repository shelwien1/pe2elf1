#!/bin/sh
# Linux/POSIX counterpart of import-all.bat: fold an optimizer export back
# into every .idx source in this directory.
#
#   ./import-all.sh [export-file]      (default: ../export.!!!)
#
# The .bat makes a dated rar backup of the .idx files first; tar stands in for
# rar here.  Run from the IDX directory.

set -e

f=${1:-../export.\!\!\!}

[ -f "$f" ] || { echo "no export file: $f" >&2; exit 1; }

tar czf "idx_$(date +%Y%m%d_%H%M%S).tar.gz" -- *.idx

for a in *.idx; do
  perl import.pl "$a" "$f" > "$a.tmp"
  mv -f "$a.tmp" "$a"
done
