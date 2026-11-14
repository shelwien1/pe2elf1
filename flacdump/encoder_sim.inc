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

// residual bits of g_res2 (predictor order 'order', n samples) as libFLAC's rice parameter
// search over the learned partition order range estimates them; bps: expected bits per sample
// (gives the libFLAC <= 1.3 parameter hint for partition order 0)
static u64 sim_rice_bits(Ctx& cx, FrameState& st, int n, int order, double bps) {
    ResStats& rs = cx.rs;
    int lim = cx.sp.bps > 16 ? 30 : 14;
    bool newv = st.rice_new;
    rs.compute(g_res2, n, order, false);
    int hi = Min(st.maxpo, rs.maxpo);
    while (hi > 0 && ((u32)n >> hi) <= (u32)order) hi--;
    int lo = Min(st.minpo, hi);
    rs.k0 = st.k0mode ? k0_from_bps(bps, lim) : -1;
    u64 b = rs.bits_lf(lo, hi, lim, newv);
    rs.k0 = -1;
    return b;
}

// fixed predictor candidates for xs (w wasted bits, eb bits per sample): libFLAC's order guess,
// or all orders with exhaustive search; returns true if the signal is constant (best is final)
static bool sim_fixed(Ctx& cx, FrameState& st, const s64* xs, int n, int w, int eb, SimOut& best) {
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
            return true;
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
        u64 b = 8 + (u64)w + (u64)ord * eb + sim_rice_bits(cx, st, n, ord, rbps[ord]);
        if (b < best.bits) best = {b, ST_FIXED, ord, -1};
    }
    return false;
}

// LPC candidates for xs with the windows wids[0..nw): libFLAC's order guess (or all orders with
// exhaustive search) at the learned maximum order and precision
static void sim_lpc(Ctx& cx, FrameState& st, const s64* xs, int n, int w, int eb, const int* wids, int nw,
                    SimOut& best) {
    LpcPredictor& P = *cx.lp;
    P.begin(xs, n);
    const s32* x32 = to32(xs, n);
    int maxo = Min(st.lpc_exh ? st.lpc_maxseen : st.lpc_maxo, n - 1);
    if (maxo < 1) return;
    for (int wi = 0; wi < nw; wi++) {
        int id = wids[wi];
        int mo = P.eval(id, maxo);
        if (!mo) continue;
        int o0, o1;
        if (st.lpc_exh) {
            o0 = 1;
            o1 = mo;
        } else {
            o0 = o1 = lpc_best_order(P.lerr, mo, n, eb + (st.lpc_ov ? 5 : st.maxprec), st.ln2sq);
        }
        for (int ord = o0; ord <= o1; ord++) {
            double ebs = lpc_expected_bps(P.lerr[ord - 1], n - ord, st.ln2sq);
            if (ebs >= (double)eb) continue;
            int prec = st.maxprec;
            if (eb <= (st.fix_le ? 17 : 16)) prec = Min(prec, 32 - eb - ilog2_64(ord));
            s32 q[32];
            int sh;
            if (!lpc_quantize(P.lpall + (ord - 1) * 32, ord, prec, q, &sh)) continue;
            lpc_residual_fast(xs, x32, n, q, ord, sh, g_res2);
            u64 b = 8 + (u64)w + 9 + (u64)ord * (prec + eb) + sim_rice_bits(cx, st, n, ord, ebs);
            if (b < best.bits) best = {b, ST_LPC, ord, id};
        }
    }
}

// libFLAC's choice for one subframe signal xin (sbps bits): the cheapest of verbatim, constant,
// fixed and LPC by the encoder's own estimates
static SimOut sim_subframe(Ctx& cx, FrameState& st, const s64* xin, int n, int sbps, const int* wids, int nw) {
    s64* xs = g_simx;
    u64 o = 0;
    for (int i = 0; i < n; i++) o |= (u64)xin[i];
    int w = o ? ctz64(o) : 0;
    if (w >= sbps) w = sbps - 1;
    for (int i = 0; i < n; i++) xs[i] = xin[i] >> w;
    int eb = sbps - w;
    SimOut best = {8 + (u64)w + (u64)n * eb, ST_VERB, 0, -1};
    if (n <= 4) return best;
    if (sim_fixed(cx, st, xs, n, w, eb, best)) return best;
    if (nw) sim_lpc(cx, st, xs, n, w, eb, wids, nw, best);
    return best;
}

// libFLAC's channel assignment decision (-m); returns chcode or -1
static s64 g_stsig[4][MAXBS];  // L, R, M, S signals (2 MiB)
static int sim_stereo(Ctx& cx, FrameState& st, s32** pcm, int n) {
    LpcPredictor& P = *cx.lp;
    int wids[16];
    int nw = sim_windows(P, wids);
    for (int i = 0; i < n; i++) {
        s64 l = pcm[0][i], r = pcm[1][i];
        g_stsig[0][i] = l;
        g_stsig[1][i] = r;
        g_stsig[2][i] = (l + r) >> 1;
        g_stsig[3][i] = l - r;
    }
    u64 b[4];
    for (int k = 0; k < 4; k++) {
        g_sim4[k] = sim_subframe(cx, st, g_stsig[k], n, cx.sp.bps + (k == 3), wids, nw);
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
