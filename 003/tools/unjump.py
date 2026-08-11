#!/usr/bin/env python3
"""Turn a `goto` into the block it jumps into.

    python3 tools/unjump.py subs1.hpp
    python3 tools/unjump.py subs1.hpp --all

`degoto.py --why` counts eleven labels it declines because the `goto` jumps
*into* a block. Every one of them is the same shape, and it is not irreducible:

    if ( n15 >= 15 )              if ( n15 < 15 )
      goto LABEL_115;             {
    <region>                        <region>
    if ( n15 > 0 )        →       }
    {                             if ( n15 >= 15 || n15 > 0 )
    LABEL_115:                    {
      <body>                        <body>
    }                             }

The rewrite is exact whatever the two conditions are -- no implication between
them is assumed, and `C || D` is left for a reader to simplify -- provided the
region cannot change the answer to either. So that is what is checked:

  * the label is the first line of a block, and the `{` above it is the body of
    an `if`;
  * exactly one `goto` names it, it is forward, and it is the whole consequent
    of another `if`;
  * the region between them brace-balances and contains no label;
  * the region assigns nothing that either condition reads, takes no address of
    anything either reads, and calls nothing with a bare identifier either
    condition reads.

The last one is the interesting condition and it is deliberately blunt: a call
in the region is allowed, because `__rescale_three_way(&v445[1])` plainly
cannot touch `n15`, but only while no name in the conditions is anywhere in the
call. A condition on a global would fail it, which is the right answer for a
tool that cannot see the callee.
"""
import re
import sys

sys.path.insert(0, __file__.rsplit('/', 1)[0])
import structs                                                    # noqa: E402

LABEL = re.compile(r'^(\s*)(LABEL_\d+):\s*$')
IF = re.compile(r'^(\s*)if \( (.*) \)\s*$')
WRITE = [r'\b%s\s*(?:\+\+|--)', r'(?:\+\+|--)\s*%s\b', r'\b%s\s*[-+*/|&^%%]?=(?!=)',
         r'&\s*%s\b', r'\b(?:LO|HI)(?:BYTE|WORD|DWORD)\d?\s*\(\s*%s\s*\)']


def candidates(lines):
    out = []
    for a, b, nm, _ in structs.bodies(lines):
        labs, gotos = {}, {}
        for i in range(a, b + 1):
            m = LABEL.match(lines[i])
            if m:
                labs[m.group(2)] = i
            for g in re.finditer(r'goto (LABEL_\d+);', lines[i].split('//')[0]):
                gotos.setdefault(g.group(1), []).append(i)
        for name, at in labs.items():
            gs = gotos.get(name, [])
            if len(gs) != 1 or gs[0] >= at:
                continue
            g = gs[0]
            # the goto must be the whole consequent of an `if`
            mg = IF.match(lines[g - 1])
            if not mg or lines[g].strip() != 'goto %s;' % name:
                continue
            # the label must be the first line of an `if` body
            if lines[at - 1].strip() != '{':
                continue
            md = IF.match(lines[at - 2])
            if not md:
                continue
            region = lines[g + 1:at - 2]
            if any(LABEL.match(r) for r in region):
                continue
            if sum(r.count('{') - r.count('}') for r in region) != 0:
                continue
            names = set(re.findall(r'\b([A-Za-z_]\w*)\b', mg.group(2) + ' ' + md.group(2)))
            names -= {'uint8_t', 'int8_t', 'uint16_t', 'int16_t', 'uint32_t',
                      'int32_t', 'uint64_t', 'int64_t', 'sizeof'}
            body = '\n'.join(region)
            if any(re.search(p % re.escape(v), body) for v in names for p in WRITE):
                continue
            # a call anywhere in the region must not mention a condition name
            calls = re.findall(r'\w+\s*\(([^;]*)\)', body)
            if any(re.search(r'\b%s\b' % re.escape(v), c) for c in calls for v in names):
                continue
            out.append((nm, name, g, at, mg, md, region))
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'subs1.hpp'
    lines = open(path).read().split('\n')
    found = candidates(lines)

    if '--all' not in sys.argv:
        for nm, name, g, at, mg, md, region in found:
            print('%6d  %-22s %-11s if ( %s ) over %d lines into if ( %s )' %
                  (g + 1, nm.lstrip('_'), name, mg.group(2)[:24], len(region),
                   md.group(2)[:24]))
        print('%d gotos into a block' % len(found))
        return 0

    for nm, name, g, at, mg, md, region in sorted(found, key=lambda f: -f[2]):
        ind = mg.group(1)
        new = ([ind + 'if ( !(%s) )' % mg.group(2), ind + '{'] +
               ['  ' + r for r in region] +
               [ind + '}',
                md.group(1) + 'if ( %s || %s )' % (mg.group(2), md.group(2))])
        # `at - 1` is the block's own `{`, which stays where it is
        lines[g - 1:at - 1] = new
        # the label line moved with the block; find and drop it
        for k in range(g, min(g + len(new) + 40, len(lines))):
            m = LABEL.match(lines[k])
            if m and m.group(2) == name:
                del lines[k]
                break
    open(path, 'w').write('\n'.join(lines))
    print('%d gotos folded into the block they entered' % len(found))
    return 0


if __name__ == '__main__':
    sys.exit(main())
