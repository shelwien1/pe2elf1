#!/usr/bin/env python3
"""Check that a context word's sign claims are ones the types prove.

    python3 tools/ctxidx.py alt_p2_context.inc            # audit the chains
    python3 tools/ctxidx.py alt_p2_context.inc --list     # migrate: what is left
    python3 tools/ctxidx.py alt_p2_context.inc --line 242 --apply

Two jobs, and which one runs depends on what the file still holds.  While a
word is a chain of `expr & <one bit>` this migrates it; once every word is a
`CtxIdx` chain there is nothing to migrate and it audits instead.

That second job exists because of what happened when the first one finished.
All six words converted, no `& 0x…|` was left in the file, and this answered
`not applicable: no masked context words` -- which is this tool reporting a
clean tree because its subject changed shape, the exact failure it was written
to expose.  The subject did not go away.  A converted word carries claims, and
they are checkable by the same rule that made them.

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

  * and no additive constant, because the bound does not cover one.
    `208 - rb0->val` weighs 1 and passes every other test, and `208 -
    INT16_MIN` is 32976 -- bit 15 set on a positive value.  It never happens,
    since a `val` is a sample and stays small, but that is a fact about the
    model and this rule's entire claim is that it needs none.  The rule missed
    it for as long as declining was cheap enough that no line ever converted.

**What it declines, and writes as `bit_of<k>`.**

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

A declined term reads `.bit_of<15>(expr)`: the bit where it already is, with no
claim about the rest of the value.  Same code as the mask, and the position
generates the mask instead of sitting beside it as a hex literal with a `// bit
15` label that nothing checks.

That is the point, and it is why every line converts now.  The declined form
used to be `.raw((expr)&0x8000)` -- the original with scaffolding on it -- so
this refused all six words rather than make them worse, and the refusal was
right about `raw` and wrong about the words.  Which terms are proven and which
are believed is visible in the source either way; what `bit_of` adds is that
the position cannot disagree with the mask, sixty-six times over.

Across the six words it is 1 proven of 66.
"""
import re
from collections import Counter
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


# The wrapper two of the six words carry: `bank3 = (int32_t)( <chain> )>>11;`.
# Neither the cast nor the shift is a term, and both have to survive a rewrite.
WRAP = re.compile(r'^\s*\((?:u?int\d+_t)\)\(')
TRAIL = re.compile(r'(>>\s*\d+)\s*$')


def unwrap(rhs):
    """(prefix, core, trailer) -- the cast, the chain, and the shift.

    Splitting these off *before* the terms are read is what makes the rest of
    this file work on all six words rather than four.  Left in, `(int32_t)(`
    survives `residue` as a leftover piece and would be emitted as
    `.raw((int32_t)()`, and the `>>11` would be dropped -- which is the sort of
    thing a rewrite of a 500-character expression gets exactly one chance to
    get right.
    """
    m = WRAP.match(rhs)
    pre = m.group(0) if m else ''
    t = TRAIL.search(rhs)
    trail = t.group(1) if t else ''
    core = rhs[len(pre):len(rhs) - len(trail) if trail else len(rhs)]
    # The cast opened a parenthesis the chain now closes; drop its partner from
    # the end of the core so the rebuild does not emit it twice.
    depth = pre.count('(') - pre.count(')')
    while depth > 0 and core.rstrip().endswith(')'):
        core = core.rstrip()[:-1]
        depth -= 1
    return pre, core, trail


def unparen(s):
    """Strip parentheses that have no partner inside `s`, both ends."""
    s = s.strip()
    while s.startswith('(') and s.count('(') > s.count(')'):
        s = s[1:].strip()
    while s.endswith(')') and s.count(')') > s.count('('):
        s = s[:-1].strip()
    return s


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
    # An additive constant is not a coefficient, and the bound below does not
    # cover one.  `208 - rb0->val` weighed 1 and was declared a proven sign test
    # at bit 15 -- and it is not one: `val` is `int16_t`, so the difference
    # reaches 32976, which sets bit 15 while being positive.  It never happens,
    # because a `val` is a sample and stays small, but that is a fact about the
    # model and this rule's whole claim is that it does not need one.
    #
    # It had never fired: every line was refused before `--apply` could reach
    # it, so the one term this got wrong sat in `--list`'s count of convertible
    # terms and nowhere else.  Making the declines worth applying is what would
    # have shipped it.
    if re.search(r'(?<![\w*])\d+(?!\s*\*)', e):
        return False, 0
    weight = 0
    for m in re.finditer(r'(\d+)?\s*\*?\s*@', e):
        weight += int(m.group(1)) if m.group(1) else 1
    return True, weight


def convert(term, mask):
    """`.bit<k>(expr < 0)` where the sign is provable, `.bit_of<k>(expr)` else.

    The declined form used to be `.raw((expr)&0x2000000)`, and that is what
    made this tool refuse every line it looked at: eleven of those beside one
    `.bit<>` is the original with scaffolding on it, which the rule below said
    out loud and correctly.

    `CtxIdx::bit_of<Pos>` changes what a decline costs.  It keeps the bit where
    it is and claims nothing about the rest of the value -- so it is still a
    decline, and still visibly different from the `.bit<>` next to it -- but the
    position becomes a template parameter that generates the mask, instead of a
    hex literal with a `// bit 25` comment beside it that nothing checks.

    So a converted line now carries the same information it always did about
    which terms are proven and which are believed, and carries it in a form
    where a mask cannot disagree with its label.  The refusal is gone with its
    premise.
    """
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
    return '.bit_of<%d>(%s)' % (bit, inner), False


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


# A term of a converted word: `.bit<24>(expr < 0)` or `.bit_of<15>(expr)`.
# Anchored on the closing `)` of the chain element, which is the last one before
# a newline or the next `.bit`, so a subscript inside the expression does not
# end it early.
AUDIT = re.compile(r'\.bit(_of)?<(\d+)>\((.*?)\)\s*$', re.M)


def audit(path, src):
    """Re-check every converted term against the rule that converted it.

    Once the six words became `CtxIdx` chains this file had no `& 0x…|` left to
    find and answered `not applicable`, which is the failure it was built to
    expose, pointed at itself: a tool whose subject is gone reports exactly what
    a clean tree reports.

    The subject is not gone, it changed shape.  A converted word carries two
    kinds of term and the difference is a claim -- `bit<k>(expr < 0)` says the
    masked bit *is* the sign, `bit_of<k>(expr)` says only that it is bit k --
    and that claim is checkable with the same `leaves` rule that made it.  So
    this now reads the converted form and answers two questions:

      * a `bit<>` whose expression the rule cannot prove.  That is a defect: the
        code computes `(expr < 0) << k`, and if the bound does not hold that is
        not what the mask did.  This is not hypothetical -- `208 - rb0->val` was
        offered as provable by an earlier version of the rule, which did not
        count additive constants, and `208 - INT16_MIN` is 32976, over bit 15.
      * a `bit_of<>` the rule *can* prove.  Not a defect; the two forms compute
        the same thing when the proof holds.  It is a term whose meaning is
        stronger than its spelling, worth tightening, and it is counted apart.
    """
    text = '\n'.join(src)
    bad, tight = [], []
    for m in AUDIT.finditer(text):
        positional, bit, expr = bool(m.group(1)), int(m.group(2)), m.group(3)
        line = text[:m.start()].count('\n') + 1
        signed = expr.strip().endswith('< 0')
        if not positional and not signed:
            continue            # `bit<5>(a == b)` -- a plain flag, not this
        inner = expr[:expr.rfind('< 0')].strip() if signed else expr.strip()
        bounded, weight = leaves(inner)
        floor = 15
        while weight > (1 << (floor - 15)):
            floor += 1
        provable = bounded and 0 < weight and bit >= floor
        if signed and not provable:
            bad.append((line, bit, inner))
        elif positional and provable:
            tight.append((line, bit, inner))
    for line, bit, inner in bad:
        print('%s:%d  bit<%d> claims a sign the rule cannot prove: %s'
              % (path, line, bit, inner[:60]))
    for line, bit, inner in tight:
        print('%s:%d  bit_of<%d> is provably a sign; bit<%d>(… < 0) would say so'
              % (path, line, bit, bit))
    print('%d unfounded sign claims, %d positional terms that are provable'
          % (len(bad), len(tight)))
    return 1 if bad else 0


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
        pre, core, trail = unwrap(rhs)
        terms = split_terms(core)
        pieces = []
        for t, m, _s, _e in terms:
            if not (m and not (m & (m - 1))):
                continue
            piece, ok = convert(t, m)
            pieces.append((m.bit_length() - 1, piece))
            n_conv += ok
            n_raw += not ok
        rest = [unparen(r) for r in residue(core, terms)]
        rest = [r for r in rest if r]
        print('%s:%d  %2d masked -> %d proven sign, %d positional;'
              '  %d other piece(s)'
              % (path, i, len(pieces),
                 sum(1 for _, p in pieces if p.startswith('.bit<')),
                 sum(1 for _, p in pieces if p.startswith('.bit_of<')),
                 len(rest)))
        for r in rest:
            done = row(r)
            pieces.append((-1, done if done else '.raw(%s)' % r))
        if want:
            body = [p for _, p in sorted(pieces, reverse=True)]
            # The wrapper around the chain, kept verbatim.  Two of the six words
            # are `bank3 = (int32_t)( <chain> )>>11;` -- the cast and the shift
            # are not terms and must survive, and the chain replaces everything
            # between them.  So: whatever precedes the first term, then the
            # chain, then one `)` per parenthesis the prefix left open, then any
            # trailing operator.
            #
            # Reading the tail instead was the first attempt and it is where the
            # old `--apply` would have failed: these lines wrap eight of their
            # eleven terms in one extra parenthesis, so the tail carries closers
            # that belong to a group the chain has already absorbed.  Emitting
            # them produced `...24)));` -- one paren over -- on a path no line
            # ever reached, because all six were refused before it ran.
            closers = ')' * (pre.count('(') - pre.count(')'))
            indent = ' ' * (len(src[at]) - len(src[at].lstrip()))
            lead = '%s%s= %sCtxIdx{}' % (indent, head.strip() + ' ', pre)
            out = [lead + body[0]]
            pad = ' ' * (len(lead) - len('CtxIdx{}'))
            for b2 in body[1:]:
                out.append(pad + b2)
            out[-1] += closers + trail + ';'
            text = '\n'.join(out)
            # Nothing dropped, nothing duplicated.  Strip the masks, the
            # grouping and the whitespace out of both the original right-hand
            # side and the operands this is about to emit; the two must be the
            # same string.  A rewrite of six 500-character expressions that
            # feed the counter selection is not something to hand to the gate
            # on the strength of having read it.
            # As a character multiset, not as a string.  The pieces are sorted
            # by bit position on the way out and the source writes them in
            # whatever order the register allocator left, so the two differ in
            # order on at least one word -- and order does not matter here,
            # because every term is joined by `|`.  What does matter is that no
            # term is dropped, duplicated or mangled, and a multiset says that.
            def bare(s):
                s = re.sub(r'&\s*0x[0-9A-Fa-f]+', '', s)
                return sorted(re.sub(r'[()|\s]', '', s))
            was = bare(core)
            now = bare(''.join(t for t, _m, _s, _e in terms) + ''.join(rest))
            if was != now:
                print('   REFUSED: rebuilding %s:%d would not preserve it' % (path, i))
                print('   dropped: %r' % ''.join(sorted((Counter(was)
                                                         - Counter(now)).elements())))
                print('   added:   %r' % ''.join(sorted((Counter(now)
                                                         - Counter(was)).elements())))
                return 1
            print(text)
            if '--apply' in sys.argv:
                src[at:last + 1] = out
                open(path, 'w').write('\n'.join(src))
                print('-- applied to %s:%d' % (path, i))
    if not want:
        if not seen:
            # No masked words left to migrate.  That is the finished state, not
            # an absent subject: the words are here, converted, and the claims
            # they now carry are checkable.  `not applicable` was the honest
            # answer while this tool only knew how to migrate; answering it
            # after the migration would be a tool reporting a clean tree
            # because it stopped looking.
            if re.search(r'\.bit(_of)?<\d+>', '\n'.join(src)):
                return audit(path, src)
            where = 'alt_p2_context.inc'
            print('not applicable: %s has no masked context words%s'
                  % (path, '' if path.endswith(where)
                     else ' -- they are in ' + where))
            return 0
        # Every line converts now, so the count that matters is not how many
        # can be acted on -- it is how many of their terms are *proven* rather
        # than merely placed.  That number is 1 of 66, and it should stay
        # visible: `bit_of<>` makes a term's position checkable, and deliberately
        # does not make its meaning true.
        print('%d lines convert; %d terms proven a sign test, %d positional'
              % (seen, n_conv, n_raw))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:      # `| head` is not an error
        sys.exit(0)
