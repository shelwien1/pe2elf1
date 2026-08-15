#!/usr/bin/env python3
"""A bit of a header byte, masked as a number instead of by its name.

    python3 tools/flagnames.py bmf.cpp
    python3 tools/flagnames.py bmf.cpp --all   # also list the names in force

This program has five bytes that are bit fields, and until they were given
enums every one of them was read as a literal: `depth & 0x40` and `flags & 0x40`
are different questions about different bytes, and the source said `0x40`
twice.  Four enums name them now -- `depth_*` and `flags_*` in `bmp.inc`,
`desc_*` and `pred_*` beside `PlaneDesc`, `ctx_*` beside `ctx_group_flags` --
and this is the check that they stay named.

**The rule.**  An identifier whose name carries `flag` or `depth` may not be
combined with a numeric literal by `&`, `|` or `^`.  That is deliberately about
the *name* rather than about a type: the same byte is reached as `img->flags`,
`hdr.flags`, `frame.flags_b`, `hdr_flags`, `best_flags` and `desc_flags`
depending on which side of the round trip you are on, and a rule that followed
the type would have to know that `ExpandImageFrame::flags_b` and
`BmfImage::flags` are the same eight bits.  A rule about the name catches all
six spellings and costs one convention: a bit field's variable says so.

Shifts are not reported.  `flags<<2` in the descriptor packer is the wire
format -- four bits of `flags` over two of `nrefs` -- and so is the `>>2` that
reads it back; those are a layout, not a mask.  What this is looking for is a
*test*, and a test is `&`.

**Which literals count.**  A single bit, an enumerator the unit declares, or
one byte with an enumerator cleared out of it (`flags & 0xFB` was
`flags & ~desc_alt_model`).  Everything else is left alone, and the reason is
`3<<(depth_b&31)`: a shift count masked to five bits is not a question about
the palette bit, it is the x86 shift the decompiler wrote out.  31 is neither a
bit nor a name, so it does not answer, while `& 0x3F` -- the same shape, one
bit wider -- does, because that one *is* `depth_bits`.  A line inside an `enum`
body is skipped for the same reason: `depth_one_plane = depth_grey|8` is the
declaration, not a use of it.

**What it prints.**  The site, and -- when the literal matches an enumerator
the unit declares -- what it should have said.  The value is looked up across
every enum in the unit rather than the one that fits the field, because a
suggestion that names the wrong byte is exactly the confusion this exists to
stop: `0x40` finds both `depth_grey` and `flags_packed`, and both are printed.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

# A name that carries `flag` or `depth`, then `&`, `|` or `^`, then a number --
# or the same two the other way round.  `&&` and `||` are excluded by the
# character class: `flags&&x` is a logical test of a whole byte and says
# nothing about a bit.
# `\b\w*` and not `[A-Za-z_]\w*`: the second needs a character in front of the
# word it is looking for, so `frame.depth_f&0x80` did not match and `hdr_flags`
# did.  Half a rule is worse than none -- it reports enough to look like it is
# working.
NAME = r'\b\w*(?:flag|depth)\w*'
NUM = r'(?:0[xX][0-9a-fA-F]+|\d+)'
HIT = re.compile(r'(?:(%s)\s*([&|^])\s*(~?)(%s)\b|(%s)\s*([&|^])\s*(~?)\s*(%s)\b)'
                 % (NAME, NUM, NUM, NAME))

# `enum { name = value, ... }` anywhere in the unit, at any indent.  Only the
# enumerators whose value is a literal are collected -- `try_p1 = pred_p1|
# desc_alt_model` is a composition of names and already says what it is.
ENUM = re.compile(r'\benum\b[^{;]*\{(.*?)\}\s*;', re.S)
ENUMERATOR = re.compile(r'(\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*(?:,|$)', re.M)


def names(lines):
    """{value: [enumerator, ...]} over every enum in the unit."""
    out = {}
    for m in ENUM.finditer('\n'.join(lines)):
        for e in ENUMERATOR.finditer(m.group(1)):
            out.setdefault(int(e.group(2), 0), []).append(e.group(1))
    return out


def enum_lines(lines):
    """The line numbers an `enum { ... };` body covers."""
    text = '\n'.join(lines)
    starts = [0]
    for l in lines:
        starts.append(starts[-1] + len(l) + 1)
    out = set()
    for m in ENUM.finditer(text):
        a = next(i for i in range(len(lines)) if starts[i + 1] > m.start())
        b = next(i for i in range(len(lines)) if starts[i + 1] > m.end() - 1)
        out.update(range(a, b + 1))
    return out


def answers(v, known):
    """Whether a literal of this value is asking about a bit."""
    return v in known or (v and not (v & (v - 1))) or (0xFF ^ v) in known


def survey(path):
    """[(file, line, text, [what it could have said])]."""
    lines, origin = structs.splice(path)
    known = names(lines)
    inside = enum_lines(lines)
    out = []
    for i, l in enumerate(lines):
        if i in inside:
            continue
        body = l.split('//')[0]
        for m in HIT.finditer(body):
            g = m.groups()
            lit = int(g[3] if g[0] else g[7], 0)
            if not answers(lit, known):
                continue
            out.append((origin[i][0], origin[i][1], m.group(0).strip(),
                        known.get(lit, []) or
                        ['~%s' % n for n in known.get(0xFF ^ lit, [])]))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('-') \
        else 'bmf.cpp'
    found = survey(path)
    for f, ln, text, could in found:
        print('  %-26s %-34s %s' % ('%s:%d' % (f, ln), text,
                                    ' or '.join(could) or '(no name for it)'))
    if '--all' in sys.argv:
        lines, _origin = structs.splice(path)
        for v, ns in sorted(names(lines).items()):
            print('  0x%02X  %s' % (v, ', '.join(ns)))
    print('%d bits of a header byte masked by number rather than by name'
          % len(found))


if __name__ == '__main__':
    main()
