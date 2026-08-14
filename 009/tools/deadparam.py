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

**What it declines.**  An argument that *does* something.  Dropping
`f(unread_flag)` or `f(16*ctx[0])` deletes a value nobody reads; dropping
`f(g())` or `f(i++)` deletes an effect somebody depends on.  So the test is
for side effects -- a call, an increment, an assignment -- and not, as it was
first written, for the argument being a plain name.  That stricter version
declined four real sites (`16*ctx[0]`, `img_at->height`) whose only sin was
arithmetic.

A decline abandons the parameter, not the site.  Dropping it from the
definition while one call still passes it is a compile error, and a tool whose
partial success does not build is worse than one that says no: any decline
rolls the whole parameter back and reports it, so the tree is always in a
state the gate can judge.

**Splitting the argument list** is the part that is easy to get wrong.
`f(a, g(b, c), d)` has three arguments, and `d[i, j]` is not a comma
expression in any code here but `Type<a, b>` would be.  The split tracks
`()`, `[]` and `{}` depth and string literals, which is enough for this file
and would not be enough for a template.
"""
import re
import sys
import glob

# Three shapes of definition, and the first version of this saw only one.
# Leading whitespace, because `AltP2Block::alt_p2_d8_body` is declared inside
# its class body and a pattern anchored at column zero cannot see it.  A `:`
# ending the return type, because `ModelBlock*ModelBlock::layout_workspace(`
# is an out-of-line method and the name before the paren is the half after the
# `::`.  Between them those two were the two parameters this tool reported
# nothing about while looking like it had checked everything.
DEF = re.compile(r'^[ \t]*([A-Za-z_][\w:*&<>, ]*?[\s*&:])(\w+)\(([^;{)]*)\)\s*\{', re.M)
DEAD = re.compile(r'\b(unread_\w+|unused_\w+)\b')
# A call, an increment, or an assignment.  `(uint8_t*)x` is a cast and not a
# call, which is why the call test wants a word character before the paren.
EFFECT = re.compile(r'\w\s*\(|\+\+|--|(?<![=!<>+\-*/%&|^])=(?!=)')


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
    """Remove parameter `idx` from `fn` -- its definition, declaration, calls.

    All of them or none: `srcs` is only written back once every site is known
    to be droppable.
    """
    # `alt_p2_d8_body` is a template and its calls read `f<1>(...)`, so the
    # explicit argument list has to be allowed between the name and the paren.
    call = re.compile(r'\b%s\s*(?:<[^<>()]*>)?\s*\(' % re.escape(fn))
    edits, hit = {}, 0
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
            # is nothing to hoist.  A call: it has to be free of side effects.
            is_decl = bool(re.match(r'^[A-Za-z_][\w:*&<> ]*[\s*&]\w+$', gone))
            if not is_decl and EFFECT.search(gone):
                declined.append('%-24s %s:%d  `%s` does something'
                                % (fn, path, s[:m.start()].count('\n') + 1, gone))
                return 0
            # Every piece but the first already carries the space that
            # followed its comma, so they are rejoined on the comma alone and
            # only the new first one is stripped.  Joining on `', '` instead
            # doubles the space at every site the removed argument was not
            # last -- `f(a,  b)` -- which compiles and reads like a typo.
            kept = [a for j, a in enumerate(args) if j != idx]
            if kept:
                kept[0] = kept[0].lstrip()
            out.append(s[at:m.end()] + ','.join(kept))
            at = i
            hit += 1
        if out:
            edits[path] = ''.join(out) + s[at:]
    srcs.update(edits)
    return hit


def main():
    srcs = {f: open(f).read() for f in glob.glob('*.inc') + glob.glob('*.cpp')}
    declined, total, stuck = [], 0, set()
    # One at a time, re-deriving the list after each.  Dropping a parameter
    # renumbers the ones to its right, so a list computed once and walked is
    # right for its first entry and wrong for the rest -- which cost two of
    # `__choose_plane_coding`'s and `__expand_predictor_mode0`'s the first time
    # this was written, silently, because a stale index still points at *an*
    # argument.
    while True:
        dead = [d for d in find_dead(srcs) if (d[0], d[2]) not in stuck]
        if not dead:
            break
        fn, idx, name, path = dead[0]
        n = drop(srcs, fn, idx, declined)
        if not n:
            stuck.add((fn, name))
            continue
        total += 1
        print('  %-26s %-12s (%s, %d sites)' % (fn, name, path, n))
    for d in declined:
        print('  DECLINED ' + d)
    print('%d parameters dropped' % total)
    if '--apply' in sys.argv:
        for path, s in srcs.items():
            open(path, 'w').write(s)
        print('-- applied')
    return 0


if __name__ == '__main__':
    sys.exit(main())
