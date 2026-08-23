#!/usr/bin/env python3
# sweep.py -- the clamp test.
#
# IDX-FORMAT.md sec.5: a pattern is a search space and the optimizer will visit
# the ends of it, so anything used as a size, a shift or a divisor needs a range
# check at the point of use.  A parameter that can crash the program when swept
# is a bug in the *consumer*, not in the sweep.
#
# This is what checks that.  It takes the tuning build, flips pattern bits at
# random across every "!MAP!" parameter at once, and round-trips an image
# through the patched binary.  Encoder and decoder are the same binary, so the
# stream is expected to change and to decode back to the original pixels; what
# it is looking for is a crash, a hang, or a round trip that no longer matches.
#
#   python3 IDX/sweep.py [rounds] [exe] [image...]
#
# Every round reports the parameters it moved and the outcome.  A FAIL names
# the round; re-run with that round's seed to reproduce it.
import os
import random
import re
import subprocess
import sys
import tempfile

rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 8
exe = sys.argv[2] if len(sys.argv) > 2 else './bmf'
images = sys.argv[3:] or ['testfiles/t1.bmp', 'testfiles/t8p.bmp',
                          'testfiles/t24.bmp']

data = bytearray(open(exe, 'rb').read())
maps = []
for m in re.finditer(rb'!MAP!(.*?)!(.*?)\x00(.*?)\x00', data, re.S):
    name, base, pat = m.group(1), m.group(2), m.group(3)
    at = m.start() + 5 + len(name) + 1 + len(base) + 1
    maps.append((name.decode(), at, len(pat)))
if not maps:
    sys.exit('no !MAP! markers -- this is a release build, nothing to sweep')
print('%d parameters, %d pattern bits' % (len(maps), sum(n for _, _, n in maps)))

fails = 0
for r in range(rounds):
    rng = random.Random(r)
    patched = bytearray(data)
    moved = 0
    for _, at, n in maps:
        for i in range(n):
            if rng.random() < 0.15:
                patched[at + i] ^= 1
                moved += 1
    with tempfile.TemporaryDirectory() as d:
        bin_ = os.path.join(d, 'bmf')
        open(bin_, 'wb').write(patched)
        os.chmod(bin_, 0o755)
        ok = True
        for img in images:
            c, o = os.path.join(d, 'c.bmf'), os.path.join(d, 'o.bmp')
            try:
                e = subprocess.run([bin_, 'c', img, c], capture_output=True,
                                   timeout=300)
                if e.returncode or not os.path.exists(c):
                    print('round %d FAIL encode %s rc=%d' % (r, img, e.returncode))
                    ok = False
                    continue
                e = subprocess.run([bin_, 'd', c, o], capture_output=True,
                                   timeout=300)
                if e.returncode or not os.path.exists(o):
                    print('round %d FAIL decode %s rc=%d' % (r, img, e.returncode))
                    ok = False
                    continue
            except subprocess.TimeoutExpired:
                print('round %d FAIL timeout %s' % (r, img))
                ok = False
                continue
            if open(img, 'rb').read() != open(o, 'rb').read():
                print('round %d FAIL round trip %s' % (r, img))
                ok = False
        print('round %d: %d bits moved -- %s' % (r, moved, 'ok' if ok else 'FAILED'))
        fails += not ok

print('%d/%d rounds clean' % (rounds - fails, rounds))
sys.exit(1 if fails else 0)
