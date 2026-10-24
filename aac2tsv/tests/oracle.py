#!/usr/bin/env python3
"""oracle.py BIN AACDEC WORK TESTDIR LTPFILE : inject rarely used AAC tools into TSVs (pulse, LTP, SSR gain
control, CCE), rebuild with 'd', check that 'c' gives back the same TSV and that aacdec decodes it.
TESTDIR holds s1.aac and m1.aac of the test set, LTPFILE is an AAC LTP profile ADTS stream (e.g. ffmpeg
-strict -2 -profile:a aac_ltp)."""
import sys, os, subprocess
BIN, DEC, W, T, LTPF = sys.argv[1:6]
os.makedirs(W, exist_ok=True)
ELEM = {'sce','cpe','cce','lfe','dse','pce','fil','end'}
def run(*a):
    r = subprocess.run(list(a), capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr
def to_tsv(src):
    out = os.path.join(W, os.path.basename(src) + '.tsv'); rc, msg = run(BIN, 'c', src, out); assert rc == 0, msg
    return [l.rstrip('\n').split('\t') for l in open(out)]
def write(recs, name):
    p = os.path.join(W, name); open(p, 'w').write(''.join('\t'.join(r) + '\n' for r in recs)); return p
def check(name, recs):
    tsv = write(recs, name + '.tsv'); aac = os.path.join(W, name + '.aac')
    rc, msg = run(BIN, 'd', tsv, aac)
    if rc: print(name, 'd FAILED', msg[:300]); return
    back = os.path.join(W, name + '.back.tsv'); rc, msg = run(BIN, 'c', '-v', aac, back)
    same = [l for l in open(back) if not l.startswith('eof')] == [l for l in open(tsv) if not l.startswith('eof')]
    rc2, dmsg = run(DEC, '-v', aac, os.path.join(W, name + '.wav'))
    last = [l for l in dmsg.splitlines() if 'packets' in l]
    print('%-14s tsv round trip %s; c: %s; aacdec: %s' % (name, 'same' if same else 'DIFFERENT', msg.strip().splitlines()[-1], last[-1] if last else dmsg[-200:]))
def channels(recs):
    """yield (index of ics record, ws, max_sfb) with ws/max_sfb from its ics_info or the cpe common window"""
    cw_info = None
    for i, r in enumerate(recs):
        if r[0] == 'cpe':
            cw_info = (int(r[4]), int(r[6])) if r[2] == '1' else None
        elif r[0] in ELEM:
            cw_info = None
        if r[0] == 'ics':
            if len(r) > 2: yield i, int(r[3]), int(r[5])
            elif cw_info: yield i, cw_info[0], cw_info[1]
def after_sf(recs, i):
    j = i + 1
    while j < len(recs) and recs[j][0] in ('sec', 'sf'): j += 1
    return j
# A. pulse data in long windows
recs = to_tsv(os.path.join(T, 's1.aac')); ins = []
for i, ws, msfb in channels(recs):
    if ws != 2 and msfb > 3: ins.append(after_sf(recs, i))
for j in reversed(ins): recs.insert(j, ['pulse', '1', '2', '3', '1', '5', '2'])
check('pulse', recs)
# B. LTP in an LTP-profile stream
recs = to_tsv(LTPF)
for r in recs:
    if r[0] == 'cpe' and r[2] == '1' and r[4] != '2' and r[7] == '0':
        n = min(int(r[6]), 40); bits = '1' * n or '-'; bits2 = ('1' * (n - 1) + '0') if n else '-'
        r[7:8] = ['1', '1', '1000', '2', bits, '1', '600', '5', bits2]
    elif r[0] == 'ics' and len(r) > 2 and r[3] != '2' and r[6] == '0':
        n = min(int(r[5]), 40)
        r[6:7] = ['1', '1', '900', '3', ('01' * (n // 2) + '1' * (n % 2)) or '-']
check('ltp', recs)
# C. SSR gain control
recs = to_tsv(os.path.join(T, 's1.aac'))
for r in recs:
    if r[0] == 'adtsh': r[4] = '2'
ins = []
for i, ws, msfb in channels(recs):
    j = after_sf(recs, i)
    while j < len(recs) and recs[j][0] in ('pulse', 'tns'): j += 1
    wd = {0: 1, 1: 2, 2: 8, 3: 2}[ws]
    f = ['gc', '2']
    for bd in range(2):
        for w in range(wd):
            f += ['1', '5', '1']
    ins.append((j, f))
for j, f in reversed(ins): recs.insert(j, f)
check('ssr_gc', recs)
# D. CCE: a coupling channel (from a mono stream) after each CPE
for ind_sw in ('0', '1'):
    st = to_tsv(os.path.join(T, 's1.aac')); mo = to_tsv(os.path.join(T, 'm1.aac'))
    sce = []; cur = None
    for r in mo:
        if r[0] == 'sce': cur = []; sce.append(cur); continue
        if r[0] in ELEM or r[0] in ('adts', 'adtsh', 'eof', 'raw') or r[0].startswith('#'): cur = None; continue
        if cur is not None: cur.append(r)
    out = []; k = 0
    for r in st:
        if r[0] == 'adtsh': r[7] = '0'
        if r[0] == 'adts':
            out.append(r)
            out.append(['pce', '0', '1', '3', '1', '0', '0', '0', '0', '1', '0', '0', '0', '1', '0', ind_sw, '0', '-'])
            continue
        if r[0] == 'end' and k < len(sce):
            out.append(['cce', '0', ind_sw, '0', '1', '0', '3', '0', '0', '0'])
            out += [list(x) for x in sce[k]]
            out.append(['cgain', '0'] if ind_sw == '1' else ['cgain', '1', '0'])
            k += 1
        out.append(r)
    check('cce_sw' + ind_sw, out)
