#!/usr/bin/env python3
"""Fold an 18-byte record copy whose loads MSVC hoisted above its stores.

    python3 tools/unrechoist.py bmf.cpp --list
    python3 tools/unrechoist.py bmf.cpp --all

`unrec.py` takes the unrolled copy whose five statements are five
load-stores in a row.  MSVC did not always emit it that way: where it had
registers to spare it issued some of the loads early and interleaved them
with the stores, so the five statements become eight and `unrec` sees none
of them.

    v22 = *(uint32_t *)(v21 - 10);
    v23 = *(uint32_t *)(v21 - 6);
    *(uint32_t *)(v21 + 18) = *(uint32_t *)(v21 - 18);
    *(uint32_t *)(v21 + 22) = *(uint32_t *)(v21 - 14);
    v24 = *(uint16_t *)(v21 - 2);
    *(uint32_t *)(v21 + 26) = v22;
    *(uint32_t *)(v21 + 30) = v23;
    *(uint16_t *)(v21 + 34) = v24;

The five stores cover +18 .. +35 and their five sources cover -18 .. -1, so
the block is `((P2Ctx *)v21)[1] = ((P2Ctx *)v21)[-1]` -- one record forward
from one record back, which is what the sliding window does.

Only the *stores* are replaced.  The loads stay where they are, because
proving a hoisted value is dead afterwards is a separate question from
proving the stores are one move -- `LOWORD(v29) = ...` overwrites half of a
value loaded three lines earlier, and whether anything downstream still
wants it is `unwrite.py`'s job, run afterwards.  Source and destination are
required not to overlap, which is what makes the ordering inside the block
irrelevant.
"""
import re
import sys

REC = 18
W = {'uint32_t': 4, 'uint16_t': 2}
STORE_L = re.compile(r'^(\s*)\*\((uint32_t|uint16_t) \*\)\((\w+) ([-+]) (\d+)\) = '
                     r'\*\((uint32_t|uint16_t) \*\)\((\w+) ([-+]) (\d+)\);$')
# The value is a name, sometimes behind the cast a pointer-typed spill slot
# needs: MSVC put a half-word load into a register that also held an address,
# so `LOWORD(v34) = ...` is followed by `(uint16_t)(uintptr_t)v34`.
STORE_V = re.compile(r'^(\s*)\*\((uint32_t|uint16_t) \*\)\((\w+) ([-+]) (\d+)\) = '
                     r'(?:\(uint16_t\)\(uintptr_t\))?(\w+);$')
LOAD = re.compile(r'^\s*(?:LOWORD\()?(\w+)\)? = '
                  r'\*\((uint32_t|uint16_t) \*\)\((\w+) ([-+]) (\d+)\);$')


def off(sign, n):
    return int(n) * (-1 if sign == '-' else 1)


def group(lines, i):
    """(indent, base, dst record, src record, [store line]) starting at line i."""
    base, loads, stores, ind = None, {}, [], None
    j = i
    while j < len(lines):
        line = lines[j]
        m = STORE_L.match(line)
        if m and m.group(2) == m.group(6) and m.group(3) == m.group(7):
            if base not in (None, m.group(3)):
                break
            base, ind = m.group(3), ind if ind is not None else m.group(1)
            stores.append((j, W[m.group(2)], off(m.group(4), m.group(5)),
                           off(m.group(8), m.group(9))))
            j += 1
            continue
        m = STORE_V.match(line)
        if m and m.group(6) in loads:
            w, src = loads[m.group(6)]
            if base not in (None, m.group(3)) or w != W[m.group(2)]:
                break
            base, ind = m.group(3), ind if ind is not None else m.group(1)
            stores.append((j, W[m.group(2)], off(m.group(4), m.group(5)), src))
            j += 1
            continue
        m = LOAD.match(line)
        if m and base in (None, m.group(3)):
            base = m.group(3)
            loads[m.group(1)] = (W[m.group(2)], off(m.group(4), m.group(5)))
            j += 1
            continue
        break

    if len(stores) != 5:
        return None
    stores.sort(key=lambda s: s[2])
    if [s[1] for s in stores] != [4, 4, 4, 4, 2]:
        return None
    d0, s0, step = stores[0][2], stores[0][3], 0
    for _, w, d, s in stores:
        if d != d0 + step or s != s0 + step:
            return None
        step += w
    # A record boundary on both ends, and no overlap -- the second is what
    # makes the order of the eight statements irrelevant.
    if step != REC or d0 % REC or s0 % REC or abs(d0 - s0) < REC:
        return None
    return ind, base, d0 // REC, s0 // REC, [s[0] for s in stores], j


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found, i = [], 0
    while i < len(lines):
        g = group(lines, i)
        if not g:
            i += 1
            continue
        found.append(g)
        i = g[5]

    if '--all' not in sys.argv:
        for ind, base, d, s, at, _ in found:
            print('%6d  ((P2Ctx *)%s)[%d] = ((P2Ctx *)%s)[%d];' % (at[0] + 1, base, d, base, s))
        print('%d hoisted record copies, %d stores' % (len(found), 5 * len(found)))
        return 0

    for ind, base, d, s, at, _ in reversed(found):
        for k in reversed(at[1:]):
            del lines[k]
        lines[at[0]] = '%s((P2Ctx *)%s)[%d] = ((P2Ctx *)%s)[%d];' % (ind, base, d, base, s)
    open(path, 'w').write('\n'.join(lines))
    print('%d hoisted record copies became one assignment each' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
