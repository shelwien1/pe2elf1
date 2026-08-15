#!/usr/bin/env python3
"""Collapse a run of consecutive same-type members into the array it is.

    python3 tools/arrayify.py subs1.hpp --list
    python3 tools/arrayify.py subs1.hpp --apply

`Obj0` declares fifty-one `uint32_t` at +12, +16, +20 … +212, one member each.
Nothing in that says they are contiguous, which is the only thing about them
anyone needs to know; `structs.py` writes a member per observed offset and has
no way to tell a run from a coincidence.

The run *is* the evidence: fifty-one adjacent slots of one type, no gap, is an
array in any language that has them. So the run becomes

    uint32_t f12[51];        // +12 .. +212

and `->f16` becomes `->f12[1]`. The name keeps the offset, because the offset is
what is known — this makes no claim about meaning, and is not the same operation
as naming `Obj1`'s two runs `row` and `cur`, which took reading the rotation in
`alt_model_p1_decode` to justify.

`tools/unstruct.py` handles the case where the *whole* struct is an array and
the struct should go. This is the other case: a run inside a struct that has
other members too, where the struct stays and the run stops being fifty-one
names.

Four is the minimum. Three consecutive same-type fields is as likely to be a
record — an x, y, z — as an array, and the point of these tools is to stop short
of the places where that judgement is needed.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

WIDTH = {'uint8_t': 1, 'int8_t': 1, 'char': 1, 'uint16_t': 2, 'int16_t': 2,
         'uint32_t': 4, 'int32_t': 4, 'uint64_t': 8, 'int64_t': 8,
         'float': 4, 'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16}
LEAST = 4


def runs_in(body):
    """[(first offset, count, type, [offsets])] for each run of >= LEAST."""
    mem = [(d.group(1), int(d.group(2)[1:]))
           for l in body.split('\n')
           for d in [re.match(r'\s*(\S+)\s+(f\d+);', l)] if d]
    out, i = [], 0
    while i < len(mem):
        ty, w, j = mem[i][0], WIDTH.get(mem[i][0]), i
        if w:
            while (j + 1 < len(mem) and mem[j + 1][0] == ty
                   and mem[j + 1][1] == mem[j][1] + w):
                j += 1
        if j - i + 1 >= LEAST:
            out.append((mem[i][1], j - i + 1, ty, [o for _, o in mem[i:j + 1]]))
        i = j + 1
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith('--'):
        sys.exit('usage: %s needs the file to read; `bmf.cpp` for a tool that\n       splices the unit, one .inc for a tool that does not'
                         % __file__.rsplit('/', 1)[-1])
    path = sys.argv[1]
    src = open(path).read()
    found = []
    for m in re.finditer(r'struct (Obj\d+) \{\n(.*?)\n\};', src, re.S):
        for r in runs_in(m.group(2)):
            found.append((m.group(1),) + r)
    if not found:
        print('no same-type runs of %d or more' % LEAST)
        return 0
    if '--apply' not in sys.argv:
        for o, off, n, ty, offs in found:
            print('%-8s +%-8d %2d x %-9s -> %s f%d[%d]'
                  % (o, off, n, ty, ty, off, n))
        print('%d runs, %d members' % (len(found), sum(f[2] for f in found)))
        return 0

    for o, off, n, ty, offs in found:
        # the declaration: one array where the run was
        decl = re.compile(r'(struct %s \{\n)(.*?)(\n\};)' % o, re.S)
        def one(m, off=off, n=n, ty=ty, offs=offs):
            body = m.group(2)
            keep, dropped = [], False
            for l in body.split('\n'):
                d = re.match(r'\s*(\S+)\s+f(\d+);', l)
                if d and int(d.group(2)) in offs:
                    if not dropped:
                        keep.append('  %-9s f%d[%d];   // +%d .. +%d'
                                    % (ty, off, n, off, offs[-1]))
                        dropped = True
                    continue
                keep.append(l)
            return m.group(1) + '\n'.join(keep) + m.group(3)
        src = decl.sub(one, src, count=1)
        # The generated static_assert names the struct's last member, and if
        # that member has just gone inside an array the assert stops compiling.
        # It is the guard that says the layout did not move, so it has to keep
        # naming the same byte.
        for k, o2 in enumerate(offs):
            src = src.replace('__builtin_offsetof(%s, f%d)' % (o, o2),
                              '__builtin_offsetof(%s, f%d[%d])' % (o, off, k))

    # the accesses, per body and per variable, because a name means one thing
    # inside one function and something else in the next
    lines = src.split('\n')
    out, total = list(lines), 0
    for a, b, nm, sig in structs.bodies(lines):
        if 'static inline' in sig:
            continue
        t = '\n'.join(lines[a:b + 1])
        for o, off, n, ty, offs in found:
            who = set(re.findall(r'\b%s\s*\*+\s*&?\s*(\w+)' % o, sig + '\n' + t))
            if not who:
                continue
            alt = '|'.join(re.escape(v) for v in sorted(who, key=len,
                                                        reverse=True))
            for k, o2 in enumerate(offs):
                t, c = re.subn(r'(?<![\w.>])(%s)->f%d\b' % (alt, o2),
                               r'\1->f%d[%d]' % (off, k), t)
                total += c
        out[a:b + 1] = t.split('\n')
    open(path, 'w').write('\n'.join(out))
    print('%d runs collapsed, %d members, %d accesses rewritten'
          % (len(found), sum(f[2] for f in found), total))
    return 0


if __name__ == '__main__':
    sys.exit(main())
