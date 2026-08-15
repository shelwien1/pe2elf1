#!/usr/bin/env python3
"""Fold an unrolled 18-byte record copy into one assignment.

    python3 tools/unrec.py subs1.hpp --list
    python3 tools/unrec.py subs1.hpp --all

`algorithm_v2.md` §9 found the p2 table's record size by counting the places
that copy one: the compiler unrolled every 18-byte move into four dwords and a
word, and 56 of those survived into the source.

    *(uint32_t *)(v45 + v46 - 18) = *(uint32_t *)(v46 + v45);
    *(uint32_t *)(v45 + v46 - 14) = *(uint32_t *)(v46 + v45 + 4);
    *(uint32_t *)(v45 + v46 - 10) = *(uint32_t *)(v46 + v45 + 8);
    *(uint32_t *)(v45 + v46 - 6)  = *(uint32_t *)(v46 + v45 + 12);
    *(uint16_t *)(v45 + v46 - 2)  = *(uint16_t *)(v46 + v45 + 16);

is `((P2Ctx *)(v45 + v46))[-1] = ((P2Ctx *)(v45 + v46))[0];` -- one record
back, which is what the sliding window does at the end of a row.

A group has to be five consecutive statements, 4+4+4+4+2 bytes, contiguous on
both sides, over the same set of names on both sides.  Nothing else is folded:
the point is that the five lines are one move, and five lines that are not one
move are not this.
"""
import re
import sys

STORE = re.compile(r'^(\s*)\*\((uint32_t|uint16_t) \*\)\(([^()]*)\) = '
                   r'\*\((uint32_t|uint16_t) \*\)\(([^()]*)\);$')
WIDTH = {'uint32_t': 4, 'uint16_t': 2}


def addr(expr):
    """(frozenset of names, constant) for `a + b - 18`, or None."""
    expr = expr.strip()
    names, const, sign = [], 0, 1
    for tok in re.findall(r'[-+]|[\w]+', expr):
        if tok == '+':
            sign = 1
        elif tok == '-':
            sign = -1
        elif tok.isdigit():
            const += sign * int(tok)
            sign = 1
        elif re.fullmatch(r'\w+', tok):
            if sign != 1:
                return None                   # a subtracted name is not a base
            names.append(tok)
        else:
            return None
    return frozenset(names), const


def group(lines, i):
    """(base names, dst, src, indent) if a record copy starts at line i."""
    parts = []
    for j in range(i, min(i + 5, len(lines))):
        m = STORE.match(lines[j])
        if not m or m.group(2) != m.group(4):
            return None
        d, s = addr(m.group(3)), addr(m.group(5))
        if d is None or s is None or d[0] != s[0]:
            return None
        parts.append((m.group(1), WIDTH[m.group(2)], d, s))
    if len(parts) < 5:
        return None
    if [p[1] for p in parts] != [4, 4, 4, 4, 2]:
        return None
    d0, s0 = parts[0][2][1], parts[0][3][1]
    off = 0
    for ind, w, d, s in parts:
        if d[1] != d0 + off or s[1] != s0 + off:
            return None
        off += w
    # Both ends have to sit on a record boundary, not merely a record apart:
    # `base - 14` to `base + 4` is eighteen bytes and is not `rec[-1] = rec[0]`.
    if off != 18 or d0 % 18 or s0 % 18:
        return None
    return parts[0][2][0], d0, s0, parts[0][0]


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    out, i, n = [], 0, 0
    while i < len(lines):
        g = group(lines, i)
        if not g:
            out.append(lines[i])
            i += 1
            continue
        names, d0, s0, ind = g
        base = ' + '.join(sorted(names))
        rec = '((P2Ctx *)(%s))' % base
        out.append('%s%s[%d] = %s[%d];' % (ind, rec, d0 // 18, rec, s0 // 18))
        n += 1
        i += 5
    if '--list' in sys.argv and '--all' not in sys.argv:
        print('%d unrolled record copies' % n)
        return 0
    open(path, 'w').write('\n'.join(out))
    print('%d unrolled record copies became one assignment each' % n)
    return 0


if __name__ == '__main__':
    sys.exit(main())
