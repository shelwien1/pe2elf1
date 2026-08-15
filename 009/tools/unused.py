#!/usr/bin/env python3
"""Delete the locals nothing uses, with the compiler saying which.

    ./build.sh >/dev/null && python3 tools/unused.py subs1.hpp
    ./build.sh >/dev/null && python3 tools/unused.py subs1.hpp --all

Folding the six mode switches deleted about a third of the file, and every
deletion left behind the declarations of whatever that code had been using.
`model_planes` alone carried seventeen: `Sizea_1`, `Sizea_2`, `Size_1`,
`Size_2`, `Blockb_2` and a dozen `vNN`, all named for a `-E` block that has not
existed since Phase 2.1.

The list comes from `g++ -Wunused-variable` rather than from a pattern here.
That matters more than it sounds: a declaration in this file can span three
lines, share a type with names that *are* used, be an array, a pointer, an
`alignas(16)` buffer or a reference to a `frame` member, and a scanner that
gets any of those wrong deletes something live.  The compiler already has the
answer and cannot be wrong about it, so this reads the warnings and edits
exactly what they point at.

Warnings come from running the compile, or from a file given with `--from`.  A declaration statement is rebuilt from its declarators
after the named ones are dropped, wrapped at the width the file already uses,
and deleted outright when nothing is left.
"""
import os
import re
import subprocess
import sys

# The default target, which is x64: pinned at -m32 this needed the multilib
# runtime to answer a question that has nothing to do with pointer width.
CXX = ('g++ -march=k8 -std=c++17 -fno-strict-aliasing '
       '-fpermissive -fno-rtti -fno-exceptions -O2 -DNDEBUG -U_FORTIFY_SOURCE '
       '-D_FORTIFY_SOURCE=0 -Wunused-variable -fdiagnostics-plain-output '
       '-fsyntax-only bmf.cpp')
# `-fdiagnostics-plain-output` in the command above is what makes this pattern
# reliable: without it GCC quotes the name with the locale's quotation marks,
# and this file was silently finding nothing under a locale that gave it curly
# ones.  The pattern accepts both anyway, because a tool that depends on a flag
# staying in a string is one edit from being wrong again.
WARN = re.compile(r'^(\S+):(\d+):\d+: warning: unused variable '
                  r'[\'‘"`]([^\'’"`]+)[\'’"`]')


def warnings(argv, path):
    # Read the compile, unless a saved one is named.  This used to take stdin
    # whenever stdin was not a terminal, which is most of the time a script runs
    # it: the tool then read nothing, reported "no unused-variable warnings",
    # and left 66 of them in the file.  A default that silently means "do
    # nothing" is worse than no default.
    if '--from' in argv:
        text = open(argv[argv.index('--from') + 1]).read()
    else:
        text = subprocess.run(CXX, shell=True, capture_output=True,
                              text=True).stderr
    # The warning says which file it is about, and until bmf.cpp became an
    # include list there was only one it could be -- so the name was captured
    # and thrown away, and the line numbers applied to whatever `path` was.
    # Against a split tree that is a warning about `model.inc:7975` deleting
    # line 7975 of bmf.cpp, which is a file 160 lines long: the tool reported
    # zero and its three live sites were in another file entirely.
    want = os.path.basename(path)
    out = {}
    for l in text.split('\n'):
        m = WARN.match(l)
        if m and os.path.basename(m.group(1)) == want:
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
    want = warnings(sys.argv, path)
    if not want:
        print('no unused-variable warnings')
        return 0
    lines = open(path).read().split('\n')

    # One statement can hold names reported against several of its lines, so
    # collect by statement before editing, and edit from the bottom up.
    stmts = {}
    for ln in want:
        i = ln - 1
        # Walk back to the statement's first line -- but stop at a comment or a
        # blank, or the walk climbs into the paragraph above a declaration and
        # the type match fails silently.  Round eight put comments above enough
        # declarations to make that five missed deletions.
        while (i > 0 and ';' not in lines[i - 1].split('//')[0]
               and lines[i - 1].strip()
               and not lines[i - 1].lstrip().startswith('//')):
            i -= 1
        a, b = statement(lines, i)
        stmts.setdefault((a, b), set()).update(want[ln])

    gone = kept = deleted = 0
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
            deleted += 1
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

    # `--all` to write.  This used to write unconditionally, which meant asking
    # it what it found *was* applying it -- and `tools/sweep.sh`, which runs
    # every tool and fails if the file moved, caught exactly that.  A tool that
    # cannot be asked a question without answering it with an edit is one that
    # has to be left out of any survey.
    if '--all' in sys.argv:
        open(path, 'w').write('\n'.join(lines))
    # `len(stmts) - kept` counted a statement the type match skipped as a
    # deletion, which is how five of them were reported gone while the file was
    # byte-identical afterwards.  Count what was actually removed.
    print('%d declarations %s, %d rewritten, %d names gone'
          % (deleted, 'deleted' if '--all' in sys.argv else 'deletable',
             kept, gone))
    return 0


if __name__ == '__main__':
    sys.exit(main())
