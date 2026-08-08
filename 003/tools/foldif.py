#!/usr/bin/env python3
"""Fold an `if` whose condition is now a known constant.

    python3 foldif.py subs1.hpp <line> true|false ["note"]

<line> is the 1-based line of the `if (...)`.  With `true` the then-branch is
kept and any `else` deleted; with `false` the then-branch is deleted and the
else-branch promoted.  Braces are matched, so the branches may be blocks or
single statements.

The then-branch keeps its braces when it had them: Hex-Rays output is full of
labels jumped to from inside these blocks, and a bare compound statement is
always safe where an `if` block was.
"""
import sys


def block_end(lines, i):
    """Index of the last line of the statement starting at line i."""
    s = lines[i].split('//')[0]
    if s.strip().startswith('{') or s.rstrip().endswith('{'):
        d = 0
        j = i
        while j < len(lines):
            t = lines[j].split('//')[0]
            d += t.count('{') - t.count('}')
            if d == 0:
                return j
            j += 1
        raise SystemExit('unbalanced block at %d' % (i + 1))
    # a single statement, possibly spanning lines until its ';'
    j = i
    while j < len(lines) and not lines[j].split('//')[0].rstrip().endswith(';'):
        j += 1
    return j


def main():
    path, line, truth = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    note = sys.argv[4] if len(sys.argv) > 4 else None
    lines = open(path).read().split('\n')
    i = line - 1
    if 'if' not in lines[i]:
        raise SystemExit('line %d is not an if: %r' % (line, lines[i]))
    indent = ' ' * (len(lines[i]) - len(lines[i].lstrip()))

    then_start = i + 1
    then_end = block_end(lines, then_start)
    has_else = (then_end + 1 < len(lines)
                and lines[then_end + 1].strip().startswith('else'))
    if has_else:
        e = then_end + 1
        rest = lines[e].strip()[4:].strip()
        if rest:                       # `else if (...)` or `else stmt;`
            else_start, else_end = e, block_end(lines, e)
            if rest.startswith('if'):
                # keep the chain: the else-if becomes a plain if
                else_body = [lines[e].replace('else ', '', 1)] + \
                            lines[e + 1:block_end(lines, e + 1) + 1]
                else_end = block_end(lines, e + 1)
            else:
                else_body = [indent + rest]
        else:
            else_start, else_end = e, block_end(lines, e + 1)
            else_body = lines[e + 1:else_end + 1]
    else:
        else_end, else_body = then_end, []

    head = [indent + '// ' + note] if note else []
    if truth == 'true':
        kept = lines[then_start:then_end + 1]
    elif truth == 'false':
        kept = else_body
    else:
        raise SystemExit('truth must be true or false')

    out = lines[:i] + head + kept + lines[else_end + 1:]
    open(path, 'w').write('\n'.join(out))
    print('folded line %d to %s: %d lines -> %d'
          % (line, truth, else_end - i + 1, len(head) + len(kept)))


main()
