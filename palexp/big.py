# The negative control: for palettes too large for bmg to code as an index plane,
# the order-0 MED cost of that plane and the cost of the palette.  Table 6.6 of
# PALETTE.md.  Also splits an RGBA image into its RGB-only palette.
#
#   python3 palexp/big.py testfiles/t24.bmp testfiles/x_ep.bmp

import sys, numpy as np, palexp, bmpio
from palexp import order_to_pos as o2p

def variant(fn, drop_alpha=False):
    W,Hh,px,pal,bpp,ri = bmpio.load(fn)
    nc = px.shape[2]
    v = px.reshape(-1,nc).astype(np.uint32)
    key = v[:,0] | (v[:,1]<<8) | (v[:,2]<<16)
    if nc==4 and not drop_alpha: key |= v[:,3].astype(np.uint32)<<24
    u, inv, cnt = np.unique(key, return_inverse=True, return_counts=True)
    K=len(u); I=inv.reshape(Hh,W).astype(np.int32)
    hm = palexp.H(np.bincount(palexp.med_res(I,K).ravel(), minlength=K))
    N=Hh*W
    B=(u&255).astype(np.int64); G=((u>>8)&255).astype(np.int64); R=((u>>16)&255).astype(np.int64)
    nb = 3 if (nc==3 or drop_alpha) else 4
    d = np.diff(np.sort(u.astype(np.int64)))
    pal_delta = (palexp.H(np.bincount(np.minimum(d,1<<20)))*(K-1)+32)/8
    pal_set = palexp.log2_choose(1<<(8*nb), K)/8
    print('  %-28s K=%-7d %5.2f bits/index  MED H0 %7.4f b/px = %8.0f B   palette %7.0f B (set bound %8.0f B)'
          % (('RGB only' if drop_alpha else 'RGBA' if nc==4 else 'RGB'), K, np.log2(K), hm, hm*N/8, pal_delta, pal_set))
    return K, hm, N, pal_delta

for fn in sys.argv[1:]:
    print(fn.split('/')[-1])
    W,Hh,px,pal,bpp,ri = bmpio.load(fn)
    variant(fn, False)
    if px.shape[2]==4: variant(fn, True)
