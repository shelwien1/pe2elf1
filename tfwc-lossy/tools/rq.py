# requantization helpers
import numpy as np, wd, re, subprocess, os, json

def pairs(ts):
    """yield (qname, sname) for int4 tensors with their scales"""
    names = [t[0] for t in ts]
    S = set(names)
    for n in names:
        if n.endswith('.weight.q'):
            s = n[:-2] + '.scale'
            assert s in S, s
            yield n, s

def ent0(q):
    h = np.bincount((q.astype(np.int64)+7).ravel(), minlength=15)
    p = h[h>0]/h.sum()
    return -(p*np.log2(p)).sum()*q.size/8.0

def requant_row(w, s_new, mode='rtn', lam=0.0):
    """w: (rows, cols) float targets; s_new: (rows,) float scales (bf16-rounded). returns q int8"""
    x = w / np.where(s_new[:,None] == 0, 1, s_new[:,None])
    q = np.clip(np.rint(x), -7, 7)
    return q.astype(np.int8)

def group_of(name):
    # coarse group name for a q tensor
    m = re.match(r'blocks\.(\d+)\.(attention|mlp)\.(.*)\.weight\.q', name)
    if not m:
        return name.replace('.weight.q', '')
    l, a, rest = m.groups()
    if 'gate_projection' in rest: rest = 'gates'
    else: rest = rest.replace('_projection', '')
    return 'L%d.%s' % (int(l), rest)

def apply(ts, kmap, default_k=1.0):
    """kmap: dict group -> k (step multiplier). returns new ts list (copy) and stats"""
    T = {t[0]: i for i, t in enumerate(ts)}
    out = [list(t) for t in ts]
    rate = 0.0; rate0 = 0.0; nch = 0
    for qn, sn in pairs(ts):
        g = group_of(qn)
        k = kmap.get(g, default_k)
        q = ts[T[qn]][3].astype(np.float32)
        s_bits = ts[T[sn]][3]
        s = wd.bf16(s_bits)
        rate0 += ent0(ts[T[qn]][3])
        if k == 1.0:
            rate += ent0(ts[T[qn]][3]); continue
        w = q * s[:, None]
        s2_bits = wd.to_bf16(s * k)
        s2 = wd.bf16(s2_bits)
        q2 = requant_row(w, s2)
        out[T[qn]][3] = q2
        out[T[sn]][3] = s2_bits
        rate += ent0(q2)
        nch += q2.size
    return out, rate, rate0

def write_tfwc2(ts, path):
    wd.save(path + '.wd', ts)
    subprocess.check_call([os.path.join(os.path.dirname(os.path.abspath(__file__)), 'wdump'), 'p', path + '.wd', path])
    os.remove(path + '.wd')
    return os.path.getsize(path)
