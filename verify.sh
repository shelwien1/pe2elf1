#!/bin/bash
# verify.sh <binary> [tag]
#
# Byte-identity regression for a paq8hpc_speed build.  Every gate in
# paq8hp_speed.hpp is claimed output-preserving, so every build must reproduce
# the same archives as the pristine baseline and must round-trip its own output
# and cross-decode the baseline's.
#
# Reference values are the documented ones from paq8hpc_speed.md plus levels
# generated once from the pristine baseline build (ref/ directory).
set -u
BIN=${1:?usage: verify.sh <binary> [tag]}
TAG=${2:-$(basename "$BIN")}
W=${W:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad/vfy}
REF=${REF:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad/ref}
mkdir -p "$W"
fail=0

# input, level  (kept small enough that the whole set runs in ~30 s)
CASES="b300:0 b64k:4 b64k:0 b128k:6 b128k:8 b16k:1 b16k:11"

for c in $CASES; do
  inp=${c%:*}; lvl=${c#*:}
  out=$W/$TAG.$inp.$lvl
  if ! "$BIN" c "$W/$inp" "$out" "$lvl" >/dev/null 2>&1; then
    echo "  FAIL($TAG) encode $inp L$lvl: nonzero exit"; fail=1; continue
  fi
  got=$(md5sum <"$out" | cut -d' ' -f1)
  want=$(md5sum <"$REF/$inp.$lvl" | cut -d' ' -f1)
  if [ "$got" != "$want" ]; then
    echo "  FAIL($TAG) encode $inp L$lvl: $got != $want ($(stat -c%s "$out") vs $(stat -c%s "$REF/$inp.$lvl") bytes)"; fail=1; continue
  fi
  # own round-trip
  if ! "$BIN" d "$out" "$out.dec" >/dev/null 2>&1 || ! cmp -s "$W/$inp" "$out.dec"; then
    echo "  FAIL($TAG) decode $inp L$lvl: round-trip mismatch"; fail=1; continue
  fi
  # cross-decode: this build must decode the pristine baseline's archive
  if ! "$BIN" d "$REF/$inp.$lvl" "$out.xdec" >/dev/null 2>&1 || ! cmp -s "$W/$inp" "$out.xdec"; then
    echo "  FAIL($TAG) cross-decode $inp L$lvl"; fail=1; continue
  fi
  rm -f "$out.dec" "$out.xdec"
done

# the two values published in paq8hpc_speed.md, checked by hand
h=$(md5sum <"$W/$TAG.b64k.4" | cut -d' ' -f1)
s=$(stat -c%s "$W/$TAG.b64k.4")
[ "$s" = 20321 ] && [ "$h" = 5b20ec75933c5f38b209d9948e432ed8 ] || { echo "  FAIL($TAG) doc vector b64k L4: $s $h"; fail=1; }
[ "$(stat -c%s "$W/$TAG.b128k.6")" = 37970 ] || { echo "  FAIL($TAG) doc vector b128k L6"; fail=1; }

[ $fail = 0 ] && echo "  ok($TAG) 7 cases x {encode,round-trip,cross-decode} + 2 doc vectors"
exit $fail
