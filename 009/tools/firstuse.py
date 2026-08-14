#!/usr/bin/env python3
"""Declare a local where it is first assigned, not at the top of the body.

    python3 tools/firstuse.py                 # every .inc, what it would take
    python3 tools/firstuse.py model.inc --list
    python3 tools/firstuse.py --apply

CLEANER.md Phase 3.  Six bodies still open with more than a hundred
declarations, and a name declared 400 lines above its only assignment tells the
reader nothing except that the decompiler put it there.

This is the same rule `MINIMAL-SYNTAX.md` Phase 5 ran by hand, written down so
the round after this one can re-take it.  Two passes:

  * **same depth** -- the single assignment is at the declaration's own brace
    depth, so the declaration moves *down* onto it;
  * **deeper** -- the assignment is inside a nested block and every mention of
    the name is inside that block too, so the declaration moves *in*.

**What it declines, and why each one is a defect and not a preference.**

  * **more than one assignment.**  Moving the declaration onto the first leaves
    the second assigning a different object if the first is inside a loop.
  * **a first mention that is not a plain `name = expr;`.**  A compound
    assignment, a `&name`, a use as an argument -- each needs the old value.
  * **a `case` or a label between the declaration and the assignment.**  C++
    forbids jumping into the scope of a variable with an initialiser; the
    compiler would catch it, but a tool that hands the compiler a program it
    knows is ill-formed is not reporting, it is guessing.
  * **the innermost enclosing block is a struct body.**  A declaration there is
    a *member*.  `MINIMAL-SYNTAX.md` records `SymEntry::cnt` being deleted this
    way, and the near-miss that matters: asking whether *any* enclosing block is
    a struct excludes every local of every method, which reads as a much
    smaller phase rather than as a bug.
  * **`do { … } while( name … );`**  The condition is outside the block's
    declarative region even though it sits between the braces and the `;`.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# A declaration statement: a type, then names, no initialiser.  `alignas` and
# function-pointer declarators are left alone for the reasons compact_locals.py
# gives -- the attribute would spread, and the syntax does not survive a split.
DECL = re.compile(r'^(\s*)((?:const\s+)?[A-Za-z_]\w*(?:\s*\*)*)\s+([^=;]+);\s*$')
NAME = re.compile(r'^\s*(\*?\s*)([A-Za-z_]\w*)\s*$')


def depths(lines):
    """Brace depth *before* each line, and whether each `{` opens a struct."""
    d, out, kinds, stack = 0, [], [], []
    for l in lines:
        out.append(d)
        code = l.split('//')[0]
        struct_here = re.search(r'\b(struct|class|union|enum)\b', code) is not None
        for c in code:
            if c == '{':
                stack.append(struct_here)
                d += 1
            elif c == '}':
                d -= 1
                if stack:
                    stack.pop()
        kinds.append(tuple(stack))
    return out, kinds


def block_of(lines, dep, at, want):
    """(start, end) of the innermost block at depth `want` containing `at`."""
    a = at
    while a > 0 and dep[a] >= want:
        a -= 1
    b = at
    while b + 1 < len(lines) and dep[b + 1] >= want:
        b += 1
    return a, b


def candidates(lines, a, b):
    """[(decl_line, name, star, type, assign_line, kind)] inside body a..b."""
    dep, kinds = depths(lines)
    base = dep[a] + 1
    body = lines[a:b + 1]
    out = []
    for i in range(a + 1, b):
        if dep[i] != base:
            continue
        m = DECL.match(lines[i])
        if not m or '(' in lines[i] or 'alignas' in lines[i] or 'return' in lines[i]:
            continue
        indent, ty, rest = m.groups()
        if ty.split()[-1].rstrip('*') in ('return', 'else', 'case', 'goto'):
            continue
        names = [NAME.match(n) for n in rest.split(',')]
        if not all(names):
            continue
        # `SymEntry* list_prev, * list_sym;` declares two `SymEntry*`.  The
        # regex takes `SymEntry*` as the type, so the star belongs to the first
        # declarator and only to it -- carrying it into the type as well made
        # every later name a `SymEntry**`, which is a compile error and was.
        ty_base = ty.replace('*', '').strip()
        lead = '*' * ty.count('*')
        for slot, nm in enumerate(names):
            star, n = ((lead if slot == 0 else '')
                       + nm.group(1).replace(' ', '')), nm.group(2)
            ty_k = ty_base
            hits = [h for h in range(a, b + 1)
                    if h != i and re.search(r'(?<![\w.>])%s(?![\w])' % n, lines[h])]
            if not hits:
                continue
            asg = [h for h in hits
                   if re.match(r'^\s*%s\s*=[^=]' % n, lines[h])]
            if len(asg) != 1 or asg[0] != hits[0]:
                continue
            j = asg[0]
            rhs = re.match(r'^\s*%s\s*=\s*(.*);\s*$' % n, lines[j])
            if not rhs or re.search(r'(?<![\w.>])%s(?![\w])' % n, rhs.group(1)):
                continue
            # A label anywhere the new declaration would govern -- not just
            # between the old declaration and the assignment -- is a `goto`
            # that can enter the scope past the initialiser.  g++ takes that
            # under -fpermissive with a warning, so the build stays green and
            # the variable is simply uninitialised on that path: four sites in
            # `model.inc` alone, all jumping to one LABEL_74.
            def labelled(lo, hi):
                return any(re.match(r'^\s*(case\b|default:|[A-Za-z_]\w*:\s*$)',
                                    lines[h]) for h in range(lo, hi + 1))
            if labelled(i + 1, j):
                continue
            # `if( !f_DEC )` with an unbraced body: moving the declaration onto
            # the next line makes the declaration the whole substatement, which
            # C++ forbids -- the name is then not in scope anywhere.  g++ says
            # only "not declared in this scope" 9 lines later, so the shape has
            # to be recognised here.
            prev = lines[j - 1].split('//')[0].rstrip() if j else ''
            if re.match(r'^\s*(\}\s*)?(if|else if|for|while)\s*\(', prev) \
               and prev.endswith(')') or re.match(r'^\s*(\}\s*)?else\s*$', prev):
                continue
            if dep[j] == base:
                if labelled(j, b):
                    continue
                out.append((i, n, star, ty_k, j, 'same', rhs.group(1)))
                continue
            if kinds[j][-1:] == (True,):          # innermost block is a struct
                continue
            s, e = block_of(lines, dep, j, dep[j])
            if not all(s <= h <= e for h in hits):
                continue
            # `} while( g0+1<15 );` puts the condition on the *closing brace's
            # own line* as often as on the next one, and the closing brace is
            # inside the block this walks.  Checking only the line after it
            # moved a declaration into a `do` whose condition then could not
            # see it -- caught by the compiler, which is not where a rule with
            # a written-down guard should be caught.
            for tail in (lines[e], lines[e + 1] if e + 1 < len(lines) else ''):
                if re.search(r'\bwhile\s*\(', tail) and \
                   re.search(r'(?<![\w.>])%s(?![\w])' % n, tail):
                    break
            else:
                if not labelled(j, e):
                    out.append((i, n, star, ty_k, j, 'deep', rhs.group(1)))
            continue
    return out


def scan(path):
    lines = open(path).read().split('\n')
    found = []
    for a, b, nm, _sig, _d in structs.defs(lines):
        found += [(nm,) + c for c in candidates(lines, a, b)]
    return lines, found


def apply(lines, found):
    drop = {}
    put = {}
    for _fn, i, n, star, ty, j, _kind, rhs in found:
        drop.setdefault(i, []).append(n)
        indent = re.match(r'^(\s*)', lines[j]).group(1)
        put[j] = '%s%s %s%s = %s;' % (indent, ty, star, n, rhs)
    out = []
    for i, l in enumerate(lines):
        if i in drop:
            m = DECL.match(l)
            indent, ty, rest = m.groups()
            base = ty.replace('*', '').strip()
            lead = '*' * ty.count('*')
            decls = []
            for slot, x in enumerate(rest.split(',')):
                mm = NAME.match(x)
                decls.append(((lead if slot == 0 else '')
                              + mm.group(1).replace(' ', ''), mm.group(2)))
            keep = [d for d in decls if d[1] not in drop[i]]
            if keep:
                out.append('%s%s %s;' % (indent, base, ', '.join(
                    ('%s%s' % d) if d[0] else d[1] for d in keep)))
            continue
        out.append(put.get(i, l))
    return out


def main():
    paths = [a for a in sys.argv[1:] if not a.startswith('--')]
    if not paths:
        import glob
        paths = sorted(glob.glob('*.inc'))
    total = {'same': 0, 'deep': 0}
    for p in paths:
        lines, found = scan(p)
        for f in found:
            total[f[6]] += 1
            if '--list' in sys.argv:
                print('  %-22s %s:%d -> %d  %s' % (f[0], p, f[1] + 1, f[5] + 1, f[2]))
        if found and '--apply' in sys.argv:
            open(p, 'w').write('\n'.join(apply(lines, found)))
    print('%d locals can be declared at first use (%d at the same depth, '
          '%d one block in)' % (sum(total.values()), total['same'], total['deep']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
