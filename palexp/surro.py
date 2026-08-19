# Which surrogate objective predicts bmg's output?  Measured on every ordering we
# can afford to actually compress.
import sys, os, subprocess, tempfile, itertools
sys.path.insert(0,'palexp')
import numpy as np, palexp, wbmp
from palexp import order_to_pos as o2p

BMG='./bmg'; TD=tempfile.mkdtemp(prefix='sur')
def bmg_size(idx):
    f=os.path.join(TD,'x.bmp'); g=os.path.join(TD,'x.bmg')
    wbmp.write8(f, idx.astype(np.uint8)); subprocess.run([BMG,'c',f,g],capture_output=True)
    return os.path.getsize(g)
def sp(a,b):
    a=np.asarray(a,float);b=np.asarray(b,float)
    ra=np.argsort(np.argsort(a)).astype(float);rb=np.argsort(np.argsort(b)).astype(float)
    ra-=ra.mean();rb-=rb.mean();return float((ra*rb).sum()/np.sqrt((ra*ra).sum()*(rb*rb).sum()))

def surrogates(p,pos):
    d=np.abs(pos[p.pa]-pos[p.pb]).astype(float)
    return {'Hmed':p.Hmed(pos), 'Hpair':p.Hpair(pos),
            'L1':float((p.pc*d).sum()), 'log2(1+d)':float((p.pc*np.log2(1+d)).sum())}

KEYS=('Hmed','Hpair','L1','log2(1+d)')
for f,nsamp in [('x_ci',12),('x_ai',40),('t8p',24)]:
    p=palexp.Pal('testfiles/%s.bmp'%f); K=p.K
    if K<=8:
        perms=[np.array(q) for q in itertools.permutations(range(K)) if q[0]<q[-1]]
        idx=range(len(perms)) if len(perms)<=nsamp else np.random.default_rng(5).choice(len(perms),nsamp,replace=False)
        cands=[perms[i] for i in idx]
    else:
        rng=np.random.default_rng(5)
        cands=[palexp.ord_packed(p), palexp.ord_luma(p), palexp.ord_hilbert(p), palexp.ord_freq(p),
               palexp.ord_tsp_rgb(p), palexp.ord_cooc_greedy(p), palexp.ord_spectral(p)]
        cands=[c for c in cands if c is not None]
        for _ in range(nsamp-len(cands)):
            w=rng.normal(size=3); cands.append(palexp.ord_linear(p,w))
    rows={k:[] for k in KEYS}; by=[]
    for q in cands:
        pos=o2p(q,K); s=surrogates(p,pos)
        for k in KEYS: rows[k].append(s[k])
        by.append(bmg_size(pos[p.I0]))
    print('\n%s  K=%d  %d orderings compressed;  bytes %d..%d (spread %.2f%%)'
          % (f,K,len(cands),min(by),max(by),100*(max(by)/min(by)-1)))
    print('  %-12s %8s   %s' % ('surrogate','spearman','bytes of its own best pick'))
    for k in KEYS:
        j=int(np.argmin(rows[k]))
        print('  %-12s %+8.3f   %d%s' % (k, sp(rows[k],by), by[j], '   <- true best' if by[j]==min(by) else ''))
