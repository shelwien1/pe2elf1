#!/usr/bin/env python3
"""Write the incdec corpus besides book1: deterministic, so the gate's references never move.

  mixed.bin  200 KB: runs, skewed bytes, repeated phrases and noise, in blocks
  rep.bin    150 KB: one 3 KB block repeated, then mutated copies (long matches)
"""
import sys, os
out = sys.argv[1] if len(sys.argv) > 1 else '.'
os.makedirs(out, exist_ok=True)
x = 0x12345678
def rnd():
    global x
    x = (x * 1103515245 + 12345) & 0xFFFFFFFF
    return x >> 16
b = bytearray()
words = [b'model', b'context', b'archive', b'volume', b'\x00\x01\x02', b'0123456789', b'VBN']
while len(b) < 200 * 1024:
    k = rnd() % 4
    if k == 0: b += bytes([rnd() & 0xFF]) * (rnd() % 40 + 1)
    elif k == 1: b += bytes((rnd() % 16) * (rnd() % 16) & 0xFF for _ in range(rnd() % 300))
    elif k == 2: b += words[rnd() % len(words)] * (rnd() % 8 + 1)
    else: b += bytes(rnd() & 0xFF for _ in range(rnd() % 64))
open(os.path.join(out, 'mixed.bin'), 'wb').write(bytes(b[:200 * 1024]))
blk = bytes((rnd() >> 3) & 0x7F for _ in range(3072))
r = bytearray(blk * 20)
for i in range(30):
    c = bytearray(blk)
    for j in range(rnd() % 20): c[rnd() % len(c)] = rnd() & 0xFF
    r += c
open(os.path.join(out, 'rep.bin'), 'wb').write(bytes(r[:150 * 1024]))
