#!/bin/sh
# Tests of the refactoring tools (rose-ren, rose-using, rose-m2g).
#
# usage: tests/tools/run-tests.sh [--update] <bin dir> [test...]
#
# Each test directory has
#   input/     the source files
#   run        the commands, run in a copy of input/ ($REN, $USING and $M2G are the tools; a
#              command that starts with "! " must fail)
#   program    the programs to build: a source file and compiler options per line
#   expected/  the files after the commands (--update writes them)
# The programs are built ($CXX, default g++) and run before and after the commands, and must
# print the same; a program that does not compile before (as for rose-using) must compile
# after.  Windows tools (bin/*.exe) are run with $RUN (default: wine).
#
# The tests whose directory has a file "msvc" use the headers of Visual C++ ($MSVC in the run
# file is --msvc=$MSVC_DIR): they run only when MSVC_DIR is the folder of a portable Visual C++
# (with include, ucrt/include and sdk/include).
set -u
update=false
if [ "${1:-}" = "--update" ]; then
  update=true
  shift
fi
here=$(cd "$(dirname "$0")" && pwd)
bin=$(cd "$1" && pwd)
shift
exe=
run=
if [ -f "$bin/rose-ren.exe" ]; then
  exe=.exe
  run=${RUN:-wine}
fi
REN="$run $bin/rose-ren$exe"
USING="$run $bin/rose-using$exe"
M2G="$run $bin/rose-m2g$exe"
cxx=${CXX:-g++}
MSVC=
if [ -n "${MSVC_DIR:-}" ]; then
  if [ -n "$exe" ]; then
    MSVC="--msvc=$(winepath -w "$MSVC_DIR" 2>/dev/null)"
  else
    MSVC="--msvc=$MSVC_DIR"
  fi
fi
if [ $# -gt 0 ]; then
  tests="$*"
else
  tests=$(cd "$here" && for d in */; do echo "${d%/}"; done)
fi
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM

# Builds and runs the programs of a test, writing the output to <file>.<suffix>.out
programs() {
  t=$1
  d=$2
  suffix=$3
  while read -r prog opts; do
    [ -n "$prog" ] || continue
    if $cxx $opts -w -o "$d/$prog.$suffix" "$d/$prog" > "$d/$prog.$suffix.log" 2>&1; then
      (cd "$d" && "./$prog.$suffix") > "$d/$prog.$suffix.out" 2>&1 || echo "exit status $?" >> "$d/$prog.$suffix.out"
      rm -f "$d/$prog.$suffix"
    fi
  done < "$here/$t/program"
}

pass=0
fail=0
skip=0
for t in $tests; do
  if [ -f "$here/$t/msvc" ] && [ -z "$MSVC" ]; then
    echo "SKIP  $t (MSVC_DIR is not set)"
    skip=$((skip + 1))
    continue
  fi
  d=$work/$t
  mkdir -p "$d"
  cp -R "$here/$t/input/." "$d/"
  log=$work/$t.log
  : > "$log"
  problem=
  programs "$t" "$d" before
  while IFS= read -r line; do
    case "$line" in
      '' | '#'*) continue ;;
    esac
    expect=0
    case "$line" in
      '! '*)
        expect=1
        line=${line#! }
        ;;
    esac
    echo "\$ $line" >> "$log"
    (cd "$d" && eval "$line") >> "$log" 2>&1
    status=$?
    if [ $expect = 0 ] && [ $status != 0 ]; then
      problem="failed: $line"
      break
    fi
    if [ $expect = 1 ] && [ $status = 0 ]; then
      problem="did not fail: $line"
      break
    fi
  done < "$here/$t/run"
  # The files
  if [ -z "$problem" ]; then
    if $update; then
      rm -rf "$here/$t/expected"
      mkdir -p "$here/$t/expected"
      (cd "$here/$t/input" && for f in *; do cp "$d/$f" "$here/$t/expected/$f"; done)
    fi
    for f in $(cd "$here/$t/expected" && ls); do
      if ! cmp -s "$here/$t/expected/$f" "$d/$f"; then
        problem="$f differs from expected/$f"
        diff "$here/$t/expected/$f" "$d/$f" | sed 's/^/      /' | head -20 >> "$log"
        break
      fi
    done
  fi
  # The programs
  if [ -z "$problem" ]; then
    programs "$t" "$d" after
    while read -r prog opts; do
      [ -n "$prog" ] || continue
      if [ ! -f "$d/$prog.after.out" ]; then
        problem="$prog does not compile after the changes"
        sed 's/^/      /' "$d/$prog.after.log" | head -20 >> "$log"
        break
      fi
      if [ -f "$d/$prog.before.out" ] && ! cmp -s "$d/$prog.before.out" "$d/$prog.after.out"; then
        problem="$prog prints something else after the changes"
        break
      fi
    done < "$here/$t/program"
  fi
  if [ -z "$problem" ]; then
    echo "PASS  $t"
    pass=$((pass + 1))
  else
    echo "FAIL  $t ($problem)"
    sed 's/^/      /' "$log" | tail -30
    fail=$((fail + 1))
  fi
done
echo "$pass passed, $fail failed$([ $skip = 0 ] || echo ", $skip skipped")"
[ $fail = 0 ]
