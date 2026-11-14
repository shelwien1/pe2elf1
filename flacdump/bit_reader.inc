// flacdump: BitReader - MSB-first bit reader for FLAC frames
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct BitReader {
    const u8* d;  // must be followed by >= 16 readable bytes of padding
    u64 nbits, pos;
    void init(const u8* p, size_t nbytes) {
        d = p;
        nbits = (u64)nbytes * 8;
        pos = 0;
    }
    inline u64 peek() const {
        const u8* q = d + (pos >> 3);
        u64 v = ((u64)q[0] << 56) | ((u64)q[1] << 48) | ((u64)q[2] << 40) | ((u64)q[3] << 32) | ((u64)q[4] << 24) |
                ((u64)q[5] << 16) | ((u64)q[6] << 8) | (u64)q[7];
        return v << (pos & 7);
    }
    inline u32 get(int n) {  // n <= 32
        if (!n) return 0;
        u64 v = peek() >> (64 - n);
        pos += n;
        return (u32)v;
    }
    inline u64 get64(int n) {  // n <= 64
        if (n <= 32) return get(n);
        u64 hi = get(n - 32);
        return (hi << 32) | get(32);
    }
    inline s64 gets(int n) {
        if (!n) return 0;
        u64 v = get64(n);
        if (n < 64) {
            u64 m = 1ull << (n - 1);
            v = (v ^ m) - m;
        }
        return (s64)v;
    }
    inline bool unary(u64& out) {
        u64 c = 0;
        for (;;) {
            u64 v = peek();
            if (v) {
                int z = clz64(v);
                pos += z + 1;
                out = c + z;
                return pos <= nbits;
            }
            c += 57;
            pos += 57;
            if (pos > nbits) return false;
        }
    }
    bool ok() const { return pos <= nbits; }
};
