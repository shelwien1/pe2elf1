"""Tokenize all of enwik9, mapping words to enwik8 candidate ranks (words that
enwik8 lacks become the sentinel len(words)). Caches ids9.npy."""
import pickle

import numpy as np

from common import ENWIK9, TOKEN, work

words, _ = pickle.load(open(work('candidates.pkl'), 'rb'))
rank = {w: i for i, w in enumerate(words)}
N = len(words)
parts, rest = [], b''
with open(ENWIK9, 'rb') as f:
    while True:
        chunk = f.read(64 << 20)
        data = rest + chunk
        if chunk:  # cut at the last newline; words never span one
            cut = data.rfind(b'\n') + 1
            data, rest = data[:cut], data[cut:]
        toks = TOKEN.findall(data)
        parts.append(np.fromiter((rank.get(t.lower(), N) for t in toks), dtype=np.int32, count=len(toks)))
        if not chunk:
            break
ids9 = np.concatenate(parts)
np.save(work('ids9.npy'), ids9)
print(f'{len(ids9):,} tokens, {(ids9 == N).mean():.2%} not in the enwik8 word list')
