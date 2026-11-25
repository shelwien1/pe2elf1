#!/bin/bash
# Corpus harness: for each input file, pdfdump c -> pdfdump d -> cmp.
# Writes a CSV (stdout) with per-channel sizes, file counts and timings; failures go to stderr.
#
#   tests/roundtrip.sh [-b pdfdump] [-o workdir] [-x "extra c options"] files...
#
# Exit status is non-zero if any file fails to round-trip.

PDFDUMP=./pdfdump
WORK=${TMPDIR:-/tmp}/pdfdump-rt.$$
EXTRA=""
while getopts "b:o:x:" o; do
  case $o in
    b) PDFDUMP=$OPTARG ;;
    o) WORK=$OPTARG ;;
    x) EXTRA=$OPTARG ;;
    *) exit 2 ;;
  esac
done
shift $((OPTIND - 1))
mkdir -p "$WORK"

fsize() { if [ -f "$1" ]; then stat -c %s "$1"; else echo 0; fi; }

echo "file,size,status,meta,txt,ops,fmt,rec,bin,cs,cmap,own_files,own_bytes,stored,c_sec,d_sec"
fail=0
n=0
for f in "$@"; do
  n=$((n + 1))
  p="$WORK/d$n"
  rm -f "$p".* "$p"[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].* "$p"b[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].*
  t0=$(date +%s.%N)
  if ! $PDFDUMP c $EXTRA "$f" "$p" 2> "$p.err"; then
    echo "\"$f\",$(fsize "$f"),c-error,,,,,,,,,,,,," ; echo "c failed: $f: $(cat "$p.err")" >&2; fail=$((fail + 1)); continue
  fi
  t1=$(date +%s.%N)
  # the encoder's own verify failing is a bug even though stored mode keeps the round trip exact
  if grep -q "internal error" "$p.err"; then
    echo "verify failed in c: $f: $(head -c 300 "$p.err")" >&2; fail=$((fail + 1))
  fi
  if ! $PDFDUMP d "$p" "$p.out" 2>> "$p.err"; then
    echo "\"$f\",$(fsize "$f"),d-error,,,,,,,,,,,,,"; echo "d failed: $f: $(cat "$p.err")" >&2; fail=$((fail + 1)); continue
  fi
  t2=$(date +%s.%N)
  status=ok
  if ! cmp -s "$f" "$p.out"; then status=MISMATCH; fail=$((fail + 1)); echo "mismatch: $f" >&2; fi
  own=$(ls "$p"[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].* 2>/dev/null | wc -l)
  ownb=$(cat "$p"[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].* 2>/dev/null | wc -c)
  stored=$(grep -c "(stored:" "$p.err")
  echo "\"$f\",$(fsize "$f"),$status,$(fsize "$p.meta"),$(fsize "$p.txt"),$(fsize "$p.ops"),$(fsize "$p.fmt"),$(fsize "$p.rec"),$(fsize "$p.bin"),$(fsize "$p.cs"),$(fsize "$p.cmap"),$own,$ownb,$stored,$(echo "$t1 - $t0" | bc),$(echo "$t2 - $t1" | bc)"
  rm -f "$p".* "$p"[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].* "$p"b[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].*
done
echo "round trip: $((n - fail))/$n ok" >&2
[ $fail -eq 0 ]
