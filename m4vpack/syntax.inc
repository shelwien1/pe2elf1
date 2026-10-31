// syntax.inc -- MPEG-4 Part 2 video syntax, from m4vdec.cpp (a port of FFmpeg's decoder): the bit
// reader and writer, the VLC tables, the stream state the syntax depends on (VOL, user data),
// and the records (VOP headers, video packet headers, macroblocks), each one walk that reads it
// from the bits or writes it back.

static inline int sign_extend(int val, int bits) { int sh = U32_BITS - bits; return (int)((U32)val << sh) >> sh; }
static int bitlen(int v) { U32 a = v < 0 ? 0u - (U32)v : (U32)v; int n = 0; while (a) { n++; a >>= 1; } return n; }
static inline int stuffing_bits(int nb) { return (1 << (nb - 1)) - 1; }  // nb bits of the usual stuffing: 0, then 1s

//--- bit reader (from m4vdec; buffers carry 64 bytes of zero padding) -------------------------

struct GB {
  const U8* buf;
  int size, pos;  // in bits

  void init(pcU8 p, size_t n) { buf = p; size = (int)n * BYTE_BITS; pos = 0; }  // the n bytes at p
  U32 show(int n) const {  // 0 <= n <= 32
    const U8* p = buf + (pos >> BYTE_SHIFT);
    U64 v = 0;
    int i;
    for (i = 0; i < U64_BYTES; i++) v = v << BYTE_BITS | p[i];  // (the next 8 bytes, big endian)
    v <<= (pos & (BYTE_BITS - 1));
    return n ? (U32)(v >> (U64_BITS - n)) : 0;
  }
  U32 get(int n) { U32 v = show(n); pos += n; return v; }
  int get1() { return (int)get(1); }
  void skip(int n) { pos += n; }
  int left() const { return size - pos; }
  void align() { pos += to_byte(pos); }
  int sget(int n) { return sign_extend((int)get(n), n); }
  int xbits(int n) {  // FFmpeg get_xbits: MSB set -> positive, else negative
    int v = (int)get(n);
    return (v >> (n - 1)) ? v : v - (1 << n) + 1;
  }
};

//--- bit writer ------------------------------------------------------------------------------

// n bits pending in acc; a full buffer goes to flush (with ctx), where there is one (else what
// doesn't fit is an error), done counting the bytes it took
struct PB {
  U8* buf;
  size_t cap, len;
  U64 acc;
  int n, err;
  void (*flush)(void*, pcU8, size_t);
  void* ctx;
  U64 done;

  void init(pU8 b, size_t c) { *this = PB(); buf = b; cap = c; }
  void put(U32 v, int k) {  // 0 <= k <= 32
    if (k <= 0) return;
    acc = (acc << k) | (k < U32_BITS ? v & ((1u << k) - 1) : v);
    for (n += k; n >= BYTE_BITS;) {
      n -= BYTE_BITS;
      if (len == cap && flush) { flush(ctx, buf, len); done += len; len = 0; }
      if (len < cap) buf[len++] = (U8)(acc >> n);
      else err = 1;
    }
  }
  void finish() {  // the bytes in the buffer to flush
    if (flush && len) { flush(ctx, buf, len); done += len; len = 0; }
  }
  void xbits(int v, int k) { put((U32)(v > 0 ? v : v + (1 << k) - 1), k); }
};

//--- VLC (from m4vdec) -------------------------------------------------------------------------

// A VLC both ways: for the reader a lookup table of 1 << bits entries, by a code's first bits (its
// symbol and length; 0: no code starts so), for the writer each of its n symbols' code and length
// (0: none)
struct VLCE { S16 sym, len; };
constexpr int VLC_SYMS_MAX = 128;  // a VLC's symbols, at most (the RL tables have the most)
struct VLC {
  rp<VLCE> tab;  // (each VLC's a table of its own)
  int bits, n;
  U32 code[VLC_SYMS_MAX];
  U8 len[VLC_SYMS_MAX];

  void build(int count, rp<const U32> codes, pcU8 lens) {
    int i;
    n = count;
    for (i = 0; i < count; i++) {
      U32 j, first, cnt;
      code[i] = codes[i];
      len[i] = lens[i];
      if (!lens[i]) continue;
      first = codes[i] << (bits - lens[i]);
      cnt = 1u << (bits - lens[i]);
      for (j = 0; j < cnt; j++) { tab[first + j].sym = (S16)i; tab[first + j].len = lens[i]; }
    }
  }
  // from a table of codes and one of their lengths
  template <size_t N>
  void from(const U8 (&codes)[N], const U8 (&lens)[N]) {
    U32 c[N];
    U8 l[N];
    size_t i;
    for (i = 0; i < N; i++) { c[i] = codes[i]; l[i] = lens[i]; }
    build((int)N, c, l);
  }
  // ... and from a table of (code, length) pairs
  template <size_t N>
  void from2(const U8 (&t)[N][2]) {
    U32 c[N];
    U8 l[N];
    size_t i;
    for (i = 0; i < N; i++) { c[i] = t[i][0]; l[i] = t[i][1]; }
    build((int)N, c, l);
  }
  int get(GB& g) const {  // the symbol read, -1 where no code starts
    const VLCE& e = tab[g.show(bits)];
    if (!e.len) return -1;
    g.skip(e.len);
    return e.sym;
  }
};
// a VLC with its static lookup table
#define VLC_DEF(name, nbits) static VLCE name##_tab[1 << (nbits)]; static VLC name = { name##_tab, nbits, 0, {0}, {0} }

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

// FFmpeg's RLTable: the symbols (n of them; then the escape code) as codes (vlc), their runs and
// levels, the first with last = 1 (first_last)
constexpr int RL_MAX_RUN = 64, RL_MAX_LEVEL = 64;  // (FFmpeg's MAX_RUN, MAX_LEVEL)
struct RL {
  int n, first_last;
  rp<const U16[2]> vlc;
  rp<const S8> run_of;
  rp<const S8> level_of;
  U8 max_level[2][RL_MAX_RUN + 1], max_run[2][RL_MAX_LEVEL + 1];
  VLC v;
  S16 index[2][RL_MAX_RUN][RL_MAX_LEVEL + 1];  // [last][run][level] -> symbol, -1 if there is none

  void init() {  // the rest from the symbols
    int last, i;
    U32 codes[countof(intra_vlc)];
    U8 lens[countof(intra_vlc)];
    memset(max_level, 0, sizeof max_level);
    memset(max_run, 0, sizeof max_run);
    memset(index, -1, sizeof index);
    for (last = 0; last < 2; last++) {
      const int start = last ? first_last : 0, end = last ? n : first_last;
      for (i = start; i < end; i++) {
        const int run = run_of[i], level = level_of[i];
        if (level > max_level[last][run]) max_level[last][run] = (U8)level;
        if (run > max_run[last][level]) max_run[last][level] = (U8)run;
        index[last][run][level] = (S16)i;
      }
    }
    for (i = 0; i <= n; i++) { codes[i] = vlc[i][0]; lens[i] = (U8)vlc[i][1]; }
    v.build(n + 1, codes, lens);
  }

  // Length of an event (last, run, |level|) coded in an escape mode (ESC_VLC: plain VLC), 0 if that
  // mode can't code it.
  int esc_bits(int mode, int last, int run, int alevel) const {
    const int esc = vlc[n][1];  // (the escape code's length)
    int s, k;
    if (run < 0 || run > BLK_LAST) return 0;
    switch (mode) {
    case ESC_VLC:
      if (alevel < 1 || alevel > RL_MAX_LEVEL || (s = index[last][run][alevel]) < 0) return 0;
      return vlc[s][1] + SIGN_BITS;
    case ESC_LEVEL:
      k = alevel - max_level[last][run];
      if (k < 1 || k > RL_MAX_LEVEL || (s = index[last][run][k]) < 0) return 0;
      return esc + esc_tab[ESC_LEVEL][1] + vlc[s][1] + SIGN_BITS;
    case ESC_RUN:
      if (alevel < 1 || alevel > RL_MAX_LEVEL) return 0;
      k = run - max_run[last][alevel] - 1;
      if (k < 0 || (s = index[last][k][alevel]) < 0) return 0;
      return esc + esc_tab[ESC_RUN][1] + vlc[s][1] + SIGN_BITS;
    default:
      return alevel <= ESC3_MAX ? esc + esc_tab[ESC_FIXED][1] + ESC3_LEN : 0;
    }
  }

  // default escape mode: plain VLC when possible, else the first escape mode that can code it
  int esc_default(int last, int run, int alevel) const {
    int m;
    for (m = ESC_VLC; m < ESC_FIXED; m++)
      if (esc_bits(m, last, run, alevel)) return m;
    return ESC_FIXED;
  }

  // A coefficient event (last, run, level) both ways (the walks below say how): a symbol and the
  // sign, or the escape code, the mode's code (esc_tab) and the event as the mode has it: ESC_LEVEL,
  // a symbol of the level less the run's largest; ESC_RUN, a symbol of the run less the level's
  // longest, less 1 (each with the sign); ESC_FIXED, last, run, a marker, the level, a marker
  template <class IO>
  int event_walk(IO& io, pInt last, pInt run, pInt level, pInt mode) const {
    int sym = n, s, neg = *level < 0, r, l;
    if (IO::W) {
      const int al = neg ? -*level : *level;
      if (!esc_bits(*mode, *last, *run, al)) return 0;  // (one its mode can't code)
      if (*mode == ESC_VLC) sym = index[*last][*run][al];
      else if (*mode == ESC_LEVEL) sym = index[*last][*run][al - max_level[*last][*run]];
      else if (*mode == ESC_RUN) sym = index[*last][*run - max_run[*last][al] - 1][al];
    }
    s = IO::W && *mode == ESC_VLC ? sym : n;
    if (!io.vlc(&s, v)) return 0;
    if (s != n) {
      *mode = ESC_VLC;
      sym = s;
    } else {
      if (!io.vlc(mode, vlc_esc)) return 0;
      if (*mode == ESC_FIXED)
        return io.bit(last) && io.bits(run, ESC3_RUN) && io.marker() && io.sbits(level, ESC3_LEVEL) && io.marker();
      if (!io.vlc(&sym, v) || sym == n) return 0;
    }
    *last = sym >= first_last;
    r = run_of[sym];
    l = level_of[sym];
    if (*mode == ESC_LEVEL) l += max_level[*last][r];
    else if (*mode == ESC_RUN) r += max_run[*last][l] + 1;
    if (!io.bit(&neg)) return 0;
    *run = r;
    *level = neg ? -l : l;
    return 1;
  }
};
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

static int tables_init(void) {  // (the VLC and RL tables above)
  constexpr int NTRAJ = countof(sprite_trajectory_lens);
  U32 c[NTRAJ]; U8 l[NTRAJ]; int i;
  U32 code = 0;
  vlc_intra_mcbpc.from(intra_mcbpc_code, intra_mcbpc_bits);
  vlc_inter_mcbpc.from(inter_mcbpc_code, inter_mcbpc_bits);
  vlc_cbpy.from2(cbpy_tab);
  vlc_mv.from2(mv_tab);
  vlc_dc_lum.from2(DCtab_lum);
  vlc_dc_chrom.from2(DCtab_chrom);
  vlc_mb_type_b.from2(mb_type_b_tab);
  vlc_dbquant.from2(dbquant_tab);
  vlc_esc.from2(esc_tab);
  for (i = 0; i < NTRAJ; i++) {  // canonical codes from lengths, in listed order (as ff_vlc_init_from_lengths)
    l[i] = sprite_trajectory_lens[i];
    c[i] = code >> (U32_BITS - l[i]);
    code += 1u << (U32_BITS - l[i]);
  }
  vlc_sprite_traj.build(NTRAJ, c, l);
  rl_intra.init();
  rl_inter.init();
  return 1;
}
static const int tables_made = tables_init();  // (as the program starts, as cm.inc's tables)

//--- stream context ----------------------------------------------------------------------------

enum { PT_I = 1, PT_P = 2, PT_B = 3, PT_S = 4, PT_N };  // same values as AVPictureType (FFmpeg relies on them)
// the start code at p (SC_LEN bytes): its code, -1 if there is none
static inline int sc_code(pcU8 p) { return !p[0] && !p[1] && p[2] == SC_START ? p[SC_PREFIX] : -1; }
// a VOP's type (PT_*) from its vop_coding_type, the top bits of the byte after the start code at p
constexpr int VOP_TYPE_SHIFT = 6;
static inline int vop_type(pcU8 p) { return PT_I + (p[SC_LEN] >> VOP_TYPE_SHIFT); }
enum { STATIC_SPRITE = 1, GMC_SPRITE = 2 };

#include "ctx.inc"  // Ctx: the stream's state the syntax depends on (the VOL, user data, GOP)

// a macroblock record's codes (m4vtsv's): t, its type (P-VOP, S-VOP: MB_INTER, MB_4MV, MB_INTRA;
// B-VOP, mb_type's VLC: MB_DIRECT, MB_BI, MB_BWD, MB_FWD), and mb, a B-VOP's modb (MODB_NONE:
// direct without a vector or coefficients, MODB_NOCBP: no coded block pattern, MODB_CBP)
enum { MB_INTER, MB_4MV, MB_INTRA };
enum { MB_DIRECT, MB_BI, MB_BWD, MB_FWD };
enum { MODB_NONE, MODB_NOCBP, MODB_CBP };
static int fwd_dir(int t) { return t == MB_BI || t == MB_FWD; }   // B-VOP macroblock types using the forward / backward reference
static int bwd_dir(int t) { return t == MB_BI || t == MB_BWD; }

// the place of v in t (-1: none)
template <size_t N>
static int index_of(const S8 (&t)[N], int v) {
  int i;
  for (i = 0; i < (int)N; i++)
    if (t[i] == v) return i;
  return -1;
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

// Each record's syntax is one walk for both directions: the walk() of VopHdr, VpHdr, MB and the parts
// of a macroblock take Rd, which reads the fields from the bits into the record, or Wr, which writes
// the record's fields as bits. The walk says which fields there are, in what order, of what length,
// and what they may be; an element's two directions are side by side in Rd and Wr. An element
// returns 0 where its field can't be (the reader at a code that isn't one, the writer at a value the
// field can't hold: it gets records decoded from data that may be damaged, and checks what the reader
// does), and a walk returns 0 where its record can't be. What a walk works out before an element (a
// code, a length) is the writer's: the reader's element overwrites it.

struct Rd {  // the bits read into a record
  static constexpr bool W = false;
  GB& g;
  int pos() const { return g.pos; }
  int over() const { return g.pos > g.size; }      // past the end (where the walks stop)
  int short_of(int n) const { return g.left() < n; }  // fewer than n bits left
  void count(int k, int p0) const { pbits[k] += g.pos - p0; }
  int bits(pInt v, int n) { *v = (int)g.get(n); return 1; }
  int bit(pInt v) { return bits(v, 1); }
  int sbits(pInt v, int n) { *v = g.sget(n); return 1; }   // two's complement
  int xbits(pInt v, int n) { *v = g.xbits(n); return 1; }  // FFmpeg's get_xbits
  int marker() { return g.get1(); }
  int vlc(pInt s, const VLC& v) { return (*s = v.get(g)) >= 0; }
  int ones(pInt n) {  // 1s, then a 0 (modulo_time_base)
    for (*n = 0; g.get1();)
      if (++*n > RUN_MAX || g.left() <= 0) return 0;
    return 1;
  }
  int stuffing(pInt n, int type) {  // MCBPC stuffing codes, counted
    for (*n = 0; g.show(mcbpc_stuff_len(type)) == mcbpc_stuff_code(type);) {
      g.skip(mcbpc_stuff_len(type));
      if (++*n > RUN_MAX) return 0;
    }
    return 1;
  }
  void byte_stuffing() { g.skip(1); g.align(); }  // (a 0, then 1s up to the byte boundary)
  // vop_time_increment's length: the VOL's, or where it has none or what follows doesn't fit it,
  // FFmpeg's guess from the header's next fields (P-VOPs and GMC S-VOPs: vop_rounding_type among
  // them), kept in c
  int time_bits(Ctx& c, int type, pInt tib) {
    if (c.time_inc_bits == 0 || !(g.show(c.time_inc_bits + 1) & 1)) {
      for (c.time_inc_bits = TIB_MIN; c.time_inc_bits < TIB_MAX; c.time_inc_bits++) {
        if (c.vop_has_rnd(type)) {
          if ((g.show(c.time_inc_bits + TIB_GUESS_P_BITS) & TIB_GUESS_P_MASK) == TIB_GUESS_P) break;
        } else if ((g.show(c.time_inc_bits + TIB_GUESS_BITS) & TIB_GUESS_MASK) == TIB_GUESS)
          break;
      }
    }
    *tib = c.time_inc_bits;
    return 1;
  }
};

struct Wr {  // a record written as bits
  static constexpr bool W = true;
  PB& p;
  int pos() const { return 0; }
  int over() const { return 0; }
  int short_of(int) const { return 0; }
  void count(int, int) const {}
  int bits(pInt v, int n) {
    if (n < U32_BITS && (U32)*v >> n) return 0;
    p.put((U32)*v, n);
    return 1;
  }
  int bit(pInt v) { return bits(v, 1); }
  int sbits(pInt v, int n) {
    if (*v < -(1 << (n - 1)) || *v >= 1 << (n - 1)) return 0;
    p.put((U32)*v & ((1u << n) - 1), n);
    return 1;
  }
  int xbits(pInt v, int n) {  // (n: the value's length)
    if (bitlen(*v) != n) return 0;
    p.xbits(*v, n);
    return 1;
  }
  int marker() { p.put(1, MARKER_BITS); return 1; }
  int vlc(pInt s, const VLC& v) {
    if (*s < 0 || *s >= v.n || !v.len[*s]) return 0;
    p.put(v.code[*s], v.len[*s]);
    return 1;
  }
  int ones(pInt n) {
    int i;
    if (*n < 0 || *n > RUN_MAX) return 0;
    for (i = 0; i < *n; i++) p.put(1, 1);
    p.put(0, 1);
    return 1;
  }
  int stuffing(pInt n, int type) {
    int i;
    if (*n < 0 || *n > RUN_MAX) return 0;
    for (i = 0; i < *n; i++) p.put(mcbpc_stuff_code(type), mcbpc_stuff_len(type));
    return 1;
  }
  void byte_stuffing() {
    p.put(0, 1);
    while (p.n) p.put(1, 1);
  }
  int time_bits(Ctx&, int, pInt) { return 1; }  // (the record's)
};

// a vector difference (FFmpeg's h263 decode_motion, without the prediction): its magnitude's code
// (vlc_mv), a sign and f_code - 1 low bits
template <class IO>
static int mv_walk(IO& io, pInt d, int f_code) {
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
  if (!io.vlc(&code, vlc_mv)) return 0;
  if (!code) {
    *d = 0;
    return 1;
  }
  if (!io.bit(&neg) || !io.bits(&lo, shift)) return 0;
  *d = ((code - 1) << shift | lo) + 1;
  if (neg) *d = -*d;
  return 1;
}

//--- records ------------------------------------------------------------------------------------

constexpr size_t VOP_PEEK = 64;  // the bytes of a VOP's header (from its start code on) VopHdr::coded_at() reads

struct VopHdr {
  int type, mtb, tib, tinc, coded, rnd, dcthr, tff, alt, quant, fcode, bcode;
  int warp[2 * WARP_POINTS_MAX];
  int ncplx; char cplx[CPLX_BITS_MAX];

  void clear() {  // the optional fields absent (NA): where one read or decoded starts
    rnd = dcthr = tff = alt = quant = fcode = bcode = NA;
    ncplx = 0;
  }

  // A VOP's header after its start code (FFmpeg's decode_vop_header): vop_coding_type,
  // modulo_time_base, vop_time_increment, vop_coded; then the rounding type, the complexity
  // estimates, intra_dc_vlc_thr, the field flags, the trajectory, the quantiser and the f_codes.
  // Reading it keeps FFmpeg's guess of the time increment's length in c.
  template <class IO>
  int walk(IO& io, Ctx& c) {
    int i, t = type - PT_I;
    if (!IO::W) clear();
    if (!io.bits(&t, VOP_TYPE_BITS)) return 0;
    type = PT_I + t;
    if (!io.ones(&mtb) || !io.marker() || !io.time_bits(c, type, &tib)) return 0;
    if (!io.bits(&tinc, tib) || !io.marker() || !io.bit(&coded)) return 0;
    if (!coded) return 1;
    if (c.dp && type != PT_B) return 0;  // (data partitioning: B-VOPs only)
    if (c.vop_has_rnd(type) && !io.bit(&rnd)) return 0;
    ncplx = c.vop_ncplx(type);
    for (i = 0; i < ncplx; i++) {
      int b = cplx[i] - '0';
      if (!io.bit(&b)) return 0;
      cplx[i] = (char)('0' + b);
    }
    if (io.short_of(DCTHR_BITS) || !io.bits(&dcthr, DCTHR_BITS)) return 0;
    if (c.interlaced && (!io.bit(&tff) || !io.bit(&alt))) return 0;
    if (c.vop_gmc(type) && !c.traj_walk(io, warp)) return 0;
    if (!io.bits(&quant, c.quant_precision) || !quant) return 0;
    if (type != PT_I && (!io.bits(&fcode, FCODE_BITS) || !fcode)) return 0;
    if (type == PT_B && (!io.bits(&bcode, FCODE_BITS) || !bcode)) return 0;
    return !io.over();
  }

  // Whether the VOP at p (n bytes from its start code on) is coded (vop_coded), as its header reads
  // with the VOL c has (and its guess of the time increment's length, kept in c); -1 if the header
  // doesn't parse as far
  static int coded_at(Ctx& c, pcU8 p, size_t n) {
    U8 b[VOP_PEEK + GB_PAD];
    GB gb;
    VopHdr h;
    Rd rd = {gb};
    const size_t k = n - SC_LEN < VOP_PEEK ? n - SC_LEN : VOP_PEEK;
    memset(b, 0, sizeof b);
    memcpy(b, p + SC_LEN, k);
    gb.init(b, k);
    h.coded = -1;
    h.walk(rd, c);  // (coded as far as it got: the rest of the header doesn't matter here)
    return h.coded;
  }
};

struct Vop {  // a VOP's state as a walk goes through it (the quantiser follows dquant)
  int type, gmc, fcode, bcode, qscale, dcthr, tib;

  void start(const Ctx& c, const VopHdr& h) {  // as its header has it
    type = h.type;
    gmc = c.vop_gmc(h.type);
    fcode = h.fcode == NA ? 1 : h.fcode;
    bcode = h.bcode == NA ? 1 : h.bcode;
    qscale = clip(h.quant, QS_MIN, QS_MAX);
    dcthr = dc_threshold[h.dcthr];
    tib = h.tib;
  }

  int prefix_length() const {  // zeros of a resync marker
    switch (type) {
    case PT_I: return RESYNC_ZEROS;
    case PT_P: case PT_S: return RESYNC_ZEROS + fcode - 1;
    default: { int m = fcode > bcode ? fcode : bcode; return RESYNC_ZEROS + max2(m, RESYNC_B_MIN) - 1; }
    }
  }

  // FFmpeg's mpeg4_is_resync on g, a copy of the reader: the number of the macroblock a resync
  // marker right after the current position announces (the number of macroblocks for the end of
  // the VOP), or 0. *nstuff: MCBPC stuffing codes FFmpeg skips first.
  int resync_ahead(const Ctx& c, GB g, pInt nstuff) const {
    constexpr int B = BYTE_BITS - 1;  // (a bit's place in its byte: & B)
    int bits_count = g.pos, w = (int)g.show(RESYNC_WIN);
    *nstuff = 0;
    while (w <= (int)BYTE_MASK) {  // (its first byte's bits 0)
      if (type == PT_B || (w >> (RESYNC_WIN - stuffing_len(type)) != 1)) break;
      g.skip(stuffing_len(type));
      bits_count += stuffing_len(type);
      w = (int)g.show(RESYNC_WIN);
      ++*nstuff;
    }
    if (bits_count + BYTE_BITS >= g.size) {  // the end: the usual stuffing up to the byte boundary?
      w >>= BYTE_BITS;
      w |= stuffing_bits(BYTE_BITS) >> (B - (bits_count & B));
      if (w == stuffing_bits(BYTE_BITS)) return c.mb_num;
    } else {
      static const U16 resync_prefix[BYTE_BITS] = { 0x7F00, 0x7E00, 0x7C00, 0x7800, 0x7000, 0x6000, 0x4000, 0x0000 };
      if (w == resync_prefix[bits_count & B]) {
        int len, mb_num;
        g.skip(1);
        g.align();
        for (len = 0; len < ZEROS_MAX; len++)
          if (g.get1()) break;
        mb_num = (int)g.get(c.mb_num_bits);
        if (!mb_num || mb_num > c.mb_num || g.pos + RESYNC_TAIL > g.size) mb_num = -1;
        if (len >= prefix_length()) return mb_num;
      }
    }
    return 0;
  }
};

struct VpHdr {
  int st, mbnum, quant, hec, mtb, tinc, type, dcthr, warp[2 * WARP_POINTS_MAX], fcode, bcode;

  void clear() { mtb = tinc = type = dcthr = fcode = bcode = NA; }  // (as VopHdr's)

  // A video packet's header after the macroblock before it (FFmpeg's
  // mpeg4_decode_video_packet_header): MCBPC stuffing (in an S-VOP, as FFmpeg skips it, there's
  // none), the stuffing up to the byte boundary, the resync marker, the macroblock's number, the
  // quantiser (0: as it was) and header_extension_code, after which the VOP's times, type, DC
  // threshold, trajectory and f_codes again. The quantiser in v follows it.
  template <class IO>
  int walk(IO& io, const Ctx& c, Vop& v) {
    int mk = 1;
    if (!IO::W) clear();
    if (v.type == PT_S ? st != 0 : !io.stuffing(&st, v.type)) return 0;
    io.byte_stuffing();
    if (io.short_of(VP_HDR_MIN)) return 0;
    if (!io.bits(&mk, v.prefix_length() + 1) || mk != 1) return 0;  // (its 0s, then a 1)
    if (!io.bits(&mbnum, c.mb_num_bits) || !mbnum || mbnum >= c.mb_num) return 0;
    if (!io.bits(&quant, c.quant_precision) || !io.bit(&hec)) return 0;
    if (quant) v.qscale = quant;
    v.qscale = clip(v.qscale, QS_MIN, QS_MAX);
    if (hec) {
      if (!io.ones(&mtb) || !io.marker() || !io.bits(&tinc, v.tib) || !io.marker()) return 0;
      if (!io.bits(&type, VOP_TYPE_BITS) || !io.bits(&dcthr, DCTHR_BITS)) return 0;
      if (v.gmc && !c.traj_walk(io, warp)) return 0;
      if (v.type != PT_I && !io.bits(&fcode, FCODE_BITS)) return 0;
      if (v.type == PT_B && !io.bits(&bcode, FCODE_BITS)) return 0;
    }
    return !io.over();
  }
};

struct Blk {
  int dc;          // intra DC difference (when the DC is coded separately)
  int n;           // events, 0 = not coded
  U8 run[BLK_COEFS], esc[BLK_COEFS];
  S16 level[BLK_COEFS];

  // A block (the k-th of its macroblock: 0-3 luma, 4-5 chroma): an intra DC coded apart (dcvlc) as
  // its size's code, the difference and a marker after a long one; then, if coded, its events up to
  // the last.
  template <class IO>
  int walk(IO& io, int k, int coded, int intra, int dcvlc) {
    const RL& rl = intra ? rl_intra : rl_inter;
    int i = -1, j, p0 = io.pos();
    if (!IO::W) n = dc = 0;
    if (intra && dcvlc) {
      int size = bitlen(dc);
      if (!io.vlc(&size, k < MB_LUMA ? vlc_dc_lum : vlc_dc_chrom) || size > DC_SIZE_MAX) return 0;
      if (size && (!io.xbits(&dc, size) || (size > DC_MARKER_SIZE && !io.marker()))) return 0;
      i = 0;
    }
    io.count(PB_DC, p0);
    p0 = io.pos();
    if (!coded) return 1;
    for (j = 0;; j++) {  // (i, the coefficient's position, keeps j within the block)
      int last = 0, r = 0, l = 0, mode = ESC_VLC;  // (the event's run and level)
      if (io.over()) return 0;
      if (IO::W) {
        last = j + 1 == n;
        r = run[j];
        l = level[j];
        mode = esc[j];
      }
      if (!rl.event_walk(io, &last, &r, &l, &mode)) return 0;
      i += r + 1;
      if (i > (last ? BLK_LAST : BLK_LAST - 1)) return 0;
      run[j] = (U8)r;
      level[j] = (S16)l;
      esc[j] = (U8)mode;
      if (last) {
        n = j + 1;
        io.count(PB_COEF, p0);
        return 1;
      }
    }
  }
};

#include "mb.inc"  // MB: a macroblock's record, and its walk
