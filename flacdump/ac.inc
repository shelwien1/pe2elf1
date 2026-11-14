// flacdump: AC - binary arithmetic coder, adaptive bit models (BM), cost categories
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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
                out->push((u8)(x2 >> 24));
            else
                x = (x << 8) | byte();
            x1 <<= 8;
            x2 = (x2 << 8) | 255;
        }
        return bit;
    }
    void flush() {
        if (!dec) out->push((u8)(x1 >> 24));
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
