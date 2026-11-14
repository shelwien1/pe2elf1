// flacdump: ByteCM - order-0..3 context mixing byte model and its table pool
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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

// tables of the byte models come from a small static pool (models nest at most a few levels:
// a VORBIS_COMMENT model, plus code_bytes' model and its trial copy, plus a cost estimate)
enum { CM_H2 = 1 << 16, CM_H3 = 1 << 18, CM_POOL = 6 };
struct ByteCMTables {
    u16 t0[256], t1[65536], t2[CM_H2], t3[CM_H3];
};
static ByteCMTables g_cmpool[CM_POOL];  // ~4.7 MB
static bool g_cmbusy[CM_POOL];

struct ByteCM {
    enum { N = 4, H2 = CM_H2, H3 = CM_H3 };
    u16* t0;
    u16* t1;
    u16* t2;
    u16* t3;
    int slot;
    int w[256][N + 1];
    u32 c1, c2, c3;
    void init() {
        slot = 0;
        while (slot < CM_POOL && g_cmbusy[slot]) slot++;
        if (slot == CM_POOL) die("internal: byte model pool exhausted");
        g_cmbusy[slot] = true;
        ByteCMTables& T = g_cmpool[slot];
        t0 = T.t0;
        t1 = T.t1;
        t2 = T.t2;
        t3 = T.t3;
        for (int i = 0; i < 256; i++) t0[i] = 2048;
        for (int i = 0; i < 65536; i++) t1[i] = 2048;
        for (int i = 0; i < H2; i++) t2[i] = 2048;
        for (int i = 0; i < H3; i++) t3[i] = 2048;
        for (int i = 0; i < 256; i++)
            for (int j = 0; j <= N; j++) w[i][j] = j < N ? 20000 : 0;
        c1 = c2 = c3 = 0;
    }
    void release() { g_cmbusy[slot] = false; }
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
