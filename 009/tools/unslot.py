#!/usr/bin/env python3
"""Give a frame slot's extra names their own storage.

    python3 tools/unslot.py bmf.cpp --list
    python3 tools/unslot.py bmf.cpp alt_p2_context slot0

REFACTORING4.md §5 item 3.  MSVC reused one stack slot for several variables
whose live ranges do not overlap, and Hex-Rays named each use, so round three's
frame reconstruction ended with

    int32_t &v246 = *(int32_t *)((char *)frame.slot0);
    int32_t &v248 = *(int32_t *)((char *)frame.slot0);

-- eight names for four bytes in `alt_p2_context` alone.  Where the names
really are separate variables, each can have its own storage and the reader
stops having to prove they never overlap.  Where they are not -- where the
body writes through one and reads through another -- the gate says so, and
`frame-sweep.sh`'s discipline applies: propose, gate, keep or put back.

The first name on a slot keeps the slot, so a frame's byte layout never moves;
only the extras become locals.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

# Every alias spelling round three left, because the extra names on one slot
# rarely share a type: `T &x = frame.m;`, `T *&x = *(T **)((char *)&frame.m);`
# and `T &x = *(T *)&frame.m[0];` are all one name bound to one member.  The
# reference declaration's own type is the one to keep -- it is what the body
# reads through.
ALIAS = re.compile(r'^  ((?:const )?[\w]+(?: ?\*)*) ?& ?(\w+) = '
                   r'[^;]*frame\.(\w+(?:\[\d+\])?)[^;]*;\s*(?://.*)?$')


def slots(lines):
    """{function: {slot: [(name, type, line)]}} for slots with two or more."""
    out = {}
    for a, b, nm, _ in structs.bodies(lines):
        g = collections.defaultdict(list)
        for i in range(a, b + 1):
            m = ALIAS.match(lines[i])
            if m:
                g[m.group(3)].append((m.group(2), m.group(1).rstrip(), i))
        g = {k: v for k, v in g.items() if len(v) > 1}
        if g:
            out[nm.lstrip('_')] = g
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found = slots(lines)
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    if len(args) < 2:
        for fn, g in found.items():
            for slot, al in g.items():
                print('%-24s %-8s %s' % (fn, slot,
                                         ', '.join('%s:%s' % (n, t) for n, t, _ in al)))
        if structs.no_frames(lines):
            print('not applicable: no `struct alignas(16)` frame in this file')
            return 0
        print('%d slots, %d extra names'
              % (sum(len(g) for g in found.values()),
                 sum(len(v) - 1 for g in found.values() for v in g.values())))
        return 0

    fn, slot = args[0], args[1]
    al = found.get(fn, {}).get(slot)
    if not al:
        print('no such slot')
        return 1
    for name, ty, i in al[1:]:                 # the first keeps the slot
        sep = '' if ty.endswith('*') else ' '
        lines[i] = '  %s%s%s;   // was a second name for frame.%s' % (ty, sep, name, slot)
    open(path, 'w').write('\n'.join(lines))
    print('%s %s: %d names got their own storage' % (fn, slot, len(al) - 1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
