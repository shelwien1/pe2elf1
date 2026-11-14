// flacdump: WAV header and channel mask; sample buffer shared by WavWriter and WavReader
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static u32 default_mask(int nch) {
    static const u32 m[9] = {0, 0x4, 0x3, 0x7, 0x33, 0x37, 0x3F, 0x70F, 0x63F};
    return m[nch];
}

static void wav_header(u8* h, int& hl, int nch, u32 rate, int bps, u64 nsamples) {
    int cb = (bps + 7) / 8;
    u64 data = nsamples * nch * cb;
    bool ext = nch > 2 || (bps & 7);
    int fmtlen = ext ? 40 : 16;
    hl = 12 + 8 + fmtlen + 8;
    u64 riff = hl - 8 + data;
    memcpy(h, "RIFF", 4);
    wr_le32(h + 4, riff > 0xFFFFFFFFull ? 0xFFFFFFFF : (u32)riff);
    memcpy(h + 8, "WAVEfmt ", 8);
    wr_le32(h + 16, fmtlen);
    wr_le16(h + 20, ext ? 0xFFFE : 1);
    wr_le16(h + 22, nch);
    wr_le32(h + 24, rate);
    wr_le32(h + 28, rate * nch * cb);
    wr_le16(h + 32, nch * cb);
    wr_le16(h + 34, cb * 8);
    int p = 36;
    if (ext) {
        wr_le16(h + 36, 22);
        wr_le16(h + 38, bps);
        wr_le32(h + 40, default_mask(nch));
        static const u8 guid[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71};
        memcpy(h + 44, guid, 16);
        p = 60;
    }
    memcpy(h + p, "data", 4);
    wr_le32(h + p + 4, data > 0xFFFFFFFFull ? 0xFFFFFFFF : (u32)data);
}

static u8 g_wavbuf[(size_t)MAXBS * MAXCH * 4];  // one block of interleaved samples
