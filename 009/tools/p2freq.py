#!/usr/bin/env python3
"""Fold `alt_p2_model`'s rescale-then-bump pairs into `p2_freq_add`.

    python3 tools/p2freq.py                    # what it would take
    python3 tools/p2freq.py --list             # and every site, with its verdict
    python3 tools/p2freq.py --apply

`REVIEW.md`, `alt_p2_model.inc`.  Fifty sites write out the same two steps --
keep the record's three counts from overflowing, then add a sixteenth-fraction
of its step to one of them:

    if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 )
      grp[1].rescale_three_way();
    mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;

which is `p2_freq_add(&grp[1], is_dec, 5);`.

**Why it takes an increment *numerator* and not the increment.**
`rescale_three_way` halves the counts *and steps the record's own `step` down*,
and the bump reads `step` afterwards.  Passing the computed increment would read
it before.  So the helper takes `num` and does the multiply itself.

**Three spellings of one record.**  The reason these read as fifty different
things is that the record is named up to four ways at once -- `frec2`,
`frec_step[1]` and `p2_rec[1]` are one record, and `mir_top` is a `uint16_t*`
alias onto its `f[]` with the index shifted by one, because `f[]` sits one
`uint16_t` past `step`.  So this resolves aliases before it compares, and
requires the guard, the rescale, the `+=` target and the step source to be *the
same record* after resolution.  A site where they are not is not folded; it is
reported.

**Three increment spellings, one meaning.**  `(N*step)>>4`, `(step&0xFFFC)>>2`
and `(step&0xFFF8)>>3`.  The masks clear bits the shift discards anyway, so the
second is `step>>2` and the third `step>>3`, which are `(4*step)>>4` and
`(2*step)>>4`.  Normalising all three onto sixteenths is what makes the set of
increments visible: 2, 3, 4, 5, 6, 7, 10 and 13 sixteenths, and nothing else.

**What it declines.**  A site whose bump is not the next statement -- the
increment stashed in a temporary and added under a later `if( a4 )` -- because
folding it would move the add across a branch.  Those stay, and the count of
them is the tool's output.
"""
import re
import sys

PATH = 'alt_p2_model.inc'

SUB = r'(?:\[(?:[^\[\]]|\[[^\[\]]*\])*\])'
REC = r'([A-Za-z_]\w*(?:' + SUB + r')?)(?:->|\.)'
GUARD = re.compile(r'^(\s*)if\( ' + REC + r'f\[2\]\+.*?>29696 \)\s*\{?\s*$')
# `(&freq[idx0+1])->rescale_three_way();` -- the decompiler parenthesises
# the address-of at nine sites and writes the record plainly at the rest.
RESC = re.compile(r'^\s*(\(&)?([A-Za-z_]\w*(?:' + SUB + r')?)\)?(->|\.)'
                  r'rescale_three_way\(\);\s*$')
# `X.f[slot] += …;`  or the `uint16_t*` alias `mir[slot+1] += …;`
# `x += y` and `x = x + y` are the same statement; the decompiler emits both.
ADD_F = re.compile(r'^\s*' + REC + r'f\[([^\]]+)\] \+= (.+);\s*$')
ADD_A = re.compile(r'^\s*(\w+)\[([^\]]+?)\+1\] \+= (.+);\s*$')
SELF_A = re.compile(r'^\s*(\w+)\[([^\]]+?)\+1\] = \1\[\2\+1\]\+\((.+)\);\s*$')
# one temporary between the rescale and the bump: `t = <incr>; rec.f[s] += t;`
TMP = re.compile(r'^\s*(\w+) = (.+);\s*$')
# the three spellings of one increment, as sixteenths
INCR = [(re.compile(r'^\(?(\d+)\*\(?(?:\(uint32_t\))?' + REC + r'step\)?\)>>4$'), None),
        (re.compile(r'^\(?(?:\(uint16_t\))?\(' + REC + r'step&0xFFFC\)>>2$'), 4),
        (re.compile(r'^\(?(?:\(uint16_t\))?\(+' + REC + r'step&0xFFF8\)+>>3$'), 2),
        # `mir_top[0]` is the record's `step`: the alias points at the record
        # and `step` is its first `uint16_t`, which is the same reason the
        # bump's index is `slot + 1`.
        (re.compile(r'^\(?(?:\(uint16_t\))?\((\w+)\[0\]&0xFFFC\)>>2$'), 4)]


def canon(name, alias, depth=0):
    """`frec2` and `p2_rec[1]` are one record; say so as one string.

    Recursive, because the aliases chain: `frec_step = &freq[step_v]`, then
    `p2_rec = frec_step`, then `frec2 = &frec_step[1]`.  Resolving one step
    left `frec2` as `frec_step[1]` and `p2_rec[1]` as `freq[step_v]|[1]`, which
    compare unequal while naming the same record.
    """
    m = re.match(r'^([A-Za-z_]\w*)(' + SUB + r')?$', name)
    if not m or depth > 4:
        return name
    base, idx = m.group(1), m.group(2) or '[0]'
    if base in alias and canon('%s%s' % alias[base], alias, depth + 1) != \
            '%s%s' % alias[base]:
        b2, i2 = alias[base]
        deep = canon('%s%s' % (b2, i2), alias, depth + 1)
        m2 = re.match(r'^([A-Za-z_]\w*)(' + SUB + r')(\|.*)?$', deep)
        if m2 and idx == '[0]':
            return deep
        if m2:
            return '%s%s|%s' % (m2.group(1), m2.group(2), idx)
    if base in alias:
        b2, i2 = alias[base]
        if idx == '[0]':
            return '%s%s' % (b2, i2)
        if i2 == '[0]' and re.fullmatch(r'\[-?\d+\]', idx):
            return '%s%s' % (b2, idx)
        if re.fullmatch(r'\[-?\d+\]', idx) and re.fullmatch(r'\[-?\d+\]', i2):
            return '%s[%d]' % (b2, int(idx[1:-1]) + int(i2[1:-1]))
        # `frec_step` and `p2_rec` both alias `freq[step_v]`, and the offsets
        # cannot be added because one of them is a variable.  A composite key
        # still says the two are the same record at the same offset, which is
        # all the comparison needs.
        return '%s%s|%s' % (b2, i2, idx)
    return base + idx


def track(line, alias):
    """Record `X = &Y[n];`, `X = Y;` and `X = (uint16_t*)&Y[n];`."""
    m = re.match(r'^\s*(?:\w+\s*\*+\s*)?(\w+) = \(*(?:uint16_t\*|P2Freq\*)?\)?'
                 r'\s*&?([A-Za-z_]\w*)(' + SUB + r')?\)?;\s*$', line)
    if not m:
        return
    dst, src, idx = m.group(1), m.group(2), m.group(3) or '[0]'
    if src in alias and idx == '[0]':
        alias[dst] = alias[src]
    elif src in alias and re.fullmatch(r'\[-?\d+\]', idx):
        b2, i2 = alias[src]
        if re.fullmatch(r'\[-?\d+\]', i2):
            alias[dst] = (b2, '[%d]' % (int(idx[1:-1]) + int(i2[1:-1])))
        else:
            alias[dst] = (src, idx)
    else:
        alias[dst] = (src, idx)


def ptr(rec, via):
    """`grp[1].rescale…` is a value and needs `&`; `frec2->rescale…` is already
    a pointer and must not get one."""
    return rec if via == '->' else '&' + rec


def sites(lines):
    out, alias = [], {}
    i = 0
    while i < len(lines):
        g = GUARD.match(lines[i])
        if not g:
            track(lines[i], alias)
            i += 1
            continue
        indent, guard_rec = g.groups()
        j = i + 1
        while j < len(lines) and lines[j].strip() == '{':
            j += 1
        r = RESC.match(lines[j]) if j < len(lines) else None
        if not r:
            out.append((i, i, 'no rescale', None))
            i += 1
            continue
        # `(&X)->rescale…` is a *value* wearing an address-of, so it needs the
        # `&` back; `X->rescale…` is already a pointer and must not get one.
        resc_rec = r.group(2)
        resc_via = '.' if r.group(1) else r.group(3)
        # the bump must be the next statement, braces aside
        k = j + 1
        while k < len(lines) and lines[k].strip() in ('}', '{'):
            k += 1
        add = ADD_F.match(lines[k]) if k < len(lines) else None
        alias_add = None if add else (ADD_A.match(lines[k]) if k < len(lines) else None)
        if not add and not alias_add and k < len(lines):
            alias_add = SELF_A.match(lines[k])
        # One temporary is allowed between the rescale and the bump, because
        # the decompiler put the increment in a register at four sites and the
        # add is still the next statement.  Two would not be: the second could
        # be anything.
        if not add and not alias_add and k + 1 < len(lines):
            t = TMP.match(lines[k])
            nxt = ADD_F.match(lines[k + 1])
            if t and nxt and nxt.group(3).strip() == t.group(1):
                add, k = ADD_F.match('%s%s.f[%s] += %s;'
                                     % (' ', nxt.group(1), nxt.group(2),
                                        t.group(2))), k + 1
            elif t and nxt and t.group(1) in nxt.group(3):
                sub = nxt.group(3).replace(t.group(1), '(%s)' % t.group(2))
                add, k = ADD_F.match(' %s.f[%s] += %s;'
                                     % (nxt.group(1), nxt.group(2), sub)), k + 1
        if not add and not alias_add:
            out.append((i, k, 'bump is not the next statement', None))
            i = j + 1
            continue
        if add:
            tgt, slot, incr = add.groups()
        else:
            nm, slot, incr = alias_add.groups()
            if nm not in alias:
                out.append((i, k, 'unresolved alias %s' % nm, None))
                i = j + 1
                continue
            tgt = '%s%s' % alias[nm]
        num, step_rec = None, None
        for pat, fixed in INCR:
            m = pat.match(incr.strip())
            if m:
                if fixed is None:
                    num, step_rec = int(m.group(1)), m.group(2)
                else:
                    num, step_rec = fixed, m.group(1)
                break
        if num is None:
            out.append((i, k, 'increment shape: %s' % incr[:40], None))
            i = j + 1
            continue
        recs = {canon(x, alias) for x in (guard_rec, resc_rec, tgt, step_rec)}
        if len(recs) != 1:
            out.append((i, k, 'four records, %d distinct: %s' % (len(recs), sorted(recs)), None))
            i = j + 1
            continue
        # The canonical name is for *comparison*: `freq[step_v]|[1]` says two
        # spellings are one record and is not an expression.  What gets emitted
        # is the rescale target, which is always written as real source.
        out.append((i, k, 'fold', '%sp2_freq_add(%s, %s, %d);'
                    % (indent, ptr(resc_rec, resc_via), slot.strip(), num)))
        i = k + 1
    return out


def main():
    lines = open(PATH).read().split('\n')
    found = sites(lines)
    take = [f for f in found if f[2] == 'fold']
    for a, b, why, call in found:
        if '--list' in sys.argv or why != 'fold':
            print('  %s:%d-%d  %s%s' % (PATH, a + 1, b + 1, why,
                                        '  ->  ' + call.strip() if call else ''))
    print('%d of %d rescale-then-bump pairs fold to p2_freq_add'
          % (len(take), len(found)))
    if '--apply' in sys.argv and take:
        out, skip = [], {a: (b, call) for a, b, why, call in take}
        i = 0
        while i < len(lines):
            if i in skip:
                b, call = skip[i]
                out.append(call)
                i = b + 1
            else:
                out.append(lines[i])
                i += 1
        open(PATH, 'w').write('\n'.join(out))
        print('-- applied to %s (%d lines -> %d)' % (PATH, len(lines), len(out)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
