#!/usr/bin/env python3
# gen_p2b.py -- second half of the P2 port: the constants still written as
# literals inside alt_p2.inc.  Appends them to IDX/bmf-P2.idx and rewrites the
# use sites to name them.  One-time; the .idx is the source of truth afterwards.
import re
import sys

sys.path.insert(0, 'IDX')
from f2idx import dyadic

# (idx name, literal as it appears in the source, replacement expression)
FLOATS = [
    # NbRow::predict seeds the three scratch slots the covariance blend uses.
    ('cov_seed', '47.0f', None),
    ('var_seed', '169.2f', None),
    ('msscale_seed', '1.0f', None),
    # seat_nb_row: the error gain and the normaliser floor of the seating step.
    ('seat_gain', '2.0999999f', None),
    ('seat_floor', '7744.0f', None),
    # nlms_track_two_rows: the fast and slow rows' floors and rates.
    ('floor_fast', '26896.0f', None),
    ('floor_slow', '5041.0f', None),
    ('rate_slow', '0.013f', None),
    ('ms_rate_fast', '0.05f', None),
    # nlms_predict_and_correct: the normaliser floor, and the mean-square scale's
    # own target and approach rate.
    ('ms_floor', '529.0f', None),
    ('msscale_target', '10.0f', None),
    ('msscale_rate', '0.00019999999f', None),
    # The covariance/variance EMAs that set how much of the coarse view is kept.
    ('cov_rate', '0.001f', None),
    ('var_rate', '0.001f', None),
    ('var_floor', '0.000099999997f', None),
    ('cov_keep', '0.1f', None),
    ('conf_floor', '576.0f', None),
    ('conf_gain', '2.0f', None),
    ('mix_gain', '2.5999999f', None),
]

# Integer thresholds, by use site.
INTS = [
    ('filt_thr_0', 3536), ('filt_thr_1', 720), ('filt_thr_2', 288),
    ('g3_thr_0', 752), ('g3_thr_1', 400), ('g3_thr_2', 240),
    ('rb_bias', 208),
]
QUANT = [
    ('q0a', (2256, 1056, 144, 55, 10, 24)),
    ('q0b', (2400, 1024, 240, 39, 24, 11)),
    ('q1a', (2576, 1280, 640, 33, 12, 4)),
    ('q1b', (2464, 1216, 688, 58, 25, 13)),
    ('q2', (2896, 1568, 592, 37, 19, 9)),
    ('q3', (3056, 1952, 368, 39, 21, 10)),
]

lines = ['', '# ---------------------------------------------------------------------',
         '# The NLMS cascade\'s own rates, floors and gains -- the numbers that were',
         '# written into alt_p2.inc as float literals.',
         '# ---------------------------------------------------------------------', '']
ks = {}
for name, lit, _ in FLOATS:
    v = float(lit.rstrip('f'))
    m, k = dyadic(v)
    ks[name] = k
    mult = -1 if m < 0 else 1
    lines.append('Number %-15s %2d,0!%-24s # %s' %
                 (name + ',', mult, format(abs(m), 'b'), lit))

lines += ['', '# The per-bank context quantisers: three run-length edges and three',
          '# activity edges apiece, one set per bank and reference state.', '']
for name, v in INTS:
    lines.append('Number %-15s  1,0!%s' % (name + ',', format(v, 'b')))
lines.append('')
for tag, vals in QUANT:
    for i, v in enumerate(vals):
        who = ('r%d' % i) if i < 3 else ('s%d' % (i - 3))
        lines.append('Number %-15s  1,0!%s' % ('%s_%s,' % (tag, who), format(v, 'b')))
    lines.append('')

open('IDX/bmf-P2.idx', 'a').write('\n'.join(lines) + '\n')

# ---- rewrite the use sites ------------------------------------------------
src = open('alt_p2.inc').read()


def sub(old, new, count=1):
    global src
    n = src.count(old)
    assert n == count, 'expected %d of %r, found %d' % (count, old, n)
    src = src.replace(old, new)


F = lambda n: 'idx_f(P2_%s, %d)' % (n, ks[n])

sub('w[14][0] = 47.0f;', 'w[14][0] = %s;' % F('cov_seed'))
sub('w[14][1] = 169.2f;', 'w[14][1] = %s;' % F('var_seed'))
sub('w[14][2] = 1.0f;', 'w[14][2] = %s;' % F('msscale_seed'), 2)
sub('+bias[0]))*2.0999999f;', '+bias[0]))*%s;' % F('seat_gain'))
sub('const float floor_ = 7744.0f*', 'const float floor_ = %s*' % F('seat_floor'))
sub('const float floor_fast = 26896.0f*', 'const float floor_fast = %s*' % F('floor_fast'))
sub('const float floor_slow = 5041.0f*', 'const float floor_slow = %s*' % F('floor_slow'))
sub('const float rate_slow = 0.013f*conf;', 'const float rate_slow = %s*conf;' % F('rate_slow'))
sub('nlms_step(fast, j, k, x, err_fast, 0.05f, floor_fast);',
    'nlms_step(fast, j, k, x, err_fast, %s, floor_fast);' % F('ms_rate_fast'))
sub('+ms_scale*529.0f);', '+ms_scale*%s);' % F('ms_floor'))
sub('ms_scale+((10.0f-ms_scale)*0.00019999999f);',
    'ms_scale+((%s-ms_scale)*%s);' % (F('msscale_target'), F('msscale_rate')))
sub('-wrow_cur[0].w[14][0])*0.001f)', '-wrow_cur[0].w[14][0])*%s)' % F('cov_rate'))
sub('-wrow_cur[0].w[14][1])*0.001f)', '-wrow_cur[0].w[14][1])*%s)' % F('var_rate'))
sub('w[14][1]+0.000099999997f)', 'w[14][1]+%s)' % F('var_floor'))
sub('float cov_kept = 0.1f*var;', 'float cov_kept = %s*var;' % F('cov_keep'))
sub('(var+576.0f)))*2.0f;', '(var+%s)))*%s;' % (F('conf_floor'), F('conf_gain')))
sub('(sample-mixed)*2.5999999f,', '(sample-mixed)*%s,' % F('mix_gain'))

sub('(filt>3536)+(filt>720)+(filt>288)',
    '(filt>P2_filt_thr_0)+(filt>P2_filt_thr_1)+(filt>P2_filt_thr_2)')
sub('((g3pair+g3sum)>752)+((g3pair+g3sum)>400)+((g3pair+g3sum)>240)',
    '((g3pair+g3sum)>P2_g3_thr_0)+((g3pair+g3sum)>P2_g3_thr_1)+((g3pair+g3sum)>P2_g3_thr_2)')
sub('bit_of<15>(208-rb0[0].val)', 'bit_of<15>(P2_rb_bias-rb0[0].val)')
for tag, vals in QUANT:
    old = 'ctx_quant(%s, %d, %d, %d, sum_all, %d, %d, %d)' % (
        {'q0a': 'run0', 'q0b': 'run0', 'q1a': 'run1', 'q1b': 'run1',
         'q2': 'run2', 'q3': 'run3'}[tag], *vals)
    new = 'ctx_quant(%s, P2_%s_r0, P2_%s_r1, P2_%s_r2, sum_all, P2_%s_s0, P2_%s_s1, P2_%s_s2)' % (
        {'q0a': 'run0', 'q0b': 'run0', 'q1a': 'run1', 'q1b': 'run1',
         'q2': 'run2', 'q3': 'run3'}[tag], tag, tag, tag, tag, tag, tag)
    sub(old, new)

open('alt_p2.inc', 'w').write(src)
print('alt_p2.inc rewritten; %d floats, %d ints, %d quantiser sets appended' %
      (len(FLOATS), len(INTS), len(QUANT)))
