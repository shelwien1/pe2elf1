#!/usr/bin/env python3
"""dtshuff.py TS102114.txt OUT.inc -- the Huffman code books of ETSI TS 102 114 Annex D.5 (the text
of `pdftotext -layout` of V1.6.1) as C++ tables, each checked: every level of its range once, codes
that fit their lengths, prefix-free, complete (Kraft sum 1)

A data line is triplets (level, code length, code); a code of 1000 or more is printed with spaces
between groups of 3 digits, and as no level or length has 3 digits, a 3-digit token after a code is
always a group of it. Tables printed side by side take a line's triplets by the x of the nearest
table heading."""
import re, sys
from fractions import Fraction

lines = open(sys.argv[1], encoding='utf-8', errors='replace').read().split('\n')
start = [i for i, l in enumerate(lines) if re.match(r'^D\.5\s+Huffman Code Books\s*$', l)][-1]  # (not the contents' line)
end = next(i for i, l in enumerate(lines) if i > start and re.search(r'Table V\.3: 3-level', l))


def rng(name):  # the levels of a table
    n = int(re.match(r'S?[A-G](\d+)$', name).group(1))
    if name.startswith('S'):
        return list(range(-64, 65))
    if n == 4:  # TMODE
        return list(range(0, 4))
    if n == 12:  # BHUFF: ABITS 1-12
        return list(range(1, 13))
    h = (n - 1) // 2
    return list(range(-h, h + 1))


tables = {}
group = []      # the tables of the current heading lines: (x, name)
data_seen = True
num = re.compile(r'-?\d+')
for i in range(start, end):
    l = lines[i]
    if 'ETSI' in l or not l.strip() or re.match(r'^\s*D\.5\.\d+', l):
        continue
    hs = [(m.start(), m.group(1)) for m in re.finditer(r'Table (S?[A-G]\d+)\b', l)]
    if hs:
        if data_seen:
            group = []
            data_seen = False
        group += hs
        group.sort()
        for _, n in hs:
            tables.setdefault(n, [])
        continue
    if re.search(r'[A-Za-z]', l):  # column headers, notes
        continue
    toks = [(m.start(), m.group(0)) for m in num.finditer(l)]
    if not toks or not group:
        continue
    data_seen = True
    k = 0
    while k < len(toks):
        if k + 2 >= len(toks):
            sys.exit('line %d: a triplet cut short: %s' % (i + 1, toks))
        x0, lev, ln, code = toks[k][0], int(toks[k][1]), int(toks[k + 1][1]), toks[k + 2][1]
        k += 3
        while k < len(toks) and re.fullmatch(r'\d{3}', toks[k][1]):
            code += toks[k][1]
            k += 1
        name = min(group, key=lambda g: abs(g[0] - x0))[1] if len(group) > 1 else group[0][1]
        tables[name].append((lev, ln, int(code)))

# Two misprints, in V1.4.1, V1.5.1 and V1.6.1 alike. Table D65 stops at +-30: +-31 and +-32 take the
# four 15-bit codes left free (28848-28851, below +-30's 28852 and 28853), larger levels lower codes
# as in the rest of the table. Table D129 has 37 twice: the second, code 1581 (37's is 1582), is
# -37. (FFmpeg's decoder has them so too.)
def free_codes(t, ln):
    used = set()
    for _, l2, c in t:
        if l2 <= ln:
            for x in range(c << (ln - l2), (c + 1) << (ln - l2)):
                used.add(x)
        else:
            used.add(c >> (l2 - ln))
    return sorted(set(range(1 << ln)) - used)


if len(tables.get('D65', [])) == 61 and not any(abs(e[0]) > 30 for e in tables['D65']):
    fc = free_codes(tables['D65'], 15)
    if fc != [28848, 28849, 28850, 28851]:
        sys.exit('D65: free codes %s, not the four expected' % fc[:8])
    tables['D65'] += [(-32, 15, 28848), (32, 15, 28849), (-31, 15, 28850), (31, 15, 28851)]
d129 = tables.get('D129', [])
if sorted(e[0] for e in d129).count(37) == 2 and (37, 11, 1581) in d129:
    d129[d129.index((37, 11, 1581))] = (-37, 11, 1581)

order = ['A3', 'A4', 'B4', 'C4', 'D4', 'A5', 'B5', 'C5', 'A7', 'B7', 'C7', 'A9', 'B9', 'C9',
         'A12', 'B12', 'C12', 'D12', 'E12', 'A13', 'B13', 'C13'] + \
        ['%s%d' % (c, n) for n in (17, 25, 33, 65) for c in 'ABCDEFG'] + \
        ['S%s129' % c for c in 'ABCDE'] + ['%s129' % c for c in 'ABCDEFG']
bad = 0
for name in order:
    t = tables.get(name, [])
    levels = sorted(e[0] for e in t)
    kraft = sum(Fraction(1, 1 << e[1]) for e in t)
    fits = all(0 <= e[2] < (1 << e[1]) for e in t)
    codes = sorted(format(e[2], '0%db' % e[1]) for e in t)
    pfx = all(not codes[k + 1].startswith(codes[k]) for k in range(len(codes) - 1))
    if levels != rng(name) or kraft != 1 or not fits or not pfx:
        bad += 1
        miss = sorted(set(rng(name)) - set(levels))
        dup = sorted(v for v in set(levels) if levels.count(v) > 1)
        print('%-6s %3d entries; missing %s, twice %s, kraft %s, fits %s, prefix-free %s' % (
            name, len(t), miss[:10], dup[:10], kraft, fits, pfx))
print('%d tables, %d bad' % (len(order), bad))
if bad and '-force' not in sys.argv:
    sys.exit(1)
with open(sys.argv[2], 'w') as f:
    f.write('// dtscore_huff.inc -- the Huffman code books of ETSI TS 102 114 V1.6.1, Annex D.5 (made by\n'
            '// dtshuff.py from the text of the PDF): a book\'s entries {level, code length, code}\n')
    for name in order:
        t = sorted(tables[name], key=lambda e: (e[1], e[2]))
        f.write('static const HuffEntry HB_%s[%d] = {' % (name, len(t)))
        f.write(', '.join('{%d, %d, %d}' % e for e in t))
        f.write('};\n')
print('written', sys.argv[2])
