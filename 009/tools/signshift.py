#!/usr/bin/env python3
"""Find the comparisons written as a shift of a sign bit.

    python3 tools/signshift.py               # every site, and what it means
    python3 tools/signshift.py --apply       # rewrite the ones it can prove

`(uint32_t)(a-b)>>31` is the sign bit of `a-b`, which is `a < b`.  MSVC emits
it because a branchless 0/1 is cheaper than a `setcc` in the middle of an
expression, and Hex-Rays keeps it because it is what the binary does.  The
program means `a < b`.

Three spellings, all of them here:

    (uint32_t)(a-b)>>31      -> a<b       the cast is on the difference
    (a-(uint32_t)b)>>31      -> a<b       the cast is on one operand, which
                                          makes the subtraction unsigned and
                                          the shift a borrow-out rather than a
                                          sign bit -- the same predicate
    ((x&7u)-7)>>31           -> (x&7)<7   the same, with an expression on the
                                          left instead of a name

**Why this needs an argument and not just a regex.**  The rewrite is exact only
if `a-b` does not overflow, and that is a fact about the values, not the
syntax.  Two of the sites here subtract a `uint8_t` sum from 16, and two
subtract a constant under 60 from an activity sum -- neither can overflow, and
each is said so at the site.  A site whose operands are two arbitrary `int32_t`
is *not* rewritten and is reported instead: for those the shift and the
comparison genuinely differ, at exactly one pair of inputs, and choosing which
one the program meant is not a job for a script.

So `--apply` rewrites only the sites listed in `PROVEN` below, each with the
bound that makes it safe.  Everything else is printed for a human.  This is the
same shape as `planedesc.py`'s `ONE_BASED`: the tool does the edit, a person
does the reasoning, and the reasoning is in the file rather than in a commit
message.
"""
import re
import sys
import glob

# The shifted expression, one level of nesting deep.  The outer parentheses are
# not required: `quiet = (x-16)>>31;` has none, and requiring them cost this
# tool the two `quiet` sites the first time it ran.
SHIFT = re.compile(r'\(([^()]*(?:\([^()]*\)[^()]*)*)\)\s*>>\s*31')

# The sites whose subtraction is proven not to overflow, and why.  The key is
# the text inside the shift; the value is (replacement, reason).
PROVEN = {
    '6-ctx_lo': ('ctx_lo>6', '`ctx_lo` is `ctx&0xF`, so 0..15'),
    '4-ctx_lo': ('ctx_lo>4', '`ctx_lo` is `ctx&0xF`, so 0..15'),
    '9-ctx_lo': ('ctx_lo>9', '`ctx_lo` is `ctx&0xF`, so 0..15'),
    # Not 0..255: `guess` is a MED prediction, `guess+west-northwest` of three
    # `uint8_t`, so -255..510.  It does not matter -- what the rewrite needs is
    # that `216-guess` fits in an int32, and it is three orders of magnitude
    # inside that.  Recorded because "it is a pixel so it is 0..255" is the
    # wrong reason for the right answer.
    '216-(uint32_t)guess': ('guess>216', '`216-guess` is nowhere near overflowing'),
    '22-(uint32_t)guess': ('guess>22', '`22-guess` is nowhere near overflowing'),
    '(uint32_t)r0[-1].mag-8': ('r0[-1].mag<8', '`mag` is `uint8_t`'),
    'r1[-1].mag+(uint32_t)r0[-1].mag-16':
        ('r1[-1].mag+r0[-1].mag<16', 'two `uint8_t`, so the sum is at most 510'),
    '(ctx[1]&7u)-7': ('(ctx[1]&7)<7', 'the mask makes the left side 0..7'),
    'q2-115': ('q2<115', '`q2` is a quantised magnitude, well inside int32'),
    'q2-17': ('q2<17', '`q2` is a quantised magnitude, well inside int32'),
}

# Shifts of a sign bit that are *not* a comparison, so there is nothing to
# rewrite.  Recorded rather than left to be re-read every time the tool runs.
DECLINED = {
    # `(img_w + sign(img_w+1) + 1)>>1` is signed division by two, rounding
    # toward zero: the sign term is the bias a signed `/2` adds before the
    # shift.  Not a predicate, and the value it produces is always 0 here
    # because `img_w` is a width.
    'img_w+1': 'the bias term of a signed division by two, not a predicate',
}


def sites():
    """(path, line, inner text) for every sign-shift in the tree."""
    out = []
    for path in sorted(glob.glob('*.inc') + glob.glob('*.cpp')):
        s = open(path).read()
        for m in SHIFT.finditer(s):
            # Not the ones inside a comment: this file's own prose describes
            # the idiom, and a tool that counts its own documentation is a
            # tool that can never reach zero.
            bol = s.rfind('\n', 0, m.start()) + 1
            if s[bol:m.start()].lstrip().startswith('//'):
                continue
            out.append((path, s[:m.start()].count('\n') + 1, m.group(1), m.group(0)))
    return out


def main():
    found = sites()
    todo = [f for f in found if f[2] in PROVEN]
    for path, line, inner, whole in found:
        if inner in DECLINED:
            print('  %-22s:%-4d %-34s    not a comparison: %s'
                  % (path, line, inner, DECLINED[inner]))
        elif inner in PROVEN:
            new, why = PROVEN[inner]
            print('  %-22s:%-4d %-34s -> %-26s %s'
                  % (path, line, inner, new, why or 'NO BOUND RECORDED'))
        else:
            print('  %-22s:%-4d %-34s    not proven -- read it and add it to PROVEN'
                  % (path, line, inner))
    unproven = [f for f in found if f[2] not in PROVEN and f[2] not in DECLINED]
    # The unread count first: that is the one that is a defect.  `sweep.sh`
    # reads the leading number, and it used to read `len(found)` -- every site
    # this recognises, proven and declined ones included -- which is a census.
    print('%d sign-shift comparisons neither proven nor declined; %d found, '
          '%d proven, %d declined'
          % (len(unproven), len(found), len(todo),
             sum(1 for f in found if f[2] in DECLINED)))
    if '--apply' in sys.argv:
        n = 0
        for path in sorted(set(f[0] for f in found)):
            s = open(path).read()
            for inner in {f[2] for f in found if f[0] == path and f[2] in PROVEN}:
                whole = next(f[3] for f in found if f[2] == inner)
                n += s.count(whole)
                s = s.replace(whole, '(' + PROVEN[inner][0] + ')')
            open(path, 'w').write(s)
        print('-- applied to %d sites' % n)
    return 1 if unproven else 0


if __name__ == '__main__':
    sys.exit(main())
