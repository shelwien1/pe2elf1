#!/bin/sh
# t.sh -- round-trip + size check over every wav given (or over the two test
# files in the tree).  Every solid mode, plus a multi-input / multi-output run,
# plus the in-memory self-test.  Prints the coded size of each run so a
# regression against a recorded baseline is visible without a second tool.
set -e
[ $# -gt 0 ] && set -- "$@" || set -- wavs2 Player_Death_Music_ima.wav
X=./xadpcm
fail=0

for f in "$@"; do
  for m in "" "-s" "-ss"; do
    $X c $m "$f" _t.xac >/dev/null 2>&1
    $X d _t.xac _t.out  >/dev/null 2>&1
    if cmp -s "$f" _t.out; then
      printf "OK   %-30s %-4s %9d -> %9d\n" "$f" "${m:-none}" `stat -c%s "$f"` `stat -c%s _t.xac`
    else
      printf "FAIL %-30s %-4s roundtrip mismatch\n" "$f" "${m:-none}"; fail=1
    fi
  done
  $X c -t "$f" _t.xac >/dev/null 2>&1 && printf "OK   %-30s -t   self-test passed\n" "$f" \
                                      || { printf "FAIL %-30s -t\n" "$f"; fail=1; }
done

# several inputs in, a directory out -- exercises the multi-file pump on pin0
# and the split sink on pin1, and (with -ss) a solid run across two formats
if [ $# -gt 1 ]; then
  rm -rf _t.dir && mkdir -p _t.dir
  for m in "" "-ss"; do
    $X c $m "$@" _t.xac >/dev/null 2>&1
    $X d _t.xac _t.dir/ >/dev/null 2>&1
    ok=1
    for f in "$@"; do
      b=`basename "$f"`
      cmp -s "$f" "_t.dir/$b" || ok=0
    done
    if [ $ok = 1 ]; then
      printf "OK   %-30s %-4s %9d\n" "multi($#)" "${m:-none}" `stat -c%s _t.xac`
    else
      printf "FAIL %-30s %-4s multi roundtrip\n" "multi($#)" "${m:-none}"; fail=1
    fi
  done
  rm -rf _t.dir
fi

rm -f _t.xac _t.out
exit $fail
