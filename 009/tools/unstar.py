#!/usr/bin/env python3
"""A dereference that would read as a subscript, beside subscripts that do.

    python3 tools/unstar.py bmf.cpp             # report
    python3 tools/unstar.py sym_code.inc --list # and every one
    python3 tools/unstar.py sym_code.inc --all  # rewrite them

    *(freq+3)          ->  freq[3]
    *freq              ->  freq[0]      (in a body that also says `freq[j]`)
    *slot              ->  *slot        (in a body that says `++slot`)

Hex-Rays writes an array access as a pointer arithmetic expression whenever the
original's addressing mode was a base plus a constant, so one table ends up
reached three ways in one function: `f0[j]` in a loop, `*(freq+3)` in a rescale
written out longhand, and `*freq` for element zero.  `code_symbol_tree` has all
three over forty lines, and `model.inc` and `sym_list.inc` each carry a line
that spells the same base both ways at once -- `*slot = slot[-1];`.

**The `*(p + K)` half is unconditional.**  `p[K]` is that expression's
definition; there is no body in which one is right and the other is not.

**The `*p` half needs the body**, and the distinction is whether `p` walks.  A
cursor advanced with `++p` and read with `*p` is idiomatic and reads worse as
`p[0]` -- `*slot` inside `code_symbol_tree`'s decode loop is the counter the
walk has arrived at, not element zero of anything.  So `*p` is reported only
when the body **never** moves `p`: no `++p`, `p++`, `--p`, `p--`, `p +=`,
`p -=`, and no assignment back to `p` -- and when the same body reaches `p`
through a subscript somewhere else, which is what says the reader is already
being asked to think of it as an array.

That second condition is deliberately narrow.  `*p` on a pointer nothing
subscripts is a dereference and reads correctly as one; turning it into `p[0]`
everywhere would be a spelling preference and this is not that.  What it reports
is one base spelled two ways in one body, which is a thing the reader has to
notice and reconcile.

**It does not touch a declaration.**  `uint16_t *slot;`, `P2Count *direct[11]`,
`float (*nb)[4]` and `P2Freq *frec = &freq[ctx];` are stars that bind types, not
operators.  Three shapes, and all three were found by reading the first run's
findings one at a time rather than by counting them -- see `binds_type`.

**Run it until it says zero.**  The two halves feed each other: `code_symbol_tree`
reached `freq` only as `*freq` and `*(freq+K)`, so on the first pass the second
condition was false -- nothing subscripted `freq` -- and the four `*freq` were
correctly left.  Rewriting the twenty-five offsets *created* the subscripts, and
the second pass reported them.  Two passes and a zero on this tree.

**One it leaves, and it is worth naming so the next reader does not think it
was missed.**  `model.inc` and `sym_list.inc` each carry the same line -- one
base spelled both ways at once, `*slot = slot[-1];` and `*list = list[-1];` --
and the rule splits them.  `slot` in `init_tables` is `&ent[n_live]` and never
moves, so it is `slot[0] = slot[-1]` now.  `list` in `add_weight` has a
`list += n_live;` in it, so `*list` there means "the record we arrived at" and
the star stays.

The first reading of this pair called both of them cursors and left both, on a
body span that ran far past the method; the second called both findings, on a
grep that truncated before the line that moves `list`.  The rule got it right
both times and the reader did not, which is the argument for the rule.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# `*(name + offset)` -- the unconditional half.  The offset is not required to
# be a literal: `update_binary_pair` reaches one base three ways in seven lines
# -- `*_this`, `_this[1]` and `*(_this+lvl+2)` -- and a rule that only took
# constants would have left the third.  No parentheses inside it, which keeps
# the rewrite textual and keeps a cast (`*(uint32_t*)&x`) out, since a cast has
# no `+` after the name.
OFFSET = re.compile(r'(?<![\w\)\]])\*\(\s*([A-Za-z_]\w*)\s*\+\s*([^()]+?)\s*\)')
# `*name` used as an operator: nothing that could end an operand before it, and
# not a second star (`**p` is a different question).
STAR = re.compile(r'(?<![\w\)\]\*])\*([A-Za-z_]\w*)\b')
# A star that binds a type or applies to something other than the bare name.
# Both were found by reading the first run's thirty-one findings one at a time,
# which turned up four that were not dereferences at all:
#
#   `P2Count *direct[11], *mirror[11], *rot[11];`  -- declarators in a list
#   `float (*nb)[4]`                               -- a pointer to an array
#   `*out[c+2]++ = rp[c];`                         -- the star is on `out[c+2]`
#
# The first and third are the same shape read two ways: a star whose name is
# followed by `[` binds to the subscript, not to the name.  The second is the
# parenthesised declarator, which is a star between `(` and a name followed by
# `)`.  Two characters of context settle all four; a rule about what the *line*
# looks like would have had to tell `uint32_t tot = *freq;` from
# `uint16_t fq, *slot;` and would have been argued with.
# And the third shape, which only showed once the first two were gone: a
# declarator with an initialiser.  `P2Freq *frec = &freq[ctx_l];` has an
# operator context on both sides of the star and is a declaration all the same,
# and seven of them came through.  What settles it is the *type name* to the
# left -- an identifier at the start of the statement, or after `const`.
TYPED = re.compile(r'(?:^|[;,\(]|\b(?:const|volatile|static|inline)\b)\s*'
                   r'[A-Za-z_][\w:]*(?:\s*<[^;]*>)?\s*$')


def binds_type(c, start, end, decl_line=False):
    after = c[end:end + 1]
    if after == '[':
        return True
    before = c[:start].rstrip()[-1:]
    if before == '(' and after == ')':
        return True
    if TYPED.search(c[:start]):
        return True
    # A fourth shape, and the last: the second and later declarators of a list
    # with initialisers.  `const uint8_t *e0 = &pal[6*j], *e1 = &pal[6*j+3];`
    # has no type name before `*e1`, only a comma -- and a comma before a star
    # is also what a call argument looks like.  What tells them apart is the
    # *first* star on the line: if that one bound a type, this is a declarator
    # list and so does this one.
    return decl_line and before == ','


def decl_star(c):
    """Does the first star on this line bind a type?"""
    m = STAR.search(c)
    return bool(m) and bool(TYPED.search(c[:m.start()]))
OPEN = re.compile(r'^\S.*\)\s*(?:const\s*)?\{\s*$')


def bodies(lines):
    """(name, first, last) for each definition.

    **A signature that wraps is still a signature**, and requiring the whole of
    one on a line skipped every body whose parameters did not fit --
    `bmp_rle_encode`, `write_bmp_palette`, `fit_alpha_weights`,
    `choose_alpha_plane`, `search_planes` and thirty more -- along with every
    in-class method, which is most of the tree.  Those were reported as nothing
    rather than as skipped.

    A hand-rolled version of this fixed the wrapping and still missed the
    methods, which is the argument for not hand-rolling it: `structs.bodies`
    exists, says in its own docstring that it was written after exactly this
    happened to forty-three other tools here, and answers 331 where the walk it
    replaced answered 190.
    """
    return [(nm, a, b) for a, b, nm, _sig in structs.bodies(lines)]


def declares(line, name):
    """Does `line` declare `name`?"""
    return bool(re.search(r'(?:^|[;,(])\s*(?:const\s+|static\s+|volatile\s+)*'
                          r'[A-Za-z_][\w:]*(?:\s*<[^;]*>)?[\s*&]+%s\b'
                          % re.escape(name), line))


def moves(lines, name):
    """Does this body advance `name`, or assign to it after declaring it?

    Two things it must not count.  `*p = x` assigns *through* `p` and does not
    move it.  And the declaration is where the pointer is set up, not where it
    walks -- without that, `SymEntry* list = ent;` read as "list is assigned
    here" and excluded `*list = list[-1];`, while `SymEntry *slot = &ent[n];`
    two files over did not, because the star sits against the name there and the
    lookbehind happened to catch it.  The same line, reported or not by where
    the space fell.  Both are findings; a declaration is skipped whole.
    """
    n = re.escape(name)
    pats = (r'\+\+\s*%s\b', r'%s\s*\+\+', r'--\s*%s\b', r'%s\s*--',
            r'(?<![*\]])\b%s\s*[-+]=', r'(?<![=!<>*])(?<!\]\s)\b%s\s*=(?!=)')
    for l in lines:
        if declares(l, name):
            continue
        for pat in pats:
            if re.search(pat % n, l):
                return True
    return False


def code(line):
    return line.split('//')[0]


def findings(path):
    lines = open(path).read().split('\n')
    spans = bodies(lines)
    out = []
    for i, l in enumerate(lines):
        c = code(l)
        for m in OFFSET.finditer(c):
            out.append((path, i + 1, '*(%s+%s)' % m.groups(),
                        '%s[%s]' % m.groups(), l.strip()[:70]))
        decl_line = decl_star(c)
        for m in STAR.finditer(c):
            if binds_type(c, m.start(), m.end(), decl_line):
                continue
            name = m.group(1)
            span = [s for s in spans if s[1] <= i <= s[2]]
            if not span:
                continue
            rows = [code(x) for x in lines[span[0][1]:span[0][2] + 1]]
            if moves(rows, name):
                continue
            if not re.search(r'\b%s\s*\[' % re.escape(name), '\n'.join(rows)):
                continue
            out.append((path, i + 1, '*' + name, name + '[0]', l.strip()[:70]))
    return out


def apply(path, found):
    lines = open(path).read().split('\n')
    for _p, line, was, now, _t in sorted(found, key=lambda f: -f[1]):
        i = line - 1
        c, _, note = lines[i].partition('//')
        c = c.replace(was, now)
        lines[i] = c + ('//' + note if note else '')
    open(path, 'w').write('\n'.join(lines))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if not args:
        raise SystemExit('usage: unstar.py <file>... [--list] [--all]')
    total = []
    for path in args:
        found = findings(path)
        total += found
        if '--all' in sys.argv and found:
            apply(path, found)
    if '--list' in sys.argv:
        for path, line, was, now, text in total:
            print('%-22s %s:%-5d %-14s -> %-14s %s'
                  % ('', path, line, was, now, text))
    if '--all' in sys.argv:
        print('%d dereferences rewritten as subscripts' % len(total))
        return
    print('%d dereferences that would read as subscripts beside subscripts '
          'that do' % len(total))


main()
