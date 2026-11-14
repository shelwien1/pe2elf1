// flacdump: BitWriter - MSB-first bit writer into a FixOut
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct BitWriter {
    FixOut* b;
    u64 acc;
    int na;
    void init(FixOut* buf) {
        b = buf;
        acc = 0;
        na = 0;
    }
    inline void put(u32 v, int n) {  // n <= 32, v < 2^n
        if (!n) return;
        acc = (acc << n) | v;
        na += n;
        while (na >= 8) {
            na -= 8;
            b->put((u8)(acc >> na));
        }
    }
    inline void put64(u64 v, int n) {
        if (n > 32) {
            put((u32)(v >> 32) & (u32)((1ull << (n - 32)) - 1), n - 32);
            put((u32)v, 32);
        } else if (n) {
            put((u32)(v & ((1ull << n) - 1)), n);
        }
    }
    inline void puts(s64 v, int n) { put64((u64)v, n); }
    inline void zeros(u64 q) {
        while (q >= 32) {
            put(0, 32);
            q -= 32;
        }
        put(0, (int)q);
    }
    inline void rice(u64 u, int k) {
        zeros(u >> k);
        if (k < 32)
            put((u32)((1ull << k) | (u & ((1ull << k) - 1))), k + 1);
        else {
            put(1, 1);
            put64(u, k);
        }
    }
    void align() {
        if (na) put(0, 8 - na);
    }
};
