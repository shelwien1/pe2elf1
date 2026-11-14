// flacdump: Buf - growable byte buffer (heap)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct Buf {
    u8* p = nullptr;
    size_t n = 0, cap = 0;
    void reserve(size_t c) {
        if (c <= cap) return;
        size_t nc = cap ? cap : 4096;
        while (nc < c) nc *= 2;
        p = (u8*)xrealloc(p, nc);
        cap = nc;
    }
    inline void put(u8 b) {
        if (n == cap) reserve(n + 1);
        p[n++] = b;
    }
    void append(const u8* s, size_t k) {
        reserve(n + k);
        memcpy(p + n, s, k);
        n += k;
    }
    void release() {
        free(p);
        p = nullptr;
        n = cap = 0;
    }
};
