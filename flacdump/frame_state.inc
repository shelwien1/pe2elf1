// flacdump: FrameState - learned encoder hypotheses and previous-frame contexts
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct ChState {
    int ptype, plorder, pprec, pmethod, prank, psim;  // previous subframe's choices (contexts)
};

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
    int pblocking, psrcode, pchcode, pbpscode;
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
    int sim_on;  // libFLAC simulation enabled
    u32 simstat[2];
    int rice_new, fix_le;  // libFLAC >= 1.4.3 rice estimate / >= 1.4.0 fixed-order tie rule
    int k0mode;            // libFLAC <= 1.3 suggested rice parameter for partition order 0
    u32 k0vote[2];
    int ln2sq;  // libFLAC <= 1.3.1 error scaling
    u32 esvote[2];
    u32 rvote[2], fvote[2];
    u32 dbg[4];  // -v: LPC stage 0 / stage 1 misses and hits
    ChState ch[MAXCH];
    // initial state; vendor: encoder hint (0 unknown, -1 ffmpeg, else libFLAC version*100)
    void init(const StreamParams& sp, int vendor) {
        memset(this, 0, sizeof(*this));
        first = 1;
        pbs = 4096;
        psrcode = canon_srcode(sp.rate);
        pbpscode = canon_bpscode(sp.bps);
        minpo = 15;
        maxpo = 0;
        lpc_maxo = 8;
        sim_on = vendor >= 0;
        rice_new = vendor == 0 || vendor >= 143;
        fix_le = vendor == 0 || vendor >= 140;
        k0mode = vendor > 0 && vendor < 140;
        ln2sq = vendor > 0 && vendor <= 131;
        for (int c = 0; c < MAXCH; c++) {
            ch[c].ptype = 4;
            ch[c].plorder = 32;
            ch[c].pprec = 15;
            ch[c].pmethod = 2;
        }
    }
    // partition orders libFLAC searches for n residual samples after a predictor of the given order:
    // the learned range, reduced while the first partition would not be longer than the order
    void porder_range(const ResStats& rs, int n, int order, int& lo, int& hi) const {
        hi = Min(maxpo, rs.maxpo);
        while (hi > 0 && ((u32)n >> hi) <= (u32)order) hi--;
        lo = Min(minpo, hi);
    }
};
