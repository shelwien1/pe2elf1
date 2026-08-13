#!/usr/bin/env python3
"""Group the names that denote the same allocation, and describe each object.

    python3 tools/objects.py subs1.hpp            # the objects
    python3 tools/objects.py subs1.hpp --fields N # the field map of object N

REFACTORING.md §4.2 and §7 say the obstacle to typing the structure fields is
not the offsets -- those are mostly constant -- but object identity: `_this` in
one function and `this_4` in another may or may not be the same allocation, and
the source does not say.  It does say it indirectly, and this reads it.

The relation is: two names are the same object if one is assigned from the
other, or if one is passed where the other is the parameter.  Take the
transitive closure of that over the whole file -- through the `__fwd_*` shims,
which are resolved to what they call -- and each class is one allocation, or a
set of allocations used interchangeably, which for typing purposes is the same
thing.

Then every `*(T *)(name + N)` in the class contributes a field at N of width T,
and the class's field map is the union.  Conflicts are reported rather than
resolved: an offset read as both `uint8_t` and `uint32_t` is a union or a
mistake, and which one it is is not this script's call.

It stops at 159 classes, and that is the honest answer rather than a limitation
to work around.  A field-sensitive pass -- treating values loaded from the same
field of the same object as one object -- was tried and merges nothing, because
the classes it would join are already joined by an assignment or a call.  What
keeps the count at 159 is that the program really does hand around that many
distinct allocations; the big ones are the model block and its tables, and
telling which of an object's uint32_t fields point at which of the others is
what reading ALGORITHM.md §9's open functions would settle.
"""
import re
import sys
import collections


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
    out, depth, cur = [], 0, ''
    for ch in m.group(1):
        if ch == '(' or ch == '<':
            depth += 1
        elif ch == ')' or ch == '>':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    out.append(cur)
    names = []
    for p in out:
        pm = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', p.strip())
        names.append(pm.group(1) if pm else None)
    return names


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


def main():
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    fns = bodies(lines)
    params = {n: params_of(sig) for _, _, n, sig in fns}

    # __fwd_X_Y is a one-line shim: resolve it to the Y it calls.
    shim = {}
    for l in lines:
        m = re.match(r'^static inline .*?\b(__fwd_[A-Za-z0-9_]+)\s*\(.*?\)\s*\{\s*'
                     r'(?:return\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*\(', l)
        if m:
            shim[m.group(1)] = m.group(2)

    uf = UF()
    ASSIGN = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = '
                        r'\(?(?:const )?[A-Za-z_][A-Za-z0-9_]*\s*\**\s*\)?\s*'
                        r'([A-Za-z_][A-Za-z0-9_]*);\s*$')
    PLAIN = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*) = ([A-Za-z_][A-Za-z0-9_]*);\s*$')
    CALL = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(([^();]*)\)')

    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            code = l.split('//')[0]
            m = PLAIN.match(code) or ASSIGN.match(code)
            if m:
                uf.union((fn, m.group(1)), (fn, m.group(2)))
            for cm in CALL.finditer(code):
                callee = shim.get(cm.group(1), cm.group(1))
                if callee not in params:
                    continue
                args = [x.strip() for x in cm.group(2).split(',')]
                for i, arg in enumerate(args):
                    am = re.fullmatch(r'\(?[A-Za-z_0-9_ \*]*\)?\s*'
                                      r'([A-Za-z_][A-Za-z0-9_]*)', arg)
                    if not am or i >= len(params[callee]):
                        continue
                    p = params[callee][i]
                    if p:
                        uf.union((fn, am.group(1)), (callee, p))

    # every *(T *)(name + N) and *(T *)name
    DEREF = re.compile(r'\*\((?:const )?([A-Za-z_][A-Za-z0-9_]*\s*\**)\s*\*\)'
                       r'\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*(?:\+\s*(\d+)\s*)?\)')
    acc = collections.defaultdict(lambda: collections.defaultdict(set))
    hits = collections.Counter()
    for a, b, fn, _ in fns:
        for l in lines[a:b + 1]:
            for m in DEREF.finditer(l.split('//')[0]):
                ty, nm, off = m.group(1).strip(), m.group(2), m.group(3)
                key = uf.find((fn, nm))
                hits[key] += 1
                if off is not None:
                    acc[key][int(off)].add(ty)

    objs = sorted(hits.items(), key=lambda kv: -kv[1])
    if '--fields' in sys.argv:
        n = int(sys.argv[sys.argv.index('--fields') + 1])
        key = objs[n][0]
        members = sorted((fn, nm) for (fn, nm) in uf.p if uf.find((fn, nm)) == key)
        print('object %d: %d dereferences, %d names' % (n, hits[key], len(members)))
        print('names:', ', '.join('%s/%s' % m for m in members[:20]))
        print('\n%8s %s' % ('offset', 'types'))
        for off in sorted(acc[key]):
            ts = acc[key][off]
            print('%8d %s%s' % (off, ', '.join(sorted(ts)),
                                '   <-- conflict' if len(ts) > 1 else ''))
        return

    print('%3s %8s %7s %7s %s' % ('#', 'derefs', 'names', 'offsets', 'biggest names'))
    for i, (key, n) in enumerate(objs[:14]):
        members = [nm for (fn, nm) in uf.p if uf.find((fn, nm)) == key]
        common = collections.Counter(members).most_common(4)
        print('%3d %8d %7d %7d %s'
              % (i, n, len(members), len(acc[key]),
                 ' '.join(x for x, _ in common)))
    print('\n%d objects with a dereference, %d names total'
          % (len(objs), len(uf.p)))


if __name__ == '__main__':
    main()
