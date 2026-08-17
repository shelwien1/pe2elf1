#!/bin/sh
# Verify dff2dsf against FFmpeg, both directions.
#
#   tests/verify.sh input.dff [ffmpeg-binary]
#
#   1. decode input.dff to .dsf; FFmpeg must decode both to identical samples
#   2. re-compress that .dsf to .dff; FFmpeg must decode it to the same samples
#   3. decode the re-compressed .dff again; it must match the .dsf bit for bit
#   4. compare the compressed size against the input
set -e

DFF="$1"
FFMPEG="${2:-ffmpeg}"
BIN="$(dirname "$0")/../dff2dsf"

if [ -z "$DFF" ]; then
    echo "usage: $0 input.dff [ffmpeg-binary]" >&2
    exit 1
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
DSF="$TMP/a.dsf"
DFF2="$TMP/b.dff"
DSF2="$TMP/c.dsf"

# The size of the DST frames themselves, which is the compressed audio proper:
# container extras such as the frame index chunk some encoders add are not
# compressed audio and would otherwise flatter whichever side omits them.
payload() {
    sed -n 's/^dst payload: \([0-9]*\).*/\1/p' "$1"
}

status=0

"$BIN" d "$DFF" "$DSF" 2>"$TMP/dec1.log" || { cat "$TMP/dec1.log"; exit 1; }
ref=$("$FFMPEG" -v error -i "$DFF" -c:a pcm_s24le -f md5 -)
got=$("$FFMPEG" -v error -i "$DSF" -c:a pcm_s24le -f md5 -)
echo "decode:  dff=$ref"
echo "         dsf=$got"
if [ "$ref" = "$got" ]; then
    echo "  PASS: decoded output is bit-exact with FFmpeg"
else
    echo "  FAIL: decoded output differs from FFmpeg" >&2
    status=1
fi

"$BIN" c "$DSF" "$DFF2" 2>"$TMP/enc.log" || { cat "$TMP/enc.log"; exit 1; }
got=$("$FFMPEG" -v error -i "$DFF2" -c:a pcm_s24le -f md5 -)
echo "encode:  dff=$got"
if [ "$ref" = "$got" ]; then
    echo "  PASS: FFmpeg decodes our DST stream to the same samples"
else
    echo "  FAIL: FFmpeg decodes our DST stream differently" >&2
    status=1
fi

"$BIN" d "$DFF2" "$DSF2" 2>"$TMP/dec2.log" || { cat "$TMP/dec2.log"; exit 1; }
if cmp -s "$DSF" "$DSF2"; then
    echo "  PASS: compress/decompress round trip is bit-exact"
else
    echo "  FAIL: round trip is not bit-exact" >&2
    status=1
fi

a=$(payload "$TMP/dec1.log")
b=$(payload "$TMP/dec2.log")
if [ -n "$a" ] && [ -n "$b" ] && [ "$a" -gt 0 ]; then
    echo "size:    input DST data=$a  ours=$b"
    echo "$a $b" | awk '{printf "  %+.2f%% against the input\n", ($2-$1)*100/$1}'
fi

exit $status
