#!/usr/bin/env python3
"""Give every blob global its own definition, and see what breaks.

    python3 tools/deblob.py subs1.hpp blob.inc            # report
    python3 tools/deblob.py subs1.hpp blob.inc --apply    # rewrite

`blob.inc` is BMF.exe's data segment as one array, and every global is a
reference into it at its original offset.  That is faithful, but it means the
program still depends on where the globals sit relative to each other, and
`REFACTORING.md` §4.1 is what that costs.

This replaces the array with one definition per global, each initialised from
the bytes it had, each followed by a guard gap so the linker cannot put them
back together by accident.  A global's extent is the distance to the next one,
because the declared `[0x10000]` bounds are Hex-Rays guessing and there is no
way to tell from the source which of them are real.

**That is the point.**  If the program still produces the same ten streams
afterwards, the globals are independent and the guesses did not matter.  If it
does not, the failure names the place where one global's array really does run
into the next, which is the one thing static analysis could not answer.

The 39 absolute pointers `blob1_relocs` lists are rebased the same way they
already were, only against the new addresses -- see bmf_data_relocate() in the
generated block.
"""
import re
import sys


def read_blob(path):
    text = open(path).read()
    m = re.search(r'unsigned char blob1\[BMF_BLOB_LEN\] = \{(.*?)\n\};', text, re.S)
    data = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', m.group(1)))
    m = re.search(r'blob1_relocs\[\] = \{(.*?)\};', text, re.S)
    relocs = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]+)', m.group(1))]
    base = int(re.search(r'#define BMF_BLOB_BASE (0x[0-9A-Fa-f]+)', text).group(1), 16)
    return data, relocs, base


def globals_of(text):
    out = []
    for m in re.finditer(
            r'^typedef ([A-Za-z_][A-Za-z0-9_ ]*?) (t_[A-Za-z0-9_]+)(?:\[([^\]]+)\])?;\n'
            r'static t_[A-Za-z0-9_]+& ([A-Za-z_][A-Za-z0-9_]*) = '
            r'\*\(t_[A-Za-z0-9_]+\*\)\(blob1 \+ (0x[0-9A-F]+) - BMF_BLOB_BASE\);\n', text, re.M):
        out.append({'elem': m.group(1), 'typedef': m.group(2), 'bound': m.group(3),
                    'name': m.group(4), 'va': int(m.group(5), 16), 'span': m.span()})
    out.sort(key=lambda g: g['va'])
    return out


def main():
    src, inc = sys.argv[1], sys.argv[2]
    apply_ = '--apply' in sys.argv
    text = open(src).read()
    data, relocs, base = read_blob(inc)
    gs = globals_of(text)
    if not gs:
        sys.exit('no blob globals left')

    end = base + len(data)
    for i, g in enumerate(gs):
        g['bytes'] = (gs[i + 1]['va'] if i + 1 < len(gs) else end) - g['va']
    lead = gs[0]['va'] - base

    print('%d globals, %d bytes of data, %d before the first global'
          % (len(gs), len(data), lead))
    print('%d relocations' % len(relocs))
    big = sorted(gs, key=lambda g: -g['bytes'])[:5]
    print('largest extents: %s'
          % ', '.join('%s %d' % (g['name'], g['bytes']) for g in big))
    if not apply_:
        print('\n(--apply to rewrite)')
        return

    GAP = int(next((a.split("=")[1] for a in sys.argv if a.startswith("--gap=")), 64))
    ZERO_GUARD = '--zero-guard' in sys.argv
    lines = ['// ---------------------------------------------------------------',
             '// BMF.exe\'s data segment, one definition per global.',
             '//',
             '// Each carries the bytes it had, and %d bytes of guard after it so' % GAP,
             '// that a global running past its own end lands in padding rather than',
             '// in its neighbour.  The extent is the distance to the next global:',
             '// the [0x10000] bounds Hex-Rays emitted are guesses and there is no',
             '// way to tell from the source which are real.  REFACTORING.md §4.1.',
             '// ---------------------------------------------------------------']
    for g in gs:
        off = g['va'] - base
        # The guard carries the bytes that followed this global in the data
        # segment, not zeros.  A global that reads past its own end then sees
        # what it always saw, and a run that still works says the extents are
        # wrong but the neighbours' *contents* are what matter -- while a run
        # that works with zeros says they do not matter at all.
        raw = data[off:off + g['bytes'] + (0 if ZERO_GUARD else GAP)]
        raw = raw + bytes(g['bytes'] + GAP - len(raw))
        body = ','.join('0x%02x' % b for b in raw)
        wrapped, cur = [], '  '
        for tok in body.split(','):
            if len(cur) + len(tok) + 1 > 92:
                wrapped.append(cur)
                cur = '  '
            cur += tok + ','
        wrapped.append(cur)
        lines.append('alignas(16) static uint8_t bmf_%s[%d + %d] = {   // 0x%06X'
                     % (g['name'].lstrip('_'), g['bytes'], GAP, g['va']))
        lines += wrapped
        lines.append('};')
        decl = 'typedef %s %s%s;' % (g['elem'], g['typedef'],
                                     '[%s]' % g['bound'] if g['bound'] else '')
        lines.append(decl)
        lines.append('static %s& %s = *(%s*)bmf_%s;'
                     % (g['typedef'], g['name'], g['typedef'], g['name'].lstrip('_')))

    lines += ['',
              '// The absolute pointers BMF.exe baked into its data, rebased onto',
              '// wherever the linker put the definitions above.',
              'static const struct { unsigned va, slot_va; } bmf_relocs[] = {']
    for r in relocs:
        lines.append('  { 0, 0x%08X },' % (base + r))
    lines += ['};',
              'static void bmf_data_relocate()',
              '{',
              '  for (unsigned i = 0; i < sizeof bmf_relocs / sizeof *bmf_relocs; i++) {',
              '    unsigned char *slot = bmf_addr(bmf_relocs[i].slot_va);',
              '    unsigned va;',
              '    __builtin_memcpy(&va, slot, 4);',
              '    unsigned char *p = bmf_addr(va);',
              '    __builtin_memcpy(slot, &p, sizeof p);',
              '  }',
              '}']

    # bmf_addr: VA -> the byte inside whichever definition owns it
    addr = ['static unsigned char *bmf_addr(unsigned va)',
            '{',
            '  switch (0) { default: break; }']
    for g in gs:
        addr.append('  if (va >= 0x%06Xu && va < 0x%06Xu) return bmf_%s + (va - 0x%06Xu);'
                    % (g['va'], g['va'] + g['bytes'], g['name'].lstrip('_'), g['va']))
    addr += ['  return nullptr;', '}']

    # bmf_addr has to come after the definitions it names.
    at = lines.index('')
    block = '\n'.join(lines[:at] + [''] + addr + lines[at:]) + '\n'
    out = text[:gs[0]['span'][0]] + block + text[gs[0]['span'][0]:]
    for g in reversed(gs):
        out_i = out.index(text[g['span'][0]:g['span'][1]])
        out = out[:out_i] + out[out_i + (g['span'][1] - g['span'][0]):]
    open(src, 'w').write(out)
    print('rewritten: %d definitions' % len(gs))


if __name__ == '__main__':
    main()
