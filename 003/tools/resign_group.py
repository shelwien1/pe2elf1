#!/usr/bin/env python3
"""Give a set of locals that must agree the same signedness.

    python3 tools/resign_group.py subs1.hpp
    python3 tools/resign_group.py subs1.hpp --all --only=K

`resign.py` flips one local at a time and stops where the flip only moves the
warning: a local converted on the way in *and* on the way out cannot be helped
alone, because whichever way it is declared, one end converts.

Those locals are not independent. `cum = ...; cum_hi = cum + *slot;
rc.encode(cum, cum_hi, tot)` is three names that have to agree, and Hex-Rays
declared them from three registers. Flipping any one of them trades; flipping
all three removes every conversion between them.

So: build the graph whose nodes are locals and whose edges are the signedness
conversions between two locals of one body, take its connected components, and
propose making each component agree.

Two locals are also joined when their names are the same stem with a different
trailing number. `alti2`..`alti7` in `alt_p1_model` are six registers holding
one quantity and *none* of them is assigned from any other, so there is no
conversion edge between them -- and each keeps converting against the ones
still declared the old way, so no single flip among them can ever pay. Six
together are worth six warnings. 167 locals in 59 groups, in a file where
flipping any of them alone had been measured not to pay.

Both directions are offered, as two candidates, and the driver measures which
pays. The first version took the component's majority signedness, which is not
a majority question: `alti0` and `alti1` had already been flipped to `uint32_t`
because that direction pays, so a headcount over `alti0`..`alti7` said
`int32_t` and proposed flipping the two right ones back.

Signedness only, never width. A component whose members are not all the same
width has no single signedness to agree on, and the majority vote proposes a
truncation instead: `uint8_t <- half (int32_t)` got as far as a green gate
before this check existed, which is a change the fifteen reference streams
happened not to exercise rather than one they approved.

Every member has to pass `resign.safe()`, the same test the single-local rule
uses: not a parameter, address never taken, never the left operand of `>>`,
`/` or `%`, and every ordering comparison it takes part in already protected.
One member failing disqualifies the group, because a group that only half
agrees is worse than one that does not agree at all.

Whether a group pays is still the compiler's answer and not this file's.
`tools/resign-drive.sh --group` applies one and rebuilds.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import buildlog                                                   # noqa: E402
import resign                                                     # noqa: E402

# Only the signedness of a type may change here, never its width.
WIDTH = {'int8_t': 8, 'uint8_t': 8, 'int16_t': 16, 'uint16_t': 16,
         'int32_t': 32, 'uint32_t': 32, 'int64_t': 64, 'uint64_t': 64}
import structs                                                    # noqa: E402
import unreload                                                   # noqa: E402


def components(lines, log='warn.log'):
    """[(body, {name: want}, edges)] for every set of locals that must agree."""
    bodies, fn = {}, {}
    for a, b, nm, sig in structs.bodies(lines):
        bodies[nm] = (a, b, sig)
        for i in range(a, b + 1):
            fn[i] = nm
    rows = buildlog.rows(log, sys.argv[1])

    types = {}
    edges = collections.defaultdict(set)
    for l in rows:
        m = resign.WARN.match(l)
        if not m:
            continue
        nm = fn.get(int(m.group(1)) - 1)
        if nm is None:
            continue
        a, b, _sig = bodies[nm]
        if nm not in types:
            types[nm] = unreload.types([x.split('//')[0] for x in lines[a:b + 1]])
        ty = types[nm]
        code = lines[int(m.group(1)) - 1].split('//')[0]
        d = resign.STORE.match(code)
        if not d or d.group(1) not in ty:
            continue
        dst = d.group(1)
        if ty[dst][1] or ty[dst][0] not in resign.FLIP:
            continue
        for w in re.findall(r'(?<![\w.])(?<!->)([A-Za-z_]\w*)\b',
                            code[code.index('=') + 1:]):
            if w in ty and not ty[w][1] and ty[w][0] in resign.FLIP \
                    and ty[w][0] != ty[dst][0]:
                edges[(nm, dst)].add((nm, w))
                edges[(nm, w)].add((nm, dst))

    # Siblings by name.  `alti2`..`alti7` are six registers holding one
    # quantity, and none of them is assigned from any other -- so there is no
    # conversion edge between them and each keeps converting against the ones
    # still declared the old way.  No single flip among them can ever pay, and
    # the six together are worth six warnings.
    #
    # A sibling is a local of the same body whose name is the same stem with a
    # different trailing number, and whose declared type is the same or its
    # flip.  The stem has to be at least two characters, so `n2` and `n5` are
    # not siblings of each other by accident.
    bystem = collections.defaultdict(list)
    for nm, ty in types.items():
        for v, t in ty.items():
            if t[1] or t[0] not in resign.FLIP:
                continue
            m = re.fullmatch(r'([A-Za-z_]\w*?[A-Za-z_])(\d+)', v)
            if m and len(m.group(1)) >= 2:
                bystem[(nm, m.group(1), WIDTH[t[0]])].append(v)
    for (nm, _stem, _w), names in bystem.items():
        if len(names) < 2:
            continue
        for a_name in names:
            for b_name in names:
                if a_name != b_name:
                    edges[(nm, a_name)].add((nm, b_name))

    seen, out = set(), []
    for node in edges:
        if node in seen:
            continue
        stack, comp = [node], set()
        while stack:
            x = stack.pop()
            if x in seen:
                continue
            seen.add(x)
            comp.add(x)
            stack += list(edges[x])
        if len(comp) < 2:
            continue
        nm = next(iter(comp))[0]
        ty = types[nm]
        # Signedness only.  A component whose members are not all the same
        # width has no single signedness to agree on, and the majority vote
        # would propose a *truncation* -- `uint8_t <- half (int32_t)` got as
        # far as a green gate before this check existed, which is a change the
        # fifteen reference streams happened not to exercise rather than one
        # they approved.
        if len({WIDTH[ty[v][0]] for _n, v in comp}) != 1:
            continue
        votes = collections.Counter(ty[v][0] for _n, v in comp)
        top = votes.most_common()
        # Both directions, as two candidates.  The first version took the
        # component's majority signedness and it is not a majority question:
        # `alti0` and `alti1` had already been flipped to `uint32_t` because
        # that direction pays, so a headcount of `alti0`..`alti7` said
        # `int32_t` and proposed flipping the two right ones back.  Which way
        # a component should go is the compiler's answer, like everything else
        # here, so the rule offers both and the driver measures.
        a, b, sig = bodies[nm]
        code = [x.split('//')[0] for x in lines[a:b + 1]]
        for want in votes:
            flips = {v: want for _n, v in comp if ty[v][0] != want}
            if not flips:
                continue
            if not all(resign.safe(v, ty[v][0], want, code, sig) for v in flips):
                continue
            out.append((nm, a, b, flips, {v: ty[v][0] for v in flips}))
    return sorted(out, key=lambda g: (g[0], sorted(g[3])))


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    resign.SRC[0] = path
    lines = open(path).read().split('\n')
    found = components(lines)
    only = next((a for a in sys.argv if a.startswith('--only=')), None)
    if only:
        k = int(only.split('=')[1])
        found = found[k:k + 1]

    if '--all' not in sys.argv:
        for nm, _a, _b, flips, cur in found:
            print('%-24s %-9s <- %s' % (nm.lstrip('_'),
                                        next(iter(flips.values())),
                                        ', '.join('%s (%s)' % (v, cur[v])
                                                  for v in sorted(flips))[:60]))
        print('%d groups of locals that have to agree, %d flips'
              % (len(found), sum(len(f[3]) for f in found)))
        return 0

    done = 0
    for nm, a, b, flips, cur in sorted(found, key=lambda g: -g[1]):
        for name, want in sorted(flips.items()):
            got = resign.decl_line(lines, a, b + done, name, cur[name])
            if got is None:
                continue
            resign.retype(lines, got, name, cur[name], want)
            done += 1
    open(path, 'w').write(resign.tidy('\n'.join(lines)))
    print('%d locals retyped in %d groups' % (done, len(found)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
