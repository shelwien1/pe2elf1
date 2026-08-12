#!/usr/bin/env python3
"""Check §1's table against what `shape.py` says right now.

    python3 tools/checktable.py [REFACTORING9.md]

§1's table is labelled "`python3 tools/shape.py`, verbatim" and has been wrong
seven times: four because the measure behind a row was wrong, and three because
somebody copied a number into it and the file kept changing underneath.  The
first four needed thought.  The last three are a diff, and a document that
claims to quote a tool should be checked against the tool rather than reread.

Only the last column is compared -- the earlier ones are history and cannot be
recomputed.  A row whose label `shape.py` no longer prints is reported too: a
table quoting a row that no longer exists is the same defect from the other
side.
"""
import re
import sys
import subprocess

# `sweep.sh` hands every tool a copy of the *source* to check, so an argument
# that is not the document is not a mistake -- but it is not nothing either.
# `shape.py` was being run with no path at all, so it read `subs1.hpp` from the
# working directory whatever this was handed, and the check answered about the
# working copy while claiming to answer about the file it was given.  That is
# the exact defect `proven.sh` exists to catch, in the tool that checks the
# table `proven.sh`'s own numbers go into.
DOC = next((a for a in sys.argv[1:] if a.endswith('.md')), 'REFACTORING9.md')
SRC = next((a for a in sys.argv[1:] if not a.endswith('.md')
            and not a.startswith('--')), 'subs1.hpp')
HERE = __file__.rsplit('/', 1)[0]


def live():
    out = subprocess.check_output(
        [sys.executable, HERE + '/shape.py', '--rows', SRC]).decode()
    return dict(l.split('\t', 1) for l in out.split('\n') if '\t' in l)


def quoted():
    txt = open(DOC).read()
    # The last column used to be headed `round 9 end` and was re-taken by
    # every round after it, which made the header the one part of the table
    # nothing checked and nothing was true of.  It says `now`.
    m = re.search(r'```\n\s*round 8\s+round 9\s+now\n(.*?)```', txt, re.S)
    if not m:
        sys.exit('%s: no §1 table found' % DOC)
    rows = []
    for l in m.group(1).split('\n'):
        if not l.strip():
            continue
        cols = re.split(r'\s{2,}', l.strip())
        if len(cols) >= 2:
            rows.append((cols[0].strip(), cols[-1].strip()))
    return rows


if __name__ == '__main__':
    now, bad = live(), 0
    for label, want in quoted():
        # The two rows that are history only: `shape.py` never printed them.
        if label in ('byte offsets on a typed base',):
            continue
        # `shape.py` labels its first row with the source's basename, which is
        # right -- the row should say what was measured -- but it means that
        # against a copy the table's `subs1.hpp lines` matches nothing and the
        # sweep fails on the typography.  Only that one row is remapped, and
        # only when the file being checked is not the one the table quotes.
        if label.endswith(' lines') and label not in now:
            alt = SRC.rsplit('/', 1)[-1] + ' lines'
            if label == 'subs1.hpp lines' and alt in now:
                label = alt
        got = now.get(label)
        if got is None:
            print('%-40s quoted but shape.py prints no such row' % label)
            bad += 1
        # Compare without the spacing: the table is hand-aligned and prints
        # `49/33` where the tool prints `49 / 33`, and a checker that called
        # that a disagreement would be checking the typography.
        elif got.replace(' ', '') != want.replace(' ', ''):
            print('%-40s table says %s, shape.py says %s' % (label, want, got))
            bad += 1
    print('%d rows in §1 disagree with tools/shape.py' % bad)
    sys.exit(1 if bad else 0)
