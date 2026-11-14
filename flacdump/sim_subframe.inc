// flacdump: sim_subframe - libFLAC process_subframe_ cost simulation for one signal
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

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
        rs.k0 = st.k0mode ? k0_from_bps(rbps[ord], lim) : -1;
        u64 b = 8 + (u64)w + (u64)ord * eb + rs.bits_lf(lo, hi, lim, newv);
        rs.k0 = -1;
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
            rs.compute(g_res2, n, ord, false);
            int hi = Min(st.maxpo, rs.maxpo);
            while (hi > 0 && ((u32)n >> hi) <= (u32)ord) hi--;
            int lo = Min(st.minpo, hi);
            rs.k0 = st.k0mode ? k0_from_bps(ebs, lim) : -1;
            u64 b = 8 + (u64)w + 9 + (u64)ord * (prec + eb) + rs.bits_lf(lo, hi, lim, newv);
            rs.k0 = -1;
            if (b < best.bits) best = {b, ST_LPC, ord, id};
        }
    }
    return best;
}
