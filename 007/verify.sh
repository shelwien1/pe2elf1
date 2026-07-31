#!/bin/sh
# verify.sh <binary> [label] -- the test for the contract mk.sh states in its
# header and IDX-FORMAT.md states in sections 1 and 12: the tuning build and the
# shipping build must produce the SAME archive.
#
#   ./mk.sh         && ./verify.sh ./paq8hpc_idx tuning
#   ./mk.sh release && ./verify.sh ./paq8hpc_idx release
#
# It records the archive checksum on first run and compares against it after
# that, so the second of those two lines is the check.  Delete verify.md5 to
# re-baseline after a deliberate model change -- and only then, because a
# changed checksum is otherwise exactly what this is here to catch.
#
# Why it matters, from the one time it did: CM_DECAY_BIAS was seeded below
# CM_DECAY_FROM, which made a shift count negative.  Undefined behaviour, and
# the two builds resolved it differently -- the shipping build proved the
# ContextMap bit-history decay unreachable and deleted it, the tuning build ran
# it.  Both compiled, both round-tripped, both looked fine on their own.  Only
# comparing the two archives says anything.
set -e

BIN=${1:-./paq8hpc_idx}
LABEL=${2:-build}
IN=${IN:-book1}
LEVEL=${LEVEL:-7}          # paq8hp.hpp pins FIXED_LEVEL; this must match it
SUMS=${SUMS:-verify.md5}

[ -x "$BIN" ] || { echo "verify.sh: no such binary: $BIN" >&2; exit 2; }
[ -r "$IN" ]  || { echo "verify.sh: no such input: $IN" >&2; exit 2; }

tmp=`mktemp -d`
trap 'rm -rf "$tmp"' EXIT

"$BIN" c "$IN" "$tmp/a" $LEVEL >/dev/null
"$BIN" d "$tmp/a" "$tmp/b" >/dev/null

# Round-trip first: a checksum that matches on a build that cannot decode its
# own output would be a pass for the wrong reason.
if ! cmp -s "$IN" "$tmp/b"; then
  echo "verify.sh: FAIL ($LABEL) -- $IN does not survive the round trip" >&2
  exit 1
fi

sum=`md5sum < "$tmp/a" | cut -d' ' -f1`
size=`wc -c < "$tmp/a" | tr -d ' '`

if [ -r "$SUMS" ]; then
  want=`cut -d' ' -f1 "$SUMS"`
  if [ "$sum" != "$want" ]; then
    echo "verify.sh: FAIL ($LABEL) -- $IN codes to $sum, $SUMS says $want" >&2
    echo "verify.sh: the two builds do not agree, or the model moved." >&2
    exit 1
  fi
  echo "verify.sh: ok ($LABEL) -- $IN -> $size, $sum"
else
  echo "$sum  $IN level $LEVEL" > "$SUMS"
  echo "verify.sh: recorded ($LABEL) -- $IN -> $size, $sum -> $SUMS"
fi
