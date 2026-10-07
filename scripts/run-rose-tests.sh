#!/bin/sh
# Translates ROSE's own compile tests with a ROSE translator and checks that
# the unparsed code compiles, the way the README's status numbers were
# obtained.
#
# usage: scripts/run-rose-tests.sh <CompileTests dir> [translator] [jobs]
#
# <CompileTests dir> is tests/nonsmoke/functional/CompileTests of a ROSE
# checkout (https://github.com/llnl/rose).  Each test file of the C_tests,
# C99_tests, C11_tests, Cxx_tests, Cxx11_tests, Cxx14_tests and Cxx17_tests
# directories that GCC compiles is translated with "translator -c", with the
# -std option ROSE's test harness uses for the directory.  Results are written
# to ./rose-tests/results.txt (PASS, FAIL with the first error, or SKIP when
# GCC does not compile the original), with the logs of failures under
# ./rose-tests/work.
set -u
tests=$(cd "$1" && pwd)
here=$(cd "$(dirname "$0")" && pwd)
translator=${2:-$here/../build/bin/identityTranslator}
translator=$(cd "$(dirname "$translator")" && pwd)/$(basename "$translator")
jobs=${3:-$(nproc 2>/dev/null || echo 1)}
out=$(pwd)/rose-tests
rm -rf "$out"
mkdir -p "$out/work"

ls "$tests"/C_tests/*.c "$tests"/C99_tests/*.c "$tests"/C11_tests/*.c \
   "$tests"/Cxx_tests/*.C \
   "$tests"/Cxx11_tests/*.C "$tests"/Cxx14_tests/*.C "$tests"/Cxx17_tests/*.C 2>/dev/null > "$out/files.txt"

run_one() {
  f=$1
  n=$(basename "$f")
  dir=$(basename "$(dirname "$f")")
  case "$dir" in
    Cxx11_tests) std=-std=c++11 ;;
    Cxx14_tests) std=-std=c++14 ;;
    Cxx17_tests) std=-std=c++17 ;;
    C99_tests) std=-std=c99 ;;
    C11_tests) std=-std=c11 ;;
    *) std= ;;
  esac
  case "$n" in
    *.c) cc="${CC:-gcc} -std=gnu11" ;;
    *) cc="${CXX:-g++} -std=gnu++17" ;;
  esac
  d="$out/work/$dir/$n"
  mkdir -p "$d"
  if ! (cd "$d" && timeout 60 $cc -w -fsyntax-only -I"$(dirname "$f")" "$f" > native.log 2>&1); then
    echo "SKIP $dir/$n"
    rm -rf "$d"
    return
  fi
  (cd "$d" && timeout 300 "$translator" $std -w -I"$(dirname "$f")" -c "$f" > rose.log 2>&1)
  st=$?
  if [ $st -eq 0 ]; then
    echo "PASS $dir/$n"
    rm -rf "$d"
  else
    reason=$(grep -m1 -o "Rose\[FATAL\].*\|Assertion.*\|error: .*\|Internal error.*\|Segmentation.*" "$d/rose.log" | head -1)
    [ -z "$reason" ] && reason="exit $st"
    echo "FAIL $dir/$n :: $reason"
  fi
}

i=0
while [ $i -lt "$jobs" ]; do
  awk -v n="$jobs" -v i="$i" 'NR % n == i' "$out/files.txt" | while read -r f; do run_one "$f"; done > "$out/part.$i" &
  i=$((i + 1))
done
wait
cat "$out"/part.* | sort -k2 > "$out/results.txt"
rm -f "$out"/part.*

for dir in C_tests C99_tests C11_tests Cxx_tests Cxx11_tests Cxx14_tests Cxx17_tests; do
  p=$(grep -c "^PASS $dir/" "$out/results.txt")
  f=$(grep -c "^FAIL $dir/" "$out/results.txt")
  s=$(grep -c "^SKIP $dir/" "$out/results.txt")
  echo "$dir: $p passed, $f failed ($s not compiled by GCC)"
done
