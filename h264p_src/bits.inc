// Bit-level reading and writing of RBSP data (emulation prevention removed).

namespace h264p {

// What cannot be parsed (a syntax error or a feature not supported), or
// written (the values a channel gives are inconsistent), raises an error
// (err.inc): the compressor's dry run then stores the NAL unit verbatim.

// bit i of the bits at p, the first p[0]'s most significant; and the bits
// before bit n in its byte, as the low bits of a byte (0 if none)
INLINE int bit_at(const uint8_t* p, size_t i) { return p[i >> 3] >> (7 - (i & 7)) & 1; }
INLINE uint8_t head_bits(const uint8_t* p, size_t n) { return (n & 7) ? (uint8_t)(p[n >> 3] >> (8 - (n & 7))) : 0; }

struct BitReader {
  void Init(const uint8_t* data, size_t size) {
    data_ = data;
    nbits_ = size * 8;
    pos_ = 0;
    // position of the last 1 bit, the rbsp_stop_one_bit
    last_one_ = -1;
    for (size_t i = size; i-- > 0;)
      if (data[i]) {
        int b = 0;
        while (!((data[i] >> b) & 1)) b++;
        last_one_ = (int64_t)(i * 8 + 7 - b);
        break;
      }
  }
  size_t pos() const { return pos_; }
  size_t size() const { return nbits_; }
  bool byte_aligned() const { return (pos_ & 7) == 0; }
  bool more_rbsp_data() const { return (int64_t)pos_ < last_one_; }

  int bit() {
    if (pos_ >= nbits_) err_raise("unexpected end of NAL unit");
    int b = bit_at(data_, pos_);
    pos_++;
    return b;
  }
  uint32_t u(int n) {
    uint32_t v = 0;
    while (n-- > 0) v = (v << 1) | bit();
    return v;
  }
  // next n <= 32 bits without consuming them, zeros past the end
  uint32_t peek(int n) const {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
      size_t p = pos_ + i;
      int b = p < nbits_ ? bit_at(data_, p) : 0;
      v = (v << 1) | b;
    }
    return v;
  }
  void skip(int n) {
    if (pos_ + n > nbits_) err_raise("unexpected end of NAL unit");
    pos_ += n;
  }
  uint32_t ue() {
    int lz = 0;
    while (!bit())
      if (++lz > 31) err_raise("invalid Exp-Golomb code");
    return lz ? (uint32_t)((1ull << lz) - 1 + u(lz)) : 0;
  }
  int64_t se() {
    uint32_t k = ue();
    return (k & 1) ? (int64_t)(k / 2) + 1 : -(int64_t)(k / 2);
  }

  const uint8_t* data_;
  size_t nbits_, pos_;
  int64_t last_one_;
};

struct BitWriter {
  void Init() {
    buf_.Init();
    padded_.Init();
    cur_ = 0;
    n_ = 0;
  }
  void Quit() {
    buf_.Quit();
    padded_.Quit();
  }
  // nothing written (the room stays)
  void clear() {
    buf_.clear();
    cur_ = 0;
    n_ = 0;
  }
  // the first nbits bits of p, as if written
  void set(const uint8_t* p, size_t nbits) {
    buf_.assign(p, nbits / 8);
    cur_ = head_bits(p, nbits);
    n_ = nbits;
  }
  // the bits o wrote
  void copy(const BitWriter& o) {
    buf_.copy(o.buf_);
    cur_ = o.cur_;
    n_ = o.n_;
  }
  // whether these bits are the first nbits bits of p
  bool same(const uint8_t* p, size_t nbits) const {
    return n_ == nbits && buf_.same(p, nbits / 8) && (!(nbits & 7) || cur_ == head_bits(p, nbits));
  }

  void bit(int b) {
    cur_ = (uint8_t)(cur_ << 1 | b);
    if ((++n_ & 7) == 0) {
      buf_.push_back(cur_);
      cur_ = 0;
    }
  }
  void put(uint64_t v, int n) {
    for (int i = n - 1; i >= 0; i--) bit((int)((v >> i) & 1));
  }
  void ue(uint64_t v) {
    uint64_t x = v + 1;
    int len = 0;
    while ((x >> len) > 1) len++;
    put(0, len);
    put(x, len + 1);
  }
  void se(int64_t v) { ue(v > 0 ? 2 * (uint64_t)v - 1 : 2 * (uint64_t)(-v)); }
  size_t pos() const { return n_; }
  int at(size_t i) const {
    if (i >> 3 < buf_.size()) return bit_at(buf_.data(), i);
    return (cur_ >> ((n_ & 7) - 1 - (i & 7))) & 1;
  }
  void truncate(size_t nbits) {
    if (nbits >= n_) return;
    if (nbits >> 3 == buf_.size())  // within the incomplete byte
      cur_ = (uint8_t)(cur_ >> ((n_ & 7) - (nbits & 7)));
    else
      cur_ = head_bits(buf_.data(), nbits);
    buf_.resize(nbits >> 3);
    n_ = nbits;
  }
  // whole bytes, the last one padded with zero bits
  const Buf<uint8_t>& data() {
    if (n_ & 7) {
      padded_.copy(buf_);
      padded_.push_back((uint8_t)(cur_ << (8 - (n_ & 7))));
      return padded_;
    }
    return buf_;
  }
  // appends whole bytes; the bit position must be byte aligned
  void append(size_t n, uint8_t value) { buf_.append(n, value), n_ += 8 * n; }

 private:
  friend struct CabacEncoder;  // (which writes whole bytes, and carries into them)
  Buf<uint8_t> buf_, padded_;  // complete bytes
  uint8_t cur_;                // the bits of the incomplete byte
  size_t n_;
};

}  // namespace h264p
