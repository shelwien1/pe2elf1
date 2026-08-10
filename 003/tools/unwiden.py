#!/usr/bin/env python3
"""Give a cursor the type its dereferences read, not the one it was declared.

    python3 tools/unwiden.py subs1.hpp
    python3 tools/unwiden.py subs1.hpp --all

    uint32_t *v13;
    v13 = &((uint32_t *)this_3)[4 * v11];
    *((uint16_t *)v13 + 49) = 2;

Every read through `v13` is a `uint16_t`, so that is what it points at: the
declaration says `uint16_t *`, the 35 casts become subscripts, and the one
assignment keeps a cast because its right-hand side is still scaled in the
old units.  A minority cast of another width stays a cast -- `*((uint8_t
*)v13 + 111)` is a byte inside the same object and says so.

Only a *narrowing* is taken.  Widening a cursor would make an index step
further than it did, which is §7's first hazard and not a spelling change.
"""
import re, sys, collections
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs, merge
p = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
lines = open(p).read().split('\n')
SC = r'(?:u?int(?:8|16|32|64)_t|float|double)'
apply = '--all' in sys.argv
report = []
for a, b, nm, sig in structs.bodies(lines):
    decls = {}
    for i in range(a, b + 1):
        m = re.match(r'\s+(%s)\s+([\w, *]+);' % SC, lines[i].split('//')[0])
        if m:
            for d in m.group(2).split(','):
                d = d.strip()
                if d.startswith('*'):
                    decls[d.lstrip('* ')] = (m.group(1), i)
    for v, (t, decl) in list(decls.items()):
        e = re.escape(v)
        casts = collections.Counter()
        forms = [r'\*\(\((%s) \*\)%s ([-+]) ([^()]*?)\)' % (SC, e),
                 r'\*\(\((%s) \*\)%s\)' % (SC, e),
                 r'\*\((%s) \*\)%s\b(?![\[\w])' % (SC, e)]
        ok, assigns, hits = True, [], []
        for i in range(a, b + 1):
            if i == decl:
                continue
            c = lines[i].split('//')[0]
            if not re.search(r'(?<![\w.])%s\b' % e, c):
                continue
            spans = []
            for f in forms:
                for m in re.finditer(f, c):
                    casts[m.group(1)] += 1
                    spans.append((m.span(), m))
            rest = c
            for (s_, t_), _ in sorted(spans, reverse=True):
                rest = rest[:s_] + ' ' * (t_ - s_) + rest[t_:]
            for m in re.finditer(r'(?<![\w.])%s\b' % e, rest):
                after = rest[m.end():].lstrip()
                if after.startswith('=') and not after.startswith('=='):
                    assigns.append(i)
                else:
                    ok = False
            hits.append((i, spans))
        if not ok or not casts:
            continue
        u = casts.most_common(1)[0][0]
        if u == t or merge.WIDTH[u] >= merge.WIDTH[t]:
            continue
        report.append((nm.lstrip('_'), v, t, u, casts[u], sum(casts.values()) - casts[u]))
        if not apply:
            continue
        for i, spans in hits:
            code, sep, com = lines[i].partition('//')
            code = re.sub(r'\*\(\(%s \*\)%s ([-+]) ([^()]*?)\)' % (u, e),
                          lambda m: '%s[%s%s]' % (v, '-' if m.group(1) == '-' else '',
                                                  m.group(2).strip()), code)
            code = re.sub(r'\*\(\(%s \*\)%s\)' % (u, e), '*' + v, code)
            code = re.sub(r'\*\(%s \*\)%s\b(?![\[\w])' % (u, e), '*' + v, code)
            if i in assigns:
                code = re.sub(r'(?<![\w.])(%s = )(?!\(%s \*\))' % (e, u),
                              r'\1(%s *)' % u, code)
            lines[i] = code + sep + com
        m = re.match(r'(\s+)(%s)\s+([\w, *]+);(.*)$' % SC, lines[decl])
        names = [x.strip() for x in m.group(3).split(',')]
        keep = [x for x in names if x.lstrip('* ') != v]
        out = []
        if keep:
            out.append('%s%s %s;%s' % (m.group(1), m.group(2), ', '.join(keep), m.group(4)))
        out.append('%s%s *%s;   // was %s *, read only as %s'
                   % (m.group(1), u, v, t, u))
        lines[decl:decl + 1] = out
        b += len(out) - 1
for r in sorted(report, key=lambda x: -x[4])[:16]:
    print('%-24s %-8s %-9s -> %-9s %2d subs, %d other casts' % r)
print(len(report), 'cursors,', sum(r[4] for r in report), 'casts become subscripts')
if apply:
    open(p, 'w').write('\n'.join(lines))
