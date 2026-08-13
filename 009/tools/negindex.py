#!/usr/bin/env python3
"""Find `p[-u]` where `u` is unsigned — a subtraction that only i386 undoes.

    python3 tools/negindex.py subs1.hpp
    python3 tools/negindex.py subs1.hpp --all     # every negative index

Hex-Rays writes `&p[-n]` for a pointer stepped backwards, and gives `n` the
type MSVC's register had, which is often unsigned.  `-n` on a `uint32_t` is
0xFFFFFFFF-ish, and pointer arithmetic converts that index to the target's
`ptrdiff_t`: on i386 it is 32 bits and the value is exactly `-n`, so the
subtraction happens and nothing is wrong.  With a 64-bit pointer it is a
four-gigabyte step forward.

Two of these were the second and third things `tools/x64.sh` died on:

    q = (&p[-done]);          // predict_med, `done` a uint32_t
    neg = &(fold)[-lo];       // alt_init_tables, `lo` a uint32_t

Both are `p - n`, which says the same thing on both targets.  Neither is
visible to `strict64.log`: no pointer is narrowed and no conversion is
diagnosed -- the expression is well-formed and means two different things.

The type comes from `structs.decl_types`, which reads the body's own
declarations, so a name it cannot type is reported as `?` rather than guessed
at.  `--all` lists the signed ones too; they are correct, and seeing them is
how you check the rule is looking in the right places.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# The name is the *last* component: `p[-__frame.nplanes]` is typed by
# `nplanes`, and `structs.decl_types` reports a frame's members as if they
# were locals, which is the answer wanted here.  Matching the head of the
# chain reported `__frame` as untyped and buried the real row in a `?`.
INDEX = re.compile(r'\[\s*-\s*'
                   r'(?P<cast>\(\s*[A-Za-z_]\w*\s*\**\s*\)\s*)?\**\s*'
                   r'(?:[A-Za-z_]\w*\s*(?:->|\.)\s*)*(?P<name>[A-Za-z_]\w*)')


def survey(lines):
    """[(line, body, name, type, text)] for every `x[-name]`."""
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        types = structs.decl_types(sig, lines, a, b)
        for i in range(a, b + 1):
            for m in INDEX.finditer(lines[i].split('//')[0]):
                v = m.group('name')
                # A cast in front decides it: `&p[-*(uint32_t *)x]` is a
                # negated unsigned whatever `x` is declared as.
                ty = m.group('cast') or types.get(v, '?')
                out.append((i + 1, nm, v, ty.strip(' ()*'),
                            lines[i].strip()[:64]))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    rows = survey(lines)
    # `?` counts as suspect: a name this cannot type is exactly the one whose
    # signedness nobody has checked, and reporting it as clean would be the
    # tool telling a comfortable lie.
    bad = [r for r in rows if r[3].startswith('uint') or r[3] == '?']
    show = rows if '--all' in sys.argv else bad
    for line, nm, v, ty, text in show:
        print('%6d %-24s %-12s %-10s %s' % (line, nm, v, ty, text))
    print('%d negative indices on an unsigned or untyped name, of %d'
          % (len(bad), len(rows)))


if __name__ == '__main__':
    main()
