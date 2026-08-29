# make 4-bit-per-component versions of a BMP, in the two forms that occur:
#   step16  v = (v>>4)<<4     low nibble always zero      (bits are constant)
#   rep17   v = (v>>4)*17     4 bits replicated to 8      (no bit is constant)
#   idx16   v = (v>>4)        the 16 values themselves     (what it really is)
import struct, sys, numpy as np
src, mode, dst = sys.argv[1], sys.argv[2], sys.argv[3]
d = bytearray(open(src,'rb').read())
off = struct.unpack('<I', d[10:14])[0]
w,h = struct.unpack('<ii', d[18:26]); bpp = struct.unpack('<H', d[28:30])[0]
nc = bpp//8; st = ((w*bpp+31)//32)*4
a = np.frombuffer(bytes(d[off:off+st*abs(h)]), dtype=np.uint8).reshape(abs(h), st).copy()
p = a[:, :w*nc]
q = p >> 4
p[:] = {'step16': q<<4, 'rep17': q*17, 'idx16': q}[mode]
a[:, :w*nc] = p
d[off:off+st*abs(h)] = a.tobytes()
open(dst,'wb').write(bytes(d))
