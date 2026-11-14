// flacdump: model_residual - codes partition order and rice parameters of a subframe
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static bool model_residual(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* res, int bs, int k0) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    ResStats& rs = cx.rs;
    rs.compute(res, bs, s.order);
    int lim = cx.sp.bps > 16 ? 30 : 14;
    rs.k0 = st.k0mode ? k0 : -1;
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
    if (st.rvote[0] + st.rvote[1] >= RICE_VOTE_MIN) st.rice_new = st.rvote[1] > st.rvote[0];
    rs.k0 = -1;
    return true;
}
