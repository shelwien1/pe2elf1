#!/usr/bin/env python3
"""adts2adif.py in.aac out.adif [bitstream_type] [comment] [raw] : ADTS -> ADIF (one PCE), or with 'raw' -> back-to-back
raw access units (prints the ASC hex)."""
import sys
d = open(sys.argv[1], 'rb').read(); out = sys.argv[2]
bt = int(sys.argv[3]) if len(sys.argv) > 3 else 1
comment = sys.argv[4].encode() if len(sys.argv) > 4 else b''
raw = len(sys.argv) > 5
p = 0; blocks = []; hdr = None
while p + 7 <= len(d):
    if d[p] != 0xff or (d[p+1] & 0xf6) != 0xf0: p += 1; continue
    pa = d[p+1] & 1; flen = ((d[p+3] & 3) << 11) | (d[p+4] << 3) | (d[p+5] >> 5)
    if hdr is None: hdr = d[p:p+7]
    blocks.append(d[p + (7 if pa else 9):p + flen]); p += flen
prof = hdr[2] >> 6; sfi = (hdr[2] >> 2) & 15; ch = ((hdr[2] & 1) << 2) | (hdr[3] >> 6)
class BW:
    def __init__(s): s.b = []
    def put(s, v, n):
        for i in range(n - 1, -1, -1): s.b.append((v >> i) & 1)
    def align(s):
        while len(s.b) % 8: s.b.append(0)
    def bytes(s):
        s.align(); return bytes(int(''.join(map(str, s.b[i:i+8])), 2) for i in range(0, len(s.b), 8))
if raw:
    asc = BW(); asc.put(prof + 1, 5); asc.put(sfi, 4); asc.put(ch, 4); asc.put(0, 3)
    print(asc.bytes().hex()); open(out, 'wb').write(b''.join(blocks)); sys.exit(0)
w = BW()
for c in b'ADIF': w.put(c, 8)
w.put(0, 1); w.put(0, 1); w.put(0, 1); w.put(bt, 1); w.put(128000, 23); w.put(0, 4)
if bt == 0: w.put(12345, 20)
# PCE: front elements for the ADTS channel configuration
front = {1: [(0, 0)], 2: [(1, 0)]}.get(ch, [(1, 0)])
w.put(0, 4); w.put(prof, 2); w.put(sfi, 4); w.put(len(front), 4); w.put(0, 4); w.put(0, 4); w.put(0, 2); w.put(0, 3); w.put(0, 4)
w.put(0, 1); w.put(0, 1); w.put(0, 1)
for is_cpe, tag in front: w.put(is_cpe, 1); w.put(tag, 4)
w.align(); w.put(len(comment), 8)
for c in comment: w.put(c, 8)
open(out, 'wb').write(w.bytes() + b''.join(blocks))
