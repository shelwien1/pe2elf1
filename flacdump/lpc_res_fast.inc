// flacdump: LpcFast - fast LPC residual for 32-bit-safe signals (analysis only): order-specialised kernels
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static s32 g_x32[MAXBS];  // 32-bit copy of the signal being analysed

struct LpcFast {
    typedef void (*Fn)(const s32*, int, const s32*, int, s64*);
    static inline Fn kernel[33];  // kernel[order], order 1..32

    template <int O> static void res(const s32* x, int n, const s32* c, int shift, s64* r) {
        for (int i = O; i < n; i++) {
            s64 s = 0;
            const s32* h = x + i;
            for (int j = 0; j < O; j++) s += (s64)c[j] * h[-1 - j];
            r[i - O] = (s64)x[i] - (s >> shift);
        }
    }
    template <int O> static void fill() {
        kernel[O] = res<O>;
        if constexpr (O > 1) fill<O - 1>();
    }
    static void init() { fill<32>(); }
    // 32-bit copy of x if all samples fit (products then cannot overflow 64-bit sums), else nullptr
    static const s32* to32(const s64* x, int n) {
        for (int i = 0; i < n; i++) {
            s64 v = x[i];
            if (v < -(1ll << 31) || v >= (1ll << 31)) return nullptr;
            g_x32[i] = (s32)v;
        }
        return g_x32;
    }
    // residual using the 32-bit copy x32 when there is one (nullptr: generic)
    static void residual(const s64* x, const s32* x32, int n, const s32* c, int order, int shift, s64* r) {
        if (x32 && order <= 32)
            kernel[order](x32, n, c, shift, r);
        else
            lpc_residual(x, n, c, order, shift, r);
    }
};
