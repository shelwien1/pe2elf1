#!/bin/sh
# mkcorpus.sh -- test/tuning corpus from ../connectome (needs its make + make data):
#   corpus/qb-<seq>.tsv   quantize output with the GRU baseline predictions
#   corpus/qx-<seq>.tsv   quantize output with the true targets in pred_t0/pred_t1,
#                         and the t0/t1 columns dropped from the side columns
set -e
cd "$(dirname "$0")"
C=../connectome
[ -x $C/quantize ] || make -C $C >/dev/null
make -C $C data >/dev/null
mkdir -p corpus
for f in $C/data/seq-*.tsv; do
  n=$(basename $f)
  $C/quantize c $f $C/pred_baseline/$n corpus/qb-$n 2>/dev/null
  awk -F'\t' 'NR==1{for(i=1;i<=NF;i++)c[$i]=i; print "seq_ix\tstep_in_seq\tt0\tt1"; next}
              $c["need_prediction"]==1{print $c["seq_ix"]"\t"$c["step_in_seq"]"\t"$c["t0"]"\t"$c["t1"]}' $f > corpus/targ.tmp
  $C/quantize c $f corpus/targ.tmp corpus/qt.tmp 2>/dev/null
  awk -F'\t' 'NR==1{for(i=1;i<=NF;i++) if($i!~/^t[01]\//) k[i]=1}
              {s="";for(i=1;i<=NF;i++) if(k[i]) s=s (s==""?"":"\t") $i; print s}' corpus/qt.tmp > corpus/qx-$n
done
rm -f corpus/targ.tmp corpus/qt.tmp
ls corpus
