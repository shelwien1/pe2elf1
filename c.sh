#!/bin/sh
# c.sh - Compress a PNG file (linux equivalent of c1.bat).
# Usage: c.sh <input.png>
# Produces:  <name>.tar.xz  (containing <name>.bmp, <name>.bmp.meta2, dump*.hif)

set -e

if [ $# -lt 1 ] || [ ! -f "$1" ]; then
    echo "usage: $0 <input.png>" >&2
    exit 1
fi

here="$(cd "$(dirname "$0")" && pwd)"
PNGDUMP="$here/pngdump"
PNG2BMP="$here/png2bmp"
PREFLATE="$here/preflate_raw"

in_png="$1"
base="$(basename "$in_png" .png)"

rm -rf out
mkdir -p out

echo "[1/3] dump deflate streams"
"$PNGDUMP" "$in_png" out/meta

echo "[2/3] recompress deflate streams with preflate"
for f in out/dump*.bin; do
    "$PREFLATE" -s "$f" >/dev/null
    name="${f%.bin}"
    mv "$f.u" "$name.unp"
    mv "$f.r" "$name.hif"
done

echo "[3/3] convert unpacked dump data to .bmp"
"$PNG2BMP" c out/meta "$base.bmp"

echo "packing $base.tar.xz"
tar --transform 's|^out/||' -cJf "$base.tar.xz" \
    "$base.bmp" "$base.bmp.meta2" out/dump*.hif

rm -rf out "$base.bmp" "$base.bmp.meta2"

ls -l "$base.tar.xz"
