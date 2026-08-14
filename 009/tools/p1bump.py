#!/usr/bin/env python3
"""Fold `update_model`'s counter bumps into `CounterNode::bump`.

    python3 tools/p1bump.py                  # what it would take
    python3 tools/p1bump.py --list           # and every site
    python3 tools/p1bump.py --apply

`REVIEW.md`, `alt_p1_block.inc`.  `update_model` is 533 lines and 121 pairs of
them are one counter bumped with the node's total kept in step:

    node_alt[1].c[slot_r] += 7;
    node_alt[1].total     += 7;

which is `node_alt[1].bump(slot_r, 7)`.  The increment is one of nine constants
-- 4 at 36 sites, 6 at 27, 3 at 27, 2 at 17, 7 at 10, and 11, 13, 17 and 5 once
each -- and *that* is the thing the file is about: which counters a coded symbol
moves, and by how much.  Written out, the weights are invisible in the noise.

**Both halves have to move together.**  This is why the fold is worth doing
rather than merely shorter: `c[slot]` and `total` are two `uint16_t` of one
record and the coder divides by the total, so a site that bumped one and not the
other would be a defect no type could catch.  As a method there is one place
that can be wrong.

**What it declines.**  Two sites keep the new total in a local because they read
it back a few lines later, so the `+=` on `total` is not the next statement.
They are left alone and reported.
"""
import re
import sys

PATH = 'alt_p1_block.inc'
BODY = 'int32_t update_model()'

# `<node>.c[<slot>] += <n>;` then `<the same node>.total += <the same n>;`
CNT = re.compile(r'^(\s*)(.+?)\.c\[(\w+)\] \+= (\d+);\s*$')
TOT = re.compile(r'^\s*(.+?)\.total \+= (\d+);\s*$')


def sites(lines):
    at = next(i for i, l in enumerate(lines) if BODY in l)
    out = []
    for i in range(at, len(lines) - 1):
        m = CNT.match(lines[i])
        if not m:
            continue
        indent, node, slot, n = m.groups()
        t = TOT.match(lines[i + 1])
        if t and t.group(1) == node and t.group(2) == n:
            out.append((i, 'fold', '%s%s.bump(%s, %s);' % (indent, node, slot, n)))
        else:
            out.append((i, 'total is not the next statement', None))
    return out


def main():
    lines = open(PATH).read().split('\n')
    found = sites(lines)
    take = [f for f in found if f[1] == 'fold']
    for i, why, call in found:
        if '--list' in sys.argv or why != 'fold':
            print('  %s:%d  %s%s' % (PATH, i + 1, why,
                                     '  ->  ' + call.strip() if call else ''))
    print('%d of %d counter bumps fold to CounterNode::bump' % (len(take), len(found)))
    if '--apply' in sys.argv and take:
        drop = {i: call for i, why, call in take}
        out, i = [], 0
        while i < len(lines):
            if i in drop:
                out.append(drop[i])
                i += 2                       # the `total +=` line goes with it
            else:
                out.append(lines[i])
                i += 1
        open(PATH, 'w').write('\n'.join(out))
        print('-- applied to %s (%d lines -> %d)' % (PATH, len(lines), len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
