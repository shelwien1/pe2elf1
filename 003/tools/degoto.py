#!/usr/bin/env python3
"""Turn `if (c) goto L; ... L:` back into `if (!c) { ... }`.

    python3 tools/degoto.py subs1.hpp --list
    python3 tools/degoto.py subs1.hpp --apply 1     # the first candidate

REFACTORING.md's Phase 6 expected the file's `goto`s to be four rewritable
shapes -- loop `continue`, loop `break`, early `return`, shared error tail.
Measured, none of them is any of them: no label is followed by a `return`, a
`break` or a `continue`, none is unreachable, and none has its `goto` on the
line above.

Two shapes do rewrite, and both are forward jumps over a region nothing else
enters.  The first is a `goto` that is the *whole* of an `if`: invert the
condition and put the region inside it.  The second is a `goto` that is the
*last statement* of a braced `if` with no `else`, which is the same jump with
statements above it:

    if ( c ) goto L;        R   L:   ->   if ( !c ) { R }
    if ( c ) { S; goto L; } R   L:   ->   if ( c ) { S; } else { R }

The second is exact for the same reason the first is: the `goto` is the last
statement, so a consequent that is entered either leaves by an earlier
`return`/`break` or reaches the `goto`.  It never falls into `R`.

The conditions are checked, not assumed:

  * the `goto` must be the entire consequent of an `if`, on its line or the one
    below -- or the last statement of a braced one that has no `else`;
  * the label must be forward of it and reached by no other `goto`;
  * no label inside the skipped region may be reached from outside it;
  * the region must be one run of complete statements, so it can become a
    block.  *Balance is not the sum:* a region that closes the enclosing
    `while` and then opens an `if` sums to zero, and moving that into an `else`
    left the loop unterminated.  The running depth has to stay non-negative as
    well as end at zero, and it did not until a candidate at `4481630` was
    applied and read.

Anything else is left alone.  A `goto` that jumps backwards is a loop the
decompiler could not name, and a forward one over a region with its own entries
is genuine irreducible flow; neither is improved by being rewritten blind.

`--why` says which condition each rejected label fails, and prints the counts
rather than this file quoting them: a count of zero candidates is a statement
about *this* rule and not about the `goto`s, and the breakdown is what turns
"irreducible" into a measurement.  The categories are shared tails, jumps into
or out of a block, backward jumps, and regions something else enters -- every
one of which needs a flag or a copy of the body to remove, which is a change to
the control structure rather than a spelling of it.

Both shapes report zero on this file and both have a case in its history: the
`else` shape fires at `b3f71fe` and the inverting one twice in the same
revision.  A rule that has never been shown to fire is indistinguishable from
one that never looks -- `tools/proven.sh` is the general form of that argument.
"""
import re
import sys


def analyse(lines, why=None):
    lab = {}
    for i, l in enumerate(lines):
        m = re.match(r'^(\s*)(LABEL_\d+):\s*$', l)
        if m:
            lab[m.group(2)] = i
    gotos = {}
    for i, l in enumerate(lines):
        # Comments explain jumps that are no longer there; only code has them.
        m = re.search(r'goto (LABEL_\d+);', l.split('//')[0])
        if m:
            gotos.setdefault(m.group(1), []).append(i)

    out = []
    for n, sites in gotos.items():
        j = lab.get(n)
        if j is None:
            note(why, 'no label of that name', n)
            continue
        if len(sites) != 1:
            note(why, 'more than one goto reaches it', n)
            continue
        i = sites[0]
        if j <= i:
            note(why, 'backward: a loop the decompiler could not name', n)
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
            got = tail_of_if(lines, i)
            if got is None:
                note(why, 'the goto is not the whole of an if', n)
                continue
            head, close, cond = got
            region = lines[close + 1:j]
            if not balanced(region):
                note(why, 'the region after the if is not one run of '
                          'statements', n)
                continue
            inner = [m2.group(1) for l2 in region
                     for m2 in [re.match(r'^\s*(LABEL_\d+):', l2)] if m2]
            if any(any(k < i or k > j for k in gotos.get(x, [])) for x in inner):
                note(why, 'something else enters the region it skips', n)
                continue
            out.append((head, i, j, n, cond, close))
            continue
        region = lines[i + 1:j]
        if not balanced(region):
            d = sum(l.count('{') - l.count('}') for l in region)
            note(why, 'jumps %s a block, depth %+d'
                 % ('into' if d > 0 else 'out of', d) if d else
                 'the region it skips is not one run of statements', n)
            continue
        inner = [m2.group(1) for l2 in region
                 for m2 in [re.match(r'^\s*(LABEL_\d+):', l2)] if m2]
        if any(any(k < i or k > j for k in gotos.get(x, [])) for x in inner):
            note(why, 'something else enters the region it skips', n)
            continue
        out.append((head, i, j, n, cond, None))
    return sorted(out)


def tail_of_if(lines, i):
    """(head, close, cond) when the `goto` on line `i` is the last statement
    of a braced `if` consequent that has no `else`.

    The other shape this file rewrites is a `goto` that is the *whole* of an
    `if`.  Nine of the file's labels failed only because their `goto` had
    statements above it inside the same braces:

        if ( c ) { S; goto L; }   R   L:    ->    if ( c ) { S; } else { R }

    Exact for the same reason the first shape is: the `goto` is the last
    statement, so a consequent that is entered either leaves the block by some
    earlier `return`/`break` or reaches the `goto`.  It never falls into `R`.

    Declined when the `if` already has an `else` -- there is nowhere to put the
    region -- and when the block's head is not a plain `if`, which is what
    keeps a `while` or a `for` from being rewritten as one.
    """
    k = i + 1
    while k < len(lines) and not lines[k].split('//')[0].strip():
        k += 1
    if k >= len(lines) or lines[k].split('//')[0].strip() != '}':
        return None
    close, depth, o = k, 0, k
    while o >= 0:
        c = lines[o].split('//')[0]
        depth += c.count('}') - c.count('{')
        if depth == 0:
            break
        o -= 1
    else:
        return None
    # `else` on the closing line or the next one: no room for the region.
    if re.search(r'\belse\b', lines[close].split('//')[0]):
        return None
    nxt = close + 1
    while nxt < len(lines) and not lines[nxt].split('//')[0].strip():
        nxt += 1
    if nxt < len(lines) and re.match(r'\s*else\b', lines[nxt].split('//')[0]):
        return None
    line = lines[o].split('//')[0].strip()
    m = re.fullmatch(r'if \( (.*) \)\s*\{', line)
    if m:
        return o, close, m.group(1)
    if line == '{':
        m = re.fullmatch(r'if \( (.*) \)', lines[o - 1].split('//')[0].strip())
        if m:
            return o - 1, close, m.group(1)
    return None


def balanced(region):
    """Is `region` a run of complete statements at one depth?

    The sum of `{` minus `}` being zero is not the same question, and taking
    it for the same question produced a wrong rewrite: a region that closes the
    enclosing `while` and then opens an `if` sums to zero, and moving it into
    an `else` left the loop unterminated.  The running depth has to stay
    non-negative as well as end at zero.
    """
    d = 0
    for l in region:
        c = l.split('//')[0]
        for ch in c:
            if ch == '{':
                d += 1
            elif ch == '}':
                d -= 1
                if d < 0:
                    return False
    return d == 0


def note(why, reason, name):
    if why is not None:
        why.setdefault(reason, []).append(name)


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

    if '--why' in sys.argv:
        # A count of zero says nothing about whether the rest could be
        # rewritten by some other rule.  This says which rule each one fails
        # and by how much, so "irreducible" is a measurement rather than a
        # tool's silence.
        why = {}
        analyse(lines, why)
        for reason, names in sorted(why.items(), key=lambda kv: -len(kv[1])):
            print('%3d  %s' % (len(names), reason))
            print('     %s' % ', '.join(sorted(names)[:8]))
        # Count the jumps, not the mentions.  `sum(1 for l in lines if
        # 'goto LABEL_' in l)` reported 91 for a file with 89, because two
        # comments explain a `goto` that is no longer there -- and a headline
        # figure that a comment can move is not a measurement.
        print('%d labels rejected, %d gotos in the file'
              % (sum(len(v) for v in why.values()),
                 sum(1 for l in lines
                     if re.search(r'goto LABEL_\d+;', l.split('//')[0]))))
        return 0

    cands = analyse(lines)

    if '--list' in sys.argv or len(sys.argv) < 3:
        for k, (head, i, j, n, cond, close) in enumerate(cands, 1):
            print('%3d  line %-6d %-10s %3d lines   %s if ( %s )'
                  % (k, head + 1, n, j - (close or i) - 1,
                     'else on' if close else 'invert', cond[:40]))
        print('%d candidates of %d gotos' % (len(cands), sum(
            1 for l in lines if re.search(r'goto LABEL_\d+;', l.split('//')[0]))))
        return

    k = int(sys.argv[sys.argv.index('--apply') + 1]) - 1
    head, i, j, n, cond, close = cands[k]
    if close is None:
        ind = re.match(r'^(\s*)', lines[head]).group(1)
        body = [('  ' + l if l.strip() else l) for l in lines[i + 1:j]]
        new = [ind + 'if ( %s )' % invert(cond), ind + '{'] + body + [ind + '}']
        out = lines[:head] + new + lines[j + 1:]
        open(path, 'w').write('\n'.join(out))
        print('%s: %d lines now inside `if ( %s )`'
              % (n, j - i - 1, invert(cond)))
        return
    # The `else` shape: drop the `goto`, and put the region between the block's
    # `}` and the label into an `else` in its place.
    ind = re.match(r'^(\s*)', lines[close]).group(1)
    body = [('  ' + l if l.strip() else l) for l in lines[close + 1:j]]
    out = (lines[:i] + lines[close:close + 1]
           + [ind + 'else', ind + '{'] + body + [ind + '}']
           + lines[j + 1:])
    open(path, 'w').write('\n'.join(out))
    print('%s: %d lines now in an `else` on `if ( %s )`'
          % (n, j - close - 1, cond))


if __name__ == '__main__':
    main()
