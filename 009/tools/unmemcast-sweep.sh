#!/bin/bash
# Retype one frame member, run the gate, keep it or put it back.
# `frame-sweep.sh`'s discipline: the rules propose and the gate decides.
set -u
cd "$(dirname "$0")/.."
SAVE=$(mktemp /tmp/unmemcast.XXXXXX.hpp)
trap 'rm -f "$SAVE"' EXIT
kept=0 reverted=0
# a type can carry a `*`, so split on the arrow rather than on whitespace
python3 tools/unmemcast.py subs1.hpp --list |
  sed -E 's/^([^ ]+) +([^ ]+) +-> +(.*[^ ]) +[0-9]+ sites$/__\1|\2|\3/' > /tmp/unmemcast.txt
while IFS='|' read -r fn mem ty; do
  cp subs1.hpp "$SAVE"
  python3 tools/unmemcast.py subs1.hpp "$fn" "$mem" "$ty" >/dev/null || { cp "$SAVE" subs1.hpp; continue; }
  if ./build.sh >/tmp/um.build 2>&1 && ./test.sh >/tmp/um.test 2>&1; then
    echo "keep    ${fn#__} $mem -> $ty"; kept=$((kept+1))
  else
    echo "revert  ${fn#__} $mem -> $ty   $(grep -m1 -E 'error:|FAILED|CHANGED|not 7' /tmp/um.build /tmp/um.test 2>/dev/null | head -c 80)"
    cp "$SAVE" subs1.hpp; reverted=$((reverted+1))
  fi
done < /tmp/unmemcast.txt
echo "$kept kept, $reverted reverted"
