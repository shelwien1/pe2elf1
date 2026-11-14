// flacdump: WavReader - reads the PCM back from a WAV file
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct WavReader {
    FILE* f;
    int nch, bps, cb;
    u64 nsamples, pos;
    bool open(const char* name) {
        f = fopen(name, "rb");
        if (!f) return false;
        u8 h[12];
        if (fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
        int got_fmt = 0;
        int fch = 0, fbits = 0;
        for (;;) {
            u8 ch[8];
            if (fread(ch, 1, 8, f) != 8) return false;
            u32 sz = rd_le32(ch + 4);
            if (!memcmp(ch, "fmt ", 4)) {
                u8 fm[64];
                u32 k = Min(sz, (u32)64);
                if (fread(fm, 1, k, f) != k) return false;
                fch = rd_le16(fm + 2);
                fbits = rd_le16(fm + 14);
                got_fmt = 1;
                if (sz > k) fseek64(f, sz - k, SEEK_CUR);
                if (sz & 1) fseek64(f, 1, SEEK_CUR);
            } else if (!memcmp(ch, "data", 4)) {
                if (!got_fmt) return false;
                nch = fch;
                cb = (fbits + 7) / 8;
                s64 here = ftell64(f);
                fseek64(f, 0, SEEK_END);
                s64 end = ftell64(f);
                fseek64(f, here, SEEK_SET);
                u64 avail = (u64)(end - here);
                u64 dsz = sz;
                if (dsz == 0xFFFFFFFF || dsz == 0 || dsz > avail) dsz = avail;
                if (!nch || !cb) return false;
                nsamples = dsz / ((u64)nch * cb);
                pos = 0;
                return true;
            } else {
                fseek64(f, (s64)sz + (sz & 1), SEEK_CUR);
            }
        }
    }
    bool read(s32** pcm, int n_ch, u32 n) {
        if (n_ch != nch || nch > MAXCH || cb > 4 || n > MAXBS || pos + n > nsamples) return false;
        size_t bytes = (size_t)n * nch * cb;
        if (fread(g_wavbuf, 1, bytes, f) != bytes) return false;
        int sh = cb * 8 - bps;
        const u8* p = g_wavbuf;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                s32 v;
                if (cb == 1)
                    v = (s32)(s8)(p[0] - 128);
                else if (cb == 2)
                    v = (s16)(p[0] | (p[1] << 8));
                else if (cb == 3)
                    v = (s32)((u32)(p[0] | (p[1] << 8) | (p[2] << 16)) << 8) >> 8;
                else
                    v = (s32)rd_le32(p);
                p += cb;
                pcm[c][i] = v >> sh;
            }
        pos += n;
        return true;
    }
    void close() {
        if (f) fclose(f);
        f = nullptr;
    }
};
