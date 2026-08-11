#!/usr/bin/env python3
"""Write testfiles/altp1.bmp — the image that reaches `alt_model_p1_decode`.

    python3 tools/mkaltp1.py testfiles/altp1.bmp

Of the four alternate model families, this was the one nothing reached: 268
unexecuted lines, the largest untested body in the file.  It needs a plane
coded with predictor 1 *and* the alternate model at a depth other than 8, and
an earlier search across 35 images — 4-, 24- and 32-bit, identical and
independent channels, smooth, noisy and structured — never produced that
combination.  It concluded, wrongly, that the combination looked out of reach.

The region it had missed is three identical channels carrying one function and
a *fourth* carrying a different one.  `x*x/3 + y` in the colour channels keeps
the plane out of the 8-bit split, so the descriptor's depth stays 32, and makes
MED the best predictor; `(x + y)` in the fourth is what tips the search into
the alternate model as well.

What that body did when it finally ran is in REFACTORING.md §6: it had been
broken since Phase 2 and no test could see it.
"""
import struct
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import outpath                                                    # noqa: E402

W = H = 96


def main():
    path = outpath.bmp(sys.argv, 'testfiles/altp1.bmp')
    px = bytearray()
    for y in range(H - 1, -1, -1):
        for x in range(W):
            v = ((x * x) // 3 + y) & 255
            px += bytes((v, v, v, (x + y) & 255))
        while len(px) % 4:
            px.append(0)
    open(path, 'wb').write(
        struct.pack('<2sIHHI', b'BM', 54 + len(px), 0, 0, 54)
        + struct.pack('<IiiHHIIiiII', 40, W, H, 1, 32, 0, len(px), 0, 0, 0, 0)
        + bytes(px))
    print('%s: %dx%d, 32bpp, %d bytes' % (path, W, H, 54 + len(px)))


if __name__ == '__main__':
    main()
