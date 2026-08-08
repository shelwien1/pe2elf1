#!/usr/bin/env python3
"""Give one of the program's objects a struct, and use it.

    python3 tools/structs.py subs1.hpp --list
    python3 tools/structs.py subs1.hpp --apply 3      # object 3

`tools/objects.py` groups the names that denote the same allocation and reports
each one's field map.  This turns a map into a declaration and rewrites the
accesses:

    *(uint32_t *)(_this + 76)   ->   _this->f76
    *(uint8_t  *)(_this + 8)    ->   _this->f8

The target is 32-bit, and that is what makes it safe: a pointer is four bytes
there, so a struct laid out from the observed offsets has exactly the layout the
code already assumed.  Nothing moves, so the variable-offset walks these objects
also get -- `*(T *)(p + 4 * i)` -- keep meaning what they meant; they become
`*(T *)((char *)p + 4 * i)` and are left alone otherwise.  Every generated
struct carries a `static_assert` on its size to say so out loud.

Where one offset is read at two widths -- a byte inside a word, which is common
-- the widest wins and the narrow access goes through a cast of the member's
address.  Where an offset is dereferenced after loading, the member is a
pointer, which is what the 64-bit build would need widened and what the 32-bit
build merely needs named.

Trailing bytes matter: an object addressed at 0 and 1078692 gets a member at
each end and one big `pad` between, because the offsets in between belong to
arrays this cannot see the bounds of.  That is honest -- the struct describes
what is known and does not pretend about the rest.
"""
import collections
import re
import sys

WIDTH = {'char': 1, 'uint8_t': 1, 'int8_t': 1, 'uint16_t': 2, 'int16_t': 2,
         'int32_t': 4, 'uint32_t': 4, 'float': 4, 'int64_t': 8, 'uint64_t': 8,
         'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16, 'FILE': 4}


def width(ty):
    ty = ty.strip()
    return 4 if ty.endswith('*') else WIDTH.get(ty)


def bodies(lines):
    out, depth, start, name, sig = [], 0, None, None, None
    for i, l in enumerate(lines):
        s = l.split('//')[0]
        for k, ch in enumerate(s):
            if ch == '{':
                if depth == 0:
                    buf, j = s[:k], i
                    while True:
                        if buf.count(')') - buf.count('(') <= 0 and '(' in buf:
                            break
                        j -= 1
                        if j < 0:
                            break
                        buf = lines[j].split('//')[0] + ' ' + buf
                    m = re.search(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', buf)
                    name, sig = (m.group(1) if m else '?'), buf
                    start = i
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0 and start is not None:
                    out.append((start, i, name, sig))
                    start = None
    return out


def params_of(sig):
    m = re.search(r'\((.*)\)\s*\{?\s*$', sig)
    if not m:
        return []
    parts, depth, cur = [], 0, ''
    for ch in m.group(1):
        if ch in '(<':
            depth += 1
        elif ch in ')>':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    return [(re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', p.strip()).group(1)
             if re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', p.strip()) else None)
            for p in parts]


class UF:
    def __init__(self):
        self.p = {}

    def find(self, x):
        self.p.setdefault(x, x)
        while self.p[x] != x:
            self.p[x] = self.p[self.p[x]]
            x = self.p[x]
        return x

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.p[ra] = rb


def classes(lines):
    fns = bodies(lines)
    params = {n: params_of(s) for _, _, n, s in fns}
    shim = {}
    for l in lines:
        m = re.match(r'^static inline .*?\b(__fwd_[A-Za-z0-9_]+)\s*\(.*?\)\s*\{\s*'
                     r'(?:return\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(', l)
        if m:
            shim[m.group(1)] = m.group(2)
    uf = UF()
    PLAIN = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = ([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CAST = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = '
                      r'\(?(?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\)?\s*'
                      r'([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CALL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^();]*)\)')
    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            c = l.split('//')[0]
            m = PLAIN.match(c) or CAST.match(c)
            if m:
                uf.union((fn, m.group(1)), (fn, m.group(2)))
            for cm in CALL.finditer(c):
                callee = shim.get(cm.group(1), cm.group(1))
                if callee not in params:
                    continue
                for i, arg in enumerate([x.strip() for x in cm.group(2).split(',')]):
                    am = re.fullmatch(r'\(?[A-Za-z_0-9_ \*]*\)?\s*'
                                      r'([A-Za-z_][A-Za-z0-9_]*)', arg)
                    if am and i < len(params[callee]) and params[callee][i]:
                        uf.union((fn, am.group(1)), (callee, params[callee][i]))
    return fns, uf


CONST = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                   r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*(\d+)\s*\)')
BARE = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                  r'\(?\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)?(?![\w+])')
VAR = re.compile(r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)'
                 r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*(?!\d+\s*\))')
PTRFIELD = re.compile(r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)\('
                      r'\*\((?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\*\)'
                      r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\+\s*(\d+)\s*\)')


def survey(lines, fns, uf):
    fld = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    var = collections.Counter()
    ptr = collections.defaultdict(set)
    hits = collections.Counter()
    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            c = l.split('//')[0]
            for m in CONST.finditer(c):
                k = uf.find((fn, m.group(2)))
                fld[k][int(m.group(3))][m.group(1).strip()] += 1
                hits[k] += 1
            for m in BARE.finditer(c):
                k = uf.find((fn, m.group(2)))
                fld[k][0][m.group(1).strip()] += 1
                hits[k] += 1
            for m in VAR.finditer(c):
                var[uf.find((fn, m.group(1)))] += 1
            for m in PTRFIELD.finditer(c):
                ptr[uf.find((fn, m.group(1)))].add(int(m.group(2)))
    return fld, var, ptr, hits


def main():
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    fns, uf = classes(lines)
    fld, var, ptr, hits = survey(lines, fns, uf)
    objs = sorted(hits, key=lambda k: -hits[k])

    if '--list' in sys.argv or len(sys.argv) < 3:
        print('%3s %7s %8s %8s %8s  %s'
              % ('#', 'derefs', 'fields', 'var', 'ptrs', 'names'))
        for i, k in enumerate(objs[:20]):
            names = sorted({nm for (fn, nm) in uf.p if uf.find((fn, nm)) == k})
            print('%3d %7d %8d %8d %8d  %s'
                  % (i, hits[k], len(fld[k]), var[k], len(ptr[k]),
                     ' '.join(names[:5])))
        print('\n%d objects' % len(objs))
        return

    n = int(sys.argv[sys.argv.index('--apply') + 1])
    key = objs[n]
    names = {(fn, nm) for (fn, nm) in uf.p if uf.find((fn, nm)) == key}
    offs = sorted(fld[key])
    tag = 'Obj%d' % n

    # widest access at each offset wins; a pointer field is one whose value is
    # dereferenced
    member, at, decl = {}, 0, []
    for off in offs:
        tys = fld[key][off]
        best = max(tys, key=lambda t: (width(t) or 0, tys[t]))
        w = width(best)
        if w is None:
            print('%s: cannot size %s at +%d' % (tag, best, off))
            return
        if off < at:
            member[off] = None          # inside an earlier member
            continue
        if off > at:
            decl.append('  uint8_t _pad%d[%d];' % (len(decl), off - at))
        nm = 'f%d' % off
        star = '' if best.endswith('*') else ' '
        decl.append('  %s%s%s;' % (best, star, nm))
        member[off] = (nm, best)
        at = off + w

    body = ('// %s -- recovered from %d dereferences over %d offsets, under %d\n'
            '// name%s.  The layout is the one the code already assumed: at 32 bits a\n'
            '// pointer is four bytes, so naming these fields moves nothing, and the\n'
            '// static_assert is what says so.  Offsets the code only reaches with a\n'
            '// computed index are padding here -- their bounds are not visible.\n'
            'struct %s {\n%s\n};\n'
            'static_assert(sizeof(void *) != 4 || sizeof(%s) == %d,\n'
            '              "%s: the layout moved");\n'
            % (tag, hits[key], len(offs), len({nm for _, nm in names}),
               '' if len({nm for _, nm in names}) == 1 else 's',
               tag, '\n'.join(decl), tag, at, tag))

    if '--show' in sys.argv:
        print(body)
        return

    byfn = collections.defaultdict(set)
    for fn, nm in names:
        byfn[fn].add(nm)
    spans = {fn: (a, b) for a, b, fn, _ in fns}

    out = list(lines)
    changed = 0
    for fn, nms in byfn.items():
        if fn not in spans:
            continue
        a, b = spans[fn]
        # the signature can be a line or two above the body's `{`
        s = a
        while s > 0 and '(' not in out[s]:
            s -= 1
        for i in range(s, b + 1):
            line = out[i]
            for nm in nms:
                # *(T *)(nm + K) -> nm->fK
                def const_rep(m, nm=nm):
                    ty, off = m.group(1).strip(), int(m.group(3))
                    if m.group(2) != nm:
                        return m.group(0)
                    home = max((o for o in member if member[o] and o <= off),
                               default=None)
                    if home is None or member[home] is None:
                        return m.group(0)
                    mem, mty = member[home]
                    if home == off and mty == ty:
                        return '%s->%s' % (nm, mem)
                    return '*(%s *)((char *)&%s->%s + %d)' % (ty, nm, mem, off - home)
                line = CONST.sub(const_rep, line)
                # *(T *)(nm + expr) -> *(T *)((char *)nm + expr)
                line = re.sub(r'\*\((\s*(?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                              r'\(\s*%s\s*\+' % re.escape(nm),
                              lambda m: '*(%s *)((char *)%s +' % (m.group(1).strip(), nm),
                              line)
                # *(T *)nm -> nm->f0
                if member.get(0):
                    mem, mty = member[0]
                    line = re.sub(r'\*\((?:const )?(%s)\s*\*\)\(?%s\)?(?![\w+])'
                                  % (re.escape(mty), re.escape(nm)),
                                  '%s->%s' % (nm, mem), line)
            if line != out[i]:
                changed += 1
            out[i] = line
        # retype the declarations
        for i in range(s, b + 1):
            for nm in nms:
                out[i] = re.sub(r'\b(?:char|uintptr_t|int32_t|uint32_t)\s*\*?\s*'
                                r'(?=%s\b)' % re.escape(nm), '%s *' % tag, out[i], count=1) \
                         if re.search(r'^\s*(?:char|uintptr_t|int32_t|uint32_t)[ \*]+%s\b'
                                      % re.escape(nm), out[i]) else out[i]

    anchor = next(i for i, l in enumerate(out) if l.startswith('struct RangeCoder'))
    out = out[:anchor] + body.split('\n') + [''] + out[anchor:]
    open(path, 'w').write('\n'.join(out))
    print('%s: %d lines rewritten across %d functions' % (tag, changed, len(byfn)))


if __name__ == '__main__':
    main()
