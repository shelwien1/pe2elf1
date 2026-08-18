#!/bin/sh
# Round-trip and size report over a corpus.  Usage: ./t.sh [files...]
# With no arguments it uses testfiles/*.bmp.
set -e
EXE=${BMG:-./bmg}
TMP=${TMPDIR:-/tmp}/bmg_t.$$
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

files="$*"
[ -z "$files" ] && files=$(ls testfiles/*.bmp 2>/dev/null)
[ -z "$files" ] && { echo "no test files; put some in testfiles/ or name them"; exit 1; }

tin=0; tout=0; fail=0
printf "%-16s %10s %10s %8s  %s\n" file input output ratio "round-trip"
for f in $files; do
  b=$(basename "$f")
  "$EXE" c "$f" "$TMP/$b.bmg" >/dev/null 2>&1
  "$EXE" d "$TMP/$b.bmg" "$TMP/$b.out" >/dev/null 2>&1
  i=$(wc -c < "$f"); o=$(wc -c < "$TMP/$b.bmg")
  if cmp -s "$f" "$TMP/$b.out"; then rt="OK"; else rt="*** MISMATCH ***"; fail=1; fi
  printf "%-16s %10s %10s %7.3f  %s\n" "$b" "$i" "$o" \
         "$(echo "$o $i" | awk '{print $1/$2}')" "$rt"
  tin=$((tin+i)); tout=$((tout+o))
done
printf "%-16s %10s %10s %7.3f\n" TOTAL "$tin" "$tout" \
       "$(echo "$tout $tin" | awk '{print $1/$2}')"
exit $fail
