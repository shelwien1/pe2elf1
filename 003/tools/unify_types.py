#!/usr/bin/env python3
"""Rewrite subs1.hpp's type vocabulary onto <cstdint>.

Hex-Rays emits a type per whim: _DWORD here, `unsigned int` there, `unsigned
__int16` for the same 16 bits a `_WORD` holds two lines up.  Every one of them
is a fixed-width integer on this (i386) target, so they all have an exact
stdint spelling.  This maps each onto that spelling, which also gets rid of
every multi-word type name.

The only 8-bit type left alone is `char`: int8_t is `signed char`, a distinct
type from `char`, and `char *` here is the C string type — it reaches strcpy,
strrchr, sprintf and fopen.  `unsigned char` still becomes uint8_t (the same
type under a different name), so nothing multi-word survives.

Nothing here changes a type's identity; the substitutions below are all
spelling changes for this target, and the object file is expected to come out
bit-identical.
"""
import re
import sys

# Longest-first: the multi-word forms have to be tried before their tails.
SUBS = [
    # Hex-Rays' __intN, and MSVC's, keyed off defs.h:
    #   __int8 -> char, __int16 -> short, __int32 -> int, __int64 -> long long
    (r'unsigned\s+__int64', 'uint64_t'),
    (r'unsigned\s+__int32', 'uint32_t'),
    (r'unsigned\s+__int16', 'uint16_t'),
    (r'unsigned\s+__int8',  'uint8_t'),
    (r'signed\s+__int64',   'int64_t'),
    (r'signed\s+__int32',   'int32_t'),
    (r'signed\s+__int16',   'int16_t'),
    (r'signed\s+__int8',    'int8_t'),
    # Multi-word C spellings.
    (r'unsigned\s+long\s+long', 'uint64_t'),
    (r'signed\s+long\s+long',   'int64_t'),
    (r'long\s+long',            'int64_t'),
    (r'unsigned\s+char',        'uint8_t'),
    (r'signed\s+char',          'int8_t'),
    (r'unsigned\s+short',       'uint16_t'),
    (r'signed\s+short',         'int16_t'),
    (r'unsigned\s+long',        'uint32_t'),
    (r'signed\s+long',          'int32_t'),
    (r'unsigned\s+int',         'uint32_t'),
    (r'signed\s+int',           'int32_t'),
    # Bare `unsigned` / `signed`, which mean int.
    (r'unsigned',           'uint32_t'),
    (r'signed',             'int32_t'),
    # Single-word __intN.
    (r'__int64',            'int64_t'),
    (r'__int32',            'int32_t'),
    (r'__int16',            'int16_t'),
    (r'__int8',             'int8_t'),
    # 128 bits has no stdint name.  Both spellings are M128I already (bmf.cpp
    # maps `__int128` and `_OWORD` onto it), so say so: these are SSE
    # registers, not integers, and the bodies use __m128i everywhere else.
    (r'__int128',           '__m128i'),
    (r'_OWORD',             '__m128i'),
    # Hex-Rays' partially-defined types (defs.h: uint8/uint16/uint32/uint64).
    (r'_BYTE',              'uint8_t'),
    (r'_WORD',              'uint16_t'),
    (r'_DWORD',             'uint32_t'),
    (r'_QWORD',             'uint64_t'),
    # windows.h's, at the widths bmf.cpp declares them with.
    (r'BOOL',               'int32_t'),
    (r'DWORD',              'uint32_t'),
    (r'WORD',               'uint16_t'),
    (r'WCHAR',              'uint16_t'),
    (r'CHAR',               'char'),
    (r'LPCSTR',             'const char *'),
    (r'LPVOID',             'void *'),
    # size_t is `unsigned int` here; the bodies use it as a plain 32-bit
    # counter, next to _DWORDs holding the same values.
    (r'size_t',             'uint32_t'),
    # Plain C.
    (r'short',              'int16_t'),
    (r'int',                'int32_t'),
]

PAT = re.compile(r'\b(?:' + '|'.join('(?:%s)' % p for p, _ in SUBS) + r')\b')
INDIVIDUAL = [(re.compile(r'\A(?:%s)\Z' % p), r) for p, r in SUBS]

# String literals, character literals, and both comment forms: skipped whole,
# so a `%d int` inside a format string is not a type.
SKIP = re.compile(r'"(?:[^"\\\n]|\\.)*"'
                  r"|'(?:[^'\\\n]|\\.)*'"
                  r'|//[^\n]*'
                  r'|/\*.*?\*/', re.S)


def replace_type(m):
    text = m.group(0)
    for pat, rep in INDIVIDUAL:
        if pat.match(text):
            return rep
    raise AssertionError('unmapped type: %r' % text)


def rewrite(src):
    out = []
    pos = 0
    for m in SKIP.finditer(src):
        out.append(PAT.sub(replace_type, src[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(PAT.sub(replace_type, src[pos:]))
    return ''.join(out)


def main():
    path = sys.argv[1]
    src = open(path).read()
    new = rewrite(src)
    open(path, 'w').write(new)

    # Report what is left, so anything unmapped shows up rather than hiding.
    kw = (r'unsigned|signed|long|short|int|char|__int8|__int16|__int32|__int64'
          r'|__int128|_BYTE|_WORD|_DWORD|_QWORD|_OWORD|BOOL|BYTE|WORD|DWORD'
          r'|LONG|QWORD|size_t|WCHAR|CHAR|LPCSTR|LPVOID|bool|float|double|void'
          r'|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t')
    seen = {}
    scan = re.compile(r'\b(?:(?:' + kw + r')\b\s*)+')
    pos = 0
    pieces = []
    for m in SKIP.finditer(new):
        pieces.append(new[pos:m.start()])
        pos = m.end()
    pieces.append(new[pos:])
    for piece in pieces:
        for m in scan.finditer(piece):
            t = ' '.join(m.group(0).split())
            seen[t] = seen.get(t, 0) + 1
    print('type spellings remaining in %s:' % path)
    for t, n in sorted(seen.items(), key=lambda x: -x[1]):
        flag = '  <-- MULTI-WORD' if len(t.split()) > 1 else ''
        print('  %6d  %s%s' % (n, t, flag))


if __name__ == '__main__':
    main()
