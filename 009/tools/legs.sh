#!/usr/bin/env bash
# legs.sh — run every build `build.sh` documents, and say what each answered.
#
#     tools/legs.sh              every leg
#     tools/legs.sh --fast       the compile-only ones; no test.sh
#
# `build.sh` documents seven builds and `test.sh` runs one of them.  For three
# rounds that gap was not theoretical: `BMF_BITS=32 ./build.sh` did not compile
# at all, because Phase 6 added eight `static_assert`s on struct offsets and
# wrote every number at 64 bits.  Nothing said so, because nothing ran it — and
# `tools/triage.sh` meanwhile told the reader that `test.sh` covered "both
# compiler ratchets", which it never has.
#
# The legs are not a nicety.  Three of the seven have caught something no other
# leg would have:
#
#   BMF_ASSERTS=1    `p2_bump`'s shift range, written from a plan's word as
#                    `>= 2` and wrong: fifteen checks failed the moment the
#                    assert could run.
#   BMF_WARN=1       thirty unused locals left behind by one round's
#                    extractions, invisible to the default build and to
#                    `unused.py`, which reads the spliced copy.
#   BMF_BITS=32      the eight offsets above, and — once it built — the much
#                    stronger statement that all seventeen streams are
#                    byte-identical at both pointer widths.
#
# **This is a once-a-round driver, not a per-commit one.**  Four of the legs run
# `test.sh`, which is minutes apiece; `./build.sh && ./test.sh` is what a commit
# is gated on and this is what a round ends with.  `--fast` runs the four
# compile-only legs, which take seconds and catch the warning and conversion
# regressions — that one is cheap enough to run whenever something moved.
set -u
cd "$(dirname "$0")/.."

fast=0
[ "${1:-}" = --fast ] && fast=1

fails=0
CXX=${CXX:-g++}
say() { printf '%-22s %s\n' "$1" "$2"; }
bad() { say "$1" "$2"; fails=$((fails + 1)); }

# A leg that compiles and counts.  The count is the last line either way, and
# what "right" is differs per leg, so the expected value is an argument.
#
# The exit status is checked as well as the line, and that is not belt and
# braces: the two `-fsyntax-only` legs count warnings, and a *compile error*
# leaves the count intact -- a broken `static_assert` is an `error:` line, which
# no warning grep sees.  Written without this, `--fast` answered "183, at the
# ratchet" for a 32-bit build that did not compile, which is the exact failure
# this file exists to catch, one level down.
count_leg() {                                   # name, expected, env...
  name=$1 want=$2
  shift 2
  out=$(env "$@" ./build.sh 2>&1)
  rc=$?
  got=$(printf '%s\n' "$out" | tail -1)
  if [ "$rc" != 0 ]; then
    bad "$name" "$CXX exited $rc -- $(printf '%s\n' "$out" | grep -m1 ': error: ' || echo 'see the log')"
    return
  fi
  case "$got" in
    "$want"*) say "$name" "$got" ;;
    *)        bad "$name" "$got  (wanted $want)" ;;
  esac
}

# A leg that builds a binary and runs the gate against it.  `test.sh` names its
# own count, and the arena leg's is one lower by design — the out-of-memory
# check cannot fire when the arena is reserved up front.
gate_leg() {                                    # name, expected, env..., -- , build args
  name=$1 want=$2
  shift 2
  envs=()
  while [ $# -gt 0 ] && [ "$1" != -- ]; do envs+=("$1"); shift; done
  [ $# -gt 0 ] && shift
  if ! env "${envs[@]}" ./build.sh "$@" >/dev/null 2>&1; then
    bad "$name" 'did not compile'
    return
  fi
  got=$(./test.sh 2>&1 | tail -1)
  case "$got" in
    "$want"*) say "$name" "$got" ;;
    *)        bad "$name" "$got  (wanted $want)" ;;
  esac
}

count_leg 'BMF_WARN=1'   '0'   BMF_WARN=1
count_leg 'BMF_STRICT=1' '0'   BMF_STRICT=1
count_leg 'BMF_CONV=1'   '196, at the ratchet' BMF_CONV=1
count_leg 'BMF_CONV=1 (32)' '183, at the ratchet' BMF_CONV=1 BMF_BITS=32

if [ "$fast" = 1 ]; then
  env BMF_X= ./build.sh >/dev/null 2>&1 || true
  say 'skipped' 'the four gate legs; drop --fast for those'
else
  gate_leg 'default'          'PASS: 111 checks' BMF_X= --
  gate_leg 'BMF_ASSERTS=1'    'PASS: 111 checks' BMF_ASSERTS=1 --
  gate_leg 'BMF_BITS=32'      'PASS: 111 checks' BMF_BITS=32 --
  gate_leg '-DBMF_HIGH_ARENA' 'PASS: 110 checks' BMF_X= -- -DBMF_HIGH_ARENA
  # Leave the tree holding the binary everything else expects.
  ./build.sh >/dev/null 2>&1
fi

if [ "$fails" != 0 ]; then
  echo "FAILED: $fails of the legs did not answer what they should" >&2
  exit 1
fi
echo "every leg answered what it should"
