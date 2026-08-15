#!/usr/bin/env python3
# bmpcrop.py -- cut a BMP down to its first N stored rows, fixing the three
# header fields that describe the size.  Used to build the optimizer corpus
# (IDX/opt.lst) and sweep.py's stand-in for the 8 MB image: a full pass over
# the real files costs ~20 s, which is too slow for a bit-flipping search.
#
#   tools/bmpcrop.py in.bmp out.bmp 128
import struct, sys
d = bytearray(open(sys.argv[1], 'rb').read())
out_path, rows = sys.argv[2], int(sys.argv[3])
off = struct.unpack_from('<I', d, 10)[0]
W   = struct.unpack_from('<i', d, 18)[0]
H   = struct.unpack_from('<i', d, 22)[0]
bpp = struct.unpack_from('<H', d, 28)[0]
stride = ((W * bpp + 31) // 32) * 4
rows = min(rows, abs(H))
out = bytearray(d[:off])
struct.pack_into('<i', out, 22, rows if H > 0 else -rows)
struct.pack_into('<I', out, 2, off + stride * rows)
struct.pack_into('<I', out, 34, stride * rows)
out += d[off:off + stride * rows]
open(out_path, 'wb').write(out)
print('%s %dx%d %dbpp -> %s (%d bytes)' % (sys.argv[1], W, rows, bpp, out_path, len(out)))
