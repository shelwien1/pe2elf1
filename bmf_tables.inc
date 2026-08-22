// bmf_tables.inc -- the tuned constant tables: context quantiser edges, thresholds, NLMS seeds
// Included from bmf.cpp; not a standalone translation unit.

alignas(16) static constexpr uint8_t ctx_group_flags[32] = {
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static constexpr int32_t p2_ctx_rotate[4] = {
  4, 8, 12, 8,
};
alignas(16) static constexpr uint8_t p2_ctx_edges[16] = {
  17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0,
};
alignas(16) static constexpr uint8_t p2_len_edges[32] = {
  4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static constexpr int32_t bmf_p2_thresholds[6][13] = {
  {6, 14, 28, 4, 8, 26, 4096, 6144, 13056, 10240, 14336, 26624, 36864}, {4, 15, 42, 1, 8, 19, 2560, 6912, 11264, 10240, 28672, 71680, 139264}, {5, 14, 21, 4, 9, 25, 3840, 8448, 12032, 16384, 26624, 43008, 83968}, {6, 11, 20, 5, 11, 23, 2816, 5888, 10496, 22528, 32768, 51200, 131072}, {0, 11, 30, 7, 13, 22, 4352, 8448, 14848, 16384, 38912, 57344, 94208}, {7, 10, 31, -1, 11, 21, -256, 1024, 5888, -2048, -2048, -2048, 145408},
};
alignas(16) static const uint8_t p2_b1_reload[3] = {7, 46, 197};
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
alignas(16) static const float bmf_p2_coef_init[7][4] = {
  {-0.05f, -0.07f, 0.3f, 0.18f}, {-0.03f, 0.04f, 0.06f, -0.01f}, {0.06f, 0.16f, 0.0f, -0.01f}, {-0.1f, -0.02f, 0.05f, 0.05f}, {0.06f, 0.0f, 0.04f, 0.02f}, {0.04f, 0.02f, 0.04f, 0.05f}, {0.07f, 0.0f, 0.03f, 0.02f},
};
alignas(16) static const float bmf_p2_rate_init[7][4] = {
  {0.0108f, 0.0069f, 0.0054f, 0.0052f}, {0.0042f, 0.0042f, 0.0042f, 0.0039f}, {0.0035f, 0.0035f, 0.0028f, 0.0028f}, {0.0027f, 0.0026f, 0.0026f, 0.0024f}, {0.0023f, 0.0023f, 0.0022f, 0.0021f}, {0.0019f, 0.0019f, 0.0017f, 0.0017f}, {0.0015f, 0.0015f, 0.0011f, 0.0009f},
};
const float bmf_p2_mix[4][6] = {
  {0.4f, 0.1f, 0.15f, 0.23f, 0.04f, 0.03f},
  {0.26f, 0.23f, 0.13f, 0.12f, 0.15f, 0.12f},
  {0.27f, 0.22f, 0.26f, 0.08f, 0.12f, 0.05f},
  {0.3f, 0.14f, 0.08f, 0.22f, 0.18f, 0.09f},
};

const float bmf_p2_rate_reset = 0.0024f;
const float bmf_p2_ms_rate = 0.023f;
const float bmf_p2_decay = 0.78f;
const float bmf_p2_seed = 0.19f;
