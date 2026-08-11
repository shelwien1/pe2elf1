#!/usr/bin/env python3
"""Give a local the signedness of the values it actually holds.

    python3 tools/resign.py subs1.hpp
    python3 tools/resign.py subs1.hpp --all

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
  * it is never an operand of `<`, `>`, `<=` or `>=`.  An ordering comparison
    is where signedness changes the answer rather than the bits, and the other
    side is usually a plain local with nothing to protect it;
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
import structs                                                    # noqa: E402
import unreload                                                   # noqa: E402

# GCC writes the two operands in opposite orders for the two warnings --
# `-Wconversion` says "from A to B", `-Wsign-conversion` says "to B from A" --
# and reading both the same way had every sign conversion backwards, which
# looked like a local being assigned from both signednesses at once.  204 of
# the 319 candidates were rejected as "mixed direction" for that reason alone.
WARN = re.compile(r"^subs1\.hpp:(\d+):\d+: warning: conversion "
                  r"(from|to) '([^']+)'(?: \{aka '[^']*'\})? (?:to|from) '([^']+)'")
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
    try:
        rows = open(log).read().split('\n')
    except OSError:
        return []
    # The line numbers in the log are only about the file the log was built
    # from.  Running against a stale one retypes whatever now sits on those
    # lines: it applied 17 changes to the wrong locals and put the warning
    # count *up* by 43, which the ratchet caught and the streams did not.
    #
    # `build.sh` stamps the log with the source's checksum, because mtime is
    # not the check: a `cp` of the source is newer than a log that describes it
    # exactly, and a rebuild for any other reason makes a stale log look fresh.
    # The first version used mtime and silently did nothing through an entire
    # bisection, which reported the change innocent twice.
    stamp = next((r for r in rows if r.startswith('# subs1.hpp ')), None)
    have = os.popen('cksum < %s' % SRC[0]).read().strip()
    if stamp is None or stamp[len('# subs1.hpp '):].strip() != have:
        # Not an error when the file simply is not the one the log is about --
        # `sweep.sh` runs every tool against a copy, and a tool that has
        # nothing to say should say so rather than exit non-zero.
        print('nothing to say about %s: %s describes another file' % (SRC[0], log))
        raise SystemExit(0)
    for l in rows:
        m = WARN.match(l)
        if not m:
            continue
        ln = int(m.group(1))
        nm = fn.get(ln - 1)
        if nm is None:
            continue
        src, dst = ((m.group(4), m.group(3)) if m.group(2) == 'to'
                    else (m.group(3), m.group(4)))
        d = STORE.match(lines[ln - 1].split('//')[0])
        if d:
            seen[(nm, d.group(1))].append((src, dst))

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
        if not same or any(dst != cur[0] for _s, dst in same):
            continue
        if re.search(r'\b%s\b' % re.escape(name), sig):
            continue
        body = '\n'.join(code)
        if re.search(r'&\s*%s\b(?!\s*(?:->|\.|\[))' % re.escape(name), body):
            continue
        if re.search(MEANS % re.escape(name), body):
            continue
        # An ordering comparison is where signedness changes the *answer* and
        # not just the bits, and the other side is usually a plain local with
        # no cast to protect it.  `alt_p2_model`'s `e_top` is the case that
        # found this: it is a residual, so `e_top < deadzone_lo` is asking
        # whether the error is below a negative bound, and declaring it
        # unsigned makes every negative error enormous.  Four streams moved.
        #
        # Equality is exempt -- it compares the same bits either way.
        if re.search(ORDER % (re.escape(name), re.escape(name)), body):
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


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
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

    if '--all' not in sys.argv:
        for nm, _a, _b, name, cur, want, n in found:
            print('%-24s %-16s %-9s -> %-9s %d conversions'
                  % (nm.lstrip('_'), name, cur, want, n))
        print('%d locals declared against their own assignments, %d conversions'
              % (len(found), sum(f[6] for f in found)))
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
        top, lo = got
        text = lines[lo]
        for pat in (r'\b%s\s*,\s*' % re.escape(name),
                    r'\s*,\s*\b%s\b' % re.escape(name),
                    r'\b%s\b' % re.escape(name)):
            new_text, k = re.subn(pat, '', text, count=1)
            if k:
                break
        indent = text[:len(text) - len(text.lstrip())]
        # Nothing left but the type: the whole declaration was this one name.
        # The comment has to come off before that test and go back on after --
        # `uint32_t n0x10_2;   // a record index` left `uint32_t ;   // ...`,
        # which compiles only under `-fpermissive` and so failed the gate's
        # strict pass rather than the streams.
        bare, _sep, note = new_text.partition('//')
        if re.fullmatch(r'\s*(?:const\s+|static\s+)*%s\s*;?\s*' % cur, bare):
            lines[lo] = '%s%s %s;%s' % (indent, want, name,
                                        ('   //' + note) if note else '')
        else:
            lines[lo] = new_text
            head = lines[top][:len(lines[top]) - len(lines[top].lstrip())]
            lines.insert(top, '%s%s %s;' % (head, want, name))
            grown[nm] += 1
        done += 1
    # Taking the last name off a wrapped line leaves the comma that separated
    # it and, when the line held only that name, an orphan `;` under it.
    text = '\n'.join(lines)
    text = re.sub(r',\s*,', ',', text)
    text = re.sub(r',[ \t]*\n[ \t]*;', ';', text)
    text = re.sub(r',[ \t]*;', ';', text)
    open(path, 'w').write(text)
    print('%d locals retyped' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
