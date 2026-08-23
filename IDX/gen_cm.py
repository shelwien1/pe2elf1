#!/usr/bin/env python3
# gen_cm.py -- port the CM (direct context model), AP (probability map and
# mixer) and PA (palette model) modules.  One-time.
import sys

sys.path.insert(0, 'IDX')
from idxgen import emit, rewrite

# ---------------------------------------------------------------- CM --------
emit('IDX/bmf-CM.idx', 'CM', [
    '# ---------------------------------------------------------------------',
    '# CtxModel: the direct context model for small-alphabet planes.',
    '#',
    '# The table widths are live rather than frozen -- every table here is',
    '# heap-allocated from a runtime count, so nothing needs them to be constant',
    '# expressions, and how wide a hashed order should be is exactly the kind of',
    '# question the optimizer is for.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural: these size members, so they must fold.'], 1, [
        ('max_neighbours', 24), ('orders', 4), ('max_alpha', 16),
        ('match_lens', 32), ('count_cap_max', 255),
    ]),
    (['Table widths, in bits: the direct-mapped short order, the two hashed',
      'middle orders and the longest.  All allocated from a runtime count.'], 0, [
        ('short_nb', 4), ('table_bits', 20), ('mid_bits', 18), ('match_bits', 20),
    ]),
    (['How many neighbours the longest order sees, by alphabet width: one entry',
      'per bit count.  At four values the table saturates; at eight and sixteen',
      'the learning cost turns it around first; at two it keeps paying to 24.'], 0, [
        ('nb_for_bits_1', 24), ('nb_for_bits_2', 10),
        ('nb_for_bits_3', 7), ('nb_for_bits_4', 7),
    ]),
    (['Counter adaptation: the cap the 1/(n+2) schedule anneals to, and the',
      'offset in that schedule.'], 0, [
        ('count_cap', 60), ('adapt_offset', 2),
    ]),
    (['The mixer: the count-weighted blend the weights are seeded from, the',
      'width of the evidence bucket that selects a weight set, and the',
      'gradient-descent shift.'], 0, [
        ('blend', 24), ('nbucket_shift', 3),
    ]),
    (['The match model: the window length in pixels is match_span/bits, capped;',
      'the per-length confidence starts here.'], 0, [
        ('match_span', 12), ('match_cap', 40), ('match_seed', 40000),
    ]),
    (['The neighbourhood probe: how many pixels it prices, how many bands it',
      'spreads them over, the three shorter candidates it tries as sixths of',
      'the longest -- each with its own rounding offset, round to nearest',
      'rather than a ceiling -- and the bias toward short contexts a crop',
      'needs (a shorter candidate must win by more than 1/2^probe_bias).'], 0, [
        ('probe_shift', 17), ('probe_bands', 8),
        ('probe_num_1', 5), ('probe_den_1', 6), ('probe_round_1', 3),
        ('probe_num_2', 2), ('probe_den_2', 3), ('probe_round_2', 1),
        ('probe_num_3', 1), ('probe_den_3', 2), ('probe_round_3', 1),
        ('probe_bias', 6),
    ]),
    (['The two hash multipliers: Knuth\'s 32-bit constant for the context key,',
      'the 64-bit one for the match window.  Declared so they are visible and',
      'movable, frozen because a hash multiplier has no gradient to climb.'], 0, [
        ('key_hash', 2654435761),
    ]),
    (['The seeded cell probability, in 1/65536.'], 0, [
        ('cell_seed', 32768),
    ]),
])

rewrite('ctx_model.inc', [
    ('  static const int32_t kMaxNeighbours = 24;\n'
     '  static const int32_t kOrders = 4;\n'
     '  static const int32_t kInputs = kOrders+1;\n'
     '  static const int32_t kMatchBits = 20;\n'
     '  static const int32_t kMatchLens = 32;\n'
     '  static const int32_t kShortNb = 4;\n'
     '  static const int32_t kTableBits = 20;\n'
     '  static const int32_t kMidBits = 18;',
     '  static const int32_t kMaxNeighbours = CM_max_neighbours;\n'
     '  static const int32_t kOrders = CM_orders;\n'
     '  static const int32_t kInputs = kOrders+1;\n'
     '  // Table widths are heap-allocated from a runtime count, so unlike the\n'
     '  // sizes above they do not have to be constant expressions and stay live\n'
     '  // for the optimizer.\n'
     '  static const int32_t kMatchBits = idx_clamp(CM_match_bits, 8, 26);\n'
     '  static const int32_t kMatchLens = CM_match_lens;\n'
     '  static const int32_t kShortNb = idx_clamp(CM_short_nb, 1, CM_max_neighbours);\n'
     '  static const int32_t kTableBits = idx_clamp(CM_table_bits, 8, 26);\n'
     '  static const int32_t kMidBits = idx_clamp(CM_mid_bits, 8, 26);', 1),
    ('  static const int32_t kCountCap = 60;\n'
     '  static const int32_t kBlend = 24;\n'
     '  static const int32_t kMaxAlpha = 16;',
     '  // recip[] is sized by the frozen ceiling; the cap itself stays live\n'
     '  // underneath it, so the optimizer can move the anneal point without the\n'
     '  // member array\'s size following it.\n'
     '  static const int32_t kCountCapMax = CM_count_cap_max;\n'
     '  static const int32_t kCountCap = idx_clamp(CM_count_cap, 1, kCountCapMax);\n'
     '  static const int32_t kBlend = CM_blend;\n'
     '  static const int32_t kMaxAlpha = CM_max_alpha;', 1),
    ('  int32_t recip[kCountCap+1];', '  int32_t recip[kCountCapMax+1];', 1),
    ('    for( int32_t k = 0; k<=kCountCap; ++k )\n      recip[k] = 65536/(k+2);',
     '    for( int32_t k = 0; k<=kCountCapMax; ++k )\n'
     '      recip[k] = 65536/(k+idx_clamp(CM_adapt_offset, 1, 64));', 1),
    ('    nb_for_bits[1] = 24;\n    nb_for_bits[2] = 10;\n'
     '    nb_for_bits[3] = 7;\n    nb_for_bits[4] = 7;',
     '    nb_for_bits[1] = CM_nb_for_bits_1;\n    nb_for_bits[2] = CM_nb_for_bits_2;\n'
     '    nb_for_bits[3] = CM_nb_for_bits_3;\n    nb_for_bits[4] = CM_nb_for_bits_4;', 1),
    ('      match_p[k] = 40000u<<16;', '      match_p[k] = (uint32_t)CM_match_seed<<16;', 2),
    ('    match_nb = 12/bits;\n    if( match_nb>40 )\n      match_nb = 40;',
     '    match_nb = CM_match_span/bits;\n    if( match_nb>CM_match_cap )\n'
     '      match_nb = CM_match_cap;', 1),
    ('        ctr[0][j] = 32768u<<16;', '        ctr[0][j] = (uint32_t)CM_cell_seed<<16;', 1),
    ('    const uint32_t hashed = key*2654435761u;',
     '    const uint32_t hashed = key*(uint32_t)CM_key_hash;', 1),
    ('      const int32_t n = 8*b+4;',
     '      const int32_t n = (b<<CM_nbucket_shift)+(1<<idx_clamp(CM_nbucket_shift-1, 0, 12));', 1),
    ('const int32_t p = mixer.mix(p_in, ((n_long>>3)<<4)|at);',
     'const int32_t p = mixer.mix(p_in, ((n_long>>idx_clamp(CM_nbucket_shift, 1, 12))<<4)|at);', 1),
    ('  static const int32_t kProbePixels = 1<<17;',
     '  static const int32_t kProbePixels = 1<<idx_clamp(CM_probe_shift, 12, 24);', 1),
    ('    const int32_t bands = 8;', '    const int32_t bands = idx_clamp(CM_probe_bands, 1, 64);', 1),
    ('    const int32_t want[4] = {longest, (longest*5+3)/6, (longest*2+1)/3, (longest+1)/2};',
     '    const int32_t want[4] = {longest,\n'
     '                             (longest*CM_probe_num_1+CM_probe_round_1)/idx_clamp(CM_probe_den_1, 1, 64),\n'
     '                             (longest*CM_probe_num_2+CM_probe_round_2)/idx_clamp(CM_probe_den_2, 1, 64),\n'
     '                             (longest*CM_probe_num_3+CM_probe_round_3)/idx_clamp(CM_probe_den_3, 1, 64)};', 1),
    ('      const uint64_t priced = k ? measured+(measured>>6) : measured;',
     '      const uint64_t priced = k ? measured+(measured>>idx_clamp(CM_probe_bias, 1, 20)) : measured;', 1),
])

# ---------------------------------------------------------------- AP --------
emit('IDX/bmf-AP.idx', 'AP', [
    '# ---------------------------------------------------------------------',
    '# The adaptive probability map and the logistic mixer, shared by every',
    '# model that has a decision worth refining.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural: bucket and context counts size member arrays.'], 1, [
        ('buckets', 33), ('map_ctx', 256), ('mix_sets', 256),
        ('adapt_cap_max', 255),
    ]),
    (['The map: how far into the stretch domain the buckets reach (in 1/256 of',
      'a stretch unit), how fast an entry moves, and how much of its opinion is',
      'taken against the model\'s own.  The map has fewer samples per entry than',
      'the cell does, so it is the less certain of the two.'], 0, [
        ('span', 2048), ('rate', 5), ('map_weight', 3),
    ]),
    (['Clamps on a probability leaving either stage.'], 0, [
        ('p_floor', 32), ('p_ceil', 65504),
    ]),
    (['The mixer: the stretch scale (in 1/256 units per nat) and the',
      'gradient-descent shift.'], 0, [
        ('stretch_scale', 256), ('mix_shift', 18),
    ]),
    (['The count-adaptive counter shared outside CtxModel: the anneal cap, the',
      'offset in the 1/(n+offset) schedule, and the probability a fresh cell',
      'starts at.'], 0, [
        ('adapt_cap', 60), ('adapt_offset', 2), ('cell_seed', 32768),
    ]),
])

rewrite('apm.inc', [
    ('const int32_t kApmBuckets = 33;\nconst double kApmSpan = 8.0;',
     'const int32_t kApmBuckets = AP_buckets;\n'
     '// The span is carried as 1/256 of a stretch unit so it can be an integer\n'
     '// parameter; 2048 is the 8.0 it replaces.\n'
     'const double kApmSpan = idx_d(AP_span, 8);', 1),
    ('    if( v<32 )\n      v = 32;\n    if( v>65504 )\n      v = 65504;\n'
     '    apm_squash[b] = (uint16_t)v;',
     '    if( v<AP_p_floor )\n      v = AP_p_floor;\n    if( v>AP_p_ceil )\n'
     '      v = AP_p_ceil;\n    apm_squash[b] = (uint16_t)v;', 1),
    ('  static const int32_t kCtx = 256;\n  static const int32_t kRate = 5;',
     '  static const int32_t kCtx = AP_map_ctx;\n'
     '  static const int32_t kRate = idx_clamp(AP_rate, 1, 12);', 1),
    ('    const int32_t out = (3*mapped+p16)>>2;\n'
     '    return out<32 ? 32 : out>65504 ? 65504 : out;',
     '    const int32_t w = idx_clamp(AP_map_weight, 1, 15);\n'
     '    const int32_t out = (w*mapped+p16)/(w+1);\n'
     '    return out<AP_p_floor ? AP_p_floor : out>AP_p_ceil ? AP_p_ceil : out;', 1),
    ('    double s = 256.0*log(p/(1.0-p));\n'
     '    if( s<-2047.0 )\n      s = -2047.0;\n'
     '    if( s>2047.0 )\n      s = 2047.0;',
     '    double s = (double)AP_stretch_scale*log(p/(1.0-p));\n'
     '    // The clip is the table\'s own half-width, not a number of its own:\n'
     '    // mix() indexes mix_squash by dot+kMixCentre.\n'
     '    if( s<-(double)(kMixCentre-1) )\n      s = -(double)(kMixCentre-1);\n'
     '    if( s>(double)(kMixCentre-1) )\n      s = (double)(kMixCentre-1);', 1),
    ('    int32_t v = (int32_t)(65536.0/(1.0+exp(-(double)(x-2048)/256.0)));\n'
     '    if( v<32 )\n      v = 32;\n    if( v>65504 )\n      v = 65504;',
     '    int32_t v = (int32_t)(65536.0/(1.0+exp(-(double)(x-kMixCentre)/(double)AP_stretch_scale)));\n'
     '    if( v<AP_p_floor )\n      v = AP_p_floor;\n    if( v>AP_p_ceil )\n      v = AP_p_ceil;', 1),
    ('int32_t mix_stretch[4096];\nuint16_t mix_squash[4096];',
     '// The mixer\'s stretch table spans [-kMixCentre, kMixCentre).\n'
     'const int32_t kMixCentre = 2048;\n'
     'int32_t mix_stretch[4096];\nuint16_t mix_squash[4096];', 1),
    ('  static const int32_t kSets = 256;\n  static const int32_t kShift = 18;',
     '  static const int32_t kSets = AP_mix_sets;\n'
     '  static const int32_t kShift = idx_clamp(AP_mix_shift, 8, 24);', 1),
    ('    if( dot<-2047 )\n      dot = -2047;\n    if( dot>2047 )\n      dot = 2047;\n'
     '    return mix_squash[dot+2048];',
     '    if( dot<-(kMixCentre-1) )\n      dot = -(kMixCentre-1);\n'
     '    if( dot>kMixCentre-1 )\n      dot = kMixCentre-1;\n'
     '    return mix_squash[dot+kMixCentre];', 1),
    ('const int32_t kAdaptCap = 60;\nint32_t adapt_recip[kAdaptCap+1];',
     'const int32_t kAdaptCapMax = AP_adapt_cap_max;\n'
     'const int32_t kAdaptCap = idx_clamp(AP_adapt_cap, 1, kAdaptCapMax);\n'
     'int32_t adapt_recip[kAdaptCapMax+1];', 1),
    ('  for( int32_t k = 0; k<=kAdaptCap; ++k )\n    adapt_recip[k] = 65536/(k+2);',
     '  for( int32_t k = 0; k<=kAdaptCapMax; ++k )\n'
     '    adapt_recip[k] = 65536/(k+idx_clamp(AP_adapt_offset, 1, 64));', 1),
    ('uint32_t adapt_init() {\n  return 32768u<<16;\n}',
     'uint32_t adapt_init() {\n  return (uint32_t)AP_cell_seed<<16;\n}', 1),
])

# ---------------------------------------------------------------- PA --------
emit('IDX/bmf-PA.idx', 'PA', [
    '# ---------------------------------------------------------------------',
    '# The palette model: the entries themselves go out through this, ahead of',
    '# the image.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural: the byte tree and the residual context count size arrays.'], 1, [
        ('tree', 256), ('res_ctx', 6),
    ]),
    (['The binary counter: seed, increment and the total it halves at.'], 0, [
        ('seed', 1), ('step', 24), ('limit', 4096),
    ]),
])

rewrite('palette.inc', [
    ('  static const int32_t kStep = 24;\n  static const int32_t kLimit = 4096;',
     '  static const int32_t kStep = PA_step;\n  static const int32_t kLimit = PA_limit;', 1),
    ('    n[0] = 1;\n    n[1] = 1;', '    n[0] = PA_seed;\n    n[1] = PA_seed;', 1),
    ('  static const int32_t kTree = 256;', '  static const int32_t kTree = PA_tree;', 1),
    ('  static const int32_t kResCtx = 6;', '  static const int32_t kResCtx = PA_res_ctx;', 1),
])

print('CM, AP and PA modules written')
