#!/usr/bin/env python3
"""Two names for one value, where one is only ever a copy of the other.

    python3 tools/unspillpair.py bmf.cpp                 # report
    python3 tools/unspillpair.py bmf.cpp --list          # and every site
    python3 tools/unspillpair.py bmf.cpp --declined      # and what stopped each
    python3 tools/unspillpair.py model.inc --apply code_pixel cache=sym_cache

MSVC keeps a value in a register and spills it to a stack slot wherever control
leaves the region the register is live over.  Hex-Rays names the register and
the slot separately, so a reader gets two names for one thing.

**The rule, and it is deliberately narrow.**  `x` is a shadow of `y` when

  * both are locals of the body and neither is a parameter, a member, a global
    or a constant -- a global on the right is not a copy, it is a *load*, and
    merging the local into it would write the global where nothing wrote it;
  * every assignment to `x` in the body is exactly `x = y;`;
  * `x`'s address is never taken, nor `y`'s;
  * and **every read of `x` sits in the same straight-line run as the copy that
    feeds it, with no write to `y` in between.**

That last clause is the whole of the soundness argument and it is what the
first version of this file did not have.  Without it the tool proposed

    pi3 = plane;
    f2 = plane_desc[plane++].flags&desc_has_refs|try_p1;
    plane_desc[pi3].flags = f2;

as a merge, which is wrong: `plane` moves between the copy and the read, and
`pi3` is the *old* value.  That is not a spill pair, it is the decompiler
splitting a post-increment, and a tool that hands it to the gate and waits for
the streams to move is not measuring anything -- it is guessing with a
thirty-second confirmation.  So the run is checked here: a label, a `goto`, a
loop header, a call, or a closing brace between the copy and the read ends the
run and the pair is declined with that reason.

**Why this is not `uncopy.py`.**  That tool wants a local assigned *exactly
once* and reports the reads that could name its source instead.  A spill pair is
assigned at every exit of the region so it never matches, and `uncopy.py` reads
zero over hundreds of two-names-for-one-value sites.  Its zero is honest; this
is the neighbouring rule.

**What the declines are worth.**  More than the accepts, on this tree.  A pair
declined for "a write to the source in between" is the decompiler splitting an
expression; one declined for "the source is a global" is a load that should
probably *stay* a load; one declined for a `goto` in between is the shape the
whole phase was named for, and it is the one this cannot do by reading text.
`--declined` prints them with the reason so the next reader gets the list
rather than the summary.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

COPY = re.compile(r'^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*;\s*$')
WRITE = re.compile(r'(?<![\w.>])([A-Za-z_]\w*)\s*(?:=(?!=)|\+=|-=|\*=|/=|\|='
                   r'|&=|\^=|<<=|>>=)')
BUMP = re.compile(r'(?:\+\+|--)\s*([A-Za-z_]\w*)|([A-Za-z_]\w*)\s*(?:\+\+|--)')
# What ends a straight-line run: a jump, a label, a loop or branch header, a
# call (which could touch anything reachable), or a brace.
BREAK = re.compile(r'\bgoto\b|^\s*\w+\s*:(?!:)|\b(?:while|for|do|if|else|'
                   r'switch|case|return)\b|[{}]|\w\s*\(')


# `structs.decl_types` is not used for the local set here, and the reason is
# worth writing down: its scan starts on the signature line, which
# `starts_declaration` accepts, and the first `;` it then finds is the end of
# the body's *first declaration* -- so the signature and that declaration are
# joined into one string, `declaration()` rejects it for the `(`, and the names
# on it are never typed.  On a body whose declarations start a line or two down
# it works; on one where they start immediately it silently loses the first.
# That is shared machinery twenty tools import and widening it is a change to
# all of them, so this collects what it needs and says why.
DECL_LINE = re.compile(r'^\s*(?:const\s+|static\s+|unsigned\s+|signed\s+)*'
                       r'((?:struct\s+)?[A-Za-z_]\w*)\s*((?:[\s\*]*[A-Za-z_]\w*'
                       r'(?:\[[^\]]*\])*\s*(?:=[^;,]*)?,)*'
                       r'[\s\*]*[A-Za-z_]\w*(?:\[[^\]]*\])*\s*(?:=[^;]*)?)\s*;\s*$')
NOT_A_TYPE = {'return', 'goto', 'break', 'continue', 'else', 'do', 'case',
              'default', 'delete', 'typedef', 'sizeof'}
# The opening of a declaration, for the wrap-joining in `joined` below: a type
# and then a declarator, with no `(` or `{` to make it a call or a body.
DECL_HEAD = re.compile(r'^\s*(?:const\s+|static\s+|unsigned\s+|signed\s+)*'
                       r'(?:struct\s+)?[A-Za-z_]\w*[\s\*]+[A-Za-z_]\w*')


def is_decl(line):
    """Whether this line is a declaration statement.

    `return slot;` matches the declarator pattern as neatly as
    `int32_t slot;` does -- a name, then a name, then a semicolon -- and the
    first version of this file used the pattern without the keyword check at
    one of its two call sites.  The effect was that a `return` of a shadow was
    skipped as a declaration rather than counted as the read it is, and
    `probe_jump` in the probe file was accepted with an `if` between the copy
    and the read.  One guard, both places.
    """
    m = DECL_LINE.match(line)
    return m if m and m.group(1) not in NOT_A_TYPE else None


def joined(lines, a, b):
    """The body's lines, with a declaration that wraps read as the one line it is.

    Hex-Rays writes one `int32_t` list across as many rows as it needs, and
    reading only the first row leaves every name on rows two and after with no
    declaration at all.  `unreload.types` records fixing exactly this and names
    three tools that had it; this was a fourth, and it did not fail silently --
    it declined those names as "not a local of this body", which reads like a
    judgement and was a parse.  `code_banks` declares twenty-eight `int32_t`
    across three rows and every pair among them was declined that way.
    """
    out, i = [], a + 1
    while i <= b:
        c = lines[i].split('//')[0]
        if (DECL_HEAD.match(c) and not c.rstrip().endswith(';')
                and '(' not in c and '{' not in c):
            j = i
            while j < b and not lines[j].split('//')[0].rstrip().endswith(';'):
                j += 1
            c = ' '.join(lines[k].split('//')[0].strip() for k in range(i, j + 1))
            i = j
        out.append(c)
        i += 1
    return out


def locals_in(lines, a, b):
    """{name: type} for the body's own declarations, signature line excluded."""
    out = {}
    for c in joined(lines, a, b):
        m = is_decl(c)
        if not m:
            continue
        base = m.group(1)
        for part in m.group(2).split(','):
            nm = re.match(r'[\s\*]*([A-Za-z_]\w*)', part)
            if nm:
                out.setdefault(nm.group(1),
                               base + '*' * (part.split('=')[0].count('*')
                                             + part.count('[')))
    return out


def params_of(sig):
    inner = sig[sig.find('(') + 1:sig.rfind(')')] if '(' in sig else ''
    return set(re.findall(r'(\w+)\s*(?:,|$)', inner))


def reads(line, name):
    """Positions at which `name` is read on this line (not written)."""
    out = []
    for m in re.finditer(r'(?<![\w.>])%s\b' % re.escape(name), line):
        after = line[m.end():]
        if re.match(r'\s*(?:=(?!=)|\+=|-=|\*=|/=|\|=|&=|\^=|<<=|>>=)', after):
            continue
        out.append(m.start())
    return out


def shadows(lines, a, b, sig, locals_of):
    body = [lines[i].split('//')[0] for i in range(a, b + 1)]
    text = '\n'.join(body)
    params = params_of(sig)
    writes = collections.Counter()
    copies = collections.defaultdict(list)
    for i, c in enumerate(body):
        for m in WRITE.finditer(c):
            writes[m.group(1)] += 1
        for m in BUMP.finditer(c):
            writes[m.group(1) or m.group(2)] += 1
        m = COPY.match(c)
        # `x = nullptr;` is not one name copied into another.  Four of these
        # were reaching the decline list and being turned away as "not a local
        # of this body", which is true of a keyword and says nothing.
        if m and m.group(2) not in ('nullptr', 'NULL', 'true', 'false'):
            copies[(m.group(1), m.group(2))].append(i)
    out, declined = [], []
    for (x, y), at in sorted(copies.items()):
        if x == y or writes[x] != len(at):
            continue
        why = None
        if x not in locals_of or y not in locals_of:
            why = 'one of the two is not a local of this body'
        elif x in params or y in params:
            why = 'one of the two is a parameter'
        elif re.search(r'&\s*(?:%s|%s)\b' % (re.escape(x), re.escape(y)), text):
            why = 'the address of one of the two is taken'
        elif re.sub(r'\s+', '', locals_of[x]) != re.sub(r'\s+', '', locals_of[y]):
            why = 'the declarations differ in type: %s and %s' % (
                locals_of[x], locals_of[y])
        if not why:
            # every read of x must be fed by a copy in the same straight run
            for i, c in enumerate(body):
                # A declaration mentions the name and is not a read of it, and
                # neither is the signature.  Counting them made every pair
                # decline with "a read precedes every copy", including the two
                # in the probe file written to prove this reports.
                if i == 0 or is_decl(c):
                    continue
                if i in at or not reads(c, x):
                    continue
                src = max((k for k in at if k < i), default=None)
                if src is None:
                    why = 'a read of %s precedes every copy into it' % x
                    break
                run = body[src + 1:i]
                if any(BREAK.search(l) for l in run):
                    why = ('the run from the copy to a read of %s is not '
                           'straight-line' % x)
                    break
                if any(WRITE.search(l) and y in [m.group(1) for m in
                                                 WRITE.finditer(l)]
                       for l in run) or \
                   any((m.group(1) or m.group(2)) == y
                       for l in run for m in BUMP.finditer(l)):
                    why = 'the source %s is written between the copy and a ' \
                          'read of %s' % (y, x)
                    break
        (declined if why else out).append(
            (x, y, why) if why else (x, y, [a + k for k in at]))
    return out, declined


def survey(path):
    lines, origin = structs.splice(path)
    found, declined = [], []
    for a, b, nm, sig, _d in structs.defs(lines):
        loc = locals_in(lines, a, b)
        f, d = shadows(lines, a, b, sig, loc)
        for x, y, at in f:
            found.append((nm, x, y, at, origin[at[0]][0]))
        for x, y, why in d:
            declined.append((nm, x, y, why))
    return found, declined


def apply_one(path, want_body, x, y):
    src = open(path).read().split('\n')
    lines, origin = structs.splice(path)
    for a, b, nm, sig, _d in structs.defs(lines):
        if nm.lstrip('_') != want_body.lstrip('_'):
            continue
        home = origin[a][0]
        if home != path.rsplit('/', 1)[-1]:
            sys.exit('%s is in %s; run this against that file' % (nm, home))
        lo, hi = origin[a][1] - 1, origin[b][1] - 1
        out, dropped = [], 0
        for i in range(len(src)):
            if not lo <= i <= hi:
                out.append(src[i])
                continue
            new = re.sub(r'(?<![\w.>])%s\b' % re.escape(x), y, src[i])
            if new.split('//')[0].strip() == '%s = %s;' % (y, y):
                dropped += 1
                continue
            out.append(new)
        open(path, 'w').write('\n'.join(out))
        return '%s: %s is %s, %d copies dropped (the declaration of %s is ' \
               'yours to remove)' % (nm, x, y, dropped, x)
    sys.exit('%s: no such body in %s' % (want_body, path))


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: python3 tools/unspillpair.py bmf.cpp [--list] '
                 '[--declined]\n'
                 '       python3 tools/unspillpair.py one.inc --apply BODY x=y')
    path = sys.argv[1]
    if '--apply' in sys.argv:
        i = sys.argv.index('--apply')
        x, y = sys.argv[i + 2].split('=')
        print(apply_one(path, sys.argv[i + 1], x, y))
        return
    found, declined = survey(path)
    if '--list' in sys.argv:
        for nm, x, y, at, home in sorted(found):
            print('  %-26s %-16s is %-16s %d copies  %s'
                  % (nm, x, y, len(at), home))
    if '--declined' in sys.argv:
        for nm, x, y, why in sorted(declined):
            print('  %-26s %-16s / %-14s %s' % (nm, x, y, why))
    kinds = collections.Counter(w.split(':')[0].split(',')[0]
                                for _n, _x, _y, w in declined)
    print('%d names that are only a copy of another and can be merged by '
          'reading, in %d bodies; %d declined (%s)'
          % (len(found), len({f[0] for f in found}), len(declined),
             ', '.join('%d %s' % (v, k) for k, v in kinds.most_common(4))))


if __name__ == '__main__':
    main()
