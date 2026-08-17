#!/usr/bin/env python3
"""Give a cursor the type its dereferences read, not the one it was declared.

    python3 tools/unwiden.py bmf.cpp
    python3 tools/unwiden.py bmf.cpp --all

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
if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
    sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                     % __file__.rsplit('/', 1)[-1])
p = sys.argv[1]
lines = open(p).read().split('\n')
pending = []
SC = r'(?:u?int(?:8|16|32|64)_t|float|double)'
apply = '--all' in sys.argv
report = []
for a, b, nm, sig in structs.bodies(lines):
    # Declarations wrap: `int32_t a, b,` on one line and `*c, d;` on the next,
    # and the continuation is where half the cursors are declared.
    decls, i = {}, a
    while i <= b:
        c = lines[i].split('//')[0]
        m = re.match(r'\s+(%s)\s+(.*)$' % SC, c)
        if m:
            names, j = m.group(2), i
            while ';' not in names and j < b:
                j += 1
                names += ' ' + lines[j].split('//')[0].strip()
            for d in names.split(';')[0].split(','):
                d = d.strip()
                if d.startswith('*'):
                    decls[d.lstrip('* ')] = (m.group(1), i, j)
            i = j
        i += 1
    for v, (t, decl, dend) in list(decls.items()):
        e = re.escape(v)
        casts = collections.Counter()
        forms = [r'\*\(\((%s) \*\)%s ([-+]) ([^()]*?)\)' % (SC, e),
                 r'\*\(\((%s) \*\)%s\)' % (SC, e),
                 r'\*\((%s) \*\)%s\b(?![\[\w])' % (SC, e)]
        ok, assigns, hits = True, [], []
        for i in range(a, b + 1):
            if decl <= i <= dend:
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
                before, after = rest[:m.start()].rstrip(), rest[m.end():].lstrip()
                if after.startswith('=') and not after.startswith('=='):
                    assigns.append(i)
                elif re.search(r'\((?:u?int(?:32|ptr)_t|uintptr_t)\)$', before):
                    pass                      # the pointer stored as an integer
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
        pending.append((decl, dend, v, t, u))
for r in sorted(report, key=lambda x: -x[4])[:16]:
    print('%-24s %-8s %-9s -> %-9s %2d subs, %d other casts' % r)
print(len(report), 'cursors,', sum(r[4] for r in report), 'casts become subscripts')
# The declaration edits change line counts, so they run last and from the
# bottom up -- a line number recorded during the scan is only good until the
# first insertion above it.
for decl, dend, v, t, u in sorted(pending, reverse=True):
    e = re.escape(v)
    ind = re.match(r'(\s*)', lines[decl]).group(1)
    for k in range(decl, dend + 1):
        lines[k] = re.sub(r'(?<![\w])\*%s\b\s*,\s*' % e, '', lines[k])
        lines[k] = re.sub(r',\s*\*%s\b(?=\s*;)' % e, '', lines[k])
    drop = [k for k in range(decl, dend + 1)
            if re.fullmatch(r'\s*(?:%s)?\s*(?:\*%s\s*)?;?\s*' % (SC, e), lines[k])]
    for k in reversed(drop):
        del lines[k]
    at = dend + 1 - len(drop)
    lines[at:at] = ['%s%s *%s;   // was %s *, read only as %s' % (ind, u, v, t, u)]
if apply:
    open(p, 'w').write('\n'.join(lines))
