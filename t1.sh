#!/bin/sh
# t1.sh -- roundtrip test for coder0 (counterpart of t1.bat).
#
#   ./t1.sh                 test ./coder0 on book1 and wcc386
#   ./t1.sh coder0t         test another executable
#   ./t1.sh coder0 f1 f2..  test on the given files
#
# For every file: compress to <file>.1, decompress to <file>.2, compare the
# md5 of the original and the decoded copy, print size and timings, and
# finish with a log.txt-style summary line.  Exit status is non-zero if any
# roundtrip fails.

cd "$(dirname "$0")"

EXE=${1:-./coder0}
case "$EXE" in */*) ;; *) EXE="./$EXE" ;; esac
[ -x "$EXE" ] || { echo "no executable $EXE -- run ./gc.sh first" >&2; exit 2; }
[ $# -gt 0 ] && shift
[ $# -gt 0 ] || set -- book1 wcc386

now() { date +%s.%N; }

fail=0
summary=""
total=0
for f in "$@"; do
  rm -f "$f.1" "$f.2"
  t0=$(now); "$EXE" c "$f" "$f.1"; t1=$(now)
  "$EXE" d "$f.1" "$f.2"; t2=$(now)
  m1=$(md5sum < "$f" | cut -c1-32)
  m2=$(md5sum < "$f.2" | cut -c1-32)
  sz=$(wc -c < "$f.1")
  tc=$(echo "$t1 - $t0" | bc); td=$(echo "$t2 - $t1" | bc)
  if [ "$m1" = "$m2" ]; then st=OK; else st=FAIL; fail=1; fi
  printf "%-10s %8d -> %8d  c=%6.3fs d=%6.3fs  md5 %s %s\n" "$f" "$(wc -c < "$f")" "$sz" "$tc" "$td" "$m1" "$st"
  summary="$summary$(printf '%-8s %7d %6.3fs  ' "$f" "$sz" "$tc")"
  total=$((total + sz))
  rm -f "$f.2"
done
echo "$summary total $total"
exit $fail
