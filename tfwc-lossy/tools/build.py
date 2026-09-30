# build.py choice.json out_prefix [mode]
#   choice.json: {"groups": {group: k}, "f32_mant": 7, "uniform": false}
# writes out_prefix.tfwz and out_prefix.tfwc2 (decoded from the tfwz)
import numpy as np, wd, rq, stats as st, gq, json, sys, subprocess, os
cfg = json.load(open(sys.argv[1])); outp = sys.argv[2]
S = st.load(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../runs/stats_b128k.bin'))
ts = wd.load(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'orig.wd'))
T = {t[0]: i for i, t in enumerate(ts)}
uniform = cfg.get('uniform', False)
import rowimp
T0 = {t[0]: t for t in ts}
plan_lines = []
imps = {l: rowimp.up_importance(S, T0, l) for l in range(12)} if cfg.get('weighted_up') else {}
for qn, sn in rq.pairs(ts):
    g = rq.group_of(qn); k = cfg['groups'].get(g, 1.0)
    if k == 1.0: continue
    key = qn[:-len('.weight.q')]
    q = ts[T[qn]][3].astype(np.float64); s = wd.bf16(ts[T[sn]][3]).astype(np.float64)
    W_ = q * s[:, None]
    if cfg.get('weighted_up') and g.endswith('.up'):
        kr = rowimp.row_steps(k, imps[int(g[1:g.index('.')])])
        s2 = s * kr
        plan_lines.append('classes %s %s' % (qn, ' '.join(map(str, rowimp.class_of(kr)))))
    elif uniform or g in cfg.get('unif_groups', []): s2 = np.maximum(k * np.median(np.abs(s)), np.abs(W_).max(1) / 7.0)
    else: s2 = s * k
    s2b = wd.to_bf16(s2)
    sb = cfg.get('scale_bits', 7)
    if sb < 7:  # snap the bf16 scale to sb mantissa bits (round to nearest)
        drop = 7 - sb
        s2b = ((s2b.astype(np.uint32) + (1 << (drop - 1))) >> drop << drop).astype(np.uint16)
    s2 = wd.bf16(s2b).astype(np.float64)
    A = S[key]['A'] / S[key]['nf'] if key in S else None
    meth = 'rtn' if g in ('embedding', 'prior_embedding') else 'gptq'
    qq = gq.requant_tensor(W_, A, s2, meth, 0.0, damp=cfg.get('damp', 0.01))
    lm = cfg.get('lam_mult', 0.0)
    if lm > 0 and meth == 'gptq':
        e_k = gq.werr(W_, qq, s2, A) / qq.size
        qq = gq.requant_tensor(W_, A, s2, meth, lm * e_k, iters=3)
    ts[T[qn]][3] = qq
    ts[T[sn]][3] = s2b
mb = cfg.get('f32_mant', 7)
if mb < 7:
    for t in ts:
        if t[1] == 2 and t[3].size > 6 and t[0] != 'rope.inv_freq':
            u = t[3].astype(np.float32).view(np.uint32).astype(np.uint64); drop = 23 - mb
            u = (u + (1 << (drop - 1)) - 1 + ((u >> drop) & 1)) >> drop << drop
            t[3] = u.astype(np.uint32).view(np.float32)
rq.write_tfwc2(ts, outp + '.pre.tfwc2')
if plan_lines:
    open(outp + '.plan', 'w').write('\n'.join(plan_lines) + '\n')
    cfg['plan'] = outp + '.plan'

tf = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tfwz', 'tfwz')
r = subprocess.run([tf, 'c', outp + '.pre.tfwc2', outp + '.tfwz'] + ([cfg['plan']] if cfg.get('plan') else []), capture_output=True, text=True)
print(r.stdout.strip(), r.stderr.strip())
subprocess.check_call([tf, 'd', outp + '.tfwz', outp + '.tfwc2'])
os.remove(outp + '.pre.tfwc2')
