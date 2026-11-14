// flacdump: WinCache - apodization windows for the current block size
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct WinCache {
    int bs;
    float w[NWIN][MAXBS];  // 5.5 MiB (the only instance is part of the static g_lpred)
    void init() { bs = 0; }
    void ensure(int L) {
        if (L == bs) return;
        bs = L;
        win_tukey(w[W_T50], L, 0.5f, 0);
        win_tukey(w[W_T50_12], L, 0.5f, 1);
        win_tukey(w[W_T25], L, 0.5f / 2, 0);
        win_tukey(w[W_T16], L, 0.5f / 3, 0);
        win_tukey(w[W_T50G], L, 0.5f, 2);
        win_tukey(w[W_T25G], L, 0.5f / 2, 2);
        win_tukey(w[W_T16G], L, 0.5f / 3, 2);
        // partial_tukey(2) (overlap 0.1) and punchout_tukey(3) (overlap 0.2), for each cosine variant
        static const int pt2[3] = {W_PT2_0, W_PT2C_0, W_PT2G_0}, po3[3] = {W_PO3_0, W_PO3C_0, W_PO3G_0};
        float ou2 = 1.0f / (1.0f - 0.1f) - 1.0f, ou3 = 1.0f / (1.0f - 0.2f) - 1.0f;
        for (int v = 0; v < 3; v++) {
            for (int m = 0; m < 2; m++)
                win_partial_tukey(w[pt2[v] + m], L, 0.2f, m / (2 + ou2), (m + 1 + ou2) / (2 + ou2), v);
            for (int m = 0; m < 3; m++)
                win_punchout_tukey(w[po3[v] + m], L, 0.2f, m / (3 + ou3), (m + 1 + ou3) / (3 + ou3), v);
        }
    }
};
