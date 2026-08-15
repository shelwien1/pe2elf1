#!/usr/bin/env python3
"""Delete the casts that cast a thing to the type it already has.

    python3 tools/decast.py subs1.hpp [--from warnings.txt]

Same arrangement as `tools/unused.py`, and for the same reason: the list comes
from the compiler, not from a pattern here.  `-Wuseless-cast` fires only when
the target type is *identical* to the operand's, so removing one cannot change
what an expression means -- which is a much stronger guarantee than any textual
rule about `(uint32_t)(...)` could give, in a file where nearly six thousand
casts are load-bearing.

The warnings carry a column, and the cast starts there, so each is removed by
balancing one pair of parentheses from that point.  Several on a line are taken
right to left so the earlier columns still line up.

What is left after this is not noise: 5700-odd casts remain and they are how the
decompilation says what a byte range is.  This only removes the ones that say
nothing.

One warning is deliberately not acted on, and it is the reason to be careful
here at all -- `-Wuseless-cast` answers for the target it was run against, and
this file is built for two:

    char *p = (char *)((raw + (BMF_PAGE - 1)) & ~(uintptr_t)(BMF_PAGE - 1));

`uintptr_t` is `unsigned int` at 32 bits, so that cast is a no-op and the
warning is right; at 64 bits it is eight bytes and the cast is the whole point.
It lives in `bmf.cpp`, which is why this tool edits only the file it is given --
and why the 64-bit syntax-only compile is worth checking either side of a run.
"""
import re
import subprocess
import sys

CXX = ('g++ -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing '
       '-fpermissive -fno-rtti -fno-exceptions -O2 -DNDEBUG -U_FORTIFY_SOURCE '
       '-D_FORTIFY_SOURCE=0 -Wuseless-cast -fdiagnostics-plain-output '
       '-fsyntax-only bmf.cpp')
WARN = re.compile(r'^(\S+):(\d+):(\d+): warning: useless cast to type')


def warnings(argv, path):
    if '--from' in argv:
        text = open(argv[argv.index('--from') + 1]).read()
    else:
        text = subprocess.run(CXX, shell=True, capture_output=True,
                              text=True).stderr
    out = {}
    for l in text.split('\n'):
        m = WARN.match(l)
        if m and m.group(1).endswith(path.rsplit('/', 1)[-1]):
            out.setdefault(int(m.group(2)), []).append(int(m.group(3)))
    return out


def cast_span(line, col):
    """The `(type)` at 1-based column `col`, or None if that is not one."""
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
    else:
        return None
    inner = line[i + 1:j]
    # a cast, not a parenthesised expression: a type name, stars, and an
    # optional `&` -- the frame aliases are spelled `(int16_t *&)frame.v282`,
    # and leaving that form out skipped five of the seventy
    if not re.fullmatch(r'\s*(?:const\s+|unsigned\s+|signed\s+)*'
                        r'[A-Za-z_][A-Za-z0-9_]*(?:\s*::\s*[A-Za-z_][A-Za-z0-9_]*)*'
                        r'\s*\**\s*&?\s*', inner):
        return None
    return i, j + 1


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    want = warnings(sys.argv, path)
    if not want:
        print('no useless casts')
        return 0
    lines = open(path).read().split('\n')
    gone = skipped = 0
    for ln in sorted(want):
        i = ln - 1
        if i >= len(lines):
            continue
        for col in sorted(want[ln], reverse=True):
            span = cast_span(lines[i], col)
            if not span:
                skipped += 1
                continue
            a, b = span
            lines[i] = lines[i][:a] + lines[i][b:]
            gone += 1
    open(path, 'w').write('\n'.join(lines))
    print('%d casts removed%s'
          % (gone, ', %d not at a cast and left' % skipped if skipped else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
