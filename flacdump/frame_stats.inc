// flacdump: FrameStats - frame list and MD5 gathered while streaming frames
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Stats gathered while streaming frames (both directions)
struct FrameStats {
    u64 nframes;
    u32 first_bs, min_bs, max_bs, min_bs_nl, max_bs_nl, last_bs;
    u32 min_fs, max_fs;
    u64 total;
    MD5 md5;
    u8 md5sum[16];
    Vec<u64> foff;   // frame offsets relative to audio start
    Vec<u64> fsamp;  // first sample of frame
    Vec<u32> fbs;    // frame block size
    u64 audio_start;
    void init() {
        nframes = 0;
        first_bs = 0;
        min_bs = min_bs_nl = 0xFFFFFFFF;
        max_bs = max_bs_nl = 0;
        last_bs = 0;
        min_fs = 0xFFFFFFFF;
        max_fs = 0;
        total = 0;
        md5.init();
        foff.n = fsamp.n = fbs.n = 0;
    }
    void add_frame(u64 off, u32 len, u32 bs) {
        if (nframes == 0) first_bs = bs;
        if (nframes > 0) {
            min_bs_nl = Min(min_bs_nl, last_bs);
            max_bs_nl = Max(max_bs_nl, last_bs);
        }
        last_bs = bs;
        min_bs = Min(min_bs, bs);
        max_bs = Max(max_bs, bs);
        min_fs = Min(min_fs, len);
        max_fs = Max(max_fs, len);
        foff.push(off - audio_start);
        fsamp.push(total);
        fbs.push(bs);
        total += bs;
        nframes++;
    }
    void add_pcm(s32** pcm, int nch, int bps, u32 n) {
        int bytes = (bps + 7) / 8;
        u8 tmp[4096];
        size_t k = 0;
        for (u32 i = 0; i < n; i++)
            for (int c = 0; c < nch; c++) {
                u32 v = (u32)pcm[c][i];
                for (int b = 0; b < bytes; b++) tmp[k++] = (u8)(v >> (8 * b));
                if (k > sizeof(tmp) - 32) {
                    md5.update(tmp, k);
                    k = 0;
                }
            }
        if (k) md5.update(tmp, k);
    }
    // index of the last frame starting at or before 'sample' (0 if there are no frames)
    u64 frame_containing(u64 sample) const {
        // binary search last frame with fsamp <= sample
        if (fsamp.n == 0) return 0;
        size_t lo = 0, hi = fsamp.n - 1;
        while (lo < hi) {
            size_t mid = (lo + hi + 1) / 2;
            if (fsamp[mid] <= sample)
                lo = mid;
            else
                hi = mid - 1;
        }
        return lo;
    }
};
