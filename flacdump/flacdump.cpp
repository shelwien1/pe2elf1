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
// Build: g++ -std=c++20 -O2 -fno-exceptions -fno-rtti -o flacdump flacdump.cpp
// No STL is used; only the C runtime (stdio, stdlib, string, math).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

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

static void* xmalloc(size_t n) {
    void* p = malloc(n ? n : 1);
    if (!p) die("out of memory");
    return p;
}
static void* xcalloc(size_t n) {
    void* p = calloc(n ? n : 1, 1);
    if (!p) die("out of memory");
    return p;
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

struct Buf {
    u8* p = nullptr;
    size_t n = 0, cap = 0;
    void reserve(size_t c) {
        if (c <= cap) return;
        size_t nc = cap ? cap : 4096;
        while (nc < c) nc *= 2;
        p = (u8*)xrealloc(p, nc);
        cap = nc;
    }
    inline void put(u8 b) {
        if (n == cap) reserve(n + 1);
        p[n++] = b;
    }
    void append(const u8* s, size_t k) {
        reserve(n + k);
        memcpy(p + n, s, k);
        n += k;
    }
    void release() {
        free(p);
        p = nullptr;
        n = cap = 0;
    }
};

template <class T> struct Vec {
    T* p = nullptr;
    size_t n = 0, cap = 0;
    void push(const T& v) {
        if (n == cap) {
            cap = cap ? cap * 2 : 64;
            p = (T*)xrealloc(p, cap * sizeof(T));
        }
        p[n++] = v;
    }
    T& operator[](size_t i) { return p[i]; }
    const T& operator[](size_t i) const { return p[i]; }
    T& back() { return p[n - 1]; }
    void release() {
        free(p);
        p = nullptr;
        n = cap = 0;
    }
};

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

struct MD5 {
    u32 h[4];
    u8 buf[64];
    u64 len;
    void init() {
        h[0] = 0x67452301;
        h[1] = 0xefcdab89;
        h[2] = 0x98badcfe;
        h[3] = 0x10325476;
        len = 0;
    }
    static inline u32 rol(u32 x, int c) { return (x << c) | (x >> (32 - c)); }
    void block(const u8* p) {
        static const u32 K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
        static const int R[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5, 9,  14, 20, 5, 9,
                                  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                  4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
        u32 w[16];
        for (int i = 0; i < 16; i++) w[i] = rd_le32(p + i * 4);
        u32 a = h[0], b = h[1], c = h[2], d = h[3];
        for (int i = 0; i < 64; i++) {
            u32 f;
            int g;
            if (i < 16) {
                f = (b & c) | (~b & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | (~d & c);
                g = (5 * i + 1) & 15;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3 * i + 5) & 15;
            } else {
                f = c ^ (b | ~d);
                g = (7 * i) & 15;
            }
            u32 t = d;
            d = c;
            c = b;
            b = b + rol(a + f + K[i] + w[g], R[i]);
            a = t;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
    }
    void update(const u8* p, size_t n) {
        size_t have = (size_t)(len & 63);
        len += n;
        if (have) {
            size_t k = 64 - have;
            if (k > n) k = n;
            memcpy(buf + have, p, k);
            p += k;
            n -= k;
            have += k;
            if (have < 64) return;
            block(buf);
        }
        while (n >= 64) {
            block(p);
            p += 64;
            n -= 64;
        }
        if (n) memcpy(buf, p, n);
    }
    void final(u8 out[16]) {
        u64 bits = len * 8;
        u8 pad = 0x80;
        update(&pad, 1);
        u8 z = 0;
        while ((len & 63) != 56) update(&z, 1);
        u8 lb[8];
        for (int i = 0; i < 8; i++) lb[i] = (u8)(bits >> (8 * i));
        update(lb, 8);
        for (int i = 0; i < 4; i++) wr_le32(out + 4 * i, h[i]);
    }
};

// ---------------------------------------------------------------------------
// Bit reader / writer (MSB first)

struct BitReader {
    const u8* d;  // must be followed by >= 16 readable bytes of padding
    u64 nbits, pos;
    void init(const u8* p, size_t nbytes) {
        d = p;
        nbits = (u64)nbytes * 8;
        pos = 0;
    }
    inline u64 peek() const {
        const u8* q = d + (pos >> 3);
        u64 v = ((u64)q[0] << 56) | ((u64)q[1] << 48) | ((u64)q[2] << 40) | ((u64)q[3] << 32) | ((u64)q[4] << 24) |
                ((u64)q[5] << 16) | ((u64)q[6] << 8) | (u64)q[7];
        return v << (pos & 7);
    }
    inline u32 get(int n) {  // n <= 32
        if (!n) return 0;
        u64 v = peek() >> (64 - n);
        pos += n;
        return (u32)v;
    }
    inline u64 get64(int n) {  // n <= 64
        if (n <= 32) return get(n);
        u64 hi = get(n - 32);
        return (hi << 32) | get(32);
    }
    inline s64 gets(int n) {
        if (!n) return 0;
        u64 v = get64(n);
        if (n < 64) {
            u64 m = 1ull << (n - 1);
            v = (v ^ m) - m;
        }
        return (s64)v;
    }
    inline bool unary(u64& out) {
        u64 c = 0;
        for (;;) {
            u64 v = peek();
            if (v) {
                int z = clz64(v);
                pos += z + 1;
                out = c + z;
                return pos <= nbits;
            }
            c += 57;
            pos += 57;
            if (pos > nbits) return false;
        }
    }
    bool ok() const { return pos <= nbits; }
};

struct BitWriter {
    Buf* b;
    u64 acc;
    int na;
    void init(Buf* buf) {
        b = buf;
        acc = 0;
        na = 0;
    }
    inline void put(u32 v, int n) {  // n <= 32, v < 2^n
        if (!n) return;
        acc = (acc << n) | v;
        na += n;
        while (na >= 8) {
            na -= 8;
            b->put((u8)(acc >> na));
        }
    }
    inline void put64(u64 v, int n) {
        if (n > 32) {
            put((u32)(v >> 32) & (u32)((1ull << (n - 32)) - 1), n - 32);
            put((u32)v, 32);
        } else if (n) {
            put((u32)(v & ((1ull << n) - 1)), n);
        }
    }
    inline void puts(s64 v, int n) { put64((u64)v, n); }
    inline void zeros(u64 q) {
        while (q >= 32) {
            put(0, 32);
            q -= 32;
        }
        put(0, (int)q);
    }
    inline void rice(u64 u, int k) {
        zeros(u >> k);
        if (k < 32)
            put((u32)((1ull << k) | (u & ((1ull << k) - 1))), k + 1);
        else {
            put(1, 1);
            put64(u, k);
        }
    }
    void align() {
        if (na) put(0, 8 - na);
    }
};

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

static void alloc_work() {
    for (int c = 0; c < MAXCH; c++) {
        g_sig[c] = (s64*)xmalloc(sizeof(s64) * MAXBS);
        g_pcm[c] = (s32*)xmalloc(sizeof(s32) * MAXBS);
    }
    g_x = (s64*)xmalloc(sizeof(s64) * MAXBS);
    g_res = (s64*)xmalloc(sizeof(s64) * MAXBS);
    g_res2 = (s64*)xmalloc(sizeof(s64) * MAXBS);

    g_simx = (s64*)xmalloc(sizeof(s64) * MAXBS);
    g_parambuf = (u8*)xcalloc((size_t)MAXCH * MAXPART * 2);
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

// fast LPC residual for 32-bit-safe signals (analysis only)
template <int O> static void lpc_res_t(const s32* x, int n, const s32* c, int shift, s64* r) {
    for (int i = O; i < n; i++) {
        s64 s = 0;
        const s32* h = x + i;
        for (int j = 0; j < O; j++) s += (s64)c[j] * h[-1 - j];
        r[i - O] = (s64)x[i] - (s >> shift);
    }
}
typedef void (*LpcResFn)(const s32*, int, const s32*, int, s64*);
static LpcResFn g_lpcres[33];
template <int O> struct LpcResInit {
    static void run() {
        g_lpcres[O] = lpc_res_t<O>;
        LpcResInit<O - 1>::run();
    }
};
template <> struct LpcResInit<0> {
    static void run() {}
};
static s32* g_x32;
// residual using a 32-bit copy of x when possible (x32 = nullptr: generic)
static void lpc_residual_fast(const s64* x, const s32* x32, int n, const s32* c, int order, int shift, s64* r) {
    if (x32 && order <= 32)
        g_lpcres[order](x32, n, c, shift, r);
    else
        lpc_residual(x, n, c, order, shift, r);
}
// 32-bit copy if all samples fit (and products cannot overflow 64-bit sums)
static const s32* to32(const s64* x, int n) {
    for (int i = 0; i < n; i++) {
        s64 v = x[i];
        if (v < -(1ll << 31) || v >= (1ll << 31)) return nullptr;
        g_x32[i] = (s32)v;
    }
    return g_x32;
}

static void init_fast() {
    g_x32 = (s32*)xmalloc(sizeof(s32) * MAXBS);
    LpcResInit<32>::run();
}

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

static void encode_frame(const FrameInfo& f, s32** pcm, Buf& out) {
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
}

// ---------------------------------------------------------------------------
// Binary arithmetic coder (carry-less range coder) + adaptive bit models

struct BM {
    u16 p;  // P(bit==1) * 65536
    u16 n;
};
static int g_rate[256];
static void init_rates() {
    for (int i = 0; i < 256; i++) g_rate[i] = (int)(65536.0 / (i + 1.6));
}
static inline void bm_init(BM* m, size_t cnt) {
    for (size_t i = 0; i < cnt; i++) {
        m[i].p = 32768;
        m[i].n = 0;
    }
}
template <class T> static void model_init(T& t) { bm_init((BM*)&t, sizeof(T) / sizeof(BM)); }

enum {
    C_HDR, C_BS, C_CH, C_WASTED, C_TYPE, C_ORDER, C_PREC, C_COEF, C_PORDER, C_PARAM, C_ESC, C_META, C_RAW, C_SEEK, NCAT
};
static const char* k_catname[NCAT] = {"header", "blocksize", "chmode", "wasted", "subtype", "order",  "prec/shift",
                                      "coefs",  "porder",    "riceparm", "escape", "metadata", "raw", "seektable"};

struct AC {
    int dec;
    u32 x1, x2, x;
    Buf* out;
    const u8* in;
    size_t ip, in_n;
    int cat;
    double cost[NCAT];
    int limit;  // adaptation count limit
    void enc_init(Buf* o) {
        dec = 0;
        x1 = 0;
        x2 = 0xFFFFFFFF;
        out = o;
        cat = 0;
        limit = 30;
        for (int i = 0; i < NCAT; i++) cost[i] = 0;
    }
    void dec_init(const u8* p, size_t n) {
        dec = 1;
        x1 = 0;
        x2 = 0xFFFFFFFF;
        in = p;
        in_n = n;
        ip = 0;
        x = 0;
        cat = 0;
        limit = 30;
        for (int i = 0; i < 4; i++) x = (x << 8) | byte();
    }
    inline u32 byte() { return ip < in_n ? in[ip++] : 255; }
    inline int code(int bit, u32 p16) {
        if (p16 < 16) p16 = 16;
        if (p16 > 65520) p16 = 65520;
        u32 xmid = x1 + (u32)(((u64)(x2 - x1) * p16) >> 16);
        if (dec)
            bit = x <= xmid;
        else if (g_verbose)
            cost[cat] -= log2(bit ? p16 / 65536.0 : 1.0 - p16 / 65536.0);
        if (bit)
            x2 = xmid;
        else
            x1 = xmid + 1;
        while (((x1 ^ x2) & 0xFF000000) == 0) {
            if (!dec)
                out->put((u8)(x2 >> 24));
            else
                x = (x << 8) | byte();
            x1 <<= 8;
            x2 = (x2 << 8) | 255;
        }
        return bit;
    }
    void flush() {
        if (!dec) out->put((u8)(x1 >> 24));
    }
    inline int bit(BM& m, int b) {
        b = code(b, m.p);
        int target = b ? 65535 : 0;
        m.p = (u16)(m.p + (int)(((s64)(target - (int)m.p) * g_rate[m.n]) >> 16));
        if (m.n < limit) m.n++;
        return b;
    }
    inline u64 raw(u64 v, int n) {  // equiprobable bits
        u64 r = 0;
        for (int i = n - 1; i >= 0; i--) r = (r << 1) | (u64)code((int)((v >> i) & 1), 32768);
        return r;
    }
    inline u32 tree(BM* t, int n, u32 v) {  // t[1 << n]
        u32 c = 1;
        for (int i = n - 1; i >= 0; i--) c = (c << 1) | (u32)bit(t[c], (int)((v >> i) & 1));
        return c - (1u << n);
    }
};

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

static int g_stretch[4096];
static int g_squash[4096];
static void init_squash() {
    for (int i = 0; i < 4096; i++) {
        double x = (i - 2048) / 256.0;
        g_squash[i] = (int)(4096.0 / (1.0 + exp(-x)));
        if (g_squash[i] > 4095) g_squash[i] = 4095;
        if (g_squash[i] < 1) g_squash[i] = 1;
    }
    int pi = 0;
    for (int x = -2047; x <= 2047; x++) {
        int v = g_squash[x + 2048];
        for (int j = pi; j <= v; j++) g_stretch[j] = x;
        pi = v + 1;
    }
    for (int j = pi; j < 4096; j++) g_stretch[j] = 2047;
}
static inline int squash(int x) {
    if (x > 2047) x = 2047;
    if (x < -2047) x = -2047;
    return g_squash[x + 2048];
}

struct ByteCM {
    enum { N = 4, H2 = 1 << 16, H3 = 1 << 18 };
    u16* t0;
    u16* t1;
    u16* t2;
    u16* t3;
    int w[256][N + 1];
    u32 c1, c2, c3;
    void init() {
        t0 = (u16*)xmalloc(256 * 2);
        t1 = (u16*)xmalloc(65536 * 2);
        t2 = (u16*)xmalloc(H2 * 2);
        t3 = (u16*)xmalloc(H3 * 2);
        for (int i = 0; i < 256; i++) t0[i] = 2048;
        for (int i = 0; i < 65536; i++) t1[i] = 2048;
        for (int i = 0; i < H2; i++) t2[i] = 2048;
        for (int i = 0; i < H3; i++) t3[i] = 2048;
        for (int i = 0; i < 256; i++)
            for (int j = 0; j <= N; j++) w[i][j] = j < N ? 20000 : 0;
        c1 = c2 = c3 = 0;
    }
    void release() {
        free(t0);
        free(t1);
        free(t2);
        free(t3);
    }
    // code one byte; if ac==null just measure cost (bits) and update
    int byte(AC* ac, int c, double* cost) {
        u32 cx = 1;
        u32 h2 = (c1 | (c2 << 8)) * 0x9E3779B1u;
        u32 h3 = (c1 | (c2 << 8) | (c3 << 16)) * 0x85EBCA77u;
        for (int i = 7; i >= 0; i--) {
            u16* e[N];
            e[0] = &t0[cx];
            e[1] = &t1[(c1 << 8) | cx];
            e[2] = &t2[((h2 >> 16) ^ (cx * 0x2F)) & (H2 - 1)];
            e[3] = &t3[((h3 >> 14) ^ (cx * 0x3B1)) & (H3 - 1)];
            int st[N];
            s64 dot = 0;
            int* ww = w[cx];
            for (int j = 0; j < N; j++) {
                st[j] = g_stretch[*e[j]];
                dot += (s64)st[j] * ww[j];
            }
            dot += ww[N] * 256;
            int p = squash((int)(dot >> 16));
            int bit = (c >> i) & 1;
            if (ac)
                bit = ac->code(bit, (u32)p << 4);
            else if (cost)
                *cost -= log2(bit ? p / 4096.0 : 1.0 - p / 4096.0);
            int err = ((bit << 12) - p) * 6;
            for (int j = 0; j < N; j++) ww[j] += (st[j] * err) >> 10;
            ww[N] += (256 * err) >> 10;
            for (int j = 0; j < N; j++) {
                int t = *e[j];
                *e[j] = (u16)(t + ((((bit << 12) - t) * (j == 0 ? 24 : 40)) >> 8));
                if (*e[j] < 1) *e[j] = 1;
                if (*e[j] > 4095) *e[j] = 4095;
            }
            cx = (cx << 1) | bit;
        }
        c = cx & 255;
        c3 = c2;
        c2 = c1;
        c1 = (u32)c;
        return c;
    }
};

static const char* k_prime_text =
    "reference libFLAC 1.2.1 20070917reference libFLAC 1.3.0 20130526reference libFLAC 1.3.1 20141125"
    "reference libFLAC 1.3.2 20170101reference libFLAC 1.3.3 20190804reference libFLAC 1.3.4 20220220"
    "reference libFLAC 1.4.0 20220909reference libFLAC 1.4.1 20220922reference libFLAC 1.4.2 20221022"
    "reference libFLAC 1.4.3 20230623reference libFLAC 1.5.0 20250211reference libFLAC 1.1.4 20070213"
    "reference libFLAC 1.1.2 20050205reference libFLAC 1.2.0 20070715Lavf58.76.100Lavf60.16.100Lavf61.7.100"
    "TITLE=ARTIST=ALBUM=DATE=TRACKNUMBER=TRACKTOTAL=DISCNUMBER=GENRE=COMMENT=ALBUMARTIST=COMPOSER="
    "ENCODER=REPLAYGAIN_TRACK_GAIN=REPLAYGAIN_TRACK_PEAK=REPLAYGAIN_ALBUM_GAIN=REPLAYGAIN_ALBUM_PEAK=";

// Code a byte string (length known to both sides). Chooses CM or stored.
static void code_bytes(AC& ac, u8* data, size_t len, bool prime) {
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
            for (size_t i = 0; i < len; i++) t.byte(nullptr, data[i], &cost);
            t.release();
            stored = cost > (double)len * 8;
        }
    }
    stored = ac.code(stored, 32768);
    if (stored) {
        ac.cat = C_RAW;
        for (size_t i = 0; i < len; i++) data[i] = (u8)ac.raw(data[i], 8);
    } else {
        for (size_t i = 0; i < len; i++) data[i] = (u8)cm.byte(&ac, data[i], nullptr);
    }
    cm.release();
    ac.cat = savecat;
}

// ---------------------------------------------------------------------------
// LPC / rice estimation helpers used as predictors for the meta model

// libFLAC fixed order guess (data starts at index 4), two variants of tie rule
static void fixed_guess(const s64* x, int n, int& g_new, int& g_old) {
    if (n <= 4) {
        g_new = g_old = 0;
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

// Per-subframe residual statistics for porder / rice parameter prediction
struct ResStats {
    int bs, order, maxpo;
    // sums per order: abs sums (libFLAC) and zigzag sums (ffmpeg); layout: offset[o] .. +2^o
    u64* asum;
    u64* zsum;
    int off[16];
    void alloc() {
        asum = (u64*)xmalloc(sizeof(u64) * 2 * MAXPART + 64);
        zsum = (u64*)xmalloc(sizeof(u64) * 2 * MAXPART + 64);
    }
    void compute(const s64* r, int bs_, int order_, bool withz = true) {
        bs = bs_;
        order = order_;
        maxpo = 0;
        while (maxpo < 15 && !((bs >> maxpo) & 1) && ((u32)bs >> (maxpo + 1)) >= (u32)order) maxpo++;
        // offsets: highest order first
        int o = 0;
        for (int po = maxpo; po >= 0; po--) {
            off[po] = o;
            o += 1 << po;
        }
        int np = 1 << maxpo;
        u32 psz = (u32)bs >> maxpo;
        const s64* p = r;
        u64* A = asum + off[maxpo];
        u64* Z = zsum + off[maxpo];
        for (int i = 0; i < np; i++) {
            u32 n = psz - (i == 0 ? order : 0);
            u64 a = 0, z = 0;
            if (withz) {
                for (u32 j = 0; j < n; j++) {
                    s64 v = p[j];
                    a += (u64)(v < 0 ? -v : v);
                    z += ((u64)v << 1) ^ (u64)(v >> 63);
                }
            } else {
                for (u32 j = 0; j < n; j++) {
                    s64 v = p[j];
                    a += (u64)(v < 0 ? -v : v);
                }
            }
            p += n;
            A[i] = a;
            Z[i] = z;
        }
        for (int po = maxpo - 1; po >= 0; po--) {
            u64* a2 = asum + off[po];
            u64* a1 = asum + off[po + 1];
            u64* z2 = zsum + off[po];
            u64* z1 = zsum + off[po + 1];
            for (int i = 0; i < (1 << po); i++) {
                a2[i] = a1[2 * i] + a1[2 * i + 1];
                if (withz) z2[i] = z1[2 * i] + z1[2 * i + 1];
            }
        }
    }
    u32 psamples(int po, int i) const { return ((u32)bs >> po) - (i == 0 ? order : 0); }
    // libFLAC estimate for partition i of order po
    int est_lf(int po, int i, int lim, bool newv) const {
        u32 ps = psamples(po, i);
        if (ps == 0) return 0;
        u64 m = asum[off[po] + i];
        int k = newv ? rice_est_new(m, ps) : rice_est_old(m, ps);
        return Min(k, lim);
    }
    int est_ff(int po, int i, int lim) const {
        u32 ps = psamples(po, i);
        if (ps == 0) return 0;
        return rice_est_ff(zsum[off[po] + i], ps, lim);
    }
    // best partition order libFLAC style in [lo,hi] (hi already limited)
    // minimal libFLAC-estimated residual bits over partition orders [lo,hi]
    u64 bits_lf(int lo, int hi, int lim, bool newv) const {
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) <= (u32)order && po > 0) continue;
            u64 bits = 6;
            for (int i = 0; i < (1 << po); i++) {
                u32 ps = psamples(po, i);
                int k = est_lf(po, i, lim, newv);
                bits += lf_bits(k, ps, asum[off[po] + i]);
            }
            if (bits < bb) bb = bits;
        }
        return bb;
    }
    int best_lf(int lo, int hi, int lim, bool newv) const {
        int best = hi;
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) <= (u32)order && po > 0) continue;
            u64 bits = 6;
            for (int i = 0; i < (1 << po); i++) {
                u32 ps = psamples(po, i);
                int k = est_lf(po, i, lim, newv);
                bits += lf_bits(k, ps, asum[off[po] + i]);
            }
            if (bits < bb) {
                bb = bits;
                best = po;
            }
        }
        return best;
    }
    u64 bits_ff(int lo, int hi, int lim) const {
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) < (u32)order) continue;
            u64 bits = 4ull << po;
            for (int i = 0; i < (1 << po); i++) {
                u32 n = psamples(po, i);
                int k = est_ff(po, i, lim);
                u64 s = zsum[off[po] + i];
                bits += (u64)n * (k + 1) + ((s - (n >> 1)) >> k);
            }
            if (bits < bb || po == hi) bb = Min(bb, bits);
        }
        return bb;
    }
    int best_ff(int lo, int hi, int lim) const {
        int best = hi;
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) < (u32)order) continue;
            u64 bits = 4ull << po;
            for (int i = 0; i < (1 << po); i++) {
                u32 n = psamples(po, i);
                int k = est_ff(po, i, lim);
                u64 s = zsum[off[po] + i];
                bits += (u64)n * (k + 1) + ((s - (n >> 1)) >> k);
            }
            if (bits < bb) {
                bb = bits;
                best = po;
            }
        }
        return best;
    }
};

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

static const double k_pi = 3.14159265358979323846;

// window variants: v14 = libFLAC >= 1.3 (cosf), v12 = libFLAC <= 1.2 (cos)
// 0.5 - 0.5*cos(pi*i/N): libFLAC <= 1.3.3 uses double cos, >= 1.3.4 cosf
static inline float hann_tap(int i, int N, bool v12) {
    if (v12) return (float)(0.5 - 0.5 * det_cos(k_pi * i / N));
    return 0.5f - 0.5f * det_cosf((float)(k_pi * i / N));
}

static void win_tukey(float* w, int L, float p, bool v12) {
    for (int n = 0; n < L; n++) w[n] = 1.0f;
    if (p <= 0.0f) return;
    int Np = (int)(p / 2.0f * (float)L) - 1;
    if (Np > 0) {
        for (int n = 0; n <= Np; n++) {
            if (v12) {
                w[n] = (float)(0.5 - 0.5 * det_cos(k_pi * n / Np));
                w[L - Np - 1 + n] = (float)(0.5 - 0.5 * det_cos(k_pi * (n + Np) / Np));
            } else {
                w[n] = 0.5f - 0.5f * det_cosf((float)(k_pi * n / Np));
                w[L - Np - 1 + n] = 0.5f - 0.5f * det_cosf((float)(k_pi * (n + Np) / Np));
            }
        }
    }
}
static void win_partial_tukey(float* w, int L, float p, float start, float end, bool v12) {
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
static void win_punchout_tukey(float* w, int L, float p, float start, float end, bool v12) {
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
    NWIN
};

struct WinCache {
    int bs;
    float* w[NWIN];
    void init() {
        bs = 0;
        for (int i = 0; i < NWIN; i++) w[i] = (float*)xmalloc(sizeof(float) * MAXBS);
    }
    void ensure(int L) {
        if (L == bs) return;
        bs = L;
        win_tukey(w[W_T50], L, 0.5f, false);
        win_tukey(w[W_T50_12], L, 0.5f, true);
        win_tukey(w[W_T25], L, 0.5f / 2, false);
        win_tukey(w[W_T16], L, 0.5f / 3, false);
        {
            float overlap = 0.1f;
            float ou = 1.0f / (1.0f - overlap) - 1.0f;
            int parts = 2;
            for (int m = 0; m < parts; m++) {
                win_partial_tukey(w[W_PT2_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), false);
                win_partial_tukey(w[W_PT2C_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), true);
            }
        }
        {
            float overlap = 0.2f;
            float ou = 1.0f / (1.0f - overlap) - 1.0f;
            int parts = 3;
            for (int m = 0; m < parts; m++) {
                win_punchout_tukey(w[W_PO3_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), false);
                win_punchout_tukey(w[W_PO3C_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), true);
            }
        }
    }
};

// autocorrelation variants
enum { AC_DBL = 0, AC_FASC, AC_FNEW8, AC_FNEW12, AC_FNEW16, AC_X87, AC_FFMA, NAC };

static void autoc_compute(const float* d, int n, int lags, int mode, double* out) {
    if (mode == AC_DBL) {
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
    double error_scale = 0.5 / (double)total_samples;
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

struct CandDef {
    u8 kind;  // 0 full window, 1 subdivide partial, 2 subdivide punchout, 3 ffmpeg
    u8 win;
    u8 b, c;
    u8 quirk;
    u8 ac;
    u8 fma;  // Levinson compiled with fma contraction
};

enum { MAXCAND = 96 };

struct LpcPredictor {
    WinCache wc;
    CandDef cand[MAXCAND];
    int ncand;
    int mru[MAXCAND];  // candidate ids in rank order
    u32 hits[MAXCAND];
    u32 nsub;   // LPC subframes seen
    u32 nmiss;  // LPC subframes without exact candidate
    float* wd;
    // per-subframe memo of autocorrelations
    struct Memo {
        u8 kind, win, b, c, ac, nl;
        double v[33];
    } memo[160];
    int nmemo;
    const s64* x;
    int n;
    float lpall[32 * 32];
    double lerr[32];
    double lpd[32 * 32];  // ffmpeg coefficients (double)
    double* ffwbuf;

    void add(int kind, int win, int b, int c, int quirk, int acm, int fm = 0) {
        if (ncand >= MAXCAND) die("internal: too many candidates");
        CandDef& d = cand[ncand];
        d.kind = (u8)kind;
        d.win = (u8)win;
        d.b = (u8)b;
        d.c = (u8)c;
        d.quirk = (u8)quirk;
        d.ac = (u8)acm;
        d.fma = (u8)fm;
        mru[ncand] = ncand;
        hits[ncand] = 0;
        ncand++;
    }
    void init() {
        wc.init();
        wd = (float*)xmalloc(sizeof(float) * (MAXBS + 64));
        ffwbuf = (double*)xcalloc(sizeof(double) * (MAXBS + 80));
        ncand = 0;
        nsub = 0;
        nmiss = 0;
        add(0, W_T50, 0, 0, 0, AC_DBL);         // libFLAC 1.4 -0..-5
        add(3, 0, 0, 0, 0, 0);                  // ffmpeg flacenc
        add(0, W_T50_12, 0, 0, 0, AC_FASC);     // libFLAC 1.2/1.3.0-1.3.3
        add(0, W_T50_12, 0, 0, 0, AC_FNEW12);   // libFLAC <= 1.3.3 x86-64 -3..-6
        add(0, W_T50, 0, 0, 0, AC_FNEW12);      // libFLAC 1.3.4 x86-64
        add(0, W_T50, 0, 0, 0, AC_FASC);        // libFLAC 1.3.4 generic
        add(0, W_T50_12, 0, 0, 0, AC_X87);      // libFLAC 1.2 ia32 -7/-8
        add(0, W_T50_12, 0, 0, 0, AC_FNEW16);   // libFLAC <= 1.3.3 -7/-8
        add(0, W_PT2C_0, 0, 0, 0, AC_FNEW16);
        add(0, W_PT2C_1, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_0, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_1, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_2, 0, 0, 0, AC_FNEW16);
        add(0, W_PT2C_0, 0, 0, 0, AC_FASC);
        add(0, W_PT2C_1, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_0, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_1, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_2, 0, 0, 0, AC_FASC);
        add(0, W_PT2C_0, 0, 0, 0, AC_FNEW12);  // <= 1.3.3 -6
        add(0, W_PT2C_1, 0, 0, 0, AC_FNEW12);
        // libFLAC 1.4 subdivide_tukey(2)
        add(0, W_T25, 0, 0, 0, AC_DBL);
        add(1, W_T25, 2, 0, 0, AC_DBL);
        add(1, W_T25, 2, 2, 0, AC_DBL);
        // libFLAC 1.4 subdivide_tukey(3)
        add(0, W_T16, 0, 0, 0, AC_DBL);
        add(1, W_T16, 2, 0, 0, AC_DBL);
        add(1, W_T16, 2, 2, 0, AC_DBL);
        for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16, 3, c, 0, AC_DBL);
        for (int c = 1; c < 6; c += 2) add(2, W_T16, 3, c, 1, AC_DBL);
        // libFLAC 1.3.4 -6..-8
        add(0, W_T50, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FNEW12);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FASC);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FASC);
        // misc
        add(0, W_T50, 0, 0, 0, AC_FNEW8);
        add(0, W_T50_12, 0, 0, 0, AC_FNEW8);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_DBL);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_DBL);
        add(0, W_T50_12, 0, 0, 0, AC_DBL);
        add(0, W_T50, 0, 0, 0, AC_X87);
        // builds with fma contraction (arm64 etc.)
        add(0, W_T50, 0, 0, 0, AC_DBL, 1);
        add(0, W_T25, 0, 0, 0, AC_DBL, 1);
        add(1, W_T25, 2, 0, 0, AC_DBL, 1);
        add(1, W_T25, 2, 2, 0, AC_DBL, 1);
        add(0, W_T16, 0, 0, 0, AC_DBL, 1);
        add(1, W_T16, 2, 0, 0, AC_DBL, 1);
        add(1, W_T16, 2, 2, 0, AC_DBL, 1);
        for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16, 3, c, 0, AC_DBL, 1);
        add(0, W_T50_12, 0, 0, 0, AC_FFMA, 1);
        add(0, W_T50, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 2; k++) add(0, W_PT2C_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3C_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_X87);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_X87);
    }
    void begin(const s64* x_, int n_) {
        x = x_;
        n = n_;
        nmemo = 0;
        wc.ensure(n);
    }
    // autocorrelation of (window kind) with lags 0..maxlag
    const double* autoc(int kind, int win, int b, int c, int acm, int maxlag) {
        int lags = maxlag + 1;
        Memo* mp = nullptr;
        for (int i = 0; i < nmemo; i++) {
            Memo& m = memo[i];
            if (m.kind == kind && m.win == win && m.b == b && m.c == c && m.ac == acm) {
                if (m.nl >= lags) return m.v;
                mp = &m;
                break;
            }
        }
        if (!mp) mp = &memo[nmemo < 160 ? nmemo++ : 159];
        Memo& m = *mp;
        m.kind = (u8)kind;
        m.win = (u8)win;
        m.b = (u8)b;
        m.c = (u8)c;
        m.ac = (u8)acm;
        m.nl = (u8)lags;
        const float* w = wc.w[win];
        if (kind == 0) {
            for (int i = 0; i < n; i++) wd[i] = (float)x[i] * w[i];
            autoc_compute(wd, n, lags, acm, m.v);
        } else {
            // FLAC__lpc_window_data_partial
            int part_size = n / b / 2;
            int data_shift = (c / 2 * n) / b;
            int len = n / b;
            if (part_size + data_shift < n) {
                int i, j;
                for (i = 0; i < part_size; i++) wd[i] = (float)x[data_shift + i] * w[i];
                i = Min(i, n - part_size - data_shift);
                for (j = n - part_size; j < n; i++, j++) wd[i] = (float)x[data_shift + i] * w[j];
                if (i < n) wd[i] = 0.0f;
            }
            autoc_compute(wd, len, lags, acm, m.v);
        }
        return m.v;
    }
    // run candidate id up to max order mo: fills lpall/lerr, returns usable max order (0 = unusable)
    int eval(int id, int mo) {
        const CandDef& d = cand[id];
        double a[33];
        if (mo >= n) mo = n - 1;
        if (mo < 1) return 0;
        if (d.kind == 3) {
            if (n < 5) return 0;
            double* w = ffwbuf + 40;
            int sh33 = d.b;  // analysis on x>>1 for 33-bit side channels
            if (sh33) {
                static s64 tmp[MAXBS];
                for (int i = 0; i < n; i++) tmp[i] = x[i] >> 1;
                ff_welch(tmp, n, w);
            } else
                ff_welch(x, n, w);
            w[-1] = 0.0;
            w[n] = 0.0;
            w[n + 1] = 0.0;
            ff_autocorr(w, n, mo, a);
            ff_levinson(a, mo, lpd);
            return mo;
        }
        if ((d.kind == 1 || d.kind == 2) && n / d.b <= 32) return 0;
        if (d.kind == 2) {
            const double* root = autoc(0, d.win, 0, 0, d.ac, mo);
            const double* part = autoc(1, d.win, d.b, d.c - 1, d.ac, mo);
            for (int i = 0; i <= mo; i++) a[i] = root[i] - part[i];
            if (d.quirk) a[mo] = part[mo];
        } else {
            const double* v = autoc(d.kind, d.win, d.b, d.c, d.ac, mo);
            for (int i = 0; i <= mo; i++) a[i] = v[i];
        }
        if (a[0] == 0.0) return 0;
        return levinson_all(a, mo, lpall, lerr, d.fma);
    }
    // quantized coefficients of the last eval()ed candidate id at an order (forced_sh >= 0: at that shift)
    bool quant(int id, int order, int prec, s32* q, int* sh, int forced_sh = -1) {
        if (cand[id].kind == 3) {
            ff_quantize(lpd + (order - 1) * 32, order, prec, q, sh, forced_sh);
            return true;
        }
        if (forced_sh >= 0) {
            lpc_quantize_at(lpall + (order - 1) * 32, order, prec, forced_sh, q);
            *sh = forced_sh;
            return true;
        }
        return lpc_quantize(lpall + (order - 1) * 32, order, prec, q, sh);
    }
    // candidate id at exactly this order -> quantized coefficients
    bool at_order(int id, int order, int prec, s32* q, int* sh) {
        int mo = eval(id, order);
        if (mo < order) return false;
        return quant(id, order, prec, q, sh);
    }
};

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
    BM k_eq1[8];
    BM k_eq2[8];
    BM k_eq3[8];
    BM k_t[32][32];
    BM esc_t[33][32];
    // misc
    UModel len_m;
    UModel num_m;
    BM mflag[16];
    BM mtype[8][128];
};

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

static bool model_residual(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* res, int bs) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ResStats& rs = cx.rs;
    rs.compute(res, bs, s.order);
    int lim = cx.sp.bps > 16 ? 30 : 14;
    // partition order
    ac.cat = C_PORDER;
    int hi = Min(st.maxpo, rs.maxpo);
    while (hi > 0 && ((u32)bs >> hi) <= (u32)s.order) hi--;
    int lo = Min(st.minpo, hi);
    int pL = rs.best_lf(lo, hi, lim, true);
    int pF = rs.best_ff(lo, Min(st.maxpo, rs.maxpo), lim);
    int po = s.porder;
    int c0 = (pL == pF);
    int e = ac.bit(M.po_eqL[c0], po == pL);
    if (e)
        po = pL;
    else {
        int e2 = 0;
        if (pF != pL) e2 = ac.bit(M.po_eqF[0], po == pF);
        if (e2)
            po = pF;
        else
            po = ac.tree(M.po_t[pL], 4, po);
    }
    s.porder = po;
    if (po > rs.maxpo) {
        if (!ac.dec) die("internal: porder");
        return false;
    }
    if (po < st.minpo) st.minpo = po;
    if (po > st.maxpo) st.maxpo = po;
    // rice parameters (escape coded as 31)
    ac.cat = C_PARAM;
    int np = 1 << po;
    int esc_code = s.method ? 31 : 15;
    int need2 = 0;
    u32 roff = 0;
    for (int i = 0; i < np; i++) {
        u32 pn = rs.psamples(po, i);
        int kn = rs.est_lf(po, i, lim, true);
        int ko = rs.est_lf(po, i, lim, false);
        int kf = rs.est_ff(po, i, lim);
        int k = ac.dec ? 0 : (s.param[i] == esc_code ? 31 : s.param[i]);
        int ctx = (kn == ko) + 2 * (kn == kf) + 4 * (i == 0);
        int kvote = kn != ko;
        if (ac.bit(M.k_eq1[ctx], k == kn))
            k = kn;
        else {
            int done = 0;
            if (ko != kn) {
                if (ac.bit(M.k_eq2[ctx], k == ko)) {
                    k = ko;
                    done = 1;
                }
            }
            if (!done && kf != kn && kf != ko) {
                if (ac.bit(M.k_eq3[ctx], k == kf)) {
                    k = kf;
                    done = 1;
                }
            }
            if (!done) k = ac.tree(M.k_t[kn], 5, k);
        }
        if (kvote && k == kn) st.rvote[1]++;
        if (kvote && k == ko) st.rvote[0]++;
        if (k == 31) {
            ac.cat = C_ESC;
            // predicted raw bits: minimal signed width
            u32 n = pn;
            u64 mx = 0;
            for (u32 j = 0; j < n; j++) {
                s64 v = res[roff + j];
                u64 a = v < 0 ? (u64)(~v) : (u64)v;
                mx |= a;
            }
            int pred = n ? bitlen(mx) + 1 : 0;
            if (mx == 0) pred = 0;
            int rb = ac.tree(M.esc_t[Min(pred, 32)], 5, s.raw[i]);
            s.raw[i] = (u8)rb;
            ac.cat = C_PARAM;
        } else {
            if (k > 14) need2 = 1;
        }
        s.param[i] = (u8)k;  // temporarily 31 = escape
        roff += pn;
    }
    ac.cat = C_PARAM;
    if (need2)
        s.method = 1;
    else
        s.method = ac.bit(M.method_b[cs.pmethod][0], s.method);
    cs.pmethod = s.method;
    if (!s.method)
        for (int i = 0; i < np; i++)
            if (s.param[i] == 31) s.param[i] = 15;
    if (st.rvote[0] + st.rvote[1] >= 8) st.rice_new = st.rvote[1] > st.rvote[0];
    return true;
}

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
        double e = P.lerr[o - 1], ebs;
        if (e > 0.0) {
            ebs = 0.5 * det_log2(0.5 / (double)(bs - o) * e);
            if (ebs < 0.0) ebs = 0.0;
        } else
            ebs = e < 0.0 ? 1e32 : 0.0;
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
        u64 bits = (u64)o * (prec + ebps) + rs.bits_lf(lo, hi, lim, newv);
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
        if (mo) gn = lpc_best_order(P.lerr, mo, bs, ebps + (st.lpc_ov ? 5 : prec));
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

static bool model_lpc(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* x, int bs, int ebps,
                      const SimOut& sim) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    LpcPredictor& P = *cx.lp;
    P.begin(x, bs);
    s32 q[32];
    int sh = 0;
    // encoder-side search budget: candidates that never matched are only tried when
    // exact matches are common (or occasionally), to bound the time spent on unknown encoders
    int try_unhit = P.nsub < 64 || P.nmiss * 4 < P.nsub || (P.nsub & 15) == 0;
    P.nsub++;
    // stage 0: the simulated encoder's choice (window + order)
    if (sim.type == ST_LPC && sim.wid >= 0 && sim.order <= bs - 1) {
        ac.cat = C_ORDER;
        int id = sim.wid;
        int maxo = Min(st.lpc_exh ? st.lpc_maxseen : st.lpc_maxo, bs - 1);
        int ok = 0;
        int mo = P.eval(id, Max(maxo, sim.order));
        if (mo >= sim.order && P.quant(id, sim.order, s.prec, q, &sh)) ok = 1;
        int hit = ok && !ac.dec && sim.order == s.order && sh == s.shift && !memcmp(q, s.coef, sizeof(s32) * s.order);
        hit = ac.bit(M.lpc_sim[ok][cs.psim], hit);
        cs.psim = hit;
        st.dbg[hit]++;
        if (!hit && !ac.dec && g_verbose >= 4) {
            int ge = lpc_exh_guess(cx, st, id, st.lpc_maxseen, x, bs, ebps);
            fprintf(stderr, "s0miss: sim order %d wid %d, actual order %d prec %d, exhguess %d ok=%d ebps=%d mp=%d\n", sim.order, id, s.order, s.prec, ge, ok, ebps, st.maxprec);
        }
        if (hit) {
            if (!ok) return false;
            s.order = sim.order;
            s.shift = sh;
            memcpy(s.coef, q, sizeof(s32) * s.order);
            int r = 0;
            while (r < P.ncand && P.mru[r] != id) r++;
            if (r < P.ncand) mru_hit(P, r);
            st.lfh++;
            cs.plorder = s.order - 1;
            cs.pshift = s.shift;
            cs.prank = 0;
            lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
            return true;
        }
    }
    // stage 1: window candidate with the encoder's own order choice
    ac.cat = C_ORDER;
    int found = -1;
    if (!ac.dec) {
        // (never-matched candidates are left to the cheaper stage-2 search)
        for (int pass = 0; pass < 2 && found < 0; pass++) {
            if (pass && (!try_unhit || P.nsub > 64)) break;
            for (int r = 0; r < P.ncand; r++) {
                int id = P.mru[r];
                if ((P.hits[id] == 0) != (pass == 1)) continue;
                int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
                if (g != s.order) continue;
                if (!P.quant(id, g, s.prec, q, &sh)) continue;
                if (sh != s.shift || memcmp(q, s.coef, sizeof(s32) * g)) continue;
                found = r;
                break;
            }
        }
    }
    int r;
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_g[Min(r, 23)][cs.prank], r == found)) break;
    st.dbg[2 + (r < P.ncand)]++;
    if (r < P.ncand) {
        int id = P.mru[r];
        int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
        if (g < 1 || g > bs || !P.quant(id, g, s.prec, q, &sh)) return false;
        s.order = g;
        s.shift = sh;
        memcpy(s.coef, q, sizeof(s32) * g);
        mru_hit(P, r);
        if (P.cand[id].kind == 3) st.ffh++; else st.lfh++;
        if (st.ffh > 8 && st.ffh > st.lfh) st.sim_on = 0;
        cs.prank = r == 0 ? 0 : r < 4 ? 1 : 2;
        cs.plorder = s.order - 1;
        cs.pshift = s.shift;
        lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
        return true;
    }
    // stage 2: explicit order, then window candidate at that order
    s.order = ac.tree(M.lorder_t[cs.plorder], 5, s.order - 1) + 1;
    if (s.order > bs) return false;
    cs.plorder = s.order - 1;
    ac.cat = C_COEF;
    found = -1;
    if (!ac.dec) {
        for (int pass = 0; pass < 2 && found < 0; pass++) {
            if (pass && !try_unhit) break;
            for (int k = 0; k < P.ncand; k++) {
                int id = P.mru[k];
                if ((P.hits[id] == 0) != (pass == 1)) continue;
                if (!P.at_order(id, s.order, s.prec, q, &sh)) continue;
                if (sh != s.shift || memcmp(q, s.coef, sizeof(s32) * s.order)) continue;
                found = k;
                break;
            }
        }
    }
    if (!ac.dec && found < 0 && g_verbose >= 3) {
        fprintf(stderr, "miss: n=%d order=%d prec=%d shift=%d coefs:", bs, s.order, s.prec, s.shift);
        for (int i = 0; i < s.order; i++) fprintf(stderr, " %d", s.coef[i]);
        fprintf(stderr, "\n");
        for (int id = 0; id < P.ncand; id++) {
            if (!P.at_order(id, s.order, s.prec, q, &sh)) continue;
            int nd = 0, md = 0;
            for (int i = 0; i < s.order; i++) {
                int d = q[i] - s.coef[i];
                if (d) nd++;
                if (d < 0) d = -d;
                if (d > md) md = d;
            }
            if (sh == s.shift && md < 50) {
                fprintf(stderr, "   cand %2d sh=%d ndiff=%d maxd=%d:", id, sh, nd, md);
                for (int i = 0; i < s.order; i++) fprintf(stderr, " %d", q[i]);
                fprintf(stderr, "\n");
            }
        }
    }
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_f[Min(r, 23)][cs.prank], r == found)) break;
    if (r < P.ncand) {
        int id = P.mru[r];
        if (!P.at_order(id, s.order, s.prec, q, &sh)) return false;
        s.shift = sh;
        memcpy(s.coef, q, sizeof(s32) * s.order);
        mru_hit(P, r);
        if (P.cand[id].kind == 3) st.ffh++; else st.lfh++;
        if (st.ffh > 8 && st.ffh > st.lfh) st.sim_on = 0;
        cs.prank = r == 0 ? 0 : r < 4 ? 1 : 2;
        lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
    } else {
        cs.prank = 3;
        P.nmiss++;
        // no exact candidate: code shift and deltas against the best guess
        int have = -1;
        for (int k = 0; k < Min(P.ncand, 4) && have < 0; k++)
            if (P.at_order(P.mru[k], s.order, s.prec, q, &sh)) have = P.mru[k];
        int gs = have >= 0 ? sh : 32;
        ac.cat = C_PREC;
        s.shift = ac.tree(M.shift_t[gs], 5, (u32)s.shift & 31);
        if (s.shift > 15) return false;
        ac.cat = C_COEF;
        if (have >= 0)
            P.quant(have, s.order, s.prec, q, &sh, s.shift);
        else
            memset(q, 0, sizeof(q));
        s32 lim = 1 << (s.prec - 1);
        for (int i = 0; i < s.order; i++) {
            s64 d = code_sint(ac, M.coef_m[i], M.coef_s[i], (s64)s.coef[i] - q[i]);
            s64 v = q[i] + d;
            if (v < -lim || v >= lim) return false;
            s.coef[i] = (s32)v;
        }
    }
    cs.pshift = s.shift;
    return true;
}

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
            bool samewin = b.win == a.win || ((a.win == W_T50 || a.win == W_T50_12) && (b.win == W_T50 || b.win == W_T50_12));
            if (!samewin) continue;
            if (P.hits[j] > P.hits[i] || (P.hits[j] == P.hits[i] && j < i)) dominated = true;
        }
        if (!dominated) ids[n++] = i;
    }
    return n;
}

static SimOut sim_subframe(Ctx& cx, FrameState& st, const s64* xin, int n, int sbps, const int* wids, int nw) {
    LpcPredictor& P = *cx.lp;
    s64* xs = g_simx;
    u64 o = 0;
    for (int i = 0; i < n; i++) o |= (u64)xin[i];
    int w = o ? ctz64(o) : 0;
    if (w >= sbps) w = sbps - 1;
    for (int i = 0; i < n; i++) xs[i] = xin[i] >> w;
    int eb = sbps - w;
    SimOut best = {8 + (u64)w + (u64)n * eb, ST_VERB, 0, -1};
    if (n <= 4) return best;
    bool newv = st.rice_new;
    int lim = cx.sp.bps > 16 ? 30 : 14;
    ResStats& rs = cx.rs;
    // fixed predictor estimate
    u64 e[5] = {0, 0, 0, 0, 0};
    for (int i = 4; i < n; i++) {
        s64 a = xs[i], b = xs[i - 1], c = xs[i - 2], d = xs[i - 3], f = xs[i - 4];
        s64 r0 = a, r1 = a - b, r2 = a - 2 * b + c, r3 = a - 3 * b + 3 * c - d, r4 = a - 4 * b + 6 * c - 4 * d + f;
        e[0] += (u64)(r0 < 0 ? -r0 : r0);
        e[1] += (u64)(r1 < 0 ? -r1 : r1);
        e[2] += (u64)(r2 < 0 ? -r2 : r2);
        e[3] += (u64)(r3 < 0 ? -r3 : r3);
        e[4] += (u64)(r4 < 0 ? -r4 : r4);
    }
    float rbps[5];
    for (int k = 0; k < 5; k++)
        rbps[k] = e[k] ? (float)det_log2(0.69314718055994530942 * (double)e[k] / (double)(n - 4)) : 0.0f;
    if (rbps[1] == 0.0f) {
        bool cst = true;
        for (int i = 1; i < n; i++)
            if (xs[i] != xs[0]) {
                cst = false;
                break;
            }
        if (cst) {
            u64 b = 8 + (u64)w + eb;
            if (b < best.bits) best = {b, ST_CONST, 0, -1};
            return best;
        }
    }
    int g;
    if (st.fix_le) {
        if (e[0] <= Min(Min(Min(e[1], e[2]), e[3]), e[4])) g = 0;
        else if (e[1] <= Min(Min(e[2], e[3]), e[4])) g = 1;
        else if (e[2] <= Min(e[3], e[4])) g = 2;
        else if (e[3] <= e[4]) g = 3;
        else g = 4;
    } else {
        if (e[0] < Min(Min(Min(e[1], e[2]), e[3]), e[4])) g = 0;
        else if (e[1] < Min(Min(e[2], e[3]), e[4])) g = 1;
        else if (e[2] < Min(e[3], e[4])) g = 2;
        else if (e[3] < e[4]) g = 3;
        else g = 4;
    }
    int f0 = st.lpc_exh ? 0 : g, f1 = st.lpc_exh ? 4 : g;
    if (f1 >= n) f1 = n - 1;
    for (int ord = f0; ord <= f1; ord++) {
        if (rbps[ord] >= (float)eb) continue;
        fixed_residual(xs, n, ord, g_res2);
        rs.compute(g_res2, n, ord, false);
        int hi = Min(st.maxpo, rs.maxpo);
        while (hi > 0 && ((u32)n >> hi) <= (u32)ord) hi--;
        int lo = Min(st.minpo, hi);
        u64 b = 8 + (u64)w + (u64)ord * eb + rs.bits_lf(lo, hi, lim, newv);
        if (b < best.bits) best = {b, ST_FIXED, ord, -1};
    }
    // LPC
    if (!nw) return best;
    P.begin(xs, n);
    const s32* x32 = to32(xs, n);
    int maxo = Min(st.lpc_exh ? st.lpc_maxseen : st.lpc_maxo, n - 1);
    if (maxo < 1) return best;
    for (int wi = 0; wi < nw; wi++) {
        int id = wids[wi];
        int mo = P.eval(id, maxo);
        if (!mo) continue;
        int o0, o1;
        if (st.lpc_exh) {
            o0 = 1;
            o1 = mo;
        } else {
            o0 = o1 = lpc_best_order(P.lerr, mo, n, eb + (st.lpc_ov ? 5 : st.maxprec));
        }
        for (int ord = o0; ord <= o1; ord++) {
            double er = P.lerr[ord - 1], ebs;
            if (er > 0.0) {
                ebs = 0.5 * det_log2(0.5 / (double)(n - ord) * er);
                if (ebs < 0.0) ebs = 0.0;
            } else
                ebs = er < 0.0 ? 1e32 : 0.0;
            if (ebs >= (double)eb) continue;
            int prec = st.maxprec;
            if (eb <= (st.fix_le ? 17 : 16)) prec = Min(prec, 32 - eb - ilog2_64(ord));
            s32 q[32];
            int sh;
            if (!lpc_quantize(P.lpall + (ord - 1) * 32, ord, prec, q, &sh)) continue;
            lpc_residual_fast(xs, x32, n, q, ord, sh, g_res2);
            rs.compute(g_res2, n, ord, false);
            int hi = Min(st.maxpo, rs.maxpo);
            while (hi > 0 && ((u32)n >> hi) <= (u32)ord) hi--;
            int lo = Min(st.minpo, hi);
            u64 b = 8 + (u64)w + 9 + (u64)ord * (prec + eb) + rs.bits_lf(lo, hi, lim, newv);
            if (b < best.bits) best = {b, ST_LPC, ord, id};
        }
    }
    return best;
}

// libFLAC's channel assignment decision (-m); returns chcode or -1
static SimOut g_sim4[4];
static int sim_stereo(Ctx& cx, FrameState& st, s32** pcm, int n) {
    LpcPredictor& P = *cx.lp;
    int wids[16];
    int nw = sim_windows(P, wids);
    static s64* sig[4];
    if (!sig[0])
        for (int k = 0; k < 4; k++) sig[k] = (s64*)xmalloc(sizeof(s64) * MAXBS);
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

// cheap estimate of the best stereo decorrelation (libFLAC-like fixed predictor cost)
static double fixed_cost(const s32* a, const s32* b, int mode, int n) {
    // mode: 0 = a, 1 = b, 2 = mid, 3 = side
    if (n <= 4) return 0;
    u64 e[5] = {0, 0, 0, 0, 0};
    s64 h1 = 0, h2 = 0, h3 = 0, h4 = 0;
    for (int i = 0; i < n; i++) {
        s64 v = mode == 0 ? a[i] : mode == 1 ? b[i] : mode == 2 ? (((s64)a[i] + b[i]) >> 1) : ((s64)a[i] - b[i]);
        if (i >= 4) {
            s64 r0 = v, r1 = v - h1, r2 = v - 2 * h1 + h2, r3 = v - 3 * h1 + 3 * h2 - h3,
                r4 = v - 4 * h1 + 6 * h2 - 4 * h3 + h4;
            e[0] += (u64)(r0 < 0 ? -r0 : r0);
            e[1] += (u64)(r1 < 0 ? -r1 : r1);
            e[2] += (u64)(r2 < 0 ? -r2 : r2);
            e[3] += (u64)(r3 < 0 ? -r3 : r3);
            e[4] += (u64)(r4 < 0 ? -r4 : r4);
        }
        h4 = h3;
        h3 = h2;
        h2 = h1;
        h1 = v;
    }
    u64 m = e[0];
    for (int k = 1; k < 5; k++) m = Min(m, e[k]);
    return (double)(n - 4) * det_log2((double)m / (n - 4) + 1.0);
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

static int stereo_guess(s32** pcm, int n) {
    double c[4];
    for (int k = 0; k < 4; k++) c[k] = fixed_cost(pcm[0], pcm[1], k, n);
    double t[4] = {c[0] + c[1], c[0] + c[3], c[1] + c[3], c[2] + c[3]};
    int b = 0;
    for (int k = 1; k < 4; k++)
        if (t[k] < t[b]) b = k;
    return b;
}

// Model one FRAME segment (enc: f/pcm filled; dec: fills f, reads pcm via src)
static bool model_frame(Ctx& cx, FrameInfo& f, s32** pcm, FrameState& st, WavReader* src, u64 samples_done) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ac.cat = C_HDR;
    f.blocking = ac.bit(M.blocking[st.pblocking], f.blocking);
    // block size
    ac.cat = C_BS;
    u64 remaining = cx.total_samples > samples_done ? cx.total_samples - samples_done : 0;
    u32 pbs = st.pbs;
    if (remaining && remaining < pbs) pbs = (u32)remaining;
    int ctx = st.first ? 0 : (pbs != st.pbs ? 2 : 1);
    if (ac.bit(M.bs_eq[ctx], f.bs == pbs))
        f.bs = pbs;
    else {
        u32 v = ac.tree(M.bsval, 16, f.bs - 1);
        f.bs = v + 1;
    }
    if (f.bs == 0 || f.bs > MAXBS) return false;
    if (ac.dec) {
        f.nch = cx.sp.nch;
        f.bps = cx.sp.bps;
        if (!wav_read(src, pcm, f.nch, f.bs)) return false;
    }
    int bs = (int)f.bs;
    ac.cat = C_HDR;
    int cb = canon_bscode(f.bs);
    if (ac.bit(M.bscode_eq[cb], f.bscode == cb))
        f.bscode = cb;
    else
        f.bscode = ac.tree(M.bscode_t[cb], 4, f.bscode);
    if (!bscode_ok(f.bscode, f.bs)) return false;
    if (ac.bit(M.sr_eq[0], f.srcode == st.psrcode))
        f.srcode = st.psrcode;
    else
        f.srcode = ac.tree(M.sr_t, 4, f.srcode);
    if (f.srcode >= 12 && f.srcode <= 14) {
        int nb = f.srcode == 12 ? 8 : 16;
        if (ac.bit(M.srv_eq[0], f.srval == st.psrval))
            f.srval = st.psrval;
        else
            f.srval = (u32)ac.raw(f.srval, nb);
    } else
        f.srval = 0;
    if (ac.bit(M.bpsc_eq[0], f.bpscode == st.pbpscode))
        f.bpscode = st.pbpscode;
    else
        f.bpscode = ac.tree(M.bpsc_t, 3, f.bpscode);
    u64 pnum = f.blocking ? st.sample_counter : st.frame_counter;
    if (ac.bit(M.num_eq[f.blocking], f.number == pnum))
        f.number = pnum;
    else
        f.number = ac.raw(f.number, 36);
    ac.cat = C_CH;
    if (cx.sp.nch == 2) {
        int pf = ff_stereo_guess(pcm, bs, cx.sp.bps > 16 ? 30 : 14);
        static const int map[4] = {1, 8, 9, 10};
        int cf = map[pf];
        int cl = st.sim_on ? sim_stereo(cx, st, pcm, bs) : -1;
        int done = 0;
        if (cl >= 0) {
            if (ac.bit(M.ch_sim[cl == cf][cl == st.pchcode], f.chcode == cl)) {
                f.chcode = cl;
                done = 1;
            }
        }
        if (!done && cf != cl) {
            if (ac.bit(M.ch_ff[cf == st.pchcode][cl >= 0], f.chcode == cf)) {
                f.chcode = cf;
                done = 1;
            }
        }
        if (!done) f.chcode = ac.tree(M.ch_t[st.pchcode][pf][0], 4, f.chcode);
        if (!ac.dec && g_verbose >= 2 && cl >= 0)
            st.simstat[f.chcode == cl]++;
    }
    else
        f.chcode = cx.sp.nch - 1;
    f.nch = f.chcode < 8 ? f.chcode + 1 : 2;
    if (f.nch != cx.sp.nch) return false;
    f.bps = cx.sp.bps;
    for (int ch = 0; ch < f.nch; ch++) {
        SubInfo& s = f.sub[ch];
        ChState& cs = st.ch[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        s64* x = g_x;
        subframe_signal(f.chcode, ch, pcm, bs, x);
        SimOut sim = {0, 4, 0, -1};
        if (st.sim_on) {
            if (cx.sp.nch == 2) {
                static const int sig_of[11][2] = {{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
                                                  {0, 1}, {0, 1}, {0, 3}, {3, 1}, {2, 3}};
                sim = g_sim4[sig_of[f.chcode][ch]];
                if (f.chcode < 8 && f.chcode != 1) sim.type = 4;
            } else {
                int wids[16];
                int nw = sim_windows(*cx.lp, wids);
                sim = sim_subframe(cx, st, x, bs, sbps, wids, nw);
            }
        }
        // wasted bits
        ac.cat = C_WASTED;
        u64 o = 0;
        for (int i = 0; i < bs; i++) o |= (u64)x[i];
        int pw = o ? ctz64(o) : 0;
        if (pw >= sbps) pw = sbps - 1;
        if (ac.bit(M.w_eq[pw ? 1 : 0][Min(ch, 3)], s.wasted == pw))
            s.wasted = pw;
        else
            s.wasted = (int)code_uint(ac, M.w_val, s.wasted);
        if (s.wasted >= sbps) return false;
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] >>= s.wasted;
        // type
        ac.cat = C_TYPE;
        int isconst = 1;
        for (int i = 1; i < bs; i++)
            if (x[i] != x[0]) {
                isconst = 0;
                break;
            }
        s.type = ac.tree(M.type_t[isconst][sim.type][cs.ptype], 2, s.type);
        cs.ptype = s.type;
        if (s.type == ST_CONST) {
            if (!isconst) return false;
            s.order = 0;
            continue;
        }
        if (s.type == ST_VERB) {
            s.order = 0;
            continue;
        }
        ac.cat = C_ORDER;
        if (s.type == ST_FIXED) {
            int gn, go;
            fixed_guess(x, bs, gn, go);
            int gs = sim.type == ST_FIXED ? sim.order : 5;
            s.order = ac.tree(M.forder_t[gn][gn == go][gs], 3, s.order);
            if (s.order > 4 || s.order > bs) return false;
            if (gn != go) {
                if (s.order == gn) st.fvote[1]++;
                if (s.order == go) st.fvote[0]++;
                if (st.fvote[0] + st.fvote[1] >= 4) st.fix_le = st.fvote[1] > st.fvote[0];
            }
            cs.pfixed = s.order;
            fixed_residual(x, bs, s.order, g_res);
        } else {
            ac.cat = C_PREC;
            s.prec = ac.tree(M.prec_t[cs.pprec], 4, s.prec - 1) + 1;
            if (s.prec > 15) return false;
            cs.pprec = s.prec - 1;
            if (!model_lpc(cx, s, cs, st, x, bs, sbps - s.wasted, sim)) return false;
            lpc_residual(x, bs, s.coef, s.order, s.shift, g_res);
        }
        if (!model_residual(cx, s, cs, st, g_res, bs)) return false;
    }
    // update state
    st.first = 0;
    st.pblocking = f.blocking;
    st.pbs = f.bs;
    st.psrcode = f.srcode;
    st.psrval = f.srval;
    st.pbpscode = f.bpscode;
    st.pchcode = f.chcode;
    if (f.blocking)
        st.sample_counter = f.number + f.bs;
    else
        st.frame_counter = f.number + 1;
    return true;
}

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
    u8* data;  // owned copy
    int structured;
};

struct FlacLayout {
    Buf prefix;  // bytes before "fLaC"
    Vec<MBlock> blocks;
    StreamParams sp;
};

// Stats gathered while streaming frames (both directions)
struct FrameStats {
    u64 nframes;
    u32 first_bs, min_bs, max_bs, min_bs_nl, max_bs_nl, last_bs;
    u32 min_fs, max_fs;
    u64 total;
    MD5 md5;
    u8 md5sum[16];
    Vec<u64> foff;   // frame offsets relative to audio start
    Vec<u64> fsamp;  // first sample of frame
    Vec<u32> fbs;    // frame block size
    u64 audio_start;
    void init() {
        nframes = 0;
        first_bs = 0;
        min_bs = min_bs_nl = 0xFFFFFFFF;
        max_bs = max_bs_nl = 0;
        last_bs = 0;
        min_fs = 0xFFFFFFFF;
        max_fs = 0;
        total = 0;
        md5.init();
        foff.n = fsamp.n = fbs.n = 0;
    }
    void add_frame(u64 off, u32 len, u32 bs) {
        if (nframes == 0) first_bs = bs;
        if (nframes > 0) {
            min_bs_nl = Min(min_bs_nl, last_bs);
            max_bs_nl = Max(max_bs_nl, last_bs);
        }
        last_bs = bs;
        min_bs = Min(min_bs, bs);
        max_bs = Max(max_bs, bs);
        min_fs = Min(min_fs, len);
        max_fs = Max(max_fs, len);
        foff.push(off - audio_start);
        fsamp.push(total);
        fbs.push(bs);
        total += bs;
        nframes++;
    }
    void add_pcm(s32** pcm, int nch, int bps, u32 n) {
        int bytes = (bps + 7) / 8;
        u8 tmp[4096];
        size_t k = 0;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                u32 v = (u32)pcm[c][i];
                for (int b = 0; b < bytes; b++) tmp[k++] = (u8)(v >> (8 * b));
                if (k > sizeof(tmp) - 32) {
                    md5.update(tmp, k);
                    k = 0;
                }
            }
        if (k) md5.update(tmp, k);
    }
};

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

struct WavWriter {
    FILE* f;
    int nch, bps, cb;
    u32 rate;
    u64 nsamples;
    Buf buf;
    bool open(const char* name, int nch_, u32 rate_, int bps_) {
        f = fopen(name, "wb");
        if (!f) return false;
        nch = nch_;
        bps = bps_;
        rate = rate_;
        cb = (bps + 7) / 8;
        nsamples = 0;
        u8 h[80];
        int hl;
        wav_header(h, hl, nch, rate, bps, 0);
        fwrite(h, 1, hl, f);
        return true;
    }
    void write(s32** pcm, u32 n) {
        int sh = cb * 8 - bps;
        buf.n = 0;
        buf.reserve((size_t)n * nch * cb);
        u8* o = buf.p;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                u32 v = (u32)pcm[c][i] << sh;
                if (cb == 1)
                    *o++ = (u8)(v + 128);
                else
                    for (int b = 0; b < cb; b++) *o++ = (u8)(v >> (8 * b));
            }
        fwrite(buf.p, 1, o - buf.p, f);
        nsamples += n;
    }
    bool close() {
        u8 h[80];
        int hl;
        wav_header(h, hl, nch, rate, bps, nsamples);
        fseek64(f, 0, SEEK_SET);
        fwrite(h, 1, hl, f);
        bool ok = !ferror(f);
        fclose(f);
        buf.release();
        return ok;
    }
};

struct WavReader {
    FILE* f;
    int nch, bps, cb;
    u64 nsamples, pos;
    Buf buf;
    bool open(const char* name) {
        f = fopen(name, "rb");
        if (!f) return false;
        u8 h[12];
        if (fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
        int got_fmt = 0;
        int fch = 0, fbits = 0;
        for (;;) {
            u8 ch[8];
            if (fread(ch, 1, 8, f) != 8) return false;
            u32 sz = rd_le32(ch + 4);
            if (!memcmp(ch, "fmt ", 4)) {
                u8 fm[64];
                u32 k = Min(sz, (u32)64);
                if (fread(fm, 1, k, f) != k) return false;
                fch = rd_le16(fm + 2);
                fbits = rd_le16(fm + 14);
                got_fmt = 1;
                if (sz > k) fseek64(f, sz - k, SEEK_CUR);
                if (sz & 1) fseek64(f, 1, SEEK_CUR);
            } else if (!memcmp(ch, "data", 4)) {
                if (!got_fmt) return false;
                nch = fch;
                cb = (fbits + 7) / 8;
                s64 here = ftell64(f);
                fseek64(f, 0, SEEK_END);
                s64 end = ftell64(f);
                fseek64(f, here, SEEK_SET);
                u64 avail = (u64)(end - here);
                u64 dsz = sz;
                if (dsz == 0xFFFFFFFF || dsz == 0 || dsz > avail) dsz = avail;
                if (!nch || !cb) return false;
                nsamples = dsz / ((u64)nch * cb);
                pos = 0;
                return true;
            } else {
                fseek64(f, (s64)sz + (sz & 1), SEEK_CUR);
            }
        }
    }
    bool read(s32** pcm, int n_ch, u32 n) {
        if (n_ch != nch || pos + n > nsamples) return false;
        size_t bytes = (size_t)n * nch * cb;
        buf.reserve(bytes);
        if (fread(buf.p, 1, bytes, f) != bytes) return false;
        int sh = cb * 8 - bps;
        const u8* p = buf.p;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                s32 v;
                if (cb == 1)
                    v = (s32)(s8)(p[0] - 128);
                else if (cb == 2)
                    v = (s16)(p[0] | (p[1] << 8));
                else if (cb == 3)
                    v = (s32)((u32)(p[0] | (p[1] << 8) | (p[2] << 16)) << 8) >> 8;
                else
                    v = (s32)rd_le32(p);
                p += cb;
                pcm[c][i] = v >> sh;
            }
        pos += n;
        return true;
    }
    void close() {
        if (f) fclose(f);
        f = nullptr;
        buf.release();
    }
};

static bool wav_read(WavReader* w, s32** pcm, int nch, u32 n) { return w->read(pcm, nch, n); }

// ---------------------------------------------------------------------------
// Output sink for restoration (file, or compare against original)

struct Sink {
    FILE* f;
    const u8* ref;
    u64 ref_n;
    bool mismatch;
    u64 pos;
    void write_at(u64 off, const u8* p, size_t n) {
        if (f) {
            if (off != pos) fseek64(f, (s64)off, SEEK_SET);
            fwrite(p, 1, n, f);
            pos = off + n;
        } else {
            if (off + n > ref_n || memcmp(ref + off, p, n)) mismatch = true;
            pos = off + n;
        }
    }
};

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

// Part A: prefix, block list, contents not depending on frames
static bool code_layout(AC& ac, Models& M, FlacLayout& L) {
    ac.cat = C_META;
    u64 plen = code_uint(ac, M.len_m, L.prefix.n);
    if (ac.dec) {
        L.prefix.n = 0;
        L.prefix.reserve(plen + 1);
        L.prefix.n = (size_t)plen;
    }
    code_bytes(ac, L.prefix.p, (size_t)plen, false);
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
        L.blocks[0].data = (u8*)xcalloc(34);
    }
    int pt = 0;
    for (size_t i = 1; i < L.blocks.n; i++) {
        MBlock& b = L.blocks[i];
        b.type = (int)ac.tree(M.mtype[Min(pt, 7)], 7, b.type);
        pt = b.type;
        b.len = (u32)code_uint(ac, M.len_m, b.len);
        if (b.len >= (1u << 24)) return false;
        if (ac.dec) b.data = (u8*)xcalloc(b.len + 1);
        if (b.type == 1) {  // PADDING
            int z = 1;
            if (!ac.dec)
                for (u32 j = 0; j < b.len; j++)
                    if (b.data[j]) z = 0;
            z = ac.bit(M.mflag[0], z);
            b.structured = z;
            if (!z) code_bytes(ac, b.data, b.len, false);
        } else if (b.type == 3) {  // SEEKTABLE
            int s = ac.bit(M.mflag[1], b.len % 18 == 0);
            b.structured = s;
            if (!s) code_bytes(ac, b.data, b.len, false);
        } else if (b.type == 4) {
            code_bytes(ac, b.data, b.len, true);
        } else {
            code_bytes(ac, b.data, b.len, false);
        }
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

static void code_seektable(AC& ac, MBlock& b, const FrameStats& fs, const StreamParams& sp) {
    ac.cat = C_SEEK;
    SeekModels S;
    model_init(S);
    u32 np = b.len / 18;
    u8* d = b.data;
    // spacing mode: 0 = fixed spacing in samples, 1 = total*j/np, 2 = none
    u64 spacing = 0;
    int mode = 2;
    u64 T = fs.total;
    if (!ac.dec) {
        // try candidates; count matching points
        u64 cs[3] = {10ull * sp.rate, 0, 0};
        if (np >= 2) {
            u64 s1 = rd_be64(d + 18);
            if (s1 != ~0ull) {
                u64 k = frame_containing(fs, s1);
                cs[1] = s1;
                (void)k;
            }
        }
        int best = -1;
        u32 bestc = 0;
        for (int c = 0; c < 3; c++) {
            u32 cnt = 0;
            for (u32 j = 0; j < np; j++) {
                u64 target = c < 2 ? cs[c] * j : (np ? T * j / np : 0);
                if (c < 2 && cs[c] == 0) break;
                u64 k = frame_containing(fs, target);
                if (k < fs.fsamp.n && rd_be64(d + 18 * j) == fs.fsamp[k]) cnt++;
            }
            if (cnt > bestc) {
                bestc = cnt;
                best = c;
            }
        }
        if (best >= 0 && bestc > np / 4) {
            mode = best < 2 ? 0 : 1;
            spacing = best < 2 ? cs[best] : 0;
        }
    }
    mode = ac.tree(S.mode, 2, mode);
    if (mode == 0) {
        if (ac.code(spacing == 10ull * sp.rate, 32768))
            spacing = 10ull * sp.rate;
        else
            spacing = code_uint(ac, S.dk, spacing);
    }
    u64 prevk = 0;
    int pkind = 0;
    for (u32 j = 0; j < np; j++) {
        u8* p = d + 18 * j;
        u64 sn = 0, off = 0;
        u32 fsm = 0;
        int kind = 2;
        u64 k = 0;
        if (!ac.dec) {
            sn = rd_be64(p);
            off = rd_be64(p + 8);
            fsm = (p[16] << 8) | p[17];
            if (sn == ~0ull && off == 0 && fsm == 0)
                kind = 0;
            else {
                u64 kk = frame_containing(fs, sn);
                if (kk < fs.fsamp.n && fs.fsamp[kk] == sn && fs.foff[kk] == off && fs.fbs[kk] == fsm) {
                    kind = 1;
                    k = kk;
                }
            }
        }
        kind = ac.tree(S.kind[Min(pkind, 2)], 2, kind);
        pkind = kind;
        if (kind == 0) {
            sn = ~0ull;
            off = 0;
            fsm = 0;
        } else if (kind == 1) {
            u64 pk = prevk;
            int have = 0;
            if (mode == 0) {
                pk = frame_containing(fs, spacing * j);
                have = 1;
            } else if (mode == 1) {
                pk = frame_containing(fs, np ? T * j / np : 0);
                have = 1;
            }
            int mt = have ? ac.bit(S.match[mode], k == pk) : 0;
            if (mt)
                k = pk;
            else {
                // code k - prevk (>=0 normally) with sign
                int neg = ac.code(k < prevk, 2000);
                u64 dlt = neg ? prevk - k : k - prevk;
                dlt = code_uint(ac, S.dk, dlt);
                k = neg ? prevk - dlt : prevk + dlt;
            }
            if (k >= fs.fsamp.n) k = fs.fsamp.n ? fs.fsamp.n - 1 : 0;
            if (fs.fsamp.n) {
                sn = fs.fsamp[k];
                off = fs.foff[k];
                fsm = fs.fbs[k];
            }
            prevk = k;
        } else {
            sn = ac.raw(sn, 64);
            off = ac.raw(off, 64);
            fsm = (u32)ac.raw(fsm, 16);
        }
        if (ac.dec) {
            wr_be64(p, sn);
            wr_be64(p + 8, off);
            p[16] = (u8)(fsm >> 8);
            p[17] = (u8)fsm;
        }
    }
}

static void code_derived(AC& ac, FlacLayout& L, FrameStats& fs) {
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
    }
    for (size_t i = 1; i < L.blocks.n; i++)
        if (L.blocks[i].type == 3 && L.blocks[i].structured) code_seektable(ac, L.blocks[i], fs, L.sp);
}

// encoder hint from the VORBIS_COMMENT vendor string: 0 unknown, -1 ffmpeg, libFLAC version*100
static int vendor_hint(const FlacLayout& L) {
    for (size_t i = 0; i < L.blocks.n; i++) {
        const MBlock& b = L.blocks[i];
        if (b.type != 4 || b.len < 4) continue;
        u32 vl = rd_le32(b.data);
        if ((u64)vl + 4 > b.len) return 0;
        const char* v = (const char*)b.data + 4;
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
    return 0;
}

static u64 header_size(const FlacLayout& L) {
    u64 s = L.prefix.n + 4;
    for (size_t i = 0; i < L.blocks.n; i++) s += 4 + L.blocks[i].len;
    return s;
}

static void write_header(Sink& out, const FlacLayout& L) {
    out.write_at(0, L.prefix.p, L.prefix.n);
    u64 pos = L.prefix.n;
    out.write_at(pos, (const u8*)"fLaC", 4);
    pos += 4;
    for (size_t i = 0; i < L.blocks.n; i++) {
        const MBlock& b = L.blocks[i];
        u8 h[4];
        h[0] = (u8)((i + 1 == L.blocks.n ? 0x80 : 0) | b.type);
        wr_be24(h + 1, b.len);
        out.write_at(pos, h, 4);
        pos += 4;
        out.write_at(pos, b.data, b.len);
        pos += b.len;
    }
}

// ---------------------------------------------------------------------------
// Compression (flac -> wav + meta)

static const u8 k_magic[2] = {'F', 0xD1};

struct Options {
    int verify;
};

static bool parse_layout(const Buf& in, FlacLayout& L, u64& audio_start) {
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
    L.prefix.n = 0;
    L.prefix.append(in.p, pos);
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
        b.data = (u8*)xmalloc(b.len + 1);
        memcpy(b.data, in.p + pos, b.len);
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

static int do_compress(const char* inname, const char* prefix, const Options& opt) {
    Buf in;
    if (!read_file(inname, in, 64)) die("cannot read input");
    FlacLayout L;
    u64 audio_start = 0;
    if (!parse_layout(in, L, audio_start)) die("not a FLAC file (or broken metadata)");
    const StreamParams& sp = L.sp;
    char wname[4096], mname[4096];
    snprintf(wname, sizeof(wname), "%s.wav", prefix);
    snprintf(mname, sizeof(mname), "%s.meta", prefix);

    // pass 1: segment the audio region, decode, verify re-encoding, write wav
    WavWriter ww;
    if (!ww.open(wname, sp.nch, sp.rate, sp.bps)) die("cannot create wav");
    Vec<Segment> segs;
    FrameInfo fi;
    frameinfo_init(fi);
    Buf tmp;
    u64 pos = audio_start;
    u64 nframes = 0, nraw = 0, njunk = 0;
    while (pos < in.n) {
        size_t len = parse_frame(in.p + pos, (size_t)(in.n - pos), sp, fi, g_pcm);
        if (len) {
            tmp.n = 0;
            encode_frame(fi, g_pcm, tmp);
            int same = tmp.n == len && !memcmp(tmp.p, in.p + pos, len);
            Segment s;
            s.kind = same ? SEG_FRAME : SEG_RAWFRAME;
            s.off = pos;
            s.len = len;
            s.nsamp = fi.bs;
            segs.push(s);
            ww.write(g_pcm, fi.bs);
            if (same)
                nframes++;
            else
                nraw++;
            pos += len;
        } else {
            u64 q = pos + 1;
            while (q < in.n) {
                if (in.p[q] == 0xFF && (in.p[q + 1] & 0xFE) == 0xF8 &&
                    parse_frame(in.p + q, (size_t)(in.n - q), sp, fi, g_pcm))
                    break;
                q++;
            }
            if (segs.n && segs.back().kind == SEG_RAWBYTES)
                segs.back().len += q - pos;
            else {
                Segment s;
                s.kind = SEG_RAWBYTES;
                s.off = pos;
                s.len = q - pos;
                s.nsamp = 0;
                segs.push(s);
            }
            njunk += q - pos;
            pos = q;
        }
    }
    u64 total_samples = ww.nsamples;
    if (!ww.close()) die("error writing wav");

    // pass 2: model
    Buf meta;
    meta.append(k_magic, 2);
    AC ac;
    ac.enc_init(&meta);
    Models* M = (Models*)xmalloc(sizeof(Models));
    model_init(*M);
    if (!code_layout(ac, *M, L)) die("internal: layout");
    Ctx cx;
    cx.ac = &ac;
    cx.m = M;
    cx.sp = sp;
    cx.rs.alloc();
    cx.lp = (LpcPredictor*)xmalloc(sizeof(LpcPredictor));
    cx.lp->init();
    cx.total_samples = total_samples;
    FrameState st;
    fstate_init(st, sp, vendor_hint(L));
    FrameStats fs;
    fs.init();
    fs.audio_start = audio_start;
    u64 done = 0;
    int pk = 4;
    for (size_t i = 0; i <= segs.n; i++) {
        int kind = i < segs.n ? segs[i].kind : SEG_END;
        ac.cat = C_HDR;
        ac.tree(M->seg[pk], 2, kind);
        pk = kind;
        if (kind == SEG_END) break;
        Segment& s = segs[i];
        if (kind == SEG_FRAME) {
            size_t len = parse_frame(in.p + s.off, (size_t)(in.n - s.off), sp, fi, g_pcm);
            if (len != s.len) die("internal: reparse");
            if (!model_frame(cx, fi, g_pcm, st, nullptr, done)) die("internal: model_frame");
            fs.add_frame(s.off, (u32)s.len, fi.bs);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, fi.bs);
            done += fi.bs;
        } else if (kind == SEG_RAWFRAME) {
            size_t len = parse_frame(in.p + s.off, (size_t)(in.n - s.off), sp, fi, g_pcm);
            if (len != s.len) die("internal: reparse");
            ac.cat = C_RAW;
            code_uint(ac, M->len_m, s.nsamp - 1);
            code_uint(ac, M->len_m, s.len);
            for (u64 j = 0; j < s.len; j++) ac.raw(in.p[s.off + j], 8);
            fs.add_frame(s.off, (u32)s.len, s.nsamp);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, s.nsamp);
            done += s.nsamp;
            st.frame_counter++;
            st.sample_counter += s.nsamp;
        } else {
            ac.cat = C_RAW;
            code_uint(ac, M->len_m, s.len - 1);
            code_bytes(ac, in.p + s.off, (size_t)s.len, false);
        }
    }
    fs.md5.final(fs.md5sum);
    code_derived(ac, L, fs);
    ac.flush();
    FILE* mf = fopen(mname, "wb");
    if (!mf) die("cannot create meta");
    fwrite(meta.p, 1, meta.n, mf);
    if (fclose(mf)) die("error writing meta");
    if (g_verbose) {
        fprintf(stderr, "frames: %llu modelled, %llu raw, %llu junk bytes; %llu samples\n", (unsigned long long)nframes,
                (unsigned long long)nraw, (unsigned long long)njunk, (unsigned long long)total_samples);
        print_stats(ac, meta.n);
        fprintf(stderr, "stage0 %u/%u stage1 %u/%u exscore %u/%u\n", st.dbg[1], st.dbg[0], st.dbg[3], st.dbg[2],
                st.exscore[0], st.exscore[1]);
        fprintf(stderr, "stereo sim: %u hit, %u miss; rice_new=%d fix_le=%d exh=%d maxo=%d\n", st.simstat[1], st.simstat[0],
                st.rice_new, st.fix_le, st.lpc_exh, st.lpc_maxo);
        LpcPredictor& P = *cx.lp;
        fprintf(stderr, "lpc subframes %u, candidate hits:", P.nsub);
        for (int i = 0; i < P.ncand; i++)
            if (P.hits[i]) fprintf(stderr, " %d:%u", i, P.hits[i]);
        fprintf(stderr, "\n");
    }
    printf("%s: %zu bytes -> wav %llu samples, meta %zu bytes\n", inname, in.n, (unsigned long long)total_samples,
           meta.n);
    free(M);
    if (opt.verify) {
        WavReader wr;
        wr.f = nullptr;
        if (!wr.open(wname)) die("verify: cannot reopen wav");
        wr.bps = sp.bps;
        Sink sk;
        memset(&sk, 0, sizeof(sk));
        sk.ref = in.p;
        sk.ref_n = in.n;
        int g = g_verbose;
        g_verbose = 0;
        bool ok = decompress_core(meta.p, meta.n, wr, sk);
        g_verbose = g;
        wr.close();
        if (!ok || sk.mismatch || sk.pos != in.n) {
            fprintf(stderr, "flacdump: VERIFY FAILED\n");
            return 3;
        }
        if (g_verbose) fprintf(stderr, "verify ok\n");
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Decompression (wav + meta -> flac)

static bool decompress_core(const u8* meta, size_t meta_n, WavReader& wav, Sink& out) {
    if (meta_n < 2 || memcmp(meta, k_magic, 2)) return false;
    AC ac;
    ac.dec_init(meta + 2, meta_n - 2);
    Models* M = (Models*)xmalloc(sizeof(Models));
    model_init(*M);
    FlacLayout L;
    if (!code_layout(ac, *M, L)) return false;
    const StreamParams& sp = L.sp;
    wav.bps = sp.bps;
    if (wav.nch != sp.nch || wav.cb != (sp.bps + 7) / 8) return false;
    u64 pos = header_size(L);
    Ctx cx;
    cx.ac = &ac;
    cx.m = M;
    cx.sp = sp;
    cx.rs.alloc();
    cx.lp = (LpcPredictor*)xmalloc(sizeof(LpcPredictor));
    cx.lp->init();
    cx.total_samples = wav.nsamples;
    FrameState st;
    fstate_init(st, sp, vendor_hint(L));
    FrameStats fs;
    fs.init();
    fs.audio_start = pos;
    FrameInfo fi;
    frameinfo_init(fi);
    Buf tmp;
    u64 done = 0;
    int pk = 4;
    for (;;) {
        int kind = ac.tree(M->seg[pk], 2, 0);
        pk = kind;
        if (kind == SEG_END) break;
        if (kind == SEG_FRAME) {
            if (!model_frame(cx, fi, g_pcm, st, &wav, done)) return false;
            tmp.n = 0;
            encode_frame(fi, g_pcm, tmp);
            out.write_at(pos, tmp.p, tmp.n);
            fs.add_frame(pos, (u32)tmp.n, fi.bs);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, fi.bs);
            pos += tmp.n;
            done += fi.bs;
        } else if (kind == SEG_RAWFRAME) {
            u32 ns = (u32)code_uint(ac, M->len_m, 0) + 1;
            u64 len = code_uint(ac, M->len_m, 0);
            if (ns > MAXBS || !wav.read(g_pcm, sp.nch, ns)) return false;
            tmp.n = 0;
            tmp.reserve(len);
            for (u64 j = 0; j < len; j++) tmp.p[j] = (u8)ac.raw(0, 8);
            out.write_at(pos, tmp.p, (size_t)len);
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
            code_bytes(ac, tmp.p, (size_t)len, false);
            out.write_at(pos, tmp.p, (size_t)len);
            pos += len;
        }
    }
    fs.md5.final(fs.md5sum);
    code_derived(ac, L, fs);
    u64 endpos = pos;
    write_header(out, L);
    out.pos = endpos;
    free(M);
    for (size_t i = 0; i < L.blocks.n; i++) free(L.blocks[i].data);
    L.blocks.release();
    L.prefix.release();
    tmp.release();
    return true;
}

static int do_decompress(const char* prefix, const char* outname) {
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
            "  -n   (c) skip the built-in restore verification\n");
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
