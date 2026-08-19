# For a palette small enough to enumerate: score every ordering with the pairwise
# proxy, then compress a spread of them with bmg and correlate.  Tables 6.2 and
# 6.3 of PALETTE.md.
#
#   python3 palexp/exact.py testfiles/x_ci.bmp

import os, sys, time, subprocess, itertools, tempfile
import numpy as np
import palexp, wbmp
from palexp import order_to_pos as o2p

BMG = os.environ.get('BMG', './bmg')
TD  = os.environ.get('BMGTMP', tempfile.mkdtemp(prefix='palexp'))
os.makedirs(TD, exist_ok=True)

def bmg_size(idx, tag='e'):
    f=os.path.join(TD,tag+'.bmp'); g=os.path.join(TD,tag+'.bmg')
    wbmp.write8(f, idx.astype(np.uint8))
    subprocess.run([BMG,'c',f,g],capture_output=True)
    n=os.path.getsize(g); os.remove(f); os.remove(g); return n

def spearman(a,b):
    a=np.asarray(a,float); b=np.asarray(b,float)
    ra=np.argsort(np.argsort(a)).astype(float); rb=np.argsort(np.argsort(b)).astype(float)
    ra-=ra.mean(); rb-=rb.mean()
    return float((ra*rb).sum()/np.sqrt((ra*ra).sum()*(rb*rb).sum()))

fn=sys.argv[1]; p=palexp.Pal(fn)
K=p.K
print('%s  K=%d  N=%d' % (os.path.basename(fn), K, p.N))
perms=[np.array(q) for q in itertools.permutations(range(K)) if q[0] < q[-1]]
print('  %d orderings up to reversal' % len(perms))
hp=np.array([p.Hpair(o2p(q,K)) for q in perms])
order=np.argsort(hp)
if len(perms) <= 16: pick=list(range(len(perms)))
else:
    pick=sorted(set(order[:6].tolist() + order[-4:].tolist()
                    + order[len(order)//2-2:len(order)//2+2].tolist()
                    + np.random.default_rng(3).choice(len(perms), 30, replace=False).tolist()))
rows=[]
t0=time.time()
for i in pick:
    q=perms[i]; pos=o2p(q,K)
    hm=p.Hmed(pos); n=bmg_size(pos[p.I0])
    rows.append((hp[i],hm,n,q.tolist()))
rows.sort(key=lambda r:r[2])
print('  %8s %8s %10s   permutation        (%.0fs for %d coder runs)'
      % ('Hpair','Hmed','bmg bytes', time.time()-t0, len(pick)))
for r in rows: print('  %8.4f %8.4f %10d   %s' % r)
a=[r[0] for r in rows]; b=[r[1] for r in rows]; c=[r[2] for r in rows]
print('  spearman(Hpair, bytes) = %+.3f    spearman(Hmed, bytes) = %+.3f' % (spearman(a,c), spearman(b,c)))
print('  best by Hpair -> %d bytes;  best by Hmed -> %d bytes;  true best -> %d bytes'
      % (rows[int(np.argmin(a))][2], rows[int(np.argmin(b))][2], c[0]))
