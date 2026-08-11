#!/usr/bin/env python3
"""Names still spelled the way Hex-Rays spelled them, asked of the source.

    python3 tools/unnamed.py
    python3 tools/unnamed.py --all      # the conventional names too

Every other measure of this in `shape.py` tests a *spelling* -- `[vt]NN`, then
`nNNNN`, then the same behind a `rename.py` prefix, then MSVC's CRT parameter
vocabulary -- and each of the four read zero while names it could not match sat
in the file.  Four corrections to one row is a pattern, and the pattern is that
a test which is a spelling gets defeated by a spelling.

This is not a spelling test.  The first commit of `subs1.hpp` *is* the Hex-Rays
output, `addrmap.py` maps every current body to the address it was decompiled
from, and the original's bodies are still called `sub_<address>`.  So the two
join, and a local declared in a body under the same name in both is by
construction a name nobody here has chosen.  The answer is exact and finite:
there is no pattern to get wrong, and nothing can hide behind a prefix.

What it cannot say is whether a surviving name is *wrong*.  Hex-Rays writes
`i`, `j`, `k` and `result`, and so would anyone; those are listed in `KEPT`
below and reported apart rather than dropped, because a name kept on purpose
and a name nobody looked at are different things and only one of them is work.
"""
import re
import sys
import subprocess
import collections

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

ORIGIN = '40ce575:003/subs1.hpp'
ADDR = 'tools/addrmap.py'
# Names Hex-Rays chose that a person would have chosen too.  Kept on purpose,
# counted apart -- not excluded, because a list that silently drops entries is
# the defect this whole tool exists to answer.
KEPT = {'i', 'j', 'k', 'm', 'n', 'x', 'y', 'result', 'buf', '_this'}


def original():
    """The decompiler's own output, by body address."""
    try:
        src = subprocess.check_output(['git', 'show', ORIGIN],
                                      stderr=subprocess.DEVNULL).decode()
    except (subprocess.CalledProcessError, OSError):
        return None
    lines = src.split('\n')
    out = {}
    for a, b, nm, sig in structs.bodies(lines):
        m = re.fullmatch(r'sub_([0-9A-Fa-f]+)', nm.lstrip('_'))
        if m:
            out[int(m.group(1), 16)] = set(structs.decl_types(sig, lines, a, b))
    return out


def addresses():
    try:
        txt = subprocess.check_output([sys.executable, ADDR],
                                      stderr=subprocess.DEVNULL).decode()
    except (subprocess.CalledProcessError, OSError):
        return {}
    out = {}
    for l in txt.split('\n'):
        m = re.fullmatch(r'(\w+)\s+(0x[0-9A-Fa-f]+)\s*', l)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def survey(path='subs1.hpp'):
    """({name: bodies} still Hex-Rays', {name: bodies} kept on purpose, joined)."""
    was, addr = original(), addresses()
    if was is None:
        return None, None, 0
    lines = open(path).read().split('\n')
    live, kept, joined = (collections.defaultdict(set),
                          collections.defaultdict(set), 0)
    for a, b, nm, sig in structs.bodies(lines):
        fn = nm.lstrip('_')
        at = addr.get(fn)
        if at is None or at not in was:
            continue
        joined += 1
        for n in set(structs.decl_types(sig, lines, a, b)) & was[at]:
            (kept if n in KEPT else live)[n].add(fn)
    return live, kept, joined


if __name__ == '__main__':
    live, kept, joined = survey(sys.argv[1] if len(sys.argv) > 1
                                and not sys.argv[1].startswith('--')
                                else 'subs1.hpp')
    if live is None:
        sys.exit('cannot read %s -- not a checkout of this repository?' % ORIGIN)
    rows = sorted(live.items(), key=lambda kv: (-len(kv[1]), kv[0]))
    if '--all' in sys.argv:
        rows += sorted(kept.items(), key=lambda kv: (-len(kv[1]), kv[0]))
    for n, v in rows:
        print('  %-16s %2d  %s' % (n, len(v), ', '.join(sorted(v))[:56]))
    print('%d names still spelled as Hex-Rays spelled them, %d declarations, '
          'in %d bodies joined by address (%d more kept on purpose)'
          % (len(live), sum(len(v) for v in live.values()), joined,
             sum(len(v) for v in kept.values())))
