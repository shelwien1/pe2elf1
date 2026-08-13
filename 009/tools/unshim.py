#!/usr/bin/env python3
"""Inline the `__fwd_*` shims and call the function they call.

    python3 tools/unshim.py subs1.hpp --list
    python3 tools/unshim.py subs1.hpp --all

Round one gave every cross-function call a shim, because the extraction did not
agree with itself about argument types:

    static inline FILE *__fwd_bmf_destroy_archive_bmf_close_archive(void *a0)
    { return __bmf_close_archive((BmfArc *)a0); }
    ...
    __fwd_bmf_destroy_archive_bmf_close_archive(Block);

94 of them, one per call site, each one `void *` in and a cast out.  Now that
the types on both sides are the same the shim says nothing: the call becomes
`__bmf_close_archive((BmfArc *)Block)`, and `uncast.py` takes the cast when the
compiler agrees it is useless.

The rewrite substitutes the call's arguments into the body's casts positionally,
so it is exact by construction -- the shim performed those casts and nothing
else.  A shim whose body is not one call of one function is left alone.
"""
import re
import sys

SHIM = re.compile(
    r'^static inline ([\w ]+\**)\s*(__fwd_\w+)\(([^)]*)\)\s*\{\s*'
    r'(?:return\s+)?(\w+)\((.*)\);\s*\}$')


def split(s):
    """Top-level comma split."""
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def shims(lines):
    """{name: (line, callee, [param], [arg-template])}"""
    out = {}
    for i, line in enumerate(lines):
        m = SHIM.match(line.strip())
        if not m:
            continue
        params = [p.strip().split()[-1].lstrip('*') for p in split(m.group(3))]
        out[m.group(2)] = (i, m.group(4), params, split(m.group(5)))
    return out


def call(lines, i, name):
    """(start, end) of the call's parentheses, spanning lines if it has to."""
    j = lines[i].find(name + '(')
    if j < 0:
        return None
    k, depth = j + len(name), 0
    text = lines[i]
    while True:
        if k >= len(text):
            return None
        if text[k] == '(':
            depth += 1
        elif text[k] == ')':
            depth -= 1
            if depth == 0:
                return j, k, text
        k += 1


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = shims(lines)

    if '--list' in sys.argv and '--all' not in sys.argv:
        for n, (i, callee, params, args) in sorted(found.items()):
            print('%-58s -> %s' % (n.lstrip('_'), callee.lstrip('_')))
        print('%d shims' % len(found))
        return 0

    # Multi-line calls are joined first so one regex sees the whole argument
    # list; the file is reflowed by hand afterwards only where it got long.
    # The declarations go first: each one contains its own name, so a rewrite
    # that ran before them would substitute into the shim it is deleting.
    src = '\n'.join(l for i, l in enumerate(lines)
                    if i not in {v[0] for v in found.values()})
    done = skipped = 0
    for n, (i, callee, params, args) in sorted(found.items(),
                                               key=lambda kv: -len(kv[0])):
        rx = re.compile(r'(?<![\w])%s\(' % re.escape(n))
        out, pos = [], 0
        while True:
            m = rx.search(src, pos)
            if not m:
                break
            k, depth = m.end() - 1, 0
            while k < len(src):
                if src[k] == '(':
                    depth += 1
                elif src[k] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                k += 1
            inner = split(src[m.end():k])
            if len(inner) != len(params):
                skipped += 1
                pos = k
                continue
            sub = dict(zip(params, inner))
            # One pass over the template, not one per parameter: an argument
            # expression can contain another parameter's name -- `a1[1]` is the
            # *caller's* `a1` inside the value substituted for `a0` -- and a
            # second pass would rewrite it.
            pr = re.compile(r'(?<![\w])(%s)(?![\w])'
                            % '|'.join(re.escape(p) for p in params))
            new = [pr.sub(lambda m: sub[m.group(1)], a) for a in args]
            out.append((m.start(), k + 1, '%s(%s)' % (callee, ', '.join(new))))
            pos = k
        for a, b, text in reversed(out):
            src = src[:a] + text + src[b:]
        done += len(out)
    open(path, 'w').write(src)
    print('%d call sites inlined, %d shims, %d left alone' %
          (done, len(found), skipped))
    return 0


if __name__ == '__main__':
    sys.exit(main())
