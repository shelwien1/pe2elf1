#!/usr/bin/env python3
"""Re-measure the encode/decode pairs bmf.cpp says it declined to merge.

    python3 tools/pairshare.py bmf.cpp
    python3 tools/pairshare.py bmf.cpp --all    # print every pair, drifted or not

Nine encode/decode pairs in this program are one `template<int f_DEC>` and five
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

ROW = re.compile(r'^//\s+(\w+)\s*/\s*(\w+)\s+(\d+) of (\d+) \((\d+)%\)\s*$')


def claims(path):
    """(a, b, shared, total, percent) for every row of the table."""
    out = []
    for l in open(path).read().split('\n'):
        m = ROW.match(l)
        if m:
            out.append((m.group(1), m.group(2), int(m.group(3)),
                        int(m.group(4)), int(m.group(5))))
    return out


def norm(rows):
    """The lines that are program rather than comment or blank."""
    return [re.sub(r'\s+', ' ', l.split('//')[0].strip())
            for l in rows if l.split('//')[0].strip()]


def measure(bodies, a, b):
    if a not in bodies or b not in bodies:
        return None
    x, y = norm(bodies[a]), norm(bodies[b])
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
    bodies = {}
    for a, b, n, _sig, _d in structs.defs(lines):
        bodies.setdefault(n, lines[a:b + 1])

    drift, missing = 0, 0
    for a, b, shared, total, pct in rows:
        got = measure(bodies, a, b)
        if got is None:
            missing += 1
            print('  %-24s/%-24s  named in the table, not in the unit' % (a, b))
            continue
        if got != (shared, total, pct):
            drift += 1
            print('  %-24s/%-24s  says %d of %d (%d%%), is %d of %d (%d%%)'
                  % (a, b, shared, total, pct, got[0], got[1], got[2]))
        elif '--all' in sys.argv:
            print('  %-24s/%-24s  %d of %d (%d%%)'
                  % (a, b, got[0], got[1], got[2]))
    print('%d of %d declined pairs have drifted, %d named but not found'
          % (drift, len(rows), missing))


if __name__ == '__main__':
    main()
