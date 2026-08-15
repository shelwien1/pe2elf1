#!/usr/bin/env python3
"""Classify `*(T *)&x.m` by the width of the cast against the width of `m`.

    python3 tools/uncastwidth.py subs1.hpp            # the census
    python3 tools/uncastwidth.py subs1.hpp --same     # list the deletable ones
    python3 tools/uncastwidth.py subs1.hpp --wide     # and the real packed stores
    python3 tools/uncastwidth.py subs1.hpp --apply    # delete the same-width ones

Hex-Rays writes a store through `*(T *)&obj->member` whenever MSVC's store did
not have the member's type, and that covers two entirely different things:

  same width    a signedness cast and nothing else.  `*(int32_t *)&this->
                ctr_node = ctx0` on a `uint32_t` member is `this->ctr_node =
                ctx0` with a conversion warning instead of a cast, and the
                warning is the honest form -- REFACTORING10.md section 2.
  wider         a real packed store.  `*(uint32_t *)&rec->match` writes four
                bytes of a `uint8_t[6]` because MSVC merged four byte stores,
                and deleting the cast would write one byte.  These want a named
                union member, one reading job each.

Sorting them by hand is what REFACTORING10.md's first draft did, and it got the
narrow row wrong: its throwaway classifier took `\\*+` and compared
`sizeof(uint8_t)` against the member for `*(uint8_t **)&...`, turning six
pointer stores into "narrow" casts.  Pointer depth is a column here, not a
rounding error.

The width of a member comes from the record it is declared in, resolved through
the local's own declared type -- `img->height` is `BmfImage::height` and not
whatever else in the file is called `height`.  A member this cannot type is
reported as `untyped` rather than guessed at.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

FLOAT = {'float', 'double'}

SIZE = {'uint8_t': 1, 'int8_t': 1, 'char': 1, 'bool': 1,
        'uint16_t': 2, 'int16_t': 2, 'uint32_t': 4, 'int32_t': 4, 'float': 4,
        'uint64_t': 8, 'int64_t': 8, 'double': 8}

CAST = re.compile(r'\*\(\s*([A-Za-z_]\w*)\s*(\*+)\s*\)\s*&\s*'
                  r'((?:[A-Za-z_]\w*\s*(?:->|\.)\s*)*)([A-Za-z_]\w*)'
                  r'\s*(?P<sub>\[[^\]]*\])?')


def records(lines):
    """{record: {member: type}}, for resolving `obj->m` through `obj`'s type."""
    out, cur, depth = collections.defaultdict(dict), None, 0
    for l in lines:
        c = l.split('//')[0]
        m = re.match(r'\s*struct\s+([A-Za-z_]\w*)\s*\{', c)
        if m and depth == 0:
            cur, depth = m.group(1), c.count('{') - c.count('}')
            continue
        if not cur:
            continue
        depth += c.count('{') - c.count('}')
        if depth <= 0:
            cur = None
            continue
        d = re.match(r'\s*((?:u?int(?:8|16|32|64)_t|float|double|char|bool))\s+'
                     r'(\*?)\s*(\w+)\s*(?:\[[^\]]*\])?\s*;', c)
        if d:
            # Keep the star.  Dropping it reported `uint8_t *run_bucket` as a
            # one-byte member, so a 32-bit cast on it looked like a four-byte
            # packed store when it is a pointer read through an integer -- three
            # rows, all of them in the `wide` column and none of them belonging
            # there.  A member that holds an address is its own thing.
            out[cur][d.group(3)] = d.group(1) + d.group(2)
    return out


def anywhere(lines):
    """{member: type} for names whose width is the same in every record."""
    seen = collections.defaultdict(set)
    for rec in records(lines).values():
        for n, t in rec.items():
            seen[n].add(t)
    return {n: t.pop() for n, t in seen.items() if len(t) == 1}


def survey(lines):
    """(line, kind, cast, member type, text) for every `*(T *)&…` on a member."""
    rec, any_ = records(lines), anywhere(lines)
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        types = structs.decl_types(sig, lines, a, b)
        # `inline T Class::method(...)` -- what `this` is, which is how a third
        # of these resolve and none of them did before.
        me = re.search(r'\b([A-Za-z_]\w*)::', sig)
        me = me.group(1) if me else None
        for i in range(a, b + 1):
            c = lines[i].split('//')[0]
            for m in CAST.finditer(c):
                ct, stars, chain, mem = m.group(1, 2, 3, 4)
                if len(stars) > 1:
                    out.append((i + 1, 'pointer', ct + stars, None, c.strip()))
                    continue
                # No `->` or `.` in front: the target is a plain name, so this
                # is `*(T *)&buf[i]` on an array and not a store to a member at
                # all.  Reading four bytes at a byte offset of a palette buffer
                # is the program, not a decompiler artefact, and lumping it in
                # with the packed stores is what made the first census of this
                # report 43 untyped rows it had quietly mistyped.
                if not chain:
                    out.append((i + 1, 'array', ct, None, c.strip()))
                    continue
                # the last name before the member is the object; its declared
                # type names the record the member lives in
                own = re.findall(r'([A-Za-z_]\w*)\s*(?:->|\.)', chain)
                mt = None
                if own and own[-1] == 'this' and me:
                    mt = rec.get(me, {}).get(mem)
                if mt is None and own:
                    mt = rec.get((types.get(own[-1]) or '').rstrip('*'),
                                 {}).get(mem)
                # A frame member.  Frames are anonymous structs declared inside
                # a body, so `records()` above never sees them -- but
                # `structs.decl_types` reports their members as if they were
                # locals, which is exactly the answer wanted here.  Gated on the
                # chain starting at `frame` so that a local sharing a name
                # with some record's member cannot answer for it.
                if mt is None and own and own[0] == 'frame':
                    mt = types.get(mem)
                if mt is None:
                    mt = any_.get(mem)
                # A member reached *with a subscript* is an array read,
                # whatever `decl_types` says its type is -- and what it says
                # is `uint8_t *` for every one of them, because it decays
                # arrays.  Eighteen `*(uint32_t *)&frame.buf[8 * node]`
                # rows sat in the `ptr-member` column for that reason: a
                # 32-bit word read out of a byte buffer, filed under "a
                # member that holds an address".
                if m.group('sub'):
                    out.append((i + 1, 'array', ct, mt, c.strip()))
                    continue
                if mt and mt.endswith('*'):
                    out.append((i + 1, 'ptr-member', ct, mt, c.strip()))
                    continue
                if mt is None or ct not in SIZE or mt not in SIZE:
                    out.append((i + 1, 'untyped', ct, mt, c.strip()))
                elif (ct in FLOAT) != (mt in FLOAT):
                    # Same width, different *kind*.  `*(double *)&frame.q0`
                    # on an `int64_t` member is not a signedness cast -- it
                    # stores the double's bits in an integer slot, and deleting
                    # it turns a reinterpretation into a conversion.
                    #
                    # This is the mistake the tool exists to prevent and it made
                    # it anyway: the first version compared widths alone, called
                    # six of these `same`, and deleted them.  The gate passed --
                    # fifteen streams byte-identical -- because the solve they
                    # are part of does not reach the chosen filter for any image
                    # in the corpus, which is exactly why a rule may not rely on
                    # the gate to catch a wrong rewrite.
                    out.append((i + 1, 'reinterpret', ct, mt, c.strip()))
                elif SIZE[ct] == SIZE[mt]:
                    out.append((i + 1, 'same', ct, mt, c.strip()))
                elif SIZE[ct] > SIZE[mt]:
                    out.append((i + 1, 'wide', ct, mt, c.strip()))
                else:
                    out.append((i + 1, 'narrow', ct, mt, c.strip()))
    return out


def apply(lines, rows):
    """Drop the cast on the same-width rows.  `*(T *)&x.m = e` becomes `x.m = e`.

    Only where the cast is the whole left-hand side of an assignment: a
    same-width cast inside an expression is still a read of the same bits, but
    removing it there can change what an enclosing operator does, and this rule
    does not need the argument.
    """
    done = 0
    for ln, kind, ct, mt, _ in rows:
        if kind != 'same':
            continue
        c = lines[ln - 1]
        new = re.sub(r'\*\(\s*%s\s*\*\s*\)\s*&\s*' % re.escape(ct), '', c, count=1)
        if new != c and re.search(r'=[^=]', new):
            lines[ln - 1] = new
            done += 1
    return done


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    p = sys.argv[1]
    lines = open(p).read().split('\n')
    rows = survey(lines)
    counts = collections.Counter(k for _, k, _, _, _ in rows)
    for want in ('--same', '--wide', '--narrow', '--untyped', '--pointer',
                 '--array', '--reinterpret', '--ptr-member'):
        if want in sys.argv:
            k = want[2:]
            for ln, kind, ct, mt, text in rows:
                if kind == k:
                    print('%6d  %-9s as %-9s %s' % (ln, mt or '?', ct, text[:72]))
            print('%d %s' % (counts[k], k))
            sys.exit(0)
    if '--apply' in sys.argv:
        n = apply(lines, rows)
        open(p, 'w').write('\n'.join(lines))
        print('%d same-width casts deleted' % n)
        sys.exit(0)
    for k in ('same', 'wide', 'narrow', 'reinterpret', 'ptr-member', 'pointer',
              'array', 'untyped'):
        print('%-9s %d' % (k, counts[k]))
    print('%d casts of the form *(T *)&… on a member' % len(rows))
