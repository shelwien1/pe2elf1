#!/bin/sh
# gen.sh [DIR] -- a small corpus of AAC streams for make test (DIR, by
# default test/corpus): a test signal (chords, a chirp, plucks, noise bursts)
# through ffmpeg's AAC encoder (LC, Main, LTP, mono, 5.1, with PNS,
# intensity and TNS) as ADTS, LATM and MP4 made AUS1 (by mp42aus1.py), and
# the ADTS made ADIF, LOAS of audioMuxVersion 1 and raw access units (named
# NAME.asc-HEX.raw, for roundtrip.sh's -asc HEX); with fdkaac and faac, when
# they are there, LD and ELD, CRCs, VBR, and HE-AAC and HE-AACv2 when
# fdkaac's library has SBR (a stream an encoder cannot make is said and left
# out). Needs ffmpeg and python3. DIR/.done says that the corpus is whole.
set -e
D=${1:-test/corpus}
T=$(dirname "$0")
mkdir -p "$D"
rm -f "$D/.done"
w=$D/.sig.wav
w48=$D/.sig48.wav
wm=$D/.sigm.wav
# (the signal and what is made from it on the way go however this ends)
trap 'rm -f "$w" "$w48" "$wm" "$D/.ff_lc128.m4a" "$D/.raw" "$D/.enc.err"' EXIT
q="-hide_banner -loglevel error -y"
# 8 s: a chord whose notes change each second, a chirp, plucks every 0.5 s,
# noise bursts; the right channel a little different from the left
L="0.12*sin(2*PI*(220+55*floor(mod(t,4)))*t)+0.08*sin(2*PI*330*t+2*sin(2*PI*2*t))+0.05*sin(2*PI*(500*t+400*t*t/8))"
L="$L+0.2*sin(2*PI*880*t)*exp(-8*mod(t,0.5))+0.06*(random(0)-0.5)*lt(mod(t,2),0.25)"
R="0.12*sin(2*PI*(220+55*floor(mod(t,4)))*t+0.3)+0.07*sin(2*PI*440*t)+0.05*sin(2*PI*(500*t+400*t*t/8))"
R="$R+0.15*sin(2*PI*1320*t)*exp(-6*mod(t+0.25,0.5))+0.06*(random(1)-0.5)*lt(mod(t+1,2),0.25)"
ffmpeg $q -f lavfi -i "aevalsrc='$L|$R':s=44100:d=8" -c:a pcm_s16le "$w"
ffmpeg $q -i "$w" -ar 48000 "$w48"
ffmpeg $q -i "$w" -ac 1 "$wm"

ff() {  # ff NAME ffmpeg-options...: the signal through ffmpeg's AAC encoder
  n=$1
  shift
  ffmpeg $q -i "$w" -c:a aac "$@" "$D/$n"
}
ff ff_lc128.aac -b:a 128k -f adts
ff ff_lc256.aac -b:a 256k -f adts
ff ff_mono64.aac -ac 1 -b:a 64k -f adts
ff ff_tools96.aac -b:a 96k -aac_pns 1 -aac_is 1 -aac_tns 1 -f adts
ff ff_main.aac -profile:a aac_main -b:a 128k -f adts
ff ff_ltp.aac -profile:a aac_ltp -strict -2 -b:a 128k -f adts
ff ff_51.aac -af "pan=5.1|FL=c0|FR=c1|FC=0.5*c0+0.5*c1|LFE=0.3*c0+0.3*c1|BL=0.7*c0|BR=0.7*c1" -ar 48000 -b:a 320k -f adts
ff ff_lc128.latm -b:a 128k -f latm
ff .ff_lc128.m4a -b:a 128k -f mp4
python3 "$T/mp42aus1.py" "$D/.ff_lc128.m4a" "$D/ff_lc128.aus1" > /dev/null
python3 "$T/adts2adif.py" "$D/ff_lc128.aac" "$D/ff_lc128.adif" 1 "a comment"
python3 "$T/adts2loas.py" "$D/ff_lc128.aac" "$D/ff_lc128_v1.loas" 1 1 8 4
python3 "$T/adts2loas.py" "$D/ff_main.aac" "$D/ff_main_v0.loas" 0 2 0 3
asc=$(python3 "$T/adts2adif.py" "$D/ff_mono64.aac" "$D/.raw" 1 "" raw)
mv "$D/.raw" "$D/ff_mono64.asc-$asc.raw"
# a stream between tags and junk, and two containers one after the other
{ printf 'ID3\003\000\000\000\000\000\020'; head -c 16 /dev/zero; cat "$D/ff_lc128.aac"; printf 'TAG junk'; } > "$D/ff_tags.aac"
cat "$D/ff_mono64.aac" "$D/ff_lc128.latm" > "$D/ff_mixed.bin"

if command -v fdkaac > /dev/null; then
  fdk() {  # fdk NAME WAV fdkaac-options...
    n=$1 i=$2
    shift 2
    if ! fdkaac -S "$@" -o "$D/$n" "$i" 2> "$D/.enc.err"; then
      echo "gen.sh: no $n: fdkaac $*: $(tr '\n' ' ' < "$D/.enc.err")" >&2
      rm -f "$D/$n"
    fi
  }
  fdk fdk_lc128.aac "$w" -b 128 -f 2
  fdk fdk_crc.aac "$w" -b 128 -f 2 -C
  fdk fdk_vbr4.aac "$w" -m 4 -f 2
  fdk fdk_he48.aac "$w" -p 5 -b 48 -f 2
  fdk fdk_hev2_24.aac "$w" -p 29 -b 24 -f 2
  fdk fdk_he48.latm "$w" -p 5 -b 48 -f 10
  fdk fdk_ld96.latm "$w48" -p 23 -b 96 -f 10
  fdk fdk_eld96.latm "$w48" -p 39 -b 96 -f 10
  fdk fdk_mono64.aac "$wm" -b 64 -f 2
fi
if command -v faac > /dev/null; then
  if ! faac -b 128 -o "$D/faac128.aac" "$w" > "$D/.enc.err" 2>&1; then
    echo "gen.sh: no faac128.aac: faac: $(tail -n 1 "$D/.enc.err")" >&2
    rm -f "$D/faac128.aac"
  fi
fi
# what an encoder did not write (an option it does not have)
find "$D" -type f -size 0 -exec rm -f {} +
ls "$D"/fdk_he* > /dev/null 2>&1 || echo "gen.sh: no HE-AAC or HE-AACv2 stream: make test covers no SBR or PS" >&2
: > "$D/.done"
echo "$(ls "$D" | wc -l) files in $D"
