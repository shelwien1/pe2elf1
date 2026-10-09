#!/usr/bin/env python3
"""Compare xz compression of executables with and without x64pp.

usage: bench.py [-p PRESET] [--x64pp PATH] [--x64flt3 PATH] [--opt ARGS]... FILE_OR_DIR...

Columns (bytes after xz, container overhead excluded with --check=none):
  xz       xz --lzma2=preset=P
  bcj      xz --x86 --lzma2=preset=P
  flt3     x64flt3 c, its 4 output files concatenated, then xz (if given)
  x64pp    x64pp c, then xz; one column per --opt (default: no extra
           arguments), e.g. --opt= --opt=-a for the default and the search
Every x64pp output is decoded again and compared with the input.
"""
import argparse, os, shutil, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

ap = argparse.ArgumentParser()
ap.add_argument('-p', '--preset', default='9e')
ap.add_argument('-j', '--jobs', type=int, default=os.cpu_count() or 1)
ap.add_argument('--x64pp', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'x64pp'))
ap.add_argument('--x64flt3', default=None)
ap.add_argument('--opt', action='append', help='extra x64pp arguments; repeat for more columns')
ap.add_argument('inputs', nargs='+')
args = ap.parse_args()
variants = args.opt if args.opt else ['']

files = []
for p in args.inputs:
    if os.path.isdir(p):
        files += sorted(os.path.join(p, f) for f in os.listdir(p) if os.path.isfile(os.path.join(p, f)))
    else:
        files.append(p)


def xz(path, extra=()):
    r = subprocess.run(['xz', '-c', '-T1', '--check=none'] + list(extra) + ['--lzma2=preset=' + args.preset, path],
                       stdout=subprocess.PIPE, check=True)
    return len(r.stdout)


def measure(path):
    res = {'size': os.path.getsize(path), 'xz': xz(path), 'bcj': xz(path, ['--x86'])}
    d = tempfile.mkdtemp()
    try:
        enc, dec = os.path.join(d, 'enc'), os.path.join(d, 'dec')
        for v in variants:
            subprocess.run([args.x64pp, 'c', '-n'] + v.split() + [path, enc], check=True)
            subprocess.run([args.x64pp, 'd', enc, dec], check=True)
            with open(path, 'rb') as a, open(dec, 'rb') as b:
                if a.read() != b.read():
                    sys.exit('round trip failed: ' + path)
            res[('x64pp ' + v).strip()] = xz(enc)
        if args.x64flt3:
            st = os.path.join(d, 's')
            subprocess.run([args.x64flt3, 'c', path, st], check=True)
            cat = os.path.join(d, 'cat')
            with open(cat, 'wb') as o:
                for q in (st, st + '_1', st + '_2', st + '_3'):
                    with open(q, 'rb') as i:
                        o.write(i.read())
            res['flt3'] = xz(cat)
    finally:
        shutil.rmtree(d)
    return res


with ThreadPoolExecutor(args.jobs) as ex:
    rows = list(ex.map(measure, files))

cols = ['size', 'xz', 'bcj'] + (['flt3'] if args.x64flt3 else []) + [('x64pp ' + v).strip() for v in variants]
w = max([len(os.path.basename(f)) for f in files] + [5])
print('%-*s' % (w, 'file') + ''.join('%12s' % c for c in cols))
tot = dict.fromkeys(cols, 0)
for f, r in zip(files, rows):
    print('%-*s' % (w, os.path.basename(f)) + ''.join('%12d' % r[c] for c in cols))
    for c in cols:
        tot[c] += r[c]
print('%-*s' % (w, 'total') + ''.join('%12d' % tot[c] for c in cols))
print('%-*s' % (w, 'vs xz') + ''.join('%12s' % ('' if c == 'size' else '%+.2f%%' % (100.0 * (tot[c] / tot['xz'] - 1))) for c in cols))
