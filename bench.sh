#!/bin/sh
# bench.sh <bmf-binary> <tag> [crop-dir]
# Compress every image in the crop corpus and print "<name> <bytes>".
# Build the corpus first:  for f in LPCB-bmp/*.bmp; do python3 crop.py "$f" crops/$(basename $f) 1024 1024; done
B=$1; T=${2:-run}; D=${3:-crops}
for f in "$D"/*.bmp; do
  n=$(basename "$f" .bmp); o="/tmp/bench_${T}_${n}"
  "$B" c "$f" "$o" >/dev/null 2>&1
  echo "$n $(wc -c < "$o")"; rm -f "$o"
done
