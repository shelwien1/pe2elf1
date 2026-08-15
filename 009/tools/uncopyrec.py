#!/usr/bin/env python3
"""Turn an 18-byte `bmf_copy` into a record assignment.

    python3 tools/uncopyrec.py subs1.hpp --list
    python3 tools/uncopyrec.py subs1.hpp --all

Round three folded the unrolled 18-byte moves into `bmf_copy(dst, src, 18)`
before there was a type for the thing being moved.  There is one now:

    bmf_copy((void *)((uint8_t *)v25 + 36), (const void *)((uintptr_t)v25 - 18), 18);
    ((P2Ctx *)v25)[2] = ((P2Ctx *)v25)[-1];

Both ends have to be a name plus a multiple of eighteen; that is what makes
the two spellings the same bytes.  The names need not be the same one -- half
of these copy between two cursors Hex-Rays gave different names to the same
`f278736[0]` -- and a call whose ends do not land on a record keeps `bmf_copy`,
which stays in the file for the copies that are not records.
"""
import re
import sys

CALL = re.compile(r'bmf_copy\(\(void \*\)(.+?), \(const void \*\)(.+?), 18\);')


def addr(expr):
    """(base, byte offset) for `((uint8_t *)v25 + 36)`, or None."""
    e = expr.strip()
    while e.startswith('(') and e.endswith(')'):
        inner = e[1:-1]
        if inner.count('(') != inner.count(')'):
            break
        e = inner.strip()
    e = re.sub(r'^\((?:uint8_t|int8_t|char) \*\)\s*', '', e)
    e = re.sub(r'^\(uintptr_t\)\s*', '', e)
    m = re.fullmatch(r'(\w+)(?:\s*([-+])\s*(\d+))?', e)
    if not m:
        return None
    off = int(m.group(3) or 0) * (-1 if m.group(2) == '-' else 1)
    return m.group(1), off


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    src = open(path).read()
    n = [0, 0]

    def rep(m):
        d, s = addr(m.group(1)), addr(m.group(2))
        if not d or not s or d[1] % 18 or s[1] % 18:
            n[1] += 1
            return m.group(0)
        n[0] += 1
        return '((P2Ctx *)%s)[%d] = ((P2Ctx *)%s)[%d];' % (
            d[0], d[1] // 18, s[0], s[1] // 18)

    out = CALL.sub(rep, src)
    if '--list' in sys.argv and '--all' not in sys.argv:
        print('%d of %d 18-byte copies are record assignments'
              % (n[0], n[0] + n[1]))
        return 0
    open(path, 'w').write(out)
    print('%d became record assignments, %d left as bmf_copy' % tuple(n))
    return 0


if __name__ == '__main__':
    sys.exit(main())
