# fuzz.py BIN WORKDIR SEED N files... (mp3pack's, as aacpack's test/fuzz.py): each
# file compressed by BIN with one model and with -t 2 -c 16k, then its stream (or,
# one time in five, its outside data) mutated N times -- cut, bits flipped, bytes
# inserted, deleted or repeated -- and BIN d run on each. A run must end with 0 (the
# file back) or 1: no signal, no hang (60 s), no sanitizer report on stderr. The bad
# cases stay in WORKDIR (r_*/m.pk, m.out, why.txt), the others are removed.
import os, random, subprocess, sys, shutil
from concurrent.futures import ThreadPoolExecutor
binp, work, seed, n = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
files = sys.argv[5:]
os.makedirs(work, exist_ok=True)
env = dict(os.environ)
def mutate(b, rnd):
    b = bytearray(b)
    k = rnd.randrange(6)
    if k == 0 or len(b) < 16:   # flip bytes
        for _ in range(rnd.randint(1, 4)):
            if b: b[rnd.randrange(len(b))] ^= 1 << rnd.randrange(8) if rnd.random() < .5 else rnd.randrange(1, 256)
    elif k == 1:  # cut
        del b[rnd.randrange(len(b)):]
    elif k == 2:  # insert junk
        p = rnd.randrange(len(b) + 1); b[p:p] = bytes(rnd.randrange(256) for _ in range(rnd.randint(1, 64)))
    elif k == 3:  # delete a range
        p = rnd.randrange(len(b)); del b[p:p + rnd.randint(1, 256)]
    elif k == 4:  # repeat a range
        p = rnd.randrange(len(b)); q = min(len(b), p + rnd.randint(1, 4096)); b[q:q] = b[p:q]
    else:         # a byte past the header
        p = rnd.randrange(11, len(b)) if len(b) > 11 else 0; b[p] = rnd.randrange(256)
    return bytes(b)
def prep(f, mode):
    tag = os.path.basename(f) + ('.t2' if mode else '.t1')
    d = os.path.join(work, 'seed_' + tag); os.makedirs(d, exist_ok=True)
    opts = ['-t', '2', '-c', '16k'] if mode else []
    subprocess.run([binp, 'c'] + opts + [f, d + '/s.pk', d + '/s.out'], check=True, env=env, stderr=subprocess.DEVNULL)
    return tag, d, open(f, 'rb').read(), open(d + '/s.pk', 'rb').read(), open(d + '/s.out', 'rb').read()
def run(args):
    tag, d, orig, pk, out, i = args
    rnd = random.Random(seed * 1000003 + i + hash(tag) % 10007)
    w = os.path.join(work, 'r_%s_%d' % (tag, i)); os.makedirs(w, exist_ok=True)
    m_out = rnd.random() < .2
    a = pk if m_out else mutate(pk, rnd); x = mutate(out, rnd) if m_out else out
    open(w + '/m.pk', 'wb').write(a); open(w + '/m.out', 'wb').write(x)
    try:
        p = subprocess.run([binp, 'd', w + '/m.pk', w + '/m.out', w + '/m.mp3'], env=env, capture_output=True, timeout=60)
        rc, err = p.returncode, p.stderr.decode('latin-1')
    except subprocess.TimeoutExpired:
        rc, err = 'TIMEOUT', ''
    bad = None
    if rc == 0:
        if open(w + '/m.mp3', 'rb').read() != orig: bad = 'exit 0, wrong output'
    elif rc != 1: bad = 'exit %s' % rc
    if 'runtime error' in err or 'Sanitizer' in err: bad = (bad or '') + ' sanitizer'
    if bad:
        open(w + '/why.txt', 'w').write(bad + '\n' + err[:4000]); return (tag, i, bad, err[:300])
    shutil.rmtree(w); return None
seeds = [prep(f, m) for f in files for m in (0, 1)]
jobs = [(t, d, o, p, x, i) for (t, d, o, p, x) in seeds for i in range(n)]
with ThreadPoolExecutor(4) as ex: res = [r for r in ex.map(run, jobs) if r]
print('runs %d, bad %d' % (len(jobs), len(res)))
for r in res[:40]: print('  ', r[0], r[1], r[2], '|', r[3].replace('\n', ' ')[:200])
