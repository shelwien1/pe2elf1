#!/usr/bin/env bash
# Regenerate the coder variants in this directory from the images in ../.
#
# jpegtran transcodes losslessly in the DCT domain, so every variant of an image
# holds exactly the same coefficients.  That is what makes them a test: pjpg's
# Huffman and arithmetic decoders must report identical MCU and block counts for
# all four, and `make coders` checks exactly that.
#
# Needs libjpeg-turbo's jpegtran (Debian/Ubuntu: libjpeg-turbo-progs).
set -u
cd "$(dirname "$0")"
command -v jpegtran >/dev/null || { echo "jpegtran not found (apt install libjpeg-turbo-progs)"; exit 1; }

# Small images only -- this data is checked in.  jcaron is 4-component CMYK,
# which exercises a component count above 3.
for b in 10-2-t laplata4_1 jcaron; do
  src="../$b.jpg"
  [ -f "$src" ] || { echo "missing $src"; continue; }
  jpegtran -copy none -optimize                 -outfile "$b.base.jpg"      "$src" || continue
  jpegtran -copy none -progressive              -outfile "$b.prog.jpg"      "$src"
  jpegtran -copy none -arithmetic               -outfile "$b.arith.jpg"     "$src"
  jpegtran -copy none -arithmetic -progressive  -outfile "$b.arithprog.jpg" "$src"
  echo "  $b -> base prog arith arithprog"
done
