// flacdump: WinCache - apodization windows for the current block size
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct WinCache {
    int bs;
    float* w[NWIN];
    void init() {
        bs = 0;
        static float s_win[NWIN][MAXBS];  // ~5.8 MB
        for (int i = 0; i < NWIN; i++) w[i] = s_win[i];
    }
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
        {
            float overlap = 0.1f;
            float ou = 1.0f / (1.0f - overlap) - 1.0f;
            int parts = 2;
            for (int m = 0; m < parts; m++) {
                win_partial_tukey(w[W_PT2_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 0);
                win_partial_tukey(w[W_PT2C_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 1);
                win_partial_tukey(w[W_PT2G_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 2);
            }
        }
        {
            float overlap = 0.2f;
            float ou = 1.0f / (1.0f - overlap) - 1.0f;
            int parts = 3;
            for (int m = 0; m < parts; m++) {
                win_punchout_tukey(w[W_PO3_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 0);
                win_punchout_tukey(w[W_PO3C_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 1);
                win_punchout_tukey(w[W_PO3G_0 + m], L, 0.2f, m / (parts + ou), (m + 1 + ou) / (parts + ou), 2);
            }
        }
    }
};
