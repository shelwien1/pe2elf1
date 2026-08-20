#!/bin/sh
# Compress a few images and compare against their reference streams.
#
#     tools/triage.sh                 # the four that fail first
#     tools/triage.sh t8g x_ci        # named ones
#
# `test.sh` is the gate and this is not: it skips the round trip, the archive,
# the malformed inputs and the OOM ladder.  What it is for is bisecting a change
# that broke something, where the question is only "does the stream still match"
# and the full gate's minutes per attempt are the whole cost.  Answer with
# `test.sh` before committing, always.
#
# This used to say `test.sh` covered "both compiler ratchets" as well.  It never
# has -- the ratchets and the other six builds are legs of `build.sh`, and for
# three rounds nothing ran any of them, which is how `BMF_BITS=32` came to not
# compile at all without anybody noticing.  `tools/legs.sh` runs them now, and
# is what a round ends with.
#
# The output file is removed first.  It used to be load-bearing -- `bmf c`
# appended a member to an archive that already existed, so a second run against
# a stale file produced a two-member stream and a difference that was entirely
# the script's own doing, which cost one wrong bisection here.  `c` creates its
# output now and the `rm` is belt and braces: a stale file still makes "did not
# write" and "wrote the same bytes" look alike.
#
# The images come from `testfiles/`, which is where they are.  This used to
# compress `run/orig_$i.bmp` from inside a `run/` directory that no revision of
# this tree has ever had, so every image reported "compress failed" and the
# answer was `TRIAGE BAD` whatever the code did -- and `tools/resign-drive.sh`
# gates each candidate on that answer, so it reverted every one of them.
set -e
cd "$(dirname "$0")/.."
./build.sh >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 1; }
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
bad=0
for i in ${*:-med32 noise24 t24 t32}; do
  rm -f "$out/tri_$i.bmf"
  # Run it, *then* read the code: `$?` after `if ! cmd` is the negation's
  # status and is always 0, which is how this reported "compress failed
  # (rc=0)" for four images at once.  Same trap tools/x64.sh documents.
  timeout "${BMF_TIMEOUT:-120}" ./bmf c "testfiles/$i.bmp" "$out/tri_$i.bmf" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    echo "  $i: compress failed (rc=$rc)"; bad=$((bad + 1)); continue
  fi
  cmp -s "$out/tri_$i.bmf" "testfiles/ref_$i.bmf" ||
    { echo "  $i: stream differs"; bad=$((bad + 1)); }
done
[ "$bad" -eq 0 ] && echo "TRIAGE OK" || echo "TRIAGE BAD ($bad)"
[ "$bad" -eq 0 ]
