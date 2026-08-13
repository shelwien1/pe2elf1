#!/usr/bin/env python3
"""Give a `bmf_bss` global storage of its own.

    python3 tools/unbss.py subs1.hpp --list
    python3 tools/unbss.py subs1.hpp __dword_4458E0 __dword_4458E4
    python3 tools/unbss.py subs1.hpp --group C --name bias --array

REFACTORING3.md §3.  The 60 globals still live at their 1997 addresses:

    typedef int32_t t_dword_4458E0;
    static t_dword_4458E0& __dword_4458E0 = *(t_dword_4458E0*)(bmf_bss + 0x4458E0 - 0x443380);

which is one object pretending to be sixty because the code strides between
them.  Where it does not -- `tools/blob-independence.txt` marks 41 of the 60
SHARED and the other 19 were never tested -- the global can just be a variable:

    static int32_t __dword_4458E0;

`bmf_bss` keeps its size and every other global keeps its offset, so this is
one global at a time and the gate answers each one.

`--array NAME` does the same to a run of consecutive same-type globals and
makes them one array, which is what four parallel accumulators or a table of
per-plane records actually are.  The globals then become `NAME[0]`, `NAME[1]`
and so on at every use site.
"""
import re
import sys

DECL = re.compile(r'^typedef (.+?) (t_\w+)(\[[^]]*\])?;\n'
                  r'static t_\w+& (\w+) = \*\(t_\w+\*\)\(bmf_bss \+ '
                  r'(0x[0-9A-F]+) - 0x443380\);\n', re.M)

GROUPS = {'A': (0x44338C, 0x4433DB), 'B': (0x445714, 0x445733),
          'C': (0x4458E0, 0x4458F7)}


def globals_of(src):
    """[(span, type, array, name, address)] in declaration order."""
    return [(m.span(), m.group(1), m.group(3) or '', m.group(4),
             int(m.group(5), 16)) for m in DECL.finditer(src)]


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    src = open(path).read()
    gs = globals_of(src)
    flags = {'--name', '--array', '--group'}
    argv, args = sys.argv[2:], []
    skip = False
    for i, x in enumerate(argv):
        if skip:
            skip = False
            continue
        if x in flags:
            skip = True
            continue
        if not x.startswith('--'):
            args.append(x)

    if '--group' in sys.argv:
        lo, hi = GROUPS[sys.argv[sys.argv.index('--group') + 1]]
        args = [g[3] for g in gs if lo <= g[4] <= hi]
    if '--list' in sys.argv or not args:
        for span, ty, arr, name, addr in gs:
            print('0x%06X  %-22s %s%s' % (addr, name, ty, arr))
        print('%d globals in bmf_bss' % len(gs))
        return 0

    want = [g for g in gs if g[3] in args]
    if len(want) != len(args):
        print('not found: %s' % ', '.join(set(args) - set(g[3] for g in want)))
        return 1

    arrname = None
    if '--array' in sys.argv:
        arrname = sys.argv[sys.argv.index('--array') + 1] \
            if len(sys.argv) > sys.argv.index('--array') + 1 \
            and not sys.argv[sys.argv.index('--array') + 1].startswith('-') else None
    if '--name' in sys.argv:
        arrname = sys.argv[sys.argv.index('--name') + 1]

    if arrname:
        # The `[0x10000]` on some typedefs is round two's placeholder for "this
        # is subscripted and the extent is unknown", not a real extent; the run
        # this builds is what settles the extent, so ignore it here.
        tys = set(g[1] for g in want)
        if len(tys) != 1:
            print('a run has to have one type, got %s' % ', '.join(sorted(tys)))
            return 1
        want.sort(key=lambda g: g[4])
        step = {'char': 1, 'int8_t': 1, 'uint8_t': 1, 'int16_t': 2,
                'uint16_t': 2}.get(want[0][1], 4)
        gaps = [b[4] - a[4] for a, b in zip(want, want[1:])]
        if any(g != step for g in gaps):
            print('not consecutive: gaps %s, element %d' % (gaps, step))
            return 1

    # Declarations first, back to front, so the spans stay valid; the use-site
    # rewrite runs afterwards because it changes lengths all over the file and
    # would invalidate every span if it went first.
    first = min(want, key=lambda g: g[0][0])
    for span, ty, arr, name, addr in sorted(want, key=lambda g: -g[0][0]):
        new = '' if arrname else ('static %s %s%s;   // was 0x%06X in bmf_bss\n'
                                  % (ty, name, arr, addr))
        src = src[:span[0]] + new + src[span[1]:]
    if arrname:
        at = first[0][0]
        src = (src[:at] + 'static %s %s[%d];   // was 0x%06X..0x%06X in bmf_bss\n'
               % (first[1], arrname, len(want), want[0][4], want[-1][4]) + src[at:])
        for i, g in enumerate(want):
            # A global that was already subscripted -- `__dword_44573C[n4]` is
            # the whole run read as the table it is -- keeps its subscript and
            # only changes name; the rest become elements.
            src = re.sub(r'\b%s\b(?=\s*\[)' % g[3], arrname, src)
            src = re.sub(r'\b%s\b' % g[3], '%s[%d]' % (arrname, i), src)
    open(path, 'w').write(src)
    print('%d globals given storage of their own%s'
          % (len(want), ' as %s[%d]' % (arrname, len(want)) if arrname else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
