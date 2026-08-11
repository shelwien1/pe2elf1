#!/usr/bin/env python3
"""One address, written more than one way.

    python3 tools/unspell.py subs1.hpp

    rc_begin_encode   row[4 * i + 20] = 60      # row is uint8_t *
    rc_begin_decode   row[2 * i + 10] = 60      # row is uint16_t *

Those are the same byte.  Nothing else in `tools/` can see that they are:
`shape.py` counts raw offsets and only the first is one; §2's lens takes the
gcd of a cursor's *constant* subscripts and neither of these has any; the
compiler is happy with both.  So a duplicated address computation survives
every measure this project has unless the two copies happen to be read
together.

The rule is arithmetic rather than pattern.  For each pointer whose element
size is known, every `p[expr]` and `p + expr` is parsed as a sum of `c * v`
terms, each coefficient multiplied by the element size, and the result keyed by
the map from variable to *byte* coefficient.  Two sites with the same key are
the same address; if their source text differs, they are two spellings of it,
and that is what gets reported.

It reports and does not rewrite, because which spelling should win is a
judgement.  Usually it is the one whose pointer type matches the record --
`rc_begin_encode` walked a 508-byte row as bytes and `rc_begin_decode` walked
the identical row as 254 words, and the words were right because every field
in it is sixteen bits.

What it cannot see is a spelling whose terms it cannot parse: anything with a
shift, a call, or a nested subscript in the index is skipped rather than
guessed at, and a single-term index is ignored because `p[i]` and `p + i` are
the same spelling by any account.
"""
import re
import sys
import collections

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

WIDTH = {'uint8_t': 1, 'int8_t': 1, 'char': 1, 'void': 1,
         'uint16_t': 2, 'int16_t': 2,
         'uint32_t': 4, 'int32_t': 4, 'float': 4,
         'uint64_t': 8, 'int64_t': 8, 'double': 8}
# `p[i]` reaches through one indirection; `p[i][j]` and `&p[i]` used as a base
# do not, and a pointer-to-pointer steps four bytes on this target whatever it
# points at.
INDEX = re.compile(r'&?\b([A-Za-z_]\w*)\s*\[([^\[\]]+)\]')
PLUS = re.compile(r'\b([A-Za-z_]\w*)\s*\+\s*([0-9A-Za-z_ *+\-]+?)\s*[);,\]]')


def elem(ty, sizes):
    """Bytes one step of this pointer covers, or None if it is not one."""
    ty = ty.replace(' ', '')
    if not ty.endswith('*'):
        return None
    if ty.count('*') > 1:
        return 4                       # a pointer, on this target
    return WIDTH.get(ty[:-1], sizes.get(ty[:-1]))


def terms(expr):
    """{variable: coefficient} for a sum, with '' the constant; None if not."""
    out = collections.Counter()
    for sign, t in re.findall(r'([+-])\s*([^+-]+)', '+' + expr):
        t, s = t.strip(), -1 if sign == '-' else 1
        if not t:
            return None
        m = re.fullmatch(r'(\d+)\s*\*\s*([A-Za-z_]\w*)', t)
        if m:
            out[m.group(2)] += s * int(m.group(1))
            continue
        m = re.fullmatch(r'([A-Za-z_]\w*)\s*\*\s*(\d+)', t)
        if m:
            out[m.group(1)] += s * int(m.group(2))
            continue
        if re.fullmatch(r'\d+|0[xX][0-9A-Fa-f]+', t):
            out[''] += s * int(t, 0)
            continue
        if re.fullmatch(r'[A-Za-z_]\w*', t):
            out[t] += s
            continue
        return None
    return out


def sizes_of(lines):
    """Every record whose size the file asserts, so a cast can be measured."""
    out = {}
    for m in re.finditer(r'static_assert\(sizeof\((\w+)\)\s*==\s*(\d+)',
                         '\n'.join(lines)):
        out[m.group(1)] = int(m.group(2))
    return out


def survey(lines):
    sizes = sizes_of(lines)
    addrs = collections.defaultdict(set)
    for a, b, nm, sig in structs.bodies(lines):
        types = structs.decl_types(sig, lines, a, b)
        for i in range(a, b + 1):
            c = lines[i].split('//')[0]
            for pat in (INDEX, PLUS):
                for m in pat.finditer(c):
                    e = elem(types.get(m.group(1), ''), sizes)
                    if not e:
                        continue
                    t = terms(m.group(2))
                    # One term is one spelling: `p[i]` and `p + i` say the same
                    # thing and reporting them would be reporting C's grammar.
                    if t is None or len(t) < 2:
                        continue
                    key = (m.group(1),
                           tuple(sorted((k, v * e) for k, v in t.items() if v)))
                    addrs[key].add((nm.lstrip('_'), i + 1,
                                    re.sub(r'\s+', '', m.group(2)),
                                    c.strip()[:88]))
    return {k: v for k, v in addrs.items() if len({s[2] for s in v}) > 1}


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    found = survey(open(path).read().split('\n'))
    for key, sites in sorted(found.items(), key=lambda kv: -len(kv[1])):
        print('%s  %s' % (key[0], dict(key[1])))
        for fn, ln, idx, txt in sorted(sites):
            print('   %-24s %6d  [%s]  %s' % (fn, ln, idx, txt))
    print('%d addresses written more than one way' % len(found))
