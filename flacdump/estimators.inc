// flacdump: libFLAC/ffmpeg estimators: fixed order guess, rice parameters, expected bits
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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

// libFLAC <= 1.3 "suggested" rice parameter for partition order 0
static int k0_from_bps(double bps, int lim) {
    int k = (bps > 0.0 ? (int)(u32)(bps + 0.5) : 0) + 1;
    return k > lim ? lim : k;
}
// libFLAC <= 1.3.1 scaled the LPC error by ln(2)^2 when estimating bits (ln2sq)
static inline double err_scale(int total, bool ln2sq) {
    return ln2sq ? 0.5 * 0.69314718055994530942 * 0.69314718055994530942 / (double)total : 0.5 / (double)total;
}
static double lpc_expected_bps(double err, int total, bool ln2sq) {
    if (err > 0.0) {
        double b = 0.5 * det_log2(err_scale(total, ln2sq) * err);
        return b >= 0.0 ? b : 0.0;
    }
    return err < 0.0 ? 1e32 : 0.0;
}
static float fixed_rbps(u64 e, int len) {
    return e ? (float)det_log2(0.69314718055994530942 * (double)e / (double)len) : 0.0f;
}
