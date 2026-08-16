#!/bin/sh
# mrpsweep.sh -- build mrpc with -D overrides and total the crop corpus.
S=${TMPDIR:-/tmp}/mrpsweep; mkdir -p $S
CORPUS=${CORPUS:-"work/mrpc/c24.bmp work/mrpc/c32.bmp work/mrpc/cep.bmp"}
for cfg in "$@"; do
  opts=""; for t in $cfg; do opts="$opts -D$t"; done
  GCOUT=$S/m GCOPTS="$opts" ./gc.sh mrpc >/dev/null 2>&1 || { echo "BUILD FAIL: $cfg"; continue; }
  tot=0; ok=OK; t0=$(date +%s)
  for f in $CORPUS; do
    $S/m c $f $S/o >/dev/null 2>&1; $S/m d $S/o $S/d >/dev/null 2>&1
    cmp -s $f $S/d || ok=FAIL
    tot=$((tot + $(stat -c%s $S/o)))
  done
  t1=$(date +%s)
  printf '%9d  %s  %3ds  %s\n' $tot $ok $((t1-t0)) "$cfg"
done
