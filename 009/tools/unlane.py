#!/usr/bin/env python3
"""Turn `p->f278528[i].m128_XX[k]` into the member of `Obj11` at that offset.

    python3 tools/unlane.py subs1.hpp --list
    python3 tools/unlane.py subs1.hpp --all

REFACTORING4.md §2.5.  `Obj11`'s union declares the same bytes four ways, and
the first way -- `__m128 f278528[21]` -- is Hex-Rays' record of a 16-byte
access unit, not a type.  Every one of its 175 remaining sites has a constant
element and a constant lane, so every one is a fixed offset, and the union's
other layers already name most of them:

    a1->f278528[13].m128_i32[0]     +278736     a1->f278736[0]
    a1->f278528[11].m128_i32[0]     +278704     a1->f278704

**The bytes read must not change**, so the type at the site decides the
spelling: a member whose declared type is the access type is written plainly,
and anything else is written `*(access *)&p->member`.  That reads the same
bytes at the same width whatever the member is -- which is the rule
`unoffset.py` already follows for the same reason, and the reason is that a
`uint16_t` store rewritten as a `uint32_t` member compiles, passes seven of
the fifteen images, and corrupts the heap.

Ten of the row cursors are `uint8_t *`, read as `int32_t` 78 times because
that is how the original moved an address through a register.  Those come out
as `*(int32_t *)&p->f278736[k]`, and a second pass folds the two idioms where
the int is only on its way back to being a pointer:

    *(T *)(*(int32_t *)&p->m + N)   ->   *(T *)(p->m + N)
    (T *)*(int32_t *)&p->m          ->   (T *)p->m

both exact, because the member is a byte pointer and byte arithmetic on it is
the arithmetic the int was doing.
"""
import re
import sys

# offset -> (member expression, declared type)
MEMBERS = {}
for off in (278640, 278648):
    MEMBERS[off] = ('f%d' % off, 'uint64_t')
MEMBERS[278656] = ('f278656', 'float (*)[4]')
MEMBERS[278660] = ('f278660', 'int32_t *')
for off in (278664, 278668, 278672):
    MEMBERS[off] = ('f%d' % off, 'uint8_t *')
for off in range(278676, 278700, 4):
    MEMBERS[off] = ('f%d' % off, 'int32_t')
MEMBERS[278700] = ('f278700', 'uint16_t')
for off in (278704, 278708, 278712):
    MEMBERS[off] = ('f%d' % off, 'uint32_t')
for off in (278716, 278720, 278724):
    MEMBERS[off] = ('f%d' % off, 'int32_t')
MEMBERS[278728] = ('f278728', 'uint32_t')
MEMBERS[278732] = ('f278732', 'int32_t')
for k in range(10):
    MEMBERS[278736 + 4 * k] = ('f278736[%d]' % k, 'uint8_t *')
for k in range(24):
    MEMBERS.setdefault(278760 + 4 * k, ('f278760[%d]' % k, 'uint32_t'))

ACCESS = {'m128_i32': ('int32_t', 4), 'm128_u32': ('uint32_t', 4),
          'm128_p': ('char *', 4), 'm128_f32': ('float', 4),
          'm128_i64': ('int64_t', 8), 'm128_u64': ('uint64_t', 8),
          'm128_i16': ('int16_t', 2), 'm128_u16': ('uint16_t', 2),
          'm128_i8': ('int8_t', 1), 'm128_u8': ('uint8_t', 1)}

# the base is part of the match: `*(T *)&` has to go outside the `->`
SITE = re.compile(r'(\w+(?:\[\w+\])?)->f278528\[(\d+)\]\.(m128_\w+)\[(\d+)\]')


def spell(off, lane, is_write):
    """The member at `off`, read as `lane` reads it.

    A write has to stay an lvalue, so it always goes through `*(T *)&m`.  A
    read of a pointer member as `m128_p` is the stored pointer either way, and
    `(char *)m` is the shorter of the two.
    """
    if off not in MEMBERS:
        return None
    name, decl = MEMBERS[off]
    acc, _ = ACCESS[lane]
    if decl == acc:
        return '%%s->%s' % name
    if lane == 'm128_p' and decl.endswith('*') and not is_write:
        return '(char *)%%s->%s' % name
    return '*(%s *)&%%s->%s' % (acc.replace(' *', '*'), name)


def rewrite(src):
    missing, n = set(), 0

    def rep(m):
        nonlocal n
        base, i, lane, k = m.group(1), int(m.group(2)), m.group(3), int(m.group(4))
        off = 278528 + 16 * i + ACCESS[lane][1] * k
        tail = m.string[m.end():m.end() + 4]
        out = spell(off, lane, re.match(r'\s*=[^=]', tail) is not None)
        if out is None:
            missing.add(off)
            return m.group(0)
        n += 1
        return out % base

    src = SITE.sub(rep, src)
    # the int was only on its way back to being a pointer
    src, a = re.subn(r'\*\(int32_t \*\)&(\w+(?:\[\w+\])?->f278736\[\d+\])(?= [-+] \d+\))',
                     r'\1', src)
    src, b = re.subn(r'(\(\s*(?:const )?\w+ [*\s]*\)\s*)\*\(int32_t \*\)&'
                     r'(\w+(?:\[\w+\])?->f278736\[\d+\])', r'\1\2', src)
    return src, n, a + b, missing


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    out, n, folded, missing = rewrite(src)

    if '--list' in sys.argv or '--all' not in sys.argv:
        import collections
        c = collections.Counter()
        for m in SITE.finditer(src):
            i, lane, k = int(m.group(2)), m.group(3), int(m.group(4))
            c[(278528 + 16 * i + ACCESS[lane][1] * k, lane)] += 1
        for (off, lane), k in sorted(c.items()):
            sp = spell(off, lane, False)
            print('%7d  %-9s x%-3d %s' % (off, lane, k, (sp % 'p') if sp else 'NO MEMBER'))
        print('%d sites, %d of them fold back to a pointer, %d offsets unnamed: %s'
              % (n, folded, len(missing), sorted(missing)))
        return 0

    assert not missing, sorted(missing)
    print('%d sites, %d folded back to a pointer' % (n, folded))
    open(path, 'w').write(out)
    return 0


if __name__ == '__main__':
    sys.exit(main())
