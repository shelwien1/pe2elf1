# A BMP reader for the depths bmg accepts -- 8bpp BI_RGB and BI_RLE8 expanded
# through the palette, 24bpp and 32bpp -- returning the pixels as a numpy array
# in BMP channel order (B,G,R[,A]) plus, for 8-bit files, the stored index plane.

import struct, numpy as np

def load(fn):
    d = open(fn,'rb').read()
    assert d[:2]==b'BM'
    off, dib = struct.unpack_from('<II', d, 10)
    W, sh = struct.unpack_from('<ii', d, 18)
    bpp, comp = struct.unpack_from('<HI', d, 28)
    clr = struct.unpack_from('<I', d, 46)[0]
    topdown = sh < 0
    H = -sh if topdown else sh
    pal = None
    if bpp <= 8:
        n = clr if clr else (1<<bpp)
        pt = np.frombuffer(d, np.uint8, n*4, 14+dib).reshape(n,4)
        pal = pt[:,:3].astype(np.int32)          # B,G,R
    if bpp == 8 and comp == 1:                    # BI_RLE8
        img = np.zeros((H,W), np.uint8); i = off; x = y = 0
        while i+1 < len(d):
            a, b = d[i], d[i+1]
            if a:
                img[y, x:x+a] = b; x += a; i += 2
            elif b == 0: x = 0; y += 1; i += 2
            elif b == 1: break
            elif b == 2: x += d[i+2]; y += d[i+3]; i += 4
            else:
                img[y, x:x+b] = np.frombuffer(d, np.uint8, b, i+2); x += b
                i += 2 + b + ((b & 1))
        idx = img if topdown else img[::-1]
        px = pal[idx]                             # H,W,3 as B,G,R
        return W, H, px.astype(np.uint8), pal, 8, idx.astype(np.uint8)
    stride = ((W*bpp + 31)//32)*4
    raw = np.frombuffer(d, np.uint8, stride*H, off).reshape(H, stride)
    if not topdown: raw = raw[::-1]
    if bpp == 8:
        idx = raw[:, :W]
        px = pal[idx].astype(np.uint8)
        return W, H, px, pal, 8, idx.copy()
    npx = bpp//8
    px = raw[:, :W*npx].reshape(H, W, npx)
    return W, H, px.copy(), None, bpp, None
