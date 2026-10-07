#!/bin/sh
# Runs a ROSE translator (by default the identityTranslator built by the
# Makefile) over the test corpus.  Each test is translated (EDG front end ->
# Sage III AST -> unparsed source), the unparsed source is compiled and linked
# by the backend compiler, and the program's output is compared with the
# output of the same program compiled directly (with $CC or $CXX).
#
# usage: tests/run-tests.sh [translator] [test files...]
#
# A Windows translator (*.exe) and the Windows programs are run with $RUN
# (default: wine).  The translator runs gcc or g++ from Wine's PATH as backend
# compiler (e.g. WINEPATH='Z:\path\to\msys64\mingw64\bin'), and $CC and $CXX
# should be MinGW-w64 compilers too (e.g. x86_64-w64-mingw32-gcc).
set -u
here=$(cd "$(dirname "$0")" && pwd)
translator=${1:-$here/../build/bin/identityTranslator}
[ $# -gt 0 ] && shift
translator=$(cd "$(dirname "$translator")" && pwd)/$(basename "$translator")
# The reference programs are linked statically on Windows, so that they do not pick up the
# run-time DLLs of the backend compiler from the PATH.
case "$translator" in
  *.exe) exe=.exe; run=${RUN:-wine}; native_flags=-static ;;
  *) exe=; run=${RUN:-}; native_flags= ;;
esac
if [ $# -gt 0 ]; then
  tests="$*"
else
  tests=$(ls "$here"/c/*.c "$here"/cxx/*.cpp 2>/dev/null)
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
pass=0
fail=0
failed=""

for src in $tests; do
  src=$(cd "$(dirname "$src")" && pwd)/$(basename "$src")
  name=$(basename "$src")
  case "$name" in
    *.c) cc=${CC:-gcc}; std=-std=gnu11 ;;
    *) cc=${CXX:-g++}; std=-std=gnu++17 ;;
  esac
  dir=$work/$name
  mkdir -p "$dir"
  if ! $cc $std -w $native_flags -o "$dir/native$exe" "$src" -lm > "$dir/native.log" 2>&1; then
    echo "SKIP  $name (does not build natively)"
    continue
  fi
  $run "$dir/native$exe" > "$dir/native.out" 2>&1
  native_status=$?
  if ! (cd "$dir" && $run "$translator" $std -w "$src" -o "$dir/rose$exe" -lm > "$dir/rose.log" 2>&1); then
    echo "FAIL  $name (translation or backend compilation)"
    sed 's/^/      /' "$dir/rose.log" | tail -15
    fail=$((fail + 1))
    failed="$failed $name"
    continue
  fi
  $run "$dir/rose$exe" > "$dir/rose.out" 2>&1
  rose_status=$?
  if [ $native_status -ne $rose_status ] || ! cmp -s "$dir/native.out" "$dir/rose.out"; then
    echo "FAIL  $name (different behavior: exit $native_status vs $rose_status)"
    diff "$dir/native.out" "$dir/rose.out" | sed 's/^/      /' | head -15
    fail=$((fail + 1))
    failed="$failed $name"
    continue
  fi
  echo "PASS  $name"
  pass=$((pass + 1))
done

echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
