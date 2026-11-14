// flacdump: Ctx - the coding context of the meta model (coder, models, stream, predictors)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct Ctx {
    AC* ac;
    Models* m;
    StreamParams sp;
    ResStats rs;
    LpcPredictor* lp;
    u64 total_samples;  // total PCM samples (per channel) in wav
    // largest rice parameter of the encoders: 14 (4-bit parameters) up to 16 bits per sample, 30 above
    int rice_limit() const { return sp.bps > 16 ? 30 : 14; }
};
