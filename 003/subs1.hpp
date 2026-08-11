// ---------------------------------------------------------------------------
// BMF's globals.

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

// The typedefs carry the array shapes, which cannot be written inline in the
// reference declaration.  Where one body reads an address as a bare scalar and
// another indexes it, the array shape is the one declared and the scalar's
// users say [0].

// Nothing below this block writes an address as a number any more.  Hex-Rays
// left some baked into expressions rather than into named globals -- the
// `*(_QWORD *)(n64 + 4469652)` shape, 4469652 being 0x00443394 -- and those now
// take the address of the global that owns the byte they start at.  Where that
// byte is inside an object rather than at its start, the offset is written out
// rather than given a global of its own; the exception is 0x0044337D, byte 1 of
// the dword array based at coded_buf, which is indexed like an array and so got
// a byte global of its own.

// A few functions declare a local with the same name as the global they use
// and reach the global through `::`; those locals still carry their original
// `__sub_XXXXXX_` names, which is now what tells the two apart.

// incdec.md §6.1 is why the addresses are still the names: giving these
// objects real names is a much larger job than moving them, because the same
// address is an int to one function and a char[] to the next.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------
// BMF.exe's data segment, one definition per global.

// Each carries the bytes it had, and 64 bytes of guard after it so
// that a global running past its own end lands in padding rather than
// in its neighbour.  The extent is the distance to the next global:
// the [0x10000] bounds Hex-Rays emitted are guesses and there is no
// way to tell from the source which are real.  REFACTORING.md §4.1.
// ---------------------------------------------------------------
// BMF's .bss, the last of its data segment that is still one object.

// Every global below is a reference into it at its original offset, which is
// what `blob1` used to be -- but this is 19 584 bytes of zeroes rather than a
// generated copy of the whole data segment, because that is all the surviving
// globals need.  Two measurements say so:

//   * 0x44294C..0x448000 is one unbroken run of zero bytes in BMF.exe's data
//     segment, and all of these globals are inside it.  There are no
//     initialisers to recover: this is bss.
//   * Everything below it is dead.  Filling 43 184 bytes of blob.inc with 0xCC
//     -- the whole segment under 0x44294C, bar the relocation slots -- leaves
//     all fifteen streams byte-identical.  The string tables down there belong
//     to the modes that are gone.

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

//     _this[14][1] = 169.2f;                                 // [ 0]
//     v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;          // [ 1] [ 7] [ 6]
//     const float floor_a = 26896.0f * f278656[14][2];       // [ 9]
//     ... / (f278656[7 + j][k] + ms_scale * 529.0f);         // [12]

// MSVC pooled the literals into .rdata and loaded them from here; Hex-Rays put
// them back inline, so nothing reads the pool as floats any more.  Only
// [ 2] -2784.44f and [ 8] 2.6f have no surviving literal, which is what the
// compression mode being a constant leaves behind -- the arithmetic that used
// them is in a branch this build folds away.

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
// How much a p1 counter goes up, by level.  `ctx_of` indexes it with
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

//   * the subscripts the code already writes -- `plane_desc[plane + 1].flags`,
//     `plane_desc[plane + 1].w4` -- both step whole records;
//   * field +1 is reached from two origins one record apart, `transform_planes`
//     pre-incrementing from 0 into `plane_desc[n].w0` and `rc_begin_encode`
//     indexing `plane_desc[p + 1].src_plane` by plane, which is what a header entry
//     in front of an array looks like;
//   * and `alt_model_p2_encode` walks all four plane records in one loop using
//     four of the record-0 names as its field bases.

// So `plane_count`, read as a scalar throughout, is field +8 of record 0.

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

// All of it is dead, and measured to be.  Taking both relocation calls out
// leaves every one of the fifteen streams byte-identical, because the 39
// pointers they rebased all point into the string tables under 0x44294C --
// which the poisoning experiment in REFACTORING2.md §3.1 shows nothing reads.
// They belong to the modes that are gone (REFACTORING.md §2.1).
// ---------------------------------------------------------------------------
// The compression mode.

// BMF read these six from its .ini and then from the command line; this program
// has one mode and always did -- `bmf c` pinned -S and -Q9 and let the other
// four keep the values BMF.exe's data segment starts them at.  They are
// constants here instead of blob words, so the compiler folds the branches that
// test them and everything only the other modes could reach becomes
// unreachable rather than merely unreached.  See REFACTORING.md §2.

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
// 0x44573C were the elements it was stepping onto.  `init_tables` reads
// elements 1..3 as bare symbol values.  What the index means is not
// established, so the name still records the address rather than a role.
static int32_t mode_symbol[5];

// The four plane records, `plane_desc[1]` onwards, as a byte cursor.

// Six places copy all four records in or out with 64-bit moves, and Hex-Rays
// wrote those as offsets from four *other* globals -- coded_buf,
// desc_slow_mode, and the two dwords that are now `plane_desc[0].w0` and
// `plane_desc[0].w8` -- which sit 32, 24, 16 and 8 bytes below the records.
// That is the original compiler's strength reduction showing through, not
// something the program means; those four are a buffer pointer, a mode flag, a
// counter and a plane count, and none of them is a base for this.  Written
// against the records themselves the arithmetic is the same and the dependency
// on where four unrelated globals sit is gone.  REFACTORING.md §4.1.

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

// BMF's entropy coder, as it implements it: a carry-counting range coder over
// a 31-bit `low`, renormalised a byte at a time, in the Subbotin lineage.
// ALGORITHM.md §5 describes it in prose; this is the same thing as code.

// The state used to be six globals in the block above -- __n0x800000,
// __n0x7F800000, __dword_4456E8, __dword_4456EC, __byte_4456F0 -- and the
// arguments of a coding step three more, __n0x2000_1 / __n0x2000_0 /
// __n0x2000, which every caller assigned before calling an entry that took no
// parameters.  Both sets are members here, and the entries take arguments.

// It lives in this file rather than in bmf.cpp because it shares its output
// cursor with the bit packer: out_cursor is the one position both advance
// through, and that is a blob global declared above.

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

// `alt_p2_alloc` fills its five groups with

//   0  64  128     0  192  384     0  576  1152     0  1728  3456     0  5184  10368

// -- 0, 64*3^g and 128*3^g -- so the five selectors are the digits of a base-3
// number scaled by 64: 3^5 = 243 contexts, 64 counters apart.

// The encoder adds `w[sel]` and the decoder `w[2 - sel]`; the two are the same
// slot only when `sel` is 1, so the direction is each side's, not a symmetry.
struct CtxWeight {
  int32_t  sel;
  uint32_t w[3];
};
static_assert(sizeof(CtxWeight) == 16, "CtxWeight: a selector and three weights");

// The two runs of five are arrays, and the code says so rather than the
// offsets: alt_model_p1_decode rotates `row` by one place every pass --

//   v25 = row[4]; v26 = row[3]; v27 = row[2]; v28 = row[1]; v29 = row[0];
//   row[4] = v26; row[3] = v27; row[2] = v28; row[1] = v29; row[0] = v25;

// a five-deep ring of row pointers -- and then derives `cur` from it, three of
// the five offset by eight.  Naming ten consecutive `uint32_t` f176 through
// cursor[4] hid the one fact about them worth having.  The layout is unchanged:
// `row[0]` is still +176, and the static_assert says so.

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
  // that `ctx_of` writes five of them through `*(uint8_t **)&f0[k]`,
  // which is the pointer costume the rest of that function wears.
  // The plane's geometry, the prediction the context step left behind, and the
  // five context words the model assembles.  `pred` is what `ctx_of`
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
  // `p1_slot_edges`, times eight.  `ctx_of` reads all three with the
  // same symbol and folds them into `ctx[0]` and `ctx[1]`.
  uint8_t level_of[512];     // +216  .. +727
  uint8_t slot_of[256];     // +728  .. +983
  // A pair of inverse maps, the same pair in both blocks -- `alt_init_tables`
  // is handed `(fold, unfold)` by each allocator.
  
  // `unfold` is built as 0, -1, 1, -2, 2, ... : `unfold[0] = 0`,
  // `unfold[4i + 1] = -(2i + 1)`, `unfold[4i + 2] = 2i + 1`, and the same one
  // further out for the even pair, ending at `unfold[253] = -127`,
  // `unfold[254] = 127`.  That is the zigzag that turns a signed residual into
  // a non-negative symbol, and `fold` is the way back: the coders read
  // `n = fold[residual]`, code `n`, and reconstruct with `unfold[n] +
  // predictor`.  Neither table's name says more than that pairing.
  
  // `fold_hi` is the 256 bytes after `fold`, and the two are filled by one
  // call: a sentinel written over `fold_hi` immediately before
  // `alt_init_tables` is gone immediately after, at both ends of the range and
  // in both blocks.  So the write extent is 512 bytes and the split is the
  // readers' -- some index `fold`, some `fold_hi`, none crosses.
  
  // What separates the two index spaces was left open here for several rounds.
  // It is nothing: both halves are byte-indexed maps of the same residual to
  // the same code, and in this build they hold the same 256 bytes.  Reading
  // `alt_init_tables` says why -- the quantiser walk fills `fold[0..255]` and
  // the closing loop fills `fold_hi[b] = code` by inverting `unfold` outright,
  // and with `-E` at 0 the quantiser's buckets are one residual wide, so the
  // walk computes that same inverse the long way round.  Measured rather than
  // argued: a probe comparing the two halves at the end of every
  // `alt_init_tables` call reports 0 of 256 differing, on every test stream.
  
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
  int32_t ctx_of(AltP1Block *nb0, AltP1Block *nb1);
  void d8_encode_body(uint8_t *src, uint8_t *out);
  int32_t update_model();
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

// Recovered from 174 dereferences over 25 offsets, under 5
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
// Sixteen bytes: eight words, of which the last is only ever two bytes.  The
// union is not a choice between two readings -- word 7 is touched as `+14` and
// `+15` and never as a word, and the two spellings sit on the same bytes
// because the code uses both.

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
// and fills them with (symbol, 1); `add_weight` promotes one entry
// towards the front and halves the counts when they run out.  The header is 24
// bytes, which is what every caller's `+ 24 * k` says.
#pragma pack(push, 1)
struct SymEntry {
  uint16_t sym;
  uint8_t  cnt;
  // Both fields at once.  The symbol lists write them in that order nineteen
  // times and never one without the other: MSVC stored the pair with two
  // `mov`s and Hex-Rays wrote down two statements.  Still an aggregate, still
  // three bytes -- the assertion below is what says so.
  void set(uint32_t s, uint32_t c) { sym = (uint16_t)s; cnt = (uint8_t)c; }
};
#pragma pack(pop)
static_assert(sizeof(SymEntry) == 3, "SymEntry: the record is three bytes");

struct SymList {
  // Unsigned because `add_weight` read them through a `uint32_t *` and
  // every one of them counts something; `init_symbol_list` took the same bytes
  // as `int32_t *` and its comparisons are unsigned either way.
  uint32_t  n;        // +0   the alphabet size
  uint32_t  live;     // +4   entries in use
  // The three numbers the list's own rescale runs on.  `tot` is the sum of
  // every entry's count -- it is what the range coder is handed as its total,
  // and `add_weight` adds to it on every new entry and adds back one
  // for each zero-count entry it prunes.  `since_rescale` accumulates the
  // weight each update carries (`a3`, plus 4 for a new entry) and `rescale_at`
  // is the ceiling it is tested against; when it is passed, every count is
  // halved and so are these two.  `rescale_at` is `8 * n` or `20 * n` at init,
  // and which of the two also decides whether the halving rounds up.
  uint32_t  tot;           // +8
  uint32_t  since_rescale; // +12  12 * n at init
  uint32_t  rescale_at;    // +16  8 * n at init
  SymEntry *ent;      // +20

  // Halve every count, re-sort what the halving moved, and drop the tail that
  // reached zero.  Returns whether it ran.
  //
  // `code_symbol` and `add_weight` both had this inline, in
  // fifty-three lines that differed in one identifier -- the local holding the
  // count that triggers it.  Both walked from `head`, and `head` is `ent` at
  // both sites and can only be: each body assigns it three times and always
  // from this member.
  //
  // The bias is what `rescale_at` decides.  A list initialised at `8 * n`
  // halves rounding down and one at `20 * n` rounds up, which is the whole of
  // `bias = due < 20 * n`.
  bool rescale(int32_t count);
  int32_t code_symbol(int32_t want);
  void add_weight(int32_t want, uint32_t add);
};
static_assert(sizeof(SymList) == 24, "SymList: the header is 24 bytes");

inline bool SymList::rescale(int32_t count)
{
  int32_t half, bias, up_cnt, back_cnt, last_cnt;
  uint32_t n_left, running, since, due = rescale_at;
  uint16_t keep;
  SymEntry *cur, *prev, *up, *back;
  if ( count <= 251 && due >= since_rescale )
    return false;
  n_left = live;
  bias = due < 20 * n;
  cur = ent - 1;
  prev = cur;
  do
  {
    prev = cur;
    ++cur;
    half = (bias + (uint32_t)cur->cnt) >> 1;
    cur->cnt = half;
    if ( cur != ent )
    {
      up = cur - 1;
      up_cnt = up->cnt;
      if ( half > up_cnt )
      {
        keep = cur->sym;
        cur->set(up->sym, up_cnt);
        if ( up != ent )
        {
          do
          {
            back = up - 1;
            back_cnt = back->cnt;
            if ( half <= back_cnt )
              break;
            up->set(back->sym, back_cnt);
            --up;
          }
          while ( back != ent );
        }
        up->set(keep, half);
      }
    }
    --n_left;
  }
  while ( n_left );
  running = tot;
  if ( !cur->cnt )
  {
    do
    {
      ++n_left;
      tot = ++running;
      last_cnt = prev->cnt;
      --prev;
    }
    while ( !last_cnt );
    live -= n_left;
  }
  since = since_rescale;
  tot = running - (running >> 1);
  since_rescale = since - (since >> 1);
  return true;
}

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

// Not a counter pair, whatever its two `uint16_t` and its 0x2000 seeds
// suggest -- and this comment said it was one until the seeds were read
// against what uses them.  It is the two most recent symbols seen in one
// context.  `layout_workspace` writes 0x10000 of them per context group;
// `init_tables` writes `pix_cur[1] = pix_cur[0]` and then
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

// The counter pair `SymPair` is not.  One node of the binary tree the symbol
// coders walk below level 1: a count for the bit being 0 and a count for it
// being 1, indexed by the bit itself, which is why they are an array and not
// two names.
struct FreqPair {
  uint16_t f[2];
};
static_assert(sizeof(FreqPair) == 4, "FreqPair: two counts, four bytes");

// Where one level's tree of those starts inside a counter block.

// Three functions walk this tree -- `encode_symbol_tree`, `decode_symbol_tree`
// and `update_binary_pair` -- and each reached it by a different arithmetic:
// `(uint8_t *)&freq[2 * tbl_base + 8] + 4 * (span + node)`, `freq + 2 *
// tbl_base + 2 * span + 2 * node + 8`, and a byte pointer laundered through an
// `int32_t`.  All three are this address; only the third was a raw offset that
// any measure could see, which is why the other two survived four rounds.

// The tree is 1-indexed, and the `+ 8` rather than `+ 10` is how it says so.
// The block's header is ten `uint16_t` -- a total, an escape weight and eight
// fixed counts, which is what `rc_begin_encode` seeds -- so the pairs start at
// byte 20 and element 0 of this array is the header's last two words.  No walk
// reaches it: all three start at `span = 1`, and the first pair they touch is
// element 1.
static inline FreqPair *bit_tree(uint16_t *freq, int32_t lvl)
{
  return (FreqPair *)&freq[2 * level_geom[lvl].tbl_base + 8];
}

// One adaptive binary counter: a count per bit value, and the total at which
// the pair is rescaled.  `layout_workspace` seeds them (40, 16, 512) or
// (4, 4, 72), and the four tables that hold them are all indexed `3 * k`.

// `encode_context_bit` codes with `rc.encode_bit(n[0], n[1], bit)`, adds 8 to
// `n[bit]`, and when `n[0] + n[1]` passes `limit` halves both and raises
// `limit` by 64 -- to a ceiling of 0x4000, so the counter forgets faster while
// it is young and settles as it ages.

// A record has three states, and the first two are why `n` is an array.  Cold
// is `n[0] == 0`: the coder falls back on the second counter it was handed,
// codes from that, and leaves `n[0] = bit + 1`, so `n[0]` is *the first bit
// seen plus one* and not a count at all.  Half-warm is `n[1] == 0`: `n[0]` and
// `n[1]` are seeded from the fallback's ratio scaled to 64, `limit` is set to
// 512, and `n[n[0] - 1] += 4` bumps the bit that cold state remembered.  Live
// is everything after.

// That reach only lands inside the record while `n[0]` is 1 or 2, which the
// seeds actively contradict -- 40 and 4.  It holds because it is reachable
// only from half-warm: a `__builtin_trap()` on `n[0] > 2` there fires on none
// of the fifteen images.
struct BitCtr {
  uint16_t n[2];    // +0 .. +3
  uint16_t limit;   // +4
  int32_t encode_context_bit(BitCtr *a2, int32_t bit);
  int32_t decode_context_bit(BitCtr *a2);
};
static_assert(sizeof(BitCtr) == 6, "BitCtr: two counts and a rescale limit");
static_assert(__builtin_offsetof(BitCtr, limit) == 4, "BitCtr: the limit is last");


// The pixel model's per-pixel record, and the unit every pointer in
// `ModelBlock::row_cur` steps.

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
  // `init_tables` reads it to decide whether the symbol lists get the
  // match promoted into them.  It is the coder's answer to "was this
  // candidate the pixel", carried across a call rather than returned.
  uint32_t hit;
  // Write-only, like the three in `AltP2Block`: both coders store the
  // neighbourhood word here once per pixel and nothing reads it back, and
  // no computed reach lands on +36.  It stays because the layout does.
  uint32_t ctx_state_seen;   // +36
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
  // beside it: `init_tables` initialises `_this + 1078184` and
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
  
  // `ctx_id3` has a ceiling: past 53248 ids the index is or-ed with 15, which
  // collapses every further context into the last slot of its group rather
  // than growing the table.
  uint16_t ctx_id1[192512];   // +6075824 .. +6460847
  uint16_t ctx_id2[108800];   // +6460848 .. +6678447
  uint16_t ctx_id3[712000];   // +6678448 .. +8102447
  int32_t pixel_context(uint32_t *nb);
  int32_t init_tables();
  void expand_alphabet();
  void unmodel_plane_slow(uint8_t *dst);
  int32_t decode_pixel(int32_t x);
  int32_t code_pixel(int32_t x);
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

// `encode_symbol` reads it as a three-way alphabet -- `rc.encode` gets
// a cumulative pair out of `f[0 .. 2]` and `f[0] + f[1] + f[2]` as the total --
// and ends with `*chosen = step + *chosen`.  So `step` is not a fourth count:
// it is the amount an update adds, and the rescale that fires when a count
// passes 0x4000 halves the three frequencies and *lowers* `step` -- by half
// above 256, by 32 above 32, by 2 or 0 below that.  A learning rate that
// decays as the record matures.

// Both halves of that are checked rather than read off: a `__builtin_trap()`
// on the addend differing from `step`, and one on the halving branch failing
// to lower it, fire on none of the fifteen images.
struct P2Freq {
  uint16_t step;   // +0
  uint16_t f[3];   // +2 .. +7
  uint32_t rescale_three_way();
  int32_t encode_symbol(const uint32_t *ctx_pair, int32_t sym);
  int32_t decode_symbol(const uint32_t *ctx_pair);
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

// `rate` is `int32_t` and not `int8_t` because a third of the sites reach b0
// through a widened temporary; `-Wsign-conversion` is what checks that none of
// them passes an unsigned value, which would have made `>>` a logical shift.
static inline int32_t p2_pred(int32_t w2, int32_t rate) {
  return (w2 + (1 << ((rate + 31) & 31))) >> (rate & 31);
}

// The counter update, and the file's most repeated expression: 117 sites.

// `err` is the prediction error.  A dead zone around zero contributes +-32 --
// `deadzone_hi` and `deadzone_lo` are the edges -- and the rest is the error
// scaled down by `shift`, with the rounding term `1 << (shift - 1)` that MSVC
// emitted as the constant 2 or 4.

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

// The last two bytes are not a ninth lane.  Both pixel bodies end a record
// with `cursor[17] = 2` and `cursor[16] = (lane[2] <= 0) + (lane[2] < 0)`, and
// `alt_p2_model` overwrites them with `abs32(err)` and a comparison.

// `mag` is read 32 times, always summed over a neighbourhood.  `sign` is read
// exactly once, and that once is the thing worth having: `alt_p2_context`
// assigns it to `ctx_w[4].sel`, so the three-way sign of a record's gradient
// *is* the fifth base-3 digit of the next context (§9.1).  Hex-Rays had that
// read as `(uint8_t)lane[8]` -- one past the eight lanes it then had -- which
// is how it stayed hidden while the record claimed nine of them.

// What the eight lanes hold is what the decoder writes as it emits a pixel:

//   val     = pixel * 16
//   dval    = the same, and zeroed again at the start of the next row
//   err     = val - the previous record's val, signed
//   aerr = dup = dupleft = dupright = |err|
//   dleft   = |err| / 2
//   sign    = the three-way sign of `err`, 0 / 1 / 2
//   mag     = 2

// so a fresh record is a pixel, its gradient, and five copies of that
// gradient's magnitude that then diverge -- `alt_p2_model` writes `dval`
// through `dupright` separately afterwards.  Five is `bank_ctx`'s five and the five planes' five;
// whether those threes are the same five is not established.
struct P2Ctx {
  // These were `lane[0]` through `lane[7]`.  `alt_p2_model`'s prologue writes
  // seven of them in a row and that is where the names come from: the sample,
  // the residual against the running prediction, its magnitude, and the
  // magnitudes of the differences against the left, above, above-left and
  // above-right neighbours.  `alt_p2_context`'s four weighted sums corroborate
  // it from the other side -- seven taps each over lanes 4, 5, 6 and 7, one
  // sum per direction, which is only a sensible thing to build if those four
  // lanes are the four directions.
  
  // The array spelling is gone: all 506 reaches named a literal lane.
  int16_t val;       // +0   lane[0]  16 * the sample
  int16_t dval;      // +2   lane[1]  `val` less whatever this slot held
  int16_t err;       // +4   lane[2]  `val` - the running prediction, signed
  int16_t aerr;      // +6   lane[3]  |err|
  int16_t dleft;     // +8   lane[4]  |val - left|
  int16_t dup;       // +10  lane[5]  |val - above|
  int16_t dupleft;   // +12  lane[6]  |val - above-left|
  int16_t dupright;  // +14  lane[7]  |val - above-right|
  uint8_t sign;      // +16
  uint8_t mag;       // +17
};
static_assert(sizeof(P2Ctx) == 18, "P2Ctx: the record is eighteen bytes");
static_assert(__builtin_offsetof(P2Ctx, dupright) == 14, "P2Ctx: lane 7 is dupright");
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
      // The same fifteen bytes as `p2_row[0][0]`..`[0][2]` and the low three
      // of `[0][3]`, which every one of the four p2 bodies clears at a row
      // start.  Hex-Rays had it as four scalars because that is how MSVC
      // writes fifteen bytes -- an 8, a 4, a 2 and a 1 -- and called them
      // readerless, which is true of the names and false of the bytes: the
      // union's other arm is what the filter reads.
      uint8_t p2_row0_head[15];
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
      int32_t _unused278696;   // no reader, and no writer either
      // The filter's last prediction.  `alt_p2_context` leaves its output
      // here and `alt_p2_model` reads it one pixel later as
      // `err = sample - pred_prev`, which is the residual the record
      // carries and the sign of which is one of the five context digits.
      uint16_t pred_prev;
      uint8_t _u0_0_12[2];
      uint32_t ctx;
      // Two indices derived from `ctx`, one per parity of a bit the coders
      // are handed alongside the record: `encode_symbol` reads
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
  
  // `encode_symbol` is handed `&freq[ctx]`, `ctx` being that
  // index, and `alt_p2_model` updates records `k - 1`, `k` and `k + 1` --
  // three adjacent, the way the p1 model does.
  
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
  FILE *fp;
  fp = _this->fp;
  if ( fp )
  {
    fseek(fp, 0, 2);
    return (FILE *)fclose(_this->fp);
  }
  return fp;
}

BmfArc *__bmf_destroy_archive(BmfArc *arc, int8_t do_free)
{
  ;
  __bmf_close_archive((BmfArc *)arc);
  if ( (do_free & 1) != 0 )
    free(arc);
  return arc;
}

void __expand_predictor_mode0(uint32_t unread_src, int32_t i, int32_t unread_h)
{
  ;
  // never taken: -E is 0
}

// MED, the LOCO-I / JPEG-LS median edge predictor, applied to a whole plane in
// place: every byte is replaced by its residual, folded to an unsigned code.

// It runs *backwards*, from the last pixel to the first, which is what lets it
// work in place -- each pixel's predictors are up and to the left, so they are
// still the originals when it reaches them.  That also fixes the shape of the
// loops: the last row is the general case, the first column of each row has no
// west neighbour, and the first row has no north one, so those two are peeled
// out rather than guarded.

// It also counts every code it writes into `hist_scratch`, and nothing in this
// build reads that count back: `hist_scratch` is a scratch region that
// `model_planes` immediately reuses as `__model_planes_buf`.  The histogram was
// for a cost estimate that this build's constant compression mode does not
// reach -- as is `predict_med` itself, whose two call sites are both under
// `plane_predictor == 1`, which `-E` being 0 makes unreachable.  `unpredict_med`
// is not: a decoder still has to read streams an encoder with `-E` produced,
// which is what `testfiles/med32.bmp` exists to exercise.

// So this body is here to be *read*, as the definition of what the inverse
// undoes, and the naming below is worth more than the code is.
uint32_t __predict_med(uint8_t *pixels, int32_t width, int32_t height)
{
  ;
  uint8_t left;
  uint32_t done;
  int32_t rows_left, x_left, north, pred, northwest, code, pairs, ofs;
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
  if ( height != 1 )
  {
   do
   {
    x_left = width - 1;
    if ( width != 1 )
    {
     do
     {
      // `up` trails `p` by exactly one row and the two step together, so
      // these three are the neighbourhood of the pixel at `*p`:
      
      //     northwest  north
      //     pred(west) *p
      
      // `pred` starts as the west neighbour because two of MED's three
      // outcomes are `west` or `north` unchanged, and west is the more common.
      north = (uint8_t)*--up;
      pred = (uint8_t)*(--p - 1);
      northwest = (uint8_t)p[-width - 1];
      // MED: northwest above both neighbours predicts the smaller, below both
      // predicts the larger, between them predicts the plane through all three.
      // An edge through the corner therefore predicts across it, not along it.
      // MED as one shape rather than four jumps: northwest outside the
      // interval [min(west, north), max(west, north)] predicts the far end of
      // it, northwest inside predicts the plane through all three.  The two
      // arms differ only in which way round the comparisons go, because which
      // of west and north is the min depends on the arm.
      if ( pred < north )
      {
        if ( northwest < pred )
          LOBYTE(pred) = (uint8_t)north;
        else if ( northwest <= north )
          LOBYTE(pred) = (uint8_t)(north + pred - northwest);
      }
      else
      {
        if ( northwest > pred )
          LOBYTE(pred) = (uint8_t)north;
        else if ( northwest >= north )
          LOBYTE(pred) = (uint8_t)(north + pred - northwest);
      }
      code = (uint8_t)fold[(uint8_t)(*p - pred)];
      *p = code;
      ++*(uint32_t *)&hist_scratch[4 * code];
      --x_left;
     }
     while ( x_left );
    }
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
  }
  // The first row, which has no north neighbour: predict each pixel from the
  // one to its left.  MSVC unrolled it two pixels at a time, so `left` is the
  // one value both halves of a pair need and `ofs` walks back in twos; `done`
  // is how far the unrolled part got, and the `if` below is the odd pixel it
  // leaves when the row is an even number wide.
  
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
      while ( k < (uint32_t)pairs );
      done = 2 * k + 1;
    }
    else
    {
      done = 1;
    }
    if ( last > (done - 1) )
    {
      q = (&p[-done]);
      last = (uint8_t)(*q - *((int8_t *)q - 1));
      *q = fold[(uint8_t)last];
    }
  }
  // Whatever code was written last, which is not a result: both call sites
  // discard it.  `-Wmaybe-uninitialized` flags it, correctly -- a 1x1 plane
  // reaches no loop and returns the stack -- and it is left as the
  // transcription rather than seeded, because seeding it would invent a value
  // the original does not have.
  return last;
}

// Build the residual folding pair the alternate models use, which is the same
// pair `predict_med` and `unpredict_med` build inline for themselves:

//   unfold[code]        the signed residual a code stands for: 0, -1, +1, -2,
//                       +2 ... out to -127, +127, -128.
//   fold[residual+256]  the inverse, indexed by the residual as a byte.  The
//                       +256 is the zero point, so `fold` is addressed from
//                       -128 to +127 either side of it.

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
  uint32_t done, lo;
  int32_t bucket_size, half, in_bucket, ofs, bucket, bucket_1;
  uint32_t i, k, b, span, pairs;
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
    ((uint8_t *)unfold)[4 * i + 1] = (uint8_t)(-2 * i - 1);
    ((uint8_t *)unfold)[4 * i + 4] = 2 * i + 2;
    ((uint8_t *)unfold)[4 * i + 3] = (uint8_t)(-2 * i - 2);
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
  // The wrap is the point: the odd codes count down from -1, which in a byte
  // is 255.  The cast says so, where the bare `-1` left the compiler to point
  // out that the value changes -- which it does, deliberately.
  fold[128] = (uint8_t)-1;
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
    if ( (done - 1) < span )
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
  for ( b = 0; b < 0x80; ++b )
  {
    (fold)[(uint8_t)((uint8_t *)unfold)[2 * b] + 256] = 2 * b;
    (fold)[(uint8_t)((uint8_t *)unfold)[2 * b + 1] + 256] = 2 * b + 1;
  }
  // 0x80, and no caller reads it.
  return b;
}

uint32_t __rc_decode_flat(uint32_t tot)
{
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym + 1, tot);
  return sym;
}

inline int32_t BitCtr::encode_context_bit(BitCtr *a2, int32_t bit)
{
  ;
  uint32_t c1, p0, p1, c0;
  int32_t par_tot;
  int32_t cap, result, p_cap, par0, par_n;
  uint32_t tot, p_tot, n1_old, p1_old;
  c0 = this->n[0];
  if ( this->n[0] )
  {
    c1 = this->n[1];
    if ( !this->n[1] )
    {
      par0 = a2->n[0];
      par_tot = par0 + a2->n[1];
      this->n[0] = (par_tot + (par0 << 6) - 64) / par_tot;
      this->n[1] = ((a2->n[1] << 6) + par_tot - 64) / par_tot;
      this->n[c0 - 1] += 4;
      this->limit = 512;
      par_n = a2->n[c0 - 1];
      a2->n[c0 - 1] = -3 * ((uint32_t)(3 - par_n) >> 31) + par_n;
      c0 = this->n[0];
      c1 = this->n[1];
    }
    tot = c0 + c1;
    rc.encode_bit(c0, c1, bit);
    cap = this->limit;
    if ( tot > (uint32_t)cap )
    {
      n1_old = this->n[1];
      this->n[0] -= this->n[0] >> 1;
      this->n[1] = n1_old - (n1_old >> 1);
      if ( cap < 0x4000 )
        this->limit = cap + 64;
    }
    result = this->n[bit] + 8;
    this->n[bit] = result;
    a2->n[bit] += (uint32_t)tot < 0x88;
    return result;
  }
  p0 = a2->n[0];
  p1 = a2->n[1];
  p_tot = p0 + p1;
  rc.encode_bit(p0, p1, bit);
  p_cap = a2->limit;
  if ( p_tot > (uint32_t)p_cap )
  {
    p1_old = a2->n[1];
    a2->n[0] -= a2->n[0] >> 1;
    a2->n[1] = p1_old - (p1_old >> 1);
    if ( p_cap < 0x4000 )
      a2->limit = p_cap + 64;
  }
  result = a2->n[bit] + 8;
  a2->n[bit] = result;
  this->n[0] = bit + 1;
  return result;
}

inline int32_t BitCtr::decode_context_bit(BitCtr *a2)
{
  ;
  uint32_t c0, c1, p0, p1;
  int32_t par_tot;
  int32_t result, cap, p_cap, par0, par_n;
  uint32_t tot, p_tot, n1_old, p1_old;
  c0 = this->n[0];
  if ( this->n[0] )
  {
    c1 = this->n[1];
    if ( !this->n[1] )
    {
      par0 = a2->n[0];
      par_tot = par0 + a2->n[1];
      this->n[0] = (par_tot + (par0 << 6) - 64) / par_tot;
      this->n[1] = ((a2->n[1] << 6) + par_tot - 64) / par_tot;
      this->n[c0 - 1] += 4;
      this->limit = 512;
      par_n = a2->n[c0 - 1];
      a2->n[c0 - 1] = -3 * ((uint32_t)(3 - par_n) >> 31) + par_n;
      c0 = this->n[0];
      c1 = this->n[1];
    }
    tot = c0 + c1;
    result = rc.decode_bit(c0, c1);
    cap = this->limit;
    if ( tot > (uint32_t)cap )
    {
      n1_old = this->n[1];
      this->n[0] -= this->n[0] >> 1;
      this->n[1] = n1_old - (n1_old >> 1);
      if ( cap < 0x4000 )
        this->limit = cap + 64;
    }
    this->n[result] += 8;
    a2->n[result] += (uint32_t)tot < 0x88;
  }
  else
  {
    p0 = a2->n[0];
    p1 = a2->n[1];
    p_tot = p0 + p1;
    result = rc.decode_bit(p0, p1);
    p_cap = a2->limit;
    if ( p_tot > (uint32_t)p_cap )
    {
      p1_old = a2->n[1];
      a2->n[0] -= a2->n[0] >> 1;
      a2->n[1] = p1_old - (p1_old >> 1);
      if ( p_cap < 0x4000 )
        a2->limit = p_cap + 64;
    }
    a2->n[result] += 8;
    this->n[0] = result + 1;
  }
  return result;
}

inline int32_t SymList::code_symbol(int32_t want)
{
  ;
  int8_t gen;
  int32_t enc_cum, enc_high, enc_tot;
  uint32_t left;
  uint8_t c_a, c_b;
  // Every one of these walked `this[5]`'s entries three bytes at a time,
  // reading the symbol as `*(uint16_t *)p` and the count as `p[2]`.
  SymEntry *p, *head, *q;
  // A cumulative count, a high count and a total: the three arguments
  // `RangeCoder::encode` takes, and it takes them unsigned.
  int32_t cum, s, c, result, c2, top;
  uint16_t s_a, s_b;
  uint32_t rest, i;
  gen = exclusion_gen;
  left = this->live;
  p = this->ent - 1;
  cum = 0;
  while ( 1 )
  {
    ++p;
    s = p->sym;
    if ( exclusion_mask[s] != exclusion_gen )
    {
      c = p->cnt;
      cum += c;
      if ( s == want )
        break;
    }
    if ( !--left )
    {
      if ( !cum )
        return 0;
      enc_cum = cum;
      enc_tot = this->tot + cum;
      enc_high = enc_tot;
      do
      {
        exclusion_mask[p->sym] = gen;
        --p;
      }
      while ( p >= this->ent );
      result = 0;
      {
        rc.encode(enc_cum, enc_high, enc_tot);
        return result;
      }
    }
  }
  enc_high = cum;
  rest = left - 1;
  enc_cum = cum - c;
  if ( rest )
  {
    for ( i = 0; i < rest; ++i )
    {
      if ( exclusion_mask[p[i + 1].sym] == exclusion_gen )
        c2 = 0;
      else
        c2 = p[i + 1].cnt;
      cum += c2;
    }
  }
  enc_tot = this->tot + cum;
  p->cnt += 4;
  head = this->ent;
  this->since_rescale += 4;
  if ( p == head )
  {
LABEL_37:
    top = p->cnt;
  }
  else
  {
    s_a = p->sym;
    c_a = p->cnt;
    q = p - 1;
    *p = *q;
    q->set(s_a, c_a);
    head = this->ent;
    if ( q == head )
    {
      top = q->cnt;
    }
    else
    {
      while ( 1 )
      {
        top = q->cnt;
        p = q - 1;
        if ( top <= p->cnt )
          break;
        s_b = q->sym;
        c_b = q->cnt;
        *q = *p;
        p->set(s_b, c_b);
        head = this->ent;
        --q;
        if ( p == head )
          goto LABEL_37;
      }
    }
  }
  this->rescale(top);
  result = 1;
  rc.encode(enc_cum, enc_high, enc_tot);
  return result;
}

// Add `a3` to symbol `a2`'s count in the list `code_symbol` codes from
// -- the same 3-byte entries, `_this[5]` base, `_this[1]` length -- then bubble
// the entry forward while it outweighs its predecessor, and halve every count
// when one passes 251 or the total passes `_this[4]`.  Sort, then rescale: this
// is the model update, and the only caller is `init_tables`.
// The return value is never read: `init_tables` calls this twelve times
// and discards every one.  That matters, because the slot Hex-Rays called
// `n251` carried two things -- the list base and a symbol's count byte -- and
// which of them reached the final `return` depended on the branch.  Split into
// `list` and `count`, there is nothing sensible to return, so it returns
// nothing.
inline void SymList::add_weight(int32_t want, uint32_t add)
{
  ;
  SymEntry *list;     // the three-byte entries, `this->ent`
  uint32_t count;     // a symbol's count byte, while it is being compared
  bool full;
  uint8_t c3, c, c2;
  // All of these walk the entries; the -3 and +2 they carried were the record
  // stride and the count field.
  SymEntry *q, *p, *head;
  uint16_t s3, s, s2;
  int32_t recycled;
  uint32_t n_live, left;
  list = this->ent;
  n_live = this->live;
  p = list;
  left = n_live;
  if ( n_live )
  {
    while ( p->sym != want )
    {
      ++p;
      if ( !--left )
        goto LABEL_4;
    }
    p->cnt += add;
    this->since_rescale += add;
    head = this->ent;
    if ( p == head )
    {
LABEL_16:
      count = p->cnt;
    }
    else
    {
      // Swap this entry with the one before it.
      s = p->sym;
      c = p->cnt;
      q = p - 1;
      *p = *q;
      q->set(s, c);
      head = this->ent;
      if ( q == head )
      {
        count = q->cnt;
      }
      else
      {
        while ( 1 )
        {
          count = q->cnt;
          p = q - 1;
          if ( count <= p->cnt )
            break;
          s2 = q->sym;
          c2 = q->cnt;
          *q = *p;
          p->set(s2, c2);
          head = this->ent;
          --q;
          if ( p == head )
            goto LABEL_16;
        }
      }
    }
    this->rescale(count);
    return;
  }
  else
  {
LABEL_4:
    full = n_live == this->n;
    if ( n_live >= this->n )
    {
      if ( add <= 1 )
        return;
      full = n_live == this->n;
    }
    if ( full )
    {
      this->live = --n_live;
      recycled = list[n_live].cnt;
    }
    else
    {
      recycled = 1;
    }
    list += n_live;
    this->live = n_live + 1;
    this->tot = recycled + this->tot + 1;
    list->cnt = 2;
    list->sym = want;
    this->since_rescale += 4;
    if ( list != this->ent )
    {
      // The new entry starts one place forward, same swap as above.
      s3 = list->sym;
      c3 = list->cnt;
      *list = list[-1];
      list[-1].set(s3, c3);
    }
  }
}

CounterNode *__init_counter_node(CounterNode *node)
{
  ;
  // The one body still spelled exactly as Hex-Rays spelled it, found by
  // diffing against the first commit rather than by looking: eight `uint16_t`
  // written through a pointer, which `CounterNode` has declared since round
  // nine.  22 is the sum of the seven counts, which is the invariant the
  // record is for and which `*_this = 22` does not say.
  node->c[0] = 8;
  node->c[1] = 2;
  node->c[2] = 2;
  node->c[3] = 2;
  node->c[4] = 2;
  node->c[5] = 3;
  node->c[6] = 3;
  node->total = 22;
  return node;
}

// Code a symbol as a level, then the bits that pick it out within that level.

// `freq` is a counter block: the total at [0], the escape weight at [1], and
// eight level counts from [2].  `model_geometry[sym]` says which level `sym`
// falls in -- 0 and 1 are levels of their own, then 2, 4, 8, 16, 32 and 64
// symbols per level -- so the level goes through the range coder against those
// eight counts, and anything above level 1 then needs `level_geom[lvl].half`
// bits to say which symbol within it.

// Those bits are coded down a binary tree of counter pairs at
// `freq[2 * level_geom[lvl].tbl_base + 8]`, most significant first, with `node`
// the index of the pair reached so far and `span` the width of the level below.
// `decode_symbol_tree` walks the identical tree and reassembles `node` into the
// symbol by adding `level_geom[lvl].first`.

// The rescale is the unrolled part: when the total passes 0x4000 all eight
// counts halve, in a chain of partial sums MSVC interleaved with the stores,
// and the escape weight steps down by 4 or by 16 depending on where it sits
// relative to `alt_freq_limit`.
int32_t __encode_symbol_tree(uint16_t *freq, int32_t sym)
{
  ;
  uint16_t add;
  int16_t sum4;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t cum;
  int32_t lvl, result, go, fa, esc, path, node, span, f1, mask;
  uint16_t *f0, fq, h2, h4, *slot;
  FreqPair *pair;
  uint32_t j, f1_old, acc, acc6, acc8, h9, tot, cum_hi;
  lvl = model_geometry[sym];
  f0 = freq + 2;
  // The counts below `sym`, which is where the range coder's interval starts.
  // Four at a time in SSE, then a scalar tail; integer addition does not care
  // about the order, so it is one loop.
  cum = 0;
  for ( j = 0; j < (uint32_t)lvl; j++ )
    cum += f0[j];
  slot = &f0[lvl];
  cum_hi = cum + *slot;
  tot = *freq;
  result = rc.encode(cum, cum_hi, tot);
  if ( *freq > 0x4000u )
  {
    h2 = *(freq + 2) - (*(freq + 2) >> 1);
    acc = *(freq + 3);
    *(freq + 2) = h2;
    LOWORD(acc) = acc - (acc >> 1);
    *(freq + 3) = acc;
    LOWORD(acc) = h2 + acc;
    h4 = *(freq + 4) - (*(freq + 4) >> 1);
    *(freq + 4) = h4;
    sum4 = acc + h4;
    LOWORD(acc) = *(freq + 5) - (*(freq + 5) >> 1);
    acc6 = *(freq + 6);
    *(freq + 5) = acc;
    LOWORD(acc6) = acc6 - (acc6 >> 1);
    *(freq + 6) = acc6;
    LOWORD(acc6) = sum4 + acc + acc6;
    LOWORD(acc) = *(freq + 7) - (*(freq + 7) >> 1);
    acc8 = *(freq + 8);
    *(freq + 7) = acc;
    LOWORD(acc) = acc6 + acc;
    LOWORD(acc8) = acc8 - (acc8 >> 1);
    h9 = *(freq + 9);
    *(freq + 8) = acc8;
    LOWORD(acc8) = acc + acc8;
    LOWORD(h9) = h9 - (h9 >> 1);
    esc = *(freq + 1);
    *(freq + 9) = h9;
    *freq = acc8 + h9;
    if ( (esc <= 4 * alt_freq_limit) )
    {
      result = 4 * (esc > alt_freq_limit);
      add = esc - result;
      *(freq + 1) = esc - result;
    }
    else
    {
      add = esc - 16;
      *(freq + 1) = esc - 16;
    }
  }
  else
  {
    add = *(freq + 1);
  }
  *slot += add;
  *freq += *(freq + 1);
  if ( sym >= 2 )
  {
    mask = level_geom[lvl].half;
    path = sym - level_geom[lvl].first;
    node = 0;
    for ( span = 1; ; span *= 2 )
    {
      pair = bit_tree(freq, lvl) + span + node;
      f1 = pair->f[1];
      go = (mask & path) != 0;
      fa = pair->f[0];
      result = rc.encode_bit(fa, f1, go);
      fq = pair->f[go];
      if ( fq > 0x4000u )
      {
        f1_old = pair->f[1];
        pair->f[0] -= pair->f[0] >> 1;
        pair->f[1] = f1_old - (f1_old >> 1);
        fq = pair->f[go];
      }
      pair->f[go] = alt_freq_init + fq;
      mask >>= 1;
      node = go + 2 * node;
      if ( !mask )
        return result;
    }
  }
  return result;
}

// The second parameter is never read: three of the five call sites pass
// `16 * ctx[0]` and the other two pass the symbol they are coding.
int32_t __alt_p1_encode_symbol(uint16_t *freq, int32_t unread_ctx, int32_t ctx, int32_t sym)
{
  ;
  bool done;
  uint16_t tot0;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t cum;
  int32_t slot, result, lo;
  uint16_t *p, *q, nf, *cur, *base;
  uint32_t tot, k, cum_hi;
  tot0 = *freq;
  slot = 6 - (sym & 1);
  if ( sym < 5 )
    slot = sym;
  base = freq + 1;
  cur = freq + 1;
  tot = tot0 & 0x7FFF;
  // The counts below the symbol, same shape as `encode_symbol_tree`'s.
  cum = 0;
  for ( k = 0; k < (uint32_t)slot; k++ )
    cum += base[k];
  cur = &base[slot];
  cum_hi = cum + *cur;
  rc.encode(cum, cum_hi, tot);
  if ( tot > 0x2000 )
  {
    *freq = 0x8000;
    p = freq + 7;
    lo = 256;
    done = freq + 7 < base;
    q = freq + 7;
    if ( freq + 7 >= base )
    {
      do
      {
        if ( *q < lo )
          lo = *q;
        --q;
      }
      while ( q >= base );
      done = p < base;
    }
    if ( !done )
    {
      do
      {
        if ( (uint16_t)lo <= 1u )
          nf = *p - (*p >> 1);
        else
          nf = (*p + 2) / 3;
        *p = nf;
        *freq += nf;
        --p;
      }
      while ( p >= base );
    }
  }
  *cur += 32;
  result = *freq + 32;
  *freq = result;
  if ( slot >= 5 )
    return __encode_symbol_tree(model_strip(
             128 * (uint32_t)(slot & 1)
             + ((((freq[1] + (result & 0x7FFF) + 96 - 2 * (uint32_t)freq[slot + 1]) >> 25) & 0xFFFFFFC0)
             + (uint32_t)ctx)), (sym - 5) >> 1);
  return result;
}

// The decoder half of `encode_symbol_tree`.  Same object -- `model_tables +
// 254 * index`, counts from `_this + 2`, total in `*_this` -- same halving when
// the total passes 0x4000, and the two are called from the matching sides:
// `encode_symbol_tree` from alt_p1/encode_symbol, this from
// alt_p1_decode_symbol and decode_symbol.
// The inverse of `encode_symbol_tree`, and the same block in the same order:
// find the level by cumulative count, rescale on the same threshold with the
// same chain, then walk the same binary tree and add `level_geom[lvl].first`
// to turn the path back into a symbol.
int32_t __decode_symbol_tree(uint16_t *freq)
{
  ;
  int16_t sum4, sum6;
  // The cumulative count the range coder takes, which it takes unsigned.
  int32_t k, fa, go, node1, esc, mask, node, sym, span, f1;
  uint16_t *cur, add, fq, h2, h4, h6, h8, *slot;
  FreqPair *pair;
  uint32_t target, cum, acc, acc8, tot;
  tot = *freq;
  sym = 0;
  target = rc.get_freq(tot);
  cur = freq + 2;
  cum = *(freq + 2);
  if ( cum <= target )
  {
    k = 0;
    do
    {
      ++cur;
      ++k;
      cum += *cur;
    }
    while ( cum <= target );
    sym = k;
  }
  slot = cur;
  rc.decode((cum - *cur), cum, tot);
  if ( *freq > 0x4000u )
  {
    h2 = *(freq + 2) - (*(freq + 2) >> 1);
    acc = *(freq + 3);
    *(freq + 2) = h2;
    LOWORD(acc) = acc - (acc >> 1);
    *(freq + 3) = acc;
    LOWORD(acc) = h2 + acc;
    h4 = *(freq + 4) - (*(freq + 4) >> 1);
    *(freq + 4) = h4;
    sum4 = acc + h4;
    LOWORD(acc) = *(freq + 5) - (*(freq + 5) >> 1);
    *(freq + 5) = acc;
    LOWORD(acc) = sum4 + acc;
    h6 = *(freq + 6) - (*(freq + 6) >> 1);
    *(freq + 6) = h6;
    sum6 = acc + h6;
    LOWORD(acc) = *(freq + 7) - (*(freq + 7) >> 1);
    *(freq + 7) = acc;
    LOWORD(acc) = sum6 + acc;
    h8 = *(freq + 8) - (*(freq + 8) >> 1);
    acc8 = *(freq + 9);
    *(freq + 8) = h8;
    LOWORD(acc8) = acc8 - (acc8 >> 1);
    *(freq + 9) = acc8;
    LOWORD(acc8) = acc + h8 + acc8;
    esc = *(freq + 1);
    *freq = acc8;
    if ( esc <= 4 * alt_freq_limit )
      add = esc - 4 * (esc > alt_freq_limit);
    else
      add = esc - 16;
    *(freq + 1) = add;
  }
  else
  {
    add = *(freq + 1);
  }
  *slot += add;
  *freq += *(freq + 1);
  if ( sym < 2 )
    return sym;
  else
  {
    mask = level_geom[sym].half;
    node = 0;
    span = 1;
    do
    {
      pair = bit_tree(freq, sym) + span + node;
      fa = pair->f[0];
      f1 = pair->f[1];
      go = rc.decode_bit(fa, f1);
      fq = pair->f[go];
      if ( fq > 0x4000u )
      {
        pair->f[0] -= pair->f[0] >> 1;
        pair->f[1] -= pair->f[1] >> 1;
        fq = pair->f[go];
      }
      pair->f[go] = alt_freq_init + fq;
      mask >>= 1;
      span *= 2;
      node1 = go + 2 * node;
      node = node1;
    }
    while ( mask );
    return node1 + level_geom[sym].first;
  }
}

int32_t __alt_p1_decode_symbol(uint16_t *freq, int32_t unread_arg, int32_t ctx)
{
  ;
  bool done;
  int16_t tot_new, nf;
  int32_t slot, lo;
  uint16_t *cur, *p, *q;
  uint32_t tot, sym, cum;
  uint16_t *base;   // was uint32_t: the base of the counts, as an address
  tot = *freq & 0x7FFF;
  sym = rc.get_freq(tot);
  cur = freq + 1;
  cum = (uint16_t)freq[1];
  base = freq + 1;
  while ( cum <= sym )
    cum += (uint16_t)*++cur;
  rc.decode(cum - (uint16_t)*cur, cum, tot);
  if ( tot > 0x2000 )
  {
    *freq = 0x8000;
    p = freq + 7;
    lo = 256;
    done = freq + 7 < base;
    q = freq + 7;
    if ( freq + 7 >= base )
    {
      do
      {
        if ( *q < lo )
          lo = *q;
        --q;
      }
      while ( q >= base );
      done = p < base;
    }
    if ( !done )
    {
      do
      {
        if ( (uint16_t)lo <= 1u )
          nf = *p - (*p >> 1);
        else
          nf = ((uint16_t)*p + 2) / 3;
        *p = nf;
        *freq += nf;
        --p;
      }
      while ( p >= base );
    }
  }
  *cur += 32;
  tot_new = *freq + 32;
  *freq = tot_new;
  slot = cur - base;
  if ( slot >= 5 )
    slot += 2
        * __decode_symbol_tree(model_strip(
            128 * (uint32_t)(slot & 1)
            + (((((uint16_t)freq[1] + (tot_new & 0x7FFF) + 96 - 2 * (uint32_t)(uint16_t)freq[slot + 1]) >> 25)
              & 0xFFFFFFC0)
            + (uint32_t)ctx)));
  return slot;
}

inline int32_t P2Freq::encode_symbol(const uint32_t *ctx_pair, int32_t sym)
{
  ;
  uint16_t f_before, *result;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t cum;
  int32_t st;
  uint16_t *slot;
  uint32_t tot, tot_1,
           f1_old, f2_old, down;
  cum = this->f[1] + this->f[0];
  tot = cum + this->f[2];
  if ( sym )
  {
    if ( (sym & 1) != 0 )
    {
      cum = this->f[0];
      slot = &this->f[1];
    }
    else
    {
      slot = &this->f[2];
    }
  }
  else
  {
    cum = 0;
    slot = &this->f[0];
  }
  tot_1 = cum + *slot;
  rc.encode(cum, tot_1, tot);
  f_before = *slot;
  if ( *slot > 0x4000u )
  {
    f1_old = this->f[1];
    f2_old = this->f[2];
    this->f[0] -= this->f[0] >> 1;
    st = this->step;
    this->f[1] = f1_old - (f1_old >> 1);
    this->f[2] = f2_old - (f2_old >> 1);
    if ( st <= 256 )
    {
      if ( st <= 32 )
        down = ((uint32_t)(16 - st) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(down) = 32;
      LOWORD(st) = st - down;
      this->step = st;
      f_before = *slot;
    }
    else
    {
      st = (uint32_t)st >> 1;
      this->step = st;
      f_before = *slot;
    }
  }
  else
  {
    LOWORD(st) = this->step;
  }
  result = slot;
  *slot = st + f_before;
  if ( sym > 0 )
    return __encode_symbol_tree(model_strip(ctx_pair[sym & 1]), (sym - 1) >> 1);
  return (int32_t)result;
}

// encode_symbol's counterpart, and called on the same two objects from
// the p2 decoders that the encoders hand to it.  It was `decode_three_way` for
// the shape of its first step -- a three-way choice over the counts at
// `_this[1..3]` -- but so is the encoder's first step, and a pair that codes
// the same thing should read as one.  The three-way part is still the first
// twenty lines; the name now says which half of the pair this is.
inline int32_t P2Freq::decode_symbol(const uint32_t *ctx_pair)
{
  ;
  // The cumulative count the range coder takes, which it takes unsigned.
  uint32_t cum;
  int32_t target, fq, st, idx, c01;
  uint16_t *slot, *base;
  uint32_t f1_old, f2_old, tot;
  c01 = this->f[1] + this->f[0];
  tot = this->f[2] + c01;
  target = rc.get_freq(tot);
  cum = this->f[0];
  if ( (uint32_t)target >= cum )
  {
    if ( target >= c01 )
    {
      cum = c01;
      slot = &this->f[2];
    }
    else
    {
      slot = &this->f[1];
    }
    base = &this->f[0];
  }
  else
  {
    slot = &this->f[0];
    cum = 0;
    base = &this->f[0];
  }
  rc.decode(cum, (cum + ((uint16_t)*slot)), tot);
  fq = (uint16_t)*slot;
  if ( fq > 0x4000 )
  {
    f1_old = this->f[1];
    f2_old = this->f[2];
    this->f[0] -= this->f[0] >> 1;
    st = this->step;
    this->f[1] = f1_old - (f1_old >> 1);
    this->f[2] = f2_old - (f2_old >> 1);
    if ( st <= 256 )
    {
      if ( st <= 32 )
        fq = ((uint32_t)(16 - st) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(fq) = 32;
      LOWORD(st) = st - fq;
      this->step = st;
      LOWORD(fq) = *slot;
    }
    else
    {
      st = (uint32_t)st >> 1;
      this->step = st;
      LOWORD(fq) = *slot;
    }
  }
  else
  {
    LOWORD(st) = this->step;
  }
  *slot = st + fq;
  idx = slot - base;
  if ( idx )
    return idx + 2 * __decode_symbol_tree(model_strip(ctx_pair[idx & 1]));
  else
    return 0;
}

inline uint32_t P2Freq::rescale_three_way()
{
  ;
  uint32_t cut, budget;
  this->f[0] -= this->f[0] >> 1;
  this->f[1] -= this->f[1] >> 1;
  cut = this->f[2] >> 1;
  this->f[2] -= cut;
  budget = this->step;
  if ( budget <= 0x100 )
  {
    if ( this->step <= 0x20u )
      cut = ((16 - budget) >> 30) & 0xFFFFFFFE;
    else
      cut = 32;
    this->step = budget - cut;
  }
  else
  {
    this->step >>= 1;
  }
  return cut;
}

int32_t __rescale_counter_pair(BitCtr *ctr)
{
  ;
  int32_t tot;
  ctr->n[0] -= ctr->n[0] >> 1;
  tot = ctr->limit;
  ctr->n[1] -= ctr->n[1] >> 1;
  if ( tot < 0x4000 )
  {
    tot += 64;
    ctr->limit = tot;
  }
  return tot;
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

void **__free_workspace(ModelBlock *blk, int8_t do_free)
{
  ;
  uint32_t n, left;
  int32_t i;
  SymList *lists, *p;
  free(blk->sym_code);
  free(blk->sym_word);
  free(blk->run_bucket);
  free(*(void**)&blk->alpha_map);
  // Both arrays are allocated as `bmf_new(24 * n + 4)` with the count in the
  // word before the first list, so `n` is `sym_list_count(lists)` and that
  // same word is what gets freed.  Each list owns its entries.
  
  // Hex-Rays had five more locals here: `Blocka_1`, `Blocka_2` and `Blocka_3`
  // were the block saved and restored around two loops that never touched it,
  // and `sel1_list`/`sel0_list` were second copies of the two list pointers,
  // restored the same way.  Nothing in either loop writes what they protect.
  lists = blk->sel1_list;
  if ( lists )
  {
    n = sym_list_count(lists);
    if ( n )
    {
      left = n;
      p = &lists[n];
      do
      {
        --p;
        free(p->ent);
        --left;
      }
      while ( left );
    }
    free(sym_list_block(lists));
  }
  lists = blk->sel0_list;
  if ( lists )
  {
    n = sym_list_count(lists);
    if ( n )
    {
      left = n;
      p = &lists[n];
      do
      {
        --p;
        free(p->ent);
        --left;
      }
      while ( left );
    }
    free(sym_list_block(lists));
  }
  for ( i = 0; i < 5; ++i )
    free(((void**)blk)[i + 14]);
  free(blk->escape.ent);
  if ( (do_free & 1) != 0 )
    free(blk);
  return (void **)blk;
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

inline int32_t ModelBlock::pixel_context(uint32_t *nb)
{
  ;
  SymList *sel0_list;
  bool near, far;
  int32_t band;
  uint32_t result;
  int32_t near_hit, far_hit, ctx0, ctx1, fallback, ctx2, pos;
  SymEntry *list_prev, *list_sym;
  pos = *(int32_t *)&this->sym_pos;
  result = (nb)[pos];
  if ( exclusion_mask[result] == exclusion_gen )
    return -1;
  // Three bands of the neighbour list, each worth a different bit: the symbol
  // at [10] on its own, the five at [11..15], and the sixteen at [16..31].
  // MSVC unrolled the two ranges into twenty-one comparisons of one shape.
  // `|=` rather than `||` because none of them has a side effect to short out.
  near = false;
  for ( band = 11; band < 16; ++band )
    near |= result == nb[band];
  far = false;
  for ( band = 16; band < 32; ++band )
    far |= result == nb[band];
  near_hit = 32 * near + ((result == nb[10]) << 6);
  far_hit = 16 * far;
  ctx0 = far_hit + near_hit;
  *(int32_t *)&this->ctr_node = ctx0;
  if ( (far_hit + near_hit == 0) && pos > 6 )
    return -1;
  sel0_list = this->sel0_list;
  ctx1 = ctx0 + 8 * sym_in_top((sel0_list[mode_symbol[1]].ent), 10, result);
  *(int32_t *)&this->ctr_node = ctx1;
  list_prev = sel0_list[mode_symbol[2]].ent;
  list_sym = sel0_list[result].ent;
  fallback = sym_in_top((sel0_list[mode_symbol[3]].ent), 6, result)
      + 2 * sym_in_top(list_sym, 10, mode_symbol[2])
      + 4 * sym_in_top(list_prev, 4, result);
  ctx2 = fallback + ctx1;
  if ( pos <= 14 || (ctx2 & 0xB) != 0 )
  {
    *(int32_t *)&this->ctr_node = (pos << 7) + ctx2;
    *(int32_t *)&this->ctr_fallback = fallback + 8 * (pos > 9);
  }
  else
  {
    *(int32_t *)&this->ctr_fallback = fallback;
    return -1;
  }
  return result;
}

inline int32_t ModelBlock::init_tables()
{
  ;
  SymEntry *slot, *ent;
  // The two rows above, walked one record at a time.  Every reach through
  // them is `match[0]` -- byte 2 of an eight-byte record -- so the byte
  // offsets 34 and -30 were records +4 and -4: the window `grad` slides,
  // adding the record entering on the right and dropping the one leaving.
  PixRec *up1, *up2;            // row cursors out of row_cur
  uint8_t cnt;
  SymList **cur;
  bool promoted;
  uint16_t *sym_cache;
  SymList *list, **prev;
  uint32_t blocks, n_live, hit0;
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  uint16_t want, sym;
  PixRec *row;   // the current row, one record past the pixel just written
  int32_t recycled, hit1, just, c0, c1, c2, c3, c4, c5, c6,
          result;
  hit0 = this->hit;
  if ( !hit0 )
  {
    if ( this->sel == this->sel_cur )
    {
      if ( this->sel[0] )
      {
        this->sel0_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 3u);
        this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
        this->sel1_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 4u);
        this->sel1_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
      }
      else
      {
        this->sel0_list[mode_symbol[2]].add_weight(this->row_cur[5]->sym, (this->sym_pos > 3) + 2);
      }
    }
    else
    {
      this->sel0_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 3u);
      this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
      this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 1u);
      this->sel1_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
      cur = this->sel_cur;
      do
      {
        prev = cur - 1;
        this->sel_cur = prev;
        // `add_weight`'s insert path, inlined: append the symbol at
        // `live`, evicting the last entry when the list is full, then swap it
        // one place forward.
        list = *prev;
        want = this->row_cur[5]->sym;
        n_live = list->live;
        ent = list->ent;
        if ( n_live == list->n )
        {
          list->live = --n_live;
          recycled = list->ent[n_live].cnt;
        }
        else
        {
          recycled = 1;
        }
        slot = &ent[n_live];
        list->live = n_live + 1;
        list->tot = recycled + list->tot + 1;
        slot->set(want, 2);
        list->since_rescale += 4;
        if ( slot != list->ent )
        {
          sym = slot->sym;
          cnt = slot->cnt;
          *slot = slot[-1];
          slot[-1].set(sym, cnt);
        }
        cur = this->sel_cur;
      }
      while ( cur != this->sel );
    }
    if ( exclusion_gen == -1 )
    {
      exclusion_gen = 1;
      buf = (uint8_t *)exclusion_mask;
      blocks = (this->alphabet + 15) >> 4;
      do
      {
        bmf_zero16(buf);
        buf += 16;
        --blocks;
      }
      while ( blocks );
      hit1 = this->hit;
    }
    else
    {
      ++exclusion_gen;
      hit1 = this->hit;
    }
    promoted = hit1 && hit1 <= 2;
  }
  else
  {
    // The `goto LABEL_21` that ended the block above skipped exactly this, and
    // it was the block's last statement -- so the two are an `if`/`else` and
    // the label was the join.
    
    // `LABEL_19` was a jump *into* the arm above, to reach the one test at the
    // end of it.  Both arms decide the same thing -- whether the symbol was
    // already near the front of its list -- so they both set it, and the
    // cache-reordering block below runs when neither did.
    promoted = hit0 <= 2;
    if ( !promoted && mode_symbol[3] != mode_symbol[4] )
    {
      this->sel0_list[mode_symbol[2]].add_weight(this->row_cur[5]->sym, 1u);
      hit1 = this->hit;
      promoted = hit1 && hit1 <= 2;
    }
  }
  if ( !promoted )
  {
  just = this->row_cur[5]->sym;
  sym_cache = this->sym_cache;
  c0 = sym_cache[0];
  if ( just != c0 )
  {
    c1 = sym_cache[1];
    if ( just == c1 )
    {
      sym_cache[1] = c0;
    }
    else
    {
      c2 = sym_cache[2];
      if ( just == c2 )
      {
        sym_cache[2] = c1;
        this->sym_cache[1] = this->sym_cache[0];
      }
      else
      {
        c3 = sym_cache[3];
        if ( just == c3 )
        {
          sym_cache[3] = c2;
          this->sym_cache[2] = this->sym_cache[1];
          this->sym_cache[1] = this->sym_cache[0];
        }
        else
        {
          c4 = sym_cache[4];
          if ( just == c4 )
          {
            sym_cache[4] = c3;
            this->sym_cache[3] = this->sym_cache[2];
            this->sym_cache[2] = this->sym_cache[1];
            this->sym_cache[1] = this->sym_cache[0];
          }
          else
          {
            c5 = sym_cache[5];
            if ( just == c5 )
            {
              sym_cache[5] = c4;
              this->sym_cache[4] = this->sym_cache[3];
              this->sym_cache[3] = this->sym_cache[2];
              this->sym_cache[2] = this->sym_cache[1];
              this->sym_cache[1] = this->sym_cache[0];
            }
            else
            {
              c6 = sym_cache[6];
              if ( just == c6 )
              {
                sym_cache[6] = c5;
              }
              else
              {
                sym_cache[7] = c6;
                this->sym_cache[6] = this->sym_cache[5];
              }
              this->sym_cache[5] = this->sym_cache[4];
              this->sym_cache[4] = this->sym_cache[3];
              this->sym_cache[3] = this->sym_cache[2];
              this->sym_cache[2] = this->sym_cache[1];
              this->sym_cache[1] = this->sym_cache[0];
            }
          }
        }
      }
    }
    this->sym_cache[0] = just;
  }
  }
  // The other fourteen sites already spell the 16-bit accesses out --
  // `*(uint16_t *)(pix_cur + 2) = *(uint16_t *)pix_cur` at 11752 is this
  // line -- so the field is a byte cursor and these two were the odd ones.
  this->pix_cur[1] = this->pix_cur[0];
  this->pix_cur[0] = this->row_cur[5]->sym;
  // Six neighbours, compared against the symbol just written.
  {
    PixRec *const here = this->row_cur[5];
    PixRec *const up   = this->row_cur[6];
    here->match[0] = here->sym == up->sym;
    here->match[1] = here->sym == here[-1].sym;
    here->match[2] = here->sym == up[1].sym;
    here->match[3] = here->sym == up[-1].sym;
    here->match[4] = here->sym == up[2].sym;
    here->match[5] = here->sym == up[3].sym;
  }
  up1 = this->row_cur[6];
  up2 = this->row_cur[7];
  row = this->row_cur[5] + 1;
  this->row_cur[5] = row;
  ++this->row_cur[8];
  ++up1;
  this->row_cur[6] = up1;
  ++up2;
  ++this->row_cur[9];
  this->row_cur[7] = up2;
  this->grad[0] += up1[4].match[0] - up1[-4].match[0];
  this->grad[1] += up2[4].match[0] - up2[-4].match[0];
  this->grad[2] += row[-1].match[1] - row[-5].match[1];
  result = row[-1].match[0] - row[-8].match[0];
  this->grad[3] += result;
  return result;
}

void **__alt_p2_free(void **blk, int8_t do_free)
{
  ;
  free(*(blk + 69689));
  free(*(blk + 69690));
  free(*(blk + 69691));
  free(*(blk + 69692));
  free(*(blk + 69693));
  free(*(blk + 69665));
  free(*(blk + 69666));
  if ( (do_free & 1) != 0 )
    bmf_page_free(blk);
  return blk;
}


void **__alt_p1_free(void **blk, int8_t do_free)
{
  ;
  free(*(blk + 44));
  free(*(blk + 45));
  free(*(blk + 46));
  free(*(blk + 47));
  free(*(blk + 48));
  if ( (do_free & 1) != 0 )
    free(blk);
  return blk;
}

// Context for the alternate p1 model: reads the causal neighbourhood through
// the row pointers at `_this[49..51]`, forms the gradients (`2*W - WW - N` and
// friends) and a weighted neighbour sum, and stores them along with the model
// table pointers they select.  It codes nothing -- no `rc.` call anywhere in it
// -- and all four p1 bodies call it, encoder and decoder alike, which is what
// says it is context rather than coding.
// Build the coding context for one p1 pixel, and the prediction it is coded
// against.  Three things come out: `pred`, `ctx[0..1]`, and the nine
// three-way selectors in `ctx_w`, whose weights sum to the context index the
// caller uses.

// It opens with MED -- the same median edge tree as `predict_med`, over
// `cursor[1]` (north), `cur[-1]` (west) and `cursor[1][-1]` (northwest).

// `act` is the activity: a weighted sum of the error magnitudes around the
// pixel, 6 on the west neighbour, 4 on north and two-west, 3 on north-east and
// the row above that, 2 further out, 1 at the edges.  `act_all` adds whatever
// the neighbouring planes contribute, which is why there are three arms -- two
// neighbours, one, or none -- and each arm also picks a different set of
// gradients for `ctx_w[3]` and `ctx_w[5..8]`.  With no neighbouring plane the
// gradients come from this plane's own rows instead.

// Then `act_q` quantises the activity, `level_of` maps it to a level and
// `step` is that level's dead zone, and each gradient becomes a three-way
// selector: below `-step`, within, or above `+step`.  Nine selectors, nine
// weights, one context.
inline int32_t AltP1Block::ctx_of(AltP1Block *nb0, AltP1Block *nb1)
{
  P1Ctx *nb0_row, *nb0_row2, *cursor1, *cursor2;
  ;
  P1Ctx *cur, *cursor4;
  uint32_t nb0_b;
  bool pick, is_zero, is_neg;
  int32_t west, northwest, act, nb0_a, act_q, s1, hi, lo, s3, s5, g6, h6, g7, h7,
          s8, result, step, act_all, s2, s4, s6, s7;
  uint32_t quiet, s0;
  // Byte values Hex-Rays gave a pointer type: none is dereferenced here,
  // and `&guess[k]` on a `uint8_t *` is the addition it looks like.  The
  // `(uint32_t)` casts on them stay: `(216 - (uint32_t)guess) >> 31` is a
  // logical shift, and it is a selector -- signed it would be -1, not 1.
  int32_t guess, plane_a, plane_b, level_of;
  cursor1 = this->cursor[1];
  guess = cursor1->sym;
  cur = this->cursor[0];
  west = cur[-1].sym;
  northwest = cursor1[-1].sym;
  if ( west < guess )
  {
    if ( northwest >= west )
    {
      plane_b = guess + west - northwest;
      pick = northwest < guess;
      guess = cur[-1].sym;
      if ( pick )
        guess = plane_b;
    }
  }
  else if ( northwest <= west )
  {
    plane_a = guess + west - northwest;
    pick = northwest <= guess;
    guess = cur[-1].sym;
    if ( !pick )
      guess = plane_a;
  }
  this->pred = guess;
  cursor2 = this->cursor[2];
  act = cursor1[-1].mag
      + cur[-3].mag
      + 3 * (cursor1[1].mag + cursor2->mag)
      + 6 * cur[-1].mag
      + 4 * (cursor1->mag + cur[-2].mag)
      + 2 * (cursor2[2].mag + cursor1[2].mag + cur[-4].mag);
  // The two neighbouring planes, reached through their own row cursors.  Both
  // rows are two bytes a pixel -- the reconstructed sample at +0 and the size
  // of the prediction error at +1 -- so the odd subscript `[-1]` is the error
  // magnitude one pixel back and the even ones `[-2]` and `[-4]` are the
  // samples one and two pixels back.  Hex-Rays typed both parameters
  // `uint32_t *` and reached the cursors as `nb0[49]` and `nb0[50]`, which is
  // +196 and +200: `cursor[0]` and `cursor[1]`.  `nb0[2]` is `f8`.
  if ( nb0 )
  {
    if ( nb1 )
    {
      nb0_row = nb0->cursor[0];
      act_all = act + 2 * (nb1->cursor[0][-1].mag + nb0_row[-1].mag);
      this->ctx_w[5].sel = (cursor1->sym
                                       - (uint32_t)guess
                                       + nb0_row[-1].sym
                                       - nb0->cursor[1][-1].sym);
      nb0_row2 = nb0->cursor[0];
      nb0_a = nb0_row2[-1].sym;
      nb0_b = nb0_row2[-2].sym;
      this->ctx_w[6].sel = (cur[-1].sym - (uint32_t)guess + nb0_a - nb0_b);
      this->ctx_w[7].sel = (cur[-1].sym
                                       - (uint32_t)guess
                                       + nb1->cursor[0][-1].sym
                                       - nb1->cursor[0][-2].sym);
      this->ctx_w[8].sel = (nb1->cursor[0][-1].sym - nb1->pred);
      this->ctx_w[3].sel = (nb0->cursor[0][-1].sym - nb0->pred);
      quiet = (nb1->cursor[0][-1].mag + (uint32_t)nb0->cursor[0][-1].mag - 16) >> 31;
    }
    else
    {
      act_all = cursor1[3].mag + act + 3 * nb0->cursor[0][-1].mag;
      this->ctx_w[5].sel = (2 * cur[-1].sym - cur[-2].sym - (uint32_t)guess);
      this->ctx_w[6].sel = (2 * cur[-1].sym - cur[-2].sym - (uint32_t)guess);
      this->ctx_w[7].sel = (-guess - cursor1->sym + cursor1[1].sym + cur[-1].sym);
      this->ctx_w[8].sel = (cur[-1].sym
                                       - (uint32_t)guess
                                       + nb0->cursor[0][-1].sym
                                       - nb0->cursor[0][-2].sym);
      this->ctx_w[3].sel = (nb0->cursor[0][-1].sym - nb0->pred);
      quiet = ((uint32_t)nb0->cursor[0][-1].mag - 8) >> 31;
    }
  }
  else
  {
    cursor4 = this->cursor[4];
    act_all = cursor4->mag + cursor2[-2].mag + cursor1[3].mag + act + cursor4[2].mag;
    this->ctx_w[5].sel = (2 * cur[-1].sym - cur[-2].sym - (uint32_t)guess);
    this->ctx_w[6].sel = (2 * cursor1->sym - cursor2->sym - (uint32_t)guess);
    this->ctx_w[7].sel = (-guess - cursor1->sym + cursor1[1].sym + cur[-1].sym);
    this->ctx_w[8].sel = (-3 * (cur[-2].sym - cur[-1].sym)
                                     + cur[-3].sym
                                     - (uint32_t)guess);
    this->ctx_w[3].sel = (cursor1[2].sym - (uint32_t)guess);
    quiet = cur->mag + cursor4->mag + this->cursor[3]->mag + cursor2->mag + cursor1->mag == 0;
  }
  act_q = (act_all + 7) >> 4;
  level_of = this->level_of[act_q];
  step = p1_level_step[(uint32_t)level_of];
  this->ctx[0] = level_of;
  this->ctx[1] = this->group_of[act_q] + this->slot_of[(uint32_t)guess];
  this->ctx_w[0].sel = (((216 - (uint32_t)guess) >> 31) + ((22 - (uint32_t)guess) >> 31));
  s0 = ((216 - (uint32_t)guess) >> 31) + ((22 - (uint32_t)guess) >> 31);
  s1 = (cursor1[-1].sym - cursor1->sym >= 0) + (cursor1[-1].sym > (int32_t)cursor1->sym);
  this->ctx_w[1].sel = s1;
  s2 = (cursor1[-1].sym - cur[-1].sym >= 0) + (cursor1[-1].sym > (int32_t)cur[-1].sym);
  this->ctx_w[2].sel = s2;
  is_zero = this->ctx_w[3].sel == 0;
  is_neg = this->ctx_w[3].sel < 0;
  s4 = (cursor1[1].sym - guess >= -step) + (cursor1[1].sym - guess > step);
  this->ctx_w[4].sel = s4;
  hi = step < this->ctx_w[5].sel;
  lo = -step <= this->ctx_w[5].sel;
  s3 = !is_neg + (!is_neg && !is_zero);
  this->ctx_w[3].sel = s3;
  s5 = lo + hi;
  is_zero = this->ctx_w[6].sel == 0;
  is_neg = this->ctx_w[6].sel < 0;
  this->ctx_w[5].sel = s5;
  g6 = !is_neg && !is_zero;
  h6 = !is_neg;
  is_zero = this->ctx_w[7].sel == 0;
  is_neg = this->ctx_w[7].sel < 0;
  s6 = h6 + g6;
  this->ctx_w[6].sel = (h6 + g6);
  g7 = !is_neg && !is_zero;
  h7 = !is_neg;
  is_zero = this->ctx_w[8].sel == 0;
  is_neg = this->ctx_w[8].sel < 0;
  s7 = h7 + g7;
  this->ctx_w[7].sel = (h7 + g7);
  s8 = !is_neg + (!is_neg && !is_zero);
  this->ctx_w[8].sel = s8;
  result = this->ctx_w[0].w[s0] + this->ctx_w[1].w[s1]
         + this->ctx_w[2].w[s2] + this->ctx_w[3].w[s3]
         + this->ctx_w[4].w[s4] + this->ctx_w[5].w[s5]
         + this->ctx_w[6].w[s6] + this->ctx_w[7].w[s7]
         + this->ctx_w[8].w[s8]
         + 16 * quiet
         + 8 * (this->ctx[3 + this->ctx[2]] == 0)
         + level_of;
  this->ctx[0] = result;
  return result;
}

int32_t __update_binary_pair(uint16_t *_this, int32_t symbol)
{
  ;
  // Was `int32_t`, then a byte pointer laundered back through one: Hex-Rays
  // kept the tree base in a register and every step in bytes.  It is the tree.
  FreqPair *tbl, *pair;
  int32_t tot, mask, lvl, node, span, path, go;
  uint32_t step, f, f1_old;
  tot = *_this;
  if ( (uint32_t)tot <= 0x8000 )
  {
    lvl = model_geometry[symbol];
    step = (_this[1] >> 2) & 0xFFFFFFE0;
    if ( ::plane_predictor == 2 )
      step = 15 * (_this[1] >> 5);
    *(_this + lvl + 2) += step + 4;
    tot = *_this + step + 4;
    *_this = tot;
    if ( symbol >= 2 )
    {
      // One local, `n0x8000`, held the running total up to here and the walk
      // mask from here down: one register doing two jobs, which is why its
      // name was a constant.  Splitting it is what lets the loop below say
      // what it walks -- one bit of `path` per level, most significant first.
      mask = level_geom[lvl].half;
      path = symbol - level_geom[lvl].first;
      node = 0;
      tbl = bit_tree(_this, lvl);
      span = 1;
      do
      {
        pair = tbl + node + span;
        go = (mask & path) != 0;
        f = pair->f[go];
        if ( f > 0x2000 )
        {
          f1_old = pair->f[1];
          pair->f[0] -= pair->f[0] >> 1;
          pair->f[1] = f1_old - (f1_old >> 1);
          f = pair->f[go];
        }
        span *= 2;
        mask >>= 1;
        node = go + 2 * node;
        pair->f[go] = f + ((alt_freq_init * ((uint32_t)(::plane_predictor == 2) + 5)) >> 3);
      }
      while ( mask );
      // The loop leaves only when `mask` is 0, and every one of the nine
      // call sites discards the result -- one of them inside a comma
      // expression -- so this is Hex-Rays reading the register, not a value.
      return 0;
    }
  }
  return tot;
}

// Update the p1 model after a pixel: add to the counters the coded symbol
// reached, and to the counters its *neighbours* in context space would have
// reached, so a context seen once carries evidence from the contexts either
// side of it.

// The body is nine copies of one block, one per `ctx_w[k]`, and naming them
// with the index is what makes that visible.  Each block:

//   selK              the three-way selector this weight ended on
//   midK, loK, hiK    when the selector is the middle one, the context with
//                     `w[1]` removed and the two counter nodes either side
//   oppK              otherwise, the node on the opposite side of the selector
//   w1K, altiK        `w[1]` and the alternate context built from it
//   midnK, altnK      the two nodes that alternate gets
//   ctxK              `ctx[0]` reloaded, because every `+=` above may have
//                     moved it

// The increments fall away from the symbol -- 17 at the node itself, 13 and 11
// one context up and down, 7, 6, 5, 4, 3 and 2 further out -- which is the
// same "spread the evidence" idea `sym_rev` implements for symbols.
inline int32_t AltP1Block::update_model()
{
  ;
  uintptr_t result;   // an index into the counter table, and the return value
  CounterNode *node;  // was `result` too: the address role, under its own name
  CounterNode *node_alt, *node_up, *node_dn;
  int32_t alti8;
  uint32_t w11, w12, w13, w14, w15, w16, w17, w18, alti1, ctx_alt, alti0, alti2, alti3, alti4,
           alti5, alti6, alti7, x2, x3, x4, x5, x6, x7;
  CounterNode *opp0, *opp1, *opp2, *opp3, *opp4, *opp5, *opp6, *opp7, *opp8;
  int16_t tot_up, tot_dn;
  int32_t sel1_top, code_r, sel2_top, slot_f, slot_r, ctx_dn, sel0,
          ctx0, sel1, ctx1, sel2, ctx2,
          sel3, ctx3, sel4, ctx4, sel5, 
          ctx5, sel6, ctx6, sel7, 
          ctx7, sel8, midn8, ctx8, mid7, mid6, mid5, mid4, mid3,
          mid2, mid1, mid0, tree_sym;
  CounterNode *lo0, *hi0, *midn0, *altn0, *lo1, *hi1, *midn1, *altn1, *lo2, *hi2, *midn2, *altn2,
              *lo3, *hi3, *midn3, *altn3, *lo4, *hi4, *midn4, *altn4, *lo5, *hi5, *midn5, *altn5,
              *lo6, *hi6, *midn6, *altn6, *lo7, *hi7, *midn7, *altn7, *altn8, *x8;
  uint32_t key;
  uint32_t code_f, ctx_up;
  code_f = *((uint8_t)(this->cursor[0]->sym - (uint8_t)this->pred) + this->fold);
  sel1_top = this->ctx_w[1].sel;
  code_r = *((uint8_t)((uint8_t)this->pred - this->cursor[0]->sym) + this->fold);
  sel2_top = this->ctx_w[2].sel;
  tree_sym = (int32_t)(code_f - 5) >> 1;
  slot_f = 6 - (code_f & 1);
  if ( code_f < 5 )
    slot_f = *((uint8_t)(this->cursor[0]->sym - (uint8_t)this->pred) + this->fold);
  slot_r = 6 - (code_r & 1);
  if ( code_r < 5 )
    slot_r = code_r;
  ctx_alt = this->ctx_w[8].w[2 - this->ctx_w[8].sel]
     + this->ctx_w[7].w[2 - this->ctx_w[7].sel]
     + this->ctx_w[6].w[2 - this->ctx_w[6].sel]
     + this->ctx_w[5].w[2 - this->ctx_w[5].sel]
     + this->ctx_w[4].w[2 - this->ctx_w[4].sel]
     + this->ctx_w[3].w[2 - this->ctx_w[3].sel]
     + this->ctx_w[2].w[2 - sel2_top]
     + this->ctx_w[1].w[2 - sel1_top]
     + this->ctx_w[0].w[1]
     + (this->ctx[0] & 0x1F);
  node_alt = &this->counters[ctx_alt];
  node_alt[0].c[slot_r] += 17;
  node_alt[0].total += 17;
  result = this->ctx[0];
  if ( (result & 7) != 7 )
  {
    node_up = &this->counters[result];
    ctx_up = (((this->ctx[1] & 7u) - 7) >> 31) + this->ctx[1];
    node_up[1].c[slot_f] += 11;
    tot_up = node_up[1].total + 11;
    node_up[1].total = tot_up;
    if ( slot_f >= 5 )
      __update_binary_pair(model_strip(
        128 * (slot_f & 1)
        + ctx_up
        + (((((tot_up & 0x7FFF)
            + node_up[1].c[0]
            - 2 * (uint32_t)node_up[1].c[slot_f]) >> 25)
          & 0xFFFFFFC0))), (int32_t)(code_f - 5) >> 1);
    result = this->ctx[0];
  }
  if ( (result & 7) != 0 )
  {
    node_dn = &this->counters[result];
    ctx_dn = this->ctx[1] - ((this->ctx[1] & 7) != 0);
    node_dn[-1].c[slot_f] += 13;
    tot_dn = node_dn[-1].total + 13;
    node_dn[-1].total = tot_dn;
    if ( slot_f >= 5 )
      __update_binary_pair(model_strip(
        128 * (uint32_t)(slot_f & 1)
        + (uint32_t)ctx_dn
        + (((((tot_dn & 0x7FFF)
            + node_dn[-1].c[0]
            - 2 * (uint32_t)node_dn[-1].c[slot_f]) >> 25)
          & 0xFFFFFFC0))), tree_sym);
    result = this->ctx[0];
  }
  if ( this->counters[result].total < 0xCCCu )
  {
    if ( (result & 7u) < 7 )
    {
      node_alt[1].c[slot_r] += 7;
      node_alt[1].total += 7;
      result = this->ctx[0];
    }
    if ( (result & 7) != 0 )
    {
      node_alt[-1].c[slot_r] += 5;
      node_alt[-1].total += 5;
      result = this->ctx[0];
    }
    if ( slot_f >= 5 )
    {
      key = this->ctx[1]
           + (((this->counters[result].c[0]
              + (this->counters[result].total & 0x7FFF)
              - 2 * (uint32_t)this->counters[result].c[slot_f]) >> 25)
            & 0xFFFFFFC0)
           + ((slot_f & 1) << 7);
      if ( (key & 0x38) >= 0x38
        || (__update_binary_pair(model_strip(
              128 * (slot_f & 1)
              + this->ctx[1]
              + ((((this->counters[result].c[0]
                  + (this->counters[result].total & 0x7FFF)
                  - 2 * (uint32_t)this->counters[result].c[slot_f]) >> 25)
                & 0xFFFFFFC0))
              + 8), tree_sym),
            (key & 0x38) != 0) )
      {
        __update_binary_pair(model_strip(key - 8), tree_sym);
      }
      result = this->ctx[0];
    }
    sel0 = this->ctx_w[0].sel;
    if ( sel0 == 1 )
    {
      mid0 = result - this->ctx_w[0].w[1];
      lo0 = &this->counters[mid0 + this->ctx_w[0].w[0]];
      hi0 = &this->counters[this->ctx_w[0].w[2] + mid0];
      lo0[0].c[slot_f] += 6;
      lo0[0].total += 6;
      hi0[0].c[slot_f] += 6;
      hi0[0].total += 6;
      ctx0 = this->ctx[0];
      if ( (ctx0 & 7) != 7 )
      {
        lo0[1].c[slot_f] += 4;
        lo0[1].total += 4;
        hi0[1].c[slot_f] += 4;
        hi0[1].total += 4;
        ctx0 = this->ctx[0];
      }
      if ( (ctx0 & 7) != 0 )
      {
        lo0[-1].c[slot_f] += 3;
        lo0[-1].total += 3;
        hi0[-1].c[slot_f] += 3;
        hi0[-1].total += 3;
        ctx0 = this->ctx[0];
      }
    }
    else
    {
      opp0 = &this->counters[this->ctx_w[0].w[2 - sel0] + result - this->ctx_w[0].w[sel0]];
      opp0[0].c[slot_f] += 7;
      opp0[0].total += 7;
      alti0 = ctx_alt + this->ctx_w[0].w[0] - this->ctx_w[0].w[1];
      midn0 = &this->counters[this->ctx_w[0].w[1] + this->ctx[0] - this->ctx_w[0].w[this->ctx_w[0].sel]];
      midn0[0].c[slot_f] += 6;
      midn0[0].total += 6;
      altn0 = &this->counters[alti0];
      altn0[0].c[slot_r] += 4;
      altn0[0].total += 4;
      ctx0 = this->ctx[0];
      if ( (ctx0 & 7) != 7 )
      {
        midn0[1].c[slot_f] += 4;
        midn0[1].total += 4;
        altn0[1].c[slot_r] += 2;
        altn0[1].total += 2;
        ctx0 = this->ctx[0];
      }
      if ( (ctx0 & 7) != 0 )
      {
        midn0[-1].c[slot_f] += 3;
        midn0[-1].total += 3;
        altn0[-1].c[slot_r] += 2;
        altn0[-1].total += 2;
        ctx0 = this->ctx[0];
      }
    }
    sel1 = this->ctx_w[1].sel;
    if ( sel1 == 1 )
    {
      mid1 = ctx0 - this->ctx_w[1].w[1];
      lo1 = &this->counters[mid1 + this->ctx_w[1].w[0]];
      hi1 = &this->counters[this->ctx_w[1].w[2] + mid1];
      lo1[0].c[slot_f] += 6;
      lo1[0].total += 6;
      hi1[0].c[slot_f] += 6;
      hi1[0].total += 6;
      ctx1 = this->ctx[0];
      if ( (ctx1 & 7) != 7 )
      {
        lo1[1].c[slot_f] += 4;
        lo1[1].total += 4;
        hi1[1].c[slot_f] += 4;
        hi1[1].total += 4;
        ctx1 = this->ctx[0];
      }
      if ( (ctx1 & 7) != 0 )
      {
        lo1[-1].c[slot_f] += 3;
        lo1[-1].total += 3;
        hi1[-1].c[slot_f] += 3;
        hi1[-1].total += 3;
        ctx1 = this->ctx[0];
      }
    }
    else
    {
      opp1 = &this->counters[this->ctx_w[1].w[2 - sel1] + ctx0 - this->ctx_w[1].w[sel1]];
      opp1[0].c[slot_f] += 7;
      opp1[0].total += 7;
      w11 = this->ctx_w[1].w[1];
      alti1 = ctx_alt + w11 - this->ctx_w[1].w[2 - this->ctx_w[1].sel];
      midn1 = &this->counters[(w11 + this->ctx[0] - this->ctx_w[1].w[this->ctx_w[1].sel])];
      midn1[0].c[slot_f] += 6;
      midn1[0].total += 6;
      altn1 = &this->counters[alti1];
      altn1[0].c[slot_r] += 4;
      altn1[0].total += 4;
      ctx1 = this->ctx[0];
      if ( (ctx1 & 7) != 7 )
      {
        midn1[1].c[slot_f] += 4;
        midn1[1].total += 4;
        altn1[1].c[slot_r] += 2;
        altn1[1].total += 2;
        ctx1 = this->ctx[0];
      }
      if ( (ctx1 & 7) != 0 )
      {
        midn1[-1].c[slot_f] += 3;
        midn1[-1].total += 3;
        altn1[-1].c[slot_r] += 2;
        altn1[-1].total += 2;
        ctx1 = this->ctx[0];
      }
    }
    sel2 = this->ctx_w[2].sel;
    if ( sel2 == 1 )
    {
      mid2 = ctx1 - this->ctx_w[2].w[1];
      lo2 = &this->counters[mid2 + this->ctx_w[2].w[0]];
      hi2 = &this->counters[this->ctx_w[2].w[2] + mid2];
      lo2[0].c[slot_f] += 6;
      lo2[0].total += 6;
      hi2[0].c[slot_f] += 6;
      hi2[0].total += 6;
      ctx2 = this->ctx[0];
      if ( (ctx2 & 7) != 7 )
      {
        lo2[1].c[slot_f] += 4;
        lo2[1].total += 4;
        hi2[1].c[slot_f] += 4;
        hi2[1].total += 4;
        ctx2 = this->ctx[0];
      }
      if ( (ctx2 & 7) != 0 )
      {
        lo2[-1].c[slot_f] += 3;
        lo2[-1].total += 3;
        hi2[-1].c[slot_f] += 3;
        hi2[-1].total += 3;
        ctx2 = this->ctx[0];
      }
    }
    else
    {
      opp2 = &this->counters[this->ctx_w[2].w[2 - sel2] + ctx1 - this->ctx_w[2].w[sel2]];
      opp2[0].c[slot_f] += 7;
      opp2[0].total += 7;
      w12 = this->ctx_w[2].w[1];
      x2 = w12 + this->ctx[0] - this->ctx_w[2].w[this->ctx_w[2].sel];
      alti2 = ctx_alt + w12 - this->ctx_w[2].w[2 - this->ctx_w[2].sel];
      midn2 = &this->counters[x2];
      midn2[0].c[slot_f] += 6;
      midn2[0].total += 6;
      altn2 = &this->counters[alti2];
      altn2[0].c[slot_r] += 4;
      altn2[0].total += 4;
      ctx2 = this->ctx[0];
      if ( (ctx2 & 7) != 7 )
      {
        midn2[1].c[slot_f] += 4;
        midn2[1].total += 4;
        altn2[1].c[slot_r] += 2;
        altn2[1].total += 2;
        ctx2 = this->ctx[0];
      }
      if ( (ctx2 & 7) != 0 )
      {
        midn2[-1].c[slot_f] += 3;
        midn2[-1].total += 3;
        altn2[-1].c[slot_r] += 2;
        altn2[-1].total += 2;
        ctx2 = this->ctx[0];
      }
    }
    sel3 = this->ctx_w[3].sel;
    if ( sel3 == 1 )
    {
      mid3 = ctx2 - this->ctx_w[3].w[1];
      lo3 = &this->counters[mid3 + this->ctx_w[3].w[0]];
      hi3 = &this->counters[this->ctx_w[3].w[2] + mid3];
      lo3[0].c[slot_f] += 6;
      lo3[0].total += 6;
      hi3[0].c[slot_f] += 6;
      hi3[0].total += 6;
      ctx3 = this->ctx[0];
      if ( (ctx3 & 7) != 7 )
      {
        lo3[1].c[slot_f] += 4;
        lo3[1].total += 4;
        hi3[1].c[slot_f] += 4;
        hi3[1].total += 4;
        ctx3 = this->ctx[0];
      }
      if ( (ctx3 & 7) != 0 )
      {
        lo3[-1].c[slot_f] += 3;
        lo3[-1].total += 3;
        hi3[-1].c[slot_f] += 3;
        hi3[-1].total += 3;
        ctx3 = this->ctx[0];
      }
    }
    else
    {
      opp3 = &this->counters[this->ctx_w[3].w[2 - sel3] + ctx2 - this->ctx_w[3].w[sel3]];
      opp3[0].c[slot_f] += 7;
      opp3[0].total += 7;
      w13 = this->ctx_w[3].w[1];
      x3 = w13 + this->ctx[0] - this->ctx_w[3].w[this->ctx_w[3].sel];
      alti3 = ctx_alt + w13 - this->ctx_w[3].w[2 - this->ctx_w[3].sel];
      midn3 = &this->counters[x3];
      midn3[0].c[slot_f] += 6;
      midn3[0].total += 6;
      altn3 = &this->counters[alti3];
      altn3[0].c[slot_r] += 4;
      altn3[0].total += 4;
      ctx3 = this->ctx[0];
      if ( (ctx3 & 7) != 7 )
      {
        midn3[1].c[slot_f] += 4;
        midn3[1].total += 4;
        altn3[1].c[slot_r] += 2;
        altn3[1].total += 2;
        ctx3 = this->ctx[0];
      }
      if ( (ctx3 & 7) != 0 )
      {
        midn3[-1].c[slot_f] += 3;
        midn3[-1].total += 3;
        altn3[-1].c[slot_r] += 2;
        altn3[-1].total += 2;
        ctx3 = this->ctx[0];
      }
    }
    sel4 = this->ctx_w[4].sel;
    if ( sel4 == 1 )
    {
      mid4 = ctx3 - this->ctx_w[4].w[1];
      lo4 = &this->counters[mid4 + this->ctx_w[4].w[0]];
      hi4 = &this->counters[this->ctx_w[4].w[2] + mid4];
      lo4[0].c[slot_f] += 6;
      lo4[0].total += 6;
      hi4[0].c[slot_f] += 6;
      hi4[0].total += 6;
      ctx4 = this->ctx[0];
      if ( (ctx4 & 7) != 7 )
      {
        lo4[1].c[slot_f] += 4;
        lo4[1].total += 4;
        hi4[1].c[slot_f] += 4;
        hi4[1].total += 4;
        ctx4 = this->ctx[0];
      }
      if ( (ctx4 & 7) != 0 )
      {
        lo4[-1].c[slot_f] += 3;
        lo4[-1].total += 3;
        hi4[-1].c[slot_f] += 3;
        hi4[-1].total += 3;
        ctx4 = this->ctx[0];
      }
    }
    else
    {
      opp4 = &this->counters[this->ctx_w[4].w[2 - sel4] + ctx3 - this->ctx_w[4].w[sel4]];
      opp4[0].c[slot_f] += 7;
      opp4[0].total += 7;
      w14 = this->ctx_w[4].w[1];
      x4 = w14 + this->ctx[0] - this->ctx_w[4].w[this->ctx_w[4].sel];
      alti4 = ctx_alt + w14 - this->ctx_w[4].w[2 - this->ctx_w[4].sel];
      midn4 = &this->counters[x4];
      midn4[0].c[slot_f] += 6;
      midn4[0].total += 6;
      altn4 = &this->counters[alti4];
      altn4[0].c[slot_r] += 4;
      altn4[0].total += 4;
      ctx4 = this->ctx[0];
      if ( (ctx4 & 7) != 7 )
      {
        midn4[1].c[slot_f] += 4;
        midn4[1].total += 4;
        altn4[1].c[slot_r] += 2;
        altn4[1].total += 2;
        ctx4 = this->ctx[0];
      }
      if ( (ctx4 & 7) != 0 )
      {
        midn4[-1].c[slot_f] += 3;
        midn4[-1].total += 3;
        altn4[-1].c[slot_r] += 2;
        altn4[-1].total += 2;
        ctx4 = this->ctx[0];
      }
    }
    sel5 = this->ctx_w[5].sel;
    if ( sel5 == 1 )
    {
      mid5 = ctx4 - this->ctx_w[5].w[1];
      lo5 = &this->counters[mid5 + this->ctx_w[5].w[0]];
      hi5 = &this->counters[this->ctx_w[5].w[2] + mid5];
      lo5[0].c[slot_f] += 6;
      lo5[0].total += 6;
      hi5[0].c[slot_f] += 6;
      hi5[0].total += 6;
      ctx5 = this->ctx[0];
      if ( (ctx5 & 7) != 7 )
      {
        lo5[1].c[slot_f] += 4;
        lo5[1].total += 4;
        hi5[1].c[slot_f] += 4;
        hi5[1].total += 4;
        ctx5 = this->ctx[0];
      }
      if ( (ctx5 & 7) != 0 )
      {
        lo5[-1].c[slot_f] += 3;
        lo5[-1].total += 3;
        hi5[-1].c[slot_f] += 3;
        hi5[-1].total += 3;
        ctx5 = this->ctx[0];
      }
    }
    else
    {
      opp5 = &this->counters[this->ctx_w[5].w[2 - sel5] + ctx4 - this->ctx_w[5].w[sel5]];
      opp5[0].c[slot_f] += 7;
      opp5[0].total += 7;
      w15 = this->ctx_w[5].w[1];
      x5 = w15 + this->ctx[0] - this->ctx_w[5].w[this->ctx_w[5].sel];
      alti5 = ctx_alt + w15 - this->ctx_w[5].w[2 - this->ctx_w[5].sel];
      midn5 = &this->counters[x5];
      midn5[0].c[slot_f] += 6;
      midn5[0].total += 6;
      altn5 = &this->counters[alti5];
      altn5[0].c[slot_r] += 4;
      altn5[0].total += 4;
      ctx5 = this->ctx[0];
      if ( (ctx5 & 7) != 7 )
      {
        midn5[1].c[slot_f] += 4;
        midn5[1].total += 4;
        altn5[1].c[slot_r] += 2;
        altn5[1].total += 2;
        ctx5 = this->ctx[0];
      }
      if ( (ctx5 & 7) != 0 )
      {
        midn5[-1].c[slot_f] += 3;
        midn5[-1].total += 3;
        altn5[-1].c[slot_r] += 2;
        altn5[-1].total += 2;
        ctx5 = this->ctx[0];
      }
    }
    sel6 = this->ctx_w[6].sel;
    if ( sel6 == 1 )
    {
      mid6 = ctx5 - this->ctx_w[6].w[1];
      lo6 = &this->counters[mid6 + this->ctx_w[6].w[0]];
      hi6 = &this->counters[this->ctx_w[6].w[2] + mid6];
      lo6[0].c[slot_f] += 6;
      lo6[0].total += 6;
      hi6[0].c[slot_f] += 6;
      hi6[0].total += 6;
      ctx6 = this->ctx[0];
      if ( (ctx6 & 7) != 7 )
      {
        lo6[1].c[slot_f] += 4;
        lo6[1].total += 4;
        hi6[1].c[slot_f] += 4;
        hi6[1].total += 4;
        ctx6 = this->ctx[0];
      }
      if ( (ctx6 & 7) != 0 )
      {
        lo6[-1].c[slot_f] += 3;
        lo6[-1].total += 3;
        hi6[-1].c[slot_f] += 3;
        hi6[-1].total += 3;
        ctx6 = this->ctx[0];
      }
    }
    else
    {
      opp6 = &this->counters[this->ctx_w[6].w[2 - sel6] + ctx5 - this->ctx_w[6].w[sel6]];
      opp6[0].c[slot_f] += 7;
      opp6[0].total += 7;
      w16 = this->ctx_w[6].w[1];
      x6 = w16 + this->ctx[0] - this->ctx_w[6].w[this->ctx_w[6].sel];
      alti6 = ctx_alt + w16 - this->ctx_w[6].w[2 - this->ctx_w[6].sel];
      midn6 = &this->counters[x6];
      midn6[0].c[slot_f] += 6;
      midn6[0].total += 6;
      altn6 = &this->counters[alti6];
      altn6[0].c[slot_r] += 4;
      altn6[0].total += 4;
      ctx6 = this->ctx[0];
      if ( (ctx6 & 7) != 7 )
      {
        midn6[1].c[slot_f] += 4;
        midn6[1].total += 4;
        altn6[1].c[slot_r] += 2;
        altn6[1].total += 2;
        ctx6 = this->ctx[0];
      }
      if ( (ctx6 & 7) != 0 )
      {
        midn6[-1].c[slot_f] += 3;
        midn6[-1].total += 3;
        altn6[-1].c[slot_r] += 2;
        altn6[-1].total += 2;
        ctx6 = this->ctx[0];
      }
    }
    sel7 = this->ctx_w[7].sel;
    if ( sel7 == 1 )
    {
      mid7 = ctx6 - this->ctx_w[7].w[1];
      lo7 = &this->counters[mid7 + this->ctx_w[7].w[0]];
      hi7 = &this->counters[this->ctx_w[7].w[2] + mid7];
      lo7[0].c[slot_f] += 6;
      lo7[0].total += 6;
      hi7[0].c[slot_f] += 6;
      hi7[0].total += 6;
      ctx7 = this->ctx[0];
      if ( (ctx7 & 7) != 7 )
      {
        lo7[1].c[slot_f] += 4;
        lo7[1].total += 4;
        hi7[1].c[slot_f] += 4;
        hi7[1].total += 4;
        ctx7 = this->ctx[0];
      }
      if ( (ctx7 & 7) != 0 )
      {
        lo7[-1].c[slot_f] += 3;
        lo7[-1].total += 3;
        hi7[-1].c[slot_f] += 3;
        hi7[-1].total += 3;
        ctx7 = this->ctx[0];
      }
    }
    else
    {
      opp7 = &this->counters[this->ctx_w[7].w[2 - sel7] + ctx6 - this->ctx_w[7].w[sel7]];
      opp7[0].c[slot_f] += 7;
      opp7[0].total += 7;
      w17 = this->ctx_w[7].w[1];
      x7 = w17 + this->ctx[0] - this->ctx_w[7].w[this->ctx_w[7].sel];
      alti7 = ctx_alt + w17 - this->ctx_w[7].w[2 - this->ctx_w[7].sel];
      midn7 = &this->counters[x7];
      midn7[0].c[slot_f] += 6;
      midn7[0].total += 6;
      altn7 = &this->counters[alti7];
      altn7[0].c[slot_r] += 4;
      altn7[0].total += 4;
      ctx7 = this->ctx[0];
      if ( (ctx7 & 7) != 7 )
      {
        midn7[1].c[slot_f] += 4;
        midn7[1].total += 4;
        altn7[1].c[slot_r] += 2;
        altn7[1].total += 2;
        ctx7 = this->ctx[0];
      }
      if ( (ctx7 & 7) != 0 )
      {
        midn7[-1].c[slot_f] += 3;
        midn7[-1].total += 3;
        altn7[-1].c[slot_r] += 2;
        altn7[-1].total += 2;
        ctx7 = this->ctx[0];
      }
    }
    sel8 = this->ctx_w[8].sel;
    if ( sel8 == 1 )
    {
      midn8 = ctx7 - this->ctx_w[8].w[1];
      altn8 = &this->counters[midn8 + this->ctx_w[8].w[0]];
      node = &this->counters[this->ctx_w[8].w[2] + midn8];
      altn8[0].c[slot_f] += 6;
      altn8[0].total += 6;
      node[0].c[slot_f] += 6;
      node[0].total += 6;
      ctx8 = this->ctx[0];
      if ( (ctx8 & 7) != 7 )
      {
        altn8[1].c[slot_f] += 4;
        altn8[1].total += 4;
        node[1].c[slot_f] += 4;
        node[1].total += 4;
        ctx8 = this->ctx[0];
      }
      if ( (ctx8 & 7) != 0 )
      {
        altn8[-1].c[slot_f] += 3;
        altn8[-1].total += 3;
        node[-1].c[slot_f] += 3;
        node[-1].total += 3;
      }
    }
    else
    {
      opp8 = &this->counters[this->ctx_w[8].w[2 - sel8] + ctx7 - this->ctx_w[8].w[sel8]];
      opp8[0].c[slot_f] += 7;
      opp8[0].total += 7;
      w18 = this->ctx_w[8].w[1];
      node = &this->counters[w18 + this->ctx[0] - this->ctx_w[8].w[this->ctx_w[8].sel]];
      x8 = &this->counters[w18 - this->ctx_w[8].w[2 - this->ctx_w[8].sel] + ctx_alt];
      node[0].c[slot_f] += 6;
      node[0].total += 6;
      x8[0].c[slot_r] += 4;
      x8[0].total += 4;
      alti8 = this->ctx[0];
      if ( (alti8 & 7) != 7 )
      {
        node[1].c[slot_f] += 4;
        node[1].total += 4;
        x8[1].c[slot_r] += 2;
        x8[1].total += 2;
        alti8 = this->ctx[0];
      }
      if ( (alti8 & 7) != 0 )
      {
        node[-1].c[slot_f] += 3;
        node[-1].total += 3;
        result = (uint16_t)x8[-1].c[slot_r] + 2;
        x8[-1].c[slot_r] = result;
        x8[-1].total += 2;
      }
    }
  }
  return result;
}

int32_t *__alt_p1_alloc(AltP1Block *_this, int32_t img_w, int32_t img_h, int32_t plane)
{
  ;
  int32_t lvl, grp, k, lvl1, odd, grp1, slot, slot1, bump, wid, r, plane_hi;
  uint32_t ctr, pair, row;
  ctr = 0;
  _this->width = img_w;
  _this->height = img_h;
  do
    __init_counter_node(&_this->counters[ctr++]);
  while ( ctr < 0x99C60 );
  _this->pred = 0;
  lvl = 0;
  grp = 0;
  k = 0;
  plane_hi = plane << 8;
  do
  {
    _this->level_of[2 * k] = lvl;
    lvl1 = (2 * k == p1_level_edges[lvl]) + lvl;
    _this->group_of[2 * k] = plane_hi | grp;
    _this->level_of[2 * k + 1] = lvl1;
    odd = 2 * k + 1;
    grp1 = (2 * k == p1_group_edges[grp]) + grp;
    lvl = (odd == p1_level_edges[lvl1]) + lvl1;
    _this->group_of[2 * k + 1] = grp1 | plane_hi;
    grp = (odd == p1_group_edges[grp1]) + grp1;
    ++k;
  }
  while ( (uint32_t)k < 0x100 );
  pair = 0;
  slot = 0;
  do
  {
    _this->slot_of[2 * pair] = 8 * slot;
    slot1 = (2 * pair == p1_slot_edges[slot]) + slot;
    _this->slot_of[2 * pair + 1] = 8 * slot1;
    bump = 2 * pair++ + 1 == p1_slot_edges[slot1];
    slot = bump + slot1;
  }
  while ( pair < 0x80 );
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
  row = 0;
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
    _this->buf[row++] = (P1Ctx *)bmf_new(2 * _this->width + 20);
  while ( row < 5 );
  __alt_init_tables(_this->fold, (int8_t *)_this->unfold);
  wid = _this->width;
  if ( _this->width > -10 )
  {
    r = 0;
    do
    {
      _this->buf[4][r].sym = 72;
      _this->buf[3][r].sym = 72;
      _this->buf[2][r].sym = 72;
      _this->buf[1][r].sym = 72;
      _this->buf[0][r].sym = 72;
      _this->buf[4][r].mag = 0;
      _this->buf[3][r].mag = 0;
      _this->buf[2][r].mag = 0;
      _this->buf[1][r].mag = 0;
      _this->buf[0][r].mag = 0;
      wid = _this->width;
      ++r;
    }
    while ( r < _this->width + 10 );
  }
  _this->cursor[0] = _this->buf[0] + wid + 4;
  _this->cursor[1] = _this->buf[1] + wid + 4;
  _this->cursor[2] = _this->buf[2] + wid + 4;
  _this->cursor[3] = _this->buf[3] + wid + 4;
  _this->cursor[4] = _this->buf[4] + wid + 4;
  return (int32_t *)_this;
}

uint16_t *__rc_begin_encode()
{
  ;
  uint16_t *tbl, *row;
  FreqPair *seed;   // the row's tree of counter pairs, seeded (60, 36)
  int32_t at4, at5, k;
  int32_t bits;          // the same slot as the buffer pointer below, in a
  uint8_t *at;       // register MSVC reused; two roles, two names
  int32_t at6, at7;
  uint32_t i;   // offsets into model_geometry, not pointers
  *packer_word = ::packer_acc;
  at = out_cursor;
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
        --at;
        bits -= 8;
      }
      while ( bits >= 0 );
      out_cursor = at;
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
    at4 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    memset(&model_geometry[at4], 4, 8);
    level_geom[5].half = 8;
    at5 = at4 + 2 * level_geom[4].half;
    level_geom[5].first = at5;
    level_geom[5].tbl_base = at5 - 5;
    memset(&model_geometry[at5], 0x05, 16);
    level_geom[6].half = 16;
    at6 = at5 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)at6;
    level_geom[6].tbl_base = (uint8_t)at6 - 6;
    memset(&model_geometry[at6], 0x06, 32);
    level_geom[7].half = 32;
    at7 = at6 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)at7;
    level_geom[7].tbl_base = (uint8_t)at7 - 7;
    // 64 bytes of 7, after 16 of 5 and 32 of 6 -- one level per line.  MSVC
    // inlined this third one because the length crossed its threshold, which
    // is why it arrived as a scalar head, three aligned stores and a tail.
    memset(&model_geometry[at7], 0x07, 64);
    // 0x400 rows of 508 bytes is 0x7F000 exactly: one seeded counter table per
    // context, each a header of ten `uint16_t` and then 0x7A pairs of (60, 36).
    // The header's first two words -- 635 and `24 * alt_freq_limit` -- are the
    // total and the escape weight; the eight after them are a fixed
    // distribution, 205 124 147 83 48 16 8 4, halving away from the front.
    tbl = (uint16_t *)bmf_new(0x7F000u);
    if ( tbl )
    {
      k = 0;
      row = tbl;
      do
      {
        row[1] = 24 * alt_freq_limit;
        row[2] = 205;
        row[6] = 48;
        row[3] = 124;
        row[7] = 16;
        row[4] = 147;
        row[5] = 83;
        row[8] = 8;
        row[9] = 4;
        *row = 635;
        // Word 10 is `bit_tree` element 1: the tree is 1-indexed and its
        // element 0 is the last two words of the header seeded above.
        seed = (FreqPair *)&row[10];
        for ( i = 0; i < 0x7A; ++i )
        {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
        row += 254;
        ++k;
      }
      while ( (uint32_t)k < 0x400 );
    }
    // An `else { tbl = nullptr; }` stood here, assigning null to what was
    // already null.
    ::model_tables = tbl;
  }
  rc.enc_init();
  // Only assigned under `plane_alt_model`, so `-Wmaybe-uninitialized` flags
  // it -- correctly, and it is left alone for the same reason `predict_med`'s
  // return is: no caller reads it, and seeding it would invent a value the
  // original does not have.
  return tbl;
}

inline void AltP1Block::d8_encode_body(uint8_t *src, uint8_t *out)
{
  P1Ctx *cursor2, *cursor4, *cursor0, *b4, *buf3, *buf2, *buf1, *b0;
  ;
  uint8_t pred, recon;
  int32_t y, resid, code, drift, val, x;
  // The five planes, held across the rotation that ends a row, and the cursor
  // the row is written through.
  int64_t err;
  uint8_t *q;
  __rc_begin_encode();
  if ( this->height > 0 )
  {
    y = 0;
    do
    {
      ++y;
      // The row end mirrored into the right margin: records 0..5 take -1..-6.
      // Two bytes each, which is what the twelve even offsets were.
      {
        P1Ctx *const here = (P1Ctx *)this->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      b4 = this->buf[4];
      buf3 = this->buf[3];
      buf2 = this->buf[2];
      buf1 = this->buf[1];
      b0 = this->buf[0];
      this->buf[4] = buf3;
      this->buf[3] = buf2;
      this->buf[2] = buf1;
      this->buf[1] = b0;
      this->buf[0] = b4;
      b4 += 4;
      this->cursor[0] = b4;
      b0 += 4;
      this->cursor[1] = b0;
      this->cursor[2] = buf1 + 4;
      this->cursor[3] = buf2 + 4;
      this->cursor[4] = buf3 + 4;
      ((P1Ctx *)b4)[-4] = ((P1Ctx *)b0)[3];
      // And the rest of the new row's left margin, from the row above.
      {
        P1Ctx *const here = (P1Ctx *)this->cursor[0];
        P1Ctx *const up   = (P1Ctx *)this->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = this->cursor[2];
      cursor4 = this->cursor[4];
      this->ctx[2] = 0;
      cursor0 = this->cursor[0];
      // Ten `.mag` terms each, from the two rows above and this row's left
      // margin: `ctx[3]` takes the even offsets and `ctx[4]` the odd.  MSVC
      // kept both running sums in registers and stored every partial back, so
      // this arrived as twenty stores of which eighteen were dead, and
      // nineteen locals holding one accumulator at nineteen points.
      this->ctx[3] = cursor2[-2].mag + cursor4[-2].mag
                    + cursor2[0].mag  + cursor4[0].mag
                    + cursor2[2].mag  + cursor4[2].mag
                    + cursor2[4].mag  + cursor4[4].mag
                    + cursor0[-4].mag + cursor0[-2].mag;
      this->ctx[4] = cursor2[-1].mag + cursor4[-1].mag
                    + cursor2[1].mag  + cursor4[1].mag
                    + cursor2[3].mag  + cursor4[3].mag
                    + cursor2[5].mag  + cursor4[5].mag
                    + cursor0[-3].mag + cursor0[-1].mag;
      if ( !(this->width <= 0) )
      {
        q = out;
        x = 0;
        do
        {
          ++x;
          (this)->ctx_of((AltP1Block *)nullptr, (AltP1Block *)0);
          pred = (uint8_t)this->pred;
          resid = (uint8_t)(*src - pred);
          recon = *(this->fold[resid] + this->unfold) + pred;
          code = this->fold[resid];
          drift = (uint8_t)*q - (uint8_t)(recon + *q - *src);
          if ( drift < -16 || drift > 16 )
          {
            *q = *src;
            code = this->fold_hi[resid];
          }
          else
          {
            *q = recon;
          }
          __alt_p1_encode_symbol(&this->counters[this->ctx[0]].total, 16 * this->ctx[0], this->ctx[1], code);
          val = (uint8_t)*q;
          err = val - this->pred;
          this->cursor[0]->sym = val;
          this->cursor[0]->mag = (BYTE4(err) ^ err) - BYTE4(err);
          this->ctx[3 + this->ctx[2]] = this->ctx[3 + this->ctx[2]]
                                                              + this->cursor[0]->mag
                                                              - this->cursor[0][-4].mag
                                                              - (this->cursor[4][-2].mag
                                                               - this->cursor[4][6].mag
                                                               + this->cursor[2][-2].mag
                                                               - this->cursor[2][6].mag);
                  this->ctx[2] = this->ctx[2] == 0;
          if ( this->counters[this->ctx[0]].total < 0x4000u )
            this->update_model();
          ++this->cursor[0];
          ++q;
          ++this->cursor[1];
          ++this->cursor[2];
          ++this->cursor[3];
          ++this->cursor[4];
          ++src;
        }
        while ( x < this->width );
        out = q;
      }
    }
    while ( (uint32_t)y < *(uint32_t *)&this->height );
  }
  __rc_end_encode();
}

void __alt_model_p1_d8_encode(uint8_t *src, int32_t i, int32_t height, uint8_t *out)
{
  ;
  AltP1Block *raw;
  void **blk;
  raw = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
  if ( raw )
    blk = (void **)__alt_p1_alloc((AltP1Block *)raw, i, height, 0);
  else
    blk = nullptr;
  ((AltP1Block *)blk)->d8_encode_body(src, out);
  if ( blk )
    __alt_p1_free((void **)blk, 1);
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

// SSE2 is a given here -- the bodies are full of it and would fault long
// before this ran -- which settles all three questions: the level test is
// always taken, DAZ has been available on every part that has SSE2, and the
// no-SSE2 exit is unreachable.  What is left is the two mode bits, and
// <xmmintrin.h> and <pmmintrin.h> already name them.

// main passed 3 -- flush-to-zero and denormals-are-zero, not the third bit --
// so these two lines are the whole of what it did.
static void bmf_set_denormal_mode()
{
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}


uint8_t *__alt_p2_alloc(AltP2Block *_this, int32_t img_w, int32_t plane)
{
  ;
  uint32_t done, e;
  void *buf1;
  int32_t dz, band, row_bytes, lvl, lvl1, bump, len, len1;
  uint32_t j, k, ctr, pairs, p, row, pair, n;
  _this->plane_idx = plane;
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
  ctr = 0;
  do
  {
    // Two records a pass, 0x1E60 passes: 15552 of them.
    e = 2 * ctr;
    _this->freq[e].f[0] = 2048;
    ++ctr;
    _this->freq[e].f[1] = 2816;
    _this->freq[e].f[2] = 2816;
    _this->freq[e].step = 4096;
    _this->freq[e + 1].f[0] = 2048;
    _this->freq[e + 1].f[1] = 2816;
    _this->freq[e + 1].f[2] = 2816;
    _this->freq[e + 1].step = 4096;
  }
  while ( ctr < 0x1E60 );
  dz = 4 * plane_desc[0].w12 + 1;
  band = 16 * plane_desc[0].w12;
  *(uint32_t *)&_this->has_ref = (uint8_t)(plane_desc[plane_desc[_this->plane_idx + 1].src_plane + 1].flags
                                               & 8) >> 3;
  deadzone_hi = dz;
  deadzone_lo = -dz;
  *(uint32_t *)&_this->band_lo = -band - 7;
  *(uint32_t *)&_this->band_hi = band + 8;
  _this->row0 = (int32_t *)bmf_new(4 * img_w + 16);
  buf1 = bmf_new(4 * img_w + 16);
  // Byte 232 is row 14 lane 2 of the first neighbourhood's weight block --
  // `14 * 16 + 2 * 4` -- which is the slot `alt_p2_context` and `alt_p2_model`
  // both read as the scale on their update floor.  Seeding it to 1 is what
  // stops the first neighbourhood dividing by nothing.
  ((float (*)[4])_this)[14][2] = 1.0f;
  _this->row1 = (int32_t *)buf1;
  _this->cur = _this->row0 + img_w + 2;
  if ( img_w > -4 )
  {
    pairs = (img_w + 4) / 2;
    if ( pairs )
    {
      for ( p = 0; p < pairs; ++p )
      {
        *(uint8_t **)&_this->row1[2 * p] = (uint8_t *)_this;
        *(uint8_t **)&_this->row0[2 * p] = (uint8_t *)_this;
        *(uint8_t **)&_this->row1[2 * p + 1] = (uint8_t *)_this;
        *(uint8_t **)&_this->row0[2 * p + 1] = (uint8_t *)_this;
      }
      done = 2 * p + 1;
    }
    else
    {
      done = 1;
    }
    if ( (uint32_t)(img_w + 4) > (done - 1) )
    {
      *(uint8_t **)&_this->row1[done - 1] = (uint8_t *)_this;
      *(uint8_t **)&_this->row0[done - 1] = (uint8_t *)_this;
    }
  }
  row = 0;
  row_bytes = 18 * img_w + 234;
  do
    _this->buf[row++] = (P2Ctx *)bmf_new(row_bytes);
  while ( row < 5 );
  memset(_this->buf[0],0,row_bytes);
  ctx_bias[3] = 0;
  lvl = 0;
  ctx_bias[2] = 0;
  ctx_bias[1] = 0;
  pair = 0;
  ctx_bias[0] = 0;
  _this->cursor[0] = _this->buf[0] + 8;
  do
  {
    *(uint32_t *)&_this->ctx_delta[2 * pair] = (_this->plane_idx << 8) | (16 * lvl);
    lvl1 = (2 * pair == p2_ctx_edges[lvl]) + lvl;
    *(uint32_t *)&_this->ctx_delta[2 * pair + 1] = (_this->plane_idx << 8) | (16 * lvl1);
    bump = 2 * pair++ + 1 == p2_ctx_edges[lvl1];
    lvl = bump + lvl1;
  }
  while ( pair < 0x82 );
  len = 0;
  for ( n = 0; n < 0x3C; ++n )
  {
    _this->nb_ctx[2 * n] = len;
    len1 = (2 * n == p2_len_edges[len]) + len;
    _this->nb_ctx[2 * n + 1] = len1;
    len = (2 * n + 1 == p2_len_edges[len1]) + len1;
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

// This one is not documented anywhere -- it is BMF's own -- but it does not
// need to be inferred from the offsets its readers touch, because
// `alloc_image` writes all four words in a row and the arithmetic around them
// says what each is:

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

int32_t *__alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed)
{
  ;
  uint8_t *buf;
  int32_t *result;
  bool byte_rows;
  int32_t bits, word2, row_pack, row_bytes, data_bytes, pal_bytes;
  uint16_t row16;
  row16 = img_w;
  bits = bpp;
  word2 = (uint8_t)bpp << 16;
  // How wide a row is, in bytes.  Four cases, and the decompilation reached
  // them with four `goto`s -- one of them jumping two blocks deep into the
  // middle of the `!packed` arm, which is why the arms did not look like
  // cases at all:
  
  //   bpp 5..7          one byte a pixel, and `row16` already holds `img_w`
  //   bits >= 8         `(bits + 7) / 8` bytes a pixel
  //   unpacked, 4 bits  two pixels a byte, rounded up to four bytes
  //   packed, 1/2/4     the bit-packed widths, and the 0x40000000 tag
  
  // `bpp == 3` never takes the second: the test for it sits in the other arm
  // of the `bpp == 3` branch, which is what `byte_rows` records.
  if ( bpp >= 5 && bpp <= 7 )
  {
    row_bytes = row16;
  }
  else
  {
    if ( bpp == 3 )
      bits = 4;
    else if ( !packed )
      bits = bpp > 4 ? bpp : 4;
    byte_rows = bpp != 3 && bits >= 8;

    if ( !packed )
    {
      if ( !byte_rows && bits == 4 )
        // The mask was `0xFFFFFFFC` while this was a 32-bit register and the
        // truncation happened at the read.  Sixteen bits is where it lands,
        // so sixteen is what the mask says, and the compiler can then see the
        // value fits instead of warning that it might not.
        row16 = ((img_w + 7) >> 1) & 0xFFFC;
      else
        row16 = (uint16_t)(((bits + 7) >> 3) * img_w);
      row_bytes = row16;
    }
    else if ( byte_rows )
    {
      row16 = (uint16_t)(((bits + 7) >> 3) * img_w);
      row_bytes = row16;
    }
    else
    {
      if ( bits == 1 )
      {
        row_pack = (int32_t)(((uint32_t)((img_w + 7) >> 2) >> 29) + img_w + 7) >> 3;
      }
      else if ( bits == 2 )
      {
        row_pack = (int32_t)(((uint32_t)((img_w + 3) >> 1) >> 30) + img_w + 3) >> 2;
      }
      else
      {
        row_pack = (int32_t)(img_w + ((uint32_t)(img_w + 1) >> 31) + 1) >> 1;
      }
      word2 = ((uint8_t)bpp << 16) | 0x40000000;
      row_bytes = (uint16_t)row_pack;
    }
  }
  data_bytes = row_bytes * img_h;
  if ( palette )
  {
    pal_bytes = 3 << (bpp & 31);
    if ( bpp > 8 )
      pal_bytes = 0;
    else
      word2 = ((BYTE2(word2) | 0xFFFF0080) << 16) | word2 & 0xFF00FFFF;
  }
  else
  {
    pal_bytes = 0;
  }
  result = ((int32_t *)bmf_new(data_bytes + pal_bytes + 19));
  if ( !result )
    return nullptr;
  // The descriptor, and the reason BmfImage looks the way it does.  Three of
  // these four words are one field each and say so now; `result[2]` is not,
  // and stays a packed store: it covers +8 through +11, and the two bytes at
  // +8 are zero only because they are written as part of it.  Splitting it
  // would mean zeroing them separately, which is more code saying less.
  BmfImage *const img = (BmfImage *)result;
  img->width = img_w;
  img->height = img_h;
  img->stride = row_bytes;
  result[2] = word2;                 // +8 and +9 zero, depth at +10, flags at +11
  img->data_size = data_bytes;
  if ( pal_bytes )
  {
    if ( (((const BmfImage *)result)->depth & 0x80) != 0 )
      buf = (uint8_t *)result + result[3] + 16;
    else
      buf = nullptr;
    memset(buf,0,pal_bytes);
    return result;
  }
  return result;
}

// The two headers a .bmp file begins with: a 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER after it.

// Unlike the ObjN structs this is not recovered from the offsets the code
// happens to touch -- it is the documented layout of the format, and the code
// agrees with it at every offset it uses.  `biSize = 40` at +14 is the one that
// settles it: that field exists to say which info header this is, and 40 is
// this one.  read_bmp checks the same two numbers on the way in, `'BM'` at +0
// and 40 at +14.

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

int32_t __write_bmp(uintptr_t img_addr, char *path, int32_t want_rle)
{
  // img_addr, img_at and pix are the same descriptor -- `img_at = img_addr` and
  // `pix = (uint16_t *)img_addr`, and none is stepped -- so one view serves all
  // three.  Where a read of +12 was typed `uint8_t *` it stays a value cast back
  // from the size, because that is what the code then does with it: `&x[img_addr]`
  // with x the size and img_addr the descriptor is `img_addr + data_size`, which is where
  // alloc_image put the palette.
  BmfImage *const img = (BmfImage *)img_addr;
  uint32_t Buffera;
  // These shared `Buffera` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t levels;
  uint8_t *off_bits, *abs_end;
  int32_t rows2;
  uint8_t *buf_2;
  uint32_t end;
  uint8_t *data_ofs, *out;
  uint32_t stride, pairs, done;
  int32_t ncol, pal_bytes;
  uint8_t *buf, *abs_end2;
  int32_t row_i2, run_max, nib;
  // These shared `pairs` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `ncol` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `ncol` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  ;
  uintptr_t img_at;   // were int32_t: addresses, masked and tagged
  FILE *fp, *fp2;
  bool can_rle;
  uint32_t stride1, at, at2, slot;
  uint16_t *pix;
  uint8_t *out_buf, *data_ofs2, *pal, *pal2, *out_at, *p, *q, *out_end,
          *out2;
  int32_t rle_on, i, rows, bits, ncolours, grey, bgr, r, bgr2, rle_mode,
          src_bits, rle_kind, rows3, row_i, lit_len, byte, run, coded_bytes, y;
  uint32_t k, j, data_len, stride3, n_bytes, pad;
  rle_on = want_rle;
  fp = fopen(path, "wb");
  if ( !fp )
    return 0;
  out_buf = (uint8_t *)bmf_new(img->data_size
                                 + 8 * img->height
                                 + (img->data_size >> 5) + 2048);
  img_at = img_addr;
  // out, out_buf, out2 and out_buf are one allocation: the chain
  // is out_buf = out_buf, out = out_buf, out2 = out, and
  // none of them is ever stepped.  So one view of the header serves all four,
  // and the pixel writes through out_buf[k + 54] keep the spelling they had.
  BmpHeader *bmp = (BmpHeader *)out_buf;
  i = img->width;
  bmp->biSize = 40;
  bmp->bfType = 0x4D42 /* 'BM' */;
  rows = img->height;
  rows2 = rows;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = i;
  bmp->biHeight = rows;
  LOBYTE(rows) = img->depth;
  (*((int8_t *)&Buffera)) = rows;
  bmp->biPlanes = 1;
  bits = rows & 0x3F;
  bmp->biBitCount = bits;
  bmp->biClrImportant = 0;
  bmp->biClrUsed = 0;
  bmp->biYPelsPerMeter = 0;
  bmp->biXPelsPerMeter = 0;
  buf = out_buf + 54;
  if ( bits <= 8 )
  {
    ncolours = 1 << (bits & 31);
    ncol = 1 << (bits & 31);
    if ( ((*((int8_t *)&Buffera)) & 0x40) != 0 )
    {
      levels = 0x100u >> (bits & 31);
      if ( ncol > 0 )
      {
        if ( ncol / 2 )
        {
          k = 0;
          grey = 0;
          do
          {
            *(uint32_t *)&out_buf[8 * k + 54] = ((uint8_t)grey << 16)
                                                | (uint8_t)grey
                                                | ((uint8_t)grey << 8);
            *(uint32_t *)&out_buf[8 * k + 58] = ((uint8_t)(grey - levels + 2 * levels) << 16)
                                                | (uint8_t)(grey - levels + 2 * levels)
                                                | ((uint8_t)(grey - levels + 2 * levels) << 8);
            grey += 2 * levels;
            ++k;
          }
          while ( k < (uint32_t)(ncol / 2) );
          img_at = img_addr;
          at = 2 * k + 1;
        }
        else
        {
          at = 1;
        }
        if ( (at - 1) < (uint32_t)ncol )
          *(uint32_t *)&out_buf[4 * at + 50] = ((uint8_t)(levels * (at - 1)) << 16)
                                              | (uint8_t)(levels * (at - 1))
                                              | ((uint8_t)(levels * (at - 1)) << 8);
      }
      data_ofs2 = (uint8_t *)(uintptr_t)img->data_size;
      pal_bytes = 4 * ncol;
    }
    else if ( (*((int8_t *)&Buffera)) < 0 )
    {
      data_ofs2 = (uint8_t *)(uintptr_t)img->data_size;
      if ( ncol <= 0 )
      {
        pal_bytes = 4 * ncolours;
      }
      else
      {
        pairs = ncolours / 2;
        if ( ncolours / 2 )
        {
          data_ofs = (uint8_t *)(uintptr_t)img->data_size;
          j = 0;
          pal = &data_ofs2[img_addr];
          do
          {
            slot = 2 * j;
            bgr = *(uint16_t *)&pal[6 * j + 19];
            r = (uint8_t)pal[6 * j + 21];
            *(uint32_t *)&out_buf[4 * slot + 54] = ((uint8_t)pal[6 * j + 18] << 16)
                                                | *(uint16_t *)&pal[6 * j + 16];
            *(uint32_t *)&out_buf[4 * slot + 58] = (r << 16) | bgr;
            ++j;
          }
          while ( j < pairs );
          data_ofs2 = data_ofs;
          img_at = img_addr;
          rle_on = want_rle;
          at2 = 2 * j + 1;
          done = at2;
        }
        else
        {
          at2 = 1;
          done = 1;
        }
        if ( (at2 - 1) < (uint32_t)ncol )
        {
          pal2 = &data_ofs2[img_at];
          bgr2 = *(uint16_t *)&pal2[3 * done + 13];
          *(uint32_t *)&out_buf[4 * done + 50] = (((uint8_t)pal2[3 * done + 15]) << 16) | bgr2;
          img_at = img_addr;
        }
        pal_bytes = 4 * ncol;
      }
    }
    else
    {
      pal_bytes = 4 * ncolours;
      memset(buf,0,4 * ncolours);
      img_at = img_addr;
      data_ofs2 = (uint8_t *)(uintptr_t)img->data_size;
      rows2 = img->height;
    }
    buf = &out_buf[pal_bytes + 54];
  }
  else
  {
    data_ofs2 = (uint8_t *)(uintptr_t)img->data_size;
  }
  stride = img->stride;
  out = out_buf;
  off_bits = (uint8_t *)(buf - out_buf);
  pix = (uint16_t *)img_at;
  rle_mode = rle_on;
  data_len = (uint32_t)data_ofs2;
  while ( 1 )
  {
    out_at = buf;
    p = (uint8_t *)pix + data_len - stride + 16;
    bmp->bfOffBits = (uintptr_t)off_bits;
    if ( !rle_mode )
      break;
    src_bits = pix[5] & 0x3F;
    can_rle = src_bits == 4;
    if ( src_bits != 4 )
    {
      if ( src_bits != 8 )
        break;
      can_rle = 0;
    }
    rle_kind = 2;
    if ( !can_rle )
      rle_kind = 1;
    bmp->biCompression = rle_kind;
    nib = rle_kind - 1;
    run_max = (0x100u >> ((rle_kind - 1) & 31)) - 1;
    if ( rows2 > 0 )
    {
      rows3 = rows2;
      stride3 = stride;
      row_i = 0;
      while ( 1 )
      {
        if ( p >= &p[stride3] )
          goto LABEL_72;
        buf_2 = out_at;
        end = (uint32_t)&p[stride3];
        row_i2 = row_i;
        lit_len = 0;
        do
        {
          while ( 1 )
          {
            while ( 1 )
            {
              q = p + 1;
              if ( (uint32_t)(p + 1) >= end )
                break;
              byte = (uint8_t)*p;
              run = 1;
              do
              {
                if ( byte != (uint8_t)p[run] )
                  break;
                ++run;
              }
              while ( end > (uint32_t)&p[run] );
              if ( run <= 2 && (run != 2 || lit_len) )
                break;
              if ( run_max < run )
                run = run_max;
              if ( lit_len )
              {
                // Flush the literals collected so far.  An absolute run is
                // worth its two-byte header from two literals in 4-bit mode
                // and three in 8-bit, and below that the literals go out as
                // one- or two-pixel encoded runs instead.  `LABEL_67` was
                // that one condition written as a jump from inside the other
                // arm; the two spellings of it are `lit_len != 1` and `lit_len >= 3`.
                if ( nib ? lit_len != 1 : lit_len >= 3 )
                {
                  *buf_2 = 0;
                  abs_end2 = buf_2 + 2;
                  buf_2[1] = lit_len << (nib & 31);
                  memcpy(buf_2 + 2,&p[-lit_len],lit_len);
                  buf_2 += lit_len + 2;
                  if ( (lit_len & 1) != 0 )
                  {
                    abs_end2[lit_len] = 0;
                    ++buf_2;
                  }
                }
                else if ( nib )
                {
                  *buf_2 = 2;
                  buf_2[1] = *(p - 1);
                  buf_2 += 2;
                }
                else
                {
                  if ( lit_len == 2 )
                  {
                    *buf_2 = 1;
                    buf_2[1] = *(p - 2);
                    buf_2 += 2;
                  }
                  *buf_2 = 1;
                  buf_2[1] = *(p - 1);
                  buf_2 += 2;
                }
                lit_len = 0;
                LOBYTE(byte) = *p;
              }
              buf_2[1] = byte;
              p += run;
              *buf_2 = run << (nib & 31);
              out_end = buf_2 + 2;
              buf_2 += 2;
              if ( (uint32_t)p >= end )
              {
                out_at = out_end;
                row_i = row_i2;
                rows3 = img->height;
                stride3 = img->stride;
                goto LABEL_72;
              }
            }
            ++p;
            if ( ++lit_len != run_max )
              break;
            // The same flush as above, at the point where a literal run hits
            // its length cap, and the same one condition: two literals pay for
            // an absolute run in 4-bit mode, three in 8-bit.
            if ( nib ? lit_len != 1 : lit_len >= 3 )
            {
              *buf_2 = 0;
              abs_end = buf_2 + 2;
              buf_2[1] = lit_len << (nib & 31);
              memcpy(buf_2 + 2,&q[-lit_len],lit_len);
              buf_2 += lit_len + 2;
              if ( (lit_len & 1) != 0 )
              {
                abs_end[lit_len] = 0;
                ++buf_2;
              }
            }
            else if ( nib )
            {
              *buf_2 = 2;
              buf_2[1] = *(q - 1);
              buf_2 += 2;
            }
            else
            {
              if ( lit_len == 2 )
              {
                *buf_2 = 1;
                buf_2[1] = *(q - 2);
                buf_2 += 2;
              }
              *buf_2 = 1;
              buf_2[1] = *(q - 1);
              buf_2 += 2;
            }
            if ( (uint32_t)q >= end )
            {
              out_at = buf_2;
              row_i = row_i2;
              rows3 = img->height;
              stride3 = img->stride;
              goto LABEL_72;
            }
            lit_len = 0;
          }
        }
        while ( (uint32_t)q < end );
        out_at = buf_2;
        row_i = row_i2;
        // End of the row: flush whatever literals are left, by the same rule
        // once more.  `LABEL_89` and `LABEL_97` were this written as four
        // entries into two blocks; every one of them ends by loading the
        // image geometry and falling into the row terminator below.
        if ( lit_len && (nib ? lit_len != 1 : lit_len >= 3) )
        {
          *buf_2 = 0;
          buf_2[1] = lit_len << (nib & 31);
          memcpy(buf_2 + 2,&q[-lit_len],lit_len);
          row_i = row_i2;
          out_at = &buf_2[lit_len + 2];
          if ( (lit_len & 1) != 0 )
          {
            buf_2[lit_len + 2] = 0;
            ++out_at;
          }
        }
        else if ( lit_len && nib )
        {
          buf_2[1] = *(q - 1);
          *buf_2 = 2;
          out_at = buf_2 + 2;
        }
        else if ( lit_len )
        {
          if ( lit_len == 2 )
          {
            *buf_2 = 1;
            buf_2[1] = *(q - 2);
            out_at = buf_2 + 2;
          }
          out_at[1] = *(q - 1);
          *out_at = 1;
          out_at += 2;
        }
        rows3 = img->height;
        stride3 = img->stride;
        // The row terminator, and the one place five paths above jump to.
        // It stays a label: they are early exits out of three nested loops,
        // which is the shape a forward jump to a single join is for.
LABEL_72:
        *out_at = 0;
        out_at[1] = 0;
        out_at += 2;
        ++row_i;
        p -= 2 * stride3;
        if ( row_i >= rows3 )
        {
          rows2 = rows3;
          stride = stride3;
          pix = (uint16_t *)img_addr;
          data_len = img->data_size;
          break;
        }
      }
    }
    *out_at = 0;
    out_at[1] = 1;
    out_at += 2;
    coded_bytes = out_at - buf;
    if ( data_len > (uint32_t)(out_at - buf) )
    {
      // The shared tail, copied here: the two assignments it used to reach it
      // through are gone with the `goto`, so it names `out` and `fp`
      // directly.
      bmp->biSizeImage = (uint32_t)coded_bytes;
      n_bytes = (uint32_t)(out_at - out);
      bmp->bfSize = n_bytes;
      if ( fwrite(out, 1u, n_bytes, fp) != bmp->bfSize )
        return 0;
      free(out);
      fclose(fp);
      return 1;
    }
    rle_mode = 0;
  }
  fp2 = fp;
  out2 = out;
  bmp->biCompression = 0;
  if ( rows2 <= 0 )
  {
    coded_bytes = 0;
  }
  else
  {
    pad = ((stride + 3) & 0xFFFFFFFC) - stride;
    stride1 = stride;
    y = 0;
    do
    {
      memcpy(out_at,p,stride1);
      stride1 = img->stride;
      out_at += stride1;
      p -= stride1;
      if ( pad )
      {
        *(uint32_t *)out_at = 0;
        out_at += pad;
      }
      ++y;
    }
    while ( y < img->height );
    out2 = out;
    fp2 = fp;
    coded_bytes = out_at - buf;
  }
  bmp->biSizeImage = (uint32_t)coded_bytes;
  n_bytes = (uint32_t)(out_at - out2);
  bmp->bfSize = n_bytes;
  if ( fwrite(out2, 1u, n_bytes, fp2) != bmp->bfSize )
    return 0;
  free(out2);
  fclose(fp2);
  return 1;
}
uint32_t __init_symbol_list(SymList *list, int32_t unread_this, int32_t n_syms, int32_t dense)
{
  ;
  SymEntry *buf;
  uint32_t result;   // a count, like the header fields it moves
  // Hex-Rays had this twice, once per arm, as `v7 = list->n` and
  // `v10 = list->n` read back immediately after the store below -- MSVC keeping
  // the count in a register.  It is the parameter, unsigned, because every
  // field it feeds is a count and none of them can be negative.
  const uint32_t n = (uint32_t)n_syms;
  list->n = n_syms;
  buf = (SymEntry *)bmf_new(3 * n_syms);
  list->ent = buf;
  if ( dense )
  {
    list->tot = 0;
    list->live = n;
    result = 12 * n;
    list->since_rescale = 12 * n;
    list->rescale_at = 8 * n;
    if ( n )
    {
      result = 0;
      do
      {
        list->ent[result].sym = result;
        list->ent[result].cnt = 1;
        ++result;
      }
      while ( result < list->live );
    }
  }
  else
  {
    list->tot = 2;
    list->rescale_at = 20 * n;
    list->live = 0;
    list->since_rescale = 18 * n;
    return (uint32_t)memset(buf,0,3 * n);
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

// The inverse of `colour_transform`: take one decoded plane and write it back
// into the interleaved image, undoing whatever decorrelation that plane's
// `plane_desc` entry records.  Same three predictors, same weights, same
// rounding -- with the shift added back instead of subtracted, and the
// reference planes read from the image as it is being rebuilt, which is why
// the planes are done in `src_plane` order.

uint8_t * __interleave_plane(uint8_t *img, uint8_t *src, int32_t plane, int8_t unread_flag)
{
  ;
  uint8_t *ref;
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3, x3, left2, x2, mode, to_ref0,
          wgt2, to_ref1, wgt0, stride, dc, n;
  uint32_t base;
  uint8_t *p3, *p2;
  if ( (plane_desc[plane + 1].flags & 8) == 0 )
  {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(&((BmfImage *)img)->pixels[plane],src,n_flat);
    img += plane;
    i = 0;
    ofs = 0;
    do
    {
      img[ofs + 16] = src[i];
      ofs += step;
      ++i;
    }
    while ( i < n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane - plane;
  to_ref1 = plane_desc[2].src_plane - plane;
  mode = plane_desc[plane + 1].predictor;
  dc = plane_desc[plane + 1].b3;
  base = (uint32_t)&((const BmfImage *)img)->pixels[plane];
  wgt1 = plane_desc[plane + 1].w8;
  wgt0 = plane_desc[plane + 1].w4;
  wgt2 = plane_desc[plane + 1].w12;
  // The same disjunction as `colour_transform`'s, and the same reading: the
  // weights select a reference outright, or the predictor does.  Five `goto`s
  // stood here for it.
  by_weights = false;
  if ( mode == 2 && wgt0 + wgt1 == 128 )
  {
    if ( !wgt1 )
    {
      by_weights = true;
    }
    else if ( !wgt0 )
    {
      to_ref0 = plane_desc[2].src_plane - plane;
      by_weights = true;
    }
  }
  if ( !by_weights )
  {
    if ( plane_desc[plane + 1].predictor != 1 )
    {
      if ( mode == 2 )
      {
        left2 = bmf_pixels(img);
        p2 = (uint8_t *)base;
        do
        {
          x2 = dc + (uint8_t)*src++;
          *p2 = (uint8_t)(((wgt0 * (uint8_t)p2[to_ref0]
                               + wgt1 * (uint32_t)(uint8_t)p2[to_ref1] + 40) >> 7)
                             + x2);
          p2 += stride;
          --left2;
        }
        while ( left2 );
      }
      else if ( mode == 3 )
      {
        left3 = bmf_pixels(img);
        p3 = (uint8_t *)base;
        do
        {
          x3 = dc + (uint8_t)*src++;
          *p3 = (uint8_t)(((wgt1 * (uint8_t)*(p3 - 2)
                               + wgt0 * (uint8_t)*(p3 - 3)
                               + wgt2 * (uint32_t)(uint8_t)*(p3 - 1)
                               + 63) >> 7)
                             + x3);
          p3 += stride;
          --left3;
        }
        while ( left3 );
      }
      return img;
    }
  }
  img += plane;
  ref = &img[to_ref0];
  // Twenty-six lines of aliasing test stood here, spelled with five `goto`s
  // into the loop below and a duplicate of it at the bottom -- `undup.py` finds
  // this shape when it is written as an `if`/`else` and cannot when it is
  // written as a jump.  Both destinations were the same six lines.
  i2 = 0;
  ofs2 = 0;
  do
  {
    img[ofs2 + 16] = ref[ofs2 + 16] + dc + src[i2];
    ofs2 += stride;
    ++i2;
  }
  while ( i2 < n );
  return img;
}

// Pull one plane out of the interleaved image and decorrelate it against the
// others -- BMF's colour transform, chosen per plane by the search in
// `choose_plane_coding` and recorded in that plane's `plane_desc` entry.

// The pixels are interleaved, so `plane` is a byte offset within a pixel and
// every loop here steps by `plane_count`.  Flag 8 clear means no transform at
// all, and the body is a plain de-interleave.  Otherwise `predictor` picks one
// of three:

//   1  dst = x - dc - ref0                     subtract one other plane whole
//   2  dst = x - dc - ((w0*ref0 + w1*ref1 + 40) >> 7)     a weighted blend
//   3  dst = x - dc - ((w1*p[-2] + w0*p[-3] + w2*p[-1] + 63) >> 7)

// The weights are sevenths-of-a-bit fixed point and the `+ 40` and `+ 63` are
// the rounding.  Mode 3 reaches backwards inside the *pixel* rather than to a
// named plane, which is why it needs no `to_refN`.

// Mode 2 with the weights summing to 128 and one of them zero is mode 1 in
// disguise -- all the weight on one reference -- and the code says so by
// jumping into mode 1's loop with `to_ref0` pointed at whichever plane won.

// Returns `img` advanced to the plane, which two of the four paths do and two
// do not; no caller uses it.
uint8_t * __colour_transform(uint8_t *img, uint8_t *dst, int32_t plane, int8_t unread_flag)
{
  ;
  uint8_t d;
  uint8_t *ref;   // `uint8_t *` beside the `char` scalars above
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3,
          x3, wsum, w12sum, left2, x2, mode, to_ref0, wgt2, to_ref1, wgt0, dc, stride, n;
  uint32_t blend, src;
  uint8_t *p3, *p2;
  if ( (plane_desc[plane + 1].flags & 8) == 0 )
  {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(dst,&((const BmfImage *)img)->pixels[plane],n_flat);
    img += plane;
    // MSVC's overlap check stood here, and both of its arms were this loop --
    // the same six lines character for character, reached through two `goto`s
    // and two labels.  A test whose arms agree is not a decision, and the two
    // copies exist because the compiler unrolled its own aliasing proof, not
    // because the program had a case to distinguish.  The same shape, with the
    // same reasoning, is at LABEL_4 below.
    i = 0;
    ofs = 0;
    do
    {
      dst[i] = img[ofs + 16];
      ofs += step;
      ++i;
    }
    while ( i < n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane - plane;
  to_ref1 = plane_desc[2].src_plane - plane;
  mode = plane_desc[plane + 1].predictor;
  dc = plane_desc[plane + 1].b3;
  src = (uint32_t)&((const BmfImage *)img)->pixels[plane];
  wgt1 = plane_desc[plane + 1].w8;
  wgt0 = plane_desc[plane + 1].w4;
  wgt2 = plane_desc[plane + 1].w12;
  // Two ways into the same loop: the weights say this plane is a straight
  // copy of one reference -- mode 2 with all 128 on one side, and which side
  // decides which reference -- or the predictor says so outright.  The three
  // `goto`s were that disjunction; the flag is the same thing with a name.
  by_weights = false;
  if ( mode == 2 && wgt0 + wgt1 == 128 )
  {
    if ( !wgt1 )
    {
      by_weights = true;
    }
    else if ( !wgt0 )
    {
      to_ref0 = plane_desc[2].src_plane - plane;
      by_weights = true;
    }
  }
  if ( by_weights || plane_desc[plane + 1].predictor == 1 )
  {
    img += plane;
    ref = &img[to_ref0];
    // The second of the two duplicated bodies -- six lines of aliasing test
    // whose `if` and `else` were the same loop.
    i2 = 0;
    ofs2 = 0;
    do
    {
      d = img[ofs2 + 16] - dc - ref[ofs2 + 16];
      ofs2 += stride;
      dst[i2++] = d;
    }
    while ( i2 < n );
    return img;
  }
  if ( mode == 2 )
  {
    left2 = bmf_pixels(img);
    p2 = (uint8_t *)src;
    do
    {
      x2 = *p2 - dc;
      blend = wgt0 * p2[to_ref0] + wgt1 * p2[to_ref1] + 40;
      p2 += stride;
      *dst++ = (uint8_t)(x2 - (blend >> 7));
      --left2;
    }
    while ( left2 );
  }
  else if ( mode == 3 )
  {
    left3 = bmf_pixels(img);
    p3 = (uint8_t *)src;
    do
    {
      x3 = *p3 - dc;
      wsum = wgt1 * *(p3 - 2) + wgt0 * *(p3 - 3);
      w12sum = wgt2 * *(p3 - 1);
      p3 += stride;
      *dst++ = (uint8_t)(x3 - ((uint32_t)(wsum + w12sum + 63) >> 7));
      --left3;
    }
    while ( left3 );
  }
  return img;
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
int32_t __rc_begin_decode(int8_t unread_flag)
{
  ;
  int32_t bits_left, at4, at5, k;
  uint16_t *tbl0, *row;
  FreqPair *seed;   // the row's tree of counter pairs, seeded (60, 36)
  int32_t at6, at7;
  uint8_t *cursor;
  uint32_t i;   // offsets into model_geometry, not pointers
  cursor = out_cursor;
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
        --cursor;
        bits_left -= 8;
      }
      while ( bits_left >= 0 );
      out_cursor = cursor;
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
    at4 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    memset(&model_geometry[at4], 4, 8);
    level_geom[5].half = 8;
    at5 = at4 + 2 * level_geom[4].half;
    level_geom[5].first = at5;
    level_geom[5].tbl_base = at5 - 5;
    memset(&model_geometry[at5], 0x05, 16);
    level_geom[6].half = 16;
    at6 = at5 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)at6;
    level_geom[6].tbl_base = (uint8_t)at6 - 6;
    memset(&model_geometry[at6], 0x06, 32);
    level_geom[7].half = 32;
    at7 = at6 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)at7;
    level_geom[7].tbl_base = (uint8_t)at7 - 7;
    memset(&model_geometry[at7], 0x07, 64);
    // The same 0x400 x 508-byte table `rc_begin_encode` builds, seeded
    // identically -- see the note there.  254 `uint16_t` is 508 bytes.
    tbl0 = (uint16_t *)bmf_new(0x7F000u);
    if ( tbl0 )
    {
      row = tbl0;
      k = 0;
      do
      {
        row[1] = 24 * alt_freq_limit;
        row[2] = 205;
        row[6] = 48;
        row[3] = 124;
        row[7] = 16;
        row[4] = 147;
        row[5] = 83;
        row[8] = 8;
        row[9] = 4;
        *row = 635;
        // Word 10 is `bit_tree` element 1: the tree is 1-indexed and its
        // element 0 is the last two words of the header seeded above.
        seed = (FreqPair *)&row[10];
        for ( i = 0; i < 0x7A; ++i )
        {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
        row += 254;
        ++k;
      }
      while ( (uint32_t)k < 0x400 );
    }
    // An `else { tbl0 = nullptr; }` stood here, assigning null to what was
    // already null -- the same one `rc_begin_encode` had.
    ::model_tables = tbl0;
    cursor = out_cursor;
  }
  rc.dec_init();
  return (int32_t)(uintptr_t)out_cursor;
}

// The inverse of `predict_med`, and the direction that actually runs: this is
// what `testfiles/med32.bmp` exercises.  It walks *forwards*, the opposite way,
// for the same reason -- a residual can only be added to neighbours that have
// already been reconstructed, and forwards is where those are.

// So the peeled cases are the mirror image.  Pixel 0 is stored raw and is
// already correct, which is why `p` starts one past it; the rest of the first
// row has only a west neighbour; the first column of every later row has only
// a north one; and everything else gets the full MED tree, identical to the
// one in `predict_med` because it must reproduce the same prediction from the
// same three neighbours.

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
  }
  else
  {
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
  }
  rows_left = height - 1;
  if ( height == 1 )
    return p;
  }
  // Every path into here has set `rows_left` and returned already if there is
  // only one row.  `LABEL_29` was that join written as a jump: three
  // predecessors, all of them doing exactly this first.
  {
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
        // The same MED shape as `predict_med`, and the same reading: the
        // `goto`s were the two `pred = north` arms jumping over the gradient
        // case, which an `else if` says without them.
        if ( pred < north )
        {
          if ( northwest < pred )
            LOBYTE(pred) = (uint8_t)north;
          else if ( northwest <= north )
            LOBYTE(pred) = (uint8_t)(north + pred - northwest);
        }
        else
        {
          if ( northwest > pred )
            LOBYTE(pred) = (uint8_t)north;
          else if ( northwest >= north )
            LOBYTE(pred) = (uint8_t)(north + pred - northwest);
        }
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

// Two running pairs rather than one running total, because that is what the
// SSE original had: even bins accumulated in lane 0, odd bins in lane 1, and
// the two added at the end.  Double addition is not associative, so folding
// them into one accumulator would be a different number.
int32_t __estimate_cost(uint8_t *bins, int32_t n)
{
  const int32_t *bin = (const int32_t *)bins;
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  double total, entropy;
  int32_t i;

  for ( i = 0; i + 1 < n; i += 2 )
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
  for ( ; i < n; i++ )
    if ( bin[i] )
    {
      total   += (double)bin[i];
      entropy += (double)bin[i] * log((double)bin[i]);
    }

  if ( total != 0.0 )
    total = total * log(total);
  return (int32_t)((total - entropy) * 1.442695040888963);
}

void ** __alt_model_p1_d8_decode(int8_t unread_flag, uint8_t *out, int32_t i, int32_t height)
{
  P1Ctx *buf1, *b0, *cursor0, *b4, *buf3, *buf2;
  ;
  AltP1Block *blk, *raw;
  int64_t err;
  uint8_t *out_at;
  int32_t y, val, x;
  P1Ctx *cursor2, *cursor4;
  raw = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
  if ( raw )
    blk = (AltP1Block *)(__alt_p1_alloc((AltP1Block *)raw, i, height, 0));
  else
    blk = (AltP1Block *)(nullptr);
  __rc_begin_decode(unread_flag);
  if ( blk->height > 0 )
  {
    y = 0;
    do
    {
      ++y;
      // The row end mirrored into the right margin: records 0..5 take -1..-6.
      // Two bytes each, which is what the twelve even offsets were.
      {
        P1Ctx *const here = (P1Ctx *)blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      b4 = blk->buf[4];
      buf3 = blk->buf[3];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[1] = b0;
      blk->buf[0] = b4;
      b4 += 4;
      blk->cursor[0] = b4;
      b0 += 4;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1 + 4;
      blk->cursor[3] = buf2 + 4;
      blk->cursor[4] = buf3 + 4;
      ((P1Ctx *)b4)[-4] = ((P1Ctx *)b0)[3];
      // And the rest of the new row's left margin, from the row above.
      {
        P1Ctx *const here = (P1Ctx *)blk->cursor[0];
        P1Ctx *const up   = (P1Ctx *)blk->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = blk->cursor[2];
      cursor4 = blk->cursor[4];
      blk->ctx[2] = 0;
      cursor0 = blk->cursor[0];
      // The encoder's block, byte for byte -- and it arrived as bytes, because
      // `cursor2` and `cursor4` were `uint8_t *` over two-byte records.  A
      // record is `sym` then `mag`, so the odd byte offsets are all `mag`:
      // byte -3 is record -2, byte 1 is record 0, byte 11 is record 5.  The
      // stride scan of REFACTORING9.md section 2 cannot see this one, because
      // the gcd of -3, -1, 1, 3, 5, 7, 9 and 11 is 1.
      blk->ctx[3] = cursor2[-2].mag + cursor4[-2].mag
                 + cursor2[0].mag  + cursor4[0].mag
                 + cursor2[2].mag  + cursor4[2].mag
                 + cursor2[4].mag  + cursor4[4].mag
                 + cursor0[-4].mag + cursor0[-2].mag;
      blk->ctx[4] = cursor2[-1].mag + cursor4[-1].mag
                 + cursor2[1].mag  + cursor4[1].mag
                 + cursor2[3].mag  + cursor4[3].mag
                 + cursor2[5].mag  + cursor4[5].mag
                 + cursor0[-3].mag + cursor0[-1].mag;
      if ( !(blk->width <= 0) )
      {
        out_at = out;
        x = 0;
        do
        {
          ++x;
          ((AltP1Block *)blk)->ctx_of((AltP1Block *)nullptr, (AltP1Block *)0);
          val = (uint8_t)((uint8_t)blk->pred
                                + *((uint8_t *)blk + (uint8_t)__alt_p1_decode_symbol((uint16_t *)&((int32_t *)blk)[4 * blk->ctx[0] + 950], 0, blk->ctx[1]) + 1496));
          *out_at = val;
          err = val - blk->pred;
          // The record's two writers, one line apart: the reconstructed
          // sample, then the size of the prediction error.
          blk->cursor[0]->sym = val;
          blk->cursor[0]->mag = (BYTE4(err) ^ err) - BYTE4(err);
          blk->ctx[3 + blk->ctx[2]] = blk->ctx[3 + blk->ctx[2]]
                        + blk->cursor[0]->mag
                        - blk->cursor[0][-4].mag
                        - (blk->cursor[4][-2].mag
                         - blk->cursor[4][6].mag
                         + blk->cursor[2][-2].mag
                         - blk->cursor[2][6].mag);
            blk->ctx[2] = blk->ctx[2] == 0;
          if ( blk->counters[blk->ctx[0]].total < 0x4000u )
            ((AltP1Block *)blk)->update_model();
          ++blk->cursor[0];
          ++out_at;
          ++blk->cursor[1];
          ++blk->cursor[2];
          ++blk->cursor[3];
          ++blk->cursor[4];
        }
        while ( x < blk->width );
        out = out_at;
      }
    }
    while ( y < blk->height );
  }
  __rc_end_decode();
  return __alt_p1_free((void **)blk, 1);
}


int32_t __alt_model_p1_decode(uint16_t *hdr, uint8_t *out) {   P1Ctx *b4,
        *buf3, *buf2, *buf1, *b0, *cursor0;
  // Phase 2 split this frame into plain locals, which is what the other eight
  // frames took -- and it is wrong here.  The frame was 116 bytes and its
  // aliases only reach offset 84; the code writes into the 32 bytes of slack
  // past the end, and once each local has its own storage those writes land on
  // whatever the compiler put next.  At -O2 that is a null pointer handed to
  // ctx_of; at -O0 it is a plane that decodes to the wrong pixels.
  
  // Nothing caught it because nothing reached it: this is the body
  // REFACTORING.md section 2.3 lists as unexercised.  testfiles/altp1.bmp
  // reaches it now.
  void *p0v;
  int32_t val3x;
  void * plane[4];
  AltP1Block * &plane1 = (AltP1Block * &)plane[1];
  AltP1Block * &plane2 = (AltP1Block * &)plane[2];
  AltP1Block * &plane3 = (AltP1Block * &)plane[3];
  int32_t dc1;
  uint32_t x;
  int32_t val1x;
  AltP1Block *blk1;
  uintptr_t code1;
  AltP1Block *blk_k, *raw;
  uint32_t at0;
  int32_t code0;
  uint32_t pred0, pred1;
  AltP1Block *blk2, *blk3;
  P1Ctx *cursor2, *cursor4;
  void **q;
  ;
  int32_t width, height, k, *made, src1, src2, src3, dc2, dc3, xf1, xf2, xf3, w, n_planes,
          p, val0, val1, code2, val2, at2,
          code3, val3, at3, np, f;
  uint32_t y, *p0;
  uint8_t *pred2, *pred3;
  width = *hdr;
  height = hdr[1];
  if ( plane_count > 0 )
  {
    k = 0;
    do
    {
      raw = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( raw )
        made = __alt_p1_alloc((AltP1Block *)raw, width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    }
    while ( k < plane_count );
  }
  // Records 2, 3 and 4 name a source plane each; each of the three is then
  // read for its flags and its b3.  Record indices now -- they were 16 times
  // these, the byte offsets into the table.
  src1 = plane_desc[2].src_plane;
  src2 = plane_desc[3].src_plane;
  src3 = plane_desc[4].src_plane;
  dc2 = plane_desc[src2 + 1].b3;
  dc3 = plane_desc[src3 + 1].b3;
  dc1 = plane_desc[src1 + 1].b3;
  xf1 = plane_desc[src1 + 1].flags & 8;
  xf2 = plane_desc[src2 + 1].flags & 8;
  xf3 = plane_desc[src3 + 1].flags & 8;
  __rc_begin_decode(xf2);
  if ( height > 0 )
  {
    y = 0;
    w = width;
    n_planes = plane_count;
    do
    {
      if ( n_planes > 0 )
      {
        p = 0;
        do
        {
          ++p;
          // `&val3x + n` is `plane[n - 1]`: val3x is the member
          // before the array, and this loop pre-increments from 0.
          blk_k = (AltP1Block *)plane[p - 1];
          // The row end mirrored into the right margin: records 0..5 take -1..-6.
          // Two bytes each, which is what the twelve even offsets were.
          {
            P1Ctx *const here = (P1Ctx *)blk_k->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          b4 = blk_k->buf[4];
          buf3 = blk_k->buf[3];
          buf2 = blk_k->buf[2];
          buf1 = blk_k->buf[1];
          b0 = blk_k->buf[0];
          blk_k->buf[4] = buf3;
          blk_k->buf[3] = buf2;
          blk_k->buf[2] = buf1;
          blk_k->buf[1] = b0;
          blk_k->buf[0] = b4;
          b4 += 4;
          blk_k->cursor[0] = b4;
          b0 += 4;
          blk_k->cursor[1] = b0;
          blk_k->cursor[2] = buf1 + 4;
          blk_k->cursor[3] = buf2 + 4;
          blk_k->cursor[4] = buf3 + 4;
          ((P1Ctx *)b4)[-4] = ((P1Ctx *)b0)[3];
          // And the rest of the new row's left margin, from the row above.
          {
            P1Ctx *const here = (P1Ctx *)blk_k->cursor[0];
            P1Ctx *const up   = (P1Ctx *)blk_k->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          cursor2 = blk_k->cursor[2];
          cursor4 = blk_k->cursor[4];
          cursor0 = blk_k->cursor[0];
          blk_k->ctx[2] = 0;
          // The same twenty terms `d8_encode_body` sums, arriving here
          // as byte offsets on a `uint8_t *`: a record is `sym` then `mag`, so
          // every odd offset is a `mag` and byte 2k+1 is record k.
          
          // The four `(int8_t)` casts are load-bearing, and this is measured
          // rather than argued.  MSVC emitted `movsx` for the four left-margin
          // loads and `movzx` for the other sixteen; a magnitude is
          // `abs(sample - prediction)` on eight-bit samples, so it can exceed
          // 127, and a probe counting how often it does says **21 of 1536** on
          // `testfiles/altp1.bmp`.  The sign shows.  (That is also the only
          // stream in the corpus that reaches this body at all, which is what
          // `tools/mkaltp1.py` exists for.)
          
          // Contrast REFACTORING9.md section 3, where `PixRec`'s three `movsx`
          // loads keep their casts on the opposite finding: there the writers
          // are comparisons and the seed is 1, so the sign *cannot* show.  Two
          // casts kept for two different reasons, and only one of them was a
          // transcription.
          
          // The `_d8` pair reads all twenty unsigned, which is the same source
          // compiled differently.
          blk_k->ctx[3] = (int8_t)cursor2[-2].mag + (int8_t)cursor4[-2].mag
                      + cursor2[0].mag  + cursor4[0].mag
                      + cursor2[2].mag  + cursor4[2].mag
                      + cursor2[4].mag  + cursor4[4].mag
                      + cursor0[-4].mag + cursor0[-2].mag;
          blk_k->ctx[4] = (int8_t)cursor2[-1].mag + (int8_t)cursor4[-1].mag
                      + cursor2[1].mag  + cursor4[1].mag
                      + cursor2[3].mag  + cursor4[3].mag
                      + cursor2[5].mag  + cursor4[5].mag
                      + cursor0[-3].mag + cursor0[-1].mag;
          n_planes = plane_count;
        }
        while ( p < plane_count );
        w = width;
      }
      if ( w > 0 )
      {
        x = 0;
        do
        {
          p0 = (uint32_t *)plane[0];
          ((AltP1Block *)plane[0])->ctx_of((AltP1Block *)nullptr, (AltP1Block *)0);
          code0 = __alt_p1_decode_symbol((uint16_t *)&p0[4 * p0[3] + 950], 0, p0[4]);
          pred0 = p0[2];
          AltP1Block *const blk = (AltP1Block *)p0;
          val0 = (uint8_t)(pred0 + blk->unfold[code0]);
          // The row record is two bytes: the reconstructed sample at +0 and the
          // size of the prediction error at +1.  `[1]` is this pixel's error,
          // `[-7]` is the one four pixels back, and the four terms off
          // `cursor[2]` and `cursor[4]` are two records back and six forward on
          // the other two planes.
          blk->cursor[0]->sym = val0;
          blk->cursor[0]->mag = abs32(val0 - pred0);
          p0[p0[5] + 6] = p0[p0[5] + 6]
                          + blk->cursor[0]->mag
                          - blk->cursor[0][-4].mag
                          - (blk->cursor[4][-2].mag
                           - blk->cursor[4][6].mag
                           + blk->cursor[2][-2].mag
                           - blk->cursor[2][6].mag);
          at0 = 4 * p0[3];
          p0[5] = p0[5] == 0;
          if ( LOWORD(p0[at0 + 950]) < 0x4000u )
            ((AltP1Block *)p0)->update_model();
          p0[49] += 2;
          p0[50] += 2;
          p0[51] += 2;
          p0[52] += 2;
          p0[53] += 2;
          blk1 = (AltP1Block *)plane1;
          *(plane_desc[1].src_plane + out) = val0;
          ((AltP1Block *)blk1)->ctx_of((AltP1Block *)plane[0], (AltP1Block *)0);
          code1 = __alt_p1_decode_symbol(&blk1->counters[blk1->ctx[0]].total, 0, blk1->ctx[1]);
          pred1 = *(uint32_t *)&blk1->pred;
          val1 = (uint8_t)(pred1 + blk1->unfold[code1]);
          val1x = val1;
          blk1->cursor[0]->sym = val1;
          blk1->cursor[0]->mag = abs32(val1 - pred1);
          blk1->ctx[3 + blk1->ctx[2]] = blk1->ctx[3 + blk1->ctx[2]]
                                                            + blk1->cursor[0]->mag
                                                            - blk1->cursor[0][-4].mag
                                                            - (blk1->cursor[4][-2].mag
                                                             - blk1->cursor[4][6].mag
                                                             + blk1->cursor[2][-2].mag
                                                             - blk1->cursor[2][6].mag);
          blk1->ctx[2] = blk1->ctx[2] == 0;
          if ( blk1->counters[blk1->ctx[0]].total < 0x4000u )
            blk1->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          if ( xf1 )
            val1x += dc1 + *(plane_desc[1].src_plane + out);
          blk2 = (AltP1Block *)(plane2);
          p0v = plane[0];
          *(out + plane_desc[2].src_plane) = val1x;
          ((AltP1Block *)blk2)->ctx_of((AltP1Block *)plane1, (AltP1Block *)(int32_t)p0v);
          code2 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)blk2)[4 * blk2->ctx[0] + 950], 0, (int32_t)blk2->ctx[1]);
          pred2 = (uint8_t *)(blk2->pred);
          val2 = (uint8_t)((uint8_t)(uintptr_t)pred2 + blk2->unfold[code2]);
          blk2->cursor[0]->sym = val2;
          blk2->cursor[0]->mag = abs32(val2 - (uint32_t)pred2);
          ((uint8_t**)blk2)[blk2->ctx[2] + 6] = &((uint8_t**)blk2)[blk2->ctx[2] + 6][blk2->cursor[0]->mag
                                                           - blk2->cursor[0][-4].mag
                                                           - (blk2->cursor[4][-2].mag
                                                            - blk2->cursor[4][6].mag)
                                                           - (blk2->cursor[2][-2].mag
                                                            - blk2->cursor[2][6].mag)];
          at2 = 4 * blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2] == 0;
          if ( LOWORD(((uint8_t**)blk2)[at2 + 950]) < 0x4000u )
            ((AltP1Block *)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          if ( xf2 )
            *(plane_desc[3].src_plane + out) = ((plane_desc[plane_desc[3].src_plane + 1].w4
                                                                * *(plane_desc[1].src_plane
                                                                                     + out)
                                                                + plane_desc[plane_desc[3].src_plane + 1].w8
                                                                * (uint32_t)*(plane_desc[2].src_plane + out)
                                                                + 40) >> 7)
                                                              + dc2
                                                              + val2;
          else
            *(plane_desc[3].src_plane + out) = val2;
          n_planes = plane_count;
          if ( plane_count >= 4 )
          {
            blk3 = (AltP1Block *)(plane3);
            ((AltP1Block *)plane3)->ctx_of((AltP1Block *)plane2, (AltP1Block *)(int32_t)plane1);
            code3 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)blk3)[4 * blk3->ctx[0] + 950], 0, (int32_t)blk3->ctx[1]);
            pred3 = (uint8_t *)(blk3->pred);
            val3 = (uint8_t)((uint8_t)(uintptr_t)pred3 + blk3->unfold[code3]);
            val3x = val3;
            blk3->cursor[0]->sym = val3;
            blk3->cursor[0]->mag = abs32(val3 - (uint32_t)pred3);
            ((uint8_t**)blk3)[blk3->ctx[2] + 6] = &((uint8_t**)blk3)[blk3->ctx[2] + 6][blk3->cursor[0]->mag
                                                             - blk3->cursor[0][-4].mag
                                                             - (blk3->cursor[4][-2].mag
                                                              - blk3->cursor[4][6].mag)
                                                             - (blk3->cursor[2][-2].mag
                                                              - blk3->cursor[2][6].mag)];
            at3 = 4 * blk3->ctx[0];
            blk3->ctx[2] = blk3->ctx[2] == 0;
            if ( LOWORD(((uint8_t**)blk3)[at3 + 950]) < 0x4000u )
              ((AltP1Block *)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            if ( xf3 )
              val3x += ((plane_desc[plane_desc[4].src_plane + 1].w8 * *(plane_desc[4].src_plane + out - 2)
                     + plane_desc[plane_desc[4].src_plane + 1].w4 * *(plane_desc[4].src_plane + out - 3)
                     + plane_desc[plane_desc[4].src_plane + 1].w12 * *(plane_desc[4].src_plane + out - 1)
                     + 64) >> 7)
                   + dc3;
            *(plane_desc[4].src_plane + out) = val3x;
            n_planes = plane_count;
          }
          out += n_planes;
          ++x;
        }
        while ( x < (uint32_t)width );
        w = width;
      }
      ++y;
    }
    while ( y < (uint32_t)height );
  }
  __rc_end_decode();
  np = plane_count;
  if ( plane_count > 0 )
  {
    f = 0;
    do
    {
      q = (void **)plane[f];
      if ( q )
      {
        __alt_p1_free((void **)q, 1);
        np = plane_count;
      }
      ++f;
    }
    while ( f < np );
  }
  return np;
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

int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], CtxWeights *w, int32_t mode)
{
  const float *mix = bmf_p2_mix[mode];
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
      mixed[j][k] = mix[0] * w->f0[0][j][k];
      for ( i = 1; i < 6; i++ )
        mixed[j][k] += mix[i] * w->f0[i][j][k];
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
      _this[j + 7][k] = w->f0[0][j + 7][k] * bmf_p2_seed;
    }
  _this[14][0] = 47.0f;
  _this[14][1] = 169.2f;
  _this[14][2] = 1.0f;
  a2[7][1] = prediction;
  return (int32_t)prediction;
}

// `ctx_of`'s opposite number for p2, and the largest body in the file
// at a thousand lines.  Same evidence for the name: no `rc.` call in it, and
// all four p2 bodies call it from both sides.  It ends by folding the
// neighbourhood sums into three context words and returning the 0..255 index
// they are read with.  What the individual terms *mean* is ALGORITHM.md §9's
// question, and it is still open -- this names the role, not the algorithm.
int32_t __alt_p2_context(AltP2Block *blk, AltP2Block *refa, AltP2Block *refb) {   P2Ctx *unused_p,
        *row0;
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "altp1 segfaults while compressing"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
  struct alignas(16) AltP2ContextFrame {   // 208 bytes, one stack frame
      // Three slots MSVC used for `cursor[0]`, one digit of the
      // neighbourhood index, and `(int16_t *)cursor[1]`.  All three had one
      // other name that says what they are, and the layout is pinned, so
      // they stay as the size they were.
      uint8_t   _gapA[4];   // was P2Ctx * v246, which is `cursor[0]`
      uint8_t   _gapB[4];   // was int32_t v256, which is `64 * gA`
      uint8_t   _gapC[4];   // was int16_t * v268, which is `cursor[1]`
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
      uint8_t   _gap0[4];   // was int16_t * ra1
      uint8_t   _gap1[4];   // was int16_t * ra0
      uint8_t   _gap2[4];   // was int16_t * rb1
      uint8_t   _gap3[4];   // was int16_t * rb0
      uint8_t   _gap4[4];   // was int16_t * rb2
      uint8_t   _gap5[4];   // was int16_t * ra2
      uint8_t   _gap6[4];   // was int32_t n1840_2
      uint8_t   _gap7[4];   // was int32_t n1840_1
      uint8_t   _gap8[4];   // was AltP2Block * blk
      uint8_t   _gap9[4];   // was int32_t sum_all
      uint8_t   _gap10[4];   // was int32_t n3536
      uint8_t   _gap11[4];   // was int32_t v292
      uint8_t   _gap12[4];   // was P2Ctx *cx1, now a `P2Ctx *`
      uint8_t   _gap13[4];   // was int16_t * cx3
      uint8_t   _gap14[4];   // was int16_t * cx2
      uint8_t   _gap15[4];   // was int16_t * v296
      uint8_t   _gap16[4];   // was uint32_t q39
      uint8_t   _gap17[4];   // was uint32_t q10
      uint8_t   _gap18[4];   // was int32_t n960_1
      uint8_t   _gap19[4];   // was int32_t n1840
      uint8_t   _gap20[4];   // was int32_t v301
      uint8_t   _gap21[4];   // was int32_t v302
      uint8_t   _gap22[4];   // was int32_t v303
      uint8_t   _gap23[4];   // was int32_t v304
      uint8_t   _gap24[4];   // was int32_t v305
      uint8_t   _gap25[4];   // was int32_t run_dv3
      uint8_t   _gap26[4];   // was int32_t dv_now4
      uint8_t   _gap27[4];   // was int32_t v308
      uint8_t   _gap28[4];   // was int32_t v309
      uint8_t   _gap29[4];   // was int32_t v310
      uint8_t   _gap30[4];   // was int32_t v311
      uint8_t   _gap31[4];   // was int32_t run_dv4
      uint8_t   _gap32[4];   // was int32_t run_up4
      uint8_t   _gap33[4];   // was int32_t v314
      uint8_t   _gap34[4];   // was int32_t v315
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 208, "frame layout moved");
  // Sixteen locals shared three stack slots with the names that used to bind
  // them: MSVC gave one slot to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is the
  // gate's answer -- nothing writes one of them and reads another.  The three
  // slots are `_gapA`..`_gapC` above now; nothing is left to share.
  int32_t dv_now, mag_ref1, ctx15, row0_i, mag_ref0, d_run0b, g3pair;
  P2Ctx *ra1, *ra0, *rb1, *rb0, *rb2, *ra2, *sub0_row, *row2c;
  int32_t sum_all;
  P2Ctx *cx1, *cx3;
  uint32_t q39, q10;
  int32_t dtop2, run_dv3, dv_now4, run_dv4, run_up4, *up_row;
  int8_t rate1;
  uint8_t *cx2p;
  P2Ctx *nb4;   // row cursors into the neighbourhood table
  // The six spill slots before `alt_p2_filter`'s six weight pointers are
  // loaded into them: a row cursor, a neighbour, a threshold-row index and
  // three counts.  Both lifetimes were `__frame.sub[0..5]`.
  int32_t  num_b, band, num_d;
  // The five digits of the neighbourhood index and the index itself.
  int32_t  gA, gB, gC, gD, den_d, nb_slot;
  P2Ctx *cx0;   // the p2 row cursor, `cursor[0]`
  // The p2 row cursor and its copies.  Every dereference is a lane of the
  // 18-byte record or the high byte of one, so the byte offsets divide into
  // a record and a lane -- `-37` is record -3, lane 8, high byte.
  // A `P2Ctx *`: Hex-Rays typed it `int16_t (*)[8]`, a sixteen-byte
  // stride over eighteen-byte records, so its subscripts were out of
  // phase with the grid.  All 48 reaches through it land on a field
  // boundary once the byte offsets are decoded against the real size,
  // which is what says the stride was the artefact and not the table.
  P2Ctx *cx2;   // row cursors into the neighbourhood table
  // Three slots MSVC reused between the bank stages: each held one term
  // of the stage-2 context word and then a magnitude sum at the end.
  int32_t up5, lap, magl;
  int32_t dvsum2, magu;
  ;
  // `cursor[0]` again: 45 reaches, all of them records -6 .. 0 at
  // lanes 0 and 1, which is the row this pass is writing.
  P2Ctx *nb0, *nb3, *nb2, *cx4, *nb3x;
  float (*wrow)[4];
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
  bool in_band, no_ref;
  int16_t *nb2w, *nb2w2;
  P2Ctx *cursor1, *nb1, *nb2x;
  int32_t sum4, lane5, nb_id, next_id, plane, in63, *cur, c_lo, mode, c_mid,
          filt, lane3, g3sum, run_s, lane2, ctx0, bank0, pred0, sum_c, run0,
          ctx1, bank1, w1c, one1, pred1, run1, cx2_val0, ctx2, bank2, pred2,
          run2, bank3, w3c, pred3, run3, bank4, pred4, band_lo, band_hi,
          d_run4, magsum, run_s2, mag, flat_a, flat_b, d_run4b, d_rb_rb1,
          d_rb_left, ra0_val, d_ra_ra1b, d_ra_leftb, ctx_idx, d_ra_ra1,
          d_ra_left;
  uint32_t ctx0_lo, q24, q10a, q10b, q9;
  // One name per lifetime.  MSVC gave each of these slots two, three or
  // four unrelated jobs -- a context sum, then a difference term, then a
  // band bound -- and Hex-Rays named the slot after the first constant it
  // saw in it, so all four jobs answered to `n1840_1`.
  int32_t sum_u, run, nb4_4, run4, sum_ul, d_up, d_ra;
  int32_t sum_ur, d_up5, dv1, d_run0, cx1_val, magsum_s;
  ;
  row0 = blk->cursor[0];
  cursor1 = blk->cursor[1];
  sub0_row = (P2Ctx *)blk->cursor[2];
  sum_ul = 21 * sub0_row[-1].dupleft
          + 12 * cursor1[3].dupleft
          + 16 * cursor1[2].dupleft
          + 22 * cursor1[1].dupleft
          + (23 * row0[-1].dupleft)
          + 20 * cursor1->dupleft
          + ctx_bias[0]
          + 14 * row0[-2].dupleft;
  sum_ur = 17 * sub0_row[-2].dupright
          + 21 * cursor1[2].dupright
          + 15 * cursor1[1].dupright
          + 25 * cursor1->dupright
          + 9 * cursor1[-1].dupright
          + 22 * row0[-1].dupright
          + ctx_bias[1]
          + 19 * row0[-2].dupright;
  sum4 = 17 * cursor1[3].dleft
      + 15 * cursor1[2].dleft
      + 21 * cursor1[1].dleft
      + 18 * cursor1->dleft
      + 16 * cursor1[-1].dleft
      + 22 * row0[-1].dleft
      + ctx_bias[2]
      + 19 * row0[-2].dleft;
  lane5 = sub0_row->dup;
  // The same seven taps as the three above, on lane 5.  Hex-Rays reached
  // three of them through `(int16_t *)cursor1`, whose subscripts are byte
  // offsets 64, 28 and 10 -- records 3, 1 and 0 at lane 5 of an 18-byte
  // record, which is the same neighbourhood the other three sums walk.
  sum_u = 14 * cursor1[3].dup
        + 23 * cursor1[1].dup
        + 19 * cursor1->dup
        + 25 * row0[-1].dup
        + ctx_bias[3]
        + 17 * row0[-2].dup
        + 15 * (blk->cursor[3]->dup + lane5);
  sum_all = sum4 + sum_u + sum_ul + sum_ur;
  // Which row of the threshold table: how many of five ratios the coded
  // length has passed.  This was `13 * <the same sum>` used as a flat
  // subscript, with the sum itself recomputed two statements later.
  band = (8 * sum4 > 43 * sum_u)
       + (8 * sum4 > 17 * sum_u)
       + (8 * sum4 > 9 * sum_u)
       + (8 * sum4 > 5 * sum_u)
       + (8 * sum4 > 2 * sum_u);
  // Which neighbourhood the plane is in, as a mixed-radix index: the band
  // above, then four more ratios each counting how many of their own
  // thresholds this plane has passed.
  
  //     nb_slot = 320 * band + 64 * gA + 16 * gB + 4 * gC + gD
  
  // `gA` runs [0,5) and the other three [0,4), so the digits pack exactly and
  // reach 5 * 5 * 4 * 4 * 4 = 1600 slots.  `nb_id` spans 1916 entries -- that
  // is the distance to the next field, not a bound this index knows.  All five
  // arrived in different disguises -- `gB` as a `P2Ctx *` holding `16 * gB`
  // and nothing else, the band and `gC` as an `int16_t` subscript on it,
  // `gA` through a stack slot, and the whole index as a `uint8_t *`.
  den_d = 2 * row0[-2].val + 2 * row0[-1].val;
  num_b = cursor1[1].val + (cursor1->val + 2 * row0[-1].val);
  num_d = 16 * (sub0_row[2].val + sub0_row->val
              + cursor1[1].val + sub0_row[-1].val);
  gA = (sum_all > bmf_p2_thresholds[band][12])
     + (sum_all > bmf_p2_thresholds[band][11])
     + (sum_all > bmf_p2_thresholds[band][10])
     + (sum_all > bmf_p2_thresholds[band][9]);
  gB = (num_b > bmf_p2_thresholds[band][8])
     + (num_b > bmf_p2_thresholds[band][7])
     + (num_b > bmf_p2_thresholds[band][6]);
  gC = (16 * sum_ur > sum_ul * bmf_p2_thresholds[band][5])
     + (16 * sum_ur > sum_ul * bmf_p2_thresholds[band][4])
     + (16 * sum_ur > sum_ul * bmf_p2_thresholds[band][3]);
  gD = (num_d > bmf_p2_thresholds[band][2] * den_d)
     + (num_d > den_d * bmf_p2_thresholds[band][1])
     + (num_d > den_d * bmf_p2_thresholds[band][0]);
  nb_slot = 320 * band + 64 * gA + 16 * gB + 4 * gC + gD;
  nb_id = blk->nb_id[(uint32_t)nb_slot];
  if ( blk->nb_id[(uint32_t)nb_slot] )
  {
    wrow = &((float (*)[4])blk)[16 * nb_id];
    *(int32_t *)&blk->f278656 = (int32_t)wrow;
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
        acc[k] = wrow[0][k] * blk->p2_row[0][k];
        for ( j = 1; j < 7; ++j )
          acc[k] += wrow[j][k] * blk->p2_row[j][k];
      }
      err = ((float)row0[-1].val
             - (bmf_hsum4(acc) + blk->bias[0])) * 2.0999999f;
      floor_ = 7744.0f * wrow[14][2];

      for ( j = 0; j < 7; ++j )
        for ( k = 0; k < 4; ++k )
        {
          float x  = blk->p2_row[j][k];
          float ms = wrow[7 + j][k]
                   + (x * x - wrow[7 + j][k]) * bmf_p2_ms_rate;
          wrow[7 + j][k] = ms;
          wrow[j][k] += bmf_p2_rate[j][k] * err * x / (ms + floor_);
        }
    }
  }
  else
  {
    next_id = blk->nb_id_used;
    blk->nb_id_used = ++next_id;
    blk->nb_id[(uint32_t)nb_slot] = next_id;
    *(int32_t *)&blk->f278656 = (int32_t)&((float (*)[4])blk)[16 * (int16_t)next_id];
  }
  nb1 = (P2Ctx *)blk->cursor[1];
  blk->p2_row[0][0] = (float)nb1->dval;
  nb0 = (P2Ctx *)blk->cursor[0];
  blk->p2_row[0][1] = (float)nb1[1].dval;
  blk->p2_row[0][2] = (float)(nb0[-1].dval + nb1->dval - nb1[-1].dval);
  row2c = (P2Ctx *)blk->cursor[2];
  blk->p2_row[0][3] = (float)(nb0[-2].dval + (nb1->dval - nb1[-2].dval));
  nb3 = (P2Ctx *)blk->cursor[3];
  blk->p2_row[1][0] = (float)(nb1[-1].dval + nb1->dval - row2c[-1].dval);
  blk->p2_row[1][1] = (float)(-3 * (row2c->dval - nb1->dval) + nb3->dval);
  blk->p2_row[1][2] = (float)(nb0[-1].dval + nb1[2].dval - nb1[1].dval);
  blk->p2_row[1][3] = (float)(nb0[-2].dval + nb1[1].dval - nb1[-1].dval);
  blk->p2_row[2][0] = (float)(2 * nb0[-1].dval - nb0[-2].dval);
  nb2 = (P2Ctx *)(row2c);
  blk->p2_row[2][1] = (float)(nb0[-3].dval + nb1->dval - nb1[-3].dval);
  blk->p2_row[2][2] = (float)(nb2->dval + nb1[1].dval - nb3[1].dval);
  blk->p2_row[2][3] = (float)nb0[-3].dval;
  blk->p2_row[3][0] = (float)(nb1[-2].dval + nb1->dval - nb2[-2].dval);
  blk->p2_row[3][1] = (float)(nb0[-2].dval + nb1[-1].dval - nb1[-3].dval);
  blk->p2_row[3][2] = (float)(nb1[1].dval + ((nb1[2].dval + nb1->dval) >> 1) - nb2[2].dval);
  blk->p2_row[3][3] = (float)nb3->dval;
  if ( refa )
  {
    ra0 = (P2Ctx *)refa->cursor[0] - 1;
    ra1 = (P2Ctx *)((int16_t *)(refa->cursor[1] - 1));
    ra2 = (P2Ctx *)((int16_t *)(refa->cursor[2] - 1));
    rb0 = (P2Ctx *)refb->cursor[0] - 1;
    no_ref = blk->has_ref == 0;
    rb1 = refb->cursor[1] - 1;
    rb2 = refb->cursor[2] - 1;
    if ( !no_ref )
    {
      // One number added to all sixteen floats of the first four rows.
      {
        float bias = (float)nb0->dval;
        int32_t j, k;
        for ( j = 0; j < 4; ++j )
          for ( k = 0; k < 4; ++k )
            blk->p2_row[j][k] += bias;
      }
      nb0 = (P2Ctx *)blk->cursor[0];
      nb1 = (P2Ctx *)blk->cursor[1];
    }
    plane = *(int32_t *)&blk->plane_idx;
    if ( plane )
    {
      if ( plane == 1 )
      {
        blk->p2_row[4][0] = (float)(nb0->dval + nb1[3].dval);
        blk->p2_row[4][1] = (float)(nb0[-2].val + ra1[2].val - ra1->val);
        blk->p2_row[4][2] = (float)(nb1[1].val + ra0[-1].val - ra1->val);
        blk->p2_row[4][3] = (float)(nb0[-1].val + ra1->val - ra1[-1].val);
        blk->p2_row[5][0] = (float)(nb1[1].val + (rb1[2].val - ((P2Ctx *)(rb2))[3].val));
        blk->p2_row[5][1] = (float)(nb0[-2].val + rb0->val - rb0[-2].val);
        blk->p2_row[5][2] = (float)(nb1->val + rb0->val - rb1->val);
        no_ref = blk->has_ref == 0;
        blk->p2_row[5][3] = (float)(nb0[-2].val + ra0->err);
        if ( no_ref )
        {
          row0_i = (int32_t)(uintptr_t)nb0;
          nb2w = (int16_t *)blk->cursor[2];
          blk->p2_row[6][0] = ((float)(nb0[-2].val + ra0->val - ra0[-2].val));
          blk->p2_row[6][1] = (float)(*nb2w + ra0->val - ((P2Ctx *)(ra2))->val);
          blk->p2_row[6][2] = (float)(ra2->val + ra0->val - *nb2w + 2 * (nb1->val - ra1->val));
          in63 = *(int16_t *)((uint8_t *)row0_i - 18) + nb1[-1].val + ra1[-2].val + ra0->val - ra0[-1].val - ra1[-1].val - nb1[-2].val;
        }
        else
        {
          blk->p2_row[6][0] = (float)nb1->val;
          blk->p2_row[6][1] = (float)nb0[-3].val;
          blk->p2_row[6][2] = (float)(nb0[-1].val + ra0[-1].val - ra0[-2].val);
          in63 = nb0[-3].val + nb0[-1].val - nb0[-4].val;
        }
        blk->p2_row[6][3] = (float)in63;
      }
      else
      {
        blk->p2_row[4][0] = (float)(nb0[-3].val + nb0[-1].val - nb0[-4].val);
        blk->p2_row[4][1] = ((float)(nb0[-1].val + ra0[-1].val - ra0[-2].val));
        blk->p2_row[4][2] = (float)(nb1[1].val + ra1->val - ra2[1].val);
        blk->p2_row[4][3] = (float)(nb0[-2].val + ra1[2].val - ra1->val);
        blk->p2_row[5][0] = (float)(nb1->val + ra0[-2].val - ra1[-2].val);
        blk->p2_row[5][1] = (float)(nb1->val + ra0->val - ra1->val);
        blk->p2_row[5][2] = (float)(nb0[-2].val + ra0->val - ra0[-2].val);
        nb2w2 = (int16_t *)blk->cursor[2];
        blk->p2_row[5][3] = (float)(nb0[-2].val + rb0->val - rb0[-2].val);
        blk->p2_row[6][0] = (float)(*nb2w2 + rb0->val - ((P2Ctx *)(rb2))->val);
        blk->p2_row[6][1] = (float)(rb0[-2].val
                                       + rb0->val
                                       - nb0[-2].val
                                       + 2 * (nb0[-1].val - rb0[-1].val));
        blk->p2_row[6][2] = ((float)(nb0[-2].val + rb0->err));
        blk->p2_row[6][3] = (float)(*nb2w2 + ra0->err);
      }
    }
    else
    {
      blk->p2_row[4][0] = (float)(nb0[-3].val + nb0[-1].val - nb0[-4].val);
      nb3x = blk->cursor[3];
      blk->p2_row[4][1] = (float)(nb0[-5].val + nb1->val - nb1[-5].val);
      nb2x = (P2Ctx *)blk->cursor[2];
      blk->p2_row[4][2] = (float)(nb0[-4].val + nb1->val - nb1[-4].val);
      blk->p2_row[4][3] = ((float)((nb2x->val + 3 * nb1[1].val - 4 * nb2x[1].val) - (((nb1[2].val - nb1->val - (nb3x[2].val - nb3x->val)) >> 1) - nb3x[1].val)));
      blk->p2_row[5][0] = (float)(nb0[-2].val + ra0->val - ra0[-2].val);
      blk->p2_row[5][1] = (float)(nb1[1].val + ra1[1].val - ra2[2].val);
      blk->p2_row[5][2] = (float)(nb1[-2].val + ra1[2].val - ra2->val);
      blk->p2_row[5][3] = ((float)((ra0[-2].val + ra0->val - nb0[-2].val) + 2 * (nb0[-1].val - ra0[-1].val)));
      blk->p2_row[6][0] = (float)(nb2x[1].val + rb0->val - rb2[1].val);
      blk->p2_row[6][1] = ((float)(nb0[-2].val + rb0->val - rb0[-2].val));
      blk->p2_row[6][2] = (float)(nb0[-1].val + rb1[1].val - rb1->val);
      blk->p2_row[6][3] = (float)(nb1[1].val + rb1[2].val - rb2[3].val);
    }
  }
  else
  {
    blk->p2_row[4][0] = (float)(nb1[3].val + nb1->val - nb2[3].val);
    row2c = nb2;
    blk->p2_row[4][1] = (float)(nb0[-4].val + nb1->val - nb1[-4].val);
    blk->p2_row[4][2] = ((float)((nb2->val + 3 * nb1[1].val - 4 * nb2[1].val) - ((((nb1[2].val - nb1->val) - (nb3[2].val - nb3->val)) >> 1) - nb3[1].val)));
    nb4 = (P2Ctx *)blk->cursor[4];
    blk->p2_row[4][3] = (float)(row2c[-1].val + nb1[1].val - nb3->val);
    blk->p2_row[5][0] = (float)(nb3[1].val + nb1->val - nb4[1].val);
    blk->p2_row[5][1] = (float)nb1[3].val;
    blk->p2_row[5][2] = (float)(nb0[-3].val + nb0[-1].val - nb0[-4].val);
    blk->p2_row[5][3] = (float)(nb0[-1].val + nb3->val - nb3[-1].val);
    blk->p2_row[6][0] = (float)(nb0[-5].val + nb1->val - nb1[-5].val);
    blk->p2_row[6][1] = (float)nb4->val;
    blk->p2_row[6][2] = (float)(nb0[-5].val + nb0[-1].val - nb0[-6].val);
    blk->p2_row[6][3] = (float)nb1[-6].val;
    rb2 = (P2Ctx *)(nullptr);
    rb1 = (P2Ctx *)(nullptr);
    rb0 = nullptr;
    ra2 = (P2Ctx *)(nullptr);
    ra1 = (P2Ctx *)(nullptr);
    ra0 = nullptr;
  }
  cur = blk->cur;
  __frame.sub0 = (float (*)[4])*(cur - 1);
  up_row = blk->above;
  __frame.sub1 = (float (*)[4])up_row[1];
  __frame.sub2 = (float (*)[4])up_row[2];
  __frame.sub3 = (float (*)[4])*(cur - 2);
  __frame.sub4 = (float (*)[4])*up_row;
  __frame.sub5 = (float (*)[4])*cur;
  c_lo = 14 * sum_u;
  mode = 1;
  c_mid = 13 * sum_ur;
  if ( 16 * sum4 <= 14 * sum_u )
  {
    c_lo = 16 * sum4;
    mode = 0;
  }
  if ( c_lo > c_mid )
    mode = 2;
  else
    c_mid = c_lo;
  if ( c_mid > 11 * sum_ul )
    mode = 3;
  filt = __alt_p2_filter((float (*)[4])(void *)*(int32_t *)&blk->f278656, (float (*)[4])blk->p2_row, (CtxWeights *)__frame.sub, mode);
  cx0 = (P2Ctx *)blk->cursor[0];
  cx1 = (P2Ctx *)blk->cursor[1];
  *(int32_t *)&blk->pred_prev = filt;
  lane3 = cx1->aerr;
  g3pair = cx0[-1].aerr + lane3;
  if ( refa )
    g3pair += (rb0->aerr + ra0->aerr) >> 1;
  run = filt;
  cx3 = (P2Ctx *)(blk->cursor[3]);
  g3sum = cx1[3].aerr + cx1[-1].aerr + cx0[-1].aerr + cx0[-3].aerr + cx0[-4].aerr + cx0[-2].aerr;
  run_s = run;
  ctx0_lo = (cx1[1].err + cx1->err + cx1[-1].err + cx0[-1].err) & 0x80000
       | (cx3->err + cx0[-2].err + 2 * cx0[-4].err) & 0x40000
       | (cx1[-3].err + cx0[-3].err + cx0[-5].err + cx0[-7].err) & 0x20000
       | cx0[-1].err & 0x10000
       | (uint16_t)cx0[-3].err & 0x8000
       | (((run > 3536) + (run > 720) + (run > 288)) << 13)
       | ((((uint32_t)(752 - (g3pair + g3sum)) >> 31)
         + ((uint32_t)(400 - (g3pair + g3sum)) >> 31)
         + ((uint32_t)(240 - (g3pair + g3sum)) >> 31)) << 11);
  blk->bank_ctx[0] = ctx0_lo;
  if ( refa )
  {
    lane2 = ra0->err;
    run = run_s;
    ctx0 = rb0[-1].err & 0x2000000
         | rb0->err & 0x1000000
         | ra0[-1].err & 0x800000
         | (rb0->err + rb0[-2].err) & 0x400000
         | (ra0[-2].err + lane2) & 0x200000
         | cx0[-2].err & 0x100000
         | ctx0_lo;
    cx2 = (P2Ctx *)blk->cursor[2];
  }
  else
  {
    run = run_s;
    cx2 = (P2Ctx *)blk->cursor[2];
    ctx0 = (cx1[3].err + cx0[-3].err + cx0[-7].err + cx0[-5].err) & 0x2000000
         | (cx2[1].err + cx2->err + cx0[-4].err + cx0[-8].err) & 0x1000000
         | (cx0[-4].err + cx0[-6].err) & 0x800000
         | cx0[-4].err & 0x400000
         | cx0[-5].err & 0x200000
         | cx0[-7].err & 0x100000
         | ctx0_lo;
  }
  bank0 = ctx0 >> 11;
  blk->bank_ctx[0] = bank0;
  pred0 = p2_pred(blk->p2_ctr[bank0].w2, blk->p2_ctr[bank0].b0);
  sum_c = sum_all;
  cx4 = blk->cursor[4];
  blk->nb_sum[1] = pred0;
  run0 = pred0 + run_s;
  blk->nb_sum[0] = run0;
  nb4_4 = cx4[4].val;
  sum_all = ((g3pair << 9) + sum_c) >> 13;
  d_run0 = cx0[-5].val - run0;
  q24 = ((uint32_t)(24 - sum_all) >> 20) & 0xFFFFF800;
  d_up = nb4_4 - run0;
  up5 = cx1[5].val;
  d_up5 = up5 - run0;
  q39 = ((uint32_t)(39 - sum_all) >> 20) & 0xFFFFF800;
  if ( refa )
  {
    q10 = ((uint32_t)(10 - sum_all) >> 20) & 0xFFFFF800;
    dv_now = cx0[0].dval;
    q10a = q10;
    ctx1 = (cx0[-1].val + ra0->val - ra0[-1].val - run0) & 0x2000000
         | (ra2->dval + ra1->dval - 2 * ra0->dval) & 0x1000000
         | ((dv_now + cx0[-3].dval - run0 - (cx1[2].dval - cx1[5].dval)) & 0x800000 | (dv_now + cx0[-5].dval - run0) & 0x400000 | (dv_now + cx1->dval + rb1[2].dval - rb2[2].dval - run0) & 0x200000 | (dv_now + cx0[-1].dval + rb0->dval - rb0[-1].dval - run0) & 0x100000 | (cx2[-1].dval + dv_now + ra0->dval - ra2[-1].dval - run0) & 0x80000 | (dv_now + cx1[2].dval + ra0->dval - ra1[2].dval - run0) & 0x40000 | d_up5 & 0x20000 | d_up & 0x10000 | d_run0 & 0x8000 | (((run0 > 2256) + (run0 > 1056) + (run0 > 144)) << 13) | ((((uint32_t)(55 - sum_all) >> 20) & 0xFFFFF800) + q10 + q24));
  }
  else
  {
    q10 = ((uint32_t)(10 - sum_all) >> 20) & 0xFFFFF800;
    d_run0b = run0 - cx0[-3].val;
    ctx1 = (cx1[-5].val - cx1[-2].val + d_run0b) & 0x1000000
         | (d_run0b + cx1[2].val - up5) & 0x800000
         | (cx4[3].val - cx2[2].val + run0 - cx2[1].val) & 0x400000
         | (cx4[-1].val - cx3->val + run0 - cx1[-1].val) & 0x200000
         | (cx1->val - cx1[2].val + run0 - cx0[-2].val) & 0x100000
         | -d_up5 & 0x80000
         | -d_up & 0x40000
         | (run0 - cx4[2].val) & 0x20000
         | (run0 - cx4[-3].val) & 0x10000
         | -d_run0 & 0x8000
         | (((run0 > 2400) + (run0 > 1024) + (run0 > 240)) << 13)
         | (q39 + q24 + (((uint32_t)(11 - sum_all) >> 20) & 0xFFFFF800))
         | (nb4_4 - cx3->val + run0 - cx1[4].val) & 0x2000000;
    q10a = q10;
  }
  q10 = q10a;
  bank1 = ctx1 >> 11;
  blk->bank_ctx[1] = bank1;
  rate1 = blk->p2_ctr[bank1 + 32768].b0;
  w1c = blk->p2_ctr[bank1 + 32768].w2;
  one1 = 1 << ((rate1 + 31) & 31);
  pred1 = (one1 + w1c) >> (rate1 & 31);
  run1 = pred1 + run0;
  blk->nb_sum[2] = run1;
  blk->nb_sum[3] = pred1;
  cx2_val0 = cx2->val;
  cx1_val = cx1->val;
  dvsum2 = cx0[-2].val - run1;
  lap = cx0[-2].val + run1 - 2 * cx0[-1].val;
  dtop2 = cx2_val0 - run1;
  if ( refa )
  {
    dv1 = cx1[1].dval;
    d_ra = ra0->val - run1;
    q10b = q10;
    ctx2 = (cx0[0].dval + cx1[-1].dval - run1 - (cx2->dval - dv1)) & 0x2000000
         | (cx0[0].dval + cx0[-1].dval - run1 - (cx1->dval - dv1)) & 0x1000000
         | (rb0->val - run1 + cx2_val0 - rb2->val) & 0x800000
         | (d_ra + cx2_val0 - ra2->val) & 0x400000
         | (d_ra + cx1[-1].val - ra1[-1].val) & 0x200000
         | ((cx0[0].dval - cx0[-2].dval + 2 * cx0[-1].dval - run1) & 0x100000 | (2 * cx1_val - run1 - cx2_val0) & 0x80000 | -lap & 0x40000 | dtop2 & 0x20000 | dvsum2 & 0x10000 | (208 - rb0->val) & 0x8000 | (((run1 > 2576) + (run1 > 1280) + (run1 > 640)) << 13) | ((((uint32_t)(33 - sum_all) >> 31) + ((uint32_t)(12 - sum_all) >> 31) + ((uint32_t)(4 - sum_all) >> 31)) << 11));
  }
  else
  {
    q10b = q10;
    ctx2 = (run1 + 3 * (cx2_val0 - cx1_val) - cx3->val) & 0x2000000
         | (run1 + cx4->val - (cx2[2].val + cx2[-2].val)) & 0x1000000
         | (cx3[2].val - cx2[-1].val + run1 - cx1[3].val) & 0x800000
         | (cx3[1].val - cx1[1].val - dtop2) & 0x400000
         | (cx3->val - cx1_val - dtop2) & 0x200000
         | lap & 0x100000
         | (run1 - cx4[3].val) & 0x80000
         | (cx3[1].val - run1) & 0x40000
         | (run1 - cx4->val) & 0x20000
         | (run1 - cx3[-2].val) & 0x10000
         | -dvsum2 & 0x8000
         | (((run1 > 2464) + (run1 > 1216) + (run1 > 688)) << 13)
         | ((((uint32_t)(58 - sum_all) >> 31) + ((uint32_t)(25 - sum_all) >> 31) + ((uint32_t)(13 - sum_all) >> 31)) << 11);
  }
  q10 = q10b;
  bank2 = ctx2 >> 11;
  blk->bank_ctx[2] = bank2;
  pred2 = p2_pred(blk->p2_ctr[bank2 + 65536].w2, blk->p2_ctr[bank2 + 65536].b0);
  blk->nb_sum[5] = pred2;
  run2 = pred2 + run1;
  blk->nb_sum[4] = run2;
  run_dv3 = run2 - cx0[0].dval;
  q9 = 9 - sum_all;
  sum_all = -sum_all;
  bank3 = (int32_t)((3 * (cx0[-2].val - cx0[-1].val) + run2 - cx0[-3].val) & 0x2000000
             | (cx2[1].dval
              - ((uint32_t)(cx1[1].dval + cx1[2].dval + cx1[-1].dval + cx1->dval) >> 1)
              + run_dv3)
             & 0x1000000
             | (cx2[-3].val - cx1[-2].val + run2 - cx1[-1].val) & 0x800000
             | -(cx2[2].val + run2 - 2 * cx1[1].val) & 0x400000
             | (cx1[-2].dval - cx0[-2].val + run2 - cx1->dval) & 0x200000
             | (run2 - cx1[3].val) & 0x100000
             | (run_dv3 - cx2[1].dval) & 0x80000
             | (run2 - cx3->val) & 0x40000
             | (2 * run2 - cx1->dval - (cx1->val + cx0[0].dval)) & 0x20000
             | (run2 - cx0[-1].dval - cx0[0].dval) & 0x10000
             | (run2 - cx0[-3].val) & 0x8000
             | (((run2 > 2896) + (run2 > 1568) + (run2 > 592)) << 13)
             | ((((uint32_t)(sum_all + 37) >> 31) + ((uint32_t)(sum_all + 19) >> 31) + (q9 >> 31)) << 11)) >> 11;
  blk->bank_ctx[3] = bank3;
  w3c = blk->p2_ctr[bank3 + 98304].w2;
  // The rate reached `>>` through two `LOBYTE` copies, `v163` then `v164`;
  // both masks read only the byte each copy wrote.
  pred3 = p2_pred(w3c, blk->p2_ctr[bank3 + 98304].b0);
  blk->nb_sum[7] = pred3;
  run3 = pred3 + run2;
  run = run3;
  blk->nb_sum[6] = run3;
  dv_now4 = cx0[0].dval;
  run_dv4 = run3 - dv_now4;
  run_up4 = run3 + cx3->val;
  bank4 = (int32_t)((run - ((uint32_t)(cx1[1].val + cx1[-1].val + 2 * cx1->val) >> 2)) & 0x2000000
             | (run_up4 - cx0[-2].val - (cx3[2].dval + dv_now4)) & 0x1000000
             | (run - 2 * cx0[-2].dval - (dv_now4 - cx0[-4].dval)) & 0x800000
             | (run - cx0[-3].dval - dv_now4 - (cx1->val - cx1[-3].val)) & 0x400000
             | (run_up4 - dv_now4 - (cx1[1].val + cx2[-1].dval)) & 0x200000
             | (cx2[-2].val + run - 2 * cx1[-1].val) & 0x100000
             | (cx2->dval - 2 * cx1->dval + run_dv4) & 0x80000
             | ((run3 - cx0[-1].val) - (cx1->val - cx1[-1].val)) & 0x40000
             | (run_dv4 - cx1[4].dval) & 0x20000
             | (run_dv4 - cx1[-2].dval) & 0x10000
             | (((uint16_t)run3 - (uint16_t)cx0[-4].dval - (uint16_t)dv_now4) & 0x8000)
             | ((((run3 > 3056) + (run3 > 1952) + (run3 > 368)) << 13) | (q39 + (((uint32_t)(sum_all + 21) >> 20) & 0xFFFFF800) + q10))) >> 11;
  blk->bank_ctx[4] = bank4;
  pred4 = p2_pred(blk->p2_ctr[bank4 + 131072].w2, blk->p2_ctr[bank4 + 131072].b0);
  cx2p = (uint8_t *)cx2;
  blk->nb_sum[9] = pred4;
  run4 = run + pred4;
  blk->nb_sum[8] = run + pred4;
  magsum_s = cx1[5].mag
         + cx1[4].mag
         + cx1[-3].mag
         + cx1[-4].mag
         + 3 * (cx1[2].mag + cx2[1].mag)
         + 7 * cx1->mag
         + 6 * cx1[1].mag
         + cx0[-6].mag
         + cx0[-7].mag
         + cx0[-8].mag
         + 8 * cx0[-1].mag
         + cx4[2].mag
         + cx4[1].mag
         + cx4->mag
         + cx4[-1].mag
         + cx0[-4].mag
         + cx0[-5].mag
         + (cx3[2].mag + cx3[1].mag + cx3[-1].mag + cx3[-2].mag + cx4[-2].mag + cx2p[107] + cx2p[89] + cx2p[71] + (*(cx2p - 19)) + *(cx2p - 37))
         + 4 * (cx1[-1].mag + cx0[-2].mag + cx2p[17])
         + 2 * (cx1[-2].mag + cx1[3].mag + cx0[-3].mag + cx3->mag + *(cx2p - 1) + cx2p[53]);
  band_lo = blk->band_lo;
  blk->ctx_w[0].sel = (run4 < 1840) + (run4 < 272);
  band_hi = blk->band_hi;
  blk->ctx_w[1].sel = ((run4 - cx1->val <= band_hi) + (run4 - cx1->val < band_lo));
  d_run4 = run4 - cx0[-1].val;
  in_band = d_run4 <= band_hi;
  blk->ctx_w[2].sel = in_band + (d_run4 < band_lo);
  blk->ctx_w[3].sel = cx1->sign;
  // The previous record's stored sign digit, straight into the fifth
  // group's selector.  Hex-Rays read it as `(uint8_t)lane[8]` -- one past
  // the eight lanes, whose low byte is `sign`.
  blk->ctx_w[4].sel = cx0[-1].sign;
  magsum = magsum_s;
  magu = (cx2[0].mag + cx1->mag);
  run_s2 = run;
  magl = cx0[-1].mag + cx0[-2].mag;
  if ( refa )
  {
    mag = rb0->mag;
    mag_ref1 = rb1->mag + ra1->mag;
    mag_ref0 = mag + ra0->mag;
    magsum = mag_ref1 + magsum_s + 4 * mag_ref0 + 2 * (ra0[-1].mag + rb0[-1].mag);
    flat_a = mag_ref0 + magl + rb0[-2].mag + rb0[-1].mag + ra0[-2].mag + ra0[-1].mag;
    run_s2 = run;
    if ( blk->has_ref )
    {
      d_run4b = run4 - cx1->dval - cx0[0].dval;
      if ( d_run4b < band_lo || d_run4b > band_hi )
      {
        run4 = run4 - cx0[-1].dval - cx0[0].dval;
        flat_b = run4 >= band_lo && band_hi >= run4;
      }
      else
      {
        flat_b = 1;
      }
    }
    else
    {
      flat_b = mag_ref1 + mag_ref0 + magu + rb2->mag + ra2->mag;
    }
    if ( *(int32_t *)&blk->plane_idx == 1 )
    {
      magsum_s = magsum;
      d_ra_ra1 = ra0->val - ra1->val;
      blk->ctx_w[3].sel = (d_ra_ra1 <= band_hi) + (d_ra_ra1 < band_lo);
      d_ra_left = ra0->val - ra0[-1].val;
      magsum = magsum_s;
      run_s2 = run;
      blk->ctx_w[4].sel = ((d_ra_left <= band_hi) + (d_ra_left < band_lo));
    }
    else if ( *(int32_t *)&blk->plane_idx > 1 )
    {
      magsum_s = magsum;
      d_rb_rb1 = rb0->val - rb1->val;
      blk->ctx_w[3].sel = (d_rb_rb1 <= band_hi) + (d_rb_rb1 < band_lo);
      d_rb_left = rb0->val - rb0[-1].val;
      in_band = d_rb_left <= band_hi;
      magsum = magsum_s;
      blk->ctx_w[4].sel = in_band + (d_rb_left < band_lo);
      ra0_val = ra0->val;
      d_ra_ra1b = ra0_val - ((P2Ctx *)(ra1))->val;
      in_band = band_lo <= d_ra_ra1b;
      run_s2 = run;
      if ( in_band && d_ra_ra1b <= band_hi )
      {
        flat_a = 1;
      }
      else
      {
        d_ra_leftb = ra0_val - ra0[-1].val;
        flat_a = d_ra_leftb >= band_lo && d_ra_leftb <= band_hi;
      }
    }
  }
  else
  {
    flat_a = magl + cx0[-3].mag + cx0[-4].mag + cx0[-5].mag;
    flat_b = cx4->mag + cx3->mag + magu + cx0[0].mag;
  }
  if ( magsum >= 960 )
  {
    ctx15 = 15;
    blk->ctx = 15;
  }
  else
  {
    ctx15 = blk->nb_ctx[magsum >> 3];
    blk->ctx = ctx15;
  }
  ctx_idx = (run_s2 + pred4 + 7) >> 4;
  if ( ctx_idx >= 255 )
    ctx_idx = 255;
  if ( ctx_idx < 0 )
    ctx_idx = 0;
  blk->ctx_pair[0] = blk->ctx_delta[ctx_idx + 4] + ctx15;
  blk->ctx_pair[1] = ctx15 + blk->ctx_delta[ctx_idx];
  blk->ctx = ctx15
                          + 32 * (flat_b == 0)
                          + 16 * (flat_a == 0)
                          + blk->ctx_w[4].w[blk->ctx_w[4].sel]
                          + blk->ctx_w[3].w[blk->ctx_w[3].sel]
                          + blk->ctx_w[2].w[blk->ctx_w[2].sel]
                          + blk->ctx_w[1].w[blk->ctx_w[1].sel]
                          + blk->ctx_w[0].w[blk->ctx_w[0].sel];
  return ctx_idx;
}


// Find the distinct symbol values a plane actually uses, number them 0..n-1,
// rewrite the plane in those numbers, and code the numbering so the decoder can
// undo it.  `expand_alphabet` is the other half.

// Two paths.  At eight bits or fewer the value fits a byte, so the map is a
// 64 KiB flag array indexed by value; the plane is walked once to set a flag
// per value seen, the flags are renumbered in order, and the plane is walked
// again to substitute.  The numbering goes out as gaps between consecutive
// used values through one symbol list.

// Above eight bits the value does not fit an index, so the same job is done
// with a **binary search tree** over the distinct values: `buf[8 * node]` holds
// a value, `v82[node]` holds its two child indices as `uint16_t`, and the
// comparison at each step picks the side.  A value not found is inserted and
// takes the next number.  That is the same interning idea `ModelBlock`'s
// `ctx_id1/2/3` use for context signatures, done here for symbols.

// Above 0x2000 distinct values it gives up on both and splits the plane into
// byte planes -- height times the byte count, depth 8 -- then calls itself.

// The `Blockaa_1..4` reloads of `__frame.slot7` are *not* foldable, and that is
// worth saying because everything else of that shape in this file was.  The
// frame's `slot` array is also walked by index -- `slot[2 * j + 2]`,
// `slot[v44 + 1]` -- and those indices can reach 7, so `slot7` is not
// guaranteed to still hold the block when the next reload reads it.  One
// storage, two uses, and no way to tell them apart from the text.
void __reduce_alphabet(ModelBlock *blk, int8_t unread_flag, uint8_t *src)
{
  // This one is a layout, not a bag of locals: giving its members
  // their own storage makes DLRAW abort while compressing.  Re-checked
  // by `tools/liftframe.py` against the file as it is now --
  // `frame-sweep.sh`, which this note used to cite, lifts aliases
  // and has had none to lift since round nine.
  struct alignas(16) ReduceAlphabetFrame {   // 66064 bytes, one stack frame
      uint32_t zero_base[15];
      void *tmp;
      int32_t kids_i;
      // 64 KiB: `memset(buf, 0, 0x10000)` clears it and the body walks it as
      // 8192 eight-byte records, so `kids`, `y_spill`, `at_spill`, `bits_spill` and `_pad0` are
      // inside it -- the same bytes under other names at other times.  It stays
      // four bytes here because `kids` wants eight-byte alignment and `buf` does
      // not, and a union of the two would move the whole frame four bytes.
      uint8_t buf[4];
      uint64_t kids[127];
      int32_t y_spill;
      int32_t at_spill;
      int32_t bits_spill;
      uint8_t _pad0[64504];
      // The same sixteen `SymList`s `expand_alphabet`'s frame ends in, and
      // recovered the same way -- as three untyped members.  16 + 4 + 364 is
      // 384, which is 16 * sizeof(SymList), and the free loop below walks back
      // from `slot` six words at a time taking `[5]`, which is `ent`.
      SymList lists[16];
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
                || __builtin_offsetof(__typeof__(__frame), lists)
                   - __builtin_offsetof(__typeof__(__frame), buf) == 0x10000,
                "buf is not the 64 KiB the memset clears");
  ;
  ModelBlock *blk2;
  int8_t mode;
  ModelBlock *blk1, *blk4;
  uint32_t alpha_n, done, done2, off, slot_a, sym, depth_bits;
  bool more, packed, first;
  uint8_t *half;   // `uint8_t *` beside the `char` scalars above
  int32_t node, side, alpha_m, carry, img_w, img_h, *slotp, z2,
          n_moved, zoff, height, n_distinct, row_w, y, at, bits, bpp, shift, sym2, *slotp2, z1, alphabet,
          alpha, prev, s, s_next, *slotp1, z0;
  ModelBlock *blk3;
  uint32_t n_kids, i, written, li, si, si_end, word, k, pairs, n_pairs, j,
           x, idx, n_syms, n_syms3, next_id, m;
  uint16_t *kidp;   // was uint64_t *, read only as uint16_t
  uint8_t *srcp, *p, *rp, *dst_a, *dst_b, *dst_c, *q;
  void *val, *newbuf;
  __frame.slot11 = (ModelBlock *)(blk);
  srcp = src;
  depth_bits = blk->depth;
  __frame.slot2 = src;
  __frame.slot7 = (ModelBlock *)(blk);
  __frame.slot4 = 0xFFFFFFFF >> (-depth_bits & 31);
  n_kids = (depth_bits + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    __frame.lists[2 * i].ent = nullptr;
    __frame.lists[2 * i + 1].ent = nullptr;
  }
  blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
  if ( depth_bits <= 8 )
  {
    // 1024 bytes cleared, sixteen at a time from the top: `zero_base` is
    // 60 bytes before `buf`, and `&zero_base[16]` through `&zero_base[268]`
    // is `buf[0 .. 1023]` -- the 256 four-byte flags an eight-bit alphabet
    // needs.  The other path memsets the whole 64 KiB because its values do
    // not fit 256 slots.
    zoff = 256;
    do
    {
      bmf_zero16(&__frame.zero_base[zoff + 12]);
      bmf_zero16(&__frame.zero_base[zoff + 8]);
      bmf_zero16(&__frame.zero_base[zoff + 4]);
      bmf_zero16(&__frame.zero_base[zoff]);
      zoff -= 16;
    }
    while ( zoff * 4 );
    packed = *(int32_t *)&blk1->depth < 8;
    height = blk1->height;
    *(int32_t *)&blk1->alphabet = 0;
    if ( packed )
    {
      n_distinct = 0;
      if ( height )
      {
        row_w = *(int32_t *)&blk1->width;
        __frame.slot8 = src - 1;
        __frame.at_spill = 0;
        y = 0;
        do
        {
          if ( !row_w )
            break;
          __frame.y_spill = y;
          x = 0;
          at = __frame.at_spill;
          bits = 0;
          do
          {
            bpp = *(int32_t *)&blk1->depth;
            shift = bits - bpp;
            if ( shift < 0 )
            {
              ++__frame.slot8;
              shift = 8 - bpp;
            }
            sym = __frame.slot4 & (*__frame.slot8 >> (shift & 31));
            first = *(uint32_t *)&__frame.buf[4 * sym - 4] == 0;
            __frame.bits_spill = shift;
            ++x;
            *(uint32_t *)&__frame.buf[4 * sym - 4] = 1;
            bits = __frame.bits_spill;
            *(int32_t *)&blk1->alphabet += first;
            blk1->sym_word[at] = sym;
            row_w = *(int32_t *)&blk1->width;
            ++at;
          }
          while ( x < (uint32_t)(*(int32_t *)&blk1->width) );
          n_distinct = *(int32_t *)&blk1->alphabet;
          __frame.at_spill = at;
          y = __frame.y_spill + 1;
        }
        while ( (uint32_t)(__frame.y_spill + 1) < (uint32_t)blk1->height );
      }
    }
    else if ( height * *(int32_t *)&blk1->width )
    {
      q = __frame.slot2;
      idx = 0;
      do
      {
        *(int32_t *)&blk1->alphabet += *(uint32_t *)&__frame.buf[4 * *q - 4] == 0;
        sym2 = *q;
        *(uint32_t *)&__frame.buf[4 * sym2 - 4] = 1;
        ++q;
        blk1->sym_word[idx++] = sym2;
      }
      while ( idx < (uint32_t)(blk1->height * *(int32_t *)&blk1->width) );
      n_distinct = *(int32_t *)&blk1->alphabet;
    }
    else
    {
      n_distinct = 0;
    }
    rc.encode(n_distinct - 1, n_distinct, __frame.slot4 + 1);
    n_syms = *(int32_t *)&blk1->alphabet;
    if ( n_syms <= __frame.slot4 )
    {
      __init_symbol_list(&__frame.lists[0], (int32_t)blk1, __frame.slot4 - n_syms + 2, 1);
      __frame.lists[0].rescale_at = 19 * __frame.lists[0].n;
      n_syms3 = *(int32_t *)&blk1->alphabet;
      if ( n_syms3 )
      {
        prev = 0;
        s = 0;
        next_id = 0;
        do
        {
          if ( *(uint32_t *)&__frame.buf[4 * s - 4] )
          {
            __frame.lists[0].code_symbol(s - prev);
            n_syms3 = *(int32_t *)&blk1->alphabet;
            *(uint32_t *)&__frame.buf[4 * s - 4] = next_id;
            s_next = s + 1;
            prev = s + 1;
            ++next_id;
          }
          else
          {
            s_next = s + 1;
          }
          s = s_next;
        }
        while ( next_id < n_syms3 );
      }
      if ( blk1->height * *(int32_t *)&blk1->width )
      {
        m = 0;
        do
        {
          blk1->sym_word[m] = *(uint32_t *)&__frame.buf[4
                                                                  * blk1->sym_word[m]
                                                                  - 4];
          ++m;
        }
        while ( m < (uint32_t)(blk1->height * *(int32_t *)&blk1->width) );
      }
      slotp1 = (int32_t *)__frame.slot;
      z0 = 16;
      do
      {
        slotp1 -= 6;
        free((void *)slotp1[5]);
        --z0;
      }
      while ( z0 );
    }
    else
    {
      slotp2 = (int32_t *)__frame.slot;
      z1 = 16;
      do
      {
        slotp2 -= 6;
        free((void *)slotp2[5]);
        --z1;
      }
      while ( z1 );
    }
  }
  else
  {
    memset(__frame.buf,0,0x10000);
    *(int32_t *)&blk1->alphabet = 1;
    *(uint32_t *)__frame.buf = __frame.slot4 & *(uint32_t *)src;
    blk1->sym_word[0] = 0;
    if ( (uint32_t)(blk1->height * *(int32_t *)&blk1->width) > 1 )
    {
      __frame.slot8 = src;
      __frame.slot9 = n_kids;
      __frame.slot7 = (ModelBlock *)((int32_t)blk1);
      p = __frame.slot2;
      node = 0;
      written = 1;
      while ( 1 )
      {
        p += __frame.slot9;
        val = (void *)(__frame.slot4 & *(uint32_t *)p);
        if ( val != *(void **)&__frame.buf[8 * node] )
        {
          node = 0;
          if ( val != *(void **)__frame.buf )
          {
            __frame.slot3 = written;
            __frame.slot2 = p;
            while ( 1 )
            {
              side = *(uint32_t *)&__frame.buf[8 * node] < (uint32_t)val;
              kidp = (uint16_t *)&__frame.kids[node];
              node = kidp[side];
              if ( !kidp[side] )
                break;
              if ( val == *(void **)&__frame.buf[8 * node] )
              {
                written = __frame.slot3;
                p = __frame.slot2;
                mode_symbol[1] = side;
                goto LABEL_12;
              }
            }
            __frame.kids_i = (int32_t)kidp;
            p = __frame.slot2;
            __frame.slot0 = side;
            (__frame.slot[1]) = val;
            blk2 = (ModelBlock *)(__frame.slot7);
            alphabet = __frame.slot7->alphabet;
            mode_symbol[1] = side;
            node = (uint16_t)alphabet;
            alpha = alphabet + 1;
            *(uint16_t *)(__frame.kids_i + 2 * side) = node;
            written = __frame.slot3;
            blk2->alphabet = alpha;
            if ( alpha > 0x2000 )
            {
              srcp = __frame.slot8;
              alpha_n = alpha;
              n_kids = __frame.slot9;
              blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
              goto LABEL_14;
            }
            *(void **)&__frame.buf[8 * node] = (__frame.slot[1]);
          }
        }
LABEL_12:
        blk3 = (ModelBlock *)((uint32_t *)__frame.slot7);
        __frame.slot7->sym_word[written++] = node;
        if ( written >= *(uint32_t *)&blk3->height * blk3->width )
        {
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
          alpha_n = __frame.slot7->alphabet;
          goto LABEL_14;
        }
      }
    }
    alpha_n = *(int32_t *)&blk1->alphabet;
LABEL_14:
    rc.encode(alpha_n - 1, alpha_n, 0x2001u);
    alpha_m = *(int32_t *)&blk1->alphabet;
    if ( alpha_m > 0x2000 )
    {
      (__frame.slot[1]) = bmf_new(blk1->height * n_kids * *(int32_t *)&blk1->width);
      img_w = *(int32_t *)&blk1->width;
      img_h = blk1->height;
      __frame.kids_i = *(int32_t *)&blk1->width;
      __frame.slot0 = img_h;
      if ( n_kids )
      {
        __frame.slot6 = __frame.slot0 * img_w;
        if ( n_kids >> 1 )
        {
          half = (uint8_t *)(__frame.slot[1]) + __frame.slot0 * __frame.kids_i;
          __frame.slot8 = srcp;
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock *)((int32_t)blk1);
          pairs = 0;
          do
          {
            slot_a = 2 * pairs;
            off = 2 * pairs++ * __frame.slot6;
            __frame.slot[slot_a + 2] = (uint8_t *)(__frame.slot[1]) + off;
            __frame.slot[slot_a + 3] = (void *)&half[off];
          }
          while ( pairs < n_kids >> 1 );
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
          done = 2 * pairs + 1;
        }
        else
        {
          done = 1;
        }
        if ( n_kids > (done - 1) )
          __frame.slot[done + 1] = (uint8_t *)(__frame.slot[1]) + __frame.slot0 * -__frame.kids_i + __frame.slot6 * done;
      }
      else
      {
        __frame.slot6 = __frame.slot0 * img_w;
      }
      if ( __frame.slot6 )
      {
        rp = src;
        if ( n_kids )
        {
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock *)((int32_t)blk1);
          __frame.kids_i = 0;
          n_moved = 0;
          n_pairs = n_kids >> 1;
          while ( 1 )
          {
            while ( 1 )
            {
              if ( n_pairs )
              {
                for ( j = 0; j < n_pairs; ++j )
                {
                  dst_a = (uint8_t *)(__frame.slot[2 * j + 2]);
                  *dst_a = rp[2 * j];
                  dst_b = (uint8_t *)__frame.slot[2 * j + 3];
                  __frame.slot[2 * j + 2] = dst_a + 1;
                  *dst_b = rp[2 * j + 1];
                  __frame.slot[2 * j + 3] = dst_b + 1;
                }
                done2 = 2 * j + 1;
                srcp = &rp[2 * j];
              }
              else
              {
                done2 = 1;
              }
              if ( (done2 - 1) >= __frame.slot9 )
                break;
              dst_c = (uint8_t *)__frame.slot[done2 + 1];
              srcp = &rp[done2];
              *dst_c = rp[done2 - 1];
              more = (uint32_t)(++n_moved) < __frame.slot6;
              __frame.slot[done2 + 1] = dst_c + 1;
              if ( !more )
                goto LABEL_71;
              rp += done2;
            }
            if ( (uint32_t)(++n_moved) >= __frame.slot6 )
              break;
            rp = srcp;
          }
LABEL_71:
          n_kids = __frame.slot9;
          blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
        }
      }
      __frame.tmp = blk1->sym_word;
      blk1->height = n_kids * __frame.slot0;
      *(int32_t *)&blk1->depth = 8;
      free(__frame.tmp);
      newbuf = bmf_new(2 * blk1->height * *(int32_t *)&blk1->width);
      __frame.tmp = (__frame.slot[1]);
      blk1->sym_word = (uint16_t *)newbuf;
      __reduce_alphabet((ModelBlock *)blk1, mode, (uint8_t *)__frame.tmp);
      free((__frame.slot[1]));
    }
    else
    {
      if ( 4 * n_kids )
      {
        __frame.slot7 = (ModelBlock *)((int32_t)blk1);
        li = 0;
        do
        {
          __init_symbol_list(&__frame.lists[li], li, 256, 1);
          ++li;
        }
        while ( li < 4 * n_kids );
        blk1 = (ModelBlock *)((int32_t *)__frame.slot7);
        alpha_m = __frame.slot7->alphabet;
      }
      if ( alpha_m )
      {
        carry = 0;
        si = 0;
        si_end = alpha_m;
        blk4 = (ModelBlock *)((int32_t)blk1);
        do
        {
          word = *(uint32_t *)&__frame.buf[8 * si];
          if ( n_kids )
          {
            __frame.kids_i = si;
            __frame.slot9 = n_kids;
            __frame.slot7 = (ModelBlock *)(blk4);
            for ( k = 0; k < __frame.slot9; ++k )
            {
              __frame.lists[4 * k + carry].code_symbol((uint8_t)word);
              carry = (uint8_t)word >> 6;
              word >>= 8;
            }
            si = __frame.kids_i;
            n_kids = __frame.slot9;
            blk4 = (ModelBlock *)(__frame.slot7);
            word = *(uint32_t *)&__frame.buf[8 * __frame.kids_i];
            si_end = __frame.slot7->alphabet;
          }
          carry = (uint8_t)word >> 7;
          ++si;
        }
        while ( si < si_end );
      }
    }
    slotp = (int32_t *)__frame.slot;
    z2 = 16;
    do
    {
      slotp -= 6;
      free((void *)slotp[5]);
      --z2;
    }
    while ( z2 );
  }
}

// `n2` is the candidate index 0, 1 or 2 -- the callers pass those three
// literals -- and `a8` is an array of four-word cost records, one per
// candidate.  Both arrived as `uint8_t *` and neither is an address.
// Price one candidate plane pairing, in bits, for `choose_plane_coding`'s
// search.  Six histograms and two least-squares weights.

// The first loop walks the plane once and bins five differences per pixel:
// `dx` and `dy` are the two-dimensional gradients of the two planes being
// paired, `dz` is the third plane's, and the last two bins are `dz - dx`,
// `dz - dy` and `dz` against the average of `dx` and `dy`.  It accumulates the
// five products a two-variable least-squares fit needs -- `sxx`, `syy`, `sxy`,
// `sxz`, `syz` -- in `double`, which is the only floating-point arithmetic in
// the encoder's search.

// `w1` and `w2` come out of that fit, clamped to [-64, 191] and used as
// sevenths-of-a-bit fixed point: the second loop bins `dz` minus the weighted
// blend `(w1 * dx + w2 * dy + 40) >> 7`, which is exactly the residual
// `colour_transform`'s predictor 2 would leave.  `estimate_cost` turns each
// histogram into bits.

// The tail picks the cheaper of two pairings, swapping the two weights and the
// two cost words with it, and writes the winner into the descriptor.

// `a4`, `a5`, `a6` and `a7` are unread.
int32_t __cost_candidate(uint8_t *img, int32_t cand, uint8_t *desc, int8_t unread4, int32_t unread5, int32_t unread6, int32_t unread7, uint32_t *costs)
{
  // This one is a layout, not a bag of locals: lifting its members to
  // ordinary locals makes altp1 abort while compressing.  Re-checked against the
  // file as it is now, by `tools/liftframe.py` -- `frame-sweep.sh`, which
  // the note here used to cite, lifts aliases and has none left to lift.
  struct alignas(16) CostCandidateFrame {   // 26712 bytes, one stack frame
      uint8_t buf[4096];
      int32_t hist_y[1024];
      int32_t hist_yx[1024];
      int32_t hist_zx[1024];
      int32_t hist_zy[1024];
      int32_t hist_zp[1024];
      uint8_t buf_1[4];
      uint32_t img_end;
      int32_t step_d;
      int32_t off_b0;
      int32_t off_b1;
      int32_t off_a0;
      int32_t off_a1;
      int32_t nplanes;
      int32_t off_a2;
      int32_t off_b2;
      uint8_t _pad0[2008];
      int32_t cand_i;
      uint8_t *desc_f;
      uint8_t *img_f;
      int32_t rec_off;
      // `v91` held three unrelated `int32_t` here in three phases -- a base
      // address, a pixel difference inside the inner loop, and a cost -- with
      // no overlap between them.  All three are locals now and the slot is
      // dead; it stays to hold the layout.
      int32_t _pad91;
      int32_t d1_f;
      int32_t best;
      int32_t rows;
      int32_t d2_f;
      int32_t wb;
      int32_t wa;
      uint8_t *r1_f;
      uint8_t *r0_f;
      uint8_t *cursor;
      int32_t off_up;
      uint8_t _pad1[28];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 26720, "frame layout moved");
  ;
  int32_t cand_base;   // was `__frame.v91`, phase one: `&base[cand2]` as a number
  int32_t cost;        // was `__frame.v91`, phase three: an `estimate_cost`
  int32_t off;      // a byte offset into `descp`, which is the address
  uint8_t *descp;
  bool deep;
  uint32_t row_b2, swap;
  double syz, syy, sxz, sxy, sxx, inv, w1f, w2f;
  int32_t row_b, d1, d2, dx, o1, dy, o2, diag, west, dz, bin, w1,
          w2, img_w, nstep, at, at1, e0, e1, e2, left, bin2, pick, c0, rec,
          c2, lo1, s1, s2, c2b, tmp, w2s, w1s;
  uint32_t lo2, lo3;
  uint8_t *base, *p, *q, *r0, *r2, *r1;
  int32_t cand2, idx1, idx2;   // candidate indices, not addresses
  __frame.off_up = (int32_t)(uintptr_t)desc;
  __frame.cursor = (uint8_t *)cand;   // the slot is reused as an address below
  __frame.r0_f = img;
  __frame.desc_f = desc;
  __frame.nplanes = plane_count;
  row_b = *(const uint16_t *)&((const BmfImage *)img)->stride;   // the low half of the stride
  __frame.cand_i = cand;
  __frame.img_f = img;
  __frame.d1_f = (int32_t)(__frame.cursor + 1) % 3 - (uint32_t)__frame.cursor;
  __frame.d2_f = (int32_t)(__frame.cursor + 2) % 3 - (uint32_t)__frame.cursor;
  __frame.img_end = (uint32_t)&__frame.r0_f[*((uint32_t *)__frame.r0_f + 3) + 16];
  d1 = __frame.d1_f;
  *(uint32_t *)__frame.buf_1 = row_b;
  memset(__frame.buf,0,24576);
  row_b2 = *(uint32_t *)__frame.buf_1;
  cand2 = __frame.cand_i;
  base = __frame.img_f;
  syz = 0;
  syy = 0.0;
  sxz = 0.0;
  sxy = 0.0;
  sxx = 0.0;
  __frame.rec_off = 16 * __frame.cand_i;
  __frame.desc_f[16 * __frame.cand_i] = 2;
  __frame.desc_f[33] = (uint8_t)cand2;
  cand_base = (int32_t)&base[cand2];
  p = &base[cand2 + 16 + row_b2 + __frame.nplanes];
  if ( (uint32_t)p < __frame.img_end )
  {
    __frame.d1_f = d1;
    *(uint32_t *)__frame.buf_1 = row_b2;
    __frame.off_a2 = d1 - row_b2;
    __frame.off_a1 = d1 - row_b2 - __frame.nplanes;
    __frame.off_a0 = d1 - __frame.nplanes;
    __frame.off_b2 = __frame.d2_f - row_b2;
    __frame.off_b1 = __frame.d2_f - row_b2 - __frame.nplanes;
    __frame.off_b0 = __frame.d2_f - __frame.nplanes;
    __frame.step_d = row_b2 + __frame.nplanes;
    do
    {
      d2 = __frame.d2_f;
      dx = p[__frame.off_a1] + p[__frame.d1_f] - (p[__frame.off_a2] + p[__frame.off_a0]);
      o1 = __frame.off_b1;
      ++*(uint32_t *)&__frame.buf[4 * dx + 2048];
      dy = p[o1] + p[d2] - (p[__frame.off_b2] + p[__frame.off_b0]);
      o2 = __frame.step_d;
      sxx = sxx + (double)dx * (double)dx;
      ++__frame.hist_y[dy + 512];
      syy = syy + (double)dy * (double)dy;
      sxy = sxy + (double)dx * (double)dy;
      ++__frame.hist_yx[((uint16_t)dy - (uint16_t)dx - 512) & 0x3FF];
      // The same four-term cross difference `dx` and `dy` are, on this plane:
      // (northwest + here) - (north + west), where north is one row back and
      // west one pixel, so `o2` is `stride + plane_count`.
      diag = p[-o2] + *p;
      west = p[-__frame.nplanes];
      q = &p[-*(uint32_t *)__frame.buf_1];
      p += __frame.nplanes;
      dz = diag - (*q + west);
      sxz = sxz + (double)dx * (double)dz;
      ++__frame.hist_zx[((uint16_t)dz - (uint16_t)dx - 512) & 0x3FF];
      syz = syz + (double)dy * (double)dz;
      ++__frame.hist_zy[((uint16_t)dz - (uint16_t)dy - 512) & 0x3FF];
      bin = ((uint16_t)dz - (uint16_t)((uint32_t)(((dx + dy) << 6) + 40) >> 7) - 512) & 0x3FF;
      ++__frame.hist_zp[bin];
    }
    while ( (uint32_t)p < __frame.img_end );
    d1 = __frame.d1_f;
  }
  inv = 128.0 / (0.1 - sxy * sxy + sxx * syy);
  w1f = (syy * sxz - sxy * syz) * inv;
  w2f = inv * (sxx * syz - sxy * sxz);
  w1 = (int32_t)w1f;
  if ( (int32_t)w1f >= 191 )
    w1 = 191;
  if ( w1 < -64 )
    w1 = -64;
  __frame.wa = w1;
  w2 = (int32_t)w2f;
  if ( (int32_t)w2f >= 191 )
    w2 = 191;
  if ( w2 < -64 )
    w2 = -64;
  __frame.wb = w2;
  memset(__frame.buf_1,0,2048);
  img_w = *((uint16_t *)__frame.img_f + 2);
  __frame.d1_f = d1;
  __frame.rows = ((*((uint16_t *)__frame.img_f + 1) - 1) * *(uint16_t *)__frame.img_f) - 1;
  __frame.best = -img_w;
  nstep = -plane_count;
  at = cand_base - (-img_w - plane_count);
  __frame.off_up = -img_w - plane_count;
  __frame.r0_f = (uint8_t *)(at + 16);
  at1 = d1 + at + 16;
  r0 = (uint8_t *)(at + 16);
  __frame.r1_f = (uint8_t *)at1;
  __frame.cursor = (uint8_t *)(__frame.d2_f + at + 16);
  r2 = __frame.cursor;
  r1 = (uint8_t *)at1;
  do
  {
    e0 = r0[__frame.off_up];
    __frame.r1_f = r1;
    __frame.r0_f = r0;
    __frame.cursor = r2;
    e1 = e0 + *r0 - r0[__frame.best] - r0[nstep];
    e2 = *r1;
    left = __frame.rows;
    r2 = &__frame.cursor[-nstep];
    bin2 = ((uint16_t)e1
         - (uint16_t)((__frame.wa * (r1[__frame.off_up] + e2 - r1[__frame.best] - r1[nstep])
                             + __frame.wb * (__frame.cursor[__frame.off_up] + *__frame.cursor - __frame.cursor[__frame.best] - (uint32_t)__frame.cursor[nstep])
                             + 40) >> 7)
         - 256)
        & 0x1FF;
    r1 -= nstep;
    ++*(uint32_t *)&__frame.buf_1[4 * bin2];
    r0 = &__frame.r0_f[-nstep];
    __frame.rows = left - 1;
  }
  while ( left != 1 );
  pick = __frame.d1_f;
  c0 = __estimate_cost((uint8_t *)__frame.buf_1, 512);
  rec = 4 * __frame.cand_i;   // this candidate's four-word record
  costs[rec] = c0;
  costs[rec + 1] = __estimate_cost((uint8_t *)__frame.hist_zx, 1024);
  costs[rec + 2] = __estimate_cost((uint8_t *)__frame.hist_zy, 1024);
  costs[rec + 3] = __estimate_cost((uint8_t *)__frame.hist_zp, 1024);
  cost = __estimate_cost((uint8_t *)__frame.buf, 1024);
  __frame.d1_f = __estimate_cost((uint8_t *)__frame.hist_y, 1024);
  c2 = __estimate_cost((uint8_t *)__frame.hist_yx, 1024);
  __frame.rows = c2;
  lo1 = c2;
  if ( cost < c2 )
    lo1 = cost;
  if ( __frame.d1_f < c2 )
    c2 = __frame.d1_f;
  s1 = __frame.d1_f + lo1;
  s2 = cost + c2;
  __frame.best = s2;
  c2b = __frame.rows;
  if ( (s1 < s2) )
  {
    __frame.best = s1;
    tmp = pick;
    pick = __frame.d2_f;
    cost = __frame.d1_f;
    __frame.d2_f = tmp;
    swap = costs[rec + 1];
    costs[rec + 1] = costs[rec + 2];
    w2s = __frame.wb;
    costs[rec + 2] = swap;
    w1s = __frame.wa;
    __frame.wa = w2s;
    __frame.wb = w1s;
  }
  off = __frame.rec_off;
  descp = __frame.desc_f;
  *(uint32_t *)&__frame.desc_f[__frame.rec_off + 4] = __frame.wa;
  *(uint32_t *)&descp[off + 8] = __frame.wb;
  idx1 = __frame.cand_i + pick;
  idx2 = __frame.cand_i + __frame.d2_f;
  descp[16 * idx1] = 0;
  descp[1] = (uint8_t)idx1;
  deep = 0;                            // -S
  descp[16 * idx2] = 1;
  descp[17] = (uint8_t)idx2;
  if ( !deep && *((uint32_t *)__frame.img_f + 3) > 0x1000000u )
    return cost + c2b + costs[rec];
  lo2 = costs[rec];
  lo3 = costs[rec + 2];
  if ( lo2 >= costs[rec + 1] )
    lo2 = costs[rec + 1];
  if ( lo3 >= costs[rec + 3] )
    lo3 = costs[rec + 3];
  if ( lo2 < lo3 )
    lo3 = lo2;
  return __frame.best + lo3;
}


int32_t __choose_plane_coding(BmfImage *img, int32_t unused_h, int8_t unused_c)
{
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "altp1 segfaults while compressing"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
  struct alignas(16) ChoosePlaneCodingFrame {   // 41456 bytes, one stack frame
      uint8_t _pad0[4];
      union {
          uint8_t buf[32768];
          struct {
            uint8_t _buf_head[4096];
            int32_t hist_a[1024];
            int32_t hist_b[1024];
            int32_t hist_c[5120];
          };
      };
      union {
          uint8_t buf_1[2048];
          struct {
            uint8_t _buf_1_head[4];
            int32_t s0;
            int32_t s1;
            int32_t s2;
            uint8_t *p0;
            uint8_t *p1;
            int32_t s3;
            int32_t s4;
            uint8_t *p2;
            int32_t s5;
            int32_t s6;
            int32_t s7;
            uint8_t *p3;
            uint8_t *p4;
            int32_t s8;
            int32_t s9;
            uint8_t *p5;
            int32_t s10;
            uint8_t _pad3[1976];
          };
      };
      union {
          int64_t q0;              // the solve's first coefficient, as a double
          struct {
              uint32_t row_stride;   // `(uint16_t)img->stride`
              uint32_t plane_b;      // `plane_desc[2].src_plane - x3[2]`
          };
      };
      union {
          int64_t q1;              // the solve's second coefficient
          struct {
              uint32_t plane_a;      // `plane_desc[1].src_plane - x3[2]`
              uint32_t nplanes_s;    // the plane count, kept across the search
          };
      };
      union {
          double d0;               // the solve's third coefficient
          // The weight the search is tuning, and -- on the last write before
          // the solve begins -- the end-of-pixels pointer parked in the same
          // four bytes.  Two lifetimes inside one half of one slot.
          uint32_t wt_slot;
      };
      uint8_t _pad6[4];
      uint8_t _pad5[4];
      uint8_t _pad7[28];
  } __frame;
  int32_t best0;
  int32_t best1;
  int32_t best2;
  uint8_t buf_3[2048];
  uint8_t buf_4[2048];
  uint8_t buf_2[2048];
  int32_t x0[4];
  int32_t x1[4];
  int32_t x2[4];
  int32_t x3[4];
  int32_t x4[4];
  int32_t x5[4];
  double d1;
  double d2;
  double d3;
  int32_t acc0[4];
  uint64_t acc1[5];
  double d4;
  int32_t tbl16[16];
  uint8_t tbl64a[64];
  uint8_t tbl64b[64];
  int64_t q2;
  double d5;
  double d6;
  uint8_t *pp;
  uint32_t uu;
  int32_t wa_slot;
  BmfImage *img_a;
  uint32_t data_end;
  ;
  bool keep0, cheaper, keep3, pick0;
  int8_t unread0, unread1, unread2, pos, pos3;
  uint8_t pl_even, pl_odd;
  uint8_t *hist;   // `uint8_t *` beside the `char` scalars above
  double sum22, sum11, dv0, dv1, dv2, sum01, sum02, sum12, sum02_c, inv;
  int16_t g1_lo;
  uint32_t n_quads, next_plane, row, slack;
  int32_t n_planes, data_size, result, pick01, xform, dw, *win_row,
          wt8, wt4, c2, c2w, c1, c1w, c0, bin0, pred, xform_row, win,
          sum, best_sum, i, pos2, best_sum2, sum2, wt4_dn, wt8_dn, wt4_dn_end,
          wt8_dn_end, dv3, wa, wb, wc, nx0, dg0, dn0, ad0, dnx1, g0,
          dn1, nx2, g1, dnx2, g2, pa, pb, bin_lin, quad_r, alpha, bin_lin2,
          r0, r1, r2, wa_pick, pred4, pred4b, *hist2, sum3, best_sum3, npix_c,
          stride_c, left_c, ul_c, g_c, cur_c, n_c, bin_c, wt4_best_dn, npix_d,
          stride_d, left_d, ul_d, g_d, cur_d, n_d, bin_d, wt8_best_dn, wt4_up,
          wt8_up, wt4_up_end, wt8_up_end, npix_a, stride_a, left_a, r_a, ul_a,
          g_a, cur_a, n_a, bin_a, wt4_best, npix_b, stride_b, left_b, r_b,
          ul_b, g_b, cur_b, n_b, bin_b, wt8_best;
  uint32_t pair, cost0, cost1, best_cost, best, cost_flat, i4, quad, cost_lin, best4, cost_c0, cost_c1,
           cost_c2, cost_c, cost_d, cost_a, cost_b;
  uint8_t *px, *end_px, *q_c, *r_c, *p_c, *q_d, *r_d, *p_d, *q_a, *p_a, *q_b,
          *p_b;
  n_planes = plane_count;
  data_size = img->data_size;
  img_a = (BmfImage *)((uint8_t *)img);
  alphabet_reduced = 1;
  __frame.row_stride = ((uint16_t)img->stride);
  data_end = (uintptr_t)&img->pixels[data_size];
  memset(__frame.buf,0,0x8000);
  result = 192;
  do
  {
    bmf_zero16(((uint8_t *)&acc1[4] + result));
    bmf_zero16(((uint8_t *)&acc1[2] + result));
    bmf_zero16(((uint8_t *)acc1 + result));
    bmf_zero16(((uint8_t *)acc0 + result));
    result -= 64;
  }
  while ( result );
  if ( n_planes > 0 )
  {
    if ( n_planes / 2 )
    {
      pair = 0;
      do
      {
        pl_even = 2 * pair;
        row = 2 * pair;   // a record index; it was 32 * pair, two records' worth
        plane_desc[row + 1].src_plane = 2 * pair;
        pl_odd = 2 * pair++ + 1;
        plane_desc[row + 1].predictor = pl_even;
        plane_desc[row + 2].src_plane = pl_odd;
        plane_desc[row + 2].predictor = pl_odd;
      }
      while ( pair < (uint32_t)(n_planes / 2) );
      next_plane = 2 * pair + 1;
    }
    else
    {
      next_plane = 1;
    }
    result = next_plane - 1;
    if ( (uint32_t)n_planes > (next_plane - 1) )
    {
      plane_desc[next_plane].src_plane = result;
      plane_desc[next_plane].predictor = result;
    }
    if ( n_planes >= 3 )
    {
      cost0 = __cost_candidate((uint8_t *)img_a, 0, (uint8_t *)tbl16, unread0, best0, best1, best2, (uint32_t *)&acc0[2]);
      cost1 = __cost_candidate((uint8_t *)img_a, 1, tbl64a, unread1, best0, best1, best2, (uint32_t *)&acc0[2]);
      keep0 = cost1 >= cost0;
      if ( cost1 >= cost0 )
        cost1 = cost0;
      pick01 = !keep0;
      x3[2] = pick01;
      cheaper = (uint32_t)(__cost_candidate((uint8_t *)img_a, 2, tbl64b, unread2, best0, best1, best2, (uint32_t *)&acc0[2])) < cost1;
      xform = pick01;
      if ( cheaper )
        xform = 2;
      x3[2] = xform;
      dw = 16;
      win_row = &tbl16[16 * xform];
      do
      {
        *(uint64_t *)(bmf_plane_desc(dw * 4 - 8)) = *(uint64_t *)&win_row[dw - 2];
        *(uint64_t *)(bmf_plane_desc(dw * 4 - 16)) = *(uint64_t *)&win_row[dw - 4];
        *(uint64_t *)(bmf_plane_desc(dw * 4 - 24)) = *(uint64_t *)&win_row[dw - 6];
        *(uint64_t *)(bmf_plane_desc(dw * 4 - 32)) = *(uint64_t *)&win_row[dw - 8];
        dw -= 8;
      }
      while ( dw * 4 );
      best_cost = acc0[4 * x3[2] + 2];
      x3[1] = 16 * x3[2];
      __frame.plane_a = plane_desc[1].src_plane - x3[2];
      wt8 = plane_desc[x3[2] + 1].w8;
      __frame.plane_b = plane_desc[2].src_plane - x3[2];
      wt4 = plane_desc[x3[2] + 1].w4;
      // always taken: -S is on.  (The block is kept braced -- LABEL_19 below
      // is jumped to from inside it.)
      {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        __frame.nplanes_s = n_planes;
        x0[1] = 4;            // (opt_search_quality + 5) / 3, and -Q is 9
        __frame.s1 = wt4 - 1;
        wt4_dn = wt4 - 1;
        __frame.s6 = wt8 - 1;
        wt8_dn = wt8 - 1;
        __frame.s0 = wt4 - x0[1];
        wt4_dn_end = __frame.s0;
        *(uint32_t *)__frame.buf_1 = wt8 - x0[1];
        x0[0] = (int32_t)&((uint8_t *)img_a)[x3[2]];
        wt8_dn_end = wt8 - x0[1];
        while ( 1 )
        {
          if ( wt4_dn <= wt4_dn_end )
          {
            if ( wt8_dn <= wt8_dn_end )
            {
              x1[0] = x5[3] + 1;
              wt4_up = x5[3] + 1;
              x3[3] = __frame.wt_slot + 1;
              wt8_up = __frame.wt_slot + 1;
              x0[3] = x5[3] + x0[1];
              wt4_up_end = x5[3] + x0[1];
              x0[2] = __frame.wt_slot + x0[1];
              wt8_up_end = __frame.wt_slot + x0[1];
              // The two weights step outward together and each stops at its
              // own limit, so on any pass one of the two may be finished while
              // the other is not.  `LABEL_109` was that case written as a jump
              // past the `wt4` pass, and the inner `if ( wt4_up >= wt4_up_end )`
              // guarding it is trivially true -- it is already inside that
              // test.  Guarding each pass with its own limit says the same.
              while ( 1 )
              {
                if ( wt4_up >= wt4_up_end && wt8_up >= wt8_up_end )
                {
                  wt8 = __frame.wt_slot;
                  wt4 = x5[3];
                  n_planes = __frame.nplanes_s;
                  goto LABEL_19;
                }
                if ( wt4_up < wt4_up_end && wt4_up < 192 )
                {
                  x0[2] = wt8_up_end;
                  memset(__frame.buf_1,0,2048);
                  npix_a = img_a->width * (img_a->height - 1);
                  stride_a = (uint16_t)img_a->stride;
                  x3[3] = wt8_up;
                  x1[0] = wt4_up;
                  x1[1] = npix_a - 1;
                  x3[0] = best_cost;
                  x2[1] = -stride_a;
                  left_a = -plane_count;
                  x2[0] = -stride_a - plane_count;
                  x1[3] = x0[0] - x2[0] + 16;
                  x1[2] = __frame.q1 + x0[0] - x2[0] + 16;
                  q_a = (uint8_t *)x1[2];
                  r_a = __frame.plane_b + x0[0] - x2[0] + 16;
                  p_a = (uint8_t *)x1[3];
                  do
                  {
                    ul_a = p_a[x2[0]];
                    *(int64_t *)&x1[2] = __PAIR64__((uint32_t)p_a, q_a);
                    x2[2] = r_a;
                    g_a = ul_a + *p_a - p_a[x2[1]] - p_a[left_a];
                    cur_a = *q_a;
                    x2[3] = g_a;
                    r_a = x2[2] - left_a;
                    n_a = x1[1];
                    bin_a = (((int16_t *)x2)[6]
                          - (uint16_t)((x1[0]
                                              * (q_a[x2[0]] + cur_a - q_a[x2[1]] - q_a[left_a])
                                              + __frame.wt_slot
                                              * (*(uint8_t *)(x2[2] + x2[0])
                                               + *(uint8_t *)x2[2]
                                               - *(uint8_t *)(x2[2] + x2[1])
                                               - (uint32_t)*(uint8_t *)(x2[2] + left_a))
                                              + 40) >> 7)
                          - 256)
                         & 0x1FF;
                    ++*(uint32_t *)&__frame.buf_1[4 * bin_a];
                    q_a -= left_a;
                    p_a = (uint8_t *)(x1[3] - left_a);
                    x1[1] = n_a - 1;
                  }
                  while ( n_a != 1 );
                  wt8_up = x3[3];
                  wt4_up = x1[0];
                  best_cost = x3[0];
                  cost_a = __estimate_cost((uint8_t *)__frame.buf_1, 512);
                  wt8_up_end = x0[2];
                  wt4_best = x5[3];
                  if ( cost_a < best_cost )
                  {
                    best_cost = cost_a;
                    wt4_best = wt4_up;
                  }
                  x5[3] = wt4_best;
                  wt4_up_end = x0[1] + wt4_best;
                }
                if ( wt8_up < wt8_up_end && wt8_up < 192 )
                {
                  {
                    x0[3] = wt4_up_end;
                    memset(buf_2,0,2048);
                    npix_b = img_a->width * (img_a->height - 1);
                    stride_b = (uint16_t)img_a->stride;
                    x3[3] = wt8_up;
                    x1[0] = wt4_up;
                    x4[0] = npix_b - 1;
                    x3[0] = best_cost;
                    x5[0] = -stride_b;
                    left_b = -plane_count;
                    x4[3] = -stride_b - plane_count;
                    x4[2] = x0[0] - x4[3] + 16;
                    q_b = (uint8_t *)(__frame.q1 + x0[0] - x4[3] + 16);
                    x4[1] = (int32_t)q_b;
                    r_b = __frame.plane_b + x0[0] - x4[3] + 16;
                    p_b = (uint8_t *)x4[2];
                    do
                    {
                      ul_b = p_b[x4[3]];
                      *(int64_t *)((uint8_t *)x4 + 4) = __PAIR64__((uint32_t)p_b, q_b);
                      x5[1] = r_b;
                      g_b = ul_b + *p_b - p_b[x5[0]] - p_b[left_b];
                      cur_b = *q_b;
                      x5[2] = g_b;
                      r_b = x5[1] - left_b;
                      n_b = x4[0];
                      bin_b = (((int16_t *)x5)[4]
                            - (uint16_t)((x5[3]
                                                * (q_b[x4[3]] + cur_b - q_b[x5[0]] - q_b[left_b])
                                                + x3[3]
                                                * (*(uint8_t *)(x5[1] + x4[3])
                                                 + *(uint8_t *)x5[1]
                                                 - *(uint8_t *)(x5[1] + x5[0])
                                                 - (uint32_t)*(uint8_t *)(x5[1] + left_b))
                                                + 40) >> 7)
                            - 256)
                           & 0x1FF;
                      ++*(uint32_t *)&buf_2[4 * bin_b];
                      q_b -= left_b;
                      p_b = (uint8_t *)(x4[2] - left_b);
                      x4[0] = n_b - 1;
                    }
                    while ( n_b != 1 );
                    wt8_up = x3[3];
                    wt4_up = x1[0];
                    best_cost = x3[0];
                    cost_b = __estimate_cost((uint8_t *)buf_2, 512);
                    wt4_up_end = x0[3];
                    wt8_best = __frame.wt_slot;
                    if ( cost_b < best_cost )
                    {
                      best_cost = cost_b;
                      wt8_best = wt8_up;
                    }
                    __frame.wt_slot = wt8_best;
                    wt8_up_end = x0[1] + wt8_best;
                  }
                }
                ++wt4_up;
                ++wt8_up;
              }
            }
          }
          // The same shape as the pass above, walking inward: `LABEL_55` was
          // the case where `wt4` has reached its limit and `wt8` has not.
          
          // The `wt8_dn > wt8_dn_end` guard below is new and is a no-op:
          // reaching here with `wt8` *also* finished would mean the outward
          // search above had run, and that loop has no exit except the one
          // that leaves this whole function's search -- so it cannot fall out
          // of it.  The jump relied on that; the guard makes it visible.
          if ( wt4_dn > wt4_dn_end && wt4_dn >= -64 )
          {
            *(uint32_t *)__frame.buf_1 = wt8_dn_end;
            memset(buf_3,0,2048);
            npix_c = img_a->width * (img_a->height - 1);
            stride_c = (uint16_t)img_a->stride;
            __frame.s6 = wt8_dn;
            __frame.s1 = wt4_dn;
            __frame.s2 = npix_c - 1;
            x3[0] = best_cost;
            __frame.s4 = -stride_c;
            left_c = -plane_count;
            __frame.s3 = -stride_c - plane_count;
            __frame.p1 = (uint8_t *)(x0[0] - __frame.s3 + 16);
            __frame.p0 = (uint8_t *)(__frame.q1 + x0[0] - __frame.s3 + 16);
            q_c = __frame.p0;
            r_c = (uint8_t *)(__frame.plane_b + x0[0] - __frame.s3 + 16);
            p_c = __frame.p1;
            do
            {
              ul_c = p_c[__frame.s3];
              __frame.p0 = q_c;
              __frame.p1 = p_c;
              __frame.p2 = r_c;
              g_c = ul_c + *p_c - p_c[__frame.s4] - p_c[left_c];
              cur_c = *q_c;
              __frame.s5 = g_c;
              r_c = &__frame.p2[-left_c];
              n_c = __frame.s2;
              bin_c = ((uint16_t)__frame.s5
                    - (uint16_t)((__frame.s1 * (q_c[__frame.s3] + cur_c - q_c[__frame.s4] - q_c[left_c])
                                        + __frame.wt_slot * (__frame.p2[__frame.s3] + *__frame.p2 - __frame.p2[__frame.s4] - (uint32_t)__frame.p2[left_c])
                                        + 40) >> 7)
                    - 256)
                   & 0x1FF;
              ++*(uint32_t *)&buf_3[4 * bin_c];
              q_c -= left_c;
              p_c = &__frame.p1[-left_c];
              __frame.s2 = n_c - 1;
            }
            while ( n_c != 1 );
            wt8_dn = __frame.s6;
            wt4_dn = __frame.s1;
            best_cost = x3[0];
            cost_c = __estimate_cost((uint8_t *)buf_3, 512);
            wt8_dn_end = *(uint32_t *)__frame.buf_1;
            wt4_best_dn = x5[3];
            if ( cost_c < best_cost )
            {
              best_cost = cost_c;
              wt4_best_dn = wt4_dn;
            }
            x5[3] = wt4_best_dn;
            wt4_dn_end = wt4_best_dn - x0[1];
          }
          if ( wt8_dn > wt8_dn_end && wt8_dn >= -64 )
          {
            {
              __frame.s0 = wt4_dn_end;
              memset(buf_4,0,2048);
              npix_d = img_a->width * (img_a->height - 1);
              stride_d = (uint16_t)img_a->stride;
              __frame.s6 = wt8_dn;
              __frame.s1 = wt4_dn;
              __frame.s7 = npix_d - 1;
              x3[0] = best_cost;
              __frame.s9 = -stride_d;
              left_d = -plane_count;
              __frame.s8 = -stride_d - plane_count;
              __frame.p4 = (uint8_t *)(x0[0] - __frame.s8 + 16);
              q_d = (uint8_t *)(__frame.q1 + x0[0] - __frame.s8 + 16);
              __frame.p3 = q_d;
              r_d = (uint8_t *)(__frame.plane_b + x0[0] - __frame.s8 + 16);
              p_d = __frame.p4;
              do
              {
                ul_d = p_d[__frame.s8];
                __frame.p3 = q_d;
                __frame.p4 = p_d;
                __frame.p5 = r_d;
                g_d = ul_d + *p_d - p_d[__frame.s9] - p_d[left_d];
                cur_d = *q_d;
                __frame.s10 = g_d;
                r_d = &__frame.p5[-left_d];
                n_d = __frame.s7;
                bin_d = ((uint16_t)__frame.s10
                      - (uint16_t)((x5[3] * (q_d[__frame.s8] + cur_d - q_d[__frame.s9] - q_d[left_d])
                                          + __frame.s6 * (__frame.p5[__frame.s8] + *__frame.p5 - __frame.p5[__frame.s9] - (uint32_t)__frame.p5[left_d])
                                          + 40) >> 7)
                      - 256)
                     & 0x1FF;
                ++*(uint32_t *)&buf_4[4 * bin_d];
                q_d -= left_d;
                p_d = &__frame.p4[-left_d];
                __frame.s7 = n_d - 1;
              }
              while ( n_d != 1 );
              wt8_dn = __frame.s6;
              wt4_dn = __frame.s1;
              best_cost = x3[0];
              cost_d = __estimate_cost((uint8_t *)buf_4, 512);
              wt4_dn_end = __frame.s0;
              wt8_best_dn = __frame.wt_slot;
              if ( cost_d < best_cost )
              {
                best_cost = cost_d;
                wt8_best_dn = wt8_dn;
              }
              __frame.wt_slot = wt8_best_dn;
              wt8_dn_end = wt8_best_dn - x0[1];
            }
          }
          --wt4_dn;
          --wt8_dn;
        }
      }
LABEL_19:
      px = &((uint8_t *)img_a)[__frame.q0 + 16 + n_planes + x3[2]];
      if ( (uint32_t)px < data_end )
      {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        x3[0] = best_cost;
        __frame.nplanes_s = n_planes;
        do
        {
          c2 = px[__frame.plane_b];
          c2w = c2 * __frame.wt_slot;
          c1 = px[__frame.q1];
          c1w = c1 * x5[3];
          ++__frame.hist_c[c2 - c1 + 1280];
          c0 = *px + 512;
          px += __frame.nplanes_s;
          bin0 = ((uint16_t)c0 - (uint16_t)((uint32_t)(c2w + c1w + 40) >> 7)) & 0x3FF;
          ++*(uint32_t *)&__frame.buf[4 * bin0];
          ++__frame.hist_a[c0 - c1];
          ++__frame.hist_b[c0 - c2];
          ++__frame.hist_c[(c0 - ((uint32_t)(((c1 + c2) << 6) + 40) >> 7))];
        }
        while ( (uint32_t)px < data_end );
        wt8 = __frame.wt_slot;
        wt4 = x5[3];
        best_cost = x3[0];
        n_planes = __frame.nplanes_s;
      }
      slack = best_cost >> 7;
      if ( best_cost >> 7 >= 0x4000 )
        slack = 0x4000;
      best = slack + best_cost;
      cost_flat = *(int32_t *)((uint8_t *)&acc0[3] + x3[1]);
      cheaper = cost_flat < best;
      if ( cost_flat < best )
      {
        best = *(int32_t *)((uint8_t *)&acc0[3] + x3[1]);
        wt4 = 128;
        wt8 = 0;
      }
      pred = cheaper;
      if ( *(uint32_t *)((uint8_t *)acc1 + x3[1]) < best )
      {
        best = *(uint32_t *)((uint8_t *)acc1 + x3[1]);
        pred = 2;
        wt4 = 0;
        wt8 = 128;
      }
      keep3 = best <= *(uint32_t *)((uint8_t *)acc1 + x3[1] + 4);
      if ( best > *(uint32_t *)((uint8_t *)acc1 + x3[1] + 4) )
        pred = 3;
      xform_row = x3[2];   // a record index; it was the byte offset 16 * it
      if ( !keep3 )
      {
        wt4 = 64;
        wt8 = 64;
      }
      plane_desc[x3[2] + 1].w4 = wt4;
      plane_desc[xform_row + 1].w8 = wt8;
      hist = &__frame.buf[4096 * pred];
      win = (uint8_t)(uintptr_t)hist & 0xF;
      // The first 256-wide window over these 1024 counters.  Where it starts
      // is the pointer's low four bits, which the frame's alignas(16) makes
      // zero; sixteen counters an iteration is all the vectors were doing,
      // and integer addition does not care which lane a term landed in.
      sum = 0;
      for ( i = 0; i < 256; ++i )
        sum += *(int32_t *)&hist[4 * (win + i)];
      win += 256;
      best_sum = sum;
      pos = -1;
      if ( win < 1024 )
      {
        __frame.nplanes_s = n_planes;
        do
        {
          sum = *(uint32_t *)&hist[4 * win] + sum - *(uint32_t *)&hist[4 * win - 1024];
          if ( sum >= best_sum )
          {
            pos = win;
            best_sum = sum;
          }
          ++win;
        }
        while ( win < 1024 );
        n_planes = __frame.nplanes_s;
      }
      plane_desc[x3[2] + 1].b3 = pos + 1;
      // Same window, over the second table.  `i` is left at 256 for the slide
      // that follows.
      best_sum2 = 0;
      for ( i = 0; i < 0x100; ++i )
        best_sum2 += __frame.hist_c[i + 1024];
      pos2 = 255;
      for ( sum2 = best_sum2; i < 512; ++i )
      {
        sum2 = __frame.hist_c[i + 1024] + sum2 - __frame.hist_c[i + 768];
        if ( sum2 >= best_sum2 )
        {
          pos2 = i;
          best_sum2 = sum2;
        }
      }
      result = pos2 + 1;
      // `m128_i32[1]` is `16 * m128_i32[2]`, a byte offset, so the record index
      // is `[2]`.  This was `__byte_44339F[16 * HIDWORD(q0) + x3[1]]`
      // and the fold that made it a record access divided the first term by 16
      // and not the second.  The chosen transform is 0 on all fifteen reference
      // images -- both spellings agree there, which is why the gate stayed green
      // -- but it is 1 or 2 for an image that picks another one.
      plane_desc[__frame.plane_b + x3[2] + 1].b3 = result;
      if ( n_planes >= 4 )
      {
        __builtin_memset(x0, 0, 16);
        __builtin_memset(x1, 0, 16);
        __builtin_memset(x2, 0, 16);
        __builtin_memset(x3, 0, 16);
        __builtin_memset(x4, 0, 16);
        __builtin_memset(x5, 0, 16);
        memset(__frame.buf,0,0x8000);
        pp = &((uint8_t *)img_a)[__frame.q0];
        n_quads = (int32_t)(data_end - 17 - (uint32_t)&((uint8_t *)img_a)[__frame.q0]) / 4;
        end_px = &((uint8_t *)img_a)[__frame.q0 + 20];
        if ( data_end <= (uint32_t)end_px )
        {
          sum01 = *(double *)&x0[2];
          sum02 = *(double *)x1;
          sum12 = *(double *)&x2[2];
          __frame.q1 = *(int64_t *)&x0[2];
          __frame.q0 = *(int64_t *)&x2[2];
          d4 = *(double *)x0;
          q2 = *(int64_t *)&x4[2];
          sum02_c = *(double *)x1;
        }
        else
        {
          sum22 = *(double *)x4;
          sum11 = *(double *)x2;
          q2 = *(int64_t *)&x4[2];
          d4 = *(double *)x0;
          d5 = *(double *)&x0[2];
          d2 = *(double *)x1;
          d6 = *(double *)&x2[2];
          d3 = *(double *)x5;
          d1 = *(double *)&x5[2];
          __frame.wt_slot = (uintptr_t)&((uint8_t *)img_a)[__frame.q0 + 20];
          i4 = 0;
          uu = (int32_t)(data_end - 17 - (uint32_t)&((uint8_t *)img_a)[__frame.q0]) / 4;
          do
          {
            dv0 = (double)(((uint8_t *)img_a)[4 * i4 + 16] + pp[4 * i4 + 20] - (((uint8_t *)img_a)[4 * i4 + 20] + pp[4 * i4 + 16]));
            dv1 = (double)(((uint8_t *)img_a)[4 * i4 + 17] + pp[4 * i4 + 21] - (((uint8_t *)img_a)[4 * i4 + 21] + pp[4 * i4 + 17]));
            dv2 = (double)(((uint8_t *)img_a)[4 * i4 + 18] + pp[4 * i4 + 22] - (((uint8_t *)img_a)[4 * i4 + 22] + pp[4 * i4 + 18]));
            dv3 = ((uint8_t *)img_a)[4 * i4 + 19] + pp[4 * i4 + 23] - (((uint8_t *)img_a)[4 * i4 + 23] + pp[4 * i4 + 19]);
            d4 = d4 + dv0 * dv0;
            ++i4;
            d5 = d5 + dv0 * dv1;
            d2 = d2 + dv0 * dv2;
            sum11 = sum11 + dv1 * dv1;
            d6 = d6 + dv1 * dv2;
            sum22 = sum22 + dv2 * dv2;
            *(double *)&q2 = *(double *)&q2 + dv0 * (double)dv3;
            d3 = d3 + dv1 * (double)dv3;
            d1 = d1 + dv2 * (double)dv3;
          }
          while ( i4 < uu );
          sum01 = d5;
          sum02 = d2;
          sum12 = d6;
          end_px = (uint8_t *)__frame.wt_slot;
          n_quads = uu;
          *(double *)&x5[2] = d1;
          *(double *)x5 = d3;
          *(int64_t *)&x4[2] = q2;
          *(double *)x4 = sum22;
          *(double *)&x2[2] = d6;
          *(double *)x2 = sum11;
          *(double *)x1 = d2;
          *(double *)&x0[2] = d5;
          *(double *)x0 = d4;
          sum02_c = d2;
          *(double *)&__frame.q0 = d6;
          *(double *)&__frame.q1 = d5;
        }
        d5 = sum01;
        d6 = sum12;
        __frame.d0 = sum02_c;
        *(double *)&x1[2] = sum01;
        *(double *)x3 = sum02;
        *(double *)&x3[2] = sum12;
        d1 = d4 * *(double *)x4 - sum02_c * sum02;
        d2 = 0.0 - d4 * sum12 + sum02 * *(double *)&__frame.q1;
        d3 = sum02_c * sum12 - *(double *)&__frame.q1 * *(double *)x4;
        inv = 128.0 / (*(double *)x2 * d1 + *(double *)&__frame.q0 * d2 + sum01 * d3 + 0.1);
        *(double *)acc0 = inv;
        wa = (int32_t)(((sum02 * *(double *)&__frame.q0 - sum01 * *(double *)x4) * *(double *)x5
                    + (0.0 - sum02 * *(double *)x2 + sum01 * d6) * *(double *)&x5[2]
                    + (*(double *)x4 * *(double *)x2 - d6 * *(double *)&__frame.q0)
                    * *(double *)&q2)
                   * inv);
        if ( wa >= 191 )
          wa = 191;
        if ( wa < -64 )
          wa = -64;
        wa_slot = wa;
        wb = (int32_t)((d1 * *(double *)x5 + d2 * *(double *)&x5[2]
                                                         + d3 * *(double *)&q2)
                     * inv);
        if ( wb >= 191 )
          wb = 191;
        if ( wb < -64 )
          wb = -64;
        wc = (int32_t)(*(double *)acc0
                     * ((d4 * *(double *)&x5[2] - __frame.d0 * *(double *)&q2) * *(double *)x2
                      + (0.0 - d4 * *(double *)x5 + *(double *)&q2 * *(double *)&__frame.q1)
                      * *(double *)&__frame.q0
                      + (__frame.d0 * *(double *)x5 - *(double *)&__frame.q1 * *(double *)&x5[2]) * d5));
        if ( wc >= 191 )
          wc = 191;
        if ( wc < -64 )
          wc = -64;
        if ( (uint32_t)end_px < data_end )
        {
          *(int64_t *)x0 = __PAIR64__(wc, wb);
          uu = n_quads;
          quad = 0;
          do
          {
            nx0 = ((uint8_t *)img_a)[4 * quad + 20];
            dg0 = ((uint8_t *)img_a)[4 * quad + 16] + pp[4 * quad + 20];
            dn0 = pp[4 * quad + 16];
            x0[2] = quad;
            ad0 = nx0 + dn0;
            dnx1 = pp[4 * quad + 21];
            g0 = dg0 - ad0;
            dn1 = pp[4 * quad + 17];
            x0[3] = g0;
            nx2 = ((uint8_t *)img_a)[4 * quad + 22];
            g1 = ((uint8_t *)img_a)[4 * quad + 17] + dnx1 - (((uint8_t *)img_a)[4 * quad + 21] + dn1);
            dnx2 = pp[4 * quad + 22];
            x1[0] = g1;
            g2 = ((uint8_t *)img_a)[4 * quad + 18] + dnx2 - (nx2 + pp[4 * quad + 18]);
            LOWORD(g1) = ((uint8_t *)img_a)[4 * quad + 19] + pp[4 * quad + 23] - pp[4 * quad + 19] - ((uint8_t *)img_a)[4 * quad + 23];
            g1_lo = ((int16_t *)x1)[0];
            LOWORD(g1) = g1 - 512;
            pa = x1[0] * x0[0] + x0[3] * wa_slot;
            pb = g2 * x0[1];
            ++__frame.hist_a[((uint16_t)g1 - ((int16_t *)x0)[6]) & 0x3FF];
            bin_lin = ((uint16_t)g1 - (uint16_t)((uint32_t)(pa + pb + 63) >> 7)) & 0x3FF;
            ++*(uint32_t *)&__frame.buf[4 * bin_lin];
            ++__frame.hist_b[((uint16_t)g1 - g1_lo) & 0x3FF];
            LOWORD(g1) = g1 - g2;
            quad_r = x0[2];
            ++__frame.hist_c[g1 & 0x3FF];
            alpha = pp[4 * quad_r + 23] + 256;
            bin_lin2 = ((uint16_t)alpha
                 - (uint16_t)((x0[0] * pp[4 * quad_r + 21]
                                     + wa_slot * pp[4 * quad_r + 20]
                                     + x0[1] * (uint32_t)pp[4 * quad_r + 22]
                                     + 63) >> 7)
                 + 256)
                & 0x3FF;
            ++__frame.hist_c[bin_lin2 + 1024];
            r0 = alpha - pp[4 * quad_r + 20];
            ++__frame.hist_c[r0 + 2048];
            r1 = alpha - pp[4 * quad_r + 21];
            ++__frame.hist_c[r1 + 3072];
            r2 = alpha - pp[4 * quad_r + 22];
            ++__frame.hist_c[r2 + 4096];
            quad = quad_r + 1;
          }
          while ( quad < uu );
          wc = x0[1];
          wb = x0[0];
        }
        cost_lin = __estimate_cost((uint8_t *)__frame.buf, 1024);
        best4 = (cost_lin >> 7) + cost_lin;
        cost_c0 = __estimate_cost((uint8_t *)__frame.hist_a, 1024);
        pick0 = cost_c0 < best4;
        if ( cost_c0 < best4 )
          best4 = cost_c0;
        wa_pick = wa_slot;
        if ( pick0 )
        {
          wa_pick = 128;
          wc = 0;
          wb = 0;
        }
        wa_slot = wa_pick;
        x0[0] = pick0;
        cost_c1 = __estimate_cost((uint8_t *)__frame.hist_b, 1024);
        pred4 = x0[0];
        if ( cost_c1 < best4 )
        {
          best4 = cost_c1;
          pred4 = 2;
          wb = 128;
          wc = 0;
          wa_slot = 0;
        }
        x0[0] = pred4;
        cost_c2 = __estimate_cost((uint8_t *)__frame.hist_c, 1024);
        pred4b = x0[0];
        if ( cost_c2 < best4 )
        {
          pred4b = 3;
          wc = 128;
          wb = 0;
          wa_slot = 0;
        }
        plane_desc[4].w4 = wa_slot;
        plane_desc[4].w8 = wb;
        plane_desc[4].w12 = wc;
        plane_desc[4].src_plane = 3;
        plane_desc[4].predictor = 3;
        hist2 = &__frame.hist_c[1024 * pred4b + 1024];
        result = (uint8_t)(uintptr_t)hist2 & 0xF;
        // And the same again, over the third.
        sum3 = 0;
        for ( i = 0; i < 256; ++i )
          sum3 += hist2[result + i];
        result += 256;
        best_sum3 = sum3;
        for ( pos3 = -1; result < 1024; ++result )
        {
          sum3 = hist2[result] + sum3 - hist2[result - 256];
          if ( sum3 >= best_sum3 )
          {
            pos3 = result;
            best_sum3 = sum3;
          }
        }
        plane_desc[4].b3 = pos3 + 1;
      }
    }
  }
  return result;
}

int32_t *__read_bmp(char *path)
{
  // This one is a layout, not a bag of locals: lifting its members to
  // ordinary locals makes DLRAW abort while compressing.  Re-checked against the
  // file as it is now, by `tools/liftframe.py` -- `frame-sweep.sh`, which
  // the note here used to cite, lifts aliases and has none left to lift.
  struct alignas(16) ReadBmpFrame {   // 128 bytes, one stack frame
      uint32_t Size_4;
      int32_t pal_bytes;
      BmfImage *img_f;
      int32_t row_ofs;
      void *pal_buf;
      uint8_t _pad0[4];
      uint8_t *row;
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
  int32_t y4, y8;
  uint8_t pix;
  int32_t run4;
  // These shared `__frame.pal_bytes` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `__frame.row_ofs` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  ;
  uintptr_t row_at;   // were int32_t: addresses, masked and tagged
  uint8_t *pal, *pal2, *pal3;   // were int32_t: these hold addresses
  FILE *fp;
  uint8_t lo;
  BmfImage *img;
  uint32_t row_pad, byte;
  uint8_t cur, lo16;
  uint8_t *row4, *row3, *row6, *pal_at, *row5;   // `uint8_t *` beside the `char` scalars above
  int32_t pal_n, i, run, run_val, hi_nibble, left4, left4b, y, dx, dxy, step;
  uint32_t stride_pad, pair, hi, stride, got, left;
  // These two freads land in the frame, and each writes across several of the
  // slots Hex-Rays split it into -- which is why the fields do not look like
  // fields.  `bmp_info_hdr` is declared `uint32_t[2]` and the read is 40 bytes:
  
  //   frame +36  bmp_info_hdr[0]  biSize          checked == 40 below
  //         +40  bmp_info_hdr[1]  biWidth
  //         +44  bmp_height       biHeight
  //         +48  bmp_planes       biPlanes        checked == 1 below
  //         +50  bmp_bits         biBitCount
  //         +52  bmp_compression  biCompression   0 none, 1 RLE8, 2 RLE4
  //         +56  _pad2[12]        biSizeImage and the two pixels-per-metre
  //         +68  bmp_clr_used     biClrUsed
  //         +72  _pad3[4]         biClrImportant
  
  // and the 14-byte read covers `bmp_file_hdr[0..4]` and the slot after it,
  // whose four bytes are `bfOffBits` -- `bmp_off_bits`, which the fseek below
  // uses.  Every one of these names is confirmed by what the code does with it,
  // not by the offset alone: bmp_compression is tested against 1 and 2,
  // bmp_clr_used overrides `1 << bmp_bits` as the palette size, bmp_height goes
  // to alloc_image as the height.  The struct that says the same thing in one
  // piece is `BmpHeader`, above write_bmp, which builds this on the way out.
  fp = fopen(path, "rb");
  if ( !fp
    || fread(__frame.bmp_file_hdr, 0xEu, 1u, fp) != 1
    || __frame.bmp_file_hdr[0] != 0x4D42 /* 'BM' */
    || fread(__frame.bmp_info_hdr, 0x28u, 1u, fp) != 1
    || __frame.bmp_info_hdr[0] != 40
    || __frame.bmp_planes != 1 )
  {
    return nullptr;
  }
  img = (BmfImage *)(__alloc_image(__frame.bmp_info_hdr[1], __frame.bmp_height, __frame.bmp_bits, __frame.bmp_bits <= 8u, 1));
  stride_pad = (img->stride + 3) & 0xFFFFFFFC;
  if ( __frame.bmp_bits <= 8u )
  {
    pal_n = 1 << (__frame.bmp_bits & 31);
    if ( __frame.bmp_clr_used )
      pal_n = __frame.bmp_clr_used;
    if ( pal_n > 0 )
    {
      __frame.Size_4 = (img->stride + 3) & 0xFFFFFFFC;
      __frame.pal_bytes = pal_n;
      for ( i = 0; i < __frame.pal_bytes; ++i )
      {
        fread(__frame.bmp_bgra, 4u, 1u, fp);
        if ( (img->depth & 0x80) != 0 )
          pal = (uint8_t *)(uintptr_t)img + img->data_size + 16;
        else
          pal = 0;
        *(pal + 3 * i + 2) = __frame.bmp_bgra[2];
        if ( (img->depth & 0x80) != 0 )
          pal2 = (uint8_t *)(uintptr_t)img + img->data_size + 16;
        else
          pal2 = 0;
        *(pal2 + 3 * i + 1) = __frame.bmp_bgra[1];
        if ( (img->depth & 0x80) != 0 )
          pal3 = (uint8_t *)(uintptr_t)img + img->data_size + 16;
        else
          pal3 = 0;
        *(pal3 + 3 * i) = __frame.bmp_bgra[0];
      }
      stride_pad = __frame.Size_4;
    }
  }
  __frame.pal_buf = bmf_new(stride_pad);
  __frame.row = (uint8_t *)img + img->data_size - img->stride + 16;
  fseek(fp, (*(int32_t *)((uint8_t *)__frame.bmp_off_bits)), 0);
  if ( __frame.bmp_compression )
  {
    if ( __frame.bmp_compression == 1 )
    {
      memset(img->pixels,0,img->data_size);
      row_at = (int32_t)__frame.row;
      __frame.img_f = img;
      y4 = img->height - 1;
      while ( 1 )
      {
        __frame.row_ofs = row_at;
        if ( ferror(fp) )
          return nullptr;
        run = fgetc(fp);
        run_val = fgetc(fp);
        if ( run )
        {
          // An RLE8 run: `run` copies of one byte.  What was here instead was
          // a scalar head to reach sixteen-byte alignment, sixteen bytes an
          // iteration, and a scalar tail -- memset with the alignment written
          // out, and a separate short-run path for anything under 16 + the
          // head.
          
          // The write is still not bounded by the pixel buffer: a stream that
          // ends mid-run keeps writing.  That is a real defect, recorded
          // rather than repaired (REFACTORING.md §6), and it is why the
          // malformed-input check truncates an uncompressed BMP instead.
          __builtin_memset((void *)__frame.row_ofs, run_val, run);
          row_at = __frame.row_ofs + run;
        }
        else if ( run_val )
        {
          if ( run_val == 1 )
            goto LABEL_61;
          if ( run_val == 2 )
          {
            dx = fgetc(fp);
            row_at = dx + row_at - fgetc(fp) * *((uint16_t *)__frame.img_f + 2);
          }
          else
          {
            fread(__frame.pal_buf, (run_val + 1) & 0xFFFFFFFE, 1u, fp);
            memcpy((uint8_t *)row_at,(uint8_t *)__frame.pal_buf,run_val);
            row_at += run_val;
          }
        }
        else
        {
          if ( --y4 < 0 )
            goto LABEL_61;
          row_at = (int32_t)__frame.img_f + y4 * *((uint16_t *)__frame.img_f + 2) + 16;
        }
      }
    }
    if ( __frame.bmp_compression != 2 )
      return nullptr;
    memset(img->pixels,0,img->data_size);
    __frame.img_f = img;
    hi_nibble = 1;
    y8 = img->height - 1;
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
LABEL_44:
          if ( ferror(fp) )
            return nullptr;
          run4 = fgetc(fp);
          byte = fgetc(fp);
          pair = byte;
          if ( !run4 )
            break;
          lo = byte & 0xF;
          if ( hi_nibble )
          {
            left4b = run4;
            row3 = __frame.row;
            while ( left4b != 1 )
            {
              *row3++ = pair;
              left4b -= 2;
              if ( !left4b )
              {
                __frame.row = row3;
                hi_nibble = 1;
                goto LABEL_44;
              }
            }
            __frame.row = row3;
            *row3 = pair & 0xF0;
            hi_nibble = 0;
          }
          else
          {
            left4 = run4;
            row4 = __frame.row;
            cur = *__frame.row;
            hi = pair >> 4;
            lo16 = 16 * lo;
            while ( 1 )
            {
              *row4++ = hi | cur;
              if ( left4 == 1 )
                break;
              cur = lo16;
              left4 -= 2;
              if ( !left4 )
              {
                __frame.row = row4;
                *row4 = lo16;
                hi_nibble = 0;
                goto LABEL_44;
              }
            }
            __frame.row = row4;
            hi_nibble = 1;
          }
        }
        if ( byte )
          break;
        if ( --y8 < 0 )
          goto LABEL_61;
        __frame.row = (uint8_t *)__frame.img_f + y8 * *((uint16_t *)__frame.img_f + 2) + 16;
      }
      if ( byte == 1 )
        goto LABEL_61;
      if ( byte != 2 )
        break;
      dxy = fgetc(fp);
      step = (dxy >> 1) - fgetc(fp) * *((uint16_t *)__frame.img_f + 2);
      if ( (dxy & 1) == 1 )
      {
        if ( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      __frame.row += step;
    }
    fread(__frame.pal_buf, (((byte + 1) >> 1) + 1) & 0xFFFFFFFE, 1u, fp);
    pal_at = (uint8_t *)__frame.pal_buf;
    row5 = __frame.row;
    while ( 1 )
    {
      pix = *pal_at;
      if ( hi_nibble )
      {
        left = pair - 1;
        if ( !left )
        {
          __frame.row = row5;
          *row5 = *pal_at & 0xF0;
          hi_nibble = 0;
          goto LABEL_44;
        }
        *row5++ = pix;
        hi_nibble = 1;
      }
      else
      {
        *row5++ |= (uint8_t)*pal_at >> 4;
        left = pair - 1;
        if ( !left )
        {
          __frame.row = row5;
          hi_nibble = 1;
          goto LABEL_44;
        }
        *row5 = 16 * (pix & 0xF);
        hi_nibble = 0;
      }
      ++pal_at;
      pair = left - 1;
      if ( !pair )
      {
        __frame.row = row5;
        goto LABEL_44;
      }
    }
  }
  stride = img->stride;
  row_pad = stride_pad - stride;
  if ( __frame.bmp_height - 1 >= 0 )
  {
    y = __frame.bmp_height - 1;
    __frame.img_f = img;
    row6 = __frame.row;
    while ( 1 )
    {
      got = fread(row6, 1u, stride, fp);
      stride = *((uint16_t *)__frame.img_f + 2);
      if ( got != stride )
        return nullptr;
      if ( row_pad )
      {
        fseek(fp, row_pad, 1);
        stride = *((uint16_t *)__frame.img_f + 2);
      }
      row6 -= stride;
      if ( --y < 0 )
        goto LABEL_61;
    }
  }
  // Four jumps arrive here and all four mean the same thing: the image is
  // complete, stop decoding and hand it back.  Three of them are in the RLE8
  // and RLE4 decoders, which are separate loops, and the label used to sit
  // *inside* the uncompressed decoder's loop -- so reaching it was a jump into
  // a block followed by a `break` out of a loop the jumper was never in.
  
  // What made that readable as an exit at all was `img = __frame.img_f`, which
  // is a reload and not an assignment: `img` is written once, at the
  // allocation, and `__frame.img_f` is written three times and always from
  // `img`.  With the reload gone the label is the function's success exit and
  // says so by where it is.
LABEL_61:
  fclose(fp);
  free(__frame.pal_buf);
  return (int32_t *)img;
}

int32_t __decode_symbol_list(SymList *syms)
{
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "DLRAW segfaults while decompressing"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
  struct alignas(16) DecodeSymbolListFrame {   // 32824 bytes, one stack frame
      union {
          SymEntry *list[8192];   // the symbol list: 8 named slots and 32736 bytes of tail, one array
          struct {   // the locals MSVC spilled into these bytes
            // Slot 0 holds an entry pointer while the list is being built and
            // a loop count once the rescale pass starts; the two spellings are
            // the two roles, which is what the original casts were hiding.
            uint32_t list0;
            SymEntry *list1;
            uint32_t list2;
            int32_t list3;
            int32_t list4;
            uint32_t list5;
            SymEntry *list6;
            int32_t list7;
            SymEntry *list_tail[8184];
          };
      };
      uint8_t _pad1[32];
  } __frame;
  int32_t tot;
  int32_t tot0;
  SymEntry *first;
  SymList *owner;
  uint32_t gen;
  ;
  // Every cursor here walks `syms->ent`'s three-byte entries: the symbol was
  // `*(uint16_t *)p` and the count `p[2]`, and the steps were +3 and -3.
  SymEntry *ent, *e, *p, *q, *head, *q2, *r, *cur, *prev, *up, *back;
  SymEntry **w, **rd, **rd2;
  int8_t gen_b;
  uint32_t cum_lo, sym_cum, sym_high, cum_hi;
  int32_t tot_1, tot_all, target;
  uint8_t c_a, c_b;
  uint16_t s_a, s_b;
  // The cumulative count the range coder takes, which it takes unsigned.
  int32_t live, cum, i, c, top, half, back_cnt, last_cnt;
  uint32_t n_left, zeros;   // counts that MSVC spilled into the list's first slot
  uint32_t list5_s, 
           rescale_at, limit20, running,
           since_rescale;
  live = syms->live;
  ent = syms->ent;
  w = __frame.list;
  gen = (uint8_t)exclusion_gen;
  owner = syms;
  cum = 0;
  i = 0;
  do
  {
    if ( (uint8_t)exclusion_mask[ent[i].sym] == gen )
    {
      c = 0;
    }
    else
    {
      e = &ent[i];
      c = e->cnt;
      *w++ = e;
    }
    cum += c;
    ++i;
  }
  while ( i < live );
  if ( !cum )
    return -1;
  *w = nullptr;
  tot0 = owner->tot;
  tot_all = cum + tot0;
  target = rc.get_freq(tot_all);
  tot = cum + tot0;
  p = (__frame.list[0]);
  rd = &__frame.list[1];
  first = (__frame.list[0]);
  cum_lo = 0;
  while ( 1 )
  {
    cum_lo += p->cnt;
    if ( cum_lo > (uint32_t)(int32_t)target )
      break;
    p = *rd++;
    if ( !p )
    {
      gen_b = (int8_t)gen;
      sym_cum = cum_lo;
      cum_hi = tot;
      sym_high = tot;
      q = first;
      rd2 = &__frame.list[1];
      do
      {
        exclusion_mask[q->sym] = gen_b;
        q = *rd2++;
      }
      while ( q );
      tot_1 = tot;
      __frame.list7 = -1;
      {
        rc.decode(cum_lo, cum_hi, tot_1);
        return __frame.list7;
      }
    }
  }
  sym_high = cum_lo;
  sym_cum = cum_lo - p->cnt;
  __frame.list7 = p->sym;
  p->cnt += 4;
  head = owner->ent;
  owner->since_rescale += 4;
  if ( p == head )
  {
    top = p->cnt;
  }
  else
  {
    // Swap this entry with the one before it.
    c_a = p->cnt;
    s_a = p->sym;
    q2 = p - 1;
    *p = *q2;
    q2->set(s_a, c_a);
    head = owner->ent;
    if ( q2 == head )
    {
      top = q2->cnt;
    }
    else
    {
      __frame.list5 = list5_s;
      while ( 1 )
      {
        top = q2->cnt;
        r = q2 - 1;
        if ( top <= r->cnt )
          break;
        // Swap the two entries: the more-used one moves towards the front.
        s_b = q2->sym;
        c_b = q2->cnt;
        *q2 = *r;
        r->set(s_b, c_b);
        head = owner->ent;
        --q2;
        if ( r == head )
        {
          list5_s = __frame.list5;
          top = r->cnt;
          goto LABEL_30;
        }
      }
      list5_s = __frame.list5;
    }
  }
LABEL_30:
  rescale_at = owner->rescale_at;
  if ( top > 251 || rescale_at < owner->since_rescale )
  {
    __frame.list0 = owner->live;
    limit20 = 20 * owner->n;
    n_left = __frame.list0;
    __frame.list5 = list5_s;
    __frame.list4 = rescale_at < limit20;
    cur = head - 1;
    do
    {
      prev = cur;
      ++cur;
      half = (__frame.list4 + (uint32_t)cur->cnt) >> 1;
      cur->cnt = half;
      if ( cur != owner->ent )
      {
        up = cur - 1;
        __frame.list3 = up->cnt;
        if ( half > __frame.list3 )
        {
          __frame.list2 = cur->sym;
          cur->set(up->sym, __frame.list3);
          if ( up != owner->ent )
          {
            (__frame.list[1]) = cur;
            __frame.list0 = n_left;
            do
            {
              back = up - 1;
              back_cnt = back->cnt;
              if ( half <= back_cnt )
                break;
              up->set(back->sym, back_cnt);
              --up;
            }
            while ( back != owner->ent );
            cur = (__frame.list[1]);
            n_left = __frame.list0;
          }
          up->set(__frame.list2, half);
        }
      }
      --n_left;
    }
    while ( n_left );
    list5_s = __frame.list5;
    running = owner->tot;
    __frame.list0 = 0;
    if ( !cur->cnt )
    {
      zeros = __frame.list0;
      do
      {
        ++zeros;
        owner->tot = ++running;
        last_cnt = prev->cnt;
        --prev;
      }
      while ( !last_cnt );
      __frame.list0 = zeros;
      owner->live -= zeros;
    }
    since_rescale = owner->since_rescale;
    owner->tot = running - (running >> 1);
    cum_lo = sym_cum;
    cum_hi = sym_high;
    tot_1 = tot_all;
    owner->since_rescale = since_rescale - (since_rescale >> 1);
  }
  else
  {
    tot_1 = tot_all;
    cum_lo = sym_cum;
    cum_hi = sym_high;
  }
  rc.decode(cum_lo, cum_hi, tot_1);
  return __frame.list7;
}

inline int32_t ModelBlock::decode_pixel(int32_t x)
{
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "DLRAW segfaults while decompressing"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
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
      uint8_t   _gap0[4];   // was int32_t idx_t
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t idx_t;
  ;
  FreqRec *freq;
  uint16_t *id3p;
  uint8_t g_ab;
  int8_t gen;
  ModelBlock *blk;
  FreqRec *freq3, *freq2;
  uint16_t *sym_cache;
  PixRec *up4;
  FreqRec *freq_tbl;
  SymList *sel1_list, **sel_p;
  int32_t bin_tot;
  int32_t arg_cum, arg_high, arg_tot;
  uint32_t done, lvl_a, pos1;
  PixRec *r8, *r7, *rec;   // row cursors out of ModelBlock
  PixRec *s0p;   // `row_cur[6]`, the row above
  uint16_t *pixq;   // a copy of `pix_cur`
  PixRec *r8b, *r7b, *next, *r5, *cur6b;
  uint8_t g_a, g_b, g_c, g_d, g_e;
  uint8_t *n2r;   // `uint8_t *` beside the `char` scalars above
  int16_t sym_rev, s1c, w1s, s3b;
  // A cumulative count, a high count and a total: the three arguments the
  // range coder takes, and it takes them unsigned.
  int32_t up_sym, left_sym, up_p1_sym, up_m1_sym, m_lo, m_up, nb, key, ctx_state, pair_last,
          cap, ctx_bucket, up_m0, m_w1, nb2, all_up, sig1,
          id1, sig2, id2, id3_used, s1a, m_up0, idx1, run, bucket,
          bit, msym1c, hit_a, idx_s, mask, seen, run0, s1b, *recw,
          flags_word, s3c, s3d, s1d, tot, target, cum, 
          w6a, b15a, w5a, msym3, msym1, msym2, pix1, cache0, cache1, c4,
          c5, c6, c7, h11, h10, h12, h13, h14, h15, h16, h17, h18, h19, h20,
          h21, h24, h28, h26, h30, h31, psym, bit2, msym1b, lsym, s0b,
          b15, w2t, w3t, w4t, w5, w0r, w1r, q1, w2r, w3, w3r, s0a,
          span, w4r, n4r, target2, cum2, lvl_b, b15b, w5b, w6b, s0c,
          s3a;
  PixRec *row;      // `row_cur[5]`, the current row
  // `row_cur[6]`, the row above: every reach through it is a multiple of
  // four `uint16_t`, which is one `PixRec`, and every one is `sym`.
  SymPair *pair;   // the group's counter pair for this context
  PixRec *cur6;   // `row_cur[6]`, the row above
  uint16_t pair_prev;   // a symbol, compared against four others
  PixRec *up5, *r8c;   // `row_cur[7]` and `row_cur[8]`
  uint16_t *pixp, *pixr, q1w, tot2,
           k0;
  uint32_t si, s2a, g2, g1, g0, g3, g4, k1, k2, k2h, k3,
           k4;
  PixRec *up2;     // `row_cur[7]`, two rows above
  PixRec *up1;     // `row_cur[6]`, the row above
  PixRec *up3;      // `row_cur[7]`, two rows above
  PixRec *cur6c;   // `row_cur[6]`, the row above
  cur6 = (PixRec *)this->row_cur[6];
  up_sym = cur6->sym;
  row = this->row_cur[5];
  left_sym = row[-1].sym;
  up_p1_sym = cur6[1].sym;
  // `sym5` is a `ModelBlock *` here and a *symbol* at the bottom of the body,
  // because the frame's union puts MSVC's spill slots over the 32-entry symbol
  // history `pixel_context` reads.  The eight reloads of it that used to sit
  // between here and there are gone, and the argument is worth writing down:
  // every write of `sym5` before the last reload comes from the block chain,
  // the one write that does not is *after* the last reload, and all three of
  // this body's `goto`s are forward -- so no reload could see the symbol.
  // `reduce_alphabet` has the same shape and fails that test, which is why
  // its reloads stay.
  __frame.sym5 = (this);
  up_m1_sym = cur6[-1].sym;
  __frame.sym0 = up_sym;
  ::mode_symbol[1] = up_sym;
  __frame.sym1 = left_sym;
  ::mode_symbol[2] = left_sym;
  __frame.sym3 = up_p1_sym;
  ::mode_symbol[3] = up_p1_sym;
  m_lo = cur6->match[1] + 4 * (up_p1_sym == up_m1_sym);
  m_up = cur6[1].match[1];
  __frame.sym2 = up_m1_sym;
  mode_symbol[4] = up_m1_sym;
  nb = 32 * row[-1].match[2] + 16 * row[-1].match[4] + (2 * row[-1].match[0] + 8 * m_up + m_lo);
  if ( up_sym == left_sym )
  {
    if ( __frame.sym3 == __frame.sym0 )
    {
      sym_rev = __frame.sym5->sym_rev[up_sym];
      if ( up_sym == __frame.sym2 )
        key = (uint16_t)(sym_rev - row[-2].sym);
      else
        key = (uint16_t)(sym_rev - __frame.sym2);
    }
    else
    {
      key = (uint16_t)(__frame.sym5->sym_rev[up_sym] - __frame.sym3);
    }
  }
  else
  {
    key = (uint16_t)(__frame.sym5->sym_rev[up_sym] - __frame.sym1);
  }
  __frame.sym5->sym_cache = &__frame.sym5->sym_ctr[8 * key];
  ctx_state = this->ctx_state[nb];
  this->ctx_state_seen = ctx_state;
  pair = &this->group_ctr[ctx_state][key];
  this->pix_cur = (uint16_t *)pair;
  pair_last = pair->last;
  if ( pair_last == __frame.sym0 )
  {
    cap = 15;
  }
  else if ( pair_last == __frame.sym1 )
  {
    cap = 30;
  }
  else if ( pair_last == __frame.sym3 )
  {
    cap = 45;
  }
  else
  {
    cap = 60;
    if ( !(pair_last == __frame.sym2) )
      cap = 0;
  }
  pair_prev = pair->prev;
  if ( pair_prev == __frame.sym0 )
  {
    cap += 75;
  }
  else if ( pair_prev == __frame.sym1 )
  {
    cap += 150;
  }
  else if ( pair_prev == __frame.sym3 )
  {
    cap += 225;
  }
  else if ( pair_prev == __frame.sym2 )
  {
    cap += 300;
  }
  r8 = this->row_cur[8];
  __frame.sym3 = (int32_t)cur6;
  ctx_bucket = this->ctx_bucket[ctx_state + cap];
  r7 = this->row_cur[7];
  this->bucket_idx = ctx_bucket;
  up_m0 = cur6->match[0];
  __frame.sym4 = (FreqRec *)row;
  __frame.sym5 = (this);
  __frame.sym2 = up_m0;
  m_w1 = row[-1].match[1];
  // `v186` is one stack slot with two roles: a row cursor here, and the
  // `uint16_t` value out of `sym_cache[4]` at 11290.  Splitting it needs the frame
  // to dissolve first, so the cast records the double booking (§4.2).
  __frame.sym6 = (int32_t)this->row_cur[9];
  nb2 = 8 * row[-2].match[2] + 4 * row[-2].match[5] + m_w1 + 2 * row[-2].match[4];
  all_up = ((uint8_t)(((PixRec *)__frame.sym6)->match[0] & r8->match[0] & __frame.sym2 & r7->match[0]) << 9)
      + ((uint8_t)(((PixRec *)__frame.sym6)->match[1] & r8->match[1] & r7->match[1] & cur6->match[1]) << 8)
      + (ctx_bucket << 10)
      + nb2;
  cur6b = (PixRec *)__frame.sym3;
  sig1 = ((__frame.sym5->grad[3] == 0) << 7)
      + ((__frame.sym5->grad[2] == 0) << 6)
      + 32 * (__frame.sym5->grad[1] == 0)
      + 16 * (__frame.sym5->grad[0] == 0)
      + all_up;
  id1 = __frame.sym5->ctx_id1[sig1];
  if ( id1 == 0xFFFF )
  {
    __frame.sym5->ctx_id1[sig1] = __frame.sym5->ctx_id1_used;
    cur6b = this->row_cur[6];
    row = this->row_cur[5];
    ++this->ctx_id1_used;
    id1 = this->ctx_id1[sig1];
    __frame.sym2 = cur6b->match[0];
  }
  sig2 = row[-1].match[5] + 4 * cur6b[1].match[3] + 2 * __frame.sym2 + 8 * id1;
  id2 = this->ctx_id2[sig2];
  if ( id2 == 0xFFFF )
  {
    this->ctx_id2[sig2] = this->ctx_id2_used++;
    id2 = this->ctx_id2[sig2];
  }
  if ( (int32_t)this->alphabet < 32 )
  {
    id3_used = this->ctx_id3_used;
    __frame.sym1 = 16 * id2 + (__frame.sym1 & 0xF);
    id3p = &this->ctx_id3[__frame.sym1];
    id2 = *id3p;
    if ( id2 == 0xFFFF )
    {
      s1a = __frame.sym1;
      if ( id3_used > 53248 )
        s1a = __frame.sym1 | 0xF;
      id3p = &this->ctx_id3[s1a];
      id2 = *id3p;
    }
    if ( id2 >= id3_used )
    {
      *id3p = id3_used;
      ++this->ctx_id3_used;
      id2 = *id3p;
    }
  }
  if ( (this->row_cur[5][-1].match[1] & this->row_cur[5][-1].match[0]) != 0 )
  {
    up1 = this->row_cur[6];
    up2 = this->row_cur[7];
    // Nine "matches the pixel to the left" flags and one "matches the pixel
    // above": the run about to be coded is the same colour all the way back.
    if ( ((uint8_t)(up2[2].match[1] & up2[1].match[1] & up2[0].match[1] & up1[3].match[1] & up1[2].match[1]
                  & up1[1].match[1] & up1[0].match[1] & up1[0].match[0] & (int8_t)up1[-1].match[1])
        & up2[3].match[1]) != 0 )
    {
      m_up0 = up2[0].match[0];
      idx1 = 1;
      if ( this->width - x <= 1 )
      {
        run = 1;
      }
      else
      {
        __frame.sym1 = this->width - x;
        __frame.sym5 = (this);
        while ( 1 )
        {
          run = idx1;
          if ( (up1[idx1 + 2].match[1] & up1[idx1 + 2].match[0]) == 0 )
            break;
          m_up0 = (uint8_t)(up2[run].match[0] & m_up0);
          if ( ++idx1 >= __frame.sym1 )
          {
            run = idx1;
            goto LABEL_42;
          }
        }
      }
LABEL_42:
      bucket = *(this->run_bucket + idx1);
      // Record `8 * bucket + 4 * (two neighbour flags) + 2 * m_up0 + sym + 1`
      // of the 257-record grid: `269089 * 4` is +1 076 356, four bytes past
      // `esc_ctr`, and every term above it is a multiple of three words.
      bit = this->esc_ctr[(8 * bucket
                                   + 4 * (uint8_t)(up2[run + 3].match[1] & up2[run + 2].match[1])
                                   + 2 * m_up0
                                   + *(this->alpha_map + __frame.sym0)
                                   + 1)].decode_context_bit(this->esc_ctr);
      msym1c = ::mode_symbol[1];
      this->hit = bit;
      *(((uint8_t *)this->alpha_map) + msym1c) = bit;
      hit_a = this->hit;
      if ( hit_a )
      {
        idx_s = idx1;
      }
      else
      {
        idx_s = 0;
        if ( idx1 == 1 )
          goto LABEL_57;
        __frame.sym3 = idx1;
        __frame.sym0 = bucket;
        __frame.sym5 = (this);
        idx_s = 0;
        mask = 1 << (bucket & 31);
        seen = 0;
        do
        {
          if ( (mask | seen) < __frame.sym3 )
          {
            run0 = __frame.sym5->run_ctr[16 * ((seen == 0) + (bucket == __frame.sym0)) + bucket].n[0];
            __frame.sym2 = (int32_t)&__frame.sym5->run_ctr[16 * ((seen == 0) + (bucket == __frame.sym0)) + bucket];
            bin_tot = run0 + __frame.sym5->run_ctr[16 * ((seen == 0) + (bucket == __frame.sym0)) + bucket].n[1];
            __frame.sym1 = rc.decode_bit(
                     run0,
                     __frame.sym5->run_ctr[16 * ((seen == 0) + (bucket == __frame.sym0)) + bucket].n[1]);
            if ( __frame.sym5->run_ctr[16 * ((seen == 0) + (bucket == __frame.sym0)) + bucket].limit < (uint32_t)bin_tot )
              __rescale_counter_pair((BitCtr *)__frame.sym2);
            s1b = __frame.sym1;
            *(uint16_t *)(__frame.sym2 + 2 * __frame.sym1) += 8;
            if ( s1b )
              idx_s |= mask;
            seen |= idx_s & mask;
          }
          --bucket;
          mask >>= 1;
        }
        while ( mask );
      }
      if ( idx_s )
        this->row_cur[5][idx_s - 1].sym = this->row_cur[5][-1].sym;
      hit_a = this->hit;
LABEL_57:
      if ( idx_s > hit_a )
      {
        this->row_cur[6] = this->row_cur[6] + idx_s - hit_a;
        r8b = this->row_cur[8];
        r7b = this->row_cur[7] + idx_s;
        this->row_cur[7] = r7b - hit_a;
        this->row_cur[8] = r8b + idx_s - hit_a;
        rec = this->row_cur[5];
        this->row_cur[9] = this->row_cur[9] + idx_s - hit_a;
        *(uint32_t *)&rec->match[2] = 0x01010101;
        *(uint32_t *)this->row_cur[5] = 0x01010101;
        pixp = (uint16_t *)this->pix_cur;
        LOWORD(r7b) = ::mode_symbol[1];
        LOWORD(r8b) = *pixp;
        __frame.sym1 = ::mode_symbol[1];
        pixp[1] = (uint16_t)(uintptr_t)r8b;
        this->row_cur[5]->sym = (uint16_t)(uintptr_t)r7b;
        this->pix_cur[0] = (uint16_t)(uintptr_t)r7b;
        recw = (int32_t *)this->row_cur[5];
        flags_word = recw[1];
        __frame.sym3 = *recw;
        next = this->row_cur[5] + 1;
        this->row_cur[5] = next;
        if ( idx_s - hit_a != 1 )
        {
          __frame.sym2 = (idx_s - hit_a - 1) / 2;
          if ( __frame.sym2 )
          {
            s3c = __frame.sym3;
            __frame.sym0 = hit_a;
            s1c = __frame.sym1;
            si = 0;
            idx_t = idx_s;
            s2a = __frame.sym2;
            do
            {
              this->pix_cur[1] = s1c;
              *(uint32_t *)this->row_cur[5] = s3c;
              *(uint32_t *)&this->row_cur[5]->match[2] = flags_word;
              pixq = this->pix_cur;
              ++this->row_cur[5];
              pixq[1] = s1c;
              *(uint32_t *)this->row_cur[5] = s3c;
              *(uint32_t *)&this->row_cur[5]->match[2] = flags_word;
              next = this->row_cur[5] + 1;
              this->row_cur[5] = next;
              ++si;
            }
            while ( si < s2a );
            hit_a = __frame.sym0;
            idx_s = idx_t;
            done = 2 * si + 1;
          }
          else
          {
            done = 1;
          }
          if ( (uint32_t)(idx_s - hit_a - 1) > (done - 1) )
          {
            s3d = __frame.sym3;
            this->pix_cur[1] = __frame.sym1;
            *(uint32_t *)this->row_cur[5] = s3d;
            *(uint32_t *)&this->row_cur[5]->match[2] = flags_word;
            next = this->row_cur[5] + 1;
            this->row_cur[5] = next;
          }
        }
        idx_t = idx_s;
        cur6c = (PixRec *)this->row_cur[6];
        g_a = cur6c[-3].match[0];
        g_b = cur6c[-2].match[0];
        g_c = cur6c[2].match[0];
        g_d = cur6c[3].match[0];
        __frame.sym0 = (int32_t)cur6c;
        g_ab = g_b + g_a;
        g_e = cur6c[4].match[0];
        up3 = (PixRec *)this->row_cur[7];
        this->grad[0] = g_d + g_c + g_ab + g_e - 5;
        // Eight records of the row two above, `match[0]` in each: the
        // byte offsets 2, 10, 18, 26, 34 and -6, -14, -22 were records
        // 0..4 and -1..-3.  Three of the eight loads are `movsx` in the
        // original and five are `movzx`; every writer of these bytes is
        // a comparison, so the sign never shows, and the casts stay
        // because the instruction is what is being transcribed.
        this->grad[1] = up3[3].match[0]
                                     + up3[2].match[0]
                                     + up3[1].match[0]
                                     + up3[0].match[0]
                                     + (int8_t)up3[-1].match[0]
                                     + (int8_t)up3[-2].match[0]
                                     + (int8_t)up3[-3].match[0]
                                     + up3[4].match[0]
                                     - 8;
        s0p = (PixRec *)__frame.sym0;
        this->grad[2] = next[-4].match[1] + next[-3].match[1] - 2;
        s1d = __frame.sym1;
        this->grad[3] = next[-5].match[0]
                                     + next[-6].match[0]
                                     + next[-7].match[0]
                                     + next[-4].match[0]
                                     - 4;
        next[-1].match[4] = s1d == s0p[1].sym;
        idx_s = idx_t;
        this->row_cur[5][-1].match[5] = s1d == this->row_cur[6][2].sym;
        hit_a = this->hit;
      }
      if ( hit_a )
        return idx_s;
      goto LABEL_86;
    }
  }
  freq = (FreqRec *)&this->row_cur[4 * this->bucket_idx + 10];
  __frame.sym4 = freq;
  freq_tbl = &this->grid[id2 + 188];
  tot = freq_tbl->w[5];
  if ( freq_tbl->w[5] )
  {
    if ( tot == 1 )
    {
      b15 = freq->b15;
      w2t = b15 * freq_tbl->w[2];
      w3t = b15 * freq_tbl->w[3];
      __frame.sym1 = b15 * freq_tbl->w[0];
      w1s = b15 * freq_tbl->w[1];
      __frame.sym2 = w2t;
      w4t = b15 * freq_tbl->w[4];
      __frame.sym0 = w3t;
      __frame.sym3 = w4t;
      *freq_tbl = *freq;
      w5 = freq_tbl->w[5];
      w0r = freq_tbl->w[0];
      freq_tbl->b14 *= 8;
      __frame.sym5 = (this);
      w1r = 21 * freq_tbl->w[1];
      __frame.sym1 += (21 * w0r + w5 - 1) / w5;
      freq_tbl->w[0] = __frame.sym1;
      q1 = (w1r + w5 - 1) / w5;
      w2r = 21 * freq_tbl->w[2];
      w3 = freq_tbl->w[3];
      q1w = q1 + w1s;
      freq_tbl->w[1] = q1w;
      w3r = 21 * w3;
      s0a = __frame.sym0;
      n2r = (uint8_t *)((w2r + w5 - 1) / w5 + __frame.sym2);
      freq_tbl->w[2] = (uint16_t)(uintptr_t)n2r;
      span = (w3r + w5 - 1) / w5 + s0a;
      w4r = 21 * freq_tbl->w[4];
      freq_tbl->w[3] = span;
      n4r = (w4r + w5 - 1) / w5 + __frame.sym3;
      freq_tbl->w[4] = n4r;
      tot2 = __frame.sym1 + n4r + (span + (uint16_t)(uintptr_t)n2r + q1w);
      tot = tot2;
      freq_tbl->w[5] = tot2;
    }
    arg_tot = tot;
    target = rc.get_freq(arg_tot);
    cum = freq_tbl->w[0];
    if ( cum <= target )
    {
      cum += freq_tbl->w[1];
      if ( cum <= target )
      {
        cum += freq_tbl->w[2];
        if ( cum <= target )
        {
          cum += freq_tbl->w[3];
          if ( cum <= target )
          {
            cum += freq_tbl->w[4];
            lvl_a = 4;
          }
          else
          {
            lvl_a = 3;
          }
        }
        else
        {
          lvl_a = 2;
        }
      }
      else
      {
        lvl_a = 1;
      }
    }
    else
    {
      lvl_a = 0;
    }
    w6a = freq_tbl->w[6];
    arg_high = cum;
    b15a = freq_tbl->b15;
    arg_cum = cum - freq_tbl->w[lvl_a];
    w5a = freq_tbl->w[5];
    if ( w5a > w6a && (freq_tbl->w[lvl_a] + b15a + 8 < w5a || freq_tbl->w[5] > 0x4000u) )
    {
      g2 = freq_tbl->w[2];
      __frame.sym0 = w6a;
      g1 = freq_tbl->w[1];
      __frame.sym5 = (this);
      g0 = freq_tbl->w[0];
      __frame.sym1 = lvl_a;
      __frame.sym2 = b15a;
      LOWORD(g0) = g0 - (g0 >> 1);
      freq_tbl->w[0] = g0;
      LOWORD(g1) = g1 - (g1 >> 1);
      freq_tbl->w[1] = g1;
      LOWORD(g2) = g2 - (g2 >> 1);
      g3 = freq_tbl->w[3];
      freq_tbl->w[2] = g2;
      LOWORD(g3) = g3 - (g3 >> 1);
      g4 = freq_tbl->w[4];
      freq_tbl->w[3] = g3;
      LOWORD(g4) = g4 - (g4 >> 1);
      freq_tbl->w[4] = g4;
      LOWORD(g1) = g3 + g2 + g1;
      b15a = __frame.sym2;
      LOWORD(g4) = g0 + g4;
      w5a = (uint16_t)(g4 + g1);
      s0b = __frame.sym0;
      lvl_a = __frame.sym1;
      freq_tbl->w[5] = w5a;
      if ( s0b < 256 && !freq_tbl->b14 )
      {
        s0b = 256;
        freq_tbl->w[6] = 256;
      }
      if ( w5a > s0b )
      {
        if ( b15a < 15 )
          LOWORD(b15a) = 15;
        freq_tbl->b15 = b15a;
      }
    }
    freq_tbl->w[5] = b15a + w5a;
    freq_tbl->w[lvl_a] += b15a;
    rc.decode(arg_cum, arg_high, arg_tot);
    this->hit = lvl_a;
    if ( freq_tbl->b14 )
    {
      --freq_tbl->b14;
      freq2 = __frame.sym4;
      ++__frame.sym4->w[5];
      ++freq2->w[lvl_a];
      lvl_a = this->hit;
    }
  }
  else
  {
    arg_tot = freq->w[5];
    target2 = rc.get_freq(arg_tot);
    cum2 = __frame.sym4->w[0];
    if ( cum2 <= target2 )
    {
      cum2 += __frame.sym4->w[1];
      if ( cum2 <= target2 )
      {
        cum2 += __frame.sym4->w[2];
        if ( cum2 <= target2 )
        {
          cum2 += __frame.sym4->w[3];
          if ( cum2 <= target2 )
          {
            cum2 += __frame.sym4->w[4];
            lvl_b = 4;
          }
          else
          {
            lvl_b = 3;
          }
        }
        else
        {
          lvl_b = 2;
        }
      }
      else
      {
        lvl_b = 1;
      }
    }
    else
    {
      lvl_b = 0;
    }
    b15b = __frame.sym4->b15;
    arg_high = cum2;
    arg_cum = cum2 - __frame.sym4->w[lvl_b];
    w5b = __frame.sym4->w[5];
    w6b = __frame.sym4->w[6];
    __frame.sym3 = b15b;
    if ( w5b > w6b && (__frame.sym4->w[lvl_b] + __frame.sym3 + 8 < w5b || w5b > 0x4000) )
    {
      __frame.sym0 = w6b;
      __frame.sym5 = (this);
      __frame.sym1 = (int32_t)(uintptr_t)freq_tbl;
      __frame.sym2 = lvl_b;
      freq3 = __frame.sym4;
      k1 = __frame.sym4->w[1];
      k2 = __frame.sym4->w[2];
      k0 = __frame.sym4->w[0] - (__frame.sym4->w[0] >> 1);
      __frame.sym4->w[0] = k0;
      LOWORD(k1) = k1 - (k1 >> 1);
      freq3->w[1] = k1;
      k2h = k2 - (k2 >> 1);
      k3 = (uint16_t)freq3->w[3];
      freq3->w[2] = k2h;
      LOWORD(k3) = k3 - (k3 >> 1);
      k4 = (uint16_t)freq3->w[4];
      freq3->w[3] = k3;
      LOWORD(k4) = k4 - (k4 >> 1);
      freq3->w[4] = k4;
      LOWORD(k4) = k0 + k4;
      w5b = (uint16_t)(k4 + k3 + k2h + k1);
      s0c = __frame.sym0;
      freq_tbl = (FreqRec *)__frame.sym1;
      freq3->w[5] = w5b;
      lvl_b = __frame.sym2;
      if ( s0c < 256 && !__frame.sym4->b14 )
      {
        s0c = 256;
        __frame.sym4->w[6] = 256;
      }
      if ( w5b > s0c )
      {
        s3a = __frame.sym3;
        if ( __frame.sym3 < 15 )
          s3a = 15;
        __frame.sym3 = s3a;
        __frame.sym4->b15 = s3a;
      }
    }
    s3b = __frame.sym3;
    __frame.sym4->w[5] = __frame.sym3 + w5b;
    __frame.sym4->w[lvl_b] += s3b;
    rc.decode(arg_cum, arg_high, arg_tot);
    this->hit = lvl_b;
    freq_tbl->w[5] = freq_tbl->w[lvl_b]++ != 0;
    lvl_a = this->hit;
  }
  if ( lvl_a )
  {
    this->row_cur[5]->sym = mode_symbol[lvl_a];
    return 1;
  }
  idx_s = 0;
LABEL_86:
  gen = exclusion_gen;
  msym3 = ::mode_symbol[3];
  msym1 = ::mode_symbol[1];
  idx_t = idx_s;
  msym2 = ::mode_symbol[2];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  pixr = (uint16_t *)this->pix_cur;
  exclusion_mask[msym3] = gen;
  exclusion_mask[msym2] = gen;
  exclusion_mask[msym1] = gen;
  __byte_445440[0] = gen;
  this->sel[0] = nullptr;
  pix1 = pixr[1];
  __frame.sym0 = *pixr;
  sym_cache = this->sym_cache;
  cache0 = sym_cache[0];
  cache1 = sym_cache[1];
  __frame.sym1 = pix1;
  // The tail of the body reloads the symbol cache into the frame slots for
  // the next pixel, and four of those slots are typed as pointers by an
  // earlier lifetime.  A `uint16_t` symbol going into a 32-bit pointer slot
  // is what the width warning was about; the `uintptr_t` says the value is
  // being parked, not dereferenced.
  blk = (ModelBlock *)(uintptr_t)sym_cache[3];
  __frame.sym2 = cache0;
  c4 = sym_cache[4];
  __frame.sym3 = cache1;
  c5 = sym_cache[5];
  __frame.sym4 = (FreqRec *)(uintptr_t)sym_cache[2];
  c6 = sym_cache[6];
  c7 = sym_cache[7];
  __frame.sym5 = (ModelBlock *)((uint32_t *)blk);
  up4 = (PixRec *)this->row_cur[6];
  __frame.sym6 = c4;
  h11 = up4[2].sym;
  __frame.sym7 = c5;
  r5 = this->row_cur[5];
  __frame.sym8 = c6;
  h10 = r5[-2].sym;
  __frame.sym9 = c7;
  __frame.sym10 = h10;
  up5 = this->row_cur[7];
  h12 = up5[1].sym;
  __frame.sym11 = h11;
  h13 = up5->sym;
  __frame.sym12 = h12;
  h14 = up4[-2].sym;
  __frame.sym13 = h13;
  h15 = up5[-1].sym;
  __frame.sym14 = h14;
  h16 = r5[-3].sym;
  __frame.sym15 = h15;
  h17 = up4[3].sym;
  __frame.sym16 = h16;
  h18 = up4[4].sym;
  __frame.sym17 = h17;
  h19 = r5[-4].sym;
  __frame.sym18 = h18;
  h20 = up4[-3].sym;
  __frame.sym19 = h19;
  h21 = up5[2].sym;
  __frame.sym20 = h20;
  __frame.sym21 = h21;
  r8c = this->row_cur[8];
  __frame.sym22 = r8c->sym;
  __frame.sym23 = up5[-2].sym;
  h24 = r5[-5].sym;
  h28 = r5[-7].sym;
  __frame.sym24 = h24;
  __frame.sym25 = r8c[1].sym;
  h26 = up4[5].sym;
  h30 = up4[7].sym;
  __frame.sym26 = h26;
  __frame.sym27 = this->row_cur[9]->sym;
  __frame.sym28 = h28;
  __frame.sym29 = r8c[-1].sym;
  h31 = up5[3].sym;
  this->sym_pos = 0;
  __frame.sym30 = h30;
  __frame.sym31 = h31;
  do
  {
    psym = (this)->pixel_context((uint32_t *)__frame.sym);
    if ( psym >= 0 )
    {
      bit2 = this->bit_node[this->ctr_node].decode_context_bit(&this->bit_root[this->ctr_fallback]);
      this->row_cur[5]->sym = psym;
      if ( bit2 )
        return idx_t + 1;
      exclusion_mask[psym] = exclusion_gen;
    }
    pos1 = this->sym_pos + 1;
    this->sym_pos = pos1;
  }
  while ( pos1 < 32 );
  msym1b = ::mode_symbol[1];
  sel1_list = this->sel1_list;
  this->sel[0] = &this->sel0_list[::mode_symbol[2]];
  sel_p = this->sel_cur;
  this->sel[1] = &sel1_list[msym1b];
  while ( 1 )
  {
    if ( (*sel_p)->live )
    {
      lsym = __decode_symbol_list(*sel_p);
      this->row_cur[5]->sym = lsym;
      if ( lsym >= 0 )
        return idx_t + 1;
      sel_p = this->sel_cur;
    }
    this->sel_cur = ++sel_p;
  }
}

inline int32_t ModelBlock::code_pixel(int32_t x)
{
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "DLRAW segfaults while compressing"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
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
      uint8_t   _gap0[4];   // was int32_t runlen_s
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t runlen_s;
  ;
  PixRec *r4;
  uint8_t *alpha_map;
  bool up_eq_west;
  int8_t mode;
  uint16_t rev;
  ModelBlock *blk;
  int32_t arg_cum;
  FreqRec *frec;
  uint16_t *wr;
  PixRec *up4;
  uint16_t wp;
  SymList *sel1_list, **sel_p;
  uint32_t arg_tot, arg_high, done, rec_word, grid_kind;
  PixRec *r1, *r2, *cur6b, *r3, *r5;   // row cursors out of ModelBlock
  FreqRec *binp, *binp_s;
  uint16_t *pixp;   // a copy of `pix_cur`
  uint8_t match1, m0, m1, m2, m3;
  uint8_t *bp;   // `uint8_t *` beside the `char` scalars above
  uint16_t *id3p;   // a cursor into `ctx_id3`
  int16_t s8, w1s, acc2, s3, g0;
  SymPair *pair;   // the group's counter pair for this context
  // A cumulative count, a high count and a total: the three arguments the
  // range coder takes, and it takes them unsigned.
  int32_t up_sym, left_sym, up_next_sym, upleft_sym, nb, key, ctx_state, pair_last, cap,
          pair_prev, ctx_bucket, up_match0, cur9v, m_w1, nb2, sig1, id1, sig2,
          id2, id3_used, sig3, m_w1b, m_w0, m_up0, to_edge, one, run,
          run_pair, amap, runlen, run_hit, row_cur9, run_left, s1,
          s8b, s6, msym1, s4, bit5, first, s5, s1b,
          bucket_i, s9, cum1, lvl_a, w6a, w5a,
          b15a, msym3, excl_sym_a, excl_sym_b, w6d, wq1, cache2,
          cache1, cache3, cache4, cache6, cur5_back2, h11, h12, h13, h14, h15,
          h16, h17, h18, h19, h20, h21, h24, h28, h26, h30, esym, up_hit, pos1,
          msym1b, w6b, *ip, b15, w2t, w3t, w4t, w5, acc, q1, w2s, q2,
          w3s, w4s, s9b, cum2, lvl_b, w5c, w6c, w5b, s3b;
  // `row_cur[6]`, the row above: every reach through it is a multiple of
  // four `uint16_t`, which is one `PixRec`, and every one is `sym`.
  PixRec *row;   // `row_cur[5]`, the current row
  PixRec *cur6;   // `row_cur[6]`, the row above
  PixRec *cur5, *cur5p1, *cur5p1b, *r6, *r7;   // row cursors out of ModelBlock
  uint16_t *runp, *wq, *sym_cache, *cache0p, w6;
  PixRec *cur2;   // `row_cur[5]`, one record past the pixel just written
  uint32_t bin_tot, half, k, h2, w1a, w0, h3, h4, w1, g2, g2h, g3,
          g4;
  PixRec *up3;      // `row_cur[7]`, two rows above
  PixRec *cur6c, *up2;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  cur6 = (PixRec *)this->row_cur[6];
  up_sym = cur6->sym;
  cur5 = this->row_cur[5];
  left_sym = cur5[-1].sym;
  up_next_sym = cur6[1].sym;
  // `sym7` is a `ModelBlock *` here and a *symbol* at the bottom of the body:
  // the frame's union puts MSVC's spill slots over the 32-entry symbol history
  // `pixel_context` reads.  The nine reloads that used to sit between the two
  // are gone, and the argument is the same as `decode_pixel`'s -- every write
  // before the last reload comes from the block chain, the one that does not is
  // after it, and this body's only `goto` is forward.
  __frame.sym7 = (this);
  upleft_sym = cur6[-1].sym;
  __frame.sym8 = up_sym;
  ::mode_symbol[1] = up_sym;
  __frame.sym10 = left_sym;
  ::mode_symbol[2] = left_sym;
  __frame.sym6 = up_next_sym;
  ::mode_symbol[3] = up_next_sym;
  __frame.sym5 = upleft_sym;
  mode_symbol[4] = upleft_sym;
  nb = 32 * cur5[-1].match[2]
      + 16 * cur5[-1].match[4]
      + 2 * cur5[-1].match[0]
      + 8 * cur6[1].match[1]
      + (cur6->match[1] + 4 * (up_next_sym == upleft_sym));
  up_eq_west = up_sym == left_sym;
  if ( up_eq_west )
  {
    if ( __frame.sym8 == __frame.sym6 )
    {
      rev = *((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720);
      if ( __frame.sym8 == __frame.sym5 )
        key = (uint16_t)(rev - cur5[-2].sym);
      else
        key = (uint16_t)(rev - __frame.sym5);
    }
    else
    {
      key = (uint16_t)(*((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720) - __frame.sym6);
    }
  }
  else
  {
    key = (uint16_t)(*((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720) - __frame.sym10);
  }
  __frame.sym7->sym_cache = &__frame.sym7->sym_ctr[8 * key];
  ctx_state = this->ctx_state[nb];
  *(int32_t *)&this->ctx_state_seen = ctx_state;
  pair = &this->group_ctr[ctx_state][key];
  this->pix_cur = (uint16_t *)pair;
  pair_last = pair->last;
  if ( pair_last == __frame.sym8 )
  {
    cap = 15;
  }
  else if ( pair_last == __frame.sym10 )
  {
    cap = 30;
  }
  else if ( pair_last == __frame.sym6 )
  {
    cap = 45;
  }
  else
  {
    up_eq_west = pair_last == __frame.sym5;
    cap = 60;
    if ( !up_eq_west )
      cap = 0;
  }
  pair_prev = pair->prev;
  if ( pair_prev == __frame.sym8 )
  {
    cap += 75;
  }
  else if ( pair_prev == __frame.sym10 )
  {
    cap += 150;
  }
  else if ( pair_prev == __frame.sym6 )
  {
    cap += 225;
  }
  else if ( pair_prev == __frame.sym5 )
  {
    cap += 300;
  }
  r1 = this->row_cur[8];
  __frame.sym1 = (int32_t)cur6;
  ctx_bucket = this->ctx_bucket[ctx_state + cap];
  r2 = this->row_cur[7];
  *(int32_t *)&this->bucket_idx = ctx_bucket;
  up_match0 = cur6->match[0];
  match1 = cur6->match[1];
  __frame.sym2 = (uint16_t *)cur5;
  __frame.sym7 = (this);
  __frame.sym0 = up_match0;
  cur9v = (int32_t)this->row_cur[9];
  m_w1 = cur5[-1].match[1];
  __frame.sym3 = cur9v;
  nb2 = 8 * cur5[-2].match[2]
      + 4 * cur5[-2].match[5]
      + m_w1
      + 2 * cur5[-2].match[4];
  sig1 = ((__frame.sym7->grad[3] == 0) << 7)
      + ((__frame.sym7->grad[2] == 0) << 6)
      + 32 * (__frame.sym7->grad[1] == 0)
      + 16 * (__frame.sym7->grad[0] == 0)
      + ((uint8_t)(((PixRec *)__frame.sym3)->match[0] & r1->match[0] & __frame.sym0 & r2->match[0]) << 9)
      + ((uint8_t)(((PixRec *)__frame.sym3)->match[1] & r1->match[1] & r2->match[1] & match1) << 8)
      + (ctx_bucket << 10)
      + nb2;
  id1 = __frame.sym7->ctx_id1[sig1];
  cur6b = (PixRec *)__frame.sym1;
  if ( id1 == 0xFFFF )
  {
    __frame.sym7->ctx_id1[sig1] = *(int32_t *)&__frame.sym7->ctx_id1_used;
    cur6b = this->row_cur[6];
    cur5 = this->row_cur[5];
    ++*(int32_t *)&this->ctx_id1_used;
    id1 = this->ctx_id1[sig1];
    __frame.sym0 = cur6b->match[0];
  }
  sig2 = cur5[-1].match[5] + 4 * cur6b[1].match[3] + 2 * __frame.sym0 + 8 * id1;
  id2 = this->ctx_id2[sig2];
  if ( id2 == 0xFFFF )
  {
    this->ctx_id2[sig2] = (*(int32_t *)&this->ctx_id2_used)++;
    id2 = this->ctx_id2[sig2];
  }
  if ( *(int32_t *)&this->alphabet < 32 )
  {
    id3_used = *(int32_t *)&this->ctx_id3_used;
    sig3 = 16 * id2 + (__frame.sym10 & 0xF);
    id3p = &this->ctx_id3[sig3];
    id2 = *id3p;
    if ( id2 == 0xFFFF )
    {
      if ( id3_used > 53248 )
        sig3 |= 0xFu;
      id3p = &this->ctx_id3[sig3];
      id2 = *id3p;
    }
    if ( id2 >= id3_used )
    {
      *id3p = id3_used;
      ++*(int32_t *)&this->ctx_id3_used;
      id2 = *id3p;
    }
  }
  row = this->row_cur[5];
  m_w1b = row[-1].match[1];
  m_w0 = row[-1].match[0];
  __frame.sym9 = row;
  if ( (m_w1b & m_w0) != 0
    && (cur6c = this->row_cur[6],
        up2 = this->row_cur[7],
        ((uint8_t)(up2[2].match[1]
                         & up2[1].match[1]
                         & up2[0].match[1]
                         & cur6c[3].match[1]
                         & cur6c[2].match[1]
                         & cur6c[1].match[1]
                         & cur6c[0].match[1]
                         & cur6c[0].match[0]
                         & cur6c[-1].match[1])
       & up2[3].match[1]) != 0) )
  {
    m_up0 = up2[0].match[0];
    to_edge = *(int32_t *)&this->width - x;
    __frame.sym4 = 1;
    if ( to_edge <= 1 )
    {
      run = 1;
    }
    else
    {
      __frame.sym0 = to_edge;
      one = 1;
      __frame.sym7 = (this);
      while ( 1 )
      {
        run = one;
        if ( (cur6c[one + 2].match[1] & cur6c[one + 2].match[0]) == 0 )
          break;
        m_up0 = (uint8_t)(up2[run].match[0] & m_up0);
        if ( ++one >= __frame.sym0 )
        {
          __frame.sym4 = one;
          run = one;
          goto LABEL_42;
        }
      }
      __frame.sym4 = one;
    }
LABEL_42:
    run_pair = (uint8_t)(up2[run + 3].match[1] & up2[run + 2].match[1]);
    __frame.sym5 = *(uint8_t *)(*(int32_t *)&this->run_bucket + __frame.sym4);
    amap = *(uint8_t *)(*(int32_t *)&this->alpha_map + __frame.sym8) + 8 * __frame.sym5 + 4 * run_pair + 2 * m_up0;
    runlen = 0;
    if ( __frame.sym9->sym == __frame.sym8 )
    {
      __frame.sym0 = (int32_t)cur6c;
      __frame.sym7 = (this);
      do
        ++runlen;
      while ( runlen < __frame.sym4 && __frame.sym9[runlen].sym == __frame.sym8 );
      cur6c = (PixRec *)__frame.sym0;
    }
    run_hit = runlen == __frame.sym4;
    __frame.sym6 = run_hit;
    if ( runlen > run_hit )
    {
      this->row_cur[6] = &cur6c[runlen - run_hit];
      this->row_cur[7] = &up2[runlen - run_hit];
      row_cur9 = (int32_t)this->row_cur[9];
      this->row_cur[8] = this->row_cur[8] + runlen - run_hit;
      this->row_cur[9] = (PixRec *)row_cur9 + runlen - run_hit;
      *(uint32_t *)&__frame.sym9->match[2] = 0x01010101;
      *(uint32_t *)this->row_cur[5] = 0x01010101;
      this->pix_cur[1] = this->pix_cur[0];
      wp = __frame.sym8;
      this->row_cur[5]->sym = __frame.sym8;
      this->pix_cur[0] = wp;
      r3 = this->row_cur[5];
      rec_word = *(uint32_t *)r3;
      __frame.sym1 = *(uint32_t *)&r3->match[2];
      __frame.sym2 = (uint16_t *)(r3 + 1);
      this->row_cur[5] = r3 + 1;
      if ( runlen - run_hit != 1 )
      {
        run_left = runlen - run_hit - 1;
        half = run_left / 2;
        if ( run_left / 2 )
        {
          __frame.sym0 = run_left;
          s1 = __frame.sym1;
          runlen_s = runlen;
          k = 0;
          __frame.sym3 = amap;
          s8 = __frame.sym8;
          do
          {
            this->pix_cur[1] = s8;
            *(uint32_t *)this->row_cur[5] = rec_word;
            *(uint32_t *)&this->row_cur[5]->match[2] = s1;
            pixp = this->pix_cur;
            ++this->row_cur[5];
            pixp[1] = s8;
            *(uint32_t *)this->row_cur[5] = rec_word;
            *(uint32_t *)&this->row_cur[5]->match[2] = s1;
            cur5p1 = this->row_cur[5] + 1;
            this->row_cur[5] = cur5p1;
            ++k;
          }
          while ( k < half );
          run_left = __frame.sym0;
          amap = __frame.sym3;
          __frame.sym2 = (uint16_t *)cur5p1;
          runlen = runlen_s;
          done = 2 * k + 1;
        }
        else
        {
          done = 1;
        }
        if ( (uint32_t)run_left > (done - 1) )
        {
          this->pix_cur[1] = __frame.sym8;
          *(uint32_t *)this->row_cur[5] = rec_word;
          *(uint32_t *)&this->row_cur[5]->match[2] = __frame.sym1;
          cur5p1b = this->row_cur[5] + 1;
          this->row_cur[5] = cur5p1b;
          __frame.sym2 = (uint16_t *)cur5p1b;
        }
      }
      r4 = (PixRec *)this->row_cur[6];
      m0 = r4[-2].match[0];
      m1 = r4[2].match[0];
      m2 = r4[4].match[0];
      __frame.sym3 = amap;
      m3 = r4[-3].match[0];
      runlen_s = runlen;
      up3 = (PixRec *)this->row_cur[7];
      this->grad[0] = (r4[3].match[0] + m1 + m0 + m3 + m2 - 5);
      cur2 = (PixRec *)__frame.sym2;
      // Eight records of the row two above, `match[0]` in each: the
      // byte offsets 2, 10, 18, 26, 34 and -6, -14, -22 were records
      // 0..4 and -1..-3.  Three of the eight loads are `movsx` in the
      // original and five are `movzx`; every writer of these bytes is
      // a comparison, so the sign never shows, and the casts stay
      // because the instruction is what is being transcribed.
      this->grad[1] = up3[3].match[0]
                                   + up3[2].match[0]
                                   + up3[1].match[0]
                                   + up3[0].match[0]
                                   + (int8_t)up3[-1].match[0]
                                   + (int8_t)up3[-2].match[0]
                                   + (int8_t)up3[-3].match[0]
                                   + up3[4].match[0]
                                   - 8;
      this->grad[2] = cur2[-4].match[1] + cur2[-3].match[1] - 2;
      s8b = __frame.sym8;
      this->grad[3] = cur2[-5].match[0]
                                   + cur2[-6].match[0]
                                   + cur2[-7].match[0]
                                   + cur2[-4].match[0]
                                   - 4;
      cur2[-1].match[4] = s8b == r4[1].sym;
      amap = __frame.sym3;
      this->row_cur[5][-1].match[5] = s8b == this->row_cur[6][2].sym;
      runlen = runlen_s;
    }
    (&this->esc_ctr[(amap + 1)])->encode_context_bit(this->esc_ctr, __frame.sym6);
    s6 = __frame.sym6;
    msym1 = ::mode_symbol[1];
    alpha_map = (uint8_t *)this->alpha_map;
    *(int32_t *)&this->hit = __frame.sym6;
    *(alpha_map + msym1) = s6;
    if ( !s6 && __frame.sym4 != 1 )
    {
      runlen_s = runlen;
      __frame.sym0 = __frame.sym5;
      __frame.sym7 = (this);
      s4 = __frame.sym4;
      bit5 = 1 << (__frame.sym5 & 31);
      first = 0;
      s5 = __frame.sym5;
      do
      {
        if ( s4 > (bit5 | first) )
        {
          __frame.sym3 = first;
          runp = (uint16_t *)&__frame.sym7->run_ctr[16 * ((first == 0) + (s5 == __frame.sym0)) + s5];
          __frame.sym2 = runp;
          __frame.sym1 = runlen_s & bit5;
          bin_tot = *runp + runp[1];
          rc.encode_bit(*runp, runp[1], (runlen_s & bit5) != 0);
          if ( __frame.sym7->run_ctr[16 * ((first == 0) + (s5 == __frame.sym0)) + s5].limit < (uint32_t)bin_tot )
            __rescale_counter_pair((BitCtr *)__frame.sym2);
          s1b = __frame.sym1;
          __frame.sym4 = bit5;
          __frame.sym2[__frame.sym1 != 0] += 8;
          first |= s1b;
          bit5 = __frame.sym4;
        }
        --s5;
        bit5 >>= 1;
      }
      while ( bit5 );
      runlen = runlen_s;
    }
    if ( *(int32_t *)&this->hit )
      return runlen;
  }
  else
  {
    bucket_i = 4 * *(int32_t *)&this->bucket_idx;
    binp = (FreqRec *)&this->row_cur[bucket_i + 10];
    __frame.sym4 = (int32_t)(uintptr_t)binp;
    frec = &this->grid[id2 + 188];
    grid_kind = HIWORD(((int32_t *)this)[4 * id2 + 778]);
    if ( grid_kind )
    {
      if ( grid_kind == 1 )
      {
        ip = (int32_t *)&this->row_cur[bucket_i + 10];
        b15 = binp->b15;
        w2t = b15 * frec->w[2];
        w3t = b15 * frec->w[3];
        __frame.sym0 = b15 * frec->w[0];
        w1s = b15 * frec->w[1];
        __frame.sym1 = w2t;
        w4t = b15 * frec->w[4];
        __frame.sym2 = (uint16_t *)w3t;
        __frame.sym3 = w4t;
        *frec = *(FreqRec *)ip;
        w5 = frec->w[5];
        frec->b14 *= 8;
        __frame.sym7 = (this);
        acc = 21 * frec->w[1];
        __frame.sym0 += (21 * frec->w[0] + w5 - 1) / w5;
        frec->w[0] = __frame.sym0;
        q1 = (acc + w5 - 1) / w5;
        LOWORD(acc) = __frame.sym1;
        w2s = 21 * frec->w[2];
        w6 = q1 + w1s;
        frec->w[1] = w6;
        q2 = (w2s + w5 - 1) / w5;
        w3s = 21 * frec->w[3];
        LOWORD(acc) = q2 + acc;
        frec->w[2] = acc;
        bp = (uint8_t *)__frame.sym2 + (w3s + w5 - 1) / w5;
        w4s = 21 * frec->w[4];
        frec->w[3] = (uint16_t)(uintptr_t)bp;
        acc2 = (uint16_t)(uintptr_t)bp + acc + w6;
        LOWORD(w5) = (w4s + w5 - 1) / w5 + __frame.sym3;
        frec->w[4] = w5;
        grid_kind = (uint16_t)(__frame.sym0 + w5 + acc2);
        frec->w[5] = grid_kind;
        __frame.sym9 = this->row_cur[5];
      }
      s9 = __frame.sym9->sym;
      arg_tot = grid_kind;
      if ( s9 == __frame.sym8 )
      {
        cum1 = frec->w[0];
        lvl_a = 1;
      }
      else if ( s9 == __frame.sym10 )
      {
        cum1 = frec->w[0] + frec->w[1];
        lvl_a = 2;
      }
      else if ( s9 == __frame.sym6 )
      {
        cum1 = frec->w[0] + frec->w[2] + frec->w[1];
        lvl_a = 3;
      }
      else if ( s9 == __frame.sym5 )
      {
        cum1 = frec->w[5] - frec->w[4];
        lvl_a = 4;
      }
      else
      {
        cum1 = 0;
        lvl_a = 0;
      }
      w6a = frec->w[6];
      arg_cum = cum1;
      arg_high = frec->w[lvl_a] + cum1;
      w5a = frec->w[5];
      b15a = frec->b15;
      if ( w5a > w6a
        && (frec->w[lvl_a] + b15a + 8 < w5a || frec->w[5] > 0x4000u) )
      {
        h2 = frec->w[2];
        __frame.sym0 = w6a;
        w1a = frec->w[1];
        __frame.sym7 = (this);
        __frame.sym1 = lvl_a;
        w0 = frec->w[0];
        __frame.sym2 = (uint16_t *)b15a;
        LOWORD(w0) = w0 - (w0 >> 1);
        frec->w[0] = w0;
        LOWORD(w1a) = w1a - (w1a >> 1);
        frec->w[1] = w1a;
        LOWORD(h2) = h2 - (h2 >> 1);
        h3 = frec->w[3];
        frec->w[2] = h2;
        LOWORD(h3) = h3 - (h3 >> 1);
        h4 = frec->w[4];
        frec->w[3] = h3;
        LOWORD(h4) = h4 - (h4 >> 1);
        frec->w[4] = h4;
        LOWORD(w1a) = h3 + h2 + w1a;
        b15a = (int32_t)__frame.sym2;
        LOWORD(w1a) = w0 + h4 + w1a;
        lvl_a = __frame.sym1;
        w5a = (uint16_t)w1a;
        w6b = __frame.sym0;
        frec->w[5] = w5a;
        if ( w6b < 256 && !frec->b14 )
        {
          w6b = 256;
          frec->w[6] = 256;
        }
        if ( w5a > w6b )
        {
          if ( b15a < 15 )
            LOWORD(b15a) = 15;
          frec->b15 = b15a;
        }
      }
      frec->w[5] = b15a + w5a;
      frec->w[lvl_a] += b15a;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this->hit = lvl_a;
      if ( frec->b14 )
      {
        --frec->b14;
        binp_s = binp;
        ++binp->w[5];
        ++binp_s->w[lvl_a];
        lvl_a = *(int32_t *)&this->hit;
      }
    }
    else
    {
      wr = ((uint16_t *)&this->row_cur[bucket_i + 10]);
      s9b = __frame.sym9->sym;
      arg_tot = binp->w[5];
      if ( s9b == __frame.sym8 )
      {
        cum2 = *wr;
        lvl_b = 1;
      }
      else if ( s9b == __frame.sym10 )
      {
        cum2 = *wr + wr[1];
        lvl_b = 2;
      }
      else if ( s9b == __frame.sym6 )
      {
        cum2 = *wr + wr[2] + wr[1];
        lvl_b = 3;
      }
      else if ( s9b == __frame.sym5 )
      {
        cum2 = wr[5] - wr[4];
        lvl_b = 4;
      }
      else
      {
        cum2 = 0;
        lvl_b = 0;
      }
      arg_cum = cum2;
      arg_high = binp->w[lvl_b] + cum2;
      w5c = binp->w[5];
      w6c = binp->w[6];
      __frame.sym3 = binp->b15;
      if ( w5c > w6c && (binp->w[lvl_b] + __frame.sym3 + 8 < w5c || w5c > 0x4000) )
      {
        __frame.sym0 = w6c;
        __frame.sym7 = (this);
        w1 = binp->w[1];
        g2 = binp->w[2];
        __frame.sym1 = (int32_t)(uintptr_t)frec;
        __frame.sym2 = (uint16_t *)lvl_b;
        g0 = binp->w[0] - (binp->w[0] >> 1);
        binp->w[0] = g0;
        LOWORD(w1) = w1 - (w1 >> 1);
        binp->w[1] = w1;
        g2h = g2 - (g2 >> 1);
        g3 = (uint16_t)binp->w[3];
        binp->w[2] = g2h;
        LOWORD(g3) = g3 - (g3 >> 1);
        g4 = (uint16_t)binp->w[4];
        binp->w[3] = g3;
        LOWORD(g4) = g4 - (g4 >> 1);
        binp->w[4] = g4;
        LOWORD(g4) = g0 + g4;
        lvl_b = (int32_t)__frame.sym2;
        w5c = (uint16_t)(g4 + g3 + g2h + w1);
        w5b = __frame.sym0;
        frec = (FreqRec *)__frame.sym1;
        binp->w[5] = w5c;
        if ( w5b < 256 && !binp->b14 )
        {
          w5b = 256;
          binp->w[6] = 256;
        }
        if ( w5c > w5b )
        {
          s3b = __frame.sym3;
          if ( __frame.sym3 < 15 )
            s3b = 15;
          __frame.sym3 = s3b;
          binp->b15 = s3b;
        }
      }
      s3 = __frame.sym3;
      binp->w[5] = __frame.sym3 + w5c;
      binp->w[lvl_b] += s3;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this->hit = lvl_b;
      frec->w[5] = (frec->w[lvl_b])++ != 0;
      lvl_a = *(int32_t *)&this->hit;
    }
    if ( lvl_a )
      return 1;
    runlen = 0;
  }
  mode = exclusion_gen;
  msym3 = ::mode_symbol[3];
  excl_sym_a = ::mode_symbol[2];
  excl_sym_b = ::mode_symbol[1];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  exclusion_mask[msym3] = mode;
  wq = (uint16_t *)this->pix_cur;
  exclusion_mask[excl_sym_a] = mode;
  exclusion_mask[excl_sym_b] = mode;
  sym_cache = this->sym_cache;
  __byte_445440[0] = mode;
  runlen_s = runlen;
  this->sel[0] = nullptr;
  w6d = *wq;
  wq1 = wq[1];
  cache0p = (uint16_t *)(uintptr_t)*sym_cache;
  cache2 = sym_cache[2];
  __frame.sym0 = w6d;
  cache1 = sym_cache[1];
  __frame.sym1 = wq1;
  cache3 = sym_cache[3];
  __frame.sym2 = cache0p;
  cache4 = sym_cache[4];
  __frame.sym3 = cache1;
  // Same as `decode_pixel`'s tail: symbols going into pointer-typed slots.
  blk = (ModelBlock *)(uintptr_t)sym_cache[5];
  __frame.sym4 = cache2;
  cache6 = sym_cache[6];
  __frame.sym5 = cache3;
  r5 = this->row_cur[5];
  __frame.sym6 = cache4;
  cur5_back2 = r5[-2].sym;
  __frame.sym7 = (ModelBlock *)(blk);
  up4 = (PixRec *)this->row_cur[6];
  __frame.sym8 = cache6;
  h11 = up4[2].sym;
  __frame.sym9 = (PixRec *)(uintptr_t)sym_cache[7];
  __frame.sym10 = cur5_back2;
  r6 = this->row_cur[7];
  h12 = r6[1].sym;
  __frame.sym11 = h11;
  h13 = r6->sym;
  __frame.sym12 = h12;
  h14 = up4[-2].sym;
  __frame.sym13 = h13;
  h15 = r6[-1].sym;
  __frame.sym14 = h14;
  h16 = r5[-3].sym;
  __frame.sym15 = h15;
  h17 = up4[3].sym;
  __frame.sym16 = h16;
  h18 = up4[4].sym;
  __frame.sym17 = h17;
  h19 = r5[-4].sym;
  __frame.sym18 = h18;
  h20 = up4[-3].sym;
  __frame.sym19 = h19;
  h21 = r6[2].sym;
  __frame.sym20 = h20;
  __frame.sym21 = h21;
  r7 = this->row_cur[8];
  __frame.sym22 = r7->sym;
  __frame.sym23 = r6[-2].sym;
  h24 = r5[-5].sym;
  h28 = r5[-7].sym;
  __frame.sym24 = h24;
  __frame.sym25 = r7[1].sym;
  h26 = up4[5].sym;
  h30 = up4[7].sym;
  __frame.sym26 = h26;
  __frame.sym27 = this->row_cur[9]->sym;
  __frame.sym28 = h28;
  __frame.sym29 = r7[-1].sym;
  __frame.sym30 = h30;
  __frame.sym31 = r6[3].sym;
  *(int32_t *)&this->sym_pos = 0;
  do
  {
    esym = (this)->pixel_context((uint32_t *)__frame.sym);
    if ( esym >= 0 )
    {
      up_hit = esym == this->row_cur[5]->sym;
      (&this->bit_node[*(int32_t *)&this->ctr_node])->encode_context_bit(&this->bit_root[*(int32_t *)&this->ctr_fallback], up_hit);
      if ( up_hit )
        return runlen_s + 1;
      exclusion_mask[esym] = exclusion_gen;
    }
    pos1 = *(int32_t *)&this->sym_pos + 1;
    *(int32_t *)&this->sym_pos = pos1;
  }
  while ( pos1 < 32 );
  msym1b = ::mode_symbol[1];
  sel1_list = this->sel1_list;
  this->sel[0] = &this->sel0_list[::mode_symbol[2]];
  sel_p = this->sel_cur;
  this->sel[1] = &sel1_list[msym1b];
  while ( 1 )
  {
    if ( (*sel_p)->live )
    {
      if ( (*sel_p)->code_symbol(this->row_cur[5]->sym) )
        return runlen_s + 1;
      sel_p = this->sel_cur;
    }
    this->sel_cur = ++sel_p;
  }
}

inline void ModelBlock::expand_alphabet()
{
  // Its frame was lifted: `tools/liftframe.py` gave every member its own
  // storage and the gate held.  The comment that stood here said the opposite,
  // on the authority of a sweep that by then could no longer make the attempt.
  
  // It is sixteen `SymList`s, and the three untyped arrays it was recovered as
  // were one array all along.  `sizeof(SymList)` is 24 and `ent` is at +20, so
  // the free loop at the end -- which walked back from one past the last of
  // them, six words at a time, taking `[5]` -- is `lists[15]` down to
  // `lists[0]`, `.ent` each; and the zeroing loop at the top, which wrote
  // `v30[12 * i]` and `v30[12 * i + 6]`, is bytes 20 + 48i and 44 + 48i, which
  // are `lists[2i].ent` and `lists[2i + 1].ent`.
  
  // `spill` is genuinely past the array: MSVC used it to hold `nbytes` across
  // the inner decode loop.
  SymList lists[16];
  uint32_t spill[5];
  ;
  uint32_t *codes_p;   // was int32_t: this holds an address
  uint32_t nbytes, bits;
  void *codes;
  int32_t left2, left, run, gap;
  uint32_t mask, i, cap, n_1, n_syms, j, k, carry, s, b, piece, s2;
  bits = this->depth;
  mask = 0xFFFFFFFF >> (-(uint8_t)this->depth & 31);
  nbytes = (bits + 7) >> 3;
  // Two records a pass, which is how MSVC unrolled it.
  for ( i = 0; i < 8; ++i )
  {
    lists[2 * i].ent = nullptr;
    lists[2 * i + 1].ent = nullptr;
  }
  cap = mask + 1;
  if ( bits > 8 )
    cap = 8193;
  n_1 = __rc_decode_flat(cap);
  this->alphabet = n_1 + 1;
  if ( (int32_t)(n_1 + 1) <= 0x2000 )
  {
    codes = bmf_new(4 * n_1 + 4);
    n_syms = this->alphabet;
    this->sym_code = (uint32_t *)codes;
    if ( n_syms )
    {
      for ( j = 0; j < n_syms; ++j )
      {
        this->sym_code[j] = j;
        n_syms = this->alphabet;
      }
    }
    if ( (int32_t)this->depth > 8 )
    {
      if ( 4 * nbytes )
      {
        k = 0;
        do
          __init_symbol_list(&lists[k++], (int32_t)this, 256, 1);
        while ( k < (4 * nbytes) );
        n_syms = this->alphabet;
      }
      if ( n_syms )
      {
        codes_p = this->sym_code;
        carry = 0;
        s = 0;
        do
        {
          codes_p[s] = 0;
          if ( nbytes )
          {
            spill[0] = nbytes;
            b = 0;
            do
            {
              piece = __decode_symbol_list(&lists[4 * b + carry]);
              carry = piece >> 6;
              this->sym_code[s] += (piece << ((8 * b) & 31));
              ++b;
            }
            while ( b < spill[0] );
            nbytes = spill[0];
          }
          codes_p = this->sym_code;
          carry = (uint8_t)codes_p[s++] >> 7;
        }
        while ( s < this->alphabet );
      }
    }
    else if ( n_syms <= mask )
    {
      __init_symbol_list(&lists[0], (int32_t)this, mask - n_syms + 2, 1);
      lists[0].rescale_at = 19 * lists[0].n;
      if ( !(this->alphabet == 0) )
      {
        run = 0;
        s2 = 0;
        do
        {
          gap = __decode_symbol_list(&lists[0]);
          this->sym_code[s2] = gap + run;
          run += gap + 1;
          ++s2;
        }
        while ( s2 < this->alphabet );
      }
    }
    // Every list's entries, whether or not this path initialised it: the
    // uninitialised ones are the null `ent`s zeroed at the top, so those
    // `free`s are no-ops.
    for ( left = 15; left >= 0; --left )
      free(lists[left].ent);
  }
  else
  {
    this->depth = 8;
    *(uint32_t *)&this->height = (*(uint32_t *)&this->height * nbytes);
    this->expand_alphabet();
    for ( left2 = 15; left2 >= 0; --left2 )
      free(lists[left2].ent);
  }
}

ModelBlock *__layout_workspace(ModelBlock *blk, int32_t unread_flag, int32_t img_w, int32_t img_h, int32_t img_depth)
{
  ;
  PixRec *buf;   // one of the five row buffers
  uint8_t bucket;
  uint16_t rev;
  uint32_t e0, e1, w;
  uint8_t *runs;
  int32_t j, r, x, bits;
  uint32_t k, m, s, n, i8, i24;
  w = img_w;
  exclusion_gen = 1;
  blk->width = img_w;
  *(uint32_t *)&blk->height = img_h;
  blk->depth = img_depth;
  blk->depth_raw = img_depth;
  blk->escape.ent = nullptr;
  blk->sym_code = nullptr;
  for ( j = 0; j < 5; ++j )
  {
    // `8 * w + 128` is the plane's width plus sixteen records: eight of
    // left margin, which is what the cursor starts past, and eight of right.
    // Every record is seeded "matches all six neighbours", so a margin read
    // off either end of the row contributes 1 and not whatever `bmf_new`
    // left there.
    buf = (PixRec *)bmf_new(8 * w + 128);
    blk->row_cur[j] = buf;
    blk->row_cur[j + 5] = buf + 8;
    w = blk->width;
    if ( (int32_t)blk->width > -16 )
    {
      r = 0;
      do
      {
        blk->row_cur[j][r].sym = 0;
        blk->row_cur[j][r].match[5] = 1;
        blk->row_cur[j][r].match[4] = 1;
        blk->row_cur[j][r].match[3] = 1;
        blk->row_cur[j][r].match[2] = 1;
        blk->row_cur[j][r].match[1] = 1;
        blk->row_cur[j][r].match[0] = 1;
        w = blk->width;
        ++r;
      }
      while ( (uint32_t)r < blk->width + 16 );
    }
  }
  runs = (uint8_t *)bmf_new(w + 1);
  blk->run_bucket = runs;
  *runs = 0;
  if ( (int32_t)blk->width > 0 )
  {
    bucket = 0;
    x = 0;
    do
    {
      bucket += x == 2 << (bucket & 31);
      *(uint8_t *)(*(uint32_t *)&blk->run_bucket + x++ + 1) = bucket;
    }
    while ( (uint32_t)x < blk->width );
  }
  // 0x2000 sixteen-bit counters cleared.  What was here instead was the same
  // range in three passes -- a scalar head to reach sixteen-byte alignment,
  // thirty-two counters an iteration, a scalar tail -- with a branch for the
  // case where `blk` is odd and no alignment is reachable at all.
  __builtin_memset(blk->sym_rev, 0, sizeof blk->sym_rev);
  for ( k = 0; k < 0x2000; ++k )
  {
    rev = blk->sym_rev[k];
    bits = k;
    for ( m = 0; m < 0xD; ++m )
    {
      rev += rev + (bits & 1);
      bits >>= 1;
    }
    blk->sym_rev[k] = rev;
  }
  // ... and every one of them scaled by eight, the same range in the same
  // three passes.
  for ( s = 0; s < 0x2000; ++s )
    blk->sym_rev[s] *= 8;
  memset(&blk->grid[188],0,0x100000);
  blk->ctx_id3_used = 0;
  blk->ctx_id2_used = 0;
  blk->ctx_id1_used = 0;
  memset(blk->ctx_id1,255,sizeof blk->ctx_id1);
  memset(blk->ctx_id2,255,sizeof blk->ctx_id2);
  memset(blk->ctx_id3,255,sizeof blk->ctx_id3);
  memset(exclusion_mask,0,8193);
  (*(uint64_t *)&blk->sel[0]) = 0;
  *(uint64_t *)&blk->escape_list = 0;
  for ( n = 0; n < 0x40000; ++n )
  {
    blk->sym_ctr[2 * n] = 0x2000;
    blk->sym_ctr[2 * n + 1] = 0x2000;
  }
  i8 = 0;
  do
  {
    e0 = 2 * i8;   // two records a pass
    blk->bit_root[e0].n[0] = 40;
    ++i8;
    blk->bit_root[e0].n[1] = 16;
    blk->bit_root[e0].limit = 512;
    blk->bit_root[e0 + 1].n[0] = 40;
    blk->bit_root[e0 + 1].n[1] = 16;
    blk->bit_root[e0 + 1].limit = 512;
  }
  while ( i8 < 8 );
  i24 = 0;
  memset(blk->bit_node,0,sizeof blk->bit_node);
  blk->sym_word = (uint16_t *)bmf_new(2 * blk->height * blk->width);
  blk->esc_ctr[0].n[0] = 4;
  blk->esc_ctr[0].n[1] = 4;
  blk->esc_ctr[0].limit = 72;
  memset(&blk->esc_ctr[1],0,1536);
  do
  {
    e1 = 2 * i24;   // two records a pass
    blk->run_ctr[e1].n[0] = 4;
    ++i24;
    blk->run_ctr[e1].n[1] = 4;
    blk->run_ctr[e1].limit = 72;
    blk->run_ctr[e1 + 1].n[0] = 4;
    blk->run_ctr[e1 + 1].n[1] = 4;
    blk->run_ctr[e1 + 1].limit = 72;
  }
  while ( i24 < 0x18 );
  return blk;
}

inline void ModelBlock::unmodel_plane_slow(uint8_t *dst)
{
  uint8_t *row[19];
  uint32_t alpha_n;
  ModelBlock *this_1, *blk;
  int16_t lvl;
  uint32_t has4, jj, wt, nbytes, row_w, *out32;
  uint16_t *out16;
  SymEntry *out_ent;
  uint8_t *out_at, *dst_keep;
  int32_t g0, y0;
  int32_t done, f_b0, f_b3, f_b4, f_b5;
  int32_t f_b1, lo1, m5, g1;
  // These shared `g0` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `v85` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These ten shared `ArgList`: one stack slot MSVC gave to locals
  // whose live ranges do not overlap, and Hex-Rays named every use after it.
  // That they can have storage of their own is the gate's answer -- nothing
  // writes one of them and reads another.
  
  // What they are is one output cursor at the width the plane's depth calls
  // for.  `unmodel_plane_slow` writes a reconstructed plane at four, three,
  // two or one bytes a pixel or packed below a byte, and each width walks the
  // same buffer through its own type: `out32`, `out_ent`, `out16`, `out8`,
  // `out_bits`.  `out_at` is where the next one starts and where the last one
  // left off, which is why every branch begins by casting it and ends by
  // casting back.
  ;
  uint32_t *x6;   // the alphabet map again
  uint8_t *dst_base, *dst_buf, *buf, *expand_buf, *out_bits, *row_at,
          *interleave_at, *p, *out8;
  PixRec *kk, *row_cur3, *row_cur2, *row_cur1, *n_syms;   // the five row buffers, rotated
  int32_t g, flags, lo, k, w2, w4, w2n, lvl_n, live, gi, s,
          bucket, x, x2, y, step, x7, x3, x4, bits, depth,
          y2, depth_raw, nchunk, n_pix, chunk, q5, q1, at, q2, written, n_pix2, x5;
  // `out32` was one register carrying two cursors: four bytes per pixel in
  // the 32-bit branch and two in the 16-bit one, which is why every store
  // through it in the second loop had to cast the width back down.  The two
  // branches shared the two-line tail that puts the cursor back, and the goto
  // into the middle of the second block was how they shared it -- the only
  // jump into a block left in the file.  Duplicating two lines separates the
  // lifetimes, and the second one can then say what it points at.
  SymListBlock *has3, *alpha;
  SymPair *group_ctr;   // one group's row of counter pairs
  SymList *list_a, *i, *list_b, *j;
  FreqRec *rec;   // a bucket record: `grid[bucket]`
  PixRec *lists, *t;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  dst_keep = dst;
  // One byte back when the depth is sub-byte, because `out_bits` writes with
  // `*++out_bits` and so starts one before the first byte it fills.
  dst_base = &dst[-(this->depth < 8)];
  __rc_begin_decode(0);
  (this)->expand_alphabet();
  this_1 = (this);
  g1 = 0;
  g = 0;
  do
  {
    flags = ctx_group_flags[g];
    this_1->ctx_state[flags] = g;
    lo1 = 0;
    g0 = g;

    y0 = flags & 4;
    // Six bits of `flags`, each gating one "fold this weight into that one"
    // step, and each named for the bit it tests.  Hex-Rays called them
    // `m1`..`m4` here and `m1`..`m6` in `model_plane`, which is the encoder's
    // mirror of this loop -- and the two numberings disagree: `m1` was bit 0
    // here and bit 3 there, `m2` the other way round.  Two mirror functions
    // whose masks swap names is the same defect as one address written two
    // ways, and the bit is the one name both can be checked against.
    f_b1 = flags & 2;
    f_b4 = flags & 0x10;
    lo = 0;
    f_b0 = flags & 1;

    f_b5 = flags & 0x20;
    f_b3 = flags & 8;
    do
    {
      lo1 = lo;
      k = 0;

      do
      {
        this_1->ctx_bucket[g + 15 * lo + 75 * k] = g1;
        m5 = this_1->alphabet;
        // A sixteen-byte record per bucket, at `+96 + 16 * bucket`: five
        // counts at words 0..4, their total at 5, a scaled weight at 6, and
        // two bytes at 7 -- a level (`<= f16`) and the weight `1 << (5 -
        // level)` it derives.  The base is the object and the counter starts
        // at zero, so record 0 is +96 .. +111 -- which `row_cur[10..13]` also
        // claims, and which the `f1051664[k] = row_cur[10 + k]` copy after this
        // loop reads back.  That copy is dead -- nothing reads `f1051664` --
        // so the collision costs nothing at run time, but it does mean one
        // of `row_cur`'s length and `f1051664`'s type is wrong.
        
        // The bucket counter reaches exactly 188: a `__builtin_trap()` on
        // `>= 188` fires on fourteen of the gate's streams and one on
        // `>= 189` fires on none.  So this table is 189 records, +96 ..
        // +3119.  `FreqRec` is on the same grid from record 188 (+3104),
        // which makes the last bucket record and the first frequency record
        // the same sixteen bytes -- the two tables abut and share one.
        rec = &this_1->grid[g1];
        rec->w[1] = 2;
        rec->w[2] = 2;
        rec->w[3] = 2;
        rec->w[4] = 2;
        if ( y0 )
        {
          w2 = (uint16_t)(rec->w[4] + rec->w[3]);
          rec->w[3] = w2;
          w4 = 0;
          rec->w[4] = 0;
        }
        else
        {
          w2 = rec->w[3];
          w4 = rec->w[4];
        }
        if ( f_b1 )
        {
          w2n = (uint16_t)(w4 + rec->w[2]);
          rec->w[2] = w2n;
          w4 = 0;
          rec->w[4] = 0;
        }
        else
        {
          w2n = rec->w[2];
        }
        if ( f_b4 )
        {
          w2n = (uint16_t)(w2 + w2n);
          rec->w[2] = w2n;
          w2 = 0;
          rec->w[3] = 0;
        }
        if ( f_b0 )
        {
          rec->w[1] += w4;
          w4 = 0;
          rec->w[4] = 0;
        }
        if ( f_b3 )
        {
          rec->w[1] += w2;
          w2 = 0;
          rec->w[3] = 0;
        }
        if ( f_b5 )
        {
          rec->w[1] += w2n;
          w2n = 0;
          rec->w[2] = 0;
        }
        lvl_n = (w2 != 0) + (w2n != 0) + (w4 != 0) + 2;
        if ( lvl_n <= m5 )
        {
          rec->b14 = lvl_n;
          rec->w[0] = 2;
        }
        else
        {
          LOBYTE(lvl_n) = lvl_n - 1;
          rec->b14 = lvl_n;
          rec->w[0] = 0;
        }
        if ( rec->w[lo1] && rec->w[k] && (uint8_t)lvl_n <= m5 )
        {
          live = 1;
          lvl = (uint8_t)(1 << ((5 - lvl_n) & 31));
          rec->b15 = lvl;
          rec->w[6] = lvl << 6;
          rec->w[lo1] += lvl;
          rec->w[k] += rec->b15;
          rec->w[5] = rec->w[0]
                               + rec->w[4]
                               + rec->w[3]
                               + rec->w[2]
                               + rec->w[1];
        }
        else
        {
          live = 0;
        }
        g1 += live;
        ++k;
      }
      while ( k < 5 );
      lo = lo1 + 1;
    }
    while ( lo1 + 1 < 5 );
    gi = 0;
    group_ctr = this_1->group_ctr[g0];
    do
    {
      group_ctr[gi].last = 0x2000;
      group_ctr[gi++].prev = 0x2000;
    }
    while ( gi < 0x10000 );
    g = g0 + 1;
  }
  while ( g0 + 1 < 15 );
  dst_buf = dst_base;
  blk = (ModelBlock *)((int32_t)this_1);
  buf = (uint8_t *)bmf_new(this_1->alphabet);
  alpha_n = this_1->alphabet;
  this_1->alpha_map = (uint8_t *)buf;
  memset(buf,1,alpha_n);
  blk->escape_list = &blk->escape;
  __init_symbol_list(&blk->escape, (int32_t)blk, blk->alphabet, 1);
  blk->sel_cur = blk->sel;
  // `24 * n + 4`: the count word, then `n` lists.  `free_workspace` reads the
  // count back from `sym_list_count(lists)`.
  wt = blk->alphabet;
  has3 = (SymListBlock *)bmf_new(24 * wt + 4);
  if ( has3 )
  {
    has3->n = wt;
    list_a = has3->list;
    for ( i = list_a; wt; --wt )
      (list_a++)->ent = nullptr;
  }
  else
  {
    i = nullptr;
  }
  has4 = blk->alphabet;
  blk->sel1_list = i;
  alpha = (SymListBlock *)bmf_new(24 * has4 + 4);
  if ( alpha )
  {
    alpha->n = has4;
    list_b = alpha->list;
    for ( j = list_b; has4; --has4 )
      (list_b++)->ent = nullptr;
  }
  else
  {
    j = nullptr;
  }
  blk->sel0_list = j;
  if ( !(blk->alphabet <= 0) )
  {
    s = 0;
    do
    {
      __init_symbol_list(&blk->sel1_list[s], (int32_t)blk, 99, 0);
      __init_symbol_list(&blk->sel0_list[s++], (int32_t)blk, 33, 0);
    }
    while ( (uint32_t)s < blk->alphabet );
  }
  jj = blk->depth;
  if ( (uint32_t)jj == blk->depth_raw )
  {
    expand_buf = nullptr;
  }
  else
  {
    dst_buf = (uint8_t *)bmf_new(*(uint32_t *)&blk->height * blk->width + 3);
    jj = blk->depth;
    expand_buf = dst_buf;
  }
  nbytes = (jj + 7) >> 3;
  if ( blk->height > 0 )
  {
    out_at = dst_buf;
    bucket = 0;
    while ( 1 )
    {
      blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym == 0;
      blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym == 0;
      kk = blk->row_cur[4];
      row_cur3 = blk->row_cur[3];
      row_cur2 = blk->row_cur[2];
      row_cur1 = blk->row_cur[1];
      n_syms = blk->row_cur[0];
      blk->row_cur[4] = row_cur3;
      blk->row_cur[3] = row_cur2;
      blk->row_cur[2] = row_cur1;
      blk->row_cur[1] = n_syms;
      blk->row_cur[0] = kk;
      kk += 7;
      blk->row_cur[5] = kk;
      n_syms += 7;
      blk->row_cur[6] = n_syms;
      blk->row_cur[7] = row_cur1 + 7;
      blk->row_cur[8] = row_cur2 + 7;
      blk->row_cur[9] = row_cur3 + 7;
      // Two "is this count zero" flags, written to three and two places.
      // MSVC put each in the low byte of a register that held a cursor, which
      // is where `LOBYTE(n_syms) = ...` came from; neither cursor is read again.
      {
        uint8_t zero = n_syms[1].sym == 0;
        kk->match[2] = zero;
        blk->row_cur[5][-1].match[4] = zero;
        blk->row_cur[5][-2].match[5] = zero;
        zero = blk->row_cur[6][2].sym == 0;
        blk->row_cur[5]->match[4] = zero;
        blk->row_cur[5][-1].match[5] = zero;
      }
      blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym == 0;
      lists = blk->row_cur[6];
      t = blk->row_cur[7];
      ++blk->row_cur[5];
      ++lists;
      ++blk->row_cur[8];
      blk->row_cur[6] = lists;
      ++t;
      blk->row_cur[7] = t;
      ++blk->row_cur[9];
      blk->grad[0] = lists[3].match[0] + lists[2].match[0] + lists[1].match[0] + lists[0].match[0] + lists[4].match[0] - 5;
      // The same five counts as the line above, off the other row.  MSVC
      // spilled each byte into a register whose upper bits were leftovers,
      // and the destination is one byte, so only the low bytes ever counted.
      blk->grad[3] = 0;
      blk->grad[2] = 0;
      row_w = blk->width;
      blk->grad[1] = t[3].match[0] + t[2].match[0] + t[1].match[0] + t[0].match[0] + t[4].match[0] - 5;
      x = bucket;
      if ( row_w <= 0 )
        break;
      x2 = 0;
      do
      {
        y = ((ModelBlock *)blk)->decode_pixel(x2);
        blk->init_tables();
        row_w = blk->width;
        x2 += y;
      }
      while ( (uint32_t)x2 < blk->width );
      x = bucket;
      if ( nbytes != 4 )
        goto LABEL_53;
      if ( row_w > 0 )
      {
        out32 = (uint32_t *)out_at;
        y2 = 0;
        do
          *out32++ = blk->sym_code[blk->row_cur[0][y2++ + 8].sym];
        while ( (uint32_t)y2 < blk->width );
        out_at = (uint8_t *)out32;
        goto LABEL_74;
      }
LABEL_74:
      bucket = x + 1;
      if ( (uint32_t)bucket >= *(uint32_t *)&blk->height )
        goto LABEL_76;
    }
    if ( nbytes == 4 )
      goto LABEL_74;
LABEL_53:
    if ( nbytes == 3 )
    {
      if ( row_w > 0 )
      {
        // Three bytes out of each 24-bit code: the low half and byte 2, which
        // is a `SymEntry` -- the same three-byte pair the symbol lists hold.
        out_ent = (SymEntry *)out_at;
        step = 0;
        do
        {
          x6 = blk->sym_code;
          x7 = blk->row_cur[0][step + 8].sym;
          out_ent->set((uint16_t)x6[x7], (uint8_t)(x6[x7] >> 16));
          ++step;
          ++out_ent;
        }
        while ( (uint32_t)step < blk->width );
        out_at = (uint8_t *)out_ent;
      }
      goto LABEL_74;
    }
    if ( nbytes != 2 )
    {
      if ( blk->depth == 8 )
      {
        if ( row_w > 0 )
        {
          out8 = out_at;
          x5 = 0;
          do
            // A byte, not a word: this branch is the 8-bits-per-pixel one, and
            // `sym_code` was a `uint8_t *` when this dereference was written.
            *out8++ = (uint8_t)blk->sym_code[blk->row_cur[0][x5++ + 8].sym];
          while ( (uint32_t)x5 < blk->width );
          out_at = out8;
        }
      }
      else if ( row_w > 0 )
      {
        x4 = 0;
        bits = 0;
        out_bits = out_at;
        do
        {
          depth = blk->depth;
          bits -= depth;
          if ( bits < 0 )
          {
            bits = 8 - depth;
            *++out_bits = blk->sym_code[blk->row_cur[0][x4 + 8].sym] << ((8 - depth) & 31);
          }
          else
          {
            *out_bits |= blk->sym_code[blk->row_cur[0][x4 + 8].sym] << (bits & 31);
          }
          ++x4;
        }
        while ( (uint32_t)x4 < blk->width );
        out_at = out_bits;
      }
      goto LABEL_74;
    }
    if ( row_w > 0 )
    {
      out16 = (uint16_t *)out_at;
      x3 = 0;
      do
        *out16++ = blk->sym_code[blk->row_cur[0][x3++ + 8].sym];
      while ( (uint32_t)x3 < blk->width );
      out_at = (uint8_t *)out16;
    }
    goto LABEL_74;
  }
LABEL_76:
  __rc_end_decode();
  depth_raw = blk->depth_raw;
  if ( (uint32_t)depth_raw != blk->depth )
  {
    nchunk = (depth_raw + 7) >> 3;
    if ( nchunk <= 0 )
    {
      n_pix = *(uint32_t *)&blk->height * blk->width;
    }
    else
    {
      done = 0;
      n_pix = *(uint32_t *)&blk->height * blk->width;
      chunk = n_pix / nchunk;
      if ( nchunk >= 6 )
      {
        q5 = 0;
        row_at = expand_buf;
        do
        {
          row[q5] = row_at;
          row_at += 5 * chunk;
          row[q5 + 1] = &expand_buf[chunk * (q5 + 1)];
          row[q5 + 2] = &expand_buf[chunk * (q5 + 2)];
          row[q5 + 3] = &expand_buf[chunk * (q5 + 3)];
          row[q5 + 4] = &expand_buf[chunk * (q5 + 4)];
          q5 += 5;
        }
        while ( q5 <= nchunk - 6 );
        done = q5;
      }
      q1 = done;
      at = chunk * done;
      do
      {
        row[q1] = &expand_buf[at];
        at += chunk;
        ++q1;
      }
      while ( q1 < nchunk );
    }
    if ( n_pix > 0 )
    {
      interleave_at = dst_keep;
      q2 = 0;
      written = 0;
      do
      {
        p = row[q2];
        *interleave_at = *p;
        n_pix2 = *(uint32_t *)&blk->height * blk->width;
        ++interleave_at;
        row[q2++] = p + 1;
        if ( q2 == nchunk )
          q2 = 0;
        ++written;
      }
      while ( written < n_pix2 );
    }
    free(expand_buf);
  }
}

int32_t __alt_model_p1_encode(uint16_t *hdr, uint8_t *src)
{
  P1Ctx *b4, *buf3, *buf2, *buf1, *b0, *cursor0;
  int8_t dc3;
  uint8_t out3;
  int32_t resid3, cur3;
  void *plane[4];
  // These shared `m1` and `m2` with the names that still
  // bind them: one stack slot MSVC gave to locals whose live ranges do not
  // overlap, and Hex-Rays named every use.  That they can have storage of
  // their own is the gate's answer -- nothing writes one of them and reads
  // another.
  AltP1Block * &plane1 = (AltP1Block * &)plane[1];
  AltP1Block * &plane2 = (AltP1Block * &)plane[2];
  AltP1Block * &plane3 = (AltP1Block * &)plane[3];
  int32_t dc1;
  uint32_t x, y;
  AltP1Block *blk0;
  void **q;
  int32_t out1, resid0, off1, off0, resid2, cur0, off2, cur1, code1;
  ;
  uintptr_t off3;   // were int32_t: addresses, masked and tagged
  AltP1Block *blk_k, *raw;
  uint8_t fl1, fl2, fl3, pred1, pred2, pred3;
  int32_t width, height, k, *made, src1, src2, src3, dc2, xf1, xf2, xf3, w, n_planes, p,
          want0, keep0, code0, recon0, out0, drift0, err0, resid1, at1, recon1, drift1, code1x,
          want2, code2, recon2, out2, drift2, at2, code3, recon3, drift3, np, f;
  int64_t err1, err2, err3;
  AltP1Block *blk1, *blk2, *blk3;
  P1Ctx *cursor2, *cursor4;
  uint8_t pred0, want1, want3;
  width = *hdr;
  height = hdr[1];
  if ( plane_count > 0 )
  {
    k = 0;
    do
    {
      raw = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( raw )
        made = __alt_p1_alloc((AltP1Block *)raw, width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    }
    while ( k < plane_count );
  }
  // The decoder's block, mirrored.  `src3` is read for its record before its
  // low byte is overwritten with that record's b3, and `dc3` -- a char -- is
  // the only thing that reads it after, so the byte is all of it.
  src1 = plane_desc[2].src_plane;
  src2 = plane_desc[3].src_plane;
  src3 = plane_desc[4].src_plane;
  fl1 = plane_desc[src1 + 1].flags;
  fl2 = plane_desc[src2 + 1].flags;
  fl3 = plane_desc[src3 + 1].flags;
  dc2 = plane_desc[src2 + 1].b3;
  LOBYTE(src3) = plane_desc[src3 + 1].b3;
  dc1 = plane_desc[src1 + 1].b3;
  xf1 = fl1 & 8;
  dc3 = src3;
  xf2 = fl2 & 8;
  xf3 = fl3 & 8;
  __rc_begin_encode();
  if ( height > 0 )
  {
    y = 0;
    w = width;
    n_planes = plane_count;
    do
    {
      if ( n_planes > 0 )
      {
        p = 0;
        do
        {
          ++p;
          blk_k = (AltP1Block *)plane[p - 1];
          // The row end mirrored into the right margin: records 0..5 take -1..-6.
          // Two bytes each, which is what the twelve even offsets were.
          {
            P1Ctx *const here = (P1Ctx *)blk_k->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          b4 = blk_k->buf[4];
          buf3 = blk_k->buf[3];
          buf2 = blk_k->buf[2];
          buf1 = blk_k->buf[1];
          b0 = blk_k->buf[0];
          blk_k->buf[4] = buf3;
          blk_k->buf[3] = buf2;
          blk_k->buf[2] = buf1;
          blk_k->buf[1] = b0;
          blk_k->buf[0] = b4;
          b4 += 4;
          blk_k->cursor[0] = b4;
          b0 += 4;
          blk_k->cursor[1] = b0;
          blk_k->cursor[2] = buf1 + 4;
          blk_k->cursor[3] = buf2 + 4;
          blk_k->cursor[4] = buf3 + 4;
          ((P1Ctx *)b4)[-4] = ((P1Ctx *)b0)[3];
          // And the rest of the new row's left margin, from the row above.
          {
            P1Ctx *const here = (P1Ctx *)blk_k->cursor[0];
            P1Ctx *const up   = (P1Ctx *)blk_k->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          cursor2 = blk_k->cursor[2];
          cursor4 = blk_k->cursor[4];
          cursor0 = blk_k->cursor[0];
          blk_k->ctx[2] = 0;
          // `alt_model_p1_decode`'s block, term for term and cast for cast --
          // see the note there for why the four `(int8_t)`s stay.
          blk_k->ctx[3] = (int8_t)cursor2[-2].mag + (int8_t)cursor4[-2].mag
                      + cursor2[0].mag  + cursor4[0].mag
                      + cursor2[2].mag  + cursor4[2].mag
                      + cursor2[4].mag  + cursor4[4].mag
                      + cursor0[-4].mag + cursor0[-2].mag;
          blk_k->ctx[4] = (int8_t)cursor2[-1].mag + (int8_t)cursor4[-1].mag
                      + cursor2[1].mag  + cursor4[1].mag
                      + cursor2[3].mag  + cursor4[3].mag
                      + cursor2[5].mag  + cursor4[5].mag
                      + cursor0[-3].mag + cursor0[-1].mag;
          n_planes = plane_count;
        }
        while ( p < plane_count );
        w = width;
      }
      if ( w > 0 )
      {
        x = 0;
        do
        {
          blk0 = (AltP1Block *)plane[0];
          want0 = *(src + plane_desc[1].src_plane);
          off0 = plane_desc[1].src_plane;
          cur0 = want0;
          ((AltP1Block *)plane[0])->ctx_of((AltP1Block *)nullptr, (AltP1Block *)0);
          keep0 = cur0;
          pred0 = (uint8_t)blk0->pred;
          resid0 = (uint8_t)(cur0 - pred0);
          code0 = blk0->fold[resid0];
          recon0 = (uint8_t)(blk0->unfold[code0] + pred0);
          out0 = (uint8_t)(recon0 + *(src + off0) - cur0);
          drift0 = *(src + off0) - out0;
          if ( drift0 < -16 || drift0 > 16 )
          {
            code0 = blk0->fold_hi[resid0];
          }
          else
          {
            cur0 = recon0;
            *(src + off0) = out0;
            keep0 = recon0;
          }
          __alt_p1_encode_symbol(&blk0->counters[blk0->ctx[0]].total, recon0, blk0->ctx[1], code0);
          err0 = keep0 - blk0->pred;
          blk0->cursor[0]->sym = cur0;
          blk0->cursor[0]->mag = abs32(err0);
          blk0->ctx[blk0->ctx[2] + 3] = blk0->ctx[blk0->ctx[2] + 3]
                                                         + blk0->cursor[0]->mag
                                                         - blk0->cursor[0][-4].mag
                                                         - (blk0->cursor[4][-2].mag
                                                          - blk0->cursor[4][6].mag
                                                          + blk0->cursor[2][-2].mag
                                                          - blk0->cursor[2][6].mag);
          blk0->ctx[2] = blk0->ctx[2] == 0;
          if ( blk0->counters[blk0->ctx[0]].total < 0x4000u )
            blk0->update_model();
          ++blk0->cursor[0];
          ++blk0->cursor[1];
          ++blk0->cursor[2];
          ++blk0->cursor[3];
          ++blk0->cursor[4];
          want1 = *(src + plane_desc[2].src_plane);
          off1 = plane_desc[2].src_plane;
          if ( xf1 )
            want1 = want1 - dc1 - *(plane_desc[1].src_plane + src);
          blk1 = (AltP1Block *)(plane1);
          cur1 = want1;
          ((AltP1Block *)plane1)->ctx_of((AltP1Block *)plane[0], (AltP1Block *)0);
          pred1 = (uint8_t)blk1->pred;
          resid1 = (uint8_t)(cur1 - pred1);
          code1 = blk1->fold[resid1];
          at1 = *(src + off1);
          recon1 = (uint8_t)(blk1->unfold[code1] + pred1);
          out1 = (uint8_t)(recon1 + *(src + off1) - cur1);
          drift1 = at1 - out1;
          code1x = code1;
          if ( drift1 < -16 || drift1 > 16 )
          {
            code1x = blk1->fold_hi[resid1];
          }
          else
          {
            *(src + off1) = out1;
            cur1 = recon1;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)blk1)[4 * blk1->ctx[0] + 950], 16 * blk1->ctx[0], (int32_t)blk1->ctx[1], code1x);
          err1 = cur1 - blk1->pred;
          blk1->cursor[0]->sym = cur1;
          blk1->cursor[0]->mag = (BYTE4(err1) ^ err1) - BYTE4(err1);
          ((uint8_t**)blk1)[blk1->ctx[2] + 6] = &((uint8_t**)blk1)[blk1->ctx[2] + 6][blk1->cursor[0]->mag
                                                           - blk1->cursor[0][-4].mag
                                                           - (blk1->cursor[4][-2].mag
                                                            - blk1->cursor[4][6].mag)
                                                           - (blk1->cursor[2][-2].mag
                                                            - blk1->cursor[2][6].mag)];
          blk1->ctx[2] = blk1->ctx[2] == 0;
          if ( blk1->counters[blk1->ctx[0]].total < 0x4000u )
            ((AltP1Block *)blk1)->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          want2 = *(plane_desc[3].src_plane + src);
          off2 = plane_desc[3].src_plane;
          if ( xf2 )
            want2 = (uint8_t)(want2
                                  - dc2
                                  - ((plane_desc[plane_desc[3].src_plane + 1].w4
                                    * *(plane_desc[1].src_plane + src)
                                    + plane_desc[plane_desc[3].src_plane + 1].w8
                                    * (uint32_t)*(plane_desc[2].src_plane + src)
                                    + 40) >> 7));
          blk2 = (AltP1Block *)(plane2);
          ((AltP1Block *)plane2)->ctx_of((AltP1Block *)plane1, (AltP1Block *)(int32_t)plane[0]);
          pred2 = (uint8_t)blk2->pred;
          resid2 = (uint8_t)(want2 - pred2);
          code2 = blk2->fold[resid2];
          recon2 = (uint8_t)(blk2->unfold[code2] + pred2);
          out2 = (uint8_t)(recon2 + *(off2 + src) - want2);
          drift2 = *(off2 + src) - out2;
          if ( drift2 < -16 || drift2 > 16 )
          {
            code2 = blk2->fold_hi[resid2];
          }
          else
          {
            want2 = recon2;
            *(off2 + src) = out2;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)blk2)[4 * blk2->ctx[0] + 950], code2, (int32_t)blk2->ctx[1], code2);
          err2 = want2 - blk2->pred;
          blk2->cursor[0]->sym = want2;
          blk2->cursor[0]->mag = (BYTE4(err2) ^ err2) - BYTE4(err2);
          ((uint8_t**)blk2)[blk2->ctx[2] + 6] = &((uint8_t**)blk2)[blk2->ctx[2] + 6][blk2->cursor[0]->mag
                                                           - blk2->cursor[0][-4].mag
                                                           - (blk2->cursor[4][-2].mag
                                                            - blk2->cursor[4][6].mag)
                                                           - (blk2->cursor[2][-2].mag
                                                            - blk2->cursor[2][6].mag)];
          at2 = 4 * blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2] == 0;
          if ( LOWORD(((uint8_t**)blk2)[at2 + 950]) < 0x4000u )
            ((AltP1Block *)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          n_planes = plane_count;
          if ( plane_count >= 4 )
          {
            off3 = plane_desc[4].src_plane;
            if ( xf3 )
              want3 = *(plane_desc[4].src_plane + src)
                  - dc3
                  - ((plane_desc[plane_desc[4].src_plane + 1].w8 * *(plane_desc[4].src_plane + src - 2)
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *(plane_desc[4].src_plane + src - 3)
                    + plane_desc[plane_desc[4].src_plane + 1].w12 * *(plane_desc[4].src_plane + src - 1)
                    + 64) >> 7);
            else
              want3 = *(plane_desc[4].src_plane + src);
            blk3 = (AltP1Block *)(plane3);
            cur3 = want3;
            ((AltP1Block *)plane3)->ctx_of((AltP1Block *)plane2, (AltP1Block *)(int32_t)plane1);
            pred3 = (uint8_t)blk3->pred;
            code3 = blk3->fold[(uint8_t)(cur3 - pred3)];
            resid3 = (uint8_t)(cur3 - pred3);
            recon3 = (uint8_t)(blk3->unfold[code3] + pred3);
            drift3 = *(off3 + src) - (uint8_t)(recon3 + *(off3 + src) - cur3);
            out3 = recon3 + *(off3 + src) - cur3;
            if ( drift3 < -16 || drift3 > 16 )
            {
              code3 = blk3->fold_hi[resid3];
            }
            else
            {
              cur3 = recon3;
              *(off3 + src) = out3;
            }
            __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)blk3)[4 * blk3->ctx[0] + 950], code3, (int32_t)blk3->ctx[1], code3);
            err3 = cur3 - blk3->pred;
            blk3->cursor[0]->sym = cur3;
            blk3->cursor[0]->mag = (BYTE4(err3) ^ err3) - BYTE4(err3);
            ((uint8_t**)blk3)[blk3->ctx[2] + 6] = &((uint8_t**)blk3)[blk3->ctx[2] + 6][blk3->cursor[0]->mag
                                                             - blk3->cursor[0][-4].mag
                                                             - (blk3->cursor[4][-2].mag
                                                              - blk3->cursor[4][6].mag)
                                                             - (blk3->cursor[2][-2].mag
                                                              - blk3->cursor[2][6].mag)];
            blk3->ctx[2] = blk3->ctx[2] == 0;
            if ( blk3->counters[blk3->ctx[0]].total < 0x4000u )
              ((AltP1Block *)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            n_planes = plane_count;
          }
          src += n_planes;
          ++x;
        }
        while ( x < (uint32_t)width );
        w = width;
      }
      ++y;
    }
    while ( y < (uint32_t)height );
  }
  __rc_end_encode();
  np = plane_count;
  if ( plane_count > 0 )
  {
    f = 0;
    do
    {
      q = (void **)plane[f];
      if ( q )
      {
        __alt_p1_free((void **)q, 1);
        np = plane_count;
      }
      ++f;
    }
    while ( f < np );
  }
  return np;
}

uint32_t __alt_p2_model(AltP2Block *blk, int32_t sample_in, uint8_t a4, int32_t resid_in)
{
  P2Ctx *unused_c, *cursor0;
  float    n2_bias;   // the p2 filter's bias term, one of three lifetimes MSVC
  int32_t  n2_half;   // gave one slot; the third is the cursor below
  // Lanes of the counter table.  Every read through this is a `uint16_t`
  // except the two shift counts at +-4, which are the low byte of the
  // neighbouring record; the byte offsets it carried were all even.
  uint16_t *mir_top;
  P2Freq *grp;
  uint32_t rec_idx;
  int32_t hi_nibble;
  P2Freq *p2_rec;
  uintptr_t pair_ctx;
  uint32_t step_s, bank_off2, ctxw_s;
  int32_t sample16;
  uint32_t bank;
  int32_t countdown;
  P2Count *node0;
  int32_t fold_sel2, is_dec;
  P2Count *m0080, *r0400, *r0800, *m0800, *d1000, *r1000, *m1000, *d2000, *r2000, *m2000, *d4000,
          *r4000, *m4000;
  int32_t lowbits, nres1, nres2, nres3, res_c;
  P2Count *d0800, *d0400, *m0400, *d0200, *m0200, *d0100, *m0100, *r0100, *d0080, *r0080, *d0040,
          *m0040, *d0020, *m0020, *r0020, *d0010, *m0010;
  int32_t nres4, nres5;
  // Two more things MSVC kept in the register `grp` names.
  uint32_t idx0;   // a record index in four regions ...
  // Two things in one slot, and both are read as numbers: the strip index
  // out of `ctx_pair[0]` or `ctx_pair[1]`, and later an address.  It is neither a
  // `uint16_t *` nor an index -- it is the register MSVC put both in.
  // The counter this pass updates and its two neighbours: `node0[-1]`,
  // `node0[0]` and `node0[1]` are +284 708, +284 712 and +284 716 off the
  // row base, which is `p2_ctr` reached through `bank_off`.
  ;
  uint16_t *mir_top2;   // a second name for `mir_top`
  // The parameter this used to copy is never read: `sample` is written from
  // `sample16` below before any of its four readers, and lanes 1..3 were never
  // touched at all.  It is XMM0 being reused as a scratch register, which is
  // what MSVC did and what Hex-Rays recorded.  `d_bias2` is the same, one lane.
  float sample, d_bias2;
  uint8_t b_top3, b0n;
  P2Freq *frecg2, *frecg1, *frecg0, *frecg3, *gtop, *frec;
  P2Count *r0200, *r0040, *r0010;
  uint32_t off0, off1, off2, off3, off4, ctxw;
  float (*f278656)[4];
  float (*wrow_b)[4];
  bool go, eq_hi, lt_hi, ovf;
  uint8_t *bankp;   // `uint8_t *` beside the `char` scalars above
  float ms1, bias2, bias1, d_bias, ms_a, ms_b, ms_b10, conf;
  int32_t ctx_lo, bank_ctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, x0020, x0010,
          neg, w_top, e_top, bump, res_s, w_top3, w_topm1, wd4000a, ed4000a, wr4000b, er4000b,
          wd4000m1b, ed4000m1b, wm4000c, em4000c, wd2000c, ed2000c, wr2000c, er2000c, wd2000m1b, ed2000m1b, wm2000c,
          em2000c, wd1000c, ed1000c, wr1000c, er1000c, wd1000m1b, ed1000m1b, wm1000c, em1000c, wd0800c, ed0800c,
          wr0800c, er0800c, wd0800m1b, ed0800m1b, wm0800c, em0800c, wd0400c, ed0400c, wr0400c, er0400c, wd0400m1b,
          ed0400m1b, wm0400x, wd0200c, ed0200c, wr0200c, er0200c, wd0200m1b, ed0200m1b, neg_b, wm0200c, em0200c,
          wd0100c, ed0100c, wr0100c, er0100c, wd0100m1b, ed0100m1b, wm0100c, em0100c, wd0080c, ed0080c, wr0080c,
          er0080c, wd0080m1b, ed0080m1b, wm0080c, em0080c, wd0040c, ed0040c, wr0040c, er0040c, wd0040m1b, ed0040m1b,
          wm0040c, em0040c, wd0020c, res_t, ed0020c, wr0020c, er0020c, wd0020m1b, ed0020m1b, wm0020c, em0020c,
          wd0010c, ed0010c, wr0010c, er0010c, wd0010m1b, ed0010m1b, wm0010b, em0010b, wd4000m1a, ed4000m1a, neg_c,
          wm4000b, em4000b, wd2000b, ed2000b, wr2000b, er2000b, wd2000m1a, ed2000m1a, wm2000b, em2000b, wd1000b,
          ed1000b, wr1000b, er1000b, wd1000m1a, ed1000m1a, wm1000b, em1000b, wd0800b, ed0800b, wr0800b, er0800b,
          wd0800m1a, ed0800m1a, wm0800b, em0800b, wd0400b, ed0400b, wr0400b, er0400b, wd0400m1a, ed0400m1a, wm0400y,
          nb_slot, wd0200b, ed0200b, wr0200b, er0200b, wd0200m1a, ed0200m1a, wm0200b, em0200b, wd0100b, ed0100b,
          wr0100b, er0100b, wd0100m1a, ed0100m1a, wm0100b, em0100b, wd0080b, ed0080b, wr0080b, er0080b, wd0080m1a,
          ed0080m1a, wm0080b, em0080b, wd0040b, ed0040b, wr0040b, er0040b, wd0040m1a, ed0040m1a, neg_d, wm0040b,
          em0040b, wd0020b, ed0020b, wr0020b, er0020b, wd0020m1a, ed0020m1a, wm0020b, em0020b, wd0010b, ed0010b,
          wr0010b, er0010b, wd0010m1a, ed0010m1a, wd4000b, ed4000b, wr4000a, er4000a, wd4000p1a, pd4000p1a, wm4000a,
          em4000a, wd2000a, ed2000a, wr2000a, er2000a, wm2000a, em2000a, wd1000a, ed1000a, wr1000a, er1000a,
          wm1000a, em1000a, wd0800a, ed0800a, wr0800a, er0800a, wm0800a, em0800a, wd0400a, ed0400a, wr0400a,
          er0400a, wm0400z, wd0200a, ed0200a, wr0200a, er0200a, wm0200a, em0200a, wd0100a, ed0100a, wr0100a,
          er0100a, wm0100a, em0100a, wd0080a, ed0080a, wr0080a, er0080a, wm0080a, em0080a, wd0040a, ed0040a,
          wr0040a, er0040a, wm0040a, em0040a, wd0020a, ed0020a, wr0020a, er0020a, wm0020a, pm0020a, wd0010a,
          ed0010a, wr0010a, er0010a, wm0010a, em0010a, ctx, ctx15, fold_sel, sel_alt, sel0, sel1,
          sel2, sel3, sel4;
  int64_t dl, du, dul, dur, resid;
  // Every one of these is a record: `alt_p2_model` walks the table by the
  // context index and its two neighbours.
  P2Freq *frec_step, *frec4c;
  // Records: the one the composed index selects, and its two neighbours.
  P2Freq *frec2, *frec3, *frec4b, *frec5, *frec6, *frec7, *prec0, *g0, *g1,
         *g2, *g3, *g4, *h4, *nxt4, *prec_m1, *h3, *nxt3, *h2,
         *nxt2, *h1, *nxt1, *h0, *nxt0;
  uint32_t bank_off, ri0100, ri0010, w_new, step_v, step10, step13;
  ctx_lo = blk->ctx & 0xF;
  sample16 = 16 * sample_in;
  blk->cursor[0]->val = 16 * sample_in;
  blk->cursor[0]->dval = blk->cursor[0]->val - blk->cursor[0]->dval;
  blk->cursor[0][1].dval = 0;
  blk->cursor[0]->sign = (resid_in <= (int32_t)(((uint32_t)(6 - ctx_lo) >> 31)
                                                         + ((uint32_t)(4 - ctx_lo) >> 31)
                                                         + 2 * ((uint32_t)(9 - ctx_lo) >> 31)))
                                            + (resid_in < (int32_t)-(((uint32_t)(6 - ctx_lo) >> 31)
                                                                + ((uint32_t)(4 - ctx_lo) >> 31)
                                                                + 2 * ((uint32_t)(9 - ctx_lo) >> 31)));
  blk->cursor[0]->mag = abs32(resid_in);
  cursor0 = blk->cursor[0];
  sample = (float)sample16;
  dl = sample16 - cursor0[-1].val;
  cursor0->dleft = (WORD2(dl) ^ dl) - WORD2(dl);
  du = sample16 - blk->cursor[1]->val;
  blk->cursor[0]->dup = (WORD2(du) ^ du) - WORD2(du);
  dul = sample16 - blk->cursor[1][-1].val;
  blk->cursor[0]->dupleft = (WORD2(dul) ^ dul) - WORD2(dul);
  dur = sample16 - blk->cursor[1][1].val;
  blk->cursor[0]->dupright = (WORD2(dur) ^ dur) - WORD2(dur);
  resid = (int16_t)(sample16 - blk->pred_prev);
  blk->cursor[0]->err = resid;
  blk->cursor[0]->aerr = (WORD2(resid) ^ resid) - WORD2(resid);
  f278656 = blk->f278656;
  ms1 = f278656[14][1] + 0.000099999997f;
  wrow_b = *(float (**)[4])(blk->cur - 1);
  bias2 = blk->bias[2];
  n2_bias = blk->bias[0];
  d_bias2 = sample - bias2;
  bias1 = blk->bias[1];
  d_bias = bias1 - bias2;
  ms_a = ((((sample - bias2) * (bias1 - bias2)) - f278656[14][0]) * 0.001f)
      + f278656[14][0];
  ms_b = ms1 + (((d_bias * d_bias) - f278656[14][1]) * 0.001f);
  f278656[14][1] = ms_b;
  ms_b10 = 0.1f * ms_b;
  if ( (0.1f * ms_b) <= ms_a )
    ms_b10 = fminf(ms_b, ms_a);
  f278656[14][0] = ms_b10;
  // Two normalised-LMS updates side by side, on two weight blocks: `f278656` at a
  // fixed mean-square rate and `wrow_b` at one scaled by the confidence `conf`.
  // Same shape as `alt_p2_context`'s, run twice with different errors and
  // different floors.
  conf = (1.0f - (ms_b10 / (ms_b + 576.0f))) * 2.0f;
  bank = 0;
  {
    const float err_a     = (sample - bias1) * 2.5999999f;
    const float err_b     = d_bias2 * conf;
    const float floor_a   = 26896.0f * f278656[14][2];
    const float floor_b   = 5041.0f * wrow_b[14][2];
    const float ms_rate_b = 0.013f * conf;
    int32_t j, k;

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
      {
        float x = blk->p2_row[j][k];
        float ms;

        ms = f278656[7 + j][k]
           + (x * x - f278656[7 + j][k]) * 0.05f;      // 0x439B40
        f278656[7 + j][k] = ms;
        f278656[j][k] += bmf_p2_rate[j][k] * err_a * x / (ms + floor_a);

        ms = wrow_b[7 + j][k]
           + (x * x - wrow_b[7 + j][k]) * ms_rate_b;
        wrow_b[7 + j][k] = ms;
        wrow_b[j][k] += bmf_p2_rate[j][k] * err_b * x / (ms + floor_b);
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
      acc[k] = f278656[0][k] * blk->p2_row[0][k];
      for ( j = 1; j < 7; ++j )
        acc[k] += f278656[j][k] * blk->p2_row[j][k];
    }
    pred     = n2_bias + bmf_hsum4(acc);
    err      = sample - pred;
    ms_scale = f278656[14][2];

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
        f278656[j][k] += bmf_p2_rate[j][k] * err * blk->p2_row[j][k]
                            / (f278656[7 + j][k] + ms_scale * 529.0f);
    ++*(int32_t *)&f278656[15][0];
    f278656[14][2] = ms_scale + ((10.0f - ms_scale) * 0.00019999999f);
  }
  *blk->cur = *(uint32_t *)&blk->f278656;
  ++blk->cur;
  ++blk->above;
  do
  {
    bank_ctx = (uint32_t)blk->bank_ctx[bank];
    res = sample16 - (*(uint32_t *)&blk->nb_sum[2 * bank]);
    bank_off = bank << 17;
    // 71178 records is +284712, `p2_ctr`, and `bank_off` is `bank << 17` --
    // 32768 records a bank, which is what the five banks are.
    node0 = &blk->p2_ctr[32768 * bank + bank_ctx];
    w0 = res + (uint16_t)node0->w2;
    *(uint16_t *)&node0->w2 = w0;
    countdown = node0->b1;
    if ( countdown )
    {
      ctxw_s = bank_ctx;
      w0b = w0 + 4 * ((res > deadzone_hi) - (res < deadzone_lo));
      *(uint16_t *)&node0->w2 = w0b;
      ctxw = ctxw_s;
      if ( (int32_t)abs32(res) < 38 )
      {
        if ( (uint8_t)--countdown )
        {
          node0->b1 = countdown;
        }
        else
        {
          if ( *(uint8_t *)&node0->b0 < 8u )
          {
            b0n = *(uint8_t *)&node0->b0 + 1;
            *(uint8_t *)&node0->b0 = b0n;
            node0->b1 = *((uint8_t *)&p2_float_pool + b0n + 3);   // two floats read sideways
            *(uint16_t *)&node0->w2 = 2 * w0b;
          }
          else
          {
            node0->b1 = countdown;
          }
        }
      }
      if ( !alphabet_reduced )
      {
        __builtin_prefetch(&mir_top, 0, 1);
        mir_top = (uint16_t *)((uint8_t *)&blk->p2_ctr[(ctxw ^ 0x7FF0)] + bank_off);
        res2 = (*(uint32_t *)&blk->nb_sum[2 * bank + 1]) + res;
        lowbits = ctxw & 3;
        if ( (uint32_t)lowbits >= 3
          || (bank_off2 = bank_off,
              ctxw_s = ctxw,
              w1 = node0[1].w2,
              e1 = res2 - p2_pred(w1, *(uint8_t *)&node0[1].b0),
              go = lowbits <= 0,
              *(uint16_t *)&node0[1].w2 = w1
                                                      + ((32
                                                        * ((e1 > deadzone_hi) - (uint32_t)(e1 < deadzone_lo))
                                                        + e1
                                                        + 1) >> 1),
              ctxw = ctxw_s,
              !go) )
        {
          bank_off2 = bank_off;
          ctxw_s = ctxw;
          wnode0m1a = node0[-1].w2;
          enode0m1a = res2 - p2_pred(wnode0m1a, *(uint8_t *)&node0[-1].b0);
          *(uint16_t *)&node0[-1].w2 = wnode0m1a
                                                  + ((32 * ((enode0m1a > deadzone_hi) - (uint32_t)(enode0m1a < deadzone_lo))
                                                    + enode0m1a
                                                    + 2) >> 2);
          ctxw = ctxw_s;
        }
        bankp = (uint8_t *)blk + bank_off;
        res_c = res2;
        __builtin_prefetch(&bankp[4 * (ctxw ^ 0x4000) + 284712], 0, 1);
        d4000 = (P2Count *)((int32_t)&bankp[4 * (ctxw ^ 0x4000) + 284712]);
        m4000 = (P2Count *)(&bankp[4 * (ctxw ^ 0x3FF0) + 284712]);
        __builtin_prefetch(m4000, 0, 1);
        r4000 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x4000) >> 2) & 3]
                  + 284712
                  + 4 * ((ctxw ^ 0x4000) & 0xFFFFFFF3)]);
        __builtin_prefetch(r4000, 0, 1);
        __builtin_prefetch(&bankp[4 * (ctxw ^ 0x2000) + 284712], 0, 1);
        d2000 = (P2Count *)((int32_t)&bankp[4 * (ctxw ^ 0x2000) + 284712]);
        __builtin_prefetch(&bankp[4 * (ctxw ^ 0x5FF0) + 284712], 0, 1);
        m2000 = (P2Count *)((int32_t)&bankp[4 * (ctxw ^ 0x5FF0) + 284712]);
        r2000 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x2000) >> 2) & 3]
                  + 284712
                  + 4 * ((ctxw ^ 0x2000) & 0xFFFFFFF3)]);
        __builtin_prefetch(r2000, 0, 1);
        __builtin_prefetch(&bankp[4 * (ctxw ^ 0x1000) + 284712], 0, 1);
        d1000 = (P2Count *)((int32_t)&bankp[4 * (ctxw ^ 0x1000) + 284712]);
        __builtin_prefetch(&bankp[4 * (ctxw ^ 0x6FF0) + 284712], 0, 1);
        m1000 = (P2Count *)((int32_t)&bankp[4 * (ctxw ^ 0x6FF0) + 284712]);
        r1000 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x1000) >> 2) & 3]
                  + 284712
                  + 4 * ((ctxw ^ 0x1000) & 0xFFFFFFF3)]);
        __builtin_prefetch(r1000, 0, 1);
        d0800 = (P2Count *)(&bankp[4 * (ctxw ^ 0x800) + 284712]);
        m0800 = (P2Count *)(&bankp[4 * (ctxw ^ 0x77F0) + 284712]);
        r0800 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x800) >> 2) & 3]
                  + 284712
                  + 4 * ((ctxw ^ 0x800) & 0xFFFFFFF3)]);
        d0400 = (P2Count *)(&bankp[4 * (ctxw ^ 0x400) + 284712]);
        __builtin_prefetch(d0800, 0, 1);
        __builtin_prefetch(m0800, 0, 1);
        __builtin_prefetch(r0800, 0, 1);
        m0400 = (P2Count *)(&bankp[4 * (ctxw ^ 0x7BF0) + 284712]);
        r0400 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x400) >> 2) & 3]
                  + 284712
                  + 4 * ((ctxw ^ 0x400) & 0xFFFFFFF3)]);
        d0200 = (P2Count *)(&bankp[4 * (ctxw ^ 0x200) + 284712]);
        __builtin_prefetch(d0400, 0, 1);
        m0200 = (P2Count *)(&bankp[4 * (ctxw ^ 0x7DF0) + 284712]);
        r0200 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x200) >> 2) & 3]
                 + 284712
                 + 4 * ((ctxw ^ 0x200) & 0xFFFFFFF3)]);
        __builtin_prefetch(m0400, 0, 1);
        __builtin_prefetch(r0400, 0, 1);
        d0100 = (P2Count *)(&bankp[4 * (ctxw ^ 0x100) + 284712]);
        ri0100 = p2_ctx_rotate[((ctxw ^ 0x100) >> 2) & 3] + ((ctxw ^ 0x100) & 0xFFFFFFF3);
        m0100 = (P2Count *)(&bankp[4 * (ctxw ^ 0x7EF0) + 284712]);
        __builtin_prefetch(d0200, 0, 1);
        r0100 = (P2Count *)((int32_t)&bankp[4 * ri0100 + 284712]);
        d0080 = (P2Count *)(&bankp[4 * (ctxw ^ 0x80) + 284712]);
        __builtin_prefetch(m0200, 0, 1);
        __builtin_prefetch(r0200, 0, 1);
        m0080 = (P2Count *)(&bankp[4 * (ctxw ^ 0x7F70) + 284712]);
        r0080 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x80) >> 2) & 3] + 284712 + 4 * ((ctxw ^ 0x80) & 0xFFFFFFF3)]);
        d0040 = (P2Count *)(&bankp[4 * (ctxw ^ 0x40) + 284712]);
        __builtin_prefetch(d0100, 0, 1);
        m0040 = (P2Count *)(&bankp[4 * (ctxw ^ 0x7FB0) + 284712]);
        r0040 = (P2Count *)(&bankp[4 * p2_ctx_rotate[((ctxw ^ 0x40) >> 2) & 3] + 284712 + 4 * ((ctxw ^ 0x40) & 0xFFFFFFF3)]);
        __builtin_prefetch(m0100, 0, 1);
        __builtin_prefetch(r0100, 0, 1);
        x0020 = ctxw ^ 0x20;
        x0010 = ctxw ^ 0x10;
        d0020 = (P2Count *)(&bankp[4 * x0020 + 284712]);
        m0020 = (P2Count *)(&bankp[4 * (x0020 ^ 0x7FF0) + 284712]);
        __builtin_prefetch(d0080, 0, 1);
        __builtin_prefetch(m0080, 0, 1);
        r0020 = (P2Count *)((uint32_t)&bankp[4 * p2_ctx_rotate[(x0020 >> 2) & 3] + 284712 + 4 * (x0020 & 0xFFFFFFF3)]);
        d0010 = (P2Count *)((int32_t)&bankp[4 * x0010 + 284712]);
        ri0010 = p2_ctx_rotate[(x0010 >> 2) & 3] + (x0010 & 0xFFFFFFF3);
        m0010 = (P2Count *)((int32_t)&bankp[4 * (x0010 ^ 0x7FF0) + 284712]);
        __builtin_prefetch(r0080, 0, 1);
        r0010 = (P2Count *)((int32_t)&bankp[4 * ri0010 + 284712]);
        mir_top2 = mir_top;
        neg = -res_c;
        LOBYTE(ri0010) = (uint8_t)mir_top[0];
        __builtin_prefetch(d0040, 0, 1);
        w_top = (int16_t)mir_top2[1];
        __builtin_prefetch(m0040, 0, 1);
        e_top = neg - p2_pred(w_top, ri0010);
        ovf = __OFSUB__(e_top, deadzone_hi);
        eq_hi = e_top == deadzone_hi;
        lt_hi = e_top - deadzone_hi < 0;
        __builtin_prefetch(r0040, 0, 1);
        bump = 32 * (!(lt_hi ^ ovf | eq_hi) - (e_top < deadzone_lo));
        __builtin_prefetch(d0020, 0, 1);
        __builtin_prefetch(m0020, 0, 1);
        res_s = res_c;
        w_new = bump + e_top + 2;
        LOWORD(w_new) = w_top + (w_new >> 2);
        go = lowbits < 3;
        mir_top[1] = w_new;
        __builtin_prefetch(((P2Count *)(r0020)), 0, 1);
        __builtin_prefetch(d0010, 0, 1);
        __builtin_prefetch(m0010, 0, 1);
        __builtin_prefetch(r0010, 0, 1);
        if ( go
          && (w_top3 = (int16_t)mir_top[3],
              b_top3 = (uint8_t)mir_top[2],
              res_c = res_s,
              go = lowbits <= 0,
              mir_top[3] = ((uint32_t)(-res_s - p2_pred(w_top3, b_top3) + 2) >> 2) + w_top3,
              go) )
        {
          wd4000b = d4000->w2;
          res_c = res_s;
          ed4000b = res_s - p2_pred(wd4000b, (*(uint8_t *)&d4000->b0));
          d4000->w2 = p2_bump(wd4000b, ed4000b, 2);
          wr4000a = r4000->w2;
          er4000a = res_s - p2_pred(wr4000a, r4000->b0);
          r4000->w2 = p2_bump(wr4000a, er4000a, 3);
          wd4000p1a = d4000[1].w2;
          pd4000p1a = p2_pred(wd4000p1a, *(uint8_t *)&d4000[1].b0);
          nres3 = -res_s;
          *(uint16_t *)&d4000[1].w2 = ((uint32_t)(res_s - pd4000p1a + 2) >> 2) + wd4000p1a;
          wm4000a = m4000->w2;
          em4000a = -res_s - p2_pred(wm4000a, m4000->b0);
          m4000->w2 = p2_bump(wm4000a, em4000a, 3);
          wd2000a = d2000->w2;
          ed2000a = res_c - p2_pred(wd2000a, *(uint8_t *)&d2000->b0);
          d2000->w2 = p2_bump(wd2000a, ed2000a, 2);
          wr2000a = r2000->w2;
          er2000a = res_c - p2_pred(wr2000a, r2000->b0);
          r2000->w2 = p2_bump(wr2000a, er2000a, 3);
          *(uint16_t *)&d2000[1].w2 += (uint32_t)(res_c
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&d2000[1].w2, *(uint8_t *)&d2000[1].b0)
                                               + 2) >> 2;
          wm2000a = m2000->w2;
          em2000a = nres3 - p2_pred(wm2000a, *(uint8_t *)&m2000->b0);
          m2000->w2 = p2_bump(wm2000a, em2000a, 3);
          wd1000a = d1000->w2;
          ed1000a = res_c - p2_pred(wd1000a, *(uint8_t *)&d1000->b0);
          d1000->w2 = p2_bump(wd1000a, ed1000a, 2);
          wr1000a = r1000->w2;
          er1000a = res_c - p2_pred(wr1000a, r1000->b0);
          r1000->w2 = p2_bump(wr1000a, er1000a, 3);
          *(uint16_t *)&d1000[1].w2 += (uint32_t)(res_c
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&d1000[1].w2, *(uint8_t *)&d1000[1].b0)
                                               + 2) >> 2;
          wm1000a = m1000->w2;
          em1000a = nres3 - p2_pred(wm1000a, *(uint8_t *)&m1000->b0);
          m1000->w2 = p2_bump(wm1000a, em1000a, 3);
          wd0800a = d0800->w2;
          ed0800a = res_c - p2_pred(wd0800a, d0800->b0);
          d0800->w2 = p2_bump(wd0800a, ed0800a, 2);
          wr0800a = r0800->w2;
          er0800a = res_c - p2_pred(wr0800a, r0800->b0);
          r0800->w2 = p2_bump(wr0800a, er0800a, 3);
          *(uint16_t *)&d0800[1].w2 += (uint32_t)(res_c - p2_pred(d0800[1].w2, d0800[1].b0) + 2) >> 2;
          wm0800a = m0800->w2;
          em0800a = nres3 - p2_pred(wm0800a, m0800->b0);
          m0800->w2 = p2_bump(wm0800a, em0800a, 3);
          wd0400a = d0400->w2;
          ed0400a = res_c - p2_pred(wd0400a, d0400->b0);
          d0400->w2 = p2_bump(wd0400a, ed0400a, 2);
          wr0400a = r0400->w2;
          er0400a = res_c - p2_pred(wr0400a, r0400->b0);
          r0400->w2 = p2_bump(wr0400a, er0400a, 3);
          *(uint16_t *)&d0400[1].w2 += (uint32_t)(res_c - p2_pred(d0400[1].w2, d0400[1].b0) + 2) >> 2;
          wm0400z = m0400->w2;
          nres3 -= p2_pred(wm0400z, m0400->b0);
          m0400->w2 = p2_bump(wm0400z, nres3, 3);
          wd0200a = d0200->w2;
          ed0200a = res_c - p2_pred(wd0200a, d0200->b0);
          d0200->w2 = p2_bump(wd0200a, ed0200a, 2);
          wr0200a = r0200->w2;
          er0200a = res_c - p2_pred(wr0200a, r0200->b0);
          r0200->w2 = p2_bump(wr0200a, er0200a, 3);
          *(uint16_t *)&d0200[1].w2 += (uint32_t)(res_c - p2_pred(d0200[1].w2, d0200[1].b0) + 2) >> 2;
          wm0200a = m0200->w2;
          nres2 = -res_c;
          em0200a = -res_c - p2_pred(wm0200a, m0200->b0);
          m0200->w2 = p2_bump(wm0200a, em0200a, 3);
          wd0100a = d0100->w2;
          ed0100a = res_c - p2_pred(wd0100a, d0100->b0);
          d0100->w2 = p2_bump(wd0100a, ed0100a, 2);
          wr0100a = r0100->w2;
          er0100a = res_c - p2_pred(wr0100a, *(uint8_t *)&r0100->b0);
          r0100->w2 = p2_bump(wr0100a, er0100a, 3);
          *(uint16_t *)&d0100[1].w2 += (uint32_t)(res_c - p2_pred(d0100[1].w2, d0100[1].b0) + 2) >> 2;
          wm0100a = m0100->w2;
          em0100a = nres2 - p2_pred(wm0100a, m0100->b0);
          m0100->w2 = p2_bump(wm0100a, em0100a, 3);
          wd0080a = d0080->w2;
          ed0080a = res_c - p2_pred(wd0080a, d0080->b0);
          d0080->w2 = p2_bump(wd0080a, ed0080a, 2);
          wr0080a = r0080->w2;
          er0080a = res_c - p2_pred(wr0080a, r0080->b0);
          r0080->w2 = p2_bump(wr0080a, er0080a, 3);
          *(uint16_t *)&d0080[1].w2 += (uint32_t)(res_c - p2_pred(d0080[1].w2, d0080[1].b0) + 2) >> 2;
          wm0080a = m0080->w2;
          em0080a = nres2 - p2_pred(wm0080a, m0080->b0);
          m0080->w2 = p2_bump(wm0080a, em0080a, 3);
          wd0040a = d0040->w2;
          ed0040a = res_c - p2_pred(wd0040a, d0040->b0);
          d0040->w2 = p2_bump(wd0040a, ed0040a, 2);
          wr0040a = r0040->w2;
          er0040a = res_c - p2_pred(wr0040a, r0040->b0);
          r0040->w2 = p2_bump(wr0040a, er0040a, 3);
          *(uint16_t *)&d0040[1].w2 += (uint32_t)(res_c - p2_pred(d0040[1].w2, d0040[1].b0) + 2) >> 2;
          wm0040a = m0040->w2;
          em0040a = nres2 - p2_pred(wm0040a, m0040->b0);
          m0040->w2 = p2_bump(wm0040a, em0040a, 3);
          wd0020a = d0020->w2;
          ed0020a = res_c - p2_pred(wd0020a, d0020->b0);
          d0020->w2 = p2_bump(wd0020a, ed0020a, 2);
          wr0020a = r0020->w2;
          er0020a = res_c - p2_pred(wr0020a, *(uint8_t *)&r0020->b0);
          r0020->w2 = p2_bump(wr0020a, er0020a, 3);
          *(uint16_t *)&d0020[1].w2 += (uint32_t)(res_c - p2_pred(d0020[1].w2, d0020[1].b0) + 2) >> 2;
          wm0020a = m0020->w2;
          pm0020a = p2_pred(wm0020a, m0020->b0);
          *(uint16_t *)&m0020->w2 = wm0020a
                               + ((32 * ((nres2 - pm0020a > deadzone_hi) - (uint32_t)(nres2 - pm0020a < deadzone_lo))
                                 + nres2
                                 - pm0020a
                                 + 4) >> 3);
          wd0010a = d0010->w2;
          ed0010a = res_c - p2_pred(wd0010a, *(uint8_t *)&d0010->b0);
          d0010->w2 = p2_bump(wd0010a, ed0010a, 2);
          wr0010a = r0010->w2;
          er0010a = res_c - p2_pred(wr0010a, *(uint8_t *)&r0010->b0);
          r0010->w2 = p2_bump(wr0010a, er0010a, 3);
          *(uint16_t *)&d0010[1].w2 += (uint32_t)(res_c
                                               - p2_pred(*(int16_t *)&*(uint16_t *)&d0010[1].w2, *(uint8_t *)&d0010[1].b0)
                                               + 2) >> 2;
          wm0010a = m0010->w2;
          em0010a = -res_c - p2_pred(wm0010a, *(uint8_t *)&m0010->b0);
          m0010->w2 = p2_bump(wm0010a, em0010a, 3);
        }
        else
        {
          w_topm1 = (int16_t)mir_top[-1];
          res_c = res_s;
          mir_top[-1] = ((uint32_t)(-res_s - p2_pred(w_topm1, ((uint8_t)mir_top[-2])) + 4) >> 3) + w_topm1;
          wd4000a = d4000->w2;
          ed4000a = res_c - p2_pred(wd4000a, *(uint8_t *)&d4000->b0);
          d4000->w2 = p2_bump(wd4000a, ed4000a, 2);
          wr4000b = r4000->w2;
          go = lowbits < 3;
          er4000b = res_c - p2_pred(wr4000b, r4000->b0);
          r4000->w2 = p2_bump(wr4000b, er4000b, 3);
          if ( go )
          {
            *(uint16_t *)&d4000[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d4000[1].w2, *(uint8_t *)&d4000[1].b0)
                                                 + 2) >> 2;
            wd4000m1a = d4000[-1].w2;
            ed4000m1a = res_c - p2_pred(wd4000m1a, *(uint8_t *)&d4000[-1].b0);
            neg_c = -res_c;
            d4000[-1].w2 = p2_bump(wd4000m1a, ed4000m1a, 3);
            wm4000b = m4000->w2;
            em4000b = neg_c - p2_pred(wm4000b, m4000->b0);
            m4000->w2 = p2_bump(wm4000b, em4000b, 3);
            wd2000b = d2000->w2;
            ed2000b = res_c - p2_pred(wd2000b, *(uint8_t *)&d2000->b0);
            d2000->w2 = p2_bump(wd2000b, ed2000b, 2);
            wr2000b = r2000->w2;
            er2000b = res_c - p2_pred(wr2000b, r2000->b0);
            r2000->w2 = p2_bump(wr2000b, er2000b, 3);
            *(uint16_t *)&d2000[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&d2000[1].w2, *(uint8_t *)&d2000[1].b0)
                                                 + 2) >> 2;
            wd2000m1a = d2000[-1].w2;
            ed2000m1a = res_c - p2_pred(wd2000m1a, *(uint8_t *)&d2000[-1].b0);
            d2000[-1].w2 = p2_bump(wd2000m1a, ed2000m1a, 3);
            wm2000b = m2000->w2;
            em2000b = neg_c - p2_pred(wm2000b, *(uint8_t *)&m2000->b0);
            m2000->w2 = p2_bump(wm2000b, em2000b, 3);
            wd1000b = d1000->w2;
            ed1000b = res_c - p2_pred(wd1000b, *(uint8_t *)&d1000->b0);
            d1000->w2 = p2_bump(wd1000b, ed1000b, 2);
            wr1000b = r1000->w2;
            er1000b = res_c - p2_pred(wr1000b, r1000->b0);
            r1000->w2 = p2_bump(wr1000b, er1000b, 3);
            *(uint16_t *)&d1000[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&d1000[1].w2, *(uint8_t *)&d1000[1].b0)
                                                 + 2) >> 2;
            wd1000m1a = d1000[-1].w2;
            ed1000m1a = res_c - p2_pred(wd1000m1a, *(uint8_t *)&d1000[-1].b0);
            d1000[-1].w2 = p2_bump(wd1000m1a, ed1000m1a, 3);
            wm1000b = m1000->w2;
            em1000b = neg_c - p2_pred(wm1000b, *(uint8_t *)&m1000->b0);
            m1000->w2 = p2_bump(wm1000b, em1000b, 3);
            wd0800b = d0800->w2;
            ed0800b = res_c - p2_pred(wd0800b, d0800->b0);
            d0800->w2 = p2_bump(wd0800b, ed0800b, 2);
            wr0800b = r0800->w2;
            er0800b = res_c - p2_pred(wr0800b, r0800->b0);
            r0800->w2 = p2_bump(wr0800b, er0800b, 3);
            *(uint16_t *)&d0800[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0800[1].w2, d0800[1].b0)
                                                 + 2) >> 2;
            wd0800m1a = d0800[-1].w2;
            ed0800m1a = res_c - p2_pred(wd0800m1a, d0800[-1].b0);
            d0800[-1].w2 = p2_bump(wd0800m1a, ed0800m1a, 3);
            wm0800b = m0800->w2;
            em0800b = neg_c - p2_pred(wm0800b, m0800->b0);
            m0800->w2 = p2_bump(wm0800b, em0800b, 3);
            wd0400b = d0400->w2;
            ed0400b = res_c - p2_pred(wd0400b, d0400->b0);
            d0400->w2 = p2_bump(wd0400b, ed0400b, 2);
            wr0400b = r0400->w2;
            // `LOBYTE(wd0400b) = r0400->b0` and `wd0400b & 31` was the rate: the mask
            // reads only the byte that was written.
            er0400b = res_c - p2_pred(wr0400b, r0400->b0);
            r0400->w2 = p2_bump(wr0400b, er0400b, 3);
            *(uint16_t *)&d0400[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0400[1].w2, d0400[1].b0)
                                                 + 2) >> 2;
            wd0400m1a = d0400[-1].w2;
            ed0400m1a = res_c - p2_pred(wd0400m1a, d0400[-1].b0);
            d0400[-1].w2 = p2_bump(wd0400m1a, ed0400m1a, 3);
            wm0400y = m0400->w2;
            nres5 = -res_c;
            nb_slot = -res_c - p2_pred(wm0400y, m0400->b0);
            m0400->w2 = p2_bump(wm0400y, nb_slot, 3);
            wd0200b = d0200->w2;
            ed0200b = res_c - p2_pred(wd0200b, d0200->b0);
            d0200->w2 = p2_bump(wd0200b, ed0200b, 2);
            wr0200b = r0200->w2;
            er0200b = res_c - p2_pred(wr0200b, r0200->b0);
            r0200->w2 = p2_bump(wr0200b, er0200b, 3);
            *(uint16_t *)&d0200[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0200[1].w2, d0200[1].b0)
                                                 + 2) >> 2;
            wd0200m1a = d0200[-1].w2;
            ed0200m1a = res_c - p2_pred(wd0200m1a, d0200[-1].b0);
            d0200[-1].w2 = p2_bump(wd0200m1a, ed0200m1a, 3);
            wm0200b = m0200->w2;
            em0200b = nres5 - p2_pred(wm0200b, m0200->b0);
            m0200->w2 = p2_bump(wm0200b, em0200b, 3);
            wd0100b = d0100->w2;
            ed0100b = res_c - p2_pred(wd0100b, d0100->b0);
            d0100->w2 = p2_bump(wd0100b, ed0100b, 2);
            wr0100b = r0100->w2;
            er0100b = res_c - p2_pred(wr0100b, *(uint8_t *)&r0100->b0);
            r0100->w2 = p2_bump(wr0100b, er0100b, 3);
            *(uint16_t *)&d0100[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0100[1].w2, d0100[1].b0)
                                                 + 2) >> 2;
            wd0100m1a = d0100[-1].w2;
            ed0100m1a = res_c - p2_pred(wd0100m1a, d0100[-1].b0);
            d0100[-1].w2 = p2_bump(wd0100m1a, ed0100m1a, 3);
            wm0100b = m0100->w2;
            em0100b = nres5 - p2_pred(wm0100b, m0100->b0);
            m0100->w2 = p2_bump(wm0100b, em0100b, 3);
            wd0080b = d0080->w2;
            ed0080b = res_c - p2_pred(wd0080b, d0080->b0);
            d0080->w2 = p2_bump(wd0080b, ed0080b, 2);
            wr0080b = r0080->w2;
            er0080b = res_c - p2_pred(wr0080b, r0080->b0);
            r0080->w2 = p2_bump(wr0080b, er0080b, 3);
            *(uint16_t *)&d0080[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0080[1].w2, d0080[1].b0)
                                                 + 2) >> 2;
            wd0080m1a = d0080[-1].w2;
            ed0080m1a = res_c - p2_pred(wd0080m1a, d0080[-1].b0);
            d0080[-1].w2 = p2_bump(wd0080m1a, ed0080m1a, 3);
            wm0080b = m0080->w2;
            em0080b = nres5 - p2_pred(wm0080b, m0080->b0);
            m0080->w2 = p2_bump(wm0080b, em0080b, 3);
            wd0040b = d0040->w2;
            ed0040b = res_c - p2_pred(wd0040b, d0040->b0);
            d0040->w2 = p2_bump(wd0040b, ed0040b, 2);
            wr0040b = r0040->w2;
            er0040b = res_c - p2_pred(wr0040b, r0040->b0);
            r0040->w2 = p2_bump(wr0040b, er0040b, 3);
            *(uint16_t *)&d0040[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0040[1].w2, d0040[1].b0)
                                                 + 2) >> 2;
            wd0040m1a = d0040[-1].w2;
            ed0040m1a = res_c - p2_pred(wd0040m1a, d0040[-1].b0);
            neg_d = -res_c;
            d0040[-1].w2 = p2_bump(wd0040m1a, ed0040m1a, 3);
            wm0040b = m0040->w2;
            em0040b = neg_d - p2_pred(wm0040b, m0040->b0);
            m0040->w2 = p2_bump(wm0040b, em0040b, 3);
            wd0020b = d0020->w2;
            ed0020b = res_c - p2_pred(wd0020b, d0020->b0);
            d0020->w2 = p2_bump(wd0020b, ed0020b, 2);
            wr0020b = r0020->w2;
            er0020b = res_c - p2_pred(wr0020b, *(uint8_t *)&r0020->b0);
            r0020->w2 = p2_bump(wr0020b, er0020b, 3);
            *(uint16_t *)&d0020[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(d0020[1].w2, d0020[1].b0)
                                                 + 2) >> 2;
            wd0020m1a = d0020[-1].w2;
            ed0020m1a = res_c - p2_pred(wd0020m1a, d0020[-1].b0);
            d0020[-1].w2 = p2_bump(wd0020m1a, ed0020m1a, 3);
            wm0020b = m0020->w2;
            em0020b = neg_d - p2_pred(wm0020b, m0020->b0);
            m0020->w2 = p2_bump(wm0020b, em0020b, 3);
            wd0010b = d0010->w2;
            ed0010b = res_c - p2_pred(wd0010b, *(uint8_t *)&d0010->b0);
            d0010->w2 = p2_bump(wd0010b, ed0010b, 2);
            wr0010b = r0010->w2;
            er0010b = res_c - p2_pred(wr0010b, *(uint8_t *)&r0010->b0);
            r0010->w2 = p2_bump(wr0010b, er0010b, 3);
            *(uint16_t *)&d0010[1].w2 += (uint32_t)(res_c
                                                 - p2_pred(*(int16_t *)&*(uint16_t *)&d0010[1].w2, *(uint8_t *)&d0010[1].b0)
                                                 + 2) >> 2;
            wd0010m1a = d0010[-1].w2;
            ed0010m1a = res_c - p2_pred(wd0010m1a, *(uint8_t *)&d0010[-1].b0);
            d0010[-1].w2 = p2_bump(wd0010m1a, ed0010m1a, 3);
            wm0010b = m0010->w2;
            em0010b = neg_d - p2_pred(wm0010b, *(uint8_t *)&m0010->b0);
          }
          else
          {
            wd4000m1b = d4000[-1].w2;
            ed4000m1b = res_c - p2_pred(wd4000m1b, *(uint8_t *)&d4000[-1].b0);
            d4000[-1].w2 = p2_bump(wd4000m1b, ed4000m1b, 3);
            wm4000c = m4000->w2;
            nres1 = -res_c;
            em4000c = -res_c - p2_pred(wm4000c, m4000->b0);
            m4000->w2 = p2_bump(wm4000c, em4000c, 3);
            wd2000c = d2000->w2;
            ed2000c = res_c - p2_pred(wd2000c, *(uint8_t *)&d2000->b0);
            d2000->w2 = p2_bump(wd2000c, ed2000c, 2);
            wr2000c = r2000->w2;
            er2000c = res_c - p2_pred(wr2000c, r2000->b0);
            r2000->w2 = p2_bump(wr2000c, er2000c, 3);
            wd2000m1b = d2000[-1].w2;
            ed2000m1b = res_c - p2_pred(wd2000m1b, *(uint8_t *)&d2000[-1].b0);
            d2000[-1].w2 = p2_bump(wd2000m1b, ed2000m1b, 3);
            wm2000c = m2000->w2;
            em2000c = nres1 - p2_pred(wm2000c, *(uint8_t *)&m2000->b0);
            m2000->w2 = p2_bump(wm2000c, em2000c, 3);
            wd1000c = d1000->w2;
            ed1000c = res_c - p2_pred(wd1000c, *(uint8_t *)&d1000->b0);
            d1000->w2 = p2_bump(wd1000c, ed1000c, 2);
            wr1000c = r1000->w2;
            er1000c = res_c - p2_pred(wr1000c, r1000->b0);
            r1000->w2 = p2_bump(wr1000c, er1000c, 3);
            wd1000m1b = d1000[-1].w2;
            ed1000m1b = res_c - p2_pred(wd1000m1b, *(uint8_t *)&d1000[-1].b0);
            d1000[-1].w2 = p2_bump(wd1000m1b, ed1000m1b, 3);
            wm1000c = m1000->w2;
            em1000c = nres1 - p2_pred(wm1000c, *(uint8_t *)&m1000->b0);
            m1000->w2 = p2_bump(wm1000c, em1000c, 3);
            wd0800c = d0800->w2;
            ed0800c = res_c - p2_pred(wd0800c, d0800->b0);
            d0800->w2 = p2_bump(wd0800c, ed0800c, 2);
            wr0800c = r0800->w2;
            er0800c = res_c - p2_pred(wr0800c, r0800->b0);
            r0800->w2 = p2_bump(wr0800c, er0800c, 3);
            wd0800m1b = d0800[-1].w2;
            ed0800m1b = res_c - p2_pred(wd0800m1b, d0800[-1].b0);
            d0800[-1].w2 = p2_bump(wd0800m1b, ed0800m1b, 3);
            wm0800c = m0800->w2;
            em0800c = nres1 - p2_pred(wm0800c, m0800->b0);
            m0800->w2 = p2_bump(wm0800c, em0800c, 3);
            wd0400c = d0400->w2;
            ed0400c = res_c - p2_pred(wd0400c, d0400->b0);
            d0400->w2 = p2_bump(wd0400c, ed0400c, 2);
            wr0400c = r0400->w2;
            er0400c = res_c - p2_pred(wr0400c, r0400->b0);
            r0400->w2 = p2_bump(wr0400c, er0400c, 3);
            wd0400m1b = d0400[-1].w2;
            ed0400m1b = res_c - p2_pred(wd0400m1b, d0400[-1].b0);
            d0400[-1].w2 = p2_bump(wd0400m1b, ed0400m1b, 3);
            wm0400x = m0400->w2;
            nres1 -= p2_pred(wm0400x, m0400->b0);
            m0400->w2 = p2_bump(wm0400x, nres1, 3);
            wd0200c = d0200->w2;
            ed0200c = res_c - p2_pred(wd0200c, d0200->b0);
            d0200->w2 = p2_bump(wd0200c, ed0200c, 2);
            wr0200c = r0200->w2;
            er0200c = res_c - p2_pred(wr0200c, r0200->b0);
            r0200->w2 = p2_bump(wr0200c, er0200c, 3);
            wd0200m1b = d0200[-1].w2;
            ed0200m1b = res_c - p2_pred(wd0200m1b, d0200[-1].b0);
            neg_b = -res_c;
            d0200[-1].w2 = p2_bump(wd0200m1b, ed0200m1b, 3);
            wm0200c = m0200->w2;
            em0200c = neg_b - p2_pred(wm0200c, m0200->b0);
            m0200->w2 = p2_bump(wm0200c, em0200c, 3);
            wd0100c = d0100->w2;
            ed0100c = res_c - p2_pred(wd0100c, d0100->b0);
            d0100->w2 = p2_bump(wd0100c, ed0100c, 2);
            wr0100c = r0100->w2;
            er0100c = res_c - p2_pred(wr0100c, *(uint8_t *)&r0100->b0);
            r0100->w2 = p2_bump(wr0100c, er0100c, 3);
            wd0100m1b = d0100[-1].w2;
            ed0100m1b = res_c - p2_pred(wd0100m1b, d0100[-1].b0);
            d0100[-1].w2 = p2_bump(wd0100m1b, ed0100m1b, 3);
            wm0100c = m0100->w2;
            em0100c = neg_b - p2_pred(wm0100c, m0100->b0);
            m0100->w2 = p2_bump(wm0100c, em0100c, 3);
            wd0080c = d0080->w2;
            ed0080c = res_c - p2_pred(wd0080c, d0080->b0);
            d0080->w2 = p2_bump(wd0080c, ed0080c, 2);
            wr0080c = r0080->w2;
            er0080c = res_c - p2_pred(wr0080c, r0080->b0);
            r0080->w2 = p2_bump(wr0080c, er0080c, 3);
            wd0080m1b = d0080[-1].w2;
            ed0080m1b = res_c - p2_pred(wd0080m1b, d0080[-1].b0);
            d0080[-1].w2 = p2_bump(wd0080m1b, ed0080m1b, 3);
            wm0080c = m0080->w2;
            em0080c = neg_b - p2_pred(wm0080c, m0080->b0);
            m0080->w2 = p2_bump(wm0080c, em0080c, 3);
            wd0040c = d0040->w2;
            ed0040c = res_c - p2_pred(wd0040c, d0040->b0);
            d0040->w2 = p2_bump(wd0040c, ed0040c, 2);
            wr0040c = r0040->w2;
            er0040c = res_c - p2_pred(wr0040c, r0040->b0);
            r0040->w2 = p2_bump(wr0040c, er0040c, 3);
            wd0040m1b = d0040[-1].w2;
            ed0040m1b = res_c - p2_pred(wd0040m1b, d0040[-1].b0);
            d0040[-1].w2 = p2_bump(wd0040m1b, ed0040m1b, 3);
            wm0040c = m0040->w2;
            em0040c = neg_b - p2_pred(wm0040c, m0040->b0);
            m0040->w2 = p2_bump(wm0040c, em0040c, 3);
            wd0020c = d0020->w2;
            res_t = res_c;
            ed0020c = res_c - p2_pred(wd0020c, d0020->b0);
            d0020->w2 = p2_bump(wd0020c, ed0020c, 2);
            wr0020c = r0020->w2;
            er0020c = res_t - p2_pred(wr0020c, *(uint8_t *)&r0020->b0);
            res_c = res_t;
            r0020->w2 = p2_bump(wr0020c, er0020c, 3);
            wd0020m1b = d0020[-1].w2;
            ed0020m1b = res_t - p2_pred(wd0020m1b, d0020[-1].b0);
            d0020[-1].w2 = p2_bump(wd0020m1b, ed0020m1b, 3);
            wm0020c = m0020->w2;
            nres4 = -res_t;
            em0020c = -res_t - p2_pred(wm0020c, m0020->b0);
            m0020->w2 = p2_bump(wm0020c, em0020c, 3);
            wd0010c = d0010->w2;
            ed0010c = res_c - p2_pred(wd0010c, *(uint8_t *)&d0010->b0);
            d0010->w2 = p2_bump(wd0010c, ed0010c, 2);
            wr0010c = r0010->w2;
            er0010c = res_c - p2_pred(wr0010c, *(uint8_t *)&r0010->b0);
            r0010->w2 = p2_bump(wr0010c, er0010c, 3);
            wd0010m1b = d0010[-1].w2;
            ed0010m1b = res_c - p2_pred(wd0010m1b, *(uint8_t *)&d0010[-1].b0);
            d0010[-1].w2 = p2_bump(wd0010m1b, ed0010m1b, 3);
            wm0010b = m0010->w2;
            em0010b = nres4 - p2_pred(wm0010b, *(uint8_t *)&m0010->b0);
          }
          m0010->w2 = p2_bump(wm0010b, em0010b, 3);
        }
      }
    }
    ++bank;
  }
  while ( bank < 5 );
  ctx = blk->ctx;
  ++blk->cursor[0];
  frec = (&blk->freq[ctx]);
  ++blk->cursor[1];
  ++blk->cursor[2];
  ++blk->cursor[3];
  ++blk->cursor[4];
  step_v = frec[0].step;
  if ( step_v > 0x10 )
  {
    is_dec = a4 & 1;
    ctx15 = ctx & 0xF;
    pair_ctx = blk->ctx_pair[a4 & 1];
    if ( ctx15 < 15 )
    {
      mir_top = (uint16_t *)&frec[1];
      if ( frec[1].f[2] + frec[1].f[1] + frec[1].f[0] > 29696 )
        frec[1].rescale_three_way();
      step10 = (10 * (uint32_t)frec[1].step) >> 4;
      if ( a4 )
      {
        mir_top[3 - is_dec] += step10;
        __update_binary_pair(model_strip((uint32_t)pair_ctx + 1), (a4 - 1) >> 1);
      }
      else
      {
        mir_top[1] += step10;
      }
      if ( ctx15 <= 0 )
      {
LABEL_37:
        step_v = blk->ctx;
        if ( blk->freq[step_v].step <= 0x1Au )
          return step_v;
        fold_sel = 2 - (blk->fold[(uint8_t)-resid_in] & 1);
        if ( !blk->fold[(uint8_t)-resid_in] )
          fold_sel = blk->fold[(uint8_t)-resid_in];
        fold_sel2 = fold_sel;
        step_v = blk->ctx_w[4].w[2 - blk->ctx_w[4].sel]
              + blk->ctx_w[3].w[2 - blk->ctx_w[3].sel]
              + blk->ctx_w[2].w[2 - blk->ctx_w[2].sel]
              + blk->ctx_w[1].w[2 - blk->ctx_w[1].sel]
              + (blk->ctx_w[0].w[1] + (blk->ctx & 0x3F));
        frec_step = &blk->freq[step_v];
        p2_rec = frec_step;
        if ( ctx15 < 15 )
        {
          frec2 = &frec_step[1];
          mir_top = (uint16_t *)frec2;
          if ( frec2->f[2] + frec_step[1].f[1] + frec_step[1].f[0] > 29696 )
          {
            step_s = step_v;
            frec2->rescale_three_way();
            step_v = step_s;
          }
          mir_top[fold_sel2 + 1] += (p2_rec[1].step & 0xFFFC) >> 2;
          if ( ctx15 <= 0 )
            goto LABEL_48;
        }
        else
        {
        }
        mir_top = (uint16_t *)&p2_rec[-1];
        if ( p2_rec[-1].f[2] + p2_rec[-1].f[1] + p2_rec[-1].f[0] > 29696 )
        {
          step_s = step_v;
          p2_rec[-1].rescale_three_way();
          step_v = step_s;
        }
        mir_top[fold_sel2 + 1] += (uint16_t)(p2_rec[-1].step & 0xFFFC) >> 2;
LABEL_48:
        if ( a4 )
        {
          n2_half = (a4 - 1) >> 1;
          // Not the record: `grp` is the high nibble here, and a record
          // address from the next assignment on.  MSVC gave both one register.
          hi_nibble = (uint8_t)pair_ctx & 0xF0;
          if ( hi_nibble >= 0xF0
            || (step_s = step_v,
                __update_binary_pair(model_strip((uint32_t)pair_ctx + 16), n2_half),
                step_v = step_s,
                hi_nibble > 0) )
          {
            step_s = step_v;
            __update_binary_pair(model_strip((uint32_t)pair_ctx - 16), n2_half);
            step_v = step_s;
          }
        }
        prec0 = &p2_rec[0];
        if ( p2_rec[0].f[2]
           + p2_rec[0].f[1]
           + p2_rec[0].f[0] > 29696 )
        {
          step_s = step_v;
          p2_rec[0].rescale_three_way();
          step_v = step_s;
        }
        prec0->f[fold_sel2] += (6 * (uint32_t)p2_rec[0].step) >> 4;
        if ( !blk->plane_idx || blk->freq[blk->ctx].step > 0x100u )
        {
          sel_alt = 2 - is_dec;
          if ( !a4 )
            sel_alt = 0;
          sel0 = blk->ctx_w[0].sel;
          is_dec = sel_alt;
          if ( sel0 == 1 )
          {
            h0 = &blk->freq[blk->ctx_w[0].w[0] + step_v - blk->ctx_w[0].w[1]];
            pair_ctx = (uintptr_t)h0;
            if ( h0->f[2] + (blk->freq[blk->ctx_w[0].w[0] + step_v - blk->ctx_w[0].w[1]].f[1]) + (blk->freq[blk->ctx_w[0].w[0] + step_v - blk->ctx_w[0].w[1]].f[0]) > 29696 )
            {
              step_s = step_v;
              h0->rescale_three_way();
              step_v = step_s;
            }
            h0->f[fold_sel2] += (uint16_t)(h0->step & 0xFFFC) >> 2;
            off0 = blk->ctx - blk->ctx_w[0].w[1];
            frecg0 = (&blk->freq[off0 + blk->ctx_w[0].w[0]]);
            idx0 = blk->ctx_w[0].w[2] + off0;
            grp = frecg0;
            frec3 = &frecg0[0];
            mir_top = (uint16_t *)frec3;
            if ( frec3->f[2] + (frec3->f[1] + frec3->f[0]) > 29696 )
            {
              step_s = step_v;
              frec3->rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec + 1] += (3 * grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (uint16_t)(grp[1].step & 0xFFFC) >> 2;
              nxt0 = &blk->freq[idx0 + 1];
              if ( blk->freq[idx0 + 1].f[2]
                 + blk->freq[idx0 + 1].f[1]
                 + blk->freq[idx0 + 1].f[0] > 29696 )
              {
                step_s = step_v;
                (&blk->freq[idx0 + 1])->rescale_three_way();
                step_v = step_s;
              }
              nxt0->f[is_dec] += (nxt0->step & 0xFFF8) >> 3;
            }
            if ( ctx15 > 2 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          else
          {
            g0 = &blk->freq[step_v + blk->ctx_w[0].w[1] - blk->ctx_w[0].w[2 - sel0]];
            if ( blk->freq[step_v + blk->ctx_w[0].w[1] - blk->ctx_w[0].w[2 - sel0]].f[2]
               + blk->freq[step_v + blk->ctx_w[0].w[1] - blk->ctx_w[0].w[2 - sel0]].f[1]
               + blk->freq[step_v + blk->ctx_w[0].w[1] - blk->ctx_w[0].w[2 - sel0]].f[0] > 29696 )
            {
              step_s = step_v;
              g0->rescale_three_way();
              step_v = step_s;
            }
            g0->f[fold_sel2] += (7 * (uint32_t)g0->step) >> 4;
            grp = ((&blk->freq[blk->ctx_w[0].w[1] + (blk->ctx - blk->ctx_w[0].w[blk->ctx_w[0].sel])]));
            mir_top = (uint16_t *)&grp[0];
            if ( grp[0].f[2] + grp[0].f[1] + grp[0].f[0] > 29696 )
            {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec + 1] += (7 * grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              step_s = step_v;
              mir_top[is_dec + 1] = mir_top[is_dec + 1] + ((5 * (uint32_t)grp[1].step) >> 4);
              step_v = step_s;
            }
            if ( ctx15 > 0 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          sel1 = blk->ctx_w[1].sel;
          if ( sel1 == 1 )
          {
            frec4b = &blk->freq[blk->ctx_w[1].w[0] + step_v - blk->ctx_w[1].w[1]];
            mir_top = (uint16_t *)frec4b;
            if ( frec4b->f[2] + (blk->freq[blk->ctx_w[1].w[0] + step_v - blk->ctx_w[1].w[1]].f[1]) + (blk->freq[blk->ctx_w[1].w[0] + step_v - blk->ctx_w[1].w[1]].f[0]) > 29696 )
            {
              step_s = step_v;
              frec4b->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2 + 1] += (uint16_t)(mir_top[0] & 0xFFFC) >> 2;
            off1 = blk->ctx - blk->ctx_w[1].w[1];
            frecg1 = (&blk->freq[off1 + blk->ctx_w[1].w[0]]);
            idx0 = blk->ctx_w[1].w[2] + off1;
            grp = frecg1;
            h1 = &frecg1[0];
            if ( h1->f[2] + h1->f[1] + h1->f[0] > 29696 )
            {
              step_s = step_v;
              h1->rescale_three_way();
              step_v = step_s;
            }
            h1->f[is_dec] += (3 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (uint16_t)(grp[1].step & 0xFFFC) >> 2;
              nxt1 = &blk->freq[idx0 + 1];
              if ( blk->freq[idx0 + 1].f[2]
                 + blk->freq[idx0 + 1].f[1]
                 + blk->freq[idx0 + 1].f[0] > 29696 )
              {
                step_s = step_v;
                (&blk->freq[idx0 + 1])->rescale_three_way();
                step_v = step_s;
              }
              nxt1->f[is_dec] += (nxt1->step & 0xFFF8) >> 3;
            }
            if ( ctx15 > 2 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          else
          {
            g1 = &blk->freq[step_v + blk->ctx_w[1].w[1] - blk->ctx_w[1].w[2 - sel1]];
            if ( blk->freq[step_v + blk->ctx_w[1].w[1] - blk->ctx_w[1].w[2 - sel1]].f[2]
               + blk->freq[step_v + blk->ctx_w[1].w[1] - blk->ctx_w[1].w[2 - sel1]].f[1]
               + blk->freq[step_v + blk->ctx_w[1].w[1] - blk->ctx_w[1].w[2 - sel1]].f[0] > 29696 )
            {
              step_s = step_v;
              g1->rescale_three_way();
              step_v = step_s;
            }
            g1->f[fold_sel2] += (7 * (uint32_t)g1->step) >> 4;
            grp = ((&blk->freq[blk->ctx_w[1].w[1] + (blk->ctx - blk->ctx_w[1].w[blk->ctx_w[1].sel])]));
            mir_top = (uint16_t *)&grp[0];
            if ( grp[0].f[2] + grp[0].f[1] + grp[0].f[0] > 29696 )
            {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec + 1] += (7 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (5 * (uint32_t)grp[1].step) >> 4;
            }
            if ( ctx15 > 0 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          sel2 = blk->ctx_w[2].sel;
          if ( sel2 == 1 )
          {
            frec5 = &blk->freq[blk->ctx_w[2].w[0] + step_v - blk->ctx_w[2].w[1]];
            mir_top = (uint16_t *)frec5;
            if ( frec5->f[2] + (blk->freq[blk->ctx_w[2].w[0] + step_v - blk->ctx_w[2].w[1]].f[1]) + (blk->freq[blk->ctx_w[2].w[0] + step_v - blk->ctx_w[2].w[1]].f[0]) > 29696 )
            {
              step_s = step_v;
              frec5->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2 + 1] += (uint16_t)(mir_top[0] & 0xFFFC) >> 2;
            off2 = blk->ctx - blk->ctx_w[2].w[1];
            frecg2 = (&blk->freq[off2 + blk->ctx_w[2].w[0]]);
            idx0 = blk->ctx_w[2].w[2] + off2;
            grp = frecg2;
            h2 = &frecg2[0];
            if ( h2->f[2] + h2->f[1] + h2->f[0] > 29696 )
            {
              step_s = step_v;
              h2->rescale_three_way();
              step_v = step_s;
            }
            h2->f[is_dec] += (3 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (uint16_t)(grp[1].step & 0xFFFC) >> 2;
              nxt2 = &blk->freq[idx0 + 1];
              if ( blk->freq[idx0 + 1].f[2]
                 + blk->freq[idx0 + 1].f[1]
                 + blk->freq[idx0 + 1].f[0] > 29696 )
              {
                step_s = step_v;
                (&blk->freq[idx0 + 1])->rescale_three_way();
                step_v = step_s;
              }
              nxt2->f[is_dec] += (nxt2->step & 0xFFF8) >> 3;
            }
            if ( ctx15 > 2 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          else
          {
            g2 = &blk->freq[step_v + blk->ctx_w[2].w[1] - blk->ctx_w[2].w[2 - sel2]];
            if ( blk->freq[step_v + blk->ctx_w[2].w[1] - blk->ctx_w[2].w[2 - sel2]].f[2]
               + blk->freq[step_v + blk->ctx_w[2].w[1] - blk->ctx_w[2].w[2 - sel2]].f[1]
               + blk->freq[step_v + blk->ctx_w[2].w[1] - blk->ctx_w[2].w[2 - sel2]].f[0] > 29696 )
            {
              step_s = step_v;
              g2->rescale_three_way();
              step_v = step_s;
            }
            g2->f[fold_sel2] += (7 * (uint32_t)g2->step) >> 4;
            grp = ((&blk->freq[blk->ctx_w[2].w[1] + (blk->ctx - blk->ctx_w[2].w[blk->ctx_w[2].sel])]));
            mir_top = (uint16_t *)&grp[0];
            if ( grp[0].f[2] + grp[0].f[1] + grp[0].f[0] > 29696 )
            {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec + 1] += (7 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (5 * (uint32_t)grp[1].step) >> 4;
            }
            if ( ctx15 > 0 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          sel3 = blk->ctx_w[3].sel;
          if ( sel3 == 1 )
          {
            frec6 = &blk->freq[blk->ctx_w[3].w[0] + step_v - blk->ctx_w[3].w[1]];
            mir_top = (uint16_t *)frec6;
            if ( frec6->f[2] + (blk->freq[blk->ctx_w[3].w[0] + step_v - blk->ctx_w[3].w[1]].f[1]) + (blk->freq[blk->ctx_w[3].w[0] + step_v - blk->ctx_w[3].w[1]].f[0]) > 29696 )
            {
              step_s = step_v;
              frec6->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2 + 1] += (uint16_t)(mir_top[0] & 0xFFFC) >> 2;
            off3 = blk->ctx - blk->ctx_w[3].w[1];
            frecg3 = (&blk->freq[off3 + blk->ctx_w[3].w[0]]);
            idx0 = blk->ctx_w[3].w[2] + off3;
            grp = frecg3;
            h3 = &frecg3[0];
            if ( h3->f[2] + h3->f[1] + h3->f[0] > 29696 )
            {
              step_s = step_v;
              h3->rescale_three_way();
              step_v = step_s;
            }
            h3->f[is_dec] += (3 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (uint16_t)(grp[1].step & 0xFFFC) >> 2;
              nxt3 = &blk->freq[idx0 + 1];
              if ( blk->freq[idx0 + 1].f[2]
                 + blk->freq[idx0 + 1].f[1]
                 + blk->freq[idx0 + 1].f[0] > 29696 )
              {
                step_s = step_v;
                (&blk->freq[idx0 + 1])->rescale_three_way();
                step_v = step_s;
              }
              nxt3->f[is_dec] += (nxt3->step & 0xFFF8) >> 3;
            }
            if ( ctx15 > 2 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          else
          {
            g3 = &blk->freq[step_v + blk->ctx_w[3].w[1] - blk->ctx_w[3].w[2 - sel3]];
            if ( blk->freq[step_v + blk->ctx_w[3].w[1] - blk->ctx_w[3].w[2 - sel3]].f[2]
               + blk->freq[step_v + blk->ctx_w[3].w[1] - blk->ctx_w[3].w[2 - sel3]].f[1]
               + blk->freq[step_v + blk->ctx_w[3].w[1] - blk->ctx_w[3].w[2 - sel3]].f[0] > 29696 )
            {
              step_s = step_v;
              g3->rescale_three_way();
              step_v = step_s;
            }
            g3->f[fold_sel2] += (7 * (uint32_t)g3->step) >> 4;
            grp = ((&blk->freq[blk->ctx_w[3].w[1] + (blk->ctx - blk->ctx_w[3].w[blk->ctx_w[3].sel])]));
            mir_top = (uint16_t *)&grp[0];
            if ( grp[0].f[2] + grp[0].f[1] + grp[0].f[0] > 29696 )
            {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec + 1] += (7 * (uint32_t)grp[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&grp[1];
              if ( grp[1].f[2] + grp[1].f[1] + grp[1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (5 * (uint32_t)grp[1].step) >> 4;
            }
            if ( ctx15 > 0 )
            {
              mir_top = (uint16_t *)&grp[-1];
              if ( grp[-1].f[2] + grp[-1].f[1] + grp[-1].f[0] > 29696 )
              {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec + 1] += (6 * (uint32_t)grp[-1].step) >> 4;
            }
          }
          sel4 = blk->ctx_w[4].sel;
          if ( sel4 == 1 )
          {
            frec7 = &blk->freq[blk->ctx_w[4].w[0] + step_v - blk->ctx_w[4].w[1]];
            mir_top = (uint16_t *)frec7;
            if ( frec7->f[2] + (blk->freq[blk->ctx_w[4].w[0] + step_v - blk->ctx_w[4].w[1]].f[1]) + (blk->freq[blk->ctx_w[4].w[0] + step_v - blk->ctx_w[4].w[1]].f[0]) > 29696 )
              frec7->rescale_three_way();
            mir_top[fold_sel2 + 1] += (uint16_t)(mir_top[0] & 0xFFFC) >> 2;
            off4 = blk->ctx - blk->ctx_w[4].w[1];
            frec4c = &blk->freq[off4 + blk->ctx_w[4].w[0]];
            // An index, not an address -- the same register again.
            rec_idx = blk->ctx_w[4].w[2] + off4;
            p2_rec = frec4c;
            h4 = &frec4c[0];
            if ( frec4c[0].f[2] + frec4c[0].f[1] + frec4c[0].f[0] > 29696 )
              h4->rescale_three_way();
            h4->f[is_dec] += (3 * (uint32_t)p2_rec[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              mir_top = (uint16_t *)&p2_rec[1];
              if ( p2_rec[1].f[2] + (p2_rec[1].f[1] + p2_rec[1].f[0]) > 29696 )
                p2_rec[1].rescale_three_way();
              mir_top[is_dec + 1] += (uint16_t)(p2_rec[1].step & 0xFFFC) >> 2;
              nxt4 = &blk->freq[rec_idx + 1];
              if ( nxt4->f[2] + nxt4->f[1] + nxt4->f[0] > 29696 )
                nxt4->rescale_three_way();
              step_v = (nxt4->step & 0xFFF8) >> 3;
              nxt4->f[is_dec] += step_v;
            }
            if ( ctx15 > 2 )
            {
              prec_m1 = &p2_rec[-1];
              if ( p2_rec[-1].f[2]
                 + p2_rec[-1].f[1]
                 + p2_rec[-1].f[0] > 29696 )
                p2_rec[-1].rescale_three_way();
              step_v = (uintptr_t)p2_rec;   // the slot's last value, and what this path returns
              prec_m1->f[is_dec] += (6
                                                               * (uint32_t)p2_rec[-1].step) >> 4;
            }
          }
          else
          {
            g4 = &blk->freq[step_v + blk->ctx_w[4].w[1] - blk->ctx_w[4].w[2 - sel4]];
            if ( blk->freq[step_v + blk->ctx_w[4].w[1] - blk->ctx_w[4].w[2 - sel4]].f[2]
               + blk->freq[step_v + blk->ctx_w[4].w[1] - blk->ctx_w[4].w[2 - sel4]].f[1]
               + blk->freq[step_v + blk->ctx_w[4].w[1] - blk->ctx_w[4].w[2 - sel4]].f[0] > 29696 )
              g4->rescale_three_way();
            g4->f[fold_sel2] += (7 * (uint32_t)g4->step) >> 4;
            gtop = ((&blk->freq[blk->ctx_w[4].w[1] + (blk->ctx - blk->ctx_w[4].w[blk->ctx_w[4].sel])]));
            if ( gtop[0].f[2] + gtop[0].f[1] + gtop[0].f[0] > 29696 )
              gtop[0].rescale_three_way();
            gtop[0].f[is_dec] += (7 * (uint32_t)gtop[0].step) >> 4;
            if ( ctx15 < 15 )
            {
              if ( gtop[1].f[2] + gtop[1].f[1] + gtop[1].f[0] > 29696 )
                gtop[1].rescale_three_way();
              step_v = gtop[1].step;
              gtop[1].f[is_dec] += (5 * step_v) >> 4;
            }
            if ( ctx15 > 0 )
            {
              if ( gtop[-1].f[2] + gtop[-1].f[1] + gtop[-1].f[0] > 29696 )
                gtop[-1].rescale_three_way();
              step_v = gtop[-1].step;
              gtop[-1].f[is_dec] += (6 * step_v) >> 4;
            }
          }
        }
        return step_v;
      }
      frec = (&blk->freq[blk->ctx]);
    }
    mir_top = (uint16_t *)&frec[-1];
    if ( frec[-1].f[2] + frec[-1].f[1] + frec[-1].f[0] > 29696 )
      frec[-1].rescale_three_way();
    step13 = (13 * (uint32_t)frec[-1].step) >> 4;
    if ( a4 )
    {
      mir_top[3 - is_dec] += step13;
      __update_binary_pair(model_strip((uint32_t)pair_ctx - 1), (a4 - 1) >> 1);
    }
    else
    {
      mir_top[1] += step13;
    }
    goto LABEL_37;
  }
  return step_v;
}


void __alt_p2_d8_decode_body(AltP2Block *blk, int8_t unread_flag, uint8_t *out, int32_t width, int32_t height)
{
  P2Ctx *pix, *prev, *rec, *buf3, *b4, *buf2, *buf1, *b0;
  int32_t x;
  // These shared `__frame.x` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t y;
  uint8_t *row;
  ;
  uintptr_t code;
  int32_t *cur, *r1;   // the row cursors, four bytes a step
  bool more;
  int16_t val;
  // The five planes, held across the rotation that ends a row.
  int32_t wid, q, last, *r0, pred, val2;
  int64_t err;
  uint32_t j;
  __rc_begin_decode(unread_flag);
  wid = width;
  pix = blk->cursor[0];
  prev = pix;
  if ( width > 0 )
  {
    x = 0;
    while ( 1 )
    {
      q = pix[-1].val >> 4;
      blk->ctx_pair[0] = blk->ctx + (*(uint32_t *)&blk->ctx_delta[q + 4]);
      blk->ctx_pair[1] = blk->ctx + (*(uint32_t *)&blk->ctx_delta[q]);
      val = (uint8_t)(((uint16_t)blk->cursor[0][-1].val >> 4)
                            + *(uint8_t *)(blk->freq[blk->ctx
                                                               + blk->ctx_w[3].w[(prev[-1].val <= prev[-2].val)
                                                                            + (prev[-1].val < prev[-2].val)]
                                                               + blk->ctx_w[2].w[prev[-2].sign]
                                                               + blk->ctx_w[1].w[prev[-1].sign]
                                                               + blk->ctx_w[0].w[(((uint32_t)(q - 115) >> 31)
                                                                            + ((uint32_t)(q - 17) >> 31))]
                                                               + blk->ctx_w[4].w[1]].decode_symbol(blk->ctx_pair)
                                       + (uintptr_t)blk
                                       + 280496));
      *out = val;
      val *= 16;
      blk->cursor[0]->val = val;
      ++out;
      blk->cursor[0]->dval = val;
      rec = blk->cursor[0];
      err = (int16_t)(rec->val - rec[-1].val);
      rec->err = err;
      LOWORD(err) = (WORD2(err) ^ err) - WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup >> 1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err <= 0)
                                                       + (blk->cursor[0]->err < 0);
      pix = (blk->cursor[0] + 1);
      more = x + 1 < width;
      blk->cursor[0] = pix;
      ++x;
      if ( !more )
        break;
      prev = blk->cursor[0];
    }
    wid = width;
  }
  // The row end mirrored into the right margin: five copies of record -1.
  // MSVC reloaded `cursor[0]` between every pair and nothing here writes it,
  // which is why this arrived as five cursors.
  {
    P2Ctx *const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec1 = blk->cursor[0] + (-wid);
  rec1[-1] = rec1[0];
  rec1[-2] = rec1[1];
  rec1[-3] = rec1[2];
  rec1[-4] = rec1[3];
  rec1[-5] = rec1[4];
  rec1[-6] = rec1[5];
  rec1[-7] = rec1[6];
  rec1[-8] = rec1[7];
  memcpy(blk->buf[1],blk->buf[0],18 * wid + 234);
  memcpy(blk->buf[2],blk->buf[0],18 * wid + 234);
  memcpy(blk->buf[3],blk->buf[0],18 * wid + 234);
  if ( height > 1 )
  {
    y = 0;
    do
    {
      // Start the next row: carry the last word of this one forward, swap
      // the two row buffers, and re-derive the two cursors from them.
      cur = blk->cur;
      last = cur[-1];
      row = out;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      r0 += 2;
      blk->cur = r1;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;

      blk->bias[1] = 0.0f;

      blk->bias[2] = 0.0f;

      blk->bias[3] = 0.0f;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      // And at the row start, records 0..4 take -1..-5 -- the mirror of the
      // block above, and the same five reloaded cursors.
      {
        P2Ctx *const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      // The five planes rotate right by one, and each cursor follows its own
      // 144 bytes in.
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1 + 8;
      blk->cursor[3] = buf2 + 8;
      blk->cursor[4] = buf3 + 8;
      ((P2Ctx *)b4)[-1] = ((P2Ctx *)b0)[0];
      // The new row's left margin, from the row above, reversed: eight more
      // reloads of the same two cursors, half of them through `uintptr_t`.
      {
        P2Ctx *const here = blk->cursor[0];
        P2Ctx *const up   = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if ( width > 0 )
      {
        for ( j = 0; j < (uint32_t)width; ++j )
        {
          pred = __alt_p2_context((AltP2Block *)blk, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          code = blk->freq[blk->ctx].decode_symbol(blk->ctx_pair);
          val2 = (uint8_t)(pred + (*(uint8_t *)&blk->unfold[code]));
          row[j] = val2;
          __alt_p2_model(blk, val2, code, val2 - pred);
          out = &row[j + 1];
        }
      }
      ++y;
    }
    while ( y < (uint32_t)(height - 1) );
  }
  __rc_end_decode();
}

void __alt_model_p2_d8_decode( uint8_t *out, int32_t i, int32_t height)
{
  ;
  void *raw, **blk;
  raw = bmf_page_alloc(0x103E30u);
  if ( raw )
    blk = (void **)__alt_p2_alloc((AltP2Block *)raw, i, 0);
  else
    blk = nullptr;
  __alt_p2_d8_decode_body((AltP2Block *)(int32_t)blk, i, out, i, height);
  if ( blk )
    __alt_p2_free((void **)blk, 1);
}

int32_t __alt_model_p2_decode(uint16_t *p_i, uint8_t *out) {   P2Ctx *cur0,
        *cur1, *cur2, *cur3, *buf3, *bb4, *buf2, *buf1, *bb0;
  // These shared `x` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t xf4, val3;
  AltP2Block *plane[4];
  int32_t w, pl2;
  uint32_t back, row_bytes;
  int32_t dc_flag, xf0;
  uint32_t i_1, first;
  uint32_t i_2, np;
  ;
  AltP2Block *blk_r, *src3, *blk_k, *blk2, *blk3, *blk1;
  int16_t seed1;
  uint32_t i;
  int16_t seed3, seed2;
  P2Freq *freq;
  AltP2Block *blk0;
  uint32_t src2;
  AltP2Block **planep;
  float saved_p2_coef[7][4];
  int32_t *cur, *r1;   // the row cursors, four bytes a step
  bool xf3, off;
  int32_t raw, pl, nplanes, xf1, xf2, b4, *b0, pred0, code0, val0, pred1, code1,
          val1, l7a, l4a, l5a, pred2, code2, val2, l7b, l4b, l5b,
          pred3, code3, l7c, l4c, l5c, nplanes2, pl3;
  void *made, *src1, **plane_p;
  // The p2 filter coefficients drift over a run: rows 4..6 are folded into
  // rows 0..2 here and then zeroed, and the whole table goes back on the way
  // out, so a run starts from the same place the last one did.
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
  raw = p_i[1];
  if ( plane_count > 0 )
  {
    pl = 0;
    do
    {
      made = bmf_page_alloc(0x103E30u);
      if ( made )
        src1 = __alt_p2_alloc((AltP2Block *)made, i, pl);
      else
        src1 = nullptr;
      plane[pl++] = (AltP2Block *)src1;
    }
    while ( pl < plane_count );
  }
  xf0 = plane_desc[plane_desc[2].src_plane + 1].flags & 8;
  dc_flag = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  xf4 = (plane_desc[plane_desc[4].src_plane + 1].flags) & 8;
  __rc_begin_decode(dc_flag);
  if ( raw > 0 )
  {
    src2 = 0;
    back = -i;
    nplanes = plane_count;
    row_bytes = 18 * i + 234;
    np = i + 13;
    do
    {
      if ( nplanes > 0 )
      {
        first = src2;
        pl2 = 0;
        i_1 = i;
        do
        {
          ++pl2;
          if ( first )
          {
            planep = &plane[pl2];
            if ( first == 1 )
            {
              blk_k = (AltP2Block *)((int32_t)*(planep - 1));
              // The row end mirrored into the right margin: five copies of
              // record -1.  `v31` and the `LOWORD(v34)` between them read two
              // of record -1's fields into registers that are never read back.
              {
                P2Ctx *const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec2 = blk_k->cursor[0] + back;
              rec2[-1] = rec2[0];
              rec2[-2] = rec2[1];
              rec2[-3] = rec2[2];
              rec2[-4] = rec2[3];
              rec2[-5] = rec2[4];
              rec2[-6] = rec2[5];
              rec2[-7] = rec2[6];
              rec2[-8] = rec2[7];
              memcpy(blk_k->buf[1],blk_k->buf[0],row_bytes);
              memcpy(blk_k->buf[2],blk_k->buf[0],row_bytes);
              memcpy(blk_k->buf[3],blk_k->buf[0],row_bytes);
            }
          }
          else
          {
            src3 = (AltP2Block *)((int32_t)plane[pl2 - 1]);
            w = (int32_t)(&plane[pl2]);
            xf1 = 0;
            do
            {
              xf2 = xf1;
              src3->buf[0][xf2].val = 256;
              xf3 = (uint32_t)(++xf1) < np;
              src3->buf[0][xf2].dval = 256;
              src3->buf[0][xf2].err = -16;
              src3->buf[0][xf2].sign = 1;
              src3->buf[0][xf2].mag = 3;
              src3->buf[0][xf2].aerr = 512;
              src3->buf[0][xf2].dupright = 512;
              src3->buf[0][xf2].dupleft = 512;
              src3->buf[0][xf2].dup = 1024;
              src3->buf[0][xf2].dleft = 256;
            }
            while ( xf3 );
            planep = (AltP2Block **)w;
            memcpy(src3->buf[1],src3->buf[0],row_bytes);
            memcpy(src3->buf[2],src3->buf[0],row_bytes);
            memcpy(src3->buf[3],src3->buf[0],row_bytes);
            src3->cursor[0] = src3->buf[0] + i + 8;
            src3->cursor[1] = (src3->buf[1] + i + 8);
            src3->cursor[2] = src3->buf[2] + i + 8;
            src3->cursor[3] = src3->buf[3] + i + 8;
            src3->cursor[4] = (src3->buf[4] + i + 8);
          }
          blk_r = (AltP2Block *)((int32_t)*(planep - 1));
          // Start the next row: carry the last word of this one forward, swap
          // the two row buffers, and re-derive the two cursors from them.
          cur = blk_r->cur;
          b4 = cur[-1];
          cur[1] = b4;
          *blk_r->cur = b4;
          b0 = blk_r->row0;
          r1 = blk_r->row1;
          blk_r->row0 = r1;
          blk_r->row1 = b0;
          r1 += 2;
          blk_r->cur = r1;
          b0 += 2;
          blk_r->above = b0;
          r1[-1] = *b0;
          blk_r->cur[-2] = *b0;
          memset(blk_r->p2_row0_head, 0, sizeof blk_r->p2_row0_head);
          blk_r->bias[0] = 0.0f;

          blk_r->bias[1] = 0.0f;

          blk_r->bias[2] = 0.0f;

          blk_r->bias[3] = 0.0f;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(blk_r->p2_row, 0, sizeof blk_r->p2_row);
          // And at the row start, records 0..4 take -1..-5.
          {
            P2Ctx *const here = blk_r->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
          }
          buf3 = blk_r->buf[3];
          bb4 = blk_r->buf[4];
          buf2 = blk_r->buf[2];
          buf1 = blk_r->buf[1];
          bb0 = blk_r->buf[0];
          blk_r->buf[4] = buf3;
          blk_r->buf[3] = buf2;
          blk_r->buf[2] = buf1;
          blk_r->buf[0] = bb4;
          blk_r->buf[1] = bb0;
          bb4 += 8;
          blk_r->cursor[0] = bb4;
          bb0 += 8;
          blk_r->cursor[1] = bb0;
          blk_r->cursor[2] = buf1 + 8;
          blk_r->cursor[3] = buf2 + 8;
          blk_r->cursor[4] = buf3 + 8;
          ((P2Ctx *)bb4)[-1] = ((P2Ctx *)bb0)[0];
          // The new row's left margin, from the row above, reversed.
          {
            P2Ctx *const here = blk_r->cursor[0];
            P2Ctx *const up   = blk_r->cursor[1];
            here[-2] = up[1];
            here[-3] = up[2];
            here[-4] = up[3];
            here[-5] = up[4];
            here[-6] = up[5];
            here[-7] = up[6];
            here[-8] = up[7];
          }
          blk_r->cursor[0]->dval = 0;
          nplanes = plane_count;
        }
        while ( plane_count > pl2 );
        src2 = first;
        i = i_1;
      }
      ctx_bias[3] = 0;
      ctx_bias[2] = 0;
      ctx_bias[1] = 0;
      ctx_bias[0] = 0;
      if ( i > 0 )
      {
        first = src2;
        i_2 = 0;
        i_1 = i;
        do
        {
          ctx_bias[0] >>= 3;
          ctx_bias[1] >>= 3;
          ctx_bias[2] >>= 3;
          ctx_bias[3] >>= 3;
          blk0 = (AltP2Block *)(plane[0]);
          pred0 = __alt_p2_context((AltP2Block *)plane[0], (AltP2Block *)plane[2], (AltP2Block *)plane[1]);
          freq = &blk0->freq[blk0->ctx];
          code0 = freq->decode_symbol(blk0->ctx_pair);
          val0 = (uint8_t)(pred0 + (*(uint8_t *)&blk0->unfold[code0]));
          __alt_p2_model((AltP2Block *)blk0, val0, code0, val0 - pred0);
          cur0 = blk0->cursor[0];
          ctx_bias[0] += 32 * cur0[-1].dupleft;
          ctx_bias[1] += (32 * cur0[-1].dupright);
          ctx_bias[2] += 32 * cur0[-1].dleft;
          ctx_bias[3] += 32 * cur0[-1].dup;
          off = xf0 == 0;
          out[plane_desc[1].src_plane] = val0;
          if ( off )
            seed1 = 0;
          else
            seed1 = 16 * out[plane_desc[1].src_plane];
          blk1 = (AltP2Block *)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          pred1 = __alt_p2_context((AltP2Block *)blk1, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          code1 = blk1->freq[blk1->ctx].decode_symbol(blk1->ctx_pair);
          val1 = (uint8_t)(pred1 + blk1->unfold[code1]);
          __alt_p2_model((AltP2Block *)blk1, val1, code1, val1 - pred1);
          cur1 = blk1->cursor[0];
          l7a = cur1[-1].dupright;
          l4a = cur1[-1].dleft;
          ctx_bias[0] += 32 * cur1[-1].dupleft;
          l5a = cur1[-1].dup;
          ctx_bias[1] += 32 * l7a;
          ctx_bias[2] += 32 * l4a;
          ctx_bias[3] += 32 * l5a;
          off = dc_flag == 0;
          out[plane_desc[2].src_plane] = val1;
          if ( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[3].src_plane + 1].w8 * out[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * out[plane_desc[1].src_plane]) >> 3;
          blk2 = (AltP2Block *)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          pred2 = __alt_p2_context((AltP2Block *)blk2, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          code2 = blk2->freq[blk2->ctx].decode_symbol(blk2->ctx_pair);
          val2 = (uint8_t)(pred2 + blk2->unfold[code2]);
          __alt_p2_model((AltP2Block *)blk2, val2, code2, val2 - pred2);
          cur2 = blk2->cursor[0];
          l7b = cur2[-1].dupright;
          l4b = cur2[-1].dleft;
          ctx_bias[0] += 32 * cur2[-1].dupleft;
          l5b = cur2[-1].dup;
          ctx_bias[1] += 32 * l7b;
          ctx_bias[2] += 32 * l4b;
          ctx_bias[3] += 32 * l5b;
          out[plane_desc[3].src_plane] = val2;
          nplanes = plane_count;
          if ( plane_count >= 4 )
          {
            if ( xf4 )
              seed3 = (plane_desc[plane_desc[4].src_plane + 1].w12 * out[2]
                    + plane_desc[plane_desc[4].src_plane + 1].w8 * out[1]
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *out) >> 3;
            else
              seed3 = 0;
            blk3 = (AltP2Block *)(plane[3]);
            plane[3]->cursor[0]->dval = seed3;
            pred3 = __alt_p2_context((AltP2Block *)blk3, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            code3 = blk3->freq[blk3->ctx].decode_symbol(blk3->ctx_pair);
            val3 = (uint8_t)(pred3 + blk3->unfold[code3]);
            __alt_p2_model((AltP2Block *)blk3, val3, code3, val3 - pred3);
            cur3 = blk3->cursor[0];
            l7c = cur3[-1].dupright;
            l4c = cur3[-1].dleft;
            l5c = cur3[-1].dup;
            ctx_bias[0] += 32 * cur3[-1].dupleft;
            ctx_bias[1] += 32 * l7c;
            ctx_bias[2] += 32 * l4c;
            ctx_bias[3] += 32 * l5c;
            out[3] = val3;
            nplanes = plane_count;
          }
          out += nplanes;
          ++i_2;
        }
        while ( i_2 < i_1 );
        src2 = first;
        i = i_1;
      }
      ++src2;
    }
    while ( src2 < (uint32_t)raw );
  }
  __rc_end_decode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  nplanes2 = plane_count;
  if ( plane_count > 0 )
  {
    pl3 = 0;
    do
    {
      plane_p = (void **)plane[pl3];
      if ( plane_p )
      {
        __alt_p2_free((void **)plane_p, 1);
        nplanes2 = plane_count;
      }
      ++pl3;
    }
    while ( pl3 < nplanes2 );
  }
  return nplanes2;
}

void __unmodel_plane(int8_t unread_flag, uint16_t *p_i, uint8_t *out)
{
  ;
  ModelBlock *blk;
  void *raw;
  if ( plane_alt_model )
  {
    if ( plane_predictor == 1 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __alt_model_p1_d8_decode(unread_flag, out, *p_i, p_i[1]);
      else
        __alt_model_p1_decode(p_i, out);
    }
    else if ( plane_predictor == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __alt_model_p2_d8_decode(out, *p_i, p_i[1]);
      else
        __alt_model_p2_decode(p_i, out);
    }
  }
  else
  {
    raw = bmf_new(0x7BA230u);
    if ( raw )
      blk = __layout_workspace((ModelBlock *)raw, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F);
    else
      blk = (ModelBlock *)(nullptr);
    ((ModelBlock *)blk)->unmodel_plane_slow(out);
    if ( blk )
      __free_workspace((ModelBlock *)blk, 1);
  }
}

void __alt_p2_d8_encode_body(AltP2Block *blk, uint8_t *src, int32_t width, int32_t height, uint8_t *out)
{
  int32_t val;   // the symbol MSVC kept in the low half of `rec`
  P2Ctx *pix, *rec, *buf3, *b4, *buf2, *buf1, *b0;
  int32_t j;
  uint32_t y;
  uint8_t *p;
  int32_t code0;
  uint8_t *outp, want, *srcp, code2;
  // These shared `__frame.j` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  // These shared `__frame.code0` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t *pair;   // the block's `ctx_pair`
  ;
  int32_t *cur, *r1;   // the row cursors, four bytes a step
  uint8_t recon, recon2;
  int32_t q, resid, drift, q2, row_bytes, last, *r0, pred, resid2, drift2, code;
  int64_t err;
  P2Ctx *rec2;
  uint32_t k;
  uint8_t *src_end;
  src_end = src;
  __rc_begin_encode();
  pix = blk->cursor[0];
  if ( width > 0 )
  {
    p = src;
    for ( j = 0; j < width; ++j )
    {
      q = (uint16_t)pix[-1].val >> 4;
      resid = (uint8_t)(*p - q);
      code0 = blk->fold[resid];
      recon = blk->unfold[code0] + q;
      drift = (uint8_t)*out - (uint8_t)(recon + *out - *p);
      if ( drift < -16 || drift > 16 )
      {
        *out = *p;
        code0 = blk->fold_hi[resid];
      }
      else
      {
        *out = recon;
      }
      rec = blk->cursor[0];
      q2 = rec[-1].val >> 4;
      blk->ctx_pair[0] = blk->ctx + blk->ctx_delta[q2 + 4];
      blk->ctx_pair[1] = blk->ctx + blk->ctx_delta[q2];
      // The composed index, the encoder's own copy of `alt_p2_context`'s sum:
      // four selected weights, one constant, and the running context.
      blk->freq[blk->ctx
                         + blk->ctx_w[3].w[(rec[-1].val <= rec[-2].val)
                                               + (rec[-1].val < rec[-2].val)]
                         + blk->ctx_w[2].w[rec[-2].sign]
                         + blk->ctx_w[1].w[rec[-1].sign]
                         + blk->ctx_w[0].w[((uint32_t)(q2 - 115) >> 31)
                                               + ((uint32_t)(q2 - 17) >> 31)]
                         + blk->ctx_w[4].w[1]].encode_symbol(blk->ctx_pair, code0);
      ++p;
      val = 16 * (uint8_t)*out;
      blk->cursor[0]->val = val;
      blk->cursor[0]->dval = val;
      ++out;
      rec2 = blk->cursor[0];
      err = (int16_t)(rec2->val - rec2[-1].val);
      rec2->err = err;
      LOWORD(err) = (WORD2(err) ^ err) - WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup >> 1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err <= 0)
                                                    + (blk->cursor[0]->err < 0);
      pix = (blk->cursor[0] + 1);
      blk->cursor[0] = pix;
    }
    src_end = p;
  }
  // The row end mirrored into the right margin: five copies of record -1.
  // `v31` read two of record -1's fields between the third and fourth copy
  // and was never read back -- a register MSVC filled and did not use.
  {
    P2Ctx *const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec3 = blk->cursor[0] + (-width);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  row_bytes = 18 * width + 234;
  memcpy(blk->buf[1],blk->buf[0],row_bytes);
  memcpy(blk->buf[2],blk->buf[0],row_bytes);
  memcpy(blk->buf[3],blk->buf[0],row_bytes);
  if ( height > 1 )
  {
    pair = blk->ctx_pair;
    outp = out;
    y = 0;
    do
    {
      // Start the next row: carry the last word of this one forward, swap
      // the two row buffers, and re-derive the two cursors from them.
      cur = blk->cur;
      last = cur[-1];
      srcp = src_end;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      blk->cur = r1;
      r0 += 2;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;

      blk->bias[1] = 0.0f;

      blk->bias[2] = 0.0f;

      blk->bias[3] = 0.0f;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      // And at the row start, records 0..4 take -1..-5.
      {
        P2Ctx *const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1 + 8;
      blk->cursor[3] = buf2 + 8;
      blk->cursor[4] = buf3 + 8;
      ((P2Ctx *)b4)[-1] = ((P2Ctx *)b0)[0];
      // The new row's left margin, from the row above, reversed.
      {
        P2Ctx *const here = blk->cursor[0];
        P2Ctx *const up   = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if ( width > 0 )
      {
        for ( k = 0; k < (uint32_t)width; ++k )
        {
          pred = __alt_p2_context((AltP2Block *)blk, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          want = srcp[k];
          resid2 = (uint8_t)(want - pred);
          recon2 = pred + blk->unfold[blk->fold[resid2]];
          drift2 = (uint8_t)outp[k] - (uint8_t)(recon2 + outp[k] - want);
          code = blk->fold[resid2];
          if ( drift2 < -16 || drift2 > 16 )
          {
            outp[k] = want;
            code = blk->fold_hi[(uint8_t)(want - pred)];
          }
          else
          {
            outp[k] = recon2;
          }
          code2 = code;
          blk->freq[blk->ctx].encode_symbol(pair, code);
          __alt_p2_model((AltP2Block *)blk, (uint8_t)outp[k], code2, (uint8_t)outp[k] - pred);
        }
        // Both cursors advanced one per pixel inside the loop, the second
        // through an `int32_t` spill; after `width` pixels they are these.
        src_end = &srcp[width];
        outp = &outp[width];
      }
      ++y;
    }
    while ( y < (uint32_t)(height - 1) );
  }
  __rc_end_encode();
}

void __alt_model_p2_d8_encode( uint8_t *src, int32_t i, int32_t height, uint8_t *out)
{
  ;
  void * raw;
  AltP2Block *blk;
  raw = bmf_page_alloc(0x103E30u);
  if ( raw )
    blk = (AltP2Block *)__alt_p2_alloc((AltP2Block *)raw, i, 0);
  else
    blk = (AltP2Block *)(nullptr);
  __alt_p2_d8_encode_body((AltP2Block *)blk, src, i, height, out);
  if ( blk )
    __alt_p2_free((void **)blk, 1);
}

int32_t __alt_model_p2_encode(BmfImage *p_i, uint8_t *a2) {   P2Ctx *rec0,
        *rec1, *rec2, *rec3, *buf3, *bb4, *buf2, *buf1, *bb0;
  int32_t y, alpha_src, x, w;
  AltP2Block *plane[4];
  uint32_t row_bytes;
  uint8_t *out;
  uint32_t xf4;
  int32_t pl2;
  uint32_t i, code3;
  int32_t back, first;
  int32_t np, xf0;
  int32_t recon0b, code2b, recon1b, off1, off2, pred0, pred1, pred2, resid0, resid2, resid1,
          off0b;
  ;
  AltP2Block *blk_r, *src2, *blk_k, *blk2, *l4c, *blk1;
  bool xf2;
  int16_t seed1, l7c, seed2;
  uint32_t src1;
  AltP2Block *blk0, **xf3;
  int32_t *cur, *r1;   // the row cursors, four bytes a step
  int32_t row_i, height, pl, nplanes, src3, xf1, want0, b4, *b0, off0, seed0, at0,
          cur0, code0, out0, recon0, drift0, l7a, l4a, l5a, cur1, code1, out1,
          recon1, drift1, l7b, l4b, l5b, cur2, code2, out2, recon2, drift2,
          l5c, seed3, blk3, cur3, drift3, nplanes2, pl3;
  void *raw, *made, **plane_p;
  out = a2;
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
  row_i = p_i->width;
  height = p_i->height;
  if ( plane_count > 0 )
  {
    pl = 0;
    do
    {
      raw = bmf_page_alloc(0x103E30u);
      if ( raw )
        made = (void *)__alt_p2_alloc((AltP2Block *)raw, row_i, pl);
      else
        made = nullptr;
      plane[pl++] = (AltP2Block *)made;
    }
    while ( pl < plane_count );
  }
  xf0 = plane_desc[plane_desc[2].src_plane + 1].flags & 8;
  np = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  y = (plane_desc[plane_desc[4].src_plane + 1].flags) & 8;
  __rc_begin_encode();
  if ( height > 0 )
  {
    src1 = 0;
    first = -row_i;
    row_bytes = 18 * row_i + 234;
    code3 = row_i + 13;
    nplanes = plane_count;
    do
    {
      if ( nplanes > 0 )
      {
        xf4 = src1;
        pl2 = 0;
        do
        {
          ++pl2;
          if ( xf4 )
          {
            xf3 = &plane[pl2];
            if ( xf4 == 1 )
            {
              blk_k = (AltP2Block *)((int32_t)*(xf3 - 1));
              // The row end mirrored into the right margin: five copies of
              // record -1, with two registers filled from record -1 between
              // copies and never read back.
              {
                P2Ctx *const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec4 = blk_k->cursor[0] + first;
              rec4[-1] = rec4[0];
              rec4[-2] = rec4[1];
              rec4[-3] = rec4[2];
              rec4[-4] = rec4[3];
              rec4[-5] = rec4[4];
              rec4[-6] = rec4[5];
              rec4[-7] = rec4[6];
              rec4[-8] = rec4[7];
              memcpy(blk_k->buf[1],blk_k->buf[0],row_bytes);
              memcpy(blk_k->buf[2],blk_k->buf[0],row_bytes);
              memcpy(blk_k->buf[3],blk_k->buf[0],row_bytes);
            }
          }
          else
          {
            src2 = (AltP2Block *)((int32_t)plane[pl2 - 1]);
            back = (int32_t)(&plane[pl2]);
            src3 = 0;
            do
            {
              xf1 = src3;
              src2->buf[0][xf1].val = 256;
              xf2 = (uint32_t)(++src3) < code3;
              src2->buf[0][xf1].dval = 256;
              src2->buf[0][xf1].err = -16;
              src2->buf[0][xf1].sign = 1;
              src2->buf[0][xf1].mag = 3;
              src2->buf[0][xf1].aerr = 512;
              src2->buf[0][xf1].dupright = 512;
              src2->buf[0][xf1].dupleft = 512;
              src2->buf[0][xf1].dup = 1024;
              src2->buf[0][xf1].dleft = 256;
            }
            while ( xf2 );
            xf3 = (AltP2Block **)back;
            memcpy(src2->buf[1],src2->buf[0],row_bytes);
            memcpy(src2->buf[2],src2->buf[0],row_bytes);
            memcpy(src2->buf[3],src2->buf[0],row_bytes);
            src2->cursor[0] = src2->buf[0] + row_i + 8;
            src2->cursor[1] = (src2->buf[1] + row_i + 8);
            src2->cursor[2] = src2->buf[2] + row_i + 8;
            src2->cursor[3] = src2->buf[3] + row_i + 8;
            src2->cursor[4] = (src2->buf[4] + row_i + 8);
          }
          blk_r = (AltP2Block *)((int32_t)*(xf3 - 1));
          // Start the next row: carry the last word of this one forward, swap
          // the two row buffers, and re-derive the two cursors from them.
          cur = blk_r->cur;
          b4 = cur[-1];
          cur[1] = b4;
          *blk_r->cur = b4;
          b0 = blk_r->row0;
          r1 = blk_r->row1;
          blk_r->row0 = r1;
          blk_r->row1 = b0;
          r1 += 2;
          blk_r->cur = r1;
          b0 += 2;
          blk_r->above = b0;
          r1[-1] = *b0;
          blk_r->cur[-2] = *b0;
          memset(blk_r->p2_row0_head, 0, sizeof blk_r->p2_row0_head);
          blk_r->bias[0] = 0.0f;

          blk_r->bias[1] = 0.0f;

          blk_r->bias[2] = 0.0f;

          blk_r->bias[3] = 0.0f;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(blk_r->p2_row, 0, sizeof blk_r->p2_row);
          // And at the row start, records 0..4 take -1..-5.
          {
            P2Ctx *const here = blk_r->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
          }
          buf3 = blk_r->buf[3];
          bb4 = blk_r->buf[4];
          buf2 = blk_r->buf[2];
          buf1 = blk_r->buf[1];
          bb0 = blk_r->buf[0];
          blk_r->buf[4] = buf3;
          blk_r->buf[3] = buf2;
          blk_r->buf[2] = buf1;
          blk_r->buf[0] = bb4;
          blk_r->buf[1] = bb0;
          bb4 += 8;
          blk_r->cursor[0] = bb4;
          bb0 += 8;
          blk_r->cursor[1] = bb0;
          blk_r->cursor[2] = buf1 + 8;
          blk_r->cursor[3] = buf2 + 8;
          blk_r->cursor[4] = buf3 + 8;
          ((P2Ctx *)bb4)[-1] = ((P2Ctx *)bb0)[0];
          // The new row's left margin, from the row above, reversed.
          {
            P2Ctx *const here = blk_r->cursor[0];
            P2Ctx *const up   = blk_r->cursor[1];
            here[-2] = up[1];
            here[-3] = up[2];
            here[-4] = up[3];
            here[-5] = up[4];
            here[-6] = up[5];
            here[-7] = up[6];
            here[-8] = up[7];
          }
          blk_r->cursor[0]->dval = 0;
          nplanes = plane_count;
        }
        while ( plane_count > pl2 );
        src1 = xf4;
      }
      if ( row_i <= 0 )
      {
        ctx_bias[0] = 0;
        ctx_bias[1] = 0;
        ctx_bias[2] = 0;
        ctx_bias[3] = 0;
      }
      else
      {
        xf4 = src1;
        want0 = 0;
        off0 = 0;
        seed0 = 0;
        at0 = 0;
        for ( i = 0; i < (uint32_t)row_i; ++i )
        {
          off0b = plane_desc[1].src_plane;
          ctx_bias[0] = at0 >> 3;
          ctx_bias[1] = seed0 >> 3;
          ctx_bias[2] = off0 >> 3;
          ctx_bias[3] = want0 >> 3;
          blk0 = (AltP2Block *)(plane[0]);
          cur0 = out[plane_desc[1].src_plane];
          pred0 = __alt_p2_context((AltP2Block *)plane[0], (AltP2Block *)plane[2], (AltP2Block *)plane[1]);
          resid0 = (uint8_t)(cur0 - pred0);
          code0 = (uint8_t)blk0->fold[resid0];
          out0 = out[off0b];
          recon0b = (uint8_t)(pred0 + (*(uint8_t *)&blk0->unfold[code0]));
          recon0 = (uint8_t)(recon0b + out0 - cur0);
          drift0 = out0 - recon0;
          if ( drift0 < -16 || drift0 > 16 )
          {
            code0 = (uint8_t)blk0->fold_hi[resid0];
          }
          else
          {
            cur0 = recon0b;
            out[off0b] = recon0;
          }
          blk0->freq[blk0->ctx].encode_symbol(blk0->ctx_pair, code0);
          __alt_p2_model((AltP2Block *)blk0, cur0, code0, cur0 - pred0);
          rec0 = blk0->cursor[0];
          l7a = rec0[-1].dupright;
          l4a = rec0[-1].dleft;
          l5a = rec0[-1].dup;
          ctx_bias[0] += 32 * rec0[-1].dupleft;
          ctx_bias[1] += 32 * l7a;
          ctx_bias[2] += 32 * l4a;
          ctx_bias[3] += 32 * l5a;
          if ( xf0 )
            seed1 = 16 * out[plane_desc[1].src_plane];
          else
            seed1 = 0;
          blk1 = (AltP2Block *)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          off1 = plane_desc[2].src_plane;
          cur1 = out[plane_desc[2].src_plane];
          pred1 = __alt_p2_context((AltP2Block *)blk1, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          resid1 = (uint8_t)(cur1 - pred1);
          code1 = blk1->fold[resid1];
          out1 = out[off1];
          recon1b = (uint8_t)(pred1 + blk1->unfold[code1]);
          recon1 = (uint8_t)(recon1b + out1 - cur1);
          drift1 = out1 - recon1;
          if ( drift1 < -16 || drift1 > 16 )
          {
            code1 = blk1->fold_hi[resid1];
          }
          else
          {
            cur1 = recon1b;
            out[off1] = recon1;
          }
          blk1->freq[blk1->ctx].encode_symbol(blk1->ctx_pair, code1);
          __alt_p2_model((AltP2Block *)blk1, cur1, code1, cur1 - pred1);
          rec1 = blk1->cursor[0];
          l7b = rec1[-1].dupright;
          l4b = rec1[-1].dleft;
          l5b = rec1[-1].dup;
          ctx_bias[0] += 32 * rec1[-1].dupleft;
          ctx_bias[1] += 32 * l7b;
          ctx_bias[2] += 32 * l4b;
          ctx_bias[3] += 32 * l5b;
          if ( np )
            seed2 = (plane_desc[plane_desc[3].src_plane + 1].w8 * out[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * out[plane_desc[1].src_plane]) >> 3;
          else
            seed2 = 0;
          blk2 = (AltP2Block *)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          off2 = plane_desc[3].src_plane;
          cur2 = out[plane_desc[3].src_plane];
          pred2 = __alt_p2_context((AltP2Block *)blk2, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          resid2 = (uint8_t)(cur2 - pred2);
          code2 = blk2->fold[resid2];
          out2 = out[off2];
          code2b = (uint8_t)(pred2 + blk2->unfold[code2]);
          recon2 = (uint8_t)(code2b + out2 - cur2);
          drift2 = out2 - recon2;
          if ( drift2 < -16 || drift2 > 16 )
          {
            code2 = blk2->fold_hi[resid2];
          }
          else
          {
            cur2 = code2b;
            out[off2] = recon2;
          }
          blk2->freq[blk2->ctx].encode_symbol(blk2->ctx_pair, code2);
          __alt_p2_model((AltP2Block *)blk2, cur2, code2, cur2 - pred2);
          rec2 = blk2->cursor[0];
          nplanes = plane_count;
          at0 = ctx_bias[0] + 32 * rec2[-1].dupleft;
          seed0 = ctx_bias[1] + 32 * rec2[-1].dupright;
          off0 = ctx_bias[2] + 32 * rec2[-1].dleft;
          want0 = ctx_bias[3] + 32 * rec2[-1].dup;
          if ( plane_count >= 4 )
          {
            ctx_bias[0] += 32 * rec2[-1].dupleft;
            ctx_bias[1] = seed0;
            ctx_bias[2] = off0;
            ctx_bias[3] = want0;
            if ( y )
              l7c = (plane_desc[plane_desc[4].src_plane + 1].w12 * out[2]
                    + plane_desc[plane_desc[4].src_plane + 1].w8 * out[1]
                    + plane_desc[plane_desc[4].src_plane + 1].w4 * *out) >> 3;
            else
              l7c = 0;
            l4c = (AltP2Block *)(plane[3]);
            plane[3]->cursor[0]->dval = l7c;
            alpha_src = plane_desc[4].src_plane;
            l5c = out[plane_desc[4].src_plane];
            x = __alt_p2_context((AltP2Block *)l4c, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            w = (uint8_t)(l5c - x);
            seed3 = l4c->fold[w];
            blk3 = out[alpha_src];
            cur3 = (uint8_t)(x + l4c->unfold[seed3] + blk3 - l5c);
            drift3 = blk3 - cur3;
            if ( drift3 < -16 || drift3 > 16 )
            {
              seed3 = l4c->fold_hi[w];
            }
            else
            {
              l5c = (uint8_t)(x + l4c->unfold[seed3]);
              out[alpha_src] = cur3;
            }
            l4c->freq[l4c->ctx].encode_symbol(l4c->ctx_pair, seed3);
            __alt_p2_model((AltP2Block *)l4c, l5c, seed3, l5c - x);
            rec3 = l4c->cursor[0];
            nplanes = plane_count;
            at0 = ctx_bias[0] + 32 * rec3[-1].dupleft;
            seed0 = ctx_bias[1] + 32 * rec3[-1].dupright;
            off0 = ctx_bias[2] + 32 * rec3[-1].dleft;
            want0 = ctx_bias[3] + 32 * rec3[-1].dup;
          }
          out += nplanes;
        }
        src1 = xf4;
        ctx_bias[0] = at0;
        ctx_bias[1] = seed0;
        ctx_bias[2] = off0;
        ctx_bias[3] = want0;
      }
      ++src1;
    }
    while ( src1 < (uint32_t)height );
  }
  __rc_end_encode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  nplanes2 = plane_count;
  if ( plane_count > 0 )
  {
    pl3 = 0;
    do
    {
      plane_p = (void **)plane[pl3];
      if ( plane_p )
      {
        __alt_p2_free((void **)plane_p, 1);
        nplanes2 = plane_count;
      }
      ++pl3;
    }
    while ( pl3 < nplanes2 );
  }
  return nplanes2;
}

void __model_plane( BmfImage *p_i, uint8_t *pixels, uint8_t *raw)
{
  uint32_t alpha_n, hi;
  int32_t y1, bucket, f_b3, f_b0, f_b4, f_b1, f_b2, f_b5, alpha;
  // These three shared a stack slot each with a local the main declaration
  // below still names: MSVC gave one slot to locals whose live ranges do not
  // overlap, and Hex-Rays named every use.  That they can have storage of
  // their own is the gate's answer -- nothing writes one of them and reads
  // another.  The two names that used to head this list, `v58` and
  // `Blocka_2`, are gone: the first was the outer loop's counter written
  // twice and the second was a second name for the block.
  uint16_t *word;   // a row cursor into sym_word
  ;
  ModelBlock *blk;
  PixRec *r4, *r0;   // row cursors out of row_cur
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  int16_t w2n, wt;
  uint32_t row_w;
  int32_t g, flags, lo, w2, w3, w4, has3, has4, lvl, lvl2, live, s, y,
          x, x2, step;
  PixRec *row_cur3, *row_cur2, *row_cur1;   // three of the five row buffers
  uint32_t gi, n_syms, j, n_syms2, k;
  SymListBlock *blk1, *blk0;
  SymPair *group_ctr;   // one group's row of counter pairs
  SymList *lists1, *lists0;
  FreqRec *rec;   // a bucket record: `grid[bucket]`
  PixRec *up1, *up2;   // `row_cur[6]` and `row_cur[7]`, the two rows above
  void *ws;
  if ( plane_alt_model )
  {
    if ( ::plane_predictor == 1 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p1_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p1_encode((uint16_t *)p_i, pixels);
    }
    else if ( ::plane_predictor == 2 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p2_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p2_encode((BmfImage *)p_i, pixels);
    }
  }
  else
  {
    ws = bmf_new(0x7BA230u);
    if ( ws )
      blk = __layout_workspace((ModelBlock *)ws, p_i->height, p_i->width, p_i->height, p_i->depth & 0x3F);
    else
      blk = (ModelBlock *)(0);
    __rc_begin_encode();
    // `mode` was declared and never assigned.  `reduce_alphabet` does not
    // read that parameter, and neither do the three below in
    // `compress_image` -- `search_filter` forwards it to
    // `choose_plane_coding`, which ignores it; `transform_planes` ignores it;
    // and `model_planes` forwards it to `colour_transform`, which ignores it.
    // Four uninitialised bytes travelling through five functions to be
    // dropped.  `-Wmaybe-uninitialized` is what found them.
    __reduce_alphabet(blk, 0, pixels);
    bucket = 0;
    g = 0;
    do
    {
      flags = ctx_group_flags[g];
      blk->ctx_state[flags] = g;
      lo = 0;
      f_b2 = flags & 4;
      f_b1 = flags & 2;
      f_b4 = flags & 0x10;
      f_b0 = flags & 1;
      f_b3 = flags & 8;
      f_b5 = flags & 0x20;
      do
      {
        hi = 0;
        do
        {
          blk->ctx_bucket[g + 15 * lo + 75 * hi] = bucket;
          alpha = blk->alphabet;
          // A sixteen-byte record per bucket, at `+96 + 16 * bucket`: five
          // counts at words 0..4, their total at 5, a scaled weight at 6, and
          // two bytes at 7 -- a level (`<= f16`) and the weight `1 << (5 -
          // level)` it derives.  The base is the object and the counter starts
          // at zero, so record 0 is +96 .. +111 -- which `row_cur[10..13]` also
          // claims, and which the `f1051664[k] = row_cur[10 + k]` copy after this
          // loop reads back.  That copy is dead -- nothing reads `f1051664` --
          // so the collision costs nothing at run time, but it does mean one
          // of `row_cur`'s length and `f1051664`'s type is wrong.
          
          // The bucket counter reaches exactly 188: a `__builtin_trap()` on
          // `>= 188` fires on fourteen of the gate's streams and one on
          // `>= 189` fires on none.  So this table is 189 records, +96 ..
          // +3119.  `FreqRec` is on the same grid from record 188 (+3104),
          // which makes the last bucket record and the first frequency record
          // the same sixteen bytes -- the two tables abut and share one.
          rec = &blk->grid[bucket];
          w2n = 2;
          rec->w[0] = 2;
          LOWORD(w2) = 2;
          w3 = 2;
          w4 = 2;
          if ( f_b2 )
          {
            w3 = 4;
            w4 = 0;
          }
          if ( f_b1 )
          {
            LOWORD(w2) = w4 + 2;
            w4 = 0;
          }
          if ( f_b4 )
          {
            LOWORD(w2) = w3 + w2;
            w3 = 0;
          }
          if ( f_b0 )
          {
            w2n = w4 + 2;
            w4 = 0;
            rec->w[4] = 0;
          }
          else
          {
            rec->w[4] = w4;
          }
          if ( f_b3 )
          {
            w2n += w3;
            w3 = 0;
            rec->w[3] = 0;
          }
          else
          {
            rec->w[3] = w3;
          }
          if ( f_b5 )
          {
            rec->w[1] = w2 + w2n;
            LOWORD(w2) = 0;
            rec->w[2] = 0;
          }
          else
          {
            rec->w[2] = w2;
            rec->w[1] = w2n;
          }
          has3 = w3 != 0;
          has4 = w4 != 0;
          w2 = (uint16_t)w2;
          if ( (uint16_t)w2 )
            w2 = 1;
          lvl = has4 + has3 + w2 + 2;
          if ( lvl <= alpha )
          {
            rec->b14 = lvl;
          }
          else
          {
            rec->b14 = has4 + has3 + w2 + 1;
            rec->w[0] = 0;
          }
          if ( rec->w[lo]
            && rec->w[hi]
            && (lvl2 = rec->b14, lvl2 <= alpha) )
          {
            live = 1;
            wt = (uint8_t)(1 << ((5 - lvl2) & 31));
            rec->b15 = wt;
            rec->w[6] = wt << 6;
            rec->w[lo] += wt;
            rec->w[hi] += rec->b15;
            rec->w[5] = rec->w[0]
                                 + rec->w[4]
                                 + rec->w[3]
                                 + rec->w[2]
                                 + rec->w[1];
          }
          else
          {
            live = 0;
          }
          bucket += live;
          ++hi;
        }
        while ( hi < 5 );
        ++lo;
      }
      while ( (uint32_t)lo < 5 );
      gi = 0;
      group_ctr = blk->group_ctr[g];
      do
      {
        group_ctr[gi].last = 0x2000;
        group_ctr[gi++].prev = 0x2000;
      }
      while ( gi < 0x10000 );
      ++g;
    }
    while ( (uint32_t)g < 0xF );
    buf = (uint8_t *)bmf_new(blk->alphabet);
    alpha_n = blk->alphabet;
    blk->alpha_map = buf;
    memset(buf,1,alpha_n);
    blk->escape_list = &blk->escape;
    __init_symbol_list(&blk->escape, 0, blk->alphabet, 1);
    blk->sel_cur = blk->sel;
    n_syms = blk->alphabet;
    blk1 = (SymListBlock *)bmf_new(24 * n_syms + 4);
    if ( blk1 )
    {
      blk1->n = n_syms;
      lists1 = blk1->list;
      if ( n_syms )
      {
        // MSVC unrolled this two lists at a time and left a scalar tail; both
        // halves write `ent`, and between them they cover 0 .. n_syms - 1.
        for ( j = 0; j < n_syms; ++j )
          lists1[j].ent = nullptr;
      }
    }
    else
    {
      lists1 = nullptr;
    }
    n_syms2 = blk->alphabet;
    blk->sel1_list = lists1;
    blk0 = (SymListBlock *)bmf_new(24 * n_syms2 + 4);
    if ( blk0 )
    {
      blk0->n = n_syms2;
      lists0 = blk0->list;
      if ( n_syms2 )
      {
        // MSVC unrolled this two lists at a time and left a scalar tail; both
        // halves write `ent`, and between them they cover 0 .. n_syms2 - 1.
        for ( k = 0; k < n_syms2; ++k )
          lists0[k].ent = nullptr;
      }
    }
    else
    {
      lists0 = nullptr;
    }
    blk->sel0_list = lists0;
    if ( !(blk->alphabet <= 0) )
    {
      s = 0;
      do
      {
        __init_symbol_list(&blk->sel1_list[s], 0, 99, 0);
        __init_symbol_list(&blk->sel0_list[s++], 0, 33, 0);
      }
      while ( (uint32_t)s < blk->alphabet );
    }
    if ( blk->height > 0 )
    {
      word = blk->sym_word;
      y = 0;
      do
      {
        y1 = y + 1;
        blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym == 0;
        blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym == 0;
        r4 = blk->row_cur[4];
        row_cur3 = blk->row_cur[3];
        row_cur2 = blk->row_cur[2];
        row_cur1 = blk->row_cur[1];
        r0 = blk->row_cur[0];
        blk->row_cur[4] = row_cur3;
        blk->row_cur[3] = row_cur2;
        blk->row_cur[2] = row_cur1;
        blk->row_cur[1] = r0;
        blk->row_cur[0] = r4;
        r4 += 7;
        blk->row_cur[5] = r4;
        r0 += 7;
        blk->row_cur[6] = r0;
        blk->row_cur[7] = row_cur1 + 7;
        blk->row_cur[8] = row_cur2 + 7;
        blk->row_cur[9] = row_cur3 + 7;
        // The same two flags, on the encoding side.
        {
          uint8_t zero = r0[1].sym == 0;
          r4->match[2] = zero;
          blk->row_cur[5][-1].match[4] = zero;
          blk->row_cur[5][-2].match[5] = zero;
          zero = blk->row_cur[6][2].sym == 0;
          blk->row_cur[5]->match[4] = zero;
          blk->row_cur[5][-1].match[5] = zero;
        }
        blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym == 0;
        up1 = blk->row_cur[6];
        up2 = blk->row_cur[7];
        ++blk->row_cur[5];
        ++up1;
        blk->row_cur[6] = up1;
        ++blk->row_cur[8];
        ++up2;
        blk->row_cur[7] = up2;
        ++blk->row_cur[9];
        blk->grad[0] = up1[3].match[0] + up1[2].match[0] + up1[1].match[0] + up1[0].match[0] + up1[4].match[0] - 5;
        // The same five counts as the line above, off the other row.
        blk->grad[3] = 0;
        blk->grad[2] = 0;
        y = y1;
        row_w = blk->width;
        blk->grad[1] = up2[3].match[0] + up2[2].match[0] + up2[1].match[0] + up2[0].match[0] + up2[4].match[0] - 5;
        if ( row_w > 0 )
        {
          x = 0;
          do
          {
            ++x;
            blk->row_cur[5][x - 1].sym = word[x - 1];
            row_w = blk->width;
          }
          while ( (uint32_t)x < blk->width );
          word += x;
        }
        if ( row_w > 0 )
        {
          x2 = 0;
          do
          {
            step = ((ModelBlock *)blk)->code_pixel(x2);
            blk->init_tables();
            x2 += step;
          }
          while ( (uint32_t)x2 < blk->width );
          y = y1;
        }
      }
      while ( (uint32_t)y < *(uint32_t *)&blk->height );
    }
    __rc_end_encode();
    __free_workspace((ModelBlock *)blk, 1);
  }
}

void __model_planes(uint8_t *img, uint8_t *pixels, int32_t plane, int8_t unread_flag)
{
  // Its frame was lifted: `tools/liftframe.py` gave every member its own
  // storage and the gate held.  The comment that stood here said the opposite,
  // on the authority of a sweep that by then could no longer make the attempt.
  BmfImage hdr;
  ;
  uint8_t *aligned;   // was int32_t: these hold addresses
  uint8_t *scratch;
  int32_t ofs;
  plane_predictor = plane_desc[plane + 1].flags & 3;
  plane_alt_model = (uint8_t)(plane_desc[plane + 1].flags & 4) >> 2;
  __colour_transform(img, pixels, plane, unread_flag);
  scratch = ::hist_scratch;
  aligned = (uint8_t *)((uintptr_t)(::hist_scratch + 15) & 0xFFFFFFF0);
  // Fifteen bytes at the front and sixteen at +1008.  MSVC split the first
  // into 8 + 4 + 2 + 1 and the second into two eight-byte stores.
  memset(scratch, 0, 15);
  bmf_zero16(&scratch[1008]);
  ofs = 1008;
  do
  {
    bmf_zero16((aligned + ofs - 16));
    bmf_zero16((aligned + ofs - 32));
    bmf_zero16((aligned + ofs - 48));
    bmf_zero16((aligned + ofs - 64));
    bmf_zero16((aligned + ofs - 80));
    bmf_zero16((aligned + ofs - 96));
    bmf_zero16((aligned + ofs - 112));
    ofs -= 112;
  }
  while ( ofs );
  // always taken: -S
  {
    // The caller's header, with the depth byte replaced: 72 is 8 bits plus the
    // 0x40 flag `alloc_image` sets.
    hdr = *(BmfImage *)img;
    hdr.depth = 72;
    // never taken: -E is 0
    if ( plane_predictor == 1 && !plane_alt_model )
      __predict_med(pixels, ((const BmfImage *)img)->width, ((const BmfImage *)img)->height);
    __model_plane(&hdr, pixels, pixels);
    // `if ( pixels != pixels )` stood here, and behind it an interleave and a
    // free.  It was the test for "the -E block above allocated a second
    // buffer"; with that block gone, both names held the caller's one buffer
    // and the test was false on every path.  Deleting a block does not delete
    // the test that asked whether it ran, and this one outlived it by a
    // fortnight -- as the last thing in the file gcov could report as never
    // executed.  tools/deadcheck.py looks for the shape now.  The two names
    // are gone as well, which is what made the argument above checkable.
  }
}


void __transform_planes(BmfImage *p_i, int32_t unread_mode, int8_t unread_flag)
{
  ;
  uint8_t *arc, *hdr, *tmp, *dst;   // `uint8_t *` beside the `char` scalars above
  int32_t k, plane, predictor, alt, n, stride, ofs, i;
  uint8_t *src_pixels;
  memset(hist_scratch,0,4096);
  arc = (uint8_t *)::coded_buf;
  hdr = (uint8_t *)::coded_buf + 16;
  // The whole sixteen-byte header, which MSVC moved a word at a time.
  *(BmfImage *)hdr = *p_i;
  src_pixels = (uint8_t *)((uint16_t *)p_i->pixels);
  memcpy(arc + 32,p_i->pixels,p_i->data_size);
  tmp = (uint8_t *)bmf_new(p_i->width * p_i->height);
  if ( plane_count > 0 )
  {
    k = 0;
    do
    {
      ++k;
      plane = BYTE1(plane_desc[k].w0);
      predictor = plane_desc[plane + 1].flags & 3;
      ::plane_predictor = predictor;
      alt = (uint8_t)(plane_desc[plane + 1].flags & 4) >> 2;
      plane_alt_model = alt;
      if ( ((plane_desc[plane + 1].flags & 8) != 0 || predictor) && !alt )
      {
        // `v11` was declared and never assigned: an uninitialised byte read into
        // a parameter neither `colour_transform` nor `interleave_plane` looks at.
        // Zero says the same thing without the undefined read.
        __colour_transform(hdr, tmp, plane, 0);
        if ( ::plane_predictor != 2 )
        {
          // never taken: -E is 0
          if ( ::plane_predictor == 1 )
          {
            __predict_med(tmp, p_i->width, p_i->height);
          }
        }
        n = p_i->width * p_i->height;
        stride = plane_count;
        if ( plane_count == 1 )
        {
          memcpy(&p_i->pixels[plane],tmp,n);
        }
        else
        {
          dst = (uint8_t *)p_i + plane;
          // The third of MSVC's aliasing proofs, and the only one whose two
          // arms are not the same code: the `if` side was a `do`/`while` and
          // the `else` side a `for`, so they disagree when `n` is zero --
          // the `do`/`while` writes one byte first and asks afterwards.  Both
          // are otherwise the same six lines, with the `else` carrying a save
          // and restore of `k` around a loop that never touched it.
          
          // The `for` is the one kept, which is a choice and not a
          // transcription: a zero-pixel plane cannot reach here through any
          // input the gate has, so the difference is unobservable, and between
          // two arms that disagree the right one to keep is the one that is
          // correct rather than the one that came first.
          ofs = 0;
          for ( i = 0; i < n; ++i )
          {
            dst[ofs + 16] = tmp[i];
            ofs += stride;
          }
        }
      }
    }
    while ( k < plane_count );
  }
  free(tmp);
  // always taken: -S
  {
    __model_plane((BmfImage *)p_i, src_pixels, src_pixels);
  }
}

uint8_t * __expand_image(uint8_t *arc_in, int32_t want_pal, void **p_coded_buf)
{
  // This one is a layout, not a bag of locals: giving its members
  // their own storage makes DLRAW exit 3 while decompressing.  Re-checked
  // by `tools/liftframe.py` against the file as it is now --
  // `frame-sweep.sh`, which this note used to cite, lifts aliases
  // and has had none to lift since round nine.
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
      int32_t nplanes_s;
      void *row_step;
      uint8_t *arc_f;
      BmfImage *p_i_2;
      int32_t depth_f;
      uint32_t pal_len;
      uint16_t hdr_words[5];
      uint8_t depth_b;
      int8_t flags_b;
      uint32_t data_len;
      uint8_t   hdr[8];   // the 8-byte member header `fread` takes in one call
      uint32_t magic_word;
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 112, "frame layout moved");
  // These shared `__frame.Block` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  ;
  uint8_t *arc;   // were int32_t: these hold addresses
  FILE *fp1, *fp;
  BmfImage *img_at;
  int8_t hdr_flags;
  uint8_t bpp, has_coded, dc_v;
  uint32_t near_lossless, pred_s, w12_v, w4_v, w8_v;
  uint8_t *pal_at, *copy, *srcp, *dst;   // `uint8_t *` beside the `char` scalars above
  int32_t hdr_word, pl, predictor, plane_i, plane,
          pl2, plane2, pred, i, n_pix2, pix_at, nplanes_c, i2, n_planes,
          last_row, img_h, at, y, pl_i, left;
  uint16_t w16;
  int32_t n_pix;
  uint32_t magic, pad_len, *blk, pal_bytes, want2,
           desc, desc_flags, want, got, word12, word8, word4,
           worddc, word6, word4b;
  uint8_t *plane_buf;
  void *last_row2;
  arc = arc_in;
  if ( p_coded_buf )
    *p_coded_buf = nullptr;
  fp1 = ((BmfArc *)arc_in)->fp;
  if ( !fp1 )
    return nullptr;
  while ( 1 )
  {
    if ( fread(&__frame.magic_word, 4u, 1u, fp1) != 1 )
    {
      fp = ((BmfArc *)arc)->fp;
      if ( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        ((BmfArc *)arc)->fp = 0;
        return nullptr;
      }
    }
    magic = __frame.magic_word;
    if ( (uint16_t)__frame.magic_word != 0x9081 )
      break;
    plane_desc[0].w4 = ((BYTE2(__frame.magic_word) << 8) - 12288) | (HIBYTE(__frame.magic_word) - 48);
    if ( plane_desc[0].w4 != 512 || fread(__frame.hdr, 8u, 1u, ((BmfArc *)arc)->fp) != 1 )
      break;
    fseek(((BmfArc *)arc)->fp, (*(uint32_t *)&__frame.hdr[4]), 1);
    fp1 = ((BmfArc *)arc)->fp;
  }
  if ( (uint16_t)magic != 0x8A81
    || (plane_desc[0].w4 = ((BYTE2(magic) << 8) - 12288) | (HIBYTE(magic) - 48), plane_desc[0].w4 != 512)
    || fread(__frame.hdr_words, 0x10u, 1u, ((BmfArc *)arc)->fp) != 1 )
  {
    fp = ((BmfArc *)arc)->fp;
    fclose(fp);
    ((BmfArc *)arc)->fp = 0;
    return nullptr;
  }
  ++*(uint32_t *)arc;
  if ( __frame.flags_b < 0 )
  {
    fread(__frame.hdr, 8u, 1u, ((BmfArc *)arc)->fp);
    if ( p_coded_buf )
    {
      hdr_word = (*(int32_t *)&__frame.hdr[0]);
      pad_len = ((*(uint32_t *)&__frame.hdr[4]) + ((*(uint32_t *)&__frame.hdr[4]) == 0) + 3) & 0xFFFFFFFC;
      blk = (uint32_t *)bmf_new(pad_len + 8);
      *blk = hdr_word;
      blk[1] = pad_len;
      *(uint32_t *)((uint8_t *)blk + pad_len + 4) = 0;
      *p_coded_buf = blk;
      fread(blk + 2, (*(uint32_t *)&__frame.hdr[4]), 1u, ((BmfArc *)arc)->fp);
    }
    else
    {
      fseek(((BmfArc *)arc)->fp, (*(uint32_t *)&__frame.hdr[4]), 1);
    }
  }
  pal_bytes = 3 << (__frame.depth_b & 31);
  if ( (__frame.depth_b & 0x80) == 0 )
    pal_bytes = __frame.depth_b & 0x80;
  __frame.pal_len = pal_bytes;
  if ( want_pal )
  {
    fseek(((BmfArc *)arc)->fp, __frame.pal_len + __frame.data_len, 1);
    return nullptr;
  }
  img_at = (BmfImage *)((uint8_t *)__alloc_image(__frame.hdr_words[0], __frame.hdr_words[1], __frame.depth_b & 0x3F, (uint8_t)(__frame.depth_b & 0x80) >> 7, 1));
  __frame.depth_f = __frame.depth_b;
  img_at->depth = __frame.depth_b;
  // The flag is whether there is a block at all; Hex-Rays kept the pointer's
  // low byte and then overwrote it with 1, which is the same thing said twice.
  has_coded = p_coded_buf && *p_coded_buf;
  hdr_flags = __frame.flags_b;
  img_at->flags |= __frame.flags_b & 2 | (has_coded << 7);
  ::plane_count = ((__frame.depth_f & 0x3Fu) + 7) >> 3;
  if ( (hdr_flags & 0x20) == 0 )
  {
    want = __frame.data_len;
    if ( fread(img_at->pixels, 1u, __frame.data_len, ((BmfArc *)arc)->fp) != want )
      {
        fclose(((BmfArc *)arc)->fp);
        ((BmfArc *)arc)->fp = 0;
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
  if ( (hdr_flags & 4) == 0 )
  {
    printf("\nwritten in fast mode; this build only decodes -S streams\n");
    exit(3);
  }
  desc_slow_mode = 1;
  coded_size = __frame.data_len;
  ::coded_buf = (uint8_t *)bmf_new(__frame.data_len);
  out_cursor = ::coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  ::packer_word = (uint32_t *)::coded_buf;
  hist_scratch = ::coded_buf + coded_size - 4096;
  want2 = __frame.data_len;
  if ( fread(::coded_buf, 1u, __frame.data_len, ((BmfArc *)arc)->fp) != want2 )
  {
    fclose(((BmfArc *)arc)->fp);
    ((BmfArc *)arc)->fp = 0;
    return nullptr;
  }
  bpp = img_at->depth;
  if ( (bpp & 0x3Fu) <= 4 || (__frame.flags_b & 0x10) == 0 )
  {
    ::plane_predictor = 0;
    plane_alt_model = 0;
    // always taken: -S
      __unmodel_plane(want2, (uint16_t *)img_at, img_at->pixels);
    goto LABEL_106;
  }
  if ( ::plane_count == 1 )
  {
    if ( (bpp & 0x40) == 0 )
      goto LABEL_42;
  }
  else if ( ::plane_count <= 2 )
  {
    goto LABEL_42;
  }
  packer_free_bits -= 4;
  if ( packer_free_bits < 0 )
  {
    word4b = *(uint32_t *)out_cursor;
    out_cursor += 4;
    want2 = word4b >> (-packer_free_bits & 31);
    near_lossless = packer_acc | (word4b << ((packer_free_bits + 4) & 31)) & 0xF;
    packer_acc = want2;
    packer_free_bits += 32;
  }
  else
  {
    near_lossless = packer_acc & 0xF;
    packer_acc = packer_acc >> 4;
  }
  // The 4-bit near-lossless field, ALGORITHM.md §4.1.  This build compresses
  // with -E0 and the code that reconstructs a quantised plane is gone with the
  // rest of the modes, so a stream that asks for E>0 is refused rather than
  // expanded wrongly.
  if ( near_lossless )
  {
    printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
    exit(3);
  }
  plane_desc[0].w12 = 0;
LABEL_42:
  if ( ::plane_count > 0 )
  {
    LOBYTE(want2) = 63;
    __frame.mask = 255;
    __frame.p_i_2 = img_at;
    __frame.arc_f = arc;
    pl = 0;
    do
    {
      packer_free_bits -= 6;
      if ( packer_free_bits < 0 )
      {
        word6 = *(uint32_t *)out_cursor;
        out_cursor += 4;
        desc = packer_acc | (word6 << ((packer_free_bits + 6) & 31)) & 0x3F;
        packer_acc = word6 >> (-packer_free_bits & 31);
        packer_free_bits += 32;
      }
      else
      {
        desc = packer_acc & 0x3F;
        packer_acc = packer_acc >> 6;
      }
      desc_flags = desc >> 2;
      predictor = desc & 3;
      plane_desc[pl + 1].flags = desc_flags;
      plane_desc[pl + 1].predictor = predictor;
      plane_desc[predictor + 1].src_plane = pl;
      if ( (plane_desc[pl + 1].flags & 8) != 0 )
      {
        packer_free_bits -= 8;
        if ( packer_free_bits < 0 )
        {
          worddc = *(uint32_t *)out_cursor;
          out_cursor += 4;
          dc_v = packer_acc | __frame.mask & (worddc << ((packer_free_bits + 8) & 31));
          packer_acc = worddc >> (-packer_free_bits & 31);
          packer_free_bits += 32;
        }
        else
        {
          dc_v = packer_acc & (uint8_t)__frame.mask;
          packer_acc = packer_acc >> 8;
        }
        plane_desc[pl + 1].b3 = dc_v;
        if ( predictor > 1 )
        {
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            word4 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            w4_v = packer_acc | __frame.mask & (word4 << ((packer_free_bits + 8) & 31));
            packer_acc = word4 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            w4_v = packer_acc & __frame.mask;
            packer_acc = packer_acc >> 8;
          }
          plane_desc[pl + 1].w4 = w4_v - 64;
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            word8 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            w8_v = packer_acc | __frame.mask & (word8 << ((packer_free_bits + 8) & 31));
            packer_acc = word8 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            w8_v = packer_acc & __frame.mask;
            packer_acc = packer_acc >> 8;
          }
          plane_desc[pl + 1].w8 = w8_v - 64;
          if ( predictor > 2 )
          {
            packer_free_bits -= 8;
            if ( packer_free_bits < 0 )
            {
              word12 = *(uint32_t *)out_cursor;
              out_cursor += 4;
              w12_v = packer_acc | __frame.mask & (word12 << ((packer_free_bits + 8) & 31));
              packer_acc = word12 >> (-packer_free_bits & 31);
              packer_free_bits += 32;
            }
            else
            {
              w12_v = packer_acc & __frame.mask;
              packer_acc = packer_acc >> 8;
            }
            plane_desc[pl + 1].w12 = w12_v - 64;
          }
        }
      }
      ++pl;
    }
    while ( pl < ::plane_count );
    img_at = __frame.p_i_2;
    arc = __frame.arc_f;
  }
  plane_buf = (uint8_t *)bmf_new(img_at->width * img_at->height);
  if ( (__frame.flags_b & 8) != 0 )
  {
    // The caller's header with the depth byte replaced, exactly as
    // `model_planes` does it: 72 is 8 bits plus the 0x40 flag.
    __frame.img = *img_at;
    __frame.img.depth = 72;
    if ( ::plane_count > 0 )
    {
      __frame.arc_f = arc;
      plane_i = 0;
      do
      {
        plane = plane_desc[plane_i + 1].src_plane;
        ::plane_predictor = plane_desc[plane + 1].flags & 3;
        plane_alt_model = (uint8_t)(plane_desc[plane + 1].flags & 4) >> 2;
        // always taken: -S
          __unmodel_plane(plane_i, (uint16_t *)&__frame.img, plane_buf);
        if ( ::plane_predictor )
        {
          if ( ::plane_predictor == 1 )
          {
            if ( !plane_alt_model )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          }
          // `else if ( !desc_slow_mode && ::plane_predictor == 2 )` -- the fast-mode
          // predictor-2 expander, never reached: -S is on.
        }
        else
        {
          __expand_predictor_mode0((uint32_t)plane_buf, img_at->width, img_at->height);
        }
        // `v34` here and `v35` below were declared and never assigned: two more
        // uninitialised bytes into a parameter `interleave_plane` does not read.
        __interleave_plane((uint8_t *)img_at, plane_buf, plane, 0);
        ++plane_i;
      }
      while ( plane_i < ::plane_count );
LABEL_104:
      arc = __frame.arc_f;
    }
  }
  else
  {
    ::plane_predictor = plane_desc[1].flags & 3;
    plane_alt_model = (uint8_t)(plane_desc[1].flags & 4) >> 2;
    // always taken: -S
    {
      __unmodel_plane(want2, (uint16_t *)img_at, img_at->pixels);
      if ( plane_alt_model )
        goto LABEL_105;
    }
    if ( ::plane_count > 0 )
    {
      __frame.arc_f = arc;
      pl2 = 0;
      do
      {
        ++pl2;
        plane2 = BYTE1(plane_desc[pl2].w0);
        pred = plane_desc[plane2 + 1].flags & 3;
        ::plane_predictor = pred;
        if ( (plane_desc[plane2 + 1].flags & 8) != 0 || pred )
        {
          i = img_at->width;
          __frame.row_step = &((uint8_t *)img_at)[plane2 + 16];
          n_pix = i * img_at->height;
          __frame.nplanes_s = ::plane_count;
          __frame.s12 = n_pix;
          if ( ::plane_count == 1 )
          {
            memcpy(plane_buf,(uint8_t *)__frame.row_step,__frame.s12);
            pred_s = ::plane_predictor;
          }
          else
          {
            __frame.Block = &((uint8_t *)img_at)[plane2];
            __frame.s0 = pred;
            __frame.s4 = plane2;
            __frame.s8 = pl2;
            n_pix2 = __frame.s12;
            __frame.p_i_2 = img_at;
            pix_at = 0;
            nplanes_c = __frame.nplanes_s;
            i2 = 0;
            do
            {
              plane_buf[pix_at] = __frame.Block[i2 + 16];
              i2 += nplanes_c;
              ++pix_at;
            }
            while ( pix_at < n_pix2 );
            pred_s = __frame.s0;
            plane2 = __frame.s4;
            pl2 = __frame.s8;
            img_at = __frame.p_i_2;
          }
          if ( pred_s )
          {
            if ( pred_s == 1 )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          }
          else
          {
            __expand_predictor_mode0((uint32_t)plane_buf, img_at->width, img_at->height);
          }
          __interleave_plane((uint8_t *)img_at, plane_buf, plane2, 0);
        }
      }
      while ( pl2 < ::plane_count );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(plane_buf);
LABEL_106:
  if ( ::coded_buf + __frame.data_len != out_cursor )
  {
    fclose(((BmfArc *)arc)->fp);
    ((BmfArc *)arc)->fp = 0;
    return nullptr;
  }
  free(::coded_buf);
  __frame.depth_f = __frame.depth_b;
LABEL_109:
  if ( (__frame.depth_f & 0x80) != 0 )
  {
    // `f10 < 0` was a signed int8_t testing its own top bit -- the palette
    // flag.  depth is unsigned, so the test has to name the bit; it read as
    // always-false otherwise, which is what the gate caught.
    pal_at = (img_at->depth & 0x80) ? &((uint8_t *)img_at)[img_at->data_size + 16] : nullptr;
    got = fread(pal_at, 1u, __frame.pal_len, ((BmfArc *)arc)->fp);
    if ( got != __frame.pal_len )
      {
        fclose(((BmfArc *)arc)->fp);
        ((BmfArc *)arc)->fp = 0;
        return nullptr;
      }
  }
  if ( (img_at->flags & 2) != 0 )
  {
    copy = (uint8_t *)bmf_new(img_at->data_size);
    n_planes = ::plane_count;
    __frame.nplanes_s = (int32_t)copy;
    last_row = ::plane_count * (img_at->height - 1);
    memcpy(copy,img_at->pixels,img_at->data_size);
    LOWORD(img_h) = img_at->height;
    if ( (uint16_t)img_h )
    {
      __frame.row_step = (void *)last_row;
      srcp = (uint8_t *)__frame.nplanes_s;
      at = 0;
      y = 0;
      do
      {
        pl_i = img_at->width;
        __frame.nplanes_s = at;
        __frame.arc_f = (uint8_t *)y;
        last_row2 = __frame.row_step;
        __frame.p_i_2 = img_at;
        dst = &((uint8_t *)img_at)[at + 16];
        do
        {
          left = n_planes;
          do
          {
            *dst++ = *srcp++;
            --left;
          }
          while ( left );
          dst = &dst[(uint32_t)last_row2];
          --pl_i;
        }
        while ( pl_i );
        img_at = __frame.p_i_2;
        img_h = *((uint16_t *)__frame.p_i_2 + 1);
        at = n_planes + __frame.nplanes_s;
        y = (int32_t)(uintptr_t)__frame.arc_f + 1;
      }
      while ( __frame.arc_f + 1 < (uint8_t *)(uintptr_t)img_h );
    }
    // The deinterleave swaps width and height and rewrites the stride; only
    // the low half of `stride` is touched, which is what `(uint16_t *)p_i + 2`
    // was addressing.
    w16 = img_at->width;
    img_at->width = img_h;
    img_at->flags ^= 2u;
    img_at->height = w16;
    *(uint16_t *)&img_at->stride = img_h * n_planes;
    free(copy);
  }
  return (uint8_t *)img_at;
}


// Choose each plane's coding by *trying* it: set the descriptor's flags, run
// `model_planes` over a tile of the image, measure how many bits came out, and
// keep the cheapest setting.  The coder's own output is the cost function --
// there is no model of compressibility here, only the compressor run at each
// candidate and the byte count read back off `out_cursor`.

// The tile is the whole image at the default quality (`-Q 9`); a lower -Q
// capped it, and that cap is gone with the mode.  After each trial the packer
// is rewound -- `out_cursor`, `packer_word`, `packer_acc`, `packer_free_bits`
// and `hist_scratch` all reset -- which is why the same five lines recur
// between candidates.

// `choose_plane_coding` is the other half: this one picks flags per plane,
// that one picks the plane pairing they apply to.
uint32_t __search_filter(BmfImage *img, int8_t mode)
{
  // This one is a layout, not a bag of locals: lifting its members to
  // ordinary locals makes altp1 abort while compressing.  Re-checked against the
  // file as it is now, by `tools/liftframe.py` -- `frame-sweep.sh`, which
  // the note here used to cite, lifts aliases and has none left to lift.
  struct alignas(16) SearchFilterFrame {   // 164 bytes, one stack frame
      uint8_t *base;
      uint8_t marks[16];
      uint8_t *rows[2];
      int32_t dims[2];
      int32_t plane_i;
      uint32_t costs[4];
      int32_t pi;
      void *tile_buf;
      int32_t flag8;
      uint8_t *best_flags;
      uint8_t _pad0[28];
      void *tile_src;
      uint8_t *n_hard;
      int32_t bits_total;
      int32_t n_flagged;
      uint8_t *best_bits;
      BmfImage *p_i_2;
      uint8_t *tile_img;
      uint8_t _pad1[36];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  // These shared `__frame.base` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *base2;
  ;
  bool deep, deep2;
  // `v19`, `v44`, `v63`, `v82` and `v93` were declared and never assigned,
  // and every one was the last argument to `model_planes` or
  // `transform_planes` -- which neither reads.  Nine more uninitialised
  // bytes travelling to be dropped, on top of section 11's six.
  int8_t f4;
  uint16_t w_a, w_d;
  BmfImage *img_c;
  uint8_t f0, f1, f2, f3;
  uint8_t *srcp, *y, *dstp, *hard, *bits_a, *p0, *p1, *bits_b, *p2, *p3,
          *tile_a, *p4, *p5, *p6, *tile_b, *p7, *p10, *p11, *tile_c,
          *p12, *p13, *p14, *tile_d, *cost_f13, *flags_s;   // `uint8_t *` beside the `char` scalars above
  int16_t w_b, w_e;
  int32_t tile_w, tile_h, pl, nplanes, y0, dx, off_y, row_bytes, best_cost, bits_f5,
          cost_f5, f5, pred, c0, c1, c2, c3, cand, sv1, nplanes_c, pl_a,
          pi0, sv2, pl_b, pi1, bits2, sv0, plane, pi3, bits_e, nplanes_b, sv3,
          pl_c, pi4, sv4, pl_d, bits_c, img_h1, img_h1b, y1, off1, x1,
          bits_d, pk, pl_k, bits, img_h3, img_h3b, y3, off3, pl_i, rows_x_planes, img_h2,
          y2, off2, x2, pl2, bits_f8, cost_f8, f8, pl3,
          bits_f13, f13, pl4, cost_f14, f14, pl0,
          bits_f6, cost_f6, f6, pl1, bits_f0;
  uint16_t w16, w_c;
  uint32_t rs2, rs1, rs0, rs3, rs4;
  uint8_t *p8, *p9;
  img_c = (BmfImage *)(img);
  tile_w = img->width;
  tile_h = img->height;
  // `if ( (uint32_t)opt_search_quality < 9 )` -- the tile-size cap that a -Q below 9 put on
  // the filter search.  -Q is 9, so the search sees the whole image.
  if ( tile_w < 4 || tile_h < 3 )
  {
    if ( ::plane_count > 0 )
    {
      pl = 0;
      do
      {
        plane_desc[pl + 1].flags = 0;
        plane_desc[pl + 1].src_plane = pl;
        plane_desc[pl + 1].predictor = pl;
        ++pl;
      }
      while ( pl < ::plane_count );
    }
    return 0;
  }
  __choose_plane_coding((BmfImage *)img_c, tile_h, mode);
  // `if ( opt_filter_template == 2 )` -- 94 lines of the -T2 filter-template path, gone
  // with the mode.  See REFACTORING.md §2.
  __frame.tile_img = (uint8_t *)__alloc_image(tile_w, tile_h, img_c->depth & 0x3F, 0, 0);
  coded_size = *((uint32_t *)__frame.tile_img + 3) + 0x20000;
  coded_buf = (uint8_t *)bmf_new(coded_size);
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t *)coded_buf;
  __frame.dims[0] = tile_h * tile_w;
  hist_scratch = coded_buf + coded_size - 4096;
  __frame.tile_buf = bmf_new(tile_h * tile_w);
  nplanes = ::plane_count;
  y0 = (img_c->height - tile_h) >> 1;
  dx = img_c->width - tile_w;
  off_y = y0 * img_c->stride;
  __frame.rows[1] = (uint8_t *)y0;
  srcp = (uint8_t *)img_c + ::plane_count * (dx >> 1) + off_y + 16;
  __frame.tile_src = __frame.tile_img + 16;
  __frame.rows[0] = __frame.tile_img + 16;
  if ( y0 < tile_h + y0 )
  {
    row_bytes = *((uint16_t *)__frame.tile_img + 2);
    __frame.dims[1] = tile_h;
    y = __frame.rows[1];
    __frame.p_i_2 = (BmfImage *)(img_c);
    dstp = __frame.rows[0];
    do
    {
      memcpy(dstp,srcp,row_bytes);
      row_bytes = *((uint16_t *)__frame.tile_img + 2);
      dstp += row_bytes;
      srcp += __frame.p_i_2->stride;
      ++y;
    }
    while ( (int32_t)y < __frame.dims[1] + ((__frame.p_i_2->height - __frame.dims[1]) >> 1) );
    img_c = (BmfImage *)(__frame.p_i_2);
    nplanes = ::plane_count;
  }
  __frame.n_hard = nullptr;
  __frame.bits_total = 0;
  __frame.n_flagged = 0;
  __frame.best_bits = nullptr;
  if ( nplanes > 0 )
  {
    __frame.p_i_2 = (BmfImage *)(img_c);
    hard = __frame.n_hard;
    __frame.best_flags = nullptr;
    __frame.pi = 0;
    __frame.dims[0] *= 8;
    while ( 1 )
    {
      __frame.dims[1] = plane_desc[__frame.pi + 1].src_plane;
      __frame.plane_i = __frame.dims[1];   // a record index; it was 16 * it, the byte offset
      if ( hard )
      {
        best_cost = 0x7FFFFFFF;
      }
      else
      {
        pl1 = __frame.dims[1];
        plane_desc[__frame.dims[1] + 1].flags = 0;
        __model_planes(__frame.tile_img, (uint8_t *)__frame.tile_buf, pl1, 0);
        bits_f0 = 8 * (out_cursor - coded_buf);
        best_cost = plane_desc[0].w0 - packer_free_bits + bits_f0 + 32;
        deep = 0;                            // -S
        *(uint32_t *)packer_word = packer_acc;
        if ( !deep )
          best_cost = bits_f0;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        flags_s = __frame.best_flags;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( best_cost == 0x7FFFFFFF )
          best_cost = 0x7FFFFFFF;
        else
          flags_s = nullptr;
        __frame.best_flags = flags_s;
      }
      plane_desc[__frame.plane_i + 1].flags = 5;
      __model_planes(__frame.tile_img, (uint8_t *)__frame.tile_buf, __frame.dims[1], 0);
      bits_f5 = 8 * (out_cursor - coded_buf);
      cost_f5 = plane_desc[0].w0 - packer_free_bits + bits_f5 + 32;
      deep = 0;                            // -S
      *(uint32_t *)packer_word = packer_acc;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      if ( !deep )
        cost_f5 = bits_f5;
      packer_free_bits = 0;
      packer_acc = 0;
      hist_scratch = coded_buf + coded_size - 4096;
      f5 = (int32_t)__frame.best_flags;
      if ( cost_f5 < best_cost )
      {
        best_cost = cost_f5;
        f5 = 5;
      }
      __frame.best_flags = (uint8_t *)f5;
      if ( cost_f5 < best_cost + (best_cost >> 5) || hard )
      {
        pl0 = __frame.dims[1];
        plane_desc[__frame.plane_i + 1].flags = 6;
        __model_planes(__frame.tile_img, (uint8_t *)__frame.tile_buf, pl0, 0);
        bits_f6 = 8 * (out_cursor - coded_buf);
        cost_f6 = plane_desc[0].w0 - packer_free_bits + bits_f6 + 32;
        deep = 0;                            // -S
        *(uint32_t *)packer_word = packer_acc;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        if ( !deep )
          cost_f6 = bits_f6;
        packer_free_bits = 0;
        packer_acc = 0;
        f6 = (int32_t)__frame.best_flags;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( cost_f6 < best_cost )
        {
          best_cost = cost_f6;
          f6 = 6;
        }
        __frame.best_flags = (uint8_t *)f6;
        if ( __frame.pi )
        {
          // always taken: -Q is 9, so `opt_search_quality > 5` decides it whatever hard is
          {
LABEL_191:
            pl2 = __frame.dims[1];
            plane_desc[__frame.plane_i + 1].flags = 8;
            __model_planes(__frame.tile_img, (uint8_t *)__frame.tile_buf, pl2, 0);
            bits_f8 = 8 * (out_cursor - coded_buf);
            cost_f8 = plane_desc[0].w0 - packer_free_bits + bits_f8 + 32;
            deep = 0;                            // -S
            *(uint32_t *)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            if ( !deep )
              cost_f8 = bits_f8;
            packer_free_bits = 0;
            packer_acc = 0;
            f8 = (int32_t)__frame.best_flags;
            hist_scratch = coded_buf + coded_size - 4096;
            if ( cost_f8 < best_cost )
            {
              best_cost = cost_f8;
              f8 = 8;
            }
            __frame.best_flags = (uint8_t *)f8;
          }
          pl3 = __frame.dims[1];
          plane_desc[__frame.plane_i + 1].flags = 13;
          __model_planes(__frame.tile_img, (uint8_t *)__frame.tile_buf, pl3, 0);
          bits_f13 = 8 * (out_cursor - coded_buf);
          cost_f13 = (uint8_t *)(plane_desc[0].w0 - packer_free_bits + bits_f13 + 32);
          deep = 0;                            // -S
          *(uint32_t *)packer_word = packer_acc;
          out_cursor = coded_buf;
          packer_word = (uint32_t *)coded_buf;
          if ( !deep )
            cost_f13 = (uint8_t *)bits_f13;
          __frame.rows[0] = cost_f13;
          packer_free_bits = 0;
          packer_acc = 0;
          f13 = (int32_t)__frame.best_flags;
          hist_scratch = coded_buf + coded_size - 4096;
          if ( (int32_t)cost_f13 < best_cost )
          {
            best_cost = (int32_t)cost_f13;
            f13 = 13;
          }
          __frame.best_flags = (uint8_t *)f13;
          pred = f13 & 3;
          if ( pred == 2 || best_cost + (best_cost >> 5) > (int32_t)__frame.rows[0] )
          {
            pl4 = __frame.dims[1];
            plane_desc[__frame.plane_i + 1].flags = 14;
            __model_planes((uint8_t *)__frame.tile_img, (uint8_t *)__frame.tile_buf, pl4, 0);
            cost_f14 = plane_desc[0].w0 - packer_free_bits + 8 * (out_cursor - coded_buf) + 32;
            // always taken: -S
              cost_f14 = 8 * (out_cursor - coded_buf);
            deep2 = cost_f14 < best_cost;
            if ( cost_f14 < best_cost )
              best_cost = cost_f14;
            *(uint32_t *)packer_word = packer_acc;
            packer_free_bits = 0;
            packer_acc = 0;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            hist_scratch = coded_buf + coded_size - 4096;
            f14 = (int32_t)__frame.best_flags;
            if ( deep2 )
              f14 = 14;
            __frame.best_flags = (uint8_t *)f14;
            if ( deep2 )
              pred = 2;
          }
          goto LABEL_43;
        }
        pred = f6 & 3;
      }
      else
      {
        if ( __frame.pi )
          goto LABEL_191;
        pred = (uint8_t)(uintptr_t)__frame.best_flags & 3;
      }
LABEL_43:
      // always taken: -S
      {
        __frame.flag8 = (uint8_t)(uintptr_t)__frame.best_flags & 8;
      }
      __frame.best_bits += best_cost;
      c0 = pred == 2;
      if ( pred != 1 )
        pred = 0;
      c1 = __frame.plane_i;
      hard = (uint8_t *)((uintptr_t)hard + (c0));
      __frame.bits_total += pred;
      c2 = __frame.flag8;
      __frame.costs[__frame.dims[1]] = best_cost;
      c3 = __frame.pi;
      __frame.n_flagged += c2 != 0;
      plane_desc[c1 + 1].flags = (uint8_t)(uintptr_t)__frame.best_flags;
      __frame.pi = c3 + 1;
      if ( c3 + 1 >= ::plane_count )
      {
        __frame.n_hard = hard;
        img_c = (BmfImage *)(__frame.p_i_2);
        break;
      }
    }
  }
  // always taken: -Q is 9
  {
    p2 = (uint8_t *)bmf_new(*((uint32_t *)__frame.tile_img + 3));
    img_h1 = *((uint16_t *)__frame.tile_img + 1);
    __frame.rows[1] = (uint8_t *)::plane_count;
    __frame.plane_i = (int32_t)p2;
    __frame.pi = ::plane_count * (img_h1 - 1);
    memcpy(p2,(uint8_t *)__frame.tile_src,*((uint32_t *)__frame.tile_img + 3));
    LOWORD(img_h1b) = *((uint16_t *)__frame.tile_img + 1);
    if ( (uint16_t)img_h1b )
    {
      __frame.rows[0] = p2;
      p3 = __frame.rows[1];
      __frame.p_i_2 = (BmfImage *)(img_c);
      y1 = 0;
      tile_a = __frame.tile_img;
      off1 = 0;
      do
      {
        x1 = ((const BmfImage *)tile_a)->width;
        __frame.dims[1] = off1;
        __frame.dims[0] = y1;
        p4 = &tile_a[off1 + 16];
        p5 = (uint8_t *)__frame.plane_i;
        do
        {
          p6 = p3;
          do
          {
            *p4++ = *p5++;
            p6 = (uint8_t *)((uintptr_t)p6 - 1);
          }
          while ( p6 );
          p4 += __frame.pi;
          --x1;
        }
        while ( x1 );
        tile_a = __frame.tile_img;
        img_h1b = *((uint16_t *)__frame.tile_img + 1);
        __frame.plane_i = (int32_t)p5;
        off1 = (int32_t)&p3[__frame.dims[1]];
        y1 = __frame.dims[0] + 1;
      }
      while ( __frame.dims[0] + 1 < img_h1b );
      p2 = __frame.rows[0];
      img_c = (BmfImage *)(__frame.p_i_2);
    }
    tile_b = __frame.tile_img;
    w_a = ((const BmfImage *)__frame.tile_img)->width;
    w_b = img_h1b * LOWORD(__frame.rows[1]);
    ((BmfImage *)__frame.tile_img)->width = img_h1b;
    ((BmfImage *)tile_b)->height = w_a;
    ((BmfImage *)tile_b)->flags ^= 2u;
    *(uint16_t *)&((BmfImage *)tile_b)->stride = w_b;
    free(p2);
    bits_d = 0;
    if ( ::plane_count > 0 )
    {
      __frame.p_i_2 = (BmfImage *)(img_c);
      pk = 0;
      while ( 1 )
      {
        pl_k = plane_desc[pk + 1].src_plane;
        __frame.rows[0] = (uint8_t *)pl_k;
        __model_planes((uint8_t *)__frame.tile_img, (uint8_t *)__frame.tile_buf, pl_k, f4);
        bits = 8 * (out_cursor - coded_buf);
        // never taken: -S
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        bits_d += bits;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( (uint32_t)(bits - (bits >> 8)) > __frame.costs[(int32_t)__frame.rows[0]] )
          break;
        if ( ++pk >= ::plane_count )
        {
          img_c = (BmfImage *)(__frame.p_i_2);
          goto LABEL_172;
        }
      }
      img_c = (BmfImage *)(__frame.p_i_2);
      bits_d += (int32_t)(__frame.best_bits + 1);
    }
LABEL_172:
    if ( bits_d + (bits_d >> 12) >= (int32_t)__frame.best_bits )
    {
      __frame.rows[0] = (uint8_t *)bmf_new(*((uint32_t *)__frame.tile_img + 3));
      rows_x_planes = ::plane_count * (*((uint16_t *)__frame.tile_img + 1) - 1);
      __frame.rows[1] = (uint8_t *)::plane_count;
      __frame.plane_i = (int32_t)__frame.rows[0];
      memcpy(__frame.rows[0],(uint8_t *)__frame.tile_src,*((uint32_t *)__frame.tile_img + 3));
      LOWORD(img_h2) = *((uint16_t *)__frame.tile_img + 1);
      if ( (uint16_t)img_h2 )
      {
        __frame.costs[0] = rows_x_planes;
        p11 = __frame.rows[1];
        __frame.p_i_2 = (BmfImage *)(img_c);
        y2 = 0;
        tile_c = __frame.tile_img;
        off2 = 0;
        do
        {
          x2 = ((const BmfImage *)tile_c)->width;
          __frame.dims[1] = off2;
          __frame.dims[0] = y2;
          p12 = &tile_c[off2 + 16];
          p13 = (uint8_t *)__frame.plane_i;
          do
          {
            p14 = p11;
            do
            {
              *p12++ = *p13++;
              p14 = (uint8_t *)((uintptr_t)p14 - 1);
            }
            while ( p14 );
            p12 += __frame.costs[0];
            --x2;
          }
          while ( x2 );
          tile_c = __frame.tile_img;
          img_h2 = *((uint16_t *)__frame.tile_img + 1);
          __frame.plane_i = (int32_t)p13;
          off2 = (int32_t)&p11[__frame.dims[1]];
          y2 = __frame.dims[0] + 1;
        }
        while ( __frame.dims[0] + 1 < img_h2 );
        img_c = (BmfImage *)(__frame.p_i_2);
      }
      tile_d = __frame.tile_img;
      base2 = __frame.rows[0];
      w_d = ((const BmfImage *)__frame.tile_img)->width;
      w_e = img_h2 * LOWORD(__frame.rows[1]);
      ((BmfImage *)__frame.tile_img)->width = img_h2;
      ((BmfImage *)tile_d)->height = w_d;
      ((BmfImage *)tile_d)->flags ^= 2u;
      *(uint16_t *)&((BmfImage *)tile_d)->stride = w_e;
      free(base2);
    }
    else
    {
      __frame.best_bits = (uint8_t *)bits_d;
      __frame.rows[0] = (uint8_t *)bmf_new(img_c->data_size);
      img_h3 = img_c->height;
      __frame.rows[1] = (uint8_t *)::plane_count;
      __frame.dims[0] = (int32_t)__frame.rows[0];
      __frame.costs[0] = ::plane_count * (img_h3 - 1);
      memcpy(__frame.rows[0],img_c->pixels,img_c->data_size);
      LOWORD(img_h3b) = img_c->height;
      if ( (uint16_t)img_h3b )
      {
        p7 = __frame.rows[1];
        y3 = 0;
        off3 = 0;
        do
        {
          pl_i = img_c->width;
          __frame.dims[1] = off3;
          __frame.plane_i = y3;
          p8 = (uint8_t *)img_c + off3 + 16;
          __frame.p_i_2 = (BmfImage *)(img_c);
          p9 = (uint8_t *)__frame.dims[0];
          do
          {
            p10 = p7;
            do
            {
              *p8++ = *p9++;
              p10 = (uint8_t *)((uintptr_t)p10 - 1);
            }
            while ( p10 );
            p8 += __frame.costs[0];
            --pl_i;
          }
          while ( pl_i );
          __frame.dims[0] = (int32_t)p9;
          img_c = (BmfImage *)(__frame.p_i_2);
          img_h3b = __frame.p_i_2->height;
          off3 = (int32_t)&p7[__frame.dims[1]];
          y3 = __frame.plane_i + 1;
        }
        while ( __frame.plane_i + 1 < img_h3b );
      }
      __frame.base = __frame.rows[0];
      w16 = img_c->width;
      w_c = img_h3b * LOWORD(__frame.rows[1]);
      img_c->width = img_h3b;
      img_c->height = w16;
      img_c->flags ^= 2u;
      img_c->stride = w_c;
      free(__frame.base);
    }
  }
  free(__frame.tile_buf);
  if ( ::plane_count > 2 )
  {
    if ( __frame.bits_total )
    {
      sv0 = 16;
      do
      {
        __frame.dims[sv0 + 1] = plane_desc[sv0 / 4].w12;
        __frame.dims[sv0] = *(int32_t *)((uint8_t *)&::plane_count + sv0 * 4);
        __frame.rows[sv0 + 1] = (uint8_t *)plane_desc[sv0 / 4].w4;
        p1 = (uint8_t *)plane_desc[sv0 / 4].w0;
        __frame.rows[sv0] = p1;
        sv0 -= 4;
      }
      while ( sv0 * 4 );
      if ( ::plane_count > 0 )
      {
        plane = 0;
        do
        {
          pi3 = plane;   // a record index; it was 16 * it
          f2 = plane_desc[plane++ + 1].flags & 8 | 5;
          plane_desc[pi3 + 1].flags = f2;
        }
        while ( plane < ::plane_count );
      }
      __transform_planes((BmfImage *)__frame.tile_img, (int32_t)p1, 0);
      bits_e = 8 * (out_cursor - coded_buf);
      // never taken: -S
      deep2 = bits_e <= (int32_t)__frame.best_bits;
      *(uint32_t *)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      hist_scratch = coded_buf + coded_size - 4096;
      if ( deep2 )
      {
        __frame.best_bits = (uint8_t *)bits_e;
        nplanes_b = ::plane_count;
        cand = 0;
      }
      else
      {
        rs0 = 64;
        do
        {
          *(uint64_t *)(bmf_plane_desc(rs0 - 8)) = *(uint64_t *)&__frame.dims[rs0 / 4];
          *(uint64_t *)(bmf_plane_desc(rs0 - 16)) = *(uint64_t *)&__frame.rows[rs0 / 4];
          *(uint64_t *)(bmf_plane_desc(rs0 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs0 / 8 + 1];
          *(uint64_t *)(bmf_plane_desc(rs0 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs0 / 8];
          rs0 -= 32;
        }
        while ( rs0 );
        nplanes_b = ::plane_count;
        cand = 1;
      }
      if ( nplanes_b <= 2 )
        goto LABEL_63;
    }
    else
    {
      cand = 1;
    }
    if ( &__frame.n_hard[cand == 0] )
    {
      sv1 = 16;
      do
      {
        __frame.dims[sv1 + 1] = plane_desc[sv1 / 4].w12;
        __frame.dims[sv1] = *(int32_t *)((uint8_t *)&::plane_count + sv1 * 4);
        __frame.rows[sv1 + 1] = (uint8_t *)plane_desc[sv1 / 4].w4;
        __frame.rows[sv1] = (uint8_t *)plane_desc[sv1 / 4].w0;
        sv1 -= 4;
      }
      while ( sv1 * 4 );
      nplanes_c = ::plane_count;
      if ( ::plane_count > 0 )
      {
        pl_a = 0;
        do
        {
          pi0 = pl_a;   // a record index; it was 16 * it
          f0 = plane_desc[pl_a++ + 1].flags & 8 | 6;
          plane_desc[pi0 + 1].flags = f0;
          nplanes_c = ::plane_count;
        }
        while ( pl_a < ::plane_count );
      }
      if ( (uint8_t *)nplanes_c == __frame.n_hard && nplanes_c - 1 == __frame.n_flagged )
      {
        cand = 0;
      }
      else
      {
        __transform_planes((BmfImage *)__frame.tile_img, cand, 0);
        bits_a = (uint8_t *)(8 * (out_cursor - coded_buf));
        // never taken: -S
        deep2 = (int32_t)bits_a <= (int32_t)__frame.best_bits;
        __frame.rows[0] = bits_a;
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( deep2 )
        {
          __frame.best_bits = bits_a;
          __frame.n_hard = bits_a;
          if ( ::plane_count - 1 == __frame.n_flagged )
          {
            cand = 0;
          }
          else
          {
            sv2 = 16;
            do
            {
              __frame.dims[sv2 + 1] = plane_desc[sv2 / 4].w12;
              __frame.dims[sv2] = *(int32_t *)((uint8_t *)&::plane_count + sv2 * 4);
              __frame.rows[sv2 + 1] = (uint8_t *)plane_desc[sv2 / 4].w4;
              p0 = (uint8_t *)plane_desc[sv2 / 4].w0;
              __frame.rows[sv2] = p0;
              sv2 -= 4;
            }
            while ( sv2 * 4 );
            if ( ::plane_count > 0 )
            {
              pl_b = 0;
              do
              {
                pi1 = pl_b;   // a record index; it was 16 * it
                f1 = plane_desc[pl_b++ + 1].predictor;
                plane_desc[pi1 + 1].flags |= 8 * (f1 != 0);
              }
              while ( pl_b < ::plane_count );
            }
            __transform_planes((BmfImage *)__frame.tile_img, (int32_t)p0, 0);
            bits2 = 8 * (out_cursor - coded_buf);
            // never taken: -S
            deep2 = bits2 <= (int32_t)__frame.rows[0];
            *(uint32_t *)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t *)coded_buf;
            packer_free_bits = 0;
            packer_acc = 0;
            hist_scratch = coded_buf + coded_size - 4096;
            if ( deep2 )
            {
              cand = 0;
            }
            else
            {
              rs1 = 64;
              do
              {
                *(uint64_t *)(bmf_plane_desc(rs1 - 8)) = *(uint64_t *)&__frame.dims[rs1 / 4];
                *(uint64_t *)(bmf_plane_desc(rs1 - 16)) = *(uint64_t *)&__frame.rows[rs1 / 4];
                *(uint64_t *)(bmf_plane_desc(rs1 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs1 / 8 + 1];
                *(uint64_t *)(bmf_plane_desc(rs1 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs1 / 8];
                rs1 -= 32;
              }
              while ( rs1 );
              cand = 0;
            }
          }
        }
        else
        {
          rs2 = 64;
          do
          {
            *(uint64_t *)(bmf_plane_desc(rs2 - 8)) = *(uint64_t *)&__frame.dims[rs2 / 4];
            *(uint64_t *)(bmf_plane_desc(rs2 - 16)) = *(uint64_t *)&__frame.rows[rs2 / 4];
            *(uint64_t *)(bmf_plane_desc(rs2 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs2 / 8 + 1];
            *(uint64_t *)(bmf_plane_desc(rs2 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs2 / 8];
            rs2 -= 32;
          }
          while ( rs2 );
        }
      }
    }
  }
  else
  {
    cand = 1;
  }
LABEL_63:
  if ( !__frame.n_hard && ::plane_count > 1 )
  {
    __frame.rows[0] = (uint8_t *)bmf_new(*((uint32_t *)__frame.tile_img + 3));
    memcpy(__frame.rows[0],(uint8_t *)__frame.tile_src,*((uint32_t *)__frame.tile_img + 3));
    sv3 = 16;
    do
    {
      __frame.dims[sv3 + 1] = plane_desc[sv3 / 4].w12;
      __frame.dims[sv3] = *(int32_t *)((uint8_t *)&::plane_count + sv3 * 4);
      __frame.rows[sv3 + 1] = (uint8_t *)plane_desc[sv3 / 4].w4;
      __frame.rows[sv3] = (uint8_t *)plane_desc[sv3 / 4].w0;
      sv3 -= 4;
    }
    while ( sv3 * 4 );
    if ( ::plane_count > 0 )
    {
      pl_c = 0;
      do
      {
        pi4 = pl_c;   // a record index; it was 16 * it
        f3 = plane_desc[pl_c++ + 1].flags & 0xFB;
        plane_desc[pi4 + 1].flags = f3;
      }
      while ( pl_c < ::plane_count );
    }
    __transform_planes((BmfImage *)__frame.tile_img, cand, 0);
    bits_b = (uint8_t *)(8 * (out_cursor - coded_buf));
    // never taken: -S
    __frame.rows[1] = bits_b;
    *(uint32_t *)packer_word = packer_acc;
    packer_free_bits = 0;
    packer_acc = 0;
    out_cursor = coded_buf;
    packer_word = (uint32_t *)coded_buf;
    hist_scratch = coded_buf + coded_size - 4096;
    if ( (int32_t)bits_b > (int32_t)__frame.best_bits )
    {
      rs3 = 64;
      do
      {
        *(uint64_t *)(bmf_plane_desc(rs3 - 8)) = *(uint64_t *)&__frame.dims[rs3 / 4];
        *(uint64_t *)(bmf_plane_desc(rs3 - 16)) = *(uint64_t *)&__frame.rows[rs3 / 4];
        *(uint64_t *)(bmf_plane_desc(rs3 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs3 / 8 + 1];
        *(uint64_t *)(bmf_plane_desc(rs3 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs3 / 8];
        rs3 -= 32;
      }
      while ( rs3 );
    }
    else
    {
      __frame.best_bits = bits_b;
      cand = 0;
    }
    if ( __frame.n_flagged + __frame.bits_total )                // the left disjunct is -S, always true
    {
      sv4 = 16;
      do
      {
        __frame.dims[sv4 + 1] = plane_desc[sv4 / 4].w12;
        __frame.dims[sv4] = *(int32_t *)((uint8_t *)&::plane_count + sv4 * 4);
        __frame.rows[sv4 + 1] = (uint8_t *)plane_desc[sv4 / 4].w4;
        __frame.rows[sv4] = (uint8_t *)plane_desc[sv4 / 4].w0;
        sv4 -= 4;
      }
      while ( sv4 * 4 );
      if ( ::plane_count > 0 )
      {
        pl_d = 0;
        do
          plane_desc[pl_d++ + 1].flags = 0;
        while ( pl_d < ::plane_count );
      }
      memcpy((uint8_t *)__frame.tile_src,__frame.rows[0],*((uint32_t *)__frame.tile_img + 3));
      __transform_planes((BmfImage *)__frame.tile_img, cand, 0);
      bits_c = 8 * (out_cursor - coded_buf);
      // never taken: -S
      deep2 = bits_c <= (int32_t)__frame.best_bits;
      *(uint32_t *)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      hist_scratch = coded_buf + coded_size - 4096;
      if ( deep2 )
      {
        cand = 0;
      }
      else
      {
        rs4 = 64;
        do
        {
          *(uint64_t *)(bmf_plane_desc(rs4 - 8)) = *(uint64_t *)&__frame.dims[rs4 / 4];
          *(uint64_t *)(bmf_plane_desc(rs4 - 16)) = *(uint64_t *)&__frame.rows[rs4 / 4];
          *(uint64_t *)(bmf_plane_desc(rs4 - 24)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs4 / 8 + 1];
          *(uint64_t *)(bmf_plane_desc(rs4 - 32)) = (*(uint64_t (*)[2])((uint8_t *)__frame.marks))[rs4 / 8];
          rs4 -= 32;
        }
        while ( rs4 );
      }
    }
    free(__frame.rows[0]);
  }
  free(coded_buf);
  free(__frame.tile_img);
  // `if ( opt_filter_template == 1 )` -- 38 lines that built the -T1 filter template into
  // __dword_4410A4.  With -T off nothing writes that word, so it keeps the 0
  // BMF.exe's data segment starts it at, and the -T2 reader above is gone too.
  return cand;
}


BmfArc *__bmf_open_archive(BmfArc *out, char *path, int32_t read_only)
{
  ;
  BmfArc *arc;
  FILE *fp, *fp2;
  const char *mode;   // an fopen mode string, so `char` and not a byte
  int32_t rc, live;
  arc = out;
  // "a+b", as the original had it, and not "w+b".  An earlier pass here
  // reasoned that since the command line writes one image per run, appending
  // only meant that compressing twice to the same name grew the file instead
  // of replacing it.  That is what appending *is*: `bmf c a.bmp arc.bmf`
  // followed by `bmf c b.bmp arc.bmf` is how a multi-image archive is built,
  // and `bmf d` reads every member back -- it prints "number: 1", "number: 2"
  // and decodes both.  Opening "w+b" did not tidy a harness annoyance, it
  // removed half of a feature the format carries a flag for.
  
  // Not "wb" either: the pass below reads the stream back, and it is not only
  // walking the images already in the file -- it also sets up state the writer
  // goes on to use.
  mode = "a+b";
  if ( read_only )
    mode = "rb";
  out->images = 0;
  fp = fopen(path, mode);
  arc->fp = fp;
  if ( !fp )
    __exit_402E40(6, path);
  arc->images = 0;
  rc = fseek(fp, 0, 0);
  live = arc->fp != nullptr;
  if ( rc )
  {
    if ( !live )
      __exit_402E40(3, path);
    return arc;
  }
  if ( !live )
    {
      __exit_402E40(3, path);
      return arc;
    }
  if ( !feof(arc->fp) )
  {
    __expand_image((uint8_t *)arc, 1, (void **)nullptr);
    fp2 = arc->fp;
    if ( !fp2 )
      {
        __exit_402E40(3, path);
        return arc;
      }
    if ( !feof(arc->fp) )
    {
      arc->images = 0;
      fseek(fp2, 0, 0);
      live = arc->fp != nullptr;
      {
        if ( !live )
        __exit_402E40(3, path);
        return arc;
      }
    }
  }
  return arc;
}


int32_t __compress_image(uint8_t *arc_in, BmfImage *p_i, void *coded_buf)
{
  // The union below is MSVC's slot sharing written down, and lifting
  // its arms to separate locals is not the same program, so the frame
  // stays.  Everything outside it has been lifted where the gate
  // allowed.  The note here used to read "the two-member archive fails to decompress"
  // on the authority of `frame-sweep.sh`, which lifts aliases and has
  // had none to lift since round nine.
  struct alignas(16) CompressImageFrame {   // 80 bytes, one stack frame
      uint8_t _pad0[12];
      union {
          // The 16-byte archive member header `fwrite` sends in one call, and
          // the scratch MSVC put in the same bytes afterwards.  The two live in
          // mutually exclusive branches: everything below `if ( fits )` returns,
          // so the header is finished with before the deinterleave starts.
          BmfImage hdr;
          struct {
            uint8_t  _scratch0[4];
            int32_t  plane_n;    // +4   `plane_count`, the deinterleave stride
            uint8_t *row;        // +8   the source cursor, then the row index
            int32_t  row_step;   // +12  `plane_count * (height - 1)`
          };
      };
      uint8_t _pad1[32];
  } __frame;
  uint32_t filtered;
  int32_t y0;
  uint8_t *arc_f;
  void *pixels;
  // These shared `filtered` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t coded_bytes;
  // These shared `__frame.hdr[0]` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint8_t *pix_copy;
  ;
  uint32_t coded_len;   // word 1 of the coded block, its length
  uint8_t *arc;   // were int32_t: these hold addresses
  FILE *i;
  bool fits;
  uint8_t bpp;
  uint32_t acc, data_bytes, img_stride, row_bytes, shifted;
  uint8_t has_coded;   // 0/1, shifted into bit 7 of the header byte
  uint8_t *plane_buf, *row_at, *row_next;   // `uint8_t *` beside the `char` scalars above
  uint32_t hdr_pad8, hdr_pad8b;   // the header's pad/depth/flags word, not an address
  int32_t bits_left, pl, free_bits, pl2, ok_all, img_h,
          rows_left, y, pl_i, step, countdown, data_size;
  BmfImage *img;
  uint16_t w16;
  uint32_t pal_bytes, word_flags, word_dc, word_w4, word_w8, word_w12, n_pix, written;
  uint8_t ok, *dst, ok_raw;
  arc = arc_in;
  if ( !((BmfArc *)arc_in)->fp )
    return 0;
  if ( !feof(((BmfArc *)arc_in)->fp) )
  {
    __expand_image(arc, 1, (void **)nullptr);
    for ( i = ((BmfArc *)arc)->fp; i; i = ((BmfArc *)arc)->fp )
    {
      if ( feof(((BmfArc *)arc)->fp) )
        break;
      if ( feof(i) )
        break;
      __expand_image(arc, 1, (void **)nullptr);
    }
  }
  has_coded = (uint8_t)(uintptr_t)coded_buf;
  img = (BmfImage *)(p_i);
  row_bytes = p_i->stride;
  if ( coded_buf )
    has_coded = 1;
  __frame.hdr.width = p_i->width;
  __frame.hdr.height = p_i->height;
  p_i->flags |= has_coded << 7;
  hdr_pad8 = *(uint32_t *)&p_i->_pad8;
  __frame.hdr.stride = (uint32_t)row_bytes;
  plane_desc[0].w4 = 512;
  plane_desc[0].w12 = 0;
  *(uint32_t *)&__frame.hdr._pad8 = hdr_pad8;
  __frame.hdr.data_size = (uint32_t)p_i->data_size;
  ::plane_count = ((p_i->depth & 0x3Fu) + 7) >> 3;
  if ( fwrite("\x81\x8A""20\x81\x90""20a+b", 4u, 1u, ((BmfArc *)arc)->fp) != 1 )
    return 0;
  bpp = p_i->depth;
  ++*(uint32_t *)arc;
  pal_bytes = bpp & 0x80;
  if ( (bpp & 0x80) != 0 )   // bit 7 is the palette flag, not a sign
    pal_bytes = 3 << (bpp & 31);
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
  filtered = __search_filter((BmfImage *)p_i, 0);
  __frame.hdr.flags |= 0x10u;
  if ( (p_i->flags & 2) != 0 )
  {
    img_stride = p_i->stride;
    hdr_pad8b = *(uint32_t *)&p_i->_pad8;
    __frame.hdr.width = p_i->width;
    __frame.hdr.height = p_i->height;
    data_bytes = p_i->data_size;
    __frame.hdr.stride = (uint32_t)img_stride;
    *(uint32_t *)&__frame.hdr._pad8 = hdr_pad8b;
    __frame.hdr.data_size = (uint32_t)data_bytes;
    __frame.hdr.flags = 0x34 | (uint8_t)(hdr_pad8b >> 24);   // -S in bit 2
  }
  else
  {
    data_bytes = p_i->data_size;
  }
  coded_size = data_bytes + 0x20000;
  ::coded_buf = (uint8_t *)bmf_new(data_bytes + 0x20000);
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
    arc_f = arc;
    pl = 0;
    do
    {
      word_flags = (4 * plane_desc[pl + 1].flags) | plane_desc[pl + 1].predictor;
      if ( bits_left < 6 )
      {
        *(uint32_t *)::packer_word = ::packer_acc | (2 * (word_flags << ((31 - bits_left) & 31)));
        ::packer_word = (uint32_t *)out_cursor;
        out_cursor += 4;
        bits_left = ::packer_free_bits + 26;
        ::packer_acc = word_flags >> (::packer_free_bits & 31);
      }
      else
      {
        ::packer_acc |= word_flags << (-bits_left & 31);
        bits_left = ::packer_free_bits - 6;
      }
      ::packer_free_bits = bits_left;
      if ( (plane_desc[pl + 1].flags & 8) != 0 )
      {
        word_dc = plane_desc[pl + 1].b3;
        if ( bits_left < 8 )
        {
          *(uint32_t *)::packer_word = ::packer_acc | (2 * (word_dc << ((31 - bits_left) & 31)));
          ::packer_word = (uint32_t *)out_cursor;
          out_cursor += 4;
          bits_left = ::packer_free_bits + 24;
          ::packer_acc = word_dc >> (::packer_free_bits & 31);
        }
        else
        {
          shifted = word_dc << (-bits_left & 31);
          bits_left -= 8;
          ::packer_acc |= shifted;
        }
        ::packer_free_bits = bits_left;
        if ( plane_desc[pl + 1].predictor > 1u )
        {
          word_w4 = plane_desc[pl + 1].w4 + 64;
          if ( bits_left < 8 )
          {
            *(uint32_t *)::packer_word = ::packer_acc | (2 * (word_w4 << ((31 - bits_left) & 31)));
            ::packer_word = (uint32_t *)out_cursor;
            out_cursor += 4;
            free_bits = ::packer_free_bits + 24;
            ::packer_acc = word_w4 >> (::packer_free_bits & 31);
          }
          else
          {
            ::packer_acc |= word_w4 << (-bits_left & 31);
            free_bits = ::packer_free_bits - 8;
          }
          ::packer_free_bits = free_bits;
          word_w8 = plane_desc[pl + 1].w8 + 64;
          if ( free_bits < 8 )
          {
            *(uint32_t *)::packer_word = ::packer_acc | (2 * (word_w8 << ((31 - free_bits) & 31)));
            ::packer_word = (uint32_t *)out_cursor;
            out_cursor += 4;
            bits_left = ::packer_free_bits + 24;
            ::packer_acc = word_w8 >> (::packer_free_bits & 31);
          }
          else
          {
            bits_left = free_bits - 8;
            ::packer_acc |= (word_w8 << (-free_bits & 31));
          }
          ::packer_free_bits = bits_left;
          if ( plane_desc[pl + 1].predictor > 2u )
          {
            word_w12 = plane_desc[pl + 1].w12 + 64;
            if ( bits_left < 8 )
            {
              *(uint32_t *)::packer_word = ::packer_acc | (2 * (word_w12 << ((31 - bits_left) & 31)));
              ::packer_word = (uint32_t *)out_cursor;
              out_cursor += 4;
              acc = word_w12 >> (::packer_free_bits & 31);
              bits_left = ::packer_free_bits + 24;
              ::packer_free_bits += 24;
              ::packer_acc = acc;
            }
            else
            {
              ::packer_acc |= word_w12 << (-bits_left & 31);
              bits_left = ::packer_free_bits - 8;
              ::packer_free_bits -= 8;
            }
          }
        }
      }
      ++pl;
    }
    while ( pl < ::plane_count );
    arc = arc_f;
  }
  if ( filtered )
  {
    n_pix = p_i->width * p_i->height;
    __frame.hdr.flags |= 8u;
    plane_buf = (uint8_t *)bmf_new(n_pix);
    if ( ::plane_count > 0 )
    {
      arc_f = arc;
      pl2 = 0;
      do
        __model_planes((uint8_t *)p_i, plane_buf, plane_desc[pl2++ + 1].src_plane, 0);
      while ( pl2 < ::plane_count );
      arc = arc_f;
    }
    free(plane_buf);
  }
  else
  {
    __transform_planes((BmfImage *)p_i, (int32_t)p_i, 0);
  }
LABEL_57:
  *(uint32_t *)::packer_word = ::packer_acc;
  fits = (uint32_t)(out_cursor - (uint32_t)::coded_buf) < p_i->data_size;
  coded_bytes = out_cursor - ::coded_buf;
  __frame.hdr.data_size = (uint32_t)(out_cursor - ::coded_buf);
  if ( fits )
  {
    ok = fwrite(&__frame.hdr, 1u, 0x10u, ((BmfArc *)arc)->fp) == 16;
    if ( coded_buf )
    {
      // Word 1 of the coded block is its length; the eight is the header in
      // front of it.
      coded_len = ((const uint32_t *)coded_buf)[1];
      ok &= fwrite(coded_buf, 1u, coded_len + 8, ((BmfArc *)arc)->fp) == coded_len + 8;
    }
    ok_all = (fwrite(::coded_buf, 1u, coded_bytes, ((BmfArc *)arc)->fp) == (uint32_t)coded_bytes) & ok;
    free(::coded_buf);
    if ( ok_all && (p_i->depth & 0x80) != 0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, ((BmfArc *)arc)->fp);
    fflush(((BmfArc *)arc)->fp);
    if ( ok_all )
      return (int32_t)__frame.hdr.data_size;
    return ok_all;
  }
  free(::coded_buf);
  if ( (p_i->flags & 2) != 0 )
  {
    pix_copy = (uint8_t *)bmf_new(p_i->data_size);
    __frame.plane_n = ::plane_count;
    __frame.row = pix_copy;
    __frame.row_step = ::plane_count * (p_i->height - 1);
    pixels = (uint16_t *)p_i->pixels;
    memcpy(pix_copy,p_i->pixels,p_i->data_size);
    LOWORD(img_h) = p_i->height;
    if ( (uint16_t)img_h )
    {
      rows_left = __frame.plane_n;
      row_at = __frame.row;
      row_next = nullptr;
      y = 0;
      arc_f = arc;
      do
      {
        pl_i = img->width;
        y0 = y;
        __frame.row = row_next;
        dst = (uint8_t *)img + y + 16;
        step = __frame.row_step;
        do
        {
          countdown = rows_left;
          do
          {
            *dst++ = *row_at++;
            --countdown;
          }
          while ( countdown );
          dst += step;
          --pl_i;
        }
        while ( pl_i );
        img = (BmfImage *)(p_i);
        img_h = p_i->height;
        y = rows_left + y0;
        row_next = __frame.row + 1;
      }
      while ( (int32_t)(__frame.row + 1) < img_h );
      arc = arc_f;
    }
    w16 = img->width;
    img->width = img_h;
    img->height = w16;
    img->flags ^= 2u;
    img->stride = (img_h * __frame.plane_n);
    free(pix_copy);
    goto LABEL_77;
  }
LABEL_76:
  pixels = (uint16_t *)p_i->pixels;
LABEL_77:
  ok_raw = fwrite(img, 1u, 0x10u, ((BmfArc *)arc)->fp) == 16;
  if ( coded_buf )
  {
    coded_len = ((const uint32_t *)coded_buf)[1];
    ok_raw &= fwrite(coded_buf, 1u, coded_len + 8, ((BmfArc *)arc)->fp) == coded_len + 8;
  }
  written = fwrite(pixels, 1u, pal_bytes + img->data_size, ((BmfArc *)arc)->fp);
  data_size = img->data_size;
  if ( (ok_raw & (written == data_size + pal_bytes)) == 0 )
    return 0;
  return data_size;
}

// ---------------------------------------------------------------------------
// The two things this program does.

// BMF's own driver, sub_4015C0, sniffed the first four bytes of its argument
// to choose between six readers, derived the output name from the input's by
// swapping the extension, and picked the writer from a switch.  The command
// line is now

//     bmf c input.bmp output      compress a BMP into a BMF stream
//     bmf d input output.bmp      expand a BMF stream back into a BMP

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
  FILE *fp;
  const uint8_t *Palette;
  int32_t *p_i, Arc, Flags, Colours, Step, Grey, i;
  int32_t coded_len;

  // The reader below answers "no" the same way whether the file is missing or
  // is not a BMP, and BMF told those apart -- so open it once first, as its
  // format sniffer used to, and keep the two messages.
  fp = fopen(InName, "rb");
  if ( !fp )
    __exit_402E40(6, InName);
  fclose(fp);
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

  coded_len = __compress_image((uint8_t *)Arc, (BmfImage *)p_i, (void *)coded_block);
  if ( !coded_len )
    __exit_402E40(5, OutName);
  printf(
    "%6.3f bpp\n",
    (double)coded_len * 8.0 / (double)(p_i_img->height * p_i_img->width));
  free(p_i);
}

// Expand the BMF stream InName into a BMP named OutName.
void __bmf_decompress(
                              const char *InName, const char *OutName)
{
  ;
  int32_t Number, Depth;
  uint32_t *p_i;
  void *arc;

  if ( void *__nb = bmf_new(sizeof(BmfArc)) )
    arc = (void *)__bmf_open_archive((BmfArc *)__nb, (char *)InName, 1);
  else
    arc = nullptr;
  printf("File %16s,\r", InName);
  Number = 0;
  while ( 1 )
  {
    p_i = (uint32_t *)__expand_image((uint8_t *)arc, 0, &coded_block);
    BmfImage *const p_i_img = (BmfImage *)p_i;
    if ( !p_i )
    {
      printf("\n");
      if ( !((BmfArc *)arc)->fp )
        __exit_402E40(3, InName);
      __bmf_destroy_archive((BmfArc *)arc, 1);
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
  int32_t mode;

  bmf_set_denormal_mode();
  __set_new_handler(__out_of_memory_handler);
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");

  mode = argc == 4 && !argv[1][1] ? toupper(argv[1][0]) : 0;
  if ( mode != 'C' && mode != 'D' )
  {
    printf(
      "e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
      "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
      "       bmf d input output.bmp     expand\n");
    return 1;
  }

  if ( mode == 'C' )
    __bmf_compress( argv[2], argv[3]);
  else
    __bmf_decompress( argv[2], argv[3]);
  return 0;
}
void __out_of_memory_handler() { __exit_402E40(7); }
// `char **`, which is what a hosted `main` takes.  The decompilation had
// `uint8_t **` because that is the type Hex-Rays gave the argv walk below,
// and the cast into `__main` was already doing the conversion.
int32_t main(int32_t argc, char **argv) {
  return __main(argc, (const char **)argv);
}
