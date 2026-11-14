// flacdump: FrameInfo - header fields and subframe parameters of one FLAC frame
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct SubInfo {
    int type, wasted, order, prec, shift, method, porder;
    s32 coef[32];
    u8* param;  // rice parameter per partition (escape code = 15 / 31)
    u8* raw;    // raw bits for escaped partitions
};

struct FrameInfo {
    int blocking, bscode, srcode, chcode, bpscode;
    u32 bs, srval;
    u64 number;
    int nch, bps;
    SubInfo sub[MAXCH];
    // zeroed; the rice parameters and raw bit counts of the subframes point into g_parambuf
    void init() {
        memset(this, 0, sizeof(*this));
        for (int c = 0; c < MAXCH; c++) {
            sub[c].param = g_parambuf + (size_t)c * MAXPART * 2;
            sub[c].raw = sub[c].param + MAXPART;
        }
    }
};
