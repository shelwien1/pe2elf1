#!/usr/bin/env python3
"""Delete the `const __m128 &` parameter thread.

    python3 tools/dethread.py subs1.hpp --list
    python3 tools/dethread.py subs1.hpp --all

REFACTORING4.md §2.6 item 1.  `main` declares

    __m128 v3, v4;

without initialising them, and hands them to `bmf_compress`/`bmf_decompress`,
which hand them down fifteen functions and 21 `__fwd_*` shims.  Nothing reads
them: poisoning all 31 entry sites with `__builtin_memset(&a, 0xA5, 16)` and
running the whole gate passes.  They are MSVC's XMM spill slots showing through
a `__fastcall` signature Hex-Rays could not narrow.

The two parameters that *were* read go first, by hand -- `alt_p2_model::a2` is
a local `float` and `search_filter::a3`/`a4` were a 64-bit register copy -- so
what is left is a thread that only forwards itself.

A parameter goes from a declaration and its argument goes from every call, by
position.  Declaration spans are found first and skipped when the calls are
rewritten, so a `__fwd_*` shim -- which is a declaration and a call on one line
-- is handled once as each.  **The compiler is the check**: an arity that does
not match is an error, not a silent miscall.  The gate is still the proof that
nothing moved.
"""
import re
import sys

DECL = re.compile(r'^([\w \*]+?[\w\*] )(__\w+)\(([^;{]*?)\)(?=\s*(?:\{|$))', re.M)


def split_args(s):
    """Top-level comma split, respecting parens and brackets."""
    out, depth, cur = [], 0, ''
    for c in s:
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        if c == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += c
    if cur.strip() or out:
        out.append(cur)
    return out


def decls(src):
    """[(name, [positions], span-of-the-argument-list)] for every definition."""
    out = []
    for m in DECL.finditer(src):
        args = split_args(m.group(3))
        pos = [i for i, a in enumerate(args) if '__m128 &' in a]
        if pos:
            out.append((m.group(2), pos, m.span(3)))
    return out


def rewrite(src):
    ds = decls(src)
    hits = {}
    for name, pos, _ in ds:
        assert hits.setdefault(name, pos) == pos, name
    spans = {a for _, _, (a, _) in ds}

    # calls first, so the declaration spans still hold
    call = re.compile(r'\b(%s)\(' % '|'.join(re.escape(k) for k in hits))
    out, i, n = [], 0, 0
    while True:
        m = call.search(src, i)
        if not m:
            out.append(src[i:])
            break
        j, depth = m.end(), 1
        while depth:
            if src[j] in '([':
                depth += 1
            elif src[j] in ')]':
                depth -= 1
            j += 1
        if m.end() in spans:                      # the declaration itself
            out.append(src[i:j])
            i = j
            continue
        args = split_args(src[m.end():j - 1])
        pos = hits[m.group(1)]
        assert max(pos) < len(args), (m.group(1), args)
        out.append(src[i:m.end()]
                   + ','.join(a for k, a in enumerate(args) if k not in pos)
                   + ')')
        n += 1
        i = j
    src = ''.join(out)

    def strip(m):
        if m.group(2) not in hits:
            return m.group(0)
        args = split_args(m.group(3))
        keep = [a for k, a in enumerate(args) if k not in hits[m.group(2)]]
        return '%s%s(%s)' % (m.group(1), m.group(2), ','.join(keep))
    src = DECL.sub(strip, src)

    # the entry copies, and the uninitialised locals the thread started from
    src, copies = re.subn(r'^ *__m128 \w+ = \w+__ref;\n', '', src, flags=re.M)
    src, locals_ = re.subn(r'^ *__m128 v3, v4;\n', '', src, flags=re.M)
    return src, hits, n, copies, locals_


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()

    if '--list' in sys.argv or '--all' not in sys.argv:
        hits = {n: p for n, p, _ in decls(src)}
        for k in sorted(hits):
            print('%-52s %s' % (k, hits[k]))
        print('%d functions, %d parameters'
              % (len(hits), sum(len(v) for v in hits.values())))
        return 0

    src, hits, n, copies, locals_ = rewrite(src)
    print('%d parameters in %d declarations, %d call sites, %d entry copies, '
          '%d uninitialised locals'
          % (sum(len(v) for v in hits.values()), len(hits), n, copies, locals_))
    open(path, 'w').write(src)
    return 0


if __name__ == '__main__':
    sys.exit(main())
