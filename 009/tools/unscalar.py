#!/usr/bin/env python3
"""Delete a struct that is a run of one scalar, and point at the scalar.

    python3 tools/unscalar.py bmf.cpp --list
    python3 tools/unscalar.py bmf.cpp Obj13
    python3 tools/unscalar.py bmf.cpp --all

`structs.py` recovers a struct per access pattern, and a pattern that only ever
dereferences its base gives a struct with one member at offset zero:

    struct Obj13 { uint16_t f0; };
    v25 = (Obj13 *)(v24);
    v26 = v25->f0;

`uint16_t *` says all of that, and says it in the vocabulary the rest of the
file uses.  The struct, its `static_assert` and its header comment go; `p->f0`
becomes `*p`, and a cast of the struct becomes a cast of the scalar.

A struct whose members are all the *same* scalar, each at a multiple of its
width, is the same thing with more than one offset read: `struct Obj9 {
uint16_t f0; uint8_t _pad1[4]; uint16_t f6; uint8_t _pad3[4]; uint16_t f12; }`
is `uint16_t *` read at `[0]`, `[3]` and `[6]`.  The subscript is what the
byte offset was hiding.

A struct with two *different* scalars in it is not this -- it is a view of
something with a layout, and `unp2.py` or `merge.py` is what it wants.

The `->f0` rewrite is scoped to the declaring body, for the reason `unp2.py`
gives: a name is a different variable in another function, and `f0` is a
member of eleven other structs.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import merge                                                      # noqa: E402
import structs                                                    # noqa: E402
import unp2                                                       # noqa: E402

# A member is not the local of the same name: `blk->slot` is not `slot`.  A
# pattern that names an identifier has to say it is not reaching through one.
NAMED = r'(?<![\w.])(?<!->)%s\b'


def singles(src):
    """{ObjN: (type, {member: index})} for every struct that is one scalar run."""
    out = {}
    for name in re.findall(r'^struct (Obj\d+) \{', src, re.M):
        ms, _ = merge.parse(src, name)
        if not ms:
            continue
        tys = {f['ty'].strip() for f in ms}
        if len(tys) != 1:
            continue
        ty = tys.pop()
        w = merge.width(ty)
        if not w or any(f['off'] % w for f in ms):
            continue
        out[name] = (ty, {f['name']: (f['off'] // w, f['count']) for f in ms})
    return out


def rewrite(lines, name, at):
    """`p->fN` -> `p[i]` in every body that declares a `name *`.  Returns sites."""
    n = 0
    for a, b, _, sig in structs.bodies(lines):
        vs = {m.group(1) for i in range(a, b + 1)
              for m in re.finditer(r'\b%s \*(\w+)' % name, lines[i].split('//')[0])}
        vs |= {m.group(1) for m in re.finditer(r'\b%s \*(\w+)' % name, sig)}
        # a frame member is a variable too, and its name carries the prefix
        vs |= {'frame.' + m.group(1) for i in range(a, b + 1)
               for m in re.finditer(r'\b%s \*(\w+)' % name, lines[i].split('//')[0])}
        for v in vs:
            for i in range(a, b + 1):
                code, sep, com = lines[i].partition('//')
                for mem, (k, count) in at.items():
                    e = re.escape(v)
                    if count > 1:
                        # `p->f0[i]` is element `k + i`, not `(*p)[i]`.
                        code, c = re.subn(
                            r'(?<![\w.])%s->%s\[([^\]]*)\]' % (e, mem),
                            lambda m, v=v, k=k: '%s[%s]' % (
                                v, m.group(1) if not k else '%d + %s' % (k, m.group(1))),
                            code)
                        n += c
                        code, c = re.subn(r'(?<![\w.])%s->%s\b' % (e, mem),
                                          v if not k else '%s + %d' % (v, k), code)
                        n += c
                        continue
                    new = ('*' + v) if k == 0 else '%s[%d]' % (v, k)
                    code, c = re.subn(r'(?<![\w.])%s->%s\b' % (e, mem), new, code)
                    n += c
                lines[i] = code + sep + com
    return n


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    src = open(path).read()
    args = [x for x in sys.argv[2:] if not x.startswith('--')]
    found = singles(src)

    if '--list' in sys.argv or not (args or '--all' in sys.argv):
        for n, (t, at) in sorted(found.items(), key=lambda kv: int(kv[0][3:])):
            print('%-8s %-10s %d uses, %s'
                  % (n, t, len(re.findall(NAMED % n, src)),
                     ' '.join('%s=[%d]%s' % (m, k, '..' if c > 1 else '')
                              for m, (k, c) in sorted(at.items(), key=lambda y: y[1]))))
        print('%d structs are a run of one scalar' % len(found))
        return 0

    want = [n for n in (args or found) if n in found]
    lines = src.split('\n')
    gone = sites = 0
    for n in want:
        t, at = found[n]
        sites += rewrite(lines, n, at)
        sep = '' if t.endswith('*') else ' '
        src = '\n'.join(lines)
        src = re.sub(r'\b%s \*' % n, '%s%s*' % (t, sep), src)
        src = re.sub(r'\((?:const )?%s \*\)' % n, '(%s%s*)' % (t, sep), src)
        lines = src.split('\n')
        span = unp2.declaration(lines, n)
        if span and not any(re.search(NAMED % n, lines[j])
                            for j in range(len(lines))
                            if not span[0] <= j <= span[1]):
            del lines[span[0]:span[1] + 1]
            gone += 1
            print('%-8s -> %s *' % (n, t))
    open(path, 'w').write('\n'.join(lines))
    print('%d structs deleted, %d dereferences' % (gone, sites))
    return 0


if __name__ == '__main__':
    sys.exit(main())
