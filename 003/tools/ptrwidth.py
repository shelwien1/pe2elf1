#!/usr/bin/env python3
"""Classify the pointers this file still puts through a 32-bit integer.

    BMF_STRICT=1 BMF_BITS=64 ./build.sh     # writes strict64.log
    python3 tools/ptrwidth.py subs1.hpp             # the census
    python3 tools/ptrwidth.py subs1.hpp --roundtrip # list one kind
    python3 tools/ptrwidth.py subs1.hpp --apply     # rewrite the safe kind

`BMF_STRICT=1 ./build.sh` has said 0 for six rounds, and on i386 that is true:
`(int32_t)p` is not a narrowing when a pointer is four bytes wide, so the whole
class is invisible to it.  Ask the same question of `-m64` and 112 sites answer.
They are not all the same defect, and the reason this is a classifier rather
than a rewrite is that only one of the kinds can be fixed without reading:

  roundtrip   `(T *)((int32_t)e)` where `e` is already a pointer.  The integer
              never reaches a variable -- Hex-Rays emitted it because MSVC's
              `mov` had no type -- so deleting it is the same value on i386 and
              the right value everywhere else.  This is what `--apply` does.
  slot        the cast is stored: `x = (int32_t)p`.  The fix is the *slot*'s
              type, and which slot it is is the useful part of the report.
  arg         a pointer handed to an `int32_t` parameter.  The fix is the
              callee's signature, so these come in groups and are listed with
              the callee named.
  arith       the cast is inside a comparison or an expression.  Some of these
              are address arithmetic that wants a pointer difference and some
              are a small integer that was living in a pointer slot; there is
              no rule, and this column is the reading list.

The kind is decided from the source text around the column gcc reports, and the
*fact that the operand is a pointer* comes from gcc rather than from a guess
here -- which is the whole reason this reads a log instead of grepping.  A
regex that matched `(T *)(int32_t)x` on its own would also match the one shape
that must never be touched: `x` an integer, where the cast is a real narrowing
and removing it changes the value.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import buildlog                                                   # noqa: E402

LOG = 'strict64.log'
SITE = re.compile(r'^subs1\.hpp:(\d+):(\d+): error: cast from '
                  r"'([^']+)' .*loses precision")
INT = re.compile(r'\((?:u?int32_t|unsigned int|int)\)')
# `(T *)` or `(T **)`, allowing the space Hex-Rays puts before the star.
PTRCAST = re.compile(r'\(\s*[A-Za-z_]\w*\s*\*+\s*\)\s*\(?\s*$')
ASSIGN = re.compile(r'([A-Za-z_][\w.\[\]>*-]*)\s*=\s*\(?\s*$')
CALL = re.compile(r'([A-Za-z_]\w*)\s*\([^()]*$')


def sites(path, log=LOG):
    """[(line, col, from-type)] — the truncations gcc found, deduplicated.

    gcc reports the outer cast of `(uint32_t)(a - (uint32_t)b)` twice, at two
    columns on one line; both are real and both are listed.

    `path` and not the literal `subs1.hpp`: `sweep.sh` hands every tool a copy,
    and a stamp check against the original would pass while the line numbers
    were applied to something else.  The copy has the same contents, so the
    cksum matches and the answer is the same -- but only because it is checked
    against the file actually being read.
    """
    rows, note = buildlog.read(log, path)
    if note:
        return None, note
    out = []
    for r in rows:
        m = SITE.match(r)
        if m:
            out.append((int(m.group(1)), int(m.group(2)), m.group(3)))
    return sorted(set(out)), ''


def classify(lines, line, col):
    """(kind, detail) for the cast at 1-based (line, col)."""
    text = lines[line - 1]
    before = text[:col - 1]
    m = INT.match(text[col - 1:])
    if not m:
        return 'unmatched', ''
    if PTRCAST.search(before):
        return 'roundtrip', before.strip()
    m2 = ASSIGN.search(before)
    if m2:
        return 'slot', m2.group(1)
    m3 = CALL.search(before) or CALL.search(re.sub(r'\([^()]*\)', '', before))
    if m3 and m3.group(1) not in ('if', 'while', 'for', 'return', 'switch'):
        return 'arg', m3.group(1)
    return 'arith', text.strip()[:60]


def apply(lines, rows):
    """Delete the integer cast on every `roundtrip` row.

    Right to left within a line: two casts on one line and removing the first
    moves the second's column.
    """
    done = 0
    for line, col, _ in sorted(rows, key=lambda r: (-r[0], -r[1])):
        text = lines[line - 1]
        m = INT.match(text[col - 1:])
        if not m or classify(lines, line, col)[0] != 'roundtrip':
            continue
        lines[line - 1] = text[:col - 1] + text[col - 1 + m.end():]
        done += 1
    return done


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    rows, note = sites(path)
    if note:
        print('0 pointers through a 32-bit integer (%s)' % note)
        return
    kinds = collections.defaultdict(list)
    for line, col, ty in rows:
        kind, detail = classify(lines, line, col)
        kinds[kind].append((line, col, ty, detail))
    for want in ('--roundtrip', '--slot', '--arg', '--arith', '--unmatched'):
        if want in sys.argv:
            k = want[2:]
            for line, col, ty, detail in kinds[k]:
                print('%6d:%-3d %-16s %s' % (line, col, ty, detail))
            print('%d %s' % (len(kinds[k]), k))
            return
    if '--apply' in sys.argv:
        n = apply(lines, rows)
        open(path, 'w').write('\n'.join(lines))
        print('%d round-trip casts deleted' % n)
        return
    # The slot column is the interesting one, so name the busiest slots: one
    # slot retyped closes every site that names it, and the count is how many.
    slots = collections.Counter(d for _, _, _, d in kinds['slot'])
    for k in ('roundtrip', 'slot', 'arg', 'arith', 'unmatched'):
        print('%-10s %d' % (k, len(kinds[k])))
    if slots:
        print('busiest slots: %s'
              % ', '.join('%s %d' % (n, c) for n, c in slots.most_common(8)))
    print('%d pointers through a 32-bit integer' % len(rows))


if __name__ == '__main__':
    main()
