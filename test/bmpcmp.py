#!/usr/bin/env python3
"""Compares two BMPs by what they depict rather than byte for byte.

The coder rebuilds the file header from scratch -- it does not carry the
original's resolution fields, its declared palette size or its run-length
encoding -- so file equality is the wrong question for the first round trip.
This asks the right one: same geometry, same palette, same pixels.
"""

import struct
import sys


def read_bmp(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("%s: not a BMP" % path)
    off_bits = struct.unpack_from("<I", data, 10)[0]
    (info_size, width, height, planes, bpp, compression, _size_image,
     _xppm, _yppm, clr_used, _clr_imp) = struct.unpack_from("<IiiHHIIiiII", data, 14)
    if info_size != 40:
        raise ValueError("%s: unsupported info header (%d bytes)" % (path, info_size))
    if compression not in (0, 1, 2):
        raise ValueError("%s: unsupported compression %d" % (path, compression))
    palette = []
    if bpp <= 8:
        n = clr_used or (1 << bpp)
        for i in range(n):
            b, g, r, _ = struct.unpack_from("<4B", data, 14 + info_size + 4 * i)
            palette.append((r, g, b))
    used = (width * bpp + 7) // 8
    if compression == 0:
        stride = (width * bpp + 31) // 32 * 4
        rows = []
        for y in range(height):
            start = off_bits + (height - 1 - y) * stride
            rows.append(data[start:start + used])
    else:
        rows = unrle(data[off_bits:], width, height, used, 8 if compression == 1 else 4)
    return dict(width=width, height=height, bpp=bpp, palette=palette, rows=rows,
                planes=planes)


def unrle(data, width, height, used, bpp):
    """Expands a run-length encoded bitmap into top-down rows of packed pixels.

    Only what the test corpus produces is handled: runs, literals, end of line
    and end of bitmap.  Delta moves would need a second index and nothing here
    emits them.
    """
    rows = [bytearray(used) for _ in range(height)]

    def put(row, x, value):
        if x >= width:
            return
        if bpp == 8:
            row[x] = value
        else:
            shift = 4 if x % 2 == 0 else 0
            row[x // 2] = (row[x // 2] & ~(0xF << shift)) | ((value & 0xF) << shift)

    y, x, i = height - 1, 0, 0
    while i + 1 < len(data) and y >= 0:
        n, v = data[i], data[i + 1]
        i += 2
        if n:
            if bpp == 8:
                for k in range(n):
                    put(rows[y], x + k, v)
            else:
                for k in range(n):
                    put(rows[y], x + k, v >> 4 if k % 2 == 0 else v & 0xF)
            x += n
        elif v == 0:
            y -= 1
            x = 0
        elif v == 1:
            break
        elif v == 2:
            x += data[i]
            y -= data[i + 1]
            i += 2
        else:
            count = v if bpp == 8 else (v + 1) // 2
            for k in range(v):
                byte = data[i + (k if bpp == 8 else k // 2)]
                put(rows[y], x + k, byte if bpp == 8 else
                    (byte >> 4 if k % 2 == 0 else byte & 0xF))
            x += v
            i += count + (count & 1)
    return [bytes(r) for r in rows]


def pixels(img):
    """Row-major pixel values: an index for the palette depths, a tuple otherwise."""
    bpp, w = img["bpp"], img["width"]
    out = []
    for row in img["rows"]:
        if bpp in (24, 32):
            n = bpp // 8
            out.append([tuple(row[x * n:x * n + n]) for x in range(w)])
        elif bpp == 8:
            out.append(list(row[:w]))
        else:
            per = 8 // bpp
            mask = (1 << bpp) - 1
            out.append([(row[x // per] >> (8 - bpp * (x % per + 1))) & mask
                        for x in range(w)])
    return out


def compare(path_a, path_b):
    a, b = read_bmp(path_a), read_bmp(path_b)
    for key in ("width", "height", "bpp"):
        if a[key] != b[key]:
            return "%s differs: %s vs %s" % (key, a[key], b[key])
    if a["bpp"] <= 8:
        # The coder always writes a full palette; the source may have declared a
        # shorter one, so only the entries the source actually carried are compared.
        n = min(len(a["palette"]), len(b["palette"]))
        if a["palette"][:n] != b["palette"][:n]:
            for i in range(n):
                if a["palette"][i] != b["palette"][i]:
                    return "palette entry %d differs: %s vs %s" % (
                        i, a["palette"][i], b["palette"][i])
    pa, pb = pixels(a), pixels(b)
    for y in range(a["height"]):
        if pa[y] != pb[y]:
            for x in range(a["width"]):
                if pa[y][x] != pb[y][x]:
                    return "pixel (%d, %d) differs: %s vs %s" % (x, y, pa[y][x], pb[y][x])
    return None


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: bmpcmp.py a.bmp b.bmp")
    problem = compare(sys.argv[1], sys.argv[2])
    if problem:
        sys.exit("%s vs %s: %s" % (sys.argv[1], sys.argv[2], problem))
