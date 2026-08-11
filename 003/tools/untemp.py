#!/usr/bin/env python3
"""Inline a local that is assigned once and read once.

    python3 tools/untemp.py subs1.hpp
    python3 tools/untemp.py subs1.hpp --all

MSVC spills a subexpression into a register and Hex-Rays gives the register a
name, so a swap of two counts arrives as four statements and two names:

    v17 = v16->cnt;                     n251_1->cnt = v16->cnt;
    n251_1->cnt = v17;          →       v16->cnt = n251_1->cnt;
    v20 = n251_1->cnt;
    v16->cnt = v20;

`unaliasvar.py` folds a local that is one assignment of *another local*; this
one folds a local that is one assignment of an *expression*, which is the same
idea with a harder safety question, because the expression has inputs that
something in between might change.

So that is what is checked, and every condition is about the range between the
assignment and the read:

  * the name is declared once, assigned once, and read once, and the read is
    not itself a write -- no `++`, no `LOWORD(v) =`, no `&v`;
  * the right-hand side contains no call, so re-evaluating it later has no
    effect of its own;
  * nothing in the range assigns, increments or takes the address of any
    identifier the right-hand side reads -- which catches the aliasing case
    too, since `v4->cnt = ...` writes the name `cnt`;
  * the range contains no call at all, because a call can write through a
    pointer this cannot see;
  * the range contains no label and no `goto`, so control cannot enter it;
  * the range's brace depth never goes negative, so the assignment is not
    inside a block the read is outside of -- otherwise an expression guarded by
    an `if` would be evaluated unguarded, and a pointer that is only valid
    inside the block would be dereferenced outside it;
  * the line it produces is not longer than `--width`, 96 by default. A
    temporary whose whole value is that the line fits is not noise.

What it will not do is fold a name read twice. That is a judgement about
whether the expression is worth a name, and this file has plenty that are.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

NAME = re.compile(r'\bv\d+\b')
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         r'&\s*%s\b', r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)']
TYPE = re.compile(r'^\s*(?:const\s+)?(?:u?int\d+_t|uintptr_t|size_t|float|double'
                  r'|void|char|bool|signed|unsigned|[A-Z]\w*)\b')
CALL = re.compile(r'\w+\s*\(')


def candidates(lines, width):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        uses = {}
        for i in range(a, b + 1):
            for v in set(NAME.findall(lines[i].split('//')[0])):
                uses.setdefault(v, []).append(i)
        for v, us in uses.items():
            if len(us) != 3:
                continue
            dec, d, u = us
            if '=' in lines[dec] or not TYPE.match(lines[dec]):
                continue
            m = re.match(r'^(\s*)%s = ([^=;][^;]*);\s*$' % v, lines[d])
            if not m or u <= d:
                continue
            rhs = m.group(2)
            # A call in the right-hand side has an effect of its own, and so
            # does `n0x80++`; moving either would move when it happens.
            if CALL.search(rhs) or re.search(r'\+\+|--|(?<![=!<>])=(?!=)', rhs):
                continue
            use = lines[u]
            if len(NAME.findall(use)) and len(re.findall(r'\b%s\b' % v, use)) != 1:
                continue
            if any(re.search(p % re.escape(v), use) for p in WRITE):
                continue
            mid = lines[d + 1:u]
            joined = '\n'.join(mid)
            if 'goto ' in joined or re.search(r'^\s*LABEL_\d+:', joined, re.M):
                continue
            if any(CALL.search(l.split('//')[0]) for l in mid):
                continue
            # Depth must never go negative and must be back to zero at the
            # read, so the read is in the same block as the assignment. Ending
            # above zero means a loop or a branch opened in between and the
            # expression would move inside it -- where a later line of that
            # body could change an input before the next iteration reads it.
            depth, ok = 0, True
            for l in mid:
                depth += l.count('{') - l.count('}')
                if depth < 0:
                    ok = False
                    break
            if not ok or depth != 0:
                continue
            srcs = set(re.findall(r'\b([A-Za-z_]\w*)\b', rhs))
            if any(re.search(p % re.escape(x), joined) for x in srcs for p in WRITE):
                continue
            folded = re.sub(r'\b%s\b' % v,
                            lambda _: ('(%s)' % rhs) if _needs(rhs) else rhs, use)
            if len(folded.rstrip()) > width:
                continue
            out.append((nm, v, d, u, folded))
    # Two temporaries can land on the same line -- `ctx_w[6].sel` below is
    # assembled from `v14` and `v15` -- and each fold is computed against the
    # original, so applying both would lose one. One at a time; the second is
    # found on the next run.
    seen = {}
    for c in out:
        seen.setdefault(c[3], []).append(c)
    lines_used = {c[2] for c in out}
    return [c[0] for c in [v for v in seen.values() if len(v) == 1]
            if c[0][3] not in lines_used]


PRIMARY = re.compile(r'^(?:[&*]?\w+(?:(?:->|\.)\w+|\[[^\[\]]*\])*'
                     r"|\*\([^()]*\)|\(\w+ \*\)\w+)$")


def _needs(rhs):
    """Does the expression need bracketing where it lands?

    Anything that is not a single primary does. The first version of this asked
    what characters surrounded the name at the use site instead, decided that
    `(v36 + ...)` was safe because a `(` preceded it, and turned
    `(v21 < 20 * n) + cnt` into `v21 < (20 * n + cnt)`. Four streams
    segfaulted. Precedence is not a property of the neighbours.
    """
    return not PRIMARY.match(rhs.strip())


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    width = int(sys.argv[sys.argv.index('--width') + 1]) if '--width' in sys.argv else 96
    lines = open(path).read().split('\n')
    found = candidates(lines, width)
    if '--only' in sys.argv:
        lo, hi = (int(x) for x in sys.argv[sys.argv.index('--only') + 1].split(':'))
        found = found[lo:hi]

    if '--all' not in sys.argv:
        for nm, v, d, u, folded in found[:40]:
            print('%6d  %-22s %-6s +%-3d %s' %
                  (d + 1, nm.lstrip('_'), v, u - d, folded.strip()[:70]))
        print('%d single-use temporaries' % len(found))
        return 0

    # Every edit is one line, so applying them from the bottom up keeps every
    # index valid -- sorting by the *assignment* line instead lets a deletion
    # above a later candidate's read shift that read out from under it, which
    # is how the first run of this deleted an opening brace.
    edits = []
    for nm, v, d, u, folded in found:
        edits.append((u, folded))
        edits.append((d, None))
    for i, folded in sorted(edits, key=lambda e: -e[0]):
        if folded is None:
            del lines[i]
        else:
            lines[i] = folded
    open(path, 'w').write('\n'.join(lines))
    print('%d temporaries inlined' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
