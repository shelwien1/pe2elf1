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

# Which rule to drive.  `--group` measures `resign_group.py`'s components --
# sets of locals that have to agree, where flipping any one alone was already
# measured not to pay.
tool=tools/resign.py
[ "${1:-}" = --group ] && tool=tools/resign_group.py
DRIVEN=tools/driven.txt
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
BMF_WARN=1 ./build.sh >/dev/null 2>&1
base=$(BMF_WARN=1 ./build.sh 2>/dev/null | head -1)
echo "start $base"
# No fixed cap: the loop ends when no single candidate reduces the count,
# which is the fixpoint.  A cap of eight looked like a fixpoint once and
# was not -- it stopped with the count still falling every round.
round=0
stop="ran out of rounds"
while round=$((round + 1)); [ $round -le 200 ]; do
  cp subs1.hpp "$T/base.hpp"; cp warn.log "$T/base.warn"
  n=$(python3 "$tool" subs1.hpp | tail -1 | cut -d' ' -f1)
  [ "$n" = 0 ] && { echo "round $round: no candidates"; stop="no candidates left"; break; }
  best=-1; bestn=$base
  for k in $(seq 0 $((n - 1))); do
    cp "$T/base.hpp" subs1.hpp; cp "$T/base.warn" warn.log
    python3 "$tool" subs1.hpp --all --only=$k >/dev/null 2>&1
    # `BMF_WARN=1` is `-fsyntax-only`, so one pass both counts the warnings and
    # says whether the edit compiles.  Doing a full -O2 build per candidate as
    # well made a round of 27 take twenty-five minutes for information the
    # syntax pass already had.
    now=$(BMF_WARN=1 ./build.sh 2>/dev/null | head -1)
    { [ -n "$now" ] && ! grep -q ' error: ' warn.log; } || continue
    [ "$now" -lt "$bestn" ] && { best=$k; bestn=$now; }
  done
  cp "$T/base.hpp" subs1.hpp; cp "$T/base.warn" warn.log
  [ "$best" = -1 ] && { echo "round $round: none of $n reduces"; stop="none of $n reduces the count"; break; }
  who=$(python3 "$tool" subs1.hpp | sed -n "$((best + 1))p")
  python3 "$tool" subs1.hpp --all --only=$best >/dev/null
  ./build.sh >/dev/null 2>&1
  if [ "$(./tools/triage.sh 2>&1 | tail -1)" != "TRIAGE OK" ]; then
    echo "round $round: $who moved a stream -- reverted"
    stop="stopped early: $who moved a stream"
    cp "$T/base.hpp" subs1.hpp; break
  fi
  echo "round $round: $base -> $bestn  $who"
  base=$bestn
  BMF_WARN=1 ./build.sh >/dev/null 2>&1
done
./build.sh >/dev/null 2>&1

# Record the verdict where the tool that made the offers can read it.
#
# `resign.py` prints 28 candidates and `resign_group.py` 23, and every one of
# them has been measured here and rejected -- but nothing said so, so the two
# tools read as 51 things still to do.  That is `liftframe.py`'s defect in
# REFACTORING9.md section 32 exactly: a list of offers with no note that they
# were tried.  `liftframe` hardcodes its failures; these change whenever the
# file does, so the verdict is stamped with the source's checksum and goes
# stale by itself.
{
  grep -v "^$(basename "$tool") " "$DRIVEN" 2>/dev/null | grep -v '^# subs1.hpp '
  printf '%s %s\n' "$(basename "$tool")" "$stop"
  printf '# subs1.hpp %s\n' "$(cksum < subs1.hpp)"
} > "$T/driven" && mv "$T/driven" "$DRIVEN"
echo "$DRIVEN: $(basename "$tool") $stop"
