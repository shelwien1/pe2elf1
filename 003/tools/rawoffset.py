#!/usr/bin/env python3
"""Find numbers in the code that are a struct member's byte offset.

    python3 tools/rawoffset.py subs1.hpp
    python3 tools/rawoffset.py subs1.hpp --all    # every match, not just big ones

An earlier round reported "raw-offset sites 0" and the count was a spelling:
it looked for `_this + ofs`, which is the shape Hex-Rays emits for a *small*
member, and every one of those had been named.  What it could not see is the
same thing written as an index into a byte view --

    bankp = (uint8_t *)blk + (bank << 17);
    d4000 = (P2Count *)&bankp[4 * (ctxw ^ 0x4000) + 284712];

-- where 284712 is `offsetof(AltP2Block, p2_ctr)`, thirty times over.  On i386
the number is right.  Five pointers earlier in that record are four bytes each;
give them eight and the counters are twenty bytes further on, and every one of
those thirty reads lands in the middle of a record.  Nothing diagnoses it: the
expression is well-formed and simply means something else.

So this works the other way round.  It reads the offsets the file *states* --
the `// +N` on each member and the `__builtin_offsetof(...) == N` in the
static_asserts -- and then looks for those numbers in the code.  A match is
not proof (13 is an offset and also a number), which is why the default only
reports offsets past `--min`, where a coincidence is unlikely and a member is
almost certainly what is meant.

Zero is the target and it is reachable: every one of these has a name.
"""
import collections
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

MIN = 256
MEMBER = re.compile(r'^\s*(?:[A-Za-z_]\w*\s+)+\*?\s*(\w+)\s*(?:\[[^\]]*\])*\s*;'
                    r'\s*//\s*\+(\d+)')
ASSERT = re.compile(r'__builtin_offsetof\(\s*(\w+)\s*,\s*([\w\[\]]+)\s*\)\s*==\s*(\d+)')
NUM = re.compile(r'(?<![\w.])(\d{3,})(?![\w.])')


def offsets(lines):
    """{byte offset: ['Struct::member', ...]} from what the file states."""
    out, cur, depth = collections.defaultdict(list), None, 0
    for l in lines:
        m = re.match(r'\s*struct\s+([A-Za-z_]\w*)\s*\{', l.split('//')[0])
        if m and depth == 0:
            cur, depth = m.group(1), 1
            continue
        if cur:
            c = l.split('//')[0]
            depth += c.count('{') - c.count('}')
            if depth <= 0:
                cur = None
            else:
                d = MEMBER.match(l)
                if d:
                    out[int(d.group(2))].append('%s::%s' % (cur, d.group(1)))
        for a in ASSERT.finditer(l):
            name = '%s::%s' % (a.group(1), a.group(2))
            if name not in out[int(a.group(3))]:
                out[int(a.group(3))].append(name)
    return out


def survey(lines, low=MIN):
    """[(line, number, [names], text)] for every stated offset used as a number."""
    known = offsets(lines)
    out = []
    for a, b, nm, sig in structs.bodies(lines):
        for i in range(a, b + 1):
            code = lines[i].split('//')[0]
            # A `static_assert` *states* a layout; it does not use one.
            if 'static_assert' in code or '__builtin_offsetof' in code:
                continue
            for m in NUM.finditer(code):
                v = int(m.group(1))
                if v >= low and v in known:
                    out.append((i + 1, v, known[v], lines[i].strip()[:60]))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    low = 0 if '--all' in sys.argv else MIN
    lines = open(path).read().split('\n')
    rows = survey(lines, low)
    for line, v, names, text in rows:
        print('%6d %-9d %-34s %s' % (line, v, ', '.join(names)[:34], text))
    print('%d numbers in code that a struct states as a member offset' % len(rows))


if __name__ == '__main__':
    main()
