// ---------------------------------------------------------------------------
// BMF's globals.
//
// Every global the decompiled bodies touch is a reference bound to a fixed
// offset inside blob.inc, BMF.exe's data segment.  Hex-Rays named each one
// after the function it was recovered in, so the same object arrived under one
// name per user -- 905 declarations for 337 objects -- each sitting just above
// its function.  They are collected here instead, sorted by address, one
// declaration per object, under the name IDA gave the address with the
// function prefix dropped.  293 are left of those 337: 34 went with the bodies
// the `c`/`d` command line cannot reach, 9 were the range coder's state and are
// now private to the class below, and 2 more went with the command-line and
// SSE2 cleanups.  One, __byte_44337D, is not from the extractor at all -- see
// the note further down.
//
// The typedefs carry the array shapes, which cannot be written inline in the
// reference declaration.  Where one body reads an address as a bare scalar and
// another indexes it, the array shape is the one declared and the scalar's
// users say [0].
//
// Nothing below this block writes an address as a number any more.  Hex-Rays
// left some baked into expressions rather than into named globals -- the
// `*(_QWORD *)(n64 + 4469652)` shape, 4469652 being 0x00443394 -- and those now
// take the address of the global that owns the byte they start at.  Where that
// byte is inside an object rather than at its start, the offset is written out
// rather than given a global of its own; the exception is 0x0044337D, byte 1 of
// the dword array based at coded_buf, which is indexed like an array and so got
// a byte global of its own.
//
// A few functions declare a local with the same name as the global they use
// and reach the global through `::`; those locals still carry their original
// `__sub_XXXXXX_` names, which is now what tells the two apart.
//
// incdec.md §6.1 is why the addresses are still the names: giving these
// objects real names is a much larger job than moving them, because the same
// address is an int to one function and a char[] to the next.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------
// BMF.exe's data segment, one definition per global.
//
// Each carries the bytes it had, and 64 bytes of guard after it so
// that a global running past its own end lands in padding rather than
// in its neighbour.  The extent is the distance to the next global:
// the [0x10000] bounds Hex-Rays emitted are guesses and there is no
// way to tell from the source which are real.  REFACTORING.md §4.1.
// ---------------------------------------------------------------
// BMF's .bss, the last of its data segment that is still one object.
//
// Every global below is a reference into it at its original offset, which is
// what `blob1` used to be -- but this is 19 584 bytes of zeroes rather than a
// generated copy of the whole data segment, because that is all the surviving
// globals need.  Two measurements say so:
//
//   * 0x44294C..0x448000 is one unbroken run of zero bytes in BMF.exe's data
//     segment, and all of these globals are inside it.  There are no
//     initialisers to recover: this is bss.
//   * Everything below it is dead.  Filling 43 184 bytes of blob.inc with 0xCC
//     -- the whole segment under 0x44294C, bar the relocation slots -- leaves
//     all fifteen streams byte-identical.  The string tables down there belong
//     to the modes that are gone.
//
// The offsets stay because the code still strides between these globals with
// variable subscripts: `plane_desc[plane + 1].flags` walks four sixteen-byte
// records that Hex-Rays split into a name per field (REFACTORING.md Phase 3
// calls these SHARED, and all 41 are).  Giving them separate storage is what
// §3.6 is for; keeping one object is what makes it safe not to have done it
// yet.

alignas(16) static uint8_t ctx_group_flags[32] = {   // 0x439860
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};
// Bits 2..3 of a p2 context, permuted.  `alt_p2_model` reaches a neighbouring
// counter with `(ctx & ~0xC) + p2_ctx_rotate[(ctx >> 2) & 3]`, which sends
// {0,1,2,3} to {4,8,12,8} -- the fourth neighbour is the second again.
alignas(16) static int32_t p2_ctx_rotate[4] = {   // 0x439880
  4, 8, 12, 8,
};
// Quantiser edges: an ascending list, and the loop that reads it walks a value
// up from zero and steps to the next bucket when the value reaches the edge.
// `alt_p2_alloc` builds the p2 model's 0x82-entry context map from this one and
// its 0x3C-entry length map from the next.
alignas(16) static uint8_t p2_ctx_edges[16] = {   // 0x439890
  17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0,
};
alignas(16) static uint8_t p2_len_edges[32] = {   // 0x4398A0
  4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0,
};
// alt_p2_context's context thresholds: six rows of thirteen, chosen by how
// far the coded length has run past the plane's size.  BMF.exe kept them at
// 0x4398C0..0x4399F7 and Hex-Rays split the first twelve columns into twelve
// globals, each subscripted by the same `13 * row`, leaving the thirteenth as
// one array at 0x4398F0.  It is one table.
//
// The columns come in four runs -- 0..2, 3..5, 6..8, 9..12 -- each an ascending
// ladder compared against one of the four neighbourhood sums, and each turning
// into a small count that becomes part of the context index.
alignas(16) static int32_t bmf_p2_thresholds[6][13] = {
  {       6,      14,      28,       4,       8,      26,    4096,    6144,   13056,   10240,   14336,   26624,   36864 },
  {       4,      15,      42,       1,       8,      19,    2560,    6912,   11264,   10240,   28672,   71680,  139264 },
  {       5,      14,      21,       4,       9,      25,    3840,    8448,   12032,   16384,   26624,   43008,   83968 },
  {       6,      11,      20,       5,      11,      23,    2816,    5888,   10496,   22528,   32768,   51200,  131072 },
  {       0,      11,      30,       7,      13,      22,    4352,    8448,   14848,   16384,   38912,   57344,   94208 },
  {       7,      10,      31,      -1,      11,      21,    -256,    1024,    5888,   -2048,   -2048,   -2048,  145408 },
};
// `alt_p2_model`'s float constant pool: seventeen IEEE-754 single-precision
// bit patterns, which is what every one of these decodes to and what fifteen of
// them are also written as, verbatim, a few hundred lines below --
//
//     _this[14][1] = 169.2f;                                 // [ 0]
//     v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;          // [ 1] [ 7] [ 6]
//     const float floor_a = 26896.0f * f278656[14][2];       // [ 9]
//     ... / (f278656[7 + j][k] + ms_scale * 529.0f);         // [12]
//
// MSVC pooled the literals into .rdata and loaded them from here; Hex-Rays put
// them back inline, so nothing reads the pool as floats any more.  Only
// [ 2] -2784.44f and [ 8] 2.6f have no surviving literal, which is what the
// compression mode being a constant leaves behind -- the arithmetic that used
// them is in a branch this build folds away.
//
// It stays `int32_t` and keeps its address, because the one thing that still
// touches it does so a byte at a time and must keep reading the same bytes:
// `alt_p2_model` seeds `P2Count::b1` from `*((uint8_t *)&p2_float_pool + b0 + 3)`
// for a `b0` of 1..8, which is bytes 4..11 -- the low halves of [ 1] and [ 2],
// `00 00 80 3F 02 07 2E C5`.  Those are not counter seeds and were never meant
// to be; they are two floats read sideways.  It does not matter, and that is
// the point: `P2Count::b1` is written here and read nowhere, so the only use
// of this table in the whole program is a store that no one loads.
alignas(16) static int32_t p2_float_pool[17] = {   // 0x439B7C
  0x43293333,   // [ 0]   169.2f
  0x3F800000,   // [ 1]     1.0f
  (int32_t)0xC52E0702,   // [ 2] -2784.44f
  0x3A83126F,   // [ 3]   0.001f
  0x38D1B717,   // [ 4]   0.0001f
  0x3DCCCCCD,   // [ 5]   0.1f
  0x40000000,   // [ 6]   2.0f
  0x44100000,   // [ 7]   576.0f
  0x40266666,   // [ 8]     2.6f
  0x46D22000,   // [ 9] 26896.0f
  0x3C54FDF4,   // [10]   0.013f
  0x459D8800,   // [11]  5041.0f
  0x44044000,   // [12]   529.0f
  0x3951B717,   // [13]   0.0002f
  0x41200000,   // [14]    10.0f
  0x3F800000,   // [15]     1.0f
  0x3F800000,   // [16]     1.0f
};
// The p1 model's three quantiser edge lists, read the same way as
// `p2_ctx_edges`: `alt_p1_alloc` fills a 256-entry level map and a 256-entry
// group map from the first two together, and a 0x80-entry slot map from the
// third.
alignas(16) static uint8_t p1_level_edges[8] = {   // 0x439BC0
  1, 2, 4, 8, 14, 35, 103, 0,
};
alignas(16) static uint8_t p1_group_edges[8] = {   // 0x439BC8
  1, 3, 6, 10, 16, 27, 52, 0,
};
alignas(16) static uint8_t p1_slot_edges[8] = {   // 0x439BD0
  5, 10, 36, 98, 154, 236, 248, 0,
};
// The eight int32 its typedef always said it was.  deblob.py gave it the
// 30 024 bytes to the next global because that is the only upper bound the
// source offers; poisoning says the other 29 992 have no reader -- they were
// the string-table pointers the relocation layer used to rebase.
// How much a p1 counter goes up, by level.  `alt_p1_context` indexes it with
// the level it just resolved.
alignas(16) static int32_t p1_level_step[8] = {
  1, 1, 2, 2, 2, 4, 4, 4,
};
// The p2 filter's seven coefficient rows.  BMF.exe kept them at
// 0x441120..0x44118F, one xmmword each, and they are not constants: on the way
// into a run `alt_model_p2_decode`/`_encode` save all seven, fold rows 4..6
// into rows 0..2, zero rows 4..6, and restore the lot on the way out.  So rows
// 0..2 are the live coefficients, rows 4..6 the increments folded into them,
// and row 3 is never touched at all.
alignas(16) static float bmf_p2_coef[7][4] = {
  {    -0.05f,    -0.07f,      0.3f,     0.18f },   // 0x441120
  {    -0.03f,     0.04f,     0.06f,    -0.01f },   // 0x441130
  {     0.06f,     0.16f,      0.0f,    -0.01f },   // 0x441140
  {     -0.1f,    -0.02f,     0.05f,     0.05f },   // 0x441150
  {     0.06f,      0.0f,     0.04f,     0.02f },   // 0x441160
  {     0.04f,     0.02f,     0.04f,     0.05f },   // 0x441170
  {     0.07f,      0.0f,     0.03f,     0.02f },   // 0x441180
};
// The p2 model's seven learning rates, one four-lane row per input.  BMF.exe
// kept them at 0x441190..0x4411FF.  Rows 4..6 are not constants: a run starts
// by saving them and setting all three to `bmf_p2_rate_reset`, and puts them
// back at the end, so the last three inputs learn at a flat rate inside a run
// and at the schedule's rate outside it.
alignas(16) static float bmf_p2_rate[7][4] = {
  { 0.0108f, 0.0069f, 0.0054f, 0.0052f },   // 0x441190
  { 0.0042f, 0.0042f, 0.0042f, 0.0039f },   // 0x4411A0
  { 0.0035f, 0.0035f, 0.0028f, 0.0028f },   // 0x4411B0
  { 0.0027f, 0.0026f, 0.0026f, 0.0024f },   // 0x4411C0
  { 0.0023f, 0.0023f, 0.0022f, 0.0021f },   // 0x4411D0
  { 0.0019f, 0.0019f, 0.0017f, 0.0017f },   // 0x4411E0
  { 0.0015f, 0.0015f, 0.0011f, 0.0009f },   // 0x4411F0
};

// What a run resets rows 4..6 to.  BMF.exe had it twice, 0x439B50 for the
// encoder and 0x439B60 for the decoder, the same number in both, and passed it
// to `alt_p2_model` as that half's mean-square rate.
static const float bmf_p2_rate_reset = 0.0024f;

// How fast an input's running mean square follows it.  0x439B10.
static const float bmf_p2_ms_rate = 0.023f;
// One int32, which is what its typedef says.  It had the 1 968 bytes to the
// next global because that was the only bound available; poisoning says
// nothing reads past the first four.
alignas(16) static void *coded_block = nullptr;   // the out-of-band block a member can carry
alignas(16) static int32_t plane_predictor = 0;
alignas(16) static int32_t plane_alt_model = 0;
alignas(16) static int32_t packer_free_bits = 0;
alignas(16) static uint32_t packer_acc = 0;   // a bit accumulator: every shift of it is logical
alignas(16) static int32_t coded_size = 0;
static int32_t desc_slow_mode;   // was 0x443384 in bmf_bss
// Set by `reduce_alphabet` and cleared at the start of every plane; `alt_p2_model`
// takes a shorter path through the counters while it is set.
static int32_t alphabet_reduced;   // was 0x443388 in bmf_bss
// The plane-descriptor table: five 16-byte records, based at what used to be
// 0x44338C.  Record 0 holds the image-wide parameters and records 1..4 are the
// four planes, so the plane `p` a reader sees is record `p + 1`.  Three things
// say it is one table rather than twenty globals:
//
//   * the subscripts the code already writes -- `plane_desc[plane + 1].flags`,
//     `plane_desc[plane + 1].w4` -- both step whole records;
//   * field +1 is reached from two origins one record apart, `transform_planes`
//     pre-incrementing from 0 into `plane_desc[n].w0` and `rc_begin_encode`
//     indexing `plane_desc[p + 1].src_plane` by plane, which is what a header entry
//     in front of an array looks like;
//   * and `alt_model_p2_encode` walks all four plane records in one loop using
//     four of the record-0 names as its field bases.
//
// So `plane_count`, read as a scalar throughout, is field +8 of record 0.
//
// The union is the one thing here that is still a question rather than a
// layout: records 1..4 use +0..+3 as four separate bytes, and record 0 has its
// +0..+3 read as a whole dword by the packer bit accounting at 18433.
struct PlaneDesc {
  union {
    int32_t w0;
    struct {
      uint8_t predictor;   // `v23 & 3`, the plane's predictor number
      uint8_t src_plane;   // a plane number, fed back as `plane_desc[src_plane + 1]`
      uint8_t flags;       // & 3 predictor, & 4 alt model, & 8 a third mode
      uint8_t b3;
    };
  };
  int32_t w4;
  int32_t w8;
  int32_t w12;   // record 0: the near-lossless quantiser, `4 * w12 + 1` wide
};
static_assert(sizeof(void *) != 4 || sizeof(PlaneDesc) == 16,
              "PlaneDesc: the layout moved");
static PlaneDesc plane_desc[5];

// The table arrived as twenty separate globals, one per field per record, and
// round three bound all twenty to it as views so that the ~221 subscript sites
// could move a few at a time.  Nineteen of them are folded now; a subscript
// that stepped a whole record -- `[16 * p]` on a byte field, `[4 * p]` on a
// dword one -- says `plane_desc[p + 1]` instead.
//
// This one stays.  It is record 0's `w8` read as a scalar 149 times, always as
// the number of planes, and no other field is read that way.
static int32_t &plane_count = *&plane_desc[0].w8;
// Symbol -> level, one byte a symbol.  `rc_begin_encode` fills it one level a
// line, and the runs are the level sizes: one symbol at level 0, one at 1, two
// at 2, then 4, 8, 16, 32 and 64 -- which is `level_geom[n].first` and
// `2 * level_geom[n].half` said the other way round.  128 bytes exactly.
// It was `int32_t[32]` because MSVC folded the four shortest runs into two
// immediate stores; every reader indexes it by a byte.
static uint8_t model_geometry[128];   // was 0x445660 in bmf_bss
static int8_t exclusion_gen;   // was 0x445700 in bmf_bss
// The two symbol-list constants `alt_init_tables` picks from the predictor:
// the initial count a new symbol gets, and the count at which the list
// rescales.  (8, 8) under predictor 2 and (64, 16) otherwise.
static int32_t alt_freq_limit;   // was 0x44570C in bmf_bss
static int32_t alt_freq_init;   // was 0x445710 in bmf_bss
// The level geometry, and it is a table: eight 4-byte records with three
// bytes used in each.  `rc_begin_encode` writes records 2..7 out longhand, one
// level a line -- `level_geom[7].first = v6; level_geom[7].tbl_base = v6 - 7;
// memset(model_geometry + v6, 7, 64);` -- and the readers index it,
// `level_geom[n].first`, which is why the three record-0 globals existed
// twice: once as record 0 and once as the base the subscript walked from.
// Record 1 has no name because nothing read it on its own.
// Was 0x445714..0x445733 in bmf_bss.
struct LevelGeom {
  uint8_t first;      // the level's first symbol; readers compute `sym - first`
  uint8_t half;       // half the level's symbol count, and the count doubles
  uint8_t tbl_base;   // `first - level`, an index into a table of 16-bit words
  uint8_t _pad;
};
static LevelGeom level_geom[8];
// Four running bias terms, one per context sum.  `alt_model_p2_encode` zeroes
// them at the start of a plane, decays them `>>= 3` each row and re-accumulates
// them; `alt_p2_context` adds one apiece into the four neighbourhood sums it
// folds into a context word.  Were 0x4458E0..0x4458EC in bmf_bss.
static int32_t ctx_bias[4];

// The near-lossless dead zone.  `rc_begin_encode` sets these to +d and -d for
// d = 4 * plane_desc[0].w12 + 1, and every reader spells the same shape --
// `(x > deadzone_hi) - (x < deadzone_lo)`, a sign that is 0 inside the zone.
// Were 0x4458F0 and 0x4458F4 in bmf_bss.
static int32_t deadzone_hi;
static int32_t deadzone_lo;
// MSVC's CRT new-handler slot, and nothing else.  This was 10 292 bytes of
// CRT state because that is how far it was to the next global; one word of it
// is live -- `set_new_handler` writes it and `bmf_new` reads it (REFACTORING.md
// §6) -- and the other 10 288 bytes had no reader at all.
typedef void (*t_new_handler)();
static t_new_handler __pout_of_memory_handler = nullptr;

// bmf_addr, bmf_reloc_slots and bmf_data_relocate were here, with
// bmf_blob_relocate in blob.inc: an address-translation layer that let a global
// be reached by the virtual address it had in BMF.exe, and two startup passes
// that rebased the absolute pointers sitting inside the data.
//
// All of it is dead, and measured to be.  Taking both relocation calls out
// leaves every one of the fifteen streams byte-identical, because the 39
// pointers they rebased all point into the string tables under 0x44294C --
// which the poisoning experiment in REFACTORING2.md §3.1 shows nothing reads.
// They belong to the modes that are gone (REFACTORING.md §2.1).
// ---------------------------------------------------------------------------
// The compression mode.
//
// BMF read these six from its .ini and then from the command line; this program
// has one mode and always did -- `bmf c` pinned -S and -Q9 and let the other
// four keep the values BMF.exe's data segment starts them at.  They are
// constants here instead of blob words, so the compiler folds the branches that
// test them and everything only the other modes could reach becomes
// unreachable rather than merely unreached.  See REFACTORING.md §2.
//
// Their addresses in BMF.exe, for anyone comparing against a disassembly:
// 0x0044108C, 0x00441090, 0x00441094, 0x00441098, 0x0044109C, 0x004410A0.
// ---------------------------------------------------------------------------
static constexpr int32_t opt_use_filters = 1;   // -F  use filters
static constexpr int32_t opt_slow = 1;   // -S  slow but efficient
static constexpr int32_t opt_filter_template = 0;   // -T  filter template
static constexpr int32_t opt_pack_output = 1;   // -N  pack the output
static constexpr int32_t opt_search_quality = 9;   // -Q  filter search quality
static constexpr int32_t opt_max_error = 0;   // -E  max error, near-lossless
// The coded buffer and the two cursors into it, ALGORITHM.md §4.  These were
// int32_t words in the data segment holding addresses; they are pointers now,
// and out of the blob, because an address is not an int on a 64-bit target and
// because nothing reads the bytes they used to occupy.  Their addresses in
// BMF.exe were 0x00443374, 0x00443378 and 0x0044337C.
static uint32_t *packer_word;   // the word the bit packer is filling
static uint8_t  *out_cursor;    // the byte both the packer and rc advance
static uint8_t  *coded_buf;     // base of the buffer, from malloc
// The filter search's histogram scratch, ALGORITHM.md §8: a pointer parked at
// the top of the coded buffer.  Hex-Rays typed it int32_t[0x10000] and the code
// only ever touches element 0, where it keeps an address -- so it is one
// pointer, and out of the blob for the same reason as the cursors above.
// BMF.exe had it at 0x00443380.
static uint8_t *hist_scratch;
// The last two.  `tools/blob-independence.txt` marks both SHARED, and they are
// shared with each other: `exclusion_mask` is 8192 bytes and the 544 of
// `__byte_445440` follow it immediately, and giving either one storage of its
// own segfaults every multi-plane image.  Keeping them adjacent is what the
// evidence supports; which side reads across the boundary is still open, and
// finding it is what would let them separate.
alignas(16) static uint8_t bss_exclusion[8192 + 544];
static int8_t  (&exclusion_mask)[8192] = *(int8_t (*)[8192])bss_exclusion;
static uint8_t (&__byte_445440)[544]   = *(uint8_t (*)[544])(bss_exclusion + 8192);
// The model's counter tables, ALGORITHM.md §8: one allocation, handed out in
// 254-entry strips.  An int32_t in the data segment holding an address, so a
// pointer here, and out of the blob.  BMF.exe had it at 0x00445708.
static uint16_t *model_tables;
// One 254-entry strip of `model_tables`.  Every reader indexes it in whole
// strips: the constants 254, 2032, 4064 and 32512 are 1, 8, 16 and 128 of
// them, and writing the multiply once is what makes that visible.
static inline uint16_t *model_strip(uint32_t k) { return model_tables + 254 * k; }

// A five-entry table, and the extent is measured rather than assumed: the one
// site that subscripts it -- `*(uint16_t *)row_cur[5] = mode_symbol[n4]`, with `n4`
// out of `ModelBlock::hit` -- steps four bytes, and the four globals after
// 0x44573C were the elements it was stepping onto.  `init_model_tables` reads
// elements 1..3 as bare symbol values.  What the index means is not
// established, so the name still records the address rather than a role.
static int32_t mode_symbol[5];

// The four plane records, `plane_desc[1]` onwards, as a byte cursor.
//
// Six places copy all four records in or out with 64-bit moves, and Hex-Rays
// wrote those as offsets from four *other* globals -- coded_buf,
// desc_slow_mode, and the two dwords that are now `plane_desc[0].w0` and
// `plane_desc[0].w8` -- which sit 32, 24, 16 and 8 bytes below the records.
// That is the original compiler's strength reduction showing through, not
// something the program means; those four are a buffer pointer, a mode flag, a
// counter and a plane count, and none of them is a base for this.  Written
// against the records themselves the arithmetic is the same and the dependency
// on where four unrelated globals sit is gone.  REFACTORING.md §4.1.
//
// `off` runs 0..56, the 64 bytes of records 1..4, eight at a time.
// Sixteen zero bytes.  MSVC unrolled `memset(p, 0, n)` into aligned SSE stores
// and Hex-Rays wrote each one as `*(M128 *)p = 0`; three loops in this file
// are that, four to seven stores at a time.  The width is the store's, not a
// vector's -- nothing here reads sixteen bytes at once.
static inline void bmf_zero16(void *p) { __builtin_memset(p, 0, 16); }

static inline uint8_t *bmf_plane_desc(int32_t off)
{
  return (uint8_t *)&plane_desc[1] + off;
}

// ---------------------------------------------------------------------------
// The range coder.
//
// BMF's entropy coder, as it implements it: a carry-counting range coder over
// a 31-bit `low`, renormalised a byte at a time, in the Subbotin lineage.
// ALGORITHM.md §5 describes it in prose; this is the same thing as code.
//
// The state used to be six globals in the block above -- __n0x800000,
// __n0x7F800000, __dword_4456E8, __dword_4456EC, __byte_4456F0 -- and the
// arguments of a coding step three more, __n0x2000_1 / __n0x2000_0 /
// __n0x2000, which every caller assigned before calling an entry that took no
// parameters.  Both sets are members here, and the entries take arguments.
//
// It lives in this file rather than in bmf.cpp because it shares its output
// cursor with the bit packer: out_cursor is the one position both advance
// through, and that is a blob global declared above.
//
// Encoder and decoder never both run, so `low` doubles as the decoder's
// `code`, and `rdiv` occupies what is `pending` while encoding -- exactly as
// the donor overlapped them in one word.
// ---------------------------------------------------------------------------
__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...);

// One context weight.  `sel` is 0, 1 or 2 -- written from a pair of
// comparisons -- and chooses one of `w`; a model's index is its groups' chosen
// weights added together.  Both models are built this way and neither declared
// it: the p2 block has five groups and the p1 block nine, and in both the flat
// array Hex-Rays saw is read as `t[t[4g] + 4g+1]`, which is this record.
//
// `alt_p2_alloc` fills its five groups with
//
//   0  64  128     0  192  384     0  576  1152     0  1728  3456     0  5184  10368
//
// -- 0, 64*3^g and 128*3^g -- so the five selectors are the digits of a base-3
// number scaled by 64: 3^5 = 243 contexts, 64 counters apart.
//
// The encoder adds `w[sel]` and the decoder `w[2 - sel]`; the two are the same
// slot only when `sel` is 1, so the direction is each side's, not a symmetry.
struct CtxWeight {
  int32_t  sel;
  uint32_t w[3];
};
static_assert(sizeof(CtxWeight) == 16, "CtxWeight: a selector and three weights");

// The two runs of five are arrays, and the code says so rather than the
// offsets: alt_model_p1_decode rotates `row` by one place every pass --
//
//   v25 = row[4]; v26 = row[3]; v27 = row[2]; v28 = row[1]; v29 = row[0];
//   row[4] = v26; row[3] = v27; row[2] = v28; row[1] = v29; row[0] = v25;
//
// a five-deep ring of row pointers -- and then derives `cur` from it, three of
// the five offset by eight.  Naming ten consecutive `uint32_t` f176 through
// cursor[4] hid the one fact about them worth having.  The layout is unchanged:
// `row[0]` is still +176, and the static_assert says so.
//
// AltP1Block -- recovered from 260 dereferences over 54 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
// One counter node: a total and seven counts, the thing `init_counter_node`
// seeds with (22; 8, 2, 2, 2, 2, 3, 3) -- 22 being the sum of the other seven.
// `alt_p1_alloc` writes 0x99C60 of them at `_this + 8 * k + 1900` in
// `uint16_t`, which is +3800 and a stride of sixteen bytes.
struct CounterNode {
  uint16_t total;   // +0
  uint16_t c[7];    // +2 .. +15
};
static_assert(sizeof(CounterNode) == 16, "CounterNode: the record is sixteen bytes");

// The alternate p1 model's per-pixel record.  Two bytes, and both writers sit
// one line apart in `alt_model_p1_decode`: the reconstructed sample at +0 and
// `abs32(v56 - v54)` -- the size of the prediction error -- at +1.
// `alt_p1_alloc` takes `2 * width + 20`, the width plus ten records, seeds
// every one of them to (72, 0) and starts each cursor four records in.
struct P1Ctx {
  uint8_t sym;   // +0
  uint8_t mag;   // +1
};
static_assert(sizeof(P1Ctx) == 2, "P1Ctx: the record is two bytes");

struct AltP1Block {
  // Hex-Rays had two readings of these thirty-two bytes, `f0[8]` and `f4`/`f8`
  // with `f12[]` behind them, and the eight words are the same eight either
  // way: `f0[k]` is `f12[k - 3]` for k >= 3.  The reason it looked like two is
  // that `alt_p1_context` writes five of them through `*(uint8_t **)&f0[k]`,
  // which is the pointer costume the rest of that function wears.
  // The plane's geometry, the prediction the context step left behind, and the
  // five context words the model assembles.  `pred` is what `alt_p1_context`
  // computed for this pixel: every reader subtracts it from a neighbour's
  // sample, so it is the centre the residuals are measured from.
  int32_t width;    // +0
  int32_t height;   // +4
  int32_t pred;     // +8
  int32_t ctx[5];   // +12 .. +31
  // Nine weight groups, the shape `CtxWeight` describes.  Hex-Rays had them as
  // `f12[5 .. 40]` and the 144 bytes of `_u0_0_1` in the reading beside it;
  // `f12[6 + f12[5]]` -- group zero's `w[sel]` -- is what says they are
  // four words each and not thirty-six.
  CtxWeight ctx_w[9];   // +32 .. +175
  // Five p1 planes and a cursor into each: `alt_p1_alloc` allocates the five
  // and `alt_model_p1_decode` rotates them by one place a row, deriving each
  // cursor from its own plane.  Three readings had these forty bytes --
  // `f176[10]`, `f12[41 .. 50]`, and `cur[5]` with four more named pointers --
  // and all three rotate the same five and step the same five.
  P1Ctx *buf[5];       // +176 .. +195
  P1Ctx *cursor[5];    // +196 .. +215
  // `alt_p1_alloc` seeds five tables between +216 and +3800 and they tile the
  // range exactly, which is what says where each one ends: 216 + 512 = 728,
  // + 256 = 984, + 256 = 1240, + 256 = 1496, + 256 = 1752, + 2048 = 3800.
  // The first is a level map indexed by a byte, the second a slot map indexed
  // by seven bits, and the fourth `int32_t` pair holds a level and a group.
  // Three maps over a symbol, built by `alt_p1_alloc` from the three edge
  // tables beside them: `level_of[k]` is how many of `p1_level_edges` `k` has
  // passed, `group_of[k]` is the same against `p1_group_edges` with the plane
  // index in its high byte, and `slot_of[k]` is the same against
  // `p1_slot_edges`, times eight.  `alt_p1_context` reads all three with the
  // same symbol and folds them into `ctx[0]` and `ctx[1]`.
  uint8_t level_of[512];     // +216  .. +727
  uint8_t slot_of[256];     // +728  .. +983
  // A pair of inverse maps, the same pair in both blocks -- `alt_init_tables`
  // is handed `(fold, unfold)` by each allocator.
  //
  // `unfold` is built as 0, -1, 1, -2, 2, ... : `unfold[0] = 0`,
  // `unfold[4i + 1] = -(2i + 1)`, `unfold[4i + 2] = 2i + 1`, and the same one
  // further out for the even pair, ending at `unfold[253] = -127`,
  // `unfold[254] = 127`.  That is the zigzag that turns a signed residual into
  // a non-negative symbol, and `fold` is the way back: the coders read
  // `n = fold[residual]`, code `n`, and reconstruct with `unfold[n] +
  // predictor`.  Neither table's name says more than that pairing.
  //
  // `fold_hi` is the 256 bytes after `fold`, and the two are filled by one
  // call: a sentinel written over `fold_hi` immediately before
  // `alt_init_tables` is gone immediately after, at both ends of the range and
  // in both blocks.  So the write extent is 512 bytes and the split is the
  // readers' -- some index `fold`, some `fold_hi`, none crosses.
  //
  // What separates the two index spaces was left open here for several rounds.
  // It is nothing: both halves are byte-indexed maps of the same residual to
  // the same code, and in this build they hold the same 256 bytes.  Reading
  // `alt_init_tables` says why -- the quantiser walk fills `fold[0..255]` and
  // the closing loop fills `fold_hi[b] = code` by inverting `unfold` outright,
  // and with `-E` at 0 the quantiser's buckets are one residual wide, so the
  // walk computes that same inverse the long way round.  Measured rather than
  // argued: a probe comparing the two halves at the end of every
  // `alt_init_tables` call reports 0 of 256 differing, on every test stream.
  //
  // They stay two members because they are two *objects* -- the near-lossless
  // build would fill them differently, and a reader that indexes `fold_hi`
  // is asking for the exact inverse while one that indexes `fold` is asking
  // for the quantised one.
  uint8_t fold[256];     // +984  .. +1239
  uint8_t fold_hi[256];    // +1240 .. +1495
  uint8_t unfold[256];   // +1496 .. +1751
  int32_t group_of[512];    // +1752 .. +3799
  // The counter table.  Hex-Rays read it as a `P1Count` grid anchored at the
  // object's base -- `_this->counters[k]` -- which is the right bytes
  // at the wrong phase: `P1Count::total` sits at +8, and 16 * 237 + 8 is 3800,
  // so record `k + 237`'s `total` and `bin[0..2]` are node k's four words and
  // its `w[0..3]` are node k-1's last four.  That is why `w[]` is never read
  // at those indices and `bin[]` never past 2.  The extent is the allocator's
  // own loop bound, `n0x99C60 < 0x99C60`, and 0x99C60 * 16 fills the object.
  CounterNode counters[629856];   // +3800 .. +10081495
};
static_assert(sizeof(void *) != 4 || sizeof(AltP1Block) == 0x99D4D8,
              "AltP1Block is not what alt_p1_alloc's callers allocate");
// Where the weights stop and the planes start, so a fifth reading of these
// 216 bytes cannot creep back in.
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP1Block, ctx_w) == 32,
              "AltP1Block::ctx_w: nine weight groups at +32");
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP1Block, buf) == 176,
              "AltP1Block::buf: five planes at +176");
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP1Block, cursor) == 196,
              "AltP1Block::cursor: five plane cursors at +196");
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(AltP1Block, cursor[4]) == 212,
              "AltP1Block: the layout moved");


// ModelBlock -- the model block, and the only recovered object whose role is
// established: the allocation rc_begin_encode and rc_begin_decode make,
// reached here at offsets 0x64 to 0x5C75AC.  See REFACTORING.md section 4.2.  The other structs keep their
// ObjN names because naming their fields waits on ALGORITHM.md section 9.
//
// Recovered from 174 dereferences over 25 offsets, under 5
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
// Sixteen bytes: eight words, of which the last is only ever two bytes.  The
// union is not a choice between two readings -- word 7 is touched as `+14` and
// `+15` and never as a word, and the two spellings sit on the same bytes
// because the code uses both.
//
// One record type, two roles, one array.  `code_pixel` and `decode_pixel` copy
// a whole record with two 64-bit moves, which is what fixes the size; they
// reach it as `&((uint32_t *)this)[4 * k + 776]`, grid record k + 188.
// `model_plane` and `unmodel_plane_slow` fill records 0..188 as context
// buckets -- five counts in `w[0..4]`, their total in `w[5]`, a scaled weight
// in `w[6]`, and the level and its weight in the two bytes.
struct FreqRec {
  union {
    uint16_t w[8];   // +0 .. +15
    struct {
      uint16_t _w0to6[7];
      uint8_t  b14;  // +14, multiplied by 8 on every refill
      uint8_t  b15;  // +15
    };
  };
};
static_assert(sizeof(FreqRec) == 16, "FreqRec: the record is sixteen bytes");
static_assert(__builtin_offsetof(FreqRec, b14) == 14, "FreqRec: the byte moved");

// A symbol list.  `init_symbol_list` allocates `3 * n` bytes for the entries
// and fills them with (symbol, 1); `symbol_list_update` promotes one entry
// towards the front and halves the counts when they run out.  The header is 24
// bytes, which is what every caller's `+ 24 * k` says.
#pragma pack(push, 1)
struct SymEntry {
  uint16_t sym;
  uint8_t  cnt;
};
#pragma pack(pop)
static_assert(sizeof(SymEntry) == 3, "SymEntry: the record is three bytes");

struct SymList {
  // Unsigned because `symbol_list_update` read them through a `uint32_t *` and
  // every one of them counts something; `init_symbol_list` took the same bytes
  // as `int32_t *` and its comparisons are unsigned either way.
  uint32_t  n;        // +0   the alphabet size
  uint32_t  live;     // +4   entries in use
  // The three numbers the list's own rescale runs on.  `tot` is the sum of
  // every entry's count -- it is what the range coder is handed as its total,
  // and `symbol_list_update` adds to it on every new entry and adds back one
  // for each zero-count entry it prunes.  `since_rescale` accumulates the
  // weight each update carries (`a3`, plus 4 for a new entry) and `rescale_at`
  // is the ceiling it is tested against; when it is passed, every count is
  // halved and so are these two.  `rescale_at` is `8 * n` or `20 * n` at init,
  // and which of the two also decides whether the halving rounds up.
  uint32_t  tot;           // +8
  uint32_t  since_rescale; // +12  12 * n at init
  uint32_t  rescale_at;    // +16  8 * n at init
  SymEntry *ent;      // +20
};
static_assert(sizeof(SymList) == 24, "SymList: the header is 24 bytes");

// What `bmf_new(24 * n + 4)` returns: the count, then `n` lists.  Four sites
// build one and step past the count with `(SymList *)(p + 1)`; `free_workspace`
// reads the count back from the word before the first list and frees that word.
// Only the allocating end holds the block; everything downstream is handed
// `list`, which is why the count is reached backwards there rather than named.
struct SymListBlock {
  uint32_t n;         // +0
  SymList  list[0];   // +4
};
static_assert(__builtin_offsetof(SymListBlock, list) == 4,
              "SymListBlock: the count word comes first");

// The count that `bmf_new` left in the word before `list[0]`.
static inline uint32_t sym_list_count(const SymList *list) {
  return ((const uint32_t *)list)[-1];
}

// The block a `SymList *` came out of -- what `free` is given.
static inline SymListBlock *sym_list_block(SymList *list) {
  return (SymListBlock *)((uint32_t *)list - 1);
}

// A binary counter pair: two `uint16_t`, both seeded 0x2000 -- p = 1/2 for a
// fresh counter.  `layout_workspace` writes 0x10000 of them per context group.
// Not a counter pair, whatever the name says: the two most recent symbols seen
// in one context.  `init_model_tables` writes `pix_cur[1] = pix_cur[0]` and then
// `pix_cur[0] = <this pixel>` with `pix_cur` pointing at one of these, which is
// a two-entry move-to-front; both coders read them back as candidates and score
// a hit on `last` at 15 and on `prev` at 75.  The seed is (0x2000, 0x2000),
// which is one past the largest thirteen-bit symbol -- a value no pixel can
// have, so a fresh context matches neither.
struct SymPair {
  uint16_t last;
  uint16_t prev;
};
static_assert(sizeof(SymPair) == 4, "SymPair: the record is four bytes");

// One adaptive binary counter: a count per bit value, and the total at which
// the pair is rescaled.  `layout_workspace` seeds them (40, 16, 512) or
// (4, 4, 72), and the four tables that hold them are all indexed `3 * k`.
//
// `encode_context_bit` codes with `rc.encode_bit(n[0], n[1], bit)`, adds 8 to
// `n[bit]`, and when `n[0] + n[1]` passes `limit` halves both and raises
// `limit` by 64 -- to a ceiling of 0x4000, so the counter forgets faster while
// it is young and settles as it ages.
//
// A record has three states, and the first two are why `n` is an array.  Cold
// is `n[0] == 0`: the coder falls back on the second counter it was handed,
// codes from that, and leaves `n[0] = bit + 1`, so `n[0]` is *the first bit
// seen plus one* and not a count at all.  Half-warm is `n[1] == 0`: `n[0]` and
// `n[1]` are seeded from the fallback's ratio scaled to 64, `limit` is set to
// 512, and `n[n[0] - 1] += 4` bumps the bit that cold state remembered.  Live
// is everything after.
//
// That reach only lands inside the record while `n[0]` is 1 or 2, which the
// seeds actively contradict -- 40 and 4.  It holds because it is reachable
// only from half-warm: a `__builtin_trap()` on `n[0] > 2` there fires on none
// of the fifteen images.
struct BitCtr {
  uint16_t n[2];    // +0 .. +3
  uint16_t limit;   // +4
};
static_assert(sizeof(BitCtr) == 6, "BitCtr: two counts and a rescale limit");
static_assert(__builtin_offsetof(BitCtr, limit) == 4, "BitCtr: the limit is last");


// The pixel model's per-pixel record, and the unit every pointer in
// `ModelBlock::row_cur` steps.
//
// `code_pixel` writes the symbol at +0 and then six comparisons of it against
// six neighbours' symbols at +2..+7 -- the match state `ALGORITHM.md` §8.2
// describes.  All six have readers now that the row pointers count records.
// `match[0]` -- "same as the pixel above" -- is what `grad[0]`, `grad[1]` and
// `grad[3]` slide an eight-record window over, off `row_cur[6]`, `row_cur[7]` and the
// current row.  `match[1]` -- "same as the pixel to the left" -- is `grad[2]`'s
// four-record window, and it is also what both coders scan record by record to
// find how far a run of one colour reaches.  `match[2..5]`, the up-left,
// up-right and two-away comparisons, are read one record and two records back
// and weighted 2, 4, 8, 16 and 32 into the context indices the coders build.
struct PixRec {
  uint16_t sym;        // +0
  uint8_t  match[6];   // +2 .. +7
};
static_assert(sizeof(PixRec) == 8, "PixRec: the record is eight bytes");

struct ModelBlock {
  // The plane's geometry.  `reduce_alphabet` sizes every buffer it takes as
  // `height * width` and `layout_workspace` is called with both.
  uint32_t width;
  int32_t height;
  // The plane's bit depth, twice, and the alphabet that survived reduction.
  // `layout_workspace` seeds both depths from the header; `reduce_alphabet`
  // and `expand_alphabet` then set `depth` to 8 when the symbols fit in a byte
  // and leave `depth_raw` alone, which is what `v66 != depth` and
  // `v40 == depth_raw` are comparing.  `alphabet` is the count `reduce_alphabet`
  // arrives at -- `bmf_new(alphabet)` and `while (k < alphabet)` size the map,
  // `(alphabet + 15) >> 4` is how many sixteen-symbol blocks it takes, and
  // `alphabet < 32` is what gates the third level of `ctx_id`.
  uint32_t depth;
  uint32_t depth_raw;
  uint32_t alphabet;
  uint32_t ctx_id1_used;
  uint32_t ctx_id2_used;
  uint32_t ctx_id3_used;
  // Did the last binary decision say yes?  `decode_pixel` writes the bit it
  // just decoded here and reads it back three statements later, and
  // `init_model_tables` reads it to decide whether the symbol lists get the
  // match promoted into them.  It is the coder's answer to "was this
  // candidate the pixel", carried across a call rather than returned.
  uint32_t hit;
  // Write-only, like the three in `AltP2Block`: both coders store the
  // neighbourhood word here once per pixel and nothing reads it back, and
  // no computed reach lands on +36.  It stays because the layout does.
  uint32_t f36;
  // Which of `grid`'s 188 context buckets this pixel selected.  The only
  // reader is `&row_cur[4 * bucket_idx + 10]`, which in `uint32_t` steps is
  // +3104 + 16 * bucket_idx -- record `bucket_idx + 188` of the grid, the
  // frequency record the coder then walks.
  uint32_t bucket_idx;
  uint32_t sym_pos;   // 0..31; the index pixel_context reads sym[] with
  // The two counter indices `pixel_context` leaves behind, which is what
  // `code_pixel` hands to `encode_context_bit(node, fallback, bit)`:
  // `bit_node[ctr_node]` and `bit_root[ctr_fallback]`.  `ctr_node` is
  // `(sym_pos << 7) + <seven bits of neighbourhood agreement>`, so twelve
  // bits into the 4096-record table; `ctr_fallback` is three of those bits
  // plus 8 if the candidate is past the ninth, so four into the 16-record
  // one.  A context with no history codes from the fallback.
  uint32_t ctr_fallback;
  uint32_t ctr_node;
  // Ten pointers into the plane's five row buffers.  `row_cur[0..4]` are the
  // buffers, rotated one step per row -- what was the current row becomes
  // the row above -- and `row_cur[5..9]` are the cursors the coders walk: the
  // current row and the four above it.  Each buffer opens with an eight-
  // record left margin -- `layout_workspace` sets `row_cur[j + 5] = row_cur[j] + 8`
  // -- which is why the rotation sets every cursor to `buffer + 7` and then
  // steps it once more before the row starts.  Nothing reaches
  // through these as bytes: every offset that looked raw -- 56, `8 * k`,
  // -16, 27, 19, 11, 3, -5, -21, -29 -- is a record and a field in it.
  PixRec   *row_cur[10];   // +56 .. +95
  // One grid of sixteen-byte records, and two walkers over it.  Records 0..188
  // are the context buckets `model_plane` and `unmodel_plane_slow` fill -- five
  // counts, their total, a scaled weight, and two bytes -- and records 188 and
  // up are the frequency records `code_pixel` and `decode_pixel` walk, reached
  // as `(FreqRec *)&((uint32_t *)this)[4 * k + 776]`, which is +3104 + 16k.
  // Record 188 is both: the bucket loop's last iteration seeds the frequency
  // table's first record.  The bucket count is measured -- a `__builtin_trap`
  // on `>= 188` fires on fourteen of the gate's streams and one on `>= 189`
  // fires on none -- and the extent runs to the next declared member.
  FreqRec  grid[65723];   // +96 .. +1051663
  // Two functions copied `row_cur[10..13]` here after the bucket loop and nothing
  // ever read it back -- and `row_cur[10..13]` was `grid[0]`, so the copy was
  // moving a bucket record into a slot with no reader.  Both are gone; the
  // sixteen bytes stay to hold the layout.
  uint8_t  _pad1051664[16];   // +1051664 .. +1051679
  // What `layout_workspace` seeds, and the boundaries are its own: sixteen
  // three-word records at +1 051 680, a 24 KiB block it `memset`s, one record
  // and 1536 bytes it `memset`s at +1 076 352, and 48 more records at
  // +1 077 894.  Each record is (40, 16, 512) or (4, 4, 72), which is two
  // counts and a total -- the unit `encode_context_bit` takes a pointer to,
  // which is why every reader of these four indexes them `3 * k`.
  // The counter pair `encode_context_bit(node, root, bit)` takes.  A pixel's
  // context picks `bit_node[ctr_node]` out of 4096 and `bit_root[ctr_fallback]`
  // out of 16; a node with no history codes from the root and remembers the
  // bit, and a half-warm one seeds itself from the root's ratio.  The 4096
  // start at zero and the 16 at (40, 16, 512).
  BitCtr   bit_root[16];      // +1051680 .. +1051775, 16 records
  BitCtr   bit_node[4096];   // +1051776 .. +1076351, 4096 records
  // The escape ladder's own counters: `esc_ctr[0]` is the root every one of
  // them codes against, seeded (4, 4, 72), and the other 256 start at zero.
  // The index is `8 * <run bucket> + 4 * <two neighbour flags> + 2 * <flag>
  // + <alphabet slot> + 1`, which is why the reads are all `+ 1`.
  BitCtr   esc_ctr[257];     // +1076352 .. +1077893, 257 records
  // Three groups of sixteen, and the grouping is what the reaches said:
  // `((uint32_t *)block)[269473 + 24 * a + 24 * b]` is +1 077 892 plus 96
  // bytes a step, and 96 bytes is sixteen six-byte records.  The two
  // conditions -- "is this the first pass" and "does the run length match
  // the one already coded" -- pick the group, and the run length picks the
  // record inside it.  Every one starts at (4, 4, 72).
  BitCtr   run_ctr[48];      // +1077894 .. +1078181, 3 groups of 16, 48 records
  uint8_t _pad16[2];   // +1078182 .. +1078183
  // A twenty-fourth symbol list, inside the object rather than in the array
  // beside it: `init_model_tables` initialises `_this + 1078184` and
  // `escape_list` holds its address, so the `void *` that used to sit at +1078204
  // was this list's `ent` -- which is why `free_workspace` frees it on its own.
  SymList escape;      // +1078184 .. +1078207
  // Two arrays of symbol lists, both indexed by a symbol -- `mode_symbol[1]`,
  // `mode_symbol[2]` and the pixel at `row_cur[5]` are what reach them -- and
  // named for the `sel` slot each feeds, which is the only thing that
  // distinguishes them here.
  SymList *sel1_list;   // +1078208, chosen into `sel[1]`
  SymList *sel0_list;   // +1078212, chosen into `sel[0]`
  // The lists the current context selects -- `sel[0]` out of `sel0_list` and
  // `sel[1]` out of `sel1_list`.  Two adjacent members of one type that
  // `sel_cur` walks as an array, which is why the reset is spelled
  // `_this + 1078216` and the end test compares against the same address.
  SymList *sel[2];       // +1078216, +1078220
  SymList *escape_list;   // always `&escape`
  uint8_t _pad22[4];
  SymList **sel_cur;    // the cursor over `sel`
  // A `uint16_t` per symbol -- `bmf_new(2 * f4 * f8)` -- and every reader
  // indexes it `*(uint16_t *)(p + 2 * k)`.
  uint16_t *sym_word;
  // The alphabet map: one 24-bit symbol code per entry.  `expand_alphabet`
  // builds it as whole words -- `= j`, `+= v20 << 8 * k`, `= v27 + v25` -- and
  // `unmodel_plane_slow` reads a word back per pixel.
  uint32_t *sym_code;
  // Two tables the plane setup fills, and both loops give their own bounds.
  // `ctx_state` inverts `ctx_group_flags`: it stores `s` at
  // `ctx_group_flags[s]` for the fifteen states, and the largest entry there
  // is 63, so it is 64 bytes and ends where the next table starts.
  // `ctx_bucket` is [5][5][15] -- the two four-way neighbour matches and the
  // state -- which is why every index into it arrives as
  // `state + 15 * a + 75 * b` and why the writers step a base by one per state.
  uint8_t ctx_state[64];     // +1078244 .. +1078307
  uint8_t ctx_bucket[375];   // +1078308 .. +1078682
  uint8_t _pad25[1];         // +1078683
  // Run length to bucket, `width + 1` bytes.  `layout_workspace` fills it
  // with `v12 += v13 == 2 << v12`, so entry `k` is how many powers of two
  // `k` has passed: 0, 1, 2, 2, 3, 3, 3, 3, ... -- the base-2 magnitude of a
  // run.  Both coders read it with the length of the same-colour run they
  // just scanned and weight the answer by eight into a counter index.
  uint8_t *run_bucket;
  uint8_t *alpha_map;   // the alphabet map, one byte a symbol
  // Four directional differences of causal neighbours, accumulated a pixel at
  // a time.  What is established is how they are read: `code_pixel` takes the
  // four zero-flags as bits 4..7 of a context, so what matters about each is
  // whether its direction is flat.
  uint8_t grad[4];   // +1078692 .. +1078695
  // `layout_workspace` seeds this one 0x40000 times, two counters an
  // iteration, both 0x2000.
  uint16_t sym_ctr[524288];   // +1078696 .. +2127271
  // Fifteen context groups of 0x10000 counter pairs.  `model_plane` and
  // `unmodel_plane_slow` seed every one to (0x2000, 0x2000) through a base of
  // `&((uint32_t *)this)[0x10000 * group]` and an index of `entry + 531818`,
  // which is +2 127 272 -- and 15 * 0x10000 * 4 is 3 932 160, this member's
  // whole extent.  The entry is the 16-bit context `code_pixel` derives from
  // the neighbourhood; the group is `ctx_state[flags]`.
  SymPair group_ctr[15][65536];   // +2127272 .. +6059431
  // The symbol cache `code_pixel` promotes through: a cursor into
  // `sym_ctr`, set to `&sym_ctr[8 * ctx]` and walked as `[0..6]`.
  uint16_t *sym_cache;
  // A row cursor, and every read and write through it is a `uint16_t`:
  // `[0]` is this position and `[1]` the one ahead.
  uint16_t *pix_cur;
  // The four regions `layout_workspace` ends on.  Each extent is the loop
  // bound or the memset length that fills it, and the four of them run to
  // 8102448 -- which is the 0x7BA230 both callers ask `bmf_new` for, so the
  // last one ends exactly at the end of the object.
  //
  // `sym_rev[k]` is the low thirteen bits of `k` reversed, times eight:
  // `layout_workspace` builds it with `v += v + (k & 1)` thirteen times and
  // then scales the lot.  Its three readers subtract a neighbouring symbol
  // from it and index `sym_ctr[8 * that]`, so reversing is what spreads
  // consecutive symbols across the counter array instead of stacking them.
  // It was reached as `*((uint16_t *)block + k + 3029720)`, and 3029720 words
  // is this member's own offset -- the six reads were the table all along.
  uint16_t sym_rev[8192];   // +6059440 .. +6075823
  // Three levels of interned context id, each `0xFFFF` until claimed.  A
  // pixel's neighbourhood signature indexes `ctx_id1`; if the slot is unused it
  // takes the next id from `ctx_id1_used`.  That id, shifted up three and
  // joined with three more match flags, indexes `ctx_id2`; that id, shifted up
  // four and joined with four bits of the symbol, indexes `ctx_id3` -- and the
  // third level is entered only while the alphabet is under 32 symbols.  So a
  // context is not a number the coder computes but a number it *hands out*,
  // and identical neighbourhoods share one however they were reached.
  //
  // `ctx_id3` has a ceiling: past 53248 ids the index is or-ed with 15, which
  // collapses every further context into the last slot of its group rather
  // than growing the table.
  uint16_t ctx_id1[192512];   // +6075824 .. +6460847
  uint16_t ctx_id2[108800];   // +6460848 .. +6678447
  uint16_t ctx_id3[712000];   // +6678448 .. +8102447
};
static_assert(sizeof(void *) != 4
              || (__builtin_offsetof(ModelBlock, pix_cur) == 6059436
                  && __builtin_offsetof(ModelBlock, sym_ctr) == 1078696
                  && __builtin_offsetof(ModelBlock, sym_rev) == 6059440
                  && __builtin_offsetof(ModelBlock, ctx_id1) == 6075824
                  && __builtin_offsetof(ModelBlock, ctx_id2) == 6460848
                  && __builtin_offsetof(ModelBlock, ctx_id3) == 6678448
                  && sizeof(ModelBlock) == 0x7BA230),
              "ModelBlock: the layout moved");


// One p2 counter.  `alt_p2_alloc` resets every one of them to b0 = 5, b1 = 2,
// w2 = 0; `alt_p2_model` raises b0 by one per update while it is under 8 and
// rescales w2.  Every reader says the same thing -- `p2_pred` below -- so b0 is
// a shift and w2 is what it scales.  The signedness is each site's: b0 and w2
// are read signed, b1 is written unsigned and never read.
//
// b1 is the odd one.  It is written on every update, from a byte of
// `p2_float_pool` indexed by the new b0 -- which is the middle of a float
// constant, not a table of seeds -- and nothing in the program reads it back.
// A store whose source is meaningless and whose value is never loaded is not a
// puzzle to solve; it is a field this build does not use.
struct P2Count {
  int8_t b0;
  uint8_t b1;
  int16_t w2;
};
static_assert(sizeof(P2Count) == 4, "P2Count: the record is four bytes");

// One p2 frequency record: an adaptive step and three symbol frequencies,
// seeded (4096; 2048, 2816, 2816).
//
// `alt_p2_encode_symbol` reads it as a three-way alphabet -- `rc.encode` gets
// a cumulative pair out of `f[0 .. 2]` and `f[0] + f[1] + f[2]` as the total --
// and ends with `*chosen = step + *chosen`.  So `step` is not a fourth count:
// it is the amount an update adds, and the rescale that fires when a count
// passes 0x4000 halves the three frequencies and *lowers* `step` -- by half
// above 256, by 32 above 32, by 2 or 0 below that.  A learning rate that
// decays as the record matures.
//
// Both halves of that are checked rather than read off: a `__builtin_trap()`
// on the addend differing from `step`, and one on the halving branch failing
// to lower it, fire on none of the fifteen images.
struct P2Freq {
  uint16_t step;   // +0
  uint16_t f[3];   // +2 .. +7
};
static_assert(sizeof(P2Freq) == 8, "P2Freq: a step and three frequencies");
static_assert(__builtin_offsetof(P2Freq, f) == 2, "P2Freq: the step comes first");


// A counter's prediction: `w2` rounded and shifted right by its own rate.
// `1 << ((rate + 31) & 31)` is the rounding half-step written the way MSVC
// emits it, and the `& 31` on the shift is x86's own masking showing through.
// 150 sites, in nine spellings that differ only in how the counter and the
// rate were reached -- including three that add the two terms the other way
// round, and five where the rate arrived through a `LOBYTE` copy.  Both masks
// are mod 32, so only the byte such a copy writes can reach the shift.
//
// `rate` is `int32_t` and not `int8_t` because a third of the sites reach b0
// through a widened temporary; `-Wsign-conversion` is what checks that none of
// them passes an unsigned value, which would have made `>>` a logical shift.
static inline int32_t p2_pred(int32_t w2, int32_t rate) {
  return (w2 + (1 << ((rate + 31) & 31))) >> (rate & 31);
}

// The counter update, and the file's most repeated expression: 117 sites.
//
// `err` is the prediction error.  A dead zone around zero contributes +-32 --
// `deadzone_hi` and `deadzone_lo` are the edges -- and the rest is the error
// scaled down by `shift`, with the rounding term `1 << (shift - 1)` that MSVC
// emitted as the constant 2 or 4.
//
// The arithmetic stays unsigned, which is what the `(uint32_t)` on the second
// comparison made it.  For a negative sum an unsigned shift and an arithmetic
// one differ only above bit 29, and the result is kept in sixteen bits, so the
// low half is the same either way -- but writing it unsigned keeps that a fact
// about this function rather than an assumption at 117 call sites.
static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u * (uint32_t)((err > deadzone_hi) - (err < deadzone_lo));
  return (int16_t)((uint32_t)w2 + ((kick + (uint32_t)err + (1u << (shift - 1))) >> shift));
}

// The p2 model's neighbourhood table: eighteen bytes a record, rows 144 bytes
// apart -- eight records to a row.  `alt_p2_context` reaches records -2 .. +4
// of the cursor it is given.
//
// The last two bytes are not a ninth lane.  Both pixel bodies end a record
// with `cursor[17] = 2` and `cursor[16] = (lane[2] <= 0) + (lane[2] < 0)`, and
// `alt_p2_model` overwrites them with `abs32(err)` and a comparison.
//
// `mag` is read 32 times, always summed over a neighbourhood.  `sign` is read
// exactly once, and that once is the thing worth having: `alt_p2_context`
// assigns it to `ctx_w[4].sel`, so the three-way sign of a record's gradient
// *is* the fifth base-3 digit of the next context (§9.1).  Hex-Rays had that
// read as `(uint8_t)lane[8]` -- one past the eight lanes -- which is how it
// stayed hidden while the record claimed nine of them.
//
// What the eight lanes hold is what the decoder writes as it emits a pixel:
//
//   lane[0] = pixel * 16
//   lane[1] = the same, and zeroed again at the start of the next row
//   lane[2] = lane[0] - the previous record's lane[0], signed
//   lane[3] = lane[5] = lane[6] = lane[7] = |lane[2]|
//   lane[4] = |lane[2]| / 2
//   sign    = the three-way sign of lane[2], 0 / 1 / 2
//   mag     = 2
//
// so a fresh record is a pixel, its gradient, and five copies of that
// gradient's magnitude that then diverge -- `alt_p2_model` writes lanes 1..7
// separately afterwards.  Five is `bank_ctx`'s five and the five planes' five;
// whether those threes are the same five is not established.
struct P2Ctx {
  int16_t lane[8];   // +0 .. +15
  uint8_t sign;      // +16
  uint8_t mag;       // +17
};
static_assert(sizeof(P2Ctx) == 18, "P2Ctx: the record is eighteen bytes");
static_assert(__builtin_offsetof(P2Ctx, mag) == 17, "P2Ctx: mag is the last byte");


// AltP2Block -- recovered from 353 dereferences over 39 offsets, under 23
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct AltP2Block {
  uint8_t _pad0[278528];
  // 336 bytes, 278528..278863, and four readings of them.  The extent is the
  // distance to `nb_sum`, not a measurement of any one reading: Hex-Rays had
  // a fifth, `__m128 f278528[21]`, which was its record of the sixteen-byte
  // access unit the original used to reach all of this, and every one of that
  // one's 175 sites is one of the four below at a fixed offset.
  union {
    // The seven rows of four the p2 model predicts from.  `alt_p2_context`
    // fills all 28 with differences of neighbouring pixels; `alt_p2_filter`
    // takes `&f278528[0]` as its `a2` and multiplies the seven rows against
    // seven coefficient rows; `alt_p2_model` runs the same loop twice more.
    // Every one of those is `for j < 7, for k < 4`, which is what says this is
    // an array and not 28 scalars.
    float p2_row[7][4];   // +278528 .. +278639
    struct {
      uint64_t f278528_q;
      // Three scalars every one of the four p2 bodies clears at a row start
      // and nothing reads -- twelve writes, no readers, and no computed reach
      // lands on +278536, +278540 or +278542 either.  They stay because the
      // layout does; the observation is that a row start in this model
      // resets more state than it uses.
      uint32_t f278536;
      uint16_t f278540;
      uint8_t f278542;
      uint8_t _u0_0_4[97];
      // Four floats, and only three have a reader: `alt_p2_filter` takes
      // `bias[0]` and `bias[1]`, `alt_p2_context` takes `bias[2]`, and all
      // four are cleared together at the start of a plane.  Hex-Rays had them
      // as two `uint64_t` because that is how the clearing writes them.
      float bias[4];   // +278640 .. +278655
      float (*f278656)[4];   // a weight block: 16 rows of four, `alt_p2_filter`'s `_this`
      // Two row buffers out of one `bmf_new(4 * i + 16)`, swapped once a row,
      // and two cursors into them: `cur` walks the row being written and
      // `above` the one before it, both re-derived as `row + 2` after the
      // swap.  The six neighbours `alt_p2_context` reads are `cur[-2 .. 0]`
      // and `above[0 .. 2]` -- three behind on this row, three at and ahead on
      // the last.
      int32_t *row0;    // +278660
      int32_t *row1;    // +278664
      int32_t *cur;     // +278668
      int32_t *above;   // +278672
      // One per bank: `alt_p2_model` runs five passes and reads
      // `bank_ctx[n5]` at the top of each, which is what says these five
      // scalars are an array.  `alt_p2_context` writes them one at a time.
      int32_t bank_ctx[5];   // +278676 .. +278695
      int32_t f278696;   // no reader, and no writer either
      // The filter's last prediction.  `alt_p2_context` leaves its output
      // here and `alt_p2_model` reads it one pixel later as
      // `lane[2] = sample - pred_prev`, which is the residual the record
      // carries and the sign of which is one of the five context digits.
      uint16_t pred_prev;
      uint8_t _u0_0_12[2];
      uint32_t ctx;
      // Two indices derived from `ctx`, one per parity of a bit the coders
      // are handed alongside the record: `alt_p2_encode_symbol` reads
      // `a2[a3 & 1]`, and `alt_p2_context` writes both from `ctx_delta`.
      uint32_t ctx_pair[2];   // +278708 .. +278715
      int32_t nb_id_used;
      // The context band: `alt_p2_alloc` sets it to -(16q + 7) .. 16q + 8 for
      // `q = plane_desc[0].w12`, and three of the five weight groups classify a
      // difference against it as `(d <= band_hi) + (d < band_lo)` -- above,
      // inside, below, which is a base-3 digit.  The same `q` sets
      // `deadzone_hi`/`deadzone_lo` to +-(4q + 1) two lines earlier, so the
      // model carries one tolerance at two scales: the narrow one decides
      // whether a counter update counts, the wide one which context an error
      // falls in.
      int32_t band_lo;   // +278720
      int32_t band_hi;   // +278724
      // The plane this block is coding: every reader uses it as
      // `plane_desc[plane_idx + 1]`'s subscript, and `alt_p2_alloc` folds it
      // into the high bits of every `ctx_delta` entry.
      uint32_t plane_idx;
      // Whether this plane has a reference to predict from, out of
      // `plane_desc`.  Zero skips the bias `alt_p2_context` adds to its first
      // four rows and skips the sub-model rows that read the other two
      // blocks, so a plane with no reference predicts from itself alone.
      int32_t has_ref;
      // Five p2 planes and a cursor into each.  `alt_p2_alloc` takes five
      // buffers of `18 * width + 234` bytes -- `sizeof(P2Ctx)` times
      // `width + 13` -- and sets each cursor 144 bytes in, which is one row of
      // eight records.  A row is finished by rotating the five buffers by one
      // and re-deriving the five cursors.
      //
      // Hex-Rays had three readings of these twenty bytes: `f278736[6 .. 9]`
      // as pointers, `f278760[0 .. 3]` as `uint32_t`, and four named pointers
      // `buf[1]`, `buf[2]`, `buf[3]`, `buf[4]`.  The same three
      // `memcpy`s from `buf[0]` and the same five-way rotation appear in each,
      // which is what says the three are one array and not three.
      P2Ctx *cursor[5];   // +278736 .. +278755
      P2Ctx *buf[5];      // +278756 .. +278775
      CtxWeight ctx_w[5];    // +278776 .. +278855
      uint8_t _u0_0_tail[8];   // +278856 .. +278863, to the end of the 336
    };
  };
  uint8_t _pad1[40];
  // Ten scalars `alt_p2_context` writes as it folds its neighbourhood sums.
  int32_t nb_sum[10];   // +278904 .. +278943
  // One table, reached from two bases four elements apart -- Hex-Rays' 16-byte
  // view called them element 3 and element 4 -- which is what a `t[n]`/
  // `t[n + 4]` pair looks like when the compiler keeps both addresses in
  // registers.  `n` is capped at 255 one line above each read, and 255 + 4 is
  // the last element.
  int32_t ctx_delta[260];   // +278944 .. +279983
  // Three 256-entry maps over a byte value.  `fold` and `unfold` are the pair
  // `AltP1Block` has, built by the same `alt_init_tables` call; what
  // `fold_hi` between them holds has no writer in this binary.
  uint8_t fold[256];   // +279984 .. +280239
  uint8_t fold_hi[256];   // +280240 .. +280495
  int8_t unfold[256];   // +280496 .. +280751
  // 120 = 960 / 8, and the reader caps at 960 before shifting: the guard is
  // the extent.  Hex-Rays' 16-byte view of this ran into the next table, whose
  // real base is +280872 -- which is why every read of that one said `+ 4`.
  uint8_t nb_ctx[120];   // +280752 .. +280871
  // The same interning as `ModelBlock::ctx_id`, one level deep: a
  // neighbourhood signature indexes `nb_id`, and a slot still zero takes the
  // next id from `nb_id_used`.  `nb_ctx` is the other half of the pairing --
  // 120 entries for the 960 the reader caps at, indexed `sum >> 3`.
  int16_t nb_id[1916];   // +280872 .. +284703
  uint8_t _pad2[8];   // +284704 .. +284711
  // The p2 counter table: five banks of 32768 records.  Hex-Rays named the
  // first sixteen bytes of each bank -- f284704, f415776, f546848, f677920,
  // f808992, 131072 bytes apart -- and every read of one said `+ 8`, because
  // the records start eight bytes past the name.  Four things agree on the
  // shape:
  //
  //   * `alt_p2_alloc` resets all of it in one loop of 0x14000 iterations
  //     eight bytes wide, two records at a time, and 284712 + 0x14000 * 8 is
  //     940072 -- exactly where the next table starts;
  //   * 131072 bytes is 32768 records, and every index is a context word
  //     shifted right by 11, which is 0..32767;
  //   * all five read sites are the same expression, and `alt_p2_model`
  //     updates the same three fields through raw offsets from +284712;
  //   * five banks of 32768 is 163840, and the last record ends at 940071.
  P2Count p2_ctr[163840];   // +284712 .. +940071
  // The frequency records the composed context index selects, one per context.
  // `alt_p2_alloc` seeds 0x1E60 = 7776 pairs of them, eight `uint16_t` a pass,
  // which is 15552 records -- and 15552 is 3^5 * 64, the p2 context space
  // `algorithm_v2.md` §9.1 derives from the five weight groups.  The size is
  // not an independent fact about the table; it is what `ctx_w` can index.
  //
  // `alt_p2_encode_symbol` is handed `&freq[ctx]`, `ctx` being that
  // index, and `alt_p2_model` updates records `k - 1`, `k` and `k + 1` --
  // three adjacent, the way the p1 model does.
  //
  // `alt_p2_model` reached all of that through `uint16_t *` cursors at
  // `block + 8 * k`, so the table's offset arrived folded into the index in
  // whatever unit was to hand: 470036 as a `uint16_t` index, 235018 as a
  // `uint32_t` one, 940072 written out.  The three cursor constants 470032,
  // 470036 and 470040 are the three adjacent records.  Nothing spells it that
  // way now -- the cursors are `P2Freq *` and the neighbours are `[-1]` and
  // `[1]` -- which is why none of those numbers is left in the file.
  P2Freq freq[15552];   // +940072 .. +1064487
  uint8_t _pad3[8];   // +1064488 .. +1064495
};
// The three readings this replaced disagreed about where the pointers stop and
// the weights start; these say where, so a fourth reading cannot creep back in.
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP2Block, cursor) == 278736,
              "AltP2Block::cursor: five plane cursors at +278736");
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP2Block, buf) == 278756,
              "AltP2Block::buf: five planes at +278756");
static_assert(sizeof(void *) != 4 || __builtin_offsetof(AltP2Block, ctx_w) == 278776,
              "AltP2Block::ctx_w: five weight groups at +278776");
static_assert(sizeof(void *) != 4 || sizeof(AltP2Block) == 0x103E30,
              "AltP2Block: bmf_page_alloc asks for 0x103E30 and this is it");
static_assert(sizeof(void *) != 4
              || (__builtin_offsetof(AltP2Block, nb_sum) == 278904
                  && __builtin_offsetof(AltP2Block, ctx_delta) == 278944
                  && __builtin_offsetof(AltP2Block, fold) == 279984
                  && __builtin_offsetof(AltP2Block, fold_hi) == 280240
                  && __builtin_offsetof(AltP2Block, unfold) == 280496
                  && __builtin_offsetof(AltP2Block, nb_ctx) == 280752
                  && __builtin_offsetof(AltP2Block, nb_id) == 280872
                  && __builtin_offsetof(AltP2Block, p2_ctr) == 284712
                  && __builtin_offsetof(AltP2Block, freq) == 940072),
              "AltP2Block: the layout moved");


// CtxWeights -- recovered from 49 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct CtxWeights {
  // Six pointers to the same shape, one per sub-model, which is what
  // `alt_p2_filter` treats them as: it walks all six with one weight each.
  // Each points at rows of four floats -- the function reads nothing else
  // through them -- so that is what they are.
  float (*f0[6])[4];   // +0 .. +20
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(CtxWeights, f0[5]) == 20,
              "CtxWeights: the layout moved");



struct RangeCoder {
  static const uint32_t kTop    = 0x00800000;   // renormalise at or below this
  static const uint32_t kPend   = 0x7F800000;   // low here still has a live carry
  static const uint32_t kCarry  = 0x80000000;   // the carry bit itself
  static const uint32_t kMask   = 0x7FFFFFFF;   // low is 31 bits
  static const uint8_t  kMarker = 0x97;         // section marker == the initial cache

  // Private, so that "does anything outside still touch the coder's state?"
  // is a question the compiler answers rather than a grep.
 private:
  uint32_t range;      // (0x00800000, 0x80000000]
  uint32_t low;        // encoding: low.  decoding: code.  One word, two jobs.
  uint8_t  cache;      // encoding: the byte a carry could still reach
                       // decoding: the previous input byte, for its dropped bit
  uint32_t pending;    // encoding: deferred 0xFF bytes behind the cache
  uint32_t rdiv;       // decoding: the range/tot from the last get_freq
  uint32_t bytes;      // encoding: emitted so far; the flush writes it out

  uint8_t *p()               { return out_cursor; }
  void     set_p(uint8_t *q) { out_cursor = q; }

 public:

  // ---- encoder ---------------------------------------------------------

  void enc_init() {
    range = 0x80000000; low = 0; pending = 0; bytes = 0; cache = kMarker;
  }

  // Emit the held cache byte plus `carry`, then the run of bytes the carry
  // rippled through: 0xFF each if it did not carry, 0x00 each if it did.
  void emit(uint32_t carry) {
    uint8_t *q = p();
    *q++ = (uint8_t)(cache + carry);
    for (; pending; --pending)
      *q++ = (uint8_t)(carry - 1);
    set_p(q);
    cache = (uint8_t)(low >> 23);
  }

  void enc_normalise() {
    while (range <= kTop) {
      ++bytes;
      if      (low < kPend)    emit(0);      // the top byte is settled
      else if (low & kCarry)   emit(1);      // it carried
      else                     ++pending;    // it is 0xFF and might still carry
      range <<= 8;
      low = (low << 8) & kMask;
    }
  }

  // Encode(cumFreq, cumFreq + freq, totFreq).
  uint32_t encode(uint32_t cum, uint32_t high, uint32_t tot) {
    enc_normalise();
    uint32_t r = range / tot;
    uint32_t below = r * cum;
    uint32_t width = r * (high - cum);
    range = (high < tot) ? width : range - below;
    low  += below;
    return width;                        // freq * r; one caller reads it
  }

  // One bit against two frequencies.  Returns the width of the interval it
  // chose -- which is the new `range`, and is what one caller passes on as its
  // own result.
  uint32_t encode_bit(uint32_t f0, uint32_t f1, int32_t bit) {
    enc_normalise();
    uint32_t rt = f0 * (range / (f0 + f1));
    if (bit) { low += rt; range -= rt; }
    else     { range = rt; }
    return range;
  }

  // Close the section: the last emit, a rounded final byte, the section
  // length, padding, and the marker.  Leaves the bit packer word-aligned.
  void flush() {
    enc_normalise();
    bytes += 5;
    uint32_t len     = bytes;
    uint32_t rounded = ((low & 0x7FFFFF) >= ((len & 0xFFFFFF) >> 1)) + (low >> 23);
    uint32_t carry   = (rounded > 0xFF);
    uint8_t *q = p();
    *q++ = (uint8_t)(cache + carry);
    for (; pending; --pending)
      *q++ = (uint8_t)(carry - 1);
    *q++ = (uint8_t)rounded;
    *q++ = (uint8_t)(len >> 16);
    *q++ = (uint8_t)(len >> 8);
    *q++ = (uint8_t)len;
    while ((uint32_t)(q - (uint8_t *)coded_buf) % 4 != 3)
      *q++ = 0;
    *q++ = kMarker;
    set_p(q);
    packer_free_bits = 0; packer_acc = 0; packer_word = (uint32_t *)out_cursor;
  }

  // ---- decoder ---------------------------------------------------------

  // Consume the marker and prime `code` with 31 bits.  range = 2^7 makes the
  // first normalise run three times, which is what fills it.
  void dec_init() {
    uint8_t *q = p();
    if (*q++ != kMarker)
      __exit_402E40(4);
    cache = *q++;
    set_p(q);
    range = 128;
    low   = cache >> 1;
  }

  // Eight more bits of a byte-aligned stream, through a window that sits one
  // bit short of a byte boundary: put back the bit `dec_init`'s >> 1 dropped
  // from the previous byte, then take seven from the next.
  void dec_normalise() {
    uint8_t *q = p();
    while (range <= kTop) {
      uint32_t head = (uint32_t)(uint8_t)(cache << 7) | (low << 8);
      cache = *q++;
      low   = (cache >> 1) | head;
      range <<= 8;
    }
    set_p(q);
  }

  // Which slot of `tot` the code falls in.  decode() finishes the step once
  // the caller has turned that into a (cumFreq, cumFreq + freq) pair.
  uint32_t get_freq(uint32_t tot) {
    dec_normalise();
    rdiv = range / tot;
    uint32_t count = low / rdiv;
    return (count >= tot) ? tot - 1 : count;
  }

  void decode(uint32_t cum, uint32_t high, uint32_t tot) {
    uint32_t below = cum * rdiv;
    low  -= below;
    range = (high < tot) ? (high - cum) * rdiv : range - below;
  }

  int32_t decode_bit(uint32_t f0, uint32_t f1) {
    dec_normalise();
    uint32_t rt = f0 * (range / (f0 + f1));
    if (low >= rt) { range -= rt; low -= rt; return 1; }
    range = rt;
    return 0;
  }

  // Skip to the marker that closed the section, and hand the position back to
  // the bit packer.
  void finish() {
    dec_normalise();
    uint8_t *q = p();
    while (*q++ != kMarker) { }
    set_p(q);
    packer_word = (uint32_t *)out_cursor;
    packer_free_bits = 0; packer_acc = 0;
  }
};

static RangeCoder rc;

// The archive handle: the eight bytes `bmf c` and `bmf d` allocate to hold the
// stream they are working on and a count of the images seen in it.  Hex-Rays
// had it as a bare `malloc(8)` addressed by offset -- `*(FILE **)(v5 + 4)` --
// which stores a FILE * in four bytes.  Named, it is the same eight bytes at 32
// bits and the right sixteen at 64.
struct BmfArc {
  uint32_t images;   // +0
  FILE    *fp;       // +4
};
static_assert(sizeof(void *) != 4 || sizeof(BmfArc) == 8,
              "BmfArc must still be the eight bytes the original allocated");

FILE *__bmf_close_archive(BmfArc *_this)
{
  ;
  FILE *Stream_v;
  Stream_v = _this->fp;
  if ( Stream_v )
  {
    fseek(Stream_v, 0, 2);
    return (FILE *)fclose(_this->fp);
  }
  return Stream_v;
}

BmfArc *__bmf_destroy_archive(BmfArc *Block, int8_t a2)
{
  ;
  __bmf_close_archive((BmfArc *)Block);
  if ( (a2 & 1) != 0 )
    free(Block);
  return Block;
}

void __expand_predictor_mode0(uint32_t Src, int32_t i, int32_t a3)
{
  ;
  // never taken: -E is 0
}

// MED, the LOCO-I / JPEG-LS median edge predictor, applied to a whole plane in
// place: every byte is replaced by its residual, folded to an unsigned code.
//
// It runs *backwards*, from the last pixel to the first, which is what lets it
// work in place -- each pixel's predictors are up and to the left, so they are
// still the originals when it reaches them.  That also fixes the shape of the
// loops: the last row is the general case, the first column of each row has no
// west neighbour, and the first row has no north one, so those two are peeled
// out rather than guarded.
//
// It also counts every code it writes into `hist_scratch`, and nothing in this
// build reads that count back: `hist_scratch` is a scratch region that
// `model_planes` immediately reuses as `__model_planes_buf`.  The histogram was
// for a cost estimate that this build's constant compression mode does not
// reach -- as is `predict_med` itself, whose two call sites are both under
// `plane_predictor == 1`, which `-E` being 0 makes unreachable.  `unpredict_med`
// is not: a decoder still has to read streams an encoder with `-E` produced,
// which is what `testfiles/med32.bmp` exists to exercise.
//
// So this body is here to be *read*, as the definition of what the inverse
// undoes, and the naming below is worth more than the code is.
uint32_t __predict_med(uint8_t *pixels, int32_t width, int32_t height)
{
  ;
  uint8_t left;
  int32_t rows_left, x_left, north, pred, northwest, code, pairs, ofs, done;
  uint32_t j, last, k;
  uint8_t *q;
  uint8_t *p, *up;
  alignas(16) uint8_t fold[272];
  p = (pixels + height * width);
  up = &p[-width];
  // The folding table, value -> code: a non-negative residual takes the even
  // codes counting up from 0, a negative one the odd codes counting down from
  // -1.  `unpredict_med` builds the inverse of this.  Wrapping to eight bits
  // is what the byte stores did.
  for ( j = 0; j < 128; ++j )
  {
    fold[j]       = (uint8_t)(2 * j);
    fold[128 + j] = (uint8_t)(-1 - 2 * (int32_t)j);
  }
  rows_left = height - 1;
  if ( height == 1 )
    goto LABEL_24;
  do
  {
    x_left = width - 1;
    if ( width == 1 )
      goto LABEL_23;
    do
    {
      // `up` trails `p` by exactly one row and the two step together, so
      // these three are the neighbourhood of the pixel at `*p`:
      //
      //     northwest  north
      //     pred(west) *p
      //
      // `pred` starts as the west neighbour because two of MED's three
      // outcomes are `west` or `north` unchanged, and west is the more common.
      north = (uint8_t)*--up;
      pred = (uint8_t)*(--p - 1);
      northwest = (uint8_t)p[-width - 1];
      // MED: northwest above both neighbours predicts the smaller, below both
      // predicts the larger, between them predicts the plane through all three.
      // An edge through the corner therefore predicts across it, not along it.
      if ( pred < north )
      {
        if ( northwest < pred )
        {
          LOBYTE(pred) = *up;
          goto LABEL_21;
        }
        if ( northwest <= north )
LABEL_20:
          LOBYTE(pred) = north + pred - northwest;
      }
      else
      {
        if ( northwest > pred )
        {
          LOBYTE(pred) = *up;
          goto LABEL_21;
        }
        if ( northwest >= north )
          goto LABEL_20;
      }
LABEL_21:
      code = (uint8_t)fold[(uint8_t)(*p - pred)];
      *p = code;
      ++*(uint32_t *)&hist_scratch[4 * code];
      --x_left;
    }
    while ( x_left );
LABEL_23:
    // The first column of the row: no west neighbour, so predict from north
    // alone.  This is also where a width of 1 lands, which is why the inner
    // loop is skipped rather than entered zero times.
    --up;
    --p;
    last = (uint8_t)fold[(uint8_t)(*p - p[-width])];
    *p = last;
    ++*(uint32_t *)&hist_scratch[4 * last];
    --rows_left;
  }
  while ( rows_left );
LABEL_24:
  // The first row, which has no north neighbour: predict each pixel from the
  // one to its left.  MSVC unrolled it two pixels at a time, so `left` is the
  // one value both halves of a pair need and `ofs` walks back in twos; `done`
  // is how far the unrolled part got, and the `if` below is the odd pixel it
  // leaves when the row is an even number wide.
  //
  // Nothing here touches `hist_scratch`, unlike the two cases above -- the
  // first row is one row out of `height`, and leaving it out saves the
  // unrolled loop a dependent store.
  if ( width != 1 )
  {
    last = width - 1;
    pairs = (width - 1) / 2;
    if ( pairs )
    {
      k = 0;
      ofs = 0;
      do
      {
        left = p[ofs - 2];
        ++k;
        p[ofs - 1] = fold[(uint8_t)(p[ofs - 1] - left)];
        p[ofs - 2] = fold[(uint8_t)(left - p[ofs - 3])];
        ofs -= 2;
      }
      while ( k < pairs );
      done = 2 * k + 1;
    }
    else
    {
      done = 1;
    }
    if ( last > done - 1 )
    {
      q = (&p[-done]);
      last = (uint8_t)(*q - *((int8_t *)q - 1));
      *q = fold[(uint8_t)last];
    }
  }
  // Whatever code was written last, which is not a result: both call sites
  // discard it.
  return last;
}

// Build the residual folding pair the alternate models use, which is the same
// pair `predict_med` and `unpredict_med` build inline for themselves:
//
//   unfold[code]        the signed residual a code stands for: 0, -1, +1, -2,
//                       +2 ... out to -127, +127, -128.
//   fold[residual+256]  the inverse, indexed by the residual as a byte.  The
//                       +256 is the zero point, so `fold` is addressed from
//                       -128 to +127 either side of it.
//
// The middle block is the near-lossless quantiser: with a maximum error of `w`
// it maps a run of `2w+1` residuals onto one code, and `bucket_size` is that
// run.  This build is lossless -- `-E` is 0, so `w` is 0, `bucket_size` is 1
// and every bucket holds one residual -- which is why `lo` is 1 and the walk
// degenerates to writing consecutive codes.  It is kept whole rather than
// folded to that special case, because the special case is a build option and
// not a property of the format.
uint32_t __alt_init_tables(uint8_t *fold, int8_t *unfold)
{
  ;
  uint8_t even, odd;
  int32_t bucket_size, lo, half, in_bucket, ofs, bucket, bucket_1, done;
  uint32_t i, k, n0x80, span, pairs;
  uint8_t *pos, *neg;
  bucket_size = 2 * plane_desc[0].w12 + 1;
  // The predictor-mode-0 branch was here: 111 lines building a 256-entry
  // identity table with SSE.  Nothing can reach it.  This function is called
  // only by alt_p2_alloc and alt_p1_alloc; those are called only by the eight
  // alt_model_p{1,2}[_d8]_{encode,decode} bodies; and model_plane and
  // unmodel_plane dispatch to those bodies only under plane_predictor == 1
  // or == 2.  The call graph is closed and has no path with the predictor at
  // 0, which a run over the corpus agrees with: 164 entries, 83 at 1 and 81
  // at 2.  Deleted on the same grounds as the fast path (REFACTORING.md
  // section 2.1) -- code no dispatch reaches is not a feature to keep.
  *unfold = 0;
  unfold[255] = 0x80;
  // MSVC's overlap check for the loop below, and it is always true:
  // `unfold + 1 <= unfold + 2` settles the first half, and the second falls to
  // `(uint32_t)(v18 - v19) < 0xFE`, which is `1 < 254`.  Both `v18` and
  // `v19` are constants of `unfold`, so nothing at run time can change it.
  // Kept as a comment for the same reason the other always-taken tests in
  // this function are: the branch is the artefact, the body is the code.
  for ( i = 0; i < 0x3F; ++i )
  {
    ((uint8_t *)unfold)[4 * i + 2] = 2 * i + 1;
    ((uint8_t *)unfold)[4 * i + 1] = -2 * i - 1;
    ((uint8_t *)unfold)[4 * i + 4] = 2 * i + 2;
    ((uint8_t *)unfold)[4 * i + 3] = -2 * i - 2;
  }
  unfold[254] = 127;
  unfold[253] = -127;
  // The `else` that stood here is gone with the test: it wrote the same
  // 256 values in two passes instead of one, which is what an overlap
  // fallback does, and it is what says the branch was MSVC's and not the
  // program's.
  // never taken: -E is 0
  // The test here was `if ( plane_predictor )`, with an else for predictor
  // mode 0.  This function is called only by alt_p1_alloc and alt_p2_alloc,
  // and those only by the eight alt_model bodies, which the dispatch reaches
  // only under the predictor being 1 or 2.  Always true, so it and its
  // 22-line else are gone.  Same argument as the block above it.
  *fold = 0;
  lo = 1;
  fold[128] = -1;
  // -E is 0, so the near-lossless fill that stood here never ran: the jump
  // over it was `if ( 1 ) goto LABEL_52`, and LABEL_52 had no other source.
  // The `lo < 128` test it jumped past went with it -- the only live
  // path entered the block without evaluating it.
  {
    span = 128 - lo;
    pairs = (128 - lo) / 2;
    half = 0;
    in_bucket = 1;
    if ( pairs )
    {
      k = 0;
      ofs = 0;
      pos = &(fold)[lo];
      neg = &(fold)[-lo];
      do
      {
        bucket = bucket_size;
        bucket_1 = in_bucket - 1;
        if ( bucket_1 )
          bucket = bucket_1;
        else
          ++half;
        even = 2 * half;
        pos[2 * k] = 2 * half;
        odd = 2 * half - 1;
        in_bucket = bucket - 1;
        neg[ofs + 256] = odd;
        if ( bucket == 1 )
        {
          in_bucket = bucket_size;
          even = 2 * ++half;
          odd = 2 * half - 1;
        }
        pos[2 * k++ + 1] = even;
        neg[ofs + 255] = odd;
        ofs -= 2;
      }
      while ( k < pairs );
      done = 2 * k + 1;
    }
    else
    {
      done = 1;
    }
    if ( done - 1 < span )
    {
      if ( in_bucket == 1 )
        LOBYTE(half) = half + 1;
      (fold)[lo - 1 + done] = 2 * half;
      (fold)[-lo - done + 257] = 2 * half - 1;
    }
  }
  // `fold` is finished by inverting `unfold` outright: whatever residual a code
  // stands for, that residual folds back to that code.  Two codes per turn
  // because the residuals come in signed pairs.
  for ( n0x80 = 0; n0x80 < 0x80; ++n0x80 )
  {
    (fold)[(uint8_t)((uint8_t *)unfold)[2 * n0x80] + 256] = 2 * n0x80;
    (fold)[(uint8_t)((uint8_t *)unfold)[2 * n0x80 + 1] + 256] = 2 * n0x80 + 1;
  }
  // 0x80, and no caller reads it.
  return n0x80;
}

uint32_t __rc_decode_flat(uint32_t tot)
{
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym + 1, tot);
  return sym;
}

int32_t __encode_context_bit(BitCtr *_this, BitCtr *a2, int32_t n15)
{
  ;
  int32_t v3, v4, n0x4000, result, v18, v19, n0x4000_1, v31, v33;
  uint32_t tot, n0x88_1, v30, v32, v34;
  v3 = _this->n[0];
  if ( _this->n[0] )
  {
    v4 = _this->n[1];
    if ( !_this->n[1] )
    {
      v31 = a2->n[0];
      v32 = v31 + a2->n[1];
      _this->n[0] = (v32 + (v31 << 6) - 64) / v32;
      _this->n[1] = ((a2->n[1] << 6) + v32 - 64) / v32;
      _this->n[v3 - 1] += 4;
      _this->limit = 512;
      v33 = a2->n[v3 - 1];
      a2->n[v3 - 1] = -3 * ((uint32_t)(3 - v33) >> 31) + v33;
      v3 = _this->n[0];
      v4 = _this->n[1];
    }
    tot = v3 + v4;
    rc.encode_bit(v3, v4, n15);
    n0x4000 = _this->limit;
    if ( tot > n0x4000 )
    {
      v30 = _this->n[1];
      _this->n[0] -= _this->n[0] >> 1;
      _this->n[1] = v30 - (v30 >> 1);
      if ( n0x4000 < 0x4000 )
        _this->limit = n0x4000 + 64;
    }
    result = _this->n[n15] + 8;
    _this->n[n15] = result;
    a2->n[n15] += (uint32_t)tot < 0x88;
    return result;
  }
  v18 = a2->n[0];
  v19 = a2->n[1];
  n0x88_1 = v18 + v19;
  rc.encode_bit(v18, v19, n15);
  n0x4000_1 = a2->limit;
  if ( n0x88_1 > n0x4000_1 )
  {
    v34 = a2->n[1];
    a2->n[0] -= a2->n[0] >> 1;
    a2->n[1] = v34 - (v34 >> 1);
    if ( n0x4000_1 < 0x4000 )
      a2->limit = n0x4000_1 + 64;
  }
  result = a2->n[n15] + 8;
  a2->n[n15] = result;
  _this->n[0] = n15 + 1;
  return result;
}

int32_t __decode_context_bit(BitCtr *_this, BitCtr *a2)
{
  ;
  int32_t v2, v3, result, n0x4000, v13, v14, n0x4000_1, v24, v26;
  uint32_t tot, n0x88_1, v23, v25, v27;
  v2 = _this->n[0];
  if ( _this->n[0] )
  {
    v3 = _this->n[1];
    if ( !_this->n[1] )
    {
      v24 = a2->n[0];
      v25 = v24 + a2->n[1];
      _this->n[0] = (v25 + (v24 << 6) - 64) / v25;
      _this->n[1] = ((a2->n[1] << 6) + v25 - 64) / v25;
      _this->n[v2 - 1] += 4;
      _this->limit = 512;
      v26 = a2->n[v2 - 1];
      a2->n[v2 - 1] = -3 * ((uint32_t)(3 - v26) >> 31) + v26;
      v2 = _this->n[0];
      v3 = _this->n[1];
    }
    tot = v2 + v3;
    result = rc.decode_bit(v2, v3);
    n0x4000 = _this->limit;
    if ( tot > n0x4000 )
    {
      v23 = _this->n[1];
      _this->n[0] -= _this->n[0] >> 1;
      _this->n[1] = v23 - (v23 >> 1);
      if ( n0x4000 < 0x4000 )
        _this->limit = n0x4000 + 64;
    }
    _this->n[result] += 8;
    a2->n[result] += (uint32_t)tot < 0x88;
  }
  else
  {
    v13 = a2->n[0];
    v14 = a2->n[1];
    n0x88_1 = v13 + v14;
    result = rc.decode_bit(v13, v14);
    n0x4000_1 = a2->limit;
    if ( n0x88_1 > n0x4000_1 )
    {
      v27 = a2->n[1];
      a2->n[0] -= a2->n[0] >> 1;
      a2->n[1] = v27 - (v27 >> 1);
      if ( n0x4000_1 < 0x4000 )
        a2->limit = n0x4000_1 + 64;
    }
    a2->n[result] += 8;
    _this->n[0] = result + 1;
  }
  return result;
}

int32_t __encode_symbol_list(SymList *_this, int32_t a2)
{
  ;
  int8_t v2;
  uint8_t v27, v32;
  // Every one of these walked `_this[5]`'s entries three bytes at a time,
  // reading the symbol as `*(uint16_t *)p` and the count as `p[2]`.
  SymEntry *v4, *v25, *v28, *v36, *v37, *v39, *v40, *v49;
  uint16_t v51;
  // A cumulative count, a high count and a total: the three arguments
  // `RangeCoder::encode` takes, and it takes them unsigned.
  uint32_t enc_cum, enc_high, enc_tot;
  int32_t v3, v5, v6, v7, v8, v24, n251, v35, v38,
          v41, v43, v47, v53, v54;
  uint16_t v26, v31;
  uint32_t i_1, i, rescale_at, v42, since_rescale;
  SymList *this_1;
  v2 = exclusion_gen;
  v3 = _this->live;
  v4 = _this->ent - 1;
  v5 = 0;
  while ( 1 )
  {
    ++v4;
    v6 = v4->sym;
    if ( exclusion_mask[v6] != exclusion_gen )
    {
      v7 = v4->cnt;
      v5 += v7;
      if ( v6 == a2 )
        break;
    }
    if ( !--v3 )
    {
      if ( !v5 )
        return 0;
      enc_cum = v5;
      enc_tot = _this->tot + v5;
      enc_high = enc_tot;
      do
      {
        exclusion_mask[v4->sym] = v2;
        --v4;
      }
      while ( v4 >= _this->ent );
      v8 = 0;
      {
        rc.encode(enc_cum, enc_high, enc_tot);
        return v8;
      }
    }
  }
  enc_high = v5;
  i_1 = v3 - 1;
  enc_cum = v5 - v7;
  if ( i_1 )
  {
    this_1 = _this;
    for ( i = 0; i < i_1; ++i )
    {
      if ( exclusion_mask[v4[i + 1].sym] == exclusion_gen )
        v24 = 0;
      else
        v24 = v4[i + 1].cnt;
      v5 += v24;
    }
    _this = this_1;
  }
  enc_tot = _this->tot + v5;
  v4->cnt += 4;
  v25 = _this->ent;
  _this->since_rescale += 4;
  if ( v4 == v25 )
  {
LABEL_37:
    n251 = v4->cnt;
  }
  else
  {
    v26 = v4->sym;
    v27 = v4->cnt;
    v28 = v4 - 1;
    v4->sym = v28->sym;
    v4->cnt = v28->cnt;
    v28->sym = v26;
    v28->cnt = v27;
    v25 = _this->ent;
    if ( v28 == v25 )
    {
      n251 = v28->cnt;
    }
    else
    {
      while ( 1 )
      {
        n251 = v28->cnt;
        v4 = v28 - 1;
        if ( n251 <= v4->cnt )
          break;
        v31 = v28->sym;
        v32 = v28->cnt;
        v28->sym = v4->sym;
        v28->cnt = v4->cnt;
        v4->sym = v31;
        v4->cnt = v32;
        v25 = _this->ent;
        --v28;
        if ( v4 == v25 )
          goto LABEL_37;
      }
    }
  }
  rescale_at = _this->rescale_at;
  if ( n251 > 251 || rescale_at < _this->since_rescale )
  {
    v35 = _this->live;
    v54 = rescale_at < 20 * _this->n;
    v36 = v25 - 1;
    do
    {
      v37 = v36;
      ++v36;
      v38 = (v54 + (uint32_t)v36->cnt) >> 1;
      v36->cnt = v38;
      if ( v36 != _this->ent )
      {
        v39 = v36 - 1;
        v53 = v39->cnt;
        if ( v38 > v53 )
        {
          v51 = v36->sym;
          v36->sym = v39->sym;
          v36->cnt = v53;
          if ( v39 != _this->ent )
          {
            v49 = v36;
            v47 = v35;
            do
            {
              v40 = v39 - 1;
              v41 = v40->cnt;
              if ( v38 <= v41 )
                break;
              v39->sym = v40->sym;
              v39->cnt = v41;
              --v39;
            }
            while ( v40 != _this->ent );
            v36 = v49;
            v35 = v47;
          }
          v39->sym = v51;
          v39->cnt = v38;
        }
      }
      --v35;
    }
    while ( v35 );
    v42 = _this->tot;
    if ( !v36->cnt )
    {
      do
      {
        ++v35;
        _this->tot = ++v42;
        v43 = v37->cnt;
        --v37;
      }
      while ( !v43 );
      _this->live -= v35;
    }
    since_rescale = _this->since_rescale;
    _this->tot = v42 - (v42 >> 1);
    _this->since_rescale = since_rescale - (since_rescale >> 1);
    v8 = 1;
  }
  else
  {
    v8 = 1;
  }
  rc.encode(enc_cum, enc_high, enc_tot);
  return v8;
}

// Add `a3` to symbol `a2`'s count in the list `encode_symbol_list` codes from
// -- the same 3-byte entries, `_this[5]` base, `_this[1]` length -- then bubble
// the entry forward while it outweighs its predecessor, and halve every count
// when one passes 251 or the total passes `_this[4]`.  Sort, then rescale: this
// is the model update, and the only caller is `init_model_tables`.
// The return value is never read: `init_model_tables` calls this twelve times
// and discards every one.  That matters, because the slot Hex-Rays called
// `n251` carried two things -- the list base and a symbol's count byte -- and
// which of them reached the final `return` depended on the branch.  Split into
// `list` and `count`, there is nothing sensible to return, so it returns
// nothing.
void __symbol_list_update(SymList *_this, int32_t a2, uint32_t a3)
{
  ;
  SymEntry *list;     // the three-byte entries, `_this->ent`
  uint32_t count;     // a symbol's count byte, while it is being compared
  bool v7;
  uint8_t v11, v15, v19;
  // All of these walk the entries; the -3 and +2 they carried were the record
  // stride and the count field.
  SymEntry *v16, *v23, *v24, *v26, *v27, *v33, *n251_1, *n251_2;
  uint16_t v10, v14, v18, v34;
  int32_t v8, v25, v28, v30, v32, v35, v36;
  uint32_t v22;   // a count, like `live` which it is subtracted from
  uint32_t v4, v6, rescale_at, v29, since_rescale;
  list = _this->ent;
  v4 = _this->live;
  n251_1 = list;
  v6 = v4;
  if ( v4 )
  {
    while ( n251_1->sym != a2 )
    {
      ++n251_1;
      if ( !--v6 )
        goto LABEL_4;
    }
    n251_1->cnt += a3;
    _this->since_rescale += a3;
    n251_2 = _this->ent;
    if ( n251_1 == n251_2 )
    {
LABEL_16:
      count = n251_1->cnt;
    }
    else
    {
      // Swap this entry with the one before it.
      v14 = n251_1->sym;
      v15 = n251_1->cnt;
      v16 = n251_1 - 1;
      n251_1->sym = v16->sym;
      n251_1->cnt = v16->cnt;
      v16->sym = v14;
      v16->cnt = v15;
      n251_2 = _this->ent;
      if ( v16 == n251_2 )
      {
        count = v16->cnt;
      }
      else
      {
        while ( 1 )
        {
          count = v16->cnt;
          n251_1 = v16 - 1;
          if ( count <= n251_1->cnt )
            break;
          v18 = v16->sym;
          v19 = v16->cnt;
          v16->sym = n251_1->sym;
          v16->cnt = n251_1->cnt;
          n251_1->sym = v18;
          n251_1->cnt = v19;
          n251_2 = _this->ent;
          --v16;
          if ( n251_1 == n251_2 )
            goto LABEL_16;
        }
      }
    }
    rescale_at = _this->rescale_at;
    if ( count > 251 || rescale_at < _this->since_rescale )
    {
      v22 = _this->live;
      v36 = rescale_at < 20 * _this->n;
      v23 = n251_2 - 1;
      do
      {
        v24 = v23;
        ++v23;
        v25 = (v36 + (uint32_t)v23->cnt) >> 1;
        v23->cnt = v25;
        if ( v23 != _this->ent )
        {
          v26 = v23 - 1;
          v35 = v26->cnt;
          if ( v25 > v35 )
          {
            v34 = v23->sym;
            v23->sym = v26->sym;
            v23->cnt = v35;
            if ( v26 != _this->ent )
            {
              v33 = v23;
              v32 = v22;
              do
              {
                v27 = v26 - 1;
                v28 = v27->cnt;
                if ( v25 <= v28 )
                  break;
                v26->sym = v27->sym;
                v26->cnt = v28;
                --v26;
              }
              while ( v27 != _this->ent );
              v23 = v33;
              v22 = v32;
            }
            v26->sym = v34;
            v26->cnt = v25;
          }
        }
        --v22;
      }
      while ( v22 );
      v29 = _this->tot;
      if ( !v23->cnt )
      {
        do
        {
          ++v22;
          _this->tot = ++v29;
          v30 = v24->cnt;
          --v24;
        }
        while ( !v30 );
        _this->live -= v22;
      }
      since_rescale = _this->since_rescale;
      _this->tot = v29 - (v29 >> 1);
      _this->since_rescale = since_rescale - (since_rescale >> 1);
      return;
    }
  }
  else
  {
LABEL_4:
    v7 = v4 == _this->n;
    if ( v4 >= _this->n )
    {
      if ( a3 <= 1 )
        return;
      v7 = v4 == _this->n;
    }
    if ( v7 )
    {
      _this->live = --v4;
      v8 = list[v4].cnt;
    }
    else
    {
      v8 = 1;
    }
    list += v4;
    _this->live = v4 + 1;
    _this->tot = v8 + _this->tot + 1;
    list->cnt = 2;
    list->sym = a2;
    _this->since_rescale += 4;
    if ( list != _this->ent )
    {
      // The new entry starts one place forward, same swap as above.
      v10 = list->sym;
      v11 = list->cnt;
      list->sym = list[-1].sym;
      list->cnt = list[-1].cnt;
      list[-1].sym = v10;
      list[-1].cnt = v11;
    }
  }
}

uint16_t *__init_counter_node(uint16_t *_this)
{
  ;
  *(_this + 1) = 8;
  *(_this + 2) = 2;
  *(_this + 3) = 2;
  *(_this + 4) = 2;
  *(_this + 5) = 2;
  *(_this + 6) = 3;
  *(_this + 7) = 3;
  *_this = 22;
  return _this;
}

int32_t __encode_symbol_tree(uint16_t *_this, int32_t n2) {
  ;
  int16_t v24, v42;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t v8;
  int32_t n4, n0x7F800000_6, n0x800000_5, v27, v28, v46, v56, v59, i, v65,
          v66, n0x7F800000_7;
  uint16_t *v3, *v26, n0x4000, v39, v41, *v51, *this_2;
  uint32_t n4_2, v38, v40, v43, v44, v45, v48, v52;
  n4 = model_geometry[n2];
  v3 = _this + 2;
  v51 = _this + 2;
  // The counts below `n2`, which is where the range coder's interval starts.
  // Four at a time in SSE, then a scalar tail; integer addition does not care
  // about the order, so it is one loop.
  v8 = 0;
  for ( n4_2 = 0; n4_2 < n4; n4_2++ )
    v8 += v3[n4_2];
  v51 = &v3[n4];
  v52 = v8 + *v51;
  v48 = *_this;
  n0x800000_5 = rc.encode(v8, v52, v48);
  if ( *_this > 0x4000u )
  {
    n0x7F800000_7 = n0x7F800000_6;
    v39 = *(_this + 2) - (*(_this + 2) >> 1);
    v40 = *(_this + 3);
    *(_this + 2) = v39;
    LOWORD(v40) = v40 - (v40 >> 1);
    *(_this + 3) = v40;
    LOWORD(v40) = v39 + v40;
    v41 = *(_this + 4) - (*(_this + 4) >> 1);
    *(_this + 4) = v41;
    v42 = v40 + v41;
    LOWORD(v40) = *(_this + 5) - (*(_this + 5) >> 1);
    v43 = *(_this + 6);
    *(_this + 5) = v40;
    LOWORD(v43) = v43 - (v43 >> 1);
    *(_this + 6) = v43;
    LOWORD(v43) = v42 + v40 + v43;
    LOWORD(v40) = *(_this + 7) - (*(_this + 7) >> 1);
    v44 = *(_this + 8);
    *(_this + 7) = v40;
    LOWORD(v40) = v43 + v40;
    LOWORD(v44) = v44 - (v44 >> 1);
    v45 = *(_this + 9);
    *(_this + 8) = v44;
    LOWORD(v44) = v40 + v44;
    LOWORD(v45) = v45 - (v45 >> 1);
    v46 = *(_this + 1);
    *(_this + 9) = v45;
    *_this = v44 + v45;
    n0x7F800000_6 = n0x7F800000_7;
    if ( (v46 <= 4 * alt_freq_limit) )
    {
      n0x800000_5 = 4 * (v46 > alt_freq_limit);
      v24 = v46 - n0x800000_5;
      *(_this + 1) = v46 - n0x800000_5;
    }
    else
    {
      v24 = v46 - 16;
      *(_this + 1) = v46 - 16;
    }
  }
  else
  {
    v24 = *(_this + 1);
  }
  *v51 += v24;
  *_this += *(_this + 1);
  if ( n2 >= 2 )
  {
    v66 = level_geom[n4].half;
    v56 = n2 - level_geom[n4].first;
    v59 = 0;
    this_2 = _this;
    for ( i = 1; ; i *= 2 )
    {
      v26 = (uint16_t *)(((uint8_t *)&this_2[2 * level_geom[n4].tbl_base + 8]) + 4 * (i + v59));
      v65 = v26[1];
      v27 = (v66 & v56) != 0;
      v28 = *v26;
      n0x800000_5 = rc.encode_bit(v28, v65, v27);
      n0x4000 = v26[v27];
      if ( n0x4000 > 0x4000u )
      {
        v38 = v26[1];
        *v26 -= *v26 >> 1;
        v26[1] = v38 - (v38 >> 1);
        n0x4000 = v26[v27];
      }
      v26[v27] = alt_freq_init + n0x4000;
      v66 >>= 1;
      v59 = v27 + 2 * v59;
      if ( !v66 )
        return n0x800000_5;
    }
  }
  return n0x800000_5;
}

int32_t __alt_p1_encode_symbol(uint16_t *a1, int32_t n5, int32_t a3, int32_t n5a)
{
  ;
  bool v29;
  int16_t v4;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t n0x2000_2;
  int32_t n5a_1, result, n256, n5a_2;
  uint16_t *v27, *v30, v31, *v35, *v37;
  uint32_t tot, n5a_3, n0x2000_3;
  v4 = *a1;
  n5a_1 = 6 - (n5a & 1);
  if ( n5a < 5 )
    n5a_1 = n5a;
  n5a_2 = n5a_1;
  v37 = a1 + 1;
  v35 = a1 + 1;
  tot = v4 & 0x7FFF;
  // The counts below the symbol, same shape as `encode_symbol_tree`'s.
  n0x2000_2 = 0;
  for ( n5a_3 = 0; n5a_3 < n5a_2; n5a_3++ )
    n0x2000_2 += v37[n5a_3];
  v35 = &v37[n5a_2];
  n0x2000_3 = n0x2000_2 + *v35;
  rc.encode(n0x2000_2, n0x2000_3, tot);
  if ( tot > 0x2000 )
  {
    *a1 = 0x8000;
    v27 = a1 + 7;
    n256 = 256;
    v29 = a1 + 7 < v37;
    v30 = a1 + 7;
    if ( a1 + 7 >= v37 )
    {
      do
      {
        if ( *v30 < n256 )
          n256 = *v30;
        --v30;
      }
      while ( v30 >= v37 );
      v29 = v27 < v37;
    }
    if ( !v29 )
    {
      do
      {
        if ( (uint16_t)n256 <= 1u )
          v31 = *v27 - (*v27 >> 1);
        else
          v31 = (*v27 + 2) / 3;
        *v27 = v31;
        *a1 += v31;
        --v27;
      }
      while ( v27 >= v37 );
    }
  }
  *v35 += 32;
  result = *a1 + 32;
  *a1 = result;
  if ( n5a_2 >= 5 )
    return __encode_symbol_tree(model_strip(
             128 * (uint32_t)(n5a_2 & 1)
             + ((((a1[1] + (result & 0x7FFF) + 96 - 2 * (uint32_t)a1[n5a_2 + 1]) >> 25) & 0xFFFFFFC0)
             + (uint32_t)a3)), (n5a - 5) >> 1);
  return result;
}

// The decoder half of `encode_symbol_tree`.  Same object -- `model_tables +
// 254 * index`, counts from `_this + 2`, total in `*_this` -- same halving when
// the total passes 0x4000, and the two are called from the matching sides:
// `encode_symbol_tree` from alt_p1/alt_p2_encode_symbol, this from
// alt_p1_decode_symbol and alt_p2_decode_symbol.
int32_t __decode_symbol_tree(uint16_t *_this)
{
  ;
  int16_t v31, v33;
  // The cumulative count the range coder takes, which it takes unsigned.
  int32_t n2_1, v18, v23, v25, v36, v38, v40, n2, v43, v44;
  uint16_t *v8, v16, *v17, n0x4000, v28, v30, v32, v34, *v37;
  uint32_t v7, v9, v29, v35, v39;
  v39 = *_this;
  n2 = 0;
  v7 = rc.get_freq(v39);
  v8 = _this + 2;
  v9 = *(_this + 2);
  if ( v9 <= v7 )
  {
    n2_1 = 0;
    do
    {
      ++v8;
      ++n2_1;
      v9 += *v8;
    }
    while ( v9 <= v7 );
    n2 = n2_1;
  }
  v37 = v8;
  rc.decode((v9 - *v8), v9, v39);
  if ( *_this > 0x4000u )
  {
    v28 = *(_this + 2) - (*(_this + 2) >> 1);
    v29 = *(_this + 3);
    *(_this + 2) = v28;
    LOWORD(v29) = v29 - (v29 >> 1);
    *(_this + 3) = v29;
    LOWORD(v29) = v28 + v29;
    v30 = *(_this + 4) - (*(_this + 4) >> 1);
    *(_this + 4) = v30;
    v31 = v29 + v30;
    LOWORD(v29) = *(_this + 5) - (*(_this + 5) >> 1);
    *(_this + 5) = v29;
    LOWORD(v29) = v31 + v29;
    v32 = *(_this + 6) - (*(_this + 6) >> 1);
    *(_this + 6) = v32;
    v33 = v29 + v32;
    LOWORD(v29) = *(_this + 7) - (*(_this + 7) >> 1);
    *(_this + 7) = v29;
    LOWORD(v29) = v33 + v29;
    v34 = *(_this + 8) - (*(_this + 8) >> 1);
    v35 = *(_this + 9);
    *(_this + 8) = v34;
    LOWORD(v35) = v35 - (v35 >> 1);
    *(_this + 9) = v35;
    LOWORD(v35) = v29 + v34 + v35;
    v36 = *(_this + 1);
    *_this = v35;
    if ( v36 <= 4 * alt_freq_limit )
      v16 = v36 - 4 * (v36 > alt_freq_limit);
    else
      v16 = v36 - 16;
    *(_this + 1) = v16;
  }
  else
  {
    v16 = *(_this + 1);
  }
  *v37 += v16;
  *_this += *(_this + 1);
  if ( n2 < 2 )
    return n2;
  else
  {
    v38 = level_geom[n2].half;
    v40 = 0;
    v43 = 1;
    do
    {
      v17 = _this + 2 * level_geom[n2].tbl_base + 2 * v43 + 2 * v40 + 8;
      v18 = *v17;
      v44 = v17[1];
      v23 = rc.decode_bit(v18, v44);
      n0x4000 = v17[v23];
      if ( n0x4000 > 0x4000u )
      {
        *v17 -= *v17 >> 1;
        v17[1] -= v17[1] >> 1;
        n0x4000 = v17[v23];
      }
      v17[v23] = alt_freq_init + n0x4000;
      v38 >>= 1;
      v43 *= 2;
      v25 = v23 + 2 * v40;
      v40 = v25;
    }
    while ( v38 );
    return v25 + level_geom[n2].first;
  }
}

int32_t __alt_p1_decode_symbol(uint16_t *a1, int32_t a2, int32_t a3)
{
  ;
  bool v21;
  int16_t v16, v23;
  int32_t n5, n256;
  uint16_t *v11, *v19, *v22;
  uint32_t tot, sym,
           n0x2000_2, v24;
  tot = *a1 & 0x7FFF;
  sym = rc.get_freq(tot);
  v11 = a1 + 1;
  n0x2000_2 = (uint16_t)a1[1];
  v24 = (uint32_t)(a1 + 1);
  while ( n0x2000_2 <= sym )
    n0x2000_2 += (uint16_t)*++v11;
  rc.decode(n0x2000_2 - (uint16_t)*v11, n0x2000_2, tot);
  if ( tot > 0x2000 )
  {
    *a1 = 0x8000;
    v19 = a1 + 7;
    n256 = 256;
    v21 = (uint32_t)(a1 + 7) < v24;
    v22 = a1 + 7;
    if ( (uint32_t)(a1 + 7) >= v24 )
    {
      do
      {
        if ( *v22 < n256 )
          n256 = *v22;
        --v22;
      }
      while ( (uint32_t)v22 >= v24 );
      v21 = (uint32_t)v19 < v24;
    }
    if ( !v21 )
    {
      do
      {
        if ( (uint16_t)n256 <= 1u )
          v23 = *v19 - (*v19 >> 1);
        else
          v23 = ((uint16_t)*v19 + 2) / 3;
        *v19 = v23;
        *a1 += v23;
        --v19;
      }
      while ( (uint32_t)v19 >= v24 );
    }
  }
  *v11 += 32;
  v16 = *a1 + 32;
  *a1 = v16;
  n5 = (int32_t)((int32_t)v11 - v24) >> 1;
  if ( n5 >= 5 )
    n5 += 2
        * __decode_symbol_tree(model_strip(
            128 * (uint32_t)(n5 & 1)
            + (((((uint16_t)a1[1] + (v16 & 0x7FFF) + 96 - 2 * (uint32_t)(uint16_t)a1[n5 + 1]) >> 25)
              & 0xFFFFFFC0)
            + (uint32_t)a3)));
  return n5;
}

int32_t __alt_p2_encode_symbol(P2Freq *_this, const uint32_t *a2, int32_t a3)
{
  ;
  int16_t v18;
  uint16_t *result;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t v3;
  int32_t n32;
  uint16_t *v25;
  uint32_t tot, tot_1,
           f1, f2, v23;
  v3 = _this->f[1] + _this->f[0];
  tot = v3 + _this->f[2];
  if ( a3 )
  {
    if ( (a3 & 1) != 0 )
    {
      v3 = _this->f[0];
      v25 = &_this->f[1];
    }
    else
    {
      v25 = &_this->f[2];
    }
  }
  else
  {
    v3 = 0;
    v25 = &_this->f[0];
  }
  tot_1 = v3 + *v25;
  rc.encode(v3, tot_1, tot);
  v18 = *v25;
  if ( *v25 > 0x4000u )
  {
    f1 = _this->f[1];
    f2 = _this->f[2];
    _this->f[0] -= _this->f[0] >> 1;
    n32 = _this->step;
    _this->f[1] = f1 - (f1 >> 1);
    _this->f[2] = f2 - (f2 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        v23 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(v23) = 32;
      LOWORD(n32) = n32 - v23;
      _this->step = n32;
      v18 = *v25;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      _this->step = n32;
      v18 = *v25;
    }
  }
  else
  {
    LOWORD(n32) = _this->step;
  }
  result = v25;
  *v25 = n32 + v18;
  if ( a3 > 0 )
    return __encode_symbol_tree(model_strip(a2[a3 & 1]), (a3 - 1) >> 1);
  return (int32_t)result;
}

// alt_p2_encode_symbol's counterpart, and called on the same two objects from
// the p2 decoders that the encoders hand to it.  It was `decode_three_way` for
// the shape of its first step -- a three-way choice over the counts at
// `_this[1..3]` -- but so is the encoder's first step, and a pair that codes
// the same thing should read as one.  The three-way part is still the first
// twenty lines; the name now says which half of the pair this is.
int32_t __alt_p2_decode_symbol(P2Freq *_this, const uint32_t *a2)
{
  ;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t v8;
  int32_t v7, n0x4000, n32, v16, v21;
  uint16_t *v9, *v24;
  uint32_t f1, f2, v20;
  v21 = _this->f[1] + _this->f[0];
  v20 = _this->f[2] + v21;
  v7 = rc.get_freq(v20);
  v8 = _this->f[0];
  if ( v7 >= v8 )
  {
    if ( v7 >= v21 )
    {
      v8 = v21;
      v9 = &_this->f[2];
    }
    else
    {
      v9 = &_this->f[1];
    }
    v24 = &_this->f[0];
  }
  else
  {
    v9 = &_this->f[0];
    v8 = 0;
    v24 = &_this->f[0];
  }
  rc.decode(v8, (v8 + ((uint16_t)*v9)), v20);
  n0x4000 = (uint16_t)*v9;
  if ( n0x4000 > 0x4000 )
  {
    f1 = _this->f[1];
    f2 = _this->f[2];
    _this->f[0] -= _this->f[0] >> 1;
    n32 = _this->step;
    _this->f[1] = f1 - (f1 >> 1);
    _this->f[2] = f2 - (f2 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        n0x4000 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(n0x4000) = 32;
      LOWORD(n32) = n32 - n0x4000;
      _this->step = n32;
      LOWORD(n0x4000) = *v9;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      _this->step = n32;
      LOWORD(n0x4000) = *v9;
    }
  }
  else
  {
    LOWORD(n32) = _this->step;
  }
  *v9 = n32 + n0x4000;
  v16 = v9 - v24;
  if ( v16 )
    return v16 + 2 * __decode_symbol_tree(model_strip(a2[v16 & 1]));
  else
    return 0;
}

uint32_t __rescale_three_way(P2Freq *_this)
{
  ;
  uint32_t n32, n0x100;
  _this->f[0] -= _this->f[0] >> 1;
  _this->f[1] -= _this->f[1] >> 1;
  n32 = _this->f[2] >> 1;
  _this->f[2] -= n32;
  n0x100 = _this->step;
  if ( n0x100 <= 0x100 )
  {
    if ( _this->step <= 0x20u )
      n32 = ((16 - n0x100) >> 30) & 0xFFFFFFFE;
    else
      n32 = 32;
    _this->step = n0x100 - n32;
  }
  else
  {
    _this->step >>= 1;
  }
  return n32;
}

int32_t __rescale_counter_pair(uint16_t *_this)
{
  ;
  int32_t n0x4000;
  *_this -= *_this >> 1;
  n0x4000 = (uint16_t)*(_this + 2);
  *(_this + 1) -= *(_this + 1) >> 1;
  if ( n0x4000 < 0x4000 )
  {
    n0x4000 += 64;
    *(_this + 2) = n0x4000;
  }
  return n0x4000;
}
void __rc_end_decode()
{
  rc.finish();
  if ( plane_alt_model )
    free(model_tables);
}
void __rc_end_encode()
{
  rc.flush();
  if ( plane_alt_model )
    free(model_tables);
}

void **__free_workspace(ModelBlock *Blocka, int8_t a2)
{
  ;
  int32_t v4, v6, v10, v13, i;
  SymList *v3, *sel1_list, *v8, *v9, *sel0_list, *v14;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_1;
  ModelBlock *Blocka_2;
  Blocka_1 = (ModelBlock *)(Blocka);
  free(Blocka->sym_code);
  free(Blocka_1->sym_word);
  free(Blocka_1->run_bucket);
  free(*(void**)&Blocka_1->alpha_map);
  // Both arrays are allocated as `bmf_new(24 * n + 4)` with the count in the
  // word before the first list, so `n` is `sym_list_count(lists)` and that
  // same word is what gets freed.  Each list owns its entries.
  v3 = Blocka_1->sel1_list;
  if ( v3 )
  {
    v4 = sym_list_count(v3);
    if ( v4 )
    {
      Blocka_2 = (ModelBlock *)(Blocka_1);
      v6 = v4;
      sel1_list = Blocka_1->sel1_list;
      v8 = &v3[v4];
      do
      {
        --v8;
        free(v8->ent);
        --v6;
      }
      while ( v6 );
      v3 = sel1_list;
      Blocka_1 = (ModelBlock *)(Blocka_2);
    }
    free(sym_list_block(v3));
  }
  v9 = Blocka_1->sel0_list;
  if ( v9 )
  {
    v10 = sym_list_count(v9);
    if ( v10 )
    {
      Blocka_3 = (ModelBlock *)(Blocka_1);
      sel0_list = Blocka_1->sel0_list;
      v13 = v10;
      v14 = &v9[v10];
      do
      {
        --v14;
        free(v14->ent);
        --v13;
      }
      while ( v13 );
      v9 = sel0_list;
      Blocka_1 = (ModelBlock *)(Blocka_3);
    }
    free(sym_list_block(v9));
  }
  for ( i = 0; i < 5; ++i )
    free(((void**)Blocka_1)[i + 14]);
  free(Blocka_1->escape.ent);
  if ( (a2 & 1) != 0 )
    free(Blocka_1);
  return (void **)Blocka_1;
}

// Is `sym` one of the list's `n` most-used symbols?  The list is kept sorted by
// count, so entry 0 is the most used; each of the four tests below walks its
// window from the far end back to entry 0 and stops at the first match, which
// is the order and the early exit MSVC unrolled.
static inline bool sym_in_top(const SymEntry *ent, int32_t n, int32_t sym) {
  while ( n-- )
    if ( ent[n].sym == sym )
      return true;
  return false;
}

int32_t __pixel_context(ModelBlock *_this, uint32_t *p_n15)
{
  ;
  SymList *sel0_list;
  int32_t result, v3, v4, v6, v9, v13, v14, n6;
  SymEntry *v10, *v11;
  n6 = *(int32_t *)&_this->sym_pos;
  result = (p_n15)[n6];
  if ( exclusion_mask[result] == exclusion_gen )
    return -1;
  v3 = 32
     * (result == p_n15[15] || result == p_n15[14] || result == p_n15[13] || result == p_n15[12] || result == p_n15[11])
     + ((result == p_n15[10]) << 6);
  v4 = 16
     * (result == p_n15[31]
     || result == p_n15[30]
     || result == p_n15[29]
     || result == p_n15[28]
     || result == p_n15[27]
     || result == p_n15[26]
     || result == p_n15[25]
     || result == p_n15[24]
     || result == p_n15[23]
     || result == p_n15[22]
     || result == p_n15[21]
     || result == p_n15[20]
     || result == p_n15[19]
     || result == p_n15[18]
     || result == p_n15[17]
     || result == p_n15[16]);
  v6 = v4 + v3;
  *(int32_t *)&_this->ctr_node = v6;
  if ( (v4 + v3 == 0) && n6 > 6 )
    return -1;
  sel0_list = _this->sel0_list;
  v9 = v6 + 8 * sym_in_top((sel0_list[mode_symbol[1]].ent), 10, result);
  *(int32_t *)&_this->ctr_node = v9;
  v10 = sel0_list[mode_symbol[2]].ent;
  v11 = sel0_list[result].ent;
  v13 = sym_in_top((sel0_list[mode_symbol[3]].ent), 6, result)
      + 2 * sym_in_top(v11, 10, mode_symbol[2])
      + 4 * sym_in_top(v10, 4, result);
  v14 = v13 + v9;
  if ( n6 <= 14 || (v14 & 0xB) != 0 )
  {
    *(int32_t *)&_this->ctr_node = (n6 << 7) + v14;
    *(int32_t *)&_this->ctr_fallback = v13 + 8 * (n6 > 9);
  }
  else
  {
    *(int32_t *)&_this->ctr_fallback = v13;
    return -1;
  }
  return result;
}

int32_t __init_model_tables(ModelBlock *_this)
{
  ;
  SymEntry *v11, *ent;
  // The two rows above, walked one record at a time.  Every reach through
  // them is `match[0]` -- byte 2 of an eight-byte record -- so the byte
  // offsets 34 and -30 were records +4 and -4: the window `grad` slides,
  // adding the record entering on the right and dropping the one leaving.
  PixRec *v28, *v29;            // row cursors out of row_cur
  uint8_t cnt;
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  uint16_t v6, sym;
  SymList **v3;
  PixRec *v30;   // the current row, one record past the pixel just written
  int32_t n2_1, v7, v9, v17, n2, v19, v21, v22, v23, v24, v25, v26, v27,
          result;
  uint16_t *sym_cache;
  SymList *v5;
  SymList **v4;
  n2_1 = _this->hit;
  if ( !n2_1 )
  {
    if ( _this->sel == _this->sel_cur )
    {
      if ( _this->sel[0] )
      {
        __symbol_list_update(&_this->sel0_list[mode_symbol[1]], _this->row_cur[5]->sym, 3u);
        __symbol_list_update(&_this->sel0_list[_this->row_cur[5]->sym], mode_symbol[2], 2u);
        __symbol_list_update(&_this->sel1_list[mode_symbol[1]], _this->row_cur[5]->sym, 4u);
        __symbol_list_update(&_this->sel1_list[_this->row_cur[5]->sym], mode_symbol[1], 2u);
      }
      else
      {
        __symbol_list_update(&_this->sel0_list[mode_symbol[2]], _this->row_cur[5]->sym, (_this->sym_pos > 3) + 2);
      }
    }
    else
    {
      __symbol_list_update(&_this->sel0_list[mode_symbol[1]], _this->row_cur[5]->sym, 3u);
      __symbol_list_update(&_this->sel0_list[_this->row_cur[5]->sym], mode_symbol[2], 2u);
      __symbol_list_update(&_this->sel0_list[_this->row_cur[5]->sym], mode_symbol[1], 1u);
      __symbol_list_update(&_this->sel1_list[_this->row_cur[5]->sym], mode_symbol[1], 2u);
      v3 = _this->sel_cur;
      do
      {
        v4 = v3 - 1;
        _this->sel_cur = v4;
        // `symbol_list_update`'s insert path, inlined: append the symbol at
        // `live`, evicting the last entry when the list is full, then swap it
        // one place forward.
        v5 = *v4;
        v6 = _this->row_cur[5]->sym;
        v7 = v5->live;
        ent = v5->ent;
        if ( v7 == (int32_t)v5->n )
        {
          v5->live = --v7;
          v9 = v5->ent[v7].cnt;
        }
        else
        {
          v9 = 1;
        }
        v11 = &ent[v7];
        v5->live = v7 + 1;
        v5->tot = v9 + v5->tot + 1;
        v11->sym = v6;
        v11->cnt = 2;
        v5->since_rescale += 4;
        if ( v11 != v5->ent )
        {
          sym = v11->sym;
          cnt = v11->cnt;
          v11->sym = v11[-1].sym;
          v11->cnt = v11[-1].cnt;
          v11[-1].sym = sym;
          v11[-1].cnt = cnt;
        }
        v3 = _this->sel_cur;
      }
      while ( v3 != _this->sel );
    }
    if ( exclusion_gen == -1 )
    {
      exclusion_gen = 1;
      buf = (uint8_t *)exclusion_mask;
      v17 = (_this->alphabet + 15) >> 4;
      do
      {
        bmf_zero16(buf);
        buf += 16;
        --v17;
      }
      while ( v17 );
      n2 = _this->hit;
    }
    else
    {
      ++exclusion_gen;
      n2 = _this->hit;
    }
LABEL_19:
    if ( n2 && n2 <= 2 )
      goto LABEL_37;
  }
  else
  {
    // The `goto LABEL_21` that ended the block above skipped exactly this, and
    // it was the block's last statement -- so the two are an `if`/`else` and
    // the label was the join.
    if ( n2_1 <= 2 )
      goto LABEL_37;
    if ( mode_symbol[3] != mode_symbol[4] )
    {
      __symbol_list_update(&_this->sel0_list[mode_symbol[2]], _this->row_cur[5]->sym, 1u);
      n2 = _this->hit;
      goto LABEL_19;
    }
  }
  v19 = _this->row_cur[5]->sym;
  sym_cache = _this->sym_cache;
  v21 = sym_cache[0];
  if ( v19 != v21 )
  {
    v22 = sym_cache[1];
    if ( v19 == v22 )
    {
      sym_cache[1] = v21;
    }
    else
    {
      v23 = sym_cache[2];
      if ( v19 == v23 )
      {
        sym_cache[2] = v22;
        _this->sym_cache[1] = _this->sym_cache[0];
      }
      else
      {
        v24 = sym_cache[3];
        if ( v19 == v24 )
        {
          sym_cache[3] = v23;
          _this->sym_cache[2] = _this->sym_cache[1];
          _this->sym_cache[1] = _this->sym_cache[0];
        }
        else
        {
          v25 = sym_cache[4];
          if ( v19 == v25 )
          {
            sym_cache[4] = v24;
            _this->sym_cache[3] = _this->sym_cache[2];
            _this->sym_cache[2] = _this->sym_cache[1];
            _this->sym_cache[1] = _this->sym_cache[0];
          }
          else
          {
            v26 = sym_cache[5];
            if ( v19 == v26 )
            {
              sym_cache[5] = v25;
              _this->sym_cache[4] = _this->sym_cache[3];
              _this->sym_cache[3] = _this->sym_cache[2];
              _this->sym_cache[2] = _this->sym_cache[1];
              _this->sym_cache[1] = _this->sym_cache[0];
            }
            else
            {
              v27 = sym_cache[6];
              if ( v19 == v27 )
              {
                sym_cache[6] = v26;
              }
              else
              {
                sym_cache[7] = v27;
                _this->sym_cache[6] = _this->sym_cache[5];
              }
              _this->sym_cache[5] = _this->sym_cache[4];
              _this->sym_cache[4] = _this->sym_cache[3];
              _this->sym_cache[3] = _this->sym_cache[2];
              _this->sym_cache[2] = _this->sym_cache[1];
              _this->sym_cache[1] = _this->sym_cache[0];
            }
          }
        }
      }
    }
    _this->sym_cache[0] = v19;
  }
LABEL_37:
  // The other fourteen sites already spell the 16-bit accesses out --
  // `*(uint16_t *)(pix_cur + 2) = *(uint16_t *)pix_cur` at 11752 is this
  // line -- so the field is a byte cursor and these two were the odd ones.
  _this->pix_cur[1] = _this->pix_cur[0];
  _this->pix_cur[0] = _this->row_cur[5]->sym;
  // Six neighbours, compared against the symbol just written.
  {
    PixRec *const here = _this->row_cur[5];
    PixRec *const up   = _this->row_cur[6];
    here->match[0] = here->sym == up->sym;
    here->match[1] = here->sym == here[-1].sym;
    here->match[2] = here->sym == up[1].sym;
    here->match[3] = here->sym == up[-1].sym;
    here->match[4] = here->sym == up[2].sym;
    here->match[5] = here->sym == up[3].sym;
  }
  v28 = _this->row_cur[6];
  v29 = _this->row_cur[7];
  v30 = _this->row_cur[5] + 1;
  _this->row_cur[5] = v30;
  ++_this->row_cur[8];
  ++v28;
  _this->row_cur[6] = v28;
  ++v29;
  ++_this->row_cur[9];
  _this->row_cur[7] = v29;
  _this->grad[0] += v28[4].match[0] - v28[-4].match[0];
  _this->grad[1] += v29[4].match[0] - v29[-4].match[0];
  _this->grad[2] += v30[-1].match[1] - v30[-5].match[1];
  result = v30[-1].match[0] - v30[-8].match[0];
  _this->grad[3] += result;
  return result;
}

void **__alt_p2_free(void **lpAddress, int8_t a2)
{
  ;
  free(*(lpAddress + 69689));
  free(*(lpAddress + 69690));
  free(*(lpAddress + 69691));
  free(*(lpAddress + 69692));
  free(*(lpAddress + 69693));
  free(*(lpAddress + 69665));
  free(*(lpAddress + 69666));
  if ( (a2 & 1) != 0 )
    bmf_page_free(lpAddress);
  return lpAddress;
}


void **__alt_p1_free(void **Block, int8_t a2)
{
  ;
  free(*(Block + 44));
  free(*(Block + 45));
  free(*(Block + 46));
  free(*(Block + 47));
  free(*(Block + 48));
  if ( (a2 & 1) != 0 )
    free(Block);
  return Block;
}

// Context for the alternate p1 model: reads the causal neighbourhood through
// the row pointers at `_this[49..51]`, forms the gradients (`2*W - WW - N` and
// friends) and a weighted neighbour sum, and stores them along with the model
// table pointers they select.  It codes nothing -- no `rc.` call anywhere in it
// -- and all four p1 bodies call it, encoder and decoder alike, which is what
// says it is context rather than coding.
int32_t __alt_p1_context(AltP1Block *_this, AltP1Block *a2, AltP1Block *a3)
{
  P1Ctx *v12, *v13, *cursor1, *cursor2;
  ;
  P1Ctx *v5;
  P1Ctx *cursor4;
  bool v9, v19, v20;
  P1Ctx *v34;
  int32_t v6, v7, v11, v14, v15, v16, v18, v21, v22, v23, v24, v25, v26, v27, v28,
          v29, result, v35, v36, v39, v40, v41, v42;
  uint32_t v37, v38;
  // Byte values Hex-Rays gave a pointer type: none is dereferenced here,
  // and `&v4[k]` on a `uint8_t *` is the addition it looks like.  The
  // `(uint32_t)` casts on them stay: `(216 - (uint32_t)v4) >> 31` is a
  // logical shift, and it is a selector -- signed it would be -1, not 1.
  int32_t v4, v8, v10, level_of;
  cursor1 = _this->cursor[1];
  v4 = cursor1->sym;
  v5 = _this->cursor[0];
  v6 = v5[-1].sym;
  v7 = cursor1[-1].sym;
  if ( v6 < v4 )
  {
    if ( v7 >= v6 )
    {
      v10 = v4 + v6 - v7;
      v9 = v7 < v4;
      v4 = v5[-1].sym;
      if ( v9 )
        v4 = v10;
    }
  }
  else if ( v7 <= v6 )
  {
    v8 = v4 + v6 - v7;
    v9 = v7 <= v4;
    v4 = v5[-1].sym;
    if ( !v9 )
      v4 = v8;
  }
  _this->pred = v4;
  cursor2 = _this->cursor[2];
  v11 = cursor1[-1].mag
      + v5[-3].mag
      + 3 * (cursor1[1].mag + cursor2->mag)
      + 6 * v5[-1].mag
      + 4 * (cursor1->mag + v5[-2].mag)
      + 2 * (cursor2[2].mag + cursor1[2].mag + v5[-4].mag);
  // The two neighbouring planes, reached through their own row cursors.  Both
  // rows are two bytes a pixel -- the reconstructed sample at +0 and the size
  // of the prediction error at +1 -- so the odd subscript `[-1]` is the error
  // magnitude one pixel back and the even ones `[-2]` and `[-4]` are the
  // samples one and two pixels back.  Hex-Rays typed both parameters
  // `uint32_t *` and reached the cursors as `a2[49]` and `a2[50]`, which is
  // +196 and +200: `cursor[0]` and `cursor[1]`.  `a2[2]` is `f8`.
  if ( a2 )
  {
    if ( a3 )
    {
      v34 = v5;
      v12 = a2->cursor[0];
      v36 = v11 + 2 * (a3->cursor[0][-1].mag + v12[-1].mag);
      _this->ctx_w[5].sel = (cursor1->sym
                                       - (uint32_t)v4
                                       + v12[-1].sym
                                       - a2->cursor[1][-1].sym);
      v13 = a2->cursor[0];
      v14 = v13[-1].sym;
      v15 = v13[-2].sym;
      v5 = v34;
      _this->ctx_w[6].sel = (v34[-1].sym - (uint32_t)v4 + v14 - v15);
      _this->ctx_w[7].sel = (v34[-1].sym
                                       - (uint32_t)v4
                                       + a3->cursor[0][-1].sym
                                       - a3->cursor[0][-2].sym);
      _this->ctx_w[8].sel = (a3->cursor[0][-1].sym - a3->pred);
      _this->ctx_w[3].sel = (a2->cursor[0][-1].sym - a2->pred);
      v37 = (a3->cursor[0][-1].mag + (uint32_t)a2->cursor[0][-1].mag - 16) >> 31;
    }
    else
    {
      v36 = cursor1[3].mag + v11 + 3 * a2->cursor[0][-1].mag;
      _this->ctx_w[5].sel = (2 * v5[-1].sym - v5[-2].sym - (uint32_t)v4);
      _this->ctx_w[6].sel = (2 * v5[-1].sym - v5[-2].sym - (uint32_t)v4);
      _this->ctx_w[7].sel = (-v4 - cursor1->sym + cursor1[1].sym + v5[-1].sym);
      _this->ctx_w[8].sel = (v5[-1].sym
                                       - (uint32_t)v4
                                       + a2->cursor[0][-1].sym
                                       - a2->cursor[0][-2].sym);
      _this->ctx_w[3].sel = (a2->cursor[0][-1].sym - a2->pred);
      v37 = ((uint32_t)a2->cursor[0][-1].mag - 8) >> 31;
    }
  }
  else
  {
    cursor4 = _this->cursor[4];
    v36 = cursor4->mag + cursor2[-2].mag + cursor1[3].mag + v11 + cursor4[2].mag;
    _this->ctx_w[5].sel = (2 * v5[-1].sym - v5[-2].sym - (uint32_t)v4);
    _this->ctx_w[6].sel = (2 * cursor1->sym - cursor2->sym - (uint32_t)v4);
    _this->ctx_w[7].sel = (-v4 - cursor1->sym + cursor1[1].sym + v5[-1].sym);
    _this->ctx_w[8].sel = (-3 * (v5[-2].sym - v5[-1].sym)
                                     + v5[-3].sym
                                     - (uint32_t)v4);
    _this->ctx_w[3].sel = (cursor1[2].sym - (uint32_t)v4);
    v37 = v5->mag + cursor4->mag + _this->cursor[3]->mag + cursor2->mag + cursor1->mag == 0;
  }
  v16 = (v36 + 7) >> 4;
  level_of = _this->level_of[v16];
  v35 = p1_level_step[(uint32_t)level_of];
  _this->ctx[0] = level_of;
  _this->ctx[1] = _this->group_of[v16] + _this->slot_of[(uint32_t)v4];
  _this->ctx_w[0].sel = (((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31));
  v38 = ((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31);
  v18 = (cursor1[-1].sym - cursor1->sym >= 0) + (cursor1[-1].sym > (int32_t)cursor1->sym);
  _this->ctx_w[1].sel = v18;
  v39 = (cursor1[-1].sym - v5[-1].sym >= 0) + (cursor1[-1].sym > (int32_t)v5[-1].sym);
  _this->ctx_w[2].sel = v39;
  v19 = _this->ctx_w[3].sel == 0;
  v20 = _this->ctx_w[3].sel < 0;
  v40 = (cursor1[1].sym - v4 >= -v35) + (cursor1[1].sym - v4 > v35);
  _this->ctx_w[4].sel = v40;
  v21 = v35 < _this->ctx_w[5].sel;
  v22 = -v35 <= _this->ctx_w[5].sel;
  v23 = !v20 + (!v20 && !v19);
  _this->ctx_w[3].sel = v23;
  v24 = v22 + v21;
  v19 = _this->ctx_w[6].sel == 0;
  v20 = _this->ctx_w[6].sel < 0;
  _this->ctx_w[5].sel = v24;
  v25 = !v20 && !v19;
  v26 = !v20;
  v19 = _this->ctx_w[7].sel == 0;
  v20 = _this->ctx_w[7].sel < 0;
  v41 = v26 + v25;
  _this->ctx_w[6].sel = (v26 + v25);
  v27 = !v20 && !v19;
  v28 = !v20;
  v19 = _this->ctx_w[8].sel == 0;
  v20 = _this->ctx_w[8].sel < 0;
  v42 = v28 + v27;
  _this->ctx_w[7].sel = (v28 + v27);
  v29 = !v20 + (!v20 && !v19);
  _this->ctx_w[8].sel = v29;
  result = _this->ctx_w[0].w[v38] + _this->ctx_w[1].w[v18]
         + _this->ctx_w[2].w[v39] + _this->ctx_w[3].w[v23]
         + _this->ctx_w[4].w[v40] + _this->ctx_w[5].w[v24]
         + _this->ctx_w[6].w[v41] + _this->ctx_w[7].w[v42]
         + _this->ctx_w[8].w[v29]
         + 16 * v37
         + 8 * (_this->ctx[3 + _this->ctx[2]] == 0)
         + level_of;
  _this->ctx[0] = result;
  return result;
}

int32_t __update_binary_pair(uint16_t *_this, int32_t symbol)
{
  ;
  uint8_t *v12;   // was int32_t: these hold addresses
  int32_t n0x8000, v3, v6, v7, v11, v13;
  uint16_t *v8;
  uint32_t v4, n0x2000, v10;
  n0x8000 = *_this;
  if ( (uint32_t)n0x8000 <= 0x8000 )
  {
    v3 = model_geometry[symbol];
    v4 = (_this[1] >> 2) & 0xFFFFFFE0;
    if ( ::plane_predictor == 2 )
      v4 = 15 * (_this[1] >> 5);
    *(_this + v3 + 2) += v4 + 4;
    n0x8000 = *_this + v4 + 4;
    *_this = n0x8000;
    if ( symbol >= 2 )
    {
      n0x8000 = level_geom[v3].half;
      v11 = symbol - level_geom[v3].first;
      v6 = 0;
      v12 = (uint8_t *)((int32_t)(_this + 2 * level_geom[v3].tbl_base + 8));
      v7 = 1;
      do
      {
        v8 = (uint16_t *)(v12 + 4 * (v6 + v7));
        n0x2000 = (uint16_t)v8[(n0x8000 & v11) != 0];
        v13 = (n0x8000 & v11) != 0;
        if ( n0x2000 > 0x2000 )
        {
          v10 = (uint16_t)v8[1];
          *v8 -= *v8 >> 1;
          v8[1] = v10 - (v10 >> 1);
          LOWORD(n0x2000) = v8[v13];
        }
        v7 *= 2;
        n0x8000 >>= 1;
        v6 = v13 + 2 * v6;
        v8[v13] = n0x2000 + ((alt_freq_init * ((uint32_t)(::plane_predictor == 2) + 5)) >> 3);
      }
      while ( n0x8000 );
    }
  }
  return n0x8000;
}

int32_t __alt_p1_model(AltP1Block *_this)
{
  ;
  uintptr_t result;   // an index into the counter table, and the return value
  CounterNode *node;  // was `result` too: the address role, under its own name
  CounterNode *v9;
  CounterNode *v11;
  CounterNode *v13;
  CounterNode *v17, *v23, *v31, *v39, *v47, *v55, *v63, *v71, *v79;
  int16_t v12, v15;
  int32_t v3, n5_1, v5, n5_2, n5_3, v8, v14, v16, v18, v21, v22, v24, v26,
          v29, v30, v32, v33, v34, v37, v38, v40, v41, v42, v45, v46, v48,
          v49, v50, v53, v54, v56, v57, v58, v61, v62, v64, v65, v66, v69,
          v70, v72, v73, v74, v77, v78, v80, v82, v83, v85, v86, v89, v92,
          v95, v98, v101, v104, v107, n2, n5_4;
  CounterNode *v108;
  CounterNode *v109;
  CounterNode *v19;
  CounterNode *v20;
  CounterNode *v105;
  CounterNode *v106;
  CounterNode *v27;
  CounterNode *v28;
  CounterNode *v102;
  CounterNode *v103;
  CounterNode *v35;
  CounterNode *v36;
  CounterNode *v99;
  CounterNode *v100;
  CounterNode *v43;
  CounterNode *v44;
  CounterNode *v96;
  CounterNode *v97;
  CounterNode *v51;
  CounterNode *v52;
  CounterNode *v93;
  CounterNode *v94;
  CounterNode *v59;
  CounterNode *v60;
  CounterNode *v90;
  CounterNode *v91;
  CounterNode *v67;
  CounterNode *v68;
  CounterNode *v87;
  CounterNode *v88;
  CounterNode *v75;
  CounterNode *v76;
  CounterNode *v84;
  CounterNode *v81;
  uint32_t n5, v110, v112;
  n5 = *((uint8_t)(_this->cursor[0]->sym - (uint8_t)_this->pred) + _this->fold);
  v3 = _this->ctx_w[1].sel;
  n5_1 = *((uint8_t)((uint8_t)_this->pred - _this->cursor[0]->sym) + _this->fold);
  v5 = _this->ctx_w[2].sel;
  n2 = (int32_t)(n5 - 5) >> 1;
  n5_2 = 6 - (n5 & 1);
  if ( n5 < 5 )
    n5_2 = *((uint8_t)(_this->cursor[0]->sym - (uint8_t)_this->pred) + _this->fold);
  n5_3 = 6 - (n5_1 & 1);
  if ( n5_1 < 5 )
    n5_3 = n5_1;
  n5_4 = n5_3;
  v8 = _this->ctx_w[8].w[2 - _this->ctx_w[8].sel]
     + _this->ctx_w[7].w[2 - _this->ctx_w[7].sel]
     + _this->ctx_w[6].w[2 - _this->ctx_w[6].sel]
     + _this->ctx_w[5].w[2 - _this->ctx_w[5].sel]
     + _this->ctx_w[4].w[2 - _this->ctx_w[4].sel]
     + _this->ctx_w[3].w[2 - _this->ctx_w[3].sel]
     + _this->ctx_w[2].w[2 - v5]
     + _this->ctx_w[1].w[2 - v3]
     + _this->ctx_w[0].w[1]
     + (_this->ctx[0] & 0x1F);
  v9 = &_this->counters[v8];
  v9[0].c[n5_3] += 17;
  v9[0].total += 17;
  result = _this->ctx[0];
  if ( (result & 7) != 7 )
  {
    v11 = &_this->counters[result];
    v110 = (((_this->ctx[1] & 7u) - 7) >> 31) + _this->ctx[1];
    v11[1].c[n5_2] += 11;
    v12 = v11[1].total + 11;
    v11[1].total = v12;
    if ( n5_2 >= 5 )
      __update_binary_pair(model_strip(
        128 * (n5_2 & 1)
        + v110
        + (((((v12 & 0x7FFF)
            + v11[1].c[0]
            - 2 * (uint32_t)v11[1].c[n5_2]) >> 25)
          & 0xFFFFFFC0))), (int32_t)(n5 - 5) >> 1);
    result = _this->ctx[0];
  }
  if ( (result & 7) != 0 )
  {
    v13 = &_this->counters[result];
    v14 = _this->ctx[1] - ((_this->ctx[1] & 7) != 0);
    v13[-1].c[n5_2] += 13;
    v15 = v13[-1].total + 13;
    v13[-1].total = v15;
    if ( n5_2 >= 5 )
      __update_binary_pair(model_strip(
        128 * (uint32_t)(n5_2 & 1)
        + (uint32_t)v14
        + (((((v15 & 0x7FFF)
            + v13[-1].c[0]
            - 2 * (uint32_t)v13[-1].c[n5_2]) >> 25)
          & 0xFFFFFFC0))), n2);
    result = _this->ctx[0];
  }
  if ( _this->counters[result].total < 0xCCCu )
  {
    if ( (result & 7u) < 7 )
    {
      v9[1].c[n5_4] += 7;
      v9[1].total += 7;
      result = _this->ctx[0];
    }
    if ( (result & 7) != 0 )
    {
      v9[-1].c[n5_4] += 5;
      v9[-1].total += 5;
      result = _this->ctx[0];
    }
    if ( n5_2 >= 5 )
    {
      v112 = _this->ctx[1]
           + (((_this->counters[result].c[0]
              + (_this->counters[result].total & 0x7FFF)
              - 2 * (uint32_t)_this->counters[result].c[n5_2]) >> 25)
            & 0xFFFFFFC0)
           + ((n5_2 & 1) << 7);
      if ( (v112 & 0x38) >= 0x38
        || (__update_binary_pair(model_strip(
              128 * (n5_2 & 1)
              + _this->ctx[1]
              + ((((_this->counters[result].c[0]
                  + (_this->counters[result].total & 0x7FFF)
                  - 2 * (uint32_t)_this->counters[result].c[n5_2]) >> 25)
                & 0xFFFFFFC0))
              + 8), n2),
            (v112 & 0x38) != 0) )
      {
        __update_binary_pair(model_strip(v112 - 8), n2);
      }
      result = _this->ctx[0];
    }
    v16 = _this->ctx_w[0].sel;
    if ( v16 == 1 )
    {
      v107 = result - _this->ctx_w[0].w[1];
      v108 = &_this->counters[v107 + _this->ctx_w[0].w[0]];
      v109 = &_this->counters[_this->ctx_w[0].w[2] + v107];
      v108[0].c[n5_2] += 6;
      v108[0].total += 6;
      v109[0].c[n5_2] += 6;
      v109[0].total += 6;
      v21 = _this->ctx[0];
      if ( (v21 & 7) != 7 )
      {
        v108[1].c[n5_2] += 4;
        v108[1].total += 4;
        v109[1].c[n5_2] += 4;
        v109[1].total += 4;
        v21 = _this->ctx[0];
      }
      if ( (v21 & 7) != 0 )
      {
        v108[-1].c[n5_2] += 3;
        v108[-1].total += 3;
        v109[-1].c[n5_2] += 3;
        v109[-1].total += 3;
        v21 = _this->ctx[0];
      }
    }
    else
    {
      v17 = &_this->counters[_this->ctx_w[0].w[2 - v16] + result - _this->ctx_w[0].w[v16]];
      v17[0].c[n5_2] += 7;
      v17[0].total += 7;
      v18 = v8 + _this->ctx_w[0].w[0] - _this->ctx_w[0].w[1];
      v19 = &_this->counters[_this->ctx_w[0].w[1] + _this->ctx[0] - _this->ctx_w[0].w[_this->ctx_w[0].sel]];
      v19[0].c[n5_2] += 6;
      v19[0].total += 6;
      v20 = &_this->counters[v18];
      v20[0].c[n5_4] += 4;
      v20[0].total += 4;
      v21 = _this->ctx[0];
      if ( (v21 & 7) != 7 )
      {
        v19[1].c[n5_2] += 4;
        v19[1].total += 4;
        v20[1].c[n5_4] += 2;
        v20[1].total += 2;
        v21 = _this->ctx[0];
      }
      if ( (v21 & 7) != 0 )
      {
        v19[-1].c[n5_2] += 3;
        v19[-1].total += 3;
        v20[-1].c[n5_4] += 2;
        v20[-1].total += 2;
        v21 = _this->ctx[0];
      }
    }
    v22 = _this->ctx_w[1].sel;
    if ( v22 == 1 )
    {
      v104 = v21 - _this->ctx_w[1].w[1];
      v105 = &_this->counters[v104 + _this->ctx_w[1].w[0]];
      v106 = &_this->counters[_this->ctx_w[1].w[2] + v104];
      v105[0].c[n5_2] += 6;
      v105[0].total += 6;
      v106[0].c[n5_2] += 6;
      v106[0].total += 6;
      v29 = _this->ctx[0];
      if ( (v29 & 7) != 7 )
      {
        v105[1].c[n5_2] += 4;
        v105[1].total += 4;
        v106[1].c[n5_2] += 4;
        v106[1].total += 4;
        v29 = _this->ctx[0];
      }
      if ( (v29 & 7) != 0 )
      {
        v105[-1].c[n5_2] += 3;
        v105[-1].total += 3;
        v106[-1].c[n5_2] += 3;
        v106[-1].total += 3;
        v29 = _this->ctx[0];
      }
    }
    else
    {
      v23 = &_this->counters[_this->ctx_w[1].w[2 - v22] + v21 - _this->ctx_w[1].w[v22]];
      v23[0].c[n5_2] += 7;
      v23[0].total += 7;
      v24 = _this->ctx_w[1].w[1];
      v26 = v8 + v24 - _this->ctx_w[1].w[2 - _this->ctx_w[1].sel];
      v27 = &_this->counters[(v24 + _this->ctx[0] - _this->ctx_w[1].w[_this->ctx_w[1].sel])];
      v27[0].c[n5_2] += 6;
      v27[0].total += 6;
      v28 = &_this->counters[v26];
      v28[0].c[n5_4] += 4;
      v28[0].total += 4;
      v29 = _this->ctx[0];
      if ( (v29 & 7) != 7 )
      {
        v27[1].c[n5_2] += 4;
        v27[1].total += 4;
        v28[1].c[n5_4] += 2;
        v28[1].total += 2;
        v29 = _this->ctx[0];
      }
      if ( (v29 & 7) != 0 )
      {
        v27[-1].c[n5_2] += 3;
        v27[-1].total += 3;
        v28[-1].c[n5_4] += 2;
        v28[-1].total += 2;
        v29 = _this->ctx[0];
      }
    }
    v30 = _this->ctx_w[2].sel;
    if ( v30 == 1 )
    {
      v101 = v29 - _this->ctx_w[2].w[1];
      v102 = &_this->counters[v101 + _this->ctx_w[2].w[0]];
      v103 = &_this->counters[_this->ctx_w[2].w[2] + v101];
      v102[0].c[n5_2] += 6;
      v102[0].total += 6;
      v103[0].c[n5_2] += 6;
      v103[0].total += 6;
      v37 = _this->ctx[0];
      if ( (v37 & 7) != 7 )
      {
        v102[1].c[n5_2] += 4;
        v102[1].total += 4;
        v103[1].c[n5_2] += 4;
        v103[1].total += 4;
        v37 = _this->ctx[0];
      }
      if ( (v37 & 7) != 0 )
      {
        v102[-1].c[n5_2] += 3;
        v102[-1].total += 3;
        v103[-1].c[n5_2] += 3;
        v103[-1].total += 3;
        v37 = _this->ctx[0];
      }
    }
    else
    {
      v31 = &_this->counters[_this->ctx_w[2].w[2 - v30] + v29 - _this->ctx_w[2].w[v30]];
      v31[0].c[n5_2] += 7;
      v31[0].total += 7;
      v32 = _this->ctx_w[2].w[1];
      v33 = v32 + _this->ctx[0] - _this->ctx_w[2].w[_this->ctx_w[2].sel];
      v34 = v8 + v32 - _this->ctx_w[2].w[2 - _this->ctx_w[2].sel];
      v35 = &_this->counters[v33];
      v35[0].c[n5_2] += 6;
      v35[0].total += 6;
      v36 = &_this->counters[v34];
      v36[0].c[n5_4] += 4;
      v36[0].total += 4;
      v37 = _this->ctx[0];
      if ( (v37 & 7) != 7 )
      {
        v35[1].c[n5_2] += 4;
        v35[1].total += 4;
        v36[1].c[n5_4] += 2;
        v36[1].total += 2;
        v37 = _this->ctx[0];
      }
      if ( (v37 & 7) != 0 )
      {
        v35[-1].c[n5_2] += 3;
        v35[-1].total += 3;
        v36[-1].c[n5_4] += 2;
        v36[-1].total += 2;
        v37 = _this->ctx[0];
      }
    }
    v38 = _this->ctx_w[3].sel;
    if ( v38 == 1 )
    {
      v98 = v37 - _this->ctx_w[3].w[1];
      v99 = &_this->counters[v98 + _this->ctx_w[3].w[0]];
      v100 = &_this->counters[_this->ctx_w[3].w[2] + v98];
      v99[0].c[n5_2] += 6;
      v99[0].total += 6;
      v100[0].c[n5_2] += 6;
      v100[0].total += 6;
      v45 = _this->ctx[0];
      if ( (v45 & 7) != 7 )
      {
        v99[1].c[n5_2] += 4;
        v99[1].total += 4;
        v100[1].c[n5_2] += 4;
        v100[1].total += 4;
        v45 = _this->ctx[0];
      }
      if ( (v45 & 7) != 0 )
      {
        v99[-1].c[n5_2] += 3;
        v99[-1].total += 3;
        v100[-1].c[n5_2] += 3;
        v100[-1].total += 3;
        v45 = _this->ctx[0];
      }
    }
    else
    {
      v39 = &_this->counters[_this->ctx_w[3].w[2 - v38] + v37 - _this->ctx_w[3].w[v38]];
      v39[0].c[n5_2] += 7;
      v39[0].total += 7;
      v40 = _this->ctx_w[3].w[1];
      v41 = v40 + _this->ctx[0] - _this->ctx_w[3].w[_this->ctx_w[3].sel];
      v42 = v8 + v40 - _this->ctx_w[3].w[2 - _this->ctx_w[3].sel];
      v43 = &_this->counters[v41];
      v43[0].c[n5_2] += 6;
      v43[0].total += 6;
      v44 = &_this->counters[v42];
      v44[0].c[n5_4] += 4;
      v44[0].total += 4;
      v45 = _this->ctx[0];
      if ( (v45 & 7) != 7 )
      {
        v43[1].c[n5_2] += 4;
        v43[1].total += 4;
        v44[1].c[n5_4] += 2;
        v44[1].total += 2;
        v45 = _this->ctx[0];
      }
      if ( (v45 & 7) != 0 )
      {
        v43[-1].c[n5_2] += 3;
        v43[-1].total += 3;
        v44[-1].c[n5_4] += 2;
        v44[-1].total += 2;
        v45 = _this->ctx[0];
      }
    }
    v46 = _this->ctx_w[4].sel;
    if ( v46 == 1 )
    {
      v95 = v45 - _this->ctx_w[4].w[1];
      v96 = &_this->counters[v95 + _this->ctx_w[4].w[0]];
      v97 = &_this->counters[_this->ctx_w[4].w[2] + v95];
      v96[0].c[n5_2] += 6;
      v96[0].total += 6;
      v97[0].c[n5_2] += 6;
      v97[0].total += 6;
      v53 = _this->ctx[0];
      if ( (v53 & 7) != 7 )
      {
        v96[1].c[n5_2] += 4;
        v96[1].total += 4;
        v97[1].c[n5_2] += 4;
        v97[1].total += 4;
        v53 = _this->ctx[0];
      }
      if ( (v53 & 7) != 0 )
      {
        v96[-1].c[n5_2] += 3;
        v96[-1].total += 3;
        v97[-1].c[n5_2] += 3;
        v97[-1].total += 3;
        v53 = _this->ctx[0];
      }
    }
    else
    {
      v47 = &_this->counters[_this->ctx_w[4].w[2 - v46] + v45 - _this->ctx_w[4].w[v46]];
      v47[0].c[n5_2] += 7;
      v47[0].total += 7;
      v48 = _this->ctx_w[4].w[1];
      v49 = v48 + _this->ctx[0] - _this->ctx_w[4].w[_this->ctx_w[4].sel];
      v50 = v8 + v48 - _this->ctx_w[4].w[2 - _this->ctx_w[4].sel];
      v51 = &_this->counters[v49];
      v51[0].c[n5_2] += 6;
      v51[0].total += 6;
      v52 = &_this->counters[v50];
      v52[0].c[n5_4] += 4;
      v52[0].total += 4;
      v53 = _this->ctx[0];
      if ( (v53 & 7) != 7 )
      {
        v51[1].c[n5_2] += 4;
        v51[1].total += 4;
        v52[1].c[n5_4] += 2;
        v52[1].total += 2;
        v53 = _this->ctx[0];
      }
      if ( (v53 & 7) != 0 )
      {
        v51[-1].c[n5_2] += 3;
        v51[-1].total += 3;
        v52[-1].c[n5_4] += 2;
        v52[-1].total += 2;
        v53 = _this->ctx[0];
      }
    }
    v54 = _this->ctx_w[5].sel;
    if ( v54 == 1 )
    {
      v92 = v53 - _this->ctx_w[5].w[1];
      v93 = &_this->counters[v92 + _this->ctx_w[5].w[0]];
      v94 = &_this->counters[_this->ctx_w[5].w[2] + v92];
      v93[0].c[n5_2] += 6;
      v93[0].total += 6;
      v94[0].c[n5_2] += 6;
      v94[0].total += 6;
      v61 = _this->ctx[0];
      if ( (v61 & 7) != 7 )
      {
        v93[1].c[n5_2] += 4;
        v93[1].total += 4;
        v94[1].c[n5_2] += 4;
        v94[1].total += 4;
        v61 = _this->ctx[0];
      }
      if ( (v61 & 7) != 0 )
      {
        v93[-1].c[n5_2] += 3;
        v93[-1].total += 3;
        v94[-1].c[n5_2] += 3;
        v94[-1].total += 3;
        v61 = _this->ctx[0];
      }
    }
    else
    {
      v55 = &_this->counters[_this->ctx_w[5].w[2 - v54] + v53 - _this->ctx_w[5].w[v54]];
      v55[0].c[n5_2] += 7;
      v55[0].total += 7;
      v56 = _this->ctx_w[5].w[1];
      v57 = v56 + _this->ctx[0] - _this->ctx_w[5].w[_this->ctx_w[5].sel];
      v58 = v8 + v56 - _this->ctx_w[5].w[2 - _this->ctx_w[5].sel];
      v59 = &_this->counters[v57];
      v59[0].c[n5_2] += 6;
      v59[0].total += 6;
      v60 = &_this->counters[v58];
      v60[0].c[n5_4] += 4;
      v60[0].total += 4;
      v61 = _this->ctx[0];
      if ( (v61 & 7) != 7 )
      {
        v59[1].c[n5_2] += 4;
        v59[1].total += 4;
        v60[1].c[n5_4] += 2;
        v60[1].total += 2;
        v61 = _this->ctx[0];
      }
      if ( (v61 & 7) != 0 )
      {
        v59[-1].c[n5_2] += 3;
        v59[-1].total += 3;
        v60[-1].c[n5_4] += 2;
        v60[-1].total += 2;
        v61 = _this->ctx[0];
      }
    }
    v62 = _this->ctx_w[6].sel;
    if ( v62 == 1 )
    {
      v89 = v61 - _this->ctx_w[6].w[1];
      v90 = &_this->counters[v89 + _this->ctx_w[6].w[0]];
      v91 = &_this->counters[_this->ctx_w[6].w[2] + v89];
      v90[0].c[n5_2] += 6;
      v90[0].total += 6;
      v91[0].c[n5_2] += 6;
      v91[0].total += 6;
      v69 = _this->ctx[0];
      if ( (v69 & 7) != 7 )
      {
        v90[1].c[n5_2] += 4;
        v90[1].total += 4;
        v91[1].c[n5_2] += 4;
        v91[1].total += 4;
        v69 = _this->ctx[0];
      }
      if ( (v69 & 7) != 0 )
      {
        v90[-1].c[n5_2] += 3;
        v90[-1].total += 3;
        v91[-1].c[n5_2] += 3;
        v91[-1].total += 3;
        v69 = _this->ctx[0];
      }
    }
    else
    {
      v63 = &_this->counters[_this->ctx_w[6].w[2 - v62] + v61 - _this->ctx_w[6].w[v62]];
      v63[0].c[n5_2] += 7;
      v63[0].total += 7;
      v64 = _this->ctx_w[6].w[1];
      v65 = v64 + _this->ctx[0] - _this->ctx_w[6].w[_this->ctx_w[6].sel];
      v66 = v8 + v64 - _this->ctx_w[6].w[2 - _this->ctx_w[6].sel];
      v67 = &_this->counters[v65];
      v67[0].c[n5_2] += 6;
      v67[0].total += 6;
      v68 = &_this->counters[v66];
      v68[0].c[n5_4] += 4;
      v68[0].total += 4;
      v69 = _this->ctx[0];
      if ( (v69 & 7) != 7 )
      {
        v67[1].c[n5_2] += 4;
        v67[1].total += 4;
        v68[1].c[n5_4] += 2;
        v68[1].total += 2;
        v69 = _this->ctx[0];
      }
      if ( (v69 & 7) != 0 )
      {
        v67[-1].c[n5_2] += 3;
        v67[-1].total += 3;
        v68[-1].c[n5_4] += 2;
        v68[-1].total += 2;
        v69 = _this->ctx[0];
      }
    }
    v70 = _this->ctx_w[7].sel;
    if ( v70 == 1 )
    {
      v86 = v69 - _this->ctx_w[7].w[1];
      v87 = &_this->counters[v86 + _this->ctx_w[7].w[0]];
      v88 = &_this->counters[_this->ctx_w[7].w[2] + v86];
      v87[0].c[n5_2] += 6;
      v87[0].total += 6;
      v88[0].c[n5_2] += 6;
      v88[0].total += 6;
      v77 = _this->ctx[0];
      if ( (v77 & 7) != 7 )
      {
        v87[1].c[n5_2] += 4;
        v87[1].total += 4;
        v88[1].c[n5_2] += 4;
        v88[1].total += 4;
        v77 = _this->ctx[0];
      }
      if ( (v77 & 7) != 0 )
      {
        v87[-1].c[n5_2] += 3;
        v87[-1].total += 3;
        v88[-1].c[n5_2] += 3;
        v88[-1].total += 3;
        v77 = _this->ctx[0];
      }
    }
    else
    {
      v71 = &_this->counters[_this->ctx_w[7].w[2 - v70] + v69 - _this->ctx_w[7].w[v70]];
      v71[0].c[n5_2] += 7;
      v71[0].total += 7;
      v72 = _this->ctx_w[7].w[1];
      v73 = v72 + _this->ctx[0] - _this->ctx_w[7].w[_this->ctx_w[7].sel];
      v74 = v8 + v72 - _this->ctx_w[7].w[2 - _this->ctx_w[7].sel];
      v75 = &_this->counters[v73];
      v75[0].c[n5_2] += 6;
      v75[0].total += 6;
      v76 = &_this->counters[v74];
      v76[0].c[n5_4] += 4;
      v76[0].total += 4;
      v77 = _this->ctx[0];
      if ( (v77 & 7) != 7 )
      {
        v75[1].c[n5_2] += 4;
        v75[1].total += 4;
        v76[1].c[n5_4] += 2;
        v76[1].total += 2;
        v77 = _this->ctx[0];
      }
      if ( (v77 & 7) != 0 )
      {
        v75[-1].c[n5_2] += 3;
        v75[-1].total += 3;
        v76[-1].c[n5_4] += 2;
        v76[-1].total += 2;
        v77 = _this->ctx[0];
      }
    }
    v78 = _this->ctx_w[8].sel;
    if ( v78 == 1 )
    {
      v83 = v77 - _this->ctx_w[8].w[1];
      v84 = &_this->counters[v83 + _this->ctx_w[8].w[0]];
      node = &_this->counters[_this->ctx_w[8].w[2] + v83];
      v84[0].c[n5_2] += 6;
      v84[0].total += 6;
      node[0].c[n5_2] += 6;
      node[0].total += 6;
      v85 = _this->ctx[0];
      if ( (v85 & 7) != 7 )
      {
        v84[1].c[n5_2] += 4;
        v84[1].total += 4;
        node[1].c[n5_2] += 4;
        node[1].total += 4;
        v85 = _this->ctx[0];
      }
      if ( (v85 & 7) != 0 )
      {
        v84[-1].c[n5_2] += 3;
        v84[-1].total += 3;
        node[-1].c[n5_2] += 3;
        node[-1].total += 3;
      }
    }
    else
    {
      v79 = &_this->counters[_this->ctx_w[8].w[2 - v78] + v77 - _this->ctx_w[8].w[v78]];
      v79[0].c[n5_2] += 7;
      v79[0].total += 7;
      v80 = _this->ctx_w[8].w[1];
      node = &_this->counters[v80 + _this->ctx[0] - _this->ctx_w[8].w[_this->ctx_w[8].sel]];
      v81 = &_this->counters[v80 - _this->ctx_w[8].w[2 - _this->ctx_w[8].sel] + v8];
      node[0].c[n5_2] += 6;
      node[0].total += 6;
      v81[0].c[n5_4] += 4;
      v81[0].total += 4;
      v82 = _this->ctx[0];
      if ( (v82 & 7) != 7 )
      {
        node[1].c[n5_2] += 4;
        node[1].total += 4;
        v81[1].c[n5_4] += 2;
        v81[1].total += 2;
        v82 = _this->ctx[0];
      }
      if ( (v82 & 7) != 0 )
      {
        node[-1].c[n5_2] += 3;
        node[-1].total += 3;
        result = (uint16_t)v81[-1].c[n5_4] + 2;
        v81[-1].c[n5_4] = result;
        v81[-1].total += 2;
      }
    }
  }
  return result;
}

int32_t *__alt_p1_alloc(AltP1Block *_this, int32_t i, int32_t a3, int32_t n4) {   P1Ctx *v22;
  ;
  int32_t v6, v7, v8, v10, v12, v13, v15, v17, v18, v20, v21, v27, v28;
  uint32_t n0x99C60, n0x80, n5;
  n0x99C60 = 0;
  _this->width = i;
  _this->height = a3;
  do
    __init_counter_node((uint16_t *)&_this->counters[n0x99C60++]);
  while ( n0x99C60 < 0x99C60 );
  _this->pred = 0;
  v6 = 0;
  v7 = 0;
  v8 = 0;
  v27 = n4 << 8;
  do
  {
    v28 = v8;
    _this->level_of[2 * v8] = v6;
    v10 = (2 * v8 == p1_level_edges[v6]) + v6;
    _this->group_of[2 * v8] = v27 | v7;
    _this->level_of[2 * v8 + 1] = v10;
    v12 = 2 * v8 + 1;
    v13 = (2 * v8 == p1_group_edges[v7]) + v7;
    v6 = (v12 == p1_level_edges[v10]) + v10;
    _this->group_of[2 * v8 + 1] = v13 | v27;
    v7 = (v12 == p1_group_edges[v13]) + v13;
    ++v8;
  }
  while ( (uint32_t)(v28 + 1) < 0x100 );
  n0x80 = 0;
  v15 = 0;
  do
  {
    _this->slot_of[2 * n0x80] = 8 * v15;
    v17 = (2 * n0x80 == p1_slot_edges[v15]) + v15;
    _this->slot_of[2 * n0x80 + 1] = 8 * v17;
    v18 = 2 * n0x80++ + 1 == p1_slot_edges[v17];
    v15 = v18 + v17;
  }
  while ( n0x80 < 0x80 );
  _this->ctx_w[0].w[1] = 32;
  _this->ctx_w[8].w[0] = 0;
  _this->ctx_w[7].w[0] = 0;
  _this->ctx_w[0].w[2] = 64;
  _this->ctx_w[6].w[0] = 0;
  _this->ctx_w[5].w[0] = 0;
  _this->ctx_w[4].w[0] = 0;
  _this->ctx_w[1].w[1] = 96;
  _this->ctx_w[3].w[0] = 0;
  _this->ctx_w[2].w[0] = 0;
  _this->ctx_w[1].w[0] = 0;
  _this->ctx_w[1].w[2] = 192;
  _this->ctx_w[0].w[0] = 0;
  n5 = 0;
  _this->ctx_w[2].w[1] = 288;
  _this->ctx_w[2].w[2] = 576;
  _this->ctx_w[3].w[1] = 864;
  _this->ctx_w[3].w[2] = 1728;
  _this->ctx_w[4].w[1] = 2592;
  _this->ctx_w[4].w[2] = 5184;
  _this->ctx_w[5].w[1] = 7776;
  _this->ctx_w[5].w[2] = 15552;
  _this->ctx_w[6].w[1] = 23328;
  _this->ctx_w[6].w[2] = 46656;
  _this->ctx_w[7].w[1] = 69984;
  _this->ctx_w[7].w[2] = 139968;
  _this->ctx_w[8].w[1] = 209952;
  _this->ctx_w[8].w[2] = 419904;
  do
    _this->buf[n5++] = (P1Ctx *)bmf_new(2 * _this->width + 20);
  while ( n5 < 5 );
  __alt_init_tables(_this->fold, (int8_t *)_this->unfold);
  v20 = _this->width;
  if ( _this->width > -10 )
  {
    v21 = 0;
    do
    {
      _this->buf[4][v21].sym = 72;
      _this->buf[3][v21].sym = 72;
      _this->buf[2][v21].sym = 72;
      _this->buf[1][v21].sym = 72;
      _this->buf[0][v21].sym = 72;
      _this->buf[4][v21].mag = 0;
      _this->buf[3][v21].mag = 0;
      _this->buf[2][v21].mag = 0;
      _this->buf[1][v21].mag = 0;
      _this->buf[0][v21].mag = 0;
      v20 = _this->width;
      ++v21;
    }
    while ( v21 < _this->width + 10 );
  }
  _this->cursor[0] = _this->buf[0] + v20 + 4;
  _this->cursor[1] = _this->buf[1] + v20 + 4;
  _this->cursor[2] = _this->buf[2] + v20 + 4;
  _this->cursor[3] = _this->buf[3] + v20 + 4;
  _this->cursor[4] = _this->buf[4] + v20 + 4;
  return (int32_t *)_this;
}

uint8_t *__rc_begin_encode()
{
  ;
  uint8_t *__rc_begin_encode_n256, *__rc_begin_encode_n256_1;
  int32_t v2, v3, v9, v10, v13;
  int32_t bits;          // the same slot as the buffer pointer below, in a
  uint8_t *Buffer;       // register MSVC reused; two roles, two names
  uint32_t i, v4, v6;   // offsets into model_geometry, not pointers
  *packer_word = ::packer_acc;
  Buffer = out_cursor;
  // Back the output cursor up over whatever whole bytes the packer has not
  // filled, and keep the leftover bit count.
  if ( out_cursor != (uint8_t *)packer_word )
  {
    bits = packer_free_bits - 8;
    if ( bits < 0 )
    {
      packer_free_bits = bits;
    }
    else
    {
      do
      {
        --Buffer;
        bits -= 8;
      }
      while ( bits >= 0 );
      out_cursor = Buffer;
      packer_free_bits = bits;
    }
  }
  if ( plane_alt_model )
  {
    if ( plane_predictor == 2 )
    {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    }
    else
    {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    level_geom[2].first = 2;
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    model_geometry[2] = 2;
    model_geometry[3] = 2;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    memset(&model_geometry[4], 3, 4);
    level_geom[4].half = 4;
    v2 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    memset(&model_geometry[v2], 4, 8);
    level_geom[5].half = 8;
    v3 = v2 + 2 * level_geom[4].half;
    level_geom[5].first = v3;
    level_geom[5].tbl_base = v3 - 5;
    memset(&model_geometry[v3], 0x05, 16);
    level_geom[6].half = 16;
    v4 = v3 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)v4;
    level_geom[6].tbl_base = (uint8_t)v4 - 6;
    memset(&model_geometry[v4], 0x06, 32);
    level_geom[7].half = 32;
    v6 = v4 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)v6;
    level_geom[7].tbl_base = (uint8_t)v6 - 7;
    // 64 bytes of 7, after 16 of 5 and 32 of 6 -- one level per line.  MSVC
    // inlined this third one because the length crossed its threshold, which
    // is why it arrived as a scalar head, three aligned stores and a tail.
    memset(&model_geometry[v6], 0x07, 64);
    __rc_begin_encode_n256 = (uint8_t *)bmf_new(0x7F000u);
    if ( __rc_begin_encode_n256 )
    {
      v9 = 0;
      v10 = 0;
      __rc_begin_encode_n256_1 = __rc_begin_encode_n256;
      do
      {
        v13 = v9;
        *((uint16_t *)__rc_begin_encode_n256_1 + 1) = 24 * alt_freq_limit;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 4] = 205;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 12] = 48;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 6] = 124;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 14] = 16;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 8] = 147;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 10] = 83;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 16] = 8;
        *(uint16_t *)&__rc_begin_encode_n256[v10 + 18] = 4;
        *(uint16_t *)__rc_begin_encode_n256_1 = 635;
        for ( i = 0; i < 0x7A; ++i )
        {
          *(uint16_t *)&__rc_begin_encode_n256_1[4 * i + 20] = 60;
          *(uint16_t *)&__rc_begin_encode_n256_1[4 * i + 22] = 36;
        }
        __rc_begin_encode_n256_1 += 508;
        v10 += 508;
        ++v9;
      }
      while ( (uint32_t)(v13 + 1) < 0x400 );
    }
    else
    {
      __rc_begin_encode_n256 = nullptr;
    }
    ::model_tables = (uint16_t *)__rc_begin_encode_n256;
  }
  rc.enc_init();
  return __rc_begin_encode_n256;
}

void __alt_p1_d8_encode_body(AltP1Block *_this, uint8_t *a2, uint8_t *a3)
{
  P1Ctx *cursor2, *cursor4, *cursor0, *v5, *buf3, *buf2, *buf1, *v9;
  ;
  uint8_t pred, v35;
  int32_t v4, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24,
          v25, v26, v28, v29, v30, v34, n5, n16, v38, v40, v42;
  // The five planes, held across the rotation that ends a row, and the cursor
  // the row is written through.
  int64_t v39;
  uint8_t *v32;
  __rc_begin_encode();
  if ( _this->height > 0 )
  {
    v4 = 0;
    do
    {
      ++v4;
      // The row end mirrored into the right margin: records 0..5 take -1..-6.
      // Two bytes each, which is what the twelve even offsets were.
      {
        P1Ctx *const here = (P1Ctx *)_this->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      v5 = _this->buf[4];
      buf3 = _this->buf[3];
      buf2 = _this->buf[2];
      buf1 = _this->buf[1];
      v9 = _this->buf[0];
      _this->buf[4] = buf3;
      _this->buf[3] = buf2;
      _this->buf[2] = buf1;
      _this->buf[1] = v9;
      _this->buf[0] = v5;
      v5 += 4;
      _this->cursor[0] = v5;
      v9 += 4;
      _this->cursor[1] = v9;
      _this->cursor[2] = buf1 + 4;
      _this->cursor[3] = buf2 + 4;
      _this->cursor[4] = buf3 + 4;
      ((P1Ctx *)v5)[-4] = ((P1Ctx *)v9)[3];
      // And the rest of the new row's left margin, from the row above.
      {
        P1Ctx *const here = (P1Ctx *)_this->cursor[0];
        P1Ctx *const up   = (P1Ctx *)_this->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = _this->cursor[2];
      cursor4 = _this->cursor[4];
      _this->ctx[2] = 0;
      _this->ctx[3] = 0;
      _this->ctx[4] = 0;
      v12 = cursor2[-2].mag;
      _this->ctx[3] = v12;
      v13 = cursor2[-1].mag;
      _this->ctx[4] = v13;
      v14 = cursor4[-2].mag + v12;
      _this->ctx[3] = v14;
      v15 = cursor4[-1].mag + v13;
      _this->ctx[4] = v15;
      v16 = cursor2->mag + v14;
      _this->ctx[3] = v16;
      v17 = cursor2[1].mag + v15;
      _this->ctx[4] = v17;
      v18 = cursor4->mag + v16;
      _this->ctx[3] = v18;
      v19 = cursor4[1].mag + v17;
      _this->ctx[4] = v19;
      v20 = cursor2[2].mag + v18;
      _this->ctx[3] = v20;
      v21 = cursor2[3].mag + v19;
      _this->ctx[4] = v21;
      v22 = cursor4[2].mag + v20;
      _this->ctx[3] = v22;
      v23 = cursor4[3].mag + v21;
      _this->ctx[4] = v23;
      v24 = cursor2[4].mag + v22;
      _this->ctx[3] = v24;
      v25 = cursor2[5].mag + v23;
      _this->ctx[4] = v25;
      v26 = cursor4[4].mag + v24;
      cursor0 = _this->cursor[0];
      _this->ctx[3] = v26;
      v28 = cursor4[5].mag + v25;
      _this->ctx[4] = v28;
      v29 = cursor0[-4].mag + v26;
      _this->ctx[3] = v29;
      v30 = cursor0[-3].mag + v28;
      _this->ctx[4] = v30;
      _this->ctx[3] = cursor0[-2].mag + v29;
      _this->ctx[4] = cursor0[-1].mag + v30;
      if ( !(_this->width <= 0) )
      {
        v32 = a3;
        v40 = v4;
        v42 = 0;
        do
        {
          ++v42;
          __alt_p1_context((AltP1Block *)(uint8_t **)_this, (AltP1Block *)nullptr, (AltP1Block *)0);
          pred = (uint8_t)_this->pred;
          v34 = (uint8_t)(*a2 - pred);
          v35 = *(_this->fold[v34] + _this->unfold) + pred;
          n5 = _this->fold[v34];
          n16 = (uint8_t)*v32 - (uint8_t)(v35 + *v32 - *a2);
          if ( n16 < -16 || n16 > 16 )
          {
            *v32 = *a2;
            n5 = _this->fold_hi[v34];
          }
          else
          {
            *v32 = v35;
          }
          __alt_p1_encode_symbol(&_this->counters[_this->ctx[0]].total, 16 * _this->ctx[0], _this->ctx[1], n5);
          v38 = (uint8_t)*v32;
          v39 = v38 - _this->pred;
          _this->cursor[0]->sym = v38;
          _this->cursor[0]->mag = (BYTE4(v39) ^ v39) - BYTE4(v39);
          _this->ctx[3 + _this->ctx[2]] = _this->ctx[3 + _this->ctx[2]]
                                                              + _this->cursor[0]->mag
                                                              - _this->cursor[0][-4].mag
                                                              - (_this->cursor[4][-2].mag
                                                               - _this->cursor[4][6].mag
                                                               + _this->cursor[2][-2].mag
                                                               - _this->cursor[2][6].mag);
                  _this->ctx[2] = _this->ctx[2] == 0;
          if ( _this->counters[_this->ctx[0]].total < 0x4000u )
            __alt_p1_model(_this);
          ++_this->cursor[0];
          ++v32;
          ++_this->cursor[1];
          ++_this->cursor[2];
          ++_this->cursor[3];
          ++_this->cursor[4];
          ++a2;
        }
        while ( v42 < _this->width );
        v4 = v40;
        a3 = v32;
      }
    }
    while ( v4 < *(uint32_t *)&_this->height );
  }
  __rc_end_encode();
}

void __alt_model_p1_d8_encode(uint8_t *a1, int32_t i, int32_t a3, uint8_t *a4)
{
  ;
  AltP1Block *v4;
  void **v5;
  v4 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
  if ( v4 )
    v5 = (void **)__alt_p1_alloc((AltP1Block *)v4, i, a3, 0);
  else
    v5 = nullptr;
  __alt_p1_d8_encode_body((AltP1Block *)v5, a1, a4);
  if ( v5 )
    __alt_p1_free((void **)v5, 1);
}
t_new_handler __set_new_handler(t_new_handler __out_of_memory_handler)
{
  ;
  t_new_handler __set_new_handler_pout_of_memory_handler;
  __set_new_handler_pout_of_memory_handler = ::__pout_of_memory_handler;
  ::__pout_of_memory_handler = __out_of_memory_handler;
  return __set_new_handler_pout_of_memory_handler;
}
// BMF ran its filters with the SSE unit in a particular mode: denormal
// results flushed to zero, and denormal inputs treated as zero.  It got there
// through Intel's dispatcher -- sub_4346D0, which read a CPU-feature level out
// of a global that sub_434A30 had filled in from CPUID, took an FXSAVE to ask
// whether the part could do DAZ at all, and printed an Intel runtime error and
// exited on a machine without SSE2.
//
// SSE2 is a given here -- the bodies are full of it and would fault long
// before this ran -- which settles all three questions: the level test is
// always taken, DAZ has been available on every part that has SSE2, and the
// no-SSE2 exit is unreachable.  What is left is the two mode bits, and
// <xmmintrin.h> and <pmmintrin.h> already name them.
//
// main passed 3 -- flush-to-zero and denormals-are-zero, not the third bit --
// so these two lines are the whole of what it did.
static void bmf_set_denormal_mode()
{
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}


uint8_t *__alt_p2_alloc(AltP2Block *_this, int32_t i, int32_t n4)
{
  P2Ctx *v17;
  ;
  int32_t v7, v8, v9, v13, Size, v18, v21, v23, v24, v27;
  uint32_t j, k, n0x1E60, m_1, m, n5, n0x82, n;
  void *v10;
  _this->plane_idx = n4;
  // Two records a step, which is how MSVC unrolled the seed of all 163 840.
  for ( j = 0; j < 0x14000; ++j )
  {
    _this->p2_ctr[2 * j].w2 = 0;
    _this->p2_ctr[2 * j + 1].w2 = 0;
  }
  for ( k = 0; k < 0x14000; ++k )
  {
    _this->p2_ctr[2 * k].b0 = 5;
    _this->p2_ctr[2 * k].b1 = 2;
    _this->p2_ctr[2 * k + 1].b0 = 5;
    _this->p2_ctr[2 * k + 1].b1 = 2;
  }
  n0x1E60 = 0;
  do
  {
    // Two records a pass, 0x1E60 passes: 15552 of them.
    v7 = 2 * n0x1E60;
    _this->freq[v7].f[0] = 2048;
    ++n0x1E60;
    _this->freq[v7].f[1] = 2816;
    _this->freq[v7].f[2] = 2816;
    _this->freq[v7].step = 4096;
    _this->freq[v7 + 1].f[0] = 2048;
    _this->freq[v7 + 1].f[1] = 2816;
    _this->freq[v7 + 1].f[2] = 2816;
    _this->freq[v7 + 1].step = 4096;
  }
  while ( n0x1E60 < 0x1E60 );
  v8 = 4 * plane_desc[0].w12 + 1;
  v9 = 16 * plane_desc[0].w12;
  *(uint32_t *)&_this->has_ref = (uint8_t)(plane_desc[plane_desc[_this->plane_idx + 1].src_plane + 1].flags
                                               & 8) >> 3;
  deadzone_hi = v8;
  deadzone_lo = -v8;
  *(uint32_t *)&_this->band_lo = -v9 - 7;
  *(uint32_t *)&_this->band_hi = v9 + 8;
  _this->row0 = (int32_t *)bmf_new(4 * i + 16);
  v10 = bmf_new(4 * i + 16);
  *(uint32_t *)((uint8_t *)_this + 232) = 0x3F800000 /* 1.0f */;
  _this->row1 = (int32_t *)v10;
  _this->cur = _this->row0 + i + 2;
  if ( i > -4 )
  {
    m_1 = (i + 4) / 2;
    if ( m_1 )
    {
      for ( m = 0; m < m_1; ++m )
      {
        *(uint8_t **)&_this->row1[2 * m] = (uint8_t *)_this;
        *(uint8_t **)&_this->row0[2 * m] = (uint8_t *)_this;
        *(uint8_t **)&_this->row1[2 * m + 1] = (uint8_t *)_this;
        *(uint8_t **)&_this->row0[2 * m + 1] = (uint8_t *)_this;
      }
      v13 = 2 * m + 1;
    }
    else
    {
      v13 = 1;
    }
    if ( i + 4 > (uint32_t)(v13 - 1) )
    {
      *(uint8_t **)&_this->row1[v13 - 1] = (uint8_t *)_this;
      *(uint8_t **)&_this->row0[v13 - 1] = (uint8_t *)_this;
    }
  }
  n5 = 0;
  Size = 18 * i + 234;
  do
    _this->buf[n5++] = (P2Ctx *)bmf_new(Size);
  while ( n5 < 5 );
  memset(_this->buf[0],0,Size);
  ctx_bias[3] = 0;
  v18 = 0;
  ctx_bias[2] = 0;
  ctx_bias[1] = 0;
  n0x82 = 0;
  ctx_bias[0] = 0;
  _this->cursor[0] = _this->buf[0] + 8;
  do
  {
    *(uint32_t *)&_this->ctx_delta[2 * n0x82] = (_this->plane_idx << 8) | (16 * v18);
    v21 = (2 * n0x82 == p2_ctx_edges[v18]) + v18;
    *(uint32_t *)&_this->ctx_delta[2 * n0x82 + 1] = (_this->plane_idx << 8) | (16 * v21);
    v23 = 2 * n0x82++ + 1 == p2_ctx_edges[v21];
    v18 = v23 + v21;
  }
  while ( n0x82 < 0x82 );
  v24 = 0;
  for ( n = 0; n < 0x3C; ++n )
  {
    _this->nb_ctx[2 * n] = v24;
    v27 = (2 * n == p2_len_edges[v24]) + v24;
    _this->nb_ctx[2 * n + 1] = v27;
    v24 = (2 * n + 1 == p2_len_edges[v27]) + v27;
  }
  _this->ctx = 15;
  __alt_init_tables(_this->fold, _this->unfold);
  _this->ctx_w[4].w[0] = 0;
  _this->ctx_w[0].w[1] = 64;
  _this->ctx_w[3].w[0] = 0;
  _this->ctx_w[0].w[2] = 128;
  _this->ctx_w[2].w[0] = 0;
  _this->ctx_w[1].w[1] = 192;
  _this->ctx_w[1].w[0] = 0;
  _this->ctx_w[1].w[2] = 384;
  _this->ctx_w[0].w[0] = 0;
  _this->ctx_w[2].w[1] = 576;
  _this->ctx_w[2].w[2] = 1152;
  _this->ctx_w[3].w[1] = 1728;
  _this->ctx_w[3].w[2] = 3456;
  _this->ctx_w[4].w[1] = 5184;
  _this->ctx_w[4].w[2] = 10368;
  return (uint8_t *)_this;
}

// The image descriptor `alloc_image` returns, and every reader of an image
// takes.  Sixteen bytes, then the pixels.
//
// This one is not documented anywhere -- it is BMF's own -- but it does not
// need to be inferred from the offsets its readers touch, because
// `alloc_image` writes all four words in a row and the arithmetic around them
// says what each is:
//
//   result[0] = (a2 << 16) | (uint16_t)a1;   width in the low half, height in
//                                            the high half -- a1 and a2 are its
//                                            first two parameters
//   result[1] = v9;                          v9 is the row length in bytes,
//                                            computed from the width and the
//                                            depth and rounded up per depth
//   result[2] = v7;                          v7 = ((uint8_t)n5 << 16) | ...,
//                                            so the depth byte lands at +10
//   result[3] = v10;                         v10 = v9 * a2 -- stride times
//                                            height, the size of the pixels
//   buf = (uint8_t *)result + result[3] + 16;   which start at +16
//
// The depth byte carries two flags above its six bits of depth.  0x80 is set
// only on the palette path -- `a4` true and a depth of 8 or less -- and every
// reader tests it before looking for a palette; `bmf_compress` toggles it and
// sets 0x40 around the call to the coder.  The byte after it starts as 0x40
// from `v7`'s `| 0x40000000` and is a second flag byte: `compress_image` ors
// bit 7 into it and tests bit 1.  Only +8 and +9 are genuinely unread.
struct BmfImage {
  uint16_t width;             // +0
  uint16_t height;            // +2
  uint32_t stride;            // +4   bytes per row -- a word, as result[1] is
  uint8_t  _pad8[2];          // +8
  uint8_t  depth;             // +10  bits 0..5 the depth; 0x40 and 0x80 flags
  uint8_t  flags;             // +11  0x40 from alloc_image; compress_image
                              //      sets bit 7 and toggles bit 1
  uint32_t data_size;         // +12  stride * height
  uint8_t  pixels[0];         // +16  the image itself, `data_size` bytes
};
static_assert(sizeof(BmfImage) == 16, "BmfImage is not the image header");
static_assert(__builtin_offsetof(BmfImage, stride) == 4
              && __builtin_offsetof(BmfImage, flags) == 11
              && __builtin_offsetof(BmfImage, depth) == 10
              && __builtin_offsetof(BmfImage, data_size) == 12,
              "BmfImage fields are not where alloc_image puts them");

int32_t *__alloc_image(int32_t a1, int32_t a2, int32_t n5, int32_t a4, int32_t a5)
{
  ;
  uint8_t *buf;
  int32_t *result;
  int32_t *v15;
  int32_t n4, v7, v8, v9, v10, Size;
  uint32_t v5;
  LOWORD(v5) = a1;
  n4 = n5;
  v7 = (uint8_t)n5 << 16;
  if ( n5 >= 5 && n5 <= 7 )
    goto LABEL_19;
  if ( n5 == 3 )
  {
    n4 = 4;
  }
  else
  {
    if ( !a5 )
    {
      n4 = 4;
      if ( n5 > 4 )
        n4 = n5;
    }
    if ( n4 >= 8 )
      goto LABEL_18;
  }
  if ( !a5 )
  {
    if ( n4 == 4 )
    {
      v5 = ((a1 + 7) >> 1) & 0xFFFFFFFC;
LABEL_19:
      v9 = (uint16_t)v5;
      goto LABEL_20;
    }
LABEL_18:
    LOWORD(v5) = ((n4 + 7) >> 3) * a1;
    goto LABEL_19;
  }
  if ( n4 == 1 )
  {
    v8 = (int32_t)(((uint32_t)((a1 + 7) >> 2) >> 29) + a1 + 7) >> 3;
  }
  else if ( n4 == 2 )
  {
    v8 = (int32_t)(((uint32_t)((a1 + 3) >> 1) >> 30) + a1 + 3) >> 2;
  }
  else
  {
    v8 = (int32_t)(a1 + ((uint32_t)(a1 + 1) >> 31) + 1) >> 1;
  }
  v7 = ((uint8_t)n5 << 16) | 0x40000000;
  v9 = (uint16_t)v8;
LABEL_20:
  v10 = v9 * a2;
  if ( a4 )
  {
    Size = 3 << (n5 & 31);
    if ( n5 > 8 )
      Size = 0;
    else
      v7 = ((BYTE2(v7) | 0xFFFF0080) << 16) | v7 & 0xFF00FFFF;
  }
  else
  {
    Size = 0;
  }
  result = ((int32_t *)bmf_new(v10 + Size + 19));
  if ( !result )
    return nullptr;
  // The descriptor, and the reason BmfImage looks the way it does.  Three of
  // these four words are one field each and say so now; `result[2]` is not,
  // and stays a packed store: it covers +8 through +11, and the two bytes at
  // +8 are zero only because they are written as part of it.  Splitting it
  // would mean zeroing them separately, which is more code saying less.
  BmfImage *const img = (BmfImage *)result;
  img->width = a1;
  img->height = a2;
  img->stride = v9;
  result[2] = v7;                 // +8 and +9 zero, depth at +10, flags at +11
  img->data_size = v10;
  if ( Size )
  {
    if ( (((const BmfImage *)result)->depth & 0x80) != 0 )
      buf = (uint8_t *)result + result[3] + 16;
    else
      buf = nullptr;
    v15 = (int32_t *)(result);
    memset(buf,0,Size);
    return v15;
  }
  return result;
}

// The two headers a .bmp file begins with: a 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER after it.
//
// Unlike the ObjN structs this is not recovered from the offsets the code
// happens to touch -- it is the documented layout of the format, and the code
// agrees with it at every offset it uses.  `biSize = 40` at +14 is the one that
// settles it: that field exists to say which info header this is, and 40 is
// this one.  read_bmp checks the same two numbers on the way in, `'BM'` at +0
// and 40 at +14.
//
// Packed, because `bfSize` sits at +2 and every later field is odd of its own
// alignment.  The static_asserts are the same guard the recovered structs
// carry, and here they check the port against a published layout rather than
// against itself.
#pragma pack(push, 1)
struct BmpHeader {
  uint16_t bfType;            // +0   'BM'
  uint32_t bfSize;            // +2   the whole file, header included
  uint16_t bfReserved1;       // +6
  uint16_t bfReserved2;       // +8
  uint32_t bfOffBits;         // +10  where the pixels start
  uint32_t biSize;            // +14  40
  int32_t  biWidth;           // +18
  int32_t  biHeight;          // +22  negative means top-down (§6)
  uint16_t biPlanes;          // +26  1
  uint16_t biBitCount;        // +28
  uint32_t biCompression;     // +30  0 none, 1 RLE8, 2 RLE4
  uint32_t biSizeImage;       // +34
  int32_t  biXPelsPerMeter;   // +38
  int32_t  biYPelsPerMeter;   // +42
  uint32_t biClrUsed;         // +46
  uint32_t biClrImportant;    // +50
};                            // 54 bytes, then the palette
#pragma pack(pop)
static_assert(sizeof(BmpHeader) == 54, "BmpHeader is not the BMP header");
static_assert(__builtin_offsetof(BmpHeader, bfOffBits) == 10
              && __builtin_offsetof(BmpHeader, biSize) == 14
              && __builtin_offsetof(BmpHeader, biBitCount) == 28
              && __builtin_offsetof(BmpHeader, biClrImportant) == 50,
              "BmpHeader fields are not where the format puts them");

int32_t __write_bmp(uintptr_t p_i, char *FileName, int32_t a3)
{
  // p_i, p_i_1 and p_i_2 are the same descriptor -- `p_i_1 = p_i` and
  // `p_i_2 = (uint16_t *)p_i`, and none is stepped -- so one view serves all
  // three.  Where a read of +12 was typed `uint8_t *` it stays a value cast back
  // from the size, because that is what the code then does with it: `&x[p_i]`
  // with x the size and p_i the descriptor is `p_i + data_size`, which is where
  // alloc_image put the palette.
  BmfImage *const img = (BmfImage *)p_i;
  struct alignas(16) WriteBmpFrame {   // 96 bytes, one stack frame
      uint32_t  Buffera;
      uint8_t   _gap10[4];   // was uint32_t v62
      uint8_t   _gap0[4];   // was uint8_t * v65
      uint8_t   _gap11[4];   // was int32_t v66
      uint8_t   _gap1[4];   // was FILE * Stream_v
      uint8_t   _gap2[4];   // was int32_t Buffer_2
      uint8_t   _gap3[4];   // was uint8_t * buf
      uint8_t   _gap4[4];   // was uint8_t * v72
      uint8_t   _pad8[4];
      uint8_t   _gap5[4];   // was int32_t Buffer_5
      uint8_t   _pad10[4];
      uint8_t   _gap6[4];   // was int32_t n2_2
      uint8_t   _gap7[4];   // was int32_t v75
      uint8_t   _gap8[4];   // was uint8_t * buf_2
      uint8_t   _pad14[4];
      uint8_t   _gap9[4];   // was uint32_t v77
      uint8_t   _pad16[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 96,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad16) == 64,
                "the named part of the frame moved");
  // These shared `__frame.Buffera` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t Buffera;
  uint8_t *Bufferb;
  uint8_t *Bufferc;
  uint8_t *Bufferd;
  uint32_t Size_2;
  // These shared `__frame.v62` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t v62;
  uint32_t v63;
  uint8_t *v65;
  int32_t v66;
  // These shared `__frame.v66` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v67;
  // These shared `__frame.v66` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *v68;
  int32_t Buffer_2;
  uint8_t *buf;
  uint8_t *v72;
  int32_t Buffer_5;
  int32_t n2_2;
  int32_t v75;
  uint8_t *buf_2;
  uint32_t v77;
  ;
  uintptr_t p_i_1;   // were int32_t: addresses, masked and tagged
  FILE *Stream_1, *Stream_2;
  bool v33;
  uint8_t *Bufferc_3, *Bufferc_1, *Bufferb_1, *v19, *v24, *buf_1, *v31, *v39, *buf_3, *Bufferc_2;   // `uint8_t *` beside the `char` scalars above
  int32_t v3, i, Buffer_1, n8, v14, v16, v17, v20, v21, v22, v23, v25, v28,
          n4, n2, Buffer_3, Buffer_4, Size, v40, n2_1, v49, Size_1, v55;
  uint16_t *p_i_2;
  uint32_t v15, v18, Bufferb_2, Size_3, ElementCount, v53;
  v3 = a3;
  Stream_1 = fopen(FileName, "wb");
  if ( !Stream_1 )
    return 0;
  Bufferc_3 = (uint8_t *)bmf_new(img->data_size
                                 + 8 * img->height
                                 + (img->data_size >> 5) + 2048);
  p_i_1 = p_i;
  Bufferc_1 = Bufferc_3;
  // Bufferc, Bufferc_1, Bufferc_2 and Bufferc_3 are one allocation: the chain
  // is Bufferc_1 = Bufferc_3, Bufferc = Bufferc_1, Bufferc_2 = Bufferc, and
  // none of them is ever stepped.  So one view of the header serves all four,
  // and the pixel writes through Bufferc_1[k + 54] keep the spelling they had.
  BmpHeader *bmp = (BmpHeader *)Bufferc_3;
  i = img->width;
  bmp->biSize = 40;
  bmp->bfType = 0x4D42 /* 'BM' */;
  Buffer_1 = img->height;
  Buffer_2 = Buffer_1;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = i;
  bmp->biHeight = Buffer_1;
  LOBYTE(Buffer_1) = img->depth;
  (*((int8_t *)&__frame.Buffera)) = Buffer_1;
  bmp->biPlanes = 1;
  n8 = Buffer_1 & 0x3F;
  bmp->biBitCount = n8;
  bmp->biClrImportant = 0;
  bmp->biClrUsed = 0;
  bmp->biYPelsPerMeter = 0;
  bmp->biXPelsPerMeter = 0;
  buf = Bufferc_3 + 54;
  if ( n8 <= 8 )
  {
    v14 = 1 << (n8 & 31);
    v66 = 1 << (n8 & 31);
    if ( ((*((int8_t *)&__frame.Buffera)) & 0x40) != 0 )
    {
      Buffera = 0x100u >> (n8 & 31);
      if ( v66 > 0 )
      {
        if ( v66 / 2 )
        {
          v15 = 0;
          v16 = 0;
          do
          {
            *(uint32_t *)&Bufferc_1[8 * v15 + 54] = ((uint8_t)v16 << 16)
                                                | (uint8_t)v16
                                                | ((uint8_t)v16 << 8);
            *(uint32_t *)&Bufferc_1[8 * v15 + 58] = ((uint8_t)(v16 - Buffera + 2 * Buffera) << 16)
                                                | (uint8_t)(v16 - Buffera + 2 * Buffera)
                                                | ((uint8_t)(v16 - Buffera + 2 * Buffera) << 8);
            v16 += 2 * Buffera;
            ++v15;
          }
          while ( v15 < v66 / 2 );
          p_i_1 = p_i;
          v17 = 2 * v15 + 1;
        }
        else
        {
          v17 = 1;
        }
        if ( v17 - 1 < (uint32_t)v66 )
          *(uint32_t *)&Bufferc_1[4 * v17 + 50] = ((uint8_t)(Buffera * (v17 - 1)) << 16)
                                              | (uint8_t)(Buffera * (v17 - 1))
                                              | ((uint8_t)(Buffera * (v17 - 1)) << 8);
      }
      Bufferb_1 = (uint8_t *)(uintptr_t)img->data_size;
      v67 = 4 * v66;
    }
    else if ( (*((int8_t *)&__frame.Buffera)) < 0 )
    {
      Bufferb_1 = (uint8_t *)(uintptr_t)img->data_size;
      if ( v66 <= 0 )
      {
        v67 = 4 * v14;
      }
      else
      {
        v62 = v14 / 2;
        if ( v14 / 2 )
        {
          Bufferb = (uint8_t *)(uintptr_t)img->data_size;
          v18 = 0;
          v19 = &Bufferb_1[p_i];
          do
          {
            v20 = 2 * v18;
            v21 = *(uint16_t *)&v19[6 * v18 + 19];
            v22 = (uint8_t)v19[6 * v18 + 21];
            *(uint32_t *)&Bufferc_1[4 * v20 + 54] = ((uint8_t)v19[6 * v18 + 18] << 16)
                                                | *(uint16_t *)&v19[6 * v18 + 16];
            *(uint32_t *)&Bufferc_1[4 * v20 + 58] = (v22 << 16) | v21;
            ++v18;
          }
          while ( v18 < v62 );
          Bufferb_1 = Bufferb;
          p_i_1 = p_i;
          v3 = a3;
          v23 = 2 * v18 + 1;
          v63 = v23;
        }
        else
        {
          v23 = 1;
          v63 = 1;
        }
        if ( v23 - 1 < (uint32_t)v66 )
        {
          Bufferd = Bufferb_1;
          v24 = &Bufferb_1[p_i_1];
          v25 = *(uint16_t *)&v24[3 * v63 + 13];
          Bufferb_1 = Bufferd;
          *(uint32_t *)&Bufferc_1[4 * v63 + 50] = (((uint8_t)v24[3 * v63 + 15]) << 16) | v25;
          p_i_1 = p_i;
        }
        v67 = 4 * v66;
      }
    }
    else
    {
      v67 = 4 * v14;
      memset(buf,0,4 * v14);
      p_i_1 = p_i;
      Bufferb_1 = (uint8_t *)(uintptr_t)img->data_size;
      Buffer_2 = img->height;
    }
    buf = &Bufferc_1[v67 + 54];
  }
  else
  {
    Bufferb_1 = (uint8_t *)(uintptr_t)img->data_size;
  }
  Size_2 = img->stride;
  Bufferc = Bufferc_1;
  v65 = (uint8_t *)(buf - Bufferc_1);
  p_i_2 = (uint16_t *)p_i_1;
  v28 = v3;
  Bufferb_2 = (uint32_t)Bufferb_1;
  while ( 1 )
  {
    buf_1 = buf;
    v31 = (uint8_t *)p_i_2 + Bufferb_2 - Size_2 + 16;
    bmp->bfOffBits = (uintptr_t)v65;
    if ( !v28 )
      break;
    n4 = p_i_2[5] & 0x3F;
    v33 = n4 == 4;
    if ( n4 != 4 )
    {
      if ( n4 != 8 )
        break;
      v33 = 0;
    }
    n2 = 2;
    if ( !v33 )
      n2 = 1;
    bmp->biCompression = n2;
    v75 = n2 - 1;
    n2_2 = (0x100u >> ((n2 - 1) & 31)) - 1;
    if ( Buffer_2 > 0 )
    {
      Buffer_3 = Buffer_2;
      Size_3 = Size_2;
      Buffer_4 = 0;
      while ( 1 )
      {
        if ( v31 >= &v31[Size_3] )
          goto LABEL_72;
        buf_2 = buf_1;
        v77 = (uint32_t)&v31[Size_3];
        Buffer_5 = Buffer_4;
        Size = 0;
        do
        {
          while ( 1 )
          {
            while ( 1 )
            {
              v39 = v31 + 1;
              if ( (uint32_t)(v31 + 1) >= v77 )
                break;
              v40 = (uint8_t)*v31;
              n2_1 = 1;
              do
              {
                if ( v40 != (uint8_t)v31[n2_1] )
                  break;
                ++n2_1;
              }
              while ( v77 > (uint32_t)&v31[n2_1] );
              if ( n2_1 <= 2 && (n2_1 != 2 || Size) )
                break;
              if ( n2_2 < n2_1 )
                n2_1 = n2_2;
              if ( Size )
              {
                if ( v75 )
                {
                  if ( Size != 1 )
                  {
  LABEL_67:
                    *buf_2 = 0;
                    v72 = buf_2 + 2;
                    buf_2[1] = Size << (v75 & 31);
                    memcpy(buf_2 + 2,&v31[-Size],Size);
                    buf_2 += Size + 2;
                    if ( (Size & 1) != 0 )
                    {
                      v72[Size] = 0;
                      ++buf_2;
                    }
                    goto LABEL_69;
                  }
                  *buf_2 = 2;
                  buf_2[1] = *(v31 - 1);
                  buf_2 += 2;
                }
                else
                {
                  if ( Size >= 3 )
                    goto LABEL_67;
                  if ( Size == 2 )
                  {
                    *buf_2 = 1;
                    buf_2[1] = *(v31 - 2);
                    buf_2 += 2;
                  }
                  *buf_2 = 1;
                  buf_2[1] = *(v31 - 1);
                  buf_2 += 2;
                }
  LABEL_69:
                Size = 0;
                LOBYTE(v40) = *v31;
              }
              buf_2[1] = v40;
              v31 += n2_1;
              *buf_2 = n2_1 << (v75 & 31);
              buf_3 = buf_2 + 2;
              buf_2 += 2;
              if ( (uint32_t)v31 >= v77 )
              {
                buf_1 = buf_3;
                Buffer_4 = Buffer_5;
                Buffer_3 = img->height;
                Size_3 = img->stride;
                goto LABEL_72;
              }
            }
            ++v31;
            if ( ++Size != n2_2 )
              break;
            if ( v75 )
            {
              if ( Size == 1 )
              {
                *buf_2 = 2;
                buf_2[1] = *(v39 - 1);
                buf_2 += 2;
                goto LABEL_55;
              }
            }
            else if ( Size < 3 )
            {
              if ( Size == 2 )
              {
                *buf_2 = 1;
                buf_2[1] = *(v39 - 2);
                buf_2 += 2;
              }
              *buf_2 = 1;
              buf_2[1] = *(v39 - 1);
              buf_2 += 2;
              goto LABEL_55;
            }
            *buf_2 = 0;
            v68 = buf_2 + 2;
            buf_2[1] = Size << (v75 & 31);
            memcpy(buf_2 + 2,&v39[-Size],Size);
            buf_2 += Size + 2;
            if ( (Size & 1) != 0 )
            {
              v68[Size] = 0;
              ++buf_2;
            }
LABEL_55:
            if ( (uint32_t)v39 >= v77 )
            {
              buf_1 = buf_2;
              Buffer_4 = Buffer_5;
              Buffer_3 = img->height;
              Size_3 = img->stride;
              goto LABEL_72;
            }
            Size = 0;
          }
        }
        while ( (uint32_t)v39 < v77 );
        buf_1 = buf_2;
        Buffer_4 = Buffer_5;
        if ( !Size )
          goto LABEL_89;
        if ( v75 )
        {
          if ( Size == 1 )
          {
            buf_2[1] = *(v39 - 1);
            *buf_2 = 2;
            Buffer_3 = img->height;
            Size_3 = img->stride;
            buf_1 = buf_2 + 2;
            goto LABEL_72;
          }
LABEL_97:
          *buf_2 = 0;
          buf_2[1] = Size << (v75 & 31);
          memcpy(buf_2 + 2,&v39[-Size],Size);
          Buffer_4 = Buffer_5;
          buf_1 = &buf_2[Size + 2];
          if ( (Size & 1) != 0 )
          {
            buf_2[Size + 2] = 0;
            Buffer_3 = img->height;
            Size_3 = img->stride;
            ++buf_1;
            goto LABEL_72;
          }
LABEL_89:
          Buffer_3 = img->height;
          Size_3 = img->stride;
          goto LABEL_72;
        }
        if ( Size >= 3 )
          goto LABEL_97;
        if ( Size == 2 )
        {
          *buf_2 = 1;
          buf_2[1] = *(v39 - 2);
          buf_1 = buf_2 + 2;
        }
        buf_1[1] = *(v39 - 1);
        *buf_1 = 1;
        Buffer_3 = img->height;
        Size_3 = img->stride;
        buf_1 += 2;
LABEL_72:
        *buf_1 = 0;
        buf_1[1] = 0;
        buf_1 += 2;
        ++Buffer_4;
        v31 -= 2 * Size_3;
        if ( Buffer_4 >= Buffer_3 )
        {
          Buffer_2 = Buffer_3;
          Size_2 = Size_3;
          p_i_2 = (uint16_t *)p_i;
          Bufferb_2 = img->data_size;
          break;
        }
      }
    }
    *buf_1 = 0;
    buf_1[1] = 1;
    buf_1 += 2;
    v49 = buf_1 - buf;
    if ( Bufferb_2 > buf_1 - buf )
    {
      // The shared tail, copied here: the two assignments it used to reach it
      // through are gone with the `goto`, so it names `Bufferc` and `Stream_1`
      // directly.
      bmp->biSizeImage = (uint32_t)v49;
      ElementCount = (uint32_t)(buf_1 - Bufferc);
      bmp->bfSize = ElementCount;
      if ( fwrite(Bufferc, 1u, ElementCount, Stream_1) != bmp->bfSize )
        return 0;
      free(Bufferc);
      fclose(Stream_1);
      return 1;
    }
    v28 = 0;
  }
  Stream_2 = Stream_1;
  Bufferc_2 = Bufferc;
  bmp->biCompression = 0;
  if ( Buffer_2 <= 0 )
  {
    v49 = 0;
  }
  else
  {
    v53 = ((Size_2 + 3) & 0xFFFFFFFC) - Size_2;
    Size_1 = Size_2;
    v55 = 0;
    do
    {
      memcpy(buf_1,v31,Size_1);
      Size_1 = img->stride;
      buf_1 += Size_1;
      v31 -= Size_1;
      if ( v53 )
      {
        *(uint32_t *)buf_1 = 0;
        buf_1 += v53;
      }
      ++v55;
    }
    while ( v55 < img->height );
    Bufferc_2 = Bufferc;
    Stream_2 = Stream_1;
    v49 = buf_1 - buf;
  }
  bmp->biSizeImage = (uint32_t)v49;
  ElementCount = (uint32_t)(buf_1 - Bufferc_2);
  bmp->bfSize = ElementCount;
  if ( fwrite(Bufferc_2, 1u, ElementCount, Stream_2) != bmp->bfSize )
    return 0;
  free(Bufferc_2);
  fclose(Stream_2);
  return 1;
}
uint32_t __init_symbol_list(SymList *a1, int32_t a2, int32_t a3, int32_t a4)
{
  ;
  SymEntry *buf;
  uint32_t v7, v10, result;   // counts, like the header fields they move
  a1->n = a3;
  buf = (SymEntry *)bmf_new(3 * a3);
  a1->ent = buf;
  if ( a4 )
  {
    v7 = a1->n;
    a1->tot = 0;
    a1->live = v7;
    result = 12 * v7;
    a1->since_rescale = 12 * v7;
    a1->rescale_at = 8 * v7;
    if ( v7 )
    {
      result = 0;
      do
      {
        a1->ent[result].sym = result;
        a1->ent[result].cnt = 1;
        ++result;
      }
      while ( result < a1->live );
    }
  }
  else
  {
    v10 = a1->n;
    a1->tot = 2;
    a1->rescale_at = 20 * v10;
    a1->live = 0;
    a1->since_rescale = 18 * v10;
    return (uint32_t)memset(buf,0,3 * v10);
  }
  return result;
}

// `width * height` off an image header the caller holds as bytes.  Both fields
// are `uint16_t` at +0 and +2, which is what `*(uint16_t *)p * *((uint16_t *)p
// + 1)` was spelling at eight sites in the two plane-shuffling functions --
// where the same name is also a byte cursor, so it cannot simply be retyped.
static inline int32_t bmf_pixels(const uint8_t *p) {
  const BmfImage *img = (const BmfImage *)p;
  return img->width * img->height;
}

uint8_t * __interleave_plane(uint8_t *p_i, uint8_t *Src, int32_t a3, int8_t a4)
{
  ;
  uint8_t *Src_5, *v25;
  int32_t v4, n6_2, v8, n6_1, v10, Size, n4, Size_2, v15, Size_1, v17, v18,
          v20, v21, v23, n2, v29, v30, v31, v32, n4_1, v34, n6;
  uint32_t v6, Src_1, Src_2;
  uint8_t *Src_4, *Src_3;
  if ( (plane_desc[a3 + 1].flags & 8) == 0 )
  {
    Size = bmf_pixels(p_i);
    n4 = plane_count;
    Src_1 = (uint32_t)&((const BmfImage *)p_i)->pixels[a3];
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(&((BmfImage *)p_i)->pixels[a3],Src,Size);
    p_i += a3;
    if ( Size <= 6
      || plane_count <= 0
      || (Src_1 <= (uint32_t)Src || Size > Src_1 - (uint32_t)Src)
      && (plane_count > 1 || (uint32_t)Src <= Src_1 || (uint32_t)&Src[-Src_1] < Size * plane_count) )
    {
      Size_1 = 0;
      v17 = 0;
      do
      {
        p_i[v17 + 16] = Src[Size_1];
        v17 += n4;
        ++Size_1;
      }
      while ( Size_1 < Size );
    }
    else
    {
      Size_2 = 0;
      v15 = 0;
      do
      {
        p_i[v15 + 16] = Src[Size_2];
        v15 += n4;
        ++Size_2;
      }
      while ( Size_2 < Size );
    }
    return p_i;
  }
  n4_1 = plane_count;
  n6 = bmf_pixels(p_i);
  v29 = plane_desc[1].src_plane - a3;
  v31 = plane_desc[2].src_plane - a3;
  n2 = plane_desc[a3 + 1].predictor;
  v34 = plane_desc[a3 + 1].b3;
  Src_2 = (uint32_t)&((const BmfImage *)p_i)->pixels[a3];
  v4 = plane_desc[a3 + 1].w8;
  v32 = plane_desc[a3 + 1].w4;
  v30 = plane_desc[a3 + 1].w12;
  if ( n2 != 2 || v32 + v4 != 128 )
    goto LABEL_3;
  if ( !v4 )
    goto LABEL_4;
  if ( v32 )
  {
LABEL_3:
    if ( plane_desc[a3 + 1].predictor != 1 )
    {
      if ( n2 == 2 )
      {
        v21 = bmf_pixels(p_i);
        Src_3 = (uint8_t *)Src_2;
        do
        {
          v23 = v34 + (uint8_t)*Src++;
          *Src_3 = (uint8_t)(((v32 * (uint8_t)Src_3[v29]
                               + v4 * (uint32_t)(uint8_t)Src_3[v31] + 40) >> 7)
                             + v23);
          Src_3 += n4_1;
          --v21;
        }
        while ( v21 );
      }
      else if ( n2 == 3 )
      {
        v18 = bmf_pixels(p_i);
        Src_4 = (uint8_t *)Src_2;
        do
        {
          v20 = v34 + (uint8_t)*Src++;
          *Src_4 = (uint8_t)(((v4 * (uint8_t)*(Src_4 - 2)
                               + v32 * (uint8_t)*(Src_4 - 3)
                               + v30 * (uint32_t)(uint8_t)*(Src_4 - 1)
                               + 63) >> 7)
                             + v20);
          Src_4 += n4_1;
          --v18;
        }
        while ( v18 );
      }
      return p_i;
    }
    goto LABEL_4;
  }
  v29 = plane_desc[2].src_plane - a3;
LABEL_4:
  p_i += a3;
  v25 = &p_i[v29];
  if ( n6 <= 6 )
    goto LABEL_25;
  Src_5 = &p_i[v29 + 16];
  if ( plane_count <= 0 )
  {
    if ( plane_count >= 0 )
      goto LABEL_25;
    if ( (uint32_t)Src_5 >= Src_2 || (v6 = n6 * plane_count, Src_2 - (uint32_t)Src_5 <= -(n6 * plane_count)) )
    {
      if ( (uint32_t)Src_5 <= Src_2 )
        goto LABEL_25;
      v6 = n6 * plane_count;
      if ( (uint32_t)&Src_5[-Src_2] <= -(n6 * plane_count) )
        goto LABEL_25;
    }
  }
  else if ( (uint32_t)Src_5 >= Src_2 || (v6 = n6 * plane_count, n6 * plane_count > Src_2 - (uint32_t)Src_5) )
  {
    if ( (uint32_t)Src_5 <= Src_2 )
      goto LABEL_25;
    v6 = n6 * plane_count;
    if ( (uint32_t)&Src_5[-Src_2] < n6 * plane_count )
      goto LABEL_25;
  }
  if ( plane_count <= 0
    || ((uint32_t)Src >= Src_2 || Src_2 - (uint32_t)Src < n6)
    && (plane_count > 1 || (uint32_t)Src <= Src_2 || (uint32_t)&Src[-Src_2] < v6) )
  {
LABEL_25:
    n6_1 = 0;
    v10 = 0;
    do
    {
      p_i[v10 + 16] = v25[v10 + 16] + v34 + Src[n6_1];
      v10 += n4_1;
      ++n6_1;
    }
    while ( n6_1 < n6 );
    return p_i;
  }
  n6_2 = 0;
  v8 = 0;
  do
  {
    p_i[v8 + 16] = v25[v8 + 16] + v34 + Src[n6_2];
    v8 += n4_1;
    ++n6_2;
  }
  while ( n6_2 < n6 );
  return p_i;
}

uint8_t * __colour_transform(uint8_t *Blockb, uint8_t *Src, int32_t a3, int8_t a4)
{
  ;
  uint8_t v8, v11;
  uint8_t *v29;   // `uint8_t *` beside the `char` scalars above
  int32_t v4, n6_2, v7, n6_1, v10, Size, n4, Size_1, v16, Size_2, v18, v19,
          v21, v22, v23, v24, v26, n2, v33, v34, v35, v36, v37, n4_1, n6;
  uint32_t Src_2, Src_1, v27, Src_3;
  uint8_t *Src_5, *Src_4;
  if ( (plane_desc[a3 + 1].flags & 8) == 0 )
  {
    Size = bmf_pixels(Blockb);
    n4 = plane_count;
    Src_1 = (uint32_t)&((const BmfImage *)Blockb)->pixels[a3];
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(Src,&((const BmfImage *)Blockb)->pixels[a3],Size);
    Blockb += a3;
    if ( Size > 6 && plane_count > 0 )
    {
      if ( plane_count > 1 )
        goto LABEL_30;
      if ( (uint32_t)Src > Src_1 && (uint32_t)&Src[-Src_1] >= Size * plane_count )
        goto LABEL_31;
      if ( plane_count > 0 )
      {
LABEL_30:
        if ( Src_1 > (uint32_t)Src && Size <= Src_1 - (uint32_t)Src )
        {
LABEL_31:
          Size_1 = 0;
          v16 = 0;
          do
          {
            Src[Size_1] = Blockb[v16 + 16];
            v16 += n4;
            ++Size_1;
          }
          while ( Size_1 < Size );
          return Blockb;
        }
      }
    }
    Size_2 = 0;
    v18 = 0;
    do
    {
      Src[Size_2] = Blockb[v18 + 16];
      v18 += n4;
      ++Size_2;
    }
    while ( Size_2 < Size );
    return Blockb;
  }
  n4_1 = plane_count;
  n6 = bmf_pixels(Blockb);
  v33 = plane_desc[1].src_plane - a3;
  v35 = plane_desc[2].src_plane - a3;
  n2 = plane_desc[a3 + 1].predictor;
  v37 = plane_desc[a3 + 1].b3;
  Src_3 = (uint32_t)&((const BmfImage *)Blockb)->pixels[a3];
  v4 = plane_desc[a3 + 1].w8;
  v36 = plane_desc[a3 + 1].w4;
  v34 = plane_desc[a3 + 1].w12;
  if ( n2 == 2 && v36 + v4 == 128 )
  {
    if ( !v4 )
      goto LABEL_4;
    if ( !v36 )
    {
      v33 = plane_desc[2].src_plane - a3;
      goto LABEL_4;
    }
  }
  if ( plane_desc[a3 + 1].predictor == 1 )
  {
LABEL_4:
    Blockb += a3;
    v29 = &Blockb[v33];
    if ( n6 <= 6
      || (Src_2 = (uint32_t)&Blockb[v33 + 16], plane_count <= 0)
      || (plane_count > 1 || (uint32_t)Src <= Src_2 || (uint32_t)&Src[-Src_2] < n6 * plane_count)
      && (Src_2 <= (uint32_t)Src || Src_2 - (uint32_t)Src < n6)
      || (plane_count > 1 || (uint32_t)Src <= Src_3 || (uint32_t)&Src[-Src_3] < n6 * plane_count)
      && ((uint32_t)Src >= Src_3 || Src_3 - (uint32_t)Src < n6) )
    {
      n6_1 = 0;
      v10 = 0;
      do
      {
        v11 = Blockb[v10 + 16] - v37 - v29[v10 + 16];
        v10 += n4_1;
        Src[n6_1++] = v11;
      }
      while ( n6_1 < n6 );
    }
    else
    {
      n6_2 = 0;
      v7 = 0;
      do
      {
        v8 = Blockb[v7 + 16] - v37 - v29[v7 + 16];
        v7 += n4_1;
        Src[n6_2++] = v8;
      }
      while ( n6_2 < n6 );
    }
    return Blockb;
  }
  if ( n2 == 2 )
  {
    v24 = bmf_pixels(Blockb);
    Src_4 = (uint8_t *)Src_3;
    do
    {
      v26 = *Src_4 - v37;
      v27 = v36 * Src_4[v33] + v4 * Src_4[v35] + 40;
      Src_4 += n4_1;
      *Src++ = (uint8_t)(v26 - (v27 >> 7));
      --v24;
    }
    while ( v24 );
  }
  else if ( n2 == 3 )
  {
    v19 = bmf_pixels(Blockb);
    Src_5 = (uint8_t *)Src_3;
    do
    {
      v21 = *Src_5 - v37;
      v22 = v4 * *(Src_5 - 2) + v36 * *(Src_5 - 3);
      v23 = v34 * *(Src_5 - 1);
      Src_5 += n4_1;
      *Src++ = (uint8_t)(v21 - ((uint32_t)(v22 + v23 + 63) >> 7));
      --v19;
    }
    while ( v19 );
  }
  return Blockb;
}

__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...)
{
  ;
  va_list ap;
  // BMF kept these in a table of pointers at 0x00441068 and indexed it by the
  // exit code, which is why the code is both the message and the status.  Codes
  // 0 and 2 are the usage text and "Unknown option", and neither can be reached
  // from this command line; they are here because the numbering is the
  // original's and shifting it would make the exit statuses lie.
  static const char *const message[] = {
    "",                             // 0  usage -- BMF's, not reachable here
    "File not found: %s",           // 1
    "Unknown option: %s",           // 2  no switches to get wrong
    "%s: bad file!",                // 3
    "Read error!",                  // 4
    "Write error for file %s",      // 5
    "Can't open file: %s",          // 6
    "Out of memory!",               // 7  the new-handler, via __out_of_memory_handler
    "User break!",                  // 8
  };
  va_start(ap, Code);
  vprintf(message[(uint32_t)Code < 9 ? Code : 4], ap);
  va_end(ap);
  printf("\n");
  exit(Code);
  __builtin_unreachable();
}
int32_t __rc_begin_decode(int8_t ArgList_1)
{
  ;
  int32_t bits_left, v7, v8, v16, v17, v20;
  uint16_t *n256, *tbl;
  uint32_t i, v9, v11;   // offsets into model_geometry, not pointers
  uint8_t *v1;
  v1 = out_cursor;
  if ((uint32_t *)out_cursor != packer_word)
  {
    bits_left = ::packer_free_bits - 8;
    if ( ::packer_free_bits - 8 < 0 )
    {
      ::packer_free_bits -= 8;
    }
    else
    {
      do
      {
        --v1;
        bits_left -= 8;
      }
      while ( bits_left >= 0 );
      out_cursor = v1;
      ::packer_free_bits = bits_left;
    }
  }
  if ( plane_alt_model )
  {
    if ( plane_predictor == 2 )
    {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    }
    else
    {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    level_geom[2].first = 2;
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    model_geometry[2] = 2;
    model_geometry[3] = 2;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    memset(&model_geometry[4], 3, 4);
    level_geom[4].half = 4;
    v7 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    memset(&model_geometry[v7], 4, 8);
    level_geom[5].half = 8;
    v8 = v7 + 2 * level_geom[4].half;
    level_geom[5].first = v8;
    level_geom[5].tbl_base = v8 - 5;
    memset(&model_geometry[v8], 0x05, 16);
    level_geom[6].half = 16;
    v9 = v8 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)v9;
    level_geom[6].tbl_base = (uint8_t)v9 - 6;
    memset(&model_geometry[v9], 0x06, 32);
    level_geom[7].half = 32;
    v11 = v9 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)v11;
    level_geom[7].tbl_base = (uint8_t)v11 - 7;
    memset(&model_geometry[v11], 0x07, 64);
    n256 = (uint16_t *)bmf_new(0x7F000u);
    if ( n256 )
    {
      tbl = n256;
      v16 = 0;
      v17 = 0;
      do
      {
        v20 = v16;
        tbl[1] = 24 * alt_freq_limit;
        n256[v17 + 2] = 205;
        n256[v17 + 6] = 48;
        n256[v17 + 3] = 124;
        n256[v17 + 7] = 16;
        n256[v17 + 4] = 147;
        n256[v17 + 5] = 83;
        n256[v17 + 8] = 8;
        n256[v17 + 9] = 4;
        *tbl = 635;
        for ( i = 0; i < 0x7A; ++i )
        {
          tbl[2 * i + 10] = 60;
          tbl[2 * i + 11] = 36;
        }
        tbl += 254;
        v17 += 254;
        ++v16;
      }
      while ( (uint32_t)(v20 + 1) < 0x400 );
    }
    else
    {
      n256 = nullptr;
    }
    ::model_tables = n256;
    v1 = out_cursor;
  }
  rc.dec_init();
  return (int32_t)(uintptr_t)out_cursor;
}

// The inverse of `predict_med`, and the direction that actually runs: this is
// what `testfiles/med32.bmp` exercises.  It walks *forwards*, the opposite way,
// for the same reason -- a residual can only be added to neighbours that have
// already been reconstructed, and forwards is where those are.
//
// So the peeled cases are the mirror image.  Pixel 0 is stored raw and is
// already correct, which is why `p` starts one past it; the rest of the first
// row has only a west neighbour; the first column of every later row has only
// a north one; and everything else gets the full MED tree, identical to the
// one in `predict_med` because it must reproduce the same prediction from the
// same three neighbours.
//
// Returns one past the last byte it wrote.
uint8_t *__unpredict_med(uint8_t *pixels, int32_t width, int32_t height)
{
  ;
  uint32_t done;     // how far the unrolled first row got: an index, not an address
  uint8_t cur;
  int32_t rows_left, pred, north, northwest;
  uint32_t j, row_rest, pairs, k, x_left;
  uint8_t *p, *up;
  alignas(16) uint8_t unfold[255];
  p = (pixels + 1);
  // The test here was `if ( plane_predictor )`, with a 45-line else building
  // a table for predictor mode 0.  Nothing reaches it: expand_image calls
  // this from two places and both are guarded by the predictor being 1 --
  // the second through n2_2, which is `*(uint32_t *)p_i` read back from
  // `*(uint32_t *)p_i = n2_1`, and n2_1 is `plane_desc[v37 + 1].flags & 3`,
  // the predictor itself.  The test is always true, so it is gone with its
  // else.  See REFACTORING.md section 2.3.
  // The unfolding table, code -> residual: code 0 is no change and the rest
  // alternate -1, +1, -2, +2 ... out to +-127.  `predict_med` builds the
  // inverse.
  unfold[0] = 0;
  for ( j = 0; j < 127; ++j )
  {
    unfold[2 * j + 1] = (uint8_t)(-1 - (int32_t)j);
    unfold[2 * j + 2] = (uint8_t)(1 + j);
  }
  if ( width == 1 )
  {
    rows_left = height - 1;
    if ( height == 1 )
      return p;
    row_rest = 0;
    goto LABEL_29;
  }
  // The first row: each pixel is its west neighbour plus its residual, and
  // MSVC unrolled it two at a time -- `cur` is the first of the pair, needed
  // whole before the second can use it.  `done` is where the unrolled part
  // stopped and the `if` below finishes the odd pixel.
  row_rest = width - 1;
  pairs = (width - 1) / 2;
  if ( pairs )
  {
    for ( k = 0; k < pairs; ++k )
    {
      cur = pixels[2 * k] + unfold[pixels[2 * k + 1]];
      pixels[2 * k + 1] = cur;
      pixels[2 * k + 2] = cur + (unfold[pixels[2 * k + 2]]);
      p = &pixels[2 * k + 3];
    }
    done = 2 * k + 1;
  }
  else
  {
    done = 1;
  }
  if ( row_rest > done - 1 )
  {
    pixels[done] = pixels[done - 1] + unfold[pixels[done]];
    p = &pixels[done + 1];
    rows_left = height - 1;
    if ( height == 1 )
      return p;
    goto LABEL_29;
  }
  rows_left = height - 1;
  if ( height != 1 )
  {
LABEL_29:
    up = &p[-width];
    do
    {
      // The first column of a row: north is the only neighbour.  For a plane
      // one pixel wide that is every pixel, which is what the loop is for --
      // it spins here and never reaches the MED tree below.  For any other
      // width it runs exactly once and breaks.
      while ( 1 )
      {
        ++up;
        *p = unfold[(uint8_t)*p] + p[-width];
        x_left = row_rest;
        ++p;
        if ( width != 1 )
          break;
        if ( !--rows_left )
          return p;
      }
      do
      {
        pred = (uint8_t)*(p - 1);
        north = (uint8_t)*up;
        northwest = (uint8_t)p[-width - 1];
        if ( pred < north )
        {
          if ( northwest < pred )
          {
            LOBYTE(pred) = *up;
            goto LABEL_41;
          }
          if ( northwest <= north )
            LOBYTE(pred) = (uint8_t)(north + pred - northwest);
        }
        else
        {
          if ( northwest > pred )
          {
            LOBYTE(pred) = *up;
            goto LABEL_41;
          }
          // The `goto` here entered the branch above to reach this one
          // statement; both arms then fall to `LABEL_41`, so a copy says the
          // same thing.  `pred + north - northwest` against the two bounds is the
          // median of three, which is what `predict_med` is named for.
          if ( northwest >= north )
            LOBYTE(pred) = (uint8_t)(north + pred - northwest);
        }
LABEL_41:
        *p = pred + unfold[(uint8_t)*p];
        ++up;
        ++p;
        --x_left;
      }
      while ( x_left );
      --rows_left;
    }
    while ( rows_left );
  }
  return p;
}

// The zeroth-order cost of a histogram, in bits: `total*log(total) - sum(n*log n)`
// over ln 2.  `a1` is `n2` int32 bins.
//
// Two running pairs rather than one running total, because that is what the
// SSE original had: even bins accumulated in lane 0, odd bins in lane 1, and
// the two added at the end.  Double addition is not associative, so folding
// them into one accumulator would be a different number.
int32_t __estimate_cost(uint8_t *a1, int32_t n2)
{
  const int32_t *bin = (const int32_t *)a1;
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  double total, entropy;
  int32_t i;

  for ( i = 0; i + 1 < n2; i += 2 )
  {
    // An empty bin contributes nothing.  The original masked it off rather
    // than branching, and took the logarithm anyway -- of 0.5, so it could not
    // raise anything -- which is the same arithmetic with the same result.
    if ( bin[i] )
    {
      sum_even += (double)bin[i];
      ent_even += (double)bin[i] * log((double)bin[i]);
    }
    if ( bin[i + 1] )
    {
      sum_odd += (double)bin[i + 1];
      ent_odd += (double)bin[i + 1] * log((double)bin[i + 1]);
    }
  }
  entropy = ent_even + ent_odd;
  total   = sum_even + sum_odd;

  // The odd bin at the end, if there is one.  No call has ever reached this:
  // every one of the fifteen passes 512 or 1024 (REFACTORING2.md §6), so it is
  // the one part of this function the gate does not cover.
  for ( ; i < n2; i++ )
    if ( bin[i] )
    {
      total   += (double)bin[i];
      entropy += (double)bin[i] * log((double)bin[i]);
    }

  if ( total != 0.0 )
    total = total * log(total);
  return (int32_t)((total - entropy) * 1.442695040888963);
}

void ** __alt_model_p1_d8_decode(int8_t ArgList, uint8_t *Src, int32_t i, int32_t a4)
{
  P1Ctx *buf1, *v11, *cursor0, *v7, *buf3, *buf2;
  ;
  AltP1Block *v5;
  AltP1Block *v4;
  int32_t v6, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26,
          v27, v28, v30, v31, v32, v35, v36, v39, v41;
  int64_t v37;
  uint8_t *cursor2, *cursor4, *Src_1;
  v4 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
  if ( v4 )
    v5 = (AltP1Block *)(__alt_p1_alloc((AltP1Block *)v4, i, a4, 0));
  else
    v5 = (AltP1Block *)(nullptr);
  __rc_begin_decode(ArgList);
  if ( v5->height > 0 )
  {
    v6 = 0;
    do
    {
      ++v6;
      // The row end mirrored into the right margin: records 0..5 take -1..-6.
      // Two bytes each, which is what the twelve even offsets were.
      {
        P1Ctx *const here = (P1Ctx *)v5->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      v7 = v5->buf[4];
      buf3 = v5->buf[3];
      buf2 = v5->buf[2];
      buf1 = v5->buf[1];
      v11 = v5->buf[0];
      v5->buf[4] = buf3;
      v5->buf[3] = buf2;
      v5->buf[2] = buf1;
      v5->buf[1] = v11;
      v5->buf[0] = v7;
      v7 += 4;
      v5->cursor[0] = v7;
      v11 += 4;
      v5->cursor[1] = v11;
      v5->cursor[2] = buf1 + 4;
      v5->cursor[3] = buf2 + 4;
      v5->cursor[4] = buf3 + 4;
      ((P1Ctx *)v7)[-4] = ((P1Ctx *)v11)[3];
      // And the rest of the new row's left margin, from the row above.
      {
        P1Ctx *const here = (P1Ctx *)v5->cursor[0];
        P1Ctx *const up   = (P1Ctx *)v5->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = (uint8_t *)v5->cursor[2];
      cursor4 = (uint8_t *)v5->cursor[4];
      v5->ctx[2] = 0;
      v5->ctx[3] = 0;
      v5->ctx[4] = 0;
      v14 = *(cursor2 - 3);
      v5->ctx[3] = v14;
      v15 = *(cursor2 - 1);
      v5->ctx[4] = v15;
      v16 = *(cursor4 - 3) + v14;
      v5->ctx[3] = v16;
      v17 = *(cursor4 - 1) + v15;
      v5->ctx[4] = v17;
      v18 = cursor2[1] + v16;
      v5->ctx[3] = v18;
      v19 = cursor2[3] + v17;
      v5->ctx[4] = v19;
      v20 = cursor4[1] + v18;
      v5->ctx[3] = v20;
      v21 = cursor4[3] + v19;
      v5->ctx[4] = v21;
      v22 = cursor2[5] + v20;
      v5->ctx[3] = v22;
      v23 = cursor2[7] + v21;
      v5->ctx[4] = v23;
      v24 = cursor4[5] + v22;
      v5->ctx[3] = v24;
      v25 = cursor4[7] + v23;
      v5->ctx[4] = v25;
      v26 = cursor2[9] + v24;
      v5->ctx[3] = v26;
      v27 = cursor2[11] + v25;
      v5->ctx[4] = v27;
      v28 = cursor4[9] + v26;
      cursor0 = v5->cursor[0];
      v5->ctx[3] = v28;
      v30 = cursor4[11] + v27;
      v5->ctx[4] = v30;
      v31 = cursor0[-4].mag + v28;
      v5->ctx[3] = v31;
      v32 = cursor0[-3].mag + v30;
      v5->ctx[4] = v32;
      v5->ctx[3] = cursor0[-2].mag + v31;
      v5->ctx[4] = cursor0[-1].mag + v32;
      if ( !(v5->width <= 0) )
      {
        Src_1 = Src;
        v39 = v6;
        v41 = 0;
        do
        {
          ++v41;
          __alt_p1_context((AltP1Block *)(uint32_t *)v5, (AltP1Block *)nullptr, (AltP1Block *)0);
          v36 = (uint8_t)((uint8_t)v5->pred
                                + *((uint8_t *)v5 + (uint8_t)__alt_p1_decode_symbol((uint16_t *)&((int32_t *)v5)[4 * v5->ctx[0] + 950], v35, v5->ctx[1]) + 1496));
          *Src_1 = v36;
          v37 = v36 - v5->pred;
          // The record's two writers, one line apart: the reconstructed
          // sample, then the size of the prediction error.
          v5->cursor[0]->sym = v36;
          v5->cursor[0]->mag = (BYTE4(v37) ^ v37) - BYTE4(v37);
          v5->ctx[3 + v5->ctx[2]] = v5->ctx[3 + v5->ctx[2]]
                        + v5->cursor[0]->mag
                        - v5->cursor[0][-4].mag
                        - (v5->cursor[4][-2].mag
                         - v5->cursor[4][6].mag
                         + v5->cursor[2][-2].mag
                         - v5->cursor[2][6].mag);
            v5->ctx[2] = v5->ctx[2] == 0;
          if ( v5->counters[v5->ctx[0]].total < 0x4000u )
            __alt_p1_model((AltP1Block *)v5);
          ++v5->cursor[0];
          ++Src_1;
          ++v5->cursor[1];
          ++v5->cursor[2];
          ++v5->cursor[3];
          ++v5->cursor[4];
        }
        while ( v41 < v5->width );
        v6 = v39;
        Src = Src_1;
      }
    }
    while ( v6 < v5->height );
  }
  __rc_end_decode();
  return __alt_p1_free((void **)v5, 1);
}


int32_t __alt_model_p1_decode(uint16_t *p_i, uint8_t *Src) {   P1Ctx *v25,
        *buf3, *buf2, *buf1, *v29, *cursor0;
  // Phase 2 split this frame into plain locals, which is what the other eight
  // frames took -- and it is wrong here.  The frame was 116 bytes and its
  // aliases only reach offset 84; the code writes into the 32 bytes of slack
  // past the end, and once each local has its own storage those writes land on
  // whatever the compiler put next.  At -O2 that is a null pointer handed to
  // alt_p1_context; at -O0 it is a plane that decodes to the wrong pixels.
  //
  // Nothing caught it because nothing reached it: this is the body
  // REFACTORING.md section 2.3 lists as unexercised.  testfiles/altp1.bmp
  // reaches it now.
  void *v84;
  uint32_t v86;
  uint32_t v87;
  int32_t v88;
  int32_t v89;
  uint8_t *Src_1;
  int32_t v92;
  void * Block_plane[4];
  AltP1Block * &v94 = (AltP1Block * &)Block_plane[1];
  AltP1Block * &v95 = (AltP1Block * &)Block_plane[2];
  AltP1Block * &v96 = (AltP1Block * &)Block_plane[3];
  int32_t v97;
  uint32_t i_4;
  int32_t v104;
  ;
  AltP1Block *v59;
  uintptr_t v61;
  AltP1Block *v24;
  AltP1Block *v6;
  int32_t i, v3, n4, *v7, v8, v9, v10, v14, v15, v16, ArgList, v18, i_3, n4_1,
          n4_2, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43,
          v44, v45, v46, v48, v49, v50, v52, v53, v54, v56, v57, v60,
          v62, v64, v67, v68, v71, v72, v74, v75, v78, v79, n4_3, n4_4;
  uint32_t v20, *v51;
  AltP1Block *v66;
  AltP1Block *v73;
  uint8_t *v30;
  uint8_t *v31;
  uint8_t *v69, *v76;
  void **v82;
  i = *p_i;
  v3 = p_i[1];
  if ( plane_count > 0 )
  {
    n4 = 0;
    do
    {
      v6 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __alt_p1_alloc((AltP1Block *)v6, i, v3, n4);
      else
        v7 = nullptr;
      Block_plane[n4++] = v7;
    }
    while ( n4 < plane_count );
  }
  // Records 2, 3 and 4 name a source plane each; each of the three is then
  // read for its flags and its b3.  Record indices now -- they were 16 times
  // these, the byte offsets into the table.
  v8 = plane_desc[2].src_plane;
  v9 = plane_desc[3].src_plane;
  v10 = plane_desc[4].src_plane;
  v14 = plane_desc[v9 + 1].b3;
  v15 = plane_desc[v10 + 1].b3;
  v97 = plane_desc[v8 + 1].b3;
  v16 = plane_desc[v8 + 1].flags & 8;
  ArgList = plane_desc[v9 + 1].flags & 8;
  v18 = plane_desc[v10 + 1].flags & 8;
  __rc_begin_decode(ArgList);
  if ( v3 > 0 )
  {
    v20 = 0;
    i_3 = i;
    n4_1 = plane_count;
    do
    {
      if ( n4_1 > 0 )
      {
        v86 = v20;
        n4_2 = 0;
        v88 = v3;
        Src_1 = Src;
        do
        {
          ++n4_2;
          // `&v92 + n` is `Block_plane[n - 1]`: v92 is the member
          // before the array, and this loop pre-increments from 0.
          v24 = (AltP1Block *)Block_plane[n4_2 - 1];
          // The row end mirrored into the right margin: records 0..5 take -1..-6.
          // Two bytes each, which is what the twelve even offsets were.
          {
            P1Ctx *const here = (P1Ctx *)v24->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          v25 = v24->buf[4];
          buf3 = v24->buf[3];
          buf2 = v24->buf[2];
          buf1 = v24->buf[1];
          v29 = v24->buf[0];
          v24->buf[4] = buf3;
          v24->buf[3] = buf2;
          v24->buf[2] = buf1;
          v24->buf[1] = v29;
          v24->buf[0] = v25;
          v25 += 4;
          v24->cursor[0] = v25;
          v29 += 4;
          v24->cursor[1] = v29;
          v24->cursor[2] = buf1 + 4;
          v24->cursor[3] = buf2 + 4;
          v24->cursor[4] = buf3 + 4;
          ((P1Ctx *)v25)[-4] = ((P1Ctx *)v29)[3];
          // And the rest of the new row's left margin, from the row above.
          {
            P1Ctx *const here = (P1Ctx *)v24->cursor[0];
            P1Ctx *const up   = (P1Ctx *)v24->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          v30 = (uint8_t *)(*&v24->cursor[2]);
          v31 = (uint8_t *)(*&v24->cursor[4]);
          v24->ctx[2] = 0;
          v24->ctx[3] = 0;
          v24->ctx[4] = 0;
          v32 = *((int8_t *)v30 - 3);
          v24->ctx[3] = v32;
          v33 = *((int8_t *)v30 - 1);
          v24->ctx[4] = v33;
          v34 = *((int8_t *)v31 - 3) + v32;
          v24->ctx[3] = v34;
          v35 = *((int8_t *)v31 - 1) + v33;
          v24->ctx[4] = v35;
          v36 = v30[1] + v34;
          v24->ctx[3] = v36;
          v37 = v30[3] + v35;
          v24->ctx[4] = v37;
          v38 = v31[1] + v36;
          v24->ctx[3] = v38;
          v39 = v31[3] + v37;
          v24->ctx[4] = v39;
          v40 = v30[5] + v38;
          v24->ctx[3] = v40;
          v41 = v30[7] + v39;
          v24->ctx[4] = v41;
          v42 = v31[5] + v40;
          v24->ctx[3] = v42;
          v43 = v31[7] + v41;
          v24->ctx[4] = v43;
          v44 = v30[9] + v42;
          v24->ctx[3] = v44;
          v45 = v30[11] + v43;
          v24->ctx[4] = v45;
          v46 = v31[9] + v44;
          cursor0 = v24->cursor[0];
          v24->ctx[3] = v46;
          v48 = v31[11] + v45;
          v24->ctx[4] = v48;
          v49 = cursor0[-4].mag + v46;
          v24->ctx[3] = v49;
          v50 = cursor0[-3].mag + v48;
          v24->ctx[4] = v50;
          v24->ctx[3] = cursor0[-2].mag + v49;
          v24->ctx[4] = cursor0[-1].mag + v50;
          n4_1 = plane_count;
        }
        while ( n4_2 < plane_count );
        v20 = v86;
        v3 = v88;
        i_3 = i;
        Src = Src_1;
      }
      if ( i_3 > 0 )
      {
        v87 = v20;
        i_4 = 0;
        v89 = v3;
        do
        {
          v51 = (uint32_t *)Block_plane[0];
          __alt_p1_context((AltP1Block *)(uint8_t **)Block_plane[0], (AltP1Block *)nullptr, (AltP1Block *)0);
          v53 = __alt_p1_decode_symbol((uint16_t *)&v51[4 * v51[3] + 950], v52, v51[4]);
          v54 = v51[2];
          AltP1Block *const blk = (AltP1Block *)v51;
          v56 = (uint8_t)(v54 + blk->unfold[v53]);
          // The row record is two bytes: the reconstructed sample at +0 and the
          // size of the prediction error at +1.  `[1]` is this pixel's error,
          // `[-7]` is the one four pixels back, and the four terms off
          // `cursor[2]` and `cursor[4]` are two records back and six forward on
          // the other two planes.
          blk->cursor[0]->sym = v56;
          blk->cursor[0]->mag = abs32(v56 - v54);
          v51[v51[5] + 6] = v51[v51[5] + 6]
                          + blk->cursor[0]->mag
                          - blk->cursor[0][-4].mag
                          - (blk->cursor[4][-2].mag
                           - blk->cursor[4][6].mag
                           + blk->cursor[2][-2].mag
                           - blk->cursor[2][6].mag);
          v57 = 4 * v51[3];
          v51[5] = v51[5] == 0;
          if ( LOWORD(v51[v57 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v51);
          v51[49] += 2;
          v51[50] += 2;
          v51[51] += 2;
          v51[52] += 2;
          v51[53] += 2;
          v59 = (AltP1Block *)v94;
          *(plane_desc[1].src_plane + Src) = v56;
          __alt_p1_context((AltP1Block *)(uint8_t **)v59, (AltP1Block *)Block_plane[0], (AltP1Block *)0);
          v61 = __alt_p1_decode_symbol(&v59->counters[v59->ctx[0]].total, v60, v59->ctx[1]);
          v62 = *(uint32_t *)&v59->pred;
          v64 = (uint8_t)(v62 + v59->unfold[v61]);
          v104 = v64;
          v59->cursor[0]->sym = v64;
          v59->cursor[0]->mag = abs32(v64 - v62);
          v59->ctx[3 + v59->ctx[2]] = v59->ctx[3 + v59->ctx[2]]
                                                            + v59->cursor[0]->mag
                                                            - v59->cursor[0][-4].mag
                                                            - (v59->cursor[4][-2].mag
                                                             - v59->cursor[4][6].mag
                                                             + v59->cursor[2][-2].mag
                                                             - v59->cursor[2][6].mag);
          v59->ctx[2] = v59->ctx[2] == 0;
          if ( v59->counters[v59->ctx[0]].total < 0x4000u )
            __alt_p1_model(v59);
          ++v59->cursor[0];
          ++v59->cursor[1];
          ++v59->cursor[2];
          ++v59->cursor[3];
          ++v59->cursor[4];
          if ( v16 )
            v104 += v97 + *(plane_desc[1].src_plane + Src);
          v66 = (AltP1Block *)(v95);
          v84 = Block_plane[0];
          *(Src + plane_desc[2].src_plane) = v104;
          __alt_p1_context((AltP1Block *)v66, (AltP1Block *)v94, (AltP1Block *)(int32_t)v84);
          v68 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)v66)[4 * v66->ctx[0] + 950], v67, (int32_t)v66->ctx[1]);
          v69 = (uint8_t *)(v66->pred);
          v71 = (uint8_t)((uint8_t)(uintptr_t)v69 + v66->unfold[v68]);
          v66->cursor[0]->sym = v71;
          v66->cursor[0]->mag = abs32(v71 - (uint32_t)v69);
          ((uint8_t**)v66)[v66->ctx[2] + 6] = &((uint8_t**)v66)[v66->ctx[2] + 6][v66->cursor[0]->mag
                                                           - v66->cursor[0][-4].mag
                                                           - (v66->cursor[4][-2].mag
                                                            - v66->cursor[4][6].mag)
                                                           - (v66->cursor[2][-2].mag
                                                            - v66->cursor[2][6].mag)];
          v72 = 4 * v66->ctx[0];
          v66->ctx[2] = v66->ctx[2] == 0;
          if ( LOWORD(((uint8_t**)v66)[v72 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v66);
          ++v66->cursor[0];
          ++v66->cursor[1];
          ++v66->cursor[2];
          ++v66->cursor[3];
          ++v66->cursor[4];
          if ( ArgList )
            *(plane_desc[3].src_plane + Src) = ((plane_desc[plane_desc[3].src_plane + 1].w4
                                                                * *(plane_desc[1].src_plane
                                                                                     + Src)
                                                                + plane_desc[plane_desc[3].src_plane + 1].w8
                                                                * (uint32_t)*(plane_desc[2].src_plane + Src)
                                                                + 40) >> 7)
                                                              + v14
                                                              + v71;
          else
            *(plane_desc[3].src_plane + Src) = v71;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            v73 = (AltP1Block *)(v96);
            __alt_p1_context((AltP1Block *)v96, (AltP1Block *)v95, (AltP1Block *)(int32_t)v94);
            v75 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)v73)[4 * v73->ctx[0] + 950], v74, (int32_t)v73->ctx[1]);
            v76 = (uint8_t *)(v73->pred);
            v78 = (uint8_t)((uint8_t)(uintptr_t)v76 + v73->unfold[v75]);
            v92 = v78;
            v73->cursor[0]->sym = v78;
            v73->cursor[0]->mag = abs32(v78 - (uint32_t)v76);
            ((uint8_t**)v73)[v73->ctx[2] + 6] = &((uint8_t**)v73)[v73->ctx[2] + 6][v73->cursor[0]->mag
                                                             - v73->cursor[0][-4].mag
                                                             - (v73->cursor[4][-2].mag
                                                              - v73->cursor[4][6].mag)
                                                             - (v73->cursor[2][-2].mag
                                                              - v73->cursor[2][6].mag)];
            v79 = 4 * v73->ctx[0];
            v73->ctx[2] = v73->ctx[2] == 0;
            if ( LOWORD(((uint8_t**)v73)[v79 + 950]) < 0x4000u )
              __alt_p1_model((AltP1Block *)v73);
            ++v73->cursor[0];
            ++v73->cursor[1];
            ++v73->cursor[2];
            ++v73->cursor[3];
            ++v73->cursor[4];
            if ( v18 )
              v92 += ((plane_desc[plane_desc[4].src_plane + 1].w8 * *(plane_desc[4].src_plane + Src - 2)
                     + plane_desc[plane_desc[4].src_plane + 1].w4 * *(plane_desc[4].src_plane + Src - 3)
                     + plane_desc[plane_desc[4].src_plane + 1].w12 * *(plane_desc[4].src_plane + Src - 1)
                     + 64) >> 7)
                   + v15;
            *(plane_desc[4].src_plane + Src) = v92;
            n4_1 = plane_count;
          }
          Src += n4_1;
          ++i_4;
        }
        while ( i_4 < i );
        v20 = v87;
        v3 = v89;
        i_3 = i;
      }
      ++v20;
    }
    while ( v20 < v3 );
  }
  __rc_end_decode();
  n4_3 = plane_count;
  if ( plane_count > 0 )
  {
    n4_4 = 0;
    do
    {
      v82 = (void **)Block_plane[n4_4];
      if ( v82 )
      {
        __alt_p1_free((void **)v82, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}

// The fixed-coefficient weighted sum `alt_p2_context` calls, and its only
// caller.  Sums `a2`'s rows against the constants at 0x441120 onward, keeps the
// result in `a2->f112[1]` and returns it, and reloads `_this` from `a3` scaled
// by two more constants.  Float throughout, and it codes nothing.
// The horizontal sum the SSE original spelled `x + movehl(x, x)`, which pairs
// lane 0 with 2 and 1 with 3, then one more add.  The order is load-bearing:
// float addition is not associative, and summing 0+1+2+3 instead moves three
// of the fifteen reference streams (REFACTORING2.md §2.3, measured).
static inline float bmf_hsum4(const float x[4])
{
  return (x[0] + x[2]) + (x[1] + x[3]);
}

// The six mixing weights, one row per plane index.  BMF.exe built this table
// at startup by copying broadcast xmmwords into 0x445760..0x4458DF -- four
// identical lanes each, so one float per weight says the same thing -- and the
// copying is gone with them.  Every row very nearly sums to one, which is what
// these are: a mixture.
static const float bmf_p2_mix[4][6] = {
  { 0.4f,  0.1f,  0.15f, 0.23f, 0.04f, 0.03f },
  { 0.26f, 0.23f, 0.13f, 0.12f, 0.15f, 0.12f },
  { 0.27f, 0.22f, 0.26f, 0.08f, 0.12f, 0.05f },
  { 0.3f,  0.14f, 0.08f, 0.22f, 0.18f, 0.09f },
};

static const float bmf_p2_decay = 0.78f;   // 0x439B20
static const float bmf_p2_seed  = 0.19f;   // 0x439B30

int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], CtxWeights *a3, int32_t n2)
{
  const float *mix = bmf_p2_mix[n2];
  float acc[4], mixed[7][4], centre, prediction, own;
  int32_t i, j, k;

  // One number for the whole of `a2`: its seven rows against the seven
  // coefficient rows, then a horizontal sum of what is left.
  for ( k = 0; k < 4; k++ )
  {
    acc[k] = bmf_p2_coef[0][k] * a2[0][k];
    for ( j = 1; j < 7; j++ )
      acc[k] += bmf_p2_coef[j][k] * a2[j][k];
  }
  centre = bmf_hsum4(acc);
  a2[7][0] = centre;

  // Every row is then recentred on it, in place -- this is the only thing the
  // function writes back into `a2[0..6]`.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
      a2[j][k] -= centre;

  // The six sub-models mixed with this plane count's weights, row by row.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
    {
      mixed[j][k] = mix[0] * a3->f0[0][j][k];
      for ( i = 1; i < 6; i++ )
        mixed[j][k] += mix[i] * a3->f0[i][j][k];
    }

  // The mixture against the recentred rows, summed and put back on the centre.
  for ( k = 0; k < 4; k++ )
  {
    acc[k] = mixed[0][k] * a2[0][k];
    for ( j = 1; j < 7; j++ )
      acc[k] += mixed[j][k] * a2[j][k];
  }
  prediction = bmf_hsum4(acc) + centre;
  a2[7][2] = prediction;

  if ( *(uint32_t *)&_this[15][0] )
  {
    // `_this` holds a second set of weights, kept from the previous call, and
    // the answer is the two predictions blended 47:169.2.
    for ( k = 0; k < 4; k++ )
    {
      acc[k] = _this[0][k] * a2[0][k];
      for ( j = 1; j < 7; j++ )
        acc[k] += _this[j][k] * a2[j][k];
    }
    own = centre + bmf_hsum4(acc);
    a2[7][1] = own;
    return (int32_t)(prediction
                     + ((own - prediction) * _this[14][0])
                         / _this[14][1]);
  }

  // First call against this `_this`: seed it from what was just computed and
  // answer with the mixture alone.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
    {
      _this[j][k]     = mixed[j][k] * bmf_p2_decay;
      _this[j + 7][k] = a3->f0[0][j + 7][k] * bmf_p2_seed;
    }
  _this[14][0] = 47.0f;
  _this[14][1] = 169.2f;
  _this[14][2] = 1.0f;
  a2[7][1] = prediction;
  return (int32_t)prediction;
}

// `alt_p1_context`'s opposite number for p2, and the largest body in the file
// at a thousand lines.  Same evidence for the name: no `rc.` call in it, and
// all four p2 bodies call it from both sides.  It ends by folding the
// neighbourhood sums into three context words and returning the 0..255 index
// they are read with.  What the individual terms *mean* is ALGORITHM.md §9's
// question, and it is still open -- this names the role, not the algorithm.
int32_t __alt_p2_context(AltP2Block *a1, AltP2Block *a4, AltP2Block *a5) {   P2Ctx *v55,
        *v6;
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and altp1 segfaults while compressing.
  struct alignas(16) AltP2ContextFrame {   // 208 bytes, one stack frame
      P2Ctx *v246;   // `cursor[0]`, the current row
      int32_t v256;
      int16_t *v268;
      // `alt_p2_filter`'s six sub-model weight blocks, which is what `CtxWeights`
      // says and the only thing the six slots hold by the time it is called.
      // The scratch the body kept here first is six locals now.
      union {
          void *sub[6];
          struct {
            float (*sub0)[4];
            float (*sub1)[4];
            float (*sub2)[4];
            float (*sub3)[4];
            float (*sub4)[4];
            float (*sub5)[4];
          };
      };
      uint8_t   _gap0[4];   // was int16_t * v281
      uint8_t   _gap1[4];   // was int16_t * v282
      uint8_t   _gap2[4];   // was int16_t * v283
      uint8_t   _gap3[4];   // was int16_t * v284
      uint8_t   _gap4[4];   // was int16_t * v285
      uint8_t   _gap5[4];   // was int16_t * v286
      uint8_t   _gap6[4];   // was int32_t n1840_2
      uint8_t   _gap7[4];   // was int32_t n1840_1
      uint8_t   _gap8[4];   // was AltP2Block * v289
      uint8_t   _gap9[4];   // was int32_t v290
      uint8_t   _gap10[4];   // was int32_t n3536
      uint8_t   _gap11[4];   // was int32_t v292
      uint8_t   _gap12[4];   // was P2Ctx *v293, now a `P2Ctx *`
      uint8_t   _gap13[4];   // was int16_t * v294
      uint8_t   _gap14[4];   // was int16_t * v295
      uint8_t   _gap15[4];   // was int16_t * v296
      uint8_t   _gap16[4];   // was uint32_t v297
      uint8_t   _gap17[4];   // was uint32_t v298
      uint8_t   _gap18[4];   // was int32_t n960_1
      uint8_t   _gap19[4];   // was int32_t n1840
      uint8_t   _gap20[4];   // was int32_t v301
      uint8_t   _gap21[4];   // was int32_t v302
      uint8_t   _gap22[4];   // was int32_t v303
      uint8_t   _gap23[4];   // was int32_t v304
      uint8_t   _gap24[4];   // was int32_t v305
      uint8_t   _gap25[4];   // was int32_t v306
      uint8_t   _gap26[4];   // was int32_t v307
      uint8_t   _gap27[4];   // was int32_t v308
      uint8_t   _gap28[4];   // was int32_t v309
      uint8_t   _gap29[4];   // was int32_t v310
      uint8_t   _gap30[4];   // was int32_t v311
      uint8_t   _gap31[4];   // was int32_t v312
      uint8_t   _gap32[4];   // was int32_t v313
      uint8_t   _gap33[4];   // was int32_t v314
      uint8_t   _gap34[4];   // was int32_t v315
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 208, "frame layout moved");
  // These shared `__frame.v246` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `__frame.v246` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v248;
  int32_t v250;
  int32_t v251;
  int32_t n15;
  int32_t v254;
  // These shared `__frame.v246` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  P2Ctx *v255;   // row cursors into the neighbourhood table
  // These shared `__frame.v256` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *v257;
  // These shared `__frame.v256` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  P2Ctx *v258;
  int32_t v259;
  int32_t v260;
  int32_t v264;
  int32_t v266;
  int32_t v267;
  // These shared `__frame.v268` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v269;
  // These shared `__frame.v268` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v270;
  int32_t v272;
  int32_t v273;
  // These shared `__frame.v268` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  P2Ctx *v274;   // row cursors into the neighbourhood table
  P2Ctx *v281;
  P2Ctx *v282;
  P2Ctx *v283;
  P2Ctx *v284;
  P2Ctx *v285;
  P2Ctx *v286;
  int32_t n1840_2;
  int32_t n1840_1;
  AltP2Block *v289;
  // The six spill slots before `alt_p2_filter`'s six weight pointers are
  // loaded into them: a row cursor, a neighbour, a threshold-row index and
  // three counts.  Both lifetimes were `__frame.sub[0..5]`.
  P2Ctx *sub0_row;
  P2Ctx   *sub1_nb;
  int32_t  sub2_n, sub3_row, sub4_n, sub5_n;
  int32_t v290;
  int32_t n3536;
  P2Ctx *v109;   // the p2 row cursor, `cursor[0]`
  int32_t v292_cost;   // the first of two lifetimes MSVC gave one slot
  // The p2 row cursor and its copies.  Every dereference is a lane of the
  // 18-byte record or the high byte of one, so the byte offsets divide into
  // a record and a lane -- `-37` is record -3, lane 8, high byte.
  // A `P2Ctx *`: Hex-Rays typed it `int16_t (*)[8]`, a sixteen-byte
  // stride over eighteen-byte records, so its subscripts were out of
  // phase with the grid.  All 48 reaches through it land on a field
  // boundary once the byte offsets are decoded against the real size,
  // which is what says the stride was the artefact and not the table.
  P2Ctx *v293;
  P2Ctx *v294;
  P2Ctx *v295;   // row cursors into the neighbourhood table
  P2Ctx *v296;
  uint32_t v297;
  uint32_t v298;
  int32_t n960_1;
  int32_t n1840;
  int32_t v301;
  int32_t v302;
  int32_t v303;
  int32_t v306;
  int32_t v307;
  int32_t v312;
  int32_t v313;
  int32_t v314;
  ;
  // `cursor[0]` again: 45 reaches, all of them records -6 .. 0 at
  // lanes 0 and 1, which is the row this pass is writing.
  P2Ctx *v46;
  float (*v31)[4];
  AltP2Block *v196;
  AltP2Block *v28;
  AltP2Block *v166;
  AltP2Block *v118;
  AltP2Block *v184;
  // Row cursors: sixteen bytes a step over an int16 plane.  Hex-Rays wrote
  // the stride as a 16-byte pointer and every read as a lane of it; eight
  // int16 is the same sixteen bytes and says which element.  Twelve of the
  // 72 reads take a byte at an odd offset -- the high half of an element --
  // and those keep saying it in bytes.
  // All eight were `int16_t (*)[8]`, a sixteen-byte stride over eighteen-byte
  // records.  Every reach through them decodes onto a field boundary, which is
  // what says the stride was Hex-Rays' and not the table's.
  // `cursor[1]`, under one name where Hex-Rays had six.
  // `cursor[3]`, under one name where Hex-Rays had four -- `sub0_row`
  // is the third of them, and keeps only its first lifetime as
  // `cursor[2]` above.
  P2Ctx *v50;
  bool v26, v58;
  float v70, v244;
  P2Ctx *v73;
  P2Ctx *v71;
  P2Ctx *v87;
  P2Ctx *v230;
  P2Ctx *v97;
  P2Ctx *v93;
  P2Ctx *v98;
  P2Ctx *v217;
  P2Ctx *v76;
  P2Ctx *v86;
  P2Ctx *v95;
  P2Ctx *v92;
  P2Ctx *v81;
  P2Ctx *v69;
  P2Ctx *v170;
  P2Ctx *v80;
  P2Ctx *v83;
  P2Ctx *v72;
  P2Ctx *v96;
  P2Ctx *v49;
  P2Ctx *v52;
  P2Ctx *v78;
  P2Ctx *v129;
  P2Ctx *v66;
  int16_t *v90, *v99;
  P2Ctx *cursor1, *v45, *v67;
  int32_t *v104;
  int32_t v12, lane5, v14, v15, v16, v17, v18, v19, v21, v25, nb_id, v30, v65,
          v68, v74, v75, v85, v88, *cur, v105, n2, v107, n3536_5, lane3, v112,
          v113, v114, v116, v117, n3536_1, v121, v122, lane2, v124, v125, v126,
          v127, v128, n2256, v132, v134, v137, v138, v140, v141, v144, n2576,
          n1840_13, v147, v148, v149, v151, v154, v155, v156, n2896, v161,
          v162, v163, v164, v167, v168, v171, n3536_2, v175, v176, v177, v178,
          v179, v181, v182, n3536_3, v185, v186, v188, v189, v190, v191, v192,
          v193, n1840_14, n1840_15, v198, n1840_16, v200, n1840_17, v203,
          v206, v207, v208, n960, n3536_4, mag, v212, v213, v214, v215,
          n1840_3, n1840_8, n1840_7, n1840_10, n1840_9, v222, v224, n1840_11,
          n1840_12, n255, v228, n1840_5, n1840_4, n1840_6, v234, v235, v236,
          v237, v238, v239, v240, v241, v242;
  int8_t v139;
  uint32_t v120, v133, v135, v136, v152, v153, v165, v180;
  uint8_t *v187;
  v6 = a1->cursor[0];
  cursor1 = a1->cursor[1];
  v289 = (AltP2Block *)(a1);
  __frame.v246 = v6;
  __frame.v268 = (int16_t *)cursor1;
  sub0_row = (P2Ctx *)a1->cursor[2];
  n1840_1 = 21 * sub0_row[-1].lane[6]
          + 12 * cursor1[3].lane[6]
          + 16 * cursor1[2].lane[6]
          + 22 * cursor1[1].lane[6]
          + (23 * v6[-1].lane[6])
          + 20 * cursor1->lane[6]
          + ctx_bias[0]
          + 14 * v6[-2].lane[6];
  n1840_2 = 17 * sub0_row[-2].lane[7]
          + 21 * cursor1[2].lane[7]
          + 15 * cursor1[1].lane[7]
          + 25 * cursor1->lane[7]
          + 9 * cursor1[-1].lane[7]
          + 22 * v6[-1].lane[7]
          + ctx_bias[1]
          + 19 * v6[-2].lane[7];
  v12 = 17 * cursor1[3].lane[4]
      + 15 * cursor1[2].lane[4]
      + 21 * cursor1[1].lane[4]
      + 18 * cursor1->lane[4]
      + 16 * cursor1[-1].lane[4]
      + 22 * v6[-1].lane[4]
      + ctx_bias[2]
      + 19 * v6[-2].lane[4];
  lane5 = sub0_row->lane[5];
  v14 = v289->cursor[3]->lane[5];
  v15 = v6[-2].lane[5];
  v16 = v6[-1].lane[5];
  v292_cost = v12;
  n3536 = 14 * __frame.v268[32] + 23 * __frame.v268[14] + 19 * __frame.v268[5] + 25 * v16 + ctx_bias[3] + 17 * v15 + 15 * (v14 + lane5);
  v17 = sub0_row[-1].lane[0];
  v18 = sub0_row[2].lane[0] + sub0_row->lane[0];
  v290 = v12 + n3536 + n1840_1 + n1840_2;
  v19 = __frame.v268[9];
  v21 = __frame.v246[-1].lane[0];
  sub1_nb = (P2Ctx *)(2 * __frame.v246[-2].lane[0] + 2 * v21);
  sub2_n = v19 + (*__frame.v268 + 2 * v21);
  sub4_n = 16 * (v18 + v19 + v17);
  // Which row of the threshold table: how many of five ratios the coded
  // length has passed.  This was `13 * <the same sum>` used as a flat
  // subscript, with the sum itself recomputed two statements later.
  sub3_row = (8 * v12 > 43 * n3536)
       + (8 * v12 > 17 * n3536)
       + (8 * v12 > 9 * n3536)
       + (8 * v12 > 5 * n3536)
       + (8 * v12 > 2 * n3536);
  v25 = v290 > bmf_p2_thresholds[sub3_row][9];
  v26 = v290 <= bmf_p2_thresholds[sub3_row][10];
  __frame.v256 = ((v290 > bmf_p2_thresholds[sub3_row][12]) + (v290 > bmf_p2_thresholds[sub3_row][11]) + !v26 + v25) << 6;
  sub5_n = (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[sub3_row][5])
       + (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[sub3_row][4])
       + (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[sub3_row][3]);
  v281 = (P2Ctx *)((int16_t *)(16 * ((sub2_n > bmf_p2_thresholds[sub3_row][8]) + (sub2_n > bmf_p2_thresholds[sub3_row][7]) + (sub2_n > bmf_p2_thresholds[sub3_row][6]))));
  v28 = (AltP2Block *)(v289);
  v257 = (uint8_t *)&((int16_t *)v281)[160 * sub3_row + 2 * sub5_n]
       + (sub4_n > bmf_p2_thresholds[sub3_row][2] * (int32_t)sub1_nb)
       + (sub4_n > (int32_t)sub1_nb * bmf_p2_thresholds[sub3_row][1])
       + (sub4_n > (int32_t)sub1_nb * bmf_p2_thresholds[sub3_row][0])
       + __frame.v256;
  nb_id = v289->nb_id[(uint32_t)v257];
  if ( v289->nb_id[(uint32_t)v257] )
  {
    v31 = &((float (*)[4])v289)[16 * nb_id];
    *(int32_t *)&v289->f278656 = (int32_t)v31;
    // Normalised LMS.  Predict from the seven weight rows against the seven
    // inputs, take the error against what actually came, and step every weight
    // by  rate * error * input / (that input's running mean square + a floor).
    // Rows 7..13 of the block hold the mean squares; row 14 lane 2 scales the
    // floor.
    {
      float acc[4], err, floor_;
      int32_t j, k;

      for ( k = 0; k < 4; ++k )
      {
        acc[k] = v31[0][k] * v28->p2_row[0][k];
        for ( j = 1; j < 7; ++j )
          acc[k] += v31[j][k] * v28->p2_row[j][k];
      }
      err = ((float)__frame.v246[-1].lane[0]
             - (bmf_hsum4(acc) + v28->bias[0])) * 2.0999999f;
      floor_ = 7744.0f * v31[14][2];

      for ( j = 0; j < 7; ++j )
        for ( k = 0; k < 4; ++k )
        {
          float x  = v28->p2_row[j][k];
          float ms = v31[7 + j][k]
                   + (x * x - v31[7 + j][k]) * bmf_p2_ms_rate;
          v31[7 + j][k] = ms;
          v31[j][k] += bmf_p2_rate[j][k] * err * x / (ms + floor_);
        }
    }
  }
  else
  {
    v30 = v289->nb_id_used;
    v289->nb_id_used = ++v30;
    v28->nb_id[(uint32_t)v257] = v30;
    *(int32_t *)&v28->f278656 = (int32_t)&((float (*)[4])v28)[16 * (int16_t)v30];
  }
  v45 = (P2Ctx *)v28->cursor[1];
  v28->p2_row[0][0] = (float)v45->lane[1];
  v46 = (P2Ctx *)v28->cursor[0];
  v28->p2_row[0][1] = (float)v45[1].lane[1];
  v28->p2_row[0][2] = (float)(v46[-1].lane[1] + v45->lane[1] - v45[-1].lane[1]);
  sub1_nb = (P2Ctx *)v28->cursor[2];
  v49 = (P2Ctx *)(sub1_nb);
  v28->p2_row[0][3] = (float)(v46[-2].lane[1] + (v45->lane[1] - v45[-2].lane[1]));
  v50 = (P2Ctx *)v28->cursor[3];
  v28->p2_row[1][0] = (float)(v45[-1].lane[1] + v45->lane[1] - v49[-1].lane[1]);
  v28->p2_row[1][1] = (float)(-3 * (v49->lane[1] - v45->lane[1]) + v50->lane[1]);
  v28->p2_row[1][2] = (float)(v46[-1].lane[1] + v45[2].lane[1] - v45[1].lane[1]);
  v28->p2_row[1][3] = (float)(v46[-2].lane[1] + v45[1].lane[1] - v45[-1].lane[1]);
  v28->p2_row[2][0] = (float)(2 * v46[-1].lane[1] - v46[-2].lane[1]);
  v52 = (P2Ctx *)(sub1_nb);
  v28->p2_row[2][1] = (float)(v46[-3].lane[1] + v45->lane[1] - v45[-3].lane[1]);
  v28->p2_row[2][2] = (float)(v52->lane[1] + v45[1].lane[1] - v50[1].lane[1]);
  v28->p2_row[2][3] = (float)v46[-3].lane[1];
  v28->p2_row[3][0] = (float)(v45[-2].lane[1] + v45->lane[1] - v52[-2].lane[1]);
  v28->p2_row[3][1] = (float)(v46[-2].lane[1] + v45[-1].lane[1] - v45[-3].lane[1]);
  v28->p2_row[3][2] = (float)(v45[1].lane[1] + ((v45[2].lane[1] + v45->lane[1]) >> 1) - v52[2].lane[1]);
  v28->p2_row[3][3] = (float)v50->lane[1];
  if ( a4 )
  {
    v282 = (P2Ctx *)a4->cursor[0] - 1;
    v281 = (P2Ctx *)((int16_t *)(a4->cursor[1] - 1));
    v286 = (P2Ctx *)((int16_t *)(a4->cursor[2] - 1));
    v284 = (P2Ctx *)a5->cursor[0] - 1;
    v58 = v28->has_ref == 0;
    v283 = a5->cursor[1] - 1;
    v285 = a5->cursor[2] - 1;
    if ( !v58 )
    {
      // One number added to all sixteen floats of the first four rows.
      {
        float bias = (float)v46->lane[1];
        int32_t j, k;
        for ( j = 0; j < 4; ++j )
          for ( k = 0; k < 4; ++k )
            v28->p2_row[j][k] += bias;
      }
      v46 = (P2Ctx *)v28->cursor[0];
      v45 = (P2Ctx *)v28->cursor[1];
    }
    v65 = *(int32_t *)&v28->plane_idx;
    if ( v65 )
    {
      if ( v65 == 1 )
      {
        v81 = (P2Ctx *)(v281);
        v28->p2_row[4][0] = (float)(v46->lane[1] + v45[3].lane[1]);
        v28->p2_row[4][1] = (float)(v46[-2].lane[0] + v81[2].lane[0] - v81->lane[0]);
        v28->p2_row[4][2] = (float)(v45[1].lane[0] + v282[-1].lane[0] - v81->lane[0]);
        v83 = (P2Ctx *)(v283);
        v28->p2_row[4][3] = (float)(v46[-1].lane[0] + v81->lane[0] - v81[-1].lane[0]);
        v85 = v83[2].lane[0] - ((P2Ctx *)(v285))[3].lane[0];
        v86 = v284;
        v28->p2_row[5][0] = (float)(v45[1].lane[0] + v85);
        v87 = (P2Ctx *)((int16_t *)(v282));
        v28->p2_row[5][1] = (float)(v46[-2].lane[0] + v86->lane[0] - v86[-2].lane[0]);
        v28->p2_row[5][2] = (float)(v45->lane[0] + v86->lane[0] - v83->lane[0]);
        v58 = v28->has_ref == 0;
        v28->p2_row[5][3] = (float)(v46[-2].lane[0] + v87->lane[2]);
        if ( v58 )
        {
          v259 = (int32_t)(uintptr_t)v46;
          v90 = (int16_t *)v28->cursor[2];
          v28->p2_row[6][0] = ((float)(v46[-2].lane[0] + v87->lane[0] - v87[-2].lane[0]));
          v92 = (P2Ctx *)(v281);
          v28->p2_row[6][1] = (float)(*v90 + v87->lane[0] - ((P2Ctx *)(v286))->lane[0]);
          v28->p2_row[6][2] = (float)(v286->lane[0] + v87->lane[0] - *v90 + 2 * (v45->lane[0] - v92->lane[0]));
          v88 = *(int16_t *)((uint8_t *)v259 - 18) + v45[-1].lane[0] + v281[-2].lane[0] + v87->lane[0] - v87[-1].lane[0] - v281[-1].lane[0] - v45[-2].lane[0];
        }
        else
        {
          v28->p2_row[6][0] = (float)v45->lane[0];
          v28->p2_row[6][1] = (float)v46[-3].lane[0];
          v28->p2_row[6][2] = (float)(v46[-1].lane[0] + v87[-1].lane[0] - v87[-2].lane[0]);
          v88 = v46[-3].lane[0] + v46[-1].lane[0] - v46[-4].lane[0];
        }
        v28->p2_row[6][3] = (float)v88;
      }
      else
      {
        v93 = v282;
        v28->p2_row[4][0] = (float)(v46[-3].lane[0] + v46[-1].lane[0] - v46[-4].lane[0]);
        v95 = (P2Ctx *)(v281);
        v96 = (P2Ctx *)(v286);
        v28->p2_row[4][1] = ((float)(v46[-1].lane[0] + v93[-1].lane[0] - v93[-2].lane[0]));
        v28->p2_row[4][2] = (float)(v45[1].lane[0] + v95->lane[0] - v96[1].lane[0]);
        v97 = v282;
        v28->p2_row[4][3] = (float)(v46[-2].lane[0] + v95[2].lane[0] - v95->lane[0]);
        v28->p2_row[5][0] = (float)(v45->lane[0] + v97[-2].lane[0] - v95[-2].lane[0]);
        v28->p2_row[5][1] = (float)(v45->lane[0] + v97->lane[0] - v95->lane[0]);
        v98 = (P2Ctx *)((int16_t *)(v284));
        v28->p2_row[5][2] = (float)(v46[-2].lane[0] + v97->lane[0] - v97[-2].lane[0]);
        v99 = (int16_t *)v28->cursor[2];
        v28->p2_row[5][3] = (float)(v46[-2].lane[0] + v98->lane[0] - v98[-2].lane[0]);
        v28->p2_row[6][0] = (float)(*v99 + v98->lane[0] - ((P2Ctx *)(v285))->lane[0]);
        v28->p2_row[6][1] = (float)(v98[-2].lane[0]
                                       + v98->lane[0]
                                       - v46[-2].lane[0]
                                       + 2 * (v46[-1].lane[0] - v98[-1].lane[0]));
        v28->p2_row[6][2] = ((float)(v46[-2].lane[0] + v98->lane[2]));
        v28->p2_row[6][3] = (float)(*v99 + v282->lane[2]);
      }
    }
    else
    {
      v28->p2_row[4][0] = (float)(v46[-3].lane[0] + v46[-1].lane[0] - v46[-4].lane[0]);
      v258 = v46;
      v66 = v28->cursor[3];
      v28->p2_row[4][1] = (float)(v46[-5].lane[0] + v45->lane[0] - v45[-5].lane[0]);
      v67 = (P2Ctx *)v28->cursor[2];
      v28->p2_row[4][2] = (float)(v46[-4].lane[0] + v45->lane[0] - v45[-4].lane[0]);
      v68 = v67->lane[0] + 3 * v45[1].lane[0] - 4 * v67[1].lane[0];
      v69 = (P2Ctx *)(v281);
      v70 = (float)(v68 - (((v45[2].lane[0] - v45->lane[0] - (v66[2].lane[0] - v66->lane[0])) >> 1) - v66[1].lane[0]));
      v71 = v282;
      v28->p2_row[4][3] = v70;
      v72 = (P2Ctx *)(v286);
      v28->p2_row[5][0] = (float)(v258[-2].lane[0] + v71->lane[0] - v71[-2].lane[0]);
      v28->p2_row[5][1] = (float)(v45[1].lane[0] + v69[1].lane[0] - v72[2].lane[0]);
      v73 = v282;
      v28->p2_row[5][2] = (float)(v45[-2].lane[0] + v69[2].lane[0] - v72->lane[0]);
      v74 = v258[-1].lane[0] - v73[-1].lane[0];
      v75 = v73[-2].lane[0] + v73->lane[0] - v258[-2].lane[0];
      v76 = v284;
      v78 = (P2Ctx *)(v285);
      v28->p2_row[5][3] = ((float)(v75 + 2 * v74));
      v28->p2_row[6][0] = (float)(v67[1].lane[0] + v76->lane[0] - v78[1].lane[0]);
      v80 = (P2Ctx *)(v283);
      v28->p2_row[6][1] = ((float)(v258[-2].lane[0] + v76->lane[0] - v76[-2].lane[0]));
      v28->p2_row[6][2] = (float)(v258[-1].lane[0] + v80[1].lane[0] - v80->lane[0]);
      v28->p2_row[6][3] = (float)(v45[1].lane[0] + v80[2].lane[0] - v78[3].lane[0]);
    }
  }
  else
  {
    v274 = v45;
    v28->p2_row[4][0] = (float)(v45[3].lane[0] + v45->lane[0] - v52[3].lane[0]);
    v239 = v45->lane[0];
    v240 = v45[-4].lane[0];
    sub1_nb = v52;
    v28->p2_row[4][1] = (float)(v46[-4].lane[0] + v239 - v240);
    v241 = v52->lane[0] + 3 * v45[1].lane[0] - 4 * v52[1].lane[0];
    v242 = v45[2].lane[0] - v45->lane[0];
    v244 = (float)(v241 - (((v242 - (v50[2].lane[0] - v50->lane[0])) >> 1) - v50[1].lane[0]));
    v28->p2_row[4][2] = v244;
    v255 = (P2Ctx *)v28->cursor[4];
    v28->p2_row[4][3] = (float)(sub1_nb[-1].lane[0] + v274[1].lane[0] - v50->lane[0]);
    v28->p2_row[5][0] = (float)(v50[1].lane[0] + v274->lane[0] - v255[1].lane[0]);
    v28->p2_row[5][1] = (float)v274[3].lane[0];
    v28->p2_row[5][2] = (float)(v46[-3].lane[0] + v46[-1].lane[0] - v46[-4].lane[0]);
    v28->p2_row[5][3] = (float)(v46[-1].lane[0] + v50->lane[0] - v50[-1].lane[0]);
    v28->p2_row[6][0] = (float)(v46[-5].lane[0] + v274->lane[0] - v274[-5].lane[0]);
    v28->p2_row[6][1] = (float)v255->lane[0];
    v28->p2_row[6][2] = (float)(v46[-5].lane[0] + v46[-1].lane[0] - v46[-6].lane[0]);
    v28->p2_row[6][3] = (float)v274[-6].lane[0];
    v285 = (P2Ctx *)(nullptr);
    v283 = (P2Ctx *)(nullptr);
    v284 = nullptr;
    v286 = (P2Ctx *)(nullptr);
    v281 = (P2Ctx *)(nullptr);
    v282 = nullptr;
  }
  cur = v28->cur;
  __frame.sub0 = (float (*)[4])*(cur - 1);
  v104 = v28->above;
  __frame.sub1 = (float (*)[4])v104[1];
  __frame.sub2 = (float (*)[4])v104[2];
  __frame.sub3 = (float (*)[4])*(cur - 2);
  __frame.sub4 = (float (*)[4])*v104;
  __frame.sub5 = (float (*)[4])*cur;
  v105 = 14 * n3536;
  n2 = 1;
  v107 = 13 * n1840_2;
  if ( 16 * v292_cost <= 14 * n3536 )
  {
    v105 = 16 * v292_cost;
    n2 = 0;
  }
  if ( v105 > v107 )
    n2 = 2;
  else
    v107 = v105;
  if ( v107 > 11 * n1840_1 )
    n2 = 3;
  n3536_5 = __alt_p2_filter((float (*)[4])(void *)*(int32_t *)&v28->f278656, (float (*)[4])v28->p2_row, (CtxWeights *)__frame.sub, n2);
  v109 = (P2Ctx *)v28->cursor[0];
  v293 = (P2Ctx *)v28->cursor[1];
  *(int32_t *)&v28->pred_prev = n3536_5;
  lane3 = v293->lane[3];
  v112 = v109[-1].lane[3];
  v293 = v293;
  v269 = v112 + lane3;
  if ( a4 )
    v269 += (v284->lane[3] + v282->lane[3]) >> 1;
  v113 = v109[-2].lane[3];
  v114 = v109[-3].lane[3];
  v289 = (AltP2Block *)(v28);
  n3536 = n3536_5;
  v116 = v109[-4].lane[3];
  v294 = (P2Ctx *)(v28->cursor[3]);
  v117 = v293[3].lane[3] + v293[-1].lane[3] + v112 + v114 + v116 + v113;
  v118 = (AltP2Block *)(v289);
  n3536_1 = n3536;
  v260 = v109[-1].lane[2];
  v120 = (v293[1].lane[2] + v293->lane[2] + v293[-1].lane[2] + v260) & 0x80000
       | (v294->lane[2] + v109[-2].lane[2] + 2 * v109[-4].lane[2]) & 0x40000
       | (v293[-3].lane[2] + v109[-3].lane[2] + v109[-5].lane[2] + v109[-7].lane[2]) & 0x20000
       | v260 & 0x10000
       | (uint16_t)v109[-3].lane[2] & 0x8000
       | (((n3536 > 3536) + (n3536 > 720) + (n3536 > 288)) << 13)
       | ((((uint32_t)(752 - (v269 + v117)) >> 31)
         + ((uint32_t)(400 - (v269 + v117)) >> 31)
         + ((uint32_t)(240 - (v269 + v117)) >> 31)) << 11);
  v289->bank_ctx[0] = v120;
  if ( a4 )
  {
    v121 = v282[-2].lane[2];
    v289 = (AltP2Block *)(v118);
    v122 = v109[-2].lane[2];
    lane2 = v282->lane[2];
    v124 = v282[-1].lane[2];
    n3536 = n3536_1;
    v125 = v284[-1].lane[2] & 0x2000000
         | v284->lane[2] & 0x1000000
         | v124 & 0x800000
         | (v284->lane[2] + v284[-2].lane[2]) & 0x400000
         | (v121 + lane2) & 0x200000
         | v122 & 0x100000
         | v120;
    v118 = (AltP2Block *)(v289);
    v295 = (P2Ctx *)v289->cursor[2];
  }
  else
  {
    v236 = v109[-4].lane[2];
    v237 = v109[-5].lane[2];
    v238 = v109[-7].lane[2];
    v289 = (AltP2Block *)(v118);
    n3536 = n3536_1;
    v295 = (P2Ctx *)v118->cursor[2];
    v125 = (v293[3].lane[2] + v109[-3].lane[2] + v238 + v237) & 0x2000000
         | (v295[1].lane[2] + v295->lane[2] + v236 + v109[-8].lane[2]) & 0x1000000
         | (v236 + v109[-6].lane[2]) & 0x800000
         | v236 & 0x400000
         | v237 & 0x200000
         | v238 & 0x100000
         | v120;
  }
  v126 = v125 >> 11;
  v118->bank_ctx[0] = v126;
  v127 = p2_pred(v118->p2_ctr[v126].w2, v118->p2_ctr[v126].b0);
  v128 = v290;
  v129 = v118->cursor[4];
  v118->nb_sum[1] = v127;
  n2256 = v127 + n3536_1;
  v118->nb_sum[0] = n2256;
  v132 = v109[-5].lane[0];
  v296 = (P2Ctx *)(v129);
  n1840 = v129[4].lane[0];
  v290 = ((v269 << 9) + v128) >> 13;
  n960_1 = v132 - n2256;
  v133 = ((uint32_t)(24 - v290) >> 20) & 0xFFFFF800;
  n1840_1 = n1840 - n2256;
  v301 = v293[5].lane[0];
  n1840_2 = v301 - n2256;
  v297 = ((uint32_t)(39 - v290) >> 20) & 0xFFFFF800;
  if ( a4 )
  {
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v289 = (AltP2Block *)(v118);
    v134 = v282->lane[1];
    v248 = v109[0].lane[1];
    v270 = v293[2].lane[1];
    v135 = (v248 + v109[-3].lane[1] - n2256 - (v270 - v293[5].lane[1])) & 0x800000
         | (v248 + v109[-5].lane[1] - n2256) & 0x400000
         | (v248 + v293->lane[1] + v283[2].lane[1] - v285[2].lane[1] - n2256) & 0x200000
         | (v248 + v109[-1].lane[1] + v284->lane[1] - v284[-1].lane[1] - n2256) & 0x100000
         | (v295[-1].lane[1] + v248 + v134 - v286[-1].lane[1] - n2256) & 0x80000
         | (v248 + v270 + v134 - v281[2].lane[1] - n2256) & 0x40000
         | n1840_2 & 0x20000
         | n1840_1 & 0x10000
         | n960_1 & 0x8000
         | (((n2256 > 2256) + (n2256 > 1056) + (n2256 > 144)) << 13)
         | ((((uint32_t)(55 - v290) >> 20) & 0xFFFFF800) + v298 + v133);
    v118 = (AltP2Block *)(v289);
    v136 = v298;
    v137 = (v109[-1].lane[0] + v282->lane[0] - v282[-1].lane[0] - n2256) & 0x2000000
         | (v286->lane[1] + v281->lane[1] - 2 * v282->lane[1]) & 0x1000000
         | v135;
  }
  else
  {
    v289 = (AltP2Block *)(v118);
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v254 = v294->lane[0];
    v267 = n2256 - v109[-3].lane[0];
    v273 = v293[2].lane[0];
    v137 = (v293[-5].lane[0] - v293[-2].lane[0] + v267) & 0x1000000
         | (v267 + v273 - v301) & 0x800000
         | (v296[3].lane[0] - v295[2].lane[0] + n2256 - v295[1].lane[0]) & 0x400000
         | (v296[-1].lane[0] - v254 + n2256 - v293[-1].lane[0]) & 0x200000
         | (v293->lane[0] - v273 + n2256 - v109[-2].lane[0]) & 0x100000
         | -n1840_2 & 0x80000
         | -n1840_1 & 0x40000
         | (n2256 - v296[2].lane[0]) & 0x20000
         | (n2256 - v296[-3].lane[0]) & 0x10000
         | -n960_1 & 0x8000
         | (((n2256 > 2400) + (n2256 > 1024) + (n2256 > 240)) << 13)
         | (v297 + v133 + (((uint32_t)(11 - v290) >> 20) & 0xFFFFF800))
         | (n1840 - v254 + n2256 - v293[4].lane[0]) & 0x2000000;
    v136 = v298;
  }
  v298 = v136;
  v138 = v137 >> 11;
  v118->bank_ctx[1] = v138;
  v139 = v118->p2_ctr[v138 + 32768].b0;
  v140 = v118->p2_ctr[v138 + 32768].w2;
  v141 = 1 << ((v139 + 31) & 31);
  v144 = (v141 + v140) >> (v139 & 31);
  n2576 = v144 + n2256;
  v118->nb_sum[2] = n2576;
  v118->nb_sum[3] = v144;
  n1840_13 = v295->lane[0];
  n960_1 = v293->lane[0];
  v147 = v109[-2].lane[0];
  n1840 = n1840_13;
  v302 = v147 - n2576;
  v301 = v147 + n2576 - 2 * v109[-1].lane[0];
  v303 = n1840_13 - n2576;
  if ( a4 )
  {
    v148 = v109[0].lane[1];
    v149 = v109[-1].lane[1];
    v289 = (AltP2Block *)(v118);
    v151 = v282->lane[0];
    n1840_2 = v293[1].lane[1];
    n1840_1 = v151 - n2576;
    v152 = (v148 - v109[-2].lane[1] + 2 * v149 - n2576) & 0x100000
         | (2 * n960_1 - n2576 - n1840) & 0x80000
         | -v301 & 0x40000
         | v303 & 0x20000
         | v302 & 0x10000
         | (208 - v284->lane[0]) & 0x8000
         | (((n2576 > 2576) + (n2576 > 1280) + (n2576 > 640)) << 13)
         | ((((uint32_t)(33 - v290) >> 31) + ((uint32_t)(12 - v290) >> 31) + ((uint32_t)(4 - v290) >> 31)) << 11);
    v153 = v298;
    v154 = (v148 + v293[-1].lane[1] - n2576 - (v295->lane[1] - n1840_2)) & 0x2000000
         | (v148 + v149 - n2576 - (v293->lane[1] - n1840_2)) & 0x1000000
         | (v284->lane[0] - n2576 + n1840 - v285->lane[0]) & 0x800000
         | (n1840_1 + n1840 - v286->lane[0]) & 0x400000
         | (n1840_1 + v293[-1].lane[0] - v281[-1].lane[0]) & 0x200000
         | v152;
  }
  else
  {
    v235 = v294->lane[0];
    v289 = (AltP2Block *)(v118);
    v153 = v298;
    v266 = v296->lane[0];
    v272 = v294[1].lane[0];
    v154 = (n2576 + 3 * (n1840 - n960_1) - v235) & 0x2000000
         | (n2576 + v266 - (v295[2].lane[0] + v295[-2].lane[0])) & 0x1000000
         | (v294[2].lane[0] - v295[-1].lane[0] + n2576 - v293[3].lane[0]) & 0x800000
         | (v272 - v293[1].lane[0] - v303) & 0x400000
         | (v235 - n960_1 - v303) & 0x200000
         | v301 & 0x100000
         | (n2576 - v296[3].lane[0]) & 0x80000
         | (v272 - n2576) & 0x40000
         | (n2576 - v266) & 0x20000
         | (n2576 - v294[-2].lane[0]) & 0x10000
         | -v302 & 0x8000
         | (((n2576 > 2464) + (n2576 > 1216) + (n2576 > 688)) << 13)
         | ((((uint32_t)(58 - v290) >> 31) + ((uint32_t)(25 - v290) >> 31) + ((uint32_t)(13 - v290) >> 31)) << 11);
  }
  v289 = (AltP2Block *)(v118);
  v298 = v153;
  v155 = v154 >> 11;
  v118->bank_ctx[2] = v155;
  v156 = p2_pred(v118->p2_ctr[v155 + 65536].w2, v118->p2_ctr[v155 + 65536].b0);
  v118->nb_sum[5] = v156;
  n2896 = v156 + n2576;
  v118->nb_sum[4] = n2896;
  v161 = v293->lane[1];
  v162 = v109[-2].lane[0];
  v250 = v109[0].lane[1];
  v163 = v109[-3].lane[0];
  v164 = v295[1].lane[1];
  v303 = n2896 - v163;
  v306 = n2896 - v250;
  v165 = 9 - v290;
  v290 = -v290;
  v166 = (AltP2Block *)(v289);
  v167 = (int32_t)((3 * (v162 - v109[-1].lane[0]) + n2896 - v163) & 0x2000000
             | (v164
              - ((uint32_t)(v293[1].lane[1] + v293[2].lane[1] + v293[-1].lane[1] + v161) >> 1)
              + v306)
             & 0x1000000
             | (v295[-3].lane[0] - v293[-2].lane[0] + n2896 - v293[-1].lane[0]) & 0x800000
             | -(v295[2].lane[0] + n2896 - 2 * v293[1].lane[0]) & 0x400000
             | (v293[-2].lane[1] - v162 + n2896 - v161) & 0x200000
             | (n2896 - v293[3].lane[0]) & 0x100000
             | (v306 - v164) & 0x80000
             | (n2896 - v294->lane[0]) & 0x40000
             | (2 * n2896 - v161 - (v293->lane[0] + v250)) & 0x20000
             | (n2896 - v109[-1].lane[1] - v250) & 0x10000
             | (n2896 - v163) & 0x8000
             | (((n2896 > 2896) + (n2896 > 1568) + (n2896 > 592)) << 13)
             | ((((uint32_t)(v290 + 37) >> 31) + ((uint32_t)(v290 + 19) >> 31) + (v165 >> 31)) << 11)) >> 11;
  v289->bank_ctx[3] = v167;
  v168 = v166->p2_ctr[v167 + 98304].w2;
  v170 = (P2Ctx *)(v294);
  // The rate reached `>>` through two `LOBYTE` copies, `v163` then `v164`;
  // both masks read only the byte each copy wrote.
  v171 = p2_pred(v168, v166->p2_ctr[v167 + 98304].b0);
  v166->nb_sum[7] = v171;
  n3536_2 = v171 + n2896;
  n3536 = n3536_2;
  v166->nb_sum[6] = n3536_2;
  v175 = v293->lane[0];
  v176 = v170->lane[0];
  v307 = v109[0].lane[1];
  v177 = v109[-4].lane[1];
  v178 = v293[1].lane[0];
  v179 = v293[-1].lane[0];
  v312 = n3536_2 - v307;
  v313 = n3536_2 + v176;
  v180 = (((n3536_2 > 3056) + (n3536_2 > 1952) + (n3536_2 > 368)) << 13)
       | (v297 + (((uint32_t)(v290 + 21) >> 20) & 0xFFFFF800) + v298);
  v181 = ((uint16_t)n3536_2 - (uint16_t)v177 - (uint16_t)v307) & 0x8000;
  v182 = n3536_2 - v109[-1].lane[0];
  n3536_3 = n3536;
  v184 = (AltP2Block *)(v289);
  v185 = (int32_t)((n3536 - ((uint32_t)(v178 + v179 + 2 * v175) >> 2)) & 0x2000000
             | (v313 - v109[-2].lane[0] - (v294[2].lane[1] + v307)) & 0x1000000
             | (n3536 - 2 * v109[-2].lane[1] - (v307 - v177)) & 0x800000
             | (n3536 - v109[-3].lane[1] - v307 - (v175 - v293[-3].lane[0])) & 0x400000
             | (v313 - v307 - (v178 + v295[-1].lane[1])) & 0x200000
             | (v295[-2].lane[0] + n3536 - 2 * v179) & 0x100000
             | (v295->lane[1] - 2 * v293->lane[1] + v312) & 0x80000
             | (v182 - (v175 - v179)) & 0x40000
             | (v312 - v293[4].lane[1]) & 0x20000
             | (v312 - v293[-2].lane[1]) & 0x10000
             | v181
             | v180) >> 11;
  v289->bank_ctx[4] = v185;
  v186 = p2_pred(v184->p2_ctr[v185 + 131072].w2, v184->p2_ctr[v185 + 131072].b0);
  v187 = (uint8_t *)v295;
  v184->nb_sum[9] = v186;
  n1840 = n3536_3 + v186;
  v184->nb_sum[8] = n3536_3 + v186;
  v188 = v293[-1].mag;
  v189 = v187[17];
  v314 = v293[-2].mag
       + v293[3].mag
       + v109[-3].mag
       + v294->mag
       + *(v187 - 1)
       + v187[53];
  v190 = v187[89];
  v191 = v188 + v109[-2].mag + v189;
  v192 = *(v187 - 19);
  v193 = v294[2].mag
       + v294[1].mag
       + v294[-1].mag
       + v294[-2].mag
       + v296[-2].mag
       + v187[107]
       + v190
       + v187[71]
       + v192
       + *(v187 - 37);
  n1840_14 = n1840;
  n960_1 = v293[5].mag
         + v293[4].mag
         + v293[-3].mag
         + v293[-4].mag
         + 3 * (v293[2].mag + v295[1].mag)
         + 7 * v293->mag
         + 6 * v293[1].mag
         + v109[-6].mag
         + v109[-7].mag
         + v109[-8].mag
         + 8 * v109[-1].mag
         + v296[2].mag
         + v296[1].mag
         + v296->mag
         + v296[-1].mag
         + v109[-4].mag
         + v109[-5].mag
         + v193
         + 4 * v191
         + 2 * v314;
  v196 = (AltP2Block *)(v289);
  n1840_15 = v289->band_lo;
  v289->ctx_w[0].sel = (n1840 < 1840) + (n1840 < 272);
  v198 = v293->lane[0];
  n1840_16 = v196->band_hi;
  n1840_1 = n1840_15;
  n1840_2 = n1840_16;
  v200 = (n1840_14 - v198 <= n1840_16) + (n1840_14 - v198 < n1840_15);
  v196->ctx_w[1].sel = v200;
  n1840_17 = n1840_14 - v109[-1].lane[0];
  v203 = n1840_17 < n1840_1;
  v26 = n1840_17 <= n1840_16;
  v196->ctx_w[2].sel = v26 + v203;
  v196->ctx_w[3].sel = v293->sign;
  // The previous record's stored sign digit, straight into the fifth
  // group's selector.  Hex-Rays read it as `(uint8_t)lane[8]` -- one past
  // the eight lanes, whose low byte is `sign`.
  v196->ctx_w[4].sel = v109[-1].sign;
  v206 = v109[-2].mag;
  v207 = v109[-1].mag;
  v208 = v295[0].mag + v293->mag;
  n960 = n960_1;
  v302 = v208;
  n3536_4 = n3536;
  v301 = v207 + v206;
  if ( a4 )
  {
    mag = v284->mag;
    v212 = v284[-1].mag;
    v289 = (AltP2Block *)(v196);
    v213 = v282[-1].mag;
    v251 = v283->mag + v281->mag;
    v264 = mag + v282->mag;
    n960 = v251 + n960_1 + 4 * v264 + 2 * (v213 + v212);
    v214 = v264 + v301 + v284[-2].mag + v212 + v282[-2].mag + v213;
    n3536_4 = n3536;
    if ( v196->has_ref )
    {
      n1840_3 = n1840 - v293->lane[1] - v109[0].lane[1];
      if ( n1840_3 < n1840_1 || n1840_3 > n1840_2 )
      {
        n1840 = n1840 - v109[-1].lane[1] - v109[0].lane[1];
        v215 = n1840 >= n1840_1 && n1840_2 >= n1840;
      }
      else
      {
        v215 = 1;
      }
    }
    else
    {
      v215 = v251 + v264 + v302 + v285->mag + v286->mag;
    }
    if ( *(int32_t *)&v289->plane_idx == 1 )
    {
      v230 = v282;
      n960_1 = n960;
      n1840_4 = n1840_2;
      n1840_5 = v282->lane[0] - v281->lane[0];
      v289->ctx_w[3].sel = (n1840_5 <= n1840_2) + (n1840_5 < n1840_1);
      n1840_6 = v230->lane[0] - v230[-1].lane[0];
      v234 = (n1840_6 <= n1840_4) + (n1840_6 < n1840_1);
      v214 = v214;
      n960 = n960_1;
      n3536_4 = n3536;
      v196->ctx_w[4].sel = v234;
    }
    else if ( *(int32_t *)&v196->plane_idx > 1 )
    {
      v217 = v284;
      n960_1 = n960;
      n1840_7 = n1840_2;
      n1840_8 = v284->lane[0] - v283->lane[0];
      v289->ctx_w[3].sel = (n1840_8 <= n1840_2) + (n1840_8 < n1840_1);
      n1840_10 = v217->lane[0] - v217[-1].lane[0];
      n1840_9 = n1840_1;
      v222 = n1840_10 < n1840_1;
      v26 = n1840_10 <= n1840_7;
      v215 = v215;
      n960 = n960_1;
      v196->ctx_w[4].sel = v26 + v222;
      v224 = v282->lane[0];
      n1840_11 = v224 - ((P2Ctx *)(v281))->lane[0];
      v26 = n1840_9 <= n1840_11;
      n3536_4 = n3536;
      if ( v26 && n1840_11 <= n1840_2 )
      {
        v214 = 1;
      }
      else
      {
        n1840_12 = v224 - v282[-1].lane[0];
        v214 = n1840_12 >= n1840_1 && n1840_12 <= n1840_2;
      }
    }
  }
  else
  {
    v214 = v301 + v109[-3].mag + v109[-4].mag + v109[-5].mag;
    v215 = v296->mag + v294->mag + v302 + v109[0].mag;
  }
  if ( n960 >= 960 )
  {
    n15 = 15;
    v196->ctx = 15;
  }
  else
  {
    n15 = v196->nb_ctx[n960 >> 3];
    v196->ctx = n15;
  }
  n255 = (n3536_4 + v186 + 7) >> 4;
  if ( n255 >= 255 )
    n255 = 255;
  if ( n255 < 0 )
    n255 = 0;
  v196->ctx_pair[0] = v196->ctx_delta[n255 + 4] + n15;
  v228 = v196->ctx_w[0].sel;
  v196->ctx_pair[1] = n15 + v196->ctx_delta[n255];
  v196->ctx = n15
                          + 32 * (v215 == 0)
                          + 16 * (v214 == 0)
                          + v196->ctx_w[4].w[v196->ctx_w[4].sel]
                          + v196->ctx_w[3].w[v196->ctx_w[3].sel]
                          + v196->ctx_w[2].w[v196->ctx_w[2].sel]
                          + v196->ctx_w[1].w[v196->ctx_w[1].sel]
                          + v196->ctx_w[0].w[v228];
  return n255;
}


void __reduce_alphabet(ModelBlock *Blocka, int8_t a2, uint8_t *a3)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW aborts while compressing.
  struct alignas(16) ReduceAlphabetFrame {   // 66064 bytes, one stack frame
      uint32_t v78[15];
      void *v79;
      int32_t n0x2000_5;
      // 64 KiB: `memset(buf, 0, 0x10000)` clears it and the body walks it as
      // 8192 eight-byte records, so `v82`, `v83`, `v84`, `v85` and `_pad0` are
      // inside it -- the same bytes under other names at other times.  It stays
      // four bytes here because `v82` wants eight-byte alignment and `buf` does
      // not, and a union of the two would move the whole frame four bytes.
      uint8_t buf[4];
      uint64_t v82[127];
      int32_t v83;
      int32_t v84;
      int32_t v85;
      uint8_t _pad0[64504];
      uint8_t v86[16];
      int32_t v87;
      uint32_t v88[91];
      union {
          void *slot[19];   // one array, three bases: `&n4_1`, `*(&Block + n)` and the interleaved `(&v91)[2*j]` / `*(&v92 + 2*j)`
          // The locals MSVC spilled into these bytes.  The walk above reaches
          // the same slots again with an expression, which is the same storage
          // at a later point in the function and not a second variable.
          struct {
            int32_t slot0;
            void *slot1;
            uint8_t *slot2;
            uint32_t slot3;
            uint32_t slot4;
            void *slot5;
            uint32_t slot6;
            ModelBlock *slot7;
            uint8_t *slot8;
            uint32_t slot9;
            void *slot10;
            ModelBlock *slot11;
            void *slot_tail[7];
          };
      };
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 66064, "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), v86)
                   - __builtin_offsetof(__typeof__(__frame), buf) == 0x10000,
                "buf is not the 64 KiB the memset clears");
  ;
  ModelBlock *Blockaa_2;
  bool v46, v48, v59;
  int8_t v35;
  uint8_t *v28;   // `uint8_t *` beside the `char` scalars above
  ModelBlock *Blockaa_1;
  ModelBlock *Blockaa_4;
  int32_t n8, v8, v11, n4, n0x2000_2, n0x2000_1, v20, v26, n4_2, v30, v31, v32, *p_n4, n16_2,
          v39, v44, n256, height, v50, v51, v52, v54, v55, v56, v57, v58, v63, *p_n4_2, n16_1, alphabet,
          n0x2000, v71, v72, v74, *p_n4_1, n16;
  ModelBlock *Blockaa_3;
  uint32_t k_2, i, v12, v19, n0x2000_4, n0x2000_3, v24, k, v29, k_3, j_1, j, v53, v61, v64, v70,
           v73, v75;
  uint16_t *n0x2000_6;   // was uint64_t *, read only as uint16_t
  uint8_t *v4, *v10, *v33, *v42, *v43, *v45, *v60;
  void *v13, *v34;
  __frame.slot11 = (ModelBlock *)(Blocka);
  v4 = a3;
  n8 = Blocka->depth;
  __frame.slot2 = a3;
  __frame.slot7 = (ModelBlock *)(Blocka);
  __frame.slot4 = 0xFFFFFFFF >> (-n8 & 31);
  k_2 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v8 = 12 * i;
    __frame.v88[v8] = 0;
    __frame.v88[v8 + 6] = 0;
  }
  Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
  if ( n8 <= 8 )
  {
    n256 = 256;
    do
    {
      bmf_zero16(&__frame.v78[n256 + 12]);
      bmf_zero16(&__frame.v78[n256 + 8]);
      bmf_zero16(&__frame.v78[n256 + 4]);
      bmf_zero16(&__frame.v78[n256]);
      n256 -= 16;
    }
    while ( n256 * 4 );
    v48 = *(int32_t *)&Blockaa_1->depth < 8;
    height = Blockaa_1->height;
    *(int32_t *)&Blockaa_1->alphabet = 0;
    if ( v48 )
    {
      v50 = 0;
      if ( height )
      {
        v51 = *(int32_t *)&Blockaa_1->width;
        __frame.slot8 = a3 - 1;
        __frame.v84 = 0;
        v52 = 0;
        do
        {
          if ( !v51 )
            break;
          __frame.v83 = v52;
          v53 = 0;
          v54 = __frame.v84;
          v55 = 0;
          do
          {
            v56 = *(int32_t *)&Blockaa_1->depth;
            v57 = v55 - v56;
            if ( v57 < 0 )
            {
              ++__frame.slot8;
              v57 = 8 - v56;
            }
            v58 = __frame.slot4 & (*__frame.slot8 >> (v57 & 31));
            v59 = *(uint32_t *)&__frame.buf[4 * v58 - 4] == 0;
            __frame.v85 = v57;
            ++v53;
            *(uint32_t *)&__frame.buf[4 * v58 - 4] = 1;
            v55 = __frame.v85;
            *(int32_t *)&Blockaa_1->alphabet += v59;
            Blockaa_1->sym_word[v54] = v58;
            v51 = *(int32_t *)&Blockaa_1->width;
            ++v54;
          }
          while ( v53 < *(int32_t *)&Blockaa_1->width );
          v50 = *(int32_t *)&Blockaa_1->alphabet;
          __frame.v84 = v54;
          v52 = __frame.v83 + 1;
        }
        while ( __frame.v83 + 1 < (uint32_t)Blockaa_1->height );
      }
    }
    else if ( height * *(int32_t *)&Blockaa_1->width )
    {
      v60 = __frame.slot2;
      v61 = 0;
      do
      {
        *(int32_t *)&Blockaa_1->alphabet += *(uint32_t *)&__frame.buf[4 * *v60 - 4] == 0;
        v63 = *v60;
        *(uint32_t *)&__frame.buf[4 * v63 - 4] = 1;
        ++v60;
        Blockaa_1->sym_word[v61++] = v63;
      }
      while ( v61 < Blockaa_1->height * *(int32_t *)&Blockaa_1->width );
      v50 = *(int32_t *)&Blockaa_1->alphabet;
    }
    else
    {
      v50 = 0;
    }
    rc.encode(v50 - 1, v50, __frame.slot4 + 1);
    v64 = *(int32_t *)&Blockaa_1->alphabet;
    if ( v64 <= __frame.slot4 )
    {
      __init_symbol_list((SymList *)__frame.v86, (int32_t)Blockaa_1, __frame.slot4 - v64 + 2, 1);
      __frame.v87 = 19 * ((SymList *)__frame.v86)->n;
      v70 = *(int32_t *)&Blockaa_1->alphabet;
      if ( v70 )
      {
        v71 = 0;
        v72 = 0;
        v73 = 0;
        do
        {
          if ( *(uint32_t *)&__frame.buf[4 * v72 - 4] )
          {
            __encode_symbol_list((SymList *)__frame.v86, v72 - v71);
            v70 = *(int32_t *)&Blockaa_1->alphabet;
            *(uint32_t *)&__frame.buf[4 * v72 - 4] = v73;
            v74 = v72 + 1;
            v71 = v72 + 1;
            ++v73;
          }
          else
          {
            v74 = v72 + 1;
          }
          v72 = v74;
        }
        while ( v73 < v70 );
      }
      if ( Blockaa_1->height * *(int32_t *)&Blockaa_1->width )
      {
        v75 = 0;
        do
        {
          Blockaa_1->sym_word[v75] = *(uint32_t *)&__frame.buf[4
                                                                  * Blockaa_1->sym_word[v75]
                                                                  - 4];
          ++v75;
        }
        while ( v75 < Blockaa_1->height * *(int32_t *)&Blockaa_1->width );
      }
      p_n4_1 = (int32_t *)__frame.slot;
      n16 = 16;
      do
      {
        p_n4_1 -= 6;
        free((void *)p_n4_1[5]);
        --n16;
      }
      while ( n16 );
    }
    else
    {
      p_n4_2 = (int32_t *)__frame.slot;
      n16_1 = 16;
      do
      {
        p_n4_2 -= 6;
        free((void *)p_n4_2[5]);
        --n16_1;
      }
      while ( n16_1 );
    }
  }
  else
  {
    memset(__frame.buf,0,0x10000);
    *(int32_t *)&Blockaa_1->alphabet = 1;
    *(uint32_t *)__frame.buf = __frame.slot4 & *(uint32_t *)a3;
    Blockaa_1->sym_word[0] = 0;
    if ( (uint32_t)(Blockaa_1->height * *(int32_t *)&Blockaa_1->width) > 1 )
    {
      __frame.slot8 = a3;
      __frame.slot9 = k_2;
      __frame.slot7 = (ModelBlock *)((int32_t)Blockaa_1);
      v10 = __frame.slot2;
      v11 = 0;
      v12 = 1;
      while ( 1 )
      {
        v10 += __frame.slot9;
        v13 = (void *)(__frame.slot4 & *(uint32_t *)v10);
        if ( v13 != *(void **)&__frame.buf[8 * v11] )
        {
          v11 = 0;
          if ( v13 != *(void **)__frame.buf )
          {
            __frame.slot3 = v12;
            __frame.slot2 = v10;
            while ( 1 )
            {
              n4 = *(uint32_t *)&__frame.buf[8 * v11] < (uint32_t)v13;
              n0x2000_6 = (uint16_t *)&__frame.v82[v11];
              v11 = n0x2000_6[n4];
              if ( !n0x2000_6[n4] )
                break;
              if ( v13 == *(void **)&__frame.buf[8 * v11] )
              {
                v12 = __frame.slot3;
                v10 = __frame.slot2;
                mode_symbol[1] = n4;
                goto LABEL_12;
              }
            }
            __frame.n0x2000_5 = (int32_t)n0x2000_6;
            v10 = __frame.slot2;
            __frame.slot0 = n4;
            (__frame.slot[1]) = v13;
            Blockaa_2 = (ModelBlock *)(__frame.slot7);
            alphabet = __frame.slot7->alphabet;
            mode_symbol[1] = n4;
            v11 = (uint16_t)alphabet;
            n0x2000 = alphabet + 1;
            *(uint16_t *)(__frame.n0x2000_5 + 2 * n4) = v11;
            v12 = __frame.slot3;
            Blockaa_2->alphabet = n0x2000;
            if ( n0x2000 > 0x2000 )
            {
              v4 = __frame.slot8;
              n0x2000_2 = n0x2000;
              k_2 = __frame.slot9;
              Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
              goto LABEL_14;
            }
            *(void **)&__frame.buf[8 * v11] = (__frame.slot[1]);
          }
        }
LABEL_12:
        Blockaa_3 = (ModelBlock *)((uint32_t *)__frame.slot7);
        __frame.slot7->sym_word[v12++] = v11;
        if ( v12 >= *(uint32_t *)&Blockaa_3->height * Blockaa_3->width )
        {
          v4 = __frame.slot8;
          k_2 = __frame.slot9;
          Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
          n0x2000_2 = __frame.slot7->alphabet;
          goto LABEL_14;
        }
      }
    }
    n0x2000_2 = *(int32_t *)&Blockaa_1->alphabet;
LABEL_14:
    rc.encode(n0x2000_2 - 1, n0x2000_2, 0x2001u);
    n0x2000_1 = *(int32_t *)&Blockaa_1->alphabet;
    if ( n0x2000_1 > 0x2000 )
    {
      (__frame.slot[1]) = bmf_new(Blockaa_1->height * k_2 * *(int32_t *)&Blockaa_1->width);
      v26 = *(int32_t *)&Blockaa_1->width;
      n4_2 = Blockaa_1->height;
      __frame.n0x2000_5 = *(int32_t *)&Blockaa_1->width;
      __frame.slot0 = n4_2;
      if ( k_2 )
      {
        __frame.slot6 = __frame.slot0 * v26;
        if ( k_2 >> 1 )
        {
          v28 = (uint8_t *)(__frame.slot[1]) + __frame.slot0 * __frame.n0x2000_5;
          __frame.slot8 = v4;
          __frame.slot9 = k_2;
          __frame.slot7 = (ModelBlock *)((int32_t)Blockaa_1);
          v29 = 0;
          do
          {
            v30 = 2 * v29;
            v31 = 2 * v29++ * __frame.slot6;
            __frame.slot[v30 + 2] = (uint8_t *)(__frame.slot[1]) + v31;
            __frame.slot[v30 + 3] = (void *)&v28[v31];
          }
          while ( v29 < k_2 >> 1 );
          v4 = __frame.slot8;
          k_2 = __frame.slot9;
          Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
          v32 = 2 * v29 + 1;
        }
        else
        {
          v32 = 1;
        }
        if ( k_2 > v32 - 1 )
          __frame.slot[v32 + 1] = (uint8_t *)(__frame.slot[1]) + __frame.slot0 * -__frame.n0x2000_5 + __frame.slot6 * v32;
      }
      else
      {
        __frame.slot6 = __frame.slot0 * v26;
      }
      if ( __frame.slot6 )
      {
        v33 = a3;
        if ( k_2 )
        {
          __frame.slot9 = k_2;
          __frame.slot7 = (ModelBlock *)((int32_t)Blockaa_1);
          __frame.n0x2000_5 = 0;
          k_3 = k_2;
          v39 = 0;
          j_1 = k_3 >> 1;
          while ( 1 )
          {
            while ( 1 )
            {
              if ( j_1 )
              {
                for ( j = 0; j < j_1; ++j )
                {
                  v42 = (uint8_t *)(__frame.slot[2 * j + 2]);
                  *v42 = v33[2 * j];
                  v43 = (uint8_t *)__frame.slot[2 * j + 3];
                  __frame.slot[2 * j + 2] = v42 + 1;
                  *v43 = v33[2 * j + 1];
                  __frame.slot[2 * j + 3] = (void *)(v43 + 1);
                }
                v44 = 2 * j + 1;
                v4 = &v33[2 * j];
              }
              else
              {
                v44 = 1;
              }
              if ( v44 - 1 >= __frame.slot9 )
                break;
              v45 = (uint8_t *)__frame.slot[v44 + 1];
              v4 = &v33[v44];
              *v45 = v33[v44 - 1];
              v46 = ++v39 < __frame.slot6;
              __frame.slot[v44 + 1] = v45 + 1;
              if ( !v46 )
                goto LABEL_71;
              v33 += v44;
            }
            if ( ++v39 >= __frame.slot6 )
              break;
            v33 = v4;
          }
LABEL_71:
          k_2 = __frame.slot9;
          Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
        }
      }
      __frame.v79 = Blockaa_1->sym_word;
      Blockaa_1->height = k_2 * __frame.slot0;
      *(int32_t *)&Blockaa_1->depth = 8;
      free(__frame.v79);
      v34 = bmf_new(2 * Blockaa_1->height * *(int32_t *)&Blockaa_1->width);
      __frame.v79 = (__frame.slot[1]);
      Blockaa_1->sym_word = (uint16_t *)v34;
      __reduce_alphabet((ModelBlock *)Blockaa_1, v35, (uint8_t *)__frame.v79);
      free((__frame.slot[1]));
    }
    else
    {
      if ( 4 * k_2 )
      {
        __frame.slot7 = (ModelBlock *)((int32_t)Blockaa_1);
        v19 = 0;
        do
        {
          __init_symbol_list(&((SymList *)__frame.v86)[v19], v19, 256, 1);
          ++v19;
        }
        while ( v19 < 4 * k_2 );
        Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
        n0x2000_1 = __frame.slot7->alphabet;
      }
      if ( n0x2000_1 )
      {
        v20 = 0;
        n0x2000_4 = 0;
        n0x2000_3 = n0x2000_1;
        Blockaa_4 = (ModelBlock *)((int32_t)Blockaa_1);
        do
        {
          v24 = *(uint32_t *)&__frame.buf[8 * n0x2000_4];
          if ( k_2 )
          {
            __frame.n0x2000_5 = n0x2000_4;
            __frame.slot9 = k_2;
            __frame.slot7 = (ModelBlock *)(Blockaa_4);
            for ( k = 0; k < __frame.slot9; ++k )
            {
              __encode_symbol_list(&((SymList *)__frame.v86)[4 * k + v20], (uint8_t)v24);
              v20 = (uint8_t)v24 >> 6;
              v24 >>= 8;
            }
            n0x2000_4 = __frame.n0x2000_5;
            k_2 = __frame.slot9;
            Blockaa_4 = (ModelBlock *)(__frame.slot7);
            v24 = *(uint32_t *)&__frame.buf[8 * __frame.n0x2000_5];
            n0x2000_3 = __frame.slot7->alphabet;
          }
          v20 = (uint8_t)v24 >> 7;
          ++n0x2000_4;
        }
        while ( n0x2000_4 < n0x2000_3 );
      }
    }
    p_n4 = (int32_t *)__frame.slot;
    n16_2 = 16;
    do
    {
      p_n4 -= 6;
      free((void *)p_n4[5]);
      --n16_2;
    }
    while ( n16_2 );
  }
}

// `n2` is the candidate index 0, 1 or 2 -- the callers pass those three
// literals -- and `a8` is an array of four-word cost records, one per
// candidate.  Both arrived as `uint8_t *` and neither is an address.
int32_t __cost_candidate(uint8_t *a1, int32_t n2, uint8_t *a3, int8_t a4, int32_t a5, int32_t a6, int32_t a7, uint32_t *a8)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and altp1 segfaults while compressing.
  struct alignas(16) CostCandidateFrame {   // 26712 bytes, one stack frame
      uint8_t buf[4096];
      int32_t v72[1024];
      int32_t v73[1024];
      int32_t v74[1024];
      int32_t v75[1024];
      int32_t v76[1024];
      uint8_t buf_1[4];
      uint32_t v78;
      int32_t v79;
      int32_t v80;
      int32_t v81;
      int32_t v82;
      int32_t v83;
      int32_t n4;
      int32_t v85;
      int32_t v86;
      uint8_t _pad0[2008];
      int32_t n2_2;
      uint8_t *v88;
      uint8_t *v89;
      int32_t v90;
      // `v91` held three unrelated `int32_t` here in three phases -- a base
      // address, a pixel difference inside the inner loop, and a cost -- with
      // no overlap between them.  All three are locals now and the slot is
      // dead; it stays to hold the layout.
      int32_t _pad91;
      int32_t v92;
      int32_t v93;
      int32_t v94;
      int32_t v95;
      int32_t n191_5;
      int32_t n191_2;
      uint8_t *v98;
      uint8_t *v99;
      uint8_t *n2_1;
      int32_t v101;
      uint8_t _pad1[28];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 26720, "frame layout moved");
  ;
  int32_t cand_base;   // was `__frame.v91`, phase one: `&v15[n2_3]` as a number
  int32_t cost;        // was `__frame.v91`, phase three: an `estimate_cost`
  int32_t v63;      // a byte offset into `v64`, which is the address
  uint8_t *v64;
  bool v67;
  double v16, v17, v18, v19, v20, v32, n191_1, n191_4;
  int32_t v9, v10, v12, v22, v23, v24, v25, v26, v27, v28, v30, v31, n191,
          n191_3, v37, v39, v40, v41, v45, v46, v47, v48, v49, v50, v51, v52,
          v53, v54, v55, v56, v58, v59, v60, n191_6, n191_7;
  uint32_t v69, v70;
  uint8_t *v15, *v21, *v29, *v42, *n2_4, *v44;
  int32_t n2_3, v65, v66;   // candidate indices, not addresses
  __frame.v101 = (int32_t)(uintptr_t)a3;
  __frame.n2_1 = (uint8_t *)n2;   // the slot is reused as an address below
  __frame.v99 = a1;
  __frame.v88 = a3;
  __frame.n4 = plane_count;
  v9 = *(const uint16_t *)&((const BmfImage *)a1)->stride;   // the low half of the stride
  __frame.n2_2 = n2;
  __frame.v89 = a1;
  __frame.v92 = (int32_t)(__frame.n2_1 + 1) % 3 - (uint32_t)__frame.n2_1;
  __frame.v95 = (int32_t)(__frame.n2_1 + 2) % 3 - (uint32_t)__frame.n2_1;
  __frame.v78 = (uint32_t)&__frame.v99[*((uint32_t *)__frame.v99 + 3) + 16];
  v10 = __frame.v92;
  *(uint32_t *)__frame.buf_1 = v9;
  memset(__frame.buf,0,24576);
  v12 = *(uint32_t *)__frame.buf_1;
  n2_3 = __frame.n2_2;
  v15 = __frame.v89;
  v16 = 0;
  v17 = 0.0;
  v18 = 0.0;
  v19 = 0.0;
  v20 = 0.0;
  __frame.v90 = 16 * __frame.n2_2;
  __frame.v88[16 * __frame.n2_2] = 2;
  __frame.v88[33] = (uint8_t)n2_3;
  cand_base = (int32_t)&v15[n2_3];
  v21 = &v15[n2_3 + 16 + v12 + __frame.n4];
  if ( (uint32_t)v21 < __frame.v78 )
  {
    __frame.v92 = v10;
    *(uint32_t *)__frame.buf_1 = v12;
    __frame.v85 = v10 - v12;
    __frame.v83 = v10 - v12 - __frame.n4;
    __frame.v82 = v10 - __frame.n4;
    __frame.v86 = __frame.v95 - v12;
    __frame.v81 = __frame.v95 - v12 - __frame.n4;
    __frame.v80 = __frame.v95 - __frame.n4;
    __frame.v79 = v12 + __frame.n4;
    do
    {
      v22 = __frame.v95;
      v23 = v21[__frame.v83] + v21[__frame.v92] - (v21[__frame.v85] + v21[__frame.v82]);
      v24 = __frame.v81;
      ++*(uint32_t *)&__frame.buf[4 * v23 + 2048];
      v25 = v21[v24] + v21[v22] - (v21[__frame.v86] + v21[__frame.v80]);
      v26 = __frame.v79;
      v20 = v20 + (double)v23 * (double)v23;
      ++__frame.v72[v25 + 512];
      v17 = v17 + (double)v25 * (double)v25;
      v19 = v19 + (double)v23 * (double)v25;
      ++__frame.v73[((uint16_t)v25 - (uint16_t)v23 - 512) & 0x3FF];
      v27 = v21[-v26] + *v21;
      v28 = v21[-__frame.n4];
      v29 = &v21[-*(uint32_t *)__frame.buf_1];
      v21 += __frame.n4;
      v30 = v27 - (*v29 + v28);
      v18 = v18 + (double)v23 * (double)v30;
      ++__frame.v74[((uint16_t)v30 - (uint16_t)v23 - 512) & 0x3FF];
      v16 = v16 + (double)v25 * (double)v30;
      ++__frame.v75[((uint16_t)v30 - (uint16_t)v25 - 512) & 0x3FF];
      v31 = ((uint16_t)v30 - (uint16_t)((uint32_t)(((v23 + v25) << 6) + 40) >> 7) - 512) & 0x3FF;
      ++__frame.v76[v31];
    }
    while ( (uint32_t)v21 < __frame.v78 );
    v10 = __frame.v92;
  }
  v32 = 128.0 / (0.1 - v19 * v19 + v20 * v17);
  n191_1 = (v17 * v18 - v19 * v16) * v32;
  n191_4 = v32 * (v20 * v16 - v19 * v18);
  n191 = (int32_t)n191_1;
  if ( (int32_t)n191_1 >= 191 )
    n191 = 191;
  if ( n191 < -64 )
    n191 = -64;
  __frame.n191_2 = n191;
  n191_3 = (int32_t)n191_4;
  if ( (int32_t)n191_4 >= 191 )
    n191_3 = 191;
  if ( n191_3 < -64 )
    n191_3 = -64;
  __frame.n191_5 = n191_3;
  memset(__frame.buf_1,0,2048);
  v37 = *((uint16_t *)__frame.v89 + 2);
  __frame.v92 = v10;
  __frame.v94 = ((*((uint16_t *)__frame.v89 + 1) - 1) * *(uint16_t *)__frame.v89) - 1;
  __frame.v93 = -v37;
  v39 = -plane_count;
  v40 = cand_base - (-v37 - plane_count);
  __frame.v101 = -v37 - plane_count;
  __frame.v99 = (uint8_t *)(v40 + 16);
  v41 = v10 + v40 + 16;
  v42 = (uint8_t *)(v40 + 16);
  __frame.v98 = (uint8_t *)v41;
  __frame.n2_1 = (uint8_t *)(__frame.v95 + v40 + 16);
  n2_4 = __frame.n2_1;
  v44 = (uint8_t *)v41;
  do
  {
    v45 = v42[__frame.v101];
    __frame.v98 = v44;
    __frame.v99 = v42;
    __frame.n2_1 = n2_4;
    v46 = v45 + *v42 - v42[__frame.v93] - v42[v39];
    v47 = *v44;
    v48 = __frame.v94;
    n2_4 = &__frame.n2_1[-v39];
    v49 = ((uint16_t)v46
         - (uint16_t)((__frame.n191_2 * (v44[__frame.v101] + v47 - v44[__frame.v93] - v44[v39])
                             + __frame.n191_5 * (__frame.n2_1[__frame.v101] + *__frame.n2_1 - __frame.n2_1[__frame.v93] - (uint32_t)__frame.n2_1[v39])
                             + 40) >> 7)
         - 256)
        & 0x1FF;
    v44 -= v39;
    ++*(uint32_t *)&__frame.buf_1[4 * v49];
    v42 = &__frame.v99[-v39];
    __frame.v94 = v48 - 1;
  }
  while ( v48 != 1 );
  v50 = __frame.v92;
  v51 = __estimate_cost((uint8_t *)__frame.buf_1, 512);
  v52 = 4 * __frame.n2_2;   // this candidate's four-word record
  a8[v52] = v51;
  a8[v52 + 1] = __estimate_cost((uint8_t *)__frame.v74, 1024);
  a8[v52 + 2] = __estimate_cost((uint8_t *)__frame.v75, 1024);
  a8[v52 + 3] = __estimate_cost((uint8_t *)__frame.v76, 1024);
  cost = __estimate_cost((uint8_t *)__frame.buf, 1024);
  __frame.v92 = __estimate_cost((uint8_t *)__frame.v72, 1024);
  v53 = __estimate_cost((uint8_t *)__frame.v73, 1024);
  __frame.v94 = v53;
  v54 = v53;
  if ( cost < v53 )
    v54 = cost;
  if ( __frame.v92 < v53 )
    v53 = __frame.v92;
  v55 = __frame.v92 + v54;
  v56 = cost + v53;
  __frame.v93 = v56;
  v58 = __frame.v94;
  if ( (v55 < v56) )
  {
    __frame.v93 = v55;
    v59 = v50;
    v50 = __frame.v95;
    cost = __frame.v92;
    __frame.v95 = v59;
    v60 = a8[v52 + 1];
    a8[v52 + 1] = a8[v52 + 2];
    n191_6 = __frame.n191_5;
    a8[v52 + 2] = v60;
    n191_7 = __frame.n191_2;
    __frame.n191_2 = n191_6;
    __frame.n191_5 = n191_7;
  }
  v63 = __frame.v90;
  v64 = __frame.v88;
  *(uint32_t *)&__frame.v88[__frame.v90 + 4] = __frame.n191_2;
  *(uint32_t *)&v64[v63 + 8] = __frame.n191_5;
  v65 = __frame.n2_2 + v50;
  v66 = __frame.n2_2 + __frame.v95;
  v64[16 * v65] = 0;
  v64[1] = (uint8_t)v65;
  v67 = 0;                            // -S
  v64[16 * v66] = 1;
  v64[17] = (uint8_t)v66;
  if ( !v67 && *((uint32_t *)__frame.v89 + 3) > 0x1000000u )
    return cost + v58 + a8[v52];
  v69 = a8[v52];
  v70 = a8[v52 + 2];
  if ( v69 >= a8[v52 + 1] )
    v69 = a8[v52 + 1];
  if ( v70 >= a8[v52 + 3] )
    v70 = a8[v52 + 3];
  if ( v69 < v70 )
    v70 = v69;
  return __frame.v93 + v70;
}


int32_t __choose_plane_coding(BmfImage *a1, int32_t n3, int8_t a3)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and altp1 segfaults while compressing.
  struct alignas(16) ChoosePlaneCodingFrame {   // 41456 bytes, one stack frame
      int32_t v174;
      int32_t v175;
      int32_t v176;
      uint8_t _pad0[4];
      // 32 KiB, which is what `memset(buf, 0, 0x8000)` says and what
      // `buf[4096 * n2_1]` walks; `char buf[4]` was the first four bytes of it.
      union {
          uint8_t buf[32768];
          struct {
            uint8_t _buf_head[4096];
            int32_t v178[1024];
            int32_t v179[1024];
            int32_t v180[5120];
          };
      };
      uint8_t buf_3[2048];   // `memset(buf_3, 0, 2048)`, 512 counters
      // The same 2 KiB twice: `buf_1` is 512 counters while the seventeen
      // locals below are dead, and they are live while it is not.
      union {
          uint8_t buf_1[2048];
          struct {
            uint8_t _buf_1_head[4];
            int32_t v183;
            int32_t v184;
            int32_t v185;
            uint8_t *v186;
            uint8_t *v187;
            int32_t v188;
            int32_t v189;
            uint8_t *v190;
            int32_t v191;
            int32_t v192;
            int32_t v193;
            uint8_t *v194;
            uint8_t *v195;
            int32_t v196;
            int32_t v197;
            uint8_t *v198;
            int32_t v199;
            uint8_t _pad3[1976];
          };
      };
      uint8_t buf_4[2048];   // `memset(buf_4, 0, 2048)`, 512 counters
      uint8_t buf_2[2048];   // `memset(buf_2, 0, 2048)`, 512 counters
      // Six sixteen-byte spill slots -- XMM0..XMM5 as the original used them.
      // The body reads each 157 times as four `int32_t`, 48 times as the two
      // `double` halves the 3x3 solve below works in, and four times as an
      // `int16_t` lane.  The declaration is the reading that is an array; the
      // other two say their width at the site, which is the only way to keep
      // a 64-bit access from turning into a 32-bit one.
      alignas(16) int32_t v202[4];
      alignas(16) int32_t v203[4];
      alignas(16) int32_t v204[4];
      alignas(16) int32_t v205[4];
      alignas(16) int32_t v206[4];
      alignas(16) int32_t v207[4];
      int64_t v208;
      int64_t v209;
      double v210;
      double v211;
      double v212;
      double v213;
      int32_t v214[4];
      uint64_t v215[5];
      double v216;
      int32_t v217[16];
      uint8_t v218[64];
      uint8_t v219[64];
      int64_t v220;
      double v221;
      double v222;
      uint8_t *v223;
      uint32_t v224;
      int32_t __choose_plane_coding_n191_1;
      BmfImage *v226;
      uint32_t v227;
      uint8_t _pad6[4];
      BmfImage *v228;
      uint8_t _pad7[28];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 41456, "frame layout moved");
  ;
  bool v19, n2_4, v42, v106;
  int8_t v7, v16, v18, n0x100_1, k;
  uint8_t v10, v12;
  uint8_t *v44;   // `uint8_t *` beside the `char` scalars above
  double v65, v66, v68, v69, v70, v72, v73, v74, v75, v76;
  int16_t v92;
  int32_t n4, data_size, n192, v11, v13, v14, n2_3, n2, n16, *v25, n128_1, n128, v30,
          v31, v32, v33, v34, v35, n0x4000, n2_1, v43, n0x100, v48, v49, i,
          n255, j_1, j, v57, n128_2, v59, n128_3, v61, v62, v63, v71,
          __choose_plane_coding_n191, n191_2, n191_3, v81, v82, v83, v84, v85,
          v86, v87, v88, v89, v90, v91, v93, v94, v95, v96, v98, v99, v100,
          v101, v102, n191_4, n2_2, __choose_plane_coding_n3_1, *v112, v115,
          v116, v118, v119, v120, v124, v125, v126, v127, v128, v130, v131,
          v132, v133, v137, v138, v139, v140, v141, v143, n192_1, n192_2,
          n192_4, n192_3, v148, v149, v150, v152, v154, v155, v156, v157,
          v158, n192_5, v161, v162, v163, v165, v167, v168, v169, v170, v171,
          n192_6;
  uint32_t v9, v15, v17, v26, v38, v39, v67, v80, v103, v104, v105, v108,
           v110, v129, v142, v159, v172;
  uint8_t *v29, *v64, *v97, *v121, *v122, *v123, *v134, *v135, *v136, *v151, *v153, *v164, *v166;
  __frame.v228 = (BmfImage *)(a1);
  n4 = plane_count;
  data_size = a1->data_size;
  __frame.v226 = (BmfImage *)((uint8_t *)a1);
  alphabet_reduced = 1;
  LODWORD(__frame.v208) = ((uint16_t)a1->stride);
  __frame.v227 = (uintptr_t)&a1->pixels[data_size];
  memset(__frame.buf,0,0x8000);
  n192 = 192;
  do
  {
    bmf_zero16(((uint8_t *)&__frame.v215[4] + n192));
    bmf_zero16(((uint8_t *)&__frame.v215[2] + n192));
    bmf_zero16(((uint8_t *)__frame.v215 + n192));
    bmf_zero16(((uint8_t *)__frame.v214 + n192));
    n192 -= 64;
  }
  while ( n192 );
  if ( n4 > 0 )
  {
    if ( n4 / 2 )
    {
      v9 = 0;
      do
      {
        v10 = 2 * v9;
        v11 = 2 * v9;   // a record index; it was 32 * v9, two records' worth
        plane_desc[v11 + 1].src_plane = 2 * v9;
        v12 = 2 * v9++ + 1;
        plane_desc[v11 + 1].predictor = v10;
        plane_desc[v11 + 2].src_plane = v12;
        plane_desc[v11 + 2].predictor = v12;
      }
      while ( v9 < n4 / 2 );
      v13 = 2 * v9 + 1;
    }
    else
    {
      v13 = 1;
    }
    n192 = v13 - 1;
    if ( n4 > (uint32_t)(v13 - 1) )
    {
      v14 = v13;   // a record index; it was 4 * v13, a dword index into record 0
      plane_desc[v14].src_plane = n192;
      plane_desc[v14].predictor = n192;
    }
    if ( n4 >= 3 )
    {
      v15 = __cost_candidate((uint8_t *)__frame.v226, 0, (uint8_t *)__frame.v217, v7, __frame.v174, __frame.v175, __frame.v176, (uint32_t *)&__frame.v214[2]);
      v17 = __cost_candidate((uint8_t *)__frame.v226, 1, __frame.v218, v16, __frame.v174, __frame.v175, __frame.v176, (uint32_t *)&__frame.v214[2]);
      v19 = v17 >= v15;
      if ( v17 >= v15 )
        v17 = v15;
      n2_3 = !v19;
      __frame.v205[2] = n2_3;
      n2_4 = __cost_candidate((uint8_t *)__frame.v226, 2, __frame.v219, v18, __frame.v174, __frame.v175, __frame.v176, (uint32_t *)&__frame.v214[2]) < v17;
      n2 = n2_3;
      if ( n2_4 )
        n2 = 2;
      __frame.v205[2] = n2;
      n16 = 16;
      v25 = &__frame.v217[16 * n2];
      do
      {
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 8)) = *(uint64_t *)&v25[n16 - 2];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 16)) = *(uint64_t *)&v25[n16 - 4];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 24)) = *(uint64_t *)&v25[n16 - 6];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 32)) = *(uint64_t *)&v25[n16 - 8];
        n16 -= 8;
      }
      while ( n16 * 4 );
      v26 = __frame.v214[4 * __frame.v205[2] + 2];
      __frame.v205[1] = 16 * __frame.v205[2];
      LODWORD(__frame.v209) = plane_desc[1].src_plane - __frame.v205[2];
      n128_1 = plane_desc[__frame.v205[2] + 1].w8;
      HIDWORD(__frame.v208) = plane_desc[2].src_plane - __frame.v205[2];
      n128 = plane_desc[__frame.v205[2] + 1].w4;
      // always taken: -S is on.  (The block is kept braced -- LABEL_19 below
      // is jumped to from inside it.)
      {
        LODWORD(__frame.v210) = n128_1;
        __frame.v207[3] = n128;
        HIDWORD(__frame.v209) = n4;
        __frame.v202[1] = 4;            // (opt_search_quality + 5) / 3, and -Q is 9
        __frame.v184 = n128 - 1;
        v57 = n128 - 1;
        __frame.v192 = n128_1 - 1;
        n128_2 = n128;
        v59 = n128_1 - 1;
        __frame.v183 = n128_2 - __frame.v202[1];
        n128_3 = n128_1;
        v61 = __frame.v183;
        *(uint32_t *)__frame.buf_1 = n128_3 - __frame.v202[1];
        __frame.v202[0] = (int32_t)&((uint8_t *)__frame.v226)[__frame.v205[2]];
        v62 = n128_3 - __frame.v202[1];
        while ( 1 )
        {
          if ( v57 <= v61 )
          {
            if ( v59 <= v62 )
            {
              __frame.v203[0] = __frame.v207[3] + 1;
              n192_1 = __frame.v207[3] + 1;
              __frame.v205[3] = LODWORD(__frame.v210) + 1;
              n192_2 = LODWORD(__frame.v210) + 1;
              __frame.v202[3] = __frame.v207[3] + __frame.v202[1];
              n192_4 = __frame.v207[3] + __frame.v202[1];
              __frame.v202[2] = LODWORD(__frame.v210) + __frame.v202[1];
              n192_3 = LODWORD(__frame.v210) + __frame.v202[1];
              while ( 1 )
              {
                if ( n192_1 >= n192_4 )
                {
                  if ( n192_2 >= n192_3 )
                  {
                    n128_1 = LODWORD(__frame.v210);
                    n128 = __frame.v207[3];
                    n4 = HIDWORD(__frame.v209);
                    goto LABEL_19;
                  }
                  if ( n192_1 >= n192_4 )
                    goto LABEL_109;
                }
                if ( n192_1 < 192 )
                {
                  __frame.v202[2] = n192_3;
                  memset(__frame.buf_1,0,2048);
                  v148 = __frame.v226->width * (__frame.v226->height - 1);
                  v149 = (uint16_t)__frame.v226->stride;
                  __frame.v205[3] = n192_2;
                  __frame.v203[0] = n192_1;
                  __frame.v203[1] = v148 - 1;
                  __frame.v205[0] = v26;
                  __frame.v204[1] = -v149;
                  v150 = -plane_count;
                  __frame.v204[0] = -v149 - plane_count;
                  __frame.v203[3] = __frame.v202[0] - __frame.v204[0] + 16;
                  __frame.v203[2] = __frame.v209 + __frame.v202[0] - __frame.v204[0] + 16;
                  v151 = (uint8_t *)__frame.v203[2];
                  v152 = HIDWORD(__frame.v208) + __frame.v202[0] - __frame.v204[0] + 16;
                  v153 = (uint8_t *)__frame.v203[3];
                  do
                  {
                    v154 = v153[__frame.v204[0]];
                    *(int64_t *)&__frame.v203[2] = __PAIR64__((uint32_t)v153, (uint32_t)v151);
                    __frame.v204[2] = v152;
                    v155 = v154 + *v153 - v153[__frame.v204[1]] - v153[v150];
                    v156 = *v151;
                    __frame.v204[3] = v155;
                    v152 = __frame.v204[2] - v150;
                    v157 = __frame.v203[1];
                    v158 = (((int16_t *)__frame.v204)[6]
                          - (uint16_t)((__frame.v203[0]
                                              * (v151[__frame.v204[0]] + v156 - v151[__frame.v204[1]] - v151[v150])
                                              + LODWORD(__frame.v210)
                                              * (*(uint8_t *)(__frame.v204[2] + __frame.v204[0])
                                               + *(uint8_t *)__frame.v204[2]
                                               - *(uint8_t *)(__frame.v204[2] + __frame.v204[1])
                                               - (uint32_t)*(uint8_t *)(__frame.v204[2] + v150))
                                              + 40) >> 7)
                          - 256)
                         & 0x1FF;
                    ++*(uint32_t *)&__frame.buf_1[4 * v158];
                    v151 -= v150;
                    v153 = (uint8_t *)(__frame.v203[3] - v150);
                    __frame.v203[1] = v157 - 1;
                  }
                  while ( v157 != 1 );
                  n192_2 = __frame.v205[3];
                  n192_1 = __frame.v203[0];
                  v26 = __frame.v205[0];
                  v159 = __estimate_cost((uint8_t *)__frame.buf_1, 512);
                  n192_3 = __frame.v202[2];
                  n192_5 = __frame.v207[3];
                  if ( v159 < v26 )
                  {
                    v26 = v159;
                    n192_5 = n192_1;
                  }
                  __frame.v207[3] = n192_5;
                  n192_4 = __frame.v202[1] + n192_5;
                }
                if ( n192_2 < n192_3 )
                {
LABEL_109:
                  if ( n192_2 < 192 )
                  {
                    __frame.v202[3] = n192_4;
                    memset(__frame.buf_2,0,2048);
                    v161 = __frame.v226->width * (__frame.v226->height - 1);
                    v162 = (uint16_t)__frame.v226->stride;
                    __frame.v205[3] = n192_2;
                    __frame.v203[0] = n192_1;
                    __frame.v206[0] = v161 - 1;
                    __frame.v205[0] = v26;
                    __frame.v207[0] = -v162;
                    v163 = -plane_count;
                    __frame.v206[3] = -v162 - plane_count;
                    __frame.v206[2] = __frame.v202[0] - __frame.v206[3] + 16;
                    v164 = (uint8_t *)(__frame.v209 + __frame.v202[0] - __frame.v206[3] + 16);
                    __frame.v206[1] = (int32_t)v164;
                    v165 = HIDWORD(__frame.v208) + __frame.v202[0] - __frame.v206[3] + 16;
                    v166 = (uint8_t *)__frame.v206[2];
                    do
                    {
                      v167 = v166[__frame.v206[3]];
                      *(int64_t *)((uint8_t *)__frame.v206 + 4) = __PAIR64__((uint32_t)v166, (uint32_t)v164);
                      __frame.v207[1] = v165;
                      v168 = v167 + *v166 - v166[__frame.v207[0]] - v166[v163];
                      v169 = *v164;
                      __frame.v207[2] = v168;
                      v165 = __frame.v207[1] - v163;
                      v170 = __frame.v206[0];
                      v171 = (((int16_t *)__frame.v207)[4]
                            - (uint16_t)((__frame.v207[3]
                                                * (v164[__frame.v206[3]] + v169 - v164[__frame.v207[0]] - v164[v163])
                                                + __frame.v205[3]
                                                * (*(uint8_t *)(__frame.v207[1] + __frame.v206[3])
                                                 + *(uint8_t *)__frame.v207[1]
                                                 - *(uint8_t *)(__frame.v207[1] + __frame.v207[0])
                                                 - (uint32_t)*(uint8_t *)(__frame.v207[1] + v163))
                                                + 40) >> 7)
                            - 256)
                           & 0x1FF;
                      ++*(uint32_t *)&__frame.buf_2[4 * v171];
                      v164 -= v163;
                      v166 = (uint8_t *)(__frame.v206[2] - v163);
                      __frame.v206[0] = v170 - 1;
                    }
                    while ( v170 != 1 );
                    n192_2 = __frame.v205[3];
                    n192_1 = __frame.v203[0];
                    v26 = __frame.v205[0];
                    v172 = __estimate_cost((uint8_t *)__frame.buf_2, 512);
                    n192_4 = __frame.v202[3];
                    n192_6 = LODWORD(__frame.v210);
                    if ( v172 < v26 )
                    {
                      v26 = v172;
                      n192_6 = n192_2;
                    }
                    LODWORD(__frame.v210) = n192_6;
                    n192_3 = __frame.v202[1] + n192_6;
                  }
                }
                ++n192_1;
                ++n192_2;
              }
            }
            if ( v57 <= v61 )
              goto LABEL_55;
          }
          if ( v57 >= -64 )
          {
            *(uint32_t *)__frame.buf_1 = v62;
            memset(__frame.buf_3,0,2048);
            v118 = __frame.v226->width * (__frame.v226->height - 1);
            v119 = (uint16_t)__frame.v226->stride;
            __frame.v192 = v59;
            __frame.v184 = v57;
            __frame.v185 = v118 - 1;
            __frame.v205[0] = v26;
            __frame.v189 = -v119;
            v120 = -plane_count;
            __frame.v188 = -v119 - plane_count;
            __frame.v187 = (uint8_t *)(__frame.v202[0] - __frame.v188 + 16);
            __frame.v186 = (uint8_t *)(__frame.v209 + __frame.v202[0] - __frame.v188 + 16);
            v121 = __frame.v186;
            v122 = (uint8_t *)(HIDWORD(__frame.v208) + __frame.v202[0] - __frame.v188 + 16);
            v123 = __frame.v187;
            do
            {
              v124 = v123[__frame.v188];
              __frame.v186 = v121;
              __frame.v187 = v123;
              __frame.v190 = v122;
              v125 = v124 + *v123 - v123[__frame.v189] - v123[v120];
              v126 = *v121;
              __frame.v191 = v125;
              v122 = &__frame.v190[-v120];
              v127 = __frame.v185;
              v128 = ((uint16_t)__frame.v191
                    - (uint16_t)((__frame.v184 * (v121[__frame.v188] + v126 - v121[__frame.v189] - v121[v120])
                                        + LODWORD(__frame.v210) * (__frame.v190[__frame.v188] + *__frame.v190 - __frame.v190[__frame.v189] - (uint32_t)__frame.v190[v120])
                                        + 40) >> 7)
                    - 256)
                   & 0x1FF;
              ++*(uint32_t *)&__frame.buf_3[4 * v128];
              v121 -= v120;
              v123 = &__frame.v187[-v120];
              __frame.v185 = v127 - 1;
            }
            while ( v127 != 1 );
            v59 = __frame.v192;
            v57 = __frame.v184;
            v26 = __frame.v205[0];
            v129 = __estimate_cost((uint8_t *)__frame.buf_3, 512);
            v62 = *(uint32_t *)__frame.buf_1;
            v130 = __frame.v207[3];
            if ( v129 < v26 )
            {
              v26 = v129;
              v130 = v57;
            }
            __frame.v207[3] = v130;
            v61 = v130 - __frame.v202[1];
          }
          if ( v59 > v62 )
          {
LABEL_55:
            if ( v59 >= -64 )
            {
              __frame.v183 = v61;
              memset(__frame.buf_4,0,2048);
              v131 = __frame.v226->width * (__frame.v226->height - 1);
              v132 = (uint16_t)__frame.v226->stride;
              __frame.v192 = v59;
              __frame.v184 = v57;
              __frame.v193 = v131 - 1;
              __frame.v205[0] = v26;
              __frame.v197 = -v132;
              v133 = -plane_count;
              __frame.v196 = -v132 - plane_count;
              __frame.v195 = (uint8_t *)(__frame.v202[0] - __frame.v196 + 16);
              v134 = (uint8_t *)(__frame.v209 + __frame.v202[0] - __frame.v196 + 16);
              __frame.v194 = v134;
              v135 = (uint8_t *)(HIDWORD(__frame.v208) + __frame.v202[0] - __frame.v196 + 16);
              v136 = __frame.v195;
              do
              {
                v137 = v136[__frame.v196];
                __frame.v194 = v134;
                __frame.v195 = v136;
                __frame.v198 = v135;
                v138 = v137 + *v136 - v136[__frame.v197] - v136[v133];
                v139 = *v134;
                __frame.v199 = v138;
                v135 = &__frame.v198[-v133];
                v140 = __frame.v193;
                v141 = ((uint16_t)__frame.v199
                      - (uint16_t)((__frame.v207[3] * (v134[__frame.v196] + v139 - v134[__frame.v197] - v134[v133])
                                          + __frame.v192 * (__frame.v198[__frame.v196] + *__frame.v198 - __frame.v198[__frame.v197] - (uint32_t)__frame.v198[v133])
                                          + 40) >> 7)
                      - 256)
                     & 0x1FF;
                ++*(uint32_t *)&__frame.buf_4[4 * v141];
                v134 -= v133;
                v136 = &__frame.v195[-v133];
                __frame.v193 = v140 - 1;
              }
              while ( v140 != 1 );
              v59 = __frame.v192;
              v57 = __frame.v184;
              v26 = __frame.v205[0];
              v142 = __estimate_cost((uint8_t *)__frame.buf_4, 512);
              v61 = __frame.v183;
              v143 = LODWORD(__frame.v210);
              if ( v142 < v26 )
              {
                v26 = v142;
                v143 = v59;
              }
              LODWORD(__frame.v210) = v143;
              v62 = v143 - __frame.v202[1];
            }
          }
          --v57;
          --v59;
        }
      }
LABEL_19:
      v29 = &((uint8_t *)__frame.v226)[__frame.v208 + 16 + n4 + __frame.v205[2]];
      if ( (uint32_t)v29 < __frame.v227 )
      {
        LODWORD(__frame.v210) = n128_1;
        __frame.v207[3] = n128;
        __frame.v205[0] = v26;
        HIDWORD(__frame.v209) = n4;
        do
        {
          v30 = v29[HIDWORD(__frame.v208)];
          v31 = v30 * LODWORD(__frame.v210);
          v32 = v29[__frame.v209];
          v33 = v32 * __frame.v207[3];
          ++__frame.v180[v30 - v32 + 1280];
          v34 = *v29 + 512;
          v29 += HIDWORD(__frame.v209);
          v35 = ((uint16_t)v34 - (uint16_t)((uint32_t)(v31 + v33 + 40) >> 7)) & 0x3FF;
          ++*(uint32_t *)&__frame.buf[4 * v35];
          ++__frame.v178[v34 - v32];
          ++__frame.v179[v34 - v30];
          ++__frame.v180[(v34 - ((uint32_t)(((v32 + v30) << 6) + 40) >> 7))];
        }
        while ( (uint32_t)v29 < __frame.v227 );
        n128_1 = LODWORD(__frame.v210);
        n128 = __frame.v207[3];
        v26 = __frame.v205[0];
        n4 = HIDWORD(__frame.v209);
      }
      n0x4000 = v26 >> 7;
      if ( v26 >> 7 >= 0x4000 )
        n0x4000 = 0x4000;
      v38 = n0x4000 + v26;
      v39 = *(int32_t *)((uint8_t *)&__frame.v214[3] + __frame.v205[1]);
      n2_4 = v39 < v38;
      if ( v39 < v38 )
      {
        v38 = *(int32_t *)((uint8_t *)&__frame.v214[3] + __frame.v205[1]);
        n128 = 128;
        n128_1 = 0;
      }
      n2_1 = n2_4;
      if ( *(uint32_t *)((uint8_t *)__frame.v215 + __frame.v205[1]) < v38 )
      {
        v38 = *(uint32_t *)((uint8_t *)__frame.v215 + __frame.v205[1]);
        n2_1 = 2;
        n128 = 0;
        n128_1 = 128;
      }
      v42 = v38 <= *(uint32_t *)((uint8_t *)__frame.v215 + __frame.v205[1] + 4);
      if ( v38 > *(uint32_t *)((uint8_t *)__frame.v215 + __frame.v205[1] + 4) )
        n2_1 = 3;
      v43 = __frame.v205[2];   // a record index; it was the byte offset 16 * it
      if ( !v42 )
      {
        n128 = 64;
        n128_1 = 64;
      }
      plane_desc[__frame.v205[2] + 1].w4 = n128;
      plane_desc[v43 + 1].w8 = n128_1;
      v44 = &__frame.buf[4096 * n2_1];
      n0x100 = (uint8_t)(uintptr_t)v44 & 0xF;
      // The first 256-wide window over these 1024 counters.  Where it starts
      // is the pointer's low four bits, which the frame's alignas(16) makes
      // zero; sixteen counters an iteration is all the vectors were doing,
      // and integer addition does not care which lane a term landed in.
      v48 = 0;
      for ( i = 0; i < 256; ++i )
        v48 += *(int32_t *)&v44[4 * (n0x100 + i)];
      n0x100 += 256;
      v49 = v48;
      n0x100_1 = -1;
      if ( n0x100 < 1024 )
      {
        HIDWORD(__frame.v209) = n4;
        do
        {
          v48 = *(uint32_t *)&v44[4 * n0x100] + v48 - *(uint32_t *)&v44[4 * n0x100 - 1024];
          if ( v48 >= v49 )
          {
            n0x100_1 = n0x100;
            v49 = v48;
          }
          ++n0x100;
        }
        while ( n0x100 < 1024 );
        n4 = HIDWORD(__frame.v209);
      }
      plane_desc[__frame.v205[2] + 1].b3 = n0x100_1 + 1;
      // Same window, over the second table.  `i` is left at 256 for the slide
      // that follows.
      j_1 = 0;
      for ( i = 0; i < 0x100; ++i )
        j_1 += __frame.v180[i + 1024];
      n255 = 255;
      for ( j = j_1; i < 512; ++i )
      {
        j = __frame.v180[i + 1024] + j - __frame.v180[i + 768];
        if ( j >= j_1 )
        {
          n255 = i;
          j_1 = j;
        }
      }
      n192 = n255 + 1;
      // `m128_i32[1]` is `16 * m128_i32[2]`, a byte offset, so the record index
      // is `[2]`.  This was `__byte_44339F[16 * HIDWORD(v208) + v205[1]]`
      // and the fold that made it a record access divided the first term by 16
      // and not the second.  The chosen transform is 0 on all fifteen reference
      // images -- both spellings agree there, which is why the gate stayed green
      // -- but it is 1 or 2 for an image that picks another one.
      plane_desc[HIDWORD(__frame.v208) + __frame.v205[2] + 1].b3 = n192;
      if ( n4 >= 4 )
      {
        __builtin_memset(__frame.v202, 0, 16);
        __builtin_memset(__frame.v203, 0, 16);
        __builtin_memset(__frame.v204, 0, 16);
        __builtin_memset(__frame.v205, 0, 16);
        __builtin_memset(__frame.v206, 0, 16);
        __builtin_memset(__frame.v207, 0, 16);
        memset(__frame.buf,0,0x8000);
        __frame.v223 = &((uint8_t *)__frame.v226)[__frame.v208];
        v63 = (int32_t)(__frame.v227 - 17 - (uint32_t)&((uint8_t *)__frame.v226)[__frame.v208]) / 4;
        v64 = &((uint8_t *)__frame.v226)[__frame.v208 + 20];
        if ( __frame.v227 <= (uint32_t)v64 )
        {
          v72 = *(double *)&__frame.v202[2];
          v73 = *(double *)__frame.v203;
          v74 = *(double *)&__frame.v204[2];
          __frame.v209 = *(int64_t *)&__frame.v202[2];
          __frame.v208 = *(int64_t *)&__frame.v204[2];
          __frame.v216 = *(double *)__frame.v202;
          __frame.v220 = *(int64_t *)&__frame.v206[2];
          v75 = *(double *)__frame.v203;
        }
        else
        {
          v65 = *(double *)__frame.v206;
          v66 = *(double *)__frame.v204;
          __frame.v220 = *(int64_t *)&__frame.v206[2];
          __frame.v216 = *(double *)__frame.v202;
          __frame.v221 = *(double *)&__frame.v202[2];
          __frame.v212 = *(double *)__frame.v203;
          __frame.v222 = *(double *)&__frame.v204[2];
          __frame.v213 = *(double *)__frame.v207;
          __frame.v211 = *(double *)&__frame.v207[2];
          LODWORD(__frame.v210) = (uintptr_t)&((uint8_t *)__frame.v226)[__frame.v208 + 20];
          v67 = 0;
          __frame.v224 = (int32_t)(__frame.v227 - 17 - (uint32_t)&((uint8_t *)__frame.v226)[__frame.v208]) / 4;
          do
          {
            v68 = (double)(((uint8_t *)__frame.v226)[4 * v67 + 16] + __frame.v223[4 * v67 + 20] - (((uint8_t *)__frame.v226)[4 * v67 + 20] + __frame.v223[4 * v67 + 16]));
            v69 = (double)(((uint8_t *)__frame.v226)[4 * v67 + 17] + __frame.v223[4 * v67 + 21] - (((uint8_t *)__frame.v226)[4 * v67 + 21] + __frame.v223[4 * v67 + 17]));
            v70 = (double)(((uint8_t *)__frame.v226)[4 * v67 + 18] + __frame.v223[4 * v67 + 22] - (((uint8_t *)__frame.v226)[4 * v67 + 22] + __frame.v223[4 * v67 + 18]));
            v71 = ((uint8_t *)__frame.v226)[4 * v67 + 19] + __frame.v223[4 * v67 + 23] - (((uint8_t *)__frame.v226)[4 * v67 + 23] + __frame.v223[4 * v67 + 19]);
            __frame.v216 = __frame.v216 + v68 * v68;
            ++v67;
            __frame.v221 = __frame.v221 + v68 * v69;
            __frame.v212 = __frame.v212 + v68 * v70;
            v66 = v66 + v69 * v69;
            __frame.v222 = __frame.v222 + v69 * v70;
            v65 = v65 + v70 * v70;
            *(double *)&__frame.v220 = *(double *)&__frame.v220 + v68 * (double)v71;
            __frame.v213 = __frame.v213 + v69 * (double)v71;
            __frame.v211 = __frame.v211 + v70 * (double)v71;
          }
          while ( v67 < __frame.v224 );
          v72 = __frame.v221;
          v73 = __frame.v212;
          v74 = __frame.v222;
          v64 = (uint8_t *)LODWORD(__frame.v210);
          v63 = __frame.v224;
          *(double *)&__frame.v207[2] = __frame.v211;
          *(double *)__frame.v207 = __frame.v213;
          *(int64_t *)&__frame.v206[2] = __frame.v220;
          *(double *)__frame.v206 = v65;
          *(double *)&__frame.v204[2] = __frame.v222;
          *(double *)__frame.v204 = v66;
          *(double *)__frame.v203 = __frame.v212;
          *(double *)&__frame.v202[2] = __frame.v221;
          *(double *)__frame.v202 = __frame.v216;
          v75 = __frame.v212;
          *(double *)&__frame.v208 = __frame.v222;
          *(double *)&__frame.v209 = __frame.v221;
        }
        __frame.v221 = v72;
        __frame.v222 = v74;
        __frame.v210 = v75;
        *(double *)&__frame.v203[2] = v72;
        *(double *)__frame.v205 = v73;
        *(double *)&__frame.v205[2] = v74;
        __frame.v211 = __frame.v216 * *(double *)__frame.v206 - v75 * v73;
        __frame.v212 = 0.0 - __frame.v216 * v74 + v73 * *(double *)&__frame.v209;
        __frame.v213 = v75 * v74 - *(double *)&__frame.v209 * *(double *)__frame.v206;
        v76 = 128.0 / (*(double *)__frame.v204 * __frame.v211 + *(double *)&__frame.v208 * __frame.v212 + v72 * __frame.v213 + 0.1);
        *(double *)__frame.v214 = v76;
        __choose_plane_coding_n191 = (int32_t)(((v73 * *(double *)&__frame.v208 - v72 * *(double *)__frame.v206) * *(double *)__frame.v207
                    + (0.0 - v73 * *(double *)__frame.v204 + v72 * __frame.v222) * *(double *)&__frame.v207[2]
                    + (*(double *)__frame.v206 * *(double *)__frame.v204 - __frame.v222 * *(double *)&__frame.v208)
                    * *(double *)&__frame.v220)
                   * v76);
        if ( __choose_plane_coding_n191 >= 191 )
          __choose_plane_coding_n191 = 191;
        if ( __choose_plane_coding_n191 < -64 )
          __choose_plane_coding_n191 = -64;
        __frame.__choose_plane_coding_n191_1 = __choose_plane_coding_n191;
        n191_2 = (int32_t)((__frame.v211 * *(double *)__frame.v207 + __frame.v212 * *(double *)&__frame.v207[2]
                                                         + __frame.v213 * *(double *)&__frame.v220)
                     * v76);
        if ( n191_2 >= 191 )
          n191_2 = 191;
        if ( n191_2 < -64 )
          n191_2 = -64;
        n191_3 = (int32_t)(*(double *)__frame.v214
                     * ((__frame.v216 * *(double *)&__frame.v207[2] - __frame.v210 * *(double *)&__frame.v220) * *(double *)__frame.v204
                      + (0.0 - __frame.v216 * *(double *)__frame.v207 + *(double *)&__frame.v220 * *(double *)&__frame.v209)
                      * *(double *)&__frame.v208
                      + (__frame.v210 * *(double *)__frame.v207 - *(double *)&__frame.v209 * *(double *)&__frame.v207[2]) * __frame.v221));
        if ( n191_3 >= 191 )
          n191_3 = 191;
        if ( n191_3 < -64 )
          n191_3 = -64;
        if ( (uint32_t)v64 < __frame.v227 )
        {
          *(int64_t *)__frame.v202 = __PAIR64__(n191_3, n191_2);
          __frame.v224 = v63;
          v80 = 0;
          do
          {
            v81 = ((uint8_t *)__frame.v226)[4 * v80 + 20];
            v82 = ((uint8_t *)__frame.v226)[4 * v80 + 16] + __frame.v223[4 * v80 + 20];
            v83 = __frame.v223[4 * v80 + 16];
            __frame.v202[2] = v80;
            v84 = v81 + v83;
            v85 = __frame.v223[4 * v80 + 21];
            v86 = v82 - v84;
            v87 = __frame.v223[4 * v80 + 17];
            __frame.v202[3] = v86;
            v88 = ((uint8_t *)__frame.v226)[4 * v80 + 22];
            v89 = ((uint8_t *)__frame.v226)[4 * v80 + 17] + v85 - (((uint8_t *)__frame.v226)[4 * v80 + 21] + v87);
            v90 = __frame.v223[4 * v80 + 22];
            __frame.v203[0] = v89;
            v91 = ((uint8_t *)__frame.v226)[4 * v80 + 18] + v90 - (v88 + __frame.v223[4 * v80 + 18]);
            LOWORD(v89) = ((uint8_t *)__frame.v226)[4 * v80 + 19] + __frame.v223[4 * v80 + 23] - __frame.v223[4 * v80 + 19] - ((uint8_t *)__frame.v226)[4 * v80 + 23];
            v92 = ((int16_t *)__frame.v203)[0];
            LOWORD(v89) = v89 - 512;
            v93 = __frame.v203[0] * __frame.v202[0] + __frame.v202[3] * __frame.__choose_plane_coding_n191_1;
            v94 = v91 * __frame.v202[1];
            ++__frame.v178[((uint16_t)v89 - ((int16_t *)__frame.v202)[6]) & 0x3FF];
            v95 = ((uint16_t)v89 - (uint16_t)((uint32_t)(v93 + v94 + 63) >> 7)) & 0x3FF;
            ++*(uint32_t *)&__frame.buf[4 * v95];
            ++__frame.v179[((uint16_t)v89 - v92) & 0x3FF];
            LOWORD(v89) = v89 - v91;
            v96 = __frame.v202[2];
            v97 = __frame.v223;
            ++__frame.v180[v89 & 0x3FF];
            v98 = v97[4 * v96 + 23] + 256;
            v99 = ((uint16_t)v98
                 - (uint16_t)((__frame.v202[0] * v97[4 * v96 + 21]
                                     + __frame.__choose_plane_coding_n191_1 * v97[4 * v96 + 20]
                                     + __frame.v202[1] * (uint32_t)v97[4 * v96 + 22]
                                     + 63) >> 7)
                 + 256)
                & 0x3FF;
            ++__frame.v180[v99 + 1024];
            v100 = v98 - v97[4 * v96 + 20];
            ++__frame.v180[v100 + 2048];
            v101 = v98 - v97[4 * v96 + 21];
            ++__frame.v180[v101 + 3072];
            v102 = v98 - v97[4 * v96 + 22];
            ++__frame.v180[v102 + 4096];
            v80 = v96 + 1;
          }
          while ( v80 < __frame.v224 );
          n191_3 = __frame.v202[1];
          n191_2 = __frame.v202[0];
        }
        v103 = __estimate_cost((uint8_t *)__frame.buf, 1024);
        v104 = (v103 >> 7) + v103;
        v105 = __estimate_cost((uint8_t *)__frame.v178, 1024);
        v106 = v105 < v104;
        if ( v105 < v104 )
          v104 = v105;
        n191_4 = __frame.__choose_plane_coding_n191_1;
        if ( v106 )
        {
          n191_4 = 128;
          n191_3 = 0;
          n191_2 = 0;
        }
        __frame.__choose_plane_coding_n191_1 = n191_4;
        __frame.v202[0] = v106;
        v108 = __estimate_cost((uint8_t *)__frame.v179, 1024);
        n2_2 = __frame.v202[0];
        if ( v108 < v104 )
        {
          v104 = v108;
          n2_2 = 2;
          n191_2 = 128;
          n191_3 = 0;
          __frame.__choose_plane_coding_n191_1 = 0;
        }
        __frame.v202[0] = n2_2;
        v110 = __estimate_cost((uint8_t *)__frame.v180, 1024);
        __choose_plane_coding_n3_1 = __frame.v202[0];
        if ( v110 < v104 )
        {
          __choose_plane_coding_n3_1 = 3;
          n191_3 = 128;
          n191_2 = 0;
          __frame.__choose_plane_coding_n191_1 = 0;
        }
        plane_desc[4].w4 = __frame.__choose_plane_coding_n191_1;
        plane_desc[4].w8 = n191_2;
        plane_desc[4].w12 = n191_3;
        plane_desc[4].src_plane = 3;
        plane_desc[4].predictor = 3;
        v112 = &__frame.v180[1024 * __choose_plane_coding_n3_1 + 1024];
        n192 = (uint8_t)(uintptr_t)v112 & 0xF;
        // And the same again, over the third.
        v115 = 0;
        for ( i = 0; i < 256; ++i )
          v115 += v112[n192 + i];
        n192 += 256;
        v116 = v115;
        for ( k = -1; n192 < 1024; ++n192 )
        {
          v115 = v112[n192] + v115 - v112[n192 - 256];
          if ( v115 >= v116 )
          {
            k = n192;
            v116 = v115;
          }
        }
        plane_desc[4].b3 = k + 1;
      }
    }
  }
  return n192;
}

int32_t *__read_bmp(char *FileName)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW aborts while compressing.
  struct alignas(16) ReadBmpFrame {   // 128 bytes, one stack frame
      uint32_t Size_4;
      int32_t Size;
      BmfImage *v52;
      int32_t Src_2;
      void *Buffer_3;
      uint8_t _pad0[4];
      uint8_t *Src;
      uint8_t _pad1[4];
      uint8_t bmp_bgra[4];
      uint32_t bmp_info_hdr[2];
      int32_t bmp_height;
      int16_t bmp_planes;
      uint16_t bmp_bits;
      int32_t bmp_compression;
      uint8_t _pad2[12];
      int32_t bmp_clr_used;
      uint8_t _pad3[4];
      int16_t bmp_file_hdr[5];
      uint8_t bmp_off_bits[4];
      uint8_t _pad4[38];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 128, "frame layout moved");
  // These shared `__frame.Size_4` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v46;
  int32_t v47;
  // These shared `__frame.Size` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `__frame.Size` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t Sizeb;
  // These shared `__frame.Src_2` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v54;
  ;
  uintptr_t Src_1;   // were int32_t: addresses, masked and tagged
  uint8_t *v7, *v8, *v9;   // were int32_t: these hold addresses
  FILE *Stream_v;
  int8_t v25;
  uint8_t v28, v30;
  uint8_t *Src_4, *Src_3, *Src_6, *Buffer_4, *Src_5;   // `uint8_t *` beside the `char` scalars above
  BmfImage *v3;
  int32_t Size_1, i, j_3, Sizea_1, v22, n2_1, v26, v31, Offset_2, v35, v38, v40, v41;
  uint32_t Size_2, n2_2, v29, ElementCount, ElementCount_1, v44;
  // These two freads land in the frame, and each writes across several of the
  // slots Hex-Rays split it into -- which is why the fields do not look like
  // fields.  `bmp_info_hdr` is declared `uint32_t[2]` and the read is 40 bytes:
  //
  //   frame +36  bmp_info_hdr[0]  biSize          checked == 40 below
  //         +40  bmp_info_hdr[1]  biWidth
  //         +44  bmp_height       biHeight
  //         +48  bmp_planes       biPlanes        checked == 1 below
  //         +50  bmp_bits         biBitCount
  //         +52  bmp_compression  biCompression   0 none, 1 RLE8, 2 RLE4
  //         +56  _pad2[12]        biSizeImage and the two pixels-per-metre
  //         +68  bmp_clr_used     biClrUsed
  //         +72  _pad3[4]         biClrImportant
  //
  // and the 14-byte read covers `bmp_file_hdr[0..4]` and the slot after it,
  // whose four bytes are `bfOffBits` -- `bmp_off_bits`, which the fseek below
  // uses.  Every one of these names is confirmed by what the code does with it,
  // not by the offset alone: bmp_compression is tested against 1 and 2,
  // bmp_clr_used overrides `1 << bmp_bits` as the palette size, bmp_height goes
  // to alloc_image as the height.  The struct that says the same thing in one
  // piece is `BmpHeader`, above write_bmp, which builds this on the way out.
  Stream_v = fopen(FileName, "rb");
  if ( !Stream_v
    || fread(__frame.bmp_file_hdr, 0xEu, 1u, Stream_v) != 1
    || __frame.bmp_file_hdr[0] != 0x4D42 /* 'BM' */
    || fread(__frame.bmp_info_hdr, 0x28u, 1u, Stream_v) != 1
    || __frame.bmp_info_hdr[0] != 40
    || __frame.bmp_planes != 1 )
  {
    return nullptr;
  }
  v3 = (BmfImage *)(__alloc_image(__frame.bmp_info_hdr[1], __frame.bmp_height, __frame.bmp_bits, __frame.bmp_bits <= 8u, 1));
  Size_2 = (v3->stride + 3) & 0xFFFFFFFC;
  if ( __frame.bmp_bits <= 8u )
  {
    Size_1 = 1 << (__frame.bmp_bits & 31);
    if ( __frame.bmp_clr_used )
      Size_1 = __frame.bmp_clr_used;
    if ( Size_1 > 0 )
    {
      __frame.Size_4 = (v3->stride + 3) & 0xFFFFFFFC;
      __frame.Size = Size_1;
      for ( i = 0; i < __frame.Size; ++i )
      {
        fread(__frame.bmp_bgra, 4u, 1u, Stream_v);
        if ( (v3->depth & 0x80) != 0 )
          v7 = (uint8_t *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v7 = 0;
        *(v7 + 3 * i + 2) = __frame.bmp_bgra[2];
        if ( (v3->depth & 0x80) != 0 )
          v8 = (uint8_t *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v8 = 0;
        *(v8 + 3 * i + 1) = __frame.bmp_bgra[1];
        if ( (v3->depth & 0x80) != 0 )
          v9 = (uint8_t *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v9 = 0;
        *(v9 + 3 * i) = __frame.bmp_bgra[0];
      }
      Size_2 = __frame.Size_4;
    }
  }
  __frame.Buffer_3 = bmf_new(Size_2);
  __frame.Src = (uint8_t *)v3 + v3->data_size - v3->stride + 16;
  fseek(Stream_v, (*(int32_t *)((uint8_t *)__frame.bmp_off_bits)), 0);
  if ( __frame.bmp_compression )
  {
    if ( __frame.bmp_compression == 1 )
    {
      memset(v3->pixels,0,v3->data_size);
      Src_1 = (int32_t)__frame.Src;
      __frame.v52 = v3;
      v46 = v3->height - 1;
      while ( 1 )
      {
        __frame.Src_2 = Src_1;
        if ( ferror(Stream_v) )
          return nullptr;
        j_3 = fgetc(Stream_v);
        Sizea_1 = fgetc(Stream_v);
        if ( j_3 )
        {
          // An RLE8 run: `j_3` copies of one byte.  What was here instead was
          // a scalar head to reach sixteen-byte alignment, sixteen bytes an
          // iteration, and a scalar tail -- memset with the alignment written
          // out, and a separate short-run path for anything under 16 + the
          // head.
          //
          // The write is still not bounded by the pixel buffer: a stream that
          // ends mid-run keeps writing.  That is a real defect, recorded
          // rather than repaired (REFACTORING.md §6), and it is why the
          // malformed-input check truncates an uncompressed BMP instead.
          __builtin_memset((void *)__frame.Src_2, Sizea_1, j_3);
          Src_1 = __frame.Src_2 + j_3;
        }
        else if ( Sizea_1 )
        {
          if ( Sizea_1 == 1 )
            goto LABEL_61;
          if ( Sizea_1 == 2 )
          {
            v38 = fgetc(Stream_v);
            Src_1 = v38 + Src_1 - fgetc(Stream_v) * *((uint16_t *)__frame.v52 + 2);
          }
          else
          {
            fread(__frame.Buffer_3, (Sizea_1 + 1) & 0xFFFFFFFE, 1u, Stream_v);
            memcpy((uint8_t *)Src_1,(uint8_t *)__frame.Buffer_3,Sizea_1);
            Src_1 += Sizea_1;
          }
        }
        else
        {
          if ( --v46 < 0 )
            goto LABEL_61;
          Src_1 = (int32_t)__frame.v52 + v46 * *((uint16_t *)__frame.v52 + 2) + 16;
        }
      }
    }
    if ( __frame.bmp_compression != 2 )
      return nullptr;
    memset(v3->pixels,0,v3->data_size);
    __frame.v52 = v3;
    v22 = 1;
    v47 = v3->height - 1;
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
LABEL_44:
          if ( ferror(Stream_v) )
            return nullptr;
          v54 = fgetc(Stream_v);
          n2_1 = fgetc(Stream_v);
          n2_2 = n2_1;
          if ( !v54 )
            break;
          v25 = n2_1 & 0xF;
          if ( v22 )
          {
            v31 = v54;
            Src_3 = __frame.Src;
            while ( v31 != 1 )
            {
              *Src_3++ = n2_2;
              v31 -= 2;
              if ( !v31 )
              {
                __frame.Src = Src_3;
                v22 = 1;
                goto LABEL_44;
              }
            }
            __frame.Src = Src_3;
            *Src_3 = n2_2 & 0xF0;
            v22 = 0;
          }
          else
          {
            v26 = v54;
            Src_4 = __frame.Src;
            v28 = *__frame.Src;
            v29 = n2_2 >> 4;
            v30 = 16 * v25;
            while ( 1 )
            {
              *Src_4++ = v29 | v28;
              if ( v26 == 1 )
                break;
              v28 = v30;
              v26 -= 2;
              if ( !v26 )
              {
                __frame.Src = Src_4;
                *Src_4 = v30;
                v22 = 0;
                goto LABEL_44;
              }
            }
            __frame.Src = Src_4;
            v22 = 1;
          }
        }
        if ( n2_1 )
          break;
        if ( --v47 < 0 )
          goto LABEL_61;
        __frame.Src = (uint8_t *)__frame.v52 + v47 * *((uint16_t *)__frame.v52 + 2) + 16;
      }
      if ( n2_1 == 1 )
        goto LABEL_61;
      if ( n2_1 != 2 )
        break;
      v40 = fgetc(Stream_v);
      v41 = (v40 >> 1) - fgetc(Stream_v) * *((uint16_t *)__frame.v52 + 2);
      if ( (v40 & 1) == 1 )
      {
        if ( !v22 )
          ++v41;
        v22 = !v22;
      }
      __frame.Src += v41;
    }
    fread(__frame.Buffer_3, (((n2_1 + 1) >> 1) + 1) & 0xFFFFFFFE, 1u, Stream_v);
    Buffer_4 = (uint8_t *)__frame.Buffer_3;
    Src_5 = __frame.Src;
    while ( 1 )
    {
      Sizeb = *Buffer_4;
      if ( v22 )
      {
        v44 = n2_2 - 1;
        if ( !v44 )
        {
          __frame.Src = Src_5;
          *Src_5 = *Buffer_4 & 0xF0;
          v22 = 0;
          goto LABEL_44;
        }
        *Src_5++ = Sizeb;
        v22 = 1;
      }
      else
      {
        *Src_5++ |= (uint8_t)*Buffer_4 >> 4;
        v44 = n2_2 - 1;
        if ( !v44 )
        {
          __frame.Src = Src_5;
          v22 = 1;
          goto LABEL_44;
        }
        *Src_5 = 16 * (Sizeb & 0xF);
        v22 = 0;
      }
      ++Buffer_4;
      n2_2 = v44 - 1;
      if ( !n2_2 )
      {
        __frame.Src = Src_5;
        goto LABEL_44;
      }
    }
  }
  ElementCount = v3->stride;
  Offset_2 = Size_2 - ElementCount;
  if ( __frame.bmp_height - 1 >= 0 )
  {
    v35 = __frame.bmp_height - 1;
    __frame.v52 = v3;
    Src_6 = __frame.Src;
    while ( 1 )
    {
      ElementCount_1 = fread(Src_6, 1u, ElementCount, Stream_v);
      ElementCount = *((uint16_t *)__frame.v52 + 2);
      if ( ElementCount_1 != ElementCount )
        return nullptr;
      if ( Offset_2 )
      {
        fseek(Stream_v, Offset_2, 1);
        ElementCount = *((uint16_t *)__frame.v52 + 2);
      }
      Src_6 -= ElementCount;
      if ( --v35 < 0 )
      {
LABEL_61:
        v3 = __frame.v52;
        break;
      }
    }
  }
  fclose(Stream_v);
  free(__frame.Buffer_3);
  return (int32_t *)v3;
}

int32_t __decode_symbol_list(SymList *a1)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW segfaults while decompressing.
  struct alignas(16) DecodeSymbolListFrame {   // 32824 bytes, one stack frame
      union {
          SymEntry *list[8192];   // the symbol list: 8 named slots and 32736 bytes of tail, one array
          struct {   // the locals MSVC spilled into these bytes
            // Slot 0 holds an entry pointer while the list is being built and
            // a loop count once the rescale pass starts; the two spellings are
            // the two roles, which is what the original casts were hiding.
            uint32_t list0;
            SymEntry *list1;
            int32_t list2;
            int32_t list3;
            int32_t list4;
            uint32_t list5;
            SymEntry *list6;
            int32_t list7;
            SymEntry *list_tail[8184];
          };
      };
      uint32_t n0x7F800000_1;
      int32_t tot;
      int32_t v65;
      SymEntry *v66;
      SymList *v67;
      uint32_t v68;   // the exclusion generation, not a pointer: the slot is reused
      uint8_t _pad1[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 32832, "frame layout moved");
  ;
  // Every cursor here walks `a1->ent`'s three-byte entries: the symbol was
  // `*(uint16_t *)p` and the count `p[2]`, and the steps were +3 and -3.
  SymEntry *ent, *v7, *v20, *v25, *v33, *v36, *v38, *v44, *v45, *v47, *v48;
  SymEntry **v4, **v21, **v26;
  int8_t v23;
  uint8_t v34, v40;
  uint16_t v35, v39;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t n0x2000_2;
  int32_t sym_cum, sym_high, live, v5, v6, v8, n251, v46, v49, v53;
  uint32_t v43, v52;   // counts that MSVC spilled into the list's first slot
  SymList *v9, *v32;
  uint32_t __decode_symbol_list_n0x800000, n0x2000_6, n0x2000_4,
           n0x2000_3, tot_1, rescale_at, v42, v50,
           since_rescale;
  live = a1->live;
  ent = a1->ent;
  v4 = __frame.list;
  __frame.v68 = (uint8_t)exclusion_gen;
  __frame.v67 = a1;
  v5 = 0;
  v6 = 0;
  do
  {
    if ( (uint8_t)exclusion_mask[ent[v6].sym] == __frame.v68 )
    {
      v8 = 0;
    }
    else
    {
      v7 = &ent[v6];
      v8 = v7->cnt;
      *v4++ = v7;
    }
    v5 += v8;
    ++v6;
  }
  while ( v6 < live );
  v9 = __frame.v67;
  if ( !v5 )
    return -1;
  *v4 = nullptr;
  __frame.v65 = v9->tot;
  n0x2000_6 = v5 + __frame.v65;
  n0x2000_4 = rc.get_freq(n0x2000_6);
  __frame.tot = v5 + __frame.v65;
  __frame.v67 = v9;
  v20 = (__frame.list[0]);
  v21 = &__frame.list[1];
  __frame.v66 = (__frame.list[0]);
  n0x2000_2 = 0;
  while ( 1 )
  {
    n0x2000_2 += v20->cnt;
    if ( n0x2000_2 > (int32_t)n0x2000_4 )
      break;
    v20 = *v21++;
    if ( !v20 )
    {
      v23 = (int8_t)__frame.v68;
      sym_cum = n0x2000_2;
      n0x2000_3 = __frame.tot;
      sym_high = __frame.tot;
      v25 = __frame.v66;
      v26 = &__frame.list[1];
      do
      {
        exclusion_mask[v25->sym] = v23;
        v25 = *v26++;
      }
      while ( v25 );
      tot_1 = __frame.tot;
      __frame.list7 = -1;
      {
        rc.decode(n0x2000_2, n0x2000_3, tot_1);
        return __frame.list7;
      }
    }
  }
  v32 = __frame.v67;
  sym_high = n0x2000_2;
  sym_cum = n0x2000_2 - v20->cnt;
  __frame.list7 = v20->sym;
  v20->cnt += 4;
  v33 = v32->ent;
  v32->since_rescale += 4;
  if ( v20 == v33 )
  {
    n251 = v20->cnt;
  }
  else
  {
    // Swap this entry with the one before it.
    v34 = v20->cnt;
    v35 = v20->sym;
    v36 = v20 - 1;
    v20->sym = v36->sym;
    v20->cnt = v36->cnt;
    v36->sym = v35;
    v36->cnt = v34;
    v33 = v32->ent;
    if ( v36 == v33 )
    {
      n251 = v36->cnt;
    }
    else
    {
      __frame.list5 = __decode_symbol_list_n0x800000;
      while ( 1 )
      {
        n251 = v36->cnt;
        v38 = v36 - 1;
        if ( n251 <= v38->cnt )
          break;
        // Swap the two entries: the more-used one moves towards the front.
        v39 = v36->sym;
        v40 = v36->cnt;
        v36->sym = v38->sym;
        v36->cnt = v38->cnt;
        v38->sym = v39;
        v38->cnt = v40;
        v33 = v32->ent;
        --v36;
        if ( v38 == v33 )
        {
          __decode_symbol_list_n0x800000 = __frame.list5;
          n251 = v38->cnt;
          goto LABEL_30;
        }
      }
      __decode_symbol_list_n0x800000 = __frame.list5;
    }
  }
LABEL_30:
  rescale_at = v32->rescale_at;
  if ( n251 > 251 || rescale_at < v32->since_rescale )
  {
    __frame.list0 = v32->live;
    v42 = 20 * v32->n;
    v43 = __frame.list0;
    __frame.list5 = __decode_symbol_list_n0x800000;
    __frame.list4 = rescale_at < v42;
    v44 = v33 - 1;
    do
    {
      v45 = v44;
      ++v44;
      v46 = (__frame.list4 + (uint32_t)v44->cnt) >> 1;
      v44->cnt = v46;
      if ( v44 != v32->ent )
      {
        v47 = v44 - 1;
        __frame.list3 = v47->cnt;
        if ( v46 > __frame.list3 )
        {
          __frame.list2 = v44->sym;
          v44->sym = v47->sym;
          v44->cnt = __frame.list3;
          if ( v47 != v32->ent )
          {
            (__frame.list[1]) = v44;
            __frame.list0 = v43;
            do
            {
              v48 = v47 - 1;
              v49 = v48->cnt;
              if ( v46 <= v49 )
                break;
              v47->sym = v48->sym;
              v47->cnt = v49;
              --v47;
            }
            while ( v48 != v32->ent );
            v44 = (__frame.list[1]);
            v43 = __frame.list0;
          }
          v47->sym = __frame.list2;
          v47->cnt = v46;
        }
      }
      --v43;
    }
    while ( v43 );
    __decode_symbol_list_n0x800000 = __frame.list5;
    v50 = v32->tot;
    __frame.list0 = 0;
    if ( !v44->cnt )
    {
      v52 = __frame.list0;
      do
      {
        ++v52;
        v32->tot = ++v50;
        v53 = v45->cnt;
        --v45;
      }
      while ( !v53 );
      __frame.list0 = v52;
      v32->live -= v52;
    }
    since_rescale = v32->since_rescale;
    v32->tot = v50 - (v50 >> 1);
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
    tot_1 = n0x2000_6;
    v32->since_rescale = since_rescale - (since_rescale >> 1);
  }
  else
  {
    tot_1 = n0x2000_6;
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
  }
  rc.decode(n0x2000_2, n0x2000_3, tot_1);
  return __frame.list7;
}

int32_t __decode_pixel(ModelBlock *_this, int32_t a2)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW segfaults while decompressing.
  struct alignas(16) DecodePixelFrame {   // 164 bytes, one stack frame
      union {
          uint32_t  sym[32];   // pixel_context reads sym[0..31]; `sym_pos` runs 0..31 (was n15_8 .. v211)
          struct {   // the locals MSVC spilled into these bytes
            int32_t sym0;
            int32_t sym1;
            int32_t sym2;
            int32_t sym3;
            FreqRec *sym4;   // the coder's frequency record
            ModelBlock *sym5;
            int32_t sym6;
            int32_t sym7;
            int32_t sym8;
            int32_t sym9;
            int32_t sym10;
            int32_t sym11;
            int32_t sym12;
            int32_t sym13;
            int32_t sym14;
            int32_t sym15;
            int32_t sym16;
            int32_t sym17;
            int32_t sym18;
            int32_t sym19;
            int32_t sym20;
            int32_t sym21;
            int32_t sym22;
            int32_t sym23;
            int32_t sym24;
            int32_t sym25;
            int32_t sym26;
            int32_t sym27;
            int32_t sym28;
            int32_t sym29;
            int32_t sym30;
            int32_t sym31;
          };
      };
      uint8_t   _gap0[4];   // was int32_t n15_24
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t n15_24;
  ;
  FreqRec *v80;
  uint16_t *v36;
  PixRec *v21, *v23, *v57;   // row cursors out of ModelBlock
  PixRec *n15_17;   // `row_cur[6]`, the row above
  uint16_t *v66;   // a copy of `pix_cur`
  PixRec *v54, *v55, *v61, *v108, *n15_10;
  int8_t v74, v91;
  uint8_t v70, v71, v72, v73, v75;
  uint8_t *v157;   // `uint8_t *` beside the `char` scalars above
  int16_t sym_rev, n4_14, v146, n15_4;
  ModelBlock *this_4;
  // A cumulative count, a high count and a total: the three arguments the
  // range coder takes, and it takes them unsigned.
  uint32_t arg_cum, arg_high, arg_tot;
  int32_t n4_8, n4_7, n15_6, n15_7, v8, v9, v12, v13, ctx_state, n4_9,
          __decode_pixel_n15, ctx_bucket, n4_11, v26, v27, v29, v31, n0xFFFF, v33,
          n0xFFFF_1, n53248, n4_12, v40, n15_11, run, n15_12, v44, n4_22,
          n15_14, n15_18, v49, v50, v51, n4_13, *v59, v60, n15_13, v67,
          n15_15, n4_17, v83, v84, v85, n4, n256_2, n15_1, n256_1, n15_23,
          n4_19, n4_20, n4_5, n4_6, n15_5, v102, v103, v104, v105, v107, v109,
          v111, v112, v113, v114, v115, v116, v117, v118, v119, v120, v122,
          v123, v124, v125, v126, v127, v128, n32, n15_25, n4_21, v135, n256,
          b15, n4_18, n15_19, n15_20, w5, v149, v150, v151, v152, v153,
          v155, n15_21, v158, v159, v161, v163, v164, n4_2, n15_22, n256_4,
          n256_5, n256_3, n15_2;
  FreqRec *v171;
  PixRec *v4;      // `row_cur[5]`, the current row
  FreqRec *v90;
  uint16_t *sym_cache;
  // `row_cur[6]`, the row above: every reach through it is a multiple of
  // four `uint16_t`, which is one `PixRec`, and every one is `sym`.
  PixRec *v106;
  FreqRec *freq_tbl;
  SymPair *v16;   // the group's counter pair for this context
  PixRec *n15_9;   // `row_cur[6]`, the row above
  uint16_t n4_10;   // a symbol, compared against four others
  PixRec *v110, *v121;   // `row_cur[7]` and `row_cur[8]`
  uint16_t *v58, *v95, v154, v162,
           v174;
  ModelBlock *this_3;
  ModelBlock *this_2;
  SymList *sel1_list;
  SymList **v134;
  uint32_t bin_tot, n4_16, n4_15, v136, v137, v138, v139, v140, v172, v173, v175, v176,
           v177;
  PixRec *v39;     // `row_cur[7]`, two rows above
  PixRec *v38;     // `row_cur[6]`, the row above
  PixRec *v76;      // `row_cur[7]`, two rows above
  PixRec *n15_16;   // `row_cur[6]`, the row above
  n15_9 = (PixRec *)_this->row_cur[6];
  n4_8 = n15_9->sym;
  v4 = _this->row_cur[5];
  n4_7 = v4[-1].sym;
  n15_6 = n15_9[1].sym;
  __frame.sym5 = (ModelBlock *)(_this);
  n15_7 = n15_9[-1].sym;
  __frame.sym0 = n4_8;
  ::mode_symbol[1] = n4_8;
  __frame.sym1 = n4_7;
  ::mode_symbol[2] = n4_7;
  __frame.sym3 = n15_6;
  ::mode_symbol[3] = n15_6;
  v8 = n15_9->match[1] + 4 * (n15_6 == n15_7);
  v9 = n15_9[1].match[1];
  __frame.sym2 = n15_7;
  mode_symbol[4] = n15_7;
  this_2 = (ModelBlock *)(__frame.sym5);
  v12 = 32 * v4[-1].match[2] + 16 * v4[-1].match[4] + (2 * v4[-1].match[0] + 8 * v9 + v8);
  if ( n4_8 == n4_7 )
  {
    if ( __frame.sym3 == __frame.sym0 )
    {
      sym_rev = __frame.sym5->sym_rev[n4_8];
      if ( n4_8 == __frame.sym2 )
        v13 = (uint16_t)(sym_rev - v4[-2].sym);
      else
        v13 = (uint16_t)(sym_rev - __frame.sym2);
    }
    else
    {
      v13 = (uint16_t)(__frame.sym5->sym_rev[n4_8] - __frame.sym3);
    }
  }
  else
  {
    v13 = (uint16_t)(__frame.sym5->sym_rev[n4_8] - __frame.sym1);
  }
  __frame.sym5->sym_cache = &__frame.sym5->sym_ctr[8 * v13];
  ctx_state = this_2->ctx_state[v12];
  this_2->f36 = ctx_state;
  v16 = &this_2->group_ctr[ctx_state][v13];
  this_2->pix_cur = (uint16_t *)v16;
  n4_9 = v16->last;
  if ( n4_9 == __frame.sym0 )
  {
    __decode_pixel_n15 = 15;
  }
  else if ( n4_9 == __frame.sym1 )
  {
    __decode_pixel_n15 = 30;
  }
  else if ( n4_9 == __frame.sym3 )
  {
    __decode_pixel_n15 = 45;
  }
  else
  {
    __decode_pixel_n15 = 60;
    if ( !(n4_9 == __frame.sym2) )
      __decode_pixel_n15 = 0;
  }
  n4_10 = v16->prev;
  if ( n4_10 == __frame.sym0 )
  {
    __decode_pixel_n15 += 75;
  }
  else if ( n4_10 == __frame.sym1 )
  {
    __decode_pixel_n15 += 150;
  }
  else if ( n4_10 == __frame.sym3 )
  {
    __decode_pixel_n15 += 225;
  }
  else if ( n4_10 == __frame.sym2 )
  {
    __decode_pixel_n15 += 300;
  }
  v21 = this_2->row_cur[8];
  __frame.sym3 = (int32_t)n15_9;
  ctx_bucket = this_2->ctx_bucket[ctx_state + __decode_pixel_n15];
  v23 = this_2->row_cur[7];
  this_2->bucket_idx = ctx_bucket;
  n4_11 = n15_9->match[0];
  __frame.sym4 = (FreqRec *)v4;
  __frame.sym5 = (ModelBlock *)(this_2);
  __frame.sym2 = n4_11;
  v26 = v4[-1].match[1];
  // `v186` is one stack slot with two roles: a row cursor here, and the
  // `uint16_t` value out of `sym_cache[4]` at 11290.  Splitting it needs the frame
  // to dissolve first, so the cast records the double booking (§4.2).
  __frame.sym6 = (int32_t)this_2->row_cur[9];
  v27 = 8 * v4[-2].match[2] + 4 * v4[-2].match[5] + v26 + 2 * v4[-2].match[4];
  this_3 = (ModelBlock *)(__frame.sym5);
  v29 = ((uint8_t)(((PixRec *)__frame.sym6)->match[0] & v21->match[0] & __frame.sym2 & v23->match[0]) << 9)
      + ((uint8_t)(((PixRec *)__frame.sym6)->match[1] & v21->match[1] & v23->match[1] & n15_9->match[1]) << 8)
      + (ctx_bucket << 10)
      + v27;
  n15_10 = (PixRec *)__frame.sym3;
  v31 = ((__frame.sym5->grad[3] == 0) << 7)
      + ((__frame.sym5->grad[2] == 0) << 6)
      + 32 * (__frame.sym5->grad[1] == 0)
      + 16 * (__frame.sym5->grad[0] == 0)
      + v29;
  n0xFFFF = __frame.sym5->ctx_id1[v31];
  if ( n0xFFFF == 0xFFFF )
  {
    __frame.sym5->ctx_id1[v31] = __frame.sym5->ctx_id1_used;
    n15_10 = this_3->row_cur[6];
    v4 = this_3->row_cur[5];
    ++this_3->ctx_id1_used;
    n0xFFFF = this_3->ctx_id1[v31];
    __frame.sym2 = n15_10->match[0];
  }
  v33 = v4[-1].match[5] + 4 * n15_10[1].match[3] + 2 * __frame.sym2 + 8 * n0xFFFF;
  n0xFFFF_1 = this_3->ctx_id2[v33];
  if ( n0xFFFF_1 == 0xFFFF )
  {
    this_3->ctx_id2[v33] = this_3->ctx_id2_used++;
    n0xFFFF_1 = this_3->ctx_id2[v33];
  }
  if ( (int32_t)this_3->alphabet < 32 )
  {
    n53248 = this_3->ctx_id3_used;
    __frame.sym1 = 16 * n0xFFFF_1 + (__frame.sym1 & 0xF);
    v36 = &this_3->ctx_id3[__frame.sym1];
    n0xFFFF_1 = *v36;
    if ( n0xFFFF_1 == 0xFFFF )
    {
      n4_12 = __frame.sym1;
      if ( n53248 > 53248 )
        n4_12 = __frame.sym1 | 0xF;
      v36 = &this_3->ctx_id3[n4_12];
      n0xFFFF_1 = *v36;
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *v36 = n53248;
      ++this_3->ctx_id3_used;
      n0xFFFF_1 = *v36;
    }
  }
  if ( (this_3->row_cur[5][-1].match[1] & this_3->row_cur[5][-1].match[0]) != 0 )
  {
    v38 = this_3->row_cur[6];
    v39 = this_3->row_cur[7];
    // Nine "matches the pixel to the left" flags and one "matches the pixel
    // above": the run about to be coded is the same colour all the way back.
    if ( ((uint8_t)(v39[2].match[1] & v39[1].match[1] & v39[0].match[1] & v38[3].match[1] & v38[2].match[1]
                  & v38[1].match[1] & v38[0].match[1] & v38[0].match[0] & (int8_t)v38[-1].match[1])
        & v39[3].match[1]) != 0 )
    {
      v40 = v39[0].match[0];
      n15_11 = 1;
      if ( this_3->width - a2 <= 1 )
      {
        run = 1;
      }
      else
      {
        __frame.sym1 = this_3->width - a2;
        __frame.sym5 = (ModelBlock *)(this_3);
        while ( 1 )
        {
          run = n15_11;
          if ( (v38[n15_11 + 2].match[1] & v38[n15_11 + 2].match[0]) == 0 )
            break;
          v40 = (uint8_t)(v39[run].match[0] & v40);
          if ( ++n15_11 >= __frame.sym1 )
          {
            this_3 = (ModelBlock *)(__frame.sym5);
            run = n15_11;
            goto LABEL_42;
          }
        }
        this_3 = (ModelBlock *)(__frame.sym5);
      }
LABEL_42:
      n15_12 = *(this_3->run_bucket + n15_11);
      // Record `8 * n15_12 + 4 * (two neighbour flags) + 2 * v40 + sym + 1`
      // of the 257-record grid: `269089 * 4` is +1 076 356, four bytes past
      // `esc_ctr`, and every term above it is a multiple of three words.
      v44 = __decode_context_bit(
            &this_3->esc_ctr[(8 * n15_12
                                   + 4 * (uint8_t)(v39[run + 3].match[1] & v39[run + 2].match[1])
                                   + 2 * v40
                                   + *(this_3->alpha_map + __frame.sym0)
                                   + 1)],
            this_3->esc_ctr);
      n4_22 = ::mode_symbol[1];
      this_3->hit = v44;
      *(((uint8_t *)this_3->alpha_map) + n4_22) = v44;
      n15_14 = this_3->hit;
      if ( n15_14 )
      {
        n15_18 = n15_11;
      }
      else
      {
        n15_18 = 0;
        if ( n15_11 == 1 )
          goto LABEL_57;
        __frame.sym3 = n15_11;
        __frame.sym0 = n15_12;
        __frame.sym5 = (ModelBlock *)(this_3);
        n15_18 = 0;
        v49 = 1 << (n15_12 & 31);
        v50 = 0;
        do
        {
          if ( (v49 | v50) < __frame.sym3 )
          {
            v51 = __frame.sym5->run_ctr[16 * ((v50 == 0) + (n15_12 == __frame.sym0)) + n15_12].n[0];
            __frame.sym2 = (int32_t)&__frame.sym5->run_ctr[16 * ((v50 == 0) + (n15_12 == __frame.sym0)) + n15_12];
            bin_tot = v51 + __frame.sym5->run_ctr[16 * ((v50 == 0) + (n15_12 == __frame.sym0)) + n15_12].n[1];
            __frame.sym1 = rc.decode_bit(
                     v51,
                     __frame.sym5->run_ctr[16 * ((v50 == 0) + (n15_12 == __frame.sym0)) + n15_12].n[1]);
            if ( __frame.sym5->run_ctr[16 * ((v50 == 0) + (n15_12 == __frame.sym0)) + n15_12].limit < (uint32_t)bin_tot )
              __rescale_counter_pair((uint16_t *)__frame.sym2);
            n4_13 = __frame.sym1;
            *(uint16_t *)(__frame.sym2 + 2 * __frame.sym1) += 8;
            if ( n4_13 )
              n15_18 |= v49;
            v50 |= n15_18 & v49;
          }
          --n15_12;
          v49 >>= 1;
        }
        while ( v49 );
        this_3 = (ModelBlock *)(__frame.sym5);
      }
      if ( n15_18 )
        this_3->row_cur[5][n15_18 - 1].sym = this_3->row_cur[5][-1].sym;
      n15_14 = this_3->hit;
LABEL_57:
      if ( n15_18 > n15_14 )
      {
        this_3->row_cur[6] = this_3->row_cur[6] + n15_18 - n15_14;
        v54 = this_3->row_cur[8];
        v55 = this_3->row_cur[7] + n15_18;
        this_3->row_cur[7] = v55 - n15_14;
        this_3->row_cur[8] = v54 + n15_18 - n15_14;
        v57 = this_3->row_cur[5];
        this_3->row_cur[9] = this_3->row_cur[9] + n15_18 - n15_14;
        *(uint32_t *)&v57->match[2] = 0x01010101;
        *(uint32_t *)this_3->row_cur[5] = 0x01010101;
        v58 = (uint16_t *)this_3->pix_cur;
        LOWORD(v55) = ::mode_symbol[1];
        LOWORD(v54) = *v58;
        __frame.sym1 = ::mode_symbol[1];
        v58[1] = (uint16_t)(uintptr_t)v54;
        this_3->row_cur[5]->sym = (uint16_t)(uintptr_t)v55;
        this_3->pix_cur[0] = (uint16_t)(uintptr_t)v55;
        v59 = (int32_t *)this_3->row_cur[5];
        v60 = v59[1];
        __frame.sym3 = *v59;
        v61 = (PixRec *)(v59 + 2);
        this_3->row_cur[5] = v61;
        if ( n15_18 - n15_14 != 1 )
        {
          __frame.sym2 = (n15_18 - n15_14 - 1) / 2;
          if ( __frame.sym2 )
          {
            n15_13 = __frame.sym3;
            __frame.sym0 = n15_14;
            n4_14 = __frame.sym1;
            n4_16 = 0;
            n15_24 = n15_18;
            n4_15 = __frame.sym2;
            do
            {
              this_3->pix_cur[1] = n4_14;
              *(uint32_t *)this_3->row_cur[5] = n15_13;
              *(uint32_t *)&this_3->row_cur[5]->match[2] = v60;
              v66 = this_3->pix_cur;
              ++this_3->row_cur[5];
              v66[1] = n4_14;
              *(uint32_t *)this_3->row_cur[5] = n15_13;
              *(uint32_t *)&this_3->row_cur[5]->match[2] = v60;
              v61 = this_3->row_cur[5] + 1;
              this_3->row_cur[5] = v61;
              ++n4_16;
            }
            while ( n4_16 < n4_15 );
            n15_14 = __frame.sym0;
            n15_18 = n15_24;
            v67 = 2 * n4_16 + 1;
          }
          else
          {
            v67 = 1;
          }
          if ( n15_18 - n15_14 - 1 > (uint32_t)(v67 - 1) )
          {
            n15_15 = __frame.sym3;
            this_3->pix_cur[1] = __frame.sym1;
            *(uint32_t *)this_3->row_cur[5] = n15_15;
            *(uint32_t *)&this_3->row_cur[5]->match[2] = v60;
            v61 = this_3->row_cur[5] + 1;
            this_3->row_cur[5] = v61;
          }
        }
        n15_24 = n15_18;
        n15_16 = (PixRec *)this_3->row_cur[6];
        v70 = n15_16[-3].match[0];
        v71 = n15_16[-2].match[0];
        v72 = n15_16[2].match[0];
        v73 = n15_16[3].match[0];
        __frame.sym0 = (int32_t)n15_16;
        v74 = v71 + v70;
        v75 = n15_16[4].match[0];
        v76 = (PixRec *)this_3->row_cur[7];
        this_3->grad[0] = v73 + v72 + v74 + v75 - 5;
        // Eight records of the row two above, `match[0]` in each: the
        // byte offsets 2, 10, 18, 26, 34 and -6, -14, -22 were records
        // 0..4 and -1..-3.  Three of the eight loads are `movsx` in the
        // original and five are `movzx`; every writer of these bytes is
        // a comparison, so the sign never shows, and the casts stay
        // because the instruction is what is being transcribed.
        this_3->grad[1] = v76[3].match[0]
                                     + v76[2].match[0]
                                     + v76[1].match[0]
                                     + v76[0].match[0]
                                     + (int8_t)v76[-1].match[0]
                                     + (int8_t)v76[-2].match[0]
                                     + (int8_t)v76[-3].match[0]
                                     + v76[4].match[0]
                                     - 8;
        n15_17 = (PixRec *)__frame.sym0;
        this_3->grad[2] = v61[-4].match[1] + v61[-3].match[1] - 2;
        n4_17 = __frame.sym1;
        this_3->grad[3] = v61[-5].match[0]
                                     + v61[-6].match[0]
                                     + v61[-7].match[0]
                                     + v61[-4].match[0]
                                     - 4;
        v61[-1].match[4] = n4_17 == n15_17[1].sym;
        n15_18 = n15_24;
        this_3->row_cur[5][-1].match[5] = n4_17 == this_3->row_cur[6][2].sym;
        n15_14 = this_3->hit;
      }
      if ( n15_14 )
        return n15_18;
      goto LABEL_86;
    }
  }
  v80 = (FreqRec *)&this_3->row_cur[4 * this_3->bucket_idx + 10];
  __frame.sym4 = v80;
  freq_tbl = &this_3->grid[n0xFFFF_1 + 188];
  v83 = freq_tbl->w[5];
  if ( freq_tbl->w[5] )
  {
    if ( v83 == 1 )
    {
      b15 = v80->b15;
      n4_18 = b15 * freq_tbl->w[2];
      n15_19 = b15 * freq_tbl->w[3];
      __frame.sym1 = b15 * freq_tbl->w[0];
      v146 = b15 * freq_tbl->w[1];
      __frame.sym2 = n4_18;
      n15_20 = b15 * freq_tbl->w[4];
      __frame.sym0 = n15_19;
      __frame.sym3 = n15_20;
      *freq_tbl = *v80;
      w5 = freq_tbl->w[5];
      v149 = freq_tbl->w[0];
      freq_tbl->b14 *= 8;
      __frame.sym5 = (ModelBlock *)(this_3);
      v150 = 21 * freq_tbl->w[1];
      __frame.sym1 += (21 * v149 + w5 - 1) / w5;
      freq_tbl->w[0] = __frame.sym1;
      v151 = (v150 + w5 - 1) / w5;
      v152 = 21 * freq_tbl->w[2];
      v153 = freq_tbl->w[3];
      v154 = v151 + v146;
      freq_tbl->w[1] = v154;
      v155 = 21 * v153;
      n15_21 = __frame.sym0;
      v157 = (uint8_t *)((v152 + w5 - 1) / w5 + __frame.sym2);
      freq_tbl->w[2] = (uint16_t)(uintptr_t)v157;
      v158 = (v155 + w5 - 1) / w5 + n15_21;
      v159 = 21 * freq_tbl->w[4];
      freq_tbl->w[3] = v158;
      this_3 = (ModelBlock *)(__frame.sym5);
      v161 = (v159 + w5 - 1) / w5 + __frame.sym3;
      freq_tbl->w[4] = v161;
      v162 = __frame.sym1 + v161 + (v158 + (uint16_t)(uintptr_t)v157 + v154);
      v83 = v162;
      freq_tbl->w[5] = v162;
    }
    arg_tot = v83;
    v84 = rc.get_freq(arg_tot);
    v85 = freq_tbl->w[0];
    if ( v85 <= v84 )
    {
      v85 += freq_tbl->w[1];
      if ( v85 <= v84 )
      {
        v85 += freq_tbl->w[2];
        if ( v85 <= v84 )
        {
          v85 += freq_tbl->w[3];
          if ( v85 <= v84 )
          {
            v85 += freq_tbl->w[4];
            n4 = 4;
          }
          else
          {
            n4 = 3;
          }
        }
        else
        {
          n4 = 2;
        }
      }
      else
      {
        n4 = 1;
      }
    }
    else
    {
      n4 = 0;
    }
    n256_2 = freq_tbl->w[6];
    arg_high = v85;
    n15_1 = freq_tbl->b15;
    arg_cum = v85 - freq_tbl->w[n4];
    n256_1 = freq_tbl->w[5];
    if ( n256_1 > n256_2 && (freq_tbl->w[n4] + n15_1 + 8 < n256_1 || freq_tbl->w[5] > 0x4000u) )
    {
      v136 = freq_tbl->w[2];
      __frame.sym0 = n256_2;
      v137 = freq_tbl->w[1];
      __frame.sym5 = (ModelBlock *)(this_3);
      v138 = freq_tbl->w[0];
      __frame.sym1 = n4;
      __frame.sym2 = n15_1;
      LOWORD(v138) = v138 - (v138 >> 1);
      freq_tbl->w[0] = v138;
      LOWORD(v137) = v137 - (v137 >> 1);
      freq_tbl->w[1] = v137;
      LOWORD(v136) = v136 - (v136 >> 1);
      v139 = freq_tbl->w[3];
      freq_tbl->w[2] = v136;
      LOWORD(v139) = v139 - (v139 >> 1);
      v140 = freq_tbl->w[4];
      freq_tbl->w[3] = v139;
      LOWORD(v140) = v140 - (v140 >> 1);
      freq_tbl->w[4] = v140;
      LOWORD(v137) = v139 + v136 + v137;
      n15_1 = __frame.sym2;
      LOWORD(v140) = v138 + v140;
      this_3 = (ModelBlock *)(__frame.sym5);
      n256_1 = (uint16_t)(v140 + v137);
      n256 = __frame.sym0;
      n4 = __frame.sym1;
      freq_tbl->w[5] = n256_1;
      if ( n256 < 256 && !freq_tbl->b14 )
      {
        n256 = 256;
        freq_tbl->w[6] = 256;
      }
      if ( n256_1 > n256 )
      {
        if ( n15_1 < 15 )
          LOWORD(n15_1) = 15;
        freq_tbl->b15 = n15_1;
      }
    }
    freq_tbl->w[5] = n15_1 + n256_1;
    freq_tbl->w[n4] += n15_1;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3->hit = n4;
    if ( freq_tbl->b14 )
    {
      --freq_tbl->b14;
      v90 = __frame.sym4;
      ++__frame.sym4->w[5];
      ++v90->w[n4];
      n4 = this_3->hit;
    }
  }
  else
  {
    arg_tot = v80->w[5];
    v163 = rc.get_freq(arg_tot);
    v164 = __frame.sym4->w[0];
    if ( v164 <= v163 )
    {
      v164 += __frame.sym4->w[1];
      if ( v164 <= v163 )
      {
        v164 += __frame.sym4->w[2];
        if ( v164 <= v163 )
        {
          v164 += __frame.sym4->w[3];
          if ( v164 <= v163 )
          {
            v164 += __frame.sym4->w[4];
            n4_2 = 4;
          }
          else
          {
            n4_2 = 3;
          }
        }
        else
        {
          n4_2 = 2;
        }
      }
      else
      {
        n4_2 = 1;
      }
    }
    else
    {
      n4_2 = 0;
    }
    n15_22 = __frame.sym4->b15;
    arg_high = v164;
    arg_cum = v164 - __frame.sym4->w[n4_2];
    n256_4 = __frame.sym4->w[5];
    n256_5 = __frame.sym4->w[6];
    __frame.sym3 = n15_22;
    if ( n256_4 > n256_5 && (__frame.sym4->w[n4_2] + __frame.sym3 + 8 < n256_4 || n256_4 > 0x4000) )
    {
      __frame.sym0 = n256_5;
      __frame.sym5 = (ModelBlock *)(this_3);
      __frame.sym1 = (int32_t)(uintptr_t)freq_tbl;
      __frame.sym2 = n4_2;
      v171 = __frame.sym4;
      v172 = __frame.sym4->w[1];
      v173 = __frame.sym4->w[2];
      v174 = __frame.sym4->w[0] - (__frame.sym4->w[0] >> 1);
      __frame.sym4->w[0] = v174;
      LOWORD(v172) = v172 - (v172 >> 1);
      v171->w[1] = v172;
      v175 = v173 - (v173 >> 1);
      v176 = (uint16_t)v171->w[3];
      v171->w[2] = v175;
      LOWORD(v176) = v176 - (v176 >> 1);
      v177 = (uint16_t)v171->w[4];
      v171->w[3] = v176;
      LOWORD(v177) = v177 - (v177 >> 1);
      v171->w[4] = v177;
      LOWORD(v177) = v174 + v177;
      this_3 = (ModelBlock *)(__frame.sym5);
      n256_4 = (uint16_t)(v177 + v176 + v175 + v172);
      n256_3 = __frame.sym0;
      freq_tbl = (FreqRec *)__frame.sym1;
      v171->w[5] = n256_4;
      n4_2 = __frame.sym2;
      if ( n256_3 < 256 && !__frame.sym4->b14 )
      {
        n256_3 = 256;
        __frame.sym4->w[6] = 256;
      }
      if ( n256_4 > n256_3 )
      {
        n15_2 = __frame.sym3;
        if ( __frame.sym3 < 15 )
          n15_2 = 15;
        __frame.sym3 = n15_2;
        __frame.sym4->b15 = n15_2;
      }
    }
    n15_4 = __frame.sym3;
    __frame.sym4->w[5] = __frame.sym3 + n256_4;
    __frame.sym4->w[n4_2] += n15_4;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3->hit = n4_2;
    freq_tbl->w[5] = freq_tbl->w[n4_2]++ != 0;
    n4 = this_3->hit;
  }
  if ( n4 )
  {
    this_3->row_cur[5]->sym = mode_symbol[n4];
    return 1;
  }
  n15_18 = 0;
LABEL_86:
  v91 = exclusion_gen;
  n15_23 = ::mode_symbol[3];
  n4_19 = ::mode_symbol[1];
  n15_24 = n15_18;
  n4_20 = ::mode_symbol[2];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  v95 = (uint16_t *)this_3->pix_cur;
  exclusion_mask[n15_23] = v91;
  exclusion_mask[n4_20] = v91;
  exclusion_mask[n4_19] = v91;
  __byte_445440[0] = v91;
  this_3->sel[0] = nullptr;
  n4_5 = v95[1];
  __frame.sym0 = *v95;
  sym_cache = this_3->sym_cache;
  n4_6 = sym_cache[0];
  n15_5 = sym_cache[1];
  __frame.sym1 = n4_5;
  this_4 = (ModelBlock *)(sym_cache[3]);
  __frame.sym2 = n4_6;
  v102 = sym_cache[4];
  __frame.sym3 = n15_5;
  v103 = sym_cache[5];
  __frame.sym4 = ((FreqRec *)sym_cache[2]);
  v104 = sym_cache[6];
  v105 = sym_cache[7];
  __frame.sym5 = (ModelBlock *)((uint32_t *)this_4);
  v106 = (PixRec *)this_3->row_cur[6];
  __frame.sym6 = v102;
  v107 = v106[2].sym;
  __frame.sym7 = v103;
  v108 = this_3->row_cur[5];
  __frame.sym8 = v104;
  v109 = v108[-2].sym;
  __frame.sym9 = v105;
  __frame.sym10 = v109;
  v110 = this_3->row_cur[7];
  v111 = v110[1].sym;
  __frame.sym11 = v107;
  v112 = v110->sym;
  __frame.sym12 = v111;
  v113 = v106[-2].sym;
  __frame.sym13 = v112;
  v114 = v110[-1].sym;
  __frame.sym14 = v113;
  v115 = v108[-3].sym;
  __frame.sym15 = v114;
  v116 = v106[3].sym;
  __frame.sym16 = v115;
  v117 = v106[4].sym;
  __frame.sym17 = v116;
  v118 = v108[-4].sym;
  __frame.sym18 = v117;
  v119 = v106[-3].sym;
  __frame.sym19 = v118;
  v120 = v110[2].sym;
  __frame.sym20 = v119;
  __frame.sym21 = v120;
  v121 = this_3->row_cur[8];
  __frame.sym22 = v121->sym;
  __frame.sym23 = v110[-2].sym;
  v122 = v108[-5].sym;
  v123 = v108[-7].sym;
  __frame.sym24 = v122;
  __frame.sym25 = v121[1].sym;
  v124 = v106[5].sym;
  v125 = v106[7].sym;
  __frame.sym26 = v124;
  __frame.sym27 = this_3->row_cur[9]->sym;
  __frame.sym28 = v123;
  __frame.sym29 = v121[-1].sym;
  v126 = v110[3].sym;
  this_3->sym_pos = 0;
  __frame.sym30 = v125;
  __frame.sym31 = v126;
  do
  {
    v127 = __pixel_context((ModelBlock *)this_3, (uint32_t *)__frame.sym);
    if ( v127 >= 0 )
    {
      v128 = __decode_context_bit(&this_3->bit_node[this_3->ctr_node], &this_3->bit_root[this_3->ctr_fallback]);
      this_3->row_cur[5]->sym = v127;
      if ( v128 )
        return n15_24 + 1;
      exclusion_mask[v127] = exclusion_gen;
    }
    n32 = this_3->sym_pos + 1;
    this_3->sym_pos = n32;
  }
  while ( n32 < 32 );
  n15_25 = n15_24;
  n4_21 = ::mode_symbol[1];
  sel1_list = this_3->sel1_list;
  this_3->sel[0] = &this_3->sel0_list[::mode_symbol[2]];
  v134 = this_3->sel_cur;
  this_3->sel[1] = &sel1_list[n4_21];
  while ( 1 )
  {
    if ( (*v134)->live )
    {
      v135 = __decode_symbol_list(*v134);
      this_3->row_cur[5]->sym = v135;
      if ( v135 >= 0 )
        return n15_25 + 1;
      v134 = this_3->sel_cur;
    }
    this_3->sel_cur = ++v134;
  }
}

int32_t __code_pixel(ModelBlock *_this, int32_t a2)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW segfaults while compressing.
  struct alignas(16) CodePixelFrame {   // 164 bytes, one stack frame
      union {
          uint32_t  sym[32];   // pixel_context reads sym[0..31]; `sym_pos` runs 0..31 (was p_n15 .. v207)
          struct {   // the locals MSVC spilled into these bytes
            int32_t sym0;
            int32_t sym1;
            uint16_t *sym2;
            int32_t sym3;
            int32_t sym4;
            int32_t sym5;
            int32_t sym6;
            ModelBlock *sym7;
            int32_t sym8;
            PixRec *sym9;
            int32_t sym10;
            int32_t sym11;
            int32_t sym12;
            int32_t sym13;
            int32_t sym14;
            int32_t sym15;
            int32_t sym16;
            int32_t sym17;
            int32_t sym18;
            int32_t sym19;
            int32_t sym20;
            int32_t sym21;
            int32_t sym22;
            int32_t sym23;
            int32_t sym24;
            int32_t sym25;
            int32_t sym26;
            int32_t sym27;
            int32_t sym28;
            int32_t sym29;
            int32_t sym30;
            int32_t sym31;
          };
      };
      uint8_t   _gap0[4];   // was int32_t n15_14
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t n15_14;
  ;
  PixRec *v63;
  PixRec *v20, *v22, *n2_9, *v52, *v109;   // row cursors out of ModelBlock
  uint8_t *alpha_map;
  FreqRec *n15_36, *n15_38, *n15_40;   // the coder's frequency record
  uint16_t *v59;   // a copy of `pix_cur`
  bool v11;
  int8_t v93;
  uint8_t match1, v64, v65, v66, v67;
  uint8_t *v156;   // `uint8_t *` beside the `char` scalars above
  uint16_t *v36;   // a cursor into `ctx_id3`
  int16_t v14, n15_10, v147, v158, n15_21, v170;
  ModelBlock *this_3;
  ModelBlock *this_4;
  ModelBlock *this_2;
  SymPair *v16;   // the group's counter pair for this context
  // A cumulative count, a high count and a total: the three arguments the
  // range coder takes, and it takes them unsigned.
  uint32_t arg_cum, arg_high, arg_tot;
  int32_t n4, n4_1, __code_pixel_n15, n15_1, v10, v13, ctx_state, n15_6, n15_5,
          n15_8, ctx_bucket, p_n15_1, n15_30, v26, v27, v29, n0xFFFF, v32, n0xFFFF_1,
          n53248, v35, v38, v39, v42, p_n15_2, n15_42, run, v46, n15_32,
          n15_12, n15_9, row_cur9, v53, p_n15_4, n2_10, v61, n15_11, n15_13, n4_2,
          n15_33, n15_35, n15_34, n15_15, n2_15, v82, __code_pixel_n0x2000,
          n15_16, v87, n2, p_n15_5, p_n15_7, n15_17, n15_22, excl_sym_a,
          excl_sym_b, p_n15_11, n2_5, n15_41, n15_23, n15_24, n15_25, n15_26,
          n15_27, v112, v114, v115, v116, v117, v118, v119, v120, v121, v122,
          v123, v125, v126, v127, v128, v129, n15_28, n32, n15_29, n4_5,
          p_n15_6, *v143, b15, n2_16, n2_17, n15_37, w5, v150, v151, v152,
          v154, v155, v157, n15_18, v161, n2_2, p_n15_10, p_n15_8, p_n15_9,
          n15_19;
  FreqRec *n2_3;
  uint16_t *v159;
  // `row_cur[6]`, the row above: every reach through it is a multiple of
  // four `uint16_t`, which is one `PixRec`, and every one is `sym`.
  PixRec *v111;
  uint16_t *v51;
  PixRec *v37;   // `row_cur[5]`, the current row
  PixRec *n2_7;   // `row_cur[6]`, the row above
  PixRec *n2_8, *n2_11, *n2_12, *v113, *v124;   // row cursors out of ModelBlock
  uint16_t *n2_14, *v97, *sym_cache, *n2_6, v153;
  PixRec *n2_13;   // `row_cur[5]`, one record past the pixel just written
  SymList *sel1_list;
  SymList **v136;
  uint32_t bin_tot, v55, v57, v137, p_n15_12, w0, v140, v141, w1, v169, v171, v172,
          v173;
  PixRec *v69;      // `row_cur[7]`, two rows above
  PixRec *p_n15_3, *v41;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  n2_7 = (PixRec *)_this->row_cur[6];
  n4 = n2_7->sym;
  n2_8 = _this->row_cur[5];
  n4_1 = n2_8[-1].sym;
  __code_pixel_n15 = n2_7[1].sym;
  __frame.sym7 = (ModelBlock *)(_this);
  n15_1 = n2_7[-1].sym;
  __frame.sym8 = n4;
  ::mode_symbol[1] = n4;
  __frame.sym10 = n4_1;
  ::mode_symbol[2] = n4_1;
  __frame.sym6 = __code_pixel_n15;
  ::mode_symbol[3] = __code_pixel_n15;
  __frame.sym5 = n15_1;
  mode_symbol[4] = n15_1;
  v10 = 32 * n2_8[-1].match[2]
      + 16 * n2_8[-1].match[4]
      + 2 * n2_8[-1].match[0]
      + 8 * n2_7[1].match[1]
      + (n2_7->match[1] + 4 * (__code_pixel_n15 == n15_1));
  v11 = n4 == n4_1;
  this_2 = (ModelBlock *)(__frame.sym7);
  if ( v11 )
  {
    if ( __frame.sym8 == __frame.sym6 )
    {
      v14 = *((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720);
      if ( __frame.sym8 == __frame.sym5 )
        v13 = (uint16_t)(v14 - n2_8[-2].sym);
      else
        v13 = (uint16_t)(v14 - __frame.sym5);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720) - __frame.sym6);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720) - __frame.sym10);
  }
  __frame.sym7->sym_cache = &__frame.sym7->sym_ctr[8 * v13];
  ctx_state = this_2->ctx_state[v10];
  *(int32_t *)&this_2->f36 = ctx_state;
  v16 = &this_2->group_ctr[ctx_state][v13];
  this_2->pix_cur = (uint16_t *)v16;
  n15_6 = v16->last;
  if ( n15_6 == __frame.sym8 )
  {
    n15_5 = 15;
  }
  else if ( n15_6 == __frame.sym10 )
  {
    n15_5 = 30;
  }
  else if ( n15_6 == __frame.sym6 )
  {
    n15_5 = 45;
  }
  else
  {
    v11 = n15_6 == __frame.sym5;
    n15_5 = 60;
    if ( !v11 )
      n15_5 = 0;
  }
  n15_8 = v16->prev;
  if ( n15_8 == __frame.sym8 )
  {
    n15_5 += 75;
  }
  else if ( n15_8 == __frame.sym10 )
  {
    n15_5 += 150;
  }
  else if ( n15_8 == __frame.sym6 )
  {
    n15_5 += 225;
  }
  else if ( n15_8 == __frame.sym5 )
  {
    n15_5 += 300;
  }
  v20 = this_2->row_cur[8];
  __frame.sym1 = (int32_t)n2_7;
  ctx_bucket = this_2->ctx_bucket[ctx_state + n15_5];
  v22 = this_2->row_cur[7];
  *(int32_t *)&this_2->bucket_idx = ctx_bucket;
  p_n15_1 = n2_7->match[0];
  match1 = n2_7->match[1];
  __frame.sym2 = (uint16_t *)n2_8;
  __frame.sym7 = (ModelBlock *)(this_2);
  __frame.sym0 = p_n15_1;
  n15_30 = (int32_t)this_2->row_cur[9];
  v26 = n2_8[-1].match[1];
  __frame.sym3 = n15_30;
  v27 = 8 * n2_8[-2].match[2]
      + 4 * n2_8[-2].match[5]
      + v26
      + 2 * n2_8[-2].match[4];
  this_3 = (ModelBlock *)(__frame.sym7);
  v29 = ((__frame.sym7->grad[3] == 0) << 7)
      + ((__frame.sym7->grad[2] == 0) << 6)
      + 32 * (__frame.sym7->grad[1] == 0)
      + 16 * (__frame.sym7->grad[0] == 0)
      + ((uint8_t)(((PixRec *)__frame.sym3)->match[0] & v20->match[0] & __frame.sym0 & v22->match[0]) << 9)
      + ((uint8_t)(((PixRec *)__frame.sym3)->match[1] & v20->match[1] & v22->match[1] & match1) << 8)
      + (ctx_bucket << 10)
      + v27;
  n0xFFFF = __frame.sym7->ctx_id1[v29];
  n2_9 = (PixRec *)__frame.sym1;
  if ( n0xFFFF == 0xFFFF )
  {
    __frame.sym7->ctx_id1[v29] = *(int32_t *)&__frame.sym7->ctx_id1_used;
    n2_9 = this_3->row_cur[6];
    n2_8 = this_3->row_cur[5];
    ++*(int32_t *)&this_3->ctx_id1_used;
    n0xFFFF = this_3->ctx_id1[v29];
    __frame.sym0 = n2_9->match[0];
  }
  v32 = n2_8[-1].match[5] + 4 * n2_9[1].match[3] + 2 * __frame.sym0 + 8 * n0xFFFF;
  n0xFFFF_1 = this_3->ctx_id2[v32];
  if ( n0xFFFF_1 == 0xFFFF )
  {
    this_3->ctx_id2[v32] = (*(int32_t *)&this_3->ctx_id2_used)++;
    n0xFFFF_1 = this_3->ctx_id2[v32];
  }
  if ( *(int32_t *)&this_3->alphabet < 32 )
  {
    n53248 = *(int32_t *)&this_3->ctx_id3_used;
    v35 = 16 * n0xFFFF_1 + (__frame.sym10 & 0xF);
    v36 = &this_3->ctx_id3[v35];
    n0xFFFF_1 = *v36;
    if ( n0xFFFF_1 == 0xFFFF )
    {
      if ( n53248 > 53248 )
        v35 |= 0xFu;
      v36 = &this_3->ctx_id3[v35];
      n0xFFFF_1 = *v36;
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *v36 = n53248;
      ++*(int32_t *)&this_3->ctx_id3_used;
      n0xFFFF_1 = *v36;
    }
  }
  v37 = this_3->row_cur[5];
  v38 = v37[-1].match[1];
  v39 = v37[-1].match[0];
  __frame.sym9 = v37;
  if ( (v38 & v39) != 0
    && (p_n15_3 = this_3->row_cur[6],
        v41 = this_3->row_cur[7],
        ((uint8_t)(v41[2].match[1]
                         & v41[1].match[1]
                         & v41[0].match[1]
                         & p_n15_3[3].match[1]
                         & p_n15_3[2].match[1]
                         & p_n15_3[1].match[1]
                         & p_n15_3[0].match[1]
                         & p_n15_3[0].match[0]
                         & p_n15_3[-1].match[1])
       & v41[3].match[1]) != 0) )
  {
    v42 = v41[0].match[0];
    p_n15_2 = *(int32_t *)&this_3->width - a2;
    __frame.sym4 = 1;
    if ( p_n15_2 <= 1 )
    {
      run = 1;
    }
    else
    {
      __frame.sym0 = p_n15_2;
      n15_42 = 1;
      __frame.sym7 = (ModelBlock *)(this_3);
      while ( 1 )
      {
        run = n15_42;
        if ( (p_n15_3[n15_42 + 2].match[1] & p_n15_3[n15_42 + 2].match[0]) == 0 )
          break;
        v42 = (uint8_t)(v41[run].match[0] & v42);
        if ( ++n15_42 >= __frame.sym0 )
        {
          this_3 = (ModelBlock *)(__frame.sym7);
          __frame.sym4 = n15_42;
          run = n15_42;
          goto LABEL_42;
        }
      }
      this_3 = (ModelBlock *)(__frame.sym7);
      __frame.sym4 = n15_42;
    }
LABEL_42:
    v46 = (uint8_t)(v41[run + 3].match[1] & v41[run + 2].match[1]);
    __frame.sym5 = *(uint8_t *)(*(int32_t *)&this_3->run_bucket + __frame.sym4);
    n15_32 = *(uint8_t *)(*(int32_t *)&this_3->alpha_map + __frame.sym8) + 8 * __frame.sym5 + 4 * v46 + 2 * v42;
    n15_12 = 0;
    if ( __frame.sym9->sym == __frame.sym8 )
    {
      __frame.sym0 = (int32_t)p_n15_3;
      __frame.sym7 = (ModelBlock *)(this_3);
      do
        ++n15_12;
      while ( n15_12 < __frame.sym4 && __frame.sym9[n15_12].sym == __frame.sym8 );
      this_3 = (ModelBlock *)(__frame.sym7);
      p_n15_3 = (PixRec *)__frame.sym0;
    }
    n15_9 = n15_12 == __frame.sym4;
    __frame.sym6 = n15_9;
    if ( n15_12 > n15_9 )
    {
      this_3->row_cur[6] = &p_n15_3[n15_12 - n15_9];
      this_3->row_cur[7] = &v41[n15_12 - n15_9];
      row_cur9 = (int32_t)this_3->row_cur[9];
      this_3->row_cur[8] = this_3->row_cur[8] + n15_12 - n15_9;
      v51 = (uint16_t *)__frame.sym9;
      this_3->row_cur[9] = (PixRec *)row_cur9 + n15_12 - n15_9;
      *((uint32_t *)v51 + 1) = 0x01010101;
      *(uint32_t *)this_3->row_cur[5] = 0x01010101;
      this_3->pix_cur[1] = this_3->pix_cur[0];
      LOWORD(v51) = __frame.sym8;
      this_3->row_cur[5]->sym = __frame.sym8;
      this_3->pix_cur[0] = (uint16_t)(uintptr_t)v51;
      v52 = this_3->row_cur[5];
      v53 = *(uint32_t *)v52;
      __frame.sym1 = *(uint32_t *)&v52->match[2];
      __frame.sym2 = (uint16_t *)(v52 + 1);
      this_3->row_cur[5] = v52 + 1;
      if ( n15_12 - n15_9 != 1 )
      {
        p_n15_4 = n15_12 - n15_9 - 1;
        v55 = p_n15_4 / 2;
        if ( p_n15_4 / 2 )
        {
          __frame.sym0 = p_n15_4;
          n2_10 = __frame.sym1;
          n15_14 = n15_12;
          v57 = 0;
          __frame.sym3 = n15_32;
          n15_10 = __frame.sym8;
          do
          {
            this_3->pix_cur[1] = n15_10;
            *(uint32_t *)this_3->row_cur[5] = v53;
            *(uint32_t *)&this_3->row_cur[5]->match[2] = n2_10;
            v59 = this_3->pix_cur;
            ++this_3->row_cur[5];
            v59[1] = n15_10;
            *(uint32_t *)this_3->row_cur[5] = v53;
            *(uint32_t *)&this_3->row_cur[5]->match[2] = n2_10;
            n2_11 = this_3->row_cur[5] + 1;
            this_3->row_cur[5] = n2_11;
            ++v57;
          }
          while ( v57 < v55 );
          p_n15_4 = __frame.sym0;
          n15_32 = __frame.sym3;
          __frame.sym2 = (uint16_t *)n2_11;
          n15_12 = n15_14;
          v61 = 2 * v57 + 1;
        }
        else
        {
          v61 = 1;
        }
        if ( p_n15_4 > (uint32_t)(v61 - 1) )
        {
          this_3->pix_cur[1] = __frame.sym8;
          *(uint32_t *)this_3->row_cur[5] = v53;
          *(uint32_t *)&this_3->row_cur[5]->match[2] = __frame.sym1;
          n2_12 = this_3->row_cur[5] + 1;
          this_3->row_cur[5] = n2_12;
          __frame.sym2 = (uint16_t *)n2_12;
        }
      }
      v63 = (PixRec *)this_3->row_cur[6];
      v64 = v63[-2].match[0];
      v65 = v63[2].match[0];
      v66 = v63[4].match[0];
      __frame.sym3 = n15_32;
      v67 = v63[-3].match[0];
      n15_14 = n15_12;
      v69 = (PixRec *)this_3->row_cur[7];
      this_3->grad[0] = (v63[3].match[0] + v65 + v64 + v67 + v66 - 5);
      n2_13 = (PixRec *)__frame.sym2;
      // Eight records of the row two above, `match[0]` in each: the
      // byte offsets 2, 10, 18, 26, 34 and -6, -14, -22 were records
      // 0..4 and -1..-3.  Three of the eight loads are `movsx` in the
      // original and five are `movzx`; every writer of these bytes is
      // a comparison, so the sign never shows, and the casts stay
      // because the instruction is what is being transcribed.
      this_3->grad[1] = v69[3].match[0]
                                   + v69[2].match[0]
                                   + v69[1].match[0]
                                   + v69[0].match[0]
                                   + (int8_t)v69[-1].match[0]
                                   + (int8_t)v69[-2].match[0]
                                   + (int8_t)v69[-3].match[0]
                                   + v69[4].match[0]
                                   - 8;
      this_3->grad[2] = n2_13[-4].match[1] + n2_13[-3].match[1] - 2;
      n15_11 = __frame.sym8;
      this_3->grad[3] = n2_13[-5].match[0]
                                   + n2_13[-6].match[0]
                                   + n2_13[-7].match[0]
                                   + n2_13[-4].match[0]
                                   - 4;
      n2_13[-1].match[4] = n15_11 == v63[1].sym;
      n15_32 = __frame.sym3;
      this_3->row_cur[5][-1].match[5] = n15_11 == this_3->row_cur[6][2].sym;
      n15_12 = n15_14;
    }
    __encode_context_bit(&this_3->esc_ctr[(n15_32 + 1)], this_3->esc_ctr, __frame.sym6);
    n15_13 = __frame.sym6;
    n4_2 = ::mode_symbol[1];
    alpha_map = (uint8_t *)this_3->alpha_map;
    *(int32_t *)&this_3->hit = __frame.sym6;
    *(alpha_map + n4_2) = n15_13;
    if ( !n15_13 && __frame.sym4 != 1 )
    {
      n15_14 = n15_12;
      __frame.sym0 = __frame.sym5;
      __frame.sym7 = (ModelBlock *)(this_3);
      n15_33 = __frame.sym4;
      n15_35 = 1 << (__frame.sym5 & 31);
      n15_34 = 0;
      n15_15 = __frame.sym5;
      do
      {
        if ( n15_33 > (n15_35 | n15_34) )
        {
          __frame.sym3 = n15_34;
          n2_14 = (uint16_t *)&__frame.sym7->run_ctr[16 * ((n15_34 == 0) + (n15_15 == __frame.sym0)) + n15_15];
          __frame.sym2 = n2_14;
          __frame.sym1 = n15_14 & n15_35;
          bin_tot = *n2_14 + n2_14[1];
          rc.encode_bit(*n2_14, n2_14[1], (n15_14 & n15_35) != 0);
          if ( __frame.sym7->run_ctr[16 * ((n15_34 == 0) + (n15_15 == __frame.sym0)) + n15_15].limit < (uint32_t)bin_tot )
            __rescale_counter_pair((uint16_t *)__frame.sym2);
          n2_15 = __frame.sym1;
          __frame.sym4 = n15_35;
          __frame.sym2[__frame.sym1 != 0] += 8;
          n15_34 |= n2_15;
          n15_35 = __frame.sym4;
        }
        --n15_15;
        n15_35 >>= 1;
      }
      while ( n15_35 );
      n15_12 = n15_14;
      this_3 = (ModelBlock *)(__frame.sym7);
    }
    if ( *(int32_t *)&this_3->hit )
      return n15_12;
  }
  else
  {
    v82 = 4 * *(int32_t *)&this_3->bucket_idx;
    n15_36 = (FreqRec *)&this_3->row_cur[v82 + 10];
    __frame.sym4 = (int32_t)(uintptr_t)n15_36;
    n2_3 = &this_3->grid[n0xFFFF_1 + 188];
    __code_pixel_n0x2000 = HIWORD(((int32_t *)this_3)[4 * n0xFFFF_1 + 778]);
    if ( __code_pixel_n0x2000 )
    {
      if ( __code_pixel_n0x2000 == 1 )
      {
        v143 = (int32_t *)&this_3->row_cur[v82 + 10];
        b15 = n15_36->b15;
        n2_16 = b15 * n2_3->w[2];
        n2_17 = b15 * n2_3->w[3];
        __frame.sym0 = b15 * n2_3->w[0];
        v147 = b15 * n2_3->w[1];
        __frame.sym1 = n2_16;
        n15_37 = b15 * n2_3->w[4];
        __frame.sym2 = (uint16_t *)n2_17;
        __frame.sym3 = n15_37;
        *n2_3 = *(FreqRec *)v143;
        w5 = n2_3->w[5];
        n2_3->b14 *= 8;
        __frame.sym7 = (ModelBlock *)(this_3);
        v150 = 21 * n2_3->w[1];
        __frame.sym0 += (21 * n2_3->w[0] + w5 - 1) / w5;
        n2_3->w[0] = __frame.sym0;
        v151 = (v150 + w5 - 1) / w5;
        LOWORD(v150) = __frame.sym1;
        v152 = 21 * n2_3->w[2];
        v153 = v151 + v147;
        n2_3->w[1] = v153;
        v154 = (v152 + w5 - 1) / w5;
        v155 = 21 * n2_3->w[3];
        LOWORD(v150) = v154 + v150;
        n2_3->w[2] = v150;
        v156 = (uint8_t *)__frame.sym2 + (v155 + w5 - 1) / w5;
        v157 = 21 * n2_3->w[4];
        n2_3->w[3] = (uint16_t)(uintptr_t)v156;
        v158 = (uint16_t)(uintptr_t)v156 + v150 + v153;
        this_3 = (ModelBlock *)(__frame.sym7);
        LOWORD(w5) = (v157 + w5 - 1) / w5 + __frame.sym3;
        n2_3->w[4] = w5;
        __code_pixel_n0x2000 = (uint16_t)(__frame.sym0 + w5 + v158);
        n2_3->w[5] = __code_pixel_n0x2000;
        __frame.sym9 = this_3->row_cur[5];
      }
      n15_16 = __frame.sym9->sym;
      arg_tot = __code_pixel_n0x2000;
      if ( n15_16 == __frame.sym8 )
      {
        v87 = n2_3->w[0];
        n2 = 1;
      }
      else if ( n15_16 == __frame.sym10 )
      {
        v87 = n2_3->w[0] + n2_3->w[1];
        n2 = 2;
      }
      else if ( n15_16 == __frame.sym6 )
      {
        v87 = n2_3->w[0] + n2_3->w[2] + n2_3->w[1];
        n2 = 3;
      }
      else if ( n15_16 == __frame.sym5 )
      {
        v87 = n2_3->w[5] - n2_3->w[4];
        n2 = 4;
      }
      else
      {
        v87 = 0;
        n2 = 0;
      }
      p_n15_5 = n2_3->w[6];
      arg_cum = v87;
      arg_high = n2_3->w[n2] + v87;
      p_n15_7 = n2_3->w[5];
      n15_17 = n2_3->b15;
      if ( p_n15_7 > p_n15_5
        && (n2_3->w[n2] + n15_17 + 8 < p_n15_7 || n2_3->w[5] > 0x4000u) )
      {
        v137 = n2_3->w[2];
        __frame.sym0 = p_n15_5;
        p_n15_12 = n2_3->w[1];
        __frame.sym7 = (ModelBlock *)(this_3);
        __frame.sym1 = n2;
        w0 = n2_3->w[0];
        __frame.sym2 = (uint16_t *)n15_17;
        LOWORD(w0) = w0 - (w0 >> 1);
        n2_3->w[0] = w0;
        LOWORD(p_n15_12) = p_n15_12 - (p_n15_12 >> 1);
        n2_3->w[1] = p_n15_12;
        LOWORD(v137) = v137 - (v137 >> 1);
        v140 = n2_3->w[3];
        n2_3->w[2] = v137;
        LOWORD(v140) = v140 - (v140 >> 1);
        v141 = n2_3->w[4];
        n2_3->w[3] = v140;
        LOWORD(v141) = v141 - (v141 >> 1);
        n2_3->w[4] = v141;
        LOWORD(p_n15_12) = v140 + v137 + p_n15_12;
        n15_17 = (int32_t)__frame.sym2;
        LOWORD(p_n15_12) = w0 + v141 + p_n15_12;
        n2 = __frame.sym1;
        this_3 = (ModelBlock *)(__frame.sym7);
        p_n15_7 = (uint16_t)p_n15_12;
        p_n15_6 = __frame.sym0;
        n2_3->w[5] = p_n15_7;
        if ( p_n15_6 < 256 && !n2_3->b14 )
        {
          p_n15_6 = 256;
          n2_3->w[6] = 256;
        }
        if ( p_n15_7 > p_n15_6 )
        {
          if ( n15_17 < 15 )
            LOWORD(n15_17) = 15;
          n2_3->b15 = n15_17;
        }
      }
      n2_3->w[5] = n15_17 + p_n15_7;
      n2_3->w[n2] += n15_17;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->hit = n2;
      if ( n2_3->b14 )
      {
        --n2_3->b14;
        n15_38 = n15_36;
        ++n15_36->w[5];
        ++n15_38->w[n2];
        n2 = *(int32_t *)&this_3->hit;
      }
    }
    else
    {
      v159 = ((uint16_t *)&this_3->row_cur[v82 + 10]);
      n15_18 = __frame.sym9->sym;
      arg_tot = n15_36->w[5];
      if ( n15_18 == __frame.sym8 )
      {
        v161 = *v159;
        n2_2 = 1;
      }
      else if ( n15_18 == __frame.sym10 )
      {
        v161 = *v159 + v159[1];
        n2_2 = 2;
      }
      else if ( n15_18 == __frame.sym6 )
      {
        v161 = *v159 + v159[2] + v159[1];
        n2_2 = 3;
      }
      else if ( n15_18 == __frame.sym5 )
      {
        v161 = v159[5] - v159[4];
        n2_2 = 4;
      }
      else
      {
        v161 = 0;
        n2_2 = 0;
      }
      arg_cum = v161;
      arg_high = n15_36->w[n2_2] + v161;
      p_n15_10 = n15_36->w[5];
      p_n15_8 = n15_36->w[6];
      __frame.sym3 = n15_36->b15;
      if ( p_n15_10 > p_n15_8 && (n15_36->w[n2_2] + __frame.sym3 + 8 < p_n15_10 || p_n15_10 > 0x4000) )
      {
        __frame.sym0 = p_n15_8;
        __frame.sym7 = (ModelBlock *)(this_3);
        w1 = n15_36->w[1];
        v169 = n15_36->w[2];
        __frame.sym1 = (int32_t)(uintptr_t)n2_3;
        __frame.sym2 = (uint16_t *)n2_2;
        v170 = n15_36->w[0] - (n15_36->w[0] >> 1);
        n15_36->w[0] = v170;
        LOWORD(w1) = w1 - (w1 >> 1);
        n15_36->w[1] = w1;
        v171 = v169 - (v169 >> 1);
        v172 = (uint16_t)n15_36->w[3];
        n15_36->w[2] = v171;
        LOWORD(v172) = v172 - (v172 >> 1);
        v173 = (uint16_t)n15_36->w[4];
        n15_36->w[3] = v172;
        LOWORD(v173) = v173 - (v173 >> 1);
        n15_36->w[4] = v173;
        LOWORD(v173) = v170 + v173;
        n2_2 = (int32_t)__frame.sym2;
        p_n15_10 = (uint16_t)(v173 + v172 + v171 + w1);
        p_n15_9 = __frame.sym0;
        n2_3 = (FreqRec *)__frame.sym1;
        n15_36->w[5] = p_n15_10;
        this_3 = (ModelBlock *)(__frame.sym7);
        if ( p_n15_9 < 256 && !n15_36->b14 )
        {
          p_n15_9 = 256;
          n15_36->w[6] = 256;
        }
        if ( p_n15_10 > p_n15_9 )
        {
          n15_19 = __frame.sym3;
          if ( __frame.sym3 < 15 )
            n15_19 = 15;
          __frame.sym3 = n15_19;
          n15_36->b15 = n15_19;
        }
      }
      n15_21 = __frame.sym3;
      n15_40 = n15_36;
      n15_36->w[5] = __frame.sym3 + p_n15_10;
      n15_40->w[n2_2] += n15_21;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->hit = n2_2;
      n2_3->w[5] = (n2_3->w[n2_2])++ != 0;
      n2 = *(int32_t *)&this_3->hit;
    }
    if ( n2 )
      return 1;
    n15_12 = 0;
  }
  v93 = exclusion_gen;
  n15_22 = ::mode_symbol[3];
  excl_sym_a = ::mode_symbol[2];
  excl_sym_b = ::mode_symbol[1];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  exclusion_mask[n15_22] = v93;
  v97 = (uint16_t *)this_3->pix_cur;
  exclusion_mask[excl_sym_a] = v93;
  exclusion_mask[excl_sym_b] = v93;
  sym_cache = this_3->sym_cache;
  __byte_445440[0] = v93;
  n15_14 = n15_12;
  this_3->sel[0] = nullptr;
  p_n15_11 = *v97;
  n2_5 = v97[1];
  n2_6 = (uint16_t *)*sym_cache;
  n15_41 = sym_cache[2];
  __frame.sym0 = p_n15_11;
  n15_23 = sym_cache[1];
  __frame.sym1 = n2_5;
  n15_24 = sym_cache[3];
  __frame.sym2 = n2_6;
  n15_25 = sym_cache[4];
  __frame.sym3 = n15_23;
  this_4 = (ModelBlock *)((int32_t *)sym_cache[5]);
  __frame.sym4 = n15_41;
  n15_26 = sym_cache[6];
  __frame.sym5 = n15_24;
  v109 = this_3->row_cur[5];
  __frame.sym6 = n15_25;
  n15_27 = v109[-2].sym;
  __frame.sym7 = (ModelBlock *)(this_4);
  v111 = (PixRec *)this_3->row_cur[6];
  __frame.sym8 = n15_26;
  v112 = v111[2].sym;
  __frame.sym9 = (PixRec *)(((uint16_t *)sym_cache[7]));
  __frame.sym10 = n15_27;
  v113 = this_3->row_cur[7];
  v114 = v113[1].sym;
  __frame.sym11 = v112;
  v115 = v113->sym;
  __frame.sym12 = v114;
  v116 = v111[-2].sym;
  __frame.sym13 = v115;
  v117 = v113[-1].sym;
  __frame.sym14 = v116;
  v118 = v109[-3].sym;
  __frame.sym15 = v117;
  v119 = v111[3].sym;
  __frame.sym16 = v118;
  v120 = v111[4].sym;
  __frame.sym17 = v119;
  v121 = v109[-4].sym;
  __frame.sym18 = v120;
  v122 = v111[-3].sym;
  __frame.sym19 = v121;
  v123 = v113[2].sym;
  __frame.sym20 = v122;
  __frame.sym21 = v123;
  v124 = this_3->row_cur[8];
  __frame.sym22 = v124->sym;
  __frame.sym23 = v113[-2].sym;
  v125 = v109[-5].sym;
  v126 = v109[-7].sym;
  __frame.sym24 = v125;
  __frame.sym25 = v124[1].sym;
  v127 = v111[5].sym;
  v128 = v111[7].sym;
  __frame.sym26 = v127;
  __frame.sym27 = this_3->row_cur[9]->sym;
  __frame.sym28 = v126;
  __frame.sym29 = v124[-1].sym;
  __frame.sym30 = v128;
  __frame.sym31 = v113[3].sym;
  *(int32_t *)&this_3->sym_pos = 0;
  do
  {
    v129 = __pixel_context((ModelBlock *)this_3, (uint32_t *)__frame.sym);
    if ( v129 >= 0 )
    {
      n15_28 = v129 == this_3->row_cur[5]->sym;
      __encode_context_bit(&this_3->bit_node[*(int32_t *)&this_3->ctr_node], &this_3->bit_root[*(int32_t *)&this_3->ctr_fallback], n15_28);
      if ( n15_28 )
        return n15_14 + 1;
      exclusion_mask[v129] = exclusion_gen;
    }
    n32 = *(int32_t *)&this_3->sym_pos + 1;
    *(int32_t *)&this_3->sym_pos = n32;
  }
  while ( n32 < 32 );
  n15_29 = n15_14;
  n4_5 = ::mode_symbol[1];
  sel1_list = this_3->sel1_list;
  this_3->sel[0] = &this_3->sel0_list[::mode_symbol[2]];
  v136 = this_3->sel_cur;
  this_3->sel[1] = &sel1_list[n4_5];
  while ( 1 )
  {
    if ( (*v136)->live )
    {
      if ( __encode_symbol_list(*v136, this_3->row_cur[5]->sym) )
        return n15_29 + 1;
      v136 = this_3->sel_cur;
    }
    this_3->sel_cur = ++v136;
  }
}

void __expand_alphabet(ModelBlock *_this)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW aborts while decompressing.
  struct alignas(16) ExpandAlphabetFrame {   // 420 bytes, one stack frame
      uint64_t v28[2];
      int32_t v29;
      uint32_t v30[91];
      uint32_t v31[5];
      uint8_t _pad0[16];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 432, "frame layout moved");
  ;
  uint32_t *v16;   // was int32_t: this holds an address
  int32_t n8, v4, v6, n16_1, n16, v25, v27;
  uint32_t j_2, i, n8193, v8, *v10, j_1, j, v15, v17, v18, v19, v20, *v22,
           v26;
  void *v12;
  n8 = _this->depth;
  j_2 = 0xFFFFFFFF >> (-(uint8_t)_this->depth & 31);
  v4 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v6 = 12 * i;
    __frame.v30[v6] = 0;
    __frame.v30[v6 + 6] = 0;
  }
  n8193 = j_2 + 1;
  if ( n8 > 8 )
    n8193 = 8193;
  v8 = __rc_decode_flat(n8193);
  _this->alphabet = v8 + 1;
  if ( (int32_t)(v8 + 1) <= 0x2000 )
  {
    v12 = bmf_new(4 * v8 + 4);
    j_1 = _this->alphabet;
    _this->sym_code = (uint32_t *)v12;
    if ( j_1 )
    {
      for ( j = 0; j < j_1; ++j )
      {
        _this->sym_code[j] = j;
        j_1 = _this->alphabet;
      }
    }
    if ( (int32_t)_this->depth > 8 )
    {
      if ( 4 * v4 )
      {
        v15 = 0;
        do
          __init_symbol_list(&((SymList *)__frame.v28)[v15++], (int32_t)_this, 256, 1);
        while ( v15 < 4 * v4 );
        j_1 = _this->alphabet;
      }
      if ( j_1 )
      {
        v16 = _this->sym_code;
        v17 = 0;
        v18 = 0;
        do
        {
          v16[v18] = 0;
          if ( v4 )
          {
            __frame.v31[0] = v4;
            v19 = 0;
            do
            {
              v20 = __decode_symbol_list(&((SymList *)__frame.v28)[4 * v19 + v17]);
              v17 = v20 >> 6;
              _this->sym_code[v18] += (v20 << ((8 * v19) & 31));
              ++v19;
            }
            while ( v19 < __frame.v31[0] );
            v4 = __frame.v31[0];
          }
          v16 = _this->sym_code;
          v17 = (uint8_t)v16[v18++] >> 7;
        }
        while ( v18 < _this->alphabet );
      }
    }
    else if ( j_1 <= j_2 )
    {
      __init_symbol_list((SymList *)__frame.v28, (int32_t)_this, j_2 - j_1 + 2, 1);
      __frame.v29 = 19 * ((SymList *)__frame.v28)->n;
      if ( !(_this->alphabet == 0) )
      {
        v25 = 0;
        v26 = 0;
        do
        {
          v27 = __decode_symbol_list((SymList *)__frame.v28);
          _this->sym_code[v26] = v27 + v25;
          v25 += v27 + 1;
          ++v26;
        }
        while ( v26 < _this->alphabet );
      }
    }
    v22 = __frame.v31;
    n16 = 16;
    do
    {
      v22 -= 6;
      free((void *)v22[5]);
      --n16;
    }
    while ( n16 );
  }
  else
  {
    _this->depth = 8;
    *(uint32_t *)&_this->height = (*(uint32_t *)&_this->height * v4);
    __expand_alphabet(_this);
    v10 = __frame.v31;
    n16_1 = 16;
    do
    {
      v10 -= 6;
      free((void *)v10[5]);
      --n16_1;
    }
    while ( n16_1 );
  }
}

ModelBlock *__layout_workspace(ModelBlock *a1, int32_t a2, int32_t i, int32_t a4, int32_t a5)
{
  ;
  PixRec *v8;   // one of the five row buffers
  uint8_t v12;
  int16_t v19;
  int32_t i_1, j, v9, v13, k_1, v35, v38;
  uint32_t k, m, n0x2000_1, n, n8, n0x18;
  uint8_t *v10;
  i_1 = i;
  exclusion_gen = 1;
  a1->width = i;
  *(uint32_t *)&a1->height = a4;
  a1->depth = a5;
  a1->depth_raw = a5;
  a1->escape.ent = nullptr;
  a1->sym_code = nullptr;
  for ( j = 0; j < 5; ++j )
  {
    // `8 * i_1 + 128` is the plane's width plus sixteen records: eight of
    // left margin, which is what the cursor starts past, and eight of right.
    // Every record is seeded "matches all six neighbours", so a margin read
    // off either end of the row contributes 1 and not whatever `bmf_new`
    // left there.
    v8 = (PixRec *)bmf_new(8 * i_1 + 128);
    a1->row_cur[j] = v8;
    a1->row_cur[j + 5] = v8 + 8;
    i_1 = a1->width;
    if ( (int32_t)a1->width > -16 )
    {
      v9 = 0;
      do
      {
        a1->row_cur[j][v9].sym = 0;
        a1->row_cur[j][v9].match[5] = 1;
        a1->row_cur[j][v9].match[4] = 1;
        a1->row_cur[j][v9].match[3] = 1;
        a1->row_cur[j][v9].match[2] = 1;
        a1->row_cur[j][v9].match[1] = 1;
        a1->row_cur[j][v9].match[0] = 1;
        i_1 = a1->width;
        ++v9;
      }
      while ( v9 < a1->width + 16 );
    }
  }
  v10 = (uint8_t *)bmf_new(i_1 + 1);
  a1->run_bucket = v10;
  *v10 = 0;
  if ( (int32_t)a1->width > 0 )
  {
    v12 = 0;
    v13 = 0;
    do
    {
      v12 += v13 == 2 << (v12 & 31);
      *(uint8_t *)(*(uint32_t *)&a1->run_bucket + v13++ + 1) = v12;
    }
    while ( v13 < a1->width );
  }
  // 0x2000 sixteen-bit counters cleared.  What was here instead was the same
  // range in three passes -- a scalar head to reach sixteen-byte alignment,
  // thirty-two counters an iteration, a scalar tail -- with a branch for the
  // case where `a1` is odd and no alignment is reachable at all.
  __builtin_memset(a1->sym_rev, 0, sizeof a1->sym_rev);
  for ( k = 0; k < 0x2000; ++k )
  {
    v19 = a1->sym_rev[k];
    k_1 = k;
    for ( m = 0; m < 0xD; ++m )
    {
      v19 += v19 + (k_1 & 1);
      k_1 >>= 1;
    }
    a1->sym_rev[k] = v19;
  }
  // ... and every one of them scaled by eight, the same range in the same
  // three passes.
  for ( n0x2000_1 = 0; n0x2000_1 < 0x2000; ++n0x2000_1 )
    a1->sym_rev[n0x2000_1] *= 8;
  memset(&a1->grid[188],0,0x100000);
  a1->ctx_id3_used = 0;
  a1->ctx_id2_used = 0;
  a1->ctx_id1_used = 0;
  memset(a1->ctx_id1,255,sizeof a1->ctx_id1);
  memset(a1->ctx_id2,255,sizeof a1->ctx_id2);
  memset(a1->ctx_id3,255,sizeof a1->ctx_id3);
  memset(exclusion_mask,0,8193);
  (*(uint64_t *)&a1->sel[0]) = 0;
  *(uint64_t *)&a1->escape_list = 0;
  for ( n = 0; n < 0x40000; ++n )
  {
    a1->sym_ctr[2 * n] = 0x2000;
    a1->sym_ctr[2 * n + 1] = 0x2000;
  }
  n8 = 0;
  do
  {
    v35 = 2 * n8;   // two records a pass
    a1->bit_root[v35].n[0] = 40;
    ++n8;
    a1->bit_root[v35].n[1] = 16;
    a1->bit_root[v35].limit = 512;
    a1->bit_root[v35 + 1].n[0] = 40;
    a1->bit_root[v35 + 1].n[1] = 16;
    a1->bit_root[v35 + 1].limit = 512;
  }
  while ( n8 < 8 );
  n0x18 = 0;
  memset(a1->bit_node,0,sizeof a1->bit_node);
  a1->sym_word = (uint16_t *)bmf_new(2 * a1->height * a1->width);
  a1->esc_ctr[0].n[0] = 4;
  a1->esc_ctr[0].n[1] = 4;
  a1->esc_ctr[0].limit = 72;
  memset(&a1->esc_ctr[1],0,1536);
  do
  {
    v38 = 2 * n0x18;   // two records a pass
    a1->run_ctr[v38].n[0] = 4;
    ++n0x18;
    a1->run_ctr[v38].n[1] = 4;
    a1->run_ctr[v38].limit = 72;
    a1->run_ctr[v38 + 1].n[0] = 4;
    a1->run_ctr[v38 + 1].n[1] = 4;
    a1->run_ctr[v38 + 1].limit = 72;
  }
  while ( n0x18 < 0x18 );
  return a1;
}

void __unmodel_plane_slow(ModelBlock *_this, uint8_t *Src)
{
  struct alignas(16) UnmodelPlaneSlowFrame {   // 100 bytes, one stack frame
      uint8_t   _gap0[4];   // was int32_t Size
      uint8_t   _pad1[4];
      uint8_t   _gap2[4];   // was int32_t v82
      uint8_t   _gap3[4];   // was uint8_t * v85
      uint8_t   _gap1[4];   // was int32_t v88
      uint8_t   _gap4[4];   // was uint8_t * ArgList_1
      uint8_t *row[19];   // the row-pointer array; the loop fills (&v92)[k] for k < n6, five at a time
      uint8_t   _tail[12];   // alignas(16) rounds 100 up
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 112,
                "frame layout moved");
  int32_t Size;
  // These shared `__frame.v82` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v82;
  int32_t v84;
  // These shared `__frame.v85` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v86;
  int32_t v87;
  int32_t v88;
  // These shared `__frame.ArgList` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *ArgList_5;
  int32_t n6_3;
  int32_t v93;
  int32_t v94;
  int32_t v95;
  int32_t ArgList_4;
  uint8_t *Src_1;
  ModelBlock *this_1;
  int32_t v99;
  int32_t v100;
  int32_t v101;
  int32_t v102;
  ;
  ModelBlock *this_4;
  uint32_t *v57;   // the alphabet map again
  uint8_t *ArgList, *ArgList_2, *buf, *ArgList_3, *ArgList_9, *ArgList_10,
          *Src_2, *v77, *ArgList_8;
  int16_t v20;
  PixRec *v44, *row_cur3, *row_cur2, *row_cur1, *v48;   // the five row buffers, rotated
  int32_t v5, v6, v8, n5, v14, v15, v16, v17, v18, n0x10000, v30, v34, v39,
          v40, n4_1, v43, v51, v52, v53, v54, v56, v58, v60, v61, v62, depth,
          v65, depth_raw, n6, v68, v69, n6_4, n6_1, v73, n6_2, v76, v78, v80;
  ModelBlock *this_3;
  ModelBlock *this_2;
  uint32_t *ArgList_6;
  SymListBlock *v31, *v35;
  SymPair *group_ctr;   // one group's row of counter pairs
  SymEntry *ArgList_7;
  SymList *i_1, *i, *j_1, *j;
  FreqRec *v13;   // a bucket record: `grid[bucket]`
  PixRec *v49, *v50;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  Src_1 = Src;
  ArgList = &Src[-(_this->depth < 8)];
  __rc_begin_decode(0);
  __expand_alphabet((ModelBlock *)_this);
  this_1 = (ModelBlock *)(_this);
  v102 = 0;
  v5 = 0;
  do
  {
    v6 = ctx_group_flags[v5];
    this_2 = (ModelBlock *)(this_1);
    this_1->ctx_state[v6] = v5;
    v100 = 0;
    v82 = v5;

    v88 = v6 & 4;
    v99 = v6 & 2;
    v95 = v6 & 0x10;
    v8 = 0;
    v93 = v6 & 1;

    ArgList_4 = v6 & 0x20;
    v94 = v6 & 8;
    do
    {
      v100 = v8;
      n5 = 0;

      do
      {
        this_3 = (ModelBlock *)(this_1);
        this_2->ctx_bucket[v5 + 15 * v8 + 75 * n5] = v102;
        v101 = this_3->alphabet;
        // A sixteen-byte record per bucket, at `+96 + 16 * bucket`: five
        // counts at words 0..4, their total at 5, a scaled weight at 6, and
        // two bytes at 7 -- a level (`<= f16`) and the weight `1 << (5 -
        // level)` it derives.  The base is the object and the counter starts
        // at zero, so record 0 is +96 .. +111 -- which `row_cur[10..13]` also
        // claims, and which the `f1051664[k] = row_cur[10 + k]` copy after this
        // loop reads back.  That copy is dead -- nothing reads `f1051664` --
        // so the collision costs nothing at run time, but it does mean one
        // of `row_cur`'s length and `f1051664`'s type is wrong.
        //
        // The bucket counter reaches exactly 188: a `__builtin_trap()` on
        // `>= 188` fires on fourteen of the gate's streams and one on
        // `>= 189` fires on none.  So this table is 189 records, +96 ..
        // +3119.  `FreqRec` is on the same grid from record 188 (+3104),
        // which makes the last bucket record and the first frequency record
        // the same sixteen bytes -- the two tables abut and share one.
        v13 = &this_3->grid[v102];
        v13->w[1] = 2;
        v13->w[2] = 2;
        v13->w[3] = 2;
        v13->w[4] = 2;
        if ( v88 )
        {
          v14 = (uint16_t)(v13->w[4] + v13->w[3]);
          v13->w[3] = v14;
          v15 = 0;
          v13->w[4] = 0;
        }
        else
        {
          v14 = v13->w[3];
          v15 = v13->w[4];
        }
        if ( v99 )
        {
          v16 = (uint16_t)(v15 + v13->w[2]);
          v13->w[2] = v16;
          v15 = 0;
          v13->w[4] = 0;
        }
        else
        {
          v16 = v13->w[2];
        }
        if ( v95 )
        {
          v16 = (uint16_t)(v14 + v16);
          v13->w[2] = v16;
          v14 = 0;
          v13->w[3] = 0;
        }
        if ( v93 )
        {
          v13->w[1] += v15;
          v15 = 0;
          v13->w[4] = 0;
        }
        if ( v94 )
        {
          v13->w[1] += v14;
          v14 = 0;
          v13->w[3] = 0;
        }
        if ( ArgList_4 )
        {
          v13->w[1] += v16;
          v16 = 0;
          v13->w[2] = 0;
        }
        v17 = (v14 != 0) + (v16 != 0) + (v15 != 0) + 2;
        if ( v17 <= v101 )
        {
          v13->b14 = v17;
          v13->w[0] = 2;
        }
        else
        {
          LOBYTE(v17) = v17 - 1;
          v13->b14 = v17;
          v13->w[0] = 0;
        }
        if ( v13->w[v100] && v13->w[n5] && (uint8_t)v17 <= v101 )
        {
          v18 = 1;
          v20 = (uint8_t)(1 << ((5 - v17) & 31));
          v13->b15 = v20;
          v13->w[6] = v20 << 6;
          v13->w[v100] += v20;
          v13->w[n5] += v13->b15;
          v13->w[5] = v13->w[0]
                               + v13->w[4]
                               + v13->w[3]
                               + v13->w[2]
                               + v13->w[1];
        }
        else
        {
          v18 = 0;
        }
        v102 += v18;
        ++n5;
      }
      while ( n5 < 5 );
      v8 = v100 + 1;
    }
    while ( v100 + 1 < 5 );
    n0x10000 = 0;
    group_ctr = this_1->group_ctr[v82];
    do
    {
      group_ctr[n0x10000].last = 0x2000;
      group_ctr[n0x10000++].prev = 0x2000;
    }
    while ( n0x10000 < 0x10000 );
    v5 = v82 + 1;
  }
  while ( v82 + 1 < 15 );
  ArgList_2 = ArgList;
  this_4 = (ModelBlock *)((int32_t)this_1);
  buf = (uint8_t *)bmf_new(this_1->alphabet);
  Size = this_1->alphabet;
  this_1->alpha_map = (uint8_t *)buf;
  memset(buf,1,Size);
  this_4->escape_list = &this_4->escape;
  __init_symbol_list(&this_4->escape, (int32_t)this_4, this_4->alphabet, 1);
  this_4->sel_cur = this_4->sel;
  // `24 * n + 4`: the count word, then `n` lists.  `free_workspace` reads the
  // count back from `sym_list_count(lists)`.
  v30 = this_4->alphabet;
  v31 = (SymListBlock *)bmf_new(24 * v30 + 4);
  if ( v31 )
  {
    v31->n = v30;
    i_1 = v31->list;
    for ( i = i_1; v30; --v30 )
      (i_1++)->ent = nullptr;
  }
  else
  {
    i = nullptr;
  }
  v34 = this_4->alphabet;
  this_4->sel1_list = i;
  v35 = (SymListBlock *)bmf_new(24 * v34 + 4);
  if ( v35 )
  {
    v35->n = v34;
    j_1 = v35->list;
    for ( j = j_1; v34; --v34 )
      (j_1++)->ent = nullptr;
  }
  else
  {
    j = nullptr;
  }
  this_4->sel0_list = j;
  if ( !(this_4->alphabet <= 0) )
  {
    v39 = 0;
    do
    {
      __init_symbol_list(&this_4->sel1_list[v39], (int32_t)this_4, 99, 0);
      __init_symbol_list(&this_4->sel0_list[v39++], (int32_t)this_4, 33, 0);
    }
    while ( v39 < this_4->alphabet );
  }
  v40 = this_4->depth;
  if ( v40 == this_4->depth_raw )
  {
    ArgList_3 = nullptr;
  }
  else
  {
    ArgList_2 = (uint8_t *)bmf_new(*(uint32_t *)&this_4->height * this_4->width + 3);
    v40 = this_4->depth;
    ArgList_3 = ArgList_2;
  }
  n4_1 = (v40 + 7) >> 3;
  if ( this_4->height > 0 )
  {
    ArgList_4 = (int32_t)ArgList_3;
    ArgList_5 = ArgList_2;
    v43 = 0;
    while ( 1 )
    {
      v86 = v43;
      this_4->row_cur[5]->match[1] = this_4->row_cur[5][-1].sym == 0;
      this_4->row_cur[5]->match[3] = this_4->row_cur[6][-1].sym == 0;
      v44 = this_4->row_cur[4];
      row_cur3 = this_4->row_cur[3];
      row_cur2 = this_4->row_cur[2];
      row_cur1 = this_4->row_cur[1];
      v48 = this_4->row_cur[0];
      this_4->row_cur[4] = row_cur3;
      this_4->row_cur[3] = row_cur2;
      this_4->row_cur[2] = row_cur1;
      this_4->row_cur[1] = v48;
      this_4->row_cur[0] = v44;
      v44 += 7;
      this_4->row_cur[5] = v44;
      v48 += 7;
      this_4->row_cur[6] = v48;
      this_4->row_cur[7] = row_cur1 + 7;
      this_4->row_cur[8] = row_cur2 + 7;
      this_4->row_cur[9] = row_cur3 + 7;
      // Two "is this count zero" flags, written to three and two places.
      // MSVC put each in the low byte of a register that held a cursor, which
      // is where `LOBYTE(v48) = ...` came from; neither cursor is read again.
      {
        uint8_t zero = v48[1].sym == 0;
        v44->match[2] = zero;
        this_4->row_cur[5][-1].match[4] = zero;
        this_4->row_cur[5][-2].match[5] = zero;
        zero = this_4->row_cur[6][2].sym == 0;
        this_4->row_cur[5]->match[4] = zero;
        this_4->row_cur[5][-1].match[5] = zero;
      }
      this_4->row_cur[5]->match[5] = this_4->row_cur[6][3].sym == 0;
      v49 = this_4->row_cur[6];
      v50 = this_4->row_cur[7];
      ++this_4->row_cur[5];
      ++v49;
      ++this_4->row_cur[8];
      this_4->row_cur[6] = v49;
      ++v50;
      this_4->row_cur[7] = v50;
      ++this_4->row_cur[9];
      this_4->grad[0] = v49[3].match[0] + v49[2].match[0] + v49[1].match[0] + v49[0].match[0] + v49[4].match[0] - 5;
      // The same five counts as the line above, off the other row.  MSVC
      // spilled each byte into a register whose upper bits were leftovers,
      // and the destination is one byte, so only the low bytes ever counted.
      this_4->grad[3] = 0;
      this_4->grad[2] = 0;
      v51 = this_4->width;
      this_4->grad[1] = v50[3].match[0] + v50[2].match[0] + v50[1].match[0] + v50[0].match[0] + v50[4].match[0] - 5;
      v52 = v86;
      if ( v51 <= 0 )
        break;
      v53 = 0;
      do
      {
        v54 = __decode_pixel((ModelBlock *)(uint32_t *)this_4, v53);
        __init_model_tables(this_4);
        v51 = this_4->width;
        v53 += v54;
      }
      while ( v53 < this_4->width );
      v52 = v86;
      if ( n4_1 != 4 )
        goto LABEL_53;
      if ( v51 > 0 )
      {
        ArgList_6 = (uint32_t *)ArgList_5;
        v65 = 0;
        do
          *ArgList_6++ = this_4->sym_code[this_4->row_cur[0][v65++ + 8].sym];
        while ( v65 < this_4->width );
        goto LABEL_73;
      }
LABEL_74:
      v43 = v52 + 1;
      if ( v43 >= *(uint32_t *)&this_4->height )
      {
        ArgList_3 = (uint8_t *)ArgList_4;
        goto LABEL_76;
      }
    }
    if ( n4_1 == 4 )
      goto LABEL_74;
LABEL_53:
    if ( n4_1 == 3 )
    {
      if ( v51 > 0 )
      {
        // Three bytes out of each 24-bit code: the low half and byte 2, which
        // is a `SymEntry` -- the same three-byte pair the symbol lists hold.
        ArgList_7 = (SymEntry *)ArgList_5;
        v56 = 0;
        do
        {
          v57 = this_4->sym_code;
          v58 = this_4->row_cur[0][v56 + 8].sym;
          ArgList_7->sym = (uint16_t)v57[v58];
          ArgList_7->cnt = (uint8_t)(v57[v58] >> 16);
          ++v56;
          ++ArgList_7;
        }
        while ( v56 < this_4->width );
        ArgList_5 = (uint8_t *)ArgList_7;
      }
      goto LABEL_74;
    }
    if ( n4_1 != 2 )
    {
      if ( this_4->depth == 8 )
      {
        if ( v51 > 0 )
        {
          ArgList_8 = ArgList_5;
          v80 = 0;
          do
            // A byte, not a word: this branch is the 8-bits-per-pixel one, and
            // `sym_code` was a `uint8_t *` when this dereference was written.
            *ArgList_8++ = (uint8_t)this_4->sym_code[this_4->row_cur[0][v80++ + 8].sym];
          while ( v80 < this_4->width );
          ArgList_5 = ArgList_8;
        }
      }
      else if ( v51 > 0 )
      {
        v87 = v52;
        v61 = 0;
        v62 = 0;
        ArgList_9 = ArgList_5;
        do
        {
          depth = this_4->depth;
          v62 -= depth;
          if ( v62 < 0 )
          {
            v62 = 8 - depth;
            *++ArgList_9 = this_4->sym_code[this_4->row_cur[0][v61 + 8].sym] << ((8 - depth) & 31);
          }
          else
          {
            *ArgList_9 |= this_4->sym_code[this_4->row_cur[0][v61 + 8].sym] << (v62 & 31);
          }
          ++v61;
        }
        while ( v61 < this_4->width );
        v52 = v87;
        ArgList_5 = ArgList_9;
      }
      goto LABEL_74;
    }
    if ( v51 > 0 )
    {
      ArgList_6 = (uint32_t *)ArgList_5;
      v60 = 0;
      do
      {
        *(uint16_t *)ArgList_6 = this_4->sym_code[this_4->row_cur[0][v60++ + 8].sym];
        ArgList_6 = (uint32_t *)((uint8_t *)ArgList_6 + 2);
      }
      while ( v60 < this_4->width );
LABEL_73:
      ArgList_5 = (uint8_t *)ArgList_6;
      goto LABEL_74;
    }
    goto LABEL_74;
  }
LABEL_76:
  __rc_end_decode();
  depth_raw = this_4->depth_raw;
  if ( depth_raw != this_4->depth )
  {
    n6 = (depth_raw + 7) >> 3;
    if ( n6 <= 0 )
    {
      v68 = *(uint32_t *)&this_4->height * this_4->width;
    }
    else
    {
      n6_3 = 0;
      v68 = *(uint32_t *)&this_4->height * this_4->width;
      v69 = v68 / n6;
      if ( n6 >= 6 )
      {
        v84 = *(uint32_t *)&this_4->height * this_4->width;
        n6_4 = 0;
        this_1 = (ModelBlock *)((uint32_t *)this_4);
        ArgList_10 = ArgList_3;
        do
        {
          __frame.row[n6_4] = ArgList_10;
          ArgList_10 += 5 * v69;
          __frame.row[n6_4 + 1] = &ArgList_3[v69 * (n6_4 + 1)];
          __frame.row[n6_4 + 2] = &ArgList_3[v69 * (n6_4 + 2)];
          __frame.row[n6_4 + 3] = &ArgList_3[v69 * (n6_4 + 3)];
          __frame.row[n6_4 + 4] = &ArgList_3[v69 * (n6_4 + 4)];
          n6_4 += 5;
        }
        while ( n6_4 <= n6 - 6 );
        this_4 = (ModelBlock *)((int32_t)this_1);
        n6_3 = n6_4;
        v68 = v84;
      }
      n6_1 = n6_3;
      v73 = v69 * n6_3;
      this_1 = (ModelBlock *)((uint32_t *)this_4);
      do
      {
        __frame.row[n6_1] = &ArgList_3[v73];
        v73 += v69;
        ++n6_1;
      }
      while ( n6_1 < n6 );
      this_4 = (ModelBlock *)((int32_t)this_1);
    }
    if ( v68 > 0 )
    {
      Src_2 = Src_1;
      ArgList_4 = (int32_t)ArgList_3;
      n6_2 = 0;
      v76 = 0;
      do
      {
        v77 = __frame.row[n6_2];
        *Src_2 = *v77;
        v78 = *(uint32_t *)&this_4->height * this_4->width;
        ++Src_2;
        __frame.row[n6_2++] = v77 + 1;
        if ( n6_2 == n6 )
          n6_2 = 0;
        ++v76;
      }
      while ( v76 < v78 );
      ArgList_3 = (uint8_t *)ArgList_4;
    }
    free(ArgList_3);
  }
}

int32_t __alt_model_p1_encode(uint16_t *p_i, uint8_t *a2)
{
  P1Ctx *v24, *buf3, *buf2, *buf1, *v28, *cursor0;
  struct alignas(16) AltModelP1EncodeFrame {   // 144 bytes, one stack frame
      uint8_t   _gap0[1];   // was int8_t v90
      uint8_t _pad0[3];
      uint32_t v91;
      int32_t v93;
      uint8_t   _gap1[4];   // was uint8_t * v95
      uint8_t   _gap2[4];   // was int32_t v96
      uint8_t   _gap3[1];   // was int8_t v97
      uint8_t _pad1[3];
      uint8_t   _gap4[4];   // was int32_t v98
      uint8_t   _gap5[4];   // was int32_t v99
      uint8_t   _gap6[4];   // was int32_t n5_8
      uint8_t   _gap7[16];   // was void * Block_plane
      uint8_t   _gap8[4];   // was int32_t v105
      uint8_t   _gap9[4];   // was int32_t v106
      uint8_t   _gap10[4];   // was uint32_t i_1
      uint8_t   _gap11[4];   // was int32_t v108
      uint8_t   _gap12[4];   // was int32_t v109
      uint8_t   _gap13[4];   // was uint32_t i_4
      uint8_t   _gap14[4];   // was int32_t v111
      uint8_t   _gap15[4];   // was int32_t v112
      uint8_t   _gap16[4];   // was int32_t v113
      uint8_t   _gap17[4];   // was int32_t v114
      uint8_t   _gap18[4];   // was int32_t v115
      uint8_t   _gap19[4];   // was int32_t n5_6
      uint8_t   _gap20[4];   // was int32_t v117
      uint8_t   _gap21[4];   // was int32_t v118
      uint8_t   _gap22[4];   // was int32_t n5
      uint8_t _pad2[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 144, "frame layout moved");
  int8_t v90;
  // These shared `__frame.v91` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t v92;
  // These shared `__frame.v93` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v94;
  uint8_t *v95;
  uint8_t v97;
  int32_t v98;
  int32_t v99;
  void * Block_plane[4];
  AltP1Block * &v102 = (AltP1Block * &)Block_plane[1];
  AltP1Block * &v103 = (AltP1Block * &)Block_plane[2];
  AltP1Block * &v104 = (AltP1Block * &)Block_plane[3];
  int32_t v105;
  uint32_t i_4;
  int32_t v111;
  int32_t v112;
  int32_t v113;
  int32_t v114;
  int32_t v115;
  int32_t n5_6;
  int32_t v117;
  int32_t v118;
  int32_t n5;
  ;
  uintptr_t n3;   // were int32_t: addresses, masked and tagged
  AltP1Block *v23;
  uint8_t v11, v12, v13, v62, v71, v81;
  AltP1Block *v6;
  int32_t i, v3, n4, *v7, v8, v9, v10, v14, v15, v16, v17, i_3, n4_1, n4_2,
          v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43,
          v44, v45, v47, v48, v49, n5_9, n5_7, n5_2, n5_1, v56, n16, v58,
          v63, v64, v65, n16_1, n5_3, v69, n5_4, v73, v74, n16_2, v77, n5_5,
          v84, n16_3, n4_3, n4_4;
  int64_t v68, v76, v86;
  uint32_t v19;
  AltP1Block *v61;
  AltP1Block *v70;
  AltP1Block *v80;
  uint8_t *v29;
  uint8_t *v30;
  uint8_t v53, v60, v79;
  AltP1Block *v50;
  void **v89;
  i = *p_i;
  v3 = p_i[1];
  if ( plane_count > 0 )
  {
    n4 = 0;
    do
    {
      v6 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __alt_p1_alloc((AltP1Block *)v6, i, v3, n4);
      else
        v7 = nullptr;
      Block_plane[n4++] = v7;
    }
    while ( n4 < plane_count );
  }
  // The decoder's block, mirrored.  `v10` is read for its record before its
  // low byte is overwritten with that record's b3, and `v90` -- a char -- is
  // the only thing that reads it after, so the byte is all of it.
  v8 = plane_desc[2].src_plane;
  v9 = plane_desc[3].src_plane;
  v10 = plane_desc[4].src_plane;
  v11 = plane_desc[v8 + 1].flags;
  v12 = plane_desc[v9 + 1].flags;
  v13 = plane_desc[v10 + 1].flags;
  v14 = plane_desc[v9 + 1].b3;
  LOBYTE(v10) = plane_desc[v10 + 1].b3;
  v105 = plane_desc[v8 + 1].b3;
  v15 = v11 & 8;
  v90 = v10;
  v16 = v12 & 8;
  v17 = v13 & 8;
  __rc_begin_encode();
  if ( v3 > 0 )
  {
    v19 = 0;
    i_3 = i;
    n4_1 = plane_count;
    do
    {
      if ( n4_1 > 0 )
      {
        __frame.v91 = v19;
        n4_2 = 0;
        __frame.v93 = v3;
        v95 = a2;
        do
        {
          ++n4_2;
          v23 = (AltP1Block *)Block_plane[n4_2 - 1];
          // The row end mirrored into the right margin: records 0..5 take -1..-6.
          // Two bytes each, which is what the twelve even offsets were.
          {
            P1Ctx *const here = (P1Ctx *)v23->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          v24 = v23->buf[4];
          buf3 = v23->buf[3];
          buf2 = v23->buf[2];
          buf1 = v23->buf[1];
          v28 = v23->buf[0];
          v23->buf[4] = buf3;
          v23->buf[3] = buf2;
          v23->buf[2] = buf1;
          v23->buf[1] = v28;
          v23->buf[0] = v24;
          v24 += 4;
          v23->cursor[0] = v24;
          v28 += 4;
          v23->cursor[1] = v28;
          v23->cursor[2] = buf1 + 4;
          v23->cursor[3] = buf2 + 4;
          v23->cursor[4] = buf3 + 4;
          ((P1Ctx *)v24)[-4] = ((P1Ctx *)v28)[3];
          // And the rest of the new row's left margin, from the row above.
          {
            P1Ctx *const here = (P1Ctx *)v23->cursor[0];
            P1Ctx *const up   = (P1Ctx *)v23->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          v29 = (uint8_t *)(*&v23->cursor[2]);
          v30 = (uint8_t *)(*&v23->cursor[4]);
          v23->ctx[2] = 0;
          v23->ctx[3] = 0;
          v23->ctx[4] = 0;
          v31 = *((int8_t *)v29 - 3);
          v23->ctx[3] = v31;
          v32 = *((int8_t *)v29 - 1);
          v23->ctx[4] = v32;
          v33 = *((int8_t *)v30 - 3) + v31;
          v23->ctx[3] = v33;
          v34 = *((int8_t *)v30 - 1) + v32;
          v23->ctx[4] = v34;
          v35 = v29[1] + v33;
          v23->ctx[3] = v35;
          v36 = v29[3] + v34;
          v23->ctx[4] = v36;
          v37 = v30[1] + v35;
          v23->ctx[3] = v37;
          v38 = v30[3] + v36;
          v23->ctx[4] = v38;
          v39 = v29[5] + v37;
          v23->ctx[3] = v39;
          v40 = v29[7] + v38;
          v23->ctx[4] = v40;
          v41 = v30[5] + v39;
          v23->ctx[3] = v41;
          v42 = v30[7] + v40;
          v23->ctx[4] = v42;
          v43 = v29[9] + v41;
          v23->ctx[3] = v43;
          v44 = v29[11] + v42;
          v23->ctx[4] = v44;
          v45 = v30[9] + v43;
          cursor0 = v23->cursor[0];
          v23->ctx[3] = v45;
          v47 = v30[11] + v44;
          v23->ctx[4] = v47;
          v48 = cursor0[-4].mag + v45;
          v23->ctx[3] = v48;
          v49 = cursor0[-3].mag + v47;
          v23->ctx[4] = v49;
          v23->ctx[3] = cursor0[-2].mag + v48;
          v23->ctx[4] = cursor0[-1].mag + v49;
          n4_1 = plane_count;
        }
        while ( n4_2 < plane_count );
        v19 = __frame.v91;
        v3 = __frame.v93;
        i_3 = i;
        a2 = v95;
      }
      if ( i_3 > 0 )
      {
        v92 = v19;
        i_4 = 0;
        v94 = v3;
        do
        {
          v50 = (AltP1Block *)Block_plane[0];
          n5_9 = *(a2 + plane_desc[1].src_plane);
          v114 = plane_desc[1].src_plane;
          n5_6 = n5_9;
          __alt_p1_context((AltP1Block *)(uint8_t **)Block_plane[0], (AltP1Block *)nullptr, (AltP1Block *)0);
          n5_7 = n5_6;
          v53 = (uint8_t)v50->pred;
          v112 = (uint8_t)(n5_6 - v53);
          n5_2 = v50->fold[v112];
          n5_1 = (uint8_t)(v50->unfold[n5_2] + v53);
          v56 = (uint8_t)(n5_1 + *(a2 + v114) - n5_6);
          n16 = *(a2 + v114) - v56;
          if ( n16 < -16 || n16 > 16 )
          {
            n5_2 = v50->fold_hi[v112];
          }
          else
          {
            n5_6 = n5_1;
            *(a2 + v114) = v56;
            n5_7 = n5_1;
          }
          __alt_p1_encode_symbol(&v50->counters[v50->ctx[0]].total, n5_1, v50->ctx[1], n5_2);
          v58 = n5_7 - v50->pred;
          v50->cursor[0]->sym = n5_6;
          v50->cursor[0]->mag = abs32(v58);
          v50->ctx[v50->ctx[2] + 3] = v50->ctx[v50->ctx[2] + 3]
                                                         + v50->cursor[0]->mag
                                                         - v50->cursor[0][-4].mag
                                                         - (v50->cursor[4][-2].mag
                                                          - v50->cursor[4][6].mag
                                                          + v50->cursor[2][-2].mag
                                                          - v50->cursor[2][6].mag);
          v50->ctx[2] = v50->ctx[2] == 0;
          if ( v50->counters[v50->ctx[0]].total < 0x4000u )
            __alt_p1_model(v50);
          ++v50->cursor[0];
          ++v50->cursor[1];
          ++v50->cursor[2];
          ++v50->cursor[3];
          ++v50->cursor[4];
          v60 = *(a2 + plane_desc[2].src_plane);
          v113 = plane_desc[2].src_plane;
          if ( v15 )
            v60 = v60 - v105 - *(plane_desc[1].src_plane + a2);
          v61 = (AltP1Block *)(v102);
          v118 = v60;
          __alt_p1_context((AltP1Block *)v102, (AltP1Block *)Block_plane[0], (AltP1Block *)0);
          v62 = (uint8_t)v61->pred;
          v63 = (uint8_t)(v118 - v62);
          n5 = v61->fold[v63];
          v64 = *(a2 + v113);
          v65 = (uint8_t)(v61->unfold[n5] + v62);
          v111 = (uint8_t)(v65 + *(a2 + v113) - v118);
          n16_1 = v64 - v111;
          n5_3 = n5;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            n5_3 = v61->fold_hi[v63];
          }
          else
          {
            *(a2 + v113) = v111;
            v118 = v65;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v61)[4 * v61->ctx[0] + 950], 16 * v61->ctx[0], (int32_t)v61->ctx[1], n5_3);
          v68 = v118 - v61->pred;
          v61->cursor[0]->sym = v118;
          v61->cursor[0]->mag = (BYTE4(v68) ^ v68) - BYTE4(v68);
          ((uint8_t**)v61)[v61->ctx[2] + 6] = &((uint8_t**)v61)[v61->ctx[2] + 6][v61->cursor[0]->mag
                                                           - v61->cursor[0][-4].mag
                                                           - (v61->cursor[4][-2].mag
                                                            - v61->cursor[4][6].mag)
                                                           - (v61->cursor[2][-2].mag
                                                            - v61->cursor[2][6].mag)];
          v61->ctx[2] = v61->ctx[2] == 0;
          if ( v61->counters[v61->ctx[0]].total < 0x4000u )
            __alt_p1_model((AltP1Block *)v61);
          ++v61->cursor[0];
          ++v61->cursor[1];
          ++v61->cursor[2];
          ++v61->cursor[3];
          ++v61->cursor[4];
          v69 = *(plane_desc[3].src_plane + a2);
          v117 = plane_desc[3].src_plane;
          if ( v16 )
            v69 = (uint8_t)(v69
                                  - v14
                                  - ((plane_desc[plane_desc[3].src_plane + 1].w4
                                    * *(plane_desc[1].src_plane + a2)
                                    + plane_desc[plane_desc[3].src_plane + 1].w8
                                    * (uint32_t)*(plane_desc[2].src_plane + a2)
                                    + 40) >> 7));
          v70 = (AltP1Block *)(v103);
          __alt_p1_context((AltP1Block *)v103, (AltP1Block *)v102, (AltP1Block *)(int32_t)Block_plane[0]);
          v71 = (uint8_t)v70->pred;
          v115 = (uint8_t)(v69 - v71);
          n5_4 = v70->fold[v115];
          v73 = (uint8_t)(v70->unfold[n5_4] + v71);
          v74 = (uint8_t)(v73 + *(v117 + a2) - v69);
          n16_2 = *(v117 + a2) - v74;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            n5_4 = v70->fold_hi[v115];
          }
          else
          {
            v69 = v73;
            *(v117 + a2) = v74;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v70)[4 * v70->ctx[0] + 950], n5_4, (int32_t)v70->ctx[1], n5_4);
          v76 = v69 - v70->pred;
          v70->cursor[0]->sym = v69;
          v70->cursor[0]->mag = (BYTE4(v76) ^ v76) - BYTE4(v76);
          ((uint8_t**)v70)[v70->ctx[2] + 6] = &((uint8_t**)v70)[v70->ctx[2] + 6][v70->cursor[0]->mag
                                                           - v70->cursor[0][-4].mag
                                                           - (v70->cursor[4][-2].mag
                                                            - v70->cursor[4][6].mag)
                                                           - (v70->cursor[2][-2].mag
                                                            - v70->cursor[2][6].mag)];
          v77 = 4 * v70->ctx[0];
          v70->ctx[2] = v70->ctx[2] == 0;
          if ( LOWORD(((uint8_t**)v70)[v77 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v70);
          ++v70->cursor[0];
          ++v70->cursor[1];
          ++v70->cursor[2];
          ++v70->cursor[3];
          ++v70->cursor[4];
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            n3 = plane_desc[4].src_plane;
            if ( v17 )
              v79 = *(plane_desc[4].src_plane + a2)
                  - v90
                  - ((plane_desc[plane_desc[4].src_plane + 1].w8 * *(plane_desc[4].src_plane + a2 - 2)
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *(plane_desc[4].src_plane + a2 - 3)
                    + plane_desc[plane_desc[4].src_plane + 1].w12 * *(plane_desc[4].src_plane + a2 - 1)
                    + 64) >> 7);
            else
              v79 = *(plane_desc[4].src_plane + a2);
            v80 = (AltP1Block *)(v104);
            v99 = v79;
            __alt_p1_context((AltP1Block *)v104, (AltP1Block *)v103, (AltP1Block *)(int32_t)v102);
            v81 = (uint8_t)v80->pred;
            n5_5 = v80->fold[(uint8_t)(v99 - v81)];
            v98 = (uint8_t)(v99 - v81);
            v84 = (uint8_t)(v80->unfold[n5_5] + v81);
            n16_3 = *(n3 + a2) - (uint8_t)(v84 + *(n3 + a2) - v99);
            v97 = v84 + *(n3 + a2) - v99;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              n5_5 = v80->fold_hi[v98];
            }
            else
            {
              v99 = v84;
              *(n3 + a2) = v97;
            }
            __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v80)[4 * v80->ctx[0] + 950], n5_5, (int32_t)v80->ctx[1], n5_5);
            v86 = v99 - v80->pred;
            v80->cursor[0]->sym = v99;
            v80->cursor[0]->mag = (BYTE4(v86) ^ v86) - BYTE4(v86);
            ((uint8_t**)v80)[v80->ctx[2] + 6] = &((uint8_t**)v80)[v80->ctx[2] + 6][v80->cursor[0]->mag
                                                             - v80->cursor[0][-4].mag
                                                             - (v80->cursor[4][-2].mag
                                                              - v80->cursor[4][6].mag)
                                                             - (v80->cursor[2][-2].mag
                                                              - v80->cursor[2][6].mag)];
            v80->ctx[2] = v80->ctx[2] == 0;
            if ( v80->counters[v80->ctx[0]].total < 0x4000u )
              __alt_p1_model((AltP1Block *)v80);
            ++v80->cursor[0];
            ++v80->cursor[1];
            ++v80->cursor[2];
            ++v80->cursor[3];
            ++v80->cursor[4];
            n4_1 = plane_count;
          }
          a2 += n4_1;
          ++i_4;
        }
        while ( i_4 < i );
        v19 = v92;
        v3 = v94;
        i_3 = i;
      }
      ++v19;
    }
    while ( v19 < v3 );
  }
  __rc_end_encode();
  n4_3 = plane_count;
  if ( plane_count > 0 )
  {
    n4_4 = 0;
    do
    {
      v89 = (void **)Block_plane[n4_4];
      if ( v89 )
      {
        __alt_p1_free((void **)v89, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}

uint32_t __alt_p2_model(AltP2Block *a1, int32_t a3, uint8_t a4, int32_t a5)
{
  P2Ctx *v7, *cursor0;
  float    n2_bias;   // the p2 filter's bias term, one of three lifetimes MSVC
  int32_t  n2_half;   // gave one slot; the third is the cursor below
  // Lanes of the counter table.  Every read through this is a `uint16_t`
  // except the two shift counts at +-4, which are the low byte of the
  // neighbouring record; the byte offsets it carried were all even.
  uint16_t *n2;
  P2Freq *n0xF0;
  // Two more things MSVC kept in the register `n0xF0` names.
  int32_t hi_nibble, rec_idx;
  uint32_t n0x10_2;   // a record index in four regions ...
  P2Freq *p2_rec;
  // Two things in one slot, and both are read as numbers: the strip index
  // out of `ctx_pair[0]` or `ctx_pair[1]`, and later an address.  It is neither a
  // `uint16_t *` nor an index -- it is the register MSVC put both in.
  uintptr_t v508;
  uint32_t n0x10_1;
  int32_t v510;
  int32_t v511;
  P2Count *v512;
  P2Count *v513;
  P2Count *v514;
  P2Count *v515;
  P2Count *v516;
  P2Count *v517;
  P2Count *v518;
  P2Count *v519;
  P2Count *v520;
  P2Count *v521;
  P2Count *v522;
  P2Count *v523;
  P2Count *v524;
  P2Count *v525;
  P2Count *v526;
  P2Count *v527;
  P2Count *v528;
  P2Count *v529;
  P2Count *v530;
  P2Count *v531;
  P2Count *v532;
  P2Count *v533;
  P2Count *v534;
  P2Count *v535;
  P2Count *v536;
  P2Count *v537;
  P2Count *v538;
  P2Count *v539;
  P2Count *v540;
  P2Count *v541;
  P2Count *v542;
  P2Count *v543;
  P2Count *v544;
  uint32_t v545;
  int32_t n3;
  int32_t v547;
  int32_t v548;
  int32_t v549;
  int32_t v550;
  P2Count *v551;
  P2Count *v552;
  P2Count *v553;
  P2Count *v554;
  P2Count *v555;
  P2Count *v556;
  P2Count *v557;
  P2Count *v558;
  P2Count *v559;
  P2Count *v560;
  P2Count *v561;
  P2Count *v562;
  P2Count *v563;
  P2Count *v564;
  P2Count *v565;
  P2Count *v566;
  P2Count *v567;
  P2Count *v568;
  int32_t v571;
  int32_t v573;
  int32_t v576;
  int32_t v577;
  AltP2Block *v578;
  uint32_t n5;
  int32_t v580;
  // The counter this pass updates and its two neighbours: `v581[-1]`,
  // `v581[0]` and `v581[1]` are +284 708, +284 712 and +284 716 off the
  // row base, which is `p2_ctr` reached through `v78`.
  P2Count *v581;
  ;
  uint16_t *n2_1;   // a second name for `n2`
  // The parameter this used to copy is never read: `sample` is written from
  // `v577` below before any of its four readers, and lanes 1..3 were never
  // touched at all.  It is XMM0 being reused as a scratch register, which is
  // what MSVC did and what Hex-Rays recorded.  `v19` is the same, one lane.
  float sample;
  float (*f278656)[4];
  float (*v17)[4];
  float v19;
  bool v87, v103, v104, v105;
  uint8_t v114;
  uint8_t *v90;   // `uint8_t *` beside the `char` scalars above
  P2Count *v110;
  P2Count *v94;
  P2Count *v107;
  P2Count *v112;
  P2Count *v91;
  P2Count *v93;
  float v16, bias2, bias1, v21, v22, v23, v24, v26;
  P2Count *v98;
  int32_t v6, v9, bank_ctx, v77, v79, v80, v81, v84, v85, v86, v88, v89, v95, v96, v100, v101, v102,
          v106, v108, v113, v115, v117, v118, v119, v120, v121, v122, v123, v124, v125, v126,
          v127, v128, v129, v130, v131, v132, v133, v134, v135, v136, v137, v138, v139, v140,
          v141, v142, v143, v144, v145, v146, v147, v148, v149, v150, v151, v152, v153, v154,
          v155, v156, v157, v158, v159, v160, v161, v162, v163, v164, v165, v166, v167, v168,
          v169, v170, v171, v172, v173, v174, v175, v176, v177, v178, v179, v180, v181, v182,
          v183, v184, v185, v186, v187, v188, v189, v190, v191, v192, v193, v194, v195, v196,
          v197, v198, v199, v200, v201, v202, v203, v204, v205, v206, v207, v208, v209, v210,
          v211, v212, v213, v214, v215, v216, v217, v218, v219, v220, v221, v222, v223, v224,
          v225, v226, v227, v228, v229, v230, v231, v232, v233, v234, v235, v236, v237, v238,
          v239, v240, v241, v242, v243, v244, v245, v246, v247, v248, v249, v250, v251, v252,
          v253, v254, v255, v256, v257, v258, v259, v260, v261, v262, v263, v264, v265, v266,
          v267, v268, v269, v270, v271, v272, v273, v274, v275, v276, v277, v278, v279, v280,
          v281, v282, v283, v284, v285, v286, v287, v288, v289, v290, v291, v292, v293, v294,
          v295, v296, v297, v298, v299, v300, v301, v302, v303, v304, v305, v306, v307, v308,
          v309, v310, v311, v313, v314, v315, v316, v317, v318, v319, v320, v321, v322, v323,
          v324, v325, v326, v327, v328, v329, v330, v331, v332, v333, v334, v335, v336, v337,
          v338, v339, v340, v341, v342, v343, v344, v345, v346, v347, v348, v349, v350, v351,
          v352, v353, v354, v355, v356, v357, v358, v359, v360, v361, v362, v363, v364, v365,
          v366, v367, v368, v369, v370, v371, v372, v373, v374, v375, v376, v377, v378, v379,
          v380, v381, v382, v383, v384, ctx, n15, v391, v392, v394, v395, v397, v398, v399,
          v400, v403, v404, v406, v407, v409, v410, v412, v413, v415, v416, v417, v418, v419,
          v421, v422, v423, v424, v425, v426, v427, v429, v430, v431, v432, v433, v434, v435,
          v437, v438, v439, v440, v441, v442, v443, v447, v448, v449, v450, v453, v454, v459,
          v460, v461, v464, v465, v467, v468, v470, v471, v472, v475, v476, v478, v479, v481,
          v482, v483, v486, v487, v489, v490, v492, v493, v494, v497, v498, v500, v501, v503,
          v504;
  int64_t v10, v11, v12, v13, v14;
  P2Freq *n0xF0_3;
  P2Freq *n0xF0_2;
  P2Freq *n0xF0_1;
  P2Freq *n0xF0_4;
  P2Freq *v445;
  P2Freq *v387;
  // Every one of these is a record: `alt_p2_model` walks the table by the
  // context index and its two neighbours.
  P2Freq *n0x10_3, *n0x10_4;
  // Records: the one the composed index selects, and its two neighbours.
  P2Freq *n2_2, *n2_3, *n2_4, *n2_5, *n2_6, *n2_7, *v408, *v411, *v420,
         *v428, *v436, *v444, *v452, *v456, *v457, *v463, *v466, *v474,
         *v477, *v485, *v488, *v491, *v502;
  AltP2Block *v385;
  uint32_t v78, v92, v97, v109, n0x10, v393, v396;
  uint8_t v83;
  v6 = a1->ctx & 0xF;
  v577 = 16 * a3;
  a1->cursor[0]->lane[0] = 16 * a3;
  a1->cursor[0]->lane[1] = a1->cursor[0]->lane[0] - a1->cursor[0]->lane[1];
  a1->cursor[0][1].lane[1] = 0;
  a1->cursor[0]->sign = (a5 <= (int32_t)(((uint32_t)(6 - v6) >> 31)
                                                         + ((uint32_t)(4 - v6) >> 31)
                                                         + 2 * ((uint32_t)(9 - v6) >> 31)))
                                            + (a5 < (int32_t)-(((uint32_t)(6 - v6) >> 31)
                                                                + ((uint32_t)(4 - v6) >> 31)
                                                                + 2 * ((uint32_t)(9 - v6) >> 31)));
  a1->cursor[0]->mag = abs32(a5);
  cursor0 = a1->cursor[0];
  v9 = v577;
  sample = (float)v577;
  v10 = v577 - cursor0[-1].lane[0];
  cursor0->lane[4] = (WORD2(v10) ^ v10) - WORD2(v10);
  v11 = v9 - a1->cursor[1]->lane[0];
  a1->cursor[0]->lane[5] = (WORD2(v11) ^ v11) - WORD2(v11);
  v12 = v9 - a1->cursor[1][-1].lane[0];
  a1->cursor[0]->lane[6] = (WORD2(v12) ^ v12) - WORD2(v12);
  v13 = v9 - a1->cursor[1][1].lane[0];
  a1->cursor[0]->lane[7] = (WORD2(v13) ^ v13) - WORD2(v13);
  v14 = (int16_t)(v9 - a1->pred_prev);
  a1->cursor[0]->lane[2] = v14;
  a1->cursor[0]->lane[3] = (WORD2(v14) ^ v14) - WORD2(v14);
  f278656 = a1->f278656;
  v16 = f278656[14][1] + 0.000099999997f;
  v17 = *(float (**)[4])(a1->cur - 1);
  bias2 = a1->bias[2];
  n2_bias = a1->bias[0];
  v19 = sample - bias2;
  bias1 = a1->bias[1];
  v21 = bias1 - bias2;
  v22 = ((((sample - bias2) * (bias1 - bias2)) - f278656[14][0]) * 0.001f)
      + f278656[14][0];
  v23 = v16 + (((v21 * v21) - f278656[14][1]) * 0.001f);
  f278656[14][1] = v23;
  v24 = 0.1f * v23;
  if ( (0.1f * v23) <= v22 )
    v24 = fminf(v23, v22);
  f278656[14][0] = v24;
  // Two normalised-LMS updates side by side, on two weight blocks: `f278656` at a
  // fixed mean-square rate and `v17` at one scaled by the confidence `v26`.
  // Same shape as `alt_p2_context`'s, run twice with different errors and
  // different floors.
  v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;
  n5 = 0;
  v577 = v9;
  v578 = (AltP2Block *)((uint32_t *)a1);
  {
    const float err_a     = (sample - bias1) * 2.5999999f;
    const float err_b     = v19 * v26;
    const float floor_a   = 26896.0f * f278656[14][2];
    const float floor_b   = 5041.0f * v17[14][2];
    const float ms_rate_b = 0.013f * v26;
    int32_t j, k;

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
      {
        float x = a1->p2_row[j][k];
        float ms;

        ms = f278656[7 + j][k]
           + (x * x - f278656[7 + j][k]) * 0.05f;      // 0x439B40
        f278656[7 + j][k] = ms;
        f278656[j][k] += bmf_p2_rate[j][k] * err_a * x / (ms + floor_a);

        ms = v17[7 + j][k]
           + (x * x - v17[7 + j][k]) * ms_rate_b;
        v17[7 + j][k] = ms;
        v17[j][k] += bmf_p2_rate[j][k] * err_b * x / (ms + floor_b);
      }
  }

  // The first block's prediction, and one more step of the same update against
  // the error in it.  Row 14 lane 2 scales the floor, and decays toward ten as
  // this context is seen more often.
  {
    float acc[4], pred, err, ms_scale;
    int32_t j, k;

    for ( k = 0; k < 4; ++k )
    {
      acc[k] = f278656[0][k] * a1->p2_row[0][k];
      for ( j = 1; j < 7; ++j )
        acc[k] += f278656[j][k] * a1->p2_row[j][k];
    }
    pred     = n2_bias + bmf_hsum4(acc);
    err      = sample - pred;
    ms_scale = f278656[14][2];

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
        f278656[j][k] += bmf_p2_rate[j][k] * err * a1->p2_row[j][k]
                            / (f278656[7 + j][k] + ms_scale * 529.0f);
    ++*(int32_t *)&f278656[15][0];
    f278656[14][2] = ms_scale + ((10.0f - ms_scale) * 0.00019999999f);
  }
  *a1->cur = *(uint32_t *)&a1->f278656;
  ++a1->cur;
  ++a1->above;
  do
  {
    bank_ctx = (uint32_t)v578->bank_ctx[n5];
    v77 = v577 - (*(uint32_t *)&v578->nb_sum[2 * n5]);
    v78 = n5 << 17;
    // 71178 records is +284712, `p2_ctr`, and `v78` is `n5 << 17` --
    // 32768 records a bank, which is what the five banks are.
    v581 = &v578->p2_ctr[32768 * n5 + bank_ctx];
    v79 = v77 + (uint16_t)v581->w2;
    *(uint16_t *)&v581->w2 = v79;
    v580 = v581->b1;
    if ( v580 )
    {
      v576 = bank_ctx;
      v80 = v79 + 4 * ((v77 > deadzone_hi) - (v77 < deadzone_lo));
      *(uint16_t *)&v581->w2 = v80;
      v81 = v576;
      if ( (int32_t)abs32(v77) < 38 )
      {
        if ( (uint8_t)--v580 )
        {
          v581->b1 = v580;
        }
        else
        {
          if ( *(uint8_t *)&v581->b0 < 8u )
          {
            v83 = *(uint8_t *)&v581->b0 + 1;
            *(uint8_t *)&v581->b0 = v83;
            v581->b1 = *((uint8_t *)&p2_float_pool + v83 + 3);   // two floats read sideways
            *(uint16_t *)&v581->w2 = 2 * v80;
          }
          else
          {
            v581->b1 = v580;
          }
        }
      }
      if ( !alphabet_reduced )
      {
        __builtin_prefetch(&n2, 0, 1);
        n2 = (uint16_t *)((uint8_t *)&v578->p2_ctr[(v81 ^ 0x7FF0)] + v78);
        v84 = (*(uint32_t *)&v578->nb_sum[2 * n5 + 1]) + v77;
        n3 = v81 & 3;
        if ( (uint32_t)n3 >= 3
          || (v545 = v78,
              v576 = v81,
              v85 = v581[1].w2,
              v86 = v84 - p2_pred(v85, *(uint8_t *)&v581[1].b0),
              v87 = n3 <= 0,
              *(uint16_t *)&v581[1].w2 = v85
                                                      + ((32
                                                        * ((v86 > deadzone_hi) - (uint32_t)(v86 < deadzone_lo))
                                                        + v86
                                                        + 1) >> 1),
              v81 = v576,
              !v87) )
        {
          v545 = v78;
          v576 = v81;
          v88 = v581[-1].w2;
          v89 = v84 - p2_pred(v88, *(uint8_t *)&v581[-1].b0);
          *(uint16_t *)&v581[-1].w2 = v88
                                                  + ((32 * ((v89 > deadzone_hi) - (uint32_t)(v89 < deadzone_lo))
                                                    + v89
                                                    + 2) >> 2);
          v81 = v576;
        }
        v90 = (uint8_t *)v578 + v78;
        v550 = v84;
        __builtin_prefetch(&v90[4 * (v81 ^ 0x4000) + 284712], 0, 1);
        v542 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x4000) + 284712]);
        v544 = (P2Count *)(&v90[4 * (v81 ^ 0x3FF0) + 284712]);
        __builtin_prefetch(v544, 0, 1);
        v543 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x4000) >> 2) & 3]
                  + 284712
                  + 4 * ((v81 ^ 0x4000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v543, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x2000) + 284712], 0, 1);
        v539 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x2000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x5FF0) + 284712], 0, 1);
        v541 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x5FF0) + 284712]);
        v540 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x2000) >> 2) & 3]
                  + 284712
                  + 4 * ((v81 ^ 0x2000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v540, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x1000) + 284712], 0, 1);
        v536 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x1000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x6FF0) + 284712], 0, 1);
        v538 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x6FF0) + 284712]);
        v537 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x1000) >> 2) & 3]
                  + 284712
                  + 4 * ((v81 ^ 0x1000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v537, 0, 1);
        v551 = (P2Count *)(&v90[4 * (v81 ^ 0x800) + 284712]);
        v533 = (P2Count *)(v551);
        v535 = (P2Count *)(&v90[4 * (v81 ^ 0x77F0) + 284712]);
        v534 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x800) >> 2) & 3]
                  + 284712
                  + 4 * ((v81 ^ 0x800) & 0xFFFFFFF3)]);
        v552 = (P2Count *)(&v90[4 * (v81 ^ 0x400) + 284712]);
        v530 = (P2Count *)(v552);
        __builtin_prefetch(v551, 0, 1);
        __builtin_prefetch(v535, 0, 1);
        __builtin_prefetch(v534, 0, 1);
        v553 = (P2Count *)(&v90[4 * (v81 ^ 0x7BF0) + 284712]);
        v532 = (P2Count *)(v553);
        v531 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x400) >> 2) & 3]
                  + 284712
                  + 4 * ((v81 ^ 0x400) & 0xFFFFFFF3)]);
        v554 = (P2Count *)(&v90[4 * (v81 ^ 0x200) + 284712]);
        v527 = (P2Count *)(v554);
        __builtin_prefetch(v552, 0, 1);
        v555 = (P2Count *)(&v90[4 * (v81 ^ 0x7DF0) + 284712]);
        v529 = (P2Count *)(v555);
        v91 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x200) >> 2) & 3]
                 + 284712
                 + 4 * ((v81 ^ 0x200) & 0xFFFFFFF3)]);
        __builtin_prefetch(v553, 0, 1);
        __builtin_prefetch(v531, 0, 1);
        v528 = (P2Count *)(v91);
        v556 = (P2Count *)(&v90[4 * (v81 ^ 0x100) + 284712]);
        v524 = (P2Count *)(v556);
        v92 = p2_ctx_rotate[((v81 ^ 0x100) >> 2) & 3] + ((v81 ^ 0x100) & 0xFFFFFFF3);
        v557 = (P2Count *)(&v90[4 * (v81 ^ 0x7EF0) + 284712]);
        v526 = (P2Count *)(v557);
        __builtin_prefetch(v554, 0, 1);
        v558 = (P2Count *)((int32_t)&v90[4 * v92 + 284712]);
        v525 = (P2Count *)(v558);
        v559 = (P2Count *)(&v90[4 * (v81 ^ 0x80) + 284712]);
        v521 = (P2Count *)(v559);
        __builtin_prefetch(v555, 0, 1);
        __builtin_prefetch(v91, 0, 1);
        v523 = (P2Count *)(&v90[4 * (v81 ^ 0x7F70) + 284712]);
        v560 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x80) >> 2) & 3] + 284712 + 4 * ((v81 ^ 0x80) & 0xFFFFFFF3)]);
        v522 = (P2Count *)(v560);
        v561 = (P2Count *)(&v90[4 * (v81 ^ 0x40) + 284712]);
        v518 = (P2Count *)(v561);
        __builtin_prefetch(v556, 0, 1);
        v562 = (P2Count *)(&v90[4 * (v81 ^ 0x7FB0) + 284712]);
        v520 = (P2Count *)(v562);
        v93 = (P2Count *)(&v90[4 * p2_ctx_rotate[((v81 ^ 0x40) >> 2) & 3] + 284712 + 4 * ((v81 ^ 0x40) & 0xFFFFFFF3)]);
        v94 = (P2Count *)(v558);
        __builtin_prefetch(v557, 0, 1);
        __builtin_prefetch(v94, 0, 1);
        v563 = (P2Count *)(v93);
        v519 = (P2Count *)(v93);
        v95 = v81 ^ 0x20;
        v96 = v81 ^ 0x10;
        v564 = (P2Count *)(&v90[4 * v95 + 284712]);
        v515 = (P2Count *)(v564);
        v565 = (P2Count *)(&v90[4 * (v95 ^ 0x7FF0) + 284712]);
        v517 = (P2Count *)(v565);
        __builtin_prefetch(v559, 0, 1);
        __builtin_prefetch(v523, 0, 1);
        v566 = (P2Count *)((uint32_t)&v90[4 * p2_ctx_rotate[(v95 >> 2) & 3] + 284712 + 4 * (v95 & 0xFFFFFFF3)]);
        v516 = (P2Count *)(v566);
        v567 = (P2Count *)((int32_t)&v90[4 * v96 + 284712]);
        v512 = (P2Count *)(v567);
        v97 = p2_ctx_rotate[(v96 >> 2) & 3] + (v96 & 0xFFFFFFF3);
        v568 = (P2Count *)((int32_t)&v90[4 * (v96 ^ 0x7FF0) + 284712]);
        v514 = (P2Count *)(v568);
        __builtin_prefetch(v560, 0, 1);
        v98 = (P2Count *)((int32_t)&v90[4 * v97 + 284712]);
        n2_1 = n2;
        v100 = -v550;
        v513 = (P2Count *)(v98);
        LOBYTE(v97) = (uint8_t)n2[0];
        __builtin_prefetch(v561, 0, 1);
        v101 = (int16_t)n2_1[1];
        __builtin_prefetch(v562, 0, 1);
        v102 = v100 - p2_pred(v101, v97);
        v105 = __OFSUB__(v102, deadzone_hi);
        v103 = v102 == deadzone_hi;
        v104 = v102 - deadzone_hi < 0;
        __builtin_prefetch(v563, 0, 1);
        v106 = 32 * (!(v104 ^ v105 | v103) - (v102 < deadzone_lo));
        __builtin_prefetch(v564, 0, 1);
        __builtin_prefetch(v565, 0, 1);
        v107 = (P2Count *)(v98);
        v108 = v550;
        v109 = v106 + v102 + 2;
        v110 = (P2Count *)(v567);
        LOWORD(v109) = v101 + (v109 >> 2);
        v87 = n3 < 3;
        n2[1] = v109;
        v112 = (P2Count *)(v568);
        __builtin_prefetch(((P2Count *)(v566)), 0, 1);
        __builtin_prefetch(v110, 0, 1);
        __builtin_prefetch(v112, 0, 1);
        __builtin_prefetch(v107, 0, 1);
        if ( v87
          && (v113 = (int16_t)n2[3],
              v114 = (uint8_t)n2[2],
              v550 = v108,
              v87 = n3 <= 0,
              n2[3] = ((uint32_t)(-v108 - p2_pred(v113, v114) + 2) >> 2) + v113,
              v87) )
        {
          v311 = v542->w2;
          v550 = v108;
          v313 = v108 - p2_pred(v311, (*(uint8_t *)&v542->b0));
          v542->w2 = p2_bump(v311, v313, 2);
          v314 = v543->w2;
          v315 = v108 - p2_pred(v314, v543->b0);
          v543->w2 = p2_bump(v314, v315, 3);
          v316 = v542[1].w2;
          v317 = p2_pred(v316, *(uint8_t *)&v542[1].b0);
          v549 = -v108;
          *(uint16_t *)&v542[1].w2 = ((uint32_t)(v108 - v317 + 2) >> 2) + v316;
          v318 = v544->w2;
          v319 = -v108 - p2_pred(v318, v544->b0);
          v544->w2 = p2_bump(v318, v319, 3);
          v320 = v539->w2;
          v321 = v550;
          v322 = v550 - p2_pred(v320, *(uint8_t *)&v539->b0);
          v539->w2 = p2_bump(v320, v322, 2);
          v323 = v540->w2;
          v324 = v321 - p2_pred(v323, v540->b0);
          v540->w2 = p2_bump(v323, v324, 3);
          *(uint16_t *)&v539[1].w2 += (uint32_t)(v321
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&v539[1].w2, *(uint8_t *)&v539[1].b0)
                                               + 2) >> 2;
          v325 = v541->w2;
          v326 = v549 - p2_pred(v325, *(uint8_t *)&v541->b0);
          v541->w2 = p2_bump(v325, v326, 3);
          v327 = v536->w2;
          v328 = v550 - p2_pred(v327, *(uint8_t *)&v536->b0);
          v536->w2 = p2_bump(v327, v328, 2);
          v329 = v537->w2;
          v330 = v550 - p2_pred(v329, v537->b0);
          v537->w2 = p2_bump(v329, v330, 3);
          *(uint16_t *)&v536[1].w2 += (uint32_t)(v550
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&v536[1].w2, *(uint8_t *)&v536[1].b0)
                                               + 2) >> 2;
          v331 = v538->w2;
          v332 = v549 - p2_pred(v331, *(uint8_t *)&v538->b0);
          v538->w2 = p2_bump(v331, v332, 3);
          v333 = v533->w2;
          v334 = v550 - p2_pred(v333, v533->b0);
          v533->w2 = p2_bump(v333, v334, 2);
          v335 = v534->w2;
          v336 = v550 - p2_pred(v335, v534->b0);
          v534->w2 = p2_bump(v335, v336, 3);
          *(uint16_t *)&v533[1].w2 += (uint32_t)(v550 - p2_pred(v533[1].w2, v533[1].b0) + 2) >> 2;
          v337 = v535->w2;
          v338 = v549 - p2_pred(v337, v535->b0);
          v535->w2 = p2_bump(v337, v338, 3);
          v339 = v530->w2;
          v340 = v550 - p2_pred(v339, v530->b0);
          v530->w2 = p2_bump(v339, v340, 2);
          v341 = v531->w2;
          v342 = v550 - p2_pred(v341, v531->b0);
          v531->w2 = p2_bump(v341, v342, 3);
          v343 = v550;
          *(uint16_t *)&v530[1].w2 += (uint32_t)(v550 - p2_pred(v530[1].w2, v530[1].b0) + 2) >> 2;
          v344 = v532->w2;
          v549 -= p2_pred(v344, v532->b0);
          v550 = v343;
          v532->w2 = p2_bump(v344, v549, 3);
          v345 = v527->w2;
          v346 = v550 - p2_pred(v345, v527->b0);
          v527->w2 = p2_bump(v345, v346, 2);
          v347 = v528->w2;
          v348 = v550 - p2_pred(v347, v528->b0);
          v528->w2 = p2_bump(v347, v348, 3);
          v349 = v550;
          *(uint16_t *)&v527[1].w2 += (uint32_t)(v550 - p2_pred(v527[1].w2, v527[1].b0) + 2) >> 2;
          v350 = v529->w2;
          v548 = -v349;
          v351 = -v349 - p2_pred(v350, v529->b0);
          v529->w2 = p2_bump(v350, v351, 3);
          v352 = v524->w2;
          v353 = v550;
          v354 = v550 - p2_pred(v352, v524->b0);
          v524->w2 = p2_bump(v352, v354, 2);
          v355 = v525->w2;
          v356 = v353 - p2_pred(v355, *(uint8_t *)&v525->b0);
          v525->w2 = p2_bump(v355, v356, 3);
          *(uint16_t *)&v524[1].w2 += (uint32_t)(v353 - p2_pred(v524[1].w2, v524[1].b0) + 2) >> 2;
          v357 = v526->w2;
          v358 = v548 - p2_pred(v357, v526->b0);
          v526->w2 = p2_bump(v357, v358, 3);
          v359 = v521->w2;
          v360 = v550 - p2_pred(v359, v521->b0);
          v521->w2 = p2_bump(v359, v360, 2);
          v361 = v522->w2;
          v362 = v550 - p2_pred(v361, v522->b0);
          v522->w2 = p2_bump(v361, v362, 3);
          *(uint16_t *)&v521[1].w2 += (uint32_t)(v550 - p2_pred(v521[1].w2, v521[1].b0) + 2) >> 2;
          v363 = v523->w2;
          v364 = v548 - p2_pred(v363, v523->b0);
          v523->w2 = p2_bump(v363, v364, 3);
          v365 = v518->w2;
          v366 = v550 - p2_pred(v365, v518->b0);
          v518->w2 = p2_bump(v365, v366, 2);
          v367 = v519->w2;
          v368 = v550 - p2_pred(v367, v519->b0);
          v519->w2 = p2_bump(v367, v368, 3);
          *(uint16_t *)&v518[1].w2 += (uint32_t)(v550 - p2_pred(v518[1].w2, v518[1].b0) + 2) >> 2;
          v369 = v520->w2;
          v370 = v548 - p2_pred(v369, v520->b0);
          v520->w2 = p2_bump(v369, v370, 3);
          v371 = v515->w2;
          v372 = v550 - p2_pred(v371, v515->b0);
          v515->w2 = p2_bump(v371, v372, 2);
          v373 = v516->w2;
          v374 = v550 - p2_pred(v373, *(uint8_t *)&v516->b0);
          v516->w2 = p2_bump(v373, v374, 3);
          v375 = v550;
          *(uint16_t *)&v515[1].w2 += (uint32_t)(v550 - p2_pred(v515[1].w2, v515[1].b0) + 2) >> 2;
          v376 = v517->w2;
          v377 = p2_pred(v376, v517->b0);
          v550 = v375;
          *(uint16_t *)&v517->w2 = v376
                               + ((32 * ((v548 - v377 > deadzone_hi) - (uint32_t)(v548 - v377 < deadzone_lo))
                                 + v548
                                 - v377
                                 + 4) >> 3);
          v378 = v512->w2;
          v379 = v550 - p2_pred(v378, *(uint8_t *)&v512->b0);
          v512->w2 = p2_bump(v378, v379, 2);
          v380 = v513->w2;
          v381 = v550 - p2_pred(v380, *(uint8_t *)&v513->b0);
          v513->w2 = p2_bump(v380, v381, 3);
          v382 = v550;
          *(uint16_t *)&v512[1].w2 += (uint32_t)(v550
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&v512[1].w2, *(uint8_t *)&v512[1].b0)
                                               + 2) >> 2;
          v383 = v514->w2;
          v384 = -v382 - p2_pred(v383, *(uint8_t *)&v514->b0);
          v514->w2 = p2_bump(v383, v384, 3);
        }
        else
        {
          v115 = (int16_t)n2[-1];
          v550 = v108;
          n2[-1] = ((uint32_t)(-v108 - p2_pred(v115, ((uint8_t)n2[-2])) + 4) >> 3) + v115;
          v117 = v542->w2;
          v118 = v550 - p2_pred(v117, *(uint8_t *)&v542->b0);
          v542->w2 = p2_bump(v117, v118, 2);
          v119 = v543->w2;
          v87 = n3 < 3;
          v120 = v550 - p2_pred(v119, v543->b0);
          v543->w2 = p2_bump(v119, v120, 3);
          v121 = v550;
          if ( v87 )
          {
            *(uint16_t *)&v542[1].w2 += (uint32_t)(v550
                                                 - p2_pred(v542[1].w2, *(uint8_t *)&v542[1].b0)
                                                 + 2) >> 2;
            v216 = v542[-1].w2;
            v217 = v121 - p2_pred(v216, *(uint8_t *)&v542[-1].b0);
            v218 = -v121;
            v542[-1].w2 = p2_bump(v216, v217, 3);
            v219 = v544->w2;
            v220 = v218 - p2_pred(v219, v544->b0);
            v544->w2 = p2_bump(v219, v220, 3);
            v221 = v539->w2;
            v222 = v550;
            v223 = v550 - p2_pred(v221, *(uint8_t *)&v539->b0);
            v539->w2 = p2_bump(v221, v223, 2);
            v224 = v540->w2;
            v225 = v222 - p2_pred(v224, v540->b0);
            v540->w2 = p2_bump(v224, v225, 3);
            *(uint16_t *)&v539[1].w2 += (uint32_t)(v222
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&v539[1].w2, *(uint8_t *)&v539[1].b0)
                                                 + 2) >> 2;
            v226 = v539[-1].w2;
            v227 = v222 - p2_pred(v226, *(uint8_t *)&v539[-1].b0);
            v539[-1].w2 = p2_bump(v226, v227, 3);
            v228 = v541->w2;
            v229 = v218 - p2_pred(v228, *(uint8_t *)&v541->b0);
            v541->w2 = p2_bump(v228, v229, 3);
            v230 = v536->w2;
            v231 = v550;
            v232 = v550 - p2_pred(v230, *(uint8_t *)&v536->b0);
            v536->w2 = p2_bump(v230, v232, 2);
            v233 = v537->w2;
            v234 = v231 - p2_pred(v233, v537->b0);
            v537->w2 = p2_bump(v233, v234, 3);
            *(uint16_t *)&v536[1].w2 += (uint32_t)(v231
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&v536[1].w2, *(uint8_t *)&v536[1].b0)
                                                 + 2) >> 2;
            v235 = v536[-1].w2;
            v236 = v231 - p2_pred(v235, *(uint8_t *)&v536[-1].b0);
            v536[-1].w2 = p2_bump(v235, v236, 3);
            v237 = v538->w2;
            v238 = v218 - p2_pred(v237, *(uint8_t *)&v538->b0);
            v538->w2 = p2_bump(v237, v238, 3);
            v239 = v533->w2;
            v240 = v550;
            v241 = v550 - p2_pred(v239, v533->b0);
            v533->w2 = p2_bump(v239, v241, 2);
            v242 = v534->w2;
            v243 = v240 - p2_pred(v242, v534->b0);
            v534->w2 = p2_bump(v242, v243, 3);
            *(uint16_t *)&v533[1].w2 += (uint32_t)(v240
                                                 - p2_pred(v533[1].w2, v533[1].b0)
                                                 + 2) >> 2;
            v244 = v533[-1].w2;
            v245 = v240 - p2_pred(v244, v533[-1].b0);
            v533[-1].w2 = p2_bump(v244, v245, 3);
            v246 = v535->w2;
            v247 = v218 - p2_pred(v246, v535->b0);
            v535->w2 = p2_bump(v246, v247, 3);
            v248 = v530->w2;
            v249 = v550;
            v250 = v550 - p2_pred(v248, v530->b0);
            v530->w2 = p2_bump(v248, v250, 2);
            v251 = v531->w2;
            // `LOBYTE(v248) = v531->b0` and `v248 & 31` was the rate: the mask
            // reads only the byte that was written.
            v252 = v249 - p2_pred(v251, v531->b0);
            v550 = v249;
            v531->w2 = p2_bump(v251, v252, 3);
            v253 = v550;
            *(uint16_t *)&v530[1].w2 += (uint32_t)(v550
                                                 - p2_pred(v530[1].w2, v530[1].b0)
                                                 + 2) >> 2;
            v254 = v530[-1].w2;
            v255 = v253 - p2_pred(v254, v530[-1].b0);
            v530[-1].w2 = p2_bump(v254, v255, 3);
            v256 = v532->w2;
            v573 = -v253;
            v257 = -v253 - p2_pred(v256, v532->b0);
            v532->w2 = p2_bump(v256, v257, 3);
            v258 = v527->w2;
            v259 = v550;
            v260 = v550 - p2_pred(v258, v527->b0);
            v527->w2 = p2_bump(v258, v260, 2);
            v261 = v528->w2;
            v262 = v259 - p2_pred(v261, v528->b0);
            v528->w2 = p2_bump(v261, v262, 3);
            *(uint16_t *)&v527[1].w2 += (uint32_t)(v259
                                                 - p2_pred(v527[1].w2, v527[1].b0)
                                                 + 2) >> 2;
            v263 = v527[-1].w2;
            v264 = v259 - p2_pred(v263, v527[-1].b0);
            v527[-1].w2 = p2_bump(v263, v264, 3);
            v265 = v529->w2;
            v266 = v573 - p2_pred(v265, v529->b0);
            v529->w2 = p2_bump(v265, v266, 3);
            v267 = v524->w2;
            v268 = v550;
            v269 = v550 - p2_pred(v267, v524->b0);
            v524->w2 = p2_bump(v267, v269, 2);
            v270 = v525->w2;
            v271 = v268 - p2_pred(v270, *(uint8_t *)&v525->b0);
            v525->w2 = p2_bump(v270, v271, 3);
            *(uint16_t *)&v524[1].w2 += (uint32_t)(v268
                                                 - p2_pred(v524[1].w2, v524[1].b0)
                                                 + 2) >> 2;
            v272 = v524[-1].w2;
            v273 = v268 - p2_pred(v272, v524[-1].b0);
            v524[-1].w2 = p2_bump(v272, v273, 3);
            v274 = v526->w2;
            v275 = v573 - p2_pred(v274, v526->b0);
            v526->w2 = p2_bump(v274, v275, 3);
            v276 = v521->w2;
            v277 = v550;
            v278 = v550 - p2_pred(v276, v521->b0);
            v521->w2 = p2_bump(v276, v278, 2);
            v279 = v522->w2;
            v280 = v277 - p2_pred(v279, v522->b0);
            v522->w2 = p2_bump(v279, v280, 3);
            *(uint16_t *)&v521[1].w2 += (uint32_t)(v277
                                                 - p2_pred(v521[1].w2, v521[1].b0)
                                                 + 2) >> 2;
            v281 = v521[-1].w2;
            v282 = v277 - p2_pred(v281, v521[-1].b0);
            v521[-1].w2 = p2_bump(v281, v282, 3);
            v283 = v523->w2;
            v284 = v573 - p2_pred(v283, v523->b0);
            v523->w2 = p2_bump(v283, v284, 3);
            v285 = v518->w2;
            v286 = v550;
            v287 = v550 - p2_pred(v285, v518->b0);
            v518->w2 = p2_bump(v285, v287, 2);
            v288 = v519->w2;
            v289 = v286 - p2_pred(v288, v519->b0);
            v519->w2 = p2_bump(v288, v289, 3);
            *(uint16_t *)&v518[1].w2 += (uint32_t)(v286
                                                 - p2_pred(v518[1].w2, v518[1].b0)
                                                 + 2) >> 2;
            v290 = v518[-1].w2;
            v291 = v286 - p2_pred(v290, v518[-1].b0);
            v292 = -v286;
            v518[-1].w2 = p2_bump(v290, v291, 3);
            v293 = v520->w2;
            v294 = v292 - p2_pred(v293, v520->b0);
            v520->w2 = p2_bump(v293, v294, 3);
            v295 = v515->w2;
            v296 = v550;
            v297 = v550 - p2_pred(v295, v515->b0);
            v515->w2 = p2_bump(v295, v297, 2);
            v298 = v516->w2;
            v299 = v296 - p2_pred(v298, *(uint8_t *)&v516->b0);
            v516->w2 = p2_bump(v298, v299, 3);
            *(uint16_t *)&v515[1].w2 += (uint32_t)(v296
                                                 - p2_pred(v515[1].w2, v515[1].b0)
                                                 + 2) >> 2;
            v300 = v515[-1].w2;
            v301 = v296 - p2_pred(v300, v515[-1].b0);
            v515[-1].w2 = p2_bump(v300, v301, 3);
            v302 = v517->w2;
            v303 = v292 - p2_pred(v302, v517->b0);
            v517->w2 = p2_bump(v302, v303, 3);
            v304 = v512->w2;
            v305 = v550;
            v306 = v550 - p2_pred(v304, *(uint8_t *)&v512->b0);
            v512->w2 = p2_bump(v304, v306, 2);
            v307 = v513->w2;
            v308 = v305 - p2_pred(v307, *(uint8_t *)&v513->b0);
            v513->w2 = p2_bump(v307, v308, 3);
            *(uint16_t *)&v512[1].w2 += (uint32_t)(v305
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&v512[1].w2, *(uint8_t *)&v512[1].b0)
                                                 + 2) >> 2;
            v309 = v512[-1].w2;
            v310 = v305 - p2_pred(v309, *(uint8_t *)&v512[-1].b0);
            v512[-1].w2 = p2_bump(v309, v310, 3);
            v214 = v514->w2;
            v215 = v292 - p2_pred(v214, *(uint8_t *)&v514->b0);
          }
          else
          {
            v122 = v542[-1].w2;
            v123 = v550 - p2_pred(v122, *(uint8_t *)&v542[-1].b0);
            v542[-1].w2 = p2_bump(v122, v123, 3);
            v124 = v544->w2;
            v547 = -v121;
            v125 = -v121 - p2_pred(v124, v544->b0);
            v544->w2 = p2_bump(v124, v125, 3);
            v126 = v539->w2;
            v127 = v550;
            v128 = v550 - p2_pred(v126, *(uint8_t *)&v539->b0);
            v539->w2 = p2_bump(v126, v128, 2);
            v129 = v540->w2;
            v130 = v127 - p2_pred(v129, v540->b0);
            v540->w2 = p2_bump(v129, v130, 3);
            v131 = v539[-1].w2;
            v132 = v127 - p2_pred(v131, *(uint8_t *)&v539[-1].b0);
            v539[-1].w2 = p2_bump(v131, v132, 3);
            v133 = v541->w2;
            v134 = v547 - p2_pred(v133, *(uint8_t *)&v541->b0);
            v541->w2 = p2_bump(v133, v134, 3);
            v135 = v536->w2;
            v136 = v550;
            v137 = v550 - p2_pred(v135, *(uint8_t *)&v536->b0);
            v536->w2 = p2_bump(v135, v137, 2);
            v138 = v537->w2;
            v139 = v136 - p2_pred(v138, v537->b0);
            v537->w2 = p2_bump(v138, v139, 3);
            v140 = v536[-1].w2;
            v141 = v136 - p2_pred(v140, *(uint8_t *)&v536[-1].b0);
            v536[-1].w2 = p2_bump(v140, v141, 3);
            v142 = v538->w2;
            v143 = v547 - p2_pred(v142, *(uint8_t *)&v538->b0);
            v538->w2 = p2_bump(v142, v143, 3);
            v144 = v533->w2;
            v145 = v550;
            v146 = v550 - p2_pred(v144, v533->b0);
            v533->w2 = p2_bump(v144, v146, 2);
            v147 = v534->w2;
            v148 = v145 - p2_pred(v147, v534->b0);
            v534->w2 = p2_bump(v147, v148, 3);
            v149 = v533[-1].w2;
            v150 = v145 - p2_pred(v149, v533[-1].b0);
            v533[-1].w2 = p2_bump(v149, v150, 3);
            v151 = v535->w2;
            v152 = v547 - p2_pred(v151, v535->b0);
            v535->w2 = p2_bump(v151, v152, 3);
            v153 = v530->w2;
            v154 = v550;
            v155 = v550 - p2_pred(v153, v530->b0);
            v530->w2 = p2_bump(v153, v155, 2);
            v156 = v531->w2;
            v157 = v154 - p2_pred(v156, v531->b0);
            v531->w2 = p2_bump(v156, v157, 3);
            v158 = v530[-1].w2;
            v159 = v154 - p2_pred(v158, v530[-1].b0);
            v530[-1].w2 = p2_bump(v158, v159, 3);
            v160 = v532->w2;
            v547 -= p2_pred(v160, v532->b0);
            v161 = v550;
            v532->w2 = p2_bump(v160, v547, 3);
            v162 = v527->w2;
            v163 = v161 - p2_pred(v162, v527->b0);
            v527->w2 = p2_bump(v162, v163, 2);
            v164 = v528->w2;
            v165 = v161 - p2_pred(v164, v528->b0);
            v528->w2 = p2_bump(v164, v165, 3);
            v166 = v527[-1].w2;
            v167 = v161 - p2_pred(v166, v527[-1].b0);
            v168 = -v161;
            v527[-1].w2 = p2_bump(v166, v167, 3);
            v169 = v529->w2;
            v170 = v168 - p2_pred(v169, v529->b0);
            v529->w2 = p2_bump(v169, v170, 3);
            v171 = v524->w2;
            v172 = v550;
            v173 = v550 - p2_pred(v171, v524->b0);
            v524->w2 = p2_bump(v171, v173, 2);
            v174 = v525->w2;
            v175 = v172 - p2_pred(v174, *(uint8_t *)&v525->b0);
            v525->w2 = p2_bump(v174, v175, 3);
            v176 = v524[-1].w2;
            v177 = v172 - p2_pred(v176, v524[-1].b0);
            v524[-1].w2 = p2_bump(v176, v177, 3);
            v178 = v526->w2;
            v179 = v168 - p2_pred(v178, v526->b0);
            v526->w2 = p2_bump(v178, v179, 3);
            v180 = v521->w2;
            v181 = v550;
            v182 = v550 - p2_pred(v180, v521->b0);
            v521->w2 = p2_bump(v180, v182, 2);
            v183 = v522->w2;
            v184 = v181 - p2_pred(v183, v522->b0);
            v522->w2 = p2_bump(v183, v184, 3);
            v185 = v521[-1].w2;
            v186 = v181 - p2_pred(v185, v521[-1].b0);
            v521[-1].w2 = p2_bump(v185, v186, 3);
            v187 = v523->w2;
            v188 = v168 - p2_pred(v187, v523->b0);
            v523->w2 = p2_bump(v187, v188, 3);
            v189 = v518->w2;
            v190 = v550;
            v191 = v550 - p2_pred(v189, v518->b0);
            v518->w2 = p2_bump(v189, v191, 2);
            v192 = v519->w2;
            v193 = v190 - p2_pred(v192, v519->b0);
            v519->w2 = p2_bump(v192, v193, 3);
            v194 = v518[-1].w2;
            v195 = v190 - p2_pred(v194, v518[-1].b0);
            v518[-1].w2 = p2_bump(v194, v195, 3);
            v196 = v520->w2;
            v197 = v168 - p2_pred(v196, v520->b0);
            v520->w2 = p2_bump(v196, v197, 3);
            v198 = v515->w2;
            v199 = v550;
            v200 = v550 - p2_pred(v198, v515->b0);
            v515->w2 = p2_bump(v198, v200, 2);
            v201 = v516->w2;
            v202 = v199 - p2_pred(v201, *(uint8_t *)&v516->b0);
            v550 = v199;
            v516->w2 = p2_bump(v201, v202, 3);
            v203 = v515[-1].w2;
            v204 = v199 - p2_pred(v203, v515[-1].b0);
            v515[-1].w2 = p2_bump(v203, v204, 3);
            v205 = v517->w2;
            v571 = -v199;
            v206 = -v199 - p2_pred(v205, v517->b0);
            v517->w2 = p2_bump(v205, v206, 3);
            v207 = v512->w2;
            v208 = v550;
            v209 = v550 - p2_pred(v207, *(uint8_t *)&v512->b0);
            v512->w2 = p2_bump(v207, v209, 2);
            v210 = v513->w2;
            v211 = v208 - p2_pred(v210, *(uint8_t *)&v513->b0);
            v513->w2 = p2_bump(v210, v211, 3);
            v212 = v512[-1].w2;
            v213 = v208 - p2_pred(v212, *(uint8_t *)&v512[-1].b0);
            v512[-1].w2 = p2_bump(v212, v213, 3);
            v214 = v514->w2;
            v215 = v571 - p2_pred(v214, *(uint8_t *)&v514->b0);
          }
          v514->w2 = p2_bump(v214, v215, 3);
        }
      }
    }
    ++n5;
  }
  while ( n5 < 5 );
  v385 = (AltP2Block *)(v578);
  ctx = v578->ctx;
  ++v578->cursor[0];
  v387 = (&v385->freq[ctx]);
  ++v385->cursor[1];
  ++v385->cursor[2];
  ++v385->cursor[3];
  ++v385->cursor[4];
  n0x10 = v387[0].step;
  if ( n0x10 > 0x10 )
  {
    v511 = a4 & 1;
    n15 = ctx & 0xF;
    v508 = v385->ctx_pair[a4 & 1];
    if ( n15 < 15 )
    {
      v391 = v387[1].f[0];
      v392 = v387[1].f[1];
      n2 = (uint16_t *)&v387[1];
      if ( v387[1].f[2] + v392 + v391 > 29696 )
        __rescale_three_way(&v387[1]);
      v393 = (10 * (uint32_t)v387[1].step) >> 4;
      if ( a4 )
      {
        n2[3 - v511] += v393;
        __update_binary_pair(model_strip((uint32_t)v508 + 1), (a4 - 1) >> 1);
      }
      else
      {
        n2[1] += v393;
      }
      if ( n15 <= 0 )
      {
LABEL_37:
        n0x10 = v385->ctx;
        if ( v385->freq[n0x10].step <= 0x1Au )
          return n0x10;
        v397 = v385->ctx_w[1].sel;
        v398 = 2 - (v385->fold[(uint8_t)-a5] & 1);
        if ( !v385->fold[(uint8_t)-a5] )
          v398 = v385->fold[(uint8_t)-a5];
        v399 = v385->ctx_w[2].sel;
        v400 = v385->ctx_w[0].w[1] + (v385->ctx & 0x3F);
        v510 = v398;
        n0x10 = v385->ctx_w[4].w[2 - v385->ctx_w[4].sel]
              + v385->ctx_w[3].w[2 - v385->ctx_w[3].sel]
              + v385->ctx_w[2].w[2 - v399]
              + v385->ctx_w[1].w[2 - v397]
              + v400;
        n0x10_3 = &v385->freq[n0x10];
        p2_rec = n0x10_3;
        if ( n15 < 15 )
        {
          n2_2 = &n0x10_3[1];
          v403 = n0x10_3[1].f[0];
          v404 = n0x10_3[1].f[1];
          n2 = (uint16_t *)n2_2;
          if ( n2_2->f[2] + v404 + v403 > 29696 )
          {
            n0x10_1 = n0x10;
            __rescale_three_way(n2_2);
            n0x10 = n0x10_1;
          }
          n2[v510 + 1] += (p2_rec[1].step & 0xFFFC) >> 2;
          if ( n15 <= 0 )
            goto LABEL_48;
        }
        else
        {
        }
        v406 = p2_rec[-1].f[0];
        v407 = p2_rec[-1].f[1];
        n2 = (uint16_t *)&p2_rec[-1];
        if ( p2_rec[-1].f[2] + v407 + v406 > 29696 )
        {
          n0x10_1 = n0x10;
          __rescale_three_way(&p2_rec[-1]);
          n0x10 = n0x10_1;
        }
        n2[v510 + 1] += (uint16_t)(p2_rec[-1].step & 0xFFFC) >> 2;
LABEL_48:
        if ( a4 )
        {
          n2_half = (a4 - 1) >> 1;
          // Not the record: `n0xF0` is the high nibble here, and a record
          // address from the next assignment on.  MSVC gave both one register.
          hi_nibble = (uint8_t)v508 & 0xF0;
          if ( hi_nibble >= 0xF0
            || (n0x10_1 = n0x10,
                __update_binary_pair(model_strip((uint32_t)v508 + 16), n2_half),
                n0x10 = n0x10_1,
                hi_nibble > 0) )
          {
            n0x10_1 = n0x10;
            __update_binary_pair(model_strip((uint32_t)v508 - 16), n2_half);
            n0x10 = n0x10_1;
          }
        }
        v408 = &p2_rec[0];
        if ( p2_rec[0].f[2]
           + p2_rec[0].f[1]
           + p2_rec[0].f[0] > 29696 )
        {
          n0x10_1 = n0x10;
          __rescale_three_way(&p2_rec[0]);
          n0x10 = n0x10_1;
        }
        v408->f[v510] += (6 * (uint32_t)p2_rec[0].step) >> 4;
        if ( !v385->plane_idx || v385->freq[v385->ctx].step > 0x100u )
        {
          v409 = 2 - v511;
          if ( !a4 )
            v409 = 0;
          v410 = v385->ctx_w[0].sel;
          v511 = v409;
          if ( v410 == 1 )
          {
            v491 = &v385->freq[v385->ctx_w[0].w[0] + n0x10 - v385->ctx_w[0].w[1]];
            v492 = v385->freq[v385->ctx_w[0].w[0] + n0x10 - v385->ctx_w[0].w[1]].f[0];
            v493 = v385->freq[v385->ctx_w[0].w[0] + n0x10 - v385->ctx_w[0].w[1]].f[1];
            v508 = (uintptr_t)v491;
            if ( v491->f[2] + v493 + v492 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v491);
              n0x10 = n0x10_1;
            }
            v491->f[v510] += (uint16_t)(v491->step & 0xFFFC) >> 2;
            v494 = v385->ctx - v385->ctx_w[0].w[1];
            n0xF0_1 = (&v385->freq[v494 + v385->ctx_w[0].w[0]]);
            n0x10_2 = v385->ctx_w[0].w[2] + v494;
            n0xF0 = n0xF0_1;
            n2_3 = &n0xF0_1[0];
            v497 = n2_3->f[1] + n2_3->f[0];
            v498 = n2_3->f[2];
            n2 = (uint16_t *)n2_3;
            if ( v498 + v497 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(n2_3);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (3 * n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v500 = n0xF0[1].f[0];
              v501 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v501 + v500 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (uint16_t)(n0xF0[1].step & 0xFFFC) >> 2;
              v502 = &v385->freq[n0x10_2 + 1];
              if ( v385->freq[n0x10_2 + 1].f[2]
                 + v385->freq[n0x10_2 + 1].f[1]
                 + v385->freq[n0x10_2 + 1].f[0] > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&v385->freq[n0x10_2 + 1]);
                n0x10 = n0x10_1;
              }
              v502->f[v511] += (v502->step & 0xFFF8) >> 3;
            }
            if ( n15 > 2 )
            {
              v503 = n0xF0[-1].f[0];
              v504 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v504 + v503 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          else
          {
            v411 = &v385->freq[n0x10 + v385->ctx_w[0].w[1] - v385->ctx_w[0].w[2 - v410]];
            if ( v385->freq[n0x10 + v385->ctx_w[0].w[1] - v385->ctx_w[0].w[2 - v410]].f[2]
               + v385->freq[n0x10 + v385->ctx_w[0].w[1] - v385->ctx_w[0].w[2 - v410]].f[1]
               + v385->freq[n0x10 + v385->ctx_w[0].w[1] - v385->ctx_w[0].w[2 - v410]].f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v411);
              n0x10 = n0x10_1;
            }
            v411->f[v510] += (7 * (uint32_t)v411->step) >> 4;
            n0xF0 = ((&v385->freq[v385->ctx_w[0].w[1] + (v385->ctx - v385->ctx_w[0].w[v385->ctx_w[0].sel])]));
            v412 = n0xF0[0].f[0];
            v413 = n0xF0[0].f[1];
            n2 = (uint16_t *)&n0xF0[0];
            if ( n0xF0[0].f[2] + v413 + v412 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&n0xF0[0]);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v415 = n0xF0[1].f[0];
              v416 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v416 + v415 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n0x10_1 = n0x10;
              n2[v511 + 1] = n2[v511 + 1] + ((5 * (uint32_t)n0xF0[1].step) >> 4);
              n0x10 = n0x10_1;
            }
            if ( n15 > 0 )
            {
              v417 = n0xF0[-1].f[0];
              v418 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v418 + v417 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          v419 = v385->ctx_w[1].sel;
          if ( v419 == 1 )
          {
            n2_4 = &v385->freq[v385->ctx_w[1].w[0] + n0x10 - v385->ctx_w[1].w[1]];
            v481 = v385->freq[v385->ctx_w[1].w[0] + n0x10 - v385->ctx_w[1].w[1]].f[0];
            v482 = v385->freq[v385->ctx_w[1].w[0] + n0x10 - v385->ctx_w[1].w[1]].f[1];
            n2 = (uint16_t *)n2_4;
            if ( n2_4->f[2] + v482 + v481 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(n2_4);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v483 = v385->ctx - v385->ctx_w[1].w[1];
            n0xF0_2 = (&v385->freq[v483 + v385->ctx_w[1].w[0]]);
            n0x10_2 = v385->ctx_w[1].w[2] + v483;
            n0xF0 = n0xF0_2;
            v485 = &n0xF0_2[0];
            if ( v485->f[2] + v485->f[1] + v485->f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v485);
              n0x10 = n0x10_1;
            }
            v485->f[v511] += (3 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v486 = n0xF0[1].f[0];
              v487 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v487 + v486 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (uint16_t)(n0xF0[1].step & 0xFFFC) >> 2;
              v488 = &v385->freq[n0x10_2 + 1];
              if ( v385->freq[n0x10_2 + 1].f[2]
                 + v385->freq[n0x10_2 + 1].f[1]
                 + v385->freq[n0x10_2 + 1].f[0] > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&v385->freq[n0x10_2 + 1]);
                n0x10 = n0x10_1;
              }
              v488->f[v511] += (v488->step & 0xFFF8) >> 3;
            }
            if ( n15 > 2 )
            {
              v489 = n0xF0[-1].f[0];
              v490 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v490 + v489 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          else
          {
            v420 = &v385->freq[n0x10 + v385->ctx_w[1].w[1] - v385->ctx_w[1].w[2 - v419]];
            if ( v385->freq[n0x10 + v385->ctx_w[1].w[1] - v385->ctx_w[1].w[2 - v419]].f[2]
               + v385->freq[n0x10 + v385->ctx_w[1].w[1] - v385->ctx_w[1].w[2 - v419]].f[1]
               + v385->freq[n0x10 + v385->ctx_w[1].w[1] - v385->ctx_w[1].w[2 - v419]].f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v420);
              n0x10 = n0x10_1;
            }
            v420->f[v510] += (7 * (uint32_t)v420->step) >> 4;
            n0xF0 = ((&v385->freq[v385->ctx_w[1].w[1] + (v385->ctx - v385->ctx_w[1].w[v385->ctx_w[1].sel])]));
            v421 = n0xF0[0].f[0];
            v422 = n0xF0[0].f[1];
            n2 = (uint16_t *)&n0xF0[0];
            if ( n0xF0[0].f[2] + v422 + v421 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&n0xF0[0]);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v423 = n0xF0[1].f[0];
              v424 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v424 + v423 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (5 * (uint32_t)n0xF0[1].step) >> 4;
            }
            if ( n15 > 0 )
            {
              v425 = n0xF0[-1].f[0];
              v426 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v426 + v425 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          v427 = v385->ctx_w[2].sel;
          if ( v427 == 1 )
          {
            n2_5 = &v385->freq[v385->ctx_w[2].w[0] + n0x10 - v385->ctx_w[2].w[1]];
            v470 = v385->freq[v385->ctx_w[2].w[0] + n0x10 - v385->ctx_w[2].w[1]].f[0];
            v471 = v385->freq[v385->ctx_w[2].w[0] + n0x10 - v385->ctx_w[2].w[1]].f[1];
            n2 = (uint16_t *)n2_5;
            if ( n2_5->f[2] + v471 + v470 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(n2_5);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v472 = v385->ctx - v385->ctx_w[2].w[1];
            n0xF0_3 = (&v385->freq[v472 + v385->ctx_w[2].w[0]]);
            n0x10_2 = v385->ctx_w[2].w[2] + v472;
            n0xF0 = n0xF0_3;
            v474 = &n0xF0_3[0];
            if ( v474->f[2] + v474->f[1] + v474->f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v474);
              n0x10 = n0x10_1;
            }
            v474->f[v511] += (3 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v475 = n0xF0[1].f[0];
              v476 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v476 + v475 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (uint16_t)(n0xF0[1].step & 0xFFFC) >> 2;
              v477 = &v385->freq[n0x10_2 + 1];
              if ( v385->freq[n0x10_2 + 1].f[2]
                 + v385->freq[n0x10_2 + 1].f[1]
                 + v385->freq[n0x10_2 + 1].f[0] > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&v385->freq[n0x10_2 + 1]);
                n0x10 = n0x10_1;
              }
              v477->f[v511] += (v477->step & 0xFFF8) >> 3;
            }
            if ( n15 > 2 )
            {
              v478 = n0xF0[-1].f[0];
              v479 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v479 + v478 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          else
          {
            v428 = &v385->freq[n0x10 + v385->ctx_w[2].w[1] - v385->ctx_w[2].w[2 - v427]];
            if ( v385->freq[n0x10 + v385->ctx_w[2].w[1] - v385->ctx_w[2].w[2 - v427]].f[2]
               + v385->freq[n0x10 + v385->ctx_w[2].w[1] - v385->ctx_w[2].w[2 - v427]].f[1]
               + v385->freq[n0x10 + v385->ctx_w[2].w[1] - v385->ctx_w[2].w[2 - v427]].f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v428);
              n0x10 = n0x10_1;
            }
            v428->f[v510] += (7 * (uint32_t)v428->step) >> 4;
            n0xF0 = ((&v385->freq[v385->ctx_w[2].w[1] + (v385->ctx - v385->ctx_w[2].w[v385->ctx_w[2].sel])]));
            v429 = n0xF0[0].f[0];
            v430 = n0xF0[0].f[1];
            n2 = (uint16_t *)&n0xF0[0];
            if ( n0xF0[0].f[2] + v430 + v429 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&n0xF0[0]);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v431 = n0xF0[1].f[0];
              v432 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v432 + v431 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (5 * (uint32_t)n0xF0[1].step) >> 4;
            }
            if ( n15 > 0 )
            {
              v433 = n0xF0[-1].f[0];
              v434 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v434 + v433 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          v435 = v385->ctx_w[3].sel;
          if ( v435 == 1 )
          {
            n2_6 = &v385->freq[v385->ctx_w[3].w[0] + n0x10 - v385->ctx_w[3].w[1]];
            v459 = v385->freq[v385->ctx_w[3].w[0] + n0x10 - v385->ctx_w[3].w[1]].f[0];
            v460 = v385->freq[v385->ctx_w[3].w[0] + n0x10 - v385->ctx_w[3].w[1]].f[1];
            n2 = (uint16_t *)n2_6;
            if ( n2_6->f[2] + v460 + v459 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(n2_6);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v461 = v385->ctx - v385->ctx_w[3].w[1];
            n0xF0_4 = (&v385->freq[v461 + v385->ctx_w[3].w[0]]);
            n0x10_2 = v385->ctx_w[3].w[2] + v461;
            n0xF0 = n0xF0_4;
            v463 = &n0xF0_4[0];
            if ( v463->f[2] + v463->f[1] + v463->f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v463);
              n0x10 = n0x10_1;
            }
            v463->f[v511] += (3 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v464 = n0xF0[1].f[0];
              v465 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v465 + v464 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (uint16_t)(n0xF0[1].step & 0xFFFC) >> 2;
              v466 = &v385->freq[n0x10_2 + 1];
              if ( v385->freq[n0x10_2 + 1].f[2]
                 + v385->freq[n0x10_2 + 1].f[1]
                 + v385->freq[n0x10_2 + 1].f[0] > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&v385->freq[n0x10_2 + 1]);
                n0x10 = n0x10_1;
              }
              v466->f[v511] += (v466->step & 0xFFF8) >> 3;
            }
            if ( n15 > 2 )
            {
              v467 = n0xF0[-1].f[0];
              v468 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v468 + v467 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          else
          {
            v436 = &v385->freq[n0x10 + v385->ctx_w[3].w[1] - v385->ctx_w[3].w[2 - v435]];
            if ( v385->freq[n0x10 + v385->ctx_w[3].w[1] - v385->ctx_w[3].w[2 - v435]].f[2]
               + v385->freq[n0x10 + v385->ctx_w[3].w[1] - v385->ctx_w[3].w[2 - v435]].f[1]
               + v385->freq[n0x10 + v385->ctx_w[3].w[1] - v385->ctx_w[3].w[2 - v435]].f[0] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(v436);
              n0x10 = n0x10_1;
            }
            v436->f[v510] += (7 * (uint32_t)v436->step) >> 4;
            n0xF0 = ((&v385->freq[v385->ctx_w[3].w[1] + (v385->ctx - v385->ctx_w[3].w[v385->ctx_w[3].sel])]));
            v437 = n0xF0[0].f[0];
            v438 = n0xF0[0].f[1];
            n2 = (uint16_t *)&n0xF0[0];
            if ( n0xF0[0].f[2] + v438 + v437 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&n0xF0[0]);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[0].step) >> 4;
            if ( n15 < 15 )
            {
              v439 = n0xF0[1].f[0];
              v440 = n0xF0[1].f[1];
              n2 = (uint16_t *)&n0xF0[1];
              if ( n0xF0[1].f[2] + v440 + v439 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (5 * (uint32_t)n0xF0[1].step) >> 4;
            }
            if ( n15 > 0 )
            {
              v441 = n0xF0[-1].f[0];
              v442 = n0xF0[-1].f[1];
              n2 = (uint16_t *)&n0xF0[-1];
              if ( n0xF0[-1].f[2] + v442 + v441 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way(&n0xF0[-1]);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[-1].step) >> 4;
            }
          }
          v443 = v385->ctx_w[4].sel;
          if ( v443 == 1 )
          {
            n2_7 = &v385->freq[v385->ctx_w[4].w[0] + n0x10 - v385->ctx_w[4].w[1]];
            v447 = v385->freq[v385->ctx_w[4].w[0] + n0x10 - v385->ctx_w[4].w[1]].f[0];
            v448 = v385->freq[v385->ctx_w[4].w[0] + n0x10 - v385->ctx_w[4].w[1]].f[1];
            v449 = n2_7->f[2];
            n2 = (uint16_t *)n2_7;
            if ( v449 + v448 + v447 > 29696 )
              __rescale_three_way(n2_7);
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v450 = v385->ctx - v385->ctx_w[4].w[1];
            n0x10_4 = &v385->freq[v450 + v385->ctx_w[4].w[0]];
            // An index, not an address -- the same register again.
            rec_idx = v385->ctx_w[4].w[2] + v450;
            p2_rec = n0x10_4;
            v452 = &n0x10_4[0];
            if ( n0x10_4[0].f[2] + n0x10_4[0].f[1] + n0x10_4[0].f[0] > 29696 )
              __rescale_three_way(v452);
            v452->f[v511] += (3 * (uint32_t)p2_rec[0].step) >> 4;
            if ( n15 < 15 )
            {
              v453 = p2_rec[1].f[2];
              v454 = p2_rec[1].f[1] + p2_rec[1].f[0];
              n2 = (uint16_t *)&p2_rec[1];
              if ( v453 + v454 > 29696 )
                __rescale_three_way(&p2_rec[1]);
              n2[v511 + 1] += (uint16_t)(p2_rec[1].step & 0xFFFC) >> 2;
              v456 = &v385->freq[rec_idx + 1];
              if ( v456->f[2] + v456->f[1] + v456->f[0] > 29696 )
                __rescale_three_way(v456);
              n0x10 = (v456->step & 0xFFF8) >> 3;
              v456->f[v511] += n0x10;
            }
            if ( n15 > 2 )
            {
              v457 = &p2_rec[-1];
              if ( p2_rec[-1].f[2]
                 + p2_rec[-1].f[1]
                 + p2_rec[-1].f[0] > 29696 )
                __rescale_three_way(&p2_rec[-1]);
              n0x10 = (uintptr_t)p2_rec;   // the slot's last value, and what this path returns
              v457->f[v511] += (6
                                                               * (uint32_t)p2_rec[-1].step) >> 4;
            }
          }
          else
          {
            v444 = &v385->freq[n0x10 + v385->ctx_w[4].w[1] - v385->ctx_w[4].w[2 - v443]];
            if ( v385->freq[n0x10 + v385->ctx_w[4].w[1] - v385->ctx_w[4].w[2 - v443]].f[2]
               + v385->freq[n0x10 + v385->ctx_w[4].w[1] - v385->ctx_w[4].w[2 - v443]].f[1]
               + v385->freq[n0x10 + v385->ctx_w[4].w[1] - v385->ctx_w[4].w[2 - v443]].f[0] > 29696 )
              __rescale_three_way(v444);
            v444->f[v510] += (7 * (uint32_t)v444->step) >> 4;
            v445 = ((&v385->freq[v385->ctx_w[4].w[1] + (v385->ctx - v385->ctx_w[4].w[v385->ctx_w[4].sel])]));
            if ( v445[0].f[2] + v445[0].f[1] + v445[0].f[0] > 29696 )
              __rescale_three_way(&v445[0]);
            v445[0].f[v511] += (7 * (uint32_t)v445[0].step) >> 4;
            if ( n15 < 15 )
            {
              if ( v445[1].f[2] + v445[1].f[1] + v445[1].f[0] > 29696 )
                __rescale_three_way(&v445[1]);
              n0x10 = v445[1].step;
              v445[1].f[v511] += (5 * n0x10) >> 4;
            }
            if ( n15 > 0 )
            {
              if ( v445[-1].f[2] + v445[-1].f[1] + v445[-1].f[0] > 29696 )
                __rescale_three_way(&v445[-1]);
              n0x10 = v445[-1].step;
              v445[-1].f[v511] += (6 * n0x10) >> 4;
            }
          }
        }
        return n0x10;
      }
      v387 = (&v385->freq[v385->ctx]);
    }
    v394 = v387[-1].f[0];
    v395 = v387[-1].f[1];
    n2 = (uint16_t *)&v387[-1];
    if ( v387[-1].f[2] + v395 + v394 > 29696 )
      __rescale_three_way(&v387[-1]);
    v396 = (13 * (uint32_t)v387[-1].step) >> 4;
    if ( a4 )
    {
      n2[3 - v511] += v396;
      __update_binary_pair(model_strip((uint32_t)v508 - 1), (a4 - 1) >> 1);
    }
    else
    {
      n2[1] += v396;
    }
    goto LABEL_37;
  }
  return n0x10;
}


void __alt_p2_d8_decode_body(AltP2Block *lpAddress, int8_t ArgList, uint8_t *a5, int32_t i, int32_t a7)
{
  P2Ctx *v11, *v12, *v15, *buf3, *v73, *buf2, *buf1, *v76;
  int32_t v95;
  // These shared `__frame.v95` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t v96;
  uint8_t *v97;
  ;
  P2Ctx *v25;   // a record cursor
  P2Ctx *v32;   // a record cursor
  P2Ctx *v54;   // a record cursor
  P2Ctx *v62;   // a record cursor
  P2Ctx *v69;   // a record cursor
  P2Ctx *v80;   // a record cursor
  P2Ctx *v84;   // a record cursor
  P2Ctx *v88;   // a record cursor
  uintptr_t v78, v82, v86, v90, v93;
  P2Ctx *v21;
  P2Ctx *v28, *v58, *v66;   // `cursor[0]`, the record being written
  int32_t *cur, *v51;   // the row cursors, four bytes a step
  bool v17;
  int16_t v14;
  // The five planes, held across the rotation that ends a row.
  int32_t i_1, v13, v49, *v50, v77, v79, v81, v83, v85, v87, v89, v92, v94;
  int64_t v16;
  uint32_t j;
  __rc_begin_decode(ArgList);
  i_1 = i;
  v11 = lpAddress->cursor[0];
  v12 = v11;
  if ( i > 0 )
  {
    v95 = 0;
    while ( 1 )
    {
      v13 = v11[-1].lane[0] >> 4;
      lpAddress->ctx_pair[0] = lpAddress->ctx + (*(uint32_t *)&lpAddress->ctx_delta[v13 + 4]);
      lpAddress->ctx_pair[1] = lpAddress->ctx + (*(uint32_t *)&lpAddress->ctx_delta[v13]);
      v14 = (uint8_t)(((uint16_t)lpAddress->cursor[0][-1].lane[0] >> 4)
                            + *(uint8_t *)(__alt_p2_decode_symbol(&lpAddress->freq[lpAddress->ctx
                                                               + lpAddress->ctx_w[3].w[(v12[-1].lane[0] <= v12[-2].lane[0])
                                                                            + (v12[-1].lane[0] < v12[-2].lane[0])]
                                                               + lpAddress->ctx_w[2].w[v12[-2].sign]
                                                               + lpAddress->ctx_w[1].w[v12[-1].sign]
                                                               + lpAddress->ctx_w[0].w[(((uint32_t)(v13 - 115) >> 31)
                                                                            + ((uint32_t)(v13 - 17) >> 31))]
                                                               + lpAddress->ctx_w[4].w[1]], lpAddress->ctx_pair)
                                       + (uintptr_t)lpAddress
                                       + 280496));
      *a5 = v14;
      v14 *= 16;
      lpAddress->cursor[0]->lane[0] = v14;
      ++a5;
      lpAddress->cursor[0]->lane[1] = v14;
      v15 = lpAddress->cursor[0];
      v16 = (int16_t)(v15->lane[0] - v15[-1].lane[0]);
      v15->lane[2] = v16;
      LOWORD(v16) = (WORD2(v16) ^ v16) - WORD2(v16);
      lpAddress->cursor[0]->lane[3] = v16;
      lpAddress->cursor[0]->lane[7] = v16;
      lpAddress->cursor[0]->lane[6] = v16;
      lpAddress->cursor[0]->lane[5] = v16;
      lpAddress->cursor[0]->lane[4] = (uint32_t)lpAddress->cursor[0]->lane[5] >> 1;
      lpAddress->cursor[0]->mag = 2;
      lpAddress->cursor[0]->sign = (lpAddress->cursor[0]->lane[2] <= 0)
                                                       + (lpAddress->cursor[0]->lane[2] < 0);
      v11 = (lpAddress->cursor[0] + 1);
      v17 = v95 + 1 < i;
      lpAddress->cursor[0] = v11;
      ++v95;
      if ( !v17 )
        break;
      v12 = lpAddress->cursor[0];
    }
    i_1 = i;
  }
  ((P2Ctx *)v11)[0] = ((P2Ctx *)v11)[-1];
  v21 = lpAddress->cursor[0];
  (v21)[1] = (v21)[-1];
  v25 = (P2Ctx *)lpAddress->cursor[0];
  v25[2] = v25[-1];
  v28 = lpAddress->cursor[0];
  (v28)[3] = (v28)[-1];
  v32 = (P2Ctx *)lpAddress->cursor[0];
  v32[4] = v32[-1];
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec1 = lpAddress->cursor[0] + (-i_1);
  rec1[-1] = rec1[0];
  rec1[-2] = rec1[1];
  rec1[-3] = rec1[2];
  rec1[-4] = rec1[3];
  rec1[-5] = rec1[4];
  rec1[-6] = rec1[5];
  rec1[-7] = rec1[6];
  rec1[-8] = rec1[7];
  memcpy(lpAddress->buf[1],lpAddress->buf[0],18 * i_1 + 234);
  memcpy(lpAddress->buf[2],lpAddress->buf[0],18 * i_1 + 234);
  memcpy(lpAddress->buf[3],lpAddress->buf[0],18 * i_1 + 234);
  if ( a7 > 1 )
  {
    v96 = 0;
    do
    {
      // Start the next row: carry the last word of this one forward, swap
      // the two row buffers, and re-derive the two cursors from them.
      cur = lpAddress->cur;
      v49 = cur[-1];
      v97 = a5;
      cur[1] = v49;
      *lpAddress->cur = v49;
      v50 = lpAddress->row0;
      v51 = lpAddress->row1;
      lpAddress->row0 = v51;
      lpAddress->row1 = v50;
      v51 += 2;
      v50 += 2;
      lpAddress->cur = v51;
      lpAddress->above = v50;
      v51[-1] = *v50;
      lpAddress->cur[-2] = *v50;
      lpAddress->f278528_q = 0;
      lpAddress->f278536 = 0;
      lpAddress->f278540 = 0;
      lpAddress->f278542 = 0;
      lpAddress->bias[0] = 0.0f;

      lpAddress->bias[1] = 0.0f;

      lpAddress->bias[2] = 0.0f;

      lpAddress->bias[3] = 0.0f;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(lpAddress->p2_row, 0, sizeof lpAddress->p2_row);
      v54 = (P2Ctx *)lpAddress->cursor[0];
      v54[0] = v54[-1];
      v58 = lpAddress->cursor[0];
      (v58)[1] = (v58)[-2];
      v62 = (P2Ctx *)lpAddress->cursor[0];
      v62[2] = v62[-3];
      v66 = lpAddress->cursor[0];
      (v66)[3] = (v66)[-4];
      v69 = (P2Ctx *)lpAddress->cursor[0];
      v69[4] = v69[-5];
      // The five planes rotate right by one, and each cursor follows its own
      // 144 bytes in.
      buf3 = lpAddress->buf[3];
      v73 = lpAddress->buf[4];
      buf2 = lpAddress->buf[2];
      buf1 = lpAddress->buf[1];
      v76 = lpAddress->buf[0];
      lpAddress->buf[4] = buf3;
      lpAddress->buf[3] = buf2;
      lpAddress->buf[2] = buf1;
      lpAddress->buf[0] = v73;
      lpAddress->buf[1] = v76;
      v73 += 8;
      lpAddress->cursor[0] = v73;
      v76 += 8;
      lpAddress->cursor[1] = v76;
      lpAddress->cursor[2] = buf1 + 8;
      lpAddress->cursor[3] = buf2 + 8;
      lpAddress->cursor[4] = buf3 + 8;
      ((P2Ctx *)v73)[-1] = ((P2Ctx *)v76)[0];
      v77 = (int32_t)(lpAddress->cursor[0]);
      v78 = (uintptr_t)(lpAddress->cursor[1]);
      ((P2Ctx *)v77)[-2] = ((P2Ctx *)v78)[1];
      v79 = (int32_t)(lpAddress->cursor[0]);
      v80 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v79)[-3] = v80[2];
      v81 = (int32_t)(lpAddress->cursor[0]);
      v82 = (uintptr_t)(lpAddress->cursor[1]);
      ((P2Ctx *)v81)[-4] = ((P2Ctx *)v82)[3];
      v83 = (int32_t)(lpAddress->cursor[0]);
      v84 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v83)[-5] = v84[4];
      v85 = (int32_t)(lpAddress->cursor[0]);
      v86 = (uintptr_t)(lpAddress->cursor[1]);
      ((P2Ctx *)v85)[-6] = ((P2Ctx *)v86)[5];
      v87 = (int32_t)(lpAddress->cursor[0]);
      v88 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v87)[-7] = v88[6];
      v89 = (int32_t)(lpAddress->cursor[0]);
      v90 = (uintptr_t)(lpAddress->cursor[1]);
      ((P2Ctx *)v89)[-8] = ((P2Ctx *)v90)[7];
      lpAddress->cursor[0]->lane[1] = 0;
      if ( i > 0 )
      {
        for ( j = 0; j < i; ++j )
        {
          v92 = __alt_p2_context((AltP2Block *)lpAddress, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          v93 = __alt_p2_decode_symbol(&lpAddress->freq[lpAddress->ctx], lpAddress->ctx_pair);
          v94 = (uint8_t)(v92 + (*(uint8_t *)&lpAddress->unfold[v93]));
          v97[j] = v94;
          __alt_p2_model(lpAddress, v94, v93, v94 - v92);
          a5 = &v97[j + 1];
        }
      }
      ++v96;
    }
    while ( v96 < a7 - 1 );
  }
  __rc_end_decode();
}

void __alt_model_p2_d8_decode( uint8_t *Src, int32_t i, int32_t a5)
{
  ;
  void *v5, **lpAddress;
  v5 = bmf_page_alloc(0x103E30u);
  if ( v5 )
    lpAddress = (void **)__alt_p2_alloc((AltP2Block *)v5, i, 0);
  else
    lpAddress = nullptr;
  __alt_p2_d8_decode_body((AltP2Block *)(int32_t)lpAddress, i, Src, i, a5);
  if ( lpAddress )
    __alt_p2_free((void **)lpAddress, 1);
}

int32_t __alt_model_p2_decode(uint16_t *p_i, uint8_t *Src) {   P2Ctx *v105,
        *v115, *v124, *v132, *v30, *v34, *v38, *v67, *v75, *buf3, *v82, *buf2,
        *buf1, *v85, *v86, *v88, *v90, *v92, *v94, *v96, *v98;
  struct alignas(16) AltModelP2DecodeFrame {   // 276 bytes, one stack frame
      uint8_t   _gap0[4];   // was uint32_t Size_1
      uint8_t slot4[16];
      uint8_t slot20[16];
      uint8_t slot36[16];
      uint8_t slot52[16];
      uint8_t slot68[16];
      uint8_t slot84[16];
      uint8_t slot100[16];
      uint8_t slot116[16];
      uint8_t slot132[16];
      uint8_t   _gap1[4];   // was int32_t v149
      int32_t v150;
      uint8_t   _gap2[4];   // was uint8_t * Src_1
      uint8_t   _gap3[4];   // was int32_t v153
      uint8_t   _gap4[4];   // was int32_t v154
      uint8_t   _gap5[16];   // was AltP2Block * plane
      uint8_t   _gap6[4];   // was int32_t v159
      uint8_t   _gap7[4];   // was int32_t v160
      uint8_t   _gap8[4];   // was uint32_t Size
      uint8_t   _gap9[4];   // was int32_t ArgList
      uint8_t   _gap10[4];   // was int32_t v163
      uint8_t   _gap11[4];   // was uint32_t i_1
      uint8_t   _gap12[4];   // was uint32_t v165
      uint8_t   _gap13[4];   // was int32_t n4_2
      uint8_t   _gap14[4];   // was uint32_t i_2
      uint8_t   _gap15[4];   // was int32_t v168
      uint8_t   _gap16[4];   // was int32_t v169
      uint8_t   _gap17[4];   // was int32_t v170
      uint8_t   _gap18[4];   // was uint32_t v171
      uint8_t _pad0[40];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 288, "frame layout moved");
  int32_t v149;
  // These shared `__frame.v150` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v151;
  uint8_t *Src_1;
  int32_t v153;
  int32_t v154;
  AltP2Block * plane[4];
  int32_t v159;
  int32_t v160;
  uint32_t Size;
  int32_t ArgList;
  int32_t v163;
  uint32_t i_1;
  uint32_t v165;
  int32_t n4_2;
  uint32_t i_2;
  uint32_t v171;
  ;
  AltP2Block *v56;
  AltP2Block *v14;
  AltP2Block *v25;
  P2Ctx *v26;   // a record cursor
  P2Ctx *v41;   // a record cursor
  P2Ctx *v63;   // a record cursor
  P2Ctx *v71;   // a record cursor
  P2Ctx *v78;   // a record cursor
  int32_t *cur, *v60;   // the row cursors, four bytes a step
  AltP2Block *v120;
  AltP2Block *v129;
  AltP2Block *v111;
  bool v17, v109;
  int16_t v110;
  int32_t i, v5, n4, n4_1, v15, v16, v31, v58, *v59, v101, v103, v104, v112,
          v113, v114, v116, v117, v118, v119, v121, v122, v123, v125, v126,
          v127, v128, v130, v131, v133, v134, v135, n4_3, n4_4;
  P2Freq *freq;
  AltP2Block *lpAddress_1;
  uint32_t v11;
  void *v7, *v8, **lpAddress_2;
  AltP2Block **v18;
  // The p2 filter coefficients drift over a run: rows 4..6 are folded into
  // rows 0..2 here and then zeroed, and the whole table goes back on the way
  // out, so a run starts from the same place the last one did.
  float saved_p2_coef[7][4];
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for ( int32_t k = 0; k < 4; k++ )
  {
    bmf_p2_coef[0][k] += bmf_p2_coef[4][k];
    bmf_p2_coef[1][k] += bmf_p2_coef[5][k];
    bmf_p2_coef[2][k] += bmf_p2_coef[6][k];
    bmf_p2_coef[4][k] = 0;
    bmf_p2_coef[5][k] = 0;
    bmf_p2_coef[6][k] = 0;
  }
  // Rows 4..6 of the rate schedule are flattened for the length of a run,
  // and put back at the end.
  float saved_p2_rate[3][4];
  __builtin_memcpy(saved_p2_rate, bmf_p2_rate[4], sizeof saved_p2_rate);
  for ( int32_t k = 0; k < 4; k++ )
  {
    bmf_p2_rate[4][k] = bmf_p2_rate_reset;
    bmf_p2_rate[5][k] = bmf_p2_rate_reset;
    bmf_p2_rate[6][k] = bmf_p2_rate_reset;
  }
  i = *p_i;
  v5 = p_i[1];
  if ( plane_count > 0 )
  {
    n4 = 0;
    do
    {
      v7 = bmf_page_alloc(0x103E30u);
      if ( v7 )
        v8 = __alt_p2_alloc((AltP2Block *)v7, i, n4);
      else
        v8 = nullptr;
      plane[n4++] = (AltP2Block *)v8;
    }
    while ( n4 < plane_count );
  }
  v163 = plane_desc[plane_desc[2].src_plane + 1].flags & 8;
  ArgList = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  v153 = (plane_desc[plane_desc[4].src_plane + 1].flags) & 8;
  __rc_begin_decode(ArgList);
  if ( v5 > 0 )
  {
    v11 = 0;
    v149 = i;
    v160 = -i;
    n4_1 = plane_count;
    Size = 18 * i + 234;
    v171 = i + 13;
    do
    {
      if ( n4_1 > 0 )
      {
        v165 = v11;
        n4_2 = 0;
        __frame.v150 = v5;
        i_1 = i;
        Src_1 = Src;
        do
        {
          ++n4_2;
          if ( v165 )
          {
            v18 = &plane[n4_2];
            if ( v165 == 1 )
            {
              v25 = (AltP2Block *)((int32_t)*(v18 - 1));
              v26 = (P2Ctx *)v25->cursor[0];
              v26[0] = v26[-1];
              v30 = v25->cursor[0];
              v31 = *(uint32_t *)&v30[-1].lane[2];
              ((P2Ctx *)v30)[1] = ((P2Ctx *)v30)[-1];
              LOWORD(v31) = *(uint16_t *)&v30[-1].sign;
              v34 = v25->cursor[0];
              ((P2Ctx *)v34)[2] = ((P2Ctx *)v34)[-1];
              v38 = v25->cursor[0];
              ((P2Ctx *)v38)[3] = ((P2Ctx *)v38)[-1];
              LOWORD(v34) = *(uint16_t *)&v38[-1].sign;
              v41 = (P2Ctx *)v25->cursor[0];
              v41[4] = v41[-1];
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec2 = v25->cursor[0] + v160;
              rec2[-1] = rec2[0];
              rec2[-2] = rec2[1];
              rec2[-3] = rec2[2];
              rec2[-4] = rec2[3];
              rec2[-5] = rec2[4];
              rec2[-6] = rec2[5];
              rec2[-7] = rec2[6];
              rec2[-8] = rec2[7];
              memcpy(v25->buf[1],v25->buf[0],Size);
              memcpy(v25->buf[2],v25->buf[0],Size);
              memcpy(v25->buf[3],v25->buf[0],Size);
            }
          }
          else
          {
            v14 = (AltP2Block *)((int32_t)plane[n4_2 - 1]);
            v159 = (int32_t)(&plane[n4_2]);
            v15 = 0;
            do
            {
              v16 = v15;
              v14->buf[0][v16].lane[0] = 256;
              v17 = ++v15 < v171;
              v14->buf[0][v16].lane[1] = 256;
              v14->buf[0][v16].lane[2] = -16;
              v14->buf[0][v16].sign = 1;
              v14->buf[0][v16].mag = 3;
              v14->buf[0][v16].lane[3] = 512;
              v14->buf[0][v16].lane[7] = 512;
              v14->buf[0][v16].lane[6] = 512;
              v14->buf[0][v16].lane[5] = 1024;
              v14->buf[0][v16].lane[4] = 256;
            }
            while ( v17 );
            v18 = (AltP2Block **)v159;
            memcpy(v14->buf[1],v14->buf[0],Size);
            memcpy(v14->buf[2],v14->buf[0],Size);
            memcpy(v14->buf[3],v14->buf[0],Size);
            v14->cursor[0] = v14->buf[0] + v149 + 8;
            v14->cursor[1] = (v14->buf[1] + v149 + 8);
            v14->cursor[2] = v14->buf[2] + v149 + 8;
            v14->cursor[3] = v14->buf[3] + v149 + 8;
            v14->cursor[4] = (v14->buf[4] + v149 + 8);
          }
          v56 = (AltP2Block *)((int32_t)*(v18 - 1));
          // Start the next row: carry the last word of this one forward, swap
          // the two row buffers, and re-derive the two cursors from them.
          cur = v56->cur;
          v58 = cur[-1];
          cur[1] = v58;
          *v56->cur = v58;
          v59 = v56->row0;
          v60 = v56->row1;
          v56->row0 = v60;
          v56->row1 = v59;
          v60 += 2;
          v56->cur = v60;
          v59 += 2;
          v56->above = v59;
          v60[-1] = *v59;
          v56->cur[-2] = *v59;
          v56->f278528_q = 0;
          v56->f278536 = 0;
          v56->f278540 = 0;
          v56->f278542 = 0;
          v56->bias[0] = 0.0f;

          v56->bias[1] = 0.0f;

          v56->bias[2] = 0.0f;

          v56->bias[3] = 0.0f;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(v56->p2_row, 0, sizeof v56->p2_row);
          v63 = (P2Ctx *)v56->cursor[0];
          v63[0] = v63[-1];
          v67 = v56->cursor[0];
          ((P2Ctx *)v67)[1] = ((P2Ctx *)v67)[-2];
          v71 = (P2Ctx *)v56->cursor[0];
          v71[2] = v71[-3];
          v75 = v56->cursor[0];
          ((P2Ctx *)v75)[3] = ((P2Ctx *)v75)[-4];
          v78 = (P2Ctx *)v56->cursor[0];
          v78[4] = v78[-5];
          buf3 = v56->buf[3];
          v82 = v56->buf[4];
          buf2 = v56->buf[2];
          buf1 = v56->buf[1];
          v85 = v56->buf[0];
          v56->buf[4] = buf3;
          v56->buf[3] = buf2;
          v56->buf[2] = buf1;
          v56->buf[0] = v82;
          v56->buf[1] = v85;
          v82 += 8;
          v56->cursor[0] = v82;
          v85 += 8;
          v56->cursor[1] = v85;
          v56->cursor[2] = buf1 + 8;
          v56->cursor[3] = buf2 + 8;
          v56->cursor[4] = buf3 + 8;
          ((P2Ctx *)v82)[-1] = ((P2Ctx *)v85)[0];
          v86 = v56->cursor[0];
          ((P2Ctx *)v86)[-2] = ((P2Ctx *)v56->cursor[1])[1];
          v88 = v56->cursor[0];
          ((P2Ctx *)v88)[-3] = ((P2Ctx *)v56->cursor[1])[2];
          v90 = v56->cursor[0];
          ((P2Ctx *)v90)[-4] = ((P2Ctx *)v56->cursor[1])[3];
          v92 = v56->cursor[0];
          ((P2Ctx *)v92)[-5] = ((P2Ctx *)v56->cursor[1])[4];
          v94 = v56->cursor[0];
          ((P2Ctx *)v94)[-6] = ((P2Ctx *)v56->cursor[1])[5];
          v96 = v56->cursor[0];
          ((P2Ctx *)v96)[-7] = ((P2Ctx *)v56->cursor[1])[6];
          v98 = v56->cursor[0];
          ((P2Ctx *)v98)[-8] = ((P2Ctx *)v56->cursor[1])[7];
          v56->cursor[0]->lane[1] = 0;
          n4_1 = plane_count;
        }
        while ( plane_count > n4_2 );
        v11 = v165;
        v5 = __frame.v150;
        i = i_1;
        Src = Src_1;
      }
      ctx_bias[3] = 0;
      ctx_bias[2] = 0;
      ctx_bias[1] = 0;
      ctx_bias[0] = 0;
      if ( i > 0 )
      {
        v165 = v11;
        i_2 = 0;
        v151 = v5;
        i_1 = i;
        do
        {
          ctx_bias[0] >>= 3;
          ctx_bias[1] >>= 3;
          ctx_bias[2] >>= 3;
          ctx_bias[3] >>= 3;
          lpAddress_1 = (AltP2Block *)(plane[0]);
          v101 = __alt_p2_context((AltP2Block *)plane[0], (AltP2Block *)plane[2], (AltP2Block *)plane[1]);
          freq = &lpAddress_1->freq[lpAddress_1->ctx];
          v103 = __alt_p2_decode_symbol(freq, lpAddress_1->ctx_pair);
          v104 = (uint8_t)(v101 + (*(uint8_t *)&lpAddress_1->unfold[v103]));
          __alt_p2_model((AltP2Block *)lpAddress_1, v104, v103, v104 - v101);
          v105 = lpAddress_1->cursor[0];
          ctx_bias[0] += 32 * v105[-1].lane[6];
          ctx_bias[1] += (32 * v105[-1].lane[7]);
          ctx_bias[2] += 32 * v105[-1].lane[4];
          ctx_bias[3] += 32 * v105[-1].lane[5];
          v109 = v163 == 0;
          Src[plane_desc[1].src_plane] = v104;
          if ( v109 )
            v110 = 0;
          else
            v110 = 16 * Src[plane_desc[1].src_plane];
          v111 = (AltP2Block *)(plane[1]);
          plane[1]->cursor[0]->lane[1] = v110;
          v112 = __alt_p2_context((AltP2Block *)v111, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          v113 = __alt_p2_decode_symbol(&v111->freq[v111->ctx], v111->ctx_pair);
          v114 = (uint8_t)(v112 + v111->unfold[v113]);
          __alt_p2_model((AltP2Block *)v111, v114, v113, v114 - v112);
          v115 = v111->cursor[0];
          v116 = v115[-1].lane[7];
          v117 = v115[-1].lane[4];
          ctx_bias[0] += 32 * v115[-1].lane[6];
          v118 = v115[-1].lane[5];
          ctx_bias[1] += 32 * v116;
          ctx_bias[2] += 32 * v117;
          ctx_bias[3] += 32 * v118;
          v109 = ArgList == 0;
          Src[plane_desc[2].src_plane] = v114;
          if ( v109 )
            LOWORD(v119) = 0;
          else
            v119 = (plane_desc[plane_desc[3].src_plane + 1].w8 * Src[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * Src[plane_desc[1].src_plane]) >> 3;
          v120 = (AltP2Block *)(plane[2]);
          plane[2]->cursor[0]->lane[1] = v119;
          v121 = __alt_p2_context((AltP2Block *)v120, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          v122 = __alt_p2_decode_symbol(&v120->freq[v120->ctx], v120->ctx_pair);
          v123 = (uint8_t)(v121 + v120->unfold[v122]);
          __alt_p2_model((AltP2Block *)v120, v123, v122, v123 - v121);
          v124 = v120->cursor[0];
          v125 = v124[-1].lane[7];
          v126 = v124[-1].lane[4];
          ctx_bias[0] += 32 * v124[-1].lane[6];
          v127 = v124[-1].lane[5];
          ctx_bias[1] += 32 * v125;
          ctx_bias[2] += 32 * v126;
          ctx_bias[3] += 32 * v127;
          Src[plane_desc[3].src_plane] = v123;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            if ( v153 )
              v128 = (plane_desc[plane_desc[4].src_plane + 1].w12 * Src[2]
                    + plane_desc[plane_desc[4].src_plane + 1].w8 * Src[1]
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *Src) >> 3;
            else
              LOWORD(v128) = 0;
            v129 = (AltP2Block *)(plane[3]);
            plane[3]->cursor[0]->lane[1] = v128;
            v130 = __alt_p2_context((AltP2Block *)v129, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            v131 = __alt_p2_decode_symbol(&v129->freq[v129->ctx], v129->ctx_pair);
            v154 = (uint8_t)(v130 + v129->unfold[v131]);
            __alt_p2_model((AltP2Block *)v129, v154, v131, v154 - v130);
            v132 = v129->cursor[0];
            v133 = v132[-1].lane[7];
            v134 = v132[-1].lane[4];
            v135 = v132[-1].lane[5];
            ctx_bias[0] += 32 * v132[-1].lane[6];
            ctx_bias[1] += 32 * v133;
            ctx_bias[2] += 32 * v134;
            ctx_bias[3] += 32 * v135;
            Src[3] = v154;
            n4_1 = plane_count;
          }
          Src += n4_1;
          ++i_2;
        }
        while ( i_2 < i_1 );
        v11 = v165;
        v5 = v151;
        i = i_1;
      }
      ++v11;
    }
    while ( v11 < v5 );
  }
  __rc_end_decode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  n4_3 = plane_count;
  if ( plane_count > 0 )
  {
    n4_4 = 0;
    do
    {
      lpAddress_2 = (void **)plane[n4_4];
      if ( lpAddress_2 )
      {
        __alt_p2_free((void **)lpAddress_2, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}

void __unmodel_plane(int8_t ArgList, uint16_t *p_i, uint8_t *Src)
{
  ;
  ModelBlock *v6;
  void *v5;
  if ( plane_alt_model )
  {
    if ( plane_predictor == 1 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __alt_model_p1_d8_decode(ArgList, Src, *p_i, p_i[1]);
      else
        __alt_model_p1_decode(p_i, Src);
    }
    else if ( plane_predictor == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __alt_model_p2_d8_decode(Src, *p_i, p_i[1]);
      else
        __alt_model_p2_decode(p_i, Src);
    }
  }
  else
  {
    v5 = bmf_new(0x7BA230u);
    if ( v5 )
      v6 = __layout_workspace((ModelBlock *)v5, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F);
    else
      v6 = (ModelBlock *)(nullptr);
    __unmodel_plane_slow((ModelBlock *)v6, Src);
    if ( v6 )
      __free_workspace((ModelBlock *)v6, 1);
  }
}

void __alt_p2_d8_encode_body(AltP2Block *lpAddress, uint8_t *a4, int32_t i, int32_t a6, uint8_t *a7)
{
  int32_t v20;   // the symbol MSVC kept in the low half of `v16`
  P2Ctx *v11, *v16, *v23, *v30, *v61, *v69, *buf3, *v76, *buf2, *buf1, *v79, *v80, *v82, *v84, *v86, *v88, *v90, *v92;
  int32_t j;
  // These shared `__frame.j` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t v102;
  uint8_t *v103;
  int32_t v104;
  // These shared `__frame.v104` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *v105;
  uint32_t *v106;   // the block's `ctx_pair`
  uint8_t v107;
  AltP2Block *lpAddress_1;
  uint8_t *v109;
  uint8_t v110;
  ;
  P2Ctx *v34;   // a record cursor
  P2Ctx *v57;   // a record cursor
  P2Ctx *v65;   // a record cursor
  P2Ctx *v72;   // a record cursor
  P2Ctx *v27;   // a record cursor
  int32_t *cur, *v54;   // the row cursors, four bytes a step
  P2Ctx *v93;   // a record cursor
  P2Ctx *v91;   // a record cursor
  P2Ctx *v89;   // a record cursor
  P2Ctx *v87;   // a record cursor
  P2Ctx *v85;   // a record cursor
  P2Ctx *v83;   // a record cursor
  P2Ctx *v81;   // a record cursor
  uint8_t v14, v97;
  int32_t v12, v13, n16, v17, v31, Size, v52, *v53, v95, v96, n16_1, v99,
          v100;
  int64_t v19;
  P2Ctx *v18;
  uint32_t k;
  uint8_t *v8;
  v8 = a4;
  __rc_begin_encode();
  v11 = lpAddress->cursor[0];
  if ( i > 0 )
  {
    v103 = a4;
    for ( j = 0; j < i; ++j )
    {
      v12 = (uint16_t)v11[-1].lane[0] >> 4;
      v13 = (uint8_t)(*v103 - v12);
      v104 = lpAddress->fold[v13];
      v14 = lpAddress->unfold[v104] + v12;
      n16 = (uint8_t)*a7 - (uint8_t)(v14 + *a7 - *v103);
      if ( n16 < -16 || n16 > 16 )
      {
        *a7 = *v103;
        v104 = lpAddress->fold_hi[v13];
      }
      else
      {
        *a7 = v14;
      }
      v16 = lpAddress->cursor[0];
      v17 = v16[-1].lane[0] >> 4;
      lpAddress->ctx_pair[0] = lpAddress->ctx + lpAddress->ctx_delta[v17 + 4];
      lpAddress->ctx_pair[1] = lpAddress->ctx + lpAddress->ctx_delta[v17];
      // The composed index, the encoder's own copy of `alt_p2_context`'s sum:
      // four selected weights, one constant, and the running context.
      __alt_p2_encode_symbol(
          &lpAddress->freq[lpAddress->ctx
                         + lpAddress->ctx_w[3].w[(v16[-1].lane[0] <= v16[-2].lane[0])
                                               + (v16[-1].lane[0] < v16[-2].lane[0])]
                         + lpAddress->ctx_w[2].w[v16[-2].sign]
                         + lpAddress->ctx_w[1].w[v16[-1].sign]
                         + lpAddress->ctx_w[0].w[((uint32_t)(v17 - 115) >> 31)
                                               + ((uint32_t)(v17 - 17) >> 31)]
                         + lpAddress->ctx_w[4].w[1]],
          lpAddress->ctx_pair, v104);
      ++v103;
      v20 = 16 * (uint8_t)*a7;
      lpAddress->cursor[0]->lane[0] = v20;
      lpAddress->cursor[0]->lane[1] = v20;
      ++a7;
      v18 = lpAddress->cursor[0];
      v19 = (int16_t)(v18->lane[0] - v18[-1].lane[0]);
      v18->lane[2] = v19;
      LOWORD(v19) = (WORD2(v19) ^ v19) - WORD2(v19);
      lpAddress->cursor[0]->lane[3] = v19;
      lpAddress->cursor[0]->lane[7] = v19;
      lpAddress->cursor[0]->lane[6] = v19;
      lpAddress->cursor[0]->lane[5] = v19;
      lpAddress->cursor[0]->lane[4] = (uint32_t)lpAddress->cursor[0]->lane[5] >> 1;
      lpAddress->cursor[0]->mag = 2;
      lpAddress->cursor[0]->sign = (lpAddress->cursor[0]->lane[2] <= 0)
                                                    + (lpAddress->cursor[0]->lane[2] < 0);
      v11 = (lpAddress->cursor[0] + 1);
      lpAddress->cursor[0] = v11;
    }
    v8 = v103;
  }
  ((P2Ctx *)v11)[0] = ((P2Ctx *)v11)[-1];
  v23 = lpAddress->cursor[0];
  ((P2Ctx *)v23)[1] = ((P2Ctx *)v23)[-1];
  v27 = lpAddress->cursor[0];
  v27[2] = v27[-1];
  v30 = lpAddress->cursor[0];
  v31 = *(uint32_t *)&v30[-1].lane[2];
  ((P2Ctx *)v30)[3] = ((P2Ctx *)v30)[-1];
  LOWORD(v31) = *(uint16_t *)&v30[-1].sign;
  v34 = lpAddress->cursor[0];
  v34[4] = v34[-1];
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec3 = lpAddress->cursor[0] + (-i);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  Size = 18 * i + 234;
  memcpy(lpAddress->buf[1],lpAddress->buf[0],Size);
  memcpy(lpAddress->buf[2],lpAddress->buf[0],Size);
  memcpy(lpAddress->buf[3],lpAddress->buf[0],Size);
  if ( a6 > 1 )
  {
    v106 = lpAddress->ctx_pair;
    v105 = a7;
    v102 = 0;
    do
    {
      // Start the next row: carry the last word of this one forward, swap
      // the two row buffers, and re-derive the two cursors from them.
      cur = lpAddress->cur;
      v52 = cur[-1];
      v109 = v8;
      cur[1] = v52;
      *lpAddress->cur = v52;
      v53 = lpAddress->row0;
      v54 = lpAddress->row1;
      lpAddress->row0 = v54;
      lpAddress->row1 = v53;
      v54 += 2;
      lpAddress->cur = v54;
      v53 += 2;
      lpAddress->above = v53;
      v54[-1] = *v53;
      lpAddress->cur[-2] = *v53;
      lpAddress->f278528_q = 0;
      lpAddress->f278536 = 0;
      lpAddress->f278540 = 0;
      lpAddress->f278542 = 0;
      lpAddress->bias[0] = 0.0f;

      lpAddress->bias[1] = 0.0f;

      lpAddress->bias[2] = 0.0f;

      lpAddress->bias[3] = 0.0f;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(lpAddress->p2_row, 0, sizeof lpAddress->p2_row);
      v57 = lpAddress->cursor[0];
      v57[0] = v57[-1];
      v61 = lpAddress->cursor[0];
      ((P2Ctx *)v61)[1] = ((P2Ctx *)v61)[-2];
      v65 = lpAddress->cursor[0];
      v65[2] = v65[-3];
      v69 = lpAddress->cursor[0];
      ((P2Ctx *)v69)[3] = ((P2Ctx *)v69)[-4];
      v72 = lpAddress->cursor[0];
      v72[4] = v72[-5];
      buf3 = lpAddress->buf[3];
      v76 = lpAddress->buf[4];
      buf2 = lpAddress->buf[2];
      buf1 = lpAddress->buf[1];
      v79 = lpAddress->buf[0];
      lpAddress->buf[4] = buf3;
      lpAddress->buf[3] = buf2;
      lpAddress->buf[2] = buf1;
      lpAddress->buf[0] = v76;
      lpAddress->buf[1] = v79;
      v76 += 8;
      lpAddress->cursor[0] = v76;
      v79 += 8;
      lpAddress->cursor[1] = v79;
      lpAddress->cursor[2] = buf1 + 8;
      lpAddress->cursor[3] = buf2 + 8;
      lpAddress->cursor[4] = buf3 + 8;
      ((P2Ctx *)v76)[-1] = ((P2Ctx *)v79)[0];
      v80 = lpAddress->cursor[0];
      v81 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v80)[-2] = v81[1];
      v82 = lpAddress->cursor[0];
      v83 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v82)[-3] = v83[2];
      v84 = lpAddress->cursor[0];
      v85 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v84)[-4] = v85[3];
      v86 = lpAddress->cursor[0];
      v87 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v86)[-5] = v87[4];
      v88 = lpAddress->cursor[0];
      v89 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v88)[-6] = v89[5];
      v90 = lpAddress->cursor[0];
      v91 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v90)[-7] = v91[6];
      v92 = lpAddress->cursor[0];
      v93 = (P2Ctx *)lpAddress->cursor[1];
      ((P2Ctx *)v92)[-8] = v93[7];
      lpAddress->cursor[0]->lane[1] = 0;
      if ( i > 0 )
      {
        lpAddress_1 = (AltP2Block *)(lpAddress);
        for ( k = 0; k < i; ++k )
        {
          v95 = __alt_p2_context((AltP2Block *)lpAddress_1, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          v107 = v109[k];
          v96 = (uint8_t)(v107 - v95);
          v97 = v95 + lpAddress_1->unfold[lpAddress_1->fold[v96]];
          n16_1 = (uint8_t)v105[k] - (uint8_t)(v97 + v105[k] - v107);
          v99 = lpAddress_1->fold[v96];
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v105[k] = v107;
            v99 = lpAddress_1->fold_hi[(uint8_t)(v107 - v95)];
          }
          else
          {
            v105[k] = v97;
          }
          v110 = v99;
          __alt_p2_encode_symbol(&lpAddress_1->freq[lpAddress_1->ctx], v106, v99);
          __alt_p2_model((AltP2Block *)lpAddress_1, (uint8_t)v105[k], v110, (uint8_t)v105[k] - v95);
          v8 = &v109[k + 1];
          v100 = (int32_t)&v105[k + 1];
        }
        lpAddress = (AltP2Block *)(lpAddress_1);
        v105 = (uint8_t *)v100;
      }
      ++v102;
    }
    while ( v102 < a6 - 1 );
  }
  __rc_end_encode();
}

void __alt_model_p2_d8_encode( uint8_t *a3, int32_t i, int32_t a5, uint8_t *a6)
{
  ;
  void * v6;
  AltP2Block *lpAddress;
  v6 = bmf_page_alloc(0x103E30u);
  if ( v6 )
    lpAddress = (AltP2Block *)__alt_p2_alloc((AltP2Block *)v6, i, 0);
  else
    lpAddress = (AltP2Block *)(nullptr);
  __alt_p2_d8_encode_body((AltP2Block *)lpAddress, a3, i, a5, a6);
  if ( lpAddress )
    __alt_p2_free((void **)lpAddress, 1);
}

int32_t __alt_model_p2_encode(BmfImage *p_i, uint8_t *a2) {   P2Ctx *v109,
        *v120, *v131, *v139, *v29, *v33, *v37, *v66, *v74, *buf3, *v81, *buf2,
        *buf1, *v84, *v85, *v87, *v89, *v91, *v93, *v95, *v97;
  int32_t v155;
  int32_t n3;
  int32_t v157;
  int32_t v158;
  AltP2Block * plane[4];
  int32_t v163;
  int32_t v164;
  uint32_t Size;
  uint8_t *v166;
  int32_t v168;
  int32_t v169;
  uint32_t v170;
  int32_t n4_2;
  uint32_t i;
  int32_t v173;
  int32_t v174;
  int32_t v175;
  int32_t v176;
  int32_t v177;
  int32_t v178;
  int32_t v179;
  int32_t v180;
  int32_t v181;
  int32_t v182;
  int32_t v183;
  int32_t v184;
  uint32_t v185;
  ;
  AltP2Block *v55;
  AltP2Block *v13;
  AltP2Block *v24;
  P2Ctx *v25;   // a record cursor
  P2Ctx *v40;   // a record cursor
  P2Ctx *v62;   // a record cursor
  P2Ctx *v70;   // a record cursor
  P2Ctx *v77;   // a record cursor
  int32_t *cur, *v59;   // the row cursors, four bytes a step
  AltP2Block *v125;
  AltP2Block *v133;
  AltP2Block *v114;
  bool v16;
  int16_t v113;
  int32_t i_1, height, n4, n4_1, v14, v15, v30, v99, v57, *v58, v100, v101, v102,
          v104, v105, v106, v107, n16, v110, v111, v112, v115, v116, v117,
          v118, n16_1, v121, v122, v123, v124, v126, v127, v128, v129, n16_2,
          v132, v134, v135, v136, v137, n16_3, n4_3, n4_4;
  uint32_t v11;
  AltP2Block *lpAddress_1;
  void *v7, *v8, **lpAddress_2;
  AltP2Block **v17;
  v166 = a2;
  // The p2 filter coefficients drift over a run: rows 4..6 are folded into
  // rows 0..2 here and then zeroed, and the whole table goes back on the way
  // out, so a run starts from the same place the last one did.
  float saved_p2_coef[7][4];
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for ( int32_t k = 0; k < 4; k++ )
  {
    bmf_p2_coef[0][k] += bmf_p2_coef[4][k];
    bmf_p2_coef[1][k] += bmf_p2_coef[5][k];
    bmf_p2_coef[2][k] += bmf_p2_coef[6][k];
    bmf_p2_coef[4][k] = 0;
    bmf_p2_coef[5][k] = 0;
    bmf_p2_coef[6][k] = 0;
  }
  // Rows 4..6 of the rate schedule are flattened for the length of a run,
  // and put back at the end.
  float saved_p2_rate[3][4];
  __builtin_memcpy(saved_p2_rate, bmf_p2_rate[4], sizeof saved_p2_rate);
  for ( int32_t k = 0; k < 4; k++ )
  {
    bmf_p2_rate[4][k] = bmf_p2_rate_reset;
    bmf_p2_rate[5][k] = bmf_p2_rate_reset;
    bmf_p2_rate[6][k] = bmf_p2_rate_reset;
  }
  i_1 = p_i->width;
  height = p_i->height;
  if ( plane_count > 0 )
  {
    n4 = 0;
    do
    {
      v7 = bmf_page_alloc(0x103E30u);
      if ( v7 )
        v8 = (void *)__alt_p2_alloc((AltP2Block *)v7, i_1, n4);
      else
        v8 = nullptr;
      plane[n4++] = (AltP2Block *)v8;
    }
    while ( n4 < plane_count );
  }
  v169 = plane_desc[plane_desc[2].src_plane + 1].flags & 8;
  v168 = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  v155 = (plane_desc[plane_desc[4].src_plane + 1].flags) & 8;
  __rc_begin_encode();
  if ( height > 0 )
  {
    v11 = 0;
    v164 = -i_1;
    Size = 18 * i_1 + 234;
    v185 = i_1 + 13;
    n4_1 = plane_count;
    do
    {
      if ( n4_1 > 0 )
      {
        v170 = v11;
        n4_2 = 0;
        do
        {
          ++n4_2;
          if ( v170 )
          {
            v17 = &plane[n4_2];
            if ( v170 == 1 )
            {
              v24 = (AltP2Block *)((int32_t)*(v17 - 1));
              v25 = (P2Ctx *)v24->cursor[0];
              v25[0] = v25[-1];
              v29 = v24->cursor[0];
              v30 = *(uint32_t *)&v29[-1].lane[2];
              ((P2Ctx *)v29)[1] = ((P2Ctx *)v29)[-1];
              LOWORD(v30) = *(uint16_t *)&v29[-1].sign;
              v33 = v24->cursor[0];
              ((P2Ctx *)v33)[2] = ((P2Ctx *)v33)[-1];
              v37 = v24->cursor[0];
              ((P2Ctx *)v37)[3] = ((P2Ctx *)v37)[-1];
              LOWORD(v33) = *(uint16_t *)&v37[-1].sign;
              v40 = (P2Ctx *)v24->cursor[0];
              v40[4] = v40[-1];
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec4 = v24->cursor[0] + v164;
              rec4[-1] = rec4[0];
              rec4[-2] = rec4[1];
              rec4[-3] = rec4[2];
              rec4[-4] = rec4[3];
              rec4[-5] = rec4[4];
              rec4[-6] = rec4[5];
              rec4[-7] = rec4[6];
              rec4[-8] = rec4[7];
              memcpy(v24->buf[1],v24->buf[0],Size);
              memcpy(v24->buf[2],v24->buf[0],Size);
              memcpy(v24->buf[3],v24->buf[0],Size);
            }
          }
          else
          {
            v13 = (AltP2Block *)((int32_t)plane[n4_2 - 1]);
            v163 = (int32_t)(&plane[n4_2]);
            v14 = 0;
            do
            {
              v15 = v14;
              v13->buf[0][v15].lane[0] = 256;
              v16 = ++v14 < v185;
              v13->buf[0][v15].lane[1] = 256;
              v13->buf[0][v15].lane[2] = -16;
              v13->buf[0][v15].sign = 1;
              v13->buf[0][v15].mag = 3;
              v13->buf[0][v15].lane[3] = 512;
              v13->buf[0][v15].lane[7] = 512;
              v13->buf[0][v15].lane[6] = 512;
              v13->buf[0][v15].lane[5] = 1024;
              v13->buf[0][v15].lane[4] = 256;
            }
            while ( v16 );
            v17 = (AltP2Block **)v163;
            memcpy(v13->buf[1],v13->buf[0],Size);
            memcpy(v13->buf[2],v13->buf[0],Size);
            memcpy(v13->buf[3],v13->buf[0],Size);
            v13->cursor[0] = v13->buf[0] + i_1 + 8;
            v13->cursor[1] = (v13->buf[1] + i_1 + 8);
            v13->cursor[2] = v13->buf[2] + i_1 + 8;
            v13->cursor[3] = v13->buf[3] + i_1 + 8;
            v13->cursor[4] = (v13->buf[4] + i_1 + 8);
          }
          v55 = (AltP2Block *)((int32_t)*(v17 - 1));
          // Start the next row: carry the last word of this one forward, swap
          // the two row buffers, and re-derive the two cursors from them.
          cur = v55->cur;
          v57 = cur[-1];
          cur[1] = v57;
          *v55->cur = v57;
          v58 = v55->row0;
          v59 = v55->row1;
          v55->row0 = v59;
          v55->row1 = v58;
          v59 += 2;
          v55->cur = v59;
          v58 += 2;
          v55->above = v58;
          v59[-1] = *v58;
          v55->cur[-2] = *v58;
          v55->f278528_q = 0;
          v55->f278536 = 0;
          v55->f278540 = 0;
          v55->f278542 = 0;
          v55->bias[0] = 0.0f;

          v55->bias[1] = 0.0f;

          v55->bias[2] = 0.0f;

          v55->bias[3] = 0.0f;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(v55->p2_row, 0, sizeof v55->p2_row);
          v62 = (P2Ctx *)v55->cursor[0];
          v62[0] = v62[-1];
          v66 = v55->cursor[0];
          ((P2Ctx *)v66)[1] = ((P2Ctx *)v66)[-2];
          v70 = (P2Ctx *)v55->cursor[0];
          v70[2] = v70[-3];
          v74 = v55->cursor[0];
          ((P2Ctx *)v74)[3] = ((P2Ctx *)v74)[-4];
          v77 = (P2Ctx *)v55->cursor[0];
          v77[4] = v77[-5];
          buf3 = v55->buf[3];
          v81 = v55->buf[4];
          buf2 = v55->buf[2];
          buf1 = v55->buf[1];
          v84 = v55->buf[0];
          v55->buf[4] = buf3;
          v55->buf[3] = buf2;
          v55->buf[2] = buf1;
          v55->buf[0] = v81;
          v55->buf[1] = v84;
          v81 += 8;
          v55->cursor[0] = v81;
          v84 += 8;
          v55->cursor[1] = v84;
          v55->cursor[2] = buf1 + 8;
          v55->cursor[3] = buf2 + 8;
          v55->cursor[4] = buf3 + 8;
          ((P2Ctx *)v81)[-1] = ((P2Ctx *)v84)[0];
          v85 = v55->cursor[0];
          ((P2Ctx *)v85)[-2] = ((P2Ctx *)v55->cursor[1])[1];
          v87 = v55->cursor[0];
          ((P2Ctx *)v87)[-3] = ((P2Ctx *)v55->cursor[1])[2];
          v89 = v55->cursor[0];
          ((P2Ctx *)v89)[-4] = ((P2Ctx *)v55->cursor[1])[3];
          v91 = v55->cursor[0];
          ((P2Ctx *)v91)[-5] = ((P2Ctx *)v55->cursor[1])[4];
          v93 = v55->cursor[0];
          ((P2Ctx *)v93)[-6] = ((P2Ctx *)v55->cursor[1])[5];
          v95 = v55->cursor[0];
          ((P2Ctx *)v95)[-7] = ((P2Ctx *)v55->cursor[1])[6];
          v97 = v55->cursor[0];
          ((P2Ctx *)v97)[-8] = ((P2Ctx *)v55->cursor[1])[7];
          v55->cursor[0]->lane[1] = 0;
          n4_1 = plane_count;
        }
        while ( plane_count > n4_2 );
        v11 = v170;
      }
      if ( i_1 <= 0 )
      {
        ctx_bias[0] = 0;
        ctx_bias[1] = 0;
        ctx_bias[2] = 0;
        ctx_bias[3] = 0;
      }
      else
      {
        v170 = v11;
        v99 = 0;
        v100 = 0;
        v101 = 0;
        v102 = 0;
        for ( i = 0; i < i_1; ++i )
        {
          v184 = plane_desc[1].src_plane;
          ctx_bias[0] = v102 >> 3;
          ctx_bias[1] = v101 >> 3;
          ctx_bias[2] = v100 >> 3;
          ctx_bias[3] = v99 >> 3;
          lpAddress_1 = (AltP2Block *)(plane[0]);
          v104 = v166[plane_desc[1].src_plane];
          v178 = __alt_p2_context((AltP2Block *)plane[0], (AltP2Block *)plane[2], (AltP2Block *)plane[1]);
          v181 = (uint8_t)(v104 - v178);
          v105 = (uint8_t)lpAddress_1->fold[v181];
          v106 = v166[v184];
          v173 = (uint8_t)(v178 + (*(uint8_t *)&lpAddress_1->unfold[v105]));
          v107 = (uint8_t)(v173 + v106 - v104);
          n16 = v106 - v107;
          if ( n16 < -16 || n16 > 16 )
          {
            v105 = (uint8_t)lpAddress_1->fold_hi[v181];
          }
          else
          {
            v104 = v173;
            v166[v184] = v107;
          }
          __alt_p2_encode_symbol(&lpAddress_1->freq[lpAddress_1->ctx], lpAddress_1->ctx_pair, v105);
          __alt_p2_model((AltP2Block *)lpAddress_1, v104, v105, v104 - v178);
          v109 = lpAddress_1->cursor[0];
          v110 = v109[-1].lane[7];
          v111 = v109[-1].lane[4];
          v112 = v109[-1].lane[5];
          ctx_bias[0] += 32 * v109[-1].lane[6];
          ctx_bias[1] += 32 * v110;
          ctx_bias[2] += 32 * v111;
          ctx_bias[3] += 32 * v112;
          if ( v169 )
            v113 = 16 * v166[plane_desc[1].src_plane];
          else
            v113 = 0;
          v114 = (AltP2Block *)(plane[1]);
          plane[1]->cursor[0]->lane[1] = v113;
          v176 = plane_desc[2].src_plane;
          v115 = v166[plane_desc[2].src_plane];
          v179 = __alt_p2_context((AltP2Block *)v114, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          v183 = (uint8_t)(v115 - v179);
          v116 = v114->fold[v183];
          v117 = v166[v176];
          v175 = (uint8_t)(v179 + v114->unfold[v116]);
          v118 = (uint8_t)(v175 + v117 - v115);
          n16_1 = v117 - v118;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v116 = v114->fold_hi[v183];
          }
          else
          {
            v115 = v175;
            v166[v176] = v118;
          }
          __alt_p2_encode_symbol(&v114->freq[v114->ctx], v114->ctx_pair, v116);
          __alt_p2_model((AltP2Block *)v114, v115, v116, v115 - v179);
          v120 = v114->cursor[0];
          v121 = v120[-1].lane[7];
          v122 = v120[-1].lane[4];
          v123 = v120[-1].lane[5];
          ctx_bias[0] += 32 * v120[-1].lane[6];
          ctx_bias[1] += 32 * v121;
          ctx_bias[2] += 32 * v122;
          ctx_bias[3] += 32 * v123;
          if ( v168 )
            v124 = (plane_desc[plane_desc[3].src_plane + 1].w8 * v166[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * v166[plane_desc[1].src_plane]) >> 3;
          else
            LOWORD(v124) = 0;
          v125 = (AltP2Block *)(plane[2]);
          plane[2]->cursor[0]->lane[1] = v124;
          v177 = plane_desc[3].src_plane;
          v126 = v166[plane_desc[3].src_plane];
          v180 = __alt_p2_context((AltP2Block *)v125, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          v182 = (uint8_t)(v126 - v180);
          v127 = v125->fold[v182];
          v128 = v166[v177];
          v174 = (uint8_t)(v180 + v125->unfold[v127]);
          v129 = (uint8_t)(v174 + v128 - v126);
          n16_2 = v128 - v129;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            v127 = v125->fold_hi[v182];
          }
          else
          {
            v126 = v174;
            v166[v177] = v129;
          }
          __alt_p2_encode_symbol(&v125->freq[v125->ctx], v125->ctx_pair, v127);
          __alt_p2_model((AltP2Block *)v125, v126, v127, v126 - v180);
          v131 = v125->cursor[0];
          n4_1 = plane_count;
          v102 = ctx_bias[0] + 32 * v131[-1].lane[6];
          v101 = ctx_bias[1] + 32 * v131[-1].lane[7];
          v100 = ctx_bias[2] + 32 * v131[-1].lane[4];
          v99 = ctx_bias[3] + 32 * v131[-1].lane[5];
          if ( plane_count >= 4 )
          {
            ctx_bias[0] += 32 * v131[-1].lane[6];
            ctx_bias[1] = v101;
            ctx_bias[2] = v100;
            ctx_bias[3] = v99;
            if ( v155 )
              v132 = (plane_desc[plane_desc[4].src_plane + 1].w12 * v166[2]
                    + plane_desc[plane_desc[4].src_plane + 1].w8 * v166[1]
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *v166) >> 3;
            else
              LOWORD(v132) = 0;
            v133 = (AltP2Block *)(plane[3]);
            plane[3]->cursor[0]->lane[1] = v132;
            n3 = plane_desc[4].src_plane;
            v134 = v166[plane_desc[4].src_plane];
            v157 = __alt_p2_context((AltP2Block *)v133, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            v158 = (uint8_t)(v134 - v157);
            v135 = v133->fold[v158];
            v136 = v166[n3];
            v137 = (uint8_t)(v157 + v133->unfold[v135] + v136 - v134);
            n16_3 = v136 - v137;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              v135 = v133->fold_hi[v158];
            }
            else
            {
              v134 = (uint8_t)(v157 + v133->unfold[v135]);
              v166[n3] = v137;
            }
            __alt_p2_encode_symbol(&v133->freq[v133->ctx], v133->ctx_pair, v135);
            __alt_p2_model((AltP2Block *)v133, v134, v135, v134 - v157);
            v139 = v133->cursor[0];
            n4_1 = plane_count;
            v102 = ctx_bias[0] + 32 * v139[-1].lane[6];
            v101 = ctx_bias[1] + 32 * v139[-1].lane[7];
            v100 = ctx_bias[2] + 32 * v139[-1].lane[4];
            v99 = ctx_bias[3] + 32 * v139[-1].lane[5];
          }
          v166 += n4_1;
        }
        v11 = v170;
        ctx_bias[0] = v102;
        ctx_bias[1] = v101;
        ctx_bias[2] = v100;
        ctx_bias[3] = v99;
      }
      ++v11;
    }
    while ( v11 < height );
  }
  __rc_end_encode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  n4_3 = plane_count;
  if ( plane_count > 0 )
  {
    n4_4 = 0;
    do
    {
      lpAddress_2 = (void **)plane[n4_4];
      if ( lpAddress_2 )
      {
        __alt_p2_free((void **)lpAddress_2, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}

void __model_plane( BmfImage *p_i, uint8_t *a4, uint8_t *a5)
{
  int32_t Size;
  int32_t v58;
  // These shared `__frame.v58` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint16_t *v59;   // a row cursor into sym_word
  ModelBlock *Blocka_5;
  // These shared `__frame.Blocka_5` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v61;
  // These shared `__frame.Blocka_2` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  ModelBlock *Blocka_2;
  int32_t v64;
  int32_t v65;
  int32_t v66;
  int32_t v67;
  int32_t v68;
  int32_t v69;
  int32_t v70;
  int32_t v72;
  int32_t v73;
  uint32_t n5;
  ;
  ModelBlock *Blocka_1;
  PixRec *v46, *v50;   // row cursors out of row_cur
  int8_t v7;
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  int16_t __model_plane_n2, v22;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_4;
  int32_t v8, v10, v11, v14, n2_1, n2_2, v17, v18, v19, v20, v21, v44, v45,
          v53, v54, v55, v56;
  PixRec *row_cur3, *row_cur2, *row_cur1;   // three of the five row buffers
  uint32_t n0x10000, v31, v34, v37, v40;
  SymListBlock *v32, *v38;
  SymPair *group_ctr;   // one group's row of counter pairs
  SymList *v33, *v39;
  FreqRec *v12;   // a bucket record: `grid[bucket]`
  PixRec *v51, *v52;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  void *v5;
  if ( plane_alt_model )
  {
    if ( ::plane_predictor == 1 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p1_d8_encode(a4, p_i->width, p_i->height, a5);
      else
        __alt_model_p1_encode((uint16_t *)p_i, a4);
    }
    else if ( ::plane_predictor == 2 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p2_d8_encode(a4, p_i->width, p_i->height, a5);
      else
        __alt_model_p2_encode((BmfImage *)p_i, a4);
    }
  }
  else
  {
    v5 = bmf_new(0x7BA230u);
    if ( v5 )
      Blocka_3 = __layout_workspace((ModelBlock *)v5, p_i->height, p_i->width, p_i->height, p_i->depth & 0x3F);
    else
      Blocka_3 = (ModelBlock *)(0);
    __rc_begin_encode();
    __reduce_alphabet(Blocka_3, v7, a4);
    v64 = 0;
    v8 = 0;
    Blocka_2 = (ModelBlock *)((uint32_t *)Blocka_3);
    Blocka_4 = (ModelBlock *)(Blocka_3);
    do
    {
      v10 = ctx_group_flags[v8];
      Blocka_2->ctx_state[v10] = v8;
      v11 = 0;
      v58 = v8;
      v69 = v10 & 4;
      v68 = v10 & 2;
      v67 = v10 & 0x10;
      v66 = v10 & 1;
      v65 = v10 & 8;
      v70 = v10 & 0x20;
      do
      {
        v72 = v11;
        n5 = 0;
        Blocka_5 = (ModelBlock *)(Blocka_4);
        do
        {
          Blocka_2->ctx_bucket[v8 + 15 * v11 + 75 * n5] = v64;
          v73 = Blocka_2->alphabet;
          // A sixteen-byte record per bucket, at `+96 + 16 * bucket`: five
          // counts at words 0..4, their total at 5, a scaled weight at 6, and
          // two bytes at 7 -- a level (`<= f16`) and the weight `1 << (5 -
          // level)` it derives.  The base is the object and the counter starts
          // at zero, so record 0 is +96 .. +111 -- which `row_cur[10..13]` also
          // claims, and which the `f1051664[k] = row_cur[10 + k]` copy after this
          // loop reads back.  That copy is dead -- nothing reads `f1051664` --
          // so the collision costs nothing at run time, but it does mean one
          // of `row_cur`'s length and `f1051664`'s type is wrong.
          //
          // The bucket counter reaches exactly 188: a `__builtin_trap()` on
          // `>= 188` fires on fourteen of the gate's streams and one on
          // `>= 189` fires on none.  So this table is 189 records, +96 ..
          // +3119.  `FreqRec` is on the same grid from record 188 (+3104),
          // which makes the last bucket record and the first frequency record
          // the same sixteen bytes -- the two tables abut and share one.
          v12 = &Blocka_2->grid[v64];
          __model_plane_n2 = 2;
          v12->w[0] = 2;
          LOWORD(v14) = 2;
          n2_1 = 2;
          n2_2 = 2;
          if ( v69 )
          {
            n2_1 = 4;
            n2_2 = 0;
          }
          if ( v68 )
          {
            LOWORD(v14) = n2_2 + 2;
            n2_2 = 0;
          }
          if ( v67 )
          {
            LOWORD(v14) = n2_1 + v14;
            n2_1 = 0;
          }
          if ( v66 )
          {
            __model_plane_n2 = n2_2 + 2;
            n2_2 = 0;
            v12->w[4] = 0;
          }
          else
          {
            v12->w[4] = n2_2;
          }
          if ( v65 )
          {
            __model_plane_n2 += n2_1;
            n2_1 = 0;
            v12->w[3] = 0;
          }
          else
          {
            v12->w[3] = n2_1;
          }
          if ( v70 )
          {
            v12->w[1] = v14 + __model_plane_n2;
            LOWORD(v14) = 0;
            v12->w[2] = 0;
          }
          else
          {
            v12->w[2] = v14;
            v12->w[1] = __model_plane_n2;
          }
          v17 = n2_1 != 0;
          v18 = n2_2 != 0;
          v14 = (uint16_t)v14;
          if ( (uint16_t)v14 )
            v14 = 1;
          v19 = v18 + v17 + v14 + 2;
          if ( v19 <= v73 )
          {
            v12->b14 = v19;
          }
          else
          {
            v12->b14 = v18 + v17 + v14 + 1;
            v12->w[0] = 0;
          }
          if ( v12->w[v72]
            && v12->w[n5]
            && (v20 = v12->b14, v20 <= v73) )
          {
            v21 = 1;
            v22 = (uint8_t)(1 << ((5 - v20) & 31));
            v12->b15 = v22;
            v12->w[6] = v22 << 6;
            v12->w[v72] += v22;
            v12->w[n5] += v12->b15;
            v12->w[5] = v12->w[0]
                                 + v12->w[4]
                                 + v12->w[3]
                                 + v12->w[2]
                                 + v12->w[1];
          }
          else
          {
            v21 = 0;
          }
          v64 += v21;
          ++n5;
        }
        while ( n5 < 5 );
        Blocka_4 = (ModelBlock *)(Blocka_5);
        v11 = v72 + 1;
      }
      while ( (uint32_t)(v72 + 1) < 5 );
      n0x10000 = 0;
      group_ctr = Blocka_2->group_ctr[v58];
      do
      {
        group_ctr[n0x10000].last = 0x2000;
        group_ctr[n0x10000++].prev = 0x2000;
      }
      while ( n0x10000 < 0x10000 );
      Blocka_4 = (ModelBlock *)((uintptr_t)Blocka_5 + 1);
      v8 = v58 + 1;
    }
    while ( (uint32_t)(v58 + 1) < 0xF );
    Blocka_1 = (ModelBlock *)((int32_t)Blocka_2);
    buf = (uint8_t *)bmf_new(Blocka_2->alphabet);
    Size = Blocka_2->alphabet;
    Blocka_2->alpha_map = buf;
    memset(buf,1,Size);
    Blocka_2->escape_list = &Blocka_2->escape;
    __init_symbol_list(&Blocka_1->escape, 0, Blocka_1->alphabet, 1);
    Blocka_2->sel_cur = Blocka_2->sel;
    v31 = Blocka_2->alphabet;
    v32 = (SymListBlock *)bmf_new(24 * v31 + 4);
    if ( v32 )
    {
      v32->n = v31;
      v33 = v32->list;
      if ( v31 )
      {
        // MSVC unrolled this two lists at a time and left a scalar tail; both
        // halves write `ent`, and between them they cover 0 .. v31 - 1.
        for ( v34 = 0; v34 < v31; ++v34 )
          v33[v34].ent = nullptr;
        if ( v31 >> 1 )   // the register the unrolled loop clobbered
          Blocka_1 = (ModelBlock *)((int32_t)Blocka_2);
      }
    }
    else
    {
      v33 = nullptr;
    }
    v37 = Blocka_1->alphabet;
    Blocka_1->sel1_list = v33;
    v38 = (SymListBlock *)bmf_new(24 * v37 + 4);
    if ( v38 )
    {
      v38->n = v37;
      v39 = v38->list;
      if ( v37 )
      {
        // MSVC unrolled this two lists at a time and left a scalar tail; both
        // halves write `ent`, and between them they cover 0 .. v37 - 1.
        for ( v40 = 0; v40 < v37; ++v40 )
          v39[v40].ent = nullptr;
      }
    }
    else
    {
      v39 = nullptr;
    }
    Blocka_1->sel0_list = v39;
    if ( !(Blocka_1->alphabet <= 0) )
    {
      v44 = 0;
      do
      {
        __init_symbol_list(&Blocka_1->sel1_list[v44], 0, 99, 0);
        __init_symbol_list(&Blocka_1->sel0_list[v44++], 0, 33, 0);
      }
      while ( v44 < Blocka_1->alphabet );
    }
    if ( Blocka_1->height > 0 )
    {
      v59 = Blocka_1->sym_word;
      v45 = 0;
      do
      {
        v61 = v45 + 1;
        Blocka_1->row_cur[5]->match[1] = Blocka_1->row_cur[5][-1].sym == 0;
        Blocka_1->row_cur[5]->match[3] = Blocka_1->row_cur[6][-1].sym == 0;
        v46 = Blocka_1->row_cur[4];
        row_cur3 = Blocka_1->row_cur[3];
        row_cur2 = Blocka_1->row_cur[2];
        row_cur1 = Blocka_1->row_cur[1];
        v50 = Blocka_1->row_cur[0];
        Blocka_1->row_cur[4] = row_cur3;
        Blocka_1->row_cur[3] = row_cur2;
        Blocka_1->row_cur[2] = row_cur1;
        Blocka_1->row_cur[1] = v50;
        Blocka_1->row_cur[0] = v46;
        v46 += 7;
        Blocka_1->row_cur[5] = v46;
        v50 += 7;
        Blocka_1->row_cur[6] = v50;
        Blocka_1->row_cur[7] = row_cur1 + 7;
        Blocka_1->row_cur[8] = row_cur2 + 7;
        Blocka_1->row_cur[9] = row_cur3 + 7;
        // The same two flags, on the encoding side.
        {
          uint8_t zero = v50[1].sym == 0;
          v46->match[2] = zero;
          Blocka_1->row_cur[5][-1].match[4] = zero;
          Blocka_1->row_cur[5][-2].match[5] = zero;
          zero = Blocka_1->row_cur[6][2].sym == 0;
          Blocka_1->row_cur[5]->match[4] = zero;
          Blocka_1->row_cur[5][-1].match[5] = zero;
        }
        Blocka_1->row_cur[5]->match[5] = Blocka_1->row_cur[6][3].sym == 0;
        v51 = Blocka_1->row_cur[6];
        v52 = Blocka_1->row_cur[7];
        ++Blocka_1->row_cur[5];
        ++v51;
        Blocka_1->row_cur[6] = v51;
        ++Blocka_1->row_cur[8];
        ++v52;
        Blocka_1->row_cur[7] = v52;
        ++Blocka_1->row_cur[9];
        Blocka_1->grad[0] = v51[3].match[0] + v51[2].match[0] + v51[1].match[0] + v51[0].match[0] + v51[4].match[0] - 5;
        // The same five counts as the line above, off the other row.
        Blocka_1->grad[3] = 0;
        Blocka_1->grad[2] = 0;
        v45 = v61;
        v53 = Blocka_1->width;
        Blocka_1->grad[1] = v52[3].match[0] + v52[2].match[0] + v52[1].match[0] + v52[0].match[0] + v52[4].match[0] - 5;
        if ( v53 > 0 )
        {
          v54 = 0;
          do
          {
            ++v54;
            Blocka_1->row_cur[5][v54 - 1].sym = v59[v54 - 1];
            v53 = Blocka_1->width;
          }
          while ( v54 < Blocka_1->width );
          v59 += v54;
        }
        if ( v53 > 0 )
        {
          v55 = 0;
          do
          {
            v56 = __code_pixel((ModelBlock *)(int32_t *)Blocka_1, v55);
            __init_model_tables(Blocka_1);
            v55 += v56;
          }
          while ( v55 < Blocka_1->width );
          v45 = v61;
        }
      }
      while ( v45 < *(uint32_t *)&Blocka_1->height );
    }
    __rc_end_encode();
    __free_workspace((ModelBlock *)(void **)Blocka_1, 1);
  }
}

void __model_planes(uint8_t *Blockb, uint8_t *Srca_3, int32_t a3, int8_t a4)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and med32 divides by zero while compressing.
  struct alignas(16) ModelPlanesFrame {   // 76 bytes, one stack frame
      uint8_t slot0[4];
      uint8_t slot4[4];
      uint8_t slot8[4];
      uint8_t slot12[4];
      uint8_t slot16[4];
      // Four consecutive words that `model_plane` is handed as a `BmfImage *`:
      // this is the header, copied from the caller's a word at a time.
      BmfImage hdr;   // +20 .. +35
      int32_t v52;
      int32_t v53;
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80, "frame layout moved");
  ;
  uint8_t *v12;   // was int32_t: these hold addresses
  uint8_t *Srca_1, *Srca_2;   // `uint8_t *` beside the `char` scalars above
  uint8_t *__model_planes_buf;
  int32_t n1008;
  __frame.v53 = a3;
  Srca_1 = Srca_3;
  __frame.v52 = 16 * a3;
  plane_predictor = plane_desc[a3 + 1].flags & 3;
  plane_alt_model = (uint8_t)(plane_desc[a3 + 1].flags & 4) >> 2;
  Srca_2 = Srca_3;
  __colour_transform(Blockb, Srca_3, a3, a4);
  __model_planes_buf = ::hist_scratch;
  v12 = (uint8_t *)((uintptr_t)(::hist_scratch + 15) & 0xFFFFFFF0);
  // Fifteen bytes at the front and sixteen at +1008.  MSVC split the first
  // into 8 + 4 + 2 + 1 and the second into two eight-byte stores.
  memset(__model_planes_buf, 0, 15);
  bmf_zero16(&__model_planes_buf[1008]);
  n1008 = 1008;
  do
  {
    bmf_zero16((v12 + n1008 - 16));
    bmf_zero16((v12 + n1008 - 32));
    bmf_zero16((v12 + n1008 - 48));
    bmf_zero16((v12 + n1008 - 64));
    bmf_zero16((v12 + n1008 - 80));
    bmf_zero16((v12 + n1008 - 96));
    bmf_zero16((v12 + n1008 - 112));
    n1008 -= 112;
  }
  while ( n1008 );
  // always taken: -S
  {
    // The caller's header, with the depth byte replaced: 72 is 8 bits plus the
    // 0x40 flag `alloc_image` sets.
    __frame.hdr = *(BmfImage *)Blockb;
    __frame.hdr.depth = 72;
    // never taken: -E is 0
    if ( plane_predictor == 1 && !plane_alt_model )
      __predict_med(Srca_1, ((const BmfImage *)Blockb)->width, ((const BmfImage *)Blockb)->height);
    __model_plane(&__frame.hdr, Srca_1, Srca_2);
    // `if ( Srca_2 != Srca_1 )` stood here, and behind it an interleave and a
    // free.  It was the test for "the -E block above allocated a second
    // buffer"; with that block gone, both names hold the caller's one buffer
    // and the test is false on every path.  Deleting a block does not delete
    // the test that asked whether it ran, and this one outlived it by a
    // fortnight -- as the last thing in the file gcov could report as never
    // executed.  tools/deadcheck.py looks for the shape now.
  }
}


void __transform_planes(BmfImage *p_i, int32_t a2, int8_t a3)
{
  int32_t n4_3;
  uint16_t *Srca_3;
  uint8_t *p_ia;
  ;
  int8_t v11;
  uint8_t *__transform_planes_Buffer, *p_ia_1, *Src_1, *Src_3, *Src_2, *v20;   // `uint8_t *` beside the `char` scalars above
  int32_t n4_1, v14, predictor, v16, Size_3, n4_2, v21, i, Size_4,
          v24;
  uint8_t *Srca_1;
  memset(hist_scratch,0,4096);
  __transform_planes_Buffer = (uint8_t *)::coded_buf;
  p_ia_1 = (uint8_t *)::coded_buf + 16;
  // The whole sixteen-byte header, which MSVC moved a word at a time.
  *(BmfImage *)p_ia_1 = *p_i;
  Srca_1 = (uint8_t *)((uint16_t *)p_i->pixels);
  memcpy(__transform_planes_Buffer + 32,p_i->pixels,p_i->data_size);
  Src_1 = (uint8_t *)bmf_new(p_i->width * p_i->height);
  Src_3 = Src_1;
  if ( plane_count > 0 )
  {
    Srca_3 = (uint16_t *)p_i->pixels;
    p_ia = p_ia_1;
    n4_1 = 0;
    do
    {
      ++n4_1;
      v14 = BYTE1(plane_desc[n4_1].w0);
      predictor = plane_desc[v14 + 1].flags & 3;
      ::plane_predictor = predictor;
      v16 = (uint8_t)(plane_desc[v14 + 1].flags & 4) >> 2;
      plane_alt_model = v16;
      if ( ((plane_desc[v14 + 1].flags & 8) != 0 || predictor) && !v16 )
      {
        __colour_transform(p_ia, Src_1, v14, v11);
        if ( ::plane_predictor != 2 )
        {
          // never taken: -E is 0
          if ( ::plane_predictor == 1 )
          {
            __predict_med(Src_1, p_i->width, p_i->height);
          }
        }
        Size_3 = p_i->width * p_i->height;
        n4_2 = plane_count;
        Src_2 = (uint8_t *)p_i + v14 + 16;
        if ( plane_count == 1 )
        {
          memcpy(&p_i->pixels[v14],Src_1,Size_3);
        }
        else
        {
          v20 = (uint8_t *)p_i + v14;
          if ( Size_3 <= 6
            || plane_count <= 0
            || (Src_2 <= Src_1 || Size_3 > (uint32_t)(Src_2 - Src_1))
            && (plane_count > 1 || Src_2 >= Src_1 || Src_1 - Src_2 < (uint32_t)(Size_3 * plane_count)) )
          {
            Size_4 = 0;
            v24 = 0;
            do
            {
              v20[v24 + 16] = Src_1[Size_4];
              v24 += n4_2;
              ++Size_4;
            }
            while ( Size_4 < Size_3 );
          }
          else
          {
            n4_3 = n4_1;
            v21 = 0;
            for ( i = 0; i < Size_3; ++i )
            {
              v20[v21 + 16] = Src_1[i];
              v21 += n4_2;
            }
            n4_1 = n4_3;
          }
        }
      }
    }
    while ( n4_1 < plane_count );
    Src_3 = Src_1;
    Srca_1 = (uint8_t *)Srca_3;
  }
  free(Src_3);
  // always taken: -S
  {
    __model_plane((BmfImage *)p_i, Srca_1, Srca_1);
  }
}

uint8_t * __expand_image(uint8_t *a1, int32_t a4, void **p_coded_buf)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW exits 3 while decompressing.
  struct alignas(16) ExpandImageFrame {   // 104 bytes, one stack frame
      uint8_t *Block;
      // Four words with three roles, in three phases of the function.  MSVC
      // gave one stack slot to locals whose live ranges do not overlap and
      // Hex-Rays named every use; these are the roles rather than the names.
      union {
          BmfImage img;      // the image header `unmodel_plane` is handed
          uint32_t mask;     // before that: the packer's byte mask, word 0
          struct {           // after it: scratch, saved and restored
            uint32_t s0;
            int32_t  s4;
            int32_t  s8;
            uint32_t s12;
          };
      };
      int32_t n4_1;
      void *Src;
      uint8_t *v86;
      BmfImage *p_i_2;
      int32_t v88;
      uint32_t ElementCount_3;
      uint16_t Buffer_2[5];
      uint8_t v91;
      int8_t v92;
      uint32_t ElementCount;
      uint8_t   hdr[8];   // the 8-byte member header `fread` takes in one call
      uint32_t __expand_image_Buffer;
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 112, "frame layout moved");
  // These shared `__frame.Block` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *Blocka;
  ;
  uint8_t *v5;   // were int32_t: these hold addresses
  FILE *Stream_1, *Stream_v;
  BmfImage *p_i_1;
  int8_t v17, v34, v35;
  uint8_t v20;
  uint8_t *Buffer_3, *n4_6, *n4_7, *v64;   // `uint8_t *` beside the `char` scalars above
  uint8_t has_coded;
  int32_t Buffer__1, v21, n4, predictor, v27, v28, v29, v30, ArgList, v33,
          n4_4, v37, n2_1, i, Size_4, Size_5, n4_3, v44, Size_2, Size_3, n4_2,
          v48, n2_2, n_planes, Src_2, v58, n4_8, v61, i_1, n4_9, v76;
  uint16_t i_2;
  uint32_t __expand_image_Buffer_1, v12, *v13, ElementCount_5, ElementCount_2,
           v23, v25, Size_1, ElementCount_1, ElementCount_4, v67, v68, v69,
           v70, v74, v75;
  uint8_t *Src_1;
  void *Src_3;
  v5 = a1;
  if ( p_coded_buf )
    *p_coded_buf = nullptr;
  Stream_1 = ((BmfArc *)a1)->fp;
  if ( !Stream_1 )
    return nullptr;
  while ( 1 )
  {
    if ( fread(&__frame.__expand_image_Buffer, 4u, 1u, Stream_1) != 1 )
    {
      Stream_v = ((BmfArc *)v5)->fp;
      if ( feof(Stream_v) )
        return nullptr;
      {
        fclose(Stream_v);
        ((BmfArc *)v5)->fp = 0;
        return nullptr;
      }
    }
    __expand_image_Buffer_1 = __frame.__expand_image_Buffer;
    if ( (uint16_t)__frame.__expand_image_Buffer != 0x9081 )
      break;
    plane_desc[0].w4 = ((BYTE2(__frame.__expand_image_Buffer) << 8) - 12288) | (HIBYTE(__frame.__expand_image_Buffer) - 48);
    if ( plane_desc[0].w4 != 512 || fread(__frame.hdr, 8u, 1u, ((BmfArc *)v5)->fp) != 1 )
      break;
    fseek(((BmfArc *)v5)->fp, (*(uint32_t *)&__frame.hdr[4]), 1);
    Stream_1 = ((BmfArc *)v5)->fp;
  }
  if ( (uint16_t)__expand_image_Buffer_1 != 0x8A81
    || (plane_desc[0].w4 = ((BYTE2(__expand_image_Buffer_1) << 8) - 12288) | (HIBYTE(__expand_image_Buffer_1) - 48), plane_desc[0].w4 != 512)
    || fread(__frame.Buffer_2, 0x10u, 1u, ((BmfArc *)v5)->fp) != 1 )
  {
    Stream_v = ((BmfArc *)v5)->fp;
    fclose(Stream_v);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  ++*(uint32_t *)v5;
  if ( __frame.v92 < 0 )
  {
    fread(__frame.hdr, 8u, 1u, ((BmfArc *)v5)->fp);
    if ( p_coded_buf )
    {
      Buffer__1 = (*(int32_t *)&__frame.hdr[0]);
      v12 = ((*(uint32_t *)&__frame.hdr[4]) + ((*(uint32_t *)&__frame.hdr[4]) == 0) + 3) & 0xFFFFFFFC;
      v13 = (uint32_t *)bmf_new(v12 + 8);
      *v13 = Buffer__1;
      v13[1] = v12;
      *(uint32_t *)((uint8_t *)v13 + v12 + 4) = 0;
      *p_coded_buf = v13;
      fread(v13 + 2, (*(uint32_t *)&__frame.hdr[4]), 1u, ((BmfArc *)v5)->fp);
    }
    else
    {
      fseek(((BmfArc *)v5)->fp, (*(uint32_t *)&__frame.hdr[4]), 1);
    }
  }
  ElementCount_5 = 3 << (__frame.v91 & 31);
  if ( (__frame.v91 & 0x80) == 0 )
    ElementCount_5 = __frame.v91 & 0x80;
  __frame.ElementCount_3 = ElementCount_5;
  if ( a4 )
  {
    fseek(((BmfArc *)v5)->fp, __frame.ElementCount_3 + __frame.ElementCount, 1);
    return nullptr;
  }
  p_i_1 = (BmfImage *)((uint8_t *)__alloc_image(__frame.Buffer_2[0], __frame.Buffer_2[1], __frame.v91 & 0x3F, (uint8_t)(__frame.v91 & 0x80) >> 7, 1));
  __frame.v88 = __frame.v91;
  p_i_1->depth = __frame.v91;
  // The flag is whether there is a block at all; Hex-Rays kept the pointer's
  // low byte and then overwrote it with 1, which is the same thing said twice.
  has_coded = p_coded_buf && *p_coded_buf;
  v17 = __frame.v92;
  p_i_1->flags |= __frame.v92 & 2 | (has_coded << 7);
  ::plane_count = ((__frame.v88 & 0x3Fu) + 7) >> 3;
  if ( (v17 & 0x20) == 0 )
  {
    ElementCount_1 = __frame.ElementCount;
    if ( fread(p_i_1->pixels, 1u, __frame.ElementCount, ((BmfArc *)v5)->fp) != ElementCount_1 )
      {
        fclose(((BmfArc *)v5)->fp);
        ((BmfArc *)v5)->fp = 0;
        return nullptr;
      }
    goto LABEL_109;
  }
  plane_desc[0].w12 = 0;
  alphabet_reduced = 0;
  // Bit 2 of the descriptor is -S.  This build implements that mode and only
  // that mode -- the constants at the top of the file, and the fast back end
  // that used to decode the other one is gone -- so a stream without the bit is
  // refused rather than decoded wrongly.
  if ( (v17 & 4) == 0 )
  {
    printf("\nwritten in fast mode; this build only decodes -S streams\n");
    exit(3);
  }
  desc_slow_mode = 1;
  coded_size = __frame.ElementCount;
  ::coded_buf = (uint8_t *)bmf_new(__frame.ElementCount);
  out_cursor = ::coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  ::packer_word = (uint32_t *)::coded_buf;
  hist_scratch = ::coded_buf + coded_size - 4096;
  ElementCount_2 = __frame.ElementCount;
  if ( fread(::coded_buf, 1u, __frame.ElementCount, ((BmfArc *)v5)->fp) != ElementCount_2 )
  {
    fclose(((BmfArc *)v5)->fp);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  v20 = p_i_1->depth;
  if ( (v20 & 0x3Fu) <= 4 || (__frame.v92 & 0x10) == 0 )
  {
    ::plane_predictor = 0;
    plane_alt_model = 0;
    // always taken: -S
      __unmodel_plane(ElementCount_2, (uint16_t *)p_i_1, p_i_1->pixels);
    goto LABEL_106;
  }
  if ( ::plane_count == 1 )
  {
    if ( (v20 & 0x40) == 0 )
      goto LABEL_42;
  }
  else if ( ::plane_count <= 2 )
  {
    goto LABEL_42;
  }
  packer_free_bits -= 4;
  if ( packer_free_bits < 0 )
  {
    v75 = *(uint32_t *)out_cursor;
    out_cursor += 4;
    v76 = v75 << ((packer_free_bits + 4) & 31);
    ElementCount_2 = v75 >> (-packer_free_bits & 31);
    v21 = packer_acc | v76 & 0xF;
    packer_acc = ElementCount_2;
    packer_free_bits += 32;
  }
  else
  {
    v21 = packer_acc & 0xF;
    packer_acc = packer_acc >> 4;
  }
  // The 4-bit near-lossless field, ALGORITHM.md §4.1.  This build compresses
  // with -E0 and the code that reconstructs a quantised plane is gone with the
  // rest of the modes, so a stream that asks for E>0 is refused rather than
  // expanded wrongly.
  if ( v21 )
  {
    printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", v21);
    exit(3);
  }
  plane_desc[0].w12 = 0;
LABEL_42:
  if ( ::plane_count > 0 )
  {
    LOBYTE(ElementCount_2) = 63;
    __frame.mask = 255;
    __frame.p_i_2 = p_i_1;
    __frame.v86 = v5;
    n4 = 0;
    do
    {
      packer_free_bits -= 6;
      if ( packer_free_bits < 0 )
      {
        v74 = *(uint32_t *)out_cursor;
        out_cursor += 4;
        v23 = packer_acc | (v74 << ((packer_free_bits + 6) & 31)) & 0x3F;
        packer_acc = v74 >> (-packer_free_bits & 31);
        packer_free_bits += 32;
      }
      else
      {
        v23 = packer_acc & 0x3F;
        packer_acc = packer_acc >> 6;
      }
      v25 = v23 >> 2;
      predictor = v23 & 3;
      plane_desc[n4 + 1].flags = v25;
      plane_desc[n4 + 1].predictor = predictor;
      plane_desc[predictor + 1].src_plane = n4;
      if ( (plane_desc[n4 + 1].flags & 8) != 0 )
      {
        packer_free_bits -= 8;
        if ( packer_free_bits < 0 )
        {
          v70 = *(uint32_t *)out_cursor;
          out_cursor += 4;
          v27 = packer_acc | __frame.mask & (v70 << ((packer_free_bits + 8) & 31));
          packer_acc = v70 >> (-packer_free_bits & 31);
          packer_free_bits += 32;
        }
        else
        {
          LOBYTE(v27) = packer_acc & (uint8_t)__frame.mask;
          packer_acc = packer_acc >> 8;
        }
        plane_desc[n4 + 1].b3 = v27;
        if ( predictor > 1 )
        {
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            v69 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            v28 = packer_acc | __frame.mask & (v69 << ((packer_free_bits + 8) & 31));
            packer_acc = v69 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v28 = packer_acc & __frame.mask;
            packer_acc = packer_acc >> 8;
          }
          plane_desc[n4 + 1].w4 = v28 - 64;
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            v68 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            v29 = packer_acc | __frame.mask & (v68 << ((packer_free_bits + 8) & 31));
            packer_acc = v68 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v29 = packer_acc & __frame.mask;
            packer_acc = packer_acc >> 8;
          }
          plane_desc[n4 + 1].w8 = v29 - 64;
          if ( predictor > 2 )
          {
            packer_free_bits -= 8;
            if ( packer_free_bits < 0 )
            {
              v67 = *(uint32_t *)out_cursor;
              out_cursor += 4;
              v30 = packer_acc | __frame.mask & (v67 << ((packer_free_bits + 8) & 31));
              packer_acc = v67 >> (-packer_free_bits & 31);
              packer_free_bits += 32;
            }
            else
            {
              v30 = packer_acc & __frame.mask;
              packer_acc = packer_acc >> 8;
            }
            plane_desc[n4 + 1].w12 = v30 - 64;
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::plane_count );
    p_i_1 = __frame.p_i_2;
    v5 = __frame.v86;
  }
  Src_1 = (uint8_t *)bmf_new(p_i_1->width * p_i_1->height);
  if ( (__frame.v92 & 8) != 0 )
  {
    // The caller's header with the depth byte replaced, exactly as
    // `model_planes` does it: 72 is 8 bits plus the 0x40 flag.
    __frame.img = *p_i_1;
    __frame.img.depth = 72;
    if ( ::plane_count > 0 )
    {
      __frame.v86 = v5;
      ArgList = 0;
      do
      {
        v33 = plane_desc[ArgList + 1].src_plane;
        ::plane_predictor = plane_desc[v33 + 1].flags & 3;
        plane_alt_model = (uint8_t)(plane_desc[v33 + 1].flags & 4) >> 2;
        // always taken: -S
          __unmodel_plane(ArgList, (uint16_t *)&__frame.img, Src_1);
        if ( ::plane_predictor )
        {
          if ( ::plane_predictor == 1 )
          {
            if ( !plane_alt_model )
              __unpredict_med(Src_1, p_i_1->width, p_i_1->height);
          }
          // `else if ( !desc_slow_mode && ::plane_predictor == 2 )` -- the fast-mode
          // predictor-2 expander, never reached: -S is on.
        }
        else
        {
          __expand_predictor_mode0((uint32_t)Src_1, p_i_1->width, p_i_1->height);
        }
        __interleave_plane((uint8_t *)p_i_1, Src_1, v33, v34);
        ++ArgList;
      }
      while ( ArgList < ::plane_count );
LABEL_104:
      v5 = __frame.v86;
    }
  }
  else
  {
    ::plane_predictor = plane_desc[1].flags & 3;
    plane_alt_model = (uint8_t)(plane_desc[1].flags & 4) >> 2;
    // always taken: -S
    {
      __unmodel_plane(ElementCount_2, (uint16_t *)p_i_1, p_i_1->pixels);
      if ( plane_alt_model )
        goto LABEL_105;
    }
    if ( ::plane_count > 0 )
    {
      __frame.v86 = v5;
      n4_4 = 0;
      do
      {
        ++n4_4;
        v37 = BYTE1(plane_desc[n4_4].w0);
        n2_1 = plane_desc[v37 + 1].flags & 3;
        ::plane_predictor = n2_1;
        if ( (plane_desc[v37 + 1].flags & 8) != 0 || n2_1 )
        {
          i = p_i_1->width;
          __frame.Src = &((uint8_t *)p_i_1)[v37 + 16];
          Size_1 = i * p_i_1->height;
          __frame.n4_1 = ::plane_count;
          __frame.s12 = Size_1;
          if ( ::plane_count == 1 )
          {
            memcpy(Src_1,(uint8_t *)__frame.Src,__frame.s12);
            n2_2 = ::plane_predictor;
          }
          else
          {
            __frame.Block = &((uint8_t *)p_i_1)[v37];
            if ( (int32_t)__frame.s12 <= 6
              || __frame.n4_1 <= 0
              || (__frame.n4_1 > 1 || Src_1 <= __frame.Src || Src_1 - (uint8_t *)__frame.Src < __frame.s12 * ::plane_count)
              && (Src_1 >= __frame.Src || (uint8_t *)__frame.Src - Src_1 < __frame.s12) )
            {
              __frame.s0 = n2_1;
              __frame.s4 = v37;
              __frame.s8 = n4_4;
              Size_2 = __frame.s12;
              __frame.p_i_2 = p_i_1;
              Size_3 = 0;
              n4_2 = __frame.n4_1;
              v48 = 0;
              do
              {
                Src_1[Size_3] = __frame.Block[v48 + 16];
                v48 += n4_2;
                ++Size_3;
              }
              while ( Size_3 < Size_2 );
            }
            else
            {
              __frame.s0 = n2_1;
              __frame.s4 = v37;
              __frame.s8 = n4_4;
              Size_4 = __frame.s12;
              __frame.p_i_2 = p_i_1;
              Size_5 = 0;
              n4_3 = __frame.n4_1;
              v44 = 0;
              do
              {
                Src_1[Size_5] = __frame.Block[v44 + 16];
                v44 += n4_3;
                ++Size_5;
              }
              while ( Size_5 < Size_4 );
            }
            n2_2 = __frame.s0;
            v37 = __frame.s4;
            n4_4 = __frame.s8;
            p_i_1 = __frame.p_i_2;
          }
          if ( n2_2 )
          {
            if ( n2_2 == 1 )
              __unpredict_med(Src_1, p_i_1->width, p_i_1->height);
          }
          else
          {
            __expand_predictor_mode0((uint32_t)Src_1, p_i_1->width, p_i_1->height);
          }
          __interleave_plane((uint8_t *)p_i_1, Src_1, v37, v35);
        }
      }
      while ( n4_4 < ::plane_count );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(Src_1);
LABEL_106:
  if ( ::coded_buf + __frame.ElementCount != out_cursor )
  {
    fclose(((BmfArc *)v5)->fp);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  free(::coded_buf);
  __frame.v88 = __frame.v91;
LABEL_109:
  if ( (__frame.v88 & 0x80) != 0 )
  {
    // `f10 < 0` was a signed int8_t testing its own top bit -- the palette
    // flag.  depth is unsigned, so the test has to name the bit; it read as
    // always-false otherwise, which is what the gate caught.
    Buffer_3 = (p_i_1->depth & 0x80) ? &((uint8_t *)p_i_1)[p_i_1->data_size + 16] : nullptr;
    ElementCount_4 = fread(Buffer_3, 1u, __frame.ElementCount_3, ((BmfArc *)v5)->fp);
    if ( ElementCount_4 != __frame.ElementCount_3 )
      {
        fclose(((BmfArc *)v5)->fp);
        ((BmfArc *)v5)->fp = 0;
        return nullptr;
      }
  }
  if ( (p_i_1->flags & 2) != 0 )
  {
    n4_6 = (uint8_t *)bmf_new(p_i_1->data_size);
    n_planes = ::plane_count;
    __frame.n4_1 = (int32_t)n4_6;
    Src_2 = ::plane_count * (p_i_1->height - 1);
    memcpy(n4_6,p_i_1->pixels,p_i_1->data_size);
    LOWORD(v58) = p_i_1->height;
    if ( (uint16_t)v58 )
    {
      __frame.Src = (void *)Src_2;
      n4_7 = (uint8_t *)__frame.n4_1;
      Blocka = n4_6;
      n4_8 = 0;
      v61 = 0;
      do
      {
        i_1 = p_i_1->width;
        __frame.n4_1 = n4_8;
        __frame.v86 = (uint8_t *)v61;
        Src_3 = __frame.Src;
        __frame.p_i_2 = p_i_1;
        v64 = &((uint8_t *)p_i_1)[n4_8 + 16];
        do
        {
          n4_9 = n_planes;
          do
          {
            *v64++ = *n4_7++;
            --n4_9;
          }
          while ( n4_9 );
          v64 = &v64[(uint32_t)Src_3];
          --i_1;
        }
        while ( i_1 );
        p_i_1 = __frame.p_i_2;
        v58 = *((uint16_t *)__frame.p_i_2 + 1);
        n4_8 = n_planes + __frame.n4_1;
        v61 = (int32_t)(uintptr_t)__frame.v86 + 1;
      }
      while ( __frame.v86 + 1 < (uint8_t *)(uintptr_t)v58 );
      n4_6 = Blocka;
    }
    // The deinterleave swaps width and height and rewrites the stride; only
    // the low half of `stride` is touched, which is what `(uint16_t *)p_i + 2`
    // was addressing.
    i_2 = p_i_1->width;
    p_i_1->width = v58;
    p_i_1->flags ^= 2u;
    p_i_1->height = i_2;
    *(uint16_t *)&p_i_1->stride = v58 * n_planes;
    free(n4_6);
  }
  return (uint8_t *)p_i_1;
}


uint32_t __search_filter(BmfImage *p_i, int8_t a2)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and five streams move, no signal.
  struct alignas(16) SearchFilterFrame {   // 164 bytes, one stack frame
      uint8_t *v175;
      uint8_t v177[16];
      uint8_t *v178[2];
      int32_t v179[2];
      int32_t v180;
      uint32_t v181[4];
      int32_t v182;
      void *Srca_7;
      int32_t v184;
      uint8_t *n5_1;
      uint8_t _pad0[28];
      void *Src;
      uint8_t *n4_10;
      int32_t v188;
      int32_t v189;
      uint8_t *n4_15;
      BmfImage *p_i_2;
      uint8_t *Blockb;
      uint8_t _pad1[36];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  // These shared `__frame.v175` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *v176;
  ;
  bool v35, v162;
  int8_t v19, v44, v63, v82, v93, v116;
  uint8_t v62, v70, v77, v86;
  uint8_t *v24, *v26, *v27, *n4_13, *Blockb_2, *Srca_1, *n4_14, *v67, *v74, *n4_17, *v101, *v105, *Blockb_6, *v110, *v111, *v112, *Blockb_7, *v124, *v130, *v136, *Blockb_8, *v141, *v142, *v143, *Blockb_9, *Blockb_4, *Srca_3, *Blockb_5, *Srca_4, *n0x7FFFFFFF_5, *Srca_5, *Blockb_3, *Srca_2, *Blockb_1, *Srca, *n5_6;   // `uint8_t *` beside the `char` scalars above
  int16_t v114, v115, v145, v146;
  int32_t i, i_2, n4, n4_4, v21, v22, v23, Size, n0x7FFFFFFF, n0x7FFFFFFF_8,
          n0x7FFFFFFF_2, n5, n2, v40, v41, v42, v43, v45, n16_1, n4_7, n4_8,
          v61, n16_2, n4_9, v69, v71, n16, plane, v76, n4_20, n4_6, n16_3,
          n4_11, v85, n16_4, n4_12, n4_18, v102, v104, v106, v108, v109,
          n4_19, v118, v119, v120, v121, v123, v125, v126, i_6, v133, v135,
          v137, v139, v140, v149, n0x7FFFFFFF_10, n0x7FFFFFFF_4, n5_3, v155,
          n0x7FFFFFFF_11, n5_4, v160, n0x7FFFFFFF_6, n5_5, v166,
          n0x7FFFFFFF_9, n0x7FFFFFFF_3, n5_2, v172, n0x7FFFFFFF_1;
  BmfImage *p_i_1;
  uint16_t i_7, v132;
  uint32_t n64_2, n64_1, n64, n64_3, n64_4;
  uint8_t *v128, *v129;
  p_i_1 = (BmfImage *)(p_i);
  i = p_i->width;
  i_2 = p_i->height;
  // `if ( (uint32_t)opt_search_quality < 9 )` -- the tile-size cap that a -Q below 9 put on
  // the filter search.  -Q is 9, so the search sees the whole image.
  if ( i < 4 || i_2 < 3 )
  {
    if ( ::plane_count > 0 )
    {
      n4 = 0;
      do
      {
        plane_desc[n4 + 1].flags = 0;
        plane_desc[n4 + 1].src_plane = n4;
        plane_desc[n4 + 1].predictor = n4;
        ++n4;
      }
      while ( n4 < ::plane_count );
    }
    return 0;
  }
  __choose_plane_coding((BmfImage *)p_i_1, i_2, a2);
  // `if ( opt_filter_template == 2 )` -- 94 lines of the -T2 filter-template path, gone
  // with the mode.  See REFACTORING.md §2.
  __frame.Blockb = (uint8_t *)__alloc_image(i, i_2, p_i_1->depth & 0x3F, 0, 0);
  coded_size = *((uint32_t *)__frame.Blockb + 3) + 0x20000;
  coded_buf = (uint8_t *)bmf_new(coded_size);
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t *)coded_buf;
  __frame.v179[0] = i_2 * i;
  hist_scratch = coded_buf + coded_size - 4096;
  __frame.Srca_7 = bmf_new(i_2 * i);
  n4_4 = ::plane_count;
  v21 = (p_i_1->height - i_2) >> 1;
  v22 = p_i_1->width - i;
  v23 = v21 * p_i_1->stride;
  __frame.v178[1] = (uint8_t *)v21;
  v24 = (uint8_t *)p_i_1 + ::plane_count * (v22 >> 1) + v23 + 16;
  __frame.Src = __frame.Blockb + 16;
  __frame.v178[0] = __frame.Blockb + 16;
  if ( v21 < i_2 + v21 )
  {
    Size = *((uint16_t *)__frame.Blockb + 2);
    __frame.v179[1] = i_2;
    v26 = __frame.v178[1];
    __frame.p_i_2 = (BmfImage *)(p_i_1);
    v27 = __frame.v178[0];
    do
    {
      memcpy(v27,v24,Size);
      Size = *((uint16_t *)__frame.Blockb + 2);
      v27 += Size;
      v24 += __frame.p_i_2->stride;
      ++v26;
    }
    while ( (int32_t)v26 < __frame.v179[1] + ((__frame.p_i_2->height - __frame.v179[1]) >> 1) );
    p_i_1 = (BmfImage *)(__frame.p_i_2);
    n4_4 = ::plane_count;
  }
  __frame.n4_10 = nullptr;
  __frame.v188 = 0;
  __frame.v189 = 0;
  __frame.n4_15 = nullptr;
  if ( n4_4 > 0 )
  {
    __frame.p_i_2 = (BmfImage *)(p_i_1);
    n4_13 = __frame.n4_10;
    __frame.n5_1 = nullptr;
    __frame.v182 = 0;
    __frame.v179[0] *= 8;
    while ( 1 )
    {
      __frame.v179[1] = plane_desc[__frame.v182 + 1].src_plane;
      __frame.v180 = __frame.v179[1];   // a record index; it was 16 * it, the byte offset
      if ( n4_13 )
      {
        n0x7FFFFFFF = 0x7FFFFFFF;
      }
      else
      {
        Blockb_1 = __frame.Blockb;
        Srca = (uint8_t *)__frame.Srca_7;
        v172 = __frame.v179[1];
        plane_desc[__frame.v179[1] + 1].flags = 0;
        __model_planes(Blockb_1, Srca, v172, v19);
        n0x7FFFFFFF_1 = 8 * (out_cursor - coded_buf);
        n0x7FFFFFFF = plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_1 + 32;
        v35 = 0;                            // -S
        *(uint32_t *)packer_word = packer_acc;
        if ( !v35 )
          n0x7FFFFFFF = n0x7FFFFFFF_1;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        n5_6 = __frame.n5_1;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( n0x7FFFFFFF == 0x7FFFFFFF )
          n0x7FFFFFFF = 0x7FFFFFFF;
        else
          n5_6 = nullptr;
        __frame.n5_1 = n5_6;
      }
      Blockb_2 = __frame.Blockb;
      Srca_1 = (uint8_t *)__frame.Srca_7;
      plane_desc[__frame.v180 + 1].flags = 5;
      __model_planes(Blockb_2, Srca_1, __frame.v179[1], v19);
      n0x7FFFFFFF_8 = 8 * (out_cursor - coded_buf);
      n0x7FFFFFFF_2 = plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_8 + 32;
      v35 = 0;                            // -S
      *(uint32_t *)packer_word = packer_acc;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      if ( !v35 )
        n0x7FFFFFFF_2 = n0x7FFFFFFF_8;
      packer_free_bits = 0;
      packer_acc = 0;
      hist_scratch = coded_buf + coded_size - 4096;
      n5 = (int32_t)__frame.n5_1;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF )
      {
        n0x7FFFFFFF = n0x7FFFFFFF_2;
        n5 = 5;
      }
      __frame.n5_1 = (uint8_t *)n5;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF + (n0x7FFFFFFF >> 5) || n4_13 )
      {
        Blockb_3 = __frame.Blockb;
        Srca_2 = (uint8_t *)__frame.Srca_7;
        v166 = __frame.v179[1];
        plane_desc[__frame.v180 + 1].flags = 6;
        __model_planes(Blockb_3, Srca_2, v166, v19);
        n0x7FFFFFFF_9 = 8 * (out_cursor - coded_buf);
        n0x7FFFFFFF_3 = plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_9 + 32;
        v35 = 0;                            // -S
        *(uint32_t *)packer_word = packer_acc;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        if ( !v35 )
          n0x7FFFFFFF_3 = n0x7FFFFFFF_9;
        packer_free_bits = 0;
        packer_acc = 0;
        n5_2 = (int32_t)__frame.n5_1;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( n0x7FFFFFFF_3 < n0x7FFFFFFF )
        {
          n0x7FFFFFFF = n0x7FFFFFFF_3;
          n5_2 = 6;
        }
        __frame.n5_1 = (uint8_t *)n5_2;
        if ( __frame.v182 )
        {
          // always taken: -Q is 9, so `opt_search_quality > 5` decides it whatever n4_13 is
          {
LABEL_191:
            Blockb_4 = __frame.Blockb;
            Srca_3 = (uint8_t *)__frame.Srca_7;
            v149 = __frame.v179[1];
            plane_desc[__frame.v180 + 1].flags = 8;
            __model_planes(Blockb_4, Srca_3, v149, v19);
            n0x7FFFFFFF_10 = 8 * (out_cursor - coded_buf);
            n0x7FFFFFFF_4 = plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_10 + 32;
            v35 = 0;                            // -S
            *(uint32_t *)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            if ( !v35 )
              n0x7FFFFFFF_4 = n0x7FFFFFFF_10;
            packer_free_bits = 0;
            packer_acc = 0;
            n5_3 = (int32_t)__frame.n5_1;
            hist_scratch = coded_buf + coded_size - 4096;
            if ( n0x7FFFFFFF_4 < n0x7FFFFFFF )
            {
              n0x7FFFFFFF = n0x7FFFFFFF_4;
              n5_3 = 8;
            }
            __frame.n5_1 = (uint8_t *)n5_3;
          }
          Blockb_5 = __frame.Blockb;
          Srca_4 = (uint8_t *)__frame.Srca_7;
          v155 = __frame.v179[1];
          plane_desc[__frame.v180 + 1].flags = 13;
          __model_planes(Blockb_5, Srca_4, v155, v19);
          n0x7FFFFFFF_11 = 8 * (out_cursor - coded_buf);
          n0x7FFFFFFF_5 = (uint8_t *)(plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_11 + 32);
          v35 = 0;                            // -S
          *(uint32_t *)packer_word = packer_acc;
          out_cursor = coded_buf;
          packer_word = (uint32_t *)coded_buf;
          if ( !v35 )
            n0x7FFFFFFF_5 = (uint8_t *)n0x7FFFFFFF_11;
          __frame.v178[0] = n0x7FFFFFFF_5;
          packer_free_bits = 0;
          packer_acc = 0;
          n5_4 = (int32_t)__frame.n5_1;
          hist_scratch = coded_buf + coded_size - 4096;
          if ( (int32_t)n0x7FFFFFFF_5 < n0x7FFFFFFF )
          {
            n0x7FFFFFFF = (int32_t)n0x7FFFFFFF_5;
            n5_4 = 13;
          }
          __frame.n5_1 = (uint8_t *)n5_4;
          n2 = n5_4 & 3;
          if ( n2 == 2 || n0x7FFFFFFF + (n0x7FFFFFFF >> 5) > (int32_t)__frame.v178[0] )
          {
            Srca_5 = (uint8_t *)__frame.Srca_7;
            v160 = __frame.v179[1];
            plane_desc[__frame.v180 + 1].flags = 14;
            __model_planes((uint8_t *)__frame.Blockb, Srca_5, v160, v19);
            n0x7FFFFFFF_6 = plane_desc[0].w0 - packer_free_bits + 8 * (out_cursor - coded_buf) + 32;
            // always taken: -S
              n0x7FFFFFFF_6 = 8 * (out_cursor - coded_buf);
            v162 = n0x7FFFFFFF_6 < n0x7FFFFFFF;
            if ( n0x7FFFFFFF_6 < n0x7FFFFFFF )
              n0x7FFFFFFF = n0x7FFFFFFF_6;
            *(uint32_t *)packer_word = packer_acc;
            packer_free_bits = 0;
            packer_acc = 0;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            hist_scratch = coded_buf + coded_size - 4096;
            n5_5 = (int32_t)__frame.n5_1;
            if ( v162 )
              n5_5 = 14;
            __frame.n5_1 = (uint8_t *)n5_5;
            if ( v162 )
              n2 = 2;
          }
          goto LABEL_43;
        }
        n2 = n5_2 & 3;
      }
      else
      {
        if ( __frame.v182 )
          goto LABEL_191;
        n2 = (uint8_t)(uintptr_t)__frame.n5_1 & 3;
      }
LABEL_43:
      // always taken: -S
      {
        __frame.v184 = (uint8_t)(uintptr_t)__frame.n5_1 & 8;
      }
      __frame.n4_15 += n0x7FFFFFFF;
      v40 = n2 == 2;
      if ( n2 != 1 )
        n2 = 0;
      v41 = __frame.v180;
      n4_13 = (uint8_t *)((uintptr_t)n4_13 + (v40));
      __frame.v188 += n2;
      v42 = __frame.v184;
      __frame.v181[__frame.v179[1]] = n0x7FFFFFFF;
      v43 = __frame.v182;
      __frame.v189 += v42 != 0;
      plane_desc[v41 + 1].flags = (uint8_t)(uintptr_t)__frame.n5_1;
      __frame.v182 = v43 + 1;
      if ( v43 + 1 >= ::plane_count )
      {
        __frame.n4_10 = n4_13;
        p_i_1 = (BmfImage *)(__frame.p_i_2);
        break;
      }
    }
  }
  // always taken: -Q is 9
  {
    v101 = (uint8_t *)bmf_new(*((uint32_t *)__frame.Blockb + 3));
    v102 = *((uint16_t *)__frame.Blockb + 1);
    __frame.v178[1] = (uint8_t *)::plane_count;
    __frame.v180 = (int32_t)v101;
    __frame.v182 = ::plane_count * (v102 - 1);
    memcpy(v101,(uint8_t *)__frame.Src,*((uint32_t *)__frame.Blockb + 3));
    LOWORD(v104) = *((uint16_t *)__frame.Blockb + 1);
    if ( (uint16_t)v104 )
    {
      __frame.v178[0] = v101;
      v105 = __frame.v178[1];
      __frame.p_i_2 = (BmfImage *)(p_i_1);
      v106 = 0;
      Blockb_6 = __frame.Blockb;
      v108 = 0;
      do
      {
        v109 = ((const BmfImage *)Blockb_6)->width;
        __frame.v179[1] = v108;
        __frame.v179[0] = v106;
        v110 = &Blockb_6[v108 + 16];
        v111 = (uint8_t *)__frame.v180;
        do
        {
          v112 = v105;
          do
          {
            *v110++ = *v111++;
            v112 = (uint8_t *)((uintptr_t)v112 - 1);
          }
          while ( v112 );
          v110 += __frame.v182;
          --v109;
        }
        while ( v109 );
        Blockb_6 = __frame.Blockb;
        v104 = *((uint16_t *)__frame.Blockb + 1);
        __frame.v180 = (int32_t)v111;
        v108 = (int32_t)&v105[__frame.v179[1]];
        v106 = __frame.v179[0] + 1;
      }
      while ( __frame.v179[0] + 1 < v104 );
      v101 = __frame.v178[0];
      p_i_1 = (BmfImage *)(__frame.p_i_2);
    }
    Blockb_7 = __frame.Blockb;
    v114 = ((const BmfImage *)__frame.Blockb)->width;
    v115 = v104 * LOWORD(__frame.v178[1]);
    ((BmfImage *)__frame.Blockb)->width = v104;
    ((BmfImage *)Blockb_7)->height = v114;
    ((BmfImage *)Blockb_7)->flags ^= 2u;
    *(uint16_t *)&((BmfImage *)Blockb_7)->stride = v115;
    free(v101);
    n4_19 = 0;
    if ( ::plane_count > 0 )
    {
      __frame.p_i_2 = (BmfImage *)(p_i_1);
      v118 = 0;
      while ( 1 )
      {
        v119 = plane_desc[v118 + 1].src_plane;
        __frame.v178[0] = (uint8_t *)v119;
        __model_planes((uint8_t *)__frame.Blockb, (uint8_t *)__frame.Srca_7, v119, v116);
        v120 = 8 * (out_cursor - coded_buf);
        // never taken: -S
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        n4_19 += v120;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( v120 - (v120 >> 8) > __frame.v181[(int32_t)__frame.v178[0]] )
          break;
        if ( ++v118 >= ::plane_count )
        {
          p_i_1 = (BmfImage *)(__frame.p_i_2);
          goto LABEL_172;
        }
      }
      p_i_1 = (BmfImage *)(__frame.p_i_2);
      n4_19 += (int32_t)(__frame.n4_15 + 1);
    }
LABEL_172:
    if ( n4_19 + (n4_19 >> 12) >= (int32_t)__frame.n4_15 )
    {
      __frame.v178[0] = (uint8_t *)bmf_new(*((uint32_t *)__frame.Blockb + 3));
      v133 = ::plane_count * (*((uint16_t *)__frame.Blockb + 1) - 1);
      __frame.v178[1] = (uint8_t *)::plane_count;
      __frame.v180 = (int32_t)__frame.v178[0];
      memcpy(__frame.v178[0],(uint8_t *)__frame.Src,*((uint32_t *)__frame.Blockb + 3));
      LOWORD(v135) = *((uint16_t *)__frame.Blockb + 1);
      if ( (uint16_t)v135 )
      {
        __frame.v181[0] = v133;
        v136 = __frame.v178[1];
        __frame.p_i_2 = (BmfImage *)(p_i_1);
        v137 = 0;
        Blockb_8 = __frame.Blockb;
        v139 = 0;
        do
        {
          v140 = ((const BmfImage *)Blockb_8)->width;
          __frame.v179[1] = v139;
          __frame.v179[0] = v137;
          v141 = &Blockb_8[v139 + 16];
          v142 = (uint8_t *)__frame.v180;
          do
          {
            v143 = v136;
            do
            {
              *v141++ = *v142++;
              v143 = (uint8_t *)((uintptr_t)v143 - 1);
            }
            while ( v143 );
            v141 += __frame.v181[0];
            --v140;
          }
          while ( v140 );
          Blockb_8 = __frame.Blockb;
          v135 = *((uint16_t *)__frame.Blockb + 1);
          __frame.v180 = (int32_t)v142;
          v139 = (int32_t)&v136[__frame.v179[1]];
          v137 = __frame.v179[0] + 1;
        }
        while ( __frame.v179[0] + 1 < v135 );
        p_i_1 = (BmfImage *)(__frame.p_i_2);
      }
      Blockb_9 = __frame.Blockb;
      v176 = __frame.v178[0];
      v145 = ((const BmfImage *)__frame.Blockb)->width;
      v146 = v135 * LOWORD(__frame.v178[1]);
      ((BmfImage *)__frame.Blockb)->width = v135;
      ((BmfImage *)Blockb_9)->height = v145;
      ((BmfImage *)Blockb_9)->flags ^= 2u;
      *(uint16_t *)&((BmfImage *)Blockb_9)->stride = v146;
      free(v176);
    }
    else
    {
      __frame.n4_15 = (uint8_t *)n4_19;
      __frame.v178[0] = (uint8_t *)bmf_new(p_i_1->data_size);
      v121 = p_i_1->height;
      __frame.v178[1] = (uint8_t *)::plane_count;
      __frame.v179[0] = (int32_t)__frame.v178[0];
      __frame.v181[0] = ::plane_count * (v121 - 1);
      memcpy(__frame.v178[0],p_i_1->pixels,p_i_1->data_size);
      LOWORD(v123) = p_i_1->height;
      if ( (uint16_t)v123 )
      {
        v124 = __frame.v178[1];
        v125 = 0;
        v126 = 0;
        do
        {
          i_6 = p_i_1->width;
          __frame.v179[1] = v126;
          __frame.v180 = v125;
          v128 = (uint8_t *)p_i_1 + v126 + 16;
          __frame.p_i_2 = (BmfImage *)(p_i_1);
          v129 = (uint8_t *)__frame.v179[0];
          do
          {
            v130 = v124;
            do
            {
              *v128++ = *v129++;
              v130 = (uint8_t *)((uintptr_t)v130 - 1);
            }
            while ( v130 );
            v128 += __frame.v181[0];
            --i_6;
          }
          while ( i_6 );
          __frame.v179[0] = (int32_t)v129;
          p_i_1 = (BmfImage *)(__frame.p_i_2);
          v123 = __frame.p_i_2->height;
          v126 = (int32_t)&v124[__frame.v179[1]];
          v125 = __frame.v180 + 1;
        }
        while ( __frame.v180 + 1 < v123 );
      }
      __frame.v175 = __frame.v178[0];
      i_7 = p_i_1->width;
      v132 = v123 * LOWORD(__frame.v178[1]);
      p_i_1->width = v123;
      p_i_1->height = i_7;
      p_i_1->flags ^= 2u;
      p_i_1->stride = v132;
      free(__frame.v175);
    }
  }
  free(__frame.Srca_7);
  if ( ::plane_count > 2 )
  {
    if ( __frame.v188 )
    {
      n16 = 16;
      do
      {
        __frame.v179[n16 + 1] = plane_desc[n16 / 4].w12;
        __frame.v179[n16] = *(int32_t *)((uint8_t *)&::plane_count + n16 * 4);
        __frame.v178[n16 + 1] = (uint8_t *)plane_desc[n16 / 4].w4;
        v74 = (uint8_t *)plane_desc[n16 / 4].w0;
        __frame.v178[n16] = v74;
        n16 -= 4;
      }
      while ( n16 * 4 );
      if ( ::plane_count > 0 )
      {
        plane = 0;
        do
        {
          v76 = plane;   // a record index; it was 16 * it
          v77 = plane_desc[plane++ + 1].flags & 8 | 5;
          plane_desc[v76 + 1].flags = v77;
        }
        while ( plane < ::plane_count );
      }
      __transform_planes((BmfImage *)(uint16_t *)__frame.Blockb, (int32_t)v74, v44);
      n4_20 = 8 * (out_cursor - coded_buf);
      // never taken: -S
      v162 = n4_20 <= (int32_t)__frame.n4_15;
      *(uint32_t *)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      hist_scratch = coded_buf + coded_size - 4096;
      if ( v162 )
      {
        __frame.n4_15 = (uint8_t *)n4_20;
        n4_6 = ::plane_count;
        v45 = 0;
      }
      else
      {
        n64 = 64;
        do
        {
          *(uint64_t *)(bmf_plane_desc(n64 - 8)) = *(uint64_t *)&__frame.v179[n64 / 4];
          *(uint64_t *)(bmf_plane_desc(n64 - 16)) = *(uint64_t *)&__frame.v178[n64 / 4];
          *(uint64_t *)(bmf_plane_desc(n64 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64 / 8 + 1];
          *(uint64_t *)(bmf_plane_desc(n64 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64 / 8];
          n64 -= 32;
        }
        while ( n64 );
        n4_6 = ::plane_count;
        v45 = 1;
      }
      if ( n4_6 <= 2 )
        goto LABEL_63;
    }
    else
    {
      v45 = 1;
    }
    if ( &__frame.n4_10[v45 == 0] )
    {
      n16_1 = 16;
      do
      {
        __frame.v179[n16_1 + 1] = plane_desc[n16_1 / 4].w12;
        __frame.v179[n16_1] = *(int32_t *)((uint8_t *)&::plane_count + n16_1 * 4);
        __frame.v178[n16_1 + 1] = (uint8_t *)plane_desc[n16_1 / 4].w4;
        __frame.v178[n16_1] = (uint8_t *)plane_desc[n16_1 / 4].w0;
        n16_1 -= 4;
      }
      while ( n16_1 * 4 );
      n4_7 = ::plane_count;
      if ( ::plane_count > 0 )
      {
        n4_8 = 0;
        do
        {
          v61 = n4_8;   // a record index; it was 16 * it
          v62 = plane_desc[n4_8++ + 1].flags & 8 | 6;
          plane_desc[v61 + 1].flags = v62;
          n4_7 = ::plane_count;
        }
        while ( n4_8 < ::plane_count );
      }
      if ( (uint8_t *)n4_7 == __frame.n4_10 && n4_7 - 1 == __frame.v189 )
      {
        v45 = 0;
      }
      else
      {
        __transform_planes((BmfImage *)(uint16_t *)__frame.Blockb, v45, v44);
        n4_14 = (uint8_t *)(8 * (out_cursor - coded_buf));
        // never taken: -S
        v162 = (int32_t)n4_14 <= (int32_t)__frame.n4_15;
        __frame.v178[0] = n4_14;
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( v162 )
        {
          __frame.n4_15 = n4_14;
          __frame.n4_10 = n4_14;
          if ( ::plane_count - 1 == __frame.v189 )
          {
            v45 = 0;
          }
          else
          {
            n16_2 = 16;
            do
            {
              __frame.v179[n16_2 + 1] = plane_desc[n16_2 / 4].w12;
              __frame.v179[n16_2] = *(int32_t *)((uint8_t *)&::plane_count + n16_2 * 4);
              __frame.v178[n16_2 + 1] = (uint8_t *)plane_desc[n16_2 / 4].w4;
              v67 = (uint8_t *)plane_desc[n16_2 / 4].w0;
              __frame.v178[n16_2] = v67;
              n16_2 -= 4;
            }
            while ( n16_2 * 4 );
            if ( ::plane_count > 0 )
            {
              n4_9 = 0;
              do
              {
                v69 = n4_9;   // a record index; it was 16 * it
                v70 = plane_desc[n4_9++ + 1].predictor;
                plane_desc[v69 + 1].flags |= 8 * (v70 != 0);
              }
              while ( n4_9 < ::plane_count );
            }
            __transform_planes((BmfImage *)(uint16_t *)__frame.Blockb, (int32_t)v67, v63);
            v71 = 8 * (out_cursor - coded_buf);
            // never taken: -S
            v162 = v71 <= (int32_t)__frame.v178[0];
            *(uint32_t *)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            packer_free_bits = 0;
            packer_acc = 0;
            hist_scratch = coded_buf + coded_size - 4096;
            if ( v162 )
            {
              v45 = 0;
            }
            else
            {
              n64_1 = 64;
              do
              {
                *(uint64_t *)(bmf_plane_desc(n64_1 - 8)) = *(uint64_t *)&__frame.v179[n64_1 / 4];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 16)) = *(uint64_t *)&__frame.v178[n64_1 / 4];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_1 / 8 + 1];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_1 / 8];
                n64_1 -= 32;
              }
              while ( n64_1 );
              v45 = 0;
            }
          }
        }
        else
        {
          n64_2 = 64;
          do
          {
            *(uint64_t *)(bmf_plane_desc(n64_2 - 8)) = *(uint64_t *)&__frame.v179[n64_2 / 4];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 16)) = *(uint64_t *)&__frame.v178[n64_2 / 4];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_2 / 8 + 1];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_2 / 8];
            n64_2 -= 32;
          }
          while ( n64_2 );
        }
      }
    }
  }
  else
  {
    v45 = 1;
  }
LABEL_63:
  if ( !__frame.n4_10 && ::plane_count > 1 )
  {
    __frame.v178[0] = (uint8_t *)bmf_new(*((uint32_t *)__frame.Blockb + 3));
    memcpy(__frame.v178[0],(uint8_t *)__frame.Src,*((uint32_t *)__frame.Blockb + 3));
    n16_3 = 16;
    do
    {
      __frame.v179[n16_3 + 1] = plane_desc[n16_3 / 4].w12;
      __frame.v179[n16_3] = *(int32_t *)((uint8_t *)&::plane_count + n16_3 * 4);
      __frame.v178[n16_3 + 1] = (uint8_t *)plane_desc[n16_3 / 4].w4;
      __frame.v178[n16_3] = (uint8_t *)plane_desc[n16_3 / 4].w0;
      n16_3 -= 4;
    }
    while ( n16_3 * 4 );
    if ( ::plane_count > 0 )
    {
      n4_11 = 0;
      do
      {
        v85 = n4_11;   // a record index; it was 16 * it
        v86 = plane_desc[n4_11++ + 1].flags & 0xFB;
        plane_desc[v85 + 1].flags = v86;
      }
      while ( n4_11 < ::plane_count );
    }
    __transform_planes((BmfImage *)(uint16_t *)__frame.Blockb, v45, v82);
    n4_17 = (uint8_t *)(8 * (out_cursor - coded_buf));
    // never taken: -S
    __frame.v178[1] = n4_17;
    *(uint32_t *)packer_word = packer_acc;
    packer_free_bits = 0;
    packer_acc = 0;
    out_cursor = coded_buf;
    packer_word = (uint32_t *)coded_buf;
    hist_scratch = coded_buf + coded_size - 4096;
    if ( (int32_t)n4_17 > (int32_t)__frame.n4_15 )
    {
      n64_3 = 64;
      do
      {
        *(uint64_t *)(bmf_plane_desc(n64_3 - 8)) = *(uint64_t *)&__frame.v179[n64_3 / 4];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 16)) = *(uint64_t *)&__frame.v178[n64_3 / 4];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_3 / 8 + 1];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_3 / 8];
        n64_3 -= 32;
      }
      while ( n64_3 );
    }
    else
    {
      __frame.n4_15 = n4_17;
      v45 = 0;
    }
    if ( __frame.v189 + __frame.v188 )                // the left disjunct is -S, always true
    {
      n16_4 = 16;
      do
      {
        __frame.v179[n16_4 + 1] = plane_desc[n16_4 / 4].w12;
        __frame.v179[n16_4] = *(int32_t *)((uint8_t *)&::plane_count + n16_4 * 4);
        __frame.v178[n16_4 + 1] = (uint8_t *)plane_desc[n16_4 / 4].w4;
        __frame.v178[n16_4] = (uint8_t *)plane_desc[n16_4 / 4].w0;
        n16_4 -= 4;
      }
      while ( n16_4 * 4 );
      if ( ::plane_count > 0 )
      {
        n4_12 = 0;
        do
          plane_desc[n4_12++ + 1].flags = 0;
        while ( n4_12 < ::plane_count );
      }
      memcpy((uint8_t *)__frame.Src,__frame.v178[0],*((uint32_t *)__frame.Blockb + 3));
      __transform_planes((BmfImage *)(uint16_t *)__frame.Blockb, v45, v93);
      n4_18 = 8 * (out_cursor - coded_buf);
      // never taken: -S
      v162 = n4_18 <= (int32_t)__frame.n4_15;
      *(uint32_t *)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      hist_scratch = coded_buf + coded_size - 4096;
      if ( v162 )
      {
        v45 = 0;
      }
      else
      {
        n64_4 = 64;
        do
        {
          *(uint64_t *)(bmf_plane_desc(n64_4 - 8)) = *(uint64_t *)&__frame.v179[n64_4 / 4];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 16)) = *(uint64_t *)&__frame.v178[n64_4 / 4];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_4 / 8 + 1];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.v177))[n64_4 / 8];
          n64_4 -= 32;
        }
        while ( n64_4 );
      }
    }
    free(__frame.v178[0]);
  }
  free(coded_buf);
  free(__frame.Blockb);
  // `if ( opt_filter_template == 1 )` -- 38 lines that built the -T1 filter template into
  // __dword_4410A4.  With -T off nothing writes that word, so it keeps the 0
  // BMF.exe's data segment starts it at, and the -T2 reader above is gone too.
  return v45;
}


BmfArc *__bmf_open_archive(BmfArc *v2, char *FileName, int32_t a2)
{
  ;
  BmfArc *v5;
  FILE *Stream_v, *Stream_1;
  const char *a_b;   // an fopen mode string, so `char` and not a byte
  int32_t v8, v9;
  v5 = v2;
  // "a+b", as the original had it, and not "w+b".  An earlier pass here
  // reasoned that since the command line writes one image per run, appending
  // only meant that compressing twice to the same name grew the file instead
  // of replacing it.  That is what appending *is*: `bmf c a.bmp arc.bmf`
  // followed by `bmf c b.bmp arc.bmf` is how a multi-image archive is built,
  // and `bmf d` reads every member back -- it prints "number: 1", "number: 2"
  // and decodes both.  Opening "w+b" did not tidy a harness annoyance, it
  // removed half of a feature the format carries a flag for.
  //
  // Not "wb" either: the pass below reads the stream back, and it is not only
  // walking the images already in the file -- it also sets up state the writer
  // goes on to use.
  a_b = "a+b";
  if ( a2 )
    a_b = "rb";
  v2->images = 0;
  Stream_v = fopen(FileName, a_b);
  v5->fp = Stream_v;
  if ( !Stream_v )
    __exit_402E40(6, FileName);
  v5->images = 0;
  v8 = fseek(Stream_v, 0, 0);
  v9 = v5->fp != nullptr;
  if ( v8 )
  {
    if ( !v9 )
      __exit_402E40(3, FileName);
    return v5;
  }
  if ( !v9 )
    {
      __exit_402E40(3, FileName);
      return v5;
    }
  if ( !feof(v5->fp) )
  {
    __expand_image((uint8_t *)v5, 1, (void **)nullptr);
    Stream_1 = v5->fp;
    if ( !Stream_1 )
      {
        __exit_402E40(3, FileName);
        return v5;
      }
    if ( !feof(v5->fp) )
    {
      v5->images = 0;
      fseek(Stream_1, 0, 0);
      v9 = v5->fp != nullptr;
      {
        if ( !v9 )
        __exit_402E40(3, FileName);
        return v5;
      }
    }
  }
  return v5;
}


int32_t __compress_image(uint8_t *a1, BmfImage *p_i, void *coded_buf)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and the two-member archive fails to decompress.
  struct alignas(16) CompressImageFrame {   // 80 bytes, one stack frame
      uint8_t *Buffera_4;
      uint8_t _pad0[12];
      uint32_t ElementCount;
      union {
          // The 16-byte archive member header `fwrite` sends in one call, and
          // the scratch MSVC put in the same bytes afterwards.  The two live in
          // mutually exclusive branches: everything below `if ( v38 )` returns,
          // so the header is finished with before the deinterleave starts.
          BmfImage hdr;
          struct {
            uint8_t  _scratch0[4];
            int32_t  plane_n;    // +4   `plane_count`, the deinterleave stride
            uint8_t *row;        // +8   the source cursor, then the row index
            int32_t  row_step;   // +12  `plane_count * (height - 1)`
          };
      };
      int32_t v65;
      uint8_t *v66;
      void *Buffer_2;
      uint8_t _pad1[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80, "frame layout moved");
  // These shared `__frame.ElementCount` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t ElementCounta;
  uint32_t ElementCountb;
  // These shared `__frame.hdr[0]` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *Buffer_copy;
  ;
  uint32_t coded_len;   // word 1 of the coded block, its length
  uint8_t *v5;   // were int32_t: these hold addresses
  FILE *i;
  bool v38;
  int8_t v13, v21, v36;
  uint8_t v15;
  uint8_t __compress_image_Buffer_1;   // 0/1, shifted into bit 7 of the header byte
  uint8_t *Srca, *Buffera_2, *Buffera_3;   // `uint8_t *` beside the `char` scalars above
  uint32_t Buffera_5, Buffera_6;   // the header's pad/depth/flags word, not an address
  int32_t row_bytes, v18, n4_6, bits_left, n4, v27, n8_1, acc, n4_1, v40, v43,
          n4_3, v47, i_1, v50, n4_4, data_size;
  BmfImage *p_i_1;
  uint16_t i_2;
  uint32_t ElementCount_1, v25, v26, v28, v30, v32, Size, v55;
  uint8_t v39, *v49, v54;
  v5 = a1;
  if ( !((BmfArc *)a1)->fp )
    return 0;
  if ( !feof(((BmfArc *)a1)->fp) )
  {
    __expand_image(v5, 1, (void **)nullptr);
    for ( i = ((BmfArc *)v5)->fp; i; i = ((BmfArc *)v5)->fp )
    {
      if ( feof(((BmfArc *)v5)->fp) )
        break;
      if ( feof(i) )
        break;
      __expand_image(v5, 1, (void **)nullptr);
    }
  }
  __compress_image_Buffer_1 = (uint8_t)(uintptr_t)coded_buf;
  p_i_1 = (BmfImage *)(p_i);
  row_bytes = p_i->stride;
  if ( coded_buf )
    __compress_image_Buffer_1 = 1;
  __frame.hdr.width = p_i->width;
  __frame.hdr.height = p_i->height;
  p_i->flags |= __compress_image_Buffer_1 << 7;
  Buffera_5 = *(uint32_t *)&p_i->_pad8;
  __frame.hdr.stride = (uint32_t)row_bytes;
  plane_desc[0].w4 = 512;
  plane_desc[0].w12 = 0;
  *(uint32_t *)&__frame.hdr._pad8 = Buffera_5;
  __frame.hdr.data_size = (uint32_t)p_i->data_size;
  ::plane_count = ((p_i->depth & 0x3Fu) + 7) >> 3;
  if ( fwrite("\x81\x8A""20\x81\x90""20a+b", 4u, 1u, ((BmfArc *)v5)->fp) != 1 )
    return 0;
  v15 = p_i->depth;
  ++*(uint32_t *)v5;
  ElementCount_1 = v15 & 0x80;
  if ( (v15 & 0x80) != 0 )   // bit 7 is the palette flag, not a sign
    ElementCount_1 = 3 << (v15 & 31);
  if ( p_i->data_size < 0x10u )   // -N is on, so only the size decides
    goto LABEL_76;
  desc_slow_mode = 1;               // -S
  __frame.hdr.flags |= 0x24;        // -S in bit 2, and bit 5 always set
  if ( (p_i->depth & 0x3Fu) <= 4 )         // -F is on, so only the depth decides
  {
    coded_size = p_i->data_size + 0x20000;
    ::coded_buf = (uint8_t *)bmf_new(coded_size);
    ::packer_free_bits = 0;
    ::packer_acc = 0;
    out_cursor = ::coded_buf;
    ::packer_word = (uint32_t *)::coded_buf;
    hist_scratch = ::coded_buf + coded_size - 4096;
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    // always taken: -S
      __model_plane((BmfImage *)p_i, p_i->pixels, p_i->pixels);
    goto LABEL_57;
  }
  __frame.ElementCount = __search_filter((BmfImage *)p_i, v13);
  __frame.hdr.flags |= 0x10u;
  if ( (p_i->flags & 2) != 0 )
  {
    n4_6 = p_i->stride;
    Buffera_6 = *(uint32_t *)&p_i->_pad8;
    __frame.hdr.width = p_i->width;
    __frame.hdr.height = p_i->height;
    v18 = p_i->data_size;
    __frame.hdr.stride = (uint32_t)n4_6;
    *(uint32_t *)&__frame.hdr._pad8 = Buffera_6;
    __frame.hdr.data_size = (uint32_t)v18;
    __frame.hdr.flags = 0x34 | (uint8_t)(Buffera_6 >> 24);   // -S in bit 2
  }
  else
  {
    v18 = p_i->data_size;
  }
  coded_size = v18 + 0x20000;
  ::coded_buf = (uint8_t *)bmf_new(v18 + 0x20000);
  out_cursor = ::coded_buf;
  ::packer_free_bits = 0;
  ::packer_acc = 0;
  ::packer_word = (uint32_t *)::coded_buf;
  hist_scratch = ::coded_buf + coded_size - 4096;
  if ( ::plane_count == 1 )
  {
    if ( (p_i->depth & 0x40) != 0 )
    {
LABEL_22:
      // The 4-bit near-lossless field -- ALGORITHM.md §4.1's bit packer writing
      // -E into the header.  -E is 0, so the bits are zero and only the
      // packer's cursor moves; what is left is that cursor arithmetic.
      plane_desc[0].w12 = 0;
      if ( ::packer_free_bits < 4 )
      {
        *(uint32_t *)::packer_word = ::packer_acc;
        ::packer_word = (uint32_t *)out_cursor;
        ::packer_acc = 0;
        ::packer_free_bits += 28;
        out_cursor += 4;
      }
      else
      {
        ::packer_free_bits -= 4;
      }
    }
  }
  else if ( ::plane_count > 2 )
  {
    goto LABEL_22;
  }
  alphabet_reduced = 0;
  if ( ::plane_count > 0 )
  {
    bits_left = ::packer_free_bits;
    __frame.v66 = v5;
    n4 = 0;
    do
    {
      v25 = (4 * plane_desc[n4 + 1].flags) | plane_desc[n4 + 1].predictor;
      if ( bits_left < 6 )
      {
        *(uint32_t *)::packer_word = ::packer_acc | (2 * (v25 << ((31 - bits_left) & 31)));
        ::packer_word = (uint32_t *)out_cursor;
        out_cursor += 4;
        bits_left = ::packer_free_bits + 26;
        ::packer_acc = v25 >> (::packer_free_bits & 31);
      }
      else
      {
        ::packer_acc |= v25 << (-bits_left & 31);
        bits_left = ::packer_free_bits - 6;
      }
      ::packer_free_bits = bits_left;
      if ( (plane_desc[n4 + 1].flags & 8) != 0 )
      {
        v26 = plane_desc[n4 + 1].b3;
        if ( bits_left < 8 )
        {
          *(uint32_t *)::packer_word = ::packer_acc | (2 * (v26 << ((31 - bits_left) & 31)));
          ::packer_word = (uint32_t *)out_cursor;
          out_cursor += 4;
          bits_left = ::packer_free_bits + 24;
          ::packer_acc = v26 >> (::packer_free_bits & 31);
        }
        else
        {
          v27 = v26 << (-bits_left & 31);
          bits_left -= 8;
          ::packer_acc |= v27;
        }
        ::packer_free_bits = bits_left;
        if ( plane_desc[n4 + 1].predictor > 1u )
        {
          v28 = plane_desc[n4 + 1].w4 + 64;
          if ( bits_left < 8 )
          {
            *(uint32_t *)::packer_word = ::packer_acc | (2 * (v28 << ((31 - bits_left) & 31)));
            ::packer_word = (uint32_t *)out_cursor;
            out_cursor += 4;
            n8_1 = ::packer_free_bits + 24;
            ::packer_acc = v28 >> (::packer_free_bits & 31);
          }
          else
          {
            ::packer_acc |= v28 << (-bits_left & 31);
            n8_1 = ::packer_free_bits - 8;
          }
          ::packer_free_bits = n8_1;
          v30 = plane_desc[n4 + 1].w8 + 64;
          if ( n8_1 < 8 )
          {
            *(uint32_t *)::packer_word = ::packer_acc | (2 * (v30 << ((31 - n8_1) & 31)));
            ::packer_word = (uint32_t *)out_cursor;
            out_cursor += 4;
            bits_left = ::packer_free_bits + 24;
            ::packer_acc = v30 >> (::packer_free_bits & 31);
          }
          else
          {
            bits_left = n8_1 - 8;
            ::packer_acc |= (v30 << (-n8_1 & 31));
          }
          ::packer_free_bits = bits_left;
          if ( plane_desc[n4 + 1].predictor > 2u )
          {
            v32 = plane_desc[n4 + 1].w12 + 64;
            if ( bits_left < 8 )
            {
              *(uint32_t *)::packer_word = ::packer_acc | (2 * (v32 << ((31 - bits_left) & 31)));
              ::packer_word = (uint32_t *)out_cursor;
              out_cursor += 4;
              acc = v32 >> (::packer_free_bits & 31);
              bits_left = ::packer_free_bits + 24;
              ::packer_free_bits += 24;
              ::packer_acc = acc;
            }
            else
            {
              ::packer_acc |= v32 << (-bits_left & 31);
              bits_left = ::packer_free_bits - 8;
              ::packer_free_bits -= 8;
            }
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::plane_count );
    v5 = __frame.v66;
  }
  if ( __frame.ElementCount )
  {
    Size = p_i->width * p_i->height;
    __frame.hdr.flags |= 8u;
    Srca = (uint8_t *)bmf_new(Size);
    if ( ::plane_count > 0 )
    {
      __frame.v66 = v5;
      n4_1 = 0;
      do
        __model_planes((uint8_t *)p_i, Srca, plane_desc[n4_1++ + 1].src_plane, v36);
      while ( n4_1 < ::plane_count );
      v5 = __frame.v66;
    }
    free(Srca);
  }
  else
  {
    __transform_planes((BmfImage *)p_i, (int32_t)p_i, v21);
  }
LABEL_57:
  *(uint32_t *)::packer_word = ::packer_acc;
  v38 = (uint32_t)(out_cursor - (uint32_t)::coded_buf) < p_i->data_size;
  ElementCounta = out_cursor - ::coded_buf;
  __frame.hdr.data_size = (uint32_t)(out_cursor - ::coded_buf);
  if ( v38 )
  {
    v39 = fwrite(&__frame.hdr, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
    if ( coded_buf )
    {
      // Word 1 of the coded block is its length; the eight is the header in
      // front of it.
      coded_len = ((const uint32_t *)coded_buf)[1];
      v39 &= fwrite(coded_buf, 1u, coded_len + 8, ((BmfArc *)v5)->fp) == coded_len + 8;
    }
    v40 = (fwrite(::coded_buf, 1u, ElementCounta, ((BmfArc *)v5)->fp) == ElementCounta) & v39;
    free(::coded_buf);
    if ( v40 && (p_i->depth & 0x80) != 0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, ElementCount_1, ((BmfArc *)v5)->fp);
    fflush(((BmfArc *)v5)->fp);
    if ( v40 )
      return (int32_t)__frame.hdr.data_size;
    return v40;
  }
  free(::coded_buf);
  if ( (p_i->flags & 2) != 0 )
  {
    Buffer_copy = (uint8_t *)bmf_new(p_i->data_size);
    __frame.plane_n = ::plane_count;
    __frame.row = Buffer_copy;
    __frame.row_step = ::plane_count * (p_i->height - 1);
    __frame.Buffer_2 = (uint16_t *)p_i->pixels;
    memcpy(Buffer_copy,p_i->pixels,p_i->data_size);
    LOWORD(v43) = p_i->height;
    if ( (uint16_t)v43 )
    {
      n4_3 = __frame.plane_n;
      ElementCountb = ElementCount_1;
      Buffera_2 = __frame.row;
      Buffera_3 = nullptr;
      v47 = 0;
      __frame.v66 = v5;
      do
      {
        i_1 = p_i_1->width;
        __frame.v65 = v47;
        __frame.row = Buffera_3;
        v49 = (uint8_t *)p_i_1 + v47 + 16;
        v50 = __frame.row_step;
        do
        {
          n4_4 = n4_3;
          do
          {
            *v49++ = *Buffera_2++;
            --n4_4;
          }
          while ( n4_4 );
          v49 += v50;
          --i_1;
        }
        while ( i_1 );
        p_i_1 = (BmfImage *)(p_i);
        v43 = p_i->height;
        v47 = n4_3 + __frame.v65;
        Buffera_3 = __frame.row + 1;
      }
      while ( (int32_t)(__frame.row + 1) < v43 );
      ElementCount_1 = ElementCountb;
      v5 = __frame.v66;
    }
    __frame.Buffera_4 = Buffer_copy;
    i_2 = p_i_1->width;
    p_i_1->width = v43;
    p_i_1->height = i_2;
    p_i_1->flags ^= 2u;
    p_i_1->stride = (v43 * __frame.plane_n);
    free(__frame.Buffera_4);
    goto LABEL_77;
  }
LABEL_76:
  __frame.Buffer_2 = (uint16_t *)p_i->pixels;
LABEL_77:
  v54 = fwrite(p_i_1, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
  if ( coded_buf )
  {
    coded_len = ((const uint32_t *)coded_buf)[1];
    v54 &= fwrite(coded_buf, 1u, coded_len + 8, ((BmfArc *)v5)->fp) == coded_len + 8;
  }
  v55 = fwrite(__frame.Buffer_2, 1u, ElementCount_1 + p_i_1->data_size, ((BmfArc *)v5)->fp);
  data_size = p_i_1->data_size;
  if ( (v54 & (v55 == data_size + ElementCount_1)) == 0 )
    return 0;
  return data_size;
}

// ---------------------------------------------------------------------------
// The two things this program does.
//
// BMF's own driver, sub_4015C0, sniffed the first four bytes of its argument
// to choose between six readers, derived the output name from the input's by
// swapping the extension, and picked the writer from a switch.  The command
// line is now
//
//     bmf c input.bmp output      compress a BMP into a BMF stream
//     bmf d input output.bmp      expand a BMF stream back into a BMP
//
// which names both files and fixes both formats, so what is left is one
// reader and one writer.  Everything the other five readers and the other two
// writers reached went with them; so did the wildcard walk, the .ini, the
// switch parser, the -O output name, the -D delete, the unique-name counter
// and the stored-file member handling.
// ---------------------------------------------------------------------------

// Compress InName, which must be a BMP, into a BMF stream named OutName.
void __bmf_compress(
                            const char *InName, const char *OutName)
{
  ;
  FILE *Stream_v;
  const uint8_t *Palette;
  int32_t *p_i, Arc, Flags, Colours, Step, Grey, i;
  uint32_t Size;

  // The reader below answers "no" the same way whether the file is missing or
  // is not a BMP, and BMF told those apart -- so open it once first, as its
  // format sniffer used to, and keep the two messages.
  Stream_v = fopen(InName, "rb");
  if ( !Stream_v )
    __exit_402E40(6, InName);
  fclose(Stream_v);
  p_i = __read_bmp((char *)InName);
  if ( !p_i )
    __exit_402E40(4);
  BmfImage *const p_i_img = (BmfImage *)p_i;
  printf(
    "File %16s, image %dx%dx%d, size - %d:",
    InName,
    p_i_img->width,
    p_i_img->height,
    p_i_img->depth & 0x3F,
    p_i_img->data_size);
  if ( void *__nb = bmf_new(sizeof(BmfArc)) )
    Arc = (int32_t)__bmf_open_archive((BmfArc *)__nb, (char *)OutName, 0);
  else
    Arc = 0;

  // A palette that is nothing but a grey ramp carries no information: drop it
  // (bit 0x80) and mark the image greyscale (bit 0x40) instead.  This is the
  // donor's, unchanged -- it decides what goes into the stream.
  Flags = p_i_img->depth;
  if ( (Flags & 0x80) != 0 )
  {
    if ( (Flags & 0x40) != 0 )
    {
      p_i_img->depth = Flags ^ 0x80;
    }
    else
    {
      Colours = 1 << (Flags & 31);
      Step = 0x100u >> (Flags & 31);
      Palette = (const uint8_t *)p_i + p_i_img->data_size + 16;
      Grey = 0;
      for ( i = 0; i < Colours; ++i )
      {
        if ( Palette[3 * i] != Grey || Palette[3 * i + 1] != Grey || Palette[3 * i + 2] != Grey )
          break;
        Grey += Step;
      }
      if ( i >= Colours )
        p_i_img->depth = (Flags | 0x40) ^ 0x80;
    }
  }

  Size = __compress_image((uint8_t *)Arc, (BmfImage *)(uint16_t *)p_i, (void *)coded_block);
  if ( !Size )
    __exit_402E40(5, OutName);
  printf(
    "%6.3f bpp\n",
    (double)Size * 8.0 / (double)(p_i_img->height * p_i_img->width));
  free(p_i);
}

// Expand the BMF stream InName into a BMP named OutName.
void __bmf_decompress(
                              const char *InName, const char *OutName)
{
  ;
  int32_t Number, Depth;
  uint32_t *p_i;
  void *Block;

  if ( void *__nb = bmf_new(sizeof(BmfArc)) )
    Block = (void *)__bmf_open_archive((BmfArc *)__nb, (char *)InName, 1);
  else
    Block = nullptr;
  printf("File %16s,\r", InName);
  Number = 0;
  while ( 1 )
  {
    p_i = (uint32_t *)__expand_image((uint8_t *)Block, 0, &coded_block);
    BmfImage *const p_i_img = (BmfImage *)p_i;
    if ( !p_i )
    {
      printf("\n");
      if ( !((BmfArc *)Block)->fp )
        __exit_402E40(3, InName);
      __bmf_destroy_archive((BmfArc *)(FILE **)Block, 1);
      return;
    }
    ++Number;
    printf(
      "File %16s, image %dx%dx%d, size - %d, number: %d\r",
      InName,
      p_i_img->width,
      p_i_img->height,
      p_i_img->depth & 0x3F,
      p_i_img->data_size,
      Number);
    // BMF sent 2, 15 and 16 bits per pixel to its TGA writer, because a BMP
    // cannot hold them.  The output is a BMP now, so those streams have no
    // answer here and saying so beats writing a file that is not one.
    Depth = p_i_img->depth & 0x3F;
    if ( Depth == 2 || Depth == 15 || Depth == 16 )
    {
      printf("\n%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
      exit(5);
    }
    if ( !__write_bmp((int32_t)p_i, (char *)OutName, 1) )
      __exit_402E40(5, OutName);
    free(coded_block);
    coded_block = nullptr;
    free(p_i);
  }
}

int32_t __main(int32_t argc, const char **argv)
{
  ;
  int32_t Mode;

  bmf_set_denormal_mode();
  __set_new_handler(__out_of_memory_handler);
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");

  Mode = argc == 4 && !argv[1][1] ? toupper(argv[1][0]) : 0;
  if ( Mode != 'C' && Mode != 'D' )
  {
    printf(
      "e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
      "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
      "       bmf d input output.bmp     expand\n");
    return 1;
  }

  if ( Mode == 'C' )
    __bmf_compress( argv[2], argv[3]);
  else
    __bmf_decompress( argv[2], argv[3]);
  return 0;
}
void __out_of_memory_handler() { __exit_402E40(7); }
int32_t main(int32_t argc, uint8_t **argv) {
  return __main(argc, (const char **)argv);
}
