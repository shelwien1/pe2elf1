#!/usr/bin/env python3
# gen_bank.py -- turn alt-P2's bank-context CtxIdx chains into Index
# declarations.  One-time; run from the repo root.
#
# Each chain is a run of flags at descending bit positions over a quantiser
# pair, and step_bank immediately shifts the result back down to get the bank
# value.  As a mixed radix that is a run of radix-2 factors over two radix-4
# ones, and the shift disappears: step_bank takes the bank value directly.
#
# The bit positions split in two on the way across.  For a `bit<P>` the position
# only said where the flag landed, and the declaration order says that now, so
# it retires.  For a `bit_of<P>` the position also chose *which* bit of the
# expression carried the flag -- a sign, mostly -- and that survives, renamed
# from _pN to _eN because it no longer means a position.
import re

SRC = 'alt_p2.inc'
CALL = re.compile(r'\.(bit|bits|bit_of|above|digit)<([^>]*)>\(')
QUANT = re.compile(r'ctx_quant\(\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),'
                   r'\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)')


def split_args(text, at):
    depth, i = 0, at
    while i < len(text):
        if text[i] == '(':
            depth += 1
        elif text[i] == ')':
            depth -= 1
            if depth == 0:
                return text[at + 1:i], i + 1
        i += 1
    raise ValueError('unbalanced')


def parse(line):
    out, i = [], line.index('CtxIdx{}')
    while True:
        m = CALL.search(line, i)
        if not m:
            break
        arg, end = split_args(line, m.end() - 1)
        out.append((m.group(1), [t.strip() for t in m.group(2).split(',')], arg))
        i = end
    raw, j = None, line.find('.raw(', i - 1 if i else 0)
    if j >= 0:
        raw, _ = split_args(line, j + 4)
    return out, raw


PLAN = [
    ('ctx0_lo', 0, 'Ctx0Lo', None),
    ('ctx0', 0, 'Ctx0a', ('lo', 'Ctx0Lo')),
    ('ctx0', 1, 'Ctx0b', ('lo', 'Ctx0Lo')),
    ('ctx1', 0, 'Ctx1a', 'quant'),
    ('ctx1', 1, 'Ctx1b', 'quant'),
    ('ctx2', 0, 'Ctx2a', 'quant'),
    ('ctx2', 1, 'Ctx2b', 'quant'),
    ('ctx3', 0, 'Ctx3', 'quant'),
    ('ctx4', 0, 'Ctx4', 'quant'),
]

lines = open(SRC).read().split('\n')
found = {}
for n, line in enumerate(lines):
    if 'CtxIdx{}' in line:
        m = re.match(r'\s*(?:const\s+\w+\s+)?(ctx0_lo|ctx0|ctx1|ctx2|ctx3|ctx4)\s*=', line)
        if m:
            found.setdefault(m.group(1), []).append(n)

blocks, builders, rename, drop = [], [], {}, set()
for var, occ, name, rawkind in PLAN:
    n = found[var][occ]
    calls, raw = parse(lines[n])
    factors, args, callargs, fi = [], [], [], 0
    for kind, targs, arg in calls:
        tag = 'f%d' % fi
        if kind == 'bit':
            drop.add(targs[0][3:])
            factors.append(('2', tag))
            callargs.append('(uint32_t)(%s)' % arg)
        elif kind == 'bit_of':
            new = re.sub(r'_p(\d+)$', r'_e\1', targs[0][3:])
            rename[targs[0][3:]] = new
            factors.append(('2', tag))
            callargs.append('(uint32_t)(((uint32_t)(%s)>>IX_%s)&1u)' % (arg, new))
        elif kind == 'bits':
            drop.add(targs[0][3:])
            factors.append(('(1<<IX_%s)' % targs[1][3:], tag))
            callargs.append('(uint32_t)(%s)' % arg)
        else:
            raise SystemExit('unexpected %s in %s' % (kind, name))
        args.append(tag)
        fi += 1
    if rawkind == 'quant':
        m = QUANT.search(raw)
        assert m, raw
        v, r0, r1, r2, s, s0, s1, s2 = [g.strip() for g in m.groups()]
        for t, base, a, b, c in (('qr', v, r0, r1, r2), ('qs', s, s0, s1, s2)):
            factors.append(('4', t))
            args.append(t)
            callargs.append('(uint32_t)((%s>%s)+(%s>%s)+(%s>%s))' % (base, a, base, b, base, c))
    elif rawkind is not None:
        t, sub = rawkind
        factors.append(('P2_%s_Volume' % sub, t))
        args.append(t)
        callargs.append('(uint32_t)(%s)' % raw)

    blocks.append('Index %s\n%s' % (name, '\n'.join(' ADD %s: %s' % (m, a) for m, a in factors)))
    builders.append('uint32_t Make%s( %s ) {\n  int32_t %s;\n  MakeIndex %s\n  return (uint32_t)%s;\n}'
                    % (name, ', '.join('uint32_t ' + a for a in args), name, name, name))
    lines[n] = re.sub(r'CtxIdx\{\}.*;\s*$', 'Make%s(%s);' % (name, ', '.join(callargs)), lines[n])

open(SRC, 'w').write('\n'.join(lines))

# rename the surviving extraction bits, drop the positions the order replaced
p = 'IDX/bmf-IX.idx'
out = []
for ln in open(p):
    m = re.match(r'Number ([a-z0-9_]+),', ln)
    if m and m.group(1) in drop:
        continue
    if m and m.group(1) in rename:
        ln = ln.replace('Number %s,' % m.group(1), 'Number %s,' % rename[m.group(1)], 1)
    out.append(ln)
open(p, 'w').write(''.join(out))

with open('IDX/bmf-P2.idx', 'a') as f:
    f.write('\n# The five bank contexts.  Eleven flags over a quantiser pair apiece,\n'
            '# Volume 32768 -- which is the ctx radix of BankSlot above, so how wide the\n'
            '# per-bank counter table is now follows from how many flags the context\n'
            '# declares rather than from a number written beside it.\n\n')
    f.write('\n\n'.join(blocks) + '\n')
with open('IDX/bmf-P2.inc', 'a') as f:
    f.write('\n' + '\n\n'.join(builders) + '\n')

print('%d chains ported, %d positions retired, %d extraction bits renamed'
      % (len(PLAN), len(drop), len(rename)))
