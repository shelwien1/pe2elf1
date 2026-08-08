#!/usr/bin/env python3
"""Rewrite subs1.hpp's raw BMF_BLOB(0x...) addresses as named globals.

    python3 tools/name_raw_addrs.py subs1.hpp

Most of BMF.exe's data segment reached the decompilation as named globals, but
Hex-Rays left some addresses baked into expressions instead -- the
`*(_QWORD *)(n64 + 4469652)` shape, 4469652 being 0x00443394 -- and extract.py
turned those into BMF_BLOB(0x00443394).  This puts them back on the globals
they belong to.

The substitution is address-identical by construction: BMF_BLOB(va) expands to
(int)(blob1 + va - BMF_BLOB_BASE), the global declared at va is
*(T*)(blob1 + va - BMF_BLOB_BASE), so &global == (T*)BMF_BLOB(va).

Where the address is the *start* of an object, the rewrite takes that object's
address.  Where it is inside one -- 0x00445668 is eight bytes into the table at
0x00445660 -- the offset is written out rather than declared as an overlapping
global.  One address, 0x0044337D, is indexed like an array and had no global of
any kind, so this adds one, matching __byte_44339D and __byte_4433AD next door.

Idempotent: a file with no BMF_BLOB() sites left is reported and not touched.
"""
import re
import sys

# Addresses that begin a global, and the expression for that global's address.
BASES = {
    '0x0044337C': '&__Buffer',
    '0x00443384': '&__dword_443384',
    '0x0044338C': '__n256_2',        # already an array, no &
    '0x00443394': '&__n4_5',
}

# (old, new) exact substitutions, with the number of sites each must match.
SUBS = [
    # 0x00442EA0 -- __xmmword_442EA0, indexed as the int32_t array it really is
    ('v30 = (int32_t *)(4 * v27 + BMF_BLOB(0x00442EA0));',
     'v30 = (int32_t *)&__xmmword_442EA0 + v27;', 1),
    ('v51 = (char *)(4 * v49 + BMF_BLOB(0x00442EA0));',
     'v51 = (char *)&__xmmword_442EA0 + 4 * v49;', 1),
    ('__dword_44342C = 4 * v49 + BMF_BLOB(0x00442EA0);',
     '__dword_44342C = (int32_t)((char *)&__xmmword_442EA0 + 4 * v49);', 1),

    # 0x0044337D -- byte 1 of the dword array based at __Buffer
    ('*(uint8_t *)(v17 * 4 + v16 * 4 + BMF_BLOB(0x0044337D))',
     '__byte_44337D[v17 * 4 + v16 * 4]', 1),

    # 0x00445660 -- __dword_445660[32].  0x445668/70/78 are +8/+16/+24 into that
    # same 128-byte table.  `(char *)__dword_445660 + (uint32_t)v` is the form
    # the lines just below these already use.
    ('*(uint64_t *)((char *)v4 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v4)', 1),
    ('*(uint64_t *)((char *)v4 + BMF_BLOB(0x00445668))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v4 + 8)', 1),
    ('*(uint64_t *)((char *)v4 + BMF_BLOB(0x00445670))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v4 + 16)', 1),
    ('*(uint64_t *)((char *)v4 + BMF_BLOB(0x00445678))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v4 + 24)', 1),
    ('*(uint64_t *)((char *)v9 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v9)', 1),
    ('*(uint64_t *)((char *)v9 + BMF_BLOB(0x00445668))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v9 + 8)', 1),
    ('*(uint64_t *)((char *)v9 + BMF_BLOB(0x00445670))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v9 + 16)', 1),
    ('*(uint64_t *)((char *)v9 + BMF_BLOB(0x00445678))',
     '*(uint64_t *)((char *)__dword_445660 + (uint32_t)v9 + 24)', 1),
    ('*(uint64_t *)(v2 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + v2)', 1),
    ('*(uint64_t *)(v3 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + v3)', 1),
    ('*(uint64_t *)(v3 + BMF_BLOB(0x00445668))',
     '*(uint64_t *)((char *)__dword_445660 + v3 + 8)', 1),
    ('*(uint64_t *)(v7 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + v7)', 1),
    ('*(uint64_t *)(v8 + BMF_BLOB(0x00445660))',
     '*(uint64_t *)((char *)__dword_445660 + v8)', 1),
    ('*(uint64_t *)(v8 + BMF_BLOB(0x00445668))',
     '*(uint64_t *)((char *)__dword_445660 + v8 + 8)', 1),
]

# The one address with no global of its own, inserted in address order.
NEW_GLOBAL = ('typedef uint8_t t_byte_44337D[0x10000];\n'
              'static t_byte_44337D& __byte_44337D = '
              '*(t_byte_44337D*)(blob1 + 0x0044337D - BMF_BLOB_BASE);\n')
NEW_GLOBAL_BEFORE = 'typedef int32_t t_buf[0x10000];\nstatic t_buf& __buf = '


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    text = open(path).read()

    if 'BMF_BLOB(0x' not in text:
        print('%s: no raw addresses, nothing to do' % path)
        return

    n = 0
    for va, name in BASES.items():
        # `expr * 4 + BMF_BLOB(va)` and `expr + BMF_BLOB(va)`, both byte offsets
        for pat, rep in ((r'\*\(uint64_t \*\)\((\w+) \* 4 \+ BMF_BLOB\(%s\)\)' % va,
                          r'*(uint64_t *)((char *)%s + \1 * 4)' % name),
                         (r'\*\(uint64_t \*\)\((\w+) \+ BMF_BLOB\(%s\)\)' % va,
                          r'*(uint64_t *)((char *)%s + \1)' % name)):
            text, k = re.subn(pat, rep, text)
            n += k

    for old, new, want in SUBS:
        found = text.count(old)
        if found != want:
            sys.exit('%s: expected %d of %r, found %d' % (path, want, old, found))
        text = text.replace(old, new)
        n += want

    if '__byte_44337D' not in text.split('\n// The range coder.')[0]:
        if text.count(NEW_GLOBAL_BEFORE) != 1:
            sys.exit('%s: cannot place __byte_44337D' % path)
        text = text.replace(NEW_GLOBAL_BEFORE, NEW_GLOBAL + NEW_GLOBAL_BEFORE)

    left = sorted(set(re.findall(r'BMF_BLOB\(0x[0-9A-Fa-f]+\)', text)))
    if left:
        sys.exit('%s: %d addresses left unhandled: %s' % (path, len(left), left))

    open(path, 'w').write(text)
    print('%s: %d raw addresses rewritten, none left' % (path, n))


if __name__ == '__main__':
    main()
