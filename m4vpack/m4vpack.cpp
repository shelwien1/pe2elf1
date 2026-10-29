// m4vpack - lossless compressor for MPEG-4 Part 2 video elementary streams (.m4v: XviD, DivX 4/5/6,
// libavcodec mpeg4, ...)
// Usage: m4vpack c|d [options] input output
//   c    compress
//   d    decompress, restoring the original stream byte for byte
//   -v   (c) afterwards decompress the output in memory and compare it with the input
//   -s   (c) input and output sizes per kind of syntax element
//   -q   no statistics
//
// The bitstream syntax follows m4vdec.cpp, a port of FFmpeg's MPEG-4 Part 2 decoder, so this file
// is licensed under the GNU Lesser General Public License 2.1 or later.
//
// How it works. The stream is cut at start codes (00 00 01 xx). Each VOP is parsed into the
// syntax records m4vtsv writes as text (VOP and video packet headers, one record per macroblock
// with its motion vector differences and coefficient events, and the bits after the last
// macroblock), and the records are written back as bits and compared with the input. If that
// reproduces the VOP, the records are coded with a binary arithmetic coder; otherwise (unsupported
// features, damaged data) the VOP is kept as bytes, so compression is always lossless. Headers and
// user data are kept as bytes too. The same functions code and decode every record (c->enc), so
// the two directions can't disagree.
//
// The models take H.264's CABAC as the template (see m4vtsv/h264-counterparts.md): binarised syntax
// elements with adaptive probabilities chosen by neighbouring macroblocks and blocks.
//   - Macroblock types: skip, intra, 4MV, B modes by the left, above, above-left, above-right and
//     co-located (last anchor VOP) macroblocks and the run of skipped / direct macroblocks.
//   - Motion vector differences: zero flag, magnitude in units of the VLC code (independent of
//     f_code) in unary, the f_code residual bits, sign; contexts are the neighbours' magnitudes
//     (as CABAC's mvd), the other component and the co-located macroblock.
//   - Coded block pattern: per 8x8 block, by the left/above blocks, the motion vector size and the
//     co-located macroblock.
//   - Intra DC differences: size (by the previous and neighbouring blocks' sizes), lower bits, sign.
//   - Coefficients as CABAC's residual: a significance map with "last" flags in scan order, then the
//     levels in reverse order (|level| > 1, unary, then size and bits), signs, and the escape mode
//     where it isn't the usual one. Contexts: position, the neighbouring blocks' counts (CAVLC's
//     nC), the coefficients coded so far, the left and above blocks' coefficients at the same scan
//     position, the frequency neighbours (as HEVC's template), quantiser, motion vector size.
// Each of these decisions mixes the predictions of several contexts (logistic mixing as in lpaq,
// two weight sets), coefficient decisions then pass an APM (SSE). Probabilities adapt with a rate
// of 1/n up to n = PLIMIT.
//
// File format: "m4vpack" 01, input size (8 bytes, little endian), CRC-32 of the input, then the
// arithmetic coded chunks: kind (end, VOP as records, bytes), then the records or a length and the
// bytes (order 0 and 1 contexts, mixed).
//
// Kept as bytes: as in m4vtsv, VOPs using data partitioning (B-VOPs don't), non-rectangular
// shapes, static sprites, sprite brightness change, newpred, reduced resolution, scalability or the
// studio profile; VOPs that don't parse (damaged data); VOPs longer than 64 MiB or with more than
// 1 MiB after the last macroblock.
//
// Build:
//   x86_64-w64-mingw32-g++ -O2 -s -static -o m4vpack.exe m4vpack.cpp
//   g++ -O2 -o m4vpack m4vpack.cpp
// Plain C (compiles as C or C++), no dependencies. No malloc: all buffers are static (about
// 150 MiB of .bss, touched as far as the data needs it). Integer arithmetic only where it matters
// for decoding, so files are portable between platforms.

#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef short S16;
typedef unsigned int U32;
typedef unsigned long long U64;
typedef long long S64;

#define NA (-1000)  // field not present
#ifndef IBUF_SIZE
#define IBUF_SIZE (64u << 20)    // longest chunk handled at once (longer ones are kept raw)
#endif
#define OBUF_SIZE (IBUF_SIZE + 4096)
#ifndef IREAD
#define IREAD (1u << 20)         // input read size
#endif

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
static int fwd_dir(int t) { return t == 1 || t == 3; }   // B-VOP macroblock types using the forward / backward reference
static int bwd_dir(int t) { return t == 1 || t == 2; }

static int prefix_length(const Vop* v) {  // zeros of a resync marker
  switch (v->type) {
  case PT_I: return 16;
  case PT_P: case PT_S: return v->fcode + 15;
  default: { int m = v->fcode > v->bcode ? v->fcode : v->bcode; return (m > 2 ? m : 2) + 15; }
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

static double pbits[3];  // statistics: bits of motion vectors, intra DCs, coefficients parsed

static int parse_block(GB* gb, Blk* b, int n, int coded, int intra, int dcvlc) {
  const RL* rl = intra ? &rl_intra : &rl_inter;
  int i = -1, p0 = gb->pos;
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
  pbits[1] += gb->pos - p0;
  p0 = gb->pos;
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
    if (last) { pbits[2] += gb->pos - p0; return 0; }
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
  const int p0 = gb->pos;
  int i;
  for (i = 0; i < 2 * n; i++, m->nmv++)
    if (get_mv(gb, f_code, &m->mv[m->nmv]) < 0) return -1;
  pbits[0] += gb->pos - p0;
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

static int mv_ok(int diff, int f_code) { return diff >= -(32 << (f_code - 1)) && diff <= 32 << (f_code - 1); }

static int put_traj(const Ctx* c, PB* pb, const int* w) {
  int i;
  for (i = 0; i < 2 * c->warp_points; i++) {
    const int len = bitlen(w[i]);
    if (len > 14) return -1;
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

static void put_stuffing(PB* pb, const Vop* v, int n) {  // MCBPC stuffing codes
  while (n-- > 0) {
    if (v->type == PT_I) pb_put(pb, intra_mcbpc_code[8], intra_mcbpc_bits[8]);
    else { pb_put(pb, 0, 1); pb_put(pb, inter_mcbpc_code[20], inter_mcbpc_bits[20]); }
  }
}

static int put_intra(const Ctx* c, PB* pb, Vop* v, const MB* m) {  // after the MCBPC
  const int cbp = cbp_of(m), dcvlc = v->qscale < v->dcthr;
  pb_put(pb, (U32)m->ac, 1);
  pb_put(pb, cbpy_tab[cbp >> 2][0], cbpy_tab[cbp >> 2][1]);
  if (m->dq) { pb_put(pb, (U32)dq_index(m->dq), 2); v->qscale = clipi(v->qscale + m->dq, 1, 31); }
  if (c->interlaced) pb_put(pb, (U32)m->dt, 1);
  return put_blocks(pb, m, 1, dcvlc);
}

// The writers check what the parser checks: decoded records come from data that may be damaged.

static int put_vop_hdr(const Ctx* c, PB* pb, const VopHdr* h) {  // 00 00 01 B6 and the VOP header
  int i;
  if (h->mtb > 1000) return -1;
  pb_put(pb, 0x1B6, 32);
  pb_put(pb, (U32)(h->type - PT_I), 2);
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
  pb_put(pb, (U32)h->dcthr, 3);
  if (c->interlaced) { pb_put(pb, (U32)h->tff, 1); pb_put(pb, (U32)h->alt, 1); }
  if (h->type == PT_S && c->sprite_usage == GMC_SPRITE && put_traj(c, pb, h->warp) < 0) return -1;
  pb_put(pb, (U32)h->quant, c->quant_precision);
  if (h->fcode != NA) pb_put(pb, (U32)h->fcode, 3);
  if (h->bcode != NA) pb_put(pb, (U32)h->bcode, 3);
  return 0;
}

static int put_vp(const Ctx* c, PB* pb, Vop* v, const VpHdr* k) {  // stuffing and video packet header
  int i;
  if (k->st > 1000 || (k->st && v->type == PT_S) || k->quant < 0 || k->quant >= 1 << c->quant_precision) return -1;
  put_stuffing(pb, v, k->st);
  pb_put(pb, 0, 1);                     // stuffing up to the byte boundary
  while (pb->n) pb_put(pb, 1, 1);
  for (i = 0; i < prefix_length(v); i++) pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->mbnum, c->mb_num_bits);
  pb_put(pb, (U32)k->quant, c->quant_precision);
  pb_put(pb, (U32)k->hec, 1);
  if (k->quant) v->qscale = k->quant;
  v->qscale = clipi(v->qscale, 1, 31);
  if (!k->hec) return 0;
  if (k->mtb > 1000) return -1;
  for (i = 0; i < k->mtb; i++) pb_put(pb, 1, 1);
  pb_put(pb, 0, 1);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->tinc, v->tib);
  pb_put(pb, 1, 1);
  pb_put(pb, (U32)k->type, 2);
  pb_put(pb, (U32)k->dcthr, 3);
  if (v->gmc && put_traj(c, pb, k->warp) < 0) return -1;
  if (k->fcode != NA) pb_put(pb, (U32)k->fcode, 3);
  if (k->bcode != NA) pb_put(pb, (U32)k->bcode, 3);
  return 0;
}

static int put_mb(const Ctx* c, PB* pb, Vop* v, const MB* m) {
  const int cbp = cbp_of(m);
  int i;
  if (m->st > 1000) return -1;
  if (v->type == PT_I) {
    put_stuffing(pb, v, m->st);
    i = (m->dq ? 4 : 0) | (cbp & 3);
    pb_put(pb, intra_mcbpc_code[i], intra_mcbpc_bits[i]);
    return put_intra(c, pb, v, m);
  }
  if (v->type != PT_B) {
    put_stuffing(pb, v, m->st);
    pb_put(pb, (U32)m->sk, 1);
    if (m->sk) return 0;
    if (m->t == 2) {
      i = 4 | (m->dq ? 8 : 0) | (cbp & 3);
      pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
      return put_intra(c, pb, v, m);
    }
    i = (m->t ? 16 : 0) | (m->dq ? 8 : 0) | (cbp & 3);
    pb_put(pb, inter_mcbpc_code[i], inter_mcbpc_bits[i]);
    if (m->mc != NA) pb_put(pb, (U32)m->mc, 1);
    pb_put(pb, cbpy_tab[(cbp >> 2) ^ 15][0], cbpy_tab[(cbp >> 2) ^ 15][1]);
    if (m->dq) { pb_put(pb, (U32)dq_index(m->dq), 2); v->qscale = clipi(v->qscale + m->dq, 1, 31); }
    if (m->dt != NA) pb_put(pb, (U32)m->dt, 1);
    if (m->fp != NA) pb_put(pb, (U32)m->fp, 1);
    if (m->fp == 1) pb_put(pb, (U32)m->fs, 2);
    for (i = 0; i < m->nmv; i++) {
      if (!mv_ok(m->mv[i], v->fcode)) return -1;
      put_mv(pb, m->mv[i], v->fcode);
    }
    return put_blocks(pb, m, 0, 0);
  }
  if (m->sk) return 0;                  // B-VOP
  if (!m->mb) { pb_put(pb, 1, 1); return 0; }
  pb_put(pb, 0, 1);
  pb_put(pb, m->mb == 1, 1);
  pb_put(pb, mb_type_b_tab[m->t][0], mb_type_b_tab[m->t][1]);
  if (m->mb == 2) pb_put(pb, (U32)cbp, 6);
  if (m->dq != NA) {
    if (!m->dq) pb_put(pb, 0, 1);
    else pb_put(pb, m->dq > 0 ? 3 : 2, 2);
    v->qscale = clipi(v->qscale + m->dq, 1, 31);
  }
  if (m->dt != NA) pb_put(pb, (U32)m->dt, 1);
  if (m->fp != NA) pb_put(pb, (U32)m->fp, 1);
  if (m->nfs) pb_put(pb, (U32)m->fs, m->nfs);
  for (i = 0; i < m->nmv; i++) {  // forward, then backward; delta vector of direct mode
    const int f = !m->t ? 1 : fwd_dir(m->t) && i < m->nmv / (1 + bwd_dir(m->t)) ? v->fcode : v->bcode;
    if (!mv_ok(m->mv[i], f)) return -1;
    put_mv(pb, m->mv[i], f);
  }
  return put_blocks(pb, m, 0, 0);
}

//--- input and output buffers ---------------------------------------------------------------------

static U8 ibuf[IBUF_SIZE + 64];
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

//--- binary arithmetic coder (carryless 32-bit range coder as in lpaq) ----------------------------

#define IOBUF (1u << 20)
static U8 cbuf[IOBUF];

enum { CAT_RAW, CAT_HDR, CAT_MB, CAT_CBP, CAT_MV, CAT_DC, CAT_SIG, CAT_LEV, CAT_SIGN, CAT_ESC, NCAT };
static const char* cat_name[NCAT] = { "raw chunks", "VOP and video packet headers, tails", "types, flags, dquant",
                                      "coded block patterns", "motion vectors", "intra DC", "positions",
                                      "levels", "signs", "escape modes" };

typedef struct Coder {
  int enc;
  U32 x1, x2, x;
  FILE* f;
  size_t n, pos;      // cbuf: bytes pending (enc), bytes available / read position (dec)
  U64 bytes;          // written (enc), read past the end of the input (dec)
  int err;
  int cat;            // statistics: category of the bits coded now
  double cost[NCAT];  // bits spent per category (enc)
  double orig[NCAT];  // bits they took in the input (CAT_MB includes CAT_CBP, CAT_SIG all coefficient bits)
  int stats;
} Coder;

static void co_put(Coder* c, int b) {
  c->bytes++;
  cbuf[c->n++] = (U8)b;
  if (c->n == IOBUF) {
    c->err |= fwrite(cbuf, 1, IOBUF, c->f) != IOBUF;
    c->n = 0;
  }
}
static int co_get(Coder* c) {
  if (c->pos == c->n) {
    c->n = fread(cbuf, 1, IOBUF, c->f);
    c->pos = 0;
    if (!c->n) { c->bytes++; return 255; }  // after the end: continues the flushed lower bound
  }
  return cbuf[c->pos++];
}
static void co_init(Coder* c, FILE* f, int enc) {
  int i;
  memset(c, 0, sizeof(*c));
  c->enc = enc;
  c->f = f;
  c->x2 = 0xFFFFFFFF;
  if (!enc) for (i = 0; i < 4; i++) c->x = (c->x << 8) | (U32)co_get(c);
}
static void co_flush(Coder* c) {
  co_put(c, (int)(c->x1 >> 24));
  if (c->n) c->err |= fwrite(cbuf, 1, c->n, c->f) != c->n;
  c->n = 0;
}

// adaptive probability of a 1 bit (16 bits) with a count for the adaptation rate
typedef struct Prob { U16 p, n; } Prob;
#ifndef PLIMIT
#define PLIMIT 511
#endif
static int rate[PLIMIT + 1];
static float bitcost[4097];  // -log2(i / 4096)

static int code_bit(Coder* c, U32 p, int bit) {  // p: probability of a 1 bit, 16 bits
  const U32 xmid = c->x1 + (U32)(((U64)(c->x2 - c->x1) * p) >> 16);
  if (c->enc) {
    if (c->stats) c->cost[c->cat] += bitcost[(bit ? p : 65536 - p) >> 4];
    if (bit) c->x2 = xmid; else c->x1 = xmid + 1;
    while (!((c->x1 ^ c->x2) & 0xFF000000)) {
      co_put(c, (int)(c->x2 >> 24));
      c->x1 <<= 8;
      c->x2 = (c->x2 << 8) | 255;
    }
  } else {
    bit = c->x <= xmid;
    if (bit) c->x2 = xmid; else c->x1 = xmid + 1;
    while (!((c->x1 ^ c->x2) & 0xFF000000)) {
      c->x1 <<= 8;
      c->x2 = (c->x2 << 8) | 255;
      c->x = (c->x << 8) | (U32)co_get(c);
    }
  }
  return bit;
}

static void upd(Prob* q, int bit) {
  q->p = (U16)(q->p + ((((S64)(bit ? 65535 : 0) - q->p) * rate[q->n]) >> 16));
  if (q->n < PLIMIT) q->n++;
}

static int cbit(Coder* c, Prob* q, int bit) {
  bit = code_bit(c, q->p < 64 ? 64 : q->p > 65472 ? 65472 : q->p, bit);
  upd(q, bit);
  return bit;
}

// Logistic mixing (as in lpaq): the predictions of several contexts, in the logistic domain, are
// added with weights that are trained to minimise the coding cost. A weight set is chosen by a
// small context of its own.
#define MAXIN 8
static short stretch_t[4096];  // ln(p / (1 - p)) * 256, p in 12 bits
static short squash_t[4096];   // the inverse, for -2048..2047

static int squash(int d) { return d > 2047 ? 4095 : d < -2047 ? 1 : squash_t[d + 2048]; }

#ifndef MIX_W0
#define MIX_W0 (1 << 14)
#endif
#ifndef MIX_LR
#define MIX_LR 4
#endif
#ifndef MIX_SHIFT
#define MIX_SHIFT 13
#endif

// APM (adaptive probability map, SSE): refines a probability by a context, interpolating between
// 33 points over its stretched value
#ifndef APM_RATE
#define APM_RATE 7
#endif
typedef struct Apm { U16 t[33]; } Apm;

static void apm_init(Apm* a, size_t n) {
  size_t i;
  int j;
  for (i = 0; i < n; i++)
    for (j = 0; j < 33; j++) a[i].t[j] = (U16)(squash((j - 16) * 128) * 16);
}

// One bit from the n predictions in[]. w: a bias weight, then n weights for the inputs (16.16 fixed
// point); w2: a second weight set (chosen by another context) or 0: the two outputs are averaged;
// a: an APM or 0.
static int mbit2(Coder* c, Prob* const* in, int n, int* w, int* w2, Apm* a, int bit) {
  int x[MAXIN + 1], i, pr, err, pf = 0, lo = 0, aw = 0, p1, p2 = 0;
  S64 dot = 0, dot2 = 0;
  x[0] = 256;
  for (i = 0; i < n; i++) x[i + 1] = stretch_t[in[i]->p >> 4];
  for (i = 0; i <= n; i++) dot += (S64)x[i] * w[i];
  p1 = pr = squash((int)(dot >> 16));
  if (w2) {
    for (i = 0; i <= n; i++) dot2 += (S64)x[i] * w2[i];
    p2 = squash((int)(dot2 >> 16));
    pr = squash((int)((dot + dot2) >> 17));
  }
  if (a) {
    const int st = stretch_t[pr] + 2048;
    lo = st >> 7;
    aw = st & 127;
    pf = (a->t[lo] * (128 - aw) + a->t[lo + 1] * aw) >> 11;
    pf = (pr + 3 * pf) >> 2;
    if (pf < 1) pf = 1;
    if (pf > 4095) pf = 4095;
  } else
    pf = pr;
  bit = code_bit(c, (U32)pf << 4, bit);
  if (a) {
    const int j = lo + (aw >> 6);
    a->t[j] = (U16)(a->t[j] + ((((bit << 16) - bit) - a->t[j]) >> APM_RATE));
  }
  err = ((bit << 12) - p1) * MIX_LR;
  for (i = 0; i <= n; i++) w[i] += (x[i] * err) >> MIX_SHIFT;
  if (w2) {
    err = ((bit << 12) - p2) * MIX_LR;
    for (i = 0; i <= n; i++) w2[i] += (x[i] * err) >> MIX_SHIFT;
  }
  for (i = 0; i < n; i++) upd(in[i], bit);
  return bit;
}

static int mbit(Coder* c, Prob* const* in, int n, int* w, int bit) { return mbit2(c, in, n, w, 0, 0, bit); }

// n-bit value with a binary tree of contexts (t has 1 << n entries)
static int code_bits(Coder* c, Prob* t, int n, int v) {
  int i, x = 1;
  for (i = n - 1; i >= 0; i--) x = x * 2 + cbit(c, &t[x], (v >> i) & 1);
  return x - (1 << n);
}

// n-bit value with a context per bit position
static int code_bitsi(Coder* c, Prob* t, int n, int v) {
  int i, x = 0;
  for (i = n - 1; i >= 0; i--) x |= cbit(c, &t[i < 31 ? i : 31], (v >> i) & 1) << i;
  return x;
}

#define EG_CTX 48
// v >= 0 as an Exp-Golomb code of order k with adaptive bits: t[0..23] prefix, t[24..47] suffix
static int code_eg(Coder* c, Prob* t, int k, int v) {
  int x = 0, n = 0;
  while (k < 28 && cbit(c, &t[n < 23 ? n : 23], c->enc ? v - x >= (1 << k) : 0)) {
    x += 1 << k;
    k++;
    n++;
  }
  while (k--) x += cbit(c, &t[24 + (k < 23 ? k : 23)], c->enc ? ((v - x) >> k) & 1 : 0) << k;
  return x;
}

#define SEG_CTX (EG_CTX + 2)
static int code_seg(Coder* c, Prob* t, int v) {  // signed: zero flag, sign, magnitude - 1
  int a;
  if (!cbit(c, &t[0], v != 0)) return 0;
  a = 1 + code_eg(c, t + 2, 0, v < 0 ? -v - 1 : v - 1);
  return cbit(c, &t[1], v < 0) ? -a : a;
}

//--- context models -------------------------------------------------------------------------------

#ifndef LUN
#define LUN 10  // contexts for the unary part of levels
#endif
#ifndef LUNARY
#define LUNARY 16  // levels up to this one in unary
#endif
#define NMVK 6  // kinds of motion vectors: P, P field, B forward, B backward, B direct, P 4MV
#define NCLS 8  // coefficient block classes: inter, B inter, intra, intra with AC prediction; x luma/chroma

typedef struct Model {  // only Prob members: all start at 1/2
  Prob kind[3][2];
  Prob rawlen[EG_CTX], raw0[256], raw[256][256];
  // VOP header
  Prob vtype[5][4], mtb[EG_CTX], tib_same, tib[16], tinc_same[5], tinc[32], coded[5], rnd[3], cplx[256];
  Prob dcthr_same, dcthr[8], tff[3], alt[3], warp[2][SEG_CTX], quant[5][SEG_CTX];
  Prob fcode_same[5], fcode[8], bcode_same, bcode[8];
  // video packet headers, tails
  Prob vpflag, vpst[EG_CTX], vpmb_exp, vpmb[32], vpq[SEG_CTX], hec, hmtb[EG_CTX], htinc[32], htype[4];
  Prob hdcthr[8], hwarp[SEG_CTX], hfcode[8], hbcode[8];
  Prob tail_canon, tailb[8], tailn[EG_CTX], tailbyte[256];
  // macroblock layer
  Prob st[2][EG_CTX], skip[2][6], skip2[2][5][6], skip3[2][11][3], pintra[6], pintra2[5][3];
  Prob p4mv[6], p4mv2[5][3], mcsel[3], bskip[3], modb0[9], modb02[6][5], modb03[11][3], modb2[3];
  Prob btype[3][9], btype2[3][11], ac[2][3];
  Prob cbp1[3][6][9], cbp2[3][6][7], cbp3[3][6][3][6];
  Prob dqz[3][2], dqv[3][4], bdqz[2], bdqs;
  Prob dt[2][3], fp[2][3], fs[4];
  // motion vector differences: [kind][component]
  Prob mvz[NMVK][2][6], mvz2[NMVK][2][7], mvz3[NMVK][2][7];
  Prob mvu[NMVK][2][12][6], mvu2[NMVK][2][12][7], mvu3[NMVK][2][12][7], mvr[NMVK][2][6], mvs[NMVK][2][3][3];
  // blocks
  Prob dcs[2][8][9], dcs2[2][9][9][9], dcs3[2][2][3][9], dcm[2][10][9], dcsign[2][2];
  Prob sig[NCLS][6][3][64], sig2[NCLS][8][64], sig3[NCLS][9][64], sig4[NCLS][9][3][64];
  Prob sig5[NCLS][6][64], sig6[NCLS][4][64], sig7[NCLS][7][64], sig8[NCLS][7][64];
  Prob last[NCLS][6][3][64], last2[NCLS][8][64], last3[NCLS][9][64], last4[NCLS][9][3][64];
  Prob last5[NCLS][6][64], last6[NCLS][7][64], last7[NCLS][7][64], last8[NCLS][6][6][6];
  Prob lev1[NCLS][5][6][3], lev1b[NCLS][5][6], lev1c[NCLS][5][8], lev1d[NCLS][4][6][3];
  Prob lev1e[NCLS][5][6], lev1f[NCLS][5][6], lev1g[NCLS][7][6], lev1q[NCLS][8][6];
  Prob levn[NCLS][5][3][LUN], levnb[NCLS][5][LUN], levnc[NCLS][5][LUN], levnd[NCLS][6][LUN];
  Prob levne[NCLS][5][LUN], levnq[NCLS][8][LUN];
  Prob lexp[NCLS][3][12], lexp2[NCLS][5][12], lexp3[NCLS][6][12], lman[NCLS][13][12];
  Prob sign[NCLS][2], sign2[NCLS][9][6];
  Prob escd[4], escm[4][4];
} Model;

typedef struct Last {  // previous values the models predict from
  int kind, rawprev;
  int type, tib, tinc, dinc[5], rnd, dcthr, tff, alt, quant[5], fcode[5], bcode;
} Last;

static Model model;
static Last last;

typedef struct Mixers {  // weight sets: a bias weight, then MAXIN input weights
  int sig[NCLS][64][MAXIN + 1], last[NCLS][64][MAXIN + 1], lev1[NCLS][6][5][MAXIN + 1];
  int sigb[NCLS][8][3][MAXIN + 1], lastb[NCLS][8][3][MAXIN + 1], lev1b[NCLS][5][3][MAXIN + 1];
  int levn[NCLS][LUN][5][MAXIN + 1], lexp[NCLS][12][MAXIN + 1], sign[NCLS][6][MAXIN + 1];
  int raw[8][MAXIN + 1], mbt[8][MAXIN + 1], cbp[3][6][MAXIN + 1], dcs[2][9][MAXIN + 1];
  int mvz[NMVK][2][MAXIN + 1], mvu[NMVK][2][12][MAXIN + 1];
} Mixers;
static Mixers mixw;

typedef struct Apms { Apm sig[NCLS][64], last[NCLS][64], lev1[NCLS][6][4]; } Apms;
static Apms apms;

// Coefficient magnitudes of the blocks coded last in each column of macroblocks: the left and
// above neighbours of a block (a macroblock's blocks replace those of the one above it as they are
// coded).
// mag: |level| (at most 255), sgn: 1 for negative, suf: coefficients at i..63, last: position of the
// last one (-1 none), dcs: size of the intra DC difference (-1 none)
typedef struct BlkCtx { U8 mag[64], sgn[64], suf[65]; S8 last, dcs; U8 n; } BlkCtx;
#define MAX_MB_W 512
static BlkCtx rowb[MAX_MB_W][6];

static void rowb_clear(int x) {
  int i;
  memset(rowb[x], 0, sizeof(rowb[x]));
  for (i = 0; i < 6; i++) rowb[x][i].last = rowb[x][i].dcs = -1;
}

// macroblock state the contexts of later macroblocks use
enum { C_NONE, C_SKIP, C_INTER, C_4MV, C_INTRA, C_GMC, C_BDIR0, C_BDIR, C_BFWD, C_BBWD, C_BBI };
// mva: magnitudes of the first motion vector difference in VLC code units, mvs: its signs (0, 1 +, 2 -);
// bsk: B-VOPs skip the macroblock at this position
typedef struct MBCtx { U8 cls, cbp, nz[6], mva[2], mvs[2], ac, dt, fp, bsk; } MBCtx;
static MBCtx mbcur[MAX_MBS], mbref[MAX_MBS];  // current VOP, last anchor VOP
static int ref_mbs;                            // size of the VOP in mbref, 0 if none

typedef struct VState {  // the coding side of the current VOP
  Vop v;
  int mb_w, prev_dq, dc_prev[2], run;  // run: macroblocks of the same kind (skip, direct) before this one
  const U8* scan;  // scan order (as far as known: intra blocks with AC prediction use other ones)
} VState;

static const U8 zigzag_direct[64] = {
   0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };
static const U8 alternate_vertical_scan[64] = {
   0,  8, 16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
  41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
  51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
  53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63 };

static int nz_bucket(int n) { return n < 3 ? n : n < 6 ? 3 : n < 11 ? 4 : 5; }
static int few_bucket(int n) { return n < 3 ? n : n < 6 ? 3 : 4; }

static const U8 pos_bucket[64] = {
  0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

static void model_init(void) {
  Prob* p = (Prob*)&model;
  size_t i;
  for (i = 0; i < sizeof(model) / sizeof(Prob); i++) { p[i].p = 32768; p[i].n = 0; }
  memset(&last, 0, sizeof(last));
  last.tib = 1;
  for (i = 0; i < 5; i++) { last.quant[i] = 4; last.fcode[i] = 1; }
  last.bcode = 1;
  for (i = 0; i <= PLIMIT; i++) rate[i] = (int)(65536.0 / (i + 1.5));
  bitcost[0] = 16;
  for (i = 1; i <= 4096; i++) bitcost[i] = (float)(-log((double)i / 4096) / log(2.0));
  {  // squash: interpolated from a table, stretch: its inverse (integers only, as in lpaq)
    static const int t[33] = { 1,    2,    3,    6,    10,   16,   27,   45,   73,   120,  194,
                               310,  488,  747,  1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785,
                               3901, 3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094 };
    int d, pi = 0, j;
    squash_t[0] = 1;
    for (d = -2047; d <= 2047; d++) {
      const int w = d & 127, k = (d >> 7) + 16;
      squash_t[d + 2048] = (short)((t[k] * (128 - w) + t[k + 1] * w + 64) >> 7);
    }
    for (d = -2047; d <= 2047; d++) {
      const int v = squash_t[d + 2048];
      for (j = pi; j <= v; j++) stretch_t[j] = (short)d;
      pi = v + 1;
    }
    for (j = pi; j < 4096; j++) stretch_t[j] = 2047;
  }
  {
    int* w = (int*)&mixw;
    for (i = 0; i < sizeof(mixw) / sizeof(int); i++) w[i] = i % (MAXIN + 1) ? MIX_W0 : 0;
  }
  apm_init((Apm*)&apms, sizeof(apms) / sizeof(Apm));
  ref_mbs = 0;
}

static int code_kind(Coder* c, int k) {  // 0 end, 1 VOP as records, 2 raw bytes
  Prob* t = model.kind[last.kind];
  c->cat = CAT_RAW;
  k = cbit(c, &t[0], k == 1) ? 1 : cbit(c, &t[1], k == 2) ? 2 : 0;
  return last.kind = k;
}

static size_t code_raw(Coder* c, U8* p, size_t n, size_t max) {  // a chunk kept as bytes; 0 on errors
  size_t i;
  c->cat = CAT_RAW;
  n = (size_t)code_eg(c, model.rawlen, 4, (int)n);
  if (n > max) return 0;
  for (i = 0; i < n; i++) {  // bytes: order 0 and order 1 contexts, mixed
    int x = 1, k;
    for (k = 7; k >= 0; k--) {
      Prob* in[2];
      in[0] = &model.raw0[x];
      in[1] = &model.raw[last.rawprev][x];
      x = x * 2 + mbit(c, in, 2, mixw.raw[k], (p[i] >> k) & 1);
    }
    last.rawprev = p[i] = (U8)(x - 256);
  }
  return n;
}

static void code_vop_hdr(Coder* c, const Ctx* cx, VopHdr* h) {
  Model* M = &model;
  const int t0 = last.type;
  int i, d, mask;
  c->cat = CAT_HDR;
  h->type = PT_I + code_bits(c, M->vtype[t0], 2, h->type - PT_I);
  h->mtb = code_eg(c, M->mtb, 0, h->mtb);
  h->tib = cbit(c, &M->tib_same, h->tib == last.tib) ? last.tib : 1 + code_bits(c, M->tib, 4, h->tib - 1);
  mask = (int)((1u << h->tib) - 1);
  d = (h->tinc - last.tinc) & mask;
  d = cbit(c, &M->tinc_same[h->type], d == (last.dinc[h->type] & mask)) ? last.dinc[h->type] & mask
                                                                         : code_bitsi(c, M->tinc, h->tib, d);
  h->tinc = (last.tinc + d) & mask;
  last.type = h->type;
  last.tib = h->tib;
  last.tinc = h->tinc;
  last.dinc[h->type] = d;
  h->coded = cbit(c, &M->coded[h->type], h->coded);
  if (!c->enc) {
    h->rnd = h->dcthr = h->tff = h->alt = h->quant = h->fcode = h->bcode = NA;
    h->ncplx = 0;
  }
  if (!h->coded) return;
  if (h->type == PT_P || (h->type == PT_S && cx->sprite_usage == GMC_SPRITE))
    last.rnd = h->rnd = cbit(c, &M->rnd[last.rnd], h->rnd);
  h->ncplx = cx->cplx_i + (h->type != PT_I ? cx->cplx_p : 0) + (h->type == PT_B ? cx->cplx_b : 0);
  for (i = 0; i < h->ncplx; i++) h->cplx[i] = (char)('0' + cbit(c, &M->cplx[i < 255 ? i : 255], h->cplx[i] == '1'));
  last.dcthr = h->dcthr =
      cbit(c, &M->dcthr_same, h->dcthr == last.dcthr) ? last.dcthr : code_bits(c, M->dcthr, 3, h->dcthr);
  if (cx->interlaced) {
    last.tff = h->tff = cbit(c, &M->tff[last.tff], h->tff);
    last.alt = h->alt = cbit(c, &M->alt[last.alt], h->alt);
  }
  if (h->type == PT_S && cx->sprite_usage == GMC_SPRITE)
    for (i = 0; i < 2 * cx->warp_points; i++) h->warp[i] = code_seg(c, M->warp[i & 1], h->warp[i]);
  h->quant = last.quant[h->type] + code_seg(c, M->quant[h->type], h->quant - last.quant[h->type]);
  last.quant[h->type] = h->quant;
  if (h->type != PT_I) {
    h->fcode = cbit(c, &M->fcode_same[h->type], h->fcode == last.fcode[h->type]) ? last.fcode[h->type]
                                                                                 : code_bits(c, M->fcode, 3, h->fcode);
    last.fcode[h->type] = h->fcode;
  }
  if (h->type == PT_B) {
    h->bcode = cbit(c, &M->bcode_same, h->bcode == last.bcode) ? last.bcode : code_bits(c, M->bcode, 3, h->bcode);
    last.bcode = h->bcode;
  }
}

static void code_vp(Coder* c, const Ctx* cx, VState* s, VpHdr* k, int expect) {
  Model* M = &model;
  Vop* v = &s->v;
  int i;
  c->cat = CAT_HDR;
  k->st = code_eg(c, M->vpst, 0, k->st);
  k->mbnum =
      cbit(c, &M->vpmb_exp, k->mbnum == expect) ? expect : code_bitsi(c, M->vpmb, cx->mb_num_bits, k->mbnum);
  k->quant = v->qscale + code_seg(c, M->vpq, k->quant - v->qscale);
  k->hec = cbit(c, &M->hec, k->hec);
  if (k->quant) v->qscale = k->quant;
  v->qscale = clipi(v->qscale, 1, 31);
  if (!c->enc) k->mtb = k->tinc = k->type = k->dcthr = k->fcode = k->bcode = NA;
  if (!k->hec) return;
  k->mtb = code_eg(c, M->hmtb, 0, k->mtb);
  k->tinc = code_bitsi(c, M->htinc, v->tib, k->tinc);
  k->type = code_bits(c, M->htype, 2, k->type);
  k->dcthr = code_bits(c, M->hdcthr, 3, k->dcthr);
  if (v->gmc) for (i = 0; i < 2 * cx->warp_points; i++) k->warp[i] = code_seg(c, M->hwarp, k->warp[i]);
  if (v->type != PT_I) k->fcode = code_bits(c, M->hfcode, 3, k->fcode);
  if (v->type == PT_B) k->bcode = code_bits(c, M->hbcode, 3, k->bcode);
}

// what follows the last macroblock: nb bits up to the byte boundary, then *n bytes
static int code_tail(Coder* c, int nb, int* bits, int* n, U8* bytes, int max) {
  Model* M = &model;
  int i, canon = 0;
  c->cat = CAT_HDR;
  if (c->enc) canon = nb ? *bits == (1 << (nb - 1)) - 1 && !*n : *n == 1 && bytes[0] == 0x7F;
  if (cbit(c, &M->tail_canon, canon)) {  // the usual stuffing: 0, then 1s to the byte boundary
    if (nb) { *bits = (1 << (nb - 1)) - 1; *n = 0; }
    else { *bits = 0; *n = 1; bytes[0] = 0x7F; }
    return 0;
  }
  *bits = code_bitsi(c, M->tailb, nb, *bits);
  *n = code_eg(c, M->tailn, 0, *n);
  if (*n > max) return -1;
  for (i = 0; i < *n; i++) bytes[i] = (U8)code_bits(c, M->tailbyte, 8, bytes[i]);
  return 0;
}

// intra DC difference: size, the bits below its leading one, sign. Contexts for the size: the
// previous block's, the left and above blocks', whether the block has AC coefficients.
static int code_dc(Coder* c, VState* s, int lc, int v, const BlkCtx* Lb, const BlkCtx* Ab, int coded,
                   int qctx) {
  Model* M = &model;
  const int ctx = s->dc_prev[lc] < 7 ? s->dc_prev[lc] : 7, a = v < 0 ? -v : v;
  const int ls = Lb && Lb->dcs >= 0 ? Lb->dcs : 9, as = Ab && Ab->dcs >= 0 ? Ab->dcs : 9;
  Prob* in[MAXIN];
  int size = bitlen(a), k, x;
  for (k = 0; k < 9; k++) {
    in[0] = &M->dcs[lc][ctx][k];
    in[1] = &M->dcs2[lc][ls < 8 ? ls : 8][as < 8 ? as : 8][k];
    in[2] = &M->dcs3[lc][coded][qctx][k];
    if (!mbit(c, in, 3, mixw.dcs[lc][k], size > k)) break;
  }
  s->dc_prev[lc] = size = k;
  if (!size) return 0;
  x = 1 << (size - 1);
  for (k = size - 2; k >= 0; k--) x |= cbit(c, &M->dcm[lc][size][k], (a >> k) & 1) << k;
  return cbit(c, &M->dcsign[lc][size > 4], v < 0) ? -x : x;
}

// Coefficients of a block, coded like H.264's CABAC residual: significance map in scan order,
// then the levels in reverse order, signs, and the escape modes where they aren't the default.
// Each decision mixes contexts: the neighbouring blocks' coefficient counts (CAVLC's nC), the
// coefficients found so far, and the left and above blocks' coefficients at the same position.
// cls: block class, nctx: neighbours' counts, qctx/q8: quantiser, mvc: motion vector size, rnz: the
// co-located block's count; Lb, Ab: left and above blocks or 0; out (which may be Ab) gets this one
static void code_block(Coder* c, VState* s, Blk* b, int n, int intra, int dcvlc, int coded, int cls, int nctx,
                       int qctx, int q8, int mvc, int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx* out) {
  Model* M = &model;
  const RL* rl = intra ? &rl_intra : &rl_inter;
  Prob* in[MAXIN];
  const U8* sc = s->scan;
  BlkCtx o;
  U8 rm[64];  // coefficients coded so far by raster position: 1 in the significance map, then magnitudes
  U8 nzr[64];  // the significance map by raster position
  int pos[64], lev[64], k = 0, i, j, start, prev;
  memset(&o, 0, sizeof(o));
  memset(rm, 0, sizeof(rm));
  o.last = o.dcs = -1;
  if (dcvlc) {
    c->cat = CAT_DC;
    b->dc = code_dc(c, s, n < 4 ? 0 : 1, b->dc, Lb, Ab, coded, qctx);
    o.dcs = (S8)bitlen(b->dc);
  } else if (!c->enc)
    b->dc = 0;
  if (!coded) { b->n = 0; *out = o; return; }
  start = dcvlc ? 1 : 0;
  if (c->enc)
    for (j = 0, prev = start - 1; j < b->n; j++) { pos[j] = prev += b->run[j] + 1; lev[j] = b->level[j]; }
  c->cat = CAT_SIG;
  for (i = start, prev = start - 1; i < 63; i++) {
    const int kb = k < 7 ? k : 7, pb = pos_bucket[i], run = i - prev - 1;
    const int nz = (Lb ? Lb->mag[i] != 0 : 2) * 3 + (Ab ? Ab->mag[i] != 0 : 2);
    const int past = (Lb ? i > Lb->last : 2) * 3 + (Ab ? i > Ab->last : 2);  // beyond their last coefficient
    in[0] = &M->sig[cls][nctx][qctx][i];
    in[1] = &M->sig2[cls][kb][i];
    in[2] = &M->sig3[cls][nz][i];
    in[3] = &M->sig4[cls][past][qctx][i];
    in[4] = &M->sig5[cls][run < 3 ? run : run < 5 ? 3 : run < 9 ? 4 : 5][i];
    {  // the frequency neighbours left, above and above left
      const int r = sc[i], u = r & 7, v = r >> 3;
      in[5] = &M->sig6[cls][(u ? rm[r - 1] : 0) + (v ? rm[r - 8] : 0) + (u && v ? rm[r - 9] : 0)][i];
    }
    in[6] = &M->sig7[cls][mvc][i];
    in[7] = &M->sig8[cls][rnz][i];
    if (!mbit2(c, in, 8, mixw.sig[cls][i], mixw.sigb[cls][kb][qctx], &apms.sig[cls][i],
               c->enc && k < b->n && pos[k] == i))
      continue;
    pos[k] = prev = i;
    rm[sc[i]] = 1;
    in[0] = &M->last[cls][nctx][qctx][i];
    in[1] = &M->last2[cls][kb][i];
    in[2] = &M->last3[cls][nz][i];
    in[3] = &M->last4[cls][past][qctx][i];
    in[4] = &M->last5[cls][run < 3 ? run : run < 5 ? 3 : run < 9 ? 4 : 5][i];
    in[5] = &M->last6[cls][mvc][i];
    in[6] = &M->last7[cls][rnz][i];
    in[7] = &M->last8[cls][Lb ? few_bucket(Lb->suf[i + 1]) : 5][Ab ? few_bucket(Ab->suf[i + 1]) : 5][pb];
    if (mbit2(c, in, 8, mixw.last[cls][i], mixw.lastb[cls][kb][qctx], &apms.last[cls][i], c->enc && k == b->n - 1)) {
      k++;
      break;
    }
    k++;
  }
  if (i == 63) pos[k++] = 63;  // reached the end: the last coefficient is there
  rm[sc[pos[k - 1]]] = 1;
  memcpy(nzr, rm, sizeof(rm));
  memset(rm, 0, sizeof(rm));
  c->cat = CAT_LEV;
  {
    int ngt1 = 0, neq1 = 0, pm = 0;
    const int kc = k < 4 ? k - 1 : k < 6 ? 3 : k < 10 ? 4 : k < 16 ? 5 : k < 24 ? 6 : 7;
    for (j = k - 1; j >= 0; j--) {
      const int a = c->enc ? (lev[j] < 0 ? -lev[j] : lev[j]) - 1 : 0, p = pos[j], pb = pos_bucket[p];
      const int nm = (Lb ? Lb->mag[p] : 0) + (Ab ? Ab->mag[p] : 0);
      const int nmb = nm < 3 ? nm : nm < 5 ? 3 : 4;
      const int g = ngt1 ? 0 : 1 + (neq1 < 3 ? neq1 : 3);
      int t, nb2, den;
      in[0] = &M->lev1[cls][g][pb][qctx];
      in[1] = &M->lev1b[cls][nmb][pb];
      in[2] = &M->lev1c[cls][g][kc];
      in[3] = &M->lev1d[cls][pm][pb][qctx];
      {  // the frequency neighbours right, below and below right, coded already
        const int r = sc[p], u = r & 7, v = r >> 3;
        const int sum = (u < 7 ? rm[r + 1] : 0) + (v < 7 ? rm[r + 8] : 0) + (u < 7 && v < 7 ? rm[r + 9] : 0);
        den = (u < 7 ? nzr[r + 1] : 0) + (v < 7 ? nzr[r + 8] : 0) + (u ? nzr[r - 1] : 0) + (v ? nzr[r - 8] : 0);
        in[4] = &M->lev1e[cls][sum < 3 ? sum : sum < 5 ? 3 : 4][pb];
        in[5] = &M->lev1f[cls][den][pb];
        nb2 = sum < 3 ? sum : sum < 5 ? 3 : 4;
      }
      in[6] = &M->lev1g[cls][mvc][pb];
      in[7] = &M->lev1q[cls][q8][pb];
      t = mbit2(c, in, 8, mixw.lev1[cls][pb][g], mixw.lev1b[cls][g][qctx], &apms.lev1[cls][pb][g < 3 ? g : 3], a > 0);
      if (t) {  // |level| - 1 in unary up to LUNARY, then the size of a - LUNARY + 2 and its lower bits
        for (t = 1; t < LUNARY - 1; t++) {
          const int u = t <= LUN ? t - 1 : LUN - 1;
          in[0] = &M->levn[cls][ngt1 < 4 ? ngt1 : 4][qctx][u];
          in[1] = &M->levnb[cls][nmb][u];
          in[2] = &M->levnc[cls][nb2][u];
          in[3] = &M->levnd[cls][pb][u];
          in[4] = &M->levne[cls][den][u];
          in[5] = &M->levnq[cls][q8][u];
          if (!mbit(c, in, 6, mixw.levn[cls][u][nb2], a > t)) break;
        }
        if (t == LUNARY - 1) {
          const int v = a - LUNARY + 2;
          int e, x;
          for (e = 0; e < 11; e++) {
            in[0] = &M->lexp[cls][qctx][e];
            in[1] = &M->lexp2[cls][nb2 > nmb ? nb2 : nmb][e];
            in[2] = &M->lexp3[cls][pb][e];
            if (!mbit(c, in, 3, mixw.lexp[cls][e], bitlen(v) > e + 1)) break;
          }
          x = 1 << e;
          for (e--; e >= 0; e--) x |= cbit(c, &M->lman[cls][bitlen(x)][e], (v >> e) & 1) << e;
          t = LUNARY - 2 + x;
        }
        ngt1++;
      } else
        neq1++;
      lev[j] = t + 1;
      pm = t < 3 ? t + 1 : 3;
      rm[sc[p]] = o.mag[p] = (U8)(lev[j] < 255 ? lev[j] : 255);
    }
  }
  o.last = (S8)pos[k - 1];
  o.n = (U8)k;
  for (i = 63; i >= 0; i--) o.suf[i] = (U8)(o.suf[i + 1] + (o.mag[i] != 0));
  *out = o;
  c->cat = CAT_SIGN;
  for (j = 0; j < k; j++) {
    const int p = pos[j];
    const int ls = Lb && Lb->mag[p] ? Lb->sgn[p] : 2, as = Ab && Ab->mag[p] ? Ab->sgn[p] : 2;
    in[0] = &M->sign[cls][j == 0];
    in[1] = &M->sign2[cls][ls * 3 + as][pos_bucket[p]];
    if (mbit(c, in, 2, mixw.sign[cls][pos_bucket[p]], c->enc && b->level[j] < 0)) lev[j] = -lev[j];
    o.sgn[p] = lev[j] < 0;
  }
  c->cat = CAT_ESC;
  for (j = 0, prev = start - 1; j < k; j++) {
    const int run = pos[j] - prev - 1, al = lev[j] < 0 ? -lev[j] : lev[j];
    const int d = esc_default(rl, j == k - 1, run, al);
    int e = c->enc ? b->esc[j] : d;
    if (!cbit(c, &M->escd[d], e == d)) e = code_bits(c, M->escm[d], 2, e);
    if (!c->enc) { b->run[j] = (U8)run; b->level[j] = (S16)lev[j]; b->esc[j] = (U8)e; }
    prev = pos[j];
  }
  if (!c->enc) b->n = k;
}

// coded block pattern, block 0 in bit 5. Contexts: the left and above 8x8 blocks (in this
// macroblock or a neighbour), the size of the motion vector differences (mvc, 6 for none), the
// co-located macroblock of the last anchor VOP, the blocks coded so far.
static int code_cbp(Coder* c, int cls3, const MBCtx* L, const MBCtx* A, const MBCtx* R, int mvc, int cbp) {
  Model* M = &model;
  Prob* in[MAXIN];
  int b, v = 0, n = 0;
  for (b = 0; b < 6; b++) {
    int l, u, bit;
    if (b < 4) {
      l = (b & 1) ? (v >> (6 - b)) & 1 : L ? (L->cbp >> (4 - b)) & 1 : 2;
      u = (b & 2) ? (v >> (7 - b)) & 1 : A ? (A->cbp >> (3 - b)) & 1 : 2;
    } else {
      l = L ? (L->cbp >> (5 - b)) & 1 : 2;
      u = A ? (A->cbp >> (5 - b)) & 1 : 2;
    }
    in[0] = &M->cbp1[cls3][b][l * 3 + u];
    in[1] = &M->cbp2[cls3][b][mvc];
    in[2] = &M->cbp3[cls3][b][R ? (R->cbp >> (5 - b)) & 1 : 2][n];
    bit = mbit(c, in, 3, mixw.cbp[cls3][b], (cbp >> (5 - b)) & 1);
    v |= bit << (5 - b);
    n += bit;
  }
  return v;
}

// neighbouring blocks' coefficient counts (CAVLC's nC)
static int nz_ctx(const MBCtx* me, const MBCtx* L, const MBCtx* A, int b) {
  int na, nb, m;
  if (b < 4) {
    na = (b & 1) ? me->nz[b - 1] : L ? L->nz[b + 1] : -1;
    nb = (b & 2) ? me->nz[b - 2] : A ? A->nz[b + 2] : -1;
  } else {
    na = L ? L->nz[b] : -1;
    nb = A ? A->nz[b] : -1;
  }
  if (na < 0 && nb < 0) return 0;
  m = na < 0 ? nb : nb < 0 ? na : (na + nb + 1) >> 1;
  return m == 0 ? 1 : m <= 2 ? 2 : m <= 5 ? 3 : m <= 10 ? 4 : 5;
}

static int mv_bucket(int q) { return q < 3 ? q : q < 5 ? 3 : q < 9 ? 4 : 5; }

// A motion vector difference as coded, like CABAC's mvd: a zero flag, the magnitude in units of
// the VLC code (so f_code doesn't change its statistics) in unary, the f_code residual bits, the
// sign. Contexts: the neighbours' magnitudes, the other component (oth: of this vector for y, of
// the previous one for x, 6 for none), the co-located macroblock of the last anchor VOP.
static int code_mvd(Coder* c, int kind, int comp, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R,
                    int oth, int v, int* q) {
  Model* M = &model;
  const int shift = fcode - 1, a = v < 0 ? -v : v;
  const int ns = mv_bucket((L ? L->mva[comp] : 0) + (A ? A->mva[comp] : 0));
  const int rs = R ? mv_bucket(R->mva[comp]) : 6;
  Prob* in[MAXIN];
  int k, x;
  in[0] = &M->mvz[kind][comp][ns];
  in[1] = &M->mvz2[kind][comp][oth];
  in[2] = &M->mvz3[kind][comp][rs];
  if (!mbit(c, in, 3, mixw.mvz[kind][comp], a != 0)) { *q = 0; return 0; }
  x = a ? ((a - 1) >> shift) + 1 : 0;
  for (k = 1; k < 32; k++) {
    const int u = k < 12 ? k - 1 : 11;
    in[0] = &M->mvu[kind][comp][u][ns];
    in[1] = &M->mvu2[kind][comp][u][oth];
    in[2] = &M->mvu3[kind][comp][u][rs];
    if (!mbit(c, in, 3, mixw.mvu[kind][comp][u], x > k)) break;
  }
  *q = k;
  x = (((k - 1) << shift) | code_bitsi(c, M->mvr[kind][comp], shift, a ? (a - 1) & ((1 << shift) - 1) : 0)) + 1;
  return cbit(c, &M->mvs[kind][comp][L ? L->mvs[comp] : 0][A ? A->mvs[comp] : 0], v < 0) ? -x : x;
}

static void code_mvs(Coder* c, MB* m, int* mvi, int n, int kind, int fcode, const MBCtx* L, const MBCtx* A,
                     const MBCtx* R, MBCtx* me, int* sumq) {
  int i, q, qx = 6;
  for (i = 0; i < n; i++) {
    const int first = *mvi == 0;
    int v = code_mvd(c, kind, 0, fcode, L, A, R, qx, m->mv[*mvi], &q);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    qx = mv_bucket(q);
    if (first) { me->mva[0] = (U8)q; me->mvs[0] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
    v = code_mvd(c, kind, 1, fcode, L, A, R, qx, m->mv[*mvi], &q);
    m->mv[(*mvi)++] = v;
    *sumq += q;
    if (first) { me->mva[1] = (U8)q; me->mvs[1] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
  }
}

// one macroblock record; in decoding mode m is filled in
static void code_mb(Coder* c, const Ctx* cx, VState* s, MB* m, int idx) {
  Model* M = &model;
  Vop* v = &s->v;
  const int mb_x = idx % s->mb_w;
  const MBCtx* L = mb_x > 0 && mbcur[idx - 1].cls ? &mbcur[idx - 1] : 0;
  const MBCtx* A = idx >= s->mb_w && mbcur[idx - s->mb_w].cls ? &mbcur[idx - s->mb_w] : 0;
  const MBCtx* R = ref_mbs ? &mbref[idx] : 0;
  const MBCtx* AL = mb_x > 0 && idx > s->mb_w && mbcur[idx - s->mb_w - 1].cls ? &mbcur[idx - s->mb_w - 1] : 0;
  const MBCtx* AR =
      mb_x + 1 < s->mb_w && idx >= s->mb_w && mbcur[idx - s->mb_w + 1].cls ? &mbcur[idx - s->mb_w + 1] : 0;
  const int rb = s->run < 3 ? s->run : s->run < 6 ? 3 : s->run < 12 ? 4 : 5;
  MBCtx* me = &mbcur[idx];
  Prob* in[MAXIN];
  const int dcvlc = v->qscale < v->dcthr;
  int intra = 0, cbp, cls3, i, k, mvi = 0, sumq = 0;
  if (!c->enc) {
    m->st = m->sk = 0;
    m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
    m->nfs = m->fs = m->nmv = 0;
  }
  memset(me, 0, sizeof(*me));
  c->cat = CAT_MB;
  if (v->type == PT_I) {
    m->st = code_eg(c, M->st[0], 0, m->st);
    m->t = 2;
    intra = 1;
    cls3 = 0;
  } else if (v->type != PT_B) {
    m->st = code_eg(c, M->st[1], 0, m->st);
    k = (L && L->cls == C_SKIP) + (A && A->cls == C_SKIP) + 3 * (R && R->cls == C_SKIP);
    in[0] = &M->skip[v->gmc][k];
    in[1] = &M->skip2[v->gmc][(L && L->cls == C_SKIP) + (A && A->cls == C_SKIP) + (AL && AL->cls == C_SKIP) +
                              (AR && AR->cls == C_SKIP)][rb];
    in[2] = &M->skip3[v->gmc][R ? R->cls : 0][(L && L->cls == C_SKIP) + (A && A->cls == C_SKIP)];
    if ((m->sk = mbit(c, in, 3, mixw.mbt[0], m->sk))) {
      me->cls = C_SKIP;
      me->bsk = !v->gmc;
      rowb_clear(mb_x);
      s->run++;
      return;
    }
    s->run = 0;
    k = (L && L->cls == C_INTRA) + (A && A->cls == C_INTRA) + 3 * (R && R->cls == C_INTRA);
    in[0] = &M->pintra[k];
    in[1] = &M->pintra2[(L && L->cls == C_INTRA) + (A && A->cls == C_INTRA) + (AL && AL->cls == C_INTRA) +
                        (AR && AR->cls == C_INTRA)][!R ? 0 : R->cls == C_INTRA ? 1 : R->cls == C_SKIP ? 2 : 0];
    if (mbit(c, in, 2, mixw.mbt[1], m->t == 2)) {
      m->t = 2;
      intra = 1;
      cls3 = 0;
    } else {
      k = (L && L->cls == C_4MV) + (A && A->cls == C_4MV) + 3 * (R && R->cls == C_4MV);
      in[0] = &M->p4mv[k];
      in[1] = &M->p4mv2[(L && L->cls == C_4MV) + (A && A->cls == C_4MV) + (AL && AL->cls == C_4MV) +
                        (AR && AR->cls == C_4MV)][R ? (R->cls == C_4MV) + (R->cls == C_INTER) * 2 : 0];
      m->t = mbit(c, in, 2, mixw.mbt[2], m->t == 1);
      if (v->gmc && !m->t) m->mc = cbit(c, &M->mcsel[(L && L->cls == C_GMC) + (A && A->cls == C_GMC)], m->mc);
      cls3 = 1;
    }
  } else {
    if ((m->sk = cbit(c, &M->bskip[R ? R->bsk : 2], m->sk))) { me->cls = C_SKIP; rowb_clear(mb_x); return; }
    k = 3 * ((L && L->cls == C_BDIR0) + (A && A->cls == C_BDIR0)) +
        (!R || R->cls == C_SKIP ? 0 : R->cls == C_INTRA ? 2 : 1);
    in[0] = &M->modb0[k];
    in[1] = &M->modb02[rb][(L && L->cls == C_BDIR0) + (A && A->cls == C_BDIR0) + (AL && AL->cls == C_BDIR0) +
                           (AR && AR->cls == C_BDIR0)];
    in[2] = &M->modb03[R ? R->cls : 0][(L && L->cls == C_BDIR0) + (A && A->cls == C_BDIR0)];
    if (mbit(c, in, 3, mixw.mbt[3], m->mb == 0)) {
      m->mb = m->t = 0;
      for (i = 0; i < 6; i++) m->b[i].n = 0;
      me->cls = C_BDIR0;
      rowb_clear(mb_x);
      s->run++;
      return;
    }
    s->run = 0;
    m->mb = 1 + cbit(c, &M->modb2[(L && L->cbp) + (A && A->cbp)], m->mb == 2);
    k = 3 * ((L && L->cls == C_BDIR) + (A && A->cls == C_BDIR));
    in[0] = &M->btype[0][k + (L && L->cls == C_BFWD) + (A && A->cls == C_BFWD)];
    in[1] = &M->btype2[0][R ? R->cls : 0];
    if (mbit(c, in, 2, mixw.mbt[4], m->t == 0)) m->t = 0;
    else {
      in[0] = &M->btype[1][k + (L && L->cls == C_BFWD) + (A && A->cls == C_BFWD)];
      in[1] = &M->btype2[1][R ? R->cls : 0];
      if (mbit(c, in, 2, mixw.mbt[5], m->t == 3)) m->t = 3;
      else {
        in[0] = &M->btype[2][k + (L && L->cls == C_BBWD) + (A && A->cls == C_BBWD)];
        in[1] = &M->btype2[2][R ? R->cls : 0];
        m->t = mbit(c, in, 2, mixw.mbt[6], m->t == 2) ? 2 : 1;
      }
    }
    cls3 = 2;
  }
  if (intra) m->ac = cbit(c, &M->ac[v->type == PT_I][(L && L->ac) + (A && A->ac)], m->ac);
  if (!intra && cx->interlaced && (v->type == PT_B ? m->t != 0 : !m->t && m->mc != 1)) {
    m->fp = cbit(c, &M->fp[v->type == PT_B][(L && L->fp) + (A && A->fp)], m->fp);
    if (m->fp == 1) {
      m->nfs = v->type == PT_B ? 2 * (fwd_dir(m->t) + bwd_dir(m->t)) : 2;
      m->fs = code_bitsi(c, M->fs, m->nfs, m->fs);
    }
  }
  c->cat = CAT_MV;
  if (!intra) {
    if (v->type != PT_B) {
      if (m->t == 1) code_mvs(c, m, &mvi, 4, 5, v->fcode, L, A, R, me, &sumq);
      else if (m->mc != 1) code_mvs(c, m, &mvi, m->fp == 1 ? 2 : 1, m->fp == 1, v->fcode, L, A, R, me, &sumq);
    } else if (m->t) {
      if (fwd_dir(m->t)) code_mvs(c, m, &mvi, m->fp == 1 ? 2 : 1, 2, v->fcode, L, A, R, me, &sumq);
      if (bwd_dir(m->t)) code_mvs(c, m, &mvi, m->fp == 1 ? 2 : 1, 3, v->bcode, L, A, R, me, &sumq);
    } else
      code_mvs(c, m, &mvi, 1, 4, 1, L, A, R, me, &sumq);
  }
  c->cat = CAT_CBP;
  cbp = v->type == PT_B && m->mb == 1 ? 0
      : code_cbp(c, cls3, L, A, R, mvi ? mv_bucket(sumq) : 6, c->enc ? cbp_of(m) : 0);
  c->cat = CAT_MB;
  if (v->type != PT_B) {
    k = cbit(c, &M->dqz[cls3][s->prev_dq], m->dq != 0);
    m->dq = k ? quant_tab[code_bits(c, M->dqv[cls3], 2, k ? dq_index(m->dq) : 0)] : 0;
    s->prev_dq = k;
    v->qscale = clipi(v->qscale + m->dq, 1, 31);
  } else if (m->t && cbp) {
    k = cbit(c, &M->bdqz[s->prev_dq], m->dq != 0);
    m->dq = k ? (cbit(c, &M->bdqs, m->dq > 0) ? 2 : -2) : 0;
    s->prev_dq = k;
    v->qscale = clipi(v->qscale + m->dq, 1, 31);
  }
  if (cx->interlaced && (intra || cbp)) m->dt = cbit(c, &M->dt[intra][(L && L->dt) + (A && A->dt)], m->dt);
  m->nmv = mvi;
  {
    const int qctx = v->qscale <= 4 ? 0 : v->qscale <= 10 ? 1 : 2;
    const int q8 = v->qscale < 3 ? 0 : v->qscale < 4 ? 1 : v->qscale < 5 ? 2 : v->qscale < 7 ? 3 : v->qscale < 9 ? 4
                 : v->qscale < 12 ? 5 : v->qscale < 17 ? 6 : 7;
    for (i = 0; i < 6; i++) {
      const int cls = (intra ? (m->ac == 1 ? 6 : 4) : v->type == PT_B ? 2 : 0) + (i >= 4);
      const BlkCtx* Lb = i == 1 || i == 3 ? &rowb[mb_x][i - 1] : !L ? 0 : &rowb[mb_x - 1][i < 4 ? i + 1 : i];
      const BlkCtx* Ab = i == 2 || i == 3 ? &rowb[mb_x][i - 2] : !A ? 0 : &rowb[mb_x][i < 4 ? i + 2 : i];
      code_block(c, s, &m->b[i], i, intra, intra && dcvlc, (cbp >> (5 - i)) & 1, cls, nz_ctx(me, L, A, i), qctx, q8,
                 mvi ? mv_bucket(sumq) : 6, R ? nz_bucket(R->nz[i]) : 6, Lb, Ab, &rowb[mb_x][i]);
      me->nz[i] = (U8)(m->b[i].n < 63 ? m->b[i].n : 63);
    }
  }
  me->cbp = (U8)cbp;
  me->ac = intra && m->ac == 1;
  me->dt = m->dt == 1;
  me->fp = m->fp == 1;
  if (intra) me->cls = C_INTRA;
  else if (v->type != PT_B) me->cls = m->t == 1 ? C_4MV : m->mc == 1 ? C_GMC : C_INTER;
  else me->cls = m->t == 0 ? C_BDIR : m->t == 3 ? C_BFWD : m->t == 2 ? C_BBWD : C_BBI;
}

// can the models code everything in this macroblock (escape mode 3 allows a level of 0)
static int mb_codable(const MB* m) {
  int i, j;
  for (i = 0; i < 6; i++)
    for (j = 0; j < m->b[i].n; j++)
      if (!m->b[i].level[j]) return 0;
  return 1;
}

//--- VOPs ----------------------------------------------------------------------------------------

#define TAIL_MAX (1u << 20)
static U8 tailbuf[TAIL_MAX];

enum { W_VERIFY, W_ENCODE, W_DECODE };

static void vop_state(Vop* v, const Ctx* cx, const VopHdr* h) {
  v->type = h->type;
  v->gmc = h->type == PT_S && cx->sprite_usage == GMC_SPRITE;
  v->fcode = h->fcode == NA ? 1 : h->fcode;
  v->bcode = h->bcode == NA ? 1 : h->bcode;
  v->qscale = clipi(h->quant, 1, 31);
  v->dcthr = dc_threshold[h->dcthr];
  v->tib = h->tib;
}

// One VOP (after its start code). W_VERIFY: parse it and write it again (into pb); W_ENCODE: parse
// it and code the records; W_DECODE: decode the records and write the VOP (with its start code).
static int vop_walk(Ctx* cx, int mode, GB* gb, PB* pb, Coder* c) {
  static MB m;
  VopHdr h;
  Vop vp, vw;
  VState s;
  int mb_x = 0, mb_y = 0, anchor, ret = 0, nb, tbits = 0, tn = 0, i;
  double mb_bits = 0;
  memset(&h, 0, sizeof(h));
  memset(&s, 0, sizeof(s));
  if (mode != W_DECODE && parse_vop_hdr(cx, gb, &h) < 0) return -1;
  if (mode != W_VERIFY) code_vop_hdr(c, cx, &h);
  if (mode != W_ENCODE && put_vop_hdr(cx, pb, &h) < 0) return -1;
  if (h.coded) {
    if (!cx->have_vol || cx->unsupported || !cx->mb_num || cx->mb_num > MAX_MBS) return -1;
    vop_state(&vp, cx, &h);
    vw = s.v = vp;
    s.mb_w = cx->mb_w;
    s.scan = h.alt == 1 ? alternate_vertical_scan : zigzag_direct;
    anchor = h.type != PT_B;
    if (mode != W_DECODE && anchor) memset(skip_cur, 0, cx->mb_num + 1);
    if (mode != W_VERIFY) {
      memset(mbcur, 0, cx->mb_num * sizeof(*mbcur));
      if (ref_mbs != cx->mb_num) ref_mbs = 0;
    }
    for (;;) {
      const int idx = mb_y * cx->mb_w + mb_x;
      m.x = mb_x;
      m.y = mb_y;
      if (mode != W_DECODE) {
        const double b0 = pbits[0], b1 = pbits[1], b2 = pbits[2];
        const int p0 = gb->pos;
        if (parse_mb(cx, gb, &vp, &m, idx) < 0 || (mode == W_VERIFY && !mb_codable(&m))) { ret = -1; break; }
        if (mode == W_ENCODE && c->stats) {
          c->orig[CAT_MV] += pbits[0] - b0;
          c->orig[CAT_DC] += pbits[1] - b1;
          c->orig[CAT_SIG] += pbits[2] - b2;
          c->orig[CAT_MB] += gb->pos - p0 - (pbits[0] - b0) - (pbits[1] - b1) - (pbits[2] - b2);
          mb_bits += gb->pos - p0;
        }
        if (anchor) skip_cur[idx] = (U8)(m.sk == 1 && !vp.gmc);
      }
      if (mode != W_VERIFY) code_mb(c, cx, &s, &m, idx);
      if (mode != W_ENCODE && put_mb(cx, pb, &vw, &m) < 0) { ret = -1; break; }
      if (mode == W_DECODE && (pb->err || c->bytes > 16)) { ret = -1; break; }  // damaged data
      if (++mb_x == cx->mb_w) { mb_x = 0; mb_y++; }
      if (mb_y >= cx->mb_h) break;
      {
        VpHdr k;
        int vpf = 0, nstuff = 0, next;
        if (mode != W_DECODE) {
          next = resync_ahead(cx, gb, &vp, &nstuff);
          vpf = next && !(idx + 1 < next && vp.type == PT_B && skip_ref[idx + 1]);
        }
        if (mode != W_VERIFY) { c->cat = CAT_HDR; vpf = cbit(c, &model.vpflag, vpf); }
        if (!vpf) continue;
        if (mode != W_DECODE) {
          if (nstuff && vp.type == PT_S) { ret = -1; break; }  // not MCBPC stuffing there
          k.st = nstuff;
          gb_skip(gb, nstuff * (8 + vp.type));
          gb_skip(gb, 1);
          gb_align(gb);
          if (parse_vp(cx, gb, &vp, &k) < 0) { ret = -1; break; }
          vp.qscale = clipi(vp.qscale, 1, 31);
        }
        if (mode != W_VERIFY) code_vp(c, cx, &s, &k, idx + 1);
        if ((mode != W_ENCODE && put_vp(cx, pb, &vw, &k) < 0) || k.mbnum < 1 || k.mbnum >= cx->mb_num) {
          ret = -1;
          break;
        }
        mb_x = k.mbnum % cx->mb_w;
        mb_y = k.mbnum / cx->mb_w;
      }
    }
    if (mode != W_DECODE && anchor) memcpy(skip_ref, skip_cur, cx->mb_num + 1);
    if (ret < 0) return -1;
    if (mode != W_VERIFY && anchor) {
      memcpy(mbref, mbcur, cx->mb_num * sizeof(*mbcur));
      ref_mbs = cx->mb_num;
    }
  }
  // the rest: stuffing, normally
  if (mode != W_DECODE) {
    if (gb->pos > gb->size) return -1;
    nb = (8 - (gb->pos & 7)) & 7;
    tbits = (int)gb_get(gb, nb);
    tn = (gb->size - gb->pos) >> 3;
    if (tn > (int)TAIL_MAX) return -1;
    memcpy(tailbuf, gb->buf + (gb->pos >> 3), (size_t)tn);
  } else
    nb = (8 - pb->n) & 7;
  if (mode != W_VERIFY && code_tail(c, nb, &tbits, &tn, tailbuf, TAIL_MAX) < 0) return -1;
  if (mode == W_ENCODE && c->stats) c->orig[CAT_HDR] += gb->size + 32 - mb_bits;
  if (mode != W_ENCODE) {
    pb_put(pb, (U32)tbits, nb);
    for (i = 0; i < tn; i++) pb_put(pb, tailbuf[i], 8);
  }
  return 0;
}

//--- container -----------------------------------------------------------------------------------

static U32 crc_tab[256];
static void crc_init(void) {
  U32 i, k, v;
  for (i = 0; i < 256; i++) {
    for (v = i, k = 0; k < 8; k++) v = (v >> 1) ^ (0xEDB88320u & (0u - (v & 1)));
    crc_tab[i] = v;
  }
}
static U32 crc32_upd(U32 crc, const U8* p, size_t n) {
  crc = ~crc;
  while (n--) crc = crc_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
  return ~crc;
}

typedef struct Stats {
  U32 vops, packed, by_type[5], raw_unsup, raw_long, raw_parse, raw_verify;
  double in_bytes, raw_bytes, vop_bytes;
} Stats;

static const U8 magic[8] = { 'm', '4', 'v', 'p', 'a', 'c', 'k', 1 };

static void put_le(U8* p, U64 v, int n) { int i; for (i = 0; i < n; i++) p[i] = (U8)(v >> (8 * i)); }
static U64 get_le(const U8* p, int n) { U64 v = 0; int i; for (i = n - 1; i >= 0; i--) v = (v << 8) | p[i]; return v; }

// header: magic, input size (8 bytes), CRC-32 of the input; then the arithmetic coded data
static int compress(FILE* in, FILE* out, Ctx* cx, Stats* st, Coder* c, int stats) {
  static Reader rd;
  U8 hdr[20];
  size_t pos, len;
  int part;
  U32 crc = 0;
  memset(&rd, 0, sizeof(rd));
  rd.f = in;
  memset(hdr, 0, sizeof(hdr));
  if (fwrite(hdr, 1, sizeof(hdr), out) != sizeof(hdr)) return -1;
  co_init(c, out, 1);
  c->stats = stats;
  while (rd_chunk(&rd, &pos, &len, &part)) {
    U8* p = ibuf + pos;
    const int is_vop = len >= 4 && !p[0] && !p[1] && p[2] == 1 && p[3] == 0xB6;
    crc = crc32_upd(crc, p, len);
    st->in_bytes += (double)len;
    if (is_vop && part != 2) {
      st->vops++;
      if (len >= 5) st->by_type[(p[4] >> 6) + PT_I]++;
      if (part) st->raw_long++;
      else if (!cx->have_vol || cx->unsupported || (cx->dp && (len < 5 || (p[4] >> 6) != 2))) st->raw_unsup++;
      else {
        const int tib = cx->time_inc_bits;
        GB gb;
        PB pb;
        gb.buf = p + 4;
        gb.size = (int)(len - 4) * 8;
        gb.pos = 0;
        pb_init(&pb, obuf, OBUF_SIZE);
        if (vop_walk(cx, W_VERIFY, &gb, &pb, 0) < 0) st->raw_parse++;
        else if (pb.err || pb.n || pb.len != len || memcmp(obuf, p, len)) st->raw_verify++;
        else {
          cx->time_inc_bits = tib;  // the parse again sees the same state
          gb.pos = 0;
          code_kind(c, 1);
          if (vop_walk(cx, W_ENCODE, &gb, 0, c) < 0) return -2;  // can't happen: verified above
          st->packed++;
          st->vop_bytes += (double)len;
          continue;
        }
      }
    }
    if (is_vop) st->raw_bytes += (double)len;
    c->orig[CAT_RAW] += 8.0 * len;
    code_kind(c, 2);
    code_raw(c, p, len, len);
    parse_header_chunk(cx, p, len);
  }
  code_kind(c, 0);
  co_flush(c);
  if (c->err || ferror(in)) return -1;
  memcpy(hdr, magic, 8);
  put_le(hdr + 8, (U64)st->in_bytes, 8);
  put_le(hdr + 16, crc, 4);
  if (fseek(out, 0, SEEK_SET) || fwrite(hdr, 1, sizeof(hdr), out) != sizeof(hdr)) return -1;
  return 0;
}

static int decompress(FILE* in, Sink* out, Ctx* cx, Coder* c) {
  U8 hdr[20];
  U64 size, done = 0;
  U32 crc = 0;
  int kind;
  if (fread(hdr, 1, sizeof(hdr), in) != sizeof(hdr) || memcmp(hdr, magic, 8)) return -1;
  size = get_le(hdr + 8, 8);
  co_init(c, in, 0);
  while ((kind = code_kind(c, 0)) != 0) {
    const U64 left = size - done;  // what the header announces; more means damaged data
    if (kind == 1) {
      PB pb;
      pb_init(&pb, obuf, left < OBUF_SIZE ? (size_t)left : OBUF_SIZE);
      if (vop_walk(cx, W_DECODE, 0, &pb, c) < 0 || pb.err || pb.n) return -2;
      if (!sink_write(out, obuf, pb.len)) return -3;
      crc = crc32_upd(crc, obuf, pb.len);
      done += pb.len;
    } else {
      const size_t n = code_raw(c, ibuf, 0, left < IBUF_SIZE ? (size_t)left : IBUF_SIZE);
      if (!n || c->bytes > 16) return -2;
      memset(ibuf + n, 0, 64);
      parse_header_chunk(cx, ibuf, n);
      if (!sink_write(out, ibuf, n)) return -3;
      crc = crc32_upd(crc, ibuf, n);
      done += n;
    }
    if (done > size || c->bytes > 16) return -2;
  }
  if (ferror(in)) return -4;
  if (done != size || crc != (U32)get_le(hdr + 16, 4)) return -2;
  return 0;
}

//--- frontend ------------------------------------------------------------------------------------

static char obuf_file[1 << 20];

int main(int argc, char** argv) {
  static Ctx ctx;
  static Stats st;
  static Coder co;
  FILE *in, *out;
  int a = 2, quiet = 0, verify = 0, stats = 0, mode, r, i;
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) a = argc;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-q")) quiet = 1;
    else if (!strcmp(argv[a], "-v")) verify = 1;
    else if (!strcmp(argv[a], "-s")) stats = 1;
    else break;
  }
  if (argc < 2 || a != argc - 2) {
    fprintf(stderr, "m4vpack - lossless compressor for MPEG-4 Part 2 video streams (XviD, DivX 4+)\n"
                    "Usage: m4vpack c|d [options] input output\n"
                    "  c   compress a raw .m4v stream\n"
                    "  d   decompress\n"
                    "  -v  (c) decompress the result again and compare it with the input\n"
                    "  -s  (c) bits spent per kind of syntax element\n"
                    "  -q  no statistics\n");
    return 1;
  }
  mode = argv[1][0];
  tables_init();
  crc_init();
  model_init();
  ctx_init(&ctx);
  if (!(in = fopen(argv[a], "rb"))) { fprintf(stderr, "can't open %s\n", argv[a]); return 2; }
  if (!(out = fopen(argv[a + 1], mode == 'c' ? "w+b" : "wb"))) {
    fprintf(stderr, "can't create %s\n", argv[a + 1]);
    return 3;
  }
  setvbuf(out, obuf_file, _IOFBF, sizeof(obuf_file));
  if (mode == 'c') {
    double out_bytes;
    r = compress(in, out, &ctx, &st, &co, stats);
    out_bytes = 20.0 + (double)co.bytes;
    if (fclose(out) != 0 && r >= 0) r = -1;
    if (r == -2) { fprintf(stderr, "internal error: a verified VOP couldn't be coded\n"); return 5; }
    if (r < 0) { fprintf(stderr, "%s error\n", ferror(in) ? "read" : "write"); return ferror(in) ? 2 : 3; }
    if (!quiet) {
      fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%); %u VOPs (I %u, P %u, B %u, S %u), %u packed", argv[a],
              st.in_bytes, out_bytes, st.in_bytes ? 100.0 * out_bytes / st.in_bytes : 0.0, st.vops, st.by_type[1],
              st.by_type[2], st.by_type[3], st.by_type[4], st.packed);
      if (st.vops > st.packed)
        fprintf(stderr, ", %u kept as bytes (%u unsupported, %u too long, %u unparsable, %u not reproduced)",
                st.vops - st.packed, st.raw_unsup, st.raw_long, st.raw_parse, st.raw_verify);
      fprintf(stderr, "\n");
      if (stats) {
        static const char* const grp_name[5] = { "headers, raw chunks", "macroblock types, cbp, dquant",
                                                 "motion vectors", "intra DC", "coefficients" };
        static const int grp_first[6] = { CAT_RAW, CAT_MB, CAT_MV, CAT_DC, CAT_SIG, NCAT };
        double o = 0, p = 0;
        int g;
        fprintf(stderr, "  %-33s %10s %10s\n", "bytes", "input", "packed");
        for (g = 0; g < 5; g++) {
          double og = 0, pg = 0;
          for (i = grp_first[g]; i < grp_first[g + 1]; i++) { og += co.orig[i]; pg += co.cost[i]; }
          fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", grp_name[g], og / 8, pg / 8, og ? 100 * pg / og : 0.0);
          if (grp_first[g + 1] - grp_first[g] > 1)
            for (i = grp_first[g]; i < grp_first[g + 1]; i++)
              fprintf(stderr, "    %-42s %10.0f\n", cat_name[i], co.cost[i] / 8);
          o += og;
          p += pg;
        }
        fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", "total", o / 8, p / 8, o ? 100 * p / o : 0.0);
      }
    }
    if (verify) {
      static Ctx ctx2;
      Sink s;
      FILE* t = fopen(argv[a + 1], "rb");
      if (!t) { fprintf(stderr, "can't open %s\n", argv[a + 1]); return 3; }
      rewind(in);
      memset(&s, 0, sizeof(s));
      s.f = in;
      s.compare = 1;
      model_init();
      ctx_init(&ctx2);
      r = decompress(t, &s, &ctx2, &co);
      fclose(t);
      if (r == 0 && !s.differs && fgetc(in) != EOF) { s.differs = 1; s.diff_at = s.pos; }
      if (r < 0 || s.differs) {
        fprintf(stderr, "verify: FAILED (%s at byte %.0f)\n", r < 0 ? "decoding error" : "difference", (double)s.diff_at);
        return 5;
      }
      if (!quiet) fprintf(stderr, "verify: OK\n");
    }
    fclose(in);
    return 0;
  }
  {
    Sink s;
    memset(&s, 0, sizeof(s));
    s.f = out;
    r = decompress(in, &s, &ctx, &co);
    fclose(in);
    if (fclose(out) != 0 && r == 0) r = -3;
    switch (r) {
    case 0: return 0;
    case -1: fprintf(stderr, "%s: not an m4vpack file\n", argv[a]); return 4;
    case -3: fprintf(stderr, "write error %s\n", argv[a + 1]); return 3;
    case -4: fprintf(stderr, "read error %s\n", argv[a]); return 2;
    default: fprintf(stderr, "%s: damaged data\n", argv[a]); return 4;
    }
  }
}
