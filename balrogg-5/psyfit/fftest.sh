#!/bin/sh
#  Make Ogg Vorbis files with ffmpeg's own encoder and round-trip them.
#
#      sh psyfit/fftest.sh [outdir]
#
#  `-c:a vorbis` is ffmpeg's own encoder, not libvorbis; `-strict -2` is
#  needed because ffmpeg marks it experimental.  Its codebooks are a fixed
#  set of 29, which vbooks_gen.inc carries by way of psyfit/ffbookgen.c, so
#  these are the files that exercise that row -- nothing libvorbis wrote ever
#  will.  It only encodes stereo.
#
#  The source is synthesised rather than sampled so that the files are
#  reproducible from nothing: a steady tone in one channel against a
#  quadratic sweep in the other, which gives the encoder both a stationary
#  and a moving spectrum to fit.  The audio is the same every run, but the
#  files are not quite: an Ogg stream's serial number is random, so two runs
#  give byte-different files of the same length, and TSVs that differ by a
#  digit or two.  Each file still has to round-trip to itself.
set -u
out=${1:-fftest}
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$out" || exit 2

ffmpeg -hide_banner -loglevel error -y \
  -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=6" \
  -f lavfi -i "aevalsrc='0.4*sin(2*PI*t*t*300)':s=48000:d=6" \
  -filter_complex "[0:a][1:a]amerge=inputs=2[a]" -map "[a]" -ac 2 \
  "$out/src_48000.wav" || exit 2

for r in 8000 11025 16000 22050 32000 44100; do
  ffmpeg -hide_banner -loglevel error -y -i "$out/src_48000.wav" \
    -ar $r -ac 2 "$out/src_$r.wav" || exit 2
done

for r in 8000 11025 16000 22050 32000 44100 48000; do
  for q in 1 5 10; do
    ffmpeg -hide_banner -loglevel error -y -i "$out/src_$r.wav" \
      -c:a vorbis -strict -2 -q:a $q "$out/ff_${r}_q$q.ogg" || exit 2
  done
done
#  and one at the encoder's own default quality
ffmpeg -hide_banner -loglevel error -y -i "$out/src_44100.wav" \
  -c:a vorbis -strict -2 "$out/ff_44100_def.ogg" || exit 2
rm -f "$out"/src_*.wav

echo "wrote `ls "$out"/*.ogg | wc -l` files to $out"
TSV2WAV=$here/../tsv2wav-vorbis exec "$here/../t.sh" "$out"
