#!/usr/bin/env python3
"""Build the planted-signature regression input.

A valid DDS whose PIXEL DATA contains the bytes "DDS ".  Real textures do this
by chance -- a 4096x2048 DXT3 in the wild carried two -- and a payload scan that
rejects on the bare signature throws the whole texture away.  Every other corpus
here has exactly one signature per texture, which is why this went unnoticed.
"""
import sys
src, dst = sys.argv[1], sys.argv[2]
a = bytearray(open(src, 'rb').read())
n = 0
for off in (len(a)//3, 2*len(a)//3):        # two, as the real file had
    if off + 4 <= len(a) and off >= 128:
        a[off:off+4] = b'DDS '; n += 1
open(dst, 'wb').write(b'HEAD' + bytes(a) + b'TAIL')
print("planted %d signature(s) in %s -> %s" % (n, src, dst))
