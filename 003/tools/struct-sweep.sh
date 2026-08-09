#!/bin/sh
# Give the largest remaining objects a struct, one at a time, keeping only the
# ones the gate accepts.
#
#   tools/struct-sweep.sh [rounds]
#
# Each round takes the object with the most constant-offset dereferences that
# is not already on the skip list, rewrites its accesses, and runs the build
# and the roundtrip gate.  A round that fails is undone completely -- the file
# is restored from the backup -- and the object is written to
# tools/struct-skip.txt so the next round moves past it rather than retrying
# it forever.
#
# The sweep is resumable: state lives in the file and the skip list, not in
# this script, so stopping it and starting it again continues where it left
# off.
set -e
cd "$(dirname "$0")/.."

# The inner loop is a filter, not the gate.  It runs the small images -- every
# depth, both alternate model families, both RLE forms and the raw fallback --
# and skips two of the three large photographs, which take most of the time.
#
# It used to skip all three, "which exercise nothing the others do not".  That
# was wrong: a run of 27 applied objects passed this filter and then failed the
# real gate on x_ep alone, and the whole run had to be redone.  x_ep is in the
# filter now -- it costs about eight seconds a round and it is the only image
# ever seen to discriminate on its own.
#
# Run the full `./test.sh ./bmf` before committing.  That is still the
# authority; this is what makes it affordable to ask 30 times in a row, and
# what happened above is what the authority is for.
FILTER=${BMF_SWEEP_IMAGES:-"DLRAW.bmp altp1.bmp med32.bmp noise24.bmp rle4.bmp \
rle8.bmp t1.bmp t24.bmp t32.bmp t8g.bmp t8p.bmp x_ep.bmp"}

rounds=${1:-1}
i=0
while [ "$i" -lt "$rounds" ]; do
  i=$((i + 1))
  cp subs1.hpp /tmp/struct-backup.hpp
  out=$(python3 tools/structs.py subs1.hpp --apply 0 2>&1) && rc=0 || rc=$?
  echo "round $i: $out"
  if [ "$rc" = 3 ]; then
    # the tool declined and put the object on the skip list itself
    cp /tmp/struct-backup.hpp subs1.hpp
    continue
  elif [ "$rc" != 0 ]; then
    echo "round $i: nothing left to apply"
    cp /tmp/struct-backup.hpp subs1.hpp
    break
  fi
  if ./build.sh >/tmp/struct-build.log 2>&1 &&
     BMF_TIMEOUT=${BMF_TIMEOUT:-25} BMF_IMAGES="$FILTER" \
       ./test.sh ./bmf 2>/tmp/struct-test.log | tail -1 | grep -q PASS; then
    echo "round $i: PASS"
  else
    echo "round $i: FAIL -- reverting and skipping"
    cp /tmp/struct-backup.hpp subs1.hpp
    python3 tools/structs.py subs1.hpp --skip 0
  fi
done
