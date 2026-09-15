#!/bin/sh
#
# test.sh [program] [file] - port of the original t.bat: compress and decompress
# a file, append the timings and the compressed size to log.txt (the experiment
# log), and verify the round trip.
# Defaults: program fpaq0mw, input ../book1 (Calgary corpus).
set -eu
cd "$(dirname "$0")"
p="${1:-fpaq0mw}"
f="${2:-../book1}"

rm -f 1 2
./timetest "./$p" c "$f" 1
./timetest "./$p" d 1 2

wc -c < 1 | tr -d ' ' >> log.txt
echo >> log.txt

wc -c < 1
if cmp -s "$f" 2; then echo "roundtrip OK"; else echo "roundtrip FAILED"; exit 1; fi
