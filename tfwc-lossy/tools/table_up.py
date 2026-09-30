# weighted-row table entries for mlp.up groups; also E^w of the uniform-row configs (for refitting c)
import numpy as np, wd, rq, stats as st, gq, rowimp, json, sys
S = st.load('../runs/stats_b128k.bin')
ts = wd.load('orig.wd'); T = {t[0]: t for t in ts}
KS = json.load(open('../runs/table_prop.json'))['KS']
l0 = int(sys.argv[1]); l1 = int(sys.argv[2])
out = {}
def werr_rows(W_, qq, s2, A):
    D = W_ - qq.astype(np.float64) * s2[:, None]
    return np.einsum('ri,ij,rj->r', D, A, D)
for l in range(l0, l1):
    key = 'blocks.%d.mlp.up' % l; g = 'L%d.up' % l
    A = S[key]['A'] / S[key]['nf']
    q = T[key + '.weight.q'][3].astype(np.float64); s = wd.bf16(T[key + '.weight.scale'][3]).astype(np.float64)
    W_ = q * s[:, None]
    imp = rowimp.up_importance(S, T, l)
    d = {'w': {}, 'u': {}}
    for k in KS:
        # weighted rows
        kr = rowimp.row_steps(k, imp) if k > 1.0 else np.ones(len(s))
        s2 = wd.bf16(wd.to_bf16(s * kr)).astype(np.float64)
        qq = gq.requant_tensor(W_, A, s2, 'gptq', 0.0) if k > 1.0 else T[key + '.weight.q'][3]
        er = werr_rows(W_, qq, s2, A)
        d['w'][str(k)] = [rowimp.cond_entropy_bytes(qq, rowimp.class_of(kr)) if k > 1.0 else rq.ent0(qq), float((imp * er).sum())]
        # uniform rows (for refit): E^w
        if k in (2.0, 3.0):
            s2 = wd.bf16(wd.to_bf16(s * k)).astype(np.float64)
            qq = gq.requant_tensor(W_, A, s2, 'gptq', 0.0)
            d['u'][str(k)] = float((imp * werr_rows(W_, qq, s2, A)).sum())
    out[g] = d
json.dump(out, open('../runs/table_up_%d.json' % l0, 'w'))
