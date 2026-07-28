#!/bin/sh
# md5s.sh [binary] -- the output-identity matrix.  Every Tier 0/1 change must
# leave this untouched; a difference is a bug, not a ratio regression.
X=${1:-./xadpcm}
for f in wavs2 Player_Death_Music_ima.wav; do
  for m in "" "-s" "-ss"; do
    $X c $m "$f" _m.xac >/dev/null 2>&1
    printf "%-30s %-4s %s  %d\n" "$f" "${m:-none}" "`md5sum < _m.xac | cut -c1-32`" `stat -c%s _m.xac`
  done
done
[ -d gen ] || python3 gen_testwavs.py >/dev/null 2>&1
for f in gen/*.wav; do
  $X c "$f" _m.xac >/dev/null 2>&1
  printf "%-30s %-4s %s  %d\n" "$f" none "`md5sum < _m.xac | cut -c1-32`" `stat -c%s _m.xac`
done
cat gen/*.wav > _m.all
for m in "" "-ss"; do
  $X c $m _m.all _m.xac >/dev/null 2>&1
  printf "%-30s %-4s %s  %d\n" "gen-concat" "${m:-none}" "`md5sum < _m.xac | cut -c1-32`" `stat -c%s _m.xac`
done
$X c wavs2 Player_Death_Music_ima.wav _m.xac >/dev/null 2>&1
printf "%-30s %-4s %s  %d\n" "multi(2)" none "`md5sum < _m.xac | cut -c1-32`" `stat -c%s _m.xac`
$X c -ss wavs2 Player_Death_Music_ima.wav _m.xac >/dev/null 2>&1
printf "%-30s %-4s %s  %d\n" "multi(2)" -ss "`md5sum < _m.xac | cut -c1-32`" `stat -c%s _m.xac`
rm -f _m.xac _m.all
