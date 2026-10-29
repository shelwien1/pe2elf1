// m4vpack - lossless compressor for MPEG-4 Part 2 video elementary streams (.m4v: XviD, DivX 4/5/6,
// libavcodec mpeg4, ...)
// Usage: m4vpack c|d [options] input output
//   c    compress
//   d    decompress, restoring the original stream byte for byte
//   -v   (c) afterwards decompress the output and compare it with the input
//   -s   (c) input and output sizes per kind of syntax element
//   -q   no statistics
//   -t N (c) compress on N threads (1 to 64): the stream in parts, a model for each
//   -c N (c with -t) the size of a part, at least: N MB, or N KB with a k (default 4 MB)
// input and output can be - for stdin and stdout.
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
//     co-located (last anchor VOP) macroblocks, the run of skipped / direct macroblocks, and
//     whether the predicted vector is zero.
//   - Motion vector differences: zero flag, sign, magnitude in units of the VLC code (independent
//     of f_code) in unary, the f_code residual bits; contexts are the neighbours' magnitudes (as
//     CABAC's mvd), the other component, the co-located macroblock, the spread of the median's
//     candidates and the difference that would repeat the co-located vector. For these, the
//     vectors are reconstructed as the decoder does (median prediction, B-VOP predictors, direct
//     mode scaling by the display times).
//   - Coded block pattern: per 8x8 block, by the left/above blocks, the motion vector size, whether
//     the motion is sub-pel, the co-located macroblock, the blocks coded so far, the quantiser.
//   - Intra DC differences: size (by the previous and neighbouring blocks' sizes and counts), lower
//     bits (the top ones by the bits above them), sign (by the neighbours' and the previous one's).
//   - Coefficients as CABAC's residual: a significance map with "last" flags in scan order, then the
//     levels in reverse order (|level| > 1, unary, then size and bits), signs, and the escape mode
//     where it isn't the usual one. Contexts: position, the neighbouring blocks' counts (CAVLC's
//     nC), the coefficients coded so far, the left and above blocks' coefficients at the same scan
//     position, the frequency neighbours (as HEVC's template), quantiser, motion vector size (an
//     intra block's DC size), and for a Cr block the Cb block. As in h264p, histories: the
//     significance flags before (the last 8 by position, 16 hashed), the magnitudes coded before
//     and their sum. The signs go by the signs coded before them, those of the frequency
//     neighbours left and above (zigzag codes them first), weighted by their magnitudes, and the
//     signs of the neighbouring blocks at the same position: an edge leaves sign patterns.
// Each of these decisions mixes the predictions of several contexts (logistic mixing as in lpaq,
// two weight sets), coefficient decisions then pass an APM (SSE). Probabilities adapt with a rate
// of 1/n up to n = PLIMIT.
//
// Streaming. The compressor and the decompressor are coroutines (Lib3's, as h264p's): each takes
// its input and gives its output a buffer at a time, so they run on files, pipes or memory alike,
// and any number of them at once. Memory grows with the longest chunk and the largest picture.
//
// Threads (-t). The stream is cut into parts that start at a VOP header (VOS, VO, VOL) or an
// I-VOP, with the headers before them copied in, so that each part is a stream of its own; the
// parts are dealt in turn to N workers (MTwrap), each part coded by a model of its own.
//
// File format: "m4vpack" 03, then the arithmetic coded chunks: kind (end, VOP as records, bytes),
// then the records or a length and the bytes (order 0 and 1 contexts, mixed); after the end, the
// CRC-32 of the stream. On threads: "m4vpm" 03, the threads, the part size in KB (4 bytes), then
// MTwrap's container of the workers' streams, a record for each part (its size, where the headers
// copied in are, its place, a CRC), each part as a stream of the first kind.
//
// Kept as bytes: as in m4vtsv, VOPs using data partitioning (B-VOPs don't), non-rectangular
// shapes, static sprites, sprite brightness change, newpred, reduced resolution, scalability or the
// studio profile; VOPs that don't parse (damaged data); VOPs longer than 64 MiB or with more than
// 1 MiB after the last macroblock.
//
// Build (Lib3/ and MTwrap_v3/ next to this file):
//   x86_64-w64-mingw32-g++ -O3 -s -static -ILib3 -o m4vpack.exe m4vpack.cpp
//   g++ -O3 -ILib3 -pthread -o m4vpack m4vpack.cpp
// C++11 without exceptions or RTTI. Integer arithmetic only where it matters for decoding, so
// files are portable between platforms.

#ifndef _WIN32
#define _FILE_OFFSET_BITS 64
#endif
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// Lib3's stack-copying coroutines and its CoroutinePair, which chains two of them; then MTwrap,
// which runs coroutines on threads (its win_shim.inc: build with -ILib3). After the system headers,
// which Lib3's setjmp and longjmp must not reach.
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/coro2pair.inc"
static void no_memory(void) {  // (MTwrap's buffers)
  fprintf(stderr, "m4vpack: out of memory\n");
  exit(5);
}
#define MTWRAP_NOMEM() no_memory()
#include "MTwrap_v3/MTwrap.inc"

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
#ifndef IREAD
#define IREAD (1u << 20)         // input read size
#endif

#if defined(__GNUC__) || defined(__clang__)
#define PREFETCH(p) __builtin_prefetch(p)
#else
#define PREFETCH(p) ((void)0)
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
  p->acc = (p->acc << n) | (n < 32 ? v & ((1u << n) - 1) : v);
  for (p->n += n; p->n >= 8;) {
    p->n -= 8;
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
  int time_res;              // vop_time_increment_resolution
  int qpel;                  // quarter_sample
  // c: skip flags of the last anchor VOP and of the one being parsed (a B-VOP macroblock whose
  // co-located macroblock in the backward reference was skipped has no bits), skip_cap entries
  U8 *skip_ref, *skip_cur;
  int skip_cap;
  U8 hbuf[(32 << 10) + 64];  // a header chunk being parsed
} Ctx;

static void ctx_init(Ctx* c) {  // (the skip flags stay: ctx_quit() frees them)
  U8 *r = c->skip_ref, *u = c->skip_cur;
  const int cap = c->skip_cap;
  memset(c, 0, sizeof(*c));
  c->skip_ref = r;
  c->skip_cur = u;
  c->skip_cap = cap;
  c->divx_version = c->divx_build = c->xvid_build = -1;
  c->time_inc_bits = 4;
  c->quant_precision = 5;
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
  c->time_res = time_res;
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
  c->qpel = vo_ver_id != 1 ? gb_get1(gb) : 0;
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

// a chunk (the first 32 KiB of it) that isn't a VOP: VOL headers and user data matter
static void parse_header_chunk(Ctx* c, const U8* p, size_t n) {
  GB gb;
  if (n < 4 || p[0] || p[1] || p[2] != 1) return;
  if (n > (32 << 10)) n = 32 << 10;
  memcpy(c->hbuf, p, n);
  memset(c->hbuf + n, 0, 64);
  gb.buf = c->hbuf + 4;
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

static thread_local double pbits[3];  // statistics: bits of motion vectors, intra DCs, coefficients parsed

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
  if (c->skip_ref[idx]) { m->sk = 1; return 0; }
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

//--- streams: coroutines -----------------------------------------------------------------------

// m4vpack's compressor and decompressor are coroutines of Lib3 (Lib3/coro3b.inc), in its buffer
// protocol: the input comes in the buffers addinp() gives, a 0-length one with f_quit set at its
// end, and yield Y_IN asks for the next; the output goes into the buffer addout() gave, yield
// Y_OUT handing it over full; Y_END ends a run (its last output in the buffer), Y_FAIL after an
// error (the run's note says it). Y_PART is MTwrap's: a worker's part is done (threads, below).
enum { Y_END = 0, Y_IN = 1, Y_OUT = 2, Y_FAIL = 3, Y_PART = 4 };

// A coroutine's input and output as runs of bytes. Plain data: io_init() sets it up as the run
// starts (MTwrap's workers are never constructed).
struct CoIo : Coroutine {
  int eof;          // the input has ended
  U64 n_in, n_out;  // the bytes read and written
  U64 over;         // bytes asked for past the end of the input (getb(): the range decoder's)
  int code;         // the exit status an error stands for (4 bad data, 5 internal)
  char note[256];   // the error, a line

  void io_init(void) {
    eof = 0;
    n_in = n_out = over = 0;
    code = 0;
    note[0] = 0;
  }
  // an error (exit status c): its note, and c to return
  int fail(int c, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(note, sizeof note, fmt, ap);
    va_end(ap);
    code = c;
    return c;
  }
  // the next input buffer (0: the input has ended)
  int next_input(void) {
    if (eof) return 0;
    chkinp();
    if (inpptr >= inpend) eof = 1;  // a 0-length buffer: the end
    return !eof;
  }
  // up to n bytes of the input, fewer only at its end
  size_t read(U8* p, size_t n) {
    size_t k = 0;
    while (k < n) {
      size_t l;
      if (inpptr >= inpend && !next_input()) break;
      l = n - k < (size_t)(inpend - inpptr) ? n - k : (size_t)(inpend - inpptr);
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    n_in += k;
    return k;
  }
  // all n bytes into the output
  void write(const U8* p, size_t n) {
    while (n) {
      size_t l;
      if (outptr >= outend) chkout();
      l = n < (size_t)(outend - outptr) ? n : (size_t)(outend - outptr);
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
      n_out += l;
    }
  }
  INLINE int getb(void) {  // a byte of the input, -1 after its end
    if (inpptr >= inpend && !next_input()) {
      over++;
      return -1;
    }
    n_in++;
    return *inpptr++;
  }
  INLINE void putb(int c) {
    if (outptr >= outend) chkout();
    *outptr++ = (U8)c;
    n_out++;
  }
};

// The input cut into chunks at start codes, read into buf: grown as a chunk needs, to IBUF_SIZE
// bytes (and 64 of padding after them). A longer chunk comes in pieces, and so does one that
// would need more memory than there is.
typedef struct Reader {
  CoIo* io;
  U8* buf;
  size_t cap;      // buf's size, less the padding
  size_t lo, hi;   // buf[lo, hi): data not returned yet
  int eof, cont;   // cont: the last chunk didn't fit and continues
  U8* pad;         // the 64 bytes after the last chunk, zeroed while it is parsed
  U8 saved[64];
} Reader;

static int rd_init(Reader* r, CoIo* io) {  // 0: no memory
  memset(r, 0, sizeof(*r));
  r->io = io;
  r->cap = IREAD < IBUF_SIZE ? IREAD : IBUF_SIZE;
  r->buf = (U8*)malloc(r->cap + 64);
  return r->buf != 0;
}

static int rd_fill(Reader* r) {  // moves the data not returned yet to the buffer start and reads more
  size_t n, want;
  if (r->lo) { memmove(r->buf, r->buf + r->lo, r->hi - r->lo); r->hi -= r->lo; r->lo = 0; }
  if (r->eof) return 0;
  if (r->hi == r->cap && r->cap < IBUF_SIZE) {  // full: twice the room, if there is the memory
    const size_t c = r->cap < IBUF_SIZE / 2 ? 2 * r->cap : IBUF_SIZE;
    U8* b = (U8*)realloc(r->buf, c + 64);
    if (b) r->buf = b, r->cap = c;
  }
  if (r->hi == r->cap) return 0;
  want = r->cap - r->hi < IREAD ? r->cap - r->hi : IREAD;
  n = r->io->read(r->buf + r->hi, want);
  r->hi += n;
  if (n < want) r->eof = 1;
  return n != 0;
}

static int find_sc(const U8* b, size_t* pi, size_t end) {  // 00 00 01 in b[*pi, end); else *pi = where to resume
  size_t i = *pi;
  while (i + 3 <= end) {
    if (b[i + 2] > 1) i += 3;
    else if (!b[i + 2]) i++;
    else if (!b[i] && !b[i + 1]) { *pi = i; return 1; }
    else i += 3;
  }
  *pi = i;
  return 0;
}

// Next chunk: from a start code (or the stream start) up to the next start code, in
// buf[*pos, *pos + *len), followed by 64 zero bytes until the next call. *part: 0 whole chunk,
// 1 first piece of a chunk longer than the buffer, 2 a later piece. 0 at the end of the input.
static int rd_chunk(Reader* r, size_t* pos, size_t* len, int* part) {
  size_t i;
  int found;
  if (r->pad) { memcpy(r->pad, r->saved, 64); r->pad = 0; }
  if (r->lo == r->hi && !rd_fill(r)) return 0;
  *part = r->cont ? 2 : 0;
  for (i = r->lo + !r->cont;;) {
    if ((found = find_sc(r->buf, &i, r->hi))) {
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
  r->pad = r->buf + i;
  memcpy(r->saved, r->pad, 64);
  memset(r->pad, 0, 64);
  return 1;
}

//--- CRC-32 -------------------------------------------------------------------------------------

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

//--- binary arithmetic coder (carryless 32-bit range coder as in lpaq) ----------------------------

enum { CAT_RAW, CAT_HDR, CAT_MB, CAT_CBP, CAT_MV, CAT_DC, CAT_SIG, CAT_LEV, CAT_SIGN, CAT_ESC, NCAT };
static const char* cat_name[NCAT] = { "raw chunks", "VOP and video packet headers, tails", "types, flags, dquant",
                                      "coded block patterns", "motion vectors", "intra DC", "positions",
                                      "levels", "signs", "escape modes" };

typedef struct Coder {
  int enc;
  U32 x1, x2, x;
  CoIo* io;           // where its bytes go (enc) or come from (dec)
  int cat;            // statistics: category of the bits coded now
  double cost[NCAT];  // bits spent per category (enc)
  double orig[NCAT];  // bits they took in the input (CAT_MB includes CAT_CBP, CAT_SIG all coefficient bits)
  int stats;
} Coder;

static INLINE void co_put(Coder* c, int b) { c->io->putb(b); }
static INLINE int co_get(Coder* c) {  // after the end of the input: 255, which continues the flushed lower bound
  const int v = c->io->getb();
  return v < 0 ? 255 : v;
}
static void co_init(Coder* c, CoIo* io, int enc) {
  int i;
  memset(c, 0, sizeof(*c));
  c->enc = enc;
  c->io = io;
  c->x2 = 0xFFFFFFFF;
  if (!enc) for (i = 0; i < 4; i++) c->x = (c->x << 8) | (U32)co_get(c);
}
static void co_flush(Coder* c) { co_put(c, (int)(c->x1 >> 24)); }

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
#define MAXIN 11
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

// the tables the coder and the mixers read: made once as the program starts, then only read (by
// every run at once, on threads)
static void coder_tables_init(void) {
  static const int t[33] = { 1,    2,    3,    6,    10,   16,   27,   45,   73,   120,  194,
                             310,  488,  747,  1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785,
                             3901, 3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094 };
  int i, d, pi = 0, j;
  for (i = 0; i <= PLIMIT; i++) rate[i] = (int)(65536.0 / (i + 1.5));
  bitcost[0] = 16;
  for (i = 1; i <= 4096; i++) bitcost[i] = (float)(-log((double)i / 4096) / log(2.0));
  // squash: interpolated from a table, stretch: its inverse (integers only, as in lpaq)
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

//--- context models -------------------------------------------------------------------------------

#ifndef LUN
#define LUN 10  // contexts for the unary part of levels
#endif
#ifndef LUNARY
#define LUNARY 16  // levels up to this one in unary
#endif
#define NMVK 6  // kinds of motion vectors: P, P field, B forward, B backward, B direct, P 4MV
#define NCLS 8  // coefficient block classes: inter, B inter, intra, intra with AC prediction; x luma/chroma
#define HTB 18  // the hashed tables of significance histories: 2^HTB entries
#define HBITS 16  // the significance flags they hash

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
  Prob st[2][EG_CTX], skip[2][6], skip2[2][5][6], skip3[2][11][3], skip4[2][2][3];
  Prob pintra[6], pintra2[5][3];
  Prob p4mv[6], p4mv2[5][3], mcsel[3], bskip[3], modb0[9], modb02[6][5], modb03[11][3], modb2[3];
  Prob btype[3][9], btype2[3][11], ac[2][3];
  Prob cbp1[3][6][9], cbp2[3][6][7], cbp3[3][6][3][6], cbp4[3][6][5], cbp5[3][6][32], cbp6[3][6][32];
  Prob cbp7[3][6][9][3];
  Prob dqz[3][2], dqv[3][4], bdqz[2], bdqs;
  Prob dt[2][3], fp[2][3], fs[4];
  // motion vector differences: [kind][component]
  Prob mvz[NMVK][2][6], mvz2[NMVK][2][7], mvz3[NMVK][2][7], mvz4[NMVK][2][7], mvz5[NMVK][2][7];
  Prob mvz6[NMVK][2][7][7], mvu6[NMVK][2][12][7][7];
  Prob mvu[NMVK][2][12][6], mvu2[NMVK][2][12][7], mvu3[NMVK][2][12][7], mvu4[NMVK][2][12][5];
  Prob mvu5[NMVK][2][12][7];
  Prob mvr[NMVK][2][6][3], mvs[NMVK][2][3][3], mvs2[NMVK][2][3][7];
  // blocks
  Prob dcs[2][8][9], dcs2[2][9][9][9], dcs3[2][2][3][9], dcs4[2][6][6][9], dcm[2][10][9], dcmt[2][10][8];
  Prob dcsign[2][2], dcsg2[2][3][3][3], dcsg3[2][10][3];
  Prob sig[NCLS][6][3][64], sig2[NCLS][8][64], sig3[NCLS][9][64], sig4[NCLS][9][3][64];
  Prob sig5[NCLS][6][64], sig6[NCLS][4][64], sig7[NCLS][16][64], sig8[NCLS][7][64];
  Prob sig9[NCLS][256][64], sigp[NCLS][32][64], sigx[NCLS][3][64];
  Prob last[NCLS][6][3][64], last2[NCLS][8][64], last3[NCLS][9][64], last4[NCLS][9][3][64];
  Prob last5[NCLS][6][64], last6[NCLS][16][64], last7[NCLS][7][64], last8[NCLS][6][6][6], last9[NCLS][256][64];
  Prob lev1[NCLS][5][6][3], lev1b[NCLS][5][6], lev1c[NCLS][5][8], lev1d[NCLS][4][6][3];
  Prob lev1e[NCLS][5][6], lev1f[NCLS][5][6], lev1g[NCLS][16][6], lev1q[NCLS][8][6], levh[NCLS][64][64], levf[NCLS][16][64];
  Prob levs[NCLS][8][6];
  Prob levn[NCLS][5][3][LUN], levnb[NCLS][5][LUN], levnc[NCLS][5][LUN], levnd[NCLS][6][LUN];
  Prob levne[NCLS][5][LUN], levnq[NCLS][8][LUN], levnh[NCLS][64][LUN], levns[NCLS][8][LUN];
  Prob lexp[NCLS][3][12], lexp2[NCLS][5][12], lexp3[NCLS][6][12], lman[NCLS][13][12];
  Prob sign[NCLS][2], sign2[NCLS][9][6], sgnh[NCLS][27][6], sgnh2[NCLS][64][2], sgnf[NCLS][9][64];
  Prob sgnf2[NCLS][9][9], sgnv[NCLS][15][6], sgnd[NCLS][27][6], sgnx[NCLS][3][64], sgnm[NCLS][25][6];
  Prob escd[4], escm[4][4];
  Prob sigh[1 << HTB], lasth[1 << HTB];  // the significance flags before, hashed with the class and position
} Model;

typedef struct Last {  // previous values the models predict from
  int kind, rawprev;
  int type, tib, tinc, dinc[5], rnd, dcthr, tff, alt, quant[5], fcode[5], bcode;
  S64 time_base, last_time_base, last_non_b;  // display times as FFmpeg computes them
  int pp_time, pb_time;                       // anchor to anchor, anchor to B-VOP
} Last;


typedef struct Mixers {  // weight sets: a bias weight, then MAXIN input weights
  int sig[NCLS][64][MAXIN + 1], last[NCLS][64][MAXIN + 1], lev1[NCLS][6][5][MAXIN + 1];
  int sigb[NCLS][8][3][MAXIN + 1], lastb[NCLS][8][3][MAXIN + 1], lev1b[NCLS][5][3][MAXIN + 1];
  int levn[NCLS][LUN][5][MAXIN + 1], lexp[NCLS][12][MAXIN + 1], sign[NCLS][6][MAXIN + 1];
  int raw[8][MAXIN + 1], mbt[8][MAXIN + 1], cbp[3][6][MAXIN + 1], dcs[2][9][MAXIN + 1], dcsg[2][3][MAXIN + 1];
  int levnb[NCLS][LUN][8][MAXIN + 1];
  int mvz[NMVK][2][MAXIN + 1], mvu[NMVK][2][12][MAXIN + 1], mvs[NMVK][2][MAXIN + 1];
  int sign2[NCLS][15][MAXIN + 1], cbpb[3][6][9][MAXIN + 1], mvzb[NMVK][2][7][MAXIN + 1];
  int mvub[NMVK][2][12][7][MAXIN + 1];
} Mixers;

typedef struct Apms { Apm sig[NCLS][64], last[NCLS][64], lev1[NCLS][6][4], sign[NCLS][15][6]; } Apms;

// Coefficient magnitudes of the blocks coded last in each column of macroblocks: the left and
// above neighbours of a block (a macroblock's blocks replace those of the one above it as they are
// coded).
// mag: |level| (at most 255), sgn: 1 for negative, suf: coefficients at i..63, last: position of the
// last one (-1 none), dcs: size of the intra DC difference (-1 none), dcsg: 1 for a negative one
typedef struct BlkCtx { U8 mag[64], sgn[64], suf[65]; S8 last, dcs, dcsg; U8 n; } BlkCtx;
#define MAX_MB_W 512

// macroblock state the contexts of later macroblocks use
enum { C_NONE, C_SKIP, C_INTER, C_4MV, C_INTRA, C_GMC, C_BDIR0, C_BDIR, C_BFWD, C_BBWD, C_BBI };
// mva: magnitudes of the first motion vector difference in VLC code units, mvs: its signs (0, 1 +, 2 -);
// bsk: B-VOPs skip the macroblock at this position
typedef struct MBCtx { U8 cls, cbp, nz[6], mva[2], mvs[2], ac, dt, fp, bsk; } MBCtx;
typedef S16 MV4[4][2];  // a macroblock's motion vectors, per 8x8 block

typedef struct VState {  // the coding side of the current VOP
  Vop v;
  int mb_w, prev_dq, dc_prev[2], dc_sgn[2], run;  // dc_sgn: the last intra DC difference's sign (0 none, 1 +, 2 -)  // run: macroblocks of the same kind (skip, direct) before this one
  int vp_start;          // first macroblock of the video packet
  int lmv[2][2][2];      // B-VOP vector predictors: [forward, backward][field][x, y]
  int trb, trd;          // B-VOP: time from the past anchor, between the anchors
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

#define TAIL_MAX (1u << 20)  // the most bytes after a VOP's last macroblock it codes

enum { W_VERIFY, W_ENCODE, W_DECODE };  // what vop_walk() does

// The models and the state of a run: what codes and decodes the records
struct Codec {
  Model model;
  Last last;
  Mixers mixw;
  Apms apms;
  BlkCtx rowb[MAX_MB_W][6];
  // per macroblock, mb_cap of them: the current VOP's and the last anchor VOP's
  MBCtx *mbcur, *mbref;
  MV4 *mvcur, *mvref;
  int mb_cap;
  int ref_mbs;  // size of the VOP in mbref, 0 if none
  U8 tailbuf[TAIL_MAX];
  MB mrec;      // the macroblock vop_walk() has

  void Quit(void) {
    free(mbcur);
    free(mbref);
    free(mvcur);
    free(mvref);
    mbcur = mbref = 0;
    mvcur = mvref = 0;
    mb_cap = 0;
  }
  // room for n macroblocks (0: no memory)
  int mb_room(int n) {
    void* q;
    if (n <= mb_cap) return 1;
    if (!(q = realloc(mbcur, n * sizeof(MBCtx)))) return 0;
    mbcur = (MBCtx*)q;
    if (!(q = realloc(mbref, n * sizeof(MBCtx)))) return 0;
    mbref = (MBCtx*)q;
    if (!(q = realloc(mvcur, n * sizeof(MV4)))) return 0;
    mvcur = (MV4*)q;
    if (!(q = realloc(mvref, n * sizeof(MV4)))) return 0;
    mvref = (MV4*)q;
    memset(mbref + mb_cap, 0, (n - mb_cap) * sizeof(MBCtx));
    memset(mvref + mb_cap, 0, (n - mb_cap) * sizeof(MV4));
    mb_cap = n;
    return 1;
  }

  void rowb_clear(int x) {
    int i;
    memset(rowb[x], 0, sizeof(rowb[x]));
    for (i = 0; i < 6; i++) rowb[x][i].last = rowb[x][i].dcs = -1;
  }

void model_init(void) {
  Prob* p = (Prob*)&model;
  size_t i;
  for (i = 0; i < sizeof(model) / sizeof(Prob); i++) { p[i].p = 32768; p[i].n = 0; }
  memset(&last, 0, sizeof(last));
  last.tib = 1;
  for (i = 0; i < 5; i++) { last.quant[i] = 4; last.fcode[i] = 1; }
  last.bcode = 1;
  {
    int* w = (int*)&mixw;
    for (i = 0; i < sizeof(mixw) / sizeof(int); i++) w[i] = i % (MAXIN + 1) ? MIX_W0 : 0;
  }
  apm_init((Apm*)&apms, sizeof(apms) / sizeof(Apm));
  ref_mbs = 0;
}

int code_kind(Coder* c, int k) {  // 0 end, 1 VOP as records, 2 raw bytes
  Prob* t = model.kind[last.kind];
  c->cat = CAT_RAW;
  k = cbit(c, &t[0], k == 1) ? 1 : cbit(c, &t[1], k == 2) ? 2 : 0;
  return last.kind = k;
}

// a chunk kept as bytes: its length, then the bytes (order 0 and order 1 contexts, mixed), which
// may come a piece at a time
size_t code_raw_len(Coder* c, size_t n) {
  c->cat = CAT_RAW;
  return (size_t)code_eg(c, model.rawlen, 4, (int)n);
}
void code_raw_bytes(Coder* c, U8* p, size_t n) {
  size_t i;
  c->cat = CAT_RAW;
  for (i = 0; i < n; i++) {
    int x = 1, k;
    for (k = 7; k >= 0; k--) {
      Prob* in[2];
      in[0] = &model.raw0[x];
      in[1] = &model.raw[last.rawprev][x];
      x = x * 2 + mbit(c, in, 2, mixw.raw[k], (p[i] >> k) & 1);
    }
    last.rawprev = p[i] = (U8)(x - 256);
  }
}

// 32 bits at a probability of 1/2 each (the CRC)
U32 code_u32(Coder* c, U32 v) {
  U32 x = 0;
  int i;
  c->cat = CAT_HDR;
  for (i = 31; i >= 0; i--) x |= (U32)code_bit(c, 32768, (int)(v >> i) & 1) << i;
  return x;
}

void code_vop_hdr(Coder* c, const Ctx* cx, VopHdr* h) {
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
  {  // display times (FFmpeg's decode_vop_header) for the B-VOP vector contexts
    const S64 res = cx->time_res > 0 ? cx->time_res : 1;
    S64 t;
    if (h->type != PT_B) {
      last.last_time_base = last.time_base;
      last.time_base += h->mtb;
      t = last.time_base * res + h->tinc;
      t -= last.last_non_b;
      last.pp_time = (int)(t < -(1 << 30) ? -(1 << 30) : t > 1 << 30 ? 1 << 30 : t);
      last.last_non_b = last.time_base * res + h->tinc;
    } else {
      t = (last.last_time_base + h->mtb) * res + h->tinc;
      t = last.pp_time - (last.last_non_b - t);
      last.pb_time = (int)(t < -(1 << 30) ? -(1 << 30) : t > 1 << 30 ? 1 << 30 : t);
    }
  }
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

void code_vp(Coder* c, const Ctx* cx, VState* s, VpHdr* k, int expect) {
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
int code_tail(Coder* c, int nb, int* bits, int* n, U8* bytes, int max) {
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
// previous block's, the left and above blocks' (and their coefficient counts), whether the block
// has AC coefficients. The top bits below the leading one go by the bits above them, the sign by
// the left and above blocks' and the previous one's.
int code_dc(Coder* c, VState* s, int lc, int v, const BlkCtx* Lb, const BlkCtx* Ab, int coded,
                   int qctx) {
  Model* M = &model;
  const int ctx = s->dc_prev[lc] < 7 ? s->dc_prev[lc] : 7, a = v < 0 ? -v : v;
  const int ls = Lb && Lb->dcs >= 0 ? Lb->dcs : 9, as = Ab && Ab->dcs >= 0 ? Ab->dcs : 9;
  const int ls2 = Lb && Lb->dcs > 0 ? 1 + Lb->dcsg : 0, as2 = Ab && Ab->dcs > 0 ? 1 + Ab->dcsg : 0;
  Prob* in[MAXIN];
  int size = bitlen(a), k, x, node;
  for (k = 0; k < 9; k++) {
    in[0] = &M->dcs[lc][ctx][k];
    in[1] = &M->dcs2[lc][ls < 8 ? ls : 8][as < 8 ? as : 8][k];
    in[2] = &M->dcs3[lc][coded][qctx][k];
    in[3] = &M->dcs4[lc][Lb ? few_bucket(Lb->n) : 5][Ab ? few_bucket(Ab->n) : 5][k];
    if (!mbit(c, in, 4, mixw.dcs[lc][k], size > k)) break;
  }
  s->dc_prev[lc] = size = k;
  if (!size) return 0;
  x = 1 << (size - 1);
  for (k = size - 2, node = 1; k >= 0; k--) {  // the top bits by the bits above them, the rest by position
    const int bit = cbit(c, node < 8 ? &M->dcmt[lc][size][node] : &M->dcm[lc][size][k], (a >> k) & 1);
    x |= bit << k;
    node = node * 2 + bit;
  }
  in[0] = &M->dcsign[lc][size > 4];
  in[1] = &M->dcsg2[lc][ls2][as2][s->dc_sgn[lc]];
  in[2] = &M->dcsg3[lc][size][s->dc_sgn[lc]];
  k = mbit(c, in, 3, mixw.dcsg[lc][s->dc_sgn[lc]], v < 0);
  s->dc_sgn[lc] = 1 + k;
  return k ? -x : x;
}

// Coefficients of a block, coded like H.264's CABAC residual: significance map in scan order,
// then the levels in reverse order, signs, and the escape modes where they aren't the default.
// Each decision mixes contexts: the neighbouring blocks' coefficient counts (CAVLC's nC), the
// coefficients found so far, and the left and above blocks' coefficients at the same position.
// The significance and last flags also see the flags before them (the last 8, and 16 hashed), and
// the frequency neighbours left and above in the block; the levels, the magnitudes coded before
// them and the neighbours right and below; the signs, the signs coded before them and those of
// the frequency neighbours (a block's coefficients come in patterns, as an edge leaves them).
// cls: block class, nctx: neighbours' counts, qctx/q8: quantiser, mvc: motion vector size, rnz: the
// co-located block's count; Lb, Ab: left and above blocks or 0; out (which may be Ab) gets this one;
// Xb: for a Cr block, the Cb block of its macroblock (0 else)
void code_block(Coder* c, VState* s, Blk* b, int n, int intra, int dcvlc, int coded, int cls, int nctx,
                       int qctx, int q8, int mvc, int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx* out,
                       const BlkCtx* Xb) {
  Model* M = &model;
  const RL* rl = intra ? &rl_intra : &rl_inter;
  Prob* in[MAXIN];
  const U8* sc = s->scan;
  BlkCtx o;
  U8 rm[64];  // coefficients coded so far by raster position: 1 in the significance map, then magnitudes
  U8 nzr[64];  // the significance map by raster position
  int pos[64], lev[64], k = 0, i, j, start, prev;
  U32 sh = 0;  // the significance flags so far, the last in bit 0
  memset(&o, 0, sizeof(o));
  memset(rm, 0, sizeof(rm));
  o.last = o.dcs = -1;
  if (dcvlc) {
    c->cat = CAT_DC;
    b->dc = code_dc(c, s, n < 4 ? 0 : 1, b->dc, Lb, Ab, coded, qctx);
    o.dcs = (S8)bitlen(b->dc);
    o.dcsg = b->dc < 0;
  } else if (!c->enc)
    b->dc = 0;
  if (!coded) { b->n = 0; *out = o; return; }
  start = dcvlc ? 1 : 0;
  if (dcvlc) mvc = 7 + (o.dcs < 8 ? o.dcs : 8);  // (an intra block has no vector: the DC difference's size)
  if (c->enc)
    for (j = 0, prev = start - 1; j < b->n; j++) { pos[j] = prev += b->run[j] + 1; lev[j] = b->level[j]; }
  c->cat = CAT_SIG;
  for (i = start, prev = start - 1; i < 63; i++) {
    const int kb = k < 7 ? k : 7, pb = pos_bucket[i], run = i - prev - 1;
    const int nz = (Lb ? Lb->mag[i] != 0 : 2) * 3 + (Ab ? Ab->mag[i] != 0 : 2);
    const int past = (Lb ? i > Lb->last : 2) * 3 + (Ab ? i > Ab->last : 2);  // beyond their last coefficient
    const int hist = (int)(sh & 255);
    const U32 hh = (((U32)(cls * 64 + i) * 0x9E3779B1u) ^ ((sh & ((1u << HBITS) - 1)) * 0x85EBCA6Bu)) >> (32 - HTB);
    PREFETCH(&M->sigh[hh]);  // (a hashed table's rows are far apart: asked for early, they are in cache when read)
    PREFETCH(&M->lasth[hh]);
    const int r = sc[i], u = r & 7, v = r >> 3;
    // the frequency neighbours left, above and above left (their sum, and with those 2 away a pattern)
    const int fsum = (u ? rm[r - 1] : 0) + (v ? rm[r - 8] : 0) + (u && v ? rm[r - 9] : 0);
    const int fpat = (u ? rm[r - 1] : 0) | (v ? rm[r - 8] : 0) << 1 | (u && v ? rm[r - 9] : 0) << 2 |
                     (u > 1 ? rm[r - 2] : 0) << 3 | (v > 1 ? rm[r - 16] : 0) << 4;
    in[0] = &M->sig[cls][nctx][qctx][i];
    in[1] = &M->sig2[cls][kb][i];
    in[2] = &M->sig3[cls][nz][i];
    in[3] = &M->sig4[cls][past][qctx][i];
    in[4] = &M->sig5[cls][run < 3 ? run : run < 5 ? 3 : run < 9 ? 4 : 5][i];
    in[5] = &M->sig6[cls][fsum][i];
    in[6] = &M->sig7[cls][mvc][i];
    in[7] = Xb ? &M->sigx[cls][Xb->mag[i] < 2 ? Xb->mag[i] : 2][i] : &M->sig8[cls][rnz][i];
    in[8] = &M->sig9[cls][hist][i];
    in[9] = &M->sigh[hh];
    in[10] = &M->sigp[cls][fpat][i];
    if (!mbit2(c, in, 11, mixw.sig[cls][i], mixw.sigb[cls][kb][qctx], &apms.sig[cls][i],
               c->enc && k < b->n && pos[k] == i)) {
      sh <<= 1;
      continue;
    }
    sh = sh << 1 | 1;
    pos[k] = prev = i;
    rm[r] = 1;
    in[0] = &M->last[cls][nctx][qctx][i];
    in[1] = &M->last2[cls][kb][i];
    in[2] = &M->last3[cls][nz][i];
    in[3] = &M->last4[cls][past][qctx][i];
    in[4] = &M->last5[cls][run < 3 ? run : run < 5 ? 3 : run < 9 ? 4 : 5][i];
    in[5] = &M->last6[cls][mvc][i];
    in[6] = &M->last7[cls][rnz][i];
    in[7] = &M->last8[cls][Lb ? few_bucket(Lb->suf[i + 1]) : 5][Ab ? few_bucket(Ab->suf[i + 1]) : 5][pb];
    in[8] = &M->last9[cls][hist][i];
    in[9] = &M->lasth[hh];
    if (mbit2(c, in, 10, mixw.last[cls][i], mixw.lastb[cls][kb][qctx], &apms.last[cls][i], c->enc && k == b->n - 1)) {
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
    int ngt1 = 0, neq1 = 0, pm = 0, lh = 0, ssum = 0;  // lh: the magnitudes coded before, up to 3, 2 bits each; ssum: their sum
    const int kc = k < 4 ? k - 1 : k < 6 ? 3 : k < 10 ? 4 : k < 16 ? 5 : k < 24 ? 6 : 7;
    for (j = k - 1; j >= 0; j--) {
      const int a = c->enc ? (lev[j] < 0 ? -lev[j] : lev[j]) - 1 : 0, p = pos[j], pb = pos_bucket[p];
      const int nm = (Lb ? Lb->mag[p] : 0) + (Ab ? Ab->mag[p] : 0);
      const int nmb = nm < 3 ? nm : nm < 5 ? 3 : 4;
      const int g = ngt1 ? 0 : 1 + (neq1 < 3 ? neq1 : 3);
      const int ssb = !ssum ? 0 : ssum < 2 ? 1 : ssum < 4 ? 2 : ssum < 8 ? 3 : ssum < 16 ? 4 : ssum < 32 ? 5 : ssum < 64 ? 6 : 7;
      int t, nb2, den;
      in[0] = &M->lev1[cls][g][pb][qctx];
      in[1] = &M->lev1b[cls][nmb][pb];
      in[2] = &M->lev1c[cls][g][kc];
      in[3] = &M->lev1d[cls][pm][pb][qctx];
      {  // the frequency neighbours right, below and below right, coded already
        const int r = sc[p], u = r & 7, v = r >> 3;
        const int sum = (u < 7 ? rm[r + 1] : 0) + (v < 7 ? rm[r + 8] : 0) + (u < 7 && v < 7 ? rm[r + 9] : 0);
        const int ra = u < 7 ? (rm[r + 1] < 3 ? rm[r + 1] : 3) : 0, ba = v < 7 ? (rm[r + 8] < 3 ? rm[r + 8] : 3) : 0;
        den = (u < 7 ? nzr[r + 1] : 0) + (v < 7 ? nzr[r + 8] : 0) + (u ? nzr[r - 1] : 0) + (v ? nzr[r - 8] : 0);
        in[4] = &M->lev1e[cls][sum < 3 ? sum : sum < 5 ? 3 : 4][pb];
        in[5] = &M->lev1f[cls][den][pb];
        in[9] = &M->levf[cls][ra * 4 + ba][p];
        nb2 = sum < 3 ? sum : sum < 5 ? 3 : 4;
      }
      in[6] = &M->lev1g[cls][mvc][pb];
      in[7] = &M->lev1q[cls][q8][pb];
      in[8] = &M->levh[cls][lh][p];
      in[10] = &M->levs[cls][ssb][pb];
      t = mbit2(c, in, 11, mixw.lev1[cls][pb][g], mixw.lev1b[cls][g][qctx], &apms.lev1[cls][pb][g < 3 ? g : 3], a > 0);
      if (t) {  // |level| - 1 in unary up to LUNARY, then the size of a - LUNARY + 2 and its lower bits
        for (t = 1; t < LUNARY - 1; t++) {
          const int u = t <= LUN ? t - 1 : LUN - 1;
          in[0] = &M->levn[cls][ngt1 < 4 ? ngt1 : 4][qctx][u];
          in[1] = &M->levnb[cls][nmb][u];
          in[2] = &M->levnc[cls][nb2][u];
          in[3] = &M->levnd[cls][pb][u];
          in[4] = &M->levne[cls][den][u];
          in[5] = &M->levnq[cls][q8][u];
          in[6] = &M->levnh[cls][lh][u];
          in[7] = &M->levns[cls][ssb][u];
          if (!mbit2(c, in, 8, mixw.levn[cls][u][nb2], mixw.levnb[cls][u][ssb], 0, a > t)) break;
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
      lh = (lh * 4 + pm) & 63;
      ssum += lev[j];
      rm[sc[p]] = o.mag[p] = (U8)(lev[j] < 255 ? lev[j] : 255);
    }
  }
  o.last = (S8)pos[k - 1];
  o.n = (U8)k;
  for (i = 63; i >= 0; i--) o.suf[i] = (U8)(o.suf[i + 1] + (o.mag[i] != 0));
  c->cat = CAT_SIGN;
  {
    U8 srs[64];  // the signs coded so far by raster position: 0 none, 1 +, 2 -
    int sgh = 0;  // the last 3 signs (1 +, 2 -) in base 3
    memset(srs, 0, sizeof(srs));
    for (j = 0; j < k; j++) {
      const int p = pos[j], pb = pos_bucket[p], r = sc[p], u = r & 7, v = r >> 3;
      const int ls = Lb && Lb->mag[p] ? Lb->sgn[p] : 2, as = Ab && Ab->mag[p] ? Ab->sgn[p] : 2;
      // the frequency neighbours left, above (and 2 away), above left: coded before, in zigzag order
      const int fl = u ? srs[r - 1] : 0, fu = v ? srs[r - 8] : 0, fd = u && v ? srs[r - 9] : 0;
      const int fl2 = u > 1 ? srs[r - 2] : 0, fu2 = v > 1 ? srs[r - 16] : 0;
      int vote = 0;  // their signs, weighted by their magnitudes
      if (fl) vote += (fl == 1 ? 2 : -2) * (rm[r - 1] > 1 ? 2 : 1);
      if (fu) vote += (fu == 1 ? 2 : -2) * (rm[r - 8] > 1 ? 2 : 1);
      if (fd) vote += fd == 1 ? -1 : 1;
      vote = (vote < -7 ? -7 : vote > 7 ? 7 : vote) + 7;
      in[0] = &M->sign[cls][j == 0];
      in[1] = &M->sign2[cls][ls * 3 + as][pb];
      in[2] = &M->sgnh[cls][sgh][pb];
      in[3] = &M->sgnh2[cls][p][j && lev[j - 1] < 0];
      in[4] = &M->sgnf[cls][fl * 3 + fu][p];
      in[5] = &M->sgnf2[cls][fl * 3 + fu][fl2 * 3 + fu2];
      in[6] = &M->sgnv[cls][vote][pb];
      in[7] = &M->sgnd[cls][fd * 9 + fl * 3 + fu][pb];
      in[8] = &M->sgnx[cls][Xb && Xb->mag[p] ? 1 + Xb->sgn[p] : 0][p];
      in[9] = &M->sgnm[cls][(Lb && Lb->mag[p] ? 1 + Lb->sgn[p] * 2 + (Lb->mag[p] > 1) : 0) * 5 +
                            (Ab && Ab->mag[p] ? 1 + Ab->sgn[p] * 2 + (Ab->mag[p] > 1) : 0)][pb];
      if (mbit2(c, in, 10, mixw.sign[cls][pb], mixw.sign2[cls][vote], &apms.sign[cls][vote][pb],
                c->enc && b->level[j] < 0))
        lev[j] = -lev[j];
      o.sgn[p] = lev[j] < 0;
      srs[r] = (U8)(1 + (lev[j] < 0));
      sgh = sgh * 3 % 27 + 1 + (lev[j] < 0);
    }
  }
  *out = o;  // (after the signs: out may be Ab)
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
// macroblock or a neighbour), the size of the motion vector differences (mvc, 6 for none), which
// components of the vector are sub-pel (frac, 4 for none), the co-located macroblock of the last
// anchor VOP, the blocks coded so far, the quantiser (q8: before this macroblock's dquant, which
// comes after the pattern), and lq: whether the left and above macroblocks agree on having coded
// blocks (0), else 1 + whether the co-located one has some.
int code_cbp(Coder* c, int cls3, const MBCtx* L, const MBCtx* A, const MBCtx* R, int mvc, int frac,
                    int q8, int lq, int cbp) {
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
    in[3] = &M->cbp4[cls3][b][frac];
    in[4] = &M->cbp5[cls3][b][q8 * 4 + (n < 3 ? n : 3)];
    in[5] = &M->cbp6[cls3][b][v >> (6 - b) & 31];
    in[6] = &M->cbp7[cls3][b][l * 3 + u][lq];
    bit = mbit2(c, in, 7, mixw.cbp[cls3][b], mixw.cbpb[cls3][b][l * 3 + u], 0, (cbp >> (5 - b)) & 1);
    v |= bit << (5 - b);
    n += bit;
  }
  return v;
}

// neighbouring blocks' coefficient counts (CAVLC's nC)
int nz_ctx(const MBCtx* me, const MBCtx* L, const MBCtx* A, int b) {
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

int mv_bucket(int q) { return q < 3 ? q : q < 5 ? 3 : q < 9 ? 4 : 5; }

// A motion vector difference as coded, like CABAC's mvd: a zero flag, the sign, the magnitude in
// units of the VLC code (so f_code doesn't change its statistics) in unary, the f_code residual
// bits. Contexts: the neighbours' magnitudes (their sum, and each), the other component (oth: of
// this vector for y, of the previous one for x, 6 for none; it also picks a second weight set), the
// co-located macroblock of the last anchor VOP, and d: the difference that would give this vector
// the co-located one's value (dok 0: none).
int code_mvd(Coder* c, int kind, int comp, int fcode, const MBCtx* L, const MBCtx* A, const MBCtx* R,
                    int oth, int dok, int d, int sp, int v, int* q) {
  Model* M = &model;
  const int shift = fcode - 1, a = v < 0 ? -v : v;
  const int ns = mv_bucket((L ? L->mva[comp] : 0) + (A ? A->mva[comp] : 0));
  const int rs = R ? mv_bucket(R->mva[comp]) : 6;
  const int ad = (d < 0 ? -d : d) - 1, dq = d ? (ad >> shift) + 1 : 0, dz = dok ? mv_bucket(dq) : 6;
  const int ds = !dok || !d ? 0 : d > 0 ? 1 : 2;
  const int lq = L ? mv_bucket(L->mva[comp]) : 6, aq = A ? mv_bucket(A->mva[comp]) : 6;
  Prob* in[MAXIN];
  int k, x, neg, r, i;
  in[0] = &M->mvz[kind][comp][ns];
  in[1] = &M->mvz2[kind][comp][oth];
  in[2] = &M->mvz3[kind][comp][rs];
  in[3] = &M->mvz4[kind][comp][dz];
  in[4] = &M->mvz5[kind][comp][sp];
  in[5] = &M->mvz6[kind][comp][lq][aq];
  if (!mbit2(c, in, 6, mixw.mvz[kind][comp], mixw.mvzb[kind][comp][oth], 0, a != 0)) { *q = 0; return 0; }
  in[0] = &M->mvs[kind][comp][L ? L->mvs[comp] : 0][A ? A->mvs[comp] : 0];
  in[1] = &M->mvs2[kind][comp][ds][dz];
  neg = mbit(c, in, 2, mixw.mvs[kind][comp], v < 0);
  x = a ? ((a - 1) >> shift) + 1 : 0;
  for (k = 1; k < 32; k++) {
    const int u = k < 12 ? k - 1 : 11;
    in[0] = &M->mvu[kind][comp][u][ns];
    in[1] = &M->mvu2[kind][comp][u][oth];
    in[2] = &M->mvu3[kind][comp][u][rs];
    in[3] = &M->mvu4[kind][comp][u][!ds ? 0 : ds != 1 + neg ? 1 : k < dq ? 2 : k == dq ? 3 : 4];
    in[4] = &M->mvu5[kind][comp][u][sp];
    in[5] = &M->mvu6[kind][comp][u][lq][aq];
    if (!mbit2(c, in, 6, mixw.mvu[kind][comp][u], mixw.mvub[kind][comp][u][oth], 0, x > k)) break;
  }
  *q = k;
  for (i = shift - 1, r = 0; i >= 0; i--) {  // residual bits, predicted by d's while they agree so far
    const int same = ds == 1 + neg && k == dq && ad >> (i + 1) == ((k - 1) << (shift - i - 1) | r);
    r |= cbit(c, &M->mvr[kind][comp][i][same ? 1 + ((ad >> i) & 1) : 0], ((a - 1) >> i) & 1) << i;
  }
  x = ((k - 1) << shift | r) + 1;
  return neg ? -x : x;
}

int mv_wrap(int v, int fcode) { return sign_extend(v, 5 + fcode); }  // as the decoder adds them

// The decoder's prediction of the vector of block b (FFmpeg's h263_pred_motion): the median of the
// left, above and above-right blocks. A candidate outside the picture or before the start of the
// video packet is invalid: it counts as 0, and if only one is valid, that one is the prediction.
void pred_mv(const VState* s, int idx, int b, int* px, int* py, int* sx, int* sy) {
  static const S8 nb[4][3][2] = {  // [block][left, above, above-right]: macroblock (-1 left, -2 above,
    { { -1, 1 }, { -2, 2 }, { -3, 2 } },  // -3 above right, 0 this one), block
    { { 0, 0 }, { -2, 3 }, { -3, 2 } },
    { { -1, 3 }, { 0, 0 }, { 0, 1 } },
    { { 0, 2 }, { 0, 1 }, { 0, 0 } } };
  const int w = s->mb_w, x = idx % w, y = idx / w;
  int cx[3], cy[3], nv = 0, i;
  for (i = 0; i < 3; i++) {
    const int k = nb[b][i][0];
    int j = idx, ok = 1;
    if (k == -1) { j = idx - 1; ok = x > 0; }
    else if (k == -2) { j = idx - w; ok = y > 0; }
    else if (k == -3) { j = idx - w + 1; ok = y > 0 && x + 1 < w; }
    if (ok && k && j < s->vp_start) ok = 0;
    cx[i] = ok ? mvcur[j][nb[b][i][1]][0] : 0;
    cy[i] = ok ? mvcur[j][nb[b][i][1]][1] : 0;
    nv += ok;
  }
  if (nv == 1) {
    *px = cx[0] + cx[1] + cx[2];
    *py = cy[0] + cy[1] + cy[2];
    *sx = *sy = -1;
  } else {
    *sx = (cx[0] > cx[1] ? (cx[0] > cx[2] ? cx[0] : cx[2]) : (cx[1] > cx[2] ? cx[1] : cx[2])) -
          (cx[0] < cx[1] ? (cx[0] < cx[2] ? cx[0] : cx[2]) : (cx[1] < cx[2] ? cx[1] : cx[2]));
    *sy = (cy[0] > cy[1] ? (cy[0] > cy[2] ? cy[0] : cy[2]) : (cy[1] > cy[2] ? cy[1] : cy[2])) -
          (cy[0] < cy[1] ? (cy[0] < cy[2] ? cy[0] : cy[2]) : (cy[1] < cy[2] ? cy[1] : cy[2]));
    *px = cx[0] > cx[1] ? (cx[1] > cx[2] ? cx[1] : cx[0] > cx[2] ? cx[2] : cx[0])
                        : (cx[0] > cx[2] ? cx[0] : cx[1] > cx[2] ? cx[2] : cx[1]);
    *py = cy[0] > cy[1] ? (cy[1] > cy[2] ? cy[1] : cy[0] > cy[2] ? cy[2] : cy[0])
                        : (cy[0] > cy[2] ? cy[0] : cy[1] > cy[2] ? cy[2] : cy[1]);
  }
}

int spread_bucket(int s, int fcode) {  // spread of the prediction candidates, in VLC code units
  return s < 0 ? 6 : mv_bucket(s ? ((s - 1) >> (fcode - 1)) + 1 : 0);
}

// one vector (x, y) of kind kind, predicted by (px, py) from candidates with spreads (sx, sy) (-1:
// none), with the co-located vector (tx, ty) if tok
void code_mv(Coder* c, MB* m, int* mvi, int kind, int fcode, const MBCtx* L, const MBCtx* A,
                    const MBCtx* R, MBCtx* me, int* sumq, int px, int py, int sx, int sy, int tok, int tx, int ty,
                    int* mx, int* my) {
  const int first = *mvi == 0;
  int v, q;
  v = code_mvd(c, kind, 0, fcode, L, A, R, first ? 6 : mv_bucket(me->mva[0]), tok, mv_wrap(tx - px, fcode),
               spread_bucket(sx, fcode), m->mv[*mvi], &q);
  m->mv[(*mvi)++] = v;
  *sumq += q;
  *mx = mv_wrap(px + v, fcode);
  if (first) { me->mva[0] = (U8)q; me->mvs[0] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
  v = code_mvd(c, kind, 1, fcode, L, A, R, mv_bucket(q), tok, mv_wrap(ty - py, fcode), spread_bucket(sy, fcode),
               m->mv[*mvi], &q);
  m->mv[(*mvi)++] = v;
  *sumq += q;
  *my = mv_wrap(py + v, fcode);
  if (first) { me->mva[1] = (U8)q; me->mvs[1] = (U8)(v > 0 ? 1 : v < 0 ? 2 : 0); }
}

// one macroblock record; in decoding mode m is filled in
void code_mb(Coder* c, const Ctx* cx, VState* s, MB* m, int idx) {
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
  int intra = 0, cbp, cls3, i, k, mvi = 0, sumq = 0, px = 0, py = 0, sx = -1, sy = -1, frac = 4;
  if (!c->enc) {
    m->st = m->sk = 0;
    m->mb = m->t = m->mc = m->ac = m->dq = m->dt = m->fp = NA;
    m->nfs = m->fs = m->nmv = 0;
  }
  memset(me, 0, sizeof(*me));
  c->cat = CAT_MB;
  if (v->type == PT_B && mb_x == 0) memset(s->lmv, 0, sizeof(s->lmv));
  if (v->type != PT_I && v->type != PT_B) pred_mv(s, idx, 0, &px, &py, &sx, &sy);
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
    in[3] = &M->skip4[v->gmc][px || py][!R ? 2 : mvref[idx][0][0] || mvref[idx][0][1]];
    if ((m->sk = mbit(c, in, 4, mixw.mbt[0], m->sk))) {
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
    const int tok = R != 0, fm = cx->qpel ? 3 : 1;
    int mx = 0, my = 0, mx2, my2;
    if (v->type != PT_B) {  // the vectors as the decoder gets them; their prediction
      if (m->t == 1)
        for (i = 0; i < 4; i++) {
          if (i) pred_mv(s, idx, i, &px, &py, &sx, &sy);
          code_mv(c, m, &mvi, 5, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][i][0],
                  mvref[idx][i][1], &mx, &my);
          mvcur[idx][i][0] = (S16)mx;
          mvcur[idx][i][1] = (S16)my;
        }
      else {
        if (m->mc == 1) mx = px, my = py;  // GMC: the prediction stands in for the warped vector
        else if (m->fp == 1) {
          code_mv(c, m, &mvi, 1, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1] / 2, &mx, &my);
          code_mv(c, m, &mvi, 1, v->fcode, L, A, R, me, &sumq, px, py / 2, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1] / 2, &mx2, &my2);
          mx = ((mx + mx2) >> 1) | ((mx + mx2) & 1);
          my = my + my2;
        } else
          code_mv(c, m, &mvi, 0, v->fcode, L, A, R, me, &sumq, px, py, sx, sy, tok, mvref[idx][0][0],
                  mvref[idx][0][1], &mx, &my);
        for (i = 0; i < 4; i++) { mvcur[idx][i][0] = (S16)mx; mvcur[idx][i][1] = (S16)my; }
      }
    } else if (m->t) {  // B-VOP: predicted by the last vector in the row; the co-located one scaled as in
                        // direct mode
      const int n = m->fp == 1 ? 2 : 1, cx0 = R ? mvref[idx][0][0] : 0, cy0 = R ? mvref[idx][0][1] : 0;
      const int tfx = (int)((S64)cx0 * s->trb / s->trd), tfy = (int)((S64)cy0 * s->trb / s->trd);
      int dir;
      for (dir = 0; dir < 2; dir++) {
        const int code = dir ? v->bcode : v->fcode, tx = dir ? tfx - cx0 : tfx, ty = dir ? tfy - cy0 : tfy;
        if (!(dir ? bwd_dir(m->t) : fwd_dir(m->t))) continue;
        for (i = 0; i < n; i++) {
          const int ppx = s->lmv[dir][i][0], ppy = n == 2 ? s->lmv[dir][i][1] / 2 : s->lmv[dir][i][1];
          code_mv(c, m, &mvi, 2 + dir, code, L, A, R, me, &sumq, ppx, ppy, -1, -1, tok, tx, n == 2 ? ty / 2 : ty,
                  &mx, &my);
          if (n == 1) s->lmv[dir][0][0] = s->lmv[dir][1][0] = mx, s->lmv[dir][0][1] = s->lmv[dir][1][1] = my;
          else s->lmv[dir][i][0] = mx, s->lmv[dir][i][1] = my * 2;
        }
      }
    } else
      code_mv(c, m, &mvi, 4, 1, L, A, R, me, &sumq, 0, 0, -1, -1, 0, 0, 0, &mx, &my);
    if (v->type != PT_B && m->mc != 1) mx = mvcur[idx][0][0], my = mvcur[idx][0][1];
    if ((v->type != PT_B && m->mc != 1) || (v->type == PT_B && m->t)) frac = ((mx & fm) != 0) + 2 * ((my & fm) != 0);
  }
  c->cat = CAT_CBP;
  cbp = v->type == PT_B && m->mb == 1 ? 0
      : code_cbp(c, cls3, L, A, R, mvi ? mv_bucket(sumq) : 6, frac,
                 v->qscale < 3 ? 0 : v->qscale < 5 ? 1 : v->qscale < 7 ? 2 : v->qscale < 9 ? 3 : v->qscale < 12 ? 4
                 : v->qscale < 17 ? 5 : v->qscale < 24 ? 6 : 7,
                 (L ? L->cbp != 0 : 2) == (A ? A->cbp != 0 : 2) ? 0 : 1 + (R && R->cbp), c->enc ? cbp_of(m) : 0);
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
                 mvi ? mv_bucket(sumq) : 6, R ? nz_bucket(R->nz[i]) : 6, Lb, Ab, &rowb[mb_x][i],
                 i == 5 ? &rowb[mb_x][4] : 0);
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
int mb_codable(const MB* m) {
  int i, j;
  for (i = 0; i < 6; i++)
    for (j = 0; j < m->b[i].n; j++)
      if (!m->b[i].level[j]) return 0;
  return 1;
}

//--- VOPs ----------------------------------------------------------------------------------------


void vop_state(Vop* v, const Ctx* cx, const VopHdr* h) {
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
// -1: it can't be (parsed, written: unsupported or damaged data), -2: no memory.
int vop_walk(Ctx* cx, int mode, GB* gb, PB* pb, Coder* c) {
  MB& m = mrec;
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
    if (!ctx_skip_room(cx, cx->mb_num) || !mb_room(cx->mb_num)) return -2;
    vop_state(&vp, cx, &h);
    vw = s.v = vp;
    s.mb_w = cx->mb_w;
    s.scan = h.alt == 1 ? alternate_vertical_scan : zigzag_direct;
    anchor = h.type != PT_B;
    if (mode != W_DECODE && anchor) memset(cx->skip_cur, 0, cx->mb_num + 1);
    if (mode != W_VERIFY) {
      memset(mbcur, 0, cx->mb_num * sizeof(*mbcur));
      memset(mvcur, 0, cx->mb_num * sizeof(*mvcur));
      if (ref_mbs != cx->mb_num) ref_mbs = 0;
      s.trb = last.pb_time;
      s.trd = last.pp_time;
      if (s.trb <= 0 || s.trb >= s.trd) s.trb = 1, s.trd = 2;
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
        if (anchor) cx->skip_cur[idx] = (U8)(m.sk == 1 && !vp.gmc);
      }
      if (mode != W_VERIFY) code_mb(c, cx, &s, &m, idx);
      if (mode != W_ENCODE && put_mb(cx, pb, &vw, &m) < 0) { ret = -1; break; }
      if (mode == W_DECODE && (pb->err || c->io->over > 16)) { ret = -1; break; }  // damaged data
      if (++mb_x == cx->mb_w) { mb_x = 0; mb_y++; }
      if (mb_y >= cx->mb_h) break;
      {
        VpHdr k;
        int vpf = 0, nstuff = 0, next;
        if (mode != W_DECODE) {
          next = resync_ahead(cx, gb, &vp, &nstuff);
          vpf = next && !(idx + 1 < next && vp.type == PT_B && cx->skip_ref[idx + 1]);
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
        if (mode != W_VERIFY) {
          code_vp(c, cx, &s, &k, idx + 1);
          s.vp_start = k.mbnum;
          memset(s.lmv, 0, sizeof(s.lmv));
        }
        if ((mode != W_ENCODE && put_vp(cx, pb, &vw, &k) < 0) || k.mbnum < 1 || k.mbnum >= cx->mb_num) {
          ret = -1;
          break;
        }
        mb_x = k.mbnum % cx->mb_w;
        mb_y = k.mbnum / cx->mb_w;
      }
    }
    if (mode != W_DECODE && anchor) memcpy(cx->skip_ref, cx->skip_cur, cx->mb_num + 1);
    if (ret < 0) return -1;
    if (mode != W_VERIFY && anchor) {
      memcpy(mbref, mbcur, cx->mb_num * sizeof(*mbcur));
      memcpy(mvref, mvcur, cx->mb_num * sizeof(*mvcur));
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
};

//--- the compressor and the decompressor --------------------------------------------------------

// The stream: the magic, its last byte the format's version, then the arithmetic coded data: the
// chunks (kind, then the records of a VOP or a raw chunk's length and bytes), the end, and the
// CRC-32 of the stream (32 bits at 1/2 each).
static const U8 magic[8] = { 'm', '4', 'v', 'p', 'a', 'c', 'k', 3 };

// a compression's statistics (its summary line)
typedef struct Stats {
  U32 vops, packed, by_type[5], raw_unsup, raw_long, raw_parse, raw_verify;
  double in_bytes, raw_bytes, vop_bytes;
  U64 parts;    // -t: the parts
  int threads;  // and the threads
} Stats;

// what W_VERIFY's VOP written back is compared with, as its writer flushes it
typedef struct Cmp {
  const U8* p;
  size_t n, at;
  int differs;
} Cmp;
static void cmp_flush(void* ctx, const U8* p, size_t n) {
  Cmp* c = (Cmp*)ctx;
  if (c->differs || n > c->n - c->at || memcmp(c->p + c->at, p, n)) c->differs = 1;
  else c->at += n;
}

// A run of m4vpack: compressing (f_DEC 0) or decompressing (1) a stream, as a coroutine. Plain
// data, mapped (coro_new(), zeroed) and never constructed: Init() sets it up, Quit() frees what
// it holds. A run shares nothing with another but the constant tables, so any number of them can
// go at once, on as many threads.
template <int f_DEC>
struct M4vCoro : CoIo {
  int stats;   // c -s: the bits by kind of syntax element (co.cost, co.orig)
  int status;  // the run's: 0, or the exit status of its error (note); 1 until it has ended
  Stats st;    // c's
  U32 crc;     // of the stream: c's input, d's output
  Coder co;
  Reader rd;   // c: the input
  Ctx cx;
  Codec k;
  U8 pbuf[1 << 16];  // a VOP written back (W_VERIFY, W_DECODE), a buffer at a time

  void Init(int s) {
    coro_init();
    stats = s;
    status = 1;
  }
  void Quit(void) {
    ctx_quit(&cx);
    k.Quit();
    free(rd.buf);
    rd.buf = 0;
  }

  void do_process(void) {
    int r;
    io_init();
    r = f_DEC ? run_d() : run_c();
    Quit();
    status = r;
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  int run_c(void) {
    size_t pos, len;
    int part, r;
    memset(&st, 0, sizeof st);
    crc = 0;
    ctx_init(&cx);
    k.model_init();
    if (!rd_init(&rd, this)) return fail(5, "out of memory");
    write(magic, sizeof magic);
    co_init(&co, this, 1);
    co.stats = stats;
    while (rd_chunk(&rd, &pos, &len, &part)) {
      U8* p = rd.buf + pos;
      const int is_vop = len >= 4 && !p[0] && !p[1] && p[2] == 1 && p[3] == 0xB6;
      crc = crc32_upd(crc, p, len);
      st.in_bytes += (double)len;
      if (is_vop && part != 2) {
        st.vops++;
        if (len >= 5) st.by_type[(p[4] >> 6) + PT_I]++;
        if (part) st.raw_long++;
        else if (!cx.have_vol || cx.unsupported || (cx.dp && (len < 5 || (p[4] >> 6) != 2))) st.raw_unsup++;
        else {
          const int tib = cx.time_inc_bits;
          GB gb;
          PB pb;
          Cmp cm;
          gb.buf = p + 4;
          gb.size = (int)(len - 4) * 8;
          gb.pos = 0;
          pb_init(&pb, pbuf, sizeof pbuf);
          cm.p = p;
          cm.n = len;
          cm.at = 0;
          cm.differs = 0;
          pb.flush = cmp_flush;
          pb.ctx = &cm;
          r = k.vop_walk(&cx, W_VERIFY, &gb, &pb, 0);
          if (r == -2) return fail(5, "out of memory");
          if (r < 0) st.raw_parse++;
          else {
            pb_finish(&pb);
            if (pb.err || pb.n || cm.differs || cm.at != len) st.raw_verify++;
            else {
              cx.time_inc_bits = tib;  // the parse again sees the same state
              gb.pos = 0;
              k.code_kind(&co, 1);
              r = k.vop_walk(&cx, W_ENCODE, &gb, 0, &co);
              if (r == -2) return fail(5, "out of memory");
              if (r < 0) return fail(5, "internal error: a verified VOP couldn't be coded");
              st.packed++;
              st.vop_bytes += (double)len;
              continue;
            }
          }
        }
      }
      if (is_vop) st.raw_bytes += (double)len;
      co.orig[CAT_RAW] += 8.0 * len;
      k.code_kind(&co, 2);
      k.code_raw_len(&co, len);
      k.code_raw_bytes(&co, p, len);
      parse_header_chunk(&cx, p, len);
    }
    k.code_kind(&co, 0);
    k.code_u32(&co, crc);
    co_flush(&co);
    return 0;
  }

  static void out_flush(void* ctx, const U8* p, size_t n) {  // d: a VOP's bytes, out
    M4vCoro* c = (M4vCoro*)ctx;
    c->write(p, n);
    c->crc = crc32_upd(c->crc, p, n);
  }

  int run_d(void) {
    U8 h[sizeof magic];
    int kind, r;
    crc = 0;
    if (read(h, sizeof h) != sizeof h || memcmp(h, magic, sizeof magic - 1)) return fail(4, "not an m4vpack file");
    if (h[sizeof h - 1] != magic[sizeof magic - 1])
      return fail(4, "format version %d, which this m4vpack (version %d) doesn't read", h[sizeof h - 1],
                  magic[sizeof magic - 1]);
    ctx_init(&cx);
    k.model_init();
    co_init(&co, this, 0);
    while ((kind = k.code_kind(&co, 0)) != 0) {
      if (kind == 1) {
        PB pb;
        pb_init(&pb, pbuf, sizeof pbuf);
        pb.flush = out_flush;
        pb.ctx = this;
        r = k.vop_walk(&cx, W_DECODE, 0, &pb, &co);
        if (r == -2) return fail(5, "out of memory");
        if (r < 0 || pb.err || pb.n) return fail(4, "damaged data");
        pb_finish(&pb);
      } else {
        size_t n = k.code_raw_len(&co, 0), done = 0;
        if (!n || n > IBUF_SIZE) return fail(4, "damaged data");
        while (done < n) {  // a buffer at a time; the first has what parse_header_chunk() reads
          const size_t l = n - done < sizeof pbuf ? n - done : sizeof pbuf;
          k.code_raw_bytes(&co, pbuf, l);
          if (!done) parse_header_chunk(&cx, pbuf, n);
          write(pbuf, l);
          crc = crc32_upd(crc, pbuf, l);
          done += l;
          if (over > 16) return fail(4, "damaged data");
        }
      }
      if (over > 16) return fail(4, "damaged data");
    }
    if (k.code_u32(&co, 0) != crc || over > 16) return fail(4, "damaged data");
    return 0;
  }
};

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them (its tables for the largest pictures). coro_new() makes it (plain data: then Init()),
// coro_free() frees it after its Quit().
template <class T>
T* coro_new(void) {
  return (T*)VirtualAlloc(0, sizeof(T), MEM_COMMIT, PAGE_READWRITE);
}
template <class T>
void coro_free(T* p) {
  if (p) VirtualFree(p, 0, MEM_RELEASE);
}

// The output of a call: a file, or (c -v) the input to compare with
typedef struct Sink {
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs, err;
} Sink;
static void sink_write(Sink* s, const U8* p, size_t n) {
  static U8 cmp[1 << 16];  // (the main thread's)
  if (!s->compare) {
    if (n && fwrite(p, 1, n, s->f) != n) s->err = 1;
    s->pos += n;
    return;
  }
  while (n && !s->differs) {
    const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
    const size_t got = fread(cmp, 1, k, s->f);
    size_t i;
    for (i = 0; i < got && cmp[i] == p[i]; i++) {}
    if (i < k) { s->differs = 1; s->diff_at = s->pos + i; return; }
    s->pos += k;
    p += k;
    n -= k;
  }
}

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns)
template <class C, class In, class Out>
uint drive(C& c, In in, Out out) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) {
      in();
      continue;
    }
    out(r == Y_OUT);
    if (r != Y_OUT) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s
static U8 io_bufs[2][1 << 16];  // (the main thread's)
template <class C>
uint run_file(C& c, FILE* f, Sink* s, const U8* pre = 0, size_t n = 0) {
  const uint kIo = sizeof io_bufs[0];
  U8 *ib = io_bufs[0], *ob = io_bufs[1];
  c.addout(ob, kIo);
  if (n) c.addinp((byte*)pre, (uint)n);
  return drive(
      c,
      [&]() {
        const uint l = (uint)fread(ib, 1, kIo, f);
        if (!l) c.f_quit = 1;
        c.addinp(ib, l);
      },
      [&](bool more) {
        sink_write(s, c.outbeg, c.getoutsize());
        if (more) c.addout(ob, kIo);
      });
}

//--- threads (-t) --------------------------------------------------------------------------------

// c -t N: the stream cut into parts by M4vSplit (on the main thread), each part a stream by itself,
// and the parts dealt in turn to N workers by MTwrap (MTwrap_v3, its parts mode), a model for each
// part (M4vPart runs an M4vCoro on it). A part ends:
//   - before the first I-VOP at least -c's part size after the part's start, or before the run of
//     headers (VOS, VO, VOL, user data, GOV) right before that I-VOP;
//   - where no I-VOP comes, before the first VOP but a B-VOP at least mt_cap() after its start (a
//     B-VOP's syntax depends on the VOP before it);
//   - where no VOP comes within half again of the cap, at any start code, and where none comes
//     within twice the cap, there.
// VOPs that aren't coded (vop_coded 0: the placeholders packed B-VOPs leave) don't count as I-VOPs
// or other VOPs here. The headers m4vpack's parser reads (the last VOL, the last DivX and XviD
// user data), as defined before a part, are copied in at its start (their first 32 KB, which is
// what the parser reads), unless the part defines them itself before its first VOP: so each part
// parses as it would in the whole stream, but where the parser guessed the VOP header's time
// increment size.
//
// A worker's stream is a record for each of its parts:
//   u64 g_size  the record's size, these MT_REC (48) bytes included
//   u64 plen    the size of the part as compressed (with the headers copied in)
//   u64 ins     where in it those are: [ins, ins + xlen), removed after decoding
//   u64 xlen
//   u64 part    its place in the stream, from 0: worker w's k-th part is w + kN
//   u32 crc     the CRC-32 of these 40 bytes; u32 0
//   g_size - 48 bytes: the part compressed, a stream of the single kind (magic, chunks, CRC)
// and its end, a record of the 48 bytes alone, whose part is the place its next part would have
// had (little endian). So d finds a part lost, added or moved, or a worker's stream cut short.
// MTwrap's container holds the N streams, after m4vpack's header: mt_magic, the format version,
// the threads, the part size in KB (4 bytes). d needs as many threads as c had; each part ends a
// round of MTwrap's decoder (Y_PART), which takes the parts from the workers in turn, as dealt.

static const U8 mt_magic[5] = { 'm', '4', 'v', 'p', 'm' };
enum {
  MT_HEAD = 11,     // mt_magic, the version, the threads, the part size in KB
  MT_REC = 48,      // a worker's record
  MT_FRAME = 24     // a part as the splitter frames it: u64 L, u64 ins, u64 xlen, then L - 16 bytes
};
#define MT_RING (1u << 20)      // MTwrap's buffers to and from each worker
#define MT_COPY_MAX (32u << 10) // the bytes of a header copied in, at most

static void put_le(U8* p, U64 v, int n) { int i; for (i = 0; i < n; i++) p[i] = (U8)(v >> (8 * i)); }
static U64 get_le(const U8* p, int n) { U64 v = 0; int i; for (i = n - 1; i >= 0; i--) v = (v << 8) | p[i]; return v; }

// the size past which a part ends at any VOP but a B-VOP
static U64 mt_cap(U64 part) { return 4 * part < (1ull << 31) ? 4 * part : 1ull << 31; }
// The most a part can be, as the splitter cuts it: twice the cap, and the headers copied in. d
// takes a record of a larger part to be broken, which bounds the memory a part's output takes.
static U64 mt_part_max(U64 part) { return 2 * mt_cap(part) + 3 * (U64)MT_COPY_MAX; }

static void mt_head(U8* h, int threads, U32 part) {
  memcpy(h, mt_magic, sizeof mt_magic);
  h[5] = magic[sizeof magic - 1];
  h[6] = (U8)threads;
  put_le(h + 7, part >> 10, 4);
}

static U32 mt_rec_crc(const U8* h) { return crc32_upd(0, h, 40); }
static void mt_rec(U8* h, U64 g_size, U64 plen, U64 ins, U64 xlen, U64 part) {
  put_le(h, g_size, 8);
  put_le(h + 8, plen, 8);
  put_le(h + 16, ins, 8);
  put_le(h + 24, xlen, 8);
  put_le(h + 32, part, 8);
  put_le(h + 40, mt_rec_crc(h), 4);
  put_le(h + 44, 0, 4);
}

// room for n bytes at *p (*room of them now): 0 if there is no memory
static int grow(U8** p, size_t* room, size_t n) {
  size_t r = *room ? *room : 1 << 16;
  U8* q;
  if (n <= *room) return 1;
  while (r < n) r = r < ((size_t)-1) / 2 ? 2 * r : n;
  if (!(q = (U8*)realloc(*p, r))) return 0;
  *p = q;
  *room = r;
  return 1;
}

// Whether the VOP at p (n bytes from its start code on) is coded (vop_coded), as the parser reads it
// with the VOL c has (and its guess of the time increment's length, kept in c); -1 if its header
// doesn't parse
static int vop_is_coded(Ctx* c, const U8* p, size_t n) {
  U8 b[64 + 64];
  GB gb;
  VopHdr h;
  const size_t k = n - 4 < 64 ? n - 4 : 64;
  memset(b, 0, sizeof b);
  memcpy(b, p + 4, k);
  gb.buf = b;
  gb.size = (int)k * 8;
  gb.pos = 0;
  h.coded = -1;
  parse_vop_hdr(c, &gb, &h);  // (coded as far as it got: the rest of the header doesn't matter here)
  return h.coded;
}

// The splitter: the stream in, its parts out as frames for MTwrap's parts mode (u64 L, u64 ins,
// u64 xlen, the part with the headers copied in at ins), on the main thread. Plain data (a part of
// MtEnc, mapped by coro_new()): Init() sets it up, Quit() frees what it holds.
enum { K_VOL, K_DIVX, K_XVID, NKEY };  // the headers copied in
struct M4vSplit : Coroutine {
  U64 chunksize, cap;  // -c's part size; mt_cap() of it
  U8* buf;             // the stream from begin on: the part under way
  size_t len, room;
  U64 begin, scan;     // the part's start; where to look for start codes next (stream offsets)
  U64 total, parts;    // the stream's size (at its end), the parts
  int eof;
  // the chunk under way (from its start code at chunk_at), and the run of headers it may be in
  U64 chunk_at, run_at;
  int chunk_code, run;  // chunk_code: the start code's last byte, -1 (none, or the part started in it)
  // the headers to copy in: as defined before the run under way (copy) and in it (pend, keys in
  // pend_has); the copies of the part under way (extra, xlen bytes)
  U8 copy[NKEY][MT_COPY_MAX], pend[NKEY][MT_COPY_MAX];
  U32 copy_len[NKEY], pend_len[NKEY];
  int pend_has;
  U8 extra[NKEY * MT_COPY_MAX];
  U32 xlen;
  Ctx cx;  // the last VOL's (whether a VOP is coded)

  void Init(U64 part) {
    coro_init();
    ctx_init(&cx);
    chunksize = part;
    cap = mt_cap(part);
    buf = 0;
    len = room = 0;
    begin = scan = total = parts = 0;
    eof = 0;
    chunk_at = run_at = 0;
    chunk_code = -1;
    run = 0;
    memset(copy_len, 0, sizeof copy_len);
    memset(pend_len, 0, sizeof pend_len);
    pend_has = 0;
    xlen = 0;
  }
  void Quit(void) {
    free(buf);
    buf = 0;
    len = room = 0;
    ctx_quit(&cx);
  }

  void put(const U8* p, size_t n) {
    while (n) {
      uint l;
      chkout();
      l = (uint)(n < getoutleft() ? n : getoutleft());
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
    }
  }

  // the part under way, up to the stream offset x, as a frame; the next starts at x
  void emit(U64 x) {
    const size_t n = (size_t)(x - begin);
    U8 h[MT_FRAME];
    put_le(h, MT_FRAME - 8 + (U64)xlen + n, 8);
    put_le(h + 8, 0, 8);
    put_le(h + 16, xlen, 8);
    put(h, MT_FRAME);
    put(extra, xlen);
    put(buf, n);
    memmove(buf, buf + n, len - n);
    len -= n;
    begin = x;
    parts++;
  }
  // a cut at x: the part before it out, and the copies for the one after it (the headers defined
  // before x, but those its run of headers so far defines)
  void cut(U64 x) {
    int k;
    emit(x);
    xlen = 0;
    for (k = 0; k < NKEY; k++)
      if (copy_len[k] && !(pend_has >> k & 1)) {
        memcpy(extra + xlen, copy[k], copy_len[k]);
        xlen += copy_len[k];
      }
    if (chunk_at < x) chunk_code = -1;  // (a cut inside a chunk: its start is gone)
    if (run_at < x) run_at = x;
  }
  // the run of headers ends: what it defined is defined before the next run
  void commit(void) {
    int k;
    for (k = 0; k < NKEY; k++)
      if (pend_has >> k & 1) {
        memcpy(copy[k], pend[k], pend_len[k]);
        copy_len[k] = pend_len[k];
      }
    pend_has = 0;
    run = 0;
  }
  static int is_header(int c) { return c == 0xB0 || c == 0xB5 || c == 0xB2 || c == 0xB3 || (c >= 0 && c < 0x30); }
  // the chunk under way ends at the stream offset a: a header to copy into pend
  void end_chunk(U64 a) {
    const U8* p = buf + (chunk_at - begin);
    const size_t n = (size_t)(a - chunk_at);
    int k = -1;
    if (chunk_code < 0) return;
    if (chunk_code >= 0x20 && chunk_code < 0x30) parse_header_chunk(&cx, p, n);
    if (chunk_code >= 0x20 && chunk_code < 0x30) k = K_VOL;
    else if (chunk_code == 0xB2 && n >= 8 && !memcmp(p + 4, "DivX", 4)) k = K_DIVX;
    else if (chunk_code == 0xB2 && n >= 8 && !memcmp(p + 4, "XviD", 4)) k = K_XVID;
    if (k < 0) return;
    pend_len[k] = (U32)(n < MT_COPY_MAX ? n : MT_COPY_MAX);
    memcpy(pend[k], p, pend_len[k]);
    pend_has |= 1 << k;
  }
  // a start code at the stream offset a: the chunk before it ends, and a cut before this one (or
  // before its run of headers) if one is due. A VOP that isn't coded (as packed B-VOPs leave) is no
  // place to cut: the B-VOPs after it depend on the coded VOP before it.
  void start_code(U64 a) {
    const size_t i = (size_t)(a - begin);
    const int code = i + 3 < len ? buf[i + 3] : -1;
    int type = code == 0xB6 && i + 4 < len ? buf[i + 4] >> 6 : -1;  // a VOP's: I 0, P 1, B 2, S 3
    U64 x;
    end_chunk(a);
    if (type >= 0 && !vop_is_coded(&cx, buf + i, len - i)) type = -1;
    if (is_header(code) && !run) {
      run = 1;
      run_at = a;
    }
    x = run ? run_at : a;
    if (x > begin && ((type == 0 && x - begin >= chunksize) || (type >= 0 && type != 2 && x - begin >= cap) ||
                      a - begin >= cap + cap / 2))
      cut(x);
    if (!is_header(code)) commit();
    chunk_at = a;
    chunk_code = code;
  }

  void do_process(void) {
    for (;;) {
      uint l;
      chkinp();
      l = getinplen();
      if (!l) break;  // the end
      if (!grow(&buf, &room, len + l + 64)) no_memory();
      memcpy(buf + len, inpptr, l);
      inpptr += l;
      len += l;
      scan_input();
    }
    eof = 1;
    scan_input();
    total = begin + len;
    end_chunk(total);
    commit();
    if (total > begin) emit(total);
    yield(this, Y_END);
  }
  // the start codes in what came of the input, but one whose code (and VOP type) is to come; and
  // where a stretch without one reaches past twice the cap, a cut there
  void scan_input(void) {
    for (;;) {
      size_t i = (size_t)(scan - begin);
      const int found = find_sc(buf, &i, len);
      const U64 a = begin + i;  // the start code, or where to look on
      while (a - begin > 2 * cap) cut(begin + 2 * cap);
      i = (size_t)(a - begin);
      if (!found || (i + 64 > len && !eof)) {  // (a VOP's header to come)
        scan = a;
        return;
      }
      scan = a + 3;
      start_code(a);
    }
  }
};

typedef struct MtArgs {
  int threads, stats;
  U64 part_max;  // d: the largest part a record can have (mt_part_max())
} MtArgs;

// MTwrap's Enc_class: a worker, coding the parts it is dealt one after the other, each by a model
// of its own (M4vCoro, a coroutine of the worker's, on the part in memory). Plain data, never
// constructed (MTwrap maps its workers): Init() sets it up, do_process() frees what it took as it
// ends. What a part's run fails with is the worker's error (its note).
template <int f_DEC>
struct M4vPart : CoIo {
  MtArgs args;
  U32 worker;      // its place among the workers, dealt the parts in turn
  U64 parts;       // the parts done
  Stats st;        // c: its parts', added up (-s: and the bits by kind of syntax element)
  double cost[NCAT], orig[NCAT];
  U8 *in_, *out_;  // c: a part, all of it; d: 64 KB of its record at a time. The part's run's output
  size_t in_room, out_room, out_len;

  uint Init(const MtArgs* a, uint w) {
    coro_init();
    args = *a;
    worker = w;
    parts = 0;
    memset(&st, 0, sizeof st);
    memset(cost, 0, sizeof cost);
    memset(orig, 0, sizeof orig);
    in_ = out_ = 0;
    in_room = out_room = out_len = 0;
    return 0;
  }
  void Quit(void) {
    free(in_);
    free(out_);
    in_ = out_ = 0;
    in_room = out_room = 0;
  }

  void do_process(void) {
    int r;
    io_init();
    r = f_DEC ? run_d() : run_c();
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  U64 next_part(void) const { return worker + parts * (U64)args.threads; }

  // A part's run: the coroutine c on the input feed() gives (feed(&p): the next piece at p, its size;
  // 0 at the end), its output into out_ (out_len bytes; most at most, one more fails the part); its
  // status (0, or the exit status of its error, its note copied to the worker's)
  template <class C, class Feed>
  int run_part(C* c, Feed feed, U64 most) {
    int r;
    uint y;
    out_len = 0;
    if (!grow(&out_, &out_room, 1 << 16)) return fail(5, "out of memory");
    c->addout(out_, 1 << 16);
    for (;;) {
      y = c->coro_call(c);
      if (y == Y_IN) {
        const U8* p = 0;
        const size_t l = feed(&p);
        if (!l) c->f_quit = 1;
        c->addinp((byte*)(l ? p : out_), (uint)l);
        continue;
      }
      out_len += c->getoutsize();
      if (y != Y_OUT) break;
      if (out_len > most) {  // (a part that comes out longer than its record says: d only)
        c->Quit();
        return fail(4, "damaged data: a part comes out longer than its record says");
      }
      if (!grow(&out_, &out_room, out_len + (1 << 16))) {
        c->Quit();
        return fail(5, "out of memory");
      }
      c->addout(out_ + out_len, 1 << 16);
    }
    r = c->status;
    if (r) fail(r, "%s", c->note);
    return r;
  }

  // c: each part as the splitter frames it (u64 L, u64 ins, u64 xlen, then the part, L - 16
  // bytes) into its record, and as the parts end, the record of the worker's end
  int run_c(void) {
    for (;;) {
      U8 h[MT_FRAME], rec[MT_REC];
      const size_t n = read(h, MT_FRAME);
      U64 L, ins, xlen, plen, pos = 0;
      M4vCoro<0>* c;
      int r, i;
      if (!n) {
        mt_rec(rec, MT_REC, 0, 0, 0, next_part());
        write(rec, MT_REC);
        return 0;
      }
      L = get_le(h, 8);
      ins = get_le(h + 8, 8);
      xlen = get_le(h + 16, 8);
      plen = L - (MT_FRAME - 8);
      if (n < MT_FRAME || L < MT_FRAME - 8 || ins > plen || xlen > plen - ins || plen != (size_t)plen)
        return fail(5, "internal error: a part framed wrong");
      if (!grow(&in_, &in_room, (size_t)plen + 1)) return fail(5, "out of memory");
      if (read(in_, (size_t)plen) != plen) return fail(5, "internal error: a part cut short");
      if (!(c = coro_new<M4vCoro<0> >())) return fail(5, "out of memory");
      c->Init(args.stats);
      r = run_part(c, [&](const U8** p) {  // in_, a gigabyte at most at a time (addinp() takes 32 bits)
        const size_t l = plen - pos < (1u << 30) ? (size_t)(plen - pos) : 1u << 30;
        *p = in_ + pos;
        pos += l;
        return l;
      }, ~0ull);
      if (!r) {
        st.vops += c->st.vops;
        st.packed += c->st.packed;
        for (i = 0; i < 5; i++) st.by_type[i] += c->st.by_type[i];
        st.raw_unsup += c->st.raw_unsup;
        st.raw_long += c->st.raw_long;
        st.raw_parse += c->st.raw_parse;
        st.raw_verify += c->st.raw_verify;
        st.raw_bytes += c->st.raw_bytes;
        st.vop_bytes += c->st.vop_bytes;
        for (i = 0; i < NCAT; i++) cost[i] += c->co.cost[i], orig[i] += c->co.orig[i];
      }
      coro_free(c);
      if (r) return r;
      mt_rec(rec, MT_REC + out_len, plen, ins, xlen, next_part());
      write(rec, MT_REC);
      write(out_, out_len);
      parts++;
      // the record handed over now, its last buffer as far as it is filled (a round's end): so
      // MTwrap puts each record in the container as its part is done, and d, which takes the
      // parts in turn, finds the next within about a round of the others
      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }

  // d: each record into its part, the headers copied in removed, after which MTwrap takes the
  // next part from the next worker; to the record of the worker's end, after which nothing
  int run_d(void) {
    for (;;) {
      U8 h[MT_REC];
      U64 g_size, plen, ins, xlen, left;
      M4vCoro<1>* c;
      int r;
      if (read(h, MT_REC) < MT_REC) return fail(4, "damaged data: a worker's records cut short");
      g_size = get_le(h, 8);
      plen = get_le(h + 8, 8);
      ins = get_le(h + 16, 8);
      xlen = get_le(h + 24, 8);
      if (get_le(h + 40, 4) != mt_rec_crc(h) || get_le(h + 44, 4) || get_le(h + 32, 8) != next_part())
        return fail(4, "damaged data: a part's record broken, or a part lost, added or moved");
      if (g_size == MT_REC) {  // the worker's end
        U8 x;
        if (plen || ins || xlen) return fail(4, "damaged data: a part's record broken");
        if (read(&x, 1)) return fail(4, "damaged data after a worker's end");
        return 0;
      }
      if (plen > args.part_max || ins > plen || xlen > plen - ins || g_size < MT_REC)
        return fail(4, "damaged data: a part's record broken");
      // the part compressed, read as its run asks for it, 64 KB at a time
      left = g_size - MT_REC;
      if (!grow(&in_, &in_room, 1 << 16)) return fail(5, "out of memory");
      if (!(c = coro_new<M4vCoro<1> >())) return fail(5, "out of memory");
      c->Init(0);
      r = run_part(c, [&](const U8** p) {
        const size_t got = read(in_, left < (1u << 16) ? (size_t)left : 1u << 16);
        left -= got;
        *p = in_;
        return got;
      }, plen);
      coro_free(c);
      if (r) return r;
      if (left) return fail(4, "damaged data: a part's record broken");  // (its stream ended before it)
      if (out_len != plen) return fail(4, "damaged data: a part comes out shorter than its record says");
      write(out_, (size_t)ins);
      write(out_ + ins + xlen, (size_t)(plen - ins - xlen));
      parts++;
      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }
};

// c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef CoroutinePair<M4vSplit, MTwrap<0, M4vPart<0>, const MtArgs*> > MtEnc;
typedef MTwrap<1, M4vPart<1>, const MtArgs*> MtDec;

// whether a worker failed: the first one's exit status, its note copied to note (the workers go
// with MTwrap's Quit())
template <class M>
int mt_failed(M& m, char* note, size_t n) {
  uint i;
  for (i = 0; i < m.th_N; i++)
    if (m.C1[i].f_fail) {
      snprintf(note, n, "%s", m.C1[i].note);
      return m.C1[i].code ? m.C1[i].code : 5;
    }
  return 0;
}
// whether d's workers had the parts as c deals them, in turn: of P in all, worker i the i-th of each
// round of N, one more where P % N is past it
static int mt_dealt(MtDec& d) {
  U64 P = 0, N = d.th_N;
  uint i;
  for (i = 0; i < d.th_N; i++) P += d.C1[i].parts;
  for (i = 0; i < d.th_N; i++)
    if (d.C1[i].parts != P / N + (i < P % N)) return 0;
  return 1;
}
//--- the calls -----------------------------------------------------------------------------------

typedef struct Opts {
  int quiet, verify, stats;
  int threads;  // -t
  U32 part;     // -c: a part's size, at least
} Opts;

// a call's file, "-" for stdin or stdout
static FILE* open_file(const char* name, int out) {
  if (!strcmp(name, "-")) return out ? stdout : stdin;
  return fopen(name, out ? "wb" : "rb");
}

// c's summary line (out_bytes: the compressed size), and with -s the bits by kind of syntax element
static void report(const char* name, const Stats* st, const double* cost, const double* orig, double out_bytes,
                   int stats) {
  int i;
  fprintf(stderr, "%s: %.0f -> %.0f bytes (%.2f%%); %u VOPs (I %u, P %u, B %u, S %u), %u packed", name,
          st->in_bytes, out_bytes, st->in_bytes ? 100.0 * out_bytes / st->in_bytes : 0.0, st->vops, st->by_type[1],
          st->by_type[2], st->by_type[3], st->by_type[4], st->packed);
  if (st->vops > st->packed)
    fprintf(stderr, ", %u kept as bytes (%u unsupported, %u too long, %u unparsable, %u not reproduced)",
            st->vops - st->packed, st->raw_unsup, st->raw_long, st->raw_parse, st->raw_verify);
  if (st->threads) fprintf(stderr, "; %.0f parts on %d threads", (double)st->parts, st->threads);
  fprintf(stderr, "\n");
  if (stats) {
    static const char* const grp_name[5] = { "headers, raw chunks", "macroblock types, cbp, dquant", "motion vectors",
                                             "intra DC", "coefficients" };
    static const int grp_first[6] = { CAT_RAW, CAT_MB, CAT_MV, CAT_DC, CAT_SIG, NCAT };
    double o = 0, p = 0;
    int g;
    fprintf(stderr, "  %-33s %10s %10s\n", "bytes", "input", "packed");
    for (g = 0; g < 5; g++) {
      double og = 0, pg = 0;
      for (i = grp_first[g]; i < grp_first[g + 1]; i++) { og += orig[i]; pg += cost[i]; }
      fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", grp_name[g], og / 8, pg / 8, og ? 100 * pg / og : 0.0);
      if (grp_first[g + 1] - grp_first[g] > 1)
        for (i = grp_first[g]; i < grp_first[g + 1]; i++) fprintf(stderr, "    %-42s %10.0f\n", cat_name[i], cost[i] / 8);
      o += og;
      p += pg;
    }
    fprintf(stderr, "  %-33s %10.0f %10.0f %6.2f%%\n", "total", o / 8, p / 8, o ? 100 * p / o : 0.0);
  }
}

// the end of a run on the files f (input) and s (output): its status r (0, or the exit status of an
// error the run's note says), unless reading or writing failed
static int run_end(int r, const char* note, FILE* f, const char* in_name, Sink* s, const char* out_name) {
  if (ferror(f)) {
    fprintf(stderr, "m4vpack: %s: read error\n", in_name);
    return 2;
  }
  if (s->err) {
    fprintf(stderr, "m4vpack: %s: write error\n", out_name);
    return 3;
  }
  if (r) fprintf(stderr, "m4vpack: %s: %s\n", in_name, note);
  return r;
}

// what MTwrap's Init() returned, said (0: nothing to say)
static int mt_init_failed(uint r, int threads) {
  if (r == 7) fprintf(stderr, "m4vpack: can't start %d threads\n", threads);
  else if (r) fprintf(stderr, "m4vpack: no memory for the buffers of %d threads\n", threads);
  return r != 0;
}

// d of a stream of parts (the bytes at head, got of them, read already)
static int decode_mt(FILE* f, const char* in_name, Sink* s, const char* out_name, const U8* head, size_t got) {
  const int threads = got >= MT_HEAD ? head[6] : 0;
  char note[256] = "damaged data";
  MtArgs a;
  MtDec* d;
  uint y;
  int r;
  if (got > 5 && head[5] != magic[sizeof magic - 1]) {
    fprintf(stderr, "m4vpack: %s: format version %d, which this m4vpack (version %d) doesn't read\n", in_name, head[5],
            magic[sizeof magic - 1]);
    return 4;
  }
  if (threads < 1 || threads > 64) return run_end(4, note, f, in_name, s, out_name);
  a.threads = threads;
  a.stats = 0;
  a.part_max = mt_part_max((get_le(head + 7, 4) + 1) << 10);  // (the part size in KB, rounded down)
  if (!(d = coro_new<MtDec>())) no_memory();
  if (mt_init_failed(d->Init(threads, MT_RING, &a, 1), threads)) return 5;
  y = run_file(*d, f, s, head + MT_HEAD, got - MT_HEAD);
  if (!(r = mt_failed(*d, note, sizeof note)) && (y != Y_END || !mt_dealt(*d))) r = 4;
  d->Quit();  // (the threads joined)
  coro_free(d);
  return run_end(r, note, f, in_name, s, out_name);
}

// d of the stream in f into s: 0, or the exit status of an error (said)
static int decode_to(FILE* f, const char* in_name, Sink* s, const char* out_name) {
  U8 head[16];
  const size_t got = fread(head, 1, sizeof head, f);
  M4vCoro<1>* c;
  int r;
  if (got >= sizeof mt_magic && !memcmp(head, mt_magic, sizeof mt_magic))
    return decode_mt(f, in_name, s, out_name, head, got);
  if (!(c = coro_new<M4vCoro<1> >())) no_memory();
  c->Init(0);
  run_file(*c, f, s, head, got);
  r = run_end(c->status, c->note, f, in_name, s, out_name);
  coro_free(c);
  return r;
}

// c -v: d of the output, compared with the input
static int verify(FILE* in, const char* in_name, const char* out_name, int quiet) {
  Sink s;
  FILE* t = fopen(out_name, "rb");
  int r;
  if (!t) {
    fprintf(stderr, "m4vpack: can't open %s\n", out_name);
    return 3;
  }
  rewind(in);
  memset(&s, 0, sizeof(s));
  s.f = in;
  s.compare = 1;
  r = decode_to(t, out_name, &s, in_name);
  fclose(t);
  if (!r && !s.differs && fgetc(in) != EOF) {
    s.differs = 1;
    s.diff_at = s.pos;
  }
  if (r || s.differs) {
    fprintf(stderr, "verify: FAILED (%s at byte %.0f)\n", r ? "decoding error" : "difference", (double)s.diff_at);
    return 5;
  }
  if (!quiet) fprintf(stderr, "verify: OK\n");
  return 0;
}

// c on one model: its statistics into st, cost and orig
static int compress_one(FILE* in, const char* in_name, Sink* s, const char* out_name, const Opts* o, Stats* st,
                        double* cost, double* orig) {
  M4vCoro<0>* c;
  int r;
  if (!(c = coro_new<M4vCoro<0> >())) no_memory();
  c->Init(o->stats);
  run_file(*c, in, s);
  *st = c->st;
  memcpy(cost, c->co.cost, sizeof c->co.cost);
  memcpy(orig, c->co.orig, sizeof c->co.orig);
  r = run_end(c->status, c->note, in, in_name, s, out_name);
  coro_free(c);
  return r;
}

// c -t N: the stream in parts on N threads, its statistics the parts' added up
static int compress_mt(FILE* in, const char* in_name, Sink* s, const char* out_name, const Opts* o, Stats* st,
                       double* cost, double* orig) {
  char note[256] = "internal error: a thread stopped early";
  U8 head[MT_HEAD];
  MtArgs a;
  MtEnc* e;
  uint y, i;
  int r, k;
  a.threads = o->threads;
  a.stats = o->stats;
  a.part_max = 0;
  if (!(e = coro_new<MtEnc>())) no_memory();
  e->coro_init();
  e->M1.Init(o->part);
  if (mt_init_failed(e->M2.Init(o->threads, MT_RING, &a, 1), o->threads)) return 5;  // (started threads wait: they end with the process)
  mt_head(head, o->threads, o->part);
  sink_write(s, head, sizeof head);
  y = run_file(*e, in, s);
  if (!(r = mt_failed(e->M2, note, sizeof note)) && y != Y_END) r = 5;
  memset(st, 0, sizeof *st);
  memset(cost, 0, NCAT * sizeof *cost);
  memset(orig, 0, NCAT * sizeof *orig);
  for (i = 0; i < e->M2.th_N; i++) {
    const M4vPart<0>& w = e->M2.C1[i];
    st->vops += w.st.vops;
    st->packed += w.st.packed;
    for (k = 0; k < 5; k++) st->by_type[k] += w.st.by_type[k];
    st->raw_unsup += w.st.raw_unsup;
    st->raw_long += w.st.raw_long;
    st->raw_parse += w.st.raw_parse;
    st->raw_verify += w.st.raw_verify;
    st->raw_bytes += w.st.raw_bytes;
    st->vop_bytes += w.st.vop_bytes;
    for (k = 0; k < NCAT; k++) cost[k] += w.cost[k], orig[k] += w.orig[k];
  }
  st->in_bytes = (double)e->M1.total;  // (the parts have the headers copied in)
  st->parts = e->M1.parts;
  st->threads = o->threads;
  e->M2.Quit();  // (the threads joined, the workers freed)
  e->M1.Quit();
  coro_free(e);
  return run_end(r, note, in, in_name, s, out_name);
}

static int compress(const char* in_name, const char* out_name, const Opts* o) {
  FILE *in, *out;
  Sink s;
  Stats st;
  double cost[NCAT], orig[NCAT];
  int r;
  if (!(in = open_file(in_name, 0))) {
    fprintf(stderr, "m4vpack: can't open %s\n", in_name);
    return 2;
  }
  if (!(out = open_file(out_name, 1))) {
    fprintf(stderr, "m4vpack: can't create %s\n", out_name);
    return 3;
  }
  memset(&s, 0, sizeof(s));
  s.f = out;
  r = (o->threads > 1 ? compress_mt : compress_one)(in, in_name, &s, out_name, o, &st, cost, orig);
  if ((fflush(out) || (out != stdout && fclose(out))) && !r) {
    fprintf(stderr, "m4vpack: %s: write error\n", out_name);
    r = 3;
  }
  if (!r && !o->quiet) report(in_name, &st, cost, orig, (double)s.pos, o->stats);
  if (!r && o->verify) r = verify(in, in_name, out_name, o->quiet);
  if (in != stdin) fclose(in);
  return r;
}

static int decompress(const char* in_name, const char* out_name) {
  FILE *in, *out;
  Sink s;
  int r;
  if (!(in = open_file(in_name, 0))) {
    fprintf(stderr, "m4vpack: can't open %s\n", in_name);
    return 2;
  }
  if (!(out = open_file(out_name, 1))) {
    fprintf(stderr, "m4vpack: can't create %s\n", out_name);
    return 3;
  }
  memset(&s, 0, sizeof(s));
  s.f = out;
  r = decode_to(in, in_name, &s, out_name);
  if (!r && fflush(out)) r = (fprintf(stderr, "m4vpack: %s: write error\n", out_name), 3);
  if (out != stdout && fclose(out) && !r) r = (fprintf(stderr, "m4vpack: %s: write error\n", out_name), 3);
  if (in != stdin) fclose(in);
  return r;
}

static void usage(void) {
  fprintf(stderr, "m4vpack - lossless compressor for MPEG-4 Part 2 video streams (XviD, DivX 4+)\n"
                  "Usage: m4vpack c|d [options] input output\n"
                  "  c     compress a raw .m4v stream\n"
                  "  d     decompress\n"
                  "  -v    (c) decompress the result again and compare it with the input\n"
                  "  -s    (c) bits spent per kind of syntax element\n"
                  "  -q    no statistics\n"
                  "  -t N  (c) compress on N threads, 1 to 64 (default 1): the stream in parts that\n"
                  "        start at I-VOPs (where there are none, at others past 4 times the part\n"
                  "        size), a model for each; d takes N from the stream\n"
                  "  -c N  (c with -t) the size of a part, at least: N MB (1 to 1024), or N KB with\n"
                  "        a k (default 4)\n"
                  "input and output can be - for stdin and stdout (not with -v)\n");
}

int main(int argc, char** argv) {
  Opts o;
  int a;
  memset(&o, 0, sizeof(o));
  o.threads = 1;
  o.part = 4u << 20;
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    usage();
    return 1;
  }
  for (a = 2; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (!strcmp(argv[a], "-q")) o.quiet = 1;
    else if (!strcmp(argv[a], "-v")) o.verify = 1;
    else if (!strcmp(argv[a], "-s")) o.stats = 1;
    else if ((argv[a][1] == 't' || argv[a][1] == 'c') && (argv[a][2] || a + 1 < argc)) {
      // -t N, -c N (or -tN, -cN)
      const char opt = argv[a][1];
      const char* v = argv[a][2] ? argv[a] + 2 : argv[++a];
      char* e;
      const unsigned long n = strtoul(v, &e, 10);
      if (opt == 't') {
        if (*e || e == v || n < 1 || n > 64) {
          fprintf(stderr, "m4vpack: -t takes 1 to 64 threads\n");
          return 1;
        }
        o.threads = (int)n;
      } else {
        unsigned long unit = 1ul << 20;
        if (*e == 'k' || *e == 'K') unit = 1ul << 10, e++;
        if (*e || e == v || n < 1 || n > (1024ul << 20) / unit) {
          fprintf(stderr, "m4vpack: -c takes a part size of 1 to 1024 MB, or in KB with a k\n");
          return 1;
        }
        o.part = (U32)(n * unit);
      }
    } else {
      usage();
      return 1;
    }
  }
  if (a != argc - 2) {
    usage();
    return 1;
  }
  if (o.verify && (!strcmp(argv[a], "-") || !strcmp(argv[a + 1], "-"))) {
    fprintf(stderr, "m4vpack: -v reads the input and the output again: they can't be stdin or stdout\n");
    return 1;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  tables_init();
  crc_init();
  coder_tables_init();
  return argv[1][0] == 'c' ? compress(argv[a], argv[a + 1], &o) : decompress(argv[a], argv[a + 1]);
}
