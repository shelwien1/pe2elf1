#!/usr/bin/env python3
"""Turn `if (c) goto L; ... L:` back into `if (!c) { ... }`.

    python3 tools/degoto.py subs1.hpp --list
    python3 tools/degoto.py subs1.hpp --apply 1     # the first candidate

REFACTORING.md's Phase 6 expected the file's `goto`s to be four rewritable
shapes -- loop `continue`, loop `break`, early `return`, shared error tail.
Measured, none of the 123 is any of them: no label is followed by a `return`, a
`break` or a `continue`, none is unreachable, and none has its `goto` on the
line above.

One shape does rewrite, and it is the plainest: a forward `goto` that is the
whole of an `if`, jumping over a region nothing else enters.  Inverting the
condition and putting the region inside it says the same thing without the
label.

The conditions are checked, not assumed:

  * the `goto` must be the entire consequent of an `if`, on its line or the one
    below;
  * the label must be forward of it and reached by no other `goto`;
  * no label inside the skipped region may be reached from outside it;
  * the region must brace-balance on its own, so it can become a block.

Anything else is left alone.  A `goto` that jumps backwards is a loop the
decompiler could not name, and a forward one over a region with its own entries
is genuine irreducible flow; neither is improved by being rewritten blind.
"""
import re
import sys


def analyse(lines):
    lab = {}
    for i, l in enumerate(lines):
        m = re.match(r'^(\s*)(LABEL_\d+):\s*$', l)
        if m:
            lab[m.group(2)] = i
    gotos = {}
    for i, l in enumerate(lines):
        m = re.search(r'goto (LABEL_\d+);', l)
        if m:
            gotos.setdefault(m.group(1), []).append(i)

    out = []
    for n, sites in gotos.items():
        j = lab.get(n)
        if j is None or len(sites) != 1:
            continue
        i = sites[0]
        if j <= i:
            continue
        line = lines[i].strip()
        cond, head = None, i
        m = re.fullmatch(r'if \( (.*) \)\s*goto %s;' % n, line)
        if m:
            cond = m.group(1)
        elif line == 'goto %s;' % n:
            m = re.fullmatch(r'if \( (.*) \)', lines[i - 1].strip())
            if m:
                cond, head = m.group(1), i - 1
        if cond is None:
            continue
        region = lines[i + 1:j]
        if sum(l.count('{') - l.count('}') for l in region) != 0:
            continue
        inner = [m2.group(1) for l2 in region
                 for m2 in [re.match(r'^\s*(LABEL_\d+):', l2)] if m2]
        if any(any(k < i or k > j for k in gotos.get(x, [])) for x in inner):
            continue
        out.append((head, i, j, n, cond))
    return sorted(out)


def invert(cond):
    """!(cond), simplified where the shape is obvious."""
    c = cond.strip()
    m = re.fullmatch(r'(.+?) (==|!=|<|>|<=|>=) (.+)', c)
    if m and '&&' not in c and '||' not in c:
        flip = {'==': '!=', '!=': '==', '<': '>=', '>': '<=',
                '<=': '>', '>=': '<'}[m.group(2)]
        return '%s %s %s' % (m.group(1), flip, m.group(3))
    if c.startswith('!'):
        rest = c[1:].strip()
        # !(x) -> x, and !x -> x, when what follows is a single term
        # !X -> X for anything that is already a single term: a name, a
        # parenthesised expression, either of those subscripted or dotted.
        if re.fullmatch(r'(?:\([^()]*(?:\([^()]*\)[^()]*)*\)'
                        r'|[A-Za-z_][A-Za-z0-9_]*)'
                        r'(?:\[[^\]]*\]|\.[A-Za-z_][A-Za-z0-9_]*'
                        r'|->[A-Za-z_][A-Za-z0-9_]*)*', rest):
            return rest
    if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', c):
        return '!' + c
    return '!(%s)' % c


def main():
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    cands = analyse(lines)

    if '--list' in sys.argv or len(sys.argv) < 3:
        for k, (head, i, j, n, cond) in enumerate(cands, 1):
            print('%3d  line %-6d %-10s %3d lines   if ( %s )'
                  % (k, head + 1, n, j - i - 1, cond[:46]))
        print('%d candidates of %d gotos' % (len(cands), sum(
            1 for l in lines if 'goto LABEL_' in l)))
        return

    k = int(sys.argv[sys.argv.index('--apply') + 1]) - 1
    head, i, j, n, cond = cands[k]
    ind = re.match(r'^(\s*)', lines[head]).group(1)
    body = [('  ' + l if l.strip() else l) for l in lines[i + 1:j]]
    new = ([ind + 'if ( %s )' % invert(cond), ind + '{'] + body + [ind + '}'])
    out = lines[:head] + new + lines[j + 1:]
    open(path, 'w').write('\n'.join(out))
    print('%s: %d lines now inside `if ( %s )`' % (n, j - i - 1, invert(cond)))


if __name__ == '__main__':
    main()
