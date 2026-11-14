// flacdump: apodization windows (libFLAC tukey / partial_tukey / punchout_tukey variants)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static const double k_pi = 3.14159265358979323846;

// window variants: v14 = libFLAC >= 1.3 (cosf), v12 = libFLAC <= 1.2 (cos)
// 0.5 - 0.5*cos(pi*i/N): libFLAC <= 1.3.3 uses double cos, >= 1.3.4 cosf
static inline float hann_tap(int i, int N, int v12) {
    if (v12 == 1) return (float)(0.5 - 0.5 * det_cos(k_pi * i / N));
    if (v12 == 2) return 0.5f - 0.5f * glibc_cosf((float)(k_pi * i / N));
    return 0.5f - 0.5f * det_cosf((float)(k_pi * i / N));
}

static void win_tukey(float* w, int L, float p, int v12) {
    for (int n = 0; n < L; n++) w[n] = 1.0f;
    if (p <= 0.0f) return;
    int Np = (int)(p / 2.0f * (float)L) - 1;
    if (Np > 0) {
        for (int n = 0; n <= Np; n++) {
            w[n] = hann_tap(n, Np, v12);
            w[L - Np - 1 + n] = hann_tap(n + Np, Np, v12);
        }
    }
}
static void win_partial_tukey(float* w, int L, float p, float start, float end, int v12) {
    int start_n = (int)(start * (float)L);
    int end_n = (int)(end * (float)L);
    int N = end_n - start_n;
    int Np = (int)(p / 2.0f * (float)N);
    int n, i;
    for (n = 0; n < start_n && n < L; n++) w[n] = 0.0f;
    for (i = 1; n < (start_n + Np) && n < L; n++, i++) w[n] = hann_tap(i, Np, v12);
    for (; n < (end_n - Np) && n < L; n++) w[n] = 1.0f;
    for (i = Np; n < end_n && n < L; n++, i--) w[n] = hann_tap(i, Np, v12);
    for (; n < L; n++) w[n] = 0.0f;
}
static void win_punchout_tukey(float* w, int L, float p, float start, float end, int v12) {
    int start_n = (int)(start * (float)L);
    int end_n = (int)(end * (float)L);
    int Ns = (int)(p / 2.0f * (float)start_n);
    int Ne = (int)(p / 2.0f * (float)(L - end_n));
    int n, i;
    for (n = 0, i = 1; n < Ns && n < L; n++, i++) w[n] = hann_tap(i, Ns, v12);
    for (; n < start_n - Ns && n < L; n++) w[n] = 1.0f;
    for (i = Ns; n < start_n && n < L; n++, i--) w[n] = hann_tap(i, Ns, v12);
    for (; n < end_n && n < L; n++) w[n] = 0.0f;
    for (i = 1; n < end_n + Ne && n < L; n++, i++) w[n] = hann_tap(i, Ne, v12);
    for (; n < L - (Ne) && n < L; n++) w[n] = 1.0f;
    for (i = Ne; n < L; n++, i--) w[n] = hann_tap(i, Ne, v12);
}

enum {
    W_T50,     // tukey(0.5), >= 1.3.4 (cosf)
    W_T50_12,  // tukey(0.5), <= 1.3.3 (cos)
    W_T25,     // subdivide_tukey(2) root
    W_T16,     // subdivide_tukey(3) root
    W_PT2_0,   // partial_tukey(2), cosf
    W_PT2_1,
    W_PO3_0,  // punchout_tukey(3), cosf
    W_PO3_1,
    W_PO3_2,
    W_PT2C_0,  // same, double cos (<= 1.3.3)
    W_PT2C_1,
    W_PO3C_0,
    W_PO3C_1,
    W_PO3C_2,
    W_T50G,  // glibc cosf variants (libFLAC >= 1.3.4 on Linux)
    W_T25G,
    W_T16G,
    W_PT2G_0,
    W_PT2G_1,
    W_PO3G_0,
    W_PO3G_1,
    W_PO3G_2,
    NWIN
};
// windows that differ only numerically map to the same base window
static int base_win(int w) {
    switch (w) {
        case W_T50_12: case W_T50G: return W_T50;
        case W_T25G: return W_T25;
        case W_T16G: return W_T16;
        case W_PT2C_0: case W_PT2G_0: return W_PT2_0;
        case W_PT2C_1: case W_PT2G_1: return W_PT2_1;
        case W_PO3C_0: case W_PO3G_0: return W_PO3_0;
        case W_PO3C_1: case W_PO3G_1: return W_PO3_1;
        case W_PO3C_2: case W_PO3G_2: return W_PO3_2;
        default: return w;
    }
}
