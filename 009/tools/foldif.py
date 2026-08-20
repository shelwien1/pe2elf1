#!/usr/bin/env python3
"""Fold an `if` whose condition is now a known constant.

    python3 foldif.py --scan [file...]        find conditions that are constant
    python3 foldif.py bmf.cpp <line> true|false ["note"]

<line> is the 1-based line of the `if (...)`.  With `true` the then-branch is
kept and any `else` deleted; with `false` the then-branch is deleted and the
else-branch promoted.  Braces are matched, so the branches may be blocks or
single statements.

The then-branch keeps its braces when it had them: Hex-Rays output is full of
labels jumped to from inside these blocks, and a bare compound statement is
always safe where an `if` block was.

`--scan` is the half this file was missing.  Folding needs a line number and a
verdict, which means somebody has to have *found* the `if` first -- so for as
long as this tool only folded, its silence in `sweep.sh` meant "nobody asked",
not "there is nothing".

And then the sweep went on not asking for several rounds more, because it runs
`tool <file>` and this answered that with a usage line -- counted as "wants more
arguments" and not as a zero, which was right and still left the scan unrun.  A
file and nothing else is a scan now.  The half that was missing was missing one
level up as well, which is worth knowing about any tool that grew a mode.  `filter_search` had five `if( !deep )` whose `deep` was
assigned `0` three lines above, each making the statement above it dead, and no
tool here said so.

The rule is deliberately narrow: the condition is a bare name or its negation,
the name was assigned a decimal literal at the same brace depth earlier in the
same body, and nothing between the two mentions the name.  Anything wider --
a name assigned in one arm of an `if`, a condition that is an expression --
needs reaching definitions, and a rule that guesses at that is worse than one
that reports less.
"""
import re
import sys


def block_end(lines, i):
    """Index of the last line of the statement starting at line i."""
    s = lines[i].split('//')[0]
    if s.strip().startswith('{') or s.rstrip().endswith('{'):
        d = 0
        j = i
        while j < len(lines):
            t = lines[j].split('//')[0]
            d += t.count('{') - t.count('}')
            if d == 0:
                return j
            j += 1
        raise SystemExit('unbalanced block at %d' % (i + 1))
    # a single statement, possibly spanning lines until its ';'
    j = i
    while j < len(lines) and not lines[j].split('//')[0].rstrip().endswith(';'):
        j += 1
    return j


def scan(path):
    """[(line, name, value)] for `if( name )` whose name is a known constant."""
    lines = open(path).read().split('\n')
    out, const, depth = [], {}, {}
    d = 0
    for i, l in enumerate(lines):
        code = l.split('//')[0]
        m = re.match(r'^\s*(?:const\s+)?(?:\w+\s+)?([A-Za-z_]\w*) = (\d+);\s*$', code)
        if m:
            const[m.group(1)], depth[m.group(1)] = int(m.group(2)), d
        else:
            for n in re.findall(r'(?<![\w.>])([A-Za-z_]\w*)\s*(?:=[^=]|\+\+|--|[-+|&^]=)', code):
                const.pop(n, None)
        c = re.match(r'^\s*(?:\}\s*else\s+)?if\( (!?)([A-Za-z_]\w*) \)\s*\{?\s*$', code)
        if c and c.group(2) in const and depth[c.group(2)] == d:
            v = const[c.group(2)]
            out.append((i + 1, c.group(2), v, bool(c.group(1)) != bool(v)))
        d += code.count('{') - code.count('}')
    return out


def main():
    # A file and nothing else is a scan.  `--scan` was added because this tool's
    # silence in `sweep.sh` meant "nobody asked" rather than "there is nothing"
    # -- and then the sweep went on not asking, because it runs `tool <file>`
    # and this answered that with a usage line.  The half that was missing was
    # still missing, one level up.
    #
    # Folding needs a line number and a verdict, so there is no reading of
    # `foldif.py bmf.cpp` that wants anything but the scan.
    if '--scan' in sys.argv or len(sys.argv) == 2 and not sys.argv[1].startswith('-'):
        import glob
        paths = [a for a in sys.argv[1:] if not a.startswith('--')] or sorted(glob.glob('*.inc'))
        n = 0
        for p in paths:
            for line, name, val, taken in scan(p):
                n += 1
                print('  %s:%d  `%s` is %d here, so this branch is always %s'
                      % (p, line, name, val, 'taken' if taken else 'skipped'))
        print('%d conditions on a name that is a known constant' % n)
        return 0
    if len(sys.argv) < 4:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    path, line, truth = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    note = sys.argv[4] if len(sys.argv) > 4 else None
    lines = open(path).read().split('\n')
    i = line - 1
    if 'if' not in lines[i]:
        raise SystemExit('line %d is not an if: %r' % (line, lines[i]))
    indent = ' ' * (len(lines[i]) - len(lines[i].lstrip()))

    then_start = i + 1
    then_end = block_end(lines, then_start)
    has_else = (then_end + 1 < len(lines)
                and lines[then_end + 1].strip().startswith('else'))
    if has_else:
        e = then_end + 1
        rest = lines[e].strip()[4:].strip()
        if rest:                       # `else if (...)` or `else stmt;`
            else_start, else_end = e, block_end(lines, e)
            if rest.startswith('if'):
                # keep the chain: the else-if becomes a plain if
                else_body = [lines[e].replace('else ', '', 1)] + \
                            lines[e + 1:block_end(lines, e + 1) + 1]
                else_end = block_end(lines, e + 1)
            else:
                else_body = [indent + rest]
        else:
            else_start, else_end = e, block_end(lines, e + 1)
            else_body = lines[e + 1:else_end + 1]
    else:
        else_end, else_body = then_end, []

    head = [indent + '// ' + note] if note else []
    if truth == 'true':
        kept = lines[then_start:then_end + 1]
    elif truth == 'false':
        kept = else_body
    else:
        raise SystemExit('truth must be true or false')

    out = lines[:i] + head + kept + lines[else_end + 1:]
    open(path, 'w').write('\n'.join(out))
    print('folded line %d to %s: %d lines -> %d'
          % (line, truth, else_end - i + 1, len(head) + len(kept)))


main()
