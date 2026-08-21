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
short statements match by accident -- and so are windows that are declarations
and nothing else (see `DECL`).

Occurrences that **overlap each other** are one repetitive block and not two
copies of one.  Four field zeroings in a row put a window at every line of
themselves and every window normalises alike, so `bmp->biClrImportant = 0;` and
its three neighbours were being reported as a three-line run written twice.  A
group keeps a non-overlapping subset of its occurrences; one that has none is a
block, not duplication.  This was nineteen of the lines it counted.

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
start here.**  1031 lines of copy when this was written and 39 now.  The p2
coders' prologue and epilogue used to be the largest groups and are gone: that
pair merged, and so did four more, so `pairshare.py`'s declined table is down
to two rows.

Nothing left is longer than four lines, and twelve runs of three or four is
what two coders and two run-length forms cost.  Six of the twelve are
`code_pixel`/`decode_pixel` and the two alphabet coders in `sym_code`; the rest
are a pair each of siblings that open the same way -- the two run-length
readers, the two `enum : uint8_t` blocks, the two histogram walks, the two
`save_descriptors` trials.  A shorter list than this one would mean merging
things that are not the same idea.

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
#
# 39 -> 44, and this is the first raise.  It should read like a decision, so:
#
# `alt_model_p1`'s plane release was eleven lines of decompiler noise -- a
# `do`/`while` on a counter reloaded from `plane_count` inside its own body,
# around a null check that `bmf_new` being fatal makes dead.  Stage-2 phase 2
# took it to three:
#
#     for( int32_t f = 0; f<plane_count; ++f )
#       plane[f]->alt_p1_free();
#     return plane_count;
#
# which is now a five-line match -- those three plus a brace and a blank line --
# against `alt_p2_planes_free`, which has always been exactly that.  So the copy
# did not grow; one of its two sites stopped being obscured.
#
# It cannot be shared as it stands.  The two loops differ only in the method
# name, and giving both blocks a `release()` is recorded four files over, in
# `prune_unreachable.py`, as having been tried and reverted: that tool keys a
# map on the unqualified name and asserted `two definitions of release`.  The
# alternatives are a member-function-pointer template or a pair of free-function
# overloads plumbed across an include boundary, and both read worse than the
# three lines they would remove.
#
# So: five lines of trivial loop, deliberately left, in exchange for eight lines
# of noise deleted.  Lower it again the moment something makes the pair
# shareable.
#
# 44 -> 43.  `BMFCodec` put sixty-four method declarations in one sorted list
# and two three-line stretches of it coincided, which took the residue to 46 --
# copy that no edit could remove, because the list *is* the class.  The skip at
# `FUNC` below reads a prototype as what it is, the three lines went, and what
# is left is 43.
BUDGET = 43

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
# **A statement that opens with a keyword is a statement**, and without this
# both patterns below read the keyword as the type: `return idx_s;` parses as a
# declaration of `idx_s` with type `return`, and `return f(x);` as a prototype.
# The self-test planted the second, which is how this came to be here; the first
# had been silently skipped by `DECL` since the day it was written.
_KW = (r'(?!(?:return|delete|throw|new|case|else|goto|sizeof|using|typedef'
       r'|co_return|co_await|co_yield)\b)')
DECL = re.compile(r'^%s%s[A-Za-z_][\w:]*(?:\s*<[^;]*>)?[\s*&]+%s'
                  r'(?:\s*,\s*%s)*\s*;$' % (_QUAL, _KW, _ONE, _ONE))

# **A function declared and not defined, and the `template<...>` line that can
# stand above one.**  Same skip and the same reason as `DECL`: a prototype is
# not a statement, and a list of them cannot be factored into a helper -- it is
# the list of what the helpers *are*.
#
# This arrived with `BMFCodec`.  Sixty-four method declarations in `codec.inc`,
# sorted, put `..._p1_decode; ..._p1_encode; template<int32_t f_DEC>` eight
# lines above `..._p2_decode; ..._p2_encode; template<int32_t f_DEC>`, which
# normalise to the same window -- three lines of "copy" that no edit could
# remove and that took the residue two lines over its ratchet.  Raising the
# ratchet would have recorded a class declaring its methods as a debt.
#
# A **declaration** and not a call: the pattern needs a return type before the
# name, so `save_descriptors(saved);` -- one identifier, then the paren -- does
# not match, and neither does `int32_t n = f(x);`, which has an operator where
# the paren would have to be.  Proven both ways in `selftest` below.
FUNC = re.compile(r'^%s%s[A-Za-z_][\w:]*(?:\s*<[^;()]*>)?[\s*&]+\**\w+'
                  r'\s*\([^;]*\)(?:\s*const)?\s*;$' % (_QUAL, _KW))
TMPL = re.compile(r'^template\s*<[^;]*>$')


def not_a_statement(line):
    """A field, a prototype or a template header -- none of them statements."""
    return bool(DECL.match(line) or FUNC.match(line) or TMPL.match(line))


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
        if all(not_a_statement(b) for b in block):
            continue
        windows[normal('\n'.join(block))].append(i)
    out, taken = [], set()
    for _, ats in sorted(windows.items(), key=lambda kv: -len(kv[1])):
        # Occurrences that overlap each other are one repetitive block, not two
        # copies of one.  `bmp->biClrImportant = 0;` and the three field zeroings
        # under it put a window at every line of themselves, and every window
        # normalises to the same thing, so four assignments in a row were being
        # reported as a three-line run written twice.  Keep a non-overlapping
        # subset; a group that has none is one block and not duplication.
        spaced, last = [], None
        for a in sorted(ats):
            if last is None or a - last >= WINDOW:
                spaced.append(a)
                last = a
        ats = spaced
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


# **Proof that the skip did not turn the tool off.**  A skip is the one change
# to a counting tool that can make it answer zero for the wrong reason, so both
# halves are planted: six statements written twice still count, six prototypes
# written twice do not, and the four lines that a prototype is easy to confuse
# with are checked one at a time.
#
#     python3 tools/dupblock.py --selftest
_SELFTEST = [
    # A call is a statement, whatever it looks like.  So is an initialised
    # declaration, and so is anything with an operator in it.
    ('save_descriptors(saved);', False),
    ('int32_t n = weight_pair_cost(img, p0, a, b, 4, 8);', False),
    ('blk->free_workspace();', False),
    ('return alt_model_p1_decode(hdr, out);', False),
    # These four are not.
    ('int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out);', True),
    ('AltP1Block* alt_p1_block_alloc(int32_t width, int32_t height);', True),
    ('template<int32_t f_DEC>', True),
    ('uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height);', True),
]


def selftest():
    import tempfile, os
    bad = 0
    for line, want in _SELFTEST:
        got = not_a_statement(line)
        if got != want:
            bad += 1
            print('  %-14s %s' % ('statement' if want else 'declaration', line))
    # And the whole survey, over a planted file rather than over the regex.
    # Long enough to clear MIN_CHARS: a three-line window of `x = f(k);` is
    # thirty characters and this tool ignores it, so a first version of this
    # plant reported no copy for the right reason and would have passed a
    # broken skip.
    # The two wrappers differ in *shape* and not only in their names, because
    # the normaliser renames identifiers away: `void a() {` twice is itself a
    # duplicate run, and a first version of this plant reported eight lines of
    # copy for the braces around the declarations rather than for anything in
    # them.  A second parameter is enough to tell the two apart.
    open_a = 'void first_holder(int32_t k) {'
    open_b = 'void second_holder(int32_t k, int32_t j) {'
    stmt = ['  plane_cost[0] = weight_pair_cost(img, plane0, 1, 2, 4, 8);',
            '  plane_cost[1] = weight_pair_cost(img, plane0, 2, 3, 4, 8);',
            '  plane_cost[2] = weight_pair_cost(img, plane0, 3, 1, 4, 8);',
            '  other_cost[0] = weight_pair_cost(img, plane1, 1, 2, 8, 4);',
            '  other_cost[1] = weight_pair_cost(img, plane1, 2, 3, 8, 4);',
            '  other_cost[2] = weight_pair_cost(img, plane1, 3, 1, 8, 4);']
    decl = ['  int32_t alpha_of_plane(BmfImage* img, int32_t plane_index);',
            '  int32_t beta_of_plane(BmfImage* img, int32_t plane_index);',
            '  int32_t gamma_of_plane(BmfImage* img, int32_t plane_index);',
            '  int32_t delta_of_stream(BmfImage* hdr, int32_t plane_count);',
            '  int32_t epsilon_of_stream(BmfImage* hdr, int32_t plane_count);',
            '  int32_t zeta_of_stream(BmfImage* hdr, int32_t plane_count);']
    for planted, want_copy in ((stmt, True), (decl, False)):
        text = '\n'.join([open_a] + planted + ['}', '', open_b] + planted + ['}'])
        fd, path = tempfile.mkstemp(suffix='.cpp', dir='.')
        os.write(fd, text.encode())
        os.close(fd)
        try:
            copied = sum(n * (len(ats) - 1) for n, ats, _ in survey(path))
        finally:
            os.unlink(path)
        if bool(copied) != want_copy:
            bad += 1
            print('  planted %s: %d lines of copy, wanted %s'
                  % ('statements' if want_copy else 'declarations', copied,
                     'some' if want_copy else 'none'))
    print('%d of %d self-tests disagree' % (bad, len(_SELFTEST) + 2))
    return 1 if bad else 0


def main():
    if '--selftest' in sys.argv:
        sys.exit(selftest())
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/dupblock.py bmf.cpp [--list|--selftest]')
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
