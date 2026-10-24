#!/usr/bin/env python3
"""tsvfuzz.py BIN WORK SEED N tsv... : mutate TSV files, run 'BIN d'; report crashes, sanitizer errors, hangs."""
import sys, os, random, subprocess
binp, work, seed, n = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
srcs = [open(f).read().split('\n') for f in sys.argv[5:]]
rnd = random.Random(seed); os.makedirs(work, exist_ok=True)
fin, fout = os.path.join(work, 'm.tsv'), os.path.join(work, 'm.out')
names = sorted({l.split('\t')[0] for s in srcs for l in s if l and not l.startswith('#')})
def mut_field(f):
    k = rnd.randrange(6)
    if k == 0: return str(rnd.choice([0, 1, -1, 2, 7, 8, 15, 16, 31, 63, 64, 127, 255, 256, 1023, 8191, 65535, 2**31, -2**31, 10**17]))
    if k == 1 and f.lstrip('-').isdigit(): return str(int(f) + rnd.choice([-2, -1, 1, 2, 100]))
    if k == 2: return ''.join(rnd.choice('0123456789abcdef') for _ in range(rnd.randrange(0, 40))) or '-'
    if k == 3: return ''.join(rnd.choice('01') for _ in range(rnd.randrange(0, 70))) or '-'
    if k == 4: return '-'
    return rnd.choice(['x', '', '+1', '1.5', ' 1', '0x10'])
crashes = 0
for it in range(n):
    L = list(rnd.choice(srcs))
    for _ in range(rnd.choice([1, 1, 2, 3, 5, 10])):
        if len(L) < 4: break
        i = rnd.randrange(1, len(L) - 1); k = rnd.randrange(9)
        f = L[i].split('\t')
        if k == 0 and len(f) > 1: j = rnd.randrange(1, len(f)); f[j] = mut_field(f[j]); L[i] = '\t'.join(f)
        elif k == 1 and len(f) > 1: del f[rnd.randrange(1, len(f))]; L[i] = '\t'.join(f)
        elif k == 2: f.insert(rnd.randrange(1, len(f) + 1), mut_field('0')); L[i] = '\t'.join(f)
        elif k == 3: del L[i]
        elif k == 4: L.insert(i, L[i])
        elif k == 5 and i + 1 < len(L): L[i], L[i + 1] = L[i + 1], L[i]
        elif k == 6: f[0] = rnd.choice(names); L[i] = '\t'.join(f)
        elif k == 7: L = L[:i] + [L[-2], L[-1]]
        else: L.insert(i, rnd.choice(rnd.choice(srcs)[1:]))
    open(fin, 'w').write('\n'.join(L))
    try:
        r = subprocess.run([binp, 'd', fin, fout], capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        print('HANG', it); crashes += 1; os.rename(fin, os.path.join(work, 'hang%d.tsv' % it)); continue
    if r.returncode < 0 or r.returncode > 1 or 'ERROR' in r.stderr or 'runtime error' in r.stderr:
        print('CRASH', it, r.returncode, r.stderr[-400:]); crashes += 1
        os.rename(fin, os.path.join(work, 'crash%d.tsv' % it))
print('done', n, 'crashes', crashes)
