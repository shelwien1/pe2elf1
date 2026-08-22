#!/usr/bin/env python3
"""Enumerate the encode/decode pairs in the program, and report the ones no
table has taken a decision about.

    python3 tools/pairnames.py bmf.cpp
    python3 tools/pairnames.py bmf.cpp --all   # print every pair, decided or not
    python3 tools/pairnames.py --selftest

`pairshare.py` re-measures the rows the table in `bmf.cpp` already has.  That
keeps a written number from rotting, and it is blind to the pair that was never
written down: it reads the table as its input, so a pair nobody named is a pair
nobody measured.  Two got through that gap.

`code_colour_plane` was the first -- `interleave_plane` and `colour_transform`,
an encode/decode pair under two names that do not look like one, so no share
was ever taken and a comment asserted from the armchair that the two loops "are
inverses and have nothing else in common".  They were the same thirty-two bits
written four ways.

`write_plane_descs` / `read_plane_descs` was the second, and it is the reason
this exists.  Fourteen lines against eighteen, field for field the same format
-- same loop, same two `continue` guards, same widths in the same order, same
bias -- and a measured share of two lines out of twenty-four.  **A share cannot
see a pair whose every line is spelled backwards**: `pack_bits(w + 64, 8)` and
`unpack_bits(8) - 64` have no text in common.  Ranking pairs by share and
looking at the top would never have reached it.

So this does not rank by share.  It enumerates by *name*, from the affix
conventions the program actually uses, and asks one question of each pair: has
anybody decided about this one?  A pair is decided when it is two thin wrappers
around one `template<int32_t f_DEC>`, or when the declined table names it.
Everything else is reported, whatever it measures -- the share is printed as
information, never as a filter.
"""
import difflib
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# The affix pairs this program spells its two directions with.  A rule fires
# only on a token boundary, so `read_bmp`/`write_bmp` pairs up and `spread` is
# not a `read`.
RULES = [
    ('encode', 'decode'),
    ('compress', 'expand'),
    ('compress', 'decompress'),
    ('reduce', 'expand'),
    ('write', 'read'),
    ('pack', 'unpack'),
    ('model', 'unmodel'),
    ('predict', 'unpredict'),
    ('to_memory', 'from_memory'),
]

# `BmfStream`'s own `read`/`write` are a stream's two ends, not a coder's two
# directions.  The same structural exemption `covered.py` carries, and for the
# same reason: this file is plumbing under the codec rather than part of it.
PLUMBING = {'BmfStream', 'BmfFile'}

# A row of the declined table.  Either half may carry the class that owns it,
# because two of these pairs are names that exist in more than one class.
ROW = re.compile(r'^//\s+(?:(\w+)::)?(\w+)\s*/\s*(?:(\w+)::)?(\w+)\s+'
                 r'(\d+) of (\d+) \((\d+)%\)\s*$')
SCAFFOLD = re.compile(r'^(\{|\}|\};|\}\s*else\s*\{?|else|else\s*\{|break;|'
                      r'continue;|return;|do|do\s*\{|\)\s*;?|#.*)$')
# `foo<0>(` or `foo<1>(` -- the call a merged pair's wrapper is made of
INSTANCE = re.compile(r'\b(\w+)\s*<\s*([01])\s*>\s*\(')


def row(line):
    """(a, b, shared, total, pct) for a declined-table row, or None.

    Names come back bare -- the `Class::` a row may carry says which of two
    same-named definitions is meant, and the lookup that resolves it wants the
    class separately, so `row_class` is where it goes.
    """
    m = ROW.match(line)
    if not m:
        return None
    return (m.group(2), m.group(4), int(m.group(5)), int(m.group(6)),
            int(m.group(7)))


def row_class(line):
    """The two `Class::` qualifiers of a row, each '' when it carries none."""
    m = ROW.match(line)
    return (m.group(1) or '', m.group(3) or '') if m else None


def partners(name):
    """Every name the affix rules turn `name` into."""
    out = set()
    for a, b in RULES:
        for frm, to in ((a, b), (b, a)):
            for m in re.finditer(re.escape(frm), name):
                i, j = m.start(), m.end()
                if (i == 0 or name[i - 1] == '_') and \
                   (j == len(name) or name[j] == '_'):
                    cand = name[:i] + to + name[j:]
                    if cand != name:
                        out.add(cand)
    return out


def norm(rows):
    return [re.sub(r'\s+', ' ', l.split('//')[0].strip())
            for l in rows if l.split('//')[0].strip()]


def substantive(x):
    return [l for l in x if not SCAFFOLD.match(l)]


def lcs(x, y):
    sm = difflib.SequenceMatcher(None, x, y, autojunk=False)
    return sum(n for _i, _j, n in sm.get_matching_blocks())


def wraps(body):
    """The (template, direction) a one-call wrapper forwards to, or None.

    A wrapper is a body whose only statement is a call to `name<0>` or
    `name<1>`.  Anything longer has a body of its own and is not a pair half
    that has already been merged.  The direction comes back too, so that two
    wrappers calling the *same* instantiation are not read as a merged pair.
    """
    stmts = [l for l in norm(body)[1:] if not SCAFFOLD.match(l)]
    if len(stmts) != 1:
        return None
    m = INSTANCE.search(stmts[0])
    return (m.group(1), m.group(2)) if m else None


def enclosing(lines, i):
    """The class a definition at line `i` is a member of, or ''.

    Walks back from the line *above* the definition -- the definition's own
    line carries its opening brace, and counting that would put the walk one
    level too deep before it starts.  The first line that leaves an unmatched
    `{` behind it is the enclosing scope.
    """
    depth = 0
    for j in range(i - 1, -1, -1):
        depth += lines[j].count('}') - lines[j].count('{')
        if depth < 0:
            m = re.match(r'\s*(?:struct|class)\s+(\w+)', lines[j])
            return m.group(1) if m else ''
    return ''


def decided(da, db, declined):
    """Why this pair needs no report, or None if it needs one."""
    if (da.name, db.name) in declined or (db.name, da.name) in declined:
        return 'on the declined table'
    wa, wb = wraps(da.body), wraps(db.body)
    if wa and wb and wa[0] == wb[0] and wa[1] != wb[1]:
        return 'one template, %s' % wa[0]
    if da.cls in PLUMBING or db.cls in PLUMBING:
        return 'stream plumbing, not a coder'
    return None


class Def:
    """One definition, with the class it belongs to.

    **The class is not decoration.**  `encode_sample` and `decode_sample` are
    each defined twice -- once in `AltP1Block` and once in `AltP2Block` -- and
    keying bodies by the bare name kept whichever came first and measured a
    pair that does not exist.  `pairshare.py` reads bodies the same way and has
    the same blind spot; it does not matter there because no row it checks is
    an overloaded name, which is luck rather than design.
    """

    def __init__(self, name, cls, body):
        self.name, self.cls, self.body = name, cls, body

    def label(self):
        return '%s::%s' % (self.cls, self.name) if self.cls else self.name


def collect(path):
    lines, _origin = structs.splice(path)
    defs = {}
    for a, b, n, sig, _d in structs.defs(lines):
        m = re.search(r'\b(\w+)::' + re.escape(n) + r'\b', sig)
        cls = m.group(1) if m else enclosing(lines, a)
        defs.setdefault(n, []).append(Def(n, cls, lines[a:b + 1]))
    text = open(path).read()
    declined = {(r[0], r[1])
                for r in (row(l) for l in text.split('\n')) if r}
    return defs, declined


def pairings(defs, n, p):
    """Which definition of `n` goes with which of `p`.

    Within a class first -- two `AltP2Block` halves are a pair and an
    `AltP1Block` half is not their partner.  Only when no class has both is a
    cross-class pairing taken, which is what `BMFState::reduce_alphabet` and
    `ModelBlock::expand_alphabet` are.
    """
    same = [(a, b) for a in defs[n] for b in defs[p] if a.cls == b.cls]
    return same if same else [(a, b) for a in defs[n] for b in defs[p]]


def main():
    argv = [a for a in sys.argv[1:] if not a.startswith('-')]
    path = argv[0] if argv else 'bmf.cpp'
    defs, declined = collect(path)

    seen, rows = set(), []
    for n in sorted(defs):
        for p in partners(n):
            if p not in defs or (p, n) in seen:
                continue
            seen.add((n, p))
            for da, db in pairings(defs, n, p):
                why = decided(da, db, declined)
                sx = substantive(norm(da.body))
                sy = substantive(norm(db.body))
                rows.append((da.label(), db.label(), why,
                             lcs(sx, sy), len(sx) + len(sy)))

    open_rows = [r for r in rows if r[2] is None]
    for a, b, why, s, t in sorted(rows, key=lambda r: (r[2] is None, r[0])):
        if why is None:
            print('  %-28s %-28s  %2d of %3d substantive -- no table has '
                  'decided about this pair' % (a, b, s, t))
        elif '--all' in sys.argv:
            print('  %-28s %-28s  %s' % (a, b, why))
    # The finding first.  `sweep.sh` reads the leading number of this line as
    # the count, and it says why: a tool that leads with a census passes on a
    # zero that is further along the sentence.
    print('%d encode/decode pairs nothing has decided about, of %d'
          % (len(open_rows), len(rows)))


def selftest():
    ok = True

    def check(what, got, want):
        nonlocal ok
        if got != want:
            ok = False
            print('  FAIL %s: %r != %r' % (what, got, want))

    # the affix rules, including the boundary the naive substring gets wrong
    check('encode', partners('encode_symbol'), {'decode_symbol'})
    check('read_bmp', 'write_bmp' in partners('read_bmp'), True)
    check('unmodel', 'model_plane' in partners('unmodel_plane'), True)
    # `spread` must not be `spwrite`, and `decode` inside a longer token stays
    check('no infix', partners('spread'), set())
    check('threaded', partners('threaded'), set())
    # a wrapper is one call to name<0>/name<1>
    check('wrapper', wraps(['void f() {', '  code_x<0>(a);', '}']),
          ('code_x', '0'))
    check('wrapper 1', wraps(['void f() {', '  return code_x<1>(a);', '}']),
          ('code_x', '1'))
    check('two calls', wraps(['void f() {', '  a();', '  code_x<0>();', '}']),
          None)
    check('no instance', wraps(['void f() {', '  plain(a);', '}']), None)
    # comments and blank lines do not make a body non-trivial
    check('commented', wraps(['void f() {', '  // why', '', '  code_x<1>(a);',
                              '}']), ('code_x', '1'))
    # two wrappers on the *same* instantiation are not a merged pair
    same = ['void f() {', '  code_x<0>(a);', '}']
    check('same direction',
          decided(Def('a', '', same), Def('b', '', same), set()), None)
    check('opposite direction',
          decided(Def('a', '', same),
                  Def('b', '', ['void g() {', '  code_x<1>(a);', '}']), set()),
          'one template, code_x')
    # a share is reported, never used to decide
    check('lcs', lcs(['a', 'b', 'c'], ['a', 'x', 'c']), 2)
    check('scaffold', substantive(['{', 'a;', '}', 'break;']), ['a;'])
    # the declined-table row shape pairshare.py also parses
    check('row', row('//   code_pixel / decode_pixel           13 of 211 (6%)'),
          ('code_pixel', 'decode_pixel', 13, 211, 6))
    check('row plain class', row_class(
        '//   code_pixel / decode_pixel                 13 of 211 (6%)'),
        ('', ''))
    check('row qualified', row(
        '//   AltP2Block::encode_sample / AltP2Block::decode_sample  2 of 20 (10%)'),
        ('encode_sample', 'decode_sample', 2, 20, 10))
    check('row qualified class', row_class(
        '//   AltP2Block::encode_sample / AltP2Block::decode_sample  2 of 20 (10%)'),
        ('AltP2Block', 'AltP2Block'))
    check('not a row', row('// just prose about 3 of 4 (75%) things'), None)
    check('enclosing', enclosing(['struct BmfStream {', '  void read() {',
                                  '  }', '};'], 1), 'BmfStream')
    check('enclosing free', enclosing(['void read() {', '}'], 0), '')
    # the overload the bare-name version measured wrong: two classes, one name
    d = {'encode_sample': [Def('encode_sample', 'AltP1Block', []),
                           Def('encode_sample', 'AltP2Block', [])],
         'decode_sample': [Def('decode_sample', 'AltP1Block', []),
                           Def('decode_sample', 'AltP2Block', [])]}
    got = [(a.cls, b.cls) for a, b in pairings(d, 'encode_sample',
                                               'decode_sample')]
    check('within class', sorted(got),
          [('AltP1Block', 'AltP1Block'), ('AltP2Block', 'AltP2Block')])
    # and the cross-class pairing, when no class holds both halves
    d2 = {'reduce_alphabet': [Def('reduce_alphabet', 'BMFState', [])],
          'expand_alphabet': [Def('expand_alphabet', 'ModelBlock', [])]}
    check('across classes',
          [(a.cls, b.cls) for a, b in pairings(d2, 'reduce_alphabet',
                                               'expand_alphabet')],
          [('BMFState', 'ModelBlock')])
    print('pairnames.py selftest: %s' % ('ok' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(selftest() if '--selftest' in sys.argv else main())
