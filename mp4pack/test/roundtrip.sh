#!/bin/bash
# Round trips: roundtrip.sh BINDIR OUTDIR "OPTIONS" FILE...
# BINDIR holds mp4pack.exe and the tools (run under wine), or the Linux
# mp4pack and the tools converted by pe2elf (run as they are); each FILE is
# compressed with OPTIONS, decompressed (default options) and compared.
# Prints one line per file.
export WINEDEBUG=-all
bin=$1; out=$2; opts=$3; shift 3
if [ -f "$bin/mp4pack.exe" ]; then run=(wine "$bin/mp4pack.exe"); else run=("$bin/mp4pack"); fi
mkdir -p "$out"
fail=0
for f in "$@"; do
  b=$(basename "$f")
  a="$out/$b.mpk"; r="$out/$b.out"
  rm -f "$a" "$r"
  "${run[@]}" c $opts "$f" "$a" > "$out/$b.c.log" 2>&1; rc=$?
  "${run[@]}" d "$a" "$r" > "$out/$b.d.log" 2>&1; rd=$?
  if [ $rc = 0 ] && [ $rd = 0 ] && cmp -s "$f" "$r"; then
    echo "ok   $b: $(stat -c %s "$f") -> $(stat -c %s "$a")"
  else
    echo "FAIL $b: c=$rc d=$rd"; fail=1
    tail -n 5 "$out/$b.c.log" "$out/$b.d.log"
  fi
  rm -f "$r"
done
exit $fail
