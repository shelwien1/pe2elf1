#!/usr/bin/env python3
"""mkidx.py NUMDIR OUTDIR -- dtsdpack's first IDX/l9/*.idx: each family's indices over its variables
(famvars.inc), every value of each its own bucket (a threshold at each), and the counters', mixers'
and APMs' parameters taken from ac3pack's tuned level -9 (NUMDIR/num_<its family>.txt). Tuning
moves both from here; this only makes the first declarations."""
import sys
numdir, out = sys.argv[1], sys.argv[2]
only = sys.argv[3:]  # (families to write: all where none)
# each variable's buckets: thresholds at every value from 1 to n-1 (n buckets), or a mask
R = {'fld': '&111111', 'lv': '&111111', 'l2': 3, 'pos': 32, 'st': 3, 'nch': 4,
     'cc': 3, 'bd': 32, 'pq': '&111111', 'ac': 9, 'ab': 28, 'ec': 10,
     'la': 28, 'lb': 28, 'es': '&11111111', 'de': 10, 'ls': 10, 'pm': 2,
     'kd': 3, 'pk': 2, 'pn': 8, 'lp': 10, 'hm': 3,
     'hw': 4, 'nr': 16, 'nl': 16, 'ql': 22, 'm1': 23, 'm2': 23, 'sq': 3, 's1': 3,
     'c1': '&11111111', 'c2': '&11110000', 'code': 3, 'at': 40, 'cl': 257,
     'mg': 9, 'gn': 16, 'lh': 3, 'nb': 10, 'mx': 17, 'sn': 28, 'sd': 3, 'dp': 4, 'nn': 16}
R_SID = dict(R, fld=7)  # (sid's fields: SF_N)
R_ABT = dict(R, st=10)  # (abt's st: a change, 0-9)
R_SCL = dict(R, kd=4)   # (scl's kd: and the LFE's)
R_HIT = dict(R, pm=3)   # (hit's pm: and the LFE)
def pat(fam, v):
    r = (R_SID if fam == 'sid' else R_ABT if fam == 'abt' else R_SCL if fam == 'scl' else R_HIT if fam == 'hit' else R)[v]
    return r if isinstance(r, str) else '1!' + '1' * (r - 1)
FAM = {
  'hdr': ('ac3pack', 'hdr', {
    'a': 'fld', 'b': 'fld lv', 'c': 'fld lv l2', 'd': 'fld pos', 'e': 'fld st nch', 'h': 'fld lv pos',
    'i': 'fld l2 st', 'j': 'fld lv st', 'k': 'fld pos l2', 'l': 'fld nch', 's': 'fld l2', 'f': 'fld',
    'm': 'fld', 'w': 'l2 st', 't': 'fld', 'n': 'fld', 'g': 'fld'},
    'The core frame\'s header, its coding header and its subframes\' headers, and the container\'s\n#  kinds and lengths, each field its own values (fld), against its last values.'),
  'sid': ('ac3pack', 'sid', {
    'a': 'fld lv', 'b': 'fld lv pq', 'c': 'fld bd lv', 'd': 'fld ac lv', 'e': 'fld cc bd', 'h': 'fld ab lv',
    'i': 'fld ec lv', 'j': 'fld pq st', 'k': 'fld bd ac', 'l': 'fld lv ab ac', 's': 'fld lv', 'f': 'fld ac',
    'm': 'fld', 'w': 'st', 't': 'fld', 'n': 'fld', 'g': 'fld'},
    'A subband\'s side information (PMODE, PVQ, TMODE, the joint scale factors, the HF VQ vectors)\n#  and the LFE samples, each field against its last value and the place before\'s.'),
  'abt': ('ac3pack', 'sid', {
    'a': 'bd es', 'b': 'la lb', 'c': 'sn la', 'd': 'sn sd de', 'e': 'bd sn', 'h': 'lb es bd', 'i': 'es nb la',
    'j': 'es de pm', 'k': 'sn nn', 'l': 'bd cc la', 's': 'sn sd nn', 'f': 'es', 'm': 'nn', 'w': 'st', 't': 'bd',
    'n': 'la', 'g': 'bd'},
    'The bit allocations (ABITS), against the last ones, the subband below\'s, and the audio\'s energy.'),
  'scl': ('ac3pack', 'sid', {
    'a': 'kd pk dp', 'b': 'kd pk pn dp', 'c': 'kd pk ab', 'd': 'kd pk lp', 'e': 'kd pk hm', 'h': 'kd pk pn ab',
    'i': 'kd pk pm pn', 'j': 'kd pk bd', 'k': 'kd pk lp pn', 'l': 'kd pk hm ab', 's': 'kd pk pn dp',
    'f': 'kd pk ab', 'm': 'kd pk dp', 'w': 'pn', 't': 'kd pk', 'n': 'kd', 'g': 'kd'},
    'The scale factors, against the one the audio\'s RMS makes (where the WAV is bound), else the last.'),
  'hit': ('ac3pack', 'sid', {
    'a': 'nr nl', 'b': 'nr nl pm', 'c': 'nr hm', 'd': 'nl hm', 'e': 'nr ab', 'h': 'nl ab', 'i': 'nr nl hm',
    'j': 'nr ql', 'k': 'nl ql pm', 'l': 'nr nl ab', 's': 'nr nl', 'f': 'nr nl', 'm': 'nl', 'w': 'nr', 't': 'pm',
    'n': 'pm', 'g': 'pm'},
    'Whether a subband sample\'s index is the one the audio\'s subband sample quantizes to (where the\n#  WAV is bound), and if not, the one on the other side of the rounding boundary.'),
  'smp': ('ac3pack', 'mag', {
    'a': 'pk nr nl', 'b': 'pk ab ql', 'c': 'pk nr hm', 'd': 'pk ab m1', 'e': 'pk ab m1 m2', 'h': 'pk ab nr',
    'i': 'pk ab hw pm', 'j': 'pk ab bd', 'k': 'pk nr pm', 'l': 'pk nl ab', 's': 'pk nr nl', 'f': 'pk nl nr',
    'm': 'pk nr', 'w': 'nl', 't': 'ab', 'n': 'ab', 'g': ''},
    'The subband samples\' quantization indexes, against the index the audio\'s subband sample\n#  quantizes to (where the WAV is bound), else against their neighbours\'.'),
  'vqr': ('ac3pack', 'sid', {
    'a': 'kd mg', 'b': 'kd mg gn', 'c': 'kd bd mg', 'd': 'kd lh mg', 'e': 'kd cc bd', 'h': 'kd gn lh',
    'i': 'kd mg lh gn', 'j': 'kd bd lh', 'k': 'kd cc mg', 'l': 'kd gn', 's': 'kd mg', 'f': 'kd gn',
    'm': 'kd', 'w': 'mg', 't': 'kd', 'n': 'kd', 'g': 'kd'},
    'The ranks of the ADPCM predictors and the HF VQ vectors among the code books\' entries, in the\n#  order of the encoder\'s searches on the audio (where the WAV is bound).'),
  'raw': ('ac3pack', 'raw', {
    'a': 'code', 'b': 'code c1', 'c': 'code c1 c2', 'd': 'code cl', 'e': 'code at', 'h': 'code cl c1',
    'i': 'code c1 at', 'j': 'code cl at', 'k': 'c1 c2', 'l': 'code c1', 's': 'code at', 'f': 'code c1',
    'm': 'code', 'w': 'at', 't': '', 'n': '', 'g': ''},
    'Bytes kept as bytes: between frames, frames that don\'t come back, the bits after the subframes.'),
}
for fam, (src, nf, idx, what) in FAM.items():
    if only and fam not in only: continue
    o = ['#  dtsdpack\'s %s family at level -9: its context indices and its tunable parameters.' % fam,
         '#  IDX/IDX-FORMAT.md is the format; ../../mk.sh regenerates MOD/l9/%s_h.inc and _p.inc from' % fam,
         '#  this file and family.inc.  famvars.inc says what each variable is.', '#',
         '#  ' + what, '#  (The counters\', mixers\' and APMs\' parameters start as ac3pack\'s %s family\'s.)' % nf,
         '', 'Prefix D9_%s' % fam, 'Debug 1', 'Const 0', '']
    for k in 'abcdehijklsfmwtng':
        o.append('Index %s' % k)
        vs = idx[k].split()
        if k == 'n':
            o.append('#  Frozen: the mantissa mixer it is for mixes only by a hint, and nothing gives this family one.')
            for v in vs: o.append('! n_%s: %s, %s' % (v, v, pat(fam, v)))
        else:
            for v in vs: o.append(' %s_%s: %s, %s' % (k, v, v, pat(fam, v)))
        o.append('')
    o += open('%s/num_%s.txt' % (numdir, nf)).read().rstrip('\n').split('\n')
    open('%s/%s.idx' % (out, fam), 'w').write('\n'.join(o) + '\n')
