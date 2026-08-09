#!/usr/bin/env python3
"""Turn a struct that is really an array back into one.

    python3 tools/unstruct.py subs1.hpp --list
    python3 tools/unstruct.py subs1.hpp Obj120 [Obj122 ...]
    python3 tools/unstruct.py subs1.hpp --all

`structs.py` recovers a struct from the constant offsets a pointer is
dereferenced at, and for most objects that is right. For some it is not:

    struct Obj120 {
      uint8_t _pad0[2];
      uint8_t f2;
      uint8_t _pad2[7];
      uint8_t f10;
      uint8_t _pad4[7];
      uint8_t f18;
      ...

Five members, one type, evenly spaced, seven bytes of padding between each. That
is not a record with five fields at awkward offsets -- it is an array with a
stride of eight, and `f2 f10 f18 f26 f34` hides the one thing about it worth
seeing. `structs.py` half-knew: its own generated comment says "offsets the code
only reaches with a computed index are padding here".

The honest spelling is neither the struct nor the raw `*(uint8_t *)(p + 18)` it
replaced. It is `p[18]`, with the pointer typed. So this does not revert the
recovery, it finishes it: retype the declaration to `T *`, rewrite each `p->fN`
to `p[N/sizeof T]`, and delete the struct.

Four or more members, one type, offsets in exact arithmetic progression.  A
struct with two types in it is a record and is left alone; a packed run of one
type is not spared, because `f0 f4 f8 f12` names nothing a subscript does not.

There is a second shape this rule does not reach, and `Obj89` is the example:
four `char` members at 0, 253, 254 and 255.  Not a progression -- they are the
sentinels at the two ends of a 256-entry table, and everything between them is
written `((char *)a2)[4 * i + 2]`, through a cast the struct made necessary.
Ten such casts against four named offsets.  It was converted by hand rather than
by widening the rule here, because "the members are one type and the pointer is
also cast-and-subscripted" catches `Obj11` too -- 37 `__m128` members at
scattered offsets in a large object, where subscripts would read `p[17421]` and
the named offsets are the better spelling.  Which of those two a struct is, is
a judgement about the object, and this file does not make judgements.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                  # noqa: E402

WIDTH = {'uint8_t': 1, 'int8_t': 1, 'char': 1, 'uint16_t': 2, 'int16_t': 2,
         'uint32_t': 4, 'int32_t': 4, 'uint64_t': 8, 'int64_t': 8,
         '__m128': 16, '__m128i': 16, '__m128d': 16}
DEF = re.compile(r'(?:^//[^\n]*\n)*^struct (Obj\d+|ModelBlock) \{\n(.*?)\n\};\n'
                 r'(?:static_assert\([^;]*;\n)?', re.S | re.M)


def arrays(src):
    """name -> (element type, [offsets]) for every struct that is an array."""
    out = {}
    for m in DEF.finditer(src):
        name, body = m.group(1), m.group(2)
        mem = []
        for l in body.split('\n'):
            d = re.match(r'\s*(\S+)\s+(_pad\d+|_tail|f\d+)(\[\d+\])?;', l)
            if d and not d.group(2).startswith('_'):
                mem.append((d.group(1), int(d.group(2)[1:])))
        # Four or more, and no requirement that the members be spaced apart.
        # The first version asked for padding between them, on the idea that a
        # packed run of one type might be a record -- but every member here is
        # named `fN` for its offset, so there is nothing in the struct form that
        # the array form does not say, and `Obj24`'s twenty-two consecutive
        # uint32 were as much an array as `Obj120`'s five at a stride of eight.
        if len(mem) < 4:
            continue
        types = {t for t, _ in mem}
        offs = [o for _, o in mem]
        steps = {offs[i + 1] - offs[i] for i in range(len(offs) - 1)}
        if len(types) != 1 or len(steps) != 1:
            continue
        ty = types.pop()
        w = WIDTH.get(ty)
        if not w or any(o % w for o in offs):
            continue
        out[name] = (ty, offs, steps.pop())
    return out


def convert(src, name, ty, offs):
    """Rewrite the variables declared as `name *`, inside those bodies only.

    Two mistakes are worth keeping the shape of.  The first version matched
    `p->fN` for any `p`, on the reasoning that offset N belongs to this struct.
    It does not -- 73 other structs have an `f2` -- and the rewrite reached every
    one of them: 69 compile errors, which is the good outcome for that mistake.

    Reading the variable names off the declarations fixed that and left a
    quieter version of it.  Hex-Rays numbers locals per function, so `v69` is an
    `Obj122 *` in one body and something else in the next, and a file-wide
    rewrite of `v69->f18` corrupts the second while compiling perfectly if the
    other type happens to have an `f18` too.  One of the ten did exactly that.
    The scope is a function body, which is the scope the name has.
    """
    w = WIDTH[ty]
    lines = src.split('\n')
    out, total = list(lines), 0
    for a, b, fn, sig in structs.bodies(lines):
        text = '\n'.join(lines[a:b + 1])
        # the signature is not inside the range bodies() returns, and a
        # parameter is a declaration like any other: Obj24 lives only in
        # `__pixel_context(ModelBlock *_this, Obj24 *p_n15)`, so searching the
        # body alone found no variable to rewrite and deleted the struct out
        # from under 22 live accesses.
        who = set(re.findall(r'\b%s\s*\*+\s*&?\s*([A-Za-z_][A-Za-z0-9_]*)'
                             % name, sig + '\n' + text))
        if not who:
            continue
        alt = '|'.join(re.escape(v) for v in sorted(who, key=len, reverse=True))
        for off in offs:
            text, k = re.subn(r'(?<![\w.>])(%s)->f%d\b' % (alt, off),
                              lambda m, o=off: '%s[%d]' % (m.group(1), o // w),
                              text)
            total += k
        out[a:b + 1] = text.split('\n')
    src = '\n'.join(out)
    # declarations and casts: `Obj120 *v76` -> `uint8_t *v76`, `(Obj120 *)`
    src = re.sub(r'\b%s\s*\*' % name, '%s *' % ty, src)
    # and the definition itself
    src = DEF.sub(lambda m: '' if m.group(1) == name else m.group(0), src)
    return src, total


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    found = arrays(src)
    if '--list' in sys.argv or len(sys.argv) < 3:
        for n, (ty, offs, step) in sorted(found.items(),
                                          key=lambda kv: int(kv[0][3:] or 0)):
            print('%-12s %2d x %-9s stride %d   offsets %s'
                  % (n, len(offs), ty, step,
                     ' '.join(str(o) for o in offs[:6])))
        print('%d of the recovered structs are arrays' % len(found))
        return 0
    want = list(found) if '--all' in sys.argv else sys.argv[2:]
    done = 0
    for n in want:
        if n not in found:
            print('%s: not an array-shaped struct' % n)
            continue
        ty, offs, _ = found[n]
        src, k = convert(src, n, ty, offs)
        print('%s -> %s *, %d accesses rewritten' % (n, ty, k))
        done += 1
    open(path, 'w').write(src)
    print('%d converted' % done)
    return 0


if __name__ == '__main__':
    sys.exit(main())
