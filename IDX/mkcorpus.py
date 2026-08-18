#!/usr/bin/env python3
# Builds the tuning corpus in opt/ out of testfiles/.
#
# opt.pl re-runs the whole corpus for every bit it flips, so the corpus has to
# be small enough that one measurement is about a second -- but it still has to
# cover every model, or the untouched one drifts while nothing measures it
# (opt.pl's own header says so).  These five crops are two 8-bit, one 24-bit and
# two 32-bit, cut from the middle of the corpus images.
#
#   python3 IDX/mkcorpus.py && perl IDX/opt.pl opt.lst
import struct, os, sys

def read(fn):
    d = open(fn, 'rb').read()
    off  = struct.unpack_from('<I', d, 10)[0]
    w    = struct.unpack_from('<i', d, 18)[0]
    h    = struct.unpack_from('<i', d, 22)[0]
    bpp  = struct.unpack_from('<H', d, 28)[0]
    comp = struct.unpack_from('<I', d, 30)[0]
    return d, off, w, h, bpp, comp

def unrle8(d, off, w, h):
    img = bytearray(w*h)
    s, i, x, row = d[off:], 0, 0, 0
    while i+2 <= len(s):
        n, v = s[i], s[i+1]
        if n:
            img[(h-1-row)*w+x:(h-1-row)*w+x+n] = bytes([v])*n; x += n; i += 2
        elif v == 0: x, row, i = 0, row+1, i+2
        elif v == 1: break
        elif v == 2: x += s[i+2]; row += s[i+3]; i += 4
        else:
            img[(h-1-row)*w+x:(h-1-row)*w+x+v] = s[i+2:i+2+v]
            x += v; i += 2 + ((v+1) & ~1)
    return bytes(img)

def crop(src, dst, cw, ch):
    d, off, w, h, bpp, comp = read(src)
    if comp == 1:                      # decode RLE to a flat 8-bit image first
        flat, bpp8 = unrle8(d, off, w, h), 1
    else:
        stride = ((w*bpp+31)//32)*4
        bpp8 = bpp//8
        rows = [d[off+r*stride : off+r*stride+w*bpp8] for r in range(h)][::-1]
        flat = b''.join(rows)
    cw, ch = min(cw, w), min(ch, h)
    x0, y0 = (w-cw)//2, (h-ch)//2
    ns = ((cw*bpp+31)//32)*4
    body = b''.join(
        (flat[((y0+ch-1-r)*w + x0)*bpp8 : ((y0+ch-1-r)*w + x0+cw)*bpp8]).ljust(ns, b'\0')
        for r in range(ch))
    hdr = bytearray(d[:off])
    struct.pack_into('<I', hdr,  2, off+len(body))
    struct.pack_into('<I', hdr, 10, off)
    struct.pack_into('<i', hdr, 18, cw)
    struct.pack_into('<i', hdr, 22, ch)
    struct.pack_into('<I', hdr, 30, 0)          # the crop is never RLE
    struct.pack_into('<I', hdr, 34, len(body))
    open(dst, 'wb').write(bytes(hdr) + body)
    print("%-16s %dx%d %dbpp  %d bytes" % (dst, cw, ch, bpp, off+len(body)))

WANT = [("t8g.bmp", "opt/t8g.bmp", 200, 150),
        ("t24.bmp", "opt/t24.bmp", 176, 132),
        ("t32.bmp", "opt/t32.bmp", 176, 132),
        ("x_ci.bmp", "opt/o_ci.bmp", 640, 400),
        ("x_ep.bmp", "opt/o_ep.bmp", 176, 176)]

os.makedirs("opt", exist_ok=True)
missing = [s for s, _, _, _ in WANT if not os.path.exists("testfiles/"+s)]
if missing:
    sys.exit("missing from testfiles/: " + " ".join(missing))
lst = []
for s, dst, cw, ch in WANT:
    crop("testfiles/"+s, dst, cw, ch)
    lst.append(dst)
open("opt.lst", "w").write("\n".join(lst) + "\n")
print("wrote opt.lst")
