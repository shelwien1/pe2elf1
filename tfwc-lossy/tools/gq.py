# group requantization with RTN or GPTQ(-RDO), using activation statistics
import numpy as np, wd, rq, stats as st, gptq, re
def tensors_of_group(ts, g):
    return [(qn, sn) for qn, sn in rq.pairs(ts) if rq.group_of(qn) == g]
def code_bits(q, alpha=0.5):
    h = np.bincount((q.astype(int) + 7).ravel(), minlength=15) + alpha
    return -np.log2(h / h.sum())
def requant_tensor(W, A, s_new, method='rtn', lam=0.0, iters=2, damp=0.01):
    """returns q (int8) for target float weights W with row scales s_new"""
    sz = np.where(s_new == 0, 1, s_new)
    q = np.clip(np.rint(W / sz[:, None]), -7, 7).astype(np.int8)
    if method == 'rtn' and lam == 0: return q
    if A is None:  # no stats: identity metric
        A = np.eye(W.shape[1])
    for it in range(iters):
        bits = code_bits(q)
        q, _ = gptq.gptq_rdo(W, A, s_new, bits, lam, damp=damp, act_order=(method == 'gptq'))
        if lam == 0: break
    return q
def werr(W, q, s, A):
    D = W - q.astype(np.float64) * s[:, None]
    return np.einsum('ri,ij,rj->', D, A, D)
def apply(ts, S, plan):
    """plan: dict group -> dict(k=, method=, lam=). returns new ts and info"""
    T = {t[0]: i for i, t in enumerate(ts)}
    out = [list(t) for t in ts]
    info = {}
    for g, p in plan.items():
        for qn, sn in tensors_of_group(ts, g):
            q = ts[T[qn]][3].astype(np.float64); s = wd.bf16(ts[T[sn]][3]).astype(np.float64)
            W = q * s[:, None]
            s2b = wd.to_bf16(s * p.get('k', 1.0)); s2 = wd.bf16(s2b).astype(np.float64)
            key = qn[:-len('.weight.q')]
            A = S[key]['A'] / max(S[key]['nf'], 1) if (S is not None and key in S) else None
            q2 = requant_tensor(W, A, s2, p.get('method', 'rtn'), p.get('lam', 0.0))
            out[T[qn]][3] = q2; out[T[sn]][3] = s2b
            info[qn] = dict(H0=rq.ent0(ts[T[qn]][3]), H1=rq.ent0(q2),
                            err=werr(W, q2, s2, A) if A is not None else None)
    return out, info
