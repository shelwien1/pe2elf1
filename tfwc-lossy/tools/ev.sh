#!/bin/bash
# ev.sh DATAFILE w1.tfwc2 w2.tfwc2 ...  -> "compressed_size name" per weights file
# CODER: coder0 built with patches/coder0-costlog.patch (default ./coder0e)
# RUNS:  output directory for .cmp and per-byte .cost logs (default ./runs)
# EVJ:   parallel jobs (default 4)
CODER=${CODER:-./coder0e}; RUNS=${RUNS:-./runs}; mkdir -p "$RUNS"
data=$1; shift; tag=$(basename "$data")
export CODER RUNS data tag
printf "%s\n" "$@" | xargs -P ${EVJ:-4} -I{} bash -c 'w={}; b=$(basename $w .tfwc2); out=$RUNS/$b.$tag; TF_COSTLOG=$out.cost $CODER c $data $out.cmp $w > /dev/null 2>&1; echo "$(stat -c %s $out.cmp) $b"'
