#!/usr/bin/env python3
"""Write out the conversion a signed/unsigned comparison already performs.

    python3 tools/explicitcmp.py bmf.cpp
    python3 tools/explicitcmp.py bmf.cpp --all

`while ( x < width )` with `x` unsigned and `width` signed does not compare a
signed value with an unsigned one -- C converts `width` to `unsigned` first and
compares two unsigned values.  That conversion is the whole content of
`-Wsign-compare`: the comparison means something a reader has to know the usual
arithmetic conversions to see.

    while ( k < pairs );        ->    while ( k < (uint32_t)pairs );

Nothing changes.  The cast is the conversion the compiler was already emitting,
written where it happens, and that is the only claim this makes -- it does not
decide whether the comparison is *right*, which is a question about whether the
signed side can be negative and belongs to a retyping round.

The sites come from `warn.log`, so `BMF_WARN=1 ./build.sh` has to have run.
GCC points its column at the operator and names the two types in order, which
is what makes this mechanical: the operand to wrap is whichever side the second
type belongs to, and its extent is read by scanning out from the operator to
the first unparenthesised boundary.

Comparisons whose operand does not fit on its own line are left alone. Hex-Rays
wraps a long call across several, and an operand with unbalanced parentheses
has no extent this can read -- the first version put a closing parenthesis
inside an argument list.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import buildlog                                                 # noqa: E402

# Why the site list is empty, when it is empty for a reason other than the
# file.  It goes in the summary line: `sweep.sh` wants a zero there, and a
# bare zero against a stale log is the lie this guards against.
NOTE = ['']

WARN = re.compile(r'^(\S+):(\d+):(\d+): warning: comparison of integer '
                  r"expressions of different signedness: '(\w+)'.*?and '(\w+)'")
OPS = ('<=', '>=', '==', '!=', '<', '>')
# Where an operand stops when scanning out from the operator.
STOP = ('&&', '||', ',', ';', '?', ':')
PRIMARY = re.compile(r'^(?:\([\w\s*]+\)\s*)?[A-Za-z_]\w*'
                     r'(?:\s*(?:->|\.)\s*[A-Za-z_]\w*|\s*\[[^\[\]]*\])*$')


def operand(line, at, right):
    """The extent of the operand on one side of the operator at `at`."""
    if right:
        i, depth = at, 0
        while i < len(line):
            c = line[i]
            depth += (c in '([') - (c in ')]')
            if depth < 0:
                break
            if depth == 0 and (line[i:i + 2] in STOP or c in STOP):
                break
            i += 1
        return at, i
    i, depth = at, 0
    while i > 0:
        c = line[i - 1]
        depth += (c in ')]') - (c in '([')
        if depth < 0:
            break
        if depth == 0 and (line[i - 2:i] in STOP or c in STOP or
                           line[i - 1:i + 1] in ('= ',) and line[i - 2] not in '<>!='):
            break
        i -= 1
    return i, at


def sites(path):
    # The log's line numbers are about the file it was built from and nothing
    # else.  Matching on the path's *basename*, which is all this used to do,
    # a copy of `subs1.hpp` satisfies it whatever the copy contains --
    # `tools/buildlog.py` has what that costs.
    out = []
    log, note = buildlog.read('warn.log', path)
    NOTE[:] = [note]
    for l in log:
        m = WARN.match(l)
        if not m or not path.endswith(m.group(1)):
            continue
        out.append((int(m.group(2)), int(m.group(3)), m.group(4), m.group(5)))
    return out


def rewrite(lines, ln, col, lty, rty):
    """(old, new) for one site, or None when the shape is not one to touch."""
    line = lines[ln - 1]
    at = col - 1
    op = next((o for o in OPS if line[at:at + len(o)] == o), None)
    if op is None:
        return None
    signed_right = not rty.startswith('u')
    want = lty if signed_right else rty
    if not want.startswith('u'):
        return None
    if signed_right:
        a, b = operand(line, at + len(op), True)
    else:
        a, b = operand(line, at, False)
    text = line[a:b]
    body = text.strip()
    if not body or body.startswith('(%s)' % want):
        return None
    # A comparison inside the operand means the parenthesisation is a judgement.
    if any(o in body for o in ('&&', '||')):
        return None
    # Hex-Rays wraps a long call across lines, and an operand that runs off the
    # end of its own line has no extent this can read: wrapping it put the
    # closing parenthesis inside an argument list.  One line or nothing.
    if body.count('(') != body.count(')') or body.count('[') != body.count(']'):
        return None
    # The whole operand, not its first term: `(uint32_t)a * b` and
    # `(uint32_t)(a * b)` agree on this target but only the second says which
    # comparison is being made.  Inner parentheses only when the operand is not
    # already a primary, so `(uint32_t)(pairs)` does not happen.
    lead = len(text) - len(text.lstrip())
    trail = len(text) - len(text.rstrip())
    inner = body if PRIMARY.match(body) else '(%s)' % body
    new = (line[:a] + text[:lead] + '(%s)' % want + inner
           + (text[len(text) - trail:] if trail else '') + line[b:])
    return line, new


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    # Rightmost column first: an edit shifts every column after it on its line.
    found = []
    for ln, col, lty, rty in sorted(sites(path), key=lambda s: (-s[0], -s[1])):
        r = rewrite(lines, ln, col, lty, rty)
        if r:
            found.append((ln, r[1]))
            lines[ln - 1] = r[1]

    if '--all' not in sys.argv:
        for ln, new in sorted(found):
            print('%6d  %s' % (ln, new.strip()[:96]))
        print('%d signed/unsigned comparisons whose conversion is implicit%s'
              % (len(found), NOTE[0] and ' (%s)' % NOTE[0]))
        return 0
    open(path, 'w').write('\n'.join(lines))
    print('%d conversions written out' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
