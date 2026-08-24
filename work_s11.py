import sys,collections,numpy as np
def parse(path):
    trials=[]; cur=None
    for ln in open(path):
        if ln.startswith('#TRIAL '):
            _,pl,fl=ln.split(None,2); cur=dict(plane=int(pl[5:]),flags=fl.strip(),rows=[]); trials.append(cur)
        elif ln.startswith('#ROW ') and cur is not None:
            cur['rows'].append(int(ln.split()[4]))
    return trials
for path in sys.argv[1:]:
    nm=path.split('/')[-1].split('.')[0]
    tr=[t for t in parse(path) if t['rows']]
    # group 0 = maximal prefix with no repeated (plane,flags)
    g={}; 
    for t in tr:
        k=(t['plane'],t['flags'])
        if k in g: break
        g[k]=t
    byp=collections.defaultdict(dict)
    for (pl,fl),t in g.items(): byp[pl][fl]=np.diff(np.concatenate([[0],np.array(t['rows'],dtype=np.int64)]))
    print("==",nm)
    for pl,m in sorted(byp.items()):
        for fam in (('slow','p1','p2'),('refs+slow','refs+p1','refs+p2')):
            ks=[k for k in fam if k in m]
            if len(ks)<2: continue
            n=min(len(m[k]) for k in ks); M=np.vstack([m[k][:n] for k in ks])
            tot=M.sum(1); best=tot.min(); orc=M.min(0).sum()
            w=M.argmin(0); c=collections.Counter(ks[i] for i in w)
            print("   plane%d %-28s rows=%d best %s=%d  row-oracle %d (%+.2f%%)  wins %s"%(
                pl,'/'.join(ks),n,ks[int(tot.argmin())],best,orc,100*(orc-best)/best,dict(c)))
