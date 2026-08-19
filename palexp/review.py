# The claims four external write-ups of the palette idea make, checked against the
# corpus.  Section numbers match PALETTE.md.
#
#   ./mk.sh release && python3 palexp/review.py
#
# BMG names the executable (default ./bmg), BMGTMP the scratch directory.

import os, sys, subprocess, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np, palexp, wbmp
from palexp import H, log2_choose, order_to_pos as o2p

BMG = os.environ.get('BMG', './bmg')
TD  = os.environ.get('BMGTMP', tempfile.mkdtemp(prefix='review'))
os.makedirs(TD, exist_ok=True)
FILES = ['t8g','t8p','x_ai','x_ci','x_ep','t24','t32']

def bmg_size( idx ):
    f = os.path.join(TD,'r.bmp'); g = os.path.join(TD,'r.bmg')
    wbmp.write8( f, idx.astype(np.uint8) )
    subprocess.run( [BMG,'c',f,g], capture_output=True )
    n = os.path.getsize(g); os.remove(f); os.remove(g); return n

def cost( chans ):                     # order-0 bytes for several residual streams
    t = 0.0
    for c in chans:
        _, n = np.unique( np.asarray(c,np.int64), return_counts=True )
        t += H(n)*len(c)
    return t/8 + 4

def morton( r, g, b ):
    m = np.zeros(len(r), np.int64)
    for i in range(8):
        m |= ((r>>i)&1)<<(3*i+2); m |= ((g>>i)&1)<<(3*i+1); m |= ((b>>i)&1)<<(3*i)
    return m

P = { f: palexp.Pal('testfiles/%s.bmp'%f) for f in FILES }

# ------------------------------------------------------- 2.5 palette transforms
print('===== 2.5  palette serialisation: sort order x residual transform, bytes')
print('  %-8s %7s  %-8s %9s %9s %9s %9s'
      % ('file','K','sort','packed d','chan d','YCoCg-R d','green d'))
for f in FILES:
    p = P[f]
    B,G,R,A = (p.B.astype(np.int64), p.G.astype(np.int64),
               p.R.astype(np.int64), p.A.astype(np.int64))
    for nm, o in [('packed',  np.argsort(p.key.astype(np.int64))),
                  ('morton',  np.argsort(morton(R,G,B))),
                  ('hilbert', palexp.ord_hilbert(p))]:
        r,g,b = R[o],G[o],B[o]
        extra = [np.diff(A[o])] if p.NC==4 else []
        pk = b | (g<<8) | (r<<16) | ((A[o]<<24) if p.NC==4 else 0)
        c_pk = '%9.0f' % cost([np.diff(pk)]) if nm=='packed' else '%9s'%'-'
        Co = r-b; t = b+(Co>>1); Cg = g-t; Y = t+(Cg>>1)
        dg = np.diff(g)
        print('  %-8s %7s  %-8s %s %9.0f %9.0f %9.0f'
              % (f if nm=='packed' else '', p.K if nm=='packed' else '', nm, c_pk,
                 cost([np.diff(r),np.diff(g),np.diff(b)]+extra),
                 cost([np.diff(Y),np.diff(Co),np.diff(Cg)]+extra),
                 cost([dg,np.diff(r)-dg,np.diff(b)-dg]+extra)))

# ------------------------- 2.5 "Hilbert bounds the consecutive palette distance"
print('\n===== 2.5  is the consecutive distance under a Hilbert order bounded?')
for f in ['t8p','x_ep','t24']:
    p = P[f]
    for nm, o in [('hilbert',palexp.ord_hilbert(p)), ('packed',palexp.ord_packed(p))]:
        C = np.stack([p.R,p.G,p.B],1)[o]
        d = np.abs(np.diff(C,axis=0)).sum(1)
        print('  %-8s %-8s K=%-6d consecutive L1: mean %6.2f  max %4.0f'
              % (f if nm=='hilbert' else '', nm, p.K, d.mean(), d.max()))

# ------------------------------------------------------------- 3 the count table
print('\n===== 3  the frequency table: compositions against a model of the counts')
print('  %-8s %8s %10s %12s %12s %10s' % ('file','K','singletons','compositions','order-0','adaptive'))
for f in FILES:
    p = P[f]; K,N = p.K,p.N
    m = np.bincount(p.cnt.astype(np.int64))
    print('  %-8s %8d %10d %12.0f %12.0f %10.0f'
          % (f, K, m[1] if len(m)>1 else 0, log2_choose(N-1,K-1)/8,
             H(m)*K/8, (K-1)/2*np.log2(N)/8))

# --------------------------------------------------------------- 3.4 escapes
print('\n===== 3.4  escape coding: admit a colour only if it repeats')
for f in ['x_ep','t24']:
    p = P[f]
    for T in (1,2,3,4,8):
        keep = p.cnt >= T; Kk = int(keep.sum())
        if Kk == 0: break
        esc = int(p.cnt[~keep].sum())
        print('  %-8s count>=%-2d  %7d entries (%8.0f B set bound)  %8d escapes (%5.2f%%)'
              % (f if T==1 else '', T, Kk, log2_choose(1<<(8*p.NC),Kk)/8, esc, 100*esc/p.N))

# ------------------------------------------------------- 5.4 integer sort keys
print('\n===== 5.4  how few bits does a sort weight need?  (t8p, w = .9641/.2651/.0150)')
p = P['t8p']; ref = o2p(palexp.ord_packed(p), p.K); w = np.array([0.9641,0.2651,0.0150])
for bits in (4,8,16):
    S = (1<<bits)-1
    wi = np.round(w/w.max()*S).astype(np.int64)
    key = wi[0]*p.R.astype(np.int64) + wi[1]*p.G.astype(np.int64) + wi[2]*p.B.astype(np.int64)
    pos = o2p(np.lexsort((p.key.astype(np.int64), key)), p.K)
    print('  %2d-bit %-22s same ordering as packed: %-5s  bmg %d'
          % (bits, str(wi.tolist()), np.array_equal(pos,ref), bmg_size(pos[p.I0])))

# -------------------------------------------------------- 5.5 PCA initialisers
print('\n===== 5.5  PCA as the weight initialiser')
def pca( C, w ):
    mu = (C*w[:,None]).sum(0)/w.sum(); D = C-mu
    return np.linalg.eigh((D*w[:,None]).T@D/w.sum())[1][:,-1]
for f in ['t8g','t8p','x_ai','x_ci']:
    p = P[f]
    C = np.stack([p.R,p.G,p.B]+([p.A] if p.NC==4 else []),1)
    wc = pca( C, p.cnt.astype(float) )
    wd = np.linalg.eigh(((C[p.pa]-C[p.pb])*p.pc[:,None]).T@(C[p.pa]-C[p.pb])/p.pc.sum())[1][:,-1]
    for nm,o in [('packed',palexp.ord_packed(p)),
                 ('PCA of colours',palexp.ord_linear(p,wc)),
                 ('PCA of differences',palexp.ord_linear(p,wd))]:
        print('  %-8s %-20s bmg %d' % (f if nm=='packed' else '', nm, bmg_size(o2p(o,p.K)[p.I0])))

# ------------------------------------------------------------ 6.6 Gray coding
print('\n===== 6.6  Gray-coded indices, g = i XOR (i>>1)')
for f in ['t8g','t8p','x_ai','x_ci']:
    p = P[f]; I = o2p(palexp.ord_packed(p), p.K)[p.I0]
    a, b = bmg_size(I), bmg_size(I ^ (I>>1))
    print('  %-8s plain %8d   gray %8d   %+d' % (f,a,b,b-a))

# ------------------------------------------------------ 8 alpha in the sort key
print('\n===== 8  x_ep: where alpha belongs in the ordering')
p = P['x_ep']
A = p.A.astype(np.int64); rgb = p.B.astype(np.int64)|(p.G.astype(np.int64)<<8)|(p.R.astype(np.int64)<<16)
for nm,o in [('packed (alpha-major)',    np.argsort(p.key.astype(np.int64))),
             ('alpha class {0,mid,255}', np.lexsort((rgb, np.where(A==0,0,np.where(A==255,2,1))))),
             ('RGB-major, alpha low',    np.lexsort((A, rgb)))]:
    pos = o2p(o,p.K)
    print('  %-26s Hmed %8.4f = %8.0f bytes' % (nm, p.Hmed(pos), p.Hmed(pos)*p.N/8))
