#!/usr/bin/env python3
"""fuzz.py BIN WORKDIR SEED N files... : mutate inputs, check c/d round trips are exact.
Reports any 'does not round-trip' fallbacks (R/W asymmetry) and any mismatch or crash."""
import sys, os, random, subprocess
binp, work, seed, n = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
files = sys.argv[5:]
rnd = random.Random(seed)
srcs = [open(f, 'rb').read() for f in files]
os.makedirs(work, exist_ok=True)
fin, ftsv, fout = os.path.join(work, 'in.bin'), os.path.join(work, 'x.tsv'), os.path.join(work, 'out.bin')
bad = 0; fallbacks = 0
for it in range(n):
    src = rnd.choice(srcs)
    kind = rnd.randrange(8)
    if kind == 0:   # random bytes
        d = bytes(rnd.randrange(256) for _ in range(rnd.randrange(0, 3000)))
    elif kind == 1: # truncate
        d = src[:rnd.randrange(0, len(src) + 1)]
    elif kind == 2: # bit flips
        b = bytearray(src[:rnd.randrange(1, min(len(src), 200000) + 1)])
        for _ in range(rnd.choice([1, 3, 10, 100])):
            p = rnd.randrange(len(b)); b[p] ^= 1 << rnd.randrange(8)
        d = bytes(b)
    elif kind == 3: # insert junk
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 5)):
            p = rnd.randrange(len(b) + 1); b[p:p] = bytes(rnd.randrange(256) for _ in range(rnd.randrange(1, 50)))
        d = bytes(b)
    elif kind == 4: # delete ranges
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 5)):
            p = rnd.randrange(len(b)); del b[p:p + rnd.randrange(1, 50)]
        d = bytes(b)
    elif kind == 5: # splice two files
        o = rnd.choice(srcs)
        d = src[:rnd.randrange(len(src))][:100000] + o[rnd.randrange(len(o)):][:100000]
    elif kind == 6: # byte overwrite bursts
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 20)):
            p = rnd.randrange(len(b)); b[p] = rnd.randrange(256)
        d = bytes(b)
    else:           # header-ish corruption: flip bits in the first bytes of frames
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 30)):
            p = rnd.randrange(len(b))
            while p < len(b) - 1 and not (b[p] == 0xff and (b[p+1] & 0xf0) == 0xf0): p += 1
            q = min(len(b) - 1, p + rnd.randrange(0, 12)); b[q] ^= 1 << rnd.randrange(8)
        d = bytes(b)
    open(fin, 'wb').write(d)
    r = subprocess.run([binp, 'c', fin, ftsv], capture_output=True, text=True)
    if r.returncode != 0:
        print('c failed', it, kind, r.stderr[:300]); bad += 1; open(os.path.join(work, 'bad%d.bin' % it), 'wb').write(d); continue
    if 'does not round-trip' in r.stderr:
        fallbacks += r.stderr.count('does not round-trip')
        open(os.path.join(work, 'fb%d.bin' % it), 'wb').write(d)
    r2 = subprocess.run([binp, 'd', ftsv, fout], capture_output=True, text=True)
    if r2.returncode != 0 or open(fout, 'rb').read() != d:
        print('MISMATCH', it, kind, r2.stderr[:300]); bad += 1; open(os.path.join(work, 'bad%d.bin' % it), 'wb').write(d)
print('done', n, 'bad', bad, 'fallback frames', fallbacks)
