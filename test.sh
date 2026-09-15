#!/bin/sh
#
# test.sh [program] [file] - port of the original t.bat: compress and decompress
# a file, verify the round trip, and append the timings and the compressed size
# to log.txt, the experiment log.
# Defaults: program fpaq0mw, input ../book1 (Calgary corpus; see README).
set -eu
cd "$(dirname "$0")"
p="${1:-fpaq0mw}"
f="${2:-../book1}"

[ -x "./$p" ]   || { echo "test.sh: ./$p not built - run ./build.sh $p" >&2; exit 1; }
[ -f "./timetest" ] || { echo "test.sh: ./timetest not built - see ./build.sh output" >&2; exit 1; }
[ -f "$f" ]     || { echo "test.sh: $f not found. book1 is the Calgary corpus file; see README." >&2; exit 1; }

# Time into a scratch log, not straight into the experiment log: nothing should
# reach log.txt until the round trip has verified. A half-entry there is worse
# than none, since log.pl only closes a record on the size line and would
# attribute stray timings to whatever ran next.
rm -f 1 2 .timetest.log
TIMETEST_LOG=.timetest.log ./timetest "./$p" c "$f" 1
TIMETEST_LOG=.timetest.log ./timetest "./$p" d 1 2

cmp -s "$f" 2 || { rm -f .timetest.log; echo "roundtrip FAILED - nothing written to log.txt" >&2; exit 1; }

cat .timetest.log >> log.txt
wc -c < 1 | tr -d ' ' >> log.txt
echo >> log.txt
rm -f .timetest.log

wc -c < 1
echo "roundtrip OK"
