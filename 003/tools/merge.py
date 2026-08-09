#!/usr/bin/env python3
"""Make several recoveries of one object into one struct.

    python3 tools/merge.py subs1.hpp --plan Obj11 Obj8 Obj19 Obj31 Obj69
    python3 tools/merge.py subs1.hpp --apply Obj11 Obj8 Obj19 Obj31 Obj69

`structs.py` recovers a struct per *access pattern*, so one object arrives as
several: REFACTORING3.md §2.2 shows `alt_model_p2_encode` storing an array
element as `Obj11 *` and reading it back as `Obj31 *`, `Obj19 *` and `Obj8 *`
in sixteen lines, then handing it to callees typed `Obj11 *` and `Obj69 *`.
Five names, one pointer.

This unions them into the first one named and rewrites every use of the rest.
Members are keyed by offset, which the `fNN` names carry, and `--plan` prints
what agrees and what does not before anything is written.

**Where an array in one recovery covers scalars in another, the array wins and
the scalars become subscripts** -- `Obj8::f278736[10]` spans the bytes `Obj31`
calls `f278756`, `f278760`, `f278764` and `f278768`, so those four become
`f278736[5]`, `[6]`, `[7]`, `[8]` at their use sites.  That is `arrayify.py`'s
operation applied across two declarations instead of within one, and it is the
whole reason merging is worth doing: ten row cursors read as an array say
something that ten names do not.

A conflict the rule cannot settle -- two different scalar types at one offset,
which is what `Obj10` and `ModelBlock` disagree about at +1078684 -- is
reported and refused.  Round two typed one of those by matching the other and
moved three streams.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])

WIDTH = {'char': 1, 'int8_t': 1, 'uint8_t': 1, 'int16_t': 2, 'uint16_t': 2,
         'int32_t': 4, 'uint32_t': 4, 'int64_t': 8, 'uint64_t': 8,
         'float': 4, 'double': 8, '__m128': 16, '__m128i': 16, '__m128d': 16,
         'void': 1}
MEMBER = re.compile(r'^\s*([A-Za-z_][\w ]*?\s*\**)\s*(\w+)\s*(?:\[(\d+)\])?\s*;'
                    r'\s*(//.*)?$')


def width(ty):
    ty = ty.strip()
    return 4 if ty.endswith('*') else WIDTH.get(ty)


def parse(src, name):
    """[(offset, type, member, count, comment)] plus the struct's line span."""
    m = re.search(r'^struct %s \{\n(.*?)^\};\n' % re.escape(name), src,
                  re.S | re.M)
    if not m:
        return None, None
    out, off = [], 0
    for line in m.group(1).split('\n'):
        f = MEMBER.match(line)
        if not f:
            continue
        ty, nm, cnt, com = f.group(1).strip(), f.group(2), f.group(3), f.group(4)
        w = width(ty)
        if w is None:
            return None, None
        n = int(cnt) if cnt else 1
        if not nm.startswith('_pad'):
            at = int(nm[1:]) if re.fullmatch(r'f\d+', nm) else off
            out.append(dict(off=at, ty=ty, name=nm, count=n, com=com or '',
                            end=at + w * n))
            off = at + w * n
        else:
            off += w * n
    return out, m.span()


def plan(structs):
    """Union the members by offset; return (merged, conflicts, absorbed)."""
    byoff = {}
    for who, members in structs:
        for f in members:
            byoff.setdefault(f['off'], []).append((who, f))
    merged, conflicts, absorbed = [], [], {}
    for off in sorted(byoff):
        cands = byoff[off]
        best = max(cands, key=lambda c: (c[1]['end'], c[1]['count']))
        for who, f in cands:
            # Same offset, same width, different type is the dangerous case and
            # the easiest to miss: `Obj10::f6059436` is `uint16_t *` where
            # `ModelBlock::f6059436` is `uint8_t *`, and round two moved three
            # streams by making one match the other.  Width is not the test.
            if f['ty'] != best[1]['ty'] or f['count'] != best[1]['count']:
                if f['end'] <= best[1]['end'] and f['count'] > 1 == best[1]['count']:
                    continue
                conflicts.append((off, who, '%s%s' % (f['ty'], '[%d]' % f['count']
                                                      if f['count'] > 1 else ''),
                                  best[0], '%s%s' % (best[1]['ty'],
                                                     '[%d]' % best[1]['count']
                                                     if best[1]['count'] > 1 else '')))
        merged.append(best[1])
    # a scalar covered by an earlier array becomes a subscript of it
    keep = []
    for f in merged:
        cover = next((k for k in keep
                      if k['count'] > 1 and k['off'] < f['off'] < k['end']), None)
        if cover is None:
            keep.append(f)
            continue
        step = width(cover['ty'])
        if (f['off'] - cover['off']) % step or f['end'] > cover['end']:
            conflicts.append((f['off'], '', f['ty'], '', 'inside %s' % cover['name']))
            continue
        absorbed[f['name']] = '%s[%d]' % (cover['name'],
                                          (f['off'] - cover['off']) // step)
    return keep, conflicts, absorbed


def render(name, members):
    out = ['struct %s {' % name]
    off = 0
    for i, f in enumerate(members):
        if f['off'] > off:
            out.append('  uint8_t _pad%d[%d];' % (i, f['off'] - off))
        arr = '[%d]' % f['count'] if f['count'] > 1 else ''
        sp = '' if f['ty'].endswith('*') else ' '
        out.append('  %s%s%s%s;%s' % (f['ty'], sp, f['name'], arr,
                                      ('   ' + f['com']) if f['com'] else ''))
        off = f['end']
    out.append('};')
    last = members[-1]
    out.append('static_assert(sizeof(void *) != 4')
    out.append('              || __builtin_offsetof(%s, %s) == %d,'
               % (name, last['name'], last['off']))
    out.append('              "%s: the layout moved");' % name)
    return '\n'.join(out)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    names = [x for x in sys.argv[2:] if not x.startswith('--')]
    if len(names) < 2:
        print(__doc__.split('\n\n')[1])
        return 2
    src = open(path).read()

    structs, spans = [], {}
    for n in names:
        members, span = parse(src, n)
        if members is None:
            print('cannot parse struct %s' % n)
            return 1
        structs.append((n, members))
        spans[n] = span
    target = names[0]
    merged, conflicts, absorbed = plan(structs)

    if '--apply' not in sys.argv:
        for n, members in structs:
            print('%-12s %3d members, +%d .. +%d'
                  % (n, len(members), members[0]['off'], members[-1]['end']))
        print('\nmerged: %d members, +%d .. +%d'
              % (len(merged), merged[0]['off'], merged[-1]['end']))
        for nm, to in sorted(absorbed.items()):
            print('  %-12s -> %s' % (nm, to))
        for off, a, ta, bn, tb in conflicts:
            print('  CONFLICT +%-8d %s says %s, %s says %s' % (off, a, ta, bn, tb))
        print('\n%d conflicts' % len(conflicts))
        return 0

    if conflicts:
        print('refusing: %d conflicts, resolve them first' % len(conflicts))
        return 1

    # drop the absorbed structs, back to front, then rewrite the target
    for n in sorted(names[1:], key=lambda n: -spans[n][0]):
        a, b = spans[n]
        end = src.find('\n\n', src.find('"%s: the layout moved");' % n, b) + 1) \
            if ('"%s: the layout moved");' % n) in src[b:b + 400] else b
        src = src[:a] + src[max(end, b):]
    a, b = parse(src, target)[1]
    tail = src.find('"%s: the layout moved");' % target, b)
    end = src.find('\n', tail) + 1 if tail > 0 else b
    src = src[:a] + render(target, merged) + '\n' + src[end:]

    for nm, to in absorbed.items():
        src = re.sub(r'\b%s\b(?!\s*\[)' % nm, to, src)
    for n in names[1:]:
        src = re.sub(r'\b%s\b' % n, target, src)
    open(path, 'w').write(src)
    print('%s absorbed %s: %d members' % (target, ', '.join(names[1:]), len(merged)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
