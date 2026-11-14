// flacdump: model_lpc - codes LPC order, precision, shift and coefficients against predicted candidates
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static bool model_lpc(Ctx& cx, SubInfo& s, ChState& cs, FrameState& st, const s64* x, int bs, int ebps,
                      const SimOut& sim, int& k0) {
    k0 = -1;
    const int klim = cx.sp.bps > 16 ? 30 : 14;
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    LpcPredictor& P = *cx.lp;
    P.begin(x, bs);
    s32 q[32];
    int sh = 0;
    // encoder-side search budget: candidates that never matched are only tried when
    // exact matches are common (or occasionally), to bound the time spent on unknown encoders
    int try_unhit = P.nsub < SEARCH_FULL || P.nmiss * SEARCH_MISS_RATIO < P.nsub || P.nsub % SEARCH_EVERY == 0;
    P.nsub++;
    // stage 0: the simulated encoder's choice (window + order)
    if (sim.type == ST_LPC && sim.wid >= 0 && sim.order <= bs - 1) {
        ac.cat = C_ORDER;
        int id = sim.wid;
        int maxo = Min(st.lpc_exh ? st.lpc_maxseen : st.lpc_maxo, bs - 1);
        int ok = 0;
        int mo = P.eval(id, Max(maxo, sim.order));
        if (mo >= sim.order && P.quant(id, sim.order, s.prec, q, &sh)) ok = 1;
        int hit = ok && !ac.dec && sim.order == s.order && sh == s.shift && !memcmp(q, s.coef, sizeof(s32) * s.order);
        hit = ac.bit(M.lpc_sim[ok][cs.psim], hit);
        cs.psim = hit;
        st.dbg[hit]++;
        if (hit) {
            if (!ok) return false;
            s.order = sim.order;
            s.shift = sh;
            memcpy(s.coef, q, sizeof(s32) * s.order);
            int r = 0;
            while (r < P.ncand && P.mru[r] != id) r++;
            if (r < P.ncand) mru_hit(P, r);
            st.lfh++;
            cs.plorder = s.order - 1;
            cs.pshift = s.shift;
            cs.prank = 0;
            k0 = P.k0_hint(id, s.order, bs, klim, st.ln2sq);
        lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
            return true;
        }
    }
    // stage 1: window candidate with the encoder's own order choice
    ac.cat = C_ORDER;
    int found = -1;
    if (!ac.dec) {
        // (never-matched candidates are left to the cheaper stage-2 search)
        for (int pass = 0; pass < 2 && found < 0; pass++) {
            if (pass && (!try_unhit || P.nsub > 64)) break;
            for (int r = 0; r < P.ncand; r++) {
                int id = P.mru[r];
                if ((P.hits[id] == 0) != (pass == 1)) continue;
                int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
                if (g != s.order) continue;
                if (!P.quant(id, g, s.prec, q, &sh)) continue;
                if (sh != s.shift || memcmp(q, s.coef, sizeof(s32) * g)) continue;
                found = r;
                break;
            }
        }
    }
    int r;
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_g[Min(r, 23)][cs.prank], r == found)) break;
    st.dbg[2 + (r < P.ncand)]++;
    if (r < P.ncand) {
        int id = P.mru[r];
        int g = order_guess(cx, st, id, x, bs, ebps, s.prec);
        if (g < 1 || g > bs || !P.quant(id, g, s.prec, q, &sh)) return false;
        s.order = g;
        s.shift = sh;
        memcpy(s.coef, q, sizeof(s32) * g);
        mru_hit(P, r);
        if (P.cand[id].kind == 3) st.ffh++; else st.lfh++;
        if (st.ffh > FF_SWITCH && st.ffh > st.lfh) st.sim_on = 0;
        cs.prank = r == 0 ? 0 : r < 4 ? 1 : 2;
        cs.plorder = s.order - 1;
        cs.pshift = s.shift;
        k0 = P.k0_hint(id, s.order, bs, klim, st.ln2sq);
        lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
        return true;
    }
    // stage 2: explicit order, then window candidate at that order
    s.order = ac.tree(M.lorder_t[cs.plorder], 5, s.order - 1) + 1;
    if (s.order > bs) return false;
    cs.plorder = s.order - 1;
    ac.cat = C_COEF;
    found = -1;
    if (!ac.dec) {
        for (int pass = 0; pass < 2 && found < 0; pass++) {
            if (pass && !try_unhit) break;
            for (int k = 0; k < P.ncand; k++) {
                int id = P.mru[k];
                if ((P.hits[id] == 0) != (pass == 1)) continue;
                if (!P.at_order(id, s.order, s.prec, q, &sh)) continue;
                if (sh != s.shift || memcmp(q, s.coef, sizeof(s32) * s.order)) continue;
                found = k;
                break;
            }
        }
    }
    if (!ac.dec && found < 0 && g_verbose >= 3) {
        fprintf(stderr, "miss: n=%d order=%d prec=%d shift=%d coefs:", bs, s.order, s.prec, s.shift);
        for (int i = 0; i < s.order; i++) fprintf(stderr, " %d", s.coef[i]);
        fprintf(stderr, "\n");
        for (int id = 0; id < P.ncand; id++) {
            if (!P.at_order(id, s.order, s.prec, q, &sh)) continue;
            int nd = 0, md = 0;
            for (int i = 0; i < s.order; i++) {
                int d = q[i] - s.coef[i];
                if (d) nd++;
                if (d < 0) d = -d;
                if (d > md) md = d;
            }
            if (sh == s.shift && md < 50) {
                fprintf(stderr, "   cand %2d sh=%d ndiff=%d maxd=%d:", id, sh, nd, md);
                for (int i = 0; i < s.order; i++) fprintf(stderr, " %d", q[i]);
                fprintf(stderr, "\n");
            }
        }
    }
    for (r = 0; r < P.ncand; r++)
        if (ac.bit(M.cand_f[Min(r, 23)][cs.prank], r == found)) break;
    if (r < P.ncand) {
        int id = P.mru[r];
        if (!P.at_order(id, s.order, s.prec, q, &sh)) return false;
        s.shift = sh;
        memcpy(s.coef, q, sizeof(s32) * s.order);
        mru_hit(P, r);
        if (P.cand[id].kind == 3) st.ffh++; else st.lfh++;
        if (st.ffh > FF_SWITCH && st.ffh > st.lfh) st.sim_on = 0;
        cs.prank = r == 0 ? 0 : r < 4 ? 1 : 2;
        k0 = P.k0_hint(id, s.order, bs, klim, st.ln2sq);
        lpc_learn(cx, st, id, s.order, bs, ebps, s.prec, x);
    } else {
        cs.prank = 3;
        P.nmiss++;
        // no exact candidate: code shift and deltas against the best guess
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
    }
    cs.pshift = s.shift;
    return true;
}
