#!/usr/bin/env python3
"""Delete a local that saves a value across a region that cannot change it.

    python3 tools/unsave.py subs1.hpp
    python3 tools/unsave.py subs1.hpp --all

MSVC spills a register before a loop and reloads it after, and Hex-Rays gives
the spill slot a name, so a loop that touches neither of them arrives wrapped in
a save and a restore:

    v29 = v16;                              do
    do                                      {
    {                             →           ... nothing writing v16 ...
      ... nothing writing v16 ...           }
    }                                       while ( v17 );
    while ( v17 );
    v16 = v29;

`v29` is not a variable. It is the compiler's note to itself about where `v16`
went, and reading the body as if it were one means reading a value being
carried across a region for a reason -- when the reason is that there is no
reason.

Nine of these were found by hand in one afternoon (`predict_med`,
`unpredict_med` twice, `alt_init_tables`, `transform_planes`, and four in
`free_workspace`), which is what makes it a rule and not a curiosity.

The conditions are all about the region between the two lines:

  * the save is `S = X;` and the restore is `X = S;`, with `X` a plain
    identifier and `S` a local that appears nowhere else in the body;
  * nothing between them writes `X` -- no assignment, no `++`, no `&X`, no
    `LOWORD(X) =` -- and nothing calls a function naming `X`;
  * the region contains no label and no `goto`, so control cannot enter or
    leave it in a way that skips one of the pair;
  * the region brace-balances, so the save and the restore are in the same
    block.

The `&X` and call checks are what make it safe rather than plausible: a region
that hands `X`'s address to a callee can change it without naming it, and this
cannot see inside the callee.

What it will not touch is the other shape, where the restore assigns something
the region *did* change -- that is a real save, and the body needs it.

Nor will it see a pair written with casts.  `unmodel_plane_slow` has
`this_1 = (ModelBlock *)((uint32_t *)this_4); ... this_4 = (ModelBlock *)((int32_t)this_1);`
twice, which is the same artefact, and `ASSIGN` above wants a bare name on both
sides.  Allowing casts would mean deciding which casts preserve the value, and
that is a question about widths this cannot answer from the text -- so those
two were done by hand.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import undup                                                      # noqa: E402

ASSIGN = re.compile(r'^\s*(\w+) = (\w+);\s*$')
LABEL = re.compile(r'^\s*LABEL_\d+:')
# `&x` is the variable's address; `&x->m`, `&x.m` and `&x[i]` are not -- they
# name a member or an element, and a callee holding one of those cannot change
# `x` itself.  Without the lookahead every `*(uint16_t *)&v533->w2` read as
# "address taken" and disqualified the local, which is thirteen of the counter
# pointers in `alt_p2_model` alone.
ADDR = r'&\s*%s\b(?!\s*(?:->|\.|\[))'
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         ADDR,
         # Hex-Rays writes a sub-register through these, and a partial write is
         # still a write: `BYTE1(v20) = 3` leaves the restore below stale.
         r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)',
         r'\b(?:BYTE[123]|WORD[123])\s*\(\s*%s\s*\)']


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        used = set()
        for i, l in enumerate(code):
            m = ASSIGN.match(l)
            if not m:
                continue
            slot, held = m.group(1), m.group(2)
            if slot == held:
                continue
            if i in used:
                continue
            back = re.compile(r'^\s*%s = %s;\s*$' % (re.escape(held), re.escape(slot)))
            for j in range(i + 1, len(code)):
                if back.match(code[j]):
                    break
            else:
                continue
            # The saving name must exist only for this pair and, at most, as a
            # read inside the region -- MSVC sometimes uses the spill slot as
            # the operand while the original register holds something else, so
            # `v34 = v5; ... v34[-1].sym ...; v5 = v34;` is the same artefact
            # with the alias read rather than idle.  Those reads become reads
            # of the original, which is what they are: nothing in the region
            # writes it.
            region = code[i + 1:j]
            # One slot name can serve several pairs -- MSVC reuses the stack
            # slot, so `this_1 = this_4; ... this_4 = this_1;` appears twice in
            # `unmodel_plane_slow` with the same two names.  Uses that belong
            # to a pair already accepted are not uses that block this one.
            outside = [k for k, x in enumerate(code)
                       if not (i <= k <= j) and k not in used
                       and re.search(r'\b%s\b' % re.escape(slot), x)
                       and not undup.declaration(x)]
            if outside:
                continue
            if any(re.search(p % re.escape(slot), '\n'.join(region))
                   for p in WRITE):
                continue
            body = '\n'.join(region)
            if 'goto ' in body or any(LABEL.match(r) for r in region):
                continue
            if sum(r.count('{') - r.count('}') for r in region) != 0:
                continue
            if any(re.search(p % re.escape(held), body) for p in WRITE):
                continue
            # A callee handed `X` -- or its address -- can change it unseen.
            if any(re.search(r'\b%s\b' % re.escape(held), c)
                   for c in re.findall(r'\w+\s*\(([^;]*)\)', body)):
                continue
            reads = sum(1 for r in region if re.search(r'\b%s\b' % re.escape(slot), r))
            used.update((i, j))
            out.append((nm, a + i, a + j, slot, held, len(region), reads))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, i, j, slot, held, n, reads in found:
            print('%6d  %-24s %-10s saves %-10s over %d lines%s'
                  % (i + 1, nm.lstrip('_'), slot, held, n,
                     ', read %d times as an alias' % reads if reads else ''))
        print('%d saves across a region that cannot change the value' % len(found))
        return 0

    # An alias read inside the region becomes a read of what it aliases.
    for _nm, i, j, slot, held, _l, reads in found:
        if not reads:
            continue
        for k in range(i + 1, j):
            lines[k] = re.sub(r'\b%s\b' % re.escape(slot), held, lines[k])
    # One descending pass over every line to remove: pairs can nest, and
    # deleting a pair at a time lets an inner pair's indices shift under an
    # outer one's restore.
    for k in sorted({i for _n, i, _j, _s, _h, _l, _r in found} |
                    {j for _n, _i, j, _s, _h, _l, _r in found}, reverse=True):
        del lines[k]
    open(path, 'w').write('\n'.join(lines))
    print('%d saves deleted' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
