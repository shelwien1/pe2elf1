#!/usr/bin/env python3
"""Compact one decompiled function's block of local declarations.

    python3 tools/compact_locals.py input.hpp output.hpp funcname [mode]
    python3 tools/compact_locals.py subs1.hpp [mode]     # report, write nothing

Hex-Rays emits every local on its own line, which for a body of any size is a
screenful of noise before the code starts:

    int32_t v5;
    const char *a_b;
    FILE *Stream_v;
    int32_t v8;
    int32_t v9;
    FILE *Stream_1;

mode 1 (the default) merges *runs* of adjacent declarations that share a type,
in place, so the order the decompiler chose is preserved:

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
# The `const` may sit *after* the stars -- `BmfImage *const img = ...` is how
# this file writes a view bound once -- and a frame alias is `AltP1Block *&p =
# ...`, a reference.  Leaving either out ended the block at the first one, which
# in the framed bodies is before any local has been seen.
INIT = re.compile(r'^ +(?:const\s+|volatile\s+|unsigned\s+|signed\s+)*'
                  r'[A-Za-z_][A-Za-z0-9_:]*'
                  r'\s*[\*&]*\s*(?:const\s+|volatile\s+)*[\*&]*\s*'
                  r'[A-Za-z_][A-Za-z0-9_]*(?:\[[^\]\n]*\])*\s*=.*;$')
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
        # A trailing comment is not the end of the declaration block, and
        # treating it as one is why this reported zero on every body whose
        # first local is documented -- which after the naming rounds is most of
        # them.  The line is matched without its comment; a documented
        # declaration is then stepped over rather than merged, because the
        # comment belongs to the name it follows and a comma list has nowhere
        # to put it.
        bare = line.split('//')[0].rstrip()
        # Hex-Rays' bare `;`, the empty statement it puts at the top of a body.
        # In the framed bodies it sits *after* the frame and its aliases, with
        # twenty more locals below it, and treating it as the first statement
        # put all of them out of reach.
        if bare.strip() == ';':
            i += 1
            continue
        # The frame struct is a declaration too, and it sits in the middle of
        # the block with the locals that outlived it on both sides.  Stepped
        # over as one unit, braces and all: it is `struct alignas(16) …{ … }
        # frame;` and nothing inside it is a local.
        if re.match(r'\s*(?:struct|union)\b', bare):
            depth = 0
            while i < len(lines):
                c = lines[i].split('//')[0]
                depth += c.count('{') - c.count('}')
                i += 1
                if depth <= 0 and ';' in c:
                    break
            segments.append([])
            continue
        # A `static_assert` is a declaration, not a statement, and it is how
        # this file pins a frame's layout -- so it sits in the middle of the
        # declaration block and used to end it, leaving everything below out of
        # reach.  Stepped over, however many lines it wraps to.
        if bare.lstrip().startswith('static_assert'):
            while i < len(lines) and ';' not in lines[i].split('//')[0]:
                i += 1
            i += 1
            continue
        if bare != line.rstrip():
            # A comment on its own line is not the end of the block either --
            # after the naming rounds these blocks are half prose, and treating
            # the first paragraph as the first statement stopped this at the
            # top of every body worth running it on.
            if not bare.strip() or is_decl(bare):
                i += 1
                continue
            break
        m = DECL.match(line)
        if m and m.group('type') not in KEYWORDS:
            segments[-1].append(
                (m.group('type'), re.sub(r'\s+', '', m.group('decl')), i))
            i += 1
            continue
        if is_decl(line):                       # declaration, just not one to merge
            i += 1
            continue
        # A declaration this tool already wrapped over several lines.  Its
        # continuations end in `,` rather than `;`, so neither branch above
        # matches and the block used to end at the first one -- which meant a
        # second run stopped at whatever the first had merged, and the file
        # kept 274 mergeable lines while this reported nothing left to do.
        # Stepped over, not merged: unpicking a comma list to re-lay it is a
        # different job from joining single declarations.
        if bare.endswith(',') and len(bare.split()) >= 2 \
                and bare.split()[0] not in KEYWORDS and '=' not in bare:
            while i < len(lines) and ';' not in lines[i].split('//')[0]:
                i += 1
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

    # Where the declarations start.  This used to look for the bare `;`
    # Hex-Rays puts at the top of a body and give up if a blank line or a `}`
    # came first -- which is every body with a frame, because the frame struct
    # sits between the `{` and that `;`.  Those are the seventeen largest
    # bodies in the file, so the tool had never run on the code that needed it
    # most: it reported success on `alt_p1_model` and nothing at all on
    # `alt_p2_model`, `decode_pixel` and `code_pixel`.
    #
    # So step over what is known to sit there instead: comments, blank lines,
    # the bare `;`, a `struct`/`union` frame declaration, and the
    # `static_assert`s that pin its layout.  The first thing that is none of
    # those is where the declarations begin.
    start = sig + 1
    # Some signatures carry the `{` *and* the first declaration on one line:
    # `int32_t __alt_p2_context(...) {   P2Ctx *unused_p,` continues onto the
    # next.  Starting the block at that continuation puts it in the middle of a
    # declaration, where nothing matches and everything below is lost.  Step to
    # the end of it first.
    tail = lines[sig].split('//')[0].split('{', 1)[-1].strip()
    if tail:
        while start < len(lines) and ';' not in lines[start].split('//')[0]:
            start += 1
        start += 1
    while start < len(lines):
        bare = lines[start].split('//')[0].strip()
        if bare in ('', ';'):
            start += 1
            continue
        if re.match(r'(?:struct|union)\b', bare) or bare.startswith('static_assert'):
            depth = 0
            while start < len(lines):
                c = lines[start].split('//')[0]
                depth += c.count('{') - c.count('}')
                start += 1
                if depth <= 0 and ';' in c:
                    break
            continue
        break
    if start >= len(lines):
        return text, 0

    end, segments = split_block(lines, start)
    segments = [s for s in segments if len(s) >= 2]
    if not segments:
        return text, 0

    replace, drop, merged = {}, set(), 0
    for seg in segments:
        if mode == 2:
            order, seen = [], {}
            for t, d, i in seg:                      # stable: first use wins
                if t not in seen:
                    seen[t] = []
                    order.append(t)
                seen[t].append((d, i))
            groups = [(t, [d for d, _ in seen[t]], [i for _, i in seen[t]])
                      for t in sorted(order)]
        else:
            groups, prev = [], None
            for t, d, i in seg:
                if prev == t:
                    groups[-1][1].append(d)
                    groups[-1][2].append(i)
                else:
                    groups.append((t, [d], [i]))
                    prev = t

        indent = re.match(r'^( +)', lines[seg[0][2]]).group(1)
        if mode == 2:
            # Sorting *is* relocation, so the whole segment is re-laid at its
            # first line.
            out = []
            for t, ds, _ in groups:
                out.extend(render(indent, t, ds))
            if len(out) >= len(seg):                 # wrapping ate the gain
                continue
            merged += len(seg) - len(out)
            replace[seg[0][2]] = out
            drop.update(e[2] for e in seg[1:])
            continue
        # Mode 1 promises the order the decompiler chose, and for a long time
        # it did not keep that promise: every group was re-emitted at the
        # *segment's* first line, so a declaration separated from its
        # neighbours by a comment, a comma list or Hex-Rays' bare `;` was
        # pulled up past all of them -- `choose_plane_coding`'s `int16_t
        # g1_lo;` moved nineteen lines and across the `;`.  Nothing breaks,
        # because none of these has an initialiser; the docstring was simply
        # describing mode 2.  Each group is laid at its own first line now, so
        # a run merges where it stands and everything else keeps its place.
        for t, ds, idx in groups:
            if len(ds) < 2:
                continue
            out = render(indent, t, ds)
            if len(out) >= len(ds):                  # wrapping ate the gain
                continue
            merged += len(ds) - len(out)
            replace[idx[0]] = out
            drop.update(idx[1:])

    if not merged:
        return text, 0

    body = []
    for k in range(start, end):
        if k in replace:
            body.extend(replace[k])
        elif k not in drop:
            body.append(lines[k])
    return '\n'.join(lines[:start] + body + lines[end:]), merged


def survey(path, mode=1):
    """Every body this could still merge, without writing anything.

    Asking "is there anything left?" used to mean a shell loop over every
    function name, which is not a thing anyone runs and so is not a thing
    `sweep.sh` can check.  With one argument this reports instead of rewriting,
    which puts its zero in the sweep beside the others.
    """
    sys.path.insert(0, __file__.rsplit('/', 1)[0])
    import structs
    text = open(path).read()
    lines = text.split('\n')
    out = []
    for _a, _b, name, _sig in structs.bodies(lines):
        _, n = compact(text, name, mode)
        if n:
            out.append((name, n))
    return out


def main():
    if len(sys.argv) == 2 or (len(sys.argv) == 3 and sys.argv[2] in ('1', '2')):
        path = sys.argv[1]
        mode = int(sys.argv[2]) if len(sys.argv) > 2 else 1
        found = survey(path, mode)
        for name, n in found:
            print('%6d  %s' % (n, name.lstrip('_')))
        print('%d declaration lines mergeable in %d bodies (mode %d)'
              % (sum(n for _, n in found), len(found), mode))
        return
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
