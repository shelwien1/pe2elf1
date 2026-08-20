#!/bin/bash
# Apply every retype that measurably reduces the conversion count.
#
#     tools/resign-drive.sh                    # resign.py, over bmf.cpp
#     tools/resign-drive.sh model.inc          # resign.py, over one module
#     tools/resign-drive.sh --group model.inc  # resign_group.py, over one module
#
# One file per run, because that is the unit `resign.py` rewrites.  `bmf.cpp` is
# the two bodies that still live there, so driving the whole program is a loop
# over the `.inc` files and not a single call -- and the default target being
# the include list is why this answered "no candidates" for rounds.
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
if [ "${1:-}" = --group ]; then tool=tools/resign_group.py; shift; fi
DRIVEN=tools/driven.txt

# The file the rule is applied to.  This was `subs1.hpp` for a round after the
# tree stopped having one, so every `cp` below copied nothing, `resign.py` died
# on the missing path, and `n` came out empty -- after which `[ "$n" = 0 ]` was
# false and the loop drove 200 rounds of nothing.  `sweep.sh` walks
# `tools/*.py` and never asked.
#
# `resign.py` reads the file it is given and writes it back, so the unit it can
# apply to is one file: `bmf.cpp` is the two bodies that still live there, and a
# `.inc` is that module.  Over the spliced whole -- which is what `sweep.sh`
# hands it -- both rules report zero, so there is nothing here to drive; the
# loop below says so rather than spending 200 rounds finding out.
# The file is the first argument now, after `--group` has been taken off.  It
# used to be the *second* unconditionally, so `resign-drive.sh model.inc` drove
# `bmf.cpp` and answered "no candidates" -- which is true of `bmf.cpp` and says
# nothing about `model.inc`.  There was no spelling that named a module without
# `--group`, and the rule this drives applies per file.
SRC=${1:-bmf.cpp}
[ -f "$SRC" ] || { echo "no such file: $SRC" >&2; exit 2; }
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
# **With the conversion flags, and that is the whole of what this was missing.**
# `BMF_WARN=1 ./build.sh` alone builds the *default* warning set, which carries
# neither `-Wconversion` nor `-Wsign-conversion` -- so `resign.py` had nothing
# to read, answered "0 locals declared against their own assignments", and this
# loop read the leading 0 as "no candidates" and stopped on round one calling it
# the fixpoint.  A driver that reports a fixpoint it never drove to is worse
# than one that dies, because the answer looks like the answer.
#
# Found when `resign.py` started saying `not applicable` instead of a zero and
# the `cut` below produced the word `not`.  Given the flags the rule has nine
# candidates on this tree, not none.
CONV='-Wconversion -Wsign-conversion'
BMF_WARN=1 ./build.sh $CONV >/dev/null 2>&1
base=$(BMF_WARN=1 ./build.sh $CONV 2>/dev/null | head -1)
echo "start $base"
# No fixed cap: the loop ends when no single candidate reduces the count,
# which is the fixpoint.  A cap of eight looked like a fixpoint once and
# was not -- it stopped with the count still falling every round.
round=0
stop="ran out of rounds"
while round=$((round + 1)); [ $round -le 200 ]; do
  cp "$SRC" "$T/base.hpp"; cp warn.log "$T/base.warn"
  n=$(python3 "$tool" "$SRC" | tail -1 | cut -d' ' -f1)
  # An empty `n` is not a zero: it is what a tool that died prints, and the
  # test below read it as "not 0" and drove another round.
  [ -n "$n" ] || { echo "round $round: $(basename "$tool") said nothing"; stop="$(basename "$tool") said nothing"; break; }
  # Nor is a *word* a zero.  `not applicable: ...` cuts to `not`, which reached
  # `$((n - 1))` below as an unbound variable and took the loop down with it.
  case $n in *[!0-9]*) echo "round $round: $(basename "$tool") said: $(python3 "$tool" "$SRC" | tail -1)"; stop="$(basename "$tool") had nothing to read"; break ;; esac
  [ "$n" = 0 ] && { echo "round $round: no candidates"; stop="no candidates left"; break; }
  best=-1; bestn=$base
  for k in $(seq 0 $((n - 1))); do
    cp "$T/base.hpp" "$SRC"; cp "$T/base.warn" warn.log
    python3 "$tool" "$SRC" --all --only=$k >/dev/null 2>&1
    # `BMF_WARN=1` is `-fsyntax-only`, so one pass both counts the warnings and
    # says whether the edit compiles.  Doing a full -O2 build per candidate as
    # well made a round of 27 take twenty-five minutes for information the
    # syntax pass already had.
    now=$(BMF_WARN=1 ./build.sh $CONV 2>/dev/null | head -1)
    { [ -n "$now" ] && ! grep -q ' error: ' warn.log; } || continue
    [ "$now" -lt "$bestn" ] && { best=$k; bestn=$now; }
  done
  cp "$T/base.hpp" "$SRC"; cp "$T/base.warn" warn.log
  [ "$best" = -1 ] && { echo "round $round: none of $n reduces"; stop="none of $n reduces the count"; break; }
  who=$(python3 "$tool" "$SRC" | sed -n "$((best + 1))p")
  python3 "$tool" "$SRC" --all --only=$best >/dev/null
  ./build.sh >/dev/null 2>&1
  if [ "$(./tools/triage.sh 2>&1 | tail -1)" != "TRIAGE OK" ]; then
    echo "round $round: $who moved a stream -- reverted"
    stop="stopped early: $who moved a stream"
    cp "$T/base.hpp" "$SRC"; break
  fi
  echo "round $round: $base -> $bestn  $who"
  base=$bestn
  BMF_WARN=1 ./build.sh >/dev/null 2>&1
done
./build.sh >/dev/null 2>&1

# Record the verdict where the tool that made the offers can read it.
#
# `resign.py` printed 28 candidates and `resign_group.py` 23 when this was
# written, and every one of them had been measured here and rejected -- but
# nothing said so, so the two tools read as 51 things still to do.  Both report
# zero over the spliced whole now; the mechanism is what stays.  That is
# `liftframe.py`'s defect in
# REFACTORING9.md section 32 exactly: a list of offers with no note that they
# were tried.  `liftframe` hardcodes its failures; these change whenever the
# file does, so the verdict is stamped with the source's checksum and goes
# stale by itself.
#
# The row names the file it drove, which it did not.  The rule applies per file
# and six drives in a row left one row saying "resign.py none of 2 reduces the
# count" -- true of whichever file went last and silent about the other five,
# under a stamp claiming to be current for the whole tree.  A verdict that
# cannot say what it is a verdict about is the same defect one level in.
{
  # The file's own header, if it has one: every `#` line before the first row.
  # This dropped them, so the record explained itself until the first time
  # anything drove it and was a bare list of verdicts afterwards.  The stamp at
  # the bottom is a `#` line too, which is why the filter below is anchored on
  # the rows and this one on position.
  awk '/^#/ && !seen { print; next } NF { seen = 1 }' "$DRIVEN" 2>/dev/null
  grep -v "^$(basename "$tool") $SRC " "$DRIVEN" 2>/dev/null | grep -v '^#'
  printf '%s %s %s\n' "$(basename "$tool")" "$SRC" "$stop"
  # `buildlog.STAMP`, and not a `cksum` of one file: `resign.driven` reads this
  # back through `buildlog.read`, which wants `# sources <digest>` over the
  # whole tree.  It was `# subs1.hpp <cksum>`, which that reader has never been
  # able to parse -- so every verdict written here since the split has come
  # back as "stale" and the memo has printed nothing, which is the same silence
  # as no memo at all.
  python3 -c "import sys; sys.path.insert(0, 'tools'); import buildlog
print('# sources ' + buildlog.digest('$SRC'))"
} > "$T/driven" && mv "$T/driven" "$DRIVEN"
echo "$DRIVEN: $(basename "$tool") $stop"
