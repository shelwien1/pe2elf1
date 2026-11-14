// flacdump: FLAC structures, work buffers, header code tables, fixed/LPC residuals
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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
