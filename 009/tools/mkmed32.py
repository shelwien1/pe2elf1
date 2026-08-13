#!/usr/bin/env python3
"""Write testfiles/med32.bmp — the image that reaches `unpredict_med`.

    python3 tools/mkmed32.py testfiles/med32.bmp

The ten images the gate started with never took one path through the decoder:
`unpredict_med`, the inverse of the MED prediction, had 127 unexecuted lines
and no way to get any of them run.  The reason is in REFACTORING.md §2.3 — every
plane in that corpus whose predictor is MED also has `plane_alt_model` set, and
the alternate family inverts the prediction itself.

Reaching it needs a plane that is *not* split into 8-bit planes, so that the
descriptor's depth stays 32, and whose content makes the filter search pick MED
without the alternate model.  Both fall out of one choice: four identical
channels (which is why the split is skipped) carrying `x*x/256 + y`, a gradient
that curves — flat enough for MED to win, not so flat that no prediction wins.

The DPI fields are zero because that is what BMF writes, and the gate compares
the recovered file to this one byte for byte.
"""
import struct
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import outpath                                                    # noqa: E402

W = H = 192


def main():
    path = outpath.bmp(sys.argv, 'testfiles/med32.bmp')
    px = bytearray()
    for y in range(H - 1, -1, -1):
        for x in range(W):
            px += bytes(((x * x // 256 + y) & 255,) * 4)
    open(path, 'wb').write(
        struct.pack('<2sIHHI', b'BM', 54 + len(px), 0, 0, 54)
        + struct.pack('<IiiHHIIiiII', 40, W, H, 1, 32, 0, len(px), 0, 0, 0, 0)
        + bytes(px))
    print('%s: %dx%d, 32bpp, %d bytes' % (path, W, H, 54 + len(px)))


if __name__ == '__main__':
    main()
