#!/usr/bin/env python3
"""Fold a frame alias into the member it binds, REFACTORING4.md §5 item 2.

    python3 tools/unalias.py subs1.hpp --list
    python3 tools/unalias.py subs1.hpp __read_bmp
    python3 tools/unalias.py subs1.hpp --all

Round three gave every frame member a reference so the body could keep saying
what Hex-Rays called it:

    int32_t (&v72)[1024] = __frame.v72;
    int32_t &v57 = (int32_t &)__frame.list[2];

The declaration goes and the body says the member: `v72` becomes `__frame.v72`,
and an alias that carries a cast becomes the parenthesised cast, because `->`
and `[]` bind tighter than a cast-expression and a bare substitution would
change what the line means.

A rename is answerable to the compiler rather than to the gate, which is what
makes 1828 sites in one pass reasonable: a type that stops matching is an error.

The casts this leaves are `unmemcast.py`'s: where every reader of a member
agrees on one type of its width, the member can be that type instead.  That one
goes a member at a time, because retyping is a semantic change and §7's list of
what it can cost is three items long.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import shape                                                    # noqa: E402
import structs                                                  # noqa: E402

ALIAS = re.compile(r'^(\s*)(.+?)\s*\(?&\s*(\w+)\)?(\[\d+\])?\s*=\s*(.+?);\s*(//.*)?$')
MEMBER = re.compile(r'^(\s*)([A-Za-z_][\w ]*?\s*\**)\s*(\w+)\s*(\[(\d+)\])?\s*;(.*)$')
PLAIN = re.compile(r'^__frame\.\w+(\[\d+\])?$')


def frame_of(lines, a, b):
    """(first line, last line) of the frame struct, or None."""
    for i in range(a, b + 1):
        if shape.FRAME.search(lines[i]):
            j = i
            while j <= b and not lines[j].lstrip().startswith('} __frame;'):
                j += 1
            return i, j
    return None


def aliases(lines, a, b, close):
    """[(line, name, member, type, rhs)] for every alias below the struct."""
    out = []
    for i in range(close + 1, b + 1):
        if not shape.ALIAS.search(lines[i]):
            continue
        m = ALIAS.match(lines[i])
        if not m:
            continue
        mem = re.search(r'__frame\.(\w+(?:\[\d+\])?)', m.group(5))
        out.append((i, m.group(3), mem.group(1), m.group(2).strip(), m.group(5)))
    return out


def fold(lines, a, b, al, keep):
    """Pass B: delete the declarations, put the member where the name was.

    `keep` is the struct's own line range: a member called `buf` inside it is
    the declaration of `__frame.buf`, not a use of the alias.
    """
    drop, sub = set(), {}
    for i, name, mem, ty, rhs in al:
        drop.add(i)
        sub[name] = rhs if PLAIN.match(rhs) else '(%s)' % rhs
    if not sub:
        return 0
    rx = re.compile(r'(?<![\w.])(%s)\b' % '|'.join(map(re.escape, sub)))
    n = 0
    for i in range(a, b + 1):
        if i in drop or keep[0] <= i <= keep[1]:
            continue
        code, sep, rest = lines[i].partition('//')
        new = rx.sub(lambda m: sub[m.group(1)], code)
        if new != code:
            n += len(rx.findall(code))
            lines[i] = new + sep + rest
    for i in sorted(drop, reverse=True):
        del lines[i]
    return n


def run(lines, want):
    total = [0, 0, 0]
    for a, b, nm, _ in reversed(list(structs.bodies(lines))):
        if want and nm not in want:
            continue
        fr = frame_of(lines, a, b)
        if not fr:
            continue
        al = aliases(lines, a, b, fr[1])
        if not al:
            continue
        total[2] += fold(lines, a, b, al, fr)
        total[1] += len(al)
    return total


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for a, b, nm, _ in structs.bodies(lines):
            fr = frame_of(lines, a, b)
            if not fr:
                continue
            al = aliases(lines, a, b, fr[1])
            if al:
                cast = sum(1 for x in al if not PLAIN.match(x[4]))
                print('%-24s %3d aliases, %d carry a cast' % (nm.lstrip('_'), len(al), cast))
        return 0

    _, folded, sites = run(lines, set(args))
    print('%d aliases folded, %d sites' % (folded, sites))
    open(path, 'w').write('\n'.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main())
