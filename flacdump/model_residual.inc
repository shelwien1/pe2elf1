// flacdump: model_residual - partition order and rice parameters of a subframe
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// partition order: predicted by libFLAC's and ffmpeg's searches over the learned range
static bool model_porder(Ctx& cx, SubInfo& s, FrameState& st, int bs, int lim) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ResStats& rs = cx.rs;
    ac.cat = C_PORDER;
    int lo, hi;
    st.porder_range(rs, bs, s.order, lo, hi);
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
    return true;
}

// raw bit count of an escaped partition (res[0..n)), predicted as the minimal signed width
static int model_escape_bits(Ctx& cx, int raw, const s64* res, u32 n) {
    AC& ac = *cx.ac;
    ac.cat = C_ESC;
    u64 mx = 0;
    for (u32 j = 0; j < n; j++) {
        s64 v = res[j];
        u64 a = v < 0 ? (u64)(~v) : (u64)v;
        mx |= a;
    }
    int pred = n ? bitlen(mx) + 1 : 0;
    if (mx == 0) pred = 0;
    int rb = ac.tree(cx.m->esc_t[Min(pred, 32)], 5, raw);
    ac.cat = C_PARAM;
    return rb;
}

// rice parameter of partition i (31 = escape) with residual res[0..pn): predicted by libFLAC's
// estimators (before / since 1.4.3), ffmpeg's and the libFLAC <= 1.3 hint k0 for partition order 0
static int model_rice_param(Ctx& cx, SubInfo& s, FrameState& st, int i, int k0, int lim, const s64* res, u32 pn) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ResStats& rs = cx.rs;
    int po = s.porder;
    int esc_code = s.method ? 31 : 15;
    int kn = rs.est_lf(po, i, lim, true);
    int ko = rs.est_lf(po, i, lim, false);
    int kf = rs.est_ff(po, i, lim);
    int k = ac.dec ? 0 : (s.param[i] == esc_code ? 31 : s.param[i]);
    int ctx = (kn == ko) + 2 * (kn == kf) + 4 * (i == 0);
    int kvote = kn != ko;
    int use0 = po == 0 && k0 >= 0;
    if (use0 && k0 != (st.rice_new ? kn : ko)) {
        if (k == k0) st.k0vote[1]++;
        else if (k == (st.rice_new ? kn : ko)) st.k0vote[0]++;
        if (st.k0vote[0] + st.k0vote[1] >= VOTE_MIN) st.k0mode = st.k0vote[1] > st.k0vote[0];
    }
    if (use0 && st.k0mode && ac.bit(M.k_eq0[k0 == kn], k == k0))
        k = k0;
    else if (ac.bit(M.k_eq1[ctx], k == kn))
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
    if (k == 31) s.raw[i] = (u8)model_escape_bits(cx, s.raw[i], res, pn);
    return k;
}

// residual coding parameters of a subframe: partition order, rice parameters, method
static bool model_residual(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* res, int bs, int k0) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ResStats& rs = cx.rs;
    rs.compute(res, bs, s.order);
    int lim = cx.rice_limit();
    rs.k0 = st.k0mode ? k0 : -1;
    if (!model_porder(cx, s, st, bs, lim)) return false;
    // rice parameters (escape coded as 31)
    ac.cat = C_PARAM;
    int np = 1 << s.porder;
    int need2 = 0;
    u32 roff = 0;
    for (int i = 0; i < np; i++) {
        u32 pn = rs.psamples(s.porder, i);
        int k = model_rice_param(cx, s, st, i, k0, lim, res + roff, pn);
        if (k != 31 && k > 14) need2 = 1;
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
    if (st.rvote[0] + st.rvote[1] >= RICE_VOTE_MIN) st.rice_new = st.rvote[1] > st.rvote[0];
    rs.k0 = -1;
    return true;
}
