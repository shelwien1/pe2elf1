#!/usr/bin/env python3
"""The longest function bodies, against a ratchet.

    python3 tools/long.py bmf.cpp             # report
    python3 tools/long.py bmf.cpp --list      # and the twenty longest

This exists because it was run by hand four times in one round.  Every time a
function came apart the next question was "so what is the longest one now", and
the answer was a throwaway regex in a shell.  A census run four times is a
script, which is the rule this directory is built on.

**It counts code lines, not lines.**  `AltP2Block::code_banks` is 180 lines and
73 of them are code; `alt_p2_context` was 377 and 241.  A heavily annotated
function is not the problem this reports -- the problem is a body with two
hundred statements in it, and counting comments would put the two on the same
footing and make the ratchet move whenever somebody explained something.

**The ratchet is the point, not the number.**  `RATCHET` is the largest code
count in the tree when this was last touched.  Reporting "the longest function
is N lines" tells a reader nothing they could act on; reporting that something
is now *longer than the longest was* tells them a body grew back, which is the
failure this is for.  Raising it is a decision someone makes and writes down,
the way `dupblock.py`'s is.

**What a body is** comes from `structs.bodies`, which every tool here that
reads the program should be using and which this one was not.  Its own regex
wanted a whole signature on one line, so the thirty-four definitions whose
parameters wrap were invisible -- `bmp_rle_encode` at 138 lines,
`choose_alpha_plane` at 108, `search_planes` at 99 -- and so was every in-class
method, which is most of the tree.  It reported 190 bodies where the shared
finder reports 331.

Lambdas are not counted separately -- they are part of the body that holds them,
which is the right answer for this question: a hundred-line lambda inside a
twenty-line function is a hundred-and-twenty-line function to read.

**What it is not.**  It is not a claim that a long function should be split.
Six of the ten longest here are long because their content is long -- sixty
filter inputs, eleven-term context words, a table of thresholds -- and breaking
those up moves lines without making anything clearer.  `unnest.py` reports a
shape that is always worth fixing; this reports a number that is worth looking
at.  The difference is why this one has a ratchet and that one has a zero.
"""
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# The largest code-line count in the tree: `alt_p2_context`, and what is left of
# it is the five bank context words.
#
# Those stay together, for the reason the four variants inside `fill_row_inputs`
# do.  Each is an eleven-term `bit_of<>` chain and three of the five come in two
# arms, so the body is eight chains of eleven lines that differ only in their
# terms -- and being able to read them against each other is the whole reason
# the cursors were named `c0`..`c4` and the fields spelled `d1(0)` and `v0(-2)`.
# Five word-builders would each read well alone and the set of them would read
# worse.  This number is where that decision is recorded.
#
# 174 until the census was corrected to see wrapped signatures and in-class
# methods; 171 is the same body measured properly, with the twenty-five
# `*(freq+K)` rewrites and a folded `ctx15` accounting for the three.  Nothing
# the corrected census newly sees comes near it -- `bmp_rle_encode` is the
# largest at 94 -- which is worth saying, because "the ratchet held" is only
# information if the thing that held it could have failed.
RATCHET = 171

def bodies(path):
    """Every body in `path`, measured.

    Through `structs.bodies` and not a regex of its own.  The regex this
    replaced wanted a whole signature on one line, so it never saw the
    thirty-four definitions whose parameters wrap -- `bmp_rle_encode` at 141
    lines, `choose_alpha_plane` at 108, `search_planes` at 99 -- and it never
    saw an in-class method at all.  A census that cannot see a body reports the
    same number as one that looks and finds it short, which is the defect
    `structs.bodies` was written for and says so in its own docstring.
    """
    lines = open(path).read().split('\n')
    out = []
    for a, b, nm, _sig in structs.bodies(lines):
        body = lines[a + 1:b]
        code = [x for x in body
                if x.strip() and not x.strip().startswith('//')]
        out.append((len(code), len(body), path, a + 1, nm))
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    show = '--list' in sys.argv
    if not args:
        raise SystemExit('usage: long.py <file>... [--list]')
    out = []
    for path in args:
        out += bodies(path)
    out.sort(reverse=True)
    if show:
        for code, total, path, line, name in out[:20]:
            print('%5d code %5d total  %-28s %s:%d'
                  % (code, total, name, path, line))
    over = [x for x in out if x[0] > RATCHET]
    for code, total, path, line, name in over:
        print('  over the ratchet: %s at %s:%d is %d code lines, %d over'
              % (name, path, line, code, code - RATCHET))
    print('%d bodies over the %d-line ratchet; longest is %d code lines'
          % (len(over), RATCHET, out[0][0] if out else 0))


main()
