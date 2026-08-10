#!/usr/bin/env python3
"""Turn `((T *)p)[expr + K]` into the array member of `p` that `K` lands in.

    python3 tools/unindex.py subs1.hpp --list
    python3 tools/unindex.py subs1.hpp __alt_p2_model
    python3 tools/unindex.py subs1.hpp --all

REFACTORING5.md §3.4.  `unoffset.py` handles the shape whose index is a
constant; this one handles the shape whose index is a variable *plus* a
constant, which is the same member reached by dividing its offset by the
access width and hoping the reader multiplies it back:

    ((uint32_t *)v385)[2 * n0x10 + 235020]

`235020 * 4` is byte 940 080, which is `Obj11::f940072[4]` -- so the line is
`*(uint32_t *)&v385->f940072[4 * n0x10 + 4]`, and the 940 080 stops being a
number the reader has to divide.

The access does not have to match the member's element type; it has to be a
whole number of them, aligned.  Reading a `uint32_t` out of a `uint16_t[]` is
a pair of elements and the cast says so -- what would be wrong is reading
*half* of one, and an unaligned or narrower access is left alone.

The scale is the whole risk (§7's first hazard): `expr` counts elements of
`T`, the member counts elements of its own type, so the rewritten index is
`expr * sizeof(T) / sizeof(elem)`.  Getting that factor wrong is a stride
error the gate catches on the first image, which is why this runs one
function at a time.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import merge                                                      # noqa: E402
import structs                                                    # noqa: E402
import unoffset                                                   # noqa: E402

IDX = re.compile(r'(&?)\(\(\s*(\w+ \*?)\s*\*\)\s*(\w+)\)\[([^\]]*?)\+\s*(\d+)\]')
# `*((uint8_t **)p + k)` is a scaled read of a pointer-wide member; the access
# type is `uint8_t *` and `merge.width` already gives 4 for anything starred.
STAR = re.compile(r'\*\(\((\w+) \*\*\)(\w+) \+ (?:(\w+) \+ )?(\d+)\)')
# `*(T *)((uint8_t *)p + 8 * k + 278944)`: a byte offset with a scaled variable
# in it.  The constant places the member and the scale converts the variable
# into that member's elements, which only works when the scale is a whole
# number of them.
BYTE = re.compile(r'\*(?:\((\w+) \*\))?\(\(uint8_t \*\)(\w+) \+ \(?'
                  r'(?:(\d+) \* )?(?:(\w+\+*) \+ )?(\d+)\)\)?')
# `(T *)p + K` only when `K` is the whole offset: `(int32_t *)_this + 2 * v8 +
# 438` has the same prefix and a different meaning, and taking the 2 for the
# offset there is a stride error the compiler happens to catch.
ADD = re.compile(r'\((\w+) \*\)(\w+) \+ (\d+)\s*(?=[),;]|$)')


def arrays(src):
    """{struct: [member]} for every array member, arrays only."""
    out = {}
    for name in re.findall(r'^struct (\w+) \{', src, re.M):
        ms, _ = merge.parse(src, name)
        if ms:
            out[name] = [f for f in ms if f['count'] > 1]
    return out


def place(tables, struct, ty, off):
    """(member, element index) for `off` bytes read as `ty`, or None."""
    w = merge.width(ty)
    if w is None:
        return None
    for f in tables[struct]:
        ew = merge.width(f['ty'])
        if ew and f['off'] <= off < f['end'] and (off - f['off']) % ew == 0 and w % ew == 0:
            return f, (off - f['off']) // ew, w // ew
    return None


def bases(lines, a, b, sig, tables):
    """{local: struct} for everything declared as a pointer to a known struct."""
    ty = {}
    for m in unoffset.PARAM.finditer(sig):
        if m.group(1) in tables:
            ty[m.group(2)] = m.group(1)
    for i in range(a, b + 1):
        for m in unoffset.PARAM.finditer(lines[i].split('//')[0]):
            if m.group(1) in tables:
                ty.setdefault(m.group(2), m.group(1))
    return ty


def scaled(expr, factor):
    """`expr` in units of the member's element type."""
    expr = expr.strip()
    if factor == 1:
        return expr
    m = re.fullmatch(r'(\d+) \* (.+)', expr)
    if m:
        return '%d * %s' % (int(m.group(1)) * factor, m.group(2))
    return '%d * (%s)' % (factor, expr) if re.search(r'[-+]', expr) \
        else '%d * %s' % (factor, expr)


def convert(lines, a, b, sig, tables):
    """(edits, count) for one function."""
    ty = bases(lines, a, b, sig, tables)
    edits, n = {}, 0

    def byte(m):
        nonlocal n
        acc, base, scale, var, k = m.groups()
        acc = acc or 'uint8_t'          # `*((uint8_t *)p + …)` casts once
        scale = scale or ('1' if var else None)
        if base not in ty:
            return m.group(0)
        w = merge.width(acc)
        if w is None:
            return m.group(0)
        got = place(tables, ty[base], acc, int(k))
        if not got:
            return m.group(0)
        f, j, _ = got
        ew = merge.width(f['ty'])
        if var and int(scale) % ew:
            return m.group(0)
        idx = '%d * %s' % (int(scale) // ew, var) if var else ''
        if idx and int(scale) // ew == 1:
            idx = var
        idx = '%s + %d' % (idx, j) if idx and j else (idx or str(j))
        n += 1
        at = '%s->%s[%s]' % (base, f['name'], idx)
        return at if unoffset.norm(f['ty']) == acc else '(*(%s *)&%s)' % (acc, at)

    def star(m):
        """`*((T **)p + v + K)`: a scaled read of a pointer-wide member."""
        nonlocal n
        acc, base, var, k = m.groups()
        if base not in ty:
            return m.group(0)
        acc += ' *'
        got = place(tables, ty[base], acc, 4 * int(k))
        if not got:
            return m.group(0)
        f, j, _ = got
        if merge.width(f['ty']) != 4:
            return m.group(0)
        n += 1
        idx = '%s + %d' % (var, j) if var and j else (var or str(j))
        at = '%s->%s[%s]' % (base, f['name'], idx)
        return at if unoffset.norm(f['ty']) == acc.strip() else '(*(%s *)&%s)' % (acc, at)

    def rep(m, kind):
        nonlocal n
        amp = m.group(1) if kind == 'idx' else ''
        g = m.groups()[1:] if kind == 'idx' else m.groups()
        acc, base = g[0].strip(), g[1]
        k = int(g[3] if kind == 'idx' else g[2])
        if base not in ty:
            return m.group(0)
        w = merge.width(acc)
        if w is None:
            return m.group(0)
        got = place(tables, ty[base], acc, k * w)
        if not got:
            return m.group(0)
        f, j, factor = got
        n += 1
        same = unoffset.norm(f['ty']) == acc
        if kind == 'add':
            if m.group(0).startswith('*('):        # STAR: a dereference
                at = '%s->%s[%d]' % (base, f['name'], j)
                return at if same else '(*(%s *)&%s)' % (acc, at)
            if same:
                return '%s->%s' % (base, f['name']) if j == 0 \
                    else '%s->%s + %d' % (base, f['name'], j)
            return '(%s *)&%s->%s[%d]' % (acc, base, f['name'], j)
        idx = scaled(g[2], factor)
        idx = '%s + %d' % (idx, j) if j else idx
        at = '%s->%s[%s]' % (base, f['name'], idx)
        if same:
            return amp + at
        # `&((T *)p)[i]` is a pointer, so it keeps one cast rather than
        # gaining a `&(*(T *)&…)` around a dereference it never performs.
        return '(%s *)&%s' % (acc, at) if amp else '(*(%s *)&%s)' % (acc, at)

    for i in range(a, b + 1):
        code, sep, com = lines[i].partition('//')
        s = IDX.sub(lambda m: rep(m, 'idx'), code)
        s = ADD.sub(lambda m: rep(m, 'add'), s)
        s = BYTE.sub(byte, s)
        s = STAR.sub(star, s)
        if s != code:
            edits[i] = s + sep + com
    return edits, n


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    lines = src.split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]
    tables = arrays(src)

    plans = []
    for a, b, nm, sig in structs.bodies(lines):
        edits, n = convert(lines, a, b, sig, tables)
        if n:
            plans.append((nm, edits, n))

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for nm, _, n in sorted(plans, key=lambda p: -p[2]):
            print('%-28s %3d' % (nm.lstrip('_'), n))
        print('%d scaled indices become members' % sum(p[2] for p in plans))
        return 0

    want, done = set(args), 0
    for nm, edits, n in plans:
        if want and nm not in want:
            continue
        for i, s in edits.items():
            lines[i] = s
        done += n
        print('%-28s %3d' % (nm.lstrip('_'), n))
    if done:
        open(path, 'w').write('\n'.join(lines))
    print('%d converted' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
