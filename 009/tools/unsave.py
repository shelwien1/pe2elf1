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

# A member is not the local of the same name: `blk->slot` is not `slot`.  A
# pattern that names an identifier has to say it is not reaching through one.
NAMED = r'(?<![\w.])(?<!->)%s(?!\w)'


def named(x):
    """`x` as an identifier, whether it is a name or one lane of an array.

    The trailing guard is `(?!\\w)` and not `\\b` because a lane ends in `]`,
    which is not a word character: `x3\\[3\\]\\b` requires a word character
    *after* the bracket and so matches nothing.
    """
    return NAMED % re.escape(x)


# The saving slot may be one lane of an array.  MSVC parks a register in a
# sixteen-byte stack slot as readily as in a four-byte one, and Hex-Rays writes
# that as `x3[3] = wt8_up;` -- which a pattern wanting a bare name on the left
# cannot see.  `choose_plane_coding` alone had six such pairs around two loops
# that touch neither value, under a rule reporting zero: the same blindness the
# raw-offset row had in section 30, in a different tool.
ASSIGN = re.compile(r'^\s*([A-Za-z_]\w*(?:\[\d+\])?) = ([A-Za-z_]\w*);\s*$')
LABEL = re.compile(r'^\s*LABEL_\d+:')
# `&x` is the variable's address; `&x->m`, `&x.m` and `&x[i]` are not -- they
# name a member or an element, and a callee holding one of those cannot change
# `x` itself.  Without the lookahead every `*(uint16_t *)&v533->w2` read as
# "address taken" and disqualified the local, which is thirteen of the counter
# pointers in `alt_p2_model` alone.
ADDR = r'&\s*%s(?!\w)(?!\s*(?:->|\.|\[))'
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s(?!\w)',
         r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         ADDR,
         # Hex-Rays writes a sub-register through these, and a partial write is
         # still a write: `BYTE1(v20) = 3` leaves the restore below stale.
         r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)',
         r'\b(?:BYTE[123]|WORD[123])\s*\(\s*%s\s*\)']


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        used, shapes = set(), []
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
            # The outside test is deferred: see `prune` below.  A slot MSVC
            # reuses for several disjoint pairs blocks *itself* here -- the
            # first pair sees the second pair's two lines as other uses and
            # refuses, and so does the second.  `choose_plane_coding` parks
            # `best_cost` in `x3[0]` five times over five loops, and all five
            # rejected each other.
            if any(re.search(p % re.escape(slot), '\n'.join(region))
                   for p in WRITE):
                continue
            # A lane can be written without being named.  `memset(x1, 0, 16)`
            # and `*(double *)x1` reach the whole slot, and
            # `*(int64_t *)&x1[2] = __PAIR64__(...)` writes lanes 2 *and* 3
            # through the address of lane 2.  So for a lane the region must
            # contain neither the bare array nor the address of any lane of it;
            # a plain subscript of another lane is a different variable and is
            # allowed, which is what keeps `x0[2]` offerable in a region that
            # reads `x0[0]`.
            if '[' in slot:
                arr = slot.split('[')[0]
                whole = r'(?<![\w.])(?<!->)%s(?!\w|\s*\[)' % re.escape(arr)
                lane_addr = r'&\s*%s\s*\[' % re.escape(arr)
                if re.search(whole, '\n'.join(region)) or \
                        re.search(lane_addr, '\n'.join(region)):
                    continue
            body = '\n'.join(region)
            if 'goto ' in body or any(LABEL.match(r) for r in region):
                continue
            if sum(r.count('{') - r.count('}') for r in region) != 0:
                continue
            if any(re.search(p % re.escape(held), body) for p in WRITE):
                continue
            # A callee handed `X` -- or its address -- can change it unseen.
            if any(re.search(NAMED % re.escape(held), c)
                   for c in re.findall(r'\w+\s*\(([^;]*)\)', body)):
                continue
            reads = sum(1 for r in region if re.search(named(slot), r))
            used.update((i, j))
            shapes.append((i, j, slot, held, len(region), reads))
        for i, j, slot, held, n, reads in prune(code, shapes):
            out.append((nm, a + i, a + j, slot, held, n, reads))
    return out


def prune(code, shapes):
    """Drop the pairs whose slot is named outside every pair that owns it.

    The rule is still "the saving name exists only for this pair", but a slot
    MSVC reuses is owned by *all* of its pairs, not just the ones already
    accepted -- and which of those survive depends on this test, so it is a
    fixed point.  Dropping a pair can only remove ownership, so removing the
    violators and asking again terminates.

    A line inside a pair's own region is an alias read and is allowed; that is
    what the `i <= k <= j` window says.
    """
    keep = list(shapes)
    while True:
        owned = {}
        for i, j, slot, _h, _n, _r in keep:
            owned.setdefault(slot, set()).update((i, j))
        bad = []
        for k, (i, j, slot, _h, _n, _r) in enumerate(keep):
            if any(not (i <= x <= j) and x not in owned.get(slot, ())
                   and re.search(named(slot), ln)
                   and not undup.declaration(ln)
                   for x, ln in enumerate(code)):
                bad.append(k)
        if not bad:
            return keep
        for k in reversed(bad):
            del keep[k]


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
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
            lines[k] = re.sub(named(slot), held, lines[k])
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
