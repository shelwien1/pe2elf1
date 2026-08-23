#!/usr/bin/env python3
# gen_gl.py -- port the GL (shared limits), MB (slow model) and CD (codec
# decisions) modules.  One-time.
import sys

sys.path.insert(0, 'IDX')
from idxgen import emit, rewrite

# ---------------------------------------------------------------- GL --------
emit('IDX/bmf-GL.idx', 'GL', [
    '# ---------------------------------------------------------------------',
    '# Limits and scales the whole codec shares.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural: array bounds and wire-format masks.'], 1, [
        ('max_width', 0xFFFF), ('no_symbol', 8192),
        ('model_table_bytes', 0x7F000), ('bank_mirror_mask', 0x7FF0),
        ('p2_row_pad', 234),
    ]),
    (['The NLMS weight box, the rescale point of the P2 frequency table, the',
      'third context table\'s occupancy limit, and how much worse than the',
      'fitted blend a degenerate one may be and still win (a fraction of the',
      'fitted cost, capped).'], 0, [
        ('weight_min_neg', 64), ('weight_max', 191),
        ('p2_freq_rescale_total', 29696), ('ctx_id3_limit', 53248),
        ('slack_shift', 7), ('slack_max', 0x4000),
    ]),
])

rewrite('bmf_util.inc', [
    ('''constexpr int32_t kWeightMin = -64, kWeightMax = 191;
constexpr uint32_t kSlackMax = 0x4000;
constexpr int32_t kP2FreqRescaleTotal = 29696;
constexpr int32_t kP2RowPad = 234;
constexpr int32_t kCtxId3Limit = 53248;
constexpr uint32_t kModelTableBytes = 0x7F000u;
constexpr uint32_t kBankMirrorMask = 0x7FF0;''',
     '''// The weight floor is declared as its magnitude: an IDX pattern is unsigned,
// and a sign that can be flipped by the optimizer is not a knob, it is a bug.
const int32_t kWeightMin = -GL_weight_min_neg, kWeightMax = GL_weight_max;
const uint32_t kSlackMax = GL_slack_max;
const int32_t kP2FreqRescaleTotal = GL_p2_freq_rescale_total;
const int32_t kP2RowPad = GL_p2_row_pad;
const int32_t kCtxId3Limit = GL_ctx_id3_limit;
const uint32_t kModelTableBytes = GL_model_table_bytes;
const uint32_t kBankMirrorMask = GL_bank_mirror_mask;''', 1),
    ('constexpr int32_t kMaxWidth = 0xFFFF;', 'const int32_t kMaxWidth = GL_max_width;', 1),
])

rewrite('records.inc', [
    ('constexpr int32_t no_symbol = 8192;', 'constexpr int32_t no_symbol = GL_no_symbol;', 1),
])

# ---------------------------------------------------------------- MB --------
emit('IDX/bmf-MB.idx', 'MB', [
    '# ---------------------------------------------------------------------',
    '# ModelBlock: the original rank-and-escape pixel model.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural: these size members.'], 1, [
        ('row_margin', 7), ('row_tail', 9),
        ('freq_table_offset', 188), ('freq_grid_count', 65723),
        ('sel1_syms', 99), ('sel0_syms', 33),
    ]),
    (['pixel_context: which neighbour ranks count as a near hit and which as a',
      'far one, how deep into a symbol list a membership test looks, and the',
      'rank positions the context switches shape at.'], 0, [
        ('near_lo', 11), ('near_hi', 16), ('far_hi', 32),
        ('exact_rank', 10),
        ('top_depth_0', 10), ('top_depth_1', 4),
        ('top_depth_2', 10), ('top_depth_3', 6),
        ('pos_bare', 6), ('pos_split', 14), ('pos_deep', 9),
    ]),
    (['The map context: how far up the rank ladder it separates positions',
      'before it stops caring, and the alphabet size below which the symbol',
      'cache is keyed differently.'], 0, [
        ('apm_pos_cap', 7), ('small_alphabet', 32),
    ]),
])

rewrite('model.inc', [
    ('  static const int32_t kRowMargin = 7;\n  static const int32_t kRowTail = 9;',
     '  static const int32_t kRowMargin = MB_row_margin;\n'
     '  static const int32_t kRowTail = MB_row_tail;', 1),
    ('  static const int32_t kFreqTableOffset = 188;\n'
     '  static const int32_t kFreqGridCount = 65723;',
     '  static const int32_t kFreqTableOffset = MB_freq_table_offset;\n'
     '  static const int32_t kFreqGridCount = MB_freq_grid_count;', 1),
    ('  static const int32_t kSel1Syms = 99;\n  static const int32_t kSel0Syms = 33;',
     '  static const int32_t kSel1Syms = MB_sel1_syms;\n'
     '  static const int32_t kSel0Syms = MB_sel0_syms;', 1),
    ('    for( band = 11; band<16; ++band )',
     '    for( band = MB_near_lo; band<MB_near_hi; ++band )', 1),
    ('    for( band = 16; band<32; ++band )',
     '    for( band = MB_near_hi; band<MB_far_hi; ++band )', 1),
    ('.bit<6>(result==nb[10])', '.bit<6>(result==nb[MB_exact_rank])', 1),
    ('&&pos>6 )', '&&pos>MB_pos_bare )', 1),
    ('sym_in_top((sel0[cx[0].mode_symbol[1]].ent), 10, result)',
     'sym_in_top((sel0[cx[0].mode_symbol[1]].ent), MB_top_depth_0, result)', 1),
    ('sym_in_top(list_prev, 4, result)', 'sym_in_top(list_prev, MB_top_depth_1, result)', 1),
    ('sym_in_top(list_sym, 10, cx[0].mode_symbol[2])',
     'sym_in_top(list_sym, MB_top_depth_2, cx[0].mode_symbol[2])', 1),
    ('sym_in_top((sel0[cx[0].mode_symbol[3]].ent), 6, result)',
     'sym_in_top((sel0[cx[0].mode_symbol[3]].ent), MB_top_depth_3, result)', 1),
    ('    if( pos<=14||(ctx2&0xB)!=0 ) {', '    if( pos<=MB_pos_split||(ctx2&0xB)!=0 ) {', 1),
    ('.bit<3>(pos>9)', '.bit<3>(pos>MB_pos_deep)', 1),
    ('const int32_t apm_ctx = (int32_t)(sym_pos<7 ? sym_pos : 7)|((ctx_state_seen&15)<<3);',
     'const int32_t apm_ctx = (int32_t)(sym_pos<MB_apm_pos_cap ? sym_pos : MB_apm_pos_cap)|((ctx_state_seen&15)<<3);', 1),
    ('    if( (int32_t)alphabet<32 ) {', '    if( (int32_t)alphabet<MB_small_alphabet ) {', 1),
])

# ---------------------------------------------------------------- CD --------
emit('IDX/bmf-CD.idx', 'CD', [
    '# ---------------------------------------------------------------------',
    '# Codec-level decisions: the numbers that pick a representation rather',
    '# than code a pixel.  They never appear on the wire, but they decide what',
    '# does, so moving one moves the output.',
    '# ---------------------------------------------------------------------',
], [
    (['The cross-plane blend weight scale.  weight_shift and weight_scale are',
      'one number in two forms -- the consumer derives the scale from the shift',
      'so they cannot drift -- and weight_round is the rounding offset applied',
      'before the shift.  weight_bias and weight_bits are the wire field the',
      'fitted weights travel in.'], 0, [
        ('weight_shift', 7), ('weight_round', 40),
        ('weight_bits', 8), ('weight_bias', 64),
    ]),
    (['The least-squares fit\'s ridge term, as the denominator of a unit',
      'fraction.  A ridge of 0.1 is not a dyadic rational, so it cannot go in',
      'as an exact mantissa the way the NLMS constants do; 1.0/10.0 in double',
      'is bit-for-bit the double the literal 0.1 denotes, which a fixed-point',
      'approximation would not be.'], 0, [
        ('fit_ridge_den', 10),
    ]),
    (['The DC window search: how wide a window it starts from on each of the',
      'two histograms, and the sentinel each stops at.'], 0, [
        ('dc_window_lo', 4), ('dc_window_hi', 1024),
        ('dc_diff_lo', 0), ('dc_diff_hi', 512),
    ]),
])

rewrite('codec.inc', [
    ('        const uint32_t slack = umin32(best_cost>>7, kSlackMax);',
     '        const uint32_t slack = umin32(best_cost>>idx_clamp(GL_slack_shift, 1, 24), kSlackMax);', 1),
    ('int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;',
     'int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+CD_weight_round)>>kWeightShift))&0x3FF;', 1),
    ('++hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];',
     '++hist_c[(c0-((uint32_t)((((c1+c2)*kWeightScale)>>1)+CD_weight_round)>>kWeightShift))];', 1),
    ('          wt4 = 128;\n          wt8 = 0;',
     '          wt4 = kWeightScale;\n          wt8 = 0;', 1),
    ('          wt4 = 0;\n          wt8 = 128;',
     '          wt4 = 0;\n          wt8 = kWeightScale;', 1),
    ('          wt4 = 64;\n          wt8 = 64;',
     '          wt4 = kWeightScale/2;\n          wt8 = kWeightScale/2;', 1),
    ('static const int32_t degenerate[3][2] = {{128, 0}, {0, 128}, {64, 64}};',
     'const int32_t degenerate[3][2] = {{kWeightScale, 0}, {0, kWeightScale},\n'
     '                                            {kWeightScale/2, kWeightScale/2}};', 1),
    ('      code_field<f_DEC>(d.weight0, 8, 64);\n      code_field<f_DEC>(d.weight1, 8, 64);',
     '      code_field<f_DEC>(d.weight0, CD_weight_bits, CD_weight_bias);\n'
     '      code_field<f_DEC>(d.weight1, CD_weight_bits, CD_weight_bias);', 1),
    ('        code_field<f_DEC>(d.weight2, 8, 64);',
     '        code_field<f_DEC>(d.weight2, CD_weight_bits, CD_weight_bias);', 1),
    ('    double inv = 128.0/(0.1-sxy*sxy+sxx*syy);',
     '    double inv = (double)kWeightScale/((1.0/(double)idx_clamp(CD_fit_ridge_den, 1, 1<<20))-sxy*sxy+sxx*syy);', 1),
    ('int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;',
     'int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)((((dx+dy)*kWeightScale)>>1)+CD_weight_round)>>kWeightShift)-512)&0x3FF;', 1),
    ('    if( t.mode==2&&t.wgt0+t.wgt1==128 ) {',
     '    if( t.mode==2&&t.wgt0+t.wgt1==kWeightScale ) {', 1),
    ('widest_window(hists+1024*pred, 4, 1024, -1)+1',
     'widest_window(hists+1024*pred, CD_dc_window_lo, CD_dc_window_hi, -1)+1', 1),
    ('widest_window(hist_c+1024, 0, 512, 255)+1',
     'widest_window(hist_c+1024, CD_dc_diff_lo, CD_dc_diff_hi, 255)+1', 1),
    ('widest_window(hists+1024*(4+pred4), 4, 1024, -1)+1',
     'widest_window(hists+1024*(4+pred4), CD_dc_window_lo, CD_dc_window_hi, -1)+1', 1),
])

print('GL, MB and CD modules written')
