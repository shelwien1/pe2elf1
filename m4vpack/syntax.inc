// syntax.inc -- MPEG-4 Part 2 video syntax, from m4vdec.cpp (a port of FFmpeg's decoder): the bit
// reader and writer, the VLC tables, the stream state the syntax depends on (VOL, user data),
// and each record parsed from the bits (VOP headers, macroblocks, video packet headers) and
// written back.

static inline int clipi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline int ilog2(U32 v) { int n = 0; while (v >>= 1) n++; return n; }
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

typedef struct VLCE { S16 sym; S16 len; } VLCE;
typedef struct VLC { VLCE* tab; int bits; } VLC;
// a VLC with its static (1 << bits)-entry lookup table
#define VLC_DEF(name, nbits) static VLCE name##_tab[1 << (nbits)]; static VLC name = { name##_tab, nbits }

static void vlc_build(VLC* v, int n, const U32* codes, const U8* lens) {
  const int bits = v->bits;
  int i;
  for (i = 0; i < n; i++) {
    U32 j, first, cnt;
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
constexpr int DQ_B = 2;
constexpr int RUN_MAX = 1000;
// MCBPC: the chroma blocks' bits (2), dquant (intra: in the I-VOP table, else in the P-VOP one),
// intra (in the P-VOP table), 4MV; CBPY: inverted in inter macroblocks
enum { MCBPC_CHROMA = 3, MCBPC_IQUANT = 4, MCBPC_INTRA = 4, MCBPC_DQUANT = 8, MCBPC_4MV = 16 };
constexpr int CBP_CHROMA_BITS = MB_BLOCKS - MB_LUMA;
constexpr int CBPY_INTER = (1 << MB_LUMA) - 1;
constexpr int DQUANT_BITS = 2;                         // dquant: its index into quant_tab
enum { DBQUANT_ZERO = 0, DBQUANT_NEG = 2, DBQUANT_POS = 3, DBQUANT_BITS = 2 };  // a B-VOP's dbquant: 0, 10, 11
// a coefficient's escape modes: plain VLC, the level offset, the run offset, fixed length (mode 3:
// last, run, marker, level, marker); the escape code is a symbol of the VLC, then the mode's bits
enum { ESC_VLC, ESC_LEVEL, ESC_RUN, ESC_FIXED, NESC };
constexpr U8 ESC_DEFAULT = UINT8_MAX;  // a record's escape mode left to esc_default()
enum { ESC_LEVEL_CODE = 0, ESC_LEVEL_BITS = 1, ESC_RUN_CODE = 2, ESC_FIXED_CODE = 3, ESC_MODE_BITS = 2 };
constexpr int SIGN_BITS = 1, MARKER_BITS = 1;
constexpr int ESC3_RUN = 6, ESC3_LEVEL = 12;
constexpr int ESC3_LEN = ESC_MODE_BITS + 1 + ESC3_RUN + MARKER_BITS + ESC3_LEVEL + MARKER_BITS;  // (after the code)
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
static inline int stuffing_len(int type) { return STUFF_ZEROS + type; }
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
static constexpr S8 quant_tab[] = { -1, -2, 1, 2 };
static U32 sprite_trajectory_code[countof(sprite_trajectory_lens)];

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

static VLCE rl_intra_tab[1 << RL_VLC_BITS], rl_inter_tab[1 << RL_VLC_BITS];
static RL rl_intra = { countof(intra_run), RL_INTRA_LAST, intra_vlc, intra_run, intra_level, {{0}}, {{0}},
                       { rl_intra_tab, RL_VLC_BITS }, {{{0}}} };
static RL rl_inter = { countof(inter_run), RL_INTER_LAST, inter_vlc, inter_run, inter_level, {{0}}, {{0}},
                       { rl_inter_tab, RL_VLC_BITS }, {{{0}}} };
VLC_DEF(vlc_intra_mcbpc, max_len(intra_mcbpc_bits));
VLC_DEF(vlc_inter_mcbpc, max_len(inter_mcbpc_bits));
VLC_DEF(vlc_cbpy, max_len2(cbpy_tab));
VLC_DEF(vlc_mv, max_len2(mv_tab));
VLC_DEF(vlc_dc_lum, max_len2(DCtab_lum));
VLC_DEF(vlc_dc_chrom, max_len2(DCtab_chrom));
VLC_DEF(vlc_mb_type_b, max_len2(mb_type_b_tab));
VLC_DEF(vlc_sprite_traj, max_len(sprite_trajectory_lens));

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

static void tables_init(void) {
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
  for (i = 0; i < NTRAJ; i++) {  // canonical codes from lengths, in listed order (as ff_vlc_init_from_lengths)
    l[i] = sprite_trajectory_lens[i];
    c[i] = sprite_trajectory_code[i] = code >> (U32_BITS - l[i]);
    code += 1u << (U32_BITS - l[i]);
  }
  vlc_build(&vlc_sprite_traj, NTRAJ, c, l);
  rl_init(&rl_intra);
  rl_init(&rl_inter);
}

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
    return esc + ESC_LEVEL_BITS + rl->vlc[s][1] + SIGN_BITS;
  case ESC_RUN:
    if (alevel < 1 || alevel > RL_MAX_LEVEL) return 0;
    k = run - rl->max_run[last][alevel] - 1;
    if (k < 0 || (s = rl->index[last][k][alevel]) < 0) return 0;
    return esc + ESC_MODE_BITS + rl->vlc[s][1] + SIGN_BITS;
  default:
    return alevel <= ESC3_MAX ? esc + ESC3_LEN : 0;
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
  int skip_cap;
  U8 hbuf[HDR_PEEK + GB_PAD];  // a header chunk being parsed
} Ctx;

static void ctx_init(Ctx* c) {  // (the skip flags stay: ctx_quit() frees them)
  U8 *r = c->skip_ref, *u = c->skip_cur;
  const int cap = c->skip_cap;
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
  U8 *r, *u;
  if (++n <= c->skip_cap) return 1;
  if (!(r = (U8*)realloc(c->skip_ref, n))) return 0;
  c->skip_ref = r;
  if (!(u = (U8*)realloc(c->skip_cur, n))) return 0;
  c->skip_cur = u;
  memset(r + c->skip_cap, 0, n - c->skip_cap);
  memset(u + c->skip_cap, 0, n - c->skip_cap);
  c->skip_cap = n;
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

typedef struct Vop {  // state shared by the parsing and the writing direction
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


static int prefix_length(const Vop* v) {  // zeros of a resync marker
  switch (v->type) {
  case PT_I: return RESYNC_ZEROS;
  case PT_P: case PT_S: return RESYNC_ZEROS + v->fcode - 1;
  default: { int m = v->fcode > v->bcode ? v->fcode : v->bcode; return RESYNC_ZEROS + max2(m, RESYNC_B_MIN) - 1; }
  }
}

//--- parsing ---------------------------------------------------------------------------------------

static int get_mv(GB* gb, int f_code, int* diff) {  // FFmpeg's h263 decode_motion, without the prediction
  int code = vlc_get(gb, &vlc_mv), val, shift = f_code - 1;
  if (code < 0) return -1;
  if (!code) { *diff = 0; return 0; }
  val = code;
  if (gb_get1(gb)) val = -val;
  if (shift) {
    int a = ((code - 1) << shift | (int)gb_get(gb, shift)) + 1;
    val = val < 0 ? -a : a;
  }
  *diff = val;
  return 0;
}

static int get_traj(const Ctx* c, GB* gb, int* w) {
  int i, k;
  for (i = 0; i < c->warp_points; i++) {
    for (k = 0; k < 2; k++) {
      int len = vlc_get(gb, &vlc_sprite_traj);
      if (len < 0) return -1;
      w[2 * i + k] = len ? gb_xbits(gb, len) : 0;
      if ((k || !divx413(c)) && !gb_get1(gb)) return -1;   // marker
    }
  }
  return 0;
}

static int parse_vop_hdr(Ctx* c, GB* gb, VopHdr* h) {
  int i;
  h->type = (int)gb_get(gb, VOP_TYPE_BITS) + PT_I;
  h->mtb = 0;
  while (gb_get1(gb)) if (++h->mtb > RUN_MAX || gb_left(gb) <= 0) return -1;
  if (!gb_get1(gb)) return -1;          // marker
  if (c->time_inc_bits == 0 || !(gb_show(gb, c->time_inc_bits + 1) & 1)) {  // FFmpeg's guess
    for (c->time_inc_bits = TIB_MIN; c->time_inc_bits < TIB_MAX; c->time_inc_bits++) {
      if (h->type == PT_P || (h->type == PT_S && c->sprite_usage == GMC_SPRITE)) {
        if ((gb_show(gb, c->time_inc_bits + TIB_GUESS_P_BITS) & TIB_GUESS_P_MASK) == TIB_GUESS_P) break;
      } else if ((gb_show(gb, c->time_inc_bits + TIB_GUESS_BITS) & TIB_GUESS_MASK) == TIB_GUESS)
        break;
    }
  }
  h->tib = c->time_inc_bits;
  h->tinc = (int)gb_get(gb, h->tib);
  if (!gb_get1(gb)) return -1;          // marker
  h->coded = gb_get1(gb);
  h->rnd = h->dcthr = h->tff = h->alt = h->quant = h->fcode = h->bcode = NA;
  h->ncplx = 0;
  if (!h->coded) return 0;
  if (c->dp && h->type != PT_B) return -1;
  h->rnd = (h->type == PT_P || (h->type == PT_S && c->sprite_usage == GMC_SPRITE)) ? gb_get1(gb) : NA;
  h->ncplx = c->cplx_i + (h->type != PT_I ? c->cplx_p : 0) + (h->type == PT_B ? c->cplx_b : 0);
  for (i = 0; i < h->ncplx; i++) h->cplx[i] = (char)('0' + gb_get1(gb));
  if (gb_left(gb) < DCTHR_BITS) return -1;
  h->dcthr = (int)gb_get(gb, DCTHR_BITS);
  if (c->interlaced) { h->tff = gb_get1(gb); h->alt = gb_get1(gb); }
  if (h->type == PT_S && c->sprite_usage == GMC_SPRITE && get_traj(c, gb, h->warp) < 0) return -1;
  h->quant = (int)gb_get(gb, c->quant_precision);
  if (!h->quant) return -1;
  if (h->type != PT_I && !(h->fcode = (int)gb_get(gb, FCODE_BITS))) return -1;
  if (h->type == PT_B && !(h->bcode = (int)gb_get(gb, FCODE_BITS))) return -1;
  return gb_left(gb) < 0 ? -1 : 0;
}

enum { PB_MV, PB_DC, PB_COEF, NPB };
static thread_local double pbits[NPB];  // statistics: bits of motion vectors, intra DCs, coefficients parsed

static int parse_block(GB* gb, Blk* b, int n, int coded, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i = -1, p0 = gb->pos;
  b->n = 0;
  b->dc = 0;
  if (intra && dcvlc) {
    int size = vlc_get(gb, n < MB_LUMA ? &vlc_dc_lum : &vlc_dc_chrom);
    if (size < 0 || size > DC_SIZE_MAX) return -1;
    if (size) {
      b->dc = gb_xbits(gb, size);
      if (size > DC_MARKER_SIZE && !gb_get1(gb)) return -1;   // marker
    }
    i = 0;
  }
  pbits[PB_DC] += gb->pos - p0;
  p0 = gb->pos;
  if (!coded) return 0;
  for (;;) {
    int sym, run, level, last, mode = ESC_VLC;
    if (gb->pos > gb->size) return -1;
    sym = vlc_get(gb, &rl->v);
    if (sym < 0) return -1;
    if (sym == rl->n) {                 // escape
      U32 cache = gb_show(gb, ESC_MODE_BITS);
      if (cache >> 1) {                 // (the first of the mode's bits)
        gb_skip(gb, ESC_MODE_BITS);
        if (cache == ESC_FIXED_CODE) {  // mode 3: fixed length
          last = gb_get1(gb);
          run = (int)gb_get(gb, ESC3_RUN);
          if (!gb_get1(gb)) return -1;  // marker
          level = gb_sget(gb, ESC3_LEVEL);
          if (!gb_get1(gb)) return -1;  // marker
          mode = ESC_FIXED;
        } else {                        // mode 2: run offset
          sym = vlc_get(gb, &rl->v);
          if (sym < 0 || sym == rl->n) return -1;
          last = sym >= rl->last;
          level = rl->level[sym];
          run = rl->run[sym] + rl->max_run[last][level] + 1;
          if (gb_get1(gb)) level = -level;
          mode = ESC_RUN;
        }
      } else {                          // mode 1: level offset
        gb_skip(gb, ESC_LEVEL_BITS);
        sym = vlc_get(gb, &rl->v);
        if (sym < 0 || sym == rl->n) return -1;
        last = sym >= rl->last;
        run = rl->run[sym];
        level = rl->level[sym] + rl->max_level[last][run];
        if (gb_get1(gb)) level = -level;
        mode = ESC_LEVEL;
      }
    } else {
      last = sym >= rl->last;
      run = rl->run[sym];
      level = rl->level[sym];
      if (gb_get1(gb)) level = -level;
    }
    i += run + 1;
    if (i > (last ? BLK_LAST : BLK_LAST - 1)) return -1;
    b->run[b->n] = (U8)run;
    b->level[b->n] = (S16)level;
    b->esc[b->n] = (U8)mode;
    b->n++;
    if (last) { pbits[PB_COEF] += gb->pos - p0; return 0; }
  }
}

static int parse_blocks(GB* gb, MB* m, int cbp, int intra, int dcvlc) {
  int i;
  for (i = 0; i < MB_BLOCKS; i++, cbp += cbp)
    if (parse_block(gb, &m->b[i], i, cbp & 1 << (MB_BLOCKS - 1), intra, dcvlc) < 0) return -1;
  return 0;
}

static int parse_intra(const Ctx* c, GB* gb, Vop* v, MB* m, int cbpc, int dquant) {
  int cbpy, dcvlc;
  m->ac = gb_get1(gb);
  if ((cbpy = vlc_get(gb, &vlc_cbpy)) < 0) return -1;
  dcvlc = v->qscale < v->dcthr;
  m->dq = 0;
  if (dquant) { m->dq = quant_tab[gb_get(gb, DQUANT_BITS)]; v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX); }
  m->dt = c->interlaced ? gb_get1(gb) : NA;
  return parse_blocks(gb, m, (cbpc & MCBPC_CHROMA) | (cbpy << CBP_CHROMA_BITS), 1, dcvlc);
}

static int parse_mvs(GB* gb, MB* m, int n, int f_code) {
  const int p0 = gb->pos;
  int i;
  for (i = 0; i < 2 * n; i++, m->nmv++)
    if (get_mv(gb, f_code, &m->mv[m->nmv]) < 0) return -1;
  pbits[PB_MV] += gb->pos - p0;
  return 0;
}

static int parse_mb(const Ctx* c, GB* gb, Vop* v, MB* m, int idx) {
  int cbpc, cbpy, cbp;
  m->st = m->sk = 0;
  m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
  m->nfs = m->fs = m->nmv = 0;
  if (gb->pos > gb->size) return -1;
  if (v->type == PT_I) {
    for (;;) {
      if ((cbpc = vlc_get(gb, &vlc_intra_mcbpc)) < 0) return -1;
      if (cbpc != MCBPC_STUFF_I) break;
      if (++m->st > RUN_MAX) return -1;
    }
    m->t = MB_INTRA;
    return parse_intra(c, gb, v, m, cbpc, cbpc & MCBPC_IQUANT);
  }
  if (v->type != PT_B) {
    for (;;) {
      if (gb_get1(gb)) { m->sk = 1; return 0; }
      if ((cbpc = vlc_get(gb, &vlc_inter_mcbpc)) < 0) return -1;
      if (cbpc != MCBPC_STUFF_P) break;
      if (++m->st > RUN_MAX) return -1;
    }
    if (cbpc & MCBPC_INTRA) { m->t = MB_INTRA; return parse_intra(c, gb, v, m, cbpc, cbpc & MCBPC_DQUANT); }
    m->t = (cbpc & MCBPC_4MV) ? MB_4MV : MB_INTER;
    if (v->gmc && m->t == MB_INTER) m->mc = gb_get1(gb);
    if ((cbpy = vlc_get(gb, &vlc_cbpy)) < 0) return -1;
    cbp = (cbpc & MCBPC_CHROMA) | ((cbpy ^ CBPY_INTER) << CBP_CHROMA_BITS);
    m->dq = 0;
    if (cbpc & MCBPC_DQUANT) {
      m->dq = quant_tab[gb_get(gb, DQUANT_BITS)];
      v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
    }
    if (c->interlaced && cbp) m->dt = gb_get1(gb);
    if (m->t == MB_4MV) {
      if (parse_mvs(gb, m, MB_LUMA, v->fcode) < 0) return -1;
    } else if (m->mc != 1) {
      if (c->interlaced) m->fp = gb_get1(gb);
      if (m->fp == 1) { m->nfs = 2; m->fs = (int)gb_get(gb, 2); }  // (a select per field)
      if (parse_mvs(gb, m, m->fp == 1 ? 2 : 1, v->fcode) < 0) return -1;
    }
    return parse_blocks(gb, m, cbp, 0, 0);
  }
  // B-VOP
  if (c->skip_ref[idx]) { m->sk = 1; return 0; }
  if (gb_get1(gb)) { m->mb = MODB_NONE; m->t = MB_DIRECT; return parse_blocks(gb, m, 0, 0, 0); }
  m->mb = gb_get1(gb) ? MODB_NOCBP : MODB_CBP;
  if ((m->t = vlc_get(gb, &vlc_mb_type_b)) < 0) return -1;
  cbp = m->mb == MODB_CBP ? (int)gb_get(gb, MB_BLOCKS) : 0;
  if (m->t != MB_DIRECT && cbp) {
    m->dq = gb_get1(gb) ? (gb_get1(gb) ? DQ_B : -DQ_B) : 0;
    v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
  }
  if (c->interlaced) {
    if (cbp) m->dt = gb_get1(gb);
    if (m->t != MB_DIRECT) {
      m->fp = gb_get1(gb);
      if (m->fp == 1) { m->nfs = 2 * (fwd_dir(m->t) + bwd_dir(m->t)); m->fs = (int)gb_get(gb, m->nfs); }
    }
  }
  if (m->t != MB_DIRECT) {
    const int n = m->fp == 1 ? 2 : 1;
    if (fwd_dir(m->t) && parse_mvs(gb, m, n, v->fcode) < 0) return -1;
    if (bwd_dir(m->t) && parse_mvs(gb, m, n, v->bcode) < 0) return -1;
  } else if (parse_mvs(gb, m, 1, FCODE_MIN) < 0)
    return -1;
  return parse_blocks(gb, m, cbp, 0, 0);
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

static int parse_vp(const Ctx* c, GB* gb, Vop* v, VpHdr* k) {
  int len;
  if (gb->pos > gb->size - VP_HDR_MIN) return -1;
  for (len = 0; len < ZEROS_MAX; len++)
    if (gb_get1(gb)) break;
  if (len != prefix_length(v)) return -1;
  k->mbnum = (int)gb_get(gb, c->mb_num_bits);
  if (k->mbnum >= c->mb_num || !k->mbnum) return -1;
  k->quant = (int)gb_get(gb, c->quant_precision);
  if (k->quant) v->qscale = k->quant;
  k->hec = gb_get1(gb);
  k->mtb = k->tinc = k->type = k->dcthr = k->fcode = k->bcode = NA;
  if (k->hec) {
    k->mtb = 0;
    while (gb_get1(gb)) if (++k->mtb > RUN_MAX || gb_left(gb) <= 0) return -1;
    if (!gb_get1(gb)) return -1;        // marker
    k->tinc = (int)gb_get(gb, v->tib);
    if (!gb_get1(gb)) return -1;        // marker
    k->type = (int)gb_get(gb, VOP_TYPE_BITS);
    k->dcthr = (int)gb_get(gb, DCTHR_BITS);
    if (v->gmc && get_traj(c, gb, k->warp) < 0) return -1;
    if (v->type != PT_I) k->fcode = (int)gb_get(gb, FCODE_BITS);
    if (v->type == PT_B) k->bcode = (int)gb_get(gb, FCODE_BITS);
  }
  return gb_left(gb) < 0 ? -1 : 0;
}

//--- writing ---------------------------------------------------------------------------------------

static void put_mv(PB* pb, int diff, int f_code) {
  const int shift = f_code - 1;
  int a, code;
  if (!diff) { pb_put(pb, mv_tab[0][0], mv_tab[0][1]); return; }
  a = (diff < 0 ? -diff : diff) - 1;
  code = (a >> shift) + 1;
  pb_put(pb, mv_tab[code][0], mv_tab[code][1]);
  pb_put(pb, diff < 0, 1);
  pb_put(pb, (U32)(a & ((1 << shift) - 1)), shift);
}

static int mv_ok(int diff, int f_code) {
  return diff >= -(MV_VLC_MAX << (f_code - 1)) && diff <= MV_VLC_MAX << (f_code - 1);
}

static int put_traj(const Ctx* c, PB* pb, const int* w) {
  int i;
  for (i = 0; i < 2 * c->warp_points; i++) {
    const int len = bitlen(w[i]);
    if (len >= countof(sprite_trajectory_lens)) return -1;
    pb_put(pb, sprite_trajectory_code[len], sprite_trajectory_lens[len]);
    if (len) pb_xbits(pb, w[i], len);
    if ((i & 1) || !divx413(c)) pb_put(pb, 1, 1);
  }
  return 0;
}

static int put_block(PB* pb, const Blk* b, int n, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i;
  if (intra && dcvlc) {
    const int size = bitlen(b->dc);
    const U8* t = n < MB_LUMA ? DCtab_lum[size] : DCtab_chrom[size];
    pb_put(pb, t[0], t[1]);
    if (size) {
      pb_xbits(pb, b->dc, size);
      if (size > DC_MARKER_SIZE) pb_put(pb, 1, MARKER_BITS);
    }
  }
  for (i = 0; i < b->n; i++) {
    const int run = b->run[i], level = b->level[i], last = i + 1 == b->n, alevel = level < 0 ? -level : level;
    int mode = b->esc[i] == ESC_DEFAULT ? esc_default(rl, last, run, alevel) : b->esc[i], s, k;
    if (!esc_bits(rl, mode, last, run, alevel)) return -1;
    switch (mode) {
    case ESC_VLC:
      s = rl->index[last][run][alevel];
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, SIGN_BITS);
      break;
    case ESC_LEVEL:
      s = rl->index[last][run][alevel - rl->max_level[last][run]];
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, ESC_LEVEL_CODE, ESC_LEVEL_BITS);
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, SIGN_BITS);
      break;
    case ESC_RUN:
      k = run - rl->max_run[last][alevel] - 1;
      s = rl->index[last][k][alevel];
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, ESC_RUN_CODE, ESC_MODE_BITS);
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, SIGN_BITS);
      break;
    default:
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, ESC_FIXED_CODE, ESC_MODE_BITS);
      pb_put(pb, last, 1);
      pb_put(pb, run, ESC3_RUN);
      pb_put(pb, 1, MARKER_BITS);
      pb_put(pb, (U32)level & ((1u << ESC3_LEVEL) - 1), ESC3_LEVEL);
      pb_put(pb, 1, MARKER_BITS);
      break;
    }
  }
  return 0;
}

static int cbp_of(const MB* m) {  // coded flags, block 0 in the top bit (of MB_BLOCKS)
  int i, cbp = 0;
  for (i = 0; i < MB_BLOCKS; i++) cbp = cbp * 2 + (m->b[i].n > 0);
  return cbp;
}

static int put_blocks(PB* pb, const MB* m, int intra, int dcvlc) {
  int i;
  for (i = 0; i < MB_BLOCKS; i++)
    if (put_block(pb, &m->b[i], i, intra, dcvlc) < 0) return -1;
  return 0;
}

static int dq_index(int dq) {
  int i;
  for (i = 0; i < countof(quant_tab); i++)
    if (quant_tab[i] == dq) return i;
  return -1;
}

static void put_stuffing(PB* pb, const Vop* v, int n) {  // MCBPC stuffing codes
  while (n-- > 0) {
    if (v->type == PT_I) pb_put(pb, intra_mcbpc_code[MCBPC_STUFF_I], intra_mcbpc_bits[MCBPC_STUFF_I]);
    else { pb_put(pb, 0, 1); pb_put(pb, inter_mcbpc_code[MCBPC_STUFF_P], inter_mcbpc_bits[MCBPC_STUFF_P]); }
  }
}


static int put_intra(const Ctx* c, PB* pb, Vop* v, const MB* m) {  // after the MCBPC
  const int cbp = cbp_of(m), dcvlc = v->qscale < v->dcthr;
  pb_put(pb, (U32)m->ac, 1);
  pb_put(pb, cbpy_tab[cbp >> CBP_CHROMA_BITS][0], cbpy_tab[cbp >> CBP_CHROMA_BITS][1]);
  if (m->dq) {
    pb_put(pb, (U32)dq_index(m->dq), DQUANT_BITS);
    v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
  }
  if (c->interlaced) pb_put(pb, (U32)m->dt, 1);
  return put_blocks(pb, m, 1, dcvlc);
}

// The writers check what the parser checks: decoded records come from data that may be damaged.

static int put_vop_hdr(const Ctx* c, PB* pb, const VopHdr* h) {  // 00 00 01 B6 and the VOP header
  int i;
  if (h->mtb > RUN_MAX) return -1;
  pb_put(pb, SC_START << BYTE_BITS | SC_VOP, SC_LEN * BYTE_BITS);
  pb_put(pb, (U32)(h->type - PT_I), VOP_TYPE_BITS);
  for (i = 0; i < h->mtb; i++) pb_put(pb, 1, 1);
  pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)h->tinc, h->tib);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)h->coded, 1);
  if (!h->coded) return 0;
  if (h->quant < 1 || h->quant >= 1 << c->quant_precision || !h->fcode || !h->bcode) return -1;
  if (h->rnd != NA) pb_put(pb, (U32)h->rnd, 1);
  for (i = 0; i < h->ncplx; i++) pb_put(pb, (U32)(h->cplx[i] - '0'), 1);
  pb_put(pb, (U32)h->dcthr, DCTHR_BITS);
  if (c->interlaced) { pb_put(pb, (U32)h->tff, 1); pb_put(pb, (U32)h->alt, 1); }
  if (h->type == PT_S && c->sprite_usage == GMC_SPRITE && put_traj(c, pb, h->warp) < 0) return -1;
  pb_put(pb, (U32)h->quant, c->quant_precision);
  if (h->fcode != NA) pb_put(pb, (U32)h->fcode, FCODE_BITS);
  if (h->bcode != NA) pb_put(pb, (U32)h->bcode, FCODE_BITS);
  return 0;
}

static int put_vp(const Ctx* c, PB* pb, Vop* v, const VpHdr* k) {  // stuffing and video packet header
  int i;
  if (k->st > RUN_MAX || (k->st && v->type == PT_S) || k->quant < 0 || k->quant >= 1 << c->quant_precision)
    return -1;
  put_stuffing(pb, v, k->st);
  pb_put(pb, 0, 1);                     // stuffing up to the byte boundary
  while (pb->n) pb_put(pb, 1, 1);
  for (i = 0; i < prefix_length(v); i++) pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->mbnum, c->mb_num_bits);
  pb_put(pb, (U32)k->quant, c->quant_precision);
  pb_put(pb, (U32)k->hec, 1);
  if (k->quant) v->qscale = k->quant;
  v->qscale = clipi(v->qscale, QS_MIN, QS_MAX);
  if (!k->hec) return 0;
  if (k->mtb > RUN_MAX) return -1;
  for (i = 0; i < k->mtb; i++) pb_put(pb, 1, 1);
  pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->tinc, v->tib);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->type, VOP_TYPE_BITS);
  pb_put(pb, (U32)k->dcthr, DCTHR_BITS);
  if (v->gmc && put_traj(c, pb, k->warp) < 0) return -1;
  if (k->fcode != NA) pb_put(pb, (U32)k->fcode, FCODE_BITS);
  if (k->bcode != NA) pb_put(pb, (U32)k->bcode, FCODE_BITS);
  return 0;
}

static int put_mb(const Ctx* c, PB* pb, Vop* v, const MB* m) {
  const int cbp = cbp_of(m);
  int i;
  if (m->st > RUN_MAX) return -1;
  if (v->type == PT_I) {
    put_stuffing(pb, v, m->st);
    i = (m->dq ? MCBPC_IQUANT : 0) | (cbp & MCBPC_CHROMA);
    pb_put(pb, intra_mcbpc_code[i], intra_mcbpc_bits[i]);
    return put_intra(c, pb, v, m);
  }
  if (v->type != PT_B) {
    const int cy = (cbp >> CBP_CHROMA_BITS) ^ CBPY_INTER;
    put_stuffing(pb, v, m->st);
    pb_put(pb, (U32)m->sk, 1);
    if (m->sk) return 0;
    if (m->t == MB_INTRA) {
      i = MCBPC_INTRA | (m->dq ? MCBPC_DQUANT : 0) | (cbp & MCBPC_CHROMA);
      pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
      return put_intra(c, pb, v, m);
    }
    i = (m->t == MB_4MV ? MCBPC_4MV : 0) | (m->dq ? MCBPC_DQUANT : 0) | (cbp & MCBPC_CHROMA);
    pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
    if (m->mc != NA) pb_put(pb, (U32)m->mc, 1);
    pb_put(pb, cbpy_tab[cy][0], cbpy_tab[cy][1]);
    if (m->dq) {
      pb_put(pb, (U32)dq_index(m->dq), DQUANT_BITS);
      v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
    }
    if (m->dt != NA) pb_put(pb, (U32)m->dt, 1);
    if (m->fp != NA) pb_put(pb, (U32)m->fp, 1);
    if (m->fp == 1) pb_put(pb, (U32)m->fs, 2);  // (a select per field)
    for (i = 0; i < m->nmv; i++) {
      if (!mv_ok(m->mv[i], v->fcode)) return -1;
      put_mv(pb, m->mv[i], v->fcode);
    }
    return put_blocks(pb, m, 0, 0);
  }
  if (m->sk) return 0;                  // B-VOP
  if (m->mb == MODB_NONE) { pb_put(pb, 1, 1); return 0; }
  pb_put(pb, 0, 1);
  pb_put(pb, m->mb == MODB_NOCBP, 1);
  pb_put(pb, mb_type_b_tab[m->t][0], mb_type_b_tab[m->t][1]);
  if (m->mb == MODB_CBP) pb_put(pb, (U32)cbp, MB_BLOCKS);
  if (m->dq != NA) {
    if (!m->dq) pb_put(pb, DBQUANT_ZERO, 1);
    else pb_put(pb, m->dq > 0 ? DBQUANT_POS : DBQUANT_NEG, DBQUANT_BITS);
    v->qscale = clipi(v->qscale + m->dq, QS_MIN, QS_MAX);
  }
  if (m->dt != NA) pb_put(pb, (U32)m->dt, 1);
  if (m->fp != NA) pb_put(pb, (U32)m->fp, 1);
  if (m->nfs) pb_put(pb, (U32)m->fs, m->nfs);
  for (i = 0; i < m->nmv; i++) {  // forward, then backward; delta vector of direct mode
    const int f = m->t == MB_DIRECT ? FCODE_MIN : fwd_dir(m->t) && i < m->nmv / (1 + bwd_dir(m->t)) ? v->fcode : v->bcode;
    if (!mv_ok(m->mv[i], f)) return -1;
    put_mv(pb, m->mv[i], f);
  }
  return put_blocks(pb, m, 0, 0);
}

