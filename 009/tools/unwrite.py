#!/usr/bin/env python3
"""Delete a local that is written once and never read.

    python3 tools/unwrite.py bmf.cpp
    python3 tools/unwrite.py bmf.cpp --all

`unused.py` works from the compiler's `-Wunused-variable`, which does not fire
on a variable that *is* assigned.  GCC's `-Wunused-but-set-variable` does, and
does not survive `-Wno-error`-free builds cleanly in a file this size, so this
does the same reading directly: one declaration, one use, and that use is a
plain assignment to it.

Every round of lifting leaves a few of these behind -- a name whose only reader
was folded away and whose writer nobody looked at again.  The assignment goes
with the name, which is safe only because it *is* an assignment: a call on the
right-hand side would be a side effect, and the pattern requires the whole
statement to be `v = ...;` with the call still in it, so those are kept by the
`--list` output rather than removed silently.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

DECL = re.compile(r'\s*(?:const )?[\w]+ ([\w, *]+)(;)?\s*$')
# A declaration that runs onto the next line carries no type there, so a scan
# that only matches whole statements sees the first line's names and none of
# the rest.  Sixty-eight of MSVC's `int32_t` spills are declared past a line
# break, and every one of them was invisible here until this carried the state
# down: ten of them were dead loads in front of a record copy.
MORE = re.compile(r'\s*([\w, *]+)(;)?\s*$')


def writeonly(lines):
    """[(use line, name, decl line)] for every write-only local."""
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        names, indecl = {}, False
        for i in range(a, b + 1):
            code = lines[i].split('//')[0]
            m = MORE.match(code) if indecl else DECL.match(code)
            if not m:
                indecl = False
                continue
            indecl = not m.group(2)
            for d in m.group(1).split(','):
                if d.strip():
                    names[d.strip().lstrip('* ')] = i
        for v, decl in names.items():
            uses = [i for i in range(a, b + 1) if i != decl
                    and re.search(r'(?<![\w.])%s\b' % re.escape(v), lines[i].split('//')[0])]
            if len(uses) == 1 and re.match(r'\s*%s = [^=].*;\s*$' % re.escape(v), lines[uses[0]]):
                out.append((uses[0], v, decl))
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found = writeonly(lines)
    # A call is an identifier followed by `(`.  Testing for a bare `(` counts
    # every cast as a call -- `v = (uint8_t *)p + 2` and `v = *(uint16_t *)(p -
    # 2)` are a sum and a load, and this declined to touch four of those until
    # the shapes around them changed enough to make them worth looking at.
    calls = [f for f in found
             if re.search(r'\w+\s*\(', lines[f[0]].split('=', 1)[1])]

    if '--all' not in sys.argv:
        for i, v, _ in found:
            print('%6d  %-10s %s%s' % (i + 1, v, lines[i].strip()[:70],
                                       '   <- has a call, kept' if (i, v, _) in calls else ''))
        print('%d write-only locals, %d of them with a call on the right' %
              (len(found), len(calls)))
        return 0

    for i, v, _ in sorted(set(found) - set(calls), reverse=True):
        del lines[i]
    open(path, 'w').write('\n'.join(lines))
    print('%d write-only locals deleted, %d kept for their side effect' %
          (len(found) - len(calls), len(calls)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
