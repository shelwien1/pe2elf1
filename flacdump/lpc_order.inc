// flacdump: LPC order prediction (libFLAC guess / exhaustive search, ffmpeg methods) and learning
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// residual bits of g_res2 (predictor order 'order', n samples) as libFLAC's rice parameter
// search over the learned partition order range estimates them; bps: expected bits per sample
// (gives the libFLAC <= 1.3 parameter hint for partition order 0)
static u64 lf_rice_bits(Ctx& cx, FrameState& st, int n, int order, double bps) {
    ResStats& rs = cx.rs;
    int lim = rice_limit(cx);
    int lo, hi;
    rs.compute(g_res2, n, order, false);
    porder_range(st, rs, n, order, lo, hi);
    rs.k0 = st.k0mode ? k0_from_bps(bps, lim) : -1;
    u64 b = rs.bits_lf(lo, hi, lim, st.rice_new);
    rs.k0 = -1;
    return b;
}

// libFLAC's estimate of an LPC subframe of order o for x (x32: 32-bit copy or null, eb bits per
// sample): coefficients and residual, without the subframe header; uses the candidate last
// evaluated by P. ~0 if libFLAC does not try the order or cannot quantize it.
static u64 lpc_order_bits(Ctx& cx, FrameState& st, const s64* x, const s32* x32, int n, int o, int eb) {
    LpcPredictor& P = *cx.lp;
    double ebs = lpc_expected_bps(P.lerr[o - 1], n - o, st.ln2sq);
    if (ebs >= (double)eb) return ~0ull;  // "don't even try"
    int prec = st.maxprec;
    if (eb <= (st.fix_le ? 17 : 16)) prec = Min(prec, 32 - eb - ilog2_64(o));
    s32 q[32];
    int sh;
    if (!lpc_quantize(P.lpall + (o - 1) * 32, o, prec, q, &sh)) return ~0ull;
    lpc_residual_fast(x, x32, n, q, o, sh, g_res2);
    return (u64)o * (prec + eb) + lf_rice_bits(cx, st, n, o, ebs);
}

// libFLAC exhaustive model search: order with minimal estimated subframe bits.
// Leaves P.lpall valid for the candidate (evaluated up to maxo).
static int lpc_exh_guess(Ctx& cx, FrameState& st, int id, int maxo, const s64* x, int bs, int ebps) {
    LpcPredictor& P = *cx.lp;
    int mo = P.eval(id, Min(maxo, bs - 1));
    if (!mo) return 0;
    const s32* x32 = to32(x, bs);
    int best = 0;
    u64 bb = ~0ull;
    for (int o = 1; o <= mo; o++) {
        u64 bits = lpc_order_bits(cx, st, x, x32, bs, o, ebps);
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
    int lim = rice_limit(cx);
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
    return lpc_best_order(P.lerr, mo, bs, ebps + (st.lpc_ov ? 5 : prec), st.ln2sq);
}

// learn which hypotheses reproduce the encoder's order choice
static void lpc_learn(Ctx& cx, FrameState& st, int id, int order, int bs, int ebps, int prec, const s64* x) {
    LpcPredictor& P = *cx.lp;
    if (order > st.lpc_maxseen) st.lpc_maxseen = order;
    if (prec > st.maxprec) st.maxprec = prec;
    if (st.nlearn >= LEARN_FULL && st.nlearn % LEARN_EVERY) {
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
            int ovh = ebps + (st.lpc_ov ? 5 : prec);
            gn = lpc_best_order(P.lerr, mo, bs, ovh, st.ln2sq);
            int gn2 = lpc_best_order(P.lerr, mo, bs, ovh, !st.ln2sq);  // the other error scaling
            if (gn != gn2) {
                st.esvote[st.ln2sq] += gn == order;
                st.esvote[!st.ln2sq] += gn2 == order;
                if (st.esvote[0] + st.esvote[1] >= VOTE_MIN) st.ln2sq = st.esvote[1] > st.esvote[0];
            }
        }
    }
    int ge = lpc_exh_guess(cx, st, id, st.lpc_maxseen, x, bs, ebps);
    st.exscore[0] += gn == order;
    st.exscore[1] += ge == order;
    // exhaustive search is expensive to simulate: require clear evidence
    st.lpc_exh = st.exscore[1] > st.exscore[0] + st.exscore[0] / EXH_LEAD_DIV + EXH_LEAD_MIN;
    int mo = P.eval(id, Min(32, bs - 1));
    for (int v = 0; v < 2; v++) {
        int ov = ebps + (v ? 5 : prec);
        for (int m = order; m <= mo; m++)
            if (lpc_best_order(P.lerr, m, bs, ov, st.ln2sq) == order) st.oscore[v][m]++;
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
