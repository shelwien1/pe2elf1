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
# that is not the document is the sweep asking politely and is ignored.
DOC = next((a for a in sys.argv[1:] if a.endswith('.md')), 'REFACTORING9.md')
HERE = __file__.rsplit('/', 1)[0]


def live():
    out = subprocess.check_output(
        [sys.executable, HERE + '/shape.py', '--rows']).decode()
    return dict(l.split('\t', 1) for l in out.split('\n') if '\t' in l)


def quoted():
    txt = open(DOC).read()
    m = re.search(r'```\n\s*round 8\s+round 9\s+round 9 end\n(.*?)```', txt, re.S)
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
