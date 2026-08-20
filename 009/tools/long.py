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

**What a body is.**  A line at any indentation that is a return type, a name and
a parameter list ending in `{`, whose matching `}` is at the same indentation.
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
import re
import sys

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
RATCHET = 174

DEF = re.compile(r'^(\s*)((?:[A-Za-z_][\w:<>,*&\s]*[\s*&])?'
                 r'([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?))\s*\(.*\)\s*(const\s*)?\{\s*$')
# A line that looks like a definition and is not one.
KEYWORDS = ('if(', 'for(', 'while(', 'switch(', 'return ', 'else', 'catch(',
            '= {', 'sizeof', 'static_assert')


def bodies(path):
    lines = open(path).read().split('\n')
    out = []
    for i, l in enumerate(lines):
        if l.lstrip().startswith('//') or any(k in l for k in KEYWORDS):
            continue
        m = DEF.match(l)
        if not m:
            continue
        close = m.group(1) + '}'
        for j in range(i + 1, len(lines)):
            if lines[j] == close:
                body = lines[i + 1:j]
                code = [x for x in body
                        if x.strip() and not x.strip().startswith('//')]
                out.append((len(code), len(body), path, i + 1, m.group(3)))
                break
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
