#!/bin/sh
# Builds the original and the compact coder, then for every file given:
#   * encodes with both and requires byte-identical compressed output,
#   * decodes the compact output and requires the original input back.
# usage: ./test.sh [order [MMAX]] -- file...
set -e
cd "$(dirname "$0")"
ORDER=12; MMAX=4000
if [ "$1" != "--" ] && [ -n "$1" ]; then ORDER=$1; shift; fi
if [ "$1" != "--" ] && [ -n "$1" ]; then MMAX=$1; shift; fi
[ "$1" = "--" ] && shift
OUT=${TMPDIR:-/tmp}/ppmd_test.$$
mkdir -p "$OUT"
g++ -O2 -o "$OUT/ppmd_orig" ppmd_orig.cpp
g++ -O2 -o "$OUT/ppmd" ppmd.cpp
for f in "$@"; do
  b=$(basename "$f")
  "$OUT/ppmd_orig" c "$f" "$OUT/$b.orig" $ORDER $MMAX 2>/dev/null >/dev/null
  "$OUT/ppmd" c "$f" "$OUT/$b.new" $ORDER $MMAX 2>/dev/null >/dev/null
  cmp "$OUT/$b.orig" "$OUT/$b.new"
  "$OUT/ppmd" d "$OUT/$b.new" "$OUT/$b.dec" $ORDER $MMAX 2>/dev/null >/dev/null
  cmp "$f" "$OUT/$b.dec"
  echo "$b: identical output ($(wc -c < "$OUT/$b.new") bytes), roundtrip ok"
done
rm -rf "$OUT"
