#!/bin/sh
# perf.sh -- Gate C: has this change made the program slower?
#
#     tools/perf.sh --record          # keep this build as the baseline
#     tools/perf.sh                   # compare this build against it
#     BMF_PERF_RUNS=5 tools/perf.sh   # more repeats, less noise
#
# `test.sh` answers "does it still emit the same bytes".  It cannot see a
# change that emits them a third slower, and two of the refactorings this tree
# is doing are exactly the kind that would: turning nine unrolled selector
# blocks into a loop, and taking the type punning out of the inner counter
# walk.  Both are meant to be codegen-neutral and neither is guaranteed to be.
#
# ## it compares two binaries, not two dates
#
# The obvious design -- record timings in a file, compare a later run against
# them -- was written first and measured.  It does not work.  Running the
# *identical binary* against its own recorded numbers, minutes later, reported
# +1.15%, then +4.52%, then +3.62% geomean on this machine.  That is not the
# program getting slower three times; it is a shared cloud container whose
# available CPU moves more than the thing being measured.  An absolute baseline
# in a file measures the machine, and the machine is not what changed.
#
# So `--record` keeps the *binary*, as `tools/perf-baseline`, and the gate runs
# both of them alternately -- baseline, candidate, baseline, candidate -- one
# image at a time.  Two measurements seconds apart on the same core see the
# same interference, so it cancels.  Same experiment as before, run as a paired
# comparison instead of two independent ones.
#
# `tools/perf-baseline` is deliberately not committed: it is a build of a
# particular commit on a particular machine, which is a local measurement
# artifact and not a source one.  `tools/perf.txt` is committed, and is the
# absolute numbers for the record -- useful for reading a year of history, not
# trustworthy as a gate, and labelled as such.
#
# ## min, not mean
#
# Each measurement is the minimum of `BMF_PERF_RUNS` runs.  Wall time on a
# shared machine is a true cost plus interference, and interference is
# one-sided: it can only make a run slower.  The minimum is the run that got
# the least of it, so it estimates the thing being measured; the mean estimates
# the machine's load.
#
# ## the thresholds
#
# >3% on any single image, >1% on the geometric mean, which is what the
# refactoring plan asks for.  The geomean is the one to believe: it is over
# every image in both directions, so per-image noise averages down, and a real
# regression moves every image the same way.  A single image over 3% with a
# geomean near zero is this script telling you about the machine.
#
# The geometric mean is the right average because these are ratios, and it is
# computed as the mean of the logs so that thirty numbers near 1.0 do not
# multiply into a float that has lost its low bits.
#
# ## what it does not do
#
# It does not build, for the same reason `test.sh` does not: the sweeps need to
# tell a build failure from a gate failure.  And it says nothing about which
# *part* got slower -- `perf record` is the tool for that question, and this
# one is meant to be cheap enough to run after every commit.
set -u
cd "$(dirname "$0")/.."

REC=''
case "${1:-}" in --record) REC=1; shift ;; esac
BIN=${1:-./bmf}
BASE=tools/perf-baseline
TXT=tools/perf.txt
RUNS=${BMF_PERF_RUNS:-4}
[ -x "$BIN" ] || { echo "no $BIN (run ./build.sh)"; echo "FAILED: nothing to measure"; exit 2; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

images=''
for f in testfiles/*.bmp; do
  n=$(basename "$f" .bmp)
  case $n in out_*) continue ;; esac
  images="$images $n"
done

# `date +%s%N` rather than `time`: the shell builtin prints to stderr in a
# format that differs between shells, and this needs one number.
now() { date +%s%N; }

# One run, in microseconds; 0 if the program failed.
once() {
  t0=$(now)
  if "$@" >/dev/null 2>&1; then t1=$(now); echo $(( (t1 - t0) / 1000 )); else echo 0; fi
}

lower() { [ "$2" = 0 ] && echo "$1" && return; [ "$1" = 0 ] || [ "$2" -lt "$1" ] && echo "$2" || echo "$1"; }

if [ -n "$REC" ]; then
  cp "$BIN" "$BASE" || exit 2
  {
    echo "# tools/perf.sh --record: image, encode us, decode us, stream bytes."
    echo "# Minimum of $RUNS runs each, on whatever machine took them.  These"
    echo "# are the record, not the gate: the gate is a paired comparison"
    echo "# against tools/perf-baseline, because absolute numbers taken minutes"
    echo "# apart on this container disagree with themselves by several percent."
    for n in $images; do
      ce=0 cd=0
      i=0
      while [ $i -lt "$RUNS" ]; do
        rm -f "$tmp/$n.bmf" "$tmp/$n.out"
        ce=$(lower "$ce" "$(once "$BIN" c "testfiles/$n.bmp" "$tmp/$n.bmf")")
        cd=$(lower "$cd" "$(once "$BIN" d "$tmp/$n.bmf" "$tmp/$n.out")")
        i=$((i + 1))
      done
      echo "$n $ce $cd $(stat -c%s "$tmp/$n.bmf" 2>/dev/null || echo 0)"
    done
  } > "$TXT"
  echo "-- $BASE is this build; $TXT is its absolute numbers ($RUNS runs each)"
  exit 0
fi

if [ ! -x "$BASE" ]; then
  echo "no $BASE -- run tools/perf.sh --record on the build you want to compare against"
  echo "FAILED: no baseline"
  exit 2
fi

# Baseline and candidate alternately, one image at a time -- and with the order
# within each pair swapped every other run.  Running the candidate second every
# time is not a fair comparison: whichever program goes second finds the input
# already in page cache and the core already at its boost clock, and measuring
# that way reported t24 and t32 decode 6-8% "slower" for a binary compared
# against a byte-identical copy of itself.  Swapping the order cancels it.
: > "$tmp/pairs.txt"
for n in $images; do
  be=0 bd=0 ce=0 cd=0
  i=0
  while [ $i -lt "$RUNS" ]; do
    rm -f "$tmp/b.bmf" "$tmp/b.out" "$tmp/c.bmf" "$tmp/c.out"
    if [ $((i % 2)) = 0 ]; then
      be=$(lower "$be" "$(once "$BASE" c "testfiles/$n.bmp" "$tmp/b.bmf")")
      ce=$(lower "$ce" "$(once "$BIN"  c "testfiles/$n.bmp" "$tmp/c.bmf")")
      bd=$(lower "$bd" "$(once "$BASE" d "$tmp/b.bmf" "$tmp/b.out")")
      cd=$(lower "$cd" "$(once "$BIN"  d "$tmp/c.bmf" "$tmp/c.out")")
    else
      ce=$(lower "$ce" "$(once "$BIN"  c "testfiles/$n.bmp" "$tmp/c.bmf")")
      be=$(lower "$be" "$(once "$BASE" c "testfiles/$n.bmp" "$tmp/b.bmf")")
      cd=$(lower "$cd" "$(once "$BIN"  d "$tmp/c.bmf" "$tmp/c.out")")
      bd=$(lower "$bd" "$(once "$BASE" d "$tmp/b.bmf" "$tmp/b.out")")
    fi
    i=$((i + 1))
  done
  bs=$(stat -c%s "$tmp/b.bmf" 2>/dev/null || echo 0)
  cs=$(stat -c%s "$tmp/c.bmf" 2>/dev/null || echo 0)
  echo "$n $be $ce $bd $cd $bs $cs" >> "$tmp/pairs.txt"
done

python3 - "$tmp/pairs.txt" <<'PY'
import math
import sys

bad, logs = [], []
for line in open(sys.argv[1]):
    name, be, ce, bd, cd, bs, cs = line.split()
    for b, c, what in ((be, ce, 'encode'), (bd, cd, 'decode')):
        b, c = int(b), int(c)
        if b == 0 or c == 0:
            bad.append('%-10s %s did not run (baseline %d, candidate %d)' % (name, what, b, c))
            continue
        r = c / b
        logs.append(math.log(r))
        # Under a millisecond the clock is the measurement: a 3% gate on 400us
        # fires on scheduling jitter, so those count toward the geomean and are
        # not reported on their own.
        if r > 1.03 and b >= 1000:
            bad.append('%-10s %s %+.1f%% (%d -> %d us)' % (name, what, (r - 1) * 100, b, c))
    if bs != cs:
        bad.append('%-10s stream is %s bytes, baseline writes %s -- that is a '
                   'test.sh failure, not a perf one' % (name, cs, bs))

geo = math.exp(sum(logs) / len(logs)) if logs else 1.0
print('geomean over %d paired measurements: %+.2f%%' % (len(logs), (geo - 1) * 100))
if geo > 1.01:
    bad.append('geomean %+.2f%% is over the 1%% budget' % ((geo - 1) * 100))
for line in bad:
    print('  ' + line)
print()
print(('PASS: %d measurements within budget' % len(logs)) if not bad
      else 'FAILED: %d over budget' % len(bad))
sys.exit(1 if bad else 0)
PY
