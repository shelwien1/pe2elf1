import json, numpy as np, alloc, sys
tabd = json.load(open('../runs/table_prop.json')); KS = tabd['KS']; tab = tabd['tab']
tu = json.load(open('../runs/table_up.json'))
c = json.load(open('../runs/c_prop.json'))
m30 = alloc.load_meas('../runs/gp30.txt'); m20 = alloc.load_meas('../runs/gp20.txt')
mw = alloc.load_meas('../runs/wr_b128k.txt') if len(sys.argv) > 2 else {}
wr = {l.split()[1]: int(l.split()[0]) - 49832 for l in open('../runs/wr.txt')}
tab2 = {g: dict(v) for g, v in tab.items()}
for g, d in tu.items():
    pts = [(m20[g], d['u']['2.0']), (m30[g], d['u']['3.0'])]
    if g in wr: pts.append((wr[g], d['w']['2.0'][1]))
    num = sum(a * b for a, b in pts); den = sum(b * b for a, b in pts)
    c[g] = max(num / den, 1e-9)
    tab2[g] = d['w']
json.dump(c, open('../runs/c_v2.json', 'w'), indent=0)
json.dump(tab2, open('../runs/table_v2.json', 'w'))
for B in [int(x) for x in sys.argv[1].split(',')]:
    ch, L, dR = alloc.solve(tab2, KS, c, B)
    json.dump({'groups': ch, 'weighted_up': True}, open('../runs/choice2_B%d.json' % B, 'w'), indent=0)
    ups = {g: k for g, k in ch.items() if g.endswith('.up')}
    print("B=%d pred %.0f saved %.0f  ups: %s" % (B, L, dR, ' '.join('%s:%s' % (g, k) for g, k in sorted(ups.items()))))
