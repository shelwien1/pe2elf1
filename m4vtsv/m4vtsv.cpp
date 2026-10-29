// m4vtsv - lossless converter between MPEG-4 Part 2 video elementary streams (.m4v: DivX 4/5/6,
// XviD, libavcodec mpeg4, ...) and TSV text listing their syntax elements
// Usage: m4vtsc c|d [options] input output
//   c    .m4v -> .tsv
//   d    .tsv -> .m4v, restoring the original stream byte for byte
//   -v   (c) afterwards convert the written TSV back in memory and compare it with the input
//   -q   no statistics
//
// The bitstream syntax follows m4vdec.cpp, a port of FFmpeg's MPEG-4 Part 2 decoder, so this file
// is licensed under the GNU Lesser General Public License 2.1 or later.
//
// c converts every VOP it turns into records back on the spot and compares the result with the
// original bytes. A VOP that doesn't come back identical, or that uses a feature listed as not
// supported below, is stored as raw hex instead, so the conversion is always lossless.
//
// TSV format (version 1): one record per line, fields separated by tabs, "-" for a field that
// doesn't apply, '#' starts a comment line. The stream is cut at start codes (00 00 01 xx).
//   H hex    bytes of a chunk that isn't a VOP: VOS/VO/VOL headers, user data, GOV, and whatever
//            precedes the first start code
//   R hex    bytes of a VOP kept as raw data
//            (H and R records hold at most 32 KiB; longer chunks continue on further lines)
// Any other VOP is a V record, one record per macroblock with K records for video packet
// headers, and a final T record:
//   V type mtb tib tinc coded rnd cplx dcthr tff alt warp quant fcode bcode
//       VOP header after the 00 00 01 B6 start code: type I/P/B/S; mtb modulo_time_base (number
//       of 1 bits); tib/tinc width and value of vop_time_increment; coded vop_coded (the fields
//       after it are left out when it is 0); rnd vop_rounding_type; cplx complexity estimation
//       bits (binary); dcthr intra_dc_vlc_thr; tff/alt top_field_first and alternate vertical
//       scan (interlaced VOLs); warp sprite trajectory "dx,dy,..." (GMC S-VOPs); quant;
//       fcode/bcode forward/backward f_code
//   K st mbnum quant hec mtb tinc type dcthr warp fcode bcode
//       video packet header: st MCBPC stuffing codes before its resync stuffing, then the
//       header fields (the ones after hec only when hec is 1)
//   i x y st ac dq dt b0 b1 b2 b3 b4 b5            macroblock of an I-VOP
//   p x y st sk t mc ac dq dt fp fs mv b0 .. b5    macroblock of a P- or S-VOP (ends after sk if skipped)
//   b x y sk mb t dq dt fp fs mv b0 .. b5          macroblock of a B-VOP (ends after sk if skipped,
//                                                  after mb if modb is 0)
//       x y position; st number of MCBPC stuffing codes before it; sk not_coded (b: no bits
//       because the co-located macroblock of the backward reference was skipped); t type
//       (p: 0 inter, 1 inter 4MV, 2 intra; b: 0 direct, 1 bidirectional, 2 backward, 3 forward);
//       mb modb (0: '1', 1: '01', 2: '00'); mc mcsel; ac ac_pred_flag; dq dquant (i/p: 0 none
//       or -2, -1, 1, 2; b: 0, -2, 2); dt dct_type; fp field_prediction; fs field selects as
//       bits (forward, then backward); mv motion vector differences "x,y,..." as coded, in
//       bitstream order (forward, then backward; delta vector for direct)
//       b0..b5 blocks: "-" when not coded, else the coefficient events "run:level" separated by
//       spaces, the last one being the last coefficient. An intra block whose DC is coded
//       separately (intra_dc_vlc) starts with the DC difference: "dc" or "dc|events". An event
//       that is escape-coded in another way than the default (the first escape mode of 1, 2, 3
//       that can code it) carries the mode: "run:levelE1".."E3".
//   T bits hex
//       what follows the last macroblock: the bits up to the next byte boundary (binary), then
//       the whole bytes (hex); normally just the stuffing
//
// Kept raw: VOPs using data partitioning (B-VOPs don't), non-rectangular shapes, static sprites,
// sprite brightness change, newpred, reduced resolution, scalability or the studio profile; VOPs
// that don't parse (damaged data); VOPs longer than 64 MiB, with more than 32 KiB after the last
// macroblock, or with more than 64 MiB of text.
//
// Build:
//   x86_64-w64-mingw32-g++ -O2 -s -static -o m4vtsv.exe m4vtsv.cpp
//   g++ -O2 -o m4vtsv m4vtsv.cpp
// Plain C (compiles as C or C++), no dependencies. No malloc: all buffers are static (about
// 200 MiB of .bss, touched as far as the data needs it).

#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef short S16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef long long S64;

#define NA (-1000)  // field not present
#define IBUF_SIZE (64u << 20)    // longest chunk handled at once (longer ones are kept raw)
#define TBUF_SIZE (64u << 20)    // text of one VOP
#define OBUF_SIZE (IBUF_SIZE + 4096)
#define IREAD (1u << 20)         // input read size
#define RAW_LINE (32u << 10)     // bytes per H/R record
#define LBUF_SIZE (1u << 20)     // longest TSV line d accepts

static inline int clipi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline int ilog2(U32 v) { int n = 0; while (v >>= 1) n++; return n; }
static inline int sign_extend(int val, int bits) { int sh = 32 - bits; return (int)((U32)val << sh) >> sh; }
static int bitlen(int v) { U32 a = v < 0 ? 0u - (U32)v : (U32)v; int n = 0; while (a) { n++; a >>= 1; } return n; }

//--- bit reader (from m4vdec; buffers carry 64 bytes of zero padding) -------------------------

typedef struct GB { const U8* buf; int size; int pos; } GB;  // size, pos in bits

static inline U32 gb_show(const GB* g, int n) {  // 0 <= n <= 32
  const U8* p = g->buf + (g->pos >> 3);
  U64 v = ((U64)p[0] << 56) | ((U64)p[1] << 48) | ((U64)p[2] << 40) | ((U64)p[3] << 32) |
          ((U64)p[4] << 24) | ((U64)p[5] << 16) | ((U64)p[6] << 8) | (U64)p[7];
  v <<= (g->pos & 7);
  return n ? (U32)(v >> (64 - n)) : 0;
}
static inline U32 gb_get(GB* g, int n) { U32 v = gb_show(g, n); g->pos += n; return v; }
static inline int gb_get1(GB* g) { return (int)gb_get(g, 1); }
static inline void gb_skip(GB* g, int n) { g->pos += n; }
static inline int gb_left(const GB* g) { return g->size - g->pos; }
static inline void gb_align(GB* g) { g->pos = (g->pos + 7) & ~7; }
static inline int gb_sget(GB* g, int n) { return sign_extend((int)gb_get(g, n), n); }
static inline int gb_xbits(GB* g, int n) {  // FFmpeg get_xbits: MSB set -> positive, else negative
  int v = (int)gb_get(g, n);
  return (v >> (n - 1)) ? v : v - (1 << n) + 1;
}

//--- bit writer ------------------------------------------------------------------------------

typedef struct PB { U8* buf; size_t cap, len; U64 acc; int n, err; } PB;  // n bits pending in acc

static void pb_init(PB* p, U8* buf, size_t cap) { memset(p, 0, sizeof(*p)); p->buf = buf; p->cap = cap; }
static void pb_put(PB* p, U32 v, int n) {  // 0 <= n <= 32
  if (n <= 0) return;
  p->acc = (p->acc << n) | (n < 32 ? v & ((1u << n) - 1) : v);
  for (p->n += n; p->n >= 8;) {
    p->n -= 8;
    if (p->len < p->cap) p->buf[p->len++] = (U8)(p->acc >> p->n);
    else p->err = 1;
  }
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

//--- tables (from FFmpeg via m4vdec: h263data.c, mpeg4data.h) ----------------------------------

static const U8 intra_mcbpc_code[9] = { 1, 1, 2, 3, 1, 1, 2, 3, 1 };
static const U8 intra_mcbpc_bits[9] = { 1, 3, 3, 3, 4, 6, 6, 6, 9 };
static const U8 inter_mcbpc_code[28] = { 1, 3, 2, 5,  3, 4, 3, 3,  3, 7, 6, 5,  4, 4, 3, 2,
                                         2, 5, 4, 5,  1, 0, 0, 0,  2, 12, 14, 15 };
static const U8 inter_mcbpc_bits[28] = { 1, 4, 4, 6,  5, 8, 8, 7,  3, 7, 7, 9,  6, 9, 9, 9,
                                         3, 7, 7, 8,  9, 0, 0, 0,  11, 13, 13, 13 };
static const U8 cbpy_tab[16][2] = {
  {  3, 4 }, { 5, 5 }, { 4, 5 }, { 9, 4 }, {  3, 5 }, { 7, 4 }, { 2, 6 },
  { 11, 4 }, { 2, 5 }, { 3, 6 }, { 5, 4 }, { 10, 4 }, { 4, 4 }, { 8, 4 },
  {  6, 4 }, { 3, 2 } };
static const U8 mv_tab[33][2] = {
  {  1,  1 }, {  1,  2 }, {  1,  3 }, {  1,  4 }, {  3,  6 }, {  5,  7 },
  {  4,  7 }, {  3,  7 }, { 11,  9 }, { 10,  9 }, {  9,  9 }, { 17, 10 },
  { 16, 10 }, { 15, 10 }, { 14, 10 }, { 13, 10 }, { 12, 10 }, { 11, 10 },
  { 10, 10 }, {  9, 10 }, {  8, 10 }, {  7, 10 }, {  6, 10 }, {  5, 10 },
  {  4, 10 }, {  7, 11 }, {  6, 11 }, {  5, 11 }, {  4, 11 }, {  3, 11 },
  {  2, 11 }, {  3, 12 }, {  2, 12 } };
static const U16 inter_vlc[103][2] = {
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
static const S8 inter_level[102] = {
  1,  2,  3,  4, 5, 6, 7, 8,  9, 10, 11, 12, 1, 2, 3, 4,  5,  6,  1,  2, 3, 4, 1, 2,  3,  1,  2,  3, 1, 2, 3, 1,
  2,  3,  1,  2, 1, 2, 1, 2,  1,  2,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  2, 3, 1, 2, 1,
  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,  1,  1,  1,  1, 1, 1, 1, 1,
  1,  1,  1,  1, 1, 1 };
static const S8 inter_run[102] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,
   3,  4,  4,  4,  5,  5,  5,  6,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,  0,  0,  0,  1,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, 40 };
static const U16 intra_vlc[103][2] = {
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
static const S8 intra_level[102] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  1,  2,  3,  4,  5,  1,  2,  3,  4,  1,  2,
   3,  1,  2,  3,  1,  2,  3,  1,  2,  3,  1,  2,  1,  2,  1,  1,  1,  1,  1,  1,  2,  3,  4,  5,
   6,  7,  8,  1,  2,  3,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1 };
static const S8 intra_run[102] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,
   4,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  9,  9, 10, 11, 12, 13, 14,  0,  0,  0,  0,  0,
   0,  0,  0,  1,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20 };
static const U8 DCtab_lum[13][2] = {
  {3,3}, {3,2}, {2,2}, {2,3}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, {1,10}, {1,11} };
static const U8 DCtab_chrom[13][2] = {
  {3,2}, {2,2}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, {1,10}, {1,11}, {1,12} };
static const U8 sprite_trajectory_lens[15] = { 2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
static const U8 mb_type_b_tab[4][2] = { {1, 1}, {1, 2}, {1, 3}, {1, 4} };
static const U8 dc_threshold[8] = { 99, 13, 15, 17, 19, 21, 23, 0 };
static const S8 quant_tab[4] = { -1, -2, 1, 2 };
static U32 sprite_trajectory_code[15];

typedef struct RL {
  int n, last;
  const U16 (*vlc)[2];
  const S8* run;
  const S8* level;
  U8 max_level[2][65], max_run[2][65];
  VLC v;
  S16 index[2][64][65];  // [last][run][level] -> symbol, -1 if there is none
} RL;

static VLCE rl_intra_tab[1 << 12], rl_inter_tab[1 << 12];
static RL rl_intra = { 102, 67, intra_vlc, intra_run, intra_level, {{0}}, {{0}}, { rl_intra_tab, 12 }, {{{0}}} };
static RL rl_inter = { 102, 58, inter_vlc, inter_run, inter_level, {{0}}, {{0}}, { rl_inter_tab, 12 }, {{{0}}} };
VLC_DEF(vlc_intra_mcbpc, 9);
VLC_DEF(vlc_inter_mcbpc, 13);
VLC_DEF(vlc_cbpy, 6);
VLC_DEF(vlc_mv, 12);
VLC_DEF(vlc_dc_lum, 11);
VLC_DEF(vlc_dc_chrom, 12);
VLC_DEF(vlc_mb_type_b, 4);
VLC_DEF(vlc_sprite_traj, 12);

static void rl_init(RL* rl) {
  int last, i;
  U32 codes[103]; U8 lens[103];
  memset(rl->max_level, 0, sizeof(rl->max_level));
  memset(rl->max_run, 0, sizeof(rl->max_run));
  memset(rl->index, 0xFF, sizeof(rl->index));
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

static void tables_init(void) {
  U32 c[33]; U8 l[33]; int i;
  U32 code = 0;
  for (i = 0; i < 9; i++) { c[i] = intra_mcbpc_code[i]; l[i] = intra_mcbpc_bits[i]; }
  vlc_build(&vlc_intra_mcbpc, 9, c, l);
  {
    U32 c2[28]; U8 l2[28];
    for (i = 0; i < 28; i++) { c2[i] = inter_mcbpc_code[i]; l2[i] = inter_mcbpc_bits[i]; }
    vlc_build(&vlc_inter_mcbpc, 28, c2, l2);
  }
  for (i = 0; i < 16; i++) { c[i] = cbpy_tab[i][0]; l[i] = cbpy_tab[i][1]; }
  vlc_build(&vlc_cbpy, 16, c, l);
  for (i = 0; i < 33; i++) { c[i] = mv_tab[i][0]; l[i] = mv_tab[i][1]; }
  vlc_build(&vlc_mv, 33, c, l);
  for (i = 0; i < 13; i++) { c[i] = DCtab_lum[i][0]; l[i] = DCtab_lum[i][1]; }
  vlc_build(&vlc_dc_lum, 13, c, l);
  for (i = 0; i < 13; i++) { c[i] = DCtab_chrom[i][0]; l[i] = DCtab_chrom[i][1]; }
  vlc_build(&vlc_dc_chrom, 13, c, l);
  for (i = 0; i < 4; i++) { c[i] = mb_type_b_tab[i][0]; l[i] = mb_type_b_tab[i][1]; }
  vlc_build(&vlc_mb_type_b, 4, c, l);
  for (i = 0; i < 15; i++) {  // canonical codes from lengths, in listed order (as ff_vlc_init_from_lengths)
    l[i] = sprite_trajectory_lens[i];
    c[i] = sprite_trajectory_code[i] = code >> (32 - l[i]);
    code += 1u << (32 - l[i]);
  }
  vlc_build(&vlc_sprite_traj, 15, c, l);
  rl_init(&rl_intra);
  rl_init(&rl_inter);
}

// Length of an event (last, run, |level|) coded in escape mode 0 (plain VLC) .. 3, 0 if that mode
// can't code it.
static int esc_bits(const RL* rl, int mode, int last, int run, int alevel) {
  int s, k;
  if (run < 0 || run > 63) return 0;
  switch (mode) {
  case 0:
    if (alevel < 1 || alevel > 64 || (s = rl->index[last][run][alevel]) < 0) return 0;
    return rl->vlc[s][1] + 1;
  case 1:
    k = alevel - rl->max_level[last][run];
    if (k < 1 || k > 64 || (s = rl->index[last][run][k]) < 0) return 0;
    return 7 + 1 + rl->vlc[s][1] + 1;
  case 2:
    if (alevel < 1 || alevel > 64) return 0;
    k = run - rl->max_run[last][alevel] - 1;
    if (k < 0 || (s = rl->index[last][k][alevel]) < 0) return 0;
    return 7 + 2 + rl->vlc[s][1] + 1;
  default:
    return alevel <= 2048 ? 30 : 0;
  }
}

// default escape mode: plain VLC when possible, else the first escape mode that can code it
static int esc_default(const RL* rl, int last, int run, int alevel) {
  int m;
  for (m = 0; m < 3; m++)
    if (esc_bits(rl, m, last, run, alevel)) return m;
  return 3;
}

//--- stream context ----------------------------------------------------------------------------

enum { PT_I = 1, PT_P = 2, PT_B = 3, PT_S = 4 };  // same values as AVPictureType (FFmpeg relies on them)
enum { STATIC_SPRITE = 1, GMC_SPRITE = 2 };
#define MAX_MBS (512 * 512)  // VOL width and height are 13-bit fields

typedef struct Ctx {
  int have_vol;
  const char* unsupported;   // why VOPs of the current VOL are kept raw
  int dp;                    // data partitioning: only B-VOPs are supported
  int width, height, mb_w, mb_h, mb_num, mb_num_bits;
  int interlaced, sprite_usage, warp_points, quant_precision, time_inc_bits;
  int cplx_i, cplx_p, cplx_b;
  int divx_version, divx_build, xvid_build;
} Ctx;

// c: skip flags of the last anchor VOP and of the one being parsed (a B-VOP macroblock whose
// co-located macroblock in the backward reference was skipped has no bits)
static U8 skip_ref[MAX_MBS + 1], skip_cur[MAX_MBS + 1];

static void ctx_init(Ctx* c) {
  memset(c, 0, sizeof(*c));
  c->divx_version = c->divx_build = c->xvid_build = -1;
  c->time_inc_bits = 4;
  c->quant_precision = 5;
}

// VOL header, as m4vdec's decode_vol_header: records what the VOP syntax depends on
static void parse_vol(Ctx* c, GB* gb) {
  int vo_ver_id, vo_type, w, h, time_res, k;
  c->have_vol = 1;
  c->unsupported = 0;
  c->dp = 0;
  gb_skip(gb, 1);                       // random access
  vo_type = (int)gb_get(gb, 8);
  if (vo_type == 14 || vo_type == 15) { c->unsupported = "studio profile"; return; }
  if (gb_get1(gb)) {                    // is_ol_id
    vo_ver_id = (int)gb_get(gb, 4);
    gb_skip(gb, 3);
  } else
    vo_ver_id = 1;
  if (gb_get(gb, 4) == 15) gb_skip(gb, 16);   // aspect_ratio_info, extended PAR
  if (gb_get1(gb)) {                    // vol_control_parameters
    gb_skip(gb, 3);                     // chroma_format, low_delay
    if (gb_get1(gb)) gb_skip(gb, 15 + 1 + 15 + 1 + 15 + 1 + 3 + 11 + 1 + 15 + 1);
  }
  if (gb_get(gb, 2)) { c->unsupported = "non-rectangular shapes"; return; }
  gb_skip(gb, 1);
  time_res = (int)gb_get(gb, 16);
  if (!time_res) { c->unsupported = "a time resolution of 0"; return; }
  c->time_inc_bits = ilog2(time_res - 1) + 1;
  if (c->time_inc_bits < 1) c->time_inc_bits = 1;
  gb_skip(gb, 1);
  if (gb_get1(gb)) gb_skip(gb, c->time_inc_bits);  // fixed_vop_time_increment
  gb_skip(gb, 1);
  w = (int)gb_get(gb, 13);
  gb_skip(gb, 1);
  h = (int)gb_get(gb, 13);
  gb_skip(gb, 1);
  if (w && h) { c->width = w; c->height = h; }
  c->interlaced = gb_get1(gb);
  gb_skip(gb, 1);                       // obmc disable
  c->sprite_usage = vo_ver_id == 1 ? gb_get1(gb) : (int)gb_get(gb, 2);
  c->warp_points = 0;
  if (c->sprite_usage == STATIC_SPRITE) c->unsupported = "static sprites";
  if (c->sprite_usage == STATIC_SPRITE || c->sprite_usage == GMC_SPRITE) {
    if (c->sprite_usage == STATIC_SPRITE) gb_skip(gb, 4 * 14);
    c->warp_points = (int)gb_get(gb, 6);
    if (c->warp_points > 3) { c->warp_points = 0; c->unsupported = "more than 3 sprite warping points"; }
    gb_skip(gb, 2);                     // warping accuracy
    if (gb_get1(gb)) c->unsupported = "sprite brightness change";
    if (c->sprite_usage == STATIC_SPRITE) gb_skip(gb, 1);
  }
  if (gb_get1(gb)) {                    // not_8_bit
    c->quant_precision = (int)gb_get(gb, 4);
    gb_skip(gb, 4);                     // bits_per_pixel
    if (c->quant_precision < 3 || c->quant_precision > 9) c->quant_precision = 5;
  } else
    c->quant_precision = 5;
  if (gb_get1(gb)) {                    // mpeg_quant: matrices
    for (k = 0; k < 2; k++) {
      if (gb_get1(gb)) {
        int i;
        for (i = 0; i < 64; i++) {
          if (gb_left(gb) < 8) { c->unsupported = "a damaged VOL header"; return; }
          if (!gb_get(gb, 8)) break;
        }
      }
    }
  }
  if (vo_ver_id != 1) gb_skip(gb, 1);   // quarter_sample
  if (gb_left(gb) < 4) { c->unsupported = "a damaged VOL header"; return; }
  c->cplx_i = c->cplx_p = c->cplx_b = 0;
  if (!gb_get1(gb)) {                   // complexity estimation
    int pos = gb->pos, method = (int)gb_get(gb, 2);
    if (method < 2) {
      if (!gb_get1(gb)) { for (k = 0; k < 6; k++) c->cplx_i += 8 * gb_get1(gb); }
      if (!gb_get1(gb)) {
        c->cplx_i += 8 * gb_get1(gb); c->cplx_p += 8 * gb_get1(gb);
        c->cplx_p += 8 * gb_get1(gb); c->cplx_i += 8 * gb_get1(gb);
      }
      if (!gb_get1(gb)) { gb->pos = pos; c->cplx_i = c->cplx_p = c->cplx_b = 0; goto no_cplx; }
      if (!gb_get1(gb)) {
        c->cplx_i += 8 * gb_get1(gb); c->cplx_i += 8 * gb_get1(gb);
        c->cplx_i += 8 * gb_get1(gb); c->cplx_i += 4 * gb_get1(gb);
      }
      if (!gb_get1(gb)) {
        c->cplx_p += 8 * gb_get1(gb); c->cplx_p += 8 * gb_get1(gb); c->cplx_b += 8 * gb_get1(gb);
        c->cplx_p += 8 * gb_get1(gb); c->cplx_p += 8 * gb_get1(gb); c->cplx_p += 8 * gb_get1(gb);
      }
      if (!gb_get1(gb)) { gb->pos = pos; c->cplx_i = c->cplx_p = c->cplx_b = 0; goto no_cplx; }
      if (method == 1) { c->cplx_i += 8 * gb_get1(gb); c->cplx_p += 8 * gb_get1(gb); }
    }
  }
no_cplx:
  gb_skip(gb, 1);                       // resync_marker_disable
  if ((c->dp = gb_get1(gb))) gb_skip(gb, 1);   // data_partitioned, reversible_vlc
  if (vo_ver_id != 1) {
    if (gb_get1(gb)) { c->unsupported = "newpred"; gb_skip(gb, 3); }
    if (gb_get1(gb)) c->unsupported = "reduced resolution VOPs";
  }
  if (gb_get1(gb)) {                    // scalability
    int hn, hm, vn, vm;
    gb_skip(gb, 1 + 4 + 1);
    hn = (int)gb_get(gb, 5); hm = (int)gb_get(gb, 5);
    vn = (int)gb_get(gb, 5); vm = (int)gb_get(gb, 5);
    if (hn && hm && vn && vm) c->unsupported = "scalability";
  }
  c->mb_w = (c->width + 15) >> 4;
  c->mb_h = (c->height + 15) >> 4;
  c->mb_num = c->mb_w * c->mb_h;
  if (!c->mb_num) c->unsupported = "a frame size of 0";
  c->mb_num_bits = c->mb_num > 1 ? ilog2(c->mb_num - 1) + 1 : 1;
}

// user data: encoder identification (m4vdec's decode_user_data, the parts the syntax depends on)
static void parse_user_data(Ctx* c, GB* gb) {
  char buf[256], last;
  int i, e, ver = 0, build = 0;
  for (i = 0; i < 255 && gb->pos < gb->size; i++) {
    if (gb_show(gb, 23) == 0) break;
    buf[i] = (char)gb_get(gb, 8);
  }
  buf[i] = 0;
  e = sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
  if (e < 2) e = sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
  if (e >= 2) { c->divx_version = ver; c->divx_build = build; }
  if (sscanf(buf, "XviD%d", &build) == 1) c->xvid_build = build;
}

static U8 hbuf[(32 << 10) + 64];

// a chunk (the first 32 KiB of it) that isn't a VOP: VOL headers and user data matter
static void parse_header_chunk(Ctx* c, const U8* p, size_t n) {
  GB gb;
  if (n < 4 || p[0] || p[1] || p[2] != 1) return;
  if (n > (32 << 10)) n = 32 << 10;
  memcpy(hbuf, p, n);
  memset(hbuf + n, 0, 64);
  gb.buf = hbuf + 4;
  gb.size = (int)(n - 4) * 8;
  gb.pos = 0;
  if (p[3] >= 0x20 && p[3] <= 0x2F) parse_vol(c, &gb);
  else if (p[3] == 0xB2) parse_user_data(c, &gb);
}

static int divx413(const Ctx* c) { return c->divx_version == 500 && c->divx_build == 413 && c->xvid_build < 0; }

//--- records ------------------------------------------------------------------------------------

typedef struct VopHdr {
  int type, mtb, tib, tinc, coded, rnd, dcthr, tff, alt, quant, fcode, bcode;
  int warp[6];
  int ncplx; char cplx[264];
} VopHdr;

typedef struct VpHdr { int st, mbnum, quant, hec, mtb, tinc, type, dcthr, warp[6], fcode, bcode; } VpHdr;

typedef struct Blk {
  int dc;          // intra DC difference (when the DC is coded separately)
  int n;           // events, 0 = not coded
  U8 run[64], esc[64];
  S16 level[64];
} Blk;

typedef struct MB {
  int x, y, st, sk, mb, t, mc, ac, dq, dt, fp;
  int nfs, fs;     // field selects: count, bits (first one in the highest bit)
  int nmv, mv[8];
  Blk b[6];
} MB;

typedef struct Vop {  // state shared by the parsing and the writing direction
  int type, gmc, fcode, bcode, qscale, dcthr, tib;
} Vop;

static const char vop_letters[] = "IPBS";
static int vop_letter(int type) { return vop_letters[type - PT_I]; }
static int vop_type_of(const char* s, const char* e) {  // "I".."S" -> PT_I..PT_S, 0 if not one
  const char* t;
  if (e - s != 1 || !(t = strchr(vop_letters, *s)) || !*t) return 0;
  return (int)(t - vop_letters) + PT_I;
}
static int fwd_dir(int t) { return t == 1 || t == 3; }   // B-VOP macroblock types using the forward / backward reference
static int bwd_dir(int t) { return t == 1 || t == 2; }
static int mv_count(int t, int fp) { return (fwd_dir(t) + bwd_dir(t)) * (fp == 1 ? 2 : 1); }  // B-VOP, non-direct

static int prefix_length(const Vop* v) {  // zeros of a resync marker
  switch (v->type) {
  case PT_I: return 16;
  case PT_P: case PT_S: return v->fcode + 15;
  default: { int m = v->fcode > v->bcode ? v->fcode : v->bcode; return (m > 2 ? m : 2) + 15; }
  }
}

//--- parsing (c) ---------------------------------------------------------------------------------

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
  h->type = (int)gb_get(gb, 2) + PT_I;
  h->mtb = 0;
  while (gb_get1(gb)) if (++h->mtb > 1000 || gb_left(gb) <= 0) return -1;
  if (!gb_get1(gb)) return -1;          // marker
  if (c->time_inc_bits == 0 || !(gb_show(gb, c->time_inc_bits + 1) & 1)) {  // FFmpeg's guess
    for (c->time_inc_bits = 1; c->time_inc_bits < 16; c->time_inc_bits++) {
      if (h->type == PT_P || (h->type == PT_S && c->sprite_usage == GMC_SPRITE)) {
        if ((gb_show(gb, c->time_inc_bits + 6) & 0x37) == 0x30) break;
      } else if ((gb_show(gb, c->time_inc_bits + 5) & 0x1F) == 0x18)
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
  if (gb_left(gb) < 3) return -1;
  h->dcthr = (int)gb_get(gb, 3);
  if (c->interlaced) { h->tff = gb_get1(gb); h->alt = gb_get1(gb); }
  if (h->type == PT_S && c->sprite_usage == GMC_SPRITE && get_traj(c, gb, h->warp) < 0) return -1;
  h->quant = (int)gb_get(gb, c->quant_precision);
  if (!h->quant) return -1;
  if (h->type != PT_I && !(h->fcode = (int)gb_get(gb, 3))) return -1;
  if (h->type == PT_B && !(h->bcode = (int)gb_get(gb, 3))) return -1;
  return gb_left(gb) < 0 ? -1 : 0;
}

static int parse_block(GB* gb, Blk* b, int n, int coded, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i = -1;
  b->n = 0;
  b->dc = 0;
  if (intra && dcvlc) {
    int size = vlc_get(gb, n < 4 ? &vlc_dc_lum : &vlc_dc_chrom);
    if (size < 0 || size > 9) return -1;
    if (size) {
      b->dc = gb_xbits(gb, size);
      if (size > 8 && !gb_get1(gb)) return -1;   // marker
    }
    i = 0;
  }
  if (!coded) return 0;
  for (;;) {
    int sym, run, level, last, mode = 0;
    if (gb->pos > gb->size) return -1;
    sym = vlc_get(gb, &rl->v);
    if (sym < 0) return -1;
    if (sym == rl->n) {                 // escape
      U32 cache = gb_show(gb, 2);
      if (cache & 2) {
        gb_skip(gb, 2);
        if (cache & 1) {                // mode 3: fixed length
          last = gb_get1(gb);
          run = (int)gb_get(gb, 6);
          if (!gb_get1(gb)) return -1;  // marker
          level = gb_sget(gb, 12);
          if (!gb_get1(gb)) return -1;  // marker
          mode = 3;
        } else {                        // mode 2: run offset
          sym = vlc_get(gb, &rl->v);
          if (sym < 0 || sym == rl->n) return -1;
          last = sym >= rl->last;
          level = rl->level[sym];
          run = rl->run[sym] + rl->max_run[last][level] + 1;
          if (gb_get1(gb)) level = -level;
          mode = 2;
        }
      } else {                          // mode 1: level offset
        gb_skip(gb, 1);
        sym = vlc_get(gb, &rl->v);
        if (sym < 0 || sym == rl->n) return -1;
        last = sym >= rl->last;
        run = rl->run[sym];
        level = rl->level[sym] + rl->max_level[last][run];
        if (gb_get1(gb)) level = -level;
        mode = 1;
      }
    } else {
      last = sym >= rl->last;
      run = rl->run[sym];
      level = rl->level[sym];
      if (gb_get1(gb)) level = -level;
    }
    i += run + 1;
    if (i > (last ? 63 : 62)) return -1;
    b->run[b->n] = (U8)run;
    b->level[b->n] = (S16)level;
    b->esc[b->n] = (U8)mode;
    b->n++;
    if (last) return 0;
  }
}

static int parse_blocks(GB* gb, MB* m, int cbp, int intra, int dcvlc) {
  int i;
  for (i = 0; i < 6; i++, cbp += cbp)
    if (parse_block(gb, &m->b[i], i, cbp & 32, intra, dcvlc) < 0) return -1;
  return 0;
}

static int parse_intra(const Ctx* c, GB* gb, Vop* v, MB* m, int cbpc, int dquant) {
  int cbpy, dcvlc;
  m->ac = gb_get1(gb);
  if ((cbpy = vlc_get(gb, &vlc_cbpy)) < 0) return -1;
  dcvlc = v->qscale < v->dcthr;
  m->dq = 0;
  if (dquant) { m->dq = quant_tab[gb_get(gb, 2)]; v->qscale = clipi(v->qscale + m->dq, 1, 31); }
  m->dt = c->interlaced ? gb_get1(gb) : NA;
  return parse_blocks(gb, m, (cbpc & 3) | (cbpy << 2), 1, dcvlc);
}

static int parse_mvs(GB* gb, MB* m, int n, int f_code) {
  int i;
  for (i = 0; i < 2 * n; i++, m->nmv++)
    if (get_mv(gb, f_code, &m->mv[m->nmv]) < 0) return -1;
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
      if (cbpc != 8) break;
      if (++m->st > 1000) return -1;
    }
    m->t = 2;
    return parse_intra(c, gb, v, m, cbpc, cbpc & 4);
  }
  if (v->type != PT_B) {
    for (;;) {
      if (gb_get1(gb)) { m->sk = 1; return 0; }
      if ((cbpc = vlc_get(gb, &vlc_inter_mcbpc)) < 0) return -1;
      if (cbpc != 20) break;
      if (++m->st > 1000) return -1;
    }
    if (cbpc & 4) { m->t = 2; return parse_intra(c, gb, v, m, cbpc, cbpc & 8); }
    m->t = (cbpc & 16) ? 1 : 0;
    if (v->gmc && !m->t) m->mc = gb_get1(gb);
    if ((cbpy = vlc_get(gb, &vlc_cbpy)) < 0) return -1;
    cbp = (cbpc & 3) | ((cbpy ^ 15) << 2);
    m->dq = 0;
    if (cbpc & 8) { m->dq = quant_tab[gb_get(gb, 2)]; v->qscale = clipi(v->qscale + m->dq, 1, 31); }
    if (c->interlaced && cbp) m->dt = gb_get1(gb);
    if (m->t == 1) {
      if (parse_mvs(gb, m, 4, v->fcode) < 0) return -1;
    } else if (m->mc != 1) {
      if (c->interlaced) m->fp = gb_get1(gb);
      if (m->fp == 1) { m->nfs = 2; m->fs = (int)gb_get(gb, 2); }
      if (parse_mvs(gb, m, m->fp == 1 ? 2 : 1, v->fcode) < 0) return -1;
    }
    return parse_blocks(gb, m, cbp, 0, 0);
  }
  // B-VOP
  if (skip_ref[idx]) { m->sk = 1; return 0; }
  if (gb_get1(gb)) { m->mb = 0; m->t = 0; return parse_blocks(gb, m, 0, 0, 0); }
  m->mb = gb_get1(gb) ? 1 : 2;
  if ((m->t = vlc_get(gb, &vlc_mb_type_b)) < 0) return -1;
  cbp = m->mb == 2 ? (int)gb_get(gb, 6) : 0;
  if (m->t && cbp) {
    m->dq = gb_get1(gb) ? (gb_get1(gb) ? 2 : -2) : 0;
    v->qscale = clipi(v->qscale + m->dq, 1, 31);
  }
  if (c->interlaced) {
    if (cbp) m->dt = gb_get1(gb);
    if (m->t) {
      m->fp = gb_get1(gb);
      if (m->fp == 1) { m->nfs = 2 * (fwd_dir(m->t) + bwd_dir(m->t)); m->fs = (int)gb_get(gb, m->nfs); }
    }
  }
  if (m->t) {
    const int n = m->fp == 1 ? 2 : 1;
    if (fwd_dir(m->t) && parse_mvs(gb, m, n, v->fcode) < 0) return -1;
    if (bwd_dir(m->t) && parse_mvs(gb, m, n, v->bcode) < 0) return -1;
  } else if (parse_mvs(gb, m, 1, 1) < 0)
    return -1;
  return parse_blocks(gb, m, cbp, 0, 0);
}

// FFmpeg's mpeg4_is_resync on a copy of the reader: the number of the macroblock a resync marker
// right after the current position announces (the number of macroblocks for the end of the VOP),
// or 0. *nstuff: MCBPC stuffing codes FFmpeg skips first.
static int resync_ahead(const Ctx* c, const GB* gb0, const Vop* v, int* nstuff) {
  GB g = *gb0, *gb = &g;
  int bits_count = gb->pos, w = (int)gb_show(gb, 16);
  *nstuff = 0;
  while (w <= 0xFF) {
    if (v->type == PT_B || (w >> (8 - v->type) != 1)) break;
    gb_skip(gb, 8 + v->type);
    bits_count += 8 + v->type;
    w = (int)gb_show(gb, 16);
    ++*nstuff;
  }
  if (bits_count + 8 >= gb->size) {
    w >>= 8;
    w |= 0x7F >> (7 - (bits_count & 7));
    if (w == 0x7F) return c->mb_num;
  } else {
    static const U16 resync_prefix[8] = { 0x7F00, 0x7E00, 0x7C00, 0x7800, 0x7000, 0x6000, 0x4000, 0x0000 };
    if (w == resync_prefix[bits_count & 7]) {
      int len, mb_num;
      gb_skip(gb, 1);
      gb_align(gb);
      for (len = 0; len < 32; len++)
        if (gb_get1(gb)) break;
      mb_num = (int)gb_get(gb, c->mb_num_bits);
      if (!mb_num || mb_num > c->mb_num || gb->pos + 6 > gb->size) mb_num = -1;
      if (len >= prefix_length(v)) return mb_num;
    }
  }
  return 0;
}

static int parse_vp(const Ctx* c, GB* gb, Vop* v, VpHdr* k) {
  int len;
  if (gb->pos > gb->size - 20) return -1;
  for (len = 0; len < 32; len++)
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
    while (gb_get1(gb)) if (++k->mtb > 1000 || gb_left(gb) <= 0) return -1;
    if (!gb_get1(gb)) return -1;        // marker
    k->tinc = (int)gb_get(gb, v->tib);
    if (!gb_get1(gb)) return -1;        // marker
    k->type = (int)gb_get(gb, 2);
    k->dcthr = (int)gb_get(gb, 3);
    if (v->gmc && get_traj(c, gb, k->warp) < 0) return -1;
    if (v->type != PT_I) k->fcode = (int)gb_get(gb, 3);
    if (v->type == PT_B) k->bcode = (int)gb_get(gb, 3);
  }
  return gb_left(gb) < 0 ? -1 : 0;
}

//--- text output (c) -----------------------------------------------------------------------------

#define LINE_MAX_LEN 8192  // longest record a VOP line can need

typedef struct Txt { char *p, *end; int full; } Txt;
static U32 n_events, n_esc_marked;  // coefficient events written, those with an escape mode noted

static int tx_room(Txt* t) {  // before each line
  if (t->full || t->end - t->p < LINE_MAX_LEN) t->full = 1;
  return !t->full;
}
static void tx_c(Txt* t, char c) { *t->p++ = c; }
static void tx_i(Txt* t, int v) {
  char b[12];
  int n = 0;
  U32 u = v < 0 ? 0u - (U32)v : (U32)v;
  if (v < 0) *t->p++ = '-';
  do b[n++] = (char)('0' + u % 10); while (u /= 10);
  while (n) *t->p++ = b[--n];
}
static void tx_f(Txt* t, int v) { *t->p++ = '\t'; if (v == NA) *t->p++ = '-'; else tx_i(t, v); }
static void tx_bits(Txt* t, int v, int n) {  // n bits as binary, "-" for none
  *t->p++ = '\t';
  if (!n) *t->p++ = '-';
  while (n--) *t->p++ = (char)('0' + ((v >> n) & 1));
}
static void tx_list(Txt* t, const int* a, int n) {
  int i;
  *t->p++ = '\t';
  if (!n) *t->p++ = '-';
  for (i = 0; i < n; i++) { if (i) *t->p++ = ','; tx_i(t, a[i]); }
}

static void fmt_block(Txt* t, const Blk* b, int dcvlc, int intra) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i;
  *t->p++ = '\t';
  if (dcvlc) {
    tx_i(t, b->dc);
    if (b->n) *t->p++ = '|';
  } else if (!b->n)
    *t->p++ = '-';
  for (i = 0; i < b->n; i++) {
    const int level = b->level[i], last = i + 1 == b->n;
    if (i) *t->p++ = ' ';
    tx_i(t, b->run[i]);
    *t->p++ = ':';
    tx_i(t, level);
    n_events++;
    if (b->esc[i] != esc_default(rl, last, b->run[i], level < 0 ? -level : level)) {
      n_esc_marked++;
      *t->p++ = 'E';
      *t->p++ = (char)('0' + b->esc[i]);
    }
  }
}

static void fmt_vop_hdr(Txt* t, const Ctx* c, const VopHdr* h) {
  tx_room(t);
  if (t->full) return;
  tx_c(t, 'V');
  tx_c(t, '\t');
  tx_c(t, (char)vop_letter(h->type));
  tx_f(t, h->mtb);
  tx_f(t, h->tib);
  tx_f(t, h->tinc);
  tx_f(t, h->coded);
  if (h->coded) {
    int i;
    tx_f(t, h->rnd);
    tx_c(t, '\t');
    if (!h->ncplx) tx_c(t, '-');
    for (i = 0; i < h->ncplx; i++) tx_c(t, h->cplx[i]);
    tx_f(t, h->dcthr);
    tx_f(t, h->tff);
    tx_f(t, h->alt);
    tx_list(t, h->warp, h->type == PT_S && c->sprite_usage == GMC_SPRITE ? 2 * c->warp_points : 0);
    tx_f(t, h->quant);
    tx_f(t, h->fcode);
    tx_f(t, h->bcode);
  }
  tx_c(t, '\n');
}

static void fmt_vp(Txt* t, const Vop* v, const Ctx* c, const VpHdr* k) {
  if (!tx_room(t)) return;
  tx_c(t, 'K');
  tx_f(t, k->st);
  tx_f(t, k->mbnum);
  tx_f(t, k->quant);
  tx_f(t, k->hec);
  if (k->hec) {
    tx_f(t, k->mtb);
    tx_f(t, k->tinc);
    tx_c(t, '\t');
    tx_c(t, (char)vop_letter(k->type + PT_I));
    tx_f(t, k->dcthr);
    tx_list(t, k->warp, v->gmc ? 2 * c->warp_points : 0);
    tx_f(t, k->fcode);
    tx_f(t, k->bcode);
  }
  tx_c(t, '\n');
}

static void fmt_mb(Txt* t, const Vop* v, const MB* m, int dcvlc) {
  int i;
  if (!tx_room(t)) return;
  tx_c(t, v->type == PT_I ? 'i' : v->type == PT_B ? 'b' : 'p');
  tx_f(t, m->x);
  tx_f(t, m->y);
  if (v->type == PT_I) {
    tx_f(t, m->st);
    tx_f(t, m->ac);
    tx_f(t, m->dq);
    tx_f(t, m->dt);
  } else if (v->type == PT_B) {
    tx_f(t, m->sk);
    if (m->sk) { tx_c(t, '\n'); return; }
    tx_f(t, m->mb);
    if (!m->mb) { tx_c(t, '\n'); return; }
    tx_f(t, m->t);
    tx_f(t, m->dq);
    tx_f(t, m->dt);
    tx_f(t, m->fp);
    tx_bits(t, m->fs, m->nfs);
    tx_list(t, m->mv, m->nmv);
  } else {
    tx_f(t, m->st);
    tx_f(t, m->sk);
    if (m->sk) { tx_c(t, '\n'); return; }
    tx_f(t, m->t);
    tx_f(t, m->mc);
    tx_f(t, m->ac);
    tx_f(t, m->dq);
    tx_f(t, m->dt);
    tx_f(t, m->fp);
    tx_bits(t, m->fs, m->nfs);
    tx_list(t, m->mv, m->nmv);
  }
  {
    const int intra = v->type != PT_B && m->t == 2;
    for (i = 0; i < 6; i++) fmt_block(t, &m->b[i], intra && dcvlc, intra);
  }
  tx_c(t, '\n');
}

static void fmt_tail(Txt* t, GB* gb) {  // the rest of the VOP
  const U8* p;
  const U8* e;
  if (!tx_room(t)) return;
  tx_c(t, 'T');
  tx_c(t, '\t');
  if (!(gb->pos & 7)) tx_c(t, '-');
  while (gb->pos & 7) tx_c(t, (char)('0' + gb_get1(gb)));
  tx_c(t, '\t');
  p = gb->buf + (gb->pos >> 3);
  e = gb->buf + (gb->size >> 3);
  if (p >= e) tx_c(t, '-');
  if (e - p > (long)RAW_LINE || (size_t)(e - p) * 2 + 2 > (size_t)(t->end - t->p)) { t->full = 1; return; }
  for (; p < e; p++) {
    tx_c(t, "0123456789ABCDEF"[*p >> 4]);
    tx_c(t, "0123456789ABCDEF"[*p & 15]);
  }
  tx_c(t, '\n');
}

// One VOP (after its start code) as records. Returns 0, -1 when it can't be represented, -2 when
// its text doesn't fit.
static int vop_to_text(Ctx* c, GB* gb, Txt* t) {
  VopHdr h;
  Vop v;
  static MB m;
  int mb_x = 0, mb_y = 0, anchor, ret = 0;
  if (parse_vop_hdr(c, gb, &h) < 0) return -1;
  fmt_vop_hdr(t, c, &h);
  if (!h.coded) { fmt_tail(t, gb); return t->full ? -2 : 0; }
  if (c->mb_num > MAX_MBS) return -1;
  v.type = h.type;
  v.gmc = h.type == PT_S && c->sprite_usage == GMC_SPRITE;
  v.fcode = h.fcode == NA ? 1 : h.fcode;
  v.bcode = h.bcode == NA ? 1 : h.bcode;
  v.qscale = h.quant;
  v.dcthr = dc_threshold[h.dcthr];
  v.tib = h.tib;
  anchor = h.type != PT_B;
  if (anchor) memset(skip_cur, 0, c->mb_num + 1);
  v.qscale = clipi(v.qscale, 1, 31);
  for (;;) {
    const int idx = mb_y * c->mb_w + mb_x;
    int dcvlc = v.qscale < v.dcthr, next, nstuff;
    m.x = mb_x;
    m.y = mb_y;
    if (parse_mb(c, gb, &v, &m, idx) < 0) { ret = -1; break; }
    if (anchor) skip_cur[idx] = (U8)(m.sk == 1 && !v.gmc);
    fmt_mb(t, &v, &m, dcvlc);
    if (++mb_x == c->mb_w) { mb_x = 0; mb_y++; }
    if (mb_y >= c->mb_h) break;
    if (!(next = resync_ahead(c, gb, &v, &nstuff))) continue;
    if (idx + 1 < next && v.type == PT_B && skip_ref[idx + 1]) continue;
    {  // video packet
      VpHdr k;
      if (nstuff && v.type == PT_S) { ret = -1; break; }   // not MCBPC stuffing there
      k.st = nstuff;
      gb_skip(gb, nstuff * (8 + v.type));
      gb_skip(gb, 1);
      gb_align(gb);
      if (parse_vp(c, gb, &v, &k) < 0) { ret = -1; break; }
      fmt_vp(t, &v, c, &k);
      mb_x = k.mbnum % c->mb_w;
      mb_y = k.mbnum / c->mb_w;
      v.qscale = clipi(v.qscale, 1, 31);
    }
  }
  if (anchor) memcpy(skip_ref, skip_cur, c->mb_num + 1);
  if (ret < 0 || gb->pos > gb->size) return -1;
  fmt_tail(t, gb);
  return t->full ? -2 : 0;
}

//--- text input (d) ------------------------------------------------------------------------------

#define MAX_FIELDS 24

typedef struct Line {
  const char* f[MAX_FIELDS];
  const char* e[MAX_FIELDS];
  int n;
} Line;

// splits a line (without its newline) at tabs; 0 if it has too many fields
static int split_line(Line* L, const char* s, const char* end) {
  L->n = 0;
  if (end > s && end[-1] == '\r') end--;
  for (;;) {
    const char* p = s;
    while (p < end && *p != '\t') p++;
    if (L->n == MAX_FIELDS) return 0;
    L->f[L->n] = s;
    L->e[L->n] = p;
    L->n++;
    if (p == end) return 1;
    s = p + 1;
  }
}

static int fld_is(const Line* L, int i, char c) { return i < L->n && L->e[i] - L->f[i] == 1 && L->f[i][0] == c; }
static int fld_dash(const Line* L, int i) { return fld_is(L, i, '-'); }

static int parse_int(const char** ps, const char* e, int* v) {  // [-]digits
  const char* s = *ps;
  int neg = 0;
  S64 x = 0;
  if (s < e && *s == '-') { neg = 1; s++; }
  if (s == e || *s < '0' || *s > '9') return 0;
  for (; s < e && *s >= '0' && *s <= '9'; s++)
    if ((x = x * 10 + (*s - '0')) > 0x7FFFFFFF) return 0;
  *v = neg ? (int)-x : (int)x;
  *ps = s;
  return 1;
}

// field i as an integer in [lo, hi], or "-" when the syntax doesn't have it (want = 0)
static int fld_int(const Line* L, int i, int want, int lo, int hi, int* v) {
  const char* s;
  if (i >= L->n) return 0;
  if (!want) { *v = NA; return fld_dash(L, i); }
  s = L->f[i];
  return parse_int(&s, L->e[i], v) && s == L->e[i] && *v >= lo && *v <= hi;
}

static int fld_bits(const Line* L, int i, int n, int* v) {  // n binary digits, "-" for none
  const char* s;
  if (i >= L->n) return 0;
  if (!n) return fld_dash(L, i);
  if (L->e[i] - L->f[i] != n) return 0;
  for (*v = 0, s = L->f[i]; s < L->e[i]; s++) {
    if (*s != '0' && *s != '1') return 0;
    *v = *v * 2 + (*s - '0');
  }
  return 1;
}

static int fld_list(const Line* L, int i, int n, int* a) {  // n integers separated by commas
  const char* s;
  int k;
  if (i >= L->n) return 0;
  if (!n) return fld_dash(L, i);
  for (s = L->f[i], k = 0; k < n; k++) {
    if (k && (s == L->e[i] || *s++ != ',')) return 0;
    if (!parse_int(&s, L->e[i], &a[k])) return 0;
  }
  return s == L->e[i];
}

static int fld_block(const Line* L, int i, int dcvlc, Blk* b) {
  const char *s, *e;
  int pos = dcvlc ? 0 : -1;
  if (i >= L->n) return 0;
  s = L->f[i];
  e = L->e[i];
  b->n = 0;
  b->dc = 0;
  if (dcvlc) {
    if (!parse_int(&s, e, &b->dc) || b->dc < -511 || b->dc > 511) return 0;
    if (s == e) return 1;
    if (*s++ != '|') return 0;
  } else if (fld_dash(L, i))
    return 1;
  for (;;) {
    int run, level, mode;
    if (b->n == 64 || !parse_int(&s, e, &run) || run < 0 || run > 63 || s == e || *s++ != ':' ||
        !parse_int(&s, e, &level) || level < -2048 || level > 2047)
      return 0;
    pos += run + 1;
    if (pos > 63) return 0;
    mode = -1;
    if (s < e && *s == 'E') {
      if (++s == e || *s < '0' || *s > '3') return 0;
      mode = *s++ - '0';
    }
    b->run[b->n] = (U8)run;
    b->level[b->n] = (S16)level;
    b->esc[b->n] = (U8)mode;   // resolved by put_block once "last" is known
    b->n++;
    if (s == e) break;
    if (*s++ != ' ') return 0;
  }
  return pos <= 63 && b->n > 0;
}

//--- writing (d) ---------------------------------------------------------------------------------

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

static int mv_ok(int diff, int f_code) { return diff >= -(32 << (f_code - 1)) && diff <= 32 << (f_code - 1); }

static void put_traj(const Ctx* c, PB* pb, const int* w) {
  int i;
  for (i = 0; i < 2 * c->warp_points; i++) {
    const int len = bitlen(w[i]);
    pb_put(pb, sprite_trajectory_code[len], sprite_trajectory_lens[len]);
    if (len) pb_xbits(pb, w[i], len);
    if ((i & 1) || !divx413(c)) pb_put(pb, 1, 1);
  }
}

static int put_block(PB* pb, const Blk* b, int n, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i;
  if (intra && dcvlc) {
    const int size = bitlen(b->dc);
    const U8* t = n < 4 ? DCtab_lum[size] : DCtab_chrom[size];
    pb_put(pb, t[0], t[1]);
    if (size) {
      pb_xbits(pb, b->dc, size);
      if (size > 8) pb_put(pb, 1, 1);
    }
  }
  for (i = 0; i < b->n; i++) {
    const int run = b->run[i], level = b->level[i], last = i + 1 == b->n, alevel = level < 0 ? -level : level;
    int mode = b->esc[i] == 0xFF ? esc_default(rl, last, run, alevel) : b->esc[i], s, k;
    if (!esc_bits(rl, mode, last, run, alevel)) return -1;
    switch (mode) {
    case 0:
      s = rl->index[last][run][alevel];
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, 1);
      break;
    case 1:
      s = rl->index[last][run][alevel - rl->max_level[last][run]];
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, 0, 1);
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, 1);
      break;
    case 2:
      k = run - rl->max_run[last][alevel] - 1;
      s = rl->index[last][k][alevel];
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, 2, 2);
      pb_put(pb, rl->vlc[s][0], rl->vlc[s][1]);
      pb_put(pb, level < 0, 1);
      break;
    default:
      pb_put(pb, rl->vlc[rl->n][0], rl->vlc[rl->n][1]);
      pb_put(pb, 3, 2);
      pb_put(pb, last, 1);
      pb_put(pb, run, 6);
      pb_put(pb, 1, 1);
      pb_put(pb, (U32)level & 0xFFF, 12);
      pb_put(pb, 1, 1);
      break;
    }
  }
  return 0;
}

static int cbp_of(const MB* m) {  // coded flags, block 0 in bit 5
  int i, cbp = 0;
  for (i = 0; i < 6; i++) cbp = cbp * 2 + (m->b[i].n > 0);
  return cbp;
}

static int put_blocks(PB* pb, const MB* m, int intra, int dcvlc) {
  int i;
  for (i = 0; i < 6; i++)
    if (put_block(pb, &m->b[i], i, intra, dcvlc) < 0) return -1;
  return 0;
}

static int dq_index(int dq) { int i; for (i = 0; i < 4; i++) if (quant_tab[i] == dq) return i; return -1; }

typedef struct Enc {   // d: the VOP being written
  PB pb;
  Vop v;
  int active;
} Enc;

// V record -> 00 00 01 B6 + VOP header
static int put_vop_hdr(const Ctx* c, Enc* en, const Line* L) {
  PB* pb = &en->pb;
  Vop* v = &en->v;
  VopHdr h;
  int i, gmc;
  if (L->n < 6 || !(h.type = vop_type_of(L->f[1], L->e[1]))) return -1;
  if (!fld_int(L, 2, 1, 0, 1000, &h.mtb) || !fld_int(L, 3, 1, 1, 16, &h.tib) ||
      !fld_int(L, 4, 1, 0, (1 << h.tib) - 1, &h.tinc) || !fld_int(L, 5, 1, 0, 1, &h.coded))
    return -1;
  pb_put(pb, 0x1B6, 32);
  pb_put(pb, (U32)(h.type - PT_I), 2);
  for (i = 0; i < h.mtb; i++) pb_put(pb, 1, 1);
  pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)h.tinc, h.tib);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)h.coded, 1);
  v->type = h.type;
  v->tib = h.tib;
  if (!h.coded) return L->n == 6 ? 0 : -1;
  if (!c->have_vol || c->unsupported || (c->dp && h.type != PT_B)) return -1;
  gmc = h.type == PT_S && c->sprite_usage == GMC_SPRITE;
  h.ncplx = c->cplx_i + (h.type != PT_I ? c->cplx_p : 0) + (h.type == PT_B ? c->cplx_b : 0);
  if (L->n != 15 || !fld_int(L, 6, h.type == PT_P || gmc, 0, 1, &h.rnd) ||
      !fld_int(L, 8, 1, 0, 7, &h.dcthr) || !fld_int(L, 9, c->interlaced, 0, 1, &h.tff) ||
      !fld_int(L, 10, c->interlaced, 0, 1, &h.alt) || !fld_list(L, 11, gmc ? 2 * c->warp_points : 0, h.warp) ||
      !fld_int(L, 12, 1, 1, (1 << c->quant_precision) - 1, &h.quant) ||
      !fld_int(L, 13, h.type != PT_I, 1, 7, &h.fcode) || !fld_int(L, 14, h.type == PT_B, 1, 7, &h.bcode))
    return -1;
  if (h.ncplx ? L->e[7] - L->f[7] != h.ncplx : !fld_dash(L, 7)) return -1;
  if (h.rnd != NA) pb_put(pb, (U32)h.rnd, 1);
  for (i = 0; i < h.ncplx; i++) {
    const char ch = L->f[7][i];
    if (ch != '0' && ch != '1') return -1;
    pb_put(pb, (U32)(ch - '0'), 1);
  }
  pb_put(pb, (U32)h.dcthr, 3);
  if (c->interlaced) { pb_put(pb, (U32)h.tff, 1); pb_put(pb, (U32)h.alt, 1); }
  if (gmc) {
    for (i = 0; i < 2 * c->warp_points; i++) if (bitlen(h.warp[i]) > 14) return -1;
    put_traj(c, pb, h.warp);
  }
  pb_put(pb, (U32)h.quant, c->quant_precision);
  if (h.fcode != NA) pb_put(pb, (U32)h.fcode, 3);
  if (h.bcode != NA) pb_put(pb, (U32)h.bcode, 3);
  v->gmc = gmc;
  v->fcode = h.fcode == NA ? 1 : h.fcode;
  v->bcode = h.bcode == NA ? 1 : h.bcode;
  v->qscale = clipi(h.quant, 1, 31);
  v->dcthr = dc_threshold[h.dcthr];
  en->active = 1;
  return 0;
}

static void put_stuffing(PB* pb, const Vop* v, int n) {  // MCBPC stuffing codes
  while (n-- > 0) {
    if (v->type == PT_I) pb_put(pb, intra_mcbpc_code[8], intra_mcbpc_bits[8]);
    else { pb_put(pb, 0, 1); pb_put(pb, inter_mcbpc_code[20], inter_mcbpc_bits[20]); }
  }
}

static int put_vp(const Ctx* c, Enc* en, const Line* L) {
  PB* pb = &en->pb;
  Vop* v = &en->v;
  VpHdr k;
  int i;
  if (!fld_int(L, 1, 1, 0, 1000, &k.st) || !fld_int(L, 2, 1, 1, c->mb_num - 1, &k.mbnum) ||
      !fld_int(L, 3, 1, 0, (1 << c->quant_precision) - 1, &k.quant) || !fld_int(L, 4, 1, 0, 1, &k.hec))
    return -1;
  if (k.st && v->type == PT_S) return -1;
  put_stuffing(pb, v, k.st);
  pb_put(pb, 0, 1);                     // stuffing up to the byte boundary
  while (pb->n) pb_put(pb, 1, 1);
  for (i = 0; i < prefix_length(v); i++) pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k.mbnum, c->mb_num_bits);
  pb_put(pb, (U32)k.quant, c->quant_precision);
  pb_put(pb, (U32)k.hec, 1);
  if (k.quant) v->qscale = k.quant;
  v->qscale = clipi(v->qscale, 1, 31);
  if (!k.hec) return L->n == 5 ? 0 : -1;
  if (L->n != 12 || !fld_int(L, 5, 1, 0, 1000, &k.mtb) || !fld_int(L, 6, 1, 0, (1 << v->tib) - 1, &k.tinc) ||
      !(k.type = vop_type_of(L->f[7], L->e[7])) || !fld_int(L, 8, 1, 0, 7, &k.dcthr) || !fld_list(L, 9, v->gmc ? 2 * c->warp_points : 0, k.warp) ||
      !fld_int(L, 10, v->type != PT_I, 0, 7, &k.fcode) || !fld_int(L, 11, v->type == PT_B, 0, 7, &k.bcode))
    return -1;
  for (i = 0; i < k.mtb; i++) pb_put(pb, 1, 1);
  pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k.tinc, v->tib);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)(k.type - PT_I), 2);
  pb_put(pb, (U32)k.dcthr, 3);
  if (v->gmc) {
    for (i = 0; i < 2 * c->warp_points; i++) if (bitlen(k.warp[i]) > 14) return -1;
    put_traj(c, pb, k.warp);
  }
  if (k.fcode != NA) pb_put(pb, (U32)k.fcode, 3);
  if (k.bcode != NA) pb_put(pb, (U32)k.bcode, 3);
  return 0;
}

static int put_intra(const Ctx* c, PB* pb, Vop* v, const MB* m) {  // after the MCBPC
  const int cbp = cbp_of(m), dcvlc = v->qscale < v->dcthr;
  pb_put(pb, (U32)m->ac, 1);
  pb_put(pb, cbpy_tab[cbp >> 2][0], cbpy_tab[cbp >> 2][1]);
  if (m->dq) { pb_put(pb, (U32)dq_index(m->dq), 2); v->qscale = clipi(v->qscale + m->dq, 1, 31); }
  if (c->interlaced) pb_put(pb, (U32)m->dt, 1);
  return put_blocks(pb, m, 1, dcvlc);
}

// i/p/b record -> macroblock
static int put_mb(const Ctx* c, Enc* en, const Line* L) {
  static MB m;
  PB* pb = &en->pb;
  Vop* v = &en->v;
  int i, cbp, dcvlc = v->qscale < v->dcthr, fp_want, nmv;
  if (L->n < 4) return -1;
  if (v->type == PT_I) {
    if (L->n != 13 || !fld_int(L, 3, 1, 0, 1000, &m.st) || !fld_int(L, 4, 1, 0, 1, &m.ac) ||
        !fld_int(L, 5, 1, -2, 2, &m.dq) || (m.dq && dq_index(m.dq) < 0) ||
        !fld_int(L, 6, c->interlaced, 0, 1, &m.dt))
      return -1;
    for (i = 0; i < 6; i++) if (!fld_block(L, 7 + i, dcvlc, &m.b[i])) return -1;
    cbp = cbp_of(&m);
    put_stuffing(pb, v, m.st);
    i = (m.dq ? 4 : 0) | (cbp & 3);
    pb_put(pb, intra_mcbpc_code[i], intra_mcbpc_bits[i]);
    return put_intra(c, pb, v, &m);
  }
  if (v->type != PT_B) {
    if (!fld_int(L, 3, 1, 0, 1000, &m.st) || !fld_int(L, 4, 1, 0, 1, &m.sk)) return -1;
    put_stuffing(pb, v, m.st);
    pb_put(pb, (U32)m.sk, 1);
    if (m.sk) return L->n == 5 ? 0 : -1;
    if (L->n != 19 || !fld_int(L, 5, 1, 0, 2, &m.t)) return -1;
    if (m.t == 2) {
      if (!fld_dash(L, 6) || !fld_int(L, 7, 1, 0, 1, &m.ac) || !fld_int(L, 8, 1, -2, 2, &m.dq) ||
          (m.dq && dq_index(m.dq) < 0) || !fld_int(L, 9, c->interlaced, 0, 1, &m.dt) || !fld_dash(L, 10) ||
          !fld_dash(L, 11) || !fld_dash(L, 12))
        return -1;
      for (i = 0; i < 6; i++) if (!fld_block(L, 13 + i, dcvlc, &m.b[i])) return -1;
      cbp = cbp_of(&m);
      i = 4 | (m.dq ? 8 : 0) | (cbp & 3);
      pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
      return put_intra(c, pb, v, &m);
    }
    if (!fld_int(L, 6, v->gmc && !m.t, 0, 1, &m.mc) || !fld_dash(L, 7) || !fld_int(L, 8, 1, -2, 2, &m.dq) ||
        (m.dq && dq_index(m.dq) < 0))
      return -1;
    for (i = 0; i < 6; i++) if (!fld_block(L, 13 + i, 0, &m.b[i])) return -1;
    cbp = cbp_of(&m);
    fp_want = c->interlaced && !m.t && m.mc != 1;
    if (!fld_int(L, 9, c->interlaced && cbp, 0, 1, &m.dt) || !fld_int(L, 10, fp_want, 0, 1, &m.fp)) return -1;
    nmv = m.t ? 8 : m.mc == 1 ? 0 : m.fp == 1 ? 4 : 2;
    if (!fld_bits(L, 11, m.fp == 1 ? 2 : 0, &m.fs) || !fld_list(L, 12, nmv, m.mv)) return -1;
    for (i = 0; i < nmv; i++) if (!mv_ok(m.mv[i], v->fcode)) return -1;
    i = (m.t ? 16 : 0) | (m.dq ? 8 : 0) | (cbp & 3);
    pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
    if (m.mc != NA) pb_put(pb, (U32)m.mc, 1);
    pb_put(pb, cbpy_tab[(cbp >> 2) ^ 15][0], cbpy_tab[(cbp >> 2) ^ 15][1]);
    if (m.dq) { pb_put(pb, (U32)dq_index(m.dq), 2); v->qscale = clipi(v->qscale + m.dq, 1, 31); }
    if (m.dt != NA) pb_put(pb, (U32)m.dt, 1);
    if (m.fp != NA) pb_put(pb, (U32)m.fp, 1);
    if (m.fp == 1) pb_put(pb, (U32)m.fs, 2);
    for (i = 0; i < nmv; i++) put_mv(pb, m.mv[i], v->fcode);
    return put_blocks(pb, &m, 0, 0);
  }
  // B-VOP
  if (!fld_int(L, 3, 1, 0, 1, &m.sk)) return -1;
  if (m.sk) return L->n == 4 ? 0 : -1;
  if (!fld_int(L, 4, 1, 0, 2, &m.mb)) return -1;
  if (!m.mb) { pb_put(pb, 1, 1); return L->n == 5 ? 0 : -1; }
  if (L->n != 17 || !fld_int(L, 5, 1, 0, 3, &m.t)) return -1;
  for (i = 0; i < 6; i++) if (!fld_block(L, 11 + i, 0, &m.b[i])) return -1;
  cbp = cbp_of(&m);
  if (m.mb == 1 && cbp) return -1;
  if (!fld_int(L, 6, m.t && cbp, -2, 2, &m.dq) || (m.dq != NA && (m.dq & 1)) ||
      !fld_int(L, 7, c->interlaced && cbp, 0, 1, &m.dt) || !fld_int(L, 8, c->interlaced && m.t, 0, 1, &m.fp))
    return -1;
  m.nfs = m.fp == 1 ? 2 * (fwd_dir(m.t) + bwd_dir(m.t)) : 0;
  nmv = m.t ? 2 * mv_count(m.t, m.fp) : 2;
  if (!fld_bits(L, 9, m.nfs, &m.fs) || !fld_list(L, 10, nmv, m.mv)) return -1;
  pb_put(pb, 0, 1);
  pb_put(pb, m.mb == 1, 1);
  pb_put(pb, mb_type_b_tab[m.t][0], mb_type_b_tab[m.t][1]);
  if (m.mb == 2) pb_put(pb, (U32)cbp, 6);
  if (m.dq != NA) {
    if (!m.dq) pb_put(pb, 0, 1);
    else pb_put(pb, m.dq > 0 ? 3 : 2, 2);
    v->qscale = clipi(v->qscale + m.dq, 1, 31);
  }
  if (m.dt != NA) pb_put(pb, (U32)m.dt, 1);
  if (m.fp != NA) pb_put(pb, (U32)m.fp, 1);
  if (m.nfs) pb_put(pb, (U32)m.fs, m.nfs);
  if (m.t) {
    const int n = 2 * (m.fp == 1 ? 2 : 1);
    int k = 0;
    if (fwd_dir(m.t)) for (i = 0; i < n; i++, k++) { if (!mv_ok(m.mv[k], v->fcode)) return -1; put_mv(pb, m.mv[k], v->fcode); }
    if (bwd_dir(m.t)) for (i = 0; i < n; i++, k++) { if (!mv_ok(m.mv[k], v->bcode)) return -1; put_mv(pb, m.mv[k], v->bcode); }
  } else
    for (i = 0; i < 2; i++) { if (!mv_ok(m.mv[i], 1)) return -1; put_mv(pb, m.mv[i], 1); }
  return put_blocks(pb, &m, 0, 0);
}

static int hexval(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return -1;
}

static int put_tail(Enc* en, const Line* L) {  // T record: ends the VOP
  PB* pb = &en->pb;
  const char* s;
  if (L->n != 3) return -1;
  if (!fld_dash(L, 1)) {
    for (s = L->f[1]; s < L->e[1]; s++) {
      if (*s != '0' && *s != '1') return -1;
      pb_put(pb, (U32)(*s - '0'), 1);
    }
  }
  if (pb->n) return -1;                 // must reach a byte boundary
  if (!fld_dash(L, 2)) {
    if ((L->e[2] - L->f[2]) & 1) return -1;
    for (s = L->f[2]; s < L->e[2]; s += 2) {
      const int a = hexval(s[0]), b = hexval(s[1]);
      if (a < 0 || b < 0) return -1;
      pb_put(pb, (U32)(a * 16 + b), 8);
    }
  }
  en->active = 0;
  return 0;
}

// one record of a VOP (V, K, i, p, b, T); 1 when T completed it, 0 to continue, -1 on errors
static int enc_line(const Ctx* c, Enc* en, const Line* L) {
  const char k = L->f[0][0];
  if (L->e[0] - L->f[0] != 1) return -1;
  if (k == 'V') {
    if (en->active) return -1;
    pb_init(&en->pb, en->pb.buf, en->pb.cap);
    en->active = 1;
    return put_vop_hdr(c, en, L) < 0 ? -1 : 0;
  }
  if (!en->active) return -1;
  switch (k) {
  case 'T': return put_tail(en, L) < 0 ? -1 : 1;
  case 'K': return put_vp(c, en, L) < 0 ? -1 : 0;
  case 'i': if (en->v.type != PT_I) return -1; break;
  case 'p': if (en->v.type != PT_P && en->v.type != PT_S) return -1; break;
  case 'b': if (en->v.type != PT_B) return -1; break;
  default: return -1;
  }
  return put_mb(c, en, L) < 0 ? -1 : 0;
}

//--- c: stream -> TSV ----------------------------------------------------------------------------


static U8 ibuf[IBUF_SIZE + 64];
static char tbuf[TBUF_SIZE];
static U8 obuf[OBUF_SIZE];

typedef struct Reader {
  FILE* f;
  size_t lo, hi;   // ibuf[lo, hi): data not returned yet
  int eof, cont;   // cont: the last chunk didn't fit and continues
  U8* pad;         // the 64 bytes after the last chunk, zeroed while it is parsed
  U8 saved[64];
} Reader;

static int rd_fill(Reader* r) {  // moves the data not returned yet to the buffer start and reads more
  size_t n;
  if (r->lo) { memmove(ibuf, ibuf + r->lo, r->hi - r->lo); r->hi -= r->lo; r->lo = 0; }
  if (r->eof || r->hi == IBUF_SIZE) return 0;
  n = fread(ibuf + r->hi, 1, IBUF_SIZE - r->hi < IREAD ? IBUF_SIZE - r->hi : IREAD, r->f);
  r->hi += n;
  if (!n) r->eof = 1;
  return n != 0;
}

static int find_sc(size_t* pi, size_t end) {  // 00 00 01 in ibuf[*pi, end); else *pi = where to resume
  size_t i = *pi;
  while (i + 3 <= end) {
    if (ibuf[i + 2] > 1) i += 3;
    else if (!ibuf[i + 2]) i++;
    else if (!ibuf[i] && !ibuf[i + 1]) { *pi = i; return 1; }
    else i += 3;
  }
  *pi = i;
  return 0;
}

// Next chunk: from a start code (or the file start) up to the next start code, in
// ibuf[*pos, *pos + *len), followed by 64 zero bytes until the next call. *part: 0 whole chunk,
// 1 first piece of a chunk longer than IBUF_SIZE, 2 a later piece. 0 at the end of the input.
static int rd_chunk(Reader* r, size_t* pos, size_t* len, int* part) {
  size_t i;
  int found;
  if (r->pad) { memcpy(r->pad, r->saved, 64); r->pad = 0; }
  if (r->lo == r->hi && !rd_fill(r)) return 0;
  *part = r->cont ? 2 : 0;
  for (i = r->lo + !r->cont;;) {
    if ((found = find_sc(&i, r->hi))) {
      if (i == r->lo) { *part = 0; r->cont = 0; i++; continue; }   // a continued chunk ended at the piece boundary
      r->cont = 0;
      break;
    }
    i -= r->lo;
    if (!rd_fill(r)) {
      if (r->eof) { i = r->hi; r->cont = 0; break; }
      r->cont = 1;                      // full buffer: return a piece, keep a possible start code prefix
      if (*part == 0) *part = 1;
      break;
    }
    i += r->lo;
  }
  *pos = r->lo;
  *len = i - r->lo;
  r->lo = i;
  r->pad = ibuf + i;
  memcpy(r->saved, r->pad, 64);
  memset(r->pad, 0, 64);
  return 1;
}

typedef struct Stats {
  U32 vops, by_type[5], raw_vops, raw_unsup, raw_long, raw_parse, raw_verify;  // raw: why
  double raw_bytes;
} Stats;

static int write_raw(FILE* f, char kind, const U8* p, size_t n) {
  static char line[2 * RAW_LINE + 8];
  while (n) {
    const size_t k = n < RAW_LINE ? n : RAW_LINE;
    size_t i;
    char* o = line;
    *o++ = kind;
    *o++ = '\t';
    for (i = 0; i < k; i++) {
      *o++ = "0123456789ABCDEF"[p[i] >> 4];
      *o++ = "0123456789ABCDEF"[p[i] & 15];
    }
    *o++ = '\n';
    if (fwrite(line, 1, (size_t)(o - line), f) != (size_t)(o - line)) return 0;
    p += k;
    n -= k;
  }
  return 1;
}

// converts the records in text[0, n) back into ibuf-like bytes in obuf; returns the length or -1
static long text_to_vop(const Ctx* c, const char* text, size_t n) {
  Enc en;
  Line L;
  const char *s = text, *e = text + n;
  int r = 0;
  memset(&en, 0, sizeof(en));
  en.pb.buf = obuf;
  en.pb.cap = OBUF_SIZE;
  while (s < e) {
    const char* nl = (const char*)memchr(s, '\n', (size_t)(e - s));
    if (!nl) return -1;
    if (!split_line(&L, s, nl) || (r = enc_line(c, &en, &L)) < 0) return -1;
    s = nl + 1;
    if (r == 1) break;
  }
  return r == 1 && s == e && !en.pb.err ? (long)en.pb.len : -1;
}

static void vol_comment(FILE* f, const Ctx* c) {
  fprintf(f, "# VOL %dx%d (%dx%d macroblocks), %s, time increment %d bits, quant %d bits",
          c->width, c->height, c->mb_w, c->mb_h, c->interlaced ? "interlaced" : "progressive", c->time_inc_bits,
          c->quant_precision);
  if (c->sprite_usage == GMC_SPRITE) fprintf(f, ", GMC with %d warp points", c->warp_points);
  if (c->cplx_i + c->cplx_p + c->cplx_b) fprintf(f, ", complexity estimation %d/%d/%d bits", c->cplx_i, c->cplx_p, c->cplx_b);
  if (c->unsupported) fprintf(f, "; VOPs kept raw: %s", c->unsupported);
  fputc('\n', f);
}

static const char* format_comment =
  "# m4vtsv 1: MPEG-4 Part 2 video as TSV; 'm4vtsv d' restores the stream\n"
  "# H hex | R hex (raw VOP) | V type mtb tib tinc coded rnd cplx dcthr tff alt warp quant fcode bcode\n"
  "# K st mbnum quant hec mtb tinc type dcthr warp fcode bcode | T bits hex\n"
  "# i x y st ac dq dt b0..b5 | p x y st sk t mc ac dq dt fp fs mv b0..b5 | b x y sk mb t dq dt fp fs mv b0..b5\n";

static int convert_c(FILE* in, FILE* out, Ctx* c, Stats* st) {
  static Reader rd;
  size_t pos, len;
  int part, long_vop = 0;
  memset(&rd, 0, sizeof(rd));
  rd.f = in;
  fputs(format_comment, out);
  while (rd_chunk(&rd, &pos, &len, &part)) {
    const U8* p = ibuf + pos;
    const int is_vop = len >= 4 && !p[0] && !p[1] && p[2] == 1 && p[3] == 0xB6;
    if (part == 2) {                    // rest of a chunk too long for the buffer
      if (!write_raw(out, long_vop ? 'R' : 'H', p, len)) return -1;
      if (long_vop) st->raw_bytes += len;
      continue;
    }
    if (is_vop) {
      st->vops++;
      if (len >= 5) st->by_type[(p[4] >> 6) + PT_I]++;
      if (!part && c->have_vol && !c->unsupported && !(c->dp && len >= 5 && (p[4] >> 6) != 2)) {
        Txt t;
        GB gb;
        t.p = tbuf;
        t.end = tbuf + TBUF_SIZE;
        t.full = 0;
        gb.buf = p + 4;
        gb.size = (int)(len - 4) * 8;
        gb.pos = 0;
        const int r = vop_to_text(c, &gb, &t);
        if (r == -2) st->raw_long++;
        else if (r < 0) st->raw_parse++;
        else {
          const long n = text_to_vop(c, tbuf, (size_t)(t.p - tbuf));
          if (n == (long)len && !memcmp(obuf, p, len)) {
            if (fwrite(tbuf, 1, (size_t)(t.p - tbuf), out) != (size_t)(t.p - tbuf)) return -1;
            continue;
          }
          st->raw_verify++;
        }
      } else if (part)
        st->raw_long++;
      else
        st->raw_unsup++;
      st->raw_vops++;
      st->raw_bytes += len;
      long_vop = part == 1;
      if (!write_raw(out, 'R', p, len)) return -1;
      continue;
    }
    long_vop = 0;
    parse_header_chunk(c, p, len);
    if (!write_raw(out, 'H', p, len)) return -1;
    if (len >= 4 && !p[0] && !p[1] && p[2] == 1 && p[3] >= 0x20 && p[3] <= 0x2F) vol_comment(out, c);
  }
  return ferror(in) ? -2 : 0;
}

//--- d: TSV -> stream ----------------------------------------------------------------------------

static char lbuf[LBUF_SIZE];
static U8 rawbuf[RAW_LINE + 64];

typedef struct Sink {  // output file, or the original input to compare with (c -v)
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs;
} Sink;

static int sink_write(Sink* s, const U8* p, size_t n) {
  if (!s->compare) return fwrite(p, 1, n, s->f) == n;
  while (n && !s->differs) {
    static U8 cmp[1 << 16];
    const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
    const size_t got = fread(cmp, 1, k, s->f);
    size_t i;
    for (i = 0; i < got && cmp[i] == p[i]; i++) {}
    if (i < k) { s->differs = 1; s->diff_at = s->pos + i; return 1; }
    s->pos += k;
    p += k;
    n -= k;
  }
  return 1;
}

static int convert_d(FILE* in, Sink* out, Ctx* c, U64* lineno) {
  Enc en;
  Line L;
  memset(&en, 0, sizeof(en));
  en.pb.buf = obuf;
  en.pb.cap = OBUF_SIZE;
  *lineno = 0;
  while (fgets(lbuf, LBUF_SIZE, in)) {
    size_t n = strlen(lbuf);
    ++*lineno;
    if (n && lbuf[n - 1] == '\n') n--;
    else if (!feof(in)) return -2;      // line too long
    if (!n || lbuf[0] == '#' || (n == 1 && lbuf[0] == '\r')) continue;
    if (!split_line(&L, lbuf, lbuf + n)) return -1;
    if ((lbuf[0] == 'H' || lbuf[0] == 'R') && L.e[0] - L.f[0] == 1) {
      size_t k = (size_t)(L.e[1] - L.f[1]), i;
      if (en.active || L.n != 2 || (k & 1) || k / 2 > RAW_LINE) return -1;
      for (i = 0; i < k / 2; i++) {
        const int a = hexval(L.f[1][2 * i]), b = hexval(L.f[1][2 * i + 1]);
        if (a < 0 || b < 0) return -1;
        rawbuf[i] = (U8)(a * 16 + b);
      }
      if (lbuf[0] == 'H') parse_header_chunk(c, rawbuf, k / 2);
      if (!sink_write(out, rawbuf, k / 2)) return -3;
      continue;
    }
    switch (enc_line(c, &en, &L)) {
    case -1: return -1;
    case 1:
      if (en.pb.err) return -1;
      if (!sink_write(out, en.pb.buf, en.pb.len)) return -3;
      break;
    }
  }
  if (ferror(in)) return -4;
  return en.active ? -5 : 0;
}

//--- frontend ------------------------------------------------------------------------------------

static char obuf_file[1 << 20];

int main(int argc, char** argv) {
  static Ctx ctx;
  static Stats st;
  FILE *in, *out;
  int a = 2, quiet = 0, verify = 0, mode, r;
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) a = argc;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-q")) quiet = 1;
    else if (!strcmp(argv[a], "-v")) verify = 1;
    else break;
  }
  if (argc < 2 || a != argc - 2) {
    fprintf(stderr, "m4vtsv - lossless MPEG-4 Part 2 video (.m4v) <-> TSV converter\n"
                    "Usage: m4vtsc c|d [options] input output\n"
                    "  c   .m4v -> .tsv\n"
                    "  d   .tsv -> .m4v\n"
                    "  -v  (c) convert the TSV back afterwards and compare it with the input\n"
                    "  -q  no statistics\n");
    return 1;
  }
  mode = argv[1][0];
  tables_init();
  ctx_init(&ctx);
  if (!(in = fopen(argv[a], "rb"))) { fprintf(stderr, "can't open %s\n", argv[a]); return 2; }
  if (!(out = fopen(argv[a + 1], "wb"))) { fprintf(stderr, "can't create %s\n", argv[a + 1]); return 3; }
  setvbuf(out, obuf_file, _IOFBF, sizeof(obuf_file));
  if (mode == 'c') {
    r = convert_c(in, out, &ctx, &st);
    if (fclose(out) != 0 && r >= 0) r = -1;
    if (r == -2) { fprintf(stderr, "read error %s\n", argv[a]); return 2; }
    if (r < 0) { fprintf(stderr, "write error %s\n", argv[a + 1]); return 3; }
    if (!quiet) {
      fprintf(stderr, "%s: %u VOPs (I %u, P %u, B %u, S %u), %u kept raw", argv[a], st.vops,
              st.by_type[1], st.by_type[2], st.by_type[3], st.by_type[4], st.raw_vops);
      if (st.raw_vops)
        fprintf(stderr, " (%u unsupported, %u too long, %u unparsable, %u not reproduced; %.0f bytes)",
                st.raw_unsup, st.raw_long, st.raw_parse, st.raw_verify, st.raw_bytes);
      fprintf(stderr, "; %u coefficient events, %u with escape mode noted\n", n_events, n_esc_marked);
    }
    if (verify) {
      static Ctx ctx2;
      Sink s;
      U64 lineno;
      FILE* t = fopen(argv[a + 1], "rb");
      if (!t) { fprintf(stderr, "can't open %s\n", argv[a + 1]); return 3; }
      rewind(in);
      memset(&s, 0, sizeof(s));
      s.f = in;
      s.compare = 1;
      ctx_init(&ctx2);
      r = convert_d(t, &s, &ctx2, &lineno);
      fclose(t);
      if (r == 0 && !s.differs && fgetc(in) != EOF) { s.differs = 1; s.diff_at = s.pos; }
      if (r < 0 || s.differs) {
        fprintf(stderr, "verify: FAILED (%s at byte %.0f)\n", r < 0 ? "TSV error" : "difference", (double)s.diff_at);
        return 5;
      }
      if (!quiet) fprintf(stderr, "verify: OK\n");
    }
    fclose(in);
    return 0;
  }
  {
    Sink s;
    U64 lineno;
    memset(&s, 0, sizeof(s));
    s.f = out;
    r = convert_d(in, &s, &ctx, &lineno);
    fclose(in);
    if (fclose(out) != 0 && r == 0) r = -3;
    switch (r) {
    case 0: return 0;
    case -2: fprintf(stderr, "%s: line %.0f too long\n", argv[a], (double)lineno); return 4;
    case -3: fprintf(stderr, "write error %s\n", argv[a + 1]); return 3;
    case -4: fprintf(stderr, "read error %s\n", argv[a]); return 2;
    case -5: fprintf(stderr, "%s: ends inside a VOP\n", argv[a]); return 4;
    default: fprintf(stderr, "%s: bad record in line %.0f\n", argv[a], (double)lineno); return 4;
    }
  }
}
