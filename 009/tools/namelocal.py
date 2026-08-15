#!/usr/bin/env python3
"""Name a local after the member it is one assignment of.

    python3 tools/namelocal.py subs1.hpp
    python3 tools/namelocal.py subs1.hpp --all

`unaliasvar.py` folds a local that is a copy of another local and `untemp.py`
folds one that is read once. What is left after both is the local that is a
copy of a *member* and is read several times -- which is worth a name rather
than a fold, because the member expression repeated eight times is worse than
one name:

    v45 = a1->cursor[1];        up = a1->cursor[1];
    ... v45[3].lane[1] ...  →   ... up[3].lane[1] ...

The name comes from the member, and the subscript comes with it when it is a
literal, because `cursor[0]` and `cursor[1]` are different rows and calling
both `cursor` would say they were the same:

    _this->cursor[0]  ->  cursor0        a2->n[1]     ->  n1
    v385->freq[k]     ->  freq           blk->ctx     ->  ctx

It refuses more than it renames:

  * the local must be assigned exactly once in the body, and the whole
    right-hand side must be one member expression, optionally cast;
  * the proposed name must not appear anywhere in the body already -- not as
    another local, not as a parameter, not as a member of something else;
  * the proposed name must not be a file-level identifier, so a local cannot
    start shadowing a global or a function;
  * no two locals in one body may propose the same name.

None of that makes the rename *right* -- a local named `cursor0` is still only
as good as the claim that it holds `cursor[0]` for its whole life, which is
what "assigned exactly once" buys. It makes the rename safe.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

ASSIGN = re.compile(r'^\s*(v\d+) = ([^=;][^;]*);\s*$')
# `&x->m[i]` names after `m` too: the address of an element of a table is
# the table as far as a name is concerned.
MEMBER = re.compile(r'^(?:\([\w ]+\*?\)\s*)?&?(?:\w+(?:->|\.))+(\w+)'
                    r'(?:\[\s*(?:(\d+)|[^\]]*)\s*\])?$')
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)']


def toplevel(lines):
    """Every identifier declared at file scope, which a local must not shadow."""
    out, depth = set(), 0
    for l in lines:
        if depth == 0:
            for m in re.finditer(r'\b(\w+)\s*(?:\[|;|\()', l):
                out.add(m.group(1))
        depth += l.count('{') - l.count('}')
    return out


def candidates(lines):
    top = toplevel(lines)
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        body = '\n'.join(lines[a:b + 1])
        asg, prop = {}, {}
        for i in range(a, b + 1):
            m = ASSIGN.match(lines[i].split('//')[0])
            if m:
                asg.setdefault(m.group(1), []).append((i, m.group(2).strip()))
        for v, rs in asg.items():
            if len(rs) != 1:
                continue
            i, rhs = rs[0]
            if any(re.search(p % re.escape(v), lines[j].split('//')[0])
                   for j in range(a, b + 1) for p in WRITE if j != i):
                continue
            m = MEMBER.match(rhs)
            if not m:
                continue
            name = m.group(1) + (m.group(2) or '')
            # `vNN` and `symNN` are what this is trying to get rid of
            if re.fullmatch(r'(?:v|sym|slot|n)\d+', name):
                continue
            # The member's own name is in the body by definition -- that is
            # where the proposal came from -- so the collision test has to look
            # for it as a *standalone* identifier, not after a `.` or `->`.
            if name in top or re.search(r'(?<![\w.>])%s\b' % re.escape(name), body):
                continue
            prop.setdefault(name, []).append(v)
        for name, vs in prop.items():
            if len(vs) == 1:
                out.append((nm, a, b, vs[0], name))
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, a, b, v, name in found[:30]:
            print('%-24s %-6s -> %s' % (nm.lstrip('_'), v, name))
        print('%d locals nameable after their member' % len(found))
        return 0

    for nm, a, b, v, name in found:
        for i in range(a, b + 1):
            lines[i] = re.sub(r'\b%s\b' % re.escape(v), name, lines[i])
    open(path, 'w').write('\n'.join(lines))
    print('%d locals named after their member' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
