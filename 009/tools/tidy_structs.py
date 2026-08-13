#!/usr/bin/env python3
"""Clean up what `structs.py` had to write conservatively.

    python3 tools/tidy_structs.py subs1.hpp

The rewriter emits three things wider than they need to be, because at the
moment it writes them it does not know they are safe to narrow:

    *(uint8_t * *)&p->f196          the pointee type came back with its own
                                    star, and got another one
    (intptr_t)(4 * p->f32 + 36)     the cast is there in case a term is a
                                    pointer; usually none is
    ( - 4 * p->f160 + 172)          the first term kept the space that
                                    separated it from the term before

None of these changes what the code does, and the gate says so.  The cast is
the only one that needs a check: it is dropped only when no name in the
expression is declared as a pointer, because `(char *)p + (q + 4)` with q a
pointer is an error, not a style question.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402


def main():
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    fns = structs.bodies(lines)
    out = list(lines)
    counts = {'star': 0, 'cast': 0, 'sign': 0}

    for a, b, fn, sig in fns:
        types = structs.decl_types(sig, lines, a, b)
        pointers = {nm for nm, ty in types.items() if ty.endswith('*')}
        for i in range(a, b + 1):
            line = out[i]

            def drop_cast(m):
                names = set(re.findall(r'(?<![.>\w])([A-Za-z_][A-Za-z0-9_]*)',
                                       m.group(1)))
                if names & pointers:
                    return m.group(0)
                counts['cast'] += 1
                return '(%s)' % m.group(1).strip()
            line = re.sub(r'\(intptr_t\)\(([^()]*)\)', drop_cast, line)

            n = line.count('* *)')
            if n:
                counts['star'] += n
                line = line.replace('* *)', '**)')
            n = len(re.findall(r'\(\s+-\s', line))
            if n:
                counts['sign'] += n
                line = re.sub(r'\(\s+-\s', '(-', line)
            out[i] = line

    open(path, 'w').write('\n'.join(out))
    print('%d double stars, %d casts dropped, %d leading signs'
          % (counts['star'], counts['cast'], counts['sign']))


if __name__ == '__main__':
    main()
