#!/usr/bin/env python3
"""Drop the cast when the base already has that type.

    python3 tools/unrecast.py bmf.cpp
    python3 tools/unrecast.py bmf.cpp --all

Hex-Rays casts before every scaled dereference whether or not the cast says
anything, and after five rounds of retyping most of them say nothing:

    v4 = *((uint32_t *)_this + 1);      // _this is a `uint32_t *`
    v4 = _this[1];

Both spellings of the offset are handled -- `*((T *)p + k)` and `*(T *)((uint8_t
*)p + n)`, the second only when `n` is a whole number of elements -- and the
rewrite happens only when the base is *declared* as a pointer to exactly that
type, in that body or in the signature.
"""
import re, sys, collections
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs, merge
if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
    sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                     % __file__.rsplit('/', 1)[-1])
p = sys.argv[1]
lines = open(p).read().split('\n')
W = merge.WIDTH
n = collections.Counter()
apply = '--all' in sys.argv
for a, b, nm, sig in structs.bodies(lines):
    ty = {}
    for m in re.finditer(r'\b(u?int(?:8|16|32|64)_t|float|double)\s*\*\s*(\w+)', sig):
        ty[m.group(2)] = m.group(1)
    for i in range(a, b + 1):
        c = lines[i].split('//')[0]
        mm = re.match(r'\s+(u?int(?:8|16|32|64)_t|float|double)\s+([\w, *]+);', c)
        if mm:
            for d in mm.group(2).split(','):
                d = d.strip()
                if d.startswith('*'):
                    ty.setdefault(d.lstrip('* '), mm.group(1))
    for i in range(a, b + 1):
        code, sep, com = lines[i].partition('//')
        def scaled(m):
            t, v, k = m.group(1), m.group(2), int(m.group(4))
            if ty.get(v) != t:
                return m.group(0)
            n[nm] += 1
            return '%s[%s%d]' % (v, '-' if m.group(3) == '-' else '', k)
        def bytes_(m):
            t, v, k = m.group(1), m.group(2), int(m.group(4))
            if ty.get(v) != t or k % W[t]:
                return m.group(0)
            n[nm] += 1
            return '%s[%s%d]' % (v, '-' if m.group(3) == '-' else '', k // W[t])
        def bare(m):
            t, v = m.group(1), m.group(2)
            if ty.get(v) != t:
                return m.group(0)
            n[nm] += 1
            return v
        code = re.sub(r'\*\(\((\w+) \*\)(\w+) ([-+]) (\d+)\)', scaled, code)
        code = re.sub(r'\*\((\w+) \*\)\(\(uint8_t \*\)(\w+) ([-+]) (\d+)\)', bytes_, code)
        # The cast with no dereference behind it: `(uint16_t *)_this + 2` where
        # `_this` is already a `uint16_t *`.  Only when the next token cannot
        # make it part of a larger cast or a declaration.
        #
        # Repeated until it stops changing, because Hex-Rays stacked these --
        # `(uint16_t *)(uint16_t *)n0xF0` -- and dropping the inner one leaves
        # the outer one matching a pattern it did not match before.
        while True:
            out = re.sub(r'\((u?int(?:8|16|32|64)_t|float|double) \*\)(\w+)(?=\s*[-+),;\]])',
                         bare, code)
            if out == code:
                break
            code = out
        lines[i] = code + sep + com
for k, v in n.most_common(10):
    print('%-26s %3d' % (k.lstrip('_'), v))
print(sum(n.values()), 'sites')
if apply:
    open(p, 'w').write('\n'.join(lines))
