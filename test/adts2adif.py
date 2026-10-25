#!/usr/bin/env python3
"""adts2adif.py in.aac out.adif [bitstream_type] [comment] [raw] : ADTS -> ADIF (one PCE), or with 'raw' -> back-to-back
raw access units (prints the ASC hex).
(aac2tsv's tests/adts2adif.py, its ADTS frames and bit writer in adts.py.)"""
import sys
from adts import BW, frames
d = open(sys.argv[1], 'rb').read(); out = sys.argv[2]
bt = int(sys.argv[3]) if len(sys.argv) > 3 else 1
comment = sys.argv[4].encode() if len(sys.argv) > 4 else b''
raw = len(sys.argv) > 5
fr = list(frames(d))
if not fr: sys.exit('adts2adif.py: no ADTS frame in %s' % sys.argv[1])
hdr = fr[0][0]; blocks = [b for _, b in fr]
prof = hdr[2] >> 6; sfi = (hdr[2] >> 2) & 15; ch = ((hdr[2] & 1) << 2) | (hdr[3] >> 6)
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
