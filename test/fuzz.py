#!/usr/bin/env python3
"""fuzz.py BIN WORKDIR SEED N files... -- aacpack against inputs it was not
made for (after aac2tsv's tests/fuzz.py).

Half the cases mutate an input (random bytes, a cut, bit flips, junk put in,
ranges taken out, two inputs spliced, a range repeated) and check that d(c(x))
is x, and that c stored no unit as its bytes because it would not come out as
it was (a walk that writes what it did not read: -v's "would not come out").
The other half compress an input and mutate the compressed stream (bit flips,
a cut, bytes after it), and check that d ends with an error or with some
output, as it should: no crash, no sanitizer report, no hang (a timeout).
"""
import os
import random
import subprocess
import sys

binp, work, seed, n = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
files = sys.argv[5:]
rnd = random.Random(seed)
srcs = [open(f, 'rb').read() for f in files]
os.makedirs(work, exist_ok=True)
fin, fcmp, fout = (os.path.join(work, x) for x in ('in.bin', 'x.apk', 'out.bin'))
bad = 0
unverified = 0


def run(args, timeout=120):
    try:
        p = subprocess.run([binp] + args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
        return p.returncode, p.stderr.decode('latin1')
    except subprocess.TimeoutExpired:
        return 'timeout', ''


def mutate(src):
    kind = rnd.randrange(8)
    if kind == 0:  # random bytes
        return bytes(rnd.randrange(256) for _ in range(rnd.randrange(0, 4000)))
    if kind == 1:  # a cut
        return src[:rnd.randrange(0, len(src) + 1)]
    if kind == 2:  # bit flips
        b = bytearray(src[:rnd.randrange(1, min(len(src), 200000) + 1)])
        for _ in range(rnd.choice([1, 3, 10, 100])):
            p = rnd.randrange(len(b))
            b[p] ^= 1 << rnd.randrange(8)
        return bytes(b)
    if kind == 3:  # junk put in
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 5)):
            p = rnd.randrange(len(b) + 1)
            b[p:p] = bytes(rnd.randrange(256) for _ in range(rnd.randrange(1, 50)))
        return bytes(b)
    if kind == 4:  # ranges taken out
        b = bytearray(src[:200000])
        for _ in range(rnd.randrange(1, 5)):
            if len(b) < 2:
                break
            p = rnd.randrange(len(b))
            del b[p:p + rnd.randrange(1, 300)]
        return bytes(b)
    if kind == 5:  # two inputs spliced
        o = rnd.choice(srcs)
        return src[:rnd.randrange(len(src) + 1)][:150000] + o[rnd.randrange(len(o) + 1):][:150000]
    if kind == 6:  # a range repeated
        b = bytearray(src[:150000])
        if b:
            p = rnd.randrange(len(b))
            q = min(len(b), p + rnd.randrange(1, 20000))
            b[q:q] = b[p:q] * rnd.randrange(1, 4)
        return bytes(b)
    # bytes flipped near frame headers (0xff 0xf?, 0x56 0xe?)
    b = bytearray(src[:200000])
    for _ in range(rnd.randrange(1, 20)):
        p = rnd.randrange(len(b)) if b else 0
        q = b.find(b'\xff', p)
        if q < 0 or q + 8 >= len(b):
            continue
        b[q + rnd.randrange(1, 8)] = rnd.randrange(256)
    return bytes(b)


for it in range(n):
    src = rnd.choice(srcs)
    if it % 2 == 0:
        d = mutate(src)
        open(fin, 'wb').write(d)
        r, err = run(['c', '-v', fin, fcmp])
        if r != 0:
            print('case %d: c failed (%s): %s' % (it, r, err.strip()[-300:]))
            bad += 1
            continue
        for line in err.splitlines():
            if 'would not come out as it was' in line and 'stored as its bytes' in line:
                unverified += 1
                print('case %d: %s' % (it, line.strip()))
                open(os.path.join(work, 'unverified%d.bin' % it), 'wb').write(d)
        r, err = run(['d', fcmp, fout])
        if r != 0 or open(fout, 'rb').read() != d:
            print('case %d: does not round-trip (%s): %s' % (it, r, err.strip()[-300:]))
            open(os.path.join(work, 'bad%d.bin' % it), 'wb').write(d)
            bad += 1
    else:
        open(fin, 'wb').write(src[:rnd.randrange(1, 300000)])
        r, err = run(['c', fin, fcmp])
        if r != 0:
            print('case %d: c failed (%s)' % (it, r))
            bad += 1
            continue
        b = bytearray(open(fcmp, 'rb').read())
        k = rnd.randrange(3)
        if k == 0:
            for _ in range(rnd.choice([1, 2, 5, 20])):
                p = rnd.randrange(len(b))
                b[p] ^= 1 << rnd.randrange(8)
        elif k == 1:
            b = b[:rnd.randrange(len(b))]
        else:
            b += bytes(rnd.randrange(256) for _ in range(rnd.randrange(1, 10)))
        open(fcmp, 'wb').write(bytes(b))
        r, err = run(['d', fcmp, fout], timeout=60)
        if r not in (0, 1) or 'Sanitizer' in err or 'runtime error' in err:
            print('case %d: d of a broken stream: %s %s' % (it, r, err.strip()[-500:]))
            open(os.path.join(work, 'badd%d.apk' % it), 'wb').write(bytes(b))
            bad += 1
print('%d cases, %d bad, %d units that would not come out as they were' % (n, bad, unverified))
sys.exit(1 if bad else 0)
