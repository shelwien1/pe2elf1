#!/usr/bin/env python3
"""Say a value through the variable that already holds it.

    python3 tools/unhoist.py subs1.hpp
    python3 tools/unhoist.py subs1.hpp --all

MSVC computes `-18 * i` once and then spells it both ways in the block that
follows -- `v36 + v37` on one line and `v36 - 18 * i_1` on the next -- because
the register was free in one case and not in the other.  The two are the same
value, and a tool that matches on the *names* in an address (`unrec.py`,
`uncursor.py`) cannot see a group that spells them differently.

So the long-hand form is rewritten through the variable, which is a
substitution the compiler makes anyway, and the folds that need both sides to
agree can then see the group.  A variable is only used while its assignment is
the most recent one for that index.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

SET = re.compile(r'\s*(\w+) = (-(\d+) \* (\w+));\s*$')


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    n = 0
    for a, b, nm, _ in structs.bodies(lines):
        live = {}
        for i in range(a, b + 1):
            m = SET.match(lines[i])
            if m:
                live[(m.group(3), m.group(4))] = m.group(1)
                continue
            for (mul, idx), var in live.items():
                new, k = re.subn(r'(?<![\w.])(\w+) - %s \* %s\b' % (mul, re.escape(idx)),
                                 lambda mm, v=var: '%s + %s' % (mm.group(1), v), lines[i])
                if k:
                    lines[i] = new
                    n += k
    if '--all' in sys.argv:
        open(path, 'w').write('\n'.join(lines))
    print('%d long-hand offsets say the variable that holds them' % n)
    return 0


if __name__ == '__main__':
    sys.exit(main())
