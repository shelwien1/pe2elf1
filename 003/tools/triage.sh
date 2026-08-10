#!/bin/sh
# Compress a few images and compare against their reference streams.
#
#     tools/triage.sh                 # the four that fail first
#     tools/triage.sh t8g x_ci        # named ones
#
# `test.sh` is the gate and this is not: it skips the round trip, the archive,
# the malformed inputs, the OOM ladder and both compiler ratchets.  What it is
# for is bisecting a change that broke something, where the question is only
# "does the stream still match" and the full gate's minutes per attempt are the
# whole cost.  Answer with `test.sh` before committing, always.
#
# The output file is removed first, and that is not tidiness: `bmf c` *appends*
# a member to an archive that already exists, so a second run against a stale
# file produces a two-member stream and reports a difference that is entirely
# the script's own doing.  Leaving that out cost one wrong bisection here --
# a known-good tree looked broken.
set -e
cd "$(dirname "$0")/.."
./build.sh >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 1; }
bad=0
for i in ${*:-med32 noise24 t24 t32}; do
  rm -f "run/tri_$i.bmf"
  if ! (cd run && timeout "${BMF_TIMEOUT:-120}" ../bmf c "orig_$i.bmp" "tri_$i.bmf" >/dev/null 2>&1)
  then
    echo "  $i: compress failed (rc=$?)"; bad=$((bad + 1)); continue
  fi
  cmp -s "run/tri_$i.bmf" "testfiles/ref_$i.bmf" ||
    { echo "  $i: stream differs"; bad=$((bad + 1)); }
done
[ "$bad" -eq 0 ] && echo "TRIAGE OK" || echo "TRIAGE BAD ($bad)"
[ "$bad" -eq 0 ]
