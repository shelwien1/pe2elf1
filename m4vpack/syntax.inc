// syntax.inc -- MPEG-4 Part 2 video syntax, from m4vdec.cpp (a port of FFmpeg's decoder): the bit
// reader and writer, the VLC tables, the stream state the syntax depends on (VOL, user data),
// and the records (VOP headers, video packet headers, macroblocks), each one walk that reads it
// from the bits or writes it back.

static inline int sign_extend(int val, int bits) { int sh = U32_BITS - bits; return (int)((U32)val << sh) >> sh; }
static int bitlen(int v) { U32 a = v < 0 ? 0u - (U32)v : (U32)v; int n = 0; while (a) { n++; a >>= 1; } return n; }
static inline int stuffing_bits(int nb) { return (1 << (nb - 1)) - 1; }  // nb bits of the usual stuffing: 0, then 1s

//--- bit reader (from m4vdec; buffers carry 64 bytes of zero padding) -------------------------

typedef struct GB { const U8* buf; int size; int pos; } GB;  // size, pos in bits

static inline U32 gb_show(const GB* g, int n) {  // 0 <= n <= 32
  const U8* p = g->buf + (g->pos >> BYTE_SHIFT);
  U64 v = 0;
  int i;
  for (i = 0; i < U64_BYTES; i++) v = v << BYTE_BITS | p[i];  // (the next 8 bytes, big endian)
  v <<= (g->pos & (BYTE_BITS - 1));
  return n ? (U32)(v >> (U64_BITS - n)) : 0;
}
static inline U32 gb_get(GB* g, int n) { U32 v = gb_show(g, n); g->pos += n; return v; }
static inline int gb_get1(GB* g) { return (int)gb_get(g, 1); }
static inline void gb_skip(GB* g, int n) { g->pos += n; }
static inline int gb_left(const GB* g) { return g->size - g->pos; }
static inline void gb_align(GB* g) { g->pos += to_byte(g->pos); }
static inline int gb_sget(GB* g, int n) { return sign_extend((int)gb_get(g, n), n); }
static inline int gb_xbits(GB* g, int n) {  // FFmpeg get_xbits: MSB set -> positive, else negative
  int v = (int)gb_get(g, n);
  return (v >> (n - 1)) ? v : v - (1 << n) + 1;
}

//--- bit writer ------------------------------------------------------------------------------

// n bits pending in acc; a full buffer goes to flush, where there is one (else what doesn't fit is
// an error), done counting the bytes it took
typedef struct PB {
  U8* buf;
  size_t cap, len;
  U64 acc;
  int n, err;
  void (*flush)(void* ctx, const U8* p, size_t n);
  void* ctx;
  U64 done;
} PB;

static void pb_init(PB* p, U8* buf, size_t cap) { memset(p, 0, sizeof(*p)); p->buf = buf; p->cap = cap; }
static void pb_put(PB* p, U32 v, int n) {  // 0 <= n <= 32
  if (n <= 0) return;
  p->acc = (p->acc << n) | (n < U32_BITS ? v & ((1u << n) - 1) : v);
  for (p->n += n; p->n >= BYTE_BITS;) {
    p->n -= BYTE_BITS;
    if (p->len == p->cap && p->flush) { p->flush(p->ctx, p->buf, p->len); p->done += p->len; p->len = 0; }
    if (p->len < p->cap) p->buf[p->len++] = (U8)(p->acc >> p->n);
    else p->err = 1;
  }
}
static void pb_finish(PB* p) {  // the bytes in the buffer to flush
  if (p->flush && p->len) { p->flush(p->ctx, p->buf, p->len); p->done += p->len; p->len = 0; }
}
static void pb_xbits(PB* p, int v, int n) { pb_put(p, (U32)(v > 0 ? v : v + (1 << n) - 1), n); }

//--- VLC (from m4vdec) -------------------------------------------------------------------------

// A VLC both ways: for the reader a lookup table of 1 << bits entries, by a code's first bits (its
// symbol and length; 0: no code starts so), for the writer each of its n symbols' code and length
// (0: none)
typedef struct VLCE { S16 sym; S16 len; } VLCE;
constexpr int VLC_SYMS_MAX = 128;  // a VLC's symbols, at most (the RL tables have the most)
typedef struct VLC { VLCE* tab; int bits, n; U32 code[VLC_SYMS_MAX]; U8 len[VLC_SYMS_MAX]; } VLC;
// a VLC with its static lookup table
#define VLC_DEF(name, nbits) static VLCE name##_tab[1 << (nbits)]; static VLC name = { name##_tab, nbits, 0, {0}, {0} }

static void vlc_build(VLC* v, int n, const U32* codes, const U8* lens) {
  const int bits = v->bits;
  int i;
  v->n = n;
  for (i = 0; i < n; i++) {
    U32 j, first, cnt;
    v->code[i] = codes[i];
    v->len[i] = lens[i];
    if (!lens[i]) continue;
    first = codes[i] << (bits - lens[i]);
    cnt = 1u << (bits - lens[i]);
    for (j = 0; j < cnt; j++) { v->tab[first + j].sym = (S16)i; v->tab[first + j].len = lens[i]; }
  }
}

static inline int vlc_get(GB* g, const VLC* v) {
  const VLCE* e = &v->tab[gb_show(g, v->bits)];
  if (!e->len) return -1;
  g->pos += e->len;
  return e->sym;
}

// a table's entries; the longest of its codes (a VLC's lookup width: the lengths l[], or the
// second column of t[][2])
template <class T, size_t N>
constexpr int countof(const T (&)[N]) { return (int)N; }
constexpr int max2(int a, int b) { return a > b ? a : b; }
template <size_t N>
constexpr int max_len(const U8 (&l)[N], size_t i = 0) { return i == N ? 0 : max2(l[i], max_len(l, i + 1)); }
template <class T, size_t N>
constexpr int max_len2(const T (&t)[N][2], size_t i = 0) { return i == N ? 0 : max2(t[i][1], max_len2(t, i + 1)); }

//--- the syntax's fields (ISO/IEC 14496-2 6.2): their bits, and the values the parser checks ------

// the picture's size in macroblocks: the VOL's width and height are 13-bit fields
constexpr int VOL_SIZE_BITS = 13;
constexpr int MAX_MB_W = (1 << VOL_SIZE_BITS) / MB_W, MAX_MBS = MAX_MB_W * MAX_MB_W;
// the syntax's ranges: quantiser_scale, vop_quant (quant_precision bits, 3 to 9), the time
// increment's bits, intra_dc_vlc_thr, a sprite trajectory's component (14 bits), a vector
// difference's magnitude in VLC units (mv_tab), a B-VOP's dquant (+-DQ_B); and the parser's limit
// on the runs it counts (modulo_time_base's 1s, stuffing codes)
constexpr int QS_MIN = 1, QS_MAX = 31;
constexpr int QUANT_PREC_MIN = 3, QUANT_PREC_MAX = 9, QUANT_PREC_DEFAULT = 5;
constexpr int QUANT_MAX = (1 << QUANT_PREC_MAX) - 1;
constexpr int TIB_MIN = 1, TIB_MAX = 16;
constexpr int DCTHR_MAX = 7;
constexpr int WARP_MAX = (1 << 14) - 1;
constexpr int MV_VLC_MAX = 32;
constexpr int MV_WRAP_BITS = 5;  // + f_code: the bits a vector component wraps around at (as decoders add)
// a vector component as the decoder adds it up, wrapped (f_code fcode)
static int mv_wrap(int v, int fcode) { return sign_extend(v, MV_WRAP_BITS + fcode); }
constexpr int DQ_B = 2;
constexpr int RUN_MAX = 1000;
// MCBPC: the chroma blocks' bits (2), dquant (intra: in the I-VOP table, else in the P-VOP one),
// intra (in the P-VOP table), 4MV; CBPY: inverted in inter macroblocks
enum { MCBPC_CHROMA = 3, MCBPC_IQUANT = 4, MCBPC_INTRA = 4, MCBPC_DQUANT = 8, MCBPC_4MV = 16 };
constexpr int CBP_CHROMA_BITS = MB_BLOCKS - MB_LUMA;
constexpr int CBPY_INTER = (1 << MB_LUMA) - 1;
constexpr int DQUANT_BITS = 2;                         // dquant: its index into quant_tab
// a coefficient's escape modes: plain VLC, the level offset, the run offset, fixed length (mode 3:
// last, run, marker, level, marker); the escape code is a symbol of the VLC, then the mode's code
// (esc_tab)
enum { ESC_VLC, ESC_LEVEL, ESC_RUN, ESC_FIXED };
constexpr int SIGN_BITS = 1, MARKER_BITS = 1;
constexpr int ESC3_RUN = 6, ESC3_LEVEL = 12;
constexpr int ESC3_LEN = 1 + ESC3_RUN + MARKER_BITS + ESC3_LEVEL + MARKER_BITS;  // (after the mode's code)
constexpr int ESC3_MAX = 1 << (ESC3_LEVEL - 1);        // a level's magnitude in mode 3, at most
// intra DC: the sizes 8-bit video has, at most, and those with a marker after the difference
constexpr int DC_SIZE_MAX = 9, DC_MARKER_SIZE = 8;
// the VOP header: vop_coding_type, intra_dc_vlc_thr, vop_fcode
constexpr int VOP_TYPE_BITS = 2, DCTHR_BITS = 3, FCODE_BITS = 3;
// the time increment's length, before a VOL says (FFmpeg's), and FFmpeg's guess of it from what
// follows it in a VOP header (marker, vop_coded, ...): the bits shown, masked, as they should be
constexpr int TIB_DEFAULT = 4;
constexpr int TIB_GUESS_P_BITS = 6, TIB_GUESS_P_MASK = 0x37, TIB_GUESS_P = 0x30;
constexpr int TIB_GUESS_BITS = 5, TIB_GUESS_MASK = 0x1F, TIB_GUESS = 0x18;
// resync markers: at least RESYNC_ZEROS 0s (+ f_code - 1 in P- and S-VOPs, + the larger f_code
// - 1 in B-VOPs, at least RESYNC_B_MIN), then a 1; FFmpeg's look ahead for one: RESYNC_WIN bits,
// zeros counted to ZEROS_MAX, RESYNC_TAIL bits after the macroblock number; and the least a video
// packet's header takes
constexpr int RESYNC_ZEROS = 16, RESYNC_B_MIN = 2, RESYNC_WIN = 16, ZEROS_MAX = 32, RESYNC_TAIL = 6;
constexpr int VP_HDR_MIN = 20;
// a macroblock record's vectors, at most (4MV; a B-VOP's two fields both ways)
constexpr int MB_MVS = 4;
// the VOL header's fields
constexpr int VO_TYPE_BITS = 8, VO_TYPE_STUDIO = 14, VO_TYPE_STUDIO2 = 15;  // video_object_type_indication
constexpr int VERID_BITS = 4, PRIORITY_BITS = 3;
constexpr int ASPECT_BITS = 4, ASPECT_EXTENDED = 15, PAR_BITS = 2 * BYTE_BITS;
constexpr int CHROMA_LOWDELAY_BITS = 3;
constexpr int VBV_BITS = 15 + 1 + 15 + 1 + 15 + 1 + 3 + 11 + 1 + 15 + 1;  // vbv_parameters, with markers
constexpr int SHAPE_BITS = 2;
constexpr int TIME_RES_BITS = 16;
constexpr int SPRITE_BITS = 2;                    // sprite_enable (1 bit at verid 1)
constexpr int SPRITE_DIM_BITS = 4 * (VOL_SIZE_BITS + MARKER_BITS);  // a static sprite's size and place
constexpr int WARP_POINTS_BITS = 6, WARP_POINTS_MAX = 3, WARP_ACC_BITS = 2;
constexpr int QUANT_PREC_BITS = 4, BPP_BITS = 4;
constexpr int MATRIX_BITS = BYTE_BITS;            // a quantiser matrix's entries
constexpr int CPLX_METHOD_BITS = 2, CPLX_SHAPE_FLAGS = 6;
constexpr int DCECS_BITS = 8, DCECS_SHORT_BITS = 4;  // a VOP header's complexity estimates
constexpr int CPLX_BITS_MAX = 264;                // their bits in a VOP header (a VOL asks for 172 at most)
constexpr int NEWPRED_BITS = 3;                   // requested_upstream_message_type, newpred_segment_type
constexpr int HIERARCHY_BITS = 1 + 4 + 1, SAMPLING_BITS = 5;  // scalability
constexpr int VOL_TAIL_MIN = 4;                   // the bits of the flags after the matrices, at least
// user data: the bytes read, and up to what (a start code: 23 0s); a libavcodec version as a build;
// the build "ffmpeg" stands for; a DivX build with bugs of its own
constexpr int USER_DATA_MAX = 255, SC_ZEROS = SC_PREFIX * BYTE_BITS - 1;
static inline int lavc_version(int a, int b, int c) {
  return ((a & BYTE_MASK) << 2 * BYTE_BITS) + ((b & BYTE_MASK) << BYTE_BITS) + (c & BYTE_MASK);
}
constexpr int LAVC_BUILD_FFMPEG = 4600;
constexpr int DIVX413_VERSION = 500, DIVX413_BUILD = 413;
// the GOP header's time code
constexpr int HOURS_BITS = 5, MINUTES_BITS = 6, SECONDS_BITS = 6, MINUTE_SECS = 60, HOUR_MINUTES = 60;
// the bytes of a header chunk the parser reads
constexpr size_t HDR_PEEK = 32 * KIB;

//--- tables (from FFmpeg via m4vdec: h263data.c, mpeg4data.h) ----------------------------------

static constexpr U8 intra_mcbpc_code[] = { 1, 1, 2, 3, 1, 1, 2, 3, 1 };
static constexpr U8 intra_mcbpc_bits[] = { 1, 3, 3, 3, 4, 6, 6, 6, 9 };
static constexpr U8 inter_mcbpc_code[] = { 1, 3, 2, 5,  3, 4, 3, 3,  3, 7, 6, 5,  4, 4, 3, 2,
                                           2, 5, 4, 5,  1, 0, 0, 0,  2, 12, 14, 15 };
static constexpr U8 inter_mcbpc_bits[] = { 1, 4, 4, 6,  5, 8, 8, 7,  3, 7, 7, 9,  6, 9, 9, 9,
                                           3, 7, 7, 8,  9, 0, 0, 0,  11, 13, 13, 13 };
// the MCBPC stuffing codes' entries; their length, and the not_coded bit before one in a P-VOP:
// FFmpeg's 8 + pict_type (I 9 bits, P 10; it reads an S-VOP's as 12, which m4vpack keeps as bytes)
enum { MCBPC_STUFF_I = 8, MCBPC_STUFF_P = 20 };
constexpr int STUFF_ZEROS = 8;
constexpr int stuffing_len(int type) { return STUFF_ZEROS + type; }
static constexpr U8 cbpy_tab[][2] = {
  {  3, 4 }, { 5, 5 }, { 4, 5 }, { 9, 4 }, {  3, 5 }, { 7, 4 }, { 2, 6 },
  { 11, 4 }, { 2, 5 }, { 3, 6 }, { 5, 4 }, { 10, 4 }, { 4, 4 }, { 8, 4 },
  {  6, 4 }, { 3, 2 } };
static constexpr U8 mv_tab[][2] = {
  {  1,  1 }, {  1,  2 }, {  1,  3 }, {  1,  4 }, {  3,  6 }, {  5,  7 },
  {  4,  7 }, {  3,  7 }, { 11,  9 }, { 10,  9 }, {  9,  9 }, { 17, 10 },
  { 16, 10 }, { 15, 10 }, { 14, 10 }, { 13, 10 }, { 12, 10 }, { 11, 10 },
  { 10, 10 }, {  9, 10 }, {  8, 10 }, {  7, 10 }, {  6, 10 }, {  5, 10 },
  {  4, 10 }, {  7, 11 }, {  6, 11 }, {  5, 11 }, {  4, 11 }, {  3, 11 },
  {  2, 11 }, {  3, 12 }, {  2, 12 } };
static constexpr U16 inter_vlc[][2] = {
  {  0x2,  2 }, {  0xf,  4 }, { 0x15,  6 }, { 0x17,  7 }, { 0x1f,  8 }, { 0x25,  9 }, { 0x24,  9 }, { 0x21, 10 },
  { 0x20, 10 }, {  0x7, 11 }, {  0x6, 11 }, { 0x20, 11 }, {  0x6,  3 }, { 0x14,  6 }, { 0x1e,  8 }, {  0xf, 10 },
  { 0x21, 11 }, { 0x50, 12 }, {  0xe,  4 }, { 0x1d,  8 }, {  0xe, 10 }, { 0x51, 12 }, {  0xd,  5 }, { 0x23,  9 },
  {  0xd, 10 }, {  0xc,  5 }, { 0x22,  9 }, { 0x52, 12 }, {  0xb,  5 }, {  0xc, 10 }, { 0x53, 12 }, { 0x13,  6 },
  {  0xb, 10 }, { 0x54, 12 }, { 0x12,  6 }, {  0xa, 10 }, { 0x11,  6 }, {  0x9, 10 }, { 0x10,  6 }, {  0x8, 10 },
  { 0x16,  7 }, { 0x55, 12 }, { 0x15,  7 }, { 0x14,  7 }, { 0x1c,  8 }, { 0x1b,  8 }, { 0x21,  9 }, { 0x20,  9 },
  { 0x1f,  9 }, { 0x1e,  9 }, { 0x1d,  9 }, { 0x1c,  9 }, { 0x1b,  9 }, { 0x1a,  9 }, { 0x22, 11 }, { 0x23, 11 },
  { 0x56, 12 }, { 0x57, 12 }, {  0x7,  4 }, { 0x19,  9 }, {  0x5, 11 }, {  0xf,  6 }, {  0x4, 11 }, {  0xe,  6 },
  {  0xd,  6 }, {  0xc,  6 }, { 0x13,  7 }, { 0x12,  7 }, { 0x11,  7 }, { 0x10,  7 }, { 0x1a,  8 }, { 0x19,  8 },
  { 0x18,  8 }, { 0x17,  8 }, { 0x16,  8 }, { 0x15,  8 }, { 0x14,  8 }, { 0x13,  8 }, { 0x18,  9 }, { 0x17,  9 },
  { 0x16,  9 }, { 0x15,  9 }, { 0x14,  9 }, { 0x13,  9 }, { 0x12,  9 }, { 0x11,  9 }, {  0x7, 10 }, {  0x6, 10 },
  {  0x5, 10 }, {  0x4, 10 }, { 0x24, 11 }, { 0x25, 11 }, { 0x26, 11 }, { 0x27, 11 }, { 0x58, 12 }, { 0x59, 12 },
  { 0x5a, 12 }, { 0x5b, 12 }, { 0x5c, 12 }, { 0x5d, 12 }, { 0x5e, 12 }, { 0x5f, 12 }, {  0x3,  7 } };
static constexpr S8 inter_level[] = {
  1,  2,  3,  4, 5, 6, 7, 8,  9, 10, 11, 12, 1, 2, 3, 4,  5,  6,  1,  2, 3, 4, 1, 2,  3,  1,  2,  3, 1, 2, 3, 1,
  2,  3,  1,  2, 1, 2, 1, 2,  1,  2,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  2, 3, 1, 2, 1,
  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,
  1,  1,  1,  1, 1, 1 };
static constexpr S8 inter_run[] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,
   3,  4,  4,  4,  5,  5,  5,  6,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,  0,  0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40 };
static constexpr U16 intra_vlc[][2] = {
  { 0x2, 2 }, { 0x6, 3 }, { 0xf, 4 }, { 0xd, 5 }, { 0xc, 5 }, { 0x15, 6 }, { 0x13, 6 }, { 0x12, 6 },
  { 0x17, 7 }, { 0x1f, 8 }, { 0x1e, 8 }, { 0x1d, 8 }, { 0x25, 9 }, { 0x24, 9 }, { 0x23, 9 }, { 0x21, 9 },
  { 0x21, 10 }, { 0x20, 10 }, { 0xf, 10 }, { 0xe, 10 }, { 0x7, 11 }, { 0x6, 11 }, { 0x20, 11 }, { 0x21, 11 },
  { 0x50, 12 }, { 0x51, 12 }, { 0x52, 12 }, { 0xe, 4 }, { 0x14, 6 }, { 0x16, 7 }, { 0x1c, 8 }, { 0x20, 9 },
  { 0x1f, 9 }, { 0xd, 10 }, { 0x22, 11 }, { 0x53, 12 }, { 0x55, 12 }, { 0xb, 5 }, { 0x15, 7 }, { 0x1e, 9 },
  { 0xc, 10 }, { 0x56, 12 }, { 0x11, 6 }, { 0x1b, 8 }, { 0x1d, 9 }, { 0xb, 10 }, { 0x10, 6 }, { 0x22, 9 },
  { 0xa, 10 }, { 0xd, 6 }, { 0x1c, 9 }, { 0x8, 10 }, { 0x12, 7 }, { 0x1b, 9 }, { 0x54, 12 }, { 0x14, 7 },
  { 0x1a, 9 }, { 0x57, 12 }, { 0x19, 8 }, { 0x9, 10 }, { 0x18, 8 }, { 0x23, 11 }, { 0x17, 8 }, { 0x19, 9 },
  { 0x18, 9 }, { 0x7, 10 }, { 0x58, 12 }, { 0x7, 4 }, { 0xc, 6 }, { 0x16, 8 }, { 0x17, 9 }, { 0x6, 10 },
  { 0x5, 11 }, { 0x4, 11 }, { 0x59, 12 }, { 0xf, 6 }, { 0x16, 9 }, { 0x5, 10 }, { 0xe, 6 }, { 0x4, 10 },
  { 0x11, 7 }, { 0x24, 11 }, { 0x10, 7 }, { 0x25, 11 }, { 0x13, 7 }, { 0x5a, 12 }, { 0x15, 8 }, { 0x5b, 12 },
  { 0x14, 8 }, { 0x13, 8 }, { 0x1a, 8 }, { 0x15, 9 }, { 0x14, 9 }, { 0x13, 9 }, { 0x12, 9 }, { 0x11, 9 },
  { 0x26, 11 }, { 0x27, 11 }, { 0x5c, 12 }, { 0x5d, 12 }, { 0x5e, 12 }, { 0x5f, 12 }, { 0x3, 7 } };
static constexpr S8 intra_level[] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  1,  2,  3,  4,  5,  1,  2,  3,  4,  1,  2,
   3,  1,  2,  3,  1,  2,  3,  1,  2,  3,  1,  2,  1,  2,  1,  1,  1,  1,  1,  1,  2,  3,  4,  5,
   6,  7,  8,  1,  2,  3,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1 };
static constexpr S8 intra_run[] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,
   4,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  9,  9, 10, 11, 12, 13, 14,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20 };
static constexpr U8 DCtab_lum[][2] = {
  {3,3}, {3,2}, {2,2}, {2,3}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, {1,10}, {1,11} };
static constexpr U8 DCtab_chrom[][2] = {
  {3,2}, {2,2}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, {1,10}, {1,11}, {1,12} };
static constexpr U8 sprite_trajectory_lens[] = { 2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static constexpr U8 mb_type_b_tab[][2] = { {1, 1}, {1, 2}, {1, 3}, {1, 4} };
static constexpr U8 dc_threshold[DCTHR_MAX + 1] = { 99, 13, 15, 17, 19, 21, 23, 0 };
static constexpr U8 zigzag_direct[BLK_COEFS] = {
   0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };
static constexpr U8 alternate_vertical_scan[BLK_COEFS] = {
   0,  8, 16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
  41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
  51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
  53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63 };
static constexpr U8 alternate_horizontal_scan[BLK_COEFS] = {
   0,  1,  2,  3,  8,  9, 16, 17, 10, 11,  4,  5,  6,  7, 15, 14,
  13, 12, 19, 18, 24, 25, 32, 33, 26, 27, 20, 21, 22, 23, 28, 29,
  30, 31, 34, 35, 40, 41, 48, 49, 42, 43, 36, 37, 38, 39, 44, 45,
  46, 47, 50, 51, 56, 57, 58, 59, 52, 53, 54, 55, 60, 61, 62, 63 };
static constexpr S16 default_intra_matrix[BLK_COEFS] = {
   8, 17, 18, 19, 21, 23, 25, 27, 17, 18, 19, 21, 23, 25, 27, 28,
  20, 21, 22, 23, 24, 26, 28, 30, 21, 22, 23, 24, 26, 28, 30, 32,
  22, 23, 24, 26, 28, 30, 32, 35, 23, 24, 26, 28, 30, 32, 35, 38,
  25, 26, 28, 30, 32, 35, 38, 41, 27, 28, 30, 32, 35, 38, 41, 45 };
static constexpr S16 default_non_intra_matrix[BLK_COEFS] = {
  16, 17, 18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 24,
  18, 19, 20, 21, 22, 23, 24, 25, 19, 20, 21, 22, 23, 24, 26, 27,
  20, 21, 22, 23, 25, 26, 27, 28, 21, 22, 23, 24, 26, 27, 28, 30,
  22, 23, 24, 26, 27, 28, 30, 31, 23, 24, 25, 27, 28, 30, 31, 33 };
static constexpr S8 quant_tab[] = { -1, -2, 1, 2 };  // dquant's values, by its 2 bits
// a B-VOP's dbquant: its codes (0, 10, 11), their values
static constexpr U8 dbquant_tab[][2] = { {0, 1}, {2, 2}, {3, 2} };
static constexpr S8 dbquant_dq[] = { 0, -DQ_B, DQ_B };
// the escape modes' codes after the escape code: ESC_LEVEL 0, ESC_RUN 10, ESC_FIXED 11
static constexpr U8 esc_tab[][2] = { {0, 0}, {0, 1}, {2, 2}, {3, 2} };

// FFmpeg's RLTable: the symbols (n of them; then the escape code), the first with last = 1 (last)
constexpr int RL_MAX_RUN = 64, RL_MAX_LEVEL = 64;  // (FFmpeg's MAX_RUN, MAX_LEVEL)
typedef struct RL {
  int n, last;
  const U16 (*vlc)[2];
  const S8* run;
  const S8* level;
  U8 max_level[2][RL_MAX_RUN + 1], max_run[2][RL_MAX_LEVEL + 1];
  VLC v;
  S16 index[2][RL_MAX_RUN][RL_MAX_LEVEL + 1];  // [last][run][level] -> symbol, -1 if there is none
} RL;
constexpr int RL_INTRA_LAST = 67, RL_INTER_LAST = 58;
constexpr int RL_VLC_BITS = max2(max_len2(intra_vlc), max_len2(inter_vlc));
static_assert(countof(intra_vlc) == countof(intra_run) + 1 && countof(inter_vlc) == countof(inter_run) + 1 &&
              countof(intra_vlc) == countof(inter_vlc), "the RL tables: their symbols and the escape code");
static_assert(countof(intra_vlc) <= VLC_SYMS_MAX, "a VLC's symbols");

static VLCE rl_intra_tab[1 << RL_VLC_BITS], rl_inter_tab[1 << RL_VLC_BITS];
static RL rl_intra = { countof(intra_run), RL_INTRA_LAST, intra_vlc, intra_run, intra_level, {{0}}, {{0}},
                       { rl_intra_tab, RL_VLC_BITS, 0, {0}, {0} }, {{{0}}} };
static RL rl_inter = { countof(inter_run), RL_INTER_LAST, inter_vlc, inter_run, inter_level, {{0}}, {{0}},
                       { rl_inter_tab, RL_VLC_BITS, 0, {0}, {0} }, {{{0}}} };
VLC_DEF(vlc_intra_mcbpc, max_len(intra_mcbpc_bits));
VLC_DEF(vlc_inter_mcbpc, max_len(inter_mcbpc_bits));
VLC_DEF(vlc_cbpy, max_len2(cbpy_tab));
VLC_DEF(vlc_mv, max_len2(mv_tab));
VLC_DEF(vlc_dc_lum, max_len2(DCtab_lum));
VLC_DEF(vlc_dc_chrom, max_len2(DCtab_chrom));
VLC_DEF(vlc_mb_type_b, max_len2(mb_type_b_tab));
VLC_DEF(vlc_sprite_traj, max_len(sprite_trajectory_lens));
VLC_DEF(vlc_dbquant, max_len2(dbquant_tab));
VLC_DEF(vlc_esc, max_len2(esc_tab));

static void rl_init(RL* rl) {
  int last, i;
  U32 codes[countof(intra_vlc)]; U8 lens[countof(intra_vlc)];
  memset(rl->max_level, 0, sizeof(rl->max_level));
  memset(rl->max_run, 0, sizeof(rl->max_run));
  memset(rl->index, -1, sizeof(rl->index));
  for (last = 0; last < 2; last++) {
    int start = last ? rl->last : 0, end = last ? rl->n : rl->last;
    for (i = start; i < end; i++) {
      int run = rl->run[i], level = rl->level[i];
      if (level > rl->max_level[last][run]) rl->max_level[last][run] = (U8)level;
      if (run > rl->max_run[last][level]) rl->max_run[last][level] = (U8)run;
      rl->index[last][run][level] = (S16)i;
    }
  }
  for (i = 0; i <= rl->n; i++) { codes[i] = rl->vlc[i][0]; lens[i] = (U8)rl->vlc[i][1]; }
  vlc_build(&rl->v, rl->n + 1, codes, lens);
}

// a VLC from a table of codes and one of their lengths
template <size_t N>
static void vlc_from(VLC* v, const U8 (&code)[N], const U8 (&len)[N]) {
  U32 c[N];
  U8 l[N];
  size_t i;
  for (i = 0; i < N; i++) { c[i] = code[i]; l[i] = len[i]; }
  vlc_build(v, (int)N, c, l);
}
// ... and from a table of (code, length) pairs
template <size_t N>
static void vlc_from2(VLC* v, const U8 (&t)[N][2]) {
  U32 c[N];
  U8 l[N];
  size_t i;
  for (i = 0; i < N; i++) { c[i] = t[i][0]; l[i] = t[i][1]; }
  vlc_build(v, (int)N, c, l);
}

static int tables_init(void) {  // (the VLC and RL tables above)
  constexpr int NTRAJ = countof(sprite_trajectory_lens);
  U32 c[NTRAJ]; U8 l[NTRAJ]; int i;
  U32 code = 0;
  vlc_from(&vlc_intra_mcbpc, intra_mcbpc_code, intra_mcbpc_bits);
  vlc_from(&vlc_inter_mcbpc, inter_mcbpc_code, inter_mcbpc_bits);
  vlc_from2(&vlc_cbpy, cbpy_tab);
  vlc_from2(&vlc_mv, mv_tab);
  vlc_from2(&vlc_dc_lum, DCtab_lum);
  vlc_from2(&vlc_dc_chrom, DCtab_chrom);
  vlc_from2(&vlc_mb_type_b, mb_type_b_tab);
  vlc_from2(&vlc_dbquant, dbquant_tab);
  vlc_from2(&vlc_esc, esc_tab);
  for (i = 0; i < NTRAJ; i++) {  // canonical codes from lengths, in listed order (as ff_vlc_init_from_lengths)
    l[i] = sprite_trajectory_lens[i];
    c[i] = code >> (U32_BITS - l[i]);
    code += 1u << (U32_BITS - l[i]);
  }
  vlc_build(&vlc_sprite_traj, NTRAJ, c, l);
  rl_init(&rl_intra);
  rl_init(&rl_inter);
  return 1;
}
static const int tables_made = tables_init();  // (as the program starts, as cm.inc's tables)

// Length of an event (last, run, |level|) coded in an escape mode (ESC_VLC: plain VLC), 0 if that
// mode can't code it.
static int esc_bits(const RL* rl, int mode, int last, int run, int alevel) {
  const int esc = rl->vlc[rl->n][1];  // (the escape code's length)
  int s, k;
  if (run < 0 || run > BLK_LAST) return 0;
  switch (mode) {
  case ESC_VLC:
    if (alevel < 1 || alevel > RL_MAX_LEVEL || (s = rl->index[last][run][alevel]) < 0) return 0;
    return rl->vlc[s][1] + SIGN_BITS;
  case ESC_LEVEL:
    k = alevel - rl->max_level[last][run];
    if (k < 1 || k > RL_MAX_LEVEL || (s = rl->index[last][run][k]) < 0) return 0;
    return esc + esc_tab[ESC_LEVEL][1] + rl->vlc[s][1] + SIGN_BITS;
  case ESC_RUN:
    if (alevel < 1 || alevel > RL_MAX_LEVEL) return 0;
    k = run - rl->max_run[last][alevel] - 1;
    if (k < 0 || (s = rl->index[last][k][alevel]) < 0) return 0;
    return esc + esc_tab[ESC_RUN][1] + rl->vlc[s][1] + SIGN_BITS;
  default:
    return alevel <= ESC3_MAX ? esc + esc_tab[ESC_FIXED][1] + ESC3_LEN : 0;
  }
}

// default escape mode: plain VLC when possible, else the first escape mode that can code it
static int esc_default(const RL* rl, int last, int run, int alevel) {
  int m;
  for (m = ESC_VLC; m < ESC_FIXED; m++)
    if (esc_bits(rl, m, last, run, alevel)) return m;
  return ESC_FIXED;
}

//--- stream context ----------------------------------------------------------------------------

enum { PT_I = 1, PT_P = 2, PT_B = 3, PT_S = 4, PT_N };  // same values as AVPictureType (FFmpeg relies on them)
// the start code at p (SC_LEN bytes): its code, -1 if there is none
static inline int sc_code(const U8* p) { return !p[0] && !p[1] && p[2] == SC_START ? p[SC_PREFIX] : -1; }
// a VOP's type (PT_*) from its vop_coding_type, the top bits of the byte after the start code at p
constexpr int VOP_TYPE_SHIFT = 6;
static inline int vop_type(const U8* p) { return PT_I + (p[SC_LEN] >> VOP_TYPE_SHIFT); }
enum { STATIC_SPRITE = 1, GMC_SPRITE = 2 };

typedef struct Ctx {
  int have_vol;
  const char* unsupported;   // why VOPs of the current VOL are kept raw
  int dp;                    // data partitioning: only B-VOPs are supported
  int width, height, mb_w, mb_h, mb_num, mb_num_bits;
  int interlaced, sprite_usage, warp_points, quant_precision, time_inc_bits;
  int cplx_i, cplx_p, cplx_b;
  int divx_version, divx_build, xvid_build;
  int time_res;              // vop_time_increment_resolution
  int fixed_inc;             // fixed_vop_time_increment (0: none, the VOPs' times vary)
  int qpel;                  // quarter_sample
  // what the reconstruction (recon.inc) needs beyond the syntax: the VOL's quantiser matrices
  // (natural order) and warping accuracy, the libavcodec build of the user data (its bugs), how many
  // VOL and GOP headers came (a VOL resets the field times, a GOP the time base: gop_time, seconds)
  int vo_type, mpeg_quant, warp_acc, lavc_build, vol_count, gop_count, gop_time;
  U16 intra_matrix[BLK_COEFS], inter_matrix[BLK_COEFS];
  // c: skip flags of the last anchor VOP and of the one being parsed (a B-VOP macroblock whose
  // co-located macroblock in the backward reference was skipped has no bits), skip_cap entries
  U8 *skip_ref, *skip_cur;
  size_t skip_cap;
  U8 hbuf[HDR_PEEK + GB_PAD];  // a header chunk being parsed
} Ctx;

static void ctx_init(Ctx* c) {  // (the skip flags stay: ctx_quit() frees them)
  U8 *r = c->skip_ref, *u = c->skip_cur;
  const size_t cap = c->skip_cap;
  memset(c, 0, sizeof(*c));
  c->skip_ref = r;
  c->skip_cur = u;
  c->skip_cap = cap;
  c->divx_version = c->divx_build = c->xvid_build = c->lavc_build = -1;
  c->time_inc_bits = TIB_DEFAULT;
  c->quant_precision = QUANT_PREC_DEFAULT;
}
static void ctx_quit(Ctx* c) {
  free(c->skip_ref);
  free(c->skip_cur);
  c->skip_ref = c->skip_cur = 0;
  c->skip_cap = 0;
}
// room for the skip flags of n macroblocks and one more (0: no memory)
static int ctx_skip_room(Ctx* c, int n) {
  size_t r = c->skip_cap, u = c->skip_cap;
  if ((size_t)++n <= c->skip_cap) return 1;
  if (!grow(&c->skip_ref, &r, n) || !grow(&c->skip_cur, &u, n)) return 0;
  memset(c->skip_ref + c->skip_cap, 0, r - c->skip_cap);
  memset(c->skip_cur + c->skip_cap, 0, u - c->skip_cap);
  c->skip_cap = r;
  return 1;
}

#include "parse_vol.inc"  // parse_vol(): the VOL header

// user data: encoder identification (m4vdec's decode_user_data, the parts the syntax depends on)
static void parse_user_data(Ctx* c, GB* gb) {
  char buf[USER_DATA_MAX + 1], last;
  int i, e, ver = 0, build = 0;
  for (i = 0; i < USER_DATA_MAX && gb->pos < gb->size; i++) {
    if (gb_show(gb, SC_ZEROS) == 0) break;
    buf[i] = (char)gb_get(gb, BYTE_BITS);
  }
  buf[i] = 0;
  e = sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
  if (e < 2) e = sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
  if (e >= 2) { c->divx_version = ver; c->divx_build = build; }
  {  // libavcodec's build (m4vdec's decode_user_data)
    int v2 = 0, v3 = 0, lb = 0;
    constexpr int ALL = 4;  // (the fields of the longest format, matched: sscanf()'s count, made up to it)
    e = sscanf(buf, "FFmpe%*[^b]b%d", &lb) + ALL - 1;
    if (e != ALL) e = sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &v2, &v3, &lb);
    if (e != ALL) {
      e = sscanf(buf, "Lavc%d.%d.%d", &ver, &v2, &v3) + 1;
      if (e > 1) lb = lavc_version(ver, v2, v3);
    }
    if (e != ALL) { if (strcmp(buf, "ffmpeg") == 0) c->lavc_build = LAVC_BUILD_FFMPEG; }
    if (e == ALL) c->lavc_build = lb;
  }
  if (sscanf(buf, "XviD%d", &build) == 1) c->xvid_build = build;
}

// GOP header: its time code resets the time base (m4vdec's decode_gop_header)
static void parse_gop(Ctx* c, GB* gb) {
  int hours, minutes, seconds;
  if (!gb_show(gb, SC_ZEROS)) return;
  hours = (int)gb_get(gb, HOURS_BITS);
  minutes = (int)gb_get(gb, MINUTES_BITS);
  gb_skip(gb, 1);
  seconds = (int)gb_get(gb, SECONDS_BITS);
  c->gop_time = seconds + MINUTE_SECS * (minutes + HOUR_MINUTES * hours);
  c->gop_count++;
}

// a chunk (the first 32 KiB of it) that isn't a VOP: VOL headers and user data matter
static void parse_header_chunk(Ctx* c, const U8* p, size_t n) {
  GB gb;
  int code;
  if (n < SC_LEN || (code = sc_code(p)) < 0) return;
  if (n > HDR_PEEK) n = HDR_PEEK;
  memcpy(c->hbuf, p, n);
  memset(c->hbuf + n, 0, GB_PAD);
  gb.buf = c->hbuf + SC_LEN;
  gb.size = (int)(n - SC_LEN) * BYTE_BITS;
  gb.pos = 0;
  if (sc_is_vol(code)) parse_vol(c, &gb);
  else if (code == SC_USER_DATA) parse_user_data(c, &gb);
  else if (code == SC_GOV) parse_gop(c, &gb);
}

static int divx413(const Ctx* c) {
  return c->divx_version == DIVX413_VERSION && c->divx_build == DIVX413_BUILD && c->xvid_build < 0;
}

//--- records ------------------------------------------------------------------------------------

typedef struct VopHdr {
  int type, mtb, tib, tinc, coded, rnd, dcthr, tff, alt, quant, fcode, bcode;
  int warp[2 * WARP_POINTS_MAX];
  int ncplx; char cplx[CPLX_BITS_MAX];
} VopHdr;

typedef struct VpHdr { int st, mbnum, quant, hec, mtb, tinc, type, dcthr, warp[2 * WARP_POINTS_MAX], fcode, bcode; } VpHdr;

typedef struct Blk {
  int dc;          // intra DC difference (when the DC is coded separately)
  int n;           // events, 0 = not coded
  U8 run[BLK_COEFS], esc[BLK_COEFS];
  S16 level[BLK_COEFS];
} Blk;

typedef struct MB {
  int x, y, st, sk, mb, t, mc, ac, dq, dt, fp;
  int nfs, fs;     // field selects: count, bits (first one in the highest bit)
  int nmv, mv[2 * MB_MVS];
  Blk b[MB_BLOCKS];
} MB;

typedef struct Vop {  // a VOP's state as a walk goes through it (the quantiser follows dquant)
  int type, gmc, fcode, bcode, qscale, dcthr, tib;
} Vop;
// a macroblock record's codes (m4vtsv's): t, its type (P-VOP, S-VOP: MB_INTER, MB_4MV, MB_INTRA;
// B-VOP, mb_type's VLC: MB_DIRECT, MB_BI, MB_BWD, MB_FWD), and mb, a B-VOP's modb (MODB_NONE:
// direct without a vector or coefficients, MODB_NOCBP: no coded block pattern, MODB_CBP)
enum { MB_INTER, MB_4MV, MB_INTRA };
enum { MB_DIRECT, MB_BI, MB_BWD, MB_FWD };
enum { MODB_NONE, MODB_NOCBP, MODB_CBP };
static int fwd_dir(int t) { return t == MB_BI || t == MB_FWD; }   // B-VOP macroblock types using the forward / backward reference
static int bwd_dir(int t) { return t == MB_BI || t == MB_BWD; }

// a record's optional fields absent (NA): where one read or decoded starts
static void vop_hdr_clear(VopHdr* h) {
  h->rnd = h->dcthr = h->tff = h->alt = h->quant = h->fcode = h->bcode = NA;
  h->ncplx = 0;
}
static void vp_clear(VpHdr* k) { k->mtb = k->tinc = k->type = k->dcthr = k->fcode = k->bcode = NA; }
static void mb_clear(MB* m) {
  m->st = m->sk = 0;
  m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
  m->nfs = m->fs = m->nmv = 0;
}

// Which fields a record has, for the walks below and the model alike: a VOP's global motion
// compensation (an S-VOP of a GMC sprite), its vop_rounding_type (P-VOPs, GMC S-VOPs), its
// complexity estimates' bits (as the VOL asks); a macroblock's field_prediction (interlaced, an
// inter macroblock with a vector of its own: not direct, not GMC) and the field selects after it,
// whether it says its dquant (I-, P- and S-VOPs: in MCBPC; a B-VOP's dbquant: a macroblock not
// direct, with coded blocks), and its dct_type (interlaced: intra, or with coded blocks)
static int vop_gmc(const Ctx* c, int type) { return type == PT_S && c->sprite_usage == GMC_SPRITE; }
static int vop_has_rnd(const Ctx* c, int type) { return type == PT_P || vop_gmc(c, type); }
static int vop_ncplx(const Ctx* c, int type) {
  return c->cplx_i + (type != PT_I ? c->cplx_p : 0) + (type == PT_B ? c->cplx_b : 0);
}
static int mb_has_fp(const Ctx* c, const Vop* v, const MB* m) {
  return c->interlaced && (v->type == PT_B ? m->t != MB_DIRECT : m->t == MB_INTER && m->mc != 1);
}
static int mb_nfs(const Vop* v, const MB* m) { return v->type == PT_B ? 2 * (fwd_dir(m->t) + bwd_dir(m->t)) : 2; }
static int mb_has_dq(const Vop* v, const MB* m, int cbp) { return v->type != PT_B || (m->t != MB_DIRECT && cbp); }
static int mb_has_dt(const Ctx* c, int intra, int cbp) { return c->interlaced && (intra || cbp); }

static int cbp_of(const MB* m) {  // coded flags, block 0 in the top bit (of MB_BLOCKS)
  int i, cbp = 0;
  for (i = 0; i < MB_BLOCKS; i++) cbp = cbp * 2 + (m->b[i].n > 0);
  return cbp;
}

// the place of v in t (-1: none)
template <size_t N>
static int index_of(const S8 (&t)[N], int v) {
  int i;
  for (i = 0; i < (int)N; i++)
    if (t[i] == v) return i;
  return -1;
}

static int prefix_length(const Vop* v) {  // zeros of a resync marker
  switch (v->type) {
  case PT_I: return RESYNC_ZEROS;
  case PT_P: case PT_S: return RESYNC_ZEROS + v->fcode - 1;
  default: { int m = v->fcode > v->bcode ? v->fcode : v->bcode; return RESYNC_ZEROS + max2(m, RESYNC_B_MIN) - 1; }
  }
}

// an MCBPC stuffing code before a macroblock (in a P- or S-VOP after a not_coded 0): its bits, and
// their value; FFmpeg's look-ahead for a resync marker skips stuffing_len()'s, the same but in an
// S-VOP
constexpr int mcbpc_stuff_len(int type) {
  return type == PT_I ? intra_mcbpc_bits[MCBPC_STUFF_I] : 1 + inter_mcbpc_bits[MCBPC_STUFF_P];
}
constexpr U32 mcbpc_stuff_code(int type) {
  return type == PT_I ? intra_mcbpc_code[MCBPC_STUFF_I] : inter_mcbpc_code[MCBPC_STUFF_P];
}
static_assert(mcbpc_stuff_len(PT_I) == stuffing_len(PT_I) && mcbpc_stuff_len(PT_P) == stuffing_len(PT_P),
              "MCBPC stuffing: its codes as FFmpeg's look-ahead skips them");

static int mv_ok(int d, int f_code) {  // a vector difference the VLC can code
  return d >= -(MV_VLC_MAX << (f_code - 1)) && d <= MV_VLC_MAX << (f_code - 1);
}

enum { PB_MV, PB_DC, PB_COEF, NPB };
static thread_local double pbits[NPB];  // statistics: bits of motion vectors, intra DCs, coefficients read

//--- the records' bits, both ways ----------------------------------------------------------------

// Each record's syntax is one walk for both directions: vop_hdr_io(), vp_io(), mb_io() and the parts
// of a macroblock take Rd, which reads the fields from the bits into the record, or Wr, which writes
// the record's fields as bits. The walk says which fields there are, in what order, of what length,
// and what they may be; an element's two directions are side by side in Rd and Wr. An element
// returns 0 where its field can't be (the reader at a code that isn't one, the writer at a value the
// field can't hold: it gets records decoded from data that may be damaged, and checks what the reader
// does), and a walk returns 0 where its record can't be. What a walk works out before an element (a
// code, a length) is the writer's: the reader's element overwrites it.

struct Rd {  // the bits read into a record
  static constexpr bool W = false;
  GB* g;
  int pos() const { return g->pos; }
  int over() const { return g->pos > g->size; }      // past the end (where the walks stop)
  int short_of(int n) const { return gb_left(g) < n; }  // fewer than n bits left
  void count(int k, int p0) const { pbits[k] += g->pos - p0; }
  int bits(int* v, int n) { *v = (int)gb_get(g, n); return 1; }
  int bit(int* v) { return bits(v, 1); }
  int sbits(int* v, int n) { *v = gb_sget(g, n); return 1; }   // two's complement
  int xbits(int* v, int n) { *v = gb_xbits(g, n); return 1; }  // FFmpeg's get_xbits
  int marker() { return gb_get1(g); }
  int vlc(int* s, const VLC* v) { return (*s = vlc_get(g, v)) >= 0; }
  int ones(int* n) {  // 1s, then a 0 (modulo_time_base)
    for (*n = 0; gb_get1(g);)
      if (++*n > RUN_MAX || gb_left(g) <= 0) return 0;
    return 1;
  }
  int stuffing(int* n, int type) {  // MCBPC stuffing codes, counted
    for (*n = 0; gb_show(g, mcbpc_stuff_len(type)) == mcbpc_stuff_code(type);) {
      gb_skip(g, mcbpc_stuff_len(type));
      if (++*n > RUN_MAX) return 0;
    }
    return 1;
  }
  void byte_stuffing() { gb_skip(g, 1); gb_align(g); }  // (a 0, then 1s up to the byte boundary)
  // vop_time_increment's length: the VOL's, or where it has none or what follows doesn't fit it,
  // FFmpeg's guess from the header's next fields (P-VOPs and GMC S-VOPs: vop_rounding_type among
  // them), kept in c
  int time_bits(Ctx* c, int type, int* tib) {
    if (c->time_inc_bits == 0 || !(gb_show(g, c->time_inc_bits + 1) & 1)) {
      for (c->time_inc_bits = TIB_MIN; c->time_inc_bits < TIB_MAX; c->time_inc_bits++) {
        if (vop_has_rnd(c, type)) {
          if ((gb_show(g, c->time_inc_bits + TIB_GUESS_P_BITS) & TIB_GUESS_P_MASK) == TIB_GUESS_P) break;
        } else if ((gb_show(g, c->time_inc_bits + TIB_GUESS_BITS) & TIB_GUESS_MASK) == TIB_GUESS)
          break;
      }
    }
    *tib = c->time_inc_bits;
    return 1;
  }
};

struct Wr {  // a record written as bits
  static constexpr bool W = true;
  PB* p;
  int pos() const { return 0; }
  int over() const { return 0; }
  int short_of(int) const { return 0; }
  void count(int, int) const {}
  int bits(int* v, int n) {
    if (n < U32_BITS && (U32)*v >> n) return 0;
    pb_put(p, (U32)*v, n);
    return 1;
  }
  int bit(int* v) { return bits(v, 1); }
  int sbits(int* v, int n) {
    if (*v < -(1 << (n - 1)) || *v >= 1 << (n - 1)) return 0;
    pb_put(p, (U32)*v & ((1u << n) - 1), n);
    return 1;
  }
  int xbits(int* v, int n) {  // (n: the value's length)
    if (bitlen(*v) != n) return 0;
    pb_xbits(p, *v, n);
    return 1;
  }
  int marker() { pb_put(p, 1, MARKER_BITS); return 1; }
  int vlc(int* s, const VLC* v) {
    if (*s < 0 || *s >= v->n || !v->len[*s]) return 0;
    pb_put(p, v->code[*s], v->len[*s]);
    return 1;
  }
  int ones(int* n) {
    int i;
    if (*n < 0 || *n > RUN_MAX) return 0;
    for (i = 0; i < *n; i++) pb_put(p, 1, 1);
    pb_put(p, 0, 1);
    return 1;
  }
  int stuffing(int* n, int type) {
    int i;
    if (*n < 0 || *n > RUN_MAX) return 0;
    for (i = 0; i < *n; i++) pb_put(p, mcbpc_stuff_code(type), mcbpc_stuff_len(type));
    return 1;
  }
  void byte_stuffing() {
    pb_put(p, 0, 1);
    while (p->n) pb_put(p, 1, 1);
  }
  int time_bits(Ctx*, int, int*) { return 1; }  // (the record's)
};

// a vector difference (FFmpeg's h263 decode_motion, without the prediction): its magnitude's code
// (vlc_mv), a sign and f_code - 1 low bits
template <class IO>
static int mv_io(IO& io, int* d, int f_code) {
  const int shift = f_code - 1;
  int code = 0, neg = 0, lo = 0;
  if (IO::W) {
    if (!mv_ok(*d, f_code)) return 0;
    if (*d) {
      const int a = (*d < 0 ? -*d : *d) - 1;
      code = (a >> shift) + 1;
      neg = *d < 0;
      lo = a & ((1 << shift) - 1);
    }
  }
  if (!io.vlc(&code, &vlc_mv)) return 0;
  if (!code) {
    *d = 0;
    return 1;
  }
  if (!io.bit(&neg) || !io.bits(&lo, shift)) return 0;
  *d = ((code - 1) << shift | lo) + 1;
  if (neg) *d = -*d;
  return 1;
}

// n vectors' differences (x, y), into m->mv from *k on
template <class IO>
static int mvs_io(IO& io, MB* m, int* k, int n, int f_code) {
  const int p0 = io.pos();
  int i;
  for (i = 0; i < 2 * n; i++, ++*k)
    if (!mv_io(io, &m->mv[*k], f_code)) return 0;
  io.count(PB_MV, p0);
  return 1;
}

// a sprite trajectory (GMC): each warping point's du, dv as a length's code, the value (FFmpeg's
// xbits) and a marker (DivX 4.13 leaves out a point's first one)
template <class IO>
static int traj_io(IO& io, const Ctx* c, int* w) {
  int i;
  for (i = 0; i < 2 * c->warp_points; i++) {
    int len = IO::W ? bitlen(w[i]) : 0;
    if (!io.vlc(&len, &vlc_sprite_traj)) return 0;
    if (!len) w[i] = 0;
    else if (!io.xbits(&w[i], len)) return 0;
    if (((i & 1) || !divx413(c)) && !io.marker()) return 0;
  }
  return 1;
}

// A coefficient event (last, run, level): a symbol of rl's VLC and the sign, or the escape code, the
// mode's code (esc_tab) and the event as the mode has it: ESC_LEVEL, a symbol of the level less the
// run's largest; ESC_RUN, a symbol of the run less the level's longest, less 1 (each with the sign);
// ESC_FIXED, last, run, a marker, the level, a marker
template <class IO>
static int event_io(IO& io, const RL* rl, int* last, int* run, int* level, int* mode) {
  int sym = rl->n, s, neg = *level < 0, r, l;
  if (IO::W) {
    const int al = neg ? -*level : *level;
    if (!esc_bits(rl, *mode, *last, *run, al)) return 0;  // (one its mode can't code)
    if (*mode == ESC_VLC) sym = rl->index[*last][*run][al];
    else if (*mode == ESC_LEVEL) sym = rl->index[*last][*run][al - rl->max_level[*last][*run]];
    else if (*mode == ESC_RUN) sym = rl->index[*last][*run - rl->max_run[*last][al] - 1][al];
  }
  s = IO::W && *mode == ESC_VLC ? sym : rl->n;
  if (!io.vlc(&s, &rl->v)) return 0;
  if (s != rl->n) {
    *mode = ESC_VLC;
    sym = s;
  } else {
    if (!io.vlc(mode, &vlc_esc)) return 0;
    if (*mode == ESC_FIXED)
      return io.bit(last) && io.bits(run, ESC3_RUN) && io.marker() && io.sbits(level, ESC3_LEVEL) && io.marker();
    if (!io.vlc(&sym, &rl->v) || sym == rl->n) return 0;
  }
  *last = sym >= rl->last;
  r = rl->run[sym];
  l = rl->level[sym];
  if (*mode == ESC_LEVEL) l += rl->max_level[*last][r];
  else if (*mode == ESC_RUN) r += rl->max_run[*last][l] + 1;
  if (!io.bit(&neg)) return 0;
  *run = r;
  *level = neg ? -l : l;
  return 1;
}

// A block (n: 0-3 luma, 4-5 chroma): an intra DC coded apart (dcvlc) as its size's code, the
// difference and a marker after a long one; then, if coded, its events up to the last.
template <class IO>
static int block_io(IO& io, Blk* b, int n, int coded, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i = -1, j, p0 = io.pos();
  if (!IO::W) b->n = b->dc = 0;
  if (intra && dcvlc) {
    int size = bitlen(b->dc);
    if (!io.vlc(&size, n < MB_LUMA ? &vlc_dc_lum : &vlc_dc_chrom) || size > DC_SIZE_MAX) return 0;
    if (size && (!io.xbits(&b->dc, size) || (size > DC_MARKER_SIZE && !io.marker()))) return 0;
    i = 0;
  }
  io.count(PB_DC, p0);
  p0 = io.pos();
  if (!coded) return 1;
  for (j = 0;; j++) {  // (i, the coefficient's position, keeps j within the block)
    int last = 0, run = 0, level = 0, mode = ESC_VLC;
    if (io.over()) return 0;
    if (IO::W) {
      last = j + 1 == b->n;
      run = b->run[j];
      level = b->level[j];
      mode = b->esc[j];
    }
    if (!event_io(io, rl, &last, &run, &level, &mode)) return 0;
    i += run + 1;
    if (i > (last ? BLK_LAST : BLK_LAST - 1)) return 0;
    b->run[j] = (U8)run;
    b->level[j] = (S16)level;
    b->esc[j] = (U8)mode;
    if (last) {
      b->n = j + 1;
      io.count(PB_COEF, p0);
      return 1;
    }
  }
}

template <class IO>
static int blocks_io(IO& io, MB* m, int cbp, int intra, int dcvlc) {
  int i;
  for (i = 0; i < MB_BLOCKS; i++, cbp += cbp)
    if (!block_io(io, &m->b[i], i, cbp & 1 << (MB_BLOCKS - 1), intra, dcvlc)) return 0;
  return 1;
}

// dquant (I-, P- and S-VOPs: 2 bits, an index into quant_tab) or a B-VOP's dbquant (a code of
// dbquant_tab); the quantiser in v follows it
template <class IO>
static int dquant_io(IO& io, Vop* v, MB* m) {
  const int b = v->type == PT_B;
  int s = IO::W ? (b ? index_of(dbquant_dq, m->dq) : index_of(quant_tab, m->dq)) : 0;
  if (b ? !io.vlc(&s, &vlc_dbquant) : !io.bits(&s, DQUANT_BITS)) return 0;
  m->dq = b ? dbquant_dq[s] : quant_tab[s];
  v->qscale = clip(v->qscale + m->dq, QS_MIN, QS_MAX);
  return 1;
}

// an intra macroblock after its MCBPC (cbpc; dquant: whether it says one): ac_pred_flag, cbpy,
// dquant, dct_type, the blocks (the DCs coded apart below the VOP's threshold, as the quantiser was)
template <class IO>
static int intra_io(IO& io, const Ctx* c, Vop* v, MB* m, int cbpc, int dquant) {
  const int dcvlc = v->qscale < v->dcthr;
  int cbpy = IO::W ? cbp_of(m) >> CBP_CHROMA_BITS : 0;
  if (!io.bit(&m->ac) || !io.vlc(&cbpy, &vlc_cbpy)) return 0;
  if (!dquant) m->dq = 0;
  else if (!dquant_io(io, v, m)) return 0;
  if (mb_has_dt(c, 1, 0) && !io.bit(&m->dt)) return 0;
  return blocks_io(io, m, (cbpc & MCBPC_CHROMA) | cbpy << CBP_CHROMA_BITS, 1, dcvlc);
}

// A macroblock (the idx-th; FFmpeg's mpeg4_decode_mb): MCBPC stuffing; in a P- or S-VOP not_coded;
// MCBPC (I-, P-, S-VOPs) or modb, mb_type and the coded pattern (B-VOPs), and the fields they say;
// the vectors; the blocks. The quantiser in v follows its dquant.
template <class IO>
static int mb_io(IO& io, const Ctx* c, Vop* v, MB* m, int idx) {
  const int cbp0 = IO::W ? cbp_of(m) : 0;  // (the writer's coded pattern)
  int cbpc = 0, cbpy = 0, cbp = cbp0, s = 0, nmv = 0;
  if (!IO::W) mb_clear(m);
  if (io.over()) return 0;
  if (v->type == PT_I) {
    if (IO::W) cbpc = (m->dq ? MCBPC_IQUANT : 0) | (cbp0 & MCBPC_CHROMA);
    if (!io.stuffing(&m->st, v->type) || !io.vlc(&cbpc, &vlc_intra_mcbpc)) return 0;
    m->t = MB_INTRA;
    return intra_io(io, c, v, m, cbpc, cbpc & MCBPC_IQUANT);
  }
  if (v->type != PT_B) {  // P- and S-VOPs
    if (!io.stuffing(&m->st, v->type) || !io.bit(&m->sk)) return 0;
    if (m->sk) return 1;
    if (IO::W)
      cbpc = (m->t == MB_INTRA ? MCBPC_INTRA : m->t == MB_4MV ? MCBPC_4MV : 0) | (m->dq ? MCBPC_DQUANT : 0) |
             (cbp0 & MCBPC_CHROMA);
    if (!io.vlc(&cbpc, &vlc_inter_mcbpc)) return 0;
    if (cbpc & MCBPC_INTRA) {
      m->t = MB_INTRA;
      return intra_io(io, c, v, m, cbpc, cbpc & MCBPC_DQUANT);
    }
    m->t = cbpc & MCBPC_4MV ? MB_4MV : MB_INTER;
    if (v->gmc && m->t == MB_INTER && !io.bit(&m->mc)) return 0;
    if (IO::W) cbpy = (cbp0 >> CBP_CHROMA_BITS) ^ CBPY_INTER;
    if (!io.vlc(&cbpy, &vlc_cbpy)) return 0;
    cbp = (cbpc & MCBPC_CHROMA) | (cbpy ^ CBPY_INTER) << CBP_CHROMA_BITS;
    if (!(cbpc & MCBPC_DQUANT)) m->dq = 0;
    else if (!dquant_io(io, v, m)) return 0;
  } else {  // B-VOPs: no bits where the co-located macroblock of the backward reference was skipped
    if (!IO::W) m->sk = c->skip_ref[idx];
    if (m->sk) return 1;
    if (IO::W) s = m->mb == MODB_NONE;
    if (!io.bit(&s)) return 0;
    if (s) {
      m->mb = MODB_NONE;
      m->t = MB_DIRECT;
      return blocks_io(io, m, 0, 0, 0);
    }
    if (IO::W) s = m->mb == MODB_NOCBP;
    if (!io.bit(&s)) return 0;
    m->mb = s ? MODB_NOCBP : MODB_CBP;
    if (!io.vlc(&m->t, &vlc_mb_type_b)) return 0;
    if (m->mb == MODB_NOCBP) cbp = 0;
    else if (!io.bits(&cbp, MB_BLOCKS)) return 0;
    if (mb_has_dq(v, m, cbp) && !dquant_io(io, v, m)) return 0;
  }
  if (mb_has_dt(c, 0, cbp) && !io.bit(&m->dt)) return 0;
  if (mb_has_fp(c, v, m) && !io.bit(&m->fp)) return 0;
  if (m->fp == 1) {
    m->nfs = mb_nfs(v, m);
    if (!io.bits(&m->fs, m->nfs)) return 0;
  }
  if (v->type != PT_B) {  // a vector, a field's each, or a block's each (4MV); none in GMC
    if (m->t == MB_4MV) {
      if (!mvs_io(io, m, &nmv, MB_LUMA, v->fcode)) return 0;
    } else if (m->mc != 1 && !mvs_io(io, m, &nmv, m->fp == 1 ? 2 : 1, v->fcode))
      return 0;
  } else if (m->t != MB_DIRECT) {  // forward, then backward
    const int n = m->fp == 1 ? 2 : 1;
    if (fwd_dir(m->t) && !mvs_io(io, m, &nmv, n, v->fcode)) return 0;
    if (bwd_dir(m->t) && !mvs_io(io, m, &nmv, n, v->bcode)) return 0;
  } else if (!mvs_io(io, m, &nmv, 1, FCODE_MIN))  // direct mode's delta vector
    return 0;
  m->nmv = nmv;
  return blocks_io(io, m, cbp, 0, 0);
}

// FFmpeg's mpeg4_is_resync on a copy of the reader: the number of the macroblock a resync marker
// right after the current position announces (the number of macroblocks for the end of the VOP),
// or 0. *nstuff: MCBPC stuffing codes FFmpeg skips first.
static int resync_ahead(const Ctx* c, const GB* gb0, const Vop* v, int* nstuff) {
  constexpr int B = BYTE_BITS - 1;  // (a bit's place in its byte: & B)
  GB g = *gb0, *gb = &g;
  int bits_count = gb->pos, w = (int)gb_show(gb, RESYNC_WIN);
  *nstuff = 0;
  while (w <= (int)BYTE_MASK) {  // (its first byte's bits 0)
    if (v->type == PT_B || (w >> (RESYNC_WIN - stuffing_len(v->type)) != 1)) break;
    gb_skip(gb, stuffing_len(v->type));
    bits_count += stuffing_len(v->type);
    w = (int)gb_show(gb, RESYNC_WIN);
    ++*nstuff;
  }
  if (bits_count + BYTE_BITS >= gb->size) {  // the end: the usual stuffing up to the byte boundary?
    w >>= BYTE_BITS;
    w |= stuffing_bits(BYTE_BITS) >> (B - (bits_count & B));
    if (w == stuffing_bits(BYTE_BITS)) return c->mb_num;
  } else {
    static const U16 resync_prefix[BYTE_BITS] = { 0x7F00, 0x7E00, 0x7C00, 0x7800, 0x7000, 0x6000, 0x4000, 0x0000 };
    if (w == resync_prefix[bits_count & B]) {
      int len, mb_num;
      gb_skip(gb, 1);
      gb_align(gb);
      for (len = 0; len < ZEROS_MAX; len++)
        if (gb_get1(gb)) break;
      mb_num = (int)gb_get(gb, c->mb_num_bits);
      if (!mb_num || mb_num > c->mb_num || gb->pos + RESYNC_TAIL > gb->size) mb_num = -1;
      if (len >= prefix_length(v)) return mb_num;
    }
  }
  return 0;
}

// A video packet's header after the macroblock before it (FFmpeg's
// mpeg4_decode_video_packet_header): MCBPC stuffing (in an S-VOP, as FFmpeg skips it, there's
// none), the stuffing up to the byte boundary, the resync marker, the macroblock's number, the
// quantiser (0: as it was) and header_extension_code, after which the VOP's times, type, DC
// threshold, trajectory and f_codes again. The quantiser in v follows it.
template <class IO>
static int vp_io(IO& io, const Ctx* c, Vop* v, VpHdr* k) {
  int mk = 1;
  if (!IO::W) vp_clear(k);
  if (v->type == PT_S ? k->st != 0 : !io.stuffing(&k->st, v->type)) return 0;
  io.byte_stuffing();
  if (io.short_of(VP_HDR_MIN)) return 0;
  if (!io.bits(&mk, prefix_length(v) + 1) || mk != 1) return 0;  // (its 0s, then a 1)
  if (!io.bits(&k->mbnum, c->mb_num_bits) || !k->mbnum || k->mbnum >= c->mb_num) return 0;
  if (!io.bits(&k->quant, c->quant_precision) || !io.bit(&k->hec)) return 0;
  if (k->quant) v->qscale = k->quant;
  v->qscale = clip(v->qscale, QS_MIN, QS_MAX);
  if (k->hec) {
    if (!io.ones(&k->mtb) || !io.marker() || !io.bits(&k->tinc, v->tib) || !io.marker()) return 0;
    if (!io.bits(&k->type, VOP_TYPE_BITS) || !io.bits(&k->dcthr, DCTHR_BITS)) return 0;
    if (v->gmc && !traj_io(io, c, k->warp)) return 0;
    if (v->type != PT_I && !io.bits(&k->fcode, FCODE_BITS)) return 0;
    if (v->type == PT_B && !io.bits(&k->bcode, FCODE_BITS)) return 0;
  }
  return !io.over();
}

// A VOP's header after its start code (FFmpeg's decode_vop_header): vop_coding_type,
// modulo_time_base, vop_time_increment, vop_coded; then the rounding type, the complexity
// estimates, intra_dc_vlc_thr, the field flags, the trajectory, the quantiser and the f_codes.
// Reading it keeps FFmpeg's guess of the time increment's length in c.
template <class IO>
static int vop_hdr_io(IO& io, Ctx* c, VopHdr* h) {
  int i, t = h->type - PT_I;
  if (!IO::W) vop_hdr_clear(h);
  if (!io.bits(&t, VOP_TYPE_BITS)) return 0;
  h->type = PT_I + t;
  if (!io.ones(&h->mtb) || !io.marker() || !io.time_bits(c, h->type, &h->tib)) return 0;
  if (!io.bits(&h->tinc, h->tib) || !io.marker() || !io.bit(&h->coded)) return 0;
  if (!h->coded) return 1;
  if (c->dp && h->type != PT_B) return 0;  // (data partitioning: B-VOPs only)
  if (vop_has_rnd(c, h->type) && !io.bit(&h->rnd)) return 0;
  h->ncplx = vop_ncplx(c, h->type);
  for (i = 0; i < h->ncplx; i++) {
    int b = h->cplx[i] - '0';
    if (!io.bit(&b)) return 0;
    h->cplx[i] = (char)('0' + b);
  }
  if (io.short_of(DCTHR_BITS) || !io.bits(&h->dcthr, DCTHR_BITS)) return 0;
  if (c->interlaced && (!io.bit(&h->tff) || !io.bit(&h->alt))) return 0;
  if (vop_gmc(c, h->type) && !traj_io(io, c, h->warp)) return 0;
  if (!io.bits(&h->quant, c->quant_precision) || !h->quant) return 0;
  if (h->type != PT_I && (!io.bits(&h->fcode, FCODE_BITS) || !h->fcode)) return 0;
  if (h->type == PT_B && (!io.bits(&h->bcode, FCODE_BITS) || !h->bcode)) return 0;
  return !io.over();
}
