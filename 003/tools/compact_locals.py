#!/usr/bin/env python3
"""Compact one decompiled function's block of local declarations.

    python3 tools/compact_locals.py input.hpp output.hpp funcname [mode]

Hex-Rays emits every local on its own line, which for a body of any size is a
screenful of noise before the code starts:

    int32_t v5;
    const char *a_b;
    FILE *Stream_v;
    int32_t v8;
    int32_t v9;
    FILE *Stream_1;

mode 1 (the default) merges *runs* of adjacent declarations that share a type,
so the order the decompiler chose is preserved:

    int32_t v5;
    const char *a_b;
    FILE *Stream_v;
    int32_t v8, v9;
    FILE *Stream_1;

mode 2 sorts by type first, so every type appears once:

    FILE *Stream_v, *Stream_1;
    const char *a_b;
    int32_t v5, v8, v9;

Only the run of plain declarations that follows the `;` Hex-Rays puts at the
top of a body is touched, and within it only lines this can merge without
changing what they mean.  Left alone:

  * anything with an initialiser, which includes every `__hexrays_frame` alias
    (those are references bound to offsets in the frame buffer, and both their
    order and their addresses matter);
  * `alignas(...)` declarations, since the attribute would silently spread to
    everything merged with them;
  * function-pointer declarators, whose syntax does not survive a comma list.

Sorting is by type name, and stable, so declarations of one type keep their
relative order.  It moves nothing across the block's boundaries and no
declaration has an initialiser, so it cannot change behaviour -- but it does
change the order the compiler assigns stack slots in, and this is decompiled
code that is not always as well-defined as it looks, so verify after using it.
"""
import re
import sys

# `  <type> <declarator>;` with nothing clever in it.  The declarator carries
# its own stars and array bounds so that `char *p;` and `char a[4];` merge.
DECL = re.compile(r'^(?P<indent> +)'
                  r'(?P<type>(?:const\s+|volatile\s+|unsigned\s+|signed\s+)*'
                  r'[A-Za-z_][A-Za-z0-9_]*)'
                  r'\s+(?P<decl>\**\s*[A-Za-z_][A-Za-z0-9_]*(?:\[[^\]\n]*\])*)'
                  r'\s*;$')
# `  <type> <declarator> = ...;` -- a declaration this leaves alone, but which
# still belongs to the block.  Two tokens before the `=` is what separates it
# from a plain assignment.
INIT = re.compile(r'^ +(?:const\s+|volatile\s+|unsigned\s+|signed\s+)*'
                  r'[A-Za-z_][A-Za-z0-9_:]*'
                  r'\s+\**\s*[A-Za-z_][A-Za-z0-9_]*(?:\[[^\]\n]*\])*\s*=.*;$')
KEYWORDS = {'return', 'if', 'while', 'for', 'do', 'else', 'switch', 'case',
            'goto', 'break', 'continue', 'default', 'sizeof', 'typedef',
            'static', 'alignas'}
WIDTH = 96


def is_decl(line):
    """Does `line` declare something, whether or not this can merge it?"""
    t = line.strip()
    return (t.endswith(';') and '=' not in t and '++' not in t and '--' not in t
            and len(t.split()) >= 2 and t.split()[0] not in KEYWORDS)


def split_block(lines, start):
    """The run of declarations beginning at `start`.

    Returns (end, segments).  A segment is a list of (type, declarator, line
    index) that may be merged and reordered among themselves.  Declarations
    with an initialiser are not merged *and* start a new segment, so nothing
    ever moves across one -- an initialiser can name an earlier local, and
    keeping the segments apart is what makes reordering safe.  Declarations
    this cannot merge but which have no initialiser (`alignas`, function
    pointers) are stepped over and left in place.  The run ends at the first
    statement.
    """
    segments, i = [[]], start
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        m = DECL.match(line)
        if m and m.group('type') not in KEYWORDS:
            segments[-1].append(
                (m.group('type'), re.sub(r'\s+', '', m.group('decl')), i))
            i += 1
            continue
        if is_decl(line):                       # declaration, just not one to merge
            i += 1
            continue
        # An initialised declaration -- `int v3 = a1;`, a frame alias -- still
        # belongs to the block, but closes the segment.  Anything else is the
        # first statement.
        if INIT.match(line) and line.strip().split()[0] not in KEYWORDS:
            segments.append([])
            i += 1
            continue
        break
    return i, [s for s in segments if s]


def render(indent, type_name, decls):
    """`indent type d1, d2, d3;`, wrapped."""
    out, line = [], '%s%s %s' % (indent, type_name, decls[0])
    cont = indent + ' ' * (len(type_name) + 1)
    for d in decls[1:]:
        if len(line) + 2 + len(d) > WIDTH:
            out.append(line + ',')
            line = cont + d
        else:
            line += ', ' + d
    out.append(line + ';')
    return out


def compact(text, funcname, mode=1):
    """Rewrite `funcname`'s declaration block.  Returns (text, n_merged)."""
    lines = text.split('\n')

    # The body opens with the signature, `{`, then Hex-Rays' bare `;`.  The
    # signature may wrap over several lines, and may carry a `BMF_SSE` prefix
    # or trail a previous body's `}`, so match on the name and work outwards.
    sig = None
    pat = re.compile(r'\b%s\s*\(' % re.escape(funcname))
    for i, line in enumerate(lines):
        code = line.split('//')[0]
        m = pat.search(code)
        if m is None:
            continue
        head = code[:m.start()]
        # `=` or an open paren to the left means this is a call, not a
        # definition; a trailing `;` means it is a declaration.
        if 'static inline' in head or '=' in head or '(' in head:
            continue
        if code.rstrip().endswith(';'):
            continue
        j = i
        while j < len(lines) and j < i + 40 and '{' not in lines[j]:
            j += 1
        if j < len(lines) and j < i + 40:
            sig = j
            break
    if sig is None:
        return text, 0

    start = sig + 1
    while start < len(lines) and lines[start].strip() != ';':
        # frame aliases and blank lines sit between `{` and the `;`
        if lines[start].strip() in ('', '}') or start > sig + 400:
            break
        start += 1
    if start >= len(lines) or lines[start].strip() != ';':
        return text, 0
    start += 1

    end, segments = split_block(lines, start)
    segments = [s for s in segments if len(s) >= 2]
    if not segments:
        return text, 0

    replace, drop, merged = {}, set(), 0
    for seg in segments:
        if mode == 2:
            order, seen = [], {}
            for t, d, _ in seg:                      # stable: first use wins
                if t not in seen:
                    seen[t] = []
                    order.append(t)
                seen[t].append(d)
            groups = [(t, seen[t]) for t in sorted(order)]
        else:
            groups, prev = [], None
            for t, d, _ in seg:
                if prev == t:
                    groups[-1][1].append(d)
                else:
                    groups.append((t, [d]))
                    prev = t

        indent = re.match(r'^( +)', lines[seg[0][2]]).group(1)
        out = []
        for t, ds in groups:
            out.extend(render(indent, t, ds))
        if len(out) >= len(seg):                     # wrapping ate the gain
            continue
        merged += len(seg) - len(out)
        replace[seg[0][2]] = out                     # merged block sits first
        drop.update(e[2] for e in seg[1:])

    if not merged:
        return text, 0

    body = []
    for k in range(start, end):
        if k in replace:
            body.extend(replace[k])
        elif k not in drop:
            body.append(lines[k])
    return '\n'.join(lines[:start] + body + lines[end:]), merged


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    src, dst, func = sys.argv[1:4]
    mode = int(sys.argv[4]) if len(sys.argv) > 4 else 1
    if mode not in (1, 2):
        sys.exit('mode must be 1 or 2')
    text = open(src).read()
    new, merged = compact(text, func, mode)
    open(dst, 'w').write(new)
    print('%s: %d declaration lines removed (mode %d)' % (func, merged, mode))


if __name__ == '__main__':
    main()
