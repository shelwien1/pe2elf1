#!/usr/bin/env python3
# sweep.py -- the clamp test.
#
# IDX-FORMAT.md sec.5: a pattern is a search space and the optimizer will visit
# the ends of it, so anything used as a size, a shift or a divisor needs a range
# check at the point of use.  A parameter that can crash the program when swept
# is a bug in the *consumer*, not in the sweep.
#
# This is what checks that.  It patches the "!MAP!" patterns in a copy of the
# tuning build and round-trips images through the result.  The stream is
# expected to change; what it looks for is a crash, a hang, or a round trip that
# stops matching -- encoder and decoder are the same binary, so a mismatch means
# the two halves stopped agreeing.
#
#   python3 IDX/sweep.py [--random N] [--exe ./bmf] [image...]
#
# The default walks every parameter to each of its two extremes, one at a time.
# That is deterministic, covers the whole declaration, and is what found the
# thirteen unguarded parameters the port started with.  --random N instead runs
# N rounds with every parameter perturbed at once, which reaches combinations
# the one-at-a-time walk cannot.
import os
import random
import re
import subprocess
import sys
import tempfile

args = sys.argv[1:]
rounds = 0
exe = './bmf'
while args and args[0].startswith('--'):
    if args[0] == '--random':
        rounds = int(args[1])
        args = args[2:]
    elif args[0] == '--exe':
        exe = args[1]
        args = args[2:]
    else:
        sys.exit('unknown option %s' % args[0])
images = args or ['testfiles/t1.bmp', 'testfiles/t8p.bmp']

data = bytearray(open(exe, 'rb').read())
maps = []
for m in re.finditer(rb'!MAP!(.*?)!(.*?)\x00(.*?)\x00', data, re.S):
    name, base, pat = m.group(1), m.group(2), m.group(3)
    at = m.start() + 5 + len(name) + 1 + len(base) + 1
    maps.append((name.decode(), at, len(pat)))
if not maps:
    sys.exit('no !MAP! markers -- this is a release build, nothing to sweep')
print('%d parameters, %d pattern bits' % (len(maps), sum(n for _, _, n in maps)))

work = tempfile.mkdtemp()
bin_ = os.path.join(work, 'bmf')
out_c, out_o = os.path.join(work, 'c.bmf'), os.path.join(work, 'o.bmp')


def run(patched):
    """None if the patched binary round-trips every image, else why not."""
    open(bin_, 'wb').write(patched)
    os.chmod(bin_, 0o755)
    for img in images:
        for f in (out_c, out_o):
            if os.path.exists(f):
                os.unlink(f)
        try:
            e = subprocess.run([bin_, 'c', img, out_c], capture_output=True,
                               timeout=300)
            if e.returncode or not os.path.exists(out_c):
                return 'encode %s rc=%s' % (img, e.returncode)
            e = subprocess.run([bin_, 'd', out_c, out_o], capture_output=True,
                               timeout=300)
            if e.returncode or not os.path.exists(out_o):
                return 'decode %s rc=%s' % (img, e.returncode)
        except subprocess.TimeoutExpired:
            return 'timeout %s' % img
        if open(img, 'rb').read() != open(out_o, 'rb').read():
            return 'round trip %s' % img
    return None


bad = 0
if rounds:
    for r in range(rounds):
        rng = random.Random(r)
        patched = bytearray(data)
        moved = 0
        for _, at, n in maps:
            for i in range(n):
                if rng.random() < 0.15:
                    patched[at + i] ^= 1
                    moved += 1
        why = run(patched)
        print('round %d: %d bits moved -- %s' % (r, moved, why or 'ok'))
        bad += why is not None
    print('%d/%d rounds clean' % (rounds - bad, rounds))
else:
    for name, at, n in maps:
        for fill in (b'0', b'1'):
            patched = bytearray(data)
            patched[at:at + n] = fill * n
            why = run(patched)
            if why:
                print('%s all-%s: %s' % (name, fill.decode(), why))
                bad += 1
    print('%d of %d parameters fail at an extreme' % (bad, 2 * len(maps)))

sys.exit(1 if bad else 0)
