import numpy as np, wd, sys
ts = wd.load(sys.argv[1])
tot_q = 0; tot_bits = 0; tot_other = 0
glob = np.zeros(15)
rows = []
for name, dt, shape, d in ts:
    if dt == 0:
        h = np.bincount((d.astype(int)+7).ravel(), minlength=15); glob += h
        p = h[h>0]/h.sum(); H = -(p*np.log2(p)).sum()
        tot_q += d.size; tot_bits += H*d.size
        rows.append((name, shape, d.size, H, np.abs(d).mean(), d.astype(float).std()))
    else:
        tot_other += d.size * (2 if dt==1 else 4) if d.size else 0
for r in rows: print("%-60s %-12s %8d H=%.3f mean|q|=%.2f std=%.2f" % (r[0], r[1], r[2], r[3], r[4], r[5]))
p = glob/glob.sum(); print("int4 params:", tot_q, " sum per-tensor H0 bytes:", tot_bits/8, " global H0:", -(p[p>0]*np.log2(p[p>0])).sum())
print("global hist:", glob.astype(int))
print("other bytes (raw):", tot_other)
