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

`--funcs` switches to renaming a Hex-Rays function token — `sub_402FE0` — which
is not a whole word: it appears bare in `__sub_402FE0` and twice over in the
forwarding shims, `__fwd_sub_402FE0_sub_403820`.  In that mode the token is
replaced wherever it occurs, so the shim's name follows the functions it names
instead of keeping their addresses.
"""
import re
import sys


def locals_named(text, name):
    """Bodies that declare a local called `name`."""
    pat = re.compile(r'^\s+(?:const\s+|volatile\s+)*[A-Za-z_][A-Za-z0-9_]*'
                     r'[\s*&]+(?:[A-Za-z_][A-Za-z0-9_]*\s*,\s*)*\**%s\b'
                     % re.escape(name), re.M)
    return len(pat.findall(text))


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path = sys.argv[1]
    pairs = []
    args = sys.argv[2:]
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


if __name__ == '__main__':
    main()
