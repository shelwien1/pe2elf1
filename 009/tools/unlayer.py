#!/usr/bin/env python3
"""Drop a pointer cast the next cast immediately overrides.

    python3 tools/unlayer.py subs1.hpp
    python3 tools/unlayer.py subs1.hpp --all

    transform_planes((BmfImage *)(uint16_t *)frame.tile_img, ...)
    __alt_p1_context((AltP1Block *)(uint8_t **)_this, ...)
    __bmf_destroy_archive((BmfArc *)(FILE **)arc, 1)

The inner cast is consumed by the outer one before anything can be done with
it, so it contributes nothing but a second type to read past.  MSVC put the
argument in a register through one type and Hex-Rays wrote down both.

`uncast.py` and `decast.py` cannot see these, and the reason is worth stating:
both are driven by `-Wuseless-cast`, which fires only when the target type is
*identical* to the operand's.  `(uint16_t *)` applied to a `uint8_t *` is not
identical to anything, so the compiler is silent and stays silent -- these are
useless in a way it does not have a word for.

The safety argument is not the compiler's either.  It is that on this target a
pointer is four bytes and a cast between two pointer types is a reinterpretation
that computes nothing, so `(A *)(B *)x` reaches the same address as `(A *)x`
whether `x` is a pointer or an integer.  What the rule will not touch is a
cast with anything between it and the next one -- an inner cast whose result is
indexed, added to, or dereferenced is doing work, and the `\\s*` between the two
closing parens is what says it is not.
"""
import re
import sys

# Two pointer casts back to back, nothing between them.  The `\s*` is the whole
# condition: `(A *) foo((B *)x)` has a call in the way and is not a match.
LAYER = re.compile(r'(\(\s*[A-Za-z_]\w*\s*\*+\s*\))\s*'
                   r'(\(\s*[A-Za-z_]\w*\s*\*+\s*\))(?=\s*[A-Za-z_(&])')


def survey(lines):
    out = []
    for i, l in enumerate(lines):
        code = l.split('//')[0]
        for m in LAYER.finditer(code):
            out.append((i + 1, m.group(2), code.strip()[:78]))
    return out


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = survey(lines)
    for ln, inner, txt in found:
        print('%6d  %-14s %s' % (ln, ' '.join(inner.split()), txt))
    if '--all' in sys.argv:
        for i, l in enumerate(lines):
            head, _, tail = l.partition('//')
            # One pass is enough: three layers would need two, and there are
            # none -- but the loop below re-runs on its own output anyway, so
            # a third would go with the second rather than being left half done.
            while LAYER.search(head):
                head = LAYER.sub(r'\1', head, count=1)
            lines[i] = head + ('//' + tail if tail else '')
        open(path, 'w').write('\n'.join(lines))
        print('%d overridden casts removed' % len(found))
    else:
        print('%d pointer casts the next cast overrides' % len(found))
