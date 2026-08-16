#!/usr/bin/env python3
"""A run of statements written out more than once, under different names.

    python3 tools/dupblock.py bmf.cpp             # report
    python3 tools/dupblock.py bmf.cpp --list      # and every run

    kk = blk->row_cur[4];              PixRec *r4 = blk->row_cur[4];
    row_cur3 = blk->row_cur[3];        PixRec *row_cur3 = blk->row_cur[3];
    ...                                ...
    blk->row_cur[0] = kk;              blk->row_cur[0] = r4;

REFACTOR2 phase 3 looked for six or more *identical* lines and took the tree to
zero.  This asks the same question with the identifiers normalised away, and the
answer was 108 runs and a thousand lines -- because a decompiler names a
temporary after the register it landed in, so two copies of one idea almost
never spell it the same way twice.

**The rule.**  Slide a three-line window over the spliced unit, rename each
window's identifiers to `B0`, `B1`, ... in order of first appearance, and group
the windows that agree.  Grow each group as far as it stays in agreement, take
the longest first and let it consume its lines so the same body is not reported
twice under a shorter window.  Windows holding a brace on its own, a blank, or a
preprocessor line are skipped, as are those under sixty characters -- three
short statements match by accident.

**It reports and does not apply.**  Every one of these came out as a named
thing, and what the name should be is the whole of the work: `ring_advance` was
already in the tree and eleven lines of `ModelBlock::start_row` were it,
unrecognisable because five temporaries stood between.  A tool that hoisted the
lines into `static void helper_7(...)` would have removed the duplication and
none of the finding.

**What it is not.**  It is not a claim that a group *should* be merged.  Some of
the runs left are an encoder and a decoder whose shared lines are scaffolding,
and `pairshare.py` measures those; some are a loop the original unrolled, which
is `reroll.py`'s class when the lines are identical and this one's when they are
not.

**What the residue is now, and it is worth saying so the next round does not
start here.**  1031 lines of copy when this was written and 113 now.  The p2
coders' prologue and epilogue used to be the largest groups and are gone: that
pair merged, and so did four more, so `pairshare.py`'s declined table is down
to two rows.

For several rounds the sentence here read "what is left is dominated by
`code_pixel`/`decode_pixel` -- eight of the ten longest runs are in `model.inc`
and every one of them is that pair", and by the time anyone looked again it was
not true.  Naming what that pair shares shortened it faster than the ratchet
came down, and the longest run had moved to `plane_predict.inc`: the colour
transform and its inverse, an encode/decode pair nobody had ever *named* as
one, so no share had ever been measured and no decision taken.  They are one
`template<int32_t f_DEC>` now.

After that the longest runs were four lines apiece, all of them in
`alt_p2_model`'s three unrolled counter-bank blocks -- 150 lines that carried a
comment explaining, in four bullets, why they could not be loops.  All four
bullets had dissolved under later rounds and nobody had gone back to check.
They are three loops now and the residue fell by a third in one commit.

A stale sentence in this docstring is the finding that matters here: this file
records what the residue *is*, and nothing was checking it.  Twice now the
largest entry has been something the prose above did not describe -- and once
it was not copy at all, nine consecutive `uint32_t <name>;` in `ModelBlock`,
one window three times over.  See `DECL` below.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

WINDOW = 3
MIN_CHARS = 60
# This class does not go to zero and should not: an encoder and its decoder
# share their scaffolding, a `switch` arm resembles the arm beside it, and two
# unrelated walks over the same record type look alike for three lines at a
# time.  What it can do is not grow, so the number below is a **ratchet** --
# the lines of copy measured after the round that wrote this tool, which came
# down from 1031.  Lower it whenever it falls; a rise is the finding.
BUDGET = 113

# A declaration and nothing else: a type, a name, an optional array bound, a
# semicolon.  No initialiser, no call, no operator.
#
# These are skipped, and the reason is in this file's first line: what it looks
# for is "a run of *statements* written out more than once".  A field list is
# not statements.  `ModelBlock` opens with nine consecutive `uint32_t <name>;`
# and they normalise to one window three times over -- the largest single entry
# in the residue for several rounds, and nothing a reader could act on: a
# struct's members cannot be factored into a helper, and two bodies declaring
# the same locals is `firstuse.py`'s and `compact_locals.py`'s question, not
# this one.
#
# It is a skip and not a special case for `ModelBlock`: a planted run of six
# identical declarations is not counted, and a planted run of six identical
# *statements* still is.
#
# One declarator or several -- `double dv0, dv1, dv2;` is one line and three
# names, and a first version of this pattern matched only the single-name form,
# so a run of comma-separated declarations at the top of two unrelated bodies
# came back as copy the moment something above it moved.  Leading `static`,
# `const`, `constexpr`, `inline` and `alignas(N)` are qualifiers on a
# declaration and not something else; `static_assert(...)` is not matched,
# because a type name here has to be followed by space and a declarator.
_ONE = r'\**\w+(?:\s*\[[^\]]*\])*'
_QUAL = r'(?:(?:static|const|constexpr|inline|alignas\s*\([^)]*\))\s+)*'
DECL = re.compile(r'^%s[A-Za-z_][\w:]*(?:\s*<[^;]*>)?[\s*&]+%s'
                  r'(?:\s*,\s*%s)*\s*;$' % (_QUAL, _ONE, _ONE))


def normal(text):
    """`text` with its identifiers renamed to B0, B1, ... in order."""
    seen = {}

    def sub(m):
        n = m.group(0)
        if n[0].isdigit():
            return n
        if n not in seen:
            seen[n] = 'B%d' % len(seen)
        return seen[n]
    return re.sub(r'[A-Za-z_]\w*', sub, text)


def survey(path):
    lines, origin = structs.splice(path)
    key = [l.split('//')[0].strip() for l in lines]
    windows = collections.defaultdict(list)
    for i in range(len(key) - WINDOW):
        block = key[i:i + WINDOW]
        if any(not b or b in '{}' or b.startswith('#') for b in block):
            continue
        if sum(len(b) for b in block) < MIN_CHARS:
            continue
        if all(DECL.match(b) for b in block):
            continue
        windows[normal('\n'.join(block))].append(i)
    out, taken = [], set()
    for _, ats in sorted(windows.items(), key=lambda kv: -len(kv[1])):
        if len(ats) < 2 or any(a in taken for a in ats):
            continue
        n = WINDOW
        while len({normal('\n'.join(key[a:a + n + 1])) for a in ats}) == 1:
            n += 1
        for a in ats:
            taken.update(range(a, a + n))
        out.append((n, [origin[a] for a in ats], key[ats[0]]))
    out.sort(reverse=True)
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/dupblock.py bmf.cpp [--list]')
    found = survey(sys.argv[1])
    copied = sum(n * (len(ats) - 1) for n, ats, _ in found)
    if '--list' in sys.argv:
        for n, ats, first in found:
            print('  %2d lines x%d  %s\n      %s'
                  % (n, len(ats), ', '.join('%s:%d' % o for o in ats), first[:66]))
    print('%d lines over the %d-line ratchet; %d runs written out more than '
          'once under different names, %d lines of copy'
          % (max(0, copied - BUDGET), BUDGET, len(found), copied))


if __name__ == '__main__':
    main()
