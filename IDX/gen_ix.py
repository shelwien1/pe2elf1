#!/usr/bin/env python3
# gen_ix.py -- port the CtxIdx context-index structures.
#
# Every context index in the codec is built by chaining CtxIdx calls whose bit
# positions, field widths and radix strides are template arguments.  Those are
# as much a tuning decision as any threshold -- which bit a flag sits at decides
# what the sub-context extractions downstream of it see -- but they are template
# arguments, so an IDX value can only reach them through the reference-typed
# non-type parameter of IDX-FORMAT.md sec.8.
#
# This scans the sources for CtxIdx chains with literal arguments, gives each
# argument a name derived from the site, writes IDX/bmf-IX.idx, and rewrites the
# call sites to name the parameters instead.
import collections
import re
import sys

sys.path.insert(0, 'IDX')
from idxgen import emit

FILES = ['alt_p1.inc', 'alt_p2.inc', 'counters.inc', 'model.inc']

CALL = re.compile(r'\.(bit|bits|digit|above|bit_of)<\s*(\d+)\s*(?:,\s*(\d+)\s*)?>')
# The statement a chain belongs to, for naming: the nearest assignment target or
# return above the chain on the same line.
TARGET = re.compile(r'(?:([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*=\s*)?[^=]*CtxIdx\{\}')


def site_name(path, line, seen):
    m = TARGET.search(line)
    base = m.group(1) if m and m.group(1) else 'ix'
    base = re.sub(r'^(cx|ctr|nb|ctx)_?', lambda g: g.group(0), base)
    key = '%s_%s' % (path.split('.')[0].replace('alt_', ''), base)
    n = seen[key]
    seen[key] += 1
    return key if n == 0 else '%s%d' % (key, n)


def scan():
    """(file, line-number, site name, [(kind, pos, width, span)]) per chain."""
    out = []
    seen = collections.Counter()
    for path in FILES:
        for n, line in enumerate(open(path).read().split('\n'), 1):
            if 'CtxIdx{}' not in line:
                continue
            calls = [(m.group(1), m.group(2), m.group(3), m.span())
                     for m in CALL.finditer(line)]
            if not calls:
                continue
            out.append((path, n, site_name(path, line, seen), calls))
    return out

ROLE = {'bit': 'bit', 'bit_of': 'bit', 'above': 'shift', 'bits': 'field',
        'digit': 'digit'}


def build():
    chains = scan()
    items, edits = [], {}
    for path, n, name, calls in chains:
        short = name
        rows = []
        for k, (kind, pos, width, span) in enumerate(calls):
            pname = '%s_p%d' % (short, k)
            items.append((pname, int(pos)))
            names = [pname]
            if width is not None:
                wname = '%s_w%d' % (short, k)
                items.append((wname, int(width)))
                names.append(wname)
            rows.append((kind, span, names))
        edits.setdefault(path, []).append((n, rows))
    return items, edits


def rewrite_calls(edits):
    for path, sites in edits.items():
        lines = open(path).read().split('\n')
        for n, rows in sites:
            line = lines[n - 1]
            # right to left, so earlier spans stay valid
            for kind, (a, b), names in reversed(rows):
                arg = ', '.join('IX_' + x for x in names)
                line = line[:a] + '.%s<%s>' % (kind, arg) + line[b:]
            lines[n - 1] = line
        open(path, 'w').write('\n'.join(lines))


if __name__ == '__main__':
    items, edits = build()
    emit('IDX/bmf-IX.idx', 'IX', [
        '# ---------------------------------------------------------------------',
        '# Context-index layouts.',
        '#',
        '# Every context index in the codec is a chain of CtxIdx calls that place',
        '# quantised values and flags at fixed bit positions.  The thresholds those',
        '# values come from are declared in the other modules; this one declares',
        '# where each result lands.  That is a tuning decision in its own right:',
        '# alt-P2 pulls a bank number out of a context with a plain shift, so which',
        '# bit a flag sits at decides whether it is part of the bank or part of the',
        '# row within it.',
        '#',
        '# The positions are template arguments, which an IDX value can only reach',
        '# through a reference-typed non-type parameter -- see IDXP in bmf_idx.inc',
        '# and IDX-FORMAT.md sec.8.  <site>_pN is the Nth argument\'s position or',
        '# stride, <site>_wN its field width or radix.',
        '# ---------------------------------------------------------------------',
    ], [
        (['One block per index, in source order.'], 0, items),
    ])
    rewrite_calls(edits)
    print('%d chains, %d parameters' % (len(sum(edits.values(), [])), len(items)))
