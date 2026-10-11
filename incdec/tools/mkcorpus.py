#!/usr/bin/env python3
"""Write the incdec corpus besides book1: deterministic, so the gate's references never move.

  mixed.bin  200 KB: runs, skewed bytes, repeated phrases and noise, in blocks
  rep.bin    150 KB: one 3 KB block repeated, then mutated copies (long matches)
  big.bin    2.36 MB: book1, a copy with every 997th byte changed, mixed.bin and
             book1's first 600 KB.  More than the 0x198000 bytes the coder holds
             at once, so `u` and `r` slide the window (sub_100050F8).
  tree/      five files in two levels of subdirectories, for `u archive *.*`:
             several files (sorted with qsort), recursion, and a restore that
             has to create the directories.
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
book1 = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'input.txt'), 'rb').read()
mut = bytearray(book1)
for i in range(0, len(mut), 997):
    mut[i] = (mut[i] + 1 + (rnd() & 0x3F)) & 0xFF
open(os.path.join(out, 'big.bin'), 'wb').write(book1 + bytes(mut) + bytes(b[:200 * 1024]) + book1[:600 * 1024])
exe = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'vbn.exe'), 'rb').read()
tree = {'a.txt': book1[:120 * 1024], 'b.bin': bytes(r[:60 * 1024]), 'sub/c.dat': bytes(b[:40 * 1024]),
        'sub/e.txt': book1[300 * 1024:380 * 1024], 'sub/deep/d.exe': exe[:24 * 1024]}
for name, data in tree.items():
    path = os.path.join(out, 'tree', name)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(data)
