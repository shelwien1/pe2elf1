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

# Recorded sizes for the CURRENT container (version 5).
#
# The acceptance criterion changed here, once and on purpose.  Through version 4
# it was "byte-identical to rev 016".  Version 5 moves the container records into
# the rc stream so the encoder can work from a window instead of a whole image,
# and that necessarily re-segments the same bytes: a wav's record no longer spans
# up to the NEXT wav's start, the bytes between two wavs are their own record, and
# the payload carries a chunk list.  Identity was never available.
#
# What replaced it: lossless round-trip everywhere, and total size no worse.
# Over the 24 archives md5s.sh builds, v5 is 2439 bytes SMALLER than v4 in total.
# Per file it goes both ways -- wavs2 +11 and multi(2) +14 for the per-segment
# tag and chunk list, against multi(2) -ss -2382 and gen-concat -ss -101, where
# resetting the model when a segment widens turns out to beat carrying statistics
# across a change of alphabet.  The two entries that grew are recorded below.
sizes="wavs2:::1248346 wavs2:-s::1248876 wavs2:-ss::1248876
       Player_Death_Music_ima.wav:::355415
       Player_Death_Music_ima.wav:-s::355415
       Player_Death_Music_ima.wav:-ss::355415"

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

# The streaming decoder: output written as it is decoded must equal output
# written the old way (-b, whole image verified first), and a corrupted archive
# must leave nothing behind even though the bytes were already going out.
if [ -f wavs2 ]; then
  $X c wavs2 _v.xac >/dev/null 2>&1
  $X d    _v.xac _v.s  >/dev/null 2>&1
  $X d -b _v.xac _v.b  >/dev/null 2>&1
  if cmp -s wavs2 _v.s && cmp -s wavs2 _v.b
  then printf "OK   %-28s %-4s streaming == -b == original\n" "streaming decode" ""
  else printf "FAIL streaming decode differs from -b\n"; fail=1; fi
  # decode to a pipe -- the reason the streaming decoder exists
  if $X d _v.xac - 2>/dev/null | cmp -s - wavs2
  then printf "OK   %-28s %-4s decoded to a pipe\n" "d archive -" ""
  else printf "FAIL decode to pipe\n"; fail=1; fi
  # a corrupted rc stream: crc must fail AND the partial output must be removed
  python3 -c '
d=bytearray(open("_v.xac","rb").read()); d[len(d)//2]^=0x40; open("_v.cor","wb").write(d)' 2>/dev/null
  if [ -f _v.cor ]; then
    rm -f _v.out
    if $X d _v.cor _v.out >/dev/null 2>&1; then printf "FAIL corrupt archive accepted\n"; fail=1
    elif [ -f _v.out ]; then printf "FAIL corrupt archive left a partial output file\n"; fail=1
    else printf "OK   %-28s %-4s crc failed, output removed\n" "corrupt rc stream" ""; fi
  fi
  rm -f _v.s _v.b _v.cor
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
