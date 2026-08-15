#!/bin/sh
# mkcorpus.sh -- build the two working corpora out of the source BMPs.
# Neither is in git: they are crops, and the 8 MB source image is not either.
#
#   tools/mkcorpus.sh [path-to-20000171A.bmp]
#
#   work/bmp/big_crop.bmp  128 rows of the 8 MB image -- sweep.py's stand-in
#                          for it, so a model-shape run is ~1 min not ~10
#   work/opt/*.bmp         8/24/32bpp, cropped small enough that IDX/opt.pl's
#                          per-bit measure() pass costs a couple of seconds
set -e
cd "$(dirname "$0")/.."
BIG=${1:-work/bmp/20000171A.bmp}
mkdir -p work/bmp work/opt
[ -f "$BIG" ] && tools/bmpcrop.py "$BIG" work/bmp/big_crop.bmp 128
[ -f "$BIG" ] && tools/bmpcrop.py "$BIG" work/opt/big8.bmp 8
tools/bmpcrop.py testfiles/t8g.bmp  work/opt/t8gc.bmp 120
tools/bmpcrop.py testfiles/t24.bmp  work/opt/t24c.bmp 80
tools/bmpcrop.py testfiles/x_ep.bmp work/opt/ep32.bmp 32
