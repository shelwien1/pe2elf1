// flacdump: MD5 (RFC 1321), for the STREAMINFO signature
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct MD5 {
    u32 h[4];
    u8 buf[64];
    u64 len;
    void init() {
        h[0] = 0x67452301;
        h[1] = 0xefcdab89;
        h[2] = 0x98badcfe;
        h[3] = 0x10325476;
        len = 0;
    }
    static inline u32 rol(u32 x, int c) { return (x << c) | (x >> (32 - c)); }
    void block(const u8* p) {
        static const u32 K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
        static const int R[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5, 9,  14, 20, 5, 9,
                                  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                  4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
        u32 w[16];
        for (int i = 0; i < 16; i++) w[i] = rd_le32(p + i * 4);
        u32 a = h[0], b = h[1], c = h[2], d = h[3];
        for (int i = 0; i < 64; i++) {
            u32 f;
            int g;
            if (i < 16) {
                f = (b & c) | (~b & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | (~d & c);
                g = (5 * i + 1) & 15;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3 * i + 5) & 15;
            } else {
                f = c ^ (b | ~d);
                g = (7 * i) & 15;
            }
            u32 t = d;
            d = c;
            c = b;
            b = b + rol(a + f + K[i] + w[g], R[i]);
            a = t;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
    }
    void update(const u8* p, size_t n) {
        size_t have = (size_t)(len & 63);
        len += n;
        if (have) {
            size_t k = 64 - have;
            if (k > n) k = n;
            memcpy(buf + have, p, k);
            p += k;
            n -= k;
            have += k;
            if (have < 64) return;
            block(buf);
        }
        while (n >= 64) {
            block(p);
            p += 64;
            n -= 64;
        }
        if (n) memcpy(buf, p, n);
    }
    void final(u8 out[16]) {
        u64 bits = len * 8;
        u8 pad = 0x80;
        update(&pad, 1);
        u8 z = 0;
        while ((len & 63) != 56) update(&z, 1);
        u8 lb[8];
        for (int i = 0; i < 8; i++) lb[i] = (u8)(bits >> (8 * i));
        update(lb, 8);
        for (int i = 0; i < 4; i++) wr_le32(out + 4 * i, h[i]);
    }
};
