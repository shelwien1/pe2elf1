#!/bin/bash
# Retype one frame member, run the gate, keep it or put it back.
#
#   tools/unmemcast-sweep.sh              # every .inc
#   tools/unmemcast-sweep.sh model.inc    # one file
#
# `frame-sweep.sh`'s discipline: the rules propose and the gate decides, and
# the same note about the file it names applies -- this drove `subs1.hpp` for a
# round after there was no such file, and answered `0 kept, 0 reverted` to a
# `unmemcast.py` that had died on the path.  A frame member lives in one `.inc`
# and the tool reports the same per file as over the spliced whole.
set -u
cd "$(dirname "$0")/.."

FILES=("$@")
[ "${#FILES[@]}" -gt 0 ] || FILES=(*.inc)
for f in "${FILES[@]}"; do
  [ -f "$f" ] || { echo "no such file: $f" >&2; exit 2; }
done

SAVE=$(mktemp /tmp/unmemcast.XXXXXX)
LIST=$(mktemp /tmp/unmemcast-list.XXXXXX)
trap 'rm -f "$SAVE" "$LIST"' EXIT
kept=0 reverted=0 offered=0
for f in "${FILES[@]}"; do
  # a type can carry a `*`, so split on the arrow rather than on whitespace
  python3 tools/unmemcast.py "$f" --list |
    sed -nE 's/^([^ ]+) +([^ ]+) +-> +(.*[^ ]) +[0-9]+ sites$/__\1|\2|\3/p' > "$LIST"
  # `sed -n ... p`, not `sed ...`: an untransformed line -- the tool's own
  # summary, or its header -- came through as a whole-line `$fn` with no `|`
  # in it, and every file counted one.  The run reported "36 offered" against
  # a tool that had offered nothing, which is a fake finding of exactly the
  # shape the fake zeros above were.
  while IFS='|' read -r fn mem ty; do
    [ -n "$fn" ] && [ -n "$mem" ] && [ -n "$ty" ] || continue
    offered=$((offered+1))
    cp "$f" "$SAVE"
    python3 tools/unmemcast.py "$f" "$fn" "$mem" "$ty" >/dev/null ||
      { cp "$SAVE" "$f"; continue; }
    if ./build.sh >/tmp/um.build 2>&1 && ./test.sh >/tmp/um.test 2>&1; then
      echo "keep    ${fn#__} $mem -> $ty"; kept=$((kept+1))
    else
      echo "revert  ${fn#__} $mem -> $ty   $(grep -m1 -E 'error:|FAILED|CHANGED|not 7' /tmp/um.build /tmp/um.test 2>/dev/null | head -c 80)"
      cp "$SAVE" "$f"; reverted=$((reverted+1))
    fi
  done < "$LIST"
done
if [ "$offered" = 0 ]; then
  echo "not applicable: unmemcast.py offers no member to retype in ${#FILES[@]} files"
else
  echo "$offered offered: $kept kept, $reverted reverted"
fi
