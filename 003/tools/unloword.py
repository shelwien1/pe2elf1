#!/usr/bin/env python3
"""Give a local the width its writes and reads agree on, not the register's.

    python3 tools/unloword.py subs1.hpp
    python3 tools/unloword.py subs1.hpp --all
    python3 tools/unloword.py subs1.hpp --declined   # and what stopped each one

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


def record_members(lines):
    """{record: {member: type}} for every named struct in the file.

    `members()` above answers "how wide is a member called X anywhere", and
    drops a name declared 16 bits in one record and 32 in another.  That is the
    right answer when the record is unknown, and it costs the commonest case:
    `height` is `uint16_t` in `BmfImage` and something else elsewhere, so the
    global map has nothing to say about `img->height` even though the local's
    declared type says exactly which record `img` points at.
    """
    out, cur, depth = collections.defaultdict(dict), None, 0
    for l in lines:
        c = l.split('//')[0]
        m = re.match(r'\s*struct\s+([A-Za-z_]\w*)\s*\{', c)
        if m and depth == 0:
            cur, depth = m.group(1), c.count('{') - c.count('}')
            continue
        if not cur:
            continue
        depth += c.count('{') - c.count('}')
        if depth <= 0:
            cur = None
            continue
        d = re.match(r'\s*((?:u?int(?:8|16|32|64)_t|float|double))\s+(\w+)\s*'
                     r'(?:\[[^\]]*\])?\s*;', c)
        if d:
            out[cur][d.group(2)] = d.group(1)
    return out


def fits(rhs, w, types, mem, rec=None, name=None):
    """Does this right-hand side provably store no more than `w` bits?

    Returns the signedness of the evidence, or None for "cannot tell".  Three
    forms count, and nothing else does:

      * an explicit narrowing cast -- `(uint8_t)*(p - 1)`.  The cast is the
        author saying what the value is, and it is the commonest by far;
      * a decimal or hex literal that fits;
      * a designator whose own declared width is already `w` or less --
        `this->step` on a `uint16_t` member, `*(freq + 3)` through a
        `uint16_t *`, `p_i->height`.

    The point of the whole exercise: a bare read of the local -- `if (pred <
    north)` -- is only safe to narrow when the top half was never anything but
    zero, and that is a property of the *writes*, not of the read.  Testing it
    at the reads is what made this rule decline 48 of its 49 candidates.
    """
    e = rhs.strip().rstrip(';').strip()
    m = re.match(r'\(\s*(u?)int(8|16|32|64)_t\s*\)', e)
    if m and int(m.group(2)) <= w:
        return m.group(1) == ''            # True == signed
    if re.match(r'^(?:0[xX][0-9a-fA-F]+|\d+)$', e):
        return False if int(e, 0) < (1 << w) else None
    # A designator, possibly dereferenced: `x`, `a->b`, `*(p + 3)`, `p[3]`.
    d = re.match(r'^\*?\(?\s*\*?\s*([A-Za-z_]\w*)\s*(?:->|\.)\s*([A-Za-z_]\w*)'
                 r'\s*(?:\[[^\]]*\])?\s*\)?$', e)
    if d:
        t = mem.get(d.group(2))
    else:
        d = re.match(r'^\*\s*\(?\s*([A-Za-z_]\w*)\s*(?:[+\-][^)]*)?\)?$', e) or \
            re.match(r'^([A-Za-z_]\w*)\s*\[[^\]]*\]$', e)
        t = types.get(d.group(1), '').rstrip('*') if d else None
    if t and structs.width(t) and structs.width(t) * 8 <= w:
        return t.startswith('int')
    # The same designator, resolved through the record the local points at
    # rather than through the global member map: `img->height` when `img` is a
    # `BmfImage *`.
    #
    # Only the last two components of a chain matter, which is what makes this
    # cheap: `__frame.p_i_2->height` needs `p_i_2`'s type and nothing before it,
    # and `structs.decl_types` already reports a frame's members as if they were
    # locals -- so `p_i_2` is a `BmfImage *` there without this having to know
    # what a frame is.
    o = re.match(r'^(?:[A-Za-z_]\w*\s*(?:->|\.)\s*)*?([A-Za-z_]\w*)\s*(?:->|\.)'
                 r'\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$', e)
    if rec and o:
        f = rec.get((types.get(o.group(1)) or '').rstrip('*'), {}).get(o.group(2))
        if f and structs.width(f) * 8 <= w:
            return f.startswith('int')
    # And the expressions whose *result* cannot be wider than `w`, whatever
    # their operands were.  Three shapes, each one an argument about the value
    # and not about a declaration:
    #
    #   e & K   with K < 2^w        -- the mask is the proof
    #   e >> k  with k >= 32 - w    -- a 32-bit value shifted that far fits
    #   (uint32_t)x >> k, k >= 1    -- `x` being this local, which the rest of
    #                                  this analysis is busy assuming is narrow;
    #                                  shifting it right cannot widen it, so the
    #                                  assumption is inductive rather than circular
    m = re.search(r'&\s*(0[xX][0-9a-fA-F]+|\d+)\s*$', e)
    if m and int(m.group(1), 0) < (1 << w):
        return False
    m = re.search(r'>>\s*(\d+)\s*\)?\s*(?:&[^&]*)?$', e)
    if m and int(m.group(1)) >= 32 - w:
        # A shift far enough right fits whatever follows it, and a mask after
        # it can only take bits away.  `((uint32_t)(16 - st) >> 30) & 0xFFFFFFFE`
        # is at most 3 and the mask does not widen it -- reading only the
        # trailing `& K` and rejecting it for K >= 2^w was what stopped this.
        return False
    if name and re.match(r'^\(\s*u?int32_t\s*\)\s*%s\s*>>\s*[1-9]\d*$'
                         % re.escape(name), e):
        return False
    return None


def narrow_by_writes(body, name, w, types, mem, rec):
    """(True, signed) when every full write to `name` provably fits in `w` bits.

    At least one full write is required.  A local written only through
    `LOWORD(x) =` has an indeterminate top half, and while narrowing it can
    only be an improvement, this rule does not get to decide that from a shape.
    """
    votes, seen = [], 0
    for l in body:
        for m in re.finditer(NAMED % re.escape(name), l):
            after = l[m.end():]
            if re.search(r'\b(?:LOWORD|LOBYTE)\s*\(\s*$', l[:m.start()]):
                continue
            if not re.match(r'\s*=[^=]', after):
                continue
            seen += 1
            v = fits(after.split('=', 1)[1], w, types, mem, rec, name)
            if v is None:
                return False, False
            votes.append(v)
    return (seen > 0), (bool(votes) and all(votes))


def declined(lines):
    """(body, local, why) for every partial-write target this rule will not take.

    The plan for this round asks the rule to decline *out loud*: a candidate
    that silently vanishes is indistinguishable from one the rule never saw,
    which is how `methodise.py` sat at zero for three rounds.  Anything listed
    here is a local that still carries a `LOWORD` or `LOBYTE` write and the one
    fact that stops it.
    """
    out, mem, rec = [], members(lines), record_members(lines)
    for a, b, nm, sig in bodies_with_parts(lines):
        body = [l.split('//')[0] for l in lines[a:b + 1]]
        types = structs.decl_types(sig, lines, a, b)
        for name, widths in parts_of(body).items():
            short = nm.lstrip('_')
            if len(widths) != 1:
                out.append((short, name, 'written at two widths'))
                continue
            w = next(iter(widths))
            if name not in types:
                out.append((short, name, 'no declaration this can read'))
                continue
            if structs.width(types[name]) != 4:
                out.append((short, name, 'declared %s, not a 32-bit register'
                            % types[name]))
                continue
            why, seen = None, 0
            for l in body:
                for m in re.finditer(NAMED % re.escape(name), l):
                    if re.search(r'\b(?:LOWORD|LOBYTE)\s*\(\s*$', l[:m.start()]):
                        continue
                    after = l[m.end():]
                    if not re.match(r'\s*=[^=]', after):
                        continue
                    seen += 1
                    rhs = after.split('=', 1)[1].strip().rstrip(';').strip()
                    if fits(rhs, w, types, mem, rec, name) is None:
                        why = 'full write not provably %d-bit: %s' % (w, rhs[:44])
            if why:
                out.append((short, name, why))
            elif seen == 0:
                out.append((short, name, 'no full write, so the top half is '
                                         'indeterminate'))
    taken = {(f.lstrip('_'), n) for f, _, _, n, _, _, _, _ in survey(lines)}
    return [r for r in out if (r[0], r[1]) not in taken]


def bodies_with_parts(lines):
    for a, b, nm, sig in structs.bodies(lines):
        if parts_of([l.split('//')[0] for l in lines[a:b + 1]]):
            yield a, b, nm, sig


def parts_of(body):
    part = collections.defaultdict(collections.Counter)
    for l in body:
        for m in re.finditer(r'\b(LOWORD|LOBYTE)\s*\(\s*(\w+)\s*\)\s*=[^=]', l):
            part[m.group(2)][W[m.group(1)]] += 1
    return part


def survey(lines):
    out = []
    mem = members(lines)
    rec = record_members(lines)
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
            # Whether a bare read of the whole local is safe, decided once from
            # the writes rather than at each read.  `by_writes` is the evidence
            # that the top half is always zero; `wsigned` is what the narrowing
            # casts agreed on, and it wins over `lands` below because a cast in
            # the source is a stronger statement than the type of a slot the
            # value later happens to land in.
            by_writes, wsigned = narrow_by_writes(body, name, w, types, mem, rec)
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
                    # A read this rule cannot otherwise account for.  It is
                    # harmless exactly when the local never held more than `w`
                    # bits, which `narrow_by_writes` has already settled: with
                    # the top half always zero, a 32-bit read and a 16-bit one
                    # give the same value, and `if (pred < north)` compares the
                    # same two numbers either way.
                    if by_writes:
                        continue
                    ok = False
                    break
                if not ok:
                    break
            if not ok:
                continue
            want = [t for t in lands if t and structs.width(t) * 8 == w]
            signed = bool(want) and all(t.startswith('int') for t in want)
            if by_writes and not want:
                signed = wsigned
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
        # The last name of a *wrapped* run: the comma that introduces it is on
        # the line before, so neither pattern above reaches it.  Crossing the
        # newline is safe here and only here, because the lookahead pins the
        # name to the `;` that closes the run -- there is nothing after it on
        # its line to splice onto the one before.  `k4` and `g4` are the two,
        # and without this they were the rule's only failures to apply.
        if cut == run:
            cut = re.sub(r',[ \t]*\n[ \t]*\*?%s(?=[ \t]*;)'
                         % (NAMED % re.escape(name)), '', run, count=1)
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
    elif '--declined' in sys.argv:
        no = declined(lines)
        for fn, name, why in no:
            print('%-26s %-10s %s' % (fn, name, why))
        print('%d taken, %d declined and why' % (len(found), len(no)))
    else:
        print('%d locals held in a register wider than anything reads' % len(found))
