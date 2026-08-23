#!/usr/bin/env python3
# annotate.py -- refresh the human-readable line above each mapping.
#
# A threshold mapping is a bit per input value and a masking is a bit per
# context bit, so both are unreadable as written: three edges in a 3537-character
# pattern is not something anyone can check by eye.  The comment above each one
# says where the edges are, or which bits a mask keeps.
#
# It is derived, so it goes stale the moment IDX/import.pl folds an optimizer
# run back in.  Run this afterwards:
#
#   python3 IDX/annotate.py
import glob
import re


def edges(base, pat):
    return [j + (base - 1) for j, c in enumerate(pat) if c == '1']


def kept(pat):
    return sorted(len(pat) - 1 - j for j, c in enumerate(pat) if c == '1')


THR = re.compile(r'^ ([A-Za-z][A-Za-z0-9_]*): (\S+), (-?\d+)!([01]+)$')
MSK = re.compile(r'^ ([A-Za-z][A-Za-z0-9_]*): (\S+), b?&([01]+)$')

total = 0
for path in sorted(glob.glob('IDX/bmf-*.idx')):
    out, n = [], 0
    for line in open(path).read().split('\n'):
        if line.startswith(' # ') and (' edges at ' in line or ' keeps bits ' in line):
            continue                      # the previous run's comment
        m = THR.match(line)
        if m:
            e = edges(int(m.group(3)), m.group(4))
            out.append(' # %s: %d edge%s at %s, over %d values'
                       % (m.group(1), len(e), '' if len(e) == 1 else 's',
                          ', '.join(str(x) for x in e), len(m.group(4))))
            n += 1
        else:
            m = MSK.match(line)
            if m:
                b = kept(m.group(3))
                out.append(' # %s: keeps bit%s %s of a %d-bit context'
                           % (m.group(1), '' if len(b) == 1 else 's',
                              ', '.join(str(x) for x in b), len(m.group(3))))
                n += 1
        out.append(line)
    open(path, 'w').write('\n'.join(out))
    if n:
        print('%-20s %d mappings' % (path, n))
    total += n
print('%d mappings annotated' % total)
