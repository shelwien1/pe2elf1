"""Run numbers of the 40,595 alphabetically-run words, predicted from their
contexts. Prints estimated bytes; saves the rank symbols used by build_v2.py."""
import numpy as np

from common import (add_suffix_features, adaptive_cost, load, ppmi, run_predictions,
                    svd_vectors, unit_rows, work)

d = load()
words, rank, run_of, R = d['words'], d['rank'], d['run_of'], d['runs']
tw = sorted(run_of, key=lambda w: rank[w])  # most frequent first
truth = [run_of[w] for w in tw]
targets = [rank[w] for w in tw]
N = len(words)
print(f'plain run ids, adaptive order-0: {adaptive_cost(truth, R):,.0f} bytes')


def report(label, V, betas):
    syms, new_bytes = run_predictions(V, truth, R)
    ranked = adaptive_cost(syms, R + 1) + new_bytes
    soft = {b: run_predictions(V, truth, R, beta=b) for b in betas}
    best = min(soft, key=soft.get)
    print(f'{label}: rank-coded {ranked:,.0f} bytes; softmax best {soft[best]:,.0f} (beta {best}); '
          f'top-1 {sum(1 for s in syms if s == 0) / len(syms):.1%}', flush=True)
    return syms


P8 = ppmi(d['ids'], targets, N, K=2000)
V = svd_vectors(P8, 128)
report('enwik8, K=2000, SVD 128', V, [20])
report('enwik8, K=2000, SVD 128, suffix 0.5', add_suffix_features(V, [words[r] for r in targets]), [20])
P8 = ppmi(d['ids'], targets, N, K=4000)
V = svd_vectors(P8, 200)
report('enwik8, K=4000, SVD 200, suffix 0.5', add_suffix_features(V, [words[r] for r in targets]), [15, 20, 25])
report('enwik8, K=4000, SVD 200, suffix 1.0', add_suffix_features(V, [words[r] for r in targets], weight=1.0), [20])
report('enwik8, K=4000, sparse PPMI (no SVD)', unit_rows(P8), [40])
ids9 = np.load(work('ids9.npy'))
P9 = ppmi(ids9, targets, N, K=4000)
del ids9
report('enwik9, K=4000, SVD 200, suffix 0.5',
       add_suffix_features(svd_vectors(P9, 200), [words[r] for r in targets]), [15, 20, 25])
syms = report('enwik9, K=4000, sparse PPMI (no SVD)', unit_rows(P9), [40, 60])
np.save(work('runs_rank_e9sparse.npy'), np.array(syms))
