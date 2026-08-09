#!/usr/bin/env python3
"""Write the two run-length-encoded BMPs in testfiles/.

    python3 tools/mkrle.py testfiles

`read_bmp` has a whole run-length decoder in it — encoded runs, absolute runs,
the end-of-line and end-of-bitmap escapes, the delta escape, and a separate
nibble path for RLE4 — and the ten images the gate started with reached none of
it: 101 of its lines never ran.  These two bring that to 48.

`rle8.bmp` uses encoded and absolute runs and both escapes.  `rle4.bmp` uses
the nibble path, and every fourth row stops eight pixels early and jumps to the
next with a delta — which is the only way to reach that escape, since an
encoder with nothing to skip never emits one.

Both images are deliberately blocky, so runs are long enough to encode as runs
while the boundaries still leave short spans that have to go through absolute
mode.

BMF does not reproduce either file byte for byte, and that is not a defect: it
decodes the runs on the way in and re-encodes them on the way out with its own
run splitting.  The pixels match; the files differ.  `testfiles/out_<name>.bmp`
holds what the decoder is expected to write, and `test.sh` compares against
that when it is present.
"""
import struct
import sys

W, H = 64, 48


def head(w, h, bpp, comp, data, pal):
    off = 54 + len(pal)
    return (struct.pack('<2sIHHI', b'BM', off + len(data), 0, 0, off)
            + struct.pack('<IiiHHIIiiII', 40, w, h, 1, bpp, comp, len(data),
                          0, 0, len(pal) // 4, 0)
            + pal + data)


def runs(row, stop, pack):
    """One row as encoded and absolute runs, in whichever nibble packing."""
    out, x, least = bytearray(), 0, 4 if pack else 3
    while x < stop:
        n = 1
        while x + n < stop and row[x + n] == row[x] and n < 255:
            n += 1
        if n >= least:
            out += bytes((n, (row[x] << 4) | row[x] if pack else row[x]))
            x += n
            continue
        lit = []
        while x < stop and len(lit) < 254:
            r = 1
            while x + r < stop and row[x + r] == row[x] and r < least + 1:
                r += 1
            if r >= least:
                break
            lit.append(row[x])
            x += 1
        if len(lit) < 3:                       # absolute mode needs three
            for v in lit:
                out += bytes((1, (v << 4) | v if pack else v))
        elif pack:
            b = bytearray()
            for i in range(0, len(lit), 2):
                b.append((lit[i] << 4) | (lit[i + 1] if i + 1 < len(lit) else 0))
            while len(b) % 2:
                b.append(0)
            out += bytes((0, len(lit))) + bytes(b)
        else:
            out += bytes((0, len(lit))) + bytes(lit)
            if len(lit) & 1:
                out += b'\0'                   # padded to a word
    return out


def stream(index, pack, delta):
    out = bytearray()
    for y in range(H - 1, -1, -1):
        row = [index(x, y) for x in range(W)]
        # An odd skip is the point of this: it leaves the write position on a
        # half byte, so the absolute-run decoder takes its `v22` path -- the
        # one that carries a nibble across from the previous byte.  An even
        # skip never reaches it, which is what the first version of this did.
        stop = W - (7 if y % 4 == 0 else 8) if delta else W
        out += runs(row, stop, pack)
        if stop != W:
            out += bytes((0, 2, W - stop, 0))  # delta: skip right, same row
        out += b'\0\0'                         # end of line
    return bytes(out + b'\0\1')                # end of bitmap


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else 'testfiles'
    grey = b''.join(bytes((i * 17, i * 17, i * 17, 0)) for i in range(16))
    colour = b''.join(bytes((i * 17, 255 - i * 17, i * 9, 0)) for i in range(16))

    s8 = stream(lambda x, y: ((x // 8) + (y // 6)) % 16, False, False)
    open('%s/rle8.bmp' % d, 'wb').write(head(W, H, 8, 1, s8, grey))
    print('%s/rle8.bmp: %dx%d, 8bpp RLE8, %d bytes of runs' % (d, W, H, len(s8)))

    s4 = stream(lambda x, y: ((x // 5) + (y // 7)) % 16, True, True)
    open('%s/rle4.bmp' % d, 'wb').write(head(W, H, 4, 2, s4, colour))
    print('%s/rle4.bmp: %dx%d, 4bpp RLE4 with deltas, %d bytes of runs'
          % (d, W, H, len(s4)))


if __name__ == '__main__':
    main()
