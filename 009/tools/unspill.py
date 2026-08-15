#!/usr/bin/env python3
"""Give a frame's spill area one member per slot.

    python3 tools/unspill.py subs1.hpp --list
    python3 tools/unspill.py subs1.hpp decode_pixel sym

REFACTORING5.md §5.1.  Round three declared these as arrays because a byte
range is what they looked like:

    uint32_t sym[32];
    ((ModelBlock * &)frame.sym[5]) = (ModelBlock *)(_this);
    v14 = *((uint16_t *)((ModelBlock * &)frame.sym[5]) + n4_8 + 3029720);
    ((int32_t &)frame.sym[0]) = n4_8;

An array whose every subscript is a constant and whose elements have four
different types is not an array; it is the stack area MSVC packed several
locals into.  Each slot becomes a member of its own, typed by what reads it.

**The layout cannot move**, so this only takes slots that are all one width
and only gives a member a type of that width; the frame's `static_assert`
is what says the split was free.  Where a slot has two readings of the same
width -- an `int32_t` and a pointer, which is `alt_p1_context`'s case and
`unoffset.py`'s -- the majority reading becomes the member's type and the
other stays a cast, spelled at the member instead of at a subscript.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402
import unalias                                                    # noqa: E402
import merge                                                      # noqa: E402

DECL = re.compile(r'^(\s*)([\w]+)\s*(\*?)\s*(\w+)\[(\d+)\];(.*)$')


def spills(lines):
    """{function: {member: (decl line, count, {slot: Counter(type)})}}"""
    out = {}
    for a, b, nm, _ in structs.bodies(lines):
        fr = unalias.frame_of(lines, a, b)
        if not fr:
            continue
        decl, use = {}, collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
        for i in range(fr[0], fr[1] + 1):
            m = DECL.match(lines[i].split('//')[0])
            if m:
                decl[m.group(4)] = (i, int(m.group(5)))
        bare, sub = collections.Counter(), collections.defaultdict(set)
        for i in range(fr[1] + 1, b + 1):
            code = lines[i].split('//')[0]
            for m in re.finditer(r'\(\(([\w ]+\**)\s*&\)frame\.(\w+)\[(\d+)\]\)', code):
                use[m.group(2)][int(m.group(3))][m.group(1).strip()] += 1
            for m in re.finditer(r'frame\.(\w+)\[([^\]]*)\]', code):
                if not m.group(2).strip().isdigit():
                    sub[m.group(1)].add(m.group(2))
            for m in re.finditer(r'frame\.(\w+)(?![\[\w])', code):
                bare[m.group(1)] += 1
        g = {}
        for mem, (i, n) in decl.items():
            if mem in use and len(use[mem]) > 1:
                g[mem] = (i, n, use[mem], bare[mem], sorted(sub[mem]))
        if g:
            out[nm.lstrip('_')] = g
    return out


def split(lines, fn, mem, found):
    """Rewrite one member.  Returns (slots, sites) or None if it cannot."""
    i, n, use, bare, walk = found[fn][mem]
    # `decode_pixel` and `code_pixel` both call their spill area `sym`, so the
    # rewrite is bounded by the declaring body -- a file-wide sub renamed both.
    span = next((a, b) for a, b, x, _ in structs.bodies(lines) if x.lstrip('_') == fn)
    m = DECL.match(lines[i].split('//')[0])
    indent, base, star, _, _, tail = m.groups()
    width = 4 if star else merge.width(base)
    for slot, tys in use.items():
        for ty in tys:
            if merge.width(ty) != width:
                print('%s[%d] is read as %s, not %d bytes' % (mem, slot, ty, width))
                return None
    pick = {k: v.most_common(1)[0][0] for k, v in use.items()}
    # A union rather than a replacement: `pixel_context(this, frame.sym)`
    # takes the array whole, so the array has to stay.  The named layer is the
    # same bytes under the names the spilled locals are read by.
    decl = [indent + 'union {', '    ' + lines[i]]
    if walk:
        decl += [indent + '    // The locals MSVC spilled into these bytes.  The walk above reaches',
                 indent + '    // the same slots again with an expression, which is the same storage',
                 indent + '    // at a later point in the function and not a second variable.',
                 indent + '    struct {']
    else:
        decl.append(indent + '    struct {   // the locals MSVC spilled into these bytes')
    top = max(use) + 1
    elem = ('%s %s' % (base, star)).strip()
    for k in range(top):
        ty = pick.get(k, elem)
        sep = '' if ty.endswith('*') else ' '
        decl.append('%s      %s%s%s%d;' % (indent, ty, sep, mem, k))
    if top < n:
        # Everything past the last spilled slot is the array the member is
        # named for; naming 8184 of them one at a time says nothing.
        sep = '' if elem.endswith('*') else ' '
        decl.append('%s      %s%s%s_tail[%d];' % (indent, elem, sep, mem, n - top))
    decl += [indent + '    };', indent + '};']

    sites = 0
    rx = re.compile(r'\(\(([\w ]+\**)\s*&\)frame\.%s\[(\d+)\]\)' % re.escape(mem))

    def rep(g):
        nonlocal sites
        sites += 1
        ty, k = g.group(1).strip(), int(g.group(2))
        name = 'frame.%s%d' % (mem, k)
        return name if ty == pick.get(k) else '(*(%s *)&%s)' % (ty, name)

    for j in range(span[0], span[1] + 1):
        code, sep, com = lines[j].partition('//')
        new = rx.sub(rep, code)
        if new != code:
            lines[j] = new + sep + com
    lines[i:i + 1] = decl
    return n, sites


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]
    found = spills(lines)

    if len(args) < 2:
        for fn, g in found.items():
            for mem, (i, n, use, bare, walk) in g.items():
                tys = sorted({t for c in use.values() for t in c})
                print('%-22s %-6s %3d slots, %2d used, %3d casts, %s%s'
                      % (fn, mem, n, len(use),
                         sum(sum(c.values()) for c in use.values()), tys,
                         ', %d walks' % len(walk) if walk else ''))
        print('%d spill areas in %d functions'
              % (sum(len(g) for g in found.values()), len(found)))
        return 0

    fn, mem = args[0], args[1]
    if fn not in found or mem not in found[fn]:
        print('no such spill area')
        return 1
    got = split(lines, fn, mem, found)
    if not got:
        return 1
    open(path, 'w').write('\n'.join(lines))
    print('%s %s: %d slots became members, %d sites' % (fn, mem, got[0], got[1]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
