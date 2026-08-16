#!/usr/bin/env python3
"""Two names for one value, told apart by a trailing `a`/`b`.

    python3 tools/abpair.py                 # the pairs that are one value
    python3 tools/abpair.py --list          # every a/b family, one per line

CLEANER.md Phase 5.  `alt_p2_context` had `q10a` and `q10b`: one value, read
before a call and again after it, which the decompiler could not prove was the
same value and so gave two names.  They are gone, and the phase asks whether
the shape is anywhere else.

**The answer is no, and the reason is worth more than the answer.**  54 `a`/`b`
families are left in the tree and every one of them is two genuinely different
things -- `plane_a` and `plane_b` are two planes, `tbl64a` and `tbl64b` two
tables, `ofs_ul_a` and `ofs_ul_b` two offsets into two images.  A sweep that
reported those as duplicates would be asking for 54 renames that each lose a
distinction, which is why this looks at what the names are *assigned* and not
at how they are spelled:

  * `b = a;` or `a = b;` -- one is a copy of the other, or
  * both assigned from the same expression, everywhere they are assigned.

Two survive that and neither is the pattern.  `search_filter` measures
`8*(out_cursor-coded_buf)` into `bits_a` at one point and `bits_b` at another,
and runs one plane loop with `pl_a` and a second with `pl_b`; the expressions
match because the measurement is the same measurement, taken twice, of two
different states.  They are reported so that the next reader can re-take the
judgement rather than re-derive it.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

KNOWN = {
    ('search_filter', 'bits_'): 'two cost measurements, at two points',
    ('search_filter', 'pl_'): 'two plane loops, one counter each',
}


def families(lines, a, b):
    body = lines[a:b + 1]
    names = set()
    for l in body:
        names.update(re.findall(r'(?<![\w.>])([A-Za-z_]\w*)(?![\w])', l))
    out = []
    for n in sorted(names):
        m = re.match(r'^(.*[a-z0-9_])a$', n)
        if not m or (m.group(1) + 'b') not in names:
            continue
        stem = m.group(1)
        rhs = {}
        for x in (stem + 'a', stem + 'b'):
            rhs[x] = [re.match(r'^\s*%s\s*=\s*(.*);\s*$' % x, l).group(1)
                      for l in body if re.match(r'^\s*%s\s*=[^=]' % x, l)
                      and re.match(r'^\s*%s\s*=\s*(.*);\s*$' % x, l)]
        A, B = rhs[stem + 'a'], rhs[stem + 'b']
        same = B == [stem + 'a'] or A == [stem + 'b'] or (A and A == B)
        out.append((stem, bool(same), A, B))
    return out


def main():
    lines, _origin = structs.splice('bmf.cpp')
    n_fam = n_same = n_new = 0
    for a, b, nm, _sig, _d in structs.defs(lines):
        for stem, same, A, B in families(lines, a, b):
            n_fam += 1
            if not same:
                if '--list' in sys.argv:
                    print('  %-24s %sa / %sb   two things' % (nm, stem, stem))
                continue
            n_same += 1
            why = KNOWN.get((nm, stem))
            n_new += why is None
            print('  %-24s %sa = %s ; %sb = %s   %s'
                  % (nm, stem, A, stem, B, why or '** not accounted for **'))
    # The finding first and the census after it.  `sweep.sh` reads the leading
    # number as the answer, and it used to read `n_fam` -- 28 families, which
    # is a count of what exists and not of what is wrong.  It passed only
    # because the old rule accepted a zero anywhere on the line.
    print('%d a/b pairs not accounted for and %d assigned the same value, '
          'in %d families' % (n_new, n_same, n_fam))
    return 0


if __name__ == '__main__':
    sys.exit(main())
