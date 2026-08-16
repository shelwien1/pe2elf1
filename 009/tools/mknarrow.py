#!/usr/bin/env python3
"""Write the degenerate-geometry BMPs `tools/narrow.sh` round-trips.

    python3 tools/mknarrow.py <dir>

The corpus in `testfiles/` is seventeen real images, and every one of them is
wider than three pixels and taller than two.  That is not an accident of taste
-- they are photographs and screenshots -- but it means a family of geometry
cases has never been executed at all: a one-pixel row, a one-pixel column, a
single pixel, a packed depth at width one, and the sizes either side of
`search_filter`'s `4 x 3` floor, where the encoder stops choosing predictors.

These images have no reference stream and cannot have one: they are generated
here rather than compressed by BMF.exe in 1999, so what they check is the round
trip, not fidelity.  `narrow.sh` says what that does and does not buy.

The resolution fields are zero because `write_bmp` writes zero into them, so a
generator that fills in the usual 2835 dots per metre produces a file that
differs from its own round trip in four bytes for a reason that has nothing to
do with the image.
"""
import os
import random
import struct
import sys

# width, height, bits, name.  The two-pixel and three-pixel cases are here
# because `4 x 3` is where `search_filter` gives up and the sizes either side of
# a threshold are the ones worth having.
CASES = [
    (1, 64, 24, 'w1'),
    (64, 1, 24, 'h1'),
    (1, 1, 24, 'p1'),
    (2, 32, 24, 'w2'),
    (32, 2, 24, 'h2'),
    (3, 7, 24, 'w3'),
    (5, 3, 24, 'edge'),
    (1, 64, 32, 'w1x32'),
    (64, 1, 32, 'h1x32'),
    (1, 48, 8, 'w1x8'),
    (48, 1, 8, 'h1x8'),
    (1, 48, 4, 'w1x4'),
    (1, 48, 1, 'w1x1'),
    (2, 2, 24, 'tiny'),
]


def bmp(path, w, h, bits, seed):
    """One BMP, bottom-up, with a greyscale palette when it needs one."""
    rnd = random.Random(seed)
    pal = b''
    if bits <= 8:
        n = 1 << bits
        # A ramp rather than 256 random entries: the palette is not what these
        # images are for, and a ramp makes a failure readable by eye.
        for i in range(n):
            v = (255*i)//max(1, n-1)
            pal += bytes((v, v, v, 0))
    stride = ((bits*w+31)//32)*4
    pix = bytearray()
    for _ in range(h):
        row = bytearray(stride)
        for x in range(w):
            if bits >= 24:
                for c in range(bits//8):
                    row[(bits//8)*x+c] = rnd.randrange(256)
            elif bits == 8:
                row[x] = rnd.randrange(256)
            else:
                per = 8//bits
                shift = 8-bits*(x % per+1)
                row[x//per] |= rnd.randrange(1 << bits) << shift
        pix += row
    off = 54+len(pal)
    hdr = b'BM'+struct.pack('<IHHI', off+len(pix), 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, bits, 0, len(pix),
                       0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(hdr+info+pal+bytes(pix))


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip().split('\n\n')[1], file=sys.stderr)
        return 1
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for i, (w, h, bits, name) in enumerate(CASES):
        bmp(os.path.join(out, name+'.bmp'), w, h, bits, i+1)
    print(len(CASES))
    return 0


if __name__ == '__main__':
    sys.exit(main())
