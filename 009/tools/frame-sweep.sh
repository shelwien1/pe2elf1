#!/bin/bash
# Lift one frame, run the gate, keep it or put it back.
#
#   tools/frame-sweep.sh                 # every .inc, every frame defram.py offers
#   tools/frame-sweep.sh model.inc       # one file
#   tools/frame-sweep.sh --arrays        # array members too, which is a guess
#
# `struct-sweep.sh` did this for round one's structs and the reasoning is the
# same: defram.py's rules are static analysis over decompiled code that indexes
# past the ends of arrays, so the rules propose and the gate decides.  A frame
# that fails goes back exactly as it was, and the run continues.
#
# It named `subs1.hpp` for a round after the decompilation stopped being one
# file, and `defram.py` died on the missing path -- after which this printed
# `0 kept, 0 reverted` and exited 0.  That is the zero `sweep.sh` exists to
# distrust, and `sweep.sh` never caught it because it walks `tools/*.py`.  A
# frame lives inside one function and a function inside one `.inc`, so the unit
# here is a file and the default is all of them; `defram.py` reports the same
# per file as it does over the spliced whole.
set -u
cd "$(dirname "$0")/.."

EXTRA= FILES=()
for a in "$@"; do
    case $a in --*) EXTRA="$EXTRA $a" ;; *) FILES+=("$a") ;; esac
done
[ "${#FILES[@]}" -gt 0 ] || FILES=(*.inc)
for f in "${FILES[@]}"; do
    [ -f "$f" ] || { echo "no such file: $f" >&2; exit 2; }
done

SAVE=$(mktemp /tmp/frame-sweep.XXXXXX)
trap 'rm -f "$SAVE"' EXIT

kept=0 reverted=0 offered=0
for f in "${FILES[@]}"; do
    for fn in $(python3 tools/defram.py "$f" --list $EXTRA |
                awk '$2=="lift" && $3>0 {print "__" $1}'); do
        offered=$((offered + 1))
        cp "$f" "$SAVE"
        python3 tools/defram.py "$f" "$fn" $EXTRA > /dev/null || continue
        if ./build.sh > /tmp/frame-sweep.build 2>&1 && ./test.sh > /tmp/frame-sweep.test 2>&1
        then
            echo "keep    ${fn#__}"
            kept=$((kept + 1))
        else
            echo "revert  ${fn#__}   $(grep -m1 -E 'error:|FAILED|not 7' \
                 /tmp/frame-sweep.build /tmp/frame-sweep.test 2>/dev/null |
                 head -c 100)"
            cp "$SAVE" "$f"
            reverted=$((reverted + 1))
        fi
    done
done
# "0 kept, 0 reverted" is what this said when it had crashed, so it is not what
# it says when there was nothing to do.
if [ "$offered" = 0 ]; then
    echo "not applicable: defram.py offers no frame to lift in ${#FILES[@]} files"
else
    echo "$offered offered: $kept kept, $reverted reverted"
fi
./build.sh > /dev/null 2>&1
