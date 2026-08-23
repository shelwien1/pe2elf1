#!/usr/bin/env python3
# gen_qz.py -- port the five monotone quantiser ladders to IDX Index
# declarations, the format's own mechanism for them.
#
# A ladder was seven or fifteen integers walked incrementally to fill a lookup
# table.  As an IDX threshold mapping it is one bit string over the ladder's
# whole input range, with a 1 wherever an edge sits -- so the optimizer moves
# edges, adds them and removes them, instead of nudging a fixed count of them.
# The value is unchanged: a threshold mapping returns the number of edges below
# its argument, which is exactly what the incremental walk computed.
import sys

sys.path.insert(0, 'IDX')

LADDERS = [
    ('Level', 'ql', 512, [1, 2, 4, 8, 14, 35, 103],
     ['alt-P1 activity level: the ladder the residual model climbs as the',
      'neighbourhood gets busier.  Input is the rounded activity sum, 0..511.']),
    ('Group', 'qg', 512, [1, 3, 6, 10, 16, 27, 52],
     ['alt-P1 activity group: a second, finer ladder over the same input,',
      'carried in a different field of the same context.']),
    ('Slot', 'qs', 256, [5, 10, 36, 98, 154, 236, 248],
     ['alt-P1 predicted-value slot: a ladder over the MED prediction itself,',
      'so the model can separate dark, midtone and bright neighbourhoods.']),
    ('P2Ctx', 'qc', 260, [17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228,
                          235, 236, 237],
     ['alt-P2 delta context: sixteen buckets over the accumulated delta index.']),
    ('P2Len', 'qn', 121, [4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87,
                          120],
     ['alt-P2 run length: sixteen buckets over the neighbourhood run counter.']),
]

idx = ['', 'Prefix QZ', 'Debug 1', 'Const 0', '',
       '# ---------------------------------------------------------------------',
       '# The monotone quantiser ladders, as threshold mappings.',
       '#',
       '# Each pattern spans its ladder\'s whole input range, one character per',
       '# input value, with a 1 wherever an edge sits.  The mapping returns how',
       '# many edges lie below its argument -- the same number the hand-walked',
       '# ladder produced, and the reason the port is bit-exact.',
       '#',
       '# These are wide: 1661 pattern bits between them, more than twice what',
       '# the rest of the codec declares.  That is the point -- the optimizer can',
       '# move an edge, drop one, or add one anywhere in the range rather than',
       '# nudging a fixed seven.  It is also a lot of search, so freezing the',
       '# ones with little leverage on the corpus in front of you (a ! at the',
       '# start of the line, IDX-FORMAT.md sec.10) is the first thing to reach',
       '# for when a pass is taking too long.',
       '# ---------------------------------------------------------------------']

for name, tag, span, edges, note in LADDERS:
    pattern = ''.join('1' if j in edges else '0' for j in range(span))
    idx.append('')
    for ln in note:
        idx.append('# %s' % ln)
    idx.append('Index %s' % name)
    idx.append(' %s: x, 1!%s' % (tag, pattern))
idx.append('')
open('IDX/bmf-QZ.idx', 'w').write('\n'.join(idx) + '\n')

tpl = ['// bmf-QZ.inc -- the template idx2inc.pl expands into MOD/bmf-QZ_p.inc.',
       '// The mapping declarations go to MOD/bmf-QZ_h.inc; these are the builders.',
       '']
for name, tag, span, edges, note in LADDERS:
    tpl.append('uint32_t Make%s( uint32_t x ) {' % name)
    tpl.append('  int32_t %s;' % name)
    tpl.append('  MakeIndex %s' % name)
    tpl.append('  return (uint32_t)%s;' % name)
    tpl.append('}')
    tpl.append('')
open('IDX/bmf-QZ.inc', 'w').write('\n'.join(tpl))

print('QZ: %d ladders, %d pattern bits' %
      (len(LADDERS), sum(s for _, _, s, _, _ in LADDERS)))
