#!/usr/bin/env python3
"""Give the p2 counter table's cursors the record type they point at.

    python3 tools/unp2.py subs1.hpp --list
    python3 tools/unp2.py subs1.hpp Obj102
    python3 tools/unp2.py subs1.hpp --all

REFACTORING5.md §3.1.  Round four named the table -- `Obj11::f284712` is
`P2Count[163840]`, four bytes a record -- but the model reads it through 30
`ObjN` structs that are `reinterpret_cast` with a name:

    v540 = (Obj102 *)(&v90[4 * IDX + 284712]);
    v323 = *((int16_t *)v540 + 1);
    v324 = v321 - ((v323 + (1 << ((v540->f0 + 31) & 31))) >> (v540->f0 & 31));
    *((uint16_t *)v540 + 1) = ...;

`f0` is `P2Count::b0` and the `+ 1` int16 beside it is `P2Count::w2`, so all
three lines are one record and the struct says nothing the record does not.

Every use of a cursor is classified by the *byte offset* it reaches, which is
the only reading that survives the retype: `ObjN` is one to eight bytes wide
and `P2Count` is four, so `p - 4` means something different afterwards and a
shape the tool does not recognise has to stop it rather than be guessed at.
A cursor with even one unclassified use keeps its struct, which is what keeps
`alt_p1_model`'s cursors -- same struct names, a different table, records
sixteen bytes wide -- out of this.

The signed/unsigned pair on `w2` is the counter's own idiom and is kept: the
read is `int16_t` because the count is a difference, the write is `uint16_t`
because it wraps.  Round four's `P2Count` already spells it that way.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import merge                                                      # noqa: E402
import structs                                                    # noqa: E402

RECORD = 4
RECNAME = 'P2Count' 
VIEW = re.compile(r'\((Obj\d+) \*\)\(\s*(?:\((?:u?int32_t)\))?\s*&?\w+\[4 \* ')
DECL = re.compile(r'\b(Obj\d+) \*(\w+)\b')
SIGNED = {'char': True, 'int8_t': True, 'int16_t': True, 'int32_t': True,
          'uint8_t': False, 'uint16_t': False, 'uint32_t': False}


def views(src):
    """The `ObjN` names a `4 * index + 284712` cast produces."""
    return sorted(set(VIEW.findall(src)), key=lambda n: int(n[3:]))


def field(off, ty):
    """`P2Count` expression for `off` bytes read as `ty`, or None."""
    if RECORD != 4:
        return lane(off, ty)
    w = merge.width(ty)
    signed = SIGNED.get(re.sub(r'^(?:const|volatile) ', '', ty.strip()))
    if w is None or signed is None:
        return None
    rec, rem = divmod(off, RECORD)
    def base(m):
        return '%%s[%d].%s' % (rec, m) if rec else '%%s->%s' % m
    if rem == 0 and w == 1:
        return base('b0') if signed else '*(uint8_t *)&' + base('b0')
    if rem == 1 and w == 1:
        return base('b1') if not signed else '*(int8_t *)&' + base('b1')
    if rem == 2 and w == 2:
        return base('w2') if signed else '*(uint16_t *)&' + base('w2')
    return None


def lane(off, ty):
    """`P2Ctx` expression for `off` bytes read as `ty`, or None.

    The record is nine `int16_t` and the fields have no names, so the record
    index and the lane are what a site can say: `p[2].lane[0]` where it said
    `p->f36`, and `p[-1].lane[1]` where it said `*((int16_t *)p - 8)`.
    """
    w = merge.width(ty)
    if w is None:
        return None
    rec, rem = divmod(off, RECORD)
    if w == 2 and rem % 2 == 0:
        at = '%%s[%d].lane[%d]' % (rec, rem // 2) if rec else '%%s->lane[%d]' % (rem // 2)
        return at if SIGNED.get(ty.strip()) else '*(%s *)&' % ty.strip() + at
    if w == 1:
        at = '%%s[%d].lane[%d]' % (rec, rem // 2) if rec else '%%s->lane[%d]' % (rem // 2)
        return '*((%s *)&%s + %d)' % (ty.strip(), at, rem % 2)
    return None


def uses(lines, span, var, ty, members, debug=None):
    """[(line, span, replacement)] for every use, or None if one is unreadable.

    The forms are Hex-Rays': a member of the view struct, a scaled cast off the
    cursor, a byte offset off `(uintptr_t)` of it, and the cursor itself as a
    `void *` for `__builtin_prefetch`.

    `span` is the declaring function's line range and is not optional: `v110`
    is an `Obj15 *` in one body, a `uint32_t` in another and an `int16_t
    (*)[8]` in a third, so a file-wide sweep on the name alone rewrites three
    unrelated variables.
    """
    v = re.escape(var)
    forms = [
        (re.compile(r'\*\(\((\w+) \*\)%s ([-+]) (\d+)\)' % v), 'scaled'),
        (re.compile(r'\*\((\w+) \*\)\(\(uintptr_t\)%s ([-+]) (\d+)\)' % v), 'byte'),
        (re.compile(r'\*\((\w+) \*\)%s\b(?![.\-+\w])' % v), 'zero'),
        (re.compile(r'\b%s->(\w+)' % v), 'member'),
    ]
    out, ok = [], True
    for i in range(span[0], span[1] + 1):
        line = lines[i]
        if not re.search(r'(?<![\w.])%s\b' % v, line.split('//')[0]):
            continue
        code = line.split('//')[0]
        hit = []
        for rx, kind in forms:
            for m in rx.finditer(code):
                if kind == 'member':
                    f = members.get(m.group(1))
                    if f is None:
                        ok = False
                        if debug is not None:
                            debug.append((i, 'member %s' % m.group(1)))
                        continue
                    off, aty = f
                else:
                    aty = m.group(1)
                    w = merge.width(aty)
                    if w is None:
                        ok = False
                        continue
                    step = 1 if kind == 'byte' else w
                    off = 0 if kind == 'zero' else \
                        step * int(m.group(3)) * (1 if m.group(2) == '+' else -1)
                expr = field(off, aty)
                if expr is None:
                    ok = False
                    if debug is not None:
                        debug.append((i, '%+d as %s' % (off, aty)))
                    continue
                hit.append((m.span(), expr % var))
        # Whatever the forms did not cover has to be a use the retype leaves
        # alone: the declaration, an assignment from a cast, a cast of the
        # cursor itself, or a `void *` argument.
        rest = code
        for (a, b), _ in sorted(hit, reverse=True):
            rest = rest[:a] + ' ' * (b - a) + rest[b:]
        for m in re.finditer(r'(?<![\w.])%s\b' % v, rest):
            j, k = m.span()
            before, after = rest[:j].rstrip(), rest[k:].lstrip()
            if re.search(r'Obj\d+ \*$', before):
                continue                                  # declaration
            if after.startswith('=') and not after.startswith('=='):
                continue                                  # assigned to
            if after.startswith(')') and before[-1:] in '()':
                continue                    # (Obj46 *)(v551), (Obj40 *)((const char *)v558)
            if before.endswith('(') and after.startswith(','):
                continue                                  # prefetch(v, 0, 1)
            ok = False
            if debug is not None:
                debug.append((i, code.strip()))
        if hit:
            out.append((i, hit))
    return out if ok else None


def declaration(lines, ty):
    """(first, last) line of `struct ty`, its header comment and its assert."""
    for i, line in enumerate(lines):
        if line != 'struct %s {' % ty:
            continue
        a = i
        while a and lines[a - 1].startswith('//'):
            a -= 1
        b = i
        while b < len(lines) and not lines[b].startswith('};'):
            b += 1
        if lines[b + 1:b + 2] and lines[b + 1].startswith('static_assert('):
            while not lines[b].rstrip().endswith(');'):
                b += 1
        return a, b
    return None


def members(src, name):
    """{fNN: (offset, type)} for a view struct."""
    ms, _ = merge.parse(src, name)
    return None if ms is None else {f['name']: (f['off'], f['ty']) for f in ms}


def cursors(src, lines, want, debug=None, only=None):
    """{(function, var): (view, edits)} for every cursor the retype can read."""
    out = {}
    for a, b, fn, _ in structs.bodies(lines):
        if only and fn.lstrip('_') != only:
            continue
        seen = {}
        for i in range(a, b + 1):
            code = lines[i].split('//')[0]
            for m in DECL.finditer(code):
                # `Obj46 *a, *b;` would leave `b` behind with a type that no
                # longer exists, so a shared declaration is not a cursor here.
                if ',' not in code:
                    seen.setdefault(m.group(2), m.group(1))
        for var, ty in seen.items():
            if ty not in want:
                continue
            ms = members(src, ty)
            if ms is None:
                continue
            d = [] if debug is not None else None
            u = uses(lines, (a, b), var, ty, ms, d)
            if u is not None:
                out[(fn, var)] = (ty, u)
            elif debug is not None:
                debug.append((fn, var, ty, d))
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    src = open(path).read()
    lines = src.split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    global RECORD, RECNAME
    only = None
    for x in sys.argv:
        if x.startswith('--in='):
            only = x[5:]
    if '--ctx' in sys.argv:
        # The p2 context table: 18-byte records, rows 144 bytes apart, which
        # `algorithm_v2.md` §9 established from 56 copy sites.  The views of it
        # are not produced by one expression the way §3.1's were, so the
        # candidate set is every `ObjN` and the per-cursor read does the
        # filtering -- a cursor whose uses do not all land on a lane keeps its
        # struct.
        RECORD, RECNAME = 18, 'P2Ctx'
        want = set(args) if args else set(re.findall(r'^struct (Obj\d+) \{', src, re.M))
    else:
        want = set(args) if args else set(views(src))
    dbg = [] if '--why' in sys.argv else None
    found = cursors(src, lines, want, dbg, only)

    if dbg is not None:
        for fn, var, ty, d in dbg:
            print('%s %s (%s) keeps its struct' % (fn.lstrip('_'), var, ty))
            for i, why in d[:6]:
                print('  %5d %s' % (i + 1, why[:100]))
        return 0

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        by = collections.defaultdict(list)
        for (fn, var), (ty, u) in found.items():
            by[ty].append(sum(len(h) for _, h in u))
        for ty in sorted(want, key=lambda n: int(n[3:])):
            got = by.get(ty, [])
            print('%-8s %2d of %2d cursors, %3d accesses'
                  % (ty, len(got),
                     sum(1 for line in lines for m in DECL.finditer(line) if m.group(1) == ty),
                     sum(got)))
        print('%d cursors become P2Count *, %d accesses become records'
              % (len(found), sum(sum(g) for g in by.values())))
        return 0

    edits = collections.defaultdict(list)
    for (fn, var), (ty, u) in found.items():
        for i, hit in u:
            edits[i] += hit
    for i, hit in edits.items():
        code, sep, com = lines[i].partition('//')
        for (a, b), expr in sorted(hit, reverse=True):
            code = code[:a] + expr + code[b:]
        lines[i] = code + sep + com

    # The declaration and the casts that produce the cursor, inside the
    # declaring body only -- the same name in another function is another
    # variable and usually another type.
    body = {fn: (a, b) for a, b, fn, _ in structs.bodies(lines)}
    for (fn, var), _ in found.items():
        v = re.escape(var)
        a, b = body[fn]
        for i in range(a, b + 1):
            s = lines[i]
            s = re.sub(r'\bObj\d+ (\*%s\b)' % v, RECNAME + r' \1', s)
            s = re.sub(r'(?<![\w.])(%s = )\(Obj\d+ \*\)' % v, r'\1(' + RECNAME + ' *)', s)
            s = re.sub(r'\(Obj\d+ \*\)\(\s*(?:\((?:const char \*|int32_t)\))?\s*(%s)\s*\)' % v,
                       '(' + RECNAME + ' *)(\\1)', s)
            lines[i] = s
    src = '\n'.join(lines)

    # A struct nothing names any more goes, and so do the two blocks that
    # exist only to describe it: the `// ObjN -- recovered from ...` header
    # `structs.py` writes above it and the layout `static_assert` below.
    # Line ranges rather than one regex over the whole file -- the pattern that
    # spanned all three blocks backtracked for minutes on a 19 000-line source.
    lines = src.split('\n')
    gone, drop = 0, set()
    for ty in sorted(want):
        span = declaration(lines, ty)
        if span is None:
            continue
        if any(re.search(r'\b%s\b' % ty, lines[i])
               for i in range(len(lines)) if not span[0] <= i <= span[1]):
            continue                                    # still named somewhere
        drop |= set(range(span[0], span[1] + 1))
        gone += 1
    src = '\n'.join(x for i, x in enumerate(lines) if i not in drop)
    open(path, 'w').write(src)
    print('%d cursors retyped, %d accesses, %d structs deleted'
          % (len(found), sum(len(h) for hit in edits.values() for h in [hit]), gone))
    return 0


if __name__ == '__main__':
    sys.exit(main())
