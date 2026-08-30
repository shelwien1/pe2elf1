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
#   3. if REFSTREAM names a file, the encoded stream is compared against it --
#      or recorded into it when it does not exist yet. Roundtripping only says
#      the stream decodes to the input; it does not say it is the SAME stream,
#      so a knob that silently changed the format would still pass step 2.
#      t_matrix.sh points every format-neutral row at one reference to pin
#      that down across the whole -D space.

set -e
cd "$(dirname "$0")"

IN=${TESTFILE:-../enwik8}
NITER=${NITER:-10}
NITERD=${NITERD:-$NITER}
FSM=${FSM:-../FSM0.txt}
CODER=./coder

# A build whose encoder is wrong decodes to a stream that never ends -- the
# block length is coded INSIDE the rangecoder data, so a bad stream can ask the
# decoder for bytes forever. Cap the write: 4x the input is far more than any
# correct run needs, and the cap turns "filled the disk" into "test failed".
ulimit -f $(( ($(wc -c < "$IN") / 512 + 1) * 4 )) 2>/dev/null || true

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


# Tag the line coder just appended with what this binary was built as.
lines1=$(wc -l < log.txt 2>/dev/null || echo 0)
if [ -f "$CODER.tag" ] && [ "$lines1" -gt "$lines0" ]; then
  awk -v n="$lines1" -v t=" $(cat "$CODER.tag")" \
      'NR==n { printf "%s%s\n", $0, t; next } { print }' log.txt > log.txt.new
  mv log.txt.new log.txt
fi

if [ -n "${REFSTREAM:-}" ]; then
  if [ -f "$REFSTREAM" ]; then
    if cmp -s "$enc" "$REFSTREAM"; then
      echo "   ok: stream matches the reference"
    else
      echo "   FAILED: this build's stream differs from the reference" >&2
      exit 1
    fi
  else
    cp "$enc" "$REFSTREAM"
    echo "   ok: stream recorded as the reference"
  fi
fi

echo "== $encsize bytes, $(awk -v a="$encsize" -v b="$insize" 'BEGIN{printf "%.4f", a*8/b}') bpc"
echo "== log.txt"
tail -1 log.txt
