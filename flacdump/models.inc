// flacdump: Models - the bit models of the meta model and their priors
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

#define PRIOR(arr, p1) prior((BM*)&(arr), sizeof(arr) / sizeof(BM), p1)
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
    BM mflag[7];
    BM mtype[8][128];
    // priors: "prediction was right" flags start out confident
    static void prior(BM* m, size_t cnt, int p1) {
        for (size_t i = 0; i < cnt; i++) {
            m[i].p = (u16)p1;
            m[i].n = 4;
        }
    }
    void init() {
        model_init(*this);
        const int HI = 62000, LO = 3500;
        PRIOR(blocking, LO);
        PRIOR(bs_eq, HI);
        PRIOR(bscode_eq, HI);
        PRIOR(sr_eq, HI);
        PRIOR(srv_eq, HI);
        PRIOR(bpsc_eq, HI);
        PRIOR(num_eq, HI);
        PRIOR(w_eq, HI);
        PRIOR(po_eqL, HI);
        PRIOR(k_eq0, HI);
        PRIOR(k_eq1, HI);
        PRIOR(ch_sim, HI);
        PRIOR(lpc_sim, HI);
        PRIOR(mflag, HI);
        mflag[3].p = LO;  // base64 value in a comment: rare
        mflag[5].p = LO;  // CR LF line breaks
        mflag[6].p = 32768;
        for (int k = 0; k < 4; k++) {
            cand_g[0][k].p = HI;
            cand_g[0][k].n = 2;
            for (int r = 1; r < 24; r++) {
                cand_g[r][k].p = 16000;
                cand_f[r][k].p = 16000;
            }
        }
        for (int i = 0; i < 5; i++) {
            seg[i][1].p = LO;  // FRAME segments dominate
            seg[i][1].n = 4;
        }
    }
};
