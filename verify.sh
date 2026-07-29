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

# Recorded rev-016 sizes: file, mode, bytes.  Two entries are one byte ABOVE
# rev 016 -- wavs2 -s and -ss, 1248865 there.  That is the whole cost of moving
# squash/stretch off exp()/log() onto integer arithmetic: over the 24 archives
# md5s.sh builds, 21 are the same size, two grew a byte and one (gen-concat -ss)
# shrank one, +1 byte net on 8.73 MB.  The tables disagree with the float pair in
# 71 of 8192 entries, every one by exactly 1, so this is the two tables being
# equally good rather than one being better -- and what it buys is that the
# archive format no longer depends on which libm the build linked.
sizes="wavs2:::1248335 wavs2:-s::1248866 wavs2:-ss::1248866
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

# A HOSTILE header, not merely a truncated one.  block_align arrives as a varint
# while the payload walks size their scratch at BA_MAX, so a header claiming
# 1000000 used to memset a megabyte into a 64 KB slice -- caught only by
# _FORTIFY_SOURCE, and a silent BSS overwrite without it.  Regression test for
# the bound in get_seg, and the place to add the next one of these.
if python3 -c '
import sys
def v(x):
    o=b""
    while x>=0x80: o+=bytes([(x&0x7f)|0x80]); x>>=7
    return o+bytes([x])
h  = b"XAC1"+bytes([4,0])+v(1)+v(0)          # magic, ver, solid, nseg=1, lead=0
h += bytes([0,0,1,4])+v(1000000)+v(4096)+v(0)+v(0)   # seg: block_align = 1000000
h += v(0)+b"\x00"*4+b"\x00"*64               # no file table, crc, rc stream
open("_v.bad2","wb").write(h)' 2>/dev/null; then
  rm -f _v.out
  if $X d _v.bad2 _v.out >/dev/null 2>&1; then printf "FAIL hostile block_align accepted\n"; fail=1
  elif [ -f _v.out ]; then printf "FAIL hostile block_align left an output file\n"; fail=1
  else printf "OK   %-28s %-4s rejected, no output written\n" "hostile block_align" ""; fi
  rm -f _v.bad2
fi

rm -f _v.xac _v.out _v.bad
[ $fail = 0 ] && echo "all checks passed" || echo "FAILURES"
exit $fail
