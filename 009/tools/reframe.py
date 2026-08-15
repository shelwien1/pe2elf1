#!/usr/bin/env python3
"""Give a plain-split frame back its layout, without giving back its casts.

    python3 tools/reframe.py subs1.hpp pre2.hpp --list
    python3 tools/reframe.py subs1.hpp pre2.hpp write_bmp

Phase 2 turned each `__hexrays_frame` buffer into ordinary locals.  Sixteen
functions could not take that and got a struct with explicit padding instead;
eight kept the plain split because the gate accepted it.

One of the eight was `alt_model_p1_decode`, and the gate accepted it because
nothing in the corpus ran that function.  Its frame was 116 bytes with aliases
naming 84, and the code writes into the 32 that are left; once each local has
its own storage those writes land wherever the compiler put things.  It
segfaulted the first time an image reached it, years of commits later.

Every remaining plain-split frame has the same shape -- 32 or so bytes past the
last alias -- and two of them have branches the corpus still does not reach.
Rather than argue about which are safe, this converts them: the struct is
layout-preserving by construction, costs nothing at run time, and turns "the
gate has not caught it" into "the layout cannot move".

`pre2.hpp` is the file as it stood before Phase 2 -- `git show <rev>:003/subs1.hpp`
-- because that is the only place the offsets still exist.  The types come from
the *current* file, so the retyping and struct recovery done since are kept.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

ALIAS = re.compile(r'^\s*(?P<ty>[A-Za-z_][A-Za-z0-9_ ]*?\**)\s*'
                   r'\(?&(?P<nm>[A-Za-z_][A-Za-z0-9_]*)\)?'
                   r'(?:\[(?P<cnt>\d+)\])?\s*=\s*'
                   r'\*\([^)]*\)\(__hexrays_frame \+ (?P<off>\d+)\)')
FRAME = re.compile(r'__hexrays_frame\[(\d+)\]')


def layout(pre, fn):
    """(size, [(offset, name, count)]) for one function's frame, in the old file."""
    for a, b, n, sig in structs.bodies(pre):
        if n != fn or 'static inline' in sig:
            continue
        size, out = None, []
        for l in pre[a:b + 1]:
            m = FRAME.search(l)
            if m:
                size = int(m.group(1))
            m = ALIAS.match(l)
            if m:
                out.append((int(m.group('off')), m.group('nm'),
                            int(m.group('cnt')) if m.group('cnt') else 0))
        if size:
            return size, out
    return None, []


def current_types(now, fn, names):
    """name -> the type it has today, from the plain declarations Phase 2 left."""
    types, order = {}, []
    for a, b, n, sig in structs.bodies(now):
        if n != fn or 'static inline' in sig:
            continue
        for i in range(a + 1, b + 1):
            l = now[i]
            l = l.split('//')[0]      # earlier phases left explanatory
            #                             comments on the declarations they
            #                             retyped, and a pattern anchored at
            #                             the end of the line does not see past
            #                             one.  That alone kept model_plane's
            #                             frame from converting.
            m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_ ]*?\**)\s*'
                         r'([A-Za-z_][A-Za-z0-9_]*)\s*(\[\d+\])?\s*;\s*$', l)
            if m and m.group(2) in names:
                types[m.group(2)] = (m.group(1).strip(), m.group(3) or '', i)
                order.append(i)
                continue
            # Two aliases share a slot.  Phase 2 kept both, either as a plain
            # reference or -- where the types differ -- through the owner's
            # address, and both forms have to be recognised or the member they
            # share looks undeclared.
            m = re.match(r'^\s*([A-Za-z_][A-Za-z0-9_ ]*?\**)\s*&\s*'
                         r'([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+);\s*$', l)
            if m and m.group(2) in names:
                # the owner is the last identifier in the initialiser, whatever
                # casts are wrapped around it -- there are at least four shapes
                # and enumerating them is a losing game
                ids = re.findall(r'[A-Za-z_][A-Za-z0-9_]*', m.group(3))
                if not ids:
                    continue
                types[m.group(2)] = ('&' + ids[-1], m.group(1).strip(), i)
                order.append(i)
        break
    return types, order


def build(size, slots, types, used):
    """The struct text and the reference lines."""
    members, refs, at = [], [], 0
    for off, nm, cnt in sorted(slots):
        if nm not in types:
            # a slot later work stopped using.  It still occupies its bytes --
            # that is the whole point of preserving the layout -- but nothing
            # names it, so it goes in as padding rather than as a member.
            if nm in used:
                return None, None, 'no declaration for %s' % nm
            continue
        ty, arr, _ = types[nm]
        if ty.startswith('&'):                    # shares another slot
            owner, own_ty = ty[1:], arr
            oty = types.get(owner, ('int32_t', '', 0))[0]
            if own_ty and own_ty != oty:
                refs.append('  %s &%s = *(%s *)((char *)&frame.%s);'
                            % (own_ty, nm, own_ty, owner))
            else:
                refs.append('  %s &%s = frame.%s;' % (oty, nm, owner))
            continue
        w = structs.width(ty)
        if w is None:
            return None, None, 'cannot size %s' % ty
        n = int(arr.strip('[]')) if arr else 1
        if off < at:
            return None, None, 'overlap at +%d' % off
        if off > at:
            members.append('      uint8_t   _pad%d[%d];' % (len(members), off - at))
        members.append('      %-9s %s%s;' % (ty, nm, arr))
        refs.append('  %s %s%s = frame.%s;'
                    % (ty, '(&%s)%s' % (nm, arr) if arr else '&' + nm,
                       '', nm))
        at = off + w * n
    named = at
    if size > at:
        members.append('      uint8_t   _pad%d[%d];' % (len(members), size - at))
        at = size
    pad = (16 - at % 16) % 16                     # alignas(16) rounds up
    if pad:
        members.append('      uint8_t   _tail[%d];   // alignas(16) rounds %d up'
                       % (pad, at))
        at += pad
    body = ('  struct alignas(16) {   // %d bytes, the frame Hex-Rays could not name\n'
            '%s\n  } frame;\n'
            '  static_assert(sizeof(void *) != 4 || sizeof(frame) == %d,\n'
            '                "frame layout moved");\n'
            '  static_assert(sizeof(void *) != 4\n'
            '                || __builtin_offsetof(__typeof__(frame), _pad%d) == %d,\n'
            '                "the named part of the frame moved");\n'
            % (size, '\n'.join(members), at,
               len(members) - (2 if pad and size > named else 1), named))
    return body, refs, None


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip().split('\n\n')[1].strip())
    now = open(sys.argv[1]).read().split('\n')
    pre = open(sys.argv[2]).read().split('\n')
    amap = {}
    try:
        for l in open('tools/addrmap.txt'):
            m = re.match(r'^(\w+)\s+0x00([0-9A-F]{6})', l)
            if m:
                amap['__sub_' + m.group(2)] = '__' + m.group(1)
    except IOError:
        pass

    framed = {}
    for a, b, n, sig in structs.bodies(pre):
        if 'static inline' in sig:
            continue
        for l in pre[a:b + 1]:
            m = FRAME.search(l)
            if m:
                framed[amap.get(n, n)] = (n, int(m.group(1)))
                break

    if '--list' in sys.argv:
        have = {n for a, b, n, sig in structs.bodies(now)
                if 'static inline' not in sig
                and any('} frame;' in l for l in now[a:b + 1])}
        for nm in sorted(framed):
            old, size = framed[nm]
            print('%-26s %6d bytes   %s' % (nm.lstrip('_'), size,
                                            'struct' if nm in have else 'PLAIN SPLIT'))
        return

    fn = sys.argv[3]
    old = framed.get(fn, (fn, None))[0]
    size, slots = layout(pre, old)
    if not size:
        sys.exit('%s: no frame in the old file' % fn)
    types, order = current_types(now, fn, {nm for _, nm, _ in slots})
    span = next(((a, b) for a, b, n, sig in structs.bodies(now)
                 if n == fn and 'static inline' not in sig), None)
    text = '\n'.join(now[span[0]:span[1] + 1]) if span else ''
    used = {nm for _, nm, _ in slots
            if re.search(r'(?<![\w.>])%s\b' % re.escape(nm), text)}
    body, refs, err = build(size, slots, types, used)
    if err:
        sys.exit('%s: %s' % (fn, err))
    if not order:
        sys.exit('%s: found no declarations to replace' % fn)
    # The declarations need not sit together: later phases moved some of them,
    # so each goes where it is and the struct goes where the first one was.
    out = list(now)
    for i in sorted(order, reverse=True):
        del out[i]
    at = min(order)
    out[at:at] = body.split('\n')[:-1] + refs
    open(sys.argv[1], 'w').write('\n'.join(out))
    print('%s: %d bytes, %d members restored to one frame'
          % (fn, size, len(slots)))


if __name__ == '__main__':
    main()
