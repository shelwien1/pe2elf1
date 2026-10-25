// Bits: the reader the parse takes them from, the writer the walk puts them
// into, and the CRCs of the containers (aac2tsv's bitio.h, without the C++
// library: an overrun is an error, err.inc, that the caller's trap takes).

namespace aacpack {

// bit p of buf, the first buf[0]'s most significant
INLINE unsigned bit_at(const uint8_t* buf, size_t p) { return (buf[p >> 3] >> (7 - (p & 7))) & 1; }

// MSB first over the bits [pos, end) of buf, which stays readable for 8
// bytes past the byte that holds bit end - 1 (its owner pads it)
struct BitReader {
  const uint8_t* buf;
  size_t pos, end;

  void Init(const uint8_t* b, size_t start_bit, size_t end_bit) {
    buf = b;
    pos = start_bit;
    end = end_bit;
  }
  size_t left() const { return end - pos; }

  // the 64 bits from pos (past end, whatever follows in buf)
  INLINE uint64_t peek64() const {
    const uint8_t* p = buf + (pos >> 3);
    uint64_t v = (uint64_t)p[0] << 56 | (uint64_t)p[1] << 48 | (uint64_t)p[2] << 40 | (uint64_t)p[3] << 32 |
                 (uint64_t)p[4] << 24 | (uint64_t)p[5] << 16 | (uint64_t)p[6] << 8 | (uint64_t)p[7];
    return v << (pos & 7);
  }
  // n <= 32
  INLINE uint32_t peek(int n) const { return n ? (uint32_t)(peek64() >> (64 - n)) : 0; }
  INLINE void skip(size_t n) {
    if (UNLIKELY(n > end - pos)) err_raise_s("bitstream overrun");
    pos += n;
  }
  // whether the n bits from pos are not just the zeros to a byte (more than
  // 7, or a 1 among them)
  INLINE bool not_zero_pad(size_t n) const { return n >= 8 || (n && peek((int)n)); }
  // n <= 32
  INLINE uint32_t get(int n) {
    if (!n) return 0;
    if (UNLIKELY((size_t)n > end - pos)) err_raise_s("bitstream overrun");
    uint32_t v = peek(n);
    pos += n;
    return v;
  }
  INLINE uint32_t get1() { return get(1); }
};

// MSB first into a buffer that grows (its bytes past pos are zero), up to
// cap bits: a walk that writes from values a corrupt stream decoded must
// not run on, so that more is an error
struct BitWriter {
  Buf<uint8_t> b;
  size_t pos;
  size_t cap;

  void Init() {
    b.Init();
    pos = 0;
    cap = (size_t)1 << 40;
  }
  void Quit() { b.Quit(); }
  void clear() {
    if (b.n) memset(b.p, 0, Min(b.n, (pos + 7) / 8 + 8));
    pos = 0;
  }
  // the bits to the next byte
  int to_byte() const { return (int)((8 - pos % 8) % 8); }
  // room for n bits more (and 8 bytes past them, zero)
  INLINE void reserve_bits(size_t n) {
    if (UNLIKELY(n > cap - Min(cap, pos))) err_raise_s("a unit written past its largest size");
    size_t need = (pos + n + 7) / 8 + 8;
    if (UNLIKELY(need > b.n)) b.resize(need > 2 * b.n ? need : 2 * b.n);
  }
  // the low n bits of v (n <= 32)
  INLINE void put(uint32_t v, int n) {
    if (!n) return;
    reserve_bits((size_t)n);
    if (n < 32) v &= (1u << n) - 1;
    uint8_t* p = b.p + (pos >> 3);
    int off = (int)(pos & 7);
    uint64_t x = (uint64_t)v << (64 - n - off);
    int nb = (off + n + 7) >> 3;
    for (int i = 0; i < nb; i++) p[i] |= (uint8_t)(x >> (56 - 8 * i));
    pos += n;
  }
  // nbits of src from bit from
  void put_range(const uint8_t* src, size_t from, size_t nbits) {
    if (!nbits) return;
    reserve_bits(nbits);
    if (!(from & 7) && !(pos & 7)) {
      size_t k = nbits >> 3;
      memcpy(b.p + (pos >> 3), src + (from >> 3), k);
      pos += k * 8;
      from += k * 8;
      nbits &= 7;
    }
    while (nbits) {
      int n = nbits > 24 ? 24 : (int)nbits;
      uint32_t v = 0;
      for (int i = 0; i < n; i++) v = v << 1 | bit_at(src, from + i);  // (src may be unpadded: bit by bit)
      put(v, n);
      from += n;
      nbits -= n;
    }
  }
  void append(const BitWriter& o) { put_range(o.b.p, 0, o.pos); }
  // n bits at bit position at, overwritten (n <= 32)
  void patch(size_t at, uint32_t v, int n) {
    for (int i = 0; i < n; i++) {
      size_t p = at + i;
      uint8_t m = (uint8_t)(0x80 >> (p & 7));
      if ((v >> (n - 1 - i)) & 1)
        b.p[p >> 3] |= m;
      else
        b.p[p >> 3] &= (uint8_t)~m;
    }
  }
  size_t bytes() const { return (pos + 7) >> 3; }
  const uint8_t* data() const { return b.p; }
};

// CRC-16, x^16 + x^15 + x^2 + 1 (ADTS), over nbits of buf from bit from
static uint16_t crc16_bits(uint16_t crc, const uint8_t* buf, size_t from, size_t nbits) {
  for (size_t i = 0; i < nbits; i++) {
    unsigned t = bit_at(buf, from + i) ^ (crc >> 15);
    crc = (uint16_t)(crc << 1);
    if (t) crc ^= 0x8005;
  }
  return crc;
}
static uint16_t crc16_zeros(uint16_t crc, size_t nbits) {
  for (size_t i = 0; i < nbits; i++) {
    unsigned t = crc >> 15;
    crc = (uint16_t)(crc << 1);
    if (t) crc ^= 0x8005;
  }
  return crc;
}

// CRC-10, x^10 + x^9 + x^5 + x^4 + x + 1 (SBR bs_sbr_crc_bits), from 0
static unsigned crc10_bits(const uint8_t* buf, size_t from, size_t nbits) {
  unsigned crc = 0;
  for (size_t i = 0; i < nbits; i++) {
    unsigned t = bit_at(buf, from + i) ^ ((crc >> 9) & 1);
    crc = (crc << 1) & 0x3ff;
    if (t) crc ^= 0x233;
  }
  return crc;
}

// CRC-32 (zlib's) of the stream, which the compressed stream ends with:
// its table made as the program starts
struct Crc32Tab {
  uint32_t t[256];
  void Init() {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
      t[i] = c;
    }
  }
};
static Crc32Tab crc32_make() {
  Crc32Tab t;
  t.Init();
  return t;
}
static const Crc32Tab kCrc32 = crc32_make();
struct Crc32 {
  uint32_t v;  // (inverted)
  void Init() { v = 0xffffffffu; }
  void add(const uint8_t* p, size_t n) {
    uint32_t c = v;
    for (size_t i = 0; i < n; i++) c = kCrc32.t[(c ^ p[i]) & 0xff] ^ (c >> 8);
    v = c;
  }
  uint32_t value() const { return ~v; }
};

}  // namespace aacpack
