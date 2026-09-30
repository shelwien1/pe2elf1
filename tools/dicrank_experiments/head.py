"""Order of the 3,920 explicitly ordered words: each next word predicted from
the previous two by context similarity. Saves the rank symbols for build_v2.py."""
import math

import numpy as np

from common import adaptive_cost, head_order, load, ppmi, svd_vectors, work

d = load()
words, rank, head = d['words'], d['rank'], d['head']
hr = [rank[w] for w in head]
n = len(hr)
print(f'random order would cost log2({n}!) = {math.lgamma(n + 1) / math.log(2) / 8:,.0f} bytes')
print(f'consecutive words sharing the first 4 letters: '
      f'{sum(1 for a, b in zip(head, head[1:]) if a[:4] == b[:4]) / (n - 1):.1%}')
for label, ids in (('enwik8', d['ids']), ('enwik9', np.load(work('ids9.npy')))):
    V = svd_vectors(ppmi(ids, hr, len(words), K=4000), 200)
    soft, syms = head_order(V, beta=10.0, prev2=0.5)
    print(f'{label} vectors: softmax {soft:,.0f} bytes; rank-coded '
          f'{adaptive_cost(syms, n) + math.log2(n) / 8:,.0f} bytes', flush=True)
np.save(work('head_rank_e9.npy'), np.array(syms))
