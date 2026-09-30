import json, numpy as np, alloc, sys
tabd = json.load(open('../runs/table_prop.json')); KS = tabd['KS']; tab = tabd['tab']
m30 = alloc.load_meas('../runs/gp30.txt'); m20 = alloc.load_meas('../runs/gp20.txt'); m15 = alloc.load_meas('../runs/gp15.txt')
# RTN measurements for embedding / prior_embedding at 1.5 (the sweep), GPTQ for the rest
mr15 = alloc.load_meas('../runs/g15.txt')
sets = [(3.0, m30), (2.0, m20), (1.5, {g: v for g, v in m15.items() if g not in ('prior_embedding',)})]
for g in ('embedding', 'prior_embedding'):
    sets.append((1.5, {g: mr15[g]}))
c = alloc.fit_c(tab, sets)
# floor: a group's c may not go below 10% of the median c of its layer type (noise protection)
def typ(g): return g.split('.')[-1] if g.startswith('L') else g
med = {}
for g, v in c.items():
    if v is not None: med.setdefault(typ(g), []).append(v)
med = {t: float(np.median([max(x, 0) for x in v])) for t, v in med.items()}
for g in c:
    if c[g] is not None: c[g] = max(c[g], 0.1 * med[typ(g)])
json.dump(c, open('../runs/c_prop.json', 'w'), indent=0)
for B in [250, 350, 450]:
    ch, L, dR = alloc.solve(tab, KS, c, B)
    json.dump({'groups': ch}, open('../runs/choice_B%d.json' % B, 'w'), indent=0)
    ks = {}
    for g, k in ch.items(): ks[k] = ks.get(k, 0) + 1
    print("B=%d predicted dL %.0f  entropy saved %.0f bytes  k histogram %s" % (B, L, dR, dict(sorted(ks.items()))))
