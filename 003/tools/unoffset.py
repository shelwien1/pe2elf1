#!/usr/bin/env python3
"""Turn `*(T *)((char *)p + N)` into the member of `p` that lives at N.

    python3 tools/unoffset.py subs1.hpp --list
    python3 tools/unoffset.py subs1.hpp __alt_p1_context
    python3 tools/unoffset.py subs1.hpp --all

REFACTORING3.md §2.1's whole complaint, applied where the struct already says
the answer.  `alt_p1_alloc` reached `Obj0`'s `f0`, `f4`, `f8` and `f12[51]`
forty-nine times as `*((int32_t *)_this + K)`; once `Obj92` was recognised as
`Obj0` those were members and the arithmetic was noise.

Three spellings, all of them Hex-Rays':

    *(T *)((char *)p + N)      a byte offset
    *((T *)p + K)              a scaled one, N = K * sizeof(T)
    *(T *)(p + N)              where `p` is already `char *`

**The access type has to match the member's.**  Replacing a `uint16_t` store
with a `uint32_t` member at the same offset compiles, passes seven of the
fifteen images, and corrupts the heap -- so a width mismatch is left alone and
counted rather than converted.  Reading a member through a *narrower* type is
usually a union in disguise and is left alone for the same reason.

An offset that lands inside an array member becomes a subscript when it lands
on an element, and is left alone when it does not.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import merge                                                      # noqa: E402
import structs                                                    # noqa: E402

# `T *p` in a signature or a declaration, for the struct `p` points at
PARAM = re.compile(r'\b([A-Z]\w+)\s*\*\s*&?\s*(\w+)\b')
BYTEP = r'(?:char|int8_t|uint8_t)'      # §2.2 retyped the byte pointer
FORMS = [
    # (regex, how to read the offset from the match)
    (re.compile(r'\*\(([\w ]+?\s*\*?)\s*\*\)\(\(%s \*\)(\w+) \+ (\d+)\)' % BYTEP), 'byte'),
    (re.compile(r'\*\(\(([\w ]+?\s*\*?)\s*\*\)(\w+) \+ (\d+)\)'), 'scaled'),
    (re.compile(r'\*\(([\w ]+?\s*\*?)\s*\*\)\((\w+) \+ (\d+)\)'), 'byte'),
]


def members(src, name):
    """{offset: (type, member-expression)} for one struct, arrays included."""
    ms, _ = merge.parse(src, name)
    if ms is None:
        return None
    out = {}
    for f in ms:
        w = merge.width(f['ty'])
        for k in range(f['count']):
            expr = f['name'] + ('[%d]' % k if f['count'] > 1 else '')
            out.setdefault(f['off'] + k * w, (f['ty'], expr))
    return out


def norm(ty):
    return ' '.join(ty.split()).replace(' *', '*')


def convert(lines, a, b, sig, tables, only=None):
    """(edits, converted, skipped-for-width) for one function."""
    ty = {}
    for m in PARAM.finditer(sig):
        if m.group(1) in tables:
            ty[m.group(2)] = m.group(1)
    for i in range(a, b + 1):
        for m in PARAM.finditer(lines[i].split('//')[0]):
            if m.group(1) in tables:
                ty.setdefault(m.group(2), m.group(1))
    if only:
        ty = {k: v for k, v in ty.items() if k in only}
    edits, n, wide = {}, 0, 0

    def rep(m, kind):
        nonlocal n, wide
        acc, base, num = norm(m.group(1)), m.group(2), int(m.group(3))
        if base not in ty:
            return m.group(0)
        tbl = tables[ty[base]]
        step = merge.width(m.group(1).strip()) if kind == 'scaled' else 1
        if step is None:
            return m.group(0)
        off = num * step
        if off not in tbl:
            return m.group(0)
        mty, expr = tbl[off]
        if norm(mty) == acc:
            n += 1
            return '%s->%s' % (base, expr)
        # Same width, different reading: `alt_p1_context` writes
        # `*((uint8_t **)_this + 20)` into a slot it then compares `< 0` and
        # against `nullptr`, so the slot is an integer and the cast is the
        # access's, not the member's.  Say that at the member instead of
        # arithmetic -- the bits are identical and the offset stops being a
        # number.  A *narrower* access is a union in disguise and is left alone.
        if merge.width(m.group(1).strip()) == merge.width(mty):
            n += 1
            return '*(%s *)&%s->%s' % (m.group(1).strip(), base, expr)
        wide += 1
        return m.group(0)

    for i in range(a, b + 1):
        s = lines[i]
        for rx, kind in FORMS:
            s = rx.sub(lambda m, k=kind: rep(m, k), s)
        if s != lines[i]:
            edits[i] = s
    return edits, n, wide


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    lines = src.split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    tables = {}
    for name in re.findall(r'^struct (\w+) \{', src, re.M):
        t = members(src, name)
        if t:
            tables[name] = t

    plans = []
    for a, b, nm, sig in structs.bodies(lines):
        edits, n, wide = convert(lines, a, b, sig, tables)
        if n or wide:
            plans.append((nm, edits, n, wide))

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for nm, edits, n, wide in sorted(plans, key=lambda p: -p[2]):
            print('%-28s %3d convert, %3d a different width'
                  % (nm.lstrip('_'), n, wide))
        print('%d offsets become members, %d are a width the member is not'
              % (sum(p[2] for p in plans), sum(p[3] for p in plans)))
        return 0

    want, done = set(args), 0
    for nm, edits, n, wide in plans:
        if want and nm not in want:
            continue
        for i, s in edits.items():
            lines[i] = s
        done += n
        if n:
            print('%-28s %3d offsets became members' % (nm.lstrip('_'), n))
    if done:
        open(path, 'w').write('\n'.join(lines))
    print('%d converted' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
