#!/usr/bin/env python3
"""Collapse recovered structs that are byte-for-byte the same declaration.

    python3 tools/dedup.py subs1.hpp --list
    python3 tools/dedup.py subs1.hpp --apply

`structs.py` recovers one struct per alias class, and the same shape turns up
again and again: ten copies of

    uint8_t _pad0[72]; uint32_t f72; uint32_t f76; uint32_t f80;
    uint32_t f84; uint16_t f88;

because ten cursors walk the same 18-byte record and the analysis cannot connect
them. Each declaration is correct and nine of them are noise.

This merges identical declarations and keeps the lowest-numbered name. It claims
nothing about meaning -- the names stay `ObjN`, and two structs having the same
layout is true by construction, not an inference. That is the whole reason it is
safe to do mechanically, and the reason it stops where it does:

**a shape with fewer than three members is left alone.** Fifteen structs are
just `const char f0;`. Merging those would put unrelated one-field objects under
one name, which is a claim, and one field is not evidence for it. The
five-way merge of the image descriptor in §Phase 4 went the other way -- by
hand, on evidence from the function that writes it.
"""
import re
import sys
from collections import defaultdict

DEF = re.compile(r'(?:^//[^\n]*\n)*^struct (Obj\d+) \{\n(.*?)\n\};\n'
                 r'(?:static_assert\((?:[^;]*)\);\n)?', re.S | re.M)


def groups(src, least=3):
    by = defaultdict(list)
    for m in DEF.finditer(src):
        body = re.sub(r'\s+', ' ', m.group(2)).strip()
        if len(re.findall(r'\bf\d+\s*;', body)) < least:
            continue
        by[body].append(m.group(1))
    return {k: sorted(v, key=lambda n: int(n[3:])) for k, v in by.items()
            if len(v) > 1}


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    g = groups(src)
    if '--apply' not in sys.argv:
        for body, names in sorted(g.items(), key=lambda kv: -len(kv[1])):
            print('%2d -> %-8s  %s' % (len(names), names[0], body[:70]))
        print('%d shapes, %d structs, %d declarations removable'
              % (len(g), sum(len(v) for v in g.values()),
                 sum(len(v) - 1 for v in g.values())))
        return 0

    drop = {}
    for names in g.values():
        for n in names[1:]:
            drop[n] = names[0]
    # the declarations first, then every mention of the dropped names
    src = DEF.sub(lambda m: '' if m.group(1) in drop else m.group(0), src)
    for old, new in drop.items():
        src = re.sub(r'\b%s\b' % old, new, src)
    open(path, 'w').write(src)
    print('%d declarations removed, %d names redirected'
          % (len(drop), len(drop)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
