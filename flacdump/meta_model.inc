// flacdump: the meta model instances, simulation results, run-state reset
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static Models g_models;       // the meta model (re-initialized for every run)
static LpcPredictor g_lpred;  // LPC candidate predictor

struct SimOut {
    u64 bits;
    int type, order, wid;
};
static SimOut g_sim4[4];  // simulated subframes of the L, R, M, S signals of the current frame

// Everything a run (compression pass 2, or a restore) changes, back to its initial
// state, so that the in-process verification behaves exactly like a fresh process.
static void reset_run_state() {
    g_models.init();
    g_lpred.init();
    g_extm.init();
    g_ext.count = 0;
    g_ext.bytes = 0;
    g_cutdepth = 0;
    memset(g_cmbusy, 0, sizeof(g_cmbusy));
    memset((void*)g_sim4, 0, sizeof(g_sim4));
}
