#!/usr/bin/env python3
"""Rewrite accepted.txt callees-first, and regenerate every accepted .inc.

incdec.md §6.3: a caller extracted while its callee was still in the PE binds
the callee by address; once the callee moves, that binding collides with the
real definition and the caller has to be regenerated against it -- with the
callee's .inc included first.  Doing both for the whole set on every step is
cheap and leaves nothing to remember."""
import os, re, subprocess, sys
from common import ROOT, donor_lines, load_funcs

funcs = {f['name']: f for f in load_funcs()}
src = donor_lines()
acc_path = os.path.join(ROOT, 'accepted.txt')
lines = open(acc_path).read().split('\n')
header = [l for l in lines if l.startswith('#')]
acc = [l.split()[0] for l in lines if l.strip() and not l.startswith('#')]
deps = {}
for n in acc:
    ovr = os.path.join(ROOT, 'override', n + '.inc')
    text = open(ovr).read() if os.path.exists(ovr) else '\n'.join(src[funcs[n]['start'] - 1:funcs[n]['end']])
    text = re.sub(r'//[^\n]*', '', text)          # names in comments are not calls
    deps[n] = {d for d in re.findall(r'\bsub_[0-9A-F]{8}\b', text) if d in acc and d != n}
out, done = [], set()
def visit(n, stack=()):
    if n in done:
        return
    if n in stack:
        sys.exit('call cycle among moved functions: ' + ' -> '.join(stack + (n,)))
    for d in sorted(deps[n]):
        visit(d, stack + (n,))
    done.add(n)
    out.append(n)
for n in acc:
    visit(n)
open(acc_path, 'w').write('\n'.join(header + out) + '\n')
gen = [n for n in out if not os.path.exists(os.path.join(ROOT, 'override', n + '.inc'))]
if gen:
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'extract.py')] + gen,
                   check=True, stdout=subprocess.DEVNULL)
