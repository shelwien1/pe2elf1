"""Assemble the prototype "v2" side file: every section stays text and cmix
does the coding. Needs runs.py and head.py to have run. Writes WORK/dict_rank_v2.txt
and one file per section. No v2 decoder exists yet; each section only uses
information a decoder with enwik8/enwik9 and the earlier sections would have."""
import math

import numpy as np

from common import load, work

d = load()
words, rank, run_of, R, ids = d['words'], d['rank'], d['run_of'], d['runs'], d['ids']
N = len(words)
page, case = np.load(work('page8.npy')), np.load(work('case8.npy'))
cnt = np.bincount(ids, minlength=N)
low = np.bincount(ids[case == 0], minlength=N)
df = np.bincount((np.unique(page.astype(np.int64) * N + ids) % N).astype(np.int64), minlength=N)
share = low / np.maximum(cnt, 1)
hr = [rank[w] for w in d['head']]
hs = set(hr)
sec = {}
# 1. which of the top ranks are explicitly ordered words
sec['head_set'] = ''.join('1' if r in hs else '0' for r in range(max(hr) + 1)) + '\n'
# 2. their order: first word's index in the set, then similarity ranks (head.py)
sec['head_order'] = f'{sorted(hr).index(hr[0])}\n' + ''.join(f'{s}\n' for s in np.load(work('head_rank_e9.npy')))
# 3. membership of the other words, bits ordered by (length, log2 df, log2 count, lowercase share)
member = np.zeros(N, bool)
for w in run_of:
    member[rank[w]] = True
cand = [r for r in range(N) if r not in hs]
cand = cand[:max(i for i, r in enumerate(cand) if member[r]) + 1]
key = lambda r: (min(len(words[r]), 12) // 3, int(math.log2(df[r] + 1)),
                 int(math.log2(cnt[r] + 1)), int(share[r] * 4.999), r)
sec['tail_bitmap'] = f'{len(cand)}\n' + ''.join('1' if member[r] else '0' for r in sorted(cand, key=key)) + '\n'
# 4. run numbers as similarity ranks (runs.py), words most frequent first;
#    a new run is written as n<its index among the runs not started yet>
unstarted, out = list(range(R)), []
for w, s in zip(sorted(run_of, key=lambda w: rank[w]), np.load(work('runs_rank_e9sparse.npy'))):
    if s == -1:
        out.append(f'n{unstarted.index(run_of[w])}\n')
        unstarted.remove(run_of[w])
    else:
        out.append(f'{s}\n')
sec['runs'] = ''.join(out)
full = f'dicrank 2\n100000000 {len(hr)} {R} 0\n' + ''.join(sec.values())
open(work('dict_rank_v2.txt'), 'w').write(full)
for k, v in sec.items():
    open(work(f'v2_{k}.txt'), 'w').write(v)
print({k: len(v) for k, v in sec.items()}, 'total', len(full))
