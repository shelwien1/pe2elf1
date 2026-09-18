#!/bin/sh
# tm.sh -- measure only: compress every file with the given exe, files in
# parallel, print the sizes and the total.  Used for hand experiments on a
# tuning build patched with IDX/setp.pl.
#
#   ./tm.sh [exe] [files...]        (default ./coder0, book1 wcc386)

cd "$(dirname "$0")"
EXE=${1:-./coder0}
case "$EXE" in */*) ;; *) EXE="./$EXE" ;; esac
[ $# -gt 0 ] && shift
[ $# -gt 0 ] || set -- book1 wcc386
tmp=${TMPDIR:-/tmp}/tm.$$
for f in "$@"; do "$EXE" c "$f" "$tmp.$(basename "$f")" >/dev/null 2>&1 & done
wait
total=0; line=""
for f in "$@"; do
  s=$(wc -c < "$tmp.$(basename "$f")" 2>/dev/null || echo 0); rm -f "$tmp.$(basename "$f")"
  total=$((total + s)); line="$line$(printf '%s %d  ' "$f" "$s")"
done
echo "$line total $total"
