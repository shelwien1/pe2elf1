#!/usr/bin/env python3
"""Put an unrolled block copy back together.

    python3 tools/uncopy.py subs1.hpp [--list]

The struct sweep ran out of objects worth recovering and then kept offering
things that were not objects. Every candidate left had the same shape -- five
fields, four bytes apart, five bytes wide in total 18 -- and twenty-four of them
in one function, at offsets stepping 18 at a time and alternating sign. That is
not twenty-four structs. It is one 18-byte record, copied:

    *(uint32_t *)(v73 - 18) = *(uint32_t *)v76;
    *(uint32_t *)(v73 - 14) = *(uint32_t *)(v76 + 4);
    *(uint32_t *)(v73 - 10) = *(uint32_t *)(v76 + 8);
    *(uint32_t *)(v73 -  6) = *(uint32_t *)(v76 + 12);
    *(uint16_t *)(v73 -  2) = *(uint16_t *)(v76 + 16);

which is `memcpy(v73 - 18, v76, 18)` as MSVC emitted it, four dwords and a word,
and as Hex-Rays could only give it back. Phase 5 did the same for fills: runs of
`uint64_t` stores of one value became `memset`, "which says what they are".
This is that for copies.

The rule is deliberately rigid -- one base and one stride on each side, offsets
consecutive with no gap, the widths summing to the span -- because the point is
to recognise a copy, not to guess at one. Anything that does not match exactly
is left alone.

Overlap is the one thing a `memcpy` could get wrong that the store sequence
would not, so it is checked rather than argued: `BMF_COPY_CHECK=1 ./build.sh`
compiles the rewritten calls through a wrapper that aborts if the regions
overlap, and the gate is run against it.
"""
import re
import sys

STORE = re.compile(
    r'^(?P<ind>\s*)\*\((?P<dt>uint(?:8|16|32|64)_t) \*\)'
    r'\((?P<d>[A-Za-z_][A-Za-z0-9_]*)(?: (?P<dop>[-+]) (?P<dk>\d+))?\)'
    r'\s*=\s*'
    r'\*\((?P<st>uint(?:8|16|32|64)_t) \*\)'
    r'\((?P<s>[A-Za-z_][A-Za-z0-9_]*)(?: (?P<sop>[-+]) (?P<sk>\d+))?\);\s*$')
BARE = re.compile(
    r'^(?P<ind>\s*)\*\((?P<dt>uint(?:8|16|32|64)_t) \*\)'
    r'\((?P<d>[A-Za-z_][A-Za-z0-9_]*)(?: (?P<dop>[-+]) (?P<dk>\d+))?\)'
    r'\s*=\s*'
    r'\*\((?P<st>uint(?:8|16|32|64)_t) \*\)(?P<s>[A-Za-z_][A-Za-z0-9_]*);\s*$')
WIDTH = {'uint8_t': 1, 'uint16_t': 2, 'uint32_t': 4, 'uint64_t': 8}


def store(line):
    m = STORE.match(line) or BARE.match(line)
    if not m:
        return None
    g = m.groupdict()
    d = int(g['dk'] or 0) * (-1 if g.get('dop') == '-' else 1)
    s = int(g.get('sk') or 0) * (-1 if g.get('sop') == '-' else 1)
    return (g['ind'], g['d'], d, WIDTH[g['dt']], g['s'], s, WIDTH[g['st']])


def run_at(lines, i):
    """The longest sequence from line i that is one contiguous copy."""
    first = store(lines[i])
    if not first:
        return None
    ind, dnm, d0, dw, snm, s0, sw = first
    if dw != sw:
        return None
    j, at_d, at_s = i, d0, s0
    while j < len(lines):
        cur = store(lines[j])
        if not cur:
            break
        _, dn, d, w, sn, s, w2 = cur
        if dn != dnm or sn != snm or w != w2 or d != at_d or s != at_s:
            break
        at_d, at_s = d + w, s + w
        j += 1
    n = at_d - d0
    return (i, j, ind, dnm, d0, snm, s0, n) if j - i >= 3 else None


def expr(nm, off):
    if off == 0:
        return nm
    return '%s %s %d' % (nm, '-' if off < 0 else '+', abs(off))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    runs, i = [], 0
    while i < len(lines):
        r = run_at(lines, i)
        if r:
            runs.append(r)
            i = r[1]
        else:
            i += 1
    if not runs:
        print('no unrolled copies')
        return 0
    if '--list' in sys.argv:
        for i, j, ind, d, dk, s, sk, n in runs:
            print('%s:%d: %d stores, %d bytes, %s <- %s'
                  % (path, i + 1, j - i, n, expr(d, dk), expr(s, sk)))
        print('%d runs, %d lines -> %d calls'
              % (len(runs), sum(j - i for i, j, *_ in runs), len(runs)))
        return 0

    for i, j, ind, d, dk, s, sk, n in reversed(runs):
        lines[i:j] = ['%sbmf_copy((void *)(%s), (const void *)(%s), %d);'
                      % (ind, expr(d, dk), expr(s, sk), n)]
    open(path, 'w').write('\n'.join(lines))
    print('%d runs folded, %d lines -> %d calls'
          % (len(runs), sum(j - i for i, j, *_ in runs), len(runs)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
