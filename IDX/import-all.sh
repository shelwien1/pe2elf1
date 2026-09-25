#!/bin/sh
# import-all.sh -- Linux counterpart of import-all.bat: fold an opt.pl export
# (mdesc lines) back into every .idx in this directory.
#   IDX/import-all.sh [export-file]      (default: ../export.!!!)
cd "$(dirname "$0")"
f=${1:-../export.!!!}
[ -f "$f" ] || { echo "no export file $f" >&2; exit 1; }
for a in *.idx; do
  perl import.pl "$a" "$f" > "$a.tmp" 2>/dev/null && mv -f "$a.tmp" "$a"
done
