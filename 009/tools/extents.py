#!/usr/bin/env python3
"""What each blob global is actually used as.

    python3 tools/extents.py subs1.hpp

Hex-Rays gave most of these globals the bound `[0x10000]`, which is a guess and
usually a wrong one: `hist_scratch` is declared `int32_t[65536]` and only ever
touched at `[0]`, where it holds a pointer.  Before a global can become a
definition of its own rather than a slice of `blob1`, its real extent has to be
known, and this is what says which ones are knowable from the source alone.

For every global it reports:

  const N     every subscript is a constant; the largest is N-1
  var         some subscript is an expression, so the extent is not static
  scalar      never subscripted
  addr        its address is taken, so arithmetic may reach past it

`addr` is the one to look at first: a global whose address is used as a base is
a global whose neighbours matter, and REFACTORING.md §4.1 is what that costs.
"""
import re
import sys


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    text = open(path).read()

    decl = re.findall(
        r'^typedef ([A-Za-z_][A-Za-z0-9_ ]*?) (t_[A-Za-z0-9_]+)(?:\[(\w+)\])?;\n'
        r'static t_[A-Za-z0-9_]+& ([A-Za-z_][A-Za-z0-9_]*) = '
        r'\*\(t_[A-Za-z0-9_]+\*\)\(blob1 \+ (0x[0-9A-F]+)', text, re.M)
    size = {'int32_t': 4, 'uint8_t': 1, 'char': 1, '__m128i': 16,
            'uint16_t': 2, 'int8_t': 1, 'uint32_t': 4}

    body = text[text.index('// The range coder.'):]
    rows = []
    for ty, _, bound, name, va in decl:
        w = size.get(ty, 4)
        subs = re.findall(r'\b%s\s*\[([^\]\n]+)\]' % re.escape(name), body)
        addr = len(re.findall(r'&%s\b' % re.escape(name), body))
        addr += len(re.findall(r'\(char \*\)%s\b' % re.escape(name), body))
        uses = len(re.findall(r'\b%s\b' % re.escape(name), body))
        if not subs:
            kind, n = 'scalar', 1
        else:
            const = []
            for s in subs:
                s = s.strip()
                if re.fullmatch(r'\d+|0x[0-9A-Fa-f]+', s):
                    const.append(int(s, 0))
                else:
                    const = None
                    break
            if const is None:
                kind, n = 'var', None
            else:
                kind, n = 'const', max(const) + 1
        rows.append((int(va, 16), name, ty, int(bound, 0) if bound else 1,
                     w, kind, n, addr, uses))

    rows.sort()
    print('%-8s %-24s %-9s %8s %8s %6s %5s' %
          ('addr', 'name', 'type', 'declared', 'used', 'addr?', 'uses'))
    tally = {}
    for va, name, ty, bound, w, kind, n, addr, uses in rows:
        used = '%s %s' % (kind, n if n is not None else '')
        tally[kind] = tally.get(kind, 0) + 1
        print('%08X %-24s %-9s %8d %8s %6s %5d'
              % (va, name, ty, bound, used.strip(), 'yes' if addr else '', uses))
    print('\n%d globals: %s' % (len(rows),
          ', '.join('%d %s' % (v, k) for k, v in sorted(tally.items()))))
    print('%d have their address taken' % sum(1 for r in rows if r[7]))


if __name__ == '__main__':
    main()
