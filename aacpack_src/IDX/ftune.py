#!/usr/bin/env python3
"""ftune.py -- opt.pl's hill-climb, a family at a time against its dumps.

usage: ftune.py FAMTUNE DUMPDIR [passes] [family...]

FAMTUNE is a level's replayer (mk.sh famtune L), DUMPDIR what mk.sh dump
wrote (a directory per stream, a <family>.dump in each).  Each family is
tuned in a copy of FAMTUNE of its own, its patterns flipped a bit at a
time, a flip kept when the family codes its dumps smaller (or as small with
the bit now 0, but such ties only with a pattern that gains, as opt.pl
commits only a gain), pattern after pattern in random order, pass after
pass until a pass gains less than 0.01% of the family's size, or a byte
($FTUNE_MINPASS; or `passes` of them).  A family whose dumps code to less
than a byte (no decisions: one that no stream reached) is left as it is, not
zeroed; a trial that fails counts as the worst (logged).  The families do not
interact, so four are tuned at once, the largest first, each trial a
process per stream.  Writes the
winners to export.!!! (mdesc lines, for IDX/import.pl; every pattern of
the level) at each gain, and the progress to ftune.log, both in the
current directory; with FTUNE_FROM=EXPORT it starts from that export's
patterns.  Interrupted (^C), it stops at the next trial and exports the best
so far.  Exit status 1 if it tuned no family.
"""
import glob, os, random, re, shutil, subprocess, sys, threading, time
from concurrent.futures import ThreadPoolExecutor

exe, ddir = sys.argv[1], sys.argv[2]
passes = int(sys.argv[3]) if len(sys.argv) > 3 else 9
only = sys.argv[4:]
minrel = float(os.environ.get('FTUNE_MINPASS', '1e-4'))  # a pass that gains less (of the size; a byte at least) ends a climb
data = bytearray(open(exe, 'rb').read())
pats = {}  # name: (address, length, offset field)
for m in re.finditer(rb'!MAP!(.*?)!(.*?)\x00(.*?)\x00', data, re.S):
    pats[m.group(1).decode()] = (m.start(3), len(m.group(3)), m.group(2).decode())
# $FTUNE_FROM: an export to start from (that of a run cut short, say): its patterns go into the binary first
if os.environ.get('FTUNE_FROM'):
    n = 0
    for m in re.finditer(r'mdesc\(\s*(\S+?)\s*,\s*(\S+?)\s*,\s*"([01]*)"\s*\)', open(os.environ['FTUNE_FROM']).read()):
        k, b = m.group(1), m.group(3).encode()
        if k in pats and len(b) == pats[k][1]:
            data[pats[k][0]:pats[k][0] + len(b)] = b
            n += 1
    with open(exe, 'r+b') as f:
        f.write(bytes(data))
    print('ftune.py: %d patterns from %s' % (n, os.environ['FTUNE_FROM']), file=sys.stderr)
fams = {}
for k in pats:
    m = re.match(r'H\d+_([a-z0-9]+)_', k)
    if m:
        fams.setdefault(m.group(1), []).append(k)
if not fams:
    sys.exit('ftune.py: no patterns in %s (mk.sh famtune L makes the binary)' % exe)
if not glob.glob(os.path.join(ddir, '*', '*.dump')):
    sys.exit('ftune.py: no dumps in %s (mk.sh dump L DIR STREAM... makes them)' % ddir)
unknown = [f for f in only if f not in fams]
if unknown:
    sys.exit('ftune.py: no family %s (the families: %s)' % (' '.join(unknown), ' '.join(sorted(fams))))
lock = threading.Lock()
stop = threading.Event()  # (^C: the families end at their next trial)


class Stop(Exception):
    pass
log = open('ftune.log', 'a')
t0 = time.time()

def say(s):
    with lock:
        print('%8.0fs %s' % (time.time() - t0, s), file=log, flush=True)

def export():
    with lock, open('export.!!!', 'w') as f:
        for k in sorted(pats):
            a, l, o = pats[k]
            f.write('mdesc( %s, %s, "%s" );\n' % (k, o, data[a:a + l].decode()))

def tune(fam):
    dumps = sorted(glob.glob(os.path.join(ddir, '*', fam + '.dump')))
    if not dumps:
        return fam, None, None
    mine = 'ftune_' + fam
    shutil.copy(exe, mine)
    img = bytearray(data)

    def write(a, b):
        with open(mine, 'r+b') as f:
            f.seek(a)
            f.write(bytes(b))

    def measure():
        if stop.is_set():
            raise Stop
        # a process per stream, at once: the cores the small families leave go to the large ones
        ps = [subprocess.Popen(['./' + mine, fam, d], stdout=subprocess.PIPE, stderr=subprocess.PIPE) for d in dumps]
        tot = 0.0
        for p in ps:
            out, err = p.communicate()
            try:
                if p.returncode:
                    raise ValueError(err.decode().strip())
                tot += float(out)
            except ValueError as e:
                say('%s: a trial failed (%s): the worst' % (fam, e))
                tot = float('inf')
        return tot

    try:
        return climb(fam, measure, write, img)
    except Stop:
        return fam, None, None
    finally:
        os.remove(mine)

def climb(fam, measure, write, img):
    start = best = measure()
    if not start < float('inf') or start < 1.0:
        say('%s: %.1f bytes in its dumps: left as it is' % (fam, start))
        if not start < float('inf'):
            print('ftune.py: %s: its dumps do not code (ftune.log says why): left as it is' % fam, file=sys.stderr)
        return fam, None, None
    say('%s: %d patterns, %d bits, %.1f bytes' % (fam, len(fams[fam]), sum(pats[k][1] for k in fams[fam]), best))
    try:
        best = passes_of(fam, measure, write, img, start)
    except Stop:  # (the best so far is in data, and exported)
        say('%s: interrupted' % fam)
        raise
    return fam, start, best

def passes_of(fam, measure, write, img, start):
    best = start
    for p in range(passes):
        before = best
        names = sorted(fams[fam])
        random.shuffle(names)
        for k in names:
            a, l, o = pats[k]
            cur = bytearray(img[a:a + l])
            s = best
            changed = True
            while changed:
                changed = False
                for i in random.sample(range(l), l):
                    cur[i] ^= 1
                    write(a, cur)
                    cs = measure()
                    # keep the flip if smaller, or as small and the bit is now 0
                    if cs < s - 1e-6 or (abs(cs - s) <= 1e-6 and not (cur[i] & 1)):
                        if cs < s - 1e-6:
                            changed = True
                        s = cs
                    else:
                        cur[i] ^= 1
                        write(a, cur)
            if s < best - 1e-6:
                img[a:a + l] = cur
                best = s
                with lock:
                    data[a:a + l] = cur
                say('%s %-26s %s  %.1f' % (fam, k, cur.decode(), best))
                export()  # (a run cut short resumes from here: FTUNE_FROM)
            else:
                write(a, img[a:a + l])  # (no gain: its ties go too)
        export()
        say('%s: pass %d done, %.1f bytes (%+.1f)' % (fam, p, best, best - start))
        if best > before - max(1.0, minrel * start):
            break
    return best

order = sorted((f for f in fams if not only or f in only),
               key=lambda f: -sum(os.path.getsize(d) for d in glob.glob(os.path.join(ddir, '*', f + '.dump'))))
export()
tot0 = tot1 = 0.0
tuned = 0
ex = ThreadPoolExecutor(max_workers=int(os.environ.get('J', '4')))
futs = [ex.submit(tune, f) for f in order]
try:
    for fu in futs:
        fam, a, b = fu.result()
        if a is not None:
            tuned += 1
            tot0 += a
            tot1 += b
            print('%-4s %12.1f -> %12.1f  (%+.1f)' % (fam, a, b, b - a), flush=True)
except KeyboardInterrupt:
    stop.set()
    for fu in futs:
        fu.cancel()
    print('ftune.py: interrupted: the best patterns so far go to export.!!!', file=sys.stderr)
    ex.shutdown(wait=True)
    export()
    sys.exit(130)
ex.shutdown(wait=True)
export()
say('all: %.1f -> %.1f bytes (%+.1f)' % (tot0, tot1, tot1 - tot0))
print('all: %.1f -> %.1f bytes (%+.1f)' % (tot0, tot1, tot1 - tot0))
if not tuned:
    sys.exit('ftune.py: no family was tuned (ftune.log says why)')
