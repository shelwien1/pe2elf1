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
// Classes and large functions live in the *.inc files next to this file; they
// are included at fixed positions below (one translation unit, see REFACTORING.md).

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

// ---------------------------------------------------------------------------
// Growable containers (no STL)

#include "buf.inc"

#include "vec.inc"

// ---------------------------------------------------------------------------
// File helpers

static bool read_file(const char* name, Buf& b, size_t pad) {
    FILE* f = fopen(name, "rb");
    if (!f) return false;
    fseek64(f, 0, SEEK_END);
    s64 sz = ftell64(f);
    fseek64(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return false;
    }
    b.reserve((size_t)sz + pad);
    size_t got = fread(b.p, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) return false;
    b.n = (size_t)sz;
    memset(b.p + b.n, 0, pad);
    return true;
}

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
// CRC-8 / CRC-16 as used by FLAC

static u8 g_crc8[256];
static u16 g_crc16[256];

static void init_crc() {
    for (int i = 0; i < 256; i++) {
        u32 c = i;
        for (int j = 0; j < 8; j++) c = (c & 0x80) ? ((c << 1) ^ 0x07) : (c << 1);
        g_crc8[i] = (u8)c;
        u32 d = i << 8;
        for (int j = 0; j < 8; j++) d = (d & 0x8000) ? ((d << 1) ^ 0x8005) : (d << 1);
        g_crc16[i] = (u16)d;
    }
}
static u8 crc8(const u8* p, size_t n) {
    u8 c = 0;
    for (size_t i = 0; i < n; i++) c = g_crc8[c ^ p[i]];
    return c;
}
static u16 crc16(const u8* p, size_t n) {
    u16 c = 0;
    for (size_t i = 0; i < n; i++) c = (u16)((c << 8) ^ g_crc16[(c >> 8) ^ p[i]]);
    return c;
}

// ---------------------------------------------------------------------------
// MD5 (RFC 1321)

#include "md5.inc"

// ---------------------------------------------------------------------------
// Bit reader / writer (MSB first)

#include "bit_reader.inc"

#include "fix_out.inc"
enum { FRAMEMAX = 8 << 20 };   // largest re-encoded frame (verbatim worst case is ~2.2 MB)
static u8 g_framebuf[FRAMEMAX];

#include "bit_writer.inc"

// ---------------------------------------------------------------------------
// FLAC structures

enum { MAXCH = 8, MAXBS = 65536, MAXPART = 1 << 15 };
enum { ST_CONST = 0, ST_VERB = 1, ST_FIXED = 2, ST_LPC = 3 };

struct SubInfo {
    int type, wasted, order, prec, shift, method, porder;
    s32 coef[32];
    u8* param;  // rice parameter per partition (escape code = 15 / 31)
    u8* raw;    // raw bits for escaped partitions
};

struct FrameInfo {
    int blocking, bscode, srcode, chcode, bpscode;
    u32 bs, srval;
    u64 number;
    int nch, bps;
    SubInfo sub[MAXCH];
};

struct StreamParams {
    u32 rate;
    int nch, bps;
};

static s64* g_sig[MAXCH];  // subframe signals (full, with wasted bits)
static s64* g_x;           // shifted subframe signal workspace
static s64* g_res;         // residual workspace
static s64* g_res2;
static s64* g_simx;
static s32* g_pcm[MAXCH];  // PCM of current frame
static u8* g_parambuf;

// fixed size work buffers (static storage, 8.5 MiB)
static s64 s_sig[MAXCH][MAXBS];
static s32 s_pcm[MAXCH][MAXBS];
static s64 s_x[MAXBS], s_res[MAXBS], s_res2[MAXBS], s_simx[MAXBS];
static u8 s_parambuf[(size_t)MAXCH * MAXPART * 2];

static void alloc_work() {
    for (int c = 0; c < MAXCH; c++) {
        g_sig[c] = s_sig[c];
        g_pcm[c] = s_pcm[c];
    }
    g_x = s_x;
    g_res = s_res;
    g_res2 = s_res2;
    g_simx = s_simx;
    g_parambuf = s_parambuf;
}

static void frameinfo_init(FrameInfo& f) {
    memset(&f, 0, sizeof(f));
    for (int c = 0; c < MAXCH; c++) {
        f.sub[c].param = g_parambuf + (size_t)c * MAXPART * 2;
        f.sub[c].raw = f.sub[c].param + MAXPART;
    }
}

static const int k_bps_tab[8] = {0, 8, 12, 0, 16, 20, 24, 32};
static const u32 k_sr_tab[12] = {0, 88200, 176400, 192000, 8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000};

static inline int side_channel(int chcode, int ch) {
    return (chcode == 8 && ch == 1) || (chcode == 9 && ch == 0) || (chcode == 10 && ch == 1);
}

static int canon_bscode(u32 bs) {
    if (bs == 192) return 1;
    for (int i = 2; i <= 5; i++)
        if (bs == (576u << (i - 2))) return i;
    for (int i = 8; i <= 15; i++)
        if (bs == (256u << (i - 8))) return i;
    return bs <= 256 ? 6 : 7;
}
static bool bscode_ok(int code, u32 bs) {
    if (code == 0) return false;
    if (code == 1) return bs == 192;
    if (code <= 5) return bs == (576u << (code - 2));
    if (code == 6) return bs >= 1 && bs <= 256;
    if (code == 7) return bs >= 1 && bs <= 65536;
    return bs == (256u << (code - 8));
}
static int canon_srcode(u32 rate) {
    for (int i = 1; i < 12; i++)
        if (k_sr_tab[i] == rate) return i;
    if (rate % 1000 == 0 && rate <= 255000) return 12;
    if (rate % 10 == 0 && rate <= 655350) return 14;
    if (rate <= 65535) return 13;
    return 0;
}
static int canon_bpscode(int bps) {
    for (int i = 1; i < 8; i++)
        if (k_bps_tab[i] == bps) return i;
    return 0;
}

// Fixed predictor residual: r[i-order] = x[i] - pred(x[i-1..i-order]) for i >= order
static void fixed_residual(const s64* x, int n, int order, s64* r) {
    switch (order) {
        case 0:
            for (int i = 0; i < n; i++) r[i] = x[i];
            break;
        case 1:
            for (int i = 1; i < n; i++) r[i - 1] = x[i] - x[i - 1];
            break;
        case 2:
            for (int i = 2; i < n; i++) r[i - 2] = x[i] - 2 * x[i - 1] + x[i - 2];
            break;
        case 3:
            for (int i = 3; i < n; i++) r[i - 3] = x[i] - 3 * x[i - 1] + 3 * x[i - 2] - x[i - 3];
            break;
        default:
            for (int i = 4; i < n; i++) r[i - 4] = x[i] - 4 * x[i - 1] + 6 * x[i - 2] - 4 * x[i - 3] + x[i - 4];
            break;
    }
}
static void fixed_restore(s64* x, int n, int order, const s64* r) {
    switch (order) {
        case 0:
            for (int i = 0; i < n; i++) x[i] = r[i];
            break;
        case 1:
            for (int i = 1; i < n; i++) x[i] = r[i - 1] + x[i - 1];
            break;
        case 2:
            for (int i = 2; i < n; i++) x[i] = r[i - 2] + 2 * x[i - 1] - x[i - 2];
            break;
        case 3:
            for (int i = 3; i < n; i++) x[i] = r[i - 3] + 3 * x[i - 1] - 3 * x[i - 2] + x[i - 3];
            break;
        default:
            for (int i = 4; i < n; i++) x[i] = r[i - 4] + 4 * x[i - 1] - 6 * x[i - 2] + 4 * x[i - 3] - x[i - 4];
            break;
    }
}
static void lpc_residual(const s64* x, int n, const s32* c, int order, int shift, s64* r) {
    for (int i = order; i < n; i++) {
        u64 s = 0;
        const s64* h = x + i;
        for (int j = 0; j < order; j++) s += (u64)((s64)c[j] * h[-1 - j]);
        r[i - order] = x[i] - ((s64)s >> shift);
    }
}
static void lpc_restore(s64* x, int n, const s32* c, int order, int shift, const s64* r) {
    for (int i = order; i < n; i++) {
        u64 s = 0;
        const s64* h = x + i;
        for (int j = 0; j < order; j++) s += (u64)((s64)c[j] * h[-1 - j]);
        x[i] = (s64)((u64)r[i - order] + (u64)((s64)s >> shift));
    }
}

#include "lpc_res_fast.inc"

// subframe signal of channel ch from PCM
static void subframe_signal(int chcode, int ch, s32** pcm, int n, s64* x) {
    const s32* L = pcm[0];
    const s32* R = pcm[1];
    if (chcode < 8) {
        const s32* p = pcm[ch];
        for (int i = 0; i < n; i++) x[i] = p[i];
    } else if (chcode == 8) {
        if (ch == 0)
            for (int i = 0; i < n; i++) x[i] = L[i];
        else
            for (int i = 0; i < n; i++) x[i] = (s64)L[i] - R[i];
    } else if (chcode == 9) {
        if (ch == 0)
            for (int i = 0; i < n; i++) x[i] = (s64)L[i] - R[i];
        else
            for (int i = 0; i < n; i++) x[i] = R[i];
    } else {
        if (ch == 0)
            for (int i = 0; i < n; i++) x[i] = ((s64)L[i] + R[i]) >> 1;
        else
            for (int i = 0; i < n; i++) x[i] = (s64)L[i] - R[i];
    }
}

// ---------------------------------------------------------------------------
// Frame parser / decoder. Returns frame length in bytes, 0 if not a valid frame.

static bool decode_residual(BitReader& br, int bs, int order, SubInfo& s, s64* res) {
    s.method = br.get(2);
    if (s.method > 1) return false;
    s.porder = br.get(4);
    int np = 1 << s.porder;
    u32 psz = (u32)bs >> s.porder;
    if ((psz << s.porder) != (u32)bs) return false;
    if (psz < (u32)order) return false;
    int pb = s.method ? 5 : 4, esc = s.method ? 31 : 15;
    s64* r = res;
    for (int p = 0; p < np; p++) {
        u32 n = psz - (p == 0 ? order : 0);
        int k = br.get(pb);
        s.param[p] = (u8)k;
        if (k == esc) {
            int rb = br.get(5);
            s.raw[p] = (u8)rb;
            if (br.pos + (u64)rb * n > br.nbits) return false;
            for (u32 i = 0; i < n; i++) *r++ = br.gets(rb);
        } else {
            s.raw[p] = 0;
            for (u32 i = 0; i < n; i++) {
                u64 q;
                if (!br.unary(q)) return false;
                if (q > (1ull << 40)) return false;
                u64 u = (q << k) | br.get(k);
                *r++ = (s64)(u >> 1) ^ -(s64)(u & 1);
                if (br.pos > br.nbits) return false;
            }
        }
    }
    return br.ok();
}

static size_t parse_header(const u8* d, size_t avail, const StreamParams& sp, FrameInfo& f) {
    if (avail < 6) return 0;
    if (d[0] != 0xFF || (d[1] & 0xFE) != 0xF8) return 0;
    f.blocking = d[1] & 1;
    f.bscode = d[2] >> 4;
    f.srcode = d[2] & 15;
    f.chcode = d[3] >> 4;
    f.bpscode = (d[3] >> 1) & 7;
    if (d[3] & 1) return 0;
    if (f.bscode == 0 || f.srcode == 15 || f.chcode > 10 || f.bpscode == 3) return 0;
    size_t p = 4;
    u32 b0 = d[p++];
    u64 v;
    int len;
    if (!(b0 & 0x80)) {
        v = b0;
        len = 0;
    } else if ((b0 & 0xE0) == 0xC0) {
        v = b0 & 0x1F;
        len = 1;
    } else if ((b0 & 0xF0) == 0xE0) {
        v = b0 & 0x0F;
        len = 2;
    } else if ((b0 & 0xF8) == 0xF0) {
        v = b0 & 0x07;
        len = 3;
    } else if ((b0 & 0xFC) == 0xF8) {
        v = b0 & 0x03;
        len = 4;
    } else if ((b0 & 0xFE) == 0xFC) {
        v = b0 & 0x01;
        len = 5;
    } else if (b0 == 0xFE) {
        v = 0;
        len = 6;
    } else
        return 0;
    if (p + len + 5 > avail) return 0;
    for (int i = 0; i < len; i++) {
        u32 b = d[p++];
        if ((b & 0xC0) != 0x80) return 0;
        v = (v << 6) | (b & 0x3F);
    }
    f.number = v;
    if (f.bscode == 1)
        f.bs = 192;
    else if (f.bscode <= 5)
        f.bs = 576u << (f.bscode - 2);
    else if (f.bscode == 6)
        f.bs = d[p++] + 1;
    else if (f.bscode == 7) {
        f.bs = ((d[p] << 8) | d[p + 1]) + 1;
        p += 2;
    } else
        f.bs = 256u << (f.bscode - 8);
    f.srval = 0;
    if (f.srcode == 12)
        f.srval = d[p++];
    else if (f.srcode == 13 || f.srcode == 14) {
        f.srval = (d[p] << 8) | d[p + 1];
        p += 2;
    }
    if (p >= avail) return 0;
    if (crc8(d, p) != d[p]) return 0;
    p++;
    f.nch = f.chcode < 8 ? f.chcode + 1 : 2;
    f.bps = f.bpscode ? k_bps_tab[f.bpscode] : sp.bps;
    if (f.nch != sp.nch || f.bps != sp.bps) return 0;
    if (f.bs > MAXBS - 1 && f.bs != MAXBS) return 0;
    return p;
}

static size_t parse_frame(const u8* d, size_t avail, const StreamParams& sp, FrameInfo& f, s32** pcm) {
    size_t hl = parse_header(d, avail, sp, f);
    if (!hl) return 0;
    BitReader br;
    br.init(d, avail);
    br.pos = hl * 8;
    int bs = (int)f.bs;
    for (int ch = 0; ch < f.nch; ch++) {
        SubInfo& s = f.sub[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        if (br.get(1)) return 0;
        int t = br.get(6);
        s.wasted = 0;
        if (br.get(1)) {
            u64 z;
            if (!br.unary(z)) return 0;
            if (z >= 40) return 0;
            s.wasted = (int)z + 1;
        }
        if (s.wasted >= sbps) return 0;
        int eb = sbps - s.wasted;
        s64* x = g_sig[ch];
        s.order = 0;
        if (t == 0) {
            s.type = ST_CONST;
            s64 v = br.gets(eb);
            for (int i = 0; i < bs; i++) x[i] = v;
        } else if (t == 1) {
            s.type = ST_VERB;
            if (br.pos + (u64)eb * bs > br.nbits) return 0;
            for (int i = 0; i < bs; i++) x[i] = br.gets(eb);
        } else if (t >= 8 && t <= 12) {
            s.type = ST_FIXED;
            s.order = t - 8;
            if (s.order > bs) return 0;
            for (int i = 0; i < s.order; i++) x[i] = br.gets(eb);
            if (!decode_residual(br, bs, s.order, s, g_res)) return 0;
            fixed_restore(x, bs, s.order, g_res);
        } else if (t >= 32) {
            s.type = ST_LPC;
            s.order = t - 31;
            if (s.order > bs) return 0;
            for (int i = 0; i < s.order; i++) x[i] = br.gets(eb);
            s.prec = br.get(4) + 1;
            if (s.prec == 16) return 0;
            s.shift = (int)br.gets(5);
            if (s.shift < 0) return 0;
            for (int i = 0; i < s.order; i++) s.coef[i] = (s32)br.gets(s.prec);
            if (!decode_residual(br, bs, s.order, s, g_res)) return 0;
            lpc_restore(x, bs, s.coef, s.order, s.shift, g_res);
        } else
            return 0;
        if (!br.ok()) return 0;
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] = (s64)((u64)x[i] << s.wasted);
    }
    br.pos = (br.pos + 7) & ~(u64)7;
    if (br.pos + 16 > br.nbits) return 0;
    size_t end = (size_t)(br.pos >> 3);
    u16 c = (u16)((d[end] << 8) | d[end + 1]);
    if (crc16(d, end) != c) return 0;
    // decorrelate + range check
    s64 lo = -(1ll << (f.bps - 1)), hi = (1ll << (f.bps - 1)) - 1;
    for (int ch = 0; ch < f.nch; ch++) {
        s32* o = pcm[ch];
        const s64* a = g_sig[0];
        const s64* b = g_sig[1];
        for (int i = 0; i < bs; i++) {
            s64 v;
            if (f.chcode < 8)
                v = g_sig[ch][i];
            else if (f.chcode == 8)
                v = ch == 0 ? a[i] : a[i] - b[i];
            else if (f.chcode == 9)
                v = ch == 0 ? a[i] + b[i] : b[i];
            else {
                s64 m = (s64)((u64)a[i] << 1) | (b[i] & 1);
                v = ch == 0 ? (m + b[i]) >> 1 : (m - b[i]) >> 1;
            }
            if (v < lo || v > hi) return 0;
            o[i] = (s32)v;
        }
    }
    return end + 2;
}

// ---------------------------------------------------------------------------
// Frame encoder (inverse of parse_frame given FrameInfo + PCM)

static void write_residual(BitWriter& bw, const SubInfo& s, const s64* r, int bs, int order) {
    bw.put(s.method, 2);
    bw.put(s.porder, 4);
    int np = 1 << s.porder;
    u32 psz = (u32)bs >> s.porder;
    int pb = s.method ? 5 : 4, esc = s.method ? 31 : 15;
    for (int p = 0; p < np; p++) {
        u32 n = psz - (p == 0 ? order : 0);
        int k = s.param[p];
        bw.put(k, pb);
        if (k == esc) {
            int rb = s.raw[p];
            bw.put(rb, 5);
            if (rb)
                for (u32 i = 0; i < n; i++) bw.puts(r[i], rb);
        } else {
            for (u32 i = 0; i < n; i++) {
                s64 v = r[i];
                u64 u = ((u64)v << 1) ^ (u64)(v >> 63);
                bw.rice(u, k);
            }
        }
        r += n;
    }
}

// returns false if the frame does not fit into the output storage
static bool encode_frame(const FrameInfo& f, s32** pcm, FixOut& out) {
    size_t start = out.n;
    u8 h[16];
    int p = 0;
    h[p++] = 0xFF;
    h[p++] = (u8)(0xF8 | f.blocking);
    h[p++] = (u8)((f.bscode << 4) | f.srcode);
    h[p++] = (u8)((f.chcode << 4) | (f.bpscode << 1));
    u64 v = f.number;
    if (v < 0x80)
        h[p++] = (u8)v;
    else {
        int nb = bitlen(v);
        int len = nb <= 11 ? 1 : nb <= 16 ? 2 : nb <= 21 ? 3 : nb <= 26 ? 4 : nb <= 31 ? 5 : 6;
        static const u8 lead[7] = {0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
        h[p++] = (u8)(lead[len] | (v >> (6 * len)));
        for (int i = len - 1; i >= 0; i--) h[p++] = (u8)(0x80 | ((v >> (6 * i)) & 0x3F));
    }
    if (f.bscode == 6)
        h[p++] = (u8)(f.bs - 1);
    else if (f.bscode == 7) {
        h[p++] = (u8)((f.bs - 1) >> 8);
        h[p++] = (u8)(f.bs - 1);
    }
    if (f.srcode == 12)
        h[p++] = (u8)f.srval;
    else if (f.srcode == 13 || f.srcode == 14) {
        h[p++] = (u8)(f.srval >> 8);
        h[p++] = (u8)f.srval;
    }
    h[p] = crc8(h, p);
    p++;
    out.append(h, p);
    BitWriter bw;
    bw.init(&out);
    int bs = (int)f.bs;
    for (int ch = 0; ch < f.nch; ch++) {
        const SubInfo& s = f.sub[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        s64* x = g_x;
        subframe_signal(f.chcode, ch, pcm, bs, x);
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] >>= s.wasted;
        int eb = sbps - s.wasted;
        int t = s.type == ST_CONST ? 0 : s.type == ST_VERB ? 1 : s.type == ST_FIXED ? 8 + s.order : 31 + s.order;
        bw.put(t << 1 | (s.wasted ? 1 : 0), 8);
        if (s.wasted) {
            bw.zeros(s.wasted - 1);
            bw.put(1, 1);
        }
        if (s.type == ST_CONST)
            bw.puts(x[0], eb);
        else if (s.type == ST_VERB)
            for (int i = 0; i < bs; i++) bw.puts(x[i], eb);
        else {
            for (int i = 0; i < s.order; i++) bw.puts(x[i], eb);
            if (s.type == ST_FIXED)
                fixed_residual(x, bs, s.order, g_res);
            else {
                bw.put(s.prec - 1, 4);
                bw.puts(s.shift, 5);
                for (int i = 0; i < s.order; i++) bw.puts(s.coef[i], s.prec);
                lpc_residual(x, bs, s.coef, s.order, s.shift, g_res);
            }
            write_residual(bw, s, g_res, bs, s.order);
        }
    }
    bw.align();
    u16 c = crc16(out.p + start, out.n - start);
    out.put((u8)(c >> 8));
    out.put((u8)c);
    return !out.over;
}

// ---------------------------------------------------------------------------
// Binary arithmetic coder (carry-less range coder) + adaptive bit models

#include "ac.inc"

// Adaptive Elias-gamma style integer model
struct UModel {
    BM len[48];
    BM hi[48][4];
};
static u64 code_uint(AC& ac, UModel& m, u64 v) {
    u64 w = v + 1;
    int L = ac.dec ? 0 : ilog2_64(w);
    int i = 0;
    for (;; i++) {
        int b = ac.bit(m.len[Min(i, 47)], i < L);
        if (!b) break;
        if (i >= 63) break;
    }
    L = i;
    if (L == 0) return 0;
    u64 r = 1;
    int k = L;
    // first up to 2 mantissa bits modelled
    int ctx = 1;
    for (int j = 0; j < 2 && k > 0; j++) {
        k--;
        int b = ac.bit(m.hi[Min(L, 47)][ctx], (int)((w >> k) & 1));
        ctx = (ctx << 1 | b) & 3;
        if (j == 0) ctx = 2 + b;
        r = (r << 1) | (u64)b;
    }
    if (k > 0) r = (r << k) | ac.raw(w, k);
    return r - 1;
}
static s64 code_sint(AC& ac, UModel& m, BM& sign, s64 v) {
    // zero/sign/magnitude
    u64 mag = ac.dec ? 0 : (u64)(v < 0 ? -v : v);
    int neg = v < 0;
    u64 r = code_uint(ac, m, mag);
    if (r) {
        neg = ac.bit(sign, neg);
        return neg ? -(s64)r : (s64)r;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Order-2 context mixing byte model (for metadata text, junk, tags)

#include "byte_cm.inc"

static const char* k_prime_text =
    "reference libFLAC 1.2.1 20070917reference libFLAC 1.3.0 20130526reference libFLAC 1.3.1 20141125"
    "reference libFLAC 1.3.2 20170101reference libFLAC 1.3.3 20190804reference libFLAC 1.3.4 20220220"
    "reference libFLAC 1.4.0 20220909reference libFLAC 1.4.1 20220922reference libFLAC 1.4.2 20221022"
    "reference libFLAC 1.4.3 20230623reference libFLAC 1.5.0 20250211reference libFLAC 1.1.4 20070213"
    "reference libFLAC 1.1.2 20050205reference libFLAC 1.2.0 20070715Lavf58.76.100Lavf60.16.100Lavf61.7.100"
    "TITLE=ARTIST=ALBUM=DATE=TRACKNUMBER=TRACKTOTAL=DISCNUMBER=GENRE=COMMENT=ALBUMARTIST=COMPOSER="
    "ENCODER=REPLAYGAIN_TRACK_GAIN=REPLAYGAIN_TRACK_PEAK=REPLAYGAIN_ALBUM_GAIN=REPLAYGAIN_ALBUM_PEAK=";

// Code a byte string given as pieces (concatenation, total length known to both sides).
// Chooses CM or stored. Pieces avoid copying the bytes around extracted parts of a region.
struct Piece {
    u8* p;
    size_t n;
};
static void code_pieces(AC& ac, const Piece* pc, int np, bool prime) {
    size_t len = 0;
    for (int k = 0; k < np; k++) len += pc[k].n;
    if (len == 0) return;
    int savecat = ac.cat;
    int stored = 0;
    ByteCM cm;
    cm.init();
    if (prime) {
        const char* s = k_prime_text;
        for (size_t i = 0; s[i]; i++) cm.byte(nullptr, (u8)s[i], nullptr);
        cm.c1 = cm.c2 = cm.c3 = 0;
    }
    if (!ac.dec) {
        // measure CM cost on a copy of the model
        if (len > 64) {
            ByteCM t;
            t.init();
            if (prime) {
                memcpy(t.t0, cm.t0, 256 * 2);
                memcpy(t.t1, cm.t1, 65536 * 2);
                memcpy(t.t2, cm.t2, ByteCM::H2 * 2);
                memcpy(t.t3, cm.t3, ByteCM::H3 * 2);
                memcpy(t.w, cm.w, sizeof(cm.w));
            }
            double cost = 0;
            for (int k = 0; k < np; k++)
                for (size_t i = 0; i < pc[k].n; i++) t.byte(nullptr, pc[k].p[i], &cost);
            t.release();
            stored = cost > (double)len * 8;
        }
    }
    stored = ac.code(stored, 32768);
    if (stored) ac.cat = C_RAW;
    for (int k = 0; k < np; k++) {
        u8* d = pc[k].p;
        for (size_t i = 0; i < pc[k].n; i++) d[i] = stored ? (u8)ac.raw(d[i], 8) : (u8)cm.byte(&ac, d[i], nullptr);
    }
    cm.release();
    ac.cat = savecat;
}
static void code_bytes(AC& ac, u8* data, size_t len, bool prime) {
    Piece pc = {data, len};
    code_pieces(ac, &pc, 1, prime);
}

// ---------------------------------------------------------------------------
// Embedded files (cover art etc.): stored as separate files <prefix>.<n>.<ext>
// instead of inside the .meta. The .meta keeps their position, size, type and
// a 16-bit CRC; the bytes around them are coded normally.

struct ExtFiles {
    const char* prefix;  // file name prefix (out_prefix / inp_prefix)
    int count;           // files referenced so far (numbering, same order in c and d)
    int keep_inline;     // -i: do not extract
    u64 bytes;           // total extracted bytes
};
static ExtFiles g_ext = {nullptr, 0, 0, 0};

static const char* k_ext_name[16] = {"bin", "jpg", "png", "gif", "bmp", "webp", "tif", "pdf",
                                     "zip", "gz",  "xz",  "7z",  "ogg", "flac", "txt",  "bin"};
static inline bool ext_is_image(int e) { return e >= 1 && e <= 6; }

static int sniff_ext(const u8* p, size_t n) {
    if (n >= 3 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF) return 1;
    if (n >= 8 && !memcmp(p, "\x89PNG\r\n\x1a\n", 8)) return 2;
    if (n >= 6 && (!memcmp(p, "GIF87a", 6) || !memcmp(p, "GIF89a", 6))) return 3;
    if (n >= 2 && p[0] == 'B' && p[1] == 'M') return 4;
    if (n >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WEBP", 4)) return 5;
    if (n >= 4 && (!memcmp(p, "II*\0", 4) || !memcmp(p, "MM\0*", 4))) return 6;
    if (n >= 5 && !memcmp(p, "%PDF-", 5)) return 7;
    if (n >= 4 && !memcmp(p, "PK\3\4", 4)) return 8;
    if (n >= 3 && p[0] == 0x1F && p[1] == 0x8B && p[2] == 8) return 9;
    if (n >= 6 && !memcmp(p, "\xFD" "7zXZ\0", 6)) return 10;
    if (n >= 6 && !memcmp(p, "7z\xBC\xAF\x27\x1C", 6)) return 11;
    if (n >= 4 && !memcmp(p, "OggS", 4)) return 12;
    if (n >= 4 && !memcmp(p, "fLaC", 4)) return 13;
    if (n > 0) {
        size_t i = 0;
        while (i < n && ((p[i] >= 0x20 && p[i] < 0x7F) || p[i] == '\t' || p[i] == '\r' || p[i] == '\n')) i++;
        if (i == n) return 14;
    }
    return 0;
}

// length of a JPEG starting at p (0 if not a well-formed JPEG)
static size_t jpeg_len(const u8* p, size_t n) {
    if (n < 4 || p[0] != 0xFF || p[1] != 0xD8 || p[2] != 0xFF) return 0;
    size_t i = 2;
    while (i + 1 < n) {
        if (p[i] != 0xFF) return 0;
        while (i + 1 < n && p[i + 1] == 0xFF) i++;  // fill bytes
        if (i + 1 >= n) return 0;
        u8 m = p[i + 1];
        i += 2;
        if (m == 0xD9) return i;  // EOI
        if ((m >= 0xD0 && m <= 0xD7) || m == 0x01) continue;
        if (m == 0x00 || i + 2 > n) return 0;
        size_t sl = (p[i] << 8) | p[i + 1];
        if (sl < 2 || i + sl > n) return 0;
        i += sl;
        if (m == 0xDA) {  // entropy coded data until the next marker
            while (i + 1 < n) {
                if (p[i] == 0xFF && p[i + 1] != 0x00 && !(p[i + 1] >= 0xD0 && p[i + 1] <= 0xD7)) break;
                i++;
            }
        }
    }
    return 0;
}

static size_t png_len(const u8* p, size_t n) {
    if (n < 8 || memcmp(p, "\x89PNG\r\n\x1a\n", 8)) return 0;
    size_t i = 8;
    while (i + 12 <= n) {
        u32 cl = rd_be32(p + i);
        if (cl > n || i + 12 + (size_t)cl > n) return 0;
        bool iend = !memcmp(p + i + 4, "IEND", 4);
        i += 12 + cl;
        if (iend) return i;
    }
    return 0;
}

// ---- base64 (pictures/binary embedded as text: comments, APE/ID3 tags) ----
static const char k_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static size_t b64_len(size_t n, int pad) { return pad ? (n + 2) / 3 * 4 : (n * 4 + 2) / 3; }
// base64 text layout: padding, optional line wrapping
struct B64Fmt {
    int pad;
    u32 line;       // chars per line, 0 = single line
    int crlf;       // line break is CR LF (else LF)
    int final_eol;  // text ends with a line break
};
// decoded values larger than this stay text (bounds the static buffers below)
enum { B64MAX = 16 << 20, B64TEXTMAX = B64MAX / 3 * 4 + B64MAX / 8 };
static u8 g_b64scan[B64MAX];   // encoder: candidate values while searching
static u8 g_b64dec_v[B64MAX];  // decoded VORBIS_COMMENT value being coded
static u8 g_b64dec_r[B64MAX];  // decoded value of a cut in a raw region being coded
static u8 g_b64narrow[B64TEXTMAX];  // UTF-16 text narrowed to bytes

static size_t b64f_len(size_t n, const B64Fmt& f) {
    size_t t = b64_len(n, f.pad);
    if (!f.line || !t) return t;
    size_t lines = (t + f.line - 1) / f.line;
    return t + (lines - 1 + f.final_eol) * (f.crlf ? 2 : 1);
}
// generate the text of d[0..n) in layout f, one character at a time
template <class F> static void b64f_gen(const u8* d, size_t n, const B64Fmt& f, F&& put) {
    size_t t = b64_len(n, f.pad), col = 0, done = 0;
    auto emit = [&](u8 c) {
        put(c);
        done++;
        if (f.line && ++col == f.line && (done < t || f.final_eol)) {
            col = 0;
            if (f.crlf) put('\r');
            put('\n');
        }
    };
    size_t i = 0;
    for (; i + 3 <= n; i += 3) {
        u32 v = (d[i] << 16) | (d[i + 1] << 8) | d[i + 2];
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        emit(k_b64[(v >> 6) & 63]);
        emit(k_b64[v & 63]);
    }
    if (n - i == 1) {
        u32 v = d[i] << 16;
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        if (f.pad) {
            emit('=');
            emit('=');
        }
    } else if (n - i == 2) {
        u32 v = (d[i] << 16) | (d[i + 1] << 8);
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        emit(k_b64[(v >> 6) & 63]);
        if (f.pad) emit('=');
    }
    if (f.line && f.final_eol && col) {  // last line shorter than f.line
        if (f.crlf) put('\r');
        put('\n');
    }
}
static void b64f_encode(const u8* d, size_t n, const B64Fmt& f, u8* o) {
    b64f_gen(d, n, f, [&](u8 c) { *o++ = c; });
}
// UTF-16 text (wide = 1 little endian, 2 big endian)
static void b64f_encode_wide(const u8* d, size_t n, const B64Fmt& f, u8* o, int wide) {
    b64f_gen(d, n, f, [&](u8 c) {
        o[wide == 2] = c;
        o[wide == 1] = 0;
        o += 2;
    });
}
// strict parse incl. line wrapping into out[0..cap); true only if b64f_gen reproduces the text exactly
static bool b64f_parse(const u8* s, size_t m, u8* out, size_t cap, size_t& on, B64Fmt& f) {
    static s8 rev[256];
    if (!rev['B']) {
        for (int i = 0; i < 256; i++) rev[i] = -1;
        for (int i = 0; i < 64; i++) rev[(u8)k_b64[i]] = (s8)i;
    }
    on = 0;
    f.line = 0;
    f.crlf = 0;
    f.final_eol = 0;
    size_t e = 0;
    while (e < m && s[e] != '\r' && s[e] != '\n') e++;
    if (e < m) {
        if (e < 4) return false;
        f.line = (u32)e;
        f.crlf = s[e] == '\r';
        f.final_eol = s[m - 1] == '\n';
    }
    // characters without line breaks, trailing padding
    size_t chars = 0, np = 0;
    for (size_t i = 0; i < m; i++)
        if (s[i] != '\r' && s[i] != '\n') chars++;
    for (size_t i = m; i > 0 && np < 2;) {
        i--;
        if (s[i] == '\r' || s[i] == '\n') continue;
        if (s[i] != '=') break;
        np++;
    }
    if (!chars) return false;
    f.pad = np > 0 || (chars % 4) == 0;
    size_t q = chars - np;
    if ((np && (chars % 4)) || (q % 4) == 1 || q / 4 * 3 + 2 > cap) return false;
    u32 acc = 0;
    int bits = 0;
    size_t seen = 0;
    for (size_t i = 0; i < m && seen < q; i++) {
        if (s[i] == '\r' || s[i] == '\n') continue;
        int v = rev[s[i]];
        if (v < 0) return false;
        seen++;
        acc = (acc << 6) | (u32)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[on++] = (u8)(acc >> bits);
        }
    }
    if (b64f_len(on, f) != m) return false;
    size_t k = 0;
    bool same = true;
    b64f_gen(out, on, f, [&](u8 c) {
        if (same && (k >= m || s[k] != c)) same = false;
        k++;
    });
    return same && k == m;
}
// estimated CM cost (bits) of a byte string with a fresh model
static double cm_cost(const u8* d, size_t n) {
    ByteCM t;
    t.init();
    double c = 0;
    for (size_t i = 0; i < n; i++) t.byte(nullptr, d[i], &c);
    t.release();
    return c;
}

struct Cut {
    u64 off, len;
    int ext;
    int b64;     // region bytes are base64 text of 'dn' decoded bytes
    int wide;    // for b64: text is UTF-16 (1 = little endian, 2 = big endian)
    int kind;    // for b64: 1 PICTURE structure, 2 image, 3 other binary
    B64Fmt fmt;
    size_t dn;
};
enum { MAXCUTS = 64, MINCUT = 256 };

static int find_cuts(const u8* d, size_t n, int kind, Cut* cuts, bool extract);

// should decoded base64 data replace its text? (sets kind)
static bool b64_worth(const u8* dec, size_t dn, const B64Fmt& f, bool extract, int& kind) {
    static Cut tmp[MAXCUTS];
    if (find_cuts(dec, dn, 1, tmp, false))
        kind = 1;
    else if (ext_is_image(sniff_ext(dec, dn)) && dn >= MINCUT)
        kind = 2;
    else
        kind = 3;
    if (extract && dn >= MINCUT) return true;
    // the base64 text itself costs ~6 bits per character
    return cm_cost(dec, dn) + 24 < 6.0 * (double)b64_len(dn, f.pad);
}

static inline bool is_b64c(u8 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/';
}

// try to take d[s..e) as base64 text; also without trailing NULs / line breaks and after "KEY="
static bool b64_try(const u8* d, size_t s, size_t e, bool extract, Cut& c) {
    c.b64 = 0;
    while (e > s && d[e - 1] == 0) e--;
    size_t st = s;
    while (st < e && (d[st] == '\r' || d[st] == '\n')) st++;
    for (int pass = 0; pass < 2; pass++) {
        if (pass) {  // "KEY=base64": start after the last '=' followed by base64 characters
            size_t k = e;
            while (k > st && !(d[k - 1] == '=' && k < e && is_b64c(d[k]))) k--;
            if (k <= st) break;
            st = k;
        }
        if (e - st < 64) continue;
        bool hex = true;
        for (size_t k = st; k < e && hex; k++)
            hex = (d[k] >= '0' && d[k] <= '9') || (d[k] >= 'a' && d[k] <= 'f') || (d[k] >= 'A' && d[k] <= 'F') ||
                  d[k] == '\r' || d[k] == '\n';
        if (hex) continue;
        for (int t = 0; t < 3; t++) {
            size_t en = e;
            for (int u = 0; u < t && en > st && (d[en - 1] == '\r' || d[en - 1] == '\n'); u++) en--;
            if (t && en == e) break;
            if (!b64f_parse(d + st, en - st, g_b64scan, B64MAX, c.dn, c.fmt)) continue;
            if (!b64_worth(g_b64scan, c.dn, c.fmt, extract, c.kind)) continue;
            c.b64 = 1;
            c.wide = 0;
            c.off = st;
            c.len = en - st;
            c.ext = 0;
            return true;
        }
    }
    return false;
}

// same for UTF-16 text (wide = 1 little endian, 2 big endian): characters above 0xFF break the text
static bool b64_try_wide(const u8* d, size_t s, size_t e, int wide, bool extract, Cut& c) {
    size_t nch = (e - s) / 2;
    c.b64 = 0;
    if (nch < 64 || nch > B64TEXTMAX) return false;
    for (size_t k = 0; k < nch; k++) {
        u8 lo = d[s + 2 * k + (wide == 2)], hi = d[s + 2 * k + (wide == 1)];
        g_b64narrow[k] = hi ? 0xFF : lo;
    }
    bool ok = b64_try(g_b64narrow, 0, nch, extract, c);
    if (ok) {
        c.wide = wide;
        c.off = s + 2 * c.off;
        c.len = 2 * c.len;
    }
    return ok;
}

// base64 text run in an unparsed raw region starting at d[i]; returns run length to skip
static size_t find_b64_run(const u8* d, size_t n, size_t i, bool extract, Cut& c) {
    size_t j = i;
    while (j < n && (is_b64c(d[j]) || d[j] == '=' || d[j] == '\r' || d[j] == '\n')) j++;
    size_t run = j - i;
    c.b64 = 0;
    if (run < 64) return run ? run : 1;
    for (size_t s0 = i; s0 < i + 4 && s0 + 64 <= j; s0++)
        if (is_b64c(d[s0]) && b64_try(d, s0, j, extract, c)) break;
    return run;
}

// ---- tag structures: exact spans of text values (ID3v2 text/comment frames, APEv2 text items) ----
struct Span {
    size_t off, len;
    int type;  // 0 text, 1 UTF-16LE text, 2 UTF-16BE text, 3 binary object
};
struct SpanList {
    Span s[512];
    int n;
    Span tag[16];  // parsed tag structures (no raw base64 scanning inside)
    int nt;
};
static void add_span(SpanList& L, size_t off, size_t len, int type = 0) {
    if (L.n < 512 && len >= (type == 3 ? 16u : 64u)) L.s[L.n++] = {off, len, type};
}
static inline u32 syncsafe32(const u8* p) { return ((p[0] & 0x7F) << 21) | ((p[1] & 0x7F) << 14) | ((p[2] & 0x7F) << 7) | (p[3] & 0x7F); }

// position after an ID3 string terminated by 0 (encodings 0, 3) or 00 00 (UTF-16, encodings 1, 2)
static size_t id3_skip_str(const u8* d, size_t v, size_t ve, int enc) {
    if (v >= ve) return ve;
    if (enc == 1 || enc == 2) {
        size_t k = v;
        while (k + 1 < ve && (d[k] || d[k + 1])) k += 2;
        return Min(k + 2, ve);
    }
    while (v < ve && d[v]) v++;
    return Min(v + 1, ve);
}

// ID3v2 tag at d[p]: collects text values (any encoding), GEOB objects and APIC pictures;
// returns tag size (0 if not a tag)
static size_t id3_spans(const u8* d, size_t n, size_t p, SpanList& L) {
    if (p + 10 > n || memcmp(d + p, "ID3", 3)) return 0;
    int ver = d[p + 3], fl = d[p + 5];
    if (ver < 2 || ver > 4 || d[p + 4] == 0xFF || (d[p + 6] | d[p + 7] | d[p + 8] | d[p + 9]) & 0x80) return 0;
    size_t size = 10 + syncsafe32(d + p + 6) + ((ver == 4 && (fl & 0x10)) ? 10 : 0);
    if (p + size > n) return 0;
    if (fl & 0x80) return size;  // unsynchronised: values are not plain text
    size_t q = p + 10, end = p + 10 + syncsafe32(d + p + 6);
    if (fl & 0x40) {  // extended header
        if (q + 4 > end) return size;
        q += ver == 4 ? syncsafe32(d + q) : rd_be32(d + q) + 4;
    }
    while (q + (ver == 2 ? 6 : 10) <= end) {
        const u8* h = d + q;
        if (!h[0]) break;  // padding
        size_t fs, hl;
        int fflags = 0;
        bool txxx, text, comm;
        if (ver == 2) {
            fs = rd_be24(h + 3);
            hl = 6;
            txxx = !memcmp(h, "TXX", 3);
            comm = !memcmp(h, "COM", 3);
        } else {
            fs = ver == 4 ? syncsafe32(h + 4) : rd_be32(h + 4);
            fflags = (h[8] << 8) | h[9];
            hl = 10;
            txxx = !memcmp(h, "TXXX", 4);
            comm = !memcmp(h, "COMM", 4);
        }
        text = h[0] == 'T';
        bool geob = ver == 2 ? !memcmp(h, "GEO", 3) : !memcmp(h, "GEOB", 4);
        bool apic = ver == 2 ? !memcmp(h, "PIC", 3) : !memcmp(h, "APIC", 4);
        if (q + hl + fs > end) break;
        size_t v = q + hl, ve = q + hl + fs;
        bool plain = ver == 2 || !(ver == 3 ? (fflags & 0xC0) : (fflags & 0x0F));
        int enc = fs > 1 ? d[v] : -1;
        if (plain && enc >= 0 && enc <= 3) {
            v++;
            if (text || comm) {
                if (comm) v += 3;
                if (txxx || comm) v = id3_skip_str(d, v, ve, enc);  // description
                int type = 0;
                if (enc == 1 || enc == 2) {  // UTF-16: BOM decides the byte order (enc 2: big endian)
                    type = enc == 2 ? 2 : 1;
                    if (v + 2 <= ve && d[v] == 0xFF && d[v + 1] == 0xFE) {
                        type = 1;
                        v += 2;
                    } else if (v + 2 <= ve && d[v] == 0xFE && d[v + 1] == 0xFF) {
                        type = 2;
                        v += 2;
                    }
                    if ((ve - v) & 1) ve--;
                }
                if (v < ve) add_span(L, v, ve - v, type);
            } else if (geob) {  // MIME, filename, description, object
                v = id3_skip_str(d, v, ve, 0);
                v = id3_skip_str(d, v, ve, enc);
                v = id3_skip_str(d, v, ve, enc);
                if (v < ve) add_span(L, v, ve - v, 3);
            } else if (apic) {  // MIME (v2.2: 3 char format), picture type, description, data
                v = ver == 2 ? v + 3 : id3_skip_str(d, v, ve, 0);
                v = id3_skip_str(d, v + 1, ve, enc);
                if (v < ve) add_span(L, v, ve - v, 3);
            }
        }
        q = ve;
    }
    if (L.nt < 16) L.tag[L.nt++] = {p, size};
    return size;
}

// APEv2 tag whose header or footer starts at d[p] ("APETAGEX"); returns 1 if parsed
static bool ape_spans(const u8* d, size_t n, size_t p, SpanList& L) {
    if (p + 32 > n || memcmp(d + p, "APETAGEX", 8)) return false;
    u32 size = rd_le32(d + p + 12), cnt = rd_le32(d + p + 16), fl = rd_le32(d + p + 20);
    size_t items, end;
    if (fl & (1u << 29)) {  // header: items follow
        items = p + 32;
        end = items + size - 32;
    } else {  // footer: items precede
        if (size < 32 || size > p + 32) return false;
        items = p + 32 - size;
        end = p;
    }
    if (end > n || items > end) return false;
    size_t q = items;
    for (u32 i = 0; i < cnt && q + 9 <= end; i++) {
        u32 vl = rd_le32(d + q), ifl = rd_le32(d + q + 4);
        size_t k = q + 8;
        while (k < end && d[k]) k++;
        size_t v = k + 1;
        if (v + vl > end) break;
        int it = (ifl >> 1) & 3;
        if (it == 0)
            add_span(L, v, vl);  // UTF-8 text item
        else if (it == 1) {      // binary item, by convention "filename\0data"
            size_t z = v;
            while (z < v + vl && z < v + 256 && d[z]) z++;
            if (z < v + vl && z < v + 256 && !d[z])
                add_span(L, z + 1, v + vl - z - 1, 3);
            else
                add_span(L, v, vl, 3);
        }
        q = v + vl;
    }
    if (L.nt < 16) L.tag[L.nt++] = {items, end - items};
    return true;
}

static bool in_tag(const SpanList& L, size_t i) {
    for (int k = 0; k < L.nt; k++)
        if (i >= L.tag[k].off && i < L.tag[k].off + L.tag[k].len) return true;
    return false;
}

// find embedded files: kind 1 = FLAC PICTURE block (exact), 2 = whole buffer, otherwise scan for
// JPEG/PNG (if extracting) and base64 text
static int find_cuts(const u8* d, size_t n, int kind, Cut* cuts, bool extract) {
    int nc = 0;
    if (kind == 2) {  // the whole buffer is one file (e.g. base64-decoded cover art)
        if (n < MINCUT) return 0;
        cuts[0].off = 0;
        cuts[0].len = n;
        cuts[0].ext = sniff_ext(d, n);
        cuts[0].b64 = 0;
        return 1;
    }
    if (kind == 1) {
        // type, mime, description, w, h, depth, colors, data_len, data
        size_t p = 4;
        for (int k = 0; k < 2; k++) {
            if (p + 4 > n) return 0;
            u32 l = rd_be32(d + p);
            if ((u64)p + 4 + l > n) return 0;
            p += 4 + l;
        }
        p += 16;
        if (p + 4 > n) return 0;
        u32 dl = rd_be32(d + p);
        p += 4;
        if ((u64)p + dl != n || dl < MINCUT) return 0;
        cuts[0].off = p;
        cuts[0].len = dl;
        cuts[0].ext = sniff_ext(d + p, dl);
        cuts[0].b64 = 0;
        return 1;
    }
    // text values of ID3v2 / APEv2 tags found in the region
    static SpanList L;
    L.n = L.nt = 0;
    for (size_t i = 0; i + 10 <= n; i++) {
        if (d[i] == 'I' && d[i + 1] == 'D' && d[i + 2] == '3') {
            size_t t = id3_spans(d, n, i, L);
            if (t) i += t - 1;
        } else if (d[i] == 'A' && d[i + 1] == 'P' && !memcmp(d + i, "APETAGEX", 8))
            ape_spans(d, n, i, L);
    }
    for (int k = 0; k < L.n; k++)  // spans are sorted unless an APE footer preceded its items
        for (int m = k; m > 0 && L.s[m - 1].off > L.s[m].off; m--) {
            Span t = L.s[m];
            L.s[m] = L.s[m - 1];
            L.s[m - 1] = t;
        }
    int si = 0;
    for (size_t i = 0; i + 8 <= n && nc < MAXCUTS;) {
        while (si < L.n && L.s[si].off < i) si++;
        if (si < L.n && L.s[si].off == i) {  // a tag value
            Span sp = L.s[si++];
            Cut& c = cuts[nc];
            bool done = false;
            size_t se = sp.off + sp.len;
            if (sp.type == 0)
                done = b64_try(d, sp.off, se, extract, c);
            else if (sp.type <= 2)
                done = b64_try_wide(d, sp.off, se, sp.type, extract, c);
            else {
                // binary object: base64 text after a short binary header (e.g. Serato GEOB), else a file
                for (size_t k = 0; k < 8 && !done && sp.off + k < se; k++)
                    if (is_b64c(d[sp.off + k])) done = b64_try(d, sp.off + k, se, extract, c);
                if (!done && extract && sp.len >= MINCUT) {
                    c.off = sp.off;
                    c.len = sp.len;
                    c.ext = sniff_ext(d + sp.off, sp.len);
                    c.b64 = 0;
                    done = true;
                }
            }
            if (done) {
                i = c.off + c.len;
                nc++;
                continue;
            }
        }
        size_t l = 0;
        if (extract && d[i] == 0xFF && d[i + 1] == 0xD8 && d[i + 2] == 0xFF)
            l = jpeg_len(d + i, n - i);
        else if (extract && d[i] == 0x89 && d[i + 1] == 'P')
            l = png_len(d + i, n - i);
        if (l >= MINCUT) {
            cuts[nc].off = i;
            cuts[nc].len = l;
            cuts[nc].ext = sniff_ext(d + i, l);
            cuts[nc].b64 = 0;
            nc++;
            i += l;
        } else if (is_b64c(d[i]) && !in_tag(L, i)) {
            size_t r = find_b64_run(d, n, i, extract, cuts[nc]);
            if (cuts[nc].b64) {
                i = cuts[nc].off + cuts[nc].len;
                nc++;
            } else
                i += r;
        } else
            i++;
    }
    return nc;
}

static void ext_name(char* buf, size_t bn, int num, int ext) {
    snprintf(buf, bn, "%s.%d.%s", g_ext.prefix, num, k_ext_name[ext & 15]);
}

struct ExtModels {
    UModel n, off, len, line, dlen;
    BM ext[16];
    BM isb64, pad, crlf, fin;
    BM kind[4];
    BM wide[4];
};
static ExtModels g_extm;

static bool code_region(AC& ac, u8* data, size_t len, int kind, bool prime);

// one cut: an external file, or base64 text replaced by its decoded data (coded recursively)
static bool code_cut(AC& ac, u8* data, Cut& c) {
    c.b64 = ac.bit(g_extm.isb64, c.b64);
    if (c.b64) {
        size_t dn = 0;
        if (!ac.dec) {  // decode again (the search phase does not keep decoded data)
            const u8* t = data + c.off;
            size_t tl = (size_t)c.len;
            if (c.wide) {
                tl /= 2;
                for (size_t k = 0; k < tl; k++) g_b64narrow[k] = data[c.off + 2 * k + (c.wide == 2)];
                t = g_b64narrow;
            }
            B64Fmt f2;
            if (!b64f_parse(t, tl, g_b64dec_r, B64MAX, dn, f2) || dn != c.dn) die("internal: base64 cut");
        }
        c.fmt.pad = ac.bit(g_extm.pad, c.fmt.pad);
        c.fmt.line = (u32)code_uint(ac, g_extm.line, c.fmt.line);
        if (c.fmt.line) {
            c.fmt.crlf = ac.bit(g_extm.crlf, c.fmt.crlf);
            c.fmt.final_eol = ac.bit(g_extm.fin, c.fmt.final_eol);
        } else
            c.fmt.crlf = c.fmt.final_eol = 0;
        c.kind = (int)ac.tree(g_extm.kind, 2, (u32)c.kind);
        c.wide = (int)ac.tree(g_extm.wide, 2, (u32)c.wide);
        dn = (size_t)code_uint(ac, g_extm.dlen, dn);
        size_t tl = b64f_len(dn, c.fmt);
        if (c.kind == 0 || c.wide == 3 || dn > B64MAX || tl * (c.wide ? 2 : 1) != c.len) return false;
        if (!code_region(ac, g_b64dec_r, dn, c.kind == 1 ? 1 : 2, false)) return false;
        if (ac.dec) {
            if (!c.wide)
                b64f_encode(g_b64dec_r, dn, c.fmt, data + c.off);
            else
                b64f_encode_wide(g_b64dec_r, dn, c.fmt, data + c.off, c.wide);
        }
        return true;
    }
    c.ext = (int)ac.tree(g_extm.ext, 4, (u32)c.ext);
    u16 crc = ac.dec ? 0 : crc16(data + c.off, (size_t)c.len);
    u16 want = (u16)ac.raw(crc, 16);
    char name[4200];
    ext_name(name, sizeof(name), ++g_ext.count, c.ext);
    if (!ac.dec) {
        FILE* f = fopen(name, "wb");
        if (!f || fwrite(data + c.off, 1, (size_t)c.len, f) != c.len || fclose(f)) die("cannot write embedded file");
        g_ext.bytes += c.len;
        return true;
    }
    FILE* f = fopen(name, "rb");
    if (!f) {
        fprintf(stderr, "flacdump: cannot open embedded file %s\n", name);
        return false;
    }
    size_t got = fread(data + c.off, 1, (size_t)c.len, f);
    int extra = fgetc(f);
    fclose(f);
    if (got != c.len || extra != EOF || crc16(data + c.off, (size_t)c.len) != want) {
        fprintf(stderr, "flacdump: embedded file %s does not match the .meta\n", name);
        return false;
    }
    return true;
}

// cut lists come from a static pool; regions nest at most twice (region -> decoded base64 value)
enum { CUT_DEPTH = 3 };
static Cut g_cutpool[CUT_DEPTH][MAXCUTS];
static int g_cutdepth;

// Code a byte region whose length is known on both sides, extracting embedded files
// and decoding base64 text.
static bool code_region(AC& ac, u8* data, size_t len, int kind, bool prime) {
    if (g_cutdepth >= CUT_DEPTH) die("internal: regions nested too deep");
    Cut* cuts = g_cutpool[g_cutdepth++];
    memset(cuts, 0, sizeof(Cut) * MAXCUTS);
    int nc = 0;
    bool extract = !g_ext.keep_inline && g_ext.prefix;
    if (!ac.dec && (extract || kind == 0)) nc = find_cuts(data, len, kind, cuts, extract);
    int savecat = ac.cat;
    ac.cat = C_META;
    nc = (int)code_uint(ac, g_extm.n, (u64)nc);
    bool ok = nc <= MAXCUTS;
    u64 pos = 0;
    for (int i = 0; i < nc && ok; i++) {
        Cut& c = cuts[i];
        u64 gap = code_uint(ac, g_extm.off, ac.dec ? 0 : c.off - pos);
        c.off = pos + gap;
        c.len = code_uint(ac, g_extm.len, c.len);
        if (c.off + c.len > len || !code_cut(ac, data, c)) ok = false;
        pos = c.off + c.len;
    }
    ac.cat = savecat;
    if (ok) {  // the remaining bytes: the gaps between the cuts
        Piece pc[MAXCUTS + 1];
        u64 p0 = 0;
        for (int i = 0; i < nc; i++) {
            pc[i] = {data + p0, (size_t)(cuts[i].off - p0)};
            p0 = cuts[i].off + cuts[i].len;
        }
        pc[nc] = {data + p0, (size_t)(len - p0)};
        code_pieces(ac, pc, nc + 1, prime);
    }
    g_cutdepth--;
    return ok;
}

// ---------------------------------------------------------------------------
// LPC / rice estimation helpers used as predictors for the meta model

// libFLAC fixed order guess (data starts at index 4), two variants of tie rule
static void fixed_guess(const s64* x, int n, int& g_new, int& g_old, u64* esum = nullptr) {
    if (n <= 4) {
        g_new = g_old = 0;
        if (esum) memset(esum, 0, sizeof(u64) * 5);
        return;
    }
    u64 e0 = 0, e1 = 0, e2 = 0, e3 = 0, e4 = 0;
    for (int i = 4; i < n; i++) {
        s64 a = x[i], b = x[i - 1], c = x[i - 2], d = x[i - 3], e = x[i - 4];
        s64 r0 = a, r1 = a - b, r2 = a - 2 * b + c, r3 = a - 3 * b + 3 * c - d, r4 = a - 4 * b + 6 * c - 4 * d + e;
        e0 += (u64)(r0 < 0 ? -r0 : r0);
        e1 += (u64)(r1 < 0 ? -r1 : r1);
        e2 += (u64)(r2 < 0 ? -r2 : r2);
        e3 += (u64)(r3 < 0 ? -r3 : r3);
        e4 += (u64)(r4 < 0 ? -r4 : r4);
    }
    if (esum) {
        esum[0] = e0;
        esum[1] = e1;
        esum[2] = e2;
        esum[3] = e3;
        esum[4] = e4;
    }
    if (e0 <= Min(Min(Min(e1, e2), e3), e4))
        g_new = 0;
    else if (e1 <= Min(Min(e2, e3), e4))
        g_new = 1;
    else if (e2 <= Min(e3, e4))
        g_new = 2;
    else if (e3 <= e4)
        g_new = 3;
    else
        g_new = 4;
    if (e0 < Min(Min(Min(e1, e2), e3), e4))
        g_old = 0;
    else if (e1 < Min(Min(e2, e3), e4))
        g_old = 1;
    else if (e2 < Min(e3, e4))
        g_old = 2;
    else if (e3 < e4)
        g_old = 3;
    else
        g_old = 4;
}

static inline int rice_est_old(u64 mean, u32 ps) {
    int r = 0;
    u64 k = ps;
    while (k < mean) {
        r++;
        k <<= 1;
    }
    return r;
}
static inline int rice_est_new(u64 mean, u32 ps) {
    u32 div = 0x40000 / ps;
    if (mean < 2) return 0;
    u64 t = ((mean - 1) * (u64)div) >> 18;
    if (t == 0) return 0;
    return ilog2_64(t) + 1;
}
static inline int rice_est_ff(u64 usum, u32 n, int maxk) {
    if (usum <= (n >> 1)) return 0;
    u64 s2 = usum - (n >> 1);
    u64 q = s2 / n;
    if (q > 0x7FFFFFFF) q = 0x7FFFFFFF;
    int k = q ? ilog2_64(q) : 0;
    return Min(k, maxk);
}
static inline u64 lf_bits(int k, u32 ps, u64 sum) {
    u64 b = 4 + (u64)(1 + k) * ps + (k ? (sum >> (k - 1)) : (sum << 1)) - (ps >> 1);
    return b > 0xFFFFFFFFull ? 0xFFFFFFFFull : b;
}

static double det_log2(double x);

// libFLAC <= 1.3 "suggested" rice parameter for partition order 0
static int k0_from_bps(double bps, int lim) {
    int k = (bps > 0.0 ? (int)(u32)(bps + 0.5) : 0) + 1;
    return k > lim ? lim : k;
}
// libFLAC <= 1.3.1 scaled the LPC error by ln(2)^2 when estimating bits
static int g_ln2sq = 0;
static inline double err_scale(int total) {
    return g_ln2sq ? 0.5 * 0.69314718055994530942 * 0.69314718055994530942 / (double)total : 0.5 / (double)total;
}
static double lpc_expected_bps(double err, int total) {
    if (err > 0.0) {
        double b = 0.5 * det_log2(err_scale(total) * err);
        return b >= 0.0 ? b : 0.0;
    }
    return err < 0.0 ? 1e32 : 0.0;
}
static float fixed_rbps(u64 e, int len) {
    return e ? (float)det_log2(0.69314718055994530942 * (double)e / (double)len) : 0.0f;
}

#include "res_stats.inc"

// ---------------------------------------------------------------------------
// Replication of the libFLAC LPC analysis (window -> autocorrelation ->
// Levinson-Durbin -> quantization). Used to predict the LPC coefficients from
// the PCM. Everything here uses only basic IEEE operations (no libm
// transcendental functions, no contraction) so that compression and
// decompression compute bit-identical results on any platform.

// cos(x) for |x| <= ~8, fdlibm-style kernel; deterministic
static double det_cos(double x) {
    const double invpio2 = 6.36619772367581382433e-01;
    const double pio2_1 = 1.57079632673412561417e+00;
    const double pio2_1t = 6.07710050650619224932e-11;
    double ax = x < 0 ? -x : x;
    int n = (int)(ax * invpio2 + 0.5);
    double fn = (double)n;
    double r = ax - fn * pio2_1;
    double w = fn * pio2_1t;
    double y = r - w;
    double yl = (r - y) - w;
    double z = y * y;
    double res;
    if ((n & 1) == 0) {  // cos(y)
        const double C1 = 4.16666666666666019037e-02, C2 = -1.38888888888741095749e-03,
                     C3 = 2.48015872894767294178e-05, C4 = -2.75573143513906633035e-07,
                     C5 = 2.08757232129817482790e-09, C6 = -1.13596475577881948265e-11;
        double rr = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
        res = 1.0 - (0.5 * z - (z * rr - y * yl));
    } else {  // sin(y)
        const double S1 = -1.66666666666666324348e-01, S2 = 8.33333333332248946124e-03,
                     S3 = -1.98412698298579493134e-04, S4 = 2.75573137070700676789e-06,
                     S5 = -2.50507602534068634195e-08, S6 = 1.58969099521155010221e-10;
        double v = z * y;
        double rr = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
        res = y - ((z * (0.5 * yl - v * rr) - yl) - v * S1);
    }
    switch (n & 3) {
        case 0: return res;
        case 1: return -res;
        case 2: return -res;
        default: return res;
    }
}
static inline float det_cosf(float x) { return (float)det_cos((double)x); }

// glibc (>= 2.28) cosf for 0 <= y < 120, emulated with plain double arithmetic
static float glibc_cosf(float y) {
    static const double T[2][13] = {
        {1.0, -1.0, -1.0, 1.0, 0x1.45F306DC9C883p+23, 0x1.921FB54442D18p0, 0x1p0, -0x1.ffffffd0c621cp-2,
         0x1.55553e1068f19p-5, -0x1.6c087e89a359dp-10, 0x1.99343027bf8c3p-16, -0x1.555545995a603p-3,
         0x1.1107605230bc4p-7},
        {1.0, -1.0, -1.0, 1.0, 0x1.45F306DC9C883p+23, 0x1.921FB54442D18p0, -0x1p0, 0x1.ffffffd0c621cp-2,
         -0x1.55553e1068f19p-5, 0x1.6c087e89a359dp-10, -0x1.99343027bf8c3p-16, -0x1.555545995a603p-3,
         0x1.1107605230bc4p-7}};
    static const double S3 = -0x1.994eb3774cf24p-13;
    u32 yi;
    memcpy(&yi, &y, 4);
    u32 top = (yi >> 20) & 0x7ff;
    const float pio4 = 0x1.921FB6p-1f;
    u32 pi4i;
    memcpy(&pi4i, &pio4, 4);
    double x = y;
    int n;
    const double* p = T[0];
    if (top < ((pi4i >> 20) & 0x7ff)) {
        if (top < 0x398) return 1.0f;  // abstop12(0x1p-12f)
        n = 1;
    } else {
        double r = x * p[4];
        n = ((s32)r + 0x800000) >> 24;
        x = x - n * p[5];
        double sg = p[n & 3];
        if (n & 2) p = T[1];
        x = x * sg;
        n ^= 1;
    }
    double x2 = x * x;
    if ((n & 1) == 0) {
        double x3 = x * x2;
        double s1 = p[12] + x2 * S3;
        double x7 = x3 * x2;
        double sv = x + x3 * p[11];
        return (float)(sv + x7 * s1);
    }
    double x4 = x2 * x2;
    double c2 = p[9] + x2 * p[10];
    double c1 = p[6] + x2 * p[7];
    double x6 = x4 * x2;
    double c = c1 + x4 * p[8];
    return (float)(c + x6 * c2);
}

static const double k_pi = 3.14159265358979323846;

// window variants: v14 = libFLAC >= 1.3 (cosf), v12 = libFLAC <= 1.2 (cos)
// 0.5 - 0.5*cos(pi*i/N): libFLAC <= 1.3.3 uses double cos, >= 1.3.4 cosf
static inline float hann_tap(int i, int N, int v12) {
    if (v12 == 1) return (float)(0.5 - 0.5 * det_cos(k_pi * i / N));
    if (v12 == 2) return 0.5f - 0.5f * glibc_cosf((float)(k_pi * i / N));
    return 0.5f - 0.5f * det_cosf((float)(k_pi * i / N));
}

static void win_tukey(float* w, int L, float p, int v12) {
    for (int n = 0; n < L; n++) w[n] = 1.0f;
    if (p <= 0.0f) return;
    int Np = (int)(p / 2.0f * (float)L) - 1;
    if (Np > 0) {
        for (int n = 0; n <= Np; n++) {
            w[n] = hann_tap(n, Np, v12);
            w[L - Np - 1 + n] = hann_tap(n + Np, Np, v12);
        }
    }
}
static void win_partial_tukey(float* w, int L, float p, float start, float end, int v12) {
    int start_n = (int)(start * (float)L);
    int end_n = (int)(end * (float)L);
    int N = end_n - start_n;
    int Np = (int)(p / 2.0f * (float)N);
    int n, i;
    for (n = 0; n < start_n && n < L; n++) w[n] = 0.0f;
    for (i = 1; n < (start_n + Np) && n < L; n++, i++) w[n] = hann_tap(i, Np, v12);
    for (; n < (end_n - Np) && n < L; n++) w[n] = 1.0f;
    for (i = Np; n < end_n && n < L; n++, i--) w[n] = hann_tap(i, Np, v12);
    for (; n < L; n++) w[n] = 0.0f;
}
static void win_punchout_tukey(float* w, int L, float p, float start, float end, int v12) {
    int start_n = (int)(start * (float)L);
    int end_n = (int)(end * (float)L);
    int Ns = (int)(p / 2.0f * (float)start_n);
    int Ne = (int)(p / 2.0f * (float)(L - end_n));
    int n, i;
    for (n = 0, i = 1; n < Ns && n < L; n++, i++) w[n] = hann_tap(i, Ns, v12);
    for (; n < start_n - Ns && n < L; n++) w[n] = 1.0f;
    for (i = Ns; n < start_n && n < L; n++, i--) w[n] = hann_tap(i, Ns, v12);
    for (; n < end_n && n < L; n++) w[n] = 0.0f;
    for (i = 1; n < end_n + Ne && n < L; n++, i++) w[n] = hann_tap(i, Ne, v12);
    for (; n < L - (Ne) && n < L; n++) w[n] = 1.0f;
    for (i = Ne; n < L; n++, i--) w[n] = hann_tap(i, Ne, v12);
}

enum {
    W_T50,     // tukey(0.5), >= 1.3.4 (cosf)
    W_T50_12,  // tukey(0.5), <= 1.3.3 (cos)
    W_T25,     // subdivide_tukey(2) root
    W_T16,     // subdivide_tukey(3) root
    W_PT2_0,   // partial_tukey(2), cosf
    W_PT2_1,
    W_PO3_0,  // punchout_tukey(3), cosf
    W_PO3_1,
    W_PO3_2,
    W_PT2C_0,  // same, double cos (<= 1.3.3)
    W_PT2C_1,
    W_PO3C_0,
    W_PO3C_1,
    W_PO3C_2,
    W_T50G,  // glibc cosf variants (libFLAC >= 1.3.4 on Linux)
    W_T25G,
    W_T16G,
    W_PT2G_0,
    W_PT2G_1,
    W_PO3G_0,
    W_PO3G_1,
    W_PO3G_2,
    NWIN
};
// windows that differ only numerically map to the same base window
static int base_win(int w) {
    switch (w) {
        case W_T50_12: case W_T50G: return W_T50;
        case W_T25G: return W_T25;
        case W_T16G: return W_T16;
        case W_PT2C_0: case W_PT2G_0: return W_PT2_0;
        case W_PT2C_1: case W_PT2G_1: return W_PT2_1;
        case W_PO3C_0: case W_PO3G_0: return W_PO3_0;
        case W_PO3C_1: case W_PO3G_1: return W_PO3_1;
        case W_PO3C_2: case W_PO3G_2: return W_PO3_2;
        default: return w;
    }
}

#include "win_cache.inc"

// autocorrelation variants
enum { AC_DBL = 0, AC_FASC, AC_FNEW8, AC_FNEW12, AC_FNEW16, AC_X87, AC_FFMA, AC_DDESC, NAC };

static void autoc_compute(const float* d, int n, int lags, int mode, double* out) {
    if (mode == AC_DDESC) {
        // libFLAC 1.4 SSE2 / NEON intrinsics: samples processed from the end
        double ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0;
        for (int i = n - 1; i >= 0; i--) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * (double)d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_DBL) {
        double ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0;
        for (int i = 0; i < n; i++) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * (double)d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_FASC) {
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            float di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_FFMA) {
        // generic C code compiled with fused multiply-add (e.g. arm64)
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            float di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] = fmaf(di, d[i + c], ac[c]);
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_X87) {
        // x87 asm (libFLAC <= 1.3 ia32), precision control = double
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] = (float)(di * (double)d[i + c] + (double)ac[c]);
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else {
        int N = mode == AC_FNEW8 ? 8 : mode == AC_FNEW12 ? 12 : 16;
        float ac[16];
        for (int c = 0; c < N; c++) ac[c] = 0.0f;
        int limit = n - N;
        int i;
        for (i = 0; i <= limit; i++) {
            float di = d[i];
            for (int c = 0; c < N; c++) ac[c] += d[i + c] * di;
        }
        limit++;
        if (limit < 0) limit = 0;
        for (i = n - 1; i >= limit; i--) {
            float di = d[i];
            for (int c = 0; c < N && i + c < n; c++) ac[c] += di * d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = c < N ? ac[c] : 0.0;
    }
}

// libFLAC FLAC__lpc_compute_lp_coefficients: all orders 1..max_order.
// lp[i*32 + j] = coefficient j of order i+1. Returns the usable max order.
// fm: emulate a build with fused multiply-add contraction.
static int levinson_all(const double* autoc, int max_order, float* lp, double* error, bool fm) {
    double err = autoc[0], lpc[32];
    if (err == 0.0) return 0;
    for (int i = 0; i < max_order; i++) {
        double r = -autoc[i + 1];
        int j;
        if (fm) {
            for (j = 0; j < i; j++) r = fma(-lpc[j], autoc[i - j], r);
        } else {
            for (j = 0; j < i; j++) r -= lpc[j] * autoc[i - j];
        }
        r /= err;
        lpc[i] = r;
        if (fm) {
            for (j = 0; j < (i >> 1); j++) {
                double tmp = lpc[j];
                lpc[j] = fma(r, lpc[i - 1 - j], lpc[j]);
                lpc[i - 1 - j] = fma(r, tmp, lpc[i - 1 - j]);
            }
            if (i & 1) lpc[j] = fma(lpc[j], r, lpc[j]);
            err *= fma(-r, r, 1.0);
        } else {
            for (j = 0; j < (i >> 1); j++) {
                double tmp = lpc[j];
                lpc[j] += r * lpc[i - 1 - j];
                lpc[i - 1 - j] += r * tmp;
            }
            if (i & 1) lpc[j] += lpc[j] * r;
            err *= (1.0 - r * r);
        }
        for (j = 0; j <= i; j++) lp[i * 32 + j] = (float)(-lpc[j]);
        error[i] = err;
        if (err == 0.0) return i + 1;
    }
    return max_order;
}

// deterministic log2 (frexp + atanh series), x > 0
static double det_log2(double x) {
    int e;
    double m = frexp(x, &e);  // [0.5,1)
    if (m < 0.70710678118654752440) {
        m *= 2.0;
        e--;
    }
    double t = (m - 1.0) / (m + 1.0), t2 = t * t, p = t, sum = 0.0;
    for (int k = 1; k <= 31; k += 2) {
        sum += p / k;
        p *= t2;
    }
    return (double)e + 2.0 * sum * 1.44269504088896340736;
}

// FLAC__lpc_compute_best_order
static int lpc_best_order(const double* lpc_error, int max_order, int total_samples, int overhead) {
    double error_scale = err_scale(total_samples);
    int best = 0;
    double best_bits = 4294967295.0;
    for (int i = 0; i < max_order; i++) {
        int order = i + 1;
        double e = lpc_error[i], bps;
        if (e > 0.0) {
            bps = 0.5 * det_log2(error_scale * e);
            if (bps < 0.0) bps = 0.0;
        } else if (e < 0.0)
            bps = 1e32;
        else
            bps = 0.0;
        double bits = bps * (double)(total_samples - order) + (double)(order * overhead);
        if (bits < best_bits) {
            best = i;
            best_bits = bits;
        }
    }
    return best + 1;
}

// libFLAC FLAC__lpc_quantize_coefficients
static bool lpc_quantize(const float* lp, int order, int precision, s32* q, int* shift) {
    precision--;
    s32 qmax = 1 << precision, qmin = -qmax;
    qmax--;
    double cmax = 0.0;
    for (int i = 0; i < order; i++) {
        double d = fabs((double)lp[i]);
        if (d > cmax) cmax = d;
    }
    if (cmax <= 0.0) return false;
    int log2cmax;
    (void)frexp(cmax, &log2cmax);
    log2cmax--;
    int sh = precision - log2cmax - 1;
    if (sh > 15)
        sh = 15;
    else if (sh < -16)
        return false;
    double error = 0.0;
    if (sh >= 0) {
        float mul = (float)(1 << sh);
        for (int i = 0; i < order; i++) {
            error += (double)(lp[i] * mul);
            s32 v = (s32)lround(error);
            if (v > qmax)
                v = qmax;
            else if (v < qmin)
                v = qmin;
            error -= v;
            q[i] = v;
        }
    } else {
        float div = (float)(1 << -sh);
        for (int i = 0; i < order; i++) {
            error += (double)(lp[i] / div);
            s32 v = (s32)lround(error);
            if (v > qmax)
                v = qmax;
            else if (v < qmin)
                v = qmin;
            error -= v;
            q[i] = v;
        }
        sh = 0;
    }
    *shift = sh;
    return true;
}

// quantize at a given shift (used as a guess when no candidate matches)
static void lpc_quantize_at(const float* lp, int order, int precision, int sh, s32* q) {
    s32 qmax = (1 << (precision - 1)) - 1, qmin = -(1 << (precision - 1));
    double error = 0.0;
    float mul = (float)(1 << sh);
    for (int i = 0; i < order; i++) {
        error += (double)(lp[i] * mul);
        s32 v = (s32)lround(error);
        if (v > qmax)
            v = qmax;
        else if (v < qmin)
            v = qmin;
        error -= v;
        q[i] = v;
    }
}

// ---- ffmpeg flacenc (levinson) analysis ----
static void ff_welch(const s64* x, int len, double* w) {
    if (len == 1) {
        w[0] = 0.0;
        return;
    }
    int n2 = len >> 1;
    double c = 2.0 / (len - 1.0);
    if (len & 1) {
        for (int i = 0; i < n2; i++) {
            double ww = c - i - 1.0;
            ww = 1.0 - (ww * ww);
            w[i] = (double)x[i] * ww;
            w[len - 1 - i] = (double)x[len - 1 - i] * ww;
        }
        w[n2] = 0.0;
        return;
    }
    for (int i = 0; i < n2; i++) {
        double ww = c - n2 + i;
        ww = 1.0 - (ww * ww);
        w[n2 - i - 1] = (double)x[n2 - i - 1] * ww;
        w[n2 + i] = (double)x[n2 + i] * ww;
    }
}
// d must have zero padding at d[-1] and d[len]
static void ff_autocorr(const double* d, int len, int lag, double* autoc) {
    int i, j;
    for (j = 0; j < lag; j += 2) {
        double sum0 = 1.0, sum1 = 1.0;
        for (i = j; i < len; i++) {
            sum0 += d[i] * d[i - j];
            sum1 += d[i] * d[i - j - 1];
        }
        autoc[j] = sum0;
        autoc[j + 1] = sum1;
    }
    if (j == lag) {
        double sum = 1.0;
        for (i = j - 1; i < len; i += 2) sum += d[i] * d[i - j] + d[i + 1] * d[i - j + 1];
        autoc[j] = sum;
    }
}
// compute_lpc_coefs(normalize=1, fail=0); lpc[i*32+j]
static void ff_levinson(const double* autoc, int max_order, double* lpc) {
    double err = autoc[0];
    const double* ac = autoc + 1;
    for (int i = 0; i < max_order; i++) {
        double r = -ac[i];
        const double* last = lpc + (i ? (i - 1) * 32 : 0);
        double* cur = lpc + i * 32;
        for (int j = 0; j < i; j++) r -= last[j] * ac[i - j - 1];
        if (err) r /= err;
        err *= 1.0 - (r * r);
        cur[i] = r;
        for (int j = 0; j < (i + 1) >> 1; j++) {
            double f = last[j], b = last[i - 1 - j];
            cur[j] = f + r * b;
            cur[i - 1 - j] = b + r * f;
        }
    }
}
static void ff_quantize(const double* lpc_in0, int order, int precision, s32* out, int* shift, int forced_sh) {
    double lpc_in[32];
    memcpy(lpc_in, lpc_in0, sizeof(double) * order);
    s32 qmax = (1 << (precision - 1)) - 1;
    double cmax = 0.0;
    for (int i = 0; i < order; i++) cmax = Max(cmax, fabs(lpc_in[i]));
    int sh;
    if (forced_sh < 0) {
        if (cmax * (1 << 15) < 1.0) {
            *shift = 0;
            memset(out, 0, sizeof(s32) * order);
            return;
        }
        sh = 15;
        while ((cmax * (1 << sh) > qmax) && (sh > 0)) sh--;
        if (sh == 0 && cmax > qmax) {
            double scale = ((double)qmax) / cmax;
            for (int i = 0; i < order; i++) lpc_in[i] *= scale;
        }
    } else
        sh = forced_sh;
    double error = 0;
    for (int i = 0; i < order; i++) {
        error -= lpc_in[i] * (1 << sh);
        long v = lrintf((float)error);
        if (v > qmax) v = qmax;
        if (v < -qmax) v = -qmax;
        out[i] = (s32)v;
        error -= out[i];
    }
    *shift = sh;
}
static int ff_est_order(const double* lpc, int min_order, int max_order) {
    int est = min_order;
    for (int i = max_order - 1; i >= min_order - 1; i--)
        if (fabs(lpc[i * 32 + i]) > 0.10) {
            est = i + 1;
            break;
        }
    return est;
}

#include "lpc_predictor.inc"

// ---------------------------------------------------------------------------
// The meta model

struct ChState {
    int ptype, pfixed, plorder, pprec, pshift, pmethod, pporder, prank, psim;
    int pcoef_n;
    s32 pcoef[32];
};

struct Models {
    BM seg[5][4];
    BM blocking[2];
    BM bs_eq[3];
    BM bsval[1 << 16];
    BM bscode_eq[16];
    BM bscode_t[16][16];
    BM sr_eq[2];
    BM sr_t[16];
    BM srv_eq[2];
    BM ch_t[16][4][4][16];
    BM ch_sim[2][2];
    BM ch_ff[2][2];
    BM bpsc_eq[2];
    BM bpsc_t[8];
    BM num_eq[2];
    BM w_eq[2][4];
    UModel w_val;
    BM type_t[2][5][5][4];
    BM forder_t[5][2][6][8];
    BM lorder_t[33][32];
    BM prec_t[16][16];
    BM shift_t[33][32];
    UModel coef_m[32];
    BM coef_s[32];
    BM cand_f[24][4];
    BM cand_g[24][4];
    BM lpc_sim[2][2];
    BM method_b[3][2];
    BM po_eqL[4];
    BM po_eqF[4];
    BM po_t[16][16];
    BM k_eq0[2];
    BM k_eq1[8];
    BM k_eq2[8];
    BM k_eq3[8];
    BM k_t[32][32];
    BM esc_t[33][32];
    // misc
    UModel len_m;
    UModel num_m;
    UModel clen_m;
    BM vendor_t[32];
    BM b64kind[4];
    BM mflag[16];
    BM mtype[8][128];
};

// priors: "prediction was right" flags start out confident
static void prior(BM* m, size_t cnt, int p1) {
    for (size_t i = 0; i < cnt; i++) {
        m[i].p = (u16)p1;
        m[i].n = 4;
    }
}
#define PRIOR(arr, p1) prior((BM*)&(arr), sizeof(arr) / sizeof(BM), p1)
static void models_init(Models& M) {
    model_init(M);
    const int HI = 62000, LO = 3500;
    PRIOR(M.blocking, LO);
    PRIOR(M.bs_eq, HI);
    PRIOR(M.bscode_eq, HI);
    PRIOR(M.sr_eq, HI);
    PRIOR(M.srv_eq, HI);
    PRIOR(M.bpsc_eq, HI);
    PRIOR(M.num_eq, HI);
    PRIOR(M.w_eq, HI);
    PRIOR(M.po_eqL, HI);
    PRIOR(M.k_eq0, HI);
    PRIOR(M.k_eq1, HI);
    PRIOR(M.ch_sim, HI);
    PRIOR(M.lpc_sim, HI);
    PRIOR(M.mflag, HI);
    M.mflag[3].p = LO;  // base64 value in a comment: rare
    M.mflag[6].p = LO;  // CR LF line breaks
    M.mflag[7].p = 32768;
    for (int k = 0; k < 4; k++) {
        M.cand_g[0][k].p = HI;
        M.cand_g[0][k].n = 2;
        for (int r = 1; r < 24; r++) {
            M.cand_g[r][k].p = 16000;
            M.cand_f[r][k].p = 16000;
        }
    }
    for (int i = 0; i < 5; i++) {
        M.seg[i][1].p = LO;  // FRAME segments dominate
        M.seg[i][1].n = 4;
    }
}

static Models g_models;       // the meta model (re-initialized for every run)
static LpcPredictor g_lpred;  // LPC candidate predictor

struct FrameState {
    int first;
    u32 ffh, lfh;
    int pblocking, pbscode, psrcode, pchcode, pbpscode;
    u32 psrval, pbs;
    u64 frame_counter, sample_counter;
    int minpo, maxpo;
    int lpc_maxo, lpc_ov;  // learned max lpc order and order-guess overhead variant
    u32 oscore[2][33];
    u32 nlearn;
    int lpc_maxseen, lpc_exh, maxprec;  // exhaustive-search hypothesis
    u32 exscore[2];
    int ff_method;  // ffmpeg order selection method
    u32 ffscore[4];
    int sim_on, sim_newv;  // libFLAC simulation enabled / 1.4 estimator variants
    u32 simstat[2];
    int rice_new, fix_le;  // libFLAC >= 1.4.3 rice estimate / >= 1.4.0 fixed-order tie rule
    int k0mode;            // libFLAC <= 1.3 suggested rice parameter for partition order 0
    u32 k0vote[2];
    int ln2sq;  // libFLAC <= 1.3.1 error scaling
    u32 esvote[2];
    u32 rvote[2], fvote[2];
    u32 dbg[8];
    ChState ch[MAXCH];
};

static void fstate_init(FrameState& s, const StreamParams& sp, int vendor) {
    memset(&s, 0, sizeof(s));
    s.first = 1;
    s.pbs = 4096;
    s.psrcode = canon_srcode(sp.rate);
    s.pbpscode = canon_bpscode(sp.bps);
    s.minpo = 15;
    s.maxpo = 0;
    s.lpc_maxo = 8;
    // vendor: 0 unknown, -1 ffmpeg, else libFLAC version*100 (e.g. 143)
    s.sim_on = vendor >= 0;
    s.rice_new = vendor == 0 || vendor >= 143;
    s.fix_le = vendor == 0 || vendor >= 140;
    s.k0mode = vendor > 0 && vendor < 140;
    s.ln2sq = vendor > 0 && vendor <= 131;
    for (int c = 0; c < MAXCH; c++) {
        s.ch[c].ptype = 4;
        s.ch[c].pfixed = 2;
        s.ch[c].plorder = 32;
        s.ch[c].pprec = 15;
        s.ch[c].pshift = 32;
        s.ch[c].pmethod = 2;
    }
}

// PCM provider for decoding: called once bs is known.
struct WavReader;
static bool wav_read(WavReader* w, s32** pcm, int nch, u32 n);

struct Ctx {
    AC* ac;
    Models* m;
    StreamParams sp;
    ResStats rs;
    LpcPredictor* lp;
    u64 total_samples;  // total PCM samples (per channel) in wav
};

#include "model_residual.inc"

struct SimOut {
    u64 bits;
    int type, order, wid;
};

// libFLAC exhaustive model search: order with minimal estimated subframe bits.
// Leaves P.lpall valid for the candidate (evaluated up to maxo).
static int lpc_exh_guess(Ctx& cx, FrameState& st, int id, int maxo, const s64* x, int bs, int ebps) {
    LpcPredictor& P = *cx.lp;
    int mo = P.eval(id, Min(maxo, bs - 1));
    if (!mo) return 0;
    const s32* x32 = to32(x, bs);
    bool newv = st.rice_new;
    int lim = cx.sp.bps > 16 ? 30 : 14;
    int best = 0;
    u64 bb = ~0ull;
    s32 q[32];
    for (int o = 1; o <= mo; o++) {
        // "don't even try"
        double ebs = lpc_expected_bps(P.lerr[o - 1], bs - o);
        if (ebs >= (double)ebps) continue;
        int prec = st.maxprec;
        if (ebps <= (st.fix_le ? 17 : 16)) prec = Min(prec, 32 - ebps - ilog2_64(o));
        int sh;
        if (!lpc_quantize(P.lpall + (o - 1) * 32, o, prec, q, &sh)) continue;
        lpc_residual_fast(x, x32, bs, q, o, sh, g_res2);
        ResStats& rs = cx.rs;
        rs.compute(g_res2, bs, o, false);
        int hi = Min(st.maxpo, rs.maxpo);
        while (hi > 0 && ((u32)bs >> hi) <= (u32)o) hi--;
        int lo = Min(st.minpo, hi);
        rs.k0 = st.k0mode ? k0_from_bps(ebs, lim) : -1;
        u64 bits = (u64)o * (prec + ebps) + rs.bits_lf(lo, hi, lim, newv);
        rs.k0 = -1;
        if (bits < bb) {
            bb = bits;
            best = o;
        }
    }
    return best;
}

// ffmpeg partition order limit
static int ff_pmax(int maxp, int n, int order) {
    int p = Min(maxp, ilog2_64((u64)(n ^ (n - 1))));
    if (order > 0) p = Min(p, n / order ? ilog2_64((u64)(n / order)) : 0);
    return p;
}

// ffmpeg order selection (method: 0 EST, 1 SEARCH, 2 4LEVEL, 3 LOG). Leaves P.lpd valid up to maxo.
static int ff_order_guess(Ctx& cx, FrameState& st, int id, int method, int maxo, const s64* x, int bs, int ebps,
                          int prec) {
    LpcPredictor& P = *cx.lp;
    int mo = P.eval(id, Min(maxo, bs - 1));
    if (!mo) return 0;
    const int min_order = 1;
    if (method == 0) return ff_est_order(P.lpd, min_order, mo);
    int lim = cx.sp.bps > 16 ? 30 : 14;
    u64 bits[33];
    for (int i = 0; i <= 32; i++) bits[i] = ~0ull;
    auto eval_order = [&](int o) -> u64 {  // o is 1-based
        s32 q[32];
        int sh;
        ff_quantize(P.lpd + (o - 1) * 32, o, prec, q, &sh, -1);
        lpc_residual(x, bs, q, o, sh, g_res2);
        ResStats& rs = cx.rs;
        rs.compute(g_res2, bs, o);
        int pmin = ff_pmax(Min(st.minpo, 8), bs, o), pmax = ff_pmax(st.maxpo, bs, o);
        if (pmin > pmax) pmin = pmax;
        return (u64)o * (ebps + prec) + rs.bits_ff(pmin, pmax, lim);
    };
    if (method == 1) {  // SEARCH
        int opt = 0;
        u64 bo = 0xFFFFFFFFull;
        for (int i = min_order - 1; i < mo; i++) {
            u64 b = eval_order(i + 1);
            if (b < bo) {
                bo = b;
                opt = i;
            }
        }
        return opt + 1;
    }
    if (method == 2) {  // 4LEVEL
        int levels = 4, order = -1, opt_index = levels - 1, opt_order = mo - 1;
        u64 bl[8];
        bl[opt_index] = 0xFFFFFFFFull;
        for (int i = levels - 1; i >= 0; i--) {
            int last = order;
            order = min_order + (((mo - min_order + 1) * (i + 1)) / levels) - 1;
            if (order < min_order - 1) order = min_order - 1;
            if (order > mo - 1) order = mo - 1;
            if (order == last) continue;
            bl[i] = eval_order(order + 1);
            if (bl[i] < bl[opt_index]) {
                opt_index = i;
                opt_order = order;
            }
        }
        return opt_order + 1;
    }
    // LOG
    int opt_order = min_order - 1 + (mo - min_order) / 3;
    for (int step = 16; step; step >>= 1) {
        int last = opt_order;
        for (int i = last - step; i <= last + step; i += step) {
            if (i < min_order - 1 || i >= mo || bits[i] < 0xFFFFFFFFull) continue;
            bits[i] = eval_order(i + 1);
            if (bits[i] < bits[opt_order]) opt_order = i;
        }
    }
    return opt_order + 1;
}

// the encoder's order choice for a candidate window under the current hypotheses;
// afterwards the candidate's coefficients for that order are available via P.quant
static int order_guess(Ctx& cx, FrameState& st, int id, const s64* x, int bs, int ebps, int prec) {
    LpcPredictor& P = *cx.lp;
    if (P.cand[id].kind == 3) return ff_order_guess(cx, st, id, st.ff_method, st.lpc_maxseen, x, bs, ebps, prec);
    if (st.lpc_exh) return lpc_exh_guess(cx, st, id, st.lpc_maxseen, x, bs, ebps);
    int mx = Min(st.lpc_maxo, bs - 1);
    if (mx < 1) return 0;
    int mo = P.eval(id, mx);
    if (!mo) return 0;
    return lpc_best_order(P.lerr, mo, bs, ebps + (st.lpc_ov ? 5 : prec));
}

// learn which hypotheses reproduce the encoder's order choice
static void lpc_learn(Ctx& cx, FrameState& st, int id, int order, int bs, int ebps, int prec, const s64* x) {
    LpcPredictor& P = *cx.lp;
    if (order > st.lpc_maxseen) st.lpc_maxseen = order;
    if (prec > st.maxprec) st.maxprec = prec;
    if (st.nlearn >= 64 && (st.nlearn & 15)) {
        st.nlearn++;
        return;
    }
    st.nlearn++;
    if (P.cand[id].kind == 3) {
        int best = st.ff_method;
        for (int m = 0; m < 4; m++) {
            st.ffscore[m] += ff_order_guess(cx, st, id, m, st.lpc_maxseen, x, bs, ebps, prec) == order;
            if (st.ffscore[m] > st.ffscore[best]) best = m;
        }
        st.ff_method = best;
        return;
    }
    int gn = 0;
    {
        int mo = P.eval(id, Min(st.lpc_maxo, bs - 1));
        if (mo) {
            gn = lpc_best_order(P.lerr, mo, bs, ebps + (st.lpc_ov ? 5 : prec));
            g_ln2sq = !g_ln2sq;
            int gn2 = lpc_best_order(P.lerr, mo, bs, ebps + (st.lpc_ov ? 5 : prec));
            g_ln2sq = !g_ln2sq;
            if (gn != gn2) {
                st.esvote[st.ln2sq] += gn == order;
                st.esvote[!st.ln2sq] += gn2 == order;
                if (st.esvote[0] + st.esvote[1] >= 4) st.ln2sq = st.esvote[1] > st.esvote[0];
                g_ln2sq = st.ln2sq;
            }
        }
    }
    int ge = lpc_exh_guess(cx, st, id, st.lpc_maxseen, x, bs, ebps);
    st.exscore[0] += gn == order;
    st.exscore[1] += ge == order;
    // exhaustive search is expensive to simulate: require clear evidence
    st.lpc_exh = st.exscore[1] > st.exscore[0] + st.exscore[0] / 4 + 4;
    int mo = P.eval(id, Min(32, bs - 1));
    for (int v = 0; v < 2; v++) {
        int ov = ebps + (v ? 5 : prec);
        for (int m = order; m <= mo; m++)
            if (lpc_best_order(P.lerr, m, bs, ov) == order) st.oscore[v][m]++;
    }
    // the encoder's max order is at least the largest order seen
    if (st.lpc_maxo < st.lpc_maxseen) st.lpc_maxo = st.lpc_maxseen;
    u32 best = st.oscore[st.lpc_ov][st.lpc_maxo];
    for (int v = 0; v < 2; v++)
        for (int m = st.lpc_maxseen; m <= 32; m++)
            if (st.oscore[v][m] > best) {
                best = st.oscore[v][m];
                st.lpc_maxo = m;
                st.lpc_ov = v;
            }
}

static void mru_hit(LpcPredictor& P, int r) {
    int id = P.mru[r];
    P.hits[id]++;
    for (int k = r; k > 0; k--) P.mru[k] = P.mru[k - 1];
    P.mru[0] = id;
}

#include "model_lpc.inc"

// ---------------------------------------------------------------------------
// libFLAC subframe cost simulation (process_subframe_ estimates)

// windows the encoder appears to use (libFLAC candidates with hits); for windows that exist
// in several numeric variants (autocorrelation flavour etc.) only the most used variant is kept
static int sim_windows(LpcPredictor& P, int* ids) {
    u32 mx = 0;
    for (int i = 0; i < P.ncand; i++)
        if (P.cand[i].kind != 3) mx = Max(mx, P.hits[i]);
    int n = 0;
    if (!mx) return 0;
    for (int i = 0; i < P.ncand && n < 16; i++) {
        const CandDef& a = P.cand[i];
        if (a.kind == 3 || !P.hits[i] || P.hits[i] * 32 < mx) continue;
        bool dominated = false;
        for (int j = 0; j < P.ncand; j++) {
            const CandDef& b = P.cand[j];
            if (j == i || b.kind != a.kind || b.b != a.b || b.c != a.c) continue;
            bool samewin = base_win(b.win) == base_win(a.win);
            if (!samewin) continue;
            if (P.hits[j] > P.hits[i] || (P.hits[j] == P.hits[i] && j < i)) dominated = true;
        }
        if (!dominated) ids[n++] = i;
    }
    return n;
}

#include "sim_subframe.inc"

// libFLAC's channel assignment decision (-m); returns chcode or -1
static SimOut g_sim4[4];
static int sim_stereo(Ctx& cx, FrameState& st, s32** pcm, int n) {
    LpcPredictor& P = *cx.lp;
    int wids[16];
    int nw = sim_windows(P, wids);
    static s64 sig[4][MAXBS];
    for (int i = 0; i < n; i++) {
        s64 l = pcm[0][i], r = pcm[1][i];
        sig[0][i] = l;
        sig[1][i] = r;
        sig[2][i] = (l + r) >> 1;
        sig[3][i] = l - r;
    }
    u64 b[4];
    for (int k = 0; k < 4; k++) {
        g_sim4[k] = sim_subframe(cx, st, sig[k], n, cx.sp.bps + (k == 3), wids, nw);
        b[k] = g_sim4[k].bits;
    }
    u64 t[4] = {b[0] + b[1], b[0] + b[3], b[1] + b[3], b[2] + b[3]};
    int best = 0;
    for (int k = 1; k < 4; k++)
        if (t[k] < t[best]) best = k;
    static const int map[4] = {1, 8, 9, 10};
    return map[best];
}

// ffmpeg estimate_stereo_mode
static int ff_stereo_guess(s32** pcm, int n, int maxk) {
    const s32* L = pcm[0];
    const s32* R = pcm[1];
    u64 sum[4] = {0, 0, 0, 0};
    for (int i = 2; i < n; i++) {
        s64 lt = (s64)L[i] - 2 * (s64)L[i - 1] + L[i - 2];
        s64 rt = (s64)R[i] - 2 * (s64)R[i - 1] + R[i - 2];
        s64 m = (lt + rt) >> 1, d = lt - rt;
        sum[2] += (u64)(m < 0 ? -m : m);
        sum[3] += (u64)(d < 0 ? -d : d);
        sum[0] += (u64)(lt < 0 ? -lt : lt);
        sum[1] += (u64)(rt < 0 ? -rt : rt);
    }
    for (int i = 0; i < 4; i++) {
        int k = rice_est_ff(2 * sum[i], (u32)n, maxk);
        sum[i] = (u64)n * (k + 1) + ((2 * sum[i] - (u64)(n >> 1)) >> k);
    }
    u64 score[4] = {sum[0] + sum[1], sum[0] + sum[3], sum[1] + sum[3], sum[2] + sum[3]};
    int best = 0;
    for (int i = 1; i < 4; i++)
        if (score[i] < score[best]) best = i;
    return best;
}

#include "model_frame.inc"

// ---------------------------------------------------------------------------
// Container-level structures

enum { SEG_FRAME = 0, SEG_RAWFRAME = 1, SEG_RAWBYTES = 2, SEG_END = 3 };

struct Segment {
    int kind;
    u64 off;    // byte offset in flac
    u64 len;    // byte length
    u32 nsamp;  // samples (frames)
};

struct MBlock {
    int type;
    u32 len;
    u8* data;  // c: points into the input file; d: only valid while the block is decoded
    int structured;
    u64 off;  // file offset of the block header
};
struct FlacLayout {
    u8* prefix;  // c: bytes before "fLaC" (in the input buffer)
    size_t prefix_n;
    Vec<MBlock> blocks;
    StreamParams sp;
    int vendor;  // encoder hint from the vendor string
    u8 si[34];   // d: STREAMINFO contents
};
enum { BLOCKMAX = 1 << 24 };  // metadata block length is a 24-bit field
static u8 g_blockbuf[BLOCKMAX];  // d: contents of the metadata block being decoded

#include "frame_stats.inc"

// ---------------------------------------------------------------------------
// WAV output / input

static u32 default_mask(int nch) {
    static const u32 m[9] = {0, 0x4, 0x3, 0x7, 0x33, 0x37, 0x3F, 0x70F, 0x63F};
    return m[nch];
}

static void wav_header(u8* h, int& hl, int nch, u32 rate, int bps, u64 nsamples) {
    int cb = (bps + 7) / 8;
    u64 data = nsamples * nch * cb;
    bool ext = nch > 2 || (bps & 7);
    int fmtlen = ext ? 40 : 16;
    hl = 12 + 8 + fmtlen + 8;
    u64 riff = hl - 8 + data;
    memcpy(h, "RIFF", 4);
    wr_le32(h + 4, riff > 0xFFFFFFFFull ? 0xFFFFFFFF : (u32)riff);
    memcpy(h + 8, "WAVEfmt ", 8);
    wr_le32(h + 16, fmtlen);
    wr_le16(h + 20, ext ? 0xFFFE : 1);
    wr_le16(h + 22, nch);
    wr_le32(h + 24, rate);
    wr_le32(h + 28, rate * nch * cb);
    wr_le16(h + 32, nch * cb);
    wr_le16(h + 34, cb * 8);
    int p = 36;
    if (ext) {
        wr_le16(h + 36, 22);
        wr_le16(h + 38, bps);
        wr_le32(h + 40, default_mask(nch));
        static const u8 guid[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
        memcpy(h + 44, guid, 16);
        p = 60;
    }
    memcpy(h + p, "data", 4);
    wr_le32(h + p + 4, data > 0xFFFFFFFFull ? 0xFFFFFFFF : (u32)data);
}

static u8 g_wavbuf[(size_t)MAXBS * MAXCH * 4];  // one block of interleaved samples

#include "wav_writer.inc"

#include "wav_reader.inc"

static bool wav_read(WavReader* w, s32** pcm, int nch, u32 n) { return w->read(pcm, nch, n); }

// ---------------------------------------------------------------------------
// Output sink for restoration (file, or compare against original)

#include "sink.inc"

// ---------------------------------------------------------------------------
// Metadata coding

static const u32 k_rates[8] = {44100, 48000, 96000, 88200, 192000, 176400, 32000, 22050};

static void code_stream_params(AC& ac, StreamParams& sp) {
    ac.cat = C_META;
    int ri = 7;
    for (int i = 0; i < 7; i++)
        if (k_rates[i] == sp.rate) ri = i;
    ri = (int)ac.raw(ri, 3);
    if (ri < 7)
        sp.rate = k_rates[ri];
    else
        sp.rate = (u32)ac.raw(sp.rate, 20);
    int st = ac.code(sp.nch == 2, 50000);
    if (st)
        sp.nch = 2;
    else
        sp.nch = (int)ac.raw(sp.nch - 1, 3) + 1;
    int bi = sp.bps == 16 ? 0 : sp.bps == 24 ? 1 : sp.bps == 8 ? 2 : 3;
    bi = (int)ac.raw(bi, 2);
    if (bi == 0)
        sp.bps = 16;
    else if (bi == 1)
        sp.bps = 24;
    else if (bi == 2)
        sp.bps = 8;
    else
        sp.bps = (int)ac.raw(sp.bps - 1, 5) + 1;
}

static const char* k_vendors[] = {
    "reference libFLAC 1.1.0 20030126", "reference libFLAC 1.1.1 20041001", "reference libFLAC 1.1.2 20050205",
    "reference libFLAC 1.1.3 20061120", "reference libFLAC 1.1.4 20070213", "reference libFLAC 1.2.0 20070715",
    "reference libFLAC 1.2.1 20070917", "reference libFLAC 1.3.0 20130526", "reference libFLAC 1.3.1 20141125",
    "reference libFLAC 1.3.2 20170101", "reference libFLAC 1.3.3 20190804", "reference libFLAC 1.3.4 20220220",
    "reference libFLAC 1.4.0 20220909", "reference libFLAC 1.4.1 20220922", "reference libFLAC 1.4.2 20221022",
    "reference libFLAC 1.4.3 20230623", "reference libFLAC 1.5.0 20250211"};
enum { NVENDORS = sizeof(k_vendors) / sizeof(k_vendors[0]) };

static bool vorbis_ok(const u8* d, u32 len);

#include "code_vorbis.inc"

static bool vorbis_ok(const u8* d, u32 len) {
    if (len < 8) return false;
    u32 vl = rd_le32(d);
    if ((u64)vl + 8 > len) return false;
    u32 p = 4 + vl;
    u32 cnt = rd_le32(d + p);
    p += 4;
    for (u32 i = 0; i < cnt; i++) {
        if (p + 4 > len) return false;
        u32 l = rd_le32(d + p);
        p += 4;
        if ((u64)p + l > len) return false;
        p += l;
    }
    return p == len;
}

static int vendor_hint(const u8* data, u32 len);

// Part A: prefix, block list, contents not depending on frames.
// d: everything except STREAMINFO and seek table contents is written to 'out' right away.
static bool code_layout(AC& ac, Models& M, FlacLayout& L, Sink* out) {
    ac.cat = C_META;
    g_ext.count = 0;
    model_init(g_extm);
    g_extm.n.len[0].p = 3000;  // most regions contain no embedded files
    g_extm.n.len[0].n = 4;
    u64 plen = code_uint(ac, M.len_m, L.prefix_n);
    if (ac.dec) {  // unbounded size (e.g. an ID3v2 tag): temporary buffer, written out at once
        Buf pb;
        pb.reserve((size_t)plen + 1);
        bool ok = code_region(ac, pb.p, (size_t)plen, 0, false);
        if (ok) out->write_at(0, pb.p, (size_t)plen);
        pb.release();
        if (!ok) return false;
        L.prefix_n = (size_t)plen;
        out->write_at(plen, (const u8*)"fLaC", 4);
    } else if (!code_region(ac, L.prefix, (size_t)plen, 0, false))
        return false;
    L.vendor = 0;
    ac.cat = C_META;
    code_stream_params(ac, L.sp);
    if (L.sp.nch < 1 || L.sp.nch > 8 || L.sp.bps < 4 || L.sp.bps > 32) return false;
    // block 0 is STREAMINFO
    u64 nb = code_uint(ac, M.num_m, L.blocks.n);
    if (ac.dec) {
        if (nb == 0 || nb > 100000) return false;
        L.blocks.n = 0;
        for (u64 i = 0; i < nb; i++) {
            MBlock b;
            memset(&b, 0, sizeof(b));
            L.blocks.push(b);
        }
        L.blocks[0].type = 0;
        L.blocks[0].len = 34;
        L.blocks[0].data = L.si;
    }
    u64 boff = L.prefix_n + 4;
    L.blocks[0].off = boff;
    boff += 4 + 34;
    int pt = 0;
    for (size_t i = 1; i < L.blocks.n; i++) {
        MBlock& b = L.blocks[i];
        b.type = (int)ac.tree(M.mtype[Min(pt, 7)], 7, b.type);
        pt = b.type;
        b.len = (u32)code_uint(ac, M.len_m, b.len);
        if (b.len >= BLOCKMAX) return false;
        b.off = boff;
        boff += 4 + (u64)b.len;
        if (ac.dec) {
            b.data = g_blockbuf;
            memset(g_blockbuf, 0, b.len);
        }
        if (b.type == 1) {  // PADDING
            int z = 1;
            if (!ac.dec)
                for (u32 j = 0; j < b.len; j++)
                    if (b.data[j]) z = 0;
            z = ac.bit(M.mflag[0], z);
            b.structured = z;
            if (!z && !code_region(ac, b.data, b.len, 0, false)) return false;
        } else if (b.type == 3) {  // SEEKTABLE
            int s = ac.bit(M.mflag[1], b.len % 18 == 0);
            b.structured = s;
            if (!s) code_bytes(ac, b.data, b.len, false);
        } else if (b.type == 4) {
            if (!code_vorbis(ac, M, b)) return false;
            if (!L.vendor) L.vendor = vendor_hint(b.data, b.len);
        } else {
            if (!code_region(ac, b.data, b.len, b.type == 6 ? 1 : 0, false)) return false;
        }
        if (ac.dec) {  // seek tables are filled in later (code_derived)
            if (!(b.type == 3 && b.structured)) out->write_at(b.off + 4, b.data, b.len);
            b.data = nullptr;
        }
    }
    if (ac.dec)  // block headers
        for (size_t i = 0; i < L.blocks.n; i++) {
            u8 h[4];
            h[0] = (u8)((i + 1 == L.blocks.n ? 0x80 : 0) | L.blocks[i].type);
            wr_be24(h + 1, L.blocks[i].len);
            out->write_at(L.blocks[i].off, h, 4);
        }
    return true;
}

// Part C: STREAMINFO derived fields + seek tables
static void code_choice_u64(AC& ac, BM* flags, u64& v, const u64* cand, int nc, int bits) {
    for (int i = 0; i < nc; i++) {
        int dup = 0;
        for (int j = 0; j < i; j++)
            if (cand[j] == cand[i]) dup = 1;
        if (dup) continue;
        if (ac.bit(flags[i], v == cand[i])) {
            v = cand[i];
            return;
        }
    }
    v = ac.raw(v, bits);
}

struct SeekModels {
    BM kind[3][4];
    BM match[4];
    BM mode[4];
    UModel dk;
};

static u64 frame_containing(const FrameStats& fs, u64 sample) {
    // binary search last frame with fsamp <= sample
    if (fs.fsamp.n == 0) return 0;
    size_t lo = 0, hi = fs.fsamp.n - 1;
    while (lo < hi) {
        size_t mid = (lo + hi + 1) / 2;
        if (fs.fsamp[mid] <= sample)
            lo = mid;
        else
            hi = mid - 1;
    }
    return lo;
}

#include "code_seektable.inc"

static bool code_derived(AC& ac, FlacLayout& L, FrameStats& fs, Sink* out) {
    ac.cat = C_META;
    BM fl[8][4];
    bm_init(&fl[0][0], 32);
    MBlock& si = L.blocks[0];
    u8* d = si.data;
    u64 minbs = 0, maxbs = 0, minfs = 0, maxfs = 0, total = 0;
    if (!ac.dec) {
        minbs = (d[0] << 8) | d[1];
        maxbs = (d[2] << 8) | d[3];
        minfs = rd_be24(d + 4);
        maxfs = rd_be24(d + 7);
        total = ((u64)(d[13] & 15) << 32) | rd_be32(d + 14);
    }
    u32 mfs = fs.nframes ? fs.min_fs : 0;
    u32 xfs = fs.nframes ? fs.max_fs : 0;
    u64 c_minbs[4] = {fs.first_bs, fs.nframes > 1 ? fs.min_bs_nl : fs.min_bs, fs.min_bs, 0};
    u64 c_maxbs[4] = {fs.first_bs, fs.max_bs, fs.nframes > 1 ? fs.max_bs_nl : fs.max_bs, 0};
    code_choice_u64(ac, fl[0], minbs, c_minbs, 4, 16);
    u64 c_maxbs2[4] = {minbs, c_maxbs[1], c_maxbs[2], c_maxbs[0]};
    code_choice_u64(ac, fl[1], maxbs, c_maxbs2, 4, 16);
    u64 c_minfs[2] = {mfs, 0};
    code_choice_u64(ac, fl[2], minfs, c_minfs, 2, 24);
    u64 c_maxfs[2] = {xfs, 0};
    code_choice_u64(ac, fl[3], maxfs, c_maxfs, 2, 24);
    u64 c_tot[2] = {fs.total, 0};
    code_choice_u64(ac, fl[4], total, c_tot, 2, 36);
    // md5
    int which = 2;
    if (!ac.dec) {
        static const u8 zero[16] = {0};
        if (!memcmp(d + 18, fs.md5sum, 16))
            which = 0;
        else if (!memcmp(d + 18, zero, 16))
            which = 1;
    }
    if (ac.bit(fl[5][0], which == 0))
        which = 0;
    else if (ac.bit(fl[5][1], which == 1))
        which = 1;
    else
        which = 2;
    u8 md5[16];
    if (which == 0)
        memcpy(md5, fs.md5sum, 16);
    else if (which == 1)
        memset(md5, 0, 16);
    else
        for (int i = 0; i < 16; i++) md5[i] = (u8)ac.raw(d[18 + i], 8);
    if (ac.dec) {
        d[0] = (u8)(minbs >> 8);
        d[1] = (u8)minbs;
        d[2] = (u8)(maxbs >> 8);
        d[3] = (u8)maxbs;
        wr_be24(d + 4, (u32)minfs);
        wr_be24(d + 7, (u32)maxfs);
        const StreamParams& sp = L.sp;
        d[10] = (u8)(sp.rate >> 12);
        d[11] = (u8)(sp.rate >> 4);
        d[12] = (u8)(((sp.rate & 15) << 4) | ((sp.nch - 1) << 1) | ((sp.bps - 1) >> 4));
        d[13] = (u8)((((sp.bps - 1) & 15) << 4) | ((total >> 32) & 15));
        wr_be32(d + 14, (u32)total);
        memcpy(d + 18, md5, 16);
        out->write_at(si.off + 4, d, 34);
    }
    for (size_t i = 1; i < L.blocks.n; i++)
        if (L.blocks[i].type == 3 && L.blocks[i].structured) code_seektable(ac, L.blocks[i], fs, L.sp, out);
    // 16-bit check of the PCM (detects a .wav that does not belong to this .meta)
    u32 chk = (fs.md5sum[0] << 8) | fs.md5sum[1];
    u32 got = (u32)ac.raw(chk, 16);
    return got == chk;
}

// encoder hint from the VORBIS_COMMENT vendor string: 0 unknown, -1 ffmpeg, libFLAC version*100
static int vendor_hint(const u8* data, u32 len) {
    if (len < 4) return 0;
    u32 vl = rd_le32(data);
    if ((u64)vl + 4 > len) return 0;
    const char* v = (const char*)data + 4;
    if (vl >= 4 && (!memcmp(v, "Lavf", 4) || !memcmp(v, "Lavc", 4))) return -1;
    const char* pre = "reference libFLAC ";
    size_t pl = strlen(pre);
    if (vl > pl && !memcmp(v, pre, pl)) {
        int d[3] = {0, 0, 0}, k = 0;
        for (u32 j = (u32)pl; j < vl && k < 3; j++) {
            char c = v[j];
            if (c >= '0' && c <= '9')
                d[k] = d[k] * 10 + (c - '0');
            else if (c == '.')
                k++;
            else
                break;
        }
        if (d[0] == 0 && d[1] == 0) return 150;  // git builds
        return d[0] * 100 + d[1] * 10 + d[2];
    }
    return 0;
}

static u64 header_size(const FlacLayout& L) {
    u64 s = L.prefix_n + 4;
    for (size_t i = 0; i < L.blocks.n; i++) s += 4 + L.blocks[i].len;
    return s;
}

// ---------------------------------------------------------------------------
// Compression (flac -> wav + meta)

static const u8 k_magic[2] = {'F', 0xD1};

struct Options {
    int verify;
    int dump;  // -x: write <prefix>.side, a plain serialization of the side info (for comparisons)
};

// plain serialization of everything except PCM-derived data (diagnostics only)
static void dump_frame_side(Buf& o, const u8* frame, size_t hl, const FrameInfo& f) {
    o.append(frame, hl - 1);  // header without CRC-8
    for (int ch = 0; ch < f.nch; ch++) {
        const SubInfo& s = f.sub[ch];
        o.put((u8)(s.type * 64 + s.order));
        o.put((u8)s.wasted);
        if (s.type < ST_FIXED) continue;
        if (s.type == ST_LPC) {
            o.put((u8)s.prec);
            o.put((u8)s.shift);
            for (int i = 0; i < s.order; i++) {
                o.put((u8)s.coef[i]);
                o.put((u8)(s.coef[i] >> 8));
            }
        }
        o.put((u8)(s.method * 16 + s.porder));
        for (int i = 0; i < (1 << s.porder); i++) {
            o.put(s.param[i]);
            if (s.param[i] == (s.method ? 31 : 15)) o.put(s.raw[i]);
        }
    }
}

// blocks and prefix point into the input buffer (no copies)
static bool parse_layout(Buf& in, FlacLayout& L, u64& audio_start) {
    size_t pos = 0;
    // skip ID3v2 tags
    while (pos + 10 <= in.n && !memcmp(in.p + pos, "ID3", 3)) {
        const u8* h = in.p + pos;
        u32 sz = ((h[6] & 0x7F) << 21) | ((h[7] & 0x7F) << 14) | ((h[8] & 0x7F) << 7) | (h[9] & 0x7F);
        pos += 10 + sz + ((h[5] & 0x10) ? 10 : 0);
    }
    if (pos + 4 > in.n || memcmp(in.p + pos, "fLaC", 4)) {
        // search a bit
        size_t lim = Min(in.n, (size_t)(1 << 26));
        size_t q = 0;
        for (; q + 8 <= lim; q++)
            if (!memcmp(in.p + q, "fLaC", 4) && (in.p[q + 4] & 0x7F) == 0 && rd_be24(in.p + q + 5) == 34) break;
        if (q + 8 > lim) return false;
        pos = q;
    }
    L.prefix = in.p;
    L.prefix_n = pos;
    pos += 4;
    L.blocks.n = 0;
    for (;;) {
        if (pos + 4 > in.n) return false;
        const u8* h = in.p + pos;
        MBlock b;
        b.type = h[0] & 0x7F;
        b.len = rd_be24(h + 1);
        b.structured = 0;
        int last = h[0] >> 7;
        pos += 4;
        if (pos + b.len > in.n) return false;
        b.off = pos - 4;
        b.data = in.p + pos;
        pos += b.len;
        L.blocks.push(b);
        if (last) break;
    }
    if (L.blocks[0].type != 0 || L.blocks[0].len != 34) return false;
    const u8* d = L.blocks[0].data;
    L.sp.rate = (d[10] << 12) | (d[11] << 4) | (d[12] >> 4);
    L.sp.nch = ((d[12] >> 1) & 7) + 1;
    L.sp.bps = (((d[12] & 1) << 4) | (d[13] >> 4)) + 1;
    audio_start = pos;
    return true;
}

static bool decompress_core(const u8* meta, size_t meta_n, WavReader& wav, Sink& out);

static void print_stats(AC& ac, size_t total) {
    double sum = 0;
    for (int i = 0; i < NCAT; i++) sum += ac.cost[i];
    fprintf(stderr, "meta breakdown (bytes):\n");
    for (int i = 0; i < NCAT; i++)
        if (ac.cost[i] > 0) fprintf(stderr, "  %-10s %10.1f\n", k_catname[i], ac.cost[i] / 8);
    fprintf(stderr, "  %-10s %10.1f (actual %zu)\n", "total", sum / 8, total);
}

#include "do_compress.inc"

// ---------------------------------------------------------------------------
// Decompression (wav + meta -> flac)

static bool decompress_core(const u8* meta, size_t meta_n, WavReader& wav, Sink& out) {
    if (meta_n < 2 || memcmp(meta, k_magic, 2)) return false;
    AC ac;
    ac.dec_init(meta + 2, meta_n - 2);
    Models* M = &g_models;
    models_init(*M);
    FlacLayout L = {};
    if (!code_layout(ac, *M, L, &out)) return false;
    const StreamParams& sp = L.sp;
    wav.bps = sp.bps;
    if (wav.nch != sp.nch || wav.cb != (sp.bps + 7) / 8) return false;
    u64 pos = header_size(L);
    Ctx cx;
    cx.ac = &ac;
    cx.m = M;
    cx.sp = sp;
    cx.rs.alloc();
    cx.lp = &g_lpred;
    cx.lp->init();
    cx.total_samples = wav.nsamples;
    FrameState st;
    fstate_init(st, sp, L.vendor);
    FrameStats fs;
    fs.init();
    fs.audio_start = pos;
    FrameInfo fi;
    frameinfo_init(fi);
    Buf tmp;  // raw junk regions (unbounded size)
    u64 done = 0;
    int pk = 4;
    for (;;) {
        int kind = ac.tree(M->seg[pk], 2, 0);
        pk = kind;
        if (kind == SEG_END) break;
        if (kind == SEG_FRAME) {
            if (!model_frame(cx, fi, g_pcm, st, &wav, done)) return false;
            FixOut fo = {g_framebuf, 0, FRAMEMAX, false};
            if (!encode_frame(fi, g_pcm, fo)) return false;
            out.write_at(pos, g_framebuf, fo.n);
            fs.add_frame(pos, (u32)fo.n, fi.bs);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, fi.bs);
            pos += fo.n;
            done += fi.bs;
        } else if (kind == SEG_RAWFRAME) {
            u32 ns = (u32)code_uint(ac, M->len_m, 0) + 1;
            u64 len = code_uint(ac, M->len_m, 0);
            if (ns > MAXBS || !wav.read(g_pcm, sp.nch, ns)) return false;
            for (u64 j = 0; j < len;) {  // stream through the frame buffer
                size_t k = (size_t)Min(len - j, (u64)FRAMEMAX);
                for (size_t q = 0; q < k; q++) g_framebuf[q] = (u8)ac.raw(0, 8);
                out.write_at(pos + j, g_framebuf, k);
                j += k;
            }
            fs.add_frame(pos, (u32)len, ns);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, ns);
            pos += len;
            done += ns;
            st.frame_counter++;
            st.sample_counter += ns;
        } else {
            u64 len = code_uint(ac, M->len_m, 0) + 1;
            tmp.n = 0;
            tmp.reserve(len);
            if (!code_region(ac, tmp.p, (size_t)len, 0, false)) return false;
            out.write_at(pos, tmp.p, (size_t)len);
            pos += len;
        }
    }
    fs.md5.final(fs.md5sum);
    if (!code_derived(ac, L, fs, &out)) {
        fprintf(stderr, "flacdump: the .wav does not match the .meta\n");
        return false;
    }
    out.pos = pos;
    L.blocks.release();
    tmp.release();
    return true;
}

static int do_decompress(const char* prefix, const char* outname) {
    g_ext.prefix = prefix;
    char wname[4096], mname[4096];
    snprintf(wname, sizeof(wname), "%s.wav", prefix);
    snprintf(mname, sizeof(mname), "%s.meta", prefix);
    Buf meta;
    if (!read_file(mname, meta, 16)) die("cannot read meta");
    WavReader wr;
    wr.f = nullptr;
    if (!wr.open(wname)) die("cannot read wav");
    Sink sk;
    memset(&sk, 0, sizeof(sk));
    sk.f = fopen(outname, "wb");
    if (!sk.f) die("cannot create output");
    bool ok = decompress_core(meta.p, meta.n, wr, sk);
    wr.close();
    if (fclose(sk.f) || !ok) {
        remove(outname);
        die("restore failed (corrupt or mismatched meta/wav)");
    }
    return 0;
}

// ---------------------------------------------------------------------------

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
    alloc_work();
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
