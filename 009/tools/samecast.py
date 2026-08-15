#!/usr/bin/env python3
"""Drop a cast that tells the reader nothing: two rules.

    python3 tools/samecast.py bmf.cpp
    python3 tools/samecast.py bmf.cpp --apply     # rule 1 only

**Rule 1, a cast to the type the operand already has.**

`(uint8_t)p[-1]` where `p` is `uint8_t *`; `(AltP1Block*)blk2` where `blk2`
already is one.  The decompiler writes these because it cannot see the
declaration it is casting towards, and a round that types something leaves the
casts behind: `(P2Ctx*)cursor[0]` was thirteen sites left over from when that
array was `void *[5]`.

**Locals and parameters, which is what the type table holds.**  A member is not
in it -- the thirteen `cursor` sites above are a member of `AltP2Block` and had
to be read off the declaration by hand -- so this reports 29 where the tree had
42.  Extending `decl_types` to carry the enclosing record's members would find
the rest; that is a change to shared machinery twenty tools import, and it is
not made here.

**`uncast.py` does not cover this, and its zero was honest.**  That tool asks
GCC, which is the right source for the rule it implements: `-Wuseless-cast`
names a cast to the operand's own type where the operand is an lvalue being
bound to a reference.  It says nothing about `(uint8_t)p[-1]` or
`(P2Ctx*)cursor[0]` -- checked directly, on a four-line file with one of each,
and the compiler reports none of them.  So these are a class no gate in the
tree was looking at, and they are 29 sites.

The type comes from `structs.decl_types`, which is why this could not have been
written earlier: that table stopped at the `;` ending a declarator list, so a
local declared *with* an initialiser had no type at all -- which since the
decompiler's output was cleaned up is most of them.

**What rule 1 will not do.**  Only a cast whose operand is a plain name,
possibly subscripted: `(T*)expr` where `expr` is an expression is a
reinterpretation and this has no business guessing at it.  A name the type
table does not know is skipped rather than assumed.  And a subscript peels
exactly one level of pointer -- `uint8_t *p` makes `p[k]` a `uint8_t`, and a
name with more subscripts than stars is skipped, because a `[4][4]` this cannot
see the shape of is a name it does not know the type of.

**Rule 2, a read of a variable through a pointer to its own storage.**
`*(int32_t*)&alphabet` where `alphabet` is a `uint32_t` is the decompiler's way
of writing `(int32_t)alphabet`, taking the variable's address to do it.  Eight
of these were in the tree and two of the eight were `++*(int32_t*)&x`, which is
not a signedness change at all -- incrementing a `uint32_t` and incrementing
its own storage as an `int32_t` are the same bits, so those are `++x`.

Reported, not applied: the rewrite is `(T)x` for a read, `++x` for an
increment, and `if( x )` where the pun only asks whether the value is nonzero,
and choosing between those three is a reading of the expression rather than a
substitution.  Same width only, and `uncastwidth.py` owns the member case --
`*(T*)&img->stride` is a different question, about a field narrower than the
read.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# `(T)name`, `(T *)name[i]`, `(T)name[i][j]` -- and nothing whose operand is an
# expression.  The trailing guard keeps `(T)name(` out: that is a call.
CAST = re.compile(r'\(\s*([A-Za-z_]\w*\s*\**)\s*\)\s*([A-Za-z_]\w*)'
                  r'((?:\[[^\]\[]*\])*)(?![\w(])')


def peel(ty, subs):
    """The type of `name` under `subs` subscripts, or None if that is unknown."""
    for _ in re.findall(r'\[', subs):
        if not ty.endswith('*'):
            return None
        ty = ty[:-1]
    return ty


def survey(path):
    """[(file, line, cast text, what it should read)]."""
    lines, origin = structs.splice(path)
    out = []
    for a, b, _nm, sig, _d in structs.defs(lines):
        ty = structs.decl_types(sig, lines, a, b)
        for i in range(a, b + 1):
            for m in CAST.finditer(lines[i].split('//')[0]):
                have = ty.get(m.group(2))
                if not have:
                    continue
                have = peel(re.sub(r'\s+', '', have), m.group(3))
                if have and have == re.sub(r'\s+', '', m.group(1)):
                    out.append((origin[i][0], origin[i][1], m.group(0),
                                m.group(2) + m.group(3)))
    return out


# Rule 2: `*(T *)&name`, where `name` is not reached through anything.  The
# lookahead has to exclude `->` as well as `.`, and the first version excluded
# only `.` -- so `*(int32_t*)&blk1->width` read as a pun on the pointer `blk1`
# and nine field reads looked like pointers squeezed through a 32-bit integer.
PUN = re.compile(r'\*\(\s*(u?int(?:8|16|32|64)_t)\s*\*\s*\)\s*&\s*'
                 r'([A-Za-z_]\w*)(?!\s*(?:->|\.|\[|\w))')


def members(lines):
    """{name: type} for every plain scalar member of every struct in the unit."""
    out = {}
    for m in re.finditer(r'^struct\s+(\w+)\s*\{(.*?)^\};', '\n'.join(lines),
                         re.S | re.M):
        for d in re.finditer(r'^\s*((?:const\s+)?[A-Za-z_]\w*)\s+(\w+)\s*;\s*$',
                             m.group(2), re.M):
            out.setdefault(d.group(2), d.group(1))
    return out


def puns(path):
    """[(file, line, text, name, its type)] for rule 2."""
    lines, origin = structs.splice(path)
    mem = members(lines)
    out = []
    for a, b, _nm, sig, _d in structs.defs(lines):
        ty = structs.decl_types(sig, lines, a, b)
        for i in range(a, b + 1):
            for m in PUN.finditer(lines[i].split('//')[0]):
                have = ty.get(m.group(2)) or mem.get(m.group(2))
                if have and structs.width(have) == structs.width(m.group(1)):
                    out.append((origin[i][0], origin[i][1], m.group(0),
                                m.group(2), have))
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/samecast.py bmf.cpp [--apply]')
    found = survey(sys.argv[1])
    punned = puns(sys.argv[1])
    for f, ln, text, name, have in punned:
        print('  %-26s %-26s (%s is %s)' % ('%s:%d' % (f, ln), text, name, have))
    for f, ln, old, _new in found:
        print('  %-26s %s' % ('%s:%d' % (f, ln), old))
    if '--apply' in sys.argv:
        per_file = collections.defaultdict(list)
        for f, ln, old, new in found:
            per_file[f].append((ln, old, new))
        n = 0
        for f, hits in per_file.items():
            src = open(f).read().split('\n')
            # One entry per (line, cast text).  The same cast can appear twice
            # on a line -- `alt_p2_model(outp[k], code2, outp[k]-pred)` -- and
            # `str.replace` takes both, so a second entry would look like a
            # miss and stop the run.
            for ln, old, new in sorted(set(hits)):
                n += src[ln - 1].count(old)
                src[ln - 1] = src[ln - 1].replace(old, new)
            open(f, 'w').write('\n'.join(src))
        print('%d dropped' % n)
        return
    print('%d casts to the type the operand already has, %d reads of a '
          'variable through its own storage' % (len(found), len(punned)))


if __name__ == '__main__':
    main()
