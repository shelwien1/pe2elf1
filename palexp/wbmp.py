# Writing the synthetic BMPs the experiments compress: an 8bpp file with an
# identity grey-ramp palette (so bmg takes its index path rather than expanding),
# and a 24bpp file from three planes.

import struct, numpy as np

def write8(fn, idx):
    """8bpp bottom-up BMP, identity grey-ramp palette, pixels = idx (H,W uint8)."""
    Hh, Ww = idx.shape
    stride = ((Ww + 3)//4)*4
    rows = np.zeros((Hh, stride), np.uint8)
    rows[:, :Ww] = idx
    body = rows[::-1].tobytes()
    pal = bytearray()
    for i in range(256): pal += bytes([i,i,i,0])
    off = 14 + 40 + 1024
    hdr = b'BM' + struct.pack('<IHHI', off+len(body), 0, 0, off)
    dib = struct.pack('<IiiHHIIiiII', 40, Ww, Hh, 1, 8, 0, len(body), 2835, 2835, 256, 0)
    open(fn,'wb').write(hdr + dib + bytes(pal) + body)

def write24(fn, planes):
    """24bpp bottom-up BMP from three (H,W) uint8 planes, written as B,G,R."""
    Hh, Ww = planes[0].shape
    stride = ((Ww*3 + 3)//4)*4
    rows = np.zeros((Hh, stride), np.uint8)
    for c in range(3): rows[:, c:Ww*3:3] = planes[c]
    body = rows[::-1].tobytes()
    off = 14 + 40
    hdr = b'BM' + struct.pack('<IHHI', off+len(body), 0, 0, off)
    dib = struct.pack('<IiiHHIIiiII', 40, Ww, Hh, 1, 24, 0, len(body), 2835, 2835, 0, 0)
    open(fn,'wb').write(hdr + dib + body)
