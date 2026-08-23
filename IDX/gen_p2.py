#!/usr/bin/env python3
# gen_p2.py -- one-time generator for the P2 module: everything in
# bmf_tables.inc that belongs to the alt-P2 predictor, plus the scalar rates.
#
#   python3 IDX/gen_p2.py
#
# writes IDX/bmf-P2.idx and the consumer glue printed on stdout for pasting
# into bmf_params.inc.  After this the .idx is the source of truth.
import sys

sys.path.insert(0, 'IDX')
from f2idx import dyadic

# ---- the current values, transcribed from bmf_tables.inc -------------------
coef = [[-0.05, -0.07, 0.3, 0.18], [-0.03, 0.04, 0.06, -0.01],
        [0.06, 0.16, 0.0, -0.01], [-0.1, -0.02, 0.05, 0.05],
        [0.06, 0.0, 0.04, 0.02], [0.04, 0.02, 0.04, 0.05],
        [0.07, 0.0, 0.03, 0.02]]
rate = [[0.0108, 0.0069, 0.0054, 0.0052], [0.0042, 0.0042, 0.0042, 0.0039],
        [0.0035, 0.0035, 0.0028, 0.0028], [0.0027, 0.0026, 0.0026, 0.0024],
        [0.0023, 0.0023, 0.0022, 0.0021], [0.0019, 0.0019, 0.0017, 0.0017],
        [0.0015, 0.0015, 0.0011, 0.0009]]
mix = [[0.4, 0.1, 0.15, 0.23, 0.04, 0.03], [0.26, 0.23, 0.13, 0.12, 0.15, 0.12],
       [0.27, 0.22, 0.26, 0.08, 0.12, 0.05], [0.3, 0.14, 0.08, 0.22, 0.18, 0.09]]
scalars = [('rate_reset', 0.0024), ('ms_rate', 0.023), ('decay', 0.78),
           ('seed', 0.19)]

ctx_edges = [17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0]
len_edges = [4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120] + [0] * 17
ctx_rotate = [4, 8, 12, 8]
b1_reload = [7, 46, 197]
band_edges = [43, 17, 9, 5, 2]
thresholds = [
    [6, 14, 28, 4, 8, 26, 4096, 6144, 13056, 10240, 14336, 26624, 36864],
    [4, 15, 42, 1, 8, 19, 2560, 6912, 11264, 10240, 28672, 71680, 139264],
    [5, 14, 21, 4, 9, 25, 3840, 8448, 12032, 16384, 26624, 43008, 83968],
    [6, 11, 20, 5, 11, 23, 2816, 5888, 10496, 22528, 32768, 51200, 131072],
    [0, 11, 30, 7, 13, 22, 4352, 8448, 14848, 16384, 38912, 57344, 94208],
    [7, 10, 31, -1, 11, 21, -256, 1024, 5888, -2048, -2048, -2048, 145408],
]

idx, glue = [], []


def num(name, v, comment=''):
    """An integer parameter.  A negative value rides on mult=-1 rather than a
    bias: the optimizer then moves the magnitude and leaves the sign alone,
    which is what a signed threshold wants."""
    mult = -1 if v < 0 else 1
    q = abs(v)
    c = ('   # %s' % comment) if comment else ''
    idx.append('Number %-13s %2d,0!%s%s' % (name + ',', mult, format(q, 'b'), c))


def flt(name, v):
    """A float parameter, stored as the exact dyadic mantissa.  Returns the
    binary exponent the consumer divides by."""
    m, k = dyadic(v)
    mult = -1 if m < 0 else 1
    idx.append('Number %-13s %2d,0!%-24s # %s' %
               (name + ',', mult, format(abs(m), 'b'), repr(v)))
    return k


idx += ['', 'Prefix P2', 'Debug 1', '']
idx += ['# ---------------------------------------------------------------------',
        '# alt-P2: the 28-tap NLMS predictor cascade.',
        '#',
        '# Every float here is stored as the exact mantissa of the float32 it',
        '# replaces, with its own binary exponent applied by the consumer, so the',
        '# port reproduces the pre-port stream byte for byte.  A round fixed-point',
        '# scale would have moved every rate by a fraction of a per cent.',
        '# ---------------------------------------------------------------------']

idx += ['', '# NLMS seed coefficients, one per tap row/column.', 'Const 0', '']
ck = []
for r, row in enumerate(coef):
    ck.append([flt('coef_%d%d' % (r, c), v) for c, v in enumerate(row)])

idx += ['', '# Per-tap adaptation rates.  The cascade is ordered fast to slow.', '']
rk = []
for r, row in enumerate(rate):
    rk.append([flt('rate_%d%d' % (r, c), v) for c, v in enumerate(row)])

idx += ['', '# Blend weights over the six cascade outputs, one set per mode.', '']
mk = []
for r, row in enumerate(mix):
    mk.append([flt('mix_%d%d' % (r, c), v) for c, v in enumerate(row)])

idx += ['', '# Scalar rates: the reset rate, the mean-square rate, the trace decay', '# and the initial weight.', '']
sk = [(n, flt(n, v)) for n, v in scalars]

idx += ['', '# ---------------------------------------------------------------------',
        '# Context quantisers.  These are thresholds, not sizes, so they stay live.',
        '# ---------------------------------------------------------------------', '']
idx += ['# Delta-context edges: the ladder the P2 context level climbs.', '']
for i, v in enumerate(ctx_edges[:15]):
    num('ctx_edge_%02d' % i, v)
idx += ['', '# Run-length context edges.', '']
for i, v in enumerate(len_edges[:15]):
    num('len_edge_%02d' % i, v)
idx += ['', '# Band edges, and the four-way threshold sets they select.', '']
for i, v in enumerate(band_edges):
    num('band_edge_%d' % i, v)
idx.append('')
for b, row in enumerate(thresholds):
    for i, v in enumerate(row):
        num('thr_%d_%02d' % (b, i), v)
    idx.append('')

idx += ['# The bank rotation and the b1 reload ladder.', '']
for i, v in enumerate(ctx_rotate):
    num('rotate_%d' % i, v)
idx.append('')
for i, v in enumerate(b1_reload):
    num('b1_reload_%d' % i, v)
idx.append('')

open('IDX/bmf-P2.idx', 'w').write('\n'.join(idx) + '\n')

# ---- consumer glue --------------------------------------------------------
g = glue.append
g('// alt-P2 seed coefficients, rates and blend weights: IDX integers read back')
g('// as the floats they encode.  idx_f is exact here -- every mantissa is under')
g('// 2^24 and every exponent under 64 -- so these are the same float32 values')
g('// the table held before the port.')
for name, tab, ks, dims in (('bmf_p2_coef_init', coef, ck, '[7][4]'),
                            ('bmf_p2_rate_init', rate, rk, '[7][4]'),
                            ('bmf_p2_mix', mix, mk, '[4][6]')):
    short = name.split('_')[-2] if name.endswith('_init') else 'mix'
    g('')
    g('alignas(16) static const float %s%s = {' % (name, dims))
    for r, row in enumerate(tab):
        cells = ['idx_f(P2_%s_%d%d, %d)' % (short, r, c, ks[r][c])
                 for c in range(len(row))]
        g('  {' + ', '.join(cells) + '},')
    g('};')
for n, k in sk:
    g('const float bmf_p2_%s = idx_f(P2_%s, %d);' % (n, n, k))

print('\n'.join(glue))
