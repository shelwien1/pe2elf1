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
static s32* g_pcm[MAXCH];  // PCM of current frame
static u8* g_parambuf;

static void alloc_work() {
    for (int c = 0; c < MAXCH; c++) {
        g_sig[c] = (s64*)xmalloc(sizeof(s64) * MAXBS);
        g_pcm[c] = (s32*)xmalloc(sizeof(s32) * MAXBS);
    }
    g_x = (s64*)xmalloc(sizeof(s64) * MAXBS);
    g_res = (s64*)xmalloc(sizeof(s64) * MAXBS);
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
    void compute(const s64* r, int bs_, int order_) {
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
            for (u32 j = 0; j < n; j++) {
                s64 v = p[j];
                a += (u64)(v < 0 ? -v : v);
                z += ((u64)v << 1) ^ (u64)(v >> 63);
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
                z2[i] = z1[2 * i] + z1[2 * i + 1];
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
// The meta model

struct ChState {
    int ptype, pfixed, plorder, pprec, pshift, pmethod, pporder;
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
    BM ch_t[16][16];
    BM bpsc_eq[2];
    BM bpsc_t[8];
    BM num_eq[2];
    BM w_eq[2][4];
    UModel w_val;
    BM type_t[2][5][4];
    BM forder_t[5][2][8];
    BM lorder_t[33][32];
    BM prec_t[16][16];
    BM shift_t[33][32];
    UModel coef_m[32];
    BM coef_s[32];
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
    int pblocking, pbscode, psrcode, pchcode, pbpscode;
    u32 psrval, pbs;
    u64 frame_counter, sample_counter;
    int minpo, maxpo;
    ChState ch[MAXCH];
};

static void fstate_init(FrameState& s, const StreamParams& sp) {
    memset(&s, 0, sizeof(s));
    s.first = 1;
    s.pbs = 4096;
    s.psrcode = canon_srcode(sp.rate);
    s.pbpscode = canon_bpscode(sp.bps);
    s.minpo = 15;
    s.maxpo = 0;
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
    return true;
}

static void model_coefs(Ctx& cx, SubInfo& s, ChState& cs) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ac.cat = C_COEF;
    for (int i = 0; i < s.order; i++) {
        s64 v = code_sint(ac, M.coef_m[i], M.coef_s[i], s.coef[i]);
        s.coef[i] = (s32)v;
    }
    cs.pcoef_n = s.order;
    memcpy(cs.pcoef, s.coef, sizeof(s32) * s.order);
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
    if (cx.sp.nch == 2)
        f.chcode = ac.tree(M.ch_t[st.pchcode], 4, f.chcode);
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
        s.type = ac.tree(M.type_t[isconst][cs.ptype], 2, s.type);
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
            s.order = ac.tree(M.forder_t[gn][gn == go], 3, s.order);
            if (s.order > 4 || s.order > bs) return false;
            cs.pfixed = s.order;
            fixed_residual(x, bs, s.order, g_res);
        } else {
            s.order = ac.tree(M.lorder_t[cs.plorder], 5, s.order - 1) + 1;
            if (s.order > bs) return false;
            cs.plorder = s.order - 1;
            ac.cat = C_PREC;
            s.prec = ac.tree(M.prec_t[cs.pprec], 4, s.prec - 1) + 1;
            if (s.prec > 15) return false;
            cs.pprec = s.prec - 1;
            int sh = ac.tree(M.shift_t[cs.pshift], 5, (u32)s.shift & 31);
            s.shift = sh;
            if (s.shift > 15 && ac.dec) {
                // negative shifts are never produced by our parser
                return false;
            }
            cs.pshift = sh;
            model_coefs(cx, s, cs);
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
    cx.total_samples = total_samples;
    FrameState st;
    fstate_init(st, sp);
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
    cx.total_samples = wav.nsamples;
    FrameState st;
    fstate_init(st, sp);
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
