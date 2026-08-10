#!/usr/bin/env python3
"""Measure the things REFACTORING3.md is about, so its numbers can be checked.

    python3 tools/shape.py                  # the §1 table
    python3 tools/shape.py --this           # raw offsets off `_this`, by function
    python3 tools/shape.py --frames         # one line per frame
    python3 tools/shape.py --bss            # the bmf_bss globals and their groups
    python3 tools/shape.py --overlap Obj8 Obj11 Obj19 Obj31 Obj69

`BMF_STRICT=1 ./build.sh` is the scoreboard for round two's goal.  This is the
scoreboard for round three's: a raw-offset dereference is a place where the
layout of an object is written as arithmetic instead of declared, and the count
is what Phase A is trying to move.

It is a counter, not a target -- `exclusion_mask[symbol]` is a byte buffer being
indexed and will never become a member.  See REFACTORING3.md §6.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

SRC = 'subs1.hpp'

# A type name with any number of stars: `char *`, `uint32_t **`, `Obj11 *`.
T = r'[A-Za-z_]\w*\s*\**'

# The three shapes a raw offset arrives in.  P1 is `(T *)(base + n)`, with an
# optional byte-cast on the base; P2 is `((T *)base + n)`, where the `+ n` is
# scaled; P3 is the reversed form Hex-Rays emits when the index came first,
# `*(uint8_t *)(idx + (char *)base + 984)`.
P1 = re.compile(r'\(\s*%s\s*\*\s*\)\s*\(\s*(?:\(\s*%s\s*\*\s*\)\s*)?'
                r'([A-Za-z_]\w*)\s*\+' % (T, T))
P2 = re.compile(r'\(\s*\(\s*%s\s*\*\s*\)\s*([A-Za-z_]\w*)\s*\+' % T)
P3 = re.compile(r'\+\s*\(\s*%s\s*\*\s*\)\s*([A-Za-z_]\w*)\b' % T)

# The tag came with REFACTORING4.md §5 item 1; the byte count is what counts.
FRAME = re.compile(r'struct alignas\(16\) \w* ?\{   // (\d+) bytes')
# An alias binds one name to one frame member.  Two spellings: `T &v = …` for a
# scalar and `T (&v)[N] = …` for an array member -- missing the second is how
# this file's first alias count came out 38 short.
ALIAS = re.compile(r'(?:&\s*|\(&)([A-Za-z_]\w*)\)?(?:\[\d+\])?\s*=\s*'
                   r'(?:.*?)__frame\.(\w+(?:\[\d+\])?)')
RUN = re.compile(r'\(\s*&(\w+)\s*\+\s*')
BSS = re.compile(r'^static (t_\w+)& (\w+) = \*\(t_\w+\*\)\(bmf_bss \+ (0x[0-9A-F]+)', re.M)
TYPEDEF = re.compile(r'^typedef\s+(.+?)\s+(t_\w+)\s*(\[[^\]]*\])?\s*;', re.M)


def load():
    lines = open(SRC).read().split('\n')
    span, sigs = {}, {}
    for a, b, nm, sig in structs.bodies(lines):
        sigs[nm] = (a, b, sig)
        for ln in range(a, b + 1):
            span[ln] = nm
    return lines, span, sigs


def rawoffsets(lines, span):
    """Every raw-offset site, as (line, function, base).

    One site, not one pattern: `*(uint32_t *)((char *)_this + 278720)` is P1
    with a byte-cast base *and* P2 on that inner cast, and counting it twice
    made retyping a parameter look like it doubled the arithmetic in a function
    when all it did was add the cast the new type needs.  Keyed by where the
    base identifier sits, so each one counts once.
    """
    out = []
    for i, l in enumerate(lines):
        code = l.split('//')[0]
        at = {}
        for p in (P1, P2, P3):
            for m in p.finditer(code):
                at.setdefault(m.start(1), m.group(1))
        for pos in sorted(at):
            out.append((i + 1, span.get(i, '<top>'), at[pos]))
    return out


def frames(lines):
    """One dict per function that has a frame."""
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        body = lines[a:b + 1]
        sizes = [int(m.group(1)) for m in map(FRAME.search, body) if m]
        if not sizes:
            continue
        aliases, names = [], set()
        for l in body:
            m = ALIAS.search(l)
            if m:
                aliases.append((m.group(1), m.group(2)))
                names.add(m.group(1))
        slots = collections.Counter(t for _, t in aliases)
        runs = collections.Counter()
        for l in body:
            if ALIAS.search(l):
                continue
            for m in RUN.finditer(l.split('//')[0]):
                if m.group(1) in names:
                    runs[m.group(1)] += 1
        out.append(dict(fn=nm.lstrip('_'), size=sum(sizes), alias=len(aliases),
                        slots={t: c for t, c in slots.items() if c > 1}, runs=runs))
    return out


# The five things the 60 globals in bmf_bss really are; see REFACTORING3.md §3.1.
# A starts at 0x44338C and not at 0x44339C: `__n256_2`, `__n512`, `plane_count`
# and `near_lossless_max` are record 0 of the same 16-byte-record table, which
# `alt_model_p2_encode` walks with those four names as its field bases.
GROUPS = [('A  plane descriptors', 0x44338C, 0x4433DB),
          ('B  level geometry', 0x445714, 0x445733),
          ('C  thresholds + biases', 0x4458E0, 0x4458F7)]
BUFFERS = {'exclusion_mask', '__byte_445440', 'model_geometry'}


def bss():
    src = open(SRC).read()
    ty = {m.group(2): m.group(1) + (m.group(3) or '') for m in TYPEDEF.finditer(src)}
    out = []
    for m in BSS.finditer(src):
        addr = int(m.group(3), 16)
        if m.group(2) in BUFFERS:
            grp = 'D  buffers'
        else:
            grp = next((g for g, lo, hi in GROUPS if lo <= addr <= hi), 'E  scalars')
        uses = len(re.findall(r'\b%s\b' % re.escape(m.group(2)), src)) - 1
        subs = [' '.join(s.split()) for s in
                re.findall(re.escape(m.group(2)) + r'\s*\[([^\]]*)\]', src)]
        out.append(dict(name=m.group(2), addr=addr, ty=ty.get(m.group(1), '?'),
                        group=grp, uses=uses, subs=subs))
    return sorted(out, key=lambda g: (g['group'], g['addr']))


MEMBER = re.compile(r'^\s*([A-Za-z_][\w ]*?\s*\**)\s*(\w+)\s*(\[[^\]]*\])?\s*;', re.M)


def fields():
    """{struct: ({offset: type} for fNN members, [named members])}."""
    src = open(SRC).read()
    out = {}
    for m in re.finditer(r'^struct\s+(\w+)\s*\{(.*?)^\};', src, re.S | re.M):
        got, named = {}, []
        for f in MEMBER.finditer(m.group(2)):
            o = re.fullmatch(r'f(\d+)', f.group(2))
            if o:
                got[int(o.group(1))] = f.group(1).strip() + (f.group(3) or '')
            elif not f.group(2).startswith('_pad'):
                named.append(f.group(2))
        out[m.group(1)] = (got, named)
    return out


def summary():
    lines, span, sigs = load()
    src = '\n'.join(lines)
    raw = rawoffsets(lines, span)
    this = [r for r in raw if r[2] == '_this']
    fr = frames(lines)
    members = fields()
    named = sum(len(v[1]) for v in members.values())
    row = lambda k, v: print('%-34s %s' % (k, v))                 # noqa: E731
    row('subs1.hpp / bmf.cpp lines',
        '%d / %d' % (len(lines) - 1, len(open('bmf.cpp').read().split('\n')) - 1))
    row('raw-offset sites', len(raw))
    row('  off `_this`',
        '%d, in %d functions' % (len(this), len(set(r[1] for r in this))))
    row('pointer casts', len(re.findall(r'\([A-Za-z_]\w*\s*\*+\s*\)', src)))
    row('globals still at a 1997 address', len(bss()))
    row('frames', '%d, %d bytes, %d aliases'
        % (len(fr), sum(f['size'] for f in fr), sum(f['alias'] for f in fr)))
    row('  slots carrying two names',
        '%d, %d extra names, in %d functions'
        % (sum(len(f['slots']) for f in fr),
           sum(sum(c - 1 for c in f['slots'].values()) for f in fr),
           sum(1 for f in fr if f['slots'])))
    row('  member runs walked as arrays',
        '%d sites, %d bases, %d functions'
        % (sum(sum(f['runs'].values()) for f in fr),
           sum(len(f['runs']) for f in fr),
           sum(1 for f in fr if f['runs'])))
    row('  frames that dissolve outright',
        '%d, %d aliases' % (sum(1 for f in fr if not f['slots'] and not f['runs']),
                            sum(f['alias'] for f in fr
                                if not f['slots'] and not f['runs'])))
    row('structs', '%d, %d still ObjN' % (len(members),
                                          sum(1 for k in members
                                              if re.fullmatch(r'Obj\d+', k))))
    row('  fNN members / named ones',
        '%d / %d' % (sum(len(v[0]) for v in members.values()), named))
    row('distinct vNN locals', len(set(re.findall(r'\bv\d+\b', src))))
    row('goto / LABEL_n:', '%d / %d' % (src.count('goto '),
                                        len(re.findall(r'^LABEL_\d+:', src, re.M))))
    row('__fwd_* shims', len(set(re.findall(r'\b__fwd_\w+', src))))


def main():
    if '--this' in sys.argv:
        lines, span, sigs = load()
        this = collections.Counter(r[1] for r in rawoffsets(lines, span)
                                   if r[2] == '_this')
        for fn, n in this.most_common():
            sig = sigs.get(fn, (0, 0, ''))[2]
            m = re.search(r'\(\s*([^,)]*?)\s*_this', sig)
            print('%-28s %4d   %s' % (fn.lstrip('_'), n, m.group(1) if m else '?'))
    elif '--frames' in sys.argv:
        for f in sorted(frames(open(SRC).read().split('\n')), key=lambda f: -f['size']):
            print('%-26s %7d B  %3d aliases  %2d shared slots  %2d run sites'
                  % (f['fn'], f['size'], f['alias'], len(f['slots']),
                     sum(f['runs'].values())))
    elif '--bss' in sys.argv:
        for g in bss():
            kinds = sorted(set(g['subs']))
            shape = ('' if not kinds else
                     'const' if all(re.fullmatch(r'\d+|0x[0-9A-Fa-f]+', s)
                                    for s in kinds) else
                     'var[%s]' % ', '.join(kinds[:3]))
            print('%-8s %-20s 0x%06X  %-18s uses=%-4d %s'
                  % (g['group'][0], g['name'], g['addr'], g['ty'], g['uses'], shape))
    elif '--overlap' in sys.argv:
        want = sys.argv[sys.argv.index('--overlap') + 1:]
        f = {k: v[0] for k, v in fields().items()}
        keys = sorted(set().union(*(set(f.get(n, {})) for n in want)))
        keys = [k for k in keys if sum(1 for n in want if k in f.get(n, {})) >= 2]
        print('   '.join(['%-8s' % 'offset'] + ['%-14s' % n for n in want]))
        for k in keys:
            print('   '.join(['%-8d' % k]
                             + ['%-14s' % f.get(n, {}).get(k, '-') for n in want]))
    else:
        summary()
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:                    # `| head`, which is how this is read
        sys.exit(0)
