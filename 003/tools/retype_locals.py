#!/usr/bin/env python3
"""Give a local the type the compiler says its assignments have.

    BMF_STRICT=1 ./build.sh          # writes strict.log
    python3 tools/retype_locals.py subs1.hpp --list
    python3 tools/retype_locals.py subs1.hpp --apply

Phase C's worklist is `strict.log`: every conversion the build needs
`-fpermissive` to accept.  Most of them are one shape --

    v3 = *(void **)&Blocka_1->f1078208;      // v3 is int32_t

-- where Hex-Rays gave a local an integer type and then assigned an address to
it.  The compiler already knows both types; this reads them out of the error
and rewrites the declaration.

It only moves a local when **every** complaint about it agrees on the source
type, and only when the declaration is an ordinary comma-separated one inside
the function.  A local assigned an address in one place and an integer in
another is not a typing mistake, it is one stack slot doing two jobs, and
splitting it is a decision this cannot make -- `symbol_list_update`'s `n251`
is the worked example (REFACTORING2.md §4.2).

**Retyping a local is not always neutral.**  `p + 1` steps one byte on a
`char *` and four on a `uint32_t *`, so a local that carries an address but is
walked with scaled arithmetic will change meaning.  That is what the gate is
for: run it after every batch, and revert the batch rather than the file if a
stream moves.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

# The declaration forms extract.py emits: `int32_t a, *b, c;` and friends.
DECL = re.compile(r'^(\s+)([A-Za-z_][A-Za-z0-9_]*)\s+(.+);$')


def complaints(log):
    """{(line, name): source type} for every conversion into a plain local."""
    out = {}
    for l in open(log):
        m = re.match(r"^subs1\.hpp:(\d+):\d+: error: invalid conversion from "
                     r"'([^']*)'(?: \{aka '([^']*)'\})? to "
                     r"'([^']*)'(?: \{aka '[^']*'\})?", l)
        if not m:
            continue
        out[int(m.group(1))] = (m.group(2), m.group(4))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    errs = complaints('strict.log')

    # group by (function, local), and only keep the ones that agree
    want, seen = {}, {}
    for a, b, nm, sig in structs.bodies(lines):
        for ln in range(a + 1, b + 2):
            if ln not in errs:
                continue
            src, dst = errs[ln]
            m = re.match(r'\s*([A-Za-z_]\w*)\s*=[^=]', lines[ln - 1])
            if not m:
                continue
            key = (nm, a, b, m.group(1))
            seen.setdefault(key, set()).add((src, dst))
    for key, kinds in seen.items():
        if len(kinds) == 1:
            want[key] = kinds.pop()

    moves = []
    for (nm, a, b, var), (src, dst) in sorted(want.items()):
        for ln in range(a + 1, b + 2):
            d = DECL.match(lines[ln - 1])
            if not d or d.group(2) != dst.rstrip('*').strip():
                continue
            names = [x.strip() for x in d.group(3).split(',')]
            stars = '*' * dst.count('*')
            if stars + var not in names:
                continue
            moves.append((ln, nm, var, d.group(1), d.group(2), names, src, dst))
            break

    if '--apply' not in sys.argv:
        for ln, nm, var, ind, ty, names, src, dst in moves:
            print('%-26s %-10s %-22s -> %s' % (nm, var, dst, src))
        print('%d locals can take the type their assignments have' % len(moves))
        return 0

    # rewrite back to front so earlier line numbers stay valid
    for ln, nm, var, ind, ty, names, src, dst in sorted(moves, reverse=True):
        stars = '*' * dst.count('*')
        rest = [n for n in names if n != stars + var]
        new = []
        if rest:
            new.append('%s%s %s;' % (ind, ty, ', '.join(rest)))
        new.append('%s%s %s;' % (ind, src.replace('*', ' *').rstrip(), var))
        lines[ln - 1:ln] = new
    open(path, 'w').write('\n'.join(lines))
    print('%d locals retyped' % len(moves))
    return 0


if __name__ == '__main__':
    sys.exit(main())
