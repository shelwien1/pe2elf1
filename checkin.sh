#!/bin/sh
# checkin.sh -- one fixed read-only status probe for the optimizer loop.
#
# The 10-minute check-in used to be a different ad-hoc shell string every
# cycle, so it could never match a permission rule and prompted every time.
# Keeping it as one stable command means a single exact-match allowlist
# entry -- Bash(sh checkin.sh) -- silences the whole loop.
#
# Read-only: it encodes to /tmp to prove the binary still runs, and reads
# logs.  It never touches opt/, profiles/, IDX/ or the git tree.
cd "$(dirname "$0")" || exit 1

echo "=== time ==="
date -u
uptime

# The binary is built with -march=native.  After a reclaim onto a different
# CPU every encode dies with SIGILL, and auto_opt2.pl reads that as a size of
# 2147483647 -- which looks like convergence and prints a false "Done".
# A nonzero exit here means rebuild before trusting anything below.
echo "=== sigill check ==="
./xadpcm c opt/a/wavs3 /tmp/checkin.ari >/dev/null 2>&1
echo "exit=$?"

echo "=== climbers ==="
pgrep -cf '^perl auto_opt2\.pl'

echo "=== done ==="
grep -l Done opt/*/opt.log 2>/dev/null || echo none

# The !!! counter does not reset at 133: divide for the sweep, mod for the
# position within it.
echo "=== per-instance ==="
for d in a b c d; do
  n=`awk '/^[0-9A-F]{8} /{last=NR} END{print last}' opt/$d/opt.log 2>/dev/null`
  p=`awk -v s="$n" 'NR>s && /^!!!/{c++} END{print c+0}' opt/$d/opt.log 2>/dev/null`
  b=`tail -1 opt/$d/opttimes.'!!!' 2>/dev/null | tr -d ' \n'`
  echo "$d best=${b:-none} sweep=$((p/133+1)) pos=$((p%133))"
done

echo "=== git ==="
git status --porcelain
