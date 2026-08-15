#!/usr/bin/env python3
"""One idea written out as an expression in more than one place.

    python3 tools/sameexpr.py bmf.cpp             # report
    python3 tools/sameexpr.py bmf.cpp --list      # and every copy

    grad[0] = lists[3].match[0]+lists[2].match[0]+lists[1].match[0]
             +lists[0].match[0]+lists[4].match[0]-5;

Four copies of that, two in each direction of the main model, and nothing in
this directory could see them.  `undup.py` compares the two arms of an `if`,
and the block-duplication pass REFACTOR2 phase 3 ran wants six or more
*identical lines* -- so a formula that is one line, and whose copies differ in
the name of the pointer they walk, is invisible to both.

**The rule.**  Take the right-hand side of every assignment of forty characters
or more that has three or more `+` in it, at any bracket depth.  Rename its
identifiers to
`B0`, `B1`, ... in order of first appearance.  Two expressions are the same
idea when those normal forms are equal.  Report every group of two or more.

The renaming is what makes it useful: `up1[3].match[0]+...` and
`up2[3].match[0]+...` are the same formula over different rows, and comparing
the text would say they are not.  It is also what makes it a *report* and not
an apply: whether two copies should become one function, and what that function
should be called, is a reading job.  Six groups came out this way --
`match_seed`, `match_seed_split`, `FreqRec::resum`, `AltP2Block::nlms_step`,
`AltP1Block::seed_activity` -- and the last of those turned out to hide a real
difference between its three callers rather than a duplication.

**What it declines.**

  * **fewer than three `+`.**  Two terms is not a formula, it is two terms, and
    at that length the normal form matches things that have nothing to do with
    each other.  Counting only *top-level* `+` was tried and was wrong: the
    three prediction residuals it is supposed to keep have every one of their
    additions inside brackets, so the strict rule reported zero and the
    declines below had nothing to attach to.
  * **under forty characters.**  Same reason from the other end.
  * **the loop headers `for( int32_t j = 0; ... )`.**  The rule sees the `=` in
    a `for` and takes the rest of the header as an expression.  Four such
    groups reported, all of them re-rolled loops that `reroll.py` had just
    written and that are the same by construction.

**What is left, and why each one stays.**  The groups below are real and are
not going to become one function; the reasons are the finding.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

MIN_LEN = 40
MIN_PLUS = 3

# Kept, with the reason.  Keyed by the first file:line of the group.
DECLINED = {
    'model.inc': 'writing one decoded row out through the symbol table, at '
                 'four bytes a sample in `decode_symbol_list`\'s caller and at '
                 'two in `unmodel_plane`\'s.  The expression is the same and '
                 'the destination type is not, so sharing it means a template '
                 'over the output width to save one line at each of two sites',
}


def split_top(e, op='+'):
    """`e`'s top-level `+` terms."""
    out, d, start = [], 0, 0
    for i, c in enumerate(e):
        if c in '([':
            d += 1
        elif c in ')]':
            d -= 1
        elif c == op and d == 0:
            out.append(e[start:i])
            start = i + 1
    out.append(e[start:])
    return [t.strip() for t in out]


def normal(e):
    """`e` with its identifiers renamed to B0, B1, ... in order of appearance."""
    seen = {}

    def sub(m):
        n = m.group(0)
        if n[0].isdigit():
            return n
        if n not in seen:
            seen[n] = 'B%d' % len(seen)
        return seen[n]
    return re.sub(r'[A-Za-z_]\w*', sub, e)


def survey(path):
    lines, origin = structs.splice(path)
    groups = collections.defaultdict(list)
    for i, l in enumerate(lines):
        c = l.split('//')[0]
        if '=' not in c or ';' not in c or re.match(r'\s*for\s*\(', c.lstrip()):
            continue
        rhs = c.split('=', 1)[1].rsplit(';', 1)[0].strip()
        if len(rhs) < MIN_LEN or rhs.count('+') < MIN_PLUS:
            continue
        groups[normal(rhs)].append((origin[i], rhs))
    return [v for v in groups.values() if len(v) > 1]


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/sameexpr.py bmf.cpp [--list]')
    found = survey(sys.argv[1])
    kept, live = [], []
    for g in found:
        (kept if g[0][0][0] in DECLINED else live).append(g)
    if '--list' in sys.argv:
        for g in live + kept:
            note = DECLINED.get(g[0][0][0])
            print('%d copies%s' % (len(g), '  -- kept: ' + note if note else ''))
            for o, e in g:
                print('    %-24s %s' % ('%s:%d' % o, e[:72]))
    print('%d expressions written out in more than one place, in %d groups; '
          '%d groups kept with a reason'
          % (sum(len(g) for g in live), len(live), len(kept)))


if __name__ == '__main__':
    main()
