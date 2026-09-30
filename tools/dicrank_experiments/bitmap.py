"""Membership bitmap of the alphabetically-run words: sort keys, a context
model, and ordering the bits by the context features instead."""
import collections
import lzma
import math

import numpy as np

from common import load, work

d = load()
words, rank, run_of, ids = d['words'], d['rank'], d['run_of'], d['ids']
N = len(words)
page, case = np.load(work('page8.npy')), np.load(work('case8.npy'))
cnt = np.bincount(ids, minlength=N)
low = np.bincount(ids[case == 0], minlength=N)
df = np.bincount((np.unique(page.astype(np.int64) * N + ids) % N).astype(np.int64), minlength=N)
share = low / np.maximum(cnt, 1)
head = set(rank[w] for w in d['head'])
member = np.zeros(N, bool)
for w in run_of:
    member[rank[w]] = True


def windowed_kt(bits, window=2000):
    cost, hist, ones = 0.0, collections.deque(), 0
    for b in bits:
        p1 = (ones + 0.5) / (len(hist) + 1.0)
        cost -= math.log2(p1 if b else 1 - p1)
        hist.append(b)
        ones += b
        if len(hist) > window:
            ones -= hist.popleft()
    return cost / 8


def candidates(sortkey):
    order = np.lexsort((np.arange(N), -sortkey))
    seq = [r for r in order if r not in head]
    last = max(i for i, r in enumerate(seq) if member[r])
    return seq[:last + 1]


for name, key in (('count (dicrank)', cnt), ('document frequency', df), ('lowercase count', low)):
    seq = candidates(key.astype(float))
    print(f'sort by {name:20s}: {len(seq):>7,} bits, adaptive {windowed_kt([member[r] for r in seq]):,.0f} bytes')

seq = candidates(cnt.astype(float))
bits = [int(member[r]) for r in seq]
features = lambda r: (min(len(words[r]), 12) // 3, int(math.log2(df[r] + 1)),
                      int(math.log2(cnt[r] + 1)), int(share[r] * 4.999))
table, cost = collections.defaultdict(lambda: [0.4, 0.4]), 0.0
for b, r in zip(bits, seq):
    t = table[features(r)]
    cost -= math.log2(t[b] / (t[0] + t[1]))
    t[b] += 1
print(f'context model (length, log2 df, log2 count, lowercase share): {cost / 8:,.0f} bytes')
txt = ''.join(str(int(member[r])) for r in sorted(seq, key=lambda r: features(r) + (r,))).encode()
print(f'bits ordered by those features, xz: {len(lzma.compress(txt, preset=9 | lzma.PRESET_EXTREME)):,} bytes '
      f'(count order: {len(lzma.compress("".join(map(str, bits)).encode(), preset=9 | lzma.PRESET_EXTREME)):,})')
