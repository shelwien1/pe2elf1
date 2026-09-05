#!/usr/bin/env python3
"""Generate the crafted JPEGs used as reproducers in docs/pjpg-algorithm.md.

    python3 docs/make-repros.py [outdir]      # default: ./repro
    for f in repro/*.jpg; do 011_/pjpg "$f" >/dev/null 2>&1; echo "$? $f"; done

Expected against a plain `make` build (g++ 13.3 -O2, x86-64):
    139 (SIGSEGV) for sos_overflow.jpg and sos_n22.jpg
    0 for everything else -- the remaining bugs are silent corruption,
      output amplification or misparses, not crashes.
"""
import os, struct, sys

def seg(marker, payload):
    return bytes([0xFF, marker]) + struct.pack('>H', len(payload) + 2) + payload

SOI, EOI = b'\xFF\xD8', b'\xFF\xD9'
SOF1 = bytes([8]) + struct.pack('>HH', 16, 16) + bytes([1]) + bytes([0x00, 0x11, 0x00])

def exif(align, body):
    mark = b'\x2a\x00' if align == b'II' else b'\x00\x2a'
    ofs  = struct.pack('<I', 8) if align == b'II' else struct.pack('>I', 8)
    return b'Exif\0\0' + align + mark + ofs + body

out = sys.argv[1] if len(sys.argv) > 1 else 'repro'
os.makedirs(out, exist_ok=True)
def w(name, data):
    open(os.path.join(out, name), 'wb').write(data)
    print('%-22s %7d bytes' % (name, len(data)))

# 6.1  SOS declares 22 components -> cur_comp_info[21] overwrites pjpg0::tag_id -> SIGSEGV
w('sos_overflow.jpg', SOI + seg(0xC0, SOF1)
  + seg(0xDA, bytes([22]) + bytes([0x00, 0x00]) * 22 + bytes([0, 63, 0])) + EOI)
# ... and the one below the threshold, which must NOT crash
w('sos_n21.jpg', SOI + seg(0xC0, SOF1)
  + seg(0xDA, bytes([21]) + bytes([0x00, 0x00]) * 21 + bytes([0, 63, 0])) + EOI)

# 6.2  DHT with bits[1..16] all 255 -> count 4080 written into byte[256]
#      SOF first so the overrun lands on a field you can watch (comp_info[0].component_id)
w('dht_overflow.jpg', SOI
  + seg(0xC0, bytes([8]) + struct.pack('>HH', 16, 16) + bytes([1]) + bytes([0x42, 0x11, 0x00]))
  + seg(0xC4, bytes([0x00]) + bytes([255] * 16) + bytes([0xEE]) * 4080) + EOI)

# 6.3  DQT with a bogus table index -> one output line per remaining payload byte
w('dqt_amplify.jpg', SOI + seg(0xDB, bytes([0x0F]) * 65001) + EOI)

# 6.4  SOF3 is missing from len2[], so its payload is scanned as data and the
#      FF C4 inside it is parsed as a real DHT marker
w('sof3_misparse.jpg', SOI
  + seg(0xC3, bytes([8]) + struct.pack('>HH', 16, 16) + bytes([1]) + bytes([0, 0x11, 0])
        + b'\xFF\xC4\x00\x05\x00\x00\x00') + EOI)

# 6.5  two images; the second has no SOF and its SOS is parsed against the first frame
SOF3C = bytes([8]) + struct.pack('>HH', 16, 16) + bytes([3]) + bytes([1, 0x11, 0, 2, 0x11, 1, 3, 0x11, 1])
w('two_images.jpg',
  SOI + seg(0xC0, SOF3C) + seg(0xDA, bytes([3]) + bytes([1, 0, 2, 0x11, 3, 0x11]) + bytes([0, 63, 0])) + EOI
  + SOI + seg(0xDA, bytes([1]) + bytes([7, 0x00]) + bytes([0, 63, 0])) + EOI)

# 6.6  align field "IM"/"MI" prints reversed because of the (word&)d[6] pun
for a in (b'II', b'MM', b'IM', b'MI'):
    w('align_%s.jpg' % a.decode(),
      SOI + seg(0xE1, exif(a, struct.pack('<H', 0) + b'\0\0\0\0')) + EOI)

# 6.7  thumbOfs + thumbLen wraps 32 bits and passes the <= len check
ent = lambda t, f, c, v: struct.pack('<HHII', t, f, c, v)
w('thumb_wrap.jpg', SOI + seg(0xE1, exif(b'II',
    struct.pack('<H', 2) + ent(0x0201, 4, 1, 0xFFFFFFF0) + ent(0x0202, 4, 1, 0x20)
    + struct.pack('<I', 0))) + EOI)

# §7  degenerate inputs that must all exit 0
w('empty.jpg', b'')
w('one_byte.jpg', b'\xFF')
w('not_a_jpeg.jpg', b'hello world, not a jpeg at all')
full = SOI + seg(0xE1, exif(b'II', b'\x00' * 2000))
w('truncated.jpg', full[:len(full) // 2])
