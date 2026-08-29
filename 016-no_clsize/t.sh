#!/bin/sh
#
# Build, roundtrip, stream check and benchmark -- the t.bat equivalent.
#
#   ./t.sh                                    the default configuration
#   ./t.sh -DRC_RCNUM=32 -DRC_LOWBYTES=4      built with these
#   ./t.sh "-DRC_RCNUM=32 -DRC_LOWBYTES=4"    the same, as one argument
#
#   TESTFILE=../book1 ./t.sh    what to code (default: ../enwik8)
#   NITER=3 ./t.sh              encode passes to time (default: 10)
#   NITERD=3 ./t.sh             decode passes to time (default: same as NITER)
#   FSM=../FSM1.txt ./t.sh      the counter state machine (default: ../FSM0.txt)
#   KEEP=1 ./t.sh               leave t.enc / t.dec / t.ref behind
#
# The arguments are the -D set, passed straight to build.sh -- t.sh always
# rebuilds, so the binary it measures is the one those flags describe.
#
# Then three things happen, in this order:
#
#   1. coder's own test mode encodes the file NITER times, decodes the result
#      NITERD times, and appends the best encode and decode timings to log.txt. t.sh
#      tags that line with the compiler and ISA build.sh recorded, which is
#      the "avx512" / "avx2" column the existing lines carry.
#   2. the decoded file is compared against the input.
#   3. the file is encoded again with -C, the scalar reference coder, and the
#      two streams are compared byte for byte. The vector coder is a
#      transformation of the same rc.inc and is meant to be bit-exact with it;
#      if it were not, step 2 could still pass -- the stream would just be a
#      different valid one -- so this is the check that pins it down.

set -e
cd "$(dirname "$0")"

IN=${TESTFILE:-../enwik8}
NITER=${NITER:-10}
NITERD=${NITERD:-$NITER}
FSM=${FSM:-../FSM0.txt}
CODER=./coder

enc=t.enc; dec=t.dec; ref=t.ref
[ "${KEEP:-0}" = 1 ] || trap 'rm -f "$enc" "$dec" "$ref"' EXIT

# unquoted on purpose: it also takes the whole -D set as one quoted argument
# shellcheck disable=SC2086
./build.sh $@

if [ ! -f "$FSM" ]; then
  echo "t.sh: no counter state machine at $FSM" >&2
  exit 1
fi
if [ ! -f "$IN" ]; then
  echo "t.sh: no test file at $IN -- run ../get_enwik8.sh, or set TESTFILE" >&2
  exit 1
fi

insize=$(wc -c < "$IN")
lines0=$(wc -l < log.txt 2>/dev/null || echo 0)

echo "== $IN ($insize bytes), $NITER encode / $NITERD decode passes, FSM $FSM"
rm -f "$enc" "$dec" "$ref"

# encode x NITER, decode x1, append to log.txt -- this is what t.bat runs
"$CODER" c "$IN" "$enc" "$FSM" "$NITER" "$dec" "$NITERD"

encsize=$(wc -c < "$enc")

echo "== roundtrip"
if cmp -s "$IN" "$dec"; then
  echo "   ok: decoded output matches the input"
else
  echo "   FAILED: decoded output differs from the input" >&2
  cmp "$IN" "$dec" >&2 || true
  exit 1
fi

echo "== vector stream vs the -C scalar reference"
"$CODER" -C c "$IN" "$ref" "$FSM" 1
if cmp -s "$enc" "$ref"; then
  echo "   ok: byte-identical"
else
  echo "   FAILED: the vector coder and the scalar reference disagree" >&2
  cmp "$enc" "$ref" >&2 || true
  exit 1
fi

# Tag the line coder just appended with what this binary was built as.
lines1=$(wc -l < log.txt 2>/dev/null || echo 0)
if [ -f "$CODER.tag" ] && [ "$lines1" -gt "$lines0" ]; then
  awk -v n="$lines1" -v t=" $(cat "$CODER.tag")" \
      'NR==n { printf "%s%s\n", $0, t; next } { print }' log.txt > log.txt.new
  mv log.txt.new log.txt
fi

echo "== $encsize bytes, $(awk -v a="$encsize" -v b="$insize" 'BEGIN{printf "%.4f", a*8/b}') bpc"
echo "== log.txt"
tail -1 log.txt
