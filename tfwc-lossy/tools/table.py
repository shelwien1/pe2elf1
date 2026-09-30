# per group, per step multiplier: entropy (bytes) and predicted A-weighted error
import numpy as np, wd, rq, stats as st, gq, json, sys
S = st.load('../runs/stats_b128k.bin')
ts = wd.load('orig.wd'); T = {t[0]: t for t in ts}
KS = [1.0, 1.15, 1.25, 1.35, 1.5, 1.65, 1.8, 2.0, 2.25, 2.5, 2.75, 3.0, 3.5, 4.0]
mode = sys.argv[1] if len(sys.argv) > 1 else 'prop'
sl = int(sys.argv[2]) if len(sys.argv) > 2 else 0
nsl = int(sys.argv[3]) if len(sys.argv) > 3 else 1
tab = {}
for ii, (qn, sn) in enumerate(rq.pairs(ts)):
    if ii % nsl != sl: continue
    g = rq.group_of(qn); key = qn[:-len('.weight.q')]
    q = T[qn][3].astype(np.float64); s = wd.bf16(T[sn][3]).astype(np.float64)
    W_ = q * s[:, None]
    A = S[key]['A'] / S[key]['nf'] if key in S else None
    meth = 'rtn' if g in ('embedding', 'prior_embedding') else 'gptq'
    for k in KS:
        if mode == 'prop':
            s2 = s * k
        else:
            s2 = np.maximum(k * np.median(np.abs(s)), np.abs(W_).max(1) / 7.0)
        s2 = wd.bf16(wd.to_bf16(s2)).astype(np.float64)
        if k == 1.0 and mode == 'prop':
            qq = T[qn][3]
        else:
            qq = gq.requant_tensor(W_, A, s2, meth, 0.0)
        Ae = A if A is not None else np.eye(W_.shape[1])
        e = gq.werr(W_, qq, s2, Ae)
        d = tab.setdefault(g, {}).setdefault(str(k), [0.0, 0.0])
        d[0] += rq.ent0(qq); d[1] += e
json.dump(dict(KS=KS, tab=tab), open('../runs/table_%s_%d.json' % (mode, sl), 'w'))
print("done", len(tab))
