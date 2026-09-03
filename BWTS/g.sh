#!/bin/sh
# Linux/POSIX counterpart of g.bat.  Builds all three BWTS variants:
#   bwts  bytewise bijective BWT           (chunk size argument in BYTES)
#   bwth  bitwise bijective BWT, MSB-first (chunk size argument in KB)
#   bwtl  bitwise bijective BWT, LSB-first (chunk size argument in KB)
#
#   ./bwth c256 in out    # transform, 256 KB blocks
#   ./bwth d256 in out    # inverse
#
# The bit order matters and is data-dependent: measured on 262 kB of gzip -9
# output, bwtl+cdm gives 3.01 % where bwth+cdm gives 0.42 % (deflate packs its
# Huffman codes LSB-first).  On text and JPEG, bwth wins.  Try both.

set -e

CXX=${CXX:-g++}
CXXARCH=${CXXARCH:-}
LDFLAGS=${LDFLAGS:-}

incs="-DNDEBUG -DINC_FLEN -I../Lib3"

case "$($CXX --version 2>&1 | head -1)" in
  *clang*) WHOLE="-flto" ;;
  *)       WHOLE="-fwhole-program" ;;
esac

opts="$WHOLE -fstrict-aliasing -fomit-frame-pointer -ffast-math
-fno-rtti -fno-exceptions
-fno-stack-protector -fno-stack-check -fno-check-new"

for a in bwts bwth bwtl; do
  rm -f $a
  # shellcheck disable=SC2086
  $CXX -std=gnu++1z -O3 -s $CXXARCH $incs $opts $LDFLAGS $a.cpp -o $a
done

ls -l bwts bwth bwtl
