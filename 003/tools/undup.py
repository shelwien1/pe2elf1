#!/usr/bin/env python3
"""Collapse an `if`/`else` whose two arms are the same code.

    python3 tools/undup.py subs1.hpp
    python3 tools/undup.py subs1.hpp --all

MSVC proves at compile time that a copy loop cannot alias, fails to prove it,
and emits *both* answers -- a six-line test over a pair of pointers, and behind
each arm the same loop written out again with fresh temporaries:

    if ( n <= 6 || (uint32_t)dst <= src || ... )     i = 0;
    {                                                ofs = 0;
      i = 0; ofs = 0;                                do
      do { dst[i] = img[ofs+16]; ... }        →      {
      while ( i < n );                                 dst[i] = img[ofs+16];
    }                                                  ...
    else                                             }
    {                                                while ( i < n );
      i2 = 0; ofs2 = 0;
      do { dst[i2] = img[ofs2+16]; ... }
      while ( i2 < n );
    }

A test whose arms agree is not a decision.  The condition is the compiler's
aliasing proof leaking into the source; the program has no case there to
distinguish, and both the test and one of the copies can go.

Two arms are "the same" when their token streams match exactly except at
identifiers, and the identifiers that differ line up one-to-one -- `i` against
`i2` everywhere, never `i` against `i2` in one place and `ofs2` in another.
That is a renaming, not a difference.

It refuses unless the collapse cannot be observed:

  * every name that differs between the arms must appear nowhere else in the
    function, so dropping one arm cannot orphan a value something later reads;
  * the condition must contain no assignment and no call, because deleting it
    deletes those too.  `(Src_2 = (uint32_t)&blk[d + 16], plane_count <= 0)` is
    a real condition in this file and is why the check exists;
  * the arms must be brace-balanced blocks, and neither may contain a label --
    a label in the dropped arm would orphan whatever jumps to it.

What it will not find is the same shape written with `goto`s instead of an
`else`, which is the other way MSVC spells it.  Those are visible in
`degoto.py --why` as labels reached by more than one `goto`, and collapsing one
is the hand work `colour_transform` needed.

It reports zero on the file as it stands, and that is worth nothing on its own:
a rule written from a case that has already been fixed will report zero whether
it works or not.  So it is checked the other way round --

    python3 tools/undup.py <the file before the fix>

-- which finds `colour_transform`'s pair and declines it, because the condition
assigns `Src_2` inside itself.  That is the right answer and it is the reason
the assignment check is in the list above: the collapse there was safe only
because `Src_2` was read nowhere but the condition, and reading that is not
something this can do.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

TOKEN = re.compile(r'[A-Za-z_]\w*|\d+|\S')
IDENT = re.compile(r'^[A-Za-z_]\w*$')
IFHEAD = re.compile(r'^(\s*)if \( ')
LABEL = re.compile(r'^\s*LABEL_\d+:')
# A local declaration, including the continuation lines of one Hex-Rays wrapped:
# a run of names and commas with no operator, ending in `;` or a comma.
DECL = re.compile(r'^\s*(?:(?:const|static|volatile|alignas\([^)]*\))\s+)*'
                  r'(?:[A-Za-z_]\w*[\s*]+)?[\w\s,*\[\]]+[,;]\s*$')
# `return v17;` and `goto LABEL_3;` fit that shape exactly, and a name whose
# only other appearance is in a `return` must not be read as dead.  The
# exclusion is a separate test rather than a lookahead because a lookahead
# after `^\s*` backtracks into the indentation and passes anyway.
STMT = re.compile(r'^\s*(?:return|break|continue|goto|else|case|do)\b')


def declaration(line):
    return DECL.match(line) and not STMT.match(line)
# Anything the token stream may name that is not a local: keywords, types, and
# the intrinsics.  A mismatch on one of these is a real difference.
FIXED = set('''if else for while do switch case default return goto break continue
sizeof const static struct union enum void char short int long float double signed
unsigned bool true false nullptr uint8_t int8_t uint16_t int16_t uint32_t int32_t
uint64_t int64_t uintptr_t size_t ptrdiff_t alignas LOBYTE HIBYTE LOWORD HIWORD
LODWORD HIDWORD BYTE1 BYTE2 BYTE3 WORD1'''.split())


def condition(lines, i):
    """(indent, text, line after the condition) for an `if` at line i.

    The condition is often several lines -- MSVC's aliasing proofs run to six --
    and matching only single-line `if`s silently skips exactly the cases this
    tool exists for.  That is how the first version of it reported zero on the
    very function it was written from, and why a new rule gets run against the
    file from *before* the change that motivated it.
    """
    m = IFHEAD.match(lines[i])
    if not m:
        return None
    d, txt = 0, []
    for j in range(i, min(i + 12, len(lines))):
        l = lines[j].split('//')[0]
        txt.append(l.strip())
        d += l.count('(') - l.count(')')
        if d == 0:
            if not l.rstrip().endswith(')'):
                return None
            body = ' '.join(txt)
            return m.group(1), body[body.index('(') + 1:body.rindex(')')].strip(), j + 1
    return None


def block(lines, i):
    """The half-open range of the `{ ... }` starting at line i, or None."""
    if lines[i].strip() != '{':
        return None
    d = 0
    for j in range(i, len(lines)):
        d += lines[j].count('{') - lines[j].count('}')
        if d == 0:
            return i + 1, j
    return None


def same(a, b):
    """The one-to-one renaming that makes the two token streams equal, or None."""
    ta = [t for l in a for t in TOKEN.findall(l.split('//')[0])]
    tb = [t for l in b for t in TOKEN.findall(l.split('//')[0])]
    if len(ta) != len(tb):
        return None
    fwd, rev = {}, {}
    for x, y in zip(ta, tb):
        if x == y:
            continue
        if not (IDENT.match(x) and IDENT.match(y)) or x in FIXED or y in FIXED:
            return None
        if fwd.setdefault(x, y) != y or rev.setdefault(y, x) != x:
            return None
    return fwd


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        for i in range(a, b + 1):
            c = condition(lines, i)
            if not c:
                continue
            ind, cond, after = c
            arm1 = block(lines, after)
            if not arm1 or arm1[1] + 1 > b or lines[arm1[1] + 1].strip() != 'else':
                continue
            arm2 = block(lines, arm1[1] + 2)
            if not arm2:
                continue
            A = lines[arm1[0]:arm1[1]]
            B = lines[arm2[0]:arm2[1]]
            if any(LABEL.match(l) for l in A + B):
                continue
            ren = same(A, B)
            if ren is None:
                continue
            # The condition disappears with the test, so anything it does must
            # not matter.
            if re.search(r'(?<![=!<>+\-*/|&^%])=(?!=)', cond) or re.search(r'\w+\s*\(', cond):
                out.append((nm, i, arm2[1], ren, cond, False))
                continue
            # A renamed name that lives outside the two arms would be orphaned.
            # Hex-Rays declares every local at the top of the body, so the
            # declaration mentions all of them -- counting that as a use makes
            # the check reject everything, which is how the first version of
            # this missed `interleave_plane`'s pair while finding
            # `colour_transform`'s.  A declaration is not a use.
            outside = [l.split('//')[0] for l in lines[a:i] + lines[arm2[1] + 1:b + 1]
                       if not declaration(l)]
            names = set(ren) | set(ren.values())
            if any(re.search(r'\b%s\b' % re.escape(x), l)
                   for x in names for l in outside):
                continue
            out.append((nm, i, arm2[1], ren, cond, True))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)
    ok = [c for c in found if c[5]]

    if '--all' not in sys.argv:
        for nm, i, end, ren, cond, safe in found:
            print('%6d  %-24s %2d lines, %d renamed%s   if ( %s )'
                  % (i + 1, nm.lstrip('_'), end - i, len(ren),
                     '' if safe else ', CONDITION HAS AN EFFECT', cond[:34]))
        print('%d if/else pairs with identical arms, %d collapsible'
              % (len(found), len(ok)))
        return 0

    for nm, i, end, ren, cond, _safe in sorted(ok, key=lambda c: -c[1]):
        ind, _cond, after = condition(lines, i)
        arm1 = block(lines, after)
        keep = lines[arm1[0]:arm1[1]]
        # Shift the arm left by one level, do not flatten it: re-indenting every
        # line to the `if`'s own indent puts the body of a `do` loop level with
        # its braces.  The shift is the arm's base indent minus the `if`'s.
        base = min((len(l) - len(l.lstrip()) for l in keep if l.strip()),
                   default=len(ind))
        cut = max(0, base - len(ind))
        body = [l[cut:] if l.strip() else l for l in keep]
        lines[i:end + 1] = body
    open(path, 'w').write('\n'.join(lines))
    print('%d if/else pairs collapsed to one arm' % len(ok))
    return 0


if __name__ == '__main__':
    sys.exit(main())
