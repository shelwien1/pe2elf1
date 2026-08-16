#!/usr/bin/env python3
"""Delete a local that is only ever a copy of another local.

    python3 tools/uncopy.py subs1.hpp
    python3 tools/uncopy.py subs1.hpp --all

MSVC keeps a value in more than one register across a long straight-line
block, and Hex-Rays names every register, so one row cursor arrives as a
dozen names that are all the same pointer:

    v282 = a4->cursor[0] - 1;               ra0 = a4->cursor[0] - 1;
    ...                                     ...
    v93 = v282;                       →     p2_row[4][1] = ra0[-1].lane[0]
    p2_row[4][1] = v93[-1].lane[0]                       - ra0[-2].lane[0];
                 - v93[-2].lane[0];         p2_row[5][2] = ra0->lane[0]
    v97 = v282;                                          - ra0[-2].lane[0];
    p2_row[5][2] = v97->lane[0]
                 - v97[-2].lane[0];

`alt_p2_context` has nineteen of these over six reference-row cursors, and
naming them one at a time would have produced nineteen names for six things.
Reading the body as if `v93` and `v97` were different rows is exactly the
mistake the spelling invites.

This is not `unsave.py`. There the pair is a save and a restore and both lines
go; here there is no restore -- the copy is written once, read, and abandoned,
and what makes it removable is that neither name changes while the copy is
live.

A copy qualifies when:

  * the line is `X = Y;` or `X = (T)Y;` with `X` and `Y` plain identifiers the
    body declares, and `T` spelled the same as `X`'s own declared type, so the
    cast cannot be changing the value;
  * `X` and `Y` are declared with the same type, for the same reason;
  * `X` is assigned exactly once in the body -- this line -- and its address is
    never taken;
  * `Y` is not assigned anywhere between the copy and `X`'s last use, and its
    address is never taken anywhere in the body, so no callee can write it
    behind this rule's back;
  * that span contains no label, so control cannot enter it holding a
    different `Y`.

The uses of `X` become uses of `Y` and the copy line goes; `unused.py` clears
the declaration.

`Y`'s address being taken anywhere at all -- rather than only inside the span
-- is deliberate. A callee holding `&Y` can write it on a later call, and the
span is not where that would show up.

**Two blind spots this was green through, both found by hand and both proved on
a probe before they were fixed.**

*The source has to be a name the body declares.* A parameter is declared in the
signature and `this` is declared nowhere, so `blk1 = blk` and `this_1 = this`
were not copies as far as this was concerned -- the rule read `src not in ty`
and moved on without a word. Four of them sat in `unmodel_plane_slow` and
`reduce_alphabet`, dereferenced fifty-nine times between them, through every
green sweep this tool has been in. Parameters come out of the signature now,
and `this` is admitted against the enclosing class.

*The span was textual, and a loop is not.* `Y` must not be assigned between
the copy and `X`'s last use -- and "between" was `code[i+1:last+1]`, the lines
in the file. A backward edge makes a line *below* the last use run *before*
it, so `--at` under a loop's closing brace was outside the span and inside the
region. That is `bucket_top` in both pixel coders, where folding it onto
`bucket` turns "is this the first turn" into a tautology and moves the stream.
The span now runs to the close of the innermost loop still open at the last
use; `enclosing_end` below is that, and the probe with exactly the shape is
what caught it.

*The copy has to be its own statement.* Hex-Rays declares at the top and
assigns lower down, which is the form the rule was written for; a hand-written
`ModelBlock *blk1 = blk;` is a declaration, and declarations are filtered out
of the use list, so its own line was never `uses[0]` and it was skipped. It
counts now when the declaration names exactly one variable, which is the case
where deleting the line deletes nothing else.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import undup                                                      # noqa: E402
import unreload                                                   # noqa: E402

COPY = re.compile(r'^\s*([A-Za-z_]\w*) = (.*);\s*$')
CAST = re.compile(r'^\(([A-Za-z_][\w\s*]*?)\s*\)\s*(.*)$')
NAME = re.compile(r'^[A-Za-z_]\w*$')
LABEL = re.compile(r'^\s*LABEL_\d+:')
# What opens a loop, for `enclosing_end` below.
LOOP = re.compile(r'\b(?:for|while)\s*\(|\bdo\b\s*$')
# `&x` is the variable's address; `&x->m`, `&x.m` and `&x[i]` are not -- they
# name a member or an element, and a callee holding one of those cannot change
# `x` itself.  Without the lookahead every `*(uint16_t *)&v533->w2` read as
# "address taken" and disqualified the local, which is thirteen of the counter
# pointers in `alt_p2_model` alone.
# A member is not the local of the same name.  `blk->band_lo = ...` matched
# `\bband_lo\b` and counted as a use of the local `band_lo`, which put a use
# *before* the assignment and disqualified the copy.  Same family as `ADDR`:
# a pattern that names an identifier has to say it is not reaching through one.
USE = r'(?<![\w.])(?<!->)%s\b'
ADDR = r'&\s*%s\b(?!\s*(?:->|\.|\[))'
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)',
         r'\b(?:BYTE[123]|WORD[123])\s*\(\s*%s\s*\)']


def spelled(ty):
    """A declared type as a comparable string: `P2Ctx` + one star is `P2Ctx *`."""
    return re.sub(r'\s+', ' ', '%s %s' % (ty[0], '*' * ty[1])).strip()


def wrapped(s):
    """True when the whole of `s` sits inside one pair of parentheses."""
    if not s.startswith('(') or not s.endswith(')'):
        return False
    depth = 0
    for i, c in enumerate(s):
        depth += (c == '(') - (c == ')')
        if depth == 0:
            return i == len(s) - 1
    return False


def peel(rhs):
    """(casts outermost first, bare name) for `(A *)((B *)x)`, else (_, None)."""
    casts = []
    while True:
        rhs = rhs.strip()
        while wrapped(rhs):
            rhs = rhs[1:-1].strip()
        m = CAST.match(rhs)
        if not m or not m.group(2).strip():
            break
        casts.append(re.sub(r'\s+', ' ', m.group(1)).strip())
        rhs = m.group(2)
    return casts, rhs if NAME.match(rhs) else None


PARAM = re.compile(r'^\s*(?:const\s+)?([A-Za-z_]\w*)\s*((?:\*|\s)*)([A-Za-z_]\w*)\s*$')
# `T *x = y;` -- one declarator, one initialiser.  Two identifiers before the
# `=` is what tells it from a plain `x = y;`, which `COPY` handles, and the
# anchors are what keep a `for( int32_t i = 0; ...` header out.
DECLINIT = re.compile(r'^\s*(?:const\s+)?([A-Za-z_]\w*)(\s*\*+\s*|\s+)'
                      r'([A-Za-z_]\w*)\s*=\s*([^;]*);\s*$')
# The separator between the two identifiers has to be there.  Written as
# `((?:\*|\s)*?)` it could match nothing, and `blkA = blk;` parsed as a
# declaration of `A` with type `blk` -- which took the plain-assignment form
# this rule was built for away from `COPY` and reported zero where it used to
# report one.  The probe that caught it is the third in this file's tests.
NOT_A_TYPE = ('return', 'else', 'case', 'do', 'goto')
CLASS = re.compile(r'^\s*(?:template\s*<[^;]*>\s*)?(?:struct|class)\s+'
                   r'(?:alignas\s*\([^)]*\)\s*)?([A-Za-z_]\w*)\b[^;]*\{')


def params(sig):
    """{name: (type, stars)} for the parameters `sig` names.

    Only the plain ones: `T x`, `T *x`, `const T* x`.  A parameter with a
    default, an array bound or a function type is not a name this rule can fold
    onto, and leaving it out of the table is the same as not seeing it.
    """
    m = re.search(r'\(([^()]*)\)\s*(?:\b(?:const|noexcept)\b\s*)*\{?\s*$',
                  sig.rstrip().rstrip('{').rstrip() + '{')
    if not m:
        return {}
    out = {}
    for part in m.group(1).split(','):
        d = PARAM.match(part)
        if d:
            out[d.group(3)] = (d.group(1), d.group(2).count('*'))
    return out


def receiver(lines, at):
    """The class whose body encloses line `at`, or None.

    Walked from the top rather than backwards, because a backwards walk cannot
    tell an enclosing `struct X {` from a sibling one that has already closed.
    """
    stack, depth = [], 0
    for i, l in enumerate(lines[:at]):
        s = l.split('//')[0]
        m = CLASS.match(s)
        opened = s.count('{')
        if m and opened:
            stack.append((depth, m.group(1)))
        depth += opened - s.count('}')
        while stack and depth <= stack[-1][0]:
            stack.pop()
    return stack[-1][1] if stack else None


def enclosing_end(code, i, last):
    """One past the last line that can run between the copy and `dst`'s last use.

    Textually that is `last`, and textually was wrong.  A backward edge makes a
    line *below* the last use run *before* it, so a `--src` under the loop's
    closing brace is inside the region even though it is under it in the file:

        top = at;                       // the copy
        do {
          total += (at == top);         // `top`'s last use
          --at;                         // below it, and runs first
        } while( at );

    Folding `top` onto `at` there makes the test always true.  A probe with
    exactly that shape is what caught it; `bucket_top` in both pixel coders is
    the shape in the tree, and this rule offered to break both.

    So the span runs to the end of whatever block encloses the last use, when
    that block is a loop: from the copy, count braces, and if the depth ever
    goes below where it started before reaching `last`, the copy and the use
    are not in one straight run and there is no loop to widen to.  Otherwise
    widen to the close of the innermost loop still open at `last`.
    """
    depth, opens = 0, []
    for k in range(i + 1, len(code)):
        c = code[k]
        head = ''
        for ch in c:
            if ch == '{':
                opens.append((depth, LOOP.search(head) is not None))
                depth += 1
                head = ''
            elif ch == '}':
                depth -= 1
                if opens:
                    opens.pop()
                head = ''
            else:
                head += ch
        if k >= last:
            break
    if k < last or not any(is_loop for _d, is_loop in opens):
        return last + 1
    # the innermost loop still open at `last`: walk on to its close
    want = max(d for d, is_loop in opens if is_loop)
    for j in range(k + 1, len(code)):
        for ch in code[j]:
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth <= want:
                    return j + 1
    return len(code)


def candidates(lines):
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        ty = unreload.types(code)
        # A parameter is a name of this body as much as a local is; the copy
        # rule below is about what the name holds, not where it was written.
        for n, t in params(sig or '').items():
            ty.setdefault(n, t)
        # `unreload.types` reads `T x;` and not `T x = y;`, so a hand-written
        # declaration-with-initialiser had no type either.
        for l in code:
            di = DECLINIT.match(l)
            if di and di.group(1) not in NOT_A_TYPE:
                ty.setdefault(di.group(3), (di.group(1), di.group(2).count('*')))
        cls = receiver(lines, a)
        if cls:
            ty.setdefault('this', (cls, 1))
        for i, l in enumerate(code):
            decl = DECLINIT.match(l)
            if decl and decl.group(1) in NOT_A_TYPE:
                decl = None
            m = None if decl else COPY.match(l)
            if decl:
                dst, rhs = decl.group(3), decl.group(4)
            elif m:
                dst, rhs = m.group(1), m.group(2)
            else:
                continue
            casts, src = peel(rhs)
            if src is None or dst == src or dst not in ty or src not in ty:
                continue
            if ty[dst] != ty[src]:
                continue
            # The outermost cast has to name the type the value already has --
            # anything else is a width question this cannot answer.  Casts
            # inside it are allowed only between pointers, where the round trip
            # is the identity on a target whose pointers are all one width;
            # this file pins that with `sizeof(void *) != 4 ||` asserts.
            if casts:
                if casts[0] != spelled(ty[dst]):
                    continue
                if not all('*' in c for c in casts) or not ty[dst][1]:
                    continue
            uses = [k for k, r in enumerate(code)
                    if re.search(USE % re.escape(dst), r)
                    and (k == i or not undup.declaration(r))]
            if not uses or uses[0] != i:
                continue
            last = uses[-1]
            # Assigned exactly once, and never through its address.
            if any(re.search(p % re.escape(dst), code[k])
                   for k in uses[1:] for p in WRITE):
                continue
            if re.search(ADDR % re.escape(dst), '\n'.join(code)):
                continue
            if re.search(ADDR % re.escape(src), '\n'.join(code)):
                continue
            span = code[i + 1:enclosing_end(code, i, last)]
            if any(LABEL.match(r) for r in span):
                continue
            if any(re.search(p % re.escape(src), '\n'.join(span)) for p in WRITE):
                continue
            out.append((nm, a + i, a + last, dst, src, len(uses) - 1))
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, i, _j, dst, src, n in found:
            print('%6d  %-24s %-12s is %-12s %d reads'
                  % (i + 1, nm.lstrip('_'), dst, src, n))
        print('%d locals that are only a copy of another, %d reads'
              % (len(found), sum(n for _a, _b, _c, _d, _e, n in found)))
        return 0

    # A chain has to be resolved to its root before anything is rewritten.
    # `blkA = blk; blkB = blkA;` is two folds, and applying them as written left
    # `blkA` in the text with both declarations deleted -- a name the fold had
    # just removed.  Each `dst` is assigned exactly once by the rule above, so
    # following `dst -> src` terminates.
    root = {dst: src for _n, _i, _j, dst, src, _c in found}
    def resolve(n):
        seen = set()
        while n in root and n not in seen:
            seen.add(n)
            n = root[n]
        return n
    for _nm, i, last, dst, src, _n in found:
        to = resolve(src)
        for k in range(i + 1, last + 1):
            lines[k] = re.sub(r'\b%s\b' % re.escape(dst), to, lines[k])
    for k in sorted({i for _n, i, _j, _d, _s, _c in found}, reverse=True):
        del lines[k]
    open(path, 'w').write('\n'.join(lines))
    # The assignment goes; the *declaration* does not, and it is usually a name
    # in the middle of a shared comma list two hundred lines up.  So a fold
    # leaves an unused local behind, which `BMF_WARN=1 ./build.sh` names one
    # per line -- that is the intended second step, not an oversight, and the
    # six folded when this note was written left six warnings that said exactly
    # which names to drop.
    print('%d copies folded onto what they copy; the build will name the '
          'declarations left behind' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
