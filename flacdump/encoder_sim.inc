// flacdump: libFLAC subframe/stereo cost simulation (process_subframe_ estimates), ffmpeg stereo estimate
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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
