import numpy as np, wd, rq, stats as st, gq, json, sys, os
# usage: mkgroups.py K outdir   -> one file per group, GPTQ at step K (RTN for embedding/prior_embedding)
K = float(sys.argv[1]); od = sys.argv[2]; os.makedirs(od, exist_ok=True)
S = st.load('../runs/stats_b128k.bin')
ts = wd.load('orig.wd')
gs = sorted(set(rq.group_of(q) for q, s in rq.pairs(ts)))
info_all = {}
for g in gs:
    meth = 'rtn' if g in ('embedding', 'prior_embedding') else 'gptq'
    out, info = gq.apply(ts, S, {g: dict(k=K, method=meth)})
    rq.write_tfwc2(out, '%s/%s.tfwc2' % (od, g))
    info_all[g] = dict(dR=sum(v['H0'] - v['H1'] for v in info.values()),
                       err=sum((v['err'] or 0) for v in info.values()))
json.dump(info_all, open(od + '/info.json', 'w'), indent=0)
print(K, len(gs), "total dR %.0f" % sum(v['dR'] for v in info_all.values()))
