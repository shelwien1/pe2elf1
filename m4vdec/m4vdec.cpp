// m4vdec - MPEG-4 Part 2 (DivX 4/5/6, XviD, ...) decoder: raw .m4v elementary stream -> per-frame PNG
// Usage: m4vdec [-yuv] [-n] input.m4v output_name_prefix
//   frames -> <prefix>NNNNN.png (RGB, display order)
//   -yuv   write <prefix> as one raw I420 file instead of PNGs (for bit-exact comparisons)
//   -n     don't repeat frames for not-coded VOPs (N-VOPs) in streams without B-frames
//
// The decoder core is a port of FFmpeg's MPEG-4 Part 2 decoder (libavcodec: mpeg4videodec.c,
// h263dec.c, h263.c, ituh263dec.c, mpeg4video.c, mpegvideo_motion.c, mpegvideo_dec.c,
// mpegvideo_unquantize.c, qpeldsp.c, hpeldsp.c, simple_idct, xvididct.c, mpeg4videodsp.c),
// so this file is licensed under the GNU Lesser General Public License 2.1 or later.
// Decoded YUV is bit-exact with FFmpeg's C code path: `ffmpeg -cpuflags 0 -threads 1`, plus
// -flags +bitexact for MPEG quantisation (FFmpeg's x86 SIMD XviD IDCT can differ by +-1).
// Supported: Simple + Advanced Simple profile, I/P/B/S(GMC) VOPs, qpel, 4MV, interlacing,
// H.263 and MPEG quantisation (custom matrices), resync markers, packed B-frames,
// DivX/XviD/lavc bug workarounds (XviD IDCT etc). Not supported: data partitioning,
// shapes, static sprites, scalability, studio profile, short video header (H.263).
//
// Build:
//   x86_64-w64-mingw32-g++ -O2 -s -static -o m4vdec.exe m4vdec.cpp -lz
//   g++ -O2 -o m4vdec m4vdec.cpp -lz -lpthread
// Plain C (compiles as C or C++): C runtime, zlib, Win32 threads or pthreads.

#ifdef _WIN32
#include <windows.h>
#else
#define _FILE_OFFSET_BITS 64
#if !defined(_GNU_SOURCE) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700  // fseeko/ftello/sysconf in strict C modes
#endif
#include <pthread.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

typedef unsigned char U8;
typedef signed char S8;
typedef unsigned short U16;
typedef short S16;
typedef unsigned int U32;
typedef int S32;
typedef unsigned long long U64;
typedef long long S64;

//--- helpers -----------------------------------------------------------------

static inline int clipi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline U8 clip8(int v) { return (U8)(v < 0 ? 0 : v > 255 ? 255 : v); }
static inline int mid_pred(int a, int b, int c) {
  if (a > b) { if (c > b) { if (c > a) b = a; else b = c; } }
  else { if (b > c) { if (c > a) b = c; else b = a; } }
  return b;
}
static inline int ilog2(U32 v) { int n = 0; while (v >>= 1) n++; return n; }
static inline int sign_extend(int val, int bits) { int sh = 32 - bits; return (int)((U32)val << sh) >> sh; }
#define ROUNDED_DIV(a, b) (((a) >= 0 ? (a) + ((b) >> 1) : (a) - ((b) >> 1)) / (b))
#define RSHIFT(a, b) ((a) > 0 ? ((a) + ((1 << (b)) >> 1)) >> (b) : ((a) + ((1 << (b)) >> 1) - 1) >> (b))

//--- bit reader (MSB first; buffers carry >= 8 bytes of zero padding) --------

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

//--- VLC ------------------------------------------------------------------------

typedef struct VLCE { S16 sym; S16 len; } VLCE;
typedef struct VLC { VLCE* tab; int bits; } VLC;

static void vlc_build(VLC* v, int bits, int n, const U32* codes, const U8* lens, const S16* syms) {
  int i;
  v->bits = bits;
  v->tab = (VLCE*)calloc((size_t)1 << bits, sizeof(VLCE));
  for (i = 0; i < n; i++) {
    U32 j, first, cnt;
    if (!lens[i]) continue;
    first = codes[i] << (bits - lens[i]);
    cnt = 1u << (bits - lens[i]);
    for (j = 0; j < cnt; j++) { v->tab[first + j].sym = syms ? syms[i] : (S16)i; v->tab[first + j].len = lens[i]; }
  }
}

static inline int vlc_get(GB* g, const VLC* v) {
  const VLCE* e = &v->tab[gb_show(g, v->bits)];
  if (!e->len) return -1;
  g->pos += e->len;
  return e->sym;
}

//--- tables (from FFmpeg: h263data.c, mpeg4data.h, mathtables.c, mpegvideodata.c) ---

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
static const S16 default_intra_matrix[64] = {
   8, 17, 18, 19, 21, 23, 25, 27, 17, 18, 19, 21, 23, 25, 27, 28,
  20, 21, 22, 23, 24, 26, 28, 30, 21, 22, 23, 24, 26, 28, 30, 32,
  22, 23, 24, 26, 28, 30, 32, 35, 23, 24, 26, 28, 30, 32, 35, 38,
  25, 26, 28, 30, 32, 35, 38, 41, 27, 28, 30, 32, 35, 38, 41, 45 };
static const S16 default_non_intra_matrix[64] = {
  16, 17, 18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 24,
  18, 19, 20, 21, 22, 23, 24, 25, 19, 20, 21, 22, 23, 24, 26, 27,
  20, 21, 22, 23, 25, 26, 27, 28, 21, 22, 23, 24, 26, 27, 28, 30,
  22, 23, 24, 26, 27, 28, 30, 31, 23, 24, 25, 27, 28, 30, 31, 33 };
static const U8 y_dc_scale_table[32] = {
  0, 8, 8, 8, 8, 10, 12, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 34, 36, 38, 40, 42, 44, 46 };
static const U8 c_dc_scale_table[32] = {
  0, 8, 8, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25 };
static const U8 dc_threshold[8] = { 99, 13, 15, 17, 19, 21, 23, 0 };
static const U8 zigzag_direct[64] = {
   0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };
static const U8 alternate_horizontal_scan[64] = {
   0,  1,  2,  3,  8,  9, 16, 17, 10, 11,  4,  5,  6,  7, 15, 14,
  13, 12, 19, 18, 24, 25, 32, 33, 26, 27, 20, 21, 22, 23, 28, 29,
  30, 31, 34, 35, 40, 41, 48, 49, 42, 43, 36, 37, 38, 39, 44, 45,
  46, 47, 50, 51, 56, 57, 58, 59, 52, 53, 54, 55, 60, 61, 62, 63 };
static const U8 alternate_vertical_scan[64] = {
   0,  8, 16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
  41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
  51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
  53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63 };
static const U8 h263_chroma_roundtab[16] = { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 };
static inline int round_chroma(int x) { return h263_chroma_roundtab[x & 0xf] + (x >> 3); }

typedef struct RL {
  int n, last;
  const U16 (*vlc)[2];
  const S8* run;
  const S8* level;
  U8 max_level[2][65], max_run[2][65];
  VLC v;
} RL;

static RL rl_intra = { 102, 67, intra_vlc, intra_run, intra_level };
static RL rl_inter = { 102, 58, inter_vlc, inter_run, inter_level };
static VLC vlc_intra_mcbpc, vlc_inter_mcbpc, vlc_cbpy, vlc_mv, vlc_dc_lum, vlc_dc_chrom, vlc_mb_type_b, vlc_sprite_traj;

static void rl_init(RL* rl) {
  int last, i;
  U32 codes[103]; U8 lens[103];
  memset(rl->max_level, 0, sizeof(rl->max_level));
  memset(rl->max_run, 0, sizeof(rl->max_run));
  for (last = 0; last < 2; last++) {
    int start = last ? rl->last : 0, end = last ? rl->n : rl->last;
    for (i = start; i < end; i++) {
      int run = rl->run[i], level = rl->level[i];
      if (level > rl->max_level[last][run]) rl->max_level[last][run] = (U8)level;
      if (run > rl->max_run[last][level]) rl->max_run[last][level] = (U8)run;
    }
  }
  for (i = 0; i <= rl->n; i++) { codes[i] = rl->vlc[i][0]; lens[i] = (U8)rl->vlc[i][1]; }
  vlc_build(&rl->v, 12, rl->n + 1, codes, lens, 0);
}

static void tables_init(void) {
  U32 c[33]; U8 l[33]; S16 s[33]; int i;
  static int done = 0;
  if (done) return;
  done = 1;
  for (i = 0; i < 9; i++) { c[i] = intra_mcbpc_code[i]; l[i] = intra_mcbpc_bits[i]; }
  vlc_build(&vlc_intra_mcbpc, 9, 9, c, l, 0);
  {
    U32 c2[28]; U8 l2[28];
    for (i = 0; i < 28; i++) { c2[i] = inter_mcbpc_code[i]; l2[i] = inter_mcbpc_bits[i]; }
    vlc_build(&vlc_inter_mcbpc, 13, 28, c2, l2, 0);
  }
  for (i = 0; i < 16; i++) { c[i] = cbpy_tab[i][0]; l[i] = cbpy_tab[i][1]; }
  vlc_build(&vlc_cbpy, 6, 16, c, l, 0);
  for (i = 0; i < 33; i++) { c[i] = mv_tab[i][0]; l[i] = mv_tab[i][1]; }
  vlc_build(&vlc_mv, 12, 33, c, l, 0);
  for (i = 0; i < 13; i++) { c[i] = DCtab_lum[i][0]; l[i] = DCtab_lum[i][1]; }
  vlc_build(&vlc_dc_lum, 11, 13, c, l, 0);
  for (i = 0; i < 13; i++) { c[i] = DCtab_chrom[i][0]; l[i] = DCtab_chrom[i][1]; }
  vlc_build(&vlc_dc_chrom, 12, 13, c, l, 0);
  for (i = 0; i < 4; i++) { c[i] = mb_type_b_tab[i][0]; l[i] = mb_type_b_tab[i][1]; s[i] = (S16)i; }
  vlc_build(&vlc_mb_type_b, 4, 4, c, l, s);
  {  // canonical codes from lengths, in listed order (as ff_vlc_init_from_lengths)
    U32 code = 0;
    for (i = 0; i < 15; i++) {
      l[i] = sprite_trajectory_lens[i];
      c[i] = code >> (32 - l[i]);
      code += 1u << (32 - l[i]);
    }
    vlc_build(&vlc_sprite_traj, 12, 15, c, l, 0);
  }
  rl_init(&rl_intra);
  rl_init(&rl_inter);
}

//--- decoder state -----------------------------------------------------------------

enum { PT_I = 1, PT_P = 2, PT_B = 3, PT_S = 4 };  // same values as AVPictureType (used in arithmetic)
enum { RECT_SHAPE = 0, BIN_SHAPE = 1, BIN_ONLY_SHAPE = 2, GRAY_SHAPE = 3 };
enum { STATIC_SPRITE = 1, GMC_SPRITE = 2 };
enum { MV_16X16 = 0, MV_8X8 = 1, MV_FIELD = 2 };
enum { OP_PUT = 0, OP_PUT_NORND = 1, OP_AVG = 2 };
#define MV_DIR_FORWARD  1
#define MV_DIR_BACKWARD 2
#define MV_DIRECT       4

#define MBT_INTRA      0x0001
#define MBT_16x16      0x0002
#define MBT_16x8       0x0004
#define MBT_8x8        0x0008
#define MBT_INTERLACED 0x0010
#define MBT_DIRECT2    0x0020
#define MBT_ACPRED     0x0040
#define MBT_GMC        0x0080
#define MBT_SKIP       0x0100
#define MBT_FWD        0x0200
#define MBT_BWD        0x0400
#define MBT_BIDIR      (MBT_FWD | MBT_BWD)

#define BUG_AUTODETECT       1
#define BUG_XVID_ILACE       4
#define BUG_AMV              32
#define BUG_QPEL_CHROMA      64
#define BUG_STD_QPEL         128
#define BUG_QPEL_CHROMA2     256
#define BUG_DIRECT_BLOCKSIZE 512
#define BUG_EDGE             1024
#define BUG_HPEL_CHROMA      2048
#define BUG_DC_CLIP          4096
#define BUG_IEDGE            32768

#define FRAME_SKIPPED 100
#define SLICE_OK  0
#define SLICE_END (-2)

typedef struct Pic {
  U8 *y, *u, *v;           // planes, MB-aligned (ly = mb_w*16, lc = mb_w*8)
  U8* mbskip;              // [mb_stride*mb_h + 2]
  U16* mbtype;             // [mb_stride*mb_h]
  S16 (*mvbase)[2];        // motion vectors on the 8x8 grid (b8_stride) incl. 4 entries pre-padding
  S16 (*mv)[2];
  U8* ref_index;           // [4*mb_stride*mb_h] field_select of field MBs
  S64 time;
  int type;
} Pic;

typedef struct Dec {
  GB gb;
  // VOL / stream
  int width, height, mb_w, mb_h, mb_num, mb_stride, b8_stride, ly, lc;
  int vo_type, vol_control, low_delay, shape, time_res, time_inc_bits, fixed_vop_inc;
  int progressive, sprite_usage, warp_points, warp_acc, brightness_change;
  int quant_precision, mpeg_quant, qpel, resync_marker, data_partitioning, rvlc, new_pred, scalability, enhancement_type;
  int cplx_i, cplx_p, cplx_b;
  U16 intra_matrix[64], inter_matrix[64];  // natural order
  int have_vol, picture_number;
  int color_matrix, full_range;            // from the visual object header
  // encoder identification / workarounds
  int divx_version, divx_build, xvid_build, lavc_build, divx_packed;
  int bugs, idct_xvid, h_edge, v_edge;
  // VOP
  int pict_type, no_rounding, intra_dc_threshold, top_field_first, alternate_scan;
  int qscale, chroma_qscale, y_dc_scale, c_dc_scale, f_code, b_code, mcsel;
  int time_base, last_time_base;
  S64 time, last_non_b_time;
  U16 pp_time, pb_time, pp_field_time, pb_field_time;
  int t_frame;
  S16 direct_scale_mv[2][64];
  int sprite_offset[2][2], sprite_delta[2][2], sprite_shift[2], real_warp_points;
  const U8 *intra_scan, *intra_h_scan, *intra_v_scan;
  // macroblock
  int mb_x, mb_y, resync_mb_x, resync_mb_y, first_slice_line;
  int mb_intra, ac_pred, interlaced_dct, mv_dir, mv_type, mb_skipped;
  int mv[2][4][2], field_select[2][2], last_mv[2][2][2];
  int block_index[6], block_wrap[6];
  S16 block[6][64];
  int last_index[6];
  // prediction state (same layout as FFmpeg's MpegEncContext)
  S16 *dc_val_base, *dc_val;
  S16 (*ac_val_base)[16], (*ac_val)[16];
  U8* mbintra_table;
  S8* qscale_table;
  S16 (*p_field_mv)[2][2];   // [mb_xy][field][xy], forward field MVs of the last P-VOP
  U8* mb_done;
  // pictures
  Pic pics[4];
  Pic *cur, *last, *next;
  int alloc_w, alloc_h;
  int errors;
  const char* unsupported;  // set when the stream uses a feature this decoder lacks
} Dec;

static void set_qscale(Dec* d, int q) {
  q = clipi(q, 1, 31);
  d->qscale = q;
  d->chroma_qscale = q;
  d->y_dc_scale = y_dc_scale_table[q];
  d->c_dc_scale = c_dc_scale_table[q];
}

static void pic_free(Pic* p) {
  free(p->y); free(p->mbskip); free(p->mbtype); free(p->mvbase); free(p->ref_index);
  memset(p, 0, sizeof(*p));
}

static void dec_free_buffers(Dec* d) {
  int i;
  for (i = 0; i < 4; i++) pic_free(&d->pics[i]);
  free(d->dc_val_base); free(d->ac_val_base); free(d->mbintra_table); free(d->qscale_table);
  free(d->p_field_mv); free(d->mb_done);
  d->dc_val_base = d->dc_val = 0; d->ac_val_base = d->ac_val = 0;
  d->mbintra_table = 0; d->qscale_table = 0; d->p_field_mv = 0; d->mb_done = 0;
  d->cur = d->last = d->next = 0;
  d->alloc_w = d->alloc_h = 0;
}

static int dec_alloc(Dec* d) {
  int i, y_size, c_size, yc_size, mb_array, b8_array;
  if (d->alloc_w == d->width && d->alloc_h == d->height) return 0;
  dec_free_buffers(d);
  d->mb_w = (d->width + 15) >> 4;
  d->mb_h = (d->height + 15) >> 4;
  d->mb_num = d->mb_w * d->mb_h;
  d->mb_stride = d->mb_w + 1;
  d->b8_stride = d->mb_w * 2 + 1;
  d->ly = d->mb_w * 16;
  d->lc = d->mb_w * 8;
  y_size = d->b8_stride * (2 * d->mb_h + 1);
  c_size = d->mb_stride * (d->mb_h + 1);
  yc_size = y_size + 2 * c_size;
  mb_array = d->mb_stride * d->mb_h;
  b8_array = d->b8_stride * d->mb_h * 2;
  d->dc_val_base = (S16*)malloc(sizeof(S16) * yc_size);
  d->ac_val_base = (S16(*)[16])calloc(yc_size, sizeof(*d->ac_val_base));
  d->mbintra_table = (U8*)calloc(mb_array + 2, 1);
  d->qscale_table = (S8*)calloc(mb_array + 2, 1);
  d->p_field_mv = (S16(*)[2][2])calloc(mb_array + 2, sizeof(*d->p_field_mv));
  d->mb_done = (U8*)calloc(mb_array + 2, 1);
  if (!d->dc_val_base || !d->ac_val_base || !d->mbintra_table || !d->qscale_table || !d->p_field_mv || !d->mb_done)
    return -1;
  for (i = 0; i < yc_size; i++) d->dc_val_base[i] = 1024;
  d->dc_val = d->dc_val_base + d->b8_stride + 1;
  d->ac_val = d->ac_val_base + d->b8_stride + 1;
  for (i = 0; i < 4; i++) {
    Pic* p = &d->pics[i];
    size_t ysz = (size_t)d->ly * d->mb_h * 16, csz = (size_t)d->lc * d->mb_h * 8;
    p->y = (U8*)malloc(ysz + 2 * csz);
    p->mbskip = (U8*)calloc(mb_array + 2, 1);
    p->mbtype = (U16*)calloc(mb_array + 2, sizeof(U16));
    p->mvbase = (S16(*)[2])calloc(b8_array + 4, sizeof(*p->mvbase));
    p->ref_index = (U8*)calloc(4 * (mb_array + 2), 1);
    if (!p->y || !p->mbskip || !p->mbtype || !p->mvbase || !p->ref_index) return -1;
    p->u = p->y + ysz;
    p->v = p->u + csz;
    p->mv = p->mvbase + 4;
  }
  d->alloc_w = d->width;
  d->alloc_h = d->height;
  return 0;
}

static void set_block_index(Dec* d) {
  int bs = d->b8_stride, ms = d->mb_stride;
  d->block_index[0] = bs * (d->mb_y * 2) + d->mb_x * 2;
  d->block_index[1] = d->block_index[0] + 1;
  d->block_index[2] = bs * (d->mb_y * 2 + 1) + d->mb_x * 2;
  d->block_index[3] = d->block_index[2] + 1;
  d->block_index[4] = ms * (d->mb_y + 1) + bs * d->mb_h * 2 + d->mb_x;
  d->block_index[5] = ms * (d->mb_y + d->mb_h + 2) + bs * d->mb_h * 2 + d->mb_x;
}

//--- headers ---------------------------------------------------------------------

static int decode_vol_header(Dec* d) {
  GB* gb = &d->gb;
  int width, height, vo_ver_id;
  gb_skip(gb, 1);                       // random access
  d->vo_type = (int)gb_get(gb, 8);
  if (d->vo_type == 14 || d->vo_type == 15) {
    d->unsupported = "MPEG-4 studio profile";
    return -1;
  }
  if (gb_get1(gb)) {                    // is_ol_id
    vo_ver_id = (int)gb_get(gb, 4);
    gb_skip(gb, 3);
  } else
    vo_ver_id = 1;
  if (gb_get(gb, 4) == 15) gb_skip(gb, 16);   // aspect_ratio_info, extended PAR
  if ((d->vol_control = gb_get1(gb))) {
    gb_skip(gb, 2);                     // chroma_format
    d->low_delay = gb_get1(gb);
    if (gb_get1(gb)) {                  // vbv parameters
      gb_skip(gb, 15 + 1 + 15 + 1 + 15 + 1 + 3 + 11 + 1 + 15 + 1);
    }
  } else if (d->picture_number == 0) {
    d->low_delay = (d->vo_type == 1 || d->vo_type == 17);  // SIMPLE / ADV_SIMPLE
  }
  d->shape = (int)gb_get(gb, 2);
  if (d->shape != RECT_SHAPE) { d->unsupported = "non-rectangular VOL shapes"; return -1; }
  if (d->shape == GRAY_SHAPE && vo_ver_id != 1) gb_skip(gb, 4);
  gb_skip(gb, 1);
  d->time_res = (int)gb_get(gb, 16);
  if (!d->time_res) { fprintf(stderr, "framerate==0\n"); return -1; }
  d->time_inc_bits = ilog2(d->time_res - 1) + 1;
  if (d->time_inc_bits < 1) d->time_inc_bits = 1;
  gb_skip(gb, 1);
  d->fixed_vop_inc = gb_get1(gb) ? (int)gb_get(gb, d->time_inc_bits) : 1;
  d->t_frame = 0;
  if (d->shape != BIN_ONLY_SHAPE) {
    if (d->shape == RECT_SHAPE) {
      gb_skip(gb, 1);
      width = (int)gb_get(gb, 13);
      gb_skip(gb, 1);
      height = (int)gb_get(gb, 13);
      gb_skip(gb, 1);
      if (width && height) { d->width = width; d->height = height; }
    }
    d->progressive = gb_get1(gb) ^ 1;
    d->interlaced_dct = 0;
    gb_skip(gb, 1);                     // obmc disable
    d->sprite_usage = vo_ver_id == 1 ? gb_get1(gb) : (int)gb_get(gb, 2);
    if (d->sprite_usage == STATIC_SPRITE) fprintf(stderr, "static sprites not supported\n");
    if (d->sprite_usage == STATIC_SPRITE || d->sprite_usage == GMC_SPRITE) {
      if (d->sprite_usage == STATIC_SPRITE) gb_skip(gb, 4 * 14);
      d->warp_points = (int)gb_get(gb, 6);
      if (d->warp_points > 3) { d->warp_points = 0; return -1; }
      d->warp_acc = (int)gb_get(gb, 2);
      d->brightness_change = gb_get1(gb);
      if (d->sprite_usage == STATIC_SPRITE) gb_skip(gb, 1);
    }
    if (gb_get1(gb)) {                  // not_8_bit
      d->quant_precision = (int)gb_get(gb, 4);
      if (gb_get(gb, 4) != 8) fprintf(stderr, "N-bit not supported\n");
      if (d->quant_precision < 3 || d->quant_precision > 9) d->quant_precision = 5;
    } else
      d->quant_precision = 5;
    if ((d->mpeg_quant = gb_get1(gb))) {
      int i, v, last;
      for (i = 0; i < 64; i++) { d->intra_matrix[i] = default_intra_matrix[i]; d->inter_matrix[i] = default_non_intra_matrix[i]; }
      if (gb_get1(gb)) {
        last = 0;
        for (i = 0; i < 64; i++) {
          if (gb_left(gb) < 8) return -1;
          v = (int)gb_get(gb, 8);
          if (!v) break;
          last = v;
          d->intra_matrix[zigzag_direct[i]] = (U16)last;
        }
        for (; i < 64; i++) d->intra_matrix[zigzag_direct[i]] = (U16)last;
      }
      if (gb_get1(gb)) {
        last = 0;
        for (i = 0; i < 64; i++) {
          if (gb_left(gb) < 8) return -1;
          v = (int)gb_get(gb, 8);
          if (!v) break;
          last = v;
          d->inter_matrix[zigzag_direct[i]] = (U16)v;
        }
        for (; i < 64; i++) d->inter_matrix[zigzag_direct[i]] = (U16)last;
      }
    }
    d->qpel = vo_ver_id != 1 ? gb_get1(gb) : 0;
    if (gb_left(gb) < 4) return -1;
    d->cplx_i = d->cplx_p = d->cplx_b = 0;
    if (!gb_get1(gb)) {                 // complexity estimation
      int pos = gb->pos, method = (int)gb_get(gb, 2);
      if (method < 2) {
        if (!gb_get1(gb)) { int k; for (k = 0; k < 6; k++) d->cplx_i += 8 * gb_get1(gb); }
        if (!gb_get1(gb)) {
          d->cplx_i += 8 * gb_get1(gb); d->cplx_p += 8 * gb_get1(gb);
          d->cplx_p += 8 * gb_get1(gb); d->cplx_i += 8 * gb_get1(gb);
        }
        if (!gb_get1(gb)) { gb->pos = pos; d->cplx_i = d->cplx_p = d->cplx_b = 0; goto no_cplx; }
        if (!gb_get1(gb)) {
          d->cplx_i += 8 * gb_get1(gb); d->cplx_i += 8 * gb_get1(gb);
          d->cplx_i += 8 * gb_get1(gb); d->cplx_i += 4 * gb_get1(gb);
        }
        if (!gb_get1(gb)) {
          d->cplx_p += 8 * gb_get1(gb); d->cplx_p += 8 * gb_get1(gb); d->cplx_b += 8 * gb_get1(gb);
          d->cplx_p += 8 * gb_get1(gb); d->cplx_p += 8 * gb_get1(gb); d->cplx_p += 8 * gb_get1(gb);
        }
        if (!gb_get1(gb)) { gb->pos = pos; d->cplx_i = d->cplx_p = d->cplx_b = 0; goto no_cplx; }
        if (method == 1) { d->cplx_i += 8 * gb_get1(gb); d->cplx_p += 8 * gb_get1(gb); }
      }
    }
no_cplx:
    d->resync_marker = !gb_get1(gb);
    d->data_partitioning = gb_get1(gb);
    if (d->data_partitioning) d->rvlc = gb_get1(gb);
    if (vo_ver_id != 1) {
      d->new_pred = gb_get1(gb);
      if (d->new_pred) { fprintf(stderr, "new pred not supported\n"); gb_skip(gb, 3); }
      if (gb_get1(gb)) fprintf(stderr, "reduced resolution VOP not supported\n");
    } else
      d->new_pred = 0;
    d->scalability = gb_get1(gb);
    if (d->scalability) {
      GB bak = *gb;
      int hn, hm, vn, vm;
      gb_skip(gb, 1 + 4 + 1);
      hn = (int)gb_get(gb, 5); hm = (int)gb_get(gb, 5);
      vn = (int)gb_get(gb, 5); vm = (int)gb_get(gb, 5);
      d->enhancement_type = gb_get1(gb);
      if (!hn || !hm || !vn || !vm) { d->scalability = 0; *gb = bak; }
      else fprintf(stderr, "scalability not supported\n");
    }
  }
  d->have_vol = 1;
  return 0;
}

static void decode_user_data(Dec* d) {
  GB* gb = &d->gb;
  char buf[256], last;
  int i, e, ver = 0, build = 0, ver2 = 0, ver3 = 0;
  for (i = 0; i < 255 && gb->pos < gb->size; i++) {
    if (gb_show(gb, 23) == 0) break;
    buf[i] = (char)gb_get(gb, 8);
  }
  buf[i] = 0;
  e = sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
  if (e < 2) e = sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
  if (e >= 2) {
    d->divx_version = ver;
    d->divx_build = build;
    d->divx_packed = e == 3 && last == 'p';
  }
  e = sscanf(buf, "FFmpe%*[^b]b%d", &build) + 3;
  if (e != 4) e = sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &ver2, &ver3, &build);
  if (e != 4) {
    e = sscanf(buf, "Lavc%d.%d.%d", &ver, &ver2, &ver3) + 1;
    if (e > 1) build = ((ver & 0xFF) << 16) + ((ver2 & 0xFF) << 8) + (ver3 & 0xFF);
  }
  if (e != 4) { if (strcmp(buf, "ffmpeg") == 0) d->lavc_build = 4600; }
  if (e == 4) d->lavc_build = build;
  e = sscanf(buf, "XviD%d", &build);
  if (e == 1) d->xvid_build = build;
}

static void decode_visual_object(Dec* d) {
  GB* gb = &d->gb;
  int type;
  if (gb_get1(gb)) gb_skip(gb, 4 + 3);
  type = (int)gb_get(gb, 4);
  if ((type == 1 || type == 2) && gb_get1(gb)) {   // video / still texture, video_signal_type
    gb_skip(gb, 3);
    d->full_range = gb_get1(gb);
    if (gb_get1(gb)) {
      gb_skip(gb, 16);                               // colour primaries, transfer characteristics
      d->color_matrix = (int)gb_get(gb, 8);
    }
  }
}

static int decode_sprite_trajectory(Dec* d) {
  GB* gb = &d->gb;
  int a = 2 << d->warp_acc, rho = 3 - d->warp_acc, r = 16 / a;
  int alpha = 1, beta = 0, w = d->width, h = d->height, min_ab, i, w2, h2, w3, h3;
  int sprite_ref[4][2], virtual_ref[2][2];
  S64 sprite_offset[2][2], sprite_delta[2][2];
  const int vop_ref[4][2] = { { 0, 0 }, { d->width, 0 }, { 0, d->height }, { d->width, d->height } };
  int dd[4][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
  const int divx413 = d->divx_version == 500 && d->divx_build == 413;
  if (w <= 0 || h <= 0) return -1;
  for (i = 0; i < d->warp_points; i++) {
    int length, x = 0, y = 0;
    length = vlc_get(gb, &vlc_sprite_traj);
    if (length > 0) x = gb_xbits(gb, length);
    if (!divx413) gb_skip(gb, 1);       // marker
    length = vlc_get(gb, &vlc_sprite_traj);
    if (length > 0) y = gb_xbits(gb, length);
    gb_skip(gb, 1);                     // marker
    dd[i][0] = x;
    dd[i][1] = y;
  }
  while ((1 << alpha) < w) alpha++;
  while ((1 << beta) < h) beta++;
  w2 = 1 << alpha;
  h2 = 1 << beta;
  if (divx413) {
    sprite_ref[0][0] = a * vop_ref[0][0] + dd[0][0];
    sprite_ref[0][1] = a * vop_ref[0][1] + dd[0][1];
    sprite_ref[1][0] = a * vop_ref[1][0] + dd[0][0] + dd[1][0];
    sprite_ref[1][1] = a * vop_ref[1][1] + dd[0][1] + dd[1][1];
    sprite_ref[2][0] = a * vop_ref[2][0] + dd[0][0] + dd[2][0];
    sprite_ref[2][1] = a * vop_ref[2][1] + dd[0][1] + dd[2][1];
  } else {
    sprite_ref[0][0] = (a >> 1) * (2 * vop_ref[0][0] + dd[0][0]);
    sprite_ref[0][1] = (a >> 1) * (2 * vop_ref[0][1] + dd[0][1]);
    sprite_ref[1][0] = (a >> 1) * (2 * vop_ref[1][0] + dd[0][0] + dd[1][0]);
    sprite_ref[1][1] = (a >> 1) * (2 * vop_ref[1][1] + dd[0][1] + dd[1][1]);
    sprite_ref[2][0] = (a >> 1) * (2 * vop_ref[2][0] + dd[0][0] + dd[2][0]);
    sprite_ref[2][1] = (a >> 1) * (2 * vop_ref[2][1] + dd[0][1] + dd[2][1]);
  }
  virtual_ref[0][0] = 16 * (vop_ref[0][0] + w2) +
      (int)ROUNDED_DIV(((w - w2) * (r * sprite_ref[0][0] - 16LL * vop_ref[0][0]) +
                        w2 * (r * sprite_ref[1][0] - 16LL * vop_ref[1][0])), w);
  virtual_ref[0][1] = 16 * vop_ref[0][1] +
      (int)ROUNDED_DIV(((w - w2) * (r * sprite_ref[0][1] - 16LL * vop_ref[0][1]) +
                        w2 * (r * sprite_ref[1][1] - 16LL * vop_ref[1][1])), w);
  virtual_ref[1][0] = 16 * vop_ref[0][0] +
      (int)ROUNDED_DIV(((h - h2) * (r * sprite_ref[0][0] - 16LL * vop_ref[0][0]) +
                        h2 * (r * sprite_ref[2][0] - 16LL * vop_ref[2][0])), h);
  virtual_ref[1][1] = 16 * (vop_ref[0][1] + h2) +
      (int)ROUNDED_DIV(((h - h2) * (r * sprite_ref[0][1] - 16LL * vop_ref[0][1]) +
                        h2 * (r * sprite_ref[2][1] - 16LL * vop_ref[2][1])), h);
  switch (d->warp_points) {
  case 0:
    sprite_offset[0][0] = sprite_offset[0][1] = sprite_offset[1][0] = sprite_offset[1][1] = 0;
    sprite_delta[0][0] = a; sprite_delta[0][1] = sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    break;
  case 1:
    sprite_offset[0][0] = sprite_ref[0][0] - a * vop_ref[0][0];
    sprite_offset[0][1] = sprite_ref[0][1] - a * vop_ref[0][1];
    sprite_offset[1][0] = ((sprite_ref[0][0] >> 1) | (sprite_ref[0][0] & 1)) - a * (vop_ref[0][0] / 2);
    sprite_offset[1][1] = ((sprite_ref[0][1] >> 1) | (sprite_ref[0][1] & 1)) - a * (vop_ref[0][1] / 2);
    sprite_delta[0][0] = a; sprite_delta[0][1] = sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    break;
  case 2:
    sprite_offset[0][0] = ((S64)sprite_ref[0][0] * (1 << (alpha + rho))) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-vop_ref[0][0]) +
                          ((S64)r * sprite_ref[0][1] - virtual_ref[0][1]) * ((S64)-vop_ref[0][1]) +
                          (1 << (alpha + rho - 1));
    sprite_offset[0][1] = ((S64)sprite_ref[0][1] * (1 << (alpha + rho))) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * ((S64)-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-vop_ref[0][1]) +
                          (1 << (alpha + rho - 1));
    sprite_offset[1][0] = (((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-2 * vop_ref[0][0] + 1) +
                           ((S64)r * sprite_ref[0][1] - virtual_ref[0][1]) * ((S64)-2 * vop_ref[0][1] + 1) +
                           2 * w2 * r * (S64)sprite_ref[0][0] - 16 * w2 + (1 << (alpha + rho + 1)));
    sprite_offset[1][1] = (((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * ((S64)-2 * vop_ref[0][0] + 1) +
                           ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * ((S64)-2 * vop_ref[0][1] + 1) +
                           2 * w2 * r * (S64)sprite_ref[0][1] - 16 * w2 + (1 << (alpha + rho + 1)));
    sprite_delta[0][0] = (-r * sprite_ref[0][0] + virtual_ref[0][0]);
    sprite_delta[0][1] = (+r * sprite_ref[0][1] - virtual_ref[0][1]);
    sprite_delta[1][0] = (-r * sprite_ref[0][1] + virtual_ref[0][1]);
    sprite_delta[1][1] = (-r * sprite_ref[0][0] + virtual_ref[0][0]);
    d->sprite_shift[0] = alpha + rho;
    d->sprite_shift[1] = alpha + rho + 2;
    break;
  case 3:
    min_ab = alpha < beta ? alpha : beta;
    w3 = w2 >> min_ab;
    h3 = h2 >> min_ab;
    sprite_offset[0][0] = ((S64)sprite_ref[0][0] * (1 << (alpha + beta + rho - min_ab))) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * h3 * (-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[1][0]) * w3 * (-vop_ref[0][1]) +
                          ((S64)1 << (alpha + beta + rho - min_ab - 1));
    sprite_offset[0][1] = ((S64)sprite_ref[0][1] * (1 << (alpha + beta + rho - min_ab))) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * h3 * (-vop_ref[0][0]) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[1][1]) * w3 * (-vop_ref[0][1]) +
                          ((S64)1 << (alpha + beta + rho - min_ab - 1));
    sprite_offset[1][0] = ((S64)-r * sprite_ref[0][0] + virtual_ref[0][0]) * h3 * (-2 * vop_ref[0][0] + 1) +
                          ((S64)-r * sprite_ref[0][0] + virtual_ref[1][0]) * w3 * (-2 * vop_ref[0][1] + 1) +
                          (S64)2 * w2 * h3 * r * sprite_ref[0][0] - 16 * w2 * h3 +
                          ((S64)1 << (alpha + beta + rho - min_ab + 1));
    sprite_offset[1][1] = ((S64)-r * sprite_ref[0][1] + virtual_ref[0][1]) * h3 * (-2 * vop_ref[0][0] + 1) +
                          ((S64)-r * sprite_ref[0][1] + virtual_ref[1][1]) * w3 * (-2 * vop_ref[0][1] + 1) +
                          (S64)2 * w2 * h3 * r * sprite_ref[0][1] - 16 * w2 * h3 +
                          ((S64)1 << (alpha + beta + rho - min_ab + 1));
    sprite_delta[0][0] = (-r * (S64)sprite_ref[0][0] + virtual_ref[0][0]) * h3;
    sprite_delta[0][1] = (-r * (S64)sprite_ref[0][0] + virtual_ref[1][0]) * w3;
    sprite_delta[1][0] = (-r * (S64)sprite_ref[0][1] + virtual_ref[0][1]) * h3;
    sprite_delta[1][1] = (-r * (S64)sprite_ref[0][1] + virtual_ref[1][1]) * w3;
    d->sprite_shift[0] = alpha + beta + rho - min_ab;
    d->sprite_shift[1] = alpha + beta + rho - min_ab + 2;
    break;
  default:
    return -1;
  }
  if (sprite_delta[0][0] == (S64)a << d->sprite_shift[0] && sprite_delta[0][1] == 0 &&
      sprite_delta[1][0] == 0 && sprite_delta[1][1] == (S64)a << d->sprite_shift[0]) {
    sprite_offset[0][0] >>= d->sprite_shift[0];
    sprite_offset[0][1] >>= d->sprite_shift[0];
    sprite_offset[1][0] >>= d->sprite_shift[1];
    sprite_offset[1][1] >>= d->sprite_shift[1];
    sprite_delta[0][0] = a; sprite_delta[0][1] = 0; sprite_delta[1][0] = 0; sprite_delta[1][1] = a;
    d->sprite_shift[0] = d->sprite_shift[1] = 0;
    d->real_warp_points = 1;
  } else {
    int shift_y = 16 - d->sprite_shift[0], shift_c = 16 - d->sprite_shift[1];
    const S64 IMAX = 0x7FFFFFFF;
    for (i = 0; i < 2; i++) {
      if (shift_c < 0 || shift_y < 0 ||
          llabs(sprite_offset[0][i]) >= IMAX >> shift_y || llabs(sprite_offset[1][i]) >= IMAX >> shift_c ||
          llabs(sprite_delta[0][i]) >= IMAX >> shift_y || llabs(sprite_delta[1][i]) >= IMAX >> shift_y)
        goto overflow;
    }
    for (i = 0; i < 2; i++) {
      sprite_offset[0][i] *= (S64)1 << shift_y;
      sprite_offset[1][i] *= (S64)1 << shift_c;
      sprite_delta[0][i] *= (S64)1 << shift_y;
      sprite_delta[1][i] *= (S64)1 << shift_y;
      d->sprite_shift[i] = 16;
    }
    for (i = 0; i < 2; i++) {
      S64 sd0 = sprite_delta[i][0] - a * (1LL << 16), sd1 = sprite_delta[i][1] - a * (1LL << 16);
      if (llabs(sprite_offset[0][i] + sprite_delta[i][0] * (w + 16LL)) >= IMAX ||
          llabs(sprite_offset[0][i] + sprite_delta[i][1] * (h + 16LL)) >= IMAX ||
          llabs(sprite_offset[0][i] + sprite_delta[i][0] * (w + 16LL) + sprite_delta[i][1] * (h + 16LL)) >= IMAX ||
          llabs(sprite_delta[i][0] * (w + 16LL)) >= IMAX || llabs(sprite_delta[i][1] * (h + 16LL)) >= IMAX ||
          llabs(sd0) >= IMAX || llabs(sd1) >= IMAX ||
          llabs(sprite_offset[0][i] + sd0 * (w + 16LL)) >= IMAX ||
          llabs(sprite_offset[0][i] + sd1 * (h + 16LL)) >= IMAX ||
          llabs(sprite_offset[0][i] + sd0 * (w + 16LL) + sd1 * (h + 16LL)) >= IMAX)
        goto overflow;
    }
    d->real_warp_points = d->warp_points;
  }
  for (i = 0; i < 4; i++) {
    d->sprite_offset[i & 1][i >> 1] = (int)sprite_offset[i & 1][i >> 1];
    d->sprite_delta[i & 1][i >> 1] = (int)sprite_delta[i & 1][i >> 1];
  }
  return 0;
overflow:
  memset(d->sprite_offset, 0, sizeof(d->sprite_offset));
  memset(d->sprite_delta, 0, sizeof(d->sprite_delta));
  return -1;
}

static int video_packet_prefix_length(int pict_type, int f_code, int b_code) {
  switch (pict_type) {
  case PT_I: return 16;
  case PT_P: case PT_S: return f_code + 15;
  case PT_B: { int m = f_code > b_code ? f_code : b_code; return (m > 2 ? m : 2) + 15; }
  }
  return -1;
}

static void init_direct_mv(Dec* d) {
  int i;
  for (i = 0; i < 64; i++) {
    d->direct_scale_mv[0][i] = (S16)((i - 32) * d->pb_time / d->pp_time);
    d->direct_scale_mv[1][i] = (S16)((i - 32) * (d->pb_time - d->pp_time) / d->pp_time);
  }
}

static int decode_vop_header(Dec* d) {
  GB* gb = &d->gb;
  int time_incr, time_increment;
  d->mcsel = 0;
  d->pict_type = (int)gb_get(gb, 2) + PT_I;
  if (d->pict_type == PT_B && d->low_delay && d->vol_control == 0) d->low_delay = 0;
  if (d->data_partitioning && d->pict_type != PT_B) {
    d->unsupported = "data partitioning";
    return -1;
  }
  time_incr = 0;
  while (gb_get1(gb) != 0) {
    time_incr++;
    if (gb_left(gb) <= 0) return -1;
  }
  gb_skip(gb, 1);                       // marker
  if (d->time_inc_bits == 0 || !(gb_show(gb, d->time_inc_bits + 1) & 1)) {
    for (d->time_inc_bits = 1; d->time_inc_bits < 16; d->time_inc_bits++) {
      if (d->pict_type == PT_P || (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE)) {
        if ((gb_show(gb, d->time_inc_bits + 6) & 0x37) == 0x30) break;
      } else if ((gb_show(gb, d->time_inc_bits + 5) & 0x1F) == 0x18)
        break;
    }
  }
  time_increment = (int)gb_get(gb, d->time_inc_bits);
  if (d->pict_type != PT_B) {
    d->last_time_base = d->time_base;
    d->time_base += time_incr;
    d->time = d->time_base * (S64)d->time_res + time_increment;
    d->pp_time = (U16)(d->time - d->last_non_b_time);
    d->last_non_b_time = d->time;
  } else {
    d->time = (d->last_time_base + time_incr) * (S64)d->time_res + time_increment;
    d->pb_time = (U16)(d->pp_time - (d->last_non_b_time - d->time));
    if (d->pp_time <= d->pb_time || d->pp_time <= d->pp_time - d->pb_time || d->pp_time <= 0)
      return FRAME_SKIPPED;             // messed up order
    init_direct_mv(d);
    if (d->t_frame == 0) d->t_frame = d->pb_time;
    if (d->t_frame == 0) d->t_frame = 1;
    d->pp_field_time = (U16)((ROUNDED_DIV(d->last_non_b_time, d->t_frame) -
                              ROUNDED_DIV(d->last_non_b_time - d->pp_time, d->t_frame)) * 2);
    d->pb_field_time = (U16)((ROUNDED_DIV(d->time, d->t_frame) -
                              ROUNDED_DIV(d->last_non_b_time - d->pp_time, d->t_frame)) * 2);
    if (d->pp_field_time <= d->pb_field_time || d->pb_field_time <= 1) {
      d->pb_field_time = 2;
      d->pp_field_time = 4;
      if (!d->progressive) return FRAME_SKIPPED;
    }
  }
  gb_skip(gb, 1);                       // marker
  if (gb_get1(gb) != 1) return FRAME_SKIPPED;   // vop not coded
  if (d->new_pred) {
    int len = d->time_inc_bits + 3 < 15 ? d->time_inc_bits + 3 : 15;
    gb_skip(gb, len);
    if (gb_get1(gb)) gb_skip(gb, len);
    gb_skip(gb, 1);
  }
  if (d->shape != BIN_ONLY_SHAPE &&
      (d->pict_type == PT_P || (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE)))
    d->no_rounding = gb_get1(gb);
  else
    d->no_rounding = 0;
  if (d->shape != RECT_SHAPE) {
    if (d->sprite_usage != 1 || d->pict_type != PT_I) gb_skip(gb, 13 + 1 + 13 + 1 + 13 + 1 + 13);
    gb_skip(gb, 1);
    if (gb_get1(gb)) gb_skip(gb, 8);
  }
  if (d->shape != BIN_ONLY_SHAPE) {
    gb_skip(gb, d->cplx_i);
    if (d->pict_type != PT_I) gb_skip(gb, d->cplx_p);
    if (d->pict_type == PT_B) gb_skip(gb, d->cplx_b);
    if (gb_left(gb) < 3) return -1;
    d->intra_dc_threshold = dc_threshold[gb_get(gb, 3)];
    if (!d->progressive) {
      d->top_field_first = gb_get1(gb);
      d->alternate_scan = gb_get1(gb);
    } else
      d->alternate_scan = 0;
  }
  if (d->alternate_scan) {
    d->intra_scan = alternate_vertical_scan;
    d->intra_h_scan = alternate_vertical_scan;
  } else {
    d->intra_scan = zigzag_direct;
    d->intra_h_scan = alternate_horizontal_scan;
  }
  d->intra_v_scan = alternate_vertical_scan;
  if (d->pict_type == PT_S) {
    if (d->sprite_usage == STATIC_SPRITE || d->sprite_usage == GMC_SPRITE) {
      if (decode_sprite_trajectory(d) < 0) return -1;
      if (d->brightness_change) fprintf(stderr, "sprite_brightness_change not supported\n");
    } else {
      memset(d->sprite_offset, 0, sizeof(d->sprite_offset));
      memset(d->sprite_delta, 0, sizeof(d->sprite_delta));
    }
  }
  d->f_code = 1;
  d->b_code = 1;
  if (d->shape != BIN_ONLY_SHAPE) {
    d->qscale = d->chroma_qscale = (int)gb_get(gb, d->quant_precision);
    if (d->qscale == 0) return -1;
    if (d->pict_type != PT_I) {
      d->f_code = (int)gb_get(gb, 3);
      if (d->f_code == 0) { d->f_code = 1; return -1; }
    }
    if (d->pict_type == PT_B) {
      d->b_code = (int)gb_get(gb, 3);
      if (d->b_code == 0) { d->b_code = 1; return -1; }
    }
    if (!d->scalability) {
      if (d->shape != RECT_SHAPE && d->pict_type != PT_I) gb_skip(gb, 1);
    } else {
      if (d->enhancement_type) gb_skip(gb, 1);
      gb_skip(gb, 2);
    }
  }
  // buggy encoders which don't set the low_delay flag (divx4/xvid/opendivx)
  if (d->vo_type == 0 && d->vol_control == 0 && d->divx_version == -1 && d->picture_number == 0)
    d->low_delay = 1;
  d->picture_number++;
  return 0;
}

static void decode_gop_header(Dec* d) {
  GB* gb = &d->gb;
  int hours, minutes, seconds;
  if (!gb_show(gb, 23)) return;
  hours = (int)gb_get(gb, 5);
  minutes = (int)gb_get(gb, 6);
  gb_skip(gb, 1);
  seconds = (int)gb_get(gb, 6);
  d->time_base = seconds + 60 * (minutes + 60 * hours);
  gb_skip(gb, 2);
}

// parse start codes up to and including the VOP header; 0 ok, FRAME_SKIPPED, 1 no VOP, <0 error
static int parse_picture_header(Dec* d) {
  GB* gb = &d->gb;
  U32 startcode = 0xff;
  int vol = 0;
  gb_align(gb);
  for (;;) {
    if (gb->pos >= gb->size) return 1;
    startcode = ((startcode << 8) | gb_get(gb, 8)) & 0xffffffff;
    if ((startcode & 0xFFFFFF00) != 0x100) continue;
    if (startcode >= 0x120 && startcode <= 0x12F) {
      if (!vol) {
        vol++;
        if (decode_vol_header(d) < 0) return -1;
      }
    } else if (startcode == 0x1B2) {
      decode_user_data(d);
    } else if (startcode == 0x1B3) {
      decode_gop_header(d);
    } else if (startcode == 0x1B5) {
      decode_visual_object(d);
    } else if (startcode == 0x1B6) {
      break;
    }
    gb_align(gb);
    startcode = 0xff;
  }
  if (!d->have_vol) { fprintf(stderr, "VOP before VOL header\n"); return -1; }
  return decode_vop_header(d);
}

static void workaround_bugs(Dec* d) {
  if (d->xvid_build >= 0 && d->divx_version >= 0) d->divx_version = d->divx_build = -1;
  if (d->bugs & BUG_AUTODETECT) {
    if (d->divx_version >= 500 && d->divx_build < 1814) d->bugs |= BUG_QPEL_CHROMA;
    if (d->divx_version > 502 && d->divx_build < 1814) d->bugs |= BUG_QPEL_CHROMA2;
    if ((unsigned)d->xvid_build <= 1U) d->bugs |= BUG_QPEL_CHROMA;
    if ((unsigned)d->xvid_build <= 12U) d->bugs |= BUG_EDGE;
    if ((unsigned)d->xvid_build <= 32U) d->bugs |= BUG_DC_CLIP;
    if ((unsigned)d->lavc_build < 4653U) d->bugs |= BUG_STD_QPEL;
    if ((unsigned)d->lavc_build < 4655U) d->bugs |= BUG_DIRECT_BLOCKSIZE;
    if ((unsigned)d->lavc_build < 4670U) d->bugs |= BUG_EDGE;
    if ((unsigned)d->lavc_build <= 4712U) d->bugs |= BUG_DC_CLIP;
    if ((d->lavc_build & 0xFF) >= 100) {
      if (d->lavc_build > 3621476 && d->lavc_build < 3752552 &&
          (d->lavc_build < 3752037 || d->lavc_build > 3752191))
        d->bugs |= BUG_IEDGE;
    }
    if (d->divx_version >= 0) d->bugs |= BUG_DIRECT_BLOCKSIZE;
    if ((unsigned)d->divx_version < 500U) d->bugs |= BUG_EDGE;
    if (d->divx_version >= 0) d->bugs |= BUG_HPEL_CHROMA;
  }
  if (d->xvid_build >= 0) d->idct_xvid = 1;
}

//--- IDCT (FFmpeg simple_idct 8-bit, and the XviD IDCT) --------------------------------

#define W1 22725
#define W2 21407
#define W3 19266
#define W4 16383
#define W5 12873
#define W6 8867
#define W7 4520
#define ROW_SHIFT 11
#define COL_SHIFT 20

static void simple_idct_row(S16* row) {
  U32 a0, a1, a2, a3, b0, b1, b2, b3;
  if (!(row[1] | row[2] | row[3] | row[4] | row[5] | row[6] | row[7])) {
    S16 t = (S16)((U32)(row[0] * 8) & 0xffff);
    row[0] = row[1] = row[2] = row[3] = row[4] = row[5] = row[6] = row[7] = t;
    return;
  }
  a0 = ((U32)W4 * row[0]) + (1 << (ROW_SHIFT - 1));
  a1 = a0; a2 = a0; a3 = a0;
  a0 += (U32)W2 * row[2]; a1 += (U32)W6 * row[2]; a2 -= (U32)W6 * row[2]; a3 -= (U32)W2 * row[2];
  b0 = (U32)(W1 * row[1]) + (U32)W3 * row[3];
  b1 = (U32)(W3 * row[1]) + (U32)-W7 * row[3];
  b2 = (U32)(W5 * row[1]) + (U32)-W1 * row[3];
  b3 = (U32)(W7 * row[1]) + (U32)-W5 * row[3];
  if (row[4] | row[5] | row[6] | row[7]) {
    a0 += (U32)W4 * row[4] + (U32)W6 * row[6];
    a1 += (U32)-W4 * row[4] - (U32)W2 * row[6];
    a2 += (U32)-W4 * row[4] + (U32)W2 * row[6];
    a3 += (U32)W4 * row[4] - (U32)W6 * row[6];
    b0 += (U32)W5 * row[5] + (U32)W7 * row[7];
    b1 += (U32)-W1 * row[5] + (U32)-W5 * row[7];
    b2 += (U32)W7 * row[5] + (U32)W3 * row[7];
    b3 += (U32)W3 * row[5] + (U32)-W1 * row[7];
  }
  row[0] = (S16)((int)(a0 + b0) >> ROW_SHIFT);
  row[7] = (S16)((int)(a0 - b0) >> ROW_SHIFT);
  row[1] = (S16)((int)(a1 + b1) >> ROW_SHIFT);
  row[6] = (S16)((int)(a1 - b1) >> ROW_SHIFT);
  row[2] = (S16)((int)(a2 + b2) >> ROW_SHIFT);
  row[5] = (S16)((int)(a2 - b2) >> ROW_SHIFT);
  row[3] = (S16)((int)(a3 + b3) >> ROW_SHIFT);
  row[4] = (S16)((int)(a3 - b3) >> ROW_SHIFT);
}

static void simple_idct_col(const S16* col, int out[8]) {
  U32 a0, a1, a2, a3, b0, b1, b2, b3;
  a0 = (U32)W4 * (col[8 * 0] + ((1 << (COL_SHIFT - 1)) / W4));
  a1 = a0; a2 = a0; a3 = a0;
  a0 += (U32)W2 * col[8 * 2]; a1 += (U32)W6 * col[8 * 2];
  a2 += (U32)-W6 * col[8 * 2]; a3 += (U32)-W2 * col[8 * 2];
  b0 = (U32)W1 * col[8 * 1]; b1 = (U32)W3 * col[8 * 1]; b2 = (U32)W5 * col[8 * 1]; b3 = (U32)W7 * col[8 * 1];
  b0 += (U32)W3 * col[8 * 3]; b1 += (U32)-W7 * col[8 * 3]; b2 += (U32)-W1 * col[8 * 3]; b3 += (U32)-W5 * col[8 * 3];
  a0 += (U32)W4 * col[8 * 4]; a1 += (U32)-W4 * col[8 * 4]; a2 += (U32)-W4 * col[8 * 4]; a3 += (U32)W4 * col[8 * 4];
  b0 += (U32)W5 * col[8 * 5]; b1 += (U32)-W1 * col[8 * 5]; b2 += (U32)W7 * col[8 * 5]; b3 += (U32)W3 * col[8 * 5];
  a0 += (U32)W6 * col[8 * 6]; a1 += (U32)-W2 * col[8 * 6]; a2 += (U32)W2 * col[8 * 6]; a3 += (U32)-W6 * col[8 * 6];
  b0 += (U32)W7 * col[8 * 7]; b1 += (U32)-W5 * col[8 * 7]; b2 += (U32)W3 * col[8 * 7]; b3 += (U32)-W1 * col[8 * 7];
  out[0] = (int)(a0 + b0) >> COL_SHIFT;
  out[1] = (int)(a1 + b1) >> COL_SHIFT;
  out[2] = (int)(a2 + b2) >> COL_SHIFT;
  out[3] = (int)(a3 + b3) >> COL_SHIFT;
  out[4] = (int)(a3 - b3) >> COL_SHIFT;
  out[5] = (int)(a2 - b2) >> COL_SHIFT;
  out[6] = (int)(a1 - b1) >> COL_SHIFT;
  out[7] = (int)(a0 - b0) >> COL_SHIFT;
}

static void simple_idct_put_add(U8* dest, int stride, S16* block, int add) {
  int i, j, o[8];
  for (i = 0; i < 8; i++) simple_idct_row(block + i * 8);
  for (i = 0; i < 8; i++) {
    simple_idct_col(block + i, o);
    for (j = 0; j < 8; j++) {
      U8* p = dest + j * stride + i;
      *p = clip8(add ? *p + o[j] : o[j]);
    }
  }
}

#define X_ROW_SHIFT 11
#define X_COL_SHIFT 6
static const int TAB04[] = { 22725, 21407, 19266, 16384, 12873,  8867, 4520 };
static const int TAB17[] = { 31521, 29692, 26722, 22725, 17855, 12299, 6270 };
static const int TAB26[] = { 29692, 27969, 25172, 21407, 16819, 11585, 5906 };
static const int TAB35[] = { 26722, 25172, 22654, 19266, 15137, 10426, 5315 };

static int xvid_idct_row(S16* in, const int* tab, int rnd) {
  const U32 c1 = tab[0], c2 = tab[1], c3 = tab[2], c4 = tab[3], c5 = tab[4], c6 = tab[5], c7 = tab[6];
  const int right = in[5] | in[6] | in[7];
  const int left = in[1] | in[2] | in[3];
  if (!(right | in[4])) {
    const int k = (int)(c4 * in[0] + rnd);
    if (left) {
      const U32 a0 = k + c2 * in[2], a1 = k + c6 * in[2], a2 = k - c6 * in[2], a3 = k - c2 * in[2];
      const int b0 = (int)(c1 * in[1] + c3 * in[3]);
      const int b1 = (int)(c3 * in[1] - c7 * in[3]);
      const int b2 = (int)(c5 * in[1] - c1 * in[3]);
      const int b3 = (int)(c7 * in[1] - c5 * in[3]);
      in[0] = (S16)((int)(a0 + b0) >> X_ROW_SHIFT);
      in[1] = (S16)((int)(a1 + b1) >> X_ROW_SHIFT);
      in[2] = (S16)((int)(a2 + b2) >> X_ROW_SHIFT);
      in[3] = (S16)((int)(a3 + b3) >> X_ROW_SHIFT);
      in[4] = (S16)((int)(a3 - b3) >> X_ROW_SHIFT);
      in[5] = (S16)((int)(a2 - b2) >> X_ROW_SHIFT);
      in[6] = (S16)((int)(a1 - b1) >> X_ROW_SHIFT);
      in[7] = (S16)((int)(a0 - b0) >> X_ROW_SHIFT);
    } else {
      const int a0 = k >> X_ROW_SHIFT;
      if (a0) {
        in[0] = in[1] = in[2] = in[3] = in[4] = in[5] = in[6] = in[7] = (S16)a0;
      } else
        return 0;
    }
  } else if (!(left | right)) {
    const int a0 = (int)(rnd + c4 * (in[0] + in[4])) >> X_ROW_SHIFT;
    const int a1 = (int)(rnd + c4 * (in[0] - in[4])) >> X_ROW_SHIFT;
    in[0] = in[3] = in[4] = in[7] = (S16)a0;
    in[1] = in[2] = in[5] = in[6] = (S16)a1;
  } else {
    const U32 k = c4 * in[0] + rnd;
    const U32 a0 = k + c2 * in[2] + c4 * in[4] + c6 * in[6];
    const U32 a1 = k + c6 * in[2] - c4 * in[4] - c2 * in[6];
    const U32 a2 = k - c6 * in[2] - c4 * in[4] + c2 * in[6];
    const U32 a3 = k - c2 * in[2] + c4 * in[4] - c6 * in[6];
    const U32 b0 = c1 * in[1] + c3 * in[3] + c5 * in[5] + c7 * in[7];
    const U32 b1 = c3 * in[1] - c7 * in[3] - c1 * in[5] - c5 * in[7];
    const U32 b2 = c5 * in[1] - c1 * in[3] + c7 * in[5] + c3 * in[7];
    const U32 b3 = c7 * in[1] - c5 * in[3] + c3 * in[5] - c1 * in[7];
    in[0] = (S16)((int)(a0 + b0) >> X_ROW_SHIFT);
    in[1] = (S16)((int)(a1 + b1) >> X_ROW_SHIFT);
    in[2] = (S16)((int)(a2 + b2) >> X_ROW_SHIFT);
    in[3] = (S16)((int)(a3 + b3) >> X_ROW_SHIFT);
    in[4] = (S16)((int)(a3 - b3) >> X_ROW_SHIFT);
    in[5] = (S16)((int)(a2 - b2) >> X_ROW_SHIFT);
    in[6] = (S16)((int)(a1 - b1) >> X_ROW_SHIFT);
    in[7] = (S16)((int)(a0 - b0) >> X_ROW_SHIFT);
  }
  return 1;
}

#define TAN1  0x32EC
#define TAN2  0x6A0A
#define TAN3  0xAB0E
#define SQRT2 0x5A82
#define MULT(c, x, n) ((unsigned)((int)((c) * (unsigned)(x)) >> (n)))
#define BUTTERFLY(a, b, tmp) (tmp) = (a) + (b); (b) = (a) - (b); (a) = (tmp)

static void xvid_idct_col_8(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, spill;
  mm4 = in[7 * 8]; mm5 = in[5 * 8]; mm6 = in[3 * 8]; mm7 = in[1 * 8];
  mm0 = MULT(TAN1, mm4, 16) + mm7;
  mm1 = MULT(TAN1, mm7, 16) - mm4;
  mm2 = MULT(TAN3, mm5, 16) + mm6;
  mm3 = MULT(TAN3, mm6, 16) - mm5;
  mm7 = mm0 + mm2;
  mm4 = mm1 - mm3;
  mm0 = mm0 - mm2;
  mm1 = mm1 + mm3;
  mm6 = mm0 + mm1;
  mm5 = mm0 - mm1;
  mm5 = 2 * MULT(SQRT2, mm5, 16);
  mm6 = 2 * MULT(SQRT2, mm6, 16);
  mm1 = in[2 * 8];
  mm2 = in[6 * 8];
  mm3 = MULT(TAN2, mm2, 16) + mm1;
  mm2 = MULT(TAN2, mm1, 16) - mm2;
  mm0 = in[0 * 8] + in[4 * 8];
  mm1 = in[0 * 8] - in[4 * 8];
  BUTTERFLY(mm0, mm3, spill);
  BUTTERFLY(mm0, mm7, spill);
  in[8 * 0] = (S16)(mm0 >> X_COL_SHIFT);
  in[8 * 7] = (S16)(mm7 >> X_COL_SHIFT);
  BUTTERFLY(mm3, mm4, mm0);
  in[8 * 3] = (S16)(mm3 >> X_COL_SHIFT);
  in[8 * 4] = (S16)(mm4 >> X_COL_SHIFT);
  BUTTERFLY(mm1, mm2, mm0);
  BUTTERFLY(mm1, mm6, mm0);
  in[8 * 1] = (S16)(mm1 >> X_COL_SHIFT);
  in[8 * 6] = (S16)(mm6 >> X_COL_SHIFT);
  BUTTERFLY(mm2, mm5, mm0);
  in[8 * 2] = (S16)(mm2 >> X_COL_SHIFT);
  in[8 * 5] = (S16)(mm5 >> X_COL_SHIFT);
}

static void xvid_idct_col_4(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, spill;
  mm0 = in[1 * 8];
  mm2 = in[3 * 8];
  mm1 = MULT(TAN1, mm0, 16);
  mm3 = MULT(TAN3, mm2, 16);
  mm7 = mm0 + mm2;
  mm4 = mm1 - mm3;
  mm0 = mm0 - mm2;
  mm1 = mm1 + mm3;
  mm6 = mm0 + mm1;
  mm5 = mm0 - mm1;
  mm6 = 2 * MULT(SQRT2, mm6, 16);
  mm5 = 2 * MULT(SQRT2, mm5, 16);
  mm0 = mm1 = in[0 * 8];
  mm3 = in[2 * 8];
  mm2 = MULT(TAN2, mm3, 16);
  BUTTERFLY(mm0, mm3, spill);
  BUTTERFLY(mm0, mm7, spill);
  in[8 * 0] = (S16)(mm0 >> X_COL_SHIFT);
  in[8 * 7] = (S16)(mm7 >> X_COL_SHIFT);
  BUTTERFLY(mm3, mm4, mm0);
  in[8 * 3] = (S16)(mm3 >> X_COL_SHIFT);
  in[8 * 4] = (S16)(mm4 >> X_COL_SHIFT);
  BUTTERFLY(mm1, mm2, mm0);
  BUTTERFLY(mm1, mm6, mm0);
  in[8 * 1] = (S16)(mm1 >> X_COL_SHIFT);
  in[8 * 6] = (S16)(mm6 >> X_COL_SHIFT);
  BUTTERFLY(mm2, mm5, mm0);
  in[8 * 2] = (S16)(mm2 >> X_COL_SHIFT);
  in[8 * 5] = (S16)(mm5 >> X_COL_SHIFT);
}

static void xvid_idct_col_3(S16* in) {
  int mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, spill;
  mm7 = in[1 * 8];
  mm4 = MULT(TAN1, mm7, 16);
  mm6 = mm7 + mm4;
  mm5 = mm7 - mm4;
  mm6 = 2 * MULT(SQRT2, mm6, 16);
  mm5 = 2 * MULT(SQRT2, mm5, 16);
  mm0 = mm1 = in[0 * 8];
  mm3 = in[2 * 8];
  mm2 = MULT(TAN2, mm3, 16);
  BUTTERFLY(mm0, mm3, spill);
  BUTTERFLY(mm0, mm7, spill);
  in[8 * 0] = (S16)(mm0 >> X_COL_SHIFT);
  in[8 * 7] = (S16)(mm7 >> X_COL_SHIFT);
  BUTTERFLY(mm3, mm4, mm0);
  in[8 * 3] = (S16)(mm3 >> X_COL_SHIFT);
  in[8 * 4] = (S16)(mm4 >> X_COL_SHIFT);
  BUTTERFLY(mm1, mm2, mm0);
  BUTTERFLY(mm1, mm6, mm0);
  in[8 * 1] = (S16)(mm1 >> X_COL_SHIFT);
  in[8 * 6] = (S16)(mm6 >> X_COL_SHIFT);
  BUTTERFLY(mm2, mm5, mm0);
  in[8 * 2] = (S16)(mm2 >> X_COL_SHIFT);
  in[8 * 5] = (S16)(mm5 >> X_COL_SHIFT);
}

static void xvid_idct(S16* in) {
  int i, rows = 0x07;
  xvid_idct_row(in + 0 * 8, TAB04, 65536);
  xvid_idct_row(in + 1 * 8, TAB17, 3597);
  xvid_idct_row(in + 2 * 8, TAB26, 2260);
  if (xvid_idct_row(in + 3 * 8, TAB35, 1203)) rows |= 0x08;
  if (xvid_idct_row(in + 4 * 8, TAB04, 0)) rows |= 0x10;
  if (xvid_idct_row(in + 5 * 8, TAB35, 120)) rows |= 0x20;
  if (xvid_idct_row(in + 6 * 8, TAB26, 512)) rows |= 0x40;
  if (xvid_idct_row(in + 7 * 8, TAB17, 512)) rows |= 0x80;
  if (rows & 0xF0) { for (i = 0; i < 8; i++) xvid_idct_col_8(in + i); }
  else if (rows & 0x08) { for (i = 0; i < 8; i++) xvid_idct_col_4(in + i); }
  else { for (i = 0; i < 8; i++) xvid_idct_col_3(in + i); }
}

static void idct_put(Dec* d, U8* dest, int stride, S16* block) {
  if (d->idct_xvid) {
    int i, j;
    xvid_idct(block);
    for (i = 0; i < 8; i++) for (j = 0; j < 8; j++) dest[i * stride + j] = clip8(block[i * 8 + j]);
  } else
    simple_idct_put_add(dest, stride, block, 0);
}

static void idct_add(Dec* d, U8* dest, int stride, S16* block) {
  if (d->idct_xvid) {
    int i, j;
    xvid_idct(block);
    for (i = 0; i < 8; i++) for (j = 0; j < 8; j++) dest[i * stride + j] = clip8(dest[i * stride + j] + block[i * 8 + j]);
  } else
    simple_idct_put_add(dest, stride, block, 1);
}

//--- dequantisation ------------------------------------------------------------------

static void unquant_intra(Dec* d, S16* block, int n, int qscale) {
  int i, level;
  block[0] = (S16)(block[0] * (n < 4 ? d->y_dc_scale : d->c_dc_scale));
  if (d->mpeg_quant) {   // MPEG-2 style, with mismatch control (FFmpeg's bitexact variant)
    int sum = -1, q2 = qscale << 1;
    sum += block[0];
    for (i = 1; i < 64; i++) {
      level = block[i];
      if (level) {
        if (level < 0) level = -(int)((-level) * q2 * d->intra_matrix[i] >> 4);
        else level = (int)(level * q2 * d->intra_matrix[i] >> 4);
        block[i] = (S16)level;
        sum += level;
      }
    }
    block[63] ^= sum & 1;
  } else {
    int qmul = qscale << 1, qadd = (qscale - 1) | 1;
    for (i = 1; i < 64; i++) {
      level = block[i];
      if (level) block[i] = (S16)(level < 0 ? level * qmul - qadd : level * qmul + qadd);
    }
  }
}

static void unquant_inter_mpeg(Dec* d, S16* block, int qscale) {
  int i, level, sum = -1, q2 = qscale << 1;
  for (i = 0; i < 64; i++) {
    level = block[i];
    if (level) {
      if (level < 0) level = -(int)((((-level) << 1) + 1) * q2 * d->inter_matrix[i] >> 5);
      else level = (int)(((level << 1) + 1) * q2 * d->inter_matrix[i] >> 5);
      block[i] = (S16)level;
      sum += level;
    }
  }
  block[63] ^= sum & 1;
}

//--- intra prediction ------------------------------------------------------------------

static int pred_dc(Dec* d, int n, int* dir_ptr) {
  const S16* dc_val = d->dc_val + d->block_index[n];
  const int wrap = d->block_wrap[n];
  int a = dc_val[-1], b = dc_val[-1 - wrap], c = dc_val[-wrap];
  if (d->first_slice_line && n != 3) {
    if (n != 2) b = c = 1024;
    if (n != 1 && d->mb_x == d->resync_mb_x) b = a = 1024;
  }
  if (d->mb_x == d->resync_mb_x && d->mb_y == d->resync_mb_y + 1) {
    if (n == 0 || n == 4 || n == 5) b = 1024;
  }
  if (abs(a - b) < abs(b - c)) { *dir_ptr = 1; return c; }
  *dir_ptr = 0;
  return a;
}

static int get_level_dc(Dec* d, int n, int pred, int level) {
  int scale = n < 4 ? d->y_dc_scale : d->c_dc_scale, ret;
  pred = (pred + (scale >> 1)) / scale;
  level += pred;
  ret = level;
  level *= scale;
  if (level & ~2047) {
    if (level < 0) level = 0;
    else if (!(d->bugs & BUG_DC_CLIP)) level = 2047;
  }
  d->dc_val[d->block_index[n]] = (S16)level;
  return ret;
}

static int decode_dc(Dec* d, int n, int* dir_ptr) {
  int level, code, pred;
  code = vlc_get(&d->gb, n < 4 ? &vlc_dc_lum : &vlc_dc_chrom);
  if (code < 0) return -1;
  if (code == 0) level = 0;
  else {
    level = gb_xbits(&d->gb, code);
    if (code > 8) gb_skip(&d->gb, 1);   // marker
  }
  pred = pred_dc(d, n, dir_ptr);
  return get_level_dc(d, n, pred, level);
}

static void pred_ac(Dec* d, S16* block, int n, int dir) {
  int i;
  S16 *ac_val = d->ac_val[d->block_index[n]], *ac_val1 = ac_val;
  const S8* qt = d->qscale_table;
  if (d->ac_pred) {
    if (dir == 0) {
      const int xy = d->mb_x - 1 + d->mb_y * d->mb_stride;
      ac_val -= 16;
      if (d->mb_x == 0 || d->qscale == qt[xy] || n == 1 || n == 3) {
        for (i = 1; i < 8; i++) block[i << 3] += ac_val[i];
      } else {
        for (i = 1; i < 8; i++) block[i << 3] += ROUNDED_DIV(ac_val[i] * qt[xy], d->qscale);
      }
    } else {
      const int xy = d->mb_x + d->mb_y * d->mb_stride - d->mb_stride;
      ac_val -= 16 * d->block_wrap[n];
      if (d->mb_y == 0 || d->qscale == qt[xy] || n == 2 || n == 3) {
        for (i = 1; i < 8; i++) block[i] += ac_val[i + 8];
      } else {
        for (i = 1; i < 8; i++) block[i] += ROUNDED_DIV(ac_val[i + 8] * qt[xy], d->qscale);
      }
    }
  }
  for (i = 1; i < 8; i++) ac_val1[i] = block[i << 3];
  for (i = 1; i < 8; i++) ac_val1[8 + i] = block[i];
}

static void clean_intra_table_entries(Dec* d, int xy) {
  if (d->mbintra_table[xy]) {
    const int wrap = d->b8_stride, bi = d->block_index[0];
    d->mbintra_table[xy] = 0;
    d->dc_val[bi] = d->dc_val[bi + 1] = d->dc_val[bi + wrap] = d->dc_val[bi + wrap + 1] = 1024;
    d->dc_val[d->block_index[4]] = d->dc_val[d->block_index[5]] = 1024;
    memset(d->ac_val[bi + 1], 0, sizeof(*d->ac_val));
    memset(d->ac_val[bi + wrap], 0, 2 * sizeof(*d->ac_val));
    memset(d->ac_val[d->block_index[4]], 0, sizeof(*d->ac_val));
    memset(d->ac_val[d->block_index[5]], 0, sizeof(*d->ac_val));
  }
}

static void clean_buffers(Dec* d) {  // ff_mpeg4_clean_buffers, at the start of each video packet
  const int l_wrap = d->b8_stride, c_wrap = d->mb_stride;
  const int l_xy = (2 * d->mb_y - 1) * l_wrap + d->mb_x * 2 - 1;
  const int u_xy = 2 * d->mb_h * l_wrap + d->mb_y * c_wrap + d->mb_x - 1;
  const int v_xy = u_xy + c_wrap * (d->mb_h + 1);
  memset(d->ac_val + l_xy, 0, (l_wrap * 2 + 1) * sizeof(*d->ac_val));
  memset(d->ac_val + u_xy, 0, (c_wrap + 1) * sizeof(*d->ac_val));
  memset(d->ac_val + v_xy, 0, (c_wrap + 1) * sizeof(*d->ac_val));
  d->last_mv[0][0][0] = d->last_mv[0][0][1] = d->last_mv[1][0][0] = d->last_mv[1][0][1] = 0;
}

//--- motion vectors ----------------------------------------------------------------------

static S16* pred_motion(Dec* d, int block, int* px, int* py) {
  static const int off[4] = { 2, 1, 1, -1 };
  const int wrap = d->b8_stride;
  S16 (*mot_val)[2] = d->cur->mv + d->block_index[block];
  S16 *A, *B, *C;
  A = mot_val[-1];
  if (d->first_slice_line && block < 3) {
    if (block == 0) {
      if (d->mb_x == d->resync_mb_x) {
        *px = *py = 0;
      } else if (d->mb_x + 1 == d->resync_mb_x) {
        C = mot_val[off[block] - wrap];
        if (d->mb_x == 0) { *px = C[0]; *py = C[1]; }
        else { *px = mid_pred(A[0], 0, C[0]); *py = mid_pred(A[1], 0, C[1]); }
      } else {
        *px = A[0]; *py = A[1];
      }
    } else if (block == 1) {
      if (d->mb_x + 1 == d->resync_mb_x) {
        C = mot_val[off[block] - wrap];
        *px = mid_pred(A[0], 0, C[0]); *py = mid_pred(A[1], 0, C[1]);
      } else {
        *px = A[0]; *py = A[1];
      }
    } else {
      B = mot_val[-wrap];
      C = mot_val[off[block] - wrap];
      if (d->mb_x == d->resync_mb_x) A[0] = A[1] = 0;
      *px = mid_pred(A[0], B[0], C[0]);
      *py = mid_pred(A[1], B[1], C[1]);
    }
  } else {
    B = mot_val[-wrap];
    C = mot_val[off[block] - wrap];
    *px = mid_pred(A[0], B[0], C[0]);
    *py = mid_pred(A[1], B[1], C[1]);
  }
  return *mot_val;
}

static int decode_motion(Dec* d, int pred, int f_code) {
  int code, val, sign, shift;
  code = vlc_get(&d->gb, &vlc_mv);
  if (code == 0) return pred;
  if (code < 0) return 0xffff;
  sign = gb_get1(&d->gb);
  shift = f_code - 1;
  val = code;
  if (shift) {
    val = (val - 1) << shift;
    val |= (int)gb_get(&d->gb, shift);
    val++;
  }
  if (sign) val = -val;
  val += pred;
  return sign_extend(val, 5 + f_code);
}

static void update_motion_val(Dec* d) {
  const int mb_xy = d->mb_y * d->mb_stride + d->mb_x, wrap = d->b8_stride, xy = d->block_index[0];
  S16 (*mv)[2] = d->cur->mv;
  if (d->mv_type != MV_8X8) {
    int motion_x, motion_y;
    if (d->mb_intra) {
      motion_x = motion_y = 0;
    } else if (d->mv_type == MV_16X16) {
      motion_x = d->mv[0][0][0];
      motion_y = d->mv[0][0][1];
    } else {
      int i;
      motion_x = d->mv[0][0][0] + d->mv[0][1][0];
      motion_y = d->mv[0][0][1] + d->mv[0][1][1];
      motion_x = (motion_x >> 1) | (motion_x & 1);
      for (i = 0; i < 2; i++) {
        d->p_field_mv[mb_xy][i][0] = (S16)d->mv[0][i][0];
        d->p_field_mv[mb_xy][i][1] = (S16)d->mv[0][i][1];
      }
      d->cur->ref_index[4 * mb_xy] = d->cur->ref_index[4 * mb_xy + 1] = (U8)d->field_select[0][0];
      d->cur->ref_index[4 * mb_xy + 2] = d->cur->ref_index[4 * mb_xy + 3] = (U8)d->field_select[0][1];
    }
    mv[xy][0] = mv[xy + 1][0] = mv[xy + wrap][0] = mv[xy + 1 + wrap][0] = (S16)motion_x;
    mv[xy][1] = mv[xy + 1][1] = mv[xy + wrap][1] = mv[xy + 1 + wrap][1] = (S16)motion_y;
  }
}

static int get_amv(Dec* d, int n) {
  int x, y, mb_v, sum, dx, dy, shift;
  int len = 1 << (d->f_code + 4);
  const int a = d->warp_acc;
  if (d->bugs & BUG_AMV) len >>= d->qpel;
  if (d->real_warp_points == 1) {
    if (d->divx_version == 500 && d->divx_build == 413 && a >= d->qpel)
      sum = d->sprite_offset[0][n] / (1 << (a - d->qpel));
    else
      sum = RSHIFT(d->sprite_offset[0][n] * (1 << d->qpel), a);
  } else {
    dx = d->sprite_delta[n][0];
    dy = d->sprite_delta[n][1];
    shift = d->sprite_shift[0];
    if (n) dy -= 1 << (shift + a + 1);
    else dx -= 1 << (shift + a + 1);
    mb_v = (int)(d->sprite_offset[0][n] + dx * d->mb_x * 16U + dy * d->mb_y * 16U);
    sum = 0;
    for (y = 0; y < 16; y++) {
      int v = (int)(mb_v + (unsigned)dy * y);
      for (x = 0; x < 16; x++) {
        sum += v >> shift;
        v += dx;
      }
    }
    sum = RSHIFT(sum, a + 8 - d->qpel);
  }
  if (sum < -len) sum = -len;
  else if (sum >= len) sum = len - 1;
  return sum;
}

static void set_one_direct_mv(Dec* d, int mx, int my, int i) {
  const int xy = d->block_index[i];
  const U16 time_pp = d->pp_time, time_pb = d->pb_time;
  const S16* p = d->next->mv[xy];
  int p_mx = p[0], p_my = p[1];
  if ((unsigned)(p_mx + 32) < 64) {
    d->mv[0][i][0] = d->direct_scale_mv[0][p_mx + 32] + mx;
    d->mv[1][i][0] = mx ? d->mv[0][i][0] - p_mx : d->direct_scale_mv[1][p_mx + 32];
  } else {
    d->mv[0][i][0] = p_mx * time_pb / time_pp + mx;
    d->mv[1][i][0] = mx ? d->mv[0][i][0] - p_mx : p_mx * (time_pb - time_pp) / time_pp;
  }
  if ((unsigned)(p_my + 32) < 64) {
    d->mv[0][i][1] = d->direct_scale_mv[0][p_my + 32] + my;
    d->mv[1][i][1] = my ? d->mv[0][i][1] - p_my : d->direct_scale_mv[1][p_my + 32];
  } else {
    d->mv[0][i][1] = p_my * time_pb / time_pp + my;
    d->mv[1][i][1] = my ? d->mv[0][i][1] - p_my : p_my * (time_pb - time_pp) / time_pp;
  }
}

static int set_direct_mv(Dec* d, int mx, int my) {
  const int mb_index = d->mb_x + d->mb_y * d->mb_stride;
  const int colocated = d->next->mbtype[mb_index];
  int i;
  if (colocated & MBT_8x8) {
    d->mv_type = MV_8X8;
    for (i = 0; i < 4; i++) set_one_direct_mv(d, mx, my, i);
    return MBT_DIRECT2 | MBT_8x8 | MBT_BIDIR;
  } else if (colocated & MBT_INTERLACED) {
    d->mv_type = MV_FIELD;
    for (i = 0; i < 2; i++) {
      int field_select = d->next->ref_index[4 * mb_index + 2 * i];
      U16 time_pp, time_pb;
      const S16* pf = d->p_field_mv[mb_index][i];
      d->field_select[0][i] = field_select;
      d->field_select[1][i] = i;
      if (d->top_field_first) {
        time_pp = (U16)(d->pp_field_time - field_select + i);
        time_pb = (U16)(d->pb_field_time - field_select + i);
      } else {
        time_pp = (U16)(d->pp_field_time + field_select - i);
        time_pb = (U16)(d->pb_field_time + field_select - i);
      }
      d->mv[0][i][0] = pf[0] * time_pb / time_pp + mx;
      d->mv[0][i][1] = pf[1] * time_pb / time_pp + my;
      d->mv[1][i][0] = mx ? d->mv[0][i][0] - pf[0] : pf[0] * (time_pb - time_pp) / time_pp;
      d->mv[1][i][1] = my ? d->mv[0][i][1] - pf[1] : pf[1] * (time_pb - time_pp) / time_pp;
    }
    return MBT_DIRECT2 | MBT_16x8 | MBT_BIDIR | MBT_INTERLACED;
  } else {
    set_one_direct_mv(d, mx, my, 0);
    for (i = 1; i < 4; i++) {
      d->mv[0][i][0] = d->mv[0][0][0]; d->mv[0][i][1] = d->mv[0][0][1];
      d->mv[1][i][0] = d->mv[1][0][0]; d->mv[1][i][1] = d->mv[1][0][1];
    }
    // FFmpeg tests avctx->workaround_bugs here (the user option), not the autodetected flags, so the
    // detected FF_BUG_DIRECT_BLOCKSIZE never takes effect; match that
    d->mv_type = !d->qpel ? MV_16X16 : MV_8X8;
    return MBT_DIRECT2 | MBT_16x16 | MBT_BIDIR;
  }
}

//--- macroblock / block parsing --------------------------------------------------------

static int decode_block(Dec* d, S16* block, int n, int coded, int intra, int use_intra_dc_vlc) {
  int level, i, last, run, qmul, qadd, pred = 0, dc_pred_dir = 0, sym;
  const RL* rl;
  const U8* scan;
  GB* gb = &d->gb;
  if (intra) {
    if (use_intra_dc_vlc) {
      level = decode_dc(d, n, &dc_pred_dir);
      if (level < 0) return -1;
      block[0] = (S16)level;
      i = 0;
    } else {
      i = -1;
      pred = pred_dc(d, n, &dc_pred_dir);
    }
    if (!coded) goto not_coded;
    rl = &rl_intra;
    if (d->ac_pred) scan = dc_pred_dir == 0 ? d->intra_v_scan : d->intra_h_scan;
    else scan = d->intra_scan;
    qmul = 1;
    qadd = 0;
  } else {
    i = -1;
    if (!coded) { d->last_index[n] = i; return 0; }
    rl = &rl_inter;
    scan = d->intra_scan;
    if (d->mpeg_quant) { qmul = 1; qadd = 0; }
    else { qmul = d->qscale << 1; qadd = (d->qscale - 1) | 1; }
  }
  for (;;) {
    sym = vlc_get(gb, &rl->v);
    if (sym < 0) return -1;
    if (sym == rl->n) {                 // escape
      U32 cache = gb_show(gb, 2);
      if (cache & 2) {
        if (cache & 1) {                // third escape
          gb_skip(gb, 2);
          last = gb_get1(gb);
          run = (int)gb_get(gb, 6);
          if (!gb_get1(gb)) return -1;  // marker
          level = gb_sget(gb, 12);
          if (!gb_get1(gb)) return -1;  // marker
          if (level > 0) level = level * qmul + qadd;
          else level = level * qmul - qadd;
          if ((unsigned)(level + 2048) > 4095) level = level < 0 ? -2048 : 2047;
          i += run + 1;
          if (last) i += 192;
        } else {                        // second escape: run offset
          gb_skip(gb, 2);
          sym = vlc_get(gb, &rl->v);
          if (sym < 0 || sym == rl->n) return -1;
          run = rl->run[sym];
          level = rl->level[sym];
          last = sym >= rl->last;
          i += run + 1 + (last ? 192 : 0) + rl->max_run[last][level] + 1;
          level = level * qmul + qadd;
          if (gb_get1(gb)) level = -level;
        }
      } else {                          // first escape: level offset
        gb_skip(gb, 1);
        sym = vlc_get(gb, &rl->v);
        if (sym < 0 || sym == rl->n) return -1;
        run = rl->run[sym];
        level = rl->level[sym];
        last = sym >= rl->last;
        i += run + 1 + (last ? 192 : 0);
        level = level * qmul + qadd + rl->max_level[last][run] * qmul;
        if (gb_get1(gb)) level = -level;
      }
    } else {
      run = rl->run[sym];
      level = rl->level[sym];
      last = sym >= rl->last;
      i += run + 1 + (last ? 192 : 0);
      level = level * qmul + qadd;
      if (gb_get1(gb)) level = -level;
    }
    if (i > 62) {
      i -= 192;
      if (i & ~63) return -1;
      block[scan[i]] = (S16)level;
      break;
    }
    block[scan[i]] = (S16)level;
  }
not_coded:
  if (intra) {
    if (!use_intra_dc_vlc) {
      block[0] = (S16)get_level_dc(d, n, pred, block[0]);
      i -= i >> 31;
    }
    pred_ac(d, block, n, dc_pred_dir);
    if (d->ac_pred) i = 63;
  }
  d->last_index[n] = i;
  return 0;
}

static int is_resync(Dec* d) {  // mpeg4_is_resync
  GB* gb = &d->gb;
  int bits_count = gb->pos;
  int v = (int)gb_show(gb, 16);
  while (v <= 0xFF) {
    if (d->pict_type == PT_B || (v >> (8 - d->pict_type) != 1)) break;
    gb_skip(gb, 8 + d->pict_type);
    bits_count += 8 + d->pict_type;
    v = (int)gb_show(gb, 16);
  }
  if (bits_count + 8 >= gb->size) {
    v >>= 8;
    v |= 0x7F >> (7 - (bits_count & 7));
    if (v == 0x7F) return d->mb_num;
  } else {
    static const U16 resync_prefix[8] = { 0x7F00, 0x7E00, 0x7C00, 0x7800, 0x7000, 0x6000, 0x4000, 0x0000 };
    if (v == resync_prefix[bits_count & 7]) {
      int len, mb_num, mb_num_bits = ilog2(d->mb_num - 1) + 1;
      GB bak = *gb;
      gb_skip(gb, 1);
      gb_align(gb);
      for (len = 0; len < 32; len++)
        if (gb_get1(gb)) break;
      mb_num = (int)gb_get(gb, mb_num_bits);
      if (!mb_num || mb_num > d->mb_num || gb->pos + 6 > gb->size) mb_num = -1;
      *gb = bak;
      if (len >= video_packet_prefix_length(d->pict_type, d->f_code, d->b_code)) return mb_num;
    }
  }
  return 0;
}

static int decode_mb(Dec* d) {
  static const S8 quant_tab[4] = { -1, -2, 1, 2 };
  int cbpc = 0, cbpy, i, cbp, pred_x, pred_y, mx, my, dquant;
  const int xy = d->mb_x + d->mb_y * d->mb_stride;
  GB* gb = &d->gb;
  Pic* cur = d->cur;
  if (d->pict_type == PT_P || d->pict_type == PT_S) {
    do {
      if (gb_get1(gb)) {                // skipped MB
        d->mb_intra = 0;
        for (i = 0; i < 6; i++) d->last_index[i] = -1;
        d->mv_dir = MV_DIR_FORWARD;
        d->mv_type = MV_16X16;
        if (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE) {
          cur->mbtype[xy] = MBT_SKIP | MBT_GMC | MBT_16x16 | MBT_FWD;
          d->mcsel = 1;
          d->mv[0][0][0] = get_amv(d, 0);
          d->mv[0][0][1] = get_amv(d, 1);
          cur->mbskip[xy] = 0;
          d->mb_skipped = 0;
        } else {
          cur->mbtype[xy] = MBT_SKIP | MBT_16x16 | MBT_FWD;
          d->mcsel = 0;
          d->mv[0][0][0] = d->mv[0][0][1] = 0;
          cur->mbskip[xy] = 1;
          d->mb_skipped = 1;
        }
        goto end;
      }
      cbpc = vlc_get(gb, &vlc_inter_mcbpc);
      if (cbpc < 0) return -1;
    } while (cbpc == 20);
    dquant = cbpc & 8;
    d->mb_intra = (cbpc & 4) != 0;
    if (d->mb_intra) goto intra;
    memset(d->block, 0, sizeof(d->block));
    if (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE && (cbpc & 16) == 0)
      d->mcsel = gb_get1(gb);
    else
      d->mcsel = 0;
    cbpy = vlc_get(gb, &vlc_cbpy);
    if (cbpy < 0) return -1;
    cbpy ^= 0x0F;
    cbp = (cbpc & 3) | (cbpy << 2);
    if (dquant) set_qscale(d, d->qscale + quant_tab[gb_get(gb, 2)]);
    if (!d->progressive && (cbp || (d->bugs & BUG_XVID_ILACE))) d->interlaced_dct = gb_get1(gb);
    d->mv_dir = MV_DIR_FORWARD;
    if ((cbpc & 16) == 0) {
      if (d->mcsel) {
        cur->mbtype[xy] = MBT_GMC | MBT_16x16 | MBT_FWD;
        d->mv_type = MV_16X16;
        d->mv[0][0][0] = get_amv(d, 0);
        d->mv[0][0][1] = get_amv(d, 1);
      } else if (!d->progressive && gb_get1(gb)) {
        cur->mbtype[xy] = MBT_16x8 | MBT_FWD | MBT_INTERLACED;
        d->mv_type = MV_FIELD;
        d->field_select[0][0] = gb_get1(gb);
        d->field_select[0][1] = gb_get1(gb);
        pred_motion(d, 0, &pred_x, &pred_y);
        for (i = 0; i < 2; i++) {
          mx = decode_motion(d, pred_x, d->f_code);
          if (mx >= 0xffff) return -1;
          my = decode_motion(d, pred_y / 2, d->f_code);
          if (my >= 0xffff) return -1;
          d->mv[0][i][0] = mx;
          d->mv[0][i][1] = my;
        }
      } else {
        cur->mbtype[xy] = MBT_16x16 | MBT_FWD;
        d->mv_type = MV_16X16;
        pred_motion(d, 0, &pred_x, &pred_y);
        mx = decode_motion(d, pred_x, d->f_code);
        if (mx >= 0xffff) return -1;
        my = decode_motion(d, pred_y, d->f_code);
        if (my >= 0xffff) return -1;
        d->mv[0][0][0] = mx;
        d->mv[0][0][1] = my;
      }
    } else {
      cur->mbtype[xy] = MBT_8x8 | MBT_FWD;
      d->mv_type = MV_8X8;
      for (i = 0; i < 4; i++) {
        S16* mot_val = pred_motion(d, i, &pred_x, &pred_y);
        mx = decode_motion(d, pred_x, d->f_code);
        if (mx >= 0xffff) return -1;
        my = decode_motion(d, pred_y, d->f_code);
        if (my >= 0xffff) return -1;
        d->mv[0][i][0] = mx;
        d->mv[0][i][1] = my;
        mot_val[0] = (S16)mx;
        mot_val[1] = (S16)my;
      }
    }
  } else if (d->pict_type == PT_B) {
    int modb1, modb2, mb_type;
    d->mb_intra = 0;
    d->mcsel = 0;
    if (d->mb_x == 0) {
      for (i = 0; i < 2; i++)
        d->last_mv[i][0][0] = d->last_mv[i][0][1] = d->last_mv[i][1][0] = d->last_mv[i][1][1] = 0;
    }
    d->mb_skipped = d->next->mbskip[xy];
    if (d->mb_skipped) {
      for (i = 0; i < 6; i++) d->last_index[i] = -1;
      d->mv_dir = MV_DIR_FORWARD;
      d->mv_type = MV_16X16;
      d->mv[0][0][0] = d->mv[0][0][1] = d->mv[1][0][0] = d->mv[1][0][1] = 0;
      cur->mbtype[xy] = MBT_SKIP | MBT_16x16 | MBT_FWD;
      goto end;
    }
    modb1 = gb_get1(gb);
    if (modb1) {
      mb_type = MBT_DIRECT2 | MBT_SKIP | MBT_BIDIR;
      cbp = 0;
    } else {
      static const int mb_type_b_map[4] = {
        MBT_DIRECT2 | MBT_BIDIR, MBT_BIDIR | MBT_16x16, MBT_BWD | MBT_16x16, MBT_FWD | MBT_16x16 };
      int t;
      modb2 = gb_get1(gb);
      t = vlc_get(gb, &vlc_mb_type_b);
      if (t < 0) return -1;
      mb_type = mb_type_b_map[t];
      if (modb2) cbp = 0;
      else {
        memset(d->block, 0, sizeof(d->block));
        cbp = (int)gb_get(gb, 6);
      }
      if (!(mb_type & MBT_DIRECT2) && cbp) {
        if (gb_get1(gb)) set_qscale(d, d->qscale + gb_get1(gb) * 4 - 2);
      }
      if (!d->progressive) {
        if (cbp) d->interlaced_dct = gb_get1(gb);
        if (!(mb_type & MBT_DIRECT2) && gb_get1(gb)) {
          mb_type |= MBT_16x8 | MBT_INTERLACED;
          mb_type &= ~MBT_16x16;
          if (mb_type & MBT_FWD) { d->field_select[0][0] = gb_get1(gb); d->field_select[0][1] = gb_get1(gb); }
          if (mb_type & MBT_BWD) { d->field_select[1][0] = gb_get1(gb); d->field_select[1][1] = gb_get1(gb); }
        }
      }
      d->mv_dir = 0;
      if ((mb_type & (MBT_DIRECT2 | MBT_INTERLACED)) == 0) {
        d->mv_type = MV_16X16;
        if (mb_type & MBT_FWD) {
          d->mv_dir = MV_DIR_FORWARD;
          mx = decode_motion(d, d->last_mv[0][0][0], d->f_code);
          my = decode_motion(d, d->last_mv[0][0][1], d->f_code);
          d->last_mv[0][1][0] = d->last_mv[0][0][0] = d->mv[0][0][0] = mx;
          d->last_mv[0][1][1] = d->last_mv[0][0][1] = d->mv[0][0][1] = my;
        }
        if (mb_type & MBT_BWD) {
          d->mv_dir |= MV_DIR_BACKWARD;
          mx = decode_motion(d, d->last_mv[1][0][0], d->b_code);
          my = decode_motion(d, d->last_mv[1][0][1], d->b_code);
          d->last_mv[1][1][0] = d->last_mv[1][0][0] = d->mv[1][0][0] = mx;
          d->last_mv[1][1][1] = d->last_mv[1][0][1] = d->mv[1][0][1] = my;
        }
      } else if (!(mb_type & MBT_DIRECT2)) {
        d->mv_type = MV_FIELD;
        if (mb_type & MBT_FWD) {
          d->mv_dir = MV_DIR_FORWARD;
          for (i = 0; i < 2; i++) {
            mx = decode_motion(d, d->last_mv[0][i][0], d->f_code);
            my = decode_motion(d, d->last_mv[0][i][1] / 2, d->f_code);
            d->last_mv[0][i][0] = d->mv[0][i][0] = mx;
            d->last_mv[0][i][1] = (d->mv[0][i][1] = my) * 2;
          }
        }
        if (mb_type & MBT_BWD) {
          d->mv_dir |= MV_DIR_BACKWARD;
          for (i = 0; i < 2; i++) {
            mx = decode_motion(d, d->last_mv[1][i][0], d->b_code);
            my = decode_motion(d, d->last_mv[1][i][1] / 2, d->b_code);
            d->last_mv[1][i][0] = d->mv[1][i][0] = mx;
            d->last_mv[1][i][1] = (d->mv[1][i][1] = my) * 2;
          }
        }
      }
    }
    if (mb_type & MBT_DIRECT2) {
      if (mb_type & MBT_SKIP) mx = my = 0;
      else {
        mx = decode_motion(d, 0, 1);
        my = decode_motion(d, 0, 1);
      }
      d->mv_dir = MV_DIR_FORWARD | MV_DIR_BACKWARD | MV_DIRECT;
      mb_type |= set_direct_mv(d, mx, my);
    }
    cur->mbtype[xy] = (U16)mb_type;
  } else {                              // I-VOP
    int use_intra_dc_vlc;
    do {
      cbpc = vlc_get(gb, &vlc_intra_mcbpc);
      if (cbpc < 0) return -1;
    } while (cbpc == 8);
    dquant = cbpc & 4;
    d->mb_intra = 1;
intra:
    d->ac_pred = gb_get1(gb);
    cur->mbtype[xy] = d->ac_pred ? (MBT_INTRA | MBT_ACPRED) : MBT_INTRA;
    cbpy = vlc_get(gb, &vlc_cbpy);
    if (cbpy < 0) return -1;
    cbp = (cbpc & 3) | (cbpy << 2);
    use_intra_dc_vlc = d->qscale < d->intra_dc_threshold;
    if (dquant) set_qscale(d, d->qscale + quant_tab[gb_get(gb, 2)]);
    if (!d->progressive) d->interlaced_dct = gb_get1(gb);
    memset(d->block, 0, sizeof(d->block));
    for (i = 0; i < 6; i++) {
      if (decode_block(d, d->block[i], i, cbp & 32, 1, use_intra_dc_vlc) < 0) return -1;
      cbp += cbp;
    }
    goto end;
  }
  for (i = 0; i < 6; i++) {
    if (decode_block(d, d->block[i], i, cbp & 32, 0, 0) < 0) return -1;
    cbp += cbp;
  }
end:
  if (d->resync_marker) {
    int next = is_resync(d);
    if (next) {
      if (d->mb_x + d->mb_y * d->mb_w + 1 >= next) return SLICE_END;
      if (d->pict_type == PT_B) {
        const int delta = d->mb_x + 1 == d->mb_w ? 2 : 1;
        if (d->next->mbskip[xy + delta]) return SLICE_OK;
      }
      return SLICE_END;
    }
  }
  return SLICE_OK;
}

//--- motion compensation -------------------------------------------------------------------

// Get a w x h block at (x, y) (rows y, y+ystep, ...) of a plane with coordinates clamped to the
// [0, ew-1] x [0, eh-1] reference area (equivalent to FFmpeg's emulated_edge_mc in every case).
// Blocks inside the area are returned in place (*stride = ps*ystep); others are built in buf (stride bs).
static const U8* fetch(U8* buf, int bs, const U8* plane, int ps, int x, int y, int ystep, int w, int h,
                       int ew, int eh, int* stride) {
  int i, j;
  if (x >= 0 && x + w <= ew && y >= 0 && y + (h - 1) * ystep < eh) {
    *stride = ps * ystep;
    return plane + (ptrdiff_t)y * ps + x;
  }
  for (i = 0; i < h; i++) {
    const U8* row = plane + (ptrdiff_t)clipi(y + i * ystep, 0, eh - 1) * ps;
    for (j = 0; j < w; j++) buf[i * bs + j] = row[clipi(x + j, 0, ew - 1)];
  }
  *stride = bs;
  return buf;
}

// FF_BUG_IEDGE (libavcodec 55.66-57.65 encoders): when field MC needs edge emulation, FFmpeg builds
// the Cr block in a scratch buffer overlapping the last row of the Cb one, so row 9 of the emulated
// Cb window (5th row of the bottom field) is Cr's first row. Returns the Cb block (stride 9).
static const U8* fetch_cb_iedge(U8* buf, const U8* u, const U8* v, int lc, int x, int y, int field_select,
                                int rows, int ew, int eh, int* stride) {
  int k, j;
  for (k = 0; k < rows; k++) {
    const U8* row = u + (ptrdiff_t)clipi(2 * y + field_select + 2 * k, 0, eh - 1) * lc;
    for (j = 0; j < 9; j++) buf[k * 9 + j] = row[clipi(x + j, 0, ew - 1)];
  }
  if (field_select && rows > 4) {
    const U8* row = v + (ptrdiff_t)clipi(2 * y, 0, eh - 1) * lc;
    for (j = 0; j < 9; j++) buf[4 * 9 + j] = row[clipi(x + j, 0, ew - 1)];
  }
  *stride = 9;
  return buf;
}

// half-pel interpolation: put (rounding), put_no_rnd, avg (FFmpeg hpeldsp semantics)
#define HPEL_LOOP(EXPR)                                                            \
  for (y = 0; y < h; y++, src += ss, dst += ds) {                                  \
    const U8* s = src;                                                             \
    if (op == OP_AVG) for (x = 0; x < w; x++) dst[x] = (U8)((dst[x] + (EXPR) + 1) >> 1); \
    else for (x = 0; x < w; x++) dst[x] = (U8)(EXPR);                              \
  }
static void hpel_mc(U8* dst, int ds, const U8* src, int ss, int w, int h, int dxy, int op) {
  const int r1 = op == OP_PUT_NORND ? 0 : 1, r2 = op == OP_PUT_NORND ? 1 : 2;
  int x, y;
  switch (dxy) {
  case 0:  HPEL_LOOP(s[x]); break;
  case 1:  HPEL_LOOP((s[x] + s[x + 1] + r1) >> 1); break;
  case 2:  HPEL_LOOP((s[x] + s[x + ss] + r1) >> 1); break;
  default: HPEL_LOOP((s[x] + s[x + 1] + s[x + ss] + s[x + ss + 1] + r2) >> 2); break;
  }
}
#undef HPEL_LOOP

// MPEG-4 quarter-pel (FFmpeg qpeldsp.c). n = 8 or 16. Filters mirror at the block edge.
static inline void qpel_store(U8* p, int v, int op) {
  if (op == OP_PUT) *p = clip8((v + 16) >> 5);
  else if (op == OP_PUT_NORND) *p = clip8((v + 15) >> 5);
  else *p = (U8)((*p + clip8((v + 16) >> 5) + 1) >> 1);
}

static void qpel_line(U8* dst, int dstep, const U8* src, int sstep, int n, int op) {
  int e[16 + 8], i;
  for (i = 0; i <= n; i++) e[3 + i] = src[i * sstep];
  e[2] = e[3]; e[1] = e[4]; e[0] = e[5];
  e[3 + n + 1] = e[3 + n]; e[3 + n + 2] = e[3 + n - 1]; e[3 + n + 3] = e[3 + n - 2];
  for (i = 0; i < n; i++) {
    const int* s = e + 3 + i;
    int v = (s[0] + s[1]) * 20 - (s[-1] + s[2]) * 6 + (s[-2] + s[3]) * 3 - (s[-3] + s[4]);
    qpel_store(dst + i * dstep, v, op);
  }
}

static void qpel_h(U8* dst, int ds, const U8* src, int ss, int n, int h, int op) {
  int y;
  for (y = 0; y < h; y++) qpel_line(dst + y * ds, 1, src + y * ss, 1, n, op);
}

static void qpel_v(U8* dst, int ds, const U8* src, int ss, int n, int op) {
  int x;
  for (x = 0; x < n; x++) qpel_line(dst + x, ds, src + x, ss, n, op);
}

static void avg2(U8* dst, int ds, const U8* a, int as, const U8* b, int bs, int n, int h, int op) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < n; x++) {
      int A = a[y * as + x], B = b[y * bs + x];
      U8* p = dst + y * ds + x;
      if (op == OP_PUT) *p = (U8)((A + B + 1) >> 1);
      else if (op == OP_PUT_NORND) *p = (U8)((A + B) >> 1);
      else *p = (U8)((*p + ((A + B + 1) >> 1) + 1) >> 1);
    }
}

static void avg4(U8* dst, int ds, const U8* a, int as, const U8* b, int bs, const U8* c, int cs,
                 const U8* e, int es, int n, int h, int op) {
  int x, y;
  for (y = 0; y < h; y++)
    for (x = 0; x < n; x++) {
      int s = a[y * as + x] + b[y * bs + x] + c[y * cs + x] + e[y * es + x];
      U8* p = dst + y * ds + x;
      if (op == OP_PUT) *p = (U8)((s + 2) >> 2);
      else if (op == OP_PUT_NORND) *p = (U8)((s + 1) >> 2);
      else *p = (U8)((*p + ((s + 2) >> 2) + 1) >> 1);
    }
}

// src: (n+1) x (n+1) samples with stride ss
static void qpel_mc(U8* dst, int ds, const U8* src, int ss, int n, int dxy, int op, int std_qpel_bug) {
  U8 halfH[17 * 16], halfV[16 * 16], halfHV[16 * 16], half[16 * 16];
  const int ri = op == OP_PUT_NORND ? OP_PUT_NORND : OP_PUT;   // intermediate rounding
  if (std_qpel_bug && (dxy == 5 || dxy == 7 || dxy == 9 || dxy == 11 || dxy == 13 || dxy == 15)) {
    // old lavc qpel (FF_BUG_STD_QPEL): qpel*_mc{11,31,13,33,12,32}_old_c
    const U8* full = src;
    const int fx = (dxy & 3) == 3 ? 1 : 0;
    qpel_h(halfH, n, full, ss, n, n + 1, ri);
    qpel_v(halfV, n, full + fx, ss, n, ri);
    qpel_v(halfHV, n, halfH, n, n, ri);
    if ((dxy >> 2) == 2) avg2(dst, ds, halfV, n, halfHV, n, n, n, op);     // mc12 / mc32
    else if ((dxy >> 2) == 1) avg4(dst, ds, full + fx, ss, halfH, n, halfV, n, halfHV, n, n, n, op);
    else avg4(dst, ds, full + ss + fx, ss, halfH + n, n, halfV, n, halfHV, n, n, n, op);
    return;
  }
  switch (dxy) {
  case 0:
    avg2(dst, ds, src, ss, src, ss, n, n, op == OP_AVG ? OP_AVG : OP_PUT);
    break;
  case 1:  qpel_h(half, n, src, ss, n, n, ri); avg2(dst, ds, src, ss, half, n, n, n, op); break;
  case 2:  qpel_h(dst, ds, src, ss, n, n, op); break;
  case 3:  qpel_h(half, n, src, ss, n, n, ri); avg2(dst, ds, src + 1, ss, half, n, n, n, op); break;
  case 4:  qpel_v(half, n, src, ss, n, ri); avg2(dst, ds, src, ss, half, n, n, n, op); break;
  case 8:  qpel_v(dst, ds, src, ss, n, op); break;
  case 12: qpel_v(half, n, src, ss, n, ri); avg2(dst, ds, src + ss, ss, half, n, n, n, op); break;
  case 5: case 7: case 13: case 15: {
    const int fx = (dxy & 3) == 3 ? 1 : 0, fy = (dxy >> 2) == 3 ? n : 0;
    qpel_h(halfH, n, src, ss, n, n + 1, ri);
    avg2(halfH, n, halfH, n, src + fx, ss, n, n + 1, ri);
    qpel_v(halfHV, n, halfH, n, n, ri);
    avg2(dst, ds, halfH + fy, n, halfHV, n, n, n, op);
    break;
  }
  case 6: case 14:
    qpel_h(halfH, n, src, ss, n, n + 1, ri);
    qpel_v(halfHV, n, halfH, n, n, ri);
    avg2(dst, ds, halfH + (dxy == 14 ? n : 0), n, halfHV, n, n, n, op);
    break;
  case 9: case 11:
    qpel_h(halfH, n, src, ss, n, n + 1, ri);
    avg2(halfH, n, halfH, n, src + (dxy == 11 ? 1 : 0), ss, n, n + 1, ri);
    qpel_v(dst, ds, halfH, n, n, op);
    break;
  case 10:
    qpel_h(halfH, n, src, ss, n, n + 1, ri);
    qpel_v(dst, ds, halfH, n, n, op);
    break;
  }
}

static void gmc1_dsp(U8* dst, int ds, const U8* src, int ss, int h, int x16, int y16, int rounder) {
  const int A = (16 - x16) * (16 - y16), B = x16 * (16 - y16), C = (16 - x16) * y16, D = x16 * y16;
  int i, j;
  for (i = 0; i < h; i++) {
    for (j = 0; j < 8; j++)
      dst[j] = (U8)((A * src[j] + B * src[j + 1] + C * src[ss + j] + D * src[ss + j + 1] + rounder) >> 8);
    dst += ds;
    src += ss;
  }
}

static void gmc_dsp(U8* dst, int stride, const U8* src, int h, int ox, int oy, int dxx, int dxy, int dyx,
                    int dyy, int shift, int r, int width, int height) {
  int y, vx, vy;
  const int s = 1 << shift;
  width--;
  height--;
  for (y = 0; y < h; y++) {
    int x;
    vx = ox;
    vy = oy;
    for (x = 0; x < 8; x++) {
      int index, src_x = vx >> 16, src_y = vy >> 16;
      int frac_x = src_x & (s - 1), frac_y = src_y & (s - 1);
      src_x >>= shift;
      src_y >>= shift;
      if ((unsigned)src_x < (unsigned)width) {
        if ((unsigned)src_y < (unsigned)height) {
          index = src_x + src_y * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_x) + src[index + 1] * frac_x) * (s - frac_y) +
                                      (src[index + stride] * (s - frac_x) + src[index + stride + 1] * frac_x) * frac_y +
                                      r) >> (shift * 2));
        } else {
          index = src_x + clipi(src_y, 0, height) * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_x) + src[index + 1] * frac_x) * s + r) >> (shift * 2));
        }
      } else {
        if ((unsigned)src_y < (unsigned)height) {
          index = clipi(src_x, 0, width) + src_y * stride;
          dst[y * stride + x] = (U8)(((src[index] * (s - frac_y) + src[index + stride] * frac_y) * s + r) >> (shift * 2));
        } else {
          index = clipi(src_x, 0, width) + clipi(src_y, 0, height) * stride;
          dst[y * stride + x] = src[index];
        }
      }
      vx += dxx;
      vy += dyx;
    }
    ox += dxy;
    oy += dyy;
  }
}

static void gmc1_motion(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, const Pic* ref) {
  U8 buf[17 * 17];
  const U8* src;
  int src_x, src_y, motion_x, motion_y, ss;
  motion_x = d->sprite_offset[0][0];
  motion_y = d->sprite_offset[0][1];
  src_x = d->mb_x * 16 + (motion_x >> (d->warp_acc + 1));
  src_y = d->mb_y * 16 + (motion_y >> (d->warp_acc + 1));
  motion_x *= 1 << (3 - d->warp_acc);
  motion_y *= 1 << (3 - d->warp_acc);
  src_x = clipi(src_x, -16, d->width);
  if (src_x == d->width) motion_x = 0;
  src_y = clipi(src_y, -16, d->height);
  if (src_y == d->height) motion_y = 0;
  src = fetch(buf, 17, ref->y, d->ly, src_x, src_y, 1, 17, 17, d->h_edge, d->v_edge, &ss);
  if ((motion_x | motion_y) & 7) {
    gmc1_dsp(dest_y, d->ly, src, ss, 16, motion_x & 15, motion_y & 15, 128 - d->no_rounding);
    gmc1_dsp(dest_y + 8, d->ly, src + 8, ss, 16, motion_x & 15, motion_y & 15, 128 - d->no_rounding);
  } else {
    int dxy = ((motion_x >> 3) & 1) | ((motion_y >> 2) & 2);
    hpel_mc(dest_y, d->ly, src, ss, 16, 16, dxy, d->no_rounding ? OP_PUT_NORND : OP_PUT);
  }
  motion_x = d->sprite_offset[1][0];
  motion_y = d->sprite_offset[1][1];
  src_x = d->mb_x * 8 + (motion_x >> (d->warp_acc + 1));
  src_y = d->mb_y * 8 + (motion_y >> (d->warp_acc + 1));
  motion_x *= 1 << (3 - d->warp_acc);
  motion_y *= 1 << (3 - d->warp_acc);
  src_x = clipi(src_x, -8, d->width >> 1);
  if (src_x == d->width >> 1) motion_x = 0;
  src_y = clipi(src_y, -8, d->height >> 1);
  if (src_y == d->height >> 1) motion_y = 0;
  src = fetch(buf, 9, ref->u, d->lc, src_x, src_y, 1, 9, 9, d->h_edge >> 1, d->v_edge >> 1, &ss);
  gmc1_dsp(dest_cb, d->lc, src, ss, 8, motion_x & 15, motion_y & 15, 128 - d->no_rounding);
  src = fetch(buf, 9, ref->v, d->lc, src_x, src_y, 1, 9, 9, d->h_edge >> 1, d->v_edge >> 1, &ss);
  gmc1_dsp(dest_cr, d->lc, src, ss, 8, motion_x & 15, motion_y & 15, 128 - d->no_rounding);
}

static void gmc_motion(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, const Pic* ref) {
  const int a = d->warp_acc;
  const int dl = d->ly, dc = d->lc;
  int ox, oy;
  // FFmpeg calls the gmc dsp with the destination stride for the source too (same linesize)
  ox = d->sprite_offset[0][0] + d->sprite_delta[0][0] * d->mb_x * 16 + d->sprite_delta[0][1] * d->mb_y * 16;
  oy = d->sprite_offset[0][1] + d->sprite_delta[1][0] * d->mb_x * 16 + d->sprite_delta[1][1] * d->mb_y * 16;
  gmc_dsp(dest_y, dl, ref->y, 16, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1],
          d->sprite_delta[1][0], d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding,
          d->h_edge, d->v_edge);
  gmc_dsp(dest_y + 8, dl, ref->y, 16, ox + d->sprite_delta[0][0] * 8, oy + d->sprite_delta[1][0] * 8,
          d->sprite_delta[0][0], d->sprite_delta[0][1], d->sprite_delta[1][0], d->sprite_delta[1][1],
          a + 1, (1 << (2 * a + 1)) - d->no_rounding, d->h_edge, d->v_edge);
  ox = d->sprite_offset[1][0] + d->sprite_delta[0][0] * d->mb_x * 8 + d->sprite_delta[0][1] * d->mb_y * 8;
  oy = d->sprite_offset[1][1] + d->sprite_delta[1][0] * d->mb_x * 8 + d->sprite_delta[1][1] * d->mb_y * 8;
  gmc_dsp(dest_cb, dc, ref->u, 8, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1],
          d->sprite_delta[1][0], d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding,
          (d->h_edge + 1) >> 1, (d->v_edge + 1) >> 1);
  gmc_dsp(dest_cr, dc, ref->v, 8, ox, oy, d->sprite_delta[0][0], d->sprite_delta[0][1],
          d->sprite_delta[1][0], d->sprite_delta[1][1], a + 1, (1 << (2 * a + 1)) - d->no_rounding,
          (d->h_edge + 1) >> 1, (d->v_edge + 1) >> 1);
}

// mpeg_motion_internal for H.263/MPEG-4 (frame or one field)
static void mpeg_motion(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field,
                        int field_select, const Pic* ref, int op, int motion_x, int motion_y, int h) {
  U8 buf[18 * 17];
  const U8* src;
  int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
  const int linesize = d->ly << field_based, uvlinesize = d->lc << field_based;
  dxy = ((motion_y & 1) << 1) | (motion_x & 1);
  src_x = d->mb_x * 16 + (motion_x >> 1);
  src_y = (d->mb_y << (4 - field_based)) + (motion_y >> 1);
  if ((d->bugs & BUG_HPEL_CHROMA) && field_based) {
    mx = (motion_x >> 1) | (motion_x & 1);
    my = motion_y >> 1;
    uvdxy = ((my & 1) << 1) | (mx & 1);
    uvsrc_x = d->mb_x * 8 + (mx >> 1);
    uvsrc_y = (d->mb_y << (3 - field_based)) + (my >> 1);
  } else {
    uvdxy = dxy | (motion_y & 2) | ((motion_x & 2) >> 1);
    uvsrc_x = src_x >> 1;
    uvsrc_y = src_y >> 1;
  }
  if (bottom_field) { dest_y += d->ly; dest_cb += d->lc; dest_cr += d->lc; }
  src = fetch(buf, 17, ref->y, d->ly, src_x, (src_y << field_based) + field_select, 1 << field_based, 17, h + 1,
              d->h_edge, d->v_edge, &ss);
  hpel_mc(dest_y, linesize, src, ss, 16, h, dxy, op);
  if (field_based && (d->bugs & BUG_IEDGE) &&
      ((unsigned)src_x >= (unsigned)(d->h_edge - (motion_x & 1) - 15 > 0 ? d->h_edge - (motion_x & 1) - 15 : 0) ||
       (unsigned)src_y >= (unsigned)((d->v_edge >> 1) - (motion_y & 1) - h + 1 > 0 ? (d->v_edge >> 1) - (motion_y & 1) - h + 1 : 0)))
    src = fetch_cb_iedge(buf, ref->u, ref->v, d->lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1,
                         d->h_edge >> 1, d->v_edge >> 1, &ss);
  else
    src = fetch(buf, 9, ref->u, d->lc, uvsrc_x, (uvsrc_y << field_based) + field_select, 1 << field_based, 9,
                (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cb, uvlinesize, src, ss, 8, h >> 1, uvdxy, op);
  src = fetch(buf, 9, ref->v, d->lc, uvsrc_x, (uvsrc_y << field_based) + field_select, 1 << field_based, 9,
              (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cr, uvlinesize, src, ss, 8, h >> 1, uvdxy, op);
}

static void qpel_motion(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int field_based, int bottom_field,
                        int field_select, const Pic* ref, int op, int motion_x, int motion_y, int h) {
  U8 buf[17 * 17];
  const U8* src;
  int dxy, uvdxy, mx, my, src_x, src_y, uvsrc_x, uvsrc_y, ss;
  const int linesize = d->ly << field_based, uvlinesize = d->lc << field_based;
  const int std_bug = (d->bugs & BUG_STD_QPEL) != 0;
  dxy = ((motion_y & 3) << 2) | (motion_x & 3);
  src_x = d->mb_x * 16 + (motion_x >> 2);
  src_y = d->mb_y * (16 >> field_based) + (motion_y >> 2);
  if (field_based) {
    mx = motion_x / 2;
    my = motion_y >> 1;
  } else if (d->bugs & BUG_QPEL_CHROMA2) {
    static const int rtab[8] = { 0, 0, 1, 1, 0, 0, 0, 1 };
    mx = (motion_x >> 1) + rtab[motion_x & 7];
    my = (motion_y >> 1) + rtab[motion_y & 7];
  } else if (d->bugs & BUG_QPEL_CHROMA) {
    mx = (motion_x >> 1) | (motion_x & 1);
    my = (motion_y >> 1) | (motion_y & 1);
  } else {
    mx = motion_x / 2;
    my = motion_y / 2;
  }
  mx = (mx >> 1) | (mx & 1);
  my = (my >> 1) | (my & 1);
  uvdxy = (mx & 1) | ((my & 1) << 1);
  mx >>= 1;
  my >>= 1;
  uvsrc_x = d->mb_x * 8 + mx;
  uvsrc_y = d->mb_y * (8 >> field_based) + my;
  if (!field_based) {
    src = fetch(buf, 17, ref->y, d->ly, src_x, src_y, 1, 17, 17, d->h_edge, d->v_edge, &ss);
    qpel_mc(dest_y, d->ly, src, ss, 16, dxy, op, std_bug);
  } else {
    if (bottom_field) { dest_y += d->ly; dest_cb += d->lc; dest_cr += d->lc; }
    src = fetch(buf, 17, ref->y, d->ly, src_x, (src_y << 1) + field_select, 2, 17, 9, d->h_edge, d->v_edge, &ss);
    qpel_mc(dest_y, linesize, src, ss, 8, dxy, op, std_bug);
    qpel_mc(dest_y + 8, linesize, src + 8, ss, 8, dxy, op, std_bug);
  }
  src = fetch(buf, 9, ref->v, d->lc, uvsrc_x, (uvsrc_y << field_based) + field_select, 1 << field_based, 9,
              (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cr, uvlinesize, src, ss, 8, h >> 1, uvdxy, op);
  if (field_based && (d->bugs & BUG_IEDGE) &&
      ((unsigned)src_x >= (unsigned)(d->h_edge - (motion_x & 3) - 15 > 0 ? d->h_edge - (motion_x & 3) - 15 : 0) ||
       (unsigned)src_y >= (unsigned)((d->v_edge >> 1) - (motion_y & 3) - h + 1 > 0 ? (d->v_edge >> 1) - (motion_y & 3) - h + 1 : 0)))
    src = fetch_cb_iedge(buf, ref->u, ref->v, d->lc, uvsrc_x, uvsrc_y, field_select, (h >> 1) + 1,
                         d->h_edge >> 1, d->v_edge >> 1, &ss);
  else
    src = fetch(buf, 9, ref->u, d->lc, uvsrc_x, (uvsrc_y << field_based) + field_select, 1 << field_based, 9,
                (h >> 1) + 1, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cb, uvlinesize, src, ss, 8, h >> 1, uvdxy, op);
}

static void chroma_4mv_motion(Dec* d, U8* dest_cb, U8* dest_cr, const Pic* ref, int op, int mx, int my) {
  U8 buf[9 * 9];
  const U8* src;
  int src_x, src_y, dxy, ss;
  mx = round_chroma(mx);
  my = round_chroma(my);
  dxy = ((my & 1) << 1) | (mx & 1);
  mx >>= 1;
  my >>= 1;
  src_x = d->mb_x * 8 + mx;
  src_y = d->mb_y * 8 + my;
  src_x = clipi(src_x, -8, d->width >> 1);
  if (src_x == (d->width >> 1)) dxy &= ~1;
  src_y = clipi(src_y, -8, d->height >> 1);
  if (src_y == (d->height >> 1)) dxy &= ~2;
  src = fetch(buf, 9, ref->u, d->lc, src_x, src_y, 1, 9, 9, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cb, d->lc, src, ss, 8, 8, dxy, op);
  src = fetch(buf, 9, ref->v, d->lc, src_x, src_y, 1, 9, 9, d->h_edge >> 1, d->v_edge >> 1, &ss);
  hpel_mc(dest_cr, d->lc, src, ss, 8, 8, dxy, op);
}

static void apply_8x8(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const Pic* ref, int op) {
  U8 buf[17 * 17];
  const U8* src;
  int dxy, mx = 0, my = 0, src_x, src_y, i, ss;
  for (i = 0; i < 4; i++) {
    const int motion_x = d->mv[dir][i][0], motion_y = d->mv[dir][i][1];
    U8* dest = dest_y + ((i & 1) * 8) + (i >> 1) * 8 * d->ly;
    if (d->qpel) {
      dxy = ((motion_y & 3) << 2) | (motion_x & 3);
      src_x = d->mb_x * 16 + (motion_x >> 2) + (i & 1) * 8;
      src_y = d->mb_y * 16 + (motion_y >> 2) + (i >> 1) * 8;
      src_x = clipi(src_x, -16, d->width);
      if (src_x == d->width) dxy &= ~3;
      src_y = clipi(src_y, -16, d->height);
      if (src_y == d->height) dxy &= ~12;
      src = fetch(buf, 9, ref->y, d->ly, src_x, src_y, 1, 9, 9, d->h_edge, d->v_edge, &ss);
      qpel_mc(dest, d->ly, src, ss, 8, dxy, op, (d->bugs & BUG_STD_QPEL) != 0);
      mx += motion_x / 2;
      my += motion_y / 2;
    } else {
      dxy = 0;
      src_x = d->mb_x * 16 + (i & 1) * 8 + (motion_x >> 1);
      src_y = d->mb_y * 16 + (i >> 1) * 8 + (motion_y >> 1);
      src_x = clipi(src_x, -16, d->width);
      if (src_x != d->width) dxy |= motion_x & 1;
      src_y = clipi(src_y, -16, d->height);
      if (src_y != d->height) dxy |= (motion_y & 1) << 1;
      src = fetch(buf, 9, ref->y, d->ly, src_x, src_y, 1, 9, 9, d->h_edge, d->v_edge, &ss);
      hpel_mc(dest, d->ly, src, ss, 8, 8, dxy, op);
      mx += motion_x;
      my += motion_y;
    }
  }
  chroma_4mv_motion(d, dest_cb, dest_cr, ref, op, mx, my);
}

static void mpv_motion(Dec* d, U8* dest_y, U8* dest_cb, U8* dest_cr, int dir, const Pic* ref, int op) {
  int i;
  switch (d->mv_type) {
  case MV_16X16:
    if (d->mcsel) {
      if (d->real_warp_points == 1) gmc1_motion(d, dest_y, dest_cb, dest_cr, ref);
      else gmc_motion(d, dest_y, dest_cb, dest_cr, ref);
    } else if (d->qpel)
      qpel_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, d->mv[dir][0][0], d->mv[dir][0][1], 16);
    else
      mpeg_motion(d, dest_y, dest_cb, dest_cr, 0, 0, 0, ref, op, d->mv[dir][0][0], d->mv[dir][0][1], 16);
    break;
  case MV_8X8:
    apply_8x8(d, dest_y, dest_cb, dest_cr, dir, ref, op);
    break;
  case MV_FIELD:
    for (i = 0; i < 2; i++) {
      if (d->qpel)
        qpel_motion(d, dest_y, dest_cb, dest_cr, 1, i, d->field_select[dir][i], ref, op,
                    d->mv[dir][i][0], d->mv[dir][i][1], 8);
      else
        mpeg_motion(d, dest_y, dest_cb, dest_cr, 1, i, d->field_select[dir][i], ref, op,
                    d->mv[dir][i][0], d->mv[dir][i][1], 8);
    }
    break;
  }
}

static void reconstruct_mb(Dec* d) {
  Pic* cur = d->cur;
  const int mb_xy = d->mb_y * d->mb_stride + d->mb_x;
  U8* dest_y = cur->y + (size_t)d->mb_y * 16 * d->ly + d->mb_x * 16;
  U8* dest_cb = cur->u + (size_t)d->mb_y * 8 * d->lc + d->mb_x * 8;
  U8* dest_cr = cur->v + (size_t)d->mb_y * 8 * d->lc + d->mb_x * 8;
  const int dct_linesize = d->ly << d->interlaced_dct;
  const int dct_offset = d->interlaced_dct ? d->ly : d->ly * 8;
  int i;
  d->qscale_table[mb_xy] = (S8)d->qscale;
  d->mb_done[mb_xy] = 1;
  if (!d->mb_intra) {
    int op = (!d->no_rounding || d->pict_type == PT_B) ? OP_PUT : OP_PUT_NORND;
    if (d->mv_dir & MV_DIR_FORWARD) {
      mpv_motion(d, dest_y, dest_cb, dest_cr, 0, d->last, op);
      op = OP_AVG;
    }
    if (d->mv_dir & MV_DIR_BACKWARD) mpv_motion(d, dest_y, dest_cb, dest_cr, 1, d->next, op);
    for (i = 0; i < 6; i++) {
      U8* dst;
      int ls;
      if (d->last_index[i] < 0) continue;
      if (i < 4) { dst = dest_y + (i & 1) * 8 + (i >> 1) * dct_offset; ls = dct_linesize; }
      else { dst = i == 4 ? dest_cb : dest_cr; ls = d->lc; }
      if (d->mpeg_quant) unquant_inter_mpeg(d, d->block[i], d->qscale);
      idct_add(d, dst, ls, d->block[i]);
    }
  } else {
    for (i = 0; i < 6; i++) {
      U8* dst;
      int ls;
      if (i < 4) { dst = dest_y + (i & 1) * 8 + (i >> 1) * dct_offset; ls = dct_linesize; }
      else { dst = i == 4 ? dest_cb : dest_cr; ls = d->lc; }
      unquant_intra(d, d->block[i], i, i < 4 ? d->qscale : d->chroma_qscale);
      idct_put(d, dst, ls, d->block[i]);
    }
  }
}

//--- picture / slice level -----------------------------------------------------------------

static int decode_slice(Dec* d) {
  int ret;
  d->first_slice_line = 1;
  d->resync_mb_x = d->mb_x;
  d->resync_mb_y = d->mb_y;
  set_qscale(d, d->qscale);
  for (; d->mb_y < d->mb_h; d->mb_y++) {
    for (; d->mb_x < d->mb_w; d->mb_x++) {
      const int mb_xy = d->mb_y * d->mb_stride + d->mb_x;
      set_block_index(d);
      if (d->resync_mb_x == d->mb_x && d->resync_mb_y + 1 == d->mb_y) d->first_slice_line = 0;
      d->mv_dir = MV_DIR_FORWARD;
      d->mv_type = MV_16X16;
      ret = decode_mb(d);
      if (!d->mb_intra) clean_intra_table_entries(d, mb_xy);
      else d->mbintra_table[mb_xy] = 1;
      if (d->pict_type != PT_B) update_motion_val(d);
      if (ret < 0) {
        if (ret == SLICE_END) {
          reconstruct_mb(d);
          if (++d->mb_x >= d->mb_w) { d->mb_x = 0; d->mb_y++; }
          return 0;
        }
        return -1;
      }
      reconstruct_mb(d);
    }
    d->mb_x = 0;
  }
  return 0;
}

static int decode_video_packet_header(Dec* d) {
  GB* gb = &d->gb;
  int mb_num_bits = ilog2(d->mb_num - 1) + 1, header_extension = 0, mb_num, len;
  if (gb->pos > gb->size - 20) return -1;
  for (len = 0; len < 32; len++)
    if (gb_get1(gb)) break;
  if (len != video_packet_prefix_length(d->pict_type, d->f_code, d->b_code)) return -1;
  if (d->shape != RECT_SHAPE) header_extension = gb_get1(gb);
  mb_num = (int)gb_get(gb, mb_num_bits);
  if (mb_num >= d->mb_num || !mb_num) return -1;
  d->mb_x = mb_num % d->mb_w;
  d->mb_y = mb_num / d->mb_w;
  if (d->shape != BIN_ONLY_SHAPE) {
    int qscale = (int)gb_get(gb, d->quant_precision);
    if (qscale) d->chroma_qscale = d->qscale = qscale;
  }
  if (d->shape == RECT_SHAPE) header_extension = gb_get1(gb);
  if (header_extension) {
    while (gb_get1(gb) != 0) if (gb_left(gb) <= 0) return -1;
    gb_skip(gb, 1);
    gb_skip(gb, d->time_inc_bits);
    gb_skip(gb, 1);
    gb_skip(gb, 2);
    if (d->shape != BIN_ONLY_SHAPE) {
      gb_skip(gb, 3);
      if (d->pict_type == PT_S && d->sprite_usage == GMC_SPRITE) {
        if (decode_sprite_trajectory(d) < 0) return -1;
      }
      if (d->pict_type != PT_I) gb_skip(gb, 3);
      if (d->pict_type == PT_B) gb_skip(gb, 3);
    }
  }
  if (d->new_pred) {
    int l = d->time_inc_bits + 3 < 15 ? d->time_inc_bits + 3 : 15;
    gb_skip(gb, l);
    if (gb_get1(gb)) gb_skip(gb, l);
    gb_skip(gb, 1);
  }
  return 0;
}

static int resync(Dec* d, GB* last_resync) {  // ff_h263_resync
  GB* gb = &d->gb;
  int left;
  gb_skip(gb, 1);
  gb_align(gb);
  if (gb_show(gb, 16) == 0) {
    GB bak = *gb;
    if (decode_video_packet_header(d) >= 0) return 0;
    *gb = bak;
  }
  *gb = *last_resync;
  gb_align(gb);
  for (left = gb_left(gb); left > 16 + 1 + 5 + 5; left -= 8) {
    if (gb_show(gb, 16) == 0) {
      GB bak = *gb;
      if (decode_video_packet_header(d) >= 0) return 0;
      *gb = bak;
    }
    gb_skip(gb, 8);
  }
  return -1;
}

static Pic* free_pic(Dec* d) {
  int i;
  for (i = 0; i < 3; i++)
    if (&d->pics[i] != d->last && &d->pics[i] != d->next) return &d->pics[i];
  return &d->pics[0];
}

static Pic* gray_pic(Dec* d) {
  Pic* p = &d->pics[3];
  size_t ysz = (size_t)d->ly * d->mb_h * 16, csz = (size_t)d->lc * d->mb_h * 8;
  memset(p->y, 0x80, ysz + 2 * csz);
  memset(p->mbskip, 0, d->mb_stride * d->mb_h + 2);
  memset(p->mbtype, 0, (d->mb_stride * d->mb_h + 2) * sizeof(U16));
  memset(p->mvbase, 0, (d->b8_stride * d->mb_h * 2 + 4) * sizeof(*p->mvbase));
  return p;
}

// decode one VOP whose header has been parsed; returns 0 ok, FRAME_SKIPPED, <0 error
static int decode_picture(Dec* d) {
  GB last_resync;
  int mb_array = d->mb_stride * d->mb_h, i;
  workaround_bugs(d);
  if (d->bugs & BUG_EDGE) { d->h_edge = d->width; d->v_edge = d->height; }
  else { d->h_edge = d->mb_w * 16; d->v_edge = d->mb_h * 16; }
  if (d->pict_type == PT_B && !d->last) return FRAME_SKIPPED;   // no forward reference yet
  if (d->pict_type != PT_B) {
    d->last = d->next;
    d->cur = free_pic(d);
    d->next = d->cur;
  } else
    d->cur = free_pic(d);
  if (!d->last && d->pict_type != PT_I) d->last = gray_pic(d);
  d->cur->type = d->pict_type;
  d->cur->time = d->time;
  memset(d->cur->mbskip, 0, mb_array + 2);
  memset(d->cur->mvbase, 0, (d->b8_stride * d->mb_h * 2 + 4) * sizeof(*d->cur->mvbase));
  memset(d->mb_done, 0, mb_array);
  d->mb_x = d->mb_y = 0;
  d->interlaced_dct = 0;
  last_resync = d->gb;
  if (decode_slice(d) < 0) d->errors++;
  while (d->mb_y < d->mb_h) {
    if (resync(d, &last_resync) < 0) break;
    last_resync = d->gb;
    clean_buffers(d);
    if (decode_slice(d) < 0) d->errors++;
  }
  // conceal undecoded macroblocks by copying the co-located forward reference block
  for (i = 0; i < d->mb_num; i++) {
    int mx = i % d->mb_w, my = i / d->mb_w, y;
    if (d->mb_done[my * d->mb_stride + mx]) continue;
    if (d->errors == 0) d->errors++;
    for (y = 0; y < 16; y++) {
      size_t o = (size_t)(my * 16 + y) * d->ly + mx * 16;
      if (d->last) memcpy(d->cur->y + o, d->last->y + o, 16); else memset(d->cur->y + o, 0x80, 16);
    }
    for (y = 0; y < 8; y++) {
      size_t o = (size_t)(my * 8 + y) * d->lc + mx * 8;
      if (d->last) { memcpy(d->cur->u + o, d->last->u + o, 8); memcpy(d->cur->v + o, d->last->v + o, 8); }
      else { memset(d->cur->u + o, 0x80, 8); memset(d->cur->v + o, 0x80, 8); }
    }
  }
  return 0;
}

static void dec_init(Dec* d) {
  memset(d, 0, sizeof(*d));
  tables_init();
  d->divx_version = d->divx_build = d->xvid_build = d->lavc_build = -1;
  d->bugs = BUG_AUTODETECT;
  d->time_inc_bits = 4;
  d->quant_precision = 5;
  d->low_delay = 0;
  d->intra_scan = zigzag_direct;
  d->intra_h_scan = alternate_horizontal_scan;
  d->intra_v_scan = alternate_vertical_scan;
}

// Feeds one unit (headers + one VOP). Returns 0 decoded, FRAME_SKIPPED for a not-coded VOP /
// skipped B-VOP, 1 if the unit contained no VOP, <0 on error.
static int dec_decode_unit(Dec* d, const U8* buf, int size) {
  int ret, i;
  d->gb.buf = buf;
  d->gb.size = size * 8;
  d->gb.pos = 0;
  ret = parse_picture_header(d);
  if (ret) return ret;
  if (d->width <= 0 || d->height <= 0) return -1;
  if (dec_alloc(d) < 0) { fprintf(stderr, "out of memory\n"); exit(7); }
  for (i = 0; i < 4; i++) d->block_wrap[i] = d->b8_stride;
  d->block_wrap[4] = d->block_wrap[5] = d->mb_stride;
  return decode_picture(d);
}

//--- threads (Win32 or pthreads) ----------------------------------------------------------

#ifdef _WIN32
typedef HANDLE Event;
typedef HANDLE Thread;
static Event ev_create(int initial) { return CreateEventA(0, FALSE, initial ? TRUE : FALSE, 0); }
static void ev_set(Event e) { SetEvent(e); }
static void ev_wait(Event e) { WaitForSingleObject(e, INFINITE); }
static void ev_free(Event e) { CloseHandle(e); }
static int ncpu(void) { SYSTEM_INFO si; GetSystemInfo(&si); return (int)si.dwNumberOfProcessors; }
#define THREAD_RET DWORD WINAPI
static int thread_start(Thread* t, LPTHREAD_START_ROUTINE fn, void* arg) { return (*t = CreateThread(0, 0, fn, arg, 0, 0)) != 0; }
static void thread_join(Thread t) { WaitForSingleObject(t, INFINITE); CloseHandle(t); }
#else
typedef struct EventS { pthread_mutex_t m; pthread_cond_t c; int s; } EventS;
typedef EventS* Event;
typedef pthread_t Thread;
static Event ev_create(int initial) {
  Event e = (Event)malloc(sizeof(EventS));
  if (!e) return 0;
  pthread_mutex_init(&e->m, 0);
  pthread_cond_init(&e->c, 0);
  e->s = initial;
  return e;
}
static void ev_set(Event e) { pthread_mutex_lock(&e->m); e->s = 1; pthread_cond_signal(&e->c); pthread_mutex_unlock(&e->m); }
static void ev_wait(Event e) {  // auto-reset
  pthread_mutex_lock(&e->m);
  while (!e->s) pthread_cond_wait(&e->c, &e->m);
  e->s = 0;
  pthread_mutex_unlock(&e->m);
}
static void ev_free(Event e) { pthread_mutex_destroy(&e->m); pthread_cond_destroy(&e->c); free(e); }
static int ncpu(void) { long n = sysconf(_SC_NPROCESSORS_ONLN); return n > 0 ? (int)n : 1; }
#define THREAD_RET void*
static int thread_start(Thread* t, void* (*fn)(void*), void* arg) { return pthread_create(t, 0, fn, arg) == 0; }
static void thread_join(Thread t) { pthread_join(t, 0); }
#endif

//--- YUV 4:2:0 -> RGB ------------------------------------------------------------------------

// Fixed-point (16.16) YCbCr->RGB coefficients: Y scale, Y offset, Cr->R, Cb->G, Cr->G, Cb->B
typedef struct CSC { int ycoef, yoff, rv, gu, gv, bu; } CSC;
static const CSC csc_601_tv = { 76309, 16, 104597, 25675, 53279, 132201 };
static const CSC csc_601_pc = { 65536, 0, 91881, 22554, 46802, 116130 };
static const CSC csc_709_tv = { 76309, 16, 117489, 13975, 34925, 138438 };
static const CSC csc_709_pc = { 65536, 0, 103206, 12276, 30679, 121609 };

// Chroma is upsampled bilinearly: horizontally co-sited with even luma columns (MPEG-4 "left"
// siting, as FFmpeg reports), vertically between luma rows; per field for interlaced VOLs.
static void yuv_to_rgb(U8* rgb, const U8* Y, const U8* U, const U8* V, int w, int h, int interlaced, const CSC* m) {
  const int cw = (w + 1) >> 1, ch = (h + 1) >> 1;
  int x, y;
  int* cu = (int*)malloc(sizeof(int) * cw * 2);
  int* cv = cu + cw;
  for (y = 0; y < h; y++) {
    int r0, r1;
    const U8 *u0, *u1, *v0, *v1, *yl = Y + (size_t)y * w;
    U8* o = rgb + (size_t)y * w * 3;
    if (!interlaced) {
      r0 = y >> 1;
      r1 = clipi((y & 1) ? r0 + 1 : r0 - 1, 0, ch - 1);
    } else {
      const int f = y & 1, yf = y >> 1, fc = (ch - f + 1) >> 1;
      int k0 = yf >> 1, k1 = (yf & 1) ? k0 + 1 : k0 - 1;
      k0 = clipi(k0, 0, fc - 1);
      k1 = clipi(k1, 0, fc - 1);
      r0 = 2 * k0 + f;
      r1 = 2 * k1 + f;
    }
    r0 = clipi(r0, 0, ch - 1);
    u0 = U + (size_t)r0 * cw; u1 = U + (size_t)r1 * cw;
    v0 = V + (size_t)r0 * cw; v1 = V + (size_t)r1 * cw;
    for (x = 0; x < cw; x++) { cu[x] = 3 * u0[x] + u1[x]; cv[x] = 3 * v0[x] + v1[x]; }
    for (x = 0; x < w; x++) {
      const int cx = x >> 1, cx1 = cx + 1 < cw ? cx + 1 : cw - 1;
      const int u = ((x & 1) ? cu[cx] + cu[cx1] : 2 * cu[cx]) - 1024;   // x8 scale
      const int v = ((x & 1) ? cv[cx] + cv[cx1] : 2 * cv[cx]) - 1024;
      const int yy = m->ycoef * (yl[x] - m->yoff) * 8 + (1 << 18);
      o[0] = clip8((yy + m->rv * v) >> 19);
      o[1] = clip8((yy - m->gu * u - m->gv * v) >> 19);
      o[2] = clip8((yy + m->bu * u) >> 19);
      o += 3;
    }
  }
  free(cu);
}

//--- PNG writer (from bik2dec) ---------------------------------------------------------------

#define IDAT_MAX (1u << 20)

typedef struct Worker {
  Thread th;
  Event go, idle;        // auto-reset events
  U32 w, h, bpp; int level;
  U8 *yuv;               // w*h + 2*cw*ch input planes
  int interlaced;
  const CSC* csc;
  U8* img;               // w*h*bpp
  U8* cand;              // 5 filtered rows, each rb+1
  U8* zero;              // rb zeros (row above row 0)
  U8* out;               // IDAT_MAX deflate output
  char* fn;
  volatile int quit;
  int errors;
} Worker;

static void put32be(U8* p, U32 v) { p[0] = (U8)(v >> 24); p[1] = (U8)(v >> 16); p[2] = (U8)(v >> 8); p[3] = (U8)v; }

static int png_chunk(FILE* f, const char* type, const U8* data, U32 len) {
  U8 hdr[8], c[4];
  U32 crc;
  put32be(hdr, len); memcpy(hdr + 4, type, 4);
  crc = crc32(0, hdr + 4, 4);
  if (len) crc = crc32(crc, data, len);
  put32be(c, crc);
  return fwrite(hdr, 1, 8, f) == 8 && (!len || fwrite(data, 1, len, f) == len) && fwrite(c, 1, 4, f) == 4;
}

static U8 paeth(int a, int b, int c) {
  int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
  return (U8)((pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c));
}

// adaptive per-row filter (min sum of |signed residual|), zlib deflate, IDAT chunks of IDAT_MAX
static int write_png(Worker* wk) {
  const U32 w = wk->w, h = wk->h, bpp = wk->bpp;
  const size_t rb = (size_t)w * bpp;
  static const U8 sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  U8 ihdr[13];
  z_stream zs;
  FILE* f;
  int ok, zr = Z_OK;
  U32 y;

  if (!(f = fopen(wk->fn, "wb"))) return 0;
  put32be(ihdr, w); put32be(ihdr + 4, h);
  ihdr[8] = 8; ihdr[9] = bpp == 4 ? 6 : 2; ihdr[10] = ihdr[11] = ihdr[12] = 0;
  ok = fwrite(sig, 1, 8, f) == 8 && png_chunk(f, "IHDR", ihdr, 13);

  memset(&zs, 0, sizeof(zs));
  if (ok && deflateInit2(&zs, wk->level, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY) != Z_OK) ok = 0;
  zs.next_out = wk->out; zs.avail_out = IDAT_MAX;

  for (y = 0; ok && y < h; y++) {
    const U8* cur = wk->img + y * rb;
    const U8* up = y ? cur - rb : wk->zero;
    U32 best = ~0u; int bi = 0, t;
    for (t = 0; t < 5; t++) {
      U8* d = wk->cand + t * (rb + 1);
      U32 s = 0; size_t i;
      *d++ = (U8)t;
      for (i = 0; i < rb; i++) {
        int a = i >= bpp ? cur[i - bpp] : 0, b = up[i], c = i >= bpp ? up[i - bpp] : 0, x = cur[i];
        U8 r;
        switch (t) {
          case 0:  r = (U8)x; break;
          case 1:  r = (U8)(x - a); break;
          case 2:  r = (U8)(x - b); break;
          case 3:  r = (U8)(x - ((a + b) >> 1)); break;
          default: r = (U8)(x - paeth(a, b, c)); break;
        }
        d[i] = r; s += (r < 128) ? r : 256 - r;
      }
      if (s < best) { best = s; bi = t; }
      if (!y && t == 1) break;  // row 0: up/paeth degenerate to none/sub
    }
    zs.next_in = wk->cand + bi * (rb + 1); zs.avail_in = (uInt)(rb + 1);
    {
      const int flush = y + 1 == h ? Z_FINISH : Z_NO_FLUSH;
      do {
        zr = deflate(&zs, flush);
        if (zr == Z_STREAM_ERROR) { ok = 0; break; }
        if (zs.avail_out == 0) {  // full chunk
          ok = png_chunk(f, "IDAT", wk->out, IDAT_MAX);
          zs.next_out = wk->out; zs.avail_out = IDAT_MAX;
        }
      } while (ok && (zs.avail_in || (flush == Z_FINISH && zr != Z_STREAM_END)));
    }
  }
  if (ok && zs.avail_out < IDAT_MAX) ok = png_chunk(f, "IDAT", wk->out, IDAT_MAX - zs.avail_out);
  deflateEnd(&zs);
  ok = ok && png_chunk(f, "IEND", 0, 0);
  ok = (fclose(f) == 0) && ok;
  return ok;
}

static THREAD_RET worker_proc(void* p) {
  Worker* wk = (Worker*)p;
  const size_t npix = (size_t)wk->w * wk->h, cpix = (size_t)((wk->w + 1) >> 1) * ((wk->h + 1) >> 1);
  for (;;) {
    ev_wait(wk->go);
    if (wk->quit) break;
    yuv_to_rgb(wk->img, wk->yuv, wk->yuv + npix, wk->yuv + npix + cpix, (int)wk->w, (int)wk->h, wk->interlaced, wk->csc);
    if (!write_png(wk)) { fprintf(stderr, "\nerror writing %s\n", wk->fn); wk->errors++; }
    ev_set(wk->idle);
  }
  return 0;
}

//--- frontend ---------------------------------------------------------------------------------

typedef struct Output {
  int yuv_mode, nw, digits, w, h, interlaced, rc;
  const CSC* csc;
  FILE* yuvf;
  Worker* wk;
  const char* pre;
  size_t plen;
  U32 frames;
} Output;

static void copy_planes(U8* dst, const Dec* d, const Pic* p) {  // crop to w x h, chroma (w+1)/2 x (h+1)/2
  const int w = d->width, h = d->height, cw = (w + 1) >> 1, ch = (h + 1) >> 1;
  int y;
  for (y = 0; y < h; y++) memcpy(dst + (size_t)y * w, p->y + (size_t)y * d->ly, w);
  dst += (size_t)w * h;
  for (y = 0; y < ch; y++) memcpy(dst + (size_t)y * cw, p->u + (size_t)y * d->lc, cw);
  dst += (size_t)cw * ch;
  for (y = 0; y < ch; y++) memcpy(dst + (size_t)y * cw, p->v + (size_t)y * d->lc, cw);
}

static int output_init(Output* o, const Dec* d) {
  const size_t npix = (size_t)d->width * d->height, ysz = npix + 2 * (size_t)((d->width + 1) >> 1) * ((d->height + 1) >> 1);
  int i;
  o->w = d->width;
  o->h = d->height;
  o->interlaced = !d->progressive;
  if (d->color_matrix == 1) o->csc = d->full_range ? &csc_709_pc : &csc_709_tv;
  else o->csc = d->full_range ? &csc_601_pc : &csc_601_tv;
  if (o->yuv_mode) {
    if (!(o->yuvf = fopen(o->pre, "wb"))) { fprintf(stderr, "can't create %s\n", o->pre); return 6; }
    return 0;
  }
  o->nw = ncpu() > 1 ? ncpu() - 1 : 1;
  if (o->nw > 32) o->nw = 32;
  o->wk = (Worker*)calloc(o->nw, sizeof(Worker));
  if (!o->wk) return 7;
  for (i = 0; i < o->nw; i++) {
    Worker* w = &o->wk[i];
    w->w = (U32)d->width; w->h = (U32)d->height; w->bpp = 3; w->level = 6;
    w->interlaced = o->interlaced; w->csc = o->csc;
    w->yuv  = (U8*)malloc(ysz);
    w->img  = (U8*)malloc(npix * 3);
    w->cand = (U8*)malloc(5 * ((size_t)d->width * 3 + 1));
    w->zero = (U8*)calloc((size_t)d->width * 3 + 1, 1);
    w->out  = (U8*)malloc(IDAT_MAX);
    w->fn   = (char*)malloc(o->plen + 32);
    w->go   = ev_create(0);
    w->idle = ev_create(1);
    if (!w->yuv || !w->img || !w->cand || !w->zero || !w->out || !w->fn || !w->go || !w->idle ||
        !thread_start(&w->th, worker_proc, w)) {
      fprintf(stderr, "out of memory/resources\n");
      return 7;
    }
  }
  {  // fail early on an unwritable prefix
    char* fn = (char*)malloc(o->plen + 32);
    FILE* t;
    sprintf(fn, "%s%0*u.png", o->pre, o->digits, 0u);
    if (!(t = fopen(fn, "wb"))) { fprintf(stderr, "can't create %s\n", fn); free(fn); return 6; }
    fclose(t);
    remove(fn);
    free(fn);
  }
  return 0;
}

static void output_frame(Output* o, const Dec* d, const Pic* p) {
  if (o->rc) return;
  if (d->width != o->w || d->height != o->h) {
    fprintf(stderr, "\nframe size changed (%dx%d -> %dx%d), stopping\n", o->w, o->h, d->width, d->height);
    o->rc = 8;
    return;
  }
  if (o->yuv_mode) {
    const size_t sz = (size_t)o->w * o->h + 2 * (size_t)((o->w + 1) >> 1) * ((o->h + 1) >> 1);
    U8* buf = (U8*)malloc(sz);
    copy_planes(buf, d, p);
    if (fwrite(buf, 1, sz, o->yuvf) != sz) { fprintf(stderr, "write error %s\n", o->pre); o->rc = 6; }
    free(buf);
  } else {
    Worker* w = &o->wk[o->frames % o->nw];
    ev_wait(w->idle);
    if (w->errors) o->rc = 6;
    copy_planes(w->yuv, d, p);
    sprintf(w->fn, "%s%0*u.png", o->pre, o->digits, o->frames);
    ev_set(w->go);
  }
  o->frames++;
}

static void output_close(Output* o) {
  int i;
  if (o->yuvf && fclose(o->yuvf) != 0) o->rc = 6;
  for (i = 0; o->wk && i < o->nw; i++) {  // drain and stop workers
    Worker* w = &o->wk[i];
    ev_wait(w->idle);
    if (w->errors) o->rc = 6;
    w->quit = 1;
    ev_set(w->go);
    thread_join(w->th);
    ev_free(w->go); ev_free(w->idle);
    free(w->yuv); free(w->img); free(w->cand); free(w->zero); free(w->out); free(w->fn);
  }
  free(o->wk);
}

int main(int argc, char** argv) {
  const char *in, *pre;
  FILE* f;
  U8 *data, *ubuf;
  S64 fsize;
  size_t nvop = 0, nb = 0, i, unit_start = 0, nunits = 0, cap = 1024, maxunit = 0, u;
  size_t *ustart, *uend;
  int repeat = 1, yuv_mode = 0, a = 1, rc = 0, delayed, out_ready = 0;
  U32 nvop_repeats = 0, placeholders = 0, skipped = 0;
  Dec* d;
  Output out;
  const Pic* pending = 0;

  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-yuv")) yuv_mode = 1;
    else if (!strcmp(argv[a], "-n")) repeat = 0;
    else break;
  }
  if (argc - a != 2) {
    fprintf(stderr, "m4vdec - MPEG-4 Part 2 (DivX/XviD) decoder, raw .m4v -> PNG frames\n"
                    "Usage: m4vdec [-yuv] [-n] input.m4v output_name_prefix\n"
                    "  writes <prefix>NNNNN.png per frame (display order)\n"
                    "  -yuv  write all frames to the file <prefix> as raw I420 instead\n"
                    "  -n    don't repeat the previous frame for not-coded VOPs (low-delay streams)\n");
    return 1;
  }
  in = argv[a]; pre = argv[a + 1];

  if (!(f = fopen(in, "rb"))) { fprintf(stderr, "can't open %s\n", in); return 2; }
#ifdef _WIN32
  _fseeki64(f, 0, SEEK_END); fsize = _ftelli64(f); _fseeki64(f, 0, SEEK_SET);
#else
  fseeko(f, 0, SEEK_END); fsize = ftello(f); fseeko(f, 0, SEEK_SET);
#endif
  if (fsize <= 0) { fprintf(stderr, "empty input\n"); fclose(f); return 2; }
  if ((U64)fsize > (size_t)-1 - 64 || !(data = (U8*)calloc((size_t)fsize + 64, 1))) {
    fprintf(stderr, "out of memory (input is %lld bytes)\n", (long long)fsize);
    return 7;
  }
  if (fread(data, 1, (size_t)fsize, f) != (size_t)fsize) { fprintf(stderr, "read error %s\n", in); return 2; }
  fclose(f);

  // split into units: everything up to and including one VOP (ends at the next start code)
  ustart = (size_t*)malloc(cap * sizeof(size_t));
  uend = (size_t*)malloc(cap * sizeof(size_t));
  if (!ustart || !uend) { fprintf(stderr, "out of memory\n"); return 7; }
  for (i = 0; i + 3 < (size_t)fsize; i++) {
    if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1 && data[i + 3] == 0xB6) {
      size_t j = i + 4;
      while (j + 2 < (size_t)fsize && !(data[j] == 0 && data[j + 1] == 0 && data[j + 2] == 1)) j++;
      if (j + 2 >= (size_t)fsize) j = (size_t)fsize;
      if (nunits == cap) {
        cap *= 2;
        ustart = (size_t*)realloc(ustart, cap * sizeof(size_t));
        uend = (size_t*)realloc(uend, cap * sizeof(size_t));
        if (!ustart || !uend) { fprintf(stderr, "out of memory\n"); return 7; }
      }
      ustart[nunits] = unit_start;
      uend[nunits] = j;
      if (j - unit_start > maxunit) maxunit = j - unit_start;
      nunits++;
      nvop++;
      if (i + 4 < (size_t)fsize && (data[i + 4] >> 6) == 2) nb++;
      unit_start = j;
      i = j - 1;
    }
  }
  if (!nvop) { fprintf(stderr, "%s: no MPEG-4 VOPs found\n", in); return 3; }
  delayed = nb > 0;  // B-VOPs present: output in display order with one reference of delay
  ubuf = (U8*)malloc(maxunit + 64);
  d = (Dec*)malloc(sizeof(Dec));
  if (!ubuf || !d) { fprintf(stderr, "out of memory\n"); return 7; }
  dec_init(d);
  memset(&out, 0, sizeof(out));
  out.yuv_mode = yuv_mode;
  out.pre = pre;
  out.plen = strlen(pre);
  out.digits = 5;
  for (i = nvop; i >= 100000; i /= 10) out.digits++;

  for (u = 0; u < nunits && !rc && !out.rc; u++) {
    const size_t len = uend[u] - ustart[u];
    int r;
    memcpy(ubuf, data + ustart[u], len);
    memset(ubuf + len, 0, 64);
    r = dec_decode_unit(d, ubuf, (int)len);
    if (r < 0) {
      if (d->unsupported) { fprintf(stderr, "\n%s: %s is not supported\n", in, d->unsupported); rc = 4; break; }
      fprintf(stderr, "\nVOP %u: decode error\n", (unsigned)u);
      continue;
    }
    if (r == 1) continue;
    if (!out_ready) {
      fprintf(stderr, "%s: %dx%d, vo_type %d, %s%s%s%s, %u VOPs (%u B)",
              in, d->width, d->height, d->vo_type, d->qpel ? "qpel" : "hpel",
              d->progressive ? "" : ", interlaced", d->mpeg_quant ? ", mpeg quant" : "",
              d->sprite_usage == GMC_SPRITE ? ", gmc" : "", (unsigned)nvop, (unsigned)nb);
      if (d->xvid_build >= 0) fprintf(stderr, ", XviD build %d", d->xvid_build);
      if (d->divx_version >= 0) fprintf(stderr, ", DivX %d build %d", d->divx_version, d->divx_build);
      if (d->lavc_build >= 0) fprintf(stderr, ", lavc build %d", d->lavc_build);
      if (d->divx_packed) fprintf(stderr, ", packed");
      fprintf(stderr, "\n");
      if ((rc = output_init(&out, d)) != 0) break;
      out_ready = 1;
    }
    if (r == FRAME_SKIPPED) {
      // not-coded VOP: in packed streams a placeholder carrying the time of the held-back
      // reference; otherwise "display the previous reference again" (repeat it)
      if (d->pict_type == PT_B || !d->next) { skipped++; continue; }
      if (delayed && pending && pending->time == d->time) { placeholders++; continue; }
      if (!repeat) { skipped++; continue; }
      if (pending) { output_frame(&out, d, pending); pending = 0; }
      output_frame(&out, d, d->next);
      nvop_repeats++;
      continue;
    }
    if (d->pict_type == PT_B || !delayed) output_frame(&out, d, d->cur);
    else {
      if (pending) output_frame(&out, d, pending);
      pending = d->cur;
    }
    if ((u & 31) == 31 || u + 1 == nunits) fprintf(stderr, "\rVOP %u/%u, %u frames", (unsigned)(u + 1), (unsigned)nunits, out.frames);
  }
  if (pending && out_ready) output_frame(&out, d, pending);
  if (!rc) fprintf(stderr, "\rVOP %u/%u, %u frames\n", (unsigned)u, (unsigned)nunits, out.frames);
  if (out_ready) output_close(&out);
  if (placeholders) fprintf(stderr, "%u packed-bitstream placeholder VOPs dropped\n", placeholders);
  if (nvop_repeats) fprintf(stderr, "%u not-coded VOPs output as repeated frames\n", nvop_repeats);
  if (skipped) fprintf(stderr, "%u VOPs skipped (not coded or missing references)\n", skipped);
  if (d->errors) fprintf(stderr, "%d frames had decoding errors (concealed)\n", d->errors);
  if (!rc) rc = out.rc;
  dec_free_buffers(d);
  free(d); free(ubuf); free(data); free(ustart); free(uend);
  return rc;
}
