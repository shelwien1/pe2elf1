#!/usr/bin/env python3
# sweep.py -- build bmpc with a set of -D overrides and report the total
# compressed size over a corpus.  Used to pick the model shape and the B0
# constants; every number in README-bmpc.md came out of this.
#
# The first config listed is the reference: later ones report both the total
# byte delta and 'geo', the geometric mean of the per-file size ratios.  The
# total is dominated by whichever corpus file is biggest, so a config that
# wins the total can still be a large regression on the small files -- geo is
# the number to steer by, the total the one to report.
#
#   ./sweep.py                          run the built-in experiment list
#   ./sweep.py 'MIX_LOGISTIC=1 W_LOGIT=1' ...   run specific configs
#   CORPUS=big ./sweep.py ...           add the full 8 MB BMP
#
# A config is a space-separated list of NAME=VALUE; each becomes -DNAME=VALUE.

import os, subprocess, sys, hashlib, shutil, time, threading
from concurrent.futures import ThreadPoolExecutor

BUILD_LOCK = threading.Lock()
NJOBS = int(os.environ.get('J', '3'))

ROOT = os.path.dirname(os.path.abspath(__file__))
os.chdir(ROOT)
OUT = os.path.join(ROOT, 'out')
os.makedirs(OUT, exist_ok=True)

DEV = ['testfiles/t8g.bmp', 'testfiles/t8p.bmp', 'testfiles/t24.bmp',
       'testfiles/t32.bmp', 'testfiles/x_ep.bmp', 'work/bmp/big_crop.bmp']
BIG = DEV + ['work/bmp/20000171A.bmp']

def corpus():
    c = BIG if os.environ.get('CORPUS') == 'big' else DEV
    return [f for f in c if os.path.exists(f)]

def build(cfg, verify=False):
    """cfg: dict of -D name->value.  Returns path to the binary."""
    key = ' '.join('%s=%s' % kv for kv in sorted(cfg.items()))
    tag = hashlib.md5(key.encode()).hexdigest()[:10]
    exe = os.path.join(OUT, 'bmpc_' + tag)
    stamp = exe + '.key'
    src = max(os.path.getmtime(f) for f in
              ['bmpc.cpp', 'sh_counter0.inc', 'sh_common.inc', 'config_b.hpp',
               'MOD/sh_model-B0_h.inc', 'MOD/sh_model-C0_h.inc', 'config.hpp'])
    if os.path.exists(exe) and os.path.exists(stamp) and os.path.getmtime(exe) > src:
        return exe
    opts = ' '.join('-D%s=%s' % kv for kv in sorted(cfg.items()))
    env = dict(os.environ, GCOPTS=opts, GCOUT=exe)
    with BUILD_LOCK:
        r = subprocess.run(['./gc.sh', 'bmpc'], env=env, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout + r.stderr)
        raise SystemExit('build failed for ' + key)
    open(stamp, 'w').write(key)
    return exe

def run(exe, files, verify=True):
    tot_i = tot_o = 0
    per = []
    for f in files:
        c = exe + '.c'
        subprocess.run([exe, 'c', f, c], check=True)
        n = os.path.getsize(c)
        if verify:
            d = exe + '.d'
            subprocess.run([exe, 'd', c, d], check=True)
            if open(f, 'rb').read() != open(d, 'rb').read():
                raise SystemExit('ROUNDTRIP FAILED on ' + f + ' with ' + exe)
        per.append((os.path.basename(f), os.path.getsize(f), n))
        tot_i += os.path.getsize(f); tot_o += n
    return tot_i, tot_o, per

def parse(s):
    d = {}
    for tok in s.split():
        k, _, v = tok.partition('=')
        d[k] = v or '1'
    return d

def main():
    args = sys.argv[1:]
    verify = os.environ.get('VERIFY', '1') == '1'
    detail = os.environ.get('DETAIL', '0') == '1'
    files = corpus()

    def one(a):
        t0 = time.time()
        exe = build(parse(a))
        ti, to, per = run(exe, files, verify)
        for suf in ('.c', '.d'):
            try: os.remove(exe + suf)
            except OSError: pass
        return a, ti, to, per, time.time() - t0

    with ThreadPoolExecutor(max_workers=NJOBS) as ex:
        results = list(ex.map(one, args))

    import math
    base = None; ref = None
    for a, ti, to, per, dt in results:
        if base is None: base = to; ref = [o for _, _, o in per]
        d = '' if to == base else ' %+d(%+.2f%%)' % (to - base, 100.0 * (to - base) / base)
        geo = math.exp(sum(math.log(o / float(r)) for (_, _, o), r in zip(per, ref)) / len(per))
        print('%9d %6.4f bpc  geo %+7.3f%%  %5.1fs  %-56s%s'
              % (to, to * 8.0 / ti, 100.0 * (geo - 1.0), dt, a or '(defaults)', d))
        if detail:
            for (n, i, o), r in zip(per, ref):
                print('           %-20s %9d -> %9d  %6.4f bpc  %+.2f%%'
                      % (n, i, o, o * 8.0 / i, 100.0 * (o - r) / r))
    sys.stdout.flush()

if __name__ == '__main__':
    main()
