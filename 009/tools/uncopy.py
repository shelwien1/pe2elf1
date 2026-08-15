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


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        ty = unreload.types(code)
        for i, l in enumerate(code):
            m = COPY.match(l)
            if not m:
                continue
            dst = m.group(1)
            casts, src = peel(m.group(2))
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
                    and not undup.declaration(r)]
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
            span = code[i + 1:last + 1]
            if any(LABEL.match(r) for r in span):
                continue
            if any(re.search(p % re.escape(src), '\n'.join(span)) for p in WRITE):
                continue
            out.append((nm, a + i, a + last, dst, src, len(uses) - 1))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, i, _j, dst, src, n in found:
            print('%6d  %-24s %-12s is %-12s %d reads'
                  % (i + 1, nm.lstrip('_'), dst, src, n))
        print('%d locals that are only a copy of another, %d reads'
              % (len(found), sum(n for _a, _b, _c, _d, _e, n in found)))
        return 0

    for _nm, i, last, dst, src, _n in found:
        for k in range(i + 1, last + 1):
            lines[k] = re.sub(r'\b%s\b' % re.escape(dst), src, lines[k])
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
