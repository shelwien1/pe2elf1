#!/bin/bash
# Lift one frame, run the gate, keep it or put it back.
#
#   tools/frame-sweep.sh            # every frame defram.py offers
#   tools/frame-sweep.sh --arrays   # array members too, which is a guess
#
# `struct-sweep.sh` did this for round one's structs and the reasoning is the
# same: defram.py's rules are static analysis over decompiled code that indexes
# past the ends of arrays, so the rules propose and the gate decides.  A frame
# that fails goes back exactly as it was, and the run continues.
set -u
cd "$(dirname "$0")/.."
EXTRA=${1:-}
SAVE=$(mktemp /tmp/frame-sweep.XXXXXX.hpp)
trap 'rm -f "$SAVE"' EXIT

kept=0 reverted=0
for fn in $(python3 tools/defram.py subs1.hpp --list $EXTRA |
            awk '$2=="lift" && $3>0 {print "__" $1}'); do
    cp subs1.hpp "$SAVE"
    python3 tools/defram.py subs1.hpp "$fn" $EXTRA > /dev/null || continue
    if ./build.sh > /tmp/frame-sweep.build 2>&1 && ./test.sh > /tmp/frame-sweep.test 2>&1
    then
        echo "keep    ${fn#__}"
        kept=$((kept + 1))
    else
        echo "revert  ${fn#__}   $(grep -m1 -E 'error:|FAILED|not 7' \
             /tmp/frame-sweep.build /tmp/frame-sweep.test 2>/dev/null |
             head -c 100)"
        cp "$SAVE" subs1.hpp
        reverted=$((reverted + 1))
    fi
done
echo "$kept kept, $reverted reverted"
./build.sh > /dev/null 2>&1
