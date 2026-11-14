// flacdump - lossless FLAC <-> (WAV + .meta) converter
//
//   flacdump c [options] input.flac out_prefix   -> out_prefix.wav + out_prefix.meta
//   flacdump d [options] inp_prefix restored.flac
//
// The .wav holds the decoded PCM, the .meta holds everything else needed to
// rebuild the original .flac bit-exactly (metadata blocks, encoder decisions
// such as block sizes, channel modes, predictors, rice parameters, plus any
// bytes that could not be modelled). The .meta is entropy coded with a binary
// arithmetic coder; most encoder decisions are predicted from the PCM itself
// (by re-running parts of the libFLAC / ffmpeg analysis), so usually only
// the deviations from the predictions cost space.
//
// Build: g++ -std=c++20 -O2 -fno-exceptions -fno-rtti -ffp-contract=off -o flacdump flacdump.cpp
// No STL is used; only the C runtime (stdio, stdlib, string, math).
// The code is in the *.inc files next to this file, included below in dependency
// order (one translation unit, see REFACTORING.md).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// The encoder-decision predictors replay floating point computations; both
// directions must get bit-identical results, so no contraction / x87 math.
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
#if (defined(__i386__) || defined(_M_IX86)) && !defined(__SSE2_MATH__) && !defined(_M_IX86_FP)
#error "build with SSE2 floating point math (e.g. -msse2 -mfpmath=sse)"
#endif

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

#ifdef _WIN32
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fseek64 fseeko
#define ftell64 ftello
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
static inline int clz64(u64 x) { unsigned long r; _BitScanReverse64(&r, x); return 63 - (int)r; }
static inline int ctz64(u64 x) { unsigned long r; _BitScanForward64(&r, x); return (int)r; }
#else
static inline int clz64(u64 x) { return __builtin_clzll(x); }
static inline int ctz64(u64 x) { return __builtin_ctzll(x); }
#endif

static int g_verbose = 0;

static void die(const char* msg) {
    fprintf(stderr, "flacdump: %s\n", msg);
    exit(1);
}

static void* xrealloc(void* q, size_t n) {
    void* p = realloc(q, n ? n : 1);
    if (!p) die("out of memory");
    return p;
}

template <class T> static inline T Min(T a, T b) { return a < b ? a : b; }
template <class T> static inline T Max(T a, T b) { return a > b ? a : b; }

static inline int ilog2_64(u64 x) { return 63 - clz64(x); }  // x>0
static inline int bitlen(u64 x) { return x ? 64 - clz64(x) : 0; }

static inline u32 rd_le32(const u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
static inline u32 rd_le16(const u8* p) { return p[0] | (p[1] << 8); }
static inline u32 rd_be24(const u8* p) { return (p[0] << 16) | (p[1] << 8) | p[2]; }
static inline u32 rd_be32(const u8* p) { return ((u32)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static inline u64 rd_be64(const u8* p) { return ((u64)rd_be32(p) << 32) | rd_be32(p + 4); }
static inline void wr_le32(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24); }
static inline void wr_le16(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); }
static inline void wr_be24(u8* p, u32 v) { p[0] = (u8)(v >> 16); p[1] = (u8)(v >> 8); p[2] = (u8)v; }
static inline void wr_be32(u8* p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }
static inline void wr_be64(u8* p, u64 v) { wr_be32(p, (u32)(v >> 32)); wr_be32(p + 4, (u32)v); }

// ---------------------------------------------------------------------------
// Basic containers, files, checksums

#include "buf.inc"
#include "vec.inc"
#include "file_crc.inc"
#include "md5.inc"

// ---------------------------------------------------------------------------
// Bit I/O

#include "bit_reader.inc"
#include "fix_out.inc"
#include "bit_writer.inc"

// ---------------------------------------------------------------------------
// FLAC frames

#include "flac_defs.inc"
#include "lpc_res_fast.inc"
#include "flac_frame.inc"

// ---------------------------------------------------------------------------
// WAV files

#include "wav_format.inc"
#include "wav_writer.inc"
#include "wav_reader.inc"

// ---------------------------------------------------------------------------
// Entropy coding: arithmetic coder, integers, CM byte model

#include "ac.inc"
#include "uint_model.inc"
#include "byte_cm.inc"
#include "code_bytes.inc"

// ---------------------------------------------------------------------------
// Embedded files, base64 text, ID3/APE tags

#include "embedded.inc"
#include "base64.inc"
#include "tag_spans.inc"
#include "regions.inc"

// ---------------------------------------------------------------------------
// Replication of the libFLAC / ffmpeg analysis (window -> autocorrelation ->
// Levinson-Durbin -> quantization, order and rice parameter estimates), used to
// predict encoder decisions from the PCM. Only basic IEEE operations, no
// contraction: compression and decompression compute bit-identical results.

#include "det_math.inc"
#include "estimators.inc"
#include "res_stats.inc"
#include "windows.inc"
#include "win_cache.inc"
#include "lpc_analysis.inc"
#include "ff_lpc.inc"
#include "lpc_predictor.inc"

// ---------------------------------------------------------------------------
// The meta model: FRAME segments

#include "meta_model.inc"
#include "model_residual.inc"
#include "lpc_order.inc"
#include "model_lpc.inc"
#include "encoder_sim.inc"
#include "model_frame.inc"

// ---------------------------------------------------------------------------
// Container and metadata blocks

#include "container.inc"
#include "frame_stats.inc"
#include "sink.inc"
#include "vorbis.inc"
#include "seektable.inc"
#include "metadata.inc"

// ---------------------------------------------------------------------------
// Decompression (wav + meta -> flac), compression (flac -> wav + meta)

#include "decompress.inc"
#include "compress.inc"

// ---------------------------------------------------------------------------
// main

static void usage() {
    fprintf(stderr,
            "flacdump - lossless FLAC <-> WAV+meta\n"
            "usage:\n"
            "  flacdump c [options] input.flac out_prefix   (writes out_prefix.wav, out_prefix.meta)\n"
            "  flacdump d [options] inp_prefix restored.flac\n"
            "options:\n"
            "  -v   verbose (print meta size breakdown)\n"
            "  -n   (c) skip the built-in restore verification\n"
            "  -i   (c) keep embedded files (cover art etc.) inside the .meta instead of\n"
            "       writing them to <prefix>.<n>.<ext>\n"
            "  -x   (c) also write <prefix>.side: an uncompressed dump of the side info\n"
            "       (headers, predictor/rice parameters, metadata) for comparisons\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 2) usage();
    init_crc();
    init_rates();
    init_squash();
    init_work();
    init_fast();
    Options opt;
    opt.verify = 1;
    opt.dump = 0;
    const char* mode = argv[1];
    const char* pos[2] = {nullptr, nullptr};
    int np = 0;
    for (int i = 2; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1]) {
            for (const char* p = argv[i] + 1; *p; p++) {
                if (*p == 'v')
                    g_verbose++;
                else if (*p == 'n')
                    opt.verify = 0;
                else if (*p == 'i')
                    g_ext.keep_inline = 1;
                else if (*p == 'x')
                    opt.dump = 1;
                else
                    usage();
            }
        } else if (np < 2)
            pos[np++] = argv[i];
        else
            usage();
    }
    if (np != 2) usage();
    if (!strcmp(mode, "c")) return do_compress(pos[0], pos[1], opt);
    if (!strcmp(mode, "d")) return do_decompress(pos[0], pos[1]);
    usage();
    return 1;
}
