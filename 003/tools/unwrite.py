#!/usr/bin/env python3
"""Delete a local that is written once and never read.

    python3 tools/unwrite.py subs1.hpp
    python3 tools/unwrite.py subs1.hpp --all

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

DECL = re.compile(r'\s*(?:const )?[\w]+ ([\w, *]+);\s*$')


def writeonly(lines):
    """[(use line, name, decl line)] for every write-only local."""
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        names = {}
        for i in range(a, b + 1):
            m = DECL.match(lines[i].split('//')[0])
            if m:
                for d in m.group(1).split(','):
                    names[d.strip().lstrip('* ')] = i
        for v, decl in names.items():
            uses = [i for i in range(a, b + 1) if i != decl
                    and re.search(r'(?<![\w.])%s\b' % re.escape(v), lines[i].split('//')[0])]
            if len(uses) == 1 and re.match(r'\s*%s = [^=].*;\s*$' % re.escape(v), lines[uses[0]]):
                out.append((uses[0], v, decl))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = writeonly(lines)
    calls = [f for f in found if '(' in lines[f[0]].split('=', 1)[1]]

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
