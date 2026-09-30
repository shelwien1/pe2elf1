# replace whole groups by seeded noise (matching histogram) or zeros
import numpy as np, wd, rq, sys, json
def make(groups, mode, out, seed=12345, per_row_hist=False):
    ts = wd.load('orig.wd'); T = {t[0]: i for i, t in enumerate(ts)}
    rng = np.random.default_rng(seed)
    saved = 0.0
    for qn, sn in rq.pairs(ts):
        if rq.group_of(qn) not in groups: continue
        q = ts[T[qn]][3]
        saved += rq.ent0(q)
        if mode == 'zero':
            q2 = np.zeros_like(q)
        elif mode == 'rng':
            h = np.bincount((q.astype(int) + 7).ravel(), minlength=15).astype(float)
            q2 = (rng.choice(15, size=q.shape, p=h / h.sum()) - 7).astype(np.int8)
        elif mode == 'rngrow':   # per-row permutation: keeps each row's exact histogram
            q2 = q.copy()
            for r in range(q.shape[0]): rng.shuffle(q2[r])
        ts[T[qn]][3] = q2
    rq.write_tfwc2(ts, out)
    return saved
if __name__ == '__main__':
    groups = sys.argv[1].split(','); mode = sys.argv[2]; out = sys.argv[3]
    print(out, "int4 entropy bytes removed: %.0f" % make(groups, mode, out))
