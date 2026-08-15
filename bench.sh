#!/bin/sh
# bench.sh -- best-of-N decode/encode timing, since a single run of a
# 10-second codec on a shared box is worth about +-5%.
#
#   ./bench.sh [binary] [file ...]
set -u
cd "$(dirname "$0")"
E=./bmpc
if [ $# -gt 0 ]; then E=$1; shift; fi
if [ $# -eq 0 ]; then set -- testfiles/x_ep.bmp work/bmp/big_crop.bmp; fi
N=${N:-3}
for f in "$@"; do
  "$E" c "$f" /tmp/bench.c >/dev/null 2>&1
  be=999; bd=999
  for i in $(seq $N); do
    s=$(date +%s.%N); "$E" c "$f" /tmp/bench.c >/dev/null 2>&1; e=$(date +%s.%N)
    be=$(awk -v a=$s -v b=$e -v m=$be 'BEGIN{d=b-a; print (d<m)?d:m}')
    s=$(date +%s.%N); "$E" d /tmp/bench.c /tmp/bench.d >/dev/null 2>&1; e=$(date +%s.%N)
    bd=$(awk -v a=$s -v b=$e -v m=$bd 'BEGIN{d=b-a; print (d<m)?d:m}')
  done
  cmp -s "$f" /tmp/bench.d && ok=OK || ok=FAIL
  awk -v f="$(basename "$f")" -v e="$be" -v d="$bd" -v n="$(wc -c <"$f")" \
      -v c="$(wc -c </tmp/bench.c)" -v k="$ok" 'BEGIN{
    printf "%-16s %s  enc %6.2fs (%5.0f KB/s)  dec %6.2fs (%5.0f KB/s)  %d\n",
           f, k, e, n/1024/e, d, n/1024/d, c }'
done
rm -f /tmp/bench.c /tmp/bench.d
