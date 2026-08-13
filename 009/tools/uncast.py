#!/usr/bin/env python3
"""Delete the casts the compiler calls useless.

    python3 tools/uncast.py            # one pass; re-run until it says 0

REFACTORING5.md §2.2 and §4.  A cast to the type the operand already has is
noise, and `-Wuseless-cast` says exactly where each one is -- file, line and
column, the column being the `(` of the cast.  So this does not need a grammar:
it needs the compiler's answer and a paren match.

Removing one cast can make the next one useless -- `(uint8_t *)(uint8_t *)p` is
two warnings, and GCC reports the outer -- so run it until the count stops
falling.  It never removes a cast the compiler did not name, which is what
keeps it from touching the reinterpretations that are the program.
"""
import re
import subprocess
import sys

CXX = ('g++ -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing '
       '-fpermissive -fno-rtti -fno-exceptions -DNDEBUG -U_FORTIFY_SOURCE '
       '-D_FORTIFY_SOURCE=0 -Wuseless-cast -fdiagnostics-plain-output '
       '-fsyntax-only bmf.cpp').split()


def sites(path):
    out = subprocess.run(CXX, capture_output=True, text=True).stderr
    hits = []
    for l in out.split('\n'):
        m = re.match(r'%s:(\d+):(\d+): warning: useless cast' % re.escape(path), l)
        if m:
            hits.append((int(m.group(1)), int(m.group(2))))
    return hits


def strip(line, col):
    """Remove the cast whose `(` is at `col` (1-based)."""
    i = col - 1
    if i >= len(line) or line[i] != '(':
        return None
    depth, j = 0, i
    while j < len(line):
        if line[j] == '(':
            depth += 1
        elif line[j] == ')':
            depth -= 1
            if depth == 0:
                break
        j += 1
    if j >= len(line):
        return None
    inner = line[i + 1:j]
    # only a type, or this is a call and not a cast
    if not re.fullmatch(r'\s*(?:const\s+)?[\w:]+(?:\s*\*+)*\s*&?\s*', inner):
        return None
    return line[:i] + line[j + 1:]


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    done = 0
    # right to left on each line, so an earlier column keeps its position
    for ln, col in sorted(sites(path), reverse=True):
        new = strip(lines[ln - 1], col)
        if new is not None:
            lines[ln - 1] = new
            done += 1
    open(path, 'w').write('\n'.join(lines))
    print('%d useless casts removed' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
