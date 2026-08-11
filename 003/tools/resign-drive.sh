#!/bin/bash
# Apply every retype that measurably reduces the conversion count.
#
#     tools/resign-drive.sh
#
# `resign.py`'s net-effect test sees a local flowing into other locals; it
# cannot see it flowing into a struct member or a call argument, so for a
# handful of candidates it cannot tell a removal from a move.  This settles
# those the only way they can be settled: apply one, rebuild, count.
#
# Slow on purpose -- one build per candidate per round.  It stops when no
# single candidate reduces the count, which is the fixpoint of the rule and
# not of the tool's approximation to it.
set -u
cd "$(dirname "$0")/.."
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
BMF_WARN=1 ./build.sh >/dev/null 2>&1
base=$(BMF_WARN=1 ./build.sh 2>/dev/null | head -1)
echo "start $base"
for round in 1 2 3 4 5 6 7 8; do
  cp subs1.hpp "$T/base.hpp"; cp warn.log "$T/base.warn"
  n=$(python3 tools/resign.py subs1.hpp | tail -1 | cut -d' ' -f1)
  [ "$n" = 0 ] && { echo "round $round: no candidates"; break; }
  best=-1; bestn=$base
  for k in $(seq 0 $((n - 1))); do
    cp "$T/base.hpp" subs1.hpp; cp "$T/base.warn" warn.log
    python3 tools/resign.py subs1.hpp --all --only=$k >/dev/null 2>&1
    ./build.sh >/dev/null 2>&1 || continue
    now=$(BMF_WARN=1 ./build.sh 2>/dev/null | head -1)
    [ "$now" -lt "$bestn" ] && { best=$k; bestn=$now; }
  done
  cp "$T/base.hpp" subs1.hpp; cp "$T/base.warn" warn.log
  [ "$best" = -1 ] && { echo "round $round: none of $n reduces"; break; }
  who=$(python3 tools/resign.py subs1.hpp | sed -n "$((best + 1))p")
  python3 tools/resign.py subs1.hpp --all --only=$best >/dev/null
  ./build.sh >/dev/null 2>&1
  if [ "$(./tools/triage.sh 2>&1 | tail -1)" != "TRIAGE OK" ]; then
    echo "round $round: $who moved a stream -- reverted"
    cp "$T/base.hpp" subs1.hpp; break
  fi
  echo "round $round: $base -> $bestn  $who"
  base=$bestn
  BMF_WARN=1 ./build.sh >/dev/null 2>&1
done
./build.sh >/dev/null 2>&1
