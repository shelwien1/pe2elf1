# per-row importance and per-row step allocation
import numpy as np, wd
def up_importance(S, T, l):
    """relative importance of mlp.up rows (hidden units) of layer l: 4 E[relu(h)^2] ||W_down[:, j]||^2"""
    dn = S['blocks.%d.mlp.down' % l]
    r2 = dn['X1'] / dn['nf']
    q = T['blocks.%d.mlp.down.weight.q' % l][3].astype(float); s = wd.bf16(T['blocks.%d.mlp.down.weight.scale' % l][3])
    imp = 4 * r2 * ((q * s[:, None]) ** 2).sum(0)
    return imp / imp.mean()
def row_steps(k, imp, kmax=8.0):
    return np.clip(k / np.sqrt(np.maximum(imp, 1e-6)), 1.0, kmax)
def class_of(kr, ncls=8):
    """row class: log2 of the step multiplier in half-octave bins"""
    return np.clip(np.floor(np.log2(kr) * 2 + 1e-9).astype(int), 0, ncls - 1)
def cond_entropy_bytes(q, cls):
    tot = 0.0
    for c in np.unique(cls):
        h = np.bincount((q[cls == c].astype(int) + 7).ravel(), minlength=15)
        p = h[h > 0] / h.sum(); tot += -(h[h > 0] * np.log2(p)).sum()
    hc = np.bincount(cls); p = hc[hc > 0] / hc.sum()
    tot += -(hc[hc > 0] * np.log2(p)).sum()  # class side info
    return tot / 8
def value_importance(S, T, l):
    """relative importance of value_projection rows of layer l:
    ||W_o[:, i]||^2 * E[o_in_i^2] / E[v_i^2]  (o_in = the output projection's input)"""
    pre = 'blocks.%d.attention.' % l
    qv = T[pre + 'value_projection.weight.q'][3].astype(float); sv = wd.bf16(T[pre + 'value_projection.weight.scale'][3])
    Wv = qv * sv[:, None]
    qo = T[pre + 'output_projection.weight.q'][3].astype(float); so = wd.bf16(T[pre + 'output_projection.weight.scale'][3])
    Wo = qo * so[:, None]
    Ax = S[pre + 'value_projection']['A'] / S[pre + 'value_projection']['nf']
    ev = np.einsum('ij,jk,ik->i', Wv, Ax, Wv)
    eo = np.diag(S[pre + 'output_projection']['A']) / S[pre + 'output_projection']['nf']
    imp = (Wo ** 2).sum(0) * eo / np.maximum(ev, 1e-30)
    return imp / imp.mean()
