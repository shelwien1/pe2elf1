// flacdump: model_frame - codes one FRAME segment (header, channel mode, subframes)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Model one FRAME segment (enc: f/pcm filled; dec: fills f, reads pcm via src)
static bool model_frame(Ctx& cx, FrameInfo& f, s32** pcm, FrameState& st, WavReader* src, u64 samples_done) {
    AC& ac = *cx.ac;
    Models& M = *cx.m;
    g_ln2sq = st.ln2sq;
    ac.cat = C_HDR;
    f.blocking = ac.bit(M.blocking[st.pblocking], f.blocking);
    // block size
    ac.cat = C_BS;
    u64 remaining = cx.total_samples > samples_done ? cx.total_samples - samples_done : 0;
    u32 pbs = st.pbs;
    if (remaining && remaining < pbs) pbs = (u32)remaining;
    int ctx = st.first ? 0 : (pbs != st.pbs ? 2 : 1);
    if (ac.bit(M.bs_eq[ctx], f.bs == pbs))
        f.bs = pbs;
    else {
        u32 v = ac.tree(M.bsval, 16, f.bs - 1);
        f.bs = v + 1;
    }
    if (f.bs == 0 || f.bs > MAXBS) return false;
    if (ac.dec) {
        f.nch = cx.sp.nch;
        f.bps = cx.sp.bps;
        if (!wav_read(src, pcm, f.nch, f.bs)) return false;
    }
    int bs = (int)f.bs;
    ac.cat = C_HDR;
    int cb = canon_bscode(f.bs);
    if (ac.bit(M.bscode_eq[cb], f.bscode == cb))
        f.bscode = cb;
    else
        f.bscode = ac.tree(M.bscode_t[cb], 4, f.bscode);
    if (!bscode_ok(f.bscode, f.bs)) return false;
    if (ac.bit(M.sr_eq[0], f.srcode == st.psrcode))
        f.srcode = st.psrcode;
    else
        f.srcode = ac.tree(M.sr_t, 4, f.srcode);
    if (f.srcode >= 12 && f.srcode <= 14) {
        int nb = f.srcode == 12 ? 8 : 16;
        if (ac.bit(M.srv_eq[0], f.srval == st.psrval))
            f.srval = st.psrval;
        else
            f.srval = (u32)ac.raw(f.srval, nb);
    } else
        f.srval = 0;
    if (ac.bit(M.bpsc_eq[0], f.bpscode == st.pbpscode))
        f.bpscode = st.pbpscode;
    else
        f.bpscode = ac.tree(M.bpsc_t, 3, f.bpscode);
    u64 pnum = f.blocking ? st.sample_counter : st.frame_counter;
    if (ac.bit(M.num_eq[f.blocking], f.number == pnum))
        f.number = pnum;
    else
        f.number = ac.raw(f.number, 36);
    ac.cat = C_CH;
    if (cx.sp.nch == 2) {
        int pf = ff_stereo_guess(pcm, bs, cx.sp.bps > 16 ? 30 : 14);
        static const int map[4] = {1, 8, 9, 10};
        int cf = map[pf];
        int cl = st.sim_on ? sim_stereo(cx, st, pcm, bs) : -1;
        int done = 0;
        if (cl >= 0) {
            if (ac.bit(M.ch_sim[cl == cf][cl == st.pchcode], f.chcode == cl)) {
                f.chcode = cl;
                done = 1;
            }
        }
        if (!done && cf != cl) {
            if (ac.bit(M.ch_ff[cf == st.pchcode][cl >= 0], f.chcode == cf)) {
                f.chcode = cf;
                done = 1;
            }
        }
        if (!done) f.chcode = ac.tree(M.ch_t[st.pchcode][pf][0], 4, f.chcode);
        if (!ac.dec && g_verbose >= 2 && cl >= 0)
            st.simstat[f.chcode == cl]++;
    }
    else
        f.chcode = cx.sp.nch - 1;
    f.nch = f.chcode < 8 ? f.chcode + 1 : 2;
    if (f.nch != cx.sp.nch) return false;
    f.bps = cx.sp.bps;
    for (int ch = 0; ch < f.nch; ch++) {
        SubInfo& s = f.sub[ch];
        ChState& cs = st.ch[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        s64* x = g_x;
        subframe_signal(f.chcode, ch, pcm, bs, x);
        SimOut sim = {0, 4, 0, -1};
        if (st.sim_on) {
            if (cx.sp.nch == 2) {
                static const int sig_of[11][2] = {{0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
                                                  {0, 1}, {0, 1}, {0, 3}, {3, 1}, {2, 3}};
                sim = g_sim4[sig_of[f.chcode][ch]];
                if (f.chcode < 8 && f.chcode != 1) sim.type = 4;
            } else {
                int wids[16];
                int nw = sim_windows(*cx.lp, wids);
                sim = sim_subframe(cx, st, x, bs, sbps, wids, nw);
            }
        }
        // wasted bits
        ac.cat = C_WASTED;
        u64 o = 0;
        for (int i = 0; i < bs; i++) o |= (u64)x[i];
        int pw = o ? ctz64(o) : 0;
        if (pw >= sbps) pw = sbps - 1;
        if (ac.bit(M.w_eq[pw ? 1 : 0][Min(ch, 3)], s.wasted == pw))
            s.wasted = pw;
        else
            s.wasted = (int)code_uint(ac, M.w_val, s.wasted);
        if (s.wasted >= sbps) return false;
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] >>= s.wasted;
        // type
        ac.cat = C_TYPE;
        int isconst = 1;
        for (int i = 1; i < bs; i++)
            if (x[i] != x[0]) {
                isconst = 0;
                break;
            }
        s.type = ac.tree(M.type_t[isconst][sim.type][cs.ptype], 2, s.type);
        cs.ptype = s.type;
        if (s.type == ST_CONST) {
            if (!isconst) return false;
            s.order = 0;
            continue;
        }
        if (s.type == ST_VERB) {
            s.order = 0;
            continue;
        }
        int k0 = -1;
        ac.cat = C_ORDER;
        if (s.type == ST_FIXED) {
            int gn, go;
            u64 esum[5];
            fixed_guess(x, bs, gn, go, esum);
            int gs = sim.type == ST_FIXED ? sim.order : 5;
            s.order = ac.tree(M.forder_t[gn][gn == go][gs], 3, s.order);
            if (s.order > 4 || s.order > bs) return false;
            if (gn != go) {
                if (s.order == gn) st.fvote[1]++;
                if (s.order == go) st.fvote[0]++;
                if (st.fvote[0] + st.fvote[1] >= 4) st.fix_le = st.fvote[1] > st.fvote[0];
            }
            cs.pfixed = s.order;
            fixed_residual(x, bs, s.order, g_res);
            if (bs > 4) k0 = k0_from_bps(fixed_rbps(esum[s.order], bs - 4), cx.sp.bps > 16 ? 30 : 14);
        } else {
            ac.cat = C_PREC;
            s.prec = ac.tree(M.prec_t[cs.pprec], 4, s.prec - 1) + 1;
            if (s.prec > 15) return false;
            cs.pprec = s.prec - 1;
            if (!model_lpc(cx, s, cs, st, x, bs, sbps - s.wasted, sim, k0)) return false;
            lpc_residual(x, bs, s.coef, s.order, s.shift, g_res);
        }
        if (!model_residual(cx, s, cs, st, g_res, bs, k0)) return false;
    }
    // update state
    st.first = 0;
    st.pblocking = f.blocking;
    st.pbs = f.bs;
    st.psrcode = f.srcode;
    st.psrval = f.srval;
    st.pbpscode = f.bpscode;
    st.pchcode = f.chcode;
    if (f.blocking)
        st.sample_counter = f.number + f.bs;
    else
        st.frame_counter = f.number + 1;
    return true;
}
