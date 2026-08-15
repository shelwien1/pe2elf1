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

# A synthetic raster where the mask planes are the whole story: 5-bit data
# in the high bits (low 3 always 0) and one bit always 1 in half the blocks.
# Photographs make BLK_MASKS=2 drop the masks; this makes it keep them, and
# it is 28% smaller when it does.  Regression cover for that path.
python3 - <<'PY'
import struct, random
W,H,bpp=256,256,24; stride=((W*bpp+31)//32)*4; off=54
random.seed(7)
px=bytearray()
for y in range(H):
    row=bytearray()
    for x in range(W):
        base=(x//8+y//8)*3
        for k in range(3):
            v=((base+random.randint(0,3)+k*5)&31)<<3
            if (x//64+y//64)&1: v |= 4
            row.append(v&255)
    row += b'\x00'*(stride-W*3)
    px+=row
d=bytearray(b'BM'+struct.pack('<IHHI',off+len(px),0,0,off))
d+=struct.pack('<IiiHHIIiiII',40,W,H,1,bpp,0,len(px),2835,2835,0,0)
d+=px
open('work/bmp/quant.bmp','wb').write(d)
print('work/bmp/quant.bmp %d bytes' % len(d))
PY
