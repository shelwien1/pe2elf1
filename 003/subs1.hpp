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
// (`(char *)model_geometry + v3 + 8`) rather than given a global of its own;
// the exception is 0x0044337D, byte 1 of the dword array based at coded_buf,
// which is indexed like an array and so got the byte global its neighbours
// __byte_44339D and __byte_4433AD already had.
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
// variable subscripts: `plane_desc[plane + 1].b2` walks four sixteen-byte
// records that Hex-Rays split into a name per field (REFACTORING.md Phase 3
// calls these SHARED, and all 41 are).  Giving them separate storage is what
// §3.6 is for; keeping one object is what makes it safe not to have done it
// yet.

alignas(16) static uint8_t bmf_ctx_group_flags[32] = {   // 0x439860
  0x00,0x01,0x02,0x04,0x08,0x0a,0x0d,0x10,0x11,0x16,0x20,0x23,0x24,0x38,0x3f,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
typedef uint8_t t_byte_439860[32];
static t_byte_439860& ctx_group_flags = *(t_byte_439860*)bmf_ctx_group_flags;
alignas(16) static uint8_t bmf_dword_439880[16] = {   // 0x439880
  0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
};
typedef int32_t t_dword_439880[4];
static t_dword_439880& __dword_439880 = *(t_dword_439880*)bmf_dword_439880;
alignas(16) static uint8_t bmf_byte_439890[16] = {   // 0x439890
  0x11,0x14,0x1b,0x25,0x31,0x46,0x5d,0x7c,0x9d,0xbf,0xcd,0xe4,0xeb,0xec,0xed,0x00,
};
typedef uint8_t t_byte_439890[16];
static t_byte_439890& __byte_439890 = *(t_byte_439890*)bmf_byte_439890;
alignas(16) static uint8_t bmf_byte_4398A0[32] = {   // 0x4398A0
  0x04,0x06,0x08,0x0b,0x0e,0x11,0x15,0x19,0x1e,0x25,0x2d,0x37,0x43,0x57,0x78,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
typedef uint8_t t_byte_4398A0[32];
static t_byte_4398A0& __byte_4398A0 = *(t_byte_4398A0*)bmf_byte_4398A0;
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
alignas(16) static uint8_t bmf_xmmword_439B10[16] = {   // 0x439B10
  0x7f,0x6a,0xbc,0x3c,0x7f,0x6a,0xbc,0x3c,0x7f,0x6a,0xbc,0x3c,0x7f,0x6a,0xbc,0x3c,
};
typedef __m128i t_xmmword_439B10;
static t_xmmword_439B10& __xmmword_439B10 = *(t_xmmword_439B10*)bmf_xmmword_439B10;
alignas(16) static uint8_t bmf_xmmword_439B40[16] = {   // 0x439B40
  0xcd,0xcc,0x4c,0x3d,0xcd,0xcc,0x4c,0x3d,0xcd,0xcc,0x4c,0x3d,0xcd,0xcc,0x4c,0x3d,
};
typedef __m128i t_xmmword_439B40;
static t_xmmword_439B40& __xmmword_439B40 = *(t_xmmword_439B40*)bmf_xmmword_439B40;
alignas(16) static uint8_t bmf_xmmword_439B50[16] = {   // 0x439B50
  0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,
};
typedef __m128i t_xmmword_439B50;
static t_xmmword_439B50& __xmmword_439B50 = *(t_xmmword_439B50*)bmf_xmmword_439B50;
alignas(16) static uint8_t bmf_xmmword_439B60[28] = {   // 0x439B60
  0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,0x52,0x49,0x1d,0x3b,0x66,0x66,
  0x06,0x40,0x00,0x00,0xf2,0x45,0x00,0x00,0x3c,0x42,
};
typedef __m128i t_xmmword_439B60;
static t_xmmword_439B60& __xmmword_439B60 = *(t_xmmword_439B60*)bmf_xmmword_439B60;
alignas(16) static uint8_t bmf_dword_439B7C[68] = {   // 0x439B7C
  0x33,0x33,0x29,0x43,0x00,0x00,0x80,0x3f,0x02,0x07,0x2e,0xc5,0x6f,0x12,0x83,0x3a,0x17,0xb7,
  0xd1,0x38,0xcd,0xcc,0xcc,0x3d,0x00,0x00,0x00,0x40,0x00,0x00,0x10,0x44,0x66,0x66,0x26,0x40,
  0x00,0x20,0xd2,0x46,0xf4,0xfd,0x54,0x3c,0x00,0x88,0x9d,0x45,0x00,0x40,0x04,0x44,0x17,0xb7,
  0x51,0x39,0x00,0x00,0x20,0x41,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
};
typedef int32_t t_dword_439B7C;
static t_dword_439B7C& __dword_439B7C = *(t_dword_439B7C*)bmf_dword_439B7C;
alignas(16) static uint8_t bmf_byte_439BC0[8] = {   // 0x439BC0
  0x01,0x02,0x04,0x08,0x0e,0x23,0x67,0x00,
};
typedef uint8_t t_byte_439BC0[8];
static t_byte_439BC0& __byte_439BC0 = *(t_byte_439BC0*)bmf_byte_439BC0;
alignas(16) static uint8_t bmf_byte_439BC8[8] = {   // 0x439BC8
  0x01,0x03,0x06,0x0a,0x10,0x1b,0x34,0x00,
};
typedef uint8_t t_byte_439BC8[8];
static t_byte_439BC8& __byte_439BC8 = *(t_byte_439BC8*)bmf_byte_439BC8;
alignas(16) static uint8_t bmf_byte_439BD0[8] = {   // 0x439BD0
  0x05,0x0a,0x24,0x62,0x9a,0xec,0xf8,0x00,
};
typedef uint8_t t_byte_439BD0[8];
static t_byte_439BD0& __byte_439BD0 = *(t_byte_439BD0*)bmf_byte_439BD0;
// The eight int32 its typedef always said it was.  deblob.py gave it the
// 30 024 bytes to the next global because that is the only upper bound the
// source offers; poisoning says the other 29 992 have no reader -- they were
// the string-table pointers the relocation layer used to rebase.
alignas(16) static uint8_t bmf_dword_439BD8[32] = {
  0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
};
typedef int32_t t_dword_439BD8[8];
static t_dword_439BD8& __dword_439BD8 = *(t_dword_439BD8*)bmf_dword_439BD8;
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
alignas(16) static uint8_t bmf_dwLowDateTime[4] = { 0x00,0x00,0x00,0x00 };
typedef int32_t t_dwLowDateTime;
static t_dwLowDateTime& __dwLowDateTime = *(t_dwLowDateTime*)bmf_dwLowDateTime;
alignas(16) static uint8_t bmf_plane_predictor[4] = {   // 0x443360
  0x00,0x00,0x00,0x00,
};
typedef int32_t t_n2;
static t_n2& plane_predictor = *(t_n2*)bmf_plane_predictor;
alignas(16) static uint8_t bmf_plane_alt_model[4] = {   // 0x443364
  0x00,0x00,0x00,0x00,
};
typedef int32_t t_dword_443364;
static t_dword_443364& plane_alt_model = *(t_dword_443364*)bmf_plane_alt_model;
alignas(16) static uint8_t bmf_packer_free_bits[4] = {   // 0x443368
  0x00,0x00,0x00,0x00,
};
typedef int32_t t_n8;
static t_n8& packer_free_bits = *(t_n8*)bmf_packer_free_bits;
alignas(16) static uint8_t bmf_packer_acc[4] = {   // 0x44336C
  0x00,0x00,0x00,0x00,
};
typedef int32_t t_n256;
static t_n256& packer_acc = *(t_n256*)bmf_packer_acc;
alignas(16) static uint8_t bmf_coded_size[20] = {   // 0x443370
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,
};
typedef int32_t t_ElementCount_0;
static t_ElementCount_0& coded_size = *(t_ElementCount_0*)bmf_coded_size;
static int32_t desc_slow_mode;   // was 0x443384 in bmf_bss
static int32_t __dword_443388;   // was 0x443388 in bmf_bss
// The plane-descriptor table: five 16-byte records, based at what used to be
// 0x44338C.  Record 0 holds the image-wide parameters and records 1..4 are the
// four planes, so the plane `p` a reader sees is record `p + 1`.  Three things
// say it is one table rather than twenty globals:
//
//   * the subscripts the code already writes -- `plane_desc[plane + 1].b2`,
//     `plane_desc[plane + 1].w4` -- both step whole records;
//   * field +1 is reached from two origins one record apart, `transform_planes`
//     pre-incrementing from 0 into `plane_desc[n].w0` and `rc_begin_encode`
//     indexing `plane_desc[p + 1].b1` by plane, which is what a header entry
//     in front of an array looks like;
//   * and `alt_model_p2_encode` walks all four plane records in one loop using
//     four of the record-0 names as its field bases.
//
// So `plane_count`, read 154 times as a scalar, is field +8 of record 0.
//
// The union is the one thing here that is still a question rather than a
// layout: records 1..4 use +0..+3 as four separate bytes, and record 0 has its
// +0..+3 read as a whole dword by the packer bit accounting at 18433.
struct PlaneDesc {
  union {
    int32_t w0;
    struct { uint8_t b0, b1, b2, b3; };
  };
  int32_t w4;
  int32_t w8;
  int32_t w12;
};
static_assert(sizeof(void *) != 4 || sizeof(PlaneDesc) == 16,
              "PlaneDesc: the layout moved");
static PlaneDesc plane_desc[5];

// The twenty names the table arrived as, now views of it at their old offsets.
// Each subscript that steps a whole record -- `[16 * p]` on a byte field,
// `[4 * p]` on a dword one -- still means what it meant, and reads record p+1.
static int32_t (&__n256_2)[20] = *(int32_t (*)[20])((uint8_t *)plane_desc + 0);
static int32_t (&__n512)[19] = *(int32_t (*)[19])((uint8_t *)plane_desc + 4);
static int32_t &plane_count = *(int32_t *)&plane_desc[0].w8;
static int32_t (&near_lossless_max)[17] = *(int32_t (*)[17])((uint8_t *)plane_desc + 12);
static uint8_t (&__byte_44339C)[64] = *(uint8_t (*)[64])((uint8_t *)plane_desc + 16);
static uint8_t (&__byte_44339D)[63] = *(uint8_t (*)[63])((uint8_t *)plane_desc + 17);
static uint8_t (&__byte_44339E)[62] = *(uint8_t (*)[62])((uint8_t *)plane_desc + 18);
static uint8_t (&__byte_44339F)[61] = *(uint8_t (*)[61])((uint8_t *)plane_desc + 19);
static int32_t (&__dword_4433A0)[15] = *(int32_t (*)[15])((uint8_t *)plane_desc + 20);
static int32_t (&__dword_4433A4)[14] = *(int32_t (*)[14])((uint8_t *)plane_desc + 24);
static int32_t (&__dword_4433A8)[13] = *(int32_t (*)[13])((uint8_t *)plane_desc + 28);
static uint8_t (&__byte_4433AC)[48] = *(uint8_t (*)[48])((uint8_t *)plane_desc + 32);
static uint8_t (&__byte_4433AD)[47] = *(uint8_t (*)[47])((uint8_t *)plane_desc + 33);
static uint8_t (&__byte_4433BD)[31] = *(uint8_t (*)[31])((uint8_t *)plane_desc + 49);
static char &__n3_1 = *(char *)&plane_desc[4].b0;
static char &__n3_0 = *(char *)&plane_desc[4].b1;
static char &__byte_4433CF = *(char *)&plane_desc[4].b3;
static int32_t &__n191 = *(int32_t *)&plane_desc[4].w4;
static int32_t &__n191_0 = *(int32_t *)&plane_desc[4].w8;
static int32_t &__n191_1 = *(int32_t *)&plane_desc[4].w12;
static int32_t model_geometry[32];   // was 0x445660 in bmf_bss
static char __byte_445700;   // was 0x445700 in bmf_bss
static int32_t __n8_1;   // was 0x44570C in bmf_bss
static int32_t __n8_0;   // was 0x445710 in bmf_bss
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
// d = 4 * near_lossless_max + 1, and every reader spells the same shape --
// `(x > deadzone_hi) - (x < deadzone_lo)`, a sign that is 0 inside the zone.
// Were 0x4458F0 and 0x4458F4 in bmf_bss.
static int32_t deadzone_hi;
static int32_t deadzone_lo;
// MSVC's CRT new-handler slot, and nothing else.  This was 10 292 bytes of
// CRT state because that is how far it was to the next global; one word of it
// is live -- `set_new_handler` writes it and `bmf_new` reads it (REFACTORING.md
// §6) -- and the other 10 288 bytes had no reader at all.
alignas(16) static uint8_t bmf_pout_of_memory_handler[4];
typedef int32_t t_pout_of_memory_handler;
static t_pout_of_memory_handler& __pout_of_memory_handler = *(t_pout_of_memory_handler*)bmf_pout_of_memory_handler;

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
static constexpr int32_t __dword_44108C = 1;   // -F  use filters
static constexpr int32_t __dword_441090 = 1;   // -S  slow but efficient
static constexpr int32_t __n2_4 = 0;   // -T  filter template
static constexpr int32_t __dword_441098 = 1;   // -N  pack the output
static constexpr int32_t __n7_0 = 9;   // -Q  filter search quality
static constexpr int32_t __n7_1 = 0;   // -E  max error, near-lossless
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
static char    (&exclusion_mask)[8192] = *(char (*)[8192])bss_exclusion;
static uint8_t (&__byte_445440)[544]   = *(uint8_t (*)[544])(bss_exclusion + 8192);
// The model's counter tables, ALGORITHM.md §8: one allocation, handed out in
// 254-entry strips.  An int32_t in the data segment holding an address, so a
// pointer here, and out of the blob.  BMF.exe had it at 0x00445708.
static uint16_t *model_tables;
// A five-entry table, and the extent is measured rather than assumed: the one
// site that subscripts it -- `*(uint16_t *)f56[5] = tbl44573C[n4]`, with `n4`
// out of `ModelBlock::f32` -- steps four bytes, and the four globals after
// 0x44573C were the elements it was stepping onto.  `init_model_tables` reads
// elements 1..3 as bare symbol values.  What the index means is not
// established, so the name still records the address rather than a role.
static int32_t tbl44573C[5];

// The plane descriptor table, `ALGORITHM.md` §6.2: four 16-byte records at
// 0x0044339C, whose fields the file also declares one at a time as
// __byte_44339C .. __dword_4433A8.
//
// Six places copy all four records in or out with 64-bit moves, and Hex-Rays
// wrote those as offsets from four *other* globals -- coded_buf,
// desc_slow_mode, __n256_2, plane_count -- which sit 32, 24, 16 and 8 bytes
// below the table.  That is the original compiler's strength reduction showing
// through, not something the program means; those four are a buffer pointer, a
// mode flag, a counter and a plane count, and none of them is a base for this.
// Written against the table itself the arithmetic is the same and the
// dependency on where four unrelated globals sit is gone.  REFACTORING.md §4.1.
static inline char *bmf_plane_desc(int32_t off)
{
  return (char *)__byte_44339C + off;
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

// Obj0 -- recovered from 260 dereferences over 54 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj0 {
  int32_t f0;   // signed: `alt_p1_alloc` tests `f0 > -10`
  int32_t f4;
  int32_t f8;
  uint32_t  f12[51];   // +12 .. +212
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj0, f12[50]) == 212,
              "Obj0: the layout moved");


// Obj1 -- recovered from 59 dereferences over 13 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
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
struct Obj1 {
  uint8_t _pad0[20];
  uint32_t f20;
  uint32_t f24;
  uint32_t f28;
  uint8_t _pad4[144];
  uint8_t  *row[5];           // +176 .. +192, rotated one place each pass
  uint8_t  *cur[5];           // +196 .. +212, derived from row
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj1, cur[4]) == 212,
              "Obj1: the layout moved");




// Obj3 -- recovered from 51 dereferences over 10 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj3 {
  uint8_t _pad0[2];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj3, f2) == 2,
              "Obj3: the layout moved");


// Obj4 -- recovered from 84 dereferences over 18 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj4 {
  int32_t   f0[8];   // +0 .. +28
  uint8_t _pad8[144];
  uint8_t  *f176[10];   // +176 .. +212, ten row cursors
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj4, f176[9]) == 212,
              "Obj4: the layout moved");








// Obj8 -- recovered from 50 dereferences over 20 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj8 {
  uint8_t _pad0[278528];
  uint64_t f278528;
  uint32_t f278536;
  uint16_t f278540;
  uint8_t f278542;
  uint8_t _pad5[97];
  uint64_t f278640;
  uint64_t f278648;
  uint8_t _pad8[4];
  int32_t *f278660;
  uint8_t *f278664;   // a row cursor
  uint8_t *f278668;   // a row cursor
  uint8_t *f278672;   // a row cursor
  uint8_t _pad13[60];
  uint8_t  *f278736[10];   // +278736 .. +278772, row cursors
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj8, f278736[9]) == 278772,
              "Obj8: the layout moved");




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
  uint32_t f44;
  uint32_t f48;
  uint32_t f52;
  uint8_t  *f56[14];   // +56 .. +108, the row cursors: every element is an address
  uint8_t _pad15[1051552];
  uint8_t  *f1051664[4];   // +1051664 .. +1051676, four more row cursors
  uint8_t _pad16[26524];
  void*f1078204;
  uint8_t *f1078208;   // the symbol lists, 24 bytes each
  uint8_t *f1078212;   // the symbol lists, 24 bytes each
  uint8_t *f1078216;   // where the symbol lists end
  uint8_t *f1078220;   // where the symbol lists end
  uint32_t f1078224;
  uint8_t _pad22[4];
  uint32_t **f1078232;   // the symbol-list cursor: a walk over uint32_t *
  int32_t f1078236;
  uint8_t *f1078240;   // a row cursor
  uint8_t _pad25[440];
  uint8_t *f1078684;
  uint8_t *f1078688;   // the alphabet map, one byte a symbol
  uint8_t f1078692[4];   // +1078692 .. +1078695
  uint8_t _pad28[4980736];
  uint32_t f6059432;
  uint8_t *f6059436;   // a row cursor; `f6059436 + 2` steps two bytes
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(ModelBlock, f6059436) == 6059436,
              "ModelBlock: the layout moved");


// Obj11 -- recovered from 353 dereferences over 39 offsets, under 23
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj11 {
  uint8_t _pad0[278528];
  __m128    f278528[21];   // +278528 .. +278848
  uint8_t _pad22[32];
  __m128    f278896[5];   // +278896 .. +278960
  uint8_t _pad28[1008];
  __m128 f279984;
  uint8_t _pad30[240];
  __m128 f280240;
  uint8_t _pad32[240];
  __m128 f280496;
  uint8_t _pad34[240];
  __m128 f280752;
  uint8_t _pad36[96];
  __m128 f280864;
  uint8_t _pad38[3824];
  __m128 f284704;
  uint8_t _pad40[131056];
  __m128 f415776;
  uint8_t _pad42[131056];
  __m128 f546848;
  uint8_t _pad44[131056];
  __m128 f677920;
  uint8_t _pad46[131056];
  __m128 f808992;
  uint8_t _pad48[131056];
  __m128 f940064;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj11, f940064) == 940064,
              "Obj11: the layout moved");


// Obj12 -- recovered from 49 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj12 {
  // Six pointers to the same shape, one per sub-model, which is what
  // `alt_p2_filter` treats them as: it walks all six with one weight each.
  __m128 *f0[6];   // +0 .. +20
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj12, f0[5]) == 20,
              "Obj12: the layout moved");





// Obj15 -- recovered from 28 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj15 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
  uint8_t f4;
  uint8_t _pad4[1];
  uint16_t f6;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj15, f6) == 6,
              "Obj15: the layout moved");






// Obj18 -- recovered from 26 dereferences over 8 offsets, under 8
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj18 {
  uint64_t f0;
  uint64_t f8;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj18, f8) == 8,
              "Obj18: the layout moved");


// Obj19 -- recovered from 26 dereferences over 10 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj19 {
  uint8_t _pad0[278736];
  uint8_t  *f278736[6];   // +278736 .. +278756, row cursors
  char *f278760;
  char *f278764;
  char *f278768;
  char *f278772;   // a row cursor, like its three neighbours
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj19, f278772) == 278772,
              "Obj19: the layout moved");






// Obj22 -- recovered from 24 dereferences over 8 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj22 {
  uint8_t _pad0[1];
  uint8_t f1;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj22, f1) == 1,
              "Obj22: the layout moved");


// Obj23 -- recovered from 23 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj23 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
  uint8_t f4;
  uint8_t _pad4[1];
  int16_t f6;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj23, f6) == 6,
              "Obj23: the layout moved");




// Obj25 -- recovered from 22 dereferences over 9 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj25 {
  uint8_t _pad0[8];
  uint8_t*f8;
  uint8_t*f12;
  uint8_t*f16;
  uint8_t*f20;
  uint8_t _pad5[172];
  uint8_t*f196;
  uint8_t*f200;
  uint8_t*f204;
  uint8_t*f208;
  uint8_t*f212;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj25, f212) == 212,
              "Obj25: the layout moved");












// Obj31 -- recovered from 19 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj31 {
  uint8_t _pad0[278736];
  uint8_t *f278736;   // a row cursor
  uint8_t _pad2[16];
  char *f278756;
  char *f278760;
  char *f278764;
  char *f278768;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj31, f278768) == 278768,
              "Obj31: the layout moved");









// Obj36 -- recovered from 15 dereferences over 5 offsets, under 6
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj36 {
  int16_t f0;
  int16_t f2;
  uint8_t _pad2[14];
  int16_t f18;
  uint8_t _pad4[16];
  int16_t f36;
  int16_t f38;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj36, f38) == 38,
              "Obj36: the layout moved");




// Obj38 -- recovered from 12 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj38 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj38, f2) == 2,
              "Obj38: the layout moved");


// Obj39 -- recovered from 12 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj39 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj39, f2) == 2,
              "Obj39: the layout moved");


// Obj40 -- recovered from 12 dereferences over 2 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj40 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj40, f2) == 2,
              "Obj40: the layout moved");


// Obj41 -- recovered from 12 dereferences over 2 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj41 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj41, f2) == 2,
              "Obj41: the layout moved");


// Obj42 -- recovered from 12 dereferences over 2 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj42 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj42, f2) == 2,
              "Obj42: the layout moved");




// Obj44 -- recovered from 11 dereferences over 2 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj44 {
  uint8_t f0;
  uint8_t _pad1[1];
  int16_t f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj44, f2) == 2,
              "Obj44: the layout moved");


// Obj45 -- recovered from 10 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj45 {
  uint32_t  f0[4];   // +0 .. +12
  uint8_t _pad4[4];
  uint32_t f20;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj45, f20) == 20,
              "Obj45: the layout moved");


// Obj46 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj46 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj46, f4) == 4,
              "Obj46: the layout moved");


// Obj47 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj47 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj47, f4) == 4,
              "Obj47: the layout moved");


// Obj48 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj48 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj48, f4) == 4,
              "Obj48: the layout moved");


// Obj49 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj49 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj49, f4) == 4,
              "Obj49: the layout moved");


// Obj50 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj50 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj50, f4) == 4,
              "Obj50: the layout moved");


// Obj51 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj51 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj51, f4) == 4,
              "Obj51: the layout moved");


// Obj52 -- recovered from 10 dereferences over 2 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj52 {
  const char f0;
  uint8_t _pad1[3];
  const char f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj52, f4) == 4,
              "Obj52: the layout moved");



















// Obj63 -- recovered from 9 dereferences over 9 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj63 {
  uint8_t _pad0[940064];
  uint16_t  f940064[5];   // +940064 .. +940072
  uint8_t _pad6[6];
  uint16_t  f940080[4];   // +940080 .. +940086
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj63, f940080[3]) == 940086,
              "Obj63: the layout moved");


// Obj64 -- recovered from 8 dereferences over 5 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj64 {
  int16_t f0;
  uint8_t _pad1[2];
  int16_t f4;
  uint8_t _pad3[12];
  int16_t f18;
  uint8_t _pad5[16];
  int16_t f36;
  int16_t f38;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj64, f38) == 38,
              "Obj64: the layout moved");




// Obj66 -- recovered from 8 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj66 {
  uint16_t f0;
  uint16_t f2;
  uint16_t f4;
  uint8_t _pad3[2];
  uint16_t f8;
  uint16_t f10;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj66, f10) == 10,
              "Obj66: the layout moved");


// Obj67 -- recovered from 7 dereferences over 4 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj67 {
  int16_t f0;
  uint8_t _pad1[16];
  int16_t f18;
  uint8_t _pad3[16];
  int16_t f36;
  int16_t f38;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj67, f38) == 38,
              "Obj67: the layout moved");


// Obj68 -- recovered from 7 dereferences over 4 offsets, under 5
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj68 {
  int16_t f0;
  int16_t f2;
  uint8_t _pad2[14];
  int16_t f18;
  uint8_t _pad4[16];
  int16_t f36;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj68, f36) == 36,
              "Obj68: the layout moved");


// ModelBlock -- recovered from 328 dereferences over 41 offsets, under 17
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.



// Obj69 -- recovered from 279 dereferences over 53 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj69 {
  uint8_t _pad0[278528];
  __m128    f278528[7];   // +278528 .. +278624
  uint64_t f278640;
  uint64_t f278648;
  __m128 *f278656;
  int32_t *f278660;
  uint8_t *f278664;   // a row cursor
  uint8_t *f278668;   // a row cursor
  uint8_t *f278672;   // a row cursor
  uint8_t _pad15[24];
  uint16_t f278700;
  uint8_t _pad17[2];
  uint32_t f278704;
  uint32_t f278708;
  uint32_t f278712;
  uint8_t _pad21[12];
  uint32_t f278728;
  uint8_t _pad23[4];
  uint32_t  f278736[5];   // +278736 .. +278752
  char *f278756;
  uint32_t  f278760[24];   // +278760 .. +278852
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj69, f278760[23]) == 278852,
              "Obj69: the layout moved");








































// Obj90 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj90 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj90, f0) == 0,
              "Obj90: the layout moved");


// Obj91 -- recovered from 6 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj91 {
  uint8_t _pad0[8];
  uint32_t f8;
  uint8_t _pad2[184];
  uint32_t f196;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj91, f196) == 196,
              "Obj91: the layout moved");








// Obj95 -- recovered from 6 dereferences over 5 offsets, under 5
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj95 {
  int16_t f0;
  int16_t f2;
  uint8_t _pad2[14];
  int16_t f18;
  uint8_t _pad4[18];
  int16_t f38;
  uint8_t _pad6[14];
  int16_t f54;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj95, f54) == 54,
              "Obj95: the layout moved");


// Obj96 -- recovered from 6 dereferences over 4 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj96 {
  int16_t f0;
  uint8_t _pad1[16];
  int16_t f18;
  uint8_t _pad3[18];
  int16_t f38;
  uint8_t _pad5[14];
  int16_t f54;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj96, f54) == 54,
              "Obj96: the layout moved");


// Obj97 -- recovered from 6 dereferences over 3 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj97 {
  uint16_t f0;
  uint8_t _pad1[2];
  uint16_t f4;
  uint8_t _pad3[6];
  uint32_t f12;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj97, f12) == 12,
              "Obj97: the layout moved");



// Obj99 -- recovered from 6 dereferences over 6 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj99 {
  uint8_t _pad0[8];
  uint16_t f8;
  uint8_t _pad2[8];
  uint8_t f18;
  uint8_t _pad4[7];
  uint8_t f26;
  uint8_t _pad6[7];
  uint8_t f34;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj99, f34) == 34,
              "Obj99: the layout moved");






// Obj102 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj102 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj102, f0) == 0,
              "Obj102: the layout moved");


// Obj103 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj103 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj103, f0) == 0,
              "Obj103: the layout moved");


// Obj104 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj104 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj104, f0) == 0,
              "Obj104: the layout moved");


// Obj105 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj105 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj105, f0) == 0,
              "Obj105: the layout moved");


// Obj106 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj106 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj106, f0) == 0,
              "Obj106: the layout moved");


// Obj107 -- recovered from 6 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj107 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj107, f0) == 0,
              "Obj107: the layout moved");


// Obj108 -- recovered from 6 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj108 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj108, f0) == 0,
              "Obj108: the layout moved");


// Obj109 -- recovered from 6 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj109 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj109, f0) == 0,
              "Obj109: the layout moved");


// Obj110 -- recovered from 6 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj110 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj110, f0) == 0,
              "Obj110: the layout moved");


// Obj111 -- recovered from 6 dereferences over 1 offsets, under 3
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj111 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj111, f0) == 0,
              "Obj111: the layout moved");


// Obj112 -- recovered from 6 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj112 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj112, f0) == 0,
              "Obj112: the layout moved");


// Obj113 -- recovered from 5 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj113 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj113, f0) == 0,
              "Obj113: the layout moved");








// Obj117 -- recovered from 5 dereferences over 4 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj117 {
  int16_t f0;
  uint8_t _pad1[34];
  int16_t f36;
  uint8_t _pad3[16];
  int16_t f54;
  uint8_t _pad5[16];
  int16_t f72;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj117, f72) == 72,
              "Obj117: the layout moved");


// Obj118 -- recovered from 5 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj118 {
  uint8_t _pad0[2];
  uint8_t f2;
  uint8_t f3;
  uint8_t _pad3[7];
  uint8_t f11;
  uint8_t _pad5[7];
  uint8_t f19;
  uint8_t _pad7[7];
  uint8_t f27;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj118, f27) == 27,
              "Obj118: the layout moved");






// Obj121 -- recovered from 5 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj121 {
  uint8_t _pad0[16];
  uint16_t f16;
  uint8_t _pad2[6];
  uint16_t f24;
  uint8_t _pad4[6];
  uint16_t f32;
  uint8_t _pad6[6];
  uint16_t f40;
  uint8_t _pad8[14];
  uint16_t f56;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj121, f56) == 56,
              "Obj121: the layout moved");






// Obj124 -- recovered from 5 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj124 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj124, f0) == 0,
              "Obj124: the layout moved");


// Obj125 -- recovered from 5 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj125 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj125, f0) == 0,
              "Obj125: the layout moved");


// Obj126 -- recovered from 5 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj126 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj126, f0) == 0,
              "Obj126: the layout moved");


// Obj127 -- recovered from 5 dereferences over 1 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj127 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj127, f0) == 0,
              "Obj127: the layout moved");






// Obj130 -- recovered from 5 dereferences over 5 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj130 {
  uint8_t _pad0[108];
  uint32_t  f108[4];   // +108 .. +120
  uint16_t f124;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj130, f124) == 124,
              "Obj130: the layout moved");












// Obj5 -- recovered from 5 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj5 {
  int16_t f0;
  uint8_t _pad1[2];
  int16_t f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj5, f4) == 4,
              "Obj5: the layout moved");


// Obj6 -- recovered from 4 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj6 {
  uint32_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj6, f0) == 0,
              "Obj6: the layout moved");


// Obj7 -- recovered from 4 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj7 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj7, f0) == 0,
              "Obj7: the layout moved");


// Obj13 -- recovered from 4 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj13 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj13, f0) == 0,
              "Obj13: the layout moved");


// Obj21 -- recovered from 4 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj21 {
  int16_t f0;
  uint8_t _pad1[2];
  int16_t f4;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj21, f4) == 4,
              "Obj21: the layout moved");


// Obj24 -- recovered from 4 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj24 {
  uint8_t _pad0[6678448];
  uint16_t f6678448;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj24, f6678448) == 6678448,
              "Obj24: the layout moved");


// Obj2 -- recovered from 4 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj2 {
  const char f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj2, f0) == 0,
              "Obj2: the layout moved");


// Obj9 -- recovered from 3 dereferences over 3 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj9 {
  uint16_t f0;
  uint8_t _pad1[4];
  uint16_t f6;
  uint8_t _pad3[4];
  uint16_t f12;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj9, f12) == 12,
              "Obj9: the layout moved");


// Obj14 -- recovered from 3 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj14 {
  int16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj14, f0) == 0,
              "Obj14: the layout moved");


// Obj16 -- recovered from 6 dereferences over 4 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj16 {
  char f0;
  uint8_t _pad1[252];
  char f253;
  char f254;
  char f255;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj16, f255) == 255,
              "Obj16: the layout moved");


// Obj17 -- recovered from 3 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj17 {
  uint8_t _pad0[1];
  uint8_t f1;
  uint8_t _pad2[3];
  uint8_t f5;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj17, f5) == 5,
              "Obj17: the layout moved");


// Obj20 -- recovered from 3 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj20 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj20, f0) == 0,
              "Obj20: the layout moved");


// Obj26 -- recovered from 3 dereferences over 3 offsets, under 2
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj26 {
  uint8_t _pad0[3784];
  uint16_t f3784;
  uint8_t _pad2[14];
  uint16_t f3800;
  uint8_t _pad4[14];
  uint16_t f3816;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj26, f3816) == 3816,
              "Obj26: the layout moved");


// Obj27 -- recovered from 3 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj27 {
  uint8_t _pad0[3816];
  uint16_t f3816;
  uint16_t f3818;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj27, f3818) == 3818,
              "Obj27: the layout moved");


// Obj28 -- recovered from 3 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj28 {
  uint8_t _pad0[3784];
  uint16_t f3784;
  uint16_t f3786;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj28, f3786) == 3786,
              "Obj28: the layout moved");






































































// Obj35 -- recovered from 3 dereferences over 3 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj35 {
  __m128i f0;
  __m128i f16;
  __m128i f32;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj35, f32) == 32,
              "Obj35: the layout moved");




// Obj29 -- recovered from 3 dereferences over 3 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj29 {
  int16_t f0;
  uint8_t _pad1[16];
  int16_t f18;
  uint8_t _pad3[16];
  int16_t f36;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj29, f36) == 36,
              "Obj29: the layout moved");


// Obj30 -- recovered from 3 dereferences over 3 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj30 {
  int32_t f0;
  int32_t f4;
  int32_t f8;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj30, f8) == 8,
              "Obj30: the layout moved");


// Obj32 -- recovered from 3 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj32 {
  uint16_t f0;
  char f2;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj32, f2) == 2,
              "Obj32: the layout moved");


// Obj33 -- recovered from 3 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj33 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj33, f0) == 0,
              "Obj33: the layout moved");


// Obj34 -- recovered from 3 dereferences over 1 offsets, under 4
// names.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj34 {
  uint16_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj34, f0) == 0,
              "Obj34: the layout moved");


// Obj37 -- recovered from 2 dereferences over 1 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj37 {
  uint8_t f0;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj37, f0) == 0,
              "Obj37: the layout moved");


// Obj43 -- recovered from 2 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj43 {
  uint8_t f0;
  uint8_t _pad1[127];
  uint8_t f128;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj43, f128) == 128,
              "Obj43: the layout moved");


// Obj53 -- recovered from 2 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj53 {
  uint16_t f0;
  uint8_t _pad1[4];
  uint16_t f6;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj53, f6) == 6,
              "Obj53: the layout moved");


// Obj54 -- recovered from 2 dereferences over 2 offsets, under 1
// name.  The layout is the one the code already assumed: at 32 bits a
// pointer is four bytes, so naming these fields moves nothing, and the
// static_assert is what says so.  Offsets the code only reaches with a
// computed index are padding here -- their bounds are not visible.
struct Obj54 {
  uint64_t f0;
  uint8_t _pad1[6];
  char f14;
};
static_assert(sizeof(void *) != 4
              || __builtin_offsetof(Obj54, f14) == 14,
              "Obj54: the layout moved");


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
static inline FILE * __fwd_bmf_destroy_archive_bmf_close_archive(void *a0) { return __bmf_close_archive((BmfArc *)a0); }

BmfArc *__bmf_destroy_archive(BmfArc *Block, char a2)
{
  ;
  __fwd_bmf_destroy_archive_bmf_close_archive(Block);
  if ( (a2 & 1) != 0 )
    free(Block);
  return Block;
}

void __expand_predictor_mode0(uint32_t Src, int32_t i, int32_t a3)
{
  ;
  int32_t n256, n256_1;
  n256 = plane_desc[0].w12;
  n256_1 = plane_desc[0].w12;
  // never taken: -E is 0
}

uint32_t __predict_med(char *Src, int32_t i, int32_t a3)
{
  ;
  char v26;
  int32_t i_1, v16, v17, v18, v19, v20, v21, v22, v24, v27, v29;
  uint32_t j, n15, v23, v25, n15_1;
  Obj37 *v28;
  uint8_t *v3, *v4;
  alignas(16) uint8_t v31[272];
  v3 = (uint8_t *)(Src + a3 * i);
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
      v28 = (Obj37 *)(&v3[-v27]);
      n15 = (uint8_t)(v28->f0 - *((char *)v28 - 1));
      v28->f0 = v31[(uint8_t)n15];
    }
  }
  return n15;
}

uint32_t __alt_init_tables(Obj43 *a1, Obj16 *a2)
{
  ;
  char *v18, *v19, v48, v53;
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
  a2->f0 = 0;
  v18 = (char *)a2 + 2;
  v19 = (char *)a2 + 1;
  a2->f255 = 0x80;
  if ( ((char *)a2 + 1 <= (char *)a2 + 2 || (uint32_t)(v19 - v18) < 0xFE) && (v18 <= v19 || (uint32_t)(v18 - v19) < 0xFE) )
  {
    for ( i = 0; i < 0x3F; ++i )
    {
      ((char *)a2)[4 * i + 2] = 2 * i + 1;
      ((char *)a2)[4 * i + 1] = -2 * i - 1;
      ((char *)a2)[4 * i + 4] = 2 * i + 2;
      ((char *)a2)[4 * i + 3] = -2 * i - 2;
    }
    a2->f254 = 127;
    a2->f253 = -127;
  }
  else
  {
    for ( j = 0; j < 0x3F; ++j )
    {
      ((char *)a2)[4 * j + 2] = 2 * j + 1;
      ((char *)a2)[4 * j + 4] = 2 * j + 2;
    }
    a2->f254 = 127;
    for ( k = 0; k < 0x3F; ++k )
    {
      ((char *)a2)[4 * k + 1] = -2 * k - 1;
      ((char *)a2)[4 * k + 3] = -2 * k - 2;
    }
    a2->f253 = -127;
  }
  // never taken: -E is 0
  // The test here was `if ( plane_predictor )`, with an else for predictor
  // mode 0.  This function is called only by alt_p1_alloc and alt_p2_alloc,
  // and those only by the eight alt_model bodies, which the dispatch reaches
  // only under the predictor being 1 or 2.  Always true, so it and its
  // 22-line else are gone.  Same argument as the block above it.
  a1->f0 = 0;
  n128_6 = 1;
  a1->f128 = -1;
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
    ((uint8_t *)a1)[(uint8_t)((char *)a2)[2 * n0x80] + 256] = 2 * n0x80;
    ((uint8_t *)a1)[(uint8_t)((char *)a2)[2 * n0x80 + 1] + 256] = 2 * n0x80 + 1;
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

int32_t __encode_symbol_list(uint32_t *_this, int32_t a2)
{
  ;
  char v2, v27, *v28, v29, v32, v33, *v36, *v37, *v39, *v40, *v49;
  int16_t v51;
  int32_t enc_cum, enc_high, enc_tot, v3, v5, v6, v7, v8, v24, n251, v35, v38,
          v41, v43, v47, v53, v54;
  Obj90 *v4;
  uint16_t *v25, v26, v31;
  uint32_t i_1, i, v34, v42, v44, *this_1;
  v2 = __byte_445700;
  v3 = *(_this + 1);
  v4 = (Obj90 *)((uint16_t *)(*(_this + 5) - 3));
  v5 = 0;
  while ( 1 )
  {
    v4 = (Obj90 *)((uint16_t *)((char *)v4 + 3));
    v6 = v4->f0;
    if ( exclusion_mask[v6] != __byte_445700 )
    {
      v7 = *((uint8_t *)v4 + 2);
      v5 += v7;
      if ( v6 == a2 )
        break;
    }
    if ( !--v3 )
    {
      if ( !v5 )
        return 0;
      enc_cum = v5;
      enc_tot = *(_this + 2) + v5;
      enc_high = enc_tot;
      do
      {
        exclusion_mask[v4->f0] = v2;
        v4 = (Obj90 *)((uint16_t *)((char *)v4 - 3));
      }
      while ( (uint32_t)v4 >= *(_this + 5) );
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
      if ( exclusion_mask[*(uint16_t *)((char *)v4 + 3 * i + 3)] == __byte_445700 )
        v24 = 0;
      else
        v24 = *((uint8_t *)v4 + 3 * i + 5);
      v5 += v24;
    }
    _this = this_1;
  }
  enc_tot = *(_this + 2) + v5;
  *((uint8_t *)v4 + 2) += 4;
  v25 = (uint16_t *)*(_this + 5);
  *(_this + 3) += 4;
  if ((uint16_t *)v4 == v25)
  {
LABEL_37:
    n251 = *((uint8_t *)v4 + 2);
  }
  else
  {
    v26 = v4->f0;
    v27 = *((uint8_t *)v4 + 2);
    v28 = (char *)v4 - 3;
    v29 = *((uint8_t *)v4 - 1);
    v4->f0 = *(uint16_t *)((char *)v4 - 3);
    *((uint8_t *)v4 + 2) = v29;
    *(uint16_t *)v28 = v26;
    v28[2] = v27;
    v25 = (uint16_t *)*(_this + 5);
    if ( (uint16_t *)((char *)v4 - 3) == v25 )
    {
      n251 = *((uint8_t *)v4 - 1);
    }
    else
    {
      while ( 1 )
      {
        n251 = (uint8_t)v28[2];
        v4 = (Obj90 *)((uint16_t *)(v28 - 3));
        if ( n251 <= (uint8_t)*(v28 - 1) )
          break;
        v31 = *(uint16_t *)v28;
        v32 = v28[2];
        v33 = *(v28 - 1);
        *(uint16_t *)v28 = v4->f0;
        v28[2] = v33;
        v4->f0 = v31;
        *(v28 - 1) = v32;
        v25 = (uint16_t *)*(_this + 5);
        v28 -= 3;
        if ((uint16_t *)v4 == v25)
          goto LABEL_37;
      }
    }
  }
  v34 = *(_this + 4);
  if ( n251 > 251 || v34 < *(_this + 3) )
  {
    v35 = *(_this + 1);
    v54 = v34 < 20 * *_this;
    v36 = (char *)v25 - 3;
    do
    {
      v37 = v36;
      v36 += 3;
      v38 = (v54 + (uint32_t)(uint8_t)v36[2]) >> 1;
      v36[2] = v38;
      if ( v36 != (char *)*(_this + 5) )
      {
        v39 = v36 - 3;
        v53 = (uint8_t)*(v36 - 1);
        if ( v38 > v53 )
        {
          v51 = *(uint16_t *)v36;
          *(uint16_t *)v36 = *(uint16_t *)v39;
          v36[2] = v53;
          if ( v39 != (char *)*(_this + 5) )
          {
            v49 = v36;
            v47 = v35;
            do
            {
              v40 = v39 - 3;
              v41 = (uint8_t)*(v39 - 1);
              if ( v38 <= v41 )
                break;
              *(uint16_t *)v39 = *(uint16_t *)v40;
              v39[2] = v41;
              v39 -= 3;
            }
            while ( v40 != (char *)*(_this + 5) );
            v36 = v49;
            v35 = v47;
          }
          *(uint16_t *)v39 = v51;
          v39[2] = v38;
        }
      }
      --v35;
    }
    while ( v35 );
    v42 = *(_this + 2);
    if ( !v36[2] )
    {
      do
      {
        ++v35;
        *(_this + 2) = ++v42;
        v43 = (uint8_t)v37[2];
        v37 -= 3;
      }
      while ( !v43 );
      *(_this + 1) -= v35;
    }
    v44 = *(_this + 3);
    *(_this + 2) = v42 - (v42 >> 1);
    *(_this + 3) = v44 - (v44 >> 1);
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
void __symbol_list_update(Obj6 *_this, int32_t a2, uint32_t a3)
{
  ;
  char *list;         // the three-byte entries, `_this[5]`
  uint32_t count;     // a symbol's count byte, while it is being compared
  bool v7;
  char v11, v12, v15, *v16, v17, v19, v20, *v23, *v24, *v26, *v27, *v33;
  int16_t v10, v34;
  int32_t v8, v9, v22, v25, v28, v30, v32, v35, v36;
  uint16_t *n251_1, *n251_2, v14, v18;
  uint32_t v4, v6, v21, v29, v31;
  list = (char *)*((uint32_t *)_this + 5);
  v4 = *((uint32_t *)_this + 1);
  n251_1 = (uint16_t *)list;
  v6 = v4;
  if ( v4 )
  {
    while ( *n251_1 != a2 )
    {
      n251_1 = (uint16_t *)((char *)n251_1 + 3);
      if ( !--v6 )
        goto LABEL_4;
    }
    *((uint8_t *)n251_1 + 2) += a3;
    *((uint32_t *)_this + 3) += a3;
    n251_2 = (uint16_t *)*((uint32_t *)_this + 5);
    if ( n251_1 == n251_2 )
    {
LABEL_16:
      count = *((uint8_t *)n251_1 + 2);
    }
    else
    {
      v14 = *n251_1;
      v15 = *((uint8_t *)n251_1 + 2);
      v16 = (char *)n251_1 - 3;
      v17 = *((uint8_t *)n251_1 - 1);
      *n251_1 = *(uint16_t *)((char *)n251_1 - 3);
      *((uint8_t *)n251_1 + 2) = v17;
      *(uint16_t *)v16 = v14;
      v16[2] = v15;
      n251_2 = (uint16_t *)*((uint32_t *)_this + 5);
      if ( (uint16_t *)((char *)n251_1 - 3) == n251_2 )
      {
        count = *((uint8_t *)n251_1 - 1);
      }
      else
      {
        while ( 1 )
        {
          count = (uint8_t)v16[2];
          n251_1 = (uint16_t *)(v16 - 3);
          if ( count <= (uint8_t)*(v16 - 1) )
            break;
          v18 = *(uint16_t *)v16;
          v19 = v16[2];
          v20 = *(v16 - 1);
          *(uint16_t *)v16 = *n251_1;
          v16[2] = v20;
          *n251_1 = v18;
          *(v16 - 1) = v19;
          n251_2 = (uint16_t *)*((uint32_t *)_this + 5);
          v16 -= 3;
          if ( n251_1 == n251_2 )
            goto LABEL_16;
        }
      }
    }
    v21 = *((uint32_t *)_this + 4);
    if ( count > 251 || v21 < *((uint32_t *)_this + 3) )
    {
      v22 = *((uint32_t *)_this + 1);
      v36 = v21 < 20 * _this->f0;
      v23 = (char *)n251_2 - 3;
      do
      {
        v24 = v23;
        v23 += 3;
        v25 = (v36 + (uint32_t)(uint8_t)v23[2]) >> 1;
        v23[2] = v25;
        if ( v23 != (char *)*((uint32_t *)_this + 5) )
        {
          v26 = v23 - 3;
          v35 = (uint8_t)*(v23 - 1);
          if ( v25 > v35 )
          {
            v34 = *(uint16_t *)v23;
            *(uint16_t *)v23 = *(uint16_t *)v26;
            v23[2] = v35;
            if ( v26 != (char *)*((uint32_t *)_this + 5) )
            {
              v33 = v23;
              v32 = v22;
              do
              {
                v27 = v26 - 3;
                v28 = (uint8_t)*(v26 - 1);
                if ( v25 <= v28 )
                  break;
                *(uint16_t *)v26 = *(uint16_t *)v27;
                v26[2] = v28;
                v26 -= 3;
              }
              while ( v27 != (char *)*((uint32_t *)_this + 5) );
              v23 = v33;
              v22 = v32;
            }
            *(uint16_t *)v26 = v34;
            v26[2] = v25;
          }
        }
        --v22;
      }
      while ( v22 );
      v29 = *((uint32_t *)_this + 2);
      if ( !v23[2] )
      {
        do
        {
          ++v22;
          *((uint32_t *)_this + 2) = ++v29;
          v30 = (uint8_t)v24[2];
          v24 -= 3;
        }
        while ( !v30 );
        *((uint32_t *)_this + 1) -= v22;
      }
      v31 = *((uint32_t *)_this + 3);
      *((uint32_t *)_this + 2) = v29 - (v29 >> 1);
      *((uint32_t *)_this + 3) = v31 - (v31 >> 1);
      return;
    }
  }
  else
  {
LABEL_4:
    v7 = v4 == _this->f0;
    if ( v4 >= _this->f0 )
    {
      if ( a3 <= 1 )
        return;
      v7 = v4 == _this->f0;
    }
    if ( v7 )
    {
      *((uint32_t *)_this + 1) = --v4;
      v8 = *(uint8_t *)(3 * v4 + list + 2);
    }
    else
    {
      v8 = 1;
    }
    v9 = *((uint32_t *)_this + 2);
    list += 3 * v4;
    *((uint32_t *)_this + 1) = v4 + 1;
    *((uint32_t *)_this + 2) = v8 + v9 + 1;
    *(uint8_t *)(list + 2) = 2;
    *(uint16_t *)list = a2;
    *((uint32_t *)_this + 3) += 4;
    if ( list != (char *)*((uint32_t *)_this + 5) )
    {
      v10 = *(uint16_t *)list;
      v11 = *(uint8_t *)(list + 2);
      v12 = *(uint8_t *)(list - 1);
      *(uint16_t *)list = *(uint16_t *)(list - 3);
      *(uint8_t *)(list + 2) = v12;
      *(uint16_t *)(list - 3) = v10;
      *(uint8_t *)(list - 1) = v11;
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
  char *v25;   // were int32_t: these hold addresses
  bool v47;
  int16_t v24, v42;
  int32_t n4, v8, n0x800000_6, n0x7F800000_6, n0x800000_5, v27, v28, v46, v50,
          v56, v59, i, v65, v66, n0x7F800000_5, n0x7F800000_7;
  uint16_t *v3, *v26, n0x4000, v39, v41, *v51, *this_2;
  uint32_t n4_2, v38, v40, v43, v44, v45, v48, v52, n4_1, n0x800000_3;
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
    v47 = v46 <= 4 * __n8_1;
    n0x7F800000_6 = n0x7F800000_7;
    if ( v47 )
    {
      n0x800000_5 = 4 * (v50 > __n8_1);
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
    v66 = (uint8_t)level_geom[n4_1].half;
    v56 = n2 - (uint8_t)level_geom[n4_1].first;
    v59 = 0;
    n0x800000_3 = n0x800000_6;
    this_2 = _this;
    for ( i = 1; ; i *= 2 )
    {
      v25 = (char *)&this_2[2 * (uint8_t)level_geom[n4_1].tbl_base + 8];
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
      v26[v27] = __n8_0 + n0x4000;
      v66 >>= 1;
      v59 = v27 + 2 * v59;
      if ( !v66 )
        return n0x800000_5;
    }
  }
  return n0x800000_5;
}
static inline int32_t __fwd_alt_p1_encode_symbol_encode_symbol_tree(void *a0, int32_t a1) { return __encode_symbol_tree((uint16_t *)a0, a1); }

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
    return __fwd_alt_p1_encode_symbol_encode_symbol_tree(
             model_tables
           + 32512 * (n5a_2 & 1)
           + 254 * (((a1[1] + (result & 0x7FFF) + 96 - 2 * (uint32_t)a1[n5a_2 + 1]) >> 25) & 0xFFFFFFC0)
           + 254 * a3,
             (n5a - 5) >> 1);
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
    if ( v36 <= 4 * __n8_1 )
      v16 = v36 - 4 * (v36 > __n8_1);
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
    v38 = (uint8_t)level_geom[n2].half;
    v40 = 0;
    v43 = 1;
    do
    {
      v17 = _this + 2 * (uint8_t)level_geom[n2].tbl_base + 2 * v43 + 2 * v40 + 8;
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
      v17[v23] = __n8_0 + n0x4000;
      v38 >>= 1;
      v43 *= 2;
      v25 = v23 + 2 * v40;
      v40 = v25;
    }
    while ( v38 );
    v26 = (uint8_t)level_geom[n2].first;
    return v25 + v26;
  }
}
static inline int32_t __fwd_alt_p1_decode_symbol_decode_symbol_tree(void *a0) { return __decode_symbol_tree((uint16_t *)a0); }

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
        * __fwd_alt_p1_decode_symbol_decode_symbol_tree(
            model_tables
          + 32512 * (n5 & 1)
          + 254
          * ((((uint16_t)a1[1] + (v16 & 0x7FFF) + 96 - 2 * (uint32_t)(uint16_t)a1[n5 + 1]) >> 25)
           & 0xFFFFFFC0)
          + 254 * a3);
  return n5;
}
static inline int32_t __fwd_alt_p2_encode_symbol_encode_symbol_tree(void *a0, int32_t a1) { return __encode_symbol_tree((uint16_t *)a0, a1); }

int32_t __alt_p2_encode_symbol(Obj7 *_this, char *a2, int32_t a3)
{
  ;
  int16_t v18;
  Obj113 *result;
  int32_t v3, n32;
  Obj113 *v25;
  uint16_t *v5;
  uint32_t tot, tot_1,
           v21, v22, v23;
  v3 = *((uint16_t *)_this + 2) + *((uint16_t *)_this + 1);
  tot = v3 + *((uint16_t *)_this + 3);
  if ( a3 )
  {
    if ( (a3 & 1) != 0 )
    {
      v3 = *((uint16_t *)_this + 1);
      v5 = (uint16_t *)_this + 2;
      v25 = (Obj113 *)((uint16_t *)_this + 2);
    }
    else
    {
      v5 = (uint16_t *)_this + 3;
      v25 = (Obj113 *)((uint16_t *)_this + 3);
    }
  }
  else
  {
    v5 = (uint16_t *)_this + 1;
    v3 = 0;
    v25 = (Obj113 *)((uint16_t *)_this + 1);
  }
  tot_1 = v3 + *v5;
  rc.encode(v3, tot_1, tot);
  v18 = v25->f0;
  if ( v25->f0 > 0x4000u )
  {
    v21 = *((uint16_t *)_this + 2);
    v22 = *((uint16_t *)_this + 3);
    *((uint16_t *)_this + 1) -= *((uint16_t *)_this + 1) >> 1;
    n32 = _this->f0;
    *((uint16_t *)_this + 2) = v21 - (v21 >> 1);
    *((uint16_t *)_this + 3) = v22 - (v22 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        v23 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(v23) = 32;
      LOWORD(n32) = n32 - v23;
      _this->f0 = n32;
      v18 = v25->f0;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      _this->f0 = n32;
      v18 = v25->f0;
    }
  }
  else
  {
    LOWORD(n32) = _this->f0;
  }
  result = (Obj113 *)((int32_t)v25);
  v25->f0 = n32 + v18;
  if ( a3 > 0 )
    return __fwd_alt_p2_encode_symbol_encode_symbol_tree(model_tables + 254 * *(uint32_t *)(a2 + 4 * (a3 & 1)), (a3 - 1) >> 1);
  return (int32_t)result;
}
static inline int32_t __fwd_alt_p2_decode_symbol_decode_symbol_tree(void *a0) { return __decode_symbol_tree((uint16_t *)a0); }

// alt_p2_encode_symbol's counterpart, and called on the same two objects from
// the p2 decoders that the encoders hand to it.  It was `decode_three_way` for
// the shape of its first step -- a three-way choice over the counts at
// `_this[1..3]` -- but so is the encoder's first step, and a pair that codes
// the same thing should read as one.  The three-way part is still the first
// twenty lines; the name now says which half of the pair this is.
int32_t __alt_p2_decode_symbol(Obj13 *_this, char *a2)
{
  ;
  int32_t v7, v8, v10, n0x4000, n32, v16, v21, v23;
  uint16_t *v9, *v24;
  uint32_t v11, v18, v19, v20;
  v23 = *((uint16_t *)_this + 3);
  v21 = *((uint16_t *)_this + 2) + *((uint16_t *)_this + 1);
  v20 = v23 + v21;
  v7 = rc.get_freq(v20);
  v8 = *((uint16_t *)_this + 1);
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
    v18 = *((uint16_t *)_this + 2);
    v19 = *((uint16_t *)_this + 3);
    *((uint16_t *)_this + 1) -= *((uint16_t *)_this + 1) >> 1;
    n32 = _this->f0;
    *((uint16_t *)_this + 2) = v18 - (v18 >> 1);
    *((uint16_t *)_this + 3) = v19 - (v19 >> 1);
    if ( n32 <= 256 )
    {
      if ( n32 <= 32 )
        n0x4000 = ((uint32_t)(16 - n32) >> 30) & 0xFFFFFFFE;
      else
        LOWORD(n0x4000) = 32;
      LOWORD(n32) = n32 - n0x4000;
      _this->f0 = n32;
      LOWORD(n0x4000) = *v9;
    }
    else
    {
      n32 = (uint32_t)n32 >> 1;
      _this->f0 = n32;
      LOWORD(n0x4000) = *v9;
    }
  }
  else
  {
    LOWORD(n32) = _this->f0;
  }
  *v9 = n32 + n0x4000;
  v16 = v9 - v24;
  if ( v16 )
    return v16 + 2 * __fwd_alt_p2_decode_symbol_decode_symbol_tree(model_tables + 254 * *(uint32_t *)(a2 + 4 * (v16 & 1)));
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

void **__free_workspace(ModelBlock *Blocka, char a2)
{
  ;
  int32_t v4, v6, v10, v13, i;
  uint32_t *v3, *v7, *v9, *v12;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_1;
  ModelBlock *Blocka_2;
  void **v8, **v14;
  Blocka_1 = (ModelBlock *)(Blocka);
  free(*((void **)Blocka + 269560));
  free(*(void**)&Blocka_1->f1078236);
  free(Blocka_1->f1078684);
  free(*(void**)&Blocka_1->f1078688);
  v3 = (uint32_t *)Blocka_1->f1078208;
  if ( v3 )
  {
    v4 = *(v3 - 1);
    if ( v4 )
    {
      Blocka_2 = (ModelBlock *)(Blocka_1);
      v6 = *(v3 - 1);
      v7 = (uint32_t *)Blocka_1->f1078208;
      v8 = (void **)&v3[6 * v4];
      do
      {
        v8 -= 6;
        free(v8[5]);
        --v6;
      }
      while ( v6 );
      v3 = v7;
      Blocka_1 = (ModelBlock *)(Blocka_2);
    }
    free(v3 - 1);
  }
  v9 = (uint32_t *)Blocka_1->f1078212;
  if ( v9 )
  {
    v10 = *(v9 - 1);
    if ( v10 )
    {
      Blocka_3 = (ModelBlock *)(Blocka_1);
      v12 = (uint32_t *)Blocka_1->f1078212;
      v13 = *(v9 - 1);
      v14 = (void **)&v9[6 * v10];
      do
      {
        v14 -= 6;
        free(v14[5]);
        --v13;
      }
      while ( v13 );
      v9 = v12;
      Blocka_1 = (ModelBlock *)(Blocka_3);
    }
    free(v9 - 1);
  }
  for ( i = 0; i < 5; ++i )
    free(((void**)Blocka_1)[i + 14]);
  free(Blocka_1->f1078204);
  if ( (a2 & 1) != 0 )
    free(Blocka_1);
  return (void **)Blocka_1;
}

int32_t __pixel_context(ModelBlock *_this, uint32_t *p_n15)
{
  ;
  char *v7, *v16;   // was int32_t: these hold addresses
  bool v5;
  int32_t result, v3, v4, v6, v9, v13, v14, n6;
  uint16_t *v8;
  uint16_t *v11;
  Obj9 *v12;
  Obj53 *v10;
  n6 = *((int32_t *)_this + 11);
  result = ((uint32_t *)p_n15)[n6];
  if ( exclusion_mask[result] == __byte_445700 )
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
  *((int32_t *)_this + 13) = v6;
  if ( v5 && n6 > 6 )
    return -1;
  v7 = (char *)*((int32_t *)_this + 269553);
  v16 = v7;
  v8 = (uint16_t *)(*(uint16_t **)(v7 + 24 * tbl44573C[1] + 20));
  v9 = v6
     + 8
     * (result == *(uint16_t *)((char *)v8 + 27)
     || result == v8[12]
     || result == *(uint16_t *)((char *)v8 + 21)
     || result == v8[9]
     || result == *(uint16_t *)((char *)v8 + 15)
     || result == v8[6]
     || result == *(uint16_t *)((char *)v8 + 9)
     || result == v8[3]
     || result == *(uint16_t *)((char *)v8 + 3)
     || result == v8[0]);
  *((int32_t *)_this + 13) = v9;
  v10 = (Obj53 *)(*(uint16_t **)(v7 + 24 * tbl44573C[2] + 20));
  v11 = (uint16_t *)(*(uint16_t **)(v7 + 24 * result + 20));
  v12 = (Obj9 *)(*(uint16_t **)(v16 + 24 * tbl44573C[3] + 20));
  v13 = (result == *(uint16_t *)((char *)v12 + 15)
      || result == v12->f12
      || result == *(uint16_t *)((char *)v12 + 9)
      || result == v12->f6
      || result == *(uint16_t *)((char *)v12 + 3)
      || result == v12->f0)
      + 2
      * (tbl44573C[2] == *(uint16_t *)((char *)v11 + 27)
      || tbl44573C[2] == v11[12]
      || tbl44573C[2] == *(uint16_t *)((char *)v11 + 21)
      || tbl44573C[2] == v11[9]
      || tbl44573C[2] == *(uint16_t *)((char *)v11 + 15)
      || tbl44573C[2] == v11[6]
      || tbl44573C[2] == *(uint16_t *)((char *)v11 + 9)
      || tbl44573C[2] == v11[3]
      || tbl44573C[2] == *(uint16_t *)((char *)v11 + 3)
      || tbl44573C[2] == v11[0])
      + 4
      * (result == *(uint16_t *)((char *)v10 + 9)
      || result == v10->f6
      || result == *(uint16_t *)((char *)v10 + 3)
      || result == v10->f0);
  v14 = v13 + v9;
  if ( n6 <= 14 || (v14 & 0xB) != 0 )
  {
    *((int32_t *)_this + 13) = (n6 << 7) + v14;
    *((int32_t *)_this + 12) = v13 + 8 * (n6 > 9);
  }
  else
  {
    *((int32_t *)_this + 12) = v13;
    return -1;
  }
  return result;
}
static inline void __fwd_init_model_tables_symbol_list_update(void *a0, int32_t a1, uint32_t a2) { __symbol_list_update((Obj6 *)a0, a1, a2); }

int32_t __init_model_tables(ModelBlock *_this)
{
  ;
  char *v8, *v32;   // were int32_t: these hold addresses
  uint8_t *v28, *v29;           // row cursors out of f56
  char v13, v14, *buf;
  Obj14 *v11;
  int16_t v6, v12;
  uint32_t **v3;
  uint8_t *v30;
  int32_t n2_1, v7, v9, v10, v15, v17, n2, v19, v21, v22, v23, v24, v25, v26, v27,
          result;
  uint16_t *v20;
  Obj45 *v5;
  uint32_t **v4;
  n2_1 = _this->f32;
  if ( !n2_1 )
  {
    if ( (uint32_t **)((char *)_this + 1078216) == _this->f1078232 )
    {
      if ( _this->f1078216 )
      {
        __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * tbl44573C[1]), **(uint16_t **)&_this->f56[5], 3u);
        __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * **(uint16_t **)&_this->f56[5]), tbl44573C[2], 2u);
        __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078208 + 24 * tbl44573C[1]), **(uint16_t **)&_this->f56[5], 4u);
        __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078208 + 24 * **(uint16_t **)&_this->f56[5]), tbl44573C[1], 2u);
      }
      else
      {
        __fwd_init_model_tables_symbol_list_update(
          (uint32_t *)(_this->f1078212 + 24 * tbl44573C[2]),
          **(uint16_t **)&_this->f56[5],
          (_this->f44 > 3) + 2);
      }
    }
    else
    {
      __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * tbl44573C[1]), **(uint16_t **)&_this->f56[5], 3u);
      __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * **(uint16_t **)&_this->f56[5]), tbl44573C[2], 2u);
      __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * **(uint16_t **)&_this->f56[5]), tbl44573C[1], 1u);
      __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078208 + 24 * **(uint16_t **)&_this->f56[5]), tbl44573C[1], 2u);
      v3 = _this->f1078232;
      do
      {
        v4 = v3 - 1;
        _this->f1078232 = v4;
        v5 = (Obj45 *)(*v4);
        v6 = **(uint16_t **)&_this->f56[5];
        v7 = v5->f0[1];
        v32 = (char *)(int32_t)v5->f20;
        if ( v7 == v5->f0[0] )
        {
          v8 = (char *)(int32_t)v5->f20;
          v5->f0[1] = --v7;
          v9 = *(uint8_t *)(v8 + 3 * v7 + 2);
        }
        else
        {
          v9 = 1;
        }
        v10 = v5->f0[2];
        v11 = (Obj14 *)((int16_t *)(v32 + 3 * v7));
        v5->f0[1] = v7 + 1;
        v5->f0[2] = v9 + v10 + 1;
        v11->f0 = v6;
        *((uint8_t *)v11 + 2) = 2;
        v5->f0[3] += 4;
        if ((int16_t *)v11 != (int16_t *)v5->f20 )
        {
          v12 = v11->f0;
          v13 = *((uint8_t *)v11 + 2);
          v14 = *((uint8_t *)v11 - 1);
          v11->f0 = *(int16_t *)((char *)v11 - 3);
          *((uint8_t *)v11 + 2) = v14;
          *(int16_t *)((char *)v11 - 3) = v12;
          *((uint8_t *)v11 - 1) = v13;
        }
        v3 = _this->f1078232;
      }
      while ( v3 != (uint32_t **)((char *)_this + 1078216) );
    }
    if ( __byte_445700 == -1 )
    {
      v15 = _this->f16;
      __byte_445700 = 1;
      buf = exclusion_mask;
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
      ++__byte_445700;
      n2 = _this->f32;
    }
LABEL_19:
    if ( n2 && n2 <= 2 )
      goto LABEL_37;
    goto LABEL_21;
  }
  if ( n2_1 <= 2 )
    goto LABEL_37;
  if ( tbl44573C[3] != tbl44573C[4] )
  {
    __fwd_init_model_tables_symbol_list_update((uint32_t *)(_this->f1078212 + 24 * tbl44573C[2]), **(uint16_t **)&_this->f56[5], 1u);
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
  *(_this->f56[5] + 2) = **(uint16_t **)&_this->f56[5] == **(uint16_t **)&_this->f56[6];
  *(_this->f56[5] + 3) = **(uint16_t **)&_this->f56[5] == *(uint16_t *)(_this->f56[5] - 8);
  *(_this->f56[5] + 4) = **(uint16_t **)&_this->f56[5] == *(uint16_t *)(_this->f56[6] + 8);
  *(_this->f56[5] + 5) = **(uint16_t **)&_this->f56[5] == *(uint16_t *)(_this->f56[6] - 8);
  *(_this->f56[5] + 6) = **(uint16_t **)&_this->f56[5] == *(uint16_t *)(_this->f56[6] + 16);
  *(_this->f56[5] + 7) = **(uint16_t **)&_this->f56[5] == *(uint16_t *)(_this->f56[6] + 24);
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

void **__alt_p2_free(void **lpAddress, char a2)
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


void **__alt_p1_free(void **Block, char a2)
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
int32_t __alt_p1_context(Obj25 *_this, uint32_t *a2, Obj91 *a3)
{
  ;
  Obj22 *v5;
  Obj17 *v32;
  bool v9, v19, v20;
  Obj22 *v34;
  int32_t v6, v7, v11, v12, v13, v14, v15, v16, v18, v21, v22, v23, v24, v25, v26, v27, v28,
          v29, result, v35, v36, v39, v40, v41, v42;
  uint32_t v37, v38;
  uint8_t *v3, *v4, *v8, *v10, *v17, *v31, *v33;
  v3 = *((uint8_t **)_this + 50);
  v4 = (uint8_t *)*v3;
  v5 = (Obj22 *)((int32_t)*((uint8_t **)_this + 49));
  v6 = *(uint8_t *)((char *)v5 - 2);
  v7 = *(v3 - 2);
  if ( v6 < (int32_t)v4 )
  {
    if ( v7 >= v6 )
    {
      v10 = &v4[v6 - v7];
      v9 = v7 < (int32_t)v4;
      v4 = (uint8_t *)*(uint8_t *)((char *)v5 - 2);
      if ( v9 )
        v4 = v10;
    }
  }
  else if ( v7 <= v6 )
  {
    v8 = &v4[v6 - v7];
    v9 = v7 <= (int32_t)v4;
    v4 = (uint8_t *)*(uint8_t *)((char *)v5 - 2);
    if ( !v9 )
      v4 = v8;
  }
  *((uint8_t **)_this + 2) = v4;
  v33 = *((uint8_t **)_this + 51);
  v11 = *(v3 - 1)
      + *(uint8_t *)((char *)v5 - 5)
      + 3 * (v3[3] + v33[1])
      + 6 * *(uint8_t *)((char *)v5 - 1)
      + 4 * (v3[1] + *(uint8_t *)((char *)v5 - 3))
      + 2 * (v33[5] + v3[5] + *(uint8_t *)((char *)v5 - 7));
  if ( a2 )
  {
    if ( a3 )
    {
      v34 = (Obj22 *)(v5);
      v12 = a2[49];
      v36 = v11 + 2 * (*(uint8_t *)(a3->f196 - 1) + *(uint8_t *)(v12 - 1));
      *((uint8_t **)_this + 28) = (uint8_t *)(*v3
                                       - (uint32_t)v4
                                       + *(uint8_t *)(v12 - 2)
                                       - *(uint8_t *)(a2[50] - 2));
      v13 = a2[49];
      v14 = *(uint8_t *)(v13 - 2);
      v15 = *(uint8_t *)(v13 - 4);
      v5 = (Obj22 *)(v34);
      *((uint8_t **)_this + 32) = (uint8_t *)(*(uint8_t *)((uintptr_t)v34 - 2) - (uint32_t)v4 + v14 - v15);
      *((uint8_t **)_this + 36) = (uint8_t *)(*(uint8_t *)((uintptr_t)v34 - 2)
                                       - (uint32_t)v4
                                       + *(uint8_t *)(a3->f196 - 2)
                                       - *(uint8_t *)(a3->f196 - 4));
      *((uint8_t **)_this + 40) = (uint8_t *)(*(uint8_t *)(a3->f196 - 2) - a3->f8);
      *((uint8_t **)_this + 20) = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = (*(uint8_t *)(a3->f196 - 1) + (uint32_t)*(uint8_t *)(a2[49] - 1) - 16) >> 31;
    }
    else
    {
      v36 = v3[7] + v11 + 3 * *(uint8_t *)(a2[49] - 1);
      *((uint8_t **)_this + 28) = (uint8_t *)(2 * *(uint8_t *)((char *)v5 - 2) - *(uint8_t *)((char *)v5 - 4) - (uint32_t)v4);
      *((uint8_t **)_this + 32) = (uint8_t *)(2 * *(uint8_t *)((char *)v5 - 2) - *(uint8_t *)((char *)v5 - 4) - (uint32_t)v4);
      *((uint8_t **)_this + 36) = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + *(uint8_t *)((char *)v5 - 2));
      *((uint8_t **)_this + 40) = (uint8_t *)(*(uint8_t *)((char *)v5 - 2)
                                       - (uint32_t)v4
                                       + *(uint8_t *)(a2[49] - 2)
                                       - *(uint8_t *)(a2[49] - 4));
      *((uint8_t **)_this + 20) = (uint8_t *)(*(uint8_t *)(a2[49] - 2) - a2[2]);
      v37 = ((uint32_t)*(uint8_t *)(a2[49] - 1) - 8) >> 31;
    }
  }
  else
  {
    v32 = (Obj17 *)((int32_t)*((uint8_t **)_this + 53));
    v36 = v32->f1 + *(v33 - 3) + v3[7] + v11 + v32->f5;
    *((uint8_t **)_this + 28) = (uint8_t *)(2 * *(uint8_t *)((char *)v5 - 2) - *(uint8_t *)((char *)v5 - 4) - (uint32_t)v4);
    *((uint8_t **)_this + 32) = (uint8_t *)(2 * *v3 - *v33 - (uint32_t)v4);
    *((uint8_t **)_this + 36) = (uint8_t *)(-(int32_t)v4 - *v3 + v3[2] + *(uint8_t *)((char *)v5 - 2));
    *((uint8_t **)_this + 40) = (uint8_t *)(-3 * (*(uint8_t *)((char *)v5 - 4) - *(uint8_t *)((char *)v5 - 2))
                                     + *(uint8_t *)((char *)v5 - 6)
                                     - (uint32_t)v4);
    *((uint8_t **)_this + 20) = (uint8_t *)(v3[4] - (uint32_t)v4);
    v37 = v5->f1 + v32->f1 + (*((uint8_t **)_this + 52))[1] + v33[1] + v3[1] == 0;
  }
  v16 = (v36 + 7) >> 4;
  v17 = (uint8_t *)*((uint8_t *)_this + v16 + 216);
  v31 = v17;
  v35 = __dword_439BD8[(uint32_t)v17];
  *((uint8_t **)_this + 3) = v17;
  *((uint8_t **)_this + 4) = &(*((uint8_t **)_this + v16 + 438))[*((uint8_t *)_this + (uint32_t)v4 + 728)];
  *((uint8_t **)_this + 8) = (uint8_t *)(((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31));
  v38 = ((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31);
  v18 = (*(v3 - 2) - *v3 >= 0) + (*(v3 - 2) > (int32_t)*v3);
  *((uint8_t **)_this + 12) = (uint8_t *)v18;
  v39 = (*(v3 - 2) - *(uint8_t *)((char *)v5 - 2) >= 0) + (*(v3 - 2) > (int32_t)*(uint8_t *)((char *)v5 - 2));
  *((uint8_t **)_this + 16) = (uint8_t *)v39;
  v19 = *((uint8_t **)_this + 20) == nullptr;
  v20 = (int32_t)*((uint8_t **)_this + 20) < 0;
  v40 = (v3[2] - (int32_t)v4 >= -v35) + (v3[2] - (int32_t)v4 > v35);
  *((uint8_t **)_this + 24) = (uint8_t *)v40;
  v21 = v35 < (int32_t)*((uint8_t **)_this + 28);
  v22 = -v35 <= (int32_t)*((uint8_t **)_this + 28);
  v23 = !v20 + (!v20 && !v19);
  *((uint8_t **)_this + 20) = (uint8_t *)v23;
  v24 = v22 + v21;
  v19 = *((uint8_t **)_this + 32) == nullptr;
  v20 = (int32_t)*((uint8_t **)_this + 32) < 0;
  *((uint8_t **)_this + 28) = (uint8_t *)v24;
  v25 = !v20 && !v19;
  v26 = !v20;
  v19 = *((uint8_t **)_this + 36) == nullptr;
  v20 = (int32_t)*((uint8_t **)_this + 36) < 0;
  v41 = v26 + v25;
  *((uint8_t **)_this + 32) = (uint8_t *)(v26 + v25);
  v27 = !v20 && !v19;
  v28 = !v20;
  v19 = *((uint8_t **)_this + 40) == nullptr;
  v20 = (int32_t)*((uint8_t **)_this + 40) < 0;
  v42 = v28 + v27;
  *((uint8_t **)_this + 36) = (uint8_t *)(v28 + v27);
  v29 = !v20 + (!v20 && !v19);
  *((uint8_t **)_this + 40) = (uint8_t *)v29;
  result = (int32_t)&(*((uint8_t **)_this + v29 + 41))[16 * v37
                                    + 8 * (*((uint8_t **)_this + (uint32_t)*((uint8_t **)_this + 5) + 6) == nullptr)
                                    + (uint32_t)&(*((uint8_t **)_this + v41 + 33))[(uint32_t)*((uint8_t **)_this + v42 + 37)
                                                                        + (uint32_t)&(*((uint8_t **)_this + v40 + 25))[(uint32_t)*((uint8_t **)_this + v24 + 29)]
                                                                        + (uint32_t)&(*((uint8_t **)_this + v39 + 17))[(uint32_t)*((uint8_t **)_this + v23 + 21) + (uint32_t)&(*((uint8_t **)_this + v38 + 9))[(uint32_t)*((uint8_t **)_this + v18 + 13)]]]
                                    + (uint32_t)v31];
  *((uint8_t **)_this + 3) = (uint8_t *)result;
  return result;
}

int32_t __update_binary_pair(Obj20 *_this, int32_t symbol)
{
  ;
  char *v12;   // was int32_t: these hold addresses
  int32_t n0x8000, v3, v5, v6, v7, v11, v13;
  uint16_t *v8;
  uint32_t v4, n0x2000, v10;
  n0x8000 = _this->f0;
  if ( (uint32_t)n0x8000 <= 0x8000 )
  {
    v3 = *((uint8_t *)model_geometry + symbol);
    v4 = (*((uint16_t *)_this + 1) >> 2) & 0xFFFFFFE0;
    if ( ::plane_predictor == 2 )
      v4 = 15 * (*((uint16_t *)_this + 1) >> 5);
    *((uint16_t *)_this + v3 + 2) += v4 + 4;
    n0x8000 = _this->f0 + v4 + 4;
    _this->f0 = n0x8000;
    if ( symbol >= 2 )
    {
      n0x8000 = (uint8_t)level_geom[v3].half;
      v11 = symbol - (uint8_t)level_geom[v3].first;
      v5 = (int32_t)((uint16_t *)_this + 2 * (uint8_t)level_geom[v3].tbl_base + 8);
      v6 = 0;
      v12 = (char *)v5;
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
        v8[v13] = n0x2000 + ((__n8_0 * ((uint32_t)(::plane_predictor == 2) + 5)) >> 3);
      }
      while ( n0x8000 );
    }
  }
  return n0x8000;
}
static inline int32_t __fwd_alt_p1_model_update_binary_pair(void *a0, int32_t a1) { return __update_binary_pair((Obj20 *)a0, a1); }

int32_t __alt_p1_model(Obj0 *_this)
{
  ;
  uintptr_t result;   // were int32_t: addresses, masked and tagged
  Obj26 *v111;
  Obj26 *v9;
  Obj27 *v11;
  Obj28 *v13;
  char *v17, *v23, *v31, *v39, *v47, *v55, *v63, *v71, *v79;
  int16_t v12, v15;
  int32_t v3, n5_1, v5, n5_2, n5_3, v8, v14, v16, v18, v21, v22, v24, v25, v26, v29, v30, v32,
          v33, v34, v37, v38, v40, v41, v42, v45, v46, v48, v49, v50, v53, v54, v56, v57, v58,
          v61, v62, v64, v65, v66, v69, v70, v72, v73, v74, v77, v78, v80, v82, v83, v85, v86,
          v89, v92, v95, v98, v101, v104, v107, n2, n5_4;
  Obj26 *v108;
  Obj26 *v109;
  Obj26 *v19;
  Obj26 *v20;
  Obj26 *v105;
  Obj26 *v106;
  Obj26 *v27;
  Obj26 *v28;
  Obj26 *v102;
  Obj26 *v103;
  Obj26 *v35;
  Obj26 *v36;
  Obj26 *v99;
  Obj26 *v100;
  Obj26 *v43;
  Obj26 *v44;
  Obj26 *v96;
  Obj26 *v97;
  Obj26 *v51;
  Obj26 *v52;
  Obj26 *v93;
  Obj26 *v94;
  Obj26 *v59;
  Obj26 *v60;
  Obj26 *v90;
  Obj26 *v91;
  Obj26 *v67;
  Obj26 *v68;
  Obj26 *v87;
  Obj26 *v88;
  Obj26 *v75;
  Obj26 *v76;
  Obj26 *v84;
  Obj26 *v81;
  uint32_t n5, v110, v112;
  n5 = *(uint8_t *)((uint8_t)(**(uint8_t **)&_this->f12[46] - *(uint8_t *)&_this->f8) + (char *)_this + 984);
  v3 = _this->f12[9];
  n5_1 = *(uint8_t *)((uint8_t)(*(uint8_t *)&_this->f8 - **(uint8_t **)&_this->f12[46]) + (char *)_this + 984);
  v5 = _this->f12[13];
  n2 = (int32_t)(n5 - 5) >> 1;
  n5_2 = 6 - (n5 & 1);
  if ( n5 < 5 )
    n5_2 = *(uint8_t *)((uint8_t)(**(uint8_t **)&_this->f12[46] - *(uint8_t *)&_this->f8) + (char *)_this + 984);
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
  v9 = (Obj26 *)((char *)_this + 16 * v8);
  v111 = (Obj26 *)(v9);
  *(uint16_t *)((char *)v9 + (2 * n5_3 + 3802)) += 17;
  v9->f3800 += 17;
  result = _this->f12[0];
  if ( (result & 7) != 7 )
  {
    v11 = (Obj27 *)((char *)_this + 16 * result);
    v110 = (((_this->f12[1] & 7u) - 7) >> 31) + _this->f12[1];
    *(uint16_t *)((char *)v11 + (2 * n5_2 + 3818)) += 11;
    v12 = v11->f3816 + 11;
    v11->f3816 = v12;
    if ( n5_2 >= 5 )
      __fwd_alt_p1_model_update_binary_pair(
        model_tables
      + 32512 * (n5_2 & 1)
      + 254 * v110
      + 254
      * ((((v12 & 0x7FFF)
         + v11->f3818
         - 2 * (uint32_t)*(uint16_t *)((char *)v11 + (2 * n5_2 + 3818))) >> 25)
       & 0xFFFFFFC0),
        (int32_t)(n5 - 5) >> 1);
    result = _this->f12[0];
  }
  if ( (result & 7) != 0 )
  {
    v13 = (Obj28 *)((char *)_this + 16 * result);
    v14 = _this->f12[1] - ((_this->f12[1] & 7) != 0);
    *(uint16_t *)((char *)v13 + (2 * n5_2 + 3786)) += 13;
    v15 = v13->f3784 + 13;
    v13->f3784 = v15;
    if ( n5_2 >= 5 )
      __fwd_alt_p1_model_update_binary_pair(
        model_tables
      + 32512 * (n5_2 & 1)
      + 254 * v14
      + 254
      * ((((v15 & 0x7FFF)
         + v13->f3786
         - 2 * (uint32_t)*(uint16_t *)((char *)v13 + (2 * n5_2 + 3786))) >> 25)
       & 0xFFFFFFC0),
        n2);
    result = _this->f12[0];
  }
  if ( *(uint16_t *)((char *)_this + (16 * result + 3800)) < 0xCCCu )
  {
    if ( (result & 7u) < 7 )
    {
      *(uint16_t *)((char *)v111 + (2 * n5_4 + 3818)) += 7;
      v111->f3816 += 7;
      result = _this->f12[0];
    }
    if ( (result & 7) != 0 )
    {
      *(uint16_t *)((char *)v111 + (2 * n5_4 + 3786)) += 5;
      v111->f3784 += 5;
      result = _this->f12[0];
    }
    if ( n5_2 >= 5 )
    {
      v112 = _this->f12[1]
           + (((*(uint16_t *)((char *)_this + (16 * result + 3802))
              + (*(uint16_t *)((char *)_this + (16 * result + 3800)) & 0x7FFF)
              - 2 * (uint32_t)*(uint16_t *)((char *)_this + (16 * result + 2 * n5_2 + 3802))) >> 25)
            & 0xFFFFFFC0)
           + ((n5_2 & 1) << 7);
      if ( (v112 & 0x38) >= 0x38
        || (__fwd_alt_p1_model_update_binary_pair(
              model_tables
            + 32512 * (n5_2 & 1)
            + 254 * _this->f12[1]
            + 254
            * (((*(uint16_t *)((char *)_this + (16 * result + 3802))
               + (*(uint16_t *)((char *)_this + (16 * result + 3800)) & 0x7FFF)
               - 2 * (uint32_t)*(uint16_t *)((char *)_this + (16 * result + 2 * n5_2 + 3802))) >> 25)
             & 0xFFFFFFC0)
            + 2032,
              n2),
            (v112 & 0x38) != 0) )
      {
        __fwd_alt_p1_model_update_binary_pair(model_tables + 254 * v112 - 2032, n2);
      }
      result = _this->f12[0];
    }
    v16 = _this->f12[5];
    if ( v16 == 1 )
    {
      v107 = result - _this->f12[7];
      v108 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v107 + _this->f12[6])));
      v109 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[8] + v107)));
      ((uint16_t *)v108)[n5_2 + 1901] += 6;
      v108->f3800 += 6;
      ((uint16_t *)v109)[n5_2 + 1901] += 6;
      v109->f3800 += 6;
      v21 = _this->f12[0];
      if ( (v21 & 7) != 7 )
      {
        ((uint16_t *)v108)[n5_2 + 1909] += 4;
        v108->f3816 += 4;
        ((uint16_t *)v109)[n5_2 + 1909] += 4;
        v109->f3816 += 4;
        v21 = _this->f12[0];
      }
      if ( (v21 & 7) != 0 )
      {
        ((uint16_t *)v108)[n5_2 + 1893] += 3;
        v108->f3784 += 3;
        ((uint16_t *)v109)[n5_2 + 1893] += 3;
        v109->f3784 += 3;
        v21 = _this->f12[0];
      }
    }
    else
    {
      v17 = (char *)_this + 16 * (_this->f12[8 - v16] + result - _this->f12[6 + v16]);
      *(uint16_t *)(v17 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v17 + 3800) += 7;
      v18 = v8 + _this->f12[6] - _this->f12[7];
      v19 = (Obj26 *)((uint16_t *)((char *)_this
                    + 16
                    * (_this->f12[7]
                     + _this->f12[0]
                     - _this->f12[6 + _this->f12[5]])));
      ((uint16_t *)v19)[n5_2 + 1901] += 6;
      v19->f3800 += 6;
      v20 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v18));
      ((uint16_t *)v20)[n5_4 + 1901] += 4;
      v20->f3800 += 4;
      v21 = _this->f12[0];
      if ( (v21 & 7) != 7 )
      {
        ((uint16_t *)v19)[n5_2 + 1909] += 4;
        v19->f3816 += 4;
        ((uint16_t *)v20)[n5_4 + 1909] += 2;
        v20->f3816 += 2;
        v21 = _this->f12[0];
      }
      if ( (v21 & 7) != 0 )
      {
        ((uint16_t *)v19)[n5_2 + 1893] += 3;
        v19->f3784 += 3;
        ((uint16_t *)v20)[n5_4 + 1893] += 2;
        v20->f3784 += 2;
        v21 = _this->f12[0];
      }
    }
    v22 = _this->f12[9];
    if ( v22 == 1 )
    {
      v104 = v21 - _this->f12[11];
      v105 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v104 + _this->f12[10])));
      v106 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[12] + v104)));
      ((uint16_t *)v105)[n5_2 + 1901] += 6;
      v105->f3800 += 6;
      ((uint16_t *)v106)[n5_2 + 1901] += 6;
      v106->f3800 += 6;
      v29 = _this->f12[0];
      if ( (v29 & 7) != 7 )
      {
        ((uint16_t *)v105)[n5_2 + 1909] += 4;
        v105->f3816 += 4;
        ((uint16_t *)v106)[n5_2 + 1909] += 4;
        v106->f3816 += 4;
        v29 = _this->f12[0];
      }
      if ( (v29 & 7) != 0 )
      {
        ((uint16_t *)v105)[n5_2 + 1893] += 3;
        v105->f3784 += 3;
        ((uint16_t *)v106)[n5_2 + 1893] += 3;
        v106->f3784 += 3;
        v29 = _this->f12[0];
      }
    }
    else
    {
      v23 = (char *)_this + 16 * (_this->f12[12 - v22] + v21 - _this->f12[10 + v22]);
      *(uint16_t *)(v23 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v23 + 3800) += 7;
      v24 = _this->f12[11];
      v25 = v24 + _this->f12[0] - _this->f12[10 + _this->f12[9]];
      v26 = v8 + v24 - _this->f12[12 - _this->f12[9]];
      v27 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v25));
      ((uint16_t *)v27)[n5_2 + 1901] += 6;
      v27->f3800 += 6;
      v28 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v26));
      ((uint16_t *)v28)[n5_4 + 1901] += 4;
      v28->f3800 += 4;
      v29 = _this->f12[0];
      if ( (v29 & 7) != 7 )
      {
        ((uint16_t *)v27)[n5_2 + 1909] += 4;
        v27->f3816 += 4;
        ((uint16_t *)v28)[n5_4 + 1909] += 2;
        v28->f3816 += 2;
        v29 = _this->f12[0];
      }
      if ( (v29 & 7) != 0 )
      {
        ((uint16_t *)v27)[n5_2 + 1893] += 3;
        v27->f3784 += 3;
        ((uint16_t *)v28)[n5_4 + 1893] += 2;
        v28->f3784 += 2;
        v29 = _this->f12[0];
      }
    }
    v30 = _this->f12[13];
    if ( v30 == 1 )
    {
      v101 = v29 - _this->f12[15];
      v102 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v101 + _this->f12[14])));
      v103 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[16] + v101)));
      ((uint16_t *)v102)[n5_2 + 1901] += 6;
      v102->f3800 += 6;
      ((uint16_t *)v103)[n5_2 + 1901] += 6;
      v103->f3800 += 6;
      v37 = _this->f12[0];
      if ( (v37 & 7) != 7 )
      {
        ((uint16_t *)v102)[n5_2 + 1909] += 4;
        v102->f3816 += 4;
        ((uint16_t *)v103)[n5_2 + 1909] += 4;
        v103->f3816 += 4;
        v37 = _this->f12[0];
      }
      if ( (v37 & 7) != 0 )
      {
        ((uint16_t *)v102)[n5_2 + 1893] += 3;
        v102->f3784 += 3;
        ((uint16_t *)v103)[n5_2 + 1893] += 3;
        v103->f3784 += 3;
        v37 = _this->f12[0];
      }
    }
    else
    {
      v31 = (char *)_this + 16 * (_this->f12[16 - v30] + v29 - _this->f12[14 + v30]);
      *(uint16_t *)(v31 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v31 + 3800) += 7;
      v32 = _this->f12[15];
      v33 = v32 + _this->f12[0] - _this->f12[14 + _this->f12[13]];
      v34 = v8 + v32 - _this->f12[16 - _this->f12[13]];
      v35 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v33));
      ((uint16_t *)v35)[n5_2 + 1901] += 6;
      v35->f3800 += 6;
      v36 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v34));
      ((uint16_t *)v36)[n5_4 + 1901] += 4;
      v36->f3800 += 4;
      v37 = _this->f12[0];
      if ( (v37 & 7) != 7 )
      {
        ((uint16_t *)v35)[n5_2 + 1909] += 4;
        v35->f3816 += 4;
        ((uint16_t *)v36)[n5_4 + 1909] += 2;
        v36->f3816 += 2;
        v37 = _this->f12[0];
      }
      if ( (v37 & 7) != 0 )
      {
        ((uint16_t *)v35)[n5_2 + 1893] += 3;
        v35->f3784 += 3;
        ((uint16_t *)v36)[n5_4 + 1893] += 2;
        v36->f3784 += 2;
        v37 = _this->f12[0];
      }
    }
    v38 = _this->f12[17];
    if ( v38 == 1 )
    {
      v98 = v37 - _this->f12[19];
      v99 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v98 + _this->f12[18])));
      v100 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[20] + v98)));
      ((uint16_t *)v99)[n5_2 + 1901] += 6;
      v99->f3800 += 6;
      ((uint16_t *)v100)[n5_2 + 1901] += 6;
      v100->f3800 += 6;
      v45 = _this->f12[0];
      if ( (v45 & 7) != 7 )
      {
        ((uint16_t *)v99)[n5_2 + 1909] += 4;
        v99->f3816 += 4;
        ((uint16_t *)v100)[n5_2 + 1909] += 4;
        v100->f3816 += 4;
        v45 = _this->f12[0];
      }
      if ( (v45 & 7) != 0 )
      {
        ((uint16_t *)v99)[n5_2 + 1893] += 3;
        v99->f3784 += 3;
        ((uint16_t *)v100)[n5_2 + 1893] += 3;
        v100->f3784 += 3;
        v45 = _this->f12[0];
      }
    }
    else
    {
      v39 = (char *)_this + 16 * (_this->f12[20 - v38] + v37 - _this->f12[18 + v38]);
      *(uint16_t *)(v39 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v39 + 3800) += 7;
      v40 = _this->f12[19];
      v41 = v40 + _this->f12[0] - _this->f12[18 + _this->f12[17]];
      v42 = v8 + v40 - _this->f12[20 - _this->f12[17]];
      v43 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v41));
      ((uint16_t *)v43)[n5_2 + 1901] += 6;
      v43->f3800 += 6;
      v44 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v42));
      ((uint16_t *)v44)[n5_4 + 1901] += 4;
      v44->f3800 += 4;
      v45 = _this->f12[0];
      if ( (v45 & 7) != 7 )
      {
        ((uint16_t *)v43)[n5_2 + 1909] += 4;
        v43->f3816 += 4;
        ((uint16_t *)v44)[n5_4 + 1909] += 2;
        v44->f3816 += 2;
        v45 = _this->f12[0];
      }
      if ( (v45 & 7) != 0 )
      {
        ((uint16_t *)v43)[n5_2 + 1893] += 3;
        v43->f3784 += 3;
        ((uint16_t *)v44)[n5_4 + 1893] += 2;
        v44->f3784 += 2;
        v45 = _this->f12[0];
      }
    }
    v46 = _this->f12[21];
    if ( v46 == 1 )
    {
      v95 = v45 - _this->f12[23];
      v96 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v95 + _this->f12[22])));
      v97 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[24] + v95)));
      ((uint16_t *)v96)[n5_2 + 1901] += 6;
      v96->f3800 += 6;
      ((uint16_t *)v97)[n5_2 + 1901] += 6;
      v97->f3800 += 6;
      v53 = _this->f12[0];
      if ( (v53 & 7) != 7 )
      {
        ((uint16_t *)v96)[n5_2 + 1909] += 4;
        v96->f3816 += 4;
        ((uint16_t *)v97)[n5_2 + 1909] += 4;
        v97->f3816 += 4;
        v53 = _this->f12[0];
      }
      if ( (v53 & 7) != 0 )
      {
        ((uint16_t *)v96)[n5_2 + 1893] += 3;
        v96->f3784 += 3;
        ((uint16_t *)v97)[n5_2 + 1893] += 3;
        v97->f3784 += 3;
        v53 = _this->f12[0];
      }
    }
    else
    {
      v47 = (char *)_this + 16 * (_this->f12[24 - v46] + v45 - _this->f12[22 + v46]);
      *(uint16_t *)(v47 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v47 + 3800) += 7;
      v48 = _this->f12[23];
      v49 = v48 + _this->f12[0] - _this->f12[22 + _this->f12[21]];
      v50 = v8 + v48 - _this->f12[24 - _this->f12[21]];
      v51 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v49));
      ((uint16_t *)v51)[n5_2 + 1901] += 6;
      v51->f3800 += 6;
      v52 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v50));
      ((uint16_t *)v52)[n5_4 + 1901] += 4;
      v52->f3800 += 4;
      v53 = _this->f12[0];
      if ( (v53 & 7) != 7 )
      {
        ((uint16_t *)v51)[n5_2 + 1909] += 4;
        v51->f3816 += 4;
        ((uint16_t *)v52)[n5_4 + 1909] += 2;
        v52->f3816 += 2;
        v53 = _this->f12[0];
      }
      if ( (v53 & 7) != 0 )
      {
        ((uint16_t *)v51)[n5_2 + 1893] += 3;
        v51->f3784 += 3;
        ((uint16_t *)v52)[n5_4 + 1893] += 2;
        v52->f3784 += 2;
        v53 = _this->f12[0];
      }
    }
    v54 = _this->f12[25];
    if ( v54 == 1 )
    {
      v92 = v53 - _this->f12[27];
      v93 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v92 + _this->f12[26])));
      v94 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[28] + v92)));
      ((uint16_t *)v93)[n5_2 + 1901] += 6;
      v93->f3800 += 6;
      ((uint16_t *)v94)[n5_2 + 1901] += 6;
      v94->f3800 += 6;
      v61 = _this->f12[0];
      if ( (v61 & 7) != 7 )
      {
        ((uint16_t *)v93)[n5_2 + 1909] += 4;
        v93->f3816 += 4;
        ((uint16_t *)v94)[n5_2 + 1909] += 4;
        v94->f3816 += 4;
        v61 = _this->f12[0];
      }
      if ( (v61 & 7) != 0 )
      {
        ((uint16_t *)v93)[n5_2 + 1893] += 3;
        v93->f3784 += 3;
        ((uint16_t *)v94)[n5_2 + 1893] += 3;
        v94->f3784 += 3;
        v61 = _this->f12[0];
      }
    }
    else
    {
      v55 = (char *)_this + 16 * (_this->f12[28 - v54] + v53 - _this->f12[26 + v54]);
      *(uint16_t *)(v55 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v55 + 3800) += 7;
      v56 = _this->f12[27];
      v57 = v56 + _this->f12[0] - _this->f12[26 + _this->f12[25]];
      v58 = v8 + v56 - _this->f12[28 - _this->f12[25]];
      v59 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v57));
      ((uint16_t *)v59)[n5_2 + 1901] += 6;
      v59->f3800 += 6;
      v60 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v58));
      ((uint16_t *)v60)[n5_4 + 1901] += 4;
      v60->f3800 += 4;
      v61 = _this->f12[0];
      if ( (v61 & 7) != 7 )
      {
        ((uint16_t *)v59)[n5_2 + 1909] += 4;
        v59->f3816 += 4;
        ((uint16_t *)v60)[n5_4 + 1909] += 2;
        v60->f3816 += 2;
        v61 = _this->f12[0];
      }
      if ( (v61 & 7) != 0 )
      {
        ((uint16_t *)v59)[n5_2 + 1893] += 3;
        v59->f3784 += 3;
        ((uint16_t *)v60)[n5_4 + 1893] += 2;
        v60->f3784 += 2;
        v61 = _this->f12[0];
      }
    }
    v62 = _this->f12[29];
    if ( v62 == 1 )
    {
      v89 = v61 - _this->f12[31];
      v90 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v89 + _this->f12[30])));
      v91 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[32] + v89)));
      ((uint16_t *)v90)[n5_2 + 1901] += 6;
      v90->f3800 += 6;
      ((uint16_t *)v91)[n5_2 + 1901] += 6;
      v91->f3800 += 6;
      v69 = _this->f12[0];
      if ( (v69 & 7) != 7 )
      {
        ((uint16_t *)v90)[n5_2 + 1909] += 4;
        v90->f3816 += 4;
        ((uint16_t *)v91)[n5_2 + 1909] += 4;
        v91->f3816 += 4;
        v69 = _this->f12[0];
      }
      if ( (v69 & 7) != 0 )
      {
        ((uint16_t *)v90)[n5_2 + 1893] += 3;
        v90->f3784 += 3;
        ((uint16_t *)v91)[n5_2 + 1893] += 3;
        v91->f3784 += 3;
        v69 = _this->f12[0];
      }
    }
    else
    {
      v63 = (char *)_this + 16 * (_this->f12[32 - v62] + v61 - _this->f12[30 + v62]);
      *(uint16_t *)(v63 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v63 + 3800) += 7;
      v64 = _this->f12[31];
      v65 = v64 + _this->f12[0] - _this->f12[30 + _this->f12[29]];
      v66 = v8 + v64 - _this->f12[32 - _this->f12[29]];
      v67 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v65));
      ((uint16_t *)v67)[n5_2 + 1901] += 6;
      v67->f3800 += 6;
      v68 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v66));
      ((uint16_t *)v68)[n5_4 + 1901] += 4;
      v68->f3800 += 4;
      v69 = _this->f12[0];
      if ( (v69 & 7) != 7 )
      {
        ((uint16_t *)v67)[n5_2 + 1909] += 4;
        v67->f3816 += 4;
        ((uint16_t *)v68)[n5_4 + 1909] += 2;
        v68->f3816 += 2;
        v69 = _this->f12[0];
      }
      if ( (v69 & 7) != 0 )
      {
        ((uint16_t *)v67)[n5_2 + 1893] += 3;
        v67->f3784 += 3;
        ((uint16_t *)v68)[n5_4 + 1893] += 2;
        v68->f3784 += 2;
        v69 = _this->f12[0];
      }
    }
    v70 = _this->f12[33];
    if ( v70 == 1 )
    {
      v86 = v69 - _this->f12[35];
      v87 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v86 + _this->f12[34])));
      v88 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (_this->f12[36] + v86)));
      ((uint16_t *)v87)[n5_2 + 1901] += 6;
      v87->f3800 += 6;
      ((uint16_t *)v88)[n5_2 + 1901] += 6;
      v88->f3800 += 6;
      v77 = _this->f12[0];
      if ( (v77 & 7) != 7 )
      {
        ((uint16_t *)v87)[n5_2 + 1909] += 4;
        v87->f3816 += 4;
        ((uint16_t *)v88)[n5_2 + 1909] += 4;
        v88->f3816 += 4;
        v77 = _this->f12[0];
      }
      if ( (v77 & 7) != 0 )
      {
        ((uint16_t *)v87)[n5_2 + 1893] += 3;
        v87->f3784 += 3;
        ((uint16_t *)v88)[n5_2 + 1893] += 3;
        v88->f3784 += 3;
        v77 = _this->f12[0];
      }
    }
    else
    {
      v71 = (char *)_this + 16 * (_this->f12[36 - v70] + v69 - _this->f12[34 + v70]);
      *(uint16_t *)(v71 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v71 + 3800) += 7;
      v72 = _this->f12[35];
      v73 = v72 + _this->f12[0] - _this->f12[34 + _this->f12[33]];
      v74 = v8 + v72 - _this->f12[36 - _this->f12[33]];
      v75 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v73));
      ((uint16_t *)v75)[n5_2 + 1901] += 6;
      v75->f3800 += 6;
      v76 = (Obj26 *)((uint16_t *)((char *)_this + 16 * v74));
      ((uint16_t *)v76)[n5_4 + 1901] += 4;
      v76->f3800 += 4;
      v77 = _this->f12[0];
      if ( (v77 & 7) != 7 )
      {
        ((uint16_t *)v75)[n5_2 + 1909] += 4;
        v75->f3816 += 4;
        ((uint16_t *)v76)[n5_4 + 1909] += 2;
        v76->f3816 += 2;
        v77 = _this->f12[0];
      }
      if ( (v77 & 7) != 0 )
      {
        ((uint16_t *)v75)[n5_2 + 1893] += 3;
        v75->f3784 += 3;
        ((uint16_t *)v76)[n5_4 + 1893] += 2;
        v76->f3784 += 2;
        v77 = _this->f12[0];
      }
    }
    v78 = _this->f12[37];
    if ( v78 == 1 )
    {
      v83 = v77 - _this->f12[39];
      v84 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v83 + _this->f12[38])));
      result = (uintptr_t)((char *)_this + 16 * (_this->f12[40] + v83));
      ((uint16_t *)v84)[n5_2 + 1901] += 6;
      v84->f3800 += 6;
      *(uint16_t *)(result + 2 * n5_2 + 3802) += 6;
      *(uint16_t *)(result + 3800) += 6;
      v85 = _this->f12[0];
      if ( (v85 & 7) != 7 )
      {
        ((uint16_t *)v84)[n5_2 + 1909] += 4;
        v84->f3816 += 4;
        *(uint16_t *)(result + 2 * n5_2 + 3818) += 4;
        *(uint16_t *)(result + 3816) += 4;
        v85 = _this->f12[0];
      }
      if ( (v85 & 7) != 0 )
      {
        ((uint16_t *)v84)[n5_2 + 1893] += 3;
        v84->f3784 += 3;
        *(uint16_t *)(result + 2 * n5_2 + 3786) += 3;
        *(uint16_t *)(result + 3784) += 3;
      }
    }
    else
    {
      v79 = (char *)_this + 16 * (_this->f12[40 - v78] + v77 - _this->f12[38 + v78]);
      *(uint16_t *)(v79 + 2 * n5_2 + 3802) += 7;
      *(uint16_t *)(v79 + 3800) += 7;
      v80 = _this->f12[39];
      result = (uintptr_t)((char *)_this + 16 * (v80 + _this->f12[0] - _this->f12[38 + _this->f12[37]]));
      v81 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v80 - _this->f12[40 - _this->f12[37]] + v8)));
      *(uint16_t *)(result + 2 * n5_2 + 3802) += 6;
      *(uint16_t *)(result + 3800) += 6;
      ((uint16_t *)v81)[n5_4 + 1901] += 4;
      v81->f3800 += 4;
      v82 = _this->f12[0];
      if ( (v82 & 7) != 7 )
      {
        *(uint16_t *)(result + 2 * n5_2 + 3818) += 4;
        *(uint16_t *)(result + 3816) += 4;
        ((uint16_t *)v81)[n5_4 + 1909] += 2;
        v81->f3816 += 2;
        v82 = _this->f12[0];
      }
      if ( (v82 & 7) != 0 )
      {
        *(uint16_t *)(result + 2 * n5_2 + 3786) += 3;
        *(uint16_t *)(result + 3784) += 3;
        result = (uint16_t)((uint16_t *)v81)[n5_4 + 1893] + 2;
        ((uint16_t *)v81)[n5_4 + 1893] = result;
        v81->f3784 += 2;
      }
    }
  }
  return result;
}
static inline uint32_t __fwd_alt_p1_alloc_alt_init_tables(void *a0, void *a1) { return __alt_init_tables((Obj43 *)a0, (Obj16 *)a1); }
static inline uint16_t * __fwd_alt_p1_alloc_init_counter_node(void *a0) { return __init_counter_node((uint16_t *)a0); }

int32_t *__alt_p1_alloc(Obj0 *_this, int32_t i, int32_t a3, int32_t n4)
{
  ;
  bool v11;
  int32_t v6, v7, v8, v9, v10, v12, v13, v15, v16, v17, v18, v20, v21, v22, v23, v24, v25, v27,
          v28;
  uint32_t n0x99C60, n0x80, n5;
  n0x99C60 = 0;
  _this->f0 = i;
  _this->f4 = a3;
  do
    __fwd_alt_p1_alloc_init_counter_node((uint16_t *)_this + 8 * n0x99C60++ + 1900);
  while ( n0x99C60 < 0x99C60 );
  _this->f8 = 0;
  v6 = 0;
  v7 = 0;
  v8 = 0;
  v27 = n4 << 8;
  do
  {
    v9 = (uint8_t)__byte_439BC0[v6];
    v28 = v8;
    *((uint8_t *)_this + 2 * v8 + 216) = v6;
    v10 = (2 * v8 == v9) + v6;
    *((int32_t *)_this + 2 * v8 + 438) = v27 | v7;
    v11 = 2 * v8 == (uint8_t)__byte_439BC8[v7];
    *((uint8_t *)_this + 2 * v8 + 217) = v10;
    v12 = 2 * v8 + 1;
    v13 = v11 + v7;
    v6 = (v12 == (uint8_t)__byte_439BC0[v10]) + v10;
    *((int32_t *)_this + 2 * v8 + 439) = v13 | v27;
    v7 = (v12 == (uint8_t)__byte_439BC8[v13]) + v13;
    ++v8;
  }
  while ( (uint32_t)(v28 + 1) < 0x100 );
  n0x80 = 0;
  v15 = 0;
  do
  {
    v16 = (uint8_t)__byte_439BD0[v15];
    *((uint8_t *)_this + 2 * n0x80 + 728) = 8 * v15;
    v17 = (2 * n0x80 == v16) + v15;
    *((uint8_t *)_this + 2 * n0x80 + 729) = 8 * v17;
    v18 = 2 * n0x80++ + 1 == (uint8_t)__byte_439BD0[v17];
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
    *((int32_t *)_this + n5++ + 44) = (int32_t)bmf_new(2 * _this->f0 + 20);
  while ( n5 < 5 );
  __fwd_alt_p1_alloc_alt_init_tables((uint8_t *)_this + 984, (char *)_this + 1496);
  v20 = _this->f0;
  if ( _this->f0 > -10 )
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
      v20 = _this->f0;
      ++v21;
    }
    while ( v21 < _this->f0 + 10 );
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

char *__rc_begin_encode()
{
  ;
  char *__rc_begin_encode_n256, *__rc_begin_encode_n256_1;
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
      __n8_0 = 8;
      __n8_1 = 8;
    }
    else
    {
      __n8_1 = 16;
      __n8_0 = 64;
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
    v2 = 2 * (uint8_t)level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    *(uint64_t *)((char *)model_geometry + v2) = 0x404040404040404LL;
    level_geom[5].half = 8;
    v3 = v2 + 2 * (uint8_t)level_geom[4].half;
    level_geom[5].first = v3;
    level_geom[5].tbl_base = v3 - 5;
    memset((char *)model_geometry + v3, 0x05, 16);
    level_geom[6].half = 16;
    v4 = v3 + 2 * (uint8_t)level_geom[5].half;
    level_geom[6].first = (char)v4;
    level_geom[6].tbl_base = (uint8_t)v4 - 6;
    memset((char *)model_geometry + v4, 0x06, 32);
    level_geom[7].half = 32;
    v6 = v4 + 2 * (uint8_t)level_geom[6].half;
    level_geom[7].first = (char)v6;
    level_geom[7].tbl_base = (uint8_t)v6 - 7;
    // 64 bytes of 7, after 16 of 5 and 32 of 6 -- one level per line.  MSVC
    // inlined this third one because the length crossed its threshold, which
    // is why it arrived as a scalar head, three aligned stores and a tail.
    memset((char *)model_geometry + v6, 0x07, 64);
    __rc_begin_encode_n256 = (char *)bmf_new(0x7F000u);
    if ( __rc_begin_encode_n256 )
    {
      v9 = 0;
      v10 = 0;
      __rc_begin_encode_n256_1 = __rc_begin_encode_n256;
      do
      {
        v13 = v9;
        *((uint16_t *)__rc_begin_encode_n256_1 + 1) = 24 * __n8_1;
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
static inline int32_t __fwd_alt_p1_d8_encode_body_alt_p1_encode_symbol(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_encode_symbol((uint16_t *)a0, a1, a2, a3); }
static inline int32_t __fwd_alt_p1_d8_encode_body_alt_p1_context(void *a0, void *a1, int32_t a2) { return __alt_p1_context((Obj25 *)a0, (uint32_t *)a1, (Obj91 *)a2); }

void __alt_p1_d8_encode_body(Obj0 *_this, uint8_t *a2, uint8_t *a3)
{
  ;
  char *v9;   // were int32_t: these hold addresses
  bool v31;
  char v33, v35;
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
      v9 = (char *)_this->f12[41];
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
      v31 = _this->f0 <= 0;
      _this->f12[4] = *(uint8_t *)(v27 - 1) + v30;
      if ( !v31 )
      {
        v32 = a3;
        v40 = v4;
        v42 = 0;
        do
        {
          ++v42;
          __fwd_alt_p1_d8_encode_body_alt_p1_context((uint8_t **)_this, nullptr, 0);
          v33 = *(uint8_t *)&_this->f8;
          v34 = (uint8_t)(*a2 - v33);
          v35 = *(uint8_t *)(*(uint8_t *)((char *)_this + (v34 + 984)) + (char *)_this + 1496) + v33;
          n5 = *(uint8_t *)((char *)_this + (v34 + 984));
          n16 = (uint8_t)*v32 - (uint8_t)(v35 + *v32 - *a2);
          if ( n16 < -16 || n16 > 16 )
          {
            *v32 = *a2;
            n5 = *(uint8_t *)((char *)_this + (v34 + 1240));
          }
          else
          {
            *v32 = v35;
          }
          __fwd_alt_p1_d8_encode_body_alt_p1_encode_symbol(
            (uint16_t *)((char *)_this + 16 * _this->f12[0] + 3800),
            16 * _this->f12[0],
            _this->f12[1],
            n5);
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
          LODWORD(v39) = 16 * _this->f12[0];
          _this->f12[2] = _this->f12[2] == 0;
          if ( *(uint16_t *)((char *)_this + (v39 + 3800)) < 0x4000u )
            __alt_p1_model(_this);
          _this->f12[46] += 2;
          ++v32;
          _this->f12[47] += 2;
          _this->f12[48] += 2;
          _this->f12[49] += 2;
          _this->f12[50] += 2;
          ++a2;
        }
        while ( v42 < _this->f0 );
        v4 = v40;
        a3 = v32;
      }
    }
    while ( v4 < *(uint32_t *)&_this->f4 );
  }
  __rc_end_encode();
}
static inline void ** __fwd_alt_model_p1_d8_encode_alt_p1_free(void *a0, char a1) { return __alt_p1_free((void **)a0, a1); }
static inline void __fwd_alt_model_p1_d8_encode_alt_p1_d8_encode_body(Obj0 *a0, void *a1, void *a2) { __alt_p1_d8_encode_body(a0, (uint8_t *)a1, (uint8_t *)a2); }
static inline int32_t * __fwd_alt_model_p1_d8_encode_alt_p1_alloc(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_alloc((Obj0 *)a0, a1, a2, a3); }

void __alt_model_p1_d8_encode(uint8_t *a1, int32_t i, int32_t a3, uint8_t *a4)
{
  ;
  Obj0 *v4;
  void **v5;
  v4 = (Obj0 *)((int32_t *)bmf_new(0x99D4D8u));
  if ( v4 )
    v5 = (void **)__fwd_alt_model_p1_d8_encode_alt_p1_alloc(v4, i, a3, 0);
  else
    v5 = nullptr;
  __fwd_alt_model_p1_d8_encode_alt_p1_d8_encode_body((Obj0 *)v5, a1, a4);
  if ( v5 )
    __fwd_alt_model_p1_d8_encode_alt_p1_free(v5, 1);
}
int32_t __set_new_handler(int32_t __out_of_memory_handler)
{
  ;
  int32_t __set_new_handler_pout_of_memory_handler;
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

static inline uint32_t __fwd_alt_p2_alloc_alt_init_tables(void *a0, void *a1) { return __alt_init_tables((Obj43 *)a0, (Obj16 *)a1); }

char *__alt_p2_alloc(char *_this, int32_t i, int32_t n4)
{
  ;
  int32_t v7, v8, v9, v13, Size, v17, v18, v20, v21, v22, v23, v24, v26, v27, v28, v29;
  uint32_t j, k, n0x1E60, m_1, m, n5, n0x82, n;
  void *v10;
  *(uint32_t *)(_this + 278728) = n4;
  for ( j = 0; j < 0x14000; ++j )
  {
    *(uint16_t *)(_this + 8 * j + 284714) = 0;
    *(uint16_t *)(_this + 8 * j + 284718) = 0;
  }
  for ( k = 0; k < 0x14000; ++k )
  {
    *(uint8_t *)(_this + 8 * k + 284712) = 5;
    *(uint8_t *)(_this + 8 * k + 284713) = 2;
    *(uint8_t *)(_this + 8 * k + 284716) = 5;
    *(uint8_t *)(_this + 8 * k + 284717) = 2;
  }
  n0x1E60 = 0;
  do
  {
    v7 = 16 * n0x1E60;
    *(uint16_t *)(_this + v7 + 940074) = 2048;
    ++n0x1E60;
    *(uint16_t *)(_this + v7 + 940076) = 2816;
    *(uint16_t *)(_this + v7 + 940078) = 2816;
    *(uint16_t *)(_this + v7 + 940072) = 4096;
    *(uint16_t *)(_this + v7 + 940082) = 2048;
    *(uint16_t *)(_this + v7 + 940084) = 2816;
    *(uint16_t *)(_this + v7 + 940086) = 2816;
    *(uint16_t *)(_this + v7 + 940080) = 4096;
  }
  while ( n0x1E60 < 0x1E60 );
  v8 = 4 * plane_desc[0].w12 + 1;
  v9 = 16 * plane_desc[0].w12;
  *(uint32_t *)(_this + 278732) = (uint8_t)(plane_desc[(uint8_t)plane_desc[*(uint32_t *)(_this + 278728) + 1].b1 + 1].b2
                                               & 8) >> 3;
  deadzone_hi = v8;
  deadzone_lo = -v8;
  *(uint32_t *)(_this + 278720) = -v9 - 7;
  *(uint32_t *)(_this + 278724) = v9 + 8;
  *(char **)(_this + 278660) = (char *)bmf_new(4 * i + 16);
  v10 = bmf_new(4 * i + 16);
  *(uint32_t *)(_this + 232) = 0x3F800000 /* 1.0f */;
  *(char **)(_this + 278664) = (char *)v10;
  *(char **)(_this + 278668) = *(char **)(_this + 278660) + 4 * i + 8;
  if ( i > -4 )
  {
    m_1 = (i + 4) / 2;
    if ( m_1 )
    {
      for ( m = 0; m < m_1; ++m )
      {
        *(char **)(*(char **)(_this + 278664) + 8 * m) = _this;
        *(char **)(*(char **)(_this + 278660) + 8 * m) = _this;
        *(char **)(*(char **)(_this + 278664) + 8 * m + 4) = _this;
        *(char **)(*(char **)(_this + 278660) + 8 * m + 4) = _this;
      }
      v13 = 2 * m + 1;
    }
    else
    {
      v13 = 1;
    }
    if ( i + 4 > (uint32_t)(v13 - 1) )
    {
      *(char **)(*(char **)(_this + 278664) + 4 * v13 - 4) = _this;
      *(char **)(*(char **)(_this + 278660) + 4 * v13 - 4) = _this;
    }
  }
  n5 = 0;
  Size = 18 * i + 234;
  do
    *(char **)(_this + 4 * n5++ + 278756) = (char *)bmf_new(Size);
  while ( n5 < 5 );
  memset(*(char **)(_this + 278756),0,Size);
  v17 = *(uint32_t *)(_this + 278756);
  ctx_bias[3] = 0;
  v18 = 0;
  ctx_bias[2] = 0;
  ctx_bias[1] = 0;
  n0x82 = 0;
  ctx_bias[0] = 0;
  *(uint32_t *)(_this + 278736) = v17 + 144;
  do
  {
    v20 = (uint8_t)__byte_439890[v18];
    *(uint32_t *)(_this + 8 * n0x82 + 278944) = (*(uint32_t *)(_this + 278728) << 8) | (16 * v18);
    v21 = (2 * n0x82 == v20) + v18;
    v22 = (uint8_t)__byte_439890[v21];
    *(uint32_t *)(_this + 8 * n0x82 + 278948) = (*(uint32_t *)(_this + 278728) << 8) | (16 * v21);
    v23 = 2 * n0x82++ + 1 == v22;
    v18 = v23 + v21;
  }
  while ( n0x82 < 0x82 );
  v24 = 0;
  for ( n = 0; n < 0x3C; ++n )
  {
    v26 = (uint8_t)__byte_4398A0[v24];
    *(uint8_t *)(_this + 2 * n + 280752) = v24;
    v27 = (2 * n == v26) + v24;
    v28 = (uint8_t)__byte_4398A0[v27];
    *(uint8_t *)(_this + 2 * n + 280753) = v27;
    v29 = 2 * n + 1 == v28;
    v24 = v29 + v27;
  }
  *(uint32_t *)(_this + 278704) = 15;
  __fwd_alt_p2_alloc_alt_init_tables((uint8_t *)(_this + 279984), (_this + 280496));
  *(uint32_t *)(_this + 278844) = 0;
  *(uint32_t *)(_this + 278784) = 64;
  *(uint32_t *)(_this + 278828) = 0;
  *(uint32_t *)(_this + 278788) = 128;
  *(uint32_t *)(_this + 278812) = 0;
  *(uint32_t *)(_this + 278800) = 192;
  *(uint32_t *)(_this + 278796) = 0;
  *(uint32_t *)(_this + 278804) = 384;
  *(uint32_t *)(_this + 278780) = 0;
  *(uint32_t *)(_this + 278816) = 576;
  *(uint32_t *)(_this + 278820) = 1152;
  *(uint32_t *)(_this + 278832) = 1728;
  *(uint32_t *)(_this + 278836) = 3456;
  *(uint32_t *)(_this + 278848) = 5184;
  *(uint32_t *)(_this + 278852) = 10368;
  return _this;
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
//   buf = (char *)result + result[3] + 16;   which start at +16
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
  char *buf;
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
      buf = (char *)result + result[3] + 16;
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
  // three.  Where a read of +12 was typed `char *` it stays a value cast back
  // from the size, because that is what the code then does with it: `&x[p_i]`
  // with x the size and p_i the descriptor is `p_i + data_size`, which is where
  // alloc_image put the palette.
  BmfImage *const img = (BmfImage *)p_i;
  struct alignas(16) {   // 96 bytes, the frame Hex-Rays could not name
      uint32_t  Buffera;
      uint32_t  v62;
      uint8_t   _gap0[4];   // was char * v65
      int32_t   v66;
      uint8_t   _gap1[4];   // was FILE * Stream_v
      uint8_t   _gap2[4];   // was int32_t Buffer_2
      uint8_t   _gap3[4];   // was char * buf
      uint8_t   _gap4[4];   // was uint8_t * v72
      uint8_t   _pad8[4];
      uint8_t   _gap5[4];   // was int32_t Buffer_5
      uint8_t   _pad10[4];
      uint8_t   _gap6[4];   // was int32_t n2_2
      uint8_t   _gap7[4];   // was int32_t v75
      uint8_t   _gap8[4];   // was char * buf_2
      uint8_t   _pad14[4];
      uint8_t   _gap9[4];   // was uint32_t v77
      uint8_t   _pad16[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 96,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad16) == 64,
                "the named part of the frame moved");
  char &Buffer = *((char *)&__frame.Buffera);
  uint32_t &Buffera = __frame.Buffera;
  char * &Bufferb = *(char * *)((char *)&__frame.Buffera);
  char * &Bufferc = *(char * *)((char *)&__frame.Buffera);
  char * &Bufferd = *(char * *)((char *)&__frame.Buffera);
  uint32_t &Size_2 = __frame.v62;
  uint32_t &v62 = __frame.v62;
  uint32_t &v63 = __frame.v62;
  char *v65;
  int32_t &v66 = __frame.v66;
  int32_t &v67 = __frame.v66;
  uint8_t * &v68 = *(uint8_t * *)((char *)&__frame.v66);
  FILE *Stream_v;
  int32_t Buffer_2;
  char *buf;
  uint8_t *v72;
  int32_t Buffer_5;
  int32_t n2_2;
  int32_t v75;
  char *buf_2;
  uint32_t v77;
  ;
  uintptr_t p_i_1;   // were int32_t: addresses, masked and tagged
  FILE *Stream_1, *Stream_2;
  bool v33;
  char *Bufferc_3, *Bufferc_1, *Bufferb_1, *v19, *v24, *buf_1, *v31, *v39,
       v42, v43, v44, v45, v46, v47, *buf_3, *Bufferc_2, v56;
  int32_t v3, i, Buffer_1, n8, v14, v16, v17, v20, v21, v22, v23, v25, v26, v28, n4, n2,
          Buffer_3, Buffer_4, Size, v40, n2_1, v49, Size_1, v55;
  uint16_t *p_i_2;
  uint32_t v15, v18, Bufferb_2, Size_3, ElementCount, v53;
  v3 = a3;
  Stream_1 = fopen(FileName, "wb");
  if ( !Stream_1 )
    return 0;
  Bufferc_3 = (char *)bmf_new(img->data_size
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
  Buffer = Buffer_1;
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
    if ( (Buffer & 0x40) != 0 )
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
      Bufferb_1 = (char *)(uintptr_t)img->data_size;
      v67 = 4 * v66;
    }
    else if ( Buffer < 0 )
    {
      Bufferb_1 = (char *)(uintptr_t)img->data_size;
      if ( v66 <= 0 )
      {
        v67 = 4 * v14;
      }
      else
      {
        v62 = v14 / 2;
        if ( v14 / 2 )
        {
          Bufferb = (char *)(uintptr_t)img->data_size;
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
      Bufferb_1 = (char *)(uintptr_t)img->data_size;
      Buffer_2 = img->height;
    }
    buf = &Bufferc_1[v67 + 54];
  }
  else
  {
    Bufferb_1 = (char *)(uintptr_t)img->data_size;
  }
  Size_2 = img->stride;
  Bufferc = Bufferc_1;
  v65 = (char *)(buf - Bufferc_1);
  Stream_v = Stream_1;
  p_i_2 = (uint16_t *)p_i_1;
  v28 = v3;
  Bufferb_2 = (uint32_t)Bufferb_1;
  while ( 1 )
  {
    buf_1 = buf;
    v31 = (char *)p_i_2 + Bufferb_2 - Size_2 + 16;
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
uint32_t __init_symbol_list(int32_t *a1, int32_t a2, int32_t a3, int32_t a4)
{
  ;
  char *buf;
  int32_t v7, v9, v10;
  uint32_t result;
  a1[0] = a3;
  buf = (char *)bmf_new(3 * a3);
  a1[5] = (int32_t)buf;
  if ( a4 )
  {
    v7 = a1[0];
    a1[2] = 0;
    a1[1] = v7;
    result = 12 * v7;
    a1[3] = 12 * v7;
    a1[4] = 8 * v7;
    if ( v7 )
    {
      result = 0;
      do
      {
        v9 = 3 * result;
        *(uint16_t *)(a1[5] + v9) = result++;
        *(uint8_t *)(a1[5] + v9 + 2) = 1;
      }
      while ( result < a1[1] );
    }
  }
  else
  {
    v10 = a1[0];
    a1[2] = 2;
    a1[4] = 20 * v10;
    a1[1] = 0;
    a1[3] = 18 * v10;
    return (uint32_t)memset(buf,0,3 * v10);
  }
  return result;
}

char * __interleave_plane(char *p_i, char *Src, int32_t a3, char a4)
{
  ;
  char *Src_5, *v25;
  int32_t v4, n6_2, v8, n6_1, v10, Size, n4, Size_2, v15, Size_1, v17, v18, v20, v21, v23, n2,
          v26, v27, v29, v30, v31, v32, n4_1, v34, n6;
  uint32_t v6, Src_1, Src_2;
  uint8_t *Src_4, *Src_3;
  if ( (plane_desc[a3 + 1].b2 & 8) == 0 )
  {
    Size = *(uint16_t *)p_i * *((uint16_t *)p_i + 1);
    n4 = plane_count;
    Src_1 = (uint32_t)&p_i[a3 + 16];
    if ( plane_count == 1 )
      return (char *)memcpy(&p_i[a3 + 16],Src,Size);
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
  v29 = (uint8_t)plane_desc[1].b1 - a3;
  v31 = (uint8_t)plane_desc[2].b1 - a3;
  n2 = (uint8_t)plane_desc[a3 + 1].b0;
  v34 = (uint8_t)plane_desc[a3 + 1].b3;
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
    if ( plane_desc[a3 + 1].b0 != 1 )
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
  v29 = (uint8_t)plane_desc[2].b1 - a3;
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

char * __colour_transform(char *Blockb, char *Src, int32_t a3, char a4)
{
  ;
  char v8, v11, *v29;
  int32_t v4, n6_2, v7, n6_1, v10, Size, n4, Size_1, v16, Size_2, v18, v19, v21, v22, v23, v24,
          v26, n2, v30, v31, v33, v34, v35, v36, v37, n4_1, n6;
  uint32_t Src_2, Src_1, v27, Src_3;
  uint8_t *Src_5, *Src_4;
  if ( (plane_desc[a3 + 1].b2 & 8) == 0 )
  {
    Size = *(uint16_t *)Blockb * *((uint16_t *)Blockb + 1);
    n4 = plane_count;
    Src_1 = (uint32_t)&Blockb[a3 + 16];
    if ( plane_count == 1 )
      return (char *)memcpy(Src,&Blockb[a3 + 16],Size);
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
  v33 = (uint8_t)plane_desc[1].b1 - a3;
  v35 = (uint8_t)plane_desc[2].b1 - a3;
  n2 = (uint8_t)plane_desc[a3 + 1].b0;
  v37 = (uint8_t)plane_desc[a3 + 1].b3;
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
      v33 = (uint8_t)plane_desc[2].b1 - a3;
      goto LABEL_4;
    }
  }
  if ( plane_desc[a3 + 1].b0 == 1 )
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
      *Src++ = (char)(v26 - (v27 >> 7));
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
      *Src++ = (char)(v21 - ((uint32_t)(v22 + v23 + 63) >> 7));
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
int32_t __rc_begin_decode(char ArgList_1)
{
  ;
  char ArgList;
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
      __n8_0 = 8;
      __n8_1 = 8;
    }
    else
    {
      __n8_1 = 16;
      __n8_0 = 64;
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
    v7 = 2 * (uint8_t)level_geom[3].half + 4;
    level_geom[4].first = 2 * level_geom[3].half + 4;
    level_geom[4].tbl_base = 2 * level_geom[3].half;
    *(uint64_t *)((char *)model_geometry + v7) = 0x404040404040404LL;
    level_geom[5].half = 8;
    v8 = v7 + 2 * (uint8_t)level_geom[4].half;
    level_geom[5].first = v8;
    level_geom[5].tbl_base = v8 - 5;
    memset((char *)model_geometry + v8, 0x05, 16);
    level_geom[6].half = 16;
    v9 = v8 + 2 * (uint8_t)level_geom[5].half;
    level_geom[6].first = (char)v9;
    level_geom[6].tbl_base = (uint8_t)v9 - 6;
    memset((char *)model_geometry + v9, 0x06, 32);
    level_geom[7].half = 32;
    v11 = v9 + 2 * (uint8_t)level_geom[6].half;
    level_geom[7].first = (char)v11;
    level_geom[7].tbl_base = (uint8_t)v11 - 7;
    memset((char *)model_geometry + v11, 0x07, 64);
    n256 = (uint16_t *)bmf_new(0x7F000u);
    if ( n256 )
    {
      ArgList = ArgList_1;
      tbl = n256;
      v16 = 0;
      v17 = 0;
      do
      {
        v20 = v16;
        tbl[1] = 24 * __n8_1;
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

uint8_t *__unpredict_med(char *Src, int32_t i, int32_t a3)
{
  ;
  uintptr_t Src_1, v41;   // were int32_t: addresses, masked and tagged
  char v39, v40;
  int32_t n256, n128_1, i_1, v42, v45, v46, v47, v48;
  uint32_t j, v36, m_1, m, v44, v50;
  uint8_t *result, *v43;
  alignas(16) uint8_t v52[255];
  Src_1 = (uintptr_t)Src;
  n256 = plane_desc[0].w12;
  result = (uint8_t *)(Src + 1);
  n128_1 = 2 * plane_desc[0].w12 + 1;
  // The test here was `if ( plane_predictor )`, with a 45-line else building
  // a table for predictor mode 0.  Nothing reaches it: expand_image calls
  // this from two places and both are guarded by the predictor being 1 --
  // the second through n2_2, which is `*(uint32_t *)p_i` read back from
  // `*(uint32_t *)p_i = n2_1`, and n2_1 is `plane_desc[v37 + 1].b2 & 3`,
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
int32_t __estimate_cost(char *a1, int32_t n2)
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
static inline int32_t __fwd_alt_model_p1_d8_decode_alt_p1_decode_symbol(void *a0, int32_t a1, int32_t a2) { return __alt_p1_decode_symbol((uint16_t *)a0, a1, a2); }
static inline void ** __fwd_alt_model_p1_d8_decode_alt_p1_free(void *a0, char a1) { return __alt_p1_free((void **)a0, a1); }
static inline int32_t __fwd_alt_model_p1_d8_decode_alt_p1_context(void *a0, void *a1, int32_t a2) { return __alt_p1_context((Obj25 *)a0, (uint32_t *)a1, (Obj91 *)a2); }
static inline int32_t * __fwd_alt_model_p1_d8_decode_alt_p1_alloc(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_alloc((Obj0 *)a0, a1, a2, a3); }

void ** __alt_model_p1_d8_decode(char ArgList, uint8_t *Src, int32_t i, int32_t a4)
{
  ;
  uint8_t *v7, *v8, *v9, *v10, *v11;   // the five row cursors of f176
  bool v33;
  Obj4 *v5;
  Obj0 *v4;
  int32_t v6, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26,
          v27, v28, v29, v30, v31, v32, v35, v36, v39, v41;
  int64_t v37;
  uint8_t *v12, *v13, *Src_1;
  v4 = (Obj0 *)((int32_t *)bmf_new(0x99D4D8u));
  if ( v4 )
    v5 = (Obj4 *)(__fwd_alt_model_p1_d8_decode_alt_p1_alloc(v4, i, a4, 0));
  else
    v5 = (Obj4 *)(nullptr);
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
          __fwd_alt_model_p1_d8_decode_alt_p1_context((uint32_t *)v5, nullptr, 0);
          v36 = (uint8_t)(*((uint8_t *)v5 + 8)
                                + *((uint8_t *)v5 + (uint8_t)__fwd_alt_model_p1_d8_decode_alt_p1_decode_symbol(&((int32_t *)v5)[4 * v5->f0[3] + 950], v35, v5->f0[4]) + 1496));
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
          LODWORD(v37) = 16 * v5->f0[3];
          v5->f0[5] = v5->f0[5] == 0;
          if ( *(uint16_t *)((char *)v5 + v37 + 3800) < 0x4000u )
            __alt_p1_model((Obj0 *)v5);
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
  return __fwd_alt_model_p1_d8_decode_alt_p1_free((void **)v5, 1);
}

static inline int32_t __fwd_alt_model_p1_decode_alt_p1_decode_symbol(void *a0, int32_t a1, int32_t a2) { return __alt_p1_decode_symbol((uint16_t *)a0, a1, a2); }
static inline void ** __fwd_alt_model_p1_decode_alt_p1_free(void *a0, char a1) { return __alt_p1_free((void **)a0, a1); }
static inline int32_t __fwd_alt_model_p1_decode_alt_p1_context(void *a0, void *a1, int32_t a2) { return __alt_p1_context((Obj25 *)a0, (uint32_t *)a1, (Obj91 *)a2); }
static inline int32_t * __fwd_alt_model_p1_decode_alt_p1_alloc(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_alloc((Obj0 *)a0, a1, a2, a3); }

int32_t __alt_model_p1_decode(uint16_t *p_i, char *Src)
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
  Obj0 *v83;
  void *v84;
  int32_t v85;
  uint32_t v86;
  uint32_t v87;
  int32_t v88;
  int32_t v89;
  char *Src_1;
  int32_t v91;
  int32_t v92;
  void * Block_plane[4];
  Obj0 * &v94 = (Obj0 * &)Block_plane[1];
  Obj25 * &v95 = (Obj25 * &)Block_plane[2];
  Obj25 * &v96 = (Obj25 * &)Block_plane[3];
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
  Obj0 *v59;
  uintptr_t v61;
  Obj1 *v24;
  uint8_t *v29, *v65;   // row cursors out of Obj1
  char v11, v12, v13, v58;
  Obj0 *v6;
  uint8_t *v25, *v26, *v27, *v28;   // row cursors out of Obj1
  int32_t i, v3, i_2, n4, *v7, v8, v9, v10, v14, v15, v16, ArgList, v18, i_3, n4_1, n4_2,
          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45,
          v46, v47, v48, v49, v50, v52, v53, v54, v56, v57, v60, v62, v64, v67, v68, v71, v72,
          v74, v75, v78, v79, n4_3, n4_4;
  uint32_t v20, *v51;
  Obj25 *v66;
  Obj25 *v73;
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
      v6 = (Obj0 *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __fwd_alt_model_p1_decode_alt_p1_alloc(v6, i_2, v3, n4);
      else
        v7 = nullptr;
      Block_plane[n4++] = v7;
    }
    while ( n4 < plane_count );
  }
  v8 = 16 * (uint8_t)plane_desc[2].b1;
  v9 = 16 * (uint8_t)plane_desc[3].b1;
  v10 = 16 * (uint8_t)__n3_0;
  v11 = __byte_44339E[v8];
  v12 = __byte_44339E[v9];
  v13 = __byte_44339E[v10];
  v14 = (uint8_t)__byte_44339F[v9];
  v15 = (uint8_t)__byte_44339F[v10];
  v97 = (uint8_t)__byte_44339F[v8];
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
        Src_1 = (char *)Src;
        do
        {
          ++n4_2;
          // `&v92 + n` is `Block_plane[n - 1]`: v92 is the member
          // before the array, and this loop pre-increments from 0.
          v24 = (Obj1 *)Block_plane[n4_2 - 1];
          **(uint16_t **)&v24->cur[0] = *(uint16_t *)(v24->cur[0] - 2);
          *(uint16_t *)(v24->cur[0] + 2) = *(uint16_t *)(v24->cur[0] - 4);
          *(uint16_t *)(v24->cur[0] + 4) = *(uint16_t *)(v24->cur[0] - 6);
          *(uint16_t *)(v24->cur[0] + 6) = *(uint16_t *)(v24->cur[0] - 8);
          *(uint16_t *)(v24->cur[0] + 8) = *(uint16_t *)(v24->cur[0] - 10);
          *(uint16_t *)(v24->cur[0] + 10) = *(uint16_t *)(v24->cur[0] - 12);
          v25 = v24->row[4];
          v26 = v24->row[3];
          v27 = v24->row[2];
          v28 = v24->row[1];
          v29 = v24->row[0];
          v24->row[4] = v26;
          v24->row[3] = v27;
          v24->row[2] = v28;
          v24->row[1] = v29;
          v24->row[0] = v25;
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
          v24->f20 = 0;
          v24->f24 = 0;
          v24->f28 = 0;
          v32 = *((char *)v30 - 3);
          v24->f24 = v32;
          v33 = *((char *)v30 - 1);
          v24->f28 = v33;
          v34 = *((char *)v31 - 3) + v32;
          v24->f24 = v34;
          v35 = *((char *)v31 - 1) + v33;
          v24->f28 = v35;
          v36 = v30[1] + v34;
          v24->f24 = v36;
          v37 = v30[3] + v35;
          v24->f28 = v37;
          v38 = v31[1] + v36;
          v24->f24 = v38;
          v39 = v31[3] + v37;
          v24->f28 = v39;
          v40 = v30[5] + v38;
          v24->f24 = v40;
          v41 = v30[7] + v39;
          v24->f28 = v41;
          v42 = v31[5] + v40;
          v24->f24 = v42;
          v43 = v31[7] + v41;
          v24->f28 = v43;
          v44 = v30[9] + v42;
          v24->f24 = v44;
          v45 = v30[11] + v43;
          v24->f28 = v45;
          v46 = v31[9] + v44;
          v47 = (int32_t)(uintptr_t)v24->cur[0];
          v24->f24 = v46;
          v48 = v31[11] + v45;
          v24->f28 = v48;
          v49 = *(uint8_t *)(v47 - 7) + v46;
          v24->f24 = v49;
          v50 = *(uint8_t *)(v47 - 5) + v48;
          v24->f28 = v50;
          v24->f24 = *(uint8_t *)(v47 - 3) + v49;
          v24->f28 = *(uint8_t *)(v47 - 1) + v50;
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
          __fwd_alt_model_p1_decode_alt_p1_context((uint8_t **)Block_plane[0], nullptr, 0);
          v53 = __fwd_alt_model_p1_decode_alt_p1_decode_symbol(&v51[4 * v51[3] + 950], v52, v51[4]);
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
            __alt_p1_model((Obj0 *)v51);
          v58 = v103;
          v51[49] += 2;
          v51[50] += 2;
          v51[51] += 2;
          v51[52] += 2;
          v51[53] += 2;
          v59 = (Obj0 *)v94;
          *(uint8_t *)((uint8_t)plane_desc[1].b1 + Src) = v58;
          __fwd_alt_model_p1_decode_alt_p1_context((uint8_t **)v59, Block_plane[0], 0);
          v61 = __fwd_alt_model_p1_decode_alt_p1_decode_symbol((uint16_t *)((char *)v59 + 16 * v59->f12[0] + 3800), v60, v59->f12[1]);
          v62 = *(uint32_t *)&v59->f8;
          v63 = *(uint8_t **)&v59->f12[46];
          v64 = (uint8_t)(v62 + *(uint8_t *)((char *)v59 + (v61 + 1496)));
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
          v65 = (uint8_t *)(16 * v59->f12[0]);
          v59->f12[2] = v59->f12[2] == 0;
          if ( *(uint16_t *)((char *)v59 + (intptr_t)(v65 + 3800)) < 0x4000u )
            __alt_p1_model(v59);
          v59->f12[46] += 2;
          v59->f12[47] += 2;
          v59->f12[48] += 2;
          v59->f12[49] += 2;
          v59->f12[50] += 2;
          if ( v101 )
            v104 += v97 + *(uint8_t *)((uint8_t)plane_desc[1].b1 + Src);
          v66 = (Obj25 *)(v95);
          v84 = Block_plane[0];
          v83 = v94;
          *(uint8_t *)(Src + (uint8_t)plane_desc[2].b1) = v104;
          __fwd_alt_model_p1_decode_alt_p1_context(v66, v83, (int32_t)v84);
          v68 = __fwd_alt_model_p1_decode_alt_p1_decode_symbol(&((uint8_t**)v66)[4 * (uint32_t)v66->f12 + 950], v67, (int32_t)v66->f16);
          v69 = v66->f8;
          v70 = v66->f196;
          v71 = (uint8_t)((uint8_t)(uintptr_t)v69 + *((uint8_t *)v66 + v68 + 1496));
          v105 = v71;
          *v70 = v71;
          v66->f196[1] = abs32(v71 - (uint32_t)v69);
          ((uint8_t**)v66)[(uint32_t)v66->f20 + 6] = &((uint8_t**)v66)[(uint32_t)v66->f20 + 6][v66->f196[1]
                                                           - *(v66->f196 - 7)
                                                           - (*(v66->f212 - 3)
                                                            - v66->f212[13])
                                                           - (*(v66->f204 - 3)
                                                            - v66->f204[13])];
          v72 = 4 * (uint32_t)v66->f12;
          v66->f20 = (uint8_t *)(v66->f20 == nullptr);
          if ( LOWORD(((uint8_t**)v66)[v72 + 950]) < 0x4000u )
            __alt_p1_model((Obj0 *)v66);
          v66->f196 += 2;
          v66->f200 += 2;
          v66->f204 += 2;
          v66->f208 += 2;
          v66->f212 += 2;
          if ( ArgList_1 )
            *(uint8_t *)((uint8_t)plane_desc[3].b1 + Src) = ((plane_desc[(uint8_t)plane_desc[3].b1 + 1].w4
                                                                * *(uint8_t *)((uint8_t)plane_desc[1].b1
                                                                                     + Src)
                                                                + plane_desc[(uint8_t)plane_desc[3].b1 + 1].w8
                                                                * (uint32_t)*(uint8_t *)((uint8_t)plane_desc[2].b1 + Src)
                                                                + 40) >> 7)
                                                              + v98
                                                              + v105;
          else
            *(uint8_t *)((uint8_t)plane_desc[3].b1 + Src) = v105;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            v73 = (Obj25 *)(v96);
            __fwd_alt_model_p1_decode_alt_p1_context(v96, v95, (int32_t)v94);
            v75 = __fwd_alt_model_p1_decode_alt_p1_decode_symbol(&((uint8_t**)v73)[4 * (uint32_t)v73->f12 + 950], v74, (int32_t)v73->f16);
            v76 = v73->f8;
            v77 = v73->f196;
            v78 = (uint8_t)((uint8_t)(uintptr_t)v76 + *((uint8_t *)v73 + v75 + 1496));
            v92 = v78;
            *v77 = v78;
            v73->f196[1] = abs32(v78 - (uint32_t)v76);
            ((uint8_t**)v73)[(uint32_t)v73->f20 + 6] = &((uint8_t**)v73)[(uint32_t)v73->f20 + 6][v73->f196[1]
                                                             - *(v73->f196 - 7)
                                                             - (*(v73->f212 - 3)
                                                              - v73->f212[13])
                                                             - (*(v73->f204 - 3)
                                                              - v73->f204[13])];
            v79 = 4 * (uint32_t)v73->f12;
            v73->f20 = (uint8_t *)(v73->f20 == nullptr);
            if ( LOWORD(((uint8_t**)v73)[v79 + 950]) < 0x4000u )
              __alt_p1_model((Obj0 *)v73);
            v73->f196 += 2;
            v73->f200 += 2;
            v73->f204 += 2;
            v73->f208 += 2;
            v73->f212 += 2;
            if ( v91 )
              v92 += ((plane_desc[(uint8_t)__n3_0 + 1].w8 * *(uint8_t *)((uint8_t)__n3_0 + Src - 2)
                     + plane_desc[(uint8_t)__n3_0 + 1].w4 * *(uint8_t *)((uint8_t)__n3_0 + Src - 3)
                     + plane_desc[(uint8_t)__n3_0 + 1].w12 * *(uint8_t *)((uint8_t)__n3_0 + Src - 1)
                     + 64) >> 7)
                   + v85;
            *(uint8_t *)((uint8_t)__n3_0 + Src) = v92;
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
        __fwd_alt_model_p1_decode_alt_p1_free(v82, 1);
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

int32_t __alt_p2_filter(__m128 *_this, __m128 *a2, Obj12 *a3, int32_t n2)
{
  const float *mix = bmf_p2_mix[n2];
  float acc[4], mixed[7][4], centre, prediction, own;
  int i, j, k;

  // One number for the whole of `a2`: its seven rows against the seven
  // coefficient rows, then a horizontal sum of what is left.
  for ( k = 0; k < 4; k++ )
  {
    acc[k] = bmf_p2_coef[0][k] * a2[0].m128_f32[k];
    for ( j = 1; j < 7; j++ )
      acc[k] += bmf_p2_coef[j][k] * a2[j].m128_f32[k];
  }
  centre = bmf_hsum4(acc);
  a2[7].m128_f32[0] = centre;

  // Every row is then recentred on it, in place -- this is the only thing the
  // function writes back into `a2[0..6]`.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
      a2[j].m128_f32[k] -= centre;

  // The six sub-models mixed with this plane count's weights, row by row.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
    {
      mixed[j][k] = mix[0] * a3->f0[0][j].m128_f32[k];
      for ( i = 1; i < 6; i++ )
        mixed[j][k] += mix[i] * a3->f0[i][j].m128_f32[k];
    }

  // The mixture against the recentred rows, summed and put back on the centre.
  for ( k = 0; k < 4; k++ )
  {
    acc[k] = mixed[0][k] * a2[0].m128_f32[k];
    for ( j = 1; j < 7; j++ )
      acc[k] += mixed[j][k] * a2[j].m128_f32[k];
  }
  prediction = bmf_hsum4(acc) + centre;
  a2[7].m128_f32[2] = prediction;

  if ( _this[15].m128_u32[0] )
  {
    // `_this` holds a second set of weights, kept from the previous call, and
    // the answer is the two predictions blended 47:169.2.
    for ( k = 0; k < 4; k++ )
    {
      acc[k] = _this[0].m128_f32[k] * a2[0].m128_f32[k];
      for ( j = 1; j < 7; j++ )
        acc[k] += _this[j].m128_f32[k] * a2[j].m128_f32[k];
    }
    own = centre + bmf_hsum4(acc);
    a2[7].m128_f32[1] = own;
    return (int32_t)(prediction
                     + ((own - prediction) * _this[14].m128_f32[0])
                         / _this[14].m128_f32[1]);
  }

  // First call against this `_this`: seed it from what was just computed and
  // answer with the mixture alone.
  for ( j = 0; j < 7; j++ )
    for ( k = 0; k < 4; k++ )
    {
      _this[j].m128_f32[k]     = mixed[j][k] * bmf_p2_decay;
      _this[j + 7].m128_f32[k] = a3->f0[0][j + 7].m128_f32[k] * bmf_p2_seed;
    }
  _this[14].m128_f32[0] = 47.0f;
  _this[14].m128_f32[1] = 169.2f;
  _this[14].m128_f32[2] = 1.0f;
  a2[7].m128_f32[1] = prediction;
  return (int32_t)prediction;
}
static inline int32_t __fwd_alt_p2_context_alt_p2_filter(void *a0, void *a1, void *a2, int32_t a3) { return __alt_p2_filter((__m128 *)a0, (__m128 *)a1, (Obj12 *)a2, a3); }

// `alt_p1_context`'s opposite number for p2, and the largest body in the file
// at a thousand lines.  Same evidence for the name: no `rc.` call in it, and
// all four p2 bodies call it from both sides.  It ends by folding the
// neighbourhood sums into three context words and returning the 0..255 index
// they are read with.  What the individual terms *mean* is ALGORITHM.md §9's
// question, and it is still open -- this names the role, not the algorithm.
int32_t __alt_p2_context(Obj11 *a1, const __m128 &a2__ref, const __m128 &a3__ref, Obj11 *a4, Obj11 *a5)
{
  struct alignas(16) {   // 208 bytes, the frame Hex-Rays could not name
      uint8_t slot0[4];
      uint8_t slot4[4];
      uint8_t slot8[4];
      __m128 *  sub[6];   // an Obj12: the six sub-model weight vectors alt_p2_filter walks
      uint8_t   _gap0[4];   // was int16_t * v281
      uint8_t   _gap1[4];   // was int16_t * v282
      uint8_t   _gap2[4];   // was int16_t * v283
      uint8_t   _gap3[4];   // was int16_t * v284
      uint8_t   _gap4[4];   // was int16_t * v285
      uint8_t   _gap5[4];   // was int16_t * v286
      uint8_t   _gap6[4];   // was int32_t n1840_2
      uint8_t   _gap7[4];   // was int32_t n1840_1
      uint8_t   _gap8[4];   // was __m128 * v289
      uint8_t   _gap9[4];   // was int32_t v290
      uint8_t   _gap10[4];   // was int32_t n3536
      uint8_t   _gap11[4];   // was int32_t v292
      uint8_t   _gap12[4];   // was __m128 * v293
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
  int32_t &v246 = *(int32_t *)((char *)__frame.slot0);
  int16_t *&v247 = *(int16_t **)((char *)__frame.slot0);
  int32_t &v248 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v249 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v250 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v251 = *(int32_t *)((char *)__frame.slot0);
  int32_t &n15 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v253 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v254 = *(int32_t *)((char *)__frame.slot0);
  int16_t *&v255 = *(int16_t **)((char *)__frame.slot0);
  int32_t &v256 = *(int32_t *)((char *)__frame.slot4);
  char *&v257 = *(char **)((char *)__frame.slot4);
  int32_t &v258 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v259 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v260 = *(int32_t *)((char *)__frame.slot4);
  uint32_t &v261 = *(uint32_t *)((char *)__frame.slot4);
  int32_t &v262 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v263 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v264 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v265 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v266 = *(int32_t *)((char *)__frame.slot4);
  int32_t &v267 = *(int32_t *)((char *)__frame.slot4);
  int16_t *&v268 = *(int16_t **)((char *)__frame.slot8);
  int32_t &v269 = *(int32_t *)((char *)__frame.slot8);
  int32_t &v270 = *(int32_t *)((char *)__frame.slot8);
  int32_t &v271 = *(int32_t *)((char *)__frame.slot8);
  int32_t &v272 = *(int32_t *)((char *)__frame.slot8);
  int32_t &v273 = *(int32_t *)((char *)__frame.slot8);
  int16_t *&v274 = *(int16_t **)((char *)__frame.slot8);
  __m128 * &v275 = (__m128 * &)__frame.sub[0];
  Obj95 * &v276 = (Obj95 * &)__frame.sub[1];
  int32_t &v277 = (int32_t &)__frame.sub[2];
  int32_t &v278 = (int32_t &)__frame.sub[3];
  int32_t &v279 = (int32_t &)__frame.sub[4];
  int32_t &v280 = (int32_t &)__frame.sub[5];
  Obj36 *v281;
  int16_t *v282;
  Obj67 *v283;
  int16_t *v284;
  Obj96 *v285;
  Obj68 *v286;
  int32_t n1840_2;
  int32_t n1840_1;
  Obj11 *v289;
  int32_t v290;
  int32_t n3536;
  int32_t v292;
  __m128 *v293;
  Obj64 *v294;
  int16_t *v295;
  Obj117 *v296;
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
  Obj3 *v46;
  char *v157, *v172;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  __m128 *v31;
  Obj11 *v196;
  Obj11 *v28;
  Obj11 *v166;
  Obj11 *v118;
  Obj11 *v184;
  __m128 *v50, *v53, *v59, v60, v61, v62, *v110, *v160, *v173, *v194, *v204,
         *v243;
  bool v26, v58;
  char v142;
  float v70, v77, v79, v89, v94, v101, v244;
  int16_t *v102;
  int16_t *v73;
  int16_t *v71;
  Obj5 *v87;
  int16_t *v82;
  int16_t *v230;
  int16_t *v97;
  int16_t *v93;
  Obj21 *v98;
  int16_t *v217;
  int16_t *v76;
  int16_t *v86;
  Obj36 *v95;
  Obj36 *v92;
  Obj36 *v81;
  Obj36 *v69;
  Obj36 *v223;
  Obj64 *v170;
  Obj64 *v115;
  Obj67 *v80;
  Obj67 *v83;
  Obj68 *v91;
  Obj68 *v72;
  Obj68 *v96;
  Obj95 *v245;
  Obj95 *v49;
  Obj95 *v52;
  Obj96 *v100;
  Obj96 *v78;
  Obj96 *v84;
  Obj117 *v129;
  Obj29 *v66;
  int16_t *v7, *v45, *v67, *v90, *v99, *v143, v150, *v158, *v205;
  Obj68 *v54;
  Obj95 *v22;
  Obj30 *v104;
  int32_t v6, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20,
          v21, v23, v25, v27, v29, v30, v47, v48, v51, v55, v56, v57, v65,
          v68, v74, v75, v85, v88, *v103, v105, n2, v107, n3536_5, v109, v111,
          v112, v113, v114, v116, v117, n3536_1, v121, v122, v123, v124, v125,
          v126, v127, v128, v130, n2256, v132, v134, v137, v138, v140, v141,
          v144, n2576, n1840_13, v147, v148, v149, v151, v154, v155, v156,
          n2896, v161, v162, v163, v164, v167, v168, v169, v171, n3536_2,
          v175, v176, v177, v178, v179, v181, v182, n3536_3, v185, v186, v188,
          v189, v190, v191, v192, v193, n1840_14, n1840_15, v198, n1840_16,
          v200, v201, n1840_17, v203, v206, v207, v208, n960, n3536_4, v211,
          v212, v213, v214, v215, n1840_3, n1840_8, n1840_7, n1840_10,
          n1840_9, v222, v224, n1840_11, n1840_12, n255, v228, n1840_5,
          n1840_4, n1840_6, v234, v235, v236, v237, v238, v239, v240, v241,
          v242;
  int8_t v139;
  uint32_t v120, v133, v135, v136, v152, v153, v165, v180;
  uint8_t *v187;
  v6 = a1->f278528[13].m128_i32[0];
  v7 = (int16_t *)a1->f278528[13].m128_i32[1];
  v8 = *(int16_t *)(v6 - 24);
  v9 = 23 * *(int16_t *)(v6 - 6);
  v289 = (Obj11 *)(a1);
  v246 = v6;
  v268 = v7;
  v275 = (__m128 *)a1->f278528[13].m128_i32[2];
  v10 = v275[-2].m128_i16[5];
  n1840_1 = 21 * v275[-1].m128_i16[5]
          + 12 * v7[33]
          + 16 * v7[24]
          + 22 * v7[15]
          + v9
          + 20 * v7[6]
          + ctx_bias[0]
          + 14 * v8;
  n1840_2 = 17 * v10
          + 21 * v7[25]
          + 15 * v7[16]
          + 25 * v7[7]
          + 9 * *(v7 - 2)
          + 22 * *(int16_t *)(v6 - 4)
          + ctx_bias[1]
          + 19 * *(int16_t *)(v6 - 22);
  v11 = v6;
  v12 = 17 * v7[31]
      + 15 * v7[22]
      + 21 * v7[13]
      + 18 * v7[4]
      + 16 * *(v7 - 5)
      + 22 * *(int16_t *)(v6 - 10)
      + ctx_bias[2]
      + 19 * *(int16_t *)(v6 - 28);
  v13 = v275->m128_i16[5];
  v14 = *(int16_t *)(v289->f278528[13].m128_i32[3] + 10);
  v15 = *(int16_t *)(v11 - 26);
  v16 = *(int16_t *)(v11 - 8);
  v292 = v12;
  n3536 = 14 * v268[32] + 23 * v268[14] + 19 * v268[5] + 25 * v16 + ctx_bias[3] + 17 * v15 + 15 * (v14 + v13);
  v17 = v275[-2].m128_i16[7];
  v18 = v275[2].m128_i16[2] + v275->m128_i16[0];
  v290 = v12 + n3536 + n1840_1 + n1840_2;
  v19 = v268[9];
  v20 = v18 + v19 + v17;
  v21 = *(int16_t *)(v246 - 18);
  v22 = (Obj95 *)(2 * *(int16_t *)(v246 - 36) + 2 * v21);
  v23 = *v268 + 2 * v21;
  v276 = (Obj95 *)((int16_t *)v22);
  v277 = v19 + v23;
  v279 = 16 * v20;
  // Which row of the threshold table: how many of five ratios the coded
  // length has passed.  This was `13 * <the same sum>` used as a flat
  // subscript, with the sum itself recomputed two statements later.
  v278 = (8 * v12 > 43 * n3536)
       + (8 * v12 > 17 * n3536)
       + (8 * v12 > 9 * n3536)
       + (8 * v12 > 5 * n3536)
       + (8 * v12 > 2 * n3536);
  v25 = v290 > bmf_p2_thresholds[v278][9];
  v26 = v290 <= bmf_p2_thresholds[v278][10];
  v256 = ((v290 > bmf_p2_thresholds[v278][12]) + (v290 > bmf_p2_thresholds[v278][11]) + !v26 + v25) << 6;
  v280 = (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[v278][5])
       + (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[v278][4])
       + (16 * n1840_2 > n1840_1 * bmf_p2_thresholds[v278][3]);
  v27 = bmf_p2_thresholds[v278][0];
  v281 = (Obj36 *)((int16_t *)(16 * ((v277 > bmf_p2_thresholds[v278][8]) + (v277 > bmf_p2_thresholds[v278][7]) + (v277 > bmf_p2_thresholds[v278][6]))));
  v28 = (Obj11 *)(v289);
  v257 = (char *)&((int16_t *)v281)[160 * v278 + 2 * v280]
       + (v279 > bmf_p2_thresholds[v278][2] * (int32_t)v276)
       + (v279 > (int32_t)v276 * bmf_p2_thresholds[v278][1])
       + (v279 > (int32_t)v276 * v27)
       + v256;
  v29 = v289->f280864.m128_i16[(uint32_t)v257 + 4];
  if ( v289->f280864.m128_i16[(uint32_t)v257 + 4] )
  {
    v293 = (__m128 *)v289 + 17408;
    v31 = (&((__m128 *)v289)[16 * v29]);
    v289->f278528[8].m128_i32[0] = (int32_t)v31;
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
        acc[k] = v31[0].m128_f32[k] * v28->f278528[0].m128_f32[k];
        for ( j = 1; j < 7; ++j )
          acc[k] += v31[j].m128_f32[k] * v28->f278528[j].m128_f32[k];
      }
      err = ((float)*(int16_t *)(v246 - 18)
             - (bmf_hsum4(acc) + v28->f278528[7].m128_f32[0])) * 2.0999999f;
      floor_ = 7744.0f * v31[14].m128_f32[2];

      for ( j = 0; j < 7; ++j )
        for ( k = 0; k < 4; ++k )
        {
          float x  = v28->f278528[j].m128_f32[k];
          float ms = v31[7 + j].m128_f32[k]
                   + (x * x - v31[7 + j].m128_f32[k]) * bmf_p2_ms_rate;
          v31[7 + j].m128_f32[k] = ms;
          v31[j].m128_f32[k] += bmf_p2_rate[j][k] * err * x / (ms + floor_);
        }
    }
  }
  else
  {
    v30 = v289->f278528[11].m128_i32[3];
    v293 = (__m128 *)v289 + 17408;
    v289->f278528[11].m128_i32[3] = ++v30;
    v28->f280864.m128_i16[(uint32_t)v257 + 4] = v30;
    v28->f278528[8].m128_i32[0] = (int32_t)&((__m128 *)v28)[16 * (int16_t)v30];
  }
  v45 = (int16_t *)v28->f278528[13].m128_i32[1];
  v28->f278528[0].m128_f32[0] = (float)v45[1];
  v46 = (Obj3 *)v28->f278528[13].m128_p[0];
  v28->f278528[0].m128_f32[1] = (float)v45[10];
  v28->f278528[0].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 16) + v45[1] - *(v45 - 8));
  v47 = *(int16_t *)((char *)v46 - 34);
  v48 = v45[1] - *(v45 - 17);
  v276 = (Obj95 *)((int16_t *)v28->f278528[13].m128_i32[2]);
  v49 = (Obj95 *)(v276);
  v28->f278528[0].m128_f32[3] = (float)(v47 + v48);
  v50 = (__m128 *)v28->f278528[13].m128_i32[3];
  v28->f278528[1].m128_f32[0] = (float)(*(v45 - 8) + v45[1] - *((int16_t *)v49 - 8));
  v51 = v50->m128_i16[1];
  v275 = v50;
  v28->f278528[1].m128_f32[1] = (float)(-3 * (v49->f2 - v45[1]) + v51);
  v28->f278528[1].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 16) + v45[19] - v45[10]);
  v28->f278528[1].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 34) + v45[10] - *(v45 - 8));
  v28->f278528[2].m128_f32[0] = (float)(2 * *(int16_t *)((char *)v46 - 16) - *(int16_t *)((char *)v46 - 34));
  v52 = (Obj95 *)(v276);
  v28->f278528[2].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 52) + v45[1] - *(v45 - 26));
  v28->f278528[2].m128_f32[2] = (float)(v52->f2 + v45[10] - v275[1].m128_i16[2]);
  v28->f278528[2].m128_f32[3] = (float)*(int16_t *)((char *)v46 - 52);
  v28->f278528[3].m128_f32[0] = (float)(*(v45 - 17) + v45[1] - *((int16_t *)v52 - 17));
  v28->f278528[3].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 34) + *(v45 - 8) - *(v45 - 26));
  v53 = v275;
  v28->f278528[3].m128_f32[2] = (float)(v45[10] + ((v45[19] + v45[1]) >> 1) - v52->f38);
  v28->f278528[3].m128_f32[3] = (float)v53->m128_i16[1];
  if ( a4 )
  {
    v282 = ((int16_t *)(*(uint32_t *)&a4->f278528[13] - 18));
    v54 = (Obj68 *)(*(uint32_t *)((char *)&a4->f278528[13] + 8) - 18);
    v281 = (Obj36 *)((int16_t *)(*(uint32_t *)((char *)&a4->f278528[13] + 4) - 18));
    v55 = *(uint32_t *)&a5->f278528[13];
    v286 = (Obj68 *)((int16_t *)v54);
    v56 = *(uint32_t *)((char *)&a5->f278528[13] + 4);
    v57 = *(uint32_t *)((char *)&a5->f278528[13] + 8);
    v284 = ((int16_t *)(v55 - 18));
    v58 = v28->f278528[12].m128_i32[3] == 0;
    v283 = (Obj67 *)((int16_t *)(v56 - 18));
    v285 = (Obj96 *)((int16_t *)(v57 - 18));
    if ( !v58 )
    {
      v59 = v293;
      v60 = v293[1];
      v61 = v293[2];
      v62 = v293[3];
      // One number added to all sixteen floats of the first four rows.
      {
        float bias = (float)v46->f2;
        int32_t j, k;
        for ( j = 0; j < 4; ++j )
          for ( k = 0; k < 4; ++k )
            v293[j].m128_f32[k] += bias;
      }
      v46 = (Obj3 *)v28->f278528[13].m128_p[0];
      v45 = (int16_t *)v28->f278528[13].m128_i32[1];
    }
    v65 = v28->f278528[12].m128_i32[2];
    if ( v65 )
    {
      if ( v65 == 1 )
      {
        v81 = (Obj36 *)(v281);
        v28->f278528[4].m128_f32[0] = (float)(v46->f2 + v45[28]);
        v82 = (int16_t *)(v282);
        v28->f278528[4].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 36) + v81->f36 - v81->f0);
        v28->f278528[4].m128_f32[2] = (float)(v45[9] + *((int16_t *)v82 - 9) - v81->f0);
        v83 = (Obj67 *)(v283);
        v84 = (Obj96 *)(v285);
        v28->f278528[4].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 18) + v81->f0 - *((int16_t *)v81 - 9));
        v85 = v83->f36 - v84->f54;
        v86 = (int16_t *)(v284);
        v28->f278528[5].m128_f32[0] = (float)(v45[9] + v85);
        v87 = (Obj5 *)((int16_t *)(v282));
        v28->f278528[5].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 36) + v86[0] - *((int16_t *)v86 - 18));
        v28->f278528[5].m128_f32[2] = (float)(*v45 + v86[0] - v83->f0);
        v58 = v28->f278528[12].m128_i32[3] == 0;
        v28->f278528[5].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 36) + v87->f4);
        if ( v58 )
        {
          v259 = (int32_t)(uintptr_t)v46;
          v89 = (float)(*(int16_t *)((char *)v46 - 36) + v87->f0 - *((int16_t *)v87 - 18));
          v90 = (int16_t *)v28->f278528[13].m128_i32[2];
          v91 = (Obj68 *)(v286);
          v28->f278528[6].m128_f32[0] = v89;
          v92 = (Obj36 *)(v281);
          v28->f278528[6].m128_f32[1] = (float)(*v90 + v87->f0 - v91->f0);
          v28->f278528[6].m128_f32[2] = (float)(v286->f0 + v87->f0 - *v90 + 2 * (*v45 - v92->f0));
          v88 = *(int16_t *)((char *)v259 - 18) + *(v45 - 9) + *((int16_t *)v281 - 18) + v87->f0 - *((int16_t *)v87 - 9) - *((int16_t *)v281 - 9) - *(v45 - 18);
        }
        else
        {
          v28->f278528[6].m128_f32[0] = (float)*v45;
          v28->f278528[6].m128_f32[1] = (float)*(int16_t *)((char *)v46 - 54);
          v28->f278528[6].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 18) + *((int16_t *)v87 - 9) - *((int16_t *)v87 - 18));
          v88 = *(int16_t *)((char *)v46 - 54) + *(int16_t *)((char *)v46 - 18) - *(int16_t *)((char *)v46 - 72);
        }
        v28->f278528[6].m128_f32[3] = (float)v88;
      }
      else
      {
        v93 = (int16_t *)(v282);
        v28->f278528[4].m128_f32[0] = (float)(*(int16_t *)((char *)v46 - 54) + *(int16_t *)((char *)v46 - 18) - *(int16_t *)((char *)v46 - 72));
        v94 = (float)(*(int16_t *)((char *)v46 - 18) + *((int16_t *)v93 - 9) - *((int16_t *)v93 - 18));
        v95 = (Obj36 *)(v281);
        v96 = (Obj68 *)(v286);
        v28->f278528[4].m128_f32[1] = v94;
        v28->f278528[4].m128_f32[2] = (float)(v45[9] + v95->f0 - v96->f18);
        v97 = (int16_t *)(v282);
        v28->f278528[4].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 36) + v95->f36 - v95->f0);
        v28->f278528[5].m128_f32[0] = (float)(*v45 + *((int16_t *)v97 - 18) - *((int16_t *)v95 - 18));
        v28->f278528[5].m128_f32[1] = (float)(*v45 + v97[0] - v95->f0);
        v98 = (Obj21 *)((int16_t *)(v284));
        v28->f278528[5].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 36) + v97[0] - *((int16_t *)v97 - 18));
        v99 = (int16_t *)v28->f278528[13].m128_i32[2];
        v100 = (Obj96 *)(v285);
        v28->f278528[5].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 36) + v98->f0 - *((int16_t *)v98 - 18));
        v28->f278528[6].m128_f32[0] = (float)(*v99 + v98->f0 - v100->f0);
        v28->f278528[6].m128_f32[1] = (float)(*((int16_t *)v98 - 18)
                                       + v98->f0
                                       - *(int16_t *)((char *)v46 - 36)
                                       + 2 * (*(int16_t *)((char *)v46 - 18) - *((int16_t *)v98 - 9)));
        v101 = (float)(*(int16_t *)((char *)v46 - 36) + v98->f4);
        v102 = (int16_t *)(v282);
        v28->f278528[6].m128_f32[2] = v101;
        v28->f278528[6].m128_f32[3] = (float)(*v99 + v102[2]);
      }
    }
    else
    {
      v28->f278528[4].m128_f32[0] = (float)(*(int16_t *)((char *)v46 - 54) + *(int16_t *)((char *)v46 - 18) - *(int16_t *)((char *)v46 - 72));
      v258 = (int32_t)(uintptr_t)v46;
      v66 = (Obj29 *)((int16_t *)v28->f278528[13].m128_i32[3]);
      v28->f278528[4].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 90) + *v45 - *(v45 - 45));
      v67 = (int16_t *)v28->f278528[13].m128_i32[2];
      v28->f278528[4].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 72) + *v45 - *(v45 - 36));
      v247 = v67;
      v68 = *v67 + 3 * v45[9] - 4 * v67[9];
      v69 = (Obj36 *)(v281);
      v70 = (float)(v68 - (((v45[18] - *v45 - (v66->f36 - v66->f0)) >> 1) - v66->f18));
      v71 = (int16_t *)(v282);
      v28->f278528[4].m128_f32[3] = v70;
      v72 = (Obj68 *)(v286);
      v28->f278528[5].m128_f32[0] = (float)(*(int16_t *)((char *)v258 - 36) + v71[0] - *((int16_t *)v71 - 18));
      v28->f278528[5].m128_f32[1] = (float)(v45[9] + v69->f18 - v72->f36);
      v73 = (int16_t *)(v282);
      v28->f278528[5].m128_f32[2] = (float)(*(v45 - 18) + v69->f36 - v72->f0);
      v74 = *(int16_t *)((char *)v258 - 18) - *((int16_t *)v73 - 9);
      v75 = *((int16_t *)v73 - 18) + v73[0] - *(int16_t *)((char *)v258 - 36);
      v76 = (int16_t *)(v284);
      v77 = (float)(v75 + 2 * v74);
      v78 = (Obj96 *)(v285);
      v28->f278528[5].m128_f32[3] = v77;
      v28->f278528[6].m128_f32[0] = (float)(v247[9] + v76[0] - v78->f18);
      v79 = (float)(*(int16_t *)((char *)v258 - 36) + v76[0] - *((int16_t *)v76 - 18));
      v80 = (Obj67 *)(v283);
      v28->f278528[6].m128_f32[1] = v79;
      v28->f278528[6].m128_f32[2] = (float)(*(int16_t *)((char *)v258 - 18) + v80->f18 - v80->f0);
      v28->f278528[6].m128_f32[3] = (float)(v45[9] + v80->f36 - v78->f54);
    }
  }
  else
  {
    v274 = v45;
    v28->f278528[4].m128_f32[0] = (float)(v45[27] + *v45 - v52->f54);
    v239 = *v45;
    v240 = *(v45 - 36);
    v276 = (Obj95 *)(v52);
    v28->f278528[4].m128_f32[1] = (float)(*(int16_t *)((char *)v46 - 72) + v239 - v240);
    v241 = v52->f0 + 3 * v45[9] - 4 * v52->f18;
    v242 = v45[18] - *v45;
    v243 = v275;
    v244 = (float)(v241 - (((v242 - (v275[2].m128_i16[2] - v275->m128_i16[0])) >> 1) - v275[1].m128_i16[1]));
    v245 = (Obj95 *)(v276);
    v28->f278528[4].m128_f32[2] = v244;
    v255 = (int16_t *)v28->f278528[14].m128_i32[0];
    v28->f278528[4].m128_f32[3] = (float)(*((int16_t *)v245 - 9) + v274[9] - v243->m128_i16[0]);
    v28->f278528[5].m128_f32[0] = (float)(v243[1].m128_i16[1] + *v274 - v255[9]);
    v28->f278528[5].m128_f32[1] = (float)v274[27];
    v28->f278528[5].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 54) + *(int16_t *)((char *)v46 - 18) - *(int16_t *)((char *)v46 - 72));
    v28->f278528[5].m128_f32[3] = (float)(*(int16_t *)((char *)v46 - 18) + v243->m128_i16[0] - v243[-2].m128_i16[7]);
    v28->f278528[6].m128_f32[0] = (float)(*(int16_t *)((char *)v46 - 90) + *v274 - *(v274 - 45));
    v28->f278528[6].m128_f32[1] = (float)*v255;
    v28->f278528[6].m128_f32[2] = (float)(*(int16_t *)((char *)v46 - 90) + *(int16_t *)((char *)v46 - 18) - *(int16_t *)((char *)v46 - 108));
    v28->f278528[6].m128_f32[3] = (float)*(v274 - 54);
    v285 = (Obj96 *)(nullptr);
    v283 = (Obj67 *)(nullptr);
    v284 = (int16_t *)(nullptr);
    v286 = (Obj68 *)(nullptr);
    v281 = (Obj36 *)(nullptr);
    v282 = (int16_t *)(nullptr);
  }
  v103 = (int32_t *)v28->f278528[8].m128_i32[3];
  v275 = (__m128 *)*(v103 - 1);
  v104 = (Obj30 *)((int32_t *)v28->f278528[9].m128_i32[0]);
  v276 = (Obj95 *)((int16_t *)v104->f4);
  v277 = v104->f8;
  v278 = *(v103 - 2);
  v279 = v104->f0;
  v280 = *v103;
  v105 = 14 * n3536;
  n2 = 1;
  v107 = 13 * n1840_2;
  if ( 16 * v292 <= 14 * n3536 )
  {
    v105 = 16 * v292;
    n2 = 0;
  }
  if ( v105 > v107 )
    n2 = 2;
  else
    v107 = v105;
  if ( v107 > 11 * n1840_1 )
    n2 = 3;
  n3536_5 = __fwd_alt_p2_context_alt_p2_filter((__m128 *)v28->f278528[8].m128_i32[0], v293, (Obj12 *)__frame.sub, n2);
  v109 = v28->f278528[13].m128_i32[0];
  v110 = (__m128 *)v28->f278528[13].m128_i32[1];
  v28->f278528[10].m128_i32[3] = n3536_5;
  v111 = v110->m128_i16[3];
  v292 = v109;
  v112 = *(int16_t *)(v109 - 12);
  v293 = v110;
  v269 = v112 + v111;
  if ( a4 )
    v269 += (v284[3] + v282[3]) >> 1;
  v113 = *(int16_t *)(v292 - 30);
  v114 = *(int16_t *)(v292 - 48);
  v289 = (Obj11 *)(v28);
  v115 = (Obj64 *)((int16_t *)v28->f278528[13].m128_i32[3]);
  n3536 = n3536_5;
  v116 = *(int16_t *)(v292 - 66);
  v294 = (Obj64 *)(v115);
  v117 = v293[3].m128_i16[6] + v293[-1].m128_i16[2] + v112 + v114 + v116 + v113;
  v118 = (Obj11 *)(v289);
  n3536_1 = n3536;
  v260 = *(int16_t *)(v292 - 14);
  v120 = (v293[1].m128_i16[3] + v293->m128_i16[2] + v293[-1].m128_i16[1] + v260) & 0x80000
       | (v294->f4 + *(int16_t *)(v292 - 32) + 2 * *(int16_t *)(v292 - 68)) & 0x40000
       | (v293[-4].m128_i16[7] + *(int16_t *)(v292 - 50) + *(int16_t *)(v292 - 86) + *(int16_t *)(v292 - 122)) & 0x20000
       | v260 & 0x10000
       | *(uint16_t *)(v292 - 50) & 0x8000
       | (((n3536 > 3536) + (n3536 > 720) + (n3536 > 288)) << 13)
       | ((((uint32_t)(752 - (v269 + v117)) >> 31)
         + ((uint32_t)(400 - (v269 + v117)) >> 31)
         + ((uint32_t)(240 - (v269 + v117)) >> 31)) << 11);
  v289->f278528[9].m128_i32[1] = v120;
  if ( a4 )
  {
    v261 = v120;
    v121 = *((int16_t *)v282 - 16);
    v289 = (Obj11 *)(v118);
    v122 = *(int16_t *)(v292 - 32);
    v123 = v282[2];
    v124 = *((int16_t *)v282 - 7);
    n3536 = n3536_1;
    v125 = *((int16_t *)v284 - 7) & 0x2000000
         | v284[2] & 0x1000000
         | v124 & 0x800000
         | (v284[2] + *((int16_t *)v284 - 16)) & 0x400000
         | (v121 + v123) & 0x200000
         | v122 & 0x100000
         | v261;
    v118 = (Obj11 *)(v289);
    v295 = (int16_t *)v289->f278528[13].m128_i32[2];
  }
  else
  {
    v236 = *(int16_t *)(v292 - 68);
    v237 = *(int16_t *)(v292 - 86);
    v238 = *(int16_t *)(v292 - 122);
    v289 = (Obj11 *)(v118);
    n3536 = n3536_1;
    v295 = (int16_t *)v118->f278528[13].m128_i32[2];
    v125 = (v293[3].m128_i16[5] + *(int16_t *)(v292 - 50) + v238 + v237) & 0x2000000
         | (v295[11] + v295[2] + v236 + *(int16_t *)(v292 - 140)) & 0x1000000
         | (v236 + *(int16_t *)(v292 - 104)) & 0x800000
         | v236 & 0x400000
         | v237 & 0x200000
         | v238 & 0x100000
         | v120;
  }
  v126 = v125 >> 11;
  v118->f278528[9].m128_i32[1] = v126;
  v127 = ((1 << ((v118->f284704.m128_i8[4 * v126 + 8] + 31) & 31)) + v118->f284704.m128_i16[2 * v126 + 5]) >> (v118->f284704.m128_i8[4 * v126 + 8] & 31);
  v128 = v290;
  v129 = (Obj117 *)((int16_t *)v118->f278528[14].m128_i32[0]);
  v130 = v292;
  v118->f278896[0].m128_i32[3] = v127;
  n2256 = v127 + n3536_1;
  v118->f278896[0].m128_i32[2] = n2256;
  v132 = *(int16_t *)(v130 - 90);
  v296 = (Obj117 *)(v129);
  n1840 = v129->f72;
  v290 = ((v269 << 9) + v128) >> 13;
  n960_1 = v132 - n2256;
  v133 = ((uint32_t)(24 - v290) >> 20) & 0xFFFFF800;
  n1840_1 = n1840 - n2256;
  v301 = v293[5].m128_i16[5];
  n1840_2 = v301 - n2256;
  v297 = ((uint32_t)(39 - v290) >> 20) & 0xFFFFF800;
  if ( a4 )
  {
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v289 = (Obj11 *)(v118);
    v134 = v282[1];
    v248 = *(int16_t *)(v292 + 2);
    v270 = v293[2].m128_i16[3];
    v135 = (v248 + *(int16_t *)(v292 - 52) - n2256 - (v270 - v293[5].m128_i16[6])) & 0x800000
         | (v248 + *(int16_t *)(v292 - 88) - n2256) & 0x400000
         | (v248 + v293->m128_i16[1] + v283->f38 - v285->f38 - n2256) & 0x200000
         | (v248 + *(int16_t *)(v292 - 16) + v284[1] - *((int16_t *)v284 - 8) - n2256) & 0x100000
         | (*(v295 - 8) + v248 + v134 - *((int16_t *)v286 - 8) - n2256) & 0x80000
         | (v248 + v270 + v134 - v281->f38 - n2256) & 0x40000
         | n1840_2 & 0x20000
         | n1840_1 & 0x10000
         | n960_1 & 0x8000
         | (((n2256 > 2256) + (n2256 > 1056) + (n2256 > 144)) << 13)
         | ((((uint32_t)(55 - v290) >> 20) & 0xFFFFF800) + v298 + v133);
    v118 = (Obj11 *)(v289);
    v136 = v298;
    v137 = (*(int16_t *)(v292 - 18) + v282[0] - *((int16_t *)v282 - 9) - n2256) & 0x2000000
         | (v286->f2 + v281->f2 - 2 * v282[1]) & 0x1000000
         | v135;
  }
  else
  {
    v289 = (Obj11 *)(v118);
    v298 = ((uint32_t)(10 - v290) >> 20) & 0xFFFFF800;
    v254 = v294->f0;
    v267 = n2256 - *(int16_t *)(v292 - 54);
    v273 = v293[2].m128_i16[2];
    v137 = (v293[-6].m128_i16[3] - v293[-3].m128_i16[6] + v267) & 0x1000000
         | (v267 + v273 - v301) & 0x800000
         | (v296->f54 - v295[18] + n2256 - v295[9]) & 0x400000
         | (*((int16_t *)v296 - 9) - v254 + n2256 - v293[-2].m128_i16[7]) & 0x200000
         | (v293->m128_i16[0] - v273 + n2256 - *(int16_t *)(v292 - 36)) & 0x100000
         | -n1840_2 & 0x80000
         | -n1840_1 & 0x40000
         | (n2256 - v296->f36) & 0x20000
         | (n2256 - *((int16_t *)v296 - 27)) & 0x10000
         | -n960_1 & 0x8000
         | (((n2256 > 2400) + (n2256 > 1024) + (n2256 > 240)) << 13)
         | (v297 + v133 + (((uint32_t)(11 - v290) >> 20) & 0xFFFFF800))
         | (n1840 - v254 + n2256 - v293[4].m128_i16[4]) & 0x2000000;
    v136 = v298;
  }
  v298 = v136;
  v138 = v137 >> 11;
  v118->f278528[9].m128_i32[2] = v138;
  v139 = v118->f415776.m128_i8[4 * v138 + 8];
  v140 = v118->f415776.m128_i16[2 * v138 + 5];
  v141 = 1 << ((v139 + 31) & 31);
  v142 = v139;
  v143 = v295;
  v144 = (v141 + v140) >> (v142 & 31);
  n2576 = v144 + n2256;
  v118->f278896[1].m128_i32[0] = n2576;
  v118->f278896[1].m128_i32[1] = v144;
  n1840_13 = *v143;
  n960_1 = v293->m128_i16[0];
  v147 = *(int16_t *)(v292 - 36);
  n1840 = n1840_13;
  v302 = v147 - n2576;
  v301 = v147 + n2576 - 2 * *(int16_t *)(v292 - 18);
  v303 = n1840_13 - n2576;
  if ( a4 )
  {
    v148 = *(int16_t *)(v292 + 2);
    v149 = *(int16_t *)(v292 - 16);
    v289 = (Obj11 *)(v118);
    v150 = v284[0];
    v249 = v148;
    v151 = v282[0];
    v262 = v284[0];
    n1840_2 = v293[1].m128_i16[2];
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
    v154 = (v249 + v293[-1].m128_i16[0] - n2576 - (v295[1] - n1840_2)) & 0x2000000
         | (v249 + v149 - n2576 - (v293->m128_i16[1] - n1840_2)) & 0x1000000
         | (v262 - n2576 + n1840 - v285->f0) & 0x800000
         | (n1840_1 + n1840 - v286->f0) & 0x400000
         | (n1840_1 + v293[-2].m128_i16[7] - *((int16_t *)v281 - 9)) & 0x200000
         | v152;
  }
  else
  {
    v235 = v294->f0;
    v289 = (Obj11 *)(v118);
    v153 = v298;
    v266 = v296->f0;
    v272 = v294->f18;
    v154 = (n2576 + 3 * (n1840 - n960_1) - v235) & 0x2000000
         | (n2576 + v266 - (v295[18] + *(v295 - 18))) & 0x1000000
         | (v294->f36 - *(v295 - 9) + n2576 - v293[3].m128_i16[3]) & 0x800000
         | (v272 - v293[1].m128_i16[1] - v303) & 0x400000
         | (v235 - n960_1 - v303) & 0x200000
         | v301 & 0x100000
         | (n2576 - v296->f54) & 0x80000
         | (v272 - n2576) & 0x40000
         | (n2576 - v266) & 0x20000
         | (n2576 - *((int16_t *)v294 - 18)) & 0x10000
         | -v302 & 0x8000
         | (((n2576 > 2464) + (n2576 > 1216) + (n2576 > 688)) << 13)
         | ((((uint32_t)(58 - v290) >> 31) + ((uint32_t)(25 - v290) >> 31) + ((uint32_t)(13 - v290) >> 31)) << 11);
  }
  v289 = (Obj11 *)(v118);
  v298 = v153;
  v155 = v154 >> 11;
  v118->f278528[9].m128_i32[3] = v155;
  v156 = ((1 << ((v118->f546848.m128_i8[4 * v155 + 8] + 31) & 31)) + v118->f546848.m128_i16[2 * v155 + 5]) >> (v118->f546848.m128_i8[4 * v155 + 8] & 31);
  v157 = (char *)v292;
  v158 = v295;
  v118->f278896[1].m128_i32[3] = v156;
  n2896 = v156 + n2576;
  v160 = v293;
  v118->f278896[1].m128_i32[2] = n2896;
  v161 = v160->m128_i16[1];
  v162 = *(int16_t *)(v157 - 36);
  v250 = *(int16_t *)(v157 + 2);
  v263 = v161;
  v163 = *(int16_t *)(v157 - 54);
  v164 = v158[10];
  v304 = v162;
  v305 = v164;
  v303 = n2896 - v163;
  v306 = n2896 - v250;
  v165 = 9 - v290;
  v290 = -v290;
  v166 = (Obj11 *)(v289);
  v167 = (int32_t)((3 * (v304 - *(int16_t *)(v292 - 18)) + n2896 - v163) & 0x2000000
             | (v305
              - ((uint32_t)(v293[1].m128_i16[2] + v293[2].m128_i16[3] + v293[-1].m128_i16[0] + v263) >> 1)
              + v306)
             & 0x1000000
             | (*(v295 - 27) - v293[-3].m128_i16[6] + n2896 - v293[-2].m128_i16[7]) & 0x800000
             | -(v295[18] + n2896 - 2 * v293[1].m128_i16[1]) & 0x400000
             | (v293[-3].m128_i16[7] - v304 + n2896 - v263) & 0x200000
             | (n2896 - v293[3].m128_i16[3]) & 0x100000
             | (v306 - v305) & 0x80000
             | (n2896 - v294->f0) & 0x40000
             | (2 * n2896 - v263 - (v293->m128_i16[0] + v250)) & 0x20000
             | (n2896 - *(int16_t *)(v292 - 16) - v250) & 0x10000
             | (n2896 - v163) & 0x8000
             | (((n2896 > 2896) + (n2896 > 1568) + (n2896 > 592)) << 13)
             | ((((uint32_t)(v290 + 37) >> 31) + ((uint32_t)(v290 + 19) >> 31) + (v165 >> 31)) << 11)) >> 11;
  v289->f278528[10].m128_i32[0] = v167;
  LOBYTE(v163) = v166->f677920.m128_i8[4 * v167 + 8];
  v168 = v166->f677920.m128_i16[2 * v167 + 5];
  v169 = 1 << ((v163 + 31) & 31);
  LOBYTE(v164) = v163;
  v170 = (Obj64 *)(v294);
  v171 = (v169 + v168) >> (v164 & 31);
  v172 = (char *)v292;
  v173 = v293;
  v166->f278896[2].m128_i32[1] = v171;
  n3536_2 = v171 + n2896;
  n3536 = n3536_2;
  v166->f278896[2].m128_i32[0] = n3536_2;
  v175 = v173->m128_i16[0];
  v176 = v170->f0;
  v307 = *(int16_t *)(v172 + 2);
  v308 = v175;
  v177 = *(int16_t *)(v172 - 70);
  v178 = v173[1].m128_i16[1];
  v179 = v173[-2].m128_i16[7];
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
  v184 = (Obj11 *)(v289);
  v185 = (int32_t)((n3536 - ((uint32_t)(v310 + v311 + 2 * v308) >> 2)) & 0x2000000
             | (v313 - *(int16_t *)(v292 - 36) - (v294->f38 + v307)) & 0x1000000
             | (n3536 - 2 * *(int16_t *)(v292 - 34) - (v307 - v309)) & 0x800000
             | (n3536 - *(int16_t *)(v292 - 52) - v307 - (v308 - v293[-4].m128_i16[5])) & 0x400000
             | (v313 - v307 - (v310 + *(v295 - 8))) & 0x200000
             | (*(v295 - 18) + n3536 - 2 * v311) & 0x100000
             | (v295[1] - 2 * v293->m128_i16[1] + v312) & 0x80000
             | (v182 - (v308 - v311)) & 0x40000
             | (v312 - v293[4].m128_i16[5]) & 0x20000
             | (v312 - v293[-3].m128_i16[7]) & 0x10000
             | v181
             | v180) >> 11;
  v289->f278528[10].m128_i32[1] = v185;
  v186 = ((1 << ((v184->f808992.m128_i8[4 * v185 + 8] + 31) & 31)) + v184->f808992.m128_i16[2 * v185 + 5]) >> (v184->f808992.m128_i8[4 * v185 + 8] & 31);
  v187 = (uint8_t *)v295;
  v271 = v186;
  v184->f278896[2].m128_i32[3] = v186;
  n1840 = n3536_3 + v186;
  v184->f278896[2].m128_i32[2] = n3536_3 + v186;
  v188 = v293[-1].m128_u8[15];
  v189 = v187[17];
  v314 = v293[-2].m128_u8[13]
       + v293[4].m128_u8[7]
       + *(uint8_t *)(v292 - 37)
       + *((uint8_t *)v294 + 17)
       + *(v187 - 1)
       + v187[53];
  v190 = v187[89];
  v191 = v188 + *(uint8_t *)(v292 - 19) + v189;
  v192 = *(v187 - 19);
  v315 = v191;
  v193 = *((uint8_t *)v294 + 53)
       + *((uint8_t *)v294 + 35)
       + *((uint8_t *)v294 - 1)
       + *((uint8_t *)v294 - 19)
       + *((uint8_t *)v296 - 19)
       + v187[107]
       + v190
       + v187[71]
       + v192
       + *(v187 - 37);
  v194 = v293;
  n1840_14 = n1840;
  n960_1 = v293[6].m128_u8[11]
         + v293[5].m128_u8[9]
         + v293[-3].m128_u8[11]
         + v293[-4].m128_u8[9]
         + 3 * (v293[3].m128_u8[5] + *((uint8_t *)v295 + 35))
         + 7 * v293[1].m128_u8[1]
         + 6 * v293[2].m128_u8[3]
         + *(uint8_t *)(v292 - 91)
         + *(uint8_t *)(v292 - 109)
         + *(uint8_t *)(v292 - 127)
         + 8 * *(uint8_t *)(v292 - 1)
         + *((uint8_t *)v296 + 53)
         + *((uint8_t *)v296 + 35)
         + *((uint8_t *)v296 + 17)
         + *((uint8_t *)v296 - 1)
         + *(uint8_t *)(v292 - 55)
         + *(uint8_t *)(v292 - 73)
         + v193
         + 4 * v315
         + 2 * v314;
  v196 = (Obj11 *)(v289);
  n1840_15 = v289->f278528[12].m128_i32[0];
  v289->f278528[15].m128_i32[2] = (n1840 < 1840) + (n1840 < 272);
  v198 = v194->m128_i16[0];
  n1840_16 = v196->f278528[12].m128_i32[1];
  n1840_1 = n1840_15;
  n1840_2 = n1840_16;
  v200 = (n1840_14 - v198 <= n1840_16) + (n1840_14 - v198 < n1840_15);
  v201 = v292;
  v196->f278528[16].m128_i32[2] = v200;
  n1840_17 = n1840_14 - *(int16_t *)(v201 - 18);
  v203 = n1840_17 < n1840_1;
  v26 = n1840_17 <= n1840_16;
  v204 = v293;
  v205 = v295;
  v196->f278528[17].m128_i32[2] = v26 + v203;
  v196->f278528[18].m128_i32[2] = v204[1].m128_u8[0];
  v196->f278528[19].m128_i32[2] = *(uint8_t *)(v201 - 2);
  v206 = *(uint8_t *)(v201 - 19);
  v207 = *(uint8_t *)(v201 - 1);
  v208 = *((uint8_t *)v205 + 17) + v204[1].m128_u8[1];
  n960 = n960_1;
  v302 = v208;
  n3536_4 = n3536;
  v301 = v207 + v206;
  if ( a4 )
  {
    v211 = *((uint8_t *)v284 + 17);
    v212 = *((uint8_t *)v284 - 1);
    v289 = (Obj11 *)(v196);
    v213 = *((uint8_t *)v282 - 1);
    v251 = *((uint8_t *)v283 + 17) + *((uint8_t *)v281 + 17);
    v264 = v211 + *((uint8_t *)v282 + 17);
    n960 = v251 + n960_1 + 4 * v264 + 2 * (v213 + v212);
    v214 = v264 + v301 + *((uint8_t *)v284 - 19) + v212 + *((uint8_t *)v282 - 19) + v213;
    n3536_4 = n3536;
    if ( v196->f278528[12].m128_i32[3] )
    {
      n1840_3 = n1840 - v293->m128_i16[1] - *(int16_t *)(v292 + 2);
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
      v215 = v251 + v264 + v302 + *((uint8_t *)v285 + 17) + *((uint8_t *)v286 + 17);
    }
    if ( v289->f278528[12].m128_i32[2] == 1 )
    {
      v230 = (int16_t *)(v282);
      n960_1 = n960;
      v253 = v214;
      n1840_4 = n1840_2;
      n1840_5 = v282[0] - v281->f0;
      v289->f278528[18].m128_i32[2] = (n1840_5 <= n1840_2) + (n1840_5 < n1840_1);
      n1840_6 = v230[0] - *((int16_t *)v230 - 9);
      v234 = (n1840_6 <= n1840_4) + (n1840_6 < n1840_1);
      v214 = v253;
      n960 = n960_1;
      n3536_4 = n3536;
      v196->f278528[19].m128_i32[2] = v234;
    }
    else if ( v196->f278528[12].m128_i32[2] > 1 )
    {
      v217 = (int16_t *)(v284);
      n960_1 = n960;
      v265 = v215;
      n1840_7 = n1840_2;
      n1840_8 = v284[0] - v283->f0;
      v289->f278528[18].m128_i32[2] = (n1840_8 <= n1840_2) + (n1840_8 < n1840_1);
      n1840_10 = v217[0] - *((int16_t *)v217 - 9);
      n1840_9 = n1840_1;
      v222 = n1840_10 < n1840_1;
      v26 = n1840_10 <= n1840_7;
      v215 = v265;
      v223 = (Obj36 *)(v281);
      n960 = n960_1;
      v196->f278528[19].m128_i32[2] = v26 + v222;
      v224 = v282[0];
      n1840_11 = v224 - v223->f0;
      v26 = n1840_9 <= n1840_11;
      n3536_4 = n3536;
      if ( v26 && n1840_11 <= n1840_2 )
      {
        v214 = 1;
      }
      else
      {
        n1840_12 = v224 - *((int16_t *)v282 - 9);
        v214 = n1840_12 >= n1840_1 && n1840_12 <= n1840_2;
      }
    }
  }
  else
  {
    v214 = v301 + *(uint8_t *)(v292 - 37) + *(uint8_t *)(v292 - 55) + *(uint8_t *)(v292 - 73);
    v215 = *((uint8_t *)v296 + 17) + *((uint8_t *)v294 + 17) + v302 + *(uint8_t *)(v292 + 17);
  }
  if ( n960 >= 960 )
  {
    n15 = 15;
    v196->f278528[11].m128_i32[0] = 15;
  }
  else
  {
    n15 = v196->f280752.m128_u8[n960 >> 3];
    v196->f278528[11].m128_i32[0] = n15;
  }
  n255 = (n3536_4 + v271 + 7) >> 4;
  if ( n255 >= 255 )
    n255 = 255;
  if ( n255 < 0 )
    n255 = 0;
  v196->f278528[11].m128_i32[1] = v196->f278896[4].m128_i32[n255] + n15;
  v228 = v196->f278528[15].m128_i32[2];
  v196->f278528[11].m128_i32[2] = n15 + v196->f278896[3].m128_i32[n255];
  v196->f278528[11].m128_i32[0] = n15
                          + 32 * (v215 == 0)
                          + 16 * (v214 == 0)
                          + v196->f278528[19].m128_i32[v196->f278528[19].m128_i32[2] + 3]
                          + v196->f278528[18].m128_i32[v196->f278528[18].m128_i32[2] + 3]
                          + v196->f278528[17].m128_i32[v196->f278528[17].m128_i32[2] + 3]
                          + v196->f278528[16].m128_i32[v196->f278528[16].m128_i32[2] + 3]
                          + v196->f278528[15].m128_i32[v228 + 3];
  return n255;
}

static inline int32_t __fwd_reduce_alphabet_encode_symbol_list(void *a0, int32_t a1) { return __encode_symbol_list((uint32_t *)a0, a1); }
static inline uint32_t __fwd_reduce_alphabet_init_encode_symbol_list(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __init_symbol_list((int32_t *)a0, a1, a2, a3); }

void __reduce_alphabet(ModelBlock *Blocka, char a2, uint8_t *a3)
{
  struct alignas(16) {   // 66064 bytes, the frame Hex-Rays could not name
      uint32_t v78[15];
      void *v79;
      int32_t n0x2000_5;
      char buf[4];
      uint64_t v82[127];
      int32_t v83;
      int32_t v84;
      int32_t v85;
      uint8_t _pad0[64504];
      uint8_t slot65604[16];
      int32_t v87;
      uint32_t v88[91];
      void *slot[19];   // one array, three bases: `&n4_1`, `*(&Block + n)` and the interleaved `(&v91)[2*j]` / `*(&v92 + 2*j)`
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 66064, "frame layout moved");
  uint32_t (&v78)[15] = __frame.v78;
  void *&v79 = __frame.v79;
  int32_t &n0x2000_5 = __frame.n0x2000_5;
  char (&buf)[4] = __frame.buf;
  uint64_t (&v82)[127] = __frame.v82;
  int32_t &v83 = __frame.v83;
  int32_t &v84 = __frame.v84;
  int32_t &v85 = __frame.v85;
  uint64_t (&v86)[2] = *(uint64_t (*)[2])((char *)__frame.slot65604);
  int32_t &v87 = __frame.v87;
  uint32_t (&v88)[91] = __frame.v88;
  int32_t &n4_1 = (int32_t &)__frame.slot[0];
  void * &Block = (void * &)__frame.slot[1];
  uint8_t * &v91 = (uint8_t * &)__frame.slot[2];
  uint32_t &v92 = (uint32_t &)__frame.slot[3];
  uint32_t &v93 = (uint32_t &)__frame.slot[4];
  uint32_t &v94 = (uint32_t &)__frame.slot[6];
  ModelBlock * &Blockaa = (ModelBlock * &)__frame.slot[7];
  uint8_t * &v96 = (uint8_t * &)__frame.slot[8];
  uint32_t &k_1 = (uint32_t &)__frame.slot[9];
  ModelBlock * &Blocka_1 = (ModelBlock * &)__frame.slot[11];
  ;
  ModelBlock *Blockaa_2;
  char *v62;
  bool v46, v48, v59;
  char *v28, v35;
  ModelBlock *Blockaa_1;
  ModelBlock *Blockaa_4;
  int32_t n8, v8, v11, n4, n0x2000_2, n0x2000_1, v20, v26, n4_2, v30, v31, v32, *p_n4, n16_2,
          v39, v44, n256, v49, v50, v51, v52, v54, v55, v56, v57, v58, v63, *p_n4_2, n16_1, v68,
          n0x2000, v71, v72, v74, *p_n4_1, n16;
  ModelBlock *Blockaa_3;
  uint32_t k_2, i, v12, v19, n0x2000_4, n0x2000_3, v24, k, v29, k_3, j_1, j, v53, v61, v64, v70,
           v73, v75;
  uint64_t *n0x2000_6;
  uint8_t *v4, *v10, *v33, *v42, *v43, *v45, *v60;
  void *v3, *v13, *v34;
  Blocka_1 = (ModelBlock *)(Blocka);
  v3 = alloca(65968);
  v4 = a3;
  n8 = Blocka->f8;
  v91 = a3;
  Blockaa = (ModelBlock *)(Blocka);
  v93 = 0xFFFFFFFF >> (-(char)n8 & 31);
  k_2 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v8 = 12 * i;
    v88[v8] = 0;
    v88[v8 + 6] = 0;
  }
  Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
  if ( n8 <= 8 )
  {
    n256 = 256;
    do
    {
      *(__m128i *)&v78[n256 + 12] = 0;
      *(__m128i *)&v78[n256 + 8] = 0;
      *(__m128i *)&v78[n256 + 4] = 0;
      *(__m128i *)&v78[n256] = 0;
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
        v96 = a3 - 1;
        v84 = 0;
        v52 = 0;
        do
        {
          if ( !v51 )
            break;
          v83 = v52;
          v53 = 0;
          v54 = v84;
          v55 = 0;
          do
          {
            v56 = *(int32_t *)&Blockaa_1->f8;
            v57 = v55 - v56;
            if ( v57 < 0 )
            {
              ++v96;
              v57 = 8 - v56;
            }
            v58 = v93 & (*v96 >> (v57 & 31));
            v59 = *(uint32_t *)&buf[4 * v58 - 4] == 0;
            v85 = v57;
            ++v53;
            *(uint32_t *)&buf[4 * v58 - 4] = 1;
            v55 = v85;
            *(int32_t *)&Blockaa_1->f16 += v59;
            *(uint16_t *)(Blockaa_1->f1078236 + 2 * v54) = v58;
            v51 = *(int32_t *)&Blockaa_1->f0;
            ++v54;
          }
          while ( v53 < *(int32_t *)&Blockaa_1->f0 );
          v50 = *(int32_t *)&Blockaa_1->f16;
          v84 = v54;
          v52 = v83 + 1;
        }
        while ( v83 + 1 < (uint32_t)Blockaa_1->f4 );
      }
    }
    else if ( v49 * *(int32_t *)&Blockaa_1->f0 )
    {
      v60 = v91;
      v61 = 0;
      do
      {
        *(int32_t *)&Blockaa_1->f16 += *(uint32_t *)&buf[4 * *v60 - 4] == 0;
        v62 = (char *)Blockaa_1->f1078236;
        v63 = *v60;
        *(uint32_t *)&buf[4 * v63 - 4] = 1;
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
    rc.encode(v50 - 1, v50, v93 + 1);
    v64 = *(int32_t *)&Blockaa_1->f16;
    if ( v64 <= v93 )
    {
      __fwd_reduce_alphabet_init_encode_symbol_list((int32_t *)v86, (int32_t)Blockaa_1, v93 - v64 + 2, 1);
      v87 = 19 * LODWORD(v86[0]);
      v70 = *(int32_t *)&Blockaa_1->f16;
      if ( v70 )
      {
        v71 = 0;
        v72 = 0;
        v73 = 0;
        do
        {
          if ( *(uint32_t *)&buf[4 * v72 - 4] )
          {
            __fwd_reduce_alphabet_encode_symbol_list((uint32_t *)v86, v72 - v71);
            v70 = *(int32_t *)&Blockaa_1->f16;
            *(uint32_t *)&buf[4 * v72 - 4] = v73;
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
          *(uint16_t *)(Blockaa_1->f1078236 + 2 * v75) = *(uint32_t *)&buf[4
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
    memset(buf,0,0x10000);
    *(int32_t *)&Blockaa_1->f16 = 1;
    *(uint32_t *)buf = v93 & *(uint32_t *)a3;
    *(uint16_t *)Blockaa_1->f1078236 = 0;
    if ( (uint32_t)(Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0) > 1 )
    {
      v96 = a3;
      k_1 = k_2;
      Blockaa = (ModelBlock *)((int32_t)Blockaa_1);
      v10 = v91;
      v11 = 0;
      v12 = 1;
      while ( 1 )
      {
        v10 += k_1;
        v13 = (void *)(v93 & *(uint32_t *)v10);
        if ( v13 != *(void **)&buf[8 * v11] )
        {
          v11 = 0;
          if ( v13 != *(void **)buf )
          {
            v92 = v12;
            v91 = v10;
            while ( 1 )
            {
              n4 = *(uint32_t *)&buf[8 * v11] < (uint32_t)v13;
              n0x2000_6 = &v82[v11];
              v11 = *((uint16_t *)n0x2000_6 + n4);
              if ( !*((uint16_t *)n0x2000_6 + n4) )
                break;
              if ( v13 == *(void **)&buf[8 * v11] )
              {
                v12 = v92;
                v10 = v91;
                tbl44573C[1] = n4;
                goto LABEL_12;
              }
            }
            n0x2000_5 = (int32_t)n0x2000_6;
            v10 = v91;
            n4_1 = n4;
            Block = v13;
            Blockaa_2 = (ModelBlock *)(Blockaa);
            v68 = Blockaa->f16;
            tbl44573C[1] = n4;
            v11 = (uint16_t)v68;
            n0x2000 = v68 + 1;
            *(uint16_t *)(n0x2000_5 + 2 * n4) = v11;
            v12 = v92;
            Blockaa_2->f16 = n0x2000;
            if ( n0x2000 > 0x2000 )
            {
              v4 = v96;
              n0x2000_2 = n0x2000;
              k_2 = k_1;
              Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
              goto LABEL_14;
            }
            *(void **)&buf[8 * v11] = Block;
          }
        }
LABEL_12:
        Blockaa_3 = (ModelBlock *)((uint32_t *)Blockaa);
        *(uint16_t *)(*(uint32_t *)&Blockaa->f1078236 + 2 * v12++) = v11;
        if ( v12 >= *(uint32_t *)&Blockaa_3->f4 * Blockaa_3->f0 )
        {
          v4 = v96;
          k_2 = k_1;
          Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
          n0x2000_2 = Blockaa->f16;
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
      Block = bmf_new(Blockaa_1->f4 * k_2 * *(int32_t *)&Blockaa_1->f0);
      v26 = *(int32_t *)&Blockaa_1->f0;
      n4_2 = Blockaa_1->f4;
      n0x2000_5 = *(int32_t *)&Blockaa_1->f0;
      n4_1 = n4_2;
      if ( k_2 )
      {
        v94 = n4_1 * v26;
        if ( k_2 >> 1 )
        {
          v28 = (char *)Block + n4_1 * n0x2000_5;
          v96 = v4;
          k_1 = k_2;
          Blockaa = (ModelBlock *)((int32_t)Blockaa_1);
          v29 = 0;
          do
          {
            v30 = 2 * v29;
            v31 = 2 * v29++ * v94;
            __frame.slot[v30 + 2] = (uint8_t *)Block + v31;
            __frame.slot[v30 + 3] = (uint32_t)&v28[v31];
          }
          while ( v29 < k_2 >> 1 );
          v4 = v96;
          k_2 = k_1;
          Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
          v32 = 2 * v29 + 1;
        }
        else
        {
          v32 = 1;
        }
        if ( k_2 > v32 - 1 )
          __frame.slot[v32 + 1] = (char *)Block + n4_1 * -n0x2000_5 + v94 * v32;
      }
      else
      {
        v94 = n4_1 * v26;
      }
      if ( v94 )
      {
        v33 = a3;
        if ( k_2 )
        {
          k_1 = k_2;
          Blockaa = (ModelBlock *)((int32_t)Blockaa_1);
          n0x2000_5 = 0;
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
                  v42 = __frame.slot[2 * j + 2];
                  *v42 = v33[2 * j];
                  v43 = (uint8_t *)__frame.slot[2 * j + 3];
                  __frame.slot[2 * j + 2] = v42 + 1;
                  *v43 = v33[2 * j + 1];
                  __frame.slot[2 * j + 3] = (uint32_t)(v43 + 1);
                }
                v44 = 2 * j + 1;
                v4 = &v33[2 * j];
              }
              else
              {
                v44 = 1;
              }
              if ( v44 - 1 >= k_1 )
                break;
              v45 = (uint8_t *)__frame.slot[v44 + 1];
              v4 = &v33[v44];
              *v45 = v33[v44 - 1];
              v46 = ++v39 < v94;
              __frame.slot[v44 + 1] = v45 + 1;
              if ( !v46 )
                goto LABEL_71;
              v33 += v44;
            }
            if ( ++v39 >= v94 )
              break;
            v33 = v4;
          }
LABEL_71:
          k_2 = k_1;
          Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
        }
      }
      v79 = (void *)Blockaa_1->f1078236;
      Blockaa_1->f4 = k_2 * n4_1;
      *(int32_t *)&Blockaa_1->f8 = 8;
      free(v79);
      v34 = bmf_new(2 * Blockaa_1->f4 * *(int32_t *)&Blockaa_1->f0);
      v79 = Block;
      Blockaa_1->f1078236 = (int32_t)v34;
      __reduce_alphabet((ModelBlock *)Blockaa_1, v35, (uint8_t *)v79);
      free(Block);
    }
    else
    {
      if ( 4 * k_2 )
      {
        Blockaa = (ModelBlock *)((int32_t)Blockaa_1);
        v19 = 0;
        do
        {
          __fwd_reduce_alphabet_init_encode_symbol_list((int32_t *)&v86[3 * v19], v19, 256, 1);
          ++v19;
        }
        while ( v19 < 4 * k_2 );
        Blockaa_1 = (ModelBlock *)((int32_t *)Blockaa);
        n0x2000_1 = Blockaa->f16;
      }
      if ( n0x2000_1 )
      {
        v20 = 0;
        n0x2000_4 = 0;
        n0x2000_3 = n0x2000_1;
        Blockaa_4 = (ModelBlock *)((int32_t)Blockaa_1);
        do
        {
          v24 = *(uint32_t *)&buf[8 * n0x2000_4];
          if ( k_2 )
          {
            n0x2000_5 = n0x2000_4;
            k_1 = k_2;
            Blockaa = (ModelBlock *)(Blockaa_4);
            for ( k = 0; k < k_1; ++k )
            {
              __fwd_reduce_alphabet_encode_symbol_list((uint32_t *)&v86[12 * k + 3 * v20], (uint8_t)v24);
              v20 = (uint8_t)v24 >> 6;
              v24 >>= 8;
            }
            n0x2000_4 = n0x2000_5;
            k_2 = k_1;
            Blockaa_4 = (ModelBlock *)(Blockaa);
            v24 = *(uint32_t *)&buf[8 * n0x2000_5];
            n0x2000_3 = Blockaa->f16;
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

int32_t __cost_candidate(uint8_t *a1, uint8_t *n2, int32_t a3, char a4, int32_t a5, int32_t a6, int32_t a7, char *a8)
{
  struct alignas(16) {   // 26712 bytes, the frame Hex-Rays could not name
      char buf[4096];
      int32_t v72[1024];
      int32_t v73[1024];
      int32_t v74[1024];
      int32_t v75[1024];
      int32_t v76[1024];
      char buf_1[4];
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
  char (&buf)[4096] = __frame.buf;
  int32_t (&v72)[1024] = __frame.v72;
  int32_t (&v73)[1024] = __frame.v73;
  int32_t (&v74)[1024] = __frame.v74;
  int32_t (&v75)[1024] = __frame.v75;
  int32_t (&v76)[1024] = __frame.v76;
  char (&buf_1)[4] = __frame.buf_1;
  uint32_t &v78 = __frame.v78;
  int32_t &v79 = __frame.v79;
  int32_t &v80 = __frame.v80;
  int32_t &v81 = __frame.v81;
  int32_t &v82 = __frame.v82;
  int32_t &v83 = __frame.v83;
  int32_t &n4 = __frame.n4;
  int32_t &v85 = __frame.v85;
  int32_t &v86 = __frame.v86;
  uint8_t *&n2_2 = __frame.n2_2;
  int32_t &v88 = __frame.v88;
  uint8_t *&v89 = __frame.v89;
  int32_t &v90 = __frame.v90;
  int32_t &v91 = __frame.v91;
  int32_t &v92 = __frame.v92;
  int32_t &v93 = __frame.v93;
  int32_t &v94 = __frame.v94;
  int32_t &v95 = __frame.v95;
  int32_t &n191_5 = __frame.n191_5;
  int32_t &n191_2 = __frame.n191_2;
  uint8_t *&v98 = __frame.v98;
  uint8_t *&v99 = __frame.v99;
  uint8_t *&n2_1 = __frame.n2_1;
  int32_t &v101 = __frame.v101;
  ;
  uintptr_t v63, v64;   // were int32_t: addresses, masked and tagged
  char *v14;   // were int32_t: these hold addresses
  bool v57, v67;
  double v16, v17, v18, v19, v20, v32, n191_1, n191_4;
  int32_t v9, v10, v12, v22, v23, v24, v25, v26, v27, v28, v30, v31, n191, n191_3, v37, v38,
          v39, v40, v41, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v58, v59,
          v60, n191_6, n191_7;
  uint32_t v69, v70;
  uint8_t *n2_3, *v15, *v21, *v29, *v42, *n2_4, *v44, *v65, *v66;
  void *v8;
  v101 = a3;
  n2_1 = n2;
  v99 = a1;
  v8 = alloca(26672);
  v88 = a3;
  n4 = plane_count;
  v9 = *((uint16_t *)a1 + 2);
  n2_2 = n2;
  v89 = a1;
  v92 = (int32_t)(n2_1 + 1) % 3 - (uint32_t)n2_1;
  v95 = (int32_t)(n2_1 + 2) % 3 - (uint32_t)n2_1;
  v78 = (uint32_t)&v99[*((uint32_t *)v99 + 3) + 16];
  v10 = v92;
  *(uint32_t *)buf_1 = v9;
  memset(buf,0,24576);
  v12 = *(uint32_t *)buf_1;
  n2_3 = n2_2;
  v14 = (char *)v88;
  v15 = v89;
  v16 = 0;
  v17 = 0.0;
  v18 = 0.0;
  v19 = 0.0;
  v20 = 0.0;
  v90 = 16 * (uint32_t)n2_2;
  *(uint8_t *)(16 * (uint32_t)n2_2 + v88) = 2;
  *(uint8_t *)(v14 + 33) = (uint8_t)(uintptr_t)n2_3;
  v91 = (int32_t)&n2_3[(uint32_t)v15];
  v21 = &n2_3[(uint32_t)v15 + 16 + v12 + n4];
  if ( (uint32_t)v21 < v78 )
  {
    v92 = v10;
    *(uint32_t *)buf_1 = v12;
    v85 = v10 - v12;
    v83 = v10 - v12 - n4;
    v82 = v10 - n4;
    v86 = v95 - v12;
    v81 = v95 - v12 - n4;
    v80 = v95 - n4;
    v79 = v12 + n4;
    do
    {
      v22 = v95;
      v23 = v21[v83] + v21[v92] - (v21[v85] + v21[v82]);
      v24 = v81;
      ++*(uint32_t *)&buf[4 * v23 + 2048];
      v25 = v21[v24] + v21[v22] - (v21[v86] + v21[v80]);
      v26 = v79;
      v20 = v20 + (double)v23 * (double)v23;
      ++v72[v25 + 512];
      v17 = v17 + (double)v25 * (double)v25;
      v19 = v19 + (double)v23 * (double)v25;
      ++v73[((uint16_t)v25 - (uint16_t)v23 - 512) & 0x3FF];
      v27 = v21[-v26] + *v21;
      v28 = v21[-n4];
      v29 = &v21[-*(uint32_t *)buf_1];
      v21 += n4;
      v30 = v27 - (*v29 + v28);
      v18 = v18 + (double)v23 * (double)v30;
      ++v74[((uint16_t)v30 - (uint16_t)v23 - 512) & 0x3FF];
      v16 = v16 + (double)v25 * (double)v30;
      ++v75[((uint16_t)v30 - (uint16_t)v25 - 512) & 0x3FF];
      v31 = ((uint16_t)v30 - (uint16_t)((uint32_t)(((v23 + v25) << 6) + 40) >> 7) - 512) & 0x3FF;
      ++v76[v31];
    }
    while ( (uint32_t)v21 < v78 );
    v10 = v92;
  }
  v32 = 128.0 / (0.1 - v19 * v19 + v20 * v17);
  n191_1 = (v17 * v18 - v19 * v16) * v32;
  n191_4 = v32 * (v20 * v16 - v19 * v18);
  n191 = (int32_t)n191_1;
  if ( (int32_t)n191_1 >= 191 )
    n191 = 191;
  if ( n191 < -64 )
    n191 = -64;
  n191_2 = n191;
  n191_3 = (int32_t)n191_4;
  if ( (int32_t)n191_4 >= 191 )
    n191_3 = 191;
  if ( n191_3 < -64 )
    n191_3 = -64;
  n191_5 = n191_3;
  memset(buf_1,0,2048);
  v37 = *((uint16_t *)v89 + 2);
  v38 = (*((uint16_t *)v89 + 1) - 1) * *(uint16_t *)v89;
  v92 = v10;
  v94 = v38 - 1;
  v93 = -v37;
  v39 = -plane_count;
  v40 = v91 - (-v37 - plane_count);
  v101 = -v37 - plane_count;
  v99 = (uint8_t *)(v40 + 16);
  v41 = v10 + v40 + 16;
  v42 = (uint8_t *)(v40 + 16);
  v98 = (uint8_t *)v41;
  n2_1 = (uint8_t *)(v95 + v40 + 16);
  n2_4 = n2_1;
  v44 = (uint8_t *)v41;
  do
  {
    v45 = v42[v101];
    v98 = v44;
    v99 = v42;
    n2_1 = n2_4;
    v46 = v45 + *v42 - v42[v93] - v42[v39];
    v47 = *v44;
    v91 = v46;
    v48 = v94;
    n2_4 = &n2_1[-v39];
    v49 = ((uint16_t)v91
         - (uint16_t)((n191_2 * (v44[v101] + v47 - v44[v93] - v44[v39])
                             + n191_5 * (n2_1[v101] + *n2_1 - n2_1[v93] - (uint32_t)n2_1[v39])
                             + 40) >> 7)
         - 256)
        & 0x1FF;
    v44 -= v39;
    ++*(uint32_t *)&buf_1[4 * v49];
    v42 = &v99[-v39];
    v94 = v48 - 1;
  }
  while ( v48 != 1 );
  v50 = v92;
  v51 = __estimate_cost((char *)buf_1, 512);
  v52 = 16 * (uint32_t)n2_2;
  *(uint32_t *)(a8 + 16 * (uint32_t)n2_2) = v51;
  *(uint32_t *)(a8 + v52 + 4) = __estimate_cost((char *)v74, 1024);
  *(uint32_t *)(a8 + v52 + 8) = __estimate_cost((char *)v75, 1024);
  *(uint32_t *)(a8 + v52 + 12) = __estimate_cost((char *)v76, 1024);
  v91 = __estimate_cost((char *)buf, 1024);
  v92 = __estimate_cost((char *)v72, 1024);
  v53 = __estimate_cost((char *)v73, 1024);
  v94 = v53;
  v54 = v53;
  if ( v91 < v53 )
    v54 = v91;
  if ( v92 < v53 )
    v53 = v92;
  v55 = v92 + v54;
  v56 = v91 + v53;
  v57 = v55 < v56;
  v93 = v56;
  v58 = v94;
  if ( v57 )
  {
    v93 = v55;
    v59 = v50;
    v50 = v95;
    v91 = v92;
    v95 = v59;
    v60 = *(uint32_t *)(a8 + v52 + 4);
    *(uint32_t *)(a8 + v52 + 4) = *(uint32_t *)(a8 + v52 + 8);
    n191_6 = n191_5;
    *(uint32_t *)(a8 + v52 + 8) = v60;
    n191_7 = n191_2;
    n191_2 = n191_6;
    n191_5 = n191_7;
  }
  v63 = v90;
  v64 = v88;
  *(uint32_t *)(v90 + v88 + 4) = n191_2;
  *(uint32_t *)(v63 + v64 + 8) = n191_5;
  v65 = &n2_2[v50];
  v66 = &n2_2[v95];
  *(uint8_t *)(16 * (uint32_t)v65 + v64) = 0;
  *(uint8_t *)(v64 + 1) = (uint8_t)(uintptr_t)v65;
  v67 = 0;                            // -S
  *(uint8_t *)(16 * (uint32_t)v66 + v64) = 1;
  *(uint8_t *)(v64 + 17) = (uint8_t)(uintptr_t)v66;
  if ( !v67 && *((uint32_t *)v89 + 3) > 0x1000000u )
    return v91 + v58 + *(uint32_t *)(a8 + v52);
  v69 = *(uint32_t *)(a8 + v52);
  v70 = *(uint32_t *)(a8 + v52 + 8);
  if ( v69 >= *(uint32_t *)(a8 + v52 + 4) )
    v69 = *(uint32_t *)(a8 + v52 + 4);
  if ( v70 >= *(uint32_t *)(a8 + v52 + 12) )
    v70 = *(uint32_t *)(a8 + v52 + 12);
  if ( v69 < v70 )
    v70 = v69;
  return v93 + v70;
}

static inline int32_t __fwd_choose_plane_coding_cost_candidate(void *a0, void *a1, void *a2, char a3, int32_t a4, int32_t a5, int32_t a6, char *a7) { return __cost_candidate((uint8_t *)a0, (uint8_t *)a1, (int32_t)(uintptr_t)a2, a3, a4, a5, a6, a7); }

int32_t __choose_plane_coding(Obj97 *a1, int32_t n3, char a3)
{
  struct alignas(16) {   // 41456 bytes, the frame Hex-Rays could not name
      int32_t v174;
      int32_t v175;
      int32_t v176;
      uint8_t _pad0[4];
      char buf[4];
      uint8_t _pad1[4092];
      int32_t v178[1024];
      int32_t v179[1024];
      int32_t v180[5120];
      char buf_3[4];
      uint8_t _pad2[2044];
      char buf_1[4];
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
      char buf_4[4];
      uint8_t _pad4[2044];
      char buf_2[4];
      uint8_t _pad5[2044];
      __m128i v202;
      __m128i v203;
      __m128i v204;
      __m128i v205;
      __m128i v206;
      __m128i v207;
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
      char v218[64];
      char v219[64];
      int64_t v220;
      double v221;
      double v222;
      uint8_t *v223;
      uint32_t v224;
      int32_t __choose_plane_coding_n191_1;
      uint8_t *v226;
      uint32_t v227;
      uint8_t _pad6[4];
      int32_t v228;
      uint8_t _pad7[28];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 41456, "frame layout moved");
  int32_t &v174 = __frame.v174;
  int32_t &v175 = __frame.v175;
  int32_t &v176 = __frame.v176;
  char (&buf)[4] = __frame.buf;
  int32_t (&v178)[1024] = __frame.v178;
  int32_t (&v179)[1024] = __frame.v179;
  int32_t (&v180)[5120] = __frame.v180;
  char (&buf_3)[4] = __frame.buf_3;
  char (&buf_1)[4] = __frame.buf_1;
  int32_t &v183 = __frame.v183;
  int32_t &v184 = __frame.v184;
  int32_t &v185 = __frame.v185;
  uint8_t *&v186 = __frame.v186;
  uint8_t *&v187 = __frame.v187;
  int32_t &v188 = __frame.v188;
  int32_t &v189 = __frame.v189;
  uint8_t *&v190 = __frame.v190;
  int32_t &v191 = __frame.v191;
  int32_t &v192 = __frame.v192;
  int32_t &v193 = __frame.v193;
  uint8_t *&v194 = __frame.v194;
  uint8_t *&v195 = __frame.v195;
  int32_t &v196 = __frame.v196;
  int32_t &v197 = __frame.v197;
  uint8_t *&v198 = __frame.v198;
  int32_t &v199 = __frame.v199;
  char (&buf_4)[4] = __frame.buf_4;
  char (&buf_2)[4] = __frame.buf_2;
  __m128i &v202 = __frame.v202;
  __m128i &v203 = __frame.v203;
  __m128i &v204 = __frame.v204;
  __m128i &v205 = __frame.v205;
  __m128i &v206 = __frame.v206;
  __m128i &v207 = __frame.v207;
  int64_t &v208 = __frame.v208;
  int64_t &v209 = __frame.v209;
  double &v210 = __frame.v210;
  double &v211 = __frame.v211;
  double &v212 = __frame.v212;
  double &v213 = __frame.v213;
  int32_t (&v214)[4] = __frame.v214;
  uint64_t (&v215)[5] = __frame.v215;
  double &v216 = __frame.v216;
  int32_t (&v217)[16] = __frame.v217;
  char (&v218)[64] = __frame.v218;
  char (&v219)[64] = __frame.v219;
  int64_t &v220 = __frame.v220;
  double &v221 = __frame.v221;
  double &v222 = __frame.v222;
  uint8_t *&v223 = __frame.v223;
  uint32_t &v224 = __frame.v224;
  int32_t &__choose_plane_coding_n191_1 = __frame.__choose_plane_coding_n191_1;
  Obj97 *&v226 = (Obj97 *&)__frame.v226;
  uint32_t &v227 = __frame.v227;
  Obj97 *&v228 = (Obj97 *&)__frame.v228;
  ;
  __m128i v21;
  bool v19, n2_4, v42, v106;
  char v7, v10, v12, v16, v18, *v44, n0x100_1, k;
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
  void *v3;
  v228 = (Obj97 *)(a1);
  v3 = alloca(41424);
  n4 = plane_count;
  v5 = a1->f4;
  v6 = a1->f12;
  v226 = (Obj97 *)((uint8_t *)a1);
  __dword_443388 = 1;
  LODWORD(v208) = v5;
  v227 = (uintptr_t)((char *)a1 + v6 + 16);
  memset(buf,0,0x8000);
  n192 = 192;
  do
  {
    *(__m128i *)((char *)&v215[4] + n192) = 0;
    *(__m128i *)((char *)&v215[2] + n192) = 0;
    *(__m128i *)((char *)v215 + n192) = 0;
    *(__m128i *)((char *)v214 + n192) = 0;
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
        plane_desc[v11 + 1].b1 = 2 * v9;
        v12 = 2 * v9++ + 1;
        plane_desc[v11 + 1].b0 = v10;
        plane_desc[v11 + 2].b1 = v12;
        plane_desc[v11 + 2].b0 = v12;
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
      v14 = 4 * v13;
      BYTE1(__n256_2[v14]) = n192;
      LOBYTE(__n256_2[v14]) = n192;
    }
    if ( n4 >= 3 )
    {
      v15 = __fwd_choose_plane_coding_cost_candidate(v226, nullptr, v217, v7, v174, v175, v176, (char *)&v214[2]);
      v17 = __fwd_choose_plane_coding_cost_candidate(v226, (uint8_t *)1, v218, v16, v174, v175, v176, (char *)&v214[2]);
      v19 = v17 >= v15;
      if ( v17 >= v15 )
        v17 = v15;
      n2_3 = !v19;
      v205.m128_i32[2] = n2_3;
      v21 = 0;
      n2_4 = __fwd_choose_plane_coding_cost_candidate(v226, (uint8_t *)2, v219, v18, v174, v175, v176, (char *)&v214[2]) < v17;
      n2 = n2_3;
      if ( n2_4 )
        n2 = 2;
      v205.m128_i32[2] = n2;
      n16 = 16;
      v25 = &v217[16 * n2];
      do
      {
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 8)) = *(uint64_t *)&v25[n16 - 2];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 16)) = *(uint64_t *)&v25[n16 - 4];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 24)) = *(uint64_t *)&v25[n16 - 6];
        *(uint64_t *)(bmf_plane_desc(n16 * 4 - 32)) = *(uint64_t *)&v25[n16 - 8];
        n16 -= 8;
      }
      while ( n16 * 4 );
      v26 = v214[4 * v205.m128_i32[2] + 2];
      v205.m128_i32[1] = 16 * v205.m128_i32[2];
      LODWORD(v209) = (uint8_t)plane_desc[1].b1 - v205.m128_i32[2];
      n128_1 = plane_desc[v205.m128_i32[2] + 1].w8;
      HIDWORD(v208) = (uint8_t)plane_desc[2].b1 - v205.m128_i32[2];
      n128 = plane_desc[v205.m128_i32[2] + 1].w4;
      // always taken: -S is on.  (The block is kept braced -- LABEL_19 below
      // is jumped to from inside it.)
      {
        LODWORD(v210) = n128_1;
        v207.m128_i32[3] = n128;
        HIDWORD(v209) = n4;
        v202.m128_i32[1] = 4;            // (__n7_0 + 5) / 3, and -Q is 9
        v184 = n128 - 1;
        v57 = n128 - 1;
        v192 = n128_1 - 1;
        n128_2 = n128;
        v59 = n128_1 - 1;
        v183 = n128_2 - v202.m128_i32[1];
        n128_3 = n128_1;
        v61 = v183;
        *(uint32_t *)buf_1 = n128_3 - v202.m128_i32[1];
        v202.m128_i32[0] = (int32_t)&((uint8_t *)v226)[v205.m128_i32[2]];
        v62 = n128_3 - v202.m128_i32[1];
        while ( 1 )
        {
          if ( v57 <= v61 )
          {
            if ( v59 <= v62 )
            {
              v203.m128_i32[0] = v207.m128_i32[3] + 1;
              n192_1 = v207.m128_i32[3] + 1;
              v205.m128_i32[3] = LODWORD(v210) + 1;
              n192_2 = LODWORD(v210) + 1;
              v202.m128_i32[3] = v207.m128_i32[3] + v202.m128_i32[1];
              n192_4 = v207.m128_i32[3] + v202.m128_i32[1];
              v202.m128_i32[2] = LODWORD(v210) + v202.m128_i32[1];
              n192_3 = LODWORD(v210) + v202.m128_i32[1];
              while ( 1 )
              {
                if ( n192_1 >= n192_4 )
                {
                  if ( n192_2 >= n192_3 )
                  {
                    n128_1 = LODWORD(v210);
                    n128 = v207.m128_i32[3];
                    n4 = HIDWORD(v209);
                    v21 = 0;
                    goto LABEL_19;
                  }
                  if ( n192_1 >= n192_4 )
                    goto LABEL_109;
                }
                if ( n192_1 < 192 )
                {
                  v202.m128_i32[2] = n192_3;
                  memset(buf_1,0,2048);
                  v148 = v226->f0 * (*((uint16_t *)v226 + 1) - 1);
                  v149 = *((uint16_t *)v226 + 2);
                  v205.m128_i32[3] = n192_2;
                  v203.m128_i32[0] = n192_1;
                  v203.m128_i32[1] = v148 - 1;
                  v205.m128_i32[0] = v26;
                  v204.m128_i32[1] = -v149;
                  v150 = -plane_count;
                  v204.m128_i32[0] = -v149 - plane_count;
                  v203.m128_i32[3] = v202.m128_i32[0] - v204.m128_i32[0] + 16;
                  v203.m128_i32[2] = v209 + v202.m128_i32[0] - v204.m128_i32[0] + 16;
                  v151 = (uint8_t *)v203.m128_i32[2];
                  v152 = HIDWORD(v208) + v202.m128_i32[0] - v204.m128_i32[0] + 16;
                  v153 = (uint8_t *)v203.m128_i32[3];
                  do
                  {
                    v154 = v153[v204.m128_i32[0]];
                    v203.m128_i64[1] = __PAIR64__((uint32_t)v153, (uint32_t)v151);
                    v204.m128_i32[2] = v152;
                    v155 = v154 + *v153 - v153[v204.m128_i32[1]] - v153[v150];
                    v156 = *v151;
                    v204.m128_i32[3] = v155;
                    v152 = v204.m128_i32[2] - v150;
                    v157 = v203.m128_i32[1];
                    v158 = (v204.m128_i16[6]
                          - (uint16_t)((v203.m128_i32[0]
                                              * (v151[v204.m128_i32[0]] + v156 - v151[v204.m128_i32[1]] - v151[v150])
                                              + LODWORD(v210)
                                              * (*(uint8_t *)(v204.m128_i32[2] + v204.m128_i32[0])
                                               + *(uint8_t *)v204.m128_i32[2]
                                               - *(uint8_t *)(v204.m128_i32[2] + v204.m128_i32[1])
                                               - (uint32_t)*(uint8_t *)(v204.m128_i32[2] + v150))
                                              + 40) >> 7)
                          - 256)
                         & 0x1FF;
                    ++*(uint32_t *)&buf_1[4 * v158];
                    v151 -= v150;
                    v153 = (uint8_t *)(v203.m128_i32[3] - v150);
                    v203.m128_i32[1] = v157 - 1;
                  }
                  while ( v157 != 1 );
                  n192_2 = v205.m128_i32[3];
                  n192_1 = v203.m128_i32[0];
                  v26 = v205.m128_i32[0];
                  v159 = __estimate_cost((char *)buf_1, 512);
                  n192_3 = v202.m128_i32[2];
                  n192_5 = v207.m128_i32[3];
                  if ( v159 < v26 )
                  {
                    v26 = v159;
                    n192_5 = n192_1;
                  }
                  v207.m128_i32[3] = n192_5;
                  n192_4 = v202.m128_i32[1] + n192_5;
                }
                if ( n192_2 < n192_3 )
                {
LABEL_109:
                  if ( n192_2 < 192 )
                  {
                    v202.m128_i32[3] = n192_4;
                    memset(buf_2,0,2048);
                    v161 = v226->f0 * (*((uint16_t *)v226 + 1) - 1);
                    v162 = *((uint16_t *)v226 + 2);
                    v205.m128_i32[3] = n192_2;
                    v203.m128_i32[0] = n192_1;
                    v206.m128_i32[0] = v161 - 1;
                    v205.m128_i32[0] = v26;
                    v207.m128_i32[0] = -v162;
                    v163 = -plane_count;
                    v206.m128_i32[3] = -v162 - plane_count;
                    v206.m128_i32[2] = v202.m128_i32[0] - v206.m128_i32[3] + 16;
                    v164 = (uint8_t *)(v209 + v202.m128_i32[0] - v206.m128_i32[3] + 16);
                    v206.m128_i32[1] = (int32_t)v164;
                    v165 = HIDWORD(v208) + v202.m128_i32[0] - v206.m128_i32[3] + 16;
                    v166 = (uint8_t *)v206.m128_i32[2];
                    do
                    {
                      v167 = v166[v206.m128_i32[3]];
                      *(int64_t *)((char *)v206.m128_i64 + 4) = __PAIR64__((uint32_t)v166, (uint32_t)v164);
                      v207.m128_i32[1] = v165;
                      v168 = v167 + *v166 - v166[v207.m128_i32[0]] - v166[v163];
                      v169 = *v164;
                      v207.m128_i32[2] = v168;
                      v165 = v207.m128_i32[1] - v163;
                      v170 = v206.m128_i32[0];
                      v171 = (v207.m128_i16[4]
                            - (uint16_t)((v207.m128_i32[3]
                                                * (v164[v206.m128_i32[3]] + v169 - v164[v207.m128_i32[0]] - v164[v163])
                                                + v205.m128_i32[3]
                                                * (*(uint8_t *)(v207.m128_i32[1] + v206.m128_i32[3])
                                                 + *(uint8_t *)v207.m128_i32[1]
                                                 - *(uint8_t *)(v207.m128_i32[1] + v207.m128_i32[0])
                                                 - (uint32_t)*(uint8_t *)(v207.m128_i32[1] + v163))
                                                + 40) >> 7)
                            - 256)
                           & 0x1FF;
                      ++*(uint32_t *)&buf_2[4 * v171];
                      v164 -= v163;
                      v166 = (uint8_t *)(v206.m128_i32[2] - v163);
                      v206.m128_i32[0] = v170 - 1;
                    }
                    while ( v170 != 1 );
                    n192_2 = v205.m128_i32[3];
                    n192_1 = v203.m128_i32[0];
                    v26 = v205.m128_i32[0];
                    v172 = __estimate_cost((char *)buf_2, 512);
                    n192_4 = v202.m128_i32[3];
                    n192_6 = LODWORD(v210);
                    if ( v172 < v26 )
                    {
                      v26 = v172;
                      n192_6 = n192_2;
                    }
                    LODWORD(v210) = n192_6;
                    n192_3 = v202.m128_i32[1] + n192_6;
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
            *(uint32_t *)buf_1 = v62;
            memset(buf_3,0,2048);
            v118 = v226->f0 * (*((uint16_t *)v226 + 1) - 1);
            v119 = *((uint16_t *)v226 + 2);
            v192 = v59;
            v184 = v57;
            v185 = v118 - 1;
            v205.m128_i32[0] = v26;
            v189 = -v119;
            v120 = -plane_count;
            v188 = -v119 - plane_count;
            v187 = (uint8_t *)(v202.m128_i32[0] - v188 + 16);
            v186 = (uint8_t *)(v209 + v202.m128_i32[0] - v188 + 16);
            v121 = v186;
            v122 = (uint8_t *)(HIDWORD(v208) + v202.m128_i32[0] - v188 + 16);
            v123 = v187;
            do
            {
              v124 = v123[v188];
              v186 = v121;
              v187 = v123;
              v190 = v122;
              v125 = v124 + *v123 - v123[v189] - v123[v120];
              v126 = *v121;
              v191 = v125;
              v122 = &v190[-v120];
              v127 = v185;
              v128 = ((uint16_t)v191
                    - (uint16_t)((v184 * (v121[v188] + v126 - v121[v189] - v121[v120])
                                        + LODWORD(v210) * (v190[v188] + *v190 - v190[v189] - (uint32_t)v190[v120])
                                        + 40) >> 7)
                    - 256)
                   & 0x1FF;
              ++*(uint32_t *)&buf_3[4 * v128];
              v121 -= v120;
              v123 = &v187[-v120];
              v185 = v127 - 1;
            }
            while ( v127 != 1 );
            v59 = v192;
            v57 = v184;
            v26 = v205.m128_i32[0];
            v129 = __estimate_cost((char *)buf_3, 512);
            v62 = *(uint32_t *)buf_1;
            v130 = v207.m128_i32[3];
            if ( v129 < v26 )
            {
              v26 = v129;
              v130 = v57;
            }
            v207.m128_i32[3] = v130;
            v61 = v130 - v202.m128_i32[1];
          }
          if ( v59 > v62 )
          {
LABEL_55:
            if ( v59 >= -64 )
            {
              v183 = v61;
              memset(buf_4,0,2048);
              v131 = v226->f0 * (*((uint16_t *)v226 + 1) - 1);
              v132 = *((uint16_t *)v226 + 2);
              v192 = v59;
              v184 = v57;
              v193 = v131 - 1;
              v205.m128_i32[0] = v26;
              v197 = -v132;
              v133 = -plane_count;
              v196 = -v132 - plane_count;
              v195 = (uint8_t *)(v202.m128_i32[0] - v196 + 16);
              v134 = (uint8_t *)(v209 + v202.m128_i32[0] - v196 + 16);
              v194 = v134;
              v135 = (uint8_t *)(HIDWORD(v208) + v202.m128_i32[0] - v196 + 16);
              v136 = v195;
              do
              {
                v137 = v136[v196];
                v194 = v134;
                v195 = v136;
                v198 = v135;
                v138 = v137 + *v136 - v136[v197] - v136[v133];
                v139 = *v134;
                v199 = v138;
                v135 = &v198[-v133];
                v140 = v193;
                v141 = ((uint16_t)v199
                      - (uint16_t)((v207.m128_i32[3] * (v134[v196] + v139 - v134[v197] - v134[v133])
                                          + v192 * (v198[v196] + *v198 - v198[v197] - (uint32_t)v198[v133])
                                          + 40) >> 7)
                      - 256)
                     & 0x1FF;
                ++*(uint32_t *)&buf_4[4 * v141];
                v134 -= v133;
                v136 = &v195[-v133];
                v193 = v140 - 1;
              }
              while ( v140 != 1 );
              v59 = v192;
              v57 = v184;
              v26 = v205.m128_i32[0];
              v142 = __estimate_cost((char *)buf_4, 512);
              v61 = v183;
              v143 = LODWORD(v210);
              if ( v142 < v26 )
              {
                v26 = v142;
                v143 = v59;
              }
              LODWORD(v210) = v143;
              v62 = v143 - v202.m128_i32[1];
            }
          }
          --v57;
          --v59;
        }
      }
LABEL_19:
      v29 = &((uint8_t *)v226)[v208 + 16 + n4 + v205.m128_i32[2]];
      if ( (uint32_t)v29 < v227 )
      {
        LODWORD(v210) = n128_1;
        v207.m128_i32[3] = n128;
        v205.m128_i32[0] = v26;
        HIDWORD(v209) = n4;
        do
        {
          v30 = v29[HIDWORD(v208)];
          v31 = v30 * LODWORD(v210);
          v32 = v29[v209];
          v33 = v32 * v207.m128_i32[3];
          ++v180[v30 - v32 + 1280];
          v34 = *v29 + 512;
          v29 += HIDWORD(v209);
          v35 = ((uint16_t)v34 - (uint16_t)((uint32_t)(v31 + v33 + 40) >> 7)) & 0x3FF;
          ++*(uint32_t *)&buf[4 * v35];
          ++v178[v34 - v32];
          ++v179[v34 - v30];
          v36 = v34 - ((uint32_t)(((v32 + v30) << 6) + 40) >> 7);
          ++v180[v36];
        }
        while ( (uint32_t)v29 < v227 );
        n128_1 = LODWORD(v210);
        n128 = v207.m128_i32[3];
        v26 = v205.m128_i32[0];
        n4 = HIDWORD(v209);
      }
      n0x4000 = v26 >> 7;
      if ( v26 >> 7 >= 0x4000 )
        n0x4000 = 0x4000;
      v38 = n0x4000 + v26;
      v39 = *(int32_t *)((char *)&v214[3] + v205.m128_i32[1]);
      n2_4 = v39 < v38;
      if ( v39 < v38 )
      {
        v38 = *(int32_t *)((char *)&v214[3] + v205.m128_i32[1]);
        n128 = 128;
        n128_1 = 0;
      }
      n2_1 = n2_4;
      if ( *(uint32_t *)((char *)v215 + v205.m128_i32[1]) < v38 )
      {
        v38 = *(uint32_t *)((char *)v215 + v205.m128_i32[1]);
        n2_1 = 2;
        n128 = 0;
        n128_1 = 128;
      }
      v42 = v38 <= *(uint32_t *)((char *)v215 + v205.m128_i32[1] + 4);
      if ( v38 > *(uint32_t *)((char *)v215 + v205.m128_i32[1] + 4) )
        n2_1 = 3;
      v43 = v205.m128_i32[1];
      if ( !v42 )
      {
        n128 = 64;
        n128_1 = 64;
      }
      *(int32_t *)((char *)__dword_4433A0 + v205.m128_i32[1]) = n128;
      *(int32_t *)((char *)__dword_4433A4 + v43) = n128_1;
      v44 = &buf[4096 * n2_1];
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
        HIDWORD(v209) = n4;
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
        n4 = HIDWORD(v209);
      }
      __byte_44339F[v205.m128_i32[1]] = n0x100_1 + 1;
      // Same window, over the second table.  `i` is left at 256 for the slide
      // that follows.
      j_1 = 0;
      for ( i = 0; i < 0x100; ++i )
        j_1 += v180[i + 1024];
      n255 = 255;
      for ( j = j_1; i < 512; ++i )
      {
        j = v180[i + 1024] + j - v180[i + 768];
        if ( j >= j_1 )
        {
          n255 = i;
          j_1 = j;
        }
      }
      n192 = n255 + 1;
      plane_desc[HIDWORD(v208) + v205.m128_i32[1] + 1].b3 = n192;
      if ( n4 >= 4 )
      {
        v202 = v21;
        v203 = v21;
        v204 = v21;
        v205 = v21;
        v206 = v21;
        v207 = v21;
        memset(buf,0,0x8000);
        v223 = &((uint8_t *)v226)[v208];
        v63 = (int32_t)(v227 - 17 - (uint32_t)&((uint8_t *)v226)[v208]) / 4;
        v64 = &((uint8_t *)v226)[v208 + 20];
        if ( v227 <= (uint32_t)v64 )
        {
          v72 = *(double *)&v202.m128_i64[1];
          v73 = *(double *)v203.m128_i64;
          v74 = *(double *)&v204.m128_i64[1];
          v209 = v202.m128_i64[1];
          v208 = v204.m128_i64[1];
          v216 = *(double *)v202.m128_i64;
          v220 = v206.m128_i64[1];
          v75 = *(double *)v203.m128_i64;
        }
        else
        {
          v65 = *(double *)v206.m128_i64;
          v66 = *(double *)v204.m128_i64;
          v220 = v206.m128_i64[1];
          v216 = *(double *)v202.m128_i64;
          v221 = *(double *)&v202.m128_i64[1];
          v212 = *(double *)v203.m128_i64;
          v222 = *(double *)&v204.m128_i64[1];
          v213 = *(double *)v207.m128_i64;
          v211 = *(double *)&v207.m128_i64[1];
          LODWORD(v210) = (uintptr_t)&((uint8_t *)v226)[v208 + 20];
          v67 = 0;
          v224 = (int32_t)(v227 - 17 - (uint32_t)&((uint8_t *)v226)[v208]) / 4;
          do
          {
            v68 = (double)(((uint8_t *)v226)[4 * v67 + 16] + v223[4 * v67 + 20] - (((uint8_t *)v226)[4 * v67 + 20] + v223[4 * v67 + 16]));
            v69 = (double)(((uint8_t *)v226)[4 * v67 + 17] + v223[4 * v67 + 21] - (((uint8_t *)v226)[4 * v67 + 21] + v223[4 * v67 + 17]));
            v70 = (double)(((uint8_t *)v226)[4 * v67 + 18] + v223[4 * v67 + 22] - (((uint8_t *)v226)[4 * v67 + 22] + v223[4 * v67 + 18]));
            v71 = ((uint8_t *)v226)[4 * v67 + 19] + v223[4 * v67 + 23] - (((uint8_t *)v226)[4 * v67 + 23] + v223[4 * v67 + 19]);
            v216 = v216 + v68 * v68;
            ++v67;
            v221 = v221 + v68 * v69;
            v212 = v212 + v68 * v70;
            v66 = v66 + v69 * v69;
            v222 = v222 + v69 * v70;
            v65 = v65 + v70 * v70;
            *(double *)&v220 = *(double *)&v220 + v68 * (double)v71;
            v213 = v213 + v69 * (double)v71;
            v211 = v211 + v70 * (double)v71;
          }
          while ( v67 < v224 );
          v72 = v221;
          v73 = v212;
          v74 = v222;
          v64 = (uint8_t *)LODWORD(v210);
          v63 = v224;
          *(double *)&v207.m128_i64[1] = v211;
          *(double *)v207.m128_i64 = v213;
          v206.m128_i64[1] = v220;
          *(double *)v206.m128_i64 = v65;
          *(double *)&v204.m128_i64[1] = v222;
          *(double *)v204.m128_i64 = v66;
          *(double *)v203.m128_i64 = v212;
          *(double *)&v202.m128_i64[1] = v221;
          *(double *)v202.m128_i64 = v216;
          v75 = v212;
          *(double *)&v208 = v222;
          *(double *)&v209 = v221;
        }
        v221 = v72;
        v222 = v74;
        v210 = v75;
        *(double *)&v203.m128_i64[1] = v72;
        *(double *)v205.m128_i64 = v73;
        *(double *)&v205.m128_i64[1] = v74;
        v211 = v216 * *(double *)v206.m128_i64 - v75 * v73;
        v212 = 0.0 - v216 * v74 + v73 * *(double *)&v209;
        v213 = v75 * v74 - *(double *)&v209 * *(double *)v206.m128_i64;
        v76 = 128.0 / (*(double *)v204.m128_i64 * v211 + *(double *)&v208 * v212 + v72 * v213 + 0.1);
        *(double *)v214 = v76;
        __choose_plane_coding_n191 = (int32_t)(((v73 * *(double *)&v208 - v72 * *(double *)v206.m128_i64) * *(double *)v207.m128_i64
                    + (0.0 - v73 * *(double *)v204.m128_i64 + v72 * v222) * *(double *)&v207.m128_i64[1]
                    + (*(double *)v206.m128_i64 * *(double *)v204.m128_i64 - v222 * *(double *)&v208)
                    * *(double *)&v220)
                   * v76);
        if ( __choose_plane_coding_n191 >= 191 )
          __choose_plane_coding_n191 = 191;
        if ( __choose_plane_coding_n191 < -64 )
          __choose_plane_coding_n191 = -64;
        __choose_plane_coding_n191_1 = __choose_plane_coding_n191;
        n191_2 = (int32_t)((v211 * *(double *)v207.m128_i64 + v212 * *(double *)&v207.m128_i64[1]
                                                         + v213 * *(double *)&v220)
                     * v76);
        if ( n191_2 >= 191 )
          n191_2 = 191;
        if ( n191_2 < -64 )
          n191_2 = -64;
        n191_3 = (int32_t)(*(double *)v214
                     * ((v216 * *(double *)&v207.m128_i64[1] - v210 * *(double *)&v220) * *(double *)v204.m128_i64
                      + (0.0 - v216 * *(double *)v207.m128_i64 + *(double *)&v220 * *(double *)&v209)
                      * *(double *)&v208
                      + (v210 * *(double *)v207.m128_i64 - *(double *)&v209 * *(double *)&v207.m128_i64[1]) * v221));
        if ( n191_3 >= 191 )
          n191_3 = 191;
        if ( n191_3 < -64 )
          n191_3 = -64;
        if ( (uint32_t)v64 < v227 )
        {
          v202.m128_i64[0] = __PAIR64__(n191_3, n191_2);
          v224 = v63;
          v80 = 0;
          do
          {
            v81 = ((uint8_t *)v226)[4 * v80 + 20];
            v82 = ((uint8_t *)v226)[4 * v80 + 16] + v223[4 * v80 + 20];
            v83 = v223[4 * v80 + 16];
            v202.m128_i32[2] = v80;
            v84 = v81 + v83;
            v85 = v223[4 * v80 + 21];
            v86 = v82 - v84;
            v87 = v223[4 * v80 + 17];
            v202.m128_i32[3] = v86;
            v88 = ((uint8_t *)v226)[4 * v80 + 22];
            v89 = ((uint8_t *)v226)[4 * v80 + 17] + v85 - (((uint8_t *)v226)[4 * v80 + 21] + v87);
            v90 = v223[4 * v80 + 22];
            v203.m128_i32[0] = v89;
            v91 = ((uint8_t *)v226)[4 * v80 + 18] + v90 - (v88 + v223[4 * v80 + 18]);
            LOWORD(v89) = ((uint8_t *)v226)[4 * v80 + 19] + v223[4 * v80 + 23] - v223[4 * v80 + 19] - ((uint8_t *)v226)[4 * v80 + 23];
            v92 = v203.m128_i16[0];
            LOWORD(v89) = v89 - 512;
            v93 = v203.m128_i32[0] * v202.m128_i32[0] + v202.m128_i32[3] * __choose_plane_coding_n191_1;
            v94 = v91 * v202.m128_i32[1];
            ++v178[((uint16_t)v89 - v202.m128_i16[6]) & 0x3FF];
            v95 = ((uint16_t)v89 - (uint16_t)((uint32_t)(v93 + v94 + 63) >> 7)) & 0x3FF;
            ++*(uint32_t *)&buf[4 * v95];
            ++v179[((uint16_t)v89 - v92) & 0x3FF];
            LOWORD(v89) = v89 - v91;
            v96 = v202.m128_i32[2];
            v97 = v223;
            ++v180[v89 & 0x3FF];
            v98 = v97[4 * v96 + 23] + 256;
            v99 = ((uint16_t)v98
                 - (uint16_t)((v202.m128_i32[0] * v97[4 * v96 + 21]
                                     + __choose_plane_coding_n191_1 * v97[4 * v96 + 20]
                                     + v202.m128_i32[1] * (uint32_t)v97[4 * v96 + 22]
                                     + 63) >> 7)
                 + 256)
                & 0x3FF;
            ++v180[v99 + 1024];
            v100 = v98 - v97[4 * v96 + 20];
            ++v180[v100 + 2048];
            v101 = v98 - v97[4 * v96 + 21];
            ++v180[v101 + 3072];
            v102 = v98 - v97[4 * v96 + 22];
            ++v180[v102 + 4096];
            v80 = v96 + 1;
          }
          while ( v80 < v224 );
          n191_3 = v202.m128_i32[1];
          n191_2 = v202.m128_i32[0];
        }
        v103 = __estimate_cost((char *)buf, 1024);
        v104 = (v103 >> 7) + v103;
        v105 = __estimate_cost((char *)v178, 1024);
        v106 = v105 < v104;
        if ( v105 < v104 )
          v104 = v105;
        n191_4 = __choose_plane_coding_n191_1;
        if ( v106 )
        {
          n191_4 = 128;
          n191_3 = 0;
          n191_2 = 0;
        }
        __choose_plane_coding_n191_1 = n191_4;
        v202.m128_i32[0] = v106;
        v108 = __estimate_cost((char *)v179, 1024);
        n2_2 = v202.m128_i32[0];
        if ( v108 < v104 )
        {
          v104 = v108;
          n2_2 = 2;
          n191_2 = 128;
          n191_3 = 0;
          __choose_plane_coding_n191_1 = 0;
        }
        v202.m128_i32[0] = n2_2;
        v110 = __estimate_cost((char *)v180, 1024);
        __choose_plane_coding_n3_1 = v202.m128_i32[0];
        if ( v110 < v104 )
        {
          __choose_plane_coding_n3_1 = 3;
          n191_3 = 128;
          n191_2 = 0;
          __choose_plane_coding_n191_1 = 0;
        }
        ::__n191 = __choose_plane_coding_n191_1;
        __n191_0 = n191_2;
        ::__n191_1 = n191_3;
        __n3_0 = 3;
        ::__n3_1 = 3;
        v112 = &v180[1024 * __choose_plane_coding_n3_1 + 1024];
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
        __byte_4433CF = k + 1;
      }
    }
  }
  return n192;
}

int32_t *__read_bmp(char *FileName)
{
  struct alignas(16) {   // 128 bytes, the frame Hex-Rays could not name
      uint8_t slot0[4];
      uint8_t slot4[4];
      int32_t *v52;
      uint8_t slot12[4];
      void *Buffer_3;
      uint8_t _pad0[4];
      char *Src;
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
      uint8_t slot86[4];
      uint8_t _pad4[38];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 128, "frame layout moved");
  uint32_t &Size_4 = *(uint32_t *)((char *)__frame.slot0);
  int32_t &v46 = *(int32_t *)((char *)__frame.slot0);
  int32_t &v47 = *(int32_t *)((char *)__frame.slot0);
  int32_t &Offset_1 = *(int32_t *)((char *)__frame.slot0);
  int32_t &Size = *(int32_t *)((char *)__frame.slot4);
  uint32_t &Sizea = *(uint32_t *)((char *)__frame.slot4);
  char &Sizeb = *((char *)__frame.slot4);
  BmfImage *&v52 = (BmfImage *&)__frame.v52;
  int32_t &Src_2 = *(int32_t *)((char *)__frame.slot12);
  int32_t &v54 = *(int32_t *)((char *)__frame.slot12);
  void *&Buffer_3 = __frame.Buffer_3;
  char *&Src = __frame.Src;
  uint8_t (&bmp_bgra)[4] = __frame.bmp_bgra;
  uint32_t (&bmp_info_hdr)[2] = __frame.bmp_info_hdr;
  int32_t &bmp_height = __frame.bmp_height;
  int16_t &bmp_planes = __frame.bmp_planes;
  uint16_t &bmp_bits = __frame.bmp_bits;
  int32_t &bmp_compression = __frame.bmp_compression;
  int32_t &bmp_clr_used = __frame.bmp_clr_used;
  int16_t (&bmp_file_hdr)[5] = __frame.bmp_file_hdr;
  int32_t &bmp_off_bits = *(int32_t *)((char *)__frame.slot86);
  ;
  uintptr_t Src_1;   // were int32_t: addresses, masked and tagged
  char *v7, *v8, *v9;   // were int32_t: these hold addresses
  FILE *Stream_v;
  char v25, *Src_4, v28, v30, *Src_3, *Src_6, *Buffer_4, *Src_5;
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
    || fread(bmp_file_hdr, 0xEu, 1u, Stream_v) != 1
    || bmp_file_hdr[0] != 0x4D42 /* 'BM' */
    || fread(bmp_info_hdr, 0x28u, 1u, Stream_v) != 1
    || bmp_info_hdr[0] != 40
    || bmp_planes != 1 )
  {
    return nullptr;
  }
  v3 = (BmfImage *)(__alloc_image(bmp_info_hdr[1], bmp_height, bmp_bits, bmp_bits <= 8u, 1));
  Size_2 = (v3->stride + 3) & 0xFFFFFFFC;
  if ( bmp_bits <= 8u )
  {
    Size_1 = 1 << (bmp_bits & 31);
    if ( bmp_clr_used )
      Size_1 = bmp_clr_used;
    if ( Size_1 > 0 )
    {
      Size_4 = (v3->stride + 3) & 0xFFFFFFFC;
      Size = Size_1;
      for ( i = 0; i < Size; ++i )
      {
        fread(bmp_bgra, 4u, 1u, Stream_v);
        if ( (v3->depth & 0x80) != 0 )
          v7 = (char *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v7 = 0;
        *(uint8_t *)(v7 + 3 * i + 2) = bmp_bgra[2];
        if ( (v3->depth & 0x80) != 0 )
          v8 = (char *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v8 = 0;
        *(uint8_t *)(v8 + 3 * i + 1) = bmp_bgra[1];
        if ( (v3->depth & 0x80) != 0 )
          v9 = (char *)(uintptr_t)v3 + v3->data_size + 16;
        else
          v9 = 0;
        *(uint8_t *)(v9 + 3 * i) = bmp_bgra[0];
      }
      Size_2 = Size_4;
    }
  }
  Buffer_3 = bmf_new(Size_2);
  Src = (char *)v3 + v3->data_size - v3->stride + 16;
  fseek(Stream_v, bmp_off_bits, 0);
  if ( bmp_compression )
  {
    if ( bmp_compression == 1 )
    {
      memset((char *)v3 + 16,0,v3->data_size);
      Src_1 = (int32_t)Src;
      v52 = v3;
      v46 = v3->height - 1;
      while ( 1 )
      {
        Src_2 = Src_1;
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
          __builtin_memset((void *)Src_2, Sizea, j_3);
          Src_1 = Src_2 + j_3;
        }
        else if ( Sizea_1 )
        {
          if ( Sizea_1 == 1 )
            goto LABEL_61;
          if ( Sizea_1 == 2 )
          {
            v38 = fgetc(Stream_v);
            Src_1 = v38 + Src_1 - fgetc(Stream_v) * *((uint16_t *)v52 + 2);
          }
          else
          {
            fread(Buffer_3, (Sizea_1 + 1) & 0xFFFFFFFE, 1u, Stream_v);
            memcpy((char *)Src_1,(char *)Buffer_3,Sizea);
            Src_1 += Sizea;
          }
        }
        else
        {
          if ( --v46 < 0 )
            goto LABEL_61;
          Src_1 = (int32_t)v52 + v46 * *((uint16_t *)v52 + 2) + 16;
        }
      }
    }
    if ( bmp_compression != 2 )
      return nullptr;
    memset((char *)v3 + 16,0,v3->data_size);
    v52 = v3;
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
            Src_3 = Src;
            while ( v31 != 1 )
            {
              *Src_3++ = n2_2;
              v31 -= 2;
              if ( !v31 )
              {
                Src = Src_3;
                v22 = 1;
                goto LABEL_44;
              }
            }
            Src = Src_3;
            *Src_3 = n2_2 & 0xF0;
            v22 = 0;
          }
          else
          {
            v26 = v54;
            Src_4 = Src;
            v28 = *Src;
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
                Src = Src_4;
                *Src_4 = v30;
                v22 = 0;
                goto LABEL_44;
              }
            }
            Src = Src_4;
            v22 = 1;
          }
        }
        if ( n2_1 )
          break;
        if ( --v47 < 0 )
          goto LABEL_61;
        Src = (char *)v52 + v47 * *((uint16_t *)v52 + 2) + 16;
      }
      if ( n2_1 == 1 )
        goto LABEL_61;
      if ( n2_1 != 2 )
        break;
      v40 = fgetc(Stream_v);
      v41 = (v40 >> 1) - fgetc(Stream_v) * *((uint16_t *)v52 + 2);
      if ( (v40 & 1) == 1 )
      {
        if ( !v22 )
          ++v41;
        v22 = !v22;
      }
      Src += v41;
    }
    fread(Buffer_3, (((n2_1 + 1) >> 1) + 1) & 0xFFFFFFFE, 1u, Stream_v);
    Buffer_4 = (char *)Buffer_3;
    Src_5 = Src;
    while ( 1 )
    {
      Sizeb = *Buffer_4;
      if ( v22 )
      {
        v44 = n2_2 - 1;
        if ( !v44 )
        {
          Src = Src_5;
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
          Src = Src_5;
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
        Src = Src_5;
        goto LABEL_44;
      }
    }
  }
  ElementCount = v3->stride;
  Offset_2 = Size_2 - ElementCount;
  if ( bmp_height - 1 >= 0 )
  {
    Offset_1 = Offset_2;
    v35 = bmp_height - 1;
    v52 = v3;
    Src_6 = Src;
    while ( 1 )
    {
      ElementCount_1 = fread(Src_6, 1u, ElementCount, Stream_v);
      ElementCount = *((uint16_t *)v52 + 2);
      if ( ElementCount_1 != ElementCount )
        return nullptr;
      if ( Offset_1 )
      {
        fseek(Stream_v, Offset_1, 1);
        ElementCount = *((uint16_t *)v52 + 2);
      }
      Src_6 -= ElementCount;
      if ( --v35 < 0 )
      {
LABEL_61:
        v3 = v52;
        break;
      }
    }
  }
  fclose(Stream_v);
  free(Buffer_3);
  return (int32_t *)v3;
}

int32_t __decode_symbol_list(uint32_t *a1)
{
  struct alignas(16) {   // 32824 bytes, the frame Hex-Rays could not name
      uint16_t *list[8192];   // the symbol list: 8 named slots and 32736 bytes of tail, one array
      uint32_t n0x7F800000_1;
      int32_t tot;
      int32_t v65;
      uint16_t *v66;
      uint32_t *v67;
      uint32_t *v68;
      uint8_t _pad1[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 32832, "frame layout moved");
  uint16_t * &v55 = (uint16_t * &)__frame.list[0];
  uint16_t * &v56 = (uint16_t * &)__frame.list[1];
  int32_t &v57 = (int32_t &)__frame.list[2];
  int32_t &v58 = (int32_t &)__frame.list[3];
  int32_t &v59 = (int32_t &)__frame.list[4];
  uint32_t &n0x800000_1 = (uint32_t &)__frame.list[5];
  int32_t &v62 = (int32_t &)__frame.list[7];
  int32_t &tot = __frame.tot;
  int32_t &v65 = __frame.v65;
  uint16_t *&v66 = __frame.v66;
  uint32_t *&v67 = __frame.v67;
  uint32_t *&v68 = __frame.v68;
  ;
  char *v3, *v7;   // were int32_t: these hold addresses
  Obj32 *v38;
  char v23, v34, *v36, v40;
  int16_t v39;
  int32_t sym_cum, sym_high, v2, v5, v6, v8, n0x2000_5, n0x2000_2, n251, v46,
          v49, v51, v53;
  Obj33 *v47;
  uint16_t **v4, *v20, **v21, *v25, **v26, *v33, v35, *v43, *v44, *v45, *v48, *v52;
  uint32_t *v9, __decode_symbol_list_n0x800000, n0x2000_6, n0x2000_4,
           n0x2000_3, tot_1, *v32, v41, v42, v50,
           v54;
  void *v1;
  v68 = a1;
  v1 = alloca(32788);
  v2 = a1[1];
  v3 = (char *)a1[5];
  v4 = __frame.list;
  v68 = (uint32_t *)(uint8_t)__byte_445700;
  v67 = a1;
  v5 = 0;
  v6 = 0;
  do
  {
    if ( (uint32_t *)(uint8_t)exclusion_mask[*(uint16_t *)(v3 + 3 * v6)] == v68 )
    {
      v8 = 0;
    }
    else
    {
      v7 = v3 + 3 * v6;
      v8 = *(uint8_t *)(v7 + 2);
      *v4++ = (uint16_t *)v7;
    }
    v5 += v8;
    ++v6;
  }
  while ( v6 < v2 );
  v9 = v67;
  if ( !v5 )
    return -1;
  *v4 = nullptr;
  v65 = v9[2];
  n0x2000_6 = v5 + v65;
  n0x2000_4 = rc.get_freq(n0x2000_6);
  tot = v5 + v65;
  v67 = v9;
  n0x2000_5 = v5 + v65 - 1;
  v20 = v55;
  v21 = &__frame.list[1];
  v66 = v55;
  n0x2000_2 = 0;
  while ( 1 )
  {
    n0x2000_2 += *((uint8_t *)v20 + 2);
    if ( n0x2000_2 > (int32_t)n0x2000_4 )
      break;
    v20 = *v21++;
    if ( !v20 )
    {
      v23 = (char)(uintptr_t)v68;
      sym_cum = n0x2000_2;
      n0x2000_3 = tot;
      sym_high = tot;
      v25 = v66;
      v26 = &__frame.list[1];
      do
      {
        exclusion_mask[*v25] = v23;
        v25 = *v26++;
      }
      while ( v25 );
      tot_1 = tot;
      v62 = -1;
      goto LABEL_19;
    }
  }
  v32 = v67;
  sym_high = n0x2000_2;
  sym_cum = n0x2000_2 - *((uint8_t *)v20 + 2);
  v62 = *v20;
  *((uint8_t *)v20 + 2) += 4;
  v33 = (uint16_t *)v32[5];
  v32[3] += 4;
  if ( v20 == v33 )
  {
    n251 = *((uint8_t *)v20 + 2);
  }
  else
  {
    v34 = *((uint8_t *)v20 + 2);
    v35 = *v20;
    v36 = (char *)v20 - 3;
    *v20 = *(uint16_t *)((char *)v20 - 3);
    *((uint8_t *)v20 + 2) = *((uint8_t *)v20 - 1);
    *(uint16_t *)v36 = v35;
    v36[2] = v34;
    v33 = (uint16_t *)v32[5];
    if ( (uint16_t *)((char *)v20 - 3) == v33 )
    {
      n251 = *((uint8_t *)v20 - 1);
    }
    else
    {
      n0x800000_1 = __decode_symbol_list_n0x800000;
      while ( 1 )
      {
        n251 = (uint8_t)v36[2];
        v38 = (Obj32 *)(v36 - 3);
        if ( n251 <= (uint8_t)*(v36 - 1) )
          break;
        v39 = *(uint16_t *)v36;
        v40 = v36[2];
        *(uint16_t *)v36 = v38->f0;
        v36[2] = *(v36 - 1);
        v38->f0 = v39;
        *(v36 - 1) = v40;
        v33 = (uint16_t *)v32[5];
        v36 -= 3;
        if ((char *)v38 == (char *)v33 )
        {
          __decode_symbol_list_n0x800000 = n0x800000_1;
          n251 = (uint8_t)v38->f2;
          goto LABEL_30;
        }
      }
      __decode_symbol_list_n0x800000 = n0x800000_1;
    }
  }
LABEL_30:
  v41 = v32[4];
  if ( n251 > 251 || v41 < v32[3] )
  {
    v55 = (uint16_t *)v32[1];
    v42 = 20 * *v32;
    v43 = v55;
    n0x800000_1 = __decode_symbol_list_n0x800000;
    v59 = v41 < v42;
    v44 = (uint16_t *)((char *)v33 - 3);
    do
    {
      v45 = v44;
      v44 = (uint16_t *)((char *)v44 + 3);
      v46 = (v59 + (uint32_t)*((uint8_t *)v44 + 2)) >> 1;
      *((uint8_t *)v44 + 2) = v46;
      if ( v44 != (uint16_t *)v32[5] )
      {
        v47 = (Obj33 *)((uint16_t *)((char *)v44 - 3));
        v58 = *((uint8_t *)v44 - 1);
        if ( v46 > v58 )
        {
          v57 = *v44;
          *v44 = v47->f0;
          *((uint8_t *)v44 + 2) = v58;
          if ((uint16_t *)v47 != (uint16_t *)v32[5] )
          {
            v56 = v44;
            v55 = v43;
            do
            {
              v48 = (uint16_t *)((char *)v47 - 3);
              v49 = *((uint8_t *)v47 - 1);
              if ( v46 <= v49 )
                break;
              v47->f0 = *v48;
              *((uint8_t *)v47 + 2) = v49;
              v47 = (Obj33 *)((uint16_t *)((char *)v47 - 3));
            }
            while ( v48 != (uint16_t *)v32[5] );
            v44 = v56;
            v43 = v55;
          }
          v47->f0 = v57;
          *((uint8_t *)v47 + 2) = v46;
        }
      }
      v43 = (uint16_t *)((uintptr_t)v43 - (1));
    }
    while ( v43 );
    __decode_symbol_list_n0x800000 = n0x800000_1;
    v50 = v32[2];
    v51 = *((uint8_t *)v44 + 2);
    v55 = nullptr;
    if ( !v51 )
    {
      v52 = v55;
      do
      {
        v52 = (uint16_t *)((char *)v52 + 1);
        v32[2] = ++v50;
        v53 = *((uint8_t *)v45 + 2);
        v45 = (uint16_t *)((char *)v45 - 3);
      }
      while ( !v53 );
      v55 = v52;
      v32[1] -= (uint32_t)v52;
    }
    v54 = v32[3];
    v32[2] = v50 - (v50 >> 1);
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
    tot_1 = n0x2000_6;
    v32[3] = v54 - (v54 >> 1);
  }
  else
  {
    tot_1 = n0x2000_6;
    n0x2000_2 = sym_cum;
    n0x2000_3 = sym_high;
  }
LABEL_19:
  rc.decode(n0x2000_2, n0x2000_3, tot_1);
  return v62;
}
static inline int32_t __fwd_decode_pixel_decode_context_bit(void *a0, void *a1) { return __decode_context_bit((uint16_t *)a0, (uint16_t *)a1); }
static inline int32_t __fwd_decode_pixel_decode_encode_symbol_list(void *a0) { return __decode_symbol_list((uint32_t *)a0); }
static inline int32_t __fwd_decode_pixel_rescale_counter_pair(void *a0) { return __rescale_counter_pair((uint16_t *)a0); }
static inline int32_t __fwd_decode_pixel_pixel_context(void *a0, void *a1) { return __pixel_context((ModelBlock *)a0, (uint32_t *)a1); }

int32_t __decode_pixel(ModelBlock *_this, int32_t a2)
{
  struct alignas(16) {   // 164 bytes, the frame Hex-Rays could not name
      uint32_t  sym[32];   // pixel_context reads sym[0..31]; `f44` runs 0..31 (was n15_8 .. v211)
      uint8_t   _gap0[4];   // was int32_t n15_24
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t &n15_8 = (int32_t &)__frame.sym[0];
  int32_t &n4_1 = (int32_t &)__frame.sym[1];
  int32_t &freq_i = (int32_t &)__frame.sym[2];
  int32_t &n15_3 = (int32_t &)__frame.sym[3];
  Obj18 * &v184 = (Obj18 * &)__frame.sym[4];
  ModelBlock * &this_1 = (ModelBlock * &)__frame.sym[5];
  int32_t &v186 = (int32_t &)__frame.sym[6];
  int32_t &v187 = (int32_t &)__frame.sym[7];
  int32_t &v188 = (int32_t &)__frame.sym[8];
  int32_t &v189 = (int32_t &)__frame.sym[9];
  int32_t &v190 = (int32_t &)__frame.sym[10];
  int32_t &v191 = (int32_t &)__frame.sym[11];
  int32_t &v192 = (int32_t &)__frame.sym[12];
  int32_t &v193 = (int32_t &)__frame.sym[13];
  int32_t &v194 = (int32_t &)__frame.sym[14];
  int32_t &v195 = (int32_t &)__frame.sym[15];
  int32_t &v196 = (int32_t &)__frame.sym[16];
  int32_t &v197 = (int32_t &)__frame.sym[17];
  int32_t &v198 = (int32_t &)__frame.sym[18];
  int32_t &v199 = (int32_t &)__frame.sym[19];
  int32_t &v200 = (int32_t &)__frame.sym[20];
  int32_t &v201 = (int32_t &)__frame.sym[21];
  int32_t &v202 = (int32_t &)__frame.sym[22];
  int32_t &v203 = (int32_t &)__frame.sym[23];
  int32_t &v204 = (int32_t &)__frame.sym[24];
  int32_t &v205 = (int32_t &)__frame.sym[25];
  int32_t &v206 = (int32_t &)__frame.sym[26];
  int32_t &v207 = (int32_t &)__frame.sym[27];
  int32_t &v208 = (int32_t &)__frame.sym[28];
  int32_t &v209 = (int32_t &)__frame.sym[29];
  int32_t &v210 = (int32_t &)__frame.sym[30];
  int32_t &v211 = (int32_t &)__frame.sym[31];
  int32_t n15_24;
  ;
  Obj18 *v80;
  Obj24 *v36;
  uint8_t *v21, *v23, *v57;   // row cursors out of ModelBlock
  char *v46, *v66, *n15_17;
  uint8_t *v53, *v54, *v55, *v61, *v108, *n15_10;   // row cursors out of ModelBlock
  bool v19;
  char v70, v71, v72, v73, v74, v75, v91, *v157;
  int16_t v14, n4_14, v146, v160, n15_4;
  ModelBlock *this_4;
  uint8_t *v25, *v56;   // row cursors out of f56[9]; were int32_t
  int32_t arg_cum, arg_high, arg_tot, n4_8, n4_7, n15_6, n15_7, v8, v9, v10, v12, v13, v15,
          n4_9, __decode_pixel_n15, v22, n4_11, v26, v27, v29, v31, n0xFFFF, v33, n0xFFFF_1,
          n53248, n4_12, v40, n15_11, n8, n15_12, v44, n4_22, n15_14, n15_18, v49, v50, v51,
          n4_13, *v59, v60, n15_13, v67, n15_15, n4_17, v81, v83, v84,
          v85, n4, n256_2, n15_1, n256_1, n15_23, n4_19, n4_20, n4_5, n4_6, n15_5, v102, v103,
          v104, v105, v107, v109, v111, v112, v113, v114, v115, v116, v117, v118, v119,
          v120, v122, v123, v124, v125, v126, v127, v128, n32, n15_25, n4_21, v132, v133, v135,
          n256, v143, n4_18, n15_19, n15_20, v148, v149, v150, v151, v152, v153, v155, n15_21,
          v158, v159, v161, v163, v164, n4_2, n15_22, n256_4, n256_5, n256_3, n15_2;
  Obj18 *v170;
  Obj18 *v100;
  Obj18 *v171;
  Obj18 *v4;
  Obj18 *v90;
  uint16_t *v97;
  Obj121 *v106;
  uint16_t *n15_9, *v16, *n4_10, *v58, *freq_tbl, *v95, *v110, *v121, v154, v162,
           v174;
  ModelBlock *this_3;
  ModelBlock *this_2;
  uint32_t bin_tot, n4_16, n4_15, **v134, v136, v137, v138, v139, v140, v172, v173, v175, v176,
           v177;
  Obj18 *v142;
  Obj118 *v39;
  Obj118 *v38;
  uint8_t *v76;
  uint8_t *n15_16;
  n15_9 = (uint16_t *)*((uint32_t *)_this + 20);
  n4_8 = *n15_9;
  v4 = (Obj18 *)((uint16_t *)*((uint32_t *)_this + 19));
  n4_7 = *((uint16_t *)v4 - 4);
  n15_6 = n15_9[4];
  this_1 = (ModelBlock *)(_this);
  n15_7 = *(n15_9 - 4);
  n15_8 = n4_8;
  ::tbl44573C[1] = n4_8;
  n4_1 = n4_7;
  ::tbl44573C[2] = n4_7;
  n15_3 = n15_6;
  ::tbl44573C[3] = n15_6;
  v8 = *((uint8_t *)n15_9 + 3) + 4 * (n15_6 == n15_7);
  v9 = *((uint8_t *)n15_9 + 11);
  freq_i = n15_7;
  tbl44573C[4] = n15_7;
  v10 = 2 * *((uint8_t *)v4 - 6) + 8 * v9 + v8;
  this_2 = (ModelBlock *)(this_1);
  v12 = 32 * *((uint8_t *)v4 - 4) + 16 * *((uint8_t *)v4 - 2) + v10;
  if ( n4_8 == n4_7 )
  {
    if ( n15_3 == n15_8 )
    {
      v14 = *((uint16_t *)this_1 + n4_8 + 3029720);
      if ( n4_8 == freq_i )
        v13 = (uint16_t)(v14 - *((uint16_t *)v4 - 8));
      else
        v13 = (uint16_t)(v14 - freq_i);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)this_1 + n4_8 + 3029720) - n15_3);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)this_1 + n4_8 + 3029720) - n4_1);
  }
  this_1->f6059432 = (uint32_t)&((uint32_t *)this_1)[4 * v13 + 269674];
  v15 = *((uint8_t *)this_2 + v12 + 1078244);
  this_2->f36 = v15;
  v16 = (uint16_t *)&((uint32_t *)this_2)[0x10000 * v15 + 531818 + v13];
  this_2->f6059436 = (uint8_t *)v16;
  n4_9 = *v16;
  if ( n4_9 == n15_8 )
  {
    __decode_pixel_n15 = 15;
  }
  else if ( n4_9 == n4_1 )
  {
    __decode_pixel_n15 = 30;
  }
  else if ( n4_9 == n15_3 )
  {
    __decode_pixel_n15 = 45;
  }
  else
  {
    v19 = n4_9 == freq_i;
    __decode_pixel_n15 = 60;
    if ( !v19 )
      __decode_pixel_n15 = 0;
  }
  n4_10 = (uint16_t *)v16[1];
  if ( n4_10 == (uint16_t *)n15_8 )
  {
    __decode_pixel_n15 += 75;
  }
  else if ( n4_10 == (uint16_t *)n4_1 )
  {
    __decode_pixel_n15 += 150;
  }
  else if ( n4_10 == (uint16_t *)n15_3 )
  {
    __decode_pixel_n15 += 225;
  }
  else if ( n4_10 == (uint16_t *)freq_i )
  {
    __decode_pixel_n15 += 300;
  }
  v21 = this_2->f56[8];
  n15_3 = (int32_t)n15_9;
  v22 = *((uint8_t *)this_2 + v15 + __decode_pixel_n15 + 1078308);
  v23 = this_2->f56[7];
  this_2->f40 = v22;
  n4_11 = *((uint8_t *)n15_9 + 2);
  v184 = (Obj18 *)(v4);
  this_1 = (ModelBlock *)(this_2);
  freq_i = n4_11;
  v25 = this_2->f56[9];
  v26 = *((uint8_t *)v4 - 5);
  // `v186` is one stack slot with two roles: a row cursor here, and the
  // `uint16_t` value out of `v97[4]` at 11290.  Splitting it needs the frame
  // to dissolve first, so the cast records the double booking (§4.2).
  v186 = (int32_t)v25;
  v27 = 8 * *((uint8_t *)v4 - 12) + 4 * *((uint8_t *)v4 - 9) + v26 + 2 * *((uint8_t *)v4 - 10);
  this_3 = (ModelBlock *)(this_1);
  v29 = ((uint8_t)(*(uint8_t *)(v186 + 2) & *(v21 + 2) & freq_i & *(v23 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(v186 + 3) & *(v21 + 3) & *(v23 + 3) & *((uint8_t *)n15_9 + 3)) << 8)
      + (v22 << 10)
      + v27;
  n15_10 = (uint8_t *)n15_3;
  v31 = ((*((uint8_t *)this_1 + 1078695) == 0) << 7)
      + ((*((uint8_t *)this_1 + 1078694) == 0) << 6)
      + 32 * (*((uint8_t *)this_1 + 1078693) == 0)
      + 16 * (*((uint8_t *)this_1 + 1078692) == 0)
      + v29;
  n0xFFFF = *((uint16_t *)this_1 + v31 + 3037912);
  if ( n0xFFFF == 0xFFFF )
  {
    *((uint16_t *)this_1 + v31 + 3037912) = this_1->f20;
    n15_10 = this_3->f56[6];
    v4 = (Obj18 *)((uint16_t *)this_3->f56[5]);
    ++this_3->f20;
    n0xFFFF = *((uint16_t *)this_3 + v31 + 3037912);
    freq_i = *(n15_10 + 2);
  }
  v33 = *((uint8_t *)v4 - 1) + 4 * *(n15_10 + 13) + 2 * freq_i + 8 * n0xFFFF;
  n0xFFFF_1 = *((uint16_t *)this_3 + v33 + 3230424);
  if ( n0xFFFF_1 == 0xFFFF )
  {
    *((uint16_t *)this_3 + v33 + 3230424) = this_3->f24++;
    n0xFFFF_1 = *((uint16_t *)this_3 + v33 + 3230424);
  }
  if ( (int32_t)this_3->f16 < 32 )
  {
    n53248 = this_3->f28;
    n4_1 = 16 * n0xFFFF_1 + (n4_1 & 0xF);
    v36 = (Obj24 *)((int32_t)this_3 + 2 * n4_1);
    n0xFFFF_1 = v36->f6678448;
    if ( n0xFFFF_1 == 0xFFFF )
    {
      n4_12 = n4_1;
      if ( n53248 > 53248 )
        n4_12 = n4_1 | 0xF;
      v36 = (Obj24 *)((int32_t)this_3 + 2 * n4_12);
      n0xFFFF_1 = v36->f6678448;
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      v36->f6678448 = n53248;
      ++this_3->f28;
      n0xFFFF_1 = v36->f6678448;
    }
  }
  if ( (*(this_3->f56[5] - 5) & *(this_3->f56[5] - 6)) != 0 )
  {
    v38 = (Obj118 *)((uint8_t *)this_3->f56[6]);
    v39 = (Obj118 *)((uint8_t *)this_3->f56[7]);
    if ( ((uint8_t)(v39->f19 & v39->f11 & v39->f3 & v38->f27 & v38->f19 & v38->f11 & v38->f3 & v38->f2 & *((char *)v38 - 5))
        & v39->f27) != 0 )
    {
      v40 = v39->f2;
      n15_11 = 1;
      if ( this_3->f0 - a2 <= 1 )
      {
        n8 = 8;
      }
      else
      {
        n4_1 = this_3->f0 - a2;
        this_1 = (ModelBlock *)(this_3);
        while ( 1 )
        {
          n8 = 8 * n15_11;
          if ( (((uint8_t *)v38)[8 * n15_11 + 19] & ((uint8_t *)v38)[8 * n15_11 + 18]) == 0 )
            break;
          v40 = (uint8_t)(((uint8_t *)v39)[n8 + 2] & v40);
          if ( ++n15_11 >= n4_1 )
          {
            this_3 = (ModelBlock *)(this_1);
            n8 = 8 * n15_11;
            goto LABEL_42;
          }
        }
        this_3 = (ModelBlock *)(this_1);
      }
LABEL_42:
      n15_12 = *(uint8_t *)(this_3->f1078684 + n15_11);
      v44 = __fwd_decode_pixel_decode_context_bit(
              (uint16_t *)&((uint32_t *)this_3)[12 * n15_12
                                        + 269089
                                        + 6 * (uint8_t)(((uint8_t *)v39)[n8 + 27] & ((uint8_t *)v39)[n8 + 19])
                                        + 3 * v40]
            + 3 * *(this_3->f1078688 + n15_8)
            + 1,
              (uint16_t *)this_3 + 538176);
      n4_22 = ::tbl44573C[1];
      v46 = (char *)this_3->f1078688;
      this_3->f32 = v44;
      *(uint8_t *)(v46 + n4_22) = v44;
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
        n15_3 = n15_11;
        n15_8 = n15_12;
        this_1 = (ModelBlock *)(this_3);
        n15_18 = 0;
        v49 = 1 << (n15_12 & 31);
        v50 = 0;
        do
        {
          if ( (v49 | v50) < n15_3 )
          {
            v51 = *((uint16_t *)&((uint32_t *)this_1)[24 * (v50 == 0) + 269473 + 24 * (n15_12 == n15_8)] + 3 * n15_12 + 1);
            freq_i = (int32_t)&((uint32_t *)this_1)[24 * (v50 == 0) + 269473 + 24 * (n15_12 == n15_8)] + 6 * n15_12 + 2;
            bin_tot = v51 + *((uint16_t *)&((uint32_t *)this_1)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12);
            n4_1 = rc.decode_bit(
                     v51,
                     *((uint16_t *)&((uint32_t *)this_1)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12));
            if ( *((uint16_t *)&((uint32_t *)this_1)[24 * (v50 == 0) + 269474 + 24 * (n15_12 == n15_8)] + 3 * n15_12 + 1) < (uint32_t)bin_tot )
              __fwd_decode_pixel_rescale_counter_pair((uint16_t *)freq_i);
            n4_13 = n4_1;
            *(uint16_t *)(freq_i + 2 * n4_1) += 8;
            if ( n4_13 )
              n15_18 |= v49;
            v50 |= n15_18 & v49;
          }
          --n15_12;
          v49 >>= 1;
        }
        while ( v49 );
        this_3 = (ModelBlock *)(this_1);
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
        LOWORD(v55) = ::tbl44573C[1];
        LOWORD(v54) = *v58;
        n4_1 = ::tbl44573C[1];
        v58[1] = (uint16_t)(uintptr_t)v54;
        *(uint16_t *)this_3->f56[5] = (uint16_t)(uintptr_t)v55;
        *(uint16_t *)this_3->f6059436 = (uint16_t)(uintptr_t)v55;
        v59 = (int32_t *)this_3->f56[5];
        v60 = v59[1];
        n15_3 = *v59;
        v61 = (uint8_t *)(int32_t)(v59 + 2);
        this_3->f56[5] = v61;
        if ( n15_18 - n15_14 != 1 )
        {
          freq_i = (n15_18 - n15_14 - 1) / 2;
          if ( freq_i )
          {
            n15_13 = n15_3;
            n15_8 = n15_14;
            n4_14 = n4_1;
            n4_16 = 0;
            n15_24 = n15_18;
            n4_15 = freq_i;
            do
            {
              *(uint16_t *)(this_3->f6059436 + 2) = n4_14;
              *(uint32_t *)this_3->f56[5] = n15_13;
              *(uint32_t *)(this_3->f56[5] + 4) = v60;
              v66 = (char *)(uint32_t)this_3->f6059436;
              this_3->f56[5] += 8;
              *(uint16_t *)(v66 + 2) = n4_14;
              *(uint32_t *)this_3->f56[5] = n15_13;
              *(uint32_t *)(this_3->f56[5] + 4) = v60;
              v61 = this_3->f56[5] + 8;
              this_3->f56[5] = v61;
              ++n4_16;
            }
            while ( n4_16 < n4_15 );
            n15_14 = n15_8;
            n15_18 = n15_24;
            v67 = 2 * n4_16 + 1;
          }
          else
          {
            v67 = 1;
          }
          if ( n15_18 - n15_14 - 1 > (uint32_t)(v67 - 1) )
          {
            n15_15 = n15_3;
            *(uint16_t *)(this_3->f6059436 + 2) = n4_1;
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
        n15_8 = (int32_t)n15_16;
        v74 = v71 + v70;
        v75 = n15_16[34];
        v76 = ((uint8_t *)this_3->f56[7]);
        *((uint8_t *)this_3 + 1078692) = v73 + v72 + v74 + v75 - 5;
        *((uint8_t *)this_3 + 1078693) = v76[26]
                                     + v76[18]
                                     + v76[10]
                                     + v76[2]
                                     + *((char *)v76 - 6)
                                     + *((char *)v76 - 14)
                                     + *((char *)v76 - 22)
                                     + v76[34]
                                     - 8;
        n15_17 = (char *)n15_8;
        *((uint8_t *)this_3 + 1078694) = *(v61 - 29) + *(v61 - 21) - 2;
        n4_17 = n4_1;
        *((uint8_t *)this_3 + 1078695) = *(v61 - 38)
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
  v80 = (Obj18 *)((int32_t)&((uint32_t *)this_3)[4 * this_3->f40 + 24]);
  v184 = (Obj18 *)((uint16_t *)v80);
  v81 = 4 * n0xFFFF_1;
  freq_tbl = (uint16_t *)&((uint32_t *)this_3)[v81 + 776];
  v83 = HIWORD(((uint32_t *)this_3)[v81 + 778]);
  if ( HIWORD(((uint32_t *)this_3)[v81 + 778]) )
  {
    if ( v83 == 1 )
    {
      v142 = (Obj18 *)((uint64_t *)v80);
      v143 = *(uint8_t *)((char *)&v80->f8 + 7);
      n4_18 = v143 * LOWORD(((uint32_t *)this_3)[v81 + 777]);
      n15_19 = v143 * freq_tbl[3];
      n4_1 = v143 * *freq_tbl;
      v146 = v143 * freq_tbl[1];
      freq_i = n4_18;
      n15_20 = v143 * freq_tbl[4];
      n15_8 = n15_19;
      n15_3 = n15_20;
      *(uint64_t *)freq_tbl = v142->f0;
      *((uint64_t *)freq_tbl + 1) = v142->f8;
      v148 = freq_tbl[5];
      v149 = *freq_tbl;
      *((uint8_t *)freq_tbl + 14) *= 8;
      this_1 = (ModelBlock *)(this_3);
      v150 = 21 * freq_tbl[1];
      n4_1 += (21 * v149 + v148 - 1) / v148;
      *freq_tbl = n4_1;
      v151 = (v150 + v148 - 1) / v148;
      v152 = 21 * freq_tbl[2];
      v153 = freq_tbl[3];
      v154 = v151 + v146;
      freq_tbl[1] = v154;
      v155 = 21 * v153;
      n15_21 = n15_8;
      v157 = (char *)((v152 + v148 - 1) / v148 + freq_i);
      freq_tbl[2] = (uint16_t)(uintptr_t)v157;
      v158 = (v155 + v148 - 1) / v148 + n15_21;
      v159 = 21 * freq_tbl[4];
      freq_tbl[3] = v158;
      v160 = v158 + (uint16_t)(uintptr_t)v157 + v154;
      this_3 = (ModelBlock *)(this_1);
      v161 = (v159 + v148 - 1) / v148 + n15_3;
      freq_tbl[4] = v161;
      v162 = n4_1 + v161 + v160;
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
      n15_8 = n256_2;
      v137 = freq_tbl[1];
      this_1 = (ModelBlock *)(this_3);
      v138 = *freq_tbl;
      n4_1 = n4;
      freq_i = n15_1;
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
      n15_1 = freq_i;
      LOWORD(v140) = v138 + v140;
      this_3 = (ModelBlock *)(this_1);
      n256_1 = (uint16_t)(v140 + v137);
      n256 = n15_8;
      n4 = n4_1;
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
      v90 = (Obj18 *)(v184);
      ++*(uint16_t *)((char *)&v184->f8 + 2);
      ++((uint16_t *)v90)[n4];
      n4 = this_3->f32;
    }
  }
  else
  {
    arg_tot = *(uint16_t *)((char *)&v80->f8 + 2);
    v163 = rc.get_freq(arg_tot);
    v164 = *(uint16_t *)&v184->f0;
    if ( v164 <= v163 )
    {
      v164 += *(uint16_t *)((char *)&v184->f0 + 2);
      if ( v164 <= v163 )
      {
        v164 += *(uint16_t *)((char *)&v184->f0 + 4);
        if ( v164 <= v163 )
        {
          v164 += *(uint16_t *)((char *)&v184->f0 + 6);
          if ( v164 <= v163 )
          {
            v164 += *(uint16_t *)&v184->f8;
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
    n15_22 = *((uint8_t *)v184 + 15);
    arg_high = v164;
    arg_cum = v164 - ((uint16_t *)v184)[n4_2];
    n256_4 = *(uint16_t *)((char *)&v184->f8 + 2);
    n256_5 = *(uint16_t *)((char *)&v184->f8 + 4);
    n15_3 = n15_22;
    if ( n256_4 > n256_5 && (((uint16_t *)v184)[n4_2] + n15_3 + 8 < n256_4 || n256_4 > 0x4000) )
    {
      n15_8 = n256_5;
      this_1 = (ModelBlock *)(this_3);
      n4_1 = (int32_t)freq_tbl;
      freq_i = n4_2;
      v171 = (Obj18 *)(v184);
      v172 = *(uint16_t *)((char *)&v184->f0 + 2);
      v173 = *(uint16_t *)((char *)&v184->f0 + 4);
      v174 = *(uint16_t *)&v184->f0 - (*(uint16_t *)&v184->f0 >> 1);
      *(uint16_t *)&v184->f0 = v174;
      LOWORD(v172) = v172 - (v172 >> 1);
      *(uint16_t *)((char *)&v171->f0 + 2) = v172;
      v175 = v173 - (v173 >> 1);
      v176 = (uint16_t)*(uint16_t *)((char *)&v171->f0 + 6);
      *(uint16_t *)((char *)&v171->f0 + 4) = v175;
      LOWORD(v176) = v176 - (v176 >> 1);
      v177 = (uint16_t)*(uint16_t *)&v171->f8;
      *(uint16_t *)((char *)&v171->f0 + 6) = v176;
      LOWORD(v177) = v177 - (v177 >> 1);
      *(uint16_t *)&v171->f8 = v177;
      LOWORD(v177) = v174 + v177;
      this_3 = (ModelBlock *)(this_1);
      n256_4 = (uint16_t)(v177 + v176 + v175 + v172);
      n256_3 = n15_8;
      freq_tbl = (uint16_t *)n4_1;
      *(uint16_t *)((char *)&v171->f8 + 2) = n256_4;
      n4_2 = freq_i;
      if ( n256_3 < 256 && !*((uint8_t *)v184 + 14) )
      {
        n256_3 = 256;
        *(uint16_t *)((char *)&v184->f8 + 4) = 256;
      }
      if ( n256_4 > n256_3 )
      {
        n15_2 = n15_3;
        if ( n15_3 < 15 )
          n15_2 = 15;
        n15_3 = n15_2;
        *((uint8_t *)v184 + 15) = n15_2;
      }
    }
    n15_4 = n15_3;
    v170 = (Obj18 *)(v184);
    *(uint16_t *)((char *)&v184->f8 + 2) = n15_3 + n256_4;
    ((uint16_t *)v170)[n4_2] += n15_4;
    rc.decode(arg_cum, arg_high, arg_tot);
    this_3->f32 = n4_2;
    freq_tbl[5] = freq_tbl[n4_2]++ != 0;
    n4 = this_3->f32;
  }
  if ( n4 )
  {
    *(uint16_t *)this_3->f56[5] = tbl44573C[n4];
    return 1;
  }
  n15_18 = 0;
LABEL_86:
  v91 = __byte_445700;
  n15_23 = ::tbl44573C[3];
  n4_19 = ::tbl44573C[1];
  n15_24 = n15_18;
  n4_20 = ::tbl44573C[2];
  exclusion_mask[tbl44573C[4]] = __byte_445700;
  v95 = (uint16_t *)this_3->f6059436;
  exclusion_mask[n15_23] = v91;
  exclusion_mask[n4_20] = v91;
  exclusion_mask[n4_19] = v91;
  __byte_445440[0] = v91;
  this_3->f1078216 = 0;
  n4_5 = v95[1];
  n15_8 = *v95;
  v97 = ((uint16_t *)this_3->f6059432);
  n4_6 = v97[0];
  n15_5 = v97[1];
  v100 = (Obj18 *)((uint16_t *)v97[2]);
  n4_1 = n4_5;
  this_4 = (ModelBlock *)(v97[3]);
  freq_i = n4_6;
  v102 = v97[4];
  n15_3 = n15_5;
  v103 = v97[5];
  v184 = (Obj18 *)(v100);
  v104 = v97[6];
  v105 = v97[7];
  this_1 = (ModelBlock *)((uint32_t *)this_4);
  v106 = (Obj121 *)((uint16_t *)this_3->f56[6]);
  v186 = v102;
  v107 = v106->f16;
  v187 = v103;
  v108 = this_3->f56[5];
  v188 = v104;
  v109 = *(uint16_t *)(v108 - 16);
  v189 = v105;
  v190 = v109;
  v110 = (uint16_t *)this_3->f56[7];
  v111 = v110[4];
  v191 = v107;
  v112 = *v110;
  v192 = v111;
  v113 = *((uint16_t *)v106 - 8);
  v193 = v112;
  v114 = *(v110 - 4);
  v194 = v113;
  v115 = *(uint16_t *)(v108 - 24);
  v195 = v114;
  v116 = v106->f24;
  v196 = v115;
  v117 = v106->f32;
  v197 = v116;
  v118 = *(uint16_t *)(v108 - 32);
  v198 = v117;
  v119 = *((uint16_t *)v106 - 12);
  v199 = v118;
  v120 = v110[8];
  v200 = v119;
  v201 = v120;
  v121 = (uint16_t *)this_3->f56[8];
  v202 = *v121;
  v203 = *(v110 - 8);
  v122 = *(uint16_t *)(v108 - 40);
  v123 = *(uint16_t *)(v108 - 56);
  v204 = v122;
  v205 = v121[4];
  v124 = v106->f40;
  v125 = v106->f56;
  v206 = v124;
  v207 = *(uint16_t *)this_3->f56[9];
  v208 = v123;
  v209 = *(v121 - 4);
  v126 = v110[12];
  this_3->f44 = 0;
  v210 = v125;
  v211 = v126;
  do
  {
    v127 = __fwd_decode_pixel_pixel_context(this_3, __frame.sym);
    if ( v127 >= 0 )
    {
      v128 = __fwd_decode_pixel_decode_context_bit(
               (uint16_t *)this_3 + 3 * this_3->f52 + 525888,
               (uint16_t *)this_3 + 3 * this_3->f48 + 525840);
      *(uint16_t *)this_3->f56[5] = v127;
      if ( v128 )
        return n15_24 + 1;
      exclusion_mask[v127] = __byte_445700;
    }
    n32 = this_3->f44 + 1;
    this_3->f44 = n32;
  }
  while ( n32 < 32 );
  n15_25 = n15_24;
  n4_21 = ::tbl44573C[1];
  v132 = (int32_t)(uintptr_t)this_3->f1078208;
  this_3->f1078216 = this_3->f1078212 + 24 * ::tbl44573C[2];
  v133 = v132 + 24 * n4_21;
  v134 = (uint32_t **)this_3->f1078232;
  this_3->f1078220 = (uint8_t *)v133;
  while ( 1 )
  {
    if ( (*v134)[1] )
    {
      v135 = __fwd_decode_pixel_decode_encode_symbol_list(*v134);
      *(uint16_t *)this_3->f56[5] = v135;
      if ( v135 >= 0 )
        return n15_25 + 1;
      v134 = (uint32_t **)this_3->f1078232;
    }
    this_3->f1078232 = ++v134;
  }
}
static inline int32_t __fwd_code_pixel_encode_context_bit(void *a0, void *a1, int32_t a2) { return __encode_context_bit((uint16_t *)a0, (uint16_t *)a1, a2); }
static inline int32_t __fwd_code_pixel_encode_symbol_list(void *a0, int32_t a1) { return __encode_symbol_list((uint32_t *)a0, a1); }
static inline int32_t __fwd_code_pixel_rescale_counter_pair(void *a0) { return __rescale_counter_pair((uint16_t *)a0); }
static inline int32_t __fwd_code_pixel_pixel_context(void *a0, void *a1) { return __pixel_context((ModelBlock *)a0, (uint32_t *)a1); }

int32_t __code_pixel(ModelBlock *_this, int32_t a2)
{
  struct alignas(16) {   // 164 bytes, the frame Hex-Rays could not name
      uint32_t  sym[32];   // pixel_context reads sym[0..31]; `f44` runs 0..31 (was p_n15 .. v207)
      uint8_t   _gap0[4];   // was int32_t n15_14
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  int32_t &p_n15 = (int32_t &)__frame.sym[0];
  int32_t &n2_1 = (int32_t &)__frame.sym[1];
  uint16_t * &n2_4 = (uint16_t * &)__frame.sym[2];
  int32_t &n15_20 = (int32_t &)__frame.sym[3];
  int32_t &n15_31 = (int32_t &)__frame.sym[4];
  int32_t &n15_2 = (int32_t &)__frame.sym[5];
  int32_t &n15_4 = (int32_t &)__frame.sym[6];
  ModelBlock * &this_1 = (ModelBlock * &)__frame.sym[7];
  int32_t &n15_3 = (int32_t &)__frame.sym[8];
  Obj34 * &v185 = (Obj34 * &)__frame.sym[9];
  int32_t &n15_7 = (int32_t &)__frame.sym[10];
  int32_t &v187 = (int32_t &)__frame.sym[11];
  int32_t &v188 = (int32_t &)__frame.sym[12];
  int32_t &v189 = (int32_t &)__frame.sym[13];
  int32_t &v190 = (int32_t &)__frame.sym[14];
  int32_t &v191 = (int32_t &)__frame.sym[15];
  int32_t &v192 = (int32_t &)__frame.sym[16];
  int32_t &v193 = (int32_t &)__frame.sym[17];
  int32_t &v194 = (int32_t &)__frame.sym[18];
  int32_t &v195 = (int32_t &)__frame.sym[19];
  int32_t &v196 = (int32_t &)__frame.sym[20];
  int32_t &v197 = (int32_t &)__frame.sym[21];
  int32_t &v198 = (int32_t &)__frame.sym[22];
  int32_t &v199 = (int32_t &)__frame.sym[23];
  int32_t &v200 = (int32_t &)__frame.sym[24];
  int32_t &v201 = (int32_t &)__frame.sym[25];
  int32_t &v202 = (int32_t &)__frame.sym[26];
  int32_t &v203 = (int32_t &)__frame.sym[27];
  int32_t &v204 = (int32_t &)__frame.sym[28];
  int32_t &v205 = (int32_t &)__frame.sym[29];
  int32_t &v206 = (int32_t &)__frame.sym[30];
  int32_t &v207 = (int32_t &)__frame.sym[31];
  int32_t n15_14;
  ;
  Obj99 *v63;
  uint8_t *v20, *v22, *n2_9, *v52, *v109;   // row cursors out of ModelBlock
  char *v59, *v74, *n15_36, *n15_38, *n15_40;
  bool v11;
  char v24, *v36, v64, v65, v66, v67, v68, v93, *v156;
  int16_t v14, n15_10, v147, v158, n15_21, v170;
  ModelBlock *this_3;
  ModelBlock *this_4;
  ModelBlock *this_2;
  int32_t arg_cum, arg_high, arg_tot, n4, n4_1, __code_pixel_n15, n15_1, v8, v9, v10, v13, v15,
          *v16, n15_6, n15_5, n15_8, v21, p_n15_1, n15_30, v26, v27, v29, n0xFFFF, v32,
          n0xFFFF_1, n53248, v35, v38, v39, v42, p_n15_2, n15_42, n8, v46, n15_32, n15_12,
          n15_9, v50, v53, p_n15_4, n2_10, v61, n15_11, n15_13, n4_2, n15_33, n15_35, n15_34,
          n15_15, n2_15, v82, *n2_3, __code_pixel_n0x2000, n15_16, v87, n2, p_n15_5, p_n15_7,
          n15_17, n15_22, excl_sym_a, excl_sym_b, p_n15_11, n2_5, n15_41, n15_23,
          n15_24, n15_25, n15_26, n15_27, v112, v114, v115, v116, v117, v118, v119, v120,
          v121, v122, v123, v125, v126, v127, v128, v129, n15_28, n32, n15_29, n4_5, v134, v135,
          p_n15_6, *v143, v144, n2_16, n2_17, n15_37, v149, v150, v151, v152, v154, v155, v157,
          n15_18, v161, n2_2, p_n15_10, p_n15_8, p_n15_9, n15_19;
  Obj66 *v159;
  Obj121 *v111;
  Obj34 *v51;
  Obj34 *v37;
  Obj34 *v108;
  uint16_t *n2_7, *n2_8, *n2_11, *n2_12, *n2_13, *n2_14, *v97, *v98, *n2_6, *v113, *v124, v153,
           *n15_39;
  uint32_t bin_tot, v55, v57, **v136, v137, p_n15_12, v139, v140, v141, v168, v169, v171, v172,
          v173;
  uint8_t *v69;
  uint8_t *p_n15_3, *v41;
  n2_7 = (uint16_t *)*((int32_t *)_this + 20);
  n4 = *n2_7;
  n2_8 = (uint16_t *)*((int32_t *)_this + 19);
  n4_1 = (uint16_t)*(n2_8 - 4);
  __code_pixel_n15 = n2_7[4];
  this_1 = (ModelBlock *)(_this);
  n15_1 = *(n2_7 - 4);
  n15_3 = n4;
  ::tbl44573C[1] = n4;
  n15_7 = n4_1;
  ::tbl44573C[2] = n4_1;
  n15_4 = __code_pixel_n15;
  ::tbl44573C[3] = __code_pixel_n15;
  v8 = *((uint8_t *)n2_7 + 3) + 4 * (__code_pixel_n15 == n15_1);
  v9 = *((uint8_t *)n2_7 + 11);
  n15_2 = n15_1;
  tbl44573C[4] = n15_1;
  v10 = 32 * *((uint8_t *)n2_8 - 4)
      + 16 * *((uint8_t *)n2_8 - 2)
      + 2 * *((uint8_t *)n2_8 - 6)
      + 8 * v9
      + v8;
  v11 = n4 == n4_1;
  this_2 = (ModelBlock *)(this_1);
  if ( v11 )
  {
    if ( n15_3 == n15_4 )
    {
      v14 = *((uint16_t *)this_1 + n15_3 + 3029720);
      if ( n15_3 == n15_2 )
        v13 = (uint16_t)(v14 - *(n2_8 - 8));
      else
        v13 = (uint16_t)(v14 - n15_2);
    }
    else
    {
      v13 = (uint16_t)(*((uint16_t *)this_1 + n15_3 + 3029720) - n15_4);
    }
  }
  else
  {
    v13 = (uint16_t)(*((uint16_t *)this_1 + n15_3 + 3029720) - n15_7);
  }
  this_1->f6059432 = (uint32_t)&((int32_t *)this_1)[4 * v13 + 269674];
  v15 = *((uint8_t *)this_2 + v10 + 1078244);
  *(int32_t *)&this_2->f36 = v15;
  v16 = &((int32_t *)this_2)[0x10000 * v15 + 531818 + v13];
  this_2->f6059436 = (uint8_t *)v16;
  n15_6 = *(uint16_t *)v16;
  if ( n15_6 == n15_3 )
  {
    n15_5 = 15;
  }
  else if ( n15_6 == n15_7 )
  {
    n15_5 = 30;
  }
  else if ( n15_6 == n15_4 )
  {
    n15_5 = 45;
  }
  else
  {
    v11 = n15_6 == n15_2;
    n15_5 = 60;
    if ( !v11 )
      n15_5 = 0;
  }
  n15_8 = *((uint16_t *)v16 + 1);
  if ( n15_8 == n15_3 )
  {
    n15_5 += 75;
  }
  else if ( n15_8 == n15_7 )
  {
    n15_5 += 150;
  }
  else if ( n15_8 == n15_4 )
  {
    n15_5 += 225;
  }
  else if ( n15_8 == n15_2 )
  {
    n15_5 += 300;
  }
  v20 = this_2->f56[8];
  n2_1 = (int32_t)n2_7;
  v21 = *((uint8_t *)this_2 + v15 + n15_5 + 1078308);
  v22 = this_2->f56[7];
  *(int32_t *)&this_2->f40 = v21;
  p_n15_1 = *((uint8_t *)n2_7 + 2);
  v24 = *((uint8_t *)n2_7 + 3);
  n2_4 = n2_8;
  this_1 = (ModelBlock *)(this_2);
  p_n15 = p_n15_1;
  n15_30 = *(int32_t *)&this_2->f56[9];
  v26 = *((uint8_t *)n2_8 - 5);
  n15_20 = n15_30;
  v27 = 8 * *((uint8_t *)n2_8 - 12)
      + 4 * *((uint8_t *)n2_8 - 9)
      + v26
      + 2 * *((uint8_t *)n2_8 - 10);
  this_3 = (ModelBlock *)(this_1);
  v29 = ((*((uint8_t *)this_1 + 1078695) == 0) << 7)
      + ((*((uint8_t *)this_1 + 1078694) == 0) << 6)
      + 32 * (*((uint8_t *)this_1 + 1078693) == 0)
      + 16 * (*((uint8_t *)this_1 + 1078692) == 0)
      + ((uint8_t)(*(uint8_t *)(n15_20 + 2) & *(v20 + 2) & p_n15 & *(v22 + 2)) << 9)
      + ((uint8_t)(*(uint8_t *)(n15_20 + 3) & *(v20 + 3) & *(v22 + 3) & v24) << 8)
      + (v21 << 10)
      + v27;
  n0xFFFF = *((uint16_t *)this_1 + v29 + 3037912);
  n2_9 = (uint8_t *)n2_1;
  if ( n0xFFFF == 0xFFFF )
  {
    *((uint16_t *)this_1 + v29 + 3037912) = *(int32_t *)&this_1->f20;
    n2_9 = this_3->f56[6];
    n2_8 = (uint16_t *)this_3->f56[5];
    ++*(int32_t *)&this_3->f20;
    n0xFFFF = *((uint16_t *)this_3 + v29 + 3037912);
    p_n15 = *(n2_9 + 2);
  }
  v32 = *((uint8_t *)n2_8 - 1) + 4 * *(n2_9 + 13) + 2 * p_n15 + 8 * n0xFFFF;
  n0xFFFF_1 = *((uint16_t *)this_3 + v32 + 3230424);
  if ( n0xFFFF_1 == 0xFFFF )
  {
    *((uint16_t *)this_3 + v32 + 3230424) = (*(int32_t *)&this_3->f24)++;
    n0xFFFF_1 = *((uint16_t *)this_3 + v32 + 3230424);
  }
  if ( *(int32_t *)&this_3->f16 < 32 )
  {
    n53248 = *(int32_t *)&this_3->f28;
    v35 = 16 * n0xFFFF_1 + (n15_7 & 0xF);
    v36 = (char *)this_3 + 2 * v35;
    n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    if ( n0xFFFF_1 == 0xFFFF )
    {
      if ( n53248 > 53248 )
        v35 |= 0xFu;
      v36 = (char *)this_3 + 2 * v35;
      n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    }
    if ( n0xFFFF_1 >= n53248 )
    {
      *((uint16_t *)v36 + 3339224) = n53248;
      ++*(int32_t *)&this_3->f28;
      n0xFFFF_1 = *((uint16_t *)v36 + 3339224);
    }
  }
  v37 = (Obj34 *)((uint16_t *)this_3->f56[5]);
  v38 = *((uint8_t *)v37 - 5);
  v39 = *((uint8_t *)v37 - 6);
  v185 = (Obj34 *)(v37);
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
    n15_31 = 1;
    if ( p_n15_2 <= 1 )
    {
      n8 = 8;
    }
    else
    {
      p_n15 = p_n15_2;
      n15_42 = 1;
      this_1 = (ModelBlock *)(this_3);
      while ( 1 )
      {
        n8 = 8 * n15_42;
        if ( (p_n15_3[8 * n15_42 + 19] & p_n15_3[8 * n15_42 + 18]) == 0 )
          break;
        v42 = (uint8_t)(v41[n8 + 2] & v42);
        if ( ++n15_42 >= p_n15 )
        {
          this_3 = (ModelBlock *)(this_1);
          n15_31 = n15_42;
          n8 = 8 * n15_42;
          goto LABEL_42;
        }
      }
      this_3 = (ModelBlock *)(this_1);
      n15_31 = n15_42;
    }
LABEL_42:
    v46 = (uint8_t)(v41[n8 + 27] & v41[n8 + 19]);
    n15_2 = *(uint8_t *)(*(int32_t *)&this_3->f1078684 + n15_31);
    n15_32 = *(uint8_t *)(*(int32_t *)&this_3->f1078688 + n15_3) + 8 * n15_2 + 4 * v46 + 2 * v42;
    n15_12 = 0;
    if ( v185->f0 == n15_3 )
    {
      p_n15 = (int32_t)p_n15_3;
      this_1 = (ModelBlock *)(this_3);
      do
        ++n15_12;
      while ( n15_12 < n15_31 && ((uint16_t *)v185)[4 * n15_12] == n15_3 );
      this_3 = (ModelBlock *)(this_1);
      p_n15_3 = (uint8_t *)p_n15;
    }
    n15_9 = n15_12 == n15_31;
    n15_4 = n15_9;
    if ( n15_12 > n15_9 )
    {
      this_3->f56[6] = &p_n15_3[8 * n15_12 + -8 * n15_9];
      this_3->f56[7] = &v41[8 * n15_12 + -8 * n15_9];
      v50 = *(int32_t *)&this_3->f56[9];
      this_3->f56[8] = this_3->f56[8] + 8 * n15_12 - 8 * n15_9;
      v51 = (Obj34 *)(v185);
      *(int32_t *)&this_3->f56[9] = v50 + 8 * n15_12 - 8 * n15_9;
      *((uint32_t *)v51 + 1) = 0x01010101;
      *(uint32_t *)this_3->f56[5] = 0x01010101;
      *(uint16_t *)(this_3->f6059436 + 2) = *(uint16_t *)this_3->f6059436;
      LOWORD(v51) = n15_3;
      *(uint16_t *)this_3->f56[5] = n15_3;
      *(uint16_t *)this_3->f6059436 = (uint16_t)(uintptr_t)v51;
      v52 = this_3->f56[5];
      v53 = *(uint32_t *)v52;
      n2_1 = *(uint32_t *)(v52 + 4);
      n2_4 = (uint16_t *)(v52 + 8);
      this_3->f56[5] = v52 + 8;
      if ( n15_12 - n15_9 != 1 )
      {
        p_n15_4 = n15_12 - n15_9 - 1;
        v55 = p_n15_4 / 2;
        if ( p_n15_4 / 2 )
        {
          p_n15 = p_n15_4;
          n2_10 = n2_1;
          n15_14 = n15_12;
          v57 = 0;
          n15_20 = n15_32;
          n15_10 = n15_3;
          do
          {
            *(uint16_t *)(this_3->f6059436 + 2) = n15_10;
            *(uint32_t *)this_3->f56[5] = v53;
            *(uint32_t *)(this_3->f56[5] + 4) = n2_10;
            v59 = (char *)this_3->f6059436;
            this_3->f56[5] += 8;
            *(uint16_t *)(v59 + 2) = n15_10;
            *(uint32_t *)this_3->f56[5] = v53;
            *(uint32_t *)(this_3->f56[5] + 4) = n2_10;
            n2_11 = (uint16_t *)(this_3->f56[5] + 8);
            this_3->f56[5] = (uint8_t *)n2_11;
            ++v57;
          }
          while ( v57 < v55 );
          p_n15_4 = p_n15;
          n15_32 = n15_20;
          n2_4 = n2_11;
          n15_12 = n15_14;
          v61 = 2 * v57 + 1;
        }
        else
        {
          v61 = 1;
        }
        if ( p_n15_4 > (uint32_t)(v61 - 1) )
        {
          *(uint16_t *)(this_3->f6059436 + 2) = n15_3;
          *(uint32_t *)this_3->f56[5] = v53;
          *(uint32_t *)(this_3->f56[5] + 4) = n2_1;
          n2_12 = (uint16_t *)(this_3->f56[5] + 8);
          this_3->f56[5] = (uint8_t *)n2_12;
          n2_4 = n2_12;
        }
      }
      v63 = (Obj99 *)(this_3->f56[6]);
      v64 = *(uint8_t *)((char *)v63 - 14);
      v65 = v63->f18;
      v66 = v63->f34;
      n15_20 = n15_32;
      v67 = *(uint8_t *)((char *)v63 - 22);
      n15_14 = n15_12;
      v68 = v63->f26 + v65 + v64 + v67 + v66 - 5;
      v69 = ((uint8_t *)this_3->f56[7]);
      *((uint8_t *)this_3 + 1078692) = v68;
      n2_13 = n2_4;
      *((uint8_t *)this_3 + 1078693) = v69[26]
                                   + v69[18]
                                   + v69[10]
                                   + v69[2]
                                   + *((char *)v69 - 6)
                                   + *((char *)v69 - 14)
                                   + *((char *)v69 - 22)
                                   + v69[34]
                                   - 8;
      *((uint8_t *)this_3 + 1078694) = *((uint8_t *)n2_13 - 29) + *((uint8_t *)n2_13 - 21) - 2;
      n15_11 = n15_3;
      *((uint8_t *)this_3 + 1078695) = *((uint8_t *)n2_13 - 38)
                                   + *((uint8_t *)n2_13 - 46)
                                   + *((uint8_t *)n2_13 - 54)
                                   + *((uint8_t *)n2_13 - 30)
                                   - 4;
      *((uint8_t *)n2_13 - 2) = n15_11 == v63->f8;
      n15_32 = n15_20;
      *(this_3->f56[5] - 1) = n15_11 == *(uint16_t *)(this_3->f56[6] + 16);
      n15_12 = n15_14;
    }
    __fwd_code_pixel_encode_context_bit((uint16_t *)this_3 + 3 * n15_32 + 538179, (uint16_t *)this_3 + 538176, n15_4);
    n15_13 = n15_4;
    n4_2 = ::tbl44573C[1];
    v74 = (char *)this_3->f1078688;
    *(int32_t *)&this_3->f32 = n15_4;
    *(uint8_t *)(v74 + n4_2) = n15_13;
    if ( !n15_13 && n15_31 != 1 )
    {
      n15_14 = n15_12;
      p_n15 = n15_2;
      this_1 = (ModelBlock *)(this_3);
      n15_33 = n15_31;
      n15_35 = 1 << (n15_2 & 31);
      n15_34 = 0;
      n15_15 = n15_2;
      do
      {
        if ( n15_33 > (n15_35 | n15_34) )
        {
          n15_20 = n15_34;
          n2_14 = (uint16_t *)&((int32_t *)this_1)[24 * (n15_34 == 0) + 269473 + 24 * (n15_15 == p_n15)] + 3 * n15_15 + 1;
          n2_4 = n2_14;
          n2_1 = n15_14 & n15_35;
          bin_tot = *n2_14 + n2_14[1];
          rc.encode_bit(*n2_14, n2_14[1], (n15_14 & n15_35) != 0);
          if ( *((uint16_t *)&((int32_t *)this_1)[24 * (n15_34 == 0) + 269474 + 24 * (n15_15 == p_n15)] + 3 * n15_15 + 1) < (uint32_t)bin_tot )
            __fwd_code_pixel_rescale_counter_pair(n2_4);
          n2_15 = n2_1;
          n15_31 = n15_35;
          n2_4[n2_1 != 0] += 8;
          n15_34 |= n2_15;
          n15_35 = n15_31;
        }
        --n15_15;
        n15_35 >>= 1;
      }
      while ( n15_35 );
      n15_12 = n15_14;
      this_3 = (ModelBlock *)(this_1);
    }
    if ( *(int32_t *)&this_3->f32 )
      return n15_12;
  }
  else
  {
    v82 = 4 * *(int32_t *)&this_3->f40;
    n15_36 = (char *)&((int32_t *)this_3)[v82 + 24];
    n15_31 = (int32_t)(uintptr_t)n15_36;
    n2_3 = &((int32_t *)this_3)[4 * n0xFFFF_1 + 776];
    __code_pixel_n0x2000 = HIWORD(((int32_t *)this_3)[4 * n0xFFFF_1 + 778]);
    if ( __code_pixel_n0x2000 )
    {
      if ( __code_pixel_n0x2000 == 1 )
      {
        v143 = &((int32_t *)this_3)[v82 + 24];
        v144 = *(uint8_t *)(n15_36 + 15);
        n2_16 = v144 * *((uint16_t *)n2_3 + 2);
        n2_17 = v144 * *((uint16_t *)n2_3 + 3);
        p_n15 = v144 * *(uint16_t *)n2_3;
        v147 = v144 * *((uint16_t *)n2_3 + 1);
        n2_1 = n2_16;
        n15_37 = v144 * *((uint16_t *)n2_3 + 4);
        n2_4 = (uint16_t *)n2_17;
        n15_20 = n15_37;
        *(uint64_t *)n2_3 = *(uint64_t *)v143;
        *((uint64_t *)n2_3 + 1) = *((uint64_t *)v143 + 1);
        v149 = *((uint16_t *)n2_3 + 5);
        *((uint8_t *)n2_3 + 14) *= 8;
        this_1 = (ModelBlock *)(this_3);
        v150 = 21 * *((uint16_t *)n2_3 + 1);
        p_n15 += (21 * *(uint16_t *)n2_3 + v149 - 1) / v149;
        *(uint16_t *)n2_3 = p_n15;
        v151 = (v150 + v149 - 1) / v149;
        LOWORD(v150) = n2_1;
        v152 = 21 * *((uint16_t *)n2_3 + 2);
        v153 = v151 + v147;
        *((uint16_t *)n2_3 + 1) = v153;
        v154 = (v152 + v149 - 1) / v149;
        v155 = 21 * *((uint16_t *)n2_3 + 3);
        LOWORD(v150) = v154 + v150;
        *((uint16_t *)n2_3 + 2) = v150;
        v156 = (char *)n2_4 + (v155 + v149 - 1) / v149;
        v157 = 21 * *((uint16_t *)n2_3 + 4);
        *((uint16_t *)n2_3 + 3) = (uint16_t)(uintptr_t)v156;
        v158 = (uint16_t)(uintptr_t)v156 + v150 + v153;
        this_3 = (ModelBlock *)(this_1);
        LOWORD(v149) = (v157 + v149 - 1) / v149 + n15_20;
        *((uint16_t *)n2_3 + 4) = v149;
        __code_pixel_n0x2000 = (uint16_t)(p_n15 + v149 + v158);
        *((uint16_t *)n2_3 + 5) = __code_pixel_n0x2000;
        v185 = (Obj34 *)((uint16_t *)this_3->f56[5]);
      }
      n15_16 = v185->f0;
      arg_tot = __code_pixel_n0x2000;
      if ( n15_16 == n15_3 )
      {
        v87 = *(uint16_t *)n2_3;
        n2 = 1;
      }
      else if ( n15_16 == n15_7 )
      {
        v87 = *(uint16_t *)n2_3 + *((uint16_t *)n2_3 + 1);
        n2 = 2;
      }
      else if ( n15_16 == n15_4 )
      {
        v87 = *(uint16_t *)n2_3 + *((uint16_t *)n2_3 + 2) + *((uint16_t *)n2_3 + 1);
        n2 = 3;
      }
      else if ( n15_16 == n15_2 )
      {
        v87 = *((uint16_t *)n2_3 + 5) - *((uint16_t *)n2_3 + 4);
        n2 = 4;
      }
      else
      {
        v87 = 0;
        n2 = 0;
      }
      p_n15_5 = *((uint16_t *)n2_3 + 6);
      arg_cum = v87;
      arg_high = *((uint16_t *)n2_3 + n2) + v87;
      p_n15_7 = *((uint16_t *)n2_3 + 5);
      n15_17 = *((uint8_t *)n2_3 + 15);
      if ( p_n15_7 > p_n15_5
        && (*((uint16_t *)n2_3 + n2) + n15_17 + 8 < p_n15_7 || *((uint16_t *)n2_3 + 5) > 0x4000u) )
      {
        v137 = *((uint16_t *)n2_3 + 2);
        p_n15 = p_n15_5;
        p_n15_12 = *((uint16_t *)n2_3 + 1);
        this_1 = (ModelBlock *)(this_3);
        n2_1 = n2;
        v139 = *(uint16_t *)n2_3;
        n2_4 = (uint16_t *)n15_17;
        LOWORD(v139) = v139 - (v139 >> 1);
        *(uint16_t *)n2_3 = v139;
        LOWORD(p_n15_12) = p_n15_12 - (p_n15_12 >> 1);
        *((uint16_t *)n2_3 + 1) = p_n15_12;
        LOWORD(v137) = v137 - (v137 >> 1);
        v140 = *((uint16_t *)n2_3 + 3);
        *((uint16_t *)n2_3 + 2) = v137;
        LOWORD(v140) = v140 - (v140 >> 1);
        v141 = *((uint16_t *)n2_3 + 4);
        *((uint16_t *)n2_3 + 3) = v140;
        LOWORD(v141) = v141 - (v141 >> 1);
        *((uint16_t *)n2_3 + 4) = v141;
        LOWORD(p_n15_12) = v140 + v137 + p_n15_12;
        n15_17 = (int32_t)n2_4;
        LOWORD(p_n15_12) = v139 + v141 + p_n15_12;
        n2 = n2_1;
        this_3 = (ModelBlock *)(this_1);
        p_n15_7 = (uint16_t)p_n15_12;
        p_n15_6 = p_n15;
        *((uint16_t *)n2_3 + 5) = p_n15_7;
        if ( p_n15_6 < 256 && !*((uint8_t *)n2_3 + 14) )
        {
          p_n15_6 = 256;
          *((uint16_t *)n2_3 + 6) = 256;
        }
        if ( p_n15_7 > p_n15_6 )
        {
          if ( n15_17 < 15 )
            LOWORD(n15_17) = 15;
          *((uint8_t *)n2_3 + 15) = n15_17;
        }
      }
      *((uint16_t *)n2_3 + 5) = n15_17 + p_n15_7;
      *((uint16_t *)n2_3 + n2) += n15_17;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->f32 = n2;
      if ( *((uint8_t *)n2_3 + 14) )
      {
        --*((uint8_t *)n2_3 + 14);
        n15_38 = (char *)n15_31;
        ++*(uint16_t *)(n15_31 + 10);
        ++*(uint16_t *)(n15_38 + 2 * n2);
        n2 = *(int32_t *)&this_3->f32;
      }
    }
    else
    {
      v159 = (Obj66 *)((uint16_t *)&((int32_t *)this_3)[v82 + 24]);
      n15_18 = v185->f0;
      arg_tot = *(uint16_t *)(n15_36 + 10);
      if ( n15_18 == n15_3 )
      {
        v161 = v159->f0;
        n2_2 = 1;
      }
      else if ( n15_18 == n15_7 )
      {
        v161 = v159->f0 + v159->f2;
        n2_2 = 2;
      }
      else if ( n15_18 == n15_4 )
      {
        v161 = v159->f0 + v159->f4 + v159->f2;
        n2_2 = 3;
      }
      else if ( n15_18 == n15_2 )
      {
        v161 = v159->f10 - v159->f8;
        n2_2 = 4;
      }
      else
      {
        v161 = 0;
        n2_2 = 0;
      }
      arg_cum = v161;
      arg_high = *(uint16_t *)(n15_31 + 2 * n2_2) + v161;
      p_n15_10 = *(uint16_t *)(n15_31 + 10);
      p_n15_8 = *(uint16_t *)(n15_31 + 12);
      n15_20 = *(uint8_t *)(n15_31 + 15);
      if ( p_n15_10 > p_n15_8 && (*(uint16_t *)(n15_31 + 2 * n2_2) + n15_20 + 8 < p_n15_10 || p_n15_10 > 0x4000) )
      {
        p_n15 = p_n15_8;
        this_1 = (ModelBlock *)(this_3);
        n15_39 = (uint16_t *)n15_31;
        v168 = *(uint16_t *)(n15_31 + 2);
        v169 = *(uint16_t *)(n15_31 + 4);
        n2_1 = (int32_t)n2_3;
        n2_4 = (uint16_t *)n2_2;
        v170 = *(uint16_t *)n15_31 - (*(uint16_t *)n15_31 >> 1);
        *(uint16_t *)n15_31 = v170;
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
        n2_2 = (int32_t)n2_4;
        p_n15_10 = (uint16_t)(v173 + v172 + v171 + v168);
        p_n15_9 = p_n15;
        n2_3 = (int32_t *)n2_1;
        n15_39[5] = p_n15_10;
        this_3 = (ModelBlock *)(this_1);
        if ( p_n15_9 < 256 && !*(uint8_t *)(n15_31 + 14) )
        {
          p_n15_9 = 256;
          *(uint16_t *)(n15_31 + 12) = 256;
        }
        if ( p_n15_10 > p_n15_9 )
        {
          n15_19 = n15_20;
          if ( n15_20 < 15 )
            n15_19 = 15;
          n15_20 = n15_19;
          *(uint8_t *)(n15_31 + 15) = n15_19;
        }
      }
      n15_21 = n15_20;
      n15_40 = (char *)n15_31;
      *(uint16_t *)(n15_31 + 10) = n15_20 + p_n15_10;
      *(uint16_t *)(n15_40 + 2 * n2_2) += n15_21;
      rc.encode(arg_cum, arg_high, arg_tot);
      *(int32_t *)&this_3->f32 = n2_2;
      *((uint16_t *)n2_3 + 5) = (*((uint16_t *)n2_3 + n2_2))++ != 0;
      n2 = *(int32_t *)&this_3->f32;
    }
    if ( n2 )
      return 1;
    n15_12 = 0;
  }
  v93 = __byte_445700;
  n15_22 = ::tbl44573C[3];
  excl_sym_a = ::tbl44573C[2];
  excl_sym_b = ::tbl44573C[1];
  exclusion_mask[tbl44573C[4]] = __byte_445700;
  exclusion_mask[n15_22] = v93;
  v97 = (uint16_t *)this_3->f6059436;
  exclusion_mask[excl_sym_a] = v93;
  exclusion_mask[excl_sym_b] = v93;
  v98 = (uint16_t *)*(int32_t *)&this_3->f6059432;
  __byte_445440[0] = v93;
  n15_14 = n15_12;
  *(int32_t *)&this_3->f1078216 = 0;
  p_n15_11 = *v97;
  n2_5 = v97[1];
  n2_6 = (uint16_t *)*v98;
  n15_41 = v98[2];
  p_n15 = p_n15_11;
  n15_23 = v98[1];
  n2_1 = n2_5;
  n15_24 = v98[3];
  n2_4 = n2_6;
  n15_25 = v98[4];
  n15_20 = n15_23;
  this_4 = (ModelBlock *)((int32_t *)v98[5]);
  n15_31 = n15_41;
  n15_26 = v98[6];
  v108 = (Obj34 *)((uint16_t *)v98[7]);
  n15_2 = n15_24;
  v109 = this_3->f56[5];
  n15_4 = n15_25;
  n15_27 = *(uint16_t *)(v109 - 16);
  this_1 = (ModelBlock *)(this_4);
  v111 = (Obj121 *)((uint16_t *)this_3->f56[6]);
  n15_3 = n15_26;
  v112 = v111->f16;
  v185 = (Obj34 *)(v108);
  n15_7 = n15_27;
  v113 = (uint16_t *)this_3->f56[7];
  v114 = v113[4];
  v187 = v112;
  v115 = *v113;
  v188 = v114;
  v116 = *((uint16_t *)v111 - 8);
  v189 = v115;
  v117 = *(v113 - 4);
  v190 = v116;
  v118 = *(uint16_t *)(v109 - 24);
  v191 = v117;
  v119 = v111->f24;
  v192 = v118;
  v120 = v111->f32;
  v193 = v119;
  v121 = *(uint16_t *)(v109 - 32);
  v194 = v120;
  v122 = *((uint16_t *)v111 - 12);
  v195 = v121;
  v123 = v113[8];
  v196 = v122;
  v197 = v123;
  v124 = (uint16_t *)this_3->f56[8];
  v198 = *v124;
  v199 = *(v113 - 8);
  v125 = *(uint16_t *)(v109 - 40);
  v126 = *(uint16_t *)(v109 - 56);
  v200 = v125;
  v201 = v124[4];
  v127 = v111->f40;
  v128 = v111->f56;
  v202 = v127;
  v203 = *(uint16_t *)*(int32_t *)&this_3->f56[9];
  v204 = v126;
  v205 = *(v124 - 4);
  v206 = v128;
  v207 = v113[12];
  *(int32_t *)&this_3->f44 = 0;
  do
  {
    v129 = __fwd_code_pixel_pixel_context(this_3, __frame.sym);
    if ( v129 >= 0 )
    {
      n15_28 = v129 == *(uint16_t *)this_3->f56[5];
      __fwd_code_pixel_encode_context_bit(
        (uint16_t *)this_3 + 3 * *(int32_t *)&this_3->f52 + 525888,
        (uint16_t *)this_3 + 3 * *(int32_t *)&this_3->f48 + 525840,
        n15_28);
      if ( n15_28 )
        return n15_14 + 1;
      exclusion_mask[v129] = __byte_445700;
    }
    n32 = *(int32_t *)&this_3->f44 + 1;
    *(int32_t *)&this_3->f44 = n32;
  }
  while ( n32 < 32 );
  n15_29 = n15_14;
  n4_5 = ::tbl44573C[1];
  v134 = *(int32_t *)&this_3->f1078208;
  *(int32_t *)&this_3->f1078216 = *(int32_t *)&this_3->f1078212 + 24 * ::tbl44573C[2];
  v135 = v134 + 24 * n4_5;
  v136 = (uint32_t **)*(int32_t *)&this_3->f1078232;
  *(int32_t *)&this_3->f1078220 = v135;
  while ( 1 )
  {
    if ( (*v136)[1] )
    {
      if ( __fwd_code_pixel_encode_symbol_list(*v136, *(uint16_t *)this_3->f56[5]) )
        return n15_29 + 1;
      v136 = (uint32_t **)*(int32_t *)&this_3->f1078232;
    }
    *(int32_t *)&this_3->f1078232 = (int32_t)++v136;
  }
}
static inline int32_t __fwd_expand_alphabet_decode_encode_symbol_list(void *a0) { return __decode_symbol_list((uint32_t *)a0); }
static inline uint32_t __fwd_expand_alphabet_init_encode_symbol_list(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __init_symbol_list((int32_t *)a0, a1, a2, a3); }

void __expand_alphabet(ModelBlock *_this)
{
  struct alignas(16) {   // 420 bytes, the frame Hex-Rays could not name
      uint64_t v28[2];
      int32_t v29;
      uint32_t v30[91];
      uint32_t v31[5];
      uint8_t _pad0[16];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 432, "frame layout moved");
  uint64_t (&v28)[2] = __frame.v28;
  int32_t &v29 = __frame.v29;
  uint32_t (&v30)[91] = __frame.v30;
  uint32_t (&v31)[5] = __frame.v31;
  ;
  char *v16;   // was int32_t: these hold addresses
  bool v24;
  int32_t n8, v4, v6, v9, n16_1, n16, v25, v27;
  uint32_t j_2, i, n8193, v8, *v10, j_1, j, v15, v17, v18, v19, v20, v21, *v22, v26;
  void *v12;
  n8 = *((uint32_t *)_this + 2);
  j_2 = 0xFFFFFFFF >> (-*((uint8_t *)_this + 8) & 31);
  v4 = (n8 + 7) >> 3;
  for ( i = 0; i < 8; ++i )
  {
    v6 = 12 * i;
    v30[v6] = 0;
    v30[v6 + 6] = 0;
  }
  n8193 = j_2 + 1;
  if ( n8 > 8 )
    n8193 = 8193;
  v8 = __rc_decode_flat(n8193);
  *((uint32_t *)_this + 4) = v8 + 1;
  if ( (int32_t)(v8 + 1) <= 0x2000 )
  {
    v12 = bmf_new(4 * v8 + 4);
    j_1 = *((uint32_t *)_this + 4);
    *((void **)_this + 269560) = v12;
    if ( j_1 )
    {
      for ( j = 0; j < j_1; ++j )
      {
        *(uint32_t *)(*((uint32_t *)_this + 269560) + 4 * j) = j;
        j_1 = *((uint32_t *)_this + 4);
      }
    }
    if ( (int32_t)*((uint32_t *)_this + 2) > 8 )
    {
      if ( 4 * v4 )
      {
        v15 = 0;
        do
          __fwd_expand_alphabet_init_encode_symbol_list((int32_t *)&v28[3 * v15++], (int32_t)_this, 256, 1);
        while ( v15 < 4 * v4 );
        j_1 = *((uint32_t *)_this + 4);
      }
      if ( j_1 )
      {
        v16 = (char *)*((void **)_this + 269560);
        v17 = 0;
        v18 = 0;
        do
        {
          *(uint32_t *)(v16 + 4 * v18) = 0;
          if ( v4 )
          {
            v31[0] = v4;
            v19 = 0;
            do
            {
              v20 = __fwd_expand_alphabet_decode_encode_symbol_list((uint32_t *)&v28[12 * v19 + 3 * v17]);
              v21 = v20 << ((8 * v19) & 31);
              v17 = v20 >> 6;
              *(uint32_t *)(*((uint32_t *)_this + 269560) + 4 * v18) += v21;
              ++v19;
            }
            while ( v19 < v31[0] );
            v4 = v31[0];
          }
          v16 = (char *)*((void **)_this + 269560);
          v17 = *(uint8_t *)(v16 + 4 * v18++) >> 7;
        }
        while ( v18 < *((uint32_t *)_this + 4) );
      }
    }
    else if ( j_1 <= j_2 )
    {
      __fwd_expand_alphabet_init_encode_symbol_list((int32_t *)v28, (int32_t)_this, j_2 - j_1 + 2, 1);
      v24 = *((uint32_t *)_this + 4) == 0;
      v29 = 19 * LODWORD(v28[0]);
      if ( !v24 )
      {
        v25 = 0;
        v26 = 0;
        do
        {
          v27 = __fwd_expand_alphabet_decode_encode_symbol_list(v28);
          *(uint32_t *)(*((uint32_t *)_this + 269560) + 4 * v26) = v27 + v25;
          v25 += v27 + 1;
          ++v26;
        }
        while ( v26 < *((uint32_t *)_this + 4) );
      }
    }
    v22 = v31;
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
    v9 = *((uint32_t *)_this + 1) * v4;
    *((uint32_t *)_this + 2) = 8;
    *((uint32_t *)_this + 1) = v9;
    __expand_alphabet(_this);
    v10 = v31;
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

int32_t __layout_workspace(uintptr_t a1, int32_t a2, int32_t i, int32_t a4, int32_t a5)
{
  ;
  char *v8, v12;
  int16_t v19;
  int32_t i_1, j, v9, v13, k_1, v35, v38;
  uint32_t k, m, n0x2000_1, n, n8, n0x18;
  uint8_t *v10;
  i_1 = i;
  __byte_445700 = 1;
  *(uint32_t *)a1 = i;
  *(uint32_t *)(a1 + 4) = a4;
  *(uint32_t *)(a1 + 8) = a5;
  *(uint32_t *)(a1 + 12) = a5;
  *(uint32_t *)(a1 + 1078204) = 0;
  *(uint32_t *)(a1 + 1078240) = 0;
  for ( j = 0; j < 5; ++j )
  {
    v8 = (char *)bmf_new(8 * i_1 + 128);
    *(char **)(a1 + 4 * j + 56) = v8;
    *(char **)(a1 + 4 * j + 76) = v8 + 64;
    i_1 = *(uint32_t *)a1;
    if ( *(int32_t *)a1 > -16 )
    {
      v9 = 0;
      do
      {
        *(uint16_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9) = 0;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 7) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 6) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 5) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 4) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 3) = 1;
        *(uint8_t *)(*(uint32_t *)(a1 + 4 * j + 56) + 8 * v9 + 2) = 1;
        i_1 = *(uint32_t *)a1;
        ++v9;
      }
      while ( v9 < *(uint32_t *)a1 + 16 );
    }
  }
  v10 = (uint8_t *)bmf_new(i_1 + 1);
  *(uint8_t **)(a1 + 1078684) = v10;
  *v10 = 0;
  if ( *(int32_t *)a1 > 0 )
  {
    v12 = 0;
    v13 = 0;
    do
    {
      v12 += v13 == 2 << (v12 & 31);
      *(uint8_t *)(*(uint32_t *)(a1 + 1078684) + v13++ + 1) = v12;
    }
    while ( v13 < *(uint32_t *)a1 );
  }
  // 0x2000 sixteen-bit counters cleared.  What was here instead was the same
  // range in three passes -- a scalar head to reach sixteen-byte alignment,
  // thirty-two counters an iteration, a scalar tail -- with a branch for the
  // case where `a1` is odd and no alignment is reachable at all.
  __builtin_memset((void *)(a1 + 6059440), 0, 2 * 0x2000);
  for ( k = 0; k < 0x2000; ++k )
  {
    v19 = *(uint16_t *)(a1 + 2 * k + 6059440);
    k_1 = k;
    for ( m = 0; m < 0xD; ++m )
    {
      v19 += v19 + (k_1 & 1);
      k_1 >>= 1;
    }
    *(uint16_t *)(a1 + 2 * k + 6059440) = v19;
  }
  // ... and every one of them scaled by eight, the same range in the same
  // three passes.
  for ( n0x2000_1 = 0; n0x2000_1 < 0x2000; ++n0x2000_1 )
    *(uint16_t *)(a1 + 2 * n0x2000_1 + 6059440) *= 8;
  memset((char *)(a1 + 3104),0,0x100000);
  *(uint32_t *)(a1 + 28) = 0;
  *(uint32_t *)(a1 + 24) = 0;
  *(uint32_t *)(a1 + 20) = 0;
  memset((char *)(a1 + 6075824),255,385024);
  memset((char *)(a1 + 6460848),255,217600);
  memset((char *)(a1 + 6678448),255,1424000);
  memset(exclusion_mask,0,8193);
  *(uint64_t *)(a1 + 1078216) = 0;
  *(uint64_t *)(a1 + 1078224) = 0;
  for ( n = 0; n < 0x40000; ++n )
  {
    *(uint16_t *)(a1 + 4 * n + 1078696) = 0x2000;
    *(uint16_t *)(a1 + 4 * n + 1078698) = 0x2000;
  }
  n8 = 0;
  do
  {
    v35 = 6 * n8;
    *(uint16_t *)(a1 + 2 * v35 + 1051680) = 40;
    ++n8;
    *(uint16_t *)(a1 + 2 * v35 + 1051682) = 16;
    *(uint16_t *)(a1 + 2 * v35 + 1051684) = 512;
    *(uint16_t *)(a1 + 2 * v35 + 1051686) = 40;
    *(uint16_t *)(a1 + 2 * v35 + 1051688) = 16;
    *(uint16_t *)(a1 + 2 * v35 + 1051690) = 512;
  }
  while ( n8 < 8 );
  n0x18 = 0;
  memset((char *)(a1 + 1051776),0,24576);
  *(uint32_t *)(a1 + 1078236) = (uint32_t)bmf_new(2 * *(uint32_t *)(a1 + 4) * *(uint32_t *)a1);
  *(uint16_t *)(a1 + 1076352) = 4;
  *(uint16_t *)(a1 + 1076354) = 4;
  *(uint16_t *)(a1 + 1076356) = 72;
  memset((char *)(a1 + 1076358),0,1536);
  do
  {
    v38 = 6 * n0x18;
    *(uint16_t *)(a1 + 2 * v38 + 1077894) = 4;
    ++n0x18;
    *(uint16_t *)(a1 + 2 * v38 + 1077896) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077898) = 72;
    *(uint16_t *)(a1 + 2 * v38 + 1077900) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077902) = 4;
    *(uint16_t *)(a1 + 2 * v38 + 1077904) = 72;
  }
  while ( n0x18 < 0x18 );
  return a1;
}
static inline uint32_t __fwd_unmodel_plane_slow_init_encode_symbol_list(void *a0, void *a1, int32_t a2, int32_t a3) { return __init_symbol_list((int32_t *)a0, (int32_t)a1, a2, a3); }
static inline int32_t __fwd_unmodel_plane_slow_decode_pixel(void *a0, int32_t a1) { return __decode_pixel((ModelBlock *)a0, a1); }
static inline void __fwd_unmodel_plane_slow_expand_alphabet(void *a0) { __expand_alphabet((ModelBlock *)a0); }

void __unmodel_plane_slow(ModelBlock *_this, char *Src)
{
  struct alignas(16) {   // 100 bytes, the frame Hex-Rays could not name
      uint8_t   _gap0[4];   // was int32_t Size
      uint8_t   _pad1[4];
      int32_t   v82;
      char *    v85;
      uint8_t   _gap1[4];   // was int32_t v88
      char *    ArgList_1;
      char *row[19];   // the row-pointer array; the loop fills (&v92)[k] for k < n6, five at a time
      uint8_t   _tail[12];   // alignas(16) rounds 100 up
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 112,
                "frame layout moved");
  int32_t Size;
  int32_t &n4 = __frame.v82;
  int32_t &v82 = __frame.v82;
  int32_t &v84 = __frame.v82;
  char * &v85 = __frame.v85;
  int32_t &v86 = *(int32_t *)((char *)&__frame.v85);
  int32_t &v87 = *(int32_t *)((char *)&__frame.v85);
  int32_t v88;
  char * &ArgList_1 = __frame.ArgList_1;
  char * &ArgList_5 = __frame.ArgList_1;
  int32_t &n6_3 = *(int32_t *)((char *)&__frame.ArgList_1);
  char *v92;
  int32_t v93;
  int32_t v94;
  int32_t v95;
  int32_t ArgList_4;
  char *Src_1;
  ModelBlock *this_1;
  int32_t v99;
  int32_t v100;
  int32_t v101;
  int32_t v102;
  ;
  ModelBlock *this_4;
  char *v57;   // were int32_t: these hold addresses
  bool v38;
  char *ArgList, *v9, *ArgList_2, *buf, *ArgList_3, *ArgList_9, *ArgList_10,
       *Src_2, *v77, *ArgList_8;
  int16_t v20;
  uint8_t *v27, *v28, *v29, *v44, *v45, *v46, *v47, *v48;   // row cursors out of f56
  int32_t v3, v5, v6, v8, n5, v11, v14, v15, v16, v17, v18, v19, n0x10000, v30,
          v34, v39, v40, n4_1, v43, v51, v52, v53, v54, v56, v58, v60, v61, v62,
          v64, v65, v66, n6, v68, v69, n6_4, n6_1, v73, n6_2, v76, v78, v80;
  ModelBlock *this_3;
  ModelBlock *this_2;
  uint32_t *v13, *v22, *v31, *i_1, *i, *v35, *j_1, *j, *ArgList_7, *ArgList_6;
  uint8_t *v49, *v50;
  v3 = *((uint32_t *)_this + 2) < 8;
  Src_1 = (char *)Src;
  ArgList = &Src[-v3];
  __rc_begin_decode(0);
  __fwd_unmodel_plane_slow_expand_alphabet(_this);
  ArgList_1 = ArgList;
  this_1 = (ModelBlock *)(_this);
  v102 = 0;
  v5 = 0;
  do
  {
    v6 = (uint8_t)ctx_group_flags[v5];
    this_2 = (ModelBlock *)(this_1);
    *((uint8_t *)this_1 + v6 + 1078244) = v5;
    v100 = 0;
    v82 = v5;
    v92 = (char *)this_2 + v5;
    v88 = v6 & 4;
    v99 = v6 & 2;
    v95 = v6 & 0x10;
    v8 = 0;
    v93 = v6 & 1;
    v9 = (char *)this_2 + v5;
    ArgList_4 = v6 & 0x20;
    v94 = v6 & 8;
    do
    {
      v100 = v8;
      n5 = 0;
      v85 = &v9[15 * v8];
      do
      {
        v11 = v102;
        this_3 = (ModelBlock *)(this_1);
        v85[75 * n5 + 1078308] = v102;
        v101 = this_3->f16;
        v13 = &((uint32_t *)this_3)[4 * v11];
        *((uint16_t *)v13 + 49) = 2;
        *((uint16_t *)v13 + 50) = 2;
        *((uint16_t *)v13 + 51) = 2;
        *((uint16_t *)v13 + 52) = 2;
        if ( v88 )
        {
          v14 = (uint16_t)(*((uint16_t *)v13 + 52) + *((uint16_t *)v13 + 51));
          *((uint16_t *)v13 + 51) = v14;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        else
        {
          v14 = *((uint16_t *)v13 + 51);
          v15 = *((uint16_t *)v13 + 52);
        }
        if ( v99 )
        {
          v16 = (uint16_t)(v15 + *((uint16_t *)v13 + 50));
          *((uint16_t *)v13 + 50) = v16;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        else
        {
          v16 = *((uint16_t *)v13 + 50);
        }
        if ( v95 )
        {
          v16 = (uint16_t)(v14 + v16);
          *((uint16_t *)v13 + 50) = v16;
          v14 = 0;
          *((uint16_t *)v13 + 51) = 0;
        }
        if ( v93 )
        {
          *((uint16_t *)v13 + 49) += v15;
          v15 = 0;
          *((uint16_t *)v13 + 52) = 0;
        }
        if ( v94 )
        {
          *((uint16_t *)v13 + 49) += v14;
          v14 = 0;
          *((uint16_t *)v13 + 51) = 0;
        }
        if ( ArgList_4 )
        {
          *((uint16_t *)v13 + 49) += v16;
          v16 = 0;
          *((uint16_t *)v13 + 50) = 0;
        }
        v17 = (v14 != 0) + (v16 != 0) + (v15 != 0) + 2;
        if ( v17 <= v101 )
        {
          *((uint8_t *)v13 + 110) = v17;
          *((uint16_t *)v13 + 48) = 2;
        }
        else
        {
          LOBYTE(v17) = v17 - 1;
          *((uint8_t *)v13 + 110) = v17;
          *((uint16_t *)v13 + 48) = 0;
        }
        if ( *((uint16_t *)v13 + v100 + 48) && *((uint16_t *)v13 + n5 + 48) && (uint8_t)v17 <= v101 )
        {
          v19 = v100;
          v18 = 1;
          v20 = (uint8_t)(1 << ((5 - v17) & 31));
          *((uint8_t *)v13 + 111) = v20;
          *((uint16_t *)v13 + 54) = v20 << 6;
          *((uint16_t *)v13 + v19 + 48) += v20;
          *((uint16_t *)v13 + n5 + 48) += *((uint8_t *)v13 + 111);
          *((uint16_t *)v13 + 53) = *((uint16_t *)v13 + 48)
                               + *((uint16_t *)v13 + 52)
                               + *((uint16_t *)v13 + 51)
                               + *((uint16_t *)v13 + 50)
                               + *((uint16_t *)v13 + 49);
        }
        else
        {
          v18 = 0;
        }
        v102 += v18;
        ++n5;
      }
      while ( n5 < 5 );
      v9 = v92;
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
  buf = (char *)bmf_new(this_1->f16);
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
  this_4->f1078224 = (uintptr_t)this_4 + 1078184;
  __fwd_unmodel_plane_slow_init_encode_symbol_list((int32_t *)((uintptr_t)this_4 + 1078184), this_4, this_4->f16, 1);
  this_4->f1078232 = (uint32_t **)((char *)this_4 + 1078216);
  v30 = this_4->f16;
  v31 = (uint32_t *)bmf_new(24 * v30 + 4);
  if ( v31 )
  {
    *v31 = v30;
    i_1 = v31 + 1;
    for ( i = i_1; v30; --v30 )
    {
      i_1[5] = 0;
      i_1 += 6;
    }
  }
  else
  {
    i = nullptr;
  }
  v34 = this_4->f16;
  this_4->f1078208 = (uint8_t *)i;
  v35 = (uint32_t *)bmf_new(24 * v34 + 4);
  if ( v35 )
  {
    *v35 = v34;
    j_1 = v35 + 1;
    for ( j = j_1; v34; --v34 )
    {
      j_1[5] = 0;
      j_1 += 6;
    }
  }
  else
  {
    j = nullptr;
  }
  v38 = this_4->f16 <= 0;
  this_4->f1078212 = (uint8_t *)j;
  if ( !v38 )
  {
    v39 = 0;
    do
    {
      __fwd_unmodel_plane_slow_init_encode_symbol_list((int32_t *)(this_4->f1078208 + 24 * v39), this_4, 99, 0);
      __fwd_unmodel_plane_slow_init_encode_symbol_list((int32_t *)(this_4->f1078212 + 24 * v39++), this_4, 33, 0);
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
    ArgList_2 = (char *)bmf_new(*(uint32_t *)&this_4->f4 * this_4->f0 + 3);
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
        v54 = __fwd_unmodel_plane_slow_decode_pixel((uint32_t *)this_4, v53);
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
        ArgList_3 = (char *)ArgList_4;
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
          v57 = (char *)this_4->f1078240;
          v58 = *(uint16_t *)(this_4->f56[0] + 8 * v56 + 64);
          *(uint16_t *)ArgList_7 = *(uint16_t *)(v57 + 4 * v58);
          *((uint8_t *)ArgList_7 + 2) = *(uint8_t *)(v57 + 4 * v58 + 2);
          ++v56;
          ArgList_7 = (uint32_t *)((char *)ArgList_7 + 3);
        }
        while ( v56 < this_4->f0 );
        ArgList_5 = (char *)ArgList_7;
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
        ArgList_6 = (uint32_t *)((char *)ArgList_6 + 2);
      }
      while ( v60 < this_4->f0 );
LABEL_73:
      ArgList_5 = (char *)ArgList_6;
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
          __frame.row[n6_4 + 1] = (int32_t)&ArgList_3[v69 * (n6_4 + 1)];
          __frame.row[n6_4 + 2] = (int32_t)&ArgList_3[v69 * (n6_4 + 2)];
          __frame.row[n6_4 + 3] = (int32_t)&ArgList_3[v69 * (n6_4 + 3)];
          __frame.row[n6_4 + 4] = (int32_t)&ArgList_3[v69 * (n6_4 + 4)];
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
      ArgList_3 = (char *)ArgList_4;
    }
    free(ArgList_3);
  }
}
static inline int32_t __fwd_alt_model_p1_encode_alt_p1_encode_symbol(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_encode_symbol((uint16_t *)a0, a1, a2, a3); }
static inline void ** __fwd_alt_model_p1_encode_alt_p1_free(void *a0, char a1) { return __alt_p1_free((void **)a0, a1); }
static inline int32_t __fwd_alt_model_p1_encode_alt_p1_context(void *a0, void *a1, int32_t a2) { return __alt_p1_context((Obj25 *)a0, (uint32_t *)a1, (Obj91 *)a2); }
static inline int32_t * __fwd_alt_model_p1_encode_alt_p1_alloc(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __alt_p1_alloc((Obj0 *)a0, a1, a2, a3); }

int32_t __alt_model_p1_encode(uint16_t *p_i, char *a2)
{
  struct alignas(16) {   // 144 bytes, the frame Hex-Rays could not name
      uint8_t   _gap0[1];   // was char v90
      uint8_t _pad0[3];
      uint8_t slot4[4];
      uint8_t slot8[4];
      uint8_t   _gap1[4];   // was char * v95
      uint8_t   _gap2[4];   // was int32_t v96
      uint8_t   _gap3[1];   // was char v97
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
  char v90;
  uint32_t &v91 = *(uint32_t *)((char *)__frame.slot4);
  uint32_t &v92 = *(uint32_t *)((char *)__frame.slot4);
  int32_t &v93 = *(int32_t *)((char *)__frame.slot8);
  int32_t &v94 = *(int32_t *)((char *)__frame.slot8);
  char *v95;
  int32_t v96;
  char v97;
  int32_t v98;
  int32_t v99;
  int32_t n5_8;
  void * Block_plane[4];
  Obj25 * &v102 = (Obj25 * &)Block_plane[1];
  Obj25 * &v103 = (Obj25 * &)Block_plane[2];
  Obj25 * &v104 = (Obj25 * &)Block_plane[3];
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
  Obj1 *v23;
  uint8_t *v28;   // a row cursor out of Obj1
  char v11, v12, v13, v62, v71, v81, v83;
  Obj0 *v6;
  uint8_t *v24, *v25, *v26, *v27;   // row cursors out of Obj1
  int32_t i, v3, i_2, n4, *v7, v8, v9, v10, v14, v15, v16, v17, i_3, n4_1, n4_2,
          v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46,
          v47, v48, v49, n5_9, n5_7, n5_2, n5_1, v56, n16, v58, v59, v63, v64, v65, n16_1, n5_3,
          v69, n5_4, v73, v74, n16_2, v77, n5_5, v84, n16_3, n4_3, n4_4;
  int64_t v68, v76, v86;
  uint32_t v19;
  Obj25 *v61;
  Obj25 *v70;
  Obj25 *v80;
  uint8_t *v29;
  uint8_t *v30;
  uint8_t *v50, v53, v60, v79;
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
      v6 = (Obj0 *)((int32_t *)bmf_new(0x99D4D8u));
      if ( v6 )
        v7 = __fwd_alt_model_p1_encode_alt_p1_alloc(v6, i_2, v3, n4);
      else
        v7 = nullptr;
      Block_plane[n4++] = v7;
    }
    while ( n4 < plane_count );
  }
  v8 = 16 * (uint8_t)plane_desc[2].b1;
  v9 = 16 * (uint8_t)plane_desc[3].b1;
  v10 = 16 * (uint8_t)__n3_0;
  v11 = __byte_44339E[v8];
  v12 = __byte_44339E[v9];
  v13 = __byte_44339E[v10];
  v14 = (uint8_t)__byte_44339F[v9];
  LOBYTE(v10) = __byte_44339F[v10];
  v105 = (uint8_t)__byte_44339F[v8];
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
        v91 = v19;
        n4_2 = 0;
        v93 = v3;
        v95 = a2;
        do
        {
          ++n4_2;
          v23 = (Obj1 *)Block_plane[n4_2 - 1];
          **(uint16_t **)&v23->cur[0] = *(uint16_t *)(v23->cur[0] - 2);
          *(uint16_t *)(v23->cur[0] + 2) = *(uint16_t *)(v23->cur[0] - 4);
          *(uint16_t *)(v23->cur[0] + 4) = *(uint16_t *)(v23->cur[0] - 6);
          *(uint16_t *)(v23->cur[0] + 6) = *(uint16_t *)(v23->cur[0] - 8);
          *(uint16_t *)(v23->cur[0] + 8) = *(uint16_t *)(v23->cur[0] - 10);
          *(uint16_t *)(v23->cur[0] + 10) = *(uint16_t *)(v23->cur[0] - 12);
          v24 = v23->row[4];
          v25 = v23->row[3];
          v26 = v23->row[2];
          v27 = v23->row[1];
          v28 = v23->row[0];
          v23->row[4] = v25;
          v23->row[3] = v26;
          v23->row[2] = v27;
          v23->row[1] = v28;
          v23->row[0] = v24;
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
          v23->f20 = 0;
          v23->f24 = 0;
          v23->f28 = 0;
          v31 = *((char *)v29 - 3);
          v23->f24 = v31;
          v32 = *((char *)v29 - 1);
          v23->f28 = v32;
          v33 = *((char *)v30 - 3) + v31;
          v23->f24 = v33;
          v34 = *((char *)v30 - 1) + v32;
          v23->f28 = v34;
          v35 = v29[1] + v33;
          v23->f24 = v35;
          v36 = v29[3] + v34;
          v23->f28 = v36;
          v37 = v30[1] + v35;
          v23->f24 = v37;
          v38 = v30[3] + v36;
          v23->f28 = v38;
          v39 = v29[5] + v37;
          v23->f24 = v39;
          v40 = v29[7] + v38;
          v23->f28 = v40;
          v41 = v30[5] + v39;
          v23->f24 = v41;
          v42 = v30[7] + v40;
          v23->f28 = v42;
          v43 = v29[9] + v41;
          v23->f24 = v43;
          v44 = v29[11] + v42;
          v23->f28 = v44;
          v45 = v30[9] + v43;
          v46 = (int32_t)(uintptr_t)v23->cur[0];
          v23->f24 = v45;
          v47 = v30[11] + v44;
          v23->f28 = v47;
          v48 = *(uint8_t *)(v46 - 7) + v45;
          v23->f24 = v48;
          v49 = *(uint8_t *)(v46 - 5) + v47;
          v23->f28 = v49;
          v23->f24 = *(uint8_t *)(v46 - 3) + v48;
          v23->f28 = *(uint8_t *)(v46 - 1) + v49;
          n4_1 = plane_count;
        }
        while ( n4_2 < plane_count );
        v19 = v91;
        v3 = v93;
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
          v50 = (uint8_t *)Block_plane[0];
          n5_9 = *(uint8_t *)(a2 + (uint8_t)plane_desc[1].b1);
          v114 = (uint8_t)plane_desc[1].b1;
          n5_6 = n5_9;
          __fwd_alt_model_p1_encode_alt_p1_context((uint8_t **)Block_plane[0], nullptr, 0);
          n5_7 = n5_6;
          v53 = v50[8];
          v112 = (uint8_t)(n5_6 - v53);
          n5_2 = v50[v112 + 984];
          n5_1 = (uint8_t)(v50[n5_2 + 1496] + v53);
          v56 = (uint8_t)(n5_1 + *(uint8_t *)(a2 + v114) - n5_6);
          n16 = *(uint8_t *)(a2 + v114) - v56;
          if ( n16 < -16 || n16 > 16 )
          {
            n5_2 = v50[v112 + 1240];
          }
          else
          {
            n5_6 = n5_1;
            *(uint8_t *)(a2 + v114) = v56;
            n5_7 = n5_1;
          }
          __fwd_alt_model_p1_encode_alt_p1_encode_symbol((uint16_t *)&v50[16 * *((uint32_t *)v50 + 3) + 3800], n5_1, *((uint32_t *)v50 + 4), n5_2);
          v58 = n5_7 - *((uint32_t *)v50 + 2);
          **((uint8_t **)v50 + 49) = n5_6;
          *(uint8_t *)(*((uint32_t *)v50 + 49) + 1) = abs32(v58);
          *(uint32_t *)&v50[4 * *((uint32_t *)v50 + 5) + 24] = *(uint32_t *)&v50[4 * *((uint32_t *)v50 + 5) + 24]
                                                         + *(uint8_t *)(*((uint32_t *)v50 + 49) + 1)
                                                         - *(uint8_t *)(*((uint32_t *)v50 + 49) - 7)
                                                         - (*(uint8_t *)(*((uint32_t *)v50 + 53) - 3)
                                                          - *(uint8_t *)(*((uint32_t *)v50 + 53) + 13)
                                                          + *(uint8_t *)(*((uint32_t *)v50 + 51) - 3)
                                                          - *(uint8_t *)(*((uint32_t *)v50 + 51) + 13));
          v59 = 16 * *((uint32_t *)v50 + 3);
          *((uint32_t *)v50 + 5) = *((uint32_t *)v50 + 5) == 0;
          if ( *(uint16_t *)&v50[v59 + 3800] < 0x4000u )
            __alt_p1_model((Obj0 *)v50);
          *((uint32_t *)v50 + 49) += 2;
          *((uint32_t *)v50 + 50) += 2;
          *((uint32_t *)v50 + 51) += 2;
          *((uint32_t *)v50 + 52) += 2;
          *((uint32_t *)v50 + 53) += 2;
          v60 = *(uint8_t *)(a2 + (uint8_t)plane_desc[2].b1);
          v113 = (uint8_t)plane_desc[2].b1;
          if ( v109 )
            v60 = v60 - v105 - *(uint8_t *)((uint8_t)plane_desc[1].b1 + a2);
          v61 = (Obj25 *)(v102);
          v118 = v60;
          __fwd_alt_model_p1_encode_alt_p1_context(v102, Block_plane[0], 0);
          v62 = *((uint8_t *)v61 + 8);
          v63 = (uint8_t)(v118 - v62);
          n5 = *((uint8_t *)v61 + v63 + 984);
          v64 = *(uint8_t *)(a2 + v113);
          v65 = (uint8_t)(*((uint8_t *)v61 + n5 + 1496) + v62);
          v111 = (uint8_t)(v65 + *(uint8_t *)(a2 + v113) - v118);
          n16_1 = v64 - v111;
          n5_3 = n5;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            n5_3 = *((uint8_t *)v61 + v63 + 1240);
          }
          else
          {
            *(uint8_t *)(a2 + v113) = v111;
            v118 = v65;
          }
          __fwd_alt_model_p1_encode_alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v61)[4 * (uint32_t)v61->f12 + 950], 16 * (uint32_t)v61->f12, (int32_t)v61->f16, n5_3);
          v68 = v118 - (int32_t)v61->f8;
          *v61->f196 = v118;
          v61->f196[1] = (BYTE4(v68) ^ v68) - BYTE4(v68);
          ((uint8_t**)v61)[(uint32_t)v61->f20 + 6] = &((uint8_t**)v61)[(uint32_t)v61->f20 + 6][v61->f196[1]
                                                           - *(v61->f196 - 7)
                                                           - (*(v61->f212 - 3)
                                                            - v61->f212[13])
                                                           - (*(v61->f204 - 3)
                                                            - v61->f204[13])];
          LODWORD(v68) = 16 * (uint32_t)v61->f12;
          v61->f20 = (uint8_t *)(v61->f20 == nullptr);
          if ( *(uint16_t *)((char *)v61 + v68 + 3800) < 0x4000u )
            __alt_p1_model((Obj0 *)v61);
          v61->f196 += 2;
          v61->f200 += 2;
          v61->f204 += 2;
          v61->f208 += 2;
          v61->f212 += 2;
          v69 = *(uint8_t *)((uint8_t)plane_desc[3].b1 + a2);
          v117 = (uint8_t)plane_desc[3].b1;
          if ( v108 )
            v69 = (uint8_t)(v69
                                  - v106
                                  - ((plane_desc[(uint8_t)plane_desc[3].b1 + 1].w4
                                    * *(uint8_t *)((uint8_t)plane_desc[1].b1 + a2)
                                    + plane_desc[(uint8_t)plane_desc[3].b1 + 1].w8
                                    * (uint32_t)*(uint8_t *)((uint8_t)plane_desc[2].b1 + a2)
                                    + 40) >> 7));
          v70 = (Obj25 *)(v103);
          __fwd_alt_model_p1_encode_alt_p1_context(v103, (uint32_t *)v102, (int32_t)Block_plane[0]);
          v71 = *((uint8_t *)v70 + 8);
          v115 = (uint8_t)(v69 - v71);
          n5_4 = *((uint8_t *)v70 + v115 + 984);
          v73 = (uint8_t)(*((uint8_t *)v70 + n5_4 + 1496) + v71);
          v74 = (uint8_t)(v73 + *(uint8_t *)(v117 + a2) - v69);
          n16_2 = *(uint8_t *)(v117 + a2) - v74;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            n5_4 = *((uint8_t *)v70 + v115 + 1240);
          }
          else
          {
            v69 = v73;
            *(uint8_t *)(v117 + a2) = v74;
          }
          __fwd_alt_model_p1_encode_alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v70)[4 * (uint32_t)v70->f12 + 950], n5_4, (int32_t)v70->f16, n5_4);
          v76 = v69 - (int32_t)v70->f8;
          *v70->f196 = v69;
          v70->f196[1] = (BYTE4(v76) ^ v76) - BYTE4(v76);
          ((uint8_t**)v70)[(uint32_t)v70->f20 + 6] = &((uint8_t**)v70)[(uint32_t)v70->f20 + 6][v70->f196[1]
                                                           - *(v70->f196 - 7)
                                                           - (*(v70->f212 - 3)
                                                            - v70->f212[13])
                                                           - (*(v70->f204 - 3)
                                                            - v70->f204[13])];
          v77 = 4 * (uint32_t)v70->f12;
          v70->f20 = (uint8_t *)(v70->f20 == nullptr);
          if ( LOWORD(((uint8_t**)v70)[v77 + 950]) < 0x4000u )
            __alt_p1_model((Obj0 *)v70);
          v70->f196 += 2;
          v70->f200 += 2;
          v70->f204 += 2;
          v70->f208 += 2;
          v70->f212 += 2;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            n3 = (uint8_t)__n3_0;
            if ( v96 )
              v79 = *(uint8_t *)((uint8_t)__n3_0 + a2)
                  - v90
                  - ((plane_desc[(uint8_t)__n3_0 + 1].w8 * *(uint8_t *)((uint8_t)__n3_0 + a2 - 2)
                    + plane_desc[(uint8_t)__n3_0 + 1].w4 * *(uint8_t *)((uint8_t)__n3_0 + a2 - 3)
                    + plane_desc[(uint8_t)__n3_0 + 1].w12 * *(uint8_t *)((uint8_t)__n3_0 + a2 - 1)
                    + 64) >> 7);
            else
              v79 = *(uint8_t *)((uint8_t)__n3_0 + a2);
            v80 = (Obj25 *)(v104);
            v99 = v79;
            __fwd_alt_model_p1_encode_alt_p1_context(v104, (uint32_t *)v103, (int32_t)v102);
            v81 = *((uint8_t *)v80 + 8);
            n5_5 = *((uint8_t *)v80 + (uint8_t)(v99 - v81) + 984);
            v98 = (uint8_t)(v99 - v81);
            v83 = *((uint8_t *)v80 + n5_5 + 1496);
            n5_8 = n5_5;
            v84 = (uint8_t)(v83 + v81);
            n16_3 = *(uint8_t *)(n3 + a2) - (uint8_t)(v84 + *(uint8_t *)(n3 + a2) - v99);
            v97 = v84 + *(uint8_t *)(n3 + a2) - v99;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              n5_5 = *((uint8_t *)v80 + v98 + 1240);
            }
            else
            {
              v99 = v84;
              *(uint8_t *)(n3 + a2) = v97;
            }
            __fwd_alt_model_p1_encode_alt_p1_encode_symbol((uint16_t *)&((uint8_t**)v80)[4 * (uint32_t)v80->f12 + 950], n5_5, (int32_t)v80->f16, n5_5);
            v86 = v99 - (int32_t)v80->f8;
            *v80->f196 = v99;
            v80->f196[1] = (BYTE4(v86) ^ v86) - BYTE4(v86);
            ((uint8_t**)v80)[(uint32_t)v80->f20 + 6] = &((uint8_t**)v80)[(uint32_t)v80->f20 + 6][v80->f196[1]
                                                             - *(v80->f196 - 7)
                                                             - (*(v80->f212 - 3)
                                                              - v80->f212[13])
                                                             - (*(v80->f204 - 3)
                                                              - v80->f204[13])];
            LODWORD(v86) = 16 * (uint32_t)v80->f12;
            v80->f20 = (uint8_t *)(v80->f20 == nullptr);
            if ( *(uint16_t *)((char *)v80 + v86 + 3800) < 0x4000u )
              __alt_p1_model((Obj0 *)v80);
            v80->f196 += 2;
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
        __fwd_alt_model_p1_encode_alt_p1_free(v89, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline int32_t __fwd_alt_p2_model_update_binary_pair(void *a0, int32_t a1) { return __update_binary_pair((Obj20 *)a0, a1); }
static inline uint32_t __fwd_alt_p2_model_rescale_three_way(void *a0) { return __rescale_three_way((uint16_t *)a0); }

uint32_t __alt_p2_model(Obj69 *a1, const __m128 &a2__ref, int32_t a3, uint8_t a4, int32_t a5)
{
  int32_t n2;
  uint16_t *n0xF0;
  uint32_t n0x10_2;
  uint16_t *v508;
  uint32_t n0x10_1;
  int32_t v510;
  int32_t v511;
  Obj15 *v512;
  Obj42 *v513;
  Obj44 *v514;
  Obj52 *v515;
  Obj41 *v516;
  Obj127 *v517;
  Obj51 *v518;
  Obj111 *v519;
  Obj112 *v520;
  Obj50 *v521;
  Obj109 *v522;
  Obj110 *v523;
  Obj49 *v524;
  Obj40 *v525;
  Obj108 *v526;
  Obj48 *v527;
  Obj107 *v528;
  Obj126 *v529;
  Obj47 *v530;
  Obj106 *v531;
  Obj125 *v532;
  Obj46 *v533;
  Obj104 *v534;
  Obj105 *v535;
  Obj15 *v536;
  Obj103 *v537;
  Obj39 *v538;
  Obj15 *v539;
  Obj102 *v540;
  Obj38 *v541;
  Obj23 *v542;
  Obj2 *v543;
  Obj124 *v544;
  uint32_t v545;
  int32_t n3;
  int32_t v547;
  int32_t v548;
  int32_t v549;
  int32_t v550;
  Obj46 *v551;
  Obj47 *v552;
  Obj125 *v553;
  Obj48 *v554;
  Obj126 *v555;
  Obj49 *v556;
  Obj108 *v557;
  Obj40 *v558;
  Obj50 *v559;
  Obj109 *v560;
  Obj51 *v561;
  Obj112 *v562;
  Obj111 *v563;
  Obj52 *v564;
  Obj127 *v565;
  Obj41 *v566;
  Obj15 *v567;
  Obj44 *v568;
  Obj42 *v569;
  int32_t v570;
  int32_t v571;
  int32_t v572;
  int32_t v573;
  int32_t v574;
  int32_t v575;
  int32_t v576;
  int32_t v577;
  Obj69 *v578;
  uint32_t n5;
  int32_t v580;
  uint32_t *v581;
  ;
  char *v8, *n2_1;   // were int32_t: these hold addresses
  __m128 a2 = a2__ref;
  __m128 *v15;
  __m128 *v17;
  __m128 v19;
  bool v87, v103, v104, v105;
  char *v90, v114, v116, v312;
  Obj15 *v110;
  Obj40 *v94;
  Obj41 *v111;
  Obj42 *v107;
  Obj44 *v112;
  Obj107 *v91;
  Obj111 *v93;
  float v16, v18, v20, v21, v22, v23, v24, v26;
  Obj42 *v98;
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
  Obj63 *v387;
  uint16_t *v7, *v389, *n0x10_3, *n2_2, *v408, *v411, *v420, *v428, *v436, *v444, *n2_7,
           *n0x10_4, *v452, *v456, *v457, *n2_6, *v463, *v466, *n2_5, *v474, *v477, *n2_4,
           *v485, *v488, *v491, *n2_3, *v502;
  Obj69 *v385;
  uint32_t *v76, v78, v92, v97, v109, n0x10, v393, v396, v405, v414, v499;
  uint8_t *v82, v83;
  v6 = a1->f278704 & 0xF;
  v7 = *(uint16_t **)&a1->f278736[0];
  v577 = 16 * a3;
  *v7 = 16 * a3;
  *(uint16_t *)(a1->f278736[0] + 2) = **(uint16_t **)&a1->f278736[0] - *(uint16_t *)(a1->f278736[0] + 2);
  *(uint16_t *)(a1->f278736[0] + 20) = 0;
  *(uint8_t *)(a1->f278736[0] + 16) = (a5 <= (int32_t)(((uint32_t)(6 - v6) >> 31)
                                                         + ((uint32_t)(4 - v6) >> 31)
                                                         + 2 * ((uint32_t)(9 - v6) >> 31)))
                                            + (a5 < (int32_t)-(((uint32_t)(6 - v6) >> 31)
                                                                + ((uint32_t)(4 - v6) >> 31)
                                                                + 2 * ((uint32_t)(9 - v6) >> 31)));
  *(uint8_t *)(a1->f278736[0] + 17) = abs32(a5);
  v8 = (char *)a1->f278736[0];
  v9 = v577;
  a2.m128_f32[0] = (float)v577;
  v19 = a2;
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
  v15 = (__m128 *)(a1->f278656);
  v16 = v15[14].m128_f32[1] + 0.000099999997f;
  v17 = (__m128 *)(*(__m128 **)(a1->f278668 - 4));
  v18 = *(float *)&a1->f278648;
  n2 = *(int32_t *)&a1->f278640;
  v19.m128_f32[0] = a2.m128_f32[0] - v18;
  v20 = *(float *)((char *)&a1->f278640 + 4);
  v21 = v20 - v18;
  v22 = ((((a2.m128_f32[0] - v18) * (v20 - v18)) - v15[14].m128_f32[0]) * 0.001f)
      + v15[14].m128_f32[0];
  v23 = v16 + (((v21 * v21) - v15[14].m128_f32[1]) * 0.001f);
  v15[14].m128_f32[1] = v23;
  v24 = 0.1f * v23;
  if ( (0.1f * v23) <= v22 )
    v24 = fminf(v23, v22);
  v15[14].m128_f32[0] = v24;
  // Two normalised-LMS updates side by side, on two weight blocks: `v15` at a
  // fixed mean-square rate and `v17` at one scaled by the confidence `v26`.
  // Same shape as `alt_p2_context`'s, run twice with different errors and
  // different floors.
  v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;
  n5 = 0;
  v577 = v9;
  v578 = (Obj69 *)((uint32_t *)a1);
  {
    const float err_a     = (a2.m128_f32[0] - v20) * 2.5999999f;
    const float err_b     = v19.m128_f32[0] * v26;
    const float floor_a   = 26896.0f * v15[14].m128_f32[2];
    const float floor_b   = 5041.0f * v17[14].m128_f32[2];
    const float ms_rate_b = 0.013f * v26;
    int32_t j, k;

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
      {
        float x = a1->f278528[j].m128_f32[k];
        float ms;

        ms = v15[7 + j].m128_f32[k]
           + (x * x - v15[7 + j].m128_f32[k]) * 0.05f;      // 0x439B40
        v15[7 + j].m128_f32[k] = ms;
        v15[j].m128_f32[k] += bmf_p2_rate[j][k] * err_a * x / (ms + floor_a);

        ms = v17[7 + j].m128_f32[k]
           + (x * x - v17[7 + j].m128_f32[k]) * ms_rate_b;
        v17[7 + j].m128_f32[k] = ms;
        v17[j].m128_f32[k] += bmf_p2_rate[j][k] * err_b * x / (ms + floor_b);
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
      acc[k] = v15[0].m128_f32[k] * a1->f278528[0].m128_f32[k];
      for ( j = 1; j < 7; ++j )
        acc[k] += v15[j].m128_f32[k] * a1->f278528[j].m128_f32[k];
    }
    pred     = *(float *)&n2 + bmf_hsum4(acc);
    err      = a2.m128_f32[0] - pred;
    ms_scale = v15[14].m128_f32[2];

    for ( j = 0; j < 7; ++j )
      for ( k = 0; k < 4; ++k )
        v15[j].m128_f32[k] += bmf_p2_rate[j][k] * err * a1->f278528[j].m128_f32[k]
                            / (v15[7 + j].m128_f32[k] + ms_scale * 529.0f);

    ++v15[15].m128_i32[0];
    v15[14].m128_f32[2] = ms_scale + ((10.0f - ms_scale) * 0.00019999999f);
  }
  **(uint32_t **)&a1->f278668 = *(uint32_t *)&a1->f278656;
  a1->f278668 += 4;
  a1->f278672 += 4;
  do
  {
    v75 = ((uint32_t *)v578)[n5 + 69669];
    v76 = &((uint32_t *)v578)[v75];
    v77 = v577 - ((uint32_t *)v578)[2 * n5 + 69726];
    v581 = v76;
    v78 = n5 << 17;
    v79 = v77 + HIWORD(v76[0x8000 * n5 + 71178]);
    *(uint16_t *)((char *)v76 + v78 + 284714) = v79;
    v580 = *((uint8_t *)v76 + v78 + 284713);
    if ( v580 )
    {
      v576 = v75;
      v80 = v79 + 4 * ((v77 > deadzone_hi) - (v77 < deadzone_lo));
      *(uint16_t *)((char *)v581 + v78 + 284714) = v80;
      v575 = v80;
      v81 = v576;
      if ( (int32_t)abs32(v77) < 38 )
      {
        if ( (uint8_t)--v580 )
        {
          *((uint8_t *)v581 + v78 + 284713) = v580;
        }
        else
        {
          v82 = (uint8_t *)v581;
          if ( *((uint8_t *)v581 + v78 + 284712) < 8u )
          {
            v83 = *((uint8_t *)v581 + v78 + 284712) + 1;
            *((uint8_t *)v581 + v78 + 284712) = v83;
            v82[v78 + 284713] = *((uint8_t *)&__dword_439B7C + v83 + 3);
            *(uint16_t *)&v82[v78 + 284714] = 2 * v575;
          }
          else
          {
            *((uint8_t *)v581 + v78 + 284713) = v580;
          }
        }
      }
      if ( !__dword_443388 )
      {
        __builtin_prefetch((const char *)&n2, 0, 1);
        n2 = (int32_t)&((uint32_t *)v578)[(v81 ^ 0x7FF0) + 71178] + v78;
        v84 = ((uint32_t *)v578)[2 * n5 + 69727] + v77;
        n3 = v81 & 3;
        if ( (uint32_t)n3 >= 3
          || (v545 = v78,
              v576 = v81,
              v85 = *(int16_t *)((char *)v581 + v78 + 284718),
              v86 = v84 - ((v85 + (1 << ((*((uint8_t *)v581 + v78 + 284716) + 31) & 31))) >> (*((uint8_t *)v581 + v78 + 284716) & 31)),
              v87 = n3 <= 0,
              *(uint16_t *)((char *)v581 + v78 + 284718) = v85
                                                      + ((32
                                                        * ((v86 > deadzone_hi) - (uint32_t)(v86 < deadzone_lo))
                                                        + v86
                                                        + 1) >> 1),
              v81 = v576,
              !v87) )
        {
          v545 = v78;
          v576 = v81;
          v88 = *(int16_t *)((char *)v581 + v78 + 284710);
          v89 = v84 - ((v88 + (1 << ((*((uint8_t *)v581 + v78 + 284708) + 31) & 31))) >> (*((uint8_t *)v581 + v78 + 284708) & 31));
          *(uint16_t *)((char *)v581 + v78 + 284710) = v88
                                                  + ((32 * ((v89 > deadzone_hi) - (uint32_t)(v89 < deadzone_lo))
                                                    + v89
                                                    + 2) >> 2);
          v81 = v576;
        }
        v90 = (char *)v578 + v78;
        v550 = v84;
        __builtin_prefetch(&v90[4 * (v81 ^ 0x4000) + 284712], 0, 1);
        v542 = (Obj23 *)((int32_t)&v90[4 * (v81 ^ 0x4000) + 284712]);
        v544 = (Obj124 *)(&v90[4 * (v81 ^ 0x3FF0) + 284712]);
        __builtin_prefetch(v544, 0, 1);
        v543 = (Obj2 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x4000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x4000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v543, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x2000) + 284712], 0, 1);
        v539 = (Obj15 *)((int32_t)&v90[4 * (v81 ^ 0x2000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x5FF0) + 284712], 0, 1);
        v541 = (Obj38 *)((int32_t)&v90[4 * (v81 ^ 0x5FF0) + 284712]);
        v540 = (Obj102 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x2000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x2000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v540, 0, 1);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x1000) + 284712], 0, 1);
        v536 = (Obj15 *)((int32_t)&v90[4 * (v81 ^ 0x1000) + 284712]);
        __builtin_prefetch(&v90[4 * (v81 ^ 0x6FF0) + 284712], 0, 1);
        v538 = (Obj39 *)((int32_t)&v90[4 * (v81 ^ 0x6FF0) + 284712]);
        v537 = (Obj103 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x1000) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x1000) & 0xFFFFFFF3)]);
        __builtin_prefetch(v537, 0, 1);
        v551 = (Obj46 *)(&v90[4 * (v81 ^ 0x800) + 284712]);
        v533 = (Obj46 *)(v551);
        v535 = (Obj105 *)(&v90[4 * (v81 ^ 0x77F0) + 284712]);
        v534 = (Obj104 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x800) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x800) & 0xFFFFFFF3)]);
        v552 = (Obj47 *)(&v90[4 * (v81 ^ 0x400) + 284712]);
        v530 = (Obj47 *)(v552);
        __builtin_prefetch(v551, 0, 1);
        __builtin_prefetch(v535, 0, 1);
        __builtin_prefetch(v534, 0, 1);
        v553 = (Obj125 *)(&v90[4 * (v81 ^ 0x7BF0) + 284712]);
        v532 = (Obj125 *)(v553);
        v531 = (Obj106 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x400) & 0xC))
                  + 284712
                  + 4 * ((v81 ^ 0x400) & 0xFFFFFFF3)]);
        v554 = (Obj48 *)(&v90[4 * (v81 ^ 0x200) + 284712]);
        v527 = (Obj48 *)(v554);
        __builtin_prefetch(v552, 0, 1);
        v555 = (Obj126 *)(&v90[4 * (v81 ^ 0x7DF0) + 284712]);
        v529 = (Obj126 *)(v555);
        v91 = (Obj107 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x200) & 0xC))
                 + 284712
                 + 4 * ((v81 ^ 0x200) & 0xFFFFFFF3)]);
        __builtin_prefetch(v553, 0, 1);
        __builtin_prefetch(v531, 0, 1);
        v528 = (Obj107 *)(v91);
        v556 = (Obj49 *)(&v90[4 * (v81 ^ 0x100) + 284712]);
        v524 = (Obj49 *)(v556);
        v92 = *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x100) & 0xC)) + ((v81 ^ 0x100) & 0xFFFFFFF3);
        v557 = (Obj108 *)(&v90[4 * (v81 ^ 0x7EF0) + 284712]);
        v526 = (Obj108 *)(v557);
        __builtin_prefetch(v554, 0, 1);
        v558 = (Obj40 *)((int32_t)&v90[4 * v92 + 284712]);
        v525 = (Obj40 *)(v558);
        v559 = (Obj50 *)(&v90[4 * (v81 ^ 0x80) + 284712]);
        v521 = (Obj50 *)(v559);
        __builtin_prefetch(v555, 0, 1);
        __builtin_prefetch(v91, 0, 1);
        v523 = (Obj110 *)(&v90[4 * (v81 ^ 0x7F70) + 284712]);
        v560 = (Obj109 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x80) & 0xC)) + 284712 + 4 * ((v81 ^ 0x80) & 0xFFFFFFF3)]);
        v522 = (Obj109 *)(v560);
        v561 = (Obj51 *)(&v90[4 * (v81 ^ 0x40) + 284712]);
        v518 = (Obj51 *)(v561);
        __builtin_prefetch(v556, 0, 1);
        v562 = (Obj112 *)(&v90[4 * (v81 ^ 0x7FB0) + 284712]);
        v520 = (Obj112 *)(v562);
        v93 = (Obj111 *)(&v90[4 * *(int32_t *)((char *)__dword_439880 + ((v81 ^ 0x40) & 0xC)) + 284712 + 4 * ((v81 ^ 0x40) & 0xFFFFFFF3)]);
        v94 = (Obj40 *)((const char *)v558);
        __builtin_prefetch(v557, 0, 1);
        __builtin_prefetch(v94, 0, 1);
        v563 = (Obj111 *)(v93);
        v519 = (Obj111 *)(v93);
        v95 = v81 ^ 0x20;
        v96 = v81 ^ 0x10;
        v564 = (Obj52 *)(&v90[4 * v95 + 284712]);
        v515 = (Obj52 *)(v564);
        v565 = (Obj127 *)(&v90[4 * (v95 ^ 0x7FF0) + 284712]);
        v517 = (Obj127 *)(v565);
        __builtin_prefetch(v559, 0, 1);
        __builtin_prefetch(v523, 0, 1);
        v566 = (Obj41 *)((uint32_t)&v90[4 * *(int32_t *)((char *)__dword_439880 + (v95 & 0xC)) + 284712 + 4 * (v95 & 0xFFFFFFF3)]);
        v516 = (Obj41 *)(v566);
        v567 = (Obj15 *)((int32_t)&v90[4 * v96 + 284712]);
        v512 = (Obj15 *)(v567);
        v97 = *(int32_t *)((char *)__dword_439880 + (v96 & 0xC)) + (v96 & 0xFFFFFFF3);
        v568 = (Obj44 *)((int32_t)&v90[4 * (v96 ^ 0x7FF0) + 284712]);
        v514 = (Obj44 *)(v568);
        __builtin_prefetch(v560, 0, 1);
        v98 = (Obj42 *)((int32_t)&v90[4 * v97 + 284712]);
        n2_1 = (char *)n2;
        v569 = (Obj42 *)((const char *)v98);
        v100 = -v550;
        v513 = (Obj42 *)(v98);
        LOBYTE(v97) = *(uint8_t *)n2;
        __builtin_prefetch(v561, 0, 1);
        v101 = *(int16_t *)(n2_1 + 2);
        __builtin_prefetch(v562, 0, 1);
        v102 = v100 - ((v101 + (1 << ((v97 + 31) & 31))) >> (v97 & 31));
        v105 = __OFSUB__(v102, deadzone_hi);
        v103 = v102 == deadzone_hi;
        v104 = v102 - deadzone_hi < 0;
        __builtin_prefetch(v563, 0, 1);
        v106 = 32 * (!(v104 ^ v105 | v103) - (v102 < deadzone_lo));
        __builtin_prefetch(v564, 0, 1);
        __builtin_prefetch(v565, 0, 1);
        v107 = (Obj42 *)(v569);
        v108 = v550;
        v109 = v106 + v102 + 2;
        v110 = (Obj15 *)((const char *)v567);
        LOWORD(v109) = v101 + (v109 >> 2);
        v87 = n3 < 3;
        v111 = (Obj41 *)((const char *)v566);
        *(uint16_t *)(n2 + 2) = v109;
        v112 = (Obj44 *)((const char *)v568);
        __builtin_prefetch(v111, 0, 1);
        __builtin_prefetch(v110, 0, 1);
        __builtin_prefetch(v112, 0, 1);
        __builtin_prefetch(v107, 0, 1);
        if ( v87
          && (v113 = *(int16_t *)(n2 + 6),
              v114 = *(uint8_t *)(n2 + 4),
              v550 = v108,
              v87 = n3 <= 0,
              *(uint16_t *)(n2 + 6) = ((uint32_t)(-v108 - ((v113 + (1 << ((v114 + 31) & 31))) >> (v114 & 31)) + 2) >> 2) + v113,
              v87) )
        {
          v311 = v542->f2;
          v312 = v542->f0;
          v550 = v108;
          v313 = v108 - ((v311 + (1 << ((v312 + 31) & 31))) >> (v312 & 31));
          *(uint16_t *)&v542->f2 = v311
                               + ((32 * ((v313 > deadzone_hi) - (uint32_t)(v313 < deadzone_lo)) + v313 + 2) >> 2);
          v314 = *((int16_t *)v543 + 1);
          v315 = v108 - ((v314 + (1 << ((v543->f0 + 31) & 31))) >> (v543->f0 & 31));
          *((uint16_t *)v543 + 1) = v314
                               + ((32 * ((v315 > deadzone_hi) - (uint32_t)(v315 < deadzone_lo)) + v315 + 4) >> 3);
          v316 = v542->f6;
          v317 = (v316 + (1 << ((v542->f4 + 31) & 31))) >> (v542->f4 & 31);
          v549 = -v108;
          *(uint16_t *)&v542->f6 = ((uint32_t)(v108 - v317 + 2) >> 2) + v316;
          v318 = *((int16_t *)v544 + 1);
          v319 = -v108 - ((v318 + (1 << ((v544->f0 + 31) & 31))) >> (v544->f0 & 31));
          *((uint16_t *)v544 + 1) = v318
                               + ((32 * ((v319 > deadzone_hi) - (uint32_t)(v319 < deadzone_lo)) + v319 + 4) >> 3);
          v320 = v539->f2;
          v321 = v550;
          v322 = v550 - ((v320 + (1 << ((v539->f0 + 31) & 31))) >> (v539->f0 & 31));
          *(uint16_t *)&v539->f2 = v320
                               + ((32 * ((v322 > deadzone_hi) - (uint32_t)(v322 < deadzone_lo)) + v322 + 2) >> 2);
          v323 = *((int16_t *)v540 + 1);
          v324 = v321 - ((v323 + (1 << ((v540->f0 + 31) & 31))) >> (v540->f0 & 31));
          *((uint16_t *)v540 + 1) = v323
                               + ((32 * ((v324 > deadzone_hi) - (uint32_t)(v324 < deadzone_lo)) + v324 + 4) >> 3);
          v539->f6 += (uint32_t)(v321
                                               - ((*(int16_t *)&v539->f6 + (1 << ((v539->f4 + 31) & 31))) >> (v539->f4 & 31))
                                               + 2) >> 2;
          v325 = v541->f2;
          v326 = v549 - ((v325 + (1 << ((v541->f0 + 31) & 31))) >> (v541->f0 & 31));
          *(uint16_t *)&v541->f2 = v325
                               + ((32 * ((v326 > deadzone_hi) - (uint32_t)(v326 < deadzone_lo)) + v326 + 4) >> 3);
          v327 = v536->f2;
          v328 = v550 - ((v327 + (1 << ((v536->f0 + 31) & 31))) >> (v536->f0 & 31));
          *(uint16_t *)&v536->f2 = v327
                               + ((32 * ((v328 > deadzone_hi) - (uint32_t)(v328 < deadzone_lo)) + v328 + 2) >> 2);
          v329 = *((int16_t *)v537 + 1);
          v330 = v550 - ((v329 + (1 << ((v537->f0 + 31) & 31))) >> (v537->f0 & 31));
          *((uint16_t *)v537 + 1) = v329
                               + ((32 * ((v330 > deadzone_hi) - (uint32_t)(v330 < deadzone_lo)) + v330 + 4) >> 3);
          v536->f6 += (uint32_t)(v550
                                               - ((*(int16_t *)&v536->f6 + (1 << ((v536->f4 + 31) & 31))) >> (v536->f4 & 31))
                                               + 2) >> 2;
          v331 = v538->f2;
          v332 = v549 - ((v331 + (1 << ((v538->f0 + 31) & 31))) >> (v538->f0 & 31));
          *(uint16_t *)&v538->f2 = v331
                               + ((32 * ((v332 > deadzone_hi) - (uint32_t)(v332 < deadzone_lo)) + v332 + 4) >> 3);
          v333 = *((int16_t *)v533 + 1);
          v334 = v550 - ((v333 + (1 << ((v533->f0 + 31) & 31))) >> (v533->f0 & 31));
          *((uint16_t *)v533 + 1) = v333
                               + ((32 * ((v334 > deadzone_hi) - (uint32_t)(v334 < deadzone_lo)) + v334 + 2) >> 2);
          v335 = *((int16_t *)v534 + 1);
          v336 = v550 - ((v335 + (1 << ((v534->f0 + 31) & 31))) >> (v534->f0 & 31));
          *((uint16_t *)v534 + 1) = v335
                               + ((32 * ((v336 > deadzone_hi) - (uint32_t)(v336 < deadzone_lo)) + v336 + 4) >> 3);
          *((uint16_t *)v533 + 3) += (uint32_t)(v550 - ((*((int16_t *)v533 + 3) + (1 << ((v533->f4 + 31) & 31))) >> (v533->f4 & 31)) + 2) >> 2;
          v337 = *((int16_t *)v535 + 1);
          v338 = v549 - ((v337 + (1 << ((v535->f0 + 31) & 31))) >> (v535->f0 & 31));
          *((uint16_t *)v535 + 1) = v337
                               + ((32 * ((v338 > deadzone_hi) - (uint32_t)(v338 < deadzone_lo)) + v338 + 4) >> 3);
          v339 = *((int16_t *)v530 + 1);
          v340 = v550 - ((v339 + (1 << ((v530->f0 + 31) & 31))) >> (v530->f0 & 31));
          *((uint16_t *)v530 + 1) = v339
                               + ((32 * ((v340 > deadzone_hi) - (uint32_t)(v340 < deadzone_lo)) + v340 + 2) >> 2);
          v341 = *((int16_t *)v531 + 1);
          v342 = v550 - ((v341 + (1 << ((v531->f0 + 31) & 31))) >> (v531->f0 & 31));
          *((uint16_t *)v531 + 1) = v341
                               + ((32 * ((v342 > deadzone_hi) - (uint32_t)(v342 < deadzone_lo)) + v342 + 4) >> 3);
          v343 = v550;
          *((uint16_t *)v530 + 3) += (uint32_t)(v550 - ((*((int16_t *)v530 + 3) + (1 << ((v530->f4 + 31) & 31))) >> (v530->f4 & 31)) + 2) >> 2;
          v344 = *((int16_t *)v532 + 1);
          v549 -= (v344 + (1 << ((v532->f0 + 31) & 31))) >> (v532->f0 & 31);
          v550 = v343;
          *((uint16_t *)v532 + 1) = v344
                               + ((32 * ((v549 > deadzone_hi) - (uint32_t)(v549 < deadzone_lo)) + v549 + 4) >> 3);
          v345 = *((int16_t *)v527 + 1);
          v346 = v550 - ((v345 + (1 << ((v527->f0 + 31) & 31))) >> (v527->f0 & 31));
          *((uint16_t *)v527 + 1) = v345
                               + ((32 * ((v346 > deadzone_hi) - (uint32_t)(v346 < deadzone_lo)) + v346 + 2) >> 2);
          v347 = *((int16_t *)v528 + 1);
          v348 = v550 - ((v347 + (1 << ((v528->f0 + 31) & 31))) >> (v528->f0 & 31));
          *((uint16_t *)v528 + 1) = v347
                               + ((32 * ((v348 > deadzone_hi) - (uint32_t)(v348 < deadzone_lo)) + v348 + 4) >> 3);
          v349 = v550;
          *((uint16_t *)v527 + 3) += (uint32_t)(v550 - ((*((int16_t *)v527 + 3) + (1 << ((v527->f4 + 31) & 31))) >> (v527->f4 & 31)) + 2) >> 2;
          v350 = *((int16_t *)v529 + 1);
          LOBYTE(v345) = v529->f0;
          v548 = -v349;
          v351 = -v349 - ((v350 + (1 << ((v345 + 31) & 31))) >> (v345 & 31));
          *((uint16_t *)v529 + 1) = v350
                               + ((32 * ((v351 > deadzone_hi) - (uint32_t)(v351 < deadzone_lo)) + v351 + 4) >> 3);
          v352 = *((int16_t *)v524 + 1);
          v353 = v550;
          v354 = v550 - ((v352 + (1 << ((v524->f0 + 31) & 31))) >> (v524->f0 & 31));
          *((uint16_t *)v524 + 1) = v352
                               + ((32 * ((v354 > deadzone_hi) - (uint32_t)(v354 < deadzone_lo)) + v354 + 2) >> 2);
          v355 = v525->f2;
          v356 = v353 - ((v355 + (1 << ((v525->f0 + 31) & 31))) >> (v525->f0 & 31));
          *(uint16_t *)&v525->f2 = v355
                               + ((32 * ((v356 > deadzone_hi) - (uint32_t)(v356 < deadzone_lo)) + v356 + 4) >> 3);
          *((uint16_t *)v524 + 3) += (uint32_t)(v353 - ((*((int16_t *)v524 + 3) + (1 << ((v524->f4 + 31) & 31))) >> (v524->f4 & 31)) + 2) >> 2;
          v357 = *((int16_t *)v526 + 1);
          v358 = v548 - ((v357 + (1 << ((v526->f0 + 31) & 31))) >> (v526->f0 & 31));
          *((uint16_t *)v526 + 1) = v357
                               + ((32 * ((v358 > deadzone_hi) - (uint32_t)(v358 < deadzone_lo)) + v358 + 4) >> 3);
          v359 = *((int16_t *)v521 + 1);
          v360 = v550 - ((v359 + (1 << ((v521->f0 + 31) & 31))) >> (v521->f0 & 31));
          *((uint16_t *)v521 + 1) = v359
                               + ((32 * ((v360 > deadzone_hi) - (uint32_t)(v360 < deadzone_lo)) + v360 + 2) >> 2);
          v361 = *((int16_t *)v522 + 1);
          v362 = v550 - ((v361 + (1 << ((v522->f0 + 31) & 31))) >> (v522->f0 & 31));
          *((uint16_t *)v522 + 1) = v361
                               + ((32 * ((v362 > deadzone_hi) - (uint32_t)(v362 < deadzone_lo)) + v362 + 4) >> 3);
          *((uint16_t *)v521 + 3) += (uint32_t)(v550 - ((*((int16_t *)v521 + 3) + (1 << ((v521->f4 + 31) & 31))) >> (v521->f4 & 31)) + 2) >> 2;
          v363 = *((int16_t *)v523 + 1);
          v364 = v548 - ((v363 + (1 << ((v523->f0 + 31) & 31))) >> (v523->f0 & 31));
          *((uint16_t *)v523 + 1) = v363
                               + ((32 * ((v364 > deadzone_hi) - (uint32_t)(v364 < deadzone_lo)) + v364 + 4) >> 3);
          v365 = *((int16_t *)v518 + 1);
          v366 = v550 - ((v365 + (1 << ((v518->f0 + 31) & 31))) >> (v518->f0 & 31));
          *((uint16_t *)v518 + 1) = v365
                               + ((32 * ((v366 > deadzone_hi) - (uint32_t)(v366 < deadzone_lo)) + v366 + 2) >> 2);
          v367 = *((int16_t *)v519 + 1);
          v368 = v550 - ((v367 + (1 << ((v519->f0 + 31) & 31))) >> (v519->f0 & 31));
          *((uint16_t *)v519 + 1) = v367
                               + ((32 * ((v368 > deadzone_hi) - (uint32_t)(v368 < deadzone_lo)) + v368 + 4) >> 3);
          *((uint16_t *)v518 + 3) += (uint32_t)(v550 - ((*((int16_t *)v518 + 3) + (1 << ((v518->f4 + 31) & 31))) >> (v518->f4 & 31)) + 2) >> 2;
          v369 = *((int16_t *)v520 + 1);
          v370 = v548 - ((v369 + (1 << ((v520->f0 + 31) & 31))) >> (v520->f0 & 31));
          *((uint16_t *)v520 + 1) = v369
                               + ((32 * ((v370 > deadzone_hi) - (uint32_t)(v370 < deadzone_lo)) + v370 + 4) >> 3);
          v371 = *((int16_t *)v515 + 1);
          v372 = v550 - ((v371 + (1 << ((v515->f0 + 31) & 31))) >> (v515->f0 & 31));
          *((uint16_t *)v515 + 1) = v371
                               + ((32 * ((v372 > deadzone_hi) - (uint32_t)(v372 < deadzone_lo)) + v372 + 2) >> 2);
          v373 = v516->f2;
          v374 = v550 - ((v373 + (1 << ((v516->f0 + 31) & 31))) >> (v516->f0 & 31));
          *(uint16_t *)&v516->f2 = v373
                               + ((32 * ((v374 > deadzone_hi) - (uint32_t)(v374 < deadzone_lo)) + v374 + 4) >> 3);
          v375 = v550;
          *((uint16_t *)v515 + 3) += (uint32_t)(v550 - ((*((int16_t *)v515 + 3) + (1 << ((v515->f4 + 31) & 31))) >> (v515->f4 & 31)) + 2) >> 2;
          v376 = *((int16_t *)v517 + 1);
          v377 = (v376 + (1 << ((v517->f0 + 31) & 31))) >> (v517->f0 & 31);
          v550 = v375;
          *((uint16_t *)v517 + 1) = v376
                               + ((32 * ((v548 - v377 > deadzone_hi) - (uint32_t)(v548 - v377 < deadzone_lo))
                                 + v548
                                 - v377
                                 + 4) >> 3);
          v378 = v512->f2;
          v379 = v550 - ((v378 + (1 << ((v512->f0 + 31) & 31))) >> (v512->f0 & 31));
          *(uint16_t *)&v512->f2 = v378
                               + ((32 * ((v379 > deadzone_hi) - (uint32_t)(v379 < deadzone_lo)) + v379 + 2) >> 2);
          v380 = v513->f2;
          v381 = v550 - ((v380 + (1 << ((v513->f0 + 31) & 31))) >> (v513->f0 & 31));
          *(uint16_t *)&v513->f2 = v380
                               + ((32 * ((v381 > deadzone_hi) - (uint32_t)(v381 < deadzone_lo)) + v381 + 4) >> 3);
          v382 = v550;
          v512->f6 += (uint32_t)(v550
                                               - ((*(int16_t *)&v512->f6 + (1 << ((v512->f4 + 31) & 31))) >> (v512->f4 & 31))
                                               + 2) >> 2;
          v383 = v514->f2;
          v384 = -v382 - ((v383 + (1 << ((v514->f0 + 31) & 31))) >> (v514->f0 & 31));
          *(uint16_t *)&v514->f2 = v383
                               + ((32 * ((v384 > deadzone_hi) - (uint32_t)(v384 < deadzone_lo)) + v384 + 4) >> 3);
        }
        else
        {
          v115 = *(int16_t *)(n2 - 2);
          v116 = *(uint8_t *)(n2 - 4);
          v550 = v108;
          *(uint16_t *)(n2 - 2) = ((uint32_t)(-v108 - ((v115 + (1 << ((v116 + 31) & 31))) >> (v116 & 31)) + 4) >> 3) + v115;
          v117 = v542->f2;
          v118 = v550 - ((v117 + (1 << ((v542->f0 + 31) & 31))) >> (v542->f0 & 31));
          *(uint16_t *)&v542->f2 = v117
                               + ((32 * ((v118 > deadzone_hi) - (uint32_t)(v118 < deadzone_lo)) + v118 + 2) >> 2);
          v119 = *((int16_t *)v543 + 1);
          v87 = n3 < 3;
          v120 = v550 - ((v119 + (1 << ((v543->f0 + 31) & 31))) >> (v543->f0 & 31));
          *((uint16_t *)v543 + 1) = v119
                               + ((32 * ((v120 > deadzone_hi) - (uint32_t)(v120 < deadzone_lo)) + v120 + 4) >> 3);
          v121 = v550;
          if ( v87 )
          {
            *(uint16_t *)&v542->f6 += (uint32_t)(v550
                                                 - ((v542->f6 + (1 << ((v542->f4 + 31) & 31))) >> (v542->f4 & 31))
                                                 + 2) >> 2;
            v216 = *(int16_t *)((uintptr_t)v542 - 2);
            v217 = v121 - ((v216 + (1 << ((*(uint8_t *)((uintptr_t)v542 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v542 - 4) & 31));
            v218 = -v121;
            v572 = v218;
            *(uint16_t *)((uintptr_t)v542 - 2) = v216
                                 + ((32 * ((v217 > deadzone_hi) - (uint32_t)(v217 < deadzone_lo)) + v217 + 4) >> 3);
            v219 = *((int16_t *)v544 + 1);
            v220 = v218 - ((v219 + (1 << ((v544->f0 + 31) & 31))) >> (v544->f0 & 31));
            *((uint16_t *)v544 + 1) = v219
                                 + ((32 * ((v220 > deadzone_hi) - (uint32_t)(v220 < deadzone_lo)) + v220 + 4) >> 3);
            v221 = v539->f2;
            v222 = v550;
            v223 = v550 - ((v221 + (1 << ((v539->f0 + 31) & 31))) >> (v539->f0 & 31));
            *(uint16_t *)&v539->f2 = v221
                                 + ((32 * ((v223 > deadzone_hi) - (uint32_t)(v223 < deadzone_lo)) + v223 + 2) >> 2);
            v224 = *((int16_t *)v540 + 1);
            v225 = v222 - ((v224 + (1 << ((v540->f0 + 31) & 31))) >> (v540->f0 & 31));
            *((uint16_t *)v540 + 1) = v224
                                 + ((32 * ((v225 > deadzone_hi) - (uint32_t)(v225 < deadzone_lo)) + v225 + 4) >> 3);
            v539->f6 += (uint32_t)(v222
                                                 - ((*(int16_t *)&v539->f6 + (1 << ((v539->f4 + 31) & 31))) >> (v539->f4 & 31))
                                                 + 2) >> 2;
            v226 = *(int16_t *)((uintptr_t)v539 - 2);
            v227 = v222 - ((v226 + (1 << ((*(uint8_t *)((uintptr_t)v539 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v539 - 4) & 31));
            *(uint16_t *)((uintptr_t)v539 - 2) = v226
                                 + ((32 * ((v227 > deadzone_hi) - (uint32_t)(v227 < deadzone_lo)) + v227 + 4) >> 3);
            v228 = v541->f2;
            v229 = v572 - ((v228 + (1 << ((v541->f0 + 31) & 31))) >> (v541->f0 & 31));
            *(uint16_t *)&v541->f2 = v228
                                 + ((32 * ((v229 > deadzone_hi) - (uint32_t)(v229 < deadzone_lo)) + v229 + 4) >> 3);
            v230 = v536->f2;
            v231 = v550;
            v232 = v550 - ((v230 + (1 << ((v536->f0 + 31) & 31))) >> (v536->f0 & 31));
            *(uint16_t *)&v536->f2 = v230
                                 + ((32 * ((v232 > deadzone_hi) - (uint32_t)(v232 < deadzone_lo)) + v232 + 2) >> 2);
            v233 = *((int16_t *)v537 + 1);
            v234 = v231 - ((v233 + (1 << ((v537->f0 + 31) & 31))) >> (v537->f0 & 31));
            *((uint16_t *)v537 + 1) = v233
                                 + ((32 * ((v234 > deadzone_hi) - (uint32_t)(v234 < deadzone_lo)) + v234 + 4) >> 3);
            v536->f6 += (uint32_t)(v231
                                                 - ((*(int16_t *)&v536->f6 + (1 << ((v536->f4 + 31) & 31))) >> (v536->f4 & 31))
                                                 + 2) >> 2;
            v235 = *(int16_t *)((uintptr_t)v536 - 2);
            v236 = v231 - ((v235 + (1 << ((*(uint8_t *)((uintptr_t)v536 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v536 - 4) & 31));
            *(uint16_t *)((uintptr_t)v536 - 2) = v235
                                 + ((32 * ((v236 > deadzone_hi) - (uint32_t)(v236 < deadzone_lo)) + v236 + 4) >> 3);
            v237 = v538->f2;
            v238 = v572 - ((v237 + (1 << ((v538->f0 + 31) & 31))) >> (v538->f0 & 31));
            *(uint16_t *)&v538->f2 = v237
                                 + ((32 * ((v238 > deadzone_hi) - (uint32_t)(v238 < deadzone_lo)) + v238 + 4) >> 3);
            v239 = *((int16_t *)v533 + 1);
            v240 = v550;
            v241 = v550 - ((v239 + (1 << ((v533->f0 + 31) & 31))) >> (v533->f0 & 31));
            *((uint16_t *)v533 + 1) = v239
                                 + ((32 * ((v241 > deadzone_hi) - (uint32_t)(v241 < deadzone_lo)) + v241 + 2) >> 2);
            v242 = *((int16_t *)v534 + 1);
            v243 = v240 - ((v242 + (1 << ((v534->f0 + 31) & 31))) >> (v534->f0 & 31));
            *((uint16_t *)v534 + 1) = v242
                                 + ((32 * ((v243 > deadzone_hi) - (uint32_t)(v243 < deadzone_lo)) + v243 + 4) >> 3);
            *((uint16_t *)v533 + 3) += (uint32_t)(v240
                                                 - ((*((int16_t *)v533 + 3) + (1 << ((v533->f4 + 31) & 31))) >> (v533->f4 & 31))
                                                 + 2) >> 2;
            v244 = *((int16_t *)v533 - 1);
            v245 = v240 - ((v244 + (1 << ((*((char *)v533 - 4) + 31) & 31))) >> (*((char *)v533 - 4) & 31));
            *((uint16_t *)v533 - 1) = v244
                                 + ((32 * ((v245 > deadzone_hi) - (uint32_t)(v245 < deadzone_lo)) + v245 + 4) >> 3);
            v246 = *((int16_t *)v535 + 1);
            v247 = v572 - ((v246 + (1 << ((v535->f0 + 31) & 31))) >> (v535->f0 & 31));
            *((uint16_t *)v535 + 1) = v246
                                 + ((32 * ((v247 > deadzone_hi) - (uint32_t)(v247 < deadzone_lo)) + v247 + 4) >> 3);
            v248 = *((int16_t *)v530 + 1);
            v249 = v550;
            v250 = v550 - ((v248 + (1 << ((v530->f0 + 31) & 31))) >> (v530->f0 & 31));
            *((uint16_t *)v530 + 1) = v248
                                 + ((32 * ((v250 > deadzone_hi) - (uint32_t)(v250 < deadzone_lo)) + v250 + 2) >> 2);
            v251 = *((int16_t *)v531 + 1);
            LOBYTE(v248) = v531->f0;
            v252 = v251 + (1 << ((v531->f0 + 31) & 31));
            v550 = v249;
            *((uint16_t *)v531 + 1) = v251
                                 + ((32
                                   * ((v249 - (v252 >> (v248 & 31)) > deadzone_hi)
                                    - (uint32_t)(v249 - (v252 >> (v248 & 31)) < deadzone_lo))
                                   + v249
                                   - (v252 >> (v248 & 31))
                                   + 4) >> 3);
            v253 = v550;
            *((uint16_t *)v530 + 3) += (uint32_t)(v550
                                                 - ((*((int16_t *)v530 + 3) + (1 << ((v530->f4 + 31) & 31))) >> (v530->f4 & 31))
                                                 + 2) >> 2;
            v254 = *((int16_t *)v530 - 1);
            v255 = v253 - ((v254 + (1 << ((*((char *)v530 - 4) + 31) & 31))) >> (*((char *)v530 - 4) & 31));
            *((uint16_t *)v530 - 1) = v254
                                 + ((32 * ((v255 > deadzone_hi) - (uint32_t)(v255 < deadzone_lo)) + v255 + 4) >> 3);
            v256 = *((int16_t *)v532 + 1);
            LOBYTE(v254) = v532->f0;
            v573 = -v253;
            v257 = -v253 - ((v256 + (1 << ((v254 + 31) & 31))) >> (v254 & 31));
            *((uint16_t *)v532 + 1) = v256
                                 + ((32 * ((v257 > deadzone_hi) - (uint32_t)(v257 < deadzone_lo)) + v257 + 4) >> 3);
            v258 = *((int16_t *)v527 + 1);
            v259 = v550;
            v260 = v550 - ((v258 + (1 << ((v527->f0 + 31) & 31))) >> (v527->f0 & 31));
            *((uint16_t *)v527 + 1) = v258
                                 + ((32 * ((v260 > deadzone_hi) - (uint32_t)(v260 < deadzone_lo)) + v260 + 2) >> 2);
            v261 = *((int16_t *)v528 + 1);
            v262 = v259 - ((v261 + (1 << ((v528->f0 + 31) & 31))) >> (v528->f0 & 31));
            *((uint16_t *)v528 + 1) = v261
                                 + ((32 * ((v262 > deadzone_hi) - (uint32_t)(v262 < deadzone_lo)) + v262 + 4) >> 3);
            *((uint16_t *)v527 + 3) += (uint32_t)(v259
                                                 - ((*((int16_t *)v527 + 3) + (1 << ((v527->f4 + 31) & 31))) >> (v527->f4 & 31))
                                                 + 2) >> 2;
            v263 = *((int16_t *)v527 - 1);
            v264 = v259 - ((v263 + (1 << ((*((char *)v527 - 4) + 31) & 31))) >> (*((char *)v527 - 4) & 31));
            *((uint16_t *)v527 - 1) = v263
                                 + ((32 * ((v264 > deadzone_hi) - (uint32_t)(v264 < deadzone_lo)) + v264 + 4) >> 3);
            v265 = *((int16_t *)v529 + 1);
            v266 = v573 - ((v265 + (1 << ((v529->f0 + 31) & 31))) >> (v529->f0 & 31));
            *((uint16_t *)v529 + 1) = v265
                                 + ((32 * ((v266 > deadzone_hi) - (uint32_t)(v266 < deadzone_lo)) + v266 + 4) >> 3);
            v267 = *((int16_t *)v524 + 1);
            v268 = v550;
            v269 = v550 - ((v267 + (1 << ((v524->f0 + 31) & 31))) >> (v524->f0 & 31));
            *((uint16_t *)v524 + 1) = v267
                                 + ((32 * ((v269 > deadzone_hi) - (uint32_t)(v269 < deadzone_lo)) + v269 + 2) >> 2);
            v270 = v525->f2;
            v271 = v268 - ((v270 + (1 << ((v525->f0 + 31) & 31))) >> (v525->f0 & 31));
            *(uint16_t *)&v525->f2 = v270
                                 + ((32 * ((v271 > deadzone_hi) - (uint32_t)(v271 < deadzone_lo)) + v271 + 4) >> 3);
            *((uint16_t *)v524 + 3) += (uint32_t)(v268
                                                 - ((*((int16_t *)v524 + 3) + (1 << ((v524->f4 + 31) & 31))) >> (v524->f4 & 31))
                                                 + 2) >> 2;
            v272 = *((int16_t *)v524 - 1);
            v273 = v268 - ((v272 + (1 << ((*((char *)v524 - 4) + 31) & 31))) >> (*((char *)v524 - 4) & 31));
            *((uint16_t *)v524 - 1) = v272
                                 + ((32 * ((v273 > deadzone_hi) - (uint32_t)(v273 < deadzone_lo)) + v273 + 4) >> 3);
            v274 = *((int16_t *)v526 + 1);
            v275 = v573 - ((v274 + (1 << ((v526->f0 + 31) & 31))) >> (v526->f0 & 31));
            *((uint16_t *)v526 + 1) = v274
                                 + ((32 * ((v275 > deadzone_hi) - (uint32_t)(v275 < deadzone_lo)) + v275 + 4) >> 3);
            v276 = *((int16_t *)v521 + 1);
            v277 = v550;
            v278 = v550 - ((v276 + (1 << ((v521->f0 + 31) & 31))) >> (v521->f0 & 31));
            *((uint16_t *)v521 + 1) = v276
                                 + ((32 * ((v278 > deadzone_hi) - (uint32_t)(v278 < deadzone_lo)) + v278 + 2) >> 2);
            v279 = *((int16_t *)v522 + 1);
            v280 = v277 - ((v279 + (1 << ((v522->f0 + 31) & 31))) >> (v522->f0 & 31));
            *((uint16_t *)v522 + 1) = v279
                                 + ((32 * ((v280 > deadzone_hi) - (uint32_t)(v280 < deadzone_lo)) + v280 + 4) >> 3);
            *((uint16_t *)v521 + 3) += (uint32_t)(v277
                                                 - ((*((int16_t *)v521 + 3) + (1 << ((v521->f4 + 31) & 31))) >> (v521->f4 & 31))
                                                 + 2) >> 2;
            v281 = *((int16_t *)v521 - 1);
            v282 = v277 - ((v281 + (1 << ((*((char *)v521 - 4) + 31) & 31))) >> (*((char *)v521 - 4) & 31));
            *((uint16_t *)v521 - 1) = v281
                                 + ((32 * ((v282 > deadzone_hi) - (uint32_t)(v282 < deadzone_lo)) + v282 + 4) >> 3);
            v283 = *((int16_t *)v523 + 1);
            v284 = v573 - ((v283 + (1 << ((v523->f0 + 31) & 31))) >> (v523->f0 & 31));
            *((uint16_t *)v523 + 1) = v283
                                 + ((32 * ((v284 > deadzone_hi) - (uint32_t)(v284 < deadzone_lo)) + v284 + 4) >> 3);
            v285 = *((int16_t *)v518 + 1);
            v286 = v550;
            v287 = v550 - ((v285 + (1 << ((v518->f0 + 31) & 31))) >> (v518->f0 & 31));
            *((uint16_t *)v518 + 1) = v285
                                 + ((32 * ((v287 > deadzone_hi) - (uint32_t)(v287 < deadzone_lo)) + v287 + 2) >> 2);
            v288 = *((int16_t *)v519 + 1);
            v289 = v286 - ((v288 + (1 << ((v519->f0 + 31) & 31))) >> (v519->f0 & 31));
            *((uint16_t *)v519 + 1) = v288
                                 + ((32 * ((v289 > deadzone_hi) - (uint32_t)(v289 < deadzone_lo)) + v289 + 4) >> 3);
            *((uint16_t *)v518 + 3) += (uint32_t)(v286
                                                 - ((*((int16_t *)v518 + 3) + (1 << ((v518->f4 + 31) & 31))) >> (v518->f4 & 31))
                                                 + 2) >> 2;
            v290 = *((int16_t *)v518 - 1);
            v291 = v286 - ((v290 + (1 << ((*((char *)v518 - 4) + 31) & 31))) >> (*((char *)v518 - 4) & 31));
            v292 = -v286;
            v574 = v292;
            *((uint16_t *)v518 - 1) = v290
                                 + ((32 * ((v291 > deadzone_hi) - (uint32_t)(v291 < deadzone_lo)) + v291 + 4) >> 3);
            v293 = *((int16_t *)v520 + 1);
            v294 = v292 - ((v293 + (1 << ((v520->f0 + 31) & 31))) >> (v520->f0 & 31));
            *((uint16_t *)v520 + 1) = v293
                                 + ((32 * ((v294 > deadzone_hi) - (uint32_t)(v294 < deadzone_lo)) + v294 + 4) >> 3);
            v295 = *((int16_t *)v515 + 1);
            v296 = v550;
            v297 = v550 - ((v295 + (1 << ((v515->f0 + 31) & 31))) >> (v515->f0 & 31));
            *((uint16_t *)v515 + 1) = v295
                                 + ((32 * ((v297 > deadzone_hi) - (uint32_t)(v297 < deadzone_lo)) + v297 + 2) >> 2);
            v298 = v516->f2;
            v299 = v296 - ((v298 + (1 << ((v516->f0 + 31) & 31))) >> (v516->f0 & 31));
            *(uint16_t *)&v516->f2 = v298
                                 + ((32 * ((v299 > deadzone_hi) - (uint32_t)(v299 < deadzone_lo)) + v299 + 4) >> 3);
            *((uint16_t *)v515 + 3) += (uint32_t)(v296
                                                 - ((*((int16_t *)v515 + 3) + (1 << ((v515->f4 + 31) & 31))) >> (v515->f4 & 31))
                                                 + 2) >> 2;
            v300 = *((int16_t *)v515 - 1);
            v301 = v296 - ((v300 + (1 << ((*((char *)v515 - 4) + 31) & 31))) >> (*((char *)v515 - 4) & 31));
            *((uint16_t *)v515 - 1) = v300
                                 + ((32 * ((v301 > deadzone_hi) - (uint32_t)(v301 < deadzone_lo)) + v301 + 4) >> 3);
            v302 = *((int16_t *)v517 + 1);
            v303 = v574 - ((v302 + (1 << ((v517->f0 + 31) & 31))) >> (v517->f0 & 31));
            *((uint16_t *)v517 + 1) = v302
                                 + ((32 * ((v303 > deadzone_hi) - (uint32_t)(v303 < deadzone_lo)) + v303 + 4) >> 3);
            v304 = v512->f2;
            v305 = v550;
            v306 = v550 - ((v304 + (1 << ((v512->f0 + 31) & 31))) >> (v512->f0 & 31));
            *(uint16_t *)&v512->f2 = v304
                                 + ((32 * ((v306 > deadzone_hi) - (uint32_t)(v306 < deadzone_lo)) + v306 + 2) >> 2);
            v307 = v513->f2;
            v308 = v305 - ((v307 + (1 << ((v513->f0 + 31) & 31))) >> (v513->f0 & 31));
            *(uint16_t *)&v513->f2 = v307
                                 + ((32 * ((v308 > deadzone_hi) - (uint32_t)(v308 < deadzone_lo)) + v308 + 4) >> 3);
            v512->f6 += (uint32_t)(v305
                                                 - ((*(int16_t *)&v512->f6 + (1 << ((v512->f4 + 31) & 31))) >> (v512->f4 & 31))
                                                 + 2) >> 2;
            v309 = *(int16_t *)((uintptr_t)v512 - 2);
            v310 = v305 - ((v309 + (1 << ((*(uint8_t *)((uintptr_t)v512 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v512 - 4) & 31));
            *(uint16_t *)((uintptr_t)v512 - 2) = v309
                                 + ((32 * ((v310 > deadzone_hi) - (uint32_t)(v310 < deadzone_lo)) + v310 + 4) >> 3);
            v214 = v514->f2;
            v215 = v574 - ((v214 + (1 << ((v514->f0 + 31) & 31))) >> (v514->f0 & 31));
          }
          else
          {
            v122 = *(int16_t *)((uintptr_t)v542 - 2);
            v123 = v550 - ((v122 + (1 << ((*(uint8_t *)((uintptr_t)v542 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v542 - 4) & 31));
            *(uint16_t *)((uintptr_t)v542 - 2) = v122
                                 + ((32 * ((v123 > deadzone_hi) - (uint32_t)(v123 < deadzone_lo)) + v123 + 4) >> 3);
            v124 = *((int16_t *)v544 + 1);
            LOBYTE(v122) = v544->f0;
            v547 = -v121;
            v125 = -v121 - ((v124 + (1 << ((v122 + 31) & 31))) >> (v122 & 31));
            *((uint16_t *)v544 + 1) = v124
                                 + ((32 * ((v125 > deadzone_hi) - (uint32_t)(v125 < deadzone_lo)) + v125 + 4) >> 3);
            v126 = v539->f2;
            v127 = v550;
            v128 = v550 - ((v126 + (1 << ((v539->f0 + 31) & 31))) >> (v539->f0 & 31));
            *(uint16_t *)&v539->f2 = v126
                                 + ((32 * ((v128 > deadzone_hi) - (uint32_t)(v128 < deadzone_lo)) + v128 + 2) >> 2);
            v129 = *((int16_t *)v540 + 1);
            v130 = v127 - ((v129 + (1 << ((v540->f0 + 31) & 31))) >> (v540->f0 & 31));
            *((uint16_t *)v540 + 1) = v129
                                 + ((32 * ((v130 > deadzone_hi) - (uint32_t)(v130 < deadzone_lo)) + v130 + 4) >> 3);
            v131 = *(int16_t *)((uintptr_t)v539 - 2);
            v132 = v127 - ((v131 + (1 << ((*(uint8_t *)((uintptr_t)v539 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v539 - 4) & 31));
            *(uint16_t *)((uintptr_t)v539 - 2) = v131
                                 + ((32 * ((v132 > deadzone_hi) - (uint32_t)(v132 < deadzone_lo)) + v132 + 4) >> 3);
            v133 = v541->f2;
            v134 = v547 - ((v133 + (1 << ((v541->f0 + 31) & 31))) >> (v541->f0 & 31));
            *(uint16_t *)&v541->f2 = v133
                                 + ((32 * ((v134 > deadzone_hi) - (uint32_t)(v134 < deadzone_lo)) + v134 + 4) >> 3);
            v135 = v536->f2;
            v136 = v550;
            v137 = v550 - ((v135 + (1 << ((v536->f0 + 31) & 31))) >> (v536->f0 & 31));
            *(uint16_t *)&v536->f2 = v135
                                 + ((32 * ((v137 > deadzone_hi) - (uint32_t)(v137 < deadzone_lo)) + v137 + 2) >> 2);
            v138 = *((int16_t *)v537 + 1);
            v139 = v136 - ((v138 + (1 << ((v537->f0 + 31) & 31))) >> (v537->f0 & 31));
            *((uint16_t *)v537 + 1) = v138
                                 + ((32 * ((v139 > deadzone_hi) - (uint32_t)(v139 < deadzone_lo)) + v139 + 4) >> 3);
            v140 = *(int16_t *)((uintptr_t)v536 - 2);
            v141 = v136 - ((v140 + (1 << ((*(uint8_t *)((uintptr_t)v536 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v536 - 4) & 31));
            *(uint16_t *)((uintptr_t)v536 - 2) = v140
                                 + ((32 * ((v141 > deadzone_hi) - (uint32_t)(v141 < deadzone_lo)) + v141 + 4) >> 3);
            v142 = v538->f2;
            v143 = v547 - ((v142 + (1 << ((v538->f0 + 31) & 31))) >> (v538->f0 & 31));
            *(uint16_t *)&v538->f2 = v142
                                 + ((32 * ((v143 > deadzone_hi) - (uint32_t)(v143 < deadzone_lo)) + v143 + 4) >> 3);
            v144 = *((int16_t *)v533 + 1);
            v145 = v550;
            v146 = v550 - ((v144 + (1 << ((v533->f0 + 31) & 31))) >> (v533->f0 & 31));
            *((uint16_t *)v533 + 1) = v144
                                 + ((32 * ((v146 > deadzone_hi) - (uint32_t)(v146 < deadzone_lo)) + v146 + 2) >> 2);
            v147 = *((int16_t *)v534 + 1);
            v148 = v145 - ((v147 + (1 << ((v534->f0 + 31) & 31))) >> (v534->f0 & 31));
            *((uint16_t *)v534 + 1) = v147
                                 + ((32 * ((v148 > deadzone_hi) - (uint32_t)(v148 < deadzone_lo)) + v148 + 4) >> 3);
            v149 = *((int16_t *)v533 - 1);
            v150 = v145 - ((v149 + (1 << ((*((char *)v533 - 4) + 31) & 31))) >> (*((char *)v533 - 4) & 31));
            *((uint16_t *)v533 - 1) = v149
                                 + ((32 * ((v150 > deadzone_hi) - (uint32_t)(v150 < deadzone_lo)) + v150 + 4) >> 3);
            v151 = *((int16_t *)v535 + 1);
            v152 = v547 - ((v151 + (1 << ((v535->f0 + 31) & 31))) >> (v535->f0 & 31));
            *((uint16_t *)v535 + 1) = v151
                                 + ((32 * ((v152 > deadzone_hi) - (uint32_t)(v152 < deadzone_lo)) + v152 + 4) >> 3);
            v153 = *((int16_t *)v530 + 1);
            v154 = v550;
            v155 = v550 - ((v153 + (1 << ((v530->f0 + 31) & 31))) >> (v530->f0 & 31));
            *((uint16_t *)v530 + 1) = v153
                                 + ((32 * ((v155 > deadzone_hi) - (uint32_t)(v155 < deadzone_lo)) + v155 + 2) >> 2);
            v156 = *((int16_t *)v531 + 1);
            v157 = v154 - ((v156 + (1 << ((v531->f0 + 31) & 31))) >> (v531->f0 & 31));
            *((uint16_t *)v531 + 1) = v156
                                 + ((32 * ((v157 > deadzone_hi) - (uint32_t)(v157 < deadzone_lo)) + v157 + 4) >> 3);
            v158 = *((int16_t *)v530 - 1);
            v159 = v154 - ((v158 + (1 << ((*((char *)v530 - 4) + 31) & 31))) >> (*((char *)v530 - 4) & 31));
            *((uint16_t *)v530 - 1) = v158
                                 + ((32 * ((v159 > deadzone_hi) - (uint32_t)(v159 < deadzone_lo)) + v159 + 4) >> 3);
            v160 = *((int16_t *)v532 + 1);
            v547 -= (v160 + (1 << ((v532->f0 + 31) & 31))) >> (v532->f0 & 31);
            v161 = v550;
            *((uint16_t *)v532 + 1) = v160
                                 + ((32 * ((v547 > deadzone_hi) - (uint32_t)(v547 < deadzone_lo)) + v547 + 4) >> 3);
            v162 = *((int16_t *)v527 + 1);
            v163 = v161 - ((v162 + (1 << ((v527->f0 + 31) & 31))) >> (v527->f0 & 31));
            *((uint16_t *)v527 + 1) = v162
                                 + ((32 * ((v163 > deadzone_hi) - (uint32_t)(v163 < deadzone_lo)) + v163 + 2) >> 2);
            v164 = *((int16_t *)v528 + 1);
            v165 = v161 - ((v164 + (1 << ((v528->f0 + 31) & 31))) >> (v528->f0 & 31));
            *((uint16_t *)v528 + 1) = v164
                                 + ((32 * ((v165 > deadzone_hi) - (uint32_t)(v165 < deadzone_lo)) + v165 + 4) >> 3);
            v166 = *((int16_t *)v527 - 1);
            v167 = v161 - ((v166 + (1 << ((*((char *)v527 - 4) + 31) & 31))) >> (*((char *)v527 - 4) & 31));
            v168 = -v161;
            v570 = v168;
            *((uint16_t *)v527 - 1) = v166
                                 + ((32 * ((v167 > deadzone_hi) - (uint32_t)(v167 < deadzone_lo)) + v167 + 4) >> 3);
            v169 = *((int16_t *)v529 + 1);
            v170 = v168 - ((v169 + (1 << ((v529->f0 + 31) & 31))) >> (v529->f0 & 31));
            *((uint16_t *)v529 + 1) = v169
                                 + ((32 * ((v170 > deadzone_hi) - (uint32_t)(v170 < deadzone_lo)) + v170 + 4) >> 3);
            v171 = *((int16_t *)v524 + 1);
            v172 = v550;
            v173 = v550 - ((v171 + (1 << ((v524->f0 + 31) & 31))) >> (v524->f0 & 31));
            *((uint16_t *)v524 + 1) = v171
                                 + ((32 * ((v173 > deadzone_hi) - (uint32_t)(v173 < deadzone_lo)) + v173 + 2) >> 2);
            v174 = v525->f2;
            v175 = v172 - ((v174 + (1 << ((v525->f0 + 31) & 31))) >> (v525->f0 & 31));
            *(uint16_t *)&v525->f2 = v174
                                 + ((32 * ((v175 > deadzone_hi) - (uint32_t)(v175 < deadzone_lo)) + v175 + 4) >> 3);
            v176 = *((int16_t *)v524 - 1);
            v177 = v172 - ((v176 + (1 << ((*((char *)v524 - 4) + 31) & 31))) >> (*((char *)v524 - 4) & 31));
            *((uint16_t *)v524 - 1) = v176
                                 + ((32 * ((v177 > deadzone_hi) - (uint32_t)(v177 < deadzone_lo)) + v177 + 4) >> 3);
            v178 = *((int16_t *)v526 + 1);
            v179 = v570 - ((v178 + (1 << ((v526->f0 + 31) & 31))) >> (v526->f0 & 31));
            *((uint16_t *)v526 + 1) = v178
                                 + ((32 * ((v179 > deadzone_hi) - (uint32_t)(v179 < deadzone_lo)) + v179 + 4) >> 3);
            v180 = *((int16_t *)v521 + 1);
            v181 = v550;
            v182 = v550 - ((v180 + (1 << ((v521->f0 + 31) & 31))) >> (v521->f0 & 31));
            *((uint16_t *)v521 + 1) = v180
                                 + ((32 * ((v182 > deadzone_hi) - (uint32_t)(v182 < deadzone_lo)) + v182 + 2) >> 2);
            v183 = *((int16_t *)v522 + 1);
            v184 = v181 - ((v183 + (1 << ((v522->f0 + 31) & 31))) >> (v522->f0 & 31));
            *((uint16_t *)v522 + 1) = v183
                                 + ((32 * ((v184 > deadzone_hi) - (uint32_t)(v184 < deadzone_lo)) + v184 + 4) >> 3);
            v185 = *((int16_t *)v521 - 1);
            v186 = v181 - ((v185 + (1 << ((*((char *)v521 - 4) + 31) & 31))) >> (*((char *)v521 - 4) & 31));
            *((uint16_t *)v521 - 1) = v185
                                 + ((32 * ((v186 > deadzone_hi) - (uint32_t)(v186 < deadzone_lo)) + v186 + 4) >> 3);
            v187 = *((int16_t *)v523 + 1);
            v188 = v570 - ((v187 + (1 << ((v523->f0 + 31) & 31))) >> (v523->f0 & 31));
            *((uint16_t *)v523 + 1) = v187
                                 + ((32 * ((v188 > deadzone_hi) - (uint32_t)(v188 < deadzone_lo)) + v188 + 4) >> 3);
            v189 = *((int16_t *)v518 + 1);
            v190 = v550;
            v191 = v550 - ((v189 + (1 << ((v518->f0 + 31) & 31))) >> (v518->f0 & 31));
            *((uint16_t *)v518 + 1) = v189
                                 + ((32 * ((v191 > deadzone_hi) - (uint32_t)(v191 < deadzone_lo)) + v191 + 2) >> 2);
            v192 = *((int16_t *)v519 + 1);
            v193 = v190 - ((v192 + (1 << ((v519->f0 + 31) & 31))) >> (v519->f0 & 31));
            *((uint16_t *)v519 + 1) = v192
                                 + ((32 * ((v193 > deadzone_hi) - (uint32_t)(v193 < deadzone_lo)) + v193 + 4) >> 3);
            v194 = *((int16_t *)v518 - 1);
            v195 = v190 - ((v194 + (1 << ((*((char *)v518 - 4) + 31) & 31))) >> (*((char *)v518 - 4) & 31));
            *((uint16_t *)v518 - 1) = v194
                                 + ((32 * ((v195 > deadzone_hi) - (uint32_t)(v195 < deadzone_lo)) + v195 + 4) >> 3);
            v196 = *((int16_t *)v520 + 1);
            v197 = v570 - ((v196 + (1 << ((v520->f0 + 31) & 31))) >> (v520->f0 & 31));
            *((uint16_t *)v520 + 1) = v196
                                 + ((32 * ((v197 > deadzone_hi) - (uint32_t)(v197 < deadzone_lo)) + v197 + 4) >> 3);
            v198 = *((int16_t *)v515 + 1);
            v199 = v550;
            v200 = v550 - ((v198 + (1 << ((v515->f0 + 31) & 31))) >> (v515->f0 & 31));
            *((uint16_t *)v515 + 1) = v198
                                 + ((32 * ((v200 > deadzone_hi) - (uint32_t)(v200 < deadzone_lo)) + v200 + 2) >> 2);
            v201 = v516->f2;
            v202 = v199 - ((v201 + (1 << ((v516->f0 + 31) & 31))) >> (v516->f0 & 31));
            v550 = v199;
            *(uint16_t *)&v516->f2 = v201
                                 + ((32 * ((v202 > deadzone_hi) - (uint32_t)(v202 < deadzone_lo)) + v202 + 4) >> 3);
            v203 = *((int16_t *)v515 - 1);
            v204 = v199 - ((v203 + (1 << ((*((char *)v515 - 4) + 31) & 31))) >> (*((char *)v515 - 4) & 31));
            *((uint16_t *)v515 - 1) = v203
                                 + ((32 * ((v204 > deadzone_hi) - (uint32_t)(v204 < deadzone_lo)) + v204 + 4) >> 3);
            v205 = *((int16_t *)v517 + 1);
            LOBYTE(v203) = v517->f0;
            v571 = -v199;
            v206 = -v199 - ((v205 + (1 << ((v203 + 31) & 31))) >> (v203 & 31));
            *((uint16_t *)v517 + 1) = v205
                                 + ((32 * ((v206 > deadzone_hi) - (uint32_t)(v206 < deadzone_lo)) + v206 + 4) >> 3);
            v207 = v512->f2;
            v208 = v550;
            v209 = v550 - ((v207 + (1 << ((v512->f0 + 31) & 31))) >> (v512->f0 & 31));
            *(uint16_t *)&v512->f2 = v207
                                 + ((32 * ((v209 > deadzone_hi) - (uint32_t)(v209 < deadzone_lo)) + v209 + 2) >> 2);
            v210 = v513->f2;
            v211 = v208 - ((v210 + (1 << ((v513->f0 + 31) & 31))) >> (v513->f0 & 31));
            *(uint16_t *)&v513->f2 = v210
                                 + ((32 * ((v211 > deadzone_hi) - (uint32_t)(v211 < deadzone_lo)) + v211 + 4) >> 3);
            v212 = *(int16_t *)((uintptr_t)v512 - 2);
            v213 = v208 - ((v212 + (1 << ((*(uint8_t *)((uintptr_t)v512 - 4) + 31) & 31))) >> (*(uint8_t *)((uintptr_t)v512 - 4) & 31));
            *(uint16_t *)((uintptr_t)v512 - 2) = v212
                                 + ((32 * ((v213 > deadzone_hi) - (uint32_t)(v213 < deadzone_lo)) + v213 + 4) >> 3);
            v214 = v514->f2;
            v215 = v571 - ((v214 + (1 << ((v514->f0 + 31) & 31))) >> (v514->f0 & 31));
          }
          *(uint16_t *)&v514->f2 = v214
                               + ((32 * ((v215 > deadzone_hi) - (uint32_t)(v215 < deadzone_lo)) + v215 + 4) >> 3);
        }
      }
    }
    ++n5;
  }
  while ( n5 < 5 );
  v385 = (Obj69 *)(v578);
  v386 = v578->f278704;
  v578->f278736[0] += 18;
  v387 = (Obj63 *)((uint16_t *)&((uint32_t *)v385)[2 * v386]);
  v385->f278736[1] += 18;
  v385->f278736[2] += 18;
  v385->f278736[3] += 18;
  v385->f278736[4] += 18;
  n0x10 = v387->f940064[4];
  if ( n0x10 > 0x10 )
  {
    v389 = (uint16_t *)((uint32_t *)v385)[(a4 & 1) + 69677];
    v511 = a4 & 1;
    n15 = v386 & 0xF;
    v508 = v389;
    if ( n15 < 15 )
    {
      v391 = v387->f940080[1];
      v392 = v387->f940080[2];
      n2 = (int32_t)((uint16_t *)v387 + 470040);
      if ( v387->f940080[3] + v392 + v391 > 29696 )
        __fwd_alt_p2_model_rescale_three_way((uint16_t *)v387 + 470040);
      v393 = (10 * (uint32_t)v387->f940080[0]) >> 4;
      if ( a4 )
      {
        *(uint16_t *)(n2 - 2 * v511 + 6) += v393;
        __fwd_alt_p2_model_update_binary_pair(model_tables + 254 * (uint32_t)v508 + 254, (a4 - 1) >> 1);
      }
      else
      {
        *(uint16_t *)(n2 + 2) += v393;
      }
      if ( n15 <= 0 )
      {
LABEL_37:
        n0x10 = v385->f278704;
        if ( LOWORD(((uint32_t *)v385)[2 * n0x10 + 235018]) <= 0x1Au )
          return n0x10;
        v397 = v385->f278760[8];
        v398 = 2 - (*((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984) & 1);
        if ( !*((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984) )
          v398 = *((uint8_t *)v385 + (uint8_t)-(char)a5 + 279984);
        v399 = v385->f278760[12];
        v400 = v385->f278760[6] + (v385->f278704 & 0x3F);
        v510 = v398;
        n0x10 = ((uint32_t *)v385)[-v385->f278760[20] + 69713]
              + ((uint32_t *)v385)[-v385->f278760[16] + 69709]
              + ((uint32_t *)v385)[-v399 + 69705]
              + ((uint32_t *)v385)[-v397 + 69701]
              + v400;
        n0x10_3 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10];
        n0x10_2 = (uint32_t)n0x10_3;
        if ( n15 < 15 )
        {
          n2_2 = n0x10_3 + 470040;
          v403 = n0x10_3[470041];
          v404 = n0x10_3[470042];
          n2 = (int32_t)n2_2;
          if ( n2_2[3] + v404 + v403 > 29696 )
          {
            n0x10_1 = n0x10;
            __fwd_alt_p2_model_rescale_three_way(n2_2);
            n0x10 = n0x10_1;
          }
          v405 = *(uint16_t *)(n0x10_2 + 940080) & 0xFFFC;
          v510 *= 2;
          *(uint16_t *)(n2 + v510 + 2) += v405 >> 2;
          if ( n15 <= 0 )
            goto LABEL_48;
        }
        else
        {
          v510 *= 2;
        }
        v406 = *(uint16_t *)(n0x10_2 + 940066);
        v407 = *(uint16_t *)(n0x10_2 + 940068);
        n2 = n0x10_2 + 940064;
        if ( *(uint16_t *)(n0x10_2 + 940070) + v407 + v406 > 29696 )
        {
          n0x10_1 = n0x10;
          __fwd_alt_p2_model_rescale_three_way((uint16_t *)(n0x10_2 + 940064));
          n0x10 = n0x10_1;
        }
        *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)(n0x10_2 + 940064) & 0xFFFC) >> 2;
LABEL_48:
        if ( a4 )
        {
          n2 = (a4 - 1) >> 1;
          n0xF0 = (((uint16_t *)((uint8_t)(uintptr_t)v508 & 0xF0)));
          if ( (uint32_t)n0xF0 >= 0xF0
            || (n0x10_1 = n0x10,
                __fwd_alt_p2_model_update_binary_pair(model_tables + 254 * (uint32_t)v508 + 4064, n2),
                n0x10 = n0x10_1,
                (int32_t)n0xF0 > 0) )
          {
            n0x10_1 = n0x10;
            __fwd_alt_p2_model_update_binary_pair(model_tables + 254 * (uint32_t)v508 - 4064, n2);
            n0x10 = n0x10_1;
          }
        }
        v408 = (uint16_t *)(n0x10_2 + 940072);
        if ( *(uint16_t *)(n0x10_2 + 940078)
           + *(uint16_t *)(n0x10_2 + 940076)
           + *(uint16_t *)(n0x10_2 + 940074) > 29696 )
        {
          n0x10_1 = n0x10;
          __fwd_alt_p2_model_rescale_three_way((uint16_t *)(n0x10_2 + 940072));
          n0x10 = n0x10_1;
        }
        *(uint16_t *)((char *)v408 + v510 + 2) += (6 * (uint32_t)*(uint16_t *)(n0x10_2 + 940072)) >> 4;
        if ( !v385->f278728 || LOWORD(((uint32_t *)v385)[2 * v385->f278704 + 235018]) > 0x100u )
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
              __fwd_alt_p2_model_rescale_three_way(v491);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v508 + v510 + 2) += (uint16_t)(*v508 & 0xFFFC) >> 2;
            v494 = v385->f278704 - v385->f278760[6];
            n0xF0_1 = ((uint16_t *)&((uint32_t *)v385)[2 * v494 + 2 * v385->f278760[5]]);
            n0x10_2 = v385->f278760[7] + v494;
            n0xF0 = (uint16_t *)((uint16_t *)(n0xF0_1));
            n2_3 = (uint16_t *)n0xF0_1 + 470036;
            v497 = n2_3[2] + n2_3[1];
            v498 = n2_3[3];
            n2 = (int32_t)n2_3;
            if ( v498 + v497 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(n2_3);
              n0x10 = n0x10_1;
            }
            v499 = 3 * n0xF0[470036];
            v511 *= 2;
            *(uint16_t *)(n2 + v511 + 2) += v499 >> 4;
            if ( n15 >= 15 )
              goto LABEL_180;
            v500 = n0xF0[470041];
            v501 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v501 + v500 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v502 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + LOWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v502 + v511 + 2) += (uint16_t)(*v502 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_180:
              v503 = n0xF0[470033];
              v504 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v504 + v503 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v411 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[6] - ((uint32_t *)v385)[-v410 + 69697])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[6] - ((uint32_t *)v385)[-v410 + 69697])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[6] - ((uint32_t *)v385)[-v410 + 69697])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[6] - ((uint32_t *)v385)[-v410 + 69697])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v411);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v411 + v510 + 2) += (7 * (uint32_t)*v411) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[6] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[4] + 69695])]));
            v412 = n0xF0[470037];
            v413 = n0xF0[470038];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470036);
            if ( n0xF0[470039] + v413 + v412 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            v414 = 7 * n0xF0[470036];
            v511 *= 2;
            *(uint16_t *)(n2 + v511 + 2) += v414 >> 4;
            if ( n15 >= 15 )
              goto LABEL_67;
            v415 = n0xF0[470041];
            v416 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v416 + v415 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            n0x10_1 = n0x10;
            *(uint16_t *)(n2 + v511 + 2) = *(uint16_t *)(v511 + n2 + 2) + ((5 * (uint32_t)n0xF0[470040]) >> 4);
            n0x10 = n0x10_1;
            if ( n15 > 0 )
            {
LABEL_67:
              v417 = n0xF0[470033];
              v418 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v418 + v417 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v419 = v385->f278760[8];
          if ( v419 == 1 )
          {
            n2_4 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[9] + 235018 + 2 * (n0x10 - v385->f278760[10])];
            v481 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[9] + 235018 + 2 * (n0x10 - v385->f278760[10])]);
            v482 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[9] + 235019 + 2 * (n0x10 - v385->f278760[10])]);
            n2 = (int32_t)n2_4;
            if ( n2_4[3] + v482 + v481 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(n2_4);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v483 = v385->f278704 - v385->f278760[10];
            n0xF0_2 = ((uint16_t *)&((uint32_t *)v385)[2 * v483 + 2 * v385->f278760[9]]);
            n0x10_2 = v385->f278760[11] + v483;
            n0xF0 = (uint16_t *)((uint16_t *)(n0xF0_2));
            v485 = (uint16_t *)n0xF0_2 + 470036;
            if ( v485[3] + v485[2] + v485[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v485);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v485 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_167;
            v486 = n0xF0[470041];
            v487 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v487 + v486 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v488 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + LOWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v488 + v511 + 2) += (uint16_t)(*v488 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_167:
              v489 = n0xF0[470033];
              v490 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v490 + v489 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v420 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[10] - ((uint32_t *)v385)[-v419 + 69701])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[10] - ((uint32_t *)v385)[-v419 + 69701])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[10] - ((uint32_t *)v385)[-v419 + 69701])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[10] - ((uint32_t *)v385)[-v419 + 69701])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v420);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v420 + v510 + 2) += (7 * (uint32_t)*v420) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[10] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[8] + 69699])]));
            v421 = n0xF0[470037];
            v422 = n0xF0[470038];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470036);
            if ( n0xF0[470039] + v422 + v421 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_79;
            v423 = n0xF0[470041];
            v424 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v424 + v423 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_79:
              v425 = n0xF0[470033];
              v426 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v426 + v425 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v427 = v385->f278760[12];
          if ( v427 == 1 )
          {
            n2_5 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[13] + 235018 + 2 * (n0x10 - v385->f278760[14])];
            v470 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[13] + 235018 + 2 * (n0x10 - v385->f278760[14])]);
            v471 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[13] + 235019 + 2 * (n0x10 - v385->f278760[14])]);
            n2 = (int32_t)n2_5;
            if ( n2_5[3] + v471 + v470 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(n2_5);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v472 = v385->f278704 - v385->f278760[14];
            n0xF0_3 = ((uint16_t *)&((uint32_t *)v385)[2 * v472 + 2 * v385->f278760[13]]);
            n0x10_2 = v385->f278760[15] + v472;
            n0xF0 = (uint16_t *)((uint16_t *)(n0xF0_3));
            v474 = (uint16_t *)n0xF0_3 + 470036;
            if ( v474[3] + v474[2] + v474[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v474);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v474 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_154;
            v475 = n0xF0[470041];
            v476 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v476 + v475 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v477 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + LOWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v477 + v511 + 2) += (uint16_t)(*v477 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_154:
              v478 = n0xF0[470033];
              v479 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v479 + v478 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v428 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[14] - ((uint32_t *)v385)[-v427 + 69705])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[14] - ((uint32_t *)v385)[-v427 + 69705])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[14] - ((uint32_t *)v385)[-v427 + 69705])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[14] - ((uint32_t *)v385)[-v427 + 69705])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v428);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v428 + v510 + 2) += (7 * (uint32_t)*v428) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[14] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[12] + 69703])]));
            v429 = n0xF0[470037];
            v430 = n0xF0[470038];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470036);
            if ( n0xF0[470039] + v430 + v429 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_91;
            v431 = n0xF0[470041];
            v432 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v432 + v431 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_91:
              v433 = n0xF0[470033];
              v434 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v434 + v433 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v435 = v385->f278760[16];
          if ( v435 == 1 )
          {
            n2_6 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[17] + 235018 + 2 * (n0x10 - v385->f278760[18])];
            v459 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[17] + 235018 + 2 * (n0x10 - v385->f278760[18])]);
            v460 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[17] + 235019 + 2 * (n0x10 - v385->f278760[18])]);
            n2 = (int32_t)n2_6;
            if ( n2_6[3] + v460 + v459 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(n2_6);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v461 = v385->f278704 - v385->f278760[18];
            n0xF0_4 = ((uint16_t *)&((uint32_t *)v385)[2 * v461 + 2 * v385->f278760[17]]);
            n0x10_2 = v385->f278760[19] + v461;
            n0xF0 = (uint16_t *)((uint16_t *)(n0xF0_4));
            v463 = (uint16_t *)n0xF0_4 + 470036;
            if ( v463[3] + v463[2] + v463[1] > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v463);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v463 + v511 + 2) += (3 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_141;
            v464 = n0xF0[470041];
            v465 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v465 + v464 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(n0xF0[470040] & 0xFFFC) >> 2;
            v466 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + LOWORD(((uint32_t *)v385)[2 * n0x10_2 + 235021])
               + HIWORD(((uint32_t *)v385)[2 * n0x10_2 + 235020]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)&((uint32_t *)v385)[2 * n0x10_2 + 235020]);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v466 + v511 + 2) += (uint16_t)(*v466 & 0xFFF8) >> 3;
            if ( n15 > 2 )
            {
LABEL_141:
              v467 = n0xF0[470033];
              v468 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v468 + v467 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          else
          {
            v436 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[18] - ((uint32_t *)v385)[-v435 + 69709])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[18] - ((uint32_t *)v385)[-v435 + 69709])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[18] - ((uint32_t *)v385)[-v435 + 69709])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[18] - ((uint32_t *)v385)[-v435 + 69709])]) > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way(v436);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)((char *)v436 + v510 + 2) += (7 * (uint32_t)*v436) >> 4;
            n0xF0 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[18] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[16] + 69707])]));
            v437 = n0xF0[470037];
            v438 = n0xF0[470038];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470036);
            if ( n0xF0[470039] + v438 + v437 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470036);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(n2 + v511 + 2) += (7 * (uint32_t)n0xF0[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_103;
            v439 = n0xF0[470041];
            v440 = n0xF0[470042];
            n2 = (int32_t)((uint16_t *)n0xF0 + 470040);
            if ( n0xF0[470043] + v440 + v439 > 29696 )
            {
              n0x10_1 = n0x10;
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470040);
              n0x10 = n0x10_1;
            }
            *(uint16_t *)(v511 + n2 + 2) += (5 * (uint32_t)n0xF0[470040]) >> 4;
            if ( n15 > 0 )
            {
LABEL_103:
              v441 = n0xF0[470033];
              v442 = n0xF0[470034];
              n2 = (int32_t)((uint16_t *)n0xF0 + 470032);
              if ( n0xF0[470035] + v442 + v441 > 29696 )
              {
                n0x10_1 = n0x10;
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)n0xF0 + 470032);
                n0x10 = n0x10_1;
              }
              *(uint16_t *)(n2 + v511 + 2) += (6 * (uint32_t)n0xF0[470032]) >> 4;
            }
          }
          v443 = v385->f278760[20];
          if ( v443 == 1 )
          {
            n2_7 = (uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[21] + 235018 + 2 * (n0x10 - v385->f278760[22])];
            v447 = HIWORD(((uint32_t *)v385)[2 * v385->f278760[21] + 235018 + 2 * (n0x10 - v385->f278760[22])]);
            v448 = LOWORD(((uint32_t *)v385)[2 * v385->f278760[21] + 235019 + 2 * (n0x10 - v385->f278760[22])]);
            v449 = n2_7[3];
            n2 = (int32_t)n2_7;
            if ( v449 + v448 + v447 > 29696 )
              __fwd_alt_p2_model_rescale_three_way(n2_7);
            *(uint16_t *)(n2 + v510 + 2) += (uint16_t)(*(uint16_t *)n2 & 0xFFFC) >> 2;
            v450 = v385->f278704 - v385->f278760[22];
            n0x10_4 = (uint16_t *)&((uint32_t *)v385)[2 * v450 + 2 * v385->f278760[21]];
            n0xF0 = (((uint16_t *)(v385->f278760[23] + v450)));
            n0x10_2 = (uint32_t)n0x10_4;
            v452 = n0x10_4 + 470036;
            if ( n0x10_4[470039] + n0x10_4[470038] + n0x10_4[470037] > 29696 )
              __fwd_alt_p2_model_rescale_three_way(v452);
            *(uint16_t *)((char *)v452 + v511 + 2) += (3 * (uint32_t)*(uint16_t *)(n0x10_2 + 940072)) >> 4;
            if ( n15 >= 15 )
              goto LABEL_128;
            v453 = *(uint16_t *)(n0x10_2 + 940086);
            v454 = *(uint16_t *)(n0x10_2 + 940084) + *(uint16_t *)(n0x10_2 + 940082);
            n2 = n0x10_2 + 940080;
            if ( v453 + v454 > 29696 )
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)(n0x10_2 + 940080));
            n0xF0_5 = (uint16_t *)(n0xF0);
            *(uint16_t *)(n2 + v511 + 2) += (uint16_t)(*(uint16_t *)(n0x10_2 + 940080) & 0xFFFC) >> 2;
            v456 = (uint16_t *)&((uint32_t *)v385)[2 * (uint32_t)n0xF0_5 + 235020];
            if ( v456[3] + v456[2] + v456[1] > 29696 )
              __fwd_alt_p2_model_rescale_three_way(v456);
            n0x10 = (uint16_t)(*v456 & 0xFFF8) >> 3;
            *(uint16_t *)((char *)v456 + v511 + 2) += n0x10;
            if ( n15 > 2 )
            {
LABEL_128:
              v457 = (uint16_t *)(n0x10_2 + 940064);
              if ( *(uint16_t *)(n0x10_2 + 940070)
                 + *(uint16_t *)(n0x10_2 + 940068)
                 + *(uint16_t *)(n0x10_2 + 940066) > 29696 )
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)(n0x10_2 + 940064));
              n0x10 = n0x10_2;
              *(uint16_t *)((char *)v457 + v511 + 2) += (6
                                                               * (uint32_t)*(uint16_t *)(n0x10_2 + 940064)) >> 4;
            }
          }
          else
          {
            v444 = (uint16_t *)&((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[22] - ((uint32_t *)v385)[-v443 + 69713])];
            if ( HIWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[22] - ((uint32_t *)v385)[-v443 + 69713])])
               + LOWORD(((uint32_t *)v385)[2 * n0x10 + 235019 + 2 * (v385->f278760[22] - ((uint32_t *)v385)[-v443 + 69713])])
               + HIWORD(((uint32_t *)v385)[2 * n0x10 + 235018 + 2 * (v385->f278760[22] - ((uint32_t *)v385)[-v443 + 69713])]) > 29696 )
              __fwd_alt_p2_model_rescale_three_way(v444);
            *(uint16_t *)((char *)v444 + v510 + 2) += (7 * (uint32_t)*v444) >> 4;
            v445 = (((uint16_t *)&((uint32_t *)v385)[2 * v385->f278760[22] + 2 * (v385->f278704 - ((uint32_t *)v385)[v385->f278760[20] + 69711])]));
            if ( v445[470039] + v445[470038] + v445[470037] > 29696 )
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)v445 + 470036);
            *(uint16_t *)((char *)v445 + v511 + 940074) += (7 * (uint32_t)v445[470036]) >> 4;
            if ( n15 >= 15 )
              goto LABEL_115;
            if ( v445[470043] + v445[470042] + v445[470041] > 29696 )
              __fwd_alt_p2_model_rescale_three_way((uint16_t *)v445 + 470040);
            n0x10 = v445[470040];
            *(uint16_t *)((char *)v445 + v511 + 940082) += (5 * n0x10) >> 4;
            if ( n15 > 0 )
            {
LABEL_115:
              if ( v445[470035] + v445[470034] + v445[470033] > 29696 )
                __fwd_alt_p2_model_rescale_three_way((uint16_t *)v445 + 470032);
              n0x10 = v445[470032];
              *(uint16_t *)((char *)v445 + v511 + 940066) += (6 * n0x10) >> 4;
            }
          }
        }
        return n0x10;
      }
      v387 = (Obj63 *)((uint16_t *)&((uint32_t *)v385)[2 * v385->f278704]);
    }
    v394 = v387->f940064[1];
    v395 = v387->f940064[2];
    n2 = (int32_t)((uint16_t *)v387 + 470032);
    if ( v387->f940064[3] + v395 + v394 > 29696 )
      __fwd_alt_p2_model_rescale_three_way((uint16_t *)v387 + 470032);
    v396 = (13 * (uint32_t)v387->f940064[0]) >> 4;
    if ( a4 )
    {
      *(uint16_t *)(n2 - 2 * v511 + 6) += v396;
      __fwd_alt_p2_model_update_binary_pair(model_tables + 254 * (uint32_t)v508 - 254, (a4 - 1) >> 1);
    }
    else
    {
      *(uint16_t *)(n2 + 2) += v396;
    }
    goto LABEL_37;
  }
  return n0x10;
}

static inline int32_t __fwd_alt_p2_d8_decode_body_alt_p2_decode_symbol(void *a0, char *a1) { return __alt_p2_decode_symbol((Obj13 *)a0, a1); }
static inline int32_t __fwd_alt_p2_d8_decode_body_alt_p2_context(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __alt_p2_context((Obj11 *)a0, a1, a2, (Obj11 *)a3, (Obj11 *)a4); }

void __alt_p2_d8_decode_body(Obj69 *lpAddress, char ArgList, const __m128 &a3__ref, const __m128 &a4__ref, uint8_t *a5, int32_t i, int32_t a7)
{
  struct alignas(16) {   // 44 bytes, the frame Hex-Rays could not name
      int32_t   v95;
      uint8_t   _pad1[4];
      uint8_t   _gap0[4];   // was uint8_t * v97
      uint8_t   _pad3[32];
      uint8_t   _tail[4];   // alignas(16) rounds 44 up
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 48,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad3) == 12,
                "the named part of the frame moved");
  int32_t &v95 = __frame.v95;
  uint32_t &v96 = *(uint32_t *)((char *)&__frame.v95);
  uint8_t *v97;
  ;
  char *v11;
  char *v25;
  char *v32;
  char *v54;
  char *v62;
  char *v69;
  char *v80;
  char *v84;
  Obj130 *v88;
  uintptr_t v21, v28, v36, v38, v39, v40, v41, v42, v43, v44, v48, v58, v66, v76, v78, v82, v86,
            v90, v93;
  __m128 a3 = a3__ref;
  __m128 a4 = a4__ref;
  __m128 v47;
  __m128i *v53;
  bool v17;
  int16_t v14, v24;
  char *v12;
  int32_t i_1, v13, v22, v23, v29, v30, v31, v37, v49, *v50, v51, v52, v59,
          v60, v61, v67, v68, v72, v73, v74, v75, v77, v79, v81, v83, v85,
          v87, v89, v92, v94;
  int64_t v16;
  uint16_t *v15;
  uint32_t j;
  __rc_begin_decode(ArgList);
  i_1 = i;
  v11 = (char *)(lpAddress->f278736[0]);
  v12 = (char *)(v11);
  if ( i > 0 )
  {
    v95 = 0;
    while ( 1 )
    {
      v13 = *(int16_t *)((uintptr_t)v11 - 18) >> 4;
      lpAddress->f278708 = lpAddress->f278704 + *(uint32_t *)((char *)lpAddress + (4 * v13 + 278960));
      lpAddress->f278712 = lpAddress->f278704 + *(uint32_t *)((char *)lpAddress + (4 * v13 + 278944));
      v14 = (uint8_t)((*(uint16_t *)(lpAddress->f278736[0] - 18) >> 4)
                            + *(uint8_t *)(__fwd_alt_p2_d8_decode_body_alt_p2_decode_symbol(
                                           (uint16_t *)((uintptr_t)lpAddress
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
                                                              + 940072),
                                           (char *)lpAddress + 278708)
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
      *(uint8_t *)(lpAddress->f278736[0] + 17) = 2;
      *(uint8_t *)(lpAddress->f278736[0] + 16) = (*(int16_t *)(lpAddress->f278736[0] + 4) <= 0)
                                                       + (*(int16_t *)(lpAddress->f278736[0] + 4) < 0);
      v11 = (char *)(lpAddress->f278736[0] + 18);
      v17 = v95 + 1 < i;
      lpAddress->f278736[0] = (uintptr_t)(uint8_t *)v11;
      ++v95;
      if ( !v17 )
        break;
      v12 = (char *)(lpAddress->f278736[0]);
    }
    i_1 = i;
  }
  bmf_copy((void *)(v11), (const void *)((uintptr_t)v11 - 18), 18);
  v21 = lpAddress->f278736[0];
  v22 = *(uint32_t *)(v21 - 10);
  v23 = *(uint32_t *)(v21 - 6);
  *(uint32_t *)(v21 + 18) = *(uint32_t *)(v21 - 18);
  *(uint32_t *)(v21 + 22) = *(uint32_t *)(v21 - 14);
  v24 = *(uint16_t *)(v21 - 2);
  *(uint32_t *)(v21 + 26) = v22;
  *(uint32_t *)(v21 + 30) = v23;
  *(uint16_t *)(v21 + 34) = v24;
  v25 = (char *)(lpAddress->f278736[0]);
  bmf_copy((void *)((char *)v25 + 36), (const void *)((uintptr_t)v25 - 18), 18);
  v28 = lpAddress->f278736[0];
  v29 = *(uint32_t *)(v28 - 14);
  v30 = *(uint32_t *)(v28 - 10);
  *(uint32_t *)(v28 + 54) = *(uint32_t *)(v28 - 18);
  v31 = *(uint32_t *)(v28 - 6);
  *(uint32_t *)(v28 + 58) = v29;
  LOWORD(v29) = *(uint16_t *)(v28 - 2);
  *(uint32_t *)(v28 + 62) = v30;
  *(uint32_t *)(v28 + 66) = v31;
  *(uint16_t *)(v28 + 70) = v29;
  v32 = (char *)(lpAddress->f278736[0]);
  bmf_copy((void *)((char *)v32 + 72), (const void *)((uintptr_t)v32 - 18), 18);
  v36 = lpAddress->f278736[0];
  v37 = -18 * i_1;
  *(uint32_t *)(v36 + v37 - 18) = *(uint32_t *)(v36 - 18 * i_1);
  *(uint32_t *)(v36 + v37 - 14) = *(uint32_t *)(v36 - 18 * i_1 + 4);
  *(uint32_t *)(v36 + v37 - 10) = *(uint32_t *)(v36 - 18 * i_1 + 8);
  *(uint32_t *)(v36 + v37 - 6) = *(uint32_t *)(v36 - 18 * i_1 + 12);
  *(uint16_t *)(v36 + v37 - 2) = *(uint16_t *)(v36 - 18 * i_1 + 16);
  v38 = lpAddress->f278736[0];
  *(uint32_t *)(v38 + v37 - 36) = *(uint32_t *)(v38 - 18 * i_1 + 18);
  *(uint32_t *)(v38 + v37 - 32) = *(uint32_t *)(v38 - 18 * i_1 + 22);
  *(uint32_t *)(v38 + v37 - 28) = *(uint32_t *)(v38 - 18 * i_1 + 26);
  *(uint32_t *)(v38 + v37 - 24) = *(uint32_t *)(v38 - 18 * i_1 + 30);
  *(uint16_t *)(v38 + v37 - 20) = *(uint16_t *)(v38 - 18 * i_1 + 34);
  v39 = lpAddress->f278736[0];
  *(uint32_t *)(v39 + v37 - 54) = *(uint32_t *)(v39 - 18 * i_1 + 36);
  *(uint32_t *)(v39 + v37 - 50) = *(uint32_t *)(v39 - 18 * i_1 + 40);
  *(uint32_t *)(v39 + v37 - 46) = *(uint32_t *)(v39 - 18 * i_1 + 44);
  *(uint32_t *)(v39 + v37 - 42) = *(uint32_t *)(v39 - 18 * i_1 + 48);
  *(uint16_t *)(v39 + v37 - 38) = *(uint16_t *)(v39 - 18 * i_1 + 52);
  v40 = lpAddress->f278736[0];
  *(uint32_t *)(v40 + v37 - 72) = *(uint32_t *)(v40 - 18 * i_1 + 54);
  *(uint32_t *)(v40 + v37 - 68) = *(uint32_t *)(v40 - 18 * i_1 + 58);
  *(uint32_t *)(v40 + v37 - 64) = *(uint32_t *)(v40 - 18 * i_1 + 62);
  *(uint32_t *)(v40 + v37 - 60) = *(uint32_t *)(v40 - 18 * i_1 + 66);
  *(uint16_t *)(v40 + v37 - 56) = *(uint16_t *)(v40 - 18 * i_1 + 70);
  v41 = lpAddress->f278736[0];
  *(uint32_t *)(v41 + v37 - 90) = *(uint32_t *)(v41 - 18 * i_1 + 72);
  *(uint32_t *)(v41 + v37 - 86) = *(uint32_t *)(v41 - 18 * i_1 + 76);
  *(uint32_t *)(v41 + v37 - 82) = *(uint32_t *)(v41 - 18 * i_1 + 80);
  *(uint32_t *)(v41 + v37 - 78) = *(uint32_t *)(v41 - 18 * i_1 + 84);
  *(uint16_t *)(v41 + v37 - 74) = *(uint16_t *)(v41 - 18 * i_1 + 88);
  v42 = lpAddress->f278736[0];
  *(uint32_t *)(v42 + v37 - 108) = *(uint32_t *)(v42 - 18 * i_1 + 90);
  *(uint32_t *)(v42 + v37 - 104) = *(uint32_t *)(v42 - 18 * i_1 + 94);
  *(uint32_t *)(v42 + v37 - 100) = *(uint32_t *)(v42 - 18 * i_1 + 98);
  *(uint32_t *)(v42 + v37 - 96) = *(uint32_t *)(v42 - 18 * i_1 + 102);
  *(uint16_t *)(v42 + v37 - 92) = *(uint16_t *)(v42 - 18 * i_1 + 106);
  v43 = lpAddress->f278736[0];
  *(uint32_t *)(v43 + v37 - 126) = *(uint32_t *)(v43 - 18 * i_1 + 108);
  *(uint32_t *)(v43 + v37 - 122) = *(uint32_t *)(v43 - 18 * i_1 + 112);
  *(uint32_t *)(v43 + v37 - 118) = *(uint32_t *)(v43 - 18 * i_1 + 116);
  *(uint32_t *)(v43 + v37 - 114) = *(uint32_t *)(v43 - 18 * i_1 + 120);
  *(uint16_t *)(v43 + v37 - 110) = *(uint16_t *)(v43 - 18 * i_1 + 124);
  v44 = lpAddress->f278736[0];
  *(uint32_t *)(v44 + v37 - 144) = *(uint32_t *)(v44 - 18 * i_1 + 126);
  *(uint32_t *)(v44 + v37 - 140) = *(uint32_t *)(v44 - 18 * i_1 + 130);
  *(uint32_t *)(v44 + v37 - 136) = *(uint32_t *)(v44 - 18 * i_1 + 134);
  *(uint32_t *)(v44 + v37 - 132) = *(uint32_t *)(v44 - 18 * i_1 + 138);
  *(uint16_t *)(v44 + v37 - 128) = *(uint16_t *)(v44 - 18 * i_1 + 142);
  memcpy(*(char **)&lpAddress->f278760[0],lpAddress->f278756,18 * i_1 + 234);
  memcpy(*(char **)&lpAddress->f278760[1],lpAddress->f278756,18 * i_1 + 234);
  memcpy(*(char **)&lpAddress->f278760[2],lpAddress->f278756,18 * i_1 + 234);
  if ( a7 > 1 )
  {
    v96 = 0;
    v47 = 0;
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
      *(uint64_t *)&lpAddress->f278528[0] = v47.m128_u64[0];
      *(uint32_t *)((char *)&lpAddress->f278528[0] + 8) = 0;
      *(uint16_t *)((char *)&lpAddress->f278528[0] + 12) = 0;
      v53 = ((__m128i *)(((uintptr_t)lpAddress + 278543) & 0xFFFFFFF0));
      *(uint8_t *)((char *)&lpAddress->f278528[0] + 14) = 0;
      lpAddress->f278640 = v47.m128_u64[0];
      lpAddress->f278648 = v47.m128_u64[0];
      v53[0] = v47;
      v53[1] = v47;
      v53[2] = v47;
      v53[3] = v47;
      v53[4] = v47;
      v53[5] = v47;
      v53[6] = v47;
      v54 = (char *)(lpAddress->f278736[0]);
      bmf_copy((void *)(v54), (const void *)((uintptr_t)v54 - 18), 18);
      v58 = lpAddress->f278736[0];
      v59 = *(uint32_t *)(v58 - 32);
      v60 = *(uint32_t *)(v58 - 28);
      LOWORD(v51) = *(uint16_t *)(v58 - 20);
      *(uint32_t *)(v58 + 18) = *(uint32_t *)(v58 - 36);
      v61 = *(uint32_t *)(v58 - 24);
      *(uint32_t *)(v58 + 22) = v59;
      *(uint32_t *)(v58 + 26) = v60;
      *(uint32_t *)(v58 + 30) = v61;
      *(uint16_t *)(v58 + 34) = v51;
      v62 = (char *)(lpAddress->f278736[0]);
      bmf_copy((void *)((char *)v62 + 36), (const void *)((uintptr_t)v62 - 54), 18);
      v66 = lpAddress->f278736[0];
      v67 = *(uint32_t *)(v66 - 64);
      v68 = *(uint32_t *)(v66 - 60);
      LOWORD(v51) = *(uint16_t *)(v66 - 56);
      *(uint32_t *)(v66 + 54) = *(uint32_t *)(v66 - 72);
      *(uint32_t *)(v66 + 58) = *(uint32_t *)(v66 - 68);
      *(uint32_t *)(v66 + 62) = v67;
      *(uint32_t *)(v66 + 66) = v68;
      *(uint16_t *)(v66 + 70) = v51;
      v69 = (char *)(lpAddress->f278736[0]);
      bmf_copy((void *)((char *)v69 + 72), (const void *)((uintptr_t)v69 - 90), 18);
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
      lpAddress->f278736[0] = v73;
      v76 += 144;
      lpAddress->f278736[1] = v76;
      lpAddress->f278736[2] = v75 + 144;
      lpAddress->f278736[3] = v74 + 144;
      lpAddress->f278736[4] = v72 + 144;
      bmf_copy((void *)(v73 - 18), (const void *)(v76), 18);
      v77 = lpAddress->f278736[0];
      v78 = lpAddress->f278736[1];
      bmf_copy((void *)(v77 - 36), (const void *)(v78 + 18), 18);
      v79 = lpAddress->f278736[0];
      v80 = (char *)(lpAddress->f278736[1]);
      bmf_copy((void *)(v79 - 54), (const void *)((char *)v80 + 36), 18);
      v81 = lpAddress->f278736[0];
      v82 = lpAddress->f278736[1];
      bmf_copy((void *)(v81 - 72), (const void *)(v82 + 54), 18);
      v83 = lpAddress->f278736[0];
      v84 = (char *)(lpAddress->f278736[1]);
      bmf_copy((void *)(v83 - 90), (const void *)((char *)v84 + 72), 18);
      v85 = lpAddress->f278736[0];
      v86 = lpAddress->f278736[1];
      bmf_copy((void *)(v85 - 108), (const void *)(v86 + 90), 18);
      v87 = lpAddress->f278736[0];
      v88 = (Obj130 *)(lpAddress->f278736[1]);
      bmf_copy((void *)(v87 - 126), (const void *)((char *)v88 + 108), 18);
      v89 = lpAddress->f278736[0];
      v90 = lpAddress->f278736[1];
      bmf_copy((void *)(v89 - 144), (const void *)(v90 + 126), 18);
      *(uint16_t *)(lpAddress->f278736[0] + 2) = 0;
      if ( i > 0 )
      {
        for ( j = 0; j < i; ++j )
        {
          v92 = __fwd_alt_p2_d8_decode_body_alt_p2_context((__m128 *)lpAddress, v47, a3, nullptr, nullptr);
          v93 = __fwd_alt_p2_d8_decode_body_alt_p2_decode_symbol(
                  (uint16_t *)((uintptr_t)lpAddress + 8 * lpAddress->f278704 + 940072),
                  (char *)lpAddress + 278708);
          v94 = (uint8_t)(v92 + *(uint8_t *)((char *)lpAddress + (v93 + 280496)));
          v97[j] = v94;
          __alt_p2_model(lpAddress, a4, v94, v93, v94 - v92);
          a5 = &v97[j + 1];
        }
        v47 = 0;
      }
      ++v96;
    }
    while ( v96 < a7 - 1 );
  }
  __rc_end_decode();
}
static inline void ** __fwd_alt_model_p2_d8_decode_alt_p2_free(void *a0, char a1) { return __alt_p2_free((void **)a0, a1); }
static inline void __fwd_alt_model_p2_d8_decode_alt_p2_d8_decode_body(int32_t a0, char a1, const __m128 &a2, const __m128 &a3, void *a4, int32_t a5, int32_t a6) { __alt_p2_d8_decode_body((Obj69 *)a0, a1, a2, a3, (uint8_t *)a4, a5, a6); }

void __alt_model_p2_d8_decode(const __m128 &a1__ref, const __m128 &a2__ref, uint8_t *Src, int32_t i, int32_t a5)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  void *v5, **lpAddress;
  v5 = bmf_page_alloc(0x103E30u);
  if ( v5 )
    lpAddress = (void **)__alt_p2_alloc((char *)v5, i, 0);
  else
    lpAddress = nullptr;
  __fwd_alt_model_p2_d8_decode_alt_p2_d8_decode_body((int32_t)lpAddress, i, a1, a2, Src, i, a5);
  if ( lpAddress )
    __fwd_alt_model_p2_d8_decode_alt_p2_free(lpAddress, 1);
}
static inline int32_t __fwd_alt_model_p2_decode_alt_p2_decode_symbol(void *a0, char *a1) { return __alt_p2_decode_symbol((Obj13 *)a0, a1); }
static inline void ** __fwd_alt_model_p2_decode_alt_p2_free(void *a0, char a1) { return __alt_p2_free((void **)a0, a1); }
static inline int32_t __fwd_alt_model_p2_decode_alt_p2_context(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __alt_p2_context((Obj11 *)a0, a1, a2, (Obj11 *)a3, (Obj11 *)a4); }

int32_t __alt_model_p2_decode(uint16_t *p_i, uint8_t *Src)
{
  struct alignas(16) {   // 276 bytes, the frame Hex-Rays could not name
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
      uint8_t slot152[4];
      uint8_t   _gap2[4];   // was uint8_t * Src_1
      uint8_t   _gap3[4];   // was int32_t v153
      uint8_t   _gap4[4];   // was int32_t v154
      uint8_t   _gap5[16];   // was Obj11 * plane
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
  int32_t &v150 = *(int32_t *)((char *)__frame.slot152);
  int32_t &v151 = *(int32_t *)((char *)__frame.slot152);
  uint8_t *Src_1;
  int32_t v153;
  int32_t v154;
  Obj11 * plane[4];
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
  Obj8 *v56;
  Obj19 *v14;
  Obj31 *v25;
  char *v26;
  char *v34;
  char *v41;
  char *v63;
  char *v71;
  char *v78;
  char *v89;
  char *v93;
  uint8_t *v30, *v38, *v45, *v57, *v67, *v75, *v85, *v87, *v91, *v95, *v97, *v99;
  int32_t v46;   // an offset from v45, not a cursor
  Obj11 *v120;
  Obj11 *v129;
  Obj11 *v111;
  __m128 v2, v3;
  __m128i *v62;
  bool v17, v109;
  char v9;
  int16_t v110;
  uint8_t *v47, *v48, *v49, *v50, *v51, *v52, *v53;   // row cursors
  uint8_t *v21, *v22, *v23, *v24, *v60, *v81, *v82, *v83, *v84, *v86, *v88, *v90,
          *v92, *v94, *v96, *v98;   // row cursors
  int32_t i, v5, n4, n4_1, v15, v16, v31, v32, v33, v39,
          v40, v58, *v59, v61, v68,
          v69, v70, v76, v77, v101, v103, v104, v105, v106, v107, v108, v112, v113,
          v114, v115, v116, v117, v118, v119, v121, v122, v123, v124, v125,
          v126, v127, v128, v130, v131, v132, v133, v134, v135, n4_3, n4_4;
  uint16_t *v102;
  Obj11 *lpAddress_1;
  uint32_t v11;
  void *v7, *v8, **lpAddress_2;
  Obj11 **v18;
  // The p2 filter coefficients drift over a run: rows 4..6 are folded into
  // rows 0..2 here and then zeroed, and the whole table goes back on the way
  // out, so a run starts from the same place the last one did.
  float saved_p2_coef[7][4];
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for ( int k = 0; k < 4; k++ )
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
  for ( int k = 0; k < 4; k++ )
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
        v8 = __alt_p2_alloc((char *)v7, i, n4);
      else
        v8 = nullptr;
      plane[n4++] = (Obj11 *)v8;
    }
    while ( n4 < plane_count );
  }
  v163 = plane_desc[(uint8_t)plane_desc[2].b1 + 1].b2 & 8;
  v9 = plane_desc[(uint8_t)__n3_0 + 1].b2;
  ArgList = plane_desc[(uint8_t)plane_desc[3].b1 + 1].b2 & 8;
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
        v150 = v5;
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
              v25 = (Obj31 *)((int32_t)*(v18 - 1));
              v26 = (char *)(v25->f278736);
              bmf_copy((void *)(v26), (const void *)((uintptr_t)v26 - 18), 18);
              v30 = v25->f278736;
              v31 = *(uint32_t *)(v30 - 14);
              v32 = *(uint32_t *)(v30 - 10);
              *(uint32_t *)(v30 + 18) = *(uint32_t *)(v30 - 18);
              v33 = *(uint32_t *)(v30 - 6);
              *(uint32_t *)(v30 + 22) = v31;
              LOWORD(v31) = *(uint16_t *)(v30 - 2);
              *(uint32_t *)(v30 + 26) = v32;
              *(uint32_t *)(v30 + 30) = v33;
              *(uint16_t *)(v30 + 34) = v31;
              v34 = (char *)(v25->f278736);
              bmf_copy((void *)((char *)v34 + 36), (const void *)((uintptr_t)v34 - 18), 18);
              v38 = v25->f278736;
              v39 = *(uint32_t *)(v38 - 10);
              v40 = *(uint32_t *)(v38 - 6);
              *(uint32_t *)(v38 + 54) = *(uint32_t *)(v38 - 18);
              *(uint32_t *)(v38 + 58) = *(uint32_t *)(v38 - 14);
              LOWORD(v34) = *(uint16_t *)(v38 - 2);
              *(uint32_t *)(v38 + 62) = v39;
              *(uint32_t *)(v38 + 66) = v40;
              *(uint16_t *)(v38 + 70) = (uint16_t)(uintptr_t)v34;
              v41 = (char *)(v25->f278736);
              bmf_copy((void *)((char *)v41 + 72), (const void *)((uintptr_t)v41 - 18), 18);
              v45 = v25->f278736;
              v46 = v160;
              Size_1 = Size;
              *(uint32_t *)(v45 + v160 - 18) = *(uint32_t *)(v160 + v45);
              *(uint32_t *)(v45 + v46 - 14) = *(uint32_t *)(v46 + v45 + 4);
              *(uint32_t *)(v45 + v46 - 10) = *(uint32_t *)(v46 + v45 + 8);
              *(uint32_t *)(v45 + v46 - 6) = *(uint32_t *)(v46 + v45 + 12);
              *(uint16_t *)(v45 + v46 - 2) = *(uint16_t *)(v46 + v45 + 16);
              v47 = v25->f278736;
              *(uint32_t *)(v46 + v47 - 36) = *(uint32_t *)(v46 + v47 + 18);
              *(uint32_t *)(v46 + v47 - 32) = *(uint32_t *)(v46 + v47 + 22);
              *(uint32_t *)(v46 + v47 - 28) = *(uint32_t *)(v46 + v47 + 26);
              *(uint32_t *)(v46 + v47 - 24) = *(uint32_t *)(v46 + v47 + 30);
              *(uint16_t *)(v46 + v47 - 20) = *(uint16_t *)(v46 + v47 + 34);
              v48 = v25->f278736;
              *(uint32_t *)(v46 + v48 - 54) = *(uint32_t *)(v46 + v48 + 36);
              *(uint32_t *)(v46 + v48 - 50) = *(uint32_t *)(v46 + v48 + 40);
              *(uint32_t *)(v46 + v48 - 46) = *(uint32_t *)(v46 + v48 + 44);
              *(uint32_t *)(v46 + v48 - 42) = *(uint32_t *)(v46 + v48 + 48);
              *(uint16_t *)(v46 + v48 - 38) = *(uint16_t *)(v46 + v48 + 52);
              v49 = v25->f278736;
              *(uint32_t *)(v46 + v49 - 72) = *(uint32_t *)(v46 + v49 + 54);
              *(uint32_t *)(v46 + v49 - 68) = *(uint32_t *)(v46 + v49 + 58);
              *(uint32_t *)(v46 + v49 - 64) = *(uint32_t *)(v46 + v49 + 62);
              *(uint32_t *)(v46 + v49 - 60) = *(uint32_t *)(v46 + v49 + 66);
              *(uint16_t *)(v46 + v49 - 56) = *(uint16_t *)(v46 + v49 + 70);
              v50 = v25->f278736;
              *(uint32_t *)(v46 + v50 - 90) = *(uint32_t *)(v46 + v50 + 72);
              *(uint32_t *)(v46 + v50 - 86) = *(uint32_t *)(v46 + v50 + 76);
              *(uint32_t *)(v46 + v50 - 82) = *(uint32_t *)(v46 + v50 + 80);
              *(uint32_t *)(v46 + v50 - 78) = *(uint32_t *)(v46 + v50 + 84);
              *(uint16_t *)(v46 + v50 - 74) = *(uint16_t *)(v46 + v50 + 88);
              v51 = v25->f278736;
              *(uint32_t *)(v46 + v51 - 108) = *(uint32_t *)(v46 + v51 + 90);
              *(uint32_t *)(v46 + v51 - 104) = *(uint32_t *)(v46 + v51 + 94);
              *(uint32_t *)(v46 + v51 - 100) = *(uint32_t *)(v46 + v51 + 98);
              *(uint32_t *)(v46 + v51 - 96) = *(uint32_t *)(v46 + v51 + 102);
              *(uint16_t *)(v46 + v51 - 92) = *(uint16_t *)(v46 + v51 + 106);
              v52 = v25->f278736;
              *(uint32_t *)(v46 + v52 - 126) = *(uint32_t *)(v46 + v52 + 108);
              *(uint32_t *)(v46 + v52 - 122) = *(uint32_t *)(v46 + v52 + 112);
              *(uint32_t *)(v46 + v52 - 118) = *(uint32_t *)(v46 + v52 + 116);
              *(uint32_t *)(v46 + v52 - 114) = *(uint32_t *)(v46 + v52 + 120);
              *(uint16_t *)(v46 + v52 - 110) = *(uint16_t *)(v46 + v52 + 124);
              v53 = v25->f278736;
              *(uint32_t *)(v46 + v53 - 144) = *(uint32_t *)(v46 + v53 + 126);
              *(uint32_t *)(v46 + v53 - 140) = *(uint32_t *)(v46 + v53 + 130);
              *(uint32_t *)(v46 + v53 - 136) = *(uint32_t *)(v46 + v53 + 134);
              *(uint32_t *)(v46 + v53 - 132) = *(uint32_t *)(v46 + v53 + 138);
              *(uint16_t *)(v46 + v53 - 128) = *(uint16_t *)(v46 + v53 + 142);
              memcpy(v25->f278760,v25->f278756,Size_1);
              memcpy(v25->f278764,v25->f278756,Size);
              memcpy(v25->f278768,v25->f278756,Size);
            }
          }
          else
          {
            v14 = (Obj19 *)((int32_t)plane[n4_2 - 1]);
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
            v18 = (Obj11 **)v159;
            memcpy(v14->f278760,*(char **)&v14->f278736[5],Size);
            memcpy(v14->f278764,*(char **)&v14->f278736[5],Size);
            memcpy(v14->f278768,*(char **)&v14->f278736[5],Size);
            v21 = (uint8_t *)v14->f278760 + 2 * v149 + 144;
            v14->f278736[0] = v14->f278736[5] + 2 * v149 + 144;
            v22 = (uint8_t *)v14->f278764;
            v14->f278736[1] = v21;
            v23 = (uint8_t *)v14->f278768;
            v14->f278736[2] = v22 + 2 * v149 + 144;
            v24 = (uint8_t *)v14->f278772 + 2 * v149 + 144;
            v14->f278736[3] = v23 + 2 * v149 + 144;
            v14->f278736[4] = v24;
          }
          v56 = (Obj8 *)((int32_t)*(v18 - 1));
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
          v2 = 0;
          v56->f278672 = (uint8_t *)v59;
          v61 = *v59;
          *(uint32_t *)(v60 - 4) = *v59;
          v62 = ((__m128i *)(((uintptr_t)v56 + 278543) & 0xFFFFFFF0));
          *(uint32_t *)(v56->f278668 - 8) = v61;
          v56->f278528 = 0;
          v56->f278536 = 0;
          v56->f278540 = 0;
          v56->f278542 = 0;
          v56->f278640 = 0;
          v56->f278648 = 0;
          v62[0] = 0;
          v62[1] = 0;
          v62[2] = 0;
          v62[3] = 0;
          v62[4] = 0;
          v62[5] = 0;
          v62[6] = 0;
          v63 = (char *)(v56->f278736[0]);
          bmf_copy((void *)(v63), (const void *)((uintptr_t)v63 - 18), 18);
          v67 = v56->f278736[0];
          v68 = *(uint32_t *)(v67 - 32);
          v69 = *(uint32_t *)(v67 - 28);
          *(uint32_t *)(v67 + 18) = *(uint32_t *)(v67 - 36);
          v70 = *(uint32_t *)(v67 - 24);
          *(uint32_t *)(v67 + 22) = v68;
          *(uint32_t *)(v67 + 26) = v69;
          *(uint32_t *)(v67 + 30) = v70;
          *(uint16_t *)(v67 + 34) = *(uint16_t *)(v67 - 20);
          v71 = (char *)(v56->f278736[0]);
          bmf_copy((void *)((char *)v71 + 36), (const void *)((uintptr_t)v71 - 54), 18);
          v75 = v56->f278736[0];
          v76 = *(uint32_t *)(v75 - 64);
          v77 = *(uint32_t *)(v75 - 60);
          *(uint32_t *)(v75 + 54) = *(uint32_t *)(v75 - 72);
          *(uint32_t *)(v75 + 58) = *(uint32_t *)(v75 - 68);
          *(uint32_t *)(v75 + 62) = v76;
          *(uint32_t *)(v75 + 66) = v77;
          *(uint16_t *)(v75 + 70) = *(uint16_t *)(v75 - 56);
          v78 = (char *)(v56->f278736[0]);
          bmf_copy((void *)((char *)v78 + 72), (const void *)((uintptr_t)v78 - 90), 18);
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
          bmf_copy((void *)(v82 - 18), (const void *)(v85), 18);
          v86 = v56->f278736[0];
          v87 = v56->f278736[1];
          bmf_copy((void *)(v86 - 36), (const void *)(v87 + 18), 18);
          v88 = v56->f278736[0];
          v89 = (char *)(v56->f278736[1]);
          bmf_copy((void *)(v88 - 54), (const void *)((char *)v89 + 36), 18);
          v90 = v56->f278736[0];
          v91 = v56->f278736[1];
          bmf_copy((void *)(v90 - 72), (const void *)(v91 + 54), 18);
          v92 = v56->f278736[0];
          v93 = (char *)(v56->f278736[1]);
          bmf_copy((void *)(v92 - 90), (const void *)((char *)v93 + 72), 18);
          v94 = v56->f278736[0];
          v95 = v56->f278736[1];
          bmf_copy((void *)(v94 - 108), (const void *)(v95 + 90), 18);
          v96 = v56->f278736[0];
          v97 = v56->f278736[1];
          bmf_copy((void *)(v96 - 126), (const void *)(v97 + 108), 18);
          v98 = v56->f278736[0];
          v99 = v56->f278736[1];
          bmf_copy((void *)(v98 - 144), (const void *)(v99 + 126), 18);
          *(uint16_t *)(v56->f278736[0] + 2) = 0;
          n4_1 = plane_count;
        }
        while ( plane_count > n4_2 );
        v11 = v165;
        v5 = v150;
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
          lpAddress_1 = (Obj11 *)(plane[0]);
          v101 = __fwd_alt_model_p2_decode_alt_p2_context((__m128 *)plane[0], v2, v3, plane[2], plane[1]);
          v102 = (uint16_t *)&((uint32_t *)lpAddress_1)[2 * *(uint32_t *)&lpAddress_1->f278528[11] + 235018];
          v168 = v101;
          v103 = __fwd_alt_model_p2_decode_alt_p2_decode_symbol(v102, (char *)((uint32_t *)lpAddress_1 + 69677));
          v104 = (uint8_t)(v168 + *((uint8_t *)lpAddress_1 + v103 + 280496));
          __alt_p2_model((Obj69 *)lpAddress_1, __xmmword_439B60, v104, v103, v104 - v168);
          v105 = *(uint32_t *)&lpAddress_1->f278528[13];
          v106 = *(int16_t *)(v105 - 10);
          v107 = 32 * *(int16_t *)(v105 - 4);
          ctx_bias[0] += 32 * *(int16_t *)(v105 - 6);
          v108 = *(int16_t *)(v105 - 8);
          ctx_bias[1] += v107;
          ctx_bias[2] += 32 * v106;
          ctx_bias[3] += 32 * v108;
          v109 = v163 == 0;
          Src[(uint8_t)plane_desc[1].b1] = v104;
          if ( v109 )
            v110 = 0;
          else
            v110 = 16 * Src[(uint8_t)plane_desc[1].b1];
          v111 = (Obj11 *)(plane[1]);
          *(uint16_t *)(plane[1]->f278528[13].m128_i32[0] + 2) = v110;
          v112 = __fwd_alt_model_p2_decode_alt_p2_context(v111, v2, v3, plane[0], plane[2]);
          v113 = __fwd_alt_model_p2_decode_alt_p2_decode_symbol(&v111->f940064.m128_u16[4 * v111->f278528[11].m128_i32[0] + 4], (char *)&v111->f278528[11].m128_i32[1]);
          v114 = (uint8_t)(v112 + v111->f280496.m128_i8[v113]);
          v169 = v114;
          __alt_p2_model((Obj69 *)v111, __xmmword_439B60, v114, v113, v114 - v112);
          v115 = v111->f278528[13].m128_i32[0];
          v116 = *(int16_t *)(v115 - 4);
          v117 = *(int16_t *)(v115 - 10);
          ctx_bias[0] += 32 * *(int16_t *)(v115 - 6);
          v118 = *(int16_t *)(v115 - 8);
          ctx_bias[1] += 32 * v116;
          ctx_bias[2] += 32 * v117;
          ctx_bias[3] += 32 * v118;
          v109 = ArgList == 0;
          Src[(uint8_t)plane_desc[2].b1] = v169;
          if ( v109 )
            LOWORD(v119) = 0;
          else
            v119 = (plane_desc[(uint8_t)plane_desc[3].b1 + 1].w8 * Src[(uint8_t)plane_desc[2].b1]
                  + plane_desc[(uint8_t)plane_desc[3].b1 + 1].w4 * Src[(uint8_t)plane_desc[1].b1]) >> 3;
          v120 = (Obj11 *)(plane[2]);
          *(uint16_t *)(plane[2]->f278528[13].m128_i32[0] + 2) = v119;
          v121 = __fwd_alt_model_p2_decode_alt_p2_context(v120, v2, v3, plane[0], plane[1]);
          v122 = __fwd_alt_model_p2_decode_alt_p2_decode_symbol(&v120->f940064.m128_u16[4 * v120->f278528[11].m128_i32[0] + 4], (char *)&v120->f278528[11].m128_i32[1]);
          v123 = (uint8_t)(v121 + v120->f280496.m128_i8[v122]);
          v170 = v123;
          __alt_p2_model((Obj69 *)v120, __xmmword_439B60, v123, v122, v123 - v121);
          v124 = v120->f278528[13].m128_i32[0];
          v125 = *(int16_t *)(v124 - 4);
          v126 = *(int16_t *)(v124 - 10);
          ctx_bias[0] += 32 * *(int16_t *)(v124 - 6);
          v127 = *(int16_t *)(v124 - 8);
          ctx_bias[1] += 32 * v125;
          ctx_bias[2] += 32 * v126;
          ctx_bias[3] += 32 * v127;
          Src[(uint8_t)plane_desc[3].b1] = v170;
          n4_1 = plane_count;
          if ( plane_count >= 4 )
          {
            if ( v153 )
              v128 = (plane_desc[(uint8_t)__n3_0 + 1].w12 * Src[2]
                    + plane_desc[(uint8_t)__n3_0 + 1].w8 * Src[1]
                    + plane_desc[(uint8_t)__n3_0 + 1].w4 * *Src) >> 3;
            else
              LOWORD(v128) = 0;
            v129 = (Obj11 *)(plane[3]);
            *(uint16_t *)(plane[3]->f278528[13].m128_i32[0] + 2) = v128;
            v130 = __fwd_alt_model_p2_decode_alt_p2_context(v129, v2, v3, plane[2], plane[0]);
            v131 = __fwd_alt_model_p2_decode_alt_p2_decode_symbol(&v129->f940064.m128_u16[4 * v129->f278528[11].m128_i32[0] + 4], (char *)&v129->f278528[11].m128_i32[1]);
            v154 = (uint8_t)(v130 + v129->f280496.m128_i8[v131]);
            __alt_p2_model((Obj69 *)v129, __xmmword_439B60, v154, v131, v154 - v130);
            v132 = v129->f278528[13].m128_i32[0];
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
        __fwd_alt_model_p2_decode_alt_p2_free(lpAddress_2, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline void ** __fwd_unmodel_plane_free_workspace(void *a0, char a1) { return __free_workspace((ModelBlock *)a0, a1); }
static inline void __fwd_unmodel_plane_unmodel_plane_slow(void *a0, void *a1) { __unmodel_plane_slow((ModelBlock *)a0, (char *)a1); }
static inline void __fwd_unmodel_plane_alt_model_p2_d8_decode(const __m128 &a0, const __m128 &a1, void *a2, int32_t a3, int32_t a4) { __alt_model_p2_d8_decode(a0, a1, (uint8_t *)a2, a3, a4); }
static inline int32_t __fwd_unmodel_plane_alt_model_p2_decode(void *a0, void *a1) { return __alt_model_p2_decode((uint16_t *)a0, (uint8_t *)a1); }
static inline void ** __fwd_unmodel_plane_alt_model_p1_d8_decode(char a0, void *a1, int32_t a2, int32_t a3) { return __alt_model_p1_d8_decode(a0, (uint8_t *)a1, a2, a3); }
static inline int32_t __fwd_unmodel_plane_alt_model_p1_decode(void *a0, char *a1) { return __alt_model_p1_decode((uint16_t *)a0, a1); }

void __unmodel_plane(char ArgList, const __m128 &a2__ref, const __m128 &a3__ref, uint16_t *p_i, uint8_t *Src)
{
  ;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  ModelBlock *v6;
  void *v5;
  if ( plane_alt_model )
  {
    if ( plane_predictor == 1 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_unmodel_plane_alt_model_p1_d8_decode(ArgList, Src, *p_i, p_i[1]);
      else
        __fwd_unmodel_plane_alt_model_p1_decode(p_i, (char *)Src);
    }
    else if ( plane_predictor == 2 )
    {
      if ( (p_i[5] & 0x3F) == 8 )
        __fwd_unmodel_plane_alt_model_p2_d8_decode(a2, a3, Src, *p_i, p_i[1]);
      else
        __fwd_unmodel_plane_alt_model_p2_decode(p_i, Src);
    }
  }
  else
  {
    v5 = bmf_new(0x7BA230u);
    if ( v5 )
      v6 = (ModelBlock *)((void **)__layout_workspace((int32_t)v5, p_i[1], *p_i, p_i[1], p_i[5] & 0x3F));
    else
      v6 = (ModelBlock *)(nullptr);
    __fwd_unmodel_plane_unmodel_plane_slow(v6, Src);
    if ( v6 )
      __fwd_unmodel_plane_free_workspace(v6, 1);
  }
}
static inline int32_t __fwd_alt_p2_d8_encode_body_alt_p2_encode_symbol(void *a0, char *a1, int32_t a2) { return __alt_p2_encode_symbol((Obj7 *)a0, a1, a2); }
static inline int32_t __fwd_alt_p2_d8_encode_body_alt_p2_context(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __alt_p2_context((Obj11 *)a0, a1, a2, (Obj11 *)a3, (Obj11 *)a4); }

void __alt_p2_d8_encode_body(Obj11 *lpAddress, const __m128 &a2__ref, const __m128 &a3__ref, uint8_t *a4, int32_t i, int32_t a6, uint8_t *a7)
{
  struct alignas(16) {   // 68 bytes, the frame Hex-Rays could not name
      int32_t   j;
      uint8_t   _gap0[4];   // was uint8_t * v103
      int32_t   v104;
      uint8_t   _gap1[4];   // was int8_t * v106
      uint8_t   _gap2[1];   // was char v107
      uint8_t   _pad5[3];
      uint8_t   _gap3[4];   // was Obj11 * lpAddress_1
      uint8_t   _gap4[4];   // was uint8_t * v109
      uint8_t   _pad8[4];
      uint8_t   _gap5[1];   // was uint8_t v110
      uint8_t   _pad10[35];
      uint8_t   _tail[12];   // alignas(16) rounds 68 up
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad10) == 33,
                "the named part of the frame moved");
  int32_t &j = __frame.j;
  uint32_t &v102 = *(uint32_t *)((char *)&__frame.j);
  uint8_t *v103;
  int32_t &v104 = __frame.v104;
  uint8_t * &v105 = *(uint8_t * *)((char *)&__frame.v104);
  int8_t *v106;
  char v107;
  Obj11 *lpAddress_1;
  uint8_t *v109;
  uint8_t v110;
  ;
  char *v34;
  char *v57;
  char *v65;
  char *v72;
  char *v11;
  char *v27;
  char *v23, *v30, *v38, *v40, *v41, *v42, *v43, *v44, *v45, *v46, *v51, *v61, *v69, *v79, *v81,
       *v83, *v85, *v87, *v89, *v91, *v93;
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  __m128 v50;
  __m128i *v56;
  char v14, v97;
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
  v11 = (char *)(lpAddress->f278528[13].m128_i32[0]);
  if ( i > 0 )
  {
    v103 = a4;
    for ( j = 0; j < i; ++j )
    {
      v12 = *(uint16_t *)((char *)v11 - 18) >> 4;
      v13 = (uint8_t)(*v103 - v12);
      v104 = lpAddress->f279984.m128_u8[v13];
      v14 = lpAddress->f280496.m128_i8[v104] + v12;
      n16 = (uint8_t)*a7 - (uint8_t)(v14 + *a7 - *v103);
      if ( n16 < -16 || n16 > 16 )
      {
        *a7 = *v103;
        v104 = lpAddress->f280240.m128_u8[v13];
      }
      else
      {
        *a7 = v14;
      }
      v16 = lpAddress->f278528[13].m128_i32[0];
      v17 = *(int16_t *)(v16 - 18) >> 4;
      lpAddress->f278528[11].m128_i32[1] = lpAddress->f278528[11].m128_i32[0] + lpAddress->f278896[4].m128_i32[v17];
      lpAddress->f278528[11].m128_i32[2] = lpAddress->f278528[11].m128_i32[0] + lpAddress->f278896[3].m128_i32[v17];
      __fwd_alt_p2_d8_encode_body_alt_p2_encode_symbol(
        &lpAddress->f940064.m128_u16[4 * lpAddress->f278528[11].m128_i32[0]
                                 + 4
                                 + 4
                                 * lpAddress->f278528[18].m128_i32[(*(int16_t *)(v16 - 18) <= *(int16_t *)(v16 - 36))
                                                           + 3
                                                           + (*(int16_t *)(v16 - 18) < *(int16_t *)(v16 - 36))]
                                 + 4 * lpAddress->f278528[17].m128_i32[*(uint8_t *)(v16 - 20) + 3]
                                 + 4 * lpAddress->f278528[16].m128_i32[*(uint8_t *)(v16 - 2) + 3]
                                 + 4
                                 * lpAddress->f278528[15].m128_i32[((uint32_t)(v17 - 115) >> 31)
                                                           + 3
                                                           + ((uint32_t)(v17 - 17) >> 31)]
                                 + 4 * lpAddress->f278528[20].m128_i32[0]],
        (char *)&lpAddress->f278528[11].m128_i32[1],
        v104);
      ++v103;
      LOWORD(v16) = 16 * (uint8_t)*a7;
      *(uint16_t *)lpAddress->f278528[13].m128_p[0] = v16;
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 2) = v16;
      ++a7;
      v18 = (uint16_t *)lpAddress->f278528[13].m128_i32[0];
      v19 = (int16_t)(*v18 - *(v18 - 9));
      v18[2] = v19;
      LOWORD(v19) = (WORD2(v19) ^ v19) - WORD2(v19);
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 6) = v19;
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 14) = v19;
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 12) = v19;
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 10) = v19;
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 8) = (uint32_t)*(int16_t *)(lpAddress->f278528[13].m128_i32[0] + 10) >> 1;
      *(uint8_t *)(lpAddress->f278528[13].m128_i32[0] + 17) = 2;
      *(uint8_t *)(lpAddress->f278528[13].m128_i32[0] + 16) = (*(int16_t *)(lpAddress->f278528[13].m128_i32[0] + 4) <= 0)
                                                    + (*(int16_t *)(lpAddress->f278528[13].m128_i32[0] + 4) < 0);
      v11 = (char *)(lpAddress->f278528[13].m128_i32[0] + 18);
      lpAddress->f278528[13].m128_p[0] = v11;
    }
    v8 = v103;
  }
  bmf_copy((void *)(v11), (const void *)((uintptr_t)v11 - 18), 18);
  v23 = lpAddress->f278528[13].m128_p[0];
  v24 = *(uint32_t *)(v23 - 10);
  v25 = *(uint32_t *)(v23 - 6);
  *(uint32_t *)(v23 + 18) = *(uint32_t *)(v23 - 18);
  *(uint32_t *)(v23 + 22) = *(uint32_t *)(v23 - 14);
  v26 = *(uint16_t *)(v23 - 2);
  *(uint32_t *)(v23 + 26) = v24;
  *(uint32_t *)(v23 + 30) = v25;
  *(uint16_t *)(v23 + 34) = v26;
  v27 = (char *)(lpAddress->f278528[13].m128_i32[0]);
  bmf_copy((void *)((char *)v27 + 36), (const void *)((uintptr_t)v27 - 18), 18);
  v30 = lpAddress->f278528[13].m128_p[0];
  v31 = *(uint32_t *)(v30 - 14);
  v32 = *(uint32_t *)(v30 - 10);
  *(uint32_t *)(v30 + 54) = *(uint32_t *)(v30 - 18);
  v33 = *(uint32_t *)(v30 - 6);
  *(uint32_t *)(v30 + 58) = v31;
  LOWORD(v31) = *(uint16_t *)(v30 - 2);
  *(uint32_t *)(v30 + 62) = v32;
  *(uint32_t *)(v30 + 66) = v33;
  *(uint16_t *)(v30 + 70) = v31;
  v34 = (char *)(lpAddress->f278528[13].m128_i32[0]);
  bmf_copy((void *)((char *)v34 + 72), (const void *)((uintptr_t)v34 - 18), 18);
  v38 = lpAddress->f278528[13].m128_p[0];
  v39 = -18 * i;
  *(uint32_t *)(v38 + v39 - 18) = *(uint32_t *)(v38 - 18 * i);
  *(uint32_t *)(v38 + v39 - 14) = *(uint32_t *)(v38 + v39 + 4);
  *(uint32_t *)(v38 + v39 - 10) = *(uint32_t *)(v38 + v39 + 8);
  *(uint32_t *)(v38 + v39 - 6) = *(uint32_t *)(v38 + v39 + 12);
  *(uint16_t *)(v38 + v39 - 2) = *(uint16_t *)(v38 + v39 + 16);
  v40 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v40 + v39 - 36) = *(uint32_t *)(v40 + v39 + 18);
  *(uint32_t *)(v40 + v39 - 32) = *(uint32_t *)(v40 + v39 + 22);
  *(uint32_t *)(v40 + v39 - 28) = *(uint32_t *)(v40 + v39 + 26);
  *(uint32_t *)(v40 + v39 - 24) = *(uint32_t *)(v40 + v39 + 30);
  *(uint16_t *)(v40 + v39 - 20) = *(uint16_t *)(v40 + v39 + 34);
  v41 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v41 + v39 - 54) = *(uint32_t *)(v41 + v39 + 36);
  *(uint32_t *)(v41 + v39 - 50) = *(uint32_t *)(v41 + v39 + 40);
  *(uint32_t *)(v41 + v39 - 46) = *(uint32_t *)(v41 + v39 + 44);
  *(uint32_t *)(v41 + v39 - 42) = *(uint32_t *)(v41 + v39 + 48);
  *(uint16_t *)(v41 + v39 - 38) = *(uint16_t *)(v41 + v39 + 52);
  v42 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v42 + v39 - 72) = *(uint32_t *)(v42 + v39 + 54);
  *(uint32_t *)(v42 + v39 - 68) = *(uint32_t *)(v42 + v39 + 58);
  *(uint32_t *)(v42 + v39 - 64) = *(uint32_t *)(v42 + v39 + 62);
  *(uint32_t *)(v42 + v39 - 60) = *(uint32_t *)(v42 + v39 + 66);
  *(uint16_t *)(v42 + v39 - 56) = *(uint16_t *)(v42 + v39 + 70);
  v43 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v43 + v39 - 90) = *(uint32_t *)(v43 + v39 + 72);
  *(uint32_t *)(v43 + v39 - 86) = *(uint32_t *)(v43 + v39 + 76);
  *(uint32_t *)(v43 + v39 - 82) = *(uint32_t *)(v43 + v39 + 80);
  *(uint32_t *)(v43 + v39 - 78) = *(uint32_t *)(v43 + v39 + 84);
  *(uint16_t *)(v43 + v39 - 74) = *(uint16_t *)(v43 + v39 + 88);
  v44 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v44 + v39 - 108) = *(uint32_t *)(v44 + v39 + 90);
  *(uint32_t *)(v44 + v39 - 104) = *(uint32_t *)(v44 + v39 + 94);
  *(uint32_t *)(v44 + v39 - 100) = *(uint32_t *)(v44 + v39 + 98);
  *(uint32_t *)(v44 + v39 - 96) = *(uint32_t *)(v44 + v39 + 102);
  *(uint16_t *)(v44 + v39 - 92) = *(uint16_t *)(v44 + v39 + 106);
  v45 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v45 + v39 - 126) = *(uint32_t *)(v45 + v39 + 108);
  *(uint32_t *)(v45 + v39 - 122) = *(uint32_t *)(v45 + v39 + 112);
  *(uint32_t *)(v45 + v39 - 118) = *(uint32_t *)(v45 + v39 + 116);
  *(uint32_t *)(v45 + v39 - 114) = *(uint32_t *)(v45 + v39 + 120);
  *(uint16_t *)(v45 + v39 - 110) = *(uint16_t *)(v45 + v39 + 124);
  v46 = lpAddress->f278528[13].m128_p[0];
  *(uint32_t *)(v46 + v39 - 144) = *(uint32_t *)(v46 + v39 + 126);
  *(uint32_t *)(v46 + v39 - 140) = *(uint32_t *)(v46 + v39 + 130);
  *(uint32_t *)(v46 + v39 - 136) = *(uint32_t *)(v46 + v39 + 134);
  *(uint32_t *)(v46 + v39 - 132) = *(uint32_t *)(v46 + v39 + 138);
  *(uint16_t *)(v46 + v39 - 128) = *(uint16_t *)(v46 + v39 + 142);
  Size = 18 * i + 234;
  memcpy((char *)lpAddress->f278528[14].m128_i32[2],(char *)lpAddress->f278528[14].m128_i32[1],Size);
  memcpy((char *)lpAddress->f278528[14].m128_i32[3],(char *)lpAddress->f278528[14].m128_i32[1],Size);
  memcpy((char *)lpAddress->f278528[15].m128_i32[0],(char *)lpAddress->f278528[14].m128_i32[1],Size);
  if ( a6 > 1 )
  {
    v106 = (int8_t *)&lpAddress->f278528[11].m128_i8[4];
    v105 = a7;
    v50 = 0;
    v102 = 0;
    do
    {
      v51 = lpAddress->f278528[8].m128_p[3];
      v52 = *(uint32_t *)(v51 - 4);
      v109 = v8;
      *(uint32_t *)(v51 + 4) = v52;
      *(uint32_t *)lpAddress->f278528[8].m128_i32[3] = v52;
      v53 = (int32_t *)lpAddress->f278528[8].m128_i32[1];
      v54 = lpAddress->f278528[8].m128_i32[2];
      lpAddress->f278528[8].m128_i32[1] = v54;
      lpAddress->f278528[8].m128_i32[2] = (int32_t)v53;
      v54 += 8;
      lpAddress->f278528[8].m128_i32[3] = v54;
      v53 += 2;
      lpAddress->f278528[9].m128_i32[0] = (int32_t)v53;
      v55 = *v53;
      *(uint32_t *)(v54 - 4) = *v53;
      *(uint32_t *)(lpAddress->f278528[8].m128_i32[3] - 8) = v55;
      lpAddress->f278528[0].m128_u64[0] = v50.m128_u64[0];
      lpAddress->f278528[0].m128_i32[2] = 0;
      lpAddress->f278528[0].m128_i16[6] = 0;
      lpAddress->f278528[0].m128_i8[14] = 0;
      lpAddress->f278528[7].m128_u64[0] = v50.m128_u64[0];
      lpAddress->f278528[7].m128_u64[1] = v50.m128_u64[0];
      v56 = ((__m128i *)(((uint32_t)&lpAddress->f278528[0].m128_u32[3] + 3) & 0xFFFFFFF0));
      v56[0] = v50;
      v56[1] = v50;
      v56[2] = v50;
      v56[3] = v50;
      v56[4] = v50;
      v56[5] = v50;
      v56[6] = v50;
      v57 = (char *)(lpAddress->f278528[13].m128_i32[0]);
      bmf_copy((void *)(v57), (const void *)((uintptr_t)v57 - 18), 18);
      v61 = lpAddress->f278528[13].m128_p[0];
      v62 = *(uint32_t *)(v61 - 32);
      v63 = *(uint32_t *)(v61 - 28);
      *(uint32_t *)(v61 + 18) = *(uint32_t *)(v61 - 36);
      v64 = *(uint32_t *)(v61 - 24);
      *(uint32_t *)(v61 + 22) = v62;
      *(uint32_t *)(v61 + 26) = v63;
      *(uint32_t *)(v61 + 30) = v64;
      *(uint16_t *)(v61 + 34) = *(uint16_t *)(v61 - 20);
      v65 = (char *)(lpAddress->f278528[13].m128_i32[0]);
      bmf_copy((void *)((char *)v65 + 36), (const void *)((uintptr_t)v65 - 54), 18);
      v69 = lpAddress->f278528[13].m128_p[0];
      v70 = *(uint32_t *)(v69 - 64);
      v71 = *(uint32_t *)(v69 - 60);
      *(uint32_t *)(v69 + 54) = *(uint32_t *)(v69 - 72);
      *(uint32_t *)(v69 + 58) = *(uint32_t *)(v69 - 68);
      *(uint32_t *)(v69 + 62) = v70;
      *(uint32_t *)(v69 + 66) = v71;
      *(uint16_t *)(v69 + 70) = *(uint16_t *)(v69 - 56);
      v72 = (char *)(lpAddress->f278528[13].m128_i32[0]);
      bmf_copy((void *)((char *)v72 + 72), (const void *)((uintptr_t)v72 - 90), 18);
      v75 = lpAddress->f278528[15].m128_i32[0];
      v76 = lpAddress->f278528[15].m128_i32[1];
      v77 = lpAddress->f278528[14].m128_i32[3];
      v78 = lpAddress->f278528[14].m128_i32[2];
      v79 = lpAddress->f278528[14].m128_p[1];
      lpAddress->f278528[15].m128_i32[1] = v75;
      lpAddress->f278528[15].m128_i32[0] = v77;
      lpAddress->f278528[14].m128_i32[3] = v78;
      lpAddress->f278528[14].m128_i32[1] = v76;
      lpAddress->f278528[14].m128_p[2] = v79;
      v76 += 144;
      lpAddress->f278528[13].m128_i32[0] = v76;
      v79 += 144;
      lpAddress->f278528[13].m128_p[1] = v79;
      lpAddress->f278528[13].m128_i32[2] = v78 + 144;
      lpAddress->f278528[13].m128_i32[3] = v77 + 144;
      lpAddress->f278528[14].m128_i32[0] = v75 + 144;
      bmf_copy((void *)(v76 - 18), (const void *)(v79), 18);
      v80 = lpAddress->f278528[13].m128_i32[0];
      v81 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v80 - 36), (const void *)(v81 + 18), 18);
      v82 = lpAddress->f278528[13].m128_i32[0];
      v83 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v82 - 54), (const void *)(v83 + 36), 18);
      v84 = lpAddress->f278528[13].m128_i32[0];
      v85 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v84 - 72), (const void *)(v85 + 54), 18);
      v86 = lpAddress->f278528[13].m128_i32[0];
      v87 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v86 - 90), (const void *)(v87 + 72), 18);
      v88 = lpAddress->f278528[13].m128_i32[0];
      v89 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v88 - 108), (const void *)(v89 + 90), 18);
      v90 = lpAddress->f278528[13].m128_i32[0];
      v91 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v90 - 126), (const void *)(v91 + 108), 18);
      v92 = lpAddress->f278528[13].m128_i32[0];
      v93 = lpAddress->f278528[13].m128_p[1];
      bmf_copy((void *)(v92 - 144), (const void *)(v93 + 126), 18);
      *(uint16_t *)(lpAddress->f278528[13].m128_i32[0] + 2) = 0;
      if ( i > 0 )
      {
        lpAddress_1 = (Obj11 *)(lpAddress);
        for ( k = 0; k < i; ++k )
        {
          v95 = __fwd_alt_p2_d8_encode_body_alt_p2_context(lpAddress_1, v50, a2, nullptr, nullptr);
          v107 = v109[k];
          v96 = (uint8_t)(v107 - v95);
          v97 = v95 + lpAddress_1->f280496.m128_i8[lpAddress_1->f279984.m128_u8[v96]];
          n16_1 = (uint8_t)v105[k] - (uint8_t)(v97 + v105[k] - v107);
          v99 = lpAddress_1->f279984.m128_u8[v96];
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v105[k] = v107;
            v99 = lpAddress_1->f280240.m128_u8[(uint8_t)(v107 - v95)];
          }
          else
          {
            v105[k] = v97;
          }
          v110 = v99;
          __fwd_alt_p2_d8_encode_body_alt_p2_encode_symbol(&lpAddress_1->f940064.m128_u16[4 * lpAddress_1->f278528[11].m128_i32[0] + 4], (char *)v106, v99);
          __alt_p2_model((Obj69 *)lpAddress_1, a3, (uint8_t)v105[k], v110, (uint8_t)v105[k] - v95);
          v8 = &v109[k + 1];
          v100 = (int32_t)&v105[k + 1];
        }
        lpAddress = (Obj11 *)(lpAddress_1);
        v105 = (uint8_t *)v100;
        v50 = 0;
      }
      ++v102;
    }
    while ( v102 < a6 - 1 );
  }
  __rc_end_encode();
}
static inline void ** __fwd_alt_model_p2_d8_encode_alt_p2_free(void *a0, char a1) { return __alt_p2_free((void **)a0, a1); }
static inline void __fwd_alt_model_p2_d8_encode_alt_p2_d8_encode_body(void *a0, const __m128 &a1, const __m128 &a2, void *a3, int32_t a4, int32_t a5, void *a6) { __alt_p2_d8_encode_body((Obj11 *)a0, a1, a2, (uint8_t *)a3, a4, a5, (uint8_t *)a6); }

void __alt_model_p2_d8_encode(const __m128 &a1__ref, const __m128 &a2__ref, uint8_t *a3, int32_t i, int32_t a5, uint8_t *a6)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  void * v6;
  Obj11 *lpAddress;
  v6 = bmf_page_alloc(0x103E30u);
  if ( v6 )
    lpAddress = (Obj11 *)((__m128 *)__alt_p2_alloc((char *)v6, i, 0));
  else
    lpAddress = (Obj11 *)(nullptr);
  __fwd_alt_model_p2_d8_encode_alt_p2_d8_encode_body(lpAddress, a1, a2, a3, i, a5, a6);
  if ( lpAddress )
    __fwd_alt_model_p2_d8_encode_alt_p2_free((void **)lpAddress, 1);
}
static inline int32_t __fwd_alt_model_p2_encode_alt_p2_encode_symbol(void *a0, char *a1, int32_t a2) { return __alt_p2_encode_symbol((Obj7 *)a0, a1, a2); }
static inline void ** __fwd_alt_model_p2_encode_alt_p2_free(void *a0, char a1) { return __alt_p2_free((void **)a0, a1); }
static inline int32_t __fwd_alt_model_p2_encode_alt_p2_context(void *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __alt_p2_context((Obj11 *)a0, a1, a2, (Obj11 *)a3, (Obj11 *)a4); }

int32_t __alt_model_p2_encode(BmfImage *p_i, uint8_t *a2)
{
  struct alignas(16) {   // 324 bytes, the frame Hex-Rays could not name
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
      uint8_t   _gap1[4];   // was int32_t v153
      uint8_t _pad0[4];
      uint8_t   _gap2[4];   // was uint32_t v154
      uint8_t   _gap3[4];   // was int32_t v155
      uint8_t   _gap4[4];   // was int32_t n3
      uint8_t   _gap5[4];   // was int32_t v157
      uint8_t   _gap6[4];   // was int32_t v158
      uint8_t   _gap7[16];   // was Obj11 * plane
      uint8_t   _gap8[4];   // was int32_t v163
      uint8_t   _gap9[4];   // was int32_t v164
      uint8_t   _gap10[4];   // was uint32_t Size
      uint8_t   _gap11[4];   // was uint8_t * v166
      uint8_t   _gap12[4];   // was int32_t i_2
      uint8_t   _gap13[4];   // was int32_t v168
      uint8_t   _gap14[4];   // was int32_t v169
      uint8_t   _gap15[4];   // was uint32_t v170
      uint8_t   _gap16[4];   // was int32_t n4_2
      uint8_t   _gap17[4];   // was uint32_t i
      uint8_t   _gap18[4];   // was int32_t v173
      uint8_t   _gap19[4];   // was int32_t v174
      uint8_t   _gap20[4];   // was int32_t v175
      uint8_t   _gap21[4];   // was int32_t v176
      uint8_t   _gap22[4];   // was int32_t v177
      uint8_t   _gap23[4];   // was int32_t v178
      uint8_t   _gap24[4];   // was int32_t v179
      uint8_t   _gap25[4];   // was int32_t v180
      uint8_t   _gap26[4];   // was int32_t v181
      uint8_t   _gap27[4];   // was int32_t v182
      uint8_t   _gap28[4];   // was int32_t v183
      uint8_t   _gap29[4];   // was int32_t v184
      uint8_t   _gap30[4];   // was uint32_t v185
      uint8_t _pad1[40];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 336, "frame layout moved");
  uint32_t Size_1;
  int32_t v153;
  uint32_t v154;
  int32_t v155;
  int32_t n3;
  int32_t v157;
  int32_t v158;
  Obj11 * plane[4];
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
  Obj8 *v55;
  uint8_t *v44;
  Obj19 *v13;
  Obj31 *v24;
  char *v25;
  char *v33;
  char *v40;
  char *v62;
  char *v70;
  char *v77;
  uint8_t *v29, *v37, *v56, *v66, *v74, *v84, *v86, *v88, *v90, *v92, *v94, *v96, *v98;
  int32_t v45;   // an offset from v44, not a cursor
  Obj11 *v125;
  Obj11 *v133;
  Obj11 *v114;
  __m128 v2, v3;
  __m128i *v61;
  bool v16;
  char v9;
  int16_t v113;
  uint8_t *v46, *v47, *v48, *v49, *v50, *v51, *v52;   // row cursors
  uint8_t *v20, *v21, *v22, *v23, *v59, *v80, *v81, *v82, *v83, *v85, *v87, *v89,
          *v91, *v93, *v95, *v97;   // row cursors
  int32_t i_1, v5, n4, n4_1, v14, v15, v30, v31, v32, v38, v99,
          v39, v57, *v58, v60, v67,
          v68, v69, v75, v76, v100, v101, v102, v104, v105, v106, v107, n16, v109,
          v110, v111, v112, v115, v116, v117, v118, n16_1, v120, v121, v122,
          v123, v124, v126, v127, v128, v129, n16_2, v131, v132, v134, v135,
          v136, v137, n16_3, v139, n4_3, n4_4;
  uint32_t v11;
  Obj11 *lpAddress_1;
  void *v7, *v8, **lpAddress_2;
  Obj11 **v17;
  v166 = a2;
  // The p2 filter coefficients drift over a run: rows 4..6 are folded into
  // rows 0..2 here and then zeroed, and the whole table goes back on the way
  // out, so a run starts from the same place the last one did.
  float saved_p2_coef[7][4];
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for ( int k = 0; k < 4; k++ )
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
  for ( int k = 0; k < 4; k++ )
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
        v8 = (void *)__alt_p2_alloc((char *)v7, i_1, n4);
      else
        v8 = nullptr;
      plane[n4++] = (Obj11 *)v8;
    }
    while ( n4 < plane_count );
  }
  v169 = plane_desc[(uint8_t)plane_desc[2].b1 + 1].b2 & 8;
  v9 = plane_desc[(uint8_t)__n3_0 + 1].b2;
  v168 = plane_desc[(uint8_t)plane_desc[3].b1 + 1].b2 & 8;
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
              v24 = (Obj31 *)((int32_t)*(v17 - 1));
              v25 = (char *)(v24->f278736);
              bmf_copy((void *)(v25), (const void *)((uintptr_t)v25 - 18), 18);
              v29 = v24->f278736;
              v30 = *(uint32_t *)(v29 - 14);
              v31 = *(uint32_t *)(v29 - 10);
              *(uint32_t *)(v29 + 18) = *(uint32_t *)(v29 - 18);
              v32 = *(uint32_t *)(v29 - 6);
              *(uint32_t *)(v29 + 22) = v30;
              LOWORD(v30) = *(uint16_t *)(v29 - 2);
              *(uint32_t *)(v29 + 26) = v31;
              *(uint32_t *)(v29 + 30) = v32;
              *(uint16_t *)(v29 + 34) = v30;
              v33 = (char *)(v24->f278736);
              bmf_copy((void *)((char *)v33 + 36), (const void *)((uintptr_t)v33 - 18), 18);
              v37 = v24->f278736;
              v38 = *(uint32_t *)(v37 - 10);
              v39 = *(uint32_t *)(v37 - 6);
              *(uint32_t *)(v37 + 54) = *(uint32_t *)(v37 - 18);
              *(uint32_t *)(v37 + 58) = *(uint32_t *)(v37 - 14);
              LOWORD(v33) = *(uint16_t *)(v37 - 2);
              *(uint32_t *)(v37 + 62) = v38;
              *(uint32_t *)(v37 + 66) = v39;
              *(uint16_t *)(v37 + 70) = (uint16_t)(uintptr_t)v33;
              v40 = (char *)(v24->f278736);
              bmf_copy((void *)((char *)v40 + 72), (const void *)((uintptr_t)v40 - 18), 18);
              v44 = v24->f278736;
              v45 = v164;
              Size_1 = Size;
              *(uint32_t *)(v44 + v164 - 18) = *(uint32_t *)(v164 + v44);
              *(uint32_t *)(v44 + v45 - 14) = *(uint32_t *)(v45 + v44 + 4);
              *(uint32_t *)(v44 + v45 - 10) = *(uint32_t *)(v45 + v44 + 8);
              *(uint32_t *)(v44 + v45 - 6) = *(uint32_t *)(v45 + v44 + 12);
              *(uint16_t *)(v44 + v45 - 2) = *(uint16_t *)(v45 + v44 + 16);
              v46 = v24->f278736;
              *(uint32_t *)(v45 + v46 - 36) = *(uint32_t *)(v45 + v46 + 18);
              *(uint32_t *)(v45 + v46 - 32) = *(uint32_t *)(v45 + v46 + 22);
              *(uint32_t *)(v45 + v46 - 28) = *(uint32_t *)(v45 + v46 + 26);
              *(uint32_t *)(v45 + v46 - 24) = *(uint32_t *)(v45 + v46 + 30);
              *(uint16_t *)(v45 + v46 - 20) = *(uint16_t *)(v45 + v46 + 34);
              v47 = v24->f278736;
              *(uint32_t *)(v45 + v47 - 54) = *(uint32_t *)(v45 + v47 + 36);
              *(uint32_t *)(v45 + v47 - 50) = *(uint32_t *)(v45 + v47 + 40);
              *(uint32_t *)(v45 + v47 - 46) = *(uint32_t *)(v45 + v47 + 44);
              *(uint32_t *)(v45 + v47 - 42) = *(uint32_t *)(v45 + v47 + 48);
              *(uint16_t *)(v45 + v47 - 38) = *(uint16_t *)(v45 + v47 + 52);
              v48 = v24->f278736;
              *(uint32_t *)(v45 + v48 - 72) = *(uint32_t *)(v45 + v48 + 54);
              *(uint32_t *)(v45 + v48 - 68) = *(uint32_t *)(v45 + v48 + 58);
              *(uint32_t *)(v45 + v48 - 64) = *(uint32_t *)(v45 + v48 + 62);
              *(uint32_t *)(v45 + v48 - 60) = *(uint32_t *)(v45 + v48 + 66);
              *(uint16_t *)(v45 + v48 - 56) = *(uint16_t *)(v45 + v48 + 70);
              v49 = v24->f278736;
              *(uint32_t *)(v45 + v49 - 90) = *(uint32_t *)(v45 + v49 + 72);
              *(uint32_t *)(v45 + v49 - 86) = *(uint32_t *)(v45 + v49 + 76);
              *(uint32_t *)(v45 + v49 - 82) = *(uint32_t *)(v45 + v49 + 80);
              *(uint32_t *)(v45 + v49 - 78) = *(uint32_t *)(v45 + v49 + 84);
              *(uint16_t *)(v45 + v49 - 74) = *(uint16_t *)(v45 + v49 + 88);
              v50 = v24->f278736;
              *(uint32_t *)(v45 + v50 - 108) = *(uint32_t *)(v45 + v50 + 90);
              *(uint32_t *)(v45 + v50 - 104) = *(uint32_t *)(v45 + v50 + 94);
              *(uint32_t *)(v45 + v50 - 100) = *(uint32_t *)(v45 + v50 + 98);
              *(uint32_t *)(v45 + v50 - 96) = *(uint32_t *)(v45 + v50 + 102);
              *(uint16_t *)(v45 + v50 - 92) = *(uint16_t *)(v45 + v50 + 106);
              v51 = v24->f278736;
              *(uint32_t *)(v45 + v51 - 126) = *(uint32_t *)(v45 + v51 + 108);
              *(uint32_t *)(v45 + v51 - 122) = *(uint32_t *)(v45 + v51 + 112);
              *(uint32_t *)(v45 + v51 - 118) = *(uint32_t *)(v45 + v51 + 116);
              *(uint32_t *)(v45 + v51 - 114) = *(uint32_t *)(v45 + v51 + 120);
              *(uint16_t *)(v45 + v51 - 110) = *(uint16_t *)(v45 + v51 + 124);
              v52 = v24->f278736;
              *(uint32_t *)(v45 + v52 - 144) = *(uint32_t *)(v45 + v52 + 126);
              *(uint32_t *)(v45 + v52 - 140) = *(uint32_t *)(v45 + v52 + 130);
              *(uint32_t *)(v45 + v52 - 136) = *(uint32_t *)(v45 + v52 + 134);
              *(uint32_t *)(v45 + v52 - 132) = *(uint32_t *)(v45 + v52 + 138);
              *(uint16_t *)(v45 + v52 - 128) = *(uint16_t *)(v45 + v52 + 142);
              memcpy(v24->f278760,v24->f278756,Size_1);
              memcpy(v24->f278764,v24->f278756,Size);
              memcpy(v24->f278768,v24->f278756,Size);
            }
          }
          else
          {
            v13 = (Obj19 *)((int32_t)plane[n4_2 - 1]);
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
            v17 = (Obj11 **)v163;
            memcpy(v13->f278760,*(char **)&v13->f278736[5],Size);
            memcpy(v13->f278764,*(char **)&v13->f278736[5],Size);
            memcpy(v13->f278768,*(char **)&v13->f278736[5],Size);
            v20 = (uint8_t *)v13->f278760 + 2 * v153 + 144;
            v13->f278736[0] = v13->f278736[5] + 2 * v153 + 144;
            v21 = (uint8_t *)v13->f278764;
            v13->f278736[1] = v20;
            v22 = (uint8_t *)v13->f278768;
            v13->f278736[2] = v21 + 2 * v153 + 144;
            v23 = (uint8_t *)v13->f278772 + 2 * v153 + 144;
            v13->f278736[3] = v22 + 2 * v153 + 144;
            v13->f278736[4] = v23;
          }
          v55 = (Obj8 *)((int32_t)*(v17 - 1));
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
          v2 = 0;
          v55->f278672 = (uint8_t *)v58;
          v60 = *v58;
          *(uint32_t *)(v59 - 4) = *v58;
          v61 = ((__m128i *)(((uintptr_t)v55 + 278543) & 0xFFFFFFF0));
          *(uint32_t *)(v55->f278668 - 8) = v60;
          v55->f278528 = 0;
          v55->f278536 = 0;
          v55->f278540 = 0;
          v55->f278542 = 0;
          v55->f278640 = 0;
          v55->f278648 = 0;
          v61[0] = 0;
          v61[1] = 0;
          v61[2] = 0;
          v61[3] = 0;
          v61[4] = 0;
          v61[5] = 0;
          v61[6] = 0;
          v62 = (char *)(v55->f278736[0]);
          bmf_copy((void *)(v62), (const void *)((uintptr_t)v62 - 18), 18);
          v66 = v55->f278736[0];
          v67 = *(uint32_t *)(v66 - 32);
          v68 = *(uint32_t *)(v66 - 28);
          *(uint32_t *)(v66 + 18) = *(uint32_t *)(v66 - 36);
          v69 = *(uint32_t *)(v66 - 24);
          *(uint32_t *)(v66 + 22) = v67;
          *(uint32_t *)(v66 + 26) = v68;
          *(uint32_t *)(v66 + 30) = v69;
          *(uint16_t *)(v66 + 34) = *(uint16_t *)(v66 - 20);
          v70 = (char *)(v55->f278736[0]);
          bmf_copy((void *)((char *)v70 + 36), (const void *)((uintptr_t)v70 - 54), 18);
          v74 = v55->f278736[0];
          v75 = *(uint32_t *)(v74 - 64);
          v76 = *(uint32_t *)(v74 - 60);
          *(uint32_t *)(v74 + 54) = *(uint32_t *)(v74 - 72);
          *(uint32_t *)(v74 + 58) = *(uint32_t *)(v74 - 68);
          *(uint32_t *)(v74 + 62) = v75;
          *(uint32_t *)(v74 + 66) = v76;
          *(uint16_t *)(v74 + 70) = *(uint16_t *)(v74 - 56);
          v77 = (char *)(v55->f278736[0]);
          bmf_copy((void *)((char *)v77 + 72), (const void *)((uintptr_t)v77 - 90), 18);
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
          bmf_copy((void *)(v81 - 18), (const void *)(v84), 18);
          v85 = v55->f278736[0];
          v86 = v55->f278736[1];
          bmf_copy((void *)(v85 - 36), (const void *)(v86 + 18), 18);
          v87 = v55->f278736[0];
          v88 = v55->f278736[1];
          bmf_copy((void *)(v87 - 54), (const void *)(v88 + 36), 18);
          v89 = v55->f278736[0];
          v90 = v55->f278736[1];
          bmf_copy((void *)(v89 - 72), (const void *)(v90 + 54), 18);
          v91 = v55->f278736[0];
          v92 = v55->f278736[1];
          bmf_copy((void *)(v91 - 90), (const void *)(v92 + 72), 18);
          v93 = v55->f278736[0];
          v94 = v55->f278736[1];
          bmf_copy((void *)(v93 - 108), (const void *)(v94 + 90), 18);
          v95 = v55->f278736[0];
          v96 = v55->f278736[1];
          bmf_copy((void *)(v95 - 126), (const void *)(v96 + 108), 18);
          v97 = v55->f278736[0];
          v98 = v55->f278736[1];
          bmf_copy((void *)(v97 - 144), (const void *)(v98 + 126), 18);
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
          v184 = (uint8_t)plane_desc[1].b1;
          ctx_bias[0] = v102 >> 3;
          ctx_bias[1] = v101 >> 3;
          ctx_bias[2] = v100 >> 3;
          ctx_bias[3] = v99 >> 3;
          lpAddress_1 = (Obj11 *)(plane[0]);
          v104 = v166[(uint8_t)plane_desc[1].b1];
          v178 = __fwd_alt_model_p2_encode_alt_p2_context((__m128 *)plane[0], v2, v3, plane[2], plane[1]);
          v181 = (uint8_t)(v104 - v178);
          v105 = (uint8_t)((uint8_t *)lpAddress_1)[v181 + 279984];
          v106 = v166[v184];
          v173 = (uint8_t)(v178 + ((uint8_t *)lpAddress_1)[v105 + 280496]);
          v107 = (uint8_t)(v173 + v106 - v104);
          n16 = v106 - v107;
          if ( n16 < -16 || n16 > 16 )
          {
            v105 = (uint8_t)((uint8_t *)lpAddress_1)[v181 + 280240];
          }
          else
          {
            v104 = v173;
            v166[v184] = v107;
          }
          __fwd_alt_model_p2_encode_alt_p2_encode_symbol(
            (uint16_t *)&((uint8_t *)lpAddress_1)[8 * *((uint32_t *)lpAddress_1 + 69676) + 940072],
            (char *)lpAddress_1 + 278708,
            v105);
          __alt_p2_model((Obj69 *)lpAddress_1, __xmmword_439B50, v104, v105, v104 - v178);
          v109 = *((uint32_t *)lpAddress_1 + 69684);
          v110 = *(int16_t *)(v109 - 4);
          v111 = *(int16_t *)(v109 - 10);
          v112 = *(int16_t *)(v109 - 8);
          ctx_bias[0] += 32 * *(int16_t *)(v109 - 6);
          ctx_bias[1] += 32 * v110;
          ctx_bias[2] += 32 * v111;
          ctx_bias[3] += 32 * v112;
          if ( v169 )
            v113 = 16 * v166[(uint8_t)plane_desc[1].b1];
          else
            v113 = 0;
          v114 = (Obj11 *)(plane[1]);
          *(uint16_t *)(plane[1]->f278528[13].m128_i32[0] + 2) = v113;
          v176 = (uint8_t)plane_desc[2].b1;
          v115 = v166[(uint8_t)plane_desc[2].b1];
          v179 = __fwd_alt_model_p2_encode_alt_p2_context(v114, v2, v3, plane[0], plane[2]);
          v183 = (uint8_t)(v115 - v179);
          v116 = v114->f279984.m128_u8[v183];
          v117 = v166[v176];
          v175 = (uint8_t)(v179 + v114->f280496.m128_i8[v116]);
          v118 = (uint8_t)(v175 + v117 - v115);
          n16_1 = v117 - v118;
          if ( n16_1 < -16 || n16_1 > 16 )
          {
            v116 = v114->f280240.m128_u8[v183];
          }
          else
          {
            v115 = v175;
            v166[v176] = v118;
          }
          __fwd_alt_model_p2_encode_alt_p2_encode_symbol(&v114->f940064.m128_u16[4 * v114->f278528[11].m128_i32[0] + 4], (char *)&v114->f278528[11].m128_i32[1], v116);
          __alt_p2_model((Obj69 *)v114, __xmmword_439B50, v115, v116, v115 - v179);
          v120 = v114->f278528[13].m128_i32[0];
          v121 = *(int16_t *)(v120 - 4);
          v122 = *(int16_t *)(v120 - 10);
          v123 = *(int16_t *)(v120 - 8);
          ctx_bias[0] += 32 * *(int16_t *)(v120 - 6);
          ctx_bias[1] += 32 * v121;
          ctx_bias[2] += 32 * v122;
          ctx_bias[3] += 32 * v123;
          if ( v168 )
            v124 = (plane_desc[(uint8_t)plane_desc[3].b1 + 1].w8 * v166[(uint8_t)plane_desc[2].b1]
                  + plane_desc[(uint8_t)plane_desc[3].b1 + 1].w4 * v166[(uint8_t)plane_desc[1].b1]) >> 3;
          else
            LOWORD(v124) = 0;
          v125 = (Obj11 *)(plane[2]);
          *(uint16_t *)(plane[2]->f278528[13].m128_i32[0] + 2) = v124;
          v177 = (uint8_t)plane_desc[3].b1;
          v126 = v166[(uint8_t)plane_desc[3].b1];
          v180 = __fwd_alt_model_p2_encode_alt_p2_context(v125, v2, v3, plane[0], plane[1]);
          v182 = (uint8_t)(v126 - v180);
          v127 = v125->f279984.m128_u8[v182];
          v128 = v166[v177];
          v174 = (uint8_t)(v180 + v125->f280496.m128_i8[v127]);
          v129 = (uint8_t)(v174 + v128 - v126);
          n16_2 = v128 - v129;
          if ( n16_2 < -16 || n16_2 > 16 )
          {
            v127 = v125->f280240.m128_u8[v182];
          }
          else
          {
            v126 = v174;
            v166[v177] = v129;
          }
          __fwd_alt_model_p2_encode_alt_p2_encode_symbol(&v125->f940064.m128_u16[4 * v125->f278528[11].m128_i32[0] + 4], (char *)&v125->f278528[11].m128_i32[1], v127);
          __alt_p2_model((Obj69 *)v125, __xmmword_439B50, v126, v127, v126 - v180);
          v131 = v125->f278528[13].m128_i32[0];
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
              v132 = (plane_desc[(uint8_t)__n3_0 + 1].w12 * v166[2]
                    + plane_desc[(uint8_t)__n3_0 + 1].w8 * v166[1]
                    + plane_desc[(uint8_t)__n3_0 + 1].w4 * *v166) >> 3;
            else
              LOWORD(v132) = 0;
            v133 = (Obj11 *)(plane[3]);
            *(uint16_t *)(plane[3]->f278528[13].m128_i32[0] + 2) = v132;
            n3 = (uint8_t)__n3_0;
            v134 = v166[(uint8_t)__n3_0];
            v157 = __fwd_alt_model_p2_encode_alt_p2_context(v133, v2, v3, plane[2], plane[0]);
            v158 = (uint8_t)(v134 - v157);
            v135 = v133->f279984.m128_u8[v158];
            v136 = v166[n3];
            v137 = (uint8_t)(v157 + v133->f280496.m128_i8[v135] + v136 - v134);
            n16_3 = v136 - v137;
            if ( n16_3 < -16 || n16_3 > 16 )
            {
              v135 = v133->f280240.m128_u8[v158];
            }
            else
            {
              v134 = (uint8_t)(v157 + v133->f280496.m128_i8[v135]);
              v166[n3] = v137;
            }
            __fwd_alt_model_p2_encode_alt_p2_encode_symbol(&v133->f940064.m128_u16[4 * v133->f278528[11].m128_i32[0] + 4], (char *)&v133->f278528[11].m128_i32[1], v135);
            __alt_p2_model((Obj69 *)v133, __xmmword_439B50, v134, v135, v134 - v157);
            v139 = v133->f278528[13].m128_i32[0];
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
        __fwd_alt_model_p2_encode_alt_p2_free(lpAddress_2, 1);
        n4_3 = plane_count;
      }
      ++n4_4;
    }
    while ( n4_4 < n4_3 );
  }
  return n4_3;
}
static inline uint32_t __fwd_model_plane_init_encode_symbol_list(void *a0, int32_t a1, int32_t a2, int32_t a3) { return __init_symbol_list((int32_t *)a0, a1, a2, a3); }
static inline void ** __fwd_model_plane_free_workspace(void *a0, char a1) { return __free_workspace((ModelBlock *)a0, a1); }
static inline int32_t __fwd_model_plane_code_pixel(void *a0, int32_t a1) { return __code_pixel((ModelBlock *)a0, a1); }
static inline void __fwd_model_plane_reduce_alphabet(ModelBlock *a0, char a1, void *a2) { __reduce_alphabet(a0, a1, (uint8_t *)a2); }
static inline void __fwd_model_plane_alt_model_p2_d8_encode(const __m128 &a0, const __m128 &a1, void *a2, int32_t a3, int32_t a4, void *a5) { __alt_model_p2_d8_encode(a0, a1, (uint8_t *)a2, a3, a4, (uint8_t *)a5); }
static inline int32_t __fwd_model_plane_alt_model_p2_encode(void *a0, void *a1) { return __alt_model_p2_encode((BmfImage *)a0, (uint8_t *)a1); }
static inline void __fwd_model_plane_alt_model_p1_d8_encode(void *a0, int32_t a1, int32_t a2, void *a3) { __alt_model_p1_d8_encode((uint8_t *)a0, a1, a2, (uint8_t *)a3); }
static inline int32_t __fwd_model_plane_alt_model_p1_encode(void *a0, char *a1) { return __alt_model_p1_encode((uint16_t *)a0, a1); }

void __model_plane(const __m128 &a1__ref, const __m128 &a2__ref, BmfImage *p_i, uint8_t *a4, uint8_t *a5)
{
  struct alignas(16) {   // 96 bytes, the frame Hex-Rays could not name
      uint8_t   _gap0[4];   // was int32_t Size
      uint8_t   _pad1[4];
      int32_t   v58;
      ModelBlock *   Blocka_5;
      ModelBlock *   Blocka_2;
      uint8_t   _gap1[4];   // was int32_t v64
      uint8_t   _gap2[4];   // was int32_t v65
      uint8_t   _gap3[4];   // was int32_t v66
      uint8_t   _gap4[4];   // was int32_t v67
      uint8_t   _gap5[4];   // was int32_t v68
      uint8_t   _gap6[4];   // was int32_t v69
      uint8_t   _gap7[4];   // was int32_t v70
      uint8_t   _gap8[4];   // was char * v71
      uint8_t   _gap9[4];   // was int32_t v72
      uint8_t   _gap10[4];   // was int32_t v73
      uint8_t   _gap11[4];   // was uint32_t n5
      uint8_t   _pad16[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 96,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad16) == 64,
                "the named part of the frame moved");
  int32_t Size;
  int32_t &v58 = __frame.v58;
  int32_t &v59 = __frame.v58;
  ModelBlock * &Blocka_5 = __frame.Blocka_5;
  int32_t &v61 = *(int32_t *)((char *)&__frame.Blocka_5);
  ModelBlock * &Blocka = __frame.Blocka_2;
  ModelBlock * &Blocka_2 = __frame.Blocka_2;
  int32_t v64;
  int32_t v65;
  int32_t v66;
  int32_t v67;
  int32_t v68;
  int32_t v69;
  int32_t v70;
  char *v71;
  int32_t v72;
  int32_t v73;
  uint32_t n5;
  ;
  ModelBlock *Blocka_1;
  uint8_t *v46, *v50;   // row cursors out of f56
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  bool v43;
  char v7, *buf;
  int16_t __model_plane_n2, v22;
  ModelBlock *Blocka_3;
  ModelBlock *Blocka_4;
  int32_t v8, v10, v11, v14, n2_1, n2_2, v17, v18, v19, v20, v21, v35, v36, v41, v42, v44, v45,
          v53, v54, v55, v56;
  uint8_t *v28, *v29, *v30, *v47, *v48, *v49;   // row cursors out of f56
  uint32_t *v12, n0x10000, *v24, v31, *v32, *v33, v34, v37, *v38, *v39, v40;
  uint8_t *v51, *v52;
  void *v5;
  if ( plane_alt_model )
  {
    if ( ::plane_predictor == 1 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __fwd_model_plane_alt_model_p1_d8_encode(a4, p_i->width, p_i->height, a5);
      else
        __fwd_model_plane_alt_model_p1_encode(p_i, (char *)a4);
    }
    else if ( ::plane_predictor == 2 )
    {
      if ( (p_i->depth & 0x3F) == 8 )
        __fwd_model_plane_alt_model_p2_d8_encode(a1, a2, a4, p_i->width, p_i->height, a5);
      else
        __fwd_model_plane_alt_model_p2_encode(p_i, a4);
    }
  }
  else
  {
    v5 = bmf_new(0x7BA230u);
    if ( v5 )
      Blocka_3 = (ModelBlock *)(__layout_workspace((int32_t)v5, p_i->height, p_i->width, p_i->height, p_i->depth & 0x3F));
    else
      Blocka_3 = (ModelBlock *)(0);
    __rc_begin_encode();
    __fwd_model_plane_reduce_alphabet(Blocka_3, v7, a4);
    v64 = 0;
    v8 = 0;
    Blocka_2 = (ModelBlock *)((uint32_t *)Blocka_3);
    Blocka_4 = (ModelBlock *)(Blocka_3);
    do
    {
      v10 = (uint8_t)ctx_group_flags[v8];
      *((uint8_t *)Blocka_2 + v10 + 1078244) = v8;
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
        v71 = (char *)((uintptr_t)Blocka_4 + 15 * v11);
        do
        {
          *(uint8_t *)(v71 + 75 * n5 + 1078308) = v64;
          v73 = Blocka_2->f16;
          v12 = &((uint32_t *)Blocka_2)[4 * v64];
          __model_plane_n2 = 2;
          *((uint16_t *)v12 + 48) = 2;
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
            *((uint16_t *)v12 + 52) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 52) = n2_2;
          }
          if ( v65 )
          {
            __model_plane_n2 += n2_1;
            n2_1 = 0;
            *((uint16_t *)v12 + 51) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 51) = n2_1;
          }
          if ( v70 )
          {
            *((uint16_t *)v12 + 49) = v14 + __model_plane_n2;
            LOWORD(v14) = 0;
            *((uint16_t *)v12 + 50) = 0;
          }
          else
          {
            *((uint16_t *)v12 + 50) = v14;
            *((uint16_t *)v12 + 49) = __model_plane_n2;
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
            *((uint16_t *)v12 + 48) = 0;
          }
          if ( *((uint16_t *)v12 + v72 + 48)
            && *((uint16_t *)v12 + n5 + 48)
            && (v20 = *((uint8_t *)v12 + 110), v20 <= v73) )
          {
            v21 = 1;
            v22 = (uint8_t)(1 << ((5 - v20) & 31));
            *((uint8_t *)v12 + 111) = v22;
            *((uint16_t *)v12 + 54) = v22 << 6;
            *((uint16_t *)v12 + v72 + 48) += v22;
            *((uint16_t *)v12 + n5 + 48) += *((uint8_t *)v12 + 111);
            *((uint16_t *)v12 + 53) = *((uint16_t *)v12 + 48)
                                 + *((uint16_t *)v12 + 52)
                                 + *((uint16_t *)v12 + 51)
                                 + *((uint16_t *)v12 + 50)
                                 + *((uint16_t *)v12 + 49);
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
    buf = (char *)bmf_new(Blocka_2->f16);
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
    Blocka_2->f1078224 = (uint32_t)((uint32_t *)Blocka_2 + 269546);
    __fwd_model_plane_init_encode_symbol_list((int32_t *)((char *)Blocka_1 + 1078184), 0, Blocka_1->f16, 1);
    Blocka_2->f1078232 = (uint32_t **)((uint32_t *)Blocka_2 + 269554);
    v31 = Blocka_2->f16;
    v32 = (uint32_t *)bmf_new(24 * v31 + 4);
    if ( v32 )
    {
      *v32 = v31;
      v33 = v32 + 1;
      if ( v31 )
      {
        if ( v31 >> 1 )
        {
          v34 = 0;
          do
          {
            v35 = 12 * v34;
            v32[v35 + 6] = 0;
            ++v34;
            v32[v35 + 12] = 0;
          }
          while ( v34 < v31 >> 1 );
          Blocka_1 = (ModelBlock *)((int32_t)Blocka_2);
          v36 = 2 * v34 + 1;
        }
        else
        {
          v36 = 1;
        }
        if ( v31 > v36 - 1 )
          v32[6 * v36] = 0;
      }
    }
    else
    {
      v33 = nullptr;
    }
    v37 = Blocka_1->f16;
    Blocka_1->f1078208 = (uint8_t *)v33;
    v38 = (uint32_t *)bmf_new(24 * v37 + 4);
    if ( v38 )
    {
      *v38 = v37;
      v39 = v38 + 1;
      if ( v37 )
      {
        if ( v37 >> 1 )
        {
          Blocka = (ModelBlock *)((void *)Blocka_1);
          v40 = 0;
          do
          {
            v41 = 12 * v40;
            v38[v41 + 6] = 0;
            ++v40;
            v38[v41 + 12] = 0;
          }
          while ( v40 < v37 >> 1 );
          Blocka_1 = (ModelBlock *)((int32_t)Blocka);
          v42 = 2 * v40 + 1;
        }
        else
        {
          v42 = 1;
        }
        if ( v37 > v42 - 1 )
          v38[6 * v42] = 0;
      }
    }
    else
    {
      v39 = nullptr;
    }
    v43 = Blocka_1->f16 <= 0;
    Blocka_1->f1078212 = (uint8_t *)v39;
    if ( !v43 )
    {
      v44 = 0;
      do
      {
        __fwd_model_plane_init_encode_symbol_list((int32_t *)(Blocka_1->f1078208 + 24 * v44), 0, 99, 0);
        __fwd_model_plane_init_encode_symbol_list((int32_t *)(Blocka_1->f1078212 + 24 * v44++), 0, 33, 0);
      }
      while ( v44 < Blocka_1->f16 );
    }
    if ( Blocka_1->f4 > 0 )
    {
      v59 = *(uint32_t *)&Blocka_1->f1078236;
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
            v56 = __fwd_model_plane_code_pixel((int32_t *)Blocka_1, v55);
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
    __fwd_model_plane_free_workspace((void **)Blocka_1, 1);
  }
}
static inline char * __fwd_model_planes_colour_transform(void *a0, void *a1, int32_t a2, char a3) { return __colour_transform((char *)a0, (char *)a1, a2, a3); }
static inline void __fwd_model_planes_model_plane(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __model_plane(a0, a1, (BmfImage *)a2, (uint8_t *)a3, (uint8_t *)a4); }

void __model_planes(char *Blockb, char *Srca_3, int32_t a3, char a4, const __m128 &a5__ref, const __m128 &a6__ref)
{
  struct alignas(16) {   // 76 bytes, the frame Hex-Rays could not name
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
  uint16_t (&p_i)[2] = __frame.p_i;
  int32_t &v49 = __frame.v49;
  int32_t &v50 = __frame.v50;
  int32_t &v51 = __frame.v51;
  int32_t &v52 = __frame.v52;
  int32_t &v53 = __frame.v53;
  ;
  char *v12;   // was int32_t: these hold addresses
  __m128 a5 = a5__ref;
  __m128 a6 = a6__ref;
  char *Srca_1, v8, *Srca_2;
  uint8_t *__model_planes_buf;
  int32_t n1008, v17, v18, v19;
  v53 = a3;
  Srca_1 = Srca_3;
  v8 = plane_desc[a3 + 1].b2;
  v52 = 16 * a3;
  plane_predictor = v8 & 3;
  plane_alt_model = (uint8_t)(plane_desc[a3 + 1].b2 & 4) >> 2;
  Srca_2 = Srca_3;
  __fwd_model_planes_colour_transform(Blockb, Srca_3, a3, a4);
  __model_planes_buf = ::hist_scratch;
  v12 = (char *)((uintptr_t)(::hist_scratch + 15) & 0xFFFFFFF0);
  *(uint64_t *)::hist_scratch = 0;
  *((uint32_t *)__model_planes_buf + 2) = 0;
  *((uint16_t *)__model_planes_buf + 6) = 0;
  __model_planes_buf[14] = 0;
  *((uint64_t *)__model_planes_buf + 126) = 0;
  *((uint64_t *)__model_planes_buf + 127) = 0;
  n1008 = 1008;
  do
  {
    *(__m128i *)(v12 + n1008 - 16) = 0;
    *(__m128i *)(v12 + n1008 - 32) = 0;
    *(__m128i *)(v12 + n1008 - 48) = 0;
    *(__m128i *)(v12 + n1008 - 64) = 0;
    *(__m128i *)(v12 + n1008 - 80) = 0;
    *(__m128i *)(v12 + n1008 - 96) = 0;
    *(__m128i *)(v12 + n1008 - 112) = 0;
    n1008 -= 112;
  }
  while ( n1008 );
  // always taken: -S
  {
    v17 = *((uint32_t *)Blockb + 1);
    v18 = *((uint32_t *)Blockb + 2);
    v19 = *((uint32_t *)Blockb + 3);
    *(uint32_t *)p_i = *(uint32_t *)Blockb;
    v49 = v17;
    v50 = v18;
    BYTE2(v50) = 72;
    v51 = v19;
    // never taken: -E is 0
    if ( plane_predictor == 1 && !plane_alt_model )
      __predict_med((char *)Srca_1, *(uint16_t *)Blockb, *((uint16_t *)Blockb + 1));
    __fwd_model_planes_model_plane(a5, a6, p_i, (uint8_t *)Srca_1, Srca_2);
    // `if ( Srca_2 != Srca_1 )` stood here, and behind it an interleave and a
    // free.  It was the test for "the -E block above allocated a second
    // buffer"; with that block gone, both names hold the caller's one buffer
    // and the test is false on every path.  Deleting a block does not delete
    // the test that asked whether it ran, and this one outlived it by a
    // fortnight -- as the last thing in the file gcov could report as never
    // executed.  tools/deadcheck.py looks for the shape now.
  }
}

static inline char * __fwd_transform_planes_colour_transform(void *a0, void *a1, int32_t a2, char a3) { return __colour_transform((char *)a0, (char *)a1, a2, a3); }
static inline void __fwd_transform_planes_model_plane(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __model_plane(a0, a1, (BmfImage *)a2, (uint8_t *)a3, (uint8_t *)a4); }

void __transform_planes(BmfImage *p_i, int32_t a2, char a3, const __m128 &a4__ref, const __m128 &a5__ref)
{
  struct alignas(16) {   // 76 bytes, the frame Hex-Rays could not name
      int32_t   v32;
      int32_t   v33;
      int32_t   Size;
      int32_t   n4;
      char *    Srca;
      uint8_t   _gap0[4];   // was int32_t n4_3
      uint16_t * p_i_1;
      uint8_t   _gap1[4];   // was char * Src
      uint8_t   _gap2[4];   // was uint16_t * Srca_3
      uint8_t   _gap3[4];   // was char * p_ia
      uint8_t   _gap4[4];   // was char * Buffer_1
      uint8_t   _pad11[32];
      uint8_t   _tail[4];   // alignas(16) rounds 76 up
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80,
                "frame layout moved");
  static_assert(sizeof(void *) != 4
                || __builtin_offsetof(__typeof__(__frame), _pad11) == 44,
                "the named part of the frame moved");
  int32_t n4_3;
  char *Src;
  uint16_t *Srca_3;
  char *p_ia;
  char *Buffer_1;
  ;
  __m128 a4 = a4__ref;
  __m128 a5 = a5__ref;
  char *__transform_planes_Buffer, *p_ia_1, *Src_1, v11, *Src_3, *Src_2, *v20;
  int32_t n4_1, v14, predictor, v16, Size_3, n4_2, v21, i, Size_4,
          v24;
  uint8_t *Srca_1;
  memset(hist_scratch,0,4096);
  __transform_planes_Buffer = (char *)::coded_buf;
  p_ia_1 = (char *)::coded_buf + 16;
  *((uint32_t *)::coded_buf + 4) = *(uint32_t *)&p_i->width;
  *((uint32_t *)p_ia_1 + 1) = *((uint32_t *)p_i + 1);
  *((uint32_t *)p_ia_1 + 2) = *((uint32_t *)p_i + 2);
  *((uint32_t *)p_ia_1 + 3) = *((uint32_t *)p_i + 3);
  Srca_1 = (uint8_t *)((uint16_t *)p_i + 8);
  memcpy(__transform_planes_Buffer + 32,(char *)p_i + 16,*((uint32_t *)p_i + 3));
  Src_1 = (char *)bmf_new(p_i->width * p_i->height);
  Src_3 = Src_1;
  if ( plane_count > 0 )
  {
    Src = Src_1;
    Srca_3 = (uint16_t *)p_i + 8;
    p_ia = p_ia_1;
    Buffer_1 = __transform_planes_Buffer;
    n4_1 = 0;
    do
    {
      ++n4_1;
      v14 = BYTE1(plane_desc[n4_1].w0);
      predictor = plane_desc[v14 + 1].b2 & 3;
      ::plane_predictor = predictor;
      v16 = (uint8_t)(plane_desc[v14 + 1].b2 & 4) >> 2;
      plane_alt_model = v16;
      if ( ((plane_desc[v14 + 1].b2 & 8) != 0 || predictor) && !v16 )
      {
        __fwd_transform_planes_colour_transform(p_ia, Src, v14, v11);
        if ( ::plane_predictor != 2 )
        {
          // never taken: -E is 0
          if ( ::plane_predictor == 1 )
          {
            __predict_med((char *)Src, p_i->width, p_i->height);
          }
        }
        Size_3 = p_i->width * p_i->height;
        n4_2 = plane_count;
        Src_2 = (char *)p_i + v14 + 16;
        if ( plane_count == 1 )
        {
          memcpy((char *)p_i + v14 + 16,Src,Size_3);
        }
        else
        {
          v20 = (char *)p_i + v14;
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
    __fwd_transform_planes_model_plane(a4, a5, p_i, Srca_1, Srca_1);
  }
}
static inline char * __fwd_expand_image_interleave_plane(void *a0, void *a1, int32_t a2, char a3) { return __interleave_plane((char *)a0, (char *)a1, a2, a3); }
static inline void __fwd_expand_image_unmodel_plane(char a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { __unmodel_plane(a0, a1, a2, (uint16_t *)a3, (uint8_t *)a4); }

char * __expand_image(char *a1, const __m128 &a2__ref, const __m128 &a3__ref, int32_t a4, int32_t *p_dwLowDateTime)
{
  struct alignas(16) {   // 104 bytes, the frame Hex-Rays could not name
      uint8_t slot0[4];
      uint16_t p_i[2];
      int32_t v81;
      int32_t n4_10;
      uint32_t Size;
      int32_t n4_1;
      void *Src;
      char *v86;
      char *p_i_2;
      int32_t v88;
      uint32_t ElementCount_3;
      uint16_t Buffer_2[5];
      uint8_t v91;
      char v92;
      uint32_t ElementCount;
      uint8_t   hdr[8];   // the 8-byte member header `fread` takes in one call
      uint32_t __expand_image_Buffer;
      uint8_t _pad0[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 112, "frame layout moved");
  char *&Block = *(char **)((char *)__frame.slot0);
  char *&Blocka = *(char **)((char *)__frame.slot0);
  uint16_t (&p_i)[2] = __frame.p_i;
  int32_t &v81 = __frame.v81;
  int32_t &n4_10 = __frame.n4_10;
  uint32_t &Size = __frame.Size;
  int32_t &n4_1 = __frame.n4_1;
  void *&Src = __frame.Src;
  char *&v86 = __frame.v86;
  BmfImage *&p_i_2 = (BmfImage *&)__frame.p_i_2;
  int32_t &v88 = __frame.v88;
  uint32_t &ElementCount_3 = __frame.ElementCount_3;
  uint16_t (&Buffer_2)[5] = __frame.Buffer_2;
  uint8_t &v91 = __frame.v91;
  char &v92 = __frame.v92;
  uint32_t &ElementCount = __frame.ElementCount;
  int32_t &Buffer_ = *(int32_t *)&__frame.hdr[0];
  uint32_t &ElementSize = *(uint32_t *)&__frame.hdr[4];
  uint32_t &__expand_image_Buffer = __frame.__expand_image_Buffer;
  ;
  char *v5;   // were int32_t: these hold addresses
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  FILE *Stream_1, *Stream_v;
  BmfImage *p_i_1;
  char v10, v17, v18, v20, v34, v35, *Buffer_3, *n4_6, *n4_7, *v64;
  int32_t Buffer__1, dwLowDateTime, v21, n4, v24, predictor, v27, v28,
          v29, v30, ArgList, v33, n4_4, v37, n2_1, i, Size_4, Size_5, n4_3,
          v44, Size_2, Size_3, n4_2, v48, n2_2, n_planes, v55,
          Src_2, v58, n4_8, v61, i_1, n4_9, v76;
  uint16_t i_2;
  uint32_t __expand_image_Buffer_1, v12, *v13, ElementCount_5, ElementCount_2,
           v23, v25, Size_1, ElementCount_1, ElementCount_4, v67, v68, v69,
           v70, v74, v75;
  uint8_t *Src_1;
  void *Src_3;
  v5 = a1;
  if ( p_dwLowDateTime )
    *p_dwLowDateTime = 0;
  Stream_1 = ((BmfArc *)a1)->fp;
  if ( !Stream_1 )
    return nullptr;
  while ( 1 )
  {
    if ( fread(&__expand_image_Buffer, 4u, 1u, Stream_1) != 1 )
    {
      Stream_v = ((BmfArc *)v5)->fp;
      if ( feof(Stream_v) )
        return nullptr;
      goto LABEL_15;
    }
    __expand_image_Buffer_1 = __expand_image_Buffer;
    if ( (uint16_t)__expand_image_Buffer != 0x9081 )
      break;
    plane_desc[0].w4 = ((BYTE2(__expand_image_Buffer) << 8) - 12288) | (HIBYTE(__expand_image_Buffer) - 48);
    if ( plane_desc[0].w4 != 512 || fread(__frame.hdr, 8u, 1u, ((BmfArc *)v5)->fp) != 1 )
      break;
    fseek(((BmfArc *)v5)->fp, ElementSize, 1);
    Stream_1 = ((BmfArc *)v5)->fp;
  }
  if ( (uint16_t)__expand_image_Buffer_1 != 0x8A81
    || (plane_desc[0].w4 = ((BYTE2(__expand_image_Buffer_1) << 8) - 12288) | (HIBYTE(__expand_image_Buffer_1) - 48), plane_desc[0].w4 != 512)
    || fread(Buffer_2, 0x10u, 1u, ((BmfArc *)v5)->fp) != 1 )
  {
    Stream_v = ((BmfArc *)v5)->fp;
LABEL_15:
    fclose(Stream_v);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  v10 = v92;
  ++*(uint32_t *)v5;
  if ( v10 < 0 )
  {
    fread(__frame.hdr, 8u, 1u, ((BmfArc *)v5)->fp);
    if ( p_dwLowDateTime )
    {
      Buffer__1 = Buffer_;
      v12 = (ElementSize + (ElementSize == 0) + 3) & 0xFFFFFFFC;
      v13 = (uint32_t *)bmf_new(v12 + 8);
      *v13 = Buffer__1;
      v13[1] = v12;
      *(uint32_t *)((char *)v13 + v12 + 4) = 0;
      *p_dwLowDateTime = (int32_t)v13;
      fread(v13 + 2, ElementSize, 1u, ((BmfArc *)v5)->fp);
    }
    else
    {
      fseek(((BmfArc *)v5)->fp, ElementSize, 1);
    }
  }
  ElementCount_5 = 3 << (v91 & 31);
  if ( (v91 & 0x80) == 0 )
    ElementCount_5 = v91 & 0x80;
  ElementCount_3 = ElementCount_5;
  if ( a4 )
  {
    fseek(((BmfArc *)v5)->fp, ElementCount_3 + ElementCount, 1);
    return nullptr;
  }
  p_i_1 = (BmfImage *)((char *)__alloc_image(Buffer_2[0], Buffer_2[1], v91 & 0x3F, (uint8_t)(v91 & 0x80) >> 7, 1));
  v88 = v91;
  p_i_1->depth = v91;
  if ( p_dwLowDateTime )
  {
    dwLowDateTime = *p_dwLowDateTime;
    if ( *p_dwLowDateTime )
      LOBYTE(dwLowDateTime) = 1;
  }
  else
  {
    LOBYTE(dwLowDateTime) = 0;
  }
  v17 = v92;
  v18 = v88;
  p_i_1->flags |= v92 & 2 | ((uint8_t)dwLowDateTime << 7);
  ::plane_count = ((v18 & 0x3Fu) + 7) >> 3;
  if ( (v17 & 0x20) == 0 )
  {
    ElementCount_1 = ElementCount;
    if ( fread((char *)p_i_1 + 16, 1u, ElementCount, ((BmfArc *)v5)->fp) != ElementCount_1 )
      goto LABEL_31;
    goto LABEL_109;
  }
  plane_desc[0].w12 = 0;
  __dword_443388 = 0;
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
  coded_size = ElementCount;
  ::coded_buf = (uint8_t *)bmf_new(ElementCount);
  out_cursor = ::coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  ::packer_word = (uint32_t *)::coded_buf;
  hist_scratch = ::coded_buf + coded_size - 4096;
  ElementCount_2 = ElementCount;
  if ( fread(::coded_buf, 1u, ElementCount, ((BmfArc *)v5)->fp) != ElementCount_2 )
  {
LABEL_31:
    fclose(((BmfArc *)v5)->fp);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  v20 = p_i_1->depth;
  if ( (v20 & 0x3Fu) <= 4 || (v92 & 0x10) == 0 )
  {
    ::plane_predictor = 0;
    plane_alt_model = 0;
    // always taken: -S
      __fwd_expand_image_unmodel_plane(ElementCount_2, a2, a3, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
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
    ElementCount_2 = v75 >> (-(char)packer_free_bits & 31);
    v21 = packer_acc | v76 & 0xF;
    packer_acc = ElementCount_2;
    packer_free_bits += 32;
  }
  else
  {
    v21 = packer_acc & 0xF;
    packer_acc = (uint32_t)packer_acc >> 4;
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
    *(uint32_t *)p_i = 255;
    p_i_2 = p_i_1;
    v86 = v5;
    n4 = 0;
    do
    {
      packer_free_bits -= 6;
      if ( packer_free_bits < 0 )
      {
        v74 = *(uint32_t *)out_cursor;
        out_cursor += 4;
        v23 = packer_acc | (v74 << ((packer_free_bits + 6) & 31)) & 0x3F;
        packer_acc = v74 >> (-(char)packer_free_bits & 31);
        packer_free_bits += 32;
      }
      else
      {
        v23 = packer_acc & 0x3F;
        packer_acc = (uint32_t)packer_acc >> 6;
      }
      v25 = v23 >> 2;
      predictor = v23 & 3;
      plane_desc[n4 + 1].b2 = v25;
      plane_desc[n4 + 1].b0 = predictor;
      plane_desc[predictor + 1].b1 = n4;
      if ( (plane_desc[n4 + 1].b2 & 8) != 0 )
      {
        packer_free_bits -= 8;
        if ( packer_free_bits < 0 )
        {
          v70 = *(uint32_t *)out_cursor;
          out_cursor += 4;
          v27 = packer_acc | *(uint32_t *)p_i & (v70 << ((packer_free_bits + 8) & 31));
          packer_acc = v70 >> (-(char)packer_free_bits & 31);
          packer_free_bits += 32;
        }
        else
        {
          LOBYTE(v27) = packer_acc & LOBYTE(p_i[0]);
          packer_acc = (uint32_t)packer_acc >> 8;
        }
        plane_desc[n4 + 1].b3 = v27;
        if ( predictor > 1 )
        {
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            v69 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            v28 = packer_acc | *(uint32_t *)p_i & (v69 << ((packer_free_bits + 8) & 31));
            packer_acc = v69 >> (-(char)packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v28 = packer_acc & *(uint32_t *)p_i;
            packer_acc = (uint32_t)packer_acc >> 8;
          }
          plane_desc[n4 + 1].w4 = v28 - 64;
          packer_free_bits -= 8;
          if ( packer_free_bits < 0 )
          {
            v68 = *(uint32_t *)out_cursor;
            out_cursor += 4;
            v29 = packer_acc | *(uint32_t *)p_i & (v68 << ((packer_free_bits + 8) & 31));
            packer_acc = v68 >> (-(char)packer_free_bits & 31);
            packer_free_bits += 32;
          }
          else
          {
            v29 = packer_acc & *(uint32_t *)p_i;
            packer_acc = (uint32_t)packer_acc >> 8;
          }
          plane_desc[n4 + 1].w8 = v29 - 64;
          if ( predictor > 2 )
          {
            packer_free_bits -= 8;
            if ( packer_free_bits < 0 )
            {
              v67 = *(uint32_t *)out_cursor;
              out_cursor += 4;
              v30 = packer_acc | *(uint32_t *)p_i & (v67 << ((packer_free_bits + 8) & 31));
              packer_acc = v67 >> (-(char)packer_free_bits & 31);
              packer_free_bits += 32;
            }
            else
            {
              v30 = packer_acc & *(uint32_t *)p_i;
              packer_acc = (uint32_t)packer_acc >> 8;
            }
            plane_desc[n4 + 1].w12 = v30 - 64;
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::plane_count );
    p_i_1 = p_i_2;
    v5 = v86;
  }
  Src_1 = (uint8_t *)bmf_new(*(uint16_t *)p_i_1 * *((uint16_t *)p_i_1 + 1));
  if ( (v92 & 8) != 0 )
  {
    *(uint32_t *)p_i = *(uint32_t *)&p_i_1->width;
    v81 = *((uint32_t *)p_i_1 + 1);
    n4_10 = *((uint32_t *)p_i_1 + 2);
    Size = p_i_1->data_size;
    BYTE2(n4_10) = 72;
    if ( ::plane_count > 0 )
    {
      v86 = v5;
      ArgList = 0;
      do
      {
        v33 = (uint8_t)plane_desc[ArgList + 1].b1;
        ::plane_predictor = plane_desc[v33 + 1].b2 & 3;
        plane_alt_model = (uint8_t)(plane_desc[v33 + 1].b2 & 4) >> 2;
        // always taken: -S
          __fwd_expand_image_unmodel_plane(ArgList, a2, a3, p_i, Src_1);
        if ( ::plane_predictor )
        {
          if ( ::plane_predictor == 1 )
          {
            if ( !plane_alt_model )
              __unpredict_med((char *)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          // `else if ( !desc_slow_mode && ::plane_predictor == 2 )` -- the fast-mode
          // predictor-2 expander, never reached: -S is on.
        }
        else
        {
          __expand_predictor_mode0((uint32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
        }
        __fwd_expand_image_interleave_plane(p_i_1, (char *)Src_1, v33, v34);
        ++ArgList;
      }
      while ( ArgList < ::plane_count );
LABEL_104:
      v5 = v86;
    }
  }
  else
  {
    ::plane_predictor = plane_desc[1].b2 & 3;
    plane_alt_model = (uint8_t)(plane_desc[1].b2 & 4) >> 2;
    // always taken: -S
    {
      __fwd_expand_image_unmodel_plane(ElementCount_2, a2, a3, (uint16_t *)p_i_1, (uint8_t *)p_i_1 + 16);
      if ( plane_alt_model )
        goto LABEL_105;
    }
    if ( ::plane_count > 0 )
    {
      v86 = v5;
      n4_4 = 0;
      do
      {
        ++n4_4;
        v37 = BYTE1(plane_desc[n4_4].w0);
        n2_1 = plane_desc[v37 + 1].b2 & 3;
        ::plane_predictor = n2_1;
        if ( (plane_desc[v37 + 1].b2 & 8) != 0 || n2_1 )
        {
          i = *(uint16_t *)p_i_1;
          Src = &((char *)p_i_1)[v37 + 16];
          Size_1 = i * *((uint16_t *)p_i_1 + 1);
          n4_1 = ::plane_count;
          Size = Size_1;
          if ( ::plane_count == 1 )
          {
            memcpy((char *)Src_1,(char *)Src,Size);
            n2_2 = ::plane_predictor;
          }
          else
          {
            Block = &((char *)p_i_1)[v37];
            if ( (int32_t)Size <= 6
              || n4_1 <= 0
              || (n4_1 > 1 || Src_1 <= Src || Src_1 - (uint8_t *)Src < Size * ::plane_count)
              && (Src_1 >= Src || (uint8_t *)Src - Src_1 < Size) )
            {
              *(uint32_t *)p_i = n2_1;
              v81 = v37;
              n4_10 = n4_4;
              Size_2 = Size;
              p_i_2 = p_i_1;
              Size_3 = 0;
              n4_2 = n4_1;
              v48 = 0;
              do
              {
                Src_1[Size_3] = Block[v48 + 16];
                v48 += n4_2;
                ++Size_3;
              }
              while ( Size_3 < Size_2 );
            }
            else
            {
              *(uint32_t *)p_i = n2_1;
              v81 = v37;
              n4_10 = n4_4;
              Size_4 = Size;
              p_i_2 = p_i_1;
              Size_5 = 0;
              n4_3 = n4_1;
              v44 = 0;
              do
              {
                Src_1[Size_5] = Block[v44 + 16];
                v44 += n4_3;
                ++Size_5;
              }
              while ( Size_5 < Size_4 );
            }
            n2_2 = *(uint32_t *)p_i;
            v37 = v81;
            n4_4 = n4_10;
            p_i_1 = p_i_2;
          }
          if ( n2_2 )
          {
            if ( n2_2 == 1 )
              __unpredict_med((char *)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          else
          {
            __expand_predictor_mode0((uint32_t)Src_1, *(uint16_t *)p_i_1, *((uint16_t *)p_i_1 + 1));
          }
          __fwd_expand_image_interleave_plane(p_i_1, (char *)Src_1, v37, v35);
        }
      }
      while ( n4_4 < ::plane_count );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(Src_1);
LABEL_106:
  if ( ::coded_buf + ElementCount != out_cursor )
  {
LABEL_107:
    fclose(((BmfArc *)v5)->fp);
    ((BmfArc *)v5)->fp = 0;
    return nullptr;
  }
  free(::coded_buf);
  v88 = v91;
LABEL_109:
  if ( (v88 & 0x80) != 0 )
  {
    // `f10 < 0` was a signed char testing its own top bit -- the palette
    // flag.  depth is unsigned, so the test has to name the bit; it read as
    // always-false otherwise, which is what the gate caught.
    Buffer_3 = (p_i_1->depth & 0x80) ? &((char *)p_i_1)[p_i_1->data_size + 16] : nullptr;
    ElementCount_4 = fread(Buffer_3, 1u, ElementCount_3, ((BmfArc *)v5)->fp);
    if ( ElementCount_4 != ElementCount_3 )
      goto LABEL_107;
  }
  if ( (p_i_1->flags & 2) != 0 )
  {
    n4_6 = (char *)bmf_new(p_i_1->data_size);
    n_planes = ::plane_count;
    v55 = *((uint16_t *)p_i_1 + 1);
    n4_1 = (int32_t)n4_6;
    Src_2 = ::plane_count * (v55 - 1);
    memcpy(n4_6,(char *)p_i_1 + 16,p_i_1->data_size);
    LOWORD(v58) = *((uint16_t *)p_i_1 + 1);
    if ( (uint16_t)v58 )
    {
      Src = (void *)Src_2;
      n4_7 = (char *)n4_1;
      Blocka = n4_6;
      n4_8 = 0;
      v61 = 0;
      do
      {
        i_1 = *(uint16_t *)p_i_1;
        n4_1 = n4_8;
        v86 = (char *)v61;
        Src_3 = Src;
        p_i_2 = p_i_1;
        v64 = &((char *)p_i_1)[n4_8 + 16];
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
        p_i_1 = p_i_2;
        v58 = *((uint16_t *)p_i_2 + 1);
        n4_8 = n_planes + n4_1;
        v61 = (int32_t)(uintptr_t)v86 + 1;
      }
      while ( v86 + 1 < (char *)(uintptr_t)v58 );
      n4_6 = Blocka;
    }
    i_2 = *(uint16_t *)p_i_1;
    *(uint16_t *)p_i_1 = v58;
    p_i_1->flags ^= 2u;
    *((uint16_t *)p_i_1 + 1) = i_2;
    *((uint16_t *)p_i_1 + 2) = v58 * n_planes;
    free(n4_6);
  }
  return (char *)p_i_1;
}

static inline void __fwd_search_filter_model_planes(void *a0, void *a1, int32_t a2, char a3, const __m128 &a4, const __m128 &a5) { __model_planes((char *)a0, (char *)a1, a2, a3, a4, a5); }
static inline void __fwd_search_filter_transform_planes(void *a0, int32_t a1, char a2, const __m128 &a3, const __m128 &a4) { __transform_planes((BmfImage *)a0, a1, a2, a3, a4); }

uint32_t __search_filter(BmfImage *p_i, char a2, const __m128 &a3__ref, const __m128 &a4__ref)
{
  struct alignas(16) {   // 164 bytes, the frame Hex-Rays could not name
      uint8_t slot0[4];
      uint8_t slot4[16];
      char *v178[2];
      int32_t v179[2];
      int32_t v180;
      uint32_t v181[4];
      int32_t v182;
      void *Srca_7;
      int32_t v184;
      char *n5_1;
      uint8_t _pad0[28];
      void *Src;
      char *n4_10;
      int32_t v188;
      int32_t v189;
      char *n4_15;
      uint16_t *p_i_2;
      char *Blockb;
      uint8_t _pad1[36];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 176, "frame layout moved");
  char *&v175 = *(char **)((char *)__frame.slot0);
  char *&v176 = *(char **)((char *)__frame.slot0);
  uint64_t (&v177)[2] = *(uint64_t (*)[2])((char *)__frame.slot4);
  char *(&v178)[2] = __frame.v178;
  int32_t (&v179)[2] = __frame.v179;
  int32_t &v180 = __frame.v180;
  uint32_t (&v181)[4] = __frame.v181;
  int32_t &v182 = __frame.v182;
  void *&Srca_7 = __frame.Srca_7;
  int32_t &v184 = __frame.v184;
  char *&n5_1 = __frame.n5_1;
  void *&Src = __frame.Src;
  char *&n4_10 = __frame.n4_10;
  int32_t &v188 = __frame.v188;
  int32_t &v189 = __frame.v189;
  char *&n4_15 = __frame.n4_15;
  BmfImage *&p_i_2 = (BmfImage *&)__frame.p_i_2;
  char *&Blockb = __frame.Blockb;
  ;
  __m128 a3 = a3__ref;
  __m128 a4 = a4__ref;
  bool v35, v162;
  char v19, *v24, *v26, *v27, *n4_13, *Blockb_2, *Srca_1, v44, v62, v63,
       *n4_14, *v67, v70, *v74, v77, v82, v86, *n4_17, v93, *v101, *v105,
       *Blockb_6, *v110, *v111, *v112, *Blockb_7, v116, *v124, *v130, *v136,
       *Blockb_8, *v141, *v142, *v143, *Blockb_9, *Blockb_4, *Srca_3,
       *Blockb_5, *Srca_4, *n0x7FFFFFFF_5, *Srca_5, *Blockb_3, *Srca_2,
       *Blockb_1, *Srca, *n5_6;
  int16_t v114, v115, v145, v146;
  int32_t i, i_2, n4, v10, n4_4, v21, v22, v23, Size, n0x7FFFFFFF, v32,
          n0x7FFFFFFF_8, n0x7FFFFFFF_2, n5, n2, v40, v41, v42, v43, v45,
          n16_1, n4_7, n4_8, v61, n16_2, n4_9, v69, v71, n16,
          plane, v76, n4_20, n4_6, n16_3, n4_11, v85, n4_16,
          n16_4, n4_12, n4_18, v102, v104, v106, v108, v109, n4_19, v118,
          v119, v120, v121, v123, v125, v126, i_6, v133, v135, v137, v139,
          v140, v149, n0x7FFFFFFF_10, n0x7FFFFFFF_4, n5_3, v155,
          n0x7FFFFFFF_11, n5_4, v160, n0x7FFFFFFF_6, n5_5, v166,
          n0x7FFFFFFF_9, n0x7FFFFFFF_3, n5_2, v172, n0x7FFFFFFF_1;
  BmfImage *p_i_1;
  uint16_t i_7, v132;
  uint32_t n64_2, n64_1, n64, n64_3, n64_4;
  uint8_t *v128, *v129;
  p_i_1 = (BmfImage *)(p_i);
  i = p_i->width;
  i_2 = p_i->height;
  // `if ( (uint32_t)__n7_0 < 9 )` -- the tile-size cap that a -Q below 9 put on
  // the filter search.  -Q is 9, so the search sees the whole image.
  if ( i < 4 || i_2 < 3 )
  {
    if ( ::plane_count > 0 )
    {
      n4 = 0;
      do
      {
        v10 = 16 * n4;
        __byte_44339E[v10] = 0;
        __byte_44339D[v10] = n4;
        __byte_44339C[v10] = n4++;
      }
      while ( n4 < ::plane_count );
    }
    return 0;
  }
  __choose_plane_coding((Obj97 *)p_i_1, i_2, a2);
  // `if ( __n2_4 == 2 )` -- 94 lines of the -T2 filter-template path, gone
  // with the mode.  See REFACTORING.md §2.
  Blockb = (char *)__alloc_image(i, i_2, p_i_1->depth & 0x3F, 0, 0);
  coded_size = *((uint32_t *)Blockb + 3) + 0x20000;
  coded_buf = (uint8_t *)bmf_new(coded_size);
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t *)coded_buf;
  v179[0] = i_2 * i;
  hist_scratch = coded_buf + coded_size - 4096;
  Srca_7 = bmf_new(i_2 * i);
  n4_4 = ::plane_count;
  v21 = (p_i_1->height - i_2) >> 1;
  v22 = p_i_1->width - i;
  v23 = v21 * p_i_1->stride;
  v178[1] = (char *)v21;
  v24 = (char *)p_i_1 + ::plane_count * (v22 >> 1) + v23 + 16;
  Src = Blockb + 16;
  v178[0] = Blockb + 16;
  if ( v21 < i_2 + v21 )
  {
    Size = *((uint16_t *)Blockb + 2);
    v179[1] = i_2;
    v26 = v178[1];
    p_i_2 = (BmfImage *)(p_i_1);
    v27 = v178[0];
    do
    {
      memcpy(v27,v24,Size);
      Size = *((uint16_t *)Blockb + 2);
      v27 += Size;
      v24 += p_i_2->stride;
      ++v26;
    }
    while ( (int32_t)v26 < v179[1] + ((p_i_2->height - v179[1]) >> 1) );
    p_i_1 = (BmfImage *)(p_i_2);
    n4_4 = ::plane_count;
  }
  n4_10 = nullptr;
  v188 = 0;
  v189 = 0;
  n4_15 = nullptr;
  if ( n4_4 > 0 )
  {
    p_i_2 = (BmfImage *)(p_i_1);
    n4_13 = n4_10;
    n5_1 = nullptr;
    v182 = 0;
    v179[0] *= 8;
    while ( 1 )
    {
      v179[1] = (uint8_t)plane_desc[v182 + 1].b1;
      v180 = 16 * v179[1];
      if ( n4_13 )
      {
        n0x7FFFFFFF = 0x7FFFFFFF;
      }
      else
      {
        Blockb_1 = Blockb;
        Srca = (char *)Srca_7;
        v172 = v179[1];
        plane_desc[v179[1] + 1].b2 = 0;
        __fwd_search_filter_model_planes(Blockb_1, Srca, v172, v19, a3, a4);
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
        n5_6 = n5_1;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( n0x7FFFFFFF == 0x7FFFFFFF )
          n0x7FFFFFFF = 0x7FFFFFFF;
        else
          n5_6 = nullptr;
        n5_1 = n5_6;
      }
      Blockb_2 = Blockb;
      Srca_1 = (char *)Srca_7;
      v32 = v179[1];
      __byte_44339E[v180] = 5;
      __fwd_search_filter_model_planes(Blockb_2, Srca_1, v32, v19, a3, a4);
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
      n5 = (int32_t)n5_1;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF )
      {
        n0x7FFFFFFF = n0x7FFFFFFF_2;
        n5 = 5;
      }
      n5_1 = (char *)n5;
      if ( n0x7FFFFFFF_2 < n0x7FFFFFFF + (n0x7FFFFFFF >> 5) || n4_13 )
      {
        Blockb_3 = Blockb;
        Srca_2 = (char *)Srca_7;
        v166 = v179[1];
        __byte_44339E[v180] = 6;
        __fwd_search_filter_model_planes(Blockb_3, Srca_2, v166, v19, a3, a4);
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
        n5_2 = (int32_t)n5_1;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( n0x7FFFFFFF_3 < n0x7FFFFFFF )
        {
          n0x7FFFFFFF = n0x7FFFFFFF_3;
          n5_2 = 6;
        }
        n5_1 = (char *)n5_2;
        if ( v182 )
        {
          // always taken: -Q is 9, so `__n7_0 > 5` decides it whatever n4_13 is
          {
LABEL_191:
            Blockb_4 = Blockb;
            Srca_3 = (char *)Srca_7;
            v149 = v179[1];
            __byte_44339E[v180] = 8;
            __fwd_search_filter_model_planes(Blockb_4, Srca_3, v149, v19, a3, a4);
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
            n5_3 = (int32_t)n5_1;
            hist_scratch = coded_buf + coded_size - 4096;
            if ( n0x7FFFFFFF_4 < n0x7FFFFFFF )
            {
              n0x7FFFFFFF = n0x7FFFFFFF_4;
              n5_3 = 8;
            }
            n5_1 = (char *)n5_3;
          }
          Blockb_5 = Blockb;
          Srca_4 = (char *)Srca_7;
          v155 = v179[1];
          __byte_44339E[v180] = 13;
          __fwd_search_filter_model_planes(Blockb_5, Srca_4, v155, v19, a3, a4);
          n0x7FFFFFFF_11 = 8 * (out_cursor - coded_buf);
          n0x7FFFFFFF_5 = (char *)(plane_desc[0].w0 - packer_free_bits + n0x7FFFFFFF_11 + 32);
          v35 = 0;                            // -S
          *(uint32_t *)packer_word = packer_acc;
          out_cursor = coded_buf;
          packer_word = (uint32_t *)coded_buf;
          if ( !v35 )
            n0x7FFFFFFF_5 = (char *)n0x7FFFFFFF_11;
          v178[0] = n0x7FFFFFFF_5;
          packer_free_bits = 0;
          packer_acc = 0;
          n5_4 = (int32_t)n5_1;
          hist_scratch = coded_buf + coded_size - 4096;
          if ( (int32_t)n0x7FFFFFFF_5 < n0x7FFFFFFF )
          {
            n0x7FFFFFFF = (int32_t)n0x7FFFFFFF_5;
            n5_4 = 13;
          }
          n5_1 = (char *)n5_4;
          n2 = n5_4 & 3;
          if ( n2 == 2 || n0x7FFFFFFF + (n0x7FFFFFFF >> 5) > (int32_t)v178[0] )
          {
            Srca_5 = (char *)Srca_7;
            v160 = v179[1];
            __byte_44339E[v180] = 14;
            __fwd_search_filter_model_planes(Blockb, Srca_5, v160, v19, a3, a4);
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
            n5_5 = (int32_t)n5_1;
            if ( v162 )
              n5_5 = 14;
            n5_1 = (char *)n5_5;
            if ( v162 )
              n2 = 2;
          }
          goto LABEL_43;
        }
        n2 = n5_2 & 3;
      }
      else
      {
        if ( v182 )
          goto LABEL_191;
        n2 = (uint8_t)(uintptr_t)n5_1 & 3;
      }
LABEL_43:
      // always taken: -S
      {
        v184 = (uint8_t)(uintptr_t)n5_1 & 8;
      }
      n4_15 += n0x7FFFFFFF;
      v40 = n2 == 2;
      if ( n2 != 1 )
        n2 = 0;
      v41 = v180;
      n4_13 = (char *)((uintptr_t)n4_13 + (v40));
      v188 += n2;
      v42 = v184;
      v181[v179[1]] = n0x7FFFFFFF;
      v43 = v182;
      v189 += v42 != 0;
      __byte_44339E[v41] = (char)(uintptr_t)n5_1;
      v182 = v43 + 1;
      if ( v43 + 1 >= ::plane_count )
      {
        n4_10 = n4_13;
        p_i_1 = (BmfImage *)(p_i_2);
        break;
      }
    }
  }
  // always taken: -Q is 9
  {
    v101 = (char *)bmf_new(*((uint32_t *)Blockb + 3));
    v102 = *((uint16_t *)Blockb + 1);
    v178[1] = (char *)::plane_count;
    v180 = (int32_t)v101;
    v182 = ::plane_count * (v102 - 1);
    memcpy(v101,(char *)Src,*((uint32_t *)Blockb + 3));
    LOWORD(v104) = *((uint16_t *)Blockb + 1);
    if ( (uint16_t)v104 )
    {
      v178[0] = v101;
      v105 = v178[1];
      p_i_2 = (BmfImage *)(p_i_1);
      v106 = 0;
      Blockb_6 = Blockb;
      v108 = 0;
      do
      {
        v109 = *(uint16_t *)Blockb_6;
        v179[1] = v108;
        v179[0] = v106;
        v110 = &Blockb_6[v108 + 16];
        v111 = (char *)v180;
        do
        {
          v112 = v105;
          do
          {
            *v110++ = *v111++;
            v112 = (char *)((uintptr_t)v112 - 1);
          }
          while ( v112 );
          v110 += v182;
          --v109;
        }
        while ( v109 );
        Blockb_6 = Blockb;
        v104 = *((uint16_t *)Blockb + 1);
        v180 = (int32_t)v111;
        v108 = (int32_t)&v105[v179[1]];
        v106 = v179[0] + 1;
      }
      while ( v179[0] + 1 < v104 );
      v101 = v178[0];
      p_i_1 = (BmfImage *)(p_i_2);
    }
    Blockb_7 = Blockb;
    v114 = *(uint16_t *)Blockb;
    v115 = v104 * LOWORD(v178[1]);
    *(uint16_t *)Blockb = v104;
    *((uint16_t *)Blockb_7 + 1) = v114;
    Blockb_7[11] ^= 2u;
    *((uint16_t *)Blockb_7 + 2) = v115;
    free(v101);
    n4_19 = 0;
    if ( ::plane_count > 0 )
    {
      p_i_2 = (BmfImage *)(p_i_1);
      v118 = 0;
      while ( 1 )
      {
        v119 = (uint8_t)plane_desc[v118 + 1].b1;
        v178[0] = (char *)v119;
        __fwd_search_filter_model_planes(Blockb, (char *)Srca_7, v119, v116, a3, a4);
        v120 = 8 * (out_cursor - coded_buf);
        // never taken: -S
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        n4_19 += v120;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( v120 - (v120 >> 8) > v181[(int32_t)v178[0]] )
          break;
        if ( ++v118 >= ::plane_count )
        {
          p_i_1 = (BmfImage *)(p_i_2);
          goto LABEL_172;
        }
      }
      p_i_1 = (BmfImage *)(p_i_2);
      n4_19 += (int32_t)(n4_15 + 1);
    }
LABEL_172:
    if ( n4_19 + (n4_19 >> 12) >= (int32_t)n4_15 )
    {
      v178[0] = (char *)bmf_new(*((uint32_t *)Blockb + 3));
      v133 = ::plane_count * (*((uint16_t *)Blockb + 1) - 1);
      v178[1] = (char *)::plane_count;
      v180 = (int32_t)v178[0];
      memcpy(v178[0],(char *)Src,*((uint32_t *)Blockb + 3));
      LOWORD(v135) = *((uint16_t *)Blockb + 1);
      if ( (uint16_t)v135 )
      {
        v181[0] = v133;
        v136 = v178[1];
        p_i_2 = (BmfImage *)(p_i_1);
        v137 = 0;
        Blockb_8 = Blockb;
        v139 = 0;
        do
        {
          v140 = *(uint16_t *)Blockb_8;
          v179[1] = v139;
          v179[0] = v137;
          v141 = &Blockb_8[v139 + 16];
          v142 = (char *)v180;
          do
          {
            v143 = v136;
            do
            {
              *v141++ = *v142++;
              v143 = (char *)((uintptr_t)v143 - 1);
            }
            while ( v143 );
            v141 += v181[0];
            --v140;
          }
          while ( v140 );
          Blockb_8 = Blockb;
          v135 = *((uint16_t *)Blockb + 1);
          v180 = (int32_t)v142;
          v139 = (int32_t)&v136[v179[1]];
          v137 = v179[0] + 1;
        }
        while ( v179[0] + 1 < v135 );
        p_i_1 = (BmfImage *)(p_i_2);
      }
      Blockb_9 = Blockb;
      v176 = v178[0];
      v145 = *(uint16_t *)Blockb;
      v146 = v135 * LOWORD(v178[1]);
      *(uint16_t *)Blockb = v135;
      *((uint16_t *)Blockb_9 + 1) = v145;
      Blockb_9[11] ^= 2u;
      *((uint16_t *)Blockb_9 + 2) = v146;
      free(v176);
    }
    else
    {
      n4_15 = (char *)n4_19;
      v178[0] = (char *)bmf_new(*((uint32_t *)p_i_1 + 3));
      v121 = p_i_1->height;
      v178[1] = (char *)::plane_count;
      v179[0] = (int32_t)v178[0];
      v181[0] = ::plane_count * (v121 - 1);
      memcpy(v178[0],(char *)p_i_1 + 16,*((uint32_t *)p_i_1 + 3));
      LOWORD(v123) = p_i_1->height;
      if ( (uint16_t)v123 )
      {
        v124 = v178[1];
        v125 = 0;
        v126 = 0;
        do
        {
          i_6 = p_i_1->width;
          v179[1] = v126;
          v180 = v125;
          v128 = (uint8_t *)p_i_1 + v126 + 16;
          p_i_2 = (BmfImage *)(p_i_1);
          v129 = (uint8_t *)v179[0];
          do
          {
            v130 = v124;
            do
            {
              *v128++ = *v129++;
              v130 = (char *)((uintptr_t)v130 - 1);
            }
            while ( v130 );
            v128 += v181[0];
            --i_6;
          }
          while ( i_6 );
          v179[0] = (int32_t)v129;
          p_i_1 = (BmfImage *)(p_i_2);
          v123 = p_i_2->height;
          v126 = (int32_t)&v124[v179[1]];
          v125 = v180 + 1;
        }
        while ( v180 + 1 < v123 );
      }
      v175 = v178[0];
      i_7 = p_i_1->width;
      v132 = v123 * LOWORD(v178[1]);
      p_i_1->width = v123;
      p_i_1->height = i_7;
      *((uint8_t *)p_i_1 + 11) ^= 2u;
      p_i_1->stride = v132;
      free(v175);
    }
  }
  free(Srca_7);
  if ( ::plane_count > 2 )
  {
    if ( v188 )
    {
      n16 = 16;
      do
      {
        v179[n16 + 1] = plane_desc[n16 / 4].w12;
        v179[n16] = *(int32_t *)((char *)&::plane_count + n16 * 4);
        v178[n16 + 1] = (char *)plane_desc[n16 / 4].w4;
        v74 = (char *)plane_desc[n16 / 4].w0;
        v178[n16] = v74;
        n16 -= 4;
      }
      while ( n16 * 4 );
      if ( ::plane_count > 0 )
      {
        plane = 0;
        do
        {
          v76 = 16 * plane;
          v77 = plane_desc[plane++ + 1].b2 & 8 | 5;
          __byte_44339E[v76] = v77;
        }
        while ( plane < ::plane_count );
      }
      __fwd_search_filter_transform_planes((uint16_t *)Blockb, (int32_t)v74, v44, a3, a4);
      n4_20 = 8 * (out_cursor - coded_buf);
      // never taken: -S
      v162 = n4_20 <= (int32_t)n4_15;
      *(uint32_t *)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t *)coded_buf;
      hist_scratch = coded_buf + coded_size - 4096;
      if ( v162 )
      {
        n4_15 = (char *)n4_20;
        n4_6 = ::plane_count;
        v45 = 0;
      }
      else
      {
        n64 = 64;
        do
        {
          *(uint64_t *)(bmf_plane_desc(n64 - 8)) = *(uint64_t *)&v179[n64 / 4];
          a3 = (__m128)*(uint64_t *)&v178[n64 / 4];
          *(uint64_t *)(bmf_plane_desc(n64 - 16)) = a3.m128_u64[0];
          *(uint64_t *)(bmf_plane_desc(n64 - 24)) = v177[n64 / 8 + 1];
          a4 = (uint64_t)v177[n64 / 8];
          *(uint64_t *)(bmf_plane_desc(n64 - 32)) = a4.m128_u64[0];
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
    if ( &n4_10[v45 == 0] )
    {
      n16_1 = 16;
      do
      {
        v179[n16_1 + 1] = plane_desc[n16_1 / 4].w12;
        v179[n16_1] = *(int32_t *)((char *)&::plane_count + n16_1 * 4);
        v178[n16_1 + 1] = (char *)plane_desc[n16_1 / 4].w4;
        v178[n16_1] = (char *)plane_desc[n16_1 / 4].w0;
        n16_1 -= 4;
      }
      while ( n16_1 * 4 );
      n4_7 = ::plane_count;
      if ( ::plane_count > 0 )
      {
        n4_8 = 0;
        do
        {
          v61 = 16 * n4_8;
          v62 = plane_desc[n4_8++ + 1].b2 & 8 | 6;
          __byte_44339E[v61] = v62;
          n4_7 = ::plane_count;
        }
        while ( n4_8 < ::plane_count );
      }
      if ( (char *)n4_7 == n4_10 && n4_7 - 1 == v189 )
      {
        v45 = 0;
      }
      else
      {
        __fwd_search_filter_transform_planes((uint16_t *)Blockb, v45, v44, a3, a4);
        n4_14 = (char *)(8 * (out_cursor - coded_buf));
        // never taken: -S
        v162 = (int32_t)n4_14 <= (int32_t)n4_15;
        v178[0] = n4_14;
        *(uint32_t *)packer_word = packer_acc;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t *)coded_buf;
        hist_scratch = coded_buf + coded_size - 4096;
        if ( v162 )
        {
          n4_15 = n4_14;
          n4_10 = n4_14;
          if ( ::plane_count - 1 == v189 )
          {
            v45 = 0;
          }
          else
          {
            n16_2 = 16;
            do
            {
              v179[n16_2 + 1] = plane_desc[n16_2 / 4].w12;
              v179[n16_2] = *(int32_t *)((char *)&::plane_count + n16_2 * 4);
              v178[n16_2 + 1] = (char *)plane_desc[n16_2 / 4].w4;
              v67 = (char *)plane_desc[n16_2 / 4].w0;
              v178[n16_2] = v67;
              n16_2 -= 4;
            }
            while ( n16_2 * 4 );
            if ( ::plane_count > 0 )
            {
              n4_9 = 0;
              do
              {
                v69 = 16 * n4_9;
                v70 = plane_desc[n4_9++ + 1].b0;
                __byte_44339E[v69] |= 8 * (v70 != 0);
              }
              while ( n4_9 < ::plane_count );
            }
            __fwd_search_filter_transform_planes((uint16_t *)Blockb, (int32_t)v67, v63, a3, a4);
            v71 = 8 * (out_cursor - coded_buf);
            // never taken: -S
            v162 = v71 <= (int32_t)v178[0];
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
                *(uint64_t *)(bmf_plane_desc(n64_1 - 8)) = *(uint64_t *)&v179[n64_1 / 4];
                a3 = (__m128)*(uint64_t *)&v178[n64_1 / 4];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 16)) = a3.m128_u64[0];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 24)) = v177[n64_1 / 8 + 1];
                a4 = (uint64_t)v177[n64_1 / 8];
                *(uint64_t *)(bmf_plane_desc(n64_1 - 32)) = a4.m128_u64[0];
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
            *(uint64_t *)(bmf_plane_desc(n64_2 - 8)) = *(uint64_t *)&v179[n64_2 / 4];
            a3 = (__m128)*(uint64_t *)&v178[n64_2 / 4];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 16)) = a3.m128_u64[0];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 24)) = v177[n64_2 / 8 + 1];
            a4 = (uint64_t)v177[n64_2 / 8];
            *(uint64_t *)(bmf_plane_desc(n64_2 - 32)) = a4.m128_u64[0];
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
  if ( !n4_10 && ::plane_count > 1 )
  {
    v178[0] = (char *)bmf_new(*((uint32_t *)Blockb + 3));
    memcpy(v178[0],(char *)Src,*((uint32_t *)Blockb + 3));
    n16_3 = 16;
    do
    {
      v179[n16_3 + 1] = plane_desc[n16_3 / 4].w12;
      v179[n16_3] = *(int32_t *)((char *)&::plane_count + n16_3 * 4);
      v178[n16_3 + 1] = (char *)plane_desc[n16_3 / 4].w4;
      v178[n16_3] = (char *)plane_desc[n16_3 / 4].w0;
      n16_3 -= 4;
    }
    while ( n16_3 * 4 );
    if ( ::plane_count > 0 )
    {
      n4_11 = 0;
      do
      {
        v85 = 16 * n4_11;
        v86 = plane_desc[n4_11++ + 1].b2 & 0xFB;
        __byte_44339E[v85] = v86;
      }
      while ( n4_11 < ::plane_count );
    }
    __fwd_search_filter_transform_planes((uint16_t *)Blockb, v45, v82, a3, a4);
    n4_17 = (char *)(8 * (out_cursor - coded_buf));
    // never taken: -S
    v178[1] = n4_17;
    *(uint32_t *)packer_word = packer_acc;
    packer_free_bits = 0;
    packer_acc = 0;
    out_cursor = coded_buf;
    packer_word = (uint32_t *)coded_buf;
    n4_16 = (int32_t)n4_15;
    hist_scratch = coded_buf + coded_size - 4096;
    if ( (int32_t)n4_17 > (int32_t)n4_15 )
    {
      n64_3 = 64;
      do
      {
        *(uint64_t *)(bmf_plane_desc(n64_3 - 8)) = *(uint64_t *)&v179[n64_3 / 4];
        a3 = (__m128)*(uint64_t *)&v178[n64_3 / 4];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 16)) = a3.m128_u64[0];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 24)) = v177[n64_3 / 8 + 1];
        a4 = (uint64_t)v177[n64_3 / 8];
        *(uint64_t *)(bmf_plane_desc(n64_3 - 32)) = a4.m128_u64[0];
        n64_3 -= 32;
      }
      while ( n64_3 );
    }
    else
    {
      n4_15 = n4_17;
      v45 = 0;
    }
    if ( v189 + v188 )                // the left disjunct is -S, always true
    {
      n16_4 = 16;
      do
      {
        v179[n16_4 + 1] = plane_desc[n16_4 / 4].w12;
        v179[n16_4] = *(int32_t *)((char *)&::plane_count + n16_4 * 4);
        v178[n16_4 + 1] = (char *)plane_desc[n16_4 / 4].w4;
        v178[n16_4] = (char *)plane_desc[n16_4 / 4].w0;
        n16_4 -= 4;
      }
      while ( n16_4 * 4 );
      if ( ::plane_count > 0 )
      {
        n4_12 = 0;
        do
          plane_desc[n4_12++ + 1].b2 = 0;
        while ( n4_12 < ::plane_count );
      }
      memcpy((char *)Src,v178[0],*((uint32_t *)Blockb + 3));
      __fwd_search_filter_transform_planes((uint16_t *)Blockb, v45, v93, a3, a4);
      n4_18 = 8 * (out_cursor - coded_buf);
      // never taken: -S
      v162 = n4_18 <= (int32_t)n4_15;
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
          *(uint64_t *)(bmf_plane_desc(n64_4 - 8)) = *(uint64_t *)&v179[n64_4 / 4];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 16)) = *(uint64_t *)&v178[n64_4 / 4];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 24)) = v177[n64_4 / 8 + 1];
          *(uint64_t *)(bmf_plane_desc(n64_4 - 32)) = v177[n64_4 / 8];
          n64_4 -= 32;
        }
        while ( n64_4 );
      }
    }
    free(v178[0]);
  }
  free(coded_buf);
  free(Blockb);
  // `if ( __n2_4 == 1 )` -- 38 lines that built the -T1 filter template into
  // __dword_4410A4.  With -T off nothing writes that word, so it keeps the 0
  // BMF.exe's data segment starts it at, and the -T2 reader above is gone too.
  return v45;
}

static inline char * __fwd_bmf_open_archive_expand_image(char *a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __expand_image(a0, a1, a2, a3, (int32_t *)a4); }

BmfArc *__bmf_open_archive(BmfArc *v2, char *FileName, int32_t a2)
{
  ;
  BmfArc *v5;
  FILE *Stream_v, *Stream_1;
  __m128 v3, v4;
  const char *a_b;
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
    __fwd_bmf_open_archive_expand_image((char *)v5, v3, v4, 1, nullptr);
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

static inline char * __fwd_compress_image_expand_image(char *a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __expand_image(a0, a1, a2, a3, (int32_t *)a4); }
static inline uint32_t __fwd_compress_image_search_filter(void *a0, char a1, const __m128 &a2, const __m128 &a3) { return __search_filter((BmfImage *)a0, a1, a2, a3); }
static inline void __fwd_compress_image_model_planes(void *a0, void *a1, int32_t a2, char a3, const __m128 &a4, const __m128 &a5) { __model_planes((char *)a0, (char *)a1, a2, a3, a4, a5); }
static inline void __fwd_compress_image_transform_planes(void *a0, int32_t a1, char a2, const __m128 &a3, const __m128 &a4) { __transform_planes((BmfImage *)a0, a1, a2, a3, a4); }
static inline void __fwd_compress_image_model_plane(const __m128 &a0, const __m128 &a1, void *a2, void *a3, void *a4) { __model_plane(a0, a1, (BmfImage *)a2, (uint8_t *)a3, (uint8_t *)a4); }

int32_t __compress_image(char *a1, const __m128 &a2__ref, const __m128 &a3__ref, BmfImage *p_i, void *coded_buf)
{
  struct alignas(16) {   // 80 bytes, the frame Hex-Rays could not name
      char *Buffera_4;
      uint8_t _pad0[12];
      uint8_t slot16[4];
      uint8_t   hdr[16];   // the 16-byte archive member header `fwrite` sends in one call
      int32_t v65;
      char *v66;
      void *Buffer_2;
      uint8_t _pad1[32];
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 80, "frame layout moved");
  char *&Buffera_4 = __frame.Buffera_4;
  uint32_t &ElementCount = *(uint32_t *)((char *)__frame.slot16);
  int32_t &ElementCounta = *(int32_t *)((char *)__frame.slot16);
  uint32_t &ElementCountb = *(uint32_t *)((char *)__frame.slot16);
  uint32_t &Buffera = *(uint32_t *)&__frame.hdr[0];
  char *&Buffer_copy = *(char **)&__frame.hdr[0];   // the same slot, the other role
  int32_t &n4_2 = *(int32_t *)&__frame.hdr[4];
  char * &Buffera_1 = *(char * *)&__frame.hdr[8];
  int32_t &v64 = *(int32_t *)&__frame.hdr[12];
  int32_t &v65 = __frame.v65;
  char *&v66 = __frame.v66;
  void *&Buffer_2 = __frame.Buffer_2;
  ;
  char *v5;   // were int32_t: these hold addresses
  __m128 a2 = a2__ref;
  __m128 a3 = a3__ref;
  FILE *i;
  bool v38;
  char __compress_image_Buffer_1, *Buffera_5, v12, v13, v15, v17, *Buffera_6,
       v21, *Srca, v36, *Buffera_2, *Buffera_3;
  int32_t row_bytes, v11, v18, n4_6, bits_left, n4, v27, n8_1,
          acc, n4_1, v40, v41, v43, n4_3, v47, i_1, v50, n4_4, v56;
  BmfImage *p_i_1;
  uint16_t i_2, v53;
  uint32_t ElementCount_1, n7, v25, v26, v28, v30, v31, v32, Size, v55;
  uint8_t v39, *v49, v54;
  v5 = a1;
  if ( !((BmfArc *)a1)->fp )
    return 0;
  if ( !feof(((BmfArc *)a1)->fp) )
  {
    __fwd_compress_image_expand_image(v5, a2, a3, 1, nullptr);
    for ( i = ((BmfArc *)v5)->fp; i; i = ((BmfArc *)v5)->fp )
    {
      if ( feof(((BmfArc *)v5)->fp) )
        break;
      if ( feof(i) )
        break;
      __fwd_compress_image_expand_image(v5, a2, a3, 1, nullptr);
    }
  }
  __compress_image_Buffer_1 = (char)(uintptr_t)coded_buf;
  p_i_1 = (BmfImage *)(p_i);
  row_bytes = *((uint32_t *)p_i + 1);
  if ( coded_buf )
    __compress_image_Buffer_1 = 1;
  Buffera = *(uint32_t *)&p_i->width;
  *((uint8_t *)p_i + 11) |= __compress_image_Buffer_1 << 7;
  Buffera_5 = *((char **)p_i + 2);
  v11 = *((uint32_t *)p_i + 3);
  n4_2 = row_bytes;
  plane_desc[0].w4 = 512;
  v12 = *((uint8_t *)p_i + 10);
  plane_desc[0].w12 = 0;
  Buffera_1 = Buffera_5;
  v64 = v11;
  ::plane_count = ((v12 & 0x3Fu) + 7) >> 3;
  if ( fwrite("\x81\x8A""20\x81\x90""20a+b", 4u, 1u, ((BmfArc *)v5)->fp) != 1 )
    return 0;
  v15 = *((uint8_t *)p_i + 10);
  ++*(uint32_t *)v5;
  ElementCount_1 = v15 & 0x80;
  if ( v15 < 0 )
    ElementCount_1 = 3 << (v15 & 31);
  if ( *((uint32_t *)p_i + 3) < 0x10u )   // -N is on, so only the size decides
    goto LABEL_76;
  desc_slow_mode = 1;               // -S
  v17 = *((uint8_t *)p_i + 10);
  HIBYTE(Buffera_1) |= 0x24;        // -S in bit 2, and bit 5 always set
  if ( (v17 & 0x3Fu) <= 4 )         // -F is on, so only the depth decides
  {
    coded_size = *((uint32_t *)p_i + 3) + 0x20000;
    ::coded_buf = (uint8_t *)bmf_new(coded_size);
    ::packer_free_bits = 0;
    ::packer_acc = 0;
    out_cursor = ::coded_buf;
    ::packer_word = (uint32_t *)::coded_buf;
    hist_scratch = ::coded_buf + coded_size - 4096;
    plane_predictor = 0;
    plane_alt_model = 0;
    __dword_443388 = 0;
    // always taken: -S
      __fwd_compress_image_model_plane(a2, a3, p_i, (uint8_t *)p_i + 16, (uint8_t *)p_i + 16);
    goto LABEL_57;
  }
  ElementCount = __fwd_compress_image_search_filter(p_i, v13, a2, a3);
  HIBYTE(Buffera_1) |= 0x10u;
  if ( (*((uint8_t *)p_i + 11) & 2) != 0 )
  {
    n4_6 = *((uint32_t *)p_i + 1);
    Buffera_6 = *((char **)p_i + 2);
    Buffera = *(uint32_t *)&p_i->width;
    v18 = *((uint32_t *)p_i + 3);
    n4_2 = n4_6;
    Buffera_1 = Buffera_6;
    v64 = v18;
    HIBYTE(Buffera_1) = 0x34 | HIBYTE(Buffera_6);   // -S in bit 2
  }
  else
  {
    v18 = *((uint32_t *)p_i + 3);
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
      n7 = 0;
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
  __dword_443388 = 0;
  if ( ::plane_count > 0 )
  {
    bits_left = ::packer_free_bits;
    v66 = v5;
    n4 = 0;
    do
    {
      v25 = (4 * (uint8_t)plane_desc[n4 + 1].b2) | (uint8_t)plane_desc[n4 + 1].b0;
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
        ::packer_acc |= v25 << (-(char)bits_left & 31);
        bits_left = ::packer_free_bits - 6;
      }
      ::packer_free_bits = bits_left;
      if ( (plane_desc[n4 + 1].b2 & 8) != 0 )
      {
        v26 = (uint8_t)plane_desc[n4 + 1].b3;
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
          v27 = v26 << (-(char)bits_left & 31);
          bits_left -= 8;
          ::packer_acc |= v27;
        }
        ::packer_free_bits = bits_left;
        if ( (uint8_t)plane_desc[n4 + 1].b0 > 1u )
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
            ::packer_acc |= v28 << (-(char)bits_left & 31);
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
            v31 = v30 << (-(char)n8_1 & 31);
            bits_left = n8_1 - 8;
            ::packer_acc |= v31;
          }
          ::packer_free_bits = bits_left;
          if ( (uint8_t)plane_desc[n4 + 1].b0 > 2u )
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
              ::packer_acc |= v32 << (-(char)bits_left & 31);
              bits_left = ::packer_free_bits - 8;
              ::packer_free_bits -= 8;
            }
          }
        }
      }
      ++n4;
    }
    while ( n4 < ::plane_count );
    v5 = v66;
  }
  if ( ElementCount )
  {
    Size = p_i->width * p_i->height;
    HIBYTE(Buffera_1) |= 8u;
    Srca = (char *)bmf_new(Size);
    if ( ::plane_count > 0 )
    {
      v66 = v5;
      n4_1 = 0;
      do
        __fwd_compress_image_model_planes((char *)p_i, Srca, (uint8_t)plane_desc[n4_1++ + 1].b1, v36, a2, a3);
      while ( n4_1 < ::plane_count );
      v5 = v66;
    }
    free(Srca);
  }
  else
  {
    __fwd_compress_image_transform_planes(p_i, (int32_t)p_i, v21, a2, a3);
  }
LABEL_57:
  *(uint32_t *)::packer_word = ::packer_acc;
  v38 = (uint32_t)(out_cursor - (uint32_t)::coded_buf) < *((uint32_t *)p_i + 3);
  ElementCounta = out_cursor - ::coded_buf;
  v64 = out_cursor - ::coded_buf;
  if ( v38 )
  {
    v39 = fwrite(__frame.hdr, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
    if ( coded_buf )
      v39 &= fwrite(coded_buf, 1u, *((uint32_t *)coded_buf + 1) + 8, ((BmfArc *)v5)->fp) == *((uint32_t *)coded_buf + 1) + 8;
    v40 = (fwrite(::coded_buf, 1u, ElementCounta, ((BmfArc *)v5)->fp) == ElementCounta) & v39;
    free(::coded_buf);
    if ( v40 && (p_i->depth & 0x80) != 0 )
      fwrite((char *)p_i + *((uint32_t *)p_i + 3) + 16, 1u, ElementCount_1, ((BmfArc *)v5)->fp);
    fflush(((BmfArc *)v5)->fp);
    if ( v40 )
      return v64;
    return v40;
  }
  free(::coded_buf);
  if ( (*((uint8_t *)p_i + 11) & 2) != 0 )
  {
    Buffer_copy = (char *)bmf_new(*((uint32_t *)p_i + 3));
    v41 = p_i->height;
    n4_2 = ::plane_count;
    Buffera_1 = Buffer_copy;
    v64 = ::plane_count * (v41 - 1);
    Buffer_2 = (uint16_t *)p_i + 8;
    memcpy(Buffer_copy,(char *)p_i + 16,*((uint32_t *)p_i + 3));
    LOWORD(v43) = p_i->height;
    if ( (uint16_t)v43 )
    {
      n4_3 = n4_2;
      ElementCountb = ElementCount_1;
      Buffera_2 = Buffera_1;
      Buffera_3 = nullptr;
      v47 = 0;
      v66 = v5;
      do
      {
        i_1 = p_i_1->width;
        v65 = v47;
        Buffera_1 = Buffera_3;
        v49 = (uint8_t *)p_i_1 + v47 + 16;
        v50 = v64;
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
        v47 = n4_3 + v65;
        Buffera_3 = Buffera_1 + 1;
      }
      while ( (int32_t)(Buffera_1 + 1) < v43 );
      ElementCount_1 = ElementCountb;
      v5 = v66;
    }
    Buffera_4 = Buffer_copy;
    i_2 = p_i_1->width;
    v53 = v43 * n4_2;
    p_i_1->width = v43;
    p_i_1->height = i_2;
    *((uint8_t *)p_i_1 + 11) ^= 2u;
    p_i_1->stride = v53;
    free(Buffera_4);
    goto LABEL_77;
  }
LABEL_76:
  Buffer_2 = (uint16_t *)p_i + 8;
LABEL_77:
  v54 = fwrite(p_i_1, 1u, 0x10u, ((BmfArc *)v5)->fp) == 16;
  if ( coded_buf )
    v54 &= fwrite(coded_buf, 1u, *((uint32_t *)coded_buf + 1) + 8, ((BmfArc *)v5)->fp) == *((uint32_t *)coded_buf + 1) + 8;
  v55 = fwrite(Buffer_2, 1u, ElementCount_1 + *((uint32_t *)p_i_1 + 3), ((BmfArc *)v5)->fp);
  v56 = *((uint32_t *)p_i_1 + 3);
  if ( (v54 & (v55 == v56 + ElementCount_1)) == 0 )
    return 0;
  return v56;
}
static inline int32_t * __fwd_bmf_read_bmp(void *a0) { return __read_bmp((char *)a0); }
static inline BmfArc * __fwd_bmf_bmf_open_archive(void *a0, void *a1, int32_t a2) { return __bmf_open_archive((BmfArc *)a0, (char *)a1, a2); }
static inline int32_t __fwd_bmf_compress_image(char *a0, const __m128 &a1, const __m128 &a2, void *a3, void *a4) { return __compress_image(a0, a1, a2, (BmfImage *)a3, (void *)a4); }
static inline char * __fwd_bmf_expand_image(char *a0, const __m128 &a1, const __m128 &a2, int32_t a3, void *a4) { return __expand_image(a0, a1, a2, a3, (int32_t *)a4); }
static inline int32_t __fwd_bmf_write_bmp(int32_t a0, void *a1, int32_t a2) { return __write_bmp(a0, (char *)a1, a2); }
static inline BmfArc * __fwd_bmf_bmf_destroy_archive(void *a0, char a1) { return __bmf_destroy_archive((BmfArc *)a0, a1); }

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
void __bmf_compress(const __m128 &a1__ref, const __m128 &a2__ref,
                            const char *InName, const char *OutName)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
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
  p_i = __fwd_bmf_read_bmp((void *)InName);
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
    Arc = (int32_t)__fwd_bmf_bmf_open_archive((BmfArc *)__nb, (void *)OutName, 0);
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

  Size = __fwd_bmf_compress_image((char *)Arc, a1, a2, (uint16_t *)p_i, (void *)__dwLowDateTime);
  if ( !Size )
    __exit_402E40(5, OutName);
  printf(
    "%6.3f bpp\n",
    (double)Size * 8.0 / (double)(p_i_img->height * p_i_img->width));
  free(p_i);
}

// Expand the BMF stream InName into a BMP named OutName.
void __bmf_decompress(const __m128 &a1__ref, const __m128 &a2__ref,
                              const char *InName, const char *OutName)
{
  ;
  __m128 a1 = a1__ref;
  __m128 a2 = a2__ref;
  int32_t Number, Depth;
  uint32_t *p_i;
  void *Block;

  if ( void *__nb = bmf_new(sizeof(BmfArc)) )
    Block = (void *)__fwd_bmf_bmf_open_archive((BmfArc *)__nb, (void *)InName, 1);
  else
    Block = nullptr;
  printf("File %16s,\r", InName);
  Number = 0;
  while ( 1 )
  {
    p_i = (uint32_t *)__fwd_bmf_expand_image((char *)Block, a1, a2, 0, (void *)&__dwLowDateTime);
    BmfImage *const p_i_img = (BmfImage *)p_i;
    if ( !p_i )
    {
      printf("\n");
      if ( !*((uint32_t *)Block + 1) )
        __exit_402E40(3, InName);
      __fwd_bmf_bmf_destroy_archive((FILE **)Block, 1);
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
    if ( !__fwd_bmf_write_bmp((int32_t)p_i, (void *)OutName, 1) )
      __exit_402E40(5, OutName);
    free((void *)(uintptr_t)__dwLowDateTime);
    __dwLowDateTime = 0;
    free(p_i);
  }
}

int32_t __main(int32_t argc, const char **argv)
{
  ;
  __m128 v3, v4;
  int32_t Mode;

  bmf_set_denormal_mode();
  __set_new_handler((int32_t)__out_of_memory_handler);
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
    __bmf_compress(v3, v4, argv[2], argv[3]);
  else
    __bmf_decompress(v3, v4, argv[2], argv[3]);
  return 0;
}
void __out_of_memory_handler() { __exit_402E40(7); }
int32_t main(int32_t argc, char **argv) {
  return __main(argc, (const char **)argv);
}
