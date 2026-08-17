#!/usr/bin/env python3
"""Give a local the signedness of the values it actually holds.

    python3 tools/resign.py bmf.cpp
    python3 tools/resign.py bmf.cpp --all

Hex-Rays picks `int` or `unsigned int` per register, not per quantity, so a
count assigned only from `int32_t` expressions arrives declared `uint32_t` and
every assignment to it converts.  That is what the 736 `-Wsign-conversion` and
much of the 502 `-Wconversion` are: not a narrowing anyone intended, just a
declaration that disagrees with its own right-hand sides.

The fix is the declaration, not a cast.  Casting each site away would move the
number and no information -- see REFACTORING9.md section 15 -- so this changes
the type instead, and the streams are what says it was right.

A local qualifies when:

  * it is declared `int32_t` or `uint32_t` in the body, is not a parameter, and
    its address is never taken;
  * the conversion warnings naming it as the destination all say the same
    thing -- the opposite signedness arriving here -- so flipping removes them
    rather than trading them.  A narrowing elsewhere on the same local is
    unaffected: it stays one warning either way;
  * flipping it removes more conversions than it creates.  The warnings *into*
    a local are only half the story: it also flows into other locals still
    declared the old way, and each of those becomes a conversion.  Counting
    only the first half proposed eighteen flips that put the total up;
  * every ordering comparison it takes part in has an other side that cannot
    change meaning under the flip -- an explicit cast, or a non-negative
    literal.  Ordering is where signedness changes the answer rather than the
    bits, and an unprotected other side is what breaks;
  * it is never the left operand of `>>`, `/` or `%`.  Those three are the
    operators whose *meaning* depends on signedness -- an arithmetic shift is
    not a logical one, and signed division truncates toward zero where unsigned
    cannot go negative at all.  Everything else either converts the same bits or
    was already made explicit.

That last exemption is only safe because `explicitcmp.py` has run.  Before it,
`x < width` compared as unsigned by conversion, and flipping `x` would silently
have made it signed; now the comparison carries `(uint32_t)width` and converts
either way round.  The two rules have to go in that order.

The list is what this proposes; `--all` applies it, and the gate is the check.
Fifteen byte-identical streams is a stronger statement about a type change than
any argument this file could make.
"""
import collections
import os
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import buildlog                                                   # noqa: E402

import structs                                                    # noqa: E402
import unreload                                                   # noqa: E402


DRIVEN = 'tools/driven.txt'


def driven(path, who=None):
    """What `tools/resign-drive.sh` last measured about these offers.

    Every candidate listed here can be applied, rebuilt and counted, and the
    driver does exactly that.  A run ending "none of 28 reduces" means
    twenty-eight things were *tried*, not twenty-eight things left to do --
    and nothing said so.  A list of offers with no note that they were tried
    is `liftframe.py`'s defect in REFACTORING9.md section 32.

    The verdict is stamped with the source's checksum through `buildlog`, so
    it goes stale by itself when the file moves under it, which a hardcoded
    list of failures could not.
    """
    rows, note = buildlog.read(DRIVEN, path)
    if note:
        return ''
    # `who` names the caller, because `__file__` here is always this file --
    # `resign_group.py` calls in and would have read `resign.py`'s verdict.
    me = who or __file__.rsplit('/', 1)[-1]
    for r in rows:
        if r.startswith(me + ' '):
            return 'resign-drive.sh: ' + r[len(me) + 1:].strip()
    return ''

# GCC writes the two operands in opposite orders for the two warnings --
# `-Wconversion` says "from A to B", `-Wsign-conversion` says "to B from A" --
# and reading both the same way had every sign conversion backwards, which
# looked like a local being assigned from both signednesses at once.  204 of
# the 319 candidates were rejected as "mixed direction" for that reason alone.
#
# The filename was spelled `subs1.hpp` as a literal, and since the split gcc
# names one of the thirty-seven includes and never that.  A rule that cannot
# match reports zero exactly like a rule with nothing to find, and this one has
# read zero ever since -- `shape.py` and `retype_locals.py` were each fixed for
# the same sentence and this was the third reader of the same log.  The name is
# a group now and the caller compares it against the file it was given, which
# is the only file the line numbers below can mean.
WARN = re.compile(r"^(?:.*/)?([\w.\-]+):(\d+):(\d+): warning: conversion "
                  r"(from|to) '([^']+)'(?: \{aka '[^']*'\})? (?:to|from) '([^']+)'")
HAVE = [False]
STORE = re.compile(r'^\s*([A-Za-z_]\w*)\s*=(?!=)')
# Every width, not just the 32-bit pair.  Hex-Rays picks a signedness per
# register at whatever width the register is used at, and a `uint16_t`
# holding a signed difference is the same mistake as a `uint32_t` one.
FLIP = {'int32_t': 'uint32_t', 'uint32_t': 'int32_t',
        'int16_t': 'uint16_t', 'uint16_t': 'int16_t',
        'int8_t': 'uint8_t', 'uint8_t': 'int8_t',
        'int64_t': 'uint64_t', 'uint64_t': 'int64_t'}
PAIRS = [{'int32_t', 'uint32_t'}, {'int16_t', 'uint16_t'},
         {'int8_t', 'uint8_t'}, {'int64_t', 'uint64_t'}]
SRC = ['subs1.hpp']
# The three operators whose meaning, and not just their operand's bits, depends
# on the signedness of the left-hand side.
ADDR = r'&\s*%s\b(?!\s*(?:->|\.|\[))'
MEANS = r'\b%s\b\s*(?:>>|/|%%)'
# `x < y` and `y < x`, either way round, but not `x == y`.
ORDER = (r'(?:\b%s\b\s*(?:<=|>=|<(?![<=])|>(?![>=]))'
         r'|(?:<=|>=|<(?![<=])|>(?![>=]))\s*\b%s\b)')


def candidates(lines, log='warn.log'):
    bodies = {}
    fn = {}
    for a, b, nm, sig in structs.bodies(lines):
        bodies[nm] = (a, b, sig)
        for i in range(a, b + 1):
            fn[i] = nm

    seen = collections.defaultdict(list)
    # The line numbers in the log are only about the file the log was built
    # from.  Running against a stale one retypes whatever now sits on those
    # lines: it applied 17 changes to the wrong locals and put the warning
    # count *up* by 43, which the ratchet caught and the streams did not.
    # `tools/buildlog.py` is that check, in one place -- it was in two here and
    # missing from the two other tools that read the same logs.
    here = os.path.basename(SRC[0])
    rows = buildlog.rows(log, SRC[0])
    # Whether the log carries the warnings this rule is *about*, which is not
    # the same question as whether it carries any.  `BMF_WARN=1 ./build.sh`
    # passes `-Wsign-compare -Wunused-variable -Wshadow` and not
    # `-Wconversion`, so on the tree's own ratchet log there are none at all --
    # and a bare `0 conversions` against a log with nothing in it to read is
    # the lie `tools/buildlog.py` was written about, one log along.  Rebuild
    # with `BMF_WARN=1 ./build.sh -Wconversion -Wsign-conversion` to give this
    # something to answer.
    HAVE[0] = any(' warning: conversion ' in l for l in rows)
    for l in rows:
        m = WARN.match(l)
        if not m or m.group(1) != here:
            continue
        ln = int(m.group(2))
        nm = fn.get(ln - 1)
        if nm is None:
            continue
        src, dst = ((m.group(6), m.group(5)) if m.group(4) == 'to'
                    else (m.group(5), m.group(6)))
        code = lines[ln - 1].split('//')[0]
        d = STORE.match(code)
        if d:
            seen[(nm, d.group(1))].append((src, dst))
            continue
        # The other direction: a local being *read* into something of the
        # opposite signedness -- an argument to `rc.encode`, whose counts are
        # unsigned, passed a local Hex-Rays declared signed.  The warning's
        # column is the expression being converted, so a bare identifier there
        # is the local to flip, and the flip goes toward the destination.
        col = int(m.group(3)) - 1
        w = re.match(r'[A-Za-z_]\w*', code[col:])
        back = re.search(r'[A-Za-z_]\w*$', code[:col])
        if w and not back and code[col + w.end():col + w.end() + 1] in (',', ')'):
            seen[(nm, w.group(0))].append((src, dst))

    out = []
    for (nm, name), convs in sorted(seen.items()):
        a, b, sig = bodies[nm]
        code = [l.split('//')[0] for l in lines[a:b + 1]]
        ty = unreload.types(code)
        cur = ty.get(name)
        if not cur or cur[1] or cur[0] not in FLIP:
            continue
        want = FLIP[cur[0]]
        # Among the warnings that are about signedness at this width, all must
        # point the same way.  A narrowing elsewhere on the same local -- `x`
        # stored into a `uint16_t` member -- is unaffected by the flip: it stays
        # one warning either way, so it is not a reason to leave the
        # declaration disagreeing with its own right-hand sides.
        same = [(src, dst) for src, dst in convs
                if {src, dst} == {cur[0], want}]
        # Into this local, or out of it -- but not both.  A local that is
        # converted on the way in *and* on the way out is one the flip cannot
        # help: it would only move which end warns.
        if not same or len({dst for _s, dst in same}) != 1:
            continue
        if same[0][1] != cur[0]:
            want = same[0][1]
            if FLIP.get(cur[0]) != want:
                continue
        if not safe(name, cur[0], want, code, sig):
            continue
        # Flipping removes the conversions *into* this local and creates one
        # wherever it flows into something still declared the old way.  A rule
        # that only counted the first half proposed eighteen flips that put the
        # warning count up rather than down, which the ratchet caught and the
        # streams did not -- so count both halves and require a net gain.
        adds = 0
        use = re.compile(r'(?<![\w.])(?<!->)%s\b' % re.escape(name))
        for l in code:
            d = STORE.match(l)
            if d and d.group(1) != name and ty.get(d.group(1)) == (cur[0], 0) \
                    and use.search(l[l.index('=') + 1:]):
                adds += 1
        if adds >= len(same):
            continue
        out.append((nm, a, b, name, cur[0], want, len(convs)))
    return out


DECLARES = re.compile(r'^\s*(?:const\s+|static\s+)*(\w+)[\s*]')


def safe(name, cur, want, code, sig):
    """Can this local's signedness change without changing what the body does?

    Shared with `resign_group.py`, which asks the same question about every
    member of a set that has to agree.
    """
    if re.search(r'\b%s\b' % re.escape(name), sig):
        return False
    body = '\n'.join(code)
    if re.search(ADDR % re.escape(name), body):
        return False
    if re.search(MEANS % re.escape(name), body):
        return False
    return all(protected(other, want) for other in opposites(body, name))


def opposites(body, name):
    """Every operand `name` is ordered against, either way round."""
    out = []
    for m in re.finditer(ORDER % (re.escape(name), re.escape(name)), body):
        rest = body[m.end():] if m.group(0).lstrip().startswith(name[0]) else None
        if rest is not None:
            out.append(rest.split(')')[0].split(';')[0].split('&&')[0].strip())
        else:
            head = body[:m.start()]
            out.append(re.split(r'[(;&|]', head)[-1].strip())
    return [o for o in out if o]


def protected(other, want):
    """True when comparing against this cannot change meaning under the flip."""
    if re.fullmatch(r'\d+', other):            # a non-negative literal
        return True
    return other.startswith('(%s)' % want) or other.startswith('(uint32_t)')


def decl_line(lines, a, b, name, cur):
    """(statement start, line holding `name`) for a declaration of `cur`.

    Both, because the new one-name declaration goes above the *statement* and
    not above the line the name happens to sit on -- Hex-Rays wraps a comma
    list over eight rows, and inserting into the middle of one splits it.
    """
    here = re.compile(r'\b%s\s*(?:,|;|$)' % re.escape(name))
    i = a
    while i <= b:
        code = lines[i].split('//')[0]
        m = DECLARES.match(code)
        if m and m.group(1) == cur:
            j = i
            while j <= b:
                if here.search(lines[j].split('//')[0]):
                    return i, j
                if lines[j].split('//')[0].rstrip().endswith(';'):
                    break
                j += 1
            i = j
        i += 1
    return None


def retype(lines, got, name, cur, want):
    """Move one name out of its declaration into a new one.  Returns 1 if a
    line was inserted, so a caller tracking a body's bounds can follow it.

    Shared with `resign_group.py`.
    """
    top, lo = got
    text = lines[lo]
    for pat in (r'\b%s\s*,\s*' % re.escape(name),
                r'\s*,\s*\b%s\b' % re.escape(name),
                r'\b%s\b' % re.escape(name)):
        new_text, k = re.subn(pat, '', text, count=1)
        if k:
            break
    indent = text[:len(text) - len(text.lstrip())]
    # Nothing left but the type: the whole declaration was this one name.  The
    # comment has to come off before that test and go back on after --
    # `uint32_t n0x10_2;   // a record index` left `uint32_t ;   // ...`, which
    # compiles only under `-fpermissive` and so failed the gate's strict pass
    # rather than the streams.
    bare, _sep, note = new_text.partition('//')
    if re.fullmatch(r'\s*(?:const\s+|static\s+)*%s\s*;?\s*' % cur, bare):
        lines[lo] = '%s%s %s;%s' % (indent, want, name,
                                    ('   //' + note) if note else '')
        return 0
    lines[lo] = new_text
    head = lines[top][:len(lines[top]) - len(lines[top].lstrip())]
    lines.insert(top, '%s%s %s;' % (head, want, name))
    return 1


def tidy(text):
    """Taking the last name off a wrapped line leaves the comma that separated
    it and, when the line held only that name, an orphan `;` under it."""
    text = re.sub(r',\s*,', ',', text)
    text = re.sub(r',[ \t]*\n[ \t]*;', ';', text)
    return re.sub(r',[ \t]*;', ';', text)


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    SRC[0] = path
    lines = open(path).read().split('\n')
    found = candidates(lines)
    # `--limit N` applies the first N of the list, which is what makes the set
    # bisectable when one of them turns out to change a stream.
    lim = next((a for a in sys.argv if a.startswith('--limit=')), None)
    if lim:
        found = found[:int(lim.split('=')[1])]
    # `--only=K` applies the Kth alone.  The net-effect test above cannot see a
    # local flowing into a struct member or a call argument, so for the few it
    # cannot settle the answer is a rebuild -- one candidate, one measurement.
    only = next((a for a in sys.argv if a.startswith('--only=')), None)
    if only:
        k = int(only.split('=')[1])
        found = found[k:k + 1]
    # `--pick=a,b,c` applies several at once.  Some candidates are siblings --
    # `alti2`..`alti7` are six registers holding one quantity -- and each of
    # them converts against the others until they all move, so no single one
    # of them ever pays.
    pick = next((a for a in sys.argv if a.startswith('--pick=')), None)
    if pick:
        want = {int(x) for x in pick.split('=')[1].split(',')}
        found = [f for i, f in enumerate(found) if i in want]

    if '--all' not in sys.argv:
        for nm, _a, _b, name, cur, want, n in found:
            print('%-24s %-16s %-9s -> %-9s %d conversions'
                  % (nm.lstrip('_'), name, cur, want, n))
        was = driven(SRC[0], 'resign.py')
        why = ('' if HAVE[0] else
               ' (no conversion warnings in the log: rebuild with '
               '`BMF_WARN=1 ./build.sh -Wconversion -Wsign-conversion`)')
        print('%d locals declared against their own assignments, %d conversions%s%s'
              % (len(found), sum(f[6] for f in found), why,
                 was and ' (%s)' % was))
        return 0

    # Declarations only: the uses do not mention the type.  One name has to
    # come out of a comma list that Hex-Rays may have wrapped over eight lines,
    # so the name is deleted where it sits and a fresh one-name declaration goes
    # in above the statement.  A first version split the statement in place and
    # left `int32_t a, b, ;` when the list continued on the next line.
    # Bottom-up by body, so an insert never moves a body still to be done.
    done = 0
    grown = collections.Counter()
    for nm, _a, _b, name, cur, want, _n in sorted(found, key=lambda f: -f[1]):
        got = decl_line(lines, _a, _b + grown[nm], name, cur)
        if got is None:
            continue
        grown[nm] += retype(lines, got, name, cur, want)
        done += 1
    open(path, 'w').write(tidy('\n'.join(lines)))
    print('%d locals retyped' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
