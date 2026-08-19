# The palette experiments behind PALETTE.md: build the palette and the index
# image, the co-occurrence table, the orderings, the two entropy proxies, and the
# palette and frequency-table coding costs.  Nothing here runs bmg; run.py and
# exact.py do that.

import sys, time, math, struct
import numpy as np
import bmpio

# ---------------------------------------------------------------- basics

def H(c):
    c = np.asarray(c, np.float64); t = c.sum()
    if t <= 0: return 0.0
    p = c[c > 0]/t
    return float(-(p*np.log2(p)).sum())

def med_res(I, K):
    Hh, Ww = I.shape
    Wv = np.empty_like(I); Wv[:,1:] = I[:,:-1]; Wv[1:,0] = I[:-1,0]; Wv[0,0] = 0
    Nv = np.empty_like(I); Nv[1:,:] = I[:-1,:]; Nv[0,:] = Wv[0,:]
    NW = np.empty_like(I); NW[1:,1:] = I[:-1,:-1]; NW[0,:] = Nv[0,:]; NW[1:,0] = Nv[1:,0]
    mx = np.maximum(Wv,Nv); mn = np.minimum(Wv,Nv)
    pred = np.where(NW >= mx, mn, np.where(NW <= mn, mx, Wv+Nv-NW))
    return (I - pred) % K

def log2_choose(n, k):
    from math import lgamma
    if k <= 0 or k >= n: return 0.0
    return (lgamma(n+1)-lgamma(k+1)-lgamma(n-k+1))*1.4426950408889634

# ---------------------------------------------------------------- palette

class Pal:
    def __init__(self, fn):
        W, Hh, px, pal, bpp, rawidx = bmpio.load(fn)
        self.fn, self.W, self.H, self.bpp = fn, W, Hh, bpp
        self.rawidx = rawidx
        nc = px.shape[2]
        v = px.reshape(-1, nc).astype(np.uint32)
        key = v[:,0] | (v[:,1]<<8) | (v[:,2]<<16)
        if nc == 4: key |= v[:,3].astype(np.uint32) << 24
        u, inv, cnt = np.unique(key, return_inverse=True, return_counts=True)
        self.NC = nc
        self.K = len(u)
        self.key = u                      # ascending packed value = "solid integer" order
        self.cnt = cnt
        self.I0 = inv.reshape(Hh, W).astype(np.int32)   # index image in packed order
        self.N = Hh*W
        self.B = (u & 255).astype(np.float64)
        self.G = ((u>>8) & 255).astype(np.float64)
        self.R = ((u>>16) & 255).astype(np.float64)
        self.A = ((u>>24) & 255).astype(np.float64)
        # co-occurrence over the west and north neighbours, collapsed to pairs
        I = self.I0
        a = np.concatenate([I[:,1:].ravel(), I[1:,:].ravel()])
        b = np.concatenate([I[:,:-1].ravel(), I[:-1,:].ravel()])
        pk, pc = np.unique(a.astype(np.int64)*self.K + b, return_counts=True)
        self.pa = (pk // self.K).astype(np.int64)
        self.pb = (pk %  self.K).astype(np.int64)
        self.pc = pc.astype(np.float64)
        self.npair = self.pc.sum()

    # bits per adjacent pair under one ordering -- the search objective
    def Hpair(self, pos):
        d = (pos[self.pa] - pos[self.pb]) % self.K
        return H(np.bincount(d, weights=self.pc, minlength=self.K))

    # bits per pixel of the MED residual of the reordered index image
    def Hmed(self, pos):
        I = pos[self.I0]
        r = med_res(I.astype(np.int32), self.K)
        return H(np.bincount(r.ravel(), minlength=self.K))

def order_to_pos(order, K):
    pos = np.empty(K, np.int64); pos[order] = np.arange(K)
    return pos

# ---------------------------------------------------------------- orderings

def ord_packed(p):  return np.arange(p.K)                       # already sorted
def ord_freq(p):    return np.argsort(-p.cnt, kind='stable')
def ord_luma(p):    return np.argsort(0.299*p.R + 0.587*p.G + 0.114*p.B, kind='stable')

def ord_linear(p, w, g=None):
    R, G, B, A = p.R/255.0, p.G/255.0, p.B/255.0, p.A/255.0
    if g is not None:
        R, G, B, A = R**g[0], G**g[1], B**g[2], A**(g[3] if len(g)>3 else 1.0)
    k = w[0]*R + w[1]*G + w[2]*B + (w[3]*A if len(w) > 3 else 0.0)
    return np.argsort(k, kind='stable')

def hilbert_d(x, y, z, bits=8):
    X = [np.array(x, np.int64), np.array(y, np.int64), np.array(z, np.int64)]
    M = 1 << (bits-1)
    Q = M
    while Q > 1:                       # Skilling's inverse-undo pass
        P = Q - 1
        for i in range(3):
            m = (X[i] & Q) != 0
            X[0] = np.where(m, X[0] ^ P, X[0])
            t = np.where(m, 0, (X[0] ^ X[i]) & P)
            X[0] ^= t; X[i] ^= t
        Q >>= 1
    for i in range(1,3): X[i] ^= X[i-1]
    t = np.zeros_like(X[0]); Q = M
    while Q > 1:
        t ^= np.where((X[2] & Q) != 0, Q-1, 0)
        Q >>= 1
    for i in range(3): X[i] ^= t
    d = np.zeros_like(X[0])            # interleave, most significant bit first
    for b in range(bits-1, -1, -1):
        for i in range(3):
            d = (d << 1) | ((X[i] >> b) & 1)
    return d

def ord_hilbert(p):
    return np.argsort(hilbert_d(p.R.astype(np.int64), p.G.astype(np.int64),
                                p.B.astype(np.int64)), kind='stable')

def ord_tsp_rgb(p, limit=8192):
    K = p.K
    if K > limit: return None
    C = np.stack([p.R, p.G, p.B], 1)
    left = np.ones(K, bool); cur = int(np.argmin(C.sum(1))); left[cur] = False
    out = [cur]
    for _ in range(K-1):
        d = ((C - C[cur])**2).sum(1); d[~left] = np.inf
        cur = int(np.argmin(d)); left[cur] = False; out.append(cur)
    return np.array(out)

def ord_cooc_greedy(p):
    # Grow a chain by always attaching the colour with the strongest remaining
    # link to an end of it.  A cheap heuristic for minimum linear arrangement on
    # the image's own adjacency graph -- it optimises what actually matters
    # rather than distance in RGB.
    K = p.K
    m = p.pa != p.pb
    a, b, c = p.pa[m], p.pb[m], p.pc[m]
    w = {}
    for i, j, v in zip(a.tolist(), b.tolist(), c.tolist()):
        k = (i, j) if i < j else (j, i)
        w[k] = w.get(k, 0.0) + v
    adj = [dict() for _ in range(K)]
    for (i, j), v in w.items():
        adj[i][j] = v; adj[j][i] = v
    used = np.zeros(K, bool)
    start = int(np.argmax(p.cnt)); used[start] = True
    chain = [start]
    for _ in range(K-1):
        best, bv = -1, -1.0
        for end in (chain[0], chain[-1]):
            for j, v in adj[end].items():
                if not used[j] and v > bv: best, bv, at = j, v, end
        if best < 0:
            rest = np.nonzero(~used)[0]
            best, at = int(rest[np.argmax(p.cnt[rest])]), chain[-1]
        used[best] = True
        chain.append(best) if at == chain[-1] else chain.insert(0, best)
    return np.array(chain)

def ord_spectral(p, limit=1200):
    K = p.K
    if K > limit: return None
    Wm = np.zeros((K,K))
    np.add.at(Wm, (p.pa, p.pb), p.pc)
    Wm = Wm + Wm.T
    np.fill_diagonal(Wm, 0)
    L = np.diag(Wm.sum(1)) - Wm
    val, vec = np.linalg.eigh(L)
    f = vec[:, 1] if K > 1 else vec[:, 0]
    return np.argsort(f, kind='stable')

# ---------------------------------------------------------------- search

def fib_sphere(n):
    i = np.arange(n) + 0.5
    phi = np.arccos(1 - 2*i/n); th = np.pi*(1+5**0.5)*i
    return np.stack([np.cos(th)*np.sin(phi), np.sin(th)*np.sin(phi), np.cos(phi)], 1)

def optimise_weights(p, ndir=768, refine=400, gamma=False, seed=1):
    rng = np.random.default_rng(seed)
    dirs = fib_sphere(2*ndir)
    dirs = dirs[dirs[:,1] >= 0]                      # w and -w give the same cost
    if p.NC == 4:
        extra = rng.normal(size=(len(dirs),1))*0.5
        dirs = np.concatenate([dirs, extra], 1)
    best = (1e9, None, None)
    for w in dirs:
        g = np.ones(4) if gamma else None
        o = ord_linear(p, w, g)
        c = p.Hpair(order_to_pos(o, p.K))
        if c < best[0]: best = (c, w.copy(), None if g is None else g.copy())
    cost, w, g = best
    if gamma and g is None: g = np.ones(4)
    step_w, step_g = 0.25, 0.35
    for it in range(refine):
        w2 = w + rng.normal(size=w.shape)*step_w
        n = np.linalg.norm(w2[:3])
        if n < 1e-9: continue
        w2 = w2/n
        g2 = None
        if gamma:
            g2 = np.clip(g*np.exp(rng.normal(size=4)*step_g), 0.15, 6.0)
        c = p.Hpair(order_to_pos(ord_linear(p, w2, g2), p.K))
        if c < cost: cost, w, g = c, w2, g2
        if it % 100 == 99: step_w *= 0.55; step_g *= 0.55
    return cost, w, g

# ---------------------------------------------------------------- palette cost

def palette_costs(p):
    K, u = p.K, p.key
    bytes_per = p.NC
    out = {}
    out['raw'] = K*bytes_per
    d = np.diff(np.sort(u.astype(np.int64)))
    out['delta-H0'] = (H(np.bincount(np.minimum(d, 1<<20)))*(K-1) + 32)/8
    universe = 1 << (8*bytes_per)
    out['set-bound'] = log2_choose(universe, K)/8
    # per-channel entropy of the sorted-by-packed list, which is what a byte
    # oriented coder would see
    v = np.stack([p.B, p.G, p.R] + ([p.A] if p.NC == 4 else []), 1).astype(np.int64)
    hh = 0.0
    for c in range(v.shape[1]):
        dd = np.diff(v[:,c]) % 256
        hh += H(np.bincount(dd, minlength=256))*(K-1)
    out['chan-delta-H0'] = hh/8
    return out

def freq_costs(p):
    K, N = p.K, p.N
    c = np.sort(p.cnt)[::-1].astype(np.int64)
    out = {}
    out['composition-bound'] = log2_choose(N-1, K-1)/8 if K > 1 else 0.0
    # decreasing counts: send the gaps, which is the partition bound
    g = np.concatenate([[c[0]], -np.diff(c)])
    out['sorted-gap-H0'] = (H(np.bincount(g.astype(np.int64)))*K)/8
    out['golomb-ish'] = float(np.sum(np.ceil(np.log2(np.maximum(c,1)+1))*2))/8
    return out
