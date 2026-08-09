#!/usr/bin/env python3
"""Write testfiles/noise24.bmp — data that does not compress.

    python3 tools/mknoise.py testfiles/noise24.bmp

`ALGORITHM.md` §2: "When the data does not compress (coded size >= raw size)
the member is rewritten with the descriptor's compressed flag clear and the raw
pixels in place of the coded data."  Nothing in the corpus took that path —
every image in it compresses — so the fallback, and the decoder's branch for
reading a member back out of it, had never run.

Uniform noise takes it: `bmf c` reports 24.000 bpp on a 24-bit image, which is
the stream saying it stored the pixels rather than coded them.

The generator is a 64-bit linear congruential sequence written out longhand
rather than `random`, so the file is the same on every Python.
"""
import struct
import sys

W = H = 128


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'testfiles/noise24.bmp'
    state = [20250809]

    def byte():
        state[0] = (state[0] * 6364136223846793005 + 1442695040888963407) \
            & ((1 << 63) - 1)
        return (state[0] >> 33) & 255

    px = bytearray()
    for _ in range(H):
        for _ in range(W):
            px += bytes((byte(), byte(), byte()))
        while len(px) % 4:
            px.append(0)
    open(path, 'wb').write(
        struct.pack('<2sIHHI', b'BM', 54 + len(px), 0, 0, 54)
        + struct.pack('<IiiHHIIiiII', 40, W, H, 1, 24, 0, len(px), 0, 0, 0, 0)
        + bytes(px))
    print('%s: %dx%d, 24bpp, %d bytes' % (path, W, H, 54 + len(px)))


if __name__ == '__main__':
    main()
