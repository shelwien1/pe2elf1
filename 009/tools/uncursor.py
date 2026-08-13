#!/usr/bin/env python3
"""One cursor for a run of record moves, instead of one reload each.

    python3 tools/uncursor.py subs1.hpp
    python3 tools/uncursor.py subs1.hpp --all

MSVC reloaded the table base between every pair of the window shift, and
Hex-Rays gave each reload its own name:

    v38 = (uintptr_t)(lpAddress->f278736[0]);
    ((P2Ctx *)(v37 + v38))[-2] = ((P2Ctx *)(v37 + v38))[1];
    v39 = (uintptr_t)(lpAddress->f278736[0]);
    ((P2Ctx *)(v37 + v39))[-3] = ((P2Ctx *)(v37 + v39))[2];

A run is taken only when every reload is the *same* expression and every
move is over that reload plus the same offset -- which is what says the
eight names are one cursor.  Nothing between them writes the member being
reloaded; the record assignments write the table it points at.
"""
import re
import sys

p = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
L = open(p).read().split('\n')
LOAD = re.compile(r'^(\s*)(\w+) = (?:\(uintptr_t\)|\(uint8_t \*\))?\(?(.+?)\)?;$')
COPY = re.compile(r'^\s*\(\(P2Ctx \*\)\((\w+) \+ (\w+)\)\)\[(-?\d+)\] = '
                  r'\(\(P2Ctx \*\)\((\w+) \+ (\w+)\)\)\[(-?\d+)\];$')


def pair(i):
    """(indent, base expr, offset var, dst, src) if lines i, i+1 are a reload."""
    m = LOAD.match(L[i]) if i < len(L) else None
    c = COPY.match(L[i + 1]) if m and i + 1 < len(L) else None
    if not c:
        return None
    v = m.group(2)
    a, b = {c.group(1), c.group(2)}, {c.group(4), c.group(5)}
    if a != b or v not in a or len(a) != 2:
        return None
    off = (a - {v}).pop()
    return m.group(1), m.group(3), off, int(c.group(3)), int(c.group(6))


apply = '--all' in sys.argv
out, i, runs, folded = [], 0, 0, 0
while i < len(L):
    g = pair(i)
    if not g:
        out.append(L[i])
        i += 1
        continue
    ind, base, off, d, s = g
    pairs, j = [(d, s)], i + 2
    while True:
        h = pair(j)
        if not h or h[1] != base or h[2] != off:
            break
        pairs.append((h[3], h[4]))
        j += 2
    if len(pairs) < 3:
        out.append(L[i])
        i += 1
        continue
    runs += 1
    folded += len(pairs)
    name = 'rec%d' % runs
    out.append('%s// One cursor for the %d records this shifts; MSVC reloaded the base'
               % (ind, len(pairs)))
    out.append('%s// between every pair and nothing here writes it.' % ind)
    out.append('%sP2Ctx *const %s = (P2Ctx *)((uint8_t *)(%s) + %s);' % (ind, name, base, off))
    for a, b in pairs:
        out.append('%s%s[%d] = %s[%d];' % (ind, name, a, name, b))
    i = j
print('%d runs, %d record moves, %d reloads gone' % (runs, folded, folded))
if apply:
    open(p, 'w').write('\n'.join(out))
