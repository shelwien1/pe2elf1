// tables.inc -- the constant tables, in the order they sit in BMF.exe's data segment.

// Fifteen context groups, each a six-bit mask of which of the five selector
// weights are folded together.  Indexed 0..14 by `g` in `model_plane`.
//
// The five weights are the five outcomes of the stage-one decision -- north,
// west, north-east, north-west, escape.  Each bit folds one weight into another
// and zeroes the source, and a group with no bits set gives all five their own
// weight.  Order matters, because a fold reads what the fold before it left:
// both readers apply them as 3-doubles, 4-into-2, 3-into-2, 4-into-1,
// 3-into-1, 2-into-1, which is not the bit order this enum is written in.
//
// `model_plane` and `unmodel_plane` both read this table, and until this enum
// existed one of them named the bits by role and the other by bit number
// (`f_b0`, `f_b1`, ...), so the two halves of one encode/decode pair could not
// be read against each other.
enum : uint8_t {
  ctx_w4_to_w1  = 0x01,
  ctx_w4_to_w2  = 0x02,
  ctx_w3_double = 0x04,
  ctx_w3_to_w1  = 0x08,
  ctx_w3_to_w2  = 0x10,
  ctx_w2_to_w1  = 0x20,
};

alignas(16) static constexpr uint8_t ctx_group_flags[32] = {
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// The low-bit rotation of a p2 counter index: `[(ctx>>2)&3]`.
alignas(16) static constexpr int32_t p2_ctx_rotate[4] = {
  4, 8, 12, 8,
};

// Quantiser edges for the p2 d8 body, indexed 0..15.
alignas(16) static constexpr uint8_t p2_ctx_edges[16] = {
  17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0,
};

// Run-length buckets for the p2 d8 body, indexed 0..15.
alignas(16) static constexpr uint8_t p2_len_edges[32] = {
  4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// [band][column]: `band` is 0..5 from the activity ratio, and the thirteen
// columns are read by name in `alt_p2_context` -- 0..2 for gD, 3..5 for gC,
// 6..8 for gB and 9..12 for gA.
alignas(16) static constexpr int32_t bmf_p2_thresholds[6][13] = {
  {6, 14, 28, 4, 8, 26, 4096, 6144, 13056, 10240, 14336, 26624, 36864}, {4, 15, 42, 1, 8, 19, 2560, 6912, 11264, 10240, 28672, 71680, 139264}, {5, 14, 21, 4, 9, 25, 3840, 8448, 12032, 16384, 26624, 43008, 83968}, {6, 11, 20, 5, 11, 23, 2816, 5888, 10496, 22528, 32768, 51200, 131072}, {0, 11, 30, 7, 13, 22, 4352, 8448, 14848, 16384, 38912, 57344, 94208}, {7, 10, 31, -1, 11, 21, -256, 1024, 5888, -2048, -2048, -2048, 145408},
};

// What a `P2Count`'s countdown reloads to when its rate steps up, indexed by
// the new rate less one.
//
// This was a seventeen-element `int32_t` table of float bit patterns named
// `p2_float_pool`, and its one live use read *bytes* out of the middle of it:
// `*((uint8_t*)&p2_float_pool + rate+1 + 3)`, which for a rate of 0..7 is byte
// offsets 4 through 11 -- the little-endian bytes of elements [1] and [2],
// `0x3F800000` and `0xC52E0702`.  Those eight bytes are the whole of what the
// table was for; the other nine elements had no reader at all.
//
// So it is the eight bytes, written down.  The old form also only worked
// little-endian, which is not a property a reload table should have.  A rate
// of 0 or 1 reloading to zero is deliberate -- it interacts with the
// `(uint8_t)--countdown` wrap at the use site, so it is preserved, not fixed.
// What a counter's countdown is re-armed to when its rate steps up, indexed by
// the rate it is stepping *from*.
//
// Declared `[8]` in the decompilation and read at exactly one site,
// `p2_b1_reload[b0n - 1]` with `b0n = rate + 1` under a `rate < 8` guard -- so
// the rate is 5, 6 or 7 and the index is 5, 6 or 7.  Indices 0 to 4 are
// unreachable, and the first four bytes were `00 00 80 3F`, which is `1.0f`
// little-endian: the "table" was a mis-sliced fragment of an adjacent float
// constant with the three live bytes on the end of it.
//
// The original binary is not here to check the declared bound against, so what
// was checked is the reachable range and the reader count -- one site, one
// guard, three indices -- and the gate, which would move a stream if the three
// values were wrong or if anything read past the end.  `alignas(16)` stays: it
// is the table's own alignment and not a claim about what follows it.
alignas(16) static const uint8_t p2_b1_reload[3] = { 7, 46, 197 };

alignas(16) static constexpr uint8_t p1_level_edges[8] = {
  1, 2, 4, 8, 14, 35, 103, 0,
};

alignas(16) static constexpr uint8_t p1_group_edges[8] = {
  1, 3, 6, 10, 16, 27, 52, 0,
};

alignas(16) static constexpr uint8_t p1_slot_edges[8] = {
  5, 10, 36, 98, 154, 236, 248, 0,
};

// `constexpr` like the table above it, which it was not until `tools/shared.py`
// asked why.  Its one reader indexes it and nothing writes it; it sat between
// the genuine constants and the two mutable tables below and had been read as
// belonging to the second group by nothing more than where it is.
alignas(16) static constexpr int32_t p1_level_step[8] = {
  1, 1, 2, 2, 2, 4, 4, 4,
};

// **Not `constexpr`, and not `const`.**  These two are mutable session state:
// `P2Coef` saves and restores them around a filter search, so the "constant"
// coefficient table is a live model parameter that the encoder rewrites and
// puts back.  Stage 2 Section 7 says so explicitly and it is worth repeating
// here, because they sit in a file of tables that are now all `constexpr` and
// the odd pair out is the one that would break silently.
// Read `bmf_p2_coef_init` and `bmf_p2_rate_init`: these are the values a codec
// starts from, not the values it uses.  `BMFState` keeps a copy of each and
// `P2Coef` folds *that* -- see the note there.  They were mutable file-scope
// arrays until two codecs in two threads folded them at once.
alignas(16) static const float bmf_p2_coef_init[7][4] = {
  {-0.05f, -0.07f, 0.3f, 0.18f}, {-0.03f, 0.04f, 0.06f, -0.01f}, {0.06f, 0.16f, 0.0f, -0.01f}, {-0.1f, -0.02f, 0.05f, 0.05f}, {0.06f, 0.0f, 0.04f, 0.02f}, {0.04f, 0.02f, 0.04f, 0.05f}, {0.07f, 0.0f, 0.03f, 0.02f},
};

alignas(16) static const float bmf_p2_rate_init[7][4] = {
  {0.0108f, 0.0069f, 0.0054f, 0.0052f}, {0.0042f, 0.0042f, 0.0042f, 0.0039f}, {0.0035f, 0.0035f, 0.0028f, 0.0028f}, {0.0027f, 0.0026f, 0.0026f, 0.0024f}, {0.0023f, 0.0023f, 0.0022f, 0.0021f}, {0.0019f, 0.0019f, 0.0017f, 0.0017f}, {0.0015f, 0.0015f, 0.0011f, 0.0009f},
};

static const float bmf_p2_rate_reset = 0.0024f;

static const float bmf_p2_ms_rate = 0.023f;

static const float bmf_p2_mix[4][6] = {
  {0.4f, 0.1f, 0.15f, 0.23f, 0.04f, 0.03f},
  {0.26f, 0.23f, 0.13f, 0.12f, 0.15f, 0.12f},
  {0.27f, 0.22f, 0.26f, 0.08f, 0.12f, 0.05f},
  {0.3f, 0.14f, 0.08f, 0.22f, 0.18f, 0.09f},
};

static const float bmf_p2_decay = 0.78f;

static const float bmf_p2_seed = 0.19f;
