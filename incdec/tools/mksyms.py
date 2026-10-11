#!/usr/bin/env python3
"""vbn.txt: the address of every global in vbn.exe.c's data declarations.

incdec.md §6.1 needs a symbol table, and vbn ships none: Hex-Rays prints the
data declarations in address order but without addresses, and many globals carry
invented names (n4096, buf_6, src_5) that say nothing about where they live.
Five sources, in order of trust, each checked against the previous ones:

  name      the address is in the auto-generated name (dword_1000A08C)
  guessed   a body's `// 1000C120: using guessed type int n4096;` comment
  string    an initialised char array found byte-for-byte in .vbn, between the
            addresses of its resolved neighbours in declaration order
  disasm    the one absolute data operand, between those neighbours, shared by
            every function that names the global and references anything there
  ordered   k unresolved globals between two anchors and exactly k referenced
            addresses there, consistent with the declared sizes

Every result is then validated: monotonic in declaration order, no overlap with
the next symbol given the declared size, and referenced by some instruction.
Columns: VA, kind (F function, D data, I IAT slot), name, size, type [source].
"""
import bisect, os, re, sys
from collections import Counter
from common import (ROOT, CODE_LO, DATA_LO, DATA_HI, donor_lines, load_funcs,
                    section_bytes, disassemble)

SRC = donor_lines()
IMG_VA, IMG = section_bytes('.vbn')
FUNCS = load_funcs()
fstarts = sorted(f['addr'] for f in FUNCS if f['addr'] >= CODE_LO)

refs_by_func, allrefs = {}, set()
for va, text in disassemble():
    k = bisect.bisect_right(fstarts, va) - 1
    if k < 0 or va >= 0x14000000:
        continue
    for h in re.findall(r'0x([0-9a-f]{6,8})', text.split('\t')[-1]):
        a = int(h, 16)
        if DATA_LO <= a < DATA_HI:
            refs_by_func.setdefault(fstarts[k], set()).add(a)
            allrefs.add(a)

guessed = {}
for l in SRC:
    m = re.match(r'// ([0-9A-F]+): using guessed type (.*?)([A-Za-z_]\w*)(\[[^\]]*\])?;$', l)
    if m:
        guessed.setdefault(m.group(3), set()).add(int(m.group(1), 16))

# data declarations, in order (initialisers may span lines)
decl_lines, in_data, i = [], False, 0
while i < len(SRC):
    l = SRC[i]
    if l.startswith('// Data declarations'):
        in_data = True
    elif in_data and l.startswith('//----- ('):
        break
    elif in_data and l and not l[0].isspace() and not l.startswith('//') and l not in ('{', '}', '};'):
        full = l
        if l.rstrip().endswith('=') or l.count('{') > l.count('}'):
            j = i + 1
            while j < len(SRC) and not SRC[j].startswith('}'):
                full += ' ' + SRC[j].strip()
                j += 1
            if j < len(SRC):
                full += ' ' + SRC[j].strip()
            i = j
        decl_lines.append(full)
    i += 1

TSIZE = {'char': 1, '_BYTE': 1, 'unsigned __int8': 1, '__int8': 1, '_UNKNOWN': 1, '__int16': 2,
         '_WORD': 2, 'wchar_t': 2, 'unsigned __int16': 2, 'int': 4, '_DWORD': 4, 'unsigned int': 4,
         '__int32': 4, 'volatile __int32': 4, 'float': 4, '__int64': 8, '_QWORD': 8, 'double': 8}


def literal(init):
    try:
        txt = eval(init.lstrip('L'))
    except Exception:
        return None
    if init.startswith('L'):
        return txt.encode('utf-16-le') + b'\0\0'
    return txt.encode('latin-1') + b'\0'


def parse_decl(s):
    s = re.sub(r'//.*$', '', s).strip().rstrip(';').strip()
    init = None
    if '=' in s:
        s, init = s.split('=', 1)
        s, init = s.strip(), init.strip()
    m = re.match(r'.*\(\s*(?:__\w+\s*)?\*\s*([A-Za-z_]\w*)\s*\)\s*\(.*\)$', s)
    if m:
        return dict(name=m.group(1), type='void *', size=4, init=init)
    m = re.match(r'(.*?)([A-Za-z_0-9]\w*)\s*((?:\[[^\]]*\])*)$', s)
    if not m:
        return None
    typ, name, dims = m.group(1).strip(), m.group(2), m.group(3)
    esz = 4 if typ.endswith('*') else TSIZE.get(typ, None)
    count = 1
    for dm in re.findall(r'\[([^\]]*)\]', dims):
        try:
            count = count * int(dm, 0) if dm.strip() and count is not None else None
        except ValueError:
            count = None
    size = None
    if esz is not None:
        if count is not None:
            size = esz * count
        elif init and (init.startswith('"') or init.startswith('L"')):
            b = literal(init)
            size = len(b) if b else None
        elif init and init.startswith('{'):
            size = esz * len([x for x in init.strip('{} ').split(',') if x.strip()])
    return dict(name=name, type=typ + dims, size=size, init=init)


decls = [d for d in map(parse_decl, decl_lines) if d]
decls = [d for d in decls if not re.match(r'(?:sub|nullsub|j_nullsub)_[0-9A-F]+$', d['name'])]

addr, why = {}, {}


def setaddr(n, a, w):
    if n in addr and addr[n] != a:
        sys.exit(f'conflict for {n}: {addr[n]:08X} ({why[n]}) vs {a:08X} ({w})')
    addr[n], why[n] = a, w


for d in decls:
    m = re.match(r'(?:dword|word|byte|off|unk|asc|stru|qword|xmmword|flt|dbl)_([0-9A-F]{8})$', d['name'])
    if m:
        setaddr(d['name'], int(m.group(1), 16), 'name')
    g = guessed.get(d['name'])
    if g and len(g) == 1:
        setaddr(d['name'], next(iter(g)), 'guessed')


def bounds(idx):
    lo, hi = DATA_LO, DATA_HI
    for j in range(idx - 1, -1, -1):
        if decls[j]['name'] in addr:
            lo = addr[decls[j]['name']] + (decls[j]['size'] or 1)
            break
    for j in range(idx + 1, len(decls)):
        if decls[j]['name'] in addr:
            hi = addr[decls[j]['name']]
            break
    return lo, hi


for _ in range(5):
    for idx, d in enumerate(decls):
        if d['name'] in addr or not d['init'] or d['init'][:2] not in ('"', 'L"') and d['init'][0] != '"':
            continue
        b = literal(d['init'])
        if not b:
            continue
        lo, hi = bounds(idx)
        hits, k = [], IMG.find(b)
        while k >= 0:
            if lo <= IMG_VA + k < hi:
                hits.append(IMG_VA + k)
            k = IMG.find(b, k + 1)
        if len(hits) == 1:
            setaddr(d['name'], hits[0], 'string')

uses = {}
for f in FUNCS:
    body = '\n'.join(SRC[f['start'] - 1:f['end']])
    for d in decls:
        if re.search(r'(?<!\w)' + re.escape(d['name']) + r'(?!\w)', body):
            uses.setdefault(d['name'], set()).add(f['addr'])

for _ in range(10):
    changed = False
    for idx, d in enumerate(decls):
        if d['name'] in addr:
            continue
        lo, hi = bounds(idx)
        cands = None
        for fa in uses.get(d['name'], ()):
            r = {a for a in refs_by_func.get(fa, ()) if lo <= a < hi}
            if r:                              # empty: a local or parameter of the same name
                cands = r if cands is None else cands & r
        if cands and len(cands) == 1:
            setaddr(d['name'], min(cands), 'disasm')
            changed = True
    if not changed:
        break

idx = 0
while idx < len(decls):
    if decls[idx]['name'] in addr:
        idx += 1
        continue
    j = idx
    while j < len(decls) and decls[j]['name'] not in addr:
        j += 1
    lo, hi = bounds(idx)
    group, pool = decls[idx:j], set()
    for d in group:
        for fa in uses.get(d['name'], ()):
            pool |= {a for a in refs_by_func.get(fa, ()) if lo <= a < hi}
    pool = sorted(pool)
    if len(pool) == len(group) and all(
            pool[k] + (group[k]['size'] or 1) <= (pool[k + 1] if k + 1 < len(pool) else hi)
            for k in range(len(group))):
        for d, a in zip(group, pool):
            setaddr(d['name'], a, 'ordered')
    idx = j

unres = [d['name'] for d in decls if d['name'] not in addr]
if unres:
    sys.exit('unresolved: ' + ' '.join(unres))
rows = sorted((addr[d['name']], d['name'], d['size']) for d in decls)
prev = 0
for d in decls:
    if addr[d['name']] < prev:
        sys.exit(f"not monotonic at {d['name']}")
    prev = addr[d['name']]
for (a, n, sz), (b, m, _) in zip(rows, rows[1:]):
    if sz and a + sz > b:
        sys.exit(f'{n}@{a:08X}+{sz} overlaps {m}@{b:08X}')
unref = [n for a, n, _ in rows if a not in allrefs]
if unref:
    sys.exit('never referenced by code: ' + ' '.join(unref))

with open(os.path.join(ROOT, 'vbn.txt'), 'w') as f:
    f.write('# VA\tkind\tname\tsize\ttype [source]   (generated by tools/mksyms.py; see its docstring)\n')
    out = [(x['addr'], 'F', x['name'], '', x['sig']) for x in FUNCS]
    out += [(addr[d['name']], 'D', d['name'], str(d['size'] or ''), f"{d['type']} [{why[d['name']]}]")
            for d in decls]
    out += [(0x0FFF3060, 'I', 'vbn_start', '4', 'IAT slot: vbnshim.dll!vbn_start'),
            (0x0FFF3064, 'I', 'vbn_interrupt', '4', 'IAT slot: vbnshim.dll!vbn_interrupt')]
    for r in sorted(out):
        f.write('%08X\t%s\t%s\t%s\t%s\n' % r)
print(f'{len(decls)} globals resolved:', dict(Counter(why.values())))
