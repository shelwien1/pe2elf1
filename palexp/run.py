# Every ordering, every proxy, and -- for palettes of 256 or fewer colours -- the
# real byte count bmg produces for the reordered index plane.  Tables 6.1 and 2.2
# of PALETTE.md.
#
#   ./mk.sh release && python3 palexp/run.py testfiles/t8p.bmp
#
# BMG names the executable (default ./bmg), BMGTMP the scratch directory.

import os, sys, time, subprocess, tempfile
import numpy as np
import palexp, wbmp
from palexp import order_to_pos as o2p

BMG = os.environ.get('BMG', './bmg')
TD  = os.environ.get('BMGTMP', tempfile.mkdtemp(prefix='palexp'))
os.makedirs(TD, exist_ok=True)

def bmg_size(idx, tag):
    f = os.path.join(TD, tag+'.bmp'); g = os.path.join(TD, tag+'.bmg')
    wbmp.write8(f, idx.astype(np.uint8))
    subprocess.run([BMG,'c',f,g], capture_output=True)
    n = os.path.getsize(g)
    subprocess.run([BMG,'d',g,f+'.out'], capture_output=True)
    ok = open(f,'rb').read() == open(f+'.out','rb').read()
    os.remove(f); os.remove(g); os.remove(f+'.out')
    return n, ok

def run(fn, real=True):
    name = os.path.basename(fn)
    t0 = time.time()
    p = palexp.Pal(fn)
    print('\n=== %s   %dx%d %dbpp   K=%d distinct pixels (%.4f/pixel, %.2f bits)   load %.1fs'
          % (name, p.W, p.H, p.bpp, p.K, p.K/p.N, np.log2(p.K), time.time()-t0))
    print('    index order-0 entropy %.4f bits/pixel (ordering-independent)' % palexp.H(p.cnt))

    if p.rawidx is not None:
        r = p.rawidx.astype(np.int32)
        hm = palexp.H(np.bincount(palexp.med_res(r,256).ravel(), minlength=256))
        s = ''
        if real:
            n, ok = bmg_size(r, 'n')
            s = '%10d%s' % (n, '' if ok else ' RT-FAIL')
        print('    %-30s %8s %8.4f %s   %s' % ('NATIVE  (stored 8-bit index)','-',hm,
              s if s else '%10s'%'-', 'none -- the file as it is'))

    ords = [('packed  (solid int)',    palexp.ord_packed(p),  'free'),
            ('luma    (.299/.587/.114)',palexp.ord_luma(p),   'free'),
            ('hilbert (RGB curve)',     palexp.ord_hilbert(p),'free'),
            ('freq    (descending)',    palexp.ord_freq(p),   'counts'),
            ]
    t = time.time(); o = palexp.ord_tsp_rgb(p)
    if o is not None: ords.append(('tsp-rgb (greedy NN)', o, 'free'))
    t = time.time()
    c, w, g = palexp.optimise_weights(p, gamma=False)
    ords.append(('linear  w=%s' % np.array2string(w, precision=3, separator=','), palexp.ord_linear(p, w), 'weights'))
    print('    weight search %.1fs  ->  w = %s' % (time.time()-t, np.array2string(w, precision=4)))
    t = time.time()
    c2, w2, g2 = palexp.optimise_weights(p, gamma=True, seed=7)
    ords.append(('linear+gamma', palexp.ord_linear(p, w2, g2), 'weights+gamma'))
    print('    +gamma search %.1fs  ->  w = %s  g = %s'
          % (time.time()-t, np.array2string(w2, precision=4), np.array2string(g2, precision=3)))
    o = palexp.ord_cooc_greedy(p)
    ords.append(('cooc    (greedy MinLA)', o, 'permutation'))
    o = palexp.ord_spectral(p)
    if o is not None: ords.append(('spectral(Fiedler)', o, 'permutation'))

    print('    %-30s %8s %8s %10s   %s' % ('ordering','Hpair','Hmed','bmg bytes','side info'))
    base = None
    for nm, o, side in ords:
        pos = o2p(o, p.K)
        hp, hm = p.Hpair(pos), p.Hmed(pos)
        s = ''
        if real and p.K <= 256:
            n, ok = bmg_size(pos[p.I0], 'x')
            s = '%10d%s' % (n, '' if ok else ' RT-FAIL')
            if base is None: base = n
        print('    %-30s %8.4f %8.4f %s   %s' % (nm, hp, hm, s if s else '%10s'%'-', side))

    print('    palette (%d entries x %d bytes):' % (p.K, p.NC))
    for k, v in palexp.palette_costs(p).items(): print('        %-16s %10.0f bytes' % (k, v))
    print('    frequency table:')
    for k, v in palexp.freq_costs(p).items(): print('        %-16s %10.0f bytes' % (k, v))

for fn in sys.argv[1:]:
    run(fn)
