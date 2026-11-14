// flacdump: seek table helpers
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct SeekModels {
    BM kind[3][4];
    BM match[4];
    BM mode[4];
    UModel dk;
};

static u64 frame_containing(const FrameStats& fs, u64 sample) {
    // binary search last frame with fsamp <= sample
    if (fs.fsamp.n == 0) return 0;
    size_t lo = 0, hi = fs.fsamp.n - 1;
    while (lo < hi) {
        size_t mid = (lo + hi + 1) / 2;
        if (fs.fsamp[mid] <= sample)
            lo = mid;
        else
            hi = mid - 1;
    }
    return lo;
}
