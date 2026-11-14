// flacdump: the meta model: bit models, per-frame/channel state, coding context
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct ChState {
    int ptype, pfixed, plorder, pprec, pshift, pmethod, pporder, prank, psim;
    int pcoef_n;
    s32 pcoef[32];
};

struct Models {
    BM seg[5][4];
    BM blocking[2];
    BM bs_eq[3];
    BM bsval[1 << 16];
    BM bscode_eq[16];
    BM bscode_t[16][16];
    BM sr_eq[2];
    BM sr_t[16];
    BM srv_eq[2];
    BM ch_t[16][4][4][16];
    BM ch_sim[2][2];
    BM ch_ff[2][2];
    BM bpsc_eq[2];
    BM bpsc_t[8];
    BM num_eq[2];
    BM w_eq[2][4];
    UModel w_val;
    BM type_t[2][5][5][4];
    BM forder_t[5][2][6][8];
    BM lorder_t[33][32];
    BM prec_t[16][16];
    BM shift_t[33][32];
    UModel coef_m[32];
    BM coef_s[32];
    BM cand_f[24][4];
    BM cand_g[24][4];
    BM lpc_sim[2][2];
    BM method_b[3][2];
    BM po_eqL[4];
    BM po_eqF[4];
    BM po_t[16][16];
    BM k_eq0[2];
    BM k_eq1[8];
    BM k_eq2[8];
    BM k_eq3[8];
    BM k_t[32][32];
    BM esc_t[33][32];
    // misc
    UModel len_m;
    UModel num_m;
    UModel clen_m;
    BM vendor_t[32];
    BM b64kind[4];
    BM mflag[16];
    BM mtype[8][128];
};

// priors: "prediction was right" flags start out confident
static void prior(BM* m, size_t cnt, int p1) {
    for (size_t i = 0; i < cnt; i++) {
        m[i].p = (u16)p1;
        m[i].n = 4;
    }
}
#define PRIOR(arr, p1) prior((BM*)&(arr), sizeof(arr) / sizeof(BM), p1)
static void models_init(Models& M) {
    model_init(M);
    const int HI = 62000, LO = 3500;
    PRIOR(M.blocking, LO);
    PRIOR(M.bs_eq, HI);
    PRIOR(M.bscode_eq, HI);
    PRIOR(M.sr_eq, HI);
    PRIOR(M.srv_eq, HI);
    PRIOR(M.bpsc_eq, HI);
    PRIOR(M.num_eq, HI);
    PRIOR(M.w_eq, HI);
    PRIOR(M.po_eqL, HI);
    PRIOR(M.k_eq0, HI);
    PRIOR(M.k_eq1, HI);
    PRIOR(M.ch_sim, HI);
    PRIOR(M.lpc_sim, HI);
    PRIOR(M.mflag, HI);
    M.mflag[3].p = LO;  // base64 value in a comment: rare
    M.mflag[6].p = LO;  // CR LF line breaks
    M.mflag[7].p = 32768;
    for (int k = 0; k < 4; k++) {
        M.cand_g[0][k].p = HI;
        M.cand_g[0][k].n = 2;
        for (int r = 1; r < 24; r++) {
            M.cand_g[r][k].p = 16000;
            M.cand_f[r][k].p = 16000;
        }
    }
    for (int i = 0; i < 5; i++) {
        M.seg[i][1].p = LO;  // FRAME segments dominate
        M.seg[i][1].n = 4;
    }
}

static Models g_models;       // the meta model (re-initialized for every run)
static LpcPredictor g_lpred;  // LPC candidate predictor

// learning thresholds (changing a value changes the .meta format)
enum {
    VOTE_MIN = 4,           // votes needed before a variant flag (ln2sq, fix_le, k0mode) may switch
    RICE_VOTE_MIN = 8,      // the same for the rice estimator variant (rice_new)
    LEARN_FULL = 64,        // LPC subframes that always update the order hypotheses,
    LEARN_EVERY = 16,       // afterwards only every 16th
    EXH_LEAD_MIN = 4,       // exhaustive search is assumed when it reproduces more orders than the
    EXH_LEAD_DIV = 4,       // order guess: by at least EXH_LEAD_MIN + 1/EXH_LEAD_DIV of its hits
    FF_SWITCH = 8,          // ffmpeg candidate hits after which the libFLAC simulation may be turned off
    SEARCH_FULL = 64,       // LPC candidates that never matched are tried in the first 64 subframes,
    SEARCH_MISS_RATIO = 4,  // while fewer than 1 in 4 subframes has no exact candidate,
    SEARCH_EVERY = 16,      // and in every 16th subframe
};

struct FrameState {
    int first;
    u32 ffh, lfh;
    int pblocking, pbscode, psrcode, pchcode, pbpscode;
    u32 psrval, pbs;
    u64 frame_counter, sample_counter;
    int minpo, maxpo;
    int lpc_maxo, lpc_ov;  // learned max lpc order and order-guess overhead variant
    u32 oscore[2][33];
    u32 nlearn;
    int lpc_maxseen, lpc_exh, maxprec;  // exhaustive-search hypothesis
    u32 exscore[2];
    int ff_method;  // ffmpeg order selection method
    u32 ffscore[4];
    int sim_on, sim_newv;  // libFLAC simulation enabled / 1.4 estimator variants
    u32 simstat[2];
    int rice_new, fix_le;  // libFLAC >= 1.4.3 rice estimate / >= 1.4.0 fixed-order tie rule
    int k0mode;            // libFLAC <= 1.3 suggested rice parameter for partition order 0
    u32 k0vote[2];
    int ln2sq;  // libFLAC <= 1.3.1 error scaling
    u32 esvote[2];
    u32 rvote[2], fvote[2];
    u32 dbg[8];
    ChState ch[MAXCH];
};

static void fstate_init(FrameState& s, const StreamParams& sp, int vendor) {
    memset(&s, 0, sizeof(s));
    s.first = 1;
    s.pbs = 4096;
    s.psrcode = canon_srcode(sp.rate);
    s.pbpscode = canon_bpscode(sp.bps);
    s.minpo = 15;
    s.maxpo = 0;
    s.lpc_maxo = 8;
    // vendor: 0 unknown, -1 ffmpeg, else libFLAC version*100 (e.g. 143)
    s.sim_on = vendor >= 0;
    s.rice_new = vendor == 0 || vendor >= 143;
    s.fix_le = vendor == 0 || vendor >= 140;
    s.k0mode = vendor > 0 && vendor < 140;
    s.ln2sq = vendor > 0 && vendor <= 131;
    for (int c = 0; c < MAXCH; c++) {
        s.ch[c].ptype = 4;
        s.ch[c].pfixed = 2;
        s.ch[c].plorder = 32;
        s.ch[c].pprec = 15;
        s.ch[c].pshift = 32;
        s.ch[c].pmethod = 2;
    }
}

struct Ctx {
    AC* ac;
    Models* m;
    StreamParams sp;
    ResStats rs;
    LpcPredictor* lp;
    u64 total_samples;  // total PCM samples (per channel) in wav
};

struct SimOut {
    u64 bits;
    int type, order, wid;
};
static SimOut g_sim4[4];  // simulated subframes of the L, R, M, S signals of the current frame

// Everything a run (compression pass 2, or a restore) changes, back to its initial
// state, so that the in-process verification behaves exactly like a fresh process.
static void reset_run_state() {
    models_init(g_models);
    g_lpred.init();
    extm_init();
    g_ext.count = 0;
    g_ext.bytes = 0;
    g_cutdepth = 0;
    memset(g_cmbusy, 0, sizeof(g_cmbusy));
    memset((void*)g_sim4, 0, sizeof(g_sim4));
}
