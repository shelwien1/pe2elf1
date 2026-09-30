"""Tokenize enwik8 like cmix's word transform and cache:
  candidates.pkl  words by descending count, ties alphabetical (dicrank's order), and counts
  ids8.npy        candidate rank of every token
  page8.npy       index of the <page> each token is in
  case8.npy       0 = lowercase, 1 = capitalized, 2 = all capitals
"""
import collections
import pickle
import re

import numpy as np

from common import ENWIK8, TOKEN, work

text = open(ENWIK8, 'rb').read()
toks, pos, case = [], [], []
for m in TOKEN.finditer(text):
    g = m.group()
    toks.append(g.lower())
    pos.append(m.start())
    case.append(0 if g[0] > 90 else (2 if len(g) > 1 and g[1] <= 90 else 1))
count = collections.Counter(toks)
order = sorted(count.items(), key=lambda kv: (-kv[1], kv[0]))
words = [w for w, _ in order]
pickle.dump((words, [n for _, n in order]), open(work('candidates.pkl'), 'wb'))
rank = {w: i for i, w in enumerate(words)}
page_starts = np.array([m.start() for m in re.finditer(rb'<page>', text)])
np.save(work('ids8.npy'), np.array([rank[t] for t in toks], dtype=np.int32))
np.save(work('page8.npy'), (np.searchsorted(page_starts, np.array(pos), side='right') - 1).astype(np.int32))
np.save(work('case8.npy'), np.array(case, dtype=np.int8))
print(f'{len(toks):,} tokens, {len(words):,} distinct words, {len(page_starts):,} pages')
