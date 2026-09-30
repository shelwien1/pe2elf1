import json, sys
t2 = json.load(open('../runs/table_v2.json')); tu = json.load(open('../runs/table_up.json'))
c2 = json.load(open('../runs/c_v2.json'))
def meas(p): return {l.split()[1]: int(l.split()[0]) - 49832 for l in open(p)}
m30, m20, m15g, wr = meas('../runs/gp30.txt'), meas('../runs/gp20.txt'), meas('../runs/gp15.txt'), meas('../runs/wr.txt')
ops = [(json.load(open(a)), meas(b)) for a, b in zip(sys.argv[2::2], sys.argv[3::2])]
c = dict(c2)
for g in t2:
    if g in ('embedding', 'prior_embedding'): continue
    if g.endswith('.up'):
        p = [(m20[g], tu[g]['u']['2.0']), (m30[g], tu[g]['u']['3.0'])]
        if g in wr: p.append((wr[g], tu[g]['w']['2.0'][1]))
    else:
        p = [(m20[g], t2[g]['2.0'][1]), (m30[g], t2[g]['3.0'][1])]
        if g in m15g: p.append((m15g[g], t2[g]['1.5'][1]))
    for opE, om in ops:
        if g in opE and g in om: p.append((om[g], opE[g][1]))
    num = sum(a * b for a, b in p); den = sum(b * b for a, b in p)
    c[g] = max(num / den, 1e-9)
json.dump(c, open(sys.argv[1], 'w'), indent=0)
