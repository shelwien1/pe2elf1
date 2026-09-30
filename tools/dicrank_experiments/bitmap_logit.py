"""Membership bitmap sorted by a small logistic model instead of BitmapKey
(docs/english_dic_reconstruction.md §9, "What might pay").

Reads a dump written by "dicrank3 e -D DUMP ..." (bitmap.bin, words.txt),
fits a 17-weight logistic model of dictionary membership on case, length,
document frequency, count and rank, and writes the bits in BitmapKey order
(bitmap_key.txt) and sorted by the model's probability (bitmap_logit.txt).
Compress both with cmix to compare.

usage: python3 bitmap_logit.py DUMP
"""
import struct
import sys

import numpy as np

dump = sys.argv[1]
words = [line.split() for line in open(dump + '/words.txt')]
b = open(dump + '/bitmap.bin', 'rb').read()
n_head = struct.unpack_from('<I', b, 0)[0]
pos = 4 + n_head
n = struct.unpack_from('<I', b, pos)[0]
pos += 4
ranks = np.frombuffer(b, '<u4', n, pos)
bits = np.frombuffer(b, 'u1', n, pos + 4 * n)


def ilog2(x):
    return int(x).bit_length() - 1


rows = []
for r in ranks:
    w, c, d, low = words[r][0], int(words[r][1]), int(words[r][2]), int(words[r][3])
    case = min(4, 5 * low // c) if c else 0
    share = min(15, 16 * low // c) if c else 0
    lq, dq, cq = min(len(w), 12) // 3, ilog2(d + 1), ilog2(c + 1)
    rows.append((lq, dq, cq, case, share, max(0, min(7, cq - dq)), len(w), r))
F = np.array(rows)
X = np.concatenate([np.eye(5)[F[:, 3]], np.eye(5)[F[:, 0]],
                    F[:, [1, 2, 5, 6, 4]].astype(float), np.log2(F[:, 7:8] + 1.0)], axis=1)
X = (X - X.mean(0)) / (X.std(0) + 1e-9)
y = bits.astype(float)
w, w0 = np.zeros(X.shape[1]), 0.0
for _ in range(3000):
    p = 1 / (1 + np.exp(-(X @ w + w0)))
    w -= 0.5 * (X.T @ (p - y)) / len(y)
    w0 -= 0.5 * (p - y).mean()
p = 1 / (1 + np.exp(-(X @ w + w0)))
pc = np.clip(p, 1e-6, 1 - 1e-6)
cost = -(y * np.log2(pc) + (1 - y) * np.log2(1 - pc)).sum() / 8
print(f'logistic model: {X.shape[1] + 1} weights, static code length {cost:,.0f} bytes')
order = np.lexsort((np.arange(n), -p))  # descending probability, ties in BitmapKey order
open('bitmap_key.txt', 'w').write(''.join(str(int(x)) for x in bits) + '\n')
open('bitmap_logit.txt', 'w').write(''.join(str(int(bits[i])) for i in order) + '\n')
