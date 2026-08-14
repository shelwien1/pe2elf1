#!/usr/bin/env python3
"""Drop the parameters nothing reads, and the arguments that fed them.

    python3 tools/deadparam.py                 # what it would drop
    python3 tools/deadparam.py --apply

Sixteen parameters are named `unread_*` or `unused_*`, eight of them
`unread_flag`.  They are the decompiler's report that BMF.exe's calling
convention pushed a value the callee never loaded -- a real fact about the
binary, and a fossil in a C++ program where every one of these functions is
static and has two or three call sites.

**It has to reach a fixpoint.**  Four of the sixteen are not unused: they are
*forwarded*.  `__alt_model_p1_d8_decode` reads `unread_flag` exactly once, to
pass it to `__rc_begin_decode`, which does not read it at all.  Drop the leaf
and the forwarder becomes a leaf; drop that and its caller does.  Doing one
pass over the sixteen removes seven and leaves a chain of nine looking
load-bearing, which is how a tool talks you out of the work it was written for.

**What it declines.**  An argument that does something.  `f(unread_flag)` is a
name and dropping it changes nothing, but `f(g())` or `f(i++)` at the same
position is an expression whose value happens to be discarded and whose effect
is not.  Anything that is not a plain identifier, number, or cast of one is
reported and left, for a human to hoist out first.  Nothing in this tree hit
that case; the check is here because the next parameter might.

**Splitting the argument list** is the part that is easy to get wrong.
`f(a, g(b, c), d)` has three arguments, and `d[i, j]` is not a comma
expression in any code here but `Type<a, b>` would be.  The split tracks
`()`, `[]` and `{}` depth and string literals, which is enough for this file
and would not be enough for a template.
"""
import re
import sys
import glob

DEF = re.compile(r'^([A-Za-z_][\w:*&<>, ]*?[\s*&])(\w+)\(([^;{)]*)\)\s*\{', re.M)
DEAD = re.compile(r'\b(unread_\w+|unused_\w+)\b')
PLAIN = re.compile(r'^\(?[A-Za-z_]\w*\)?$|^\(?-?\d+\)?$|^\([\w* ]+\)\s*[\w]+$')


def split_args(text):
    """The top-level commas of an argument list, as a list of pieces."""
    out, depth, start, i, quote = [], 0, 0, 0, ''
    while i < len(text):
        c = text[i]
        if quote:
            if c == '\\':
                i += 2
                continue
            if c == quote:
                quote = ''
        elif c in '"\'':
            quote = c
        elif c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
        elif c == ',' and depth == 0:
            out.append(text[start:i])
            start = i + 1
        i += 1
    out.append(text[start:])
    return out


def body_of(s, at):
    """The braced body starting at the `{` on or after `at`."""
    i = s.index('{', at)
    d = 0
    for j in range(i, len(s)):
        if s[j] == '{':
            d += 1
        elif s[j] == '}':
            d -= 1
            if d == 0:
                return s[i + 1:j]
    return s[i + 1:]


def find_dead(srcs):
    """(function, parameter index, parameter name) for every droppable one."""
    out = []
    for path, s in sorted(srcs.items()):
        for m in DEF.finditer(s):
            params = split_args(m.group(3))
            if not any(DEAD.search(p) for p in params):
                continue
            body = body_of(s, m.end() - 1)
            for k, p in enumerate(params):
                d = DEAD.search(p)
                if d and not re.search(r'\b%s\b' % d.group(1), body):
                    out.append((m.group(2), k, d.group(1), path))
    return out


def drop(srcs, fn, idx, declined):
    """Remove parameter `idx` from `fn` -- its definition, declaration, calls."""
    call = re.compile(r'\b%s\s*\(' % re.escape(fn))
    hit = 0
    for path, s in list(srcs.items()):
        out, at = [], 0
        for m in call.finditer(s):
            # the matching close paren
            i, depth = m.end() - 1, 0
            while i < len(s):
                if s[i] == '(':
                    depth += 1
                elif s[i] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            inner = s[m.end():i]
            args = split_args(inner)
            if len(args) <= idx:
                continue
            gone = args[idx].strip()
            # A definition or declaration: the piece is `type name`, and there
            # is nothing to hoist.  A call: it has to be a plain value.
            is_decl = bool(re.match(r'^[A-Za-z_][\w:*&<> ]*[\s*&]\w+$', gone))
            if not is_decl and not PLAIN.match(gone):
                declined.append('%s:%s  %s(... %s ...) is not a plain value'
                                % (path, s[:m.start()].count('\n') + 1, fn, gone))
                continue
            out.append(s[at:m.end()] + ', '.join(a for j, a in enumerate(args)
                                                 if j != idx).lstrip())
            at = i
            hit += 1
        if out:
            srcs[path] = ''.join(out) + s[at:]
    return hit


def main():
    srcs = {f: open(f).read() for f in glob.glob('*.inc') + glob.glob('*.cpp')}
    declined, rounds, total = [], 0, 0
    while True:
        dead = find_dead(srcs)
        if not dead:
            break
        rounds += 1
        for fn, idx, name, path in dead:
            n = drop(srcs, fn, idx, declined)
            total += 1
            print('  round %d  %-26s %-12s (%s, %d sites)' % (rounds, fn, name, path, n))
    for d in declined:
        print('  DECLINED ' + d)
    print('%d parameters dropped over %d rounds' % (total, rounds))
    if '--apply' in sys.argv:
        for path, s in srcs.items():
            open(path, 'w').write(s)
        print('-- applied')
    return 0


if __name__ == '__main__':
    sys.exit(main())
