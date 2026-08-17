#!/usr/bin/env python3
"""Give a frame member the type its readers cast it to.

    python3 tools/unmemcast.py bmf.cpp --list
    python3 tools/unmemcast.py bmf.cpp read_bmp Size int32_t

REFACTORING4.md §5 item 2's second half.  Folding the aliases left casts where
round three had declared a member as storage and the body read a type:

    uint8_t Size[4];        ...   (*(int32_t *)((char *)frame.Size))
    uint8_t *v226;          ...   ((Obj97 *&)frame.v226)

Where every reader agrees on one type of the member's own width, the member can
*be* that type and the cast has nothing left to do.  Two things say no before
the gate does: a member whose offset is not aligned for the new type moves the
tail of the frame, and the `static_assert` on `sizeof(frame)` catches that; a
member whose readers disagree keeps its casts.

Retyping is not free even when the width matches -- §7's third hazard is that
signedness is part of a field's type, and its first is that pointer arithmetic
scales -- so this takes one member at a time and `unmemcast-sweep.sh` runs the
whole gate after each.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import collections                                              # noqa: E402
import merge                                                    # noqa: E402
import structs                                                  # noqa: E402
import unalias                                                  # noqa: E402

PTR = re.compile(r'\(\*\(([\w ]+\**)\s*\*\)(?:\(\(char \*\))?&?frame\.(\w+)\)?\)')
REF = re.compile(r'\(\(([\w ]+\**)\s*&\)frame\.(\w+)\)')


def readers(lines, a, b, close):
    """{member: Counter(type)} over both cast spellings."""
    c = collections.defaultdict(collections.Counter)
    for i in range(close + 1, b + 1):
        code = lines[i].split('//')[0]
        for rx in (PTR, REF):
            for m in rx.finditer(code):
                c[m.group(2)][m.group(1).strip()] += 1
    return c


def candidates(lines):
    for a, b, nm, _ in structs.bodies(lines):
        fr = unalias.frame_of(lines, a, b)
        if not fr:
            continue
        decl = {}
        off = 0
        for i in range(fr[0], fr[1]):
            m = unalias.MEMBER.match(lines[i])
            if not m:
                continue
            w = merge.width(m.group(2).strip())
            if w is None:
                break
            al = min(w, 4)
            off += (-off) % al
            decl[m.group(3)] = (i, m.group(2).strip(), int(m.group(5) or 1), off)
            off += w * int(m.group(5) or 1)
        for mem, tys in readers(lines, a, b, fr[1]).items():
            if len(tys) != 1 or mem not in decl:
                continue
            ty = next(iter(tys))
            i, mty, n, at = decl[mem]
            w, wnew = merge.width(mty), merge.width(ty)
            if w is None or wnew is None or w * n != wnew or at % min(wnew, 4):
                continue
            yield nm, mem, ty, sum(tys.values())


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    lines = open(path).read().split('\n')
    args = [x for x in sys.argv[2:] if not x.startswith('--')]

    if len(args) < 3:
        seen = 0
        for nm, mem, ty, k in candidates(lines):
            print('%-24s %-16s -> %-12s %d sites' % (nm.lstrip('_'), mem, ty, k))
            seen += 1
        if structs.no_frames(lines):
            print('not applicable: no `struct alignas(16)` frame in this file')
            return 0
        print('%d frame members are read as one wider type' % seen)
        return 0

    fn, mem, ty = args
    for a, b, nm, _ in structs.bodies(lines):
        if nm != fn:
            continue
        fr = unalias.frame_of(lines, a, b)
        for i in range(fr[0], fr[1]):
            m = unalias.MEMBER.match(lines[i])
            if m and m.group(3) == mem:
                lines[i] = '%s%s%s%s;' % (m.group(1), ty,
                                          '' if ty.endswith('*') else ' ', mem)
        n = 0
        for i in range(fr[1] + 1, b + 1):
            for rx in (PTR, REF):
                lines[i], k = rx.subn(
                    lambda x: ('frame.%s' % mem
                               if x.group(2) == mem and x.group(1).strip() == ty
                               else x.group(0)), lines[i])
                n += k
        print('%s %s is %s now, %d casts gone' % (fn.lstrip('_'), mem, ty, n))
        open(path, 'w').write('\n'.join(lines))
        return 0
    print('no such function')
    return 1


if __name__ == '__main__':
    sys.exit(main())
