#!/usr/bin/env python3
# idxgen.py -- shared helpers for the one-time .idx generators: emit() writes a
# module, rewrite() applies exact-string substitutions to a source file and
# fails loudly if a pattern does not match the expected number of times.


def emit(path, prefix, banner, blocks):
    out = ['', 'Prefix %s' % prefix, 'Debug 1', '']
    out += banner
    const = None
    for title, c, items in blocks:
        out.append('')
        for ln in title:
            out.append('# %s' % ln)
        if c != const:
            out.append('Const %d' % c)
            const = c
        out.append('')
        for name, v in items:
            mult = -1 if v < 0 else 1
            out.append('Number %-16s %2d,0!%s' % (name + ',', mult, format(abs(v), 'b')))
    open(path, 'w').write('\n'.join(out) + '\n')


def rewrite(path, subs):
    src = open(path).read()
    for old, new, count in subs:
        n = src.count(old)
        assert n == count, '%s: expected %d of %r, found %d' % (path, count, old, n)
        src = src.replace(old, new)
    open(path, 'w').write(src)


