#!/bin/bash
# verify_rt.sh <binary> [tag]
#
# Round-trip-only verification, for builds that deliberately change the
# bitstream (PAQ_LOGISTIC).  verify.sh cannot be used on those: it asserts
# byte-identity against the pristine baseline's archives, which such a build must
# fail by design.  What still has to hold is self-consistency -- the encoder and
# decoder of the same build must agree exactly -- plus the archive being smaller
# than nothing and the level round-tripping through the header.
#
# Also reports the size against the baseline archive so the compression effect is
# visible per case.
set -u
BIN=${1:?usage: verify_rt.sh <binary> [tag]}
TAG=${2:-$(basename "$BIN")}
S=${S:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad}
W=${W:-$S/vfy}
REF=${REF:-$S/ref}
fail=0
tot_new=0; tot_old=0

CASES=${CASES:-"b300:0 b64k:4 b64k:0 b128k:6 b128k:8 b16k:1 b16k:11"}

for c in $CASES; do
  inp=${c%:*}; lvl=${c#*:}
  out=$S/rt.$TAG.$inp.$lvl
  if ! "$BIN" c "$W/$inp" "$out" "$lvl" >/dev/null 2>&1 || [ ! -s "$out" ]; then
    echo "  FAIL($TAG) encode $inp L$lvl"; fail=1; continue
  fi
  if ! "$BIN" d "$out" "$out.dec" >/dev/null 2>&1 || ! cmp -s "$W/$inp" "$out.dec"; then
    echo "  FAIL($TAG) ROUND-TRIP $inp L$lvl -- encoder and decoder disagree"; fail=1; continue
  fi
  n=$(stat -c%s "$out"); o=$(stat -c%s "$REF/$inp.$lvl")
  tot_new=$((tot_new+n)); tot_old=$((tot_old+o))
  printf '  ok  %-10s L%-2s  %8s -> %8s  %+7.3f%%\n' "$inp" "$lvl" "$o" "$n" \
    "$(awk -v a="$n" -v b="$o" 'BEGIN{printf "%.3f",(a/b-1)*100}')"
  rm -f "$out.dec"
done

if [ $fail = 0 ]; then
  printf '  --- %s total %s -> %s  %+.3f%%\n' "$TAG" "$tot_old" "$tot_new" \
    "$(awk -v a="$tot_new" -v b="$tot_old" 'BEGIN{printf "%.3f",(a/b-1)*100}')"
fi
exit $fail
