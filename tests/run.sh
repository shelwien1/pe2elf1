#!/bin/sh
# tests/run.sh - build everything and run the full test suite.
#
#   sh tests/run.sh [sample.mp2 ...]
#
# Any .mp2 files given on the command line are additionally round-tripped.

set -e
cd "$(dirname "$0")/.."

echo "== building =="
sh g2.sh
g++ -O2 -std=gnu++17 -fno-exceptions -fno-rtti -I. -o tests/mkmp2  tests/mkmp2.cpp
g++ -O2 -std=gnu++17 -fno-exceptions -fno-rtti -I. -o tests/mp3chk tests/mp3chk.cpp

echo
echo "== configuration sweep =="
python3 tests/sweep.py "$@" 2>/dev/null || python3 tests/sweep.py

fail=0
for f in "$@"; do
  case "$f" in
    --*) continue ;;
  esac
  [ -f "$f" ] || continue
  echo
  echo "== round trip: $f =="
  tmp="${TMPDIR:-/tmp}/mp2rt.$$"
  ./mp2 c "$f" "$tmp.mp3" "$tmp.meta"
  ./tests/mp3chk "$tmp.mp3"
  ./mp2 d "$tmp.mp3" "$tmp.meta" "$tmp.mp2"
  if cmp "$f" "$tmp.mp2"; then
    echo "IDENTICAL"
  else
    echo "MISMATCH"
    fail=1
  fi
  rm -f "$tmp.mp3" "$tmp.meta" "$tmp.mp2"
done

echo
[ $fail -eq 0 ] && echo "all tests passed"
exit $fail
