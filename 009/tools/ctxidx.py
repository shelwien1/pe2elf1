#!/usr/bin/env python3
"""Turn a masked context word into a `CtxIdx` chain, one line at a time.

    python3 tools/ctxidx.py alt_p2_context.inc --list
    python3 tools/ctxidx.py alt_p2_context.inc --line 242
    python3 tools/ctxidx.py alt_p2_context.inc --line 242 --apply

`alt_p2_context.inc` builds nine context words as a chain of `expr & <one bit>`
terms joined by `|`, plus a threshold count shifted into place.  Every one of
those masks keeps a single bit of a difference of neighbours, and for these
values that bit is the *sign* -- `P2Ctx::val`, `dval` and `err` are `int16_t`,
so they promote with bits 16..31 all copies of the sign, and a sum of at most
four of them stays under `2^17`.  So `(a - b) & 0x2000000` is "is a - b
negative", written at bit 25, and `CtxIdx` can say that.

CONTEXT-INDEX.md is the review this implements; §4 and §5 are this rule and
what it may not do.

**What it converts.**  A term becomes `.bit<k>(expr < 0)` only when the sign
argument holds without knowing anything about the model:

  * every leaf is an `int16_t` member (`.val`, `.dval`, `.err`, `.aerr`) or one
    of the locals that holds one -- `LEAF_ALIAS` below, each with the line it
    is assigned on -- and
  * the mask is at bit `15 + ceil(log2(weight))` or above, where the weight is
    the coefficients summed: `|sum| <= weight * 32767 < weight * 2^15`, so
    every bit from there up is a copy of the sign.  One member is bounded from
    bit 15, two from bit 16, three or four from bit 17.  `2*x + y + z` weighs
    four, not three.

**What it declines, and leaves masked inside a `raw`.**

  * **a mask below what the weight allows.**  At bit 16 a four-leaf *positive*
    sum reaches 131068 and sets the bit without being negative, so a four-leaf
    term masked there is not a sign test that this can prove.
  * **anything carrying a `run`.**  `run0 = pred0 + run_s`, `run1 = pred1 +
    run0`, and so on: an accumulated prediction, bounded by the model rather
    than by a type.  Eleven locals are these (`d_run0`, `d_up`, `d_up5`,
    `dtop2`, `dvsum2`, `lap` and the `run_*` pair), and they look like the easy
    case precisely because they are single names.
  * **`(uint16_t)` casts.**  Line 321 masks bit 15 of a difference of three
    values cast to `uint16_t` first, which *is* that difference's sign whatever
    the magnitudes -- but the reasoning is the cast's, not this rule's, and a
    rule that quietly widens itself to cover a case it was not written for is
    how the colour-transform defect got in.

A declined term keeps its mask and reads `.raw((expr)&0x8000)`, which is the
same code and says out loud that this one is not known to be a sign.  That is
the point: after this runs, which terms are proven and which are believed is
visible in the source instead of being a thing someone worked out once.
"""
import re
import sys

# Locals that hold one `int16_t` member value, with where each is assigned.  A
# leaf is bounded if it is a member access or one of these; anything else --
# every `run`, every difference against one -- is not.
LEAF_ALIAS = {
    'dv_now': 'cx0[0].dval',       'dv_now4': 'cx0[0].dval',
    'dv1': 'cx1[1].dval',          'lane2': 'ra0->err',
    'lane3': 'cx1->aerr',          'nb4_4': 'cx4[4].val',
    'up5': 'cx1[5].val',           'cx2_val0': 'cx2->val',
    'cx1_val': 'cx1->val',
}
MEMBER = re.compile(r'\w+(\[-?\d+\])?(->|\.)(val|dval|err|aerr)')


def split_terms(rhs):
    """[(operand, mask, start, end)] for every `expr & <mask>` in `rhs`.

    Anchored on the masks and walking *back* over the operand, rather than
    splitting on `|`: these lines carry parentheses that group several `|`
    terms together for no reason -- line 269 wraps eight of its eleven -- and a
    depth-aware split reads such a group as one term.  Walking back from a mask
    to the nearest `|` or unbalanced `(` gets the operand whatever the grouping.
    """
    out = []
    for m in re.finditer(r'&\s*0x([0-9A-F]+)', rhs):
        depth, j = 0, m.start() - 1
        while j >= 0:
            c = rhs[j]
            if c == ')':
                depth += 1
            elif c == '(':
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and c == '|':
                break
            j -= 1
        out.append((rhs[j + 1:m.start()].strip(), int(m.group(1), 16),
                    j + 1, m.end()))
    return out


def residue(rhs, terms):
    """What is left of `rhs` once every masked term is taken out of it."""
    keep, at = [], 0
    for _, _, s, e in terms:
        keep.append(rhs[at:s])
        at = e
    keep.append(rhs[at:])
    left = ''.join(keep)
    left = re.sub(r'\(\s*\)', '', left)
    return [p for p in (x.strip() for x in re.split(r'\|', left)) if p.strip('()| ')]


def leaves(expr):
    """(bounded, weight): is every leaf an int16 value, and how many are there.

    Weight counts a coefficient: `2*x` is two leaves, because the bound this
    serves is `sum of |coefficients| * 32767 < 2^17`.
    """
    e = MEMBER.sub('@', expr)
    for k in LEAF_ALIAS:
        e = re.sub(r'(?<![\w.>])%s\b' % k, '@', e)
    if re.search(r'(?<![\w.>])[A-Za-z_]\w*', e):
        return False, 0          # a name that is not a bounded leaf
    weight = 0
    for m in re.finditer(r'(\d+)?\s*\*?\s*@', e):
        weight += int(m.group(1)) if m.group(1) else 1
    return True, weight


def convert(term, mask):
    """`.bit<k>(expr < 0)` where the sign is provable, `.raw(...)` otherwise."""
    bit = mask.bit_length() - 1
    inner = term.strip()
    while inner.startswith('(') and inner.endswith(')'):
        d, ok = 0, True
        for i, c in enumerate(inner):
            d += (c == '(') - (c == ')')
            if d == 0 and i < len(inner) - 1:
                ok = False
                break
        if not ok:
            break
        inner = inner[1:-1].strip()
    bounded, weight = leaves(inner)
    floor = 15
    while weight > (1 << (floor - 15)):
        floor += 1
    if bounded and 0 < weight and bit >= floor:
        return '.bit<%d>(%s < 0)' % (bit, inner), True
    return '.raw((%s)&0x%X)' % (inner, mask), False


CMP = re.compile(r'\(\(uint32_t\)\(([^()]+?)-([\w.+ ()\[\]>-]+?)\)>>31\)')


def row(piece):
    """`((a)+(b)+(c))<<k` -> `.bits<k, 2>(a+b+c)`, or None.

    A count of three comparisons is 0..3, which is two bits, and it is already
    shifted into place -- so this is the one part of these lines that was a
    field all along.  The comparisons are rewritten as comparisons where they
    are written as a sign: `(uint32_t)(K - x) >> 31` is `x > K`.
    """
    m = re.fullmatch(r'\((.+)\)<<(\d+)\)?', piece.strip().lstrip('(').rstrip(')') and piece.strip())
    m = re.fullmatch(r'\(\((.+)\)<<(\d+)\)', piece.strip())
    if not m:
        return None
    inner, at = m.group(1), int(m.group(2))
    inner = CMP.sub(lambda c: '(%s>%s)' % (c.group(2).strip(), c.group(1).strip()), inner)
    return '.bits<%d, 2>(%s)' % (at, inner)


def logical(src):
    """[(first, last, text)] for every statement, continuations folded in.

    Phase 4 of `CLEANER.md` put one masked term per line, `|` leading, with a
    `// bit N` label on each.  That is the same statement -- but the rule below
    reads a *line*, so after the wrap this file reported that
    `alt_p2_context.inc` has no masked context words, which `sweep.sh` counted
    as "asked about something this repository does not have".  It has six of
    them.  A zero that a formatting pass can produce is the thing this project
    keeps finding, so the reader folds the continuations back first.
    """
    out, i = [], 0
    while i < len(src):
        if '=' not in src[i]:
            i += 1
            continue
        parts, j = [re.sub(r'\s*//.*$', '', src[i]).rstrip()], i
        while j + 1 < len(src):
            joined = ''.join(parts)
            # A leading `|` is the usual continuation; unbalanced parentheses
            # are the rest.  The sixth word's tail line begins `)|(((` because
            # its eleven terms sit inside one extra paren, and a rule that only
            # looked for `|` folded ten lines and dropped the eleventh.
            if not (re.match(r'^\s*\|', src[j + 1])
                    or joined.count('(') > joined.count(')')):
                break
            j += 1
            parts.append(re.sub(r'\s*//.*$', '', src[j]).strip())
        out.append((i, j, parts[0] + ''.join(p for p in parts[1:])))
        i = j + 1
    return out


def wrap(path, src, do):
    """One masked term per line, bit position first.  CLEANER.md Phase 4.

    The six context words are 468 to 664 characters and eleven masked terms
    each, and `--apply` above refuses all six because fewer than half of the
    terms are provably sign tests.  That refusal is about what the line *means*;
    it says nothing about the line being unreadable, which is a separate
    complaint with a separate and much cheaper answer.

    So this changes nothing but where the newlines are.  The terms keep their
    masks and their order, the `|` moves to the front of each continuation so
    the operator is visible without counting to the end of the line, and each
    line is labelled with the bit its mask keeps -- which is the one thing the
    original spelling makes you work out by hand, eleven times, per word.
    """
    n = 0
    for i, last, l in reversed(logical(src)):
        if not re.search(r'&0x[0-9A-F]+\|', l):
            continue
        head, rhs = l.split('=', 1)
        rhs = rhs.strip().rstrip(';')
        terms = split_terms(rhs)
        if len(terms) < 4:
            continue
        # Rebuild from the original text, keeping every character.  A term is
        # emitted with the join that *precedes* it, so the extra parenthesis
        # these lines wrap eight of their eleven terms in stays where it was and
        # the `// bit` label is always the last thing on its line.  Labelling
        # after the join instead comments the next term out, which is how the
        # first version of this produced a file that still compiled.
        lead = head.rstrip() + ' = '
        pad = ' ' * (len(head.rstrip()) - len(head.rstrip().lstrip()) + 5)
        out, prev = [], 0
        for k, (_t, mask, s, e) in enumerate(terms):
            glue = rhs[prev:s]
            bit = mask.bit_length() - 1
            body = (lead if k == 0 else pad) + glue + rhs[s:e]
            out.append('%-72s // bit %d' % (body, bit))
            prev = e
        if prev < len(rhs):
            out.append(pad + rhs[prev:] + ';')
        else:
            out[-1] = out[-1].rstrip()
            out[-1] = re.sub(r'\s+(// bit \d+)$', r';   \1', out[-1])
        n += 1
        if do:
            src[i:last + 1] = out
    return n


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'alt_p2_context.inc'
    src = open(path).read().split('\n')
    if '--wrap' in sys.argv:
        n = wrap(path, src, '--apply' in sys.argv)
        if '--apply' in sys.argv:
            open(path, 'w').write('\n'.join(src))
        print('%d masked context words wrapped, one term per line' % n)
        return 0
    want = int(sys.argv[sys.argv.index('--line') + 1]) if '--line' in sys.argv else None
    n_conv = n_raw = seen = 0
    for at, last, l in logical(src):
        i = at + 1
        if not re.search(r'&0x[0-9A-F]+\|', l):
            continue
        seen += 1
        if want and i != want:
            continue
        head, rhs = l.split('=', 1)
        rhs = rhs.strip().rstrip(';')
        terms = split_terms(rhs)
        pieces = []
        for t, m, _s, _e in terms:
            if not (m and not (m & (m - 1))):
                continue
            piece, ok = convert(t, m)
            pieces.append((m.bit_length() - 1, piece))
            n_conv += ok
            n_raw += not ok
        rest = residue(rhs, terms)
        print('%s:%d  %2d masked -> %d bit<>, %d raw;  %d other piece(s)'
              % (path, i, len(pieces),
                 sum(1 for _, p in pieces if p.startswith('.bit<')),
                 sum(1 for _, p in pieces if p.startswith('.raw')), len(rest)))
        for r in rest:
            done = row(r)
            pieces.append((-1, done if done else '.raw(%s)' % r))
        if want:
            # Before printing anything: a line where most terms stay masked
            # reads worse converted than it did before -- eleven
            # `.raw((expr)&0x...)` and one `.bit<>` is the old line with
            # scaffolding on it.  Six of the nine lines are like that, every
            # term carrying a `run`, so this refuses them rather than leaving
            # the judgement to whoever runs it.  `--force` to override one.
            n_bit = sum(1 for _, p in pieces if p.startswith('.bit<'))
            n_msk = sum(1 for _, p in pieces if p.startswith('.raw((')) + n_bit
            if n_bit * 2 <= n_msk and '--force' not in sys.argv:
                print('   declined: %d of %d masked terms convert; the rest carry'
                      ' a `run` or sit below their weight\'s bit (--force to'
                      ' override)' % (n_bit, n_msk))
                return 1
            body = [p for _, p in sorted(pieces, reverse=True)]
            indent = ' ' * (len(src[at]) - len(src[at].lstrip()))
            lead = '%s%s= CtxIdx{}' % (indent, head.strip() + ' ')
            out = [lead + body[0]]
            pad = ' ' * (len(lead) - len('CtxIdx{}'))
            for b2 in body[1:]:
                out.append(pad + b2)
            out[-1] += ';'
            text = '\n'.join(out)
            print(text)
            if '--apply' in sys.argv:
                src[at:last + 1] = out
                open(path, 'w').write('\n'.join(src))
                print('-- applied to %s:%d' % (path, i))
    if not want:
        if not seen:
            # `sweep.sh` hands every tool a copy of `bmf.cpp`, which is the
            # include list: no masked context words are in it, and "0 terms
            # convert" from a file that has none reads exactly like a tree with
            # nothing left to do.  Say which it is.
            where = 'alt_p2_context.inc'
            print('not applicable: %s has no masked context words%s'
                  % (path, '' if path.endswith(where)
                     else ' -- they are in ' + where))
            return 0
        print('%d terms convert, %d stay masked' % (n_conv, n_raw))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:      # `| head` is not an error
        sys.exit(0)
