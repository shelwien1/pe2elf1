#!/usr/bin/env python3
# -------------------------------------------------------------
# bmpn.py -- 1, 4 and 8bpp BMPs out of 24/32bpp ones, for testing the
# codec's one-component path and the two packed depths below it.
#
#   ./bmpn.py in.bmp             writes in_gray.bmp and in_pal.bmp
#   ./bmpn.py in.bmp -gray       just the grey one
#   ./bmpn.py in.bmp -pal 64     a 64-colour palette
#   ./bmpn.py in.bmp -bpp 4      16 levels and 16 colours, packed two to a byte
#   ./bmpn.py in.bmp -bpp 1      bilevel
#
# -bpp caps the palette: -bpp 4 means at most 16 entries and -bpp 1 at
# most two, and the raster is packed at that depth with the leftmost
# pixel in the most significant bits, which is what BMP means by 1 and
# 4bpp.  The suffix carries the depth, so -bpp 4 writes in_gray4.bmp.
#
# Two kinds, because they are different problems for a predictive coder:
#
#   grey    the palette is the identity ramp, so an index *is* a
#           luminance and a predictor over indices predicts the picture
#   palette median cut to N colours, and then an index is a label.
#           Neighbouring labels need have nothing to do with each other,
#           which is the case where prediction has nothing to work with
#           and the codec should be measured honestly against that.
#
# Both are lossy against the original -- that is what 8 bits per pixel
# costs -- and neither is meant to be anything but an input file.  What
# has to be lossless is mrpc's round trip of the result.
# -------------------------------------------------------------

import struct, sys, os


def read_bmp(path):
    d = open(path, 'rb').read()
    if d[:2] != b'BM':
        raise SystemExit(path + ': not a BMP')
    off, isz = struct.unpack('<I', d[10:14])[0], struct.unpack('<I', d[14:18])[0]
    w, h = struct.unpack('<ii', d[18:26])
    bpp = struct.unpack('<H', d[28:30])[0]
    comp = struct.unpack('<I', d[30:34])[0]
    if isz < 40 or comp != 0 or bpp not in (24, 32):
        raise SystemExit('%s: want an uncompressed 24 or 32bpp BMP, got %dbpp' % (path, bpp))
    nc, flip = bpp // 8, h < 0
    h = abs(h)
    stride = ((w * bpp + 31) // 32) * 4
    px = [[None] * w for _ in range(h)]
    for y in range(h):
        row = d[off + y * stride:off + y * stride + w * nc]
        for x in range(w):
            b, g, r = row[x * nc], row[x * nc + 1], row[x * nc + 2]
            px[y][x] = (r, g, b)
    return w, h, px, flip


def write_bmp(path, w, h, idx, pal, flip, bits=8):
    """One row per row, padded to four bytes.  At 1 and 4bpp the leftmost
    pixel goes in the most significant bits, and whatever is left over at
    the end of a row is zero -- mrpc round-trips those bits too, so they
    are as much a part of the test as the pixels are."""
    ncol = 1 << bits
    stride = ((w * bits + 31) // 32) * 4
    off = 54 + 4 * ncol
    raster = bytearray()
    for y in range(h):
        row = bytearray(stride)
        if bits == 8:
            row[:w] = bytes(idx[y])
        elif bits == 4:
            for x in range(w):
                row[x >> 1] |= (idx[y][x] & 15) << (0 if (x & 1) else 4)
        else:
            for x in range(w):
                row[x >> 3] |= (idx[y][x] & 1) << (7 - (x & 7))
        raster += row
    hdr = b'BM' + struct.pack('<IHHI', off + len(raster), 0, 0, off)
    hdr += struct.pack('<IiiHHIIiiII', 40, w, -h if flip else h, 1, bits, 0,
                       len(raster), 2835, 2835, ncol, 0)
    table = bytearray()
    for i in range(ncol):
        r, g, b = pal[i] if i < len(pal) else (0, 0, 0)
        table += bytes((b, g, r, 0))
    open(path, 'wb').write(hdr + bytes(table) + bytes(raster))
    return off + len(raster)


def to_gray(w, h, px, bits=8):
    """Rec.601 luma.  At 8bpp the palette is the identity ramp, so an
    index *is* a grey level; below that it is the ramp of the levels that
    are left, and the index is that level's rank."""
    n = 1 << bits
    idx = [bytes(((77 * r + 150 * g + 29 * b + 128) >> 8) * (n - 1) // 255
                 for (r, g, b) in row) for row in px]
    return idx, [(i * 255 // (n - 1),) * 3 for i in range(n)]


def to_palette(w, h, px, ncol=256):
    """Median cut: split the box with the longest side at its median,
    over and over, until there are ncol of them."""
    pts = [p for row in px for p in row]
    boxes = [pts]
    while len(boxes) < ncol:
        # the box worth splitting is the one that spans the most
        best, bi, bax = -1, -1, 0
        for i, bx in enumerate(boxes):
            if len(bx) < 2:
                continue
            for ax in range(3):
                lo = min(p[ax] for p in bx)
                hi = max(p[ax] for p in bx)
                if hi - lo > best:
                    best, bi, bax = hi - lo, i, ax
        if bi < 0 or best <= 0:
            break
        bx = sorted(boxes[bi], key=lambda p: p[bax])
        m = len(bx) // 2
        boxes[bi:bi + 1] = [bx[:m], bx[m:]]
    pal = []
    for bx in boxes:
        n = len(bx)
        pal.append(tuple(sum(p[k] for p in bx) // n for k in range(3)))
    while len(pal) < ncol:
        pal.append((0, 0, 0))

    # nearest palette entry, on a cache since an image has far fewer
    # distinct colours than pixels
    cache = {}

    def nearest(p):
        v = cache.get(p)
        if v is None:
            v = min(range(len(boxes)),
                    key=lambda i: (pal[i][0] - p[0]) ** 2 + (pal[i][1] - p[1]) ** 2 +
                                  (pal[i][2] - p[2]) ** 2)
            cache[p] = v
        return v

    idx = [bytes(nearest(p) for p in row) for row in px]
    return idx, pal


def main(argv):
    if len(argv) < 2:
        print('usage: bmpn.py in.bmp [-gray] [-pal [ncolours]] [-bpp 1|4|8]')
        return 1
    src = argv[1]
    want_gray = '-gray' in argv or '-pal' not in argv
    want_pal = '-pal' in argv or '-gray' not in argv
    bits = 8
    if '-bpp' in argv:
        i = argv.index('-bpp')
        if i + 1 >= len(argv) or argv[i + 1] not in ('1', '4', '8'):
            raise SystemExit('-bpp takes 1, 4 or 8')
        bits = int(argv[i + 1])
    ncol = 1 << bits
    if '-pal' in argv:
        i = argv.index('-pal')
        if i + 1 < len(argv) and argv[i + 1].isdigit():
            ncol = int(argv[i + 1])
    if ncol > (1 << bits):
        raise SystemExit('%d colours do not fit in %d bits' % (ncol, bits))
    tag = '' if bits == 8 else str(bits)
    base = os.path.splitext(src)[0]
    w, h, px, flip = read_bmp(src)
    if want_gray:
        idx, pal = to_gray(w, h, px, bits)
        n = write_bmp(base + '_gray%s.bmp' % tag, w, h, idx, pal, flip, bits)
        print('%s_gray%s.bmp  %dx%d %dbpp grey  %d B' % (base, tag, w, h, bits, n))
    if want_pal:
        idx, pal = to_palette(w, h, px, ncol)
        n = write_bmp(base + '_pal%s.bmp' % tag, w, h, idx, pal, flip, bits)
        print('%s_pal%s.bmp   %dx%d %dbpp %d-colour palette  %d B'
              % (base, tag, w, h, bits, ncol, n))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
