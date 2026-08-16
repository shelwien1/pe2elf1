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
    print(len(CASES), rle_pairs(out))
    return 0



# ---- run-length pairs ------------------------------------------------------
#
# `read_bmp`'s two run-length arms decode opcodes the corpus does not contain.
# `testfiles/rle4.bmp` and `rle8.bmp` are real files and they use encoded runs
# and end-of-line and nothing else: perturbing any of the four paths of
# `write_nibbles` -- the absolute-run writer -- leaves all 110 checks green.
#
# These are pairs.  One file carries an opcode stream that uses *every* opcode
# at both nibble parities; the other carries the same pixels uncompressed,
# decoded here by an implementation that is not BMF's.  Both go through
# `bmf c` and `bmf d`, and the two outputs have to agree -- which they can only
# do if the two readers agree about what the opcodes mean.  No reference stream
# is needed and none could exist.


def rle4_stream():
    """(width, height, opcodes) using every RLE4 opcode, both parities."""
    # Twenty-four wide with twenty pixels written: `read_bmp` refuses a run
    # that reaches within a byte of the end of the buffer -- `row + run/2 + 1 >
    # pix_hi` -- so a run the full width of the last row in memory is rejected
    # before any of this is exercised.  The four pixels left over stay at zero
    # in both files.
    w, h = 24, 4
    op = bytearray()
    # Bottom row: one encoded run the width of the image.
    op += bytes((20, 0x12))
    op += bytes((0, 0))                       # end of line
    # An odd encoded run leaves the cursor mid-byte; the absolute run after it
    # is the unaligned arm, and it is an odd number of nibbles.
    op += bytes((3, 0x34))
    op += bytes((0, 5, 0x56, 0x78, 0x90, 0))  # absolute, 5 nibbles + pad
    op += bytes((12, 0xAB))
    op += bytes((0, 0))
    # An odd absolute run from an *aligned* cursor -- the one arm of
    # `write_nibbles` the row above does not reach -- then an odd encoded run
    # to bring the cursor back into step, then an even absolute run.
    # The last nibble of this one is 0xD and not 0: the arm that writes it
    # masks with 0xF0, and a run whose final nibble is even cannot tell that
    # mask from a wrong one.
    op += bytes((0, 5, 0xCD, 0xEF, 0xD1, 0))  # absolute, 5 nibbles + pad
    op += bytes((3, 0x23))
    op += bytes((0, 6, 0x45, 0x67, 0x89, 0))  # absolute, 6 nibbles + pad
    op += bytes((6, 0x2A))
    op += bytes((0, 0))
    # A delta that skips two pixels, which stay at zero.
    op += bytes((4, 0x45))
    op += bytes((0, 2, 2, 0))                 # delta, right two, same row
    op += bytes((14, 0x67))
    op += bytes((0, 1))                       # end of bitmap
    return w, h, bytes(op)


def rle8_stream():
    """(width, height, opcodes) using every RLE8 opcode."""
    # Sixteen wide with twelve written, for the reason `rle4_stream` gives.
    w, h = 16, 3
    op = bytearray()
    op += bytes((12, 0x11))
    op += bytes((0, 0))
    op += bytes((0, 5, 1, 2, 3, 4, 5, 0))     # absolute, odd count + pad
    op += bytes((7, 0x22))
    op += bytes((0, 0))
    op += bytes((0, 4, 6, 7, 8, 9))           # absolute, even count, no pad
    op += bytes((2, 0x33))
    op += bytes((0, 2, 2, 0))                 # delta, right two
    op += bytes((4, 0x44))
    op += bytes((0, 1))
    return w, h, bytes(op)


def decode_rle(op, w, h, bits):
    """The opcode stream as rows of pixel values, bottom row first.

    Written from the format's own description rather than from `read_bmp`, so
    that the two agreeing is evidence.
    """
    rows = [[0]*w for _ in range(h)]
    y, x, i = 0, 0, 0
    while i < len(op):
        n, v = op[i], op[i+1]
        i += 2
        if n:
            for k in range(n):
                if x < w:
                    rows[y][x] = v if bits == 8 else (v >> 4 if k % 2 == 0
                                                      else v & 0xF)
                x += 1
            continue
        if v == 0:
            y, x = y+1, 0
        elif v == 1:
            break
        elif v == 2:
            x += op[i]
            y += op[i+1]
            i += 2
        else:
            run = op[i:i+((v+1)//2 if bits == 4 else v)]
            i += len(run)+(len(run) & 1)      # padded to a word
            for k in range(v):
                if x < w:
                    rows[y][x] = (run[k] if bits == 8 else
                                  (run[k//2] >> 4 if k % 2 == 0
                                   else run[k//2] & 0xF))
                x += 1
    return rows


def rle_bmp(path, w, h, bits, op):
    """The opcode stream itself, as a BI_RLE4 or BI_RLE8 file."""
    n = 1 << bits
    pal = b''.join(bytes(((255*i)//(n-1),)*3+(0,)) for i in range(n))
    off = 54+len(pal)
    hdr = b'BM'+struct.pack('<IHHI', off+len(op), 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, bits,
                       2 if bits == 4 else 1, len(op), 0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(hdr+info+pal+op)


def raw_bmp(path, w, h, bits, rows):
    """The same pixels, uncompressed."""
    n = 1 << bits
    pal = b''.join(bytes(((255*i)//(n-1),)*3+(0,)) for i in range(n))
    stride = ((bits*w+31)//32)*4
    pix = bytearray()
    for row in rows:
        line = bytearray(stride)
        for x, val in enumerate(row):
            if bits == 8:
                line[x] = val
            else:
                line[x//2] |= (val << 4) if x % 2 == 0 else val
        pix += line
    off = 54+len(pal)
    hdr = b'BM'+struct.pack('<IHHI', off+len(pix), 0, 0, off)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, bits, 0, len(pix),
                       0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(hdr+info+pal+bytes(pix))


def rle_pairs(out):
    """Write the pairs and answer how many there are."""
    for name, bits, make in (('rle4', 4, rle4_stream), ('rle8', 8, rle8_stream)):
        w, h, op = make()
        rle_bmp(os.path.join(out, name+'_rle.bmp'), w, h, bits, op)
        raw_bmp(os.path.join(out, name+'_raw.bmp'), w, h, bits,
                decode_rle(op, w, h, bits))
    return 2


if __name__ == '__main__':
    sys.exit(main())
