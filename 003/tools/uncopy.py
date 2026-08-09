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

# A store whose source is a struct member.  The sweep gave three of these rows a
# struct each -- Obj53, Obj54, Obj55 -- so half of some copies now reads `->f36`
# where the other half still reads `+ 36`, and a rule that wanted raw offsets on
# both sides walked past five of them.  The member's `fN` *is* its offset, so
# both spellings mean the same thing and both are accepted.
MEMBER = re.compile(
    r'^(?P<ind>\s*)\*\((?P<dt>uint(?:8|16|32|64)_t) \*\)'
    r'\((?P<d>[A-Za-z_][A-Za-z0-9_]*)(?: (?P<dop>[-+]) (?P<dk>\d+))?\)'
    r'\s*=\s*'
    r'(?P<s>[A-Za-z_][A-Za-z0-9_]*)->f(?P<sk>\d+);\s*$')

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
    m = STORE.match(line) or BARE.match(line) or MEMBER.match(line)
    if not m:
        return None
    g = m.groupdict()
    # `v80->f36` and `v80 + 36` are the same address and not the same
    # expression: v80 is an `Obj53 *`, so `v80 + 36` steps ninety bytes at a
    # time.  The first fold of these emitted exactly that -- and the flag saying
    # not to was computed *after* the line below had filled the field it tested,
    # so it was always false and the mistake shipped twice before the gate had
    # said it once.  A member source is emitted as `&v80->f36`, which cannot be
    # got wrong.
    member = 'st' not in g
    if member:                            # its width is the store's, and the
        g['st'] = g['dt']                 # member's `fN` is its offset
    d = int(g['dk'] or 0) * (-1 if g.get('dop') == '-' else 1)
    s = int(g.get('sk') or 0) * (-1 if g.get('sop') == '-' else 1)
    return (g['ind'], g['d'], d, WIDTH[g['dt']], g['s'], s, WIDTH[g['dt']],
            member)


def run_at(lines, i):
    """The longest sequence from line i that is one contiguous copy."""
    first = store(lines[i])
    if not first:
        return None
    ind, dnm, d0, dw, snm, s0, sw, mem = first
    if dw != sw:
        return None
    j, at_d, at_s = i, d0, s0
    while j < len(lines):
        cur = store(lines[j])
        if not cur:
            break
        _, dn, d, w, sn, s, w2, m2 = cur
        if (dn != dnm or sn != snm or w != w2 or d != at_d or s != at_s
                or m2 != mem):
            break
        at_d, at_s = d + w, s + w
        j += 1
    n = at_d - d0
    return (i, j, ind, dnm, d0, snm, s0, n, mem) if j - i >= 3 else None


def expr(nm, off, member=False):
    if member:
        return '&%s->f%d' % (nm, off)
    if off == 0:
        return nm
    return '%s %s %d' % (nm, '-' if off < 0 else '+', abs(off))


# ---------------------------------------------------------------------------
# The same copy, scheduled.
#
# The compiler was free to hoist the loads out of the stores, and in twelve
# places it did:
#
#     v18 = *(uint32_t *)((uintptr_t)v11 - 14);
#     v19 = *(uint32_t *)((uintptr_t)v11 - 10);
#     v11->f0 = *(uint32_t *)((uintptr_t)v11 - 18);
#     v20 = *(uint32_t *)((uintptr_t)v11 - 6);
#     v11->f4 = v18;
#     LOWORD(v18) = *(uint16_t *)((uintptr_t)v11 - 2);
#     v11->f8 = v19;
#     v11->f12 = v20;
#     v11->f16 = v18;
#
# Eighteen bytes from `v11 - 18` to `v11`, with the temporaries interleaved and
# `v18` used twice -- loaded at -14, stored to f4, then its low half reloaded
# from -2 and stored to f16.  Pairing by program order handles that and nothing
# else does: a temporary means whatever was last put in it.
#
# The block is only folded when every statement in it is one of these four
# kinds, so there is nothing between the first load and the last store that
# could observe the difference.
ADDR = r'\(\s*(?:\(\s*\w+\s*\**\s*\)\s*)?(\w+)(?:\s*([-+])\s*(\d+))?\s*\)'
LOAD = re.compile(r'^\s*(\w+) = \*\((uint(?:8|16|32|64)_t) \*\)%s;\s*$' % ADDR)
LOW = re.compile(r'^\s*LOWORD\((\w+)\) = \*\(uint16_t \*\)%s;\s*$' % ADDR)
STM = re.compile(r'^\s*(\w+)->f(\d+) = (?:\(\s*\w+\s*\)\s*)?(\w+);\s*$')
SLD = re.compile(r'^\s*(\w+)->f(\d+) = \*\((uint(?:8|16|32|64)_t) \*\)%s;\s*$' % ADDR)


def sched_at(lines, i):
    """One scheduled copy starting at line i, or None."""
    held, pairs, snm, dnm, j = {}, [], None, None, i
    while j < len(lines):
        l = lines[j]
        m = LOAD.match(l) or LOW.match(l)
        if m:
            g = m.groups()
            w = 2 if LOW.match(l) else WIDTH[LOAD.match(l).group(2)]
            base, off = g[-3], int(g[-1] or 0) * (-1 if g[-2] == '-' else 1)
            if snm and base != snm:
                break
            snm = base
            held[g[0]] = (off, w)
            j += 1
            continue
        m = SLD.match(l)
        if m:
            d, f, ty, base, sop, sk = m.groups()
            off = int(sk or 0) * (-1 if sop == '-' else 1)
            if (dnm and d != dnm) or (snm and base != snm):
                break
            dnm, snm = d, base
            pairs.append((int(f), off, WIDTH[ty]))
            j += 1
            continue
        m = STM.match(l)
        if m and m.group(3) in held:
            d, f, v = m.groups()
            if dnm and d != dnm:
                break
            dnm = d
            off, w = held[v]
            pairs.append((int(f), off, w))
            j += 1
            continue
        break
    if len(pairs) < 4 or not dnm or not snm:
        return None
    # A temporary loaded inside the block and stored after it must not be
    # folded away with the block.  `v57->f16 = (uint16_t)v53;` sat one line past
    # the end because of its cast, and the fold deleted the load of v53 while
    # leaving the store -- three streams changed, which is what the reference
    # comparison is for.  Every load has to be consumed here.
    for v, (off, w) in held.items():
        if not any(s == off and ww == w for _, s, ww in pairs):
            return None
    pairs.sort()
    deltas = {d - s for d, s, _ in pairs}
    if len(deltas) != 1:
        return None
    at = pairs[0][0]
    for d, _, w in pairs:
        if d != at:
            return None
        at = d + w
    return (i, j, re.match(r'^\s*', lines[i]).group(0), dnm, pairs[0][0],
            snm, pairs[0][1], at - pairs[0][0])


def scheduled(lines):
    out, i = [], 0
    while i < len(lines):
        r = sched_at(lines, i)
        if r:
            out.append(r)
            i = r[1]
        else:
            i += 1
    return out


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
    sched = scheduled(lines)
    if not runs and not sched:
        print('no unrolled copies')
        return 0
    if '--list' in sys.argv:
        for i, j, ind, d, dk, s, sk, n, mem in runs:
            print('%s:%d: %d stores, %d bytes, %s <- %s'
                  % (path, i + 1, j - i, n, expr(d, dk), expr(s, sk, mem)))
        for i, j, ind, d, dk, s, sk, n in sched:
            print('%s:%d: scheduled, %d bytes, %s <- %s'
                  % (path, i + 1, n, expr(d, dk, dk != 0), expr(s, sk)))
        print('%d runs and %d scheduled, %d lines -> %d calls'
              % (len(runs), len(sched),
                 sum(j - i for i, j, *_ in runs) + sum(j - i for i, j, *_ in sched),
                 len(runs) + len(sched)))
        return 0

    for i, j, ind, d, dk, s, sk, n in reversed(sched):
        lines[i:j] = ['%sbmf_copy((void *)(%s), (const void *)((uintptr_t)%s), %d);'
                      % (ind, expr(d, dk, dk != 0), expr(s, sk), n)]
    for i, j, ind, d, dk, s, sk, n, mem in reversed(runs):
        lines[i:j] = ['%sbmf_copy((void *)(%s), (const void *)(%s), %d);'
                      % (ind, expr(d, dk), expr(s, sk, mem), n)]
    open(path, 'w').write('\n'.join(lines))
    print('%d runs and %d scheduled folded, %d lines -> %d calls'
          % (len(runs), len(sched),
             sum(j - i for i, j, *_ in runs) + sum(j - i for i, j, *_ in sched),
             len(runs) + len(sched)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
