#!/bin/sh
# Verify dff2dsf against FFmpeg: decoding the source .dff and the produced .dsf
# to PCM must yield identical samples.
#
#   tests/verify.sh input.dff [ffmpeg-binary]
set -e

DFF="$1"
FFMPEG="${2:-ffmpeg}"
BIN="$(dirname "$0")/../dff2dsf"

if [ -z "$DFF" ]; then
    echo "usage: $0 input.dff [ffmpeg-binary]" >&2
    exit 1
fi

DSF="$(mktemp -u)".dsf
trap 'rm -f "$DSF"' EXIT

"$BIN" d "$DFF" "$DSF"

ref=$("$FFMPEG" -v error -i "$DFF" -c:a pcm_s24le -f md5 -)
got=$("$FFMPEG" -v error -i "$DSF" -c:a pcm_s24le -f md5 -)

echo "dff: $ref"
echo "dsf: $got"

if [ "$ref" = "$got" ]; then
    echo "PASS: bit-exact"
else
    echo "FAIL: output differs" >&2
    exit 1
fi
