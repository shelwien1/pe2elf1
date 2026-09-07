#!/usr/bin/env python3
"""Evaluate floor-post predictors on the dump `fit` writes under FITDUMP.

Each line is: D <block> <postindex> <x> <Ytrue> <Yfit> <posts> <rawfit>
where Yfit is the libvorbis fit quantised to post units and rawfit is the
same value before quantisation.  Blocks are split in half; predictors are
learned on the first half and scored on the second, so nothing here is an
in-sample number.
"""
import sys, math, collections

def load(p):
    B = collections.defaultdict(dict)
    for line in open(p):
        f = line.split()
        if f[0] != 'D':
            continue
        blk, i, x, yt, yf, posts, raw = map(int, f[1:8])
        B[blk][i] = (x, yt, yf, posts, raw)
    return B

def H(v):
    c = collections.Counter(v); n = len(v)
    return -sum(k / n * math.log2(k / n) for k in c.values())

def mode(v):
    return collections.Counter(v).most_common(1)[0][0] if v else 0

def neigh(x):
    """the decoder's low/high neighbour for each post, in list order"""
    n = len(x); lo = [0] * (n - 2); hi = [0] * (n - 2)
    for i in range(n - 2):
        l, h, lx, hx, cx = 0, 1, 0, x[1], x[i + 2]
        for k in range(i + 2):
            if x[k] > lx and x[k] < cx: l, lx = k, x[k]
            if x[k] < hx and x[k] > cx: h, hx = k, x[k]
        lo[i], hi[i] = l, h
    return lo, hi

def run(B, sel, shift, bias=None, useneigh=False):
    """returns the per-post correction and, per post index, what it was"""
    out = []; seen = collections.defaultdict(list)
    for blk in sel:
        d = B[blk]; n = len(d)
        x = [d[i][0] for i in range(n)]
        lo, hi = neigh(x); r = [0] * n
        for i in range(n):
            yt = d[i][1]; yf = d[i][4] >> shift
            pred = yf
            if useneigh and i >= 2:
                l, h = lo[i - 2], hi[i - 2]
                # both neighbours are decoded before post i, so this is causal
                # and costs nothing in the meta
                pred += int(round(0.5 * (r[l] + r[h])))
            if bias is not None:
                pred += bias.get(i, 0)
            e = yt - pred
            r[i] = yt - yf
            out.append(e); seen[i].append(e)
    return out, seen

def main(path, shift=3, basecost=None):
    B = load(path); blks = sorted(B); half = len(blks) // 2
    tr, te = blks[:half], blks[half:]
    res = {}
    res['psy fit alone'] = H(run(B, te, shift)[0])
    _, L = run(B, tr, shift)
    b1 = {k: mode(v) for k, v in L.items()}
    res['+ per-post bias'] = H(run(B, te, shift, bias=b1)[0])
    res['+ neighbour residual'] = H(run(B, te, shift, useneigh=True)[0])
    _, L2 = run(B, tr, shift, useneigh=True)
    b2 = {k: mode(v) for k, v in L2.items()}
    res['+ both'] = H(run(B, te, shift, useneigh=True, bias=b2)[0])
    for k, v in res.items():
        print("  %-24s H(test) %.3f bits" % (k, v))
    best = min(res.values())
    print("  best %.3f bits" % best, end='')
    if basecost:
        print("  -> %.0f%% of the floor stream saved" % (100 * (basecost - best) / basecost))
    else:
        print()

if __name__ == '__main__':
    main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 3,
         float(sys.argv[3]) if len(sys.argv) > 3 else None)
