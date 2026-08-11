#!/usr/bin/env python3
"""Rename identifiers in subs1.hpp, one whole-word at a time.

    python3 tools/rename.py subs1.hpp OLD=NEW [OLD=NEW ...]
    python3 tools/rename.py subs1.hpp --file names.txt

Whole-word only, and it refuses rather than guesses:

  * the new name must not already be in the file;
  * a rename that would collide with a *local* of the same name in some body is
    reported and refused, because the two would become the same identifier;
  * `::OLD` is rewritten to `::NEW` along with plain `OLD`, which is how the
    bodies that shadow a global reach past their local.

Renaming a global that some body shadows is the case worth spelling out.  The
decompilation has functions that declare a local named after the global they
also use, and reach the global through `::`.  Renaming the global there is safe
-- both `OLD` and `::OLD` move together and the local keeps its name -- but
renaming the *local* to something the global already answers to is not, and that
is what the collision check catches.

Names in a --file are one `OLD NEW` pair per line; `#` starts a comment.

`--in FUNC` scopes the rename to one function body, signature included.  Almost
every `vNN` in this file is shared -- `v59` is a local in read_bmp and a
different local in thirty other bodies -- so a whole-file rename of one is
almost always wrong, and without this the only renameable locals were the
accidentally unique ones.  Inside a body the check is the same one, asked of
that body: the new name must not already mean something there.

`--funcs` switches to renaming a Hex-Rays function token — `sub_402FE0` — which
is not a whole word: it appears bare in `__sub_402FE0` and twice over in the
forwarding shims, `__fwd_sub_402FE0_sub_403820`.  In that mode the token is
replaced wherever it occurs, so the shim's name follows the functions it names
instead of keeping their addresses.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402


def body_span(lines, fn):
    """The line range of one function, its signature included."""
    for a, b, n, sig in structs.bodies(lines):
        if n == fn and 'static inline' not in sig:
            # bodies() starts at the brace; the parameters are above it
            while a > 0 and '(' not in lines[a]:
                a -= 1
            return a, b
    return None


def rename_in(path, fn, pairs):
    lines = open(path).read().split('\n')
    span = body_span(lines, fn)
    if not span:
        sys.exit('%s: no such function' % fn)
    a, b = span
    text = '\n'.join(lines[a:b + 1])
    # the collision check reads code only.  A comment saying "predictor-2
    # expander" is not a declaration of `predictor`, and refusing on it sends
    # you off to invent a worse name; the rewrite below still covers comments,
    # because a comment naming the variable should follow it.
    code = '\n'.join(l.split('//')[0] for l in lines[a:b + 1])
    for old, new in pairs:
        if not re.search(r'\b%s\b' % re.escape(old), text):
            sys.exit('%s: %s does not appear in it' % (fn, old))
        if re.search(r'\b%s\b' % re.escape(new), code):
            sys.exit('%s: %s already means something in it' % (fn, new))
        # A local may share a name with a struct member, and the substitution
        # below is textual: renaming `rescale_at` to `due` in a body that also
        # says `_this->rescale_at` renames the member access too, and the
        # struct has no such member.  Caught by the build, but only after the
        # rename had been applied to everything else in the body.
        if re.search(r'(?:->|\.)\s*%s\b' % re.escape(old), code):
            sys.exit('%s: %s is also a member name here -- rename it in the '
                     'struct, or pick a local name that is not a member'
                     % (fn, old))
    total = 0
    for old, new in pairs:
        text, k = re.subn(r'\b%s\b' % re.escape(old), new, text)
        print('%-22s -> %-22s %4d' % (old, new, k))
        total += k
    lines[a:b + 1] = text.split('\n')
    open(path, 'w').write('\n'.join(lines))
    print('%d occurrences in %s, %d names' % (total, fn.lstrip('_'), len(pairs)))


def locals_named(text, name):
    """Bodies that declare a local called `name`."""
    pat = re.compile(r'^\s+(?:const\s+|volatile\s+)*[A-Za-z_][A-Za-z0-9_]*'
                     r'[\s*&]+(?:[A-Za-z_][A-Za-z0-9_]*\s*,\s*)*\**%s\b'
                     % re.escape(name), re.M)
    return len(pat.findall(text))


SKIP = 'tools/struct-skip.txt'


def follow_skip_list(pairs, pat):
    """Carry a rename into the objects structs.py has been told to leave alone.

    `structs.py` identifies a declined object by the sorted set of its
    `function:local` pairs, on the reasoning -- written down in its own
    docstring -- that the set "stays the same as long as the names do".  Renames
    are exactly what this file does, and for a long time it did not tell it: by
    the time anyone looked, eight bodies had been renamed and 60 of the 141
    entries named a function no longer in the file, which is to say every object
    that had been tried and rejected was back on the offer list.

    Both halves of an entry can move -- the function token and a local named
    after it -- and the sort order moves with them, so the line is rebuilt
    rather than patched.
    """
    try:
        lines = open(SKIP).read().split('\n')
    except IOError:
        return 0
    out, moved = [], 0
    for l in lines:
        toks = l.split()
        if not toks:
            continue
        new = []
        for t in toks:
            was = t
            for old, nm in pairs:
                t = re.sub(pat(old), nm, t)
            moved += was != t
            new.append(t)
        out.append(' '.join(sorted(new)))
    if moved:
        open(SKIP, 'w').write('\n'.join(out) + '\n')
    return moved


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    pairs = []
    args = sys.argv[2:]
    if args and args[0] == '--in':
        scope, args = args[1], args[2:]
        for a in args:
            old, _, new = a.partition('=')
            if not new:
                sys.exit('expected OLD=NEW, got %r' % a)
            pairs.append((old, new))
        return rename_in(path, scope, pairs)
    funcs = False
    if args[0] == '--funcs':
        funcs = True
        args = args[1:]
    if args[0] == '--file':
        for line in open(args[1]):
            line = line.split('#')[0].split()
            if len(line) == 2:
                pairs.append(tuple(line))
    else:
        for a in args:
            old, _, new = a.partition('=')
            if not new:
                sys.exit('expected OLD=NEW, got %r' % a)
            pairs.append((old, new))

    text = open(path).read()
    pat = (lambda s: re.escape(s)) if funcs else (lambda s: r'\b%s\b' % re.escape(s))
    for old, new in pairs:
        if not re.search(pat(old), text):
            sys.exit('%s: no such identifier' % old)
        if re.search(pat(new), text):
            sys.exit('%s: %s is already used in the file' % (old, new))
        n = locals_named(text, new)
        if n:
            sys.exit('%s -> %s: %d bodies already declare a local called %s'
                     % (old, new, n, new))

    total = 0
    for old, new in pairs:
        text, k = re.subn(pat(old), new, text)
        print('%-22s -> %-22s %4d' % (old, new, k))
        total += k
    open(path, 'w').write(text)
    print('%d occurrences, %d names' % (total, len(pairs)))
    n = follow_skip_list(pairs, pat)
    if n:
        print('%s: %d entries followed the rename' % (SKIP, n))


if __name__ == '__main__':
    main()
