#!/usr/bin/env bash
# x64.sh — what the 64-bit build still gets wrong.
#
#     tools/x64.sh
#
# `test.sh` is the project's oracle and it runs one target.  For six rounds
# that was the only target there was, and "fifteen streams byte-identical" was
# the whole claim; the moment a second target exists the claim has a gap in it
# the size of every pointer in the file.
#
# So this asks `test.sh`'s question of `-m64`: compress each image and compare
# the stream against the *32-bit* reference, then expand it and compare against
# the input.  Byte-identical streams across two pointer widths is a much
# stronger statement than either build alone -- it says the arithmetic that
# reaches the range coder does not depend on how wide an address is, which is
# exactly what a decompilation of 1997 x86 has no reason to guarantee and every
# reason to violate.
#
# It also prints what `strict64.log` still holds, because a run that crashes
# and a run that is wrong look the same from here and the truncation count is
# what says which is likelier.  See tools/ptrwidth.py for the four kinds.
#
# Not in `test.sh`: the 64-bit build is a target and not yet a guarantee, and a
# gate that fails on the work still to do stops being a gate.  When this run
# is clean it belongs in test.sh, and that is the point at which this goal is
# finished.
set -u
set +m
cd "$(dirname "$0")/.."

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if ! BMF_OUT="$tmp/bmf64" BMF_BITS=64 ./build.sh >"$tmp/build" 2>&1; then
  echo "the 64-bit build failed:" >&2
  grep -m5 'error:' "$tmp/build" >&2
  exit 2
fi

# A signal is not an exit code, and "exits 139" is a worse answer than the
# name of the signal -- SIGSEGV and SIGFPE are different defects and this loop
# sees both.  Anything under 128 is the program's own table (see __exit_402E40).
why() {
  case $1 in
    124)      echo "timed out" ;;
    13[4-9]|1[4-9][0-9]) echo "died on SIG$(kill -l $(($1 - 128)) 2>/dev/null || echo "?$1")" ;;
    *)        echo "exits $1" ;;
  esac
}

same=0 ran=0 bad=0
for f in testfiles/*.bmp; do
  n=$(basename "$f" .bmp)
  ran=$((ran + 1))
  # The reference stream is testfiles/$n.bmf when test.sh has one; without it
  # there is nothing to compare against and the round trip is all this can say.
  # `$?` after `if ! cmd` is the negation's status and is always 0, which is
  # how the first version of this reported "compress exits 0" under a line of
  # SIGSEGV.  Run it, then read the code.
  timeout 120 "$tmp/bmf64" c "$f" "$tmp/$n.bmf" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    printf '%-9s compress %s\n' "$n" "$(why $rc)"
    bad=$((bad + 1)); continue
  fi
  timeout 120 "$tmp/bmf64" d "$tmp/$n.bmf" "$tmp/$n.out" >/dev/null 2>&1
  rc=$?
  if [ "$rc" != 0 ]; then
    printf '%-9s expand %s\n' "$n" "$(why $rc)"
    bad=$((bad + 1)); continue
  fi
  if ! cmp -s "$tmp/$n.out" "$f"; then
    printf '%-9s round trip differs from the input\n' "$n"
    bad=$((bad + 1)); continue
  fi
  if [ -f "testfiles/$n.bmf" ] && ! cmp -s "$tmp/$n.bmf" "testfiles/$n.bmf"; then
    printf '%-9s round trips, but the stream is not the 32-bit one\n' "$n"
    bad=$((bad + 1)); continue
  fi
  same=$((same + 1))
done

BMF_STRICT=1 BMF_BITS=64 ./build.sh >/dev/null 2>&1
left=$(python3 tools/ptrwidth.py subs1.hpp | tail -1)

echo
echo "$same of $ran images agree with the 32-bit build byte for byte"
echo "$left"
exit $((bad ? 1 : 0))
