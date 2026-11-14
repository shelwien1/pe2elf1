// flacdump: WavWriter - writes the PCM as a WAV file
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct WavWriter {
    FILE* f;
    int nch, bps, cb;
    u32 rate;
    u64 nsamples;
    bool open(const char* name, int nch_, u32 rate_, int bps_) {
        f = fopen(name, "wb");
        if (!f) return false;
        nch = nch_;
        bps = bps_;
        rate = rate_;
        cb = (bps + 7) / 8;
        nsamples = 0;
        u8 h[80];
        int hl;
        wav_header(h, hl, nch, rate, bps, 0);
        fwrite(h, 1, hl, f);
        return true;
    }
    void write(s32** pcm, u32 n) {
        int sh = cb * 8 - bps;
        u8* o = g_wavbuf;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                u32 v = (u32)pcm[c][i] << sh;
                if (cb == 1)
                    *o++ = (u8)(v + 128);
                else
                    for (int b = 0; b < cb; b++) *o++ = (u8)(v >> (8 * b));
            }
        fwrite(g_wavbuf, 1, o - g_wavbuf, f);
        nsamples += n;
    }
    bool close() {
        u8 h[80];
        int hl;
        wav_header(h, hl, nch, rate, bps, nsamples);
        fseek64(f, 0, SEEK_SET);
        fwrite(h, 1, hl, f);
        bool ok = !ferror(f);
        fclose(f);
        return ok;
    }
};
