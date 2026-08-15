#!/bin/sh
# Linux test script -- port of t.bat.
#
# t.bat:   coder0 c book1 1 ; coder0 d 1 2 ; md5sum 2 book1 ; print size of 1
# Here the same thing, plus an explicit PASS/FAIL on the md5 compare, timings,
# and an optional file list so the roundtrip can be run on anything.
#
# Usage:  ./t.sh [file ...]        (default: book1)
#         CODER=./bmpc ./t.sh testfiles/t24.bmp
#         CODER=./bmpc ./t.sh testfiles/*.bmp

cd "$(dirname "$0")" || exit 1

CODER=${CODER:-./coder0}
[ $# -eq 0 ] && set -- book1

now() { date +%s.%N; }

rc=0
tot_o=0; tot_c=0
for f in "$@"; do
  rm -f 1 2
  t0=$(now); "$CODER" c "$f" 1 || { echo "FAIL encode $f"; rc=1; continue; }
  t1=$(now); "$CODER" d 1 2    || { echo "FAIL decode $f"; rc=1; continue; }
  t2=$(now)

  orig=$(wc -c < "$f")
  comp=$(wc -c < 1)
  a=$(md5sum < "$f" | cut -d' ' -f1)
  b=$(md5sum < 2    | cut -d' ' -f1)

  if [ "$a" = "$b" ]; then st="OK  "; else st="FAIL"; rc=1; fi
  tot_o=$((tot_o+orig)); tot_c=$((tot_c+comp))
  awk -v st="$st" -v f="$f" -v o="$orig" -v c="$comp" \
      -v t0="$t0" -v t1="$t1" -v t2="$t2" 'BEGIN{
    printf "%s %-28s %9d -> %9d  %6.4f bpc  enc %5.2fs dec %5.2fs\n",
           st, f, o, c, c*8.0/o, t1-t0, t2-t1 }'
done

if [ $# -gt 1 ]; then
  awk -v o="$tot_o" -v c="$tot_c" 'BEGIN{
    printf "     %-28s %9d -> %9d  %6.4f bpc\n", "TOTAL", o, c, c*8.0/o }'
fi
rm -f 1 2
exit $rc
