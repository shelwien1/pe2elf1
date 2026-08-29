#!/usr/bin/env python3
"""Writes the BMPs the round trip test runs over.

Everything here is written by hand rather than through an imaging library, so
the test corpus is reproducible on a bare machine and so the awkward cases --
odd widths that leave padding bits in a sub-byte row, a 1x1 image, RLE input --
can be produced deliberately.
"""

import math
import os
import struct
import sys

BI_RGB, BI_RLE8, BI_RLE4 = 0, 1, 2


def write_bmp(path, width, height, bpp, rows, palette=None, compression=BI_RGB,
              pixel_data=None):
    """rows: list of row bytes, bottom-up, unpadded.  Or pass pixel_data ready made."""
    pal = b""
    if palette is not None:
        pal = b"".join(struct.pack("<4B", b, g, r, 0) for (r, g, b) in palette)
    if pixel_data is None:
        stride = (len(rows[0]) + 3) & ~3
        pixel_data = b"".join(r + b"\0" * (stride - len(r)) for r in rows)
    off_bits = 14 + 40 + len(pal)
    header = struct.pack("<2sIHHI", b"BM", off_bits + len(pixel_data), 0, 0, off_bits)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, bpp, compression,
                       len(pixel_data), 2835, 2835,
                       len(palette) if palette else 0, 0)
    with open(path, "wb") as f:
        f.write(header + info + pal + pixel_data)


def pack_rows(values, width, bits):
    """One row of pixel indices to a packed row, MSB first."""
    per = 8 // bits
    out = bytearray((width + per - 1) // per)
    for x, v in enumerate(values):
        out[x // per] |= (v & ((1 << bits) - 1)) << (8 - bits * (x % per + 1))
    return bytes(out)


def gradient24(width, height):
    rows = []
    for y in range(height):
        row = bytearray()
        for x in range(width):
            r = (x * 255) // max(width - 1, 1)
            g = (y * 255) // max(height - 1, 1)
            b = (x * x + y * y) % 251
            row += bytes((b, g, r))
        rows.append(bytes(row))
    return rows[::-1]


def photoish24(width, height):
    """Smooth, correlated planes with a little noise -- what the predictor is for."""
    rows = []
    state = 12345
    for y in range(height):
        row = bytearray()
        for x in range(width):
            state = (state * 1103515245 + 12345) & 0x7FFFFFFF
            noise = (state >> 16) % 7 - 3
            base = 128 + 100 * math.sin(x / 17.0) * math.cos(y / 23.0)
            v = int(base + noise)
            r = max(0, min(255, v + 20))
            g = max(0, min(255, v))
            b = max(0, min(255, v - 25))
            row += bytes((b, g, r))
        rows.append(bytes(row))
    return rows[::-1]


def noise24(width, height):
    rows = []
    state = 987654321
    for _ in range(height):
        row = bytearray()
        for _ in range(width * 3):
            state = (state * 1103515245 + 12345) & 0x7FFFFFFF
            row.append((state >> 16) & 0xFF)
        rows.append(bytes(row))
    return rows[::-1]


def rle8_encode(rows):
    """Rows top-down to a BI_RLE8 stream, which the file stores bottom-up.

    Deliberately not the encoding the coder's own writer would choose -- runs are
    emitted greedily and literals only when a run would not pay -- so the reader
    is tested against a stream it did not produce.
    """
    out = bytearray()
    for row in reversed(rows):
        x = 0
        while x < len(row):
            n = 1
            while x + n < len(row) and row[x + n] == row[x] and n < 255:
                n += 1
            if n >= 3:
                out += bytes((n, row[x]))
                x += n
                continue
            lit = [row[x]]
            x += 1
            while x < len(row) and len(lit) < 255:
                n = 1
                while x + n < len(row) and row[x + n] == row[x] and n < 4:
                    n += 1
                if n >= 3:
                    break
                lit.append(row[x])
                x += 1
            if len(lit) >= 3:
                out += bytes((0, len(lit))) + bytes(lit)
                if len(lit) & 1:
                    out += b"\0"
            else:
                for v in lit:
                    out += bytes((1, v))
        out += bytes((0, 0))
    out += bytes((0, 1))
    return bytes(out)


def rle4_encode(rows):
    """Rows top-down to a BI_RLE4 stream.  Runs only, alternating two nibbles."""
    out = bytearray()
    for row in reversed(rows):
        x = 0
        while x < len(row):
            n = 1
            while x + n < len(row) and row[x + n] == row[x + (n & 1)] and n < 254:
                n += 1
            if n < 2:
                n = 1
            hi = row[x]
            lo = row[x + 1] if n > 1 else row[x]
            out += bytes((n, (hi << 4) | (lo & 0xF)))
            x += n
        out += bytes((0, 0))
    out += bytes((0, 1))
    return bytes(out)


def main(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    p = lambda name: os.path.join(out_dir, name)

    grey = [(i, i, i) for i in range(256)]

    # 24 bpp
    write_bmp(p("grad24.bmp"), 61, 43, 24, gradient24(61, 43))
    write_bmp(p("photo24.bmp"), 128, 96, 24, photoish24(128, 96))
    write_bmp(p("noise24.bmp"), 40, 30, 24, noise24(40, 30))
    write_bmp(p("one24.bmp"), 1, 1, 24, [bytes((7, 8, 9))])
    write_bmp(p("thin24.bmp"), 1, 20, 24, [bytes((y * 3 % 256, y * 5 % 256, y * 7 % 256))
                                           for y in range(20)])
    write_bmp(p("wide24.bmp"), 37, 1, 24, [b"".join(bytes((x, 255 - x, x * 3 % 256))
                                                    for x in range(37))])

    # 32 bpp, alpha varying independently of the colour
    rows32 = []
    for y in range(24):
        row = bytearray()
        for x in range(33):
            row += bytes((x * 7 % 256, y * 11 % 256, (x + y) * 3 % 256, (x * y) % 256))
        rows32.append(bytes(row))
    write_bmp(p("alpha32.bmp"), 33, 24, 32, rows32[::-1])

    # 8 bpp, palette
    rows8 = [bytes(((x * 3 + y * 5) % 256) for x in range(53)) for y in range(37)]
    pal8 = [((i * 7) % 256, (i * 13) % 256, (i * 29) % 256) for i in range(256)]
    write_bmp(p("pal8.bmp"), 53, 37, 8, rows8[::-1], palette=pal8)
    write_bmp(p("grey8.bmp"), 64, 64, 8,
              [bytes(((x ^ y) & 0xFF) for x in range(64)) for y in range(64)][::-1],
              palette=grey)

    # 4 bpp -- width 53 leaves four padding bits at the end of every row
    pal4 = [((i * 17) % 256, (i * 37) % 256, (i * 53) % 256) for i in range(16)]
    rows4 = [pack_rows([(x + y) % 16 for x in range(53)], 53, 4) for y in range(29)]
    write_bmp(p("pal4.bmp"), 53, 29, 4, rows4[::-1], palette=pal4)
    rows4e = [pack_rows([(x * y) % 16 for x in range(64)], 64, 4) for y in range(16)]
    write_bmp(p("pal4even.bmp"), 64, 16, 4, rows4e[::-1], palette=pal4)

    # 1 bpp -- width 37 leaves three padding bits at the end of every row
    pal1 = [(0, 0, 0), (255, 255, 255)]
    rows1 = [pack_rows([1 if (x // 3 + y // 2) % 2 else 0 for x in range(37)], 37, 1)
             for y in range(23)]
    write_bmp(p("mono1.bmp"), 37, 23, 1, rows1[::-1], palette=pal1)
    rows1e = [pack_rows([1 if (x * y) % 5 == 0 else 0 for x in range(48)], 48, 1)
              for y in range(32)]
    write_bmp(p("mono1even.bmp"), 48, 32, 1, rows1e[::-1], palette=pal1)

    # Run-length encoded sources.  The reader has to expand these, and the writer
    # has to put them back: a round trip is expected to reproduce the source's row
    # encoding, not just its pixels.
    #   flat rows, the simplest case
    rle = bytearray()
    for y in range(8):
        rle += bytes((16, y * 3))          # a run of 16 pixels
        rle += bytes((0, 0))               # end of line
    rle += bytes((0, 1))                   # end of bitmap
    write_bmp(p("rle8.bmp"), 16, 8, 8, None, palette=pal8, compression=BI_RLE8,
              pixel_data=bytes(rle))

    #   runs and literal stretches together, at a width that is not a multiple of
    #   four, and with runs longer than the 255 a single opcode can carry
    rows = []
    for y in range(37):
        row = []
        x = 0
        while x < 300:
            if (x // 7 + y) % 3:
                n = min(300 - x, 4 + (x + y) % 260)    # a run, sometimes over 255
                row += [(x + y * 5) % 256] * n
                x += n
            else:
                n = min(300 - x, 5 + (y % 4))          # a literal stretch
                row += [(x + k * 31 + y) % 256 for k in range(n)]
                x += n
        rows.append(row)
    write_bmp(p("rle8mix.bmp"), 300, 37, 8, None, palette=pal8, compression=BI_RLE8,
              pixel_data=rle8_encode(rows))

    #   the four bit encoding, which pairs pixels into every opcode.  Long runs,
    #   so the encoding is actually smaller than storing the rows -- the writer
    #   falls back to stored rows when it is not, and then there is no RLE4 to test.
    rows4 = [[(x // 17 + y // 3) % 16 for x in range(90)] for y in range(21)]
    write_bmp(p("rle4.bmp"), 90, 21, 4, None, palette=pal4, compression=BI_RLE4,
              pixel_data=rle4_encode(rows4))

    names = sorted(os.listdir(out_dir))
    print("wrote %d files into %s" % (len(names), out_dir))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "test/bmp")
