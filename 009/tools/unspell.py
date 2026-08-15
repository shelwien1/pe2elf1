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
PLUS = re.compile(r'\b([A-Za-z_]\w*)\s*\+\s*([^;,)]+?)\s*[);,]')
# A term this cannot scale or would evaluate twice.  A shift or a mask is not a
# linear offset, a call is not a value this can key on, and `k++` inside an
# index means the site is not a pure address at all.
BAD = re.compile(r'\+\+|--|\?|<<|>>|[&|^~%/]|\b[A-Za-z_]\w*\s*\(')
# `level_geom[lvl].tbl_base` and `frame.kids_i` are variables as far as an
# address is concerned: one name for one value, whatever it took to reach it.
PATH = re.compile(r'[A-Za-z_]\w*(?:(?:\.|->)[A-Za-z_]\w*|\[[^\[\]]*\])*\Z')


def indices(c):
    """(base, index) for every `name[...]`, with the brackets balanced.

    A regex with `[^\\[\\]]+` for the index cannot see
    `freq[2 * level_geom[lvl].tbl_base + 8]`, and that is the shape §19 is
    about -- so the first version of this tool could not have found the thing
    it was written for.  The subscript is scanned rather than matched.
    """
    out = []
    for m in re.finditer(r'\b([A-Za-z_]\w*)\s*\[', c):
        depth, i = 1, m.end()
        while i < len(c) and depth:
            depth += (c[i] == '[') - (c[i] == ']')
            i += 1
        if not depth:
            out.append((m.group(1), c[m.end():i - 1]))
    return out


def split_terms(e):
    """[(sign, text)] over the top-level `+` and `-` of a sum."""
    out, depth, start, sign, i = [], 0, 0, 1, 0
    e = e.strip()
    if not e:
        return []
    if e[0] in '+-':
        sign, start, i = -1 if e[0] == '-' else 1, 1, 1
    while i < len(e):
        ch = e[i]
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        elif depth == 0 and ch in '+-':
            before = e[:i].rstrip()
            if before and before[-1] not in '*(+-':
                out.append((sign, e[start:i]))
                sign, start = -1 if ch == '-' else 1, i + 1
        i += 1
    out.append((sign, e[start:]))
    return out


def elem(ty, sizes):
    """Bytes one step of this pointer covers, or None if it is not one."""
    ty = ty.replace(' ', '')
    if not ty.endswith('*'):
        return None
    if ty.count('*') > 1:
        return 4                       # a pointer, on this target
    return WIDTH.get(ty[:-1], sizes.get(ty[:-1]))


def terms(expr, scale=1, depth=0):
    """{variable: coefficient} for a sum, with '' the constant; None if not.

    Recursive because a constant distributes: `4 * (span + node)` is the same
    address as `4 * span + 4 * node`, and the two spellings of it in
    `encode_symbol_tree` and `decode_symbol_tree` are exactly the pair this is
    supposed to catch.
    """
    if depth > 4 or BAD.search(expr):
        return None
    out = collections.Counter()
    for sign, t in split_terms(expr):
        t, s = t.strip(), sign * scale
        if not t:
            return None
        m = re.fullmatch(r'\((.*)\)', t)
        if m:
            sub = terms(m.group(1), s, depth + 1)
            if sub is None:
                return None
            out += sub
            continue
        m = (re.fullmatch(r'(\d+|0[xX][0-9A-Fa-f]+)\s*\*\s*(.+)', t)
             or re.fullmatch(r'(.+?)\s*\*\s*(\d+|0[xX][0-9A-Fa-f]+)', t))
        if m:
            k, rest = (m.group(1), m.group(2)) if m.group(1)[0].isdigit() \
                else (m.group(2), m.group(1))
            sub = terms(rest, s * int(k, 0), depth + 1)
            if sub is None:
                return None
            out += sub
            continue
        if re.fullmatch(r'\d+|0[xX][0-9A-Fa-f]+', t):
            out[''] += s * int(t, 0)
            continue
        if PATH.fullmatch(t):
            out[re.sub(r'\s+', '', t)] += s
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


def wrapped(text):
    """True when one pair of parentheses encloses the whole expression."""
    if not (text.startswith('(') and text.endswith(')')):
        return False
    depth = 0
    for i, ch in enumerate(text):
        depth += (ch == '(') - (ch == ')')
        if depth == 0 and i < len(text) - 1:
            return False
    return True


def address(text, types, sizes, depth=0):
    """(base, {var: byte coefficient}, step) for a pointer expression.

    `step` is what a subsequent `+ 1` would add, which is the whole reason a
    cast has to be followed rather than stripped: after `(uint8_t *)` a `+ 4`
    is four bytes and before it, on a `uint16_t *`, it was eight.  Getting that
    wrong would make two different addresses look like one, which is worse than
    finding neither.
    """
    text = text.strip()
    if depth > 6 or not text:
        return None
    while wrapped(text):
        text = text[1:-1].strip()
    # An integer cast on a pointer is Hex-Rays' spelling of a register, not a
    # conversion: `(uint8_t *)((int32_t)(p + 8))` is `p + 8`.
    m = re.fullmatch(r'\(\s*u?int(?:8|16|32|64)_t\s*\)\s*(.+)', text, re.S)
    if m:
        return address(m.group(1), types, sizes, depth + 1)
    m = re.fullmatch(r'\(\s*(\w+)\s*\*+\s*\)\s*(.+)', text, re.S)
    if m:
        inner = address(m.group(2), types, sizes, depth + 1)
        step = WIDTH.get(m.group(1), sizes.get(m.group(1)))
        if inner is None or not step:
            return None
        return inner[0], inner[1], (4 if '**' in m.group(0) else step)
    # `x + e` at the top level, taking the last `+` so the left side is the
    # pointer and the right the offset.
    parts = split_terms(text)
    if len(parts) > 1:
        head = parts[0][1]
        rest = ' + '.join(('-' if s < 0 else '') + t for s, t in parts[1:])
        left = address(head, types, sizes, depth + 1)
        if left is None:
            return None
        off = terms(rest)
        if off is None:
            return None
        out = collections.Counter(left[1])
        for k, v in off.items():
            out[k] += v * left[2]
        return left[0], out, left[2]
    m = re.fullmatch(r'&?\s*([A-Za-z_]\w*)\s*\[(.*)\]', text, re.S)
    if m:
        e = elem(types.get(m.group(1), ''), sizes)
        idx = terms(m.group(2))
        if not e or idx is None:
            return None
        return m.group(1), collections.Counter(
            {k: v * e for k, v in idx.items()}), e
    m = re.fullmatch(r'&?\s*([A-Za-z_]\w*)', text)
    if m:
        e = elem(types.get(m.group(1), ''), sizes)
        if not e:
            return None
        return m.group(1), collections.Counter(), e
    return None


def survey(lines):
    sizes = sizes_of(lines)
    addrs = collections.defaultdict(set)
    for a, b, nm, sig in structs.bodies(lines):
        types = structs.decl_types(sig, lines, a, b)
        for i in range(a, b + 1):
            c = lines[i].split('//')[0]
            got = []
            for base, idx in indices(c):
                e = elem(types.get(base, ''), sizes)
                t = terms(idx)
                if e and t is not None:
                    got.append((base, {k: v * e for k, v in t.items()}, idx))
            # The right-hand side of a store into a pointer, which is where a
            # compound base lives: `(uint16_t *)((uint8_t *)&freq[..] + ..)`.
            m = re.fullmatch(r'\s*(\w+)\s*=\s*(.+);\s*', c)
            if m and elem(types.get(m.group(1), ''), sizes):
                r = address(m.group(2), types, sizes)
                if r:
                    got.append((r[0], r[1], re.sub(r'\s+', '', m.group(2))))
            # The two scans above see one site twice -- `p = &q[i + 1];` is
            # both a subscript and a store -- and a line reported against
            # itself is the tool finding its own duplicate, not the file's.
            here = {}
            for base, bytes_, txt in got:
                # One term is one spelling: `p[i]` and `p + i` say the same
                # thing and reporting them would be reporting C's grammar.
                live = {k: v for k, v in bytes_.items() if v}
                if len(live) < 2:
                    continue
                here.setdefault((base, tuple(sorted(live.items()))), txt)
            for key, txt in here.items():
                addrs[key].add((nm.lstrip('_'), i + 1, txt, c.strip()[:88]))
    return {k: v for k, v in addrs.items()
            if len({s[2] for s in v}) > 1 and len({(s[0], s[1]) for s in v}) > 1}


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    found = survey(open(path).read().split('\n'))
    for key, sites in sorted(found.items(), key=lambda kv: -len(kv[1])):
        print('%s  %s' % (key[0], dict(key[1])))
        for fn, ln, idx, txt in sorted(sites):
            print('   %-24s %6d  [%s]  %s' % (fn, ln, idx, txt))
    print('%d addresses written more than one way' % len(found))
