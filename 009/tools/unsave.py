#!/usr/bin/env python3
"""A value saved into a second name and loaded straight back.

    python3 tools/unsave.py bmf.cpp             # report
    python3 tools/unsave.py bmf.cpp --list      # and every pair
    python3 tools/unsave.py bmf.cpp --declined  # and what stopped each

    magsum_s = magsum;                  int32_t d = ra0->val-ra1->val;
    int32_t d = ra0->val-ra1->val;      ctx_w[3].sel = ...;
    ctx_w[3].sel = ...;          -->    int32_t e = ra0->val-ra0[-1].val;
    int32_t e = ra0->val-ra0[-1].val;   ctx_w[4].sel = ...;
    magsum = magsum_s;
    ctx_w[4].sel = ...;

MSVC spills a register to a stack slot across a region that does not touch it
and loads it back afterwards.  Hex-Rays names the slot, so the round trip
survives as two statements that together say nothing.

**Why this is not `unspillpair.py`.**  That tool wants *every* assignment to the
shadow to be `x = y`, so it can merge the two names.  These slots fail that on
their first line -- `magsum_s` is a fifty-term expression before it is ever a
spill -- and its zero is honest.  The shape here is narrower and the fix is
smaller: not "merge two names" but "delete two statements".

**The rule.**  `A = B;` at some line and `B = A;` at a later line, with

  * `A` and `B` both plain locals of the body, neither a parameter, a member, a
    global, nor address-taken -- so no call between them can write either;
  * the two statements at the same brace depth, so the load is not inside a
    branch the save is outside of;
  * no assignment to `A` or to `B` anywhere between them, at any depth.  This
    is a textual scan of the whole span, nested blocks included, which is what
    lets the span hold a loop -- one of the three pairs in this tree wraps a
    `do`/`while`.

Then `B = A;` is dead: `B` still holds what it held at the save.  The save is
dead as well only when `A` is read nowhere else in the body -- not between the
two and **not after the load either**, which is the half a first version of
this file got wrong: one of the four pairs here loads back into a name the body
goes on to use, and deleting its save would have deleted the value.  So the
report gives both counts and only says "read nowhere" when both are zero.

**What it cannot see.**  A save and a load in different branches of an `if`, and
a slot whose address is taken.  Neither occurs here; both are declines rather
than silent skips, and `--declined` prints them.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

NAME = r'[A-Za-z_]\w*'
COPY = re.compile(r'^(%s)\s*=\s*(%s)\s*;$' % (NAME, NAME))
WRITE = re.compile(r'(?<![.\w>])(%s)\s*(?:=(?!=)|\+=|-=|\*=|/=|%%=|&=|\|=|\^='
                   r'|<<=|>>=|\+\+|--)' % NAME)
PREINC = re.compile(r'(?:\+\+|--)\s*(%s)' % NAME)
ADDR = re.compile(r'&\s*(%s)\b' % NAME)


def code(line):
    return line.split('//')[0]


def writes(text):
    """Every name this line assigns to, incremented or decremented."""
    out = set()
    for m in WRITE.finditer(text):
        out.add(m.group(1))
    for m in PREINC.finditer(text):
        out.add(m.group(1))
    return out


def depths(lines, a, b):
    """Brace depth at the start of each line of the body."""
    out, d = [], 0
    for i in range(a, b + 1):
        out.append(d)
        t = code(lines[i])
        d += t.count('{') - t.count('}')
    return out


def pairs(lines, a, b, locals_):
    """(save, load, reads_between, decline) for every candidate in one body."""
    dep = depths(lines, a, b)
    found = []
    for i in range(a, b + 1):
        m = COPY.match(code(lines[i]).strip())
        if not m:
            continue
        lhs, rhs = m.group(1), m.group(2)
        if lhs not in locals_ or rhs not in locals_ or lhs == rhs:
            continue
        for j in range(i + 1, b + 1):
            n = COPY.match(code(lines[j]).strip())
            if not n or (n.group(1), n.group(2)) != (rhs, lhs):
                # A write to either name before the load ends the search.
                w = writes(code(lines[j]))
                if lhs in w or rhs in w:
                    break
                continue
            if dep[j - a] != dep[i - a]:
                found.append((i, j, None, 'the load is at another brace depth'))
                break
            span = '\n'.join(code(lines[k]) for k in range(i + 1, j))
            after = '\n'.join(code(lines[k]) for k in range(j + 1, b + 1))
            reads = (len(re.findall(r'\b%s\b' % re.escape(lhs), span)),
                     len(re.findall(r'\b%s\b' % re.escape(lhs), after)))
            found.append((i, j, reads, None))
            break
    return found


def survey(path):
    lines, origin = structs.splice(path)
    out, declined = [], []
    for a, b, name, sig, _d in structs.defs(lines):
        body = '\n'.join(code(l) for l in lines[a:b + 1])
        taken = set(ADDR.findall(body))
        names = set(structs.decl_types(sig, lines, a, b))
        names -= taken
        for i, j, reads, why in pairs(lines, a, b, names):
            row = (name, origin[i], origin[j], reads)
            if why:
                declined.append(row + (why,))
            else:
                out.append(row)
    return out, declined


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/unsave.py bmf.cpp [--list] [--declined]')
    found, declined = survey(sys.argv[1])
    if '--list' in sys.argv:
        for name, at, back, reads in found:
            note = ''
            if reads[0] or reads[1]:
                note = '  (slot read %d time%s between, %d after)' % (
                    reads[0], '' if reads[0] == 1 else 's', reads[1])
            print('  %-24s %s:%s -> %s:%s%s'
                  % (name, at[0], at[1], back[0], back[1], note))
    if '--declined' in sys.argv:
        for name, at, back, _r, why in declined:
            print('  %-24s %s:%s -> %s:%s  %s'
                  % (name, at[0], at[1], back[0], back[1], why))
    clean = sum(1 for _n, _a, _b, r in found if not r[0] and not r[1])
    print('%d values saved and loaded straight back, %d of them with a slot '
          'read nowhere else; %d declined'
          % (len(found), clean, len(declined)))


if __name__ == '__main__':
    main()
