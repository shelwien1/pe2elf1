#!/bin/sh
# mkcorpus.sh -- test/tuning corpus from ../connectome (built, data unpacked by
# this script): quantize output for each of the 10 sequences with the targets
# t0/t1 and three kinds of predictions in pred_t0/pred_t1:
#   corpus/pb-<seq>.tsv   the GRU baseline (connectome/pred_baseline)
#   corpus/pd-<seq>.tsv   the dummy predictor (connectome/predict -m0)
#   corpus/pz-<seq>.tsv   zeros: the targets given the features alone
set -e
cd "$(dirname "$0")"
C=../connectome
make -C $C >/dev/null
make -C $C data >/dev/null
mkdir -p corpus
for f in $C/data/seq-*.tsv; do
  n=$(basename $f)
  $C/quantize c $f $C/pred_baseline/$n corpus/pb-$n 2>/dev/null
  $C/predict $f corpus/dummy.tmp 2>/dev/null
  $C/quantize c $f corpus/dummy.tmp corpus/pd-$n 2>/dev/null
  awk -F'\t' 'NR==1{print; next} {print $1"\t"$2"\t0\t0"}' corpus/dummy.tmp > corpus/zero.tmp
  $C/quantize c $f corpus/zero.tmp corpus/pz-$n 2>/dev/null
done
rm -f corpus/*.tmp
ls corpus | wc -l
