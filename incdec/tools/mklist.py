#!/usr/bin/env python3
"""list.txt (incdec.md §3 step 9): every function of vbn's own code, whether
plain `u`/`r` runs it, and whether it has been moved.

Scope (incdec.md §1, adapted): vbn's own functions -- the code below the C
runtime, which starts at 0x1000704C -- that plain `u` and `r` execute.  They
are found by running the un-injected port (../vbn) under gdb with a one-shot
breakpoint on every function entry, doing what the gate does: compress and
restore each entry of the corpus, restore its `b` and `B` archives, and run
the user-error commands of errors.txt.  Two functions in the program's range
are runtime support that only the C runtime calls (the DOS extender's sbrk,
sub_100001C7, and stack-avail, sub_10000212); like the rest of the library
they stay in the image.

Columns: VA, name, scope (u/r | crt | -), state (moved | -), probe count
from the last gate run (probes.txt; all of the gate's runs summed).
"""
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
from common import ROOT, CODE_LO, CRT_LO, load_funcs

CORPUS = re.search(r'^CORPUS="([^"]*)"', open(os.path.join(ROOT, 'test.sh')).read(), re.M).group(1).split()
STAMP = 315533580                       # 1980-01-01 00:13 UTC, as test.sh pins it
CRT_ONLY = {0x100001C7: 'sbrk (DOS 350Ch); called from malloc only',
            0x10000212: 'stack-avail; called from the C runtime only'}
EXTRA = [0x10007138, 0x10008658]        # bannerless entries (mkconv.py)


def gdb_script(path, addrs):
    with open(path, 'w') as f:
        f.write('set pagination off\nset confirm off\n'
                'handle SIGILL nostop noprint pass\nhandle SIGSEGV nostop noprint pass\nstarti\n')
        for a in addrs:
            f.write(f'tbreak *0x{a:x}\n')
        f.write('run\nwhile 1\n  continue\nend\n')


def files(top):
    """{relative path: (bytes, mtime)} under top, the archive aside."""
    out = {}
    for d, _, fs in os.walk(top):
        for f in fs:
            p = os.path.join(d, f)
            if f != 'archive.vbn':
                out[os.path.relpath(p, top)] = (open(p, 'rb').read(), int(os.stat(p).st_mtime) // 60)
    return out


def trace(src, work, script):
    """Addresses hit by `u archive <file>` (`*.*` for a directory) and then
    `r archive`, and by `r` of the entry's `b` and `B` archives, each run as
    test.sh runs it."""
    name = os.path.basename(src)
    u, r = os.path.join(work, name, 'u'), os.path.join(work, name, 'r')
    os.makedirs(r)
    if os.path.isdir(os.path.join(ROOT, src)):
        shutil.copytree(os.path.join(ROOT, src), u)
        arg = '*.*'
    else:
        os.makedirs(u)
        shutil.copy(os.path.join(ROOT, src), u)
        arg = name
    for d, _, fs in os.walk(u):
        for f in fs:
            os.utime(os.path.join(d, f), (STAMP, STAMP))
    exe = os.path.abspath(os.path.join(ROOT, '..', 'vbn'))
    env = dict(os.environ, TZ='UTC')
    inputs = files(u)
    # `b` and `B` archives of the same input, made without tracing, for `r`
    runs = [(u, ['u', 'archive', arg]), (r, ['r', 'archive'])]
    for m in ('b', 'B'):
        mk, rm = os.path.join(work, name, m), os.path.join(work, name, 'r' + m)
        shutil.copytree(u, mk)
        subprocess.run([exe, m, 'archive', arg], cwd=mk, env=env, capture_output=True, timeout=1800)
        os.makedirs(rm)
        shutil.copy(os.path.join(mk, 'archive.vbn'), rm)
        runs.append((rm, ['r', 'archive']))
    hits = set()
    for d, args in runs:
        if d == r:
            shutil.copy(os.path.join(u, 'archive.vbn'), r)
        p = subprocess.run(['gdb', '-q', '-batch', '-x', script, '--args', exe] + args,
                           cwd=d, env=env, capture_output=True, text=True, timeout=1800)
        hits |= {int(m, 16) for m in re.findall(r'Temporary breakpoint \d+, 0x([0-9a-f]+)', p.stdout)}
        if args[0] == 'r' and files(d) != inputs:
            sys.exit(f'{src}: the traced run in {os.path.basename(d)} does not round-trip')
    return hits


def trace_error(n, cmd, work, script):
    """Addresses hit by one command of errors.txt, run in an empty directory."""
    d = os.path.join(work, f'err{n}')
    os.makedirs(d)
    p = subprocess.run(['gdb', '-q', '-batch', '-x', script, '--args',
                        os.path.abspath(os.path.join(ROOT, '..', 'vbn'))] + cmd.split(),
                       cwd=d, env=dict(os.environ, TZ='UTC'), capture_output=True, text=True, timeout=1800)
    return {int(m, 16) for m in re.findall(r'Temporary breakpoint \d+, 0x([0-9a-f]+)', p.stdout)}


def main():
    funcs = load_funcs()
    addrs = sorted({f['addr'] for f in funcs if f['addr'] >= CODE_LO} | set(EXTRA))
    if not all(os.path.exists(os.path.join(ROOT, c)) for c in CORPUS):   # test.sh's check
        subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'mkcorpus.py'), 'corpus'],
                       cwd=ROOT, check=True)
    work = tempfile.mkdtemp(dir=ROOT, prefix='work.')
    try:
        script = os.path.join(work, 'trace.gdb')
        gdb_script(script, addrs)
        errors = [l.strip() for l in open(os.path.join(ROOT, 'errors.txt'))
                  if l.strip() and not l.startswith('#')]
        with concurrent.futures.ThreadPoolExecutor(len(CORPUS) + len(errors)) as ex:
            jobs = [ex.submit(trace, s, work, script) for s in CORPUS]
            jobs += [ex.submit(trace_error, n, c, work, script) for n, c in enumerate(errors, 1)]
            hit = set().union(*(j.result() for j in jobs))
    finally:
        shutil.rmtree(work)

    accepted = {l.strip() for l in open(os.path.join(ROOT, 'accepted.txt'))
                if l.strip() and not l.startswith('#')}
    probes = {}
    if os.path.exists(os.path.join(ROOT, 'probes.txt')):
        for l in open(os.path.join(ROOT, 'probes.txt')):
            k, v = l.split()
            probes[k[2:]] = int(v)

    rows, n_scope, n_moved = [], 0, 0
    for f in funcs:
        a = f['addr']
        if not CODE_LO <= a < CRT_LO:
            continue
        if a in CRT_ONLY:
            scope = 'crt'
        elif a in hit:
            scope = 'u/r'
            n_scope += 1
        else:
            scope = '-'
        state = 'moved' if f['name'] in accepted else '-'
        n_moved += state == 'moved' and scope == 'u/r'
        note = CRT_ONLY.get(a, '')
        rows.append(f"{a:08X}\t{f['name']}\t{scope}\t{state}\t{probes.get(f['name'], '-')}"
                    + (f'\t{note}' if note else ''))
    stray = sorted(n for n in accepted if not any(f['name'] == n and f['addr'] in hit for f in funcs))
    with open(os.path.join(ROOT, 'list.txt'), 'w') as fo:
        fo.write('# VA\tname\tscope\tstate\tprobes   -- generated by tools/mklist.py '
                 '(gdb first-hit trace of ../vbn u/r over the gate corpus)\n')
        fo.write('\n'.join(rows) + '\n')
    print(f'{n_scope} functions in scope, {n_moved} moved'
          + (f'; moved but not in scope: {" ".join(stray)}' if stray else ''))


if __name__ == '__main__':
    main()
