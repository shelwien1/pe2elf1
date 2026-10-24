/*
 * aac2tsv - bit reader/writer, CRCs
 *
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */

/* Bitstream syntax error while parsing AAC (c mode): the unit being parsed
 * is stored raw instead. */
struct ParseError {
    const char *msg;
};

[[noreturn]] static void parse_fail(const char *msg)
{
    throw ParseError{ msg };
}

/* MSB-first reader over [pos, end) bits of buf. buf must stay readable for
 * 8 bytes past the byte holding bit end-1 (callers pad their buffers). */
struct BitReader {
    const uint8_t *buf = nullptr;
    size_t pos = 0, end = 0;

    BitReader() {}
    BitReader(const uint8_t *b, size_t start_bit, size_t end_bit) : buf(b), pos(start_bit), end(end_bit) {}

    size_t left() const { return end - pos; }

    /* 64 bits starting at pos; bits past end are whatever follows in buf */
    uint64_t peek64() const
    {
        const uint8_t *p = buf + (pos >> 3);
        uint64_t v = (uint64_t)p[0] << 56 | (uint64_t)p[1] << 48 | (uint64_t)p[2] << 40 |
                     (uint64_t)p[3] << 32 | (uint64_t)p[4] << 24 | (uint64_t)p[5] << 16 |
                     (uint64_t)p[6] << 8 | (uint64_t)p[7];
        return v << (pos & 7);
    }
    /* n <= 32 */
    uint32_t peek(int n) const { return n ? (uint32_t)(peek64() >> (64 - n)) : 0; }

    void skip(size_t n)
    {
        if (n > end - pos)
            parse_fail("bitstream overrun");
        pos += n;
    }
    /* n <= 32 */
    uint32_t get(int n)
    {
        if (!n)
            return 0;
        if ((size_t)n > end - pos)
            parse_fail("bitstream overrun");
        uint32_t v = peek(n);
        pos += n;
        return v;
    }
    uint32_t get1() { return get(1); }
};

/* MSB-first writer, grows as needed */
struct BitWriter {
    std::vector<uint8_t> b;
    size_t pos = 0;

    void clear()
    {
        b.clear();
        pos = 0;
    }
    void reserve_bits(size_t n)
    {
        size_t need = (pos + n + 7) / 8 + 8;
        if (need > b.size())
            b.resize(need > 2 * b.size() ? need : 2 * b.size(), 0);
    }
    /* n <= 32, v < 2^n */
    void put(uint32_t v, int n)
    {
        if (!n)
            return;
        reserve_bits(n);
        while (n > 0) {
            int room = 8 - (int)(pos & 7);
            int take = n < room ? n : room;
            uint32_t bits = (v >> (n - take)) & ((1u << take) - 1);
            b[pos >> 3] |= (uint8_t)(bits << (room - take));
            pos += take;
            n -= take;
        }
    }
    /* copy nbits from src starting at bit from */
    void put_range(const uint8_t *src, size_t from, size_t nbits)
    {
        reserve_bits(nbits);
        if (!(from & 7) && !(pos & 7)) {
            memcpy(&b[pos >> 3], src + (from >> 3), nbits >> 3);
            pos += nbits & ~(size_t)7;
            from += nbits & ~(size_t)7;
            nbits &= 7;
        }
        while (nbits) {
            int n = nbits > 24 ? 24 : (int)nbits;
            uint32_t v = 0;
            for (int i = 0; i < n; i++) /* src may be unpadded: bitwise */
                v = v << 1 | ((src[(from + i) >> 3] >> (7 - ((from + i) & 7))) & 1);
            put(v, n);
            from += n;
            nbits -= n;
        }
    }
    void append(const BitWriter &o) { put_range(o.b.data(), 0, o.pos); }
    /* overwrite n bits at bit position at (n <= 32) */
    void patch(size_t at, uint32_t v, int n)
    {
        for (int i = 0; i < n; i++) {
            size_t p = at + i;
            uint8_t m = (uint8_t)(0x80 >> (p & 7));
            if ((v >> (n - 1 - i)) & 1)
                b[p >> 3] |= m;
            else
                b[p >> 3] &= (uint8_t)~m;
        }
    }
    size_t bytes() const { return (pos + 7) >> 3; }
};

static inline unsigned bit_at(const uint8_t *buf, size_t p)
{
    return (buf[p >> 3] >> (7 - (p & 7))) & 1;
}

/* CRC-16, x^16 + x^15 + x^2 + 1 (ADTS), over nbits of buf from bit from */
static uint16_t crc16_bits(uint16_t crc, const uint8_t *buf, size_t from, size_t nbits)
{
    for (size_t i = 0; i < nbits; i++) {
        unsigned t = bit_at(buf, from + i) ^ (crc >> 15);
        crc = (uint16_t)(crc << 1);
        if (t)
            crc ^= 0x8005;
    }
    return crc;
}

static uint16_t crc16_zeros(uint16_t crc, size_t nbits)
{
    for (size_t i = 0; i < nbits; i++) {
        unsigned t = crc >> 15;
        crc = (uint16_t)(crc << 1);
        if (t)
            crc ^= 0x8005;
    }
    return crc;
}

/* CRC-10, x^10 + x^9 + x^5 + x^4 + x + 1 (SBR bs_sbr_crc_bits), init 0 */
static unsigned crc10_bits(const uint8_t *buf, size_t from, size_t nbits)
{
    unsigned crc = 0;
    for (size_t i = 0; i < nbits; i++) {
        unsigned t = bit_at(buf, from + i) ^ ((crc >> 9) & 1);
        crc = (crc << 1) & 0x3ff;
        if (t)
            crc ^= 0x233;
    }
    return crc;
}

/* CRC-32 (zlib) of the whole reconstructed file, for the eof record */
static uint32_t crc32_update(uint32_t crc, const uint8_t *p, size_t n)
{
    static uint32_t tab[256];
    static bool init;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
            tab[i] = c;
        }
        init = true;
    }
    crc = ~crc;
    while (n--)
        crc = tab[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    return ~crc;
}
