#!/usr/bin/env python3
"""Give a local the width its writes and reads agree on, not the register's.

    python3 tools/unloword.py subs1.hpp
    python3 tools/unloword.py subs1.hpp --all

    uint32_t row16;
    if ( !byte_rows && bits == 4 )
      row16 = ((img_w + 7) >> 1) & 0xFFFFFFFC;
    else
      LOWORD(row16) = (uint16_t)(((bits + 7) >> 3) * img_w);
    row_bytes = (uint16_t)row16;

Both arms assign one sixteen-bit quantity.  MSVC wrote one of them through the
whole register and the other through its low half, because it knew the top half
was dead, and Hex-Rays wrote the half-write as a macro and every read back as a
mask.  So the local is a `uint16_t`: the declaration says so, `LOWORD(x) = e`
becomes `x = e`, and the masks come off.

The condition is that nothing reads the other half.  A local whose value is
*used* at full width keeps its register -- `LOWORD(w_new) = w_top + (w_new >> 2)`
reads all thirty-two bits to compute the sixteen it stores, and narrowing it
would change the shift.  That is the whole hazard here, and it is why a read
this rule cannot account for disqualifies the local rather than being ignored.

Signedness comes from where the value lands.  `plane[2]->cursor[0]->dval = seed2`
puts it in an `int16_t`, so `seed2` is one too and the store stops converting;
picking unsigned by default would trade a narrowing warning for a signedness
one and call it progress.
"""
import re, sys, collections
sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs

W = {'LOWORD': 16, 'LOBYTE': 8}
NARROW = {16: ('uint16_t', 'int16_t'), 8: ('uint8_t', 'int8_t')}
# `blk->band_lo` is not a use of a local called `band_lo`, and `.x` is not `x`.
NAMED = r'(?<![\w.>])(?<!->)%s\b'


def members(lines):
    """Every struct member name whose width is the same wherever it appears.

    A name declared 16 bits in one record and 32 in another says nothing about
    the store it is the destination of, so it is dropped rather than guessed.
    """
    seen = collections.defaultdict(set)
    depth, inside = 0, False
    for l in lines:
        c = l.split('//')[0]
        if re.match(r'\s*(?:struct|union)\b.*\{', c):
            inside, depth = True, depth + c.count('{') - c.count('}')
            continue
        if inside:
            depth += c.count('{') - c.count('}')
            if depth <= 0:
                inside = False
            m = re.match(r'\s*((?:u?int(?:8|16|32|64)_t|float|double))\s+'
                         r'(\w+)\s*(?:\[[^\]]*\])?\s*;', c)
            if m:
                seen[m.group(2)].add(m.group(1))
    return {n: t.pop() for n, t in seen.items() if len(t) == 1}


def destination(lhs, mem, types):
    """The type a store lands in, or None when it cannot be read off."""
    m = re.search(r'(?:\.|->)(\w+)\s*(?:\[[^\]]*\])?\s*$', lhs)
    if m:
        return mem.get(m.group(1))
    m = re.match(r'\s*(\w+)\s*$', lhs)
    if m:
        return types.get(m.group(1))
    return None


def survey(lines):
    out = []
    mem = members(lines)
    for a, b, nm, sig in structs.bodies(lines):
        body = [l.split('//')[0] for l in lines[a:b + 1]]
        types = structs.decl_types(sig, lines, a, b)
        # A declaration is not a read of the name it declares, and half of
        # these declarations wrap: `int32_t val1, l7a,` on one line and the
        # rest of the list on the next.  Testing the *first* line of the run
        # is what made this rule silently skip every local declared in a
        # continuation, which was five of the eight it should have found.
        decl = set()
        i = a
        while i <= b:
            if structs.starts_declaration(lines[i]):
                j = i
                while ';' not in lines[j] and j < b:
                    j += 1
                decl.update(range(i, j + 1))
                i = j
            i += 1
        part = collections.defaultdict(collections.Counter)
        for l in body:
            for m in re.finditer(r'\b(LOWORD|LOBYTE)\s*\(\s*(\w+)\s*\)\s*=[^=]', l):
                part[m.group(2)][W[m.group(1)]] += 1
        for name, widths in part.items():
            if len(widths) != 1 or name not in types:
                continue
            w = next(iter(widths))
            if structs.width(types[name]) != 4:
                continue          # already narrow, or a pointer this cannot judge
            lands, ok = [], True
            for i, l in enumerate(body):
                for m in re.finditer(NAMED % re.escape(name), l):
                    before, after = l[:m.start()], l[m.end():]
                    if re.search(r'\b(?:LOWORD|LOBYTE)\s*\(\s*$', before):
                        continue                       # the partial write
                    if re.match(r'\s*=[^=]', after):
                        # A full write is only harmless if the value it stores
                        # still fits once the local narrows.  A pointer never
                        # does: `wp = (uint16_t *)__frame.sym9` is a register
                        # holding an address in one lifetime and a symbol in
                        # the next, and narrowing it around the address turned
                        # a green build into an `-fpermissive` conversion --
                        # caught by the strict pass, but the rule should not
                        # have proposed it.  Splitting the lifetimes is
                        # somebody else's job; this one declines.
                        if re.match(r'\s*=\s*\([^)]*\*\s*\)', after) or \
                           re.search(r'&\w|\bnullptr\b', after.split(';')[0]):
                            ok = False
                            break
                        continue                       # a full write
                    if re.search(r'\(\s*u?int%d_t\s*\)\s*(?:\(\s*uintptr_t\s*\)\s*)?$'
                                 % w, before):
                        lands.append(destination(before.split('=')[0], mem, types)
                                     if '=' in before else None)
                        continue                       # a masked read
                    # The *whole* right-hand side, not merely the last thing on
                    # it: `LOWORD(st) = st - down` reads `down` at full width
                    # to compute sixteen bits, and a first version that tested
                    # only for a trailing `;` took it for a bare store.
                    if (re.match(r'\s*;\s*$', after)
                            and re.search(r'(?<![=!<>+\-*/&|^%])=\s*$', before)):
                        lands.append(destination(before[:before.rindex('=')],
                                                 mem, types))
                        continue                       # the whole right-hand side
                    if a + i in decl:
                        continue
                    ok = False
                    break
                if not ok:
                    break
            if not ok:
                continue
            want = [t for t in lands if t and structs.width(t) * 8 == w]
            signed = bool(want) and all(t.startswith('int') for t in want)
            out.append((nm.lstrip('_'), a, b, name, types[name],
                        NARROW[w][signed], w, widths[w]))
    return out


def apply(lines, fn, a, b, name, want, w):
    """Retype the declaration, unwrap the partial writes, drop the masks."""
    runs, i = [], a
    while i <= b:
        if structs.starts_declaration(lines[i]):
            j = i
            while ';' not in lines[j] and j < b:
                j += 1
            runs.append((i, j))
            i = j
        i += 1
    for i, j in runs:
        run = '\n'.join(l.split('//')[0] for l in lines[i:j + 1])
        if not re.search(NAMED % re.escape(name), run):
            continue
        indent = re.match(r'\s*', lines[i]).group(0)
        # The whole run declares this one name: rewrite its type in place and
        # drop the `*` if it was a pointer used as a register.
        if re.match(r'\s*(?:u?int(?:8|16|32|64)_t)\s*\*?\s*%s\s*;\s*$' % name, run):
            lines[i:j + 1] = ['%s%s %s;' % (indent, want, name)]
            b += 1 - (j + 1 - i)
            break
        # One name out of a list: give it a line of its own rather than
        # splitting the list, which is how the first version of `resign.py`
        # left `int32_t a, b, ;` behind.
        # Take the comma *after* the name, never the space before it: these
        # runs wrap, and a name that begins a continuation line takes that
        # line's indent with it if the space in front is what gets eaten.
        # For the same reason the whitespace class is `[ \t]` and not `\s`,
        # which would cross the newline and splice two lines into one.
        cut = re.sub(r'\*?%s[ \t]*,[ \t]*' % (NAMED % re.escape(name)), '', run, count=1)
        if cut == run:
            cut = re.sub(r',[ \t]*\*?%s(?=[ \t]*;)' % (NAMED % re.escape(name)), '',
                         run, count=1)
        if cut == run:
            return False
        new = [l for l in cut.split('\n') if l.strip()]
        new += ['%s%s %s;' % (indent, want, name)]
        lines[i:j + 1] = new
        b += len(new) - (j + 1 - i)
        break
    else:
        return False
    for i in range(a, len(lines)):
        c = lines[i]
        c = re.sub(r'\b(?:LOWORD|LOBYTE)\s*\(\s*%s\s*\)(\s*=[^=])' % name,
                   name + r'\1', c)
        c = re.sub(r'\(\s*u?int%d_t\s*\)\s*(?:\(\s*uintptr_t\s*\)\s*)?(%s\b)' % (w, name),
                   r'\1', c)
        lines[i] = c
        if i >= b:
            break
    return True


if __name__ == '__main__':
    p = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(p).read().split('\n')
    found = survey(lines)
    for fn, a, b, name, cur, want, w, n in found:
        print('%-26s %-10s %-9s -> %-9s %d partial write%s'
              % (fn, name, cur, want, n, '' if n == 1 else 's'))
    if '--all' in sys.argv:
        done = 0
        # Back to front: `apply` rewrites by line number and the declaration it
        # replaces can add a line.
        for fn, a, b, name, cur, want, w, n in sorted(found, key=lambda r: -r[1]):
            done += apply(lines, fn, a, b, name, want, w)
        open(p, 'w').write('\n'.join(lines))
        print('%d of %d retyped' % (done, len(found)))
    else:
        print('%d locals held in a register wider than anything reads' % len(found))
