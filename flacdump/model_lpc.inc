// flacdump: model_lpc - LPC order, shift and coefficients, predicted from candidate analyses
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// a candidate at MRU rank r reproduced the coefficients q (shift sh) at order s.order: adopt
// them, update the statistics and learn from the encoder's choice; returns the k0 hint
static int lpc_accept(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* x, int bs, int ebps, int r,
                      const s32* q, int sh) {
    LpcPredictor& P = *cx.lp;
    int id = P.mru[r];
    s.shift = sh;
    memcpy(s.coef, q, sizeof(s32) * s.order);
    P.mru_hit(r);
    if (P.cand[id].kind == 3) st.ffh++; else st.lfh++;
    if (st.ffh > FF_SWITCH && st.ffh > st.lfh) st.sim_on = 0;
    cs.prank = r == 0 ? 0 : r < 4 ? 1 : 2;
    cs.plorder = s.order - 1;
    int k0 = P.k0_hint(id, s.order, bs, cx.rice_limit(), st.ln2sq);
    lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
    return k0;
}

// stage 0: the simulated encoder's choice (window + order); 1 = taken, 0 = not, -1 = invalid
static int lpc_stage0(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* x, int bs, int ebps,
                      const SimOut& sim, int& k0) {
    AC& ac = *cx.ac;
    LpcPredictor& P = *cx.lp;
    ac.cat = C_ORDER;
    int id = sim.wid;
    int maxo = Min(st.lpc_exh ? st.lpc_maxseen : st.lpc_maxo, bs - 1);
    s32 q[32];
    int sh = 0, ok = 0;
    int mo = P.eval(id, Max(maxo, sim.order));
    if (mo >= sim.order && P.quant(id, sim.order, s.prec, q, &sh)) ok = 1;
    int hit = ok && !ac.dec && sim.order == s.order && sh == s.shift && !memcmp(q, s.coef, sizeof(s32) * s.order);
    hit = ac.bit(cx.m->lpc_sim[ok][cs.psim], hit);
    cs.psim = hit;
    st.dbg[hit]++;
    if (!hit) return 0;
    if (!ok) return -1;
    s.order = sim.order;
    s.shift = sh;
    memcpy(s.coef, q, sizeof(s32) * s.order);
    int r = 0;
    while (r < P.ncand && P.mru[r] != id) r++;
    if (r < P.ncand) P.mru_hit(r);
    st.lfh++;
    cs.plorder = s.order - 1;
    cs.prank = 0;
    k0 = P.k0_hint(id, s.order, bs, cx.rice_limit(), st.ln2sq);
    lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
    return 1;
}

// encoder, stage 1: MRU rank of the first candidate whose own order choice reproduces the
// subframe (-1: none). Never-matched candidates only within the search budget and in the first
// subframes (later they are left to the cheaper stage-2 search).
static int lpc_find_guess(Ctx& cx, const SubInfo& s, FrameState& st, const s64* x, int bs, int ebps, bool try_unhit) {
    LpcPredictor& P = *cx.lp;
    s32 q[32];
    int sh;
    for (int pass = 0; pass < 2; pass++) {
        if (pass && (!try_unhit || P.nsub > SEARCH_FULL)) break;
        for (int r = 0; r < P.ncand; r++) {
            int id = P.mru[r];
            if ((P.hits[id] == 0) != (pass == 1)) continue;
            int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
            if (g != s.order) continue;
            if (!P.quant(id, g, s.prec, q, &sh)) continue;
            if (sh != s.shift || memcmp(q, s.coef, sizeof(s32) * g)) continue;
            return r;
        }
    }
    return -1;
}



// no candidate reproduces the subframe: code the shift and the deltas against the best guess
static bool lpc_code_explicit(Ctx& cx, SubInfo& s, ChState& cs) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    LpcPredictor& P = *cx.lp;
    s32 q[32];
    int sh = 0;
    cs.prank = 3;
    P.nmiss++;
    int have = -1;
    for (int k = 0; k < Min(P.ncand, 4) && have < 0; k++)
        if (P.at_order(P.mru[k], s.order, s.prec, q, &sh)) have = P.mru[k];
    int gs = have >= 0 ? sh : 32;
    ac.cat = C_PREC;
    s.shift = ac.tree(M.shift_t[gs], 5, (u32)s.shift & 31);
    if (s.shift > 15) return false;
    ac.cat = C_COEF;
    if (have >= 0)
        P.quant(have, s.order, s.prec, q, &sh, s.shift);
    else
        memset(q, 0, sizeof(q));
    s32 lim = 1 << (s.prec - 1);
    for (int i = 0; i < s.order; i++) {
        s64 d = code_sint(ac, M.coef_m[i], M.coef_s[i], (s64)s.coef[i] - q[i]);
        s64 v = q[i] + d;
        if (v < -lim || v >= lim) return false;
        s.coef[i] = (s32)v;
    }
    return true;
}

// LPC subframe parameters (order, shift, coefficients; the precision is already coded),
// predicted in three stages; k0: libFLAC <= 1.3 rice parameter hint for partition order 0
static bool model_lpc(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* x, int bs, int ebps,
                      const SimOut& sim, int& k0) {
    k0 = -1;
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    LpcPredictor& P = *cx.lp;
    P.begin(x, bs);
    s32 q[32];
    int sh = 0;
    // encoder-side search budget: candidates that never matched are only tried when
    // exact matches are common (or occasionally), to bound the time spent on unknown encoders
    bool try_unhit = P.nsub < SEARCH_FULL || P.nmiss * SEARCH_MISS_RATIO < P.nsub || P.nsub % SEARCH_EVERY == 0;
    P.nsub++;
    if (sim.type == ST_LPC && sim.wid >= 0 && sim.order <= bs - 1) {
        int t = lpc_stage0(cx, s, cs, st, x, bs, ebps, sim, k0);
        if (t) return t > 0;
    }
    // stage 1: window candidate with the encoder's own order choice
    ac.cat = C_ORDER;
    int found = ac.dec ? -1 : lpc_find_guess(cx, s, st, x, bs, ebps, try_unhit);
    int r;
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_g[Min(r, 23)][cs.prank], r == found)) break;
    st.dbg[2 + (r < P.ncand)]++;
    if (r < P.ncand) {
        int id = P.mru[r];
        int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
        if (g < 1 || g > bs || !P.quant(id, g, s.prec, q, &sh)) return false;
        s.order = g;
        k0 = lpc_accept(cx, s, cs, st, x, bs, ebps, r, q, sh);
        return true;
    }
    // stage 2: explicit order, then window candidate at that order
    s.order = ac.tree(M.lorder_t[cs.plorder], 5, s.order - 1) + 1;
    if (s.order > bs) return false;
    cs.plorder = s.order - 1;
    ac.cat = C_COEF;
    found = ac.dec ? -1 : P.find_at_order(s, try_unhit);
    if (!ac.dec && found < 0 && g_verbose >= 3) P.debug_miss(s, bs);
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_f[Min(r, 23)][cs.prank], r == found)) break;
    if (r < P.ncand) {
        if (!P.at_order(P.mru[r], s.order, s.prec, q, &sh)) return false;
        k0 = lpc_accept(cx, s, cs, st, x, bs, ebps, r, q, sh);
        return true;
    }
    return lpc_code_explicit(cx, s, cs);
}
