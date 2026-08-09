#!/usr/bin/env python3
"""Delete the locals nothing uses, with the compiler saying which.

    ./build.sh >/dev/null && python3 tools/unused.py subs1.hpp

Folding the six mode switches deleted about a third of the file, and every
deletion left behind the declarations of whatever that code had been using.
`model_planes` alone carried seventeen: `Sizea_1`, `Sizea_2`, `Size_1`,
`Size_2`, `Blockb_2` and a dozen `vNN`, all named for a `-E` block that has not
existed since Phase 2.1.

The list comes from `g++ -Wunused-variable` rather than from a pattern here.
That matters more than it sounds: a declaration in this file can span three
lines, share a type with names that *are* used, be an array, a pointer, an
`alignas(16)` buffer or a reference to a `__frame` member, and a scanner that
gets any of those wrong deletes something live.  The compiler already has the
answer and cannot be wrong about it, so this reads the warnings and edits
exactly what they point at.

Warnings are read from stdin or from a file given with `--from`; with neither,
the compile is run.  A declaration statement is rebuilt from its declarators
after the named ones are dropped, wrapped at the width the file already uses,
and deleted outright when nothing is left.
"""
import re
import subprocess
import sys

CXX = ('g++ -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing '
       '-fpermissive -fno-rtti -fno-exceptions -O2 -DNDEBUG -U_FORTIFY_SOURCE '
       '-D_FORTIFY_SOURCE=0 -Wunused-variable -fsyntax-only bmf.cpp')
WARN = re.compile(r"^(\S+):(\d+):\d+: warning: unused variable '([^']+)'")


def warnings(argv):
    if '--from' in argv:
        text = open(argv[argv.index('--from') + 1]).read()
    elif not sys.stdin.isatty():
        text = sys.stdin.read()
    else:
        text = subprocess.run(CXX, shell=True, capture_output=True,
                              text=True).stderr
    out = {}
    for l in text.split('\n'):
        m = WARN.match(l)
        if m:
            out.setdefault(int(m.group(2)), set()).add(m.group(3))
    return out


def statement(lines, i):
    """The line range of the declaration statement that starts at line i."""
    j = i
    while j < len(lines) and ';' not in lines[j].split('//')[0]:
        j += 1
    return i, j


def declarators(text):
    """Split `int a, *b[4], c` on the commas that separate declarators."""
    out, depth, at = [], 0, 0
    for k, c in enumerate(text):
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == ',' and depth == 0:
            out.append(text[at:k])
            at = k + 1
    out.append(text[at:])
    return [d.strip() for d in out]


def name_of(d):
    """The identifier a declarator declares: `*p[4]` -> `p`."""
    m = re.match(r'^[*&\s]*([A-Za-z_][A-Za-z0-9_]*)', d)
    return m.group(1) if m else None


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    want = warnings(sys.argv)
    if not want:
        print('no unused-variable warnings')
        return 0
    lines = open(path).read().split('\n')

    # One statement can hold names reported against several of its lines, so
    # collect by statement before editing, and edit from the bottom up.
    stmts = {}
    for ln in want:
        i = ln - 1
        while i > 0 and ';' not in lines[i - 1].split('//')[0]:
            i -= 1                      # walk back to the first line
        a, b = statement(lines, i)
        stmts.setdefault((a, b), set()).update(want[ln])

    gone = kept = 0
    for (a, b) in sorted(stmts, reverse=True):
        text = '\n'.join(lines[a:b + 1])
        code, _, tail = text.partition(';')
        indent = re.match(r'^\s*', lines[a]).group(0)
        # The type is everything up to the first declarator; declarators may
        # carry `*`, `&`, `[n]` and, for the frame aliases, an initialiser.
        m = re.match(r'^\s*((?:const\s+|static\s+|alignas\(\d+\)\s+)*'
                     r'[A-Za-z_][A-Za-z0-9_]*(?:\s*::\s*[A-Za-z_][A-Za-z0-9_]*)*)'
                     r'\s+(.*)$', code.replace('\n', ' '), re.S)
        if not m:
            continue
        ty, rest = m.group(1), m.group(2)
        keep = [d for d in declarators(rest)
                if name_of(d) not in stmts[(a, b)]]
        dropped = len(declarators(rest)) - len(keep)
        if not dropped:
            continue
        gone += dropped
        if not keep:
            del lines[a:b + 1]
            continue
        kept += 1
        out, cur = [], '%s%s ' % (indent, ty)
        for k, d in enumerate(keep):
            piece = d + (',' if k + 1 < len(keep) else ';')
            if len(cur) + len(piece) > 78 and cur.strip() != ty:
                out.append(cur.rstrip())
                cur = indent + ' ' * (len(ty) + 1)
            cur += piece + ' '
        out.append(cur.rstrip())
        lines[a:b + 1] = out

    open(path, 'w').write('\n'.join(lines))
    print('%d declarations deleted, %d rewritten, %d names gone'
          % (len(stmts) - kept, kept, gone))
    return 0


if __name__ == '__main__':
    sys.exit(main())
