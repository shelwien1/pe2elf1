// flacdump: fast LPC residual: order-specialised kernels on a 32-bit copy of the signal
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// fast LPC residual for 32-bit-safe signals (analysis only)
template <int O> static void lpc_res_t(const s32* x, int n, const s32* c, int shift, s64* r) {
    for (int i = O; i < n; i++) {
        s64 s = 0;
        const s32* h = x + i;
        for (int j = 0; j < O; j++) s += (s64)c[j] * h[-1 - j];
        r[i - O] = (s64)x[i] - (s >> shift);
    }
}
typedef void (*LpcResFn)(const s32*, int, const s32*, int, s64*);
static LpcResFn g_lpcres[33];
template <int O> struct LpcResInit {
    static void run() {
        g_lpcres[O] = lpc_res_t<O>;
        LpcResInit<O - 1>::run();
    }
};
template <> struct LpcResInit<0> {
    static void run() {}
};
static s32 g_x32[MAXBS];
// residual using a 32-bit copy of x when possible (x32 = nullptr: generic)
static void lpc_residual_fast(const s64* x, const s32* x32, int n, const s32* c, int order, int shift, s64* r) {
    if (x32 && order <= 32)
        g_lpcres[order](x32, n, c, shift, r);
    else
        lpc_residual(x, n, c, order, shift, r);
}
// 32-bit copy if all samples fit (and products cannot overflow 64-bit sums)
static const s32* to32(const s64* x, int n) {
    for (int i = 0; i < n; i++) {
        s64 v = x[i];
        if (v < -(1ll << 31) || v >= (1ll << 31)) return nullptr;
        g_x32[i] = (s32)v;
    }
    return g_x32;
}

static void init_fast() { LpcResInit<32>::run(); }
