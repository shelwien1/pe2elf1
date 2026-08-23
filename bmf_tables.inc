// bmf_tables.inc -- the tuned constant tables, built from the IDX parameters.
// Included from bmf.cpp; not a standalone translation unit.
//
// Nothing here is a literal any more.  Every value comes from IDX/*.idx via
// MOD/, so the optimizer can move it in the tuning build and the compiler can
// fold it in the shipping one, with this file spelled the same either way.
// The float tables read their values back through idx_f, which is exact:
// see the comment on idx_f in bmf_idx.inc for why that matters here.

alignas(16) static const uint8_t ctx_group_flags[32] = {
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// The bank rotation applied to the P2 context every fourth column.
alignas(16) static const int32_t p2_ctx_rotate[4] = {
  P2_rotate_0, P2_rotate_1, P2_rotate_2, P2_rotate_3,
};

// The delta-context ladder and the run-length ladder: two monotone quantiser
// edge lists, climbed a step at a time as the model walks its context space.
// They are int32_t rather than uint8_t because in the tuning build the values
// are load-time reads, and a narrowing conversion in an aggregate initialiser
// is only allowed from a constant expression.
alignas(16) static const int32_t p2_ctx_edges[16] = {
  P2_ctx_edge_00, P2_ctx_edge_01, P2_ctx_edge_02, P2_ctx_edge_03, P2_ctx_edge_04, P2_ctx_edge_05, P2_ctx_edge_06, P2_ctx_edge_07, P2_ctx_edge_08, P2_ctx_edge_09, P2_ctx_edge_10, P2_ctx_edge_11, P2_ctx_edge_12, P2_ctx_edge_13, P2_ctx_edge_14, 0,
};
alignas(16) static const int32_t p2_len_edges[32] = {
  P2_len_edge_00, P2_len_edge_01, P2_len_edge_02, P2_len_edge_03, P2_len_edge_04, P2_len_edge_05, P2_len_edge_06, P2_len_edge_07, P2_len_edge_08, P2_len_edge_09, P2_len_edge_10, P2_len_edge_11, P2_len_edge_12, P2_len_edge_13, P2_len_edge_14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Four threshold triples per band, selected by p2_band_edges: the gradient,
// neighbour-count, up-right and down-run quantisers that build the P2 slot.
alignas(16) static const int32_t bmf_p2_thresholds[6][13] = {
  {P2_thr_0_00, P2_thr_0_01, P2_thr_0_02, P2_thr_0_03, P2_thr_0_04, P2_thr_0_05, P2_thr_0_06, P2_thr_0_07, P2_thr_0_08, P2_thr_0_09, P2_thr_0_10, P2_thr_0_11, P2_thr_0_12},
  {P2_thr_1_00, P2_thr_1_01, P2_thr_1_02, P2_thr_1_03, P2_thr_1_04, P2_thr_1_05, P2_thr_1_06, P2_thr_1_07, P2_thr_1_08, P2_thr_1_09, P2_thr_1_10, P2_thr_1_11, P2_thr_1_12},
  {P2_thr_2_00, P2_thr_2_01, P2_thr_2_02, P2_thr_2_03, P2_thr_2_04, P2_thr_2_05, P2_thr_2_06, P2_thr_2_07, P2_thr_2_08, P2_thr_2_09, P2_thr_2_10, P2_thr_2_11, P2_thr_2_12},
  {P2_thr_3_00, P2_thr_3_01, P2_thr_3_02, P2_thr_3_03, P2_thr_3_04, P2_thr_3_05, P2_thr_3_06, P2_thr_3_07, P2_thr_3_08, P2_thr_3_09, P2_thr_3_10, P2_thr_3_11, P2_thr_3_12},
  {P2_thr_4_00, P2_thr_4_01, P2_thr_4_02, P2_thr_4_03, P2_thr_4_04, P2_thr_4_05, P2_thr_4_06, P2_thr_4_07, P2_thr_4_08, P2_thr_4_09, P2_thr_4_10, P2_thr_4_11, P2_thr_4_12},
  {P2_thr_5_00, P2_thr_5_01, P2_thr_5_02, P2_thr_5_03, P2_thr_5_04, P2_thr_5_05, P2_thr_5_06, P2_thr_5_07, P2_thr_5_08, P2_thr_5_09, P2_thr_5_10, P2_thr_5_11, P2_thr_5_12},
};
alignas(16) static const int32_t p2_b1_reload[3] = {P2_b1_reload_0, P2_b1_reload_1, P2_b1_reload_2};

alignas(16) static constexpr uint8_t p1_level_edges[8] = {
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
};

// The NLMS cascade: seed coefficients, per-tap adaptation rates, and the
// blend weights over the six cascade outputs.
alignas(16) static const float bmf_p2_coef_init[7][4] = {
  {idx_f(P2_coef_00, 28), idx_f(P2_coef_01, 27), idx_f(P2_coef_02, 24), idx_f(P2_coef_03, 24)},
  {idx_f(P2_coef_10, 29), idx_f(P2_coef_11, 27), idx_f(P2_coef_12, 28), idx_f(P2_coef_13, 29)},
  {idx_f(P2_coef_20, 28), idx_f(P2_coef_21, 25), idx_f(P2_coef_22, 0), idx_f(P2_coef_23, 29)},
  {idx_f(P2_coef_30, 27), idx_f(P2_coef_31, 28), idx_f(P2_coef_32, 28), idx_f(P2_coef_33, 28)},
  {idx_f(P2_coef_40, 28), idx_f(P2_coef_41, 0), idx_f(P2_coef_42, 27), idx_f(P2_coef_43, 28)},
  {idx_f(P2_coef_50, 27), idx_f(P2_coef_51, 28), idx_f(P2_coef_52, 27), idx_f(P2_coef_53, 28)},
  {idx_f(P2_coef_60, 27), idx_f(P2_coef_61, 0), idx_f(P2_coef_62, 29), idx_f(P2_coef_63, 28)},
};
alignas(16) static const float bmf_p2_rate_init[7][4] = {
  {idx_f(P2_rate_00, 28), idx_f(P2_rate_01, 31), idx_f(P2_rate_02, 29), idx_f(P2_rate_03, 31)},
  {idx_f(P2_rate_10, 31), idx_f(P2_rate_11, 31), idx_f(P2_rate_12, 31), idx_f(P2_rate_13, 30)},
  {idx_f(P2_rate_20, 31), idx_f(P2_rate_21, 31), idx_f(P2_rate_22, 30), idx_f(P2_rate_23, 30)},
  {idx_f(P2_rate_30, 30), idx_f(P2_rate_31, 32), idx_f(P2_rate_32, 32), idx_f(P2_rate_33, 31)},
  {idx_f(P2_rate_40, 32), idx_f(P2_rate_41, 32), idx_f(P2_rate_42, 27), idx_f(P2_rate_43, 32)},
  {idx_f(P2_rate_50, 31), idx_f(P2_rate_51, 31), idx_f(P2_rate_52, 33), idx_f(P2_rate_53, 33)},
  {idx_f(P2_rate_60, 32), idx_f(P2_rate_61, 32), idx_f(P2_rate_62, 28), idx_f(P2_rate_63, 33)},
};
const float bmf_p2_mix[4][6] = {
  {idx_f(P2_mix_00, 25), idx_f(P2_mix_01, 27), idx_f(P2_mix_02, 25), idx_f(P2_mix_03, 26), idx_f(P2_mix_04, 27), idx_f(P2_mix_05, 29)},
  {idx_f(P2_mix_10, 22), idx_f(P2_mix_11, 26), idx_f(P2_mix_12, 23), idx_f(P2_mix_13, 27), idx_f(P2_mix_14, 25), idx_f(P2_mix_15, 27)},
  {idx_f(P2_mix_20, 25), idx_f(P2_mix_21, 25), idx_f(P2_mix_22, 22), idx_f(P2_mix_23, 26), idx_f(P2_mix_24, 27), idx_f(P2_mix_25, 28)},
  {idx_f(P2_mix_30, 24), idx_f(P2_mix_31, 26), idx_f(P2_mix_32, 26), idx_f(P2_mix_33, 25), idx_f(P2_mix_34, 24), idx_f(P2_mix_35, 25)},
};

const float bmf_p2_rate_reset = idx_f(P2_rate_reset, 31);
const float bmf_p2_ms_rate = idx_f(P2_ms_rate, 29);
const float bmf_p2_decay = idx_f(P2_decay, 22);
const float bmf_p2_seed = idx_f(P2_seed, 24);
