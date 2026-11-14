// flacdump: UModel - adaptive Elias-gamma style integer coding
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Adaptive Elias-gamma style integer model
struct UModel {
    BM len[48];
    BM hi[48][4];
};
static u64 code_uint(AC& ac, UModel& m, u64 v) {
    u64 w = v + 1;
    int L = ac.dec ? 0 : ilog2_64(w);
    int i = 0;
    for (;; i++) {
        int b = ac.bit(m.len[Min(i, 47)], i < L);
        if (!b) break;
        if (i >= 63) break;
    }
    L = i;
    if (L == 0) return 0;
    u64 r = 1;
    int k = L;
    // first up to 2 mantissa bits modelled
    int ctx = 1;
    for (int j = 0; j < 2 && k > 0; j++) {
        k--;
        int b = ac.bit(m.hi[Min(L, 47)][ctx], (int)((w >> k) & 1));
        ctx = (ctx << 1 | b) & 3;
        if (j == 0) ctx = 2 + b;
        r = (r << 1) | (u64)b;
    }
    if (k > 0) r = (r << k) | ac.raw(w, k);
    return r - 1;
}
static s64 code_sint(AC& ac, UModel& m, BM& sign, s64 v) {
    // zero/sign/magnitude
    u64 mag = ac.dec ? 0 : (u64)(v < 0 ? -v : v);
    int neg = v < 0;
    u64 r = code_uint(ac, m, mag);
    if (r) {
        neg = ac.bit(sign, neg);
        return neg ? -(s64)r : (s64)r;
    }
    return 0;
}
