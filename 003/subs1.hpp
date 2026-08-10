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
// (`(uint8_t *)model_geometry + v3 + 8`) rather than given a global of its own;
// the exception is 0x0044337D, byte 1 of the dword array based at coded_buf,
// which is indexed like an array and so got a byte global of its own.
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
// `b1` for a counter that has just reached count `b0`, read a byte at a time
// at `b0 + 3`, so only bytes 4..11 are ever touched.  It is `int32_t[17]`
// because that is the width `deblob.py` measured, not the width of a read.
alignas(16) static int32_t p2_b1_seed[17] = {   // 0x439B7C
  1126773555, 1065353216, -986839294, 981668463, 953267991, 1036831949, 1073741824,
  1141899264, 1076258406, 1188175872, 1012202996, 1167951872, 1141129216, 961656599,
  1092616192, 1065353216, 1065353216,
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
static int32_t model_geometry[32];   // was 0x445660 in bmf_bss
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
// A five-entry table, and the extent is measured rather than assumed: the one
// site that subscripts it -- `*(uint16_t *)f56[5] = mode_symbol[n4]`, with `n4`
// out of `ModelBlock::f32` -- steps four bytes, and the four globals after
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

// The two runs of five are arrays, and the code says so rather than the
// offsets: alt_model_p1_decode rotates `row` by one place every pass --
//
//   v25 = row[4]; v26 = row[3]; v27 = row[2]; v28 = row[1]; v29 = row[0];
//   row[4] = v26; row[3] = v27; row[2] = v28; row[1] = v29; row[0] = v25;
//
// a five-deep ring of row pointers -- and then derives `cur` from it, three of
// the five offset by eight.  Naming ten consecutive `uint32_t` f176 through
// f212 hid the one fact about them worth having.  The layout is unchanged:
// `row[0]` is still +176, and the static_assert says so.
//
// AltP1Block -- recovered from 260 dereferences over 54 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct AltP1Block {
  union {   // the same bytes, 4 recoveries
    struct {
      int32_t f0[8];   // +0 .. +28
      uint8_t _u0_0_1[144];
      uint8_t *f176[10];   // +176 .. +212, ten row cursors
    };
    struct {
      uint8_t _u0_1_0[4];
      int32_t f4;
      int32_t f8;
      uint32_t f12[51];   // +12 .. +212
    };
    struct {
      uint8_t _u0_2_0[196];
      uint8_t *cur[5];   // +196 .. +212, derived from row
    };
    struct {
      uint8_t _u0_3_0[200];
      uint8_t *f200;
      uint8_t *f204;
      uint8_t *f208;
      uint8_t *f212;
    };
  };
  // `alt_p1_alloc` seeds five tables between +216 and +3800 and they tile the
  // range exactly, which is what says where each one ends: 216 + 512 = 728,
  // + 256 = 984, + 256 = 1240, + 256 = 1496, + 256 = 1752, + 2048 = 3800.
  // The first is a level map indexed by a byte, the second a slot map indexed
  // by seven bits, and the fourth `int32_t` pair holds a level and a group.
  uint8_t f216[512];     // +216  .. +727
  uint8_t f728[256];     // +728  .. +983
  uint8_t f984[256];     // +984  .. +1239
  uint8_t f1240[256];    // +1240 .. +1495
  uint8_t f1496[256];    // +1496 .. +1751
  int32_t f1752[512];    // +1752 .. +3799
  // The counter table starts here, and the code reaches it as
  // `((P1Count *)_this)[k + 237]` -- a record grid anchored at the object's
  // base, so record 237's own fields begin at +3800 and its first eight bytes
  // are the tail of `f1752`.  Naming it would have to re-base all 208 of those
  // subscripts, so the extent is pinned instead.
  uint8_t _pad3800[10077696];   // +3800 .. +10081495
};
static_assert(sizeof(void *) != 4 || sizeof(AltP1Block) == 0x99D4D8,
              "AltP1Block is not what alt_p1_alloc's callers allocate");
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(AltP1Block, f212) == 212,
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
  uint32_t  f8;       // +8
  uint32_t  f12;      // +12  12 * n at init; `symbol_list_update`'s `a3` adds here
  uint32_t  f16;      // +16  8 * n at init
  SymEntry *ent;      // +20
};
static_assert(sizeof(SymList) == 24, "SymList: the header is 24 bytes");

struct ModelBlock {
  uint32_t f0;
  int32_t f4;
  uint32_t f8;
  uint32_t f12;
  uint32_t f16;
  uint32_t f20;
  uint32_t f24;
  uint32_t f28;
  uint32_t f32;
  uint32_t f36;
  uint32_t f40;
  uint32_t sym_pos;   // 0..31; the index pixel_context reads sym[] with
  uint32_t f48;
  uint32_t f52;
  uint8_t  *f56[14];   // +56 .. +108, the row cursors: every element is an address
  uint8_t _pad15[1051552];
  uint8_t  *f1051664[4];   // +1051664 .. +1051676, four more row cursors
  // What `layout_workspace` seeds, and the boundaries are its own: sixteen
  // three-word records at +1 051 680, a 24 KiB block it `memset`s, one record
  // and 1536 bytes it `memset`s at +1 076 352, and 48 more records at
  // +1 077 894.  Each record is (40, 16, 512) or (4, 4, 72), which is two
  // counts and a total -- the unit `encode_context_bit` takes a pointer to,
  // which is why every reader of these four indexes them `3 * k`.
  uint16_t f1051680[48];      // +1051680 .. +1051775, 16 records
  uint16_t f1051776[12288];   // +1051776 .. +1076351, 4096 records
  uint16_t f1076352[771];     // +1076352 .. +1077893, 257 records
  uint16_t f1077894[144];     // +1077894 .. +1078181, 48 records
  uint8_t _pad16[2];   // +1078182 .. +1078183
  // A twenty-fourth symbol list, inside the object rather than in the array
  // beside it: `init_model_tables` initialises `_this + 1078184` and
  // `f1078224` holds its address, so the `void *` that used to sit at +1078204
  // was this list's `ent` -- which is why `free_workspace` frees it on its own.
  SymList escape;      // +1078184 .. +1078207
  SymList *f1078208;   // the symbol lists, 24 bytes each
  SymList *f1078212;   // the symbol lists, 24 bytes each
  // The lists the current context selects -- `sel[0]` out of `f1078212` and
  // `sel[1]` out of `f1078208`.  Two adjacent members of one type that
  // `f1078232` walks as an array, which is why the reset is spelled
  // `_this + 1078216` and the end test compares against the same address.
  SymList *sel[2];       // +1078216, +1078220
  SymList *f1078224;   // always `&escape`
  uint8_t _pad22[4];
  SymList **f1078232;    // the cursor over `sel`
  uint8_t *f1078236;   // freed by free_workspace: a buffer, not an int
  uint8_t *f1078240;   // a row cursor
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
  uint8_t *f1078684;
  uint8_t *f1078688;   // the alphabet map, one byte a symbol
  uint8_t f1078692[4];   // +1078692 .. +1078695
  // `layout_workspace` seeds this one 0x40000 times, two counters an
  // iteration, both 0x2000.
  uint16_t f1078696[524288];   // +1078696 .. +2127271
  uint8_t _pad28[3932160];
  uint32_t f6059432;
  uint8_t *f6059436;   // a row cursor; `f6059436 + 2` steps two bytes
  // The four regions `layout_workspace` ends on.  Each extent is the loop
  // bound or the memset length that fills it, and the four of them run to
  // 8102448 -- which is the 0x7BA230 both callers ask `bmf_new` for, so the
  // last one ends exactly at the end of the object.
  uint16_t f6059440[8192];   // +6059440 .. +6075823
  // All three are read only through `(uint16_t *)this + k`, and `memset`
  // does not care: the byte counts are the same and the element type is now
  // the one the code uses.
  uint16_t f6075824[192512];   // +6075824 .. +6460847
  uint16_t f6460848[108800];   // +6460848 .. +6678447
  uint16_t f6678448[712000];   // +6678448 .. +8102447
};
static_assert(sizeof(void *) != 4
              || (__builtin_offsetof(ModelBlock, f6059436) == 6059436
                  && __builtin_offsetof(ModelBlock, f1078696) == 1078696
                  && __builtin_offsetof(ModelBlock, f6059440) == 6059440
                  && __builtin_offsetof(ModelBlock, f6075824) == 6075824
                  && __builtin_offsetof(ModelBlock, f6460848) == 6460848
                  && __builtin_offsetof(ModelBlock, f6678448) == 6678448
                  && sizeof(ModelBlock) == 0x7BA230),
              "ModelBlock: the layout moved");


// One p2 counter.  `alt_p2_alloc` resets every one of them to b0 = 5, b1 = 2,
// w2 = 0; `alt_p2_model` raises b0 by one per update while it is under 8, sets
// b1 from `p2_b1_seed` by the new b0, and rescales w2.  The five readers
// all say the same thing:
//
//   ((1 << ((b0 + 31) & 31)) + w2) >> (b0 & 31)
//
// so b0 is a shift and w2 is what it scales.  Nothing in this build reads b1.
// The signedness is each site's: b0 and w2 are read signed, b1 is written
// unsigned and never read.
struct P2Count {
  int8_t b0;
  uint8_t b1;
  int16_t w2;
};
static_assert(sizeof(P2Count) == 4, "P2Count: the record is four bytes");

// The p2 model's neighbourhood table: eighteen bytes a record, rows 144 bytes
// apart -- eight records to a row -- which `algorithm_v2.md` §9 established
// from the 56 places that copy one.  Every reader takes it as `int16_t`, so
// nine lanes is what a record is here; what the lanes hold is not established,
// which is why they are numbered rather than named.  `alt_p2_context` reaches
// records -2 .. +4 of the cursor it is given.
struct P2Ctx {
  int16_t lane[9];
};
static_assert(sizeof(P2Ctx) == 18, "P2Ctx: the record is eighteen bytes");

// AltP2Block -- recovered from 353 dereferences over 39 offsets, under 23
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct AltP2Block {
  uint8_t _pad0[278528];
  // 336 bytes, 278528..278863, and four readings of them.  The extent is the
  // distance to `f278904`, not a measurement of any one reading: Hex-Rays had
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
      uint32_t f278536;
      uint16_t f278540;
      uint8_t f278542;
      uint8_t _u0_0_4[97];
      uint64_t f278640;
      uint64_t f278648;
      float (*f278656)[4];   // a weight block: 16 rows of four, `alt_p2_filter`'s `_this`
      int32_t *f278660;
      uint8_t *f278664;   // a row cursor
      uint8_t *f278668;   // a row cursor
      uint8_t *f278672;   // a row cursor
      int32_t f278676;
      int32_t f278680;
      int32_t f278684;
      int32_t f278688;
      int32_t f278692;
      int32_t f278696;
      uint16_t f278700;
      uint8_t _u0_0_12[2];
      uint32_t f278704;
      uint32_t f278708;
      uint32_t f278712;
      int32_t f278716;
      int32_t f278720;
      int32_t f278724;
      uint32_t f278728;
      int32_t f278732;
      uint8_t *f278736[10];   // +278736 .. +278772, row cursors
    };
    struct {
      uint8_t _u0_1_0[228];
      uint8_t *f278756;
      uint32_t f278760[24];   // +278760 .. +278855
      uint8_t _u0_1_3[8];   // +278856 .. +278863, to the end of the 336
    };
    struct {
      uint8_t _u0_2_0[232];
      uint8_t *f278760_p;
      uint8_t *f278764;
      uint8_t *f278768;
      uint8_t *f278772;   // a row cursor, like its three neighbours
    };
  };
  uint8_t _pad1[40];
  // Ten scalars `alt_p2_context` writes as it folds its neighbourhood sums.
  int32_t f278904[10];   // +278904 .. +278943
  // One table, reached from two bases four elements apart -- Hex-Rays' 16-byte
  // view called them element 3 and element 4 -- which is what a `t[n]`/
  // `t[n + 4]` pair looks like when the compiler keeps both addresses in
  // registers.  `n` is capped at 255 one line above each read, and 255 + 4 is
  // the last element.
  int32_t f278944[260];   // +278944 .. +279983
  // Three 256-entry maps over a byte value, and `f280496` is indexed by what
  // `f279984` returns.
  uint8_t f279984[256];   // +279984 .. +280239
  uint8_t f280240[256];   // +280240 .. +280495
  int8_t f280496[256];   // +280496 .. +280751
  // 120 = 960 / 8, and the reader caps at 960 before shifting: the guard is
  // the extent.  Hex-Rays' 16-byte view of this ran into the next table, whose
  // real base is +280872 -- which is why every read of that one said `+ 4`.
  uint8_t f280752[120];   // +280752 .. +280871
  int16_t f280872[1916];   // +280872 .. +284703
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
  P2Count f284712[163840];   // +284712 .. +940071
  // The three-way frequency counters, in 8-byte groups.  `alt_p2_alloc` seeds
  // 0x1E60 sixteen-byte pairs of them; `alt_p2_model` rescales a group when
  // its three add past 29696.  The extent is the allocation's.
  uint16_t f940072[62208];   // +940072 .. +1064487
  uint8_t _pad3[8];   // +1064488 .. +1064495
};
static_assert(sizeof(void *) != 4 || sizeof(AltP2Block) == 0x103E30,
              "AltP2Block: bmf_page_alloc asks for 0x103E30 and this is it");
static_assert(sizeof(void *) != 4
              || (__builtin_offsetof(AltP2Block, f278904) == 278904
                  && __builtin_offsetof(AltP2Block, f278944) == 278944
                  && __builtin_offsetof(AltP2Block, f279984) == 279984
                  && __builtin_offsetof(AltP2Block, f280240) == 280240
                  && __builtin_offsetof(AltP2Block, f280496) == 280496
                  && __builtin_offsetof(AltP2Block, f280752) == 280752
                  && __builtin_offsetof(AltP2Block, f280872) == 280872
                  && __builtin_offsetof(AltP2Block, f284712) == 284712
                  && __builtin_offsetof(AltP2Block, f940072) == 940072),
              "AltP2Block: the layout moved");


// P2Weights -- recovered from 49 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct P2Weights {
  // Six pointers to the same shape, one per sub-model, which is what
  // `alt_p2_filter` treats them as: it walks all six with one weight each.
  // Each points at rows of four floats -- the function reads nothing else
  // through them -- so that is what they are.
  float (*f0[6])[4];   // +0 .. +20
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(P2Weights, f0[5]) == 20,
              "P2Weights: the layout moved");


// The pixel model's per-pixel record.  Eight bytes, which is what every cursor
// in `ModelBlock::f56` steps: `f56[5] += 8`, `f56[8] += 8`, `f56[6] += 8 * n`,
// and `f56[7] = v49 + 56` is seven of them.
//
// `code_pixel` writes the symbol at +0 and then six comparisons of it against
// six neighbours' symbols at +2..+7 -- the match state `ALGORITHM.md` §8.2
// describes -- and its five-tap predictor sums `match[0]` of records -3, -2,
// +2, +3 and +4.  What the other five flags feed is not established.
struct PixRec {
  uint16_t sym;        // +0
  uint8_t  match[6];   // +2 .. +7
};
static_assert(sizeof(PixRec) == 8, "PixRec: the record is eight bytes");


// The p1 model's counter table: 16-byte records, and `alt_p1_model` reaches
// three of them at a time -- 236, 237 and 238 past the record its caller
// picked -- bumping a total and one of three bins in each.  The first four
// lanes have no reader in this file.
struct P1Count {
  uint16_t w[4];      // +0 .. +7
  uint16_t total;     // +8
  uint16_t bin[3];    // +10 .. +15
};
static_assert(sizeof(P1Count) == 16, "P1Count: the record is sixteen bytes");


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

uint32_t __predict_med(uint8_t *Src, int32_t i, int32_t a3)
{
  ;
  uint8_t v26;
  int32_t i_1, v16, v17, v18, v19, v20, v21, v22, v24, v27, v29;
  uint32_t j, n15, v23, v25, n15_1;
  uint8_t *v28;
  uint8_t *v3, *v4;
  alignas(16) uint8_t v31[272];
  v3 = (Src + a3 * i);
  v4 = &v3[-i];
  // The folding table, value -> code: a non-negative residual takes the even
  // codes counting up from 0, a negative one the odd codes counting down from
  // -1.  `unpredict_med` builds the inverse of this.  Wrapping to eight bits
  // is what the byte stores did.
  for ( j = 0; j < 128; ++j )
  {
    v31[j]       = (uint8_t)(2 * j);
    v31[128 + j] = (uint8_t)(-1 - 2 * (int32_t)j);
  }
  i_1 = i;
  v16 = a3 - 1;
  if ( a3 == 1 )
    goto LABEL_24;
  do
  {
    v17 = i - 1;
    if ( i_1 == 1 )
      goto LABEL_23;
    v29 = v16;
    do
    {
      v18 = (uint8_t)*--v4;
      v19 = (uint8_t)*(--v3 - 1);
      v20 = (uint8_t)v3[-i - 1];
      if ( v19 < v18 )
      {
        if ( v20 < v19 )
        {
          LOBYTE(v19) = *v4;
          goto LABEL_21;
        }
        if ( v20 <= v18 )
LABEL_20:
          LOBYTE(v19) = v18 + v19 - v20;
      }
      else
      {
        if ( v20 > v19 )
        {
          LOBYTE(v19) = *v4;
          goto LABEL_21;
        }
        if ( v20 >= v18 )
          goto LABEL_20;
      }
LABEL_21:
      v21 = (uint8_t)v31[(uint8_t)(*v3 - v19)];
      *v3 = v21;
      ++*(uint32_t *)&hist_scratch[4 * v21];
      --v17;
    }
    while ( v17 );
    v16 = v29;
    i_1 = i;
LABEL_23:
    --v4;
    --v3;
    n15 = (uint8_t)v31[(uint8_t)(*v3 - v3[-i_1])];
    *v3 = n15;
    ++*(uint32_t *)&hist_scratch[4 * n15];
    --v16;
  }
  while ( v16 );
LABEL_24:
  if ( i_1 != 1 )
  {
    n15 = i_1 - 1;
    v22 = (i_1 - 1) / 2;
    if ( v22 )
    {
      n15_1 = i_1 - 1;
      v23 = 0;
      v24 = 0;
      v25 = v22;
      do
      {
        v26 = v3[v24 - 2];
        ++v23;
        v3[v24 - 1] = v31[(uint8_t)(v3[v24 - 1] - v26)];
        v3[v24 - 2] = v31[(uint8_t)(v26 - v3[v24 - 3])];
        v24 -= 2;
      }
      while ( v23 < v25 );
      n15 = n15_1;
      v27 = 2 * v23 + 1;
    }
    else
    {
      v27 = 1;
    }
    if ( n15 > v27 - 1 )
    {
      v28 = (&v3[-v27]);
      n15 = (uint8_t)(*v28 - *((int8_t *)v28 - 1));
      *v28 = v31[(uint8_t)n15];
    }
  }
  return n15;
}

uint32_t __alt_init_tables(uint8_t *a1, int8_t *a2)
{
  ;
  uint8_t v48, v53;
  uint8_t *v18, *v19;   // `uint8_t *` beside the `char` scalars above
  int32_t n128_1, n128_6, v30, n128_5, v33, n128_3, n128_4, v36, n128_11,
          n128_2;
  uint32_t j, k, i, v32, n0x80, v49, v52;
  uint8_t *v50, *v51;
  n128_1 = 2 * plane_desc[0].w12 + 1;
  // The predictor-mode-0 branch was here: 111 lines building a 256-entry
  // identity table with SSE.  Nothing can reach it.  This function is called
  // only by alt_p2_alloc and alt_p1_alloc; those are called only by the eight
  // alt_model_p{1,2}[_d8]_{encode,decode} bodies; and model_plane and
  // unmodel_plane dispatch to those bodies only under plane_predictor == 1
  // or == 2.  The call graph is closed and has no path with the predictor at
  // 0, which a run over the corpus agrees with: 164 entries, 83 at 1 and 81
  // at 2.  Deleted on the same grounds as the fast path (REFACTORING.md
  // section 2.1) -- code no dispatch reaches is not a feature to keep.
  *a2 = 0;
  v18 = (uint8_t *)a2 + 2;
  v19 = (uint8_t *)a2 + 1;
  a2[255] = 0x80;
  if ( ((uint8_t *)a2 + 1 <= (uint8_t *)a2 + 2 || (uint32_t)(v19 - v18) < 0xFE) && (v18 <= v19 || (uint32_t)(v18 - v19) < 0xFE) )
  {
    for ( i = 0; i < 0x3F; ++i )
    {
      ((uint8_t *)a2)[4 * i + 2] = 2 * i + 1;
      ((uint8_t *)a2)[4 * i + 1] = -2 * i - 1;
      ((uint8_t *)a2)[4 * i + 4] = 2 * i + 2;
      ((uint8_t *)a2)[4 * i + 3] = -2 * i - 2;
    }
    a2[254] = 127;
    a2[253] = -127;
  }
  else
  {
    for ( j = 0; j < 0x3F; ++j )
    {
      ((uint8_t *)a2)[4 * j + 2] = 2 * j + 1;
      ((uint8_t *)a2)[4 * j + 4] = 2 * j + 2;
    }
    a2[254] = 127;
    for ( k = 0; k < 0x3F; ++k )
    {
      ((uint8_t *)a2)[4 * k + 1] = -2 * k - 1;
      ((uint8_t *)a2)[4 * k + 3] = -2 * k - 2;
    }
    a2[253] = -127;
  }
  // never taken: -E is 0
  // The test here was `if ( plane_predictor )`, with an else for predictor
  // mode 0.  This function is called only by alt_p1_alloc and alt_p2_alloc,
  // and those only by the eight alt_model bodies, which the dispatch reaches
  // only under the predictor being 1 or 2.  Always true, so it and its
  // 22-line else are gone.  Same argument as the block above it.
  *a1 = 0;
  n128_6 = 1;
  a1[128] = -1;
  // -E is 0, so the near-lossless fill that stood here never ran: the jump
  // over it was `if ( 1 ) goto LABEL_52`, and LABEL_52 had no other source.
  // The `n128_6 < 128` test it jumped past went with it -- the only live
  // path entered the block without evaluating it.
  {
    v49 = 128 - n128_6;
    v52 = (128 - n128_6) / 2;
    v30 = 0;
    n128_5 = 1;
    if ( v52 )
    {
      n128_11 = n128_6;
      v32 = 0;
      n128_2 = n128_1;
      v33 = 0;
      v50 = &((uint8_t *)a1)[n128_6];
      v51 = &((uint8_t *)a1)[-n128_6];
      do
      {
        n128_3 = n128_2;
        n128_4 = n128_5 - 1;
        if ( n128_4 )
          n128_3 = n128_4;
        else
          ++v30;
        v48 = 2 * v30;
        v50[2 * v32] = 2 * v30;
        v53 = 2 * v30 - 1;
        n128_5 = n128_3 - 1;
        v51[v33 + 256] = v53;
        if ( n128_3 == 1 )
        {
          n128_5 = n128_2;
          v48 = 2 * ++v30;
          v53 = 2 * v30 - 1;
        }
        v50[2 * v32++ + 1] = v48;
        v51[v33 + 255] = v53;
        v33 -= 2;
      }
      while ( v32 < v52 );
      n128_6 = n128_11;
      v36 = 2 * v32 + 1;
    }
    else
    {
      v36 = 1;
    }
    if ( v36 - 1 < v49 )
    {
      if ( n128_5 == 1 )
        LOBYTE(v30) = v30 + 1;
      ((uint8_t *)a1)[n128_6 - 1 + v36] = 2 * v30;
      ((uint8_t *)a1)[-n128_6 - v36 + 257] = 2 * v30 - 1;
    }
  }
  for ( n0x80 = 0; n0x80 < 0x80; ++n0x80 )
  {
    ((uint8_t *)a1)[(uint8_t)((uint8_t *)a2)[2 * n0x80] + 256] = 2 * n0x80;
    ((uint8_t *)a1)[(uint8_t)((uint8_t *)a2)[2 * n0x80 + 1] + 256] = 2 * n0x80 + 1;
  }
  return n0x80;
}

uint32_t __rc_decode_flat(uint32_t tot)
{
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym + 1, tot);
  return sym;
}

int32_t __encode_context_bit(uint16_t *_this, uint16_t *a2, int32_t n15)
{
  ;
  int32_t v3, v4, n0x4000, result, v18, v19, n0x4000_1, v31, v33;
  uint32_t tot, n0x88_1, v30, v32, v34;
  v3 = *_this;
  if ( *_this )
  {
    v4 = *(_this + 1);
    if ( !*(_this + 1) )
    {
      v31 = *a2;
      v32 = v31 + a2[1];
      *_this = (v32 + (v31 << 6) - 64) / v32;
      *(_this + 1) = ((a2[1] << 6) + v32 - 64) / v32;
      *(_this + v3 - 1) += 4;
      *(_this + 2) = 512;
      v33 = a2[v3 - 1];
      a2[v3 - 1] = -3 * ((uint32_t)(3 - v33) >> 31) + v33;
      v3 = *_this;
      v4 = *(_this + 1);
    }
    tot = v3 + v4;
    rc.encode_bit(v3, v4, n15);
    n0x4000 = *(_this + 2);
    if ( tot > n0x4000 )
    {
      v30 = *(_this + 1);
      *_this -= *_this >> 1;
      *(_this + 1) = v30 - (v30 >> 1);
      if ( n0x4000 < 0x4000 )
        *(_this + 2) = n0x4000 + 64;
    }
    result = *(_this + n15) + 8;
    *(_this + n15) = result;
    a2[n15] += (uint32_t)tot < 0x88;
    return result;
  }
  v18 = *a2;
  v19 = a2[1];
  n0x88_1 = v18 + v19;
  rc.encode_bit(v18, v19, n15);
  n0x4000_1 = a2[2];
  if ( n0x88_1 > n0x4000_1 )
  {
    v34 = a2[1];
    *a2 -= *a2 >> 1;
    a2[1] = v34 - (v34 >> 1);
    if ( n0x4000_1 < 0x4000 )
      a2[2] = n0x4000_1 + 64;
  }
  result = a2[n15] + 8;
  a2[n15] = result;
  *_this = n15 + 1;
  return result;
}

int32_t __decode_context_bit(uint16_t *_this, uint16_t *a2)
{
  ;
  int32_t v2, v3, result, n0x4000, v13, v14, n0x4000_1, v24, v26;
  uint32_t tot, n0x88_1, v23, v25, v27;
  v2 = *_this;
  if ( *_this )
  {
    v3 = *(_this + 1);
    if ( !*(_this + 1) )
    {
      v24 = *a2;
      v25 = v24 + a2[1];
      *_this = (v25 + (v24 << 6) - 64) / v25;
      *(_this + 1) = ((a2[1] << 6) + v25 - 64) / v25;
      *(_this + v2 - 1) += 4;
      *(_this + 2) = 512;
      v26 = a2[v2 - 1];
      a2[v2 - 1] = -3 * ((uint32_t)(3 - v26) >> 31) + v26;
      v2 = *_this;
      v3 = *(_this + 1);
    }
    tot = v2 + v3;
    result = rc.decode_bit(v2, v3);
    n0x4000 = *(_this + 2);
    if ( tot > n0x4000 )
    {
      v23 = *(_this + 1);
      *_this -= *_this >> 1;
      *(_this + 1) = v23 - (v23 >> 1);
      if ( n0x4000 < 0x4000 )
        *(_this + 2) = n0x4000 + 64;
    }
    *(_this + result) += 8;
    a2[result] += (uint32_t)tot < 0x88;
  }
  else
  {
    v13 = *a2;
    v14 = a2[1];
    n0x88_1 = v13 + v14;
    result = rc.decode_bit(v13, v14);
    n0x4000_1 = a2[2];
    if ( n0x88_1 > n0x4000_1 )
    {
      v27 = a2[1];
      *a2 -= *a2 >> 1;
      a2[1] = v27 - (v27 >> 1);
      if ( n0x4000_1 < 0x4000 )
        a2[2] = n0x4000_1 + 64;
    }
    a2[result] += 8;
    *_this = result + 1;
  }
  return result;
}

int32_t __encode_symbol_list(SymList *_this, int32_t a2)
{
  ;
  int8_t v2;
  uint8_t v27, v29, v32, v33;
  // Every one of these walked `_this[5]`'s entries three bytes at a time,
  // reading the symbol as `*(uint16_t *)p` and the count as `p[2]`.
  SymEntry *v4, *v25, *v28, *v36, *v37, *v39, *v40, *v49;
  uint16_t v51;
  int32_t enc_cum, enc_high, enc_tot, v3, v5, v6, v7, v8, v24, n251, v35, v38,
          v41, v43, v47, v53, v54;
  uint16_t v26, v31;
  uint32_t i_1, i, v34, v42, v44;
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
      enc_tot = _this->f8 + v5;
      enc_high = enc_tot;
      do
      {
        exclusion_mask[v4->sym] = v2;
        --v4;
      }
      while ( v4 >= _this->ent );
      v8 = 0;
      goto LABEL_9;
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
  enc_tot = _this->f8 + v5;
  v4->cnt += 4;
  v25 = _this->ent;
  _this->f12 += 4;
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
    v29 = v28->cnt;
    v4->sym = v28->sym;
    v4->cnt = v29;
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
        v33 = v4->cnt;
        v28->sym = v4->sym;
        v28->cnt = v33;
        v4->sym = v31;
        v4->cnt = v32;
        v25 = _this->ent;
        --v28;
        if ( v4 == v25 )
          goto LABEL_37;
      }
    }
  }
  v34 = _this->f16;
  if ( n251 > 251 || v34 < _this->f12 )
  {
    v35 = _this->live;
    v54 = v34 < 20 * _this->n;
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
    v42 = _this->f8;
    if ( !v36->cnt )
    {
      do
      {
        ++v35;
        _this->f8 = ++v42;
        v43 = v37->cnt;
        --v37;
      }
      while ( !v43 );
      _this->live -= v35;
    }
    v44 = _this->f12;
    _this->f8 = v42 - (v42 >> 1);
    _this->f12 = v44 - (v44 >> 1);
    v8 = 1;
  }
  else
  {
    v8 = 1;
  }
LABEL_9:
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
  uint8_t v11, v12, v15, v17, v19, v20;
  // All of these walk the entries; the -3 and +2 they carried were the record
  // stride and the count field.
  SymEntry *v16, *v23, *v24, *v26, *v27, *v33, *n251_1, *n251_2;
  uint16_t v10, v14, v18, v34;
  int32_t v8, v9, v25, v28, v30, v32, v35, v36;
  uint32_t v22;   // a count, like `live` which it is subtracted from
  uint32_t v4, v6, v21, v29, v31;
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
    _this->f12 += a3;
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
      v17 = v16->cnt;
      n251_1->sym = v16->sym;
      n251_1->cnt = v17;
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
          v20 = n251_1->cnt;
          v16->sym = n251_1->sym;
          v16->cnt = v20;
          n251_1->sym = v18;
          n251_1->cnt = v19;
          n251_2 = _this->ent;
          --v16;
          if ( n251_1 == n251_2 )
            goto LABEL_16;
        }
      }
    }
    v21 = _this->f16;
    if ( count > 251 || v21 < _this->f12 )
    {
      v22 = _this->live;
      v36 = v21 < 20 * _this->n;
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
      v29 = _this->f8;
      if ( !v23->cnt )
      {
        do
        {
          ++v22;
          _this->f8 = ++v29;
          v30 = v24->cnt;
          --v24;
        }
        while ( !v30 );
        _this->live -= v22;
      }
      v31 = _this->f12;
      _this->f8 = v29 - (v29 >> 1);
      _this->f12 = v31 - (v31 >> 1);
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
    v9 = _this->f8;
    list += v4;
    _this->live = v4 + 1;
    _this->f8 = v8 + v9 + 1;
    list->cnt = 2;
    list->sym = a2;
    _this->f12 += 4;
    if ( list != _this->ent )
    {
      // The new entry starts one place forward, same swap as above.
      v10 = list->sym;
      v11 = list->cnt;
      v12 = list[-1].cnt;
      list->sym = list[-1].sym;
      list->cnt = v12;
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
  uint8_t *v25;   // were int32_t: these hold addresses
  bool v47;
  int16_t v24, v42;
  int32_t n4, v8, n0x7F800000_6, n0x800000_5, v27, v28, v46, v50, v56, v59, i,
          v65, v66, n0x7F800000_5, n0x7F800000_7;
  uint16_t *v3, *v26, n0x4000, v39, v41, *v51, *this_2;
  uint32_t n4_2, v38, v40, v43, v44, v45, v48, v52, n4_1;
  n4 = *((uint8_t *)model_geometry + n2);
  n4_1 = n4;
  v3 = _this + 2;
  v51 = _this + 2;
  // The counts below `n2`, which is where the range coder's interval starts.
  // Four at a time in SSE, then a scalar tail; integer addition does not care
  // about the order, so it is one loop.
  v8 = 0;
  for ( n4_2 = 0; n4_2 < n4_1; n4_2++ )
    v8 += v3[n4_2];
  v51 = &v3[n4_1];
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
    v50 = v46;
    *(_this + 9) = v45;
    *_this = v44 + v45;
    v47 = v46 <= 4 * alt_freq_limit;
    n0x7F800000_6 = n0x7F800000_7;
    if ( v47 )
    {
      n0x800000_5 = 4 * (v50 > alt_freq_limit);
      v24 = v50 - n0x800000_5;
      *(_this + 1) = v50 - n0x800000_5;
    }
    else
    {
      v24 = v50 - 16;
      *(_this + 1) = v50 - 16;
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
    n0x7F800000_5 = n0x7F800000_6;
    v66 = level_geom[n4_1].half;
    v56 = n2 - level_geom[n4_1].first;
    v59 = 0;
    this_2 = _this;
    for ( i = 1; ; i *= 2 )
    {
      v25 = (uint8_t *)&this_2[2 * level_geom[n4_1].tbl_base + 8];
      v26 = (uint16_t *)(v25 + 4 * (i + v59));
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
  int32_t n5a_1, n0x2000_2, result, n256, n5a_2;
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
    return __encode_symbol_tree((uint16_t *)model_tables
           + 32512 * (n5a_2 & 1)
           + 254 * (((a1[1] + (result & 0x7FFF) + 96 - 2 * (uint32_t)a1[n5a_2 + 1]) >> 25) & 0xFFFFFFC0)
           + 254 * a3, (n5a - 5) >> 1);
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
  int32_t n2_1, v11, v18, v23, v25, v26, v36, v38, v40, n2, v43, v44;
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
  v11 = v9 - *v8;
  rc.decode(v11, v9, v39);
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
    v26 = level_geom[n2].first;
    return v25 + v26;
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
        * __decode_symbol_tree((uint16_t *)model_tables
          + 32512 * (n5 & 1)
          + 254
          * ((((uint16_t)a1[1] + (v16 & 0x7FFF) + 96 - 2 * (uint32_t)(uint16_t)a1[n5 + 1]) >> 25)
           & 0xFFFFFFC0)
          + 254 * a3);
  return n5;
}

int32_t __alt_p2_encode_symbol(uint16_t *_this, uint8_t *a2, int32_t a3)
{
  ;
  int16_t v18;
  uint16_t *result;
  int32_t v3, n32;
  uint16_t *v25;
  uint16_t *v5;
  uint32_t tot, tot_1,
           v21, v22, v23;
  v3 = _this[2] + _this[1];
  tot = v3 + _this[3];
  if ( a3 )
  {
    if ( (a3 & 1) != 0 )
    {
      v3 = _this[1];
      v5 = (uint16_t *)_this + 2;
      v25 = ((uint16_t *)_this + 2);
    }
    else
    {
      v5 = (uint16_t *)_this + 3;
      v25 = ((uint16_t *)_this + 3);
    }
  }
  else
  {
    v5 = (uint16_t *)_this + 1;
    v3 = 0;
    v25 = ((uint16_t *)_this + 1);
  }
  tot_1 = v3 + *v5;
  rc.encode(v3, tot_1, tot);
  v18 = *v25;
  if ( *v25 > 0x4000u )
  {
    v21 = _this[2];
    v22 = _this[3];
    _this[1] -= _this[1] >> 1;
    n32 = *_this;
    _this[2] = v21 - (v21 >> 1);
    _this[3] = v22 - (v22 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        v23 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(v23) = 32;
      LOWORD(n32) = n32 - v23;
      *_this = n32;
      v18 = *v25;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      *_this = n32;
      v18 = *v25;
    }
  }
  else
  {
    LOWORD(n32) = *_this;
  }
  result = (uint16_t *)((int32_t)v25);
  *v25 = n32 + v18;
  if ( a3 > 0 )
    return __encode_symbol_tree((uint16_t *)model_tables + 254 * *(uint32_t *)(a2 + 4 * (a3 & 1)), (a3 - 1) >> 1);
  return (int32_t)result;
}

// alt_p2_encode_symbol's counterpart, and called on the same two objects from
// the p2 decoders that the encoders hand to it.  It was `decode_three_way` for
// the shape of its first step -- a three-way choice over the counts at
// `_this[1..3]` -- but so is the encoder's first step, and a pair that codes
// the same thing should read as one.  The three-way part is still the first
// twenty lines; the name now says which half of the pair this is.
int32_t __alt_p2_decode_symbol(uint16_t *_this, uint8_t *a2)
{
  ;
  int32_t v7, v8, v10, n0x4000, n32, v16, v21, v23;
  uint16_t *v9, *v24;
  uint32_t v11, v18, v19, v20;
  v23 = _this[3];
  v21 = _this[2] + _this[1];
  v20 = v23 + v21;
  v7 = rc.get_freq(v20);
  v8 = _this[1];
  if ( v7 >= v8 )
  {
    if ( v7 >= v21 )
    {
      v8 = v21;
      v9 = (uint16_t *)_this + 3;
    }
    else
    {
      v9 = (uint16_t *)_this + 2;
    }
    v24 = (uint16_t *)_this + 1;
  }
  else
  {
    v9 = (uint16_t *)_this + 1;
    v8 = 0;
    v24 = (uint16_t *)_this + 1;
  }
  v10 = (uint16_t)*v9;
  v11 = v8 + v10;
  rc.decode(v8, v11, v20);
  n0x4000 = (uint16_t)*v9;
  if ( n0x4000 > 0x4000 )
  {
    v18 = _this[2];
    v19 = _this[3];
    _this[1] -= _this[1] >> 1;
    n32 = *_this;
    _this[2] = v18 - (v18 >> 1);
    _this[3] = v19 - (v19 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        n0x4000 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(n0x4000) = 32;
      LOWORD(n32) = n32 - n0x4000;
      *_this = n32;
      LOWORD(n0x4000) = *v9;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      *_this = n32;
      LOWORD(n0x4000) = *v9;
    }
  }
  else
  {
    LOWORD(n32) = *_this;
  }
  *v9 = n32 + n0x4000;
  v16 = v9 - v24;
  if ( v16 )
    return v16 + 2 * __decode_symbol_tree((uint16_t *)model_tables + 254 * *(uint32_t *)(a2 + 4 * (v16 & 1)));
  else
    return 0;
}

uint32_t __rescale_three_way(uint16_t *_this)
{
  ;
  uint32_t n32, n0x100;
  *(_this + 1) -= *(_this + 1) >> 1;
  *(_this + 2) -= *(_this + 2) >> 1;
  n32 = *(_this + 3) >> 1;
  *(_this + 3) -= n32;
  n0x100 = *_this;
  if ( n0x100 <= 0x100 )
  {
    if ( *_this <= 0x20u )
      n32 = ((16 - n0x100) >> 30) & 0xFFFFFFFE;
    else
      n32 = 32;
    *_this = n0x100 - n32;
  }
  else
  {
    *_this >>= 1;
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
  SymList *v3, *v7, *v8, *v9, *v12, *v14;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_1;
  ModelBlock *Blocka_2;
  Blocka_1 = (ModelBlock *)(Blocka);
  free(*(void **)&Blocka->f1078240);
  free(Blocka_1->f1078236);
  free(Blocka_1->f1078684);
  free(*(void**)&Blocka_1->f1078688);
  // Both arrays are allocated as `bmf_new(24 * n + 4)` with the count in the
  // word before the first list, so `n` is `((uint32_t *)lists)[-1]` and that
  // same word is what gets freed.  Each list owns its entries.
  v3 = Blocka_1->f1078208;
  if ( v3 )
  {
    v4 = ((uint32_t *)v3)[-1];
    if ( v4 )
    {
      Blocka_2 = (ModelBlock *)(Blocka_1);
      v6 = v4;
      v7 = Blocka_1->f1078208;
      v8 = &v3[v4];
      do
      {
        --v8;
        free(v8->ent);
        --v6;
      }
      while ( v6 );
      v3 = v7;
      Blocka_1 = (ModelBlock *)(Blocka_2);
    }
    free((uint32_t *)v3 - 1);
  }
  v9 = Blocka_1->f1078212;
  if ( v9 )
  {
    v10 = ((uint32_t *)v9)[-1];
    if ( v10 )
    {
      Blocka_3 = (ModelBlock *)(Blocka_1);
      v12 = Blocka_1->f1078212;
      v13 = v10;
      v14 = &v9[v10];
      do
      {
        --v14;
        free(v14->ent);
        --v13;
      }
      while ( v13 );
      v9 = v12;
      Blocka_1 = (ModelBlock *)(Blocka_3);
    }
    free((uint32_t *)v9 - 1);
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
  SymList *v7, *v16;
  bool v5;
  int32_t result, v3, v4, v6, v9, v13, v14, n6;
  SymEntry *v8, *v10, *v11, *v12;
  n6 = *(int32_t *)&_this->sym_pos;
  result = ((uint32_t *)p_n15)[n6];
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
  v5 = v4 + v3 == 0;
  v6 = v4 + v3;
  *(int32_t *)&_this->f52 = v6;
  if ( v5 && n6 > 6 )
    return -1;
  v7 = _this->f1078212;
  v16 = v7;
  v8 = v7[mode_symbol[1]].ent;
  v9 = v6 + 8 * sym_in_top(v8, 10, result);
  *(int32_t *)&_this->f52 = v9;
  v10 = v7[mode_symbol[2]].ent;
  v11 = v7[result].ent;
  v12 = v16[mode_symbol[3]].ent;
  v13 = sym_in_top(v12, 6, result)
      + 2 * sym_in_top(v11, 10, mode_symbol[2])
      + 4 * sym_in_top(v10, 4, result);
  v14 = v13 + v9;
  if ( n6 <= 14 || (v14 & 0xB) != 0 )
  {
    *(int32_t *)&_this->f52 = (n6 << 7) + v14;
    *(int32_t *)&_this->f48 = v13 + 8 * (n6 > 9);
  }
  else
  {
    *(int32_t *)&_this->f48 = v13;
    return -1;
  }
  return result;
}

int32_t __init_model_tables(ModelBlock *_this)
{
  ;
  SymEntry *v8, *v11, *v32;
  uint8_t *v28, *v29;           // row cursors out of f56
  uint8_t v13, v14;
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  uint16_t v6, v12;
  SymList **v3;
  uint8_t *v30;
  int32_t n2_1, v7, v9, v10, v15, v17, n2, v19, v21, v22, v23, v24, v25, v26, v27,
          result;
  uint16_t *v20;
  SymList *v5;
  SymList **v4;
  n2_1 = _this->f32;
  if ( !n2_1 )
  {
    if ( _this->sel == _this->f1078232 )
    {
      if ( _this->sel[0] )
      {
        __symbol_list_update(&_this->f1078212[mode_symbol[1]], **(uint16_t **)&_this->f56[5], 3u);
        __symbol_list_update(&_this->f1078212[**(uint16_t **)&_this->f56[5]], mode_symbol[2], 2u);
        __symbol_list_update(&_this->f1078208[mode_symbol[1]], **(uint16_t **)&_this->f56[5], 4u);
        __symbol_list_update(&_this->f1078208[**(uint16_t **)&_this->f56[5]], mode_symbol[1], 2u);
      }
      else
      {
        __symbol_list_update(&_this->f1078212[mode_symbol[2]], **(uint16_t **)&_this->f56[5], (_this->sym_pos > 3) + 2);
      }
    }
    else
    {
      __symbol_list_update(&_this->f1078212[mode_symbol[1]], **(uint16_t **)&_this->f56[5], 3u);
      __symbol_list_update(&_this->f1078212[**(uint16_t **)&_this->f56[5]], mode_symbol[2], 2u);
      __symbol_list_update(&_this->f1078212[**(uint16_t **)&_this->f56[5]], mode_symbol[1], 1u);
      __symbol_list_update(&_this->f1078208[**(uint16_t **)&_this->f56[5]], mode_symbol[1], 2u);
      v3 = _this->f1078232;
      do
      {
        v4 = v3 - 1;
        _this->f1078232 = v4;
        // `symbol_list_update`'s insert path, inlined: append the symbol at
        // `live`, evicting the last entry when the list is full, then swap it
        // one place forward.
        v5 = *v4;
        v6 = **(uint16_t **)&_this->f56[5];
        v7 = v5->live;
        v32 = v5->ent;
        if ( v7 == (int32_t)v5->n )
        {
          v8 = v5->ent;
          v5->live = --v7;
          v9 = v8[v7].cnt;
        }
        else
        {
          v9 = 1;
        }
        v10 = v5->f8;
        v11 = &v32[v7];
        v5->live = v7 + 1;
        v5->f8 = v9 + v10 + 1;
        v11->sym = v6;
        v11->cnt = 2;
        v5->f12 += 4;
        if ( v11 != v5->ent )
        {
          v12 = v11->sym;
          v13 = v11->cnt;
          v14 = v11[-1].cnt;
          v11->sym = v11[-1].sym;
          v11->cnt = v14;
          v11[-1].sym = v12;
          v11[-1].cnt = v13;
        }
        v3 = _this->f1078232;
      }
      while ( v3 != _this->sel );
    }
    if ( exclusion_gen == -1 )
    {
      v15 = _this->f16;
      exclusion_gen = 1;
      buf = (uint8_t *)exclusion_mask;
      v17 = (v15 + 15) >> 4;
      do
      {
        *((uint32_t *)buf + 3) = 0;
        *((uint32_t *)buf + 2) = 0;
        *((uint32_t *)buf + 1) = 0;
        *(uint32_t *)buf = 0;
        buf += 16;
        --v17;
      }
      while ( v17 );
      n2 = _this->f32;
    }
    else
    {
      ++exclusion_gen;
      n2 = _this->f32;
    }
LABEL_19:
    if ( n2 && n2 <= 2 )
      goto LABEL_37;
    goto LABEL_21;
  }
  if ( n2_1 <= 2 )
    goto LABEL_37;
  if ( mode_symbol[3] != mode_symbol[4] )
  {
    __symbol_list_update(&_this->f1078212[mode_symbol[2]], **(uint16_t **)&_this->f56[5], 1u);
    n2 = _this->f32;
    goto LABEL_19;
  }
LABEL_21:
  v19 = **(uint16_t **)&_this->f56[5];
  v20 = (uint16_t *)(*(uint16_t **)&_this->f6059432);
  v21 = v20[0];
  if ( v19 != v21 )
  {
    v22 = v20[1];
    if ( v19 == v22 )
    {
      v20[1] = v21;
    }
    else
    {
      v23 = v20[2];
      if ( v19 == v23 )
      {
        v20[2] = v22;
        *(uint16_t *)(_this->f6059432 + 2) = **(uint16_t **)&_this->f6059432;
      }
      else
      {
        v24 = v20[3];
        if ( v19 == v24 )
        {
          v20[3] = v23;
          *(uint16_t *)(_this->f6059432 + 4) = *(uint16_t *)(_this->f6059432 + 2);
          *(uint16_t *)(_this->f6059432 + 2) = **(uint16_t **)&_this->f6059432;
        }
        else
        {
          v25 = v20[4];
          if ( v19 == v25 )
          {
            v20[4] = v24;
            *(uint16_t *)(_this->f6059432 + 6) = *(uint16_t *)(_this->f6059432 + 4);
            *(uint16_t *)(_this->f6059432 + 4) = *(uint16_t *)(_this->f6059432 + 2);
            *(uint16_t *)(_this->f6059432 + 2) = **(uint16_t **)&_this->f6059432;
          }
          else
          {
            v26 = v20[5];
            if ( v19 == v26 )
            {
              v20[5] = v25;
              *(uint16_t *)(_this->f6059432 + 8) = *(uint16_t *)(_this->f6059432 + 6);
              *(uint16_t *)(_this->f6059432 + 6) = *(uint16_t *)(_this->f6059432 + 4);
              *(uint16_t *)(_this->f6059432 + 4) = *(uint16_t *)(_this->f6059432 + 2);
              *(uint16_t *)(_this->f6059432 + 2) = **(uint16_t **)&_this->f6059432;
            }
            else
            {
              v27 = v20[6];
              if ( v19 == v27 )
              {
                v20[6] = v26;
              }
              else
              {
                v20[7] = v27;
                *(uint16_t *)(_this->f6059432 + 12) = *(uint16_t *)(_this->f6059432 + 10);
              }
              *(uint16_t *)(_this->f6059432 + 10) = *(uint16_t *)(_this->f6059432 + 8);
              *(uint16_t *)(_this->f6059432 + 8) = *(uint16_t *)(_this->f6059432 + 6);
              *(uint16_t *)(_this->f6059432 + 6) = *(uint16_t *)(_this->f6059432 + 4);
              *(uint16_t *)(_this->f6059432 + 4) = *(uint16_t *)(_this->f6059432 + 2);
              *(uint16_t *)(_this->f6059432 + 2) = **(uint16_t **)&_this->f6059432;
            }
          }
        }
      }
    }
    **(uint16_t **)&_this->f6059432 = v19;
  }
LABEL_37:
  // The other fourteen sites already spell the 16-bit accesses out --
  // `*(uint16_t *)(f6059436 + 2) = *(uint16_t *)f6059436` at 11752 is this
  // line -- so the field is a byte cursor and these two were the odd ones.
  *(uint16_t *)(_this->f6059436 + 2) = *(uint16_t *)_this->f6059436;
  *(uint16_t *)_this->f6059436 = **(uint16_t **)&_this->f56[5];
  // Six neighbours, compared against the symbol just written.
  {
    PixRec *const here = (PixRec *)_this->f56[5];
    PixRec *const up   = (PixRec *)_this->f56[6];
    here->match[0] = here->sym == up->sym;
    here->match[1] = here->sym == here[-1].sym;
    here->match[2] = here->sym == up[1].sym;
    here->match[3] = here->sym == up[-1].sym;
    here->match[4] = here->sym == up[2].sym;
    here->match[5] = here->sym == up[3].sym;
  }
  v28 = _this->f56[6];
  v29 = _this->f56[7];
  v30 = _this->f56[5] + 8;
  _this->f56[5] = v30;
  _this->f56[8] += 8;
  v28 += 8;
  _this->f56[6] = v28;
  v29 += 8;
  _this->f56[9] += 8;
  _this->f56[7] = v29;
  _this->f1078692[0] += *(v28 + 34) - *(v28 - 30);
  _this->f1078692[1] += *(v29 + 34) - *(v29 - 30);
  _this->f1078692[2] += *(v30 - 5) - *(v30 - 37);
  result = *(v30 - 6) - *(v30 - 62);
  _this->f1078692[3] += result;
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
int32_t __alt_p1_context(AltP1Block *_this, uint32_t *a2, uint32_t *a3)
{
  ;
  uint8_t *v5;
  uint8_t *v32;
  bool v9, v19, v20;
  uint8_t *v34;
  int32_t v6, v7, v11, v12, v13, v14, v15, v16, v18, v21, v22, v23, v24, v25, v26, v27, v28,
          v29, result, v35, v36, v39, v40, v41, v42;
  uint32_t v37, v38;
  uint8_t *v3, *v4, *v8, *v10, *v17, *v31, *v33;
  v3 = _this->f176[6];
  v4 = (uint8_t *)*v3;
  v5 = (uint8_t *)((int32_t)_this->f176[5]);
  v6 = v5[-2];
  v7 = *(v3 - 2);
  if ( v6 < (int32_t)v4 )
  {
    if ( v7 >= v6 )
    {
      v10 = &v4[v6 - v7];
      v9 = v7 < (int32_t)v4;
      v4 = (uint8_t *)v5[-2];
      if ( v9 )
        v4 = v10;
    }
  }
  else if ( v7 <= v6 )
  {
    v8 = &v4[v6 - v7];
    v9 = v7 <= (int32_t)v4;
    v4 = (uint8_t *)v5[-2];
    if ( !v9 )
      v4 = v8;
  }
  *(uint8_t **)&_this->f0[2] = v4;
  v33 = _this->f176[7];
  v11 = *(v3 - 1)
      + v5[-5]
      + 3 * (v3[3] + v33[1])
      + 6 * v5[-1]
      + 4 * (v3[1] + v5[-3])
      + 2 * (v33[5] + v3[5] + v5[-7]);
  if ( a2 )
  {
    if ( a3 )
    {
      v34 = (uint8_t *)(v5);
      v12 = a2[49];
      v36 = v11 + 2 * (*(uint8_t *)(a3[49] - 1) + *(uint8_t *)(v12 - 1));
      *(uint8_t **)&_this->f12[25] = (uint8_t *)(*v3
                                       - (uint32_t)v4
                                       + *(uint8_t *)(v12 - 2)
                                       - *(uint8_t *)(a2[50] - 2));
      v13 = a2[49];
      v14 = *(uint8_t *)(v13 - 2);
      v15 = *(uint8_t *)(v13 - 4);
      v5 = (uint8_t *)(v34);
      *(uint8_t **)&_this->f12[29] = (uint8_t *)(*(uint8_t *)((uintptr_t)v34 - 2) - (uint32_t)v4 + v14 - v15);
      *(uint8_t **)&_this->f12[33] = (uint8_t *)(*(uint8_t *)((uintptr_t)v34 - 2)
                                       - (uint32_t)v4
                                       + *(uint8_t *)(a3[49] - 2)
                                       - *(uint8_t *)(a3[49] - 4));
      *(uint8_t **)&_this->f12[37] = (uint8_t *)(*(uint8_t *)(a3[49] - 2) - a3[2]);
      *(uint8_t **)&_this->f12[17] = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = (*(uint8_t *)(a3[49] - 1) + (uint32_t)*(uint8_t *)(a2[49] - 1) - 16) >> 31;
    }
    else
    {
      v36 = v3[7] + v11 + 3 * *(uint8_t *)(a2[49] - 1);
      *(uint8_t **)&_this->f12[25] = (uint8_t *)(2 * v5[-2] - v5[-4] - (uint32_t)v4);
      *(uint8_t **)&_this->f12[29] = (uint8_t *)(2 * v5[-2] - v5[-4] - (uint32_t)v4);
      *(uint8_t **)&_this->f12[33] = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + v5[-2]);
      *(uint8_t **)&_this->f12[37] = (uint8_t *)(v5[-2]
                                       - (uint32_t)v4
                                       + *(uint8_t *)(a2[49] - 2)
                                       - *(uint8_t *)(a2[49] - 4));
      *(uint8_t **)&_this->f12[17] = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = ((uint32_t)*(uint8_t *)(a2[49] - 1) - 8) >> 31;
    }
  }
  else
  {
    v32 = (uint8_t *)((int32_t)_this->f176[9]);
    v36 = v32[1] + *(v33 - 3) + v3[7] + v11 + v32[5];
    *(uint8_t **)&_this->f12[25] = (uint8_t *)(2 * v5[-2] - v5[-4] - (uint32_t)v4);
    *(uint8_t **)&_this->f12[29] = (uint8_t *)(2 * *v3 - *v33 - (uint32_t)v4);
    *(uint8_t **)&_this->f12[33] = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + v5[-2]);
    *(uint8_t **)&_this->f12[37] = (uint8_t *)(-3 * (v5[-4] - v5[-2])
                                     + v5[-6]
                                     - (uint32_t)v4);
    *(uint8_t **)&_this->f12[17] = (uint8_t *)(v3[4] - (uint32_t)v4);
    v37 = v5[1] + v32[1] + (_this->f176[8])[1] + v33[1] + v3[1] == 0;
  }
  v16 = (v36 + 7) >> 4;
  v17 = (uint8_t *)_this->f216[v16];
  v31 = v17;
  v35 = p1_level_step[(uint32_t)v17];
  *(uint8_t **)&_this->f0[3] = v17;
  *(uint8_t **)&_this->f0[4] = &(*(uint8_t **)&_this->f1752[v16])[*((uint8_t *)_this + (uint32_t)v4 + 728)];
  *(uint8_t **)&_this->f12[5] = (uint8_t *)(((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31));
  v38 = ((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31);
  v18 = (*(v3 - 2) - *v3 >= 0) + (*(v3 - 2) > (int32_t)*v3);
  *(uint8_t **)&_this->f12[9] = (uint8_t *)v18;
  v39 = (*(v3 - 2) - v5[-2] >= 0) + (*(v3 - 2) > (int32_t)v5[-2]);
  *(uint8_t **)&_this->f12[13] = (uint8_t *)v39;
  v19 = *(uint8_t **)&_this->f12[17] == nullptr;
  v20 = (int32_t)*(uint8_t **)&_this->f12[17] < 0;
  v40 = (v3[2] - (int32_t)v4 >= -v35) + (v3[2] - (int32_t)v4 > v35);
  *(uint8_t **)&_this->f12[21] = (uint8_t *)v40;
  v21 = v35 < (int32_t)*(uint8_t **)&_this->f12[25];
  v22 = -v35 <= (int32_t)*(uint8_t **)&_this->f12[25];
  v23 = !v20 + (!v20 && !v19);
  *(uint8_t **)&_this->f12[17] = (uint8_t *)v23;
  v24 = v22 + v21;
  v19 = *(uint8_t **)&_this->f12[29] == nullptr;
  v20 = (int32_t)*(uint8_t **)&_this->f12[29] < 0;
  *(uint8_t **)&_this->f12[25] = (uint8_t *)v24;
  v25 = !v20 && !v19;
  v26 = !v20;
  v19 = *(uint8_t **)&_this->f12[33] == nullptr;
  v20 = (int32_t)*(uint8_t **)&_this->f12[33] < 0;
  v41 = v26 + v25;
  *(uint8_t **)&_this->f12[29] = (uint8_t *)(v26 + v25);
  v27 = !v20 && !v19;
  v28 = !v20;
  v19 = *(uint8_t **)&_this->f12[37] == nullptr;
  v20 = (int32_t)*(uint8_t **)&_this->f12[37] < 0;
  v42 = v28 + v27;
  *(uint8_t **)&_this->f12[33] = (uint8_t *)(v28 + v27);
  v29 = !v20 + (!v20 && !v19);
  *(uint8_t **)&_this->f12[37] = (uint8_t *)v29;
  result = (int32_t)&(*(uint8_t **)&_this->f12[v29 + 38])[16 * v37
                                    + 8 * (*((uint8_t **)_this + (uint32_t)*(uint8_t **)&_this->f0[5] + 6) == nullptr)
                                    + (uint32_t)&(*(uint8_t **)&_this->f12[v41 + 30])[(uint32_t)(*(uint8_t **)&_this->f12[v42 + 34])
                                                                        + (uint32_t)&(*(uint8_t **)&_this->f12[v40 + 22])[(uint32_t)(*(uint8_t **)&_this->f12[v24 + 26])]
                                                                        + (uint32_t)&(*(uint8_t **)&_this->f12[v39 + 14])[(uint32_t)(*(uint8_t **)&_this->f12[v23 + 18]) + (uint32_t)&(*(uint8_t **)&_this->f12[v38 + 6])[(uint32_t)(*(uint8_t **)&_this->f12[v18 + 10])]]]
                                    + (uint32_t)v31];
  *(uint8_t **)&_this->f0[3] = (uint8_t *)result;
  return result;
}

int32_t __update_binary_pair(uint16_t *_this, int32_t symbol)
{
  ;
  uint8_t *v12;   // was int32_t: these hold addresses
  int32_t n0x8000, v3, v5, v6, v7, v11, v13;
  uint16_t *v8;
  uint32_t v4, n0x2000, v10;
  n0x8000 = *_this;
  if ( (uint32_t)n0x8000 <= 0x8000 )
  {
    v3 = *((uint8_t *)model_geometry + symbol);
    v4 = (_this[1] >> 2) & 0xFFFFFFE0;
    if ( ::plane_predictor == 2 )
      v4 = 15 * (_this[1] >> 5);
    *((uint16_t *)_this + v3 + 2) += v4 + 4;
    n0x8000 = *_this + v4 + 4;
    *_this = n0x8000;
    if ( symbol >= 2 )
    {
      n0x8000 = level_geom[v3].half;
      v11 = symbol - level_geom[v3].first;
      v5 = (int32_t)((uint16_t *)_this + 2 * level_geom[v3].tbl_base + 8);
      v6 = 0;
      v12 = (uint8_t *)v5;
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
  uintptr_t result;   // were int32_t: addresses, masked and tagged
  P1Count *v111;
  P1Count *v9;
  P1Count *v11;
  P1Count *v13;
  uint8_t *v17, *v23, *v31, *v39, *v47, *v55, *v63, *v71, *v79;
  int16_t v12, v15;
  int32_t v3, n5_1, v5, n5_2, n5_3, v8, v14, v16, v18, v21, v22, v24, v25, v26, v29, v30, v32,
          v33, v34, v37, v38, v40, v41, v42, v45, v46, v48, v49, v50, v53, v54, v56, v57, v58,
          v61, v62, v64, v65, v66, v69, v70, v72, v73, v74, v77, v78, v80, v82, v83, v85, v86,
          v89, v92, v95, v98, v101, v104, v107, n2, n5_4;
  P1Count *v108;
  P1Count *v109;
  P1Count *v19;
  P1Count *v20;
  P1Count *v105;
  P1Count *v106;
  P1Count *v27;
  P1Count *v28;
  P1Count *v102;
  P1Count *v103;
  P1Count *v35;
  P1Count *v36;
  P1Count *v99;
  P1Count *v100;
  P1Count *v43;
  P1Count *v44;
  P1Count *v96;
  P1Count *v97;
  P1Count *v51;
  P1Count *v52;
  P1Count *v93;
  P1Count *v94;
  P1Count *v59;
  P1Count *v60;
  P1Count *v90;
  P1Count *v91;
  P1Count *v67;
  P1Count *v68;
  P1Count *v87;
  P1Count *v88;
  P1Count *v75;
  P1Count *v76;
  P1Count *v84;
  P1Count *v81;
  uint32_t n5, v110, v112;
  n5 = *((uint8_t)(**(uint8_t **)&_this->f12[46] - *(uint8_t *)&_this->f8) + _this->f984);
  v3 = _this->f12[9];
  n5_1 = *((uint8_t)(*(uint8_t *)&_this->f8 - **(uint8_t **)&_this->f12[46]) + _this->f984);
  v5 = _this->f12[13];
  n2 = (int32_t)(n5 - 5) >> 1;
  n5_2 = 6 - (n5 & 1);
  if ( n5 < 5 )
    n5_2 = *((uint8_t)(**(uint8_t **)&_this->f12[46] - *(uint8_t *)&_this->f8) + _this->f984);
  n5_3 = 6 - (n5_1 & 1);
  if ( n5_1 < 5 )
    n5_3 = n5_1;
  n5_4 = n5_3;
  v8 = _this->f12[40 - _this->f12[37]]
     + _this->f12[36 - _this->f12[33]]
     + _this->f12[32 - _this->f12[29]]
     + _this->f12[28 - _this->f12[25]]
     + _this->f12[24 - _this->f12[21]]
     + _this->f12[20 - _this->f12[17]]
     + _this->f12[16 - v5]
     + _this->f12[12 - v3]
     + _this->f12[7]
     + (_this->f12[0] & 0x1F);
  v9 = &((P1Count *)_this)[v8];
  v111 = (P1Count *)(v9);
  ((P1Count *)v9)[237].bin[n5_3] += 17;
  v9[237].total += 17;
  result = _this->f12[0];
  if ( (result & 7) != 7 )
  {
    v11 = &((P1Count *)_this)[result];
    v110 = (((_this->f12[1] & 7u) - 7) >> 31) + _this->f12[1];
    ((P1Count *)v11)[238].bin[n5_2] += 11;
    v12 = v11[238].total + 11;
    v11[238].total = v12;
    if ( n5_2 >= 5 )
      __update_binary_pair((uint16_t *)model_tables
      + 32512 * (n5_2 & 1)
      + 254 * v110
      + 254
      * ((((v12 & 0x7FFF)
         + v11[238].bin[0]
         - 2 * (uint32_t)((P1Count *)v11)[238].bin[n5_2]) >> 25)
       & 0xFFFFFFC0), (int32_t)(n5 - 5) >> 1);
    result = _this->f12[0];
  }
  if ( (result & 7) != 0 )
  {
    v13 = &((P1Count *)_this)[result];
    v14 = _this->f12[1] - ((_this->f12[1] & 7) != 0);
    ((P1Count *)v13)[236].bin[n5_2] += 13;
    v15 = v13[236].total + 13;
    v13[236].total = v15;
    if ( n5_2 >= 5 )
      __update_binary_pair((uint16_t *)model_tables
      + 32512 * (n5_2 & 1)
      + 254 * v14
      + 254
      * ((((v15 & 0x7FFF)
         + v13[236].bin[0]
         - 2 * (uint32_t)((P1Count *)v13)[236].bin[n5_2]) >> 25)
       & 0xFFFFFFC0), n2);
    result = _this->f12[0];
  }
  if ( (*(P1Count *)&_this->f1752[4 * result + 510]).total < 0xCCCu )
  {
    if ( (result & 7u) < 7 )
    {
      ((P1Count *)v111)[238].bin[n5_4] += 7;
      v111[238].total += 7;
      result = _this->f12[0];
    }
    if ( (result & 7) != 0 )
    {
      ((P1Count *)v111)[236].bin[n5_4] += 5;
      v111[236].total += 5;
      result = _this->f12[0];
    }
    if ( n5_2 >= 5 )
    {
      v112 = _this->f12[1]
           + ((((*(P1Count *)&_this->f1752[4 * result + 510]).bin[0]
              + ((*(P1Count *)&_this->f1752[4 * result + 510]).total & 0x7FFF)
              - 2 * (uint32_t)(*(P1Count *)&_this->f1752[4 * result + 510]).bin[n5_2]) >> 25)
            & 0xFFFFFFC0)
           + ((n5_2 & 1) << 7);
      if ( (v112 & 0x38) >= 0x38
        || (__update_binary_pair((uint16_t *)model_tables
            + 32512 * (n5_2 & 1)
            + 254 * _this->f12[1]
            + 254
            * ((((*(P1Count *)&_this->f1752[4 * result + 510]).bin[0]
               + ((*(P1Count *)&_this->f1752[4 * result + 510]).total & 0x7FFF)
               - 2 * (uint32_t)(*(P1Count *)&_this->f1752[4 * result + 510]).bin[n5_2]) >> 25)
             & 0xFFFFFFC0)
            + 2032, n2),
            (v112 & 0x38) != 0) )
      {
        __update_binary_pair((uint16_t *)model_tables + 254 * v112 - 2032, n2);
      }
      result = _this->f12[0];
    }
    v16 = _this->f12[5];
    if ( v16 == 1 )
    {
      v107 = result - _this->f12[7];
      v108 = &((P1Count *)_this)[v107 + _this->f12[6]];
      v109 = &((P1Count *)_this)[_this->f12[8] + v107];
      v108[237].bin[n5_2] += 6;
      v108[237].total += 6;
      v109[237].bin[n5_2] += 6;
      v109[237].total += 6;
      v21 = _this->f12[0];
      if ( (v21 & 7) != 7 )
      {
        v108[238].bin[n5_2] += 4;
        v108[238].total += 4;
        v109[238].bin[n5_2] += 4;
        v109[238].total += 4;
        v21 = _this->f12[0];
      }
      if ( (v21 & 7) != 0 )
      {
        v108[236].bin[n5_2] += 3;
        v108[236].total += 3;
        v109[236].bin[n5_2] += 3;
        v109[236].total += 3;
        v21 = _this->f12[0];
      }
    }
    else
    {
      v17 = (uint8_t *)_this + 16 * (_this->f12[8 - v16] + result - _this->f12[6 + v16]);
      ((P1Count *)v17)[237].bin[n5_2] += 7;
      ((P1Count *)v17)[237].total += 7;
      v18 = v8 + _this->f12[6] - _this->f12[7];
      v19 = (P1Count *)((uint16_t *)((uint8_t *)_this
                    + 16
                    * (_this->f12[7]
                     + _this->f12[0]
                     - _this->f12[6 + _this->f12[5]])));
      v19[237].bin[n5_2] += 6;
      v19[237].total += 6;
      v20 = &((P1Count *)_this)[v18];
      v20[237].bin[n5_4] += 4;
      v20[237].total += 4;
      v21 = _this->f12[0];
      if ( (v21 & 7) != 7 )
      {
        v19[238].bin[n5_2] += 4;
        v19[238].total += 4;
        v20[238].bin[n5_4] += 2;
        v20[238].total += 2;
        v21 = _this->f12[0];
      }
      if ( (v21 & 7) != 0 )
      {
        v19[236].bin[n5_2] += 3;
        v19[236].total += 3;
        v20[236].bin[n5_4] += 2;
        v20[236].total += 2;
        v21 = _this->f12[0];
      }
    }
    v22 = _this->f12[9];
    if ( v22 == 1 )
    {
      v104 = v21 - _this->f12[11];
      v105 = &((P1Count *)_this)[v104 + _this->f12[10]];
      v106 = &((P1Count *)_this)[_this->f12[12] + v104];
      v105[237].bin[n5_2] += 6;
      v105[237].total += 6;
      v106[237].bin[n5_2] += 6;
      v106[237].total += 6;
      v29 = _this->f12[0];
      if ( (v29 & 7) != 7 )
      {
        v105[238].bin[n5_2] += 4;
        v105[238].total += 4;
        v106[238].bin[n5_2] += 4;
        v106[238].total += 4;
        v29 = _this->f12[0];
      }
      if ( (v29 & 7) != 0 )
      {
        v105[236].bin[n5_2] += 3;
        v105[236].total += 3;
        v106[236].bin[n5_2] += 3;
        v106[236].total += 3;
        v29 = _this->f12[0];
      }
    }
    else
    {
      v23 = (uint8_t *)_this + 16 * (_this->f12[12 - v22] + v21 - _this->f12[10 + v22]);
      ((P1Count *)v23)[237].bin[n5_2] += 7;
      ((P1Count *)v23)[237].total += 7;
      v24 = _this->f12[11];
      v25 = v24 + _this->f12[0] - _this->f12[10 + _this->f12[9]];
      v26 = v8 + v24 - _this->f12[12 - _this->f12[9]];
      v27 = &((P1Count *)_this)[v25];
      v27[237].bin[n5_2] += 6;
      v27[237].total += 6;
      v28 = &((P1Count *)_this)[v26];
      v28[237].bin[n5_4] += 4;
      v28[237].total += 4;
      v29 = _this->f12[0];
      if ( (v29 & 7) != 7 )
      {
        v27[238].bin[n5_2] += 4;
        v27[238].total += 4;
        v28[238].bin[n5_4] += 2;
        v28[238].total += 2;
        v29 = _this->f12[0];
      }
      if ( (v29 & 7) != 0 )
      {
        v27[236].bin[n5_2] += 3;
        v27[236].total += 3;
        v28[236].bin[n5_4] += 2;
        v28[236].total += 2;
        v29 = _this->f12[0];
      }
    }
    v30 = _this->f12[13];
    if ( v30 == 1 )
    {
      v101 = v29 - _this->f12[15];
      v102 = &((P1Count *)_this)[v101 + _this->f12[14]];
      v103 = &((P1Count *)_this)[_this->f12[16] + v101];
      v102[237].bin[n5_2] += 6;
      v102[237].total += 6;
      v103[237].bin[n5_2] += 6;
      v103[237].total += 6;
      v37 = _this->f12[0];
      if ( (v37 & 7) != 7 )
      {
        v102[238].bin[n5_2] += 4;
        v102[238].total += 4;
        v103[238].bin[n5_2] += 4;
        v103[238].total += 4;
        v37 = _this->f12[0];
      }
      if ( (v37 & 7) != 0 )
      {
        v102[236].bin[n5_2] += 3;
        v102[236].total += 3;
        v103[236].bin[n5_2] += 3;
        v103[236].total += 3;
        v37 = _this->f12[0];
      }
    }
    else
    {
      v31 = (uint8_t *)_this + 16 * (_this->f12[16 - v30] + v29 - _this->f12[14 + v30]);
      ((P1Count *)v31)[237].bin[n5_2] += 7;
      ((P1Count *)v31)[237].total += 7;
      v32 = _this->f12[15];
      v33 = v32 + _this->f12[0] - _this->f12[14 + _this->f12[13]];
      v34 = v8 + v32 - _this->f12[16 - _this->f12[13]];
      v35 = &((P1Count *)_this)[v33];
      v35[237].bin[n5_2] += 6;
      v35[237].total += 6;
      v36 = &((P1Count *)_this)[v34];
      v36[237].bin[n5_4] += 4;
      v36[237].total += 4;
      v37 = _this->f12[0];
      if ( (v37 & 7) != 7 )
      {
        v35[238].bin[n5_2] += 4;
        v35[238].total += 4;
        v36[238].bin[n5_4] += 2;
        v36[238].total += 2;
        v37 = _this->f12[0];
      }
      if ( (v37 & 7) != 0 )
      {
        v35[236].bin[n5_2] += 3;
        v35[236].total += 3;
        v36[236].bin[n5_4] += 2;
        v36[236].total += 2;
        v37 = _this->f12[0];
      }
    }
    v38 = _this->f12[17];
    if ( v38 == 1 )
    {
      v98 = v37 - _this->f12[19];
      v99 = &((P1Count *)_this)[v98 + _this->f12[18]];
      v100 = &((P1Count *)_this)[_this->f12[20] + v98];
      v99[237].bin[n5_2] += 6;
      v99[237].total += 6;
      v100[237].bin[n5_2] += 6;
      v100[237].total += 6;
      v45 = _this->f12[0];
      if ( (v45 & 7) != 7 )
      {
        v99[238].bin[n5_2] += 4;
        v99[238].total += 4;
        v100[238].bin[n5_2] += 4;
        v100[238].total += 4;
        v45 = _this->f12[0];
      }
      if ( (v45 & 7) != 0 )
      {
        v99[236].bin[n5_2] += 3;
        v99[236].total += 3;
        v100[236].bin[n5_2] += 3;
        v100[236].total += 3;
        v45 = _this->f12[0];
      }
    }
    else
    {
      v39 = (uint8_t *)_this + 16 * (_this->f12[20 - v38] + v37 - _this->f12[18 + v38]);
      ((P1Count *)v39)[237].bin[n5_2] += 7;
      ((P1Count *)v39)[237].total += 7;
      v40 = _this->f12[19];
      v41 = v40 + _this->f12[0] - _this->f12[18 + _this->f12[17]];
      v42 = v8 + v40 - _this->f12[20 - _this->f12[17]];
      v43 = &((P1Count *)_this)[v41];
      v43[237].bin[n5_2] += 6;
      v43[237].total += 6;
      v44 = &((P1Count *)_this)[v42];
      v44[237].bin[n5_4] += 4;
      v44[237].total += 4;
      v45 = _this->f12[0];
      if ( (v45 & 7) != 7 )
      {
        v43[238].bin[n5_2] += 4;
        v43[238].total += 4;
        v44[238].bin[n5_4] += 2;
        v44[238].total += 2;
        v45 = _this->f12[0];
      }
      if ( (v45 & 7) != 0 )
      {
        v43[236].bin[n5_2] += 3;
        v43[236].total += 3;
        v44[236].bin[n5_4] += 2;
        v44[236].total += 2;
        v45 = _this->f12[0];
      }
    }
    v46 = _this->f12[21];
    if ( v46 == 1 )
    {
      v95 = v45 - _this->f12[23];
      v96 = &((P1Count *)_this)[v95 + _this->f12[22]];
      v97 = &((P1Count *)_this)[_this->f12[24] + v95];
      v96[237].bin[n5_2] += 6;
      v96[237].total += 6;
      v97[237].bin[n5_2] += 6;
      v97[237].total += 6;
      v53 = _this->f12[0];
      if ( (v53 & 7) != 7 )
      {
        v96[238].bin[n5_2] += 4;
        v96[238].total += 4;
        v97[238].bin[n5_2] += 4;
        v97[238].total += 4;
        v53 = _this->f12[0];
      }
      if ( (v53 & 7) != 0 )
      {
        v96[236].bin[n5_2] += 3;
        v96[236].total += 3;
        v97[236].bin[n5_2] += 3;
        v97[236].total += 3;
        v53 = _this->f12[0];
      }
    }
    else
    {
      v47 = (uint8_t *)_this + 16 * (_this->f12[24 - v46] + v45 - _this->f12[22 + v46]);
      ((P1Count *)v47)[237].bin[n5_2] += 7;
      ((P1Count *)v47)[237].total += 7;
      v48 = _this->f12[23];
      v49 = v48 + _this->f12[0] - _this->f12[22 + _this->f12[21]];
      v50 = v8 + v48 - _this->f12[24 - _this->f12[21]];
      v51 = &((P1Count *)_this)[v49];
      v51[237].bin[n5_2] += 6;
      v51[237].total += 6;
      v52 = &((P1Count *)_this)[v50];
      v52[237].bin[n5_4] += 4;
      v52[237].total += 4;
      v53 = _this->f12[0];
      if ( (v53 & 7) != 7 )
      {
        v51[238].bin[n5_2] += 4;
        v51[238].total += 4;
        v52[238].bin[n5_4] += 2;
        v52[238].total += 2;
        v53 = _this->f12[0];
      }
      if ( (v53 & 7) != 0 )
      {
        v51[236].bin[n5_2] += 3;
        v51[236].total += 3;
        v52[236].bin[n5_4] += 2;
        v52[236].total += 2;
        v53 = _this->f12[0];
      }
    }
    v54 = _this->f12[25];
    if ( v54 == 1 )
    {
      v92 = v53 - _this->f12[27];
      v93 = &((P1Count *)_this)[v92 + _this->f12[26]];
      v94 = &((P1Count *)_this)[_this->f12[28] + v92];
      v93[237].bin[n5_2] += 6;
      v93[237].total += 6;
      v94[237].bin[n5_2] += 6;
      v94[237].total += 6;
      v61 = _this->f12[0];
      if ( (v61 & 7) != 7 )
      {
        v93[238].bin[n5_2] += 4;
        v93[238].total += 4;
        v94[238].bin[n5_2] += 4;
        v94[238].total += 4;
        v61 = _this->f12[0];
      }
      if ( (v61 & 7) != 0 )
      {
        v93[236].bin[n5_2] += 3;
        v93[236].total += 3;
        v94[236].bin[n5_2] += 3;
        v94[236].total += 3;
        v61 = _this->f12[0];
      }
    }
    else
    {
      v55 = (uint8_t *)_this + 16 * (_this->f12[28 - v54] + v53 - _this->f12[26 + v54]);
      ((P1Count *)v55)[237].bin[n5_2] += 7;
      ((P1Count *)v55)[237].total += 7;
      v56 = _this->f12[27];
      v57 = v56 + _this->f12[0] - _this->f12[26 + _this->f12[25]];
      v58 = v8 + v56 - _this->f12[28 - _this->f12[25]];
      v59 = &((P1Count *)_this)[v57];
      v59[237].bin[n5_2] += 6;
      v59[237].total += 6;
      v60 = &((P1Count *)_this)[v58];
      v60[237].bin[n5_4] += 4;
      v60[237].total += 4;
      v61 = _this->f12[0];
      if ( (v61 & 7) != 7 )
      {
        v59[238].bin[n5_2] += 4;
        v59[238].total += 4;
        v60[238].bin[n5_4] += 2;
        v60[238].total += 2;
        v61 = _this->f12[0];
      }
      if ( (v61 & 7) != 0 )
      {
        v59[236].bin[n5_2] += 3;
        v59[236].total += 3;
        v60[236].bin[n5_4] += 2;
        v60[236].total += 2;
        v61 = _this->f12[0];
      }
    }
    v62 = _this->f12[29];
    if ( v62 == 1 )
    {
      v89 = v61 - _this->f12[31];
      v90 = &((P1Count *)_this)[v89 + _this->f12[30]];
      v91 = &((P1Count *)_this)[_this->f12[32] + v89];
      v90[237].bin[n5_2] += 6;
      v90[237].total += 6;
      v91[237].bin[n5_2] += 6;
      v91[237].total += 6;
      v69 = _this->f12[0];
      if ( (v69 & 7) != 7 )
      {
        v90[238].bin[n5_2] += 4;
        v90[238].total += 4;
        v91[238].bin[n5_2] += 4;
        v91[238].total += 4;
        v69 = _this->f12[0];
      }
      if ( (v69 & 7) != 0 )
      {
        v90[236].bin[n5_2] += 3;
        v90[236].total += 3;
        v91[236].bin[n5_2] += 3;
        v91[236].total += 3;
        v69 = _this->f12[0];
      }
    }
    else
    {
      v63 = (uint8_t *)_this + 16 * (_this->f12[32 - v62] + v61 - _this->f12[30 + v62]);
      ((P1Count *)v63)[237].bin[n5_2] += 7;
      ((P1Count *)v63)[237].total += 7;
      v64 = _this->f12[31];
      v65 = v64 + _this->f12[0] - _this->f12[30 + _this->f12[29]];
      v66 = v8 + v64 - _this->f12[32 - _this->f12[29]];
      v67 = &((P1Count *)_this)[v65];
      v67[237].bin[n5_2] += 6;
      v67[237].total += 6;
      v68 = &((P1Count *)_this)[v66];
      v68[237].bin[n5_4] += 4;
      v68[237].total += 4;
      v69 = _this->f12[0];
      if ( (v69 & 7) != 7 )
      {
        v67[238].bin[n5_2] += 4;
        v67[238].total += 4;
        v68[238].bin[n5_4] += 2;
        v68[238].total += 2;
        v69 = _this->f12[0];
      }
      if ( (v69 & 7) != 0 )
      {
        v67[236].bin[n5_2] += 3;
        v67[236].total += 3;
        v68[236].bin[n5_4] += 2;
        v68[236].total += 2;
        v69 = _this->f12[0];
      }
    }
    v70 = _this->f12[33];
    if ( v70 == 1 )
    {
      v86 = v69 - _this->f12[35];
      v87 = &((P1Count *)_this)[v86 + _this->f12[34]];
      v88 = &((P1Count *)_this)[_this->f12[36] + v86];
      v87[237].bin[n5_2] += 6;
      v87[237].total += 6;
      v88[237].bin[n5_2] += 6;
      v88[237].total += 6;
      v77 = _this->f12[0];
      if ( (v77 & 7) != 7 )
      {
        v87[238].bin[n5_2] += 4;
        v87[238].total += 4;
        v88[238].bin[n5_2] += 4;
        v88[238].total += 4;
        v77 = _this->f12[0];
      }
      if ( (v77 & 7) != 0 )
      {
        v87[236].bin[n5_2] += 3;
        v87[236].total += 3;
        v88[236].bin[n5_2] += 3;
        v88[236].total += 3;
        v77 = _this->f12[0];
      }
    }
    else
    {
      v71 = (uint8_t *)_this + 16 * (_this->f12[36 - v70] + v69 - _this->f12[34 + v70]);
      ((P1Count *)v71)[237].bin[n5_2] += 7;
      ((P1Count *)v71)[237].total += 7;
      v72 = _this->f12[35];
      v73 = v72 + _this->f12[0] - _this->f12[34 + _this->f12[33]];
      v74 = v8 + v72 - _this->f12[36 - _this->f12[33]];
      v75 = &((P1Count *)_this)[v73];
      v75[237].bin[n5_2] += 6;
      v75[237].total += 6;
      v76 = &((P1Count *)_this)[v74];
      v76[237].bin[n5_4] += 4;
      v76[237].total += 4;
      v77 = _this->f12[0];
      if ( (v77 & 7) != 7 )
      {
        v75[238].bin[n5_2] += 4;
        v75[238].total += 4;
        v76[238].bin[n5_4] += 2;
        v76[238].total += 2;
        v77 = _this->f12[0];
      }
      if ( (v77 & 7) != 0 )
      {
        v75[236].bin[n5_2] += 3;
        v75[236].total += 3;
        v76[236].bin[n5_4] += 2;
        v76[236].total += 2;
        v77 = _this->f12[0];
      }
    }
    v78 = _this->f12[37];
    if ( v78 == 1 )
    {
      v83 = v77 - _this->f12[39];
      v84 = &((P1Count *)_this)[v83 + _this->f12[38]];
      result = (uintptr_t)((uint8_t *)_this + 16 * (_this->f12[40] + v83));
      v84[237].bin[n5_2] += 6;
      v84[237].total += 6;
      ((P1Count *)result)[237].bin[n5_2] += 6;
      ((P1Count *)result)[237].total += 6;
      v85 = _this->f12[0];
      if ( (v85 & 7) != 7 )
      {
        v84[238].bin[n5_2] += 4;
        v84[238].total += 4;
        ((P1Count *)result)[238].bin[n5_2] += 4;
        ((P1Count *)result)[238].total += 4;
        v85 = _this->f12[0];
      }
      if ( (v85 & 7) != 0 )
      {
        v84[236].bin[n5_2] += 3;
        v84[236].total += 3;
        ((P1Count *)result)[236].bin[n5_2] += 3;
        ((P1Count *)result)[236].total += 3;
      }
    }
    else
    {
      v79 = (uint8_t *)_this + 16 * (_this->f12[40 - v78] + v77 - _this->f12[38 + v78]);
      ((P1Count *)v79)[237].bin[n5_2] += 7;
      ((P1Count *)v79)[237].total += 7;
      v80 = _this->f12[39];
      result = (uintptr_t)((uint8_t *)_this + 16 * (v80 + _this->f12[0] - _this->f12[38 + _this->f12[37]]));
      v81 = &((P1Count *)_this)[v80 - _this->f12[40 - _this->f12[37]] + v8];
      ((P1Count *)result)[237].bin[n5_2] += 6;
      ((P1Count *)result)[237].total += 6;
      v81[237].bin[n5_4] += 4;
      v81[237].total += 4;
      v82 = _this->f12[0];
      if ( (v82 & 7) != 7 )
      {
        ((P1Count *)result)[238].bin[n5_2] += 4;
        ((P1Count *)result)[238].total += 4;
        v81[238].bin[n5_4] += 2;
        v81[238].total += 2;
        v82 = _this->f12[0];
      }
      if ( (v82 & 7) != 0 )
      {
        ((P1Count *)result)[236].bin[n5_2] += 3;
        ((P1Count *)result)[236].total += 3;
        result = (uint16_t)v81[236].bin[n5_4] + 2;
        v81[236].bin[n5_4] = result;
        v81[236].total += 2;
      }
    }
  }
  return result;
}

int32_t *__alt_p1_alloc(AltP1Block *_this, int32_t i, int32_t a3, int32_t n4)
{
  ;
  bool v11;
  int32_t v6, v7, v8, v9, v10, v12, v13, v15, v16, v17, v18, v20, v21, v22, v23, v24, v25, v27,
          v28;
  uint32_t n0x99C60, n0x80, n5;
  n0x99C60 = 0;
  _this->f0[0] = i;
  _this->f4 = a3;
  do
    __init_counter_node((uint16_t *)_this + 8 * n0x99C60++ + 1900);
  while ( n0x99C60 < 0x99C60 );
  _this->f8 = 0;
  v6 = 0;
  v7 = 0;
  v8 = 0;
  v27 = n4 << 8;
  do
  {
    v9 = p1_level_edges[v6];
    v28 = v8;
    _this->f216[2 * v8] = v6;
    v10 = (2 * v8 == v9) + v6;
    *((int32_t *)_this + 2 * v8 + 438) = v27 | v7;
    v11 = 2 * v8 == p1_group_edges[v7];
    _this->f216[2 * v8 + 1] = v10;
    v12 = 2 * v8 + 1;
    v13 = v11 + v7;
    v6 = (v12 == p1_level_edges[v10]) + v10;
    *((int32_t *)_this + 2 * v8 + 439) = v13 | v27;
    v7 = (v12 == p1_group_edges[v13]) + v13;
    ++v8;
  }
  while ( (uint32_t)(v28 + 1) < 0x100 );
  n0x80 = 0;
  v15 = 0;
  do
  {
    v16 = p1_slot_edges[v15];
    _this->f728[2 * n0x80] = 8 * v15;
    v17 = (2 * n0x80 == v16) + v15;
    _this->f728[2 * n0x80 + 1] = 8 * v17;
    v18 = 2 * n0x80++ + 1 == p1_slot_edges[v17];
    v15 = v18 + v17;
  }
  while ( n0x80 < 0x80 );
  _this->f12[7] = 32;
  _this->f12[38] = 0;
  _this->f12[34] = 0;
  _this->f12[8] = 64;
  _this->f12[30] = 0;
  _this->f12[26] = 0;
  _this->f12[22] = 0;
  _this->f12[11] = 96;
  _this->f12[18] = 0;
  _this->f12[14] = 0;
  _this->f12[10] = 0;
  _this->f12[12] = 192;
  _this->f12[6] = 0;
  n5 = 0;
  _this->f12[15] = 288;
  _this->f12[16] = 576;
  _this->f12[19] = 864;
  _this->f12[20] = 1728;
  _this->f12[23] = 2592;
  _this->f12[24] = 5184;
  _this->f12[27] = 7776;
  _this->f12[28] = 15552;
  _this->f12[31] = 23328;
  _this->f12[32] = 46656;
  _this->f12[35] = 69984;
  _this->f12[36] = 139968;
  _this->f12[39] = 209952;
  _this->f12[40] = 419904;
  do
    *((int32_t *)_this + n5++ + 44) = (int32_t)bmf_new(2 * _this->f0[0] + 20);
  while ( n5 < 5 );
  __alt_init_tables(_this->f984, (int8_t *)_this->f1496);
  v20 = _this->f0[0];
  if ( _this->f0[0] > -10 )
  {
    v21 = 0;
    do
    {
      *(uint8_t *)(_this->f12[45] + 2 * v21) = 72;
      *(uint8_t *)(_this->f12[44] + 2 * v21) = 72;
      *(uint8_t *)(_this->f12[43] + 2 * v21) = 72;
      *(uint8_t *)(_this->f12[42] + 2 * v21) = 72;
      *(uint8_t *)(_this->f12[41] + 2 * v21) = 72;
      *(uint8_t *)(_this->f12[45] + 2 * v21 + 1) = 0;
      *(uint8_t *)(_this->f12[44] + 2 * v21 + 1) = 0;
      *(uint8_t *)(_this->f12[43] + 2 * v21 + 1) = 0;
      *(uint8_t *)(_this->f12[42] + 2 * v21 + 1) = 0;
      *(uint8_t *)(_this->f12[41] + 2 * v21 + 1) = 0;
      v20 = _this->f0[0];
      ++v21;
    }
    while ( v21 < _this->f0[0] + 10 );
  }
  v22 = _this->f12[42];
  _this->f12[46] = _this->f12[41] + 2 * v20 + 8;
  v23 = _this->f12[43];
  _this->f12[47] = v22 + 2 * v20 + 8;
  v24 = _this->f12[44];
  _this->f12[48] = v23 + 2 * v20 + 8;
  v25 = _this->f12[45];
  _this->f12[49] = v24 + 2 * v20 + 8;
  _this->f12[50] = v25 + 2 * v20 + 8;
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
    model_geometry[0] = 0x02020100;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    model_geometry[1] = 0x03030303;
    level_geom[4].half = 4;
    v2 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    *(uint64_t *)((uint8_t *)model_geometry + v2) = 0x404040404040404LL;
    level_geom[5].half = 8;
    v3 = v2 + 2 * level_geom[4].half;
    level_geom[5].first = v3;
    level_geom[5].tbl_base = v3 - 5;
    memset((uint8_t *)model_geometry + v3, 0x05, 16);
    level_geom[6].half = 16;
    v4 = v3 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)v4;
    level_geom[6].tbl_base = (uint8_t)v4 - 6;
    memset((uint8_t *)model_geometry + v4, 0x06, 32);
    level_geom[7].half = 32;
    v6 = v4 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)v6;
    level_geom[7].tbl_base = (uint8_t)v6 - 7;
    // 64 bytes of 7, after 16 of 5 and 32 of 6 -- one level per line.  MSVC
    // inlined this third one because the length crossed its threshold, which
    // is why it arrived as a scalar head, three aligned stores and a tail.
    memset((uint8_t *)model_geometry + v6, 0x07, 64);
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
  ;
  uint8_t *v9;   // were int32_t: these hold addresses
  bool v31;
  uint8_t v33, v35;
  int32_t v4, v5, v6, v7, v8, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24,
          v25, v26, v27, v28, v29, v30, v34, n5, n16, v38, v40, v42;
  int64_t v39;
  uint8_t *v10, *v11, *v32;
  __rc_begin_encode();
  if ( _this->f4 > 0 )
  {
    v4 = 0;
    do
    {
      ++v4;
      **(uint16_t **)&_this->f12[46] = *(uint16_t *)(_this->f12[46] - 2);
      *(uint16_t *)(_this->f12[46] + 2) = *(uint16_t *)(_this->f12[46] - 4);
      *(uint16_t *)(_this->f12[46] + 4) = *(uint16_t *)(_this->f12[46] - 6);
      *(uint16_t *)(_this->f12[46] + 6) = *(uint16_t *)(_this->f12[46] - 8);
      *(uint16_t *)(_this->f12[46] + 8) = *(uint16_t *)(_this->f12[46] - 10);
      *(uint16_t *)(_this->f12[46] + 10) = *(uint16_t *)(_this->f12[46] - 12);
      v5 = _this->f12[45];
      v6 = _this->f12[44];
      v7 = _this->f12[43];
      v8 = _this->f12[42];
      v9 = (uint8_t *)_this->f12[41];
      _this->f12[45] = v6;
      _this->f12[44] = v7;
      _this->f12[43] = v8;
      _this->f12[42] = (uint32_t)v9;
      _this->f12[41] = v5;
      v5 += 8;
      _this->f12[46] = v5;
      v9 += 8;
      _this->f12[47] = (uint32_t)v9;
      _this->f12[48] = v8 + 8;
      _this->f12[49] = v7 + 8;
      _this->f12[50] = v6 + 8;
      *(uint16_t *)(v5 - 8) = *(uint16_t *)(v9 + 6);
      *(uint16_t *)(_this->f12[46] - 6) = *(uint16_t *)(_this->f12[47] + 4);
      *(uint16_t *)(_this->f12[46] - 4) = *(uint16_t *)(_this->f12[47] + 2);
      *(uint16_t *)(_this->f12[46] - 2) = **(uint16_t **)&_this->f12[47];
      v10 = *(uint8_t **)&_this->f12[48];
      v11 = *(uint8_t **)&_this->f12[50];
      _this->f12[2] = 0;
      _this->f12[3] = 0;
      _this->f12[4] = 0;
      v12 = *(v10 - 3);
      _this->f12[3] = v12;
      v13 = *(v10 - 1);
      _this->f12[4] = v13;
      v14 = *(v11 - 3) + v12;
      _this->f12[3] = v14;
      v15 = *(v11 - 1) + v13;
      _this->f12[4] = v15;
      v16 = v10[1] + v14;
      _this->f12[3] = v16;
      v17 = v10[3] + v15;
      _this->f12[4] = v17;
      v18 = v11[1] + v16;
      _this->f12[3] = v18;
      v19 = v11[3] + v17;
      _this->f12[4] = v19;
      v20 = v10[5] + v18;
      _this->f12[3] = v20;
      v21 = v10[7] + v19;
      _this->f12[4] = v21;
      v22 = v11[5] + v20;
      _this->f12[3] = v22;
      v23 = v11[7] + v21;
      _this->f12[4] = v23;
      v24 = v10[9] + v22;
      _this->f12[3] = v24;
      v25 = v10[11] + v23;
      _this->f12[4] = v25;
      v26 = v11[9] + v24;
      v27 = _this->f12[46];
      _this->f12[3] = v26;
      v28 = v11[11] + v25;
      _this->f12[4] = v28;
      v29 = *(uint8_t *)(v27 - 7) + v26;
      _this->f12[3] = v29;
      v30 = *(uint8_t *)(v27 - 5) + v28;
      _this->f12[4] = v30;
      _this->f12[3] = *(uint8_t *)(v27 - 3) + v29;
      v31 = _this->f0[0] <= 0;
      _this->f12[4] = *(uint8_t *)(v27 - 1) + v30;
      if ( !v31 )
      {
        v32 = a3;
        v40 = v4;
        v42 = 0;
        do
        {
          ++v42;
          __alt_p1_context((AltP1Block *)(uint8_t **)_this, (uint32_t *)nullptr, (uint32_t *)0);
          v33 = *(uint8_t *)&_this->f8;
          v34 = (uint8_t)(*a2 - v33);
          v35 = *(_this->f984[v34] + _this->f1496) + v33;
          n5 = _this->f984[v34];
          n16 = (uint8_t)*v32 - (uint8_t)(v35 + *v32 - *a2);
          if ( n16 < -16 || n16 > 16 )
          {
            *v32 = *a2;
            n5 = _this->f1240[v34];
          }
          else
          {
            *v32 = v35;
          }
          __alt_p1_encode_symbol(&((P1Count *)_this)[_this->f12[0] + 237].total, 16 * _this->f12[0], _this->f12[1], n5);
          v38 = (uint8_t)*v32;
          v39 = v38 - _this->f8;
          **(uint8_t **)&_this->f12[46] = v38;
          *(uint8_t *)(_this->f12[46] + 1) = (BYTE4(v39) ^ v39) - BYTE4(v39);
          _this->f12[3 + _this->f12[2]] = _this->f12[3 + _this->f12[2]]
                                                              + *(uint8_t *)(_this->f12[46] + 1)
                                                              - *(uint8_t *)(_this->f12[46] - 7)
                                                              - (*(uint8_t *)(_this->f12[50] - 3)
                                                               - *(uint8_t *)(_this->f12[50] + 13)
                                                               + *(uint8_t *)(_this->f12[48] - 3)
                                                               - *(uint8_t *)(_this->f12[48] + 13));
                  _this->f12[2] = _this->f12[2] == 0;
          if ( ((P1Count *)_this)[_this->f12[0] + 237].total < 0x4000u )
            __alt_p1_model(_this);
          _this->f12[46] += 2;
          ++v32;
          _this->f12[47] += 2;
          _this->f12[48] += 2;
          _this->f12[49] += 2;
          _this->f12[50] += 2;
          ++a2;
        }
        while ( v42 < _this->f0[0] );
        v4 = v40;
        a3 = v32;
      }
    }
    while ( v4 < *(uint32_t *)&_this->f4 );
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
  __alt_p1_d8_encode_body((AltP1Block *)v5, (uint8_t *)a1, (uint8_t *)a4);
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
  ;
  int32_t v7, v8, v9, v13, Size, v17, v18, v20, v21, v22, v23, v24, v26, v27, v28, v29;
  uint32_t j, k, n0x1E60, m_1, m, n5, n0x82, n;
  void *v10;
  _this->f278728 = n4;
  // Two records a step, which is how MSVC unrolled the seed of all 163 840.
  for ( j = 0; j < 0x14000; ++j )
  {
    _this->f284712[2 * j].w2 = 0;
    _this->f284712[2 * j + 1].w2 = 0;
  }
  for ( k = 0; k < 0x14000; ++k )
  {
    _this->f284712[2 * k].b0 = 5;
    _this->f284712[2 * k].b1 = 2;
    _this->f284712[2 * k + 1].b0 = 5;
    _this->f284712[2 * k + 1].b1 = 2;
  }
  n0x1E60 = 0;
  do
  {
    v7 = 8 * n0x1E60;   // two four-lane groups a step
    _this->f940072[v7 + 1] = 2048;
    ++n0x1E60;
    _this->f940072[v7 + 2] = 2816;
    _this->f940072[v7 + 3] = 2816;
    _this->f940072[v7] = 4096;
    _this->f940072[v7 + 5] = 2048;
    _this->f940072[v7 + 6] = 2816;
    _this->f940072[v7 + 7] = 2816;
    _this->f940072[v7 + 4] = 4096;
  }
  while ( n0x1E60 < 0x1E60 );
  v8 = 4 * plane_desc[0].w12 + 1;
  v9 = 16 * plane_desc[0].w12;
  *(uint32_t *)&_this->f278732 = (uint8_t)(plane_desc[plane_desc[_this->f278728 + 1].src_plane + 1].flags
                                               & 8) >> 3;
  deadzone_hi = v8;
  deadzone_lo = -v8;
  *(uint32_t *)&_this->f278720 = -v9 - 7;
  *(uint32_t *)&_this->f278724 = v9 + 8;
  *(uint8_t **)&_this->f278660 = (uint8_t *)bmf_new(4 * i + 16);
  v10 = bmf_new(4 * i + 16);
  *(uint32_t *)((uint8_t *)_this + 232) = 0x3F800000 /* 1.0f */;
  *&_this->f278664 = (uint8_t *)v10;
  *&_this->f278668 = *(uint8_t **)&_this->f278660 + 4 * i + 8;
  if ( i > -4 )
  {
    m_1 = (i + 4) / 2;
    if ( m_1 )
    {
      for ( m = 0; m < m_1; ++m )
      {
        *(uint8_t **)(*&_this->f278664 + 8 * m) = (uint8_t *)_this;
        *(uint8_t **)(*(uint8_t **)&_this->f278660 + 8 * m) = (uint8_t *)_this;
        *(uint8_t **)(*&_this->f278664 + 8 * m + 4) = (uint8_t *)_this;
        *(uint8_t **)(*(uint8_t **)&_this->f278660 + 8 * m + 4) = (uint8_t *)_this;
      }
      v13 = 2 * m + 1;
    }
    else
    {
      v13 = 1;
    }
    if ( i + 4 > (uint32_t)(v13 - 1) )
    {
      *(uint8_t **)(*&_this->f278664 + 4 * v13 - 4) = (uint8_t *)_this;
      *(uint8_t **)(*(uint8_t **)&_this->f278660 + 4 * v13 - 4) = (uint8_t *)_this;
    }
  }
  n5 = 0;
  Size = 18 * i + 234;
  do
    _this->f278736[5 + n5++] = (uint8_t *)bmf_new(Size);
  while ( n5 < 5 );
  memset(_this->f278756,0,Size);
  v17 = *(uint32_t *)&_this->f278736[5];
  ctx_bias[3] = 0;
  v18 = 0;
  ctx_bias[2] = 0;
  ctx_bias[1] = 0;
  n0x82 = 0;
  ctx_bias[0] = 0;
  *(uint32_t *)&_this->f278736[0] = v17 + 144;
  do
  {
    v20 = p2_ctx_edges[v18];
    *(uint32_t *)&_this->f278944[2 * n0x82] = (_this->f278728 << 8) | (16 * v18);
    v21 = (2 * n0x82 == v20) + v18;
    v22 = p2_ctx_edges[v21];
    *(uint32_t *)&_this->f278944[2 * n0x82 + 1] = (_this->f278728 << 8) | (16 * v21);
    v23 = 2 * n0x82++ + 1 == v22;
    v18 = v23 + v21;
  }
  while ( n0x82 < 0x82 );
  v24 = 0;
  for ( n = 0; n < 0x3C; ++n )
  {
    v26 = p2_len_edges[v24];
    _this->f280752[2 * n] = v24;
    v27 = (2 * n == v26) + v24;
    v28 = p2_len_edges[v27];
    _this->f280752[2 * n + 1] = v27;
    v29 = 2 * n + 1 == v28;
    v24 = v29 + v27;
  }
  _this->f278704 = 15;
  __alt_init_tables(_this->f279984, _this->f280496);
  _this->f278760[21] = 0;
  _this->f278760[6] = 64;
  _this->f278760[17] = 0;
  _this->f278760[7] = 128;
  _this->f278760[13] = 0;
  _this->f278760[10] = 192;
  _this->f278760[9] = 0;
  _this->f278760[11] = 384;
  _this->f278760[5] = 0;
  _this->f278760[14] = 576;
  _this->f278760[15] = 1152;
  _this->f278760[18] = 1728;
  _this->f278760[19] = 3456;
  _this->f278760[22] = 5184;
  _this->f278760[23] = 10368;
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
};                            // +16  pixels
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
    if ( (*((uint8_t *)result + 10) & 0x80) != 0 )
      buf = (uint8_t *)result + result[3] + 16;
    else
      buf = nullptr;
    v15 = (int32_t *)(result);
    memset(buf,0,Size);
    return (int32_t *)v15;
  }
  return (int32_t *)result;
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
  FILE *Stream_v;
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
  uint8_t v42, v43, v44, v45, v46, v47, v56;
  uint8_t *Bufferc_3, *Bufferc_1, *Bufferb_1, *v19, *v24, *buf_1, *v31, *v39, *buf_3, *Bufferc_2;   // `uint8_t *` beside the `char` scalars above
  int32_t v3, i, Buffer_1, n8, v14, v16, v17, v20, v21, v22, v23, v25, v26, v28, n4, n2,
          Buffer_3, Buffer_4, Size, v40, n2_1, v49, Size_1, v55;
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
          v26 = (uint8_t)v24[3 * v63 + 15];
          Bufferb_1 = Bufferd;
          *(uint32_t *)&Bufferc_1[4 * v63 + 50] = (v26 << 16) | v25;
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
  Stream_v = Stream_1;
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
              if ( !Size )
                goto LABEL_70;
              if ( v75 )
              {
                if ( Size != 1 )
                {
LABEL_67:
                  *buf_2 = 0;
                  v72 = (uint8_t *)buf_2 + 2;
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
                v47 = *(v31 - 1);
                *buf_2 = 2;
                buf_2[1] = v47;
                buf_2 += 2;
              }
              else
              {
                if ( Size >= 3 )
                  goto LABEL_67;
                if ( Size == 2 )
                {
                  v45 = *(v31 - 2);
                  *buf_2 = 1;
                  buf_2[1] = v45;
                  buf_2 += 2;
                }
                v46 = *(v31 - 1);
                *buf_2 = 1;
                buf_2[1] = v46;
                buf_2 += 2;
              }
LABEL_69:
              Size = 0;
              LOBYTE(v40) = *v31;
LABEL_70:
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
                v44 = *(v39 - 1);
                *buf_2 = 2;
                buf_2[1] = v44;
                buf_2 += 2;
                goto LABEL_55;
              }
            }
            else if ( Size < 3 )
            {
              if ( Size == 2 )
              {
                v42 = *(v39 - 2);
                *buf_2 = 1;
                buf_2[1] = v42;
                buf_2 += 2;
              }
              v43 = *(v39 - 1);
              *buf_2 = 1;
              buf_2[1] = v43;
              buf_2 += 2;
              goto LABEL_55;
            }
            *buf_2 = 0;
            v68 = (uint8_t *)buf_2 + 2;
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
          v56 = *(v39 - 2);
          *buf_2 = 1;
          buf_2[1] = v56;
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
      Bufferc_2 = Bufferc;
      Stream_2 = Stream_v;
      goto LABEL_76;
    }
    v28 = 0;
  }
  Stream_2 = Stream_v;
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
    Stream_2 = Stream_v;
    v49 = buf_1 - buf;
  }
LABEL_76:
  bmp->biSizeImage = v49;
  ElementCount = buf_1 - Bufferc_2;
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
    a1->f8 = 0;
    a1->live = v7;
    result = 12 * v7;
    a1->f12 = 12 * v7;
    a1->f16 = 8 * v7;
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
    a1->f8 = 2;
    a1->f16 = 20 * v10;
    a1->live = 0;
    a1->f12 = 18 * v10;
    return (uint32_t)memset(buf,0,3 * v10);
  }
  return result;
}

uint8_t * __interleave_plane(uint8_t *p_i, uint8_t *Src, int32_t a3, int8_t a4)
{
  ;
  uint8_t *Src_5, *v25;
  int32_t v4, n6_2, v8, n6_1, v10, Size, n4, Size_2, v15, Size_1, v17, v18, v20, v21, v23, n2,
          v26, v27, v29, v30, v31, v32, n4_1, v34, n6;
  uint32_t v6, Src_1, Src_2;
  uint8_t *Src_4, *Src_3;
  if ( (plane_desc[a3 + 1].flags & 8) == 0 )
  {
    Size = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
    n4 = plane_count;
    Src_1 = (uint32_t)&p_i[a3 + 16];
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(&p_i[a3 + 16],Src,Size);
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
  n6 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
  v29 = plane_desc[1].src_plane - a3;
  v31 = plane_desc[2].src_plane - a3;
  n2 = plane_desc[a3 + 1].predictor;
  v34 = plane_desc[a3 + 1].b3;
  Src_2 = (uint32_t)&p_i[a3 + 16];
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
        v21 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
        v27 = v4;
        Src_3 = (uint8_t *)Src_2;
        do
        {
          v23 = v34 + (uint8_t)*Src++;
          *Src_3 = (uint8_t)(((v32 * (uint8_t)Src_3[v29]
                               + v27 * (uint32_t)(uint8_t)Src_3[v31] + 40) >> 7)
                             + v23);
          Src_3 += n4_1;
          --v21;
        }
        while ( v21 );
      }
      else if ( n2 == 3 )
      {
        v18 = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
        v26 = v4;
        Src_4 = (uint8_t *)Src_2;
        do
        {
          v20 = v34 + (uint8_t)*Src++;
          *Src_4 = (uint8_t)(((v26 * (uint8_t)*(Src_4 - 2)
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
  int32_t v4, n6_2, v7, n6_1, v10, Size, n4, Size_1, v16, Size_2, v18, v19, v21, v22, v23, v24,
          v26, n2, v30, v31, v33, v34, v35, v36, v37, n4_1, n6;
  uint32_t Src_2, Src_1, v27, Src_3;
  uint8_t *Src_5, *Src_4;
  if ( (plane_desc[a3 + 1].flags & 8) == 0 )
  {
    Size = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    n4 = plane_count;
    Src_1 = (uint32_t)&Blockb[a3 + 16];
    if ( plane_count == 1 )
      return (uint8_t *)memcpy(Src,&Blockb[a3 + 16],Size);
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
  n6 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
  v33 = plane_desc[1].src_plane - a3;
  v35 = plane_desc[2].src_plane - a3;
  n2 = plane_desc[a3 + 1].predictor;
  v37 = plane_desc[a3 + 1].b3;
  Src_3 = (uint32_t)&Blockb[a3 + 16];
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
    v24 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    v31 = v4;
    Src_4 = (uint8_t *)Src_3;
    do
    {
      v26 = *Src_4 - v37;
      v27 = v36 * Src_4[v33] + v31 * Src_4[v35] + 40;
      Src_4 += n4_1;
      *Src++ = (uint8_t)(v26 - (v27 >> 7));
      --v24;
    }
    while ( v24 );
  }
  else if ( n2 == 3 )
  {
    v19 = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    v30 = v4;
    Src_5 = (uint8_t *)Src_3;
    do
    {
      v21 = *Src_5 - v37;
      v22 = v30 * *(Src_5 - 2) + v36 * *(Src_5 - 3);
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
    model_geometry[0] = 0x02020100;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    model_geometry[1] = 0x03030303;
    level_geom[4].half = 4;
    v7 = 2 * level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    *(uint64_t *)((uint8_t *)model_geometry + v7) = 0x404040404040404LL;
    level_geom[5].half = 8;
    v8 = v7 + 2 * level_geom[4].half;
    level_geom[5].first = v8;
    level_geom[5].tbl_base = v8 - 5;
    memset((uint8_t *)model_geometry + v8, 0x05, 16);
    level_geom[6].half = 16;
    v9 = v8 + 2 * level_geom[5].half;
    level_geom[6].first = (uint8_t)v9;
    level_geom[6].tbl_base = (uint8_t)v9 - 6;
    memset((uint8_t *)model_geometry + v9, 0x06, 32);
    level_geom[7].half = 32;
    v11 = v9 + 2 * level_geom[6].half;
    level_geom[7].first = (uint8_t)v11;
    level_geom[7].tbl_base = (uint8_t)v11 - 7;
    memset((uint8_t *)model_geometry + v11, 0x07, 64);
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

uint8_t *__unpredict_med(uint8_t *Src, int32_t i, int32_t a3)
{
  ;
  uintptr_t Src_1, v41;   // were int32_t: addresses, masked and tagged
  uint8_t v39, v40;
  int32_t i_1, v42, v45, v46, v47, v48;
  uint32_t j, v36, m_1, m, v44, v50;
  uint8_t *result, *v43;
  alignas(16) uint8_t v52[255];
  Src_1 = (uintptr_t)Src;
  result = (Src + 1);
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
  v52[0] = 0;
  for ( j = 0; j < 127; ++j )
  {
    v52[2 * j + 1] = (uint8_t)(-1 - (int32_t)j);
    v52[2 * j + 2] = (uint8_t)(1 + j);
  }
  Src_1 = (uintptr_t)Src;
  // never taken: -E is 0
  i_1 = i;
  if ( i == 1 )
  {
    v42 = a3 - 1;
    if ( a3 == 1 )
      return result;
    v36 = 0;
    goto LABEL_29;
  }
  v36 = i - 1;
  m_1 = (i - 1) / 2;
  if ( m_1 )
  {
    for ( m = 0; m < m_1; ++m )
    {
      v39 = *(uint8_t *)(Src_1 + 2 * m) + v52[*(uint8_t *)(Src_1 + 2 * m + 1)];
      v40 = v52[*(uint8_t *)(Src_1 + 2 * m + 2)];
      *(uint8_t *)(Src_1 + 2 * m + 1) = v39;
      *(uint8_t *)(Src_1 + 2 * m + 2) = v39 + v40;
      result = (uint8_t *)(Src_1 + 2 * m + 3);
    }
    i_1 = i;
    v41 = 2 * m + 1;
  }
  else
  {
    v41 = 1;
  }
  if ( v36 > v41 - 1 )
  {
    *(uint8_t *)(v41 + Src_1) = *(uint8_t *)(v41 + Src_1 - 1) + v52[*(uint8_t *)(v41 + Src_1)];
    result = (uint8_t *)(Src_1 + v41 + 1);
    v42 = a3 - 1;
    if ( a3 == 1 )
      return result;
    goto LABEL_29;
  }
  v42 = a3 - 1;
  if ( a3 != 1 )
  {
LABEL_29:
    v50 = v36;
    v43 = &result[-i_1];
    do
    {
      while ( 1 )
      {
        ++v43;
        *result = v52[(uint8_t)*result] + result[-i_1];
        v44 = v50;
        ++result;
        if ( i_1 != 1 )
          break;
        if ( !--v42 )
          return result;
      }
      v48 = v42;
      do
      {
        v45 = (uint8_t)*(result - 1);
        v46 = (uint8_t)*v43;
        v47 = (uint8_t)result[-i - 1];
        if ( v45 < v46 )
        {
          if ( v47 < v45 )
          {
            LOBYTE(v45) = *v43;
            goto LABEL_41;
          }
          if ( v47 <= v46 )
LABEL_40:
            LOBYTE(v45) = v46 + v45 - v47;
        }
        else
        {
          if ( v47 > v45 )
          {
            LOBYTE(v45) = *v43;
            goto LABEL_41;
          }
          if ( v47 >= v46 )
            goto LABEL_40;
        }
LABEL_41:
        *result = v45 + v52[(uint8_t)*result];
        ++v43;
        ++result;
        --v44;
      }
      while ( v44 );
      i_1 = i;
      v42 = v48 - 1;
    }
    while ( v48 != 1 );
  }
  return result;
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
  ;
  uint8_t *v7, *v8, *v9, *v10, *v11;   // the five row cursors of f176
  bool v33;
  AltP1Block *v5;
  AltP1Block *v4;
  int32_t v6, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26,
          v27, v28, v29, v30, v31, v32, v35, v36, v39, v41;
  int64_t v37;
  uint8_t *v12, *v13, *Src_1;
  v4 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
  if ( v4 )
    v5 = (AltP1Block *)(__alt_p1_alloc((AltP1Block *)v4, i, a4, 0));
  else
    v5 = (AltP1Block *)(nullptr);
  __rc_begin_decode(ArgList);
  if ( v5->f0[1] > 0 )
  {
    v6 = 0;
    do
    {
      ++v6;
      *(uint16_t *)v5->f176[5] = *(uint16_t *)(v5->f176[5] - 2);
      *(uint16_t *)(v5->f176[5] + 2) = *(uint16_t *)(v5->f176[5] - 4);
      *(uint16_t *)(v5->f176[5] + 4) = *(uint16_t *)(v5->f176[5] - 6);
      *(uint16_t *)(v5->f176[5] + 6) = *(uint16_t *)(v5->f176[5] - 8);
      *(uint16_t *)(v5->f176[5] + 8) = *(uint16_t *)(v5->f176[5] - 10);
      *(uint16_t *)(v5->f176[5] + 10) = *(uint16_t *)(v5->f176[5] - 12);
      v7 = v5->f176[4];
      v8 = v5->f176[3];
      v9 = v5->f176[2];
      v10 = v5->f176[1];
      v11 = v5->f176[0];
      v5->f176[4] = v8;
      v5->f176[3] = v9;
      v5->f176[2] = v10;
      v5->f176[1] = v11;
      v5->f176[0] = v7;
      v7 += 8;
      v5->f176[5] = v7;
      v11 += 8;
      v5->f176[6] = v11;
      v5->f176[7] = v10 + 8;
      v5->f176[8] = v9 + 8;
      v5->f176[9] = v8 + 8;
      *(uint16_t *)(v7 - 8) = *(uint16_t *)(v11 + 6);
      *(uint16_t *)(v5->f176[5] - 6) = *(uint16_t *)(v5->f176[6] + 4);
      *(uint16_t *)(v5->f176[5] - 4) = *(uint16_t *)(v5->f176[6] + 2);
      *(uint16_t *)(v5->f176[5] - 2) = *(uint16_t *)v5->f176[6];
      v12 = (uint8_t *)v5->f176[7];
      v13 = (uint8_t *)v5->f176[9];
      v5->f0[5] = 0;
      v5->f0[6] = 0;
      v5->f0[7] = 0;
      v14 = *(v12 - 3);
      v5->f0[6] = v14;
      v15 = *(v12 - 1);
      v5->f0[7] = v15;
      v16 = *(v13 - 3) + v14;
      v5->f0[6] = v16;
      v17 = *(v13 - 1) + v15;
      v5->f0[7] = v17;
      v18 = v12[1] + v16;
      v5->f0[6] = v18;
      v19 = v12[3] + v17;
      v5->f0[7] = v19;
      v20 = v13[1] + v18;
      v5->f0[6] = v20;
      v21 = v13[3] + v19;
      v5->f0[7] = v21;
      v22 = v12[5] + v20;
      v5->f0[6] = v22;
      v23 = v12[7] + v21;
      v5->f0[7] = v23;
      v24 = v13[5] + v22;
      v5->f0[6] = v24;
      v25 = v13[7] + v23;
      v5->f0[7] = v25;
      v26 = v12[9] + v24;
      v5->f0[6] = v26;
      v27 = v12[11] + v25;
      v5->f0[7] = v27;
      v28 = v13[9] + v26;
      v29 = (int32_t)(uintptr_t)v5->f176[5];
      v5->f0[6] = v28;
      v30 = v13[11] + v27;
      v5->f0[7] = v30;
      v31 = *(uint8_t *)(v29 - 7) + v28;
      v5->f0[6] = v31;
      v32 = *(uint8_t *)(v29 - 5) + v30;
      v5->f0[7] = v32;
      v5->f0[6] = *(uint8_t *)(v29 - 3) + v31;
      v33 = v5->f0[0] <= 0;
      v5->f0[7] = *(uint8_t *)(v29 - 1) + v32;
      if ( !v33 )
      {
        Src_1 = (uint8_t *)Src;
        v39 = v6;
        v41 = 0;
        do
        {
          ++v41;
          __alt_p1_context((AltP1Block *)(uint32_t *)v5, (uint32_t *)nullptr, (uint32_t *)0);
          v36 = (uint8_t)(*((uint8_t *)v5 + 8)
                                + *((uint8_t *)v5 + (uint8_t)__alt_p1_decode_symbol((uint16_t *)&((int32_t *)v5)[4 * v5->f0[3] + 950], v35, v5->f0[4]) + 1496));
          *Src_1 = v36;
          v37 = v36 - v5->f0[2];
          *(uint8_t *)v5->f176[5] = v36;
          *(v5->f176[5] + 1) = (BYTE4(v37) ^ v37) - BYTE4(v37);
          ((int32_t *)v5)[v5->f0[5] + 6] = ((int32_t *)v5)[v5->f0[5] + 6]
                        + *(v5->f176[5] + 1)
                        - *(v5->f176[5] - 7)
                        - (*(v5->f176[9] - 3)
                         - *(v5->f176[9] + 13)
                         + *(v5->f176[7] - 3)
                         - *(v5->f176[7] + 13));
            v5->f0[5] = v5->f0[5] == 0;
          if ( ((P1Count *)v5)[v5->f0[3] + 237].total < 0x4000u )
            __alt_p1_model((AltP1Block *)v5);
          v5->f176[5] += 2;
          ++Src_1;
          v5->f176[6] += 2;
          v5->f176[7] += 2;
          v5->f176[8] += 2;
          v5->f176[9] += 2;
        }
        while ( v41 < v5->f0[0] );
        v6 = v39;
        Src = Src_1;
      }
    }
    while ( v6 < v5->f0[1] );
  }
  __rc_end_decode();
  return __alt_p1_free((void **)v5, 1);
}


int32_t __alt_model_p1_decode(uint16_t *p_i, uint8_t *Src)
{
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
  AltP1Block *v83;
  void *v84;
  int32_t v85;
  uint32_t v86;
  uint32_t v87;
  int32_t v88;
  int32_t v89;
  uint8_t *Src_1;
  int32_t v91;
  int32_t v92;
  void * Block_plane[4];
  AltP1Block * &v94 = (AltP1Block * &)Block_plane[1];
  AltP1Block * &v95 = (AltP1Block * &)Block_plane[2];
  AltP1Block * &v96 = (AltP1Block * &)Block_plane[3];
  int32_t v97;
  int32_t v98;
  uint32_t i_1;
  int32_t ArgList_1;
  int32_t v101;
  uint32_t i_4;
  int32_t v103;
  int32_t v104;
  int32_t v105;
  ;
  AltP1Block *v59;
  uintptr_t v61;
  AltP1Block *v24;
  uint8_t *v29;
  uint8_t v11, v12, v13, v58;
  AltP1Block *v6;
  uint8_t *v25, *v26, *v27, *v28;   // row cursors out of AltP1Block
  int32_t i, v3, i_2, n4, *v7, v8, v9, v10, v14, v15, v16, ArgList, v18, i_3, n4_1, n4_2,
          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45,
          v46, v47, v48, v49, v50, v52, v53, v54, v56, v57, v60, v62, v64, v67, v68, v71, v72,
          v74, v75, v78, v79, n4_3, n4_4;
  uint32_t v20, *v51;
  AltP1Block *v66;
  AltP1Block *v73;
  uint8_t *v30;
  uint8_t *v31;
  uint8_t *v55, *v63, *v69, *v70, *v76, *v77;
  void **v82;
  i = *p_i;
  v3 = p_i[1];
  i_1 = i;
  if ( plane_count > 0 )
  {
    i_2 = i;
    n4 = 0;
    do
    {
      v6 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __alt_p1_alloc((AltP1Block *)v6, i_2, v3, n4);
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
  v11 = plane_desc[v8 + 1].flags;
  v12 = plane_desc[v9 + 1].flags;
  v13 = plane_desc[v10 + 1].flags;
  v14 = plane_desc[v9 + 1].b3;
  v15 = plane_desc[v10 + 1].b3;
  v97 = plane_desc[v8 + 1].b3;
  v98 = v14;
  v16 = v11 & 8;
  v85 = v15;
  ArgList = v12 & 8;
  v18 = v13 & 8;
  __rc_begin_decode(ArgList);
  if ( v3 > 0 )
  {
    v91 = v18;
    v20 = 0;
    ArgList_1 = ArgList;
    i_3 = i_1;
    v101 = v16;
    n4_1 = plane_count;
    do
    {
      if ( n4_1 > 0 )
      {
        v86 = v20;
        n4_2 = 0;
        v88 = v3;
        Src_1 = (uint8_t *)Src;
        do
        {
          ++n4_2;
          // `&v92 + n` is `Block_plane[n - 1]`: v92 is the member
          // before the array, and this loop pre-increments from 0.
          v24 = (AltP1Block *)Block_plane[n4_2 - 1];
          **(uint16_t **)&v24->cur[0] = *(uint16_t *)(v24->cur[0] - 2);
          *(uint16_t *)(v24->cur[0] + 2) = *(uint16_t *)(v24->cur[0] - 4);
          *(uint16_t *)(v24->cur[0] + 4) = *(uint16_t *)(v24->cur[0] - 6);
          *(uint16_t *)(v24->cur[0] + 6) = *(uint16_t *)(v24->cur[0] - 8);
          *(uint16_t *)(v24->cur[0] + 8) = *(uint16_t *)(v24->cur[0] - 10);
          *(uint16_t *)(v24->cur[0] + 10) = *(uint16_t *)(v24->cur[0] - 12);
          v25 = v24->f176[4];
          v26 = v24->f176[3];
          v27 = v24->f176[2];
          v28 = v24->f176[1];
          v29 = v24->f176[0];
          v24->f176[4] = v26;
          v24->f176[3] = v27;
          v24->f176[2] = v28;
          v24->f176[1] = v29;
          v24->f176[0] = v25;
          v25 += 8;
          v24->cur[0] = v25;
          v29 += 8;
          v24->cur[1] = v29;
          v24->cur[2] = v28 + 8;
          v24->cur[3] = v27 + 8;
          v24->cur[4] = v26 + 8;
          *(uint16_t *)(v25 - 8) = *(uint16_t *)(v29 + 6);
          *(uint16_t *)(v24->cur[0] - 6) = *(uint16_t *)(v24->cur[1] + 4);
          *(uint16_t *)(v24->cur[0] - 4) = *(uint16_t *)(v24->cur[1] + 2);
          *(uint16_t *)(v24->cur[0] - 2) = **(uint16_t **)&v24->cur[1];
          v30 = (uint8_t *)(*&v24->cur[2]);
          v31 = (uint8_t *)(*&v24->cur[4]);
          v24->f12[2] = 0;
          v24->f12[3] = 0;
          v24->f12[4] = 0;
          v32 = *((int8_t *)v30 - 3);
          v24->f12[3] = v32;
          v33 = *((int8_t *)v30 - 1);
          v24->f12[4] = v33;
          v34 = *((int8_t *)v31 - 3) + v32;
          v24->f12[3] = v34;
          v35 = *((int8_t *)v31 - 1) + v33;
          v24->f12[4] = v35;
          v36 = v30[1] + v34;
          v24->f12[3] = v36;
          v37 = v30[3] + v35;
          v24->f12[4] = v37;
          v38 = v31[1] + v36;
          v24->f12[3] = v38;
          v39 = v31[3] + v37;
          v24->f12[4] = v39;
          v40 = v30[5] + v38;
          v24->f12[3] = v40;
          v41 = v30[7] + v39;
          v24->f12[4] = v41;
          v42 = v31[5] + v40;
          v24->f12[3] = v42;
          v43 = v31[7] + v41;
          v24->f12[4] = v43;
          v44 = v30[9] + v42;
          v24->f12[3] = v44;
          v45 = v30[11] + v43;
          v24->f12[4] = v45;
          v46 = v31[9] + v44;
          v47 = (int32_t)(uintptr_t)v24->cur[0];
          v24->f12[3] = v46;
          v48 = v31[11] + v45;
          v24->f12[4] = v48;
          v49 = *(uint8_t *)(v47 - 7) + v46;
          v24->f12[3] = v49;
          v50 = *(uint8_t *)(v47 - 5) + v48;
          v24->f12[4] = v50;
          v24->f12[3] = *(uint8_t *)(v47 - 3) + v49;
          v24->f12[4] = *(uint8_t *)(v47 - 1) + v50;
          n4_1 = plane_count;
        }
        while ( n4_2 < plane_count );
        v20 = v86;
        v3 = v88;
        i_3 = i_1;
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
          __alt_p1_context((AltP1Block *)(uint8_t **)Block_plane[0], (uint32_t *)nullptr, (uint32_t *)0);
          v53 = __alt_p1_decode_symbol((uint16_t *)&v51[4 * v51[3] + 950], v52, v51[4]);
          v54 = v51[2];
          v55 = (uint8_t *)v51[49];
          v56 = (uint8_t)(v54 + *((uint8_t *)v51 + v53 + 1496));
          v103 = v56;
          *v55 = v56;
          *(uint8_t *)(v51[49] + 1) = abs32(v56 - v54);
          v51[v51[5] + 6] = v51[v51[5] + 6]
                          + *(uint8_t *)(v51[49] + 1)
                          - *(uint8_t *)(v51[49] - 7)
                          - (*(uint8_t *)(v51[53] - 3)
                           - *(uint8_t *)(v51[53] + 13)
                           + *(uint8_t *)(v51[51] - 3)
                           - *(uint8_t *)(v51[51] + 13));
          v57 = 4 * v51[3];
          v51[5] = v51[5] == 0;
          if ( LOWORD(v51[v57 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v51);
          v58 = v103;
          v51[49] += 2;
          v51[50] += 2;
          v51[51] += 2;
          v51[52] += 2;
          v51[53] += 2;
          v59 = (AltP1Block *)v94;
          *(plane_desc[1].src_plane + Src) = v58;
          __alt_p1_context((AltP1Block *)(uint8_t **)v59, (uint32_t *)Block_plane[0], (uint32_t *)0);
          v61 = __alt_p1_decode_symbol(&((P1Count *)v59)[v59->f12[0] + 237].total, v60, v59->f12[1]);
          v62 = *(uint32_t *)&v59->f8;
          v63 = *(uint8_t **)&v59->f12[46];
          v64 = (uint8_t)(v62 + v59->f1496[v61]);
          v104 = v64;
          *v63 = v64;
          *(uint8_t *)(v59->f12[46] + 1) = abs32(v64 - v62);
          v59->f12[3 + v59->f12[2]] = v59->f12[3 + v59->f12[2]]
                                                            + *(uint8_t *)(v59->f12[46] + 1)
                                                            - *(uint8_t *)(v59->f12[46] - 7)
                                                            - (*(uint8_t *)(v59->f12[50] - 3)
                                                             - *(uint8_t *)(v59->f12[50] + 13)
                                                             + *(uint8_t *)(v59->f12[48] - 3)
                                                             - *(uint8_t *)(v59->f12[48] + 13));
          v59->f12[2] = v59->f12[2] == 0;
          if ( ((P1Count *)v59)[v59->f12[0] + 237].total < 0x4000u )
            __alt_p1_model(v59);
          v59->f12[46] += 2;
          v59->f12[47] += 2;
          v59->f12[48] += 2;
          v59->f12[49] += 2;
          v59->f12[50] += 2;
          if ( v101 )
            v104 += v97 + *(plane_desc[1].src_plane + Src);
          v66 = (AltP1Block *)(v95);
          v84 = Block_plane[0];
          v83 = v94;
          *(Src + plane_desc[2].src_plane) = v104;
          __alt_p1_context((AltP1Block *)v66, (uint32_t *)v83, (uint32_t *)(int32_t)v84);
          v68 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)v66)[4 * v66->f12[0] + 950], v67, (int32_t)v66->f12[1]);
          v69 = (uint8_t *)(v66->f8);
          v70 = v66->cur[0];
          v71 = (uint8_t)((uint8_t)(uintptr_t)v69 + v66->f1496[v68]);
          v105 = v71;
          *v70 = v71;
          v66->cur[0][1] = abs32(v71 - (uint32_t)v69);
          ((uint8_t**)v66)[v66->f12[2] + 6] = &((uint8_t**)v66)[v66->f12[2] + 6][v66->cur[0][1]
                                                           - *(v66->cur[0] - 7)
                                                           - (*(v66->f212 - 3)
                                                            - v66->f212[13])
                                                           - (*(v66->f204 - 3)
                                                            - v66->f204[13])];
          v72 = 4 * v66->f12[0];
          v66->f12[2] = v66->f12[2] == 0;
          if ( LOWORD(((uint8_t**)v66)[v72 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v66);
          v66->cur[0] += 2;
          v66->f200 += 2;
          v66->f204 += 2;
          v66->f208 += 2;
          v66->f212 += 2;
          if ( ArgList_1 )
            *(plane_desc[3].src_plane + Src) = ((plane_desc[plane_desc[3].src_plane + 1].w4
                                                                * *(plane_desc[1].src_plane
                                                                                     + Src)
                                                                + plane_desc[plane_desc[3].src_plane + 1].w8
                                                                * (uint32_t)*(plane_desc[2].src_plane + Src)
                                                                + 40) >> 7)
                                                              + v98
                                                              + v105;
          else
            *(plane_desc[3].src_plane + Src) = v105;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            v73 = (AltP1Block *)(v96);
            __alt_p1_context((AltP1Block *)v96, (uint32_t *)v95, (uint32_t *)(int32_t)v94);
            v75 = __alt_p1_decode_symbol((uint16_t *)&((uint8_t**)v73)[4 * v73->f12[0] + 950], v74, (int32_t)v73->f12[1]);
            v76 = (uint8_t *)(v73->f8);
            v77 = v73->cur[0];
            v78 = (uint8_t)((uint8_t)(uintptr_t)v76 + v73->f1496[v75]);
            v92 = v78;
            *v77 = v78;
            v73->cur[0][1] = abs32(v78 - (uint32_t)v76);
            ((uint8_t**)v73)[v73->f12[2] + 6] = &((uint8_t**)v73)[v73->f12[2] + 6][v73->cur[0][1]
                                                             - *(v73->cur[0] - 7)
                                                             - (*(v73->f212 - 3)
                                                              - v73->f212[13])
                                                             - (*(v73->f204 - 3)
                                                              - v73->f204[13])];
            v79 = 4 * v73->f12[0];
            v73->f12[2] = v73->f12[2] == 0;
            if ( LOWORD(((uint8_t**)v73)[v79 + 950]) < 0x4000u )
              __alt_p1_model((AltP1Block *)v73);
            v73->cur[0] += 2;
            v73->f200 += 2;
            v73->f204 += 2;
            v73->f208 += 2;
            v73->f212 += 2;
            if ( v91 )
              v92 += ((plane_desc[plane_desc[4].src_plane + 1].w8 * *(plane_desc[4].src_plane + Src - 2)
                     + plane_desc[plane_desc[4].src_plane + 1].w4 * *(plane_desc[4].src_plane + Src - 3)
                     + plane_desc[plane_desc[4].src_plane + 1].w12 * *(plane_desc[4].src_plane + Src - 1)
                     + 64) >> 7)
                   + v85;
            *(plane_desc[4].src_plane + Src) = v92;
            n4_1 = plane_count;
          }
          Src += n4_1;
          ++i_4;
        }
        while ( i_4 < i_1 );
        v20 = v87;
        v3 = v89;
        i_3 = i_1;
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

int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], P2Weights *a3, int32_t n2)
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
int32_t __alt_p2_context(AltP2Block *a1, AltP2Block *a4, AltP2Block *a5)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and altp1 segfaults while compressing.
  struct alignas(16) AltP2ContextFrame {   // 208 bytes, one stack frame
      int32_t v246;
      int32_t v256;
      int16_t *v268;
      // `alt_p2_filter`'s six sub-model weight blocks, which is what `P2Weights`
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
      uint8_t   _gap12[4];   // was int16_t (*v293)[8]
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
  P2Ctx *v247;   // row cursors into the neighbourhood table
  // These shared `__frame.v246` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  int32_t v248;
  int32_t v249;
  int32_t v250;
  int32_t v251;
  int32_t n15;
  int32_t v253;
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
  int32_t v258;
  int32_t v259;
  int32_t v260;
  uint32_t v261;
  int32_t v262;
  int32_t v263;
  int32_t v264;
  int32_t v265;
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
  int32_t v271;
  int32_t v272;
  int32_t v273;
  // These shared `__frame.v268` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  P2Ctx *v274;   // row cursors into the neighbourhood table
  P2Ctx *v281;
  int16_t *v282;
  P2Ctx *v283;
  int16_t *v284;
  P2Ctx *v285;
  P2Ctx *v286;
  int32_t n1840_2;
  int32_t n1840_1;
  AltP2Block *v289;
  // The six spill slots before `alt_p2_filter`'s six weight pointers are
  // loaded into them: a row cursor, a neighbour, a threshold-row index and
  // three counts.  Both lifetimes were `__frame.sub[0..5]`.
  int16_t (*sub0_row)[8];
  P2Ctx   *sub1_nb;
  int32_t  sub2_n, sub3_row, sub4_n, sub5_n;
  int32_t v290;
  int32_t n3536;
  uint8_t *v109, *v130, *v201;   // copies of the row cursor
  int32_t v292_cost;   // the first of two lifetimes MSVC gave one slot
  uint8_t *v292;       // the p2 row cursor, `f278736[0]`
  int16_t (*v293)[8];
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
  int32_t v304;
  int32_t v305;
  int32_t v306;
  int32_t v307;
  int32_t v308;
  int32_t v309;
  int32_t v310;
  int32_t v311;
  int32_t v312;
  int32_t v313;
  int32_t v314;
  int32_t v315;
  ;
  int16_t *v46;
  uint8_t *v157, *v172;
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
  int16_t (*v50)[8], (*v53)[8], (*v110)[8], (*v160)[8], (*v173)[8],
          (*v194)[8], (*v204)[8], (*v243)[8];
  bool v26, v58;
  int8_t v142;
  float v70, v77, v79, v89, v94, v101, v244;
  int16_t *v102;
  int16_t *v73;
  int16_t *v71;
  P2Ctx *v87;
  int16_t *v82;
  int16_t *v230;
  int16_t *v97;
  int16_t *v93;
  P2Ctx *v98;
  int16_t *v217;
  int16_t *v76;
  int16_t *v86;
  P2Ctx *v95;
  P2Ctx *v92;
  P2Ctx *v81;
  P2Ctx *v69;
  P2Ctx *v223;
  P2Ctx *v170;
  P2Ctx *v115;
  P2Ctx *v80;
  P2Ctx *v83;
  P2Ctx *v91;
  P2Ctx *v72;
  P2Ctx *v96;
  P2Ctx *v245;
  P2Ctx *v49;
  P2Ctx *v52;
  P2Ctx *v100;
  P2Ctx *v78;
  P2Ctx *v84;
  P2Ctx *v129;
  P2Ctx *v66;
  int16_t *v90, *v99, v150;
  P2Ctx *v7, *v45, *v67, *v143, *v158, *v205;   // row cursors into the neighbourhood table
  P2Ctx *v54;
  P2Ctx *v22;
  int32_t *v104;
  int32_t v6, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20,
          v21, v23, v25, v27, v29, v30, v47, v48, v51, v55, v56, v57, v65,
          v68, v74, v75, v85, v88, *v103, v105, n2, v107, n3536_5, v111,
          v112, v113, v114, v116, v117, n3536_1, v121, v122, v123, v124, v125,
          v126, v127, v128, n2256, v132, v134, v137, v138, v140, v141,
          v144, n2576, n1840_13, v147, v148, v149, v151, v154, v155, v156,
          n2896, v161, v162, v163, v164, v167, v168, v169, v171, n3536_2,
          v175, v176, v177, v178, v179, v181, v182, n3536_3, v185, v186, v188,
          v189, v190, v191, v192, v193, n1840_14, n1840_15, v198, n1840_16,
          v200, n1840_17, v203, v206, v207, v208, n960, n3536_4, v211,
          v212, v213, v214, v215, n1840_3, n1840_8, n1840_7, n1840_10,
          n1840_9, v222, v224, n1840_11, n1840_12, n255, v228, n1840_5,
          n1840_4, n1840_6, v234, v235, v236, v237, v238, v239, v240, v241,
          v242;
  int8_t v139;
  uint32_t v120, v133, v135, v136, v152, v153, v165, v180;
  uint8_t *v187;
  v6 = *(int32_t *)&a1->f278736[0];
  v7 = (P2Ctx *)a1->f278736[1];
  v8 = *(int16_t *)(v6 - 24);
  v9 = 23 * *(int16_t *)(v6 - 6);
  v289 = (AltP2Block *)(a1);
  __frame.v246 = v6;
  __frame.v268 = (int16_t *)v7;
  sub0_row = (int16_t (*)[8])*(int32_t *)&a1->f278736[2];
  v10 = sub0_row[-2][5];
  n1840_1 = 21 * sub0_row[-1][5]
          + 12 * v7[3].lane[6]
          + 16 * v7[2].lane[6]
          + 22 * v7[1].lane[6]
          + v9
          + 20 * v7->lane[6]
          + ctx_bias[0]
          + 14 * v8;
  n1840_2 = 17 * v10
          + 21 * v7[2].lane[7]
          + 15 * v7[1].lane[7]
          + 25 * v7->lane[7]
          + 9 * v7[-1].lane[7]
          + 22 * *(int16_t *)(v6 - 4)
          + ctx_bias[1]
          + 19 * *(int16_t *)(v6 - 22);
  v11 = v6;
  v12 = 17 * v7[3].lane[4]
      + 15 * v7[2].lane[4]
      + 21 * v7[1].lane[4]
      + 18 * v7->lane[4]
      + 16 * v7[-1].lane[4]
      + 22 * *(int16_t *)(v6 - 10)
      + ctx_bias[2]
      + 19 * *(int16_t *)(v6 - 28);
  v13 = sub0_row[0][5];
  v14 = *(int16_t *)(v289->f278736[3] + 10);
  v15 = *(int16_t *)(v11 - 26);
  v16 = *(int16_t *)(v11 - 8);
  v292_cost = v12;
  n3536 = 14 * __frame.v268[32] + 23 * __frame.v268[14] + 19 * __frame.v268[5] + 25 * v16 + ctx_bias[3] + 17 * v15 + 15 * (v14 + v13);
  v17 = sub0_row[-2][7];
  v18 = sub0_row[2][2] + sub0_row[0][0];
  v290 = v12 + n3536 + n1840_1 + n1840_2;
  v19 = __frame.v268[9];
  v20 = v18 + v19 + v17;
  v21 = *(int16_t *)(__frame.v246 - 18);
  v22 = (P2Ctx *)(2 * *(int16_t *)(__frame.v246 - 36) + 2 * v21);
  v23 = *__frame.v268 + 2 * v21;
  sub1_nb = (P2Ctx *)v22;
  sub2_n = v19 + v23;
  sub4_n = 16 * v20;
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
  v27 = bmf_p2_thresholds[sub3_row][0];
  v281 = (P2Ctx *)((int16_t *)(16 * ((sub2_n > bmf_p2_thresholds[sub3_row][8]) + (sub2_n > bmf_p2_thresholds[sub3_row][7]) + (sub2_n > bmf_p2_thresholds[sub3_row][6]))));
  v28 = (AltP2Block *)(v289);
  v257 = (uint8_t *)&((int16_t *)v281)[160 * sub3_row + 2 * sub5_n]
       + (sub4_n > bmf_p2_thresholds[sub3_row][2] * (int32_t)sub1_nb)
       + (sub4_n > (int32_t)sub1_nb * bmf_p2_thresholds[sub3_row][1])
       + (sub4_n > (int32_t)sub1_nb * v27)
       + __frame.v256;
  v29 = v289->f280872[(uint32_t)v257];
  if ( v289->f280872[(uint32_t)v257] )
  {
    v31 = &((float (*)[4])v289)[16 * v29];
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
      err = ((float)*(int16_t *)(__frame.v246 - 18)
             - (bmf_hsum4(acc) + *(float *)&v28->f278640)) * 2.0999999f;
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
    v30 = v289->f278716;
    v289->f278716 = ++v30;
    v28->f280872[(uint32_t)v257] = v30;
    *(int32_t *)&v28->f278656 = (int32_t)&((float (*)[4])v28)[16 * (int16_t)v30];
  }
  v45 = (P2Ctx *)v28->f278736[1];
  v28->p2_row[0][0] = (float)v45->lane[1];
  v46 = (int16_t *)v28->f278736[0];
  v28->p2_row[0][1] = (float)v45[1].lane[1];
  v28->p2_row[0][2] = (float)(v46[-8] + v45->lane[1] - v45[-1].lane[1]);
  v47 = v46[-17];
  v48 = v45->lane[1] - v45[-2].lane[1];
  sub1_nb = (P2Ctx *)v28->f278736[2];
  v49 = (P2Ctx *)(sub1_nb);
  v28->p2_row[0][3] = (float)(v47 + v48);
  v50 = (int16_t (*)[8])*(int32_t *)&v28->f278736[3];
  v28->p2_row[1][0] = (float)(v45[-1].lane[1] + v45->lane[1] - v49[-1].lane[1]);
  v51 = v50[0][1];
  sub0_row = v50;
  v28->p2_row[1][1] = (float)(-3 * (v49->lane[1] - v45->lane[1]) + v51);
  v28->p2_row[1][2] = (float)(v46[-8] + v45[2].lane[1] - v45[1].lane[1]);
  v28->p2_row[1][3] = (float)(v46[-17] + v45[1].lane[1] - v45[-1].lane[1]);
  v28->p2_row[2][0] = (float)(2 * v46[-8] - v46[-17]);
  v52 = (P2Ctx *)(sub1_nb);
  v28->p2_row[2][1] = (float)(v46[-26] + v45->lane[1] - v45[-3].lane[1]);
  v28->p2_row[2][2] = (float)(v52->lane[1] + v45[1].lane[1] - sub0_row[1][2]);
  v28->p2_row[2][3] = (float)v46[-26];
  v28->p2_row[3][0] = (float)(v45[-2].lane[1] + v45->lane[1] - v52[-2].lane[1]);
  v28->p2_row[3][1] = (float)(v46[-17] + v45[-1].lane[1] - v45[-3].lane[1]);
  v53 = sub0_row;
  v28->p2_row[3][2] = (float)(v45[1].lane[1] + ((v45[2].lane[1] + v45->lane[1]) >> 1) - v52[2].lane[1]);
  v28->p2_row[3][3] = (float)v53[0][1];
  if ( a4 )
  {
    v282 = ((int16_t *)(*(uint32_t *)&a4->f278736[0] - 18));
    v54 = (P2Ctx *)(*(uint32_t *)&a4->f278736[2] - 18);
    v281 = (P2Ctx *)((int16_t *)(*(uint32_t *)&a4->f278736[1] - 18));
    v55 = *(uint32_t *)&a5->f278736[0];
    v286 = (P2Ctx *)((int16_t *)v54);
    v56 = *(uint32_t *)&a5->f278736[1];
    v57 = *(uint32_t *)&a5->f278736[2];
    v284 = ((int16_t *)(v55 - 18));
    v58 = v28->f278732 == 0;
    v283 = (P2Ctx *)((int16_t *)(v56 - 18));
    v285 = (P2Ctx *)((int16_t *)(v57 - 18));
    if ( !v58 )
    {
      // One number added to all sixteen floats of the first four rows.
      {
        float bias = (float)v46[1];
        int32_t j, k;
        for ( j = 0; j < 4; ++j )
          for ( k = 0; k < 4; ++k )
            v28->p2_row[j][k] += bias;
      }
      v46 = (int16_t *)v28->f278736[0];
      v45 = (P2Ctx *)v28->f278736[1];
    }
    v65 = *(int32_t *)&v28->f278728;
    if ( v65 )
    {
      if ( v65 == 1 )
      {
        v81 = (P2Ctx *)(v281);
        v28->p2_row[4][0] = (float)(v46[1] + v45[3].lane[1]);
        v82 = (int16_t *)(v282);
        v28->p2_row[4][1] = (float)(v46[-18] + v81[2].lane[0] - v81->lane[0]);
        v28->p2_row[4][2] = (float)(v45[1].lane[0] + v82[-9] - v81->lane[0]);
        v83 = (P2Ctx *)(v283);
        v84 = (P2Ctx *)(v285);
        v28->p2_row[4][3] = (float)(v46[-9] + v81->lane[0] - v81[-1].lane[0]);
        v85 = v83[2].lane[0] - v84[3].lane[0];
        v86 = (int16_t *)(v284);
        v28->p2_row[5][0] = (float)(v45[1].lane[0] + v85);
        v87 = (P2Ctx *)((int16_t *)(v282));
        v28->p2_row[5][1] = (float)(v46[-18] + v86[0] - v86[-18]);
        v28->p2_row[5][2] = (float)(v45->lane[0] + v86[0] - v83->lane[0]);
        v58 = v28->f278732 == 0;
        v28->p2_row[5][3] = (float)(v46[-18] + v87->lane[2]);
        if ( v58 )
        {
          v259 = (int32_t)(uintptr_t)v46;
          v89 = (float)(v46[-18] + v87->lane[0] - v87[-2].lane[0]);
          v90 = (int16_t *)v28->f278736[2];
          v91 = (P2Ctx *)(v286);
          v28->p2_row[6][0] = v89;
          v92 = (P2Ctx *)(v281);
          v28->p2_row[6][1] = (float)(*v90 + v87->lane[0] - v91->lane[0]);
          v28->p2_row[6][2] = (float)(v286->lane[0] + v87->lane[0] - *v90 + 2 * (v45->lane[0] - v92->lane[0]));
          v88 = *(int16_t *)((uint8_t *)v259 - 18) + v45[-1].lane[0] + v281[-2].lane[0] + v87->lane[0] - v87[-1].lane[0] - v281[-1].lane[0] - v45[-2].lane[0];
        }
        else
        {
          v28->p2_row[6][0] = (float)v45->lane[0];
          v28->p2_row[6][1] = (float)v46[-27];
          v28->p2_row[6][2] = (float)(v46[-9] + v87[-1].lane[0] - v87[-2].lane[0]);
          v88 = v46[-27] + v46[-9] - v46[-36];
        }
        v28->p2_row[6][3] = (float)v88;
      }
      else
      {
        v93 = (int16_t *)(v282);
        v28->p2_row[4][0] = (float)(v46[-27] + v46[-9] - v46[-36]);
        v94 = (float)(v46[-9] + v93[-9] - v93[-18]);
        v95 = (P2Ctx *)(v281);
        v96 = (P2Ctx *)(v286);
        v28->p2_row[4][1] = v94;
        v28->p2_row[4][2] = (float)(v45[1].lane[0] + v95->lane[0] - v96[1].lane[0]);
        v97 = (int16_t *)(v282);
        v28->p2_row[4][3] = (float)(v46[-18] + v95[2].lane[0] - v95->lane[0]);
        v28->p2_row[5][0] = (float)(v45->lane[0] + v97[-18] - v95[-2].lane[0]);
        v28->p2_row[5][1] = (float)(v45->lane[0] + v97[0] - v95->lane[0]);
        v98 = (P2Ctx *)((int16_t *)(v284));
        v28->p2_row[5][2] = (float)(v46[-18] + v97[0] - v97[-18]);
        v99 = (int16_t *)v28->f278736[2];
        v100 = (P2Ctx *)(v285);
        v28->p2_row[5][3] = (float)(v46[-18] + v98->lane[0] - v98[-2].lane[0]);
        v28->p2_row[6][0] = (float)(*v99 + v98->lane[0] - v100->lane[0]);
        v28->p2_row[6][1] = (float)(v98[-2].lane[0]
                                       + v98->lane[0]
                                       - v46[-18]
                                       + 2 * (v46[-9] - v98[-1].lane[0]));
        v101 = (float)(v46[-18] + v98->lane[2]);
        v102 = (int16_t *)(v282);
        v28->p2_row[6][2] = v101;
        v28->p2_row[6][3] = (float)(*v99 + v102[2]);
      }
    }
    else
    {
      v28->p2_row[4][0] = (float)(v46[-27] + v46[-9] - v46[-36]);
      v258 = (int32_t)(uintptr_t)v46;
      v66 = (P2Ctx *)((int16_t *)v28->f278736[3]);
      v28->p2_row[4][1] = (float)(v46[-45] + v45->lane[0] - v45[-5].lane[0]);
      v67 = (P2Ctx *)v28->f278736[2];
      v28->p2_row[4][2] = (float)(v46[-36] + v45->lane[0] - v45[-4].lane[0]);
      v247 = v67;
      v68 = v67->lane[0] + 3 * v45[1].lane[0] - 4 * v67[1].lane[0];
      v69 = (P2Ctx *)(v281);
      v70 = (float)(v68 - (((v45[2].lane[0] - v45->lane[0] - (v66[2].lane[0] - v66->lane[0])) >> 1) - v66[1].lane[0]));
      v71 = (int16_t *)(v282);
      v28->p2_row[4][3] = v70;
      v72 = (P2Ctx *)(v286);
      v28->p2_row[5][0] = (float)(*(int16_t *)((uint8_t *)v258 - 36) + v71[0] - v71[-18]);
      v28->p2_row[5][1] = (float)(v45[1].lane[0] + v69[1].lane[0] - v72[2].lane[0]);
      v73 = (int16_t *)(v282);
      v28->p2_row[5][2] = (float)(v45[-2].lane[0] + v69[2].lane[0] - v72->lane[0]);
      v74 = *(int16_t *)((uint8_t *)v258 - 18) - v73[-9];
      v75 = v73[-18] + v73[0] - *(int16_t *)((uint8_t *)v258 - 36);
      v76 = (int16_t *)(v284);
      v77 = (float)(v75 + 2 * v74);
      v78 = (P2Ctx *)(v285);
      v28->p2_row[5][3] = v77;
      v28->p2_row[6][0] = (float)(v247[1].lane[0] + v76[0] - v78[1].lane[0]);
      v79 = (float)(*(int16_t *)((uint8_t *)v258 - 36) + v76[0] - v76[-18]);
      v80 = (P2Ctx *)(v283);
      v28->p2_row[6][1] = v79;
      v28->p2_row[6][2] = (float)(*(int16_t *)((uint8_t *)v258 - 18) + v80[1].lane[0] - v80->lane[0]);
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
    v28->p2_row[4][1] = (float)(v46[-36] + v239 - v240);
    v241 = v52->lane[0] + 3 * v45[1].lane[0] - 4 * v52[1].lane[0];
    v242 = v45[2].lane[0] - v45->lane[0];
    v243 = sub0_row;
    v244 = (float)(v241 - (((v242 - (sub0_row[2][2] - sub0_row[0][0])) >> 1) - sub0_row[1][1]));
    v245 = sub1_nb;
    v28->p2_row[4][2] = v244;
    v255 = (P2Ctx *)v28->f278736[4];
    v28->p2_row[4][3] = (float)(v245[-1].lane[0] + v274[1].lane[0] - v243[0][0]);
    v28->p2_row[5][0] = (float)(v243[1][1] + v274->lane[0] - v255[1].lane[0]);
    v28->p2_row[5][1] = (float)v274[3].lane[0];
    v28->p2_row[5][2] = (float)(v46[-27] + v46[-9] - v46[-36]);
    v28->p2_row[5][3] = (float)(v46[-9] + v243[0][0] - v243[-2][7]);
    v28->p2_row[6][0] = (float)(v46[-45] + v274->lane[0] - v274[-5].lane[0]);
    v28->p2_row[6][1] = (float)v255->lane[0];
    v28->p2_row[6][2] = (float)(v46[-45] + v46[-9] - v46[-54]);
    v28->p2_row[6][3] = (float)v274[-6].lane[0];
    v285 = (P2Ctx *)(nullptr);
    v283 = (P2Ctx *)(nullptr);
    v284 = (int16_t *)(nullptr);
    v286 = (P2Ctx *)(nullptr);
    v281 = (P2Ctx *)(nullptr);
    v282 = (int16_t *)(nullptr);
  }
  v103 = (int32_t *)*(int32_t *)&v28->f278668;
  __frame.sub0 = (float (*)[4])*(v103 - 1);
  v104 = ((int32_t *)*(int32_t *)&v28->f278672);
  __frame.sub1 = (float (*)[4])v104[1];
  __frame.sub2 = (float (*)[4])v104[2];
  __frame.sub3 = (float (*)[4])*(v103 - 2);
  __frame.sub4 = (float (*)[4])*v104;
  __frame.sub5 = (float (*)[4])*v103;
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
  n3536_5 = __alt_p2_filter((float (*)[4])(void *)*(int32_t *)&v28->f278656, (float (*)[4])v28->p2_row, (P2Weights *)__frame.sub, n2);
  v109 = v28->f278736[0];
  v110 = (int16_t (*)[8])*(int32_t *)&v28->f278736[1];
  *(int32_t *)&v28->f278700 = n3536_5;
  v111 = v110[0][3];
  v292 = v109;
  v112 = *(int16_t *)(v109 - 12);
  v293 = v110;
  v269 = v112 + v111;
  if ( a4 )
    v269 += (v284[3] + v282[3]) >> 1;
  v113 = *(int16_t *)(v292 - 30);
  v114 = *(int16_t *)(v292 - 48);
  v289 = (AltP2Block *)(v28);
  v115 = (P2Ctx *)((int16_t *)v28->f278736[3]);
  n3536 = n3536_5;
  v116 = *(int16_t *)(v292 - 66);
  v294 = (P2Ctx *)(v115);
  v117 = v293[3][6] + v293[-1][2] + v112 + v114 + v116 + v113;
  v118 = (AltP2Block *)(v289);
  n3536_1 = n3536;
  v260 = *(int16_t *)(v292 - 14);
  v120 = (v293[1][3] + v293[0][2] + v293[-1][1] + v260) & 0x80000
       | (v294->lane[2] + *(int16_t *)(v292 - 32) + 2 * *(int16_t *)(v292 - 68)) & 0x40000
       | (v293[-4][7] + *(int16_t *)(v292 - 50) + *(int16_t *)(v292 - 86) + *(int16_t *)(v292 - 122)) & 0x20000
       | v260 & 0x10000
       | *(uint16_t *)(v292 - 50) & 0x8000
       | (((n3536 > 3536) + (n3536 > 720) + (n3536 > 288)) << 13)
       | ((((uint32_t)(752 - (v269 + v117)) >> 31)
         + ((uint32_t)(400 - (v269 + v117)) >> 31)
         + ((uint32_t)(240 - (v269 + v117)) >> 31)) << 11);
  v289->f278676 = v120;
  if ( a4 )
  {
    v261 = v120;
    v121 = v282[-16];
    v289 = (AltP2Block *)(v118);
    v122 = *(int16_t *)(v292 - 32);
    v123 = v282[2];
    v124 = v282[-7];
    n3536 = n3536_1;
    v125 = v284[-7] & 0x2000000
         | v284[2] & 0x1000000
         | v124 & 0x800000
         | (v284[2] + v284[-16]) & 0x400000
         | (v121 + v123) & 0x200000
         | v122 & 0x100000
         | v261;
    v118 = (AltP2Block *)(v289);
    v295 = (P2Ctx *)v289->f278736[2];
  }
  else
  {
    v236 = *(int16_t *)(v292 - 68);
    v237 = *(int16_t *)(v292 - 86);
    v238 = *(int16_t *)(v292 - 122);
    v289 = (AltP2Block *)(v118);
    n3536 = n3536_1;
    v295 = (P2Ctx *)v118->f278736[2];
    v125 = (v293[3][5] + *(int16_t *)(v292 - 50) + v238 + v237) & 0x2000000
         | (v295[1].lane[2] + v295->lane[2] + v236 + *(int16_t *)(v292 - 140)) & 0x1000000
         | (v236 + *(int16_t *)(v292 - 104)) & 0x800000
         | v236 & 0x400000
         | v237 & 0x200000
         | v238 & 0x100000
         | v120;
  }
  v126 = v125 >> 11;
  v118->f278676 = v126;
  v127 = ((1 << ((v118->f284712[v126].b0 + 31) & 31)) + v118->f284712[v126].w2) >> (v118->f284712[v126].b0 & 31);
  v128 = v290;
  v129 = (P2Ctx *)((int16_t *)v118->f278736[4]);
  v130 = v292;
  v118->f278904[1] = v127;
  n2256 = v127 + n3536_1;
  v118->f278904[0] = n2256;
  v132 = *(int16_t *)(v130 - 90);
  v296 = (P2Ctx *)(v129);
  n1840 = v129[4].lane[0];
  v290 = ((v269 << 9) + v128) >> 13;
  n960_1 = v132 - n2256;
  v133 = ((uint32_t)(24 - v290) >> 20) & 0xFFFFF800;
  n1840_1 = n1840 - n2256;
  v301 = v293[5][5];
  n1840_2 = v301 - n2256;
  v297 = ((uint32_t)(39 - v290) >> 20) & 0xFFFFF800;
  if ( a4 )
  {
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v289 = (AltP2Block *)(v118);
    v134 = v282[1];
    v248 = *(int16_t *)(v292 + 2);
    v270 = v293[2][3];
    v135 = (v248 + *(int16_t *)(v292 - 52) - n2256 - (v270 - v293[5][6])) & 0x800000
         | (v248 + *(int16_t *)(v292 - 88) - n2256) & 0x400000
         | (v248 + v293[0][1] + v283[2].lane[1] - v285[2].lane[1] - n2256) & 0x200000
         | (v248 + *(int16_t *)(v292 - 16) + v284[1] - v284[-8] - n2256) & 0x100000
         | (v295[-1].lane[1] + v248 + v134 - v286[-1].lane[1] - n2256) & 0x80000
         | (v248 + v270 + v134 - v281[2].lane[1] - n2256) & 0x40000
         | n1840_2 & 0x20000
         | n1840_1 & 0x10000
         | n960_1 & 0x8000
         | (((n2256 > 2256) + (n2256 > 1056) + (n2256 > 144)) << 13)
         | ((((uint32_t)(55 - v290) >> 20) & 0xFFFFF800) + v298 + v133);
    v118 = (AltP2Block *)(v289);
    v136 = v298;
    v137 = (*(int16_t *)(v292 - 18) + v282[0] - v282[-9] - n2256) & 0x2000000
         | (v286->lane[1] + v281->lane[1] - 2 * v282[1]) & 0x1000000
         | v135;
  }
  else
  {
    v289 = (AltP2Block *)(v118);
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v254 = v294->lane[0];
    v267 = n2256 - *(int16_t *)(v292 - 54);
    v273 = v293[2][2];
    v137 = (v293[-6][3] - v293[-3][6] + v267) & 0x1000000
         | (v267 + v273 - v301) & 0x800000
         | (v296[3].lane[0] - v295[2].lane[0] + n2256 - v295[1].lane[0]) & 0x400000
         | (v296[-1].lane[0] - v254 + n2256 - v293[-2][7]) & 0x200000
         | (v293[0][0] - v273 + n2256 - *(int16_t *)(v292 - 36)) & 0x100000
         | -n1840_2 & 0x80000
         | -n1840_1 & 0x40000
         | (n2256 - v296[2].lane[0]) & 0x20000
         | (n2256 - v296[-3].lane[0]) & 0x10000
         | -n960_1 & 0x8000
         | (((n2256 > 2400) + (n2256 > 1024) + (n2256 > 240)) << 13)
         | (v297 + v133 + (((uint32_t)(11 - v290) >> 20) & 0xFFFFF800))
         | (n1840 - v254 + n2256 - v293[4][4]) & 0x2000000;
    v136 = v298;
  }
  v298 = v136;
  v138 = v137 >> 11;
  v118->f278680 = v138;
  v139 = v118->f284712[v138 + 32768].b0;
  v140 = v118->f284712[v138 + 32768].w2;
  v141 = 1 << ((v139 + 31) & 31);
  v142 = v139;
  v143 = v295;
  v144 = (v141 + v140) >> (v142 & 31);
  n2576 = v144 + n2256;
  v118->f278904[2] = n2576;
  v118->f278904[3] = v144;
  n1840_13 = v143->lane[0];
  n960_1 = v293[0][0];
  v147 = *(int16_t *)(v292 - 36);
  n1840 = n1840_13;
  v302 = v147 - n2576;
  v301 = v147 + n2576 - 2 * *(int16_t *)(v292 - 18);
  v303 = n1840_13 - n2576;
  if ( a4 )
  {
    v148 = *(int16_t *)(v292 + 2);
    v149 = *(int16_t *)(v292 - 16);
    v289 = (AltP2Block *)(v118);
    v150 = v284[0];
    v249 = v148;
    v151 = v282[0];
    v262 = v284[0];
    n1840_2 = v293[1][2];
    n1840_1 = v151 - n2576;
    v152 = (v249 - *(int16_t *)(v292 - 34) + 2 * v149 - n2576) & 0x100000
         | (2 * n960_1 - n2576 - n1840) & 0x80000
         | -v301 & 0x40000
         | v303 & 0x20000
         | v302 & 0x10000
         | (208 - v150) & 0x8000
         | (((n2576 > 2576) + (n2576 > 1280) + (n2576 > 640)) << 13)
         | ((((uint32_t)(33 - v290) >> 31) + ((uint32_t)(12 - v290) >> 31) + ((uint32_t)(4 - v290) >> 31)) << 11);
    v153 = v298;
    v154 = (v249 + v293[-1][0] - n2576 - (v295->lane[1] - n1840_2)) & 0x2000000
         | (v249 + v149 - n2576 - (v293[0][1] - n1840_2)) & 0x1000000
         | (v262 - n2576 + n1840 - v285->lane[0]) & 0x800000
         | (n1840_1 + n1840 - v286->lane[0]) & 0x400000
         | (n1840_1 + v293[-2][7] - v281[-1].lane[0]) & 0x200000
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
         | (v294[2].lane[0] - v295[-1].lane[0] + n2576 - v293[3][3]) & 0x800000
         | (v272 - v293[1][1] - v303) & 0x400000
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
  v118->f278684 = v155;
  v156 = ((1 << ((v118->f284712[v155 + 65536].b0 + 31) & 31)) + v118->f284712[v155 + 65536].w2) >> (v118->f284712[v155 + 65536].b0 & 31);
  v157 = v292;
  v158 = v295;
  v118->f278904[5] = v156;
  n2896 = v156 + n2576;
  v160 = v293;
  v118->f278904[4] = n2896;
  v161 = v160[0][1];
  v162 = *(int16_t *)(v157 - 36);
  v250 = *(int16_t *)(v157 + 2);
  v263 = v161;
  v163 = *(int16_t *)(v157 - 54);
  v164 = v158[1].lane[1];
  v304 = v162;
  v305 = v164;
  v303 = n2896 - v163;
  v306 = n2896 - v250;
  v165 = 9 - v290;
  v290 = -v290;
  v166 = (AltP2Block *)(v289);
  v167 = (int32_t)((3 * (v304 - *(int16_t *)(v292 - 18)) + n2896 - v163) & 0x2000000
             | (v305
              - ((uint32_t)(v293[1][2] + v293[2][3] + v293[-1][0] + v263) >> 1)
              + v306)
             & 0x1000000
             | (v295[-3].lane[0] - v293[-3][6] + n2896 - v293[-2][7]) & 0x800000
             | -(v295[2].lane[0] + n2896 - 2 * v293[1][1]) & 0x400000
             | (v293[-3][7] - v304 + n2896 - v263) & 0x200000
             | (n2896 - v293[3][3]) & 0x100000
             | (v306 - v305) & 0x80000
             | (n2896 - v294->lane[0]) & 0x40000
             | (2 * n2896 - v263 - (v293[0][0] + v250)) & 0x20000
             | (n2896 - *(int16_t *)(v292 - 16) - v250) & 0x10000
             | (n2896 - v163) & 0x8000
             | (((n2896 > 2896) + (n2896 > 1568) + (n2896 > 592)) << 13)
             | ((((uint32_t)(v290 + 37) >> 31) + ((uint32_t)(v290 + 19) >> 31) + (v165 >> 31)) << 11)) >> 11;
  v289->f278688 = v167;
  LOBYTE(v163) = v166->f284712[v167 + 98304].b0;
  v168 = v166->f284712[v167 + 98304].w2;
  v169 = 1 << ((v163 + 31) & 31);
  LOBYTE(v164) = v163;
  v170 = (P2Ctx *)(v294);
  v171 = (v169 + v168) >> (v164 & 31);
  v172 = v292;
  v173 = v293;
  v166->f278904[7] = v171;
  n3536_2 = v171 + n2896;
  n3536 = n3536_2;
  v166->f278904[6] = n3536_2;
  v175 = v173[0][0];
  v176 = v170->lane[0];
  v307 = *(int16_t *)(v172 + 2);
  v308 = v175;
  v177 = *(int16_t *)(v172 - 70);
  v178 = v173[1][1];
  v179 = v173[-2][7];
  v309 = v177;
  v310 = v178;
  v311 = v179;
  v312 = n3536_2 - v307;
  v313 = n3536_2 + v176;
  v180 = (((n3536_2 > 3056) + (n3536_2 > 1952) + (n3536_2 > 368)) << 13)
       | (v297 + (((uint32_t)(v290 + 21) >> 20) & 0xFFFFF800) + v298);
  v181 = ((uint16_t)n3536_2 - (uint16_t)v177 - (uint16_t)v307) & 0x8000;
  v182 = n3536_2 - *(int16_t *)(v292 - 18);
  n3536_3 = n3536;
  v184 = (AltP2Block *)(v289);
  v185 = (int32_t)((n3536 - ((uint32_t)(v310 + v311 + 2 * v308) >> 2)) & 0x2000000
             | (v313 - *(int16_t *)(v292 - 36) - (v294[2].lane[1] + v307)) & 0x1000000
             | (n3536 - 2 * *(int16_t *)(v292 - 34) - (v307 - v309)) & 0x800000
             | (n3536 - *(int16_t *)(v292 - 52) - v307 - (v308 - v293[-4][5])) & 0x400000
             | (v313 - v307 - (v310 + v295[-1].lane[1])) & 0x200000
             | (v295[-2].lane[0] + n3536 - 2 * v311) & 0x100000
             | (v295->lane[1] - 2 * v293[0][1] + v312) & 0x80000
             | (v182 - (v308 - v311)) & 0x40000
             | (v312 - v293[4][5]) & 0x20000
             | (v312 - v293[-3][7]) & 0x10000
             | v181
             | v180) >> 11;
  v289->f278692 = v185;
  v186 = ((1 << ((v184->f284712[v185 + 131072].b0 + 31) & 31)) + v184->f284712[v185 + 131072].w2) >> (v184->f284712[v185 + 131072].b0 & 31);
  v187 = (uint8_t *)v295;
  v271 = v186;
  v184->f278904[9] = v186;
  n1840 = n3536_3 + v186;
  v184->f278904[8] = n3536_3 + v186;
  v188 = ((uint8_t *)v293[-1])[15];
  v189 = v187[17];
  v314 = ((uint8_t *)v293[-2])[13]
       + ((uint8_t *)v293[4])[7]
       + *(v292 - 37)
       + *((uint8_t *)&v294->lane[8] + 1)
       + *(v187 - 1)
       + v187[53];
  v190 = v187[89];
  v191 = v188 + *(v292 - 19) + v189;
  v192 = *(v187 - 19);
  v315 = v191;
  v193 = *((uint8_t *)&v294[2].lane[8] + 1)
       + *((uint8_t *)&v294[1].lane[8] + 1)
       + *((uint8_t *)&v294[-1].lane[8] + 1)
       + *((uint8_t *)&v294[-2].lane[8] + 1)
       + *((uint8_t *)&v296[-2].lane[8] + 1)
       + v187[107]
       + v190
       + v187[71]
       + v192
       + *(v187 - 37);
  v194 = v293;
  n1840_14 = n1840;
  n960_1 = ((uint8_t *)v293[6])[11]
         + ((uint8_t *)v293[5])[9]
         + ((uint8_t *)v293[-3])[11]
         + ((uint8_t *)v293[-4])[9]
         + 3 * (((uint8_t *)v293[3])[5] + *((uint8_t *)v295 + 35))
         + 7 * ((uint8_t *)v293[1])[1]
         + 6 * ((uint8_t *)v293[2])[3]
         + *(v292 - 91)
         + *(v292 - 109)
         + *(v292 - 127)
         + 8 * *(v292 - 1)
         + *((uint8_t *)&v296[2].lane[8] + 1)
         + *((uint8_t *)&v296[1].lane[8] + 1)
         + *((uint8_t *)&v296->lane[8] + 1)
         + *((uint8_t *)&v296[-1].lane[8] + 1)
         + *(v292 - 55)
         + *(v292 - 73)
         + v193
         + 4 * v315
         + 2 * v314;
  v196 = (AltP2Block *)(v289);
  n1840_15 = v289->f278720;
  *(int32_t *)&v289->f278760[4] = (n1840 < 1840) + (n1840 < 272);
  v198 = v194[0][0];
  n1840_16 = v196->f278724;
  n1840_1 = n1840_15;
  n1840_2 = n1840_16;
  v200 = (n1840_14 - v198 <= n1840_16) + (n1840_14 - v198 < n1840_15);
  v201 = v292;
  *(int32_t *)&v196->f278760[8] = v200;
  n1840_17 = n1840_14 - *(int16_t *)(v201 - 18);
  v203 = n1840_17 < n1840_1;
  v26 = n1840_17 <= n1840_16;
  v204 = v293;
  v205 = v295;
  *(int32_t *)&v196->f278760[12] = v26 + v203;
  *(int32_t *)&v196->f278760[16] = ((uint8_t *)v204[1])[0];
  *(int32_t *)&v196->f278760[20] = *(v201 - 2);
  v206 = *(v201 - 19);
  v207 = *(v201 - 1);
  v208 = *((uint8_t *)v205 + 17) + ((uint8_t *)v204[1])[1];
  n960 = n960_1;
  v302 = v208;
  n3536_4 = n3536;
  v301 = v207 + v206;
  if ( a4 )
  {
    v211 = *((uint8_t *)v284 + 17);
    v212 = *((uint8_t *)v284 - 1);
    v289 = (AltP2Block *)(v196);
    v213 = *((uint8_t *)v282 - 1);
    v251 = *((uint8_t *)&v283->lane[8] + 1) + *((uint8_t *)&v281->lane[8] + 1);
    v264 = v211 + *((uint8_t *)v282 + 17);
    n960 = v251 + n960_1 + 4 * v264 + 2 * (v213 + v212);
    v214 = v264 + v301 + *((uint8_t *)v284 - 19) + v212 + *((uint8_t *)v282 - 19) + v213;
    n3536_4 = n3536;
    if ( v196->f278732 )
    {
      n1840_3 = n1840 - v293[0][1] - *(int16_t *)(v292 + 2);
      if ( n1840_3 < n1840_1 || n1840_3 > n1840_2 )
      {
        n1840 = n1840 - *(int16_t *)(v292 - 16) - *(int16_t *)(v292 + 2);
        v215 = n1840 >= n1840_1 && n1840_2 >= n1840;
      }
      else
      {
        v215 = 1;
      }
    }
    else
    {
      v215 = v251 + v264 + v302 + *((uint8_t *)&v285->lane[8] + 1) + *((uint8_t *)&v286->lane[8] + 1);
    }
    if ( *(int32_t *)&v289->f278728 == 1 )
    {
      v230 = (int16_t *)(v282);
      n960_1 = n960;
      v253 = v214;
      n1840_4 = n1840_2;
      n1840_5 = v282[0] - v281->lane[0];
      *(int32_t *)&v289->f278760[16] = (n1840_5 <= n1840_2) + (n1840_5 < n1840_1);
      n1840_6 = v230[0] - v230[-9];
      v234 = (n1840_6 <= n1840_4) + (n1840_6 < n1840_1);
      v214 = v253;
      n960 = n960_1;
      n3536_4 = n3536;
      *(int32_t *)&v196->f278760[20] = v234;
    }
    else if ( *(int32_t *)&v196->f278728 > 1 )
    {
      v217 = (int16_t *)(v284);
      n960_1 = n960;
      v265 = v215;
      n1840_7 = n1840_2;
      n1840_8 = v284[0] - v283->lane[0];
      *(int32_t *)&v289->f278760[16] = (n1840_8 <= n1840_2) + (n1840_8 < n1840_1);
      n1840_10 = v217[0] - v217[-9];
      n1840_9 = n1840_1;
      v222 = n1840_10 < n1840_1;
      v26 = n1840_10 <= n1840_7;
      v215 = v265;
      v223 = (P2Ctx *)(v281);
      n960 = n960_1;
      *(int32_t *)&v196->f278760[20] = v26 + v222;
      v224 = v282[0];
      n1840_11 = v224 - v223->lane[0];
      v26 = n1840_9 <= n1840_11;
      n3536_4 = n3536;
      if ( v26 && n1840_11 <= n1840_2 )
      {
        v214 = 1;
      }
      else
      {
        n1840_12 = v224 - v282[-9];
        v214 = n1840_12 >= n1840_1 && n1840_12 <= n1840_2;
      }
    }
  }
  else
  {
    v214 = v301 + *(v292 - 37) + *(v292 - 55) + *(v292 - 73);
    v215 = *((uint8_t *)&v296->lane[8] + 1) + *((uint8_t *)&v294->lane[8] + 1) + v302 + *(v292 + 17);
  }
  if ( n960 >= 960 )
  {
    n15 = 15;
    *(int32_t *)&v196->f278704 = 15;
  }
  else
  {
    n15 = v196->f280752[n960 >> 3];
    *(int32_t *)&v196->f278704 = n15;
  }
  n255 = (n3536_4 + v271 + 7) >> 4;
  if ( n255 >= 255 )
    n255 = 255;
  if ( n255 < 0 )
    n255 = 0;
  *(int32_t *)&v196->f278708 = v196->f278944[n255 + 4] + n15;
  v228 = *(int32_t *)&v196->f278760[4];
  *(int32_t *)&v196->f278712 = n15 + v196->f278944[n255];
  *(int32_t *)&v196->f278704 = n15
                          + 32 * (v215 == 0)
                          + 16 * (v214 == 0)
                          + *(int32_t *)&v196->f278760[*(int32_t *)&v196->f278760[20] + 21]
                          + *(int32_t *)&v196->f278760[*(int32_t *)&v196->f278760[16] + 17]
                          + *(int32_t *)&v196->f278760[*(int32_t *)&v196->f278760[12] + 13]
                          + *(int32_t *)&v196->f278760[*(int32_t *)&v196->f278760[8] + 9]
                          + *(int32_t *)&v196->f278760[v228 + 5];
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
  uint8_t *v62;
  bool v46, v48, v59;
  int8_t v35;
  uint8_t *v28;   // `uint8_t *` beside the `char` scalars above
  ModelBlock *Blockaa_1;
  ModelBlock *Blockaa_4;
  int32_t n8, v8, v11, n4, n0x2000_2, n0x2000_1, v20, v26, n4_2, v30, v31, v32, *p_n4, n16_2,
          v39, v44, n256, v49, v50, v51, v52, v54, v55, v56, v57, v58, v63, *p_n4_2, n16_1, v68,
          n0x2000, v71, v72, v74, *p_n4_1, n16;
  ModelBlock *Blockaa_3;
  uint32_t k_2, i, v12, v19, n0x2000_4, n0x2000_3, v24, k, v29, k_3, j_1, j, v53, v61, v64, v70,
           v73, v75;
  uint16_t *n0x2000_6;   // was uint64_t *, read only as uint16_t
  uint8_t *v4, *v10, *v33, *v42, *v43, *v45, *v60;
  void *v13, *v34;
  __frame.slot11 = (ModelBlock *)(Blocka);
  v4 = a3;
  n8 = Blocka->f8;
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
    v48 = *(int32_t *)&Blockaa_1->f8 < 8;
    v49 = Blockaa_1->f4;
    *(int32_t *)&Blockaa_1->f16 = 0;
    if ( v48 )
    {
      v50 = 0;
      if ( v49 )
      {
        v51 = *(int32_t *)&Blockaa_1->f0;
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
            v56 = *(int32_t *)&Blockaa_1->f8;
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
            *(int32_t *)&Blockaa_1->f16 += v59;
            *(uint16_t *)(Blockaa_1->f1078236 + 2 * v54) = v58;
            v51 = *(int32_t *)&Blockaa_1->f0;
            ++v54;
          }
          while ( v53 < *(int32_t *)&Blockaa_1->f0 );
          v50 = *(int32_t *)&Blockaa_1->f16;
          __frame.v84 = v54;
          v52 = __frame.v83 + 1;
        }
        while ( __frame.v83 + 1 < (uint32_t)Blockaa_1->f4 );
      }
    }
    else if ( v49 * *(int32_t *)&Blockaa_1->f0 )
    {
      v60 = __frame.slot2;
      v61 = 0;
      do
      {
        *(int32_t *)&Blockaa_1->f16 += *(uint32_t *)&__frame.buf[4 * *v60 - 4] == 0;
        v62 = Blockaa_1->f1078236;
        v63 = *v60;
        *(uint32_t *)&__frame.buf[4 * v63 - 4] = 1;
        ++v60;
        *(uint16_t *)(v62 + 2 * v61++) = v63;
      }
      while ( v61 < Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0 );
      v50 = *(int32_t *)&Blockaa_1->f16;
    }
    else
    {
      v50 = 0;
    }
    rc.encode(v50 - 1, v50, __frame.slot4 + 1);
    v64 = *(int32_t *)&Blockaa_1->f16;
    if ( v64 <= __frame.slot4 )
    {
      __init_symbol_list((SymList *)__frame.v86, (int32_t)Blockaa_1, __frame.slot4 - v64 + 2, 1);
      __frame.v87 = 19 * ((SymList *)__frame.v86)->n;
      v70 = *(int32_t *)&Blockaa_1->f16;
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
            v70 = *(int32_t *)&Blockaa_1->f16;
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
      if ( Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0 )
      {
        v75 = 0;
        do
        {
          *(uint16_t *)(Blockaa_1->f1078236 + 2 * v75) = *(uint32_t *)&__frame.buf[4
                                                                  * *(uint16_t *)(Blockaa_1->f1078236 + 2 * v75)
                                                                  - 4];
          ++v75;
        }
        while ( v75 < Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0 );
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
    *(int32_t *)&Blockaa_1->f16 = 1;
    *(uint32_t *)__frame.buf = __frame.slot4 & *(uint32_t *)a3;
    *(uint16_t *)Blockaa_1->f1078236 = 0;
    if ( (uint32_t)(Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0) > 1 )
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
            v68 = __frame.slot7->f16;
            mode_symbol[1] = n4;
            v11 = (uint16_t)v68;
            n0x2000 = v68 + 1;
            *(uint16_t *)(__frame.n0x2000_5 + 2 * n4) = v11;
            v12 = __frame.slot3;
            Blockaa_2->f16 = n0x2000;
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
        *(uint16_t *)(__frame.slot7->f1078236 + 2 * v12++) = v11;
        if ( v12 >= *(uint32_t *)&Blockaa_3->f4 * Blockaa_3->f0 )
        {
          v4 = __frame.slot8;
          k_2 = __frame.slot9;
          Blockaa_1 = (ModelBlock *)((int32_t *)__frame.slot7);
          n0x2000_2 = __frame.slot7->f16;
          goto LABEL_14;
        }
      }
    }
    n0x2000_2 = *(int32_t *)&Blockaa_1->f16;
LABEL_14:
    rc.encode(n0x2000_2 - 1, n0x2000_2, 0x2001u);
    n0x2000_1 = *(int32_t *)&Blockaa_1->f16;
    if ( n0x2000_1 > 0x2000 )
    {
      (__frame.slot[1]) = bmf_new(Blockaa_1->f4 * k_2 * *(int32_t *)&Blockaa_1->f0);
      v26 = *(int32_t *)&Blockaa_1->f0;
      n4_2 = Blockaa_1->f4;
      __frame.n0x2000_5 = *(int32_t *)&Blockaa_1->f0;
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
      __frame.v79 = Blockaa_1->f1078236;
      Blockaa_1->f4 = k_2 * __frame.slot0;
      *(int32_t *)&Blockaa_1->f8 = 8;
      free(__frame.v79);
      v34 = bmf_new(2 * Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0);
      __frame.v79 = (__frame.slot[1]);
      Blockaa_1->f1078236 = (uint8_t *)v34;
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
        n0x2000_1 = __frame.slot7->f16;
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
            n0x2000_3 = __frame.slot7->f16;
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

int32_t __cost_candidate(uint8_t *a1, uint8_t *n2, int32_t a3, int8_t a4, int32_t a5, int32_t a6, int32_t a7, uint8_t *a8)
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
      uint8_t *n2_2;
      int32_t v88;
      uint8_t *v89;
      int32_t v90;
      int32_t v91;
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
  uintptr_t v63, v64;   // were int32_t: addresses, masked and tagged
  uint8_t *v14;   // were int32_t: these hold addresses
  bool v57, v67;
  double v16, v17, v18, v19, v20, v32, n191_1, n191_4;
  int32_t v9, v10, v12, v22, v23, v24, v25, v26, v27, v28, v30, v31, n191, n191_3, v37, v38,
          v39, v40, v41, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v58, v59,
          v60, n191_6, n191_7;
  uint32_t v69, v70;
  uint8_t *n2_3, *v15, *v21, *v29, *v42, *n2_4, *v44, *v65, *v66;
  __frame.v101 = a3;
  __frame.n2_1 = n2;
  __frame.v99 = a1;
  __frame.v88 = a3;
  __frame.n4 = plane_count;
  v9 = *((uint16_t *)a1 + 2);
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
  v14 = (uint8_t *)__frame.v88;
  v15 = __frame.v89;
  v16 = 0;
  v17 = 0.0;
  v18 = 0.0;
  v19 = 0.0;
  v20 = 0.0;
  __frame.v90 = 16 * (uint32_t)__frame.n2_2;
  *(uint8_t *)(16 * (uint32_t)__frame.n2_2 + __frame.v88) = 2;
  *(v14 + 33) = (uint8_t)(uintptr_t)n2_3;
  __frame.v91 = (int32_t)&n2_3[(uint32_t)v15];
  v21 = &n2_3[(uint32_t)v15 + 16 + v12 + __frame.n4];
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
  v38 = (*((uint16_t *)__frame.v89 + 1) - 1) * *(uint16_t *)__frame.v89;
  __frame.v92 = v10;
  __frame.v94 = v38 - 1;
  __frame.v93 = -v37;
  v39 = -plane_count;
  v40 = __frame.v91 - (-v37 - plane_count);
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
    __frame.v91 = v46;
    v48 = __frame.v94;
    n2_4 = &__frame.n2_1[-v39];
    v49 = ((uint16_t)__frame.v91
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
  v52 = 16 * (uint32_t)__frame.n2_2;
  *(uint32_t *)(a8 + 16 * (uint32_t)__frame.n2_2) = v51;
  *(uint32_t *)(a8 + v52 + 4) = __estimate_cost((uint8_t *)__frame.v74, 1024);
  *(uint32_t *)(a8 + v52 + 8) = __estimate_cost((uint8_t *)__frame.v75, 1024);
  *(uint32_t *)(a8 + v52 + 12) = __estimate_cost((uint8_t *)__frame.v76, 1024);
  __frame.v91 = __estimate_cost((uint8_t *)__frame.buf, 1024);
  __frame.v92 = __estimate_cost((uint8_t *)__frame.v72, 1024);
  v53 = __estimate_cost((uint8_t *)__frame.v73, 1024);
  __frame.v94 = v53;
  v54 = v53;
  if ( __frame.v91 < v53 )
    v54 = __frame.v91;
  if ( __frame.v92 < v53 )
    v53 = __frame.v92;
  v55 = __frame.v92 + v54;
  v56 = __frame.v91 + v53;
  v57 = v55 < v56;
  __frame.v93 = v56;
  v58 = __frame.v94;
  if ( v57 )
  {
    __frame.v93 = v55;
    v59 = v50;
    v50 = __frame.v95;
    __frame.v91 = __frame.v92;
    __frame.v95 = v59;
    v60 = *(uint32_t *)(a8 + v52 + 4);
    *(uint32_t *)(a8 + v52 + 4) = *(uint32_t *)(a8 + v52 + 8);
    n191_6 = __frame.n191_5;
    *(uint32_t *)(a8 + v52 + 8) = v60;
    n191_7 = __frame.n191_2;
    __frame.n191_2 = n191_6;
    __frame.n191_5 = n191_7;
  }
  v63 = __frame.v90;
  v64 = __frame.v88;
  *(uint32_t *)(__frame.v90 + __frame.v88 + 4) = __frame.n191_2;
  *(uint32_t *)(v63 + v64 + 8) = __frame.n191_5;
  v65 = &__frame.n2_2[v50];
  v66 = &__frame.n2_2[__frame.v95];
  *(uint8_t *)(16 * (uint32_t)v65 + v64) = 0;
  *(uint8_t *)(v64 + 1) = (uint8_t)(uintptr_t)v65;
  v67 = 0;                            // -S
  *(uint8_t *)(16 * (uint32_t)v66 + v64) = 1;
  *(uint8_t *)(v64 + 17) = (uint8_t)(uintptr_t)v66;
  if ( !v67 && *((uint32_t *)__frame.v89 + 3) > 0x1000000u )
    return __frame.v91 + v58 + *(uint32_t *)(a8 + v52);
  v69 = *(uint32_t *)(a8 + v52);
  v70 = *(uint32_t *)(a8 + v52 + 8);
  if ( v69 >= *(uint32_t *)(a8 + v52 + 4) )
    v69 = *(uint32_t *)(a8 + v52 + 4);
  if ( v70 >= *(uint32_t *)(a8 + v52 + 12) )
    v70 = *(uint32_t *)(a8 + v52 + 12);
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
  int32_t n4, v5, v6, n192, v11, v13, v14, n2_3, n2, n16, *v25, n128_1, n128, v30, v31, v32, v33,
          v34, v35, n0x4000, n2_1, v43, n0x100, v48, v49, i, n255, j_1, j, v57, n128_2, v59,
          n128_3, v61, v62, v63, v71, __choose_plane_coding_n191, n191_2, n191_3, v81, v82, v83, v84, v85,
          v86, v87, v88, v89, v90, v91, v93, v94, v95, v96, v98, v99, v100, v101, v102, n191_4,
          n2_2, __choose_plane_coding_n3_1, *v112, v115, v116, v118, v119, v120, v124, v125, v126, v127,
          v128, v130, v131, v132, v133, v137, v138, v139, v140, v141, v143, n192_1, n192_2,
          n192_4, n192_3, v148, v149, v150, v152, v154, v155, v156, v157, v158, n192_5, v161,
          v162, v163, v165, v167, v168, v169, v170, v171, n192_6;
  uint32_t v9, v15, v17, v26, v36, v38, v39, v67, v80, v103, v104, v105, v108, v110, v129, v142,
           v159, v172;
  uint8_t *v29, *v64, *v97, *v121, *v122, *v123, *v134, *v135, *v136, *v151, *v153, *v164, *v166;
  __frame.v228 = (BmfImage *)(a1);
  n4 = plane_count;
  v5 = (uint16_t)a1->stride;
  v6 = a1->data_size;
  __frame.v226 = (BmfImage *)((uint8_t *)a1);
  alphabet_reduced = 1;
  LODWORD(__frame.v208) = v5;
  __frame.v227 = (uintptr_t)((uint8_t *)a1 + v6 + 16);
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
      v15 = __cost_candidate((uint8_t *)__frame.v226, (uint8_t *)nullptr, (int32_t)(uintptr_t)__frame.v217, v7, __frame.v174, __frame.v175, __frame.v176, (uint8_t *)&__frame.v214[2]);
      v17 = __cost_candidate((uint8_t *)__frame.v226, (uint8_t *)1, (int32_t)(uintptr_t)__frame.v218, v16, __frame.v174, __frame.v175, __frame.v176, (uint8_t *)&__frame.v214[2]);
      v19 = v17 >= v15;
      if ( v17 >= v15 )
        v17 = v15;
      n2_3 = !v19;
      __frame.v205[2] = n2_3;
      n2_4 = __cost_candidate((uint8_t *)__frame.v226, (uint8_t *)2, (int32_t)(uintptr_t)__frame.v219, v18, __frame.v174, __frame.v175, __frame.v176, (uint8_t *)&__frame.v214[2]) < v17;
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
          v36 = v34 - ((uint32_t)(((v32 + v30) << 6) + 40) >> 7);
          ++__frame.v180[v36];
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
  int32_t Offset_1;
  // These shared `__frame.Size` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t Sizea;
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
      memset((uint8_t *)v3 + 16,0,v3->data_size);
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
        Sizea = Sizea_1;
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
          __builtin_memset((void *)__frame.Src_2, Sizea, j_3);
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
            memcpy((uint8_t *)Src_1,(uint8_t *)__frame.Buffer_3,Sizea);
            Src_1 += Sizea;
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
    memset((uint8_t *)v3 + 16,0,v3->data_size);
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
    Offset_1 = Offset_2;
    v35 = __frame.bmp_height - 1;
    __frame.v52 = v3;
    Src_6 = __frame.Src;
    while ( 1 )
    {
      ElementCount_1 = fread(Src_6, 1u, ElementCount, Stream_v);
      ElementCount = *((uint16_t *)__frame.v52 + 2);
      if ( ElementCount_1 != ElementCount )
        return nullptr;
      if ( Offset_1 )
      {
        fseek(Stream_v, Offset_1, 1);
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
  SymEntry *v3, *v7, *v20, *v25, *v33, *v36, *v38, *v44, *v45, *v47, *v48;
  SymEntry **v4, **v21, **v26;
  int8_t v23;
  uint8_t v34, v40;
  uint16_t v35, v39;
  int32_t sym_cum, sym_high, v2, v5, v6, v8, n0x2000_5, n0x2000_2, n251, v46,
          v49, v51, v53;
  uint32_t v43, v52;   // counts that MSVC spilled into the list's first slot
  SymList *v9, *v32;
  uint32_t __decode_symbol_list_n0x800000, n0x2000_6, n0x2000_4,
           n0x2000_3, tot_1, v41, v42, v50,
           v54;
  v2 = a1->live;
  v3 = a1->ent;
  v4 = __frame.list;
  __frame.v68 = (uint8_t)exclusion_gen;
  __frame.v67 = a1;
  v5 = 0;
  v6 = 0;
  do
  {
    if ( (uint8_t)exclusion_mask[v3[v6].sym] == __frame.v68 )
    {
      v8 = 0;
    }
    else
    {
      v7 = &v3[v6];
      v8 = v7->cnt;
      *v4++ = v7;
    }
    v5 += v8;
    ++v6;
  }
  while ( v6 < v2 );
  v9 = __frame.v67;
  if ( !v5 )
    return -1;
  *v4 = nullptr;
  __frame.v65 = v9->f8;
  n0x2000_6 = v5 + __frame.v65;
  n0x2000_4 = rc.get_freq(n0x2000_6);
  __frame.tot = v5 + __frame.v65;
  __frame.v67 = v9;
  n0x2000_5 = v5 + __frame.v65 - 1;
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
      goto LABEL_19;
    }
  }
  v32 = __frame.v67;
  sym_high = n0x2000_2;
  sym_cum = n0x2000_2 - v20->cnt;
  __frame.list7 = v20->sym;
  v20->cnt += 4;
  v33 = v32->ent;
  v32->f12 += 4;
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
  v41 = v32->f16;
  if ( n251 > 251 || v41 < v32->f12 )
  {
    __frame.list0 = v32->live;
    v42 = 20 * v32->n;
    v43 = __frame.list0;
    __frame.list5 = __decode_symbol_list_n0x800000;
    __frame.list4 = v41 < v42;
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
    v50 = v32->f8;
    v51 = v44->cnt;
    __frame.list0 = 0;
    if ( !v51 )
    {
      v52 = __frame.list0;
      do
      {
        ++v52;
        v32->f8 = ++v50;
        v53 = v45->cnt;
        --v45;
      }
      while ( !v53 );
      __frame.list0 = v52;
      v32->live -= v52;
    }
    v54 = v32->f12;
    v32->f8 = v50 - (v50 >> 1);
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
    tot_1 = n0x2000_6;
    v32->f12 = v54 - (v54 >> 1);
  }
  else
  {
    tot_1 = n0x2000_6;
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
  }
LABEL_19:
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
            uint64_t *sym4;
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
  uint64_t *v80;
  uint16_t *v36;
  uint8_t *v21, *v23, *v57;   // row cursors out of ModelBlock
  uint8_t *v46, *v66, *n15_17;
  uint8_t *v53, *v54, *v55, *v61, *v108, *n15_10;   // row cursors out of ModelBlock
  bool v19;
  int8_t v74, v91;
  uint8_t v70, v71, v72, v73, v75;
  uint8_t *v157;   // `uint8_t *` beside the `char` scalars above
  int16_t v14, n4_14, v146, v160, n15_4;
  ModelBlock *this_4;
  uint8_t *v25, *v56;   // row cursors out of f56[9]; were int32_t
  int32_t arg_cum, arg_high, arg_tot, n4_8, n4_7, n15_6, n15_7, v8, v9, v10, v12, v13, v15,
          n4_9, __decode_pixel_n15, v22, n4_11, v26, v27, v29, v31, n0xFFFF, v33, n0xFFFF_1,
          n53248, n4_12, v40, n15_11, n8, n15_12, v44, n4_22, n15_14, n15_18, v49, v50, v51,
          n4_13, *v59, v60, n15_13, v67, n15_15, n4_17, v81, v83, v84,
          v85, n4, n256_2, n15_1, n256_1, n15_23, n4_19, n4_20, n4_5, n4_6, n15_5, v102, v103,
          v104, v105, v107, v109, v111, v112, v113, v114, v115, v116, v117, v118, v119,
          v120, v122, v123, v124, v125, v126, v127, v128, n32, n15_25, n4_21, v135,
          n256, v143, n4_18, n15_19, n15_20, v148, v149, v150, v151, v152, v153, v155, n15_21,
          v158, v159, v161, v163, v164, n4_2, n15_22, n256_4, n256_5, n256_3, n15_2;
  uint64_t *v170;
  uint64_t *v100;
  uint64_t *v171;
  uint64_t *v4;
  uint64_t *v90;
  uint16_t *v97;
  uint16_t *v106;
  uint16_t *n15_9, *v16, *n4_10, *v58, *freq_tbl, *v95, *v110, *v121, v154, v162,
           v174;
  ModelBlock *this_3;
  ModelBlock *this_2;
  SymList *v132, *v133;
  SymList **v134;
  uint32_t bin_tot, n4_16, n4_15, v136, v137, v138, v139, v140, v172, v173, v175, v176,
           v177;
  uint64_t *v142;
  uint8_t *v39;
  uint8_t *v38;
  uint8_t *v76;
  uint8_t *n15_16;
  n15_9 = (uint16_t *)*(uint32_t *)&_this->f56[6];
  n4_8 = *n15_9;
  v4 = (uint64_t *)((uint16_t *)*(uint32_t *)&_this->f56[5]);
  n4_7 = *((uint16_t *)v4 - 4);
  n15_6 = n15_9[4];
  __frame.sym5 = (ModelBlock *)(_this);
  n15_7 = *(n15_9 - 4);
  __frame.sym0 = n4_8;
  ::mode_symbol[1] = n4_8;
  __frame.sym1 = n4_7;
  ::mode_symbol[2] = n4_7;
  __frame.sym3 = n15_6;
  ::mode_symbol[3] = n15_6;
  v8 = *((uint8_t *)n15_9 + 3) + 4 * (n15_6 == n15_7);
  v9 = *((uint8_t *)n15_9 + 11);
  __frame.sym2 = n15_7;
  mode_symbol[4] = n15_7;
  v10 = 2 * *((uint8_t *)v4 - 6) + 8 * v9 + v8;
  this_2 = (ModelBlock *)(__frame.sym5);
  v12 = 32 * *((uint8_t *)v4 - 4) + 16 * *((uint8_t *)v4 - 2) + v10;
  if ( n4_8 == n4_7 )
  {
    if ( __frame.sym3 == __frame.sym0 )
    {
      v14 = *((uint16_t *)__frame.sym5 + n4_8 + 3029720);
      if ( n4_8 == __frame.sym2 )
        v13 = (uint16_t)(v14 - *((uint16_t *)v4 - 8));
      else
        v13 = (uint16_t)(v14 - __frame.sym2);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)__frame.sym5 + n4_8 + 3029720) - __frame.sym3);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)__frame.sym5 + n4_8 + 3029720) - __frame.sym1);
  }
  __frame.sym5->f6059432 = (uint32_t)&((uint32_t *)__frame.sym5)[4 * v13 + 269674];
  v15 = this_2->ctx_state[v12];
  this_2->f36 = v15;
  v16 = (uint16_t *)&((uint32_t *)this_2)[0x10000 * v15 + 531818 + v13];
  this_2->f6059436 = (uint8_t *)v16;
  n4_9 = *v16;
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
    v19 = n4_9 == __frame.sym2;
    __decode_pixel_n15 = 60;
    if ( !v19 )
      __decode_pixel_n15 = 0;
  }
  n4_10 = (uint16_t *)v16[1];
  if ( n4_10 == (uint16_t *)__frame.sym0 )
  {
    __decode_pixel_n15 += 75;
  }
  else if ( n4_10 == (uint16_t *)__frame.sym1 )
  {
    __decode_pixel_n15 += 150;
  }
  else if ( n4_10 == (uint16_t *)__frame.sym3 )
  {
    __decode_pixel_n15 += 225;
  }
  else if ( n4_10 == (uint16_t *)__frame.sym2 )
  {
    __decode_pixel_n15 += 300;
  }
  v21 = this_2->f56[8];
  __frame.sym3 = (int32_t)n15_9;
  v22 = this_2->ctx_bucket[v15 + __decode_pixel_n15];
  v23 = this_2->f56[7];
  this_2->f40 = v22;
  n4_11 = *((uint8_t *)n15_9 + 2);
  __frame.sym4 = (uint64_t *)(v4);
  __frame.sym5 = (ModelBlock *)(this_2);
  __frame.sym2 = n4_11;
  v25 = this_2->f56[9];
  v26 = *((uint8_t *)v4 - 5);
  // `v186` is one stack slot with two roles: a row cursor here, and the
  // `uint16_t` value out of `v97[4]` at 11290.  Splitting it needs the frame
  // to dissolve first, so the cast records the double booking (§4.2).
  __frame.sym6 = (int32_t)v25;
  v27 = 8 * *((uint8_t *)v4 - 12) + 4 * *((uint8_t *)v4 - 9) + v26 + 2 * *((uint8_t *)v4 - 10);
  this_3 = (ModelBlock *)(__frame.sym5);
  v29 = ((uint8_t)(*(uint8_t *)(__frame.sym6 + 2) & *(v21 + 2) & __frame.sym2 & *(v23 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(__frame.sym6 + 3) & *(v21 + 3) & *(v23 + 3) & *((uint8_t *)n15_9 + 3)) << 8)
      + (v22 << 10)
      + v27;
  n15_10 = (uint8_t *)__frame.sym3;
  v31 = ((__frame.sym5->f1078692[3] == 0) << 7)
      + ((__frame.sym5->f1078692[2] == 0) << 6)
      + 32 * (__frame.sym5->f1078692[1] == 0)
      + 16 * (__frame.sym5->f1078692[0] == 0)
      + v29;
  n0xFFFF = __frame.sym5->f6075824[v31];
  if ( n0xFFFF == 0xFFFF )
  {
    __frame.sym5->f6075824[v31] = __frame.sym5->f20;
    n15_10 = this_3->f56[6];
    v4 = (uint64_t *)((uint16_t *)this_3->f56[5]);
    ++this_3->f20;
    n0xFFFF = this_3->f6075824[v31];
    __frame.sym2 = *(n15_10 + 2);
  }
  v33 = *((uint8_t *)v4 - 1) + 4 * *(n15_10 + 13) + 2 * __frame.sym2 + 8 * n0xFFFF;
  n0xFFFF_1 = this_3->f6460848[v33];
  if ( n0xFFFF_1 == 0xFFFF )
  {
    this_3->f6460848[v33] = this_3->f24++;
    n0xFFFF_1 = this_3->f6460848[v33];
  }
  if ( (int32_t)this_3->f16 < 32 )
  {
    n53248 = this_3->f28;
    __frame.sym1 = 16 * n0xFFFF_1 + (__frame.sym1 & 0xF);
    v36 = &this_3->f6678448[__frame.sym1];
    n0xFFFF_1 = *v36;
    if ( n0xFFFF_1 == 0xFFFF )
    {
      n4_12 = __frame.sym1;
      if ( n53248 > 53248 )
        n4_12 = __frame.sym1 | 0xF;
      v36 = &this_3->f6678448[n4_12];
      n0xFFFF_1 = *v36;
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *v36 = n53248;
      ++this_3->f28;
      n0xFFFF_1 = *v36;
    }
  }
  if ( (*(this_3->f56[5] - 5) & *(this_3->f56[5] - 6)) != 0 )
  {
    v38 = (uint8_t *)((uint8_t *)this_3->f56[6]);
    v39 = (uint8_t *)((uint8_t *)this_3->f56[7]);
    if ( ((uint8_t)(v39[19] & v39[11] & v39[3] & v38[27] & v38[19] & v38[11] & v38[3] & v38[2] & *((int8_t *)v38 - 5))
        & v39[27]) != 0 )
    {
      v40 = v39[2];
      n15_11 = 1;
      if ( this_3->f0 - a2 <= 1 )
      {
        n8 = 8;
      }
      else
      {
        __frame.sym1 = this_3->f0 - a2;
        __frame.sym5 = (ModelBlock *)(this_3);
        while ( 1 )
        {
          n8 = 8 * n15_11;
          if ( (((uint8_t *)v38)[8 * n15_11 + 19] & ((uint8_t *)v38)[8 * n15_11 + 18]) == 0 )
            break;
          v40 = (uint8_t)(((uint8_t *)v39)[n8 + 2] & v40);
          if ( ++n15_11 >= __frame.sym1 )
          {
            this_3 = (ModelBlock *)(__frame.sym5);
            n8 = 8 * n15_11;
            goto LABEL_42;
          }
        }
        this_3 = (ModelBlock *)(__frame.sym5);
      }
LABEL_42:
      n15_12 = *(this_3->f1078684 + n15_11);
      // Record `8 * n15_12 + 4 * (two neighbour flags) + 2 * v40 + sym + 1`
      // of the 257-record grid: `269089 * 4` is +1 076 356, four bytes past
      // `f1076352`, and every term above it is a multiple of three words.
      v44 = __decode_context_bit(
            &this_3->f1076352[3 * (8 * n15_12
                                   + 4 * (uint8_t)(((uint8_t *)v39)[n8 + 27] & ((uint8_t *)v39)[n8 + 19])
                                   + 2 * v40
                                   + *(this_3->f1078688 + __frame.sym0)
                                   + 1)],
            this_3->f1076352);
      n4_22 = ::mode_symbol[1];
      v46 = (uint8_t *)this_3->f1078688;
      this_3->f32 = v44;
      *(v46 + n4_22) = v44;
      n15_14 = this_3->f32;
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
            v51 = *((uint16_t *)&((uint32_t *)__frame.sym5)[24 * (v50 == 0) + 269473 + 24 * (n15_12 == __frame.sym0)] + 3 * n15_12 + 1);
            __frame.sym2 = (int32_t)&((uint32_t *)__frame.sym5)[24 * (v50 == 0) + 269473 + 24 * (n15_12 == __frame.sym0)] + 6 * n15_12 + 2;
            bin_tot = v51 + *((uint16_t *)&((uint32_t *)__frame.sym5)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == __frame.sym0)] + 3 * n15_12);
            __frame.sym1 = rc.decode_bit(
                     v51,
                     *((uint16_t *)&((uint32_t *)__frame.sym5)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == __frame.sym0)] + 3 * n15_12));
            if ( *((uint16_t *)&((uint32_t *)__frame.sym5)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == __frame.sym0)] + 3 * n15_12 + 1) < (uint32_t)bin_tot )
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
        *(uint16_t *)(this_3->f56[5] + 8 * n15_18 - 8) = *(uint16_t *)(this_3->f56[5] - 8);
      n15_14 = this_3->f32;
LABEL_57:
      if ( n15_18 > n15_14 )
      {
        v53 = this_3->f56[7];
        this_3->f56[6] = this_3->f56[6] + 8 * n15_18 - 8 * n15_14;
        v54 = this_3->f56[8];
        v55 = v53 + 8 * n15_18;
        v56 = this_3->f56[9];
        this_3->f56[7] = v55 - 8 * n15_14;
        this_3->f56[8] = v54 + 8 * n15_18 - 8 * n15_14;
        v57 = this_3->f56[5];
        this_3->f56[9] = v56 + 8 * n15_18 - 8 * n15_14;
        *(uint32_t *)(v57 + 4) = 0x01010101;
        *(uint32_t *)this_3->f56[5] = 0x01010101;
        v58 = (uint16_t *)this_3->f6059436;
        LOWORD(v55) = ::mode_symbol[1];
        LOWORD(v54) = *v58;
        __frame.sym1 = ::mode_symbol[1];
        v58[1] = (uint16_t)(uintptr_t)v54;
        *(uint16_t *)this_3->f56[5] = (uint16_t)(uintptr_t)v55;
        *(uint16_t *)this_3->f6059436 = (uint16_t)(uintptr_t)v55;
        v59 = (int32_t *)this_3->f56[5];
        v60 = v59[1];
        __frame.sym3 = *v59;
        v61 = (uint8_t *)(int32_t)(v59 + 2);
        this_3->f56[5] = v61;
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
              *(uint16_t *)(this_3->f6059436 + 2) = n4_14;
              *(uint32_t *)this_3->f56[5] = n15_13;
              *(uint32_t *)(this_3->f56[5] + 4) = v60;
              v66 = (uint8_t *)(uint32_t)this_3->f6059436;
              this_3->f56[5] += 8;
              *(uint16_t *)(v66 + 2) = n4_14;
              *(uint32_t *)this_3->f56[5] = n15_13;
              *(uint32_t *)(this_3->f56[5] + 4) = v60;
              v61 = this_3->f56[5] + 8;
              this_3->f56[5] = v61;
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
            *(uint16_t *)(this_3->f6059436 + 2) = __frame.sym1;
            *(uint32_t *)this_3->f56[5] = n15_15;
            *(uint32_t *)(this_3->f56[5] + 4) = v60;
            v61 = this_3->f56[5] + 8;
            this_3->f56[5] = v61;
          }
        }
        n15_24 = n15_18;
        n15_16 = (uint8_t *)this_3->f56[6];
        v70 = *(n15_16 - 22);
        v71 = *(n15_16 - 14);
        v72 = n15_16[18];
        v73 = n15_16[26];
        __frame.sym0 = (int32_t)n15_16;
        v74 = v71 + v70;
        v75 = n15_16[34];
        v76 = ((uint8_t *)this_3->f56[7]);
        this_3->f1078692[0] = v73 + v72 + v74 + v75 - 5;
        this_3->f1078692[1] = v76[26]
                                     + v76[18]
                                     + v76[10]
                                     + v76[2]
                                     + *((int8_t *)v76 - 6)
                                     + *((int8_t *)v76 - 14)
                                     + *((int8_t *)v76 - 22)
                                     + v76[34]
                                     - 8;
        n15_17 = (uint8_t *)__frame.sym0;
        this_3->f1078692[2] = *(v61 - 29) + *(v61 - 21) - 2;
        n4_17 = __frame.sym1;
        this_3->f1078692[3] = *(v61 - 38)
                                     + *(v61 - 46)
                                     + *(v61 - 54)
                                     + *(v61 - 30)
                                     - 4;
        *(v61 - 2) = n4_17 == *(uint16_t *)(n15_17 + 8);
        n15_18 = n15_24;
        *(this_3->f56[5] - 1) = n4_17 == *(uint16_t *)(this_3->f56[6] + 16);
        n15_14 = this_3->f32;
      }
      if ( n15_14 )
        return n15_18;
      goto LABEL_86;
    }
  }
  v80 = (uint64_t *)((int32_t)(uint32_t *)&this_3->f56[4 * this_3->f40 + 10]);
  __frame.sym4 = (uint64_t *)((uint16_t *)v80);
  v81 = 4 * n0xFFFF_1;
  freq_tbl = (uint16_t *)&((uint32_t *)this_3)[v81 + 776];
  v83 = HIWORD(((uint32_t *)this_3)[v81 + 778]);
  if ( HIWORD(((uint32_t *)this_3)[v81 + 778]) )
  {
    if ( v83 == 1 )
    {
      v142 = (uint64_t *)((uint64_t *)v80);
      v143 = *((uint8_t *)&v80[1] + 7);
      n4_18 = v143 * LOWORD(((uint32_t *)this_3)[v81 + 777]);
      n15_19 = v143 * freq_tbl[3];
      __frame.sym1 = v143 * *freq_tbl;
      v146 = v143 * freq_tbl[1];
      __frame.sym2 = n4_18;
      n15_20 = v143 * freq_tbl[4];
      __frame.sym0 = n15_19;
      __frame.sym3 = n15_20;
      *(uint64_t *)freq_tbl = *v142;
      *((uint64_t *)freq_tbl + 1) = v142[1];
      v148 = freq_tbl[5];
      v149 = *freq_tbl;
      *((uint8_t *)freq_tbl + 14) *= 8;
      __frame.sym5 = (ModelBlock *)(this_3);
      v150 = 21 * freq_tbl[1];
      __frame.sym1 += (21 * v149 + v148 - 1) / v148;
      *freq_tbl = __frame.sym1;
      v151 = (v150 + v148 - 1) / v148;
      v152 = 21 * freq_tbl[2];
      v153 = freq_tbl[3];
      v154 = v151 + v146;
      freq_tbl[1] = v154;
      v155 = 21 * v153;
      n15_21 = __frame.sym0;
      v157 = (uint8_t *)((v152 + v148 - 1) / v148 + __frame.sym2);
      freq_tbl[2] = (uint16_t)(uintptr_t)v157;
      v158 = (v155 + v148 - 1) / v148 + n15_21;
      v159 = 21 * freq_tbl[4];
      freq_tbl[3] = v158;
      v160 = v158 + (uint16_t)(uintptr_t)v157 + v154;
      this_3 = (ModelBlock *)(__frame.sym5);
      v161 = (v159 + v148 - 1) / v148 + __frame.sym3;
      freq_tbl[4] = v161;
      v162 = __frame.sym1 + v161 + v160;
      v83 = v162;
      freq_tbl[5] = v162;
    }
    arg_tot = v83;
    v84 = rc.get_freq(arg_tot);
    v85 = *freq_tbl;
    if ( v85 <= v84 )
    {
      v85 += freq_tbl[1];
      if ( v85 <= v84 )
      {
        v85 += freq_tbl[2];
        if ( v85 <= v84 )
        {
          v85 += freq_tbl[3];
          if ( v85 <= v84 )
          {
            v85 += freq_tbl[4];
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
    n256_2 = freq_tbl[6];
    arg_high = v85;
    n15_1 = *((uint8_t *)freq_tbl + 15);
    arg_cum = v85 - freq_tbl[n4];
    n256_1 = freq_tbl[5];
    if ( n256_1 > n256_2 && (freq_tbl[n4] + n15_1 + 8 < n256_1 || freq_tbl[5] > 0x4000u) )
    {
      v136 = freq_tbl[2];
      __frame.sym0 = n256_2;
      v137 = freq_tbl[1];
      __frame.sym5 = (ModelBlock *)(this_3);
      v138 = *freq_tbl;
      __frame.sym1 = n4;
      __frame.sym2 = n15_1;
      LOWORD(v138) = v138 - (v138 >> 1);
      *freq_tbl = v138;
      LOWORD(v137) = v137 - (v137 >> 1);
      freq_tbl[1] = v137;
      LOWORD(v136) = v136 - (v136 >> 1);
      v139 = freq_tbl[3];
      freq_tbl[2] = v136;
      LOWORD(v139) = v139 - (v139 >> 1);
      v140 = freq_tbl[4];
      freq_tbl[3] = v139;
      LOWORD(v140) = v140 - (v140 >> 1);
      freq_tbl[4] = v140;
      LOWORD(v137) = v139 + v136 + v137;
      n15_1 = __frame.sym2;
      LOWORD(v140) = v138 + v140;
      this_3 = (ModelBlock *)(__frame.sym5);
      n256_1 = (uint16_t)(v140 + v137);
      n256 = __frame.sym0;
      n4 = __frame.sym1;
      freq_tbl[5] = n256_1;
      if ( n256 < 256 && !*((uint8_t *)freq_tbl + 14) )
      {
        n256 = 256;
        freq_tbl[6] = 256;
      }
      if ( n256_1 > n256 )
      {
        if ( n15_1 < 15 )
          LOWORD(n15_1) = 15;
        *((uint8_t *)freq_tbl + 15) = n15_1;
      }
    }
    freq_tbl[5] = n15_1 + n256_1;
    freq_tbl[n4] += n15_1;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3->f32 = n4;
    if ( *((uint8_t *)freq_tbl + 14) )
    {
      --*((uint8_t *)freq_tbl + 14);
      v90 = (uint64_t *)(__frame.sym4);
      ++*(uint16_t *)((uint8_t *)&__frame.sym4[1] + 2);
      ++((uint16_t *)v90)[n4];
      n4 = this_3->f32;
    }
  }
  else
  {
    arg_tot = *(uint16_t *)((uint8_t *)&v80[1] + 2);
    v163 = rc.get_freq(arg_tot);
    v164 = *(uint16_t *)&*__frame.sym4;
    if ( v164 <= v163 )
    {
      v164 += *(uint16_t *)((uint8_t *)&*__frame.sym4 + 2);
      if ( v164 <= v163 )
      {
        v164 += *(uint16_t *)((uint8_t *)&*__frame.sym4 + 4);
        if ( v164 <= v163 )
        {
          v164 += *(uint16_t *)((uint8_t *)&*__frame.sym4 + 6);
          if ( v164 <= v163 )
          {
            v164 += *(uint16_t *)&__frame.sym4[1];
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
    n15_22 = *((uint8_t *)__frame.sym4 + 15);
    arg_high = v164;
    arg_cum = v164 - ((uint16_t *)__frame.sym4)[n4_2];
    n256_4 = *(uint16_t *)((uint8_t *)&__frame.sym4[1] + 2);
    n256_5 = *(uint16_t *)((uint8_t *)&__frame.sym4[1] + 4);
    __frame.sym3 = n15_22;
    if ( n256_4 > n256_5 && (((uint16_t *)__frame.sym4)[n4_2] + __frame.sym3 + 8 < n256_4 || n256_4 > 0x4000) )
    {
      __frame.sym0 = n256_5;
      __frame.sym5 = (ModelBlock *)(this_3);
      __frame.sym1 = (int32_t)freq_tbl;
      __frame.sym2 = n4_2;
      v171 = (uint64_t *)(__frame.sym4);
      v172 = *(uint16_t *)((uint8_t *)&*__frame.sym4 + 2);
      v173 = *(uint16_t *)((uint8_t *)&*__frame.sym4 + 4);
      v174 = *(uint16_t *)&*__frame.sym4 - (*(uint16_t *)&*__frame.sym4 >> 1);
      *(uint16_t *)&*__frame.sym4 = v174;
      LOWORD(v172) = v172 - (v172 >> 1);
      *(uint16_t *)((uint8_t *)&*v171 + 2) = v172;
      v175 = v173 - (v173 >> 1);
      v176 = (uint16_t)*(uint16_t *)((uint8_t *)&*v171 + 6);
      *(uint16_t *)((uint8_t *)&*v171 + 4) = v175;
      LOWORD(v176) = v176 - (v176 >> 1);
      v177 = (uint16_t)*(uint16_t *)&v171[1];
      *(uint16_t *)((uint8_t *)&*v171 + 6) = v176;
      LOWORD(v177) = v177 - (v177 >> 1);
      *(uint16_t *)&v171[1] = v177;
      LOWORD(v177) = v174 + v177;
      this_3 = (ModelBlock *)(__frame.sym5);
      n256_4 = (uint16_t)(v177 + v176 + v175 + v172);
      n256_3 = __frame.sym0;
      freq_tbl = (uint16_t *)__frame.sym1;
      *(uint16_t *)((uint8_t *)&v171[1] + 2) = n256_4;
      n4_2 = __frame.sym2;
      if ( n256_3 < 256 && !*((uint8_t *)__frame.sym4 + 14) )
      {
        n256_3 = 256;
        *(uint16_t *)((uint8_t *)&__frame.sym4[1] + 4) = 256;
      }
      if ( n256_4 > n256_3 )
      {
        n15_2 = __frame.sym3;
        if ( __frame.sym3 < 15 )
          n15_2 = 15;
        __frame.sym3 = n15_2;
        *((uint8_t *)__frame.sym4 + 15) = n15_2;
      }
    }
    n15_4 = __frame.sym3;
    v170 = (uint64_t *)(__frame.sym4);
    *(uint16_t *)((uint8_t *)&__frame.sym4[1] + 2) = __frame.sym3 + n256_4;
    ((uint16_t *)v170)[n4_2] += n15_4;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3->f32 = n4_2;
    freq_tbl[5] = freq_tbl[n4_2]++ != 0;
    n4 = this_3->f32;
  }
  if ( n4 )
  {
    *(uint16_t *)this_3->f56[5] = mode_symbol[n4];
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
  v95 = (uint16_t *)this_3->f6059436;
  exclusion_mask[n15_23] = v91;
  exclusion_mask[n4_20] = v91;
  exclusion_mask[n4_19] = v91;
  __byte_445440[0] = v91;
  this_3->sel[0] = nullptr;
  n4_5 = v95[1];
  __frame.sym0 = *v95;
  v97 = ((uint16_t *)this_3->f6059432);
  n4_6 = v97[0];
  n15_5 = v97[1];
  v100 = (uint64_t *)((uint16_t *)v97[2]);
  __frame.sym1 = n4_5;
  this_4 = (ModelBlock *)(v97[3]);
  __frame.sym2 = n4_6;
  v102 = v97[4];
  __frame.sym3 = n15_5;
  v103 = v97[5];
  __frame.sym4 = (uint64_t *)(v100);
  v104 = v97[6];
  v105 = v97[7];
  __frame.sym5 = (ModelBlock *)((uint32_t *)this_4);
  v106 = ((uint16_t *)this_3->f56[6]);
  __frame.sym6 = v102;
  v107 = v106[8];
  __frame.sym7 = v103;
  v108 = this_3->f56[5];
  __frame.sym8 = v104;
  v109 = *(uint16_t *)(v108 - 16);
  __frame.sym9 = v105;
  __frame.sym10 = v109;
  v110 = (uint16_t *)this_3->f56[7];
  v111 = v110[4];
  __frame.sym11 = v107;
  v112 = *v110;
  __frame.sym12 = v111;
  v113 = v106[-8];
  __frame.sym13 = v112;
  v114 = *(v110 - 4);
  __frame.sym14 = v113;
  v115 = *(uint16_t *)(v108 - 24);
  __frame.sym15 = v114;
  v116 = v106[12];
  __frame.sym16 = v115;
  v117 = v106[16];
  __frame.sym17 = v116;
  v118 = *(uint16_t *)(v108 - 32);
  __frame.sym18 = v117;
  v119 = v106[-12];
  __frame.sym19 = v118;
  v120 = v110[8];
  __frame.sym20 = v119;
  __frame.sym21 = v120;
  v121 = (uint16_t *)this_3->f56[8];
  __frame.sym22 = *v121;
  __frame.sym23 = *(v110 - 8);
  v122 = *(uint16_t *)(v108 - 40);
  v123 = *(uint16_t *)(v108 - 56);
  __frame.sym24 = v122;
  __frame.sym25 = v121[4];
  v124 = v106[20];
  v125 = v106[28];
  __frame.sym26 = v124;
  __frame.sym27 = *(uint16_t *)this_3->f56[9];
  __frame.sym28 = v123;
  __frame.sym29 = *(v121 - 4);
  v126 = v110[12];
  this_3->sym_pos = 0;
  __frame.sym30 = v125;
  __frame.sym31 = v126;
  do
  {
    v127 = __pixel_context((ModelBlock *)this_3, (uint32_t *)__frame.sym);
    if ( v127 >= 0 )
    {
      v128 = __decode_context_bit(&this_3->f1051776[3 * this_3->f52], &this_3->f1051680[3 * this_3->f48]);
      *(uint16_t *)this_3->f56[5] = v127;
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
  v132 = this_3->f1078208;
  this_3->sel[0] = &this_3->f1078212[::mode_symbol[2]];
  v133 = &v132[n4_21];
  v134 = this_3->f1078232;
  this_3->sel[1] = v133;
  while ( 1 )
  {
    if ( (*v134)->live )
    {
      v135 = __decode_symbol_list(*v134);
      *(uint16_t *)this_3->f56[5] = v135;
      if ( v135 >= 0 )
        return n15_25 + 1;
      v134 = this_3->f1078232;
    }
    this_3->f1078232 = ++v134;
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
            uint16_t *sym9;
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
  uint8_t *v20, *v22, *n2_9, *v52, *v109;   // row cursors out of ModelBlock
  uint8_t *v59, *v74, *n15_36, *n15_38, *n15_40;
  bool v11;
  int8_t v93;
  uint8_t v24, v64, v65, v66, v67, v68;
  uint8_t *v156;   // `uint8_t *` beside the `char` scalars above
  uint16_t *v36;   // a cursor into `f6678448`
  int16_t v14, n15_10, v147, v158, n15_21, v170;
  ModelBlock *this_3;
  ModelBlock *this_4;
  ModelBlock *this_2;
  int32_t arg_cum, arg_high, arg_tot, n4, n4_1, __code_pixel_n15, n15_1, v8, v9, v10, v13, v15,
          *v16, n15_6, n15_5, n15_8, v21, p_n15_1, n15_30, v26, v27, v29, n0xFFFF, v32,
          n0xFFFF_1, n53248, v35, v38, v39, v42, p_n15_2, n15_42, n8, v46, n15_32, n15_12,
          n15_9, v50, v53, p_n15_4, n2_10, v61, n15_11, n15_13, n4_2, n15_33, n15_35, n15_34,
          n15_15, n2_15, v82, __code_pixel_n0x2000, n15_16, v87, n2, p_n15_5, p_n15_7,
          n15_17, n15_22, excl_sym_a, excl_sym_b, p_n15_11, n2_5, n15_41, n15_23,
          n15_24, n15_25, n15_26, n15_27, v112, v114, v115, v116, v117, v118, v119, v120,
          v121, v122, v123, v125, v126, v127, v128, v129, n15_28, n32, n15_29, n4_5,
          p_n15_6, *v143, v144, n2_16, n2_17, n15_37, v149, v150, v151, v152, v154, v155, v157,
          n15_18, v161, n2_2, p_n15_10, p_n15_8, p_n15_9, n15_19;
  uint16_t *n2_3;   // was int32_t *, read only as uint16_t
  uint16_t *v159;
  uint16_t *v111;
  uint16_t *v51;
  uint16_t *v37;
  uint16_t *v108;
  uint16_t *n2_7, *n2_8, *n2_11, *n2_12, *n2_14, *v97, *v98, *n2_6, *v113, *v124, v153,
           *n15_39;
  uint8_t *n2_13;   // was uint16_t *, read only as uint8_t
  SymList *v134, *v135;
  SymList **v136;
  uint32_t bin_tot, v55, v57, v137, p_n15_12, v139, v140, v141, v168, v169, v171, v172,
          v173;
  uint8_t *v69;
  uint8_t *p_n15_3, *v41;
  n2_7 = (uint16_t *)*(int32_t *)&_this->f56[6];
  n4 = *n2_7;
  n2_8 = (uint16_t *)*(int32_t *)&_this->f56[5];
  n4_1 = (uint16_t)*(n2_8 - 4);
  __code_pixel_n15 = n2_7[4];
  __frame.sym7 = (ModelBlock *)(_this);
  n15_1 = *(n2_7 - 4);
  __frame.sym8 = n4;
  ::mode_symbol[1] = n4;
  __frame.sym10 = n4_1;
  ::mode_symbol[2] = n4_1;
  __frame.sym6 = __code_pixel_n15;
  ::mode_symbol[3] = __code_pixel_n15;
  v8 = *((uint8_t *)n2_7 + 3) + 4 * (__code_pixel_n15 == n15_1);
  v9 = *((uint8_t *)n2_7 + 11);
  __frame.sym5 = n15_1;
  mode_symbol[4] = n15_1;
  v10 = 32 * *((uint8_t *)n2_8 - 4)
      + 16 * *((uint8_t *)n2_8 - 2)
      + 2 * *((uint8_t *)n2_8 - 6)
      + 8 * v9
      + v8;
  v11 = n4 == n4_1;
  this_2 = (ModelBlock *)(__frame.sym7);
  if ( v11 )
  {
    if ( __frame.sym8 == __frame.sym6 )
    {
      v14 = *((uint16_t *)__frame.sym7 + __frame.sym8 + 3029720);
      if ( __frame.sym8 == __frame.sym5 )
        v13 = (uint16_t)(v14 - *(n2_8 - 8));
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
  __frame.sym7->f6059432 = (uint32_t)&((int32_t *)__frame.sym7)[4 * v13 + 269674];
  v15 = this_2->ctx_state[v10];
  *(int32_t *)&this_2->f36 = v15;
  v16 = &((int32_t *)this_2)[0x10000 * v15 + 531818 + v13];
  this_2->f6059436 = (uint8_t *)v16;
  n15_6 = *(uint16_t *)v16;
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
  n15_8 = *((uint16_t *)v16 + 1);
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
  v20 = this_2->f56[8];
  __frame.sym1 = (int32_t)n2_7;
  v21 = this_2->ctx_bucket[v15 + n15_5];
  v22 = this_2->f56[7];
  *(int32_t *)&this_2->f40 = v21;
  p_n15_1 = *((uint8_t *)n2_7 + 2);
  v24 = *((uint8_t *)n2_7 + 3);
  __frame.sym2 = n2_8;
  __frame.sym7 = (ModelBlock *)(this_2);
  __frame.sym0 = p_n15_1;
  n15_30 = *(int32_t *)&this_2->f56[9];
  v26 = *((uint8_t *)n2_8 - 5);
  __frame.sym3 = n15_30;
  v27 = 8 * *((uint8_t *)n2_8 - 12)
      + 4 * *((uint8_t *)n2_8 - 9)
      + v26
      + 2 * *((uint8_t *)n2_8 - 10);
  this_3 = (ModelBlock *)(__frame.sym7);
  v29 = ((__frame.sym7->f1078692[3] == 0) << 7)
      + ((__frame.sym7->f1078692[2] == 0) << 6)
      + 32 * (__frame.sym7->f1078692[1] == 0)
      + 16 * (__frame.sym7->f1078692[0] == 0)
      + ((uint8_t)(*(uint8_t *)(__frame.sym3 + 2) & *(v20 + 2) & __frame.sym0 & *(v22 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(__frame.sym3 + 3) & *(v20 + 3) & *(v22 + 3) & v24) << 8)
      + (v21 << 10)
      + v27;
  n0xFFFF = __frame.sym7->f6075824[v29];
  n2_9 = (uint8_t *)__frame.sym1;
  if ( n0xFFFF == 0xFFFF )
  {
    __frame.sym7->f6075824[v29] = *(int32_t *)&__frame.sym7->f20;
    n2_9 = this_3->f56[6];
    n2_8 = (uint16_t *)this_3->f56[5];
    ++*(int32_t *)&this_3->f20;
    n0xFFFF = this_3->f6075824[v29];
    __frame.sym0 = *(n2_9 + 2);
  }
  v32 = *((uint8_t *)n2_8 - 1) + 4 * *(n2_9 + 13) + 2 * __frame.sym0 + 8 * n0xFFFF;
  n0xFFFF_1 = this_3->f6460848[v32];
  if ( n0xFFFF_1 == 0xFFFF )
  {
    this_3->f6460848[v32] = (*(int32_t *)&this_3->f24)++;
    n0xFFFF_1 = this_3->f6460848[v32];
  }
  if ( *(int32_t *)&this_3->f16 < 32 )
  {
    n53248 = *(int32_t *)&this_3->f28;
    v35 = 16 * n0xFFFF_1 + (__frame.sym10 & 0xF);
    v36 = &this_3->f6678448[v35];
    n0xFFFF_1 = *v36;
    if ( n0xFFFF_1 == 0xFFFF )
    {
      if ( n53248 > 53248 )
        v35 |= 0xFu;
      v36 = &this_3->f6678448[v35];
      n0xFFFF_1 = *v36;
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *v36 = n53248;
      ++*(int32_t *)&this_3->f28;
      n0xFFFF_1 = *v36;
    }
  }
  v37 = ((uint16_t *)this_3->f56[5]);
  v38 = *((uint8_t *)v37 - 5);
  v39 = *((uint8_t *)v37 - 6);
  __frame.sym9 = (uint16_t *)(v37);
  if ( (v38 & v39) != 0
    && (p_n15_3 = (uint8_t *)this_3->f56[6],
        v41 = (uint8_t *)this_3->f56[7],
        ((uint8_t)(v41[19]
                         & v41[11]
                         & v41[3]
                         & p_n15_3[27]
                         & p_n15_3[19]
                         & p_n15_3[11]
                         & p_n15_3[3]
                         & p_n15_3[2]
                         & *(p_n15_3 - 5))
       & v41[27]) != 0) )
  {
    v42 = v41[2];
    p_n15_2 = *(int32_t *)&this_3->f0 - a2;
    __frame.sym4 = 1;
    if ( p_n15_2 <= 1 )
    {
      n8 = 8;
    }
    else
    {
      __frame.sym0 = p_n15_2;
      n15_42 = 1;
      __frame.sym7 = (ModelBlock *)(this_3);
      while ( 1 )
      {
        n8 = 8 * n15_42;
        if ( (p_n15_3[8 * n15_42 + 19] & p_n15_3[8 * n15_42 + 18]) == 0 )
          break;
        v42 = (uint8_t)(v41[n8 + 2] & v42);
        if ( ++n15_42 >= __frame.sym0 )
        {
          this_3 = (ModelBlock *)(__frame.sym7);
          __frame.sym4 = n15_42;
          n8 = 8 * n15_42;
          goto LABEL_42;
        }
      }
      this_3 = (ModelBlock *)(__frame.sym7);
      __frame.sym4 = n15_42;
    }
LABEL_42:
    v46 = (uint8_t)(v41[n8 + 27] & v41[n8 + 19]);
    __frame.sym5 = *(uint8_t *)(*(int32_t *)&this_3->f1078684 + __frame.sym4);
    n15_32 = *(uint8_t *)(*(int32_t *)&this_3->f1078688 + __frame.sym8) + 8 * __frame.sym5 + 4 * v46 + 2 * v42;
    n15_12 = 0;
    if ( *__frame.sym9 == __frame.sym8 )
    {
      __frame.sym0 = (int32_t)p_n15_3;
      __frame.sym7 = (ModelBlock *)(this_3);
      do
        ++n15_12;
      while ( n15_12 < __frame.sym4 && ((uint16_t *)__frame.sym9)[4 * n15_12] == __frame.sym8 );
      this_3 = (ModelBlock *)(__frame.sym7);
      p_n15_3 = (uint8_t *)__frame.sym0;
    }
    n15_9 = n15_12 == __frame.sym4;
    __frame.sym6 = n15_9;
    if ( n15_12 > n15_9 )
    {
      this_3->f56[6] = &p_n15_3[8 * n15_12 + -8 * n15_9];
      this_3->f56[7] = &v41[8 * n15_12 + -8 * n15_9];
      v50 = *(int32_t *)&this_3->f56[9];
      this_3->f56[8] = this_3->f56[8] + 8 * n15_12 - 8 * n15_9;
      v51 = (uint16_t *)(__frame.sym9);
      *(int32_t *)&this_3->f56[9] = v50 + 8 * n15_12 - 8 * n15_9;
      *((uint32_t *)v51 + 1) = 0x01010101;
      *(uint32_t *)this_3->f56[5] = 0x01010101;
      *(uint16_t *)(this_3->f6059436 + 2) = *(uint16_t *)this_3->f6059436;
      LOWORD(v51) = __frame.sym8;
      *(uint16_t *)this_3->f56[5] = __frame.sym8;
      *(uint16_t *)this_3->f6059436 = (uint16_t)(uintptr_t)v51;
      v52 = this_3->f56[5];
      v53 = *(uint32_t *)v52;
      __frame.sym1 = *(uint32_t *)(v52 + 4);
      __frame.sym2 = (uint16_t *)(v52 + 8);
      this_3->f56[5] = v52 + 8;
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
            *(uint16_t *)(this_3->f6059436 + 2) = n15_10;
            *(uint32_t *)this_3->f56[5] = v53;
            *(uint32_t *)(this_3->f56[5] + 4) = n2_10;
            v59 = (uint8_t *)this_3->f6059436;
            this_3->f56[5] += 8;
            *(uint16_t *)(v59 + 2) = n15_10;
            *(uint32_t *)this_3->f56[5] = v53;
            *(uint32_t *)(this_3->f56[5] + 4) = n2_10;
            n2_11 = (uint16_t *)(this_3->f56[5] + 8);
            this_3->f56[5] = (uint8_t *)n2_11;
            ++v57;
          }
          while ( v57 < v55 );
          p_n15_4 = __frame.sym0;
          n15_32 = __frame.sym3;
          __frame.sym2 = n2_11;
          n15_12 = n15_14;
          v61 = 2 * v57 + 1;
        }
        else
        {
          v61 = 1;
        }
        if ( p_n15_4 > (uint32_t)(v61 - 1) )
        {
          *(uint16_t *)(this_3->f6059436 + 2) = __frame.sym8;
          *(uint32_t *)this_3->f56[5] = v53;
          *(uint32_t *)(this_3->f56[5] + 4) = __frame.sym1;
          n2_12 = (uint16_t *)(this_3->f56[5] + 8);
          this_3->f56[5] = (uint8_t *)n2_12;
          __frame.sym2 = n2_12;
        }
      }
      v63 = (PixRec *)this_3->f56[6];
      v64 = v63[-2].match[0];
      v65 = v63[2].match[0];
      v66 = v63[4].match[0];
      __frame.sym3 = n15_32;
      v67 = v63[-3].match[0];
      n15_14 = n15_12;
      v68 = v63[3].match[0] + v65 + v64 + v67 + v66 - 5;
      v69 = ((uint8_t *)this_3->f56[7]);
      this_3->f1078692[0] = v68;
      n2_13 = (uint8_t *)__frame.sym2;
      this_3->f1078692[1] = v69[26]
                                   + v69[18]
                                   + v69[10]
                                   + v69[2]
                                   + *((int8_t *)v69 - 6)
                                   + *((int8_t *)v69 - 14)
                                   + *((int8_t *)v69 - 22)
                                   + v69[34]
                                   - 8;
      this_3->f1078692[2] = n2_13[-29] + n2_13[-21] - 2;
      n15_11 = __frame.sym8;
      this_3->f1078692[3] = n2_13[-38]
                                   + n2_13[-46]
                                   + n2_13[-54]
                                   + n2_13[-30]
                                   - 4;
      n2_13[-2] = n15_11 == v63[1].sym;
      n15_32 = __frame.sym3;
      *(this_3->f56[5] - 1) = n15_11 == *(uint16_t *)(this_3->f56[6] + 16);
      n15_12 = n15_14;
    }
    __encode_context_bit(&this_3->f1076352[3 * (n15_32 + 1)], this_3->f1076352, __frame.sym6);
    n15_13 = __frame.sym6;
    n4_2 = ::mode_symbol[1];
    v74 = (uint8_t *)this_3->f1078688;
    *(int32_t *)&this_3->f32 = __frame.sym6;
    *(v74 + n4_2) = n15_13;
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
          n2_14 = (uint16_t *)&((int32_t *)__frame.sym7)[24 * (n15_34 == 0) + 269473 + 24 * (n15_15 == __frame.sym0)] + 3 * n15_15 + 1;
          __frame.sym2 = n2_14;
          __frame.sym1 = n15_14 & n15_35;
          bin_tot = *n2_14 + n2_14[1];
          rc.encode_bit(*n2_14, n2_14[1], (n15_14 & n15_35) != 0);
          if ( *((uint16_t *)&((int32_t *)__frame.sym7)[24 * (n15_34 == 0) + 269474 + 24 * (n15_15 == __frame.sym0)] + 3 * n15_15 + 1) < (uint32_t)bin_tot )
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
    if ( *(int32_t *)&this_3->f32 )
      return n15_12;
  }
  else
  {
    v82 = 4 * *(int32_t *)&this_3->f40;
    n15_36 = (uint8_t *)&this_3->f56[v82 + 10];
    __frame.sym4 = (int32_t)(uintptr_t)n15_36;
    n2_3 = (uint16_t *)&((int32_t *)this_3)[4 * n0xFFFF_1 + 776];
    __code_pixel_n0x2000 = HIWORD(((int32_t *)this_3)[4 * n0xFFFF_1 + 778]);
    if ( __code_pixel_n0x2000 )
    {
      if ( __code_pixel_n0x2000 == 1 )
      {
        v143 = (int32_t *)&this_3->f56[v82 + 10];
        v144 = *(n15_36 + 15);
        n2_16 = v144 * n2_3[2];
        n2_17 = v144 * n2_3[3];
        __frame.sym0 = v144 * *n2_3;
        v147 = v144 * n2_3[1];
        __frame.sym1 = n2_16;
        n15_37 = v144 * n2_3[4];
        __frame.sym2 = (uint16_t *)n2_17;
        __frame.sym3 = n15_37;
        *(uint64_t *)n2_3 = *(uint64_t *)v143;
        *((uint64_t *)n2_3 + 1) = *((uint64_t *)v143 + 1);
        v149 = n2_3[5];
        *((uint8_t *)n2_3 + 14) *= 8;
        __frame.sym7 = (ModelBlock *)(this_3);
        v150 = 21 * n2_3[1];
        __frame.sym0 += (21 * *n2_3 + v149 - 1) / v149;
        *n2_3 = __frame.sym0;
        v151 = (v150 + v149 - 1) / v149;
        LOWORD(v150) = __frame.sym1;
        v152 = 21 * n2_3[2];
        v153 = v151 + v147;
        n2_3[1] = v153;
        v154 = (v152 + v149 - 1) / v149;
        v155 = 21 * n2_3[3];
        LOWORD(v150) = v154 + v150;
        n2_3[2] = v150;
        v156 = (uint8_t *)__frame.sym2 + (v155 + v149 - 1) / v149;
        v157 = 21 * n2_3[4];
        n2_3[3] = (uint16_t)(uintptr_t)v156;
        v158 = (uint16_t)(uintptr_t)v156 + v150 + v153;
        this_3 = (ModelBlock *)(__frame.sym7);
        LOWORD(v149) = (v157 + v149 - 1) / v149 + __frame.sym3;
        n2_3[4] = v149;
        __code_pixel_n0x2000 = (uint16_t)(__frame.sym0 + v149 + v158);
        n2_3[5] = __code_pixel_n0x2000;
        __frame.sym9 = ((uint16_t *)this_3->f56[5]);
      }
      n15_16 = *__frame.sym9;
      arg_tot = __code_pixel_n0x2000;
      if ( n15_16 == __frame.sym8 )
      {
        v87 = *n2_3;
        n2 = 1;
      }
      else if ( n15_16 == __frame.sym10 )
      {
        v87 = *n2_3 + n2_3[1];
        n2 = 2;
      }
      else if ( n15_16 == __frame.sym6 )
      {
        v87 = *n2_3 + n2_3[2] + n2_3[1];
        n2 = 3;
      }
      else if ( n15_16 == __frame.sym5 )
      {
        v87 = n2_3[5] - n2_3[4];
        n2 = 4;
      }
      else
      {
        v87 = 0;
        n2 = 0;
      }
      p_n15_5 = n2_3[6];
      arg_cum = v87;
      arg_high = n2_3[n2] + v87;
      p_n15_7 = n2_3[5];
      n15_17 = *((uint8_t *)n2_3 + 15);
      if ( p_n15_7 > p_n15_5
        && (n2_3[n2] + n15_17 + 8 < p_n15_7 || n2_3[5] > 0x4000u) )
      {
        v137 = n2_3[2];
        __frame.sym0 = p_n15_5;
        p_n15_12 = n2_3[1];
        __frame.sym7 = (ModelBlock *)(this_3);
        __frame.sym1 = n2;
        v139 = *n2_3;
        __frame.sym2 = (uint16_t *)n15_17;
        LOWORD(v139) = v139 - (v139 >> 1);
        *n2_3 = v139;
        LOWORD(p_n15_12) = p_n15_12 - (p_n15_12 >> 1);
        n2_3[1] = p_n15_12;
        LOWORD(v137) = v137 - (v137 >> 1);
        v140 = n2_3[3];
        n2_3[2] = v137;
        LOWORD(v140) = v140 - (v140 >> 1);
        v141 = n2_3[4];
        n2_3[3] = v140;
        LOWORD(v141) = v141 - (v141 >> 1);
        n2_3[4] = v141;
        LOWORD(p_n15_12) = v140 + v137 + p_n15_12;
        n15_17 = (int32_t)__frame.sym2;
        LOWORD(p_n15_12) = v139 + v141 + p_n15_12;
        n2 = __frame.sym1;
        this_3 = (ModelBlock *)(__frame.sym7);
        p_n15_7 = (uint16_t)p_n15_12;
        p_n15_6 = __frame.sym0;
        n2_3[5] = p_n15_7;
        if ( p_n15_6 < 256 && !*((uint8_t *)n2_3 + 14) )
        {
          p_n15_6 = 256;
          n2_3[6] = 256;
        }
        if ( p_n15_7 > p_n15_6 )
        {
          if ( n15_17 < 15 )
            LOWORD(n15_17) = 15;
          *((uint8_t *)n2_3 + 15) = n15_17;
        }
      }
      n2_3[5] = n15_17 + p_n15_7;
      n2_3[n2] += n15_17;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->f32 = n2;
      if ( *((uint8_t *)n2_3 + 14) )
      {
        --*((uint8_t *)n2_3 + 14);
        n15_38 = (uint8_t *)__frame.sym4;
        ++*(uint16_t *)(__frame.sym4 + 10);
        ++*(uint16_t *)(n15_38 + 2 * n2);
        n2 = *(int32_t *)&this_3->f32;
      }
    }
    else
    {
      v159 = ((uint16_t *)&this_3->f56[v82 + 10]);
      n15_18 = *__frame.sym9;
      arg_tot = *(uint16_t *)(n15_36 + 10);
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
      arg_high = *(uint16_t *)(__frame.sym4 + 2 * n2_2) + v161;
      p_n15_10 = *(uint16_t *)(__frame.sym4 + 10);
      p_n15_8 = *(uint16_t *)(__frame.sym4 + 12);
      __frame.sym3 = *(uint8_t *)(__frame.sym4 + 15);
      if ( p_n15_10 > p_n15_8 && (*(uint16_t *)(__frame.sym4 + 2 * n2_2) + __frame.sym3 + 8 < p_n15_10 || p_n15_10 > 0x4000) )
      {
        __frame.sym0 = p_n15_8;
        __frame.sym7 = (ModelBlock *)(this_3);
        n15_39 = (uint16_t *)__frame.sym4;
        v168 = *(uint16_t *)(__frame.sym4 + 2);
        v169 = *(uint16_t *)(__frame.sym4 + 4);
        __frame.sym1 = (int32_t)n2_3;
        __frame.sym2 = (uint16_t *)n2_2;
        v170 = *(uint16_t *)__frame.sym4 - (*(uint16_t *)__frame.sym4 >> 1);
        *(uint16_t *)__frame.sym4 = v170;
        LOWORD(v168) = v168 - (v168 >> 1);
        n15_39[1] = v168;
        v171 = v169 - (v169 >> 1);
        v172 = (uint16_t)n15_39[3];
        n15_39[2] = v171;
        LOWORD(v172) = v172 - (v172 >> 1);
        v173 = (uint16_t)n15_39[4];
        n15_39[3] = v172;
        LOWORD(v173) = v173 - (v173 >> 1);
        n15_39[4] = v173;
        LOWORD(v173) = v170 + v173;
        n2_2 = (int32_t)__frame.sym2;
        p_n15_10 = (uint16_t)(v173 + v172 + v171 + v168);
        p_n15_9 = __frame.sym0;
        n2_3 = (uint16_t *)(int32_t *)__frame.sym1;
        n15_39[5] = p_n15_10;
        this_3 = (ModelBlock *)(__frame.sym7);
        if ( p_n15_9 < 256 && !*(uint8_t *)(__frame.sym4 + 14) )
        {
          p_n15_9 = 256;
          *(uint16_t *)(__frame.sym4 + 12) = 256;
        }
        if ( p_n15_10 > p_n15_9 )
        {
          n15_19 = __frame.sym3;
          if ( __frame.sym3 < 15 )
            n15_19 = 15;
          __frame.sym3 = n15_19;
          *(uint8_t *)(__frame.sym4 + 15) = n15_19;
        }
      }
      n15_21 = __frame.sym3;
      n15_40 = (uint8_t *)__frame.sym4;
      *(uint16_t *)(__frame.sym4 + 10) = __frame.sym3 + p_n15_10;
      *(uint16_t *)(n15_40 + 2 * n2_2) += n15_21;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->f32 = n2_2;
      n2_3[5] = (n2_3[n2_2])++ != 0;
      n2 = *(int32_t *)&this_3->f32;
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
  v97 = (uint16_t *)this_3->f6059436;
  exclusion_mask[excl_sym_a] = v93;
  exclusion_mask[excl_sym_b] = v93;
  v98 = (uint16_t *)*(int32_t *)&this_3->f6059432;
  __byte_445440[0] = v93;
  n15_14 = n15_12;
  this_3->sel[0] = nullptr;
  p_n15_11 = *v97;
  n2_5 = v97[1];
  n2_6 = (uint16_t *)*v98;
  n15_41 = v98[2];
  __frame.sym0 = p_n15_11;
  n15_23 = v98[1];
  __frame.sym1 = n2_5;
  n15_24 = v98[3];
  __frame.sym2 = n2_6;
  n15_25 = v98[4];
  __frame.sym3 = n15_23;
  this_4 = (ModelBlock *)((int32_t *)v98[5]);
  __frame.sym4 = n15_41;
  n15_26 = v98[6];
  v108 = ((uint16_t *)v98[7]);
  __frame.sym5 = n15_24;
  v109 = this_3->f56[5];
  __frame.sym6 = n15_25;
  n15_27 = *(uint16_t *)(v109 - 16);
  __frame.sym7 = (ModelBlock *)(this_4);
  v111 = ((uint16_t *)this_3->f56[6]);
  __frame.sym8 = n15_26;
  v112 = v111[8];
  __frame.sym9 = (uint16_t *)(v108);
  __frame.sym10 = n15_27;
  v113 = (uint16_t *)this_3->f56[7];
  v114 = v113[4];
  __frame.sym11 = v112;
  v115 = *v113;
  __frame.sym12 = v114;
  v116 = v111[-8];
  __frame.sym13 = v115;
  v117 = *(v113 - 4);
  __frame.sym14 = v116;
  v118 = *(uint16_t *)(v109 - 24);
  __frame.sym15 = v117;
  v119 = v111[12];
  __frame.sym16 = v118;
  v120 = v111[16];
  __frame.sym17 = v119;
  v121 = *(uint16_t *)(v109 - 32);
  __frame.sym18 = v120;
  v122 = v111[-12];
  __frame.sym19 = v121;
  v123 = v113[8];
  __frame.sym20 = v122;
  __frame.sym21 = v123;
  v124 = (uint16_t *)this_3->f56[8];
  __frame.sym22 = *v124;
  __frame.sym23 = *(v113 - 8);
  v125 = *(uint16_t *)(v109 - 40);
  v126 = *(uint16_t *)(v109 - 56);
  __frame.sym24 = v125;
  __frame.sym25 = v124[4];
  v127 = v111[20];
  v128 = v111[28];
  __frame.sym26 = v127;
  __frame.sym27 = *(uint16_t *)*(int32_t *)&this_3->f56[9];
  __frame.sym28 = v126;
  __frame.sym29 = *(v124 - 4);
  __frame.sym30 = v128;
  __frame.sym31 = v113[12];
  *(int32_t *)&this_3->sym_pos = 0;
  do
  {
    v129 = __pixel_context((ModelBlock *)this_3, (uint32_t *)__frame.sym);
    if ( v129 >= 0 )
    {
      n15_28 = v129 == *(uint16_t *)this_3->f56[5];
      __encode_context_bit(&this_3->f1051776[3 * *(int32_t *)&this_3->f52], &this_3->f1051680[3 * *(int32_t *)&this_3->f48], n15_28);
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
  v134 = this_3->f1078208;
  this_3->sel[0] = &this_3->f1078212[::mode_symbol[2]];
  v135 = &v134[n4_5];
  v136 = this_3->f1078232;
  this_3->sel[1] = v135;
  while ( 1 )
  {
    if ( (*v136)->live )
    {
      if ( __encode_symbol_list(*v136, *(uint16_t *)this_3->f56[5]) )
        return n15_29 + 1;
      v136 = this_3->f1078232;
    }
    this_3->f1078232 = ++v136;
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
  uint8_t *v16;   // was int32_t: these hold addresses
  bool v24;
  int32_t n8, v4, v6, v9, n16_1, n16, v25, v27;
  uint32_t j_2, i, n8193, v8, *v10, j_1, j, v15, v17, v18, v19, v20, v21, *v22, v26;
  void *v12;
  n8 = _this->f8;
  j_2 = 0xFFFFFFFF >> (-*((uint8_t *)_this + 8) & 31);
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
  _this->f16 = v8 + 1;
  if ( (int32_t)(v8 + 1) <= 0x2000 )
  {
    v12 = bmf_new(4 * v8 + 4);
    j_1 = _this->f16;
    *(void **)&_this->f1078240 = v12;
    if ( j_1 )
    {
      for ( j = 0; j < j_1; ++j )
      {
        *(uint32_t *)(*(uint32_t *)&_this->f1078240 + 4 * j) = j;
        j_1 = _this->f16;
      }
    }
    if ( (int32_t)_this->f8 > 8 )
    {
      if ( 4 * v4 )
      {
        v15 = 0;
        do
          __init_symbol_list(&((SymList *)__frame.v28)[v15++], (int32_t)_this, 256, 1);
        while ( v15 < 4 * v4 );
        j_1 = _this->f16;
      }
      if ( j_1 )
      {
        v16 = (uint8_t *)*(void **)&_this->f1078240;
        v17 = 0;
        v18 = 0;
        do
        {
          *(uint32_t *)(v16 + 4 * v18) = 0;
          if ( v4 )
          {
            __frame.v31[0] = v4;
            v19 = 0;
            do
            {
              v20 = __decode_symbol_list(&((SymList *)__frame.v28)[4 * v19 + v17]);
              v21 = v20 << ((8 * v19) & 31);
              v17 = v20 >> 6;
              *(uint32_t *)(*(uint32_t *)&_this->f1078240 + 4 * v18) += v21;
              ++v19;
            }
            while ( v19 < __frame.v31[0] );
            v4 = __frame.v31[0];
          }
          v16 = (uint8_t *)*(void **)&_this->f1078240;
          v17 = *(v16 + 4 * v18++) >> 7;
        }
        while ( v18 < _this->f16 );
      }
    }
    else if ( j_1 <= j_2 )
    {
      __init_symbol_list((SymList *)__frame.v28, (int32_t)_this, j_2 - j_1 + 2, 1);
      v24 = _this->f16 == 0;
      __frame.v29 = 19 * ((SymList *)__frame.v28)->n;
      if ( !v24 )
      {
        v25 = 0;
        v26 = 0;
        do
        {
          v27 = __decode_symbol_list((SymList *)__frame.v28);
          *(uint32_t *)(*(uint32_t *)&_this->f1078240 + 4 * v26) = v27 + v25;
          v25 += v27 + 1;
          ++v26;
        }
        while ( v26 < _this->f16 );
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
    v9 = *(uint32_t *)&_this->f4 * v4;
    _this->f8 = 8;
    *(uint32_t *)&_this->f4 = v9;
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
  uint8_t *v8;
  uint8_t v12;
  int16_t v19;
  int32_t i_1, j, v9, v13, k_1, v35, v38;
  uint32_t k, m, n0x2000_1, n, n8, n0x18;
  uint8_t *v10;
  i_1 = i;
  exclusion_gen = 1;
  a1->f0 = i;
  *(uint32_t *)&a1->f4 = a4;
  a1->f8 = a5;
  a1->f12 = a5;
  a1->escape.ent = nullptr;
  *(uint32_t *)&a1->f1078240 = 0;
  for ( j = 0; j < 5; ++j )
  {
    v8 = (uint8_t *)bmf_new(8 * i_1 + 128);
    a1->f56[j] = v8;
    a1->f56[j + 5] = v8 + 64;
    i_1 = a1->f0;
    if ( (int32_t)a1->f0 > -16 )
    {
      v9 = 0;
      do
      {
        *(uint16_t *)(a1->f56[j] + 8 * v9) = 0;
        *(a1->f56[j] + 8 * v9 + 7) = 1;
        *(a1->f56[j] + 8 * v9 + 6) = 1;
        *(a1->f56[j] + 8 * v9 + 5) = 1;
        *(a1->f56[j] + 8 * v9 + 4) = 1;
        *(a1->f56[j] + 8 * v9 + 3) = 1;
        *(a1->f56[j] + 8 * v9 + 2) = 1;
        i_1 = a1->f0;
        ++v9;
      }
      while ( v9 < a1->f0 + 16 );
    }
  }
  v10 = (uint8_t *)bmf_new(i_1 + 1);
  a1->f1078684 = v10;
  *v10 = 0;
  if ( (int32_t)a1->f0 > 0 )
  {
    v12 = 0;
    v13 = 0;
    do
    {
      v12 += v13 == 2 << (v12 & 31);
      *(uint8_t *)(*(uint32_t *)&a1->f1078684 + v13++ + 1) = v12;
    }
    while ( v13 < a1->f0 );
  }
  // 0x2000 sixteen-bit counters cleared.  What was here instead was the same
  // range in three passes -- a scalar head to reach sixteen-byte alignment,
  // thirty-two counters an iteration, a scalar tail -- with a branch for the
  // case where `a1` is odd and no alignment is reachable at all.
  __builtin_memset(a1->f6059440, 0, sizeof a1->f6059440);
  for ( k = 0; k < 0x2000; ++k )
  {
    v19 = a1->f6059440[k];
    k_1 = k;
    for ( m = 0; m < 0xD; ++m )
    {
      v19 += v19 + (k_1 & 1);
      k_1 >>= 1;
    }
    a1->f6059440[k] = v19;
  }
  // ... and every one of them scaled by eight, the same range in the same
  // three passes.
  for ( n0x2000_1 = 0; n0x2000_1 < 0x2000; ++n0x2000_1 )
    a1->f6059440[n0x2000_1] *= 8;
  memset((uint8_t *)a1 + 3104,0,0x100000);
  a1->f28 = 0;
  a1->f24 = 0;
  a1->f20 = 0;
  memset(a1->f6075824,255,sizeof a1->f6075824);
  memset(a1->f6460848,255,sizeof a1->f6460848);
  memset(a1->f6678448,255,sizeof a1->f6678448);
  memset(exclusion_mask,0,8193);
  (*(uint64_t *)&a1->sel[0]) = 0;
  *(uint64_t *)((uint8_t *)a1 + 1078224) = 0;
  for ( n = 0; n < 0x40000; ++n )
  {
    a1->f1078696[2 * n] = 0x2000;
    a1->f1078696[2 * n + 1] = 0x2000;
  }
  n8 = 0;
  do
  {
    v35 = 6 * n8;
    a1->f1051680[v35] = 40;
    ++n8;
    a1->f1051680[v35 + 1] = 16;
    a1->f1051680[v35 + 2] = 512;
    a1->f1051680[v35 + 3] = 40;
    a1->f1051680[v35 + 4] = 16;
    a1->f1051680[v35 + 5] = 512;
  }
  while ( n8 < 8 );
  n0x18 = 0;
  memset(a1->f1051776,0,sizeof a1->f1051776);
  a1->f1078236 = (uint8_t *)bmf_new(2 * a1->f4 * a1->f0);
  a1->f1076352[0] = 4;
  a1->f1076352[1] = 4;
  a1->f1076352[2] = 72;
  memset(&a1->f1076352[3],0,1536);
  do
  {
    v38 = 6 * n0x18;
    a1->f1077894[v38] = 4;
    ++n0x18;
    a1->f1077894[v38 + 1] = 4;
    a1->f1077894[v38 + 2] = 72;
    a1->f1077894[v38 + 3] = 4;
    a1->f1077894[v38 + 4] = 4;
    a1->f1077894[v38 + 5] = 72;
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
  int32_t n4;
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
  uint8_t *ArgList_1;
  // These shared `__frame.ArgList_1` with the name that still binds it: one
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
  uint8_t *v57;   // were int32_t: these hold addresses
  bool v38;
  uint8_t *ArgList, *ArgList_2, *buf, *ArgList_3, *ArgList_9, *ArgList_10,
          *Src_2, *v77, *ArgList_8;
  int16_t v20;
  uint8_t *v27, *v28, *v29, *v44, *v45, *v46, *v47, *v48;   // row cursors out of f56
  int32_t v3, v5, v6, v8, n5, v11, v14, v15, v16, v17, v18, v19, n0x10000, v30,
          v34, v39, v40, n4_1, v43, v51, v52, v53, v54, v56, v58, v60, v61, v62,
          v64, v65, v66, n6, v68, v69, n6_4, n6_1, v73, n6_2, v76, v78, v80;
  ModelBlock *this_3;
  ModelBlock *this_2;
  uint32_t *v22, *v31, *v35, *ArgList_7, *ArgList_6;
  SymList *i_1, *i, *j_1, *j;
  uint16_t *v13;   // was uint32_t *, read only as uint16_t
  uint8_t *v49, *v50;
  v3 = _this->f8 < 8;
  Src_1 = (uint8_t *)Src;
  ArgList = &Src[-v3];
  __rc_begin_decode(0);
  __expand_alphabet((ModelBlock *)_this);
  ArgList_1 = ArgList;
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
        v11 = v102;
        this_3 = (ModelBlock *)(this_1);
        this_2->ctx_bucket[v5 + 15 * v8 + 75 * n5] = v102;
        v101 = this_3->f16;
        v13 = (uint16_t *)&((uint32_t *)this_3)[4 * v11];
        v13[49] = 2;
        v13[50] = 2;
        v13[51] = 2;
        v13[52] = 2;
        if ( v88 )
        {
          v14 = (uint16_t)(v13[52] + v13[51]);
          v13[51] = v14;
          v15 = 0;
          v13[52] = 0;
        }
        else
        {
          v14 = v13[51];
          v15 = v13[52];
        }
        if ( v99 )
        {
          v16 = (uint16_t)(v15 + v13[50]);
          v13[50] = v16;
          v15 = 0;
          v13[52] = 0;
        }
        else
        {
          v16 = v13[50];
        }
        if ( v95 )
        {
          v16 = (uint16_t)(v14 + v16);
          v13[50] = v16;
          v14 = 0;
          v13[51] = 0;
        }
        if ( v93 )
        {
          v13[49] += v15;
          v15 = 0;
          v13[52] = 0;
        }
        if ( v94 )
        {
          v13[49] += v14;
          v14 = 0;
          v13[51] = 0;
        }
        if ( ArgList_4 )
        {
          v13[49] += v16;
          v16 = 0;
          v13[50] = 0;
        }
        v17 = (v14 != 0) + (v16 != 0) + (v15 != 0) + 2;
        if ( v17 <= v101 )
        {
          *((uint8_t *)v13 + 110) = v17;
          v13[48] = 2;
        }
        else
        {
          LOBYTE(v17) = v17 - 1;
          *((uint8_t *)v13 + 110) = v17;
          v13[48] = 0;
        }
        if ( v13[v100 + 48] && v13[n5 + 48] && (uint8_t)v17 <= v101 )
        {
          v19 = v100;
          v18 = 1;
          v20 = (uint8_t)(1 << ((5 - v17) & 31));
          *((uint8_t *)v13 + 111) = v20;
          v13[54] = v20 << 6;
          v13[v19 + 48] += v20;
          v13[n5 + 48] += *((uint8_t *)v13 + 111);
          v13[53] = v13[48]
                               + v13[52]
                               + v13[51]
                               + v13[50]
                               + v13[49];
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
    v22 = &((uint32_t *)this_1)[0x10000 * v82];
    do
    {
      LOWORD(v22[n0x10000 + 531818]) = 0x2000;
      HIWORD(v22[n0x10000++ + 531818]) = 0x2000;
    }
    while ( n0x10000 < 0x10000 );
    v5 = v82 + 1;
  }
  while ( v82 + 1 < 15 );
  ArgList_2 = ArgList_1;
  this_4 = (ModelBlock *)((int32_t)this_1);
  buf = (uint8_t *)bmf_new(this_1->f16);
  Size = this_1->f16;
  this_1->f1078688 = (uint8_t *)buf;
  memset(buf,1,Size);
  v27 = this_4->f56[11];
  v28 = this_4->f56[12];
  this_4->f1051664[0] = this_4->f56[10];
  v29 = this_4->f56[13];
  this_4->f1051664[1] = v27;
  this_4->f1051664[2] = v28;
  this_4->f1051664[3] = v29;
  this_4->f1078224 = &this_4->escape;
  __init_symbol_list(&this_4->escape, (int32_t)this_4, this_4->f16, 1);
  this_4->f1078232 = this_4->sel;
  // `24 * n + 4`: the count word, then `n` lists.  `free_workspace` reads the
  // count back from `((uint32_t *)lists)[-1]`.
  v30 = this_4->f16;
  v31 = (uint32_t *)bmf_new(24 * v30 + 4);
  if ( v31 )
  {
    *v31 = v30;
    i_1 = (SymList *)(v31 + 1);
    for ( i = i_1; v30; --v30 )
      (i_1++)->ent = nullptr;
  }
  else
  {
    i = nullptr;
  }
  v34 = this_4->f16;
  this_4->f1078208 = i;
  v35 = (uint32_t *)bmf_new(24 * v34 + 4);
  if ( v35 )
  {
    *v35 = v34;
    j_1 = (SymList *)(v35 + 1);
    for ( j = j_1; v34; --v34 )
      (j_1++)->ent = nullptr;
  }
  else
  {
    j = nullptr;
  }
  v38 = this_4->f16 <= 0;
  this_4->f1078212 = j;
  if ( !v38 )
  {
    v39 = 0;
    do
    {
      __init_symbol_list(&this_4->f1078208[v39], (int32_t)this_4, 99, 0);
      __init_symbol_list(&this_4->f1078212[v39++], (int32_t)this_4, 33, 0);
    }
    while ( v39 < this_4->f16 );
  }
  v40 = this_4->f8;
  if ( v40 == this_4->f12 )
  {
    ArgList_3 = nullptr;
  }
  else
  {
    ArgList_2 = (uint8_t *)bmf_new(*(uint32_t *)&this_4->f4 * this_4->f0 + 3);
    v40 = this_4->f8;
    ArgList_3 = ArgList_2;
  }
  n4_1 = (v40 + 7) >> 3;
  if ( this_4->f4 > 0 )
  {
    n4 = n4_1;
    ArgList_4 = (int32_t)ArgList_3;
    ArgList_5 = ArgList_2;
    v43 = 0;
    while ( 1 )
    {
      v86 = v43;
      *(this_4->f56[5] + 3) = *(uint16_t *)(this_4->f56[5] - 8) == 0;
      *(this_4->f56[5] + 5) = *(uint16_t *)(this_4->f56[6] - 8) == 0;
      v44 = this_4->f56[4];
      v45 = this_4->f56[3];
      v46 = this_4->f56[2];
      v47 = this_4->f56[1];
      v48 = this_4->f56[0];
      this_4->f56[4] = v45;
      this_4->f56[3] = v46;
      this_4->f56[2] = v47;
      this_4->f56[1] = v48;
      this_4->f56[0] = v44;
      v44 += 56;
      this_4->f56[5] = v44;
      v48 += 56;
      this_4->f56[6] = v48;
      this_4->f56[7] = v47 + 56;
      this_4->f56[8] = v46 + 56;
      this_4->f56[9] = v45 + 56;
      // Two "is this count zero" flags, written to three and two places.
      // MSVC put each in the low byte of a register that held a cursor, which
      // is where `LOBYTE(v48) = ...` came from; neither cursor is read again.
      {
        uint8_t zero = *(uint16_t *)(v48 + 8) == 0;
        *(v44 + 4) = zero;
        *(this_4->f56[5] - 2) = zero;
        *(this_4->f56[5] - 9) = zero;
        zero = *(uint16_t *)(this_4->f56[6] + 16) == 0;
        *(this_4->f56[5] + 6) = zero;
        *(this_4->f56[5] - 1) = zero;
      }
      *(this_4->f56[5] + 7) = *(uint16_t *)(this_4->f56[6] + 24) == 0;
      v49 = *&this_4->f56[6];
      v50 = *&this_4->f56[7];
      this_4->f56[5] += 8;
      v49 += 8;
      this_4->f56[8] += 8;
      this_4->f56[6] = v49;
      v50 += 8;
      this_4->f56[7] = v50;
      this_4->f56[9] += 8;
      this_4->f1078692[0] = v49[26] + v49[18] + v49[10] + v49[2] + v49[34] - 5;
      // The same five counts as the line above, off the other row.  MSVC
      // spilled each byte into a register whose upper bits were leftovers,
      // and the destination is one byte, so only the low bytes ever counted.
      this_4->f1078692[3] = 0;
      this_4->f1078692[2] = 0;
      v51 = this_4->f0;
      this_4->f1078692[1] = v50[26] + v50[18] + v50[10] + v50[2] + v50[34] - 5;
      v52 = v86;
      if ( v51 <= 0 )
        break;
      v53 = 0;
      do
      {
        v54 = __decode_pixel((ModelBlock *)(uint32_t *)this_4, v53);
        __init_model_tables(this_4);
        v51 = this_4->f0;
        v53 += v54;
      }
      while ( v53 < this_4->f0 );
      v52 = v86;
      if ( n4 != 4 )
        goto LABEL_53;
      if ( v51 > 0 )
      {
        ArgList_6 = (uint32_t *)ArgList_5;
        v65 = 0;
        do
          *ArgList_6++ = *(uint32_t *)(this_4->f1078240
                                   + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v65++ + 64));
        while ( v65 < this_4->f0 );
        goto LABEL_73;
      }
LABEL_74:
      v43 = v52 + 1;
      if ( v43 >= *(uint32_t *)&this_4->f4 )
      {
        ArgList_3 = (uint8_t *)ArgList_4;
        goto LABEL_76;
      }
    }
    if ( n4 == 4 )
      goto LABEL_74;
LABEL_53:
    if ( n4 == 3 )
    {
      if ( v51 > 0 )
      {
        ArgList_7 = (uint32_t *)ArgList_5;
        v56 = 0;
        do
        {
          v57 = (uint8_t *)this_4->f1078240;
          v58 = *(uint16_t *)(this_4->f56[0] + 8 * v56 + 64);
          *(uint16_t *)ArgList_7 = *(uint16_t *)(v57 + 4 * v58);
          *((uint8_t *)ArgList_7 + 2) = *(v57 + 4 * v58 + 2);
          ++v56;
          ArgList_7 = (uint32_t *)((uint8_t *)ArgList_7 + 3);
        }
        while ( v56 < this_4->f0 );
        ArgList_5 = (uint8_t *)ArgList_7;
      }
      goto LABEL_74;
    }
    if ( n4 != 2 )
    {
      if ( this_4->f8 == 8 )
      {
        if ( v51 > 0 )
        {
          ArgList_8 = ArgList_5;
          v80 = 0;
          do
            *ArgList_8++ = *(this_4->f1078240
                                    + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v80++ + 64));
          while ( v80 < this_4->f0 );
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
          v64 = this_4->f8;
          v62 -= v64;
          if ( v62 < 0 )
          {
            v62 = 8 - v64;
            *++ArgList_9 = *(uint32_t *)(this_4->f1078240
                                     + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v61 + 64)) << ((8 - v64) & 31);
          }
          else
          {
            *ArgList_9 |= *(uint32_t *)(this_4->f1078240
                                    + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v61 + 64)) << (v62 & 31);
          }
          ++v61;
        }
        while ( v61 < this_4->f0 );
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
        *(uint16_t *)ArgList_6 = *(uint32_t *)(this_4->f1078240
                                        + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v60++ + 64));
        ArgList_6 = (uint32_t *)((uint8_t *)ArgList_6 + 2);
      }
      while ( v60 < this_4->f0 );
LABEL_73:
      ArgList_5 = (uint8_t *)ArgList_6;
      goto LABEL_74;
    }
    goto LABEL_74;
  }
LABEL_76:
  __rc_end_decode();
  v66 = this_4->f12;
  if ( v66 != this_4->f8 )
  {
    n6 = (v66 + 7) >> 3;
    if ( n6 <= 0 )
    {
      v68 = *(uint32_t *)&this_4->f4 * this_4->f0;
    }
    else
    {
      n6_3 = 0;
      v68 = *(uint32_t *)&this_4->f4 * this_4->f0;
      v69 = v68 / n6;
      if ( n6 >= 6 )
      {
        v84 = *(uint32_t *)&this_4->f4 * this_4->f0;
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
        v78 = *(uint32_t *)&this_4->f4 * this_4->f0;
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
  int32_t v96;
  uint8_t v97;
  int32_t v98;
  int32_t v99;
  void * Block_plane[4];
  AltP1Block * &v102 = (AltP1Block * &)Block_plane[1];
  AltP1Block * &v103 = (AltP1Block * &)Block_plane[2];
  AltP1Block * &v104 = (AltP1Block * &)Block_plane[3];
  int32_t v105;
  int32_t v106;
  uint32_t i_1;
  int32_t v108;
  int32_t v109;
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
  uint8_t *v28;   // a row cursor out of AltP1Block
  uint8_t v11, v12, v13, v62, v71, v81, v83;
  AltP1Block *v6;
  uint8_t *v24, *v25, *v26, *v27;   // row cursors out of AltP1Block
  int32_t i, v3, i_2, n4, *v7, v8, v9, v10, v14, v15, v16, v17, i_3, n4_1,
          n4_2, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42,
          v43, v44, v45, v46, v47, v48, v49, n5_9, n5_7, n5_2, n5_1, v56, n16,
          v58, v63, v64, v65, n16_1, n5_3, v69, n5_4, v73, v74, n16_2, v77,
          n5_5, v84, n16_3, n4_3, n4_4;
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
  i_1 = i;
  if ( plane_count > 0 )
  {
    i_2 = i;
    n4 = 0;
    do
    {
      v6 = (AltP1Block *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __alt_p1_alloc((AltP1Block *)v6, i_2, v3, n4);
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
  v106 = v14;
  v15 = v11 & 8;
  v90 = v10;
  v16 = v12 & 8;
  v17 = v13 & 8;
  __rc_begin_encode();
  if ( v3 > 0 )
  {
    v96 = v17;
    v19 = 0;
    v108 = v16;
    i_3 = i_1;
    v109 = v15;
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
          **(uint16_t **)&v23->cur[0] = *(uint16_t *)(v23->cur[0] - 2);
          *(uint16_t *)(v23->cur[0] + 2) = *(uint16_t *)(v23->cur[0] - 4);
          *(uint16_t *)(v23->cur[0] + 4) = *(uint16_t *)(v23->cur[0] - 6);
          *(uint16_t *)(v23->cur[0] + 6) = *(uint16_t *)(v23->cur[0] - 8);
          *(uint16_t *)(v23->cur[0] + 8) = *(uint16_t *)(v23->cur[0] - 10);
          *(uint16_t *)(v23->cur[0] + 10) = *(uint16_t *)(v23->cur[0] - 12);
          v24 = v23->f176[4];
          v25 = v23->f176[3];
          v26 = v23->f176[2];
          v27 = v23->f176[1];
          v28 = v23->f176[0];
          v23->f176[4] = v25;
          v23->f176[3] = v26;
          v23->f176[2] = v27;
          v23->f176[1] = v28;
          v23->f176[0] = v24;
          v24 += 8;
          v23->cur[0] = v24;
          v28 += 8;
          v23->cur[1] = v28;
          v23->cur[2] = v27 + 8;
          v23->cur[3] = v26 + 8;
          v23->cur[4] = v25 + 8;
          *(uint16_t *)(v24 - 8) = *(uint16_t *)(v28 + 6);
          *(uint16_t *)(v23->cur[0] - 6) = *(uint16_t *)(v23->cur[1] + 4);
          *(uint16_t *)(v23->cur[0] - 4) = *(uint16_t *)(v23->cur[1] + 2);
          *(uint16_t *)(v23->cur[0] - 2) = **(uint16_t **)&v23->cur[1];
          v29 = (uint8_t *)(*&v23->cur[2]);
          v30 = (uint8_t *)(*&v23->cur[4]);
          v23->f12[2] = 0;
          v23->f12[3] = 0;
          v23->f12[4] = 0;
          v31 = *((int8_t *)v29 - 3);
          v23->f12[3] = v31;
          v32 = *((int8_t *)v29 - 1);
          v23->f12[4] = v32;
          v33 = *((int8_t *)v30 - 3) + v31;
          v23->f12[3] = v33;
          v34 = *((int8_t *)v30 - 1) + v32;
          v23->f12[4] = v34;
          v35 = v29[1] + v33;
          v23->f12[3] = v35;
          v36 = v29[3] + v34;
          v23->f12[4] = v36;
          v37 = v30[1] + v35;
          v23->f12[3] = v37;
          v38 = v30[3] + v36;
          v23->f12[4] = v38;
          v39 = v29[5] + v37;
          v23->f12[3] = v39;
          v40 = v29[7] + v38;
          v23->f12[4] = v40;
          v41 = v30[5] + v39;
          v23->f12[3] = v41;
          v42 = v30[7] + v40;
          v23->f12[4] = v42;
          v43 = v29[9] + v41;
          v23->f12[3] = v43;
          v44 = v29[11] + v42;
          v23->f12[4] = v44;
          v45 = v30[9] + v43;
          v46 = (int32_t)(uintptr_t)v23->cur[0];
          v23->f12[3] = v45;
          v47 = v30[11] + v44;
          v23->f12[4] = v47;
          v48 = *(uint8_t *)(v46 - 7) + v45;
          v23->f12[3] = v48;
          v49 = *(uint8_t *)(v46 - 5) + v47;
          v23->f12[4] = v49;
          v23->f12[3] = *(uint8_t *)(v46 - 3) + v48;
          v23->f12[4] = *(uint8_t *)(v46 - 1) + v49;
          n4_1 = plane_count;
        }
        while ( n4_2 < plane_count );
        v19 = __frame.v91;
        v3 = __frame.v93;
        i_3 = i_1;
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
          __alt_p1_context((AltP1Block *)(uint8_t **)Block_plane[0], (uint32_t *)nullptr, (uint32_t *)0);
          n5_7 = n5_6;
          v53 = *(uint8_t *)&v50->f8;
          v112 = (uint8_t)(n5_6 - v53);
          n5_2 = v50->f984[v112];
          n5_1 = (uint8_t)(v50->f1496[n5_2] + v53);
          v56 = (uint8_t)(n5_1 + *(a2 + v114) - n5_6);
          n16 = *(a2 + v114) - v56;
          if ( n16 < -16 || n16 > 16 )
          {
            n5_2 = v50->f1240[v112];
          }
          else
          {
            n5_6 = n5_1;
            *(a2 + v114) = v56;
            n5_7 = n5_1;
          }
          __alt_p1_encode_symbol(&((P1Count *)v50)[v50->f12[0] + 237].total, n5_1, v50->f12[1], n5_2);
          v58 = n5_7 - v50->f8;
          **(uint8_t **)&v50->f12[46] = n5_6;
          *(uint8_t *)(v50->f12[46] + 1) = abs32(v58);
          v50->f12[v50->f12[2] + 3] = v50->f12[v50->f12[2] + 3]
                                                         + *(uint8_t *)(v50->f12[46] + 1)
                                                         - *(uint8_t *)(v50->f12[46] - 7)
                                                         - (*(uint8_t *)(v50->f12[50] - 3)
                                                          - *(uint8_t *)(v50->f12[50] + 13)
                                                          + *(uint8_t *)(v50->f12[48] - 3)
                                                          - *(uint8_t *)(v50->f12[48] + 13));
          v50->f12[2] = v50->f12[2] == 0;
          if ( ((P1Count *)v50)[v50->f12[0] + 237].total < 0x4000u )
            __alt_p1_model(v50);
          v50->f12[46] += 2;
          v50->f12[47] += 2;
          v50->f12[48] += 2;
          v50->f12[49] += 2;
          v50->f12[50] += 2;
          v60 = *(a2 + plane_desc[2].src_plane);
          v113 = plane_desc[2].src_plane;
          if ( v109 )
            v60 = v60 - v105 - *(plane_desc[1].src_plane + a2);
          v61 = (AltP1Block *)(v102);
          v118 = v60;
          __alt_p1_context((AltP1Block *)v102, (uint32_t *)Block_plane[0], (uint32_t *)0);
          v62 = *((uint8_t *)v61 + 8);
          v63 = (uint8_t)(v118 - v62);
          n5 = v61->f984[v63];
          v64 = *(a2 + v113);
          v65 = (uint8_t)(v61->f1496[n5] + v62);
          v111 = (uint8_t)(v65 + *(a2 + v113) - v118);
          n16_1 = v64 - v111;
          n5_3 = n5;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            n5_3 = v61->f1240[v63];
          }
          else
          {
            *(a2 + v113) = v111;
            v118 = v65;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v61)[4 * v61->f12[0] + 950], 16 * v61->f12[0], (int32_t)v61->f12[1], n5_3);
          v68 = v118 - v61->f8;
          *v61->cur[0] = v118;
          v61->cur[0][1] = (BYTE4(v68) ^ v68) - BYTE4(v68);
          ((uint8_t**)v61)[v61->f12[2] + 6] = &((uint8_t**)v61)[v61->f12[2] + 6][v61->cur[0][1]
                                                           - *(v61->cur[0] - 7)
                                                           - (*(v61->f212 - 3)
                                                            - v61->f212[13])
                                                           - (*(v61->f204 - 3)
                                                            - v61->f204[13])];
          v61->f12[2] = v61->f12[2] == 0;
          if ( ((P1Count *)v61)[v61->f12[0] + 237].total < 0x4000u )
            __alt_p1_model((AltP1Block *)v61);
          v61->cur[0] += 2;
          v61->f200 += 2;
          v61->f204 += 2;
          v61->f208 += 2;
          v61->f212 += 2;
          v69 = *(plane_desc[3].src_plane + a2);
          v117 = plane_desc[3].src_plane;
          if ( v108 )
            v69 = (uint8_t)(v69
                                  - v106
                                  - ((plane_desc[plane_desc[3].src_plane + 1].w4
                                    * *(plane_desc[1].src_plane + a2)
                                    + plane_desc[plane_desc[3].src_plane + 1].w8
                                    * (uint32_t)*(plane_desc[2].src_plane + a2)
                                    + 40) >> 7));
          v70 = (AltP1Block *)(v103);
          __alt_p1_context((AltP1Block *)v103, (uint32_t *)v102, (uint32_t *)(int32_t)Block_plane[0]);
          v71 = *((uint8_t *)v70 + 8);
          v115 = (uint8_t)(v69 - v71);
          n5_4 = v70->f984[v115];
          v73 = (uint8_t)(v70->f1496[n5_4] + v71);
          v74 = (uint8_t)(v73 + *(v117 + a2) - v69);
          n16_2 = *(v117 + a2) - v74;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            n5_4 = v70->f1240[v115];
          }
          else
          {
            v69 = v73;
            *(v117 + a2) = v74;
          }
          __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v70)[4 * v70->f12[0] + 950], n5_4, (int32_t)v70->f12[1], n5_4);
          v76 = v69 - v70->f8;
          *v70->cur[0] = v69;
          v70->cur[0][1] = (BYTE4(v76) ^ v76) - BYTE4(v76);
          ((uint8_t**)v70)[v70->f12[2] + 6] = &((uint8_t**)v70)[v70->f12[2] + 6][v70->cur[0][1]
                                                           - *(v70->cur[0] - 7)
                                                           - (*(v70->f212 - 3)
                                                            - v70->f212[13])
                                                           - (*(v70->f204 - 3)
                                                            - v70->f204[13])];
          v77 = 4 * v70->f12[0];
          v70->f12[2] = v70->f12[2] == 0;
          if ( LOWORD(((uint8_t**)v70)[v77 + 950]) < 0x4000u )
            __alt_p1_model((AltP1Block *)v70);
          v70->cur[0] += 2;
          v70->f200 += 2;
          v70->f204 += 2;
          v70->f208 += 2;
          v70->f212 += 2;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            n3 = plane_desc[4].src_plane;
            if ( v96 )
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
            __alt_p1_context((AltP1Block *)v104, (uint32_t *)v103, (uint32_t *)(int32_t)v102);
            v81 = *((uint8_t *)v80 + 8);
            n5_5 = *((uint8_t *)v80 + (uint8_t)(v99 - v81) + 984);
            v98 = (uint8_t)(v99 - v81);
            v83 = v80->f1496[n5_5];
            v84 = (uint8_t)(v83 + v81);
            n16_3 = *(n3 + a2) - (uint8_t)(v84 + *(n3 + a2) - v99);
            v97 = v84 + *(n3 + a2) - v99;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              n5_5 = v80->f1240[v98];
            }
            else
            {
              v99 = v84;
              *(n3 + a2) = v97;
            }
            __alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v80)[4 * v80->f12[0] + 950], n5_5, (int32_t)v80->f12[1], n5_5);
            v86 = v99 - v80->f8;
            *v80->cur[0] = v99;
            v80->cur[0][1] = (BYTE4(v86) ^ v86) - BYTE4(v86);
            ((uint8_t**)v80)[v80->f12[2] + 6] = &((uint8_t**)v80)[v80->f12[2] + 6][v80->cur[0][1]
                                                             - *(v80->cur[0] - 7)
                                                             - (*(v80->f212 - 3)
                                                              - v80->f212[13])
                                                             - (*(v80->f204 - 3)
                                                              - v80->f204[13])];
            v80->f12[2] = v80->f12[2] == 0;
            if ( ((P1Count *)v80)[v80->f12[0] + 237].total < 0x4000u )
              __alt_p1_model((AltP1Block *)v80);
            v80->cur[0] += 2;
            v80->f200 += 2;
            v80->f204 += 2;
            v80->f208 += 2;
            v80->f212 += 2;
            n4_1 = plane_count;
          }
          a2 += n4_1;
          ++i_4;
        }
        while ( i_4 < i_1 );
        v19 = v92;
        v3 = v94;
        i_3 = i_1;
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
  float    n2_bias;   // the p2 filter's bias term, one of three lifetimes MSVC
  int32_t  n2_half;   // gave one slot; the third is the cursor below
  // Lanes of the counter table.  Every read through this is a `uint16_t`
  // except the two shift counts at +-4, which are the low byte of the
  // neighbouring record; the byte offsets it carried were all even.
  uint16_t *n2;
  uint16_t *n0xF0;
  uint32_t n0x10_2;   // a record index in four regions ...
  uint16_t *p2_rec;   // element units: every offset through it is even    // ... and a record address in the two below
  uint16_t *v508;
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
  P2Count *v569;
  int32_t v570;
  int32_t v571;
  int32_t v572;
  int32_t v573;
  int32_t v574;
  int32_t v575;
  int32_t v576;
  int32_t v577;
  AltP2Block *v578;
  uint32_t n5;
  int32_t v580;
  // The counter this pass updates and its two neighbours: `v581[-1]`,
  // `v581[0]` and `v581[1]` are +284 708, +284 712 and +284 716 off the
  // row base, which is `f284712` reached through `v76` and `v78`.
  P2Count *v581;
  ;
  uint8_t *v8;   // was int32_t: this holds an address
  uint16_t *n2_1;   // a second name for `n2`
  // The parameter this used to copy is never read: `sample` is written from
  // `v577` below before any of its four readers, and lanes 1..3 were never
  // touched at all.  It is XMM0 being reused as a scratch register, which is
  // what MSVC did and what Hex-Rays recorded.  `v19` is the same, one lane.
  float sample;
  float (*v15)[4];
  float (*v17)[4];
  float v19;
  bool v87, v103, v104, v105;
  uint8_t v114, v116, v312;
  uint8_t *v90;   // `uint8_t *` beside the `char` scalars above
  P2Count *v110;
  P2Count *v94;
  P2Count *v111;
  P2Count *v107;
  P2Count *v112;
  P2Count *v91;
  P2Count *v93;
  float v16, v18, v20, v21, v22, v23, v24, v26;
  P2Count *v98;
  int32_t v6, v9, v75, v77, v79, v80, v81, v84, v85, v86, v88, v89, v95, v96, v100, v101, v102,
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
          v380, v381, v382, v383, v384, v386, n15, v391, v392, v394, v395, v397, v398, v399,
          v400, v403, v404, v406, v407, v409, v410, v412, v413, v415, v416, v417, v418, v419,
          v421, v422, v423, v424, v425, v426, v427, v429, v430, v431, v432, v433, v434, v435,
          v437, v438, v439, v440, v441, v442, v443, v447, v448, v449, v450, v453, v454, v459,
          v460, v461, v464, v465, v467, v468, v470, v471, v472, v475, v476, v478, v479, v481,
          v482, v483, v486, v487, v489, v490, v492, v493, v494, v497, v498, v500, v501, v503,
          v504;
  int64_t v10, v11, v12, v13, v14;
  uint16_t *n0xF0_3;
  uint16_t *n0xF0_2;
  uint16_t *n0xF0_1;
  uint16_t *n0xF0_5;
  uint16_t *n0xF0_4;
  uint16_t *v445;
  uint16_t *v387;
  uint16_t *v7, *v389, *n0x10_3, *n2_2, *v408, *v411, *v420, *v428, *v436, *v444, *n2_7,
           *n0x10_4, *v452, *v456, *v457, *n2_6, *v463, *v466, *n2_5, *v474, *v477, *n2_4,
           *v485, *v488, *v491, *n2_3, *v502;
  AltP2Block *v385;
  uint32_t *v76, v78, v92, v97, v109, n0x10, v393, v396, v405, v414, v499;
  uint8_t v83;
  v6 = a1->f278704 & 0xF;
  v7 = *(uint16_t **)&a1->f278736[0];
  v577 = 16 * a3;
  *v7 = 16 * a3;
  *(uint16_t *)(a1->f278736[0] + 2) = **(uint16_t **)&a1->f278736[0] - *(uint16_t *)(a1->f278736[0] + 2);
  *(uint16_t *)(a1->f278736[0] + 20) = 0;
  *(a1->f278736[0] + 16) = (a5 <= (int32_t)(((uint32_t)(6 - v6) >> 31)
                                                         + ((uint32_t)(4 - v6) >> 31)
                                                         + 2 * ((uint32_t)(9 - v6) >> 31)))
                                            + (a5 < (int32_t)-(((uint32_t)(6 - v6) >> 31)
                                                                + ((uint32_t)(4 - v6) >> 31)
                                                                + 2 * ((uint32_t)(9 - v6) >> 31)));
  *(a1->f278736[0] + 17) = abs32(a5);
  v8 = (uint8_t *)a1->f278736[0];
  v9 = v577;
  sample = (float)v577;
  v10 = v577 - *(int16_t *)(v8 - 18);
  *(uint16_t *)(v8 + 8) = (WORD2(v10) ^ v10) - WORD2(v10);
  v11 = v9 - **(int16_t **)&a1->f278736[1];
  *(uint16_t *)(a1->f278736[0] + 10) = (WORD2(v11) ^ v11) - WORD2(v11);
  v12 = v9 - *(int16_t *)(a1->f278736[1] - 18);
  *(uint16_t *)(a1->f278736[0] + 12) = (WORD2(v12) ^ v12) - WORD2(v12);
  v13 = v9 - *(int16_t *)(a1->f278736[1] + 18);
  *(uint16_t *)(a1->f278736[0] + 14) = (WORD2(v13) ^ v13) - WORD2(v13);
  v14 = (int16_t)(v9 - a1->f278700);
  *(uint16_t *)(a1->f278736[0] + 4) = v14;
  *(uint16_t *)(a1->f278736[0] + 6) = (WORD2(v14) ^ v14) - WORD2(v14);
  v15 = a1->f278656;
  v16 = v15[14][1] + 0.000099999997f;
  v17 = *(float (**)[4])(a1->f278668 - 4);
  v18 = *(float *)&a1->f278648;
  n2_bias = *(float *)&a1->f278640;
  v19 = sample - v18;
  v20 = *(float *)((uint8_t *)&a1->f278640 + 4);
  v21 = v20 - v18;
  v22 = ((((sample - v18) * (v20 - v18)) - v15[14][0]) * 0.001f)
      + v15[14][0];
  v23 = v16 + (((v21 * v21) - v15[14][1]) * 0.001f);
  v15[14][1] = v23;
  v24 = 0.1f * v23;
  if ( (0.1f * v23) <= v22 )
    v24 = fminf(v23, v22);
  v15[14][0] = v24;
  // Two normalised-LMS updates side by side, on two weight blocks: `v15` at a
  // fixed mean-square rate and `v17` at one scaled by the confidence `v26`.
  // Same shape as `alt_p2_context`'s, run twice with different errors and
  // different floors.
  v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;
  n5 = 0;
  v577 = v9;
  v578 = (AltP2Block *)((uint32_t *)a1);
  {
    const float err_a     = (sample - v20) * 2.5999999f;
    const float err_b     = v19 * v26;
    const float floor_a   = 26896.0f * v15[14][2];
    const float floor_b   = 5041.0f * v17[14][2];
    const float ms_rate_b = 0.013f * v26;
    int32_t j, k;

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
      {
        float x = a1->p2_row[j][k];
        float ms;

        ms = v15[7 + j][k]
           + (x * x - v15[7 + j][k]) * 0.05f;      // 0x439B40
        v15[7 + j][k] = ms;
        v15[j][k] += bmf_p2_rate[j][k] * err_a * x / (ms + floor_a);

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
      acc[k] = v15[0][k] * a1->p2_row[0][k];
      for ( j = 1; j < 7; ++j )
        acc[k] += v15[j][k] * a1->p2_row[j][k];
    }
    pred     = n2_bias + bmf_hsum4(acc);
    err      = sample - pred;
    ms_scale = v15[14][2];

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
        v15[j][k] += bmf_p2_rate[j][k] * err * a1->p2_row[j][k]
                            / (v15[7 + j][k] + ms_scale * 529.0f);
    ++*(int32_t *)&v15[15][0];
    v15[14][2] = ms_scale + ((10.0f - ms_scale) * 0.00019999999f);
  }
  **(uint32_t **)&a1->f278668 = *(uint32_t *)&a1->f278656;
  a1->f278668 += 4;
  a1->f278672 += 4;
  do
  {
    v75 = ((uint32_t *)v578)[n5 + 69669];
    v76 = &((uint32_t *)v578)[v75];
    v77 = v577 - (*(uint32_t *)&v578->f278904[2 * n5]);
    v78 = n5 << 17;
    v581 = (P2Count *)((uint8_t *)v76 + v78) + 71178;
    v79 = v77 + (uint16_t)v581->w2;
    *(uint16_t *)&v581->w2 = v79;
    v580 = v581->b1;
    if ( v580 )
    {
      v576 = v75;
      v80 = v79 + 4 * ((v77 > deadzone_hi) - (v77 < deadzone_lo));
      *(uint16_t *)&v581->w2 = v80;
      v575 = v80;
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
            v581->b1 = *((uint8_t *)&p2_b1_seed + v83 + 3);
            *(uint16_t *)&v581->w2 = 2 * v575;
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
        n2 = (uint16_t *)((uint8_t *)&v578->f284712[(v81 ^ 0x7FF0)] + v78);
        v84 = (*(uint32_t *)&v578->f278904[2 * n5 + 1]) + v77;
        n3 = v81 & 3;
        if ( (uint32_t)n3 >= 3
          || (v545 = v78,
              v576 = v81,
              v85 = v581[1].w2,
              v86 = v84 - ((v85 + (1 << ((*(uint8_t *)&v581[1].b0 + 31) & 31))) >> (*(uint8_t *)&v581[1].b0 & 31)),
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
          v89 = v84 - ((v88 + (1 << ((*(uint8_t *)&v581[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v581[-1].b0 & 31));
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
        v543 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x4000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x4000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v543, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x2000) + 284712], 0, 1);
        v539 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x2000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x5FF0) + 284712], 0, 1);
        v541 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x5FF0) + 284712]);
        v540 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x2000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x2000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v540, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x1000) + 284712], 0, 1);
        v536 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x1000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x6FF0) + 284712], 0, 1);
        v538 = (P2Count *)((int32_t)&v90[4 * (v81 ^ 0x6FF0) + 284712]);
        v537 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x1000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x1000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v537, 0, 1);
        v551 = (P2Count *)(&v90[4 * (v81 ^ 0x800) + 284712]);
        v533 = (P2Count *)(v551);
        v535 = (P2Count *)(&v90[4 * (v81 ^ 0x77F0) + 284712]);
        v534 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x800) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x800) & 0xFFFFFFF3)]);
        v552 = (P2Count *)(&v90[4 * (v81 ^ 0x400) + 284712]);
        v530 = (P2Count *)(v552);
        __builtin_prefetch(v551, 0, 1);
        __builtin_prefetch(v535, 0, 1);
        __builtin_prefetch(v534, 0, 1);
        v553 = (P2Count *)(&v90[4 * (v81 ^ 0x7BF0) + 284712]);
        v532 = (P2Count *)(v553);
        v531 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x400) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x400) & 0xFFFFFFF3)]);
        v554 = (P2Count *)(&v90[4 * (v81 ^ 0x200) + 284712]);
        v527 = (P2Count *)(v554);
        __builtin_prefetch(v552, 0, 1);
        v555 = (P2Count *)(&v90[4 * (v81 ^ 0x7DF0) + 284712]);
        v529 = (P2Count *)(v555);
        v91 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x200) & 0xC))
                 + 284712
                 + 4 * ((v81 ^ 0x200) & 0xFFFFFFF3)]);
        __builtin_prefetch(v553, 0, 1);
        __builtin_prefetch(v531, 0, 1);
        v528 = (P2Count *)(v91);
        v556 = (P2Count *)(&v90[4 * (v81 ^ 0x100) + 284712]);
        v524 = (P2Count *)(v556);
        v92 = *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x100) & 0xC)) + ((v81 ^ 0x100) & 0xFFFFFFF3);
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
        v560 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x80) & 0xC)) + 284712 + 4 * ((v81 ^ 0x80) & 0xFFFFFFF3)]);
        v522 = (P2Count *)(v560);
        v561 = (P2Count *)(&v90[4 * (v81 ^ 0x40) + 284712]);
        v518 = (P2Count *)(v561);
        __builtin_prefetch(v556, 0, 1);
        v562 = (P2Count *)(&v90[4 * (v81 ^ 0x7FB0) + 284712]);
        v520 = (P2Count *)(v562);
        v93 = (P2Count *)(&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + ((v81 ^ 0x40) & 0xC)) + 284712 + 4 * ((v81 ^ 0x40) & 0xFFFFFFF3)]);
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
        v566 = (P2Count *)((uint32_t)&v90[4 * *(int32_t *)((uint8_t *)p2_ctx_rotate + (v95 & 0xC)) + 284712 + 4 * (v95 & 0xFFFFFFF3)]);
        v516 = (P2Count *)(v566);
        v567 = (P2Count *)((int32_t)&v90[4 * v96 + 284712]);
        v512 = (P2Count *)(v567);
        v97 = *(int32_t *)((uint8_t *)p2_ctx_rotate + (v96 & 0xC)) + (v96 & 0xFFFFFFF3);
        v568 = (P2Count *)((int32_t)&v90[4 * (v96 ^ 0x7FF0) + 284712]);
        v514 = (P2Count *)(v568);
        __builtin_prefetch(v560, 0, 1);
        v98 = (P2Count *)((int32_t)&v90[4 * v97 + 284712]);
        n2_1 = n2;
        v569 = v98;
        v100 = -v550;
        v513 = (P2Count *)(v98);
        LOBYTE(v97) = (uint8_t)n2[0];
        __builtin_prefetch(v561, 0, 1);
        v101 = (int16_t)n2_1[1];
        __builtin_prefetch(v562, 0, 1);
        v102 = v100 - ((v101 + (1 << ((v97 + 31) & 31))) >> (v97 & 31));
        v105 = __OFSUB__(v102, deadzone_hi);
        v103 = v102 == deadzone_hi;
        v104 = v102 - deadzone_hi < 0;
        __builtin_prefetch(v563, 0, 1);
        v106 = 32 * (!(v104 ^ v105 | v103) - (v102 < deadzone_lo));
        __builtin_prefetch(v564, 0, 1);
        __builtin_prefetch(v565, 0, 1);
        v107 = (P2Count *)(v569);
        v108 = v550;
        v109 = v106 + v102 + 2;
        v110 = (P2Count *)(v567);
        LOWORD(v109) = v101 + (v109 >> 2);
        v87 = n3 < 3;
        v111 = (P2Count *)(v566);
        n2[1] = v109;
        v112 = (P2Count *)(v568);
        __builtin_prefetch(v111, 0, 1);
        __builtin_prefetch(v110, 0, 1);
        __builtin_prefetch(v112, 0, 1);
        __builtin_prefetch(v107, 0, 1);
        if ( v87
          && (v113 = (int16_t)n2[3],
              v114 = (uint8_t)n2[2],
              v550 = v108,
              v87 = n3 <= 0,
              n2[3] = ((uint32_t)(-v108 - ((v113 + (1 << ((v114 + 31) & 31))) >> (v114 & 31)) + 2) >> 2) + v113,
              v87) )
        {
          v311 = v542->w2;
          v312 = *(uint8_t *)&v542->b0;
          v550 = v108;
          v313 = v108 - ((v311 + (1 << ((v312 + 31) & 31))) >> (v312 & 31));
          *(uint16_t *)&v542->w2 = v311
                               + ((32 * ((v313 > deadzone_hi) - (uint32_t)(v313 < deadzone_lo)) + v313 + 2) >> 2);
          v314 = v543->w2;
          v315 = v108 - ((v314 + (1 << ((v543->b0 + 31) & 31))) >> (v543->b0 & 31));
          *(uint16_t *)&v543->w2 = v314
                               + ((32 * ((v315 > deadzone_hi) - (uint32_t)(v315 < deadzone_lo)) + v315 + 4) >> 3);
          v316 = v542[1].w2;
          v317 = (v316 + (1 << ((*(uint8_t *)&v542[1].b0 + 31) & 31))) >> (*(uint8_t *)&v542[1].b0 & 31);
          v549 = -v108;
          *(uint16_t *)&v542[1].w2 = ((uint32_t)(v108 - v317 + 2) >> 2) + v316;
          v318 = v544->w2;
          v319 = -v108 - ((v318 + (1 << ((v544->b0 + 31) & 31))) >> (v544->b0 & 31));
          *(uint16_t *)&v544->w2 = v318
                               + ((32 * ((v319 > deadzone_hi) - (uint32_t)(v319 < deadzone_lo)) + v319 + 4) >> 3);
          v320 = v539->w2;
          v321 = v550;
          v322 = v550 - ((v320 + (1 << ((*(uint8_t *)&v539->b0 + 31) & 31))) >> (*(uint8_t *)&v539->b0 & 31));
          *(uint16_t *)&v539->w2 = v320
                               + ((32 * ((v322 > deadzone_hi) - (uint32_t)(v322 < deadzone_lo)) + v322 + 2) >> 2);
          v323 = v540->w2;
          v324 = v321 - ((v323 + (1 << ((v540->b0 + 31) & 31))) >> (v540->b0 & 31));
          *(uint16_t *)&v540->w2 = v323
                               + ((32 * ((v324 > deadzone_hi) - (uint32_t)(v324 < deadzone_lo)) + v324 + 4) >> 3);
          *(uint16_t *)&v539[1].w2 += (uint32_t)(v321
                                               - ((*(int16_t *)&*(uint16_t *)&v539[1].w2 + (1 << ((*(uint8_t *)&v539[1].b0 + 31) & 31))) >> (*(uint8_t *)&v539[1].b0 & 31))
                                               + 2) >> 2;
          v325 = v541->w2;
          v326 = v549 - ((v325 + (1 << ((*(uint8_t *)&v541->b0 + 31) & 31))) >> (*(uint8_t *)&v541->b0 & 31));
          *(uint16_t *)&v541->w2 = v325
                               + ((32 * ((v326 > deadzone_hi) - (uint32_t)(v326 < deadzone_lo)) + v326 + 4) >> 3);
          v327 = v536->w2;
          v328 = v550 - ((v327 + (1 << ((*(uint8_t *)&v536->b0 + 31) & 31))) >> (*(uint8_t *)&v536->b0 & 31));
          *(uint16_t *)&v536->w2 = v327
                               + ((32 * ((v328 > deadzone_hi) - (uint32_t)(v328 < deadzone_lo)) + v328 + 2) >> 2);
          v329 = v537->w2;
          v330 = v550 - ((v329 + (1 << ((v537->b0 + 31) & 31))) >> (v537->b0 & 31));
          *(uint16_t *)&v537->w2 = v329
                               + ((32 * ((v330 > deadzone_hi) - (uint32_t)(v330 < deadzone_lo)) + v330 + 4) >> 3);
          *(uint16_t *)&v536[1].w2 += (uint32_t)(v550
                                               - ((*(int16_t *)&*(uint16_t *)&v536[1].w2 + (1 << ((*(uint8_t *)&v536[1].b0 + 31) & 31))) >> (*(uint8_t *)&v536[1].b0 & 31))
                                               + 2) >> 2;
          v331 = v538->w2;
          v332 = v549 - ((v331 + (1 << ((*(uint8_t *)&v538->b0 + 31) & 31))) >> (*(uint8_t *)&v538->b0 & 31));
          *(uint16_t *)&v538->w2 = v331
                               + ((32 * ((v332 > deadzone_hi) - (uint32_t)(v332 < deadzone_lo)) + v332 + 4) >> 3);
          v333 = v533->w2;
          v334 = v550 - ((v333 + (1 << ((v533->b0 + 31) & 31))) >> (v533->b0 & 31));
          *(uint16_t *)&v533->w2 = v333
                               + ((32 * ((v334 > deadzone_hi) - (uint32_t)(v334 < deadzone_lo)) + v334 + 2) >> 2);
          v335 = v534->w2;
          v336 = v550 - ((v335 + (1 << ((v534->b0 + 31) & 31))) >> (v534->b0 & 31));
          *(uint16_t *)&v534->w2 = v335
                               + ((32 * ((v336 > deadzone_hi) - (uint32_t)(v336 < deadzone_lo)) + v336 + 4) >> 3);
          *(uint16_t *)&v533[1].w2 += (uint32_t)(v550 - ((v533[1].w2 + (1 << ((v533[1].b0 + 31) & 31))) >> (v533[1].b0 & 31)) + 2) >> 2;
          v337 = v535->w2;
          v338 = v549 - ((v337 + (1 << ((v535->b0 + 31) & 31))) >> (v535->b0 & 31));
          *(uint16_t *)&v535->w2 = v337
                               + ((32 * ((v338 > deadzone_hi) - (uint32_t)(v338 < deadzone_lo)) + v338 + 4) >> 3);
          v339 = v530->w2;
          v340 = v550 - ((v339 + (1 << ((v530->b0 + 31) & 31))) >> (v530->b0 & 31));
          *(uint16_t *)&v530->w2 = v339
                               + ((32 * ((v340 > deadzone_hi) - (uint32_t)(v340 < deadzone_lo)) + v340 + 2) >> 2);
          v341 = v531->w2;
          v342 = v550 - ((v341 + (1 << ((v531->b0 + 31) & 31))) >> (v531->b0 & 31));
          *(uint16_t *)&v531->w2 = v341
                               + ((32 * ((v342 > deadzone_hi) - (uint32_t)(v342 < deadzone_lo)) + v342 + 4) >> 3);
          v343 = v550;
          *(uint16_t *)&v530[1].w2 += (uint32_t)(v550 - ((v530[1].w2 + (1 << ((v530[1].b0 + 31) & 31))) >> (v530[1].b0 & 31)) + 2) >> 2;
          v344 = v532->w2;
          v549 -= (v344 + (1 << ((v532->b0 + 31) & 31))) >> (v532->b0 & 31);
          v550 = v343;
          *(uint16_t *)&v532->w2 = v344
                               + ((32 * ((v549 > deadzone_hi) - (uint32_t)(v549 < deadzone_lo)) + v549 + 4) >> 3);
          v345 = v527->w2;
          v346 = v550 - ((v345 + (1 << ((v527->b0 + 31) & 31))) >> (v527->b0 & 31));
          *(uint16_t *)&v527->w2 = v345
                               + ((32 * ((v346 > deadzone_hi) - (uint32_t)(v346 < deadzone_lo)) + v346 + 2) >> 2);
          v347 = v528->w2;
          v348 = v550 - ((v347 + (1 << ((v528->b0 + 31) & 31))) >> (v528->b0 & 31));
          *(uint16_t *)&v528->w2 = v347
                               + ((32 * ((v348 > deadzone_hi) - (uint32_t)(v348 < deadzone_lo)) + v348 + 4) >> 3);
          v349 = v550;
          *(uint16_t *)&v527[1].w2 += (uint32_t)(v550 - ((v527[1].w2 + (1 << ((v527[1].b0 + 31) & 31))) >> (v527[1].b0 & 31)) + 2) >> 2;
          v350 = v529->w2;
          LOBYTE(v345) = v529->b0;
          v548 = -v349;
          v351 = -v349 - ((v350 + (1 << ((v345 + 31) & 31))) >> (v345 & 31));
          *(uint16_t *)&v529->w2 = v350
                               + ((32 * ((v351 > deadzone_hi) - (uint32_t)(v351 < deadzone_lo)) + v351 + 4) >> 3);
          v352 = v524->w2;
          v353 = v550;
          v354 = v550 - ((v352 + (1 << ((v524->b0 + 31) & 31))) >> (v524->b0 & 31));
          *(uint16_t *)&v524->w2 = v352
                               + ((32 * ((v354 > deadzone_hi) - (uint32_t)(v354 < deadzone_lo)) + v354 + 2) >> 2);
          v355 = v525->w2;
          v356 = v353 - ((v355 + (1 << ((*(uint8_t *)&v525->b0 + 31) & 31))) >> (*(uint8_t *)&v525->b0 & 31));
          *(uint16_t *)&v525->w2 = v355
                               + ((32 * ((v356 > deadzone_hi) - (uint32_t)(v356 < deadzone_lo)) + v356 + 4) >> 3);
          *(uint16_t *)&v524[1].w2 += (uint32_t)(v353 - ((v524[1].w2 + (1 << ((v524[1].b0 + 31) & 31))) >> (v524[1].b0 & 31)) + 2) >> 2;
          v357 = v526->w2;
          v358 = v548 - ((v357 + (1 << ((v526->b0 + 31) & 31))) >> (v526->b0 & 31));
          *(uint16_t *)&v526->w2 = v357
                               + ((32 * ((v358 > deadzone_hi) - (uint32_t)(v358 < deadzone_lo)) + v358 + 4) >> 3);
          v359 = v521->w2;
          v360 = v550 - ((v359 + (1 << ((v521->b0 + 31) & 31))) >> (v521->b0 & 31));
          *(uint16_t *)&v521->w2 = v359
                               + ((32 * ((v360 > deadzone_hi) - (uint32_t)(v360 < deadzone_lo)) + v360 + 2) >> 2);
          v361 = v522->w2;
          v362 = v550 - ((v361 + (1 << ((v522->b0 + 31) & 31))) >> (v522->b0 & 31));
          *(uint16_t *)&v522->w2 = v361
                               + ((32 * ((v362 > deadzone_hi) - (uint32_t)(v362 < deadzone_lo)) + v362 + 4) >> 3);
          *(uint16_t *)&v521[1].w2 += (uint32_t)(v550 - ((v521[1].w2 + (1 << ((v521[1].b0 + 31) & 31))) >> (v521[1].b0 & 31)) + 2) >> 2;
          v363 = v523->w2;
          v364 = v548 - ((v363 + (1 << ((v523->b0 + 31) & 31))) >> (v523->b0 & 31));
          *(uint16_t *)&v523->w2 = v363
                               + ((32 * ((v364 > deadzone_hi) - (uint32_t)(v364 < deadzone_lo)) + v364 + 4) >> 3);
          v365 = v518->w2;
          v366 = v550 - ((v365 + (1 << ((v518->b0 + 31) & 31))) >> (v518->b0 & 31));
          *(uint16_t *)&v518->w2 = v365
                               + ((32 * ((v366 > deadzone_hi) - (uint32_t)(v366 < deadzone_lo)) + v366 + 2) >> 2);
          v367 = v519->w2;
          v368 = v550 - ((v367 + (1 << ((v519->b0 + 31) & 31))) >> (v519->b0 & 31));
          *(uint16_t *)&v519->w2 = v367
                               + ((32 * ((v368 > deadzone_hi) - (uint32_t)(v368 < deadzone_lo)) + v368 + 4) >> 3);
          *(uint16_t *)&v518[1].w2 += (uint32_t)(v550 - ((v518[1].w2 + (1 << ((v518[1].b0 + 31) & 31))) >> (v518[1].b0 & 31)) + 2) >> 2;
          v369 = v520->w2;
          v370 = v548 - ((v369 + (1 << ((v520->b0 + 31) & 31))) >> (v520->b0 & 31));
          *(uint16_t *)&v520->w2 = v369
                               + ((32 * ((v370 > deadzone_hi) - (uint32_t)(v370 < deadzone_lo)) + v370 + 4) >> 3);
          v371 = v515->w2;
          v372 = v550 - ((v371 + (1 << ((v515->b0 + 31) & 31))) >> (v515->b0 & 31));
          *(uint16_t *)&v515->w2 = v371
                               + ((32 * ((v372 > deadzone_hi) - (uint32_t)(v372 < deadzone_lo)) + v372 + 2) >> 2);
          v373 = v516->w2;
          v374 = v550 - ((v373 + (1 << ((*(uint8_t *)&v516->b0 + 31) & 31))) >> (*(uint8_t *)&v516->b0 & 31));
          *(uint16_t *)&v516->w2 = v373
                               + ((32 * ((v374 > deadzone_hi) - (uint32_t)(v374 < deadzone_lo)) + v374 + 4) >> 3);
          v375 = v550;
          *(uint16_t *)&v515[1].w2 += (uint32_t)(v550 - ((v515[1].w2 + (1 << ((v515[1].b0 + 31) & 31))) >> (v515[1].b0 & 31)) + 2) >> 2;
          v376 = v517->w2;
          v377 = (v376 + (1 << ((v517->b0 + 31) & 31))) >> (v517->b0 & 31);
          v550 = v375;
          *(uint16_t *)&v517->w2 = v376
                               + ((32 * ((v548 - v377 > deadzone_hi) - (uint32_t)(v548 - v377 < deadzone_lo))
                                 + v548
                                 - v377
                                 + 4) >> 3);
          v378 = v512->w2;
          v379 = v550 - ((v378 + (1 << ((*(uint8_t *)&v512->b0 + 31) & 31))) >> (*(uint8_t *)&v512->b0 & 31));
          *(uint16_t *)&v512->w2 = v378
                               + ((32 * ((v379 > deadzone_hi) - (uint32_t)(v379 < deadzone_lo)) + v379 + 2) >> 2);
          v380 = v513->w2;
          v381 = v550 - ((v380 + (1 << ((*(uint8_t *)&v513->b0 + 31) & 31))) >> (*(uint8_t *)&v513->b0 & 31));
          *(uint16_t *)&v513->w2 = v380
                               + ((32 * ((v381 > deadzone_hi) - (uint32_t)(v381 < deadzone_lo)) + v381 + 4) >> 3);
          v382 = v550;
          *(uint16_t *)&v512[1].w2 += (uint32_t)(v550
                                               - ((*(int16_t *)&*(uint16_t *)&v512[1].w2 + (1 << ((*(uint8_t *)&v512[1].b0 + 31) & 31))) >> (*(uint8_t *)&v512[1].b0 & 31))
                                               + 2) >> 2;
          v383 = v514->w2;
          v384 = -v382 - ((v383 + (1 << ((*(uint8_t *)&v514->b0 + 31) & 31))) >> (*(uint8_t *)&v514->b0 & 31));
          *(uint16_t *)&v514->w2 = v383
                               + ((32 * ((v384 > deadzone_hi) - (uint32_t)(v384 < deadzone_lo)) + v384 + 4) >> 3);
        }
        else
        {
          v115 = (int16_t)n2[-1];
          v116 = (uint8_t)n2[-2];
          v550 = v108;
          n2[-1] = ((uint32_t)(-v108 - ((v115 + (1 << ((v116 + 31) & 31))) >> (v116 & 31)) + 4) >> 3) + v115;
          v117 = v542->w2;
          v118 = v550 - ((v117 + (1 << ((*(uint8_t *)&v542->b0 + 31) & 31))) >> (*(uint8_t *)&v542->b0 & 31));
          *(uint16_t *)&v542->w2 = v117
                               + ((32 * ((v118 > deadzone_hi) - (uint32_t)(v118 < deadzone_lo)) + v118 + 2) >> 2);
          v119 = v543->w2;
          v87 = n3 < 3;
          v120 = v550 - ((v119 + (1 << ((v543->b0 + 31) & 31))) >> (v543->b0 & 31));
          *(uint16_t *)&v543->w2 = v119
                               + ((32 * ((v120 > deadzone_hi) - (uint32_t)(v120 < deadzone_lo)) + v120 + 4) >> 3);
          v121 = v550;
          if ( v87 )
          {
            *(uint16_t *)&v542[1].w2 += (uint32_t)(v550
                                                 - ((v542[1].w2 + (1 << ((*(uint8_t *)&v542[1].b0 + 31) & 31))) >> (*(uint8_t *)&v542[1].b0 & 31))
                                                 + 2) >> 2;
            v216 = v542[-1].w2;
            v217 = v121 - ((v216 + (1 << ((*(uint8_t *)&v542[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v542[-1].b0 & 31));
            v218 = -v121;
            v572 = v218;
            *(uint16_t *)&v542[-1].w2 = v216
                                 + ((32 * ((v217 > deadzone_hi) - (uint32_t)(v217 < deadzone_lo)) + v217 + 4) >> 3);
            v219 = v544->w2;
            v220 = v218 - ((v219 + (1 << ((v544->b0 + 31) & 31))) >> (v544->b0 & 31));
            *(uint16_t *)&v544->w2 = v219
                                 + ((32 * ((v220 > deadzone_hi) - (uint32_t)(v220 < deadzone_lo)) + v220 + 4) >> 3);
            v221 = v539->w2;
            v222 = v550;
            v223 = v550 - ((v221 + (1 << ((*(uint8_t *)&v539->b0 + 31) & 31))) >> (*(uint8_t *)&v539->b0 & 31));
            *(uint16_t *)&v539->w2 = v221
                                 + ((32 * ((v223 > deadzone_hi) - (uint32_t)(v223 < deadzone_lo)) + v223 + 2) >> 2);
            v224 = v540->w2;
            v225 = v222 - ((v224 + (1 << ((v540->b0 + 31) & 31))) >> (v540->b0 & 31));
            *(uint16_t *)&v540->w2 = v224
                                 + ((32 * ((v225 > deadzone_hi) - (uint32_t)(v225 < deadzone_lo)) + v225 + 4) >> 3);
            *(uint16_t *)&v539[1].w2 += (uint32_t)(v222
                                                 - ((*(int16_t *)&*(uint16_t *)&v539[1].w2 + (1 << ((*(uint8_t *)&v539[1].b0 + 31) & 31))) >> (*(uint8_t *)&v539[1].b0 & 31))
                                                 + 2) >> 2;
            v226 = v539[-1].w2;
            v227 = v222 - ((v226 + (1 << ((*(uint8_t *)&v539[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v539[-1].b0 & 31));
            *(uint16_t *)&v539[-1].w2 = v226
                                 + ((32 * ((v227 > deadzone_hi) - (uint32_t)(v227 < deadzone_lo)) + v227 + 4) >> 3);
            v228 = v541->w2;
            v229 = v572 - ((v228 + (1 << ((*(uint8_t *)&v541->b0 + 31) & 31))) >> (*(uint8_t *)&v541->b0 & 31));
            *(uint16_t *)&v541->w2 = v228
                                 + ((32 * ((v229 > deadzone_hi) - (uint32_t)(v229 < deadzone_lo)) + v229 + 4) >> 3);
            v230 = v536->w2;
            v231 = v550;
            v232 = v550 - ((v230 + (1 << ((*(uint8_t *)&v536->b0 + 31) & 31))) >> (*(uint8_t *)&v536->b0 & 31));
            *(uint16_t *)&v536->w2 = v230
                                 + ((32 * ((v232 > deadzone_hi) - (uint32_t)(v232 < deadzone_lo)) + v232 + 2) >> 2);
            v233 = v537->w2;
            v234 = v231 - ((v233 + (1 << ((v537->b0 + 31) & 31))) >> (v537->b0 & 31));
            *(uint16_t *)&v537->w2 = v233
                                 + ((32 * ((v234 > deadzone_hi) - (uint32_t)(v234 < deadzone_lo)) + v234 + 4) >> 3);
            *(uint16_t *)&v536[1].w2 += (uint32_t)(v231
                                                 - ((*(int16_t *)&*(uint16_t *)&v536[1].w2 + (1 << ((*(uint8_t *)&v536[1].b0 + 31) & 31))) >> (*(uint8_t *)&v536[1].b0 & 31))
                                                 + 2) >> 2;
            v235 = v536[-1].w2;
            v236 = v231 - ((v235 + (1 << ((*(uint8_t *)&v536[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v536[-1].b0 & 31));
            *(uint16_t *)&v536[-1].w2 = v235
                                 + ((32 * ((v236 > deadzone_hi) - (uint32_t)(v236 < deadzone_lo)) + v236 + 4) >> 3);
            v237 = v538->w2;
            v238 = v572 - ((v237 + (1 << ((*(uint8_t *)&v538->b0 + 31) & 31))) >> (*(uint8_t *)&v538->b0 & 31));
            *(uint16_t *)&v538->w2 = v237
                                 + ((32 * ((v238 > deadzone_hi) - (uint32_t)(v238 < deadzone_lo)) + v238 + 4) >> 3);
            v239 = v533->w2;
            v240 = v550;
            v241 = v550 - ((v239 + (1 << ((v533->b0 + 31) & 31))) >> (v533->b0 & 31));
            *(uint16_t *)&v533->w2 = v239
                                 + ((32 * ((v241 > deadzone_hi) - (uint32_t)(v241 < deadzone_lo)) + v241 + 2) >> 2);
            v242 = v534->w2;
            v243 = v240 - ((v242 + (1 << ((v534->b0 + 31) & 31))) >> (v534->b0 & 31));
            *(uint16_t *)&v534->w2 = v242
                                 + ((32 * ((v243 > deadzone_hi) - (uint32_t)(v243 < deadzone_lo)) + v243 + 4) >> 3);
            *(uint16_t *)&v533[1].w2 += (uint32_t)(v240
                                                 - ((v533[1].w2 + (1 << ((v533[1].b0 + 31) & 31))) >> (v533[1].b0 & 31))
                                                 + 2) >> 2;
            v244 = v533[-1].w2;
            v245 = v240 - ((v244 + (1 << ((v533[-1].b0 + 31) & 31))) >> (v533[-1].b0 & 31));
            *(uint16_t *)&v533[-1].w2 = v244
                                 + ((32 * ((v245 > deadzone_hi) - (uint32_t)(v245 < deadzone_lo)) + v245 + 4) >> 3);
            v246 = v535->w2;
            v247 = v572 - ((v246 + (1 << ((v535->b0 + 31) & 31))) >> (v535->b0 & 31));
            *(uint16_t *)&v535->w2 = v246
                                 + ((32 * ((v247 > deadzone_hi) - (uint32_t)(v247 < deadzone_lo)) + v247 + 4) >> 3);
            v248 = v530->w2;
            v249 = v550;
            v250 = v550 - ((v248 + (1 << ((v530->b0 + 31) & 31))) >> (v530->b0 & 31));
            *(uint16_t *)&v530->w2 = v248
                                 + ((32 * ((v250 > deadzone_hi) - (uint32_t)(v250 < deadzone_lo)) + v250 + 2) >> 2);
            v251 = v531->w2;
            LOBYTE(v248) = v531->b0;
            v252 = v251 + (1 << ((v531->b0 + 31) & 31));
            v550 = v249;
            *(uint16_t *)&v531->w2 = v251
                                 + ((32
                                   * ((v249 - (v252 >> (v248 & 31)) > deadzone_hi)
                                    - (uint32_t)(v249 - (v252 >> (v248 & 31)) < deadzone_lo))
                                   + v249
                                   - (v252 >> (v248 & 31))
                                   + 4) >> 3);
            v253 = v550;
            *(uint16_t *)&v530[1].w2 += (uint32_t)(v550
                                                 - ((v530[1].w2 + (1 << ((v530[1].b0 + 31) & 31))) >> (v530[1].b0 & 31))
                                                 + 2) >> 2;
            v254 = v530[-1].w2;
            v255 = v253 - ((v254 + (1 << ((v530[-1].b0 + 31) & 31))) >> (v530[-1].b0 & 31));
            *(uint16_t *)&v530[-1].w2 = v254
                                 + ((32 * ((v255 > deadzone_hi) - (uint32_t)(v255 < deadzone_lo)) + v255 + 4) >> 3);
            v256 = v532->w2;
            LOBYTE(v254) = v532->b0;
            v573 = -v253;
            v257 = -v253 - ((v256 + (1 << ((v254 + 31) & 31))) >> (v254 & 31));
            *(uint16_t *)&v532->w2 = v256
                                 + ((32 * ((v257 > deadzone_hi) - (uint32_t)(v257 < deadzone_lo)) + v257 + 4) >> 3);
            v258 = v527->w2;
            v259 = v550;
            v260 = v550 - ((v258 + (1 << ((v527->b0 + 31) & 31))) >> (v527->b0 & 31));
            *(uint16_t *)&v527->w2 = v258
                                 + ((32 * ((v260 > deadzone_hi) - (uint32_t)(v260 < deadzone_lo)) + v260 + 2) >> 2);
            v261 = v528->w2;
            v262 = v259 - ((v261 + (1 << ((v528->b0 + 31) & 31))) >> (v528->b0 & 31));
            *(uint16_t *)&v528->w2 = v261
                                 + ((32 * ((v262 > deadzone_hi) - (uint32_t)(v262 < deadzone_lo)) + v262 + 4) >> 3);
            *(uint16_t *)&v527[1].w2 += (uint32_t)(v259
                                                 - ((v527[1].w2 + (1 << ((v527[1].b0 + 31) & 31))) >> (v527[1].b0 & 31))
                                                 + 2) >> 2;
            v263 = v527[-1].w2;
            v264 = v259 - ((v263 + (1 << ((v527[-1].b0 + 31) & 31))) >> (v527[-1].b0 & 31));
            *(uint16_t *)&v527[-1].w2 = v263
                                 + ((32 * ((v264 > deadzone_hi) - (uint32_t)(v264 < deadzone_lo)) + v264 + 4) >> 3);
            v265 = v529->w2;
            v266 = v573 - ((v265 + (1 << ((v529->b0 + 31) & 31))) >> (v529->b0 & 31));
            *(uint16_t *)&v529->w2 = v265
                                 + ((32 * ((v266 > deadzone_hi) - (uint32_t)(v266 < deadzone_lo)) + v266 + 4) >> 3);
            v267 = v524->w2;
            v268 = v550;
            v269 = v550 - ((v267 + (1 << ((v524->b0 + 31) & 31))) >> (v524->b0 & 31));
            *(uint16_t *)&v524->w2 = v267
                                 + ((32 * ((v269 > deadzone_hi) - (uint32_t)(v269 < deadzone_lo)) + v269 + 2) >> 2);
            v270 = v525->w2;
            v271 = v268 - ((v270 + (1 << ((*(uint8_t *)&v525->b0 + 31) & 31))) >> (*(uint8_t *)&v525->b0 & 31));
            *(uint16_t *)&v525->w2 = v270
                                 + ((32 * ((v271 > deadzone_hi) - (uint32_t)(v271 < deadzone_lo)) + v271 + 4) >> 3);
            *(uint16_t *)&v524[1].w2 += (uint32_t)(v268
                                                 - ((v524[1].w2 + (1 << ((v524[1].b0 + 31) & 31))) >> (v524[1].b0 & 31))
                                                 + 2) >> 2;
            v272 = v524[-1].w2;
            v273 = v268 - ((v272 + (1 << ((v524[-1].b0 + 31) & 31))) >> (v524[-1].b0 & 31));
            *(uint16_t *)&v524[-1].w2 = v272
                                 + ((32 * ((v273 > deadzone_hi) - (uint32_t)(v273 < deadzone_lo)) + v273 + 4) >> 3);
            v274 = v526->w2;
            v275 = v573 - ((v274 + (1 << ((v526->b0 + 31) & 31))) >> (v526->b0 & 31));
            *(uint16_t *)&v526->w2 = v274
                                 + ((32 * ((v275 > deadzone_hi) - (uint32_t)(v275 < deadzone_lo)) + v275 + 4) >> 3);
            v276 = v521->w2;
            v277 = v550;
            v278 = v550 - ((v276 + (1 << ((v521->b0 + 31) & 31))) >> (v521->b0 & 31));
            *(uint16_t *)&v521->w2 = v276
                                 + ((32 * ((v278 > deadzone_hi) - (uint32_t)(v278 < deadzone_lo)) + v278 + 2) >> 2);
            v279 = v522->w2;
            v280 = v277 - ((v279 + (1 << ((v522->b0 + 31) & 31))) >> (v522->b0 & 31));
            *(uint16_t *)&v522->w2 = v279
                                 + ((32 * ((v280 > deadzone_hi) - (uint32_t)(v280 < deadzone_lo)) + v280 + 4) >> 3);
            *(uint16_t *)&v521[1].w2 += (uint32_t)(v277
                                                 - ((v521[1].w2 + (1 << ((v521[1].b0 + 31) & 31))) >> (v521[1].b0 & 31))
                                                 + 2) >> 2;
            v281 = v521[-1].w2;
            v282 = v277 - ((v281 + (1 << ((v521[-1].b0 + 31) & 31))) >> (v521[-1].b0 & 31));
            *(uint16_t *)&v521[-1].w2 = v281
                                 + ((32 * ((v282 > deadzone_hi) - (uint32_t)(v282 < deadzone_lo)) + v282 + 4) >> 3);
            v283 = v523->w2;
            v284 = v573 - ((v283 + (1 << ((v523->b0 + 31) & 31))) >> (v523->b0 & 31));
            *(uint16_t *)&v523->w2 = v283
                                 + ((32 * ((v284 > deadzone_hi) - (uint32_t)(v284 < deadzone_lo)) + v284 + 4) >> 3);
            v285 = v518->w2;
            v286 = v550;
            v287 = v550 - ((v285 + (1 << ((v518->b0 + 31) & 31))) >> (v518->b0 & 31));
            *(uint16_t *)&v518->w2 = v285
                                 + ((32 * ((v287 > deadzone_hi) - (uint32_t)(v287 < deadzone_lo)) + v287 + 2) >> 2);
            v288 = v519->w2;
            v289 = v286 - ((v288 + (1 << ((v519->b0 + 31) & 31))) >> (v519->b0 & 31));
            *(uint16_t *)&v519->w2 = v288
                                 + ((32 * ((v289 > deadzone_hi) - (uint32_t)(v289 < deadzone_lo)) + v289 + 4) >> 3);
            *(uint16_t *)&v518[1].w2 += (uint32_t)(v286
                                                 - ((v518[1].w2 + (1 << ((v518[1].b0 + 31) & 31))) >> (v518[1].b0 & 31))
                                                 + 2) >> 2;
            v290 = v518[-1].w2;
            v291 = v286 - ((v290 + (1 << ((v518[-1].b0 + 31) & 31))) >> (v518[-1].b0 & 31));
            v292 = -v286;
            v574 = v292;
            *(uint16_t *)&v518[-1].w2 = v290
                                 + ((32 * ((v291 > deadzone_hi) - (uint32_t)(v291 < deadzone_lo)) + v291 + 4) >> 3);
            v293 = v520->w2;
            v294 = v292 - ((v293 + (1 << ((v520->b0 + 31) & 31))) >> (v520->b0 & 31));
            *(uint16_t *)&v520->w2 = v293
                                 + ((32 * ((v294 > deadzone_hi) - (uint32_t)(v294 < deadzone_lo)) + v294 + 4) >> 3);
            v295 = v515->w2;
            v296 = v550;
            v297 = v550 - ((v295 + (1 << ((v515->b0 + 31) & 31))) >> (v515->b0 & 31));
            *(uint16_t *)&v515->w2 = v295
                                 + ((32 * ((v297 > deadzone_hi) - (uint32_t)(v297 < deadzone_lo)) + v297 + 2) >> 2);
            v298 = v516->w2;
            v299 = v296 - ((v298 + (1 << ((*(uint8_t *)&v516->b0 + 31) & 31))) >> (*(uint8_t *)&v516->b0 & 31));
            *(uint16_t *)&v516->w2 = v298
                                 + ((32 * ((v299 > deadzone_hi) - (uint32_t)(v299 < deadzone_lo)) + v299 + 4) >> 3);
            *(uint16_t *)&v515[1].w2 += (uint32_t)(v296
                                                 - ((v515[1].w2 + (1 << ((v515[1].b0 + 31) & 31))) >> (v515[1].b0 & 31))
                                                 + 2) >> 2;
            v300 = v515[-1].w2;
            v301 = v296 - ((v300 + (1 << ((v515[-1].b0 + 31) & 31))) >> (v515[-1].b0 & 31));
            *(uint16_t *)&v515[-1].w2 = v300
                                 + ((32 * ((v301 > deadzone_hi) - (uint32_t)(v301 < deadzone_lo)) + v301 + 4) >> 3);
            v302 = v517->w2;
            v303 = v574 - ((v302 + (1 << ((v517->b0 + 31) & 31))) >> (v517->b0 & 31));
            *(uint16_t *)&v517->w2 = v302
                                 + ((32 * ((v303 > deadzone_hi) - (uint32_t)(v303 < deadzone_lo)) + v303 + 4) >> 3);
            v304 = v512->w2;
            v305 = v550;
            v306 = v550 - ((v304 + (1 << ((*(uint8_t *)&v512->b0 + 31) & 31))) >> (*(uint8_t *)&v512->b0 & 31));
            *(uint16_t *)&v512->w2 = v304
                                 + ((32 * ((v306 > deadzone_hi) - (uint32_t)(v306 < deadzone_lo)) + v306 + 2) >> 2);
            v307 = v513->w2;
            v308 = v305 - ((v307 + (1 << ((*(uint8_t *)&v513->b0 + 31) & 31))) >> (*(uint8_t *)&v513->b0 & 31));
            *(uint16_t *)&v513->w2 = v307
                                 + ((32 * ((v308 > deadzone_hi) - (uint32_t)(v308 < deadzone_lo)) + v308 + 4) >> 3);
            *(uint16_t *)&v512[1].w2 += (uint32_t)(v305
                                                 - ((*(int16_t *)&*(uint16_t *)&v512[1].w2 + (1 << ((*(uint8_t *)&v512[1].b0 + 31) & 31))) >> (*(uint8_t *)&v512[1].b0 & 31))
                                                 + 2) >> 2;
            v309 = v512[-1].w2;
            v310 = v305 - ((v309 + (1 << ((*(uint8_t *)&v512[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v512[-1].b0 & 31));
            *(uint16_t *)&v512[-1].w2 = v309
                                 + ((32 * ((v310 > deadzone_hi) - (uint32_t)(v310 < deadzone_lo)) + v310 + 4) >> 3);
            v214 = v514->w2;
            v215 = v574 - ((v214 + (1 << ((*(uint8_t *)&v514->b0 + 31) & 31))) >> (*(uint8_t *)&v514->b0 & 31));
          }
          else
          {
            v122 = v542[-1].w2;
            v123 = v550 - ((v122 + (1 << ((*(uint8_t *)&v542[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v542[-1].b0 & 31));
            *(uint16_t *)&v542[-1].w2 = v122
                                 + ((32 * ((v123 > deadzone_hi) - (uint32_t)(v123 < deadzone_lo)) + v123 + 4) >> 3);
            v124 = v544->w2;
            LOBYTE(v122) = v544->b0;
            v547 = -v121;
            v125 = -v121 - ((v124 + (1 << ((v122 + 31) & 31))) >> (v122 & 31));
            *(uint16_t *)&v544->w2 = v124
                                 + ((32 * ((v125 > deadzone_hi) - (uint32_t)(v125 < deadzone_lo)) + v125 + 4) >> 3);
            v126 = v539->w2;
            v127 = v550;
            v128 = v550 - ((v126 + (1 << ((*(uint8_t *)&v539->b0 + 31) & 31))) >> (*(uint8_t *)&v539->b0 & 31));
            *(uint16_t *)&v539->w2 = v126
                                 + ((32 * ((v128 > deadzone_hi) - (uint32_t)(v128 < deadzone_lo)) + v128 + 2) >> 2);
            v129 = v540->w2;
            v130 = v127 - ((v129 + (1 << ((v540->b0 + 31) & 31))) >> (v540->b0 & 31));
            *(uint16_t *)&v540->w2 = v129
                                 + ((32 * ((v130 > deadzone_hi) - (uint32_t)(v130 < deadzone_lo)) + v130 + 4) >> 3);
            v131 = v539[-1].w2;
            v132 = v127 - ((v131 + (1 << ((*(uint8_t *)&v539[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v539[-1].b0 & 31));
            *(uint16_t *)&v539[-1].w2 = v131
                                 + ((32 * ((v132 > deadzone_hi) - (uint32_t)(v132 < deadzone_lo)) + v132 + 4) >> 3);
            v133 = v541->w2;
            v134 = v547 - ((v133 + (1 << ((*(uint8_t *)&v541->b0 + 31) & 31))) >> (*(uint8_t *)&v541->b0 & 31));
            *(uint16_t *)&v541->w2 = v133
                                 + ((32 * ((v134 > deadzone_hi) - (uint32_t)(v134 < deadzone_lo)) + v134 + 4) >> 3);
            v135 = v536->w2;
            v136 = v550;
            v137 = v550 - ((v135 + (1 << ((*(uint8_t *)&v536->b0 + 31) & 31))) >> (*(uint8_t *)&v536->b0 & 31));
            *(uint16_t *)&v536->w2 = v135
                                 + ((32 * ((v137 > deadzone_hi) - (uint32_t)(v137 < deadzone_lo)) + v137 + 2) >> 2);
            v138 = v537->w2;
            v139 = v136 - ((v138 + (1 << ((v537->b0 + 31) & 31))) >> (v537->b0 & 31));
            *(uint16_t *)&v537->w2 = v138
                                 + ((32 * ((v139 > deadzone_hi) - (uint32_t)(v139 < deadzone_lo)) + v139 + 4) >> 3);
            v140 = v536[-1].w2;
            v141 = v136 - ((v140 + (1 << ((*(uint8_t *)&v536[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v536[-1].b0 & 31));
            *(uint16_t *)&v536[-1].w2 = v140
                                 + ((32 * ((v141 > deadzone_hi) - (uint32_t)(v141 < deadzone_lo)) + v141 + 4) >> 3);
            v142 = v538->w2;
            v143 = v547 - ((v142 + (1 << ((*(uint8_t *)&v538->b0 + 31) & 31))) >> (*(uint8_t *)&v538->b0 & 31));
            *(uint16_t *)&v538->w2 = v142
                                 + ((32 * ((v143 > deadzone_hi) - (uint32_t)(v143 < deadzone_lo)) + v143 + 4) >> 3);
            v144 = v533->w2;
            v145 = v550;
            v146 = v550 - ((v144 + (1 << ((v533->b0 + 31) & 31))) >> (v533->b0 & 31));
            *(uint16_t *)&v533->w2 = v144
                                 + ((32 * ((v146 > deadzone_hi) - (uint32_t)(v146 < deadzone_lo)) + v146 + 2) >> 2);
            v147 = v534->w2;
            v148 = v145 - ((v147 + (1 << ((v534->b0 + 31) & 31))) >> (v534->b0 & 31));
            *(uint16_t *)&v534->w2 = v147
                                 + ((32 * ((v148 > deadzone_hi) - (uint32_t)(v148 < deadzone_lo)) + v148 + 4) >> 3);
            v149 = v533[-1].w2;
            v150 = v145 - ((v149 + (1 << ((v533[-1].b0 + 31) & 31))) >> (v533[-1].b0 & 31));
            *(uint16_t *)&v533[-1].w2 = v149
                                 + ((32 * ((v150 > deadzone_hi) - (uint32_t)(v150 < deadzone_lo)) + v150 + 4) >> 3);
            v151 = v535->w2;
            v152 = v547 - ((v151 + (1 << ((v535->b0 + 31) & 31))) >> (v535->b0 & 31));
            *(uint16_t *)&v535->w2 = v151
                                 + ((32 * ((v152 > deadzone_hi) - (uint32_t)(v152 < deadzone_lo)) + v152 + 4) >> 3);
            v153 = v530->w2;
            v154 = v550;
            v155 = v550 - ((v153 + (1 << ((v530->b0 + 31) & 31))) >> (v530->b0 & 31));
            *(uint16_t *)&v530->w2 = v153
                                 + ((32 * ((v155 > deadzone_hi) - (uint32_t)(v155 < deadzone_lo)) + v155 + 2) >> 2);
            v156 = v531->w2;
            v157 = v154 - ((v156 + (1 << ((v531->b0 + 31) & 31))) >> (v531->b0 & 31));
            *(uint16_t *)&v531->w2 = v156
                                 + ((32 * ((v157 > deadzone_hi) - (uint32_t)(v157 < deadzone_lo)) + v157 + 4) >> 3);
            v158 = v530[-1].w2;
            v159 = v154 - ((v158 + (1 << ((v530[-1].b0 + 31) & 31))) >> (v530[-1].b0 & 31));
            *(uint16_t *)&v530[-1].w2 = v158
                                 + ((32 * ((v159 > deadzone_hi) - (uint32_t)(v159 < deadzone_lo)) + v159 + 4) >> 3);
            v160 = v532->w2;
            v547 -= (v160 + (1 << ((v532->b0 + 31) & 31))) >> (v532->b0 & 31);
            v161 = v550;
            *(uint16_t *)&v532->w2 = v160
                                 + ((32 * ((v547 > deadzone_hi) - (uint32_t)(v547 < deadzone_lo)) + v547 + 4) >> 3);
            v162 = v527->w2;
            v163 = v161 - ((v162 + (1 << ((v527->b0 + 31) & 31))) >> (v527->b0 & 31));
            *(uint16_t *)&v527->w2 = v162
                                 + ((32 * ((v163 > deadzone_hi) - (uint32_t)(v163 < deadzone_lo)) + v163 + 2) >> 2);
            v164 = v528->w2;
            v165 = v161 - ((v164 + (1 << ((v528->b0 + 31) & 31))) >> (v528->b0 & 31));
            *(uint16_t *)&v528->w2 = v164
                                 + ((32 * ((v165 > deadzone_hi) - (uint32_t)(v165 < deadzone_lo)) + v165 + 4) >> 3);
            v166 = v527[-1].w2;
            v167 = v161 - ((v166 + (1 << ((v527[-1].b0 + 31) & 31))) >> (v527[-1].b0 & 31));
            v168 = -v161;
            v570 = v168;
            *(uint16_t *)&v527[-1].w2 = v166
                                 + ((32 * ((v167 > deadzone_hi) - (uint32_t)(v167 < deadzone_lo)) + v167 + 4) >> 3);
            v169 = v529->w2;
            v170 = v168 - ((v169 + (1 << ((v529->b0 + 31) & 31))) >> (v529->b0 & 31));
            *(uint16_t *)&v529->w2 = v169
                                 + ((32 * ((v170 > deadzone_hi) - (uint32_t)(v170 < deadzone_lo)) + v170 + 4) >> 3);
            v171 = v524->w2;
            v172 = v550;
            v173 = v550 - ((v171 + (1 << ((v524->b0 + 31) & 31))) >> (v524->b0 & 31));
            *(uint16_t *)&v524->w2 = v171
                                 + ((32 * ((v173 > deadzone_hi) - (uint32_t)(v173 < deadzone_lo)) + v173 + 2) >> 2);
            v174 = v525->w2;
            v175 = v172 - ((v174 + (1 << ((*(uint8_t *)&v525->b0 + 31) & 31))) >> (*(uint8_t *)&v525->b0 & 31));
            *(uint16_t *)&v525->w2 = v174
                                 + ((32 * ((v175 > deadzone_hi) - (uint32_t)(v175 < deadzone_lo)) + v175 + 4) >> 3);
            v176 = v524[-1].w2;
            v177 = v172 - ((v176 + (1 << ((v524[-1].b0 + 31) & 31))) >> (v524[-1].b0 & 31));
            *(uint16_t *)&v524[-1].w2 = v176
                                 + ((32 * ((v177 > deadzone_hi) - (uint32_t)(v177 < deadzone_lo)) + v177 + 4) >> 3);
            v178 = v526->w2;
            v179 = v570 - ((v178 + (1 << ((v526->b0 + 31) & 31))) >> (v526->b0 & 31));
            *(uint16_t *)&v526->w2 = v178
                                 + ((32 * ((v179 > deadzone_hi) - (uint32_t)(v179 < deadzone_lo)) + v179 + 4) >> 3);
            v180 = v521->w2;
            v181 = v550;
            v182 = v550 - ((v180 + (1 << ((v521->b0 + 31) & 31))) >> (v521->b0 & 31));
            *(uint16_t *)&v521->w2 = v180
                                 + ((32 * ((v182 > deadzone_hi) - (uint32_t)(v182 < deadzone_lo)) + v182 + 2) >> 2);
            v183 = v522->w2;
            v184 = v181 - ((v183 + (1 << ((v522->b0 + 31) & 31))) >> (v522->b0 & 31));
            *(uint16_t *)&v522->w2 = v183
                                 + ((32 * ((v184 > deadzone_hi) - (uint32_t)(v184 < deadzone_lo)) + v184 + 4) >> 3);
            v185 = v521[-1].w2;
            v186 = v181 - ((v185 + (1 << ((v521[-1].b0 + 31) & 31))) >> (v521[-1].b0 & 31));
            *(uint16_t *)&v521[-1].w2 = v185
                                 + ((32 * ((v186 > deadzone_hi) - (uint32_t)(v186 < deadzone_lo)) + v186 + 4) >> 3);
            v187 = v523->w2;
            v188 = v570 - ((v187 + (1 << ((v523->b0 + 31) & 31))) >> (v523->b0 & 31));
            *(uint16_t *)&v523->w2 = v187
                                 + ((32 * ((v188 > deadzone_hi) - (uint32_t)(v188 < deadzone_lo)) + v188 + 4) >> 3);
            v189 = v518->w2;
            v190 = v550;
            v191 = v550 - ((v189 + (1 << ((v518->b0 + 31) & 31))) >> (v518->b0 & 31));
            *(uint16_t *)&v518->w2 = v189
                                 + ((32 * ((v191 > deadzone_hi) - (uint32_t)(v191 < deadzone_lo)) + v191 + 2) >> 2);
            v192 = v519->w2;
            v193 = v190 - ((v192 + (1 << ((v519->b0 + 31) & 31))) >> (v519->b0 & 31));
            *(uint16_t *)&v519->w2 = v192
                                 + ((32 * ((v193 > deadzone_hi) - (uint32_t)(v193 < deadzone_lo)) + v193 + 4) >> 3);
            v194 = v518[-1].w2;
            v195 = v190 - ((v194 + (1 << ((v518[-1].b0 + 31) & 31))) >> (v518[-1].b0 & 31));
            *(uint16_t *)&v518[-1].w2 = v194
                                 + ((32 * ((v195 > deadzone_hi) - (uint32_t)(v195 < deadzone_lo)) + v195 + 4) >> 3);
            v196 = v520->w2;
            v197 = v570 - ((v196 + (1 << ((v520->b0 + 31) & 31))) >> (v520->b0 & 31));
            *(uint16_t *)&v520->w2 = v196
                                 + ((32 * ((v197 > deadzone_hi) - (uint32_t)(v197 < deadzone_lo)) + v197 + 4) >> 3);
            v198 = v515->w2;
            v199 = v550;
            v200 = v550 - ((v198 + (1 << ((v515->b0 + 31) & 31))) >> (v515->b0 & 31));
            *(uint16_t *)&v515->w2 = v198
                                 + ((32 * ((v200 > deadzone_hi) - (uint32_t)(v200 < deadzone_lo)) + v200 + 2) >> 2);
            v201 = v516->w2;
            v202 = v199 - ((v201 + (1 << ((*(uint8_t *)&v516->b0 + 31) & 31))) >> (*(uint8_t *)&v516->b0 & 31));
            v550 = v199;
            *(uint16_t *)&v516->w2 = v201
                                 + ((32 * ((v202 > deadzone_hi) - (uint32_t)(v202 < deadzone_lo)) + v202 + 4) >> 3);
            v203 = v515[-1].w2;
            v204 = v199 - ((v203 + (1 << ((v515[-1].b0 + 31) & 31))) >> (v515[-1].b0 & 31));
            *(uint16_t *)&v515[-1].w2 = v203
                                 + ((32 * ((v204 > deadzone_hi) - (uint32_t)(v204 < deadzone_lo)) + v204 + 4) >> 3);
            v205 = v517->w2;
            LOBYTE(v203) = v517->b0;
            v571 = -v199;
            v206 = -v199 - ((v205 + (1 << ((v203 + 31) & 31))) >> (v203 & 31));
            *(uint16_t *)&v517->w2 = v205
                                 + ((32 * ((v206 > deadzone_hi) - (uint32_t)(v206 < deadzone_lo)) + v206 + 4) >> 3);
            v207 = v512->w2;
            v208 = v550;
            v209 = v550 - ((v207 + (1 << ((*(uint8_t *)&v512->b0 + 31) & 31))) >> (*(uint8_t *)&v512->b0 & 31));
            *(uint16_t *)&v512->w2 = v207
                                 + ((32 * ((v209 > deadzone_hi) - (uint32_t)(v209 < deadzone_lo)) + v209 + 2) >> 2);
            v210 = v513->w2;
            v211 = v208 - ((v210 + (1 << ((*(uint8_t *)&v513->b0 + 31) & 31))) >> (*(uint8_t *)&v513->b0 & 31));
            *(uint16_t *)&v513->w2 = v210
                                 + ((32 * ((v211 > deadzone_hi) - (uint32_t)(v211 < deadzone_lo)) + v211 + 4) >> 3);
            v212 = v512[-1].w2;
            v213 = v208 - ((v212 + (1 << ((*(uint8_t *)&v512[-1].b0 + 31) & 31))) >> (*(uint8_t *)&v512[-1].b0 & 31));
            *(uint16_t *)&v512[-1].w2 = v212
                                 + ((32 * ((v213 > deadzone_hi) - (uint32_t)(v213 < deadzone_lo)) + v213 + 4) >> 3);
            v214 = v514->w2;
            v215 = v571 - ((v214 + (1 << ((*(uint8_t *)&v514->b0 + 31) & 31))) >> (*(uint8_t *)&v514->b0 & 31));
          }
          *(uint16_t *)&v514->w2 = v214
                               + ((32 * ((v215 > deadzone_hi) - (uint32_t)(v215 < deadzone_lo)) + v215 + 4) >> 3);
        }
      }
    }
    ++n5;
  }
  while ( n5 < 5 );
  v385 = (AltP2Block *)(v578);
  v386 = v578->f278704;
  v578->f278736[0] += 18;
  v387 = ((uint16_t *)&((uint32_t *)v385)[2 * v386]);
  v385->f278736[1] += 18;
  v385->f278736[2] += 18;
  v385->f278736[3] += 18;
  v385->f278736[4] += 18;
  n0x10 = v387[470032 + 4];
  if ( n0x10 > 0x10 )
  {
    v389 = (uint16_t *)((uint32_t *)v385)[(a4 & 1) + 69677];
    v511 = a4 & 1;
    n15 = v386 & 0xF;
    v508 = v389;
    if ( n15 < 15 )
    {
      v391 = v387[470040 + 1];
      v392 = v387[470040 + 2];
      n2 = v387 + 470040;
      if ( v387[470040 + 3] + v392 + v391 > 29696 )
        __rescale_three_way((uint16_t *)v387 + 470040);
      v393 = (10 * (uint32_t)v387[470040 + 0]) >> 4;
      if ( a4 )
      {
        n2[3 - v511] += v393;
        __update_binary_pair((uint16_t *)model_tables + 254 * (uint32_t)v508 + 254, (a4 - 1) >> 1);
      }
      else
      {
        n2[1] += v393;
      }
      if ( n15 <= 0 )
      {
LABEL_37:
        n0x10 = v385->f278704;
        if ( LOWORD((*(uint32_t *)&v385->f940072[4 * n0x10])) <= 0x1Au )
          return n0x10;
        v397 = v385->f278760[8];
        v398 = 2 - (*((uint8_t *)v385 + (uint8_t)-a5 + 279984) & 1);
        if ( !*((uint8_t *)v385 + (uint8_t)-a5 + 279984) )
          v398 = *((uint8_t *)v385 + (uint8_t)-a5 + 279984);
        v399 = v385->f278760[12];
        v400 = v385->f278760[6] + (v385->f278704 & 0x3F);
        v510 = v398;
        n0x10 = ((uint32_t *)v385)[-v385->f278760[20] + 69713]
              + ((uint32_t *)v385)[-v385->f278760[16] + 69709]
              + v385->f278760[-v399 + 15]
              + v385->f278760[-v397 + 11]
              + v400;
        n0x10_3 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10];
        p2_rec = n0x10_3;
        if ( n15 < 15 )
        {
          n2_2 = n0x10_3 + 470040;
          v403 = n0x10_3[470041];
          v404 = n0x10_3[470042];
          n2 = n2_2;
          if ( n2_2[3] + v404 + v403 > 29696 )
          {
            n0x10_1 = n0x10;
            __rescale_three_way((uint16_t *)n2_2);
            n0x10 = n0x10_1;
          }
          v405 = p2_rec[470040] & 0xFFFC;
          n2[v510 + 1] += v405 >> 2;
          if ( n15 <= 0 )
            goto LABEL_48;
        }
        else
        {
        }
        v406 = p2_rec[470033];
        v407 = p2_rec[470034];
        n2 = &p2_rec[470032];
        if ( p2_rec[470035] + v407 + v406 > 29696 )
        {
          n0x10_1 = n0x10;
          __rescale_three_way(&p2_rec[470032]);
          n0x10 = n0x10_1;
        }
        n2[v510 + 1] += (uint16_t)(p2_rec[470032] & 0xFFFC) >> 2;
LABEL_48:
        if ( a4 )
        {
          n2_half = (a4 - 1) >> 1;
          n0xF0 = (((uint16_t *)((uint8_t)(uintptr_t)v508 & 0xF0)));
          if ( (uint32_t)n0xF0 >= 0xF0
            || (n0x10_1 = n0x10,
                __update_binary_pair((uint16_t *)model_tables + 254 * (uint32_t)v508 + 4064, n2_half),
                n0x10 = n0x10_1,
                (int32_t)n0xF0 > 0) )
          {
            n0x10_1 = n0x10;
            __update_binary_pair((uint16_t *)model_tables + 254 * (uint32_t)v508 - 4064, n2_half);
            n0x10 = n0x10_1;
          }
        }
        v408 = &p2_rec[470036];
        if ( p2_rec[470039]
           + p2_rec[470038]
           + p2_rec[470037] > 29696 )
        {
          n0x10_1 = n0x10;
          __rescale_three_way(&p2_rec[470036]);
          n0x10 = n0x10_1;
        }
        ((uint16_t *)v408)[v510 + 1] += (6 * (uint32_t)p2_rec[470036]) >> 4;
        if ( !v385->f278728 || LOWORD((*(uint32_t *)&v385->f940072[4 * v385->f278704])) > 0x100u )
        {
          v409 = 2 - v511;
          if ( !a4 )
            v409 = 0;
          v410 = v385->f278760[4];
          v511 = v409;
          if ( v410 == 1 )
          {
            v491 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[5] + 235018 + 2 * (n0x10 - v385->f278760[6])];
            v492 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[5] + 235018 + 2 * (n0x10 - v385->f278760[6])]);
            v493 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[5] + 235019 + 2 * (n0x10 - v385->f278760[6])]);
            v508 = v491;
            if ( v491[3] + v493 + v492 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v491);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v508)[v510 + 1] += (uint16_t)(*v508 & 0xFFFC) >> 2;
            v494 = v385->f278704 - v385->f278760[6];
            n0xF0_1 = ((uint16_t *)&((uint32_t *)v385)[2 * v494 + 2 * v385->f278760[5]]);
            n0x10_2 = v385->f278760[7] + v494;
            n0xF0 = n0xF0_1;
            n2_3 = (uint16_t *)n0xF0_1 + 470036;
            v497 = n2_3[2] + n2_3[1];
            v498 = n2_3[3];
            n2 = n2_3;
            if ( v498 + v497 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)n2_3);
              n0x10 = n0x10_1;
            }
            v499 = 3 * n0xF0[470036];
            n2[v511 + 1] += v499 >> 4;
            if ( n15 >= 15 )
              goto LABEL_180;
            v500 = n0xF0[470041];
            v501 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v501 + v500 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v502 = &v385->f940072[4 * n0x10_2 + 4];
            if ( HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + LOWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 4])) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&v385->f940072[4 * n0x10_2 + 4]);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v502)[v511 + 1] += (uint16_t)(*v502 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_180:
              v503 = n0xF0[470033];
              v504 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v504 + v503 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v411 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[6] - v385->f278760[-v410 + 7])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[6] - v385->f278760[-v410 + 7])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[6] - v385->f278760[-v410 + 7])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[6] - v385->f278760[-v410 + 7])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v411);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v411)[v510 + 1] += (7 * (uint32_t)*v411) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[6] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[4] + 69695])]));
            v412 = n0xF0[470037];
            v413 = n0xF0[470038];
            n2 = n0xF0 + 470036;
            if ( n0xF0[470039] + v413 + v412 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            v414 = 7 * n0xF0[470036];
            n2[v511 + 1] += v414 >> 4;
            if ( n15 >= 15 )
              goto LABEL_67;
            v415 = n0xF0[470041];
            v416 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v416 + v415 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n0x10_1 = n0x10;
            n2[v511 + 1] = n2[v511 + 1] + ((5 * (uint32_t)n0xF0[470040]) >> 4);
            n0x10 = n0x10_1;
            if ( n15 > 0 )
            {
LABEL_67:
              v417 = n0xF0[470033];
              v418 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v418 + v417 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v419 = v385->f278760[8];
          if ( v419 == 1 )
          {
            n2_4 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[9] + 235018 + 2 * (n0x10 - v385->f278760[10])];
            v481 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[9] + 235018 + 2 * (n0x10 - v385->f278760[10])]);
            v482 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[9] + 235019 + 2 * (n0x10 - v385->f278760[10])]);
            n2 = n2_4;
            if ( n2_4[3] + v482 + v481 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)n2_4);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v483 = v385->f278704 - v385->f278760[10];
            n0xF0_2 = ((uint16_t *)&((uint32_t *)v385)[2 * v483 + 2 * v385->f278760[9]]);
            n0x10_2 = v385->f278760[11] + v483;
            n0xF0 = n0xF0_2;
            v485 = (uint16_t *)n0xF0_2 + 470036;
            if ( v485[3] + v485[2] + v485[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v485);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v485)[v511 + 1] += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_167;
            v486 = n0xF0[470041];
            v487 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v487 + v486 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v488 = &v385->f940072[4 * n0x10_2 + 4];
            if ( HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + LOWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 4])) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&v385->f940072[4 * n0x10_2 + 4]);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v488)[v511 + 1] += (uint16_t)(*v488 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_167:
              v489 = n0xF0[470033];
              v490 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v490 + v489 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v420 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[10] - v385->f278760[-v419 + 11])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[10] - v385->f278760[-v419 + 11])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[10] - v385->f278760[-v419 + 11])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[10] - v385->f278760[-v419 + 11])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v420);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v420)[v510 + 1] += (7 * (uint32_t)*v420) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[10] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[8] + 69699])]));
            v421 = n0xF0[470037];
            v422 = n0xF0[470038];
            n2 = n0xF0 + 470036;
            if ( n0xF0[470039] + v422 + v421 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_79;
            v423 = n0xF0[470041];
            v424 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v424 + v423 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_79:
              v425 = n0xF0[470033];
              v426 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v426 + v425 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v427 = v385->f278760[12];
          if ( v427 == 1 )
          {
            n2_5 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[13] + 235018 + 2 * (n0x10 - v385->f278760[14])];
            v470 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[13] + 235018 + 2 * (n0x10 - v385->f278760[14])]);
            v471 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[13] + 235019 + 2 * (n0x10 - v385->f278760[14])]);
            n2 = n2_5;
            if ( n2_5[3] + v471 + v470 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)n2_5);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v472 = v385->f278704 - v385->f278760[14];
            n0xF0_3 = ((uint16_t *)&((uint32_t *)v385)[2 * v472 + 2 * v385->f278760[13]]);
            n0x10_2 = v385->f278760[15] + v472;
            n0xF0 = n0xF0_3;
            v474 = (uint16_t *)n0xF0_3 + 470036;
            if ( v474[3] + v474[2] + v474[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v474);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v474)[v511 + 1] += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_154;
            v475 = n0xF0[470041];
            v476 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v476 + v475 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v477 = &v385->f940072[4 * n0x10_2 + 4];
            if ( HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + LOWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 4])) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&v385->f940072[4 * n0x10_2 + 4]);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v477)[v511 + 1] += (uint16_t)(*v477 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_154:
              v478 = n0xF0[470033];
              v479 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v479 + v478 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v428 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[14] - v385->f278760[-v427 + 15])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[14] - v385->f278760[-v427 + 15])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[14] - v385->f278760[-v427 + 15])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[14] - v385->f278760[-v427 + 15])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v428);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v428)[v510 + 1] += (7 * (uint32_t)*v428) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[14] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[12] + 69703])]));
            v429 = n0xF0[470037];
            v430 = n0xF0[470038];
            n2 = n0xF0 + 470036;
            if ( n0xF0[470039] + v430 + v429 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_91;
            v431 = n0xF0[470041];
            v432 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v432 + v431 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_91:
              v433 = n0xF0[470033];
              v434 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v434 + v433 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v435 = v385->f278760[16];
          if ( v435 == 1 )
          {
            n2_6 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[17] + 235018 + 2 * (n0x10 - v385->f278760[18])];
            v459 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[17] + 235018 + 2 * (n0x10 - v385->f278760[18])]);
            v460 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[17] + 235019 + 2 * (n0x10 - v385->f278760[18])]);
            n2 = n2_6;
            if ( n2_6[3] + v460 + v459 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)n2_6);
              n0x10 = n0x10_1;
            }
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v461 = v385->f278704 - v385->f278760[18];
            n0xF0_4 = ((uint16_t *)&((uint32_t *)v385)[2 * v461 + 2 * v385->f278760[17]]);
            n0x10_2 = v385->f278760[19] + v461;
            n0xF0 = n0xF0_4;
            v463 = (uint16_t *)n0xF0_4 + 470036;
            if ( v463[3] + v463[2] + v463[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v463);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v463)[v511 + 1] += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_141;
            v464 = n0xF0[470041];
            v465 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v465 + v464 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v466 = &v385->f940072[4 * n0x10_2 + 4];
            if ( HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + LOWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 6]))
               + HIWORD((*(uint32_t *)&v385->f940072[4 * n0x10_2 + 4])) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way(&v385->f940072[4 * n0x10_2 + 4]);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v466)[v511 + 1] += (uint16_t)(*v466 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_141:
              v467 = n0xF0[470033];
              v468 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v468 + v467 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v436 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[18] - v385->f278760[-v435 + 19])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[18] - v385->f278760[-v435 + 19])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[18] - v385->f278760[-v435 + 19])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[18] - v385->f278760[-v435 + 19])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)v436);
              n0x10 = n0x10_1;
            }
            ((uint16_t *)v436)[v510 + 1] += (7 * (uint32_t)*v436) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[18] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[16] + 69707])]));
            v437 = n0xF0[470037];
            v438 = n0xF0[470038];
            n2 = n0xF0 + 470036;
            if ( n0xF0[470039] + v438 + v437 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_103;
            v439 = n0xF0[470041];
            v440 = n0xF0[470042];
            n2 = n0xF0 + 470040;
            if ( n0xF0[470043] + v440 + v439 > 29696 )
            {
              n0x10_1 = n0x10;
              __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n2[v511 + 1] += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_103:
              v441 = n0xF0[470033];
              v442 = n0xF0[470034];
              n2 = n0xF0 + 470032;
              if ( n0xF0[470035] + v442 + v441 > 29696 )
              {
                n0x10_1 = n0x10;
                __rescale_three_way((uint16_t *)(uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              n2[v511 + 1] += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v443 = v385->f278760[20];
          if ( v443 == 1 )
          {
            n2_7 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[21] + 235018 + 2 * (n0x10 - v385->f278760[22])];
            v447 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[21] + 235018 + 2 * (n0x10 - v385->f278760[22])]);
            v448 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[21] + 235019 + 2 * (n0x10 - v385->f278760[22])]);
            v449 = n2_7[3];
            n2 = n2_7;
            if ( v449 + v448 + v447 > 29696 )
              __rescale_three_way((uint16_t *)n2_7);
            n2[v510 + 1] += (uint16_t)(n2[0] & 0xFFFC) >> 2;
            v450 = v385->f278704 - v385->f278760[22];
            n0x10_4 = (uint16_t *)&((uint32_t *)v385)[2 * v450 + 2 * v385->f278760[21]];
            n0xF0 = (((uint16_t *)(v385->f278760[23] + v450)));
            p2_rec = n0x10_4;
            v452 = n0x10_4 + 470036;
            if ( n0x10_4[470039] + n0x10_4[470038] + n0x10_4[470037] > 29696 )
              __rescale_three_way((uint16_t *)v452);
            ((uint16_t *)v452)[v511 + 1] += (3 * (uint32_t)p2_rec[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_128;
            v453 = p2_rec[470043];
            v454 = p2_rec[470042] + p2_rec[470041];
            n2 = &p2_rec[470040];
            if ( v453 + v454 > 29696 )
              __rescale_three_way(&p2_rec[470040]);
            n0xF0_5 = (uint16_t *)(n0xF0);
            n2[v511 + 1] += (uint16_t)(p2_rec[470040] & 0xFFFC) >> 2;
            v456 = &v385->f940072[4 * (uint32_t)n0xF0_5 + 4];
            if ( v456[3] + v456[2] + v456[1] > 29696 )
              __rescale_three_way((uint16_t *)v456);
            n0x10 = (uint16_t)(*v456 & 0xFFF8) >> 3;
            ((uint16_t *)v456)[v511 + 1] += n0x10;
            if ( n15 > 2 )
            {
LABEL_128:
              v457 = &p2_rec[470032];
              if ( p2_rec[470035]
                 + p2_rec[470034]
                 + p2_rec[470033] > 29696 )
                __rescale_three_way(&p2_rec[470032]);
              n0x10 = (uintptr_t)p2_rec;   // the slot's last value, and what this path returns
              ((uint16_t *)v457)[v511 + 1] += (6
                                                               * (uint32_t)p2_rec[470032]) >> 4;
            }
          }
          else
          {
            v444 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[22] - v385->f278760[-v443 + 23])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[22] - v385->f278760[-v443 + 23])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[22] - v385->f278760[-v443 + 23])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[22] - v385->f278760[-v443 + 23])]) > 29696 )
              __rescale_three_way((uint16_t *)v444);
            ((uint16_t *)v444)[v510 + 1] += (7 * (uint32_t)*v444) >> 4;
            v445 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[22] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[20] + 69711])]));
            if ( v445[470039] + v445[470038] + v445[470037] > 29696 )
              __rescale_three_way((uint16_t *)(uint16_t *)v445 + 470036);
            ((uint16_t *)v445)[v511 + 470037] += (7 * (uint32_t)v445[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_115;
            if ( v445[470043] + v445[470042] + v445[470041] > 29696 )
              __rescale_three_way((uint16_t *)(uint16_t *)v445 + 470040);
            n0x10 = v445[470040];
            ((uint16_t *)v445)[v511 + 470041] += (5 * n0x10) >> 4;
            if ( n15 > 0 )
            {
LABEL_115:
              if ( v445[470035] + v445[470034] + v445[470033] > 29696 )
                __rescale_three_way((uint16_t *)(uint16_t *)v445 + 470032);
              n0x10 = v445[470032];
              ((uint16_t *)v445)[v511 + 470033] += (6 * n0x10) >> 4;
            }
          }
        }
        return n0x10;
      }
      v387 = ((uint16_t *)&((uint32_t *)v385)[2 * v385->f278704]);
    }
    v394 = v387[470032 + 1];
    v395 = v387[470032 + 2];
    n2 = v387 + 470032;
    if ( v387[470032 + 3] + v395 + v394 > 29696 )
      __rescale_three_way((uint16_t *)v387 + 470032);
    v396 = (13 * (uint32_t)v387[470032 + 0]) >> 4;
    if ( a4 )
    {
      n2[3 - v511] += v396;
      __update_binary_pair((uint16_t *)model_tables + 254 * (uint32_t)v508 - 254, (a4 - 1) >> 1);
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
  int32_t v95;
  // These shared `__frame.v95` with the name that still binds it: one
  // stack slot MSVC gave to locals whose live ranges do not overlap, and
  // Hex-Rays named every use.  That they can have storage of their own is
  // the gate's answer -- nothing writes one of them and reads another.
  uint32_t v96;
  uint8_t *v97;
  ;
  uint8_t *v11;
  P2Ctx *v25;   // a record cursor
  P2Ctx *v32;   // a record cursor
  P2Ctx *v54;   // a record cursor
  P2Ctx *v62;   // a record cursor
  P2Ctx *v69;   // a record cursor
  P2Ctx *v80;   // a record cursor
  P2Ctx *v84;   // a record cursor
  P2Ctx *v88;   // a record cursor
  uintptr_t v21, v28, v48, v58, v66, v76, v78, v82, v86, v90, v93;
  bool v17;
  int16_t v14, v24;
  uint8_t *v12;
  int32_t i_1, v13, v22, v23, v29, v30, v31, v37, v49, *v50, v51, v52, v59,
          v60, v61, v67, v68, v72, v73, v74, v75, v77, v79, v81, v83, v85,
          v87, v89, v92, v94;
  int64_t v16;
  uint16_t *v15;
  uint32_t j;
  __rc_begin_decode(ArgList);
  i_1 = i;
  v11 = (uint8_t *)(lpAddress->f278736[0]);
  v12 = (uint8_t *)(v11);
  if ( i > 0 )
  {
    v95 = 0;
    while ( 1 )
    {
      v13 = *(int16_t *)((uintptr_t)v11 - 18) >> 4;
      lpAddress->f278708 = lpAddress->f278704 + (*(uint32_t *)&lpAddress->f278944[v13 + 4]);
      lpAddress->f278712 = lpAddress->f278704 + (*(uint32_t *)&lpAddress->f278944[v13]);
      v14 = (uint8_t)((*(uint16_t *)(lpAddress->f278736[0] - 18) >> 4)
                            + *(uint8_t *)(__alt_p2_decode_symbol((uint16_t *)((uintptr_t)lpAddress
                                                              + 8
                                                              * (lpAddress->f278704
                                                               + *(uint32_t *)((uintptr_t)lpAddress
                                                                           + 4
                                                                           * ((*(int16_t *)((uintptr_t)v12 - 18) <= *(int16_t *)((uintptr_t)v12 - 36))
                                                                            + (*(int16_t *)((uintptr_t)v12 - 18) < *(int16_t *)((uintptr_t)v12 - 36)))
                                                                           + 278828)
                                                               + *(uint32_t *)((uintptr_t)lpAddress
                                                                           + 4 * *(uint8_t *)((uintptr_t)v12 - 20)
                                                                           + 278812)
                                                               + *(uint32_t *)((uintptr_t)lpAddress
                                                                           + 4 * *(uint8_t *)((uintptr_t)v12 - 2)
                                                                           + 278796)
                                                               + *(uint32_t *)((uintptr_t)lpAddress
                                                                           + 4
                                                                           * (((uint32_t)(v13 - 115) >> 31)
                                                                            + ((uint32_t)(v13 - 17) >> 31))
                                                                           + 278780)
                                                               + lpAddress->f278760[22])
                                                              + 940072), (uint8_t *)lpAddress + 278708)
                                       + (uintptr_t)lpAddress
                                       + 280496));
      *a5 = v14;
      v14 *= 16;
      **(uint16_t **)&lpAddress->f278736[0] = v14;
      ++a5;
      *(uint16_t *)(lpAddress->f278736[0] + 2) = v14;
      v15 = *(uint16_t **)&lpAddress->f278736[0];
      v16 = (int16_t)(*v15 - *(v15 - 9));
      v15[2] = v16;
      LOWORD(v16) = (WORD2(v16) ^ v16) - WORD2(v16);
      *(uint16_t *)(lpAddress->f278736[0] + 6) = v16;
      *(uint16_t *)(lpAddress->f278736[0] + 14) = v16;
      *(uint16_t *)(lpAddress->f278736[0] + 12) = v16;
      *(uint16_t *)(lpAddress->f278736[0] + 10) = v16;
      *(uint16_t *)(lpAddress->f278736[0] + 8) = (uint32_t)*(int16_t *)(lpAddress->f278736[0] + 10) >> 1;
      *(lpAddress->f278736[0] + 17) = 2;
      *(lpAddress->f278736[0] + 16) = (*(int16_t *)(lpAddress->f278736[0] + 4) <= 0)
                                                       + (*(int16_t *)(lpAddress->f278736[0] + 4) < 0);
      v11 = (lpAddress->f278736[0] + 18);
      v17 = v95 + 1 < i;
      lpAddress->f278736[0] = (uint8_t *)(uint8_t *)v11;
      ++v95;
      if ( !v17 )
        break;
      v12 = (uint8_t *)(lpAddress->f278736[0]);
    }
    i_1 = i;
  }
  ((P2Ctx *)v11)[0] = ((P2Ctx *)v11)[-1];
  v21 = (uintptr_t)(lpAddress->f278736[0]);
  v22 = *(uint32_t *)(v21 - 10);
  v23 = *(uint32_t *)(v21 - 6);
  *(uint32_t *)(v21 + 18) = *(uint32_t *)(v21 - 18);
  *(uint32_t *)(v21 + 22) = *(uint32_t *)(v21 - 14);
  v24 = *(uint16_t *)(v21 - 2);
  *(uint32_t *)(v21 + 26) = v22;
  *(uint32_t *)(v21 + 30) = v23;
  *(uint16_t *)(v21 + 34) = v24;
  v25 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[0]);
  v25[2] = v25[-1];
  v28 = (uintptr_t)(lpAddress->f278736[0]);
  v29 = *(uint32_t *)(v28 - 14);
  v30 = *(uint32_t *)(v28 - 10);
  *(uint32_t *)(v28 + 54) = *(uint32_t *)(v28 - 18);
  v31 = *(uint32_t *)(v28 - 6);
  *(uint32_t *)(v28 + 58) = v29;
  LOWORD(v29) = *(uint16_t *)(v28 - 2);
  *(uint32_t *)(v28 + 62) = v30;
  *(uint32_t *)(v28 + 66) = v31;
  *(uint16_t *)(v28 + 70) = v29;
  v32 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[0]);
  v32[4] = v32[-1];
  v37 = -18 * i_1;
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec1 = (P2Ctx *)((uint8_t *)(lpAddress->f278736[0]) + v37);
  rec1[-1] = rec1[0];
  rec1[-2] = rec1[1];
  rec1[-3] = rec1[2];
  rec1[-4] = rec1[3];
  rec1[-5] = rec1[4];
  rec1[-6] = rec1[5];
  rec1[-7] = rec1[6];
  rec1[-8] = rec1[7];
  memcpy(*(uint8_t **)&lpAddress->f278760[0],lpAddress->f278756,18 * i_1 + 234);
  memcpy(*(uint8_t **)&lpAddress->f278760[1],lpAddress->f278756,18 * i_1 + 234);
  memcpy(*(uint8_t **)&lpAddress->f278760[2],lpAddress->f278756,18 * i_1 + 234);
  if ( a7 > 1 )
  {
    v96 = 0;
    do
    {
      v48 = (uintptr_t)(int32_t *)lpAddress->f278668;
      v49 = *(uint32_t *)(v48 - 4);
      v97 = a5;
      *(uint32_t *)(v48 + 4) = v49;
      *(uint32_t *)lpAddress->f278668 = v49;
      v50 = lpAddress->f278660;
      v51 = (int32_t)(uintptr_t)(int32_t *)lpAddress->f278664;
      lpAddress->f278660 = (int32_t *)v51;
      lpAddress->f278664 = (uint8_t *)v50;
      v51 += 8;
      v50 += 2;
      lpAddress->f278668 = (uint8_t *)v51;
      lpAddress->f278672 = (uint8_t *)v50;
      v52 = *v50;
      *(uint32_t *)(v51 - 4) = *v50;
      *(uint32_t *)(lpAddress->f278668 - 8) = v52;
      lpAddress->f278528_q = 0;
      lpAddress->f278536 = 0;
      lpAddress->f278540 = 0;
      lpAddress->f278542 = 0;
      lpAddress->f278640 = 0;
      lpAddress->f278648 = 0;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(lpAddress->p2_row, 0, sizeof lpAddress->p2_row);
      v54 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[0]);
      v54[0] = v54[-1];
      v58 = (uintptr_t)(lpAddress->f278736[0]);
      v59 = *(uint32_t *)(v58 - 32);
      v60 = *(uint32_t *)(v58 - 28);
      LOWORD(v51) = *(uint16_t *)(v58 - 20);
      *(uint32_t *)(v58 + 18) = *(uint32_t *)(v58 - 36);
      v61 = *(uint32_t *)(v58 - 24);
      *(uint32_t *)(v58 + 22) = v59;
      *(uint32_t *)(v58 + 26) = v60;
      *(uint32_t *)(v58 + 30) = v61;
      *(uint16_t *)(v58 + 34) = v51;
      v62 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[0]);
      v62[2] = v62[-3];
      v66 = (uintptr_t)(lpAddress->f278736[0]);
      v67 = *(uint32_t *)(v66 - 64);
      v68 = *(uint32_t *)(v66 - 60);
      LOWORD(v51) = *(uint16_t *)(v66 - 56);
      *(uint32_t *)(v66 + 54) = *(uint32_t *)(v66 - 72);
      *(uint32_t *)(v66 + 58) = *(uint32_t *)(v66 - 68);
      *(uint32_t *)(v66 + 62) = v67;
      *(uint32_t *)(v66 + 66) = v68;
      *(uint16_t *)(v66 + 70) = v51;
      v69 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[0]);
      v69[4] = v69[-5];
      v72 = lpAddress->f278760[2];
      v73 = lpAddress->f278760[3];
      v74 = lpAddress->f278760[1];
      v75 = lpAddress->f278760[0];
      v76 = *(uint32_t *)&lpAddress->f278756;
      lpAddress->f278760[3] = v72;
      lpAddress->f278760[2] = v74;
      lpAddress->f278760[1] = v75;
      *(uint32_t *)&lpAddress->f278756 = v73;
      lpAddress->f278760[0] = v76;
      v73 += 144;
      lpAddress->f278736[0] = (uint8_t *)(v73);
      v76 += 144;
      lpAddress->f278736[1] = (uint8_t *)(v76);
      lpAddress->f278736[2] = (uint8_t *)(v75 + 144);
      lpAddress->f278736[3] = (uint8_t *)(v74 + 144);
      lpAddress->f278736[4] = (uint8_t *)(v72 + 144);
      ((P2Ctx *)v73)[-1] = ((P2Ctx *)v76)[0];
      v77 = (int32_t)(lpAddress->f278736[0]);
      v78 = (uintptr_t)(lpAddress->f278736[1]);
      ((P2Ctx *)v77)[-2] = ((P2Ctx *)v78)[1];
      v79 = (int32_t)(lpAddress->f278736[0]);
      v80 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[1]);
      ((P2Ctx *)v79)[-3] = v80[2];
      v81 = (int32_t)(lpAddress->f278736[0]);
      v82 = (uintptr_t)(lpAddress->f278736[1]);
      ((P2Ctx *)v81)[-4] = ((P2Ctx *)v82)[3];
      v83 = (int32_t)(lpAddress->f278736[0]);
      v84 = (P2Ctx *)(uint8_t *)(lpAddress->f278736[1]);
      ((P2Ctx *)v83)[-5] = v84[4];
      v85 = (int32_t)(lpAddress->f278736[0]);
      v86 = (uintptr_t)(lpAddress->f278736[1]);
      ((P2Ctx *)v85)[-6] = ((P2Ctx *)v86)[5];
      v87 = (int32_t)(lpAddress->f278736[0]);
      v88 = (P2Ctx *)lpAddress->f278736[1];
      ((P2Ctx *)v87)[-7] = v88[6];
      v89 = (int32_t)(lpAddress->f278736[0]);
      v90 = (uintptr_t)(lpAddress->f278736[1]);
      ((P2Ctx *)v89)[-8] = ((P2Ctx *)v90)[7];
      *(uint16_t *)(lpAddress->f278736[0] + 2) = 0;
      if ( i > 0 )
      {
        for ( j = 0; j < i; ++j )
        {
          v92 = __alt_p2_context((AltP2Block *)lpAddress, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          v93 = __alt_p2_decode_symbol((uint16_t *)((uintptr_t)lpAddress + 8 * lpAddress->f278704 + 940072), (uint8_t *)lpAddress + 278708);
          v94 = (uint8_t)(v92 + (*(uint8_t *)&lpAddress->f280496[v93]));
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
  __alt_p2_d8_decode_body((AltP2Block *)(int32_t)lpAddress, i, (uint8_t *)Src, i, a5);
  if ( lpAddress )
    __alt_p2_free((void **)lpAddress, 1);
}

int32_t __alt_model_p2_decode(uint16_t *p_i, uint8_t *Src)
{
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
  uint32_t Size_1;
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
  int32_t v168;
  int32_t v169;
  int32_t v170;
  uint32_t v171;
  ;
  AltP2Block *v56;
  AltP2Block *v14;
  AltP2Block *v25;
  P2Ctx *v26;   // a record cursor
  uint8_t *v34;
  P2Ctx *v41;   // a record cursor
  P2Ctx *v63;   // a record cursor
  P2Ctx *v71;   // a record cursor
  P2Ctx *v78;   // a record cursor
  P2Ctx *v89;   // a record cursor
  P2Ctx *v93;   // a record cursor
  uint8_t *v30, *v38, *v57, *v67, *v75, *v85;
  P2Ctx *v99;   // a record cursor
  P2Ctx *v97;   // a record cursor
  P2Ctx *v95;   // a record cursor
  P2Ctx *v91;   // a record cursor
  P2Ctx *v87;   // a record cursor
  int32_t v46;   // an offset from v45, not a cursor
  AltP2Block *v120;
  AltP2Block *v129;
  AltP2Block *v111;
  bool v17, v109;
  uint8_t v9;
  int16_t v110;
  uint8_t *v21, *v22, *v23, *v24, *v60, *v81, *v82, *v83, *v84, *v86, *v88, *v90,
          *v92, *v94, *v96, *v98;   // row cursors
  int32_t i, v5, n4, n4_1, v15, v16, v31, v32, v33, v39,
          v40, v58, *v59, v61, v68,
          v69, v70, v76, v77, v101, v103, v104, v105, v106, v107, v108, v112, v113,
          v114, v115, v116, v117, v118, v119, v121, v122, v123, v124, v125,
          v126, v127, v128, v130, v131, v132, v133, v134, v135, n4_3, n4_4;
  uint16_t *v102;
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
  v9 = plane_desc[plane_desc[4].src_plane + 1].flags;
  ArgList = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  v153 = v9 & 8;
  __rc_begin_decode(ArgList);
  if ( v5 > 0 )
  {
    v11 = 0;
    v149 = 9 * i;
    v160 = -18 * i;
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
        Src_1 = (uint8_t *)Src;
        do
        {
          ++n4_2;
          if ( v165 )
          {
            v18 = &plane[n4_2];
            if ( v165 == 1 )
            {
              v25 = (AltP2Block *)((int32_t)*(v18 - 1));
              v26 = (P2Ctx *)(uint8_t *)(v25->f278736[0]);
              v26[0] = v26[-1];
              v30 = v25->f278736[0];
              v31 = *(uint32_t *)(v30 - 14);
              v32 = *(uint32_t *)(v30 - 10);
              *(uint32_t *)(v30 + 18) = *(uint32_t *)(v30 - 18);
              v33 = *(uint32_t *)(v30 - 6);
              *(uint32_t *)(v30 + 22) = v31;
              LOWORD(v31) = *(uint16_t *)(v30 - 2);
              *(uint32_t *)(v30 + 26) = v32;
              *(uint32_t *)(v30 + 30) = v33;
              *(uint16_t *)(v30 + 34) = v31;
              v34 = (uint8_t *)(v25->f278736[0]);
              ((P2Ctx *)v34)[2] = ((P2Ctx *)v34)[-1];
              v38 = v25->f278736[0];
              v39 = *(uint32_t *)(v38 - 10);
              v40 = *(uint32_t *)(v38 - 6);
              *(uint32_t *)(v38 + 54) = *(uint32_t *)(v38 - 18);
              *(uint32_t *)(v38 + 58) = *(uint32_t *)(v38 - 14);
              LOWORD(v34) = *(uint16_t *)(v38 - 2);
              *(uint32_t *)(v38 + 62) = v39;
              *(uint32_t *)(v38 + 66) = v40;
              *(uint16_t *)(v38 + 70) = (uint16_t)(uintptr_t)v34;
              v41 = (P2Ctx *)(uint8_t *)(v25->f278736[0]);
              v41[4] = v41[-1];
              v46 = v160;
              Size_1 = Size;
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec2 = (P2Ctx *)((uint8_t *)(v25->f278736[0]) + v46);
              rec2[-1] = rec2[0];
              rec2[-2] = rec2[1];
              rec2[-3] = rec2[2];
              rec2[-4] = rec2[3];
              rec2[-5] = rec2[4];
              rec2[-6] = rec2[5];
              rec2[-7] = rec2[6];
              rec2[-8] = rec2[7];
              memcpy(v25->f278760_p,v25->f278756,Size_1);
              memcpy(v25->f278764,v25->f278756,Size);
              memcpy(v25->f278768,v25->f278756,Size);
            }
          }
          else
          {
            v14 = (AltP2Block *)((int32_t)plane[n4_2 - 1]);
            v159 = (int32_t)(&plane[n4_2]);
            v15 = 0;
            do
            {
              v16 = 9 * v15;
              *(uint16_t *)(v14->f278736[5] + 2 * v16) = 256;
              v17 = ++v15 < v171;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 2) = 256;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 4) = -16;
              *(v14->f278736[5] + 2 * v16 + 16) = 1;
              *(v14->f278736[5] + 2 * v16 + 17) = 3;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 6) = 512;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 14) = 512;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 12) = 512;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 10) = 1024;
              *(uint16_t *)(v14->f278736[5] + 2 * v16 + 8) = 256;
            }
            while ( v17 );
            v18 = (AltP2Block **)v159;
            memcpy(v14->f278760_p,*&v14->f278736[5],Size);
            memcpy(v14->f278764,*&v14->f278736[5],Size);
            memcpy(v14->f278768,*&v14->f278736[5],Size);
            v21 = (uint8_t *)v14->f278760_p + 2 * v149 + 144;
            v14->f278736[0] = v14->f278736[5] + 2 * v149 + 144;
            v22 = (uint8_t *)v14->f278764;
            v14->f278736[1] = v21;
            v23 = (uint8_t *)v14->f278768;
            v14->f278736[2] = v22 + 2 * v149 + 144;
            v24 = (uint8_t *)v14->f278772 + 2 * v149 + 144;
            v14->f278736[3] = v23 + 2 * v149 + 144;
            v14->f278736[4] = v24;
          }
          v56 = (AltP2Block *)((int32_t)*(v18 - 1));
          v57 = v56->f278668;
          v58 = *(uint32_t *)(v57 - 4);
          *(uint32_t *)(v57 + 4) = v58;
          *(uint32_t *)v56->f278668 = v58;
          v59 = v56->f278660;
          v60 = v56->f278664;
          v56->f278660 = (int32_t *)v60;
          v56->f278664 = (uint8_t *)v59;
          v60 += 8;
          v56->f278668 = v60;
          v59 += 2;
              v56->f278672 = (uint8_t *)v59;
          v61 = *v59;
          *(uint32_t *)(v60 - 4) = *v59;
          *(uint32_t *)(v56->f278668 - 8) = v61;
          v56->f278528_q = 0;
          v56->f278536 = 0;
          v56->f278540 = 0;
          v56->f278542 = 0;
          v56->f278640 = 0;
          v56->f278648 = 0;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(v56->p2_row, 0, sizeof v56->p2_row);
          v63 = (P2Ctx *)(uint8_t *)(v56->f278736[0]);
          v63[0] = v63[-1];
          v67 = v56->f278736[0];
          v68 = *(uint32_t *)(v67 - 32);
          v69 = *(uint32_t *)(v67 - 28);
          *(uint32_t *)(v67 + 18) = *(uint32_t *)(v67 - 36);
          v70 = *(uint32_t *)(v67 - 24);
          *(uint32_t *)(v67 + 22) = v68;
          *(uint32_t *)(v67 + 26) = v69;
          *(uint32_t *)(v67 + 30) = v70;
          *(uint16_t *)(v67 + 34) = *(uint16_t *)(v67 - 20);
          v71 = (P2Ctx *)(uint8_t *)(v56->f278736[0]);
          v71[2] = v71[-3];
          v75 = v56->f278736[0];
          v76 = *(uint32_t *)(v75 - 64);
          v77 = *(uint32_t *)(v75 - 60);
          *(uint32_t *)(v75 + 54) = *(uint32_t *)(v75 - 72);
          *(uint32_t *)(v75 + 58) = *(uint32_t *)(v75 - 68);
          *(uint32_t *)(v75 + 62) = v76;
          *(uint32_t *)(v75 + 66) = v77;
          *(uint16_t *)(v75 + 70) = *(uint16_t *)(v75 - 56);
          v78 = (P2Ctx *)(uint8_t *)(v56->f278736[0]);
          v78[4] = v78[-5];
          v81 = v56->f278736[8];
          v82 = v56->f278736[9];
          v83 = v56->f278736[7];
          v84 = v56->f278736[6];
          v85 = v56->f278736[5];
          v56->f278736[9] = v81;
          v56->f278736[8] = v83;
          v56->f278736[7] = v84;
          v56->f278736[5] = v82;
          v56->f278736[6] = v85;
          v82 += 144;
          v56->f278736[0] = v82;
          v85 += 144;
          v56->f278736[1] = v85;
          v56->f278736[2] = v84 + 144;
          v56->f278736[3] = v83 + 144;
          v56->f278736[4] = v81 + 144;
          ((P2Ctx *)v82)[-1] = ((P2Ctx *)v85)[0];
          v86 = v56->f278736[0];
          v87 = (P2Ctx *)v56->f278736[1];
          ((P2Ctx *)v86)[-2] = v87[1];
          v88 = v56->f278736[0];
          v89 = (P2Ctx *)(uint8_t *)(v56->f278736[1]);
          ((P2Ctx *)v88)[-3] = v89[2];
          v90 = v56->f278736[0];
          v91 = (P2Ctx *)v56->f278736[1];
          ((P2Ctx *)v90)[-4] = v91[3];
          v92 = v56->f278736[0];
          v93 = (P2Ctx *)(uint8_t *)(v56->f278736[1]);
          ((P2Ctx *)v92)[-5] = v93[4];
          v94 = v56->f278736[0];
          v95 = (P2Ctx *)v56->f278736[1];
          ((P2Ctx *)v94)[-6] = v95[5];
          v96 = v56->f278736[0];
          v97 = (P2Ctx *)v56->f278736[1];
          ((P2Ctx *)v96)[-7] = v97[6];
          v98 = v56->f278736[0];
          v99 = (P2Ctx *)v56->f278736[1];
          ((P2Ctx *)v98)[-8] = v99[7];
          *(uint16_t *)(v56->f278736[0] + 2) = 0;
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
          v102 = &lpAddress_1->f940072[4 * *&lpAddress_1->f278704];
          v168 = v101;
          v103 = __alt_p2_decode_symbol((uint16_t *)v102, (uint8_t *)((uint32_t *)lpAddress_1 + 69677));
          v104 = (uint8_t)(v168 + (*(uint8_t *)&lpAddress_1->f280496[v103]));
          __alt_p2_model((AltP2Block *)lpAddress_1, v104, v103, v104 - v168);
          v105 = *(uint32_t *)&lpAddress_1->f278736[0];
          v106 = *(int16_t *)(v105 - 10);
          v107 = 32 * *(int16_t *)(v105 - 4);
          ctx_bias[0] += 32 * *(int16_t *)(v105 - 6);
          v108 = *(int16_t *)(v105 - 8);
          ctx_bias[1] += v107;
          ctx_bias[2] += 32 * v106;
          ctx_bias[3] += 32 * v108;
          v109 = v163 == 0;
          Src[plane_desc[1].src_plane] = v104;
          if ( v109 )
            v110 = 0;
          else
            v110 = 16 * Src[plane_desc[1].src_plane];
          v111 = (AltP2Block *)(plane[1]);
          *(uint16_t *)(plane[1]->f278736[0] + 2) = v110;
          v112 = __alt_p2_context((AltP2Block *)v111, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          v113 = __alt_p2_decode_symbol(&v111->f940072[4 * *(int32_t *)&v111->f278704], (uint8_t *)&v111->f278708);
          v114 = (uint8_t)(v112 + v111->f280496[v113]);
          v169 = v114;
          __alt_p2_model((AltP2Block *)v111, v114, v113, v114 - v112);
          v115 = *(int32_t *)&v111->f278736[0];
          v116 = *(int16_t *)(v115 - 4);
          v117 = *(int16_t *)(v115 - 10);
          ctx_bias[0] += 32 * *(int16_t *)(v115 - 6);
          v118 = *(int16_t *)(v115 - 8);
          ctx_bias[1] += 32 * v116;
          ctx_bias[2] += 32 * v117;
          ctx_bias[3] += 32 * v118;
          v109 = ArgList == 0;
          Src[plane_desc[2].src_plane] = v169;
          if ( v109 )
            LOWORD(v119) = 0;
          else
            v119 = (plane_desc[plane_desc[3].src_plane + 1].w8 * Src[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * Src[plane_desc[1].src_plane]) >> 3;
          v120 = (AltP2Block *)(plane[2]);
          *(uint16_t *)(plane[2]->f278736[0] + 2) = v119;
          v121 = __alt_p2_context((AltP2Block *)v120, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          v122 = __alt_p2_decode_symbol(&v120->f940072[4 * *(int32_t *)&v120->f278704], (uint8_t *)&v120->f278708);
          v123 = (uint8_t)(v121 + v120->f280496[v122]);
          v170 = v123;
          __alt_p2_model((AltP2Block *)v120, v123, v122, v123 - v121);
          v124 = *(int32_t *)&v120->f278736[0];
          v125 = *(int16_t *)(v124 - 4);
          v126 = *(int16_t *)(v124 - 10);
          ctx_bias[0] += 32 * *(int16_t *)(v124 - 6);
          v127 = *(int16_t *)(v124 - 8);
          ctx_bias[1] += 32 * v125;
          ctx_bias[2] += 32 * v126;
          ctx_bias[3] += 32 * v127;
          Src[plane_desc[3].src_plane] = v170;
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
            *(uint16_t *)(plane[3]->f278736[0] + 2) = v128;
            v130 = __alt_p2_context((AltP2Block *)v129, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            v131 = __alt_p2_decode_symbol(&v129->f940072[4 * *(int32_t *)&v129->f278704], (uint8_t *)&v129->f278708);
            v154 = (uint8_t)(v130 + v129->f280496[v131]);
            __alt_p2_model((AltP2Block *)v129, v154, v131, v154 - v130);
            v132 = *(int32_t *)&v129->f278736[0];
            v133 = *(int16_t *)(v132 - 4);
            v134 = *(int16_t *)(v132 - 10);
            v135 = *(int16_t *)(v132 - 8);
            ctx_bias[0] += 32 * *(int16_t *)(v132 - 6);
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
        __alt_model_p1_d8_decode(ArgList, (uint8_t *)Src, *p_i, p_i[1]);
      else
        __alt_model_p1_decode((uint16_t *)p_i, (uint8_t *)Src);
    }
    else if ( plane_predictor == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __alt_model_p2_d8_decode((uint8_t *)Src, *p_i, p_i[1]);
      else
        __alt_model_p2_decode((uint16_t *)p_i, (uint8_t *)Src);
    }
  }
  else
  {
    v5 = bmf_new(0x7BA230u);
    if ( v5 )
      v6 = __layout_workspace((ModelBlock *)v5, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F);
    else
      v6 = (ModelBlock *)(nullptr);
    __unmodel_plane_slow((ModelBlock *)v6, (uint8_t *)Src);
    if ( v6 )
      __free_workspace((ModelBlock *)v6, 1);
  }
}

void __alt_p2_d8_encode_body(AltP2Block *lpAddress, uint8_t *a4, int32_t i, int32_t a6, uint8_t *a7)
{
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
  int8_t *v106;
  uint8_t v107;
  AltP2Block *lpAddress_1;
  uint8_t *v109;
  uint8_t v110;
  ;
  P2Ctx *v34;   // a record cursor
  P2Ctx *v57;   // a record cursor
  P2Ctx *v65;   // a record cursor
  P2Ctx *v72;   // a record cursor
  uint8_t *v11;
  P2Ctx *v27;   // a record cursor
  uint8_t *v23, *v30, *v51, *v61, *v69, *v79;
  P2Ctx *v93;   // a record cursor
  P2Ctx *v91;   // a record cursor
  P2Ctx *v89;   // a record cursor
  P2Ctx *v87;   // a record cursor
  P2Ctx *v85;   // a record cursor
  P2Ctx *v83;   // a record cursor
  P2Ctx *v81;   // a record cursor
  uint8_t v14, v97;
  int16_t v26;
  int32_t v12, v13, n16, v16, v17, v24, v25, v31, v32, v33, v39, Size, v52,
          *v53, v54, v55, v62, v63, v64, v70, v71, v75, v76, v77, v78, v80,
          v82, v84, v86, v88, v90, v92, v95, v96, n16_1, v99, v100;
  int64_t v19;
  uint16_t *v18;
  uint32_t k;
  uint8_t *v8;
  v8 = a4;
  __rc_begin_encode();
  v11 = (uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
  if ( i > 0 )
  {
    v103 = a4;
    for ( j = 0; j < i; ++j )
    {
      v12 = *(uint16_t *)((uint8_t *)v11 - 18) >> 4;
      v13 = (uint8_t)(*v103 - v12);
      v104 = lpAddress->f279984[v13];
      v14 = lpAddress->f280496[v104] + v12;
      n16 = (uint8_t)*a7 - (uint8_t)(v14 + *a7 - *v103);
      if ( n16 < -16 || n16 > 16 )
      {
        *a7 = *v103;
        v104 = lpAddress->f280240[v13];
      }
      else
      {
        *a7 = v14;
      }
      v16 = *(int32_t *)&lpAddress->f278736[0];
      v17 = *(int16_t *)(v16 - 18) >> 4;
      *(int32_t *)&lpAddress->f278708 = *(int32_t *)&lpAddress->f278704 + lpAddress->f278944[v17 + 4];
      *(int32_t *)&lpAddress->f278712 = *(int32_t *)&lpAddress->f278704 + lpAddress->f278944[v17];
      __alt_p2_encode_symbol(&lpAddress->f940072[4 * *(int32_t *)&lpAddress->f278704
                                 + 4
                                 * *(int32_t *)&lpAddress->f278760[(*(int16_t *)(v16 - 18) <= *(int16_t *)(v16 - 36))
                                                           + (*(int16_t *)(v16 - 18) < *(int16_t *)(v16 - 36)) + 17]
                                 + 4 * *(int32_t *)&lpAddress->f278760[*(uint8_t *)(v16 - 20) + 13]
                                 + 4 * *(int32_t *)&lpAddress->f278760[*(uint8_t *)(v16 - 2) + 9]
                                 + 4
                                 * *(int32_t *)&lpAddress->f278760[((uint32_t)(v17 - 115) >> 31)
                                                           + ((uint32_t)(v17 - 17) >> 31) + 5]
                                 + 4 * *(int32_t *)&lpAddress->f278760[22]], (uint8_t *)&lpAddress->f278708, v104);
      ++v103;
      LOWORD(v16) = 16 * (uint8_t)*a7;
      *(uint16_t *)*&lpAddress->f278736[0] = v16;
      *(uint16_t *)(lpAddress->f278736[0] + 2) = v16;
      ++a7;
      v18 = (uint16_t *)lpAddress->f278736[0];
      v19 = (int16_t)(*v18 - *(v18 - 9));
      v18[2] = v19;
      LOWORD(v19) = (WORD2(v19) ^ v19) - WORD2(v19);
      *(uint16_t *)(lpAddress->f278736[0] + 6) = v19;
      *(uint16_t *)(lpAddress->f278736[0] + 14) = v19;
      *(uint16_t *)(lpAddress->f278736[0] + 12) = v19;
      *(uint16_t *)(lpAddress->f278736[0] + 10) = v19;
      *(uint16_t *)(lpAddress->f278736[0] + 8) = (uint32_t)*(int16_t *)(lpAddress->f278736[0] + 10) >> 1;
      *(lpAddress->f278736[0] + 17) = 2;
      *(lpAddress->f278736[0] + 16) = (*(int16_t *)(lpAddress->f278736[0] + 4) <= 0)
                                                    + (*(int16_t *)(lpAddress->f278736[0] + 4) < 0);
      v11 = (lpAddress->f278736[0] + 18);
      lpAddress->f278736[0] = v11;
    }
    v8 = v103;
  }
  ((P2Ctx *)v11)[0] = ((P2Ctx *)v11)[-1];
  v23 = (uint8_t *)lpAddress->f278736[0];
  v24 = *(uint32_t *)(v23 - 10);
  v25 = *(uint32_t *)(v23 - 6);
  *(uint32_t *)(v23 + 18) = *(uint32_t *)(v23 - 18);
  *(uint32_t *)(v23 + 22) = *(uint32_t *)(v23 - 14);
  v26 = *(uint16_t *)(v23 - 2);
  *(uint32_t *)(v23 + 26) = v24;
  *(uint32_t *)(v23 + 30) = v25;
  *(uint16_t *)(v23 + 34) = v26;
  v27 = (P2Ctx *)(uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
  v27[2] = v27[-1];
  v30 = (uint8_t *)lpAddress->f278736[0];
  v31 = *(uint32_t *)(v30 - 14);
  v32 = *(uint32_t *)(v30 - 10);
  *(uint32_t *)(v30 + 54) = *(uint32_t *)(v30 - 18);
  v33 = *(uint32_t *)(v30 - 6);
  *(uint32_t *)(v30 + 58) = v31;
  LOWORD(v31) = *(uint16_t *)(v30 - 2);
  *(uint32_t *)(v30 + 62) = v32;
  *(uint32_t *)(v30 + 66) = v33;
  *(uint16_t *)(v30 + 70) = v31;
  v34 = (P2Ctx *)(uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
  v34[4] = v34[-1];
  v39 = -18 * i;
  // One cursor for the 8 records this shifts; MSVC reloaded the base
  // between every pair and nothing here writes it.
  P2Ctx *const rec3 = (P2Ctx *)((uint8_t *)(lpAddress->f278736[0]) + v39);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  Size = 18 * i + 234;
  memcpy((uint8_t *)lpAddress->f278736[6],(uint8_t *)lpAddress->f278736[5],Size);
  memcpy((uint8_t *)lpAddress->f278736[7],(uint8_t *)lpAddress->f278736[5],Size);
  memcpy((uint8_t *)lpAddress->f278736[8],(uint8_t *)lpAddress->f278736[5],Size);
  if ( a6 > 1 )
  {
    v106 = &*(int8_t *)&lpAddress->f278708;
    v105 = a7;
    v102 = 0;
    do
    {
      v51 = (uint8_t *)lpAddress->f278668;
      v52 = *(uint32_t *)(v51 - 4);
      v109 = v8;
      *(uint32_t *)(v51 + 4) = v52;
      *(uint32_t *)*(int32_t *)&lpAddress->f278668 = v52;
      v53 = (int32_t *)*(int32_t *)&lpAddress->f278660;
      v54 = *(int32_t *)&lpAddress->f278664;
      *(int32_t *)&lpAddress->f278660 = v54;
      *(int32_t *)&lpAddress->f278664 = (int32_t)v53;
      v54 += 8;
      *(int32_t *)&lpAddress->f278668 = v54;
      v53 += 2;
      *(int32_t *)&lpAddress->f278672 = (int32_t)v53;
      v55 = *v53;
      *(uint32_t *)(v54 - 4) = *v53;
      *(uint32_t *)(*(int32_t *)&lpAddress->f278668 - 8) = v55;
      lpAddress->f278528_q = 0;
      lpAddress->f278536 = 0;
      lpAddress->f278540 = 0;
      lpAddress->f278542 = 0;
      lpAddress->f278640 = 0;
      lpAddress->f278648 = 0;
      // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
      // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
      // Seven sixteen-byte stores are the 112 bytes of the seven rows.
      __builtin_memset(lpAddress->p2_row, 0, sizeof lpAddress->p2_row);
      v57 = (P2Ctx *)(uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
      v57[0] = v57[-1];
      v61 = (uint8_t *)lpAddress->f278736[0];
      v62 = *(uint32_t *)(v61 - 32);
      v63 = *(uint32_t *)(v61 - 28);
      *(uint32_t *)(v61 + 18) = *(uint32_t *)(v61 - 36);
      v64 = *(uint32_t *)(v61 - 24);
      *(uint32_t *)(v61 + 22) = v62;
      *(uint32_t *)(v61 + 26) = v63;
      *(uint32_t *)(v61 + 30) = v64;
      *(uint16_t *)(v61 + 34) = *(uint16_t *)(v61 - 20);
      v65 = (P2Ctx *)(uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
      v65[2] = v65[-3];
      v69 = (uint8_t *)lpAddress->f278736[0];
      v70 = *(uint32_t *)(v69 - 64);
      v71 = *(uint32_t *)(v69 - 60);
      *(uint32_t *)(v69 + 54) = *(uint32_t *)(v69 - 72);
      *(uint32_t *)(v69 + 58) = *(uint32_t *)(v69 - 68);
      *(uint32_t *)(v69 + 62) = v70;
      *(uint32_t *)(v69 + 66) = v71;
      *(uint16_t *)(v69 + 70) = *(uint16_t *)(v69 - 56);
      v72 = (P2Ctx *)(uint8_t *)(*(int32_t *)&lpAddress->f278736[0]);
      v72[4] = v72[-5];
      v75 = *(int32_t *)&lpAddress->f278736[8];
      v76 = *(int32_t *)&lpAddress->f278736[9];
      v77 = *(int32_t *)&lpAddress->f278736[7];
      v78 = *(int32_t *)&lpAddress->f278736[6];
      v79 = (uint8_t *)lpAddress->f278736[5];
      *(int32_t *)&lpAddress->f278736[9] = v75;
      *(int32_t *)&lpAddress->f278736[8] = v77;
      *(int32_t *)&lpAddress->f278736[7] = v78;
      *(int32_t *)&lpAddress->f278736[5] = v76;
      lpAddress->f278736[6] = v79;
      v76 += 144;
      *(int32_t *)&lpAddress->f278736[0] = v76;
      v79 += 144;
      lpAddress->f278736[1] = v79;
      *(int32_t *)&lpAddress->f278736[2] = v78 + 144;
      *(int32_t *)&lpAddress->f278736[3] = v77 + 144;
      *(int32_t *)&lpAddress->f278736[4] = v75 + 144;
      ((P2Ctx *)v76)[-1] = ((P2Ctx *)v79)[0];
      v80 = *(int32_t *)&lpAddress->f278736[0];
      v81 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v80)[-2] = v81[1];
      v82 = *(int32_t *)&lpAddress->f278736[0];
      v83 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v82)[-3] = v83[2];
      v84 = *(int32_t *)&lpAddress->f278736[0];
      v85 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v84)[-4] = v85[3];
      v86 = *(int32_t *)&lpAddress->f278736[0];
      v87 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v86)[-5] = v87[4];
      v88 = *(int32_t *)&lpAddress->f278736[0];
      v89 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v88)[-6] = v89[5];
      v90 = *(int32_t *)&lpAddress->f278736[0];
      v91 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v90)[-7] = v91[6];
      v92 = *(int32_t *)&lpAddress->f278736[0];
      v93 = (P2Ctx *)(uint8_t *)lpAddress->f278736[1];
      ((P2Ctx *)v92)[-8] = v93[7];
      *(uint16_t *)(lpAddress->f278736[0] + 2) = 0;
      if ( i > 0 )
      {
        lpAddress_1 = (AltP2Block *)(lpAddress);
        for ( k = 0; k < i; ++k )
        {
          v95 = __alt_p2_context((AltP2Block *)lpAddress_1, (AltP2Block *)nullptr, (AltP2Block *)nullptr);
          v107 = v109[k];
          v96 = (uint8_t)(v107 - v95);
          v97 = v95 + lpAddress_1->f280496[lpAddress_1->f279984[v96]];
          n16_1 = (uint8_t)v105[k] - (uint8_t)(v97 + v105[k] - v107);
          v99 = lpAddress_1->f279984[v96];
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v105[k] = v107;
            v99 = lpAddress_1->f280240[(uint8_t)(v107 - v95)];
          }
          else
          {
            v105[k] = v97;
          }
          v110 = v99;
          __alt_p2_encode_symbol(&lpAddress_1->f940072[4 * *(int32_t *)&lpAddress_1->f278704], (uint8_t *)v106, v99);
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
  __alt_p2_d8_encode_body((AltP2Block *)lpAddress, (uint8_t *)a3, i, a5, (uint8_t *)a6);
  if ( lpAddress )
    __alt_p2_free((void **)lpAddress, 1);
}

int32_t __alt_model_p2_encode(BmfImage *p_i, uint8_t *a2)
{
  uint32_t Size_1;
  int32_t v153;
  uint32_t v154;
  int32_t v155;
  int32_t n3;
  int32_t v157;
  int32_t v158;
  AltP2Block * plane[4];
  int32_t v163;
  int32_t v164;
  uint32_t Size;
  uint8_t *v166;
  int32_t i_2;
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
  uint8_t *v33;
  P2Ctx *v40;   // a record cursor
  P2Ctx *v62;   // a record cursor
  P2Ctx *v70;   // a record cursor
  P2Ctx *v77;   // a record cursor
  uint8_t *v29, *v37, *v56, *v66, *v74, *v84;
  P2Ctx *v98;   // a record cursor
  P2Ctx *v96;   // a record cursor
  P2Ctx *v94;   // a record cursor
  P2Ctx *v92;   // a record cursor
  P2Ctx *v90;   // a record cursor
  P2Ctx *v88;   // a record cursor
  P2Ctx *v86;   // a record cursor
  int32_t v45;   // an offset from v44, not a cursor
  AltP2Block *v125;
  AltP2Block *v133;
  AltP2Block *v114;
  bool v16;
  uint8_t v9;
  int16_t v113;
  uint8_t *v20, *v21, *v22, *v23, *v59, *v80, *v81, *v82, *v83, *v85, *v87, *v89,
          *v91, *v93, *v95, *v97;   // row cursors
  int32_t i_1, v5, n4, n4_1, v14, v15, v30, v31, v32, v38, v99,
          v39, v57, *v58, v60, v67,
          v68, v69, v75, v76, v100, v101, v102, v104, v105, v106, v107, n16, v109,
          v110, v111, v112, v115, v116, v117, v118, n16_1, v120, v121, v122,
          v123, v124, v126, v127, v128, v129, n16_2, v131, v132, v134, v135,
          v136, v137, n16_3, v139, n4_3, n4_4;
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
  v5 = p_i->height;
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
  v9 = plane_desc[plane_desc[4].src_plane + 1].flags;
  v168 = plane_desc[plane_desc[3].src_plane + 1].flags & 8;
  v155 = v9 & 8;
  __rc_begin_encode();
  if ( v5 > 0 )
  {
    v154 = v5;
    v11 = 0;
    i_2 = i_1;
    v153 = 9 * i_1;
    v164 = -18 * i_1;
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
              v25 = (P2Ctx *)(uint8_t *)(v24->f278736[0]);
              v25[0] = v25[-1];
              v29 = v24->f278736[0];
              v30 = *(uint32_t *)(v29 - 14);
              v31 = *(uint32_t *)(v29 - 10);
              *(uint32_t *)(v29 + 18) = *(uint32_t *)(v29 - 18);
              v32 = *(uint32_t *)(v29 - 6);
              *(uint32_t *)(v29 + 22) = v30;
              LOWORD(v30) = *(uint16_t *)(v29 - 2);
              *(uint32_t *)(v29 + 26) = v31;
              *(uint32_t *)(v29 + 30) = v32;
              *(uint16_t *)(v29 + 34) = v30;
              v33 = (uint8_t *)(v24->f278736[0]);
              ((P2Ctx *)v33)[2] = ((P2Ctx *)v33)[-1];
              v37 = v24->f278736[0];
              v38 = *(uint32_t *)(v37 - 10);
              v39 = *(uint32_t *)(v37 - 6);
              *(uint32_t *)(v37 + 54) = *(uint32_t *)(v37 - 18);
              *(uint32_t *)(v37 + 58) = *(uint32_t *)(v37 - 14);
              LOWORD(v33) = *(uint16_t *)(v37 - 2);
              *(uint32_t *)(v37 + 62) = v38;
              *(uint32_t *)(v37 + 66) = v39;
              *(uint16_t *)(v37 + 70) = (uint16_t)(uintptr_t)v33;
              v40 = (P2Ctx *)(uint8_t *)(v24->f278736[0]);
              v40[4] = v40[-1];
              v45 = v164;
              Size_1 = Size;
              // One cursor for the 8 records this shifts; MSVC reloaded the base
              // between every pair and nothing here writes it.
              P2Ctx *const rec4 = (P2Ctx *)((uint8_t *)(v24->f278736[0]) + v45);
              rec4[-1] = rec4[0];
              rec4[-2] = rec4[1];
              rec4[-3] = rec4[2];
              rec4[-4] = rec4[3];
              rec4[-5] = rec4[4];
              rec4[-6] = rec4[5];
              rec4[-7] = rec4[6];
              rec4[-8] = rec4[7];
              memcpy(v24->f278760_p,v24->f278756,Size_1);
              memcpy(v24->f278764,v24->f278756,Size);
              memcpy(v24->f278768,v24->f278756,Size);
            }
          }
          else
          {
            v13 = (AltP2Block *)((int32_t)plane[n4_2 - 1]);
            v163 = (int32_t)(&plane[n4_2]);
            v14 = 0;
            do
            {
              v15 = 9 * v14;
              *(uint16_t *)(v13->f278736[5] + 2 * v15) = 256;
              v16 = ++v14 < v185;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 2) = 256;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 4) = -16;
              *(v13->f278736[5] + 2 * v15 + 16) = 1;
              *(v13->f278736[5] + 2 * v15 + 17) = 3;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 6) = 512;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 14) = 512;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 12) = 512;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 10) = 1024;
              *(uint16_t *)(v13->f278736[5] + 2 * v15 + 8) = 256;
            }
            while ( v16 );
            v17 = (AltP2Block **)v163;
            memcpy(v13->f278760_p,*&v13->f278736[5],Size);
            memcpy(v13->f278764,*&v13->f278736[5],Size);
            memcpy(v13->f278768,*&v13->f278736[5],Size);
            v20 = (uint8_t *)v13->f278760_p + 2 * v153 + 144;
            v13->f278736[0] = v13->f278736[5] + 2 * v153 + 144;
            v21 = (uint8_t *)v13->f278764;
            v13->f278736[1] = v20;
            v22 = (uint8_t *)v13->f278768;
            v13->f278736[2] = v21 + 2 * v153 + 144;
            v23 = (uint8_t *)v13->f278772 + 2 * v153 + 144;
            v13->f278736[3] = v22 + 2 * v153 + 144;
            v13->f278736[4] = v23;
          }
          v55 = (AltP2Block *)((int32_t)*(v17 - 1));
          v56 = v55->f278668;
          v57 = *(uint32_t *)(v56 - 4);
          *(uint32_t *)(v56 + 4) = v57;
          *(uint32_t *)v55->f278668 = v57;
          v58 = v55->f278660;
          v59 = v55->f278664;
          v55->f278660 = (int32_t *)v59;
          v55->f278664 = (uint8_t *)v58;
          v59 += 8;
          v55->f278668 = v59;
          v58 += 2;
              v55->f278672 = (uint8_t *)v58;
          v60 = *v58;
          *(uint32_t *)(v59 - 4) = *v58;
          *(uint32_t *)(v55->f278668 - 8) = v60;
          v55->f278528_q = 0;
          v55->f278536 = 0;
          v55->f278540 = 0;
          v55->f278542 = 0;
          v55->f278640 = 0;
          v55->f278648 = 0;
          // `(p + 278543) & ~15` is `&p->p2_row[0]`: +278528 is a multiple of 16
          // and the object comes from `bmf_page_alloc`, so the round-up is a no-op.
          // Seven sixteen-byte stores are the 112 bytes of the seven rows.
          __builtin_memset(v55->p2_row, 0, sizeof v55->p2_row);
          v62 = (P2Ctx *)(uint8_t *)(v55->f278736[0]);
          v62[0] = v62[-1];
          v66 = v55->f278736[0];
          v67 = *(uint32_t *)(v66 - 32);
          v68 = *(uint32_t *)(v66 - 28);
          *(uint32_t *)(v66 + 18) = *(uint32_t *)(v66 - 36);
          v69 = *(uint32_t *)(v66 - 24);
          *(uint32_t *)(v66 + 22) = v67;
          *(uint32_t *)(v66 + 26) = v68;
          *(uint32_t *)(v66 + 30) = v69;
          *(uint16_t *)(v66 + 34) = *(uint16_t *)(v66 - 20);
          v70 = (P2Ctx *)(uint8_t *)(v55->f278736[0]);
          v70[2] = v70[-3];
          v74 = v55->f278736[0];
          v75 = *(uint32_t *)(v74 - 64);
          v76 = *(uint32_t *)(v74 - 60);
          *(uint32_t *)(v74 + 54) = *(uint32_t *)(v74 - 72);
          *(uint32_t *)(v74 + 58) = *(uint32_t *)(v74 - 68);
          *(uint32_t *)(v74 + 62) = v75;
          *(uint32_t *)(v74 + 66) = v76;
          *(uint16_t *)(v74 + 70) = *(uint16_t *)(v74 - 56);
          v77 = (P2Ctx *)(uint8_t *)(v55->f278736[0]);
          v77[4] = v77[-5];
          v80 = v55->f278736[8];
          v81 = v55->f278736[9];
          v82 = v55->f278736[7];
          v83 = v55->f278736[6];
          v84 = v55->f278736[5];
          v55->f278736[9] = v80;
          v55->f278736[8] = v82;
          v55->f278736[7] = v83;
          v55->f278736[5] = v81;
          v55->f278736[6] = v84;
          v81 += 144;
          v55->f278736[0] = v81;
          v84 += 144;
          v55->f278736[1] = v84;
          v55->f278736[2] = v83 + 144;
          v55->f278736[3] = v82 + 144;
          v55->f278736[4] = v80 + 144;
          ((P2Ctx *)v81)[-1] = ((P2Ctx *)v84)[0];
          v85 = v55->f278736[0];
          v86 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v85)[-2] = v86[1];
          v87 = v55->f278736[0];
          v88 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v87)[-3] = v88[2];
          v89 = v55->f278736[0];
          v90 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v89)[-4] = v90[3];
          v91 = v55->f278736[0];
          v92 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v91)[-5] = v92[4];
          v93 = v55->f278736[0];
          v94 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v93)[-6] = v94[5];
          v95 = v55->f278736[0];
          v96 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v95)[-7] = v96[6];
          v97 = v55->f278736[0];
          v98 = (P2Ctx *)v55->f278736[1];
          ((P2Ctx *)v97)[-8] = v98[7];
          *(uint16_t *)(v55->f278736[0] + 2) = 0;
          n4_1 = plane_count;
        }
        while ( plane_count > n4_2 );
        v11 = v170;
      }
      if ( i_2 <= 0 )
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
        for ( i = 0; i < i_2; ++i )
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
          v105 = (uint8_t)lpAddress_1->f279984[v181];
          v106 = v166[v184];
          v173 = (uint8_t)(v178 + (*(uint8_t *)&lpAddress_1->f280496[v105]));
          v107 = (uint8_t)(v173 + v106 - v104);
          n16 = v106 - v107;
          if ( n16 < -16 || n16 > 16 )
          {
            v105 = (uint8_t)lpAddress_1->f280240[v181];
          }
          else
          {
            v104 = v173;
            v166[v184] = v107;
          }
          __alt_p2_encode_symbol((uint16_t *)&((uint8_t *)lpAddress_1)[8 * lpAddress_1->f278704 + 940072], (uint8_t *)lpAddress_1 + 278708, v105);
          __alt_p2_model((AltP2Block *)lpAddress_1, v104, v105, v104 - v178);
          v109 = *(uint32_t *)&lpAddress_1->f278736[0];
          v110 = *(int16_t *)(v109 - 4);
          v111 = *(int16_t *)(v109 - 10);
          v112 = *(int16_t *)(v109 - 8);
          ctx_bias[0] += 32 * *(int16_t *)(v109 - 6);
          ctx_bias[1] += 32 * v110;
          ctx_bias[2] += 32 * v111;
          ctx_bias[3] += 32 * v112;
          if ( v169 )
            v113 = 16 * v166[plane_desc[1].src_plane];
          else
            v113 = 0;
          v114 = (AltP2Block *)(plane[1]);
          *(uint16_t *)(plane[1]->f278736[0] + 2) = v113;
          v176 = plane_desc[2].src_plane;
          v115 = v166[plane_desc[2].src_plane];
          v179 = __alt_p2_context((AltP2Block *)v114, (AltP2Block *)plane[0], (AltP2Block *)plane[2]);
          v183 = (uint8_t)(v115 - v179);
          v116 = v114->f279984[v183];
          v117 = v166[v176];
          v175 = (uint8_t)(v179 + v114->f280496[v116]);
          v118 = (uint8_t)(v175 + v117 - v115);
          n16_1 = v117 - v118;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v116 = v114->f280240[v183];
          }
          else
          {
            v115 = v175;
            v166[v176] = v118;
          }
          __alt_p2_encode_symbol(&v114->f940072[4 * *(int32_t *)&v114->f278704], (uint8_t *)&v114->f278708, v116);
          __alt_p2_model((AltP2Block *)v114, v115, v116, v115 - v179);
          v120 = *(int32_t *)&v114->f278736[0];
          v121 = *(int16_t *)(v120 - 4);
          v122 = *(int16_t *)(v120 - 10);
          v123 = *(int16_t *)(v120 - 8);
          ctx_bias[0] += 32 * *(int16_t *)(v120 - 6);
          ctx_bias[1] += 32 * v121;
          ctx_bias[2] += 32 * v122;
          ctx_bias[3] += 32 * v123;
          if ( v168 )
            v124 = (plane_desc[plane_desc[3].src_plane + 1].w8 * v166[plane_desc[2].src_plane]
                  + plane_desc[plane_desc[3].src_plane + 1].w4 * v166[plane_desc[1].src_plane]) >> 3;
          else
            LOWORD(v124) = 0;
          v125 = (AltP2Block *)(plane[2]);
          *(uint16_t *)(plane[2]->f278736[0] + 2) = v124;
          v177 = plane_desc[3].src_plane;
          v126 = v166[plane_desc[3].src_plane];
          v180 = __alt_p2_context((AltP2Block *)v125, (AltP2Block *)plane[0], (AltP2Block *)plane[1]);
          v182 = (uint8_t)(v126 - v180);
          v127 = v125->f279984[v182];
          v128 = v166[v177];
          v174 = (uint8_t)(v180 + v125->f280496[v127]);
          v129 = (uint8_t)(v174 + v128 - v126);
          n16_2 = v128 - v129;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            v127 = v125->f280240[v182];
          }
          else
          {
            v126 = v174;
            v166[v177] = v129;
          }
          __alt_p2_encode_symbol(&v125->f940072[4 * *(int32_t *)&v125->f278704], (uint8_t *)&v125->f278708, v127);
          __alt_p2_model((AltP2Block *)v125, v126, v127, v126 - v180);
          v131 = *(int32_t *)&v125->f278736[0];
          n4_1 = plane_count;
          v102 = ctx_bias[0] + 32 * *(int16_t *)(v131 - 6);
          v101 = ctx_bias[1] + 32 * *(int16_t *)(v131 - 4);
          v100 = ctx_bias[2] + 32 * *(int16_t *)(v131 - 10);
          v99 = ctx_bias[3] + 32 * *(int16_t *)(v131 - 8);
          if ( plane_count >= 4 )
          {
            ctx_bias[0] += 32 * *(int16_t *)(v131 - 6);
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
            *(uint16_t *)(plane[3]->f278736[0] + 2) = v132;
            n3 = plane_desc[4].src_plane;
            v134 = v166[plane_desc[4].src_plane];
            v157 = __alt_p2_context((AltP2Block *)v133, (AltP2Block *)plane[2], (AltP2Block *)plane[0]);
            v158 = (uint8_t)(v134 - v157);
            v135 = v133->f279984[v158];
            v136 = v166[n3];
            v137 = (uint8_t)(v157 + v133->f280496[v135] + v136 - v134);
            n16_3 = v136 - v137;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              v135 = v133->f280240[v158];
            }
            else
            {
              v134 = (uint8_t)(v157 + v133->f280496[v135]);
              v166[n3] = v137;
            }
            __alt_p2_encode_symbol(&v133->f940072[4 * *(int32_t *)&v133->f278704], (uint8_t *)&v133->f278708, v135);
            __alt_p2_model((AltP2Block *)v133, v134, v135, v134 - v157);
            v139 = *(int32_t *)&v133->f278736[0];
            n4_1 = plane_count;
            v102 = ctx_bias[0] + 32 * *(int16_t *)(v139 - 6);
            v101 = ctx_bias[1] + 32 * *(int16_t *)(v139 - 4);
            v100 = ctx_bias[2] + 32 * *(int16_t *)(v139 - 10);
            v99 = ctx_bias[3] + 32 * *(int16_t *)(v139 - 8);
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
    while ( v11 < v154 );
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
  uint8_t *v59;   // a row cursor into f1078236
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
  uint8_t *v46, *v50;   // row cursors out of f56
  bool v43;
  int8_t v7;
  uint8_t *buf;   // `uint8_t *` beside the `char` scalars above
  int16_t __model_plane_n2, v22;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_4;
  int32_t v8, v10, v11, v14, n2_1, n2_2, v17, v18, v19, v20, v21, v44, v45,
          v53, v54, v55, v56;
  uint8_t *v28, *v29, *v30, *v47, *v48, *v49;   // row cursors out of f56
  uint32_t n0x10000, *v24, v31, *v32, v34, v37, *v38, v40;
  SymList *v33, *v39;
  uint16_t *v12;   // was uint32_t *, read only as uint16_t
  uint8_t *v51, *v52;
  void *v5;
  if ( plane_alt_model )
  {
    if ( ::plane_predictor == 1 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p1_d8_encode((uint8_t *)a4, p_i->width, p_i->height, (uint8_t *)a5);
      else
        __alt_model_p1_encode((uint16_t *)p_i, (uint8_t *)a4);
    }
    else if ( ::plane_predictor == 2 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __alt_model_p2_d8_encode((uint8_t *)a4, p_i->width, p_i->height, (uint8_t *)a5);
      else
        __alt_model_p2_encode((BmfImage *)p_i, (uint8_t *)a4);
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
    __reduce_alphabet(Blocka_3, v7, (uint8_t *)a4);
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
          v73 = Blocka_2->f16;
          v12 = (uint16_t *)&((uint32_t *)Blocka_2)[4 * v64];
          __model_plane_n2 = 2;
          v12[48] = 2;
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
            v12[52] = 0;
          }
          else
          {
            v12[52] = n2_2;
          }
          if ( v65 )
          {
            __model_plane_n2 += n2_1;
            n2_1 = 0;
            v12[51] = 0;
          }
          else
          {
            v12[51] = n2_1;
          }
          if ( v70 )
          {
            v12[49] = v14 + __model_plane_n2;
            LOWORD(v14) = 0;
            v12[50] = 0;
          }
          else
          {
            v12[50] = v14;
            v12[49] = __model_plane_n2;
          }
          v17 = n2_1 != 0;
          v18 = n2_2 != 0;
          v14 = (uint16_t)v14;
          if ( (uint16_t)v14 )
            v14 = 1;
          v19 = v18 + v17 + v14 + 2;
          if ( v19 <= v73 )
          {
            *((uint8_t *)v12 + 110) = v19;
          }
          else
          {
            *((uint8_t *)v12 + 110) = v18 + v17 + v14 + 1;
            v12[48] = 0;
          }
          if ( v12[v72 + 48]
            && v12[n5 + 48]
            && (v20 = *((uint8_t *)v12 + 110), v20 <= v73) )
          {
            v21 = 1;
            v22 = (uint8_t)(1 << ((5 - v20) & 31));
            *((uint8_t *)v12 + 111) = v22;
            v12[54] = v22 << 6;
            v12[v72 + 48] += v22;
            v12[n5 + 48] += *((uint8_t *)v12 + 111);
            v12[53] = v12[48]
                                 + v12[52]
                                 + v12[51]
                                 + v12[50]
                                 + v12[49];
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
      v24 = &((uint32_t *)Blocka_2)[0x10000 * v58];
      do
      {
        LOWORD(v24[n0x10000 + 531818]) = 0x2000;
        HIWORD(v24[n0x10000++ + 531818]) = 0x2000;
      }
      while ( n0x10000 < 0x10000 );
      Blocka_4 = (ModelBlock *)((uintptr_t)Blocka_5 + 1);
      v8 = v58 + 1;
    }
    while ( (uint32_t)(v58 + 1) < 0xF );
    Blocka_1 = (ModelBlock *)((int32_t)Blocka_2);
    buf = (uint8_t *)bmf_new(Blocka_2->f16);
    Size = Blocka_2->f16;
    Blocka_2->f1078688 = (uint8_t *)buf;
    memset(buf,1,Size);
    v28 = Blocka_2->f56[11];
    v29 = Blocka_2->f56[12];
    Blocka_2->f1051664[0] = Blocka_2->f56[10];
    v30 = Blocka_2->f56[13];
    Blocka_2->f1051664[1] = v28;
    Blocka_2->f1051664[2] = v29;
    Blocka_2->f1051664[3] = v30;
    Blocka_2->f1078224 = &Blocka_2->escape;
    __init_symbol_list(&Blocka_1->escape, 0, Blocka_1->f16, 1);
    Blocka_2->f1078232 = Blocka_2->sel;
    v31 = Blocka_2->f16;
    v32 = (uint32_t *)bmf_new(24 * v31 + 4);
    if ( v32 )
    {
      *v32 = v31;
      v33 = (SymList *)(v32 + 1);
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
    v37 = Blocka_1->f16;
    Blocka_1->f1078208 = v33;
    v38 = (uint32_t *)bmf_new(24 * v37 + 4);
    if ( v38 )
    {
      *v38 = v37;
      v39 = (SymList *)(v38 + 1);
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
    v43 = Blocka_1->f16 <= 0;
    Blocka_1->f1078212 = v39;
    if ( !v43 )
    {
      v44 = 0;
      do
      {
        __init_symbol_list(&Blocka_1->f1078208[v44], 0, 99, 0);
        __init_symbol_list(&Blocka_1->f1078212[v44++], 0, 33, 0);
      }
      while ( v44 < Blocka_1->f16 );
    }
    if ( Blocka_1->f4 > 0 )
    {
      v59 = Blocka_1->f1078236;
      v45 = 0;
      do
      {
        v61 = v45 + 1;
        *(Blocka_1->f56[5] + 3) = *(uint16_t *)(Blocka_1->f56[5] - 8) == 0;
        *(Blocka_1->f56[5] + 5) = *(uint16_t *)(Blocka_1->f56[6] - 8) == 0;
        v46 = Blocka_1->f56[4];
        v47 = Blocka_1->f56[3];
        v48 = Blocka_1->f56[2];
        v49 = Blocka_1->f56[1];
        v50 = Blocka_1->f56[0];
        Blocka_1->f56[4] = v47;
        Blocka_1->f56[3] = v48;
        Blocka_1->f56[2] = v49;
        Blocka_1->f56[1] = v50;
        Blocka_1->f56[0] = v46;
        v46 += 56;
        Blocka_1->f56[5] = v46;
        v50 += 56;
        Blocka_1->f56[6] = v50;
        Blocka_1->f56[7] = v49 + 56;
        Blocka_1->f56[8] = v48 + 56;
        Blocka_1->f56[9] = v47 + 56;
        // The same two flags, on the encoding side.
        {
          uint8_t zero = *(uint16_t *)(v50 + 8) == 0;
          *(v46 + 4) = zero;
          *(Blocka_1->f56[5] - 2) = zero;
          *(Blocka_1->f56[5] - 9) = zero;
          zero = *(uint16_t *)(Blocka_1->f56[6] + 16) == 0;
          *(Blocka_1->f56[5] + 6) = zero;
          *(Blocka_1->f56[5] - 1) = zero;
        }
        *(Blocka_1->f56[5] + 7) = *(uint16_t *)(Blocka_1->f56[6] + 24) == 0;
        v51 = *&Blocka_1->f56[6];
        v52 = *&Blocka_1->f56[7];
        Blocka_1->f56[5] += 8;
        v51 += 8;
        Blocka_1->f56[6] = v51;
        Blocka_1->f56[8] += 8;
        v52 += 8;
        Blocka_1->f56[7] = v52;
        Blocka_1->f56[9] += 8;
        Blocka_1->f1078692[0] = v51[26] + v51[18] + v51[10] + v51[2] + v51[34] - 5;
        // The same five counts as the line above, off the other row.
        Blocka_1->f1078692[3] = 0;
        Blocka_1->f1078692[2] = 0;
        v45 = v61;
        v53 = Blocka_1->f0;
        Blocka_1->f1078692[1] = v52[26] + v52[18] + v52[10] + v52[2] + v52[34] - 5;
        if ( v53 > 0 )
        {
          v54 = 0;
          do
          {
            ++v54;
            *(uint16_t *)(Blocka_1->f56[5] + 8 * v54 - 8) = *(uint16_t *)(v59 + 2 * v54 - 2);
            v53 = Blocka_1->f0;
          }
          while ( v54 < Blocka_1->f0 );
          v59 += 2 * v54;
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
          while ( v55 < Blocka_1->f0 );
          v45 = v61;
        }
      }
      while ( v45 < *(uint32_t *)&Blocka_1->f4 );
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
      uint16_t p_i[2];
      int32_t v49;
      int32_t v50;
      int32_t v51;
      int32_t v52;
      int32_t v53;
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80, "frame layout moved");
  ;
  uint8_t *v12;   // was int32_t: these hold addresses
  uint8_t v8;
  uint8_t *Srca_1, *Srca_2;   // `uint8_t *` beside the `char` scalars above
  uint8_t *__model_planes_buf;
  int32_t n1008, v17, v18, v19;
  __frame.v53 = a3;
  Srca_1 = Srca_3;
  v8 = plane_desc[a3 + 1].flags;
  __frame.v52 = 16 * a3;
  plane_predictor = v8 & 3;
  plane_alt_model = (uint8_t)(plane_desc[a3 + 1].flags & 4) >> 2;
  Srca_2 = Srca_3;
  __colour_transform((uint8_t *)Blockb, (uint8_t *)Srca_3, a3, a4);
  __model_planes_buf = ::hist_scratch;
  v12 = (uint8_t *)((uintptr_t)(::hist_scratch + 15) & 0xFFFFFFF0);
  *(uint64_t *)::hist_scratch = 0;
  *((uint32_t *)__model_planes_buf + 2) = 0;
  *((uint16_t *)__model_planes_buf + 6) = 0;
  __model_planes_buf[14] = 0;
  *((uint64_t *)__model_planes_buf + 126) = 0;
  *((uint64_t *)__model_planes_buf + 127) = 0;
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
    v17 = *((uint32_t *)Blockb + 1);
    v18 = *((uint32_t *)Blockb + 2);
    v19 = *((uint32_t *)Blockb + 3);
    *(uint32_t *)__frame.p_i = *(uint32_t *)Blockb;
    __frame.v49 = v17;
    __frame.v50 = v18;
    BYTE2(__frame.v50) = 72;
    __frame.v51 = v19;
    // never taken: -E is 0
    if ( plane_predictor == 1 && !plane_alt_model )
      __predict_med((uint8_t *)Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1));
    __model_plane((BmfImage *)__frame.p_i, (uint8_t *)(uint8_t *)Srca_1, (uint8_t *)Srca_2);
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
  uint8_t *Src;
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
  *((uint32_t *)::coded_buf + 4) = *(uint32_t *)&p_i->width;
  *((uint32_t *)p_ia_1 + 1) = p_i->stride;
  *((uint32_t *)p_ia_1 + 2) = *((uint32_t *)p_i + 2);
  *((uint32_t *)p_ia_1 + 3) = p_i->data_size;
  Srca_1 = (uint8_t *)((uint16_t *)p_i + 8);
  memcpy(__transform_planes_Buffer + 32,(uint8_t *)p_i + 16,p_i->data_size);
  Src_1 = (uint8_t *)bmf_new(p_i->width * p_i->height);
  Src_3 = Src_1;
  if ( plane_count > 0 )
  {
    Src = Src_1;
    Srca_3 = (uint16_t *)p_i + 8;
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
        __colour_transform((uint8_t *)p_ia, (uint8_t *)Src, v14, v11);
        if ( ::plane_predictor != 2 )
        {
          // never taken: -E is 0
          if ( ::plane_predictor == 1 )
          {
            __predict_med((uint8_t *)Src, p_i->width, p_i->height);
          }
        }
        Size_3 = p_i->width * p_i->height;
        n4_2 = plane_count;
        Src_2 = (uint8_t *)p_i + v14 + 16;
        if ( plane_count == 1 )
        {
          memcpy((uint8_t *)p_i + v14 + 16,Src,Size_3);
        }
        else
        {
          v20 = (uint8_t *)p_i + v14;
          if ( Size_3 <= 6
            || plane_count <= 0
            || (Src_2 <= Src || Size_3 > (uint32_t)(Src_2 - Src))
            && (plane_count > 1 || Src_2 >= Src || Src - Src_2 < (uint32_t)(Size_3 * plane_count)) )
          {
            Size_4 = 0;
            v24 = 0;
            do
            {
              v20[v24 + 16] = Src[Size_4];
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
              v20[v21 + 16] = Src[i];
              v21 += n4_2;
            }
            n4_1 = n4_3;
          }
        }
      }
    }
    while ( n4_1 < plane_count );
    Src_3 = Src;
    Srca_1 = (uint8_t *)Srca_3;
  }
  free(Src_3);
  // always taken: -S
  {
    __model_plane((BmfImage *)p_i, (uint8_t *)Srca_1, (uint8_t *)Srca_1);
  }
}

uint8_t * __expand_image(uint8_t *a1, int32_t a4, void **p_coded_buf)
{
  // This one is a layout, not a bag of locals: `tools/frame-sweep.sh --arrays`
  // gives every member its own storage and DLRAW exits 3 while decompressing.
  struct alignas(16) ExpandImageFrame {   // 104 bytes, one stack frame
      uint8_t *Block;
      uint16_t p_i[2];
      int32_t v81;
      int32_t n4_10;
      uint32_t Size;
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
  int8_t v10, v17, v18, v34, v35;
  uint8_t v20;
  uint8_t *Buffer_3, *n4_6, *n4_7, *v64;   // `uint8_t *` beside the `char` scalars above
  uint8_t has_coded;
  int32_t Buffer__1, v21, n4, predictor, v27, v28, v29, v30,
          ArgList, v33, n4_4, v37, n2_1, i, Size_4, Size_5, n4_3, v44, Size_2,
          Size_3, n4_2, v48, n2_2, n_planes, v55, Src_2, v58, n4_8, v61, i_1,
          n4_9, v76;
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
      goto LABEL_15;
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
LABEL_15:
    fclose(Stream_v);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  v10 = __frame.v92;
  ++*(uint32_t *)v5;
  if ( v10 < 0 )
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
  v18 = __frame.v88;
  p_i_1->flags |= __frame.v92 & 2 | (has_coded << 7);
  ::plane_count = ((v18 & 0x3Fu) + 7) >> 3;
  if ( (v17 & 0x20) == 0 )
  {
    ElementCount_1 = __frame.ElementCount;
    if ( fread((uint8_t *)p_i_1 + 16, 1u, __frame.ElementCount, ((BmfArc *)v5)->fp) != ElementCount_1 )
      goto LABEL_31;
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
LABEL_31:
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
      __unmodel_plane(ElementCount_2, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
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
    *(uint32_t *)__frame.p_i = 255;
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
          v27 = packer_acc | *(uint32_t *)__frame.p_i & (v70 << ((packer_free_bits + 8) & 31));
          packer_acc = v70 >> (-packer_free_bits & 31);
          packer_free_bits += 32;
        }
        else
        {
          LOBYTE(v27) = packer_acc & LOBYTE(__frame.p_i[0]);
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
            v28 = packer_acc | *(uint32_t *)__frame.p_i & (v69 << ((packer_free_bits + 8) & 31));
            packer_acc = v69 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v28 = packer_acc & *(uint32_t *)__frame.p_i;
            packer_acc = packer_acc >> 8;
          }
          plane_desc[n4 + 1].w4 = v28 - 64;
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            v68 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            v29 = packer_acc | *(uint32_t *)__frame.p_i & (v68 << ((packer_free_bits + 8) & 31));
            packer_acc = v68 >> (-packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v29 = packer_acc & *(uint32_t *)__frame.p_i;
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
              v30 = packer_acc | *(uint32_t *)__frame.p_i & (v67 << ((packer_free_bits + 8) & 31));
              packer_acc = v67 >> (-packer_free_bits & 31);
              packer_free_bits += 32;
            }
            else
            {
              v30 = packer_acc & *(uint32_t *)__frame.p_i;
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
  Src_1 = (uint8_t *)bmf_new(*(uint16_t *)p_i_1 * p_i_1->height);
  if ( (__frame.v92 & 8) != 0 )
  {
    *(uint32_t *)__frame.p_i = *(uint32_t *)&p_i_1->width;
    __frame.v81 = p_i_1->stride;
    __frame.n4_10 = *((uint32_t *)p_i_1 + 2);
    __frame.Size = p_i_1->data_size;
    BYTE2(__frame.n4_10) = 72;
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
          __unmodel_plane(ArgList, (uint16_t *)__frame.p_i, (uint8_t *)Src_1);
        if ( ::plane_predictor )
        {
          if ( ::plane_predictor == 1 )
          {
            if ( !plane_alt_model )
              __unpredict_med((uint8_t *)Src_1, *(uint16_t *)p_i_1, p_i_1->height);
          }
          // `else if ( !desc_slow_mode && ::plane_predictor == 2 )` -- the fast-mode
          // predictor-2 expander, never reached: -S is on.
        }
        else
        {
          __expand_predictor_mode0((uint32_t)Src_1, *(uint16_t *)p_i_1, p_i_1->height);
        }
        __interleave_plane((uint8_t *)p_i_1, (uint8_t *)(uint8_t *)Src_1, v33, v34);
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
      __unmodel_plane(ElementCount_2, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
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
          i = *(uint16_t *)p_i_1;
          __frame.Src = &((uint8_t *)p_i_1)[v37 + 16];
          Size_1 = i * p_i_1->height;
          __frame.n4_1 = ::plane_count;
          __frame.Size = Size_1;
          if ( ::plane_count == 1 )
          {
            memcpy((uint8_t *)Src_1,(uint8_t *)__frame.Src,__frame.Size);
            n2_2 = ::plane_predictor;
          }
          else
          {
            __frame.Block = &((uint8_t *)p_i_1)[v37];
            if ( (int32_t)__frame.Size <= 6
              || __frame.n4_1 <= 0
              || (__frame.n4_1 > 1 || Src_1 <= __frame.Src || Src_1 - (uint8_t *)__frame.Src < __frame.Size * ::plane_count)
              && (Src_1 >= __frame.Src || (uint8_t *)__frame.Src - Src_1 < __frame.Size) )
            {
              *(uint32_t *)__frame.p_i = n2_1;
              __frame.v81 = v37;
              __frame.n4_10 = n4_4;
              Size_2 = __frame.Size;
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
              *(uint32_t *)__frame.p_i = n2_1;
              __frame.v81 = v37;
              __frame.n4_10 = n4_4;
              Size_4 = __frame.Size;
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
            n2_2 = *(uint32_t *)__frame.p_i;
            v37 = __frame.v81;
            n4_4 = __frame.n4_10;
            p_i_1 = __frame.p_i_2;
          }
          if ( n2_2 )
          {
            if ( n2_2 == 1 )
              __unpredict_med((uint8_t *)Src_1, *(uint16_t *)p_i_1, p_i_1->height);
          }
          else
          {
            __expand_predictor_mode0((uint32_t)Src_1, *(uint16_t *)p_i_1, p_i_1->height);
          }
          __interleave_plane((uint8_t *)p_i_1, (uint8_t *)(uint8_t *)Src_1, v37, v35);
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
LABEL_107:
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
      goto LABEL_107;
  }
  if ( (p_i_1->flags & 2) != 0 )
  {
    n4_6 = (uint8_t *)bmf_new(p_i_1->data_size);
    n_planes = ::plane_count;
    v55 = p_i_1->height;
    __frame.n4_1 = (int32_t)n4_6;
    Src_2 = ::plane_count * (v55 - 1);
    memcpy(n4_6,(uint8_t *)p_i_1 + 16,p_i_1->data_size);
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
        i_1 = *(uint16_t *)p_i_1;
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
    i_2 = *(uint16_t *)p_i_1;
    *(uint16_t *)p_i_1 = v58;
    p_i_1->flags ^= 2u;
    p_i_1->height = i_2;
    *((uint16_t *)p_i_1 + 2) = v58 * n_planes;
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
  int32_t i, i_2, n4, n4_4, v21, v22, v23, Size, n0x7FFFFFFF, v32,
          n0x7FFFFFFF_8, n0x7FFFFFFF_2, n5, n2, v40, v41, v42, v43, v45,
          n16_1, n4_7, n4_8, v61, n16_2, n4_9, v69, v71, n16, plane, v76,
          n4_20, n4_6, n16_3, n4_11, v85, n4_16, n16_4, n4_12, n4_18, v102,
          v104, v106, v108, v109, n4_19, v118, v119, v120, v121, v123, v125,
          v126, i_6, v133, v135, v137, v139, v140, v149, n0x7FFFFFFF_10,
          n0x7FFFFFFF_4, n5_3, v155, n0x7FFFFFFF_11, n5_4, v160,
          n0x7FFFFFFF_6, n5_5, v166, n0x7FFFFFFF_9, n0x7FFFFFFF_3, n5_2, v172,
          n0x7FFFFFFF_1;
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
        __model_planes((uint8_t *)Blockb_1, (uint8_t *)Srca, v172, v19);
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
      v32 = __frame.v179[1];
      plane_desc[__frame.v180 + 1].flags = 5;
      __model_planes((uint8_t *)Blockb_2, (uint8_t *)Srca_1, v32, v19);
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
        __model_planes((uint8_t *)Blockb_3, (uint8_t *)Srca_2, v166, v19);
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
            __model_planes((uint8_t *)Blockb_4, (uint8_t *)Srca_3, v149, v19);
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
          __model_planes((uint8_t *)Blockb_5, (uint8_t *)Srca_4, v155, v19);
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
            __model_planes((uint8_t *)__frame.Blockb, (uint8_t *)Srca_5, v160, v19);
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
        v109 = *(uint16_t *)Blockb_6;
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
    v114 = *(uint16_t *)__frame.Blockb;
    v115 = v104 * LOWORD(__frame.v178[1]);
    *(uint16_t *)__frame.Blockb = v104;
    *((uint16_t *)Blockb_7 + 1) = v114;
    Blockb_7[11] ^= 2u;
    *((uint16_t *)Blockb_7 + 2) = v115;
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
          v140 = *(uint16_t *)Blockb_8;
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
      v145 = *(uint16_t *)__frame.Blockb;
      v146 = v135 * LOWORD(__frame.v178[1]);
      *(uint16_t *)__frame.Blockb = v135;
      *((uint16_t *)Blockb_9 + 1) = v145;
      Blockb_9[11] ^= 2u;
      *((uint16_t *)Blockb_9 + 2) = v146;
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
      memcpy(__frame.v178[0],(uint8_t *)p_i_1 + 16,p_i_1->data_size);
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
    n4_16 = (int32_t)__frame.n4_15;
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
LABEL_10:
    if ( !v9 )
LABEL_11:
      __exit_402E40(3, FileName);
    return v5;
  }
  if ( !v9 )
    goto LABEL_11;
  if ( !feof(v5->fp) )
  {
    __expand_image((uint8_t *)v5, 1, (void **)nullptr);
    Stream_1 = v5->fp;
    if ( !Stream_1 )
      goto LABEL_11;
    if ( !feof(v5->fp) )
    {
      v5->images = 0;
      fseek(Stream_1, 0, 0);
      v9 = v5->fp != nullptr;
      goto LABEL_10;
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
      uint8_t   hdr[16];   // the 16-byte archive member header `fwrite` sends in one call
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
  uint8_t *v5;   // were int32_t: these hold addresses
  FILE *i;
  bool v38;
  int8_t v13, v21, v36;
  uint8_t v12, v15, v17;
  uint8_t __compress_image_Buffer_1;   // 0/1, shifted into bit 7 of the header byte
  uint8_t *Buffera_5, *Buffera_6, *Srca, *Buffera_2, *Buffera_3;   // `uint8_t *` beside the `char` scalars above
  int32_t row_bytes, v11, v18, n4_6, bits_left, n4, v27, n8_1,
          acc, n4_1, v40, v41, v43, n4_3, v47, i_1, v50, n4_4, v56;
  BmfImage *p_i_1;
  uint16_t i_2, v53;
  uint32_t ElementCount_1, v25, v26, v28, v30, v31, v32, Size, v55;
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
  (*(uint32_t *)&__frame.hdr[0]) = *(uint32_t *)&p_i->width;
  p_i->flags |= __compress_image_Buffer_1 << 7;
  Buffera_5 = *((uint8_t **)p_i + 2);
  v11 = p_i->data_size;
  (*(int32_t *)&__frame.hdr[4]) = row_bytes;
  plane_desc[0].w4 = 512;
  v12 = p_i->depth;
  plane_desc[0].w12 = 0;
  (*(uint8_t **)&__frame.hdr[8]) = Buffera_5;
  (*(int32_t *)&__frame.hdr[12]) = v11;
  ::plane_count = ((v12 & 0x3Fu) + 7) >> 3;
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
  v17 = p_i->depth;
  HIBYTE(*(uint8_t **)&__frame.hdr[8]) |= 0x24;        // -S in bit 2, and bit 5 always set
  if ( (v17 & 0x3Fu) <= 4 )         // -F is on, so only the depth decides
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
      __model_plane((BmfImage *)p_i, (uint8_t *)p_i + 16, (uint8_t *)p_i + 16);
    goto LABEL_57;
  }
  __frame.ElementCount = __search_filter((BmfImage *)p_i, v13);
  HIBYTE(*(uint8_t **)&__frame.hdr[8]) |= 0x10u;
  if ( (p_i->flags & 2) != 0 )
  {
    n4_6 = p_i->stride;
    Buffera_6 = *((uint8_t **)p_i + 2);
    (*(uint32_t *)&__frame.hdr[0]) = *(uint32_t *)&p_i->width;
    v18 = p_i->data_size;
    (*(int32_t *)&__frame.hdr[4]) = n4_6;
    (*(uint8_t **)&__frame.hdr[8]) = Buffera_6;
    (*(int32_t *)&__frame.hdr[12]) = v18;
    HIBYTE(*(uint8_t **)&__frame.hdr[8]) = 0x34 | HIBYTE(Buffera_6);   // -S in bit 2
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
            v31 = v30 << (-n8_1 & 31);
            bits_left = n8_1 - 8;
            ::packer_acc |= v31;
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
    HIBYTE(*(uint8_t **)&__frame.hdr[8]) |= 8u;
    Srca = (uint8_t *)bmf_new(Size);
    if ( ::plane_count > 0 )
    {
      __frame.v66 = v5;
      n4_1 = 0;
      do
        __model_planes((uint8_t *)p_i, (uint8_t *)Srca, plane_desc[n4_1++ + 1].src_plane, v36);
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
  (*(int32_t *)&__frame.hdr[12]) = out_cursor - ::coded_buf;
  if ( v38 )
  {
    v39 = fwrite(__frame.hdr, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
    if ( coded_buf )
      v39 &= fwrite(coded_buf, 1u, *((uint32_t *)coded_buf + 1) + 8, ((BmfArc *)v5)->fp) == *((uint32_t *)coded_buf + 1) + 8;
    v40 = (fwrite(::coded_buf, 1u, ElementCounta, ((BmfArc *)v5)->fp) == ElementCounta) & v39;
    free(::coded_buf);
    if ( v40 && (p_i->depth & 0x80) != 0 )
      fwrite((uint8_t *)p_i + p_i->data_size + 16, 1u, ElementCount_1, ((BmfArc *)v5)->fp);
    fflush(((BmfArc *)v5)->fp);
    if ( v40 )
      return (*(int32_t *)&__frame.hdr[12]);
    return v40;
  }
  free(::coded_buf);
  if ( (p_i->flags & 2) != 0 )
  {
    Buffer_copy = (uint8_t *)bmf_new(p_i->data_size);
    v41 = p_i->height;
    (*(int32_t *)&__frame.hdr[4]) = ::plane_count;
    (*(uint8_t **)&__frame.hdr[8]) = Buffer_copy;
    (*(int32_t *)&__frame.hdr[12]) = ::plane_count * (v41 - 1);
    __frame.Buffer_2 = (uint16_t *)p_i + 8;
    memcpy(Buffer_copy,(uint8_t *)p_i + 16,p_i->data_size);
    LOWORD(v43) = p_i->height;
    if ( (uint16_t)v43 )
    {
      n4_3 = (*(int32_t *)&__frame.hdr[4]);
      ElementCountb = ElementCount_1;
      Buffera_2 = (*(uint8_t **)&__frame.hdr[8]);
      Buffera_3 = nullptr;
      v47 = 0;
      __frame.v66 = v5;
      do
      {
        i_1 = p_i_1->width;
        __frame.v65 = v47;
        (*(uint8_t **)&__frame.hdr[8]) = Buffera_3;
        v49 = (uint8_t *)p_i_1 + v47 + 16;
        v50 = (*(int32_t *)&__frame.hdr[12]);
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
        Buffera_3 = (*(uint8_t **)&__frame.hdr[8]) + 1;
      }
      while ( (int32_t)((*(uint8_t **)&__frame.hdr[8]) + 1) < v43 );
      ElementCount_1 = ElementCountb;
      v5 = __frame.v66;
    }
    __frame.Buffera_4 = Buffer_copy;
    i_2 = p_i_1->width;
    v53 = v43 * (*(int32_t *)&__frame.hdr[4]);
    p_i_1->width = v43;
    p_i_1->height = i_2;
    p_i_1->flags ^= 2u;
    p_i_1->stride = v53;
    free(__frame.Buffera_4);
    goto LABEL_77;
  }
LABEL_76:
  __frame.Buffer_2 = (uint16_t *)p_i + 8;
LABEL_77:
  v54 = fwrite(p_i_1, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
  if ( coded_buf )
    v54 &= fwrite(coded_buf, 1u, *((uint32_t *)coded_buf + 1) + 8, ((BmfArc *)v5)->fp) == *((uint32_t *)coded_buf + 1) + 8;
  v55 = fwrite(__frame.Buffer_2, 1u, ElementCount_1 + p_i_1->data_size, ((BmfArc *)v5)->fp);
  v56 = p_i_1->data_size;
  if ( (v54 & (v55 == v56 + ElementCount_1)) == 0 )
    return 0;
  return v56;
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
      if ( !*((uint32_t *)Block + 1) )
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
