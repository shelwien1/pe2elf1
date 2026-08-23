#!/usr/bin/env python3
# gen_p1cn.py -- port the P1 (MED-residual) and CN (adaptive counter) modules.
# One-time; writes IDX/bmf-P1.idx and IDX/bmf-CN.idx and rewrites the use sites.
import sys

sys.path.insert(0, 'IDX')
from idxgen import emit, rewrite


def _unused_emit(path, prefix, banner, blocks):
    out = ['', 'Prefix %s' % prefix, 'Debug 1', '']
    out += banner
    const = None
    for title, c, items in blocks:
        out.append('')
        for ln in title:
            out.append('# %s' % ln)
        if c != const:
            out.append('Const %d' % c)
            const = c
        out.append('')
        for name, v in items:
            mult = -1 if v < 0 else 1
            out.append('Number %-16s %2d,0!%s' % (name + ',', mult, format(abs(v), 'b')))
    open(path, 'w').write('\n'.join(out) + '\n')


def _unused_rewrite(path, subs):
    src = open(path).read()
    for old, new, count in subs:
        n = src.count(old)
        assert n == count, '%s: expected %d of %r, found %d' % (path, count, old, n)
        src = src.replace(old, new)
    open(path, 'w').write(src)


# ---------------------------------------------------------------- P1 --------
emit('IDX/bmf-P1.idx', 'P1', [
    '# ---------------------------------------------------------------------',
    '# alt-P1: the MED-predictor residual model.',
    '#',
    '# The counter bumps below are the weights this model spends its evidence',
    '# at: how hard a hit updates the cell it landed in, its two neighbours in',
    '# the level ladder, and the alternate context.  They were tuned as a set.',
    '# ---------------------------------------------------------------------',
], [
    (['Structural sizes: these must stay constant expressions -- they size',
      'arrays and drive template arguments.'], 1, [
        ('row_margin', 4), ('row_tail', 6), ('counter_count', 629856),
    ]),
    (['Activity quantiser: the rounding offset and shift that turn the summed',
      'neighbourhood activity into a table row, and the two guess thresholds.'], 0, [
        ('act_round', 7), ('act_shift', 4),
        ('guess_thr_0', 216), ('guess_thr_1', 22),
    ]),
    (['The level, group and slot ladders: three monotone quantisers over the',
      'activity and the predicted value, and the dead-zone width per level.'], 0, [
        ('level_edge_0', 1), ('level_edge_1', 2), ('level_edge_2', 4),
        ('level_edge_3', 8), ('level_edge_4', 14), ('level_edge_5', 35),
        ('level_edge_6', 103),
        ('group_edge_0', 1), ('group_edge_1', 3), ('group_edge_2', 6),
        ('group_edge_3', 10), ('group_edge_4', 16), ('group_edge_5', 27),
        ('group_edge_6', 52),
        ('slot_edge_0', 5), ('slot_edge_1', 10), ('slot_edge_2', 36),
        ('slot_edge_3', 98), ('slot_edge_4', 154), ('slot_edge_5', 236),
        ('slot_edge_6', 248),
        ('level_step_0', 1), ('level_step_1', 1), ('level_step_2', 2),
        ('level_step_3', 2), ('level_step_4', 2), ('level_step_5', 4),
        ('level_step_6', 4), ('level_step_7', 4),
    ]),
    (['The selector mixer: the seed weight of the first of nine context',
      'weights and the ratio between successive ones.'], 0, [
        ('ctxw_seed', 32), ('ctxw_ratio', 3),
    ]),
    (['Counter bumps, sel==1: the centre cell and its two ladder neighbours.'], 0, [
        ('sel1_ctr', 6), ('sel1_up', 4), ('sel1_dn', 3),
    ]),
    (['Counter bumps, sel!=1: the far cell, the middle cell, the alternate',
      'context, and their ladder neighbours.'], 0, [
        ('selN_far', 7), ('selN_mid', 6), ('selN_alt', 4),
        ('selN_mid_up', 4), ('selN_alt_up', 2),
        ('selN_mid_dn', 3), ('selN_alt_dn', 2),
    ]),
    (['Counter bumps in update_model: the alternate context, the two ladder',
      'neighbours of the chosen cell, and the two nudges that follow.'], 0, [
        ('bump_alt', 17), ('bump_up', 11), ('bump_dn', 13),
        ('nudge_up', 7), ('nudge_dn', 5),
    ]),
])

rewrite('alt_p1.inc', [
    ('static const int32_t kRowMargin = 4;',
     'static const int32_t kRowMargin = P1_row_margin;', 1),
    ('static const int32_t kRowTail = 6;',
     'static const int32_t kRowTail = P1_row_tail;', 1),
    ('static const int32_t kP1CounterCount = 629856;',
     'static const int32_t kP1CounterCount = P1_counter_count;', 1),
    ('for( int32_t k = 0; k<6; ++k )\n        here[k] = here[-1-k];',
     'for( int32_t k = 0; k<kRowTail; ++k )\n        here[k] = here[-1-k];', 1),
    ('int32_t act_q = (act_all+7)>>4;',
     'int32_t act_q = (act_all+P1_act_round)>>idx_clamp(P1_act_shift, 1, 8);', 1),
    ('ctx_w[0].sel = (guess>216)+(guess>22);',
     'ctx_w[0].sel = (guess>P1_guess_thr_0)+(guess>P1_guess_thr_1);', 1),
    ('for( int32_t c = 0, w1 = 32; c<9; ++c, w1 *= 3 ) {',
     'for( int32_t c = 0, w1 = P1_ctxw_seed; c<9; ++c, w1 *= P1_ctxw_ratio ) {', 1),
    ('      lo[0].bump(slot_f, 6);\n      hi[0].bump(slot_f, 6);',
     '      lo[0].bump(slot_f, P1_sel1_ctr);\n      hi[0].bump(slot_f, P1_sel1_ctr);', 1),
    ('        lo[1].bump(slot_f, 4);\n        hi[1].bump(slot_f, 4);',
     '        lo[1].bump(slot_f, P1_sel1_up);\n        hi[1].bump(slot_f, P1_sel1_up);', 1),
    ('        lo[-1].bump(slot_f, 3);\n        hi[-1].bump(slot_f, 3);',
     '        lo[-1].bump(slot_f, P1_sel1_dn);\n        hi[-1].bump(slot_f, P1_sel1_dn);', 1),
    ('.bump(slot_f, 7);', '.bump(slot_f, P1_selN_far);', 1),
    ('      mid[0].bump(slot_f, 6);\n      alt[0].bump(slot_r, 4);',
     '      mid[0].bump(slot_f, P1_selN_mid);\n      alt[0].bump(slot_r, P1_selN_alt);', 1),
    ('        mid[1].bump(slot_f, 4);\n        alt[1].bump(slot_r, 2);',
     '        mid[1].bump(slot_f, P1_selN_mid_up);\n        alt[1].bump(slot_r, P1_selN_alt_up);', 1),
    ('        mid[-1].bump(slot_f, 3);\n        alt[-1].bump(slot_r, 2);',
     '        mid[-1].bump(slot_f, P1_selN_mid_dn);\n        alt[-1].bump(slot_r, P1_selN_alt_dn);', 1),
    ('node_alt[0].bump(slot_r, 17);', 'node_alt[0].bump(slot_r, P1_bump_alt);', 1),
    ('node_up[1].bump(slot_f, 11);', 'node_up[1].bump(slot_f, P1_bump_up);', 1),
    ('node_dn[-1].bump(slot_f, 13);', 'node_dn[-1].bump(slot_f, P1_bump_dn);', 1),
    ('node_alt[1].bump(slot_r, 7);', 'node_alt[1].bump(slot_r, P1_nudge_up);', 1),
    ('node_alt[-1].bump(slot_r, 5);', 'node_alt[-1].bump(slot_r, P1_nudge_dn);', 1),
])

# The P1 edge tables move out of bmf_tables.inc.
rewrite('bmf_tables.inc', [
    ('''alignas(16) static constexpr uint8_t p1_level_edges[8] = {
  1, 2, 4, 8, 14, 35, 103, 0,
};
alignas(16) static constexpr uint8_t p1_group_edges[8] = {
  1, 3, 6, 10, 16, 27, 52, 0,
};
alignas(16) static constexpr uint8_t p1_slot_edges[8] = {
  5, 10, 36, 98, 154, 236, 248, 0,
};
alignas(16) static constexpr int32_t p1_level_step[8] = {
  1, 1, 2, 2, 2, 4, 4, 4,
};''',
     '''// The P1 ladders.  int32_t rather than uint8_t for the same reason as the P2
// pair above: in the tuning build these are load-time reads, and narrowing one
// in an aggregate initialiser is only allowed from a constant expression.
alignas(16) static const int32_t p1_level_edges[8] = {
  P1_level_edge_0, P1_level_edge_1, P1_level_edge_2, P1_level_edge_3,
  P1_level_edge_4, P1_level_edge_5, P1_level_edge_6, 0,
};
alignas(16) static const int32_t p1_group_edges[8] = {
  P1_group_edge_0, P1_group_edge_1, P1_group_edge_2, P1_group_edge_3,
  P1_group_edge_4, P1_group_edge_5, P1_group_edge_6, 0,
};
alignas(16) static const int32_t p1_slot_edges[8] = {
  P1_slot_edge_0, P1_slot_edge_1, P1_slot_edge_2, P1_slot_edge_3,
  P1_slot_edge_4, P1_slot_edge_5, P1_slot_edge_6, 0,
};
alignas(16) static const int32_t p1_level_step[8] = {
  P1_level_step_0, P1_level_step_1, P1_level_step_2, P1_level_step_3,
  P1_level_step_4, P1_level_step_5, P1_level_step_6, P1_level_step_7,
};''', 1),
])

# ---------------------------------------------------------------- CN --------
emit('IDX/bmf-CN.idx', 'CN', [
    '# ---------------------------------------------------------------------',
    '# The adaptive counters shared by every model: the seven-slot CounterNode',
    '# and the binary BitCtr pair.',
    '# ---------------------------------------------------------------------',
], [
    (['The symbol alphabet\'s shape: codes below slot_split are their own slot,',
      'the rest fold onto the top two.  slot_split appears at four use sites and',
      'is one parameter so they cannot drift apart.'], 1, [
        ('slot_split', 5), ('slot_top', 6),
    ]),
    (['CounterNode: the seed counts, the increment per hit, the rescale',
      'threshold and how the rescale divides.  The seeded total is the sum of',
      'the seeds rather than its own parameter, so it stays consistent when the',
      'optimizer moves them.'], 0, [
        ('seed_0', 8), ('seed_1', 2), ('seed_2', 2), ('seed_3', 2),
        ('seed_4', 2), ('seed_5', 3), ('seed_6', 3),
        ('bump', 32), ('rescale_at', 0x2000),
        ('halve_shift', 1), ('rescale_round', 2), ('rescale_div', 3),
        ('escape_bias', 96), ('nudge_limit', 0xCCC),
    ]),
    (['BitCtr: the increment, the seeding bump and decay, the rescale ladder',
      'and the parent-credit threshold.'], 0, [
        ('step', 8), ('seed_bump', 4), ('seed_limit', 512),
        ('limit_step', 64), ('max_limit', 0x4000), ('parent_bump', 0x88),
        ('parent_limit', 72), ('root_n0', 40), ('root_n1', 16),
        ('seed_scale', 64), ('seed_decay', 3),
    ]),
    (['Clamps on what reaches the arithmetic coder, and the reconstruction',
      'drift a folded residual is allowed before it refuses to fold.'], 0, [
        ('p16_floor', 32), ('p16_ceil', 65504), ('drift_limit', 16),
    ]),
])

rewrite('counters.inc', [
    ('return code<5 ? code : 6-(code&1);',
     'return code<CN_slot_split ? code : CN_slot_top-(code&1);', 1),
    ('    if( tot>0x2000 ) {', '    if( tot>CN_rescale_at ) {', 1),
    ('const uint16_t nf = lo<=1 ? c[k]-(c[k]>>1) : (c[k]+2)/3;',
     'const uint16_t nf = lo<=1 ? c[k]-(c[k]>>idx_clamp(CN_halve_shift, 1, 15))\n                                 : (c[k]+CN_rescale_round)/idx_clamp(CN_rescale_div, 2, 64);', 1),
    ('    cur[0] += 32;\n    const int32_t result = total+32;',
     '    cur[0] += CN_bump;\n    const int32_t result = total+CN_bump;', 1),
    ('    if( slot>=5 ) {', '    if( slot>=CN_slot_split ) {', 1),
    ('+(uint32_t)(result&0x7FFF)+96)', '+(uint32_t)(result&0x7FFF)+CN_escape_bias)', 1),
    ('cx[0].encode_symbol_tree(strip, (sym-5)>>1);',
     'cx[0].encode_symbol_tree(strip, (sym-CN_slot_split)>>1);', 1),
    ('  static const uint32_t kStep = 8;', '  static const uint32_t kStep = CN_step;', 1),
    ('  static const uint32_t kSeedBump = 4;', '  static const uint32_t kSeedBump = CN_seed_bump;', 1),
    ('  static const uint32_t kSeedLimit = 512;', '  static const uint32_t kSeedLimit = CN_seed_limit;', 1),
    ('  static const uint32_t kLimitStep = 64;', '  static const uint32_t kLimitStep = CN_limit_step;', 1),
    ('  static const uint32_t kMaxLimit = 0x4000;', '  static const uint32_t kMaxLimit = CN_max_limit;', 1),
    ('  static const uint32_t kParentBump = 0x88;', '  static const uint32_t kParentBump = CN_parent_bump;', 1),
    ('    limit = 72;', '    limit = CN_parent_limit;', 1),
    ('    n[0] = 40;\n    n[1] = 16;', '    n[0] = CN_root_n0;\n    n[1] = CN_root_n1;', 1),
    # (par0<<6)-64 is (par0-1)*64: one scale rather than a shift and a bias that
    # have to be moved together.
    ('    n[0] = (par_tot+(par0<<6)-64)/par_tot;\n    n[1] = ((parent[0].n[1]<<6)+par_tot-64)/par_tot;',
     '    n[0] = (par_tot+(par0-1)*CN_seed_scale)/par_tot;\n    n[1] = (par_tot+((int32_t)parent[0].n[1]-1)*CN_seed_scale)/par_tot;', 1),
    ('parent[0].n[first] = par_n-3*(par_n>3);',
     'parent[0].n[first] = par_n-CN_seed_decay*(par_n>CN_seed_decay);', 1),
    ('    if( p16<32 )\n      p16 = 32;\n    if( p16>65504 )\n      p16 = 65504;',
     '    if( p16<CN_p16_floor )\n      p16 = CN_p16_floor;\n    if( p16>CN_p16_ceil )\n      p16 = CN_p16_ceil;', 1),
    ('''  node[0].c[0] = 8;
  node[0].c[1] = 2;
  node[0].c[2] = 2;
  node[0].c[3] = 2;
  node[0].c[4] = 2;
  node[0].c[5] = 3;
  node[0].c[6] = 3;
  node[0].total = 22;''',
     '''  node[0].c[0] = CN_seed_0;
  node[0].c[1] = CN_seed_1;
  node[0].c[2] = CN_seed_2;
  node[0].c[3] = CN_seed_3;
  node[0].c[4] = CN_seed_4;
  node[0].c[5] = CN_seed_5;
  node[0].c[6] = CN_seed_6;
  node[0].total = CN_seed_0+CN_seed_1+CN_seed_2+CN_seed_3+CN_seed_4+CN_seed_5+CN_seed_6;''', 1),
    ('  if( drift<-16||drift>16 ) {',
     '  if( drift<-CN_drift_limit||drift>CN_drift_limit ) {', 1),
])

rewrite('alt_p1.inc', [
    ('      if( slot_f>=5 )', '      if( slot_f>=CN_slot_split )', 2),
    ('    if( slot_f>=5 ) {', '    if( slot_f>=CN_slot_split ) {', 1),
    ('const int32_t tree_sym = (int32_t)(code_f-5)>>1;',
     'const int32_t tree_sym = (int32_t)(code_f-CN_slot_split)>>1;', 1),
    ('cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_up[1].escape_high(slot_f)).raw(ctx_up)), (int32_t)(code_f-5)>>1);',
     'cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_up[1].escape_high(slot_f)).raw(ctx_up)), tree_sym);', 1),
    ('if( counters[result].total>=kCounterNudgeLimit )',
     'if( counters[result].total>=CN_nudge_limit )', 1),
])

print('P1 and CN modules written')
