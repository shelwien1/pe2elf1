#!/bin/sh
# Runs a ROSE translator (by default the identityTranslator built by the
# Makefile) over the test corpus.  Each test is translated (EDG front end ->
# Sage III AST -> unparsed source), the unparsed source is compiled and linked
# by the backend compiler, and the program's output is compared with the
# output of the same program compiled directly.
#
# usage: tests/run-tests.sh [translator] [test files...]
set -u
here=$(cd "$(dirname "$0")" && pwd)
translator=${1:-$here/../build/bin/identityTranslator}
[ $# -gt 0 ] && shift
translator=$(cd "$(dirname "$translator")" && pwd)/$(basename "$translator")
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
  if ! "$cc" $std -w -o "$dir/native" "$src" -lm > "$dir/native.log" 2>&1; then
    echo "SKIP  $name (does not build natively)"
    continue
  fi
  "$dir/native" > "$dir/native.out" 2>&1
  native_status=$?
  if ! (cd "$dir" && "$translator" $std -w "$src" -o "$dir/rose" -lm > "$dir/rose.log" 2>&1); then
    echo "FAIL  $name (translation or backend compilation)"
    sed 's/^/      /' "$dir/rose.log" | tail -15
    fail=$((fail + 1))
    failed="$failed $name"
    continue
  fi
  "$dir/rose" > "$dir/rose.out" 2>&1
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
