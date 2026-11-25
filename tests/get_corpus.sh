#!/bin/bash
# Fetch the 005 test set and keep only the files listed in tests/corpus/005.txt.
#   tests/get_corpus.sh [destdir]        (default: corpus/005)
set -e
DEST=${1:-corpus/005}
URL=https://fish.seedhost.eu/shelwien/downloads/1/005.tar.xz
LIST=$(dirname "$0")/corpus/005.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
curl -sSL -o "$TMP/005.tar.xz" "$URL"
mkdir -p "$DEST"
grep -v '^#' "$LIST" | cut -f1 | sed 's#^#005/#' > "$TMP/names"
tar -xJf "$TMP/005.tar.xz" -C "$TMP" -T "$TMP/names"
mv "$TMP"/005/* "$DEST"/
# single files: JavaScript with base64 + zlib blobs in object text (Linux in a PDF)
curl -sSL -o "$DEST/linux.pdf" https://linux.doompdf.dev/linux.pdf || echo "linux.pdf: download failed" >&2
echo "$(ls "$DEST" | wc -l) files in $DEST"
