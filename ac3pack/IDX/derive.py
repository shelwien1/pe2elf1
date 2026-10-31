#!/usr/bin/env python3
"""derive.py IDX -- levels -5 and -1 cut from level -9: IDX/l5/*.idx and IDX/l1/*.idx written from
IDX/l9/*.idx. -5 keeps -9's ten counters, mixer 1 and the mantissa plane (no APMs, no mixers 2 and
3); -1 four of the ten (FOUR, by family, renamed a-d), each index and parameter as -9 has it. Run
it in ac3pack/ as  python3 IDX/derive.py IDX  after tuning -9, then ./mk.sh mod."""
import os, re, sys
idx = sys.argv[1]
FOUR = {'hdr': 'abck', 'sid': 'abhd', 'raw': 'bcdk', 'exp': 'bchk', 'mag': 'bcdk', 'aht': 'abck'}
for fn in sorted(os.listdir(os.path.join(idx, 'l9'))):
    if not fn.endswith('.idx'): continue
    fam = fn[:-4]
    lines = open(os.path.join(idx, 'l9', fn)).read().split('\n')
    head, blocks, nums, cur = [], {}, [], None
    order = []
    for l in lines:
        m = re.match(r'Index (\w+)$', l)
        if m:
            cur = m.group(1); blocks[cur] = []; order.append(cur); continue
        if re.match(r'^!? ?Number ', l) or (l.startswith('#') and cur == 'NUM'):
            cur = 'NUM'; nums.append(l); continue
        if cur is None: head.append(l)
        elif cur == 'NUM': nums.append(l)
        elif l.strip() == '' or l.startswith('#'):
            if l.startswith('#'): blocks[cur].append(l)
        else: blocks[cur].append(l)
    def number(name):
        for l in nums:
            m = re.match(r'^(!? ?)Number (\w+), (.*)$', l)
            if m and m.group(2) == name: return m.group(1), m.group(3)
        raise SystemExit('%s: no Number %s' % (fn, name))
    for L in (5, 1):
        out = []
        for l in head:
            l = l.replace('at level -9', 'at level -%d' % L).replace('MOD/l9/', 'MOD/l%d/' % L).replace('Prefix A9_', 'Prefix A%d_' % L)
            out.append(l)
        if L == 5:
            out.insert(len([x for x in head if x.startswith('#')]), '#  (Cut from level -9: its ten counters, mixer 1 and the mantissa plane.)')
            pairs = [(k, k) for k in 'abcdehijkl'] + [(k, k) for k in 'mtng']
        else:
            out.insert(len([x for x in head if x.startswith('#')]), '#  (Cut from level -9: four of its counters (%s as a-d), mixer 1 and the mantissa plane.)' % ', '.join(FOUR[fam]))
            pairs = list(zip('abcd', FOUR[fam])) + [(k, k) for k in 'mtng']
        for k, src in pairs:
            out.append('Index %s' % k)
            for l in blocks[src]:
                if l.startswith('#'): out.append(l); continue
                out.append(re.sub(r'^ %s_' % src, ' %s_' % k, l))
            out.append('')
        cnt = [k for k, _ in pairs if k in 'abcdehijkl']
        src_of = dict(pairs)
        out.append("#  The counters' limits (r: up to 255 counts) and margins (mw: in 64ths of 1/1024)")
        for k in cnt:
            for p in ('r', 'mw'):
                fz, v = number(p + src_of[k].upper())
                out.append('%sNumber %s%s, %s' % (fz, p, k.upper(), v))
        keep = ['lr', 'mb', 'rT', 'mwT', 'mt', 'lrm', 'mbm', 'rG', 'mwG']
        for p in keep:
            fz, v = number(p)
            out.append('%sNumber %s, %s' % (fz, p, v))
        out += ['', "#  Rows per table, as log2: a table whose index Volume fits in 2^h rows is direct, one that",
                "#  does not is hashed into 2^h rows and contexts share them.  Frozen: left live these only grow."]
        for k in cnt:
            fz, v = number('h' + src_of[k].upper())
            out.append('! Number h%s, %s' % (k.upper(), v))
        for t in 'MTN':
            fz, v = number('h' + t)
            out.append('! Number h%s, %s' % (t, v))
        open(os.path.join(idx, 'l%d' % L, fn), 'w').write('\n'.join(out) + '\n')
print('derived')
