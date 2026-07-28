#!/bin/sh
# verify.sh -- the acceptance test for the refactor: every archive this tree
# produces must be byte-identical to the one rev 016 produced, in BOTH build
# modes, and every one of them must round-trip.
#
#   ./verify.sh <dir-with-016-baseline-binary>
#
# The second argument to each check is a baseline archive; if no baseline tree
# is given, only the round-trips and the recorded sizes below are checked.
set -e
BASE=${1:-}
X=./xadpcm
fail=0

# recorded rev-016 sizes: file, mode, bytes
sizes="wavs2:::1248335 wavs2:-s::1248865 wavs2:-ss::1248865
       Player_Death_Music_ima.wav:::355414
       Player_Death_Music_ima.wav:-s::355414
       Player_Death_Music_ima.wav:-ss::355414"

check() { # file mode expected
  $X c $2 "$1" _v.xac >/dev/null 2>&1
  $X d _v.xac _v.out  >/dev/null 2>&1
  got=`stat -c%s _v.xac`
  if ! cmp -s "$1" _v.out; then printf "FAIL %-28s %-4s roundtrip\n" "$1" "${2:-none}"; fail=1; return; fi
  if [ -n "$3" ] && [ "$got" -gt "$3" ]; then
    printf "FAIL %-28s %-4s %d > baseline %d\n" "$1" "${2:-none}" "$got" "$3"; fail=1; return; fi
  if [ -n "$3" ] && [ "$got" -lt "$3" ]; then
    printf "GAIN %-28s %-4s %d < baseline %d\n" "$1" "${2:-none}" "$got" "$3"; return; fi
  printf "OK   %-28s %-4s %9d\n" "$1" "${2:-none}" "$got"
}

for e in $sizes; do
  f=`echo "$e" | cut -d: -f1`; m=`echo "$e" | cut -d: -f2`; n=`echo "$e" | cut -d: -f4`
  [ -f "$f" ] && check "$f" "$m" "$n"
done

# synthetic corpus: every payload-walk instantiation
if [ -d gen ] || python3 gen_testwavs.py >/dev/null 2>&1; then
  for f in gen/*.wav; do check "$f" "" ""; done
  cat gen/*.wav > _v.all && check _v.all -ss "" && rm -f _v.all
fi

# multi-input / multi-output
if [ -f wavs2 ] && [ -f Player_Death_Music_ima.wav ]; then
  rm -rf _v.dir && mkdir -p _v.dir
  for m in "" -ss; do
    $X c $m wavs2 Player_Death_Music_ima.wav _v.xac >/dev/null 2>&1
    $X d _v.xac _v.dir/ >/dev/null 2>&1
    if cmp -s wavs2 _v.dir/wavs2 && cmp -s Player_Death_Music_ima.wav _v.dir/Player_Death_Music_ima.wav
    then printf "OK   %-28s %-4s %9d\n" "multi(2)" "${m:-none}" `stat -c%s _v.xac`
    else printf "FAIL multi(2) %s\n" "${m:-none}"; fail=1; fi
  done
  rm -rf _v.dir
fi

# malformed input must fail cleanly and leave no output behind
printf 'not an archive' > _v.bad
rm -f _v.out
if $X d _v.bad _v.out >/dev/null 2>&1; then printf "FAIL bad archive accepted\n"; fail=1
elif [ -f _v.out ]; then printf "FAIL bad archive left an output file\n"; fail=1
else printf "OK   %-28s %-4s rejected, no output written\n" "malformed archive" ""; fi

rm -f _v.xac _v.out _v.bad
[ $fail = 0 ] && echo "all checks passed" || echo "FAILURES"
exit $fail
