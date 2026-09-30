import json, numpy as np, alloc, sys
KS = json.load(open('../runs/table_prop.json'))['KS']
tab2 = json.load(open('../runs/table_v2.json'))
c = json.load(open(sys.argv[2]))
tag = sys.argv[3]
for B in [int(x) for x in sys.argv[1].split(',')]:
    ch, L, dR = alloc.solve(tab2, KS, c, B)
    json.dump({'groups': ch, 'weighted_up': True, 'f32_mant': 4}, open('../runs/choice%s_B%d.json' % (tag, B), 'w'), indent=0)
    print("B=%d pred %.0f saved %.0f lossless %d" % (B, L, dR, sum(1 for k in ch.values() if k == 1.0)))
