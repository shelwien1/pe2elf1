#!/bin/sh
# t1.sh -- roundtrip test for coder0 (counterpart of t1.bat).
#
#   ./t1.sh                 test ./coder0 on book1 and wcc386
#   ./t1.sh coder0t         test another executable
#   ./t1.sh coder0 f1 f2..  test on the given files
#
# For every file: compress to <file>.1, decompress to <file>.2, compare the
# md5 of the original and the decoded copy, print size and timings, and
# finish with a log.txt-style summary line.  When the other build is also
# present (./coder0t next to ./coder0 or vice versa) the file is compressed
# with it too and the two streams compared: the shipping and the tuning
# build must be byte-identical (IDX-FORMAT.md sec.1, SSE-DESIGN.md sec.4.1).
# Exit status is non-zero if any roundtrip fails or the builds differ.

cd "$(dirname "$0")"

EXE=${1:-./coder0}
case "$EXE" in */*) ;; *) EXE="./$EXE" ;; esac
[ -x "$EXE" ] || { echo "no executable $EXE -- run ./gc.sh first" >&2; exit 2; }
[ $# -gt 0 ] && shift
if [ $# -eq 0 ]; then
  # book1wcc (log.txt's third column) is book1 followed by wcc386: it measures
  # re-adaptation across the file boundary.  Built here, not kept in git.
  [ -f book1wcc ] || cat book1 wcc386 > book1wcc
  set -- book1 wcc386 book1wcc
fi

now() { date +%s.%N; }

# the other build, for the identity check
OTHER=
case "$EXE" in
  ./coder0)  [ -x ./coder0t ] && OTHER=./coder0t ;;
  ./coder0t) [ -x ./coder0 ]  && OTHER=./coder0 ;;
esac

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
  if [ -n "$OTHER" ]; then
    "$OTHER" c "$f" "$f.1o"
    if cmp -s "$f.1" "$f.1o"; then st="$st builds-identical"; else st="$st BUILDS-DIFFER"; fail=1; fi
    rm -f "$f.1o"
  fi
  printf "%-10s %8d -> %8d  c=%6.3fs d=%6.3fs  md5 %s %s\n" "$f" "$(wc -c < "$f")" "$sz" "$tc" "$td" "$m1" "$st"
  summary="$summary$(printf '%-8s %7d %6.3fs  ' "$f" "$sz" "$tc")"
  total=$((total + sz))
  rm -f "$f.2"
done
echo "$summary total $total"
exit $fail
