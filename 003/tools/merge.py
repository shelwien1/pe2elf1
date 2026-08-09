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
from structs import bodies                                        # noqa: E402

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
        # `uint8_t  *` and `uint8_t *` are one type; comparing the source
        # spelling made `Obj31`'s scalar look unlike `Obj8`'s array of it.
        ty = ' '.join(f.group(1).split()).replace(' *', '*').replace('*', ' *', 1)
        nm, cnt, com = f.group(2), f.group(3), f.group(4)
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
    merged, conflicts, absorbed, alt = [], [], {}, {}
    for off in sorted(byoff):
        cands = byoff[off]
        best = max(cands, key=lambda c: (c[1]['end'], c[1]['count']))
        for who, f in cands:
            # Same offset, same width, different type is the dangerous case and
            # the easiest to miss: `Obj10::f6059436` is `uint16_t *` where
            # `ModelBlock::f6059436` is `uint8_t *`, and round two moved three
            # streams by making one match the other.  Width is not the test.
            if f['ty'] == best[1]['ty'] and f['count'] == best[1]['count']:
                continue
            if f['ty'] == best[1]['ty'] and f['end'] <= best[1]['end']:
                # A prefix of the longer array, not a rival.  A *scalar* at the
                # array's own offset is its element 0 -- `Obj31::f278736` is one
                # `uint8_t *` where `Obj8::f278736` is ten of them.
                if f['count'] == 1 and best[1]['count'] > 1:
                    absorbed[f['name']] = '%s[0]' % best[1]['name']
                continue
            if f['count'] > 1 or best[1]['count'] > 1:
                alt.setdefault(off, []).append(f)   # two granularities: union
                continue
                continue
            # Two scalars of the same width at one offset is the dangerous
            # case: `Obj10::f6059436` is `uint16_t *` where
            # `ModelBlock::f6059436` is `uint8_t *`, and round two moved three
            # streams by making one match the other.
            conflicts.append((off, who, f['ty'], best[0], best[1]['ty']))
        merged.append(best[1])
    # A scalar covered by an earlier array is one of two things.  If the array
    # is of the scalar's own type and the offset lands on an element, the
    # scalar *is* that element and becomes a subscript -- ten row cursors read
    # as an array say something ten names do not.  If it is not -- `Obj11`
    # recovered +278528 as `__m128[21]` because `alt_p2_filter` walks it with a
    # loop variable, while four other recoveries declare fifteen individual
    # fields inside those same bytes -- then the region is genuinely both, and
    # the merged struct says so with a union instead of picking one.
    keep, covered = [], {}
    for f in merged:
        cover = next((k for k in keep
                      if k['count'] > 1 and k['off'] < f['off'] < k['end']), None)
        if cover is None:
            keep.append(f)
            continue
        step = width(cover['ty'])
        if f['ty'] == cover['ty'] and not (f['off'] - cover['off']) % step \
                and f['end'] <= cover['end']:
            absorbed[f['name']] = '%s[%d]' % (cover['name'],
                                              (f['off'] - cover['off']) // step)
        else:
            covered.setdefault(cover['name'], []).append(f)
    # Every extra view belongs to whichever kept member spans its bytes -- keyed
    # by the *view's* offset, not the cover's, which is how three of them went
    # missing the first time.
    for off, extras in alt.items():
        host = next((k for k in keep if k['off'] <= off < k['end']), None)
        if host is None:
            conflicts.append((off, 'a second view', extras[0]['ty'], '', 'no host'))
            continue
        covered.setdefault(host['name'], []).extend(extras)
    for f in keep:
        u, seen_k = [], set()
        for g in sorted(covered.get(f['name'], []), key=lambda g: g['off']):
            k = (g['off'], g['ty'], g['count'], g['name'])
            if k in seen_k:          # two recoveries that agree are one view
                continue
            seen_k.add(k)
            u.append(g)
        f['union'] = u
    # Anonymous structs inside a union share the union's scope, so two views
    # that both call one offset `fNN` cannot both be emitted -- and dropping one
    # silently is how `Obj19::f278760` (a `char *`) became `Obj69`'s
    # `uint32_t[24]` and segfaulted three images.  Report it; the fix is to
    # rename the losing view's field at its own use sites first.
    for f in keep:
        seen_n = {f['name']}
        for g in f['union']:
            if g['name'] in seen_n:
                conflicts.append((g['off'], 'a second view', g['ty'],
                                  'the union', 'the name %s twice' % g['name']))
            seen_n.add(g['name'])
    return keep, conflicts, absorbed


def render(name, members):
    out = ['struct %s {' % name]
    off = 0
    for i, f in enumerate(members):
        if f['off'] > off:
            out.append('  uint8_t _pad%d[%d];' % (i, f['off'] - off))
        arr = '[%d]' % f['count'] if f['count'] > 1 else ''
        sp = '' if f['ty'].endswith('*') else ' '
        decl = '%s%s%s%s;%s' % (f['ty'], sp, f['name'], arr,
                                ('   ' + f['com']) if f['com'] else '')
        if not f.get('union'):
            out.append('  ' + decl)
        else:
            # The covered fields need not be disjoint from each other either --
            # `f278736[10]` spans the bytes `f278756` and `f278760[24]` name,
            # and `f278760[24]` spans `f278764` and `f278768`.  Lay them out in
            # as many overlapping views as it takes, greedily, widest first.
            layers = []
            for g in sorted(f['union'], key=lambda g: (g['off'], -g['end'])):
                for L in layers:
                    if L[-1]['end'] <= g['off']:
                        L.append(g)
                        break
                else:
                    layers.append([g])
            out.append('  union {   // the same bytes, %d recoveries'
                       % (len(layers) + 1))
            out.append('    ' + decl)
            for k, L in enumerate(layers):
                out.append('    struct {')
                at = f['off']
                for j, g in enumerate(L):
                    if g['off'] > at:
                        out.append('      uint8_t _u%d_%d_%d[%d];'
                                   % (i, k, j, g['off'] - at))
                    ga = '[%d]' % g['count'] if g['count'] > 1 else ''
                    gs = '' if g['ty'].endswith('*') else ' '
                    out.append('      %s%s%s%s;%s'
                               % (g['ty'], gs, g['name'], ga,
                                  ('   ' + g['com']) if g['com'] else ''))
                    at = g['end']
                out.append('    };')
            out.append('  };')
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

    # Scoped, not global: `f0`, `f4` and `f8` are members of half the structs
    # in the file, and renaming them everywhere redeclares `ModelBlock::f0` as
    # an array.  Only rewrite `v->field` where `v` is a variable of one of the
    # types being merged.
    if absorbed:
        lines = src.split('\n')
        for a, b, fn, fsig in bodies(lines):
            # the signature too: `_this` is a parameter, not a local, and
            # scanning only the body misses every function that takes one
            body = fsig + '\n' + '\n'.join(lines[a:b + 1])
            vs = set()
            for t in names:
                vs |= set(re.findall(r'\b%s\s*\*&?\s*(\w+)' % t, body))
                vs |= set(re.findall(r'\(%s \*\)\s*\(?(\w+)' % t, body))
            for i in range(a, b + 1):
                for v in vs:
                    for nm, to in absorbed.items():
                        lines[i] = re.sub(
                            r'\b%s->%s\b(?!\s*\[)' % (re.escape(v), nm),
                            '%s->%s' % (v, to), lines[i])
        src = '\n'.join(lines)
    for n in names[1:]:
        src = re.sub(r'\b%s\b' % n, target, src)
    open(path, 'w').write(src)
    print('%s absorbed %s: %d members' % (target, ', '.join(names[1:]), len(merged)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
