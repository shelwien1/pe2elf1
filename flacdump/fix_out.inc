// flacdump: FixOut - output into fixed storage; overflow is remembered
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// output into fixed storage; overflow is remembered instead of reallocating
struct FixOut {
    u8* p;
    size_t n, cap;
    bool over;
    inline void put(u8 b) {
        if (n < cap)
            p[n++] = b;
        else
            over = true;
    }
    void append(const u8* s, size_t k) {
        if (n + k > cap) {
            over = true;
            return;
        }
        memcpy(p + n, s, k);
        n += k;
    }
};
