#!/usr/bin/env python3
"""Definitions in bmf.cpp that nothing uses.

    python3 tools/undef.py [bmf.cpp]

`deadcheck.py` asks this of `subs1.hpp`'s function bodies.  Nothing asked it of
the header half -- the macros, the inline helpers and the file-scope objects in
`bmf.cpp` -- and eight had gone dead there:

  * `BMF_STANDALONE` and `_WINDOWS_`, both feature switches for `#include`s
    that are commented out;
  * `DWORD1`..`DWORD3` and `SDWORD1`..`SDWORD3`, added because "the bodies also
    index DWORD lanes directly".  No body does any more -- and the three signed
    ones never could have, because they expand to `SDWORDn`, which this file has
    never defined.  Three macros that would not compile if anyone used them,
    under a comment explaining why they were needed.

That last one is the argument for the rule.  An unused definition is not just
clutter: it is a claim about the program that nobody has had to keep true.

A definition is counted as used if its name appears anywhere in either file
outside its own definition line -- deliberately loose, because a macro read by
a `#include` this file no longer has is exactly the case that went wrong, and
the answer to a doubtful one is to look at it rather than to tighten a pattern.
"""
import re
import sys

DEFS = (
    # (kind, pattern) -- the name is group 1 in each.
    ('macro', re.compile(r'^#define\s+(\w+)', re.M)),
    ('function', re.compile(r'^(?:static\s+|inline\s+)+[\w \*]+?\b(\w+)\s*\(', re.M)),
    ('object', re.compile(r'^static\s+[\w \*]+?\b(\w+)\s*(?:\[[^\]]*\])?\s*(?:=|;)', re.M)),
)


def code(text):
    return '\n'.join(l.split('//')[0] for l in text.split('\n'))


def survey(path='bmf.cpp', other='subs1.hpp'):
    src = open(path).read()
    try:
        everything = code(src) + '\n' + code(open(other).read())
    except IOError:
        everything = code(src)
    out, seen = [], set()
    for kind, pat in DEFS:
        for m in pat.finditer(src):
            name = m.group(1)
            if name in seen:
                continue
            seen.add(name)
            if len(re.findall(r'\b%s\b' % re.escape(name), everything)) <= 1:
                out.append((src[:m.start()].count('\n') + 1, kind, name))
    return sorted(out)


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'bmf.cpp'
    # `sweep.sh` hands every tool a copy of subs1.hpp; this one reads the other
    # file, so an argument that is not a .cpp is the sweep asking politely.
    if not path.endswith('.cpp'):
        path = 'bmf.cpp'
    found = survey(path)
    for line, kind, name in found:
        print('%6d  %-9s %s' % (line, kind, name))
    print('%d definitions in %s that nothing uses' % (len(found), path))
