#!/bin/sh
# resume_opt.sh -- restart the four climbers in an EXISTING opt/ tree.
#
# run_opt.sh wipes opt/ and starts from scratch; this one does not, because the
# container running this session is reclaimed whenever the session goes idle and
# takes every background process with it.  What survives is the on-disk state --
# each instance's opttimes.!!! and export.!!!  -- so a resume has to rebuild the
# instance binary from that rather than from whatever byte pattern the kill
# happened to interrupt.
#
# auto_opt2.pl writes its best set back into the exe only at the END of each
# descriptor, so a mid-descriptor kill leaves a randomly flipped map in the file
# and its own initial run would adopt that as the baseline.  Hence: every
# instance gets a fresh copy of the pristine build, and an instance that has an
# export.!!! gets that folded back in with apply_export.pl.  An instance without
# one never beat the baseline, so the pristine build IS its best state.
set -e
ROOT=`pwd`
WORK=${1:-opt}
BASE=${2:-xadpcm}
[ -d "$WORK" ] || { echo "resume_opt.sh: no $WORK -- use run_opt.sh for a fresh start" >&2; exit 1; }

if pgrep -f '^perl auto_opt2\.pl' >/dev/null; then
  echo "resume_opt.sh: climbers already running, nothing to do" >&2
  exit 1
fi

for d in a b c d; do
  cp -f "$BASE" "$WORK/$d/xadpcm"
  if [ -s "$WORK/$d/export.!!!" ]; then
    perl "$ROOT/apply_export.pl" "$WORK/$d/xadpcm" "$WORK/$d/export.!!!"
  else
    echo "apply_export: $WORK/$d/xadpcm <- pristine (no export yet)"
  fi
  # book1bwt.ari is the climber's scratch output; a stale one from the killed run
  # would be measured before the first encode replaces it
  rm -f "$WORK/$d/book1bwt.ari"
done

for d in a b c d; do
  ( cd "$WORK/$d" && setsid nohup perl auto_opt2.pl >> opt.log 2>&1 < /dev/null & )
done
sleep 2
echo "resumed:"
ps -eo pid,etime,args | grep '[a]uto_opt2.pl'

# NOTE on launching these under the Claude Code harness rather than from a shell:
# use one background Bash task per instance, and keep the pipeline
#
#   cd opt/<d> && exec perl auto_opt2.pl 2>&1 | tee -a opt.log | grep --line-buffered -E '^(!!!|===|Done)'
#
# The tee/grep half is not cosmetic.  Redirecting straight to opt.log
# (`>> opt.log 2>&1`) leaves the harness task's own output stream empty, so the
# task list shows a running climber with no output for as long as it runs, which
# is indistinguishable there from one that died.  Sending the descriptor headers,
# the `=== <size>` improvement markers and the final `Done` to stdout keeps the
# task list truthful while opt.log still gets every trial line.
