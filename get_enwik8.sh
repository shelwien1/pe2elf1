#!/bin/sh
#
# Fetch the enwik8 benchmark file (the first 10^8 bytes of an English
# Wikipedia dump, the Large Text Compression Benchmark / Hutter Prize corpus)
# into the repository root, where t.sh looks for it.
#
#   ./get_enwik8.sh            -> ./enwik8
#   ./get_enwik8.sh /data      -> /data/enwik8
#
# ~36 MB to download, 100 MB unpacked. Already-present files are left alone.

set -e
cd "$(dirname "$0")"

URL=${URL:-https://mattmahoney.net/dc/enwik8.zip}
DEST=${1:-.}
MD5=a1fa5ffddb56f4953e226637dabbb36a   # of the unpacked enwik8

mkdir -p "$DEST"

if [ -f "$DEST/enwik8" ]; then
  echo "get_enwik8.sh: $DEST/enwik8 is already there"
else
  if [ ! -f "$DEST/enwik8.zip" ]; then
    echo "get_enwik8.sh: downloading $URL"
    if command -v curl >/dev/null 2>&1; then
      curl -fSL --retry 3 -o "$DEST/enwik8.zip.part" "$URL"
    elif command -v wget >/dev/null 2>&1; then
      wget -O "$DEST/enwik8.zip.part" "$URL"
    else
      echo "get_enwik8.sh: neither curl nor wget found" >&2
      exit 1
    fi
    mv "$DEST/enwik8.zip.part" "$DEST/enwik8.zip"
  fi
  command -v unzip >/dev/null 2>&1 || { echo "get_enwik8.sh: unzip not found" >&2; exit 1; }
  unzip -o -q -d "$DEST" "$DEST/enwik8.zip"
fi

# The benchmark is only a benchmark if everyone measures the same bytes.
if command -v md5sum >/dev/null 2>&1; then
  have=$(md5sum "$DEST/enwik8" | cut -d' ' -f1)
  if [ "$have" != "$MD5" ]; then
    echo "get_enwik8.sh: md5 mismatch -- got $have, expected $MD5" >&2
    exit 1
  fi
fi

ls -l "$DEST/enwik8"
