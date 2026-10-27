#!/bin/bash
# Round trips under wine: roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...
# BINDIR holds mp4pack.exe and the tools; each FILE is compressed with OPTIONS,
# decompressed (default options) and compared. Prints one line per file.
export WINEDEBUG=-all
bin=$1; out=$2; opts=$3; shift 3
mkdir -p "$out"
fail=0
for f in "$@"; do
  b=$(basename "$f")
  a="$out/$b.mpk"; r="$out/$b.out"
  rm -f "$a" "$r"
  wine "$bin/mp4pack.exe" c $opts "$f" "$a" > "$out/$b.c.log" 2>&1; rc=$?
  wine "$bin/mp4pack.exe" d "$a" "$r" > "$out/$b.d.log" 2>&1; rd=$?
  if [ $rc = 0 ] && [ $rd = 0 ] && cmp -s "$f" "$r"; then
    echo "ok   $b: $(stat -c %s "$f") -> $(stat -c %s "$a")"
  else
    echo "FAIL $b: c=$rc d=$rd"; fail=1
    tail -n 5 "$out/$b.c.log" "$out/$b.d.log"
  fi
  rm -f "$r"
done
exit $fail
