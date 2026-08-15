#!/usr/bin/env python3
"""Turn one function's `__hexrays_frame` buffer back into ordinary locals.

    python3 tools/unframe.py subs1.hpp __code_pixel
    python3 tools/unframe.py subs1.hpp __code_pixel --struct
    python3 tools/unframe.py subs1.hpp --list

Hex-Rays could not always recover a function's stack frame as named locals, so
the extractor emitted the frame as one array and bound each local to an offset
in it by reference:

    alignas(16) uint8_t __hexrays_frame[26712];
    char     (&buf)[4096] = *(char (*)[4096])(__hexrays_frame + 0);
    int32_t  (&v72)[1024] = *(int32_t (*)[1024])(__hexrays_frame + 4096);
    int32_t  &v83         = *(int32_t *)(__hexrays_frame + 24600);

Every one of those is a plain local wearing a costume.  This takes the costume
off: each alias becomes a declaration and the buffer goes.

**It changes the stack layout**, which is the whole reason it needs the gate
after it.  The aliases are the only thing keeping these locals adjacent and in
that order, and this is decompiled code that indexes past the ends of arrays.
`buf[4096]` above is followed immediately by `v72[1024]`; a body that walks one
past `buf` reads `v72[0]` today and reads whatever the compiler puts there
afterwards.

Aliases that share bytes are common and are not noise: Hex-Rays reuses a stack
slot and gives it two names, so `v95` and `v96` are the same four bytes, or a
`char` sits inside a `uint32_t`.  Those keep their aliasing.  Each overlapping
group gets one owner -- the member spanning the whole group, or a byte array
where no member does -- and the rest are bound to it:

    int32_t v95;
    int32_t &v96 = v95;

which is what the frame was saying, with the frame gone.

Most frames will not survive the plain split, and the gate says so loudly: 16
of the 24 here crash or change their output.  The frames have bytes no alias
names -- `__sub_405CF0` names 29 220 of 41 456 -- and the code reaches them by
running off the end of the alias next door.  Splitting takes that slack away.

`--struct` is the fallback for those, and it preserves the layout exactly:

    struct { char buf[4096]; int32_t v72[1024]; uint8_t _pad0[36]; ... } frame;
    char (&buf)[4096] = frame.buf;

Same order, same offsets, gaps spelled out as padding, and every use site
unchanged.  It does not shorten the declarations, but it removes every
`*(T *)(frame + N)` cast -- which is the part that has to go before this
compiles for a 64-bit target -- and it gives the frame's shape a name instead of
leaving it implicit in a list of offsets.

Refusals, rather than guesses:

  * an alias whose type this cannot size is left alone, and so is its function;
  * any use of `__hexrays_frame` other than the declaration and the aliases
    stops the function being converted at all.
"""
import re
import sys

DECL = re.compile(r'^(\s*)alignas\((\d+)\) uint8_t __hexrays_frame\[(\d+)\];\s*$')
# `T &name = *(T *)(__hexrays_frame + N);`
SCALAR = re.compile(r'^(\s*)(?P<type>(?:const )?[A-Za-z_][A-Za-z0-9_]*(?: *\*)*) &'
                    r'(?P<name>[A-Za-z_][A-Za-z0-9_]*) = '
                    r'\*\((?P=type) *\*\)\(__hexrays_frame \+ (?P<off>\d+)\);\s*$')
# `T (&name)[K] = *(T (*)[K])(__hexrays_frame + N);`
ARRAY = re.compile(r'^(\s*)(?P<type>(?:const )?[A-Za-z_][A-Za-z0-9_]*(?: *\*)*) '
                   r'\(&(?P<name>[A-Za-z_][A-Za-z0-9_]*)\)\[(?P<n>\d+)\] = '
                   r'\*\((?P=type) *\(\*\)\[(?P=n)\]\)\(__hexrays_frame \+ (?P<off>\d+)\);\s*$')

ALIGN_OF = {'__m128': 16, '__m128i': 16, '__m128d': 16}
SIZES = {'char': 1, 'uint8_t': 1, 'int8_t': 1, 'uint16_t': 2, 'int16_t': 2,
         'int32_t': 4, 'uint32_t': 4, 'float': 4, 'int64_t': 8, 'uint64_t': 8,
         'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16, 'FILE': 4}


def sizeof(t):
    t = t.strip()
    if t.startswith('const '):
        t = t[6:]
    if t.endswith('*'):
        return 4                      # 32-bit target; see REFACTORING.md §4.2
    return SIZES.get(t)


def alignof(t):
    """Alignment the compiler will give this type -- for an array, its element's."""
    t = t.strip()
    if t.startswith('const '):
        t = t[6:]
    if t.endswith('*'):
        return 4
    return ALIGN_OF.get(t, min(SIZES.get(t, 4), 8))


def bodies(lines):
    """(start, end, name) for every top-level body."""
    out, depth, start, name = [], 0, None, None
    for i, l in enumerate(lines):
        s = l.split('//')[0]
        for k, ch in enumerate(s):
            if ch == '{':
                if depth == 0:
                    buf, j = s[:k], i
                    while True:
                        if buf.count(')') - buf.count('(') <= 0 and '(' in buf:
                            break
                        j -= 1
                        if j < 0:
                            break
                        buf = lines[j].split('//')[0] + ' ' + buf
                    m = re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', buf)
                    name = m.group(1) if m else '?'
                    start = i
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and start is not None:
                    out.append((start, i, name))
                    start = None
    return out


def convert(lines, a, b):
    """Returns (new_body_lines, note) or (None, reason)."""
    body = lines[a:b + 1]
    decl = [k for k, l in enumerate(body) if DECL.match(l)]
    if len(decl) != 1:
        return None, 'no single frame declaration'
    d = decl[0]
    align, size = DECL.match(body[d]).group(2), int(DECL.match(body[d]).group(3))

    aliases, other = [], []
    for k, l in enumerate(body):
        if k == d or '__hexrays_frame' not in l:
            continue
        m = SCALAR.match(l) or ARRAY.match(l)
        if not m:
            other.append(k)
            continue
        g = m.groupdict()
        n = int(g.get('n', 1))
        w = sizeof(g['type'])
        if w is None:
            other.append(k)
            continue
        aliases.append((k, m.re is ARRAY, m.group(1), g['type'], g['name'],
                        n, int(g['off']), w * n))
    if other:
        return None, '%d uses this cannot read (first at body line %d)' % (
            len(other), other[0] + 1)
    if not aliases:
        return None, 'no aliases'

    # Group the aliases that share bytes.  Sorted by offset, a new group starts
    # wherever an alias begins at or after the end of everything before it.
    aliases.sort(key=lambda a: (a[6], -a[7]))
    groups, end = [], -1
    for al in aliases:
        off, sz = al[6], al[7]
        if off >= end:
            groups.append([])
            end = off
        groups[-1].append(al)
        end = max(end, off + sz)

    def declare(ind, ty, nm, n, is_arr):
        star = '' if ty.endswith('*') else ' '
        return ('%s%s%s%s[%d];' % (ind, ty, star, nm, n) if is_arr
                else '%s%s%s%s;' % (ind, ty, star, nm))

    def bind(ind, ty, nm, n, is_arr, expr):
        star = '' if ty.endswith('*') else ' '
        return ('%s%s%s(&%s)[%d] = *(%s%s(*)[%d])(%s);'
                % (ind, ty, star, nm, n, ty, star, n, expr) if is_arr
                else '%s%s%s&%s = *(%s%s*)(%s);' % (ind, ty, star, nm, ty, star, expr))

    new, shared = [], 0
    for g in groups:
        lo = min(a[6] for a in g)
        hi = max(a[6] + a[7] for a in g)
        if len(g) == 1:
            k, is_arr, ind, ty, nm, n, off, sz = g[0]
            new.append(declare(ind, ty, nm, n, is_arr))
            continue
        shared += 1
        owner = next((a for a in g if a[6] == lo and a[6] + a[7] == hi), None)
        ind = g[0][2]
        if owner is not None:
            k, is_arr, _, ty, nm, n, off, sz = owner
            new.append(declare(ind, ty, nm, n, is_arr))
            base = '(char *)&%s' % nm
        else:
            nm = '%s_u' % g[0][4]
            new.append('%salignas(%s) uint8_t %s[%d];   // shared slot'
                       % (ind, align, nm, hi - lo))
            base, owner = nm, None
            base = '(char *)%s' % nm
        for a in g:
            if owner is not None and a is owner:
                continue
            k, is_arr, _, ty, anm, n, off, sz = a
            delta = off - lo
            expr = base if delta == 0 else '%s + %d' % (base, delta)
            if owner is not None and delta == 0 and not is_arr \
               and ty == owner[3] and not owner[1]:
                new.append('%s%s%s&%s = %s;' % (ind, ty,
                                                '' if ty.endswith('*') else ' ',
                                                anm, owner[4]))
            else:
                new.append(bind(ind, ty, anm, n, is_arr, expr))

    # The declarations go where the frame was, not at the top of the body:
    # `body[0]` is the opening brace.
    drop = {k for k, *_ in aliases}
    out = []
    for k, l in enumerate(body):
        if k == d:
            out.extend(new)
        elif k not in drop:
            out.append(l)
    used = sum(a[7] for a in aliases)
    return out, ('%d aliases in %d groups (%d shared), %d of %d frame bytes'
                 % (len(aliases), len(groups), shared, used, size))


def convert_struct(lines, a, b):
    """Same aliases, but backed by a struct with the frame's exact layout."""
    body = lines[a:b + 1]
    decl = [k for k, l in enumerate(body) if DECL.match(l)]
    if len(decl) != 1:
        return None, 'no single frame declaration'
    d = decl[0]
    m0 = DECL.match(body[d])
    align, size, ind = m0.group(2), int(m0.group(3)), m0.group(1)

    aliases, other = [], []
    for k, l in enumerate(body):
        if k == d or '__hexrays_frame' not in l:
            continue
        m = SCALAR.match(l) or ARRAY.match(l)
        if not m:
            other.append(k)
            continue
        g = m.groupdict()
        n = int(g.get('n', 1))
        w = sizeof(g['type'])
        if w is None:
            other.append(k)
            continue
        aliases.append((k, m.re is ARRAY, m.group(1), g['type'], g['name'],
                        n, int(g['off']), w * n))
    if other:
        return None, '%d uses this cannot read' % len(other)
    if not aliases:
        return None, 'no aliases'

    aliases.sort(key=lambda x: (x[6], -x[7]))
    groups, end = [], -1
    for al in aliases:
        if al[6] >= end:
            groups.append([])
            end = al[6]
        groups[-1].append(al)
        end = max(end, al[6] + al[7])

    members, binds, at, pad = [], [], 0, 0
    for g in groups:
        lo = min(x[6] for x in g)
        hi = max(x[6] + x[7] for x in g)
        if lo > at:
            members.append('%s    uint8_t _pad%d[%d];' % (ind, pad, lo - at))
            pad += 1
        owner = next((x for x in g if x[6] == lo and x[6] + x[7] == hi), None)
        if owner is not None and len(g) == 1 and lo % alignof(owner[3]) == 0:
            _, is_arr, _, ty, nm, n, _, _ = owner
            star = '' if ty.endswith('*') else ' '
            members.append('%s    %s%s%s%s;' % (ind, ty, star, nm,
                                                '[%d]' % n if is_arr else ''))
            mem, base = nm, '(char *)&frame.%s' % nm
        else:
            mem = 'slot%d' % lo
            members.append('%s    uint8_t %s[%d];' % (ind, mem, hi - lo))
            base = '(char *)frame.%s' % mem
        for x in g:
            _, is_arr, _, ty, nm, n, off, _ = x
            star = '' if ty.endswith('*') else ' '
            if owner is not None and len(g) == 1 and lo % alignof(ty) == 0:
                # A member of its own: bind the name the body already uses.
                binds.append('%s%s%s(&%s)[%d] = frame.%s;'
                             % (ind, ty, star, nm, n, nm) if is_arr
                             else '%s%s%s&%s = frame.%s;' % (ind, ty, star, nm, nm))
                continue
            delta = off - lo
            expr = base if delta == 0 else '%s + %d' % (base, delta)
            if is_arr:
                binds.append('%s%s%s(&%s)[%d] = *(%s%s(*)[%d])(%s);'
                             % (ind, ty, star, nm, n, ty, star, n, expr))
            else:
                binds.append('%s%s%s&%s = *(%s%s*)(%s);'
                             % (ind, ty, star, nm, ty, star, expr))
        at = hi
    if at < size:
        members.append('%s    uint8_t _pad%d[%d];' % (ind, pad, size - at))

    rounded = -(-size // int(align)) * int(align)
    new = ([('%sstruct alignas(%s) {   // %d bytes, the frame Hex-Rays could '
             'not name' % (ind, align, size))]
           + members
           + ['%s} frame;' % ind,
              '%sstatic_assert(sizeof(frame) == %d, "frame layout moved");'
              % (ind, rounded)]
           + binds)
    drop = {k for k, *_ in aliases}
    out = []
    for k, l in enumerate(body):
        if k == d:
            out.extend(new)
        elif k not in drop:
            out.append(l)
    return out, ('%d aliases, %d members, %d pads, layout preserved'
                 % (len(aliases), len(members), pad))


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    todo = sys.argv[2]
    fn = convert_struct if '--struct' in sys.argv else convert

    if todo == '--list':
        for a, b, n in bodies(lines):
            if any(DECL.match(l) for l in lines[a:b + 1]):
                new, note = fn(lines, a, b)
                print('%-22s %s' % (n, note if new else 'SKIP: ' + note))
        return

    for a, b, n in bodies(lines):
        if n != todo:
            continue
        new, note = fn(lines, a, b)
        if new is None:
            sys.exit('%s: %s' % (n, note))
        open(path, 'w').write('\n'.join(lines[:a] + new + lines[b + 1:]))
        print('%s: %s' % (n, note))
        return
    sys.exit('%s: no such function' % todo)


if __name__ == '__main__':
    main()
