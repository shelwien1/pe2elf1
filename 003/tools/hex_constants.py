#!/usr/bin/env python3
"""Write subs1.hpp's pattern constants in hex instead of decimal.

    python3 tools/hex_constants.py subs1.hpp

Hex-Rays prints integer literals in decimal, which hides what some of them are.
A byte-fill written into a table next to `0x606060606060606LL` reads as
117901063; a float stored through an int pointer reads as 1065353216.  These
are the same numbers spelled so you can see the value:

    *((uint32_t *)v7 + 2) = 117901063;   ->  = 0x07070707;
    *((uint32_t *)_this + 58) = 1065353216;  ->  = 0x3F800000;  // 1.0f

Only constants whose *value* is a pattern are touched -- byte fills, IEEE-754
bit patterns, a reciprocal multiplier, and the BMP signature.  The rest of the
file's long decimal literals are byte offsets into heap structures
(`_this + 278736`) and stack frames (`__hexrays_frame + 66000`); hex would show
that 278736 is 16 * 17421 and little else, so they stay decimal.

None of them is an address: every constant in the file is outside BMF.exe's
image, which the decompilation reaches only through the globals block.

Idempotent: a file whose constants are already hex is reported and not touched.
"""
import re
import sys

# (decimal literal, replacement, expected sites).  Replacements are the same
# value written in hex, so this cannot change behaviour.
SUBS = [
    # Byte fills.  The two 0x07 lines finish a 7-byte run whose first four
    # bytes the line above writes, and the tables at __dword_445660 are filled
    # with 0x404040404040404LL and friends a few lines down.
    ('117901063', '0x07070707', 2),
    ('1799', '0x0707', 2),
    ('33685760', '0x02020100', 2),
    ('50529027', '0x03030303', 2),
    ('16843009', '0x01010101', 4),

    # IEEE-754 single-precision, stored through an int pointer.
    ('1065353216', '0x3F800000 /* 1.0f */', 2),
    ('1111228416', '0x423C0000 /* 47.0f */', 1),
    ('1126773555', '0x43293333 /* 169.2f */', 1),

    # 24 * 0x2AAAAAAB == 8 (mod 2^32), so for a v41 that is a multiple of 24 --
    # which the line above establishes, dividing it by 24 -- this product is
    # 8 * (v41 / 24), a divide and a scale the original compiler fused into one
    # multiply.
    ('715827883', '0x2AAAAAAB', 1),

    # 'BM', the BMP signature: written into the header on the way out, checked
    # on the way in.
    ('19778', "0x4D42 /* 'BM' */", 2),
]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    text = open(path).read()

    if not any(re.search(r'(?<![0-9A-Za-z_.])%s(?![0-9A-Za-z_])' % d, text)
               for d, _, _ in SUBS):
        print('%s: constants already in hex, nothing to do' % path)
        return

    n = 0
    for dec, hexed, want in SUBS:
        pat = re.compile(r'(?<![0-9A-Za-z_.])%s(?![0-9A-Za-z_])' % dec)
        found = len(pat.findall(text))
        if found != want:
            sys.exit('%s: expected %d sites for %s, found %d'
                     % (path, want, dec, found))
        text = pat.sub(hexed.replace('\\', '\\\\'), text)
        n += want

    open(path, 'w').write(text)
    print('%s: %d constants rewritten in hex' % (path, n))


if __name__ == '__main__':
    main()
