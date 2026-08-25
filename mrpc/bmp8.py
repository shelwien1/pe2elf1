#!/usr/bin/env python3
# -------------------------------------------------------------
# bmp8.py -- 8bpp BMPs out of 24/32bpp ones, for testing the codec's
# one-component path.
#
#   ./bmp8.py in.bmp            writes in_gray.bmp and in_pal.bmp
#   ./bmp8.py in.bmp -gray      just the grey one
#   ./bmp8.py in.bmp -pal 64    a 64-colour palette
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


def write_bmp8(path, w, h, idx, pal, flip):
    stride = (w + 3) & ~3
    off = 54 + 1024
    raster = bytearray()
    for y in range(h):
        raster += bytes(idx[y]) + bytes(stride - w)
    hdr = b'BM' + struct.pack('<IHHI', off + len(raster), 0, 0, off)
    hdr += struct.pack('<IiiHHIIiiII', 40, w, -h if flip else h, 1, 8, 0,
                       len(raster), 2835, 2835, 256, 0)
    table = bytearray()
    for i in range(256):
        r, g, b = pal[i] if i < len(pal) else (0, 0, 0)
        table += bytes((b, g, r, 0))
    open(path, 'wb').write(hdr + bytes(table) + bytes(raster))
    return off + len(raster)


def to_gray(w, h, px):
    """Rec.601 luma, and the identity palette: index == grey level."""
    idx = [bytes((77 * r + 150 * g + 29 * b + 128) >> 8 for (r, g, b) in row) for row in px]
    return idx, [(i, i, i) for i in range(256)]


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
    while len(pal) < 256:
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
        print(__doc__ or 'usage: bmp8.py in.bmp [-gray] [-pal [ncolours]]')
        return 1
    src = argv[1]
    want_gray = '-gray' in argv or '-pal' not in argv
    want_pal = '-pal' in argv or '-gray' not in argv
    ncol = 256
    if '-pal' in argv:
        i = argv.index('-pal')
        if i + 1 < len(argv) and argv[i + 1].isdigit():
            ncol = int(argv[i + 1])
    base = os.path.splitext(src)[0]
    w, h, px, flip = read_bmp(src)
    if want_gray:
        idx, pal = to_gray(w, h, px)
        n = write_bmp8(base + '_gray.bmp', w, h, idx, pal, flip)
        print('%s_gray.bmp  %dx%d 8bpp grey  %d B' % (base, w, h, n))
    if want_pal:
        idx, pal = to_palette(w, h, px, ncol)
        n = write_bmp8(base + '_pal.bmp', w, h, idx, pal, flip)
        print('%s_pal.bmp   %dx%d 8bpp %d-colour palette  %d B' % (base, w, h, ncol, n))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
