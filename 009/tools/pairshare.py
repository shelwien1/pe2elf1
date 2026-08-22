#!/usr/bin/env python3
"""Re-measure the encode/decode pairs bmf.cpp says it declined to merge.

    python3 tools/pairshare.py bmf.cpp
    python3 tools/pairshare.py bmf.cpp --all    # print every pair, drifted or not

Nine encode/decode pairs in this program are one `template<int32_t f_DEC>` and five
are two bodies.  What decided each was how much of the two bodies is actually
the same, measured as a longest common subsequence over their non-comment
lines, and the five declines carry that measurement in a comment at the top of
`bmf.cpp` so the next reader can see the decision rather than re-take it.

A measurement written into a comment is a claim, and this tree's rule is that a
claim nothing checks will rot.  All five of these had, by the time this was
written: `predict_med`/`unpredict_med` was recorded at 24 shared lines of 152
and is 10 of 123, and `code_pixel`/`decode_pixel` at 179 of 1229 was 144 of
1036.  Every round that shortens one of these bodies moves its number, and
nothing said so.

The table in the comment is the input, not a copy of it kept here: adding a pair
there is enough, and a pair named there whose bodies this cannot find is
reported rather than skipped.  The percentage is over the two bodies together,
which is what makes 50% the ceiling for two identical bodies -- the same
convention the comment was already using.
"""
import difflib
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

import pairnames                                                  # noqa: E402

# The row shape lives in `pairnames.py` along with the class a row may name,
# because a row is now allowed to say *which* `encode_sample` it means -- there
# are two.  Both tools read the same table, so they read it the same way.
ROW = pairnames.ROW

# The nine that *were* merged, named in the table above the declines as
# `name  a + b`.  Their two bodies no longer exist, so there is nothing to
# re-measure -- but the claim that each is still one template is checkable, and
# it is the half of that comment most likely to go quietly wrong: the tree spelt
# them `template<int f_DEC>` until a type-vocabulary pass made it `int32_t`, and
# the prose said `int` for a round afterwards.
# Two entries per line -- the table is two columns -- so this is `findall` over
# each line and not a match against it.  Anchored to end-of-line first, it found
# one of the nine: the only row that has no second column.
MERGED = re.compile(r'(?:(\w+)::)?(\w+)\s+\d+ \+ \d+')
TEMPLATE = re.compile(r'template\s*<\s*int(?:32_t)?\s+f_DEC\s*>')


def claims(path):
    """(a, b, shared, total, percent, classes) for every row of the table."""
    out = []
    for l in open(path).read().split('\n'):
        r = pairnames.row(l)
        if r:
            out.append(r + (pairnames.row_class(l),))
    return out


def merged(path):
    """The names the table says are one template each."""
    out = []
    for l in open(path).read().split('\n'):
        if l.startswith('//'):
            out.extend(MERGED.findall(l))
    return out


def templates(lines):
    """Every body introduced by a `template<int32_t f_DEC>`, by its name."""
    out = set()
    for i, l in enumerate(lines):
        if not TEMPLATE.search(l):
            continue
        for j in range(i, min(i + 3, len(lines))):
            m = re.search(r'\b(\w+)\s*\(', lines[j].replace('template', ''))
            if m:
                out.add(m.group(1))
                break
    return out


def norm(rows):
    """The lines that are program rather than comment or blank."""
    return [re.sub(r'\s+', ' ', l.split('//')[0].strip())
            for l in rows if l.split('//')[0].strip()]


def pick(defs, name, cls):
    """The definition a row means: the one in `cls` when the row named one.

    A row that names no class and finds more than one definition is ambiguous
    rather than fine -- it would silently measure whichever came first, which
    is the bug `pairnames.py` found in this file's own reader.
    """
    got = defs.get(name)
    if not got:
        return None
    if cls:
        got = [d for d in got if d.cls == cls]
    return got[0] if len(got) == 1 else None


def measure(defs, a, b, classes):
    da, db = pick(defs, a, classes[0]), pick(defs, b, classes[1])
    if da is None or db is None:
        return None
    x, y = norm(da.body), norm(db.body)
    sm = difflib.SequenceMatcher(None, x, y, autojunk=False)
    shared = sum(n for _i, _j, n in sm.get_matching_blocks())
    total = len(x) + len(y)
    return shared, total, int(round(100.0 * shared / total)) if total else 0


def main():
    path = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('-') \
        else 'bmf.cpp'
    rows = claims(path)
    if not rows:
        print('not applicable: %s has no table of declined pairs to check'
              % path)
        return
    lines, _origin = structs.splice(path)
    tmpl = templates(lines)
    lost = [(o, n) for o, n in merged(path) if n not in tmpl]
    for owner, n in lost:
        print('  %-24s the table says one template, and it is not one'
              % ('%s::%s' % (owner, n) if owner else n))
    defs, _declined = pairnames.collect(path)

    drift, missing = 0, 0
    for a, b, shared, total, pct, classes in rows:
        got = measure(defs, a, b, classes)
        if got is None:
            missing += 1
            print('  %-24s/%-24s  named in the table, not found in the unit '
                  '(or named twice and the row does not say which)' % (a, b))
            continue
        if got != (shared, total, pct):
            drift += 1
            print('  %-24s/%-24s  says %d of %d (%d%%), is %d of %d (%d%%)'
                  % (a, b, shared, total, pct, got[0], got[1], got[2]))
        elif '--all' in sys.argv:
            print('  %-24s/%-24s  %d of %d (%d%%)'
                  % (a, b, got[0], got[1], got[2]))
    print('%d of %d declined pairs have drifted, %d named but not found; '
          '%d of %d merged pairs are no longer one template'
          % (drift, len(rows), missing, len(lost), len(merged(path))))


if __name__ == '__main__':
    main()
