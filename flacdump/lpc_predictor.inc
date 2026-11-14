// flacdump: LpcPredictor - candidate LPC analyses (libFLAC/ffmpeg variants) in MRU order
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct CandDef {
    u8 kind;  // 0 full window, 1 subdivide partial, 2 subdivide punchout, 3 ffmpeg
    u8 win;
    u8 b, c;
    u8 quirk;
    u8 ac;
    u8 fma;  // Levinson compiled with fma contraction
};

enum { MAXCAND = 192 };

struct LpcPredictor {
    WinCache wc;
    CandDef cand[MAXCAND];
    int ncand;
    int mru[MAXCAND];  // candidate ids in rank order
    u32 hits[MAXCAND];
    u32 nsub;   // LPC subframes seen
    u32 nmiss;  // LPC subframes without exact candidate
    float* wd;
    // per-subframe memo of autocorrelations
    struct Memo {
        u8 kind, win, b, c, ac, nl;
        double v[33];
    } memo[160];
    int nmemo;
    const s64* x;
    int n;
    float lpall[32 * 32];
    double lerr[32];
    double lpd[32 * 32];  // ffmpeg coefficients (double)
    double* ffwbuf;

    void add(int kind, int win, int b, int c, int quirk, int acm, int fm = 0) {
        if (ncand >= MAXCAND) die("internal: too many candidates");
        CandDef& d = cand[ncand];
        d.kind = (u8)kind;
        d.win = (u8)win;
        d.b = (u8)b;
        d.c = (u8)c;
        d.quirk = (u8)quirk;
        d.ac = (u8)acm;
        d.fma = (u8)fm;
        mru[ncand] = ncand;
        hits[ncand] = 0;
        ncand++;
    }
    void init() {
        wc.init();
        static float s_wd[MAXBS + 64];
        static double s_ffw[MAXBS + 80];
        wd = s_wd;
        ffwbuf = s_ffw;
        memset(s_ffw, 0, sizeof(s_ffw));
        memset(lpall, 0, sizeof(lpall));
        memset(lerr, 0, sizeof(lerr));
        memset(lpd, 0, sizeof(lpd));
        nmemo = 0;
        ncand = 0;
        nsub = 0;
        nmiss = 0;
        add(0, W_T50, 0, 0, 0, AC_DBL);         // libFLAC 1.4 -0..-5
        add(3, 0, 0, 0, 0, 0);                  // ffmpeg flacenc
        add(0, W_T50_12, 0, 0, 0, AC_FASC);     // libFLAC 1.2/1.3.0-1.3.3
        add(0, W_T50_12, 0, 0, 0, AC_FNEW12);   // libFLAC <= 1.3.3 x86-64 -3..-6
        add(0, W_T50, 0, 0, 0, AC_FNEW12);      // libFLAC 1.3.4 x86-64
        add(0, W_T50, 0, 0, 0, AC_FASC);        // libFLAC 1.3.4 generic
        add(0, W_T50_12, 0, 0, 0, AC_X87);      // libFLAC 1.2 ia32 -7/-8
        add(0, W_T50_12, 0, 0, 0, AC_FNEW16);   // libFLAC <= 1.3.3 -7/-8
        add(0, W_PT2C_0, 0, 0, 0, AC_FNEW16);
        add(0, W_PT2C_1, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_0, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_1, 0, 0, 0, AC_FNEW16);
        add(0, W_PO3C_2, 0, 0, 0, AC_FNEW16);
        add(0, W_PT2C_0, 0, 0, 0, AC_FASC);
        add(0, W_PT2C_1, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_0, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_1, 0, 0, 0, AC_FASC);
        add(0, W_PO3C_2, 0, 0, 0, AC_FASC);
        add(0, W_PT2C_0, 0, 0, 0, AC_FNEW12);  // <= 1.3.3 -6
        add(0, W_PT2C_1, 0, 0, 0, AC_FNEW12);
        // libFLAC 1.4 subdivide_tukey(2)
        add(0, W_T25, 0, 0, 0, AC_DBL);
        add(1, W_T25, 2, 0, 0, AC_DBL);
        add(1, W_T25, 2, 2, 0, AC_DBL);
        // libFLAC 1.4 subdivide_tukey(3)
        add(0, W_T16, 0, 0, 0, AC_DBL);
        add(1, W_T16, 2, 0, 0, AC_DBL);
        add(1, W_T16, 2, 2, 0, AC_DBL);
        for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16, 3, c, 0, AC_DBL);
        for (int c = 1; c < 6; c += 2) add(2, W_T16, 3, c, 1, AC_DBL);
        // libFLAC 1.3.4 -6..-8
        add(0, W_T50, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FNEW12);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FASC);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FASC);
        // misc
        add(0, W_T50, 0, 0, 0, AC_FNEW8);
        add(0, W_T50_12, 0, 0, 0, AC_FNEW8);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_DBL);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_DBL);
        add(0, W_T50_12, 0, 0, 0, AC_DBL);
        add(0, W_T50, 0, 0, 0, AC_X87);
        // builds with fma contraction (arm64 etc.)
        add(0, W_T50, 0, 0, 0, AC_DBL, 1);
        add(0, W_T25, 0, 0, 0, AC_DBL, 1);
        add(1, W_T25, 2, 0, 0, AC_DBL, 1);
        add(1, W_T25, 2, 2, 0, AC_DBL, 1);
        add(0, W_T16, 0, 0, 0, AC_DBL, 1);
        add(1, W_T16, 2, 0, 0, AC_DBL, 1);
        add(1, W_T16, 2, 2, 0, AC_DBL, 1);
        for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16, 3, c, 0, AC_DBL, 1);
        add(0, W_T50_12, 0, 0, 0, AC_FFMA, 1);
        add(0, W_T50, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 2; k++) add(0, W_PT2C_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3C_0 + k, 0, 0, 0, AC_FFMA, 1);
        for (int k = 0; k < 3; k++) add(0, W_PO3_0 + k, 0, 0, 0, AC_X87);
        for (int k = 0; k < 2; k++) add(0, W_PT2_0 + k, 0, 0, 0, AC_X87);
        // libFLAC >= 1.3.4 linked against glibc (its cosf is not correctly rounded)
        for (int ac2 = 0; ac2 < 2; ac2++) {
            int am = ac2 ? AC_DDESC : AC_DBL;
            add(0, W_T50G, 0, 0, 0, am);
            add(0, W_T25G, 0, 0, 0, am);
            add(1, W_T25G, 2, 0, 0, am);
            add(1, W_T25G, 2, 2, 0, am);
            add(0, W_T16G, 0, 0, 0, am);
            add(1, W_T16G, 2, 0, 0, am);
            add(1, W_T16G, 2, 2, 0, am);
            for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16G, 3, c, 0, am);
            for (int c = 1; c < 6; c += 2) add(2, W_T16G, 3, c, 1, am);
            for (int k = 0; k < 2; k++) add(0, W_PT2G_0 + k, 0, 0, 0, am);
            for (int k = 0; k < 3; k++) add(0, W_PO3G_0 + k, 0, 0, 0, am);
        }
        add(0, W_T50G, 0, 0, 0, AC_FNEW12);  // 1.3.4 Linux
        add(0, W_T50G, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 2; k++) add(0, W_PT2G_0 + k, 0, 0, 0, AC_FNEW16);
        for (int k = 0; k < 3; k++) add(0, W_PO3G_0 + k, 0, 0, 0, AC_FNEW16);
        // libFLAC 1.4 on CPUs without FMA (SSE2) and on arm64 (NEON): descending sums
        for (int fm = 0; fm < 2; fm++) {
            add(0, W_T50, 0, 0, 0, AC_DDESC, fm);
            add(0, W_T25, 0, 0, 0, AC_DDESC, fm);
            add(1, W_T25, 2, 0, 0, AC_DDESC, fm);
            add(1, W_T25, 2, 2, 0, AC_DDESC, fm);
            add(0, W_T16, 0, 0, 0, AC_DDESC, fm);
            add(1, W_T16, 2, 0, 0, AC_DDESC, fm);
            add(1, W_T16, 2, 2, 0, AC_DDESC, fm);
            for (int c = 0; c < 6; c++) add(c & 1 ? 2 : 1, W_T16, 3, c, 0, AC_DDESC, fm);
            for (int c = 1; c < 6; c += 2) add(2, W_T16, 3, c, 1, AC_DDESC, fm);
        }
    }
    void begin(const s64* x_, int n_) {
        x = x_;
        n = n_;
        nmemo = 0;
        wc.ensure(n);
    }
    // autocorrelation of (window kind) with lags 0..maxlag
    const double* autoc(int kind, int win, int b, int c, int acm, int maxlag) {
        int lags = maxlag + 1;
        Memo* mp = nullptr;
        for (int i = 0; i < nmemo; i++) {
            Memo& m = memo[i];
            if (m.kind == kind && m.win == win && m.b == b && m.c == c && m.ac == acm) {
                if (m.nl >= lags) return m.v;
                mp = &m;
                break;
            }
        }
        if (!mp) mp = &memo[nmemo < 160 ? nmemo++ : 159];
        Memo& m = *mp;
        m.kind = (u8)kind;
        m.win = (u8)win;
        m.b = (u8)b;
        m.c = (u8)c;
        m.ac = (u8)acm;
        m.nl = (u8)lags;
        const float* w = wc.w[win];
        if (kind == 0) {
            for (int i = 0; i < n; i++) wd[i] = (float)x[i] * w[i];
            autoc_compute(wd, n, lags, acm, m.v);
        } else {
            // FLAC__lpc_window_data_partial
            int part_size = n / b / 2;
            int data_shift = (c / 2 * n) / b;
            int len = n / b;
            if (part_size + data_shift < n) {
                int i, j;
                for (i = 0; i < part_size; i++) wd[i] = (float)x[data_shift + i] * w[i];
                i = Min(i, n - part_size - data_shift);
                for (j = n - part_size; j < n; i++, j++) wd[i] = (float)x[data_shift + i] * w[j];
                if (i < n) wd[i] = 0.0f;
            }
            autoc_compute(wd, len, lags, acm, m.v);
        }
        return m.v;
    }
    // run candidate id up to max order mo: fills lpall/lerr, returns usable max order (0 = unusable)
    int eval(int id, int mo) {
        const CandDef& d = cand[id];
        double a[33];
        if (mo >= n) mo = n - 1;
        if (mo < 1) return 0;
        if (d.kind == 3) {
            if (n < 5) return 0;
            double* w = ffwbuf + 40;
            int sh33 = d.b;  // analysis on x>>1 for 33-bit side channels
            if (sh33) {
                static s64 tmp[MAXBS];
                for (int i = 0; i < n; i++) tmp[i] = x[i] >> 1;
                ff_welch(tmp, n, w);
            } else
                ff_welch(x, n, w);
            w[-1] = 0.0;
            w[n] = 0.0;
            w[n + 1] = 0.0;
            ff_autocorr(w, n, mo, a);
            ff_levinson(a, mo, lpd);
            return mo;
        }
        if ((d.kind == 1 || d.kind == 2) && n / d.b <= 32) return 0;
        if (d.kind == 2) {
            const double* root = autoc(0, d.win, 0, 0, d.ac, mo);
            const double* part = autoc(1, d.win, d.b, d.c - 1, d.ac, mo);
            for (int i = 0; i <= mo; i++) a[i] = root[i] - part[i];
            if (d.quirk) a[mo] = part[mo];
        } else {
            const double* v = autoc(d.kind, d.win, d.b, d.c, d.ac, mo);
            for (int i = 0; i <= mo; i++) a[i] = v[i];
        }
        if (a[0] == 0.0) return 0;
        return levinson_all(a, mo, lpall, lerr, d.fma);
    }
    // libFLAC <= 1.3 suggested rice parameter for partition order 0 after eval(id); none for ffmpeg
    int k0_hint(int id, int order, int bs, int klim) const {
        if (cand[id].kind == 3) return -1;
        return k0_from_bps(lpc_expected_bps(lerr[order - 1], bs - order), klim);
    }
    // quantized coefficients of the last eval()ed candidate id at an order (forced_sh >= 0: at that shift)
    bool quant(int id, int order, int prec, s32* q, int* sh, int forced_sh = -1) {
        if (cand[id].kind == 3) {
            ff_quantize(lpd + (order - 1) * 32, order, prec, q, sh, forced_sh);
            return true;
        }
        if (forced_sh >= 0) {
            lpc_quantize_at(lpall + (order - 1) * 32, order, prec, forced_sh, q);
            *sh = forced_sh;
            return true;
        }
        return lpc_quantize(lpall + (order - 1) * 32, order, prec, q, sh);
    }
    // candidate id at exactly this order -> quantized coefficients
    bool at_order(int id, int order, int prec, s32* q, int* sh) {
        int mo = eval(id, order);
        if (mo < order) return false;
        return quant(id, order, prec, q, sh);
    }
};
