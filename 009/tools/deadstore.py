#!/usr/bin/env python3
"""A store to a local that a later store overwrites before anything reads it.

    python3 tools/deadstore.py bmf.cpp             # report
    python3 tools/deadstore.py bmf.cpp --list      # and every store
    python3 tools/deadstore.py bmf.cpp --declined  # and what stopped each

    if( run_left/2 ) {
      runlen_s = runlen;      <-- nothing reads the slot before
      ...                          the store below overwrites it
    }
    ...
    exclude_stage_one();
    runlen_s = runlen;
    ...
    return runlen_s+1;

MSVC spills a register to a stack slot at more than one point of a body, and
Hex-Rays writes every spill out.  Where the paths rejoin, all but the last of
them are stores nothing can read.

**Why this is not `unwrite.py`.**  That tool asks whether a *name* is ever read,
and answers about the variable.  `runlen_s` was read twice, so its zero was
honest; what was dead was one of its two stores.  A dead store hides inside a
live variable, which is why nothing here could see one.

**The rule, and it is deliberately narrow.**  A store to `X` at some line is
dead when

  * `X` is a local of the body, not a parameter, and its address is never
    taken -- so no call between the two stores can read it;
  * the body holds no label, so nothing can jump forward past the later store
    or backward into the span;
  * a later store to `X` sits at the body's **top level** -- directly in the
    function's braces, not inside any `if` or loop -- so every path that leaves
    the first store and stays in the function runs it;
  * and **every read of `X` in the body is below that later store.**  Not just
    "no read in between": a read *above* the first store is reachable from it
    when a loop encloses both, and that is the case this rule would otherwise
    get wrong.

Anything the rule cannot establish is a decline with the reason.  On this tree
that is 275 of 275: 261 have no later store at the body's top level, 12 have a
read at or above one, and 2 are in a body that holds a label.

**Its first eleven findings were all wrong, and both reasons are worth writing
down.**  `return result;` is a name, a space and a name, which is a
declaration's shape -- read as one, it contributed no read, and a live store
two lines above a `return` that reads it looked dead.  And a store that *is*
the body of a braceless `if` is not unconditional, which brace depth cannot
see: `if( code_f<5 )` on one line and the store on the next leaves both at the
same depth.  Eleven candidates, zero survivors, and the rule is narrower for
it.  A tool whose first output is applied rather than checked would have
deleted eleven live stores.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

NAME = r'[A-Za-z_]\w*'
STORE = re.compile(r'^(%s)\s*=(?!=)' % NAME)
ADDR = re.compile(r'&\s*(%s)\b' % NAME)
LABEL = re.compile(r'^\s*(%s)\s*:(?!:)\s*(//.*)?$' % NAME)
# A declaration, with or without an initialiser.  The declared names are not
# reads -- a first version of this file counted `int32_t slot;` as a read of
# `slot`, which declined every candidate whose slot had a declaration above it,
# which is all of them.
DECL = re.compile(r'^(?:(?:const|static|constexpr|inline'
                  r'|alignas\s*\([^)]*\))\s+)*'
                  r'[A-Za-z_][\w:]*(?:\s*<[^;]*>)?[\s*&]+'
                  r'(\**[A-Za-z_]\w*[^;]*);$')
NOT_A_LABEL = ('default', 'public', 'private', 'protected', 'case')
# `return result;` has a name, a space and a name, which is a declaration's
# shape, and reading it as one made the read of `result` invisible -- the first
# finding this file printed was a live store that a `return` two lines below it
# read.  These are statements whatever follows them.
NOT_A_TYPE = ('return', 'else', 'case', 'goto', 'delete', 'throw', 'new',
              'do', 'break', 'continue', 'sizeof', 'typedef', 'using')
# A store that *is* the body of a braceless `if` is not unconditional, and
# brace depth cannot see that: `if( code_f<5 )` on one line and the store on the
# next leaves both at the same depth.  Three of the first eleven findings were
# this.
BRACELESS = re.compile(r'^(?:\}\s*)?(?:else\s+)?(?:if|for|while|switch)\s*\(.*\)$'
                       r'|^(?:\}\s*)?else$|^do$')


def code(line):
    return line.split('//')[0]


def depths(lines, a, b):
    out, d = [], 0
    for i in range(a, b + 1):
        out.append(d)
        t = code(lines[i])
        d += t.count('{') - t.count('}')
    return out


def has_label(lines, a, b):
    for i in range(a, b + 1):
        m = LABEL.match(code(lines[i]))
        if m and m.group(1) not in NOT_A_LABEL:
            return True
    return False


def braceless(lines, i):
    """Is the statement at `i` the body of a control statement with no brace?"""
    j = i - 1
    while j >= 0 and not code(lines[j]).strip():
        j -= 1
    return j >= 0 and bool(BRACELESS.match(code(lines[j]).strip()))


def survey(path):
    lines, origin = structs.splice(path)
    out, declined = [], []
    for a, b, name, sig, _d in structs.defs(lines):
        body = '\n'.join(code(l) for l in lines[a:b + 1])
        taken = set(ADDR.findall(body))
        params = set(re.findall(r'(%s)\s*[,)]' % NAME, sig))
        locals_ = set(structs.decl_types(sig, lines, a, b)) - taken - params
        if not locals_:
            continue
        labelled = has_label(lines, a, b)
        dep = depths(lines, a, b)
        stores, reads = {}, {}
        for i in range(a + 1, b + 1):
            t = code(lines[i]).strip()
            m = STORE.match(t)
            d = None if m else DECL.match(t)
            if d and t.split()[0].rstrip('*&') in NOT_A_TYPE:
                d = None
            if m and m.group(1) in locals_:
                stores.setdefault(m.group(1), []).append(i)
                rest = t[t.index('=') + 1:]
            elif d:
                rest = ''
                for part in d.group(1).split(','):
                    if '=' in part:
                        nm = part.split('=')[0].strip().lstrip('*')
                        if nm in locals_:
                            stores.setdefault(nm, []).append(i)
                        rest += ' ' + part.split('=', 1)[1]
            else:
                rest = t
            for n in set(re.findall(NAME, rest)):
                if n in locals_:
                    reads.setdefault(n, []).append(i)
        for n, at in stores.items():
            if len(at) < 2:
                continue
            top = [i for i in at[1:]
                   if dep[i - a] == 1 and not braceless(lines, i)]
            if not top:
                declined.append((name, origin[at[0]], n,
                                 'no later store at the body top level'))
                continue
            j = top[-1]
            first = [i for i in at if i < j]
            if not first:
                continue
            if labelled:
                declined.append((name, origin[first[0]], n,
                                 'the body holds a label'))
                continue
            below = [r for r in reads.get(n, []) if r <= j]
            if below:
                declined.append((name, origin[first[0]], n,
                                 'the slot is read at or above the later store'))
                continue
            for i in first:
                out.append((name, origin[i], n, origin[j]))
    return out, declined


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/deadstore.py bmf.cpp '
                 '[--list] [--declined]')
    found, declined = survey(sys.argv[1])
    if '--list' in sys.argv:
        for body, at, n, over in found:
            print('  %-24s %s:%s  %s, overwritten at %s:%s'
                  % (body, at[0], at[1], n, over[0], over[1]))
    if '--declined' in sys.argv:
        for body, at, n, why in declined:
            print('  %-24s %s:%s  %s: %s' % (body, at[0], at[1], n, why))
    print('%d stores nothing can read; %d declined'
          % (len(found), len(declined)))


if __name__ == '__main__':
    main()
