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

  * **more than one assignment, when the declaration would move *in*.**  The
    second assignment would then be to a different object, because the block it
    moved into runs more than once.  Moving *down* inside the declaration's own
    block is a different matter -- same block, same lifetime, same object -- and
    the two were sharing one check until it was measured.
  * **a first mention that is not a plain `name = expr;`.**  A compound
    assignment, a `&name`, a use as an argument -- each needs the old value.
  * **a jump that crosses the new declaration.**  C++ forbids entering the
    scope of a variable past its initialiser, and g++ takes it anyway under
    `-fpermissive`: the build stays green and the variable is uninitialised on
    that path, which is exactly the kind of thing a gate can miss.  What counts
    is a label inside the new scope that some `goto` reaches from outside it --
    not a label anywhere in the body, which is what this asked at first and
    which cost `code_pixel` and `decode_pixel` all 138 of their names.
    "Outside" is by containment, not by line: `goto code_residual` runs
    *backwards* into a scope it is not in.  A `case` declines outright, its
    `switch` head being above it by construction.
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


def blocks(lines):
    """Per line, the stack of block ids open *before* that line is read."""
    out, stack, nxt = [], [], 0
    for l in lines:
        out.append(tuple(stack))
        for c in l.split('//')[0]:
            if c == '{':
                stack.append(nxt)
                nxt += 1
            elif c == '}':
                if stack:
                    stack.pop()
    return out


def block_of(lines, ids, at):
    """(opening line, closing line) of the innermost block containing `at`.

    By brace identity and not by depth.  Depth cannot tell an `if` body from
    the `else` under it -- `} else {` closes one and opens the other, so the
    depth before that line is the same as before the lines either side of it,
    and a walk outward from inside the `if` swallowed the `else` as well.  That
    made two claims wrong at once: that every use of the name was inside the
    block, and that no jump could enter it from outside.  `search_filter`'s
    `goto try_flag8` lives in exactly that `else` and was being read as inside.
    """
    here = ids[at][-1]
    span = [h for h in range(len(lines)) if here in ids[h]]
    return span[0] - 1, span[-1]


def candidates(lines, a, b):
    """[(decl_line, name, star, type, assign_line, kind)] inside body a..b."""
    dep, kinds = depths(lines)
    ids = blocks(lines)
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
            if not asg or asg[0] != hits[0]:
                continue
            j = asg[0]
            # More than one assignment is fatal to the *deeper* pass and
            # harmless to the same-depth one, and the two were sharing a
            # check.  Moving a declaration down inside its own block does not
            # change the object: same block, same lifetime, so a later
            # assignment inside a loop still assigns the same variable.
            # Moving it *in* does change the lifetime, which is what the
            # docstring's reason is about.
            #
            # Worth 38 names on its own.  It was aimed at `code_pixel` and
            # `decode_pixel` and got none of theirs; the guard below was what
            # held those, which is why both were wrong at once.
            if len(asg) != 1 and dep[j] != base:
                continue
            rhs = re.match(r'^\s*%s\s*=\s*(.*);\s*$' % n, lines[j])
            if not rhs or re.search(r'(?<![\w.>])%s(?![\w])' % n, rhs.group(1)):
                continue
            # A jump that enters the new declaration's scope past its
            # initialiser is what C++ forbids.  g++ takes it under
            # -fpermissive with a warning, so the build stays green and the
            # variable is simply uninitialised on that path -- which is why
            # this is checked here and not left to the compiler.
            #
            # The first version of this declined on a label *anywhere* in the
            # body, which is far wider than the rule and cost `code_pixel` and
            # `decode_pixel` all 138 of their names between them.  What matters
            # is whether a jump can land inside the new scope from outside it.
            #
            # "Outside" is containment and not line order, which the second
            # version got wrong: `goto code_residual;` at alt_p2_model.inc:665
            # goes *backwards* to a label at 439, and the declaration it skips
            # is at 422 -- the jump is below the declaration and still enters
            # its scope past the initialiser.  g++ said so under -fpermissive,
            # which is a warning and not an error, so the build was green and
            # `step10` was uninitialised on that path.
            #
            # A `case` declines outright: its `switch` head is above it by
            # construction.
            def skipped(lo, hi):
                for h in range(lo, hi + 1):
                    m = re.match(r'^\s*([A-Za-z_]\w*):\s*$', lines[h])
                    if m:
                        jump = r'\bgoto\s+%s\s*;' % m.group(1)
                        if any(re.search(jump, lines[g])
                               for g in range(a, b + 1) if not lo <= g <= hi):
                            return True
                    elif re.match(r'^\s*(case\b|default:)', lines[h]):
                        return True
                return False
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
                if skipped(j, b):
                    continue
                out.append((i, n, star, ty_k, j, 'same', rhs.group(1)))
                continue
            if kinds[j][-1:] == (True,):          # innermost block is a struct
                continue
            s, e = block_of(lines, ids, j)
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
                if not skipped(j, e):
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
        # `bmf.cpp` too: it is the include list, but it also holds `main` and
        # the entry points, and a default of `*.inc` left five candidates there
        # -- which `sweep.sh` found by handing this a copy of it.
        import glob
        paths = sorted(glob.glob('*.inc')) + ['bmf.cpp']
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
