#!/bin/sh
# d.sh - Restore a PNG from a .tar.xz produced by c.sh.
# Usage: d.sh <name.tar.xz>

set -e

if [ $# -lt 1 ] || [ ! -f "$1" ]; then
    echo "usage: $0 <name.tar.xz>" >&2
    exit 1
fi

here="$(cd "$(dirname "$0")" && pwd)"
PNGDUMP="$here/pngdump"
PNG2BMP="$here/png2bmp"
PREFLATE="$here/preflate_raw"

ar="$1"
base="$(basename "$ar" .tar.xz)"

rm -rf out
mkdir -p out

echo "[1/3] unpack $ar"
tar -xJf "$ar"
mv dump*.hif out/ 2>/dev/null || true

echo "[2/3] decompose BMP back to dump00.unp + meta"
"$PNG2BMP" d "$base.bmp" out/meta

echo "[3/3] reencode deflate streams"
for f in out/dump*.hif; do
    name="${f%.hif}"
    mv "$name.unp" "$name.bin.u"
    mv "$name.hif" "$name.bin.r"
    "$PREFLATE" -r "$name.bin" >/dev/null
done

"$PNGDUMP" out/meta "$base.png"

rm -rf out "$base.bmp" "$base.bmp.meta2"

ls -l "$base.png"
