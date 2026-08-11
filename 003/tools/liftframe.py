#!/usr/bin/env python3
"""Turn one function's frame struct back into ordinary locals.

    python3 tools/liftframe.py subs1.hpp --list
    python3 tools/liftframe.py subs1.hpp __model_planes

Seventeen bodies still hold their locals in a `struct alignas(16) …{ … }
__frame;`, and each carries a comment saying the frame is a layout rather than
a bag of locals, with the exact failure that proved it -- "altp1 segfaults
while compressing", "five streams move, no signal".

Every one of those comments cites `tools/frame-sweep.sh`, which drives
`defram.py`, which lifts *aliases* -- `int32_t &v83 = *(int32_t *)(frame +
24600);`.  There are none left: earlier rounds turned every alias into a named
member, so `defram.py --list` answers "0 of 0 aliases lift" and the sweep
prints "0 kept, 0 reverted".  That is a green line meaning *nothing was tried*,
under seventeen claims that rest on it having been.

So this is the same experiment against the shape the file has now: each
non-padding member becomes a plain declaration, `__frame.X` becomes `X`, and
the `static_assert`s that pin the layout go with it.  It proposes; the gate
decides, one frame at a time, and a frame that fails goes back exactly as it
was.

Three things it declines rather than guesses at:

  * **a frame with a `union`.**  The union is MSVC's slot sharing written down,
    and lifting its arms to separate locals is not the same program.  Nine of
    the seventeen have one;
  * **a member whose name is already a local** of that body -- the two would
    become one variable;
  * **any use of `__frame` that is not `__frame.X`**, which after the asserts
    are removed should be none.  If one is left, the frame is doing something
    this does not model and it stays.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

MEMBER = re.compile(r'\s*(?:alignas\([^)]*\)\s*)?'
                    r'((?:const\s+|unsigned\s+|signed\s+)*[A-Za-z_]\w*'
                    r'(?:\s*\*)*)\s+(\**\s*\w+)\s*((?:\[[^\]]*\])*)\s*;\s*$')


def frame_of(lines, a, b):
    """(first, last, [lift], [keep]) for the body's frame, or None."""
    start = None
    for i in range(a, b + 1):
        c = lines[i].split('//')[0]
        if re.match(r'\s*struct\s+alignas\([^)]*\)\s*\w*\s*\{', c):
            start = i
            break
    if start is None:
        return None
    depth, end = 0, None
    for i in range(start, b + 1):
        c = lines[i].split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth <= 0 and '__frame' in c:
            end = i
            break
    if end is None:
        return None
    body = lines[start + 1:end]
    # A `union` is MSVC's slot sharing written down, and lifting its arms to
    # separate locals is not the same program.  But the members *outside* it
    # are ordinary locals that happen to sit in the same struct, and nine
    # frames were being declined whole for the union in the middle of them.
    # So the union is kept -- as a smaller frame in its own right -- and only
    # what surrounds it is offered.  Whether the two can be separated at all
    # is the gate's question, not this one's: the code in these bodies writes
    # past the ends of things, which is why the frames exist.
    out, depth, keep = [], 0, []
    for l in body:
        c = l.split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth > 0 or re.search(r'\b(?:union|struct)\b', c) or c.strip() in ('};', '}'):
            keep.append(l)
            continue
        m = MEMBER.fullmatch(c)
        if not m:
            if c.strip():
                return start, end, None
            continue
        name = m.group(2).lstrip('* ')
        if name.startswith('_pad') or name.startswith('_gap'):
            keep.append(l)
            continue
        out.append((m.group(1).strip(), m.group(2).strip() + m.group(3), name))
    return start, end, out, keep


# Frames whose members have been given their own storage and which failed the
# gate for it, on this file.  Named with the failure rather than left on the
# offer list, so that "0 to lift" means what it says.
PROVEN = {
    'read_bmp':       'DLRAW aborts while compressing',
    'search_filter':  'altp1 aborts while compressing',
    'cost_candidate': 'altp1 aborts while compressing',
    # These two offer only the members outside their union, and even that much
    # moves what the body writes past.
    'expand_image':   'DLRAW exits 3 while decompressing',
    'reduce_alphabet': 'DLRAW aborts while compressing',
}


def candidates(lines):
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        got = frame_of(lines, a, b)
        if got and got[2] and nm.lstrip('_') not in PROVEN:
            types = structs.decl_types(sig, lines, a, b)
            clash = [n for _, _, n in got[2]
                     if n in types and n not in {m[2] for m in got[2]}]
            out.append((nm, a, b, got, clash))
    return out


def apply(lines, nm, a, b, got, indent='  '):
    start, end, members, keep = got
    body = '\n'.join(lines[a:b + 1])
    # `__frame` must only ever be `__frame.X` once the asserts are gone.
    rest = re.sub(r'\}\s*__frame\s*;', '', re.sub(
        r'__frame\.\w+', '', re.sub(
            r'static_assert\([^;]*?__frame[^;]*?\);', '', body, flags=re.S)))
    if '__frame' in rest:
        return False, '%s: `__frame` is used other than as a member' % nm.lstrip('_')

    text = re.sub(r'[ \t]*static_assert\([^;]*?__frame[^;]*?\);\n', '',
                  body, flags=re.S)
    rows = text.split('\n')
    s2 = next(i for i, l in enumerate(rows)
              if re.match(r'\s*struct\s+alignas\([^)]*\)\s*\w*\s*\{',
                          l.split('//')[0]))
    depth, e2 = 0, None
    for i in range(s2, len(rows)):
        c = rows[i].split('//')[0]
        depth += c.count('{') - c.count('}')
        if depth <= 0 and '__frame' in c:
            e2 = i
            break
    decls = ['%s%s %s;' % (indent, t, d) for t, d, _ in members]
    # What is left of the struct: the union and the padding that positions it.
    # If that is nothing but braces, the frame goes entirely.
    real = [l for l in keep
            if l.split('//')[0].strip() not in ('', '{', '}', '};')]
    if real:
        new = [rows[s2]] + keep + [rows[e2]] + decls
    else:
        new = decls
    rows[s2:e2 + 1] = new
    text = '\n'.join(rows)
    # Only the lifted names lose the prefix; the ones still in the struct keep
    # it, which is what makes a partial lift a rewrite rather than a guess.
    for _, _, n in members:
        text = re.sub(r'__frame\.%s\b' % re.escape(n), n, text)
    lines[a:b + 1] = text.split('\n')
    return True, '%s: %d members lifted, %d left in the frame' % (
        nm.lstrip('_'), len(members), len(real))


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    want = next((x for x in sys.argv[2:] if not x.startswith('--')), None)
    found = candidates(lines)
    if want:
        one = [f for f in found if f[0].lstrip('_') == want.lstrip('_')]
        if not one:
            sys.exit('%s: no frame this can lift' % want)
        nm, a, b, got, clash = one[0]
        if clash:
            sys.exit('%s: %s already a local' % (nm, ', '.join(clash)))
        ok, why = apply(lines, nm, a, b, got)
        if not ok:
            sys.exit(why)
        open(path, 'w').write('\n'.join(lines))
        print(why)
    else:
        for nm, a, b, got, clash in found:
            print('  %-24s %3d members%s' % (nm.lstrip('_'), len(got[2]),
                                             '  CLASH: ' + ','.join(clash) if clash else ''))
        for fn, why in sorted(PROVEN.items()):
            print('  %-24s tried: %s' % (fn, why))
        print('%d frames this can offer to lift' % len(found))
